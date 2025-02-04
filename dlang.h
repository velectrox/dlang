#ifndef _DLANG_H
#define _DLANG_H

#include <inttypes.h>
#include "lispy/lispy.h"
extern hash_t *symhashtab;

typedef struct atom{
	void *data;
	uint8_t type;
	struct atom *next;
}atom_t;

struct lfunc {
	atom_t *args;
	atom_t *code;
};

atom_t * eval(atom_t *at);

enum {
	LIST = 0,
	QUOTE,
	ATOM_LFUNC,
	ATOM_INTEGER,
	ATOM_STRING,
	ATOM_NFUNC,
	ATOM_SYM,
	NTYPES
};


#endif
