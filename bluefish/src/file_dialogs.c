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
#include "undo_redo.h"

/**************************************************************************/
/* the start of the callback functions for the menu, acting on a document */
/**************************************************************************/
typedef struct {
	GtkWidget *dialog;
	GtkWidget *basedir;
	GtkWidget *find_pattern;
	GtkWidget *recursive;
	GtkWidget *grep_pattern;
	GtkWidget *is_regex;
	Tbfwin *bfwin;
} Tfiles_advanced;

static void files_advanced_win_findpattern_changed(GtkComboBox *combobox, Tfiles_advanced *tfs) {
	if (strlen(gtk_entry_get_text (GTK_ENTRY (GTK_BIN (tfs->find_pattern)->child))) > 0) {
		gtk_dialog_set_response_sensitive (GTK_DIALOG (tfs->dialog), GTK_RESPONSE_ACCEPT, TRUE);
	} else {
		gtk_dialog_set_response_sensitive (GTK_DIALOG (tfs->dialog), GTK_RESPONSE_ACCEPT, FALSE);
	}
}

static void files_advanced_win_ok_clicked(Tfiles_advanced *tfs) {
	GnomeVFSURI *baseuri;
	gchar *basedir, *content_filter, *extension_filter;

	extension_filter = gtk_editable_get_chars (GTK_EDITABLE (GTK_BIN (tfs->find_pattern)->child), 0, -1);
	basedir = gtk_editable_get_chars (GTK_EDITABLE (tfs->basedir), 0, -1);
	baseuri = gnome_vfs_uri_new (basedir);
	content_filter = gtk_editable_get_chars (GTK_EDITABLE (tfs->grep_pattern), 0, -1);

	open_advanced(tfs->bfwin, baseuri, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tfs->recursive))
			, strlen(extension_filter) ==0 ? NULL : extension_filter
			, strlen(content_filter) ==0 ? NULL : content_filter
			, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tfs->is_regex)));

	g_free (basedir);
	g_free (content_filter);
	g_free (extension_filter);
	gnome_vfs_uri_unref (baseuri);
	
	tfs->bfwin->session->adv_open_recursive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(tfs->recursive));
}

static void files_advanced_win_select_basedir_lcb(GtkWidget * widget, Tfiles_advanced *tfs) {
	gchar *newdir = NULL;
	GtkWidget *dialog;

	dialog = file_chooser_dialog(tfs->bfwin, _("Select basedir"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, NULL, TRUE, FALSE, NULL);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		newdir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	}
	gtk_widget_destroy (dialog);

	if (newdir) {
		gtk_entry_set_text (GTK_ENTRY (tfs->basedir), newdir);
		g_free (newdir);
	}
}

