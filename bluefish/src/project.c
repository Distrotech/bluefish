/* Bluefish HTML Editor
 * project.c - project functionality
 *
 * Copyright (C) 2003-2008 Olivier Sessink
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

#define DEBUG

#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

#include "bluefish.h"
#include "project.h"
#include "stringlist.h"
#include "gui.h"
#include "dialog_utils.h"
#include "document.h"
#include "gtk_easy.h"
#include "rcfile.h"
#include "bf_lib.h"
#include "filebrowser2.h"
#include "file_dialogs.h"
#include "menu.h"
#include "bookmark.h"
#include "plugins.h"

static void free_session(Tsessionvars *session) {
	free_stringlist(session->classlist);
	free_stringlist(session->colorlist);
	free_stringlist(session->targetlist);
	free_stringlist(session->urllist);
	free_stringlist(session->fontlist);
	free_stringlist(session->positionlist);
	free_stringlist(session->searchlist);
	free_stringlist(session->replacelist);
	free_arraylist(session->bmarks);
	free_stringlist(session->recent_files);
	free_stringlist(session->recent_dirs);
	if (session->opendir) g_free(session->opendir);
	if (session->savedir) g_free(session->savedir);
	if (session->encoding) g_free(session->encoding);
	if (session->last_filefilter) g_free(session->last_filefilter);
	g_free(session);
}

static void project_setup_initial_session(Tsessionvars *session, gboolean before_parse) {
	if (!before_parse) {
		/* session parts that will be filled by rcfile_parse() will not be included
		   in this initial setup */
		session->webroot = g_strdup(main_v->session->webroot);
		session->documentroot = g_strdup(main_v->session->documentroot);
		session->encoding = g_strdup(main_v->session->encoding);
		session->opendir = g_strdup(main_v->session->opendir);
		session->savedir = g_strdup(main_v->session->savedir);
		session->last_filefilter = g_strdup(main_v->session->last_filefilter);
	}
	session->snr_is_expanded = main_v->session->snr_is_expanded;
	session->adv_open_recursive = main_v->session->adv_open_recursive;
	session->adv_open_matchname = main_v->session->adv_open_matchname;
	session->bookmarks_filename_mode = main_v->session->bookmarks_filename_mode;
	session->bookmarks_show_mode = main_v->session->bookmarks_show_mode;
	session->filebrowser_focus_follow = main_v->session->filebrowser_focus_follow;
	session->filebrowser_show_hidden_files = main_v->session->filebrowser_show_hidden_files;
	session->filebrowser_show_backup_files = main_v->session->filebrowser_show_backup_files;
	session->filebrowser_viewmode = main_v->session->filebrowser_viewmode;
	session->snr_position_x = main_v->session->snr_position_x;
	session->snr_position_y = main_v->session->snr_position_y;
	session->view_main_toolbar = main_v->session->view_main_toolbar;
	session->view_left_panel = main_v->session->view_left_panel;
}

Tbfwin *project_is_open(GFile *uri) {
	GList *tmplist;
	tmplist = g_list_first(main_v->bfwinlist);
	while(tmplist){
		if (BFWIN(tmplist->data)->project 
				&& BFWIN(tmplist->data)->project->uri
				&& g_file_equal( BFWIN(tmplist->data)->project->uri, uri )) {
			return BFWIN(tmplist->data);
		}
		tmplist = g_list_next(tmplist);
	}
	return NULL;
}

static void update_project_filelist(Tbfwin *bfwin, Tproject *prj) {
	GList *tmplist;
	DEBUG_MSG("update_project_filelist, started, bfwin=%p, prj=%p\n",bfwin,prj);
	free_stringlist(prj->files);
	DEBUG_MSG("update_project_filelist, old list free'd, creating new list from documentlist %p (len=%d)\n",bfwin->documentlist,g_list_length(bfwin->documentlist));
	tmplist = return_urilist_from_doclist(bfwin->documentlist);
	prj->files = urilist_to_stringlist(tmplist);
	free_urilist(tmplist);
}

