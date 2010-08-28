/* Bluefish HTML Editor
 * charmap.h - charmap plugin
 *
 * Copyright (C) 2009 Olivier Sessink
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

#ifndef __CHARMAP_H_
#define __CHARMAP_H_

#include <gmodule.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>      /* getopt() */

#include "../config.h"
#include "../bluefish.h"

#ifdef ENABLE_NLS
#undef _
#define _(String) dgettext(PACKAGE"_plugin_charmap", String)
#endif

typedef struct {
	Tbfwin *bfwin;
	GtkWidget *chaptersv;
	GtkWidget *gcm;
	GtkWidget *entities;
}Tcharmapwin;

typedef struct {
	gint charmap_block;
} Tcharmapsession;

typedef struct {
	GHashTable* lookup; /* pointers are stored here */
	GtkTreeModel *model;
}Tcharmap;

extern Tcharmap charmap_v;

Tcharmapsession *get_charmap_session(gpointer session);

#endif /* __CHARMAP_H_  */
