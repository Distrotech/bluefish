/* Bluefish HTML Editor
 * gui.h - global function for the GUI
 *
 * Copyright (C) 2002 Olivier Sessink
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
#ifndef __GUI_H_
#define __GUI_H_
void gui_toggle_autoindent_cb(gpointer callback_data,guint action,GtkWidget *widget);
void gui_toggle_hidewidget_cb(Tbfwin *bfwin,guint action,GtkWidget *widget);

void notebook_hide(Tbfwin *bfwin);
void notebook_show(Tbfwin *bfwin);
void notebook_changed(Tbfwin *bfwin,gint newpage);

void gui_notebook_switch(Tbfwin *bfwin,guint action,GtkWidget *widget);
void gui_set_title(Tbfwin *bfwin, Tdocument *doc);
void gui_apply_settings(Tbfwin *bfwin);
void gui_set_undo_redo_widgets(Tbfwin *bfwin,gboolean undo, gboolean redo);
void gui_set_widgets(Tbfwin *bfwin,gboolean undo, gboolean redo, gboolean wrap, gboolean highlight, Tfiletype *hl, gchar *encoding, gboolean linenumbers);

void gui_notebook_bind_signals(Tbfwin *bfwin);
void gui_notebook_unbind_signals(Tbfwin *bfwin);
void gui_create_main(Tbfwin *bfwin, GList *filenames);
void gui_show_main(Tbfwin *bfwin);
void statusbar_message(Tbfwin *bfwin,gchar *message, gint time);
gboolean switch_to_document_by_index(Tbfwin *bfwin,gint index);
gboolean switch_to_document_by_pointer(Tbfwin *bfwin,Tdocument *document);
gboolean switch_to_document_by_filename(Tbfwin *bfwin,gchar *filename);
void go_to_line_win_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget);
void go_to_line_from_selection_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget);

void splash_screen_set_label(gchar *label);
GtkWidget *start_splash_screen(void);

Tbfwin *gui_new_window(GList *filenames);
void gui_window_menu_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget);
#endif /* __GUI_H_ */