void files_advanced_win(Tbfwin *bfwin, gchar *basedir) {
	GtkWidget *table;
	GtkListStore *lstore;
	GtkTreeIter iter;
	Tfiles_advanced *tfs;
	unsigned int i = 0;
	
	const gchar *fileExts[] = {
		"*.html",
		"*.htm",
		"*.php",
		"*.php3",
		"*.shtml",
		"*.pl",
		"*.cgi",
		"*.xml",
		"*.c",
		"*.h",
		"*.py",
		"*.java",
	};
	
	tfs = g_new (Tfiles_advanced, 1);
	tfs->bfwin = bfwin;

	tfs->dialog = gtk_dialog_new_with_buttons (_("Advanced open file selector"),
																GTK_WINDOW (tfs->bfwin->main_window),
																GTK_DIALOG_DESTROY_WITH_PARENT,
																GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
																GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
																NULL);	

	table = gtk_table_new (7, 5, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 10);
	gtk_table_set_row_spacings (GTK_TABLE (table), 12);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (tfs->dialog)->vbox), table, FALSE, FALSE, 0);

	bf_label_tad_with_markup(_("<b>General</b>"), 0, 0.5, table, 0, 3, 0, 1);
	if (!basedir) {
		tfs->basedir = entry_with_text(bfwin->session->opendir, 255);
	} else {
		tfs->basedir = entry_with_text(basedir, 255);
	}
	bf_mnemonic_label_tad_with_alignment(_("Base_dir:"), tfs->basedir, 0, 0.5, table, 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), tfs->basedir, 2, 4, 1, 2);
	gtk_table_attach(GTK_TABLE(table), bf_allbuttons_backend(_("_Browse..."), TRUE, 112, G_CALLBACK(files_advanced_win_select_basedir_lcb), tfs), 4, 5, 1, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);

	lstore = gtk_list_store_new (1, G_TYPE_STRING);
	for (i = 0; i < G_N_ELEMENTS (fileExts); i++) {
		gtk_list_store_append (GTK_LIST_STORE (lstore), &iter);
		gtk_list_store_set (GTK_LIST_STORE (lstore), &iter, 0, fileExts[i], -1);
	};
	tfs->find_pattern = gtk_combo_box_entry_new_with_model (GTK_TREE_MODEL (lstore), 0);
	g_object_unref (lstore);
	bf_mnemonic_label_tad_with_alignment(_("_Pattern:"), tfs->find_pattern, 0, 0.5, table, 1, 2, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(table), tfs->find_pattern, 2, 4, 2, 3);
	g_signal_connect (G_OBJECT (tfs->find_pattern), "changed", G_CALLBACK (files_advanced_win_findpattern_changed), tfs);

	tfs->recursive = checkbut_with_value(NULL, tfs->bfwin ? tfs->bfwin->session->adv_open_recursive : TRUE);
	bf_mnemonic_label_tad_with_alignment(_("_Recursive:"), tfs->recursive, 0, 0.5, table, 1, 2, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(table), tfs->recursive, 2, 3, 3, 4);	
	
	gtk_table_set_row_spacing(GTK_TABLE(table), 5, 18);
	bf_label_tad_with_markup(_("<b>Contains</b>"), 0, 0.5, table, 0, 3, 4, 5);

	tfs->grep_pattern = entry_with_text(NULL, 255);
	bf_mnemonic_label_tad_with_alignment(_("Pa_ttern:"), tfs->grep_pattern, 0, 0.5, table, 1, 2, 5, 6);
	gtk_table_attach_defaults(GTK_TABLE(table), tfs->grep_pattern, 2, 4, 5, 6);
	
	tfs->is_regex = checkbut_with_value(NULL, 0);
	bf_mnemonic_label_tad_with_alignment(_("Is rege_x:"), tfs->is_regex, 0, 0.5, table, 1, 2, 6, 7);
	gtk_table_attach_defaults(GTK_TABLE(table), tfs->is_regex, 2, 3, 6, 7);

	gtk_dialog_set_response_sensitive (GTK_DIALOG (tfs->dialog), GTK_RESPONSE_ACCEPT, FALSE);
	gtk_widget_show_all (GTK_DIALOG (tfs->dialog)->vbox);
		
	if (gtk_dialog_run (GTK_DIALOG (tfs->dialog)) == GTK_RESPONSE_ACCEPT) {
		files_advanced_win_ok_clicked(tfs);
	}
	
	gtk_widget_destroy (tfs->dialog);
	g_free (tfs);
}

