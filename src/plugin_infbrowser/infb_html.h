/* Bluefish HTML Editor
 * infb_html.h - plugin for information browser
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

#ifndef INFB_HTML_H
#define INFB_HTML_H

#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>


void infb_html_fill_node(GtkTextView *view,xmlDocPtr doc,xmlNodePtr node,gint level,
                         GtkTextTag *tag,gboolean preserve_spaces);
xmlChar *infb_html_get_title(xmlDocPtr doc);
gchar *infb_html_close_tags(gchar *fname);

#endif /* INFB_HTML_H */
