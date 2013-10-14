/*
 * Bluefish HTML Editor
 * bfwin.h
 *
 * Copyright (C) 2002-2011 Olivier Sessink
 * Copyright (C) 2011 James Hayward
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

#ifndef BFWIN_H_
#define BFWIN_H_

#include "bluefish.h"

typedef void (*DocDestroyCallback) (Tdocument *doc, gpointer data);
typedef void (*CurdocChangedCallback) (Tbfwin *bfwin, Tdocument *olddoc, Tdocument *newdoc, gpointer data);
typedef void (*DocDeleteRangeCallback) (Tdocument *doc, GtkTextIter * itstart, gint start, GtkTextIter * itend, gint end, const gchar *string, gpointer data);
typedef void (*DocInsertTextCallback) (Tdocument *doc, const gchar *string, GtkTextIter * iter, gint pos, gint len, gint clen, gpointer data);

#define bfwin_exists(bfwin) (g_list_index(main_v->bfwinlist, bfwin)!=-1)

void bfwin_fullscreen_toggle(Tbfwin * bfwin, gboolean active);
void bfwin_gotoline_search_bar_close(Tbfwin *bfwin, gboolean clean_entry_only);
void bfwin_gotoline_from_clipboard(Tbfwin * bfwin);
void bfwin_gotoline_frame_show(Tbfwin * bfwin);
void bfwin_simplesearch_show(Tbfwin *bfwin);
void bfwin_simplesearch_from_clipboard(Tbfwin *bfwin);

void bfwin_notebook_block_signals(Tbfwin * bfwin);
void bfwin_notebook_unblock_signals(Tbfwin * bfwin);

void bfwin_current_document_change_register(Tbfwin *bfwin, CurdocChangedCallback func, gpointer data);
void bfwin_current_document_change_remove_by_data(Tbfwin *bfwin, gpointer data);
void bfwin_current_document_change_remove_all(Tbfwin *bfwin);
void bfwin_document_insert_text_register(Tbfwin *bfwin, DocInsertTextCallback func, gpointer data);
void bfwin_document_insert_text_remove_by_data(Tbfwin *bfwin, gpointer data);
void bfwin_document_delete_range_register(Tbfwin *bfwin, DocDeleteRangeCallback func, gpointer data);
void bfwin_document_delete_range_remove_by_data(Tbfwin *bfwin, gpointer data);
void bfwin_document_destroy_register(Tbfwin *bfwin, DocDestroyCallback func, gpointer data);
void bfwin_document_destroy_remove_by_data(Tbfwin *bfwin, gpointer data);

void bfwin_notebook_changed(Tbfwin * bfwin, gint newpage);
void bfwin_notebook_switch(Tbfwin * bfwin, guint action);

void bfwin_main_toolbar_show(Tbfwin * bfwin, gboolean active);

void bfwin_output_pane_show(Tbfwin * bfwin, gboolean active);

gboolean bfwin_side_panel_show_hide_toggle(Tbfwin * bfwin, gboolean first_time, gboolean show,
										   gboolean sync_menu);
void bfwin_side_panel_show(Tbfwin * bfwin, gboolean active);
void bfwin_side_panel_rebuild(Tbfwin * bfwin);

void bfwin_statusbar_show(Tbfwin * bfwin, gboolean active);

void bfwin_apply_session(Tbfwin * bfwin, Tdocument *active_doc);
void bfwin_apply_settings(Tbfwin * bfwin);
void bfwin_destroy_and_cleanup(Tbfwin *bfwin);
gboolean bfwin_delete_event(GtkWidget * widget, GdkEvent * event, Tbfwin * bfwin);
#ifdef MAC_INTEGRATION
gboolean bfwin_osx_terminate_event(GtkWidget * widget, GdkEvent * event, Tbfwin * bfwin);
#endif

void bfwin_create_main(Tbfwin * bfwin);
void bfwin_show_main(Tbfwin * bfwin);

void bfwin_statusbar_message(Tbfwin * bfwin, const gchar * message, gint seconds);
void bfwin_all_statusbar_message(const gchar * message, gint seconds);

void bfwin_window_close(Tbfwin * bfwin);
Tbfwin *bfwin_window_new(void);
Tbfwin *bfwin_window_new_with_project(Tproject * project);

gboolean bfwin_has_doc(Tbfwin * bfwin, Tdocument * doc);
void bfwin_docs_not_complete(Tbfwin * bfwin, gboolean increase);
void bfwin_set_title(Tbfwin * bfwin, Tdocument * doc, gint num_modified_change);

gboolean bfwin_switch_to_document_by_index(Tbfwin * bfwin, gint index);
gboolean bfwin_switch_to_document_by_pointer(Tbfwin * bfwin, Tdocument * document);

#endif							/* BFWIN_H_ */
