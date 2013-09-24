/* Bluefish HTML Editor
 * file_dialogs.c - file dialogs
 *
 * Copyright (C) 2005-2012 Olivier Sessink
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

/* indented with indent -ts4 -kr -l110   */
/*#define DEBUG*/

#include <gtk/gtk.h>
#include <string.h>				/* memcpy */
#include <time.h>				/* strftime() */

#include "bluefish.h"
#include "file_dialogs.h"
#include "bfwin.h"
#include "bookmark.h"
#include "dialog_utils.h"
#include "document.h"
#include "doc_text_tools.h"
#include "file_autosave.h"
#include "file.h"
#include "filebrowser2.h"
#include "gtk_easy.h"
#include "snr3.h"				/* snr3_run_extern_replace() */
#include "stringlist.h"
#include "undo_redo.h"

static gchar *modified_on_disk_warning_string(const gchar * filename, GFileInfo * oldfinfo,
											  GFileInfo * newfinfo);

/**************************************************************************/
/* the start of the callback functions for the menu, acting on a document */
/**************************************************************************/
typedef struct {
	GtkWidget *dialog;
	GtkWidget *basedir;
	GtkWidget *find_pattern;
	GtkWidget *matchname;
	GtkWidget *recursive;
	GtkWidget *max_recursion;
	GtkWidget *grep_pattern;
	GtkWidget *is_regex;
	GtkWidget *regexwarn;
	Tbfwin *bfwin;
} Tfiles_advanced;

static void
files_advanced_win_findpattern_changed(GtkComboBox * combobox, Tfiles_advanced * tfs)
{
	if (strlen(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(tfs->find_pattern))))) > 0) {
		gtk_dialog_set_response_sensitive(GTK_DIALOG(tfs->dialog), GTK_RESPONSE_ACCEPT, TRUE);
	} else {
		gtk_dialog_set_response_sensitive(GTK_DIALOG(tfs->dialog), GTK_RESPONSE_ACCEPT, FALSE);
	}
}

static gboolean
files_advanced_win_ok_clicked(Tfiles_advanced * tfs)
{
	GFile *baseuri;
	gchar *basedir, *content_filter, *extension_filter;
	gboolean retval;
	GError *gerror = NULL;
	extension_filter =
		gtk_editable_get_chars(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(tfs->find_pattern))), 0, -1);
	basedir = gtk_editable_get_chars(GTK_EDITABLE(tfs->basedir), 0, -1);
	baseuri = g_file_new_for_uri(basedir);
	content_filter = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(tfs->grep_pattern));
	if (content_filter && content_filter[0]!='\0')
		tfs->bfwin->session->searchlist = add_to_history_stringlist(tfs->bfwin->session->searchlist, content_filter, TRUE);
	if (extension_filter && extension_filter[0] != '\0')
		tfs->bfwin->session->filegloblist = add_to_history_stringlist(tfs->bfwin->session->filegloblist, extension_filter, TRUE);

	retval =
		open_advanced(tfs->bfwin, baseuri, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tfs->recursive))
					  , 500, !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tfs->matchname))
					  ,
					  strlen(extension_filter) == 0 ? NULL : extension_filter,
					  strlen(content_filter) == 0 ? NULL : content_filter,
					  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tfs->is_regex)), &gerror);
	if (!retval && gerror) {
		gtk_label_set_line_wrap(GTK_LABEL(tfs->regexwarn), TRUE);
		gtk_label_set_text(GTK_LABEL(tfs->regexwarn), gerror->message);
		g_error_free(gerror);
	}
	g_free(basedir);
	g_free(content_filter);
	g_free(extension_filter);
	g_object_unref(baseuri);

	tfs->bfwin->session->adv_open_recursive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tfs->recursive));
	tfs->bfwin->session->adv_open_matchname =
		!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tfs->matchname));
	if (retval)
		bfwin_statusbar_message(tfs->bfwin,_("Started open advanced..."), 1);
	return retval;
}

static void
files_advanced_win_select_basedir_lcb(GtkWidget * widget, Tfiles_advanced * tfs)
{
	gchar *newdir = NULL;
	GtkWidget *dialog;

	dialog =
		file_chooser_dialog(tfs->bfwin, _("Select basedir"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, (gchar *)
							gtk_entry_get_text(GTK_ENTRY(tfs->basedir)), TRUE, FALSE, NULL, FALSE);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		newdir = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
	}
	gtk_widget_destroy(dialog);

	if (newdir) {
		gtk_entry_set_text(GTK_ENTRY(tfs->basedir), newdir);
		g_free(newdir);
	}
}

void
files_advanced_win(Tbfwin * bfwin, gchar * basedir)
{
	GtkWidget *alignment, *button, *carea, *table, *vbox, *vbox2;
	Tfiles_advanced *tfs;

	tfs = g_new(Tfiles_advanced, 1);
	tfs->bfwin = bfwin;

	tfs->dialog = gtk_dialog_new_with_buttons(_("Advanced open file selector"),
											  GTK_WINDOW(tfs->bfwin->main_window),
											  GTK_DIALOG_DESTROY_WITH_PARENT,
											  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
											  GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

#if !GTK_CHECK_VERSION(3, 0, 0)
	gtk_dialog_set_has_separator(GTK_DIALOG(tfs->dialog), FALSE);
#endif /* gtk3 */
	carea = gtk_dialog_get_content_area(GTK_DIALOG(tfs->dialog));

	alignment = gtk_alignment_new(0, 0, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 12, 0, 12, 6);
	gtk_box_pack_start(GTK_BOX(carea), alignment, FALSE, FALSE, 0);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(alignment), vbox);

	vbox2 = dialog_vbox_labeled(_("<b>Files</b>"), vbox);

	table = dialog_table_in_vbox(2, 6, 0, vbox2, FALSE, FALSE, 6);

	if (!basedir) {
		tfs->basedir = dialog_entry_in_table(bfwin->session->opendir, table, 1, 5, 0, 1);
	} else {
		tfs->basedir = dialog_entry_in_table(basedir, table, 1, 5, 0, 1);
	}
	dialog_mnemonic_label_in_table(_("Base _Dir:"), tfs->basedir, table, 0, 1, 0, 1);

	button =
		dialog_button_new_with_image_in_table(NULL, GTK_STOCK_OPEN, G_CALLBACK(files_advanced_win_select_basedir_lcb), tfs,
						TRUE, FALSE,
						table, 5, 6, 0,1);


	/*lstore = gtk_list_store_new(1, G_TYPE_STRING);
	for (tmplist = g_list_first(bfwin->session->filegloblist); tmplist; tmplist = g_list_next(tmplist)) {
		gtk_list_store_append(GTK_LIST_STORE(lstore), &iter);
		gtk_list_store_set(GTK_LIST_STORE(lstore), &iter, 0, tmplist->data, -1);
	}
	tfs->find_pattern = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(lstore), 0);
	g_object_unref(lstore);*/
	tfs->find_pattern = combobox_with_popdown("", bfwin->session->filegloblist, TRUE);
	dialog_mnemonic_label_in_table(_("_Pattern:"), tfs->find_pattern, table, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), tfs->find_pattern, 1, 5, 1, 2);
	g_signal_connect(G_OBJECT(tfs->find_pattern), "changed",
					 G_CALLBACK(files_advanced_win_findpattern_changed), tfs);

	table = dialog_table_in_vbox(3, 2, 0, vbox2, FALSE, FALSE, 0);

	tfs->matchname = checkbut_with_value(NULL, tfs->bfwin ? !tfs->bfwin->session->adv_open_matchname : FALSE);
	dialog_mnemonic_label_in_table(_("Pattern matches _full path:"), tfs->matchname, table, 0, 1, 0, 1);
	gtk_table_attach(GTK_TABLE(table), tfs->matchname, 1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);

	tfs->recursive = checkbut_with_value(NULL, tfs->bfwin ? tfs->bfwin->session->adv_open_recursive : TRUE);
	dialog_mnemonic_label_in_table(_("_Recursive:"), tfs->recursive, table, 0, 1, 1, 2);
	gtk_table_attach(GTK_TABLE(table), tfs->recursive, 1, 2, 1, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);

	tfs->max_recursion = spinbut_with_value("100", 1, 100000, 1, 10);
	dialog_mnemonic_label_in_table(_("Ma_x recursion:"), tfs->max_recursion, table, 0, 1, 2, 3);
	gtk_table_attach(GTK_TABLE(table), tfs->max_recursion, 1, 2, 2, 3, GTK_SHRINK, GTK_SHRINK, 0, 0);

	alignment = gtk_alignment_new(0, 0, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 12, 18, 12, 6);
	gtk_box_pack_start(GTK_BOX(carea), alignment, FALSE, FALSE, 0);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(alignment), vbox);

	vbox2 = dialog_vbox_labeled(_("<b>Contains</b>"), vbox);

	table = dialog_table_in_vbox(2, 4, 0, vbox2, FALSE, FALSE, 6);

	tfs->grep_pattern = combobox_with_popdown("", bfwin->session->searchlist, TRUE);
	dialog_mnemonic_label_in_table(_("Pa_ttern:"), tfs->grep_pattern, table, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), tfs->grep_pattern, 1, 4, 0, 1);

	tfs->is_regex = checkbut_with_value(NULL, 0);
	dialog_mnemonic_label_in_table(_("Is rege_x:"), tfs->is_regex, table, 0, 1, 1, 2);
	gtk_table_attach(GTK_TABLE(table), tfs->is_regex, 1, 2, 1, 2, GTK_FILL, GTK_SHRINK, 0, 0);

	tfs->regexwarn = gtk_label_new(NULL);
	gtk_table_attach(GTK_TABLE(table), tfs->regexwarn, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);

	gtk_dialog_set_response_sensitive(GTK_DIALOG(tfs->dialog), GTK_RESPONSE_ACCEPT, FALSE);
	gtk_widget_show_all(carea);

	while (gtk_dialog_run(GTK_DIALOG(tfs->dialog)) == GTK_RESPONSE_ACCEPT) {
		if (files_advanced_win_ok_clicked(tfs))
			break;
	}

	gtk_widget_destroy(tfs->dialog);
	g_free(tfs);
}

