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


#include "bluefish.h"

Tdocument *doc_new() {
	Tdocument *newdoc = g_new0(Tdocument, 1);

	newdoc->hl = hl_get_highlightset_by_filename(NULL);
	newdoc->buffer = gtk_text_buffer_new(newdoc->hl->tagtable);
	newdoc->view = gtk_text_view_new_with_buffer(newdoc->buffer);

	main_v->documentlist = g_list_append(main_v->documentlist, newdoc);

	gtk_notebook_append_page(GTK_NOTEBOOK(main_v->notebook), newdoc->view,newdoc->tab_label);
	gtk_widget_show(tmptable);
	
	return newdoc;
}

/* gint find_filename_in_documentlist(gchar *filename)
 * returns -1 if the file is not open, else returns the index where
 * the file is in the documentlist
 */
gint find_filename_in_documentlist(gchar *filename) {
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
gboolean test_only_empty_doc_left(Tdocument *doc) {
	if (g_list_length(main_v->documentlist) > 1) {
		return FALSE;
	} else {
		Tdocument *tmpdoc;
		GList *tmplist = g_list_first(main_v->documentlist);
#ifdef DEBUG
		g_assert(tmplist);
		g_assert(tmplist->data);
#endif
		tmpdoc = tmplist->data;
		if (tmpdoc->modified || tmpdoc->filename) {
			return FALSE;
		}
		if (gtk_text_buffer_get_char_count(tmpdoc->buffer) > 0) {
			return FALSE;
		}
	}
	return TRUE;
}

static void doc_scroll_to_line(Tdocument *doc, gint linenum, gboolean select_line) {
	GtkTextIter itstart;

	gtk_text_buffer_get_iter_at_line(doc->buffer,&itstart,linenum);
	gtk_text_view_scroll_to_iter(doc->view,&itstart,0.25,FALSE,0.5,0.5);
	if (select_line) {
		GtkTextIter itend = itstart;
		gtk_text_iter_forward_to_line_end(&itend);
		gtk_text_buffer_place_cursor (doc->buffer, &itstart);
		gtk_text_buffer_move_mark_by_name(doc->buffer,"selection_bound",&itend);
	}
}

#define STARTING_BUFFER_SIZE 1024
gboolean doc_file_to_textbox(Tdocument * doc, gchar * filename)
{
	FILE *fd;
	gchar *errmessage, line[STARTING_BUFFER_SIZE], *message;

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
		return FALSE;
	} 

	while (fgets(line, STARTING_BUFFER_SIZE, fd) != NULL) {
		gtk_text_buffer_insert_at_cursor(doc->buffer,line,-1);
	}
	fclose(fd);
	if (some_property_if_we_have_default_highlighting) {
		doc_highlight_full(doc);
	}
	return TRUE;
}
