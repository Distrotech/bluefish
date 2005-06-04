/* Copyright (C) 2005 Olivier Sessink
 * plugins.c - handle plugin loading
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "config.h"

#include <gtk/gtk.h>

#include "plugins.h"

typedef struct {
	gchar *filename;
} TPrivatePluginData;

#define PRIVATE(var) ((TPrivatePluginData *)var->private)

static TBluefishPlugin *load_plugin(const gchar *filename) {
	GModule *module;
	TBluefishPlugin *bfplugin;
	gpointer func;
	TBluefishPlugin *(*getplugin)();
	
	module = g_module_open(filename,0);
	if (!module) {
		DEBUG_MSG("load_plugin, failed to load %s with error %s\n",filename, g_module_error());
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
	return bfplugin;
}

void bluefish_load_plugins(void) {
	TBluefishPlugin *bfplugin;
	/* should be finished when plugins are really working */
	bfplugin = load_plugin("htmlbar.so");
	if (bfplugin) {
		bfplugin->init();
	}
}
void bluefish_cleanup_plugins(void) {
	/* should be finished when plugins are really working */

}
