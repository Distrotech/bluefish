/* Bluefish HTML Editor
 * file_dialogs.c --> file dialogs
 *
 * Copyright (C) 2005 Olivier Sessink
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
#define DEBUG

#include <gtk/gtk.h>
#include <string.h> /* memcpy */
#include "bluefish.h"
#include "gtk_easy.h"
#include "gui.h"
#include "document.h"
#include "file.h"
#include "stringlist.h"

/**************************************************************************/
/* the start of the callback functions for the menu, acting on a document */
/**************************************************************************/
typedef struct {
	GList *filenames_to_return;
	GtkWidget *win;
	GtkWidget *basedir;
	GtkWidget *find_pattern;
	GtkWidget *recursive;
	GtkWidget *grep_pattern;
	GtkWidget *is_regex;
	Tbfwin *bfwin;
} Tfiles_advanced;

static void files_advanced_win_destroy(GtkWidget * widget, Tfiles_advanced *tfs) {
	DEBUG_MSG("files_advanced_win_destroy, started\n");
	gtk_main_quit();
	DEBUG_MSG("files_advanced_win_destroy, gtk_main_quit called\n");
	window_destroy(tfs->win);
}

static void files_advanced_win_ok_clicked(GtkWidget * widget, Tfiles_advanced *tfs) {
	GnomeVFSURI *baseuri;
	gchar *basedir, *content_filter, *extension_filter;
	
	extension_filter = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(tfs->find_pattern)->entry), 0, -1);
	basedir = gtk_editable_get_chars(GTK_EDITABLE(tfs->basedir), 0, -1);
	baseuri = gnome_vfs_uri_new(basedir);
	content_filter = gtk_editable_get_chars(GTK_EDITABLE(tfs->grep_pattern), 0, -1);

	open_advanced(tfs->bfwin, baseuri, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tfs->recursive))
			, strlen(extension_filter) ==0 ? NULL : extension_filter
			, strlen(content_filter) ==0 ? NULL : content_filter
			, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tfs->is_regex)));
	g_free(basedir);
	g_free(content_filter);
	g_free(extension_filter);
	gnome_vfs_uri_unref(baseuri);
	
	tfs->bfwin->session->adv_open_recursive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tfs->recursive));
	
	files_advanced_win_destroy(widget, tfs);
}

/*
static void files_advanced_win_ok_clicked(GtkWidget * widget, Tfiles_advanced *tfs) {
	/ * create list here * /
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
/ *
command = `find c_basedir -name c_find_pattern c_recursive`
command = `grep -E 'c_grep_pattern' `find c_basedir -name c_find_pattern c_recursive``
* /
	if (strlen(c_grep_pattern) == 0) {
		command = g_strconcat(EXTERNAL_FIND, " ", c_basedir, " -name '", c_find_pattern, "' ", c_recursive, "> ", temp_file, NULL);
	} else {
		command = g_strconcat(EXTERNAL_GREP, " ",c_is_regex, " '", c_grep_pattern,"' `", EXTERNAL_FIND, " ", c_basedir, " -name '", c_find_pattern, "' ", c_recursive, "`", "> ", temp_file, NULL);
	}
	g_free(c_basedir);
	g_free(c_find_pattern);
	g_free(c_grep_pattern);
	DEBUG_MSG("files_advanced_win_ok_clicked, command=%s\n", command);
	statusbar_message(tfs->bfwin,_("searching files..."), 1000);
	flush_queue();
	system(command);
	tfs->filenames_to_return = get_stringlist(temp_file, tfs->filenames_to_return);
	g_free(command);
	remove_secure_dir_and_filename(temp_file);
	g_free(temp_file);
	files_advanced_win_destroy(widget, tfs);
}*/
static void files_advanced_win_cancel_clicked(GtkWidget * widget, Tfiles_advanced *tfs) {
	files_advanced_win_destroy(widget, tfs);
}

