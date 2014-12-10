/* Bluefish HTML Editor
 * document.h - global function for document handling
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * Copyright (C) 1999-2011 Olivier Sessink
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

Tselectionsave *doc_save_selection(Tdocument * doc);
void doc_restore_selection(Tselectionsave * selsave, gboolean only_if_no_selection);

GList *return_allwindows_documentlist(void);
typedef void (*foreachdocfunc) (Tdocument * doc, gpointer data);
void alldocs_foreach(foreachdocfunc func, gpointer data);
gint have_modified_documents(GList * doclist);
GList *return_urilist_from_doclist(GList * doclist);
GList *return_arraylist_from_doclist(GList * doclist);
void add_filename_to_recentlist(Tbfwin * bfwin, GFile * uri);
void remove_filename_from_recentlist(Tbfwin * bfwin, gboolean project, GFile * uri);

gint documentlist_return_index_from_uri(GList * doclist, GFile * uri);
void doc_set_uri(Tdocument *doc, GFile *uri, gboolean on_destroy);
Tdocument *documentlist_return_document_from_uri(GList * doclist, GFile * uri);
Tdocument *documentlist_return_document_from_index(GList * doclist, gint index);
gint document_return_num_notcomplete(GList * doclist);

void doc_update_highlighting(Tbfwin * bfwin, guint callback_action, GtkWidget * widget);
void doc_set_wrap(Tdocument * doc, gboolean enabled);
void doc_set_tooltip(Tdocument * doc);
void doc_set_title(Tdocument * doc, const gchar *override_label_string);
void doc_set_statusbar_lang_encoding(Tdocument * doc);
void doc_set_mimetype(Tdocument * doc, const gchar * mimetype, const gchar * filename);
void doc_reset_filetype(Tdocument * doc, GFile * newuri, gconstpointer buf, gssize buflen);
void doc_set_font(Tdocument * doc, gchar * fontstring);
void doc_set_tabsize(Tdocument * doc, gint tabsize);
gint doc_get_tabsize(Tdocument * doc);
void doc_change_tabsize(Tdocument * doc, gint direction);
void doc_font_size(Tdocument * doc, gint direction);
void doc_paste_special(Tbfwin *bfwin);
void doc_jump_matching_block_boundary(Tdocument *doc);

gboolean doc_is_empty_non_modified_and_nameless(Tdocument * doc);
gboolean test_docs_modified(GList * doclist);
gboolean test_only_empty_doc_left(GList * doclist);

#define doc_has_selection(doc) gtk_text_buffer_get_has_selection(((Tdocument *)doc)->buffer)
void doc_set_status(Tdocument * doc, gint status);
void doc_set_modified(Tdocument * doc, gint value);
void doc_select_and_scroll(Tdocument * doc, GtkTextIter * it1,
						   GtkTextIter * it2, gboolean select_it1_line, gboolean do_scroll, gboolean align_center);
void doc_scroll_to_cursor(Tdocument * doc);
gchar *doc_get_chars(Tdocument * doc, gint start, gint end);
gint doc_get_max_offset(Tdocument * doc);
/*void doc_select_region(Tdocument *doc, gint start, gint end, gboolean do_scroll);*/
void doc_select_line(Tdocument * doc, gint line, gboolean do_scroll);
void doc_select_line_by_offset(Tdocument * doc, gint offset, gboolean do_scroll, gboolean align_center);
gboolean doc_get_selection(Tdocument * doc, gint * start, gint * end);
gint doc_get_cursor_position(Tdocument * doc);
void doc_set_cursor_position(Tdocument * doc, gint cursor_offset);
/*void doc_set_statusbar_mimetype_encoding(Tdocument *doc);*/

/* the prototype for these functions is changed!! */
void doc_replace_text_backend(Tdocument * doc, const gchar * newstring, gint start, gint end);
void doc_replace_text(Tdocument * doc, const gchar * newstring, gint start, gint end);

void doc_insert_two_strings(Tdocument * doc, const gchar * before_str, const gchar * after_str);
gchar *buffer_find_encoding(gchar * buffer, gsize buflen, gchar ** encoding, const gchar * sessionencoding);
gboolean doc_buffer_to_textbox(Tdocument * doc, gchar * buffer, gsize buflen, gboolean enable_undo,
							   gboolean delay);

#define doc_block_undo_reg(doc) ((Tdocument *)doc)->block_undo_reg = 1;
#define doc_unblock_undo_reg(doc) ((Tdocument *)doc)->block_undo_reg = 0;

void update_encoding_meta_in_file(Tdocument * doc, gchar * encoding);
gchar *doc_get_buffer_in_encoding(Tdocument * doc, gsize *newbuflen);
/* gboolean buffer_to_file(Tbfwin *bfwin, gchar *buffer, gchar *filename); */
void doc_set_fileinfo(Tdocument * doc, GFileInfo * finfo);
void doc_set_readonly(Tdocument * doc, gboolean readonly);
void doc_get_iter_location(Tdocument * doc, GtkTextIter * iter, GdkRectangle * rectange);
void doc_destroy(Tdocument * doc, gboolean delay_activation);
void document_unset_filename(Tdocument * doc);
gint doc_save(Tdocument * doc, gboolean do_save_as, gboolean do_move, gboolean window_closing);
void document_set_show_symbols(Tdocument * doc, gboolean value);
void doc_recalculate_right_margin(Tdocument *doc);
void bfwin_alldoc_recalc_right_margin(Tbfwin *bfwin);
Tdocument *doc_new(Tbfwin * bfwin, gboolean delay_activate);
Tdocument *doc_new_with_template(Tbfwin * bfwin, GFile * uri, gboolean force_new);
Tdocument *doc_new_loading_in_background(Tbfwin * bfwin, GFile * uri, GFileInfo * finfo, gboolean readonly);
void doc_new_from_uri(Tbfwin * bfwin, GFile * opturi, GFileInfo * finfo, gboolean delay_activate,
					  gboolean move_to_this_win, gint goto_line, gint goto_offset, gint cursor_offset, gboolean align_center, gboolean load_first);
void doc_new_from_input(Tbfwin * bfwin, gchar * input, gboolean delay_activate, gboolean move_to_this_win,
						gint goto_line);
void docs_new_from_files(Tbfwin * bfwin, GList * file_list, gboolean move_to_this_win);
void doc_reload(Tdocument * doc, GFileInfo * newfinfo, gboolean warn_user);
void doc_start_modified_check(Tdocument * doc);
void doc_activate(Tdocument * doc);
void doc_force_activate(Tdocument * doc);

/* callbacks for the menu and toolbars */
void file_open_from_selection(Tbfwin * bfwin);

void file_insert_doc(Tbfwin * bfwin);

void doc_copy(Tbfwin * bfwin);
void doc_cut(Tbfwin * bfwin);
void doc_paste(Tbfwin * bfwin);
void doc_select_all(Tbfwin * bfwin);

void doc_toggle_highlighting(Tbfwin * bfwin, gboolean active);

void all_documents_apply_settings(void);

void doc_word_count(Tbfwin * bfwin);
void doc_indent_selection(Tdocument * doc, gboolean unindent);

GList *list_relative_document_filenames(Tdocument * curdoc);
void doc_jump(Tdocument * doc);

void doc_floating_view_new(Tbfwin * bfwin);
void doc_split_view(Tdocument * doc, gboolean enable);

#endif							/* __DOCUMENT_H_ */
