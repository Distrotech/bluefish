/* Bluefish HTML Editor
 * file_dialogs.c - file dialogs
 *
 * Copyright (C) 2005,2006,2007,2008 Olivier Sessink
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

/* #define DEBUG */

#include <gtk/gtk.h>
#include <string.h> /* memcpy */
#include <time.h> /* strftime() */
#include "bluefish.h"
#include "dialog_utils.h"
#include "document.h"
#include "file.h"
#include "filebrowser2.h"
#include "gtk_easy.h"
#include "gui.h"
#include "snr2.h" /* snr2_run_extern_replace() */
#include "stringlist.h"
#include "undo_redo.h"

/**************************************************************************/
/* the start of the callback functions for the menu, acting on a document */
/**************************************************************************/
typedef struct {
  GtkWidget *dialog;
  GtkWidget *basedir;
  GtkWidget *find_pattern;
  GtkWidget *matchname;
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
  GFile *baseuri;
  gchar *basedir, *content_filter, *extension_filter;

  extension_filter = gtk_editable_get_chars (GTK_EDITABLE (GTK_BIN (tfs->find_pattern)->child), 0, -1);
  basedir = gtk_editable_get_chars (GTK_EDITABLE (tfs->basedir), 0, -1);
  baseuri = g_file_new_for_uri(basedir);
  content_filter = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(tfs->grep_pattern)->entry), 0, -1);
  tfs->bfwin->session->searchlist = add_to_history_stringlist(tfs->bfwin->session->searchlist,content_filter,TRUE,TRUE);

  open_advanced(tfs->bfwin, baseuri, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tfs->recursive))
      , gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tfs->matchname))
      , strlen(extension_filter) ==0 ? NULL : extension_filter
      , strlen(content_filter) ==0 ? NULL : content_filter
      , gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tfs->is_regex)));

  g_free (basedir);
  g_free (content_filter);
  g_free (extension_filter);
  g_object_unref (baseuri);
  
  tfs->bfwin->session->adv_open_recursive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(tfs->recursive));
  tfs->bfwin->session->adv_open_matchname = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(tfs->matchname));
}

static void files_advanced_win_select_basedir_lcb(GtkWidget * widget, Tfiles_advanced *tfs) {
  gchar *newdir = NULL;
  GtkWidget *dialog;

  dialog = file_chooser_dialog(tfs->bfwin, _("Select basedir"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, (gchar *)gtk_entry_get_text(GTK_ENTRY(tfs->basedir)), TRUE, FALSE, NULL, FALSE);
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    newdir = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
  }
  gtk_widget_destroy (dialog);

  if (newdir) {
    gtk_entry_set_text (GTK_ENTRY (tfs->basedir), newdir);
    g_free (newdir);
  }
}