static void files_advanced_win_select_basedir_lcb(GtkWidget * widget, Tfiles_advanced *tfs) {
	gchar *olddir = gtk_editable_get_chars(GTK_EDITABLE(tfs->basedir),0,-1);
	/* concat a "/" to the end of the current directory. This fixes a bug where your 
	   current working directory was being parsed as /directory/file when you opened 
	   the dialog to browse for a directory
	*/
	gchar *tmpdir = g_strconcat(olddir, "/", NULL);
	gchar *newdir = NULL;
#ifdef HAVE_ATLEAST_GTK_2_4
	{
		GtkWidget *dialog;
		/*dialog = gtk_file_chooser_dialog_new (_("Select basedir"),NULL,
				GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);
		gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog),TRUE);
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),tmpdir);*/
		dialog = file_chooser_dialog(tfs->bfwin, _("Select basedir"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, NULL, TRUE, FALSE, NULL);
		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
			newdir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		}
		gtk_widget_destroy(dialog);
	}
#else
	newdir = return_dir(tmpdir, _("Select basedir"));
#endif
	g_free(tmpdir);
	if (newdir) {
		gtk_entry_set_text(GTK_ENTRY(tfs->basedir),newdir);
		g_free(newdir);
	}
	g_free(olddir);
}

static void files_advanced_win(Tfiles_advanced *tfs) {
	GtkWidget *vbox, *hbox, *but, *table;
	GList *list;
	if (!tfs->basedir) {
		gchar *curdir = g_get_current_dir();
		tfs->basedir = entry_with_text(curdir, 255);
		g_free (curdir);
	}
	
	tfs->win = window_full2(_("Advanced open file selector"), GTK_WIN_POS_MOUSE, 12, G_CALLBACK(files_advanced_win_destroy),tfs, TRUE, tfs->bfwin->main_window);
	DEBUG_MSG("files_advanced_win, tfs->win=%p\n",tfs->win);
	tfs->filenames_to_return = NULL;
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(tfs->win), vbox);
	
	table = gtk_table_new(9, 5, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 12);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
	
	/*gtk_table_attach_defaults(GTK_TABLE(table), gtk_label_new(_("grep {contains} `find {basedir} -name '{file type}'`")), 0, 5, 0, 1);*/
	gtk_table_attach_defaults(GTK_TABLE(table), gtk_hseparator_new(), 0, 5, 1, 2);

	/* filename part */
	/* curdir should get a value */
	bf_label_tad_with_markup(_("<b>General</b>"), 0, 0.5, table, 0, 3, 2, 3);

	bf_mnemonic_label_tad_with_alignment(_("Base_dir:"), tfs->basedir, 0, 0.5, table, 1, 2, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(table), tfs->basedir, 2, 4, 3, 4);
	gtk_table_attach(GTK_TABLE(table), bf_allbuttons_backend(_("_Browse..."), TRUE, 112, G_CALLBACK(files_advanced_win_select_basedir_lcb), tfs), 4, 5, 3, 4, GTK_SHRINK, GTK_SHRINK, 0, 0);

/*	g_free(curdir);*/
	
	list = g_list_append(NULL, "");
	list = g_list_append(list, ".php");
	list = g_list_append(list, ".php3");
	list = g_list_append(list, ".html");
	list = g_list_append(list, ".htm");
	list = g_list_append(list, ".shtml");
	list = g_list_append(list, ".pl");
	list = g_list_append(list, ".cgi");
	list = g_list_append(list, ".xml");
	list = g_list_append(list, ".c");
	list = g_list_append(list, ".h");
	list = g_list_append(list, ".py");
	list = g_list_append(list, ".java");
	tfs->find_pattern = combo_with_popdown("", list, 1);
	bf_mnemonic_label_tad_with_alignment(_("_File Type:"), tfs->find_pattern, 0, 0.5, table, 1, 2, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(table), tfs->find_pattern, 2, 4, 4, 5);

	g_list_free(list);

	tfs->recursive = checkbut_with_value(NULL, tfs->bfwin ? tfs->bfwin->session->adv_open_recursive : TRUE);
	bf_mnemonic_label_tad_with_alignment(_("_Recursive:"), tfs->recursive, 0, 0.5, table, 1, 2, 5, 6);
	gtk_table_attach_defaults(GTK_TABLE(table), tfs->recursive, 2, 3, 5, 6);	
	
	/* content */
	gtk_table_set_row_spacing(GTK_TABLE(table), 5, 18);
	bf_label_tad_with_markup(_("<b>Contains</b>"), 0, 0.5, table, 0, 3, 6, 7);

	tfs->grep_pattern = entry_with_text(NULL, 255);
	bf_mnemonic_label_tad_with_alignment(_("Pa_ttern:"), tfs->grep_pattern, 0, 0.5, table, 1, 2, 7, 8);
	gtk_table_attach_defaults(GTK_TABLE(table), tfs->grep_pattern, 2, 4, 7, 8);
	
	tfs->is_regex = checkbut_with_value(NULL, 0);
	bf_mnemonic_label_tad_with_alignment(_("Is rege_x:"), tfs->is_regex, 0, 0.5, table, 1, 2, 8, 9);
	gtk_table_attach_defaults(GTK_TABLE(table), tfs->is_regex, 2, 3, 8, 9);
	
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
	gtk_window_set_transient_for(GTK_WINDOW(tfs->win), GTK_WINDOW(tfs->bfwin->main_window));
}

