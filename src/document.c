/* Bluefish HTML Editor
 * document.c - the document
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
#include <gtk/gtk.h>
#include <sys/types.h> /* stat() */
#include <sys/stat.h> /* stat() */
#include <unistd.h> /* stat() */
#include <stdio.h> /* fopen() */
#include <string.h> /* strchr() */
#include <regex.h> 				/* regcomp() */
#include <stdlib.h> /* system() */
#include <time.h> /* ctime_r() */

/* #define DEBUG */

#include "bluefish.h"
#include "document.h"
#include "highlight.h" /* all highlight functions */
#include "gui.h" /* statusbar_message() */
#include "bf_lib.h"
#include "menu.h" /* add_to_recent_list */
#include "stringlist.h" /* free_stringlist() */
#include "gtk_easy.h" /* error_dialog() */
#include "undo_redo.h" /* doc_unre_init() */
#include "rpopup.h" /* doc_bevent_in_html_tag(), rpopup_edit_tag_cb() */
#include "char_table.h" /* convert_utf8...() */
#include "pixmap.h"
#include "snr2.h" /* snr2_run_extern_replace */
#include "filebrowser.h"

typedef struct {
	Tdocument *last_activated_doc;
} Tlocals;

/******************************/
/* global vars for this module */
/******************************/
static Tlocals locals = {NULL};


void add_filename_to_history(gchar *filename) {
	gchar *dirname;

	add_to_recent_list(filename, 0); /* the recent menu */
	dirname = g_path_get_dirname(filename);
	main_v->recent_directories = add_to_history_stringlist(main_v->recent_directories,dirname);
	g_free(dirname);
}


/* gint documentlist_return_index_from_filename(gchar *filename)
 * returns -1 if the file is not open, else returns the index where
 * the file is in the documentlist
 */
gint documentlist_return_index_from_filename(gchar *filename) {
	GList *tmplist;
	gint count=0;

	if (!filename) {
		return -1;
	}
	
	tmplist = g_list_first(main_v->documentlist);
	while (tmplist) {
		if (((Tdocument *)tmplist->data)->filename &&(strcmp(filename, ((Tdocument *)tmplist->data)->filename) ==0)) {
			return count;
		}
		count++;
		tmplist = g_list_next(tmplist);
	}
	return -1;
}

void doc_update_highlighting(GtkWidget *wid, gpointer data) {
	if (!main_v->current_document) return;
	DEBUG_MSG("doc_update_highlighting, curdoc=%p, highlightstate=%d\n", main_v->current_document, main_v->current_document->highlightstate);
	if (main_v->current_document->highlightstate == 0) {
		setup_toggle_item(gtk_item_factory_from_widget(main_v->menubar),
			  N_("/Document/Highlight syntax"), TRUE);
		DEBUG_MSG("doc_update_highlighting, calling doc_toggle_highlighting_cb\n");
		doc_toggle_highlighting_cb(NULL, 0, NULL);
	} else {
		doc_highlight_full(main_v->current_document);
	}
}

/* void doc_set_wrap(Tdocument * document, gint wraptype)
 * type=0 = none, type=1=word wrap
 */

void doc_set_wrap(Tdocument * doc) {
	if (doc->wrapstate) {
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(doc->view),GTK_WRAP_WORD);
	} else {
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(doc->view),GTK_WRAP_NONE);
	}
}

#ifdef __GNUC__
__inline__ 
#endif
void doc_set_font(Tdocument *doc, gchar *fontstring) {
	if (fontstring) {
		apply_font_style(doc->view, fontstring);
	} else {
		apply_font_style(doc->view, main_v->props.editor_font_string);
	}
}

/* This function is taken from gtksourceview
 * Copyright (C) 2001
 * Mikael Hermansson <tyan@linux.se>
 * Chris Phelps <chicane@reninet.com>
 */
static gint textview_calculate_real_tab_width(GtkWidget *textview, gint tab_size) {
	PangoLayout *layout;
	gchar *tab_string;
	gint counter = 0;
	gint tab_width = 0;

	if (tab_size <= 0)
		return 0;

	tab_string = g_malloc (tab_size + 1);
	while (counter < tab_size) {
		tab_string[counter] = ' ';
		counter++;
	}
	tab_string[tab_size] = '\0';
	layout = gtk_widget_create_pango_layout(textview, tab_string);
	g_free(tab_string);

	if (layout != NULL) {
		pango_layout_get_pixel_size (layout, &tab_width, NULL);
		g_object_unref (G_OBJECT (layout));
	} else {
		tab_width = 0;
	}
	return tab_width;
}

void doc_set_tabsize(Tdocument *doc, gint tabsize) {
	PangoTabArray *tab_array;
	gint pixels = textview_calculate_real_tab_width(GTK_WIDGET(doc->view), tabsize);
	DEBUG_MSG("doc_set_tabsize, tabsize=%d, pixels=%d\n", tabsize, pixels);
	tab_array = pango_tab_array_new (1, TRUE);
	pango_tab_array_set_tab (tab_array, 0, PANGO_TAB_LEFT, pixels);
	gtk_text_view_set_tabs (GTK_TEXT_VIEW (doc->view), tab_array);
	pango_tab_array_free(tab_array);
}

void gui_change_tabsize(gpointer callback_data,guint action,GtkWidget *widget) {
	GList *tmplist;
	PangoTabArray *tab_array;
	gint pixels;
	if (action) {
		main_v->props.editor_tab_width++;
	} else {
		main_v->props.editor_tab_width--;
	}
	{
		gchar *message = g_strdup_printf("Setting tabsize to %d", main_v->props.editor_tab_width);
		statusbar_message(message, 2000);
		g_free(message);
	}
	tmplist = g_list_first(main_v->documentlist);
	pixels = textview_calculate_real_tab_width(GTK_WIDGET(((Tdocument *)tmplist->data)->view), main_v->props.editor_tab_width);
	tab_array = pango_tab_array_new (1, TRUE);
	pango_tab_array_set_tab (tab_array, 0, PANGO_TAB_LEFT, pixels);
	while (tmplist) {
		gtk_text_view_set_tabs (GTK_TEXT_VIEW(((Tdocument *)tmplist->data)->view), tab_array);
		tmplist = g_list_next(tmplist);
	}
	pango_tab_array_free(tab_array);
}

gboolean doc_is_empty_non_modified_and_nameless(Tdocument *doc) {
	if (!doc) {
		return FALSE;
	}
	if (doc->modified || doc->filename) {
		return FALSE;
	}
	if (gtk_text_buffer_get_char_count(doc->buffer) > 0) {
		return FALSE;
	}
	return TRUE;
}


/* gboolean test_docs_modified(GList *doclist)
 * if doclist is NULL it will use main_v->documentlist as doclist
 * returns TRUE if there are any modified documents in doclist
 * returns FALSE if there are no modified documents in doclist
 */
gboolean test_docs_modified(GList *doclist) {

	GList *tmplist;
	Tdocument *tmpdoc;

	if (doclist) {
		tmplist = g_list_first(doclist);
	} else {
		tmplist = g_list_first(main_v->documentlist);
	}
	
	while (tmplist) {
		tmpdoc = (Tdocument *) tmplist->data;
#ifdef DEBUG
		g_assert(tmpdoc);
#endif
		if (tmpdoc->modified) {
			return TRUE;
		}
		tmplist = g_list_next(tmplist);
	}
	return FALSE;
}
/* gint test_only_empty_doc_left()
 * returns TRUE if there is only 1 document open, and that document
 * is not modified and 0 bytes long and without filename
 * returns FALSE if there are multiple documents open, or 
 * a modified document is open, or a > 0 bytes document is open
 * or a document with filename is open
 */
gboolean test_only_empty_doc_left() {
	if (g_list_length(main_v->documentlist) > 1) {
		return FALSE;
	} else {
		Tdocument *tmpdoc;
		GList *tmplist = g_list_first(main_v->documentlist);
		if (tmplist) {
#ifdef DEBUG
			g_assert(tmplist->data);
#endif
			tmpdoc = tmplist->data;
			if (!doc_is_empty_non_modified_and_nameless(tmpdoc)) {
				return FALSE;
			}
		}
	}
	return TRUE;
}

gboolean doc_has_selection(Tdocument *doc) {
	GtkTextMark* tmp;
	GtkTextIter itinsert, itselect;
	tmp = gtk_text_buffer_get_insert(doc->buffer);
	gtk_text_buffer_get_iter_at_mark(doc->buffer,&itinsert,tmp);
	tmp = gtk_text_buffer_get_selection_bound(doc->buffer);
	gtk_text_buffer_get_iter_at_mark(doc->buffer,&itselect,tmp);
	return (!gtk_text_iter_equal(&itinsert, &itselect));
}

void doc_set_modified(Tdocument *doc, gint value) {
	DEBUG_MSG("doc_set_modified, started, doc=%p, value=%d\n", doc, value);
	if (doc->modified != value) {
		gchar *label_string;
		doc->modified = value;
		if (doc->modified) {
			if (doc->filename) {
				gchar *tmpstr = g_path_get_basename(doc->filename);
				label_string = g_strdup(tmpstr);
				g_free(tmpstr);
			} else {
				label_string = g_strdup(_("Untitled"));
			}
			{
				GdkColor color = {0, 65535, 0, 0};
				gtk_widget_modify_fg(doc->tab_menu, GTK_STATE_NORMAL, &color);
				gtk_widget_modify_fg(doc->tab_menu, GTK_STATE_PRELIGHT, &color);
				gtk_widget_modify_fg(doc->tab_label, GTK_STATE_NORMAL, &color);
			}
		} else {
			if (doc->filename) {
				gchar *tmpstr = g_path_get_basename(doc->filename);
				label_string = g_strdup(tmpstr);
				g_free(tmpstr);
			} else {
				label_string = g_strdup(_("Untitled"));
			}
			{
				GdkColor color = {0, 0, 0, 0};
				gtk_widget_modify_fg(doc->tab_menu, GTK_STATE_NORMAL, &color);
				gtk_widget_modify_fg(doc->tab_menu, GTK_STATE_PRELIGHT, &color);
				gtk_widget_modify_fg(doc->tab_label, GTK_STATE_NORMAL, &color);
			}
		}
		if (doc->filename) {
			if (doc->modified) {
				gchar *tmpstr = g_path_get_basename(doc->filename);
				label_string = g_strdup(tmpstr);
				g_free(tmpstr);
			} else {
				gchar *tmpstr = g_path_get_basename(doc->filename);
				label_string = g_strdup(tmpstr);
				g_free(tmpstr);
			}
			gtk_label_set(GTK_LABEL(doc->tab_menu),doc->filename);
		} else {
			if (doc->modified) {
				label_string = g_strdup(_("Untitled"));
			} else {
				label_string = g_strdup(_("Untitled"));
			}
			gtk_label_set(GTK_LABEL(doc->tab_menu),label_string);
		}
		gtk_label_set(GTK_LABEL(doc->tab_label),label_string);
		g_free(label_string);
	}
	/* only when this is the current document we have to change these */
	if (doc == main_v->current_document) {
		gui_set_undo_redo_widgets(doc_has_undo_list(doc), doc_has_redo_list(doc));
	}
#ifdef DEBUG
	else {
		DEBUG_MSG("doc_set_modified, doc != current_document, so we do not update the gui widgets\n");
	}
#endif
}