void
file_open_advanced_cb(GtkWidget * widget, Tbfwin * bfwin)
{
	files_advanced_win(bfwin, NULL);
}

/*************** end of advanced open code *************/

static void
file_open_ok_lcb(GtkDialog * dialog, gint response, Tbfwin * bfwin)
{
	if (response == GTK_RESPONSE_ACCEPT) {
		GSList *slist, *tmpslist;
		GtkComboBox *combo;
		bfwin->focus_next_new_doc = TRUE;
		combo = g_object_get_data(G_OBJECT(dialog), "encodings");
		if (combo) {
			GtkTreeIter iter;
			if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter)) {
				GtkTreeModel *model;
				gchar **arr;
				model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
				gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 1, &arr, -1);
				if (bfwin->session->encoding)
					g_free(bfwin->session->encoding);
				if (arr) {
					bfwin->session->encoding = g_strdup(arr[1]);
				} else {
					bfwin->session->encoding = NULL;
				}
				DEBUG_MSG("file_open_ok_lcb, session encoding is set to %s\n", bfwin->session->encoding);
			}
		}

		tmpslist = slist = gtk_file_chooser_get_files(GTK_FILE_CHOOSER(dialog));
		while (tmpslist) {
			doc_new_from_uri(bfwin, (GFile *) tmpslist->data, NULL, (slist->next != NULL), FALSE, -1, -1, -1, TRUE, FALSE);
			g_object_unref((GFile *) tmpslist->data);
			tmpslist = tmpslist->next;
		}
		g_slist_free(slist);
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

void
file_open_doc(Tbfwin * bfwin)
{
	GtkWidget *dialog;

	dialog =
		file_chooser_dialog(bfwin, _("Select files"), GTK_FILE_CHOOSER_ACTION_OPEN, NULL, FALSE, TRUE, NULL,
							TRUE);
	g_signal_connect(dialog, "response", G_CALLBACK(file_open_ok_lcb), bfwin);
	gtk_widget_show_all(dialog);
}

/**
 * file_open_cb:
 * @widget: unused #GtkWidget
 * @bfwin: #Tbfwin* with the current window
 *
 * Prompt user for files to open.
 *
 * Return value: void
 **/
void
file_open_cb(GtkWidget * widget, Tbfwin * bfwin)
{
	GtkWidget *dialog;
	dialog =
		file_chooser_dialog(bfwin, _("Select files"), GTK_FILE_CHOOSER_ACTION_OPEN, NULL, FALSE, TRUE, NULL,
							TRUE);
	g_signal_connect(dialog, "response", G_CALLBACK(file_open_ok_lcb), bfwin);
	gtk_widget_show_all(dialog);
/*  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    GSList *slist;
    bfwin->focus_next_new_doc = TRUE;
    slist = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));
    docs_new_from_uris(bfwin, slist, FALSE);
    g_slist_free(slist);
  }
  gtk_widget_destroy(dialog);*/
}

typedef struct {
	Tbfwin *bfwin;
	GtkWidget *win;
	GtkWidget *entry;
} Tou;
static void
open_url_destroy_lcb(GtkWidget * widget, Tou * ou)
{
	g_free(ou);
}

static void
open_url_cancel_lcb(GtkWidget * widget, Tou * ou)
{
	gtk_widget_destroy(ou->win);
}

static void
open_url_ok_lcb(GtkWidget * widget, Tou * ou)
{
	gchar *url = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(ou->entry));
	DEBUG_MSG("open_url_ok_lcb, url=%s\n", url);
	doc_new_from_input(ou->bfwin, url, FALSE, FALSE, -1);
	g_free(url);
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
void
file_open_url_cb(GtkAction * action, Tbfwin * bfwin)
{
	GtkWidget *align, *vbox, *hbox, *but;
	Tou *ou;
	GList *urlhistory = NULL, *tmplist = NULL;
	ou = g_new(Tou, 1);
	ou->bfwin = bfwin;
	ou->win =
		window_full2(_("Open URL"), GTK_WIN_POS_CENTER_ON_PARENT, 12, G_CALLBACK(open_url_destroy_lcb), ou,
					 TRUE, bfwin->main_window);
	gtk_widget_set_size_request(ou->win, 450, -1);
	vbox = gtk_vbox_new(FALSE, 5);
#if !GLIB_CHECK_VERSION(2, 18, 0)
	if (glib_major_version == 2 && glib_minor_version < 18) {
		gchar *message;
		GtkWidget *label = gtk_label_new(NULL);
		message =
			g_strdup_printf(_
							("<b>Your glib version (%d.%d.%d) works unreliable with remote files (smb, ftp, sftp, webdav etc.). Please upgrade to a glib version newer than 2.18.0 if you rely on remote file support.</b>"),
							glib_major_version, glib_minor_version, glib_micro_version);
		gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
		gtk_label_set_markup(GTK_LABEL(label), message);
		g_free(message);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 4);
	}
#endif
/*	gtk_box_pack_start(GTK_BOX(vbox), bf_label_with_markup(_("<b>Open URL</b>")), FALSE, FALSE, 5);*/
	gtk_container_add(GTK_CONTAINER(ou->win), vbox);
	tmplist = g_list_first(bfwin->session->recent_files);
	while (tmplist) {
		if (tmplist->data && strlen(tmplist->data) > 5 && strncmp(tmplist->data, "file:", 5) != 0) {
			urlhistory = g_list_prepend(urlhistory, g_strdup(tmplist->data));
		}
		tmplist = g_list_next(tmplist);
	}
	ou->entry = boxed_combobox_with_popdown("", urlhistory, TRUE, vbox);
	free_stringlist(urlhistory);