void file_open_advanced_cb(GtkWidget * widget, Tbfwin *bfwin) {
	files_advanced_win(bfwin, NULL);
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
		doc_new_from_uri(ou->bfwin, NULL, uri, NULL, FALSE, FALSE, -1, -1);
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
		if (tmplist->data && strlen(tmplist->data) > 5 && strncmp(tmplist->data, "file:", 5)!=0) {
			urlhistory = g_list_prepend(urlhistory, g_strdup(tmplist->data));
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
			if (main_v->props.backup_abort_action == 0)  {
				return CHECKNSAVE_CONT;
			} else if (main_v->props.backup_abort_action == 1) {
				doc->action.save = NULL;
				gtk_text_view_set_editable(GTK_TEXT_VIEW(doc->view),TRUE);
				return CHECKNSAVE_STOP;
			} else /* if (main_v->props.backup_abort_action == 2) */{
				/* we have to ask the user */
				gchar *options[] = {_("_Abort save"), _("_Continue save"), NULL};
				gint retval;
				gchar *tmpstr = g_strdup_printf(_("A backupfile for %s could not be created. If you continue, this file will be overwritten."), gtk_label_get_text(GTK_LABEL(doc->tab_label)));
				retval = multi_warning_dialog(BFWIN(doc->bfwin)->main_window,_("File backup failure"), tmpstr, 1, 0, options);
				g_free(tmpstr);
				DEBUG_MSG("doc_checkNsave_lcb, retval=%d, returning %d\n", retval,(retval == 0) ? CHECKNSAVE_STOP : CHECKNSAVE_CONT);
				if (retval == 0) {
					doc->action.save = NULL;
					gtk_text_view_set_editable(GTK_TEXT_VIEW(doc->view),TRUE);
					return CHECKNSAVE_STOP;
				}
				return CHECKNSAVE_CONT;
			}
		break;
		case CHECKANDSAVE_ERROR:
		case CHECKANDSAVE_ERROR_NOCHANNEL:
		case CHECKANDSAVE_ERROR_NOWRITE:
		case CHECKANDSAVE_ERROR_CANCELLED:
			{
				DEBUG_MSG("doc_checkNsave_lcb, some ERROR, give user a message\n");
				errmessage = g_strconcat(_("Could not save file:\n\""),gtk_label_get_text(GTK_LABEL(doc->tab_label)), "\"", NULL);
				error_dialog(BFWIN(doc->bfwin)->main_window,_("File save failed!\n"), errmessage);
				g_free(errmessage);
				doc->action.save = NULL;
				gtk_text_view_set_editable(GTK_TEXT_VIEW(doc->view),TRUE);
			}
		break;
		case CHECKANDSAVE_ERROR_MODIFIED:
		case CHECKANDSAVE_ERROR_MODIFIED_FAILED:
			{
				/* we have to ask the user what to do */
				gchar *options[] = {_("_Abort save"), _("_Continue save"), NULL};
				gint retval;
				gchar *tmpstr;
				if (status == CHECKANDSAVE_ERROR_MODIFIED) {
					tmpstr = g_strdup_printf(_("File %s is modified on disk, overwrite?"), doc->uri);
				} else {
					tmpstr = g_strdup_printf(_("Failed to check if %s is modified on disk"), doc->uri);
				}
				retval = multi_warning_dialog(BFWIN(doc->bfwin)->main_window,_("File modified on disk"), tmpstr, 1, 0, options);
				g_free(tmpstr);
				if (retval == 0) {
					doc->action.save = NULL;
					gtk_text_view_set_editable(GTK_TEXT_VIEW(doc->view),TRUE);
					return CHECKNSAVE_STOP;
				}
				return CHECKNSAVE_CONT;
			}
		case CHECKANDSAVE_CHECKED:
		case CHECKANDSAVE_BACKUP:
		case CHECKANDSAVE_CHANNEL_OPENED:
			/* do nothing, or perhaps we could change the notebook tab color or a statusbar message? */
		break;
		case CHECKANDSAVE_FINISHED:
			/* if the user wanted to close the doc we should do very diffferent things here !! */
			doc->action.save = NULL;
			gtk_text_view_set_editable(GTK_TEXT_VIEW(doc->view),TRUE);
			if (doc->action.close_doc) {
				Tbfwin *bfwin = doc->bfwin;
				gboolean close_window = doc->action.close_window;
				doc_destroy(doc, doc->action.close_window);
				if (close_window && test_only_empty_doc_left(bfwin->documentlist)) {
					gtk_widget_destroy(bfwin->main_window);
				}
				return CHECKNSAVE_STOP; /* it actually doesn't matter what we return, this was the last callback anyway */
			} else {
				GnomeVFSURI *uri;
				/* YES! we're done! update the fileinfo !*/
				DEBUG_MSG("doc_checkNsave_lcb, re-set async doc->fileinfo (current=%p)\n",doc->fileinfo);
				if (doc->fileinfo) gnome_vfs_file_info_unref(doc->fileinfo);
				doc->fileinfo = NULL;
				uri = gnome_vfs_uri_new(doc->uri);
				file_doc_fill_fileinfo(doc, uri);
				gnome_vfs_uri_unref(uri);
				if (main_v->props.clear_undo_on_save) {
					doc_unre_clear_all(doc);
				}
				doc_set_modified(doc, 0);
			}
		break;
	}
	return CHECKNSAVE_CONT;
}

