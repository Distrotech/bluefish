/* Bluefish HTML Editor
 * infb_text.h - plugin for information browser
 *
 * Copyright (C) 2007 Oskar Świda
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

#ifndef INFB_TEXT_H
#define INFB_TEXT_H

#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "../bluefish.h"

#define INFB_INDEX_PUBLIC_ID	"-//Bluefish//DTD//Infob Index"
#define INFB_FREF2_PUBLIC_ID	"-//Bluefish//DTD//Infob Fref2"



#define INFB_STYLE_SMALL		"scale",PANGO_SCALE_SMALL
#define INFB_STYLE_BOLD			"weight",PANGO_WEIGHT_BOLD
#define INFB_STYLE_TITLE		"weight",PANGO_WEIGHT_BOLD,"paragraph-background","#E3D1AD","justification",GTK_JUSTIFY_CENTER
#define INFB_STYLE_DESC			"scale",PANGO_SCALE_SMALL,"paragraph-background","#EAD8B3","justification",GTK_JUSTIFY_CENTER
#define INFB_STYLE_FILEREF		"weight",PANGO_WEIGHT_BOLD
#define INFB_STYLE_NODE			"style",PANGO_STYLE_ITALIC
#define INFB_STYLE_GROUP		"style",PANGO_STYLE_ITALIC,"weight",PANGO_WEIGHT_BOLD
#define INFB_STYLE_SECTION		"paragraph-background","#E5E5E5","justification",GTK_JUSTIFY_CENTER
#define INFB_STYLE_ITALIC		"style",PANGO_STYLE_ITALIC

#define INFB_TT_NONE			0
#define INFB_TT_SMALL		1
#define INFB_TT_BOLD			2
#define INFB_TT_ITALIC		3
#define INFB_TT_TITLE		4
#define INFB_TT_DESC			5
#define INFB_TT_SECTION		6


void infb_fill_doc(Tbfwin *bfwin,xmlNodePtr root);
xmlXPathObjectPtr getnodeset (xmlDocPtr doc, xmlChar *xpath,xmlNodePtr start);
xmlNodePtr getnode (xmlDocPtr doc, xmlChar *xpath,xmlNodePtr start);
gint getcount (xmlDocPtr doc, xmlChar *xpath,xmlNodePtr start);

void infb_insert_icon(GtkTextView *view, GtkWidget *icon, gchar *prepend);
void infb_insert_fileref(GtkTextBuffer *buff, xmlChar *text, xmlChar *fname);
void infb_insert_node(GtkTextBuffer *buff, xmlChar *text, xmlNodePtr node,gboolean endline);
void infb_insert_group(GtkTextView *view, xmlChar *text, xmlNodePtr node);
void infb_insert_text(GtkTextBuffer *buff, xmlChar *text, gint type, gboolean eol);
void infb_insert_text_tag(GtkTextBuffer *buff, xmlChar *text, GtkTextTag *tag, gboolean eol);
void infb_insert_widget(GtkTextView *view, GtkWidget *widget,gint size);
#endif /* INFB_TEXT_H */