/*  ou->entry = boxed_entry_with_text("", 255, vbox); */

	align = gtk_alignment_new(0.0, 1.0, 1.0, 0.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align), 12, 0 ,0, 0);
	gtk_box_pack_start(GTK_BOX(vbox), align, FALSE, FALSE, 0);

#if GTK_CHECK_VERSION(3,0,0)
	hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
	hbox = gtk_hbutton_box_new();
#endif
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(hbox), 6);
	gtk_container_add(GTK_CONTAINER(align), hbox);
	but = bf_stock_cancel_button(G_CALLBACK(open_url_cancel_lcb), ou);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, TRUE, 0);
	but = bf_stock_ok_button(G_CALLBACK(open_url_ok_lcb), ou);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, TRUE, 0);
	gtk_window_set_default(GTK_WINDOW(ou->win), but);
	gtk_widget_show_all(ou->win);
}

/***********************************/
/*        async save code          */

typedef struct {
	Tdocument *doc;
	GFile *unlink_uri;
	GFile *fbrefresh_uri;
	Tdocsave_mode savemode;
} Tdocsavebackend;

static void
docsavebackend_cleanup(Tdocsavebackend * dsb)
{
	if (dsb->unlink_uri)
		g_object_unref(dsb->unlink_uri);
	if (dsb->fbrefresh_uri)
		g_object_unref(dsb->fbrefresh_uri);
	g_free(dsb);
}

static void
docsavebackend_async_unlink_lcb(gpointer data)
{
	Tdocsavebackend *dsb = data;
	fb2_refresh_parent_of_uri(dsb->unlink_uri);
	docsavebackend_cleanup(dsb);
}

