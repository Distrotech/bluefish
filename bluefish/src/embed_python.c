#include "config.h"
#ifdef HAVE_PYTHON

#include <Python.h>
#include <gtk/gtk.h>

#include "bluefish.h"
#include "document.h"

/* Bluefish - python API */

static PyObject * bluefish_curdoc_getchars(PyObject *self, PyObject *args) {
	int ok;
	Tbfwin *bfwin;
	long start,end;
	ok = PyArg_ParseTuple(args, "Oll", &bfwin, &start, &end);
	if (bfwin->current_document) {
		PyObject *pName;
		gchar *buf;
		buf = doc_get_chars(bfwin->current_document, start, end);
		pName = PyString_FromString(buf);
		g_free(buf);
		return pName;
	}
	return PyString_FromString("");
}

static PyObject * bluefish_curdoc_replace(PyObject *self, PyObject *args) {
	int ok;
	Tbfwin *bfwin;
	long start,end;
	const char *string;
	ok = PyArg_ParseTuple(args, "Olls", &bfwin, &start, &end, &string);
	if (bfwin->current_document) {
		doc_replace_text(bfwin->current_document,string,start,end);
	}
	return Py_BuildValue("");
}

static PyMethodDef BluefishMethods[] = {
	{"curdoc_getchars", bluefish_curdoc_getchars, METH_VARARGS,""},
	{"curdoc_replace", bluefish_curdoc_replace, METH_VARARGS,""},
	{NULL, NULL, 0, NULL}
};

/* run the interpreter */

void pythonRun(Tbfwin *bfwin, gchar *filename) {
	PyObject *PyBfwin, *BluefishMod;
	
	if (!bfwin) return;
	if (!filename) return;
	
	Py_Initialize();
	PyBfwin = PyCObject_FromVoidPtr(bfwin,NULL);
	BluefishMod = PyImport_AddModule("bluefish");
	Py_InitModule("bluefish", BluefishMethods);
	PyModule_AddObject(BluefishMod, "bfwin", PyBfwin);
	PyRun_AnyFile(NULL, filename);
	/* what can PySys_SetObject() do ?? */
	/* perhaps we should use PyRun_File because a pointer globals 
	can be passed to that function */
	Py_Finalize();
}
#endif /* HAVE_PYTHON */
