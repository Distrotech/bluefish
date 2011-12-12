/* Bluefish HTML Editor
 * snippets.c - plugin for snippets sidebar
 *
 * Copyright (C) 2006-2010 Olivier Sessink
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

#include <string.h>
#include "snippets.h"
#include <gmodule.h>
#include "../plugins.h"
#include "../rcfile.h"
#include "../document.h"
#include "../bf_lib.h"
#include "snippets_gui.h"
#include "snippets_load.h"
Tsnippets snippets_v;

static void snippets_init(void) {
#ifdef ENABLE_NLS
	DEBUG_MSG("snippets_init, gettext domain-name=%s\n",PACKAGE"_plugin_snippets");
	bindtextdomain(PACKAGE"_plugin_snippets", LOCALEDIR);
	bind_textdomain_codeset(PACKAGE"_plugin_snippets", "UTF-8");
#endif
	snippets_v.lookup = g_hash_table_new_full(NULL /* == g_direct_hash() */,
					NULL /* == g_direct_equal() */,
					NULL,g_free);
	snippets_v.store = gtk_tree_store_new(NUM_COLUMNS /* Total number of columns */,GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER);
	main_v->sidepanel_initgui = g_slist_prepend(main_v->sidepanel_initgui,snippets_sidepanel_initgui);
/*	main_v->sidepanel_destroygui = g_slist_prepend(main_v->sidepanel_destroygui,snippets_sidepanel_destroygui);*/
	snippets_load();
	
	DEBUG_MSG("snippets_init finished, store=%p, lookup=%p\n",snippets_v.store, snippets_v.lookup);
}

Tsnippetssession *snippets_get_session(Tsessionvars *session) {
	Tsnippetssession *sns = g_hash_table_lookup(snippets_v.lookup,session);
	if (!sns) {
		sns = g_new0(Tsnippetssession,1);
		sns->show_as_menu = TRUE;
		g_hash_table_insert(snippets_v.lookup,session,sns);
		DEBUG_MSG("create snippetsession %p for session %p\n",sns,session);
	}
	return sns;
}
Tsnippetswin *snippets_get_win(Tbfwin *bfwin) {
	Tsnippetswin *snw = g_hash_table_lookup(snippets_v.lookup,bfwin);
	if (!snw) {
		snw = g_new0(Tsnippetswin,1);
		snw->bfwin = bfwin;
		g_hash_table_insert(snippets_v.lookup,bfwin,snw);
		DEBUG_MSG("create snippetwindow %p for bfwin %p\n",snw,bfwin);
	}
	return snw;	
}
static void snippets_initgui(Tbfwin* bfwin) {
	DEBUG_MSG("snippets_initgui for bfwin %p\n",bfwin);
	snippets_create_gui(bfwin);
}
static void snippets_enforce_session(Tbfwin* bfwin) {
	Tsnippetssession *sns;
	Tsnippetswin *snw;
	DEBUG_MSG("snippets_enforce_session, bfwin=%p\n",bfwin);
	sns = g_hash_table_lookup(snippets_v.lookup,bfwin->session);
	snw = g_hash_table_lookup(snippets_v.lookup,bfwin);
	if (sns && snw) {
		snippets_show_as_menu(snw, sns->show_as_menu);
	}
}
static void snippets_cleanup(void) {
	DEBUG_MSG("snippets_cleanup\n");
	g_hash_table_unref(snippets_v.lookup);
	g_object_unref(snippets_v.store);
	main_v->sidepanel_initgui = g_slist_remove(main_v->sidepanel_initgui,snippets_sidepanel_initgui);
/*	main_v->sidepanel_destroygui = g_slist_remove(main_v->sidepanel_destroygui,snippets_sidepanel_destroygui);*/
}

static void snippets_cleanup_gui(Tbfwin *bfwin) {
	Tsnippetswin *snw;
	DEBUG_MSG("snippets_cleanup_gui\n");
	snw = snippets_get_win(bfwin);
	if (snw) {
		DEBUG_MSG("snippets_cleanup_gui, remove accelerators from bfwin %p\n",bfwin);
		gtk_window_remove_accel_group(GTK_WINDOW(snw->bfwin->main_window), snw->accel_group);
		g_object_unref(G_OBJECT(snw->accel_group));
	}	
	/* BUG: clean the keys and structures in the hashtable */
	g_hash_table_remove(snippets_v.lookup,bfwin);
}

/*static GHashTable *snippets_register_globses_config(GHashTable *configlist) {
/ *	DEBUG_MSG("snippets_register_globses_config, started\n");
	configlist = make_config_list_item(configlist, &snippets_v.quickbar_items, 'l', "snippets_quickbar:", 0);* /
	return configlist;
}*/
static GHashTable *snippets_register_session_config(GHashTable *configlist, Tsessionvars *session) {
	Tsnippetssession *sns;
	sns = snippets_get_session(session);
	configlist = make_config_list_item(configlist, &sns->show_as_menu, 'i', "snippets_show_as_menu:", 1);
	return configlist;
}

static void snippets_session_cleanup(Tsessionvars *session) {
	g_hash_table_remove(snippets_v.lookup,session);
}

static TBluefishPlugin bfplugin = {
	"Code Snippets",
	BFPLUGIN_VERSION,
	GTK_MAJOR_VERSION,
	sizeof(Tdocument),
	sizeof(Tsessionvars),
	sizeof(Tglobalsession),
	sizeof(Tbfwin),
	sizeof(Tproject),
	sizeof(Tmain),
	sizeof(Tproperties),
	BFPLUGIN_PRIORITY_LATE, /* late means after (under) the htmlbar toolbar */
	1,
	NULL, /* private */
	snippets_init, /* init */
	snippets_initgui, 
	snippets_enforce_session,
	snippets_cleanup,
	snippets_cleanup_gui,
	NULL /*snippets_register_globses_config*/,
	snippets_register_session_config,
	snippets_session_cleanup,
	NULL, /* binary compatibility */
	NULL,
	NULL
};

G_MODULE_EXPORT TBluefishPlugin *getplugin(void) {
	return &bfplugin;
}
