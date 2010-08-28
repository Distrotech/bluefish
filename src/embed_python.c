/* Bluefish HTML Editor
 * embed_python.c - embedded python functionality
 *
 * Copyright (C) 2005-2008 Oliver Sessink
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#ifdef HAVE_PYTHON

/* #define DEBUG */

#include <Python.h>
#include <gtk/gtk.h>

#include "bluefish.h"
#include "document.h"

static PyObject *GetObjectFromBluefishModule(char *name) {
	PyObject* BluefishMod, *PyDict;
	BluefishMod = PyImport_AddModule("bluefish");
	PyDict = PyModule_GetDict(BluefishMod);
	return PyDict_GetItemString(PyDict, name);
}

/* Bluefish - Python bluefish.document type */
typedef struct {
	PyObject_HEAD
	/* Type-specific fields go here. */
	Tdocument *doc;
} bluefish_DocumentObject;

/*static PyObject *Document_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	bluefish_DocumentObject *self;
	self = (bluefish_DocumentObject *)type->tp_alloc(type, 0);
	if (self != NULL) {
		/ * parse arguments ?? * /
		if (!self->doc) {
			Py_DECREF(self);
			return NULL;
		}
		return (PyObject *)self;
	}
	return NULL;
}*/
static int Document_init(bluefish_DocumentObject *self, PyObject *args, PyObject *kwds) {
	GFile *uri;
	static char *kwlist[] = {"uri", NULL};
	PyObject *PyBfwin;
	Tbfwin *bfwin;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|s", kwlist, &uri))
		return -1;

	PyBfwin = GetObjectFromBluefishModule("bfwin");
	bfwin = PyCObject_AsVoidPtr(PyBfwin);

	if (uri) {
		Tdocument *tmpdoc;
		tmpdoc = documentlist_return_document_from_uri(bfwin->documentlist, uri);
		if (!tmpdoc) return -1;
#ifdef DEBUG
		gchar *fullpath = g_file_get_parse_name (uri);
		g_print("found %p for %s\n",tmpdoc,fullpath);
		g_free(fullpath);
#endif
		self->doc = tmpdoc;
	} else {
		self->doc = bfwin->current_document;
	}
	return 0;
}

static PyObject *Document_getchars(bluefish_DocumentObject *self, PyObject *args) {
	int ok;
	long start,end;
	
	ok = PyArg_ParseTuple(args, "ll", &start, &end);
	if (ok) {
		PyObject *pName;
		gchar *buf;
		buf = doc_get_chars(self->doc, start, end);
		pName = PyString_FromString(buf);
		g_free(buf);
		return pName;
	}
	return PyString_FromString("");
}

static PyObject * Document_replace(bluefish_DocumentObject *self, PyObject *args) {
	int ok;
	long start,end;
	const char *string;
	ok = PyArg_ParseTuple(args, "lls", &start, &end, &string);
	if (ok) {
		doc_replace_text(self->doc,string,start,end);
	}
	return PyString_FromString("");
}



static PyMethodDef Document_methods[] = {
	{"getchars", (PyCFunction)Document_getchars, METH_VARARGS,"return characters"},
	{"replace", (PyCFunction)Document_replace, METH_VARARGS,"return characters"},
	{NULL}  /* Sentinel */
};

static PyTypeObject bluefish_DocumentType = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"bluefish.Document",             /*tp_name*/
	sizeof(bluefish_DocumentObject), /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	0,                         /*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	0,                         /*tp_repr*/
	0,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	0,                         /*tp_as_mapping*/
	0,                         /*tp_hash */
	0,                         /*tp_call*/
	0,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,        /*tp_flags*/
	"Document object",           /* tp_doc */
	0,		               /* tp_traverse */
	0,		               /* tp_clear */
	0,		               /* tp_richcompare */
	0,		               /* tp_weaklistoffset */
	0,		               /* tp_iter */
	0,		               /* tp_iternext */
	Document_methods,             /* tp_methods */
	NULL,             /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)Document_init,      /* tp_init */
	0,                         /* tp_alloc */
	NULL,                 /* tp_new */
};

/* Bluefish - python API */

/*static PyObject * bluefish_get_curdoc(PyObject *self, PyObject *args) {
	PyObject *PyBfwin;
	Tbfwin *bfwin;
	PyBfwin = GetObjectFromBluefishModule("bfwin");
	if (!PyBfwin) return NULL;
	bfwin = PyCObject_AsVoidPtr(PyBfwin);
	if (!bfwin) return NULL;
	
}*/

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
	
	bluefish_DocumentType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&bluefish_DocumentType) >= 0)
        PyModule_AddObject(BluefishMod, "Document", (PyObject *)&bluefish_DocumentType);

	fp = fopen(filename, "r");
	PyRun_SimpleFile(fp, filename);
	fclose(fp);
	Py_Finalize();
}
#endif /* HAVE_PYTHON */