static void setup_bfwin_for_project(Tbfwin *bfwin) {
	DEBUG_MSG("setup_bfwin_for_project, bfwin=%p, bfwin->project=%p, bfwin->session=%p\n",bfwin,bfwin->project,bfwin->session);
	DEBUG_MSG("setup_bfwin_for_project, bfwin->project->session=%p\n",bfwin->project->session);
	bfwin->bmarkdata = bfwin->project->bmarkdata;
	bmark_set_store(bfwin);
	bmark_reload(bfwin);

	gui_set_main_toolbar_visible(bfwin, bfwin->session->view_main_toolbar, TRUE);
	left_panel_show_hide_toggle(bfwin,FALSE,bfwin->session->view_left_panel, TRUE);
	gui_statusbar_show_hide_toggle(bfwin, bfwin->session->view_statusbar, TRUE);
	fb2_update_settings_from_session(bfwin);

	recent_menu_from_list(bfwin, bfwin->project->session->recent_files, FALSE);
	set_project_menu_widgets(bfwin, TRUE);
	/* force this session in the plugins */
	DEBUG_MSG("project.c: setup_bfwin_for_project: calling bfplugins_enforce_session for all plugins in %p\n",main_v->plugins);
	g_slist_foreach(main_v->plugins, bfplugins_enforce_session, bfwin);
}

/* bfwin is allowed to be NULL for an empty project */
static Tproject *create_new_project(Tbfwin *bfwin) {
	Tproject *prj;
	prj = g_new0(Tproject,1);
	prj->name = g_strdup(_("New project"));
	prj->bmarkdata = bookmark_data_new(); 
	DEBUG_MSG("create_new_project, project=%p, bfwin=%p\n",prj,bfwin);
	if (bfwin) {
		DEBUG_MSG("create_new_project, new project for bfwin %p\n",bfwin);
		update_project_filelist(bfwin,prj);
		bfwin->project = prj;
	} else {
		DEBUG_MSG("create_new_project, new project, no bfwin\n");
	}
	prj->session = g_new0(Tsessionvars,1);
	project_setup_initial_session(prj->session, FALSE);
	
	if (bfwin && prj->files) {
		GList *tmplist;
		tmplist = g_list_first(bfwin->documentlist);
		while (tmplist) {
			bmark_clean_for_doc(DOCUMENT(tmplist->data));
			tmplist = g_list_next(tmplist);
		}
	
		tmplist = g_list_first(bfwin->session->bmarks);
		while (tmplist) {
			gchar **entry = (gchar**)tmplist->data;
			if (count_array(entry) > 2) {
				GList *tmplist2 = g_list_first(prj->files);
				while (tmplist2) {
					if (strcmp(tmplist2->data, entry[2])==0) {
						/* move it out of the default session into this session */
						bfwin->session->bmarks = g_list_remove_link(bfwin->session->bmarks,tmplist);
						prj->session->bmarks = g_list_concat(prj->session->bmarks, tmplist);
						/* no further filenames to check */
						tmplist2 = g_list_last(tmplist2);
					}
					tmplist2 = g_list_next(tmplist2);
				}
			}
			tmplist = g_list_next(tmplist);
		}
	}
	prj->template = g_strdup("");
	prj->word_wrap = main_v->props.word_wrap;
	if (bfwin) {
		bfwin->session = prj->session;
		setup_bfwin_for_project(bfwin);
	}
	return prj;
}

