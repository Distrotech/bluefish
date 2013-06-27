/* Bluefish HTML Editor
 * document.c - the document
 *
 * Copyright (C) 1998-2012 Olivier Sessink
 * Copyright (C) 1998 Chris Mazuc
 * some additions Copyright (C) 2004 Eugene Morenko(More)
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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>		/* for the keyboard event codes */

#include <string.h>				/* strchr() */
#include <stdlib.h>				/* system() */

/*#define DEBUG*/

#include "bluefish.h"

#ifdef DEBUGPROFILING
#ifdef WIN32
#undef DEBUGPROFILING
#else
#include <sys/times.h>
#endif
#endif

#ifdef MAC_INTEGRATION
/*#include <ige-mac-integration.h>*/
#include <gtkosxapplication.h>
#endif

#include "bf_lib.h"
#include "bftextview2.h"
#include "bftextview2_langmgr.h"
#include "bftextview2_identifier.h"
#include "bfwin.h"
#include "bfwin_uimanager.h"
#include "bookmark.h"
#include "dialog_utils.h"
#include "document.h"
#include "file.h"
#include "filebrowser2.h"
#include "file_dialogs.h"
#include "gtk_easy.h"			/* *_dialog() */
#include "pixmap.h"
#include "stringlist.h"			/* free_stringlist() */
#include "undo_redo.h"			/* doc_unre_init() */
#include "file_autosave.h"
#ifdef HAVE_LIBENCHANT
#include "bftextview2_spell.h"
#endif

typedef struct {
	GtkWidget *textview;
	GtkWidget *window;
} Tfloatingview;
#define FLOATINGVIEW(var) ((Tfloatingview *)(var))

Tselectionsave *
doc_save_selection(Tdocument * doc)
{
	Tselectionsave *selsave;
	GtkTextIter start, end;
	selsave = g_slice_new(Tselectionsave);
	gtk_text_buffer_get_selection_bounds(doc->buffer, &start, &end);
	/*g_print("saving selection %d:%d\n",gtk_text_iter_get_offset(&start),gtk_text_iter_get_offset(&end)); */
	selsave->start = gtk_text_buffer_create_mark(doc->buffer, NULL, &start, FALSE);
	selsave->end = gtk_text_buffer_create_mark(doc->buffer, NULL, &end, TRUE);
	selsave->doc = doc;
	return selsave;
}

void
doc_restore_selection(Tselectionsave * selsave, gboolean only_if_no_selection)
{
	if (!only_if_no_selection || !gtk_text_buffer_get_has_selection(selsave->doc->buffer)) {
		GtkTextIter start, end;
		gtk_text_buffer_get_iter_at_mark(selsave->doc->buffer, &start, selsave->start);
		gtk_text_buffer_get_iter_at_mark(selsave->doc->buffer, &end, selsave->end);
		/*g_print("restoring selection %d:%d\n",gtk_text_iter_get_offset(&start),gtk_text_iter_get_offset(&end)); */
		gtk_text_buffer_select_range(selsave->doc->buffer, &start, &end);
	}
	gtk_text_buffer_delete_mark(selsave->doc->buffer, selsave->start);
	gtk_text_buffer_delete_mark(selsave->doc->buffer, selsave->end);
	g_slice_free(Tselectionsave, selsave);
}

static void
session_set_opendir(Tbfwin * bfwin, gchar * curi)
{
	if (curi) {
		gchar *pos = strrchr(curi, '/');
		if (pos != NULL) {
			if (bfwin->session->opendir)
				g_free(bfwin->session->opendir);
			bfwin->session->opendir = g_strndup(curi, pos - curi);
			DEBUG_MSG("session_set_opendir, opendir=%s\n", bfwin->session->opendir);
		}
	}
}

/**
 * return_allwindows_documentlist:
 *
 * returns a documentlist with all documents in all windows, the list should be freed, the Tdocuments obviously not
 *
 * Return value: #GList* with all documents
 */
GList *
return_allwindows_documentlist()
{
	GList *newdoclist = NULL, *bflist, *tmplist = NULL;
	bflist = g_list_first(main_v->bfwinlist);
	DEBUG_MSG("return_allwindows_documentlist, bfwinlist length=%d\n", g_list_length(main_v->bfwinlist));
	while (bflist) {
		DEBUG_MSG("return_allwindows_documentlist, current bfwin doclist length=%d\n",
				  g_list_length(BFWIN(bflist->data)->documentlist));
		tmplist = g_list_first(BFWIN(bflist->data)->documentlist);
		while (tmplist) {
			newdoclist = g_list_prepend(newdoclist, tmplist->data);
			tmplist = g_list_next(tmplist);
		}
		bflist = g_list_next(bflist);
	}
	DEBUG_MSG("return_allwindows_documentlist, returning list length %d\n", g_list_length(newdoclist));
	return newdoclist;
}

void
alldocs_foreach(foreachdocfunc func, gpointer data)
{
	GList *tmplist, *doclist = return_allwindows_documentlist();
	tmplist = g_list_first(doclist);
	while (tmplist) {
		func(tmplist->data, data);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(doclist);
}

gint
have_modified_documents(GList * doclist)
{
	GList *tmplist = g_list_first(doclist);
	gint count = 0;
	while (tmplist) {
		if (DOCUMENT(tmplist->data)->modified) {
			count++;
			if (count > 2)
				return 2;
		}
		tmplist = g_list_next(tmplist);
	}
	return count;
}

/**
 * return_urilist_from_doclist:
 * @doclist: #GList*
 *
 * Returns a stringlist with filenames given a
 * list with documents (#Tdocument*)
 *
 * Return value: #GList* stringlist with filenames
 */
GList *
return_urilist_from_doclist(GList * doclist)
{
	GList *newlist = NULL, *tmplist;
	DEBUG_MSG("return_filenamestringlist_from_doclist, started for doclist %p, len=%d\n", doclist,
			  g_list_length(doclist));
	tmplist = g_list_first(doclist);
	while (tmplist) {
		if (DOCUMENT(tmplist->data)->uri) {
			g_object_ref(DOCUMENT(tmplist->data)->uri);
			newlist = g_list_prepend(newlist, DOCUMENT(tmplist->data)->uri);
		}
		tmplist = g_list_next(tmplist);
	}
	return newlist;
}

/*
 * return_num_untitled_documents:
 * @doclist: #GList* with documents
 *
 * returns the number of untitled documents
 * opened in Bluefish
 *
 * Return value: #gint with number
 *
gint return_num_untitled_documents(GList *doclist) {
	gint retval = 0;
	GList *tmplist = g_list_first(doclist);
	while (tmplist) {
		if (DOCUMENT(tmplist->data)->filename == NULL) retval++;
		tmplist = g_list_next(tmplist);
	}
	return retval;
}*/

/**
 * add_filename_to_recentlist:
 * @bfwin: #Tbfwin*
 * @uri: a #GFile*
 *
 * adds a filename to the recently opened files list
 * will not add it to the menu, only to the list and the file
 **/
void
add_filename_to_recentlist(Tbfwin * bfwin, GFile * uri)
{
	gchar *curi = g_file_get_uri(uri);
	bfwin->session->recent_files =
				add_to_history_stringlist(bfwin->session->recent_files, curi, FALSE, TRUE);
	bfwin_recent_menu_remove(bfwin, FALSE, curi);

	if (main_v->props.register_recent_mode == 0)
		return;
	if (main_v->props.register_recent_mode == 1) {
		gtk_recent_manager_add_item(main_v->recentm, curi);
	}
	g_free(curi);
}

/**
 * documentlist_return_index_from_uri:
 * @doclist: #GList* with the documents to search in
 * @filename: a #gchar
 *
 * if the file is open, it returns the index in the documentlist
 * which is also the index in the notebook
 * if the file is not open it returns -1
 *
 * Return value: the index number on success, -1 if the file is not open
 **/
gint
documentlist_return_index_from_uri(GList * doclist, GFile * uri)
{
	GList *tmplist;
	gint count = 0;

	if (!uri) {
		return -1;
	}

	tmplist = g_list_first(doclist);
	while (tmplist) {
		Tdocument *doc = tmplist->data;
		if (doc->uri && (doc->uri == uri || g_file_equal(doc->uri, uri))) {
			return count;
		}
		count++;
		tmplist = g_list_next(tmplist);
	}
	return -1;
}

void
doc_set_uri(Tdocument *doc, GFile *uri, gboolean on_destroy)
{
	if (uri == doc->uri)
		return;

	if (doc->uri) {
		g_hash_table_remove(main_v->alldochash, doc->uri);
		fb2_set_uri_state(doc->uri, FALSE);
		g_object_unref(doc->uri);
	}
	doc->uri = uri;
	if (doc->uri) {
		g_object_ref(doc->uri);
		g_hash_table_insert(main_v->alldochash, doc->uri, doc);
		fb2_set_uri_state(doc->uri, TRUE);
	}
	if (!on_destroy) {
		DEBUG_MSG("doc_set_uri, call bmark_doc_renamed for doc %p (new uri %p)\n",doc,uri);
		bmark_doc_renamed(BFWIN(doc->bfwin), doc);
	}
}


/**
 * documentlist_return_document_from_uri:
 * @doclist: #GList* with the documents to search in
 * @filename: a #gchar
 *
 * if the file is open, it returns the Tdocument* in the documentlist
 * if the file is not open it returns NULL
 *
 * Return value: #Tdocument* or NULL if not open
 **/
Tdocument *
documentlist_return_document_from_uri(GList * doclist, GFile * uri)
{
#ifdef DEVELOPMENT
	GList *tmplist;
	if (!uri) {
		DEBUG_MSG("documentlist_return_document_from_filename, no filename! returning\n");
		return NULL;
	}
	tmplist = g_list_first(doclist);
	while (tmplist) {
		if (DOCUMENT(tmplist->data)->uri
			&& (DOCUMENT(tmplist->data)->uri == uri || g_file_equal(DOCUMENT(tmplist->data)->uri, uri))) {
			DEBUG_MSG("documentlist_return_document_from_filename, found, returning %p\n", tmplist->data);
			if (tmplist->data != g_hash_table_lookup(main_v->alldochash, uri)) {
				g_warning("document uri hash table is corrupt!!!!!!!!!!!!!!\n");
			} else {
				g_print("alldochash is correct\n");
			}
			return DOCUMENT(tmplist->data);
		}
		tmplist = g_list_next(tmplist);
	}
	DEBUG_MSG("documentlist_return_document_from_filename, not found, returning NULL\n");
	return NULL;
#else
	return g_hash_table_lookup(main_v.alldochash, uri);
#endif
}

/**
 * documentlist_return_document_from_index:
 * @doclist: #GList* with the documents to search in
 * @index: a #gint, index in the documentlist.
 *
 * If the index is valid, it returns the appropriate Tdocument.
 *
 * Return value: Pointer to Tdocument on success, NULL on invalid index.
 **/
Tdocument *
documentlist_return_document_from_index(GList * doclist, gint index)
{
	return (Tdocument *) g_list_nth_data(doclist, index);
}

/**
 * document_return_num_loading:
 * @doclist: a list of Tdocument* to count
 *
 * Return value: number of documents that are not 'complete'
 */
gint
document_return_num_notcomplete(GList * doclist)
{
	GList *tmplist;
	gint count = 0;
	for (tmplist = g_list_first(doclist); tmplist != NULL; tmplist = tmplist->next) {
		if (DOCUMENT(tmplist->data)->status != DOC_STATUS_COMPLETE)
			count++;
	}
	return count;
}

/**
 * doc_update_highlighting:
 * @bfwin: #Tbfwin* with the window
 * @callback_action: #guint ignored
 * @widget: a #GtkWidget* ignored
 *
 * this function works on the current document
 * if highlighting is disabled, this enables the highlighting
 * the highlighting is also refreshed for the full document
 *
 * Return value: void
 **/
void
doc_update_highlighting(Tbfwin * bfwin, guint callback_action, GtkWidget * widget)
{
	if (!bfwin->current_document)
		return;
	if (!BLUEFISH_TEXT_VIEW(bfwin->current_document->view)->enable_scanner) {
		DEBUG_MSG("doc_update_highlighting, set enable_scanner to TRUE\n");
		BLUEFISH_TEXT_VIEW(bfwin->current_document->view)->enable_scanner = TRUE;
	}
	bluefish_text_view_rescan(BLUEFISH_TEXT_VIEW(bfwin->current_document->view));
}

/**
 * doc_set_wrap:
 * @doc: a #Tdocument
 *
 * this function will synchronise doc->wrapstate with the textview widget
 * if doc->wrapstate TRUE it will set the textview to GTK_WRAP_WORD
 * else (FALSE) it will set the textview to GTK_WRAP_NONE
 *
 * Return value: void
 **/
void
doc_set_wrap(Tdocument * doc, gboolean enabled)
{
	GtkWrapMode wmode;
	if (enabled) {
		wmode = main_v->props.wrap_on_right_margin?GTK_WRAP_CHAR:GTK_WRAP_WORD;
	} else {
		wmode = GTK_WRAP_NONE;
	}
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(doc->view), wmode);
	if (doc->slave)
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(doc->slave), wmode);
}

/**
 * doc_set_tooltip:
 * @doc: #Tdocument*
 *
 * will set the tooltip on the notebook tab eventbox
 *
 * Return value: void
 */
void
doc_set_tooltip(Tdocument * doc)
{
	GString *retstr;
	gchar *tmp;
	gchar *mtimestr = NULL;
	gchar *sizestr = NULL;

	retstr = g_string_new(_("Name: "));
	retstr = g_string_append(retstr, gtk_label_get_text(GTK_LABEL(doc->tab_menu)));
	if (BLUEFISH_TEXT_VIEW(doc->view)->bflang) {
		retstr = g_string_append(retstr, _("\nLanguage mode: "));
		retstr = g_string_append(retstr, BLUEFISH_TEXT_VIEW(doc->view)->bflang->name);
	}
	if (doc->encoding) {
		retstr = g_string_append(retstr, _("\nEncoding: "));
		retstr = g_string_append(retstr, doc->encoding);
	}
	DEBUG_MSG("doc_set_tooltip, fileinfo=%p for doc %s\n", doc->fileinfo,
			  gtk_label_get_text(GTK_LABEL(doc->tab_menu)));
	if (doc->fileinfo) {
		const gchar *mime =
			g_file_info_get_attribute_string(doc->fileinfo, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);
		if (!mime)
			mime =
				g_file_info_get_attribute_string(doc->fileinfo, G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE);
		if (mime) {
			retstr = g_string_append(retstr, _("\nMime type: "));
			retstr = g_string_append(retstr, mime);
		}

		if (g_file_info_has_attribute(doc->fileinfo, G_FILE_ATTRIBUTE_UNIX_MODE)) {
			tmp =
				filemode_to_string(g_file_info_get_attribute_uint32
								   (doc->fileinfo, G_FILE_ATTRIBUTE_UNIX_MODE));
			g_string_append_printf(retstr, _("\nPermissions: %s\nUid: %u Gid: %u"), tmp,
								   g_file_info_get_attribute_uint32(doc->fileinfo, G_FILE_ATTRIBUTE_UNIX_UID),
								   g_file_info_get_attribute_uint32(doc->fileinfo,
																	G_FILE_ATTRIBUTE_UNIX_GID));
			g_free(tmp);
		}
		if (g_file_info_has_attribute(doc->fileinfo, G_FILE_ATTRIBUTE_STANDARD_SIZE)) {
			DEBUG_MSG("doc_set_tooltip: size for %s is %" G_GOFFSET_FORMAT "\n",
					  gtk_label_get_text(GTK_LABEL(doc->tab_menu)), g_file_info_get_size(doc->fileinfo));
#if (GLIB_CHECK_VERSION(2,30,0))
			guint64 size;

			size = g_file_info_get_attribute_uint64(doc->fileinfo, G_FILE_ATTRIBUTE_STANDARD_SIZE);
			sizestr = g_format_size(size);
#else
			sizestr = g_format_size_for_display(g_file_info_get_size(doc->fileinfo));
#endif
			retstr = g_string_append(retstr, _("\nSize (on disk): "));
			retstr = g_string_append(retstr, sizestr);
			g_free(sizestr);
		}
		if (g_file_info_has_attribute(doc->fileinfo, G_FILE_ATTRIBUTE_TIME_MODIFIED)) {
			/* this function always appends a newline to the string */
			time_t modtime =
				(time_t) g_file_info_get_attribute_uint64(doc->fileinfo, G_FILE_ATTRIBUTE_TIME_MODIFIED);
			mtimestr = bf_portable_time(&modtime);
			retstr = g_string_append(retstr, _("\nLast modified: "));
			retstr = g_string_append(retstr, mtimestr);
			g_free(mtimestr);
		}
	}
	tmp = g_string_free(retstr, FALSE);
	gtk_widget_set_tooltip_text(doc->tab_eventbox, tmp);
	g_free(tmp);
}

static void
tab_label_set_string(Tdocument *doc, const gchar *string)
{
	gtk_label_set_text(GTK_LABEL(doc->tab_label), string);
	if (main_v->props.max_shown_filename_len > 5) {
		gint len = g_utf8_strlen(string, -1);
		if (len > main_v->props.max_shown_filename_len) {
			gtk_label_set_width_chars(GTK_LABEL(doc->tab_label), MIN(main_v->props.max_shown_filename_len, len));
			gtk_label_set_ellipsize(GTK_LABEL(doc->tab_label), PANGO_ELLIPSIZE_MIDDLE);
			return;
		}
	}
	gtk_label_set_ellipsize(GTK_LABEL(doc->tab_label), PANGO_ELLIPSIZE_NONE);
}

/**
 * doc_set_title:
 * @doc: #Tdocument*
 *
 * will set the notebook tab label and the notebook tab menu label
 * and if this document->bfwin == document->bfwin->current_document
 * it will update the bfwin title
 * it will also call doc_set_tooltip() to reflect the changes in the tooltip
 *
 * Return value: void
 */
void
doc_set_title(Tdocument * doc, const gchar *override_label_string)
{
	gchar *label_string, *tabmenu_string;
	if (override_label_string) {
		label_string = g_strdup(override_label_string);
		tabmenu_string = g_strdup(override_label_string);
	} else if (doc->uri) {
		gchar *parsename, *basename;
		if (g_file_is_native(doc->uri)) {
			tabmenu_string = g_file_get_path(doc->uri);
		} else {
			tabmenu_string = g_file_get_uri(doc->uri);
		}
		parsename = g_file_get_parse_name(doc->uri);
		basename = g_path_get_basename(parsename);
		label_string = g_strdup(basename);
		g_free(parsename);
		g_free(basename);
	} else {
		label_string = g_strdup_printf(_("Untitled %d"), main_v->num_untitled_documents);
		tabmenu_string = g_strdup(label_string);
		main_v->num_untitled_documents++;
	}
	gtk_label_set_text(GTK_LABEL(doc->tab_menu), tabmenu_string);
	tab_label_set_string(doc, label_string);
	DEBUG_MSG("doc_set_title, tabmenu_string=%s,label_string=%s\n", tabmenu_string, label_string);
	doc_set_tooltip(doc);
	g_free(label_string);
	g_free(tabmenu_string);
	if (doc == BFWIN(doc->bfwin)->current_document) {
		bfwin_set_title(doc->bfwin, doc, 0);
	}
}