void files_advanced_win(Tbfwin *bfwin, gchar *basedir) {
  GtkWidget *alignment, *button, *table, *vbox, *vbox2;
  GtkListStore *lstore;
  GtkTreeIter iter;
  Tfiles_advanced *tfs;
  unsigned int i = 0;
  
  const gchar *fileExts[] = {
    "*.c",
    "*.cgi",
    "*.cpp",
    "*.css",
    "*.h",
    "*.html",
    "*.htm",
    "*.java",
    "*.js",
    "*.php",
    "*.pl",
    "*.py",
    "*.shtml",
    "*.txt",
    "*.xml"
  };
  
  tfs = g_new (Tfiles_advanced, 1);
  tfs->bfwin = bfwin;

  tfs->dialog = gtk_dialog_new_with_buttons (_("Advanced open file selector"),
                                GTK_WINDOW (tfs->bfwin->main_window),
                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                NULL);  

  gtk_dialog_set_has_separator (GTK_DIALOG (tfs->dialog), FALSE);

	alignment = gtk_alignment_new (0, 0, 1, 1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 12, 0, 12, 6);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (tfs->dialog)->vbox), alignment, FALSE, FALSE, 0);
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (alignment), vbox);

	vbox2 = dialog_vbox_labeled (_("<b>General</b>"), vbox);

  table = dialog_table_in_vbox(2, 6, 0, vbox2, FALSE, FALSE, 6);

  if (!basedir) {
    tfs->basedir = entry_with_text(bfwin->session->opendir, 255);
  } else {
    tfs->basedir = entry_with_text(basedir, 255);
  }
  dialog_mnemonic_label_in_table(_("Base _Dir:"), tfs->basedir, table, 0, 1, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(table), tfs->basedir, 1, 5, 0, 1);
	button = dialog_button_new_with_image_in_table (NULL,
                                                  -1,
                                                  GTK_STOCK_OPEN,
                                                  GTK_ICON_SIZE_MENU,
                                                  table,
                                                  5, 6,
                                                  0, 1);

	g_signal_connect (button, "clicked", G_CALLBACK (files_advanced_win_select_basedir_lcb), tfs);

  lstore = gtk_list_store_new (1, G_TYPE_STRING);
  for (i = 0; i < G_N_ELEMENTS (fileExts); i++) {
    gtk_list_store_append (GTK_LIST_STORE (lstore), &iter);
    gtk_list_store_set (GTK_LIST_STORE (lstore), &iter, 0, fileExts[i], -1);
  };
  tfs->find_pattern = gtk_combo_box_entry_new_with_model (GTK_TREE_MODEL (lstore), 0);
  g_object_unref (lstore);
  dialog_mnemonic_label_in_table(_("_Pattern:"), tfs->find_pattern, table, 0, 1, 1, 2);
  gtk_table_attach_defaults(GTK_TABLE(table), tfs->find_pattern, 1, 5, 1, 2);
  g_signal_connect (G_OBJECT (tfs->find_pattern), "changed", G_CALLBACK (files_advanced_win_findpattern_changed), tfs);

	table = dialog_table_in_vbox(2, 2, 0, vbox2, FALSE, FALSE, 0);

  tfs->matchname = checkbut_with_value(NULL, tfs->bfwin ? tfs->bfwin->session->adv_open_matchname : TRUE);
  dialog_mnemonic_label_in_table(_("_Match on file name only:"), tfs->matchname, table, 0, 1, 0, 1);
  gtk_table_attach(GTK_TABLE(table), tfs->matchname, 1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);

  tfs->recursive = checkbut_with_value(NULL, tfs->bfwin ? tfs->bfwin->session->adv_open_recursive : TRUE);
  dialog_mnemonic_label_in_table(_("_Recursive:"), tfs->recursive, table, 0, 1, 1, 2);
  gtk_table_attach(GTK_TABLE(table), tfs->recursive, 1, 2, 1, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);

	alignment = gtk_alignment_new (0, 0, 1, 1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 12, 18, 12, 6);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (tfs->dialog)->vbox), alignment, FALSE, FALSE, 0);
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (alignment), vbox);
  
  vbox2 = dialog_vbox_labeled (_("<b>Contains</b>"), vbox);

  table = dialog_table_in_vbox(2, 4, 0, vbox2, FALSE, FALSE, 6);
 
  /* TODO: This needs to be converted to use GtkComboBoxEntry */
  tfs->grep_pattern = combo_with_popdown("", bfwin->session->searchlist, TRUE);
  dialog_mnemonic_label_in_table(_("Pa_ttern:"), (GTK_COMBO (tfs->grep_pattern)->entry), table, 0, 1, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(table), tfs->grep_pattern, 1, 4, 0, 1);
  
  tfs->is_regex = checkbut_with_value(NULL, 0);
  dialog_mnemonic_label_in_table(_("Is rege_x:"), tfs->is_regex, table, 0, 1, 1, 2);
  gtk_table_attach(GTK_TABLE(table), tfs->is_regex, 1, 2, 1, 2, GTK_FILL, GTK_SHRINK, 0, 0);

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