inline static void doc_update_mtime(Tdocument *doc) {
	if (doc->filename) {
		struct stat statbuf;
		if (stat(doc->filename, &statbuf) == 0) {
			doc->mtime = statbuf.st_mtime;
		}
	} else {
		doc->mtime = 0;
	}
}
/* returns 1 if the time didn't change, returns 0 
if the file is modified by another process, returns
1 if there was no previous mtime information available 
if newtime is not NULL, it will be filled with the new mtime from the file IF IT WAS CHANGED!!!
leave NULL if you do not need this information, if the file is not changed, this field will not be set!!
*/
static gint doc_check_mtime(Tdocument *doc, time_t *newtime) {
	if (doc->filename && 0 != doc->mtime) {
		struct stat statbuf;
		if (stat(doc->filename, &statbuf) == 0) {
			if (doc->mtime < statbuf.st_mtime) {
				DEBUG_MSG("doc_check_mtime, doc->mtime=%d, statbuf.st_mtime=%d\n", (int)doc->mtime, (int)statbuf.st_mtime);
				if (newtime) {
					*newtime = statbuf.st_mtime;
				}
				return 0;
			}
		}
	} 
	return 1;
}

/* doc_set_stat_info() includes setting the mtime field, so there is no need
to call doc_update_mtime() as well */
static void doc_set_stat_info(Tdocument *doc) {
	if (doc->filename) {
		struct stat statbuf;
		if (lstat(doc->filename, &statbuf) == 0) {
			if (S_ISLNK(statbuf.st_mode)) {
				doc->is_symlink = 1;
				stat(doc->filename, &statbuf);
			} else {
				doc->is_symlink = 0;
			}
			doc->mtime = statbuf.st_mtime;
			doc->owner_uid = statbuf.st_uid;
			doc->owner_gid = statbuf.st_gid;
			doc->mode = statbuf.st_mode;
		}
	}
}

/*static void doc_scroll_to_line(Tdocument *doc, gint linenum, gboolean select_line) {
	GtkTextIter itstart;

	gtk_text_buffer_get_iter_at_line(doc->buffer,&itstart,linenum);
	gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(doc->view),&itstart,0.25,FALSE,0.5,0.5);
	if (select_line) {
		GtkTextIter itend = itstart;
		gtk_text_iter_forward_to_line_end(&itend);
		gtk_text_buffer_place_cursor (doc->buffer, &itstart);
		gtk_text_buffer_move_mark_by_name(doc->buffer,"selection_bound",&itend);
	}
}*/

void doc_scroll_to_cursor(Tdocument *doc) {
	GtkTextMark *mark = gtk_text_buffer_get_insert(doc->buffer);
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(doc->view),mark,0.25,FALSE,0.5,0.5);
}

/* gchar *doc_get_chars(Tdocument *doc, gint start, gint end)
 * if end == -1 it will return everrything to the end
 */
gchar *doc_get_chars(Tdocument *doc, gint start, gint end) {
	GtkTextIter itstart, itend;
	gchar *string;

	gtk_text_buffer_get_iter_at_offset(doc->buffer, &itstart,start);
	if (end >= 0) {
		gtk_text_buffer_get_iter_at_offset(doc->buffer, &itend, end);
	} else if (end == -1) {
		gtk_text_buffer_get_end_iter(doc->buffer, &itend);
	} else {
		DEBUG_MSG("doc_get_chars, end < -1, returning NULL\n");
		return NULL;
	}
	DEBUG_MSG("doc_get_chars, retrieving string, start=%d, end=%d\n", start, end);
	string = gtk_text_buffer_get_text(doc->buffer, &itstart, &itend,FALSE);
	DEBUG_MSG("doc_get_chars, retrieved string (%p)\n", string);
	return string;
}

gint doc_get_max_offset(Tdocument *doc) {
	return gtk_text_buffer_get_char_count(doc->buffer);
}

/*  void doc_select_region(Tdocument *doc, gint start, gint end, gboolean do_scroll)
 *  selects from start to end in the doc, and if do_scroll is set it will make
 *  sure the selection is visible to the user
 */
void doc_select_region(Tdocument *doc, gint start, gint end, gboolean do_scroll) {
	/* how do we select a region in gtk2?? */
	GtkTextIter itstart, itend;
	gtk_text_buffer_get_iter_at_offset(doc->buffer, &itstart,start);
	gtk_text_buffer_get_iter_at_offset(doc->buffer, &itend,end);
	gtk_text_buffer_move_mark_by_name(doc->buffer, "insert", &itstart);
	gtk_text_buffer_move_mark_by_name(doc->buffer, "selection_bound", &itend);
	if (do_scroll) {
		gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(doc->view),&itstart,0.25,FALSE,0.5,0.5);
	}
}

/*void doc_select_line(Tdocument *doc, gint line, gboolean do_scroll)
 * the line starts at 1, not at 0 !!
 */

void doc_select_line(Tdocument *doc, gint line, gboolean do_scroll) {
	GtkTextIter itstart, itend;
	gtk_text_buffer_get_iter_at_line(doc->buffer,&itstart,line-1);
	itend = itstart;
	gtk_text_iter_forward_to_line_end(&itend);
	gtk_text_buffer_move_mark_by_name(doc->buffer, "insert", &itstart);
	gtk_text_buffer_move_mark_by_name(doc->buffer, "selection_bound", &itend);
	if (do_scroll) {
		gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(doc->view),&itstart,0.25,FALSE,0.5,0.5);	
	}
}

/*  gboolean doc_get_selection(Tdocument *, gint *start, gint *end)
 *  returns FALSE if there is no selection
 *  returns TRUE if there is a selection, and start and end will be set
 *  to the current selection
 */
gboolean doc_get_selection(Tdocument *doc, gint *start, gint *end) {
	GtkTextIter itstart, itend;
	GtkTextMark *mark = gtk_text_buffer_get_insert(doc->buffer);
	gtk_text_buffer_get_iter_at_mark(doc->buffer,&itstart,mark);
	mark = gtk_text_buffer_get_selection_bound(doc->buffer);
	gtk_text_buffer_get_iter_at_mark(doc->buffer,&itend,mark);
	*start = gtk_text_iter_get_offset(&itstart);
	*end = gtk_text_iter_get_offset(&itend);
	DEBUG_MSG("doc_get_selection, start=%d, end=%d\n", *start, *end);
	if (*start == *end) {
		return FALSE;
	}
	if (*start > *end) {
		gint tmp = *start;
		*start = *end;
		*end = tmp;
	}
	return TRUE;
}

gint doc_get_cursor_position(Tdocument *doc) {
	GtkTextIter iter;
	GtkTextMark *mark = gtk_text_buffer_get_insert(doc->buffer);
	gtk_text_buffer_get_iter_at_mark(doc->buffer, &iter, mark);
	return gtk_text_iter_get_offset(&iter);
}

void doc_replace_text_backend(Tdocument *doc, const gchar * newstring, gint start, gint end) {
	doc_unbind_signals(doc);
	/* delete region, and add that to undo/redo list */
	{
		gchar *buf;
		GtkTextIter itstart, itend;
		DEBUG_MSG("doc_replace_text_backend, get iters at start %d and end %d\n", start, end);
		gtk_text_buffer_get_iter_at_offset(doc->buffer, &itstart,start);
		gtk_text_buffer_get_iter_at_offset(doc->buffer, &itend,end);
		buf = gtk_text_buffer_get_text(doc->buffer, &itstart, &itend,FALSE);
		gtk_text_buffer_delete(doc->buffer,&itstart,&itend);
		DEBUG_MSG("doc_replace_text_backend, calling doc_unre_add for buf=%s, start=%d and end=%d\n", buf, start, end);
		doc_unre_add(doc, buf, start, end, UndoDelete);
		g_free(buf);
		DEBUG_MSG("doc_replace_text_backend, text deleted from %d to %d\n", start, end);
	}

	/* add new text to this region, the buffer is changed so re-calculate itstart */
	{
		GtkTextIter itstart;
		gint insert = (end > start) ? start : end;
		DEBUG_MSG("doc_replace_text_backend, set insert pos to %d\n", insert);
		gtk_text_buffer_get_iter_at_offset(doc->buffer, &itstart,insert);
		gtk_text_buffer_insert(doc->buffer,&itstart,newstring,-1);
		doc_unre_add(doc, newstring, insert, insert + g_utf8_strlen(newstring,-1), UndoInsert);
	}
	doc_bind_signals(doc);
	doc_set_modified(doc, 1);
	doc->need_highlighting=TRUE;
}					  

void doc_replace_text(Tdocument * doc, const gchar * newstring, gint start, gint end) {
	doc_unre_new_group(doc);

	doc_replace_text_backend(doc, newstring, start, end);
	
	doc_set_modified(doc, 1);
	doc_unre_new_group(doc);
/*	doc_need_highlighting(doc);*/
}

static void doc_convert_chars_to_entities(Tdocument *doc, gint start, gint end, gboolean ascii, gboolean iso) {
	gchar *string;
	DEBUG_MSG("doc_convert_chars_to_entities, start=%d, end=%d\n", start,end);
	string = doc_get_chars(doc, start, end);
	if (string) {
		gchar *newstring = convert_string_utf8_to_html(string, ascii, iso);
		g_free(string);
		if (newstring) {
			doc_replace_text(doc, newstring, start, end);
			g_free(newstring);
		}
#ifdef DEBUG
		 else {
		 	DEBUG_MSG("doc_convert_chars_to_entities, newstring=NULL\n");
		 }
#endif
	}
#ifdef DEBUG
		 else {
		 	DEBUG_MSG("doc_convert_chars_to_entities, string=NULL\n");
		 }
#endif		 
}

static void doc_convert_chars_to_entities_in_selection(Tdocument *doc, gboolean ascii, gboolean iso) {
	gint start, end;
	if (doc_get_selection(doc, &start, &end)) {
		DEBUG_MSG("doc_convert_chars_to_entities_in_selection, start=%d, end=%d\n", start, end);
		doc_convert_chars_to_entities(doc, start, end, ascii, iso);
	}
}

/* inserts the first string at pos1 and the second at pos2 in doc
 * if the marks 'diag_ins' and 'diag_sel' exist, they will be used
 * if not, insert and selection_bound will be used
 */

void doc_insert_two_strings(Tdocument *doc, const gchar *before_str, const gchar *after_str) {
	GtkTextIter itinsert, itselect;
	GtkTextMark *insert, *select;
	insert = gtk_text_buffer_get_mark(doc->buffer,"diag_ins");
	if (insert) {
		select = gtk_text_buffer_get_mark(doc->buffer,"diag_sel");
	} else {
		insert = gtk_text_buffer_get_insert(doc->buffer);
		select = gtk_text_buffer_get_selection_bound(doc->buffer);
	}
	gtk_text_buffer_get_iter_at_mark(doc->buffer,&itinsert,insert);
	gtk_text_buffer_get_iter_at_mark(doc->buffer,&itselect,select);
#ifdef DEBUG
	g_print("doc_insert_two_strings, current marks: insert=%d, selection=%d\n",gtk_text_iter_get_offset(&itinsert),gtk_text_iter_get_offset(&itselect));
#endif	

	if (gtk_text_iter_equal(&itinsert, &itselect)) {
		/* no selection */
		gchar *double_str = g_strconcat(before_str, after_str, NULL);
		gtk_text_buffer_insert(doc->buffer,&itinsert,double_str,-1);
		g_free(double_str);
		if (after_str && strlen(after_str)) {
			/* the buffer has changed, renew the iterator */
			gtk_text_buffer_get_iter_at_mark(doc->buffer,&itinsert,insert);
			/* now set it between the two strings, this is definately not multibyte-char 
			safe, since strlen() returns bytes, and the functions wants chars */
			gtk_text_iter_backward_chars(&itinsert, strlen(after_str));
			gtk_text_buffer_place_cursor(doc->buffer, &itinsert);
		}
	} else { /* there is a selection */
		GtkTextMark *marktoresetto;
		GtkTextIter firstiter;
		if (gtk_text_iter_compare(&itinsert,&itselect) < 0) {
			firstiter = itinsert;
			marktoresetto = select;
		} else {
			firstiter = itselect;
			marktoresetto = insert;
		}
		/* there is a selection */
		gtk_text_buffer_insert(doc->buffer,&firstiter,before_str,-1);
		if (after_str && strlen(after_str)) {
			/* the buffer is changed, reset the select iterator */
			gtk_text_buffer_get_iter_at_mark(doc->buffer,&itselect,marktoresetto);
			gtk_text_buffer_insert(doc->buffer,&itselect,after_str,-1);
		}
	}
	doc_unre_new_group(doc);
	DEBUG_MSG("doc_insert_two_strings, finished\n");
}

