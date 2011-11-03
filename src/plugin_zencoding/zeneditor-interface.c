/*
 * zeneditor-interface.c
 *
 * Copyright (C) 2011 Olivier Sessink
 *
 */

#include <Python.h>

typedef struct {
	PyObject_HEAD
	gchar *profile;
	Tdocument *context;
} Tzeneditor;

static PyObject *
zeneditor_set_context(Tzeneditor *self, PyObject *args)
{
	PyObject *context = NULL, *tmp;
	if (PyArg_ParseTuple(args, "O", &context)) {
		self->context = context;
	}
	Py_RETURN_NONE;
}

static PyObject *
zeneditor_get_selection_range(Tzeneditor *self, PyObject *args)
{
	PyObject *result;
	gint start, end;
	doc_get_selection(DOCUMENT(self->context), &start, &end);
	result = Py_BuildValue("(ii)", start, end);
	return result;
}

static PyObject *
zeneditor_create_selection(Tzeneditor *self, PyObject *args)
{
	gint start = -1, end = -1;
	if (PyArg_ParseTuple(args, "i|i", &start, &end)) {
		/* documentation describes that the end may be undefined, in which case only the cursor is set */
		GtkTextIter itstart, itend;
		gtk_text_buffer_get_iter_at_offset(DOCUMENT(self->context)->buffer, &itstart, start);
		if (end != -1) {
			gtk_text_buffer_get_iter_at_offset(DOCUMENT(self->context)->buffer, &itend, end);
			gtk_text_buffer_select_range(DOCUMENT(self->context)->buffer,&itstart,&itend);
		} else {
			gtk_text_buffer_place_cursor(DOCUMENT(self->context)->buffer,&itstart);
		}
	}
	Py_RETURN_NONE;
}

static PyObject *
zeneditor_get_current_line_range(Tzeneditor *self, PyObject *args)
{
	PyObject *result;
	GtkTextIter itstart, itend;

	gtk_text_buffer_get_iter_at_mark(DOCUMENT(self->context)->buffer, &itstart, 
				gtk_text_buffer_get_insert(DOCUMENT(self->context)->buffer));
	itend = itstart;
	gtk_text_iter_set_line_offset(&itstart, 0);
	gtk_text_iter_forward_to_line_end(&itend);
	result = Py_BuildValue("ii", gtk_text_iter_get_offset(&itstart), gtk_text_iter_get_offset(&itend));
	return result;
}

static PyObject *
zeneditor_get_caret_pos(Tzeneditor *self, PyObject *args)
{
	PyObject *result;
	GtkTextIter itstart;
	gtk_text_buffer_get_iter_at_mark(DOCUMENT(self->context)->buffer, &itstart, 
				gtk_text_buffer_get_insert(DOCUMENT(self->context)->buffer));
	result = Py_BuildValue("i", gtk_text_iter_get_offset(&itstart));
	return result;
}

static PyObject *
zeneditor_set_caret_pos(Tzeneditor *self, PyObject *args)
{
	gint offset;
	if (PyArg_ParseTuple(args, "i", &offset)) {
		GtkTextIter itstart;
		gtk_text_buffer_get_iter_at_offset(DOCUMENT(self->context)->buffer, &itstart, offset);
		gtk_text_buffer_place_cursor(DOCUMENT(self->context)->buffer,&itstart);
	}

	Py_RETURN_NONE;
}


static PyObject *
zeneditor_get_current_line(Tzeneditor *self, PyObject *args)
{
	PyObject *result;
	GtkTextIter itstart, itend;
	gchar *text;

	gtk_text_buffer_get_iter_at_mark(DOCUMENT(self->context)->buffer, &itstart, 
				gtk_text_buffer_get_insert(DOCUMENT(self->context)->buffer));
	itend = itstart;
	gtk_text_iter_set_line_offset(&itstart, 0);
	gtk_text_iter_forward_to_line_end(&itend);

	text = gtk_text_buffer_get_text(DOCUMENT(self->context)->buffer, &itstart, &itend, TRUE);
	result = Py_BuildValue("s", text);
	g_free(text);
	return result;
}

