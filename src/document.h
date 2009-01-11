/* Bluefish HTML Editor
 * document.h - global function for document handling
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * Copyright (C) 1999-2008 Olivier Sessink
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

#ifndef __DOCUMENT_H_
#define __DOCUMENT_H_

typedef struct {
	Tdocument *doc;
	GtkTextMark *start;
	GtkTextMark *end;
} Tselectionsave;

enum {
	DOCUMENT_BACKUP_ABORT_SAVE,
	DOCUMENT_BACKUP_ABORT_ABORT,
	DOCUMENT_BACKUP_ABORT_ASK
};

Tselectionsave *doc_save_selection(Tdocument *doc);
void doc_restore_selection(Tselectionsave *selsave, gboolean only_if_no_selection);


void autoclosing_init(void);
GList *return_allwindows_documentlist(void);
typedef void (*foreachdocfunc) (Tdocument *doc, gpointer data);
void alldocs_foreach(foreachdocfunc func, gpointer data);

GList *return_urilist_from_doclist(GList *doclist);
gint documentlist_return_index_from_uri(GList *doclist, GFile *uri);
Tdocument *documentlist_return_document_from_uri(GList *doclist, GFile *uri);
Tdocument *documentlist_return_document_from_index(GList *doclist, gint index);
gint document_return_num_notcomplete(GList *doclist);

void doc_update_highlighting(Tbfwin *bfwin,guint callback_action, GtkWidget *widget);
void doc_set_wrap(Tdocument *doc);
void doc_set_tooltip(Tdocument *doc);
void doc_set_title(Tdocument *doc);
void doc_reset_filetype(Tdocument * doc, GFile *newuri, gconstpointer buf, gssize buflen);
void doc_set_font(Tdocument *doc, gchar *fontstring);
void doc_set_tabsize(Tdocument *doc, gint tabsize);
void gui_change_tabsize(Tbfwin *bfwin,guint action,GtkWidget *widget);
void session_set_savedir(Tbfwin *bfwin, gchar *curi);

gboolean doc_is_empty_non_modified_and_nameless(Tdocument *doc);
gboolean test_docs_modified(GList *doclist);
gboolean test_only_empty_doc_left(GList *doclist);

#define doc_has_selection(doc) gtk_text_buffer_get_has_selection(((Tdocument *)doc)->buffer)
void doc_set_status(Tdocument *doc, gint status);
void doc_set_modified(Tdocument *doc, gint value);
void doc_scroll_to_cursor(Tdocument *doc);
gchar *doc_get_chars(Tdocument *doc, gint start, gint end);
gint doc_get_max_offset(Tdocument *doc);
void doc_select_region(Tdocument *doc, gint start, gint end, gboolean do_scroll);
void doc_select_line(Tdocument *doc, gint line, gboolean do_scroll);
void doc_select_line_by_offset(Tdocument *doc, gint offset, gboolean do_scroll);
gboolean doc_get_selection(Tdocument *doc, gint *start, gint *end);
gint doc_get_cursor_position(Tdocument *doc);
void doc_set_statusbar_insovr(Tdocument *doc);
void doc_set_statusbar_editmode_encoding(Tdocument *doc);

/* the prototype for these functions is changed!! */
void doc_replace_text_backend(Tdocument *doc, const gchar * newstring, gint start, gint end);
void doc_replace_text(Tdocument *doc, const gchar * newstring, gint start, gint end);

void doc_insert_two_strings(Tdocument *doc, const gchar *before_str, const gchar *after_str);
gchar *buffer_find_encoding(gchar *buffer, gsize buflen, gchar **encoding, const gchar *sessionencoding);
gboolean doc_buffer_to_textbox(Tdocument * doc, gchar * buffer, gsize buflen, gboolean enable_undo, gboolean delay);
void doc_bind_signals(Tdocument *doc);
void doc_unbind_signals(Tdocument *doc);
gchar *doc_get_buffer_in_encoding(Tdocument *doc);
/* gboolean buffer_to_file(Tbfwin *bfwin, gchar *buffer, gchar *filename); */
void doc_set_fileinfo(Tdocument *doc, GFileInfo *finfo);
void doc_get_iter_location(Tdocument *doc, GtkTextIter *iter, GdkRectangle *rectange);
void doc_get_iter_at_bevent(Tdocument *doc, GdkEventButton *bevent, GtkTextIter *iter);
void doc_destroy(Tdocument * doc, gboolean delay_activation);
void document_unset_filename(Tdocument *doc);
gchar *ask_new_filename(Tbfwin *bfwin,gchar *oldfilename, const gchar *gui_name, gboolean is_move);
gint doc_save(Tdocument * doc, gboolean do_save_as, gboolean do_move, gboolean window_closing);
void document_set_line_numbers(Tdocument *doc, gboolean value);
void document_set_show_blocks(Tdocument *doc, gboolean value);
void document_set_show_symbols(Tdocument *doc, gboolean value);
Tdocument *doc_new(Tbfwin* bfwin, gboolean delay_activate);
/* void doc_new_with_new_file(Tbfwin *bfwin, gchar * new_filename); */
Tdocument *doc_new_loading_in_background(Tbfwin *bfwin, GFile *uri, GFileInfo *finfo, gboolean readonly);
void doc_new_from_uri(Tbfwin *bfwin, GFile *opturi, GFileInfo *finfo, gboolean delay_activate, gboolean move_to_this_win, gint goto_line, gint goto_offset);
void doc_new_from_input(Tbfwin *bfwin, gchar *input, gboolean delay_activate, gboolean move_to_this_win, gint goto_line);
void docs_new_from_uris(Tbfwin *bfwin, GSList *urislist, gboolean move_to_this_win);

Tdocument *doc_new_with_file(Tbfwin *bfwin, gchar * filename, gboolean delay_activate, gboolean move_to_this_win);
void docs_new_from_files(Tbfwin *bfwin, GList * file_list, gboolean move_to_this_win);
void doc_reload(Tdocument *doc,GFileInfo *newfinfo,gboolean warn_user);
void doc_start_modified_check(Tdocument *doc);
void doc_activate(Tdocument *doc);
void doc_force_activate(Tdocument *doc);
/* callbacks for the menu and toolbars */
void file_open_from_selection(Tbfwin *bfwin);

void file_insert_menucb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget);

void edit_cut_cb(GtkWidget * widget, Tbfwin *bfwin);
void edit_copy_cb(GtkWidget * widget, Tbfwin *bfwin);
void edit_paste_cb(GtkWidget * widget, Tbfwin *bfwin);
void edit_select_all_cb(GtkWidget * widget, Tbfwin *bfwin);

void doc_toggle_highlighting_cb(Tbfwin *bfwin,guint action,GtkWidget *widget);
void doc_toggle_wrap_cb(Tbfwin *bfwin,guint action,GtkWidget *widget);
void doc_toggle_linenumbers_cb(Tbfwin *bfwin,guint action,GtkWidget *widget);
void all_documents_apply_settings(void);

void word_count_cb (Tbfwin *bfwin,guint callback_action,GtkWidget *widget);
void doc_indent_selection(Tdocument *doc, gboolean unindent);
void menu_indent_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget);
GList * list_relative_document_filenames(Tdocument *curdoc);
void file_floatingview_menu_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget);

#endif /* __DOCUMENT_H_ */