void
doc_set_mimetype(Tdocument * doc, const gchar * mimetype, const gchar * filename)
{
	DEBUG_MSG("doc_set_mimetype(%p, %s)\n", doc, mimetype);
	if (doc->newdoc_autodetect_lang_id) {
		/*g_print("disable newdoc_autodetect_lang_id for doc %p\n", doc);*/
		g_source_remove(doc->newdoc_autodetect_lang_id);
		doc->newdoc_autodetect_lang_id = 0;
	}
	bluefish_text_view_select_language(BLUEFISH_TEXT_VIEW(doc->view), mimetype, filename);
	if (doc->fileinfo) {
		g_file_info_set_content_type(doc->fileinfo, mimetype);
	}
	doc_set_statusbar_lang_encoding(doc);
}

/**
 * doc_reset_filetype:
 * @doc: #Tdocument to reset
 * @newfilename: a #gchar* with the new filename
 * @buf: the contents of the file
 * @buflen: the size of the contents
 *
 * sets the new filetype based on newfilename and content, updates the widgets and highlighting
 * (using doc_set_filetype())
 *
 * Return value: void
 **/
void
doc_reset_filetype(Tdocument * doc, GFile * newuri, gconstpointer buf, gssize buflen)
{
	gboolean uncertain = FALSE;
	gchar *filename = NULL, *conttype;
#ifdef WIN32
	gchar *tmp;
#endif

	if (newuri)
		filename = g_file_get_basename(newuri);
	conttype = g_content_type_guess(filename, buf, buflen, &uncertain);

#ifdef WIN32
	/* on WIN32 conttype does not contain a MIME type, that has to be converted */
	tmp = g_content_type_get_mime_type(conttype);
	g_free(conttype);
	conttype = tmp;
#endif
	if (strcmp(conttype, "text/html") == 0 && buf) {
		const gchar *newtype=NULL;
		if (strstr(buf, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML") != NULL) {
			newtype = "application/xhtml+xml";
		} else if (strstr(buf, "<!DOCTYPE html>")!=NULL) {
			newtype = "text/x-html5";
		}
		if (newtype) {
			g_free(conttype);
			conttype = g_strdup(newtype);
		}
	}
	DEBUG_MSG("doc_reset_filetype, call doc_set_mimetype for doc=%p\n",doc);
	doc_set_mimetype(doc, conttype, filename);
	g_free(filename);
	g_free(conttype);
}

/**
 * This function is taken from gtksourceview
 * Copyright (C) 2001
 * Mikael Hermansson <tyan@linux.se>
 * Chris Phelps <chicane@reninet.com>
 */
static gint
textview_calculate_real_tab_width(GtkWidget * textview, gint tab_size)
{
	gchar *tab_string;
	gint tab_width = 0;

	if (tab_size <= 0)
		return 0;

	tab_string = g_strnfill(tab_size, ' ');
	tab_width = widget_get_string_size(textview, tab_string);
	DEBUG_MSG("textview_calculate_real_tab_width, got %d for '%s' on widget %p\n",tab_width,tab_string,textview);
	g_free(tab_string);
/*	if (tab_width < 0) tab_width = 0;*/
	return tab_width;
}

/**
 * doc_set_tabsize:
 * @doc: a #Tdocument
 * @tabsize: a #gint with the tab size
 *
 * this function will set the textview from doc to use the tabsize
 * described by tabsize
 *
 * Return value: void
 **/
void
doc_set_tabsize(Tdocument * doc, gint tabsize)
{
	PangoTabArray *tab_array;
	gint pixels = textview_calculate_real_tab_width(GTK_WIDGET(doc->view), tabsize);
	DEBUG_MSG("doc_set_tabsize, tabsize=%d, pixels=%d\n", tabsize, pixels);
	tab_array = pango_tab_array_new(1, TRUE);
	pango_tab_array_set_tab(tab_array, 0, PANGO_TAB_LEFT, pixels);
	gtk_text_view_set_tabs(GTK_TEXT_VIEW(doc->view), tab_array);
	if (doc->slave)
		gtk_text_view_set_tabs(GTK_TEXT_VIEW(doc->slave), tab_array);
	pango_tab_array_free(tab_array);
}

gint
doc_get_tabsize(Tdocument * doc)
{
	PangoTabArray *tab_array;
	PangoTabAlign align;
	gint setsize;

	tab_array = gtk_text_view_get_tabs(GTK_TEXT_VIEW(doc->view));
	if (tab_array) {
		gint singlesize;

		singlesize = textview_calculate_real_tab_width(doc->view, 1);
		pango_tab_array_get_tab(tab_array, 0, &align, &setsize);
		pango_tab_array_free(tab_array);
		DEBUG_MSG("doc_get_tabsize, return %d/%d=%d\n",setsize,singlesize,setsize/singlesize);
		return setsize / singlesize;
	}
	return 8;
}

/**
 * gui_change_tabsize:
 * @bfwin: #Tbfwin* with the window
 * @action: a #guint, if 1 increase the tabsize, if 0 decrease
 * @widget: a #GtkWidget, ignored
 *
 * this function is the callback for the menu, based on action
 * it will increase or decrease the tabsize by one
 *
 * Return value: void
 **/
void
doc_change_tabsize(Tdocument * doc, gint direction)
{
	PangoTabArray *tab_array;
	PangoTabAlign align;
	gint setsize, singlesize;
	gchar *message;
	singlesize = textview_calculate_real_tab_width(doc->view, 1);
	tab_array = gtk_text_view_get_tabs(GTK_TEXT_VIEW(doc->view));
	if (tab_array) {
		pango_tab_array_get_tab(tab_array, 0, &align, &setsize);
		/*g_print("doc_change_tabsize, got setsize=%d\n",setsize); */
	} else {
		tab_array = pango_tab_array_new(1, TRUE);
		setsize = 8;
	}
	if (direction == 0) {		/* 0 means reset to default */
		setsize = BFWIN(doc->bfwin)->session->editor_tab_width * singlesize;
	} else if (direction < 0) {
		setsize -= singlesize;
	} else {
		setsize += singlesize;
	}
	message = g_strdup_printf(_("Changed tab width to %d"), setsize / singlesize);
	bfwin_statusbar_message(BFWIN(doc->bfwin), message, 2);
	g_free(message);
	/*g_print("doc_change_tabsize, set setsize=%d\n",setsize); */
	pango_tab_array_set_tab(tab_array, 0, PANGO_TAB_LEFT, setsize);
	gtk_text_view_set_tabs(GTK_TEXT_VIEW(doc->view), tab_array);
	if (doc->slave)
		gtk_text_view_set_tabs(GTK_TEXT_VIEW(doc->slave), tab_array);
	pango_tab_array_free(tab_array);
}

void
doc_font_size(Tdocument * doc, gint direction)
{
	PangoFontDescription *font_desc;
	if (direction == 0) {
		font_desc = pango_font_description_from_string(main_v->props.editor_font_string);
	} else {
		PangoContext *pc;
		gint size;

		pc = gtk_widget_get_pango_context(doc->view);
		font_desc = pango_font_description_copy(pango_context_get_font_description(pc));
		size = pango_font_description_get_size(font_desc);
		size = (direction > 0) ? size * 12 / 10 : size * 10 / 12;
		if (pango_font_description_get_size_is_absolute(font_desc)) {
			pango_font_description_set_absolute_size(font_desc, size);
		} else {
			pango_font_description_set_size(font_desc, size);
		}
		/* do we have to unref the pangocontext, bluefish crashes if we do so, so I guess not ? */
	}
	bluefish_text_view_set_font(BLUEFISH_TEXT_VIEW(doc->view), font_desc);
	pango_font_description_free(font_desc);
}

/**
 * doc_is_empty_non_modified_and_nameless:
 * @doc: a #Tdocument
 *
 * this function returns TRUE if the document pointer to by doc
 * is an empty, nameless and non-modified document
 *
 * Return value: gboolean, TRUE if doc is empty, non-modified and nameless
 **/
gboolean
doc_is_empty_non_modified_and_nameless(Tdocument * doc)
{
	if (!doc) {
		return FALSE;
	}
	if (doc->modified || doc->uri || doc->autosave_uri || doc->status == DOC_STATUS_LOADING) {
		return FALSE;
	}
	if (gtk_text_buffer_get_char_count(doc->buffer) > 0) {
		return FALSE;
	}
	return TRUE;
}

/**
 * test_only_empty_doc_left:
 * @doclist: #GList* with all documents to test in
 *
 * returns TRUE if there is only 1 document open, and that document
 * is not modified and 0 bytes long and without filename
 * returns FALSE if there are multiple documents open, or
 * a modified document is open, or a > 0 bytes document is open
 * or a document with filename is open
 *
 * Return value: void
 **/
gboolean
test_only_empty_doc_left(GList * doclist)
{
	if (g_list_length(doclist) > 1) {
		return FALSE;
	} else {
		Tdocument *tmpdoc;
		GList *tmplist = g_list_first(doclist);
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

/**
 * doc_move_to_window:
 * @doc: #Tdocument*
 * @newwin: #Tbfwin*
 *
 * detaches the document from it's old window (doc->bfwin) and attaches
 * it to the window newwin
 *
 * Return value: void, ignored
 */
void
doc_move_to_window(Tdocument * doc, Tbfwin * oldwin, Tbfwin * newwin)
{
	GtkWidget *tab_widget;
	DEBUG_MSG("doc_move_to_window, oldwin=%p, newwin=%p, doc=%p\n", oldwin, newwin, doc);
	/* first test if the document still exists in the old window */
	if (g_list_index(oldwin->documentlist, doc) == -1 || !bfwin_exists(oldwin) || !bfwin_exists(newwin)) {
		DEBUG_MSG("doc_move_to_window, the document no longer exists in oldwin %p\n", oldwin);
		return;
	}
	tab_widget = gtk_widget_get_parent(doc->tab_eventbox);
	g_object_ref(G_OBJECT(doc->vsplit));
	g_object_ref(G_OBJECT(tab_widget));
	g_object_ref(G_OBJECT(doc->tab_menu));
	DEBUG_MSG("doc_move_to_window, tab_label=%p, tab_widget=%p\n", doc->tab_label, tab_widget);
/*	gtk_container_remove(GTK_CONTAINER(oldwin->notebook), doc->view);*/
	gtk_notebook_remove_page(GTK_NOTEBOOK(oldwin->notebook), g_list_index(oldwin->documentlist, doc));
	oldwin->documentlist = g_list_remove(oldwin->documentlist, doc);
	DEBUG_MSG("doc_move_to_window, removed doc=%p from oldwin %p\n", doc, oldwin);
	doc->bfwin = newwin;
	newwin->documentlist = g_list_append(newwin->documentlist, doc);
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(newwin->notebook), doc->vsplit, tab_widget, doc->tab_menu);
	DEBUG_MSG("doc_move_to_window, appended doc=%p to newwin %p\n", doc, newwin);

	g_object_unref(G_OBJECT(doc->vsplit));
	g_object_unref(G_OBJECT(tab_widget));
	g_object_unref(G_OBJECT(doc->tab_menu));

	gtk_widget_show_all(doc->vsplit);
	gtk_widget_show_all(tab_widget);
	gtk_widget_show(doc->tab_menu);

	if (NULL == oldwin->documentlist) {
		file_new_cb(NULL, oldwin);
	}
}

typedef struct {
	Tdocument *doc;
	Tbfwin *newwin;
	Tbfwin *oldwin;
} Tdmwd;

static void
doc_move_to_window_dialog_response_lcb(GtkDialog * dialog, gint response, gpointer user_data)
{
	Tdmwd *dmwd = (Tdmwd *) user_data;
	if (response == 1) {
		doc_move_to_window(dmwd->doc, dmwd->oldwin, dmwd->newwin);
	} else if (response == 2) {
		/* TODO: open readonly */
		file_doc_from_uri(dmwd->newwin, dmwd->doc->uri, NULL, NULL, -1, -1, TRUE);
	} else {
		/* TODO: do not open */
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_free(dmwd);
}

void
doc_move_to_window_dialog(Tdocument * doc, Tbfwin * newwin)
{
	GtkWidget *dialog;
	Tdmwd *dmwd;
	dmwd = g_new(Tdmwd, 1);
	dmwd->doc = doc;
	dmwd->newwin = newwin;
	dmwd->oldwin = doc->bfwin;
	dialog =
		gtk_message_dialog_new(GTK_WINDOW(newwin->main_window), GTK_DIALOG_DESTROY_WITH_PARENT,
							   GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
							   _("Document %s is already open in another Bluefish window.")
							   , gtk_label_get_text(GTK_LABEL(doc->tab_label)));
	gtk_dialog_add_buttons(GTK_DIALOG(dialog), _("Move to this window"), 1, _("Open readonly"), 2,
						   _("Do not open"), 3, NULL);
	g_signal_connect(dialog, "response", G_CALLBACK(doc_move_to_window_dialog_response_lcb), dmwd);
	gtk_widget_show_all(dialog);
}


/**
 * doc_set_label_color:
 * @doc: a #Tdocument
 * @color: a parsable string or NULL
 *
 * sets the text of the notebook tab and tab menu to color
 * if color is NULL, the color is reset to the gtk-theme setting
 *
 * Return value: void
 **/
static void
doc_set_label_color(Tdocument * doc, const gchar * color)
{
	GdkColor labelcolor;
	GdkColor *color_p = NULL;
	if (color != NULL) {
		gdk_color_parse(color, &labelcolor);
		color_p = &labelcolor;
	}
	gtk_widget_modify_fg(doc->tab_menu, GTK_STATE_NORMAL, color_p);
	gtk_widget_modify_fg(doc->tab_menu, GTK_STATE_PRELIGHT, color_p);
	gtk_widget_modify_fg(doc->tab_label, GTK_STATE_NORMAL, color_p);
	gtk_widget_modify_fg(doc->tab_label, GTK_STATE_PRELIGHT, color_p);
	gtk_widget_modify_fg(doc->tab_label, GTK_STATE_ACTIVE, color_p);
}

void
doc_set_status(Tdocument * doc, gint status)
{
	gchar *color = NULL;
	doc->status = status;
	switch (status) {
	case DOC_STATUS_COMPLETE:
		doc->modified = FALSE;
		break;
	case DOC_STATUS_ERROR:
		color = main_v->props.tab_color_error;
		break;
	case DOC_STATUS_LOADING:
		color = main_v->props.tab_color_loading;
		break;
	}
	doc_set_label_color(doc, color);
}

/**
 * doc_set_modified:
 * @doc: a #Tdocument
 * @value: a gint TRUE or FALSE
 *
 * sets the doc->modified to value
 * if it already has this value, do nothing
 * if it does not have this value, it will do some action
 *
 * if the document pointed to by doc == the current document
 * it will update the toolbar and menu undo/redo items
 *
 * if value is TRUE, it will make the notebook and notebook-menu
 * label red, if value is FALSE it will set them to normal
 *
 * Return value: void
 **/
void
doc_set_modified(Tdocument * doc, gboolean value)
{
	DEBUG_MSG("doc_set_modified, started, doc=%p, value=%d\n", doc, value);
	if (value) {
		need_autosave(doc);
	} else {
		remove_autosave(doc);
	}
	if (doc->modified != value) {
		gchar *color = NULL;
		gint change = (value - doc->modified);
		doc->modified = value;
		if (doc->modified)
			color = main_v->props.tab_color_modified;
		doc_set_label_color(doc, color);
		bfwin_set_title(BFWIN(doc->bfwin), BFWIN(doc->bfwin)->current_document, change);
	}
#ifdef DEBUG
	else {
		DEBUG_MSG("doc_set_modified, doc %p did have value %d already\n", doc, value);
	}
#endif
	/* only when this is the current document we have to change these */
	DEBUG_MSG("doc=%p, doc->bfwin=%p\n", doc, doc->bfwin);
	if (doc == BFWIN(doc->bfwin)->current_document) {
		bfwin_set_undo_redo_actions(BFWIN(doc->bfwin), doc_has_undo_list(doc), doc_has_redo_list(doc));
	}
#ifdef DEBUG
	else {
		DEBUG_MSG("doc_set_modified, doc != current_document, so we do not update the gui widgets\n");
	}
#endif
}

/**
 * doc_scroll_to_cursor:
 * @doc: a #Tdocument
 *
 * scolls the document pointer to by doc to its cursor position,
 * making the cursor visible
 *
 * Return value: void
 **/
void
doc_scroll_to_cursor(Tdocument * doc)
{
	GtkTextMark *mark = gtk_text_buffer_get_insert(doc->buffer);
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(doc->view), mark, 0.25, FALSE, 0.5, 0.5);
}

/**
 * doc_get_chars:
 * @doc: a #Tdocument
 * @start: a #gint, the start position
 * @end: a #gint, the end position
 *
 * returns all characters (NOT BYTES!!) from start to end from the document
 * pointer to by doc. end may be -1 to point to the end of the document
 *
 * Return value: gchar * with the requested characters
 **/
gchar *
doc_get_chars(Tdocument * doc, gint start, gint end)
{
	GtkTextIter itstart, itend;
	gchar *string;

	gtk_text_buffer_get_iter_at_offset(doc->buffer, &itstart, start);
	if (end >= 0) {
		gtk_text_buffer_get_iter_at_offset(doc->buffer, &itend, end);
	} else if (end == -1) {
		gtk_text_buffer_get_end_iter(doc->buffer, &itend);
	} else {
		g_warning("invalid call to doc_get_chars, end < -1, returning NULL\n");
		return NULL;
	}
	DEBUG_MSG("doc_get_chars, retrieving string, start=%d, end=%d\n", start, end);
	string = gtk_text_buffer_get_text(doc->buffer, &itstart, &itend, TRUE);
	DEBUG_MSG("doc_get_chars, retrieved string (%p)\n", string);
	return string;
}

void
doc_select_and_scroll(Tdocument * doc, GtkTextIter * it1,
					  GtkTextIter * it2, gboolean select_it1_line, gboolean do_scroll)
{
	GtkTextIter sit1 = *it1, sit2 = *it2;

	if (select_it1_line) {
		sit2 = sit1;
		gtk_text_iter_set_line_offset(&sit1, 0);
		gtk_text_iter_forward_to_line_end(&sit2);
	}
	gtk_text_buffer_select_range(doc->buffer, &sit1, &sit2);
	if (do_scroll) {
		/* in doc reload this works strange, there is no scrolling to the correct position...
		   perhaps this should be done in an idle callback so that the iter positions can be calculated?? */
		gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(doc->view), &sit1, 0.25, FALSE, 0.5, 0.95);
		gtk_widget_grab_focus(doc->view);
	}
}

/**
 * doc_select_region:
 * @doc: a #Tdocument
 * @start: a #gint with the start of selection
 * @end: a #gint with the end of the selection
 * @do_scroll: a #gboolean, if we should scroll to the selection
 *
 * selects from start to end in the doc, and if do_scroll is set it will make
 * sure the selection is visible to the user
 *
 * Return value: void
 **/
/*void doc_select_region(Tdocument *doc, gint start, gint end, gboolean do_scroll) {
	GtkTextIter itstart, itend;
	gtk_text_buffer_get_iter_at_offset(doc->buffer, &itstart,start);
	gtk_text_buffer_get_iter_at_offset(doc->buffer, &itend,end);
	doc_select_and_scroll(doc, &itstart, &itend,FALSE, do_scroll);
}*/

/**
 * doc_select_line:
 * @doc: a #Tdocument
 * @line: a #gint with the line number to select
 * @do_scroll: a #gboolean, if we should scroll to the selection
 *
 * selects the line in doc, and if do_scroll is set it will make
 * sure the selection is visible to the user
 * the line number starts at line 1, not at line 0!!
 *
 * Return value: void
 **/
void
doc_select_line(Tdocument * doc, gint line, gboolean do_scroll)
{
	GtkTextIter itstart;
	gtk_text_buffer_get_iter_at_line(doc->buffer, &itstart, line - 1);
	doc_select_and_scroll(doc, &itstart, &itstart, TRUE, do_scroll);
}

/**
 * doc_select_line_by_offset:
 * @doc: a #Tdocument
 * @offset: a #gint with the offset of the line number to select
 * @do_scroll: a #gboolean, if we should scroll to the selection
 *
 * selects the line in doc, and if do_scroll is set it will make
 * sure the selection is visible to the user
 *
 * Return value: void
 **/
void
doc_select_line_by_offset(Tdocument * doc, gint offset, gboolean do_scroll)
{
	GtkTextIter itstart;
	gtk_text_buffer_get_iter_at_offset(doc->buffer, &itstart, offset);
	doc_select_and_scroll(doc, &itstart, &itstart, TRUE, do_scroll);
}

/**
 * doc_get_selection:
 * @doc: a #Tdocument
 * @start: a #gint * to store the start
 * @end: a #gint * to store the end
 *
 *  returns FALSE if there is no selection
 *  returns TRUE if there is a selection, and start and end will be set
 *  to the current selection
 *
 * Return value: gboolean if there is a selection
 **/
gboolean
doc_get_selection(Tdocument * doc, gint * start, gint * end)
{
	GtkTextIter itstart, itend;
	GtkTextMark *mark = gtk_text_buffer_get_insert(doc->buffer);
	gtk_text_buffer_get_iter_at_mark(doc->buffer, &itstart, mark);
	mark = gtk_text_buffer_get_selection_bound(doc->buffer);
	gtk_text_buffer_get_iter_at_mark(doc->buffer, &itend, mark);
	if (gtk_text_iter_equal(&itstart, &itend))
		return FALSE;

	*start = gtk_text_iter_get_offset(&itstart);
	*end = gtk_text_iter_get_offset(&itend);
	DEBUG_MSG("doc_get_selection, start=%d, end=%d\n", *start, *end);
	if (*start > *end) {
		gint tmp = *start;
		*start = *end;
		*end = tmp;
	}
	return TRUE;
}

/**
 * doc_get_cursor_position:
 * @doc: a #Tdocument
 *
 * returns the cursor position in doc as character offset
 *
 * Return value: gint with the character offset of the cursor
 **/
gint
doc_get_cursor_position(Tdocument * doc)
{
	GtkTextIter iter;
	GtkTextMark *mark = gtk_text_buffer_get_insert(doc->buffer);
	gtk_text_buffer_get_iter_at_mark(doc->buffer, &iter, mark);
	return gtk_text_iter_get_offset(&iter);
}

/**
 * doc_set_statusbar_lncol:
 * @doc: a #Tdocument
 *
 * Return value: void
 **/
static void
doc_set_statusbar_lncol(Tdocument * doc)
{
	gchar *msg;
	gint line;
	gint col = 0;
	GtkTextIter iter, start;

	gtk_text_buffer_get_iter_at_mark(doc->buffer, &iter, gtk_text_buffer_get_insert(doc->buffer));

	line = gtk_text_iter_get_line(&iter);

	start = iter;
	gtk_text_iter_set_line_offset(&start, 0);

	while (!gtk_text_iter_equal(&start, &iter)) {
		if (gtk_text_iter_get_char(&start) == '\t') {
			col +=
				(BFWIN(doc->bfwin)->session->editor_tab_width -
				 (col % BFWIN(doc->bfwin)->session->editor_tab_width));
		} else
			++col;
		gtk_text_iter_forward_char(&start);
	}
	if (1) {
		msg =
			g_strdup_printf(_(" Ln: %d, Col: %d, Char: %d"), line + 1, col + 1,
							gtk_text_iter_get_offset(&iter));
	} else {
		msg = g_strdup_printf(_(" Ln: %d, Col: %d"), line + 1, col + 1);
	}

	gtk_statusbar_pop(GTK_STATUSBAR(BFWIN(doc->bfwin)->statusbar_lncol), 0);
	gtk_statusbar_push(GTK_STATUSBAR(BFWIN(doc->bfwin)->statusbar_lncol), 0, msg);

	g_free(msg);
}

/**
 * doc_set_statusbar_insovr:
 * @doc: a #Tdocument
 *
 *
 *
 * Return value: void
 **/
static void
doc_set_statusbar_insovr(Tdocument * doc)
{
	gtk_statusbar_pop(GTK_STATUSBAR(BFWIN(doc->bfwin)->statusbar_insovr), 0);
	if (!doc->readonly)
		gtk_statusbar_push(GTK_STATUSBAR(BFWIN(doc->bfwin)->statusbar_insovr), 0,
						   (gtk_text_view_get_overwrite(GTK_TEXT_VIEW(doc->view)) ? _(" OVR") : _(" INS")));
	else
		gtk_statusbar_push(GTK_STATUSBAR(BFWIN(doc->bfwin)->statusbar_insovr), 0, _(" RO"));
}

/**
 * doc_set_statusbar_mimetype_encoding:
 * @doc: a #Tdocument
 *
 * fills the statusbar for encoding and mimetype
 *
 * Return value: void
 **/
void
doc_set_statusbar_lang_encoding(Tdocument * doc)
{
	gchar *msg = NULL;

	if (BLUEFISH_TEXT_VIEW(doc->view)->bflang) {
		msg = g_strdup_printf(" %s, %s", BLUEFISH_TEXT_VIEW(doc->view)->bflang->name, doc->encoding);
	} else {
		msg = g_strdup_printf(" %s, %s", _("Unknown"), doc->encoding);
	}
	gtk_statusbar_pop(GTK_STATUSBAR(BFWIN(doc->bfwin)->statusbar_editmode), 0);
	gtk_statusbar_push(GTK_STATUSBAR(BFWIN(doc->bfwin)->statusbar_editmode), 0, msg);
	g_free(msg);
}


void
doc_insert_text_backend(Tdocument * doc, const gchar * newstring, gint position)
{
	GtkTextIter iter;
	doc_block_undo_reg(doc);
	gtk_text_buffer_get_iter_at_offset(doc->buffer, &iter, position);
	gtk_text_buffer_insert(doc->buffer, &iter, newstring, -1);
	doc_unre_add(doc, newstring, position, position + g_utf8_strlen(newstring, -1), UndoInsert);
	doc_unblock_undo_reg(doc);
	doc_set_modified(doc, 1);
}

/**
 * doc_replace_text_backend:
 * @doc: a #Tdocument
 * @newstring: a #const char * with the new string
 * @start: a gint with the start character position
 * @end: a gint with the end character position
 *
 * unbinds all signals so there will be no call to a highlighting
 * update or anything else
 * deletes the text in the region between start and end
 * registers that text to the undo/redo functionality
 * inserts newstring at that same position
 * registers this to the undo/redo functionality
 * marks the document as modified and marks it as needing highlighting
 * binds the signals again to their callbacks
 *
 * multiple calls to doc_replace_text_backend will all be in the same undo/redo group
 *
 * Return value: void
 **/
void
doc_replace_text_backend(Tdocument * doc, const gchar * newstring, gint start, gint end)
{
	doc_block_undo_reg(doc);
	/* delete region, and add that to undo/redo list */
	if (end == -1 || end > start) {
		gchar *buf;
		GtkTextIter itstart, itend;
		DEBUG_MSG("doc_replace_text_backend, get iters at start %d and end %d\n", start, end);
		gtk_text_buffer_get_iter_at_offset(doc->buffer, &itstart, start);
		if (end == -1) {
			gtk_text_buffer_get_end_iter(doc->buffer, &itend);
		} else {
			gtk_text_buffer_get_iter_at_offset(doc->buffer, &itend, end);
		}
		buf = gtk_text_buffer_get_text(doc->buffer, &itstart, &itend, TRUE);
		gtk_text_buffer_delete(doc->buffer, &itstart, &itend);
		DEBUG_MSG("doc_replace_text_backend, calling doc_unre_add for buf=%s, start=%d and end=%d\n", buf,
				  start, end);
		doc_unre_add(doc, buf, start, end, UndoDelete);
		g_free(buf);
		DEBUG_MSG("doc_replace_text_backend, text deleted from %d to %d\n", start, end);
	}

	/* add new text to this region, the buffer is changed so re-calculate itstart */
	if (newstring && newstring[0] != '\0') {
		GtkTextIter itstart;
		gint insert = (end > start) ? start : end;
		DEBUG_MSG("doc_replace_text_backend, set insert pos to %d\n", insert);
		gtk_text_buffer_get_iter_at_offset(doc->buffer, &itstart, insert);
		gtk_text_buffer_insert(doc->buffer, &itstart, newstring, -1);
		doc_unre_add(doc, newstring, insert, insert + g_utf8_strlen(newstring, -1), UndoInsert);
	}
	doc_unblock_undo_reg(doc);
	doc_set_modified(doc, 1);
}

/**
 * doc_replace_text:
 * @doc: a #Tdocument
 * @newstring: a #const char * with the new string
 * @start: a gint with the start character position
 * @end: a gint with the end character position
 *
 * identical to doc_replace_text_backend, with one difference, multiple calls to
 * doc_replace_text will be all be in a different undo/redo group
 *
 * Return value: void
 **/
void
doc_replace_text(Tdocument * doc, const gchar * newstring, gint start, gint end)
{
	doc_unre_new_group(doc);
	doc_replace_text_backend(doc, newstring, start, end);
	doc_unre_new_group(doc);
}

/*
static void doc_convert_case_in_selection(Tdocument *doc, gboolean toUpper) {
	gint start, end;
	if (doc_get_selection(doc, &start, &end)) {
		gchar *string = doc_get_chars(doc, start, end);
		if (string) {
			gchar *newstring = (toUpper) ? g_utf8_strup(string,-1) : g_utf8_strdown(string,-1);
			g_free(string);
			if (newstring) {
				doc_replace_text(doc, newstring, start, end);
				g_free(newstring);
			}
		}
	}
}
*/

/**
 * doc_insert_two_strings:
 * @doc: a #Tdocument
 * @before_str: a #const char * with the first string
 * @after_str: a #const char * with the second string
 *
 * if the marks 'diag_ins' and 'diag_sel' exist, they will be used
 * as pos1 and pos2
 * if a selection exists, the selection start and end will be pos1 and pos2
 * if both not exist the cursor position will be both pos1 and pos2
 *
 * inserts the first string at pos1 and the second at pos2 in doc
 * it does not unbind any signal, so the insert callback will have to do
 * do the undo/redo, modified and highlighting stuff
 *
 * multiple calls to this function will be in separate undo/redo groups
 *
 * Return value: void
 **/
void
doc_insert_two_strings(Tdocument * doc, const gchar * before_str, const gchar * after_str)
{
	GtkTextIter itinsert, itselect;
	GtkTextMark *insert, *select;
	gboolean have_diag_marks = FALSE;

	doc_unre_new_group(doc);
	doc_block_undo_reg(doc);
	insert = gtk_text_buffer_get_mark(doc->buffer, "diag_ins");
	select = gtk_text_buffer_get_mark(doc->buffer, "diag_sel");
	if (insert && select) {
		have_diag_marks = TRUE;
		DEBUG_MSG("doc_insert_two_strings have_diag_marks, diag_ins=%p, diag_sel=%p\n", insert, select);
	} else {
		insert = gtk_text_buffer_get_insert(doc->buffer);
		select = gtk_text_buffer_get_selection_bound(doc->buffer);
		DEBUG_MSG("doc_insert_two_strings no diag_marks, insert=%p, select=%p\n", insert, select);
	}
	gtk_text_buffer_get_iter_at_mark(doc->buffer, &itinsert, insert);
	gtk_text_buffer_get_iter_at_mark(doc->buffer, &itselect, select);
	DEBUG_MSG("doc_insert_two_strings, current marks: itinsert=%d, itselect=%d\n",
			  gtk_text_iter_get_offset(&itinsert), gtk_text_iter_get_offset(&itselect));
	if (gtk_text_iter_equal(&itinsert, &itselect)) {
		/* no selection */
		gint pos;
		gchar *double_str = g_strconcat(before_str, after_str, NULL);
		pos = gtk_text_iter_get_offset(&itinsert);
		DEBUG_MSG("doc_insert_two_strings, no selection, insert strings together as one string\n");
		doc_unre_add(doc, double_str, pos, pos + g_utf8_strlen(double_str, -1), UndoInsert);
		gtk_text_buffer_insert(doc->buffer, &itinsert, double_str, -1);
		g_free(double_str);
		if (after_str && strlen(after_str)) {
			/* the buffer has changed, but gtk_text_buffer_insert makes sure */
			/* that itinsert points to the end of the inserted text. */
			/* thus, no need to get a new one. */
			gtk_text_iter_backward_chars(&itinsert, g_utf8_strlen(after_str, -1));
			gtk_text_buffer_place_cursor(doc->buffer, &itinsert);
			gtk_widget_grab_focus(doc->view);
		}
	} else {					/* there is a selection */
		GtkTextMark *secondat;
		GtkTextIter *firstiter, seconditer;
		gint pos;
		if (gtk_text_iter_compare(&itinsert, &itselect) < 0) {
			DEBUG_MSG("doc_insert_two_strings, selection, with diag_marks=%d, first iter is itinsert at %d\n",
					  have_diag_marks, gtk_text_iter_get_offset(&itinsert));
			firstiter = &itinsert;
			secondat = select;
		} else {
			DEBUG_MSG("doc_insert_two_strings, selection with diag_marks=%d, first iter is itselect at %d\n",
					  have_diag_marks, gtk_text_iter_get_offset(&itselect));
			firstiter = &itselect;
			secondat = insert;
		}
		/* there is a selection */
		pos = gtk_text_iter_get_offset(firstiter);
		DEBUG_MSG("doc_insert_two_strings, insert first string at %d\n", pos);
		doc_unre_add(doc, before_str, pos, pos + g_utf8_strlen(before_str, -1), UndoInsert);
		gtk_text_buffer_insert(doc->buffer, firstiter, before_str, -1);
		if (after_str && strlen(after_str)) {
			/* the buffer is changed, reset the select iterator */
			gtk_text_buffer_get_iter_at_mark(doc->buffer, &seconditer, secondat);
			pos = gtk_text_iter_get_offset(&seconditer);
			DEBUG_MSG("doc_insert_two_strings, insert second string at mark %p at %d\n", secondat, pos);
			doc_unre_add(doc, after_str, pos, pos + g_utf8_strlen(after_str, -1), UndoInsert);
			gtk_text_buffer_insert(doc->buffer, &seconditer, after_str, -1);
			/* now the only thing left is to move the selection and insert mark back to their correct places
			   to preserve the users selection */
			if (have_diag_marks) {
				DEBUG_MSG("doc_insert_two_strings, reset selection with diag_marks\n");
				gtk_text_buffer_get_iter_at_mark(doc->buffer, &itinsert, insert);
				gtk_text_buffer_get_iter_at_mark(doc->buffer, &itselect, select);
				gtk_text_iter_forward_chars(firstiter, g_utf8_strlen(before_str, -1));
				gtk_text_buffer_select_range(doc->buffer, &itinsert, &itselect);
			} else {
				gtk_text_buffer_get_iter_at_mark(doc->buffer, &seconditer, secondat);
				gtk_text_iter_backward_chars(&seconditer, g_utf8_strlen(after_str, -1));
				gtk_text_buffer_move_mark(doc->buffer, secondat, &seconditer);
			}
		}

	}
	if (have_diag_marks) {
		gtk_text_buffer_delete_mark(doc->buffer, insert);
		gtk_text_buffer_delete_mark(doc->buffer, select);
	}
	doc_unre_new_group(doc);
	doc_unblock_undo_reg(doc);
	doc_set_modified(doc, 1);
	DEBUG_MSG("doc_insert_two_strings, finished\n");
}

static void
add_encoding_to_list(gchar * encoding)
{
	gboolean found = FALSE, changed = FALSE;
	GList *tmplist = g_list_first(main_v->globses.encodings);

	while (tmplist) {
		gchar **tmparr = tmplist->data;
		if (g_strv_length(tmparr) == 3 && g_ascii_strcasecmp(tmparr[1], encoding) == 0) {
			if (tmparr[2][0] != '1') {	/* enable this encoding */
				DEBUG_MSG("enable encoding %s\n", tmparr[0]);
				g_free(tmparr[2]);
				tmparr[2] = g_strdup("1");
				changed = TRUE;
			}
			found = TRUE;
		}
		tmplist = g_list_next(tmplist);
	}
	if (!found) {
		gchar *temp = g_ascii_strup(encoding, -1);
		main_v->globses.encodings =
			g_list_insert(main_v->globses.encodings, array_from_arglist(temp, temp, "1", NULL), 1);
		g_free(temp);
		changed = TRUE;
	}
	if (changed) {
		tmplist = g_list_first(main_v->bfwinlist);
		while (tmplist) {
			bfwin_encodings_menu_create(BFWIN(tmplist->data));
			tmplist = g_list_next(tmplist);
		}
	}
}

gchar *
encoding_by_regex(const gchar * buffer, const gchar * pattern, guint subpat)
{
	GRegex *reg1;
	GError *gerror = NULL;
	GMatchInfo *matchinfo = NULL;
	gchar *retstring = NULL;
	gboolean retval;
	reg1 = g_regex_new(pattern, G_REGEX_CASELESS | G_REGEX_EXTENDED, 0, &gerror);
	if (gerror) {
		g_warning("error while compiling regex pattern to search for encoding %d: %s\n", gerror->code,
				  gerror->message);
		g_error_free(gerror);
		return NULL;
	} else {
		retval = g_regex_match(reg1, buffer, 0, &matchinfo);
		if (retval && g_match_info_get_match_count(matchinfo) >= subpat) {
			retstring = g_match_info_fetch(matchinfo, subpat);
			DEBUG_MSG("encoding_by_regex, detected encoding %s\n", retstring);
		}
		g_match_info_free(matchinfo);
	}
	g_regex_unref(reg1);
	return retstring;
}

/**
 * buffer_find_encoding:
 * @buffer: gchar* with \- terminated string
 * @encoding: gchar**, if found a newly allocated encoding string will be here
 *
 * Return value: newly allocated buffer in UTF-8
 */
gchar *
buffer_find_encoding(gchar * buffer, gsize buflen, gchar ** encoding, const gchar * sessionencoding)
{
	gchar *newbuf = NULL;
	gsize wsize, rsize;
	GError *error = NULL;
	const gchar *end=NULL;
	gchar *tmpencoding = NULL;
	GList *tmplist;
	gchar endingbyte = '\0';
	/* the first try is if the encoding is set in the file
	   TODO right now only HTML is supported, but xml files should
	   use a different regex pattern to find the encoding */
	if (buflen > main_v->props.encoding_search_Nbytes) {
		/* we do a nasty trick to make regexec search only in the first N bytes */
		endingbyte = buffer[main_v->props.encoding_search_Nbytes];
		buffer[main_v->props.encoding_search_Nbytes] = '\0';
	}

	/* <meta http-equiv="content-type" content="text/html; charset=UTF-8" />
	   OR  <meta http-equiv="Content-Type" content="application/xhtml+xml; charset=iso-8859-1" />
	 */
	tmpencoding =
		encoding_by_regex(buffer,
						  "<meta[ \t\n\r\f]http-equiv[ \t\n\r\f]*=[ \t\n\r\f]*\"content-type\"[ \t\n\r\f]+content[ \t\n\r\f]*=[ \t\n\r\f]*\"[^;\"]+;[ \t\n\r\f]*charset=([a-z0-9_-]+)\"[ \t\n\r\f]*/?>",
						  1);
	if (!tmpencoding) {
		tmpencoding = encoding_by_regex(buffer, "encoding=\"([a-z0-9_-]+)\"", 1);
	}
	if (buflen > main_v->props.encoding_search_Nbytes) {
		buffer[main_v->props.encoding_search_Nbytes] = endingbyte;
	}

	if (tmpencoding) {
		DEBUG_MSG("doc_buffer_to_textbox, try encoding %s from <meta>\n", tmpencoding);
		newbuf = g_convert(buffer, buflen, "UTF-8", tmpencoding, &rsize, &wsize, &error);
		if (!newbuf || error) {
			DEBUG_MSG("doc_buffer_to_textbox, cound not convert %s to UTF-8, %"G_GSIZE_FORMAT" bytes read until error\n", tmpencoding, rsize);
			g_free(tmpencoding);
			tmpencoding = NULL;
			if (error) {
				g_error_free(error);
				error=NULL;
			}
		} else if (g_utf8_validate(newbuf, wsize, NULL)) {
			*encoding = tmpencoding;
			return newbuf;
		}
		g_free(newbuf);
	}

	/* because UTF-8 validation is very critical (very little texts in other encodings actually validate as UTF-8)
	we do this early in the detection */
	DEBUG_MSG("doc_buffer_to_textbox, file NOT is converted yet, trying UTF-8 encoding\n");
	if (g_utf8_validate(buffer, buflen, &end)) {
		*encoding = g_strdup("UTF-8");
		return g_strdup(buffer);
	} else {
		DEBUG_MSG("failed to validate as UTF-8, remaining buffer was '%s'\n", end);
		end=NULL;
	}

	if (sessionencoding) {
		DEBUG_MSG
			("doc_buffer_to_textbox, file does not have <meta> encoding, or could not convert and is not UTF8, trying session default encoding %s\n",
			 sessionencoding);
		newbuf = g_convert(buffer, buflen, "UTF-8", sessionencoding, &rsize, &wsize, &error);
		if (error) {
			DEBUG_MSG("failed to convert from sessionencoding %s, read %"G_GSIZE_FORMAT" bytes until error %s",sessionencoding,rsize,error->message);
			g_error_free(error);
			error=NULL;
		}
		if (newbuf && g_utf8_validate(newbuf, wsize, NULL)) {
			DEBUG_MSG("doc_buffer_to_textbox, file is in default encoding: %s, newbuf=%p, wsize=%"G_GSIZE_FORMAT", strlen(newbuf)=%zd\n", sessionencoding, newbuf, wsize, strlen(newbuf));
			*encoding = g_strdup(sessionencoding);
			return newbuf;
		}
		g_free(newbuf);
	}
	DEBUG_MSG
		("doc_buffer_to_textbox, file does not have <meta> encoding, or could not convert, not session encoding, not UTF8, trying newfile default encoding %s\n",
		 main_v->props.newfile_default_encoding);
	newbuf = g_convert(buffer, buflen, "UTF-8", main_v->props.newfile_default_encoding, NULL, &wsize, NULL);
	if (newbuf && g_utf8_validate(newbuf, wsize, NULL)) {
		DEBUG_MSG("doc_buffer_to_textbox, file is in default encoding: %s\n",
				  main_v->props.newfile_default_encoding);
		*encoding = g_strdup(main_v->props.newfile_default_encoding);
		return newbuf;
	}
	g_free(newbuf);

	DEBUG_MSG("doc_buffer_to_textbox, file is not in UTF-8, trying encoding from locale\n");
	newbuf = g_locale_to_utf8(buffer, buflen, NULL, &wsize, NULL);
	if (newbuf && g_utf8_validate(newbuf, wsize, NULL)) {
		const gchar *tmpencoding = NULL;
		g_get_charset(&tmpencoding);
		DEBUG_MSG("doc_buffer_to_textbox, file is in locale encoding: %s\n", tmpencoding);
		*encoding = g_strdup(tmpencoding);
		return newbuf;
	}
	g_free(newbuf);

	DEBUG_MSG("doc_buffer_to_textbox, tried the most obvious encodings, nothing found.. go trough list\n");
	tmplist = g_list_first(main_v->globses.encodings);
	while (tmplist) {
		gchar **enc = tmplist->data;
		if (enc[2] && enc[2][0] == '1') {
			DEBUG_MSG("doc_buffer_to_textbox, trying user set encoding %s\n", enc[1]);
			newbuf = g_convert(buffer, buflen, "UTF-8", enc[1], &rsize, &wsize, &error);
			if (error) {
				DEBUG_MSG("trying %s, error: %s\n",enc[1], error->message);
				g_error_free(error);
				error = NULL;
			}
			if (newbuf && g_utf8_validate(newbuf, wsize, NULL)) {
				*encoding = g_strdup(enc[1]);
				return newbuf;
			}
			g_free(newbuf);
		}
		tmplist = g_list_next(tmplist);
	}
	tmplist = g_list_first(main_v->globses.encodings);
	while (tmplist) {
		gchar **enc = tmplist->data;
		if (enc[2] || enc[2][0] != '1') {
			newbuf = g_convert(buffer, buflen, "UTF-8", enc[1], &rsize, &wsize, &error);
			if (error) {
				DEBUG_MSG("trying %s, error: %s\n",enc[1], error->message);
				g_error_free(error);
				error = NULL;
			}
			if (newbuf && g_utf8_validate(newbuf, wsize, NULL)) {
				*encoding = g_strdup(enc[1]);
				return newbuf;
			}
			g_free(newbuf);
		}
		tmplist = g_list_next(tmplist);
	}
	return NULL;
}

#define MAX_TOO_LONG_LINE 10000
#define MIN_TOO_LONG_LINE 9500

static gchar *
check_very_long_line(Tdocument *doc, gchar *buffer)
{
	gsize i=0, buflen;
	guint curline=0,maxline=0, numtoolong=0;

	for (i=0;buffer[i]!='\0';i++) {
		if (buffer[i] == '\n' || buffer[i] == '\r') {
			if (curline > maxline)
				maxline = curline;
			if (curline > MIN_TOO_LONG_LINE)
				numtoolong++;
			curline=0;
		}
		curline++;
	}
	if (curline > maxline)
		maxline = curline;
	if (curline > MIN_TOO_LONG_LINE)
		numtoolong++;
	buflen = i;
	DEBUG_MSG("check_very_long_line, maxline=%d, buflen=%ld\n",maxline,(long int)buflen);
	if (maxline > MAX_TOO_LONG_LINE) {
		gint response;
		const gchar *buttons[] = { _("_No"), _("_Split"), NULL };
		response = message_dialog_new_multi(BFWIN(doc->bfwin)->main_window,
						GTK_MESSAGE_WARNING, buttons,
						_("File contains very long lines. Split these lines?"), _("The lines in this file are longer than Bluefish can handle with reasonable performance. This split function, however, is unaware of any language syntax, and may replace spaces or tabs with newlines in any location, or insert newlines if no spaces or tabs are found."));
		if (response == 1) {
			gint inserted=0;
			buffer = g_realloc(buffer, buflen + numtoolong + (maxline / MIN_TOO_LONG_LINE) + 1);
			curline=0;
			for (i=0;buffer[i]!='\0';i++) {
				if (curline > MIN_TOO_LONG_LINE && (buffer[i] == ' ' || buffer[i] == '\t')) {
					DEBUG_MSG("check_very_long_line, replace space or tab at position %d with newline\n",i);
					buffer[i] = '\n';
					curline = 0;
				} else if (curline > MAX_TOO_LONG_LINE && (
								buffer[i] == ';' ||
								buffer[i] == ',' ||
								buffer[i] == '=' ||
								buffer[i] == '}' ||
								buffer[i] == '>' ||
								buffer[i] == ')' ||
								buffer[i] == '+' ||
								buffer[i] == '-'
								)) {
					DEBUG_MSG("check_very_long_line, insert newline at %d, move buffer %p to %p, %d bytes\n",i, buffer+i+1,buffer+i, buflen-i+inserted);
					memmove(buffer+i+1, buffer+i, buflen-i+inserted);
					buffer[i] = '\n';
					inserted++;
					curline = 0;
				} else if (buffer[i] == '\n' || buffer[i] == '\r') {
					curline = 0;
				}
				curline++;
			}
		}
	}
	return buffer;
}

/**
 * doc_buffer_to_textbox:
 * @doc: #Tdocument*
 * @buffer: #gchar*
 * @buflen: #gsize
 * @enable_undo: #gboolean, if the buffer insert should be undo-able
 * @delay: #gboolean
 *
 * inserts buffer at the current cursor position, tries to find the encoding of the document
 * using the contents of the buffer (<meta encoding)
 * and places the cursor back at this position
 *
 */
gboolean
doc_buffer_to_textbox(Tdocument * doc, gchar * buffer, gsize buflen, gboolean enable_undo, gboolean delay)
{
	gint cursor_offset;
	gchar *encoding = NULL, *newbuf;
	GtkTextMark *insert;
	GtkTextIter iter;

	if (!buffer) {
		DEBUG_MSG("doc_buffer_to_textbox, buffer==NULL, returning\n");
		return FALSE;
	}

	if (!enable_undo) {
		doc_block_undo_reg(doc);
	}
	/* now get the current cursor position */
	insert = gtk_text_buffer_get_insert(doc->buffer);
	gtk_text_buffer_get_iter_at_mark(doc->buffer, &iter, insert);
	cursor_offset = gtk_text_iter_get_offset(&iter);

	/* This opens the contents of a file to a textbox */

	newbuf = buffer_find_encoding(buffer, buflen, &encoding, BFWIN(doc->bfwin)->session->encoding);

	if (!newbuf) {
		message_dialog_new(BFWIN(doc->bfwin)->main_window,
						   GTK_MESSAGE_ERROR,
						   GTK_BUTTONS_CLOSE, _("Cannot display file, unknown characters found."), NULL);
		return FALSE;
	}
	DEBUG_MSG("doc_buffer_to_textbox, will set encoding to %s\n", encoding);
	if (doc->encoding)
		g_free(doc->encoding);
	doc->encoding = encoding;
	add_encoding_to_list(encoding);
	if (main_v->props.show_long_line_warning) {
		newbuf = check_very_long_line(doc, newbuf);
	}

	gtk_text_buffer_insert_at_cursor(doc->buffer, newbuf, -1);

	g_free(newbuf);
	if (!enable_undo) {
		doc_unblock_undo_reg(doc);
	}

	{
		/* set the cursor position back */
		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_offset(doc->buffer, &iter, cursor_offset);
		gtk_text_buffer_place_cursor(doc->buffer, &iter);
		if (!delay) {
			gtk_text_view_place_cursor_onscreen(GTK_TEXT_VIEW(doc->view));
		}
	}
	return TRUE;
}

static void
doc_buffer_insert_text_lcb(GtkTextBuffer * textbuffer, GtkTextIter * iter, gchar * string, gint len,
						   Tdocument * doc)
{
	GSList *tmpslist;
	gint pos = gtk_text_iter_get_offset(iter);
	gint clen = g_utf8_strlen(string, len);
	/*DEBUG_MSG("doc_buffer_insert_text_lcb, started, string='%s', len=%d, clen=%d\n", string, len, clen);*/
	/* 'len' is the number of bytes and not the number of characters.. */
	if (!doc->block_undo_reg) {
		if (!doc->in_paste_operation && (!doc_unre_test_last_entry(doc, UndoInsert, -1, pos)
									 || string[0] == ' '
									 || string[0] == '\n' || string[0] == '\t' || string[0] == '\r')) {
			DEBUG_MSG("doc_buffer_insert_text_lcb, create a new undogroup\n");
			doc_unre_new_group(doc);
		}
		doc_unre_add(doc, string, pos, pos + clen, UndoInsert);

		doc_set_modified(doc, 1);
	}
	/* see if any other code wants to see document changes */
	for (tmpslist=BFWIN(doc->bfwin)->doc_insert_text;tmpslist;tmpslist=g_slist_next(tmpslist)) {
		Tcallback *cb = tmpslist->data;
		((DocInsertTextCallback)cb->func)(doc, string, iter, pos, len, clen, cb->data);
	}
	DEBUG_MSG("doc_buffer_insert_text_lcb, done\n");
}

static void
doc_buffer_delete_range_lcb(GtkTextBuffer * textbuffer, GtkTextIter * itstart, GtkTextIter * itend,
							Tdocument * doc)
{
	GSList *tmpslist;
	gchar *string;
	gint start, end, len;

	start = gtk_text_iter_get_offset(itstart);
	end = gtk_text_iter_get_offset(itend);
	len = end - start;
	string = gtk_text_buffer_get_text(doc->buffer, itstart, itend, TRUE);

	DEBUG_MSG("doc_buffer_delete_range_lcb, string='%s'\n", string);
	if (!doc->block_undo_reg) {
		if (string) {
			/* undo_redo stuff */
			DEBUG_MSG("doc_buffer_delete_range_lcb, start=%d, end=%d, len=%d, string='%s'\n", start, end, len,
						  string);
			if (!doc->in_paste_operation) {
				if (len == 1) {
					if ((!doc_unre_test_last_entry(doc, UndoDelete, start, -1)	/* delete */
					 	&&!doc_unre_test_last_entry(doc, UndoDelete, end, -1))	/* backspace */
						||string[0] == ' ' || string[0] == '\n' || string[0] == '\t' || string[0] == '\r') {
						DEBUG_MSG("doc_buffer_delete_range_lcb, need a new undogroup\n");
						doc_unre_new_group(doc);
					}
				} else {
					doc_unre_new_group(doc);
				}
			}
			doc_unre_add(doc, string, start, end, UndoDelete);
		}
		doc_set_modified(doc, 1);
	}
	/* see if any other code wants to see document changes */
	for (tmpslist=BFWIN(doc->bfwin)->doc_delete_range;tmpslist;tmpslist=g_slist_next(tmpslist)) {
		Tcallback *cb = tmpslist->data;
		((DocDeleteRangeCallback)cb->func)(doc, itstart, start, itend, end, string, cb->data);
	}
	g_free(string);
}

void
doc_get_iter_location(Tdocument * doc, GtkTextIter * iter, GdkRectangle * rectangle)
{
	GdkRectangle rect;
	gint itx, ity, px, py;

	gtk_text_view_get_iter_location(GTK_TEXT_VIEW(doc->view), iter, &rect);

	/* the following function will return the position relative to the text area of the widget
	   but we also have margins!! */
	gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(doc->view)
										  , GTK_TEXT_WINDOW_TEXT, rect.x, rect.y, &itx, &ity);
	/* the following function will return the position of the total text widget */
	gdk_window_get_origin(gtk_widget_get_window(doc->view), &px, &py);

	DEBUG_MSG("doc_get_iter_location, px=%d, itx=%d,border=%d\n", px, itx,
			  gtk_text_view_get_border_window_size(GTK_TEXT_VIEW(doc->view), GTK_TEXT_WINDOW_LEFT));
	DEBUG_MSG("doc_get_iter_location, py=%d, ity=%d,border=%d\n", py, ity,
			  gtk_text_view_get_border_window_size(GTK_TEXT_VIEW(doc->view), GTK_TEXT_WINDOW_TOP));
	rectangle->x =
		px + itx + gtk_text_view_get_border_window_size(GTK_TEXT_VIEW(doc->view), GTK_TEXT_WINDOW_LEFT);
	rectangle->y =
		py + ity + gtk_text_view_get_border_window_size(GTK_TEXT_VIEW(doc->view), GTK_TEXT_WINDOW_TOP);
	rectangle->width = rect.width;
	rectangle->height = rect.height;
}