static TcheckNsave_return
doc_checkNsave_lcb(TcheckNsave_status status, GError * gerror, gpointer data)
{
	Tdocsavebackend *dsb = data;
	Tdocument *doc = dsb->doc;
	gchar *errmessage;
	DEBUG_MSG("doc_checkNsave_lcb, doc=%p, status=%d, doc->uri=%p\n", doc, status, doc->uri);
	switch (status) {
	case CHECKANDSAVE_ERROR_NOBACKUP:
		if (main_v->props.backup_abort_action == 0) {
			return CHECKNSAVE_CONT;
		} else if (main_v->props.backup_abort_action == 1) {
			doc->save = NULL;
			gtk_text_view_set_editable(GTK_TEXT_VIEW(doc->view), TRUE);
			return CHECKNSAVE_STOP;
		} else {				/* if (main_v->props.backup_abort_action == 2) */

			/* we have to ask the user */
			const gchar *buttons[] = { _("_Abort save"), _("_Continue save"), NULL };
			gint retval;
			gchar *tmpstr =
				g_strdup_printf(_
								("A backupfile for %s could not be created. If you continue, this file will be overwritten."),
gtk_label_get_text(GTK_LABEL(doc->tab_label)));
			retval =
				message_dialog_new_multi(BFWIN(doc->bfwin)->main_window, GTK_MESSAGE_WARNING, buttons,
										 _("File backup failure"), tmpstr);
			g_free(tmpstr);
			DEBUG_MSG("doc_checkNsave_lcb, retval=%d, returning %d\n", retval,
					  (retval == 0) ? CHECKNSAVE_STOP : CHECKNSAVE_CONT);
			if (retval == 0) {
				doc->save = NULL;
				gtk_text_view_set_editable(GTK_TEXT_VIEW(doc->view), TRUE);
				return CHECKNSAVE_STOP;
			}
			return CHECKNSAVE_CONT;
		}
		break;
	case CHECKANDSAVE_ERROR:
	case CHECKANDSAVE_ERROR_NOWRITE:
		{
			errmessage =
				g_strconcat(_("Could not save file "), gtk_label_get_text(GTK_LABEL(doc->tab_label)), NULL);
			message_dialog_new(BFWIN(doc->bfwin)->main_window, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
							   errmessage, gerror->message);
			g_free(errmessage);
		}
		/* no break - fall through */
	case CHECKANDSAVE_ERROR_CANCELLED:
		doc->save = NULL;
		gtk_text_view_set_editable(GTK_TEXT_VIEW(doc->view), TRUE);
		docsavebackend_cleanup(dsb);
		break;
	case CHECKANDSAVE_ERROR_MODIFIED:
		{
			/* we have to ask the user what to do */
			const gchar *buttons[] = { _("_Abort save"), _("_Continue save"), NULL };
			GFileInfo *newfinfo;
			GError *gerror = NULL;
			gint retval;
			gchar *tmpstr, *utf8uri;

			newfinfo =
				g_file_query_info(doc->uri, G_FILE_ATTRIBUTE_STANDARD_SIZE "," G_FILE_ATTRIBUTE_TIME_MODIFIED,
								  0, NULL, &gerror);
			if (gerror) {
				g_warning("file was modified on disk, but now an error??");
				g_error_free(gerror);
				return CHECKNSAVE_CONT;
			}
			utf8uri = g_file_get_uri(doc->uri);
			tmpstr = modified_on_disk_warning_string(utf8uri, doc->fileinfo, newfinfo);
			/*g_strdup_printf(_("File %s has been modified on disk, overwrite?"), utf8uri); */
			g_free(utf8uri);
			g_object_unref(newfinfo);
			retval =
				message_dialog_new_multi(BFWIN(doc->bfwin)->main_window, GTK_MESSAGE_WARNING, buttons,
										 _("File changed on disk\n"), tmpstr);
			g_free(tmpstr);
			if (retval == 0) {
				doc->save = NULL;
				gtk_text_view_set_editable(GTK_TEXT_VIEW(doc->view), TRUE);
				return CHECKNSAVE_STOP;
			}
			return CHECKNSAVE_CONT;
		}
	case CHECKANDSAVE_FINISHED:
		if (dsb->unlink_uri) {
			file_delete_async(dsb->unlink_uri, FALSE, docsavebackend_async_unlink_lcb, dsb);
		}
		/* if the user wanted to close the doc we should do very diffferent things here !! */
		doc->save = NULL;
		if (doc->close_doc) {
			Tbfwin *bfwin = doc->bfwin;
			gboolean close_window = doc->close_window;
			doc_destroy(doc, doc->close_window);
			if (close_window && test_only_empty_doc_left(bfwin->documentlist)) {
				bfwin_destroy_and_cleanup(bfwin);
			}
			return CHECKNSAVE_STOP;	/* it actually doesn't matter what we return, this was the last callback anyway */
		} else {
			/* YES! we're done! update the fileinfo ! */
			gtk_text_view_set_editable(GTK_TEXT_VIEW(doc->view), TRUE);
			if (dsb->savemode != docsave_copy) {
				DEBUG_MSG("doc_checkNsave_lcb, re-set async doc->fileinfo (current=%p) for uri %p\n", doc->fileinfo, doc->uri);
				if (doc->fileinfo)
					g_object_unref(doc->fileinfo);
				doc->fileinfo = NULL;
				file_doc_fill_fileinfo(doc, doc->uri);
				if (main_v->props.clear_undo_on_save) {
					doc_unre_clear_all(doc);
				} else {
					doc_unre_clear_not_modified(doc);
				}
				doc_set_modified(doc, 0);
			}
			/* in fact the filebrowser should also be refreshed if the document was closed, but
			   when a document is closed, the filebrowser is anyway refreshed (hmm perhaps only if
			   'follow document focus' is set). */
			if (dsb->unlink_uri && dsb->fbrefresh_uri) {
				GFile *parent1, *parent2;
				parent1 = g_file_get_parent(dsb->unlink_uri);
				parent2 = g_file_get_parent(dsb->fbrefresh_uri);
				if (!g_file_equal(parent1, parent2)) {
					/* if they are equal, the directory will be refreshed by the unlink callback */
					fb2_refresh_dir_from_uri(parent2);
				}
				g_object_unref(parent1);
				g_object_unref(parent2);
			} else if (dsb->fbrefresh_uri) {
				fb2_refresh_parent_of_uri(dsb->fbrefresh_uri);
			}
		}
		if (!dsb->unlink_uri) {
			/* if there is an unlink uri, that means the unlink callback will free the dsb structure */
			docsavebackend_cleanup(dsb);
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
gchar *
ask_new_filename(Tbfwin * bfwin, const gchar * old_curi, const gchar *dialogtext)
{
	Tdocument *exdoc;
	GList *alldocs;
	gchar *new_curi = NULL;
	GFile *uri;
	GtkWidget *dialog;


	dialog =
		file_chooser_dialog(bfwin, dialogtext, GTK_FILE_CHOOSER_ACTION_SAVE, old_curi, FALSE, FALSE, NULL,
							FALSE);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		new_curi = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
	}
	gtk_widget_destroy(dialog);

	if (!new_curi)
		return NULL;
	if (old_curi && strcmp(old_curi, new_curi) == 0) {
		g_free(new_curi);
		return NULL;
	}

	alldocs = return_allwindows_documentlist();
	uri = g_file_new_for_uri(new_curi);
	exdoc = documentlist_return_document_from_uri(alldocs, uri);
	g_object_unref(uri);
	g_list_free(alldocs);
	DEBUG_MSG("ask_new_filename, exdoc=%p, newfilename=%s\n", exdoc, new_curi);
	if (exdoc) {
		gchar *tmpstr;
		gint retval;
		const gchar *buttons[] = { _("_Cancel"), _("_Overwrite"), NULL };
		tmpstr = g_strdup_printf(_("File %s exists and is open, overwrite?"), new_curi);
		retval =
			message_dialog_new_multi(bfwin->main_window, GTK_MESSAGE_WARNING, buttons, tmpstr,
									 _("The file you have selected is being edited in Bluefish."));
		g_free(tmpstr);
		if (retval == 0) {
			g_free(new_curi);
			return NULL;
		} else {
			document_unset_filename(exdoc);
		}
	} else {
		GFile *tmp;
		gboolean exists;
		tmp = g_file_new_for_uri(new_curi);
		exists = g_file_query_exists(tmp, NULL);
		g_object_unref(tmp);
		if (exists) {
			gchar *tmpstr;
			gint retval;
			const gchar *buttons[] = { _("_Cancel"), _("_Overwrite"), NULL };
			tmpstr = g_strdup_printf(_("A file named \"%s\" already exists."), new_curi);
			retval =
				message_dialog_new_multi(bfwin->main_window, GTK_MESSAGE_WARNING, buttons, tmpstr,
										 _("Do you want to replace the existing file?"));
			g_free(tmpstr);
			if (retval == 0) {
				g_free(new_curi);
				return NULL;
			}
		}
	}
	return new_curi;
}

static void
session_set_savedir(Tbfwin * bfwin, GFile *uri)
{
	if (uri) {
		GFile *parent = g_file_get_parent(uri);
		if (bfwin->session->savedir)
			g_free(bfwin->session->savedir);
		bfwin->session->savedir = g_file_get_uri(parent);
	}
}

void
doc_save_backend(Tdocument * doc, Tdocsave_mode savemode, gboolean close_doc,
				 gboolean close_window)
{
	gchar *tmp;
	Trefcpointer *buffer;
	Tdocsavebackend *dsb;
	GFile *dest_uri=NULL;
	GFileInfo *dest_finfo=NULL;
	gboolean firstsave;
	DEBUG_MSG
		("doc_save_backend, started for doc %p, mode=%d, close_doc=%d, close_window=%d\n", doc,
		 savemode, close_doc, close_window);

	if (doc->readonly) {
		g_print("Cannot save readonly document !?!?");
		return;
	}

	dsb = g_new0(Tdocsavebackend, 1);
	dsb->doc = doc;
	dsb->savemode = savemode;
	/* should be moved to a plugin interface, because this is HTML specific */
	/* update author meta tag */
	if (main_v->props.auto_update_meta_author) {
		const gchar *realname = g_get_real_name();
		if (realname && strlen(realname) > 0) {
			gchar *author_tmp;
			author_tmp = g_strconcat("<meta name=\"author\" content=\"", realname, "\" ", NULL);
			snr3_run_extern_replace(doc,
									"<meta[ \t\n]+name[ \t\n]*=[ \t\n]*\"author\"[ \t\n]+content[ \t\n]*=[ \t\n]*\"[^\"]*\"[ \t\n]*",
									snr3scope_doc, snr3type_pcre, FALSE, author_tmp, FALSE, FALSE);
			g_free(author_tmp);
		}
	}

	/* update date meta tag */
	if (main_v->props.auto_update_meta_date) {
		time_t time_var;
		struct tm *time_struct;
		gchar isotime[60];
		gchar *date_tmp;

		time_var = time(NULL);
		time_struct = localtime(&time_var);
#ifdef WIN32
		{
			glong hours, mins;
			gchar gmtsign;
			gchar tmptime[50];

			strftime(tmptime, 30, "%Y-%m-%dT%H:%M:%S", time_struct);
			gmtsign = _timezone > 0 ? '-' : '+';
			hours = abs(_timezone) / 3600;
			mins = (abs(_timezone) % 3600) / 60;
			sprintf(isotime, "%s%c%02ld%02ld", tmptime, gmtsign, hours, mins);
		}
#else							/* WIN32 */
		strftime(isotime, 30, "%Y-%m-%dT%H:%M:%S%z", time_struct);
#endif							/* WIN32 */
		DEBUG_MSG("doc_save_backend, ISO-8601 time %s\n", isotime);

		date_tmp = g_strconcat("<meta name=\"date\" content=\"", isotime, "\" ", NULL);
		snr3_run_extern_replace(doc,
								"<meta[ \t\n]+name[ \t\n]*=[ \t\n]*\"date\"[ \t\n]+content[ \t\n]*=[ \t\n]*\"[^\"]*\"[ \t\n]*",
								snr3scope_doc, snr3type_pcre, FALSE, date_tmp, FALSE, FALSE);
		g_free(date_tmp);
	}

	/* update generator meta tag */
	if (main_v->props.auto_update_meta_generator) {
		snr3_run_extern_replace(doc,
								"<meta[ \t\n]+name[ \t\n]*=[ \t\n]*\"generator\"[ \t\n]+content[ \t\n]*=[ \t\n]*\"[^\"]*\"[ \t\n]*",
								snr3scope_doc, snr3type_pcre, FALSE,
								"<meta name=\"generator\" content=\"Bluefish " VERSION "\" ", FALSE, FALSE);
	}
	if (main_v->props.strip_trailing_spaces_on_save) {
		strip_trailing_spaces(doc);
	}

	if (doc->save) {
		gchar *errmessage;
		/* this message is not in very nice english I'm afraid */
		errmessage =
			g_strconcat(_("File:\n\""), gtk_label_get_text(GTK_LABEL(doc->tab_label)),
						_("\" save is in progress"), NULL);
		message_dialog_new(BFWIN(doc->bfwin)->main_window, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						   _("Save in progress!"), errmessage);
		g_free(errmessage);
		g_free(dsb);
		DEBUG_MSG("doc_save_backend, already save in progress, return\n");
		return;
	}
	firstsave = (doc->uri == NULL && savemode == docsave_normal);
	if (firstsave || savemode != docsave_normal) {
		gchar *newfilename, *curi, *dialogtext;
		const gchar *gui_name = gtk_label_get_text(GTK_LABEL(doc->tab_label));
		DEBUG_MSG("doc_save_backend, no uri (doc->uri=%p), or saveas/copy/move (savemode=%d)\n", doc->uri, savemode);
		curi = doc->uri ? g_file_get_uri(doc->uri) : g_strdup(gui_name); /* SaveFile dialog shows weird behavior if NULL is passed as suggested filename */
		if (savemode == docsave_normal) {
			dialogtext = g_strdup(_("Save"));
		} else if (savemode == docsave_saveas){
			dialogtext = g_strdup_printf(_("Save %s as"), gui_name);
		} else if (savemode == docsave_move){
			dialogtext = g_strdup_printf(_("Move/rename %s to"), gui_name);
		} else {
			dialogtext = g_strdup_printf(_("Save copy of %s"), gui_name);
		}
		newfilename =
			ask_new_filename(BFWIN(doc->bfwin), curi, dialogtext);
		g_free(dialogtext);
		g_free(curi);
		if (!newfilename) {
			DEBUG_MSG("doc_save_backend, no newfilename, return\n");
			g_free(dsb);
			return;
		}
		dest_uri = g_file_new_for_uri(newfilename);
		if (doc->uri == NULL || savemode == docsave_saveas || savemode == docsave_move) {
			if (doc->uri) {
				if (savemode == docsave_move) {
					dsb->unlink_uri = doc->uri;	/* unlink and refresh this uri later */
					g_object_ref(dsb->unlink_uri);
				}
			}
			doc_set_uri(doc, dest_uri, FALSE);
			dest_finfo = doc->fileinfo;
		}
		DEBUG_MSG("doc_save_backend, newfilename=%s, dest_uri=%p, doc->uri=%p\n",newfilename, dest_uri, doc->uri);
		g_free(newfilename);
	} else {
		DEBUG_MSG("doc_save_backend, have uri and normal save\n");
		dest_uri = doc->uri;
		dest_finfo = doc->fileinfo;
		g_object_ref(dest_uri);
	}
	DEBUG_MSG("doc_save_backend, dest_uri=%p\n",dest_uri);
	if ((firstsave || savemode != docsave_normal)&& dest_uri) {
		dsb->fbrefresh_uri = dest_uri;	/* refresh this uri later */
		g_object_ref(dest_uri);
	}

	session_set_savedir(doc->bfwin, dest_uri);

	tmp = doc_get_buffer_in_encoding(doc);
	if (!tmp) {
		g_free(dsb);
		return;
	}
#if !GLIB_CHECK_VERSION(2, 18, 0)
	/* check runtime glib version, check if remote file, and give warning if remote file on glib < 2.18 */
	if (glib_major_version == 2 && glib_minor_version < 18 && !g_file_is_native(doc->uri)) {
		gchar *message =
			g_strdup_printf(_
							("Your glib version (%d.%d.%d) is unreliable with remote files. Please upgrade to 2.18.0 or newer."),
glib_major_version, glib_minor_version, glib_micro_version);
		bfwin_statusbar_message(BFWIN(doc->bfwin), message, 20);
		g_free(message);
	}
#endif
	buffer = refcpointer_new(tmp);
	doc->close_doc = close_doc;
	doc->close_window = close_window;
	gtk_text_view_set_editable(GTK_TEXT_VIEW(doc->view), FALSE);
	DEBUG_MSG("doc_save_backend, calling file_checkNsave_uri_async with uri %p for %zd bytes\n", dest_uri, strlen(buffer->data));
	doc->save =
		file_checkNsave_uri_async(dest_uri, dest_finfo, buffer, strlen(buffer->data), savemode == docsave_normal,
								  main_v->props.backup_file, doc_checkNsave_lcb, dsb);

	if (firstsave || savemode == docsave_saveas || savemode == docsave_move) {
		doc->readonly = FALSE;
		doc_reset_filetype(doc, doc->uri, buffer->data, strlen(buffer->data));
		doc_set_title(doc, NULL);
		doc_force_activate(doc);
	}
	g_object_unref(dest_uri);
	refcpointer_unref(buffer);
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
void
file_save_cb(GtkWidget * widget, Tbfwin * bfwin)
{
	if (bfwin->current_document)
		doc_save_backend(bfwin->current_document, docsave_normal, FALSE, FALSE);
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
void
file_save_as_cb(GtkWidget * widget, Tbfwin * bfwin)
{
	if (bfwin->current_document)
		doc_save_backend(bfwin->current_document, docsave_saveas, FALSE, FALSE);
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
void
file_move_to_cb(GtkWidget * widget, Tbfwin * bfwin)
{
	if (bfwin->current_document)
		doc_save_backend(bfwin->current_document, docsave_move, FALSE, FALSE);
}

void
file_save_all(Tbfwin * bfwin)
{
	GList *tmplist;
	Tdocument *tmpdoc;

	tmplist = g_list_first(bfwin->documentlist);
	while (tmplist) {
		tmpdoc = (Tdocument *) tmplist->data;
		if (tmpdoc->modified) {
			doc_save_backend(tmpdoc, docsave_normal, FALSE, FALSE);
		}
		tmplist = g_list_next(tmplist);
	}
}

/**
 * file_save_all_cb:
 * @widget: unused #GtkWidget
 * @bfwin: the #Tbfwin* window pointer
 *
 *  Save all editor notebooks
 *
 * Return value: void
 **/
void
file_save_all_cb(GtkWidget * widget, Tbfwin * bfwin)
{
	GList *tmplist;
	Tdocument *tmpdoc;

	tmplist = g_list_first(bfwin->documentlist);
	while (tmplist) {
		tmpdoc = (Tdocument *) tmplist->data;
		if (tmpdoc->modified) {
			doc_save_backend(tmpdoc, docsave_normal, FALSE, FALSE);
		}
		tmplist = g_list_next(tmplist);
	}
}

gint
doc_modified_dialog(Tdocument * doc)
{
	const gchar *buttons[] = { _("Close _Without Saving"), GTK_STOCK_CANCEL, GTK_STOCK_SAVE, NULL };
	gchar *text;
	gint retval;
	text = g_strdup_printf(_("Save changes to \"%s\" before closing?"),
						   gtk_label_get_text(GTK_LABEL(doc->tab_label)));
	retval = message_dialog_new_multi(BFWIN(doc->bfwin)->main_window, GTK_MESSAGE_QUESTION, buttons, text,
									  _("If you don't save your changes they will be lost."));
	g_free(text);
	return retval;
}

gint
project_not_found_dialog(Tbfwin * bfwin, GFile * uri)
{
	gchar *tmpstr;
	gint retval;
	const gchar *buttons[] = { GTK_STOCK_CANCEL, _("_Reopen"), NULL };
	gchar *path = g_file_get_parse_name(uri);
	tmpstr = g_strdup_printf(_("Project \"%s\" failed to load"), path);
	retval =	message_dialog_new_multi(bfwin->main_window, GTK_MESSAGE_WARNING, buttons, tmpstr,
										 _("Do you want to reopen project from another location?"));
	g_free(tmpstr);
	g_free(path);
	return retval;
}

Tclose_mode
multiple_files_modified_dialog(Tbfwin * bfwin)
{
	const gchar *buttons[] = { _("Choose per _File"), _("Close _All"), _("_Cancel"), _("_Save All"), NULL };
	int retval = message_dialog_new_multi(bfwin->main_window,
										  GTK_MESSAGE_QUESTION, buttons,
										  _("One or more open files have been changed."),
										  _("If you don't save your changes they will be lost."));
	return (Tclose_mode) retval;
}

/* return TRUE if all are either closed or saved
return FALSE on cancel*/
gboolean
choose_per_file(Tbfwin * bfwin, gboolean close_window)
{
	GList *duplist, *tmplist;
	duplist = g_list_copy(bfwin->documentlist);
	tmplist = g_list_first(duplist);
	while (tmplist) {
		gint retval;
		Tdocument *tmpdoc = (Tdocument *) tmplist->data;
		DEBUG_MSG("choose_per_file, tmpdoc=%p\n", tmpdoc);
		if (tmpdoc->modified) {
			retval = doc_modified_dialog(tmpdoc);
			switch (retval) {
			case 0:			/* close */
				DEBUG_MSG("choose_per_file, call doc_close\n");
				tmpdoc->modified = FALSE;
				doc_close_single_backend(tmpdoc, TRUE, close_window);
				break;
			case 1:			/* cancel */
				return FALSE;
				break;
			case 2:			/* save */
				DEBUG_MSG("choose_per_file, call doc_save\n");
				doc_save_backend(tmpdoc, docsave_normal, TRUE, close_window);
				break;
			}
		} else {
			doc_close_single_backend(tmpdoc, TRUE, close_window);
		}
		tmplist = g_list_next(tmplist);
	}
	g_list_free(duplist);
	return TRUE;
}

gboolean
doc_close_single_backend(Tdocument * doc, gboolean delay_activate, gboolean close_window)
{
	Tbfwin *bfwin = doc->bfwin;
	if (doc->checkmodified)
		checkmodified_cancel(doc->checkmodified);
	if (doc->autosave_progress || doc->autosaved || doc->need_autosave)
		remove_autosave(doc);
	if (doc->load != NULL || doc->info != NULL) {
		/* we should cancel the action now..., and then let the callbacks close it...
		   the order is important, because the info callback will not close the document,
		   only the load callback will call doc_close_single_backend */
		doc->close_doc = TRUE;
		doc->close_window = close_window;
		if (doc->info)
			file_asyncfileinfo_cancel(doc->info);
		if (doc->load)
			file2doc_cancel(doc->load);
		/* we will not cancel save operations, because it might corrupt the file, let
		   them just timeout */
		DEBUG_MSG("doc_close_single_backend, cancelled load/info and set close_doc to TRUE, returning now\n");
		return FALSE;
	}
	if (doc->autosaved || doc->autosave_progress || doc->need_autosave) {
		remove_autosave(doc);
	}
	if (doc_is_empty_non_modified_and_nameless(doc)
		&& g_list_length(BFWIN(doc->bfwin)->documentlist) <= 1) {
		if (close_window) {
			bfwin_destroy_and_cleanup(BFWIN(doc->bfwin));
		}
		return TRUE;
	}
	if (doc->modified) {
		gint retval = doc_modified_dialog(doc);
		switch (retval) {
		case 0:
			doc_destroy(doc, close_window || delay_activate);
			break;
		case 1:
			return FALSE;
			break;
		case 2:
			doc_save_backend(doc, docsave_normal, TRUE, close_window);
			break;
		}
	} else {
		doc_destroy(doc, close_window || delay_activate);
	}
	if (close_window && bfwin->documentlist == NULL) {	/* the documentlist is empty */
		bfwin_destroy_and_cleanup(bfwin);
	}
	DEBUG_MSG("doc_close_single_backend, finished!\n");
	return TRUE;
}

void
doc_save_all_close(Tbfwin * bfwin)
{
	GList *tmplist, *duplist;
	Tdocument *tmpdoc;
	duplist = g_list_copy(bfwin->documentlist); /* Copy ducumentlist first, since using just tmplist causes unexpected behavior as docs are destroyed */
	tmplist = g_list_first(duplist);
	while (tmplist) {
		Tdocument *tmpdoc = (Tdocument *) tmplist->data;
#ifdef MAC_INTEGRATION
		if (tmpdoc->uri == NULL && main_v->osx_status == 1 ) { /* if osx app is terminating we do not save untitled files that does not have uri */
		DEBUG_MSG("bfwin_osx_terminate_event, closing untitled document\n");
		tmpdoc->modified = FALSE;
		doc_close_single_backend(tmpdoc, TRUE, TRUE); }
		else {
		DEBUG_MSG("bfwin_osx_terminate_event, saving document\n");
		doc_save_backend(tmpdoc, docsave_normal, TRUE, TRUE);	
		}
#else
		doc_save_backend(tmpdoc, docsave_normal, TRUE, TRUE);
#endif
		tmplist = g_list_next(tmplist);
	}
	g_list_free(duplist);
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
void
file_close_cb(GtkWidget * widget, Tbfwin * bfwin)
{
	if (bfwin->current_document)
		doc_close_single_backend(bfwin->current_document, FALSE, FALSE);
}

void
doc_close_multiple_backend(Tbfwin * bfwin, gboolean close_window, Tclose_mode close_mode)
{
	GList *tmplist, *duplist;
	Tdocument *tmpdoc;

/*	if (g_list_length(bfwin->documentlist) == 1) {
		return doc_close_single_backend(bfwin->current_document, FALSE, close_window);
	}
	if (have_modified_documents(bfwin->documentlist)) {
		retval = multiple_files_modified_dialog(bfwin);
		if (retval == 2) {
			return FALSE;
		}
	}*/

	/* we duplicate the documentlist so we can safely walk trough the list, in
	   our duplicate list there is no chance that the list is changed during the time
	   we walk the list */
	duplist = g_list_copy(bfwin->documentlist);
	tmplist = g_list_first(duplist);
	while (tmplist) {
		tmpdoc = (Tdocument *) tmplist->data;
		if (close_mode == close_mode_close_all) {
			/* fake that this document was not modified */
			tmpdoc->modified = FALSE;
			doc_close_single_backend(tmpdoc, TRUE, close_window);
		} else if (close_mode == close_mode_save_all) {
			doc_save_backend(tmpdoc, docsave_normal, TRUE, close_window);
		}
		tmplist = g_list_next(tmplist);
	}
	g_list_free(duplist);
	DEBUG_MSG("doc_close_multiple_backend, finished\n");
	if (!close_window)
		bfwin_notebook_changed(bfwin, -1);
}

void
file_close_all(Tbfwin * bfwin)
{
	if (have_modified_documents(bfwin->documentlist)) {
		Tclose_mode retval = multiple_files_modified_dialog(bfwin);
		switch (retval) {
		case close_mode_cancel:
			return;
			break;
		case close_mode_per_file:
			choose_per_file(bfwin, FALSE);
			break;
		case close_mode_save_all:
		case close_mode_close_all:
			doc_close_multiple_backend(bfwin, FALSE, retval);
			break;
		}
	} else {
		doc_close_multiple_backend(bfwin, FALSE, close_mode_close_all);
	}
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
void
file_close_all_cb(GtkWidget * widget, Tbfwin * bfwin)
{
	if (have_modified_documents(bfwin->documentlist)) {
		Tclose_mode retval = multiple_files_modified_dialog(bfwin);
		switch (retval) {
		case close_mode_cancel:
			return;
			break;
		case close_mode_per_file:
			choose_per_file(bfwin, FALSE);
			break;
		case close_mode_save_all:
		case close_mode_close_all:
			doc_close_multiple_backend(bfwin, FALSE, retval);
			break;
		}
	} else {
		doc_close_multiple_backend(bfwin, FALSE, close_mode_close_all);
	}
}

void
file_new_doc(Tbfwin * bfwin)
{
	Tdocument *doc;
	GFile *template = NULL;

	if (bfwin->session->template && bfwin->session->template[0]) {
		template = g_file_new_for_commandline_arg(bfwin->session->template);
	}
	doc = doc_new_with_template(bfwin, template, TRUE);
	bfwin_switch_to_document_by_pointer(bfwin, doc);
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
void
file_new_cb(GtkWidget * widget, Tbfwin * bfwin)
{
	Tdocument *doc;
	GFile *template = NULL;
	if (bfwin->session->template && bfwin->session->template[0]) {
		template = g_file_new_for_commandline_arg(bfwin->session->template);
	}
	doc = doc_new_with_template(bfwin, template, TRUE);
	bfwin_switch_to_document_by_pointer(bfwin, doc);
}

static void
file_reload_all_modified_check_lcb(Tcheckmodified_status status, GError * gerror,
								   GFileInfo * orig, GFileInfo * new, gpointer user_data)
{
	if (status == CHECKMODIFIED_MODIFIED) {
		DEBUG_MSG("file_reload_all_modified_check_lcb, reload %p\n", user_data);
		doc_reload(DOCUMENT(user_data), new, FALSE);
	}
}

void
file_reload_all_modified(Tbfwin * bfwin)
{
	GList *tmplist = g_list_first(bfwin->documentlist);
	while (tmplist) {
		if (DOCUMENT(tmplist->data)->uri && DOCUMENT(tmplist->data)->status == DOC_STATUS_COMPLETE) {
			DEBUG_MSG("file_reload_all_modified, check %p\n", tmplist->data);
			file_checkmodified_uri_async(DOCUMENT(tmplist->data)->uri, DOCUMENT(tmplist->data)->fileinfo,
										 file_reload_all_modified_check_lcb, tmplist->data);
		}
		tmplist = g_list_next(tmplist);
	}
}

typedef struct {
	GtkWidget *dialog;
	Tbfwin *bfwin;
	GtkWidget *entry_local;
	GtkWidget *entry_remote;
	GtkWidget *delete_deprecated;
	GtkWidget *include_hidden;
	GtkWidget *progress;
	GtkWidget *messagelabel;
	gulong signal_id;
} Tsyncdialog;

static void
sync_progress(GFile *uri, gint total, gint done, gint failed, gpointer user_data)
{
	Tsyncdialog *sd = user_data;
	if (total > 0) {
		gchar *text;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(sd->progress), 1.0 * done / total);
		if (uri) {
			gchar * curi = g_file_get_uri(uri);
			text = g_strdup_printf("%s (%d / %d)", curi, done, total);
			g_free(curi);
		} else {
			text = g_strdup_printf("%d / %d", done, total);
		}
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(sd->progress), text);
/*		g_print("%s\n",text);*/
		g_free(text);
		if (failed > 0) {
			text =
				g_strdup_printf(ngettext
								("<span color=\"red\">%d failure</span>",
								 "<span color=\"red\">%d failures</span>", failed), failed);
			gtk_label_set_markup(GTK_LABEL(sd->messagelabel), text);
			gtk_widget_show(sd->messagelabel);
			g_free(text);
		}
	} else if (total == -1) {
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(sd->progress), 1);
		if (failed > 0) {
			gtk_progress_bar_set_text(GTK_PROGRESS_BAR(sd->progress), _("incomplete finished"));
		} else {
			gtk_progress_bar_set_text(GTK_PROGRESS_BAR(sd->progress), _("completed"));
		}
		g_signal_handler_unblock(sd->dialog, sd->signal_id);
	}
}

static void
sync_dialog_response_lcb(GtkDialog * dialog, gint response_id, gpointer user_data)
{
	Tsyncdialog *sd = user_data;
	DEBUG_MSG("sync_dialog_response_lcb, response=%d\n", response_id);
	if (response_id > 0) {
		GFile *local, *remote;
		gtk_label_set_text(GTK_LABEL(sd->messagelabel), "");
		gtk_widget_hide(sd->messagelabel);
		local = g_file_new_for_commandline_arg(gtk_entry_get_text(GTK_ENTRY(sd->entry_local)));
		remote = g_file_new_for_commandline_arg(gtk_entry_get_text(GTK_ENTRY(sd->entry_remote)));
		if (response_id == 1) {
			sync_directory(local, remote,
						   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sd->delete_deprecated)),
						   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sd->include_hidden)), sync_progress,
						   sd);
		} else if (response_id == 2) {
			sync_directory(remote, local,
						   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sd->delete_deprecated)),
						   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sd->include_hidden)), sync_progress,
						   sd);
		}
		sd->bfwin->session->sync_delete_deprecated =
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sd->delete_deprecated));
		sd->bfwin->session->sync_include_hidden =
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sd->include_hidden));

		g_signal_handler_block(sd->dialog, sd->signal_id);
		g_free(sd->bfwin->session->sync_local_uri);
		sd->bfwin->session->sync_local_uri = g_file_get_uri(local);
		g_free(sd->bfwin->session->sync_remote_uri);
		sd->bfwin->session->sync_remote_uri = g_file_get_uri(remote);

		g_object_unref(local);
		g_object_unref(remote);
	} else {
		gtk_widget_destroy(sd->dialog);
		g_slice_free(Tsyncdialog, sd);
	}
}