static void file_open_ok_lcb(GtkDialog *dialog,gint response,Tbfwin *bfwin) {
  if (response == GTK_RESPONSE_ACCEPT) {
    GSList *slist;
    GtkComboBox *combo;
    bfwin->focus_next_new_doc = TRUE;
    slist = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));

    combo = g_object_get_data(G_OBJECT(dialog),"encodings");
    if (combo) {
      GtkTreeIter iter;
      if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo),&iter)) {
        GtkTreeModel *model;
        gchar **arr;
        model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
        gtk_tree_model_get(GTK_TREE_MODEL(model),&iter,1,&arr,-1);
        if (bfwin->session->encoding) g_free(bfwin->session->encoding);
        if (arr) {
          bfwin->session->encoding = arr[1];
        } else {
          bfwin->session->encoding = NULL;
        }
        DEBUG_MSG("file_open_ok_lcb, session encoding is set to %s\n",bfwin->session->encoding);
      }
    }
    docs_new_from_uris(bfwin, slist, FALSE);
    g_slist_foreach(slist, (GFunc) g_free, NULL);
    g_slist_free(slist);
  }
  gtk_widget_destroy(GTK_WIDGET(dialog));
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
void file_open_cb(GtkWidget * widget, Tbfwin *bfwin) {
  GtkWidget *dialog;
  dialog = file_chooser_dialog(bfwin, _("Select files"), GTK_FILE_CHOOSER_ACTION_OPEN, NULL, FALSE, TRUE, NULL, TRUE);
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
static void open_url_destroy_lcb(GtkWidget *widget, Tou *ou) {
  g_free(ou);
}
static void open_url_cancel_lcb(GtkWidget *widget, Tou *ou) {
  gtk_widget_destroy(ou->win);
}
static void open_url_ok_lcb(GtkWidget *widget, Tou *ou) {
  gchar *url = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(ou->entry)->entry),0,-1);
  DEBUG_MSG("open_url_ok_lcb, url=%s\n",url);
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
/*  ou->entry = boxed_entry_with_text("", 255, vbox); */
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
/***********************************/
/*        async save code          */

typedef struct {
  Tdocument *doc;
  GFile *unlink_uri;
  GFile *fbrefresh_uri;
} Tdocsavebackend;

static void docsavebackend_cleanup(Tdocsavebackend *dsb) {
  if (dsb->unlink_uri) g_object_unref(dsb->unlink_uri);
  if (dsb->fbrefresh_uri) g_object_unref(dsb->fbrefresh_uri);
  g_free(dsb);
}

static void docsavebackend_async_unlink_lcb(gpointer data) {
  Tdocsavebackend *dsb = data;
  fb2_refresh_parent_of_uri(dsb->unlink_uri);
  docsavebackend_cleanup(dsb);
}