static inline const gchar *
get_caret_placeholder(PyObject *mod) {
	PyObject *pcaret_placeholder = PyObject_GetAttrString(mod, "caret_placeholder");
	return = (const gchar *)PyString_AsString(pcaret_placeholder);
}


static PyObject *
zeneditor_replace_content(Tzeneditor *self, PyObject *args)
{
	PyObject *result;
	gchar *text;
	gint start=-1, end=-1;
	if (PyArg_ParseTuple(args, "s|ii", &text, &start, &end))	{
		gchar *tmp1, *tmp2;
		const gchar *placeholder;
		/* the documentation states that start end end may be undefined
		if both are -1 all text should be replaced (for gtk start=0, end=-1),
		if only end is -1 we insert starting at the current cursor position
		
		we also have to check if there is a caret placeholder in 'text' */
		placeholder = get_caret_placeholder(self->mod);
		tmp2 = g_strstr_len(text, placeholder, -1);
		tmp1 = g_strndup(text, tmp2-text+strlen(placeholder));
		
		if (start >= 0 && end == -1) {
			doc_insert_two_strings(self->context, tmp1, tmp2);
		} else {
			gchar *complete;
			GtkTextIter itstart;
			if (start == -1) {
				start = 0;
			}
			complete = g_strconcat(tmp1, tmp2, NULL);
			doc_replace_text(self->context, complete, start, end);
			g_free(complete);
			gtk_text_buffer_get_iter_at_offset(DOCUMENT(self->context)->buffer, &itstart, tmp2-text);
			gtk_text_buffer_place_cursor(DOCUMENT(self->context)->buffer,&itstart);
		}
		g_free(tmp1);
	} else {
		g_warning("zeneditor_replace_content error\n");
	}
	Py_RETURN_NONE;
}

static PyObject *
zeneditor_get_content(Tzeneditor *self, PyObject *args)
{
	PyObject *result;
	gchar *text;
	text = doc_get_chars(self->context, 0, -1);
	result = PyString_FromString(text);
	g_free(text);
	return result;
}

static PyObject *
zeneditor_get_syntax(Tzeneditor *self, PyObject *args)
{
	/* TODO: which syntaxes does zeneditor support ?? */
	return PyString_FromString("html");
}

static PyObject *
zeneditor_get_profile_name(Tzeneditor *self, PyObject *args)
{
	if (self->profile) {
		return PyString_FromString(self->profile);
	}
	return PyString_FromString("html");
}
static PyObject *
zeneditor_prompt(Tzeneditor *self, PyObject *args)
{
	PyObject *result;
	gchar *title = NULL;

	if (PyArg_ParseTuple(args, "s", &title))
		GtkWidget *win, *entry;
		const gchar *retval=NULL;
		win = gtk_dialog_new_with_buttons(title, GTK_WINDOW(BFWIN(DOCUMENT(self->context)->bfwin)->main_window),
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
					GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,NULL);
		gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
		gtk_window_set_default_size(GTK_WINDOW(dialog), 350, -1);

		entry = gtk_entry_new();
		gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), input, TRUE, TRUE, 0);
		gtk_widget_show_all(win);
	
		if (gtk_dialog_run(GTK_DIALOG(win)) == GTK_RESPONSE_ACCEPT)
			retval = gtk_entry_get_text(GTK_ENTRY(entry));
		if (!retval || retval[0]=='\0') {
			g_free(retval);
			Py_RETURN_NONE;
		}
		result = PyString_FromString(retval);
		gtk_widget_destroy(dialog);
		return result
	}
	Py_RETURN_NONE;
}

static PyObject *
zeneditor_get_selection(Tzeneditor *self, PyObject *args)
{
	PyObject *result;
	gint start,end;
	if (doc_get_selection(self->context, &start, &end)) {
		text = doc_get_chars(self->context, start, end);
		result = Py_BuildValue("s", text);
		g_free(text);
		return result;
	}
	Py_RETURN_NONE;
}

static PyObject *
zeneditor_get_file_path(Tzeneditor *self, PyObject *args)
{
	PyObject *result;
	gchar *path = g_file_get_path(DOCUMENT(self->context)->uri);
	result = Py_BuildValue("s", path);
	g_free(path);
	return result;
}

