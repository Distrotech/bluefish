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

/*#define DEBUG*/

#include <Python.h>
#include <gtk/gtk.h>

#include "zencoding.h"
#include "zeneditor-interface.h"
#include "../config.h"
#include "../bluefish.h"
#include "../plugins.h"
#include "../bfwin.h"

Tzencoding zencoding = {NULL, NULL, NULL};

static gboolean
init_python(void) {
	DEBUG_MSG("init_python()\n");
	Py_Initialize();
	PyRun_SimpleString("import sys");
	DEBUG_MSG("set zencoding path to "PKGDATADIR"/plugins/\n");
	PyRun_SimpleString("sys.path.append('"PKGDATADIR"/plugins/')");
	zencoding.module = PyImport_ImportModule("zencoding");
	if (!zencoding.module) {
		if (PyErr_Occurred()) PyErr_Print();
		DEBUG_MSG("did not found zencoding, abort\n");
		return FALSE;
	}
	/* now call my own module */
	zencoding.zenmod = zeneditor_module_init();
	if (!zencoding.zenmod) {
		if (PyErr_Occurred()) PyErr_Print();
		DEBUG_MSG("failed to initialize zeneditor-interface module\n");
		return FALSE;
	}
	
	zencoding.editor = PyObject_CallMethod(zencoding.zenmod, "zeneditor", NULL);
	if (!zencoding.editor) {
		if (PyErr_Occurred()) PyErr_Print();
		DEBUG_MSG("failed to get editor interface\n");
		return FALSE;
	}

	return TRUE;
}

static void
zencoding_run_action(Tdocument *doc, const gchar *action_name)
{
	PyObject *result;
	PyObject *ptr;
	if (!zencoding.module || !zencoding.editor) {
		if (!init_python())
			return;
	}
	ptr = PyLong_FromVoidPtr((void *) doc);
	DEBUG_MSG("zencoding_run_action, calling set_context for document %p wrapped in python object %p\n",doc, ptr);
	result = PyObject_CallMethod(zencoding.editor, "set_context", "O", ptr);
	if (!result) {
		if (PyErr_Occurred()) PyErr_Print();
		DEBUG_MSG("zencoding_run_action, failed to call set_context()\n");
		return;
	}
	Py_XDECREF(result);
	Py_DECREF(ptr);
	result = PyObject_CallMethod(zencoding.module, "run_action", "sO", action_name, zencoding.editor);
	if (!result) {
		if (PyErr_Occurred()) PyErr_Print();
		DEBUG_MSG("failed to call run_action(%s)\n",action_name);
	}
	Py_XDECREF(result);
}


/*******************************************************************/
/*   GUI callbacks */
/*******************************************************************/

static void
zencoding_expand_cb(GtkAction *action, gpointer user_data) {
	zencoding_run_action(BFWIN(user_data)->current_document, "expand_abbreviation");
}
static void
zencoding_matchpairinward_cb(GtkAction *action, gpointer user_data) {
	zencoding_run_action(BFWIN(user_data)->current_document, "match_pair_inward");
}
static void
zencoding_matchpairoutward_cb(GtkAction *action, gpointer user_data) {
	zencoding_run_action(BFWIN(user_data)->current_document, "match_pair_outward");
}
static void
zencoding_wrapwithabbreviation_cb(GtkAction *action, gpointer user_data) {
	zencoding_run_action(BFWIN(user_data)->current_document, "wrap_with_abbreviation");
}
static void
zencoding_splitjointag_cb(GtkAction *action, gpointer user_data) {
	zencoding_run_action(BFWIN(user_data)->current_document, "split_join_tag");
}
static void
zencoding_preveditpoint_cb(GtkAction *action, gpointer user_data) {
	zencoding_run_action(BFWIN(user_data)->current_document, "prev_edit_point");
}
static void
zencoding_nexteditpoint_cb(GtkAction *action, gpointer user_data) {
	zencoding_run_action(BFWIN(user_data)->current_document, "next_edit_point");
}
static void
zencoding_insertformattednewline_cb(GtkAction *action, gpointer user_data) {
	zencoding_run_action(BFWIN(user_data)->current_document, "insert_formatted_newline");
}
static void
zencoding_gotomatchingpair_cb(GtkAction *action, gpointer user_data) {
	zencoding_run_action(BFWIN(user_data)->current_document, "go_to_matching_pair");
}
static void
zencoding_mergelines_cb(GtkAction *action, gpointer user_data) {
	zencoding_run_action(BFWIN(user_data)->current_document, "merge_lines");
}
static void
zencoding_removetag_cb(GtkAction *action, gpointer user_data) {
	zencoding_run_action(BFWIN(user_data)->current_document, "remove_tag");
}
static void
zencoding_evaluatemathexpression_cb(GtkAction *action, gpointer user_data) {
	zencoding_run_action(BFWIN(user_data)->current_document, "evaluate_math_expression");
}

