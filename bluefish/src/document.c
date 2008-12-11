/* Bluefish HTML Editor
 * document.c - the document
 *
 * Copyright (C) 1998-2008 Olivier Sessink
 * Copyright (C) 1998 Chris Mazuc
 * some additions Copyright (C) 2004 Eugene Morenko(More)
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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h> /* for the keyboard event codes */

#include <string.h>         /* strchr() */
#include <regex.h>          /* regcomp() */
#include <stdlib.h>         /* system() */
#include <pcre.h>

/* #define DEBUG */

#ifdef DEBUGPROFILING
#include <sys/times.h>
#endif

#include "bluefish.h"
#include "bf_lib.h"
#include "bftextview2.h"
#include "bftextview2_langmgr.h"
#include "bookmark.h"
#include "dialog_utils.h"
#include "document.h"
#include "file.h"
#include "filebrowser2.h"
#include "filetype.h"
#include "file_dialogs.h"
#include "gtk_easy.h"       /* *_dialog() */
#include "gui.h"            /* statusbar_message() */
#include "menu.h"           /* add_to_recent_list */
#include "pixmap.h"
#include "snr2.h"           /* snr2_run_extern_replace */
#include "stringlist.h"     /* free_stringlist() */
#include "undo_redo.h"      /* doc_unre_init() */

typedef struct {
	GtkWidget *textview;
	GtkWidget *window;
} Tfloatingview;
#define FLOATINGVIEW(var) ((Tfloatingview *)(var))

typedef struct {
	gint so;
	gint eo;
} Tpasteoperation;
#define PASTEOPERATION(var) ((Tpasteoperation *)(var))

Tselectionsave *doc_save_selection(Tdocument *doc) {
	Tselectionsave *selsave;
	GtkTextIter start,end;
	selsave = g_slice_new(Tselectionsave);
	gtk_text_buffer_get_selection_bounds(doc->buffer,&start,&end);
	/*g_print("saving selection %d:%d\n",gtk_text_iter_get_offset(&start),gtk_text_iter_get_offset(&end));*/
	selsave->start = gtk_text_buffer_create_mark(doc->buffer,NULL,&start,FALSE);
	selsave->end = gtk_text_buffer_create_mark(doc->buffer,NULL,&end,TRUE);
	selsave->doc = doc;
	return selsave;
}

void doc_restore_selection(Tselectionsave *selsave, gboolean only_if_no_selection) {
	if (!only_if_no_selection || !gtk_text_buffer_get_has_selection(selsave->doc->buffer)) {
		GtkTextIter start,end;
		gtk_text_buffer_get_iter_at_mark(selsave->doc->buffer,&start,selsave->start);
		gtk_text_buffer_get_iter_at_mark(selsave->doc->buffer,&end,selsave->end);
		/*g_print("restoring selection %d:%d\n",gtk_text_iter_get_offset(&start),gtk_text_iter_get_offset(&end));*/
		gtk_text_buffer_select_range(selsave->doc->buffer,&start,&end);
	}
	gtk_text_buffer_delete_mark(selsave->doc->buffer,selsave->start);
	gtk_text_buffer_delete_mark(selsave->doc->buffer,selsave->end);
	g_slice_free(Tselectionsave,selsave);
}

#ifndef USE_BFTEXTVIEW2
void autoclosing_init(void) {
	const char *error;
	int erroffset;
	main_v->autoclosingtag_regc = pcre_compile("^<(([a-z_:]+)?[a-z0-9._:-]*)([\t\n ][^<>]*)?>$",
	                                           PCRE_CASELESS, &error, &erroffset, NULL);
#ifdef DEBUG
	if (!main_v->autoclosingtag_regc) {
		DEBUG_MSG("autoclosing_init, ERROR, %s\n",error);
	}
#endif
}
#endif

static void session_set_opendir(Tbfwin *bfwin, gchar *curi) {
	if (curi) {
		gchar *pos = strrchr(curi, '/');
		if (pos!=NULL) {
			if (bfwin->session->opendir) g_free(bfwin->session->opendir);
			bfwin->session->opendir = g_strndup(curi, pos-curi);
			DEBUG_MSG("session_set_opendir, opendir=%s\n",bfwin->session->opendir);
		}
	}
}