gboolean project_save(Tbfwin *bfwin, gboolean save_as) {
	gboolean retval;
	DEBUG_MSG("project_save, bfwin=%p, save_as=%d\n",bfwin,save_as);
	if (!bfwin->project) {
		/* there is no project yet, we have to create one */
		DEBUG_MSG("project_save, bfwin=%p does not have a project yet, create one\n",bfwin);
		bfwin->project= create_new_project(bfwin);
  	   bmark_reload(bfwin);
	}
	DEBUG_MSG("project_save, project=%p, num files was %d\n", bfwin->project, g_list_length(bfwin->project->files));
	update_project_filelist(bfwin, bfwin->project);
/*	bfwin->project->recentfiles = limit_stringlist(bfwin->project->recentfiles, main_v->props.max_recent_files, TRUE);*/
	
	bfwin->project->session->searchlist = limit_stringlist(bfwin->project->session->searchlist, 10, FALSE);
	bfwin->project->session->replacelist = limit_stringlist(bfwin->project->session->replacelist, 10, FALSE);
	bfwin->project->session->urllist = limit_stringlist(bfwin->project->session->urllist, 10, TRUE);
	bmark_store_all(bfwin);

	if (save_as || bfwin->project->uri == NULL) {
		gint suflen,filen;
		GtkWidget *dialog;
		GFile *newuri=NULL;
		dialog = file_chooser_dialog(bfwin,_("Enter Bluefish project filename"),GTK_FILE_CHOOSER_ACTION_SAVE, NULL, TRUE, FALSE, "bfproject", FALSE);
		if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
			gchar *filename;
			newuri = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
			filename = g_file_get_parse_name(newuri);
			suflen = strlen(main_v->props.project_suffix);
			filen = strlen(filename);
	
			if (filen < suflen || strcmp(&filename[filen - suflen], main_v->props.project_suffix)==0) {
				GFile *tmp2;
				gchar *tmp = g_strconcat(filename, main_v->props.project_suffix,NULL);
				tmp2 = g_file_parse_name(tmp);
				g_free(tmp);
				g_object_unref(newuri);
				newuri = tmp2;
			}		
			g_free(filename);
		}
		gtk_widget_destroy(dialog);
		if (!newuri) {
			return FALSE;
		}
		if (save_as || bfwin->project->uri == NULL) {
			gboolean exists;
			exists = g_file_query_exists(newuri,NULL);
			if (exists) {
				gchar *tmpstr,*filename;
				gint retval;
				const gchar *buttons[] = {_("_Cancel"), _("_Overwrite"), NULL};
				filename = g_file_get_parse_name(newuri);
				tmpstr = g_strdup_printf(_("A file named \"%s\" already exists."), filename);
				retval = message_dialog_new_multi(bfwin->main_window,
															 GTK_MESSAGE_WARNING,
															 buttons,
															 tmpstr,
															 _("Do you want to replace the existing file?"));
				g_free(tmpstr);
				g_free(filename);
				if (retval == 0) {
					g_object_unref(newuri);
					return FALSE;
				}
			}
		}
		bfwin->project->uri = newuri;
	}
	
	retval = rcfile_save_project(bfwin->project, bfwin->project->uri);
	DEBUG_MSG("project_save, retval=%d\n",retval);
	add_to_recent_list(bfwin,bfwin->project->uri, FALSE, TRUE);
	return retval;
}

void set_project_menu_widgets(Tbfwin *bfwin, gboolean win_has_project) {
	menuitem_set_sensitive(bfwin->menubar, "/Project/Save", win_has_project);
	menuitem_set_sensitive(bfwin->menubar, "/Project/Save as...", win_has_project);
	menuitem_set_sensitive(bfwin->menubar, "/Project/Save & close", win_has_project);
	menuitem_set_sensitive(bfwin->menubar, "/Project/Edit Project Options...", win_has_project);
}

