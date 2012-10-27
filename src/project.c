/* Bluefish HTML Editor
 * project.c - project functionality
 *
 * Copyright (C) 2003-2012 Olivier Sessink
 * Copyright (C) 2011 James Hayward
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

/*#define DEBUG*/

#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

#include "bluefish.h"

#ifdef MAC_INTEGRATION
/*#include <ige-mac-integration.h>*/
#include <gtkosxapplication.h>
#endif

#include "bf_lib.h"
#include "bftextview2_spell.h"
#include "bfwin.h"
#include "bfwin_uimanager.h"
#include "bookmark.h"
#include "dialog_utils.h"
#include "document.h"
#include "filebrowser2.h"
#include "file_dialogs.h"
#include "gtk_easy.h"
#include "plugins.h"
#include "preferences.h"
#include "project.h"
#include "rcfile.h"
#include "stringlist.h"


static void
free_session(Tsessionvars * session)
{
	/* call free_session for any of the plugins that have free_session defined */
	bfplugins_session_cleanup(session);
	free_stringlist(session->classlist);
	free_stringlist(session->colorlist);
	free_stringlist(session->targetlist);
	free_stringlist(session->urllist);
	free_stringlist(session->fontlist);
	free_stringlist(session->positionlist);
	free_stringlist(session->searchlist);
	free_stringlist(session->replacelist);
	free_stringlist(session->filegloblist);
	free_arraylist(session->bmarks);
	free_stringlist(session->recent_files);
	free_stringlist(session->recent_dirs);
	g_free(session->ssearch_text);
	g_free(session->opendir);
	g_free(session->savedir);
	g_free(session->encoding);
	g_free(session->last_filefilter);
	g_free(session->default_mime_type);
	g_free(session->template);
	g_free(session->webroot);
	g_free(session->documentroot);
	g_free(session->sync_local_uri);
	g_free(session->sync_remote_uri);
	g_free(session->convertcolumn_separator);
	g_free(session->convertcolumn_fillempty);
#ifdef HAVE_LIBENCHANT
	g_free(session->spell_lang);
#endif
	g_free(session);
}

static void
project_setup_initial_session(Tsessionvars * session, gboolean before_parse)
{
	if (!before_parse) {
		/* session parts that will be filled by rcfile_parse() will not be included
		   if before parse is TRUE (this is thus only used for new projects) */
		session->webroot = g_strdup(main_v->session->webroot);
		session->documentroot = g_strdup(main_v->session->documentroot);
		session->encoding = g_strdup(main_v->session->encoding);
		session->opendir = g_strdup(main_v->session->opendir);
		session->savedir = g_strdup(main_v->session->savedir);
		session->last_filefilter = g_strdup(main_v->session->last_filefilter);
		session->default_mime_type = g_strdup(main_v->session->default_mime_type);
		session->template = g_strdup(main_v->session->template);
#ifdef HAVE_LIBENCHANT
		session->spell_lang = g_strdup(main_v->session->spell_lang);
		session->spell_check_default = main_v->session->spell_check_default;
#endif
	}

	/* THE NUBER OF BYTES THAT IS COPIED HERE MUST BE EQUAL TO THE NUMBER OF INTEGERS
	   IN THE STRUCT IN bluefish.h ELSE ALL WILL FAIL */
	memcpy(session, main_v->session, 44 * sizeof(gint));
}

Tbfwin *
project_is_open(GFile * uri)
{
	GList *tmplist;
	tmplist = g_list_first(main_v->bfwinlist);
	while (tmplist) {
		if (BFWIN(tmplist->data)->project
			&& BFWIN(tmplist->data)->project->uri && g_file_equal(BFWIN(tmplist->data)->project->uri, uri)) {
			return BFWIN(tmplist->data);
		}
		tmplist = g_list_next(tmplist);
	}
	return NULL;
}

