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

enum {
	DOCUMENT_BACKUP_ABORT_SAVE,
	DOCUMENT_BACKUP_ABORT_ABORT,
	DOCUMENT_BACKUP_ABORT_ASK
};

gint documentlist_return_index_from_filename(gchar *filename);
Tdocument *documentlist_return_document_from_index(gint index);

void doc_update_highlighting(GtkWidget *wid, gpointer data);
void document_set_wrap(Tdocument *doc, gint wraptype);
gboolean doc_set_filetype(Tdocument *doc, Tfiletype *ft);
Tfiletype *get_filetype_by_name(gchar * name);
Tfiletype *get_filetype_by_filename_and_content(gchar *filename, gchar *buf);
void doc_reset_filetype(Tdocument * doc, gchar * newfilename, gchar *buf);
void doc_set_font(Tdocument *doc, gchar *fontstring);
void doc_set_tabsize(Tdocument *doc, gint tabsize);
void gui_change_tabsize(gpointer callback_data,guint action,GtkWidget *widget);

gboolean doc_is_empty_non_modified_and_nameless(Tdocument *doc);
gboolean test_docs_modified(GList *doclist);
gboolean test_only_empty_doc_left();

gboolean doc_has_selection(Tdocument *doc);
void doc_set_modified(Tdocument *doc, gint value);
void doc_scroll_to_cursor(Tdocument *doc);
gchar *doc_get_chars(Tdocument *doc, gint start, gint end);
gint doc_get_max_offset(Tdocument *doc);
void doc_select_region(Tdocument *doc, gint start, gint end, gboolean do_scroll);
void doc_select_line(Tdocument *doc, gint line, gboolean do_scroll);
gboolean doc_get_selection(Tdocument *doc, gint *start, gint *end);
gint doc_get_cursor_position(Tdocument *doc);
void doc_set_statusbar_insovr(Tdocument *doc);
void doc_set_statusbar_editmode_encoding(Tdocument *doc);

/* the prototype for these functions is changed!! */
void doc_replace_text_backend(Tdocument *doc, const gchar * newstring, gint start, gint end);
void doc_replace_text(Tdocument *doc, const gchar * newstring, gint start, gint end);

void doc_insert_two_strings(Tdocument *doc, const gchar *before_str, const gchar *after_str);

void doc_bind_signals(Tdocument *doc);
void doc_unbind_signals(Tdocument *doc);

gchar *ask_new_filename(gchar *oldfilename, gboolean is_move);
gint doc_save(Tdocument * doc, gint do_save_as, gint do_move);

void doc_new_with_new_file(gchar * new_filename);
Tdocument *doc_new(gboolean delay_activate);
gboolean doc_new_with_file(gchar * filename, gboolean delay_activate);
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

void edit_cut_cb(GtkWidget * widget, gpointer data);
void edit_copy_cb(GtkWidget * widget, gpointer data);
void edit_paste_cb(GtkWidget * widget, gpointer data);
void edit_select_all_cb(GtkWidget * widget, gpointer data);

void doc_toggle_highlighting_cb(gpointer callback_data,guint action,GtkWidget *widget);
void doc_toggle_wrap_cb(gpointer callback_data,guint action,GtkWidget *widget);
void doc_toggle_linenumbers_cb(gpointer callback_data,guint action,GtkWidget *widget);

void doc_convert_asciichars_in_selection(gpointer callback_data,guint callback_action,GtkWidget *widget);
void word_count_cb (gpointer callback_data,guint callback_action,GtkWidget *widget);
void doc_indent_selection(Tdocument *doc, gboolean unindent);
void menu_indent_cb(gpointer callback_data,guint callback_action, GtkWidget  *widget);
GList * list_relative_document_filenames(Tdocument *curdoc);
#endif /* __DOCUMENT_H_ */