/*******************************************************************/
/*   GUI */
/*******************************************************************/

static const gchar *zencoding_plugin_ui =
	"<ui>"
		"<menubar name='MainMenu'>"
			"<menu action='ZencodingMenu'>"
				"<menuitem action='ExpandZenCoding'/>"
				"<menuitem action='MatchPairInward'/>"
				"<menuitem action='MatchPairOutward'/>"
				"<menuitem action='WrapWithAbbreviation'/>"
				"<menuitem action='SplitJoinTag'/>"
				"<menuitem action='PrevEditPoint'/>"
				"<menuitem action='NextEditPoint'/>"
				"<menuitem action='InsertFormattedNewline'/>"
				"<menuitem action='GoToMatchingPair'/>"
				"<menuitem action='MergeLines'/>"
				"<menuitem action='RemoveTag'/>"
				"<menuitem action='EvaluateMathExpression'/>"
			"</menu>"
		"</menubar>"
	"</ui>";

static void
zencoding_initgui(Tbfwin * bfwin)
{
	GtkActionGroup *action_group;
	GError *error = NULL;

	static const GtkActionEntry zencoding_actions[] = {
		{"ZencodingMenu", NULL, N_("_Zencoding")},
		{"ExpandZenCoding", NULL, N_("E_xpand zen-coding"), "<shift><ctrl>Return",N_("Expand zen-code at cursor"), G_CALLBACK(zencoding_expand_cb)},
		{"MatchPairInward", NULL, N_("Select Tag _Content"), NULL,NULL, G_CALLBACK(zencoding_matchpairinward_cb)},
		{"MatchPairOutward", NULL, N_("Select _Tag and Content"), NULL,NULL, G_CALLBACK(zencoding_matchpairoutward_cb)},
		{"WrapWithAbbreviation", NULL, N_("Wrap with _Abbreviation"), NULL,NULL, G_CALLBACK(zencoding_wrapwithabbreviation_cb)},
		{"SplitJoinTag", NULL, N_("_Split or Join Tag"), NULL,NULL, G_CALLBACK(zencoding_splitjointag_cb)},
		{"PrevEditPoint", NULL, N_("_Previous Edit Point"), NULL,NULL, G_CALLBACK(zencoding_preveditpoint_cb)},
		{"NextEditPoint", NULL, N_("_Next Edit Point"), NULL,NULL, G_CALLBACK(zencoding_nexteditpoint_cb)},
		{"InsertFormattedNewline", NULL, N_("_Insert Formatted Newline"), NULL,NULL, G_CALLBACK(zencoding_insertformattednewline_cb)},
		{"GoToMatchingPair", NULL, N_("_Go to Matching Pair"), NULL,NULL, G_CALLBACK(zencoding_gotomatchingpair_cb)},
		{"MergeLines", NULL, N_("_Merge Lines"), NULL,NULL, G_CALLBACK(zencoding_mergelines_cb)},
		{"RemoveTag", NULL, N_("_Remove Tag"), NULL,NULL, G_CALLBACK(zencoding_removetag_cb)},
		{"EvaluateMathExpression", NULL, N_("_Evaluate Math Expression"), NULL,NULL, G_CALLBACK(zencoding_evaluatemathexpression_cb)}
		/*{"", NULL, N_(""), NULL,N_(""), G_CALLBACK(zencoding__cb)},*/
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


/*static void
zencoding_enforce_session(Tbfwin * bfwin)
{
}*/

static void
zencoding_cleanup(void)
{
	DEBUG_MSG("zencoding_cleanup\n");
	if (zencoding.module || zencoding.editor) {
		Py_XDECREF(zencoding.module);
		Py_XDECREF(zencoding.editor);
		/*Py_XDECREF(zencoding.zenmod); valgrind returns an error if we try to unref this*/
		zencoding.module = NULL;
		zencoding.editor = NULL;
		zencoding.zenmod = NULL;
		Py_Finalize();
	}
}

/*static void
zencoding_cleanup_gui(Tbfwin * bfwin)
{
}*/

/*static GHashTable *
zencoding_register_globses_config(GHashTable * configlist)
{
	return configlist;
}
*/
/*static GHashTable *
zencoding_register_session_config(GHashTable * configlist, Tsessionvars * session)
{
	return configlist;
}
*/
/*static void
zencoding_session_cleanup(Tsessionvars * session)
{
}
*/
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
	1,
	NULL,						/* private */
	zencoding_init,				/* init */
	zencoding_initgui,
	NULL /*zencoding_enforce_session*/,
	zencoding_cleanup,
	NULL /*zencoding_cleanup_gui*/,
	NULL /*zencoding_register_globses_config*/,
	NULL /*zencoding_register_session_config*/,
	NULL /*zencoding_session_cleanup*/,
	NULL,						/* binary compatibility */
	NULL,
	NULL
};

G_MODULE_EXPORT TBluefishPlugin *
getplugin(void)
{
	return &bfplugin;
}