/*
void doc_get_cursor_location(Tdocument *doc, gint *x, gint *y) {
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark(doc->buffer,&iter,gtk_text_buffer_get_insert(doc->buffer));
	doc_get_iter_location(doc, &iter, x, y);
}
*/

static void
exit_fullscreen_lcb(GtkWidget *widget, Tbfwin *bfwin)
{
	bfwin_fullscreen_toggle(bfwin, FALSE);
}

static void
rpopup_add_bookmark_lcb(GtkWidget * widget, Tdocument * doc)
{
	bmark_add_at_bevent(doc);
}

static void
rpopup_del_bookmark_lcb(GtkWidget * widget, Tdocument * doc)
{
	bmark_del_at_bevent(doc);
}


static void
doc_view_populate_popup_lcb(GtkTextView * textview, GtkMenu * menu, Tdocument * doc)
{
	GtkWidget *menuitem;
	/* I found no way to connect an item-factory to this menu widget, so we have to do it in the manual way... */

/*#ifdef HAVE_LIBENCHANT
	bftextview2_populate_preferences_popup(menu, doc);
#endif*/

	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(gtk_menu_item_new()));
/*	menuitem = gtk_image_menu_item_new_with_label(_("Replace"));
	g_signal_connect(menuitem, "activate", G_CALLBACK(replace_cb), doc->bfwin);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),
								  gtk_image_new_from_stock(GTK_STOCK_FIND_AND_REPLACE, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));

	menuitem = gtk_image_menu_item_new_with_label(_("Find"));
	g_signal_connect(menuitem, "activate", G_CALLBACK(search_cb), doc->bfwin);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),
								  gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));

	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(gtk_menu_item_new()));
*/
	if (bmark_have_bookmark_at_stored_bevent(doc)) {
		menuitem = gtk_menu_item_new_with_label(_("Delete bookmark"));
		g_signal_connect(menuitem, "activate", G_CALLBACK(rpopup_del_bookmark_lcb), doc);
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));
	} else {
		menuitem = gtk_menu_item_new_with_label(_("Add bookmark"));
		g_signal_connect(menuitem, "activate", G_CALLBACK(rpopup_add_bookmark_lcb), doc);
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));
	}
	if (!gtk_widget_get_visible(GTK_WIDGET(BFWIN(doc->bfwin)->toolbarbox))) {
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(gtk_menu_item_new()));
		menuitem = gtk_menu_item_new_with_label(_("Exit fullscreen"));
		g_signal_connect(menuitem, "activate", G_CALLBACK(exit_fullscreen_lcb), doc->bfwin);
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));
	}