void project_open_from_file(Tbfwin *bfwin, GFile *fromuri) {
	Tbfwin *prwin;
	Tproject *prj;
	gboolean retval;
	GList *tmplist;

	/* first we test if the project is already open */
	prwin = project_is_open(fromuri);
	if (prwin != NULL) {
		DEBUG_MSG("project_open_from_file, project is open in bfwin=%p\n",prwin);
		gtk_window_present(GTK_WINDOW(prwin->main_window));
		return;
	}

	prj = g_new0(Tproject,1);
	prj->session = g_new0(Tsessionvars,1);
	DEBUG_MSG("project_open_from_file, project=%p, session=%p\n",prj,prj->session);
	project_setup_initial_session(prj->session, TRUE);
	prj->bmarkdata = bookmark_data_new();
	retval = rcfile_parse_project(prj, fromuri);
	if (!retval) {
		DEBUG_MSG("project_open_from_file, failed parsing the project\n");
		bookmark_data_cleanup(prj->bmarkdata);
		g_free(prj->session);
		g_free(prj);
		return;
	}
	add_to_recent_list(bfwin,fromuri, FALSE, TRUE);
	prj->uri = fromuri;
	g_object_ref(fromuri);
	if (bfwin->project == NULL && test_only_empty_doc_left(bfwin->documentlist)) {
		/* we will use this Bluefish window to open the project */
		prwin = bfwin;
		prwin->project = prj;
		prwin->session = prj->session;
		DEBUG_MSG("project_open_from_file, project %p will be in existing prwin=%p\n",prj,bfwin);
		/* now we need to clean the session, and reset it to the session from the project */
		/* free_session(bfwin->session); there is no session specific to a window anymore, only a global one*/
	} else {
		/* we will open a new Bluefish window for this project */
		DEBUG_MSG("project_open_from_file, we need a new window\n");
		prwin = gui_new_window(prj);
	}
	tmplist = g_list_last(prj->files);
	while (tmplist) {
		GFile *uri;
		if (strstr ((gchar *) tmplist->data, "://") == NULL)
			uri = g_file_new_for_path((gchar *) tmplist->data);
		else
			uri = g_file_new_for_uri((gchar *) tmplist->data);
		doc_new_from_uri(prwin, uri, NULL, (prj->files->next==NULL), TRUE, -1, -1);
		g_object_unref(uri);
		tmplist = g_list_previous(tmplist);
	}
	DEBUG_MSG("project_open_from_file, new window with files ready at prwin=%p\n",prwin);
	setup_bfwin_for_project(prwin);
	set_project_menu_widgets(prwin, TRUE);
	recent_menu_init_project(prwin);
	DEBUG_MSG("project_open_from_file, done\n");
}