static void add_encoding_to_list(gchar *encoding) {
	gchar **enc;
	GList *tmplist = g_list_first(main_v->props.encodings);
	while (tmplist) {
		gchar **arr = tmplist->data;
		if (strcmp(arr[0], encoding)==0) {
			return;
		}
		tmplist = g_list_next(tmplist);	
	}
	/* it is not yet in the list */
	enc = g_malloc0(sizeof(gchar *)*3);
	enc[0] = g_strdup(encoding);
	enc[1] = g_strdup(encoding);
	main_v->props.encodings = g_list_append(main_v->props.encodings, enc);
	encoding_menu_rebuild();
}


#define STARTING_BUFFER_SIZE 4096
gboolean doc_file_to_textbox(Tdocument * doc, gchar * filename, gboolean enable_undo, gboolean delay_highlighting)
{
	FILE *fd;
	gchar *errmessage, *message;
	gint cursor_offset;

	if (!enable_undo) {
		doc_unbind_signals(doc);
	}
	message = g_strconcat(_("Opening file "), filename, NULL);
	statusbar_message(message, 1000);
	g_free(message);
	/* This opens the contents of a file to a textbox */
	change_dir(filename);
	fd = fopen(filename, "r");
	if (fd == NULL) {
		DEBUG_MSG("file_to_textbox, cannot open file %s\n", filename);
		errmessage =
			g_strconcat(_("Could not open file:\n"), filename, NULL);
		error_dialog(_("Error"), errmessage);	/* 7 */
		g_free(errmessage);
		if (!enable_undo) {
			doc_bind_signals(doc);
		}
		return FALSE;
	}
	/* now get the current cursor position */
	{
		GtkTextMark* insert;
		GtkTextIter iter;
		insert = gtk_text_buffer_get_insert(doc->buffer);
		gtk_text_buffer_get_iter_at_mark(doc->buffer, &iter, insert);
		cursor_offset = gtk_text_iter_get_offset(&iter);
	}
	{
/*		gchar chunk[STARTING_BUFFER_SIZE+1];
		gint buffer_size = STARTING_BUFFER_SIZE + 1;
		gchar *buffer = g_malloc(buffer_size * sizeof(gchar));
		buffer[0] = '\0';
		while (fgets(chunk, STARTING_BUFFER_SIZE, fd) != NULL) {
			strcat(buffer, chunk);
			buffer_size += STARTING_BUFFER_SIZE;
			buffer = g_realloc(buffer, buffer_size);
			DEBUG_MSG("doc_file_to_textbox, buffer_size=%d, strlen(buffer)=%d\n",buffer_size, strlen(buffer));
		}
		DEBUG_MSG("doc_file_to_textbox, final buffer_size=%d, strlen(buffer)=%d\n", buffer_size, strlen(buffer));*/
		gchar chunk[STARTING_BUFFER_SIZE];
		gint buffer_size = STARTING_BUFFER_SIZE;
		size_t size = STARTING_BUFFER_SIZE;
		gchar *buffer = g_malloc(1+(buffer_size * sizeof(gchar)));
		while (TRUE) {
			size = fread(chunk,sizeof(gchar),STARTING_BUFFER_SIZE,fd);
			memcpy(&buffer[buffer_size-STARTING_BUFFER_SIZE], chunk, STARTING_BUFFER_SIZE);
			DEBUG_MSG("doc_file_to_textbox, copy %d bytes to position %d, size=%d\n",STARTING_BUFFER_SIZE, buffer_size-STARTING_BUFFER_SIZE, size);
			if (size == STARTING_BUFFER_SIZE) {
				buffer_size += STARTING_BUFFER_SIZE;
				buffer = g_realloc(buffer, 1+(buffer_size * sizeof(gchar)));
			} else {
				DEBUG_MSG("size=%d, terminating buffer at %d\n",size,buffer_size-STARTING_BUFFER_SIZE+size);
				buffer[buffer_size-STARTING_BUFFER_SIZE+size] = '\0';
				break;
			}
		}
		DEBUG_MSG("doc_file_to_textbox, final buffer_size=%d, strlen(buffer)=%d\n", buffer_size, strlen(buffer));
		fclose(fd);
		
		/* the first try is if the encoding is set in the file */
		{
			regex_t preg;
			regmatch_t pmatch[10];
			gint retval;
			gchar *encoding=NULL;
			gchar *newbuf=NULL;
			gsize wsize;
			GError *error=NULL;
			gchar *pattern = "<meta[ \t\n]http-equiv[ \t\n]*=[ \t\n]*\"content-type\"[ \t\n]+content[ \t\n]*=[ \t\n]*\"text/html;[ \t\n]*charset=([a-z0-9-]+)\"[ \t\n]*>";
			retval = regcomp(&preg,pattern,REG_EXTENDED|REG_ICASE);
#ifdef DEBUG
			if (retval) {
				g_print("regcomp error!\n");
			}
#endif
			retval = regexec(&preg,buffer,10,pmatch,0);
#ifdef DEBUG
			if (retval) {
				gchar errbuf[1024];
				regerror(retval, &preg, errbuf, 1024);
				g_print("regexec error! %s\n", errbuf);
			}
#endif
			if (retval==0 && pmatch[0].rm_so != -1 && pmatch[1].rm_so != -1) {
				/* we have a match */
				DEBUG_MSG("doc_file_to_textbox, match so=%d,eo=%d\n", pmatch[1].rm_so,pmatch[1].rm_eo);
				encoding = g_strndup(&buffer[pmatch[1].rm_so], pmatch[1].rm_eo-pmatch[1].rm_so);
				DEBUG_MSG("doc_file_to_textbox, detected encoding %s\n", encoding);
			}
			regfree(&preg);
			if (encoding) {
				DEBUG_MSG("doc_file_to_textbox, try encoding %s\n", encoding);
				newbuf = g_convert(buffer,-1,"UTF-8",encoding,NULL, &wsize, &error);
				if (!newbuf || error) {
					DEBUG_MSG("doc_file_to_textbox, cound not convert to UTF-8: \n");
				} else {
					if (doc->encoding) {
						g_free(doc->encoding);
					}
					add_encoding_to_list(encoding);
					doc->encoding = g_strdup(encoding);
					g_free(buffer);
					buffer = newbuf;
				}
				g_free(encoding);
			}
			if (!newbuf) {
				DEBUG_MSG("doc_file_to_textbox, file is converted yet, trying UTF-8 encoding\n");
				if(!g_utf8_validate(buffer, -1, NULL)) {
					DEBUG_MSG("doc_file_to_textbox, file is not in UTF-8, trying encoding from locale\n");
					newbuf = g_locale_to_utf8(buffer,-1,NULL,&wsize,NULL);
					if (!newbuf) {
						DEBUG_MSG("doc_file_to_textbox, file is not converted, trying default encoding (from configfile) %s\n", main_v->props.newfile_default_encoding);
						newbuf = g_convert(buffer,-1,"UTF-8",main_v->props.newfile_default_encoding,NULL, &wsize, NULL);
						if (!newbuf) {
							GList *tmplist;
							DEBUG_MSG("doc_file_to_textbox, tried the most obvious encodings, nothing found.. go trough list\n");
							tmplist = g_list_first(main_v->props.encodings);
							while (tmplist) {
								gchar **enc = tmplist->data;
								DEBUG_MSG("doc_file_to_textbox, trying encoding %s\n", enc[1]);
								newbuf = g_convert(buffer,-1,"UTF-8",enc[1],NULL, &wsize, NULL);
								if (newbuf) {
									if (doc->encoding) {
										g_free(doc->encoding);
									}
									doc->encoding = g_strdup(enc[1]);
/*									g_free(buffer);
									buffer = newbuf;*/
									tmplist = NULL;
								} else {
									DEBUG_MSG("doc_file_to_textbox, no newbuf, next in list\n");
									tmplist = g_list_next(tmplist);
								}
							}
							if (!newbuf) {
								error_dialog(_("Error"), _("Cannot display file, unknown characters found."));
							}
						} else {
							DEBUG_MSG("doc_file_to_textbox, file is in %s encoding\n", main_v->props.newfile_default_encoding);
							if (doc->encoding) {
								g_free(doc->encoding);
							}
							doc->encoding = g_strdup(main_v->props.newfile_default_encoding);
						}
					} else {
						const gchar *encoding=NULL;
						g_get_charset(&encoding);
						DEBUG_MSG("doc_file_to_textbox, file is in locale encoding: %s\n", encoding);
						g_free(doc->encoding);
						doc->encoding = g_strdup(encoding);
						add_encoding_to_list(doc->encoding);
					}
					DEBUG_MSG("doc_file_to_textbox, freeing buffer, buffer=newbuf\n");
					g_free(buffer);
					buffer = newbuf;
				} else {
					DEBUG_MSG("doc_file_to_textbox, file contains valid UTF-8\n");
					g_free(doc->encoding);
					doc->encoding = g_strdup("UTF-8");
				}
			}
		}
		if (buffer) {
			gtk_text_buffer_insert_at_cursor(doc->buffer,buffer,-1);
			g_free(buffer);
		}
	}
	if (doc->highlightstate) {
		doc->need_highlighting=TRUE;
		DEBUG_MSG("doc_file_to_textbox, highlightstate=%d, need_highlighting=%d, delay_highlighting=%d\n",doc->highlightstate,doc->need_highlighting,delay_highlighting);
		if (!delay_highlighting) {
#ifdef DEBUG
			g_print("doc_file_to_textbox, doc->hlset=%p\n", doc->hl);
			if (doc->hl) {
				g_print("doc_file_to_textbox, doc->hlset->highlightlist=%p\n", doc->hl->highlightlist);
			}
#endif
			doc_highlight_full(doc);
		}
	}
	if (!enable_undo) {
		doc_bind_signals(doc);
	}
	
	/* set the cursor position back */
	{
		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_offset(doc->buffer,&iter,cursor_offset);
		gtk_text_buffer_place_cursor(doc->buffer,&iter);
		if (!delay_highlighting) {
			gtk_text_view_place_cursor_onscreen(GTK_TEXT_VIEW(doc->view));
		}
	}
	return TRUE;
}

/* static gint doc_check_backup(Tdocument *doc)
 * returns 1 on success, 0 on failure
 * if no backup is required, or no filename known, 1 is returned
 */
