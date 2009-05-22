/* Bluefish HTML Editor
 * plugins.c - handle plugin loading
 *
 * Copyright (C) 2005,2006 Olivier Sessink
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

/*#define DEBUG*/

#include "config.h"

#include <gtk/gtk.h>
#include <gmodule.h>
#include <string.h>
#include "bluefish.h"
#include "bf_lib.h"
#include "plugins.h"
#include "stringlist.h"

typedef struct {
	gchar *filename;
	GModule *module;
} TPrivatePluginData;

#define PRIVATE(var) ((TPrivatePluginData *)var->private)

static TBluefishPlugin *load_plugin(const gchar *filename) {
	GModule *module;
	TBluefishPlugin *bfplugin;
	gpointer func;
	TBluefishPlugin *(*getplugin)();
	
	module = g_module_open(filename,0);
	if (!module) {
		g_warning("failed to load plugin %s with error %s\n",filename, g_module_error());
		return NULL;
	}

	if (!g_module_symbol(module, "getplugin",&func)) {
		DEBUG_MSG("load_plugin: module %s does not have getplugin(): %s\n",filename,g_module_error());
		g_module_close(module);
		return NULL;
	}
	getplugin = func;
	bfplugin = getplugin();
	if (!bfplugin) {
		DEBUG_MSG("load_plugin: %s getplugin() returned NULL\n",filename);
		g_module_close(module);
		return NULL;
	}
	bfplugin->private = g_new0(TPrivatePluginData,1);
	PRIVATE(bfplugin)->filename = g_strdup(filename);
	PRIVATE(bfplugin)->module = module;
	return bfplugin;
}

static const gchar *plugin_from_filename(const gchar *path) {
	TBluefishPlugin *bfplugin;
	bfplugin = load_plugin(path);
	DEBUG_MSG("plugin_from_filename, bfplugin=%p\n",bfplugin);
	if (bfplugin) {
		if (bfplugin->bfplugin_version == BFPLUGIN_VERSION
					&& bfplugin->document_size == sizeof(Tdocument)
					&& bfplugin->sessionvars_size == sizeof(Tsessionvars)
					&& bfplugin->globalsession_size == sizeof(Tglobalsession)
					&& bfplugin->bfwin_size == sizeof(Tbfwin)
					&& bfplugin->project_size == sizeof(Tproject)
					&& bfplugin->main_size == sizeof(Tmain)) {
			DEBUG_MSG("bluefish_load_plugins, loaded %s properly, init!\n",path);
			bfplugin->init();
			main_v->plugins = g_slist_prepend(main_v->plugins,bfplugin);
			return bfplugin->name;
		} else {
			DEBUG_MSG("plugin_from_filename, %s binary compatibility mismatch\n",path);
			g_module_close(PRIVATE(bfplugin)->module);
		}
	}
	return NULL;
}