/**
 * ask_new_filename:
 * @bfwin: #Tbfwin* mainly used to set the dialog transient
 * @oldfilename: #gchar* with the old filename
 * @gui_name: #const gchar* with the name of the file used in the GUI
 * @is_move: #gboolean if the title should be move or save as
 *
 * returns a newly allocated string with a new filename
 *
 * if a file with the selected name name was
 * open already it will ask the user what to do, return NULL if
 * the user wants to abort, or will remove the name of the other file if the user wants
 * to continue
 *
 * Return value: gchar* with newly allocated string, or NULL on failure or abort
 **/
gchar *ask_new_filename(Tbfwin *bfwin,gchar *old_curi, const gchar *gui_name, gboolean is_move) {
	Tdocument *exdoc;
	GList *alldocs;
	gchar *new_curi = NULL;
	gchar *dialogtext;
	GtkWidget *dialog;
	
	dialogtext = g_strdup_printf((is_move) ? _("Move/rename %s to"): _("Save %s as"), gui_name);
	dialog = file_chooser_dialog(bfwin, dialogtext, GTK_FILE_CHOOSER_ACTION_SAVE, old_curi, FALSE, FALSE, NULL);
	g_free(dialogtext);
	
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		new_curi = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
	}
	gtk_widget_destroy(dialog);

	if (!new_curi) return NULL;
	if (old_curi && strcmp(old_curi,new_curi)==0) {
		g_free(new_curi);
		return NULL;
	}
	
	alldocs = return_allwindows_documentlist();
	exdoc = documentlist_return_document_from_filename(alldocs, new_curi);
	g_list_free(alldocs);
	DEBUG_MSG("ask_new_filename, exdoc=%p, newfilename=%s\n", exdoc, new_curi);
	if (exdoc) {
		gchar *tmpstr;
		gint retval;
		gchar *options[] = {_("_Cancel"), _("_Overwrite"), NULL};
		tmpstr = g_strdup_printf(_("File %s exists and is opened, overwrite?"), new_curi);
		retval = multi_warning_dialog(bfwin->main_window,tmpstr, _("The file you have selected is being edited in Bluefish."), 1, 0, options);
		g_free(tmpstr);
		if (retval == 0) {
			g_free(new_curi);
			return NULL;
		} else {
			document_unset_filename(exdoc);
		}
	} else {
		GnomeVFSURI *tmp;
		gboolean exists;
		tmp = gnome_vfs_uri_new(new_curi);
		exists = gnome_vfs_uri_exists(tmp);
		gnome_vfs_uri_unref(tmp);
		if (exists) {
			gchar *tmpstr;
			gint retval;
			gchar *options[] = {_("_Cancel"), _("_Overwrite"), NULL};
			tmpstr = g_strdup_printf(_("A file named \"%s\" already exists."), new_curi);
			retval = multi_warning_dialog(bfwin->main_window,tmpstr, 
											_("Do you want to replace the existing file?"), 1, 0, options);
			g_free(tmpstr);
			if (retval == 0) {
				g_free(new_curi);
				return NULL;
			}
		}
	}
	return new_curi;
}

