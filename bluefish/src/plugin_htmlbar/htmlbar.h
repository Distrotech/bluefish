/* Bluefish HTML Editor
 * htmlbar.h - plugin for html toolbar
 *
 * Copyright (C) 2002-2005 Olivier Sessink
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

#ifndef __HTMLBAR_H_
#define __HTMLBAR_H_

#define ENABLEPLUGINS
#define DEBUG

#include <gtk/gtk.h>
#include "../config.h"
#include "../bluefish.h"

#ifdef ENABLE_NLS
#undef _
#define _(String) dgettext(PACKAGE"_plugin_htmlbar", String)
#endif    

typedef struct {
	GHashTable* lookup;
	gint view_htmlbar;
	GList *quickbar_items;
} Thtmlbar;

typedef struct {
	Tbfwin *bfwin;
	GtkWidget *handlebox;
} Thtmlbarwin;

typedef struct {
	gint view_htmlbar;
} Thtmlbarsession;

extern Thtmlbar htmlbar_v;

#endif /* __HTMLBAR_H_ */