void
sync_dialog(Tbfwin * bfwin)
{
	Tsyncdialog *sd;
	GtkWidget *carea, *table;

	sd = g_slice_new0(Tsyncdialog);
	sd->bfwin = bfwin;
	sd->dialog = gtk_dialog_new_with_buttons(_("Upload / Download"),
											 GTK_WINDOW(bfwin->main_window),
											 GTK_DIALOG_DESTROY_WITH_PARENT,
											 _("Upload"), 1, _("Download"), 2,
											 GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);

	carea = gtk_dialog_get_content_area(GTK_DIALOG(sd->dialog));

#if !GLIB_CHECK_VERSION(2, 18, 0)
	if (glib_major_version == 2 && glib_minor_version < 18) {
		gchar *message;
		GtkWidget *label = gtk_label_new(NULL);
		message =
			g_strdup_printf(_
							("<b>Your glib version (%d.%d.%d) works unreliable with remote files (smb, ftp, sftp, webdav etc.). Please upgrade to a glib version newer than 2.18.0 if you rely on remote file support.</b>"),
							glib_major_version, glib_minor_version, glib_micro_version);
		gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
		gtk_label_set_markup(GTK_LABEL(label), message);
		g_free(message);
		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(sd->dialog))), label, FALSE, FALSE,
						   4);
	}
