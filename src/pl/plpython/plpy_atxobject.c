/*
 * the PLyAutonomous class
 *
 * src/pl/plpython/plpy_atxobject.c
 */

#include "postgres.h"

#include "access/xact.h"
#include "executor/spi.h"
#include "utils/memutils.h"

#include "plpython.h"

#include "plpy_atxobject.h"

#include "plpy_elog.h"


static void PLy_atx_dealloc(PyObject *subxact);
static PyObject *PLy_atx_enter(PyObject *self, PyObject *unused);
static PyObject *PLy_atx_exit(PyObject *self, PyObject *args);

static char PLy_atx_doc[] = {
	"PostgreSQL autonomous transaction context manager"
};

static PyMethodDef PLy_atx_methods[] = {
	{"__enter__", PLy_atx_enter, METH_VARARGS, NULL},
	{"__exit__", PLy_atx_exit, METH_VARARGS, NULL},
	/* user-friendly names for Python <2.6 */
	{"enter", PLy_atx_enter, METH_VARARGS, NULL},
	{"exit", PLy_atx_exit, METH_VARARGS, NULL},
	{NULL, NULL, 0, NULL}
};

static PyTypeObject PLy_AtxType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"PLyAtx",		/* tp_name */
	sizeof(PLyAtxObject),	/* tp_size */
	0,							/* tp_itemsize */

	/*
	 * methods
	 */
	PLy_atx_dealloc, /* tp_dealloc */
	0,							/* tp_print */
	0,							/* tp_getattr */
	0,							/* tp_setattr */
	0,							/* tp_compare */
	0,							/* tp_repr */
	0,							/* tp_as_number */
	0,							/* tp_as_sequence */
	0,							/* tp_as_mapping */
	0,							/* tp_hash */
	0,							/* tp_call */
	0,							/* tp_str */
	0,							/* tp_getattro */
	0,							/* tp_setattro */
	0,							/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,	/* tp_flags */
	PLy_atx_doc,		/* tp_doc */
	0,							/* tp_traverse */
	0,							/* tp_clear */
	0,							/* tp_richcompare */
	0,							/* tp_weaklistoffset */
	0,							/* tp_iter */
	0,							/* tp_iternext */
	PLy_atx_methods, /* tp_tpmethods */
};


void
PLy_atx_init_type(void)
{
	if (PyType_Ready(&PLy_AtxType) < 0)
		elog(ERROR, "could not initialize PLy_AtxType");
}

/* s = plpy.atx() */
PyObject *
PLy_atx_new(PyObject *self, PyObject *unused)
{
	PLyAtxObject *ob;

	ob = PyObject_New(PLyAtxObject, &PLy_AtxType);

	if (ob == NULL)
		return NULL;

	return (PyObject *) ob;
}

/* Python requires a dealloc function to be defined */
static void
PLy_atx_dealloc(PyObject *atx)
{
}

/*
 * atx.__enter__() or atx.enter()
 *
 * Start an atx.
 */
static PyObject *
PLy_atx_enter(PyObject *self, PyObject *unused)
{
	PLyAtxObject *atx = (PLyAtxObject *) self;

	SuspendTransaction();

	Py_INCREF(self);
	return self;
}

/*
 * atx.__exit__(exc_type, exc, tb) or atx.exit(exc_type, exc, tb)
 *
 * Exit an autonomous transaction. exc_type is an exception type, exc
 * is the exception object, tb is the traceback.  If exc_type is None,
 * commit the atx, if not abort it.
 *
 * The method signature is chosen to allow atx objects to
 * be used as context managers as described in
 * <http://www.python.org/dev/peps/pep-0343/>.
 */
static PyObject *
PLy_atx_exit(PyObject *self, PyObject *args)
{
	PyObject   *type;
	PyObject   *value;
	PyObject   *traceback;
	PLyAtxObject *atx = (PLyAtxObject *) self;

	if (!PyArg_ParseTuple(args, "OOO", &type, &value, &traceback))
		return NULL;

	if (type != Py_None)
	{
		AbortCurrentTransaction();
	}
	else
	{
		CommitTransactionCommand();
	}

	Py_INCREF(Py_None);
	return Py_None;
}