static gint doc_check_backup(Tdocument *doc) {
	gint res = 1;

	if (main_v->props.backup_file && doc->filename && file_exists_and_readable(doc->filename)) {
		gchar *backupfilename;
		backupfilename = g_strconcat(doc->filename, main_v->props.backup_filestring, NULL);
		res = file_copy(doc->filename, backupfilename);
		if (doc->owner_uid != -1 && !doc->is_symlink) {
			chmod(backupfilename, doc->mode);
			chown(backupfilename, doc->owner_uid, doc->owner_gid);
		}
		g_free(backupfilename);
	}
	return res;
}

/* offset is used because at the time a newline is entered in
 * doc_buffer_insert_text_lcb() the cursor is not yet forwarded to the new line, so
 * we need to add 1 to the line number in that specific case
 * also: the line number starts at line 1 and not at 0 !!
 */
static void doc_update_linenumber(Tdocument *doc, GtkTextIter *iter, gint offset) {
	gint line;
	gchar *string;
	GtkTextIter itinsert;

	if (iter == NULL) {
		GtkTextMark* insert;
		insert = gtk_text_buffer_get_insert(doc->buffer);
		gtk_text_buffer_get_iter_at_mark(doc->buffer, &itinsert, insert);
	} else {
		itinsert = *iter;
	}
	line = gtk_text_iter_get_line(&itinsert)+1;
	string = g_strdup_printf(_(" line %4d "), line + offset); 
	gtk_label_set(GTK_LABEL(main_v->statuslabel),string);	
	g_free(string);
	DEBUG_MSG("doc_update_linenumber, line=%d\n", line);
}

static void doc_buffer_insert_text_lcb(GtkTextBuffer *textbuffer,GtkTextIter * iter,gchar * string,gint len, Tdocument * doc) {

	DEBUG_MSG("doc_buffer_insert_text_lcb, started, string='%s'\n", string);

	/* undo_redo stuff */
	if (len == 1) {
		if ((string[0] == ' ' || string[0] == '\n' || string[0] == '\t') || !doc_undo_op_compare(doc,UndoInsert)) {
			DEBUG_MSG("doc_buffer_insert_text_lcb, need a new undogroup\n");
			doc_unre_new_group(doc);
		}
		if (string[0] == '\n') {
			doc_update_linenumber(doc, iter, 1);
		}
	}
	{
		gint pos = gtk_text_iter_get_offset(iter);
		doc_unre_add(doc, string, pos, pos+len, UndoInsert);
	}
	doc_set_modified(doc, 1);	
	DEBUG_MSG("doc_buffer_insert_text_lcb, done\n");
}

static void doc_buffer_insert_text_after_lcb(GtkTextBuffer *textbuffer,GtkTextIter * iter,gchar * string,gint len, Tdocument * doc) {
	gboolean do_highlighting = FALSE;
	/* highlighting stuff */
	DEBUG_MSG("doc_buffer_insert_text_after_lcb, started\n");
	if (doc->highlightstate && string && doc->hl) {
		if (strlen(doc->hl->update_chars)==0 ) {
			do_highlighting = TRUE;
		} else {
			gint i=0;
			while (string[i] != '\0') {
				if (strchr(doc->hl->update_chars, string[i])) {
					do_highlighting = TRUE;
					break;
				}
				i++;
			}
		}
		if (do_highlighting) {
			if (main_v->props.cont_highlight_full) {
				doc_highlight_full(doc);
			} else {
				doc_highlight_line(doc);
			}
		}
	}

	/* this function is only attached to the signal if the auto-indenting is active!!! */
	if (main_v->props.autoindent) {
		/* indenting, this should be the last one because it changes the 
		textbuffer --> it invalidates all GtkTextIters */
		if (len == 1 && string[0] == '\n') {
			gchar *string, *indenting;
			GtkTextIter itstart = *iter, itend = *iter;
			/* set to the beginning of the previous line */
			gtk_text_iter_backward_line (&itstart);
			gtk_text_iter_set_line_index(&itstart, 0);
		
			string = gtk_text_buffer_get_text(doc->buffer,&itstart,&itend,FALSE);
			if (string) {
				/* now count the indenting in this string */
				indenting = string;
				while (*indenting == '\t' || *indenting == ' ') {
					indenting++;
				}
				/* ending search, non-whitespace found, so terminate at this position */
				*indenting = '\0';
				if (strlen(string)) {
					DEBUG_MSG("doc_buffer_insert_text_lcb, inserting indenting\n");
					gtk_text_buffer_insert(doc->buffer,&itend,string,-1);
				}
				g_free(string);
			}
		}
	}
}

static void doc_buffer_delete_range_lcb(GtkTextBuffer *textbuffer,GtkTextIter * itstart,GtkTextIter * itend, Tdocument * doc) {
	gchar *string;
	gboolean do_highlighting=FALSE;
	string = gtk_text_buffer_get_text(doc->buffer, itstart, itend, FALSE);
	if (string) {
		/* highlighting stuff */
		if (doc->highlightstate && string && doc->hl) {
			if (strlen(doc->hl->update_chars)==0 ) {
				do_highlighting = TRUE;
			} else {
				gint i=0;
				while (string[i] != '\0') {
					if (strchr(doc->hl->update_chars, string[i])) {
						do_highlighting = TRUE;
						break;
					}
					i++;
				}
			}
			if (do_highlighting) {
				if (main_v->props.cont_highlight_full) {
					doc_highlight_full(doc);
				} else {
					doc_highlight_line(doc);
				}
			}
		}
		/* undo_redo stuff */
		{
			gint start, end, len;
			start = gtk_text_iter_get_offset(itstart);
			end = gtk_text_iter_get_offset(itend);	
			len = end - start;
			DEBUG_MSG("doc_buffer_delete_range_lcb, start=%d, end=%d, len=%d, string='%s'\n", start, end, len, string);
			if (len == 1) {
				if ((string[0] == ' ' || string[0] == '\n' || string[0] == '\t') || !doc_undo_op_compare(doc,UndoDelete)) {
					DEBUG_MSG("doc_buffer_delete_range_lcb, need a new undogroup\n");
					doc_unre_new_group(doc);
				}
				if (string[0] == '\n') {
					doc_update_linenumber(doc, NULL, 0);
				}
			} else {
				doc_unre_new_group(doc);
			}
			doc_unre_add(doc, string, start, end, UndoDelete);
		}
		g_free(string);
	}
	doc_set_modified(doc, 1);
}

static gboolean doc_view_button_release_lcb(GtkWidget *widget,GdkEventButton *bevent, Tdocument *doc) {
	DEBUG_MSG("doc_view_button_release_lcb, button %d\n", bevent->button);
	if (bevent->button == 3) {
		GtkWidget *menuitem;
		GtkWidget *submenu;
		GtkWidget *menu = gtk_menu_new ();
		gboolean tag_found;
		tag_found = doc_bevent_in_html_tag(doc, bevent);
		menuitem = gtk_menu_item_new_with_label(_("Edit tag"));
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(rpopup_edit_tag_cb), doc);
		if (!tag_found) {
			gtk_widget_set_sensitive(menuitem, FALSE);
		}
      gtk_widget_show (menuitem);
		gtk_menu_append(GTK_MENU(menu), menuitem);
		menuitem = gtk_menu_item_new();
      gtk_widget_show (menuitem);
		gtk_menu_append(GTK_MENU(menu), menuitem);

		menuitem = gtk_menu_item_new_with_mnemonic (_("Input _Methods"));
      gtk_widget_show (menuitem);
		submenu = gtk_menu_new ();
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), submenu);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
      gtk_im_multicontext_append_menuitems (GTK_IM_MULTICONTEXT (GTK_TEXT_VIEW(doc->view)->im_context),
					    GTK_MENU_SHELL (submenu));
		gtk_widget_show_all (menu);
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
			  NULL, doc->view, 0, gtk_get_current_event_time ());
	}
	return FALSE;
}

static gboolean doc_view_button_press_lcb(GtkWidget *widget,GdkEventButton *bevent, Tdocument *doc) {
	DEBUG_MSG("doc_view_button_press_lcb, button %d\n", bevent->button);
	if (bevent->button == 3) {
		return TRUE;
	}
	return FALSE;
}


static void doc_buffer_mark_set_lcb(GtkTextBuffer *buffer,GtkTextIter *iter,
                                            GtkTextMark *set_mark,
                                            Tdocument *doc) {
	GtkTextMark *ins_mark = gtk_text_buffer_get_insert(buffer);
	if (ins_mark == set_mark) {
		doc_update_linenumber(doc, iter, 0);
	}
}

void doc_bind_signals(Tdocument *doc) {
	doc->ins_txt_id = g_signal_connect(G_OBJECT(doc->buffer),
					 "insert-text",
					 G_CALLBACK(doc_buffer_insert_text_lcb), doc);
	doc->del_txt_id = g_signal_connect(G_OBJECT(doc->buffer),
					 "delete-range",
					 G_CALLBACK(doc_buffer_delete_range_lcb), doc);
	if (main_v->props.autoindent) {
		doc->ins_aft_txt_id = g_signal_connect_after(G_OBJECT(doc->buffer),
					 "insert-text",
					 G_CALLBACK(doc_buffer_insert_text_after_lcb), doc);
	} else {
		doc->ins_aft_txt_id = 0;
	}
}

void doc_unbind_signals(Tdocument *doc) {
/*	g_print("doc_unbind_signals, before unbind ins=%lu, del=%lu\n", doc->ins_txt_id, doc->del_txt_id);*/
	if (doc->ins_txt_id != 0) {
		g_signal_handler_disconnect(G_OBJECT(doc->buffer),doc->ins_txt_id);
		doc->ins_txt_id = 0;
	}
	if (doc->del_txt_id != 0) {
		g_signal_handler_disconnect(G_OBJECT(doc->buffer),doc->del_txt_id);
		doc->del_txt_id = 0;
	}
	if (doc->ins_aft_txt_id != 0) {
		g_signal_handler_disconnect(G_OBJECT(doc->buffer),doc->ins_aft_txt_id);
		doc->ins_txt_id = 0;
	}
}

/*
 * gint doc_textbox_to_file(Tdocument * doc, gchar * filename)
 * returns 1 on success
 * returns 2 on success but the backup failed
 * returns -1 if the backup failed and save was aborted
 * returns -2 if the file pointer could not be opened
 * returns -3 if the backup failed and save was aborted by the user
 */

