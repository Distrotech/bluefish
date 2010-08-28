/* Bluefish HTML Editor
 * rsync.c - plugin for rsync
 *
 * Copyright (C) 2005 Olivier Sessink
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

#include <gmodule.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>	/* getopt() */

#include "../config.h"
#include "../plugins.h"
#include "../bluefish.h"
#include "../rcfile.h"

#include "rsync.h"
#include "rsyncgui.h"

Trsync rsync_v;

static void rsync_init(void) {
#ifdef ENABLE_NLS
	bindtextdomain(PACKAGE"_plugin_rsync", LOCALEDIR);
	bind_textdomain_codeset(PACKAGE"_plugin_rsync", "UTF-8");
#endif /* ENABLE_NLS */
	rsync_v.lookup = g_hash_table_new_full(NULL /* == g_direct_hash() */,
					NULL /* == g_direct_equal() */,
					NULL,NULL);
}

#ifdef ENABLE_NLS
static gchar *rsync_menu_translate(const gchar * path, gpointer data) {
	return _(path);
}
#endif /* ENABLE_NLS */

static Trsyncsession *get_rsyncsession(Tsessionvars *session) {
	Trsyncsession *rss;
	rss = g_hash_table_lookup(rsync_v.lookup,session);
	if (!rss) {
		rss = g_new0(Trsyncsession,1);
		g_hash_table_insert(rsync_v.lookup,session,rss);
	}
	return rss;
}

static void rsync_initgui(Tbfwin* bfwin) {
	Trsyncsession *rss;
	GtkItemFactory *ifactory;
	static GtkItemFactoryEntry menu_items[] = {
		{N_("/Project/_Rsync..."), NULL, rsync_dialog_create, 0, "<Item>"}
	};
	ifactory = gtk_item_factory_from_widget(bfwin->menubar);
#ifdef ENABLE_NLS
	gtk_item_factory_set_translate_func(ifactory, rsync_menu_translate, "<bluefishmain>", NULL);
#endif
	gtk_item_factory_create_items(ifactory, sizeof(menu_items) / sizeof(menu_items[0]), menu_items, bfwin);
	gtk_widget_show_all(bfwin->menubar);
	rss = get_rsyncsession(bfwin->session);
}

static void rsync_enforce_session(Tbfwin* bfwin) {
	/* nothing to do here */
}
static void rsync_cleanup(void) {
}
static void rsync_cleanup_gui(Tbfwin *bfwin) {
	/* remove the menu item? - no will be destroyed with the gui
	but we should perhaps cleanup the session from this window? */
}

static GList *rsync_register_globses_config(GList *configlist) {
	return configlist;
}
static GList *rsync_register_session_config(GList *configlist, Tsessionvars *session) {
	Trsyncsession *rss;
	rss = get_rsyncsession(session);
	configlist = make_config_list_item(configlist, &rss->hostname, 's', "rsync_hostname:", 0);
	configlist = make_config_list_item(configlist, &rss->source_dir, 's', "rsync_source_dir:", 0);
	configlist = make_config_list_item(configlist, &rss->target_dir, 's', "rsync_target_dir:", 0);
	return configlist;
}

static TBluefishPlugin bfplugin = {
	"rsync",
	BFPLUGIN_VERSION,
	sizeof(Tdocument),
	sizeof(Tsessionvars),
	sizeof(Tglobalsession),
	sizeof(Tbfwin),
	sizeof(Tproject),
	sizeof(Tmain),
	NULL, /* private */
	rsync_init, /* init */
	rsync_initgui, 
	rsync_enforce_session,
	rsync_cleanup,
	rsync_cleanup_gui,
	rsync_register_globses_config,
	rsync_register_session_config,
	NULL, /* binary compatibility */
	NULL,
	NULL,
	NULL
};

G_MODULE_EXPORT TBluefishPlugin *getplugin(void) {
	return &bfplugin;
}
