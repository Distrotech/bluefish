/* Bluefish HTML Editor
 * gui.h - global function for the GUI
 *
 * Copyright (C) 2002-2011 Olivier Sessink
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

#ifndef __GUI_H_
#define __GUI_H_

/*#define bfwin_exists(bfwin) (g_list_index(main_v->bfwinlist, bfwin)!=-1)*/

void gui_set_html_toolbar_visible(Tbfwin *bfwin, gboolean visible, gboolean sync_menu);
void gui_set_main_toolbar_visible(Tbfwin *bfwin, gboolean visible, gboolean sync_menu);
void gui_set_custom_menu_visible(Tbfwin *bfwin, gboolean visible, gboolean sync_menu);
void tb_fullscreen_cb(GtkWidget *widget, Tbfwin *bfwin);
void gui_fullscreen_cb(Tbfwin *bfwin,guint action,GtkWidget *widget);
void gui_toggle_hidewidget_cb(Tbfwin *bfwin,guint action,GtkWidget *widget);

void notebook_hide(Tbfwin *bfwin);
void notebook_show(Tbfwin *bfwin);
void notebook_changed(Tbfwin *bfwin,gint newpage);

void gui_notebook_move(Tbfwin *bfwin, gboolean move_left);
void gui_notebook_switch(Tbfwin *bfwin,guint action,GtkWidget *widget);
void left_panel_rebuild(Tbfwin *bfwin);
gboolean left_panel_show_hide_toggle(Tbfwin *bfwin,gboolean first_time, gboolean show, gboolean sync_menu);
void gui_statusbar_show_hide_toggle(Tbfwin *bfwin, gboolean visible, gboolean sync_menu);
void gui_set_title(Tbfwin *bfwin, Tdocument *doc, gint num_modified_change);
void gui_apply_session(Tbfwin *bfwin);
void gui_apply_settings(Tbfwin *bfwin);
void gui_set_undo_redo_widgets(Tbfwin *bfwin,gboolean undo, gboolean redo);
void gui_set_document_widgets(Tdocument *doc);

void gui_notebook_block_signals(Tbfwin *bfwin);
void gui_notebook_unblock_signals(Tbfwin *bfwin);
void gui_create_main(Tbfwin *bfwin);
void gui_show_main(Tbfwin *bfwin);
void statusbar_message(Tbfwin *bfwin,const gchar *message, gint seconds);
void all_bfwin_statusbar_message(const gchar *message, gint seconds);
/*gboolean bfwin_has_doc(Tbfwin *bfwin, Tdocument *doc);*/
gboolean switch_to_document_by_index(Tbfwin *bfwin,gint index);
gboolean switch_to_document_by_pointer(Tbfwin *bfwin,Tdocument *document);
gboolean switch_to_document_by_uri(Tbfwin *bfwin,GFile *uri);
/*void bfwin_docs_not_complete(Tbfwin *bfwin, gboolean increase);*/
gboolean main_window_delete_event_lcb(GtkWidget *widget,GdkEvent *event,Tbfwin *bfwin);
void gui_gotoline_frame_show(Tbfwin *bfwin,guint callback_action, GtkWidget *widget);
void go_to_line_from_selection_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget);

Tbfwin *gui_new_window(Tproject *project);
void gui_window_menu_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget);
#endif /* __GUI_H_ */
