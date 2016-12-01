/*
 * src/pl/plpython/plpy_atxobject.h
 */

#ifndef PLPY_ATXOBJECT
#define PLPY_ATXOBJECT

#include "nodes/pg_list.h"

typedef struct PLyAtxObject
{
	PyObject_HEAD
} PLyAtxObject;

extern void PLy_atx_init_type(void);
extern PyObject *PLy_atx_new(PyObject *self, PyObject *unused);

#endif   /* PLPY_ATXOBJECT */