/*static PyObject *
zeneditor_set_profile_name(Tzeneditor *self, PyObject *args)
{
	gchar *profile = NULL;
	PyObject *tmp, *temp_profile = NULL;

	if (PyArg_ParseTuple(args, "s", &profile)) {
			if (self->profile)
				g_free(self->profile);
			self->profile = g_strdup(profile);
		}
	}
	Py_RETURN_NONE;
}
*/

static PyObject *
zeneditor_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Tzeneditor *self;
	self = (zeneditor *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->profile = g_strdup("xhtml");
		self->context = NULL;
	}
	return (PyObject *) self;
}

static int
zeneditor_init(Tzeneditor *self, PyObject *args, PyObject *kwds)
{
	PyObject *context = NULL;
	PyObject *profile = NULL;
	PyObject *tmp, *mod, *caret_ph;
	const gchar *ph;
	static gchar *kwlist[] = { "profile", "context", NULL };

	if (!self->profile)
		self->profile = g_strdup("xhtml");
	self->context = NULL;

	if (PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kwlist,
			&context, &profile)) {
		if (context) {
			self->context = PyLong_AsVoidPtr(context);
		}
		if (profile) {
			self->profile = PyString_AsString(profile);
		}
	}

	mod = PyImport_ImportModule("zencoding.utils");
	if (mod == NULL)
	{
		if (PyErr_Occurred())
			PyErr_Print();
		return -1;
	}
	Py_XDECREF(mod);
	return 0;
}

static PyMethodDef zeneditor_methods[] = {
	{"create_selection", (PyCFunction)zeneditor_create_selection, METH_VARARGS},
	{"get_caret_pos", (PyCFunction)zeneditor_get_caret_pos, METH_VARARGS},
	{"get_content", (PyCFunction)zeneditor_get_content, METH_VARARGS},
	{"get_current_line", (PyCFunction)zeneditor_get_current_line, METH_VARARGS},
	{"get_current_line_range", (PyCFunction)zeneditor_get_current_line_range, METH_VARARGS},
	{"get_file_path", (PyCFunction)zeneditor_get_file_path, METH_VARARGS},
	{"get_profile_name", (PyCFunction)zeneditor_get_profile_name, METH_VARARGS},
	{"get_selection", (PyCFunction)zeneditor_get_selection, METH_VARARGS},
	{"get_selection_range", (PyCFunction)zeneditor_get_selection_range, METH_VARARGS},
	{"get_syntax", (PyCFunction)zeneditor_get_syntax, METH_VARARGS},
	{"prompt", (PyCFunction)zeneditor_prompt, METH_VARARGS},
	{"replace_content", (PyCFunction)zeneditor_replace_content, METH_VARARGS},
	{"set_caret_pos", (PyCFunction)zeneditor_set_caret_pos, METH_VARARGS},
	{"set_context", (PyCFunction)zeneditor_set_context, METH_VARARGS},
	/*{"set_profile_name", (PyCFunction)zeneditor_set_profile_name, METH_VARARGS},*/
	{NULL}
};

static PyTypeObject zeneditorType = {
	PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "bluefish.zeneditor",         /*tp_name*/
    sizeof(zeneditor),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)zeneditor_dealloc, /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "bluefish zeneditor interface",    /* tp_doc */
    0,		                   /* tp_traverse */
    0,		               	   /* tp_clear */
    0,		                   /* tp_richcompare */
    0,		                   /* tp_weaklistoffset */
    0,		                   /* tp_iter */
    0,		                   /* tp_iternext */
    zeneditor_methods,         /* tp_methods */
    0, 						        /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)zeneditor_init,  /* tp_init */
    0,                         /* tp_alloc */
    zeneditor_new,             /* tp_new */

};

PyMethodDef Module_methods[] = { { NULL } };


PyObject *zeneditor_module_init(void) {
	PyObject *m;

	zeneditorType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&zeneditorType) < 0)
		return;

	m = Py_InitModule3("bluefish_zeneditor", Module_methods, "Bluefish zeneditor interface");

	Py_INCREF(&zeneditorType);
	PyModule_AddObject(m, "zeneditor", (PyObject *) &zeneditorType);

	return m;
}