void doc_save_backend(Tdocument *doc, gboolean do_save_as, gboolean do_move, gboolean close_doc, gboolean close_window) {
	DEBUG_MSG("doc_save_backend, started for doc %p\n",doc);
	if(doc->action.save) {
		gchar *errmessage;
		/* this message is not in very nice english I'm afraid */
		errmessage = g_strconcat(_("File:\n\""),gtk_label_get_text(GTK_LABEL(doc->tab_label)), "\" save is in progress", NULL);
		error_dialog(BFWIN(doc->bfwin)->main_window,_("Save in progress!"), errmessage);
		g_free(errmessage);
		return;
	}
	if (doc->uri == NULL) {
		do_save_as = 1;
	}
	if (do_move) {
		do_save_as = 1;
	}
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
	}
	session_set_savedir(doc->bfwin, doc->uri);
	{
		gchar *tmp;
		Trefcpointer *buffer;
		GnomeVFSURI *uri;
		tmp = doc_get_buffer_in_encoding(doc);
		if (!tmp) {
			return;
		}

		buffer = refcpointer_new(tmp);
		uri = gnome_vfs_uri_new(doc->uri);
		doc->action.close_doc = close_doc;
		doc->action.close_window = close_window;
		gtk_text_view_set_editable(GTK_TEXT_VIEW(doc->view),FALSE);
		doc->action.save = file_checkNsave_uri_async(uri, doc->fileinfo, buffer, strlen(buffer->data), !do_save_as, doc_checkNsave_lcb, doc);

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

/**
 * file_save_all_cb:
 * @widget: unused #GtkWidget
 * @bfwin: the #Tbfwin* window pointer
 *
 *	Save all editor notebooks
 *
 * Return value: void
 **/
void file_save_all_cb(GtkWidget * widget, Tbfwin *bfwin) {
	GList *tmplist;
	Tdocument *tmpdoc;

	tmplist = g_list_first(bfwin->documentlist);
	while (tmplist) {
		tmpdoc = (Tdocument *) tmplist->data;
		if (tmpdoc->modified) {
			doc_save_backend(tmpdoc, FALSE, FALSE, FALSE, FALSE);
		}
		tmplist = g_list_next(tmplist);
	}
}

void doc_close_single_backend(Tdocument *doc, gboolean delay_activate, gboolean close_window) {
	Tbfwin *bfwin = doc->bfwin;
	if (doc->action.checkmodified) checkmodified_cancel(doc->action.checkmodified);
	if (doc->action.load != NULL || doc->action.info != NULL) {
		/* we should cancel the action now..., and then let the callbacks close it...
		the order is important, because the info callback will not close the document, 
		only the load callback will call doc_close_single_backend */
		doc->action.close_doc = TRUE;
		doc->action.close_window = close_window;
   	if (doc->action.info) file_asyncfileinfo_cancel(doc->action.info);
		if (doc->action.load) file2doc_cancel(doc->action.load);
		/* we will not cancel save operations, because it might corrupt the file, let 
		them just timeout */
		DEBUG_MSG("doc_close_single_backend, cancelled load/info and set close_doc to TRUE, returning now\n");
		return;
	}
	if (doc_is_empty_non_modified_and_nameless(doc) && g_list_length(BFWIN(doc->bfwin)->documentlist) <=1) {
		if (close_window) {
			gtk_widget_destroy(BFWIN(doc->bfwin)->main_window);
		}
		return;
	}
	if (doc->modified) {
		gchar *buttons[] = {_("Do_n't save"), GTK_STOCK_CANCEL, GTK_STOCK_SAVE, NULL};
		gchar *text;
		gint retval;
		text = g_strdup_printf(_("Save changes to \"%s\" before closing?."),gtk_label_get_text (GTK_LABEL (doc->tab_label)));
		retval = multi_query_dialog(BFWIN(doc->bfwin)->main_window, text, 
						_("If you don't save your changes they will be lost."), 2, 1, buttons);
		g_free(text);
		switch (retval) {
		case 0:
			doc_destroy(doc, close_window || delay_activate);
		break;
		case 1:
			return;
		break;
		case 2:
			doc_save_backend(doc, FALSE, FALSE, TRUE, close_window);
		break;
		}
	} else {
		doc_destroy(doc, close_window||delay_activate);
	}
	if (close_window && bfwin->documentlist == NULL) { /* the documentlist is empty */
		gtk_widget_destroy(bfwin->main_window);
	}
	DEBUG_MSG("doc_close_single_backend, finished!\n");
}

/**
 * file_close_cb:
 * @widget: unused #GtkWidget
 * @data: unused #gpointer
 *
 * Close the current document.
 *
 * Return value: void
 **/
void file_close_cb(GtkWidget * widget, Tbfwin *bfwin) {
	doc_close_single_backend(bfwin->current_document, FALSE, FALSE);
}

void doc_close_multiple_backend(Tbfwin *bfwin, gboolean close_window) {
	GList *tmplist, *duplist;
	Tdocument *tmpdoc;
	gint retval=1; /* the defauilt value 1 means "close all" */

	if (g_list_length (bfwin->documentlist) == 1) {
		doc_close_single_backend(bfwin->current_document, FALSE, close_window);
		return;
	}
	/* first a warning loop */
	if (test_docs_modified(bfwin->documentlist)) {
		gchar *options[] = {_("_Save All"), _("Close _All"), _("Choose per _File"), _("_Cancel"), NULL};
		retval = multi_query_dialog(bfwin->main_window,_("Multiple open files have been changed."), 
									_("If you don't save your changes they will be lost."), 3, 3, options);
		if (retval == 3) {
			return;
		}
	}
	
	/* we duplicate the documentlist so we can safely walk trough the list, in 
	our duplicate list there is no chance that the list is changed during the time
	we walk the list */
	duplist = g_list_copy(bfwin->documentlist);
	tmplist = g_list_first(duplist);
	while (tmplist) {
		tmpdoc = (Tdocument *) tmplist->data;
		switch (retval) {
		case 0: /* save all */
			doc_save_backend(tmpdoc, FALSE, FALSE, TRUE, close_window);
		break;
		case 1: /* close all */
		   /* fake that this document was not modified */
			tmpdoc->modified = FALSE;
  			doc_close_single_backend(tmpdoc, TRUE, close_window);
		break;
		case 2: /* choose per file */
			doc_close_single_backend(tmpdoc, TRUE, close_window);
		break;
		}
		tmplist = g_list_next(tmplist);
	}
	g_list_free(duplist);
	DEBUG_MSG("doc_close_multiple_backend, after the loop, len(documentlist)=%d\n",g_list_length(bfwin->documentlist));
/*	if (retval == 1) {
		if (close_window
			&& (bfwin->documentlist == NULL 
				|| (doc_is_empty_non_modified_and_nameless(bfwin->current_document) 
					&& g_list_length(bfwin->documentlist) <=1))) {
			gtk_widget_destroy(bfwin->main_window);
		} else {
			notebook_changed(bfwin,-1);
		}
	}*/
	DEBUG_MSG("doc_close_multiple_backend, finished\n");
}

/**
 * file_close_all_cb:
 * @widget: unused #GtkWidget
 * @bfwin: #Tbfwin* 
 *
 * Close all open files. Prompt user when neccessary.
 *
 * Return value: void
 **/
void file_close_all_cb(GtkWidget * widget, Tbfwin *bfwin) {
	doc_close_multiple_backend(bfwin, FALSE);
}

/**
 * file_new_cb:
 * @windget: #GtkWidget* ignored
 * @bfwin: Tbfwin* where to open the new document
 *
 * Create a new, empty file in window bfwin
 *
 * Return value: void
 **/
void file_new_cb(GtkWidget *widget, Tbfwin *bfwin) {
	Tdocument *doc;
	doc = doc_new(bfwin, FALSE);
	switch_to_document_by_pointer(bfwin,doc);
 	if (bfwin->project && bfwin->project->template && strlen(bfwin->project->template) > 2) {
		GnomeVFSURI *uri;
		uri = gnome_vfs_uri_new(bfwin->project->template);
		if (uri) {
			file_into_doc(bfwin->current_document, uri);
			gnome_vfs_uri_unref(uri);
		}
		/*doc_file_to_textbox(doc, bfwin->project->template, FALSE, FALSE);
		doc_activate(doc);*/
 	}
}
