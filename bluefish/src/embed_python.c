#include "config.h"
#ifdef HAVE_PYTHON

#define DEBUG

#include <Python.h>
#include <gtk/gtk.h>

#include "bluefish.h"
#include "document.h"

/* Bluefish - python API */

static PyObject *GetObjectFromBluefishModule(char *name) {
	PyObject* BluefishMod, *PyDict;
	BluefishMod = PyImport_AddModule("bluefish");
	PyDict = PyModule_GetDict(BluefishMod);
	return PyDict_GetItemString(PyDict, name);
}

static PyObject * bluefish_curdoc_getchars(PyObject *self, PyObject *args) {
	int ok;
	PyObject *PyBfwin;
	Tbfwin *bfwin;
	long start,end;
	
	ok = PyArg_ParseTuple(args, "ll", &start, &end);
	if (ok) {
		PyBfwin = GetObjectFromBluefishModule("bfwin");
		bfwin = PyCObject_AsVoidPtr(PyBfwin);
		DEBUG_MSG("bluefish_curdoc_getchars, bfwin=%p\n",bfwin);
		if (bfwin->current_document) {
			PyObject *pName;
			gchar *buf;
			buf = doc_get_chars(bfwin->current_document, start, end);
			pName = PyString_FromString(buf);
			g_free(buf);
			return pName;
		}
	}
	return PyString_FromString("");
}

static PyObject * bluefish_curdoc_replace(PyObject *self, PyObject *args) {
	int ok;
	PyObject *PyBfwin;
	Tbfwin *bfwin;
	long start,end;
	const char *string;
	ok = PyArg_ParseTuple(args, "lls", &start, &end, &string);
	if (ok) {
		PyBfwin = GetObjectFromBluefishModule("bfwin");
		if (!PyBfwin) {
			DEBUG_MSG("bluefish_curdoc_replace, noPyBfwin !!\n");
			return NULL;
		}
		bfwin = PyCObject_AsVoidPtr(PyBfwin);
		DEBUG_MSG("bluefish_curdoc_replace, bfwin=%p\n",bfwin);
		if (bfwin->current_document) {
			doc_replace_text(bfwin->current_document,string,start,end);
		}
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
	FILE *fp;
	
	if (!bfwin) return;
	if (!filename) return;
	DEBUG_MSG("pythonRun %s for bfwin=%p\n",filename,bfwin);
	Py_Initialize();
	PyBfwin = PyCObject_FromVoidPtr(bfwin,NULL);
	BluefishMod = PyImport_AddModule("bluefish");
	Py_InitModule("bluefish", BluefishMethods);

	PyModule_AddObject(BluefishMod, "bfwin", PyBfwin);

	fp = fopen(filename, "r");
	PyRun_SimpleFile(fp, filename);
	fclose(fp);

	Py_Finalize();
}
#endif /* HAVE_PYTHON */
