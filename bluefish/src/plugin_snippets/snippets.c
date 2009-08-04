/* Bluefish HTML Editor
 * snippets.c - plugin for snippets sidebar
 *
 * Copyright (C) 2006 Olivier Sessink
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
/* #define DEBUG */

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
					NULL,NULL);
	snippets_v.store = gtk_tree_store_new(NUM_COLUMNS /* Total number of columns */,GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER);
	main_v->sidepanel_initgui = g_slist_prepend(main_v->sidepanel_initgui,snippets_sidepanel_initgui);
	main_v->sidepanel_destroygui = g_slist_prepend(main_v->sidepanel_destroygui,snippets_sidepanel_destroygui);
	snippets_load();
	
	DEBUG_MSG("snippets_init finished, store=%p, lookup=%p\n",snippets_v.store, snippets_v.lookup);
}

Tsnippetssession *snippets_get_session(Tsessionvars *session) {
	Tsnippetssession *sns = g_hash_table_lookup(snippets_v.lookup,session);
	if (!sns) {
		sns = g_new0(Tsnippetssession,1);
		sns->show_as_menu = TRUE;
		g_hash_table_insert(snippets_v.lookup,session,sns);
	}
	return sns;
}

static void snippets_initgui(Tbfwin* bfwin) {
	snippets_create_menu(bfwin);
}
static void snippets_enforce_session(Tbfwin* bfwin) {
	Tsnippetssession *sns;
	Tsnippetswin *snw;
	sns = g_hash_table_lookup(snippets_v.lookup,bfwin->session);
	snw = g_hash_table_lookup(snippets_v.lookup,bfwin);
	if (sns && snw) {
		snippets_show_as_menu(snw, sns->show_as_menu);
	}
}
static void snippets_cleanup(void) {
	/*GList *tmplist = g_list_first(gtk_window_list_toplevels());
	while (tmplist) {
		if (GTK_IS_WIDGET(tmplist->data)) {
			const gchar *role= gtk_window_get_role ((GtkWindow*)tmplist->data);
			if (role && strcmp(role,"html_dialog") ==0) {
				gtk_widget_hide(GTK_WIDGET(tmplist->data));
				window_destroy(GTK_WIDGET(tmplist->data));
			}
		}
		tmplist = g_list_next(tmplist);
	}*/
}

static void snippets_cleanup_gui(Tbfwin *bfwin) {
	/* BUG: clean the keys and structures in the hashtable */
}

static GHashTable *snippets_register_globses_config(GHashTable *configlist) {
/*	DEBUG_MSG("snippets_register_globses_config, started\n");
	configlist = make_config_list_item(configlist, &snippets_v.quickbar_items, 'l', "snippets_quickbar:", 0);*/
	return configlist;
}
static GHashTable *snippets_register_session_config(GHashTable *configlist, Tsessionvars *session) {
	Tsnippetssession *sns;
	sns = snippets_get_session(session);
	configlist = make_config_list_item(configlist, &sns->show_as_menu, 'i', "snippets_show_as_menu:", 1);
	return configlist;
}

static TBluefishPlugin bfplugin = {
	"snippets",
	BFPLUGIN_VERSION,
	sizeof(Tdocument),
	sizeof(Tsessionvars),
	sizeof(Tglobalsession),
	sizeof(Tbfwin),
	sizeof(Tproject),
	sizeof(Tmain),
	BFPLUGIN_PRIORITY_DEFAULT,
	1,
	NULL, /* private */
	snippets_init, /* init */
	snippets_initgui, 
	snippets_enforce_session,
	snippets_cleanup,
	snippets_cleanup_gui,
	snippets_register_globses_config,
	snippets_register_session_config,
	NULL, /* binary compatibility */
	NULL,
	NULL,
	NULL
};

G_MODULE_EXPORT TBluefishPlugin *getplugin(void) {
	return &bfplugin;
}