static void
update_project_filelist(Tbfwin * bfwin, Tproject * prj)
{
	GList *tmplist;
	DEBUG_MSG("update_project_filelist, started, bfwin=%p, prj=%p\n", bfwin, prj);
	free_stringlist(prj->files);
	DEBUG_MSG("update_project_filelist, old list free'd, creating new list from documentlist %p (len=%d)\n",
			  bfwin->documentlist, g_list_length(bfwin->documentlist));
	tmplist = return_urilist_from_doclist(bfwin->documentlist);
	prj->files = urilist_to_stringlist(tmplist);
	free_urilist(tmplist);
}

void
set_project_menu_actions(Tbfwin * bfwin, gboolean win_has_project)
{
	gtk_action_group_set_sensitive(bfwin->projectGroup, win_has_project);
}

static void
setup_bfwin_for_project(Tbfwin * bfwin)
{
	DEBUG_MSG("setup_bfwin_for_project, bfwin=%p, bfwin->project=%p, bfwin->session=%p\n", bfwin,
			  bfwin->project, bfwin->session);
	DEBUG_MSG("setup_bfwin_for_project, bfwin->project->session=%p\n", bfwin->project->session);
	bfwin->bmarkdata = bfwin->project->bmarkdata;
	bmark_set_store(bfwin);
	bmark_reload(bfwin);
#ifdef HAVE_LIBENCHANT
	reload_spell_dictionary(bfwin);
#endif
	bfwin_apply_session(bfwin);
	set_project_menu_actions(bfwin, TRUE);
#ifdef MAC_INTEGRATION
/*	ige_mac_menu_sync(GTK_MENU_SHELL(BFWIN(doc->bfwin)->menubar));*/
	gtk_osxapplication_sync_menubar(g_object_new(GTK_TYPE_OSX_APPLICATION, NULL));
#endif
}

static void
setup_bfwin_for_nonproject(Tbfwin * bfwin)
{
	DEBUG_MSG("setup_bfwin_for_nonproject, bfwin=%p\n", bfwin);
	bfwin->session = main_v->session;
	bfwin->bmarkdata = main_v->bmarkdata;
	bfwin->project = NULL;
	bmark_set_store(bfwin);
	/* normally there is always a current_document, but this function might be called in the transition
	   after all documents are just closed */
	if (bfwin->current_document)
		bfwin_set_title(bfwin, bfwin->current_document, 0);

	bfwin_apply_session(bfwin);
	set_project_menu_actions(bfwin, FALSE);
#ifdef MAC_INTEGRATION
/*	ige_mac_menu_sync(GTK_MENU_SHELL(BFWIN(bfwin)->menubar));*/
	gtk_osxapplication_sync_menubar(g_object_new(GTK_TYPE_OSX_APPLICATION, NULL));
#endif
}

static void
project_destroy(Tproject * project)
{
	DEBUG_MSG("project_destroy, project=%p, project->session=%p\n", project, project->session);
	bookmark_data_cleanup(project->bmarkdata);
	if (project->files)
		free_stringlist(project->files);
	if (project->session)
		free_session(project->session);
	if (project->uri)
		g_object_unref(project->uri);
	if (project->name)
		g_free(project->name);
	g_free(project);
}