gint doc_textbox_to_file(Tdocument * doc, gchar * filename) {
	FILE *fd;
	gint backup_retval;

	statusbar_message(_("Saving file"), 1000);
	if (main_v->props.auto_update_meta) {
		const gchar *realname = g_get_real_name();
		gchar *tmp;
		Tsearch_result res = doc_search_run_extern("<meta[ \t\n]name[ \t\n]*=[ \t\n]*\"generator\"[ \t\n]+content[ \t\n]*=[ \t\n]*\"[^\"]*bluefish[^\"]*\"[ \t\n]*>",1,0);
		if (res.end > 0) {
			snr2_run_extern_replace("<meta[ \t\n]name[ \t\n]*=[ \t\n]*\"generator\"[ \t\n]+content[ \t\n]*=[ \t\n]*\"[^\"]*\"[ \t\n]*>",0,1,0,"<meta name=\"generator\" content=\"Bluefish, see http://bluefish.openoffice.nl/\">", FALSE);
		}
		tmp = g_strconcat("<meta name=\"author\" content=\"",realname,"\">",NULL);
		snr2_run_extern_replace("<meta[ \t\n]name[ \t\n]*=[ \t\n]*\"author\"[ \t\n]+content[ \t\n]*=[ \t\n]*\"[^\"]*\"[ \t\n]*>",0,1,0,tmp,FALSE);
		g_free(tmp);
	}

	/* This writes the contents of a textbox to a file */
	backup_retval = doc_check_backup(doc);

	if (!backup_retval) {
		if (strcmp(main_v->props.backup_abort_style, "abort")==0) {
			DEBUG_MSG("doc_textbox_to_file, backup failure, abort!\n");
			return -1;
		} else if (strcmp(main_v->props.backup_abort_style, "ask")==0) {
			gchar *options[] = {N_("Abort save"), N_("Continue save"), NULL};
			gint retval;
			gchar *tmpstr =  g_strdup_printf(_("Backup for %s failed"), filename);
			retval = multi_button_dialog(_("Bluefish warning, file backup failure"), 1, tmpstr, options);
			g_free(tmpstr);
			if (retval == 0) {
				DEBUG_MSG("doc_textbox_to_file, backup failure, user aborted!\n");
				return -3;
			}
		}
	}
	change_dir(filename);
	fd = fopen(filename, "w");
	if (fd == NULL) {
		DEBUG_MSG("textbox_to_file, cannot open file %s\n", filename);
		return -2;
	} else {
		gchar *buffer;
		GtkTextIter itstart, itend;
		gtk_text_buffer_get_bounds(doc->buffer,&itstart,&itend);
		buffer = gtk_text_buffer_get_text(doc->buffer,&itstart,&itend,FALSE);
		
		if (doc->encoding) {
			gchar *newbuf;
			gsize wsize;
			newbuf = g_convert(buffer,-1,doc->encoding,"UTF-8",NULL,&wsize,NULL);
			if (newbuf) {
				g_free(buffer);
				buffer = newbuf;
			}
		}
		
		fputs(buffer, fd);
		g_free(buffer);
		fclose(fd);

		if (main_v->props.clear_undo_on_save) {
			doc_unre_clear_all(doc);
		}
		doc_set_modified(doc, 0);
		if (!backup_retval) {
			return 2;
		} else {
			return 1;
		}
	}
}

void doc_destroy(Tdocument * doc, gboolean delay_activation)
{
	if (doc->filename) {
		add_to_recent_list(doc->filename, 1);
	}
	gui_notebook_unbind_signals();
	/* to make this go really quick, we first only destroy the notebook page and run flush_queue(), 
	after the document is gone from the GUI we complete the destroy, to destroy only the notebook
	page we ref+ the scrolthingie, remove the page, and unref it again */
	g_object_ref(doc->view->parent);
	/* now we remove the document from the document list */
	if (g_list_length(main_v->documentlist) > 1) {
		main_v->documentlist = g_list_remove(main_v->documentlist, doc);
		DEBUG_MSG("removed %p from documentlist, list length=%d\n",doc, g_list_length(main_v->documentlist));
	} else {
		main_v->documentlist = g_list_remove(main_v->documentlist, doc);
		DEBUG_MSG("doc_destroy, last document removed from documentlist, documentlist=%p\n", main_v->documentlist);
		g_list_free(main_v->documentlist);
		DEBUG_MSG("doc_destroy, freed documentlist\n");
		main_v->documentlist = NULL;
		DEBUG_MSG("doc_destroy, documentlist = NULL\n");
	}
	DEBUG_MSG("doc_destroy, g_list_length(documentlist)=%d\n",g_list_length(main_v->documentlist));
	if (main_v->current_document == doc) {
		main_v->current_document = NULL;
	}
	/* then we remove the page from the notebook */
	gtk_notebook_remove_page(GTK_NOTEBOOK(main_v->notebook),
							 gtk_notebook_page_num(GTK_NOTEBOOK(main_v->notebook),doc->view->parent));
/*	flush_queue();*/
	if (!delay_activation) {
		notebook_changed(-1);
	}
	gui_notebook_bind_signals();

	/* now we really start to destroy the document */
	g_object_unref(doc->view->parent);

	if (doc->filename) {
		g_free(doc->filename);
	}
	DEBUG_MSG("doc_destroy, this is kind of tricky, I don't know if the Buffer is also detroyed when you destroy the view\n");
	g_object_unref(doc->buffer);
	DEBUG_MSG("doc_destroy, if this line shows up well I guess we needed to unref the buffer\n");
	
	doc_unre_destroy(doc);
	g_free(doc);

	DEBUG_MSG("doc_destroy, finished for %p\n", doc);
}

/* gint doc_save(Tdocument * doc, gint do_save_as, gint do_move)
 * returns 1 on success
 * returns 2 on success but the backup failed
 * returns 3 on user abort
 * returns -1 if the backup failed and save was aborted
 * returns -2 if the file pointer could not be opened 
 * returns -3 if the backup failed and save was aborted by the user
 * returns -4 if there is no filename, after asking one from the user
 * returns -5 if another process modified the file, and the user chose cancel
 */

gint doc_save(Tdocument * doc, gint do_save_as, gint do_move)
{
	gchar *oldfilename = NULL;
	gint retval;
#ifdef DEBUG
	g_assert(doc);
#endif

	DEBUG_MSG("doc_save, doc=%p, save_as=%d, do_move=%d\n", doc, do_save_as, do_move);
	if (doc->filename == NULL) {
		do_save_as = 1;
	}
	if (do_move) {
		do_save_as = 1;
	}

	if (do_save_as) {
		gchar *newfilename = NULL;
		gint index;
		statusbar_message(_("Save as..."), 1);
		oldfilename = doc->filename;
		doc->filename = NULL;
		newfilename = return_file_w_title(oldfilename, _("Save document as"));
		index = documentlist_return_index_from_filename(newfilename);
		DEBUG_MSG("doc_save, index=%d, filename=%p\n", index, newfilename);

		if (index != -1) {
			gchar *tmpstr;
			gint retval;
			gchar *options[] = {N_("Overwrite"), N_("Cancel"), NULL};
			tmpstr = g_strdup_printf(_("File %s is open, overwrite?"), newfilename);
			retval = multi_button_dialog(_("Bluefish: Warning, file is open"), 1, tmpstr, options);
			g_free(tmpstr);
			if (retval == 1) {
				g_free(newfilename);
				doc->filename = oldfilename;
				return 3;
			} else {
				Tdocument *tmpdoc;
				tmpdoc = (Tdocument *)g_list_nth_data(main_v->documentlist, index);
				DEBUG_MSG("doc_save, tmpdoc=%p\n", tmpdoc);
#ifdef DEBUG
				g_assert(tmpdoc);
				g_assert(tmpdoc->filename);
#endif
				g_free(tmpdoc->filename);
				tmpdoc->filename = NULL;
				doc_set_modified(tmpdoc, 1);
				{
					gchar *tmpstr2 = g_path_get_basename (newfilename);
					tmpstr = g_strconcat(_("Previously: "), tmpstr2, NULL);
					g_free(tmpstr2);
				}
				gtk_label_set(GTK_LABEL(tmpdoc->tab_label),tmpstr);
				g_free(tmpstr);
			}
		}
		doc->filename = newfilename;
		if (newfilename) {
			hl_reset_highlighting_type(doc, doc->filename);
			doc->modified = 1;
		}
	}

	if (doc->filename == NULL) {
		doc->filename = oldfilename;
		return -4;
	}
	{
	time_t newtime;
	if (doc_check_mtime(doc,&newtime) == 0) {
		gchar *tmpstr, oldtimestr[128], newtimestr[128];/* according to 'man ctime_r' this should be at least 26, so 128 should do ;-)*/
		gint retval;
		gchar *options[] = {N_("Overwrite"), N_("Cancel"), NULL};

		ctime_r(&newtime,newtimestr);
		ctime_r(&doc->mtime,oldtimestr);
		tmpstr = g_strdup_printf(_("File %s\nis modified by another process\nnew modification time is %s\nold modification time is %s\noverwrite?"), doc->filename,newtimestr,oldtimestr);
		retval = multi_button_dialog(_("Bluefish: Warning, file is modified"), 0, tmpstr, options);
		g_free(tmpstr);
		if (retval == 1) {
			if (oldfilename) {
				g_free(oldfilename);
			}
			return -5;
		}
	}
	}
	
	DEBUG_MSG("doc_save, returned file %s\n", doc->filename);
	if (do_save_as && oldfilename && main_v->props.link_management) {
/*		update_filenames_in_file(doc, oldfilename, doc->filename, 1);*/
	}
	{
		gchar *tmp = g_strdup_printf(_("Saving %s"), doc->filename);
		statusbar_message(tmp, 1);
		g_free(tmp);
	}
	retval = doc_textbox_to_file(doc, doc->filename);

	switch (retval) {
		gchar *errmessage;
		case -1:
			/* backup failed and aborted */
			errmessage = g_strconcat(_("File save aborted, could not backup file:\n"), doc->filename, NULL);
			error_dialog(_("Error"), errmessage);
			g_free(errmessage);
		break;
		case -2:
			/* could not open the file pointer */
			errmessage = g_strconcat(_("File save error, could not write file:\n"), doc->filename, NULL);
			error_dialog(_("Error"), errmessage);
			g_free(errmessage);
		break;
		default:
			doc_update_mtime(doc);
			DEBUG_MSG("doc_save, received return value %d from doc_textbox_to_file\n", retval);
		break;
	}
	if (oldfilename) {
/*		populate_dir_file_list();*/
		if (do_move && (retval > 0)) {
			if (main_v->props.link_management) {
/*				all_documents_update_links(doc, oldfilename,
							   doc->filename);*/
			}
			unlink(oldfilename);
		}

		g_free(oldfilename);
	}
	return retval;
}

/*
returning 0 --> cancel or abort
returning 1 --> ok, closed or saved & closed
*/
gint doc_close(Tdocument * doc, gint warn_only)
{
	gchar *text;
	gint retval;
#ifdef DEBUG
	if (!doc) {
		DEBUG_MSG("doc_close, returning because doc=NULL\n");
		return 0;
	}
#endif

	if (doc_is_empty_non_modified_and_nameless(doc) && g_list_length(main_v->documentlist) ==1) {
		/* no need to close this doc, it's an Untitled empty document */
		DEBUG_MSG("doc_close, 1 untitled empty non-modified document, returning\n");
		return 0;
	}

	if (doc->modified) {
		if (doc->filename) {
			text =
				g_strdup_printf(_("Are you sure you want to close\n%s ?"),
								doc->filename);
		} else {
			text =
				g_strdup(_
						 ("Are you sure you want to close\nthis untitled file ?"));
		}
	
		{
			gchar *buttons[] = {GTK_STOCK_SAVE, GTK_STOCK_CLOSE, GTK_STOCK_CANCEL, NULL};
			retval = multi_stockbutton_dialog(_("Bluefish warning: file is modified!"), 2, text, buttons);
		}
		g_free(text);

		switch (retval) {
		case 2:
			DEBUG_MSG("doc_close, retval=2 (cancel) , returning\n");
			return 2;
			break;
		case 0:
			doc_save(doc, 0, 0);
			if (doc->modified == 1) {
				/* something went wrong it's still not saved */
				return 0;
			}
			if (!warn_only) {
				doc_destroy(doc, FALSE);
			}
			break;
		case 1:
			if (!warn_only) {
				doc_destroy(doc, FALSE);
			}
			break;
		default:
			return 0;			/* something went wrong */
			break;
		}
	} else {
		if (!warn_only) {
			DEBUG_MSG("doc_close, starting doc_destroy for doc=%p\n", doc);
			doc_destroy(doc, FALSE);
		}
	}
	DEBUG_MSG("doc_close, finished\n");
/*	notebook_changed();*/
	return 1;
}

