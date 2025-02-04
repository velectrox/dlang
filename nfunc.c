#include <inttypes.h>
#include "dlang.h"

void _push(atom_t *n, atom_t *at)
{
	at->next = n->data;
	n->data = at;
}

int _rm(atom_t **n, atom_t *del_n)
{
	while(*n != NULL) {
		if(*n == del_n) {
			*n = del_n->next;
			free(del_n);
			return 1;
		} else {
			n = &(*n)->next;
		}
	}
	return 0;
}
/********************************************/
atom_t * _pop(atom_t **n)
{
	atom_t *old = *n;

	*n = (*n)->next;
	return old;
}

atom_t * _add(atom_t *list)
{
	atom_t *ret = malloc(sizeof(*ret));
	ret->type = ATOM_INTEGER;
	ret->data = eval(list)->data;

	while(list = list->next)
	        *((uint64_t *) &ret->data) += (uint64_t) eval(list)->data;

	return ret;
}
atom_t * _car(atom_t *list)
{
	atom_t *ret;
        ret = eval(list)->data;

	return ret;
}
atom_t * _printf(atom_t *list)
{
	atom_t *ret = malloc(sizeof(*ret));
	ret->type = ATOM_INTEGER;

	char *fmt = eval(list)->data;
	list = list->next;

	void *data = eval(list)->data;
	ret->data = (void *)(uintptr_t) printf(fmt, data);

	return ret;

}
atom_t *_setq(atom_t *list)
{
	atom_t *ret;

	if (list->type != ATOM_SYM) {
		printf("_setq error: expecting ATOM_SYM but received %d\n",
		       list->type);
		ret->data = (void *)-1;
	}
	ret = eval(list->next);
        hash_put(symhashtab, list->data, ret, sizeof(atom_t));

	return ret;
}
atom_t *_setf(atom_t *list)
{
	atom_t *ret;
	atom_t *set2;

	set2 = eval(list->next);
	ret = eval(list);

	ret->data = set2->data;
	ret->type = set2->type;

	return ret;
}

atom_t *_eval(atom_t *list)
{
	atom_t *ret;

	ret = eval(eval(list));

	return ret;
}

atom_t *_defun(atom_t *list)
{
	struct lfunc *lf = malloc(sizeof(*lf));
	atom_t *lf_atom = malloc(sizeof(*lf_atom));

	lf->args = ((atom_t *)(list->next->data));
	lf->code = list->next->next;

	lf_atom->type = ATOM_LFUNC;
	lf_atom->data = lf;

        hash_put(symhashtab, list->data, lf_atom, sizeof(*lf_atom));

	return lf_atom;
}

atom_t *_lambda(atom_t *list)
{
	struct lfunc *lf = malloc(sizeof(*lf));
	atom_t *lf_atom = malloc(sizeof(*lf_atom));

	lf->args = ((atom_t *)(list->data));
	lf->code = list->next;

	lf_atom->type = ATOM_LFUNC;
	lf_atom->data = lf;

	return lf_atom;
}

atom_t *_if (atom_t *list)
{
	atom_t *ret;

	if (eval(list)->data)
		ret = eval(list->next);
	else
		ret = eval(list->next->next);

	return ret;

}
atom_t *_eq (atom_t *list)
{
	atom_t *ret = malloc(sizeof(*ret));
	atom_t *arg1,*arg2;

	arg1 = eval(list);
	arg2 = eval(list->next);

	/*For nil*/
	if(arg1 == arg2)
		ret->data = (void *) 1;
	else if(arg1 == NULL || arg2 == NULL)
		ret->data = (void *) 0;
	else if(arg1->type == ATOM_INTEGER)
		ret->data = (void *)(uintptr_t) (arg1->data == arg2->data);
	else if(arg1->type == ATOM_STRING)
		ret->data = (void *)(uintptr_t) !strcmp(arg1->data, arg2->data);

	/* free(arg1); */
	/* free(arg2); */

	ret->type = ATOM_INTEGER;

	return ret;

}

atom_t * _sub(atom_t *list)
{
	atom_t *ret = malloc(sizeof(*ret));
	ret->type = ATOM_INTEGER;
	ret->data = (void *) eval(list)->data;

	while(list = list->next)
	        *((uint64_t *) &ret->data) -= (uint64_t) eval(list)->data;

	return ret;
}


atom_t * _mult(atom_t *list)
{
	atom_t *ret = malloc(sizeof(*ret));
	ret->type = ATOM_INTEGER;
	ret->data = (void *) eval(list)->data;

	while(list = list->next)
	        *((uint64_t *) &ret->data) *= (uint64_t) eval(list)->data;

	return ret;
}

atom_t * _cons(atom_t *list)
{
	atom_t *ret = malloc(sizeof(*ret));
	atom_t *new = malloc(sizeof(*new));

	/* data and type copied here */
	*new = *eval(list);
	new->next = ((atom_t *) (eval(list->next))->data);

	ret->type = LIST;
	ret->data = new;
	ret->next = 0;

	return ret;
}
atom_t * _cdr(atom_t *list)
{
	atom_t *ret = malloc(sizeof(*ret));
        ret->type = LIST;
        ret->data = ((atom_t *)(eval(list)->data))->next;
        ret->next = 0;

	return ret;
}