/*
	menuitem = gtk_menu_item_new_with_label(_("Add permanent bookmark"));
	g_signal_connect(menuitem, "activate", G_CALLBACK(rpopup_permanent_bookmark_lcb), doc->bfwin);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));

	menuitem = gtk_menu_item_new_with_label(_("Add temporary bookmark"));
	g_signal_connect(menuitem, "activate", G_CALLBACK(rpopup_temporary_bookmark_lcb), doc->bfwin);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem)); */

	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(gtk_menu_item_new()));

/* here we ask any plugins to add there menu item */
	if (main_v->doc_view_populate_popup_cbs) {
		GSList *tmplist = main_v->doc_view_populate_popup_cbs;
		while (tmplist) {
			void *(*func) () = tmplist->data;
			func(textview, menu, doc);
			tmplist = g_slist_next(tmplist);
		}
	}
#ifdef HAVE_LIBENCHANT
	bftextview2_populate_suggestions_popup(menu, doc);
#endif							/*HAVE_LIBENCHANT */

	gtk_widget_show_all(GTK_WIDGET(menu));
}

static void
doc_buffer_mark_set_lcb(GtkTextBuffer * buffer, GtkTextIter * iter, GtkTextMark * set_mark, Tdocument * doc)
{
	/*DEBUG_MSG("doc_buffer_mark_set_lcb, set_mark=%p, insert_mark=%p\n",set_mark,gtk_text_buffer_get_insert(buffer)); */
	if (set_mark == gtk_text_buffer_get_insert(buffer)) {
		doc_set_statusbar_lncol(doc);
	}
}