static void project_open_response_lcb(GtkDialog *dialog,gint response,Tbfwin *bfwin) {
	if (response == GTK_RESPONSE_ACCEPT) {
		GFile *file;
		file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
		project_open_from_file(bfwin,file);
		g_object_unref(file);
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
}
static void project_open(Tbfwin *bfwin) {
	GtkWidget *dialog;
	dialog = file_chooser_dialog(bfwin, _("Select Bluefish project filename"), GTK_FILE_CHOOSER_ACTION_OPEN, NULL, TRUE, FALSE, "bfproject", FALSE);
	g_signal_connect(dialog, "response", G_CALLBACK(project_open_response_lcb), bfwin);
	gtk_widget_show_all(dialog);
}

static void project_destroy(Tproject *project) {
	DEBUG_MSG("project_destroy, project=%p, project->session=%p\n",project,project->session);
	bookmark_data_cleanup(project->bmarkdata);
	free_stringlist(project->files);
	free_session(project->session);
	g_object_unref(project->uri);
	g_free(project->name);
	g_free(project->template);
	g_free(project);
}

static void setup_bfwin_for_nonproject(Tbfwin *bfwin) {
	DEBUG_MSG("setup_bfwin_for_nonproject, bfwin=%p\n",bfwin);
	bfwin->session = main_v->session;
	bfwin->bmarkdata = main_v->bmarkdata;
	bfwin->project = NULL;
	bmark_set_store(bfwin);
	/* normally there is always a current_document, but this function might be called in the transition
	after all documents are just closed */
	if (bfwin->current_document) gui_set_title(bfwin, bfwin->current_document);
	fb2_update_settings_from_session(bfwin);
	recent_menu_from_list(bfwin, main_v->session->recent_files, FALSE);
	set_project_menu_widgets(bfwin, FALSE);

	/* force this session in the plugins */
	g_slist_foreach(main_v->plugins, bfplugins_enforce_session, bfwin);
}

void project_save_and_mark_closed(Tbfwin *bfwin) {
	if (bfwin->project) {
		project_save(bfwin, FALSE);
		add_to_recent_list(bfwin,bfwin->project->uri, TRUE, TRUE);
		bfwin->project->close=TRUE;
	}
}

/* 
 * returns TRUE if the project is closed, 
 * returns FALSE if something went wrong or was cancelled
 */
gboolean project_save_and_close(Tbfwin *bfwin, gboolean close_win) {
	gboolean dont_save = FALSE;
	DEBUG_MSG("project_save_and_close, bfwin=%p, project=%p, close_win=%d, project->close=%d\n",bfwin,bfwin->project,close_win,bfwin->project->close);
	if (bfwin->project->close) {
		project_destroy(bfwin->project);
		/* we should only set the window for nonproject if the window will keep alive*/
		if (!close_win)setup_bfwin_for_nonproject(bfwin);
		return TRUE;
	}
	
	while (!bfwin->project->uri) {
		gchar *text;
		gint retval;
		const gchar *buttons[] = {_("Close _Without Saving"), GTK_STOCK_CANCEL, GTK_STOCK_SAVE, NULL};
		if (dont_save) {
			break;
		}
		DEBUG_MSG("project_save_and_close, project not named, getting action\n");
		/* dialog */

		text = g_strdup(_("Save current project?"));
		retval = message_dialog_new_multi(bfwin->main_window,
													 GTK_MESSAGE_QUESTION,
													 buttons,
													 text,
													 _("If you don't save your changes they will be lost."));
		switch (retval) {
		case 0:
			/* don't save proj. save files, though */
			DEBUG_MSG("project_save_and_close, don't save project, but save files\n");
			dont_save = TRUE;
		break;
		case 1:
			/* cancel */
			DEBUG_MSG("project_save_and_close, not closing window any more");
			return FALSE;
		break;
		case 2:
			DEBUG_MSG("project_save_and_close, bringing up save project dialog\n");
			dont_save = project_save(bfwin, FALSE);
		break;
		default:
		break;
		}
	}
	/* test if we should save */
	if (!dont_save) {
		if (!project_save(bfwin, FALSE)) {
			DEBUG_MSG("project_save failed, returning\n");
			return FALSE;
		}
		add_to_recent_list(bfwin,bfwin->project->uri, TRUE, TRUE);
	}
	if (test_only_empty_doc_left(bfwin->documentlist)) {
		project_destroy(bfwin->project);
		setup_bfwin_for_nonproject(bfwin);
	} else {
		gboolean retval;
		/* the last document that closes should close the window, and in the window-delete-event handler (in gui.c)
		project_save_and_close() is called again, which will clean-up the memory */
		DEBUG_MSG("project_save_and_close, documents are closing!, setting project->close to TRUE for project %p in bfwin %p\n", bfwin->project, bfwin);
		retval = doc_close_multiple_backend(bfwin, close_win);
		if (retval) {
			bfwin->project->close = TRUE;
		}
		return retval;
	}
	return TRUE;
}

typedef enum {
	name,
	template,
	word_wrap,
	projecteditor_entries_num
} Tprojecteditor_entries;

typedef struct {
	GtkWidget *win;
	Tbfwin *bfwin;
	Tproject *project;
	GtkWidget *entries[projecteditor_entries_num];
	gboolean destroy_project_on_close;
} Tprojecteditor;

static void project_edit_destroy_lcb(GtkWidget *widget, Tprojecteditor *pred) {
	DEBUG_MSG("project_edit_destroy_lcb, called for pred=%p\n",pred);
/*	gtk_widget_destroy(pred->win);*/
	if (pred->destroy_project_on_close) {
		project_destroy(pred->project);
		if (pred->bfwin) {
			setup_bfwin_for_nonproject(pred->bfwin);
		}
	}
	if (pred->project) {
		pred->project->editor = NULL;
	}
	g_free(pred);
}

static void project_edit_cancel_clicked_lcb(GtkWidget *widget, Tprojecteditor *pred) {
	
	gtk_widget_destroy(pred->win);
}

static void project_edit_ok_clicked_lcb(GtkWidget *widget, Tprojecteditor *pred) {
	Tproject *prj = pred->project;
	
	pred->destroy_project_on_close = FALSE;
	
	gtk_widget_hide(pred->win);
	DEBUG_MSG("project_edit_ok_clicked_lcb, Tproject at %p, bfwin at %p\n",prj,pred->bfwin);
	string_apply(&prj->name, pred->entries[name]);
	string_apply(&prj->template, pred->entries[template]);
	integer_apply(&prj->word_wrap, pred->entries[word_wrap], TRUE);

	if (pred->bfwin == NULL) {
		pred->bfwin = gui_new_window(pred->project);
		pred->bfwin->session = pred->project->session;
		setup_bfwin_for_project(pred->bfwin);
	} else {
		gui_set_title(pred->bfwin, pred->bfwin->current_document);
	}
/* set_project_menu_widgets(pred->bfwin, TRUE);*/
	project_save(pred->bfwin,FALSE);
	gtk_widget_destroy(pred->win);
}

/*
if NULL is passed as bfwin, a new window will be created for this project once OK is clicked
*/
void project_edit(Tbfwin *bfwin) {
	GtkWidget *vbox, *but, *hbox, *label, *table;
	gchar *wintitle = NULL;
	Tprojecteditor *pred;

	if (bfwin && bfwin->project && bfwin->project->editor) {
		gtk_window_present(GTK_WINDOW(((Tprojecteditor *)bfwin->project->editor)->win));
		return;
	}
	pred = g_new(Tprojecteditor,1);
	if (!bfwin || !bfwin->project) {
		/* there is no project yet, we have to create one */
		DEBUG_MSG("project_edit, no project yet, create one\n");
		pred->project= create_new_project(bfwin);
		if (bfwin) {
			bfwin->project = pred->project;
		}
		/* id the user does not press OK, we destroy the project */
		pred->destroy_project_on_close = TRUE;
		wintitle = g_strdup(_("Create New Project"));
	} else {
		pred->destroy_project_on_close = FALSE;
		wintitle = g_strdup(_("Edit Project"));
		if (bfwin) {
			pred->project = bfwin->project;
		}
	}
	DEBUG_MSG("project_edit, Tproject at %p\n",pred->project);
	pred->bfwin = bfwin;
	pred->project->editor = pred;
	
	pred->win = window_full2(wintitle, GTK_WIN_POS_CENTER_ALWAYS, 5
			, G_CALLBACK(project_edit_destroy_lcb), pred, TRUE, NULL);
	vbox = gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(pred->win),vbox);
	
	if (wintitle) {
		g_free(wintitle);
	}
	
	label = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	{
		gchar *message;
		message = g_strdup_printf(_("This project contains %d files"), g_list_length(pred->project->files));
		gtk_label_set_markup(GTK_LABEL(label), message);
		g_free(message);
	}
	
	table = gtk_table_new (5, 4, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);	
	gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 12);
	
	pred->entries[name] = entry_with_text(pred->project->name, 255);
	gtk_widget_set_size_request(GTK_WIDGET(pred->entries[name]), 250, -1);
	bf_mnemonic_label_tad_with_alignment(_("Project _Name:"), pred->entries[name], 1, 0.5, table, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), pred->entries[name], 2, 3, 0, 1);


	pred->entries[template] = entry_with_text(pred->project->template, 255);
	bf_mnemonic_label_tad_with_alignment(_("_Template:"), pred->entries[template], 1, 0.5, table, 0, 1, 3, 4);
	but = file_but_new(pred->entries[template], 1, NULL);
	gtk_table_attach_defaults(GTK_TABLE(table), pred->entries[template], 2, 3, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(table), but, 3, 4, 3, 4);

	gtk_table_set_row_spacing(GTK_TABLE(table), 3, 18);
	pred->entries[word_wrap] = checkbut_with_value(_("_Word wrap by default"), pred->project->word_wrap);
	gtk_table_attach_defaults(GTK_TABLE(table), pred->entries[word_wrap], 0, 3, 4, 5);
		
	gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new(), FALSE, FALSE, 12);
	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	but = bf_stock_cancel_button(G_CALLBACK(project_edit_cancel_clicked_lcb), pred);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 0);
	if (pred->destroy_project_on_close == TRUE) {
		but = bf_allbuttons_backend(_("Create _Project"), 1, 0, G_CALLBACK(project_edit_ok_clicked_lcb), pred);
	} else {
		but = bf_stock_ok_button(G_CALLBACK(project_edit_ok_clicked_lcb), pred);
	}
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 0);
	gtk_widget_grab_default(but);
	gtk_widget_show_all(pred->win);
}

