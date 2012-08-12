/* Bluefish HTML Editor
 * plugins.c - handle plugin loading
 *
 * Copyright (C) 2005-2010 Olivier Sessink
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

static TBluefishPlugin *plugin_from_filename(const gchar *path) {
	TBluefishPlugin *bfplugin;
	GSList *tmpslist;
	
	bfplugin = load_plugin(path);
	DEBUG_MSG("plugin_from_filename, bfplugin=%p\n",bfplugin);
	if (!bfplugin)
		return NULL;
	
	if (!(bfplugin->bfplugin_version == BFPLUGIN_VERSION
					&& bfplugin->gtkmajorversion == GTK_MAJOR_VERSION
					&& bfplugin->document_size == sizeof(Tdocument)
					&& bfplugin->sessionvars_size == sizeof(Tsessionvars)
					&& bfplugin->globalsession_size == sizeof(Tglobalsession)
					&& bfplugin->bfwin_size == sizeof(Tbfwin)
					&& bfplugin->project_size == sizeof(Tproject)
					&& bfplugin->main_size == sizeof(Tmain)
					&& bfplugin->properties_size == sizeof(Tproperties)
			)) {
		g_warning("plugin %s is not compatible with this version of bluefish",path);
		DEBUG_MSG("plugin_from_filename, %s binary compatibility mismatch\n",path);
		g_module_close(PRIVATE(bfplugin)->module);
		return NULL;
	}
	
	for(tmpslist=main_v->plugins;tmpslist;tmpslist=g_slist_next(tmpslist)) {
		if (g_strcmp0(bfplugin->name, ((TBluefishPlugin *)tmpslist->data)->name)==0) {
			g_warning("not loading %s, plugin \"%s\" was already loaded",path, bfplugin->name);
			g_module_close(PRIVATE(bfplugin)->module);
			return NULL;
		}
	}
	return bfplugin;
}

static void
bluefish_scan_dir_load_plugins(GList **oldlist,const gchar *indirname)
{
	GError *error = NULL;
	GPatternSpec* patspec;
	GDir* gdir;
	const gchar *tmp;
	gchar *dirname = ending_slash(indirname);

	DEBUG_MSG("bluefish_scan_dir_load_plugins, loading from %s\n",dirname);
	DEBUG_PATH("plugins are loaded from %s\n",dirname);
	gdir = g_dir_open(dirname ,0,&error);
	if (error) {
		g_warning("Error: failed to open plugin directory %s",dirname);
		g_error_free(error);
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
				TBluefishPlugin *plugin;
				DEBUG_MSG("bluefish_scan_dir_load_plugins, try loading plugin %s\n",path);
				plugin = plugin_from_filename(path);
				if (plugin) {
					if (!arr) {
						if (plugin->default_enabled) {
							arr = array_from_arglist(path, "1", plugin->name, NULL);
							plugin->init();
							main_v->plugins = g_slist_prepend(main_v->plugins,plugin);
						} else {
							arr = array_from_arglist(path, "0", plugin->name, NULL);
							g_module_close(PRIVATE(plugin)->module);
						}
						main_v->props.plugin_config = g_list_append(main_v->props.plugin_config, arr);
					} else {
						plugin->init();
						main_v->plugins = g_slist_prepend(main_v->plugins,plugin);
					}
				} else { /* no plugname ==> failed to load */
					DEBUG_MSG("bluefish_scan_dir_load_plugins, returned NULL -> load failed\n");
					if (arr && g_strv_length(arr)>=2) {
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
	gchar *path;	
	oldlist = main_v->props.plugin_config;
	DEBUG_MSG("bluefish_load_plugins, oldlist %p len=%d\n",oldlist,g_list_length(oldlist));
	main_v->props.plugin_config = NULL;

	/* load from the user directory first */
	path = g_strconcat(g_get_home_dir(), "/."PACKAGE"/",NULL);
	bluefish_scan_dir_load_plugins(&oldlist,path);
	g_free(path);

#if defined(NSIS) || defined(OSXAPP)
#ifdef RELPLUGINPATH
	DEBUG_MSG("using RELPLUGINPATH\n");
	path = g_build_path(G_DIR_SEPARATOR_S,RELPLUGINPATH,"lib",PACKAGE,NULL);
	bluefish_scan_dir_load_plugins(&oldlist, path);
	g_free(path);
#else /* RELPLUGINPATH */
	path = g_build_path(G_DIR_SEPARATOR_S,".","lib",PACKAGE,NULL);
	bluefish_scan_dir_load_plugins(&oldlist, path);
	g_free(path);
#endif /* RELPLUGINPATH */
#elif defined WIN32 
		gchar *topdir;

		topdir = g_win32_get_package_installation_directory_of_module(NULL);
		path = g_build_path(G_DIR_SEPARATOR_S, topdir, "lib", PACKAGE, NULL);
		bluefish_scan_dir_load_plugins(&oldlist, path);
		
		g_free(topdir);
		g_free(path);
#else /* WIN32 */
	bluefish_scan_dir_load_plugins(&oldlist,PKGLIBDIR);
#endif /* NSIS || OSXAPP */

	free_arraylist(oldlist);
	main_v->plugins = g_slist_sort(main_v->plugins,(GCompareFunc)plugins_compare_priority);
}

/* can be called by g_list_foreach() */
void bfplugins_gui(gpointer data, gpointer user_data) {
	Tbfwin *bfwin = user_data;
	TBluefishPlugin *bfplugin = data;
	if (bfplugin->init_gui) {
		DEBUG_MSG("bfplugins_gui, init_gui for plugin %p and bfwin %p\n",bfplugin,bfwin);	
		bfplugin->init_gui(bfwin);
	}
}
/* can be called by g_list_foreach() */
void bfplugins_enforce_session(gpointer data, gpointer user_data) {
	Tbfwin *bfwin = user_data;
	TBluefishPlugin *bfplugin = data;
	if (bfplugin->enforce_session) {
		DEBUG_MSG("bfplugins_enforce_session, enforce_session for plugin %p and bfwin %p\n",bfplugin,bfwin);
		bfplugin->enforce_session(bfwin);
	}
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

void bfplugins_session_cleanup(Tsessionvars *session) {
	GSList *tmplist = main_v->plugins;
	while (tmplist) {
		TBluefishPlugin *bfplugin = tmplist->data;
		if (bfplugin->session_cleanup) {
			/*g_print("cleanup session %p for plugin %p\n",session,bfplugin);*/
			bfplugin->session_cleanup(session);
		}
		tmplist =  g_slist_next(tmplist);
	}
}

void bluefish_cleanup_plugins(void) {
	GSList *tmplist = main_v->plugins;
	while (tmplist) {
		TBluefishPlugin *bfplugin = tmplist->data;
		if (bfplugin->cleanup) {
			bfplugin->cleanup();
		}
		g_free(PRIVATE(bfplugin)->filename);
		g_module_close(PRIVATE(bfplugin)->module);
		tmplist =  g_slist_next(tmplist);
	}
	g_slist_free(main_v->plugins);
	
}