static void
doc_buffer_changed_lcb(GtkTextBuffer * textbuffer, Tdocument * doc)
{
	DEBUG_MSG("doc_buffer_changed_lcb()\n");
	doc_set_statusbar_lncol(doc);
}

static void
doc_view_toggle_overwrite_lcb(GtkTextView * view, Tdocument * doc)
{
	/* if there is a slave bview, toggle that too! */
	if (doc->slave) {
		if (view == (GtkTextView *)doc->view)
			gtk_text_view_set_overwrite(GTK_TEXT_VIEW(doc->slave), gtk_text_view_get_overwrite(GTK_TEXT_VIEW(view)));
		else
			gtk_text_view_set_overwrite(GTK_TEXT_VIEW(doc->view), gtk_text_view_get_overwrite(GTK_TEXT_VIEW(view)));
	}

	doc_set_statusbar_insovr(doc);
}

/**
 * update_encoding_meta_in_file:
 * @doc: a #Tdocument*
 * @encoding: #gchar*, The documents character encoding
 *
 * Update the HTML meta encoding tags for the supplied document.
 *
 * Return value: void
 **/
void
update_encoding_meta_in_file(Tdocument * doc, gchar * encoding)
{
	if (!encoding)
		return;
	GRegex *regex;
	GMatchInfo *match_info;
	gchar *type, *xhtmlend, *fulltext, *replacestring=NULL;
	gint so, eo, cso, ceo;
	/* first find if there is a meta encoding tag already */

	fulltext = doc_get_chars(doc, 0, -1);
	regex = g_regex_new("<meta[ \t\n]http-equiv[ \t\n]*=[ \t\n]*\"content-type\"[ \t\n]+content[ \t\n]*=[ \t\n]*\"([^;]*);[ \t\n]*charset=[a-z0-9-]*\"[ \t\n]*(/?)>", G_REGEX_MULTILINE|G_REGEX_CASELESS, 0, NULL);
	if (g_regex_match(regex, fulltext, 0, &match_info)) {
		DEBUG_MSG("we have a match, replace the encoding\n");
		if (g_match_info_get_match_count(match_info)>2) {
			type = g_match_info_fetch(match_info, 1);
			xhtmlend = g_match_info_fetch(match_info, 2);
		} else {
			type = g_strdup("text/html");
			xhtmlend = g_strdup(main_v->props.xhtml ? "/" : "");
		}
		replacestring =
			g_strconcat("<meta http-equiv=\"content-type\" content=\"", type, "; charset=", encoding,
						"\" ", xhtmlend, ">", NULL);
		g_free(type);
		g_free(xhtmlend);
	} else {
		DEBUG_MSG("no match, add the encoding\n");
		/* no <meta encoding tag yet */
		g_regex_unref(regex);
		g_match_info_free(match_info);
		regex = g_regex_new("<head>", G_REGEX_CASELESS, 0, NULL);
		g_regex_match(regex, fulltext, 0, &match_info);
		if (g_match_info_matches(match_info)) {
			replacestring = g_strconcat("<head>\n<meta http-equiv=\"content-type\" content=\"text/html; charset=", encoding,
						"\" ", (main_v->props.xhtml? "/>": ">"), NULL);
		}
	}
	if (replacestring) {
		g_match_info_fetch_pos(match_info, 0, &so, &eo);
		cso = utf8_byteoffset_to_charsoffset(fulltext,so);
		ceo = utf8_byteoffset_to_charsoffset(fulltext,eo);
		DEBUG_MSG("update_encoding_meta_in_file, update from %d to %d\n",cso,ceo);
		doc_replace_text(doc, replacestring, cso, ceo);
		g_free(replacestring);
		g_match_info_free(match_info);
	}
	g_regex_unref(regex);
	g_free(fulltext);
}

/*
returns a buffer in the encoding stored in doc->encoding, or NULL if that fails
and the user aborted conversion to UTF-8
*/
gchar *
doc_get_buffer_in_encoding(Tdocument * doc)
{
	GtkTextIter itstart, itend;
	gchar *buffer;

	gtk_text_buffer_get_bounds(doc->buffer, &itstart, &itend);
	buffer = gtk_text_buffer_get_text(doc->buffer, &itstart, &itend, TRUE);

	if (doc->encoding) {
		gchar *newbuf;
		gsize bytes_written = 0, bytes_read = 0;
		DEBUG_MSG("doc_get_buffer_in_encoding, converting from UTF-8 to %s\n", doc->encoding);
		newbuf = g_convert(buffer, -1, doc->encoding, "UTF-8", &bytes_read, &bytes_written, NULL);
		if (newbuf) {
			g_free(buffer);
			buffer = newbuf;
		} else {
			const gchar *buttons[] = { _("_Abort save"), _("_Continue save in UTF-8"), NULL };
			gint retval, line, column;
			glong position;
			gchar *tmpstr, failed[6];
			GtkTextIter iter;
			position = g_utf8_pointer_to_offset(buffer, buffer + bytes_read);
			gtk_text_buffer_get_iter_at_offset(doc->buffer, &iter, position);
			line = gtk_text_iter_get_line(&iter);
			column = gtk_text_iter_get_line_offset(&iter);
			failed[0] = '\0';
			g_utf8_strncpy(failed, buffer + bytes_read, 1);
			tmpstr =
				g_strdup_printf(_
								("Failed to convert %s to character encoding %s. Encoding failed on character '%s' at line %d column %d\n\nContinue saving in UTF-8 encoding?"),
								gtk_label_get_text(GTK_LABEL(doc->tab_menu)), doc->encoding, failed, line + 1,
								column + 1);
			retval =
				message_dialog_new_multi(BFWIN(doc->bfwin)->main_window, GTK_MESSAGE_WARNING, buttons,
										 _("File encoding conversion failure"), tmpstr);
			g_free(tmpstr);
			if (retval == 0) {
				DEBUG_MSG("doc_get_buffer_in_encoding, character set conversion failed, user aborted!\n");
				return NULL;
			} else {
				/* continue in UTF-8 */
				update_encoding_meta_in_file(doc, "UTF-8");
				g_free(buffer);
				gtk_text_buffer_get_bounds(doc->buffer, &itstart, &itend);
				buffer = gtk_text_buffer_get_text(doc->buffer, &itstart, &itend, TRUE);
			}
		}
	}
	return buffer;
}

static void
delete_backupfile_lcb(gpointer data)
{
	GFile *uri = (GFile *) data;
	g_object_unref(uri);
}

/**
 * doc_destroy:
 * @doc: a #Tdocument
 * @delay_activation: #gboolean whether to delay gui-updates.
 *
 * Performs all actions neccessary to remove an open document from the fish:
 * Adds filename to recent-list,
 * removes the document from the documentlist and notebook,
 * change notebook-focus (if !delay_activation),
 * delete backupfile if required by pref,
 * free all related memory.
 *
 * Return value: void
 **/

void
doc_destroy(Tdocument * doc, gboolean delay_activation)
{
	Tbfwin *bfwin = BFWIN(doc->bfwin);
	Tdocument *switch_to_doc=NULL;
	GSList *tmpslist;
	GList *tmplist;
	DEBUG_MSG("doc_destroy(%p,%d);\n", doc, delay_activation);

	tmplist = g_list_next(doc->recentpos);
	if (tmplist)
		switch_to_doc = tmplist->data;

	if (doc->status == DOC_STATUS_ERROR) {
		bfwin_docs_not_complete(doc->bfwin, FALSE);
	}

	bfwin_gotoline_search_bar_close(doc->bfwin);
	for (tmpslist=bfwin->doc_destroy;tmpslist;tmpslist=g_slist_next(tmpslist)) {
		Tcallback *cb = tmpslist->data;
		((DocDestroyCallback)cb->func)(doc, cb->data);
	}

	DEBUG_MSG("doc_destroy, calling bmark_clean_for_doc(%p)\n", doc);
	bmark_clean_for_doc(doc);
	if (doc->uri && bfwin->session) {	/* in a special situation the bfwin does not have a session: if a project window is closing */
		gchar *curi = g_file_get_uri(doc->uri);
		bfwin_recent_menu_add(doc->bfwin,FALSE, curi);
		g_free(curi);
	}
	bfwin_notebook_block_signals(BFWIN(doc->bfwin));
	if (doc->newdoc_autodetect_lang_id) {
		g_source_remove(doc->newdoc_autodetect_lang_id);
		doc->newdoc_autodetect_lang_id = 0;
	}

	if (doc->floatingview) {
		gtk_widget_destroy(FLOATINGVIEW(doc->floatingview)->window);
		doc->floatingview = NULL;
	}
	if (doc->slave) {
		gtk_widget_destroy(doc->slave);
		doc->slave = NULL;
	}

	/* now we remove the document from the document list */
	bfwin->documentlist = g_list_remove(bfwin->documentlist, doc);
	bfwin->recentdoclist = g_list_delete_link(bfwin->recentdoclist, doc->recentpos);
	doc->recentpos = NULL;
	DEBUG_MSG("removed %p from documentlist, list %p length=%d\n", doc, bfwin->documentlist,
			  g_list_length(bfwin->documentlist));
	if (bfwin->current_document == doc) {
		bfwin->current_document = NULL;
	}
	/* then we remove the page from the notebook */
	DEBUG_MSG("about to remove widget from notebook (doc=%p, current_document=%p)\n", doc,
			  bfwin->current_document);
	gtk_notebook_remove_page(GTK_NOTEBOOK(bfwin->notebook),
							 gtk_notebook_page_num(GTK_NOTEBOOK(bfwin->notebook), doc->vsplit));
	DEBUG_MSG("doc_destroy, removed widget from notebook (doc=%p), delay_activation=%d\n", doc,
			  delay_activation);
	DEBUG_MSG("doc_destroy, (doc=%p) about to bind notebook signals...\n", doc);
	bfwin_notebook_unblock_signals(BFWIN(doc->bfwin));
#ifdef IDENTSTORING
	bftextview2_identifier_hash_remove_doc(doc->bfwin, doc);
#endif
	if (!delay_activation) {
		gint newpage = -1;
		if (switch_to_doc)
			bfwin_switch_to_document_by_pointer(bfwin, switch_to_doc);
		bfwin_notebook_changed(BFWIN(doc->bfwin), newpage);
	}
	DEBUG_MSG("doc_destroy, (doc=%p) after calling notebook_changed(), vsplit=%p\n", doc, doc->vsplit);
	remove_autosave(doc);
	if (doc->uri) {
		if (main_v->props.backup_cleanuponclose) {
			gchar *tmp, *tmp2;
			GFile *backupuri;
			tmp = g_file_get_uri(doc->uri);
			tmp2 = g_strconcat(tmp, "~", NULL);
			backupuri = g_file_new_for_uri(tmp2);
			g_free(tmp);
			g_free(tmp2);
			file_delete_async(backupuri, FALSE, delete_backupfile_lcb, backupuri);
			g_object_unref(backupuri);
		}
		DEBUG_MSG("doc_destroy, unref doc->uri %p\n",doc->uri);
		doc_set_uri(doc, NULL, TRUE);
	}

	if (doc->encoding)
		g_free(doc->encoding);

	if (doc->fileinfo) {
		DEBUG_MSG("doc_destroy, unref doc->fileinfo %p\n",doc->fileinfo);
		g_object_unref(doc->fileinfo);
	}

	g_object_unref(doc->buffer);
	doc_unre_destroy(doc);
	DEBUG_MSG("doc_destroy, finished for %p\n", doc);
	g_free(doc);
}

/**
 * document_unset_filename:
 * @document: #Tdocument*
 *
 * this function is called if some other document is saved with a filename
 * equal to this files filename, or when this file is deleted in the filebrowser
 *
 * return value: void, ignored
 */
void
document_unset_filename(Tdocument * doc)
{
	if (doc->uri) {
		gchar *tmpstr;
		tmpstr = g_strconcat(_("Previously: "), gtk_label_get_text(GTK_LABEL(doc->tab_label)), NULL);
		/* doc_set_uri calls bmark_renamed which uses the tab_label for the name, so first set the tab label */
		doc_set_title(doc, tmpstr);
		doc_set_uri(doc, NULL, FALSE);

		if (doc->fileinfo) {
			g_object_unref(doc->fileinfo);
			doc->fileinfo = NULL;
		}

		doc_set_modified(doc, TRUE);
		g_free(tmpstr);
	}
}

/**
 * doc_close:
 * @doc: The #Tdocument to clase.
 * @warn_only: a #gint set to 1 if the document shouldn't actually be destroyed.
 *
 * Get confirmation when closing an unsaved file, save it if neccessary,
 * and destroy the file unless aborted by user.
 *
 * Return value: #gint set to 0 (when cancelled/aborted) or 1 (when closed or saved&closed)
 ** /
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

	if (doc_is_empty_non_modified_and_nameless(doc) && g_list_length(BFWIN(doc->bfwin)->documentlist) ==1) {
		/ * no need to close this doc, it's an Untitled empty document * /
		DEBUG_MSG("doc_close, 1 untitled empty non-modified document, returning\n");
		return 0;
	}

	if (doc->modified) {
		/ *if (doc->tab_label) {* /
			text = g_strdup_printf(_("Save changes to \"%s\" before closing?."),
									gtk_label_get_text (GTK_LABEL (doc->tab_label)));
		/ *} else {
			text = g_strdup(_("Save changes to this untitled file before closing?"));
		}* /

		{
			gchar *buttons[] = {_("Do_n't save"), GTK_STOCK_CANCEL, GTK_STOCK_SAVE, NULL};
			retval = multi_query_dialog(BFWIN(doc->bfwin)->main_window, text,
						_("If you don't save your changes they will be lost."), 2, 1, buttons);
		}
		g_free(text);

		switch (retval) {
		case 1:
			DEBUG_MSG("doc_close, retval=2 (cancel) , returning\n");
			return 2;
			break;
		case 2:
			doc_save(doc, FALSE, FALSE, FALSE);
			if (doc->modified == 1) {
				/ * something went wrong it's still not saved * /
				return 0;
			}
			if (!warn_only) {
				doc_destroy(doc, FALSE);
			}
			break;
		case 0:
			if (!warn_only) {
				doc_destroy(doc, FALSE);
			}
			break;
		default:
			return 0;			/ * something went wrong * /
			break;
		}
	} else {
		if (!warn_only) {
			DEBUG_MSG("doc_close, starting doc_destroy for doc=%p\n", doc);
			doc_destroy(doc, FALSE);
		}
	}
	DEBUG_MSG("doc_close, finished\n");
/ *	notebook_changed();* /
	return 1;
}*/

static void
doc_close_but_clicked_lcb(GtkWidget * wid, gpointer data)
{
	/*doc_close(data, 0); */
	doc_close_single_backend(data, FALSE, FALSE);
}

static gboolean
doc_scroll_event_lcb(GtkWidget * widget, GdkEventScroll * event, gpointer user_data)
{
	gint font_zoom = 0;
	if (event->state & GDK_CONTROL_MASK) {
		if (event->direction == GDK_SCROLL_UP) {
			font_zoom = 1;
		} else if (event->direction == GDK_SCROLL_DOWN) {
			font_zoom = -1;
		}
#if GTK_CHECK_VERSION(3,4,0)
		 else if (event->direction == GDK_SCROLL_SMOOTH) {
			/* Fixes bug #1020925 for mice that outputs
			smooth scroll events for standard scrolling..
		 	delta_y values for scroll wheels:
			-1 = up
			 1 = down*/
			if (event->delta_y < 0) {
				font_zoom = 1;
			} else if (event->delta_y > 0) {
				font_zoom = -1;
			}
		}
#endif /* GTK_CHECK_VERSION(3,4,0) */
	}
	if (font_zoom) {
		doc_font_size(DOCUMENT(user_data), font_zoom);
	}
	return (font_zoom != 0);
}

static gboolean
doc_view_button_press_lcb(GtkWidget * widget, GdkEventButton * bevent, Tdocument * doc)
{
	if (bevent->button == 2) {
		doc->in_paste_operation = TRUE;
		doc_unre_new_group(doc);
	}
	return FALSE;
}

static gboolean
doc_view_button_release_lcb(GtkWidget * widget, GdkEventButton * bevent, Tdocument * doc)
{
	if (bevent->button == 2) {
		doc->in_paste_operation = FALSE;
		doc_unre_new_group(doc);
	}
	return FALSE;
}

static gboolean
doc_focus_in_lcb(GtkWidget *widget,GdkEvent  *event, Tdocument *doc)
{
	bfwin_set_cutcopypaste_actions(doc->bfwin, TRUE);
	return FALSE;
}

static gboolean
doc_focus_out_lcb(GtkWidget *widget,GdkEvent  *event, Tdocument *doc)
{
	if (gtk_window_get_focus(GTK_WINDOW(BFWIN(doc->bfwin)->main_window))!=widget) {
		bfwin_set_cutcopypaste_actions(doc->bfwin, FALSE);
	}
	return FALSE;
}

#if GTK_CHECK_VERSION(3,0,0)
static void
doc_view_style_updated_lcb(GtkWidget *widget, gpointer user_data)
{
	Tdocument *doc =user_data;
	DEBUG_MSG("doc_view_style_updated_lcb\n");
	doc_set_tabsize(doc, BFWIN(doc->bfwin)->session->editor_tab_width);
}
#endif

static gint
calc_right_margin_pixels_to_border(Tdocument *doc)
{
	gint width, retval, borderleft;
	GdkWindow *gwin;

	if (!main_v->props.wrap_on_right_margin) {
		return main_v->props.adv_textview_right_margin;
	}
	gwin = gtk_text_view_get_window(GTK_TEXT_VIEW(doc->view), GTK_TEXT_WINDOW_TEXT);
	if (!gwin)
		return main_v->props.adv_textview_right_margin;
#if GTK_CHECK_VERSION(2,24,0)
	width = gdk_window_get_width(gwin);
#else
	gdk_drawable_get_size((GdkDrawable *)gwin, &width, &retval);
#endif
	borderleft = gtk_text_view_get_border_window_size(GTK_TEXT_VIEW(doc->view), GTK_TEXT_WINDOW_LEFT);
	retval = width - BLUEFISH_TEXT_VIEW(doc->view)->margin_pixels_per_char * main_v->props.right_margin_pos /*- borderleft*/;
	DEBUG_MSG("calc_right_margin_pixels_to_border, width=%d, borderleft=%d, margin*pixel=%d, retval=%d\n",width, borderleft, BLUEFISH_TEXT_VIEW(doc->view)->margin_pixels_per_char * main_v->props.right_margin_pos, retval);
	if (retval <= 0) {
		return 0;
	}
	return retval;
}

/* called from g_list_foreach() in bfwin.c on a window configure event */
void
doc_recalculate_right_margin(gpointer data,gpointer user_data)
{
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(DOCUMENT(data)->view), calc_right_margin_pixels_to_border(DOCUMENT(data)));
}