GList *return_files_advanced(Tbfwin *bfwin, gchar *tmppath) {
  Tfiles_advanced tfs = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, bfwin};
	if (tmppath) {
		GtkWidget *curdir = entry_with_text(tmppath, 255);
		tfs.basedir = curdir;
	}
		/* this is probably called different!! */
	files_advanced_win(&tfs);
	DEBUG_MSG("return_files_advanced, calling gtk_main()\n");
	gtk_main();
	return tfs.filenames_to_return;
}

void file_open_advanced_cb(GtkWidget * widget, Tbfwin *bfwin) {
	GList *tmplist;
	tmplist = return_files_advanced(bfwin, NULL);
	if (!tmplist) {
		info_dialog(bfwin->main_window, _("No matching files found"), NULL);
		return;
	}
	{
		gint len = g_list_length(tmplist);
		gchar *message = g_strdup_printf(_("Loading %d file(s)..."), len);
		statusbar_message(bfwin,message,2000+len*50);
		g_free(message);
		flush_queue();
	}
	docs_new_from_files(bfwin,tmplist, FALSE);
	free_stringlist(tmplist);
}

void open_advanced_from_filebrowser(Tbfwin *bfwin, gchar *path) {
	GList *tmplist;
	tmplist = return_files_advanced(bfwin, path);
	if (!tmplist) {
		return;
	}
	{
		gint len = g_list_length(tmplist);
		gchar *message = g_strdup_printf(_("Loading %d file(s)..."), len);
		statusbar_message(bfwin,message,2000+len*50);
		g_free(message);
		flush_queue();
	}
	docs_new_from_files(bfwin,tmplist,FALSE);
	free_stringlist(tmplist);
}

/*************** end of advanced open code *************/

/**
 * file_open_cb:
 * @widget: unused #GtkWidget
 * @bfwin: #Tbfwin* with the current window
 *
 * Prompt user for files to open.
 *
 * Return value: void
 **/
void file_open_cb(GtkWidget * widget, Tbfwin *bfwin) {
#ifdef HAVE_ATLEAST_GTK_2_4
	GtkWidget *dialog;
	dialog = file_chooser_dialog(bfwin, _("Select files"), GTK_FILE_CHOOSER_ACTION_OPEN, NULL, FALSE, TRUE, NULL);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		GSList *slist;
		bfwin->focus_next_new_doc = TRUE;
		slist = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));
		docs_new_from_uris(bfwin, slist, FALSE);
		g_slist_free(slist);
	}
	gtk_widget_destroy(dialog);
#else
	GList *tmplist = NULL;
	gint len;
	gchar *message;
	DEBUG_MSG("file_open_cb, started, calling return_files()\n");
	tmplist = return_files(NULL);
	len = g_list_length(tmplist)
	message = g_strdup_printf(_("Loading %d file(s)..."), len);
	statusbar_message(bfwin,message,2000+len*50);
	g_free(message);
	flush_queue();
	DEBUG_MSG("file_open_cb, calling docs_new_from_files()\n");
	docs_new_from_files(bfwin,tmplist, FALSE);
	free_stringlist(tmplist);