static void doc_close_but_clicked_lcb(GtkWidget *wid, gpointer data) {
	doc_close(data, 0);
}

/* contributed by Oskar Swida <swida@aragorn.pb.bialystok.pl>, with help from the gedit source */
static gboolean doc_textview_expose_event_lcb(GtkWidget * widget, GdkEventExpose * event, gpointer data) {
	GtkTextView *view = (GtkTextView*)widget;
	GdkRectangle rect;
	GdkWindow *win;
	GtkTextIter l_start,l_end,it;
	gint l_top1,l_top2;
	PangoLayout *l;
	gchar *pomstr;
	gint numlines,w,i;

	win = gtk_text_view_get_window(view,GTK_TEXT_WINDOW_LEFT);
	if (win!=event->window) return FALSE;

	gtk_text_view_get_visible_rect(view,&rect);
	gtk_text_view_get_line_at_y(view,&l_start,rect.y,&l_top1);
	gtk_text_view_get_line_at_y(view,&l_end,rect.y+rect.height,&l_top2);
	l = gtk_widget_create_pango_layout(widget,"");

	numlines = gtk_text_buffer_get_line_count(gtk_text_view_get_buffer(view));
	pomstr = g_strdup_printf("%d",MAX(99,numlines));
	pango_layout_set_text(l,pomstr,-1);
	g_free(pomstr);
	pango_layout_get_pixel_size(l,&w,NULL);
	gtk_text_view_set_border_window_size(view,GTK_TEXT_WINDOW_LEFT,w+4);   
	it = l_start;
	for(i=gtk_text_iter_get_line(&l_start);i<=gtk_text_iter_get_line(&l_end);i++) {
		gtk_text_iter_set_line(&it,i);
		gtk_text_view_get_line_yrange(view,&it,&w,NULL);
      
		gtk_text_view_buffer_to_window_coords(view,GTK_TEXT_WINDOW_LEFT,0,w,NULL,&w);
		pomstr = g_strdup_printf("%d",i+1);
		pango_layout_set_text(l,pomstr,-1);
      
		gtk_paint_layout(widget->style,win,GTK_WIDGET_STATE(widget),FALSE,NULL,widget,NULL,2,w,l);
      g_free(pomstr);
	}
	g_object_unref(G_OBJECT(l));
	return TRUE;
}

void document_set_line_numbers(Tdocument *doc, gboolean value) {
	if (value) {
		gtk_text_view_set_left_margin(GTK_TEXT_VIEW(doc->view),2);
		gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(doc->view),GTK_TEXT_WINDOW_LEFT,20);
		g_signal_connect(G_OBJECT(doc->view),"expose-event",G_CALLBACK(doc_textview_expose_event_lcb),NULL);
	} else {
		gtk_text_view_set_left_margin(GTK_TEXT_VIEW(doc->view),0);
		gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(doc->view),GTK_TEXT_WINDOW_LEFT,0);
	}
}

Tdocument *doc_new(gboolean delay_activate) {
	GtkWidget *scroll;
	Tdocument *newdoc = g_new0(Tdocument, 1);
	DEBUG_MSG("doc_new, main_v is at %p, newdoc at %p\n", main_v, newdoc);

	newdoc->hl = hl_get_highlightset_by_filename(NULL);
	newdoc->buffer = gtk_text_buffer_new(highlight_return_tagtable());
	newdoc->view = gtk_text_view_new_with_buffer(newdoc->buffer);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
									   GTK_POLICY_AUTOMATIC,
									   GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
											(scroll), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scroll), newdoc->view);

	newdoc->linenumberstate = main_v->props.view_line_numbers;
	document_set_line_numbers(newdoc, newdoc->linenumberstate);

	newdoc->tab_label = gtk_label_new(NULL);
	GTK_WIDGET_UNSET_FLAGS(newdoc->tab_label, GTK_CAN_FOCUS);
	if (strlen(main_v->props.tab_font_string)) {
		apply_font_style(newdoc->tab_label, main_v->props.tab_font_string);
	}
	newdoc->tab_menu = gtk_label_new(NULL);

	doc_unre_init(newdoc);
	doc_set_font(newdoc, NULL);
	newdoc->wrapstate = main_v->props.word_wrap;
	doc_set_wrap(newdoc);
	doc_set_tabsize(newdoc, main_v->props.editor_tab_width);

/* this will force function doc_set_modified to update the tab label*/
	newdoc->modified = 1;
	doc_set_modified(newdoc, 0);
	newdoc->filename = NULL;
	newdoc->need_highlighting = 0;
	newdoc->mtime = 0;
	newdoc->owner_uid = -1;
	newdoc->owner_gid = -1;
	newdoc->is_symlink = 0;
	newdoc->encoding = g_strdup(main_v->props.newfile_default_encoding);
	doc_bind_signals(newdoc);

	g_signal_connect(G_OBJECT(newdoc->view), "button-release-event"
		, G_CALLBACK(doc_view_button_release_lcb), newdoc);
	g_signal_connect(G_OBJECT(newdoc->view), "button-press-event"
		, G_CALLBACK(doc_view_button_press_lcb), newdoc);
	g_signal_connect(G_OBJECT(newdoc->buffer), "mark-set"
		, G_CALLBACK(doc_buffer_mark_set_lcb), newdoc);
	main_v->documentlist = g_list_append(main_v->documentlist, newdoc);

	gtk_widget_show(newdoc->view);
	gtk_widget_show(newdoc->tab_label);
	gtk_widget_show(scroll);

	DEBUG_MSG("doc_new, appending doc to notebook\n");
	{
		GtkWidget *hbox, *but;
		hbox = gtk_hbox_new(FALSE,0);
		but = gtk_button_new();
		{
		GtkWidget *image = new_pixmap(101);
		gtk_widget_show(image);
		gtk_container_add(GTK_CONTAINER(but), image);
		gtk_container_set_border_width(GTK_CONTAINER(but), 0);
		gtk_widget_set_usize(but, 12,12);
		}
		
		gtk_button_set_relief(GTK_BUTTON(but), GTK_RELIEF_NONE);
		g_signal_connect(G_OBJECT(but), "clicked", G_CALLBACK(doc_close_but_clicked_lcb), newdoc);
		gtk_box_pack_start(GTK_BOX(hbox), newdoc->tab_label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 0);
		gtk_widget_show(hbox);
		gtk_widget_show(but);
		gtk_notebook_append_page_menu(GTK_NOTEBOOK(main_v->notebook), scroll ,hbox, newdoc->tab_menu);
	}
	newdoc->highlightstate = main_v->props.defaulthighlight;
	DEBUG_MSG("doc_new, need_highlighting=%d, highlightstate=%d\n", newdoc->need_highlighting, newdoc->highlightstate);
	if (!delay_activate) {
		DEBUG_MSG("doc_new, set notebook page to %d\n", g_list_length(main_v->documentlist) - 1);
		gtk_notebook_set_page(GTK_NOTEBOOK(main_v->notebook),g_list_length(main_v->documentlist) - 1);

		if (main_v->current_document != newdoc) {
			notebook_changed(-1);
		}
		gtk_widget_grab_focus(newdoc->view);	
	}


	return newdoc;
}

void doc_new_with_new_file(gchar * new_filename) {
	Tdocument *doc;
	if (new_filename == NULL) {
		statusbar_message(_("No filename"), 2);
		return;
	}
	if (!main_v->props.allow_multi_instances) {
		gboolean res;
		res = switch_to_document_by_filename(new_filename);
		if (res){
			return;
		}
	} 
	DEBUG_MSG("doc_new_with_new_file, new_filename=%s\n", new_filename);
	add_filename_to_history(new_filename);
	doc = doc_new(FALSE);
	doc->filename = g_strdup(new_filename);
	doc->modified = 1; /* force doc_set_modified() (in doc_save()) to update the tab-label */
/*	doc_set_modified(doc, 0);*/
	doc_save(doc, 0, 0);
	doc_set_stat_info(doc); /* also sets mtime field */
}

gboolean doc_new_with_file(gchar * filename, gboolean delay_activate) {

	Tdocument *doc;
	
	if ((filename == NULL) || (!file_exists_and_readable(filename))) {
		DEBUG_MSG("doc_new_with_file, file %s !file_exists or readable\n", filename);
		return FALSE;
	}
	if (!main_v->props.allow_multi_instances) {
		gint index = documentlist_return_index_from_filename(filename);
		if (index != -1) {
			DEBUG_MSG("doc_new_with_file, %s is already open at index=%d\n",filename,index);
			if (!delay_activate) {
				switch_to_document_by_index(index);
			}
			return TRUE;
		}
	}
	DEBUG_MSG("doc_new_with_file, filename=%s exists\n", filename);
	add_filename_to_history(filename);

	if (g_list_length(main_v->documentlist)==1 && doc_is_empty_non_modified_and_nameless(main_v->current_document)) {
		doc = main_v->current_document;
	} else {
		doc = doc_new(delay_activate);
	}
	doc->filename = g_strdup(filename);
	hl_reset_highlighting_type(doc, doc->filename);	
	DEBUG_MSG("doc_new_with_file, hl is resetted to filename, about to load file\n");
	doc_file_to_textbox(doc, doc->filename, FALSE, delay_activate);
	menu_current_document_set_toggle_wo_activate(NULL, doc->encoding);
	doc->modified = 1; /* force doc_set_modified() to update the tab-label */
	doc_set_modified(doc, 0);
	doc_set_stat_info(doc); /* also sets mtime field */
	filebrowser_open_dir(filename);
	
	return TRUE;	
}

void docs_new_from_files(GList * file_list) {

	GList *tmplist, *errorlist=NULL;
	gboolean delay = (g_list_length(file_list) > 1);
	DEBUG_MSG("docs_new_from_files, lenght=%d\n", g_list_length(file_list));
	tmplist = g_list_first(file_list);
	while (tmplist) {
		DEBUG_MSG("docs_new_from_files, about to open %s\n", (gchar *) tmplist->data);
		if (!doc_new_with_file((gchar *) tmplist->data, delay)) {
			errorlist = g_list_append(errorlist, g_strdup((gchar *) tmplist->data));
		}
		tmplist = g_list_next(tmplist);
	}
	if (errorlist){
		gchar *message, *tmp;
		tmp = stringlist_to_string(errorlist, "\n");
		message = g_strconcat(_("Unable to open file(s)\n"), tmp, NULL);
		g_free(tmp);
		error_dialog("Bluefish error", message);
		g_free(message);
	}
	free_stringlist(errorlist);

	if (delay) {
		/* since we delayed the highlighting, we do that now */
		gtk_notebook_set_page(GTK_NOTEBOOK(main_v->notebook),g_list_length(main_v->documentlist) - 1);
		notebook_changed(-1);
	}
}

void doc_reload(Tdocument *doc) {
	if ((doc->filename == NULL) || (!file_exists_and_readable(doc->filename))) {
		statusbar_message(_("Unable to open file"), 2000);
		return;
	}
	{
		GtkTextIter itstart, itend;
		gtk_text_buffer_get_bounds(doc->buffer,&itstart,&itend);
		gtk_text_buffer_delete(doc->buffer,&itstart,&itend);
	}
	
	doc_file_to_textbox(doc, doc->filename, FALSE, FALSE);
	doc_unre_clear_all(doc);
	doc_set_modified(doc, 0);
	doc_set_stat_info(doc); /* also sets mtime field */
}

