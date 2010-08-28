/* Bluefish HTML Editor
 * rsync.h - plugin for rsync
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

#ifndef __RSYNC_H_
#define __RSYNC_H_

#define ENABLEPLUGINS
#define DEBUG

#include <gtk/gtk.h>
#include "../config.h"
#include "../bluefish.h"

#ifdef ENABLE_NLS
#undef _
#define _(String) dgettext(PACKAGE"_plugin_rsync", String)
#endif    

typedef struct {
	GHashTable* lookup;
} Trsync;

typedef struct {
	gchar *hostname;
	gchar *source_dir;
	gchar *target_dir;
	GtkWidget *win;
} Trsyncsession;

extern Trsync rsync_v;

#endif /* __RSYNC_H_ */
