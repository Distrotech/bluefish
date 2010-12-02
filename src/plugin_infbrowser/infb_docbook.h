/* Bluefish HTML Editor
 * infb_docbook.h - plugin for information browser
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

#ifndef INFB_DOCBOOK_H
#define INFB_DOCBOOK_H

#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>


void infb_db_prepare_info(GtkTextView *view,xmlDocPtr doc, xmlNodePtr node);
void infb_db_format_element(GtkTextView *view,xmlDocPtr doc, xmlNodePtr node);
xmlChar *infb_db_get_title(xmlDocPtr doc,gboolean subtitle,xmlNodePtr root);
xmlNodePtr infb_db_get_info_node(xmlDocPtr doc,xmlNodePtr root);
void infb_db_fill_node(GtkTextView *view,xmlDocPtr doc,xmlNodePtr node,gint level);


#endif /* INFB_DOCBOOK_H */