typedef struct {
	GtkWidget *win;
	GtkWidget *rad[2];
	Tbfwin *bfwin;
} Tpc;

static void project_create_destroy_lcb(GtkWidget *widget, Tpc *pc) {
	g_free(pc);
}

static void project_create_cancel_clicked_lcb(GtkWidget *widget, Tpc *pc) {
	gtk_widget_destroy(pc->win);
}

static void project_create_ok_clicked_lcb(GtkWidget *widget, Tpc *pc) {
	gtk_widget_hide(pc->win);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pc->rad[0]))) {
		project_edit(pc->bfwin);
	} else {
		project_edit(NULL);
	}
	gtk_widget_destroy(pc->win);
}


void project_create_gui(Tbfwin *bfwin) {
	GtkWidget *vbox, *hbox, *but;
	Tpc *pc;
	
	pc = g_new(Tpc,1);
	pc->bfwin = bfwin;
	pc->win = window_full2(_("Create project"), GTK_WIN_POS_NONE, 5
			, G_CALLBACK(project_create_destroy_lcb), pc, TRUE, NULL);
	vbox = gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(pc->win),vbox);
	pc->rad[0] = boxed_radiobut_with_value(_("Create project with currently opened documents"), TRUE, NULL, vbox);
	pc->rad[1] = boxed_radiobut_with_value(_("Create empty project"), FALSE, GTK_RADIO_BUTTON(pc->rad[0]), vbox);
	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	but = bf_stock_cancel_button(G_CALLBACK(project_create_cancel_clicked_lcb), pc);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, TRUE, 0);
	but = bf_stock_ok_button(G_CALLBACK(project_create_ok_clicked_lcb), pc);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, TRUE, 0);
	gtk_widget_show_all(pc->win);
}

void project_menu_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget) {
	DEBUG_MSG("project_menu_cb, bfwin=%p, project=%p,callback_action=%d\n",bfwin,bfwin->project,callback_action);
	switch (callback_action) {
	case 1:
		project_open(bfwin);
	break;
	case 2:
		project_save(bfwin, FALSE);
	break;
	case 3:
		project_save(bfwin, TRUE);
	break;
	case 4:
		project_save_and_close(bfwin, (g_list_length(main_v->bfwinlist) > 1));
	break;
	case 5:
		project_edit(bfwin);
	break;
	case 6:
		if (bfwin->project) {
			project_edit(NULL);
		} else {
			if (test_only_empty_doc_left(bfwin->documentlist)) {
				project_edit(bfwin);
			} else {
				project_create_gui(bfwin);
			}
		}
	break;
	default:
		DEBUG_MSG("project_menu_cb, no such callback_action %d\n",callback_action);
		exit(54);
	break;
	}
}

