/* Bluefish HTML Editor
 * xmltools.c -
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


#include <gtk/gtk.h>

#include "xmltools.h"
#include "../config.h"
#include "../bluefish.h"
#include "../bf_lib.h"
#include "../dialog_utils.h"
#include "../document.h"
#include "../gtk_easy.h"
#include "../plugins.h"
#include "../undo_redo.h"

static void
xmltools_open_included (GtkAction *action, gpointer user_data)
{
	/* action code here ! */
}

static void
xmltools_init(void)
{
#ifdef ENABLE_NLS
	DEBUG_MSG("xmltools_init, gettext domain-name=%s\n", PACKAGE "_plugin_xmltools");
	bindtextdomain(PACKAGE "_plugin_xmltools", LOCALEDIR);
	bind_textdomain_codeset(PACKAGE "_plugin_xmltools", "UTF-8");
#endif							/* ENABLE_NLS */

}

static const gchar *xmltools_plugin_ui =
	"<ui>"
	"  <menubar name='MainMenu'>"
	"    <menu action='ToolsMenu'>"
	"      <separator/>"
	"      <menuitem action='XmlOpenIncludes'/>"
	"    </menu>"
	"  </menubar>"
	"</ui>";

static void
xmltools_initgui(Tbfwin * bfwin)
{
	GtkActionGroup *action_group;
	GError *error = NULL;

	static const GtkActionEntry xmltools_actions[] = {
		{"XmlOpenIncludes", NULL, N_("Open XML includes"), NULL,
		 N_("Open included xml files"), G_CALLBACK(xmltools_open_included)}
	};

	action_group = gtk_action_group_new("xmltoolsActions");
	gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE "_plugin_xmltools");
	gtk_action_group_add_actions(action_group, xmltools_actions, G_N_ELEMENTS(xmltools_actions), bfwin);
	gtk_ui_manager_insert_action_group(bfwin->uimanager, action_group, 0);
	g_object_unref(action_group);

	gtk_ui_manager_add_ui_from_string(bfwin->uimanager, xmltools_plugin_ui, -1, &error);
	if (error != NULL) {
		g_warning("building xmltools plugin menu failed: %s", error->message);
		g_error_free(error);
	}
}

static void
xmltools_enforce_session(Tbfwin * bfwin)
{
}

static void
xmltools_cleanup(void)
{

}

static void
xmltools_cleanup_gui(Tbfwin * bfwin)
{
}

static GHashTable *
xmltools_register_globses_config(GHashTable * configlist)
{
	return configlist;
}

static GHashTable *
xmltools_register_session_config(GHashTable * configlist, Tsessionvars * session)
{
	return configlist;
}

static void
xmltools_session_cleanup(Tsessionvars * session)
{
}

static TBluefishPlugin bfplugin = {
	"Xmltools",
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
	xmltools_init,				/* init */
	xmltools_initgui,
	xmltools_enforce_session,
	xmltools_cleanup,
	xmltools_cleanup_gui,
	xmltools_register_globses_config,
	xmltools_register_session_config,
	xmltools_session_cleanup,
	NULL,						/* binary compatibility */
	NULL,
	NULL
};

G_MODULE_EXPORT TBluefishPlugin *
getplugin(void)
{
	return &bfplugin;
}
