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

Tzencoding zencoding = {NULL};

static void
init_python(void) {
	PyObject *module;
	Py_Initialize();
	PyRun_SimpleString("import sys");
	g_print("set zencoding path to "PKGDATADIR"/plugins/zencoding/\n");
	PyRun_SimpleString("sys.path.append('"PKGDATADIR"/plugins/zencoding/')");
	module = PyImport_ImportModule("zencoding");
	if (!module) {
		if (PyErr_Occurred()) PyErr_Print();
		g_print("did not found zencoding, abort\n");
		return;
	}
	/* now call my own module */
	zencoding.module = zeneditor_module_init();
	if (!zencoding.module) {
		if (PyErr_Occurred()) PyErr_Print();
		return;
	}
}

static void
zencoding_curdocchanged_cb(Tbfwin *bfwin, Tdocument *olddoc, Tdocument *newdoc, gpointer data) {
	PyObject *cls, *editor, *set_context;
	PyObject *ptr, *result;
	
	g_print("zencoding_curdocchanged_cb, started for newdoc %p\n",newdoc);
	
	if (!zencoding.module)
		return;
	
	if (!newdoc) {
		g_print("zencoding_curdocchanged_cb, no newdoc\n");
		return;
	}
	
	cls = PyObject_GetAttrString(zencoding.module, "ZenEditor");
	if (!cls) {
		if (PyErr_Occurred()) PyErr_Print();
		g_print("failed to get ZenEditor\n");
		return;
	}

	editor = PyObject_CallObject(cls, NULL);
	if (!editor) {
		if (PyErr_Occurred()) PyErr_Print();
		g_print("failed to get editor\n");
		return;
	}

	ptr = PyLong_FromVoidPtr((void *) newdoc);
	result = PyObject_CallMethod(editor, "set_context", "O", ptr);
	if (!result) {
		if (PyErr_Occurred()) PyErr_Print();
		g_print("failed to call set_context()\n");
		return;
	}
	Py_XDECREF(result);
	Py_XDECREF(ptr);
	Py_XDECREF(editor);
	Py_XDECREF(cls);
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
	init_python();
	
}

/*static const gchar *zencoding_plugin_ui =
	"<ui>"
	"  <menubar name='MainMenu'>"
	"    <menu action='ToolsMenu'>"
	"      <separator/>"
	"      <menuitem action='XmlOpenIncludes'/>"
	"    </menu>"
	"  </menubar>"
	"</ui>";*/

static void
zencoding_initgui(Tbfwin * bfwin)
{
	bfwin_current_document_change_register(bfwin, zencoding_curdocchanged_cb, NULL);
	
/*	GtkActionGroup *action_group;
	GError *error = NULL;

	static const GtkActionEntry zencoding_actions[] = {
		{"XmlOpenIncludes", NULL, N_("Open XML includes"), NULL,
		 N_("Open included xml files"), G_CALLBACK(zencoding_open_included)}
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
	}*/
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