#endif
	table = dialog_table_in_vbox(4, 3, 6, carea, TRUE,TRUE, 3);

	sd->entry_local = dialog_entry_in_table(NULL, table, 1, 2,0, 1);
	dialog_mnemonic_label_in_table(_("Local directory"), sd->entry_local, table,
									0, 1, 0,1);
	gtk_table_attach(GTK_TABLE(table), file_but_new2(sd->entry_local, 1, bfwin, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER),
									2, 3, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

	sd->entry_remote = dialog_entry_in_table(NULL, table, 1, 2,1, 2);
	dialog_mnemonic_label_in_table(_("Remote directory"), sd->entry_remote, table,
									0, 1, 1,2);
	gtk_table_attach(GTK_TABLE(table), file_but_new2(sd->entry_remote, 1, bfwin, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER),
									2, 3, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

	sd->delete_deprecated = dialog_check_button_in_table(_("Delete deprecated files"),
						bfwin->session->sync_delete_deprecated, table,
						0, 3, 2, 3);

	sd->include_hidden = dialog_check_button_in_table(_("Include hidden files"),
						bfwin->session->sync_include_hidden, table,
						0, 3, 3, 4);

	sd->messagelabel = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(carea), sd->messagelabel, FALSE, FALSE, 4);

	sd->progress = gtk_progress_bar_new();
	gtk_progress_bar_set_ellipsize(GTK_PROGRESS_BAR(sd->progress), PANGO_ELLIPSIZE_MIDDLE);
	gtk_box_pack_start(GTK_BOX(carea), sd->progress, FALSE, FALSE, 4);

	if (bfwin->session->sync_local_uri && bfwin->session->sync_local_uri[0] != '\0') {
		gtk_entry_set_text(GTK_ENTRY(sd->entry_local), bfwin->session->sync_local_uri);
	} else if (bfwin->session->recent_dirs && bfwin->session->recent_dirs->data
			   && *(gchar *) bfwin->session->recent_dirs->data != '\0') {
		gtk_entry_set_text(GTK_ENTRY(sd->entry_local), bfwin->session->recent_dirs->data);
	}

	if (bfwin->session->sync_remote_uri && bfwin->session->sync_remote_uri[0] != '\0') {
		gtk_entry_set_text(GTK_ENTRY(sd->entry_remote), bfwin->session->sync_remote_uri);
	}

	sd->signal_id = g_signal_connect(sd->dialog, "response", G_CALLBACK(sync_dialog_response_lcb), sd);
	gtk_widget_show_all(sd->dialog);
	gtk_widget_hide(sd->messagelabel);
}

