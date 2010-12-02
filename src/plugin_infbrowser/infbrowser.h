/* Bluefish HTML Editor
 * infbrowser.h - plugin for information browser based on snippets plugin
 *
 * Copyright (C) 2007 Oskar Świda
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
#ifndef __INFB_H_
#define __INFB_H_

#define ENABLEPLUGINS
/* #define DEBUG */

#include <gtk/gtk.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xmlwriter.h>
#include <libxml/HTMLparser.h>


#include "../config.h"
#include "../bluefish.h"
#ifdef ENABLE_NLS
#undef _
#define _(String) dgettext(PACKAGE"_plugin_infbrowser", String)
#endif    

#define INFB_DOCTYPE_UNKNOWN	0
#define INFB_DOCTYPE_INDEX		1
#define INFB_DOCTYPE_FREF2		2
#define INFB_DOCTYPE_DTD		3
#define INFB_DOCTYPE_DOCBOOK	4
#define INFB_DOCTYPE_HTML		5



typedef struct {
	xmlDocPtr currentDoc;
	xmlNodePtr currentNode;
	guchar currentType;
	xmlDocPtr homeDoc;
	guchar nt_fileref, nt_node, nt_group, nt_localref; 	/* node types */
	GHashTable *windows; /* list of windows - concerning several instances of bfish ? */
} Tinfb;

typedef struct {
	Tbfwin *bfwin;
	GtkWidget *view;
	GtkWidget *sentry; /* search entry */
	GtkToolItem *btn_add,*btn_del,*btn_home,*btn_up,*btn_idx;
	GtkToolItem *saved;
	gboolean hovering_over_link;	
	GtkWidget *tip_window,*tip_label;
	GtkTextTag *search_tag;
} Tinfbwin;

extern Tinfb infb_v;

void set_normal_cursor(GtkTextView *view);
void set_link_cursor(GtkTextView *view);
 	

#endif /* __INFB_H_ */
