/* Bluefish HTML Editor
 * infbrowser.c - plugin for information browser
 *
 * Copyright (C) 2007 Oskar Świda
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
/* #define DEBUG */

#include <string.h>
#include "infbrowser.h"
#include <gmodule.h>
#include "../plugins.h"
#include "../rcfile.h"
#include "../document.h"
#include "../bf_lib.h"
#include "infb_gui.h"
#include "infb_load.h"


static GdkCursor *hand_cursor;
static GdkCursor *regular_cursor;


Tinfb infb_v;

void set_normal_cursor(GtkTextView *view) {
	gdk_window_set_cursor (gtk_text_view_get_window (GTK_TEXT_VIEW(view), GTK_TEXT_WINDOW_TEXT), regular_cursor);
}

void set_link_cursor(GtkTextView *view) {
	gdk_window_set_cursor (gtk_text_view_get_window (GTK_TEXT_VIEW(view), GTK_TEXT_WINDOW_TEXT), hand_cursor);
}



static void infb_init(void) {

#ifdef ENABLE_NLS
	DEBUG_MSG("infb_init, gettext domain-name=%s\n",PACKAGE"_plugin_infbrowser");
	bindtextdomain(PACKAGE"_plugin_infbrowser", LOCALEDIR);
	bind_textdomain_codeset(PACKAGE"_plugin_infbrowser", "UTF-8");
#endif
	infb_v.currentDoc = NULL;
	infb_v.currentNode = NULL;
	infb_v.currentType = 0;
	infb_v.homeDoc = NULL;
	infb_v.windows = g_hash_table_new_full(NULL,NULL,NULL,NULL);
	main_v->sidepanel_initgui = g_slist_prepend(main_v->sidepanel_initgui,infb_sidepanel_initgui);
	main_v->sidepanel_destroygui = g_slist_prepend(main_v->sidepanel_destroygui,infb_sidepanel_destroygui);
	hand_cursor = gdk_cursor_new (GDK_HAND2);
	regular_cursor = gdk_cursor_new (GDK_XTERM);
	
	DEBUG_MSG("infb_init finished \n");
}

static void infb_initgui(Tbfwin* bfwin) {

}

static void infb_enforce_session(Tbfwin* bfwin) {
}

static void infb_cleanup(void) {
	/*g_object_unref(infb_v.icon_note);*/
}

static void infb_cleanup_gui(Tbfwin *bfwin) {
}

static GHashTable *infb_register_globses_config(GHashTable *configlist) {
	return configlist;
}

static GHashTable *infb_register_session_config(GHashTable *configlist, Tsessionvars *session) {
	return configlist;
}

static TBluefishPlugin bfplugin = {
	"Info Browser",
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
	NULL, /* private */
	infb_init, /* init */
	infb_initgui, 
	infb_enforce_session,
	infb_cleanup,
	infb_cleanup_gui,
	infb_register_globses_config,
	infb_register_session_config,
	NULL, /* binary compatibility */
	NULL,
	NULL,
	NULL
};

G_MODULE_EXPORT TBluefishPlugin *getplugin(void) {
	return &bfplugin;
}
