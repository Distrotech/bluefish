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
	if (main_v->current_document->highlightstate == 0) {
		setup_toggle_item(gtk_item_factory_from_widget(main_v->menubar),
			  N_("/Document/Highlight syntax"), TRUE);
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
1 if there was no previous mtime information available */
static gint doc_check_mtime(Tdocument *doc) {
	if (doc->filename && 0 != doc->mtime) {
		struct stat statbuf;
		if (stat(doc->filename, &statbuf) == 0) {
			if (doc->mtime < statbuf.st_mtime) {
				DEBUG_MSG("doc_check_mtime, doc->mtime=%d, statbuf.st_mtime=%d\n", (int)doc->mtime, (int)statbuf.st_mtime);
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

void doc_insert_two_strings(Tdocument *doc, const gchar *before_str, const gchar *after_str) {
	GtkTextIter itinsert, itselect;
	GtkTextMark *insert, *select;
	insert = gtk_text_buffer_get_insert(doc->buffer);
	select = gtk_text_buffer_get_selection_bound(doc->buffer);
	gtk_text_buffer_get_iter_at_mark(doc->buffer,&itinsert,insert);
	gtk_text_buffer_get_iter_at_mark(doc->buffer,&itselect,select);
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
	} else {
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
		gchar chunk[STARTING_BUFFER_SIZE+1];
		gint buffer_size = STARTING_BUFFER_SIZE + 1;
		gchar *buffer = g_malloc(buffer_size * sizeof(gchar));
		buffer[0] = '\0';
		while (fgets(chunk, STARTING_BUFFER_SIZE, fd) != NULL) {
			strcat(buffer, chunk);
			buffer_size += STARTING_BUFFER_SIZE;
			buffer = g_realloc(buffer, buffer_size);
		}
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
				g_print("doc_file_to_textbox, try encoding %s\n", encoding);
				newbuf = g_convert(buffer,-1,"UTF-8",encoding,NULL, &wsize, &error);
				if (!newbuf || error) {
					DEBUG_MSG("doc_file_to_textbox, cound not convert to UTF-8: \n");
				} else {
					if (doc->encoding) {
						g_free(doc->encoding);
					}
					doc->encoding = g_strdup(encoding);
					g_free(buffer);
					buffer = newbuf;
				}
				g_free(encoding);
			}
			if (!newbuf) {
				if(!g_utf8_validate(buffer, -1, NULL)) {
					DEBUG_MSG("doc_file_to_textbox, file is not in UTF-8\n");
					newbuf = g_locale_to_utf8(buffer,-1,NULL,&wsize,NULL);
					if (!newbuf) {
						newbuf = g_convert(buffer,-1,"UTF-8",main_v->props.newfile_default_encoding,NULL, &wsize, NULL);
						if (!newbuf) {
							error_dialog(_("Error"), _("Cannot display file, unknown characters found."));
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
					}
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
	fclose(fd);
	doc->need_highlighting=TRUE;
	if (!delay_highlighting) {
#ifdef DEBUG
		g_print("doc_file_to_textbox, doc->hlset=%p\n", doc->hl);
		if (doc->hl) {
			g_print("doc_file_to_textbox, doc->hlset->highlightlist=%p\n", doc->hl->highlightlist);
		}
#endif
		if (main_v->props.cont_highlight_update) {
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
	/* highlighting stuff */
	DEBUG_MSG("doc_buffer_insert_text_after_lcb, started\n");
	if (string && doc->hl->update_chars) {
		gint i=0;
		while (string[i] != '\0') {
			if (strchr(doc->hl->update_chars, string[i])) {
				doc_highlight_line(doc);
				break;
			}
			i++;
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
	gint i=0;
	string = gtk_text_buffer_get_text(doc->buffer, itstart, itend, FALSE);
	if (string) {
		/* highlighting stuff */
		if (doc->hl->update_chars) {
			while (string[i] != '\0') {
				if (strchr(doc->hl->update_chars, string[i])) {
					doc_highlight_line(doc);
					break;
				}
				i++;
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

	if (delay_activation) {
		gui_notebook_unbind_signals();
	}

	gtk_notebook_remove_page(GTK_NOTEBOOK(main_v->notebook),
							 gtk_notebook_page_num(GTK_NOTEBOOK(main_v->notebook),doc->view->parent));

	if (g_list_length(main_v->documentlist) > 1) {
		main_v->documentlist = g_list_remove(main_v->documentlist, doc);
	} else {
		main_v->documentlist = g_list_remove(main_v->documentlist, doc);
		DEBUG_MSG("doc_destroy, last document removed from documentlist\n");
		g_list_free(main_v->documentlist);
		DEBUG_MSG("doc_destroy, freed documentlist\n");
		main_v->documentlist = NULL;
		DEBUG_MSG("doc_destroy, documentlist = NULL\n");
	}
	DEBUG_MSG("doc_destroy, g_list_length(documentlist)=%d\n",g_list_length(main_v->documentlist));
	if (doc->filename) {
		g_free(doc->filename);
	}
	DEBUG_MSG("doc_destroy, this is kind of tricky, I don't know if the Buffer is also detroyed when you destroy the view\n");
	g_object_unref(doc->buffer);
	DEBUG_MSG("doc_destroy, if this line shows up well I guess we needed to unref the buffer\n");
	
	doc_unre_destroy(doc);
	g_free(doc);
	if (!delay_activation) {
		notebook_changed(-1);
	} else {
		gui_notebook_bind_signals();
	}
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
		doc->modified = 1;
	}

	if (doc->filename == NULL) {
		doc->filename = oldfilename;
		return -4;
	}
	
	if (doc_check_mtime(doc) == 0) {
		gchar *tmpstr;
		gint retval;
		gchar *options[] = {N_("Overwrite"), N_("Cancel"), NULL};

		tmpstr = g_strdup_printf(_("File %s\nis modified by another process, overwrite?"), doc->filename);
		retval = multi_button_dialog(_("Bluefish: Warning, file is modified"), 0, tmpstr, options);
		g_free(tmpstr);
		if (retval == 1) {
			if (oldfilename) {
				g_free(oldfilename);
			}
			return -5;
		}
	}
	
	DEBUG_MSG("doc_save, returned file %s\n", doc->filename);
	if (do_save_as && oldfilename && main_v->props.link_management) {
/*		update_filenames_in_file(doc, oldfilename, doc->filename, 1);*/
	}
	statusbar_message(_("Save in progress"), 1);
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
		hl_reset_highlighting_type(doc, doc->filename);
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
		DEBUG_MSG("doc_close, closing doc=%p\n", doc);
		if (!warn_only) {
			doc_destroy(doc, FALSE);
		}
	}

/*	notebook_changed();*/
	return 1;
}

static void doc_close_but_clicked_lcb(GtkWidget *wid, gpointer data) {
	doc_close(data, 0);
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
		return FALSE;
	}
	if (!main_v->props.allow_multi_instances) {
		gint index = documentlist_return_index_from_filename(filename);
		if (index != -1) {
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
	doc_set_modified(doc, 0);
	doc_set_stat_info(doc); /* also sets mtime field */
}

void doc_activate(Tdocument *doc) {
	if (doc_check_mtime(doc) == 0) {
		gchar *tmpstr;
		gint retval;
		gchar *options[] = {N_("Reload"), N_("Ignore"), NULL};

		tmpstr = g_strdup_printf(_("File %s\nis modified by another process"), doc->filename);
		retval = multi_button_dialog(_("Bluefish: Warning, file is modified"), 0, tmpstr, options);
		g_free(tmpstr);
		if (retval == 1) {
			doc_set_stat_info(doc);
		} else {
			doc_reload(doc);
		}
	}
	DEBUG_MSG("doc_activate, calling gui_set_widgets\n");
	gui_set_widgets(doc_has_undo_list(doc), doc_has_redo_list(doc), doc->wrapstate, doc->highlightstate, doc->hl, doc->encoding);

	/* if highlighting is needed for this document do this now !! */
	if (doc->need_highlighting && doc->highlightstate) {
		doc_highlight_full(doc);
		DEBUG_MSG("doc_activate, after doc_highlight_full, need_highlighting=%d\n",doc->need_highlighting);
	}

	doc_scroll_to_cursor(doc);
	gtk_widget_grab_focus(GTK_WIDGET(doc->view));
}


/**************************************************************************/
/* the start of the callback functions for the menu, acting on a document */
/**************************************************************************/

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
/*		tmplist = return_files_advanced();*/
		tmplist = NULL;
	} else {
		tmplist = return_files(NULL);
	}
	if (!tmplist) {
		return;
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
		error_dialog(_("Bluefish error"), _("No filename known"));
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
		gchar *options[] = {N_("save all"), N_("close all"), N_("choose per file"), N_("cancel"), NULL};
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
	DEBUG_MSG("doc_toggle_highlighting_cb, started\n");
	main_v->current_document->highlightstate = 1 - main_v->current_document->highlightstate;
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
