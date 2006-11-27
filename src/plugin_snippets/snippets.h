/* Bluefish HTML Editor
 * snippets.h - plugin for snippets sidebar
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
#ifndef __SNIPPETS_H_
#define __SNIPPETS_H_

#define ENABLEPLUGINS
/* #define DEBUG */

#include <gtk/gtk.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xmlwriter.h>


#include "../config.h"
#include "../bluefish.h"
#include "../treetips.h"
#ifdef ENABLE_NLS
#undef _
#define _(String) dgettext(PACKAGE"_plugin_snippets", String)
#endif    

typedef struct {
	GHashTable* lookup; /* pointers are stored here */
	xmlDocPtr doc;
	GtkTreeStore *store;
} Tsnippets;

typedef struct {
	Tbfwin *bfwin;
	GtkWidget *view;
	TreeTips *ttips;
	GtkAccelGroup *accel_group;
	xmlNodePtr lastclickednode;
	GtkTreePath *lastclickedpath;
} Tsnippetswin;

extern Tsnippets snippets_v;

#endif /* __SNIPPETS_H_ */
