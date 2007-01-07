/* Bluefish HTML Editor
 * snippets_load.h - plugin for snippets sidebar
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

#ifndef SNIPPETS_LOAD_H
#define SNIPPETS_LOAD_H

enum {
	pixmap_type_none,
	pixmap_type_insert,
	pixmap_type_snr
};

void snippets_add_tree_item(GtkTreeIter *parent, GtkTreeIter *child, gint pixmaptype, const gchar *name, gpointer ptr);

void snippets_load(void);
gboolean snippets_store_lcb(gpointer data);

#endif /* SNIPPETS_LOAD_H */
