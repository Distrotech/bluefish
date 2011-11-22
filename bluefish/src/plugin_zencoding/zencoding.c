/* Bluefish HTML Editor
 * zencoding.c -
 *
 * Copyright (C) 2011 Olivier Sessink
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

#include <Python.h>
#include <gtk/gtk.h>

#include "zencoding.h"
#include "zeneditor-interface.h"
#include "../config.h"
#include "../bluefish.h"
#include "../plugins.h"
#include "../bfwin.h"

Tzencoding zencoding = {NULL, NULL};

static gboolean
init_python(void) {
	PyObject *mod/*, *interface*/;
	
	g_print("init_python()\n");
	Py_Initialize();
	PyRun_SimpleString("import sys");
	g_print("set zencoding path to "PKGDATADIR"/plugins/zencoding/\n");
	PyRun_SimpleString("sys.path.append('"PKGDATADIR"/plugins/zencoding/')");
	zencoding.module = PyImport_ImportModule("zencoding");
	if (!zencoding.module) {
		if (PyErr_Occurred()) PyErr_Print();
		g_print("did not found zencoding, abort\n");
		return FALSE;
	}
	/* now call my own module */
	mod = zeneditor_module_init();
	if (!mod) {
		if (PyErr_Occurred()) PyErr_Print();
		g_print("failed to initialize zeneditor-interface module\n");
		return FALSE;
	}
	
	zencoding.editor = PyObject_CallMethod(mod, "zeneditor", NULL);
	if (!zencoding.editor) {
		if (PyErr_Occurred()) PyErr_Print();
		g_print("failed to get editor interface\n");
		return FALSE;
	}
	
	/*interface = PyObject_GetAttrString(mod, "zeneditor");
	if (!interface) {
		if (PyErr_Occurred()) PyErr_Print();
		g_print("failed to get zeneditor interface module\n");
		return FALSE;
	}
	zencoding.editor = PyObject_CallObject(interface, NULL);
	if (!zencoding.editor) {
		if (PyErr_Occurred()) PyErr_Print();
		g_print("failed to get editor interface\n");
		return FALSE;
	}
	Py_XDECREF(interface);*/
	return TRUE;
}

static void
zencoding_curdocchanged_cb(Tbfwin *bfwin, Tdocument *olddoc, Tdocument *newdoc, gpointer data) {
	PyObject *ptr, *result;
	
	g_print("zencoding_curdocchanged_cb, started for newdoc %p\n",newdoc);
	
	if (!zencoding.module || !zencoding.editor) {
		return;
	}
	
	if (!newdoc) {
		g_print("zencoding_curdocchanged_cb, no newdoc\n");
		return;
	}
	ptr = PyLong_FromVoidPtr((void *) newdoc);
	g_print("calling set_context for document %p wrapped in python object %p\n",newdoc, ptr);
	result = PyObject_CallMethod(zencoding.editor, "set_context", "O", ptr);
	if (!result) {
		if (PyErr_Occurred()) PyErr_Print();
		g_print("failed to call set_context()\n");
		return;
	}
	Py_XDECREF(result);
	Py_XDECREF(ptr);
}
/*******************************************************************/
/*   GUI callbacks */
/*******************************************************************/

static void
zencoding_expand_cb(GtkAction *action, gpointer user_data) {
	Tbfwin *bfwin = user_data;
	PyObject *result;
	
	if (!zencoding.module || !zencoding.editor) {
		if (!init_python())
			return;
		zencoding_curdocchanged_cb(bfwin, NULL, bfwin->current_document, NULL);
	}
	g_print("zencoding_expand_cb, calling run_action expand_abbreviation\n");
	result = PyObject_CallMethod(zencoding.module, "run_action", "sO", "expand_abbreviation", zencoding.editor);
	if (!result) {
		if (PyErr_Occurred()) PyErr_Print();
		g_print("failed to call run_action()\n");
	}
	Py_XDECREF(result);
}

/*******************************************************************/
/*   GUI */
/*******************************************************************/

static const gchar *zencoding_plugin_ui =
	"<ui>"
		"<menubar name='MainMenu'>"
			"<menu action='ZencodingMenu'>"
				"<menuitem action='ExpandZenCoding'/>"
			"</menu>"
		"</menubar>"
	"</ui>";

static void
zencoding_initgui(Tbfwin * bfwin)
{
	bfwin_current_document_change_register(bfwin, zencoding_curdocchanged_cb, NULL);
	
	GtkActionGroup *action_group;
	GError *error = NULL;

	static const GtkActionEntry zencoding_actions[] = {
		{"ZencodingMenu", NULL, N_("_Zencoding")},
		{"ExpandZenCoding", NULL, N_("Expand zen-coding"), NULL,
		 N_("Expand zen-code at cursor"), G_CALLBACK(zencoding_expand_cb)}
	};

	action_group = gtk_action_group_new("zencodingActions");
	gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE "_plugin_zencoding");
	gtk_action_group_add_actions(action_group, zencoding_actions, G_N_ELEMENTS(zencoding_actions), bfwin);
	gtk_ui_manager_insert_action_group(bfwin->uimanager, action_group, 0);
	g_object_unref(action_group);

	gtk_ui_manager_add_ui_from_string(bfwin->uimanager, zencoding_plugin_ui, -1, &error);
	if (error != NULL) {
		g_warning("building zencoding plugin menu failed: %s", error->message);
		g_error_free(error);
	}
}

/*******************************************************************/
/*   general plugin functions below */
/*******************************************************************/

static void
zencoding_init(void)
{
#ifdef ENABLE_NLS
	DEBUG_MSG("zencoding_init, gettext domain-name=%s\n", PACKAGE "_plugin_zencoding");
	bindtextdomain(PACKAGE "_plugin_zencoding", LOCALEDIR);
	bind_textdomain_codeset(PACKAGE "_plugin_zencoding", "UTF-8");
#endif							/* ENABLE_NLS */
}


static void
zencoding_enforce_session(Tbfwin * bfwin)
{
}

static void
zencoding_cleanup(void)
{

}

static void
zencoding_cleanup_gui(Tbfwin * bfwin)
{
}

static GHashTable *
zencoding_register_globses_config(GHashTable * configlist)
{
	return configlist;
}

static GHashTable *
zencoding_register_session_config(GHashTable * configlist, Tsessionvars * session)
{
	return configlist;
}

static void
zencoding_session_cleanup(Tsessionvars * session)
{
}

static TBluefishPlugin bfplugin = {
	"zencoding",
	BFPLUGIN_VERSION,
	GTK_MAJOR_VERSION,
	sizeof(Tdocument),
	sizeof(Tsessionvars),
	sizeof(Tglobalsession),
	sizeof(Tbfwin),
	sizeof(Tproject),
	sizeof(Tmain),
	sizeof(Tproperties),
	BFPLUGIN_PRIORITY_DEFAULT,
	0,
	NULL,						/* private */
	zencoding_init,				/* init */
	zencoding_initgui,
	zencoding_enforce_session,
	zencoding_cleanup,
	zencoding_cleanup_gui,
	zencoding_register_globses_config,
	zencoding_register_session_config,
	zencoding_session_cleanup,
	NULL,						/* binary compatibility */
	NULL,
	NULL
};

G_MODULE_EXPORT TBluefishPlugin *
getplugin(void)
{
	return &bfplugin;
}