Tdocument *
doc_new_backend(Tbfwin * bfwin, gboolean force_new, gboolean readonly, gboolean init_fileinfo)
{
	GtkWidget *scroll;
	Tdocument *newdoc;
	GtkWidget *hbox, *button;
	GList *tmplist;

	/* test if the current document is empty and nameless, if so we return that */
	if (!force_new && bfwin->current_document && g_list_length(bfwin->documentlist) == 1
		&& doc_is_empty_non_modified_and_nameless(bfwin->current_document)) {
		newdoc = bfwin->current_document;
		DEBUG_MSG("doc_new_backend, returning existing doc %p\n", newdoc);
		return newdoc;
	}

	newdoc = g_new0(Tdocument, 1);
	DEBUG_MSG("doc_new_backend, main_v is at %p, bfwin at %p, newdoc at %p\n", main_v, bfwin, newdoc);
	newdoc->readonly = readonly;
	newdoc->bfwin = (gpointer) bfwin;
	newdoc->status = DOC_STATUS_COMPLETE;	/* if we don't set this default we will get problems for new empty files */
	newdoc->buffer = gtk_text_buffer_new(langmgr_get_tagtable());
	newdoc->view = bftextview2_new_with_buffer(newdoc->buffer);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(newdoc->view), main_v->props.adv_textview_left_margin);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(newdoc->view), calc_right_margin_pixels_to_border(newdoc));

	bluefish_text_view_multiset(BLUEFISH_TEXT_VIEW(newdoc->view), newdoc,
								BFWIN(bfwin)->session->view_line_numbers, BFWIN(bfwin)->session->view_blocks,
								BFWIN(bfwin)->session->autoindent, BFWIN(bfwin)->session->autocomplete,
								BFWIN(bfwin)->session->show_mbhl, FALSE);
	bluefish_text_view_set_show_right_margin(BLUEFISH_TEXT_VIEW(newdoc->view),
											 bfwin->session->display_right_margin);
	bluefish_text_view_set_show_visible_spacing(BLUEFISH_TEXT_VIEW(newdoc->view),
												bfwin->session->show_visible_spacing);
#ifdef HAVE_LIBENCHANT
	BLUEFISH_TEXT_VIEW(newdoc->view)->spell_check = BFWIN(bfwin)->session->spell_check_default;
#endif
	g_object_set(G_OBJECT(newdoc->view), "editable", !readonly, NULL);
	bluefish_text_view_select_language(BLUEFISH_TEXT_VIEW(newdoc->view), bfwin->session->default_mime_type,
									   NULL);
	gtk_widget_show(newdoc->view);
	if (init_fileinfo) {
		newdoc->fileinfo = g_file_info_new();
		g_file_info_set_content_type(newdoc->fileinfo, bfwin->session->default_mime_type);
	}
	scroll = gtk_scrolled_window_new(NULL, NULL);
	g_signal_connect(scroll, "scroll-event", G_CALLBACK(doc_scroll_event_lcb), newdoc);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scroll), newdoc->view);
	newdoc->highlightstate = bfwin->session->enable_syntax_scan;
	newdoc->tab_label = gtk_label_new(NULL);
	gtk_widget_set_can_focus(newdoc->tab_label, FALSE);
	if (!main_v->props.use_system_tab_font)
		apply_font_style(newdoc->tab_label, main_v->props.tab_font_string);
	newdoc->tab_menu = gtk_label_new(NULL);
	newdoc->tab_eventbox = gtk_event_box_new();
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(newdoc->tab_eventbox), FALSE);
	gtk_misc_set_alignment(GTK_MISC(newdoc->tab_menu), 0, 0);

	doc_unre_init(newdoc);
	apply_font_style(newdoc->view, main_v->props.editor_font_string);
	doc_set_wrap(newdoc, bfwin->session->wrap_text_default);

	/* we initialize already with 0 , so we don't need these:
	   newdoc->uri = NULL;
	   newdoc->modified = 0;
	   newdoc->fileinfo = NULL; */
	newdoc->encoding =
		g_strdup(main_v->props.newfile_default_encoding);
	DEBUG_MSG("doc_new_backend, encoding is %s\n", newdoc->encoding);

	doc_set_title(newdoc, NULL);

	g_signal_connect(G_OBJECT(newdoc->buffer), "insert-text", G_CALLBACK(doc_buffer_insert_text_lcb), newdoc);
	g_signal_connect(G_OBJECT(newdoc->buffer), "delete-range", G_CALLBACK(doc_buffer_delete_range_lcb), newdoc);
	g_signal_connect(G_OBJECT(newdoc->buffer), "changed", G_CALLBACK(doc_buffer_changed_lcb), newdoc);
	g_signal_connect(G_OBJECT(newdoc->buffer), "mark-set", G_CALLBACK(doc_buffer_mark_set_lcb), newdoc);
	g_signal_connect_after(G_OBJECT(newdoc->view), "toggle-overwrite",
					 G_CALLBACK(doc_view_toggle_overwrite_lcb), newdoc);
	g_signal_connect_after(G_OBJECT(newdoc->view), "populate-popup",
						   G_CALLBACK(doc_view_populate_popup_lcb), newdoc);
	g_signal_connect(G_OBJECT(newdoc->view), "button-release-event",
					 G_CALLBACK(doc_view_button_release_lcb), newdoc);
	g_signal_connect(G_OBJECT(newdoc->view), "button-press-event",
					 G_CALLBACK(doc_view_button_press_lcb), newdoc);
	g_signal_connect(G_OBJECT(newdoc->view), "focus-in-event",
					 G_CALLBACK(doc_focus_in_lcb), newdoc);
	g_signal_connect(G_OBJECT(newdoc->view), "focus-out-event",
					 G_CALLBACK(doc_focus_out_lcb), newdoc);
	bfwin->documentlist = g_list_append(bfwin->documentlist, newdoc);
	tmplist = g_list_last(bfwin->recentdoclist);
	tmplist = g_list_append(tmplist, newdoc);
	newdoc->recentpos = g_list_last(tmplist);
	if (!bfwin->recentdoclist)
		bfwin->recentdoclist = tmplist;
	/*g_print("doc_new_backend, doc=%p, recentpos=%p, recentdoclist=%p\n",newdoc, newdoc->recentpos, bfwin->recentdoclist);*/

	gtk_widget_show(newdoc->tab_label);
	gtk_widget_show(scroll);

	DEBUG_MSG("doc_new_backend, appending doc to notebook\n");

	hbox = gtk_hbox_new(FALSE, 4);
	button = bluefish_small_close_button_new();
	g_signal_connect(button, "clicked", G_CALLBACK(doc_close_but_clicked_lcb), newdoc);
	gtk_container_add(GTK_CONTAINER(newdoc->tab_eventbox), newdoc->tab_label);
	gtk_box_pack_start(GTK_BOX(hbox), newdoc->tab_eventbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show_all(hbox);

#if GTK_CHECK_VERSION(3,0,0)
	newdoc->vsplit = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
#else
	newdoc->vsplit = gtk_vpaned_new();
#endif
	gtk_paned_add1(GTK_PANED(newdoc->vsplit), scroll);
	gtk_widget_show(newdoc->vsplit);
	DEBUG_MSG("doc_new_backend, vsplit at %p\n", newdoc->vsplit);

	gtk_notebook_append_page_menu(GTK_NOTEBOOK(bfwin->notebook), newdoc->vsplit, hbox, newdoc->tab_menu);
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(bfwin->notebook), newdoc->vsplit, TRUE);
	/* for some reason it only works after the document is appended to the notebook */
	doc_set_tabsize(newdoc, BFWIN(bfwin)->session->editor_tab_width);
#if GTK_CHECK_VERSION(3,0,0)
	g_signal_connect(G_OBJECT(newdoc->view), "style-updated",
					 G_CALLBACK(doc_view_style_updated_lcb), newdoc);
#endif
	return newdoc;
}

/**
 * doc_new_loading_in_background:
 * @bfwin: the #Tbfwin* in which window to load this document
 *
 * creates a new document, which will be loaded in the background
 */
Tdocument *
doc_new_loading_in_background(Tbfwin * bfwin, GFile * uri, GFileInfo * finfo, gboolean readonly)
{
	Tdocument *doc = doc_new_backend(bfwin, FALSE, readonly, FALSE);
	DEBUG_MSG("doc_new_loading_in_background, bfwin=%p, doc=%p, for uri %p\n", bfwin, doc, uri);
	if (finfo) {
		doc->fileinfo = g_object_ref(finfo);
	} else {
		doc->fileinfo = NULL;
	}
	doc_set_uri(doc, uri, FALSE);
	doc_set_title(doc, NULL);
	doc_set_status(doc, DOC_STATUS_LOADING);
	bfwin_docs_not_complete(bfwin, TRUE);
	return doc;
}

static gboolean
doc_auto_detect_lang_lcb(gpointer data)
{
	Tdocument *doc = data;
	gchar *conttype, *buf;
#ifdef WIN32
	gchar *mimetype;
#endif
	gint buflen;
	gboolean uncertain = FALSE;
	DEBUG_MSG("doc_auto_detect_lang_lcb, started, doc=%p\n", doc);
	buf = doc_get_chars(doc, 0, -1);
	buflen = strlen(buf);
	conttype = g_content_type_guess(NULL, (guchar *) buf, buflen, &uncertain);
#ifdef WIN32
	mimetype = g_content_type_get_mime_type(conttype);
#endif
	DEBUG_MSG("doc_auto_detect_lang_lcb, buflen=%d\n",buflen);
	g_free(buf);
	if (!uncertain && conttype && (strcmp(conttype, "text/plain") != 0 || buflen > 50)) {
		DEBUG_MSG("doc_auto_detect_lang_lcb, found %s for certain\n", conttype);
#ifdef WIN32
		doc_set_mimetype(doc, mimetype, NULL);
		g_free(mimetype);
#else
		doc_set_mimetype(doc, conttype, NULL);
#endif
		g_free(conttype);
		return FALSE;
	}
	g_free(conttype);
	if (buflen > 50) {
		doc->newdoc_autodetect_lang_id = 0;
		DEBUG_MSG("doc_auto_detect_lang_lcb, filesize>50, stop detection\n");
		return FALSE;
	}
	return TRUE;
}

/**
 * doc_new:
 * @bfwin: #Tbfwin* with the window to open the document in
 * @delay_activate: Whether to perform GUI-calls and flush_queue(). Set to TRUE when loading several documents at once.
 *
 * Create a new document, related structures and a nice little textview to display the document in.
 * Finally, add a new tab to the notebook.
 * The GtkTextView is not actually gtk_widget_shown() if delay_activate == TRUE. This is done by doc_activate() instead.
 *
 * Return value: a #Tdocument* pointer to the just created document.
 **/
Tdocument *
doc_new(Tbfwin * bfwin, gboolean delay_activate)
{
	Tdocument *doc = doc_new_backend(bfwin, TRUE, FALSE, TRUE);
	doc_set_status(doc, DOC_STATUS_COMPLETE);
	DEBUG_MSG("doc_new, doc=%p, status=%d\n", doc, doc->status);

	doc->newdoc_autodetect_lang_id =
		g_timeout_add_seconds_full(G_PRIORITY_DEFAULT_IDLE, 10, doc_auto_detect_lang_lcb, doc, NULL);
	if (!delay_activate)
		gtk_widget_show(doc->view);	/* Delay _show() if neccessary */
	return doc;
}

Tdocument *
doc_new_with_template(Tbfwin * bfwin, GFile * uri, gboolean force_new)
{
	Tdocument *doc;
	if (!uri)
		return doc_new(bfwin, FALSE);
	doc = doc_new_backend(bfwin, force_new, FALSE, FALSE);
	file_into_doc(doc, uri, TRUE, FALSE);
	gtk_widget_show(doc->view);
	return doc;
}

/**
 * doc_new_with_new_file:
 * @bfwin: #Tbfwin*
 * @new_curi: #gchar* character uri to give document.
 *
 * Create a new document, name it by new_curi, and create the file.
 *
 * Return value: void
 **/
/*void doc_new_with_new_file(Tbfwin *bfwin, gchar *new_curi) {
	Tdocument *doc;
	Tfiletype *ft;
	GFile *new_uri;
	if (new_curi == NULL) {
		statusbar_message(bfwin,_("No filename"), 2);
		return;
	}
	new_uri = gnome_vfs_uri_new(new_curi);
	if (!main_v->props.allow_multi_instances) {
		gboolean res;
		res = switch_to_document_by_uri(bfwin,new_uri);
		if (res){
			return;
		}
	}
	DEBUG_MSG("doc_new_with_new_file, new_curi=%s\n", new_curi);
	add_filename_to_history(bfwin,new_curi);
	doc = doc_new(bfwin, FALSE);
	doc->uri = new_uri;
	if (bfwin->project && bfwin->project->template && strlen(bfwin->project->template) > 2) {
		GFile *uri;
		uri = gnome_vfs_uri_new(bfwin->project->template);
		if (uri) {
			file_into_doc(bfwin->current_document, uri, TRUE);
			g_object_unref(uri);
		}
 	}
	ft = get_filetype_by_filename_and_content(new_curi, NULL);
	if (ft) doc->hl = ft;
	/ * doc->modified = 1;* /
	doc_set_title(doc);
	/ * doc_save(doc, FALSE, FALSE, FALSE); * /
	/ * doc_set_stat_info(doc);  also sets mtime field * /
	switch_to_document_by_pointer(bfwin,doc);
	doc_activate(doc);
}*/

/**
 * doc_new_from_uri:
 *
 * uri should be set !
 * finfo may be NULL
 *
 * and goto_line and goto_offset should not BOTH be >= 0 (if so, offset is ignored)
 */
void
doc_new_from_uri(Tbfwin * bfwin, GFile * opturi, GFileInfo * finfo, gboolean delay_activate,
				 gboolean move_to_this_win, gint goto_line, gint goto_offset)
{
	GList *alldocs;
	Tdocument *tmpdoc;
	gchar *tmpcuri;
	GFile *uri;
	gboolean open_readonly = FALSE;
	if (!bfwin || !opturi) {
		return;
	}
	uri = opturi;
	tmpcuri = g_file_get_uri(opturi);
	DEBUG_MSG("doc_new_from_uri, started for uri(%p)=%s\n", uri, tmpcuri);

	/* check if the document already is opened */
	alldocs = return_allwindows_documentlist();
	tmpdoc = documentlist_return_document_from_uri(alldocs, uri);
	g_list_free(alldocs);
	if (tmpdoc) {				/* document is already open */
		DEBUG_MSG
			("doc_new_from_uri, doc %s is already open, delay_activate=%d, move_to_window=%d, goto_line=%d, goto_offset=%d\n",
			 tmpcuri, delay_activate, move_to_this_win, goto_line, goto_offset);
		if (tmpdoc->bfwin != bfwin && move_to_this_win) {
			/* we should aks the user if it is OK to move the document */
			if (!delay_activate)
				bfwin->focus_next_new_doc = TRUE;
			doc_move_to_window_dialog(tmpdoc, bfwin);
			/* TODO: or open the document readonly */
		} else if (!delay_activate) {	/* switch to window, only if we should */
			bfwin_switch_to_document_by_pointer(BFWIN(tmpdoc->bfwin), tmpdoc);
			if (bfwin != tmpdoc->bfwin)
				gtk_window_present(GTK_WINDOW(BFWIN(tmpdoc->bfwin)->main_window));
		}
		if (tmpdoc != NULL) {
			if (goto_line >= 0)
				doc_select_line(tmpdoc, goto_line, TRUE);
			else if (goto_offset >= 0)
				doc_select_line_by_offset(tmpdoc, goto_offset, TRUE);
		}
	} else {					/* document is not yet opened */
		if (!delay_activate)
			bfwin->focus_next_new_doc = TRUE;
		DEBUG_MSG("doc_new_from_uri, uri=%p, delay_activate=%d, focus_next_new_doc=%d\n", uri, delay_activate,
				  bfwin->focus_next_new_doc);
#if !GLIB_CHECK_VERSION(2, 18, 0)
		/* check runtime glib version, check if remote file, and give warning if remote file on glib < 2.18 */
		if (glib_major_version == 2 && glib_minor_version < 18 && !g_file_is_native(uri)) {
			gchar *message =
				g_strdup_printf(_
								("Your glib version (%d.%d.%d) is unreliable with remote files. Please upgrade to 2.18.0 or newer."),
glib_major_version, glib_minor_version, glib_micro_version);
			bfwin_statusbar_message(bfwin, message, 20);
			g_free(message);
		}
#endif
		file_doc_from_uri(bfwin, uri, NULL, finfo, goto_line, goto_offset, open_readonly);
	}
	session_set_opendir(bfwin, tmpcuri);
	g_free(tmpcuri);
}

void
doc_new_from_input(Tbfwin * bfwin, gchar * input, gboolean delay_activate, gboolean move_to_this_win,
				   gint goto_line)
{
	GFile *uri = NULL;
	if (!input) {
		return;
	}
	DEBUG_MSG("doc_new_from_input, input=%s, delay_activate=%d\n", input, delay_activate);
	if (strchr(input, '/') == NULL) {	/* no slashes in the path, relative ? */
		if (bfwin->current_document && bfwin->current_document->uri) {
			GFile *parent = g_file_get_parent(bfwin->current_document->uri);
			uri = g_file_resolve_relative_path(parent, input);
			g_object_unref(parent);
		} else {
			/* relative path to what ?!?!?! */
			/* home dir? current dir? */
		}
	} else {
		uri = g_file_new_for_commandline_arg(input);
	}
	if (uri) {
		doc_new_from_uri(bfwin, uri, NULL, delay_activate, move_to_this_win, goto_line, -1);
		g_object_unref(uri);
	}
}

static GtkWidget *
doc_create_slave_view(Tdocument * doc)
{
	GtkWidget *scroll;
	DEBUG_MSG("doc_create_slave_view, create slave view for %p\n", doc->view);
	doc->slave = bftextview2_new_slave(BLUEFISH_TEXT_VIEW(doc->view));
	gtk_text_view_set_overwrite(GTK_TEXT_VIEW(doc->slave), gtk_text_view_get_overwrite(GTK_TEXT_VIEW(doc->view)));
	g_signal_connect_after(G_OBJECT(doc->slave), "toggle-overwrite",
					 G_CALLBACK(doc_view_toggle_overwrite_lcb), doc);
	g_signal_connect_after(G_OBJECT(doc->slave), "populate-popup",
						   G_CALLBACK(doc_view_populate_popup_lcb), doc);
	g_signal_connect(G_OBJECT(doc->slave), "button-release-event",
					 G_CALLBACK(doc_view_button_release_lcb), doc);
	g_signal_connect(G_OBJECT(doc->slave), "button-press-event", G_CALLBACK(doc_view_button_press_lcb), doc);

	apply_font_style(doc->slave, main_v->props.editor_font_string);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	g_signal_connect(scroll, "scroll-event", G_CALLBACK(doc_scroll_event_lcb), doc);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scroll), doc->slave);
	doc_set_tabsize(doc, BFWIN(doc->bfwin)->session->editor_tab_width);
	g_signal_connect(G_OBJECT(doc->slave), "focus-in-event",
					 G_CALLBACK(doc_focus_in_lcb), doc);
	g_signal_connect(G_OBJECT(doc->slave), "focus-out-event",
					 G_CALLBACK(doc_focus_out_lcb), doc);
	return scroll;
}

static gboolean
doc_split_scroll(gpointer data)
{
	Tdocument *doc = data;
	GtkAdjustment *adjust =
		gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(gtk_widget_get_parent(doc->slave)));
	gtk_adjustment_set_value(adjust, gtk_adjustment_get_lower(adjust) + gtk_adjustment_get_page_size(adjust));
	return FALSE;
}