static void bluefish_scan_dir_load_plugins(GList **oldlist,const gchar *indirname) {
	GError *error = NULL;
	GPatternSpec* patspec;
	GDir* gdir;
	const gchar *tmp;
	gchar *dirname = ending_slash(indirname);

	DEBUG_MSG("bluefish_scan_dir_load_plugins, loading from %s\n",dirname);
	gdir = g_dir_open(dirname ,0,&error);
	if (error) {
		/* BUG: handle the error  */
		g_free(dirname);
		return;
	}
	patspec = g_pattern_spec_new("*."G_MODULE_SUFFIX);

	tmp = g_dir_read_name(gdir);
	while (tmp) {
		if (g_pattern_match(patspec, strlen(tmp), tmp, NULL)) {
			gchar *path = g_strconcat(dirname, tmp, NULL);
			gchar *compare[] = {path, NULL}, **arr;

			arr = arraylist_value_exists(*oldlist, (const gchar **) compare, 1, FALSE);
			if (arr) {
				GList *link;
				DEBUG_MSG("bluefish_scan_dir_load_plugins, found %s in configfile (len(oldlist)=%d, len(plugin_config=%d)\n",arr[0],g_list_length(*oldlist),g_list_length(main_v->props.plugin_config));
				link = g_list_find(*oldlist,arr);
				*oldlist = g_list_remove_link(*oldlist, link);
				main_v->props.plugin_config = g_list_concat(main_v->props.plugin_config, link);
			}
			if (arr && arr[1][0] == '0') {
				DEBUG_MSG("bluefish_scan_dir_load_plugins, plugin %s is disabled\n",path);
			} else {
				const gchar *plugname;
				DEBUG_MSG("bluefish_scan_dir_load_plugins, try loading plugin %s\n",path);
				plugname = plugin_from_filename(path);
				if (plugname) {
					if (!arr) {
						arr = array_from_arglist(path, "1", plugname, NULL);
						main_v->props.plugin_config = g_list_append(main_v->props.plugin_config, arr);
					}
				} else { /* no plugname ==> failed to load */
					DEBUG_MSG("bluefish_scan_dir_load_plugins, returned NULL -> load failed\n");
					if (arr && count_array(arr)>=2) {
						g_free(arr[1]);
						arr[1] = g_strdup("1");
					} else {
						arr = array_from_arglist(path, "1", "error loading plugin", NULL);
						main_v->props.plugin_config = g_list_append(main_v->props.plugin_config, arr);
					}
				}
			}
			g_free(path);
		}
		tmp = g_dir_read_name(gdir);
	}
	g_dir_close(gdir);
	g_pattern_spec_free(patspec);
	g_free(dirname);
}

static gint plugins_compare_priority(gpointer a, gpointer b) {
	TBluefishPlugin *pa=a,*pb=b;
	return pa->priority-pb->priority;
}

void bluefish_load_plugins(void) {
	GList *oldlist;	
	oldlist = main_v->props.plugin_config;
	DEBUG_MSG("bluefish_load_plugins, oldlist %p len=%d\n",oldlist,g_list_length(oldlist));
	main_v->props.plugin_config = NULL;

	bluefish_scan_dir_load_plugins(&oldlist,PKGLIBDIR);
/* #ifdef DEVELOPMENT */
	{
		gchar*dir = g_strconcat(g_get_home_dir(), "/."PACKAGE"/",NULL);
		bluefish_scan_dir_load_plugins(&oldlist,dir);
		g_free(dir);
	}
/* #endif */ /* DEVELOPMENT */
	free_arraylist(oldlist);
	
	main_v->plugins = g_slist_sort(main_v->plugins,(GCompareFunc)plugins_compare_priority);
}

void bluefish_cleanup_plugins(void) {
	/* BUG: should be finished when plugins are really working */
}

/* can be called by g_list_foreach() */
void bfplugins_gui(gpointer data, gpointer user_data) {
	Tbfwin *bfwin = user_data;
	TBluefishPlugin *bfplugin = data;
	DEBUG_MSG("bfplugins_gui, init_gui for plugin %p and bfwin %p\n",bfplugin,bfwin);	
	bfplugin->init_gui(bfwin);
}
/* can be called by g_list_foreach() */
void bfplugins_enforce_session(gpointer data, gpointer user_data) {
	Tbfwin *bfwin = user_data;
	TBluefishPlugin *bfplugin = data;
	DEBUG_MSG("bfplugins_enforce_session, enforce_session for plugin %p and bfwin %p\n",bfplugin,bfwin);	
	bfplugin->enforce_session(bfwin);
}

GHashTable *bfplugins_register_globses_config(GHashTable *list) {
	GSList *tmplist = main_v->plugins;
	while (tmplist) {
		TBluefishPlugin *bfplugin = tmplist->data;
		if (bfplugin->register_globses_config) {
			list = bfplugin->register_globses_config(list);
		}
		tmplist =  g_slist_next(tmplist);
	}
	return list;
}
GHashTable *bfplugins_register_session_config(GHashTable *list, Tsessionvars *session) {
	GSList *tmplist = main_v->plugins;
	while (tmplist) {
		TBluefishPlugin *bfplugin = tmplist->data;
		if (bfplugin->register_session_config) {
			list = bfplugin->register_session_config(list, session);
		}
		tmplist =  g_slist_next(tmplist);
	}
	return list;
}