void session_set_savedir(Tbfwin *bfwin, gchar *curi) {
	if (curi) {
		gchar *pos = strrchr(curi, '/');
		if (pos!=NULL) {
			if (bfwin->session->savedir) g_free(bfwin->session->savedir);
			bfwin->session->savedir = g_strndup(curi, pos-curi);
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
GList *return_allwindows_documentlist() {
	GList *newdoclist=NULL, *bflist, *tmplist=NULL;
	bflist = g_list_first(main_v->bfwinlist);
	DEBUG_MSG("return_allwindows_documentlist, bfwinlist length=%d\n",g_list_length(main_v->bfwinlist));
	while (bflist) {
		DEBUG_MSG("return_allwindows_documentlist, current bfwin doclist length=%d\n",g_list_length(BFWIN(bflist->data)->documentlist));
		tmplist = g_list_first(BFWIN(bflist->data)->documentlist);
		while (tmplist) {
			newdoclist = g_list_prepend(newdoclist,tmplist->data);
			tmplist = g_list_next(tmplist);
		}
		bflist = g_list_next(bflist);
	}
	DEBUG_MSG("return_allwindows_documentlist, returning list length %d\n",g_list_length(newdoclist));
	return newdoclist;
}

void alldocs_foreach(foreachdocfunc func, gpointer data) {
	GList *tmplist, *doclist = return_allwindows_documentlist();
	tmplist=g_list_first(doclist);
	while (tmplist) {
		func(tmplist->data,data);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(doclist);
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
GList *return_urilist_from_doclist(GList *doclist) {
	GList *newlist=NULL, *tmplist;
	DEBUG_MSG("return_filenamestringlist_from_doclist, started for doclist %p, len=%d\n",doclist,g_list_length(doclist));
	tmplist = g_list_first(doclist);
	while(tmplist){
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
 * add_filename_to_history:
 * @bfwin: #Tbfwin*
 * @filename: a #gchar
 *
 * adds a filename to the recently opened files list
 * will not add it to the menu, only to the list and the file
 **/
void add_filename_to_history(Tbfwin *bfwin, gchar *filename) {
/*	gchar *dirname;*/

	add_to_recent_list(bfwin, filename, 0, FALSE); /* the recent menu */
/*	dirname = g_path_get_dirname(filename);
	DEBUG_MSG("add_filename_to_history, adding %s\n",dirname);
	main_v->recent_directories = add_to_history_stringlist(main_v->recent_directories,dirname,FALSE,TRUE);
	g_free(dirname);*/
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
gint documentlist_return_index_from_uri(GList *doclist, GFile *uri) {
	GList *tmplist;
	gint count=0;

	if (!uri) {
		return -1;
	}

	tmplist = g_list_first(doclist);
	while (tmplist) {
		Tdocument *doc = tmplist->data;
		if (doc->uri && (doc->uri == uri || g_file_equal(doc->uri,uri) )) {
			return count;
		}
		count++;
		tmplist = g_list_next(tmplist);
	}
	return -1;
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
Tdocument *documentlist_return_document_from_uri(GList *doclist, GFile *uri) {
	GList *tmplist;
	if (!uri) {
		DEBUG_MSG("documentlist_return_document_from_filename, no filename! returning\n");
		return NULL;
	}
	tmplist = g_list_first(doclist);
	while (tmplist) {
		if (DOCUMENT(tmplist->data)->uri && (DOCUMENT(tmplist->data)->uri == uri || g_file_equal(DOCUMENT(tmplist->data)->uri,uri))) {
			DEBUG_MSG("documentlist_return_document_from_filename, found, returning %p\n", tmplist->data);
			return DOCUMENT(tmplist->data);
		}
		tmplist = g_list_next(tmplist);
	}
	DEBUG_MSG("documentlist_return_document_from_filename, not found, returning NULL\n");
	return NULL;
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
Tdocument *documentlist_return_document_from_index(GList *doclist, gint index) {
	return (Tdocument *) g_list_nth_data(doclist, index);
}

/**
 * document_return_num_loading:
 * @doclist: a list of Tdocument* to count
 *
 * Return value: number of documents that are not 'complete'
 */
gint document_return_num_notcomplete(GList *doclist) {
	GList *tmplist;
	gint count=0;
	for (tmplist = g_list_first(doclist);tmplist!=NULL;tmplist=tmplist->next) {
		if (DOCUMENT(tmplist->data)->status != DOC_STATUS_COMPLETE) count++;
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
void doc_update_highlighting(Tbfwin *bfwin,guint callback_action, GtkWidget *widget) {
	if (!bfwin->current_document) return;
#ifdef USE_BFTEXTVIEW2
	if (!BLUEFISH_TEXT_VIEW(bfwin->current_document->view)->enable_scanner) {
		BLUEFISH_TEXT_VIEW(bfwin->current_document->view)->enable_scanner = TRUE;
	}
	bluefish_text_view_rescan(BLUEFISH_TEXT_VIEW(bfwin->current_document->view));
#else
	DEBUG_MSG("doc_update_highlighting, curdoc=%p, highlight=%d\n", bfwin->current_document, BF_TEXTVIEW(bfwin->current_document->view)->highlight);
	if (BF_TEXTVIEW(bfwin->current_document->view)->highlight == 0) {
		setup_toggle_item(gtk_item_factory_from_widget(bfwin->menubar), "/Document/Highlight Syntax", TRUE);
		DEBUG_MSG("doc_update_highlighting, calling doc_toggle_highlighting_cb\n");
		doc_toggle_highlighting_cb(bfwin, 0, NULL);
	} else {
		bf_textview_scan(BF_TEXTVIEW(bfwin->current_document->view));
	}
#endif
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
void doc_set_wrap(Tdocument * doc) {
	if (doc->wrapstate) {
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(doc->view),GTK_WRAP_WORD);
	} else {
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(doc->view),GTK_WRAP_NONE);
	}
}
/**
 * get_filetype_by_name:
 * @name: a #gchar* with the filetype name
 *
 * returns the Tfiletype* for corresponding to name
 *
 * Return value: Tfiletype*
 **/
#ifndef USE_BFTEXTVIEW2
Tfiletype *get_filetype_by_name(const gchar * name) {
	GList *tmplist;
	tmplist = g_list_first(main_v->filetypelist);
	while (tmplist) {
		if (strcmp(((Tfiletype *) tmplist->data)->type, name) == 0) {
			return (Tfiletype *) tmplist->data;
		}
		tmplist = g_list_next(tmplist);
	}
	return NULL;
}
#endif
#ifndef GNOMEVFSINT
/**
 * get_filetype_by_filename_and_content:
 * @filename: a #gchar* with the filename or NULL
 * @buf: a #gchar* with the contents to search for with the Tfiletype->content_regex or NULL
 *
 * returns the Tfiletype* for corresponding to filename, using the file extension. If
 * nothing is found using the file extension or filename==NULL it will start matching
 * the contents in buf with Tfiletype->content_regex
 *
 * if no filetype is found it will return NULL
 *
 * Return value: #Tfiletype* or NULL
 **/
Tfiletype *get_filetype_by_filename_and_content(const gchar *filename, gchar *buf) {
	GList *tmplist;
	DEBUG_MSG("get_filetype_by_filename_and_content, filename=%s, buf=%p\n",filename,buf);
	if (filename) {
		tmplist = g_list_first(main_v->filetypelist);
		while (tmplist) {
			if (filename_test_extensions(((Tfiletype *) tmplist->data)->extensions, filename)) {
				DEBUG_MSG("get_filetype_by_filename_and_content, found filetype %s for extension\n",((Tfiletype *)tmplist->data)->type);
				return (Tfiletype *) tmplist->data;
			}
			tmplist = g_list_next(tmplist);
		}
	}
	if (buf) {
		tmplist = g_list_first(main_v->filetypelist);
		while (tmplist) {
			Tfiletype *ft = (Tfiletype *)tmplist->data;
			if (strlen(ft->content_regex)) {
				pcre *pcreg;
				const char *err=NULL;
				int erroffset=0;
				DEBUG_MSG("get_filetype_by_filename_and_content, compiling pattern %s\n",ft->content_regex);
				pcreg = pcre_compile(ft->content_regex, PCRE_DOTALL|PCRE_MULTILINE,&err, &erroffset,NULL);
				if (err) {
					g_print("while testing for filetype '%s', pattern '%s' resulted in error '%s' at position %d\n", ft->type, ft->content_regex, err, erroffset);
				}
				if (pcreg) {
					int ovector[30];
					int retval = pcre_exec(pcreg,NULL,buf,strlen(buf),0,0,ovector,30);
					DEBUG_MSG("get_filetype_by_filename_and_content, pcre_exec retval=%d\n",retval);
					if (retval > 0) {
						/* we have a match!! */
						pcre_free(pcreg);
						return ft;
					}
					pcre_free(pcreg);
				}
			} else {
				DEBUG_MSG("get_filetype_by_filename_and_content, type %s does not have a pattern (%s)\n",ft->type,ft->content_regex);
			}
			tmplist = g_list_next(tmplist);
		}
	}
	return NULL;
}
#endif
/**
 * doc_set_tooltip:
 * @doc: #Tdocument*
 *
 * will set the tooltip on the notebook tab eventbox
 *
 * Return value: void
 */
void doc_set_tooltip(Tdocument *doc) {
	GString *retstr;
	gchar *tmp;
	gchar *mtimestr=NULL;
	gchar *sizestr=NULL;

	retstr = g_string_new(_("Name: "));
	retstr = g_string_append(retstr, gtk_label_get_text(GTK_LABEL(doc->tab_menu)));

#ifdef USE_BFTEXTVIEW2
	if (BLUEFISH_TEXT_VIEW(doc->view)->bflang) {
		retstr = g_string_append(retstr, _("\nFile type: "));
		retstr = g_string_append(retstr, BLUEFISH_TEXT_VIEW(doc->view)->bflang->name);
	}
#else
	if (doc->hl) {
		retstr = g_string_append(retstr, _("\nFile type: "));
		retstr = g_string_append(retstr, doc->hl->type);
	}
#endif
	if (doc->encoding) {
		retstr = g_string_append(retstr, _("\nEncoding: "));
		retstr = g_string_append(retstr, doc->encoding);
	}
	DEBUG_MSG("doc_set_tooltip, fileinfo=%p for doc %s\n", doc->fileinfo, gtk_label_get_text(GTK_LABEL(doc->tab_menu)));
	if (doc->fileinfo) {
		if (g_file_info_has_attribute(doc->fileinfo, G_FILE_ATTRIBUTE_UNIX_MODE)) {
			tmp = filemode_to_string(g_file_info_get_attribute_uint32(doc->fileinfo, G_FILE_ATTRIBUTE_UNIX_MODE));
			g_string_append_printf(retstr, _("\nPermissions: %s\nUid: %u Gid: %u"),tmp,g_file_info_get_attribute_uint32(doc->fileinfo,G_FILE_ATTRIBUTE_UNIX_UID),g_file_info_get_attribute_uint32(doc->fileinfo,G_FILE_ATTRIBUTE_UNIX_GID));
			g_free(tmp);
		}
		if (g_file_info_has_attribute(doc->fileinfo, G_FILE_ATTRIBUTE_STANDARD_SIZE)) {
			sizestr = g_format_size_for_display(g_file_info_get_size(doc->fileinfo));
			retstr = g_string_append(retstr, _("\nSize (on disk): "));
			retstr = g_string_append(retstr, sizestr);
			g_free(sizestr);
		}
#ifdef USE_BFTEXTVIEW2
		if (doc->fileinfo) {
			const gchar *mime = g_file_info_get_attribute_string(doc->fileinfo, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);
			if (!mime)
				mime = g_file_info_get_attribute_string(doc->fileinfo, G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE);
			if (mime) {
				retstr = g_string_append(retstr, _("\nMime type: "));
				retstr = g_string_append(retstr, mime);
			}
		}
#else
		if (doc->hl && doc->hl->mime_type) {
			retstr = g_string_append(retstr, _("\nMime type: "));
			DEBUG_MSG("doc %p has filetype %p with mime %s\n",doc,doc->hl,doc->hl->mime_type);
			retstr = g_string_append(retstr, doc->hl->mime_type);
		}
#endif
		if (g_file_info_has_attribute(doc->fileinfo, G_FILE_ATTRIBUTE_TIME_MODIFIED)) {
			/* this function always appends a newline to the string*/
			time_t modtime = (time_t) g_file_info_get_attribute_uint64(doc->fileinfo, G_FILE_ATTRIBUTE_TIME_MODIFIED);
			mtimestr = bf_portable_time(&modtime);
			retstr = g_string_append(retstr, _("\nLast modified: "));
			retstr = g_string_append(retstr, mtimestr);
			g_free(mtimestr);
			retstr = g_string_truncate(retstr,retstr->len-1);
		}
	}
	tmp = g_string_free(retstr,FALSE);
	gtk_tooltips_set_tip(main_v->tooltips, doc->tab_eventbox, tmp, "");
	g_free(tmp);
}
#ifndef USE_BFTEXTVIEW2
/**
 * doc_set_filetype:
 * @doc: a #Tdocument
 * @ft: a #Tfiletype with the new filetype
 *
 * this function will compare the filetype from the document and the new filetype
 * and if they are different it will remove the old highlighting, set the newfiletype
 * and set the filetype widget, it will return TRUE if the type was changed
 *
 * Return value: #gboolean if the value was changed
 **/
gboolean doc_set_filetype(Tdocument *doc, Tfiletype *ft) {
	DEBUG_MSG("doc_set_filetype, will set filetype %s\n",ft->type);
	if (ft != doc->hl) {
		doc->hl = ft;
		doc->need_highlighting = TRUE;
		DEBUG_MSG("doc_set_filetype, calling bf_textview_set_language_ptr(%p)\n",ft->cfg);
		bf_textview_set_language_ptr(BF_TEXTVIEW(doc->view),ft->cfg);
		gui_set_document_widgets(doc);
		doc_set_tooltip(doc);
		return TRUE;
	}
	return FALSE;
}
#endif
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
void doc_set_title(Tdocument *doc) {
	gchar *label_string, *tabmenu_string;
	if (doc->uri) {
		gchar *parsename, *basename;
		tabmenu_string = g_file_get_uri(doc->uri);
		parsename = g_file_get_parse_name(doc->uri);
		basename = g_path_get_basename(parsename);
		label_string = g_strdup(basename);
		g_free(parsename);
		g_free(basename);
	} else {
		label_string = g_strdup_printf(_("Untitled %d"),main_v->num_untitled_documents);
		tabmenu_string =  g_strdup(label_string);
		main_v->num_untitled_documents++;
	}
	gtk_label_set(GTK_LABEL(doc->tab_menu),tabmenu_string);
	gtk_label_set(GTK_LABEL(doc->tab_label),label_string);
	DEBUG_MSG("doc_set_title, tabmenu_string=%s,label_string=%s\n",tabmenu_string,label_string);
	doc_set_tooltip(doc);
	g_free(label_string);
	g_free(tabmenu_string);
	if (doc == BFWIN(doc->bfwin)->current_document) {
		gui_set_title(doc->bfwin, doc);
	}
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
void doc_reset_filetype(Tdocument * doc, GFile *newuri, gconstpointer buf, gssize buflen) {
	const gchar *mimetype = NULL;
#ifndef USE_BFTEXTVIEW2
	Tfiletype *ft=NULL;
#endif
	gboolean uncertain=FALSE;
	char *filename, *conttype;

	filename = g_file_get_basename(newuri);
	conttype = g_content_type_guess(filename,buf,buflen,&uncertain);
	DEBUG_MSG("doc_reset_filetype,conttype=%s\n",conttype);
	g_free(filename);
	/*mimetype = g_content_type_get_mime_type(conttype);*/
	mimetype=conttype;
	DEBUG_MSG("doc_reset_filetype,mimetype=%s\n",mimetype);
	/* docs are unclear if conttype is a static string or a newly allocated string */

#ifdef USE_BFTEXTVIEW2
	bluefish_text_view_set_mimetype(BLUEFISH_TEXT_VIEW(doc->view), mimetype);
	if (doc->fileinfo) {
		g_file_info_set_content_type(doc->fileinfo,mimetype);
	}
	/* TODO: set the mime type in the doc->fileinfo so we can use it in the tooltips */
#else
	if (mimetype) {
		ft = get_filetype_for_mime_type(mimetype);
		g_free(conttype);
	} else {
		GList *tmplist;
		tmplist = g_list_first(main_v->filetypelist);
		ft = (Tfiletype *)tmplist->data;
	}
	doc_set_filetype(doc, ft);
#endif
}

void doc_set_filename(Tdocument *doc, GFile *newuri) {
	DEBUG_MSG("doc_set_filename, started\n");
	if (newuri) {
		gchar *buf;

		if (doc->uri)
			g_object_unref(doc->uri);
		doc->uri = newuri;
		g_object_ref(newuri);
		doc_set_title(doc);
		buf = doc_get_chars(doc, 0, -1);
		doc_reset_filetype(doc, doc->uri, buf, strlen(buf));
		g_free(buf);
	}
}

/**
 * doc_set_font:
 * @doc: a #Tdocument
 * @fontstring: a #gchar describing the font
 *
 * this function will set the textview from doc to use the font
 * described by fontstring
 *
 * Return value: void
 **/

#ifdef __GNUC__
__inline__
#endif
void doc_set_font(Tdocument *doc, gchar *fontstring) {
	apply_font_style(doc->view, fontstring ? fontstring : main_v->props.editor_font_string);
}

/**
 * This function is taken from gtksourceview
 * Copyright (C) 2001
 * Mikael Hermansson <tyan@linux.se>
 * Chris Phelps <chicane@reninet.com>
 */
static gint textview_calculate_real_tab_width(GtkWidget *textview, gint tab_size) {
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
	tab_width =  widget_get_string_size(textview, tab_string);
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
void doc_set_tabsize(Tdocument *doc, gint tabsize) {
	PangoTabArray *tab_array;
	gint pixels = textview_calculate_real_tab_width(GTK_WIDGET(doc->view), tabsize);
	DEBUG_MSG("doc_set_tabsize, tabsize=%d, pixels=%d\n", tabsize, pixels);
	tab_array = pango_tab_array_new (1, TRUE);
	pango_tab_array_set_tab (tab_array, 0, PANGO_TAB_LEFT, pixels);
	gtk_text_view_set_tabs (GTK_TEXT_VIEW (doc->view), tab_array);
	pango_tab_array_free(tab_array);
}

/**
 * gui_change_tabsize:
 * @bfwin: #Tbfwin* with the window
 * @action: a #guint, if 1 increase the tabsize, if 0 decrease
 * @widget: a #GtkWidget, ignored
 *
 * this function is the callback for the menu, based on action
 * it will increase or decrease the tabsize by one
 * for ALL DOCUMENTS (BUG: currently only all documents in the same window)
 *
 * Return value: void
 **/
void gui_change_tabsize(Tbfwin *bfwin,guint action,GtkWidget *widget) {
	GList *tmplist;
	PangoTabArray *tab_array;
	gint pixels;
	if (action == 1) {
		main_v->props.editor_tab_width++;
	} else {
		main_v->props.editor_tab_width--;
	}
	{
		gchar *message = g_strdup_printf("Setting tabsize to %d", main_v->props.editor_tab_width);
		statusbar_message(bfwin,message, 2000);
		g_free(message);
	}
	/* this should eventually be the total documentlist, not only for this window */
	tmplist = g_list_first(bfwin->documentlist);
	pixels = textview_calculate_real_tab_width(GTK_WIDGET(((Tdocument *)tmplist->data)->view), main_v->props.editor_tab_width);
	tab_array = pango_tab_array_new (1, TRUE);
	pango_tab_array_set_tab (tab_array, 0, PANGO_TAB_LEFT, pixels);
	while (tmplist) {
		gtk_text_view_set_tabs (GTK_TEXT_VIEW(((Tdocument *)tmplist->data)->view), tab_array);
		tmplist = g_list_next(tmplist);
	}
	pango_tab_array_free(tab_array);
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
gboolean doc_is_empty_non_modified_and_nameless(Tdocument *doc) {
	if (!doc) {
		return FALSE;
	}
	if (doc->modified || doc->uri) {
		return FALSE;
	}
	if (gtk_text_buffer_get_char_count(doc->buffer) > 0) {
		return FALSE;
	}
	return TRUE;
}

/**
 * test_docs_modified:
 * @doclist: a #GList with documents
 *
 * this function will test if any documents in doclist are modified
 *
 * Return value: gboolean
 **/

gboolean test_docs_modified(GList *doclist) {
	GList *tmplist;
	Tdocument *tmpdoc;

	tmplist = g_list_first(doclist);
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
gboolean test_only_empty_doc_left(GList *doclist) {
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
void doc_move_to_window(Tdocument *doc, Tbfwin *oldwin, Tbfwin *newwin)
{
	GtkWidget *tab_widget, *scroll;
	DEBUG_MSG("doc_move_to_window, oldwin=%p, newwin=%p, doc=%p\n",oldwin,newwin,doc);
	/* first test if the document still exists in the old window */
	if (g_list_index(oldwin->documentlist, doc)==-1 || !bfwin_exists(oldwin) || !bfwin_exists(newwin)) {
		DEBUG_MSG("doc_move_to_window, the document no longer exists in oldwin %p\n",oldwin);
		return;
	}
	tab_widget = doc->tab_eventbox->parent;
	scroll = doc->view->parent;
	gtk_widget_ref(scroll);
	gtk_widget_ref(tab_widget);
	gtk_widget_ref(doc->tab_menu);
	DEBUG_MSG("doc_move_to_window, tab_label=%p, tab_widget=%p\n",doc->tab_label, tab_widget);
/*	gtk_container_remove(GTK_CONTAINER(oldwin->notebook), doc->view);*/
	gtk_notebook_remove_page(GTK_NOTEBOOK(oldwin->notebook), g_list_index(oldwin->documentlist, doc));
	oldwin->documentlist = g_list_remove(oldwin->documentlist, doc);
	DEBUG_MSG("doc_move_to_window, removed doc=%p from oldwin %p\n",doc,oldwin);
	doc->bfwin = newwin;
	newwin->documentlist = g_list_append(newwin->documentlist, doc);
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(newwin->notebook), scroll, tab_widget, doc->tab_menu);
	DEBUG_MSG("doc_move_to_window, appended doc=%p to newwin %p\n",doc,newwin);

	gtk_widget_unref(scroll);
	gtk_widget_unref(tab_widget);
	gtk_widget_unref(doc->tab_menu);

	gtk_widget_show_all(scroll);
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

static void doc_move_to_window_dialog_response_lcb(GtkDialog *dialog,gint response,gpointer user_data) {
	Tdmwd *dmwd = (Tdmwd *)user_data;
	if (response == 1) {
		doc_move_to_window(dmwd->doc, dmwd->oldwin, dmwd->newwin);
	} else if (response == 2) {
		/* TODO: open readonly */
		file_doc_from_uri(dmwd->newwin, dmwd->doc->uri, NULL, -1, -1, TRUE);
	} else {
		/* TODO: do not open */
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_free(dmwd);
}
void doc_move_to_window_dialog(Tdocument *doc, Tbfwin *newwin) {
	GtkWidget *dialog;
	Tdmwd *dmwd;
	dmwd = g_new(Tdmwd,1);
	dmwd->doc = doc;
	dmwd->newwin = newwin;
	dmwd->oldwin = doc->bfwin;
	dialog = gtk_message_dialog_new(GTK_WINDOW(newwin->main_window),GTK_DIALOG_DESTROY_WITH_PARENT
			,GTK_MESSAGE_QUESTION,GTK_BUTTONS_NONE
			,_("Document %s is already open in another Bluefish window.")
			,gtk_label_get_text(GTK_LABEL(doc->tab_label)) );
	gtk_dialog_add_buttons(GTK_DIALOG(dialog),_("Move to this window"),1,_("Open readonly"),2,_("Do not open"),3,NULL);
	g_signal_connect(dialog, "response", G_CALLBACK(doc_move_to_window_dialog_response_lcb), dmwd);
	gtk_widget_show_all(dialog);
}

static void doc_set_label_color(Tdocument *doc, GdkColor *color) {
	gtk_widget_modify_fg(doc->tab_menu, GTK_STATE_NORMAL, color);
	gtk_widget_modify_fg(doc->tab_menu, GTK_STATE_PRELIGHT, color);
	gtk_widget_modify_fg(doc->tab_label, GTK_STATE_NORMAL, color);
	gtk_widget_modify_fg(doc->tab_label, GTK_STATE_PRELIGHT, color);
	gtk_widget_modify_fg(doc->tab_label, GTK_STATE_ACTIVE, color);
}

void doc_set_status(Tdocument *doc, gint status) {
	GdkColor colorblack = {0, 0, 0, 0};
	GdkColor colorgrey = {0, 50000, 50000, 50000};
	GdkColor colorred = {0, 65535, 0, 0};
	doc->status = status;
	switch(status) {
		case DOC_STATUS_COMPLETE:
			doc_set_label_color(doc, &colorblack);
		break;
		case DOC_STATUS_ERROR:
			doc_set_label_color(doc, &colorred);
		break;
		case DOC_STATUS_LOADING:
			doc_set_label_color(doc, &colorgrey);
		break;
	}
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
 * label red, if value is FALSE it will set them to black
 *
 * Return value: void
 **/
void doc_set_modified(Tdocument *doc, gint value) {
	DEBUG_MSG("doc_set_modified, started, doc=%p, value=%d\n", doc, value);
	if (doc->modified != value) {
		GdkColor colorblue = {0, 0, 0, 65535};
		GdkColor colorblack = {0, 0, 0, 0};

		doc->modified = value;
		if (doc->modified) {
			doc_set_label_color(doc, &colorblue);
		} else {
			doc_set_label_color(doc, &colorblack);
		}
	}
#ifdef DEBUG
	else {
		DEBUG_MSG("doc_set_modified, doc %p did have value %d already\n", doc, value);
	}
#endif
	/* only when this is the current document we have to change these */
	DEBUG_MSG("doc=%p, doc->bfwin=%p\n",doc,doc->bfwin);
	if (doc == BFWIN(doc->bfwin)->current_document) {
		gui_set_undo_redo_widgets(BFWIN(doc->bfwin),doc_has_undo_list(doc), doc_has_redo_list(doc));
	}
#ifdef DEBUG
	else {
		DEBUG_MSG("doc_set_modified, doc != current_document, so we do not update the gui widgets\n");
	}
#endif
}

/* returns 1 if the file is modified on disk, returns 0
if the file is modified by another process, returns
0 if there was no previous mtime information available
if newstatbuf is not NULL, it will be filled with the new statbuf from the file IF IT WAS CHANGED!!!
leave NULL if you do not need this information, if the file is not changed, this field will not be set!!
* /
static gboolean doc_check_modified_on_disk(Tdocument *doc, GFileInfo **newfileinfo) {
	if (main_v->props.modified_check_type == 0 || !doc->uri || doc->fileinfo == NULL) {
		return FALSE;
	} else if (main_v->props.modified_check_type < 4) {
		GFileInfo *fileinfo;
		gboolean unref_fileinfo = FALSE;
		if (*newfileinfo == NULL) {
			fileinfo = gnome_vfs_file_info_new();
			unref_fileinfo = TRUE;
			DEBUG_MSG("doc_check_modified_on_disk, allocating new fileinfo at %p\n", fileinfo);
		} else {
			fileinfo = *newfileinfo;
			DEBUG_MSG("doc_check_modified_on_disk, using existing fileinfo at %p\n", fileinfo);
		}
		if (gnome_vfs_get_file_info(doc->uri, fileinfo
					, GNOME_VFS_FILE_INFO_DEFAULT|GNOME_VFS_FILE_INFO_FOLLOW_LINKS) == GNOME_VFS_OK) {
			if (main_v->props.modified_check_type == 1 || main_v->props.modified_check_type == 2) {
				if (doc->fileinfo->mtime < fileinfo->mtime) {
					if (unref_fileinfo) g_object_unref(fileinfo);
					return TRUE;
				}
			}
			if (main_v->props.modified_check_type == 1 || main_v->props.modified_check_type == 3) {
				if (doc->fileinfo->size != fileinfo->size) {
					if (unref_fileinfo) g_object_unref(fileinfo);
					return TRUE;
				}
			}
		}
		if (unref_fileinfo) g_object_unref(fileinfo);
	} else {
		DEBUG_MSG("doc_check_mtime, type %d checking not yet implemented\n", main_v->props.modified_check_type);
	}
	return FALSE;
}*/

/* doc_set_stat_info() includes setting the mtime field, so there is no need
to call doc_update_mtime() as well */
/* static void doc_set_stat_info(Tdocument *doc) {
	if (doc->uri) {
		if (doc->fileinfo == NULL) {
			doc->fileinfo = gnome_vfs_file_info_new();
			DEBUG_MSG("doc_set_stat_info, new fileinfo at %p\n",doc->fileinfo);
		}
		gnome_vfs_get_file_info(doc->uri, doc->fileinfo
				,GNOME_VFS_FILE_INFO_DEFAULT|GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
		doc->is_symlink = GNOME_VFS_FILE_INFO_SYMLINK(doc->fileinfo);
		doc_set_tooltip(doc);
	}
} */

/**
 * doc_scroll_to_cursor:
 * @doc: a #Tdocument
 *
 * scolls the document pointer to by doc to its cursor position,
 * making the cursor visible
 *
 * Return value: void
 **/
void doc_scroll_to_cursor(Tdocument *doc) {
	GtkTextMark *mark = gtk_text_buffer_get_insert(doc->buffer);
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(doc->view),mark,0.25,FALSE,0.5,0.5);
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
	string = gtk_text_buffer_get_text(doc->buffer, &itstart, &itend,TRUE);
	DEBUG_MSG("doc_get_chars, retrieved string (%p)\n", string);
	return string;
}
/**
 * doc_get_max_offset:
 * @doc: a #Tdocument
 *
 * returns the number of characters (NOT BYTES!!) in this document
 *
 * Return value: gint with the number of characters
 **/
#ifdef __GNUC__
__inline__
#endif
gint doc_get_max_offset(Tdocument *doc) {
	return gtk_text_buffer_get_char_count(doc->buffer);
}

static void doc_select_and_scroll(Tdocument *doc, GtkTextIter *it1,
                                  GtkTextIter *it2, gboolean select_it1_line,
                                  gboolean do_scroll) {
	GtkTextIter sit1=*it1, sit2=*it2;
	GdkRectangle visirect;
	GtkTextIter visi_so, visi_eo;

	if (select_it1_line) {
		sit2 = sit1;
		gtk_text_iter_set_line_offset(&sit1,0);
		gtk_text_iter_forward_to_line_end(&sit2);
	}
	gtk_text_buffer_select_range(doc->buffer,&sit1,&sit2);
	if (do_scroll) {
		gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(doc->view),&visirect);
		gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(doc->view), &visi_so, visirect.x, visirect.y);
		gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(doc->view), &visi_eo, visirect.x + visirect.width, visirect.y + visirect.height);
		/* in doc reload this works strange, there is no scrolling to the correct position...
		perhaps this should be done in an idle callback so that the iter positions can be calculated?? */
		if (!gtk_text_iter_in_range(&sit1,&visi_so,&visi_eo)) {
			gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(doc->view),&sit1,0.25,TRUE,0.5,0.10);
			gtk_widget_grab_focus(doc->view);
		}
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
void doc_select_region(Tdocument *doc, gint start, gint end, gboolean do_scroll) {
	GtkTextIter itstart, itend;
	gtk_text_buffer_get_iter_at_offset(doc->buffer, &itstart,start);
	gtk_text_buffer_get_iter_at_offset(doc->buffer, &itend,end);
	doc_select_and_scroll(doc, &itstart, &itend,FALSE, do_scroll);
}

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
void doc_select_line(Tdocument *doc, gint line, gboolean do_scroll) {
	GtkTextIter itstart;
	gtk_text_buffer_get_iter_at_line(doc->buffer,&itstart,line-1);
	doc_select_and_scroll(doc, &itstart, &itstart,TRUE, do_scroll);
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
void doc_select_line_by_offset(Tdocument *doc, gint offset, gboolean do_scroll) {
	GtkTextIter itstart;
	gtk_text_buffer_get_iter_at_offset(doc->buffer, &itstart,offset);
	doc_select_and_scroll(doc, &itstart, &itstart,TRUE, do_scroll);
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
/**
 * doc_get_cursor_position:
 * @doc: a #Tdocument
 *
 * returns the cursor position in doc as character offset
 *
 * Return value: gint with the character offset of the cursor
 **/
gint doc_get_cursor_position(Tdocument *doc) {
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
static void doc_set_statusbar_lncol(Tdocument *doc) {
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
			col += (main_v->props.editor_tab_width - (col  % main_v->props.editor_tab_width));
		} else ++col;
		gtk_text_iter_forward_char(&start);
	}
	if (1) {
		msg = g_strdup_printf(_(" Ln: %d, Col: %d, Char: %d"), line + 1, col + 1,gtk_text_iter_get_offset(&iter));
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
void doc_set_statusbar_insovr(Tdocument *doc)
{
	gtk_statusbar_pop(GTK_STATUSBAR(BFWIN(doc->bfwin)->statusbar_insovr), 0);
	if (!doc->readonly)
		gtk_statusbar_push(GTK_STATUSBAR(BFWIN(doc->bfwin)->statusbar_insovr), 0, (doc->overwrite_mode ? _(" OVR") : _(" INS")));
	else
		gtk_statusbar_push(GTK_STATUSBAR(BFWIN(doc->bfwin)->statusbar_insovr), 0, _(" RO"));
}
/**
 * doc_set_statusbar_editmode_encoding:
 * @doc: a #Tdocument
 *
 *
 *
 *
 * Return value: void
 **/
void doc_set_statusbar_editmode_encoding(Tdocument *doc)
{
	gchar *msg;
#ifdef USE_BFTEXTVIEW2
	if (doc->fileinfo) {
		const gchar *mime = g_file_info_get_attribute_string(doc->fileinfo, "standard::content-type");
		msg = g_strdup_printf(_("  %s, %s"), mime, doc->encoding);
	} else {
		msg = g_strdup_printf(_("  %s, %s"), "unknown", doc->encoding);
	}
#else
	if (doc->hl == NULL) msg = g_strdup_printf(_("  %s, %s"), "unknown", doc->encoding);
	else msg = g_strdup_printf(_("  %s, %s"), doc->hl->type, doc->encoding);
#endif
	gtk_statusbar_pop(GTK_STATUSBAR(BFWIN(doc->bfwin)->statusbar_editmode), 0);
	gtk_statusbar_push(GTK_STATUSBAR(BFWIN(doc->bfwin)->statusbar_editmode), 0, msg);
	g_free(msg);
}

void doc_insert_text_backend(Tdocument *doc, const gchar * newstring, gint position) {
	GtkTextIter iter;
	doc_unbind_signals(doc);
	gtk_text_buffer_get_iter_at_offset(doc->buffer, &iter,position);
	gtk_text_buffer_insert(doc->buffer,&iter,newstring,-1);
	doc_unre_add(doc, newstring, position, position + g_utf8_strlen(newstring,-1), UndoInsert);
	doc_bind_signals(doc);
	doc_set_modified(doc, 1);
	doc->need_highlighting=TRUE;
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
void doc_replace_text_backend(Tdocument *doc, const gchar * newstring, gint start, gint end) {
	doc_unbind_signals(doc);
#ifndef USE_BFTEXTVIEW2
	bf_textview_set_delay_rescan(BF_TEXTVIEW(doc->view), TRUE);
#endif
	/* delete region, and add that to undo/redo list */
	{
		gchar *buf;
		GtkTextIter itstart, itend;
		DEBUG_MSG("doc_replace_text_backend, get iters at start %d and end %d\n", start, end);
		gtk_text_buffer_get_iter_at_offset(doc->buffer, &itstart,start);
		if (end == -1) {
			gtk_text_buffer_get_end_iter(doc->buffer,&itend);
		} else {
			gtk_text_buffer_get_iter_at_offset(doc->buffer, &itend,end);
		}
		buf = gtk_text_buffer_get_text(doc->buffer, &itstart, &itend,TRUE);
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
#ifndef USE_BFTEXTVIEW2
	bf_textview_set_delay_rescan(BF_TEXTVIEW(doc->view), FALSE);
	doc->need_highlighting=TRUE;
#endif
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
void doc_replace_text(Tdocument * doc, const gchar * newstring, gint start, gint end) {
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
void doc_insert_two_strings(Tdocument *doc, const gchar *before_str, const gchar *after_str) {
	GtkTextIter itinsert, itselect;
	GtkTextMark *insert, *select;
	gboolean have_diag_marks = FALSE;
	insert = gtk_text_buffer_get_mark(doc->buffer,"diag_ins");
	if (insert) {
		select = gtk_text_buffer_get_mark(doc->buffer,"diag_sel");
		have_diag_marks = TRUE;
	} else {
		insert = gtk_text_buffer_get_insert(doc->buffer);
		select = gtk_text_buffer_get_selection_bound(doc->buffer);
	}
	gtk_text_buffer_get_iter_at_mark(doc->buffer,&itinsert,insert);
	gtk_text_buffer_get_iter_at_mark(doc->buffer,&itselect,select);
#ifdef DEBUG
	g_print("doc_insert_two_strings, current marks: itinsert=%d, itselect=%d\n",gtk_text_iter_get_offset(&itinsert),gtk_text_iter_get_offset(&itselect));
#endif

	if (gtk_text_iter_equal(&itinsert, &itselect)) {
		/* no selection */
		gchar *double_str = g_strconcat(before_str, after_str, NULL);
		gtk_text_buffer_insert(doc->buffer,&itinsert,double_str,-1);
		g_free(double_str);
		if (after_str && strlen(after_str)) {
			/* the buffer has changed, but gtk_text_buffer_insert makes sure */
			/* that itinsert points to the end of the inserted text. */
			/* thus, no need to get a new one. */
			gtk_text_iter_backward_chars(&itinsert, g_utf8_strlen(after_str, -1));
			gtk_text_buffer_place_cursor(doc->buffer, &itinsert);
			gtk_widget_grab_focus(doc->view);
		}
	} else { /* there is a selection */
		GtkTextMark *marktoresetto;
		GtkTextIter firstiter;
		if (gtk_text_iter_compare(&itinsert,&itselect) < 0) {
			firstiter = itinsert;
			marktoresetto = (have_diag_marks) ? gtk_text_buffer_get_selection_bound(doc->buffer) : select;
		} else {
			firstiter = itselect;
			marktoresetto = (have_diag_marks) ? gtk_text_buffer_get_insert(doc->buffer) : insert;
		}
		/* there is a selection */
		gtk_text_buffer_insert(doc->buffer,&firstiter,before_str,-1);
		if (after_str && strlen(after_str)) {
			/* the buffer is changed, reset the select iterator */
			gtk_text_buffer_get_iter_at_mark(doc->buffer,&itselect,marktoresetto);
			gtk_text_buffer_insert(doc->buffer,&itselect,after_str,-1);
			/* now the only thing left is to move the selection and insert mark back to their correct places
			to preserve the users selection */
			gtk_text_buffer_get_iter_at_mark(doc->buffer,&itselect,marktoresetto);
			gtk_text_iter_backward_chars(&itselect, g_utf8_strlen(after_str, -1));
			gtk_text_buffer_move_mark(doc->buffer,marktoresetto,&itselect);
		}

	}
	doc_unre_new_group(doc);
	DEBUG_MSG("doc_insert_two_strings, finished\n");
}

static void add_encoding_to_list(gchar *encoding) {
	gchar **enc = g_new0(gchar *,4);
	enc[0] = g_strdup(encoding);
	if (!arraylist_value_exists(main_v->props.encodings, (const gchar **)enc, 1, FALSE)) {
		GList *tmplist;
		enc[1] = g_strdup(encoding);
		enc[2] = g_strdup("TRUE");
		main_v->props.encodings = g_list_insert(main_v->props.encodings, enc, 1);
		tmplist = g_list_first(main_v->bfwinlist);
		while (tmplist) {
			encoding_menu_rebuild(BFWIN(tmplist->data));
			tmplist = g_list_next(tmplist);
		}
	} else {
		g_free(enc[0]);
		g_free(enc);
	}
}

/**
 * buffer_find_encoding:
 * @buffer: gchar* with \- terminated string
 * @encoding: gchar**, if found a newly allocated encoding string will be here
 *
 * Return value: newly allocated buffer in UTF-8
 */
gchar *buffer_find_encoding(gchar *buffer, gsize buflen, gchar **encoding, const gchar *sessionencoding) {
	gchar *newbuf=NULL;
	gsize wsize;
	GError *error=NULL;
	gchar *tmpencoding = NULL;
	/* the first try is if the encoding is set in the file */
	{
		regmatch_t pmatch[10];
		gint retval;
		/* <meta http-equiv="content-type" content="text/html; charset=UTF-8" /> */
		/* we do a nasty trick to make regexec search only in the first N bytes */
		if (buflen > main_v->props.encoding_search_Nbytes) {
			gchar tmp = buffer[main_v->props.encoding_search_Nbytes];
			buffer[main_v->props.encoding_search_Nbytes] = '\0';
			retval = regexec(&main_v->find_encoding,buffer,10,pmatch,0);
			buffer[main_v->props.encoding_search_Nbytes] = tmp;
		} else {
			retval = regexec(&main_v->find_encoding,buffer,10,pmatch,0);
		}
#ifdef DEBUG
		if (retval) {
			gchar errbuf[1024];
			regerror(retval, &main_v->find_encoding, errbuf, 1024);
			g_print("regexec error! %s\n", errbuf);
		}
#endif
		if (retval==0 && pmatch[0].rm_so != -1 && pmatch[1].rm_so != -1) {
			/* we have a match */
			DEBUG_MSG("doc_buffer_to_textbox, match so=%d,eo=%d\n", pmatch[1].rm_so,pmatch[1].rm_eo);
			tmpencoding = g_strndup(&buffer[pmatch[1].rm_so], pmatch[1].rm_eo-pmatch[1].rm_so);
			DEBUG_MSG("doc_buffer_to_textbox, detected encoding %s\n", tmpencoding);
		}
#ifdef DEBUGPROFILING
		times(&locals.tms1);
		print_time_diff("encoding regex match", &locals.tms2, &locals.tms1);
#endif
	}
	if (tmpencoding) {
		DEBUG_MSG("doc_buffer_to_textbox, try encoding %s from <meta>\n", tmpencoding);
		newbuf = g_convert(buffer,-1,"UTF-8",tmpencoding,NULL, &wsize, &error);
		if (!newbuf || error) {
			DEBUG_MSG("doc_buffer_to_textbox, cound not convert %s to UTF-8: \n", tmpencoding);
			g_free(tmpencoding);
			tmpencoding=NULL;
		} else {
			*encoding = tmpencoding;
			return newbuf;
		}
	}
	if (sessionencoding) {
		DEBUG_MSG("doc_buffer_to_textbox, file does not have <meta> encoding, or could not convert, trying session default encoding %s\n", sessionencoding);
		newbuf = g_convert(buffer,-1,"UTF-8",sessionencoding,NULL, &wsize, NULL);
		if (newbuf) {
			DEBUG_MSG("doc_buffer_to_textbox, file is in default encoding: %s\n", sessionencoding);
			*encoding = g_strdup(sessionencoding);
			return newbuf;
		}
	}
	DEBUG_MSG("doc_buffer_to_textbox, file does not have <meta> encoding, or could not convert, trying newfile default encoding %s\n", main_v->props.newfile_default_encoding);
	newbuf = g_convert(buffer,-1,"UTF-8",main_v->props.newfile_default_encoding,NULL, &wsize, NULL);
	if (newbuf) {
		DEBUG_MSG("doc_buffer_to_textbox, file is in default encoding: %s\n", main_v->props.newfile_default_encoding);
		*encoding = g_strdup(main_v->props.newfile_default_encoding);
		return newbuf;
	}

	DEBUG_MSG("doc_buffer_to_textbox, file is not in UTF-8, trying encoding from locale\n");
	newbuf = g_locale_to_utf8(buffer,-1,NULL,&wsize,NULL);
	if (newbuf) {
		const gchar *tmpencoding=NULL;
		g_get_charset(&tmpencoding);
		DEBUG_MSG("doc_buffer_to_textbox, file is in locale encoding: %s\n", tmpencoding);
		*encoding = g_strdup(tmpencoding);
		return newbuf;
	}

	DEBUG_MSG("doc_buffer_to_textbox, file NOT is converted yet, trying UTF-8 encoding\n");
	if(g_utf8_validate(buffer, -1, NULL)) {
		*encoding = g_strdup("UTF-8");
		return g_strdup(buffer);
	}

	{
		GList *tmplist;
		DEBUG_MSG("doc_buffer_to_textbox, tried the most obvious encodings, nothing found.. go trough list\n");
		tmplist = g_list_first(main_v->props.encodings);
		while (tmplist) {
			gchar **enc = tmplist->data;
			DEBUG_MSG("doc_buffer_to_textbox, trying encoding %s\n", enc[1]);
			newbuf = g_convert(buffer,-1,"UTF-8",enc[1],NULL, &wsize, NULL);
			if (newbuf) {
				*encoding = g_strdup(enc[1]);
				return newbuf;
			}
			tmplist = g_list_next(tmplist);
		}
	}
	return NULL;
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
gboolean doc_buffer_to_textbox(Tdocument * doc, gchar * buffer, gsize buflen, gboolean enable_undo, gboolean delay) {
	gint cursor_offset;
	gchar *encoding=NULL, *newbuf;
	GtkTextMark* insert;
	GtkTextIter iter;

	if (!buffer) {
		DEBUG_MSG("doc_buffer_to_textbox, buffer==NULL, returning\n");
		return FALSE;
	}

	if (!enable_undo) {
		doc_unbind_signals(doc);
	}
	/* now get the current cursor position */
	insert = gtk_text_buffer_get_insert(doc->buffer);
	gtk_text_buffer_get_iter_at_mark(doc->buffer, &iter, insert);
	cursor_offset = gtk_text_iter_get_offset(&iter);

	/* This opens the contents of a file to a textbox */

	newbuf = buffer_find_encoding(buffer, buflen, &encoding,BFWIN(doc->bfwin)->session->encoding);

	if (!newbuf) {
		message_dialog_new(BFWIN(doc->bfwin)->main_window,
								 GTK_MESSAGE_ERROR,
								 GTK_BUTTONS_CLOSE,
								 _("Cannot display file, unknown characters found."),
								 NULL);
		return FALSE;
	}
	DEBUG_MSG("doc_buffer_to_textbox, will set encoding to %s\n",encoding);
	if (doc->encoding) g_free(doc->encoding);
	doc->encoding = encoding;
	add_encoding_to_list(encoding);
	gtk_text_buffer_insert_at_cursor(doc->buffer,newbuf,-1);
	g_free(newbuf);
#ifndef USE_BFTEXTVIEW2
	if (BF_TEXTVIEW(doc->view)->highlight) {
		doc->need_highlighting=TRUE;
		DEBUG_MSG("doc_buffer_to_textbox, highlight=%d, need_highlighting=%d, delay=%d\n",BF_TEXTVIEW(doc->view)->highlight,doc->need_highlighting,delay);
	}
#endif
	if (!enable_undo) {
		doc_bind_signals(doc);
	}

	{
		/* set the cursor position back */
		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_offset(doc->buffer,&iter,cursor_offset);
		gtk_text_buffer_place_cursor(doc->buffer,&iter);
		if (!delay) {
			gtk_text_view_place_cursor_onscreen(GTK_TEXT_VIEW(doc->view));
		}
	}
	return TRUE;
}
/**
 * doc_file_to_textbox:
 * @doc: The #Tdocument target.
 * @filename: Filename to read in.
 * @enable_undo: #gboolean
 * @delay: Whether to delay GUI-calls.
 *
 * Open and read in a file to the doc buffer.
 * The data is inserted starting at the current cursor position.
 * Charset is detected, and highlighting performed (if applicable).
 *
 * Return value: A #gboolean, TRUE if successful, FALSE on error.
 ** /
gboolean doc_file_to_textbox(Tdocument *doc, gchar *filename, gboolean enable_undo, gboolean delay) {
	gchar *buffer, *message;
	int document_size=0;
	gboolean ret;
	message = g_strconcat(_("Opening file "), filename, NULL);
	statusbar_message(BFWIN(doc->bfwin),message, 1000);
	g_free(message);

	buffer = get_buffer_from_filename(BFWIN(doc->bfwin), filename, &document_size);
	ret = doc_buffer_to_textbox(doc, buffer, document_size, enable_undo, delay);
	g_free(buffer);
	return ret;
}*/
/* code moved to file.c 
void doc_set_fileinfo(Tdocument *doc, GFileInfo *finfo) {
	DEBUG_MSG("doc_set_fileinfo, doc=%p, new finfo=%p, old fileinfo=%p\n",doc,finfo,doc->fileinfo);
	if (doc->fileinfo) {
		DEBUG_MSG("doc_set_fileinfo, unref doc->fileinfo at %p\n",doc->fileinfo);
		g_object_unref(doc->fileinfo);
		doc->fileinfo = NULL;
	}

	if (finfo != NULL) {
		g_object_ref(finfo);
		doc->fileinfo = finfo;
	}

	doc_set_tooltip(doc);
}*/

/**
 * doc_check_backup:
 * @doc: #Tdocument*
 *
 * creates a backup, depending on the configuration
 * returns 1 on success, 0 on failure
 * if no backup is required, or no filename known, 1 is returned
 *
 * Return value: #gint 1 on success or 0 on failure
 * /
static gint doc_check_backup(Tdocument *doc) {
	gint res = 1;

	if (main_v->props.backup_file && doc->uri && file_exists_and_readable(doc->uri)) {
		gchar *backupfilename;
		backupfilename = g_strconcat(doc->uri, main_v->props.backup_filestring, NULL);
		res = file_copy(doc->uri, backupfilename);
		if (doc->fileinfo) {
			gnome_vfs_set_file_info(backupfilename, doc->fileinfo, GNOME_VFS_SET_FILE_INFO_PERMISSIONS|GNOME_VFS_SET_FILE_INFO_OWNER);
		}
		g_free(backupfilename);
	}
	return res;
}*/

static void doc_buffer_insert_text_lcb(GtkTextBuffer *textbuffer,GtkTextIter * iter,gchar * string,gint len, Tdocument * doc) {
	gint pos = gtk_text_iter_get_offset(iter);
	gint clen = g_utf8_strlen(string, len);
	DEBUG_MSG("doc_buffer_insert_text_lcb, started, string='%s', len=%d, clen=%d\n", string, len, clen);
	/* the 'len' seems to be the number of bytes and not the number of characters.. */

/*	if (doc->paste_operation) {
		if ((pos + clen) > PASTEOPERATION(doc->paste_operation)->eo) PASTEOPERATION(doc->paste_operation)->eo = pos + clen;
		if (pos < PASTEOPERATION(doc->paste_operation)->so || PASTEOPERATION(doc->paste_operation)->so == -1) PASTEOPERATION(doc->paste_operation)->so = pos;
	} else if (len == 1) {*/
		/* undo_redo stuff */
		if (	!doc_unre_test_last_entry(doc, UndoInsert, -1, pos)
				|| string[0] == ' '
				|| string[0] == '\n'
				|| string[0] == '\t'
				|| string[0] == '\r') {
			DEBUG_MSG("doc_buffer_insert_text_lcb, need a new undogroup\n");
			doc_unre_new_group(doc);
		}
/*	}*/
	/* we do not call doc_unre_new_group() for multi character inserts, these are from paste, and edit_paste_cb groups them already */
	/*  else if (clen != 1) {
		doc_unre_new_group(doc);
	} */

	doc_unre_add(doc, string, pos, pos+clen, UndoInsert);
	doc_set_modified(doc, 1);
	DEBUG_MSG("doc_buffer_insert_text_lcb, done\n");
}
/*
static gboolean find_char(gunichar ch,gchar *data) {
#ifdef DEBUG
	if (ch < 127) {
		DEBUG_MSG("find_char, looking at character %c, searching for '%s', returning %d\n",ch,data,(strchr(data, ch) != NULL));
	} else {
		DEBUG_MSG("find_char, looking at character code %d, searching for '%s', returning %d\n",ch,data,(strchr(data, ch) != NULL));
	}
#endif
	return (strchr(data, ch) != NULL);
}

static gchar *closingtagtoinsert(Tdocument *doc, const gchar *tagname, GtkTextIter *iter) {
	return NULL;
}
*/
/*
static void doc_buffer_insert_text_after_lcb(GtkTextBuffer *textbuffer,GtkTextIter * iter,gchar * string,gint len, Tdocument * doc) {
	DEBUG_MSG("doc_buffer_insert_text_after_lcb, started for string '%s'\n",string);
	if (!doc->paste_operation) {
	}
#ifdef DEBUG
	else {
		DEBUG_MSG("doc_buffer_insert_text_after_lcb, paste_operation, NOT DOING ANYTHING\n");
	}
#endif
}*/

static gboolean doc_view_key_press_lcb(GtkWidget *widget,GdkEventKey *kevent,Tdocument *doc) {
	DEBUG_MSG("doc_view_key_press_lcb, keyval=%d, hardware_keycode=%d\n",kevent->keyval, kevent->hardware_keycode);
#ifndef USE_BFTEXTVIEW2
	main_v->lastkp_keyval = kevent->keyval;
	main_v->lastkp_hardware_keycode = kevent->hardware_keycode;
#endif
	if (!(kevent->state & GDK_CONTROL_MASK) &&
	       ((kevent->keyval == GDK_Home) || (kevent->keyval == GDK_KP_Home) || (kevent->keyval == GDK_End) || (kevent->keyval == GDK_KP_End)) &&
	       main_v->props.editor_smart_cursor) {
                GtkTextMark* imark;
                GtkTextIter  iter, currentpos, linestart;

        		imark = gtk_text_buffer_get_insert (doc->buffer);
        		gtk_text_buffer_get_iter_at_mark (doc->buffer, &currentpos, imark);

        		iter = currentpos;

        		if ((kevent->keyval == GDK_Home) || (kevent->keyval == GDK_KP_Home)) {
                    gtk_text_iter_backward_cursor_positions (&iter, gtk_text_iter_get_line_offset (&iter));
                    linestart = iter;

                    while (g_unichar_isspace (gtk_text_iter_get_char (&iter)) && !gtk_text_iter_ends_line (&iter))
                            gtk_text_iter_forward_char (&iter);

                } else { /* (kevent->keyval == GDK_End) || (kevent->keyval == GDK_KP_End) */
		            if (!gtk_text_iter_ends_line (&iter))
		                gtk_text_iter_forward_to_line_end (&iter);

		            linestart = iter;

                    if (gtk_text_iter_is_end (&iter) && !gtk_text_iter_starts_line (&iter))
                        gtk_text_iter_backward_char (&iter);

		            while (g_unichar_isspace (gtk_text_iter_get_char (&iter)) && !gtk_text_iter_starts_line (&iter))
			                gtk_text_iter_backward_char (&iter);

                    if ((!gtk_text_iter_starts_line (&iter) || !gtk_text_iter_ends_line (&iter)) && !g_unichar_isspace (gtk_text_iter_get_char (&iter)))
                        gtk_text_iter_forward_char (&iter);
                }

        		if (gtk_text_iter_compare (&currentpos, &iter) == 0)
        			iter = linestart;

        		if (kevent->state & GDK_SHIFT_MASK)
        			gtk_text_buffer_move_mark (doc->buffer, imark, &iter);
        		else {
        			gtk_text_buffer_place_cursor (doc->buffer, &iter);
        		}

                gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (doc->view), gtk_text_buffer_get_insert (doc->buffer));

        		return TRUE;
	}

	if (kevent->keyval == GDK_Tab && main_v->props.editor_indent_wspaces) {
		GtkTextMark* imark;
		GtkTextIter iter;
		gchar *string;
		/* replace the tab with spaces if the user wants that */
		string = bf_str_repeat(" ", main_v->props.editor_tab_width);
		imark = gtk_text_buffer_get_insert(doc->buffer);
		gtk_text_buffer_get_iter_at_mark(doc->buffer,&iter,imark);
		gtk_text_buffer_insert(doc->buffer,&iter,string,main_v->props.editor_tab_width);
		g_free(string);
		return TRUE; /* we handled the event, stop the event from cascading further */
	}
	return FALSE; /* we didn't handle all of the event */
}
#ifndef USE_BFTEXTVIEW2
static gboolean doc_view_key_release_lcb(GtkWidget *widget,GdkEventKey *kevent,Tdocument *doc) {
#ifdef DEBUG
	guint32 character = gdk_keyval_to_unicode(kevent->keyval);
	DEBUG_MSG("doc_view_key_release_lcb, started for keyval=%d (or %X), character=%d, string=%s, state=%d, hw_keycode=%d\n",kevent->keyval,kevent->keyval, character,kevent->string,kevent->state, kevent->hardware_keycode);
#endif
	/* if the shift key is released before the '>' key, we get a key release not for '>' but for '.'. We, therefore
	 have set that in the key_press event, and check if the same hardware keycode was released */
#ifndef GNOMEVFSINT
	if ((kevent->keyval == GDK_greater) || (kevent->hardware_keycode == main_v->lastkp_hardware_keycode && main_v->lastkp_keyval == GDK_greater)) {
		if (doc->autoclosingtag) {
			/* start the autoclosing! the code is modified from the patch sent by more <more@irpin.com> because that
			 * patch did not work with php code (the < and > characters can be inside a php block as well with a
			 * different meaning then a tag), it could not do closing of XML tags and it was limited to a buffer
			 * in Tdocument to hold the current tag name.
			 * This code will simply look back in the buffer once a '>' character is pressed, and look if that was
			 * the end of a tag. If so it will insert the closing tag for that same tag. Works for XML and HTML. For
			 * HTML we need an exception, since <br> and such don't need a closing tag */
			GtkTextMark* imark;
			GtkTextIter itstart,iter,maxsearch;

			imark = gtk_text_buffer_get_insert(doc->buffer);
			gtk_text_buffer_get_iter_at_mark(doc->buffer,&iter,imark);
			itstart = iter;
			maxsearch = iter;
			DEBUG_MSG("doc_view_key_release_lcb, autoclosing, started at %d\n",gtk_text_iter_get_offset(&itstart));
			gtk_text_iter_backward_chars(&maxsearch,250);
			if (gtk_text_iter_backward_find_char(&itstart,(GtkTextCharPredicate)find_char,GINT_TO_POINTER("<"),&maxsearch)) {
				/* we use a regular expression to check if the tag is valid, AND to parse the tagname from the string */
				gchar *buf;
				int ovector[30], ret;
				DEBUG_MSG("doc_view_key_release_lcb, we found a '<'\n");
				maxsearch = iter; /* re-use maxsearch */
				buf = gtk_text_buffer_get_text(doc->buffer,&itstart,&maxsearch,TRUE);
				DEBUG_MSG("doc_view_key_release_lcb, buf='%s'\n",buf);
				ret = pcre_exec(main_v->autoclosingtag_regc, NULL, buf, strlen(buf), 0,PCRE_ANCHORED, ovector, 30);
				if (ret > 0) {
					gchar *tagname, *toinsert;
					DEBUG_MSG("doc_view_key_release_lcb, autoclosing, we have a tag, ret=%d, starts at ovector[2]=%d, ovector[3]=%d\n",ret, ovector[2], ovector[3]);
					tagname = g_strndup(&buf[ovector[2]], ovector[3]-ovector[2]);
					DEBUG_MSG("doc_view_key_release_lcb, autoclosing, tagname='%s'\n",tagname);
					toinsert = closingtagtoinsert(doc, tagname, &iter);
					if (toinsert) {
						/* we place this in a new undo/redo group */
						doc_unre_new_group(doc);
						/* we re-use the maxsearch iter now */
						gtk_text_buffer_insert(doc->buffer,&maxsearch,toinsert,-1);
						/* now we set the cursor back to its previous location, re-using itstart */
						gtk_text_buffer_get_iter_at_mark(doc->buffer,&itstart,gtk_text_buffer_get_insert(doc->buffer));
						gtk_text_iter_backward_chars(&itstart,strlen(toinsert));
						gtk_text_buffer_place_cursor(doc->buffer,&itstart);
						g_free(toinsert);
						doc_unre_new_group(doc);
					}
#ifdef DEBUG
					 else {
						DEBUG_MSG("doc_view_key_release_lcb, no match!! '%s' is not a valid tag\n",buf);
					}
#endif
					g_free(tagname);
				} else {
					DEBUG_MSG("doc_view_key_release_lcb, ret=%d\n",ret);
				}
				/* cleanup and return */
				g_free(buf);
			}
#ifdef DEBUG
			 else {
				DEBUG_MSG("doc_view_key_release_lcb, did not find a '<' character\n");
			}
#endif
		}
	} else 
#endif	
	if ((kevent->keyval == GDK_Return || kevent->keyval == GDK_KP_Enter) && !(kevent->state & GDK_SHIFT_MASK || kevent->state & GDK_CONTROL_MASK || kevent->state & GDK_MOD1_MASK)) {
		if (main_v->props.autoindent) {
			gchar *string, *indenting;
			GtkTextMark* imark;
			GtkTextIter itstart, itend;
			imark = gtk_text_buffer_get_insert(doc->buffer);
			gtk_text_buffer_get_iter_at_mark(doc->buffer,&itend,imark);
			itstart = itend;
			/* set to the beginning of the previous line */
			gtk_text_iter_backward_line(&itstart);
			gtk_text_iter_set_line_index(&itstart, 0);
			string = gtk_text_buffer_get_text(doc->buffer,&itstart,&itend,TRUE);
			if (string) {
				/* now count the indenting in this string */
				indenting = string;
				while (*indenting == '\t' || *indenting == ' ') {
					indenting++;
				}
				/* ending search, non-whitespace found, so terminate at this position */
				*indenting = '\0';
				if (strlen(string)) {
					DEBUG_MSG("doc_view_key_release_lcb, inserting indenting\n");
					gtk_text_buffer_insert(doc->buffer,&itend,string,-1);
				}
				g_free(string);
			}
		}
	}
	return FALSE; /* we didn't handle all of the event */
}
#endif

static void doc_buffer_delete_range_lcb(GtkTextBuffer *textbuffer,GtkTextIter * itstart,GtkTextIter * itend, Tdocument * doc) {
	gchar *string;
	string = gtk_text_buffer_get_text(doc->buffer, itstart, itend, TRUE);
	DEBUG_MSG("doc_buffer_delete_range_lcb, string='%s'\n",string);
	if (string) {
		/* undo_redo stuff */
		{
			gint start, end, len;
			start = gtk_text_iter_get_offset(itstart);
			end = gtk_text_iter_get_offset(itend);
			len = end - start;
			DEBUG_MSG("doc_buffer_delete_range_lcb, start=%d, end=%d, len=%d, string='%s'\n", start, end, len, string);
			if (len == 1) {
				if (		(!doc_unre_test_last_entry(doc, UndoDelete, start, -1) /* delete */
							&& !doc_unre_test_last_entry(doc, UndoDelete, end, -1)) /* backspace */
						|| string[0] == ' '
						|| string[0] == '\n'
						|| string[0] == '\t'
						|| string[0] == '\r') {
					DEBUG_MSG("doc_buffer_delete_range_lcb, need a new undogroup\n");
					doc_unre_new_group(doc);
				}
			} else /*if (!doc->paste_operation)*/ {
				doc_unre_new_group(doc);
			}
			doc_unre_add(doc, string, start, end, UndoDelete);
		}
		g_free(string);
	}
	doc_set_modified(doc, 1);
}
#ifndef USE_BFTEXTVIEW2
static gboolean doc_view_button_release_lcb(GtkWidget *widget,GdkEventButton *bevent, Tdocument *doc) {
	DEBUG_MSG("doc_view_button_release_lcb, button %d\n", bevent->button);
	if (bevent->button==2) {
		/* end of paste */
		if (doc->paste_operation) {
			if (PASTEOPERATION(doc->paste_operation)->eo > PASTEOPERATION(doc->paste_operation)->so) {
				DEBUG_MSG("doc_view_button_release_lcb, start doc-highlight_region for so=%d, eo=%d\n",PASTEOPERATION(doc->paste_operation)->so,PASTEOPERATION(doc->paste_operation)->eo);
			}
			g_free(doc->paste_operation);
			doc->paste_operation = NULL;
			BF_TEXTVIEW(doc->view)->paste_operation = FALSE;
		}
		/* now we should update the highlighting for the pasted text, but how long is the pasted text ?? */
	}
/*	if (bevent->button == 3) {
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
	} */
	return FALSE;
}
#endif

void doc_get_iter_location(Tdocument *doc, GtkTextIter *iter, GdkRectangle *rectangle) {
	GdkRectangle rect;
	gint itx,ity,px,py;

	gtk_text_view_get_iter_location(GTK_TEXT_VIEW(doc->view),iter,&rect);

	/* the following function will return the position relative to the text area of the widget
	but we also have margins!! */
	gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(doc->view)
					, GTK_TEXT_WINDOW_TEXT
					, rect.x,rect.y,&itx,&ity);
	/* the following function will return the position of the total text widget */
	gdk_window_get_origin(doc->view->window,&px,&py);

	DEBUG_MSG("doc_get_iter_location, px=%d, itx=%d,border=%d\n",px,itx,gtk_text_view_get_border_window_size(GTK_TEXT_VIEW(doc->view),GTK_TEXT_WINDOW_LEFT));
	DEBUG_MSG("doc_get_iter_location, py=%d, ity=%d,border=%d\n",py,ity,gtk_text_view_get_border_window_size(GTK_TEXT_VIEW(doc->view),GTK_TEXT_WINDOW_TOP));
	rectangle->x = px+itx+gtk_text_view_get_border_window_size(GTK_TEXT_VIEW(doc->view),GTK_TEXT_WINDOW_LEFT);
	rectangle->y = py+ity+gtk_text_view_get_border_window_size(GTK_TEXT_VIEW(doc->view),GTK_TEXT_WINDOW_TOP);
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
void doc_get_iter_at_bevent(Tdocument *doc, GdkEventButton *bevent, GtkTextIter *iter) {
	gint xpos, ypos;
	GtkTextWindowType wintype;

	wintype = gtk_text_view_get_window_type(GTK_TEXT_VIEW(doc->view), doc->view->window);
	gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(doc->view), wintype,bevent->x, bevent->y,
					  &xpos, &ypos);
	xpos += gtk_text_view_get_border_window_size(GTK_TEXT_VIEW(doc->view),GTK_TEXT_WINDOW_LEFT);
	gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(doc->view), iter, xpos, ypos);
}

static gboolean doc_view_button_press_lcb(GtkWidget *widget,GdkEventButton *bevent, Tdocument *doc) {
	DEBUG_MSG("doc_view_button_press_lcb, button %d\n", bevent->button);
#ifndef USE_BFTEXTVIEW2
	if (bevent->button==2 && !doc->paste_operation) {
		doc->paste_operation = g_new(Tpasteoperation,1);
		PASTEOPERATION(doc->paste_operation)->so = -1;
		PASTEOPERATION(doc->paste_operation)->eo = -1;
		BF_TEXTVIEW(doc->view)->paste_operation = TRUE;
	}
#endif
	if (bevent->button == 3) {
		GtkTextIter iter;
		doc_get_iter_at_bevent(doc, bevent, &iter);
		bmark_store_bevent_location(doc, gtk_text_iter_get_offset(&iter));
	}
/* here we ask any plugins to do any processing */
	if (main_v->doc_view_button_press_cbs) {
		GSList *tmplist = main_v->doc_view_button_press_cbs;
		while (tmplist) {
			void *(* func)() = tmplist->data;
			DEBUG_MSG("doc_view_button_press_lcb, calling plugin func %p\n", tmplist->data);
			func(widget,bevent,doc);
			tmplist = g_slist_next(tmplist);
		}
	}
	return FALSE;
}

static void rpopup_add_bookmark_lcb(GtkWidget *widget, Tdocument *doc) {
	bmark_add_at_bevent(doc);
}
static void rpopup_del_bookmark_lcb(GtkWidget *widget, Tdocument *doc) {
	bmark_del_at_bevent(doc);
}


static void doc_view_populate_popup_lcb(GtkTextView *textview,GtkMenu *menu,Tdocument *doc) {
	GtkWidget *menuitem;
	/* I found no way to connect an item-factory to this menu widget, so we have to do it in the manual way... */
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(gtk_menu_item_new()));

	menuitem = gtk_image_menu_item_new_with_label(_("Replace"));
	g_signal_connect(menuitem, "activate", G_CALLBACK(replace_cb), doc->bfwin);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),gtk_image_new_from_stock(GTK_STOCK_FIND_AND_REPLACE, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));

	menuitem = gtk_image_menu_item_new_with_label(_("Find"));
	g_signal_connect(menuitem, "activate", G_CALLBACK(search_cb), doc->bfwin);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));

	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(gtk_menu_item_new()));

	if (bmark_have_bookmark_at_stored_bevent(doc)) {
		menuitem = gtk_menu_item_new_with_label(_("Delete bookmark"));
		g_signal_connect(menuitem, "activate", G_CALLBACK(rpopup_del_bookmark_lcb), doc);
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));
	} else {
		menuitem = gtk_menu_item_new_with_label(_("Add bookmark"));
		g_signal_connect(menuitem, "activate", G_CALLBACK(rpopup_add_bookmark_lcb), doc);
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
			void *(* func)() = tmplist->data;
			func(textview,menu,doc);
			tmplist = g_slist_next(tmplist);
		}
	}

	gtk_widget_show_all(GTK_WIDGET(menu));
}
#ifdef USER_IDLE_TIMER
static gboolean user_idle_timer_lcb(gpointer data) {
	Tbfwin *bfwin = BFWIN(data);
	if (g_timer_elapsed(bfwin->idletimer,NULL) > 0.48) {
		DEBUG_MSG("user_idle_timer_lcb, half a second no user action, time to check auto-completion and stuff lile that!\n");

		/* TODO: here we can check for things like auto-completion.. */
		/*bf_textview_autocomp_show(bfwin->current_document->view);*/


		g_timer_start(bfwin->idletimer);
	}
	return FALSE;
}
static void reset_user_idle_timer(Tbfwin *bfwin) {
	g_timer_start(bfwin->idletimer);
	g_timeout_add(500,user_idle_timer_lcb,bfwin);
}
#endif

static void doc_buffer_mark_set_lcb(GtkTextBuffer *buffer,GtkTextIter *iter,GtkTextMark *set_mark,Tdocument *doc) {
	/*DEBUG_MSG("doc_buffer_mark_set_lcb, set_mark=%p, insert_mark=%p\n",set_mark,gtk_text_buffer_get_insert(buffer));*/
	if (set_mark == gtk_text_buffer_get_insert(buffer)) {
		doc_set_statusbar_lncol(doc);
#ifdef USER_IDLE_TIMER
		/* reset the timer */
		reset_user_idle_timer(BFWIN(doc->bfwin));
#endif
	}
}
static void doc_buffer_changed_lcb(GtkTextBuffer *textbuffer,Tdocument*doc) {
	DEBUG_MSG("doc_buffer_changed_lcb()\n");
	doc_set_statusbar_lncol(doc);
#ifdef USER_IDLE_TIMER
	reset_user_idle_timer(BFWIN(doc->bfwin));
#endif
}

static void doc_view_toggle_overwrite_lcb(GtkTextView *view, Tdocument *doc)
{
	doc->overwrite_mode = (doc->overwrite_mode ? FALSE : TRUE);
	doc_set_statusbar_insovr(doc);
}

/**
 * doc_bind_signals:
 * @doc: a #Tdocument
 *
 * Bind signals related to the doc's buffer:
 * "insert-text", "delete-range" and "insert-text" for autoindent
 *
 * Return value: void
 **/
void doc_bind_signals(Tdocument *doc) {
	doc->ins_txt_id = g_signal_connect(G_OBJECT(doc->buffer),
					 "insert-text",
					 G_CALLBACK(doc_buffer_insert_text_lcb), doc);
	doc->del_txt_id = g_signal_connect(G_OBJECT(doc->buffer),
					 "delete-range",
					 G_CALLBACK(doc_buffer_delete_range_lcb), doc);
/*	doc->ins_aft_txt_id = g_signal_connect_after(G_OBJECT(doc->buffer),
					 "insert-text",
					 G_CALLBACK(doc_buffer_insert_text_after_lcb), doc);*/
}

/**
 * doc_unbind_signals:
 * @doc: a #Tdocument
 *
 * Unbind signals related to the doc's buffer:
 * "insert-text", "delete-range" and "insert-text" for autoindent.
 * This function checks if each individual signal has been bound before unbinding.
 *
 * Return value: void
 **/
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
/*	if (doc->ins_aft_txt_id != 0) {
		g_signal_handler_disconnect(G_OBJECT(doc->buffer),doc->ins_aft_txt_id);
		doc->ins_txt_id = 0;
	}*/
}

/*
returns a buffer in the encoding stored in doc->encoding, or NULL if that fails
and the user aborted conversion to UTF-8
*/
gchar *doc_get_buffer_in_encoding(Tdocument *doc) {
	GtkTextIter itstart, itend;
	gchar *buffer;

	gtk_text_buffer_get_bounds(doc->buffer,&itstart,&itend);
	buffer = gtk_text_buffer_get_text(doc->buffer,&itstart,&itend,TRUE);

	if (doc->encoding) {
		gchar *newbuf;
		gsize bytes_written=0, bytes_read=0;
		DEBUG_MSG("doc_get_buffer_in_encoding, converting from UTF-8 to %s\n", doc->encoding);
		newbuf = g_convert(buffer,-1,doc->encoding,"UTF-8",&bytes_read,&bytes_written,NULL);
		if (newbuf) {
			g_free(buffer);
			buffer = newbuf;
		} else {
			const gchar *buttons[] = {_("_Abort save"), _("_Continue save in UTF-8"), NULL};
			gint retval, line, column;
			glong position;
			gchar *tmpstr, failed[6];
			GtkTextIter iter;
			position = g_utf8_pointer_to_offset(buffer,buffer+bytes_read);
			gtk_text_buffer_get_iter_at_offset(doc->buffer,&iter,position);
			line = gtk_text_iter_get_line(&iter);
			column = gtk_text_iter_get_line_offset(&iter);
			failed[0]='\0';
			g_utf8_strncpy(failed,buffer+bytes_read,1);
			tmpstr = g_strdup_printf(_("Failed to convert %s to character encoding %s. Encoding failed on character '%s' at line %d column %d\n\nContinue saving in UTF-8 encoding?"), gtk_label_get_text(GTK_LABEL(doc->tab_menu)), doc->encoding, failed, line+1, column+1);
			retval = message_dialog_new_multi(BFWIN(doc->bfwin)->main_window,
														 GTK_MESSAGE_WARNING,
														 buttons,
														 _("File encoding conversion failure"),
														 tmpstr);
			g_free(tmpstr);
			if (retval == 0) {
				DEBUG_MSG("doc_get_buffer_in_encoding, character set conversion failed, user aborted!\n");
				return NULL;
			} else {
				/* continue in UTF-8 */
				update_encoding_meta_in_file(doc, "UTF-8");
				g_free(buffer);
				gtk_text_buffer_get_bounds(doc->buffer,&itstart,&itend);
				buffer = gtk_text_buffer_get_text(doc->buffer,&itstart,&itend,TRUE);
			}
		}
	}
	return  buffer;
}

/**
 * gint doc_textbox_to_file
 * @doc: a #Tdocument*
 * @filename: a #gchar*
 * @window_closing: a #gboolean if the window is closing, we should supress any statusbar messages then
 *
 * If applicable, backup existing file,
 * possibly update meta-tags (HTML),
 * and finally write the document to the specified file.
 *
 * Return value: #gint set to
 * 1: on success
 * 2: on success but the backup failed
 * -1: if the backup failed and save was aborted
 * -2: if the file could not be opened or written
 * -3: if the backup failed and save was aborted by the user
 * -4: if the charset encoding conversion failed and the save was aborted by the user
 **
gint doc_textbox_to_file(Tdocument * doc, gchar * filename, gboolean window_closing) {
	gint backup_retval;
	gint write_retval;
	gchar *buffer;
	GtkTextIter itstart, itend;

	if (!window_closing) statusbar_message(BFWIN(doc->bfwin),_("Saving file"), 1000);
	if (main_v->props.auto_update_meta) {
		const gchar *realname = g_get_real_name();
		if (realname && strlen(realname) > 0)  {
			gchar *tmp;
			Tsearch_result res = doc_search_run_extern(doc,"<meta[ \t\n]name[ \t\n]*=[ \t\n]*\"generator\"[ \t\n]+content[ \t\n]*=[ \t\n]*\"[^\"]*bluefish[^\"]*\"[ \t\n]*>",1,0);
			if (res.end > 0) {
				snr2_run_extern_replace(doc,"<meta[ \t\n]name[ \t\n]*=[ \t\n]*\"generator\"[ \t\n]+content[ \t\n]*=[ \t\n]*\"[^\"]*\"[ \t\n]*>",0,1,0,"<meta name=\"generator\" content=\"Bluefish, see http://bluefish.openoffice.nl/\">", FALSE);
			}
			tmp = g_strconcat("<meta name=\"author\" content=\"",realname,"\">",NULL);
			snr2_run_extern_replace(doc,"<meta[ \t\n]name[ \t\n]*=[ \t\n]*\"author\"[ \t\n]+content[ \t\n]*=[ \t\n]*\"[^\"]*\"[ \t\n]*>",0,1,0,tmp,FALSE);
			g_free(tmp);
		}
	}

	/ * This writes the contents of a textbox to a file * /
	backup_retval = doc_check_backup(doc);

	if (!backup_retval) {
		if (main_v->props.backup_abort_action == DOCUMENT_BACKUP_ABORT_ABORT) {
			DEBUG_MSG("doc_textbox_to_file, backup failure, abort!\n");
			return -1;
		} else if (main_v->props.backup_abort_action == DOCUMENT_BACKUP_ABORT_ASK) {
			gchar *options[] = {_("_Abort save"), _("_Continue save"), NULL};
			gint retval;
			gchar *tmpstr = g_strdup_printf(_("A backupfile for %s could not be created. If you continue, this file will be overwritten."), filename);
			retval = multi_warning_dialog(BFWIN(doc->bfwin)->main_window,_("File backup failure"), tmpstr, 1, 0, options);
			g_free(tmpstr);
			if (retval == 0) {
				DEBUG_MSG("doc_textbox_to_file, backup failure, user aborted!\n");
				return -3;
			}
		}
	}

	gtk_text_buffer_get_bounds(doc->buffer,&itstart,&itend);
	buffer = gtk_text_buffer_get_text(doc->buffer,&itstart,&itend,TRUE);

	if (doc->encoding) {
		gchar *newbuf;
		gsize bytes_written=0, bytes_read=0;
		DEBUG_MSG("doc_textbox_to_file, converting from UTF-8 to %s\n", doc->encoding);
		newbuf = g_convert(buffer,-1,doc->encoding,"UTF-8",&bytes_read,&bytes_written,NULL);
		if (newbuf) {
			g_free(buffer);
			buffer = newbuf;
		} else {
			gchar *options[] = {_("_Abort save"), _("_Continue save in UTF-8"), NULL};
			gint retval, line, column;
			glong position;
			gchar *tmpstr, failed[6];
			GtkTextIter iter;
			position = g_utf8_pointer_to_offset(buffer,buffer+bytes_read);
			gtk_text_buffer_get_iter_at_offset(doc->buffer,&iter,position);
			line = gtk_text_iter_get_line(&iter);
			column = gtk_text_iter_get_line_offset(&iter);
			failed[0]='\0';
			g_utf8_strncpy(failed,buffer+bytes_read,1);
			tmpstr = g_strdup_printf(_("Failed to convert %s to character encoding %s. Encoding failed on character '%s' at line %d column %d\n\nContinue saving in UTF-8 encoding?"), filename, doc->encoding, failed, line+1, column+1);
			retval = multi_warning_dialog(BFWIN(doc->bfwin)->main_window,_("File encoding conversion failure"), tmpstr, 1, 0, options);
			g_free(tmpstr);
			if (retval == 0) {
				DEBUG_MSG("doc_textbox_to_file, character set conversion failed, user aborted!\n");
				return -4;
			} else {
				/ * continue in UTF-8 * /
				update_encoding_meta_in_file(doc, "UTF-8");
				g_free(buffer);
				gtk_text_buffer_get_bounds(doc->buffer,&itstart,&itend);
				buffer = gtk_text_buffer_get_text(doc->buffer,&itstart,&itend,TRUE);
			}
		}
	}

	write_retval = buffer_to_file(BFWIN(doc->bfwin), buffer, filename);
	DEBUG_MSG("doc_textbox_to_file, write_retval=%d\n",write_retval);
	g_free(buffer);
	if (!write_retval) {
		return -2;
	}

	if (main_v->props.clear_undo_on_save) {
		doc_unre_clear_all(doc);
	}
	DEBUG_MSG("doc_textbox_to_file, calling doc_set_modified(doc, 0)\n");
	doc_set_modified(doc, 0);
	if (!backup_retval) {
		return 2;
	} else {
		return 1;
	}
}
*/

static void delete_backupfile_lcb(gpointer data) {
	GFile *uri = (GFile *)data;
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
void doc_destroy(Tdocument * doc, gboolean delay_activation) {
	Tbfwin *bfwin = BFWIN(doc->bfwin);

	if (doc->status == DOC_STATUS_ERROR) {
		bfwin_docs_not_complete(doc->bfwin, FALSE);
	}

	DEBUG_MSG("doc_destroy, calling bmark_clean_for_doc(%p)\n",doc);
	bmark_clean_for_doc(doc);
/*        bmark_adjust_visible(bfwin);   */

	if (doc->uri) {
		gchar *curi = g_file_get_parse_name(doc->uri);
		add_to_recent_list(doc->bfwin,curi, 1, FALSE);
		g_free(curi);
	}
	gui_notebook_unbind_signals(BFWIN(doc->bfwin));
	/* to make this go really quick, we first only destroy the notebook page and run flush_queue(),
	after the document is gone from the GUI we complete the destroy, to destroy only the notebook
	page we ref+ the scrolthingie, remove the page, and unref it again */
	g_object_ref(doc->view->parent);
	if (doc->floatingview) {
		gtk_widget_destroy(FLOATINGVIEW(doc->floatingview)->window);
		doc->floatingview = NULL;
	}
	/* now we remove the document from the document list */
	bfwin->documentlist = g_list_remove(bfwin->documentlist, doc);
	DEBUG_MSG("removed %p from documentlist, list %p length=%d\n",doc
			, bfwin->documentlist
			, g_list_length(bfwin->documentlist));
	if (bfwin->current_document == doc) {
		bfwin->current_document = NULL;
	}
	/* then we remove the page from the notebook */
	DEBUG_MSG("about to remove widget from notebook (doc=%p, current_document=%p)\n",doc,bfwin->current_document);
	gtk_notebook_remove_page(GTK_NOTEBOOK(bfwin->notebook),
							 gtk_notebook_page_num(GTK_NOTEBOOK(bfwin->notebook),doc->view->parent));
	DEBUG_MSG("doc_destroy, removed widget from notebook (doc=%p), delay_activation=%d\n",doc,delay_activation);
	DEBUG_MSG("doc_destroy, (doc=%p) about to bind notebook signals...\n",doc);
	gui_notebook_bind_signals(BFWIN(doc->bfwin));
	if (!delay_activation) {
		notebook_changed(BFWIN(doc->bfwin),-1);
	}
	DEBUG_MSG("doc_destroy, (doc=%p) after calling notebook_changed()\n",doc);
	/* now we really start to destroy the document, Ky Anh: "we should use gtk_object_sink and not g_object_unref" */
	gtk_object_sink(GTK_OBJECT(doc->view->parent));

	if (doc->uri) {
		if (main_v->props.backup_cleanuponclose) {
			gchar *tmp, *tmp2;
			GFile *backupuri;
			tmp = g_file_get_uri(doc->uri);
			tmp2 = g_strconcat(tmp,"~",NULL);
			backupuri = g_file_new_for_uri(tmp2);
			g_free(tmp);
			g_free(tmp2);
			file_delete_file_async(backupuri, delete_backupfile_lcb, backupuri);
			g_object_unref(backupuri);
		}
		g_object_unref(doc->uri);
	}

	if (doc->encoding)
		g_free(doc->encoding);

	if (doc->fileinfo) {
		g_object_unref (doc->fileinfo);
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
void document_unset_filename(Tdocument *doc) {
	if (doc->uri) {
		gchar *tmpstr;
		tmpstr = g_strconcat(_("Previously: "), gtk_label_get_text(GTK_LABEL(doc->tab_label)), NULL);

		g_object_unref(doc->uri);
		doc->uri = NULL;
		if (doc->fileinfo) {
			g_object_unref(doc->fileinfo);
			doc->fileinfo = NULL;
		}
		doc_set_title(doc);
		doc_set_modified(doc, TRUE);

		gtk_label_set(GTK_LABEL(doc->tab_label),tmpstr);

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

static void doc_close_but_clicked_lcb(GtkWidget *wid, gpointer data) {
	/*doc_close(data, 0);*/
	doc_close_single_backend(data, FALSE, FALSE);
}

/**
 * document_set_line_numbers:
 * @doc: a #Tdocument*
 * @value: a #gboolean
 *
 * Show or hide linenumbers (at the left of the main GtkTextView).
 *
 * Return value: void
 **/
void document_set_line_numbers(Tdocument *doc, gboolean value) {
#ifdef USE_BFTEXTVIEW2
	BLUEFISH_TEXT_VIEW(doc->view)->linenumbers = value;
#else
	bf_textview_show_lines(BF_TEXTVIEW(doc->view),value);
#endif
}

/**
 * document_set_show_blocks:
 * @doc: a #Tdocument*
 * @value: a #gboolean
 *
 * Show or hide blocks (at the left of the main GtkTextView).
 *
 * Return value: void
 **/
void document_set_show_blocks(Tdocument *doc, gboolean value) {
#ifdef USE_BFTEXTVIEW2
	BLUEFISH_TEXT_VIEW(doc->view)->showblocks = value;
#else
	bf_textview_show_blocks(BF_TEXTVIEW(doc->view),value);
#endif
}

/**
 * document_set_show_symbols:
 * @doc: a #Tdocument*
 * @value: a #gboolean
 *
 * Show or hide symbols (at the left of the main GtkTextView).
 *
 * Return value: void
 **/
#ifndef USE_BFTEXTVIEW2
void document_set_show_symbols(Tdocument *doc, gboolean value) {
	bf_textview_show_symbols(BF_TEXTVIEW(doc->view),value);
}
#endif
#ifndef USE_BFTEXTVIEW2
static void doc_view_drag_end_lcb(GtkWidget *widget,GdkDragContext *drag_context,Tdocument *doc) {
	if (doc->paste_operation) {
		if (PASTEOPERATION(doc->paste_operation)->eo > PASTEOPERATION(doc->paste_operation)->so) {
		}
		g_free(doc->paste_operation);
		doc->paste_operation = NULL;
		BF_TEXTVIEW(doc->view)->paste_operation = FALSE;
	}
}
static void doc_view_drag_begin_lcb(GtkWidget *widget,GdkDragContext *drag_context,Tdocument *doc) {
	if (!doc->paste_operation) {
		doc->paste_operation = g_new(Tpasteoperation,1);
		PASTEOPERATION(doc->paste_operation)->so = -1;
		PASTEOPERATION(doc->paste_operation)->eo = -1;
		BF_TEXTVIEW(doc->view)->paste_operation = TRUE;
	}
}
#endif
static void doc_close_but_set_style_lcb (GtkWidget *button, GtkStyle *previous_style, gpointer user_data) {
	gint h, w;

	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (button), GTK_ICON_SIZE_MENU, &w, &h);
	gtk_widget_set_size_request (button, w + 2, h + 2);
}

static Tdocument *doc_new_backend(Tbfwin *bfwin, gboolean force_new, gboolean readonly) {
	GtkWidget *scroll;
	Tdocument *newdoc;
	GdkPixbuf *pixbuf;

	/* test if the current document is empty and nameless, if so we return that */
	if (!force_new && g_list_length(bfwin->documentlist)==1 && doc_is_empty_non_modified_and_nameless(bfwin->current_document)) {
		newdoc = bfwin->current_document;
		DEBUG_MSG("doc_new_backend, returning existing doc %p\n",newdoc);
		return newdoc;
	}

	newdoc = g_new0(Tdocument, 1);
	DEBUG_MSG("doc_new_backend, main_v is at %p, bfwin at %p, newdoc at %p\n", main_v, bfwin, newdoc);
	newdoc->readonly = readonly;
	newdoc->bfwin = (gpointer)bfwin;
	newdoc->status = DOC_STATUS_COMPLETE; /* if we don't set this default we will get problems for new empty files */
#ifdef USE_BFTEXTVIEW2
	newdoc->buffer = gtk_text_buffer_new(langmgr_get_tagtable());
	newdoc->view = bftextview2_new_with_buffer(newdoc->buffer);
#else
	newdoc->buffer = gtk_text_buffer_new(textstyle_return_tagtable());
	newdoc->view = bf_textview_new_with_buffer(newdoc->buffer);
#endif
	g_object_set(G_OBJECT(newdoc->view), "editable", !readonly, NULL);
#ifdef USE_BFTEXTVIEW2
	bluefish_text_view_set_mimetype(BLUEFISH_TEXT_VIEW(newdoc->view), "text/plain");
#else
	/* set the default doc to be plain text for now.
       we should maybe add a default file type to preferences, projects, and/or session
	   or maybe a dialog asking the user what to create */
	newdoc->hl = get_filetype_for_mime_type("text/plain");
	if (newdoc->hl->cfg) {
		bf_textview_set_language_ptr(BF_TEXTVIEW(newdoc->view),newdoc->hl->cfg);
	}
	BF_TEXTVIEW(newdoc->view)->tag_autoclose = main_v->props.tag_autoclose;
	bf_textview_recolor(BF_TEXTVIEW(newdoc->view),main_v->props.editor_fg,main_v->props.editor_bg);
	bf_textview_show_rmargin(BF_TEXTVIEW(newdoc->view),main_v->props.view_rmargin,main_v->props.rmargin_at);
/*	bf_textview_set_fg_color(BF_TEXTVIEW(newdoc->view),main_v->props.editor_fg);*/
	pixbuf = gdk_pixbuf_new_from_inline(-1,pixmap_bookmarks,FALSE,NULL);
	bf_textview_add_symbol(BF_TEXTVIEW(newdoc->view),"bookmark",pixbuf);
	g_object_unref(pixbuf);
#endif
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
									   GTK_POLICY_AUTOMATIC,
									   GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
											(scroll), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scroll), newdoc->view);

	newdoc->linenumberstate = main_v->props.view_line_numbers;
/*	document_set_line_numbers(newdoc, newdoc->linenumberstate); set in the widget by default*/
	newdoc->blocksstate = main_v->props.view_blocks;
/*	document_set_show_blocks(newdoc, newdoc->blocksstate); set in the widget by default */
#ifndef USE_BFTEXTVIEW2
	newdoc->symstate = main_v->props.view_symbols;
	document_set_show_symbols(newdoc, newdoc->symstate);
#endif
	newdoc->tab_label = gtk_label_new(NULL);
	GTK_WIDGET_UNSET_FLAGS(newdoc->tab_label, GTK_CAN_FOCUS);
	if (strlen(main_v->props.tab_font_string)) {
		apply_font_style(newdoc->tab_label, main_v->props.tab_font_string);
	}
	newdoc->tab_menu = gtk_label_new(NULL);
	newdoc->tab_eventbox = gtk_event_box_new();
	gtk_misc_set_alignment(GTK_MISC(newdoc->tab_menu), 0,0);

	doc_unre_init(newdoc);
	doc_set_font(newdoc, NULL);
	newdoc->wrapstate = (bfwin->project) ? bfwin->project->word_wrap : main_v->props.word_wrap;
	doc_set_wrap(newdoc);

	doc_set_title(newdoc);
	/* we initialize already with 0 , so we don't need these:
	newdoc->need_highlighting = 0;
	newdoc->uri = NULL;
	newdoc->modified = 0;
	newdoc->fileinfo = NULL;*/
	newdoc->is_symlink = 0;
	newdoc->encoding = g_strdup( (bfwin->session->encoding) ? bfwin->session->encoding : main_v->props.newfile_default_encoding);
	DEBUG_MSG("doc_new_backend, encoding is %s\n",newdoc->encoding);
	newdoc->overwrite_mode = FALSE;
	doc_bind_signals(newdoc);

#ifndef USE_BFTEXTVIEW2
	g_signal_connect(G_OBJECT(newdoc->view), "button-release-event",
		G_CALLBACK(doc_view_button_release_lcb), newdoc);
#endif
	g_signal_connect(G_OBJECT(newdoc->view), "button-press-event",
		G_CALLBACK(doc_view_button_press_lcb), newdoc);
	g_signal_connect(G_OBJECT(newdoc->buffer), "changed",
		G_CALLBACK(doc_buffer_changed_lcb), newdoc);
	g_signal_connect(G_OBJECT(newdoc->buffer), "mark-set",
		G_CALLBACK(doc_buffer_mark_set_lcb), newdoc);
	g_signal_connect(G_OBJECT(newdoc->view), "toggle-overwrite",
		G_CALLBACK(doc_view_toggle_overwrite_lcb), newdoc);
/*	g_signal_connect(G_OBJECT(newdoc->view), "paste-clipboard",
		G_CALLBACK(doc_paste_clipboard_lcb), newdoc);
	g_signal_connect_after(G_OBJECT(newdoc->view), "button-release-event",
		G_CALLBACK(doc_view_button_release_after_lcb), newdoc);*/
#ifndef USE_BFTEXTVIEW2
	g_signal_connect_after(G_OBJECT(newdoc->view), "drag-end",
		G_CALLBACK(doc_view_drag_end_lcb), newdoc);
	g_signal_connect_after(G_OBJECT(newdoc->view), "drag-begin",
		G_CALLBACK(doc_view_drag_begin_lcb), newdoc);
	g_signal_connect_after(G_OBJECT(newdoc->view), "key-release-event",
		G_CALLBACK(doc_view_key_release_lcb), newdoc);
#endif
	g_signal_connect(G_OBJECT(newdoc->view), "key-press-event",
		G_CALLBACK(doc_view_key_press_lcb), newdoc);
	g_signal_connect_after(G_OBJECT(newdoc->view), "populate-popup",
		G_CALLBACK(doc_view_populate_popup_lcb), newdoc);

	bfwin->documentlist = g_list_append(bfwin->documentlist, newdoc);

	gtk_widget_show(newdoc->tab_label);
	gtk_widget_show(scroll);

	DEBUG_MSG("doc_new_backend, appending doc to notebook\n");
	{
		GtkWidget *hbox, *button, *image;
		GtkRcStyle *rcstyle;

		hbox = gtk_hbox_new(FALSE, 4);

		button = gtk_button_new ();
		image = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
		gtk_container_add (GTK_CONTAINER (button), image);
		gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);

		rcstyle = gtk_rc_style_new ();
		rcstyle->xthickness = rcstyle->ythickness = 0;
		gtk_widget_modify_style (button, rcstyle);
		gtk_rc_style_unref (rcstyle),

		g_signal_connect(button, "clicked", G_CALLBACK (doc_close_but_clicked_lcb), newdoc);
		g_signal_connect (button, "style-set", G_CALLBACK (doc_close_but_set_style_lcb), NULL);

		gtk_container_add(GTK_CONTAINER(newdoc->tab_eventbox), newdoc->tab_label);
		gtk_box_pack_start(GTK_BOX(hbox), newdoc->tab_eventbox, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
		gtk_widget_show_all(hbox);
		gtk_notebook_append_page_menu(GTK_NOTEBOOK(bfwin->notebook), scroll ,hbox, newdoc->tab_menu);
#if GTK_CHECK_VERSION(2,10,0)
		gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(bfwin->notebook), scroll, TRUE);
#endif
	}
	/* for some reason it only works after the document is appended to the notebook */
	doc_set_tabsize(newdoc, main_v->props.editor_tab_width);

#ifndef USE_BFTEXTVIEW2
	BF_TEXTVIEW(newdoc->view)->highlight = main_v->props.defaulthighlight;
#endif
	DEBUG_MSG("doc_new_backend, need_highlighting=%d, highlight=%d\n", newdoc->need_highlighting, BF_TEXTVIEW(newdoc->view)->highlight);
	return newdoc;
}

/**
 * doc_new_loading_in_background:
 * @bfwin: the #Tbfwin* in which window to load this document
 *
 * creates a new document, which will be loaded in the background
 */
Tdocument *doc_new_loading_in_background(Tbfwin *bfwin, GFile *uri, GFileInfo *finfo, gboolean readonly) {
	Tdocument *doc = doc_new_backend(bfwin, FALSE, readonly);
	DEBUG_MSG("doc_new_loading_in_background, bfwin=%p, doc=%p, for uri %p\n",bfwin,doc,uri);
	if (finfo) {
		doc->fileinfo = finfo;
		g_object_ref(finfo);
	} else {
		doc->fileinfo = NULL;
	}
	doc_set_filename(doc,uri);
	doc_set_status(doc, DOC_STATUS_LOADING);
	bfwin_docs_not_complete(bfwin, TRUE);
	return doc;
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
Tdocument *doc_new(Tbfwin* bfwin, gboolean delay_activate) {
	Tdocument *doc = doc_new_backend(bfwin, TRUE, FALSE);
	doc_set_status(doc, DOC_STATUS_COMPLETE);
	DEBUG_MSG("doc_new, status=%d\n",doc->status);
	if(!delay_activate) gtk_widget_show(doc->view); /* Delay _show() if neccessary */
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
void doc_new_from_uri(Tbfwin *bfwin, GFile *opturi, GFileInfo *finfo, gboolean delay_activate, gboolean move_to_this_win, gint goto_line, gint goto_offset) {
	GList *alldocs;
	Tdocument *tmpdoc;
	gchar *tmpcuri;
	GFile *uri;
	gboolean open_readonly=FALSE;
	if (!bfwin || !opturi) {
		return;
	}
	uri = opturi;
	g_object_ref(opturi);
	tmpcuri = g_file_get_uri(opturi);
	DEBUG_MSG("doc_new_from_uri, started for uri(%p)=%s\n",uri,tmpcuri);

	/* check if the document already is opened */
	alldocs = return_allwindows_documentlist();
	tmpdoc = documentlist_return_document_from_uri(alldocs, uri);
	g_list_free(alldocs);
	if (tmpdoc) { /* document is already open */
		if (tmpdoc->bfwin != bfwin && move_to_this_win) {
			/* we should aks the user if it is OK to move the document */
			if (!delay_activate)	bfwin->focus_next_new_doc = TRUE;
			doc_move_to_window_dialog(tmpdoc, bfwin);
			/* TODO: or open the document readonly */
		} else if (!delay_activate) { /* switch to window, only if we should */
			switch_to_document_by_pointer(BFWIN(tmpdoc->bfwin),tmpdoc);
			if (bfwin != tmpdoc->bfwin) gtk_window_present(GTK_WINDOW(BFWIN(tmpdoc->bfwin)->main_window));
		}
		if (tmpdoc != NULL) {
			if (goto_line >= 0) doc_select_line(tmpdoc, goto_line, TRUE);
			else if (goto_offset >= 0) doc_select_line_by_offset(tmpdoc, goto_offset, TRUE);
		}
	} else { /* document is not yet opened */
		if (!delay_activate)	bfwin->focus_next_new_doc = TRUE;
		DEBUG_MSG("doc_new_from_uri, uri=%p, delay_activate=%d, focus_next_new_doc=%d\n",uri,delay_activate, bfwin->focus_next_new_doc);
		file_doc_from_uri(bfwin, uri, finfo, goto_line, goto_offset, open_readonly);
	}
	add_filename_to_history(bfwin, tmpcuri);
	session_set_opendir(bfwin, tmpcuri);
	g_free(tmpcuri);
}

void doc_new_from_input(Tbfwin *bfwin, gchar *input, gboolean delay_activate, gboolean move_to_this_win, gint goto_line) {
	GFile *uri=NULL;
	if (!input) {
		return;
	}
	DEBUG_MSG("doc_new_from_input, input=%s, delay_activate=%d\n",input,delay_activate);
	if (strchr(input, '/')==NULL) { /* no slashes in the path, relative ?*/
		if (bfwin->current_document->uri) {
			uri = g_file_resolve_relative_path(bfwin->current_document->uri, input);
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

void docs_new_from_uris(Tbfwin *bfwin, GSList *urislist, gboolean move_to_this_win) {
	GSList *tmpslist;
	gboolean one_doc=(g_slist_length(urislist)==1);
	bfwin->focus_next_new_doc = !one_doc;
	tmpslist = urislist;
	while (tmpslist) {
		GFile *uri;
		if (strstr ((gchar *) tmpslist->data, "://") == NULL)
			uri = g_file_new_for_path((gchar *) tmpslist->data);
		else
			uri = g_file_new_for_uri((gchar *) tmpslist->data);
		doc_new_from_uri(bfwin, uri, NULL, !one_doc, move_to_this_win, -1, -1);
		g_object_unref(uri);
		tmpslist = g_slist_next(tmpslist);
	}
	DEBUG_MSG("docs_new_from_uris, done, all documents loading in background\n");
}

/**
 * docs_new_from_files:
 * @bfwin: #Tbfwin* with the window to open the document in
 * @file_list: #GList with filenames to open.
 * @move_to_this_win: #gboolean if the file needs to be moved to this window if it is open already
 *
 * Open a number of new documents from files in stringlist file_list.
 * If a file is open already in another window, it might be moved to this window, else
 * nothing is done for this file
 * Report files with problems to user.
 * If more than 8 files are opened at once, a modal progressbar is shown while loading.
 *
 * Return value: void
 **/
/* void docs_new_from_files(Tbfwin *bfwin, GList * file_list, gboolean move_to_this_win) {
	GList *tmplist, *errorlist=NULL;
	gboolean delay = (g_list_length(file_list) > 1);
	gpointer pbar = NULL;
	gint i = 0;
	DEBUG_MSG("docs_new_from_files, lenght=%d\n", g_list_length(file_list));

	/ * Hide the notebook and show a progressbar while
	 * adding several files. * /
	if(g_list_length(file_list) > 8) {
		notebook_hide(bfwin);
		pbar = progress_popup(bfwin->main_window,_("Loading files..."), g_list_length(file_list));
	}

	tmplist = g_list_first(file_list);
	while (tmplist) {
		DEBUG_MSG("docs_new_from_files, about to open %s, delay=%d\n", (gchar *) tmplist->data, delay);
		if (!doc_new_with_file(bfwin,(gchar *) tmplist->data, delay, move_to_this_win)) {
			errorlist = g_list_append(errorlist, g_strdup((gchar *) tmplist->data));
		}
		if(pbar) {
			progress_set(pbar, ++i);
			flush_queue();
		}
		tmplist = g_list_next(tmplist);
	}
	if (errorlist){
		gchar *message, *tmp;
		tmp = stringlist_to_string(errorlist, "\n");
		message = g_strconcat(_("These files could not opened:\n\n"), tmp, NULL);
		g_free(tmp);
		warning_dialog(bfwin->main_window,_("Unable to open file(s)\n"), message);
		g_free(message);
	}
	free_stringlist(errorlist);

	if (delay) {
		DEBUG_MSG("since we delayed the highlighting, we set the notebook and filebrowser page now\n");

		/ * Destroy the progressbar and show the notebook when finished. * /
		progress_destroy(pbar);
		notebook_show(bfwin);

		gtk_notebook_set_page(GTK_NOTEBOOK(bfwin->notebook),g_list_length(bfwin->documentlist) - 1);
		notebook_changed(bfwin,-1);
		if (bfwin->current_document && bfwin->current_document->filename) {
			/ *filebrowser_open_dir(bfwin,bfwin->current_document->filename); is called by doc_activate() * /
			doc_activate(bfwin->current_document);
		}
	}
	gui_set_title(bfwin, bfwin->current_document);
}*/

/**
 * doc_reload:
 * @doc: a #Tdocument
 * @finfo: if the reload is because the file is modified, pass here the new fileinfo, use NULL if unknown
 *
 * Revert to file on disk.
 *
 * Return value: void
 **/
void doc_reload(Tdocument *doc, GFileInfo *newfinfo) {
	GtkTextIter itstart, itend, cursor;
	gint cursorpos=-1;
	DEBUG_MSG("starting reload for %p\n",doc);
	if ((doc->uri == NULL)/* || (!file_exists_and_readable(doc->uri))*/) {
		statusbar_message(BFWIN(doc->bfwin),_("Unable to open file"), 2000);
		return;
	}
	/* store all bookmark positions, reload them later */
	bmark_clean_for_doc(doc);
	gtk_text_buffer_get_iter_at_mark(doc->buffer,&cursor,gtk_text_buffer_get_insert(doc->buffer));
	cursorpos = gtk_text_iter_get_line(&cursor);
	gtk_text_buffer_get_bounds(doc->buffer,&itstart,&itend);
	gtk_text_buffer_delete(doc->buffer,&itstart,&itend);
	doc_set_status(doc, DOC_STATUS_LOADING);
	bfwin_docs_not_complete(doc->bfwin, TRUE);
	doc_set_modified(doc,FALSE);
	g_object_unref(doc->fileinfo);
	doc->fileinfo = newfinfo;
	g_object_ref(doc->fileinfo);
	file_doc_fill_from_uri(doc, doc->uri, doc->fileinfo, cursorpos);
}

static void doc_activate_modified_lcb(Tcheckmodified_status status,gint error_info,GFileInfo *orig, GFileInfo *new, gpointer callback_data) {
	Tdocument *doc = callback_data;
	switch (status) {
	case CHECKMODIFIED_ERROR:
		DEBUG_MSG("doc_activate_modified_lcb, CHECKMODIFIED_ERROR ??\n");
		if (error_info == G_IO_ERROR_NOT_FOUND) {
			gchar *tmpstr;
			gint retval;
			const gchar *buttons[] = {_("_Unset file name"),_("_Save"), NULL};
			/* file is deleted on disk, what do we do now ? */
			tmpstr = g_strdup_printf(_("File name: %s"), gtk_label_get_text(GTK_LABEL(doc->tab_menu)));
			retval = message_dialog_new_multi(BFWIN(doc->bfwin)->main_window,
													 GTK_MESSAGE_WARNING,
													 buttons,
													 _("File disappeared from disk\n"),
													 tmpstr);
			g_free(tmpstr);
			if (retval == 1) { /* save */
				doc_save_backend(doc, FALSE, FALSE, FALSE, FALSE);
			} else { /* unset */
				document_unset_filename(doc);
			}
		}
	break;
	case CHECKMODIFIED_CANCELLED:
		DEBUG_MSG("doc_activate_modified_lcb, CHECKMODIFIED_CANCELLED\n");
	break;
	case CHECKMODIFIED_MODIFIED:
		{
		gchar *tmpstr, *oldtimestr, *newtimestr;
		gint retval;
		const gchar *buttons[] = {_("_Ignore"),_("_Reload"),_("Check and reload all documents"), NULL};
		time_t newtime,origtime;

		newtime = (time_t)g_file_info_get_attribute_uint64(new,G_FILE_ATTRIBUTE_TIME_MODIFIED);
		origtime = (time_t)g_file_info_get_attribute_uint64(orig,G_FILE_ATTRIBUTE_TIME_MODIFIED);
		newtimestr = bf_portable_time(&newtime);
		oldtimestr = bf_portable_time(&origtime);

		tmpstr = g_strdup_printf(_("Filename: %s\n\nNew modification time is: %s\nOld modification time is: %s"), gtk_label_get_text(GTK_LABEL(doc->tab_menu)), newtimestr, oldtimestr);
		retval = message_dialog_new_multi(BFWIN(doc->bfwin)->main_window,
													 GTK_MESSAGE_WARNING,
													 buttons,
													 _("File has been modified by another process\n"),
													 tmpstr);
		g_free(tmpstr);
		g_free(newtimestr);
		g_free(oldtimestr);
		if (retval == 0) { /* ignore */
			if (doc->fileinfo) {
				g_object_unref(doc->fileinfo);
			}
			doc->fileinfo = new;
			g_object_ref(doc->fileinfo);
			doc_set_tooltip(doc);
		} else if (retval == 1) { /* reload */
			doc_reload(doc, new);
		} else { /* reload all modified documents */
			file_reload_all_modified(doc->bfwin);
		}
		}
	break;
	case CHECKMODIFIED_OK:
		/* do nothing */
	break;
	}
	doc->action.checkmodified = NULL;
}

void doc_start_modified_check(Tdocument *doc) {
	if (doc->uri && doc->fileinfo && !doc->action.checkmodified && !doc->action.save) { /* don't check during another check, or during save */
		doc->action.checkmodified = file_checkmodified_uri_async(doc->uri, doc->fileinfo, doc_activate_modified_lcb, doc);
	}
}

static gboolean doc_close_from_activate(gpointer data) {
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
void doc_activate(Tdocument *doc) {
#ifdef DEBUG
	if (!doc) {
		DEBUG_MSG("doc_activate, doc=NULL!!! ABORTING!!\n");
		exit(44);
	}
#endif
	if (doc == NULL || doc == BFWIN(doc->bfwin)->last_activated_doc || doc->action.close_doc) {
		DEBUG_MSG("doc_activate, not doing anything, doc=%p, last_avtivated_doc=%p, close_doc=%d\n",doc, BFWIN(doc->bfwin)->last_activated_doc, doc->action.close_doc);
		return;
	}
	g_print("doc_activate for doc with view %p..\n",doc->view);
	if (doc->status == DOC_STATUS_ERROR) {
		const gchar *buttons[] = {_("_Retry"), _("Retry _all failed"),_("_Close"), _("Close all _failed"), NULL};
		gchar *tmpstr;
		gint retval;
		DEBUG_MSG("doc_activate, DOC_STATUS_ERROR, retry???\n");
		tmpstr = g_strconcat(_("File "), gtk_label_get_text(GTK_LABEL(doc->tab_menu)), _(" failed to load."), NULL);
		retval = message_dialog_new_multi(BFWIN(doc->bfwin)->main_window,
													 GTK_MESSAGE_WARNING,
													 buttons,
													 _("File failed to load\n"),
													 tmpstr);
		g_free(tmpstr);
		switch (retval) {
			case 0:
				file_doc_retry_uri(doc);
			break;
			case 1: {
				GList *tmplist;
				/* retry all failed documents */
				for (tmplist=g_list_first(BFWIN(doc->bfwin)->documentlist);tmplist!=NULL;tmplist=tmplist->next) {
					if (DOCUMENT(tmplist->data)->status == DOC_STATUS_ERROR) file_doc_retry_uri(DOCUMENT(tmplist->data));
				}
			} break;
			case 2:
				g_idle_add(doc_close_from_activate, doc);
			break;
			case 3: {
				GList *tmplist;
				/* retry all failed documents */
				for (tmplist=g_list_first(BFWIN(doc->bfwin)->documentlist);tmplist!=NULL;tmplist=tmplist->next) {
					if (DOCUMENT(tmplist->data)->status == DOC_STATUS_ERROR) g_idle_add(doc_close_from_activate,DOCUMENT(tmplist->data));
				}
			} break;
#ifdef DEVELOPMENT
			default:
				g_print("doc_activate, retval=%d does not exist\n",retval);
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
#ifdef USE_BFTEXTVIEW2
		if (!BLUEFISH_TEXT_VIEW(doc->view)->enable_scanner) {
			/*g_print("doc_activate, enable scanner for doc %p\n",doc);*/
			BLUEFISH_TEXT_VIEW(doc->view)->enable_scanner = TRUE;
			bftextview2_schedule_scanning(BLUEFISH_TEXT_VIEW(doc->view));
		}
#endif
		gtk_widget_show(doc->view); /* This might be the first time this document is activated. */
	}
	BFWIN(doc->bfwin)->last_activated_doc = doc;
	doc_start_modified_check(doc);

	DEBUG_MSG("doc_activate, calling gui_set_document_widgets()\n");
	gui_set_document_widgets(doc);
	gui_set_title(BFWIN(doc->bfwin), doc);
	doc_set_statusbar_lncol(doc);
	doc_set_statusbar_insovr(doc);
	doc_set_statusbar_editmode_encoding(doc);

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
	DEBUG_MSG("doc_activate, doc=%p, about to grab focus\n",doc);
	gtk_widget_grab_focus(GTK_WIDGET(doc->view));

	DEBUG_MSG("doc_activate, doc=%p, finished\n",doc);

}

void doc_force_activate(Tdocument *doc) {
	BFWIN(doc->bfwin)->last_activated_doc = NULL;
	doc_activate(doc);
}

void file_open_from_selection(Tbfwin *bfwin) {
	gchar *string;
	GtkClipboard* cb;

	cb = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	string = gtk_clipboard_wait_for_text(cb);
	if (string) {
		DEBUG_MSG("file_open_from_selection, opening %s\n",string);
		doc_new_from_input(bfwin, string, FALSE, FALSE, -1);
/*		if (NULL == strchr(string,'/') && bfwin->current_document->uri) {
			/ * now we should look in the directory of the current file * /
			gchar *dir, *tmp;
			dir = g_path_get_dirname(bfwin->current_document->uri);
			tmp = g_strconcat(dir, "/", string, NULL);
			DEBUG_MSG("file_open_from_selection, trying %s\n",tmp);
			doc_new_from_uri(bfwin, tmp, NULL, NULL, FALSE, FALSE, -1);
			g_free(dir);
			g_free(tmp);
		} else {

		}*/
		g_free(string);
	}
}

/**
 * file_insert_menucb:
 * @bfwin: Tbfwin* which window
 * @callback_action: unused #guint
 * @widget: #GtkWidget* unused
 *
 * Prompt user for a file, and insert the contents into the current document.
 *
 * Return value: void
 **/
void file_insert_menucb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget) {
	gchar *tmpfilename=NULL;
	{
		GtkWidget *dialog;
		dialog = file_chooser_dialog(bfwin, _("Select file to insert"), GTK_FILE_CHOOSER_ACTION_OPEN, NULL, FALSE, FALSE, NULL, TRUE);
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
			tmpfilename = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
		}
		/* BUG: get the encoding from the dialog !!! */
		gtk_widget_destroy(dialog);
	}
	if (tmpfilename == NULL) {
		statusbar_message(bfwin,_("No file to insert"), 2000);
		return;
	} else {
		GFile *uri;

		doc_unre_new_group(bfwin->current_document);
		uri = g_file_new_for_uri(tmpfilename);
		file_into_doc(bfwin->current_document, uri, FALSE);
		g_object_unref(uri);
		g_free(tmpfilename);
		/* doc_file_to_textbox(bfwin->current_document, tmpfilename, TRUE, FALSE);
		doc_set_modified(bfwin->current_document, 1);*/
	}
}

/**
 * edit_cut_cb:
 * @widget: unused #GtkWidget
 * @data: unused #gpointer
 *
 * 	Cut selection from current buffer, to clipboard.
 *
 * Return value: void
 **/
void edit_cut_cb(GtkWidget * widget, Tbfwin *bfwin) {
	doc_unre_new_group(bfwin->current_document);
	gtk_text_buffer_cut_clipboard(bfwin->current_document->buffer,gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),TRUE);
	doc_unre_new_group(bfwin->current_document);
}

/**
 * edit_copy_cb:
 * @widget: unused #GtkWidget
 * @data: unused #gpointer
 *
 * 	Copy selection from current buffer, to clipboard.
 *
 * Return value: void
 **/
void edit_copy_cb(GtkWidget * widget, Tbfwin *bfwin) {
	gtk_text_buffer_copy_clipboard(bfwin->current_document->buffer,gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
}

/**
 * edit_paste_cb:
 * @widget: unused #GtkWidget
 * @data: unused #gpointer
 *
 * 	Paste contents of clipboard. Disable highlighting while pasting, for speed.
 *
 * Return value: void
 **/
void edit_paste_cb(GtkWidget * widget, Tbfwin *bfwin) {
	GtkTextMark *mark;
	Tdocument *doc = bfwin->current_document;
	DEBUG_MSG("edit_paste_cb, started\n");
#ifndef USE_BFTEXTVIEW2
	if (!doc->paste_operation) {
		doc->paste_operation = g_new(Tpasteoperation,1);
		PASTEOPERATION(doc->paste_operation)->so = -1;
		PASTEOPERATION(doc->paste_operation)->eo = -1;
		BF_TEXTVIEW(doc->view)->paste_operation = TRUE;
	}
#endif
	doc_unre_new_group(doc);

	DEBUG_MSG("edit_paste_cb, pasting clipboard\n");
	gtk_text_buffer_paste_clipboard (doc->buffer,gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),NULL,TRUE);

	doc_unre_new_group(doc);
#ifndef USE_BFTEXTVIEW2
	if (PASTEOPERATION(doc->paste_operation)->eo > PASTEOPERATION(doc->paste_operation)->so) {
		DEBUG_MSG("edit_paste_cb, start doc_highlight_region for so=%d, eo=%d\n",PASTEOPERATION(doc->paste_operation)->so,PASTEOPERATION(doc->paste_operation)->eo);
	}
	g_free(doc->paste_operation);
	doc->paste_operation = NULL;
	BF_TEXTVIEW(doc->view)->paste_operation = FALSE;
#endif
	mark = gtk_text_buffer_get_insert(bfwin->current_document->buffer);
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(bfwin->current_document->view), mark);
	DEBUG_MSG("edit_paste_cb, finished\n");
}

/**
 * edit_select_all_cb:
 * @widget: unused #GtkWidget
 * @data: unused #gpointer
 *
 * Mark entire current document as selected.
 *
 * Return value: void
 **/
void edit_select_all_cb(GtkWidget * widget, Tbfwin *bfwin) {
	GtkTextIter itstart, itend;
	gtk_text_buffer_get_bounds(bfwin->current_document->buffer,&itstart,&itend);
	gtk_text_buffer_move_mark_by_name(bfwin->current_document->buffer,"insert",&itstart);
	gtk_text_buffer_move_mark_by_name(bfwin->current_document->buffer,"selection_bound",&itend);
}

/**
 * doc_toggle_highlighting_cb:
 * @callback_data: unused #gpointer
 * @action: unused #guint
 * @widget: unused #GtkWidget*
 *
 * Toggle highlighting on/off for current document.
 *
 * Return value: void
 **/
void doc_toggle_highlighting_cb(Tbfwin *bfwin,guint action,GtkWidget *widget) {
#ifdef USE_BFTEXTVIEW2
	BLUEFISH_TEXT_VIEW(bfwin->current_document->view)->enable_scanner = BLUEFISH_TEXT_VIEW(bfwin->current_document->view)->enable_scanner-1;
#else
	BF_TEXTVIEW(bfwin->current_document->view)->highlight =
		BF_TEXTVIEW(bfwin->current_document->view)->highlight ? FALSE : TRUE;
	DEBUG_MSG("doc_toggle_highlighting_cb, started, highlight is %d\n", BF_TEXTVIEW(bfwin->current_document->view)->highlight);
	bf_textview_scan(BF_TEXTVIEW(bfwin->current_document->view));
#endif
}

/**
 * all_documents_apply_settings:
 *
 * applies changes from the preferences to all documents
 *
 * Return value: void
 */
void all_documents_apply_settings() {
	GList *tmplist = g_list_first(return_allwindows_documentlist());
	while (tmplist){
		Tdocument *doc = tmplist->data;
		doc_set_tabsize(doc, main_v->props.editor_tab_width);
		doc_set_font(doc, main_v->props.editor_font_string);
#ifndef USE_BFTEXTVIEW2
		bf_textview_recolor(BF_TEXTVIEW(doc->view),main_v->props.editor_fg,main_v->props.editor_bg);
#endif
		tmplist = g_list_next(tmplist);
	}

}

/**
 * doc_toggle_highlighting_cb:
 * @callback_data: unused #gpointer
 * @action: unused #guint
 * @widget: unused #GtkWidget*
 *
 * Show word-, line- and charcount for current document in the statusbar.
 * Note: The wordcount() call returns number of actual utf8-chars, not bytes.
 *
 * Return value: void
 **/
void word_count_cb (Tbfwin *bfwin,guint callback_action,GtkWidget *widget) {
	guint chars = 0, lines = 0, words = 0;
	gchar *allchars, *wc_message;

   allchars = doc_get_chars(bfwin->current_document, 0, -1);
	wordcount(allchars, &chars, &lines, &words);
	g_free(allchars);

	wc_message = g_strdup_printf(_("Statistics: %d lines, %d words, %d characters"), lines, words, chars);
	statusbar_message (bfwin,wc_message, 5000);
	g_free (wc_message);
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
		if (gtk_text_iter_get_line_offset(&itstart)>0) {
			gtk_text_iter_set_line_index(&itstart,0);
		}
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
					gint i=0;
					itend = itstart;
					gtk_text_iter_forward_chars(&itend,main_v->props.editor_tab_width);
					buf = gtk_text_buffer_get_text(doc->buffer,&itstart,&itend,TRUE);
					DEBUG_MSG("tab_width=%d, strlen(buf)=%d, buf='%s'\n",main_v->props.editor_tab_width,strlen(buf),buf);
					while (cont && buf[i] != '\0') {
						cont = (buf[i] == ' ');
						DEBUG_MSG("doc_indent_selection, buf[%d]='%c'\n",i,buf[i]);
						i++;
					}
					if (!cont) {
						g_free (buf);
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
					g_free (buf);
				}
#ifdef DEBUG
				else {
					DEBUG_MSG("doc_indent_selection, NOT continue!!\n");
				}
#endif
			} else { /* indent */
				gint offsetstart = gtk_text_iter_get_offset(&itstart);
				gchar *indentstring;
				gint indentlen;
				if (main_v->props.editor_indent_wspaces) {
					indentstring = bf_str_repeat(" ", main_v->props.editor_tab_width);
					indentlen = main_v->props.editor_tab_width;
				} else {
					indentstring = g_strdup("\t");
					indentlen=1;
				}
				gtk_text_buffer_insert(doc->buffer,&itstart,indentstring,indentlen);
				doc_unre_add(doc, indentstring, offsetstart, offsetstart+indentlen, UndoInsert);
				g_free(indentstring);
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
		/* there is no selection, work on the current line */
		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_mark(doc->buffer,&iter,gtk_text_buffer_get_insert(doc->buffer));
		gtk_text_iter_set_line_offset(&iter,0);
		if (unindent) {
			gint deletelen = 0;
			gchar *tmpstr, *tmp2str;
			GtkTextIter itend = iter;
			gtk_text_iter_forward_chars(&itend,main_v->props.editor_tab_width);
			tmpstr = gtk_text_buffer_get_text(doc->buffer,&iter,&itend,TRUE);
			tmp2str = bf_str_repeat(" ", main_v->props.editor_tab_width);
			if (tmpstr[0] == '\t') {
				deletelen = 1;
			} else if (tmpstr && strncmp(tmpstr,tmp2str,main_v->props.editor_tab_width)==0) {
				deletelen = main_v->props.editor_tab_width;
			}
			g_free(tmpstr);
			g_free(tmp2str);
			if (deletelen) {
				itend = iter;
				gtk_text_iter_forward_chars(&itend,deletelen);
				gtk_text_buffer_delete(doc->buffer,&iter,&itend);
			}
		} else { /* indent */
			gchar *indentstring;
			gint indentlen;
			if (main_v->props.editor_indent_wspaces) {
				indentstring = bf_str_repeat(" ", main_v->props.editor_tab_width);
				indentlen = main_v->props.editor_tab_width;
			} else {
				indentstring = g_strdup("\t");
				indentlen=1;
			}
			gtk_text_buffer_insert(doc->buffer,&iter,indentstring,indentlen);
			g_free(indentstring);
		}
	}
}

void menu_indent_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget) {
	if (bfwin->current_document) {
		doc_indent_selection(bfwin->current_document, (callback_action == 1));
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
GList *list_relative_document_filenames(Tdocument *curdoc) {
	GList *tmplist, *retlist=NULL;
	gchar *curi;
	if (curdoc->uri == NULL) {
		return NULL;
	}
	curi = g_file_get_parse_name(curdoc->uri);

	tmplist = g_list_first(BFWIN(curdoc->bfwin)->documentlist);
	while (tmplist) {
		Tdocument *tmpdoc = tmplist->data;
		if (tmpdoc != curdoc && tmpdoc->uri != NULL) {
			gchar *tmp = g_file_get_parse_name(tmpdoc->uri);
			retlist = g_list_prepend(retlist,create_relative_link_to(curi, tmp));
			g_free(tmp);
		}
		tmplist = g_list_next(tmplist);
	}
	g_free(curi);
	return retlist;
}

static void floatingview_destroy_lcb(GtkWidget *widget, Tdocument *doc) {
	DEBUG_MSG("floatingview_destroy_lcb, called for doc=%p, doc->floatingview=%p\n",doc,doc->floatingview);
	if (doc->floatingview) {
		gtk_widget_destroy(FLOATINGVIEW(doc->floatingview)->window);
		g_free(doc->floatingview);
		doc->floatingview = NULL;
	}
}

static void new_floatingview(Tdocument *doc) {
	Tfloatingview *fv;
	gchar *title;
	GtkWidget *scrolwin;
	if (doc->floatingview) {
		fv = FLOATINGVIEW(doc->floatingview);
		gtk_window_present(GTK_WINDOW(fv->window));
		return;
	}
	fv = g_new(Tfloatingview,1);
	doc->floatingview = fv;
	DEBUG_MSG("new_floatingview for doc=%p is at %p\n",doc,doc->floatingview);
	title = (doc->uri) ? g_file_get_uri(doc->uri) : g_strdup("Untitled");
	fv->window = window_full2(title, GTK_WIN_POS_NONE, 5, G_CALLBACK(floatingview_destroy_lcb), doc, TRUE, NULL);
	g_free(title);
	gtk_window_set_role(GTK_WINDOW(fv->window), "floatingview");
	fv->textview = gtk_text_view_new_with_buffer(doc->buffer);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(fv->textview),FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(fv->textview),FALSE);
	apply_font_style(fv->textview, main_v->props.editor_font_string);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(fv->textview), GTK_WRAP_WORD);
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolwin), fv->textview);
	gtk_container_add(GTK_CONTAINER(fv->window),scrolwin);
	gtk_window_set_default_size(GTK_WINDOW(fv->window),600,600);
	gtk_widget_show_all(fv->window);
}

void file_floatingview_menu_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget) {
	new_floatingview(bfwin->current_document);
}
