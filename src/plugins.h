/* Bluefish HTML Editor
 * plugins.h - plugin structure
 *
 * Copyright (C) 2005 Olivier Sessink
 * heavily inspired by code from Eugene Morenko(More)
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

#ifndef __PLUGINS_H__
#define __PLUGINS_H__

#include "bluefish.h"


#define BFPLUGIN_VERSION 4
typedef struct {
	const gchar *name; /* plugin name */
	const gushort bfplugin_version;
	const gushort document_size;
	const gushort sessionvars_size;
	const gushort globalsession_size;
	const gushort bfwin_size;
	const gushort project_size;
	const gushort main_size;
		
	gpointer private; /* to be filled by Bluefish after loading */
	void (*init) (void); /* called once after plugin is loaded */
	void (*init_gui) (Tbfwin *bfwin); /* builds the gui */
	void (*enforce_session) (Tbfwin *bfwin); /* may change the gui to enforce some session setting */
	void (*cleanup) (void);
	void (*cleanup_gui) (Tbfwin *bfwin);
	
	GList *(*register_globses_config)(GList *configlist);
	GList *(*register_session_config)(GList *configlist, Tsessionvars *session);

	gpointer extra1; /* for binary compatibility */
	gpointer extra2;
	gpointer extra3;
	gpointer extra4;
} TBluefishPlugin;

#define BFPLUGIN(var) ((TBluefishPlugin *)var)

#ifdef __cplusplus
extern "C" {
#endif

void bluefish_load_plugins(void);
void bluefish_cleanup_plugins(void);

void bfplugins_gui(gpointer data, gpointer user_data);
void bfplugins_enforce_session(gpointer data, gpointer user_data);

GList *bfplugins_register_globses_config(GList *list);
GList *bfplugins_register_session_config(GList *list,Tsessionvars *session);

#ifdef __cplusplus
};
#endif

#endif /* __PLUGINS_H__ */
