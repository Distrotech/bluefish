/* Bluefish HTML Editor
 * document.h - global function for document handling
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * Copyright (C) 1999-2002 Olivier Sessink
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
#ifndef __DOCUMENT_H_
#define __DOCUMENT_H_

void doc_new_with_file(gchar * filename);
void docs_new_from_files(GList * file_list);
void doc_activate(Tdocument *doc);

/* callbacks for the menu and toolbars */
void file_save_cb(GtkWidget * widget, gpointer data);
void file_save_as_cb(GtkWidget * widget, gpointer data);
void file_move_to_cb(GtkWidget * widget, gpointer data);
void file_open_cb(GtkWidget * widget, gpointer data);
void file_revert_to_saved_cb(GtkWidget * widget, gpointer data);
void file_insert_cb(GtkWidget * widget, gpointer data);
void file_new_cb(GtkWidget * widget, gpointer data);
void file_close_cb(GtkWidget * widget, gpointer data);
void file_close_all_cb(GtkWidget * widget, gpointer data);
void file_save_all_cb(GtkWidget * widget, gpointer data);

#endif /* __DOCUMENT_H_ */