#endif
}

typedef struct {
	Tbfwin *bfwin;
	GtkWidget *win;
	GtkWidget *entry;
} Tou;
static void open_url_destroy_lcb(GtkWidget *widget, Tou *ou) {
	g_free(ou);
}
static void open_url_cancel_lcb(GtkWidget *widget, Tou *ou) {
	gtk_widget_destroy(ou->win);
}
static void open_url_ok_lcb(GtkWidget *widget, Tou *ou) {
	GnomeVFSURI *uri;
	gchar *url = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(ou->entry)->entry),0,-1);
	/*doc_new_with_file(ou->bfwin,url,FALSE,FALSE);*/
	uri = gnome_vfs_uri_new(url);
	if (uri) {
		file_doc_from_uri(ou->bfwin, uri, NULL);
	}
	g_free(url);
	gnome_vfs_uri_unref(uri);
	gtk_widget_destroy(ou->win);
}

/**
 * file_open_url_cb:
 * @widget: #GtkWidget* ignored
 * @bfwin: #Tbfwin* bfwin pointer
 *
 * opens a dialog where you can enter an URL to open of any kind
 * supported by gnome-vfs
 *
 * Return value: void
 **/
void file_open_url_cb(GtkWidget * widget, Tbfwin *bfwin) {
	GtkWidget *vbox, *hbox, *but;
	Tou *ou;
	GList *urlhistory = NULL, *tmplist = NULL;
	ou = g_new(Tou,1);
	ou->bfwin = bfwin;
	ou->win = window_full2(_("Open URL"), GTK_WIN_POS_CENTER_ON_PARENT, 12
			, G_CALLBACK(open_url_destroy_lcb), ou, TRUE, bfwin->main_window);
	gtk_widget_set_size_request(ou->win, 450, 140);
	vbox = gtk_vbox_new(FALSE,5);
	gtk_box_pack_start(GTK_BOX(vbox), bf_label_with_markup(_("<b>Open URL</b>")), FALSE, FALSE, 5);
	gtk_container_add(GTK_CONTAINER(ou->win),vbox);
	tmplist = g_list_first(bfwin->session->recent_files);
	while (tmplist) {
		if (NULL != strchr(tmplist->data, ':')) {
			urlhistory = g_list_append(urlhistory, g_strdup(tmplist->data));
		}
		tmplist = g_list_next(tmplist);
	}
	ou->entry = boxed_combo_with_popdown("", urlhistory, TRUE, vbox);
	free_stringlist(urlhistory);
/*	ou->entry = boxed_entry_with_text("", 255, vbox); */
	gtk_box_pack_start(GTK_BOX(vbox), gtk_hseparator_new(), FALSE, FALSE, 5);
	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	but = bf_stock_cancel_button(G_CALLBACK(open_url_cancel_lcb), ou);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, TRUE, 0);
	but = bf_stock_ok_button(G_CALLBACK(open_url_ok_lcb), ou);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, TRUE, 0);
	gtk_window_set_default(GTK_WINDOW(ou->win), but);
	gtk_widget_show_all(ou->win);
}

/* async save code */