void doc_activate(Tdocument *doc) {
	time_t newtime;
#ifdef DEBUG
	if (!doc) {
		DEBUG_MSG("doc_activate, doc=NULL!!! ABORTING!!\n");
		exit(44);
	}
#endif
	if (doc == locals.last_activated_doc) {
		return;
	}
	locals.last_activated_doc = doc;
	if (doc_check_mtime(doc,&newtime) == 0) {
		gchar *tmpstr, oldtimestr[128], newtimestr[128];/* according to 'man ctime_r' this should be at least 26, so 128 should do ;-)*/
		gint retval;
		gchar *options[] = {N_("Reload"), N_("Ignore"), NULL};

		ctime_r(&newtime,newtimestr);
		ctime_r(&doc->mtime,oldtimestr);
		tmpstr = g_strdup_printf(_("File %s\nis modified by another process\nnew modification time is %s\nold modification time is %s"), doc->filename,newtimestr,oldtimestr);
		retval = multi_button_dialog(_("Bluefish: Warning, file is modified"), 0, tmpstr, options);
		g_free(tmpstr);
		if (retval == 1) {
			doc_set_stat_info(doc);
		} else {
			doc_reload(doc);
		}
	}
	DEBUG_MSG("doc_activate, calling gui_set_widgets\n");
	gui_set_widgets(doc_has_undo_list(doc), doc_has_redo_list(doc), doc->wrapstate, doc->highlightstate, doc->hl, doc->encoding, doc->linenumberstate);

	/* if highlighting is needed for this document do this now !! */
	if (doc->need_highlighting && doc->highlightstate) {
		doc_highlight_full(doc);
		DEBUG_MSG("doc_activate, after doc_highlight_full, need_highlighting=%d\n",doc->need_highlighting);
	}

	doc_scroll_to_cursor(doc);
	gtk_widget_grab_focus(GTK_WIDGET(doc->view));
	if (doc->filename) {
		gchar *dir1 = g_path_get_dirname(doc->filename);
		gchar *dir2 = ending_slash(dir1);
		chdir(dir2);
		filebrowser_open_dir(dir2);
		g_free(dir1);
		g_free(dir2);
	}
}

/**************************************************************************/
/* the start of the callback functions for the menu, acting on a document */
/**************************************************************************/
#ifdef EXTERNAL_GREP
#ifdef EXTERNAL_FIND
typedef struct {
	GList *filenames_to_return;
	GtkWidget *win;
	GtkWidget *basedir;
	GtkWidget *find_pattern;
	GtkWidget *recursive;
	GtkWidget *grep_pattern;
	GtkWidget *is_regex;
} Tfiles_advanced;

static void files_advanced_win_destroy(GtkWidget * widget, Tfiles_advanced *tfs) {
	DEBUG_MSG("files_advanced_win_destroy, started\n");
	gtk_main_quit();
	DEBUG_MSG("files_advanced_win_destroy, gtk_main_quit called\n");
	window_destroy(tfs->win);
}

static void files_advanced_win_ok_clicked(GtkWidget * widget, Tfiles_advanced *tfs) {
	/* create list here */
	gchar *command, *temp_file;
	gchar *c_basedir, *c_find_pattern, *c_recursive, *c_grep_pattern, *c_is_regex;
	temp_file = create_secure_dir_return_filename();
	if (!temp_file) {
		files_advanced_win_destroy(widget, tfs);
		DEBUG_MSG("files_advanced_win_ok_clicked, can't get a secure temp filename ?????\n");
		return;
	}
	DEBUG_MSG("files_advanced_win_ok_clicked, temp_file=%s\n", temp_file);
	c_basedir = gtk_editable_get_chars(GTK_EDITABLE(tfs->basedir), 0, -1);
	c_find_pattern = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(tfs->find_pattern)->entry), 0, -1);	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tfs->recursive))) {
		c_recursive= "-maxdepth 100 ";
	} else {
		c_recursive = "-maxdepth 1 ";
	}

	c_grep_pattern = gtk_editable_get_chars(GTK_EDITABLE(tfs->grep_pattern), 0, -1);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tfs->is_regex))) {
		c_is_regex = "-E -l ";
	} else {
		c_is_regex = "-l ";
	}
/*
command = `find c_basedir -name c_find_pattern c_recursive`
command = `grep -E 'c_grep_pattern' `find c_basedir -name c_find_pattern c_recursive``
*/
	if (strlen(c_grep_pattern) == 0) {
		command = g_strconcat(EXTERNAL_FIND, " ", c_basedir, " -name '", c_find_pattern, "' ", c_recursive, "> ", temp_file, NULL);
	} else {
		command = g_strconcat(EXTERNAL_GREP, " ",c_is_regex, " '", c_grep_pattern,"' `", EXTERNAL_FIND, " ", c_basedir, " -name '", c_find_pattern, "' ", c_recursive, "`", "> ", temp_file, NULL);
	}
	g_free(c_basedir);
	g_free(c_find_pattern);
	g_free(c_grep_pattern);
	DEBUG_MSG("files_advanced_win_ok_clicked, command=%s\n", command);
	statusbar_message(_("searching files..."), 1000);
	flush_queue();
	system(command);
	tfs->filenames_to_return = get_stringlist(temp_file, tfs->filenames_to_return);
	g_free(command);
	remove_secure_dir_and_filename(temp_file);
	g_free(temp_file);
	files_advanced_win_destroy(widget, tfs);
}
static void files_advanced_win_cancel_clicked(GtkWidget * widget, Tfiles_advanced *tfs) {
	files_advanced_win_destroy(widget, tfs);
}

static void files_advanced_win_select_basedir_lcb(GtkWidget * widget, Tfiles_advanced *tfs) {
	gchar *olddir = gtk_editable_get_chars(GTK_EDITABLE(tfs->basedir),0,-1);
	/* concat a "/" to the end of the current directory. This fixes a bug where your 
	   current working directory was being parsed as /directory/file when you opened 
	   the dialog to browse for a directory
	*/
	gchar *newdir = return_dir(g_strconcat(olddir, "/", NULL), _("Select basedir"));
	if (newdir) {
		gtk_entry_set_text(GTK_ENTRY(tfs->basedir),newdir);
		g_free(newdir);
	}
}

static void files_advanced_win(Tfiles_advanced *tfs) {
	GtkWidget *vbox, *hbox, *but, *table;
	GList *list;
	gchar *curdir=g_get_current_dir();
	
	tfs->win = window_full2(_("Advanced open file selector"), GTK_WIN_POS_MOUSE, 12, G_CALLBACK(files_advanced_win_destroy),tfs, TRUE, main_v->main_window);
	DEBUG_MSG("files_advanced_win, tfs->win=%p\n",tfs->win);
	tfs->filenames_to_return = NULL;
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(tfs->win), vbox);
	
	table = gtk_table_new(8, 5, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 12);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
	
	gtk_table_attach_defaults(GTK_TABLE(table), gtk_label_new(_("grep {contains} `find {basedir} -name '{file type}'`")), 0, 5, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), gtk_hseparator_new(), 0, 5, 1, 2);

	/* filename part */
	/* curdir should get a value */
	tfs->basedir = entry_with_text(curdir, 255);
	bf_mnemonic_label_tad_with_alignment(_("Base_dir:"), tfs->basedir, 0, 0.5, table, 1, 2, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(table), tfs->basedir, 2, 4, 2, 3);
	gtk_table_attach(GTK_TABLE(table), bf_generic_button_with_image(_("_Browse..."), 112, G_CALLBACK(files_advanced_win_select_basedir_lcb), tfs), 4, 5, 2, 3, GTK_SHRINK, GTK_SHRINK, 0, 0);

	g_free(curdir);
	
	list = g_list_append(NULL, "*.php");
	list = g_list_append(list, "*.php3");
	list = g_list_append(list, "*.html");
	list = g_list_append(list, "*.htm");
	list = g_list_append(list, "*.shtml");
	list = g_list_append(list, "*.pl");
	list = g_list_append(list, "*.cgi");
	list = g_list_append(list, "*.xml");
	list = g_list_append(list, "*.c");
	list = g_list_append(list, "*.py");
	tfs->find_pattern = combo_with_popdown("", list, 1);
	bf_mnemonic_label_tad_with_alignment(_("_File Type:"), tfs->find_pattern, 0, 0.5, table, 1, 2, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(table), tfs->find_pattern, 2, 4, 3, 4);

	g_list_free(list);

	tfs->recursive = checkbut_with_value(NULL, 1);
	bf_mnemonic_label_tad_with_alignment(_("_Recursive:"), tfs->recursive, 0, 0.5, table, 1, 2, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(table), tfs->recursive, 2, 3, 4, 5);	
	
	/* content */
	gtk_table_set_row_spacing(GTK_TABLE(table), 6, 18);
	{
		GtkWidget * label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label), "<b>Contains</b>");
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
		gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 3, 5, 6);
	}
	tfs->grep_pattern = entry_with_text(NULL, 255);
	bf_mnemonic_label_tad_with_alignment(_("Pa_ttern:"), tfs->grep_pattern, 0, 0.5, table, 1, 2, 6, 7);
	gtk_table_attach_defaults(GTK_TABLE(table), tfs->grep_pattern, 2, 4, 6, 7);
	
	tfs->is_regex = checkbut_with_value(NULL, 0);
	bf_mnemonic_label_tad_with_alignment(_("Is rege_x:"), tfs->is_regex, 0, 0.5, table, 1, 2, 7, 8);
	gtk_table_attach_defaults(GTK_TABLE(table), tfs->is_regex, 2, 3, 7, 8);
	
	/* buttons */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_hseparator_new(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 12);
	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 12);
	but = bf_stock_cancel_button(G_CALLBACK(files_advanced_win_cancel_clicked), tfs);
	gtk_box_pack_start(GTK_BOX(hbox),but , FALSE, FALSE, 0);
	but = bf_stock_ok_button(G_CALLBACK(files_advanced_win_ok_clicked), tfs);
	gtk_box_pack_start(GTK_BOX(hbox),but , FALSE, FALSE, 0);
	gtk_window_set_default(GTK_WINDOW(tfs->win), but);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show_all(GTK_WIDGET(tfs->win));
/*	gtk_grab_add(GTK_WIDGET(tfs->win));
	gtk_widget_realize(GTK_WIDGET(tfs->win));*/
	gtk_window_set_transient_for(GTK_WINDOW(tfs->win), GTK_WINDOW(main_v->main_window));
}

GList *return_files_advanced() {
	Tfiles_advanced tfs = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};

	files_advanced_win(&tfs);
	DEBUG_MSG("return_files_advanced, calling gtk_main()\n");
	gtk_main();
	return tfs.filenames_to_return;
}	
#endif /* EXTERNAL_FIND */
#endif /* EXTERNAL_GREP */


void file_save_cb(GtkWidget * widget, gpointer data) {
	doc_save(main_v->current_document, 0, 0);
}


void file_save_as_cb(GtkWidget * widget, gpointer data) {
	doc_save(main_v->current_document, 1, 0);
}

void file_move_to_cb(GtkWidget * widget, gpointer data) {
	doc_save(main_v->current_document, 1, 1);
}