static gchar *
modified_on_disk_warning_string(const gchar * filename, GFileInfo * oldfinfo, GFileInfo * newfinfo)
{
	gchar *tmpstr, *oldtimestr, *newtimestr;
	time_t newtime, oldtime;
	goffset oldsize, newsize;
	gchar *strnewsize, *stroldsize;

	newtime = (time_t) g_file_info_get_attribute_uint64(newfinfo, G_FILE_ATTRIBUTE_TIME_MODIFIED);
	oldtime = (time_t) g_file_info_get_attribute_uint64(oldfinfo, G_FILE_ATTRIBUTE_TIME_MODIFIED);
	newtimestr = bf_portable_time(&newtime);
	oldtimestr = bf_portable_time(&oldtime);
	newsize = g_file_info_get_size(newfinfo);
	oldsize = g_file_info_get_size(oldfinfo);
	strnewsize = g_strdup_printf("%"G_GOFFSET_FORMAT"", newsize);
	stroldsize = g_strdup_printf("%"G_GOFFSET_FORMAT"", oldsize);
	/*g_print("oldtimestr=%s, newtimestr=%s\n",oldtimestr,newtimestr); */
	tmpstr = g_strdup_printf(_("Filename:%s changed on disk.\n\n"
							   "Original modification time was %s\n"
							   "New modification time is %s\n"
							   "Original size was %s bytes\n"
							   "New size is %s bytes"), filename, oldtimestr, newtimestr, stroldsize, strnewsize);
	g_free(newtimestr);
	g_free(oldtimestr);
	g_free(strnewsize);
	g_free(stroldsize);
	return tmpstr;
}

