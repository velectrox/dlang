#include <inttypes.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <strings.h>
#include "lispy/lispy.h"
#include "dlang.h"
#include "nfunc.h"
static atom_t *prev[128];
static uint8_t nprev;

hash_t *symhashtab;
hash_t *localctx[128];
static uint8_t nctx;

void new_ctx(hash_t *new_ctx)
{
	localctx[++nctx] = new_ctx;
}
hash_t * current_ctx()
{
	return localctx[nctx];
}
void destroy_ctx()
{
	--nctx;
}
atom_t *lfunc_run(atom_t *lfunc)
{
	struct lfunc *lf;
	atom_t *in_args;
	atom_t *target_args;
	atom_t *code;
	atom_t *ret;
	hash_t *localctx;
	localctx = new_hash(1);

	lf = lfunc->data;
	target_args = lf->args;
	code = lf->code;
	in_args = lfunc->next;

	while(target_args) {
		hash_put(localctx, target_args->data, eval(in_args), sizeof(atom_t));
		in_args = in_args->next;
		target_args = target_args->next;
	}
	new_ctx(localctx);
	while(code) {
		ret = eval(code);
		code = code->next;
	}

	target_args = lf->args;
	in_args = lfunc->next;

	while(target_args) {
		hash_key_free(current_ctx(), target_args->data);
		target_args = target_args->next;
	}
	destroy_ctx();
	return ret;
}
atom_t *eval(atom_t *at)
{
	atom_t *current;
	atom_t *cursor = at;
	atom_t *ret;
	/* printf("%p\n", cursor); */
	switch (cursor->type) {
	case QUOTE:
		/* printf("Parsing QUOTE\n"); */
		ret = cursor->data;
		break;
	case LIST:
		/* printf("Parsing LIST\n"); */
		ret = eval(cursor->data);
		break;
	case ATOM_NFUNC:
		/* printf("Parsing NFUNC\n"); */
		ret = ((atom_t *(*)(atom_t *))cursor->data)(cursor->next);
		break;
	case ATOM_LFUNC:
		/* printf("Parsing LFUNC\n"); */
		ret = lfunc_run(cursor);
		break;
	case ATOM_INTEGER:
		/* printf("Parsing INTEGER\n"); */
		ret = cursor;
		break;
	case ATOM_STRING:
		/* printf("Parsing STRING\n"); */
		ret = cursor;
		break;
	case ATOM_SYM:
		/* printf("Parsing SYM %s\n", (char *)cursor->data); */
		if(!strcmp(cursor->data, "nil"))
			return NULL;

		/* Some symbols are aliases to other symbol,
		 * we need to resolve multiple times
		 */
	        ret = cursor;
		current = ret;
		while (current->type == ATOM_SYM) {
		        ret = hash_get_value(symhashtab, current->data);
			if(!ret && current_ctx()) {
				/* printf("Searching locally...\n"); */
			        ret = hash_get_value(current_ctx(), current->data);
			}
			current = ret;
			if(!current) {
				printf("Error no such symbol %s\n", (char *)cursor->data);
				return NULL;
			}
		}

		/* We treat as symbols functions at first to simplify parsing.
		 * When we find out that a symbol is a funcion, we replace it in-place
		 * In this case we need to eval again with their correct type ([NL]FUNC)
		 */
		if(ATOM_NFUNC == current->type || ATOM_LFUNC == current->type) {
			cursor->data = current->data;
			cursor->type = current->type;
			ret = eval(cursor);
		} else {
			ret = current;
		}
		break;
	default:
		printf("Error!\n");
	}
	return ret;
}

static void add_to_current(atom_t **current, atom_t *new)
{
	if(!*current)
		*current = malloc(sizeof(atom_t));

	**current = *new;
}
atom_t *pop_prev(void)
{
	return prev[--nprev];
}
void *push_prev(atom_t *at)
{
	return prev[nprev++] = at;
}