static TcheckNsave_return doc_checkNsave_lcb(TcheckNsave_status status,gint error_info,gpointer data) {
	Tdocument *doc = data;
	gchar *errmessage;
	DEBUG_MSG("doc_checkNsave_lcb, doc=%p, status=%d\n",doc,status);
	switch (status) {
		case CHECKANDSAVE_ERROR_NOBACKUP:
			{
				return CHECKNSAVE_CONT;
			}
		break;
		case CHECKANDSAVE_ERROR:
		case CHECKANDSAVE_ERROR_NOCHANNEL:
		case CHECKANDSAVE_ERROR_NOWRITE:
			{
				DEBUG_MSG("doc_checkNsave_lcb, some ERROR, give user a message\n");
				errmessage = g_strconcat(_("Could not save file:\n\""), doc->uri, "\"", NULL);
				error_dialog(BFWIN(doc->bfwin)->main_window,_("File save aborted.\n"), errmessage);
				g_free(errmessage);
			}
		break;
		case CHECKANDSAVE_ERROR_MODIFIED:
			{
				return CHECKNSAVE_CONT;
			}
		case CHECKANDSAVE_CHECKED:
		case CHECKANDSAVE_BACKUP:
		case CHECKANDSAVE_CHANNEL_OPENED:
			/* do nothing, or perhaps we could change the notebook tab color ? */
		break;
		case CHECKANDSAVE_FINISHED:
			{
			GnomeVFSURI *uri;
			/* YES! we're done! update the fileinfo !*/
			DEBUG_MSG("doc_checkNsave_lcb, re-set async doc->fileinfo (current=%p)\n",doc->fileinfo);
			if (doc->fileinfo) gnome_vfs_file_info_unref(doc->fileinfo);
			doc->fileinfo = NULL;
			uri = gnome_vfs_uri_new(doc->uri);
			file_doc_fill_fileinfo(doc, uri);
			gnome_vfs_uri_unref(uri);
			}
		break;
	}
	return CHECKNSAVE_CONT;
}

static void doc_save_backend(Tdocument *doc, gboolean do_save_as, gboolean do_move, gboolean close_doc, gboolean close_window) {
	DEBUG_MSG("doc_save_backend, started for doc %p\n",doc);
	if (doc->filename == NULL) {
		do_save_as = 1;
	}
	if (do_move) {
		do_save_as = 1;
	}
/************ SHOULD BE REMOVED ************/
	if (doc->uri == NULL) {
		doc->uri = g_strdup(doc->filename);
	}
/************************/
	if (do_save_as) {
		gchar *newfilename;
		newfilename = ask_new_filename(BFWIN(doc->bfwin), doc->uri, gtk_label_get_text(GTK_LABEL(doc->tab_label)), do_move);
		if (!newfilename) {
			return;
		}
		if (doc->uri) {
			if (do_move) {
				gnome_vfs_unlink(doc->uri);
			}
			g_free(doc->uri);
		}
		doc->uri = newfilename;
		if (doc->filename) g_free(doc->filename);
		doc->filename = g_strdup(doc->uri);
	}
	{
		Trefcpointer *buffer;
		GtkTextIter itstart, itend;
		GnomeVFSURI *uri;
		uri = gnome_vfs_uri_new(doc->uri);
		gtk_text_buffer_get_bounds(doc->buffer,&itstart,&itend);

		buffer = refcpointer_new(gtk_text_buffer_get_text(doc->buffer,&itstart,&itend,FALSE));

		file_checkNsave_uri_async(uri, doc->fileinfo, buffer, strlen(buffer->data), !do_save_as, doc_checkNsave_lcb, doc);

		if (do_save_as) {
			doc_reset_filetype(doc, doc->uri, buffer->data);
			doc_set_title(doc);
		}
		refcpointer_unref(buffer);
		gnome_vfs_uri_unref(uri);
	}
}

/**
 * file_save_cb:
 * @widget: unused #GtkWidget
 * @bfwin: #Tbfwin* with the current window
 *
 * Save the current document.
 *
 * Return value: void
 **/
void file_save_cb(GtkWidget * widget, Tbfwin *bfwin) {
	doc_save_backend(bfwin->current_document, FALSE, FALSE, FALSE, FALSE);
}

/**
 * file_save_as_cb:
 * @widget: unused #GtkWidget
 * @bfwin: #Tbfwin* with the current window
 *
 * Save current document, let user choose filename.
 *
 * Return value: void
 **/
void file_save_as_cb(GtkWidget * widget, Tbfwin *bfwin) {
	doc_save_backend(bfwin->current_document, TRUE, FALSE, FALSE, FALSE);
}

/**
 * file_move_to_cb:
 * @widget: unused #GtkWidget
 * @bfwin: #Tbfwin* with the current window
 *
 * Move current document, let user choose filename.
 *
 * Return value: void
 **/
void file_move_to_cb(GtkWidget * widget, Tbfwin *bfwin) {
	doc_save_backend(bfwin->current_document, TRUE, TRUE, FALSE, FALSE);
}