static void
doc_activate_modified_lcb(Tcheckmodified_status status, GError * gerror, GFileInfo * orig, GFileInfo * new,
						  gpointer callback_data)
{
	Tdocument *doc = callback_data;
	switch (status) {
	case CHECKMODIFIED_ERROR:
		DEBUG_MSG("doc_activate_modified_lcb, CHECKMODIFIED_ERROR ??\n");
		if (gerror->code == G_IO_ERROR_NOT_FOUND) {
			gchar *tmpstr;
			gint retval;
			const gchar *buttons[] = { _("_Unset file name"), _("_Save"), NULL };
			/* file is deleted on disk, what do we do now ? */
			tmpstr = g_strdup_printf(_("File name: %s"), gtk_label_get_text(GTK_LABEL(doc->tab_menu)));
			retval = message_dialog_new_multi(BFWIN(doc->bfwin)->main_window,
											  GTK_MESSAGE_WARNING,
											  buttons, _("File disappeared from disk\n"), tmpstr);
			g_free(tmpstr);
			if (retval == 1) {	/* save */
				doc_save_backend(doc, docsave_normal, FALSE, FALSE);
			} else {			/* unset */
				document_unset_filename(doc);
			}
		} else {
			/* TODO: warn the user */
		}
		break;
	case CHECKMODIFIED_CANCELLED:
		DEBUG_MSG("doc_activate_modified_lcb, CHECKMODIFIED_CANCELLED\n");
		break;
	case CHECKMODIFIED_MODIFIED:
		{
			gchar *tmpstr /*, *oldtimestr, *newtimestr */ ;
			gint retval;
			const gchar *buttons[] =
				{ _("_Ignore"), _("_Reload"), _("Check and reload all documents"), NULL };
			/*time_t newtime,origtime;

			   newtime = (time_t)g_file_info_get_attribute_uint64(new,G_FILE_ATTRIBUTE_TIME_MODIFIED);
			   origtime = (time_t)g_file_info_get_attribute_uint64(orig,G_FILE_ATTRIBUTE_TIME_MODIFIED);
			   g_print("doc_activate_modified_lcb, newtime=%ld,%d origtime=%ld,%d newsize=%"G_GOFFSET_FORMAT" origsize=%"G_GOFFSET_FORMAT"\n",
			   newtime,g_file_info_get_attribute_uint32(new,G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC),
			   origtime,g_file_info_get_attribute_uint32(orig,G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC),
			   g_file_info_get_size(new),g_file_info_get_size(orig));
			   newtimestr = bf_portable_time(&newtime);
			   oldtimestr = bf_portable_time(&origtime);

			   tmpstr = g_strdup_printf(_("Filename: %s\n\nNew modification time is: %s\nOld modification time is: %s"), gtk_label_get_text(GTK_LABEL(doc->tab_menu)), newtimestr, oldtimestr);
			 */
			tmpstr = modified_on_disk_warning_string(gtk_label_get_text(GTK_LABEL(doc->tab_menu)), orig, new);
			retval = message_dialog_new_multi(BFWIN(doc->bfwin)->main_window,
											  GTK_MESSAGE_WARNING,
											  buttons, _("File changed on disk\n"), tmpstr);
			g_free(tmpstr);
			/*g_free(newtimestr);
			   g_free(oldtimestr); */
			if (retval == 0) {	/* ignore */
				/*if (doc->fileinfo) {
				   g_object_unref(doc->fileinfo);
				   }
				   doc->fileinfo = new;
				   g_object_ref(doc->fileinfo); */
				GTimeVal mtime;
				g_file_info_set_size(doc->fileinfo, g_file_info_get_size(new));
				g_file_info_get_modification_time(new, &mtime);
				g_file_info_set_modification_time(doc->fileinfo, &mtime);
				g_file_info_set_attribute_string(doc->fileinfo, "etag::value",
												 g_file_info_get_attribute_string(new, "etag::value"));
				doc_set_tooltip(doc);
			} else if (retval == 1) {	/* reload */
				doc_reload(doc, new, FALSE);
			} else {			/* reload all modified documents */
				file_reload_all_modified(doc->bfwin);
			}
		}
		break;
	case CHECKMODIFIED_OK:
		/* do nothing */
		break;
	}
	doc->checkmodified = NULL;
}

void
doc_start_modified_check(Tdocument * doc)
{
	if (doc->uri && doc->fileinfo && !doc->checkmodified && !doc->save) {	/* don't check during another check, or during save */
		doc->checkmodified =
			file_checkmodified_uri_async(doc->uri, doc->fileinfo, doc_activate_modified_lcb, doc);
	}
}

static gboolean
modified_on_disk_check_lcb(gpointer data)
{
	GList *tmplist = g_list_first(main_v->bfwinlist);
	while (tmplist) {
		Tbfwin *bfwin = tmplist->data;
		if (bfwin->current_document) {
			doc_start_modified_check(bfwin->current_document);
		}
		tmplist = g_list_next(tmplist);
	}
	return TRUE;
}

void
modified_on_disk_check_init(void)
{
	if (main_v->props.do_periodic_check && !main_v->periodic_check_id)
		main_v->periodic_check_id =
			g_timeout_add_seconds_full(G_PRIORITY_LOW, 15, modified_on_disk_check_lcb, NULL, NULL);
	else if (!main_v->props.do_periodic_check && main_v->periodic_check_id) {
		g_source_remove(main_v->periodic_check_id);
		main_v->periodic_check_id = 0;
	}
}