void file_open_cb(GtkWidget * widget, gpointer data) {
	GList *tmplist;
	if (GPOINTER_TO_INT(data) == 1) {
#ifdef EXTERNAL_GREP
#ifdef EXTERNAL_FIND
		tmplist = return_files_advanced();
#endif
#endif
	} else {
		tmplist = return_files(NULL);
	}
	if (!tmplist) {
		return;
	}
	{
		gint len = g_list_length(tmplist);
		gchar *message = g_strdup_printf(_("Loading %d file(s)..."), len);
		statusbar_message(message,2000+len*50);
		g_free(message);
		flush_queue();
	}
	docs_new_from_files(tmplist);
	free_stringlist(tmplist);
}

void file_revert_to_saved_cb(GtkWidget * widget, gpointer data) {
	doc_reload(main_v->current_document);
}

void file_insert_cb(GtkWidget * widget, gpointer data) {
	gchar *tmpfilename;

	tmpfilename = return_file(NULL);
	if (tmpfilename == NULL) {
		statusbar_message(_("No file to insert"), 2000);
		return;
	} else {
		/* do we need to set the insert point in some way ?? */
		doc_file_to_textbox(main_v->current_document, tmpfilename, TRUE, FALSE);
		g_free(tmpfilename);
		doc_set_modified(main_v->current_document, 1);
	}
}

void file_new_cb(GtkWidget * widget, gpointer data) {
	Tdocument *doc;

	doc = doc_new(FALSE);
/*	project management needs a rewite so this is not included yet */
/* 	if ((main_v->current_project.template) && (file_exists_and_readable(main_v->current_project.template) == 1)) {
             doc_file_to_textbox(doc, main_v->current_project.template);
 	}*/
}

void file_close_cb(GtkWidget * widget, gpointer data) {
	doc_close(main_v->current_document, 0);
}

void file_close_all_cb(GtkWidget * widget, gpointer data)
{
	GList *tmplist;
	Tdocument *tmpdoc;
	gint retval = -1;

	DEBUG_MSG("file_close_all_cb, started\n");

	/* first a warning loop */
	if (test_docs_modified(NULL)) {
		gchar *options[] = {_("save all"), _("close all"), _("choose per file"), _("cancel"), NULL};
		retval =	multi_button_dialog(_("Bluefish: Warning, some file(s) are modified!"), 3, _("Some file(s) are modified\nplease choose your action"), options);
		if (retval == 3) {
			DEBUG_MSG("file_close_all_cb, cancel clicked, returning 0\n");
			return;
		}
	} else {
		retval = 1;
	}
	DEBUG_MSG("file_close_all_cb, after the warnings, retval=%d, now close all the windows\n", retval);

	tmplist = g_list_first(main_v->documentlist);
	while (tmplist) {
		tmpdoc = (Tdocument *) tmplist->data;
		if (test_only_empty_doc_left()) {
			return;
		}
		
		switch (retval) {
		case 0:
			doc_save(tmpdoc, 0, 0);
			if (!tmpdoc->modified) {
				doc_destroy(tmpdoc, TRUE);
			} else {
				return;
			}
			tmplist = g_list_first(main_v->documentlist);
		break;
		case 1:
			doc_destroy(tmpdoc, TRUE);
			tmplist = g_list_first(main_v->documentlist);
		break;
		case 2:
			if (doc_close(tmpdoc, 0)) {
				tmplist = g_list_first(main_v->documentlist);
			} else {
/*				notebook_changed();*/
				return;
			}
		break;
		default:
/*			notebook_changed();*/
			return;
		break;
		}
	}
	notebook_changed(-1);
	DEBUG_MSG("file_close_all_cb, finished\n");
}


/*
 * Function: file_save_all_cb
 * Arguments:
 * 	widget	- callback widget
 * 	data	- data for callback function
 * Return value:
 * 	void
 * Description:
 * 	Save all editor notebooks
 */
void file_save_all_cb(GtkWidget * widget, gpointer data)
{

	GList *tmplist;
	Tdocument *tmpdoc;

	tmplist = g_list_first(main_v->documentlist);
	while (tmplist) {
		tmpdoc = (Tdocument *) tmplist->data;
		if (tmpdoc->modified) {
			doc_save(tmpdoc, 0, 0);
		}
		tmplist = g_list_next(tmplist);
	}
}

void edit_cut_cb(GtkWidget * widget, gpointer data) {
	gtk_text_buffer_cut_clipboard(main_v->current_document->buffer,gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),TRUE);
}
void edit_copy_cb(GtkWidget * widget, gpointer data) {
	gtk_text_buffer_copy_clipboard  (main_v->current_document->buffer,gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
}
void edit_paste_cb(GtkWidget * widget, gpointer data) {
	gtk_text_buffer_paste_clipboard (main_v->current_document->buffer,gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),NULL,TRUE);
}
void edit_select_all_cb(GtkWidget * widget, gpointer data) {
	GtkTextIter itstart, itend;
	gtk_text_buffer_get_bounds(main_v->current_document->buffer,&itstart,&itend);
	gtk_text_buffer_move_mark_by_name(main_v->current_document->buffer,"insert",&itstart);
	gtk_text_buffer_move_mark_by_name(main_v->current_document->buffer,"selection_bound",&itend);
}

void doc_toggle_highlighting_cb(gpointer callback_data,guint action,GtkWidget *widget) {

	main_v->current_document->highlightstate = 1 - main_v->current_document->highlightstate;
	DEBUG_MSG("doc_toggle_highlighting_cb, started, highlightstate now is %d\n", main_v->current_document->highlightstate);
	if (main_v->current_document->highlightstate == 0) {
		doc_remove_highlighting(main_v->current_document);
	} else {
		doc_highlight_full(main_v->current_document);
	}
}

void doc_toggle_wrap_cb(gpointer callback_data,guint action,GtkWidget *widget) {
	main_v->current_document->wrapstate = 1 - main_v->current_document->wrapstate;
	doc_set_wrap(main_v->current_document);
}

void doc_toggle_linenumbers_cb(gpointer callback_data,guint action,GtkWidget *widget) {
	main_v->current_document->linenumberstate = 1 - main_v->current_document->linenumberstate;
	document_set_line_numbers(main_v->current_document, main_v->current_document->linenumberstate);
}

/* callback_action: 1 only ascii, 2 only iso, 3 both
 */
void doc_convert_asciichars_in_selection(gpointer callback_data,guint callback_action,GtkWidget *widget) {
	doc_convert_chars_to_entities_in_selection(main_v->current_document, (callback_action != 2), (callback_action != 1));
}


void word_count_cb (gpointer callback_data,guint callback_action,GtkWidget *widget) {
	int wc, alnum = 0, tag = 0;
	const gchar delimiters[]=" .,;:?\n\t";
	const gchar alnums[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	gchar *wc_message, *allchars, *tmp;

	wc = 0;
   allchars = doc_get_chars(main_v->current_document, 0, -1);
	tmp = allchars;
	while (*tmp != '\0') {
		if ((strchr (delimiters, *tmp)) && (alnum == 1) && (tag == 0)) wc++;
		if (*tmp == '<') { tag = 1; } else if (*tmp == '>') { tag = 0; };
		if (strchr (alnums, *allchars)) { alnum = 1;} else { alnum = 0; }
		tmp++;
	}
	wc_message = g_strdup_printf("Word Count %d", wc);
	statusbar_message (wc_message, 5000);
	g_free (wc_message);
}

void doc_indent_selection(Tdocument *doc, gboolean unindent) {
	GtkTextIter itstart,itend;
	if (gtk_text_buffer_get_selection_bounds(doc->buffer,&itstart,&itend)) {
		GtkTextMark *end;
/*		gboolean firstrun=TRUE;*/

		doc_unbind_signals(doc);
		doc_unre_new_group(doc);
		/* we have a selection, now we loop trough the characters, and for every newline
		we add or remove a tab, we set the end with a mark */
		end = gtk_text_buffer_create_mark(doc->buffer,NULL,&itend,TRUE);

		while(gtk_text_iter_compare(&itstart,&itend) < 0) {
			GtkTextMark *cur;
/*			if (firstrun && !gtk_text_iter_starts_line(&itstart)) {
				gtk_text_iter_forward_line(&itstart);
			}
			firstrun = FALSE;*/
			cur = gtk_text_buffer_create_mark(doc->buffer,NULL,&itstart,TRUE);
			if (unindent) {
				/* when unindenting we try to set itend to the end of the indenting step
				which might be a tab or 'tabsize' spaces, then we delete that part */
				gboolean cont=TRUE;
				gchar *buf = NULL;
				gunichar cchar = gtk_text_iter_get_char(&itstart);
				if (cchar == 9) { /* 9 is ascii for tab */
					itend = itstart;
					cont = gtk_text_iter_forward_char(&itend);
					buf = g_strdup("\t");
				} else if (cchar == 32) { /* 32 is ascii for space */
					gchar *tmpstr;
					gint i=0;
					itend = itstart;
					gtk_text_iter_forward_chars(&itend,main_v->props.editor_tab_width);
					tmpstr = gtk_text_buffer_get_text(doc->buffer,&itstart,&itend,FALSE);
					DEBUG_MSG("tab_width=%d, strlen(tmpstr)=%d, tmpstr='%s'\n",main_v->props.editor_tab_width,strlen(tmpstr),tmpstr);
					while (cont && tmpstr[i] != '\0') {
						cont = (tmpstr[i] == ' ');
						DEBUG_MSG("doc_indent_selection, tmpstr[%d]='%c'\n",i,tmpstr[i]);
						i++;
					}
					if (cont) {
						buf = tmpstr;
					} else {
						g_free(tmpstr);
					}
				} else {
					cont = FALSE;
				}
				if (cont) {
					gint offsetstart, offsetend;
					offsetstart = gtk_text_iter_get_offset(&itstart);
					offsetend = gtk_text_iter_get_offset(&itend);
					gtk_text_buffer_delete(doc->buffer,&itstart,&itend);
					doc_unre_add(doc, buf, offsetstart, offsetend, UndoDelete);
				}
#ifdef DEBUG
				else {
					DEBUG_MSG("doc_indent_selection, NOT continue!!\n");
				}
#endif
			} else { /* indent */
				gint offsetstart = gtk_text_iter_get_offset(&itstart);
				gtk_text_buffer_insert(doc->buffer,&itstart,"\t",1);
				doc_unre_add(doc, "\t", offsetstart, offsetstart+1, UndoInsert);
			}
			gtk_text_buffer_get_iter_at_mark(doc->buffer,&itstart,cur);
			gtk_text_buffer_get_iter_at_mark(doc->buffer,&itend,end);
			gtk_text_buffer_delete_mark(doc->buffer,cur);
			gtk_text_iter_forward_line(&itstart);
			DEBUG_MSG("doc_indent_selection, itstart at %d, itend at %d\n",gtk_text_iter_get_offset(&itstart),gtk_text_iter_get_offset(&itend));
		}
		gtk_text_buffer_delete_mark(doc->buffer,end);
		doc_bind_signals(doc);
		doc_set_modified(doc, 1);
	} else {
		DEBUG_MSG("doc_indent_selection, put a message on the statusbar that we want a selection..\n");
		statusbar_message(_("No selection to indent"), 2000);
	}
}

void menu_indent_cb(gpointer callback_data,guint callback_action, GtkWidget  *widget) {
	if (main_v->current_document) {
		doc_indent_selection(main_v->current_document, (callback_action == 1));
	}
}
