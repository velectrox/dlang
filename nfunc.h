#ifndef _NFUNC_H
#define _NFUNC_H

#include "dlang.h"

atom_t * _add(atom_t *list);
atom_t * _pop(atom_t **n);
atom_t * _car(atom_t *list);
atom_t * _printf(atom_t *list);
atom_t * _setq(atom_t *list);
atom_t * _eval(atom_t *list);
atom_t * _defun(atom_t *list);
atom_t * _if(atom_t *list);
atom_t * _eq(atom_t *list);
atom_t * _mult(atom_t *list);
atom_t * _sub(atom_t *list);
atom_t * _cons(atom_t *list);
atom_t * _cdr(atom_t *list);
atom_t * _setf(atom_t *list);
atom_t * _lambda(atom_t *list);
#endif