static void floatingview_destroy_lcb(GtkWidget * widget, Tdocument * doc);

void
doc_split_view(Tdocument * doc, gboolean enable)
{
	GtkWidget *botscrol;

	if (enable == (doc->slave != NULL))
		return;

	if (enable) {
		GdkRectangle rect;

		if (doc->floatingview)
			floatingview_destroy_lcb(NULL, doc);

		gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(doc->view), &rect);
		botscrol = doc_create_slave_view(doc);
		DEBUG_MSG("doc_split_view, add %p to pane2\n", botscrol);
		gtk_paned_pack2(GTK_PANED(doc->vsplit), botscrol, TRUE, FALSE);
		gtk_widget_show(doc->slave);
		gtk_widget_show(botscrol);
		gtk_paned_set_position(GTK_PANED(doc->vsplit), rect.height / 2);
		DEBUG_MSG("doc_split_view, rect.height=%d, set split at %d\n", rect.height, rect.height / 2);
		g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, doc_split_scroll, doc, NULL);
	} else {
		botscrol = gtk_widget_get_parent(doc->slave);
		DEBUG_MSG("doc_split_view, destroy %p\n", botscrol);
		gtk_widget_destroy(botscrol);
		doc->slave = NULL;
	}
	DEBUG_MSG("doc_split_view, done\n");
}

/**
 * doc_reload:
 * @doc: a #Tdocument
 * @finfo: if the reload is because the file is modified, pass here the new fileinfo, use NULL if unknown
 *
 * Revert to file on disk.
 *
 * Return value: void
 **/
void
doc_reload(Tdocument * doc, GFileInfo * newfinfo, gboolean warn_user)
{
	GtkTextIter itstart, itend, cursor;
	gint cursorpos = -1;

	DEBUG_MSG("starting reload for %p\n", doc);
	if (doc->uri == NULL) {
		bfwin_statusbar_message(BFWIN(doc->bfwin), _("Unable to open file"), 2);
		return;
	}

	if (warn_user) {
		gint retval;
		const gchar *buttons[] = { GTK_STOCK_CANCEL, GTK_STOCK_REVERT_TO_SAVED, NULL };
		gchar *msgstr, *basename;

		basename = g_file_get_basename(doc->uri);
		msgstr =
			g_strdup_printf(_
							("If you revert \"%s\" to your last saved copy, your current changes will be permanently lost."),
							basename);
		retval =
			message_dialog_new_multi(BFWIN(doc->bfwin)->main_window, GTK_MESSAGE_WARNING, buttons,
									 _("Revert changes to last saved copy?"), msgstr);

		g_free(basename);
		g_free(msgstr);

		if (retval == 0)
			return;
	}

	/* store all bookmark positions, reload them later */
	bmark_clean_for_doc(doc);
	bluefish_text_view_scan_cleanup(BLUEFISH_TEXT_VIEW(doc->view));
	gtk_text_buffer_get_iter_at_mark(doc->buffer, &cursor, gtk_text_buffer_get_insert(doc->buffer));
	cursorpos = gtk_text_iter_get_line(&cursor);
	gtk_text_buffer_get_bounds(doc->buffer, &itstart, &itend);
	gtk_text_buffer_delete(doc->buffer, &itstart, &itend);
	doc_set_status(doc, DOC_STATUS_LOADING);
	bfwin_docs_not_complete(doc->bfwin, TRUE);
	doc_set_modified(doc, FALSE);
	if (doc->fileinfo)
		g_object_unref(doc->fileinfo);
	doc->fileinfo = newfinfo;
	if (newfinfo)
		g_object_ref(doc->fileinfo);
	file_doc_fill_from_uri(doc, doc->uri, doc->fileinfo, cursorpos);
}

static gboolean
doc_close_from_activate(gpointer data)
{
	doc_close_single_backend(DOCUMENT(data), FALSE, FALSE);
	return FALSE;
}

/**
 * doc_activate:
 * @doc: a #Tdocument
 *
 * Perform actions neccessary when a document is focused. I.e. called from the notebook.
 *
 * Show textview, warn if the file on disk has been changed,
 * update line-numbers etc and highlighting.
 *
 * Return value: void
 **/
void
doc_activate(Tdocument * doc)
{
#ifdef DEBUG
	if (!doc) {
		DEBUG_MSG("doc_activate, doc=NULL!!! ABORTING!!\n");
		exit(44);
	}
#endif
	if (doc == NULL)
		return;
	DEBUG_MSG("doc_activate, started for %p\n",doc);
	if (doc == BFWIN(doc->bfwin)->last_activated_doc || doc->close_doc) {
		/* DO enable the scanner, because it is disabled in notebook_changed(), but if the last document is also the new document it needs to be re-enabled again */
		DEBUG_MSG("doc_activate, enable the scanner for doc %p\n", doc);
		BLUEFISH_TEXT_VIEW(doc->view)->enable_scanner = TRUE;
		DEBUG_MSG("doc_activate, not doing anything, doc=%p, last_avtivated_doc=%p, close_doc=%d\n", doc,
				  BFWIN(doc->bfwin)->last_activated_doc, doc->close_doc);
		return;
	}
	DEBUG_MSG("doc_activate for doc with view %p..\n", doc->view);
	if (doc->status == DOC_STATUS_ERROR) {
		const gchar *buttons[] =
			{ _("_Retry"), _("Retry _all failed"), _("_Close"), _("Close all _failed"), NULL };
		gchar *tmpstr;
		gint retval;
		DEBUG_MSG("doc_activate, DOC_STATUS_ERROR, retry???\n");
		tmpstr =
			g_strconcat(_("File "), gtk_label_get_text(GTK_LABEL(doc->tab_menu)), _(" failed to load."),
						NULL);
		retval =
			message_dialog_new_multi(BFWIN(doc->bfwin)->main_window, GTK_MESSAGE_WARNING, buttons,
									 _("File failed to load\n"), tmpstr);
		g_free(tmpstr);
		switch (retval) {
		case 0:
			file_doc_retry_uri(doc);
			break;
		case 1:{
				GList *tmplist;
				/* retry all failed documents */
				for (tmplist = g_list_first(BFWIN(doc->bfwin)->documentlist); tmplist != NULL;
					 tmplist = tmplist->next) {
					if (DOCUMENT(tmplist->data)->status == DOC_STATUS_ERROR)
						file_doc_retry_uri(DOCUMENT(tmplist->data));
				}
			}
			break;
		case 2:
			g_idle_add(doc_close_from_activate, doc);
			break;
		case 3:{
				GList *tmplist;
				/* retry all failed documents */
				for (tmplist = g_list_first(BFWIN(doc->bfwin)->documentlist); tmplist != NULL;
					 tmplist = tmplist->next) {
					if (DOCUMENT(tmplist->data)->status == DOC_STATUS_ERROR)
						g_idle_add(doc_close_from_activate, DOCUMENT(tmplist->data));
				}
			}
			break;
#ifdef DEVELOPMENT
		default:
			g_print("doc_activate, retval=%d does not exist\n", retval);
			exit(123);
			break;
#endif
		}
		DEBUG_MSG("doc_activate, returning\n");
		return;
	} else if (doc->status == DOC_STATUS_LOADING) {
		DEBUG_MSG("doc_activate, STILL LOADING! returning\n");
		return;
	} else {
		if (doc->highlightstate && !BLUEFISH_TEXT_VIEW(doc->view)->enable_scanner) {
			DEBUG_MSG("doc_activate, enable scanner for %p\n",doc);
			BLUEFISH_TEXT_VIEW(doc->view)->enable_scanner = TRUE;
			bftextview2_schedule_scanning(BLUEFISH_TEXT_VIEW(doc->view));
		}
		gtk_widget_show(doc->view);	/* This might be the first time this document is activated. */
	}
	BFWIN(doc->bfwin)->last_activated_doc = doc;
	if (BFWIN(doc->bfwin)->recentdoclist != doc->recentpos) {
		/* put this document on top of the recentlist */
		DEBUG_MSG("put this document %p with recentpos %p on top of the recentlist %p\n", doc,doc->recentpos,BFWIN(doc->bfwin)->recentdoclist);
		GList *dummy = g_list_remove_link(BFWIN(doc->bfwin)->recentdoclist, doc->recentpos);
		BFWIN(doc->bfwin)->recentdoclist = g_list_concat(doc->recentpos, BFWIN(doc->bfwin)->recentdoclist);
		DEBUG_MSG("recentlist now starts at %p\n", BFWIN(doc->bfwin)->recentdoclist);
	}

	doc_start_modified_check(doc);

	DEBUG_MSG("doc_activate, calling bfwin_set_document_menu_items()\n");
	bfwin_set_document_menu_items(doc);
	bfwin_set_title(BFWIN(doc->bfwin), doc, 0);
	doc_set_statusbar_lncol(doc);
	doc_set_statusbar_insovr(doc);
	doc_set_statusbar_lang_encoding(doc);
#ifdef MAC_INTEGRATION
/*	ige_mac_menu_sync(GTK_MENU_SHELL(BFWIN(doc->bfwin)->menubar));*/
	gtkosx_application_sync_menubar(g_object_new(GTKOSX_TYPE_APPLICATION, NULL));
#endif

/*	doc_scroll_to_cursor(doc);*/
	if (doc->uri) {
/*		gchar *dir1 = g_path_get_dirname(doc->uri);
		gchar *dir2 = ending_slash(dir1);
		if (dir2[0] == '/') {
			chdir(dir2);
		}*/
		if (BFWIN(doc->bfwin)->session->filebrowser_focus_follow) {
/*			DEBUG_MSG("doc_activate, call filebrowser_open_dir() for %s\n",dir2);
			filebrowser_open_dir(BFWIN(doc->bfwin),dir2);*/
			fb2_focus_document(BFWIN(doc->bfwin), doc);
		}
/*		g_free(dir1);
		g_free(dir2);*/
	}
	if (doc->highlightstate)
		BLUEFISH_TEXT_VIEW(doc->view)->enable_scanner = TRUE;
	DEBUG_MSG("doc_activate, doc=%p, about to grab focus\n", doc);
	gtk_widget_grab_focus(GTK_WIDGET(doc->view));

	DEBUG_MSG("doc_activate, doc=%p, finished\n", doc);
}

void
doc_force_activate(Tdocument * doc)
{
	BFWIN(doc->bfwin)->last_activated_doc = NULL;
	doc_activate(doc);
}

void
file_open_from_selection(Tbfwin * bfwin)
{
	gchar *string;
	GtkClipboard *cb;

	cb = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	string = gtk_clipboard_wait_for_text(cb);
	if (string) {
		DEBUG_MSG("file_open_from_selection, opening %s\n", string);
		doc_new_from_input(bfwin, string, FALSE, FALSE, -1);

		g_free(string);
	}
}

void
file_insert_doc(Tbfwin * bfwin)
{
	gchar *tmpfilename = NULL;

	{
		GtkWidget *dialog;
		dialog =
			file_chooser_dialog(bfwin, _("Select file to insert"), GTK_FILE_CHOOSER_ACTION_OPEN, NULL, FALSE,
								FALSE, NULL, TRUE);
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
			tmpfilename = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
		}
		/* BUG: get the encoding from the dialog !!! */
		gtk_widget_destroy(dialog);
	}
	if (tmpfilename == NULL) {
		bfwin_statusbar_message(bfwin, _("No file to insert"), 2);
		return;
	} else {
		GFile *uri;

		doc_unre_new_group(bfwin->current_document);
		uri = g_file_new_for_uri(tmpfilename);
		file_into_doc(bfwin->current_document, uri, FALSE, FALSE);
		g_object_unref(uri);
		g_free(tmpfilename);
	}
}

void
doc_copy(Tbfwin * bfwin)
{
	gtk_text_buffer_copy_clipboard(bfwin->current_document->buffer,
								   gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
}

void
doc_cut(Tbfwin * bfwin)
{
	doc_unre_new_group(bfwin->current_document);
	gtk_text_buffer_cut_clipboard(bfwin->current_document->buffer, gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
								  TRUE);
	doc_unre_new_group(bfwin->current_document);
}

void
doc_paste(Tbfwin * bfwin)
{
	GtkTextMark *mark;
	Tdocument *doc = bfwin->current_document;

	DEBUG_MSG("edit_paste_cb, create new undo group\n");
	doc_unre_new_group(doc);
	doc->in_paste_operation = TRUE;
	DEBUG_MSG("edit_paste_cb, pasting clipboard\n");
	gtk_text_buffer_paste_clipboard(doc->buffer, gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), NULL, TRUE);
	doc->in_paste_operation = FALSE;
	DEBUG_MSG("edit_paste_cb, finished, create new undo group\n");
	doc_unre_new_group(doc);
	mark = gtk_text_buffer_get_insert(bfwin->current_document->buffer);
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(bfwin->current_document->view), mark);
	DEBUG_MSG("edit_paste_cb, finished\n");
}

/*************************** paste special code ***************************/

static TcheckNsave_return
paste_image_save_lcb(TcheckNsave_status status, GError * gerror, gpointer callback_data)
{
	/* TODO: handle error */
	if (gerror) {
		g_warning("paste_image_save_lcb, failed to save thumbnail: %s\n",gerror->message);
	}
	return CHECKNSAVE_CONT;
}

static void
image_received(GtkClipboard *clipboard,GdkPixbuf *pixbuf,gpointer data)
{
	DEBUG_MSG("image_received, started, got %p\n", pixbuf);
	if (!pixbuf) {
		g_print("no pixbuf received\n");
		return;
	}
	/* ask for the filename */
	gchar * filename, *insertname=NULL, *str;
	gint width, height;
	GFile *uri;
	gchar *buffer;
	gsize buflen;
	GError *gerror=NULL;
	Trefcpointer *refbuf;

	width = gdk_pixbuf_get_width(pixbuf);
	height = gdk_pixbuf_get_height(pixbuf);

	filename = ask_new_filename(BFWIN(data), NULL, _("Save pasted image as"));

	if (!filename)
		return;

	uri = g_file_new_for_uri(filename);
	gdk_pixbuf_save_to_buffer(pixbuf, &buffer, &buflen, "jpeg", &gerror,"quality", "95", NULL);
	refbuf = refcpointer_new(buffer);
	/* save the file and insert the image tag */
	file_checkNsave_uri_async(uri, NULL, refbuf, buflen, FALSE, FALSE,(CheckNsaveAsyncCallback) paste_image_save_lcb, NULL);


	if (BFWIN(data)->current_document->uri) {
		gchar *curi = g_file_get_uri(BFWIN(data)->current_document->uri);
		insertname = create_relative_link_to(curi, filename);
		g_free(curi);
	}
	str = g_strdup_printf("<img src=\"%s\" alt=\"\" width=\"%d\" height=\"%d\"/>", insertname? insertname : filename, width, height);

	doc_insert_two_strings(DOCUMENT(BFWIN(data)->current_document), str, NULL);

	refcpointer_unref(refbuf);
	g_object_unref(uri);
	g_free(filename);
	g_free(insertname);
	g_free(str);
}

static void
html_received(GtkClipboard *clipboard,GtkSelectionData *seldat,gpointer data)
{
	if (!gtk_selection_data_get_data(seldat)) {
		g_print("html_received, no text received\n");
		return;
	}
	/* strip headers and footer from the html*/
	GRegex *reg;
	GError *gerror=NULL;
	GMatchInfo *match_info;
	DEBUG_MSG("got %d bytes of data\n",gtk_selection_data_get_length(seldat));
	DEBUG_MSG("got data '%s'\n", gtk_selection_data_get_data(seldat));

	reg = g_regex_new("<body[^>]*>(.*)</body>",G_REGEX_CASELESS|G_REGEX_MULTILINE|G_REGEX_DOTALL,0,&gerror);
	if (!reg) {
		g_warning("paste special, html_received, internal regex error\n");
	}
	if (g_regex_match(reg,(gchar *)gtk_selection_data_get_data(seldat),0,&match_info)) {
		gchar *str;
		str = g_match_info_fetch(match_info,1);
		doc_insert_two_strings(DOCUMENT(BFWIN(data)->current_document), str, NULL);
		g_free(str);
	}
	g_match_info_free(match_info);
	g_regex_unref(reg);
}

static void
text_received(GtkClipboard *clipboard,const gchar *text,gpointer data)
{
	if (!text) {
		return;
	}
	doc_insert_two_strings(DOCUMENT(BFWIN(data)->current_document), text, NULL);
}

void
doc_paste_special(Tbfwin *bfwin)
{
	gint result;
	GtkWidget *win, *content_area, *rbut0=NULL, *rbut1=NULL, *rbut2=NULL, *rbut3=NULL;
	gboolean have_html=FALSE, have_image=FALSE, have_plain=FALSE;
	GSList *rgroup=NULL;
	GdkAtom *targets;
	gint numtargets;
	GtkClipboard *cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

	if (!gtk_clipboard_wait_for_targets(cb, &targets, &numtargets)) {
		DEBUG_MSG("no clipbord data available\n");
		return;
	}

	while (numtargets > 0 && (have_html==FALSE || have_image==FALSE)) {
		gchar *name;
		numtargets--;
		name = gdk_atom_name(targets[numtargets]);
		DEBUG_MSG("%d: got target %s\n", numtargets, name);
		if (strcmp(name, "text/html")==0) {
			have_html=TRUE;
		} else if (strncmp(name, "image/", 6)==0) {
			have_image=TRUE;
		} else if (strncmp(name, "text/plain",10)==0 || strcmp(name, "STRING")==0 || strcmp(name, "TEXT")==0) {
			have_plain = TRUE;
		}
		g_free(name);
	}

	win = gtk_dialog_new_with_buttons(_("Paste special")
				, GTK_WINDOW(bfwin->main_window), GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_MODAL
				, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT
				,GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,NULL);
	content_area = gtk_dialog_get_content_area(GTK_DIALOG(win));
	if (!have_html && !have_image && !have_plain) {
		gtk_box_pack_start(GTK_BOX(content_area), gtk_label_new(_("No compatible clipboard data found")), TRUE, TRUE, 4);
	}

	if (have_html) {
		rbut0 = gtk_radio_button_new_with_mnemonic(NULL, _("Paste as _HTML"));
		gtk_box_pack_start(GTK_BOX(content_area), rbut0, TRUE, TRUE, 4);
		rgroup = gtk_radio_button_get_group(GTK_RADIO_BUTTON(rbut0));
	}
	if (have_image) {
		rbut1 = gtk_radio_button_new_with_mnemonic(rgroup, _("Paste as HTML with _JPG"));
		gtk_box_pack_start(GTK_BOX(content_area), rbut1, TRUE, TRUE, 4);
		/*rbut2 = gtk_radio_button_new_with_mnemonic(rgroup, _("Paste as HTML _PNG"));
		gtk_box_pack_start(GTK_BOX(content_area), rbut2, TRUE, TRUE, 4);*/
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rbut1), TRUE);
		if (!rgroup) {
			rgroup = gtk_radio_button_get_group(GTK_RADIO_BUTTON(rbut1));
		}
	}
	if (have_plain) {
		rbut3 = gtk_radio_button_new_with_mnemonic(rgroup, _("Paste as plain text"));
		gtk_box_pack_start(GTK_BOX(content_area), rbut3, TRUE, TRUE, 4);
	}
	gtk_widget_show_all(win);
	result = gtk_dialog_run(GTK_DIALOG(win));
	DEBUG_MSG("gtk_dialog_run, got result %d\n", result);
	if (result == GTK_RESPONSE_ACCEPT) {
		GtkClipboard *cb;
		cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

		if (have_html && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rbut0))) {
			GdkAtom target;
			target = gdk_atom_intern_static_string("text/html");
			gtk_clipboard_request_contents(cb,target,html_received,bfwin);
		} else if (have_plain && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rbut3))){
			gtk_clipboard_request_text(cb,text_received,bfwin);
		} else if (have_image) {
			gtk_clipboard_request_image(cb,image_received,bfwin);
			/*paste_special_image(bfwin, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rbut1)));*/
		}
	}
	DEBUG_MSG("destroy dialog %p\n", win);
	gtk_widget_destroy(win);
	g_free(targets);
	DEBUG_MSG("destroy dialog %p, done\n", win);
}
/*************************** end of paste special code ***************************/