static TcheckNsave_return doc_checkNsave_lcb(TcheckNsave_status status,gint error_info,gpointer data) {
  Tdocsavebackend *dsb = data;
  Tdocument *doc = dsb->doc;
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
        const gchar *buttons[] = {_("_Abort save"), _("_Continue save"), NULL};
        gint retval;
        gchar *tmpstr = g_strdup_printf(_("A backupfile for %s could not be created. If you continue, this file will be overwritten."), gtk_label_get_text(GTK_LABEL(doc->tab_label)));
        retval = message_dialog_new_multi(BFWIN(doc->bfwin)->main_window,
                               GTK_MESSAGE_WARNING,
                               buttons,
                               _("File backup failure"),
                               tmpstr);
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
        errmessage = g_strconcat(_("Could not save file:\n\""),gtk_label_get_text(GTK_LABEL(doc->tab_label)), "\"", NULL);
        message_dialog_new(BFWIN(doc->bfwin)->main_window, 
                     GTK_MESSAGE_ERROR, 
                     GTK_BUTTONS_CLOSE, 
                     _("File save failed!\n"), 
                     errmessage);
        g_free(errmessage);
        doc->action.save = NULL;
        gtk_text_view_set_editable(GTK_TEXT_VIEW(doc->view),TRUE);
        docsavebackend_cleanup(dsb);
      }
    break;
    case CHECKANDSAVE_ERROR_MODIFIED:
    case CHECKANDSAVE_ERROR_MODIFIED_FAILED:
      {
        /* we have to ask the user what to do */
        const gchar *buttons[] = {_("_Abort save"), _("_Continue save"), NULL};
        gint retval;
        gchar *tmpstr, *utf8uri;
        utf8uri = g_file_get_uri(doc->uri);
        if (status == CHECKANDSAVE_ERROR_MODIFIED) {
          tmpstr = g_strdup_printf(_("File %s has been modified on disk, overwrite?"), utf8uri);
        } else {
          tmpstr = g_strdup_printf(_("Failed to check if %s has been modified on disk"), utf8uri);
        }
        g_free(utf8uri);
        retval = message_dialog_new_multi(BFWIN(doc->bfwin)->main_window,
                        GTK_MESSAGE_WARNING,buttons,"",tmpstr);
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
      if (dsb->unlink_uri) {
        file_delete_file_async(dsb->unlink_uri, docsavebackend_async_unlink_lcb, dsb);
      }
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
        /* YES! we're done! update the fileinfo !*/
        DEBUG_MSG("doc_checkNsave_lcb, re-set async doc->fileinfo (current=%p)\n",doc->fileinfo);
        if (doc->fileinfo) g_object_unref(doc->fileinfo);
        doc->fileinfo = NULL;
        file_doc_fill_fileinfo(doc, doc->uri);
        if (main_v->props.clear_undo_on_save) {
          doc_unre_clear_all(doc);
        }
        doc_set_modified(doc, 0);
        /* in fact the filebrowser should also be refreshed if the document was closed, but
        when a document is closed, the filebrowser is anyway refreshed (hmm perhaps only if 
        'follow document focus' is set).*/
        if (dsb->unlink_uri && dsb->fbrefresh_uri) {
          GFile *parent1, *parent2;
          parent1 = g_file_get_parent(dsb->unlink_uri);
          parent2 = g_file_get_parent(dsb->fbrefresh_uri);
          if (!g_file_equal(parent1,parent2)) {
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
gchar *ask_new_filename(Tbfwin *bfwin,gchar *old_curi, const gchar *gui_name, gboolean is_move) {
  Tdocument *exdoc;
  GList *alldocs;
  gchar *new_curi = NULL;
  GFile *uri;
  gchar *dialogtext;
  GtkWidget *dialog;
  
  dialogtext = g_strdup_printf((is_move) ? _("Move/rename %s to"): _("Save %s as"), gui_name);
  dialog = file_chooser_dialog(bfwin, dialogtext, GTK_FILE_CHOOSER_ACTION_SAVE, old_curi, FALSE, FALSE, NULL, FALSE);
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
  uri = g_file_new_for_uri(new_curi); 
  exdoc = documentlist_return_document_from_uri(alldocs, uri);
  g_object_unref(uri);
  g_list_free(alldocs);
  DEBUG_MSG("ask_new_filename, exdoc=%p, newfilename=%s\n", exdoc, new_curi);
  if (exdoc) {
    gchar *tmpstr;
    gint retval;
    const gchar *buttons[] = {_("_Cancel"), _("_Overwrite"), NULL};
    tmpstr = g_strdup_printf(_("File %s exists and is open, overwrite?"), new_curi);
    retval = message_dialog_new_multi(bfwin->main_window,
                           GTK_MESSAGE_WARNING,
                           buttons,
                           tmpstr,
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
    exists = g_file_query_exists(tmp,NULL);
    g_object_unref(tmp);
    if (exists) {
      gchar *tmpstr;
      gint retval;
      const gchar *buttons[] = {_("_Cancel"), _("_Overwrite"), NULL};
      tmpstr = g_strdup_printf(_("A file named \"%s\" already exists."), new_curi);
      retval = message_dialog_new_multi(bfwin->main_window,
                             GTK_MESSAGE_WARNING,
                             buttons,
                             tmpstr,
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

void doc_save_backend(Tdocument *doc, gboolean do_save_as, gboolean do_move, gboolean close_doc, gboolean close_window) {
  gchar *tmp;
  Trefcpointer *buffer;
  gchar *curi=NULL;
  Tdocsavebackend *dsb;
  DEBUG_MSG("doc_save_backend, started for doc %p, save_as=%d, do_move=%d, close_doc=%d, close_window=%d\n",doc,do_save_as,do_move,close_doc,close_window);
  
  if(doc->readonly) {
    g_print("Cannot save readonly document !?!?");
    return;
  }
  
  dsb = g_new0(Tdocsavebackend,1);
  dsb->doc = doc;

  /* should be moved to a plugin interface, because this is HTML specific */
  /* update author meta tag */
  if (main_v->props.auto_update_meta_author) {
    const gchar *realname = g_get_real_name();
    if (realname && strlen(realname) > 0)  {
      gchar *author_tmp;
      author_tmp = g_strconcat("<meta name=\"author\" content=\"",realname,"\" ",NULL);
      snr2_run_extern_replace(doc,"<meta[ \t\n]+name[ \t\n]*=[ \t\n]*\"author\"[ \t\n]+content[ \t\n]*=[ \t\n]*\"[^\"]*\"[ \t\n]*",0,1,0,author_tmp,TRUE);
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
    strftime(isotime, 30, "%Y-%m-%dT%H:%M:%S%z", time_struct);
    
    date_tmp = g_strconcat("<meta name=\"date\" content=\"",isotime,"\" ",NULL);
    snr2_run_extern_replace(doc,"<meta[ \t\n]+name[ \t\n]*=[ \t\n]*\"date\"[ \t\n]+content[ \t\n]*=[ \t\n]*\"[^\"]*\"[ \t\n]*",0,1,0,date_tmp,TRUE);
    g_free(date_tmp);
  }
  
  /* update generator meta tag */
  if (main_v->props.auto_update_meta_generator) {
    snr2_run_extern_replace(doc,"<meta[ \t\n]+name[ \t\n]*=[ \t\n]*\"generator\"[ \t\n]+content[ \t\n]*=[ \t\n]*\"[^\"]*\"[ \t\n]*",0,1,0,"<meta name=\"generator\" content=\"Bluefish "VERSION"\" ", TRUE);
  }

  if (doc->uri) curi = g_file_get_uri(doc->uri);

  if(doc->action.save) {
    gchar *errmessage;
    /* this message is not in very nice english I'm afraid */
    errmessage = g_strconcat(_("File:\n\""),gtk_label_get_text(GTK_LABEL(doc->tab_label)), "\" save is in progress", NULL);
    message_dialog_new(BFWIN(doc->bfwin)->main_window, 
                 GTK_MESSAGE_ERROR, 
                 GTK_BUTTONS_CLOSE, 
                 _("Save in progress!"), 
                 errmessage);
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
    newfilename = ask_new_filename(BFWIN(doc->bfwin), curi, gtk_label_get_text(GTK_LABEL(doc->tab_label)), do_move);
    if (!newfilename) {
      if (curi) g_free(curi);
      g_free(dsb);
      return;
    }
    if (doc->uri) {
      if (do_move) {
        dsb->unlink_uri = doc->uri;/* unlink this uri later */
        g_object_ref(dsb->unlink_uri);
      }
      g_object_unref(doc->uri);
    }

    doc->uri = g_file_new_for_uri(newfilename);

    if (curi) g_free(curi);
    curi = newfilename;
    DEBUG_MSG("doc_save_backend, new uri=%s\n",curi);
    dsb->fbrefresh_uri = doc->uri; /* refresh this uri later */
    g_object_ref(dsb->fbrefresh_uri);
  }
  session_set_savedir(doc->bfwin, curi);

  tmp = doc_get_buffer_in_encoding(doc);
  if (!tmp) {
    g_free(curi);
    return;
  }

  buffer = refcpointer_new(tmp);
  doc->action.close_doc = close_doc;
  doc->action.close_window = close_window;
  gtk_text_view_set_editable(GTK_TEXT_VIEW(doc->view),FALSE);
  DEBUG_MSG("doc_save_backend, calling file_checkNsave_uri_async for %d bytes\n",strlen(buffer->data));
  doc->action.save = file_checkNsave_uri_async(doc->uri, doc->fileinfo, buffer, strlen(buffer->data), !do_save_as, doc_checkNsave_lcb, dsb);

  if (do_save_as) {
    doc->readonly = FALSE;
    doc_reset_filetype(doc, doc->uri, buffer->data, strlen(buffer->data));
    doc_set_title(doc);
    doc_force_activate(doc);
  }
  refcpointer_unref(buffer);

  g_free(curi);
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
 *  Save all editor notebooks
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
    const gchar *buttons[] = {_("Close _Without Saving"), GTK_STOCK_CANCEL, GTK_STOCK_SAVE, NULL};
    gchar *text;
    gint retval;
    text = g_strdup_printf(_("Save changes to \"%s\" before closing?"),gtk_label_get_text (GTK_LABEL (doc->tab_label)));
    retval = message_dialog_new_multi(BFWIN(doc->bfwin)->main_window,
                           GTK_MESSAGE_QUESTION,
                           buttons,
                           text,
                           _("If you don't save your changes they will be lost."));
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
    const gchar *buttons[] = {_("Choose per _File"), _("Close _All"), _("_Cancel"), _("_Save All"), NULL};
    retval = message_dialog_new_multi(bfwin->main_window,
                           GTK_MESSAGE_QUESTION,
                           buttons,
                           _("Multiple open files have been changed."),
                           _("If you don't save your changes they will be lost."));
    if (retval == 2) {
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
    case 0: /* choose per file */
      doc_close_single_backend(tmpdoc, TRUE, close_window);
    break;
    case 1: /* close all */
       /* fake that this document was not modified */
      tmpdoc->modified = FALSE;
        doc_close_single_backend(tmpdoc, TRUE, close_window);
    break;
    case 3: /* save all */
      doc_save_backend(tmpdoc, FALSE, FALSE, TRUE, close_window);
    break;
    }
    tmplist = g_list_next(tmplist);
  }
  g_list_free(duplist);
  if (!close_window && retval != 2) {
    notebook_changed(bfwin, -1);
  }
/*  if (retval == 1) {
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
    GFile *uri;
    uri = g_file_new_for_uri (bfwin->project->template);

    if (uri) {
      file_into_doc(bfwin->current_document, uri, TRUE);
      g_object_unref (uri);
    }
    /*doc_file_to_textbox(doc, bfwin->project->template, FALSE, FALSE);
    doc_activate(doc);*/
  }
}

static void file_reload_all_modified_check_lcb(Tcheckmodified_status status,gint error_info,GFileInfo *orig, GFileInfo *new, gpointer user_data) {
	if (status == CHECKMODIFIED_MODIFIED) {
		DEBUG_MSG("file_reload_all_modified_check_lcb, reload %p\n",user_data);
		
		doc_reload(DOCUMENT(user_data), new);
	}
}

void file_reload_all_modified(Tbfwin *bfwin) {
	GList *tmplist = g_list_first(bfwin->documentlist);
	while (tmplist) {
		if (DOCUMENT(tmplist->data)->uri && DOCUMENT(tmplist->data)->status == DOC_STATUS_COMPLETE) {
			DEBUG_MSG("file_reload_all_modified, check %p\n",tmplist->data);
			file_checkmodified_uri_async(DOCUMENT(tmplist->data)->uri, DOCUMENT(tmplist->data)->fileinfo, file_reload_all_modified_check_lcb, tmplist->data);
		}
		tmplist = g_list_next(tmplist);
	}
}