/* bfwin is allowed to be NULL for an empty project */
static Tproject *
create_new_project(Tbfwin * bfwin)
{
	Tproject *prj;
	prj = g_new0(Tproject, 1);
	prj->name = g_strdup(_("New project"));
	prj->bmarkdata = bookmark_data_new();
	DEBUG_MSG("create_new_project, project=%p, bfwin=%p\n", prj, bfwin);
	if (bfwin) {
		DEBUG_MSG("create_new_project, new project for bfwin %p\n", bfwin);
		update_project_filelist(bfwin, prj);
		bfwin->project = prj;
	} else {
		DEBUG_MSG("create_new_project, new project, no bfwin\n");
	}
	prj->session = g_new0(Tsessionvars, 1);
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
			gchar **entry = (gchar **) tmplist->data;
			if (g_strv_length(entry) > 2) {
				GList *tmplist2 = g_list_first(prj->files);
				while (tmplist2) {
					if (strcmp(tmplist2->data, entry[2]) == 0) {
						/* move it out of the default session into this session */
						bfwin->session->bmarks = g_list_remove_link(bfwin->session->bmarks, tmplist);
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
	if (bfwin) {
		bfwin->session = prj->session;
		setup_bfwin_for_project(bfwin);
	}
	return prj;
}

gboolean
project_save(Tbfwin * bfwin, gboolean save_as)
{
	gboolean retval;
	DEBUG_MSG("project_save, bfwin=%p, save_as=%d\n", bfwin, save_as);
	if (!bfwin->project) {
		/* there is no project yet, we have to create one */
		DEBUG_MSG("project_save, bfwin=%p does not have a project yet, create one\n", bfwin);
		bfwin->project = create_new_project(bfwin);
		bmark_reload(bfwin);
	}
	DEBUG_MSG("project_save, project=%p, num files was %d\n", bfwin->project,
			  g_list_length(bfwin->project->files));
	update_project_filelist(bfwin, bfwin->project);

	bfwin->project->session->searchlist = limit_stringlist(bfwin->project->session->searchlist, 10, TRUE);
	bfwin->project->session->replacelist = limit_stringlist(bfwin->project->session->replacelist, 10, TRUE);
	bfwin->project->session->urllist = limit_stringlist(bfwin->project->session->urllist, 10, TRUE);
	bmark_store_all(bfwin);

	if (save_as || bfwin->project->uri == NULL) {
		gint suflen, filen;
		GtkWidget *dialog;
		GFile *newuri = NULL;
		dialog =
			file_chooser_dialog(bfwin, _("Enter Bluefish project filename"), GTK_FILE_CHOOSER_ACTION_SAVE,
								bfwin->project->name, TRUE, FALSE, "bfproject", FALSE);
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
			gchar *filename;
			newuri = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
			filename = g_file_get_parse_name(newuri);
			suflen = strlen(main_v->props.project_suffix);
			filen = strlen(filename);
			if (filen < suflen || strcmp(&filename[filen - suflen], main_v->props.project_suffix) != 0) {
				GFile *tmp2;
				gchar *tmp = g_strconcat(filename, main_v->props.project_suffix, NULL);
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
			exists = g_file_query_exists(newuri, NULL);
			if (exists) {
				gchar *tmpstr, *filename;
				gint retval;
				const gchar *buttons[] = { _("_Cancel"), _("_Overwrite"), NULL };
				filename = g_file_get_parse_name(newuri);
				tmpstr = g_strdup_printf(_("A file named \"%s\" already exists."), filename);
				retval = message_dialog_new_multi(bfwin->main_window,
												  GTK_MESSAGE_WARNING,
												  buttons,
												  tmpstr, _("Do you want to replace the existing file?"));
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
	DEBUG_MSG("project_save, retval=%d\n", retval);
	return retval;
}

void
project_open_from_file(Tbfwin * bfwin, GFile * fromuri)
{
	Tbfwin *prwin;
	Tproject *prj;
	gboolean retval;
	GList *tmplist;
	gchar *curi;

	/* first we test if the project is already open */
	prwin = project_is_open(fromuri);
	if (prwin != NULL) {
		DEBUG_MSG("project_open_from_file, project is open in bfwin=%p\n", prwin);
		gtk_window_present(GTK_WINDOW(prwin->main_window));
		return;
	}

	prj = g_new0(Tproject, 1);
	prj->session = g_new0(Tsessionvars, 1);
	DEBUG_MSG("project_open_from_file, project=%p, session=%p\n", prj, prj->session);
	project_setup_initial_session(prj->session, TRUE);
	prj->bmarkdata = bookmark_data_new();
	retval = rcfile_parse_project(prj, fromuri);
	if (!retval) {
		DEBUG_MSG("project_open_from_file, failed parsing the project\n");
		project_destroy(prj);
		/* TODO: use project_destroy() 
		   bookmark_data_cleanup(prj->bmarkdata);
		   g_free(prj->session);
		   g_free(prj); */
		return;
	}
	curi = g_file_get_uri(fromuri);
	main_v->globses.recent_projects =
				add_to_history_stringlist(main_v->globses.recent_projects, curi, FALSE, TRUE);
	bfwin_recent_menu_remove(bfwin, TRUE, curi);
	if (main_v->props.register_recent_mode != 0) {
		gtk_recent_manager_add_item(main_v->recentm, curi);
	}
	
	g_free(curi);
	prj->uri = fromuri;
	g_object_ref(fromuri);
	if (bfwin->project == NULL && test_only_empty_doc_left(bfwin->documentlist)) {
		/* we will use this Bluefish window to open the project */
		prwin = bfwin;
		prwin->project = prj;
		prwin->session = prj->session;
		DEBUG_MSG("project_open_from_file, project %p will be in existing prwin=%p\n", prj, bfwin);
		/* destroy the current empty document, it should use settings from the new session */
		if (bfwin->current_document)
			doc_destroy(bfwin->current_document, TRUE);
	} else {
		/* we will open a new Bluefish window for this project */
		DEBUG_MSG("project_open_from_file, we need a new window\n");
		prwin = bfwin_window_new_with_project(prj);
	}
	tmplist = g_list_last(prj->files);
	if (tmplist) {
		while (tmplist) {
			GFile *uri;
			if (strstr((gchar *) tmplist->data, "://") == NULL)
				uri = g_file_new_for_path((gchar *) tmplist->data);
			else
				uri = g_file_new_for_uri((gchar *) tmplist->data);
			doc_new_from_uri(prwin, uri, NULL, !(prj->files->prev == NULL), TRUE, -1, -1);
			g_object_unref(uri);
			tmplist = g_list_previous(tmplist);
		}
	} else {
		doc_new(bfwin, FALSE);
	}
	DEBUG_MSG("project_open_from_file, new window with files ready at prwin=%p\n", prwin);
	setup_bfwin_for_project(prwin);
	DEBUG_MSG("project_open_from_file, done\n");
}

static void
project_open_response_lcb(GtkDialog * dialog, gint response, Tbfwin * bfwin)
{
	if (response == GTK_RESPONSE_ACCEPT) {
		GFile *file;
		file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
		project_open_from_file(bfwin, file);
		g_object_unref(file);
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

void
project_open(Tbfwin * bfwin)
{
	GtkWidget *dialog;
	dialog =
		file_chooser_dialog(bfwin, _("Select Bluefish project filename"), GTK_FILE_CHOOSER_ACTION_OPEN, NULL,
							TRUE, FALSE, "bfproject", FALSE);
	g_signal_connect(dialog, "response", G_CALLBACK(project_open_response_lcb), bfwin);
	gtk_widget_show_all(dialog);
}

void
project_save_and_mark_closed(Tbfwin * bfwin)
{
	if (bfwin->project) {
		project_save(bfwin, FALSE);

		if (bfwin->project->uri) {
			gchar *curi = g_file_get_uri(bfwin->project->uri);
			bfwin_recent_menu_add(bfwin, TRUE, curi);
			g_free(curi);
		}

		bfwin->project->close = TRUE;
	}
}

gboolean
project_final_close(Tbfwin * bfwin, gboolean close_win)
{
	if (!bfwin->project)
		return TRUE;
/*	if (!close_win)
		bfwin_recent_menu_add(bfwin, bfwin->project->uri, NULL, TRUE);*/

	project_destroy(bfwin->project);
	/* we should only set the window for nonproject if the window will keep alive */
	if (!close_win) {
		setup_bfwin_for_nonproject(bfwin);
	}
	return TRUE;
}

void
project_save_and_close(Tbfwin * bfwin)
{
	/* if there are multiple windows we can simply close the window, else we have to convert it to a non-project window */
	if (main_v->bfwinlist && main_v->bfwinlist->next) {
		bfwin_delete_event(NULL, NULL, bfwin);
	} else {
		project_save_and_mark_closed(bfwin);
		if (!bfwin->documentlist) {
			project_final_close(bfwin, FALSE);
		} else if (have_modified_documents(bfwin->documentlist)) {
			Tclose_mode retval = multiple_files_modified_dialog(bfwin);
			switch (retval) {
			case close_mode_cancel:
				return;
				break;
			case close_mode_per_file:
				if (choose_per_file(bfwin, FALSE))
					bfwin->project->close = TRUE;
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
}

typedef enum {
	name,
	projecteditor_entries_num
} Tprojecteditor_entries;

typedef struct {
	GtkWidget *win;
	Tbfwin *bfwin;
	Tproject *project;
	Tsessionprefs sprefs;
	GtkWidget *entries[projecteditor_entries_num];
	gboolean destroy_project_on_close;
} Tprojecteditor;

static void
project_edit_destroy_lcb(GtkWidget * widget, Tprojecteditor * pred)
{
	DEBUG_MSG("project_edit_destroy_lcb, called for pred=%p\n", pred);
/*	gtk_widget_destroy(pred->win);*/
	if (pred->destroy_project_on_close) {
		if (pred->bfwin) {
			setup_bfwin_for_nonproject(pred->bfwin);
		}
		project_destroy(pred->project);
		pred->project = NULL;
	}
	if (pred->project) {
		pred->project->editor = NULL;
	}
	g_free(pred);
}

static void
project_edit_cancel_clicked_lcb(GtkWidget * widget, Tprojecteditor * pred)
{

	gtk_widget_destroy(pred->win);
}

static void
project_edit_ok_clicked_lcb(GtkWidget * widget, Tprojecteditor * pred)
{
	Tproject *prj = pred->project;

	pred->destroy_project_on_close = FALSE;

	gtk_widget_hide(pred->win);
	DEBUG_MSG("project_edit_ok_clicked_lcb, Tproject at %p, bfwin at %p\n", prj, pred->bfwin);
	string_apply(&prj->name, pred->entries[name]);
	sessionprefs_apply(&pred->sprefs, pred->project->session);
	if (pred->bfwin == NULL) {
		pred->bfwin = bfwin_window_new_with_project(pred->project);
		pred->bfwin->session = pred->project->session;
		setup_bfwin_for_project(pred->bfwin);
	} else {
		bfwin_set_title(pred->bfwin, pred->bfwin->current_document, 0);
	}
/* set_project_menu_widgets(pred->bfwin, TRUE);*/
	project_save(pred->bfwin, FALSE);
	gtk_widget_destroy(pred->win);
}

/*
if NULL is passed as bfwin, a new window will be created for this project once OK is clicked
*/
void
project_edit(Tbfwin * bfwin)
{
	GtkWidget *vbox, *but, *hbox, *label, *table;
	gchar *wintitle = NULL;
	Tprojecteditor *pred;

	if (bfwin && bfwin->project && bfwin->project->editor) {
		gtk_window_present(GTK_WINDOW(((Tprojecteditor *) bfwin->project->editor)->win));
		return;
	}
	pred = g_new(Tprojecteditor, 1);
	if (!bfwin || !bfwin->project) {
		/* there is no project yet, we have to create one */
		DEBUG_MSG("project_edit, no project yet, create one\n");
		pred->project = create_new_project(bfwin);
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
	DEBUG_MSG("project_edit, Tproject at %p\n", pred->project);
	pred->bfwin = bfwin;
	pred->project->editor = pred;

	pred->win =
		window_full2(wintitle, GTK_WIN_POS_CENTER_ALWAYS, 5, G_CALLBACK(project_edit_destroy_lcb), pred, TRUE,
					 NULL);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(pred->win), vbox);

	if (wintitle) {
		g_free(wintitle);
	}

	label = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	{
		gchar *message;
		message =
			g_strdup_printf(ngettext
							("This project contains %d file", "This project contains %d files",
							 g_list_length(pred->project->files)), g_list_length(pred->project->files));
		gtk_label_set_markup(GTK_LABEL(label), message);
		g_free(message);
	}

	table = gtk_table_new(1, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 12);

	pred->entries[name] = dialog_entry_in_table(pred->project->name, table, 1, 2, 0, 1);
	gtk_widget_set_size_request(GTK_WIDGET(pred->entries[name]), 250, -1);
	dialog_mnemonic_label_in_table(_("Project _Name:"), pred->entries[name], table, 0, 1, 0, 1);

/*	pred->entries[template] = entry_with_text(pred->project->template, 255);
	bf_mnemonic_label_tad_with_alignment(_("_Template:"), pred->entries[template], 1, 0.5, table, 0, 1, 3, 4);
	but = file_but_new(pred->entries[template], 1, NULL);
	gtk_table_attach_defaults(GTK_TABLE(table), pred->entries[template], 2, 3, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(table), but, 3, 4, 3, 4);*/

	sessionprefs(_("<b>Project Defaults</b>"), &pred->sprefs, pred->project->session);
	gtk_box_pack_start(GTK_BOX(vbox), pred->sprefs.frame, FALSE, FALSE, 6);

#if GTK_CHECK_VERSION(3,0,0)
	hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
	hbox = gtk_hbutton_box_new();
#endif
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(hbox), 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	but = bf_stock_cancel_button(G_CALLBACK(project_edit_cancel_clicked_lcb), pred);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 0);
	if (pred->destroy_project_on_close == TRUE) {
		but = dialog_button_new_with_image(_("Create _Project"), GTK_STOCK_NEW, G_CALLBACK(project_edit_ok_clicked_lcb), pred, TRUE, TRUE);
	} else {
		but = dialog_button_new_with_image(NULL, GTK_STOCK_OK, G_CALLBACK(project_edit_ok_clicked_lcb), pred, TRUE, FALSE);
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

static void
project_create_destroy_lcb(GtkWidget * widget, Tpc * pc)
{
	g_free(pc);
}

static void
project_create_cancel_clicked_lcb(GtkWidget * widget, Tpc * pc)
{
	gtk_widget_destroy(pc->win);
}

static void
project_create_ok_clicked_lcb(GtkWidget * widget, Tpc * pc)
{
	gtk_widget_hide(pc->win);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pc->rad[0]))) {
		project_edit(pc->bfwin);
	} else {
		project_edit(NULL);
	}
	gtk_widget_destroy(pc->win);
}


void
project_create_gui(Tbfwin * bfwin)
{
	GtkWidget *vbox, *hbox, *but;
	Tpc *pc;

	pc = g_new(Tpc, 1);
	pc->bfwin = bfwin;
	pc->win =
		window_full2(_("Create project"), GTK_WIN_POS_NONE, 5, G_CALLBACK(project_create_destroy_lcb), pc,
					 TRUE, NULL);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(pc->win), vbox);
	pc->rad[0] =
		boxed_radiobut_with_value(_("Create project with currently opened documents"), TRUE, NULL, vbox);
	pc->rad[1] =
		boxed_radiobut_with_value(_("Create empty project"), FALSE, GTK_RADIO_BUTTON(pc->rad[0]), vbox);
#if GTK_CHECK_VERSION(3,0,0)
	hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
	hbox = gtk_hbutton_box_new();
#endif
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(hbox), 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	but = bf_stock_cancel_button(G_CALLBACK(project_create_cancel_clicked_lcb), pc);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, TRUE, 0);
	but = bf_stock_ok_button(G_CALLBACK(project_create_ok_clicked_lcb), pc);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, TRUE, 0);
	gtk_widget_show_all(pc->win);
}

void
project_new(Tbfwin * bfwin)
{
	if (bfwin->project)
		project_edit(NULL);
	else {
		if (test_only_empty_doc_left(bfwin->documentlist))
			project_edit(bfwin);
		else
			project_create_gui(bfwin);
	}
}
