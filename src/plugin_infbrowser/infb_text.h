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




void infb_fill_doc(Tbfwin *bfwin,xmlNodePtr root);
xmlXPathObjectPtr getnodeset (xmlDocPtr doc, xmlChar *xpath,xmlNodePtr start);
xmlNodePtr getnode (xmlDocPtr doc, xmlChar *xpath,xmlNodePtr start);
void infb_insert_line(GtkTextBuffer *buff, xmlChar *text, gchar *prepend);
void infb_insert_small_line(GtkTextBuffer *buff,xmlChar *text);
void infb_insert_title(GtkTextBuffer *buff,xmlChar *text);
void infb_insert_desc(GtkTextBuffer *buff,xmlChar *text);
void infb_insert_section(GtkTextBuffer *buff,xmlChar *text);
void infb_insert_icon(GtkTextView *view, GtkWidget *icon, gchar *prepend);
void infb_insert_fileref(GtkTextBuffer *buff, xmlChar *text, xmlChar *fname);
void infb_insert_node(GtkTextBuffer *buff, xmlChar *text, xmlNodePtr node);
void infb_insert_group(GtkTextView *view, xmlChar *text, xmlNodePtr node);
void infb_insert_italic(GtkTextBuffer *buff, xmlChar *text);
void infb_insert_bold(GtkTextBuffer *buff, xmlChar *text);
void infb_insert_text(GtkTextBuffer *buff, xmlChar *text);

#endif /* INFB_TEXT_H */