void
doc_select_all(Tbfwin * bfwin)
{
	GtkTextIter itstart, itend;

	gtk_text_buffer_get_bounds(bfwin->current_document->buffer, &itstart, &itend);
	gtk_text_buffer_move_mark_by_name(bfwin->current_document->buffer, "insert", &itstart);
	gtk_text_buffer_move_mark_by_name(bfwin->current_document->buffer, "selection_bound", &itend);
}

void
doc_toggle_highlighting(Tbfwin * bfwin, gboolean active)
{
	bfwin->current_document->highlightstate = active;
	DEBUG_MSG("doc_toggle_highlighting_cb, set enable_scanner=%d\n", bfwin->current_document->highlightstate);
	BLUEFISH_TEXT_VIEW(bfwin->current_document->view)->enable_scanner =
		bfwin->current_document->highlightstate;
	if (active) {
		bluefish_text_view_rescan(BLUEFISH_TEXT_VIEW(bfwin->current_document->view));
	} else {
		bluefish_text_view_scan_cleanup(BLUEFISH_TEXT_VIEW(bfwin->current_document->view));
	}
}

/**
 * all_documents_apply_settings:
 *
 * applies changes from the preferences to all documents
 *
 * Return value: void
 */
void
all_documents_apply_settings()
{
	GList *tmplist = g_list_first(return_allwindows_documentlist());
	PangoFontDescription *font_desc;
	font_desc = pango_font_description_from_string(main_v->props.editor_font_string);
	while (tmplist) {
		Tdocument *doc = tmplist->data;
		const gchar *tmpstr;
		bluefish_text_view_set_font(BLUEFISH_TEXT_VIEW(doc->view), font_desc);
		bluefish_text_view_set_colors(BLUEFISH_TEXT_VIEW(doc->view), main_v->props.btv_color_str);
		gtk_text_view_set_right_margin(GTK_TEXT_VIEW(DOCUMENT(doc)->view), calc_right_margin_pixels_to_border(DOCUMENT(doc)));
		if (gtk_text_view_get_wrap_mode(GTK_TEXT_VIEW(doc->view))!=GTK_WRAP_NONE) {
			doc_set_wrap(doc, TRUE);
		}
		tmpstr = gtk_label_get_text(GTK_LABEL(doc->tab_label));
		tab_label_set_string(doc, tmpstr);
		tmplist = g_list_next(tmplist);
	}
	pango_font_description_free(font_desc);
}

void
doc_word_count(Tbfwin * bfwin)
{
	gint start = 0, end = -1;
	gboolean has_selection;
	guint chars = 0, lines = 0, words = 0;
	gchar *allchars, *wc_message, *tmp1, *tmp2, *tmp3;

	has_selection = doc_get_selection(CURDOC(bfwin), &start, &end);
	allchars = doc_get_chars(bfwin->current_document, start, end);

	wordcount(allchars, &chars, &lines, &words);
	g_free(allchars);
	tmp1 = g_strdup_printf(ngettext("%d line", "%d lines", lines), lines);
	tmp2 = g_strdup_printf(ngettext("%d word", "%d words", words), words);
	tmp3 = g_strdup_printf(ngettext("%d character", "%d characters", chars), chars);
	wc_message =
		g_strconcat(has_selection ? _("Selection statistics: ") : _("Statistics: "), tmp1, ", ", tmp2, ", ",
					tmp3, NULL);
	bfwin_statusbar_message(bfwin, wc_message, 7);
	g_free(wc_message);
	g_free(tmp1);
	g_free(tmp2);
	g_free(tmp3);
}

/**
 * doc_indent_selection:
 * @doc: a #Tdocument*
 * @unindent: #gboolean
 *
 * Indent the selected block in current document.
 * Set unindent to TRUE to unindent.
 *
 * Return value: void
 **/
void
doc_indent_selection(Tdocument * doc, gboolean unindent)
{
	GtkTextIter itstart, itend;
	if (gtk_text_buffer_get_selection_bounds(doc->buffer, &itstart, &itend)) {
		GtkTextMark *end;
/*		gboolean firstrun=TRUE;*/

		doc_block_undo_reg(doc);
		doc_unre_new_group(doc);
		/* we have a selection, now we loop trough the characters, and for every newline
		   we add or remove a tab, we set the end with a mark */
		end = gtk_text_buffer_create_mark(doc->buffer, NULL, &itend, TRUE);
		if (gtk_text_iter_get_line_offset(&itstart) > 0) {
			gtk_text_iter_set_line_index(&itstart, 0);
		}
		while (gtk_text_iter_compare(&itstart, &itend) < 0) {
			GtkTextMark *cur;
/*			if (firstrun && !gtk_text_iter_starts_line(&itstart)) {
				gtk_text_iter_forward_line(&itstart);
			}
			firstrun = FALSE;*/
			cur = gtk_text_buffer_create_mark(doc->buffer, NULL, &itstart, TRUE);
			if (unindent) {
				/* when unindenting we try to set itend to the end of the indenting step
				   which might be a tab or 'tabsize' spaces, then we delete that part */
				gboolean cont = TRUE;
				gchar *buf = NULL;
				gunichar cchar = gtk_text_iter_get_char(&itstart);
				if (cchar == 9) {	/* 9 is ascii for tab */
					itend = itstart;
					cont = gtk_text_iter_forward_char(&itend);
					buf = g_strdup("\t");
				} else if (cchar == 32) {	/* 32 is ascii for space */
					gint i = 0;
					itend = itstart;
					gtk_text_iter_forward_chars(&itend, BFWIN(doc->bfwin)->session->editor_tab_width);
					buf = gtk_text_buffer_get_text(doc->buffer, &itstart, &itend, TRUE);
					DEBUG_MSG("tab_width=%d, strlen(buf)=%zd, buf='%s'\n",
							  BFWIN(doc->bfwin)->session->editor_tab_width, strlen(buf), buf);
					while (cont && buf[i] != '\0') {
						cont = (buf[i] == ' ');
						DEBUG_MSG("doc_indent_selection, buf[%d]='%c'\n", i, buf[i]);
						i++;
					}
					if (!cont) {
						g_free(buf);
					}
				} else {
					cont = FALSE;
				}
				if (cont) {
					gint offsetstart, offsetend;
					offsetstart = gtk_text_iter_get_offset(&itstart);
					offsetend = gtk_text_iter_get_offset(&itend);
					gtk_text_buffer_delete(doc->buffer, &itstart, &itend);
					doc_unre_add(doc, buf, offsetstart, offsetend, UndoDelete);
					g_free(buf);
				}
#ifdef DEBUG
				else {
					DEBUG_MSG("doc_indent_selection, NOT continue!!\n");
				}
#endif
			} else {			/* indent */
				gint offsetstart = gtk_text_iter_get_offset(&itstart);
				gchar *indentstring;
				gint indentlen;
				if (BFWIN(doc->bfwin)->session->editor_indent_wspaces) {
					indentstring = bf_str_repeat(" ", BFWIN(doc->bfwin)->session->editor_tab_width);
					indentlen = BFWIN(doc->bfwin)->session->editor_tab_width;
				} else {
					indentstring = g_strdup("\t");
					indentlen = 1;
				}
				gtk_text_buffer_insert(doc->buffer, &itstart, indentstring, indentlen);
				doc_unre_add(doc, indentstring, offsetstart, offsetstart + indentlen, UndoInsert);
				g_free(indentstring);
			}
			gtk_text_buffer_get_iter_at_mark(doc->buffer, &itstart, cur);
			gtk_text_buffer_get_iter_at_mark(doc->buffer, &itend, end);
			gtk_text_buffer_delete_mark(doc->buffer, cur);
			gtk_text_iter_forward_line(&itstart);
			DEBUG_MSG("doc_indent_selection, itstart at %d, itend at %d\n",
					  gtk_text_iter_get_offset(&itstart), gtk_text_iter_get_offset(&itend));
		}
		gtk_text_buffer_delete_mark(doc->buffer, end);
		doc_unblock_undo_reg(doc);
		doc_set_modified(doc, 1);
	} else {
		/* there is no selection, work on the current line */
		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_mark(doc->buffer, &iter, gtk_text_buffer_get_insert(doc->buffer));
		gtk_text_iter_set_line_offset(&iter, 0);
		if (unindent) {
			gint deletelen = 0;
			gchar *tmpstr, *tmp2str;
			GtkTextIter itend = iter;
			gtk_text_iter_forward_chars(&itend, BFWIN(doc->bfwin)->session->editor_tab_width);
			tmpstr = gtk_text_buffer_get_text(doc->buffer, &iter, &itend, TRUE);
			tmp2str = bf_str_repeat(" ", BFWIN(doc->bfwin)->session->editor_tab_width);
			if (tmpstr[0] == '\t') {
				deletelen = 1;
			} else if (tmpstr && strncmp(tmpstr, tmp2str, BFWIN(doc->bfwin)->session->editor_tab_width) == 0) {
				deletelen = BFWIN(doc->bfwin)->session->editor_tab_width;
			}
			g_free(tmpstr);
			g_free(tmp2str);
			if (deletelen) {
				itend = iter;
				gtk_text_iter_forward_chars(&itend, deletelen);
				gtk_text_buffer_delete(doc->buffer, &iter, &itend);
			}
		} else {				/* indent */
			gchar *indentstring;
			gint indentlen;
			if (BFWIN(doc->bfwin)->session->editor_indent_wspaces) {
				indentstring = bf_str_repeat(" ", BFWIN(doc->bfwin)->session->editor_tab_width);
				indentlen = BFWIN(doc->bfwin)->session->editor_tab_width;
			} else {
				indentstring = g_strdup("\t");
				indentlen = 1;
			}
			gtk_text_buffer_insert(doc->buffer, &iter, indentstring, indentlen);
			g_free(indentstring);
		}
	}
}

/**
 * list_relative_document_filenames:
 * @curdoc: #Tdocument: the current document
 *
 * this function will generate a stringlist with a relative links to
 * all other open documents. This list should be freed using free_stringlist()
 *
 * Return value: #GList with strings
 */
GList *
list_relative_document_filenames(Tdocument * curdoc)
{
	GList *tmplist, *retlist = NULL;
	gchar *curi;
	if (curdoc->uri == NULL) {
		return NULL;
	}
	curi = g_file_get_uri(curdoc->uri);

	tmplist = g_list_first(BFWIN(curdoc->bfwin)->documentlist);
	while (tmplist) {
		Tdocument *tmpdoc = tmplist->data;
		if (tmpdoc != curdoc && tmpdoc->uri != NULL) {
			gchar *tmp = g_file_get_uri(tmpdoc->uri);
			retlist = g_list_prepend(retlist, create_relative_link_to(curi, tmp));
			g_free(tmp);
		}
		tmplist = g_list_next(tmplist);
	}
	g_free(curi);
	return retlist;
}

static gchar *
doc_text_under_cursor(Tdocument * doc, gint * context)
{
	GtkTextIter iter;
/*	GSList *taglist, *tmplist; */
	gchar *retval = NULL;
	gint len;
/*	GtkTextIter so,eo; */
	gtk_text_buffer_get_iter_at_mark(doc->buffer, &iter, gtk_text_buffer_get_insert(doc->buffer));

/*	taglist = gtk_text_iter_get_tags(&iter);
	for (tmplist=taglist;tmplist;tmplist=tmplist->next) {
		GtkTextTag *tag=tmplist->data;
		/ * avoid tags like needscanning, folded, blockheader and such * /
		if (!langmgr_in_highlight_tags(tag))
			continue;
		so=eo=iter;
		if (!gtk_text_iter_begins_tag(&so, tag))
			gtk_text_iter_backward_to_tag_toggle(&so, tag);
		if (!gtk_text_iter_ends_tag(&eo, tag))
			gtk_text_iter_forward_to_tag_toggle(&eo, tag);
		/ * use the smallest string * /
		if (retval && g_utf8_strlen(retval,-1) > (gtk_text_iter_get_offset(&eo)-gtk_text_iter_get_offset(&so))) {
			g_free(retval);
			retval=NULL;
		}
		if (!retval)
			retval = gtk_text_buffer_get_text(doc->buffer, &so,&eo,TRUE);
	}*/

	if (!retval)
		retval = bf_get_identifier_at_iter(BLUEFISH_TEXT_VIEW(doc->view), &iter, context);

	if (!retval)
		return NULL;

	/* remove any surrounding quotes */
	len = strlen(retval);
	if (retval[0] == '"' && retval[len - 1] == '"') {
		memmove(retval, retval + 1, len - 2);
		retval[len - 2] = '\0';
	} else if (retval[0] == '\'' && retval[len - 1] == '\'') {
		memmove(retval, retval + 1, len - 2);
		retval[len - 2] = '\0';
	}
	return retval;
}

typedef struct {
	GFile *uri;
	Tdocument *doc;
} Tjumpcheckfile;

static void
doc_jump_query_exists_lcb(GObject * source_object, GAsyncResult * res, gpointer user_data)
{
	GFileInfo *finfo;
	Tjumpcheckfile *jcf = user_data;
	GError *gerror = NULL;

	finfo = g_file_query_info_finish(jcf->uri, res, &gerror);
	if (gerror) {
		DEBUG_MSG("%s\n", gerror->message);
		g_error_free(gerror);
	}
	if (finfo) {
		doc_new_from_uri(jcf->doc->bfwin, jcf->uri, finfo, FALSE, FALSE, -1, -1);
		g_object_unref(finfo);
	}
	g_object_unref(jcf->uri);
	g_slice_free(Tjumpcheckfile, jcf);
}

static void
doc_jump_check_file(Tdocument * doc, const gchar * filename)
{
	Tjumpcheckfile *jcf;
	jcf = g_slice_new(Tjumpcheckfile);
	jcf->doc = doc;
	if (!doc->uri || (filename[0] == '/' || strncmp(filename, "file://", 7) == 0)) {
		jcf->uri = g_file_new_for_commandline_arg(filename);
	} else {
		gchar *tmp;
		GFile *parent = g_file_get_parent(doc->uri);
		tmp = g_uri_unescape_string(filename, NULL);
		jcf->uri = g_file_resolve_relative_path(parent, tmp);
		g_free(tmp);
		g_object_unref(parent);
	}
	/* as for BF_FILEINFO, if the file is not yet open, we can re-use the finfo for the to open document */
	g_file_query_info_async(jcf->uri, BF_FILEINFO, 0, G_PRIORITY_HIGH, NULL, doc_jump_query_exists_lcb, jcf);
}

void
doc_jump(Tdocument * doc)
{
	gchar *string;
	gint context = -1;
	/* see what's under the cursor */
	string = doc_text_under_cursor(doc, &context);
	if (!string)
		return;
	DEBUG_MSG("doc_jump, got string %s\n", string);
	/* check if this is an existing file */
	doc_jump_check_file(doc, string);

#ifdef IDENTSTORING
	DEBUG_MSG("context=%d\n", context);
	if (context != -1) {
		Tjumpdata *ijd =
			bftextview2_lookup_identifier(doc->bfwin, BLUEFISH_TEXT_VIEW(doc->view), context, string);
		if (ijd) {
			DEBUG_MSG("got doc=%p (index=%d), line=%d\n", ijd->doc,
					  g_list_index(BFWIN(doc->bfwin)->documentlist, ijd->doc), ijd->line);
			if (ijd->doc && g_list_index(BFWIN(doc->bfwin)->documentlist, ijd->doc) != -1) {
				DEBUG_MSG("jump! doc=%p, line=%d\n", ijd->doc, ijd->line);
				bfwin_switch_to_document_by_pointer(doc->bfwin, ijd->doc);
				doc_select_line(ijd->doc, ijd->line, TRUE);
			}
		}
	}
#endif
	g_free(string);
}

void
doc_jump_matching_block_boundary(Tdocument *doc)
{
	GtkTextIter it1, it2, it3, it4, location;
	guint offset;
	gpointer haveblock;
	gtk_text_buffer_get_iter_at_mark(doc->buffer, &location, gtk_text_buffer_get_insert(doc->buffer));
	offset = gtk_text_iter_get_offset(&location);
	haveblock = bftextview2_get_block_at_boundary_location(BLUEFISH_TEXT_VIEW(doc->view), offset, &it1, &it2, &it3, &it4);
	if (!haveblock)
		return;
	if (gtk_text_iter_equal(&location, &it1)) {
		gtk_text_buffer_place_cursor(doc->buffer, &it4);
	} else if (gtk_text_iter_equal(&location, &it2)) {
		gtk_text_buffer_place_cursor(doc->buffer, &it3);
	} else if (gtk_text_iter_equal(&location, &it3)) {
		gtk_text_buffer_place_cursor(doc->buffer, &it2);
	} else if (gtk_text_iter_equal(&location, &it4)) {
		gtk_text_buffer_place_cursor(doc->buffer, &it1);
	}
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(doc->view),gtk_text_buffer_get_insert(doc->buffer));
}

static void
floatingview_destroy_lcb(GtkWidget * widget, Tdocument * doc)
{
	DEBUG_MSG("floatingview_destroy_lcb, called for doc=%p, doc->floatingview=%p\n", doc, doc->floatingview);
	if (doc->floatingview) {
		gtk_widget_destroy(FLOATINGVIEW(doc->floatingview)->window);
		g_free(doc->floatingview);
		doc->floatingview = NULL;
	}
}

void
doc_floating_view_new(Tbfwin * bfwin)
{
	Tdocument *doc = bfwin->current_document;
	Tfloatingview *fv;
	gchar *title;
	GtkWidget *scrolwin;

	if (doc->slave)
		doc_split_view(doc, FALSE);

	if (doc->floatingview) {
		fv = FLOATINGVIEW(doc->floatingview);
		gtk_window_present(GTK_WINDOW(fv->window));
		return;
	}
	fv = g_new(Tfloatingview, 1);
	doc->floatingview = fv;
	DEBUG_MSG("new_floatingview for doc=%p is at %p\n", doc, doc->floatingview);
	title = (doc->uri) ? g_file_get_uri(doc->uri) : g_strdup("Untitled");
	fv->window =
		window_full2(title, GTK_WIN_POS_NONE, 5, G_CALLBACK(floatingview_destroy_lcb), doc, TRUE, NULL);
	g_free(title);
	gtk_window_set_role(GTK_WINDOW(fv->window), "floatingview");
	fv->textview = bftextview2_new_slave(BLUEFISH_TEXT_VIEW(doc->view));
	apply_font_style(fv->textview, main_v->props.editor_font_string);
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), fv->textview);
	gtk_container_add(GTK_CONTAINER(fv->window), scrolwin);
	gtk_window_set_default_size(GTK_WINDOW(fv->window), 600, 600);
	gtk_widget_show_all(fv->window);
}