atom_t * parse_lisp(char *input)
{
	atom_t *head = NULL;
	atom_t **current = &head;
	atom_t new;
	char *endptr;
	uint8_t quoting = 0;
	bzero(&new, sizeof(new));
	while(*input) {
		switch(*input) {
		case '\'':
			new.type = QUOTE;
			new.data = malloc(sizeof(atom_t));
			add_to_current(current, &new);
			push_prev(*current);
			current = (atom_t **) &(*current)->data;
			quoting = 1;
			++input;
			break;
		case '(':
			new.type = LIST;
			new.data = NULL;
			add_to_current(current, &new);
			push_prev(*current);
			current = (atom_t **) &(*current)->data;
			/*We don't need to track the quotes if there are brackets*/
			if(quoting) {
				(void) pop_prev();
				quoting = 0;
			}
			++input;
			break;
		case ')':
			current = &pop_prev()->next;
			++input;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			new.type = ATOM_INTEGER;
			new.data = (void *)strtoll(input, &endptr, 10);
			add_to_current(current, &new);
			if(!quoting)
				current = &(*current)->next;
			else {
				current = &pop_prev()->next;
				quoting = 0;
			}
			input = endptr;
			break;
		case ' ':
		case '\n':
		case '\t':
			++input;
			break;
		case '\"':
			new.type = ATOM_STRING;
			new.data = malloc(1024);
			/*Saving now as we are going to advance the data ptr*/
			add_to_current(current, &new);
			++input;
			while(*input != '\"') {
				if(*input != '\\')
					*(char *)new.data++ = *input++;
				else {
					switch(*++input) {
					case 'n':
						*(char *)new.data++ = 0x0a;
						input++;
						break;
					}
				}
			}
			*(char *)new.data = '\0';

			if(!quoting)
				current = &(*current)->next;
			else {
				current = &pop_prev()->next;
				quoting = 0;
			}
			++input;
			break;
		default:
			new.type = ATOM_SYM;
			new.data = malloc(128);
			/*Saving now as we are going to advance the data ptr*/
			add_to_current(current, &new);

			while(*input != ' ' && *input != ')' &&
			      *input != '\0' && *input != '\n' &&
			      *input != '\t')
				*(char *)new.data++ = *input++;
			*(char *)new.data = '\0';

			if(!quoting)
				current = &(*current)->next;
			else {
				current = &pop_prev()->next;
				quoting = 0;
			}
			break;
		}
	}
	return head;
}
void run_prolog()
{
	atom_t func;

	symhashtab = new_hash(1);

	func.type = ATOM_NFUNC;
	func.data = _add;
	hash_put(symhashtab, "+", &func, sizeof(atom_t));

	func.type = ATOM_NFUNC;
	func.data = _car;
	hash_put(symhashtab, "car", &func, sizeof(atom_t));

	func.type = ATOM_NFUNC;
	func.data = _printf;
	hash_put(symhashtab, "printf", &func, sizeof(atom_t));

	func.type = ATOM_NFUNC;
	func.data = _setq;
	hash_put(symhashtab, "setq", &func, sizeof(atom_t));

	func.type = ATOM_NFUNC;
	func.data = _eval;
	hash_put(symhashtab, "eval", &func, sizeof(atom_t));

	func.type = ATOM_NFUNC;
	func.data = _defun;
	hash_put(symhashtab, "defun", &func, sizeof(atom_t));

	func.type = ATOM_NFUNC;
	func.data = _if;
	hash_put(symhashtab, "if", &func, sizeof(atom_t));

	func.type = ATOM_NFUNC;
	func.data = _eq;
	hash_put(symhashtab, "=", &func, sizeof(atom_t));

	func.type = ATOM_NFUNC;
	func.data = _mult;
	hash_put(symhashtab, "*", &func, sizeof(atom_t));

	func.type = ATOM_NFUNC;
	func.data = _sub;
	hash_put(symhashtab, "-", &func, sizeof(atom_t));

	func.type = ATOM_NFUNC;
	func.data = _cons;
	hash_put(symhashtab, "cons", &func, sizeof(atom_t));

	func.type = ATOM_NFUNC;
	func.data = _cdr;
	hash_put(symhashtab, "cdr", &func, sizeof(atom_t));

	func.type = ATOM_NFUNC;
	func.data = _setf;
	hash_put(symhashtab, "setf", &func, sizeof(atom_t));

	func.type = ATOM_NFUNC;
	func.data = _lambda;
	hash_put(symhashtab, "lambda", &func, sizeof(atom_t));

}
int main(int argc, char **argv)
{
	atom_t *list;
	int fd;
	char *code_file;

	fd = open(argv[1], O_RDONLY);
	code_file = mmap(NULL, 64*4096, PROT_READ, MAP_PRIVATE, fd, 0);

	run_prolog();

	list = parse_lisp(code_file);
	while(NULL != list) {
		atom_t *out = eval(list);
		if(!out) {
			printf("(dlang eval) => nil\n");
			goto next;
		}
		switch(out->type) {
		case ATOM_INTEGER:
			printf("(dlang eval) => %ld\n",(uint64_t) out->data);
			break;
		case LIST:
			printf("(dlang eval) => (");
			out = out->data;
			while(out) {
				printf("%ld ", (uint64_t) out->data);
				out = out->next;
			}
			printf("\b)\n");
		}
	next: list = list->next;
	}
}
