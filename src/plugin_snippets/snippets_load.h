/* Bluefish HTML Editor
 * snippets_load.h - plugin for snippets sidebar
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

#ifndef SNIPPETS_LOAD_H
#define SNIPPETS_LOAD_H

enum {
	pixmap_type_none,
	pixmap_type_insert,
	pixmap_type_snr
};

void snippets_fill_tree_item_from_node(GtkTreeIter *iter, xmlNodePtr node);

void snippets_load(void);
gboolean snippets_store_lcb(gpointer data);
void reload_tree_from_doc(void);
void snippets_export_node(xmlNodePtr node, const gchar *filename);
void snippets_import_node(xmlNodePtr branch, const gchar *filename);

#endif /* SNIPPETS_LOAD_H */
