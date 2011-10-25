/* Bluefish HTML Editor
 * plugins.h - plugin structure
 *
 * Copyright (C) 2005-2010 Olivier Sessink
 * heavily inspired by code from Eugene Morenko(More)
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

#ifndef __PLUGINS_H__
#define __PLUGINS_H__

#include "bluefish.h"

#define BFPLUGIN_PRIORITY_FIRST 0
#define BFPLUGIN_PRIORITY_EARLY 63
#define BFPLUGIN_PRIORITY_DEFAULT 127
#define BFPLUGIN_PRIORITY_LATE 191
#define BFPLUGIN_PRIORITY_LAST 255

#define BFPLUGIN_VERSION 9

typedef struct {
	const gchar *name; /* plugin name */
	const gushort bfplugin_version;
	const gushort gtkmajorversion;
	const gushort document_size;  /* the sizes of the most important structs are checker here to avoid a plugin */
	const gushort sessionvars_size;    /* compiled with a different struct accessing the wrong member of a struct  */
	const gushort globalsession_size;
	const gushort bfwin_size;
	const gushort project_size;
	const gushort main_size;
	const gushort properties_size;
	const gushort priority;
	const gushort default_enabled;
	
	gpointer private; /* to be filled by Bluefish after loading */
	void (*init) (void); /* called once after plugin is loaded */
	void (*init_gui) (Tbfwin *bfwin); /* builds the gui */
	void (*enforce_session) (Tbfwin *bfwin); /* may change the gui to enforce some session setting */
	void (*cleanup) (void);
	void (*cleanup_gui) (Tbfwin *bfwin);
	
	GHashTable *(*register_globses_config)(GHashTable *configlist);
	GHashTable *(*register_session_config)(GHashTable *configlist, Tsessionvars *session);
	void (*session_cleanup) (Tsessionvars *session);
	gpointer extra1; /* for binary compatibility */
	gpointer extra2;
	gpointer extra3;
} TBluefishPlugin;

#define BFPLUGIN(var) ((TBluefishPlugin *)var)

#ifdef __cplusplus
extern "C" {
#endif

void bluefish_load_plugins(void);
void bluefish_cleanup_plugins(void);

void bfplugins_gui(gpointer data, gpointer user_data);
void bfplugins_enforce_session(gpointer data, gpointer user_data);

GHashTable *bfplugins_register_globses_config(GHashTable *list);
GHashTable *bfplugins_register_session_config(GHashTable *list,Tsessionvars *session);
void bfplugins_session_cleanup(Tsessionvars *session);
#ifdef __cplusplus
};
#endif

#endif /* __PLUGINS_H__ */
