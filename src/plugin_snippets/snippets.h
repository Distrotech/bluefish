/* Bluefish HTML Editor
 * snippets.h - plugin for snippets sidebar
 *
 * Copyright (C) 2006 Olivier Sessink
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
#ifdef ENABLE_NLS
#undef _
#define _(String) dgettext(PACKAGE"_plugin_snippets", String)
#endif    

enum {
	PIXMAP_COLUMN,
	TITLE_COLUMN,
	NODE_COLUMN,
	NUM_COLUMNS

};

typedef struct {
	GHashTable* lookup; /* pointers are stored here */
	xmlDocPtr doc;
	GtkTreeStore *store;
} Tsnippets;

typedef struct {
	Tbfwin *bfwin;
	GtkWidget *snippetsmenu;
	GtkWidget *view;
	/*TreeTips *ttips;*/
	GtkAccelGroup *accel_group;
	xmlNodePtr lastclickednode;
	GtkTreePath *lastclickedpath;
} Tsnippetswin;

typedef struct {
	gboolean show_as_menu;
} Tsnippetssession;
extern Tsnippets snippets_v;

Tsnippetssession *snippets_get_session(Tsessionvars *session);
Tsnippetswin *snippets_get_win(Tbfwin *bfwin);
#endif /* __SNIPPETS_H_ */
