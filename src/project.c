/* Bluefish HTML Editor
 * project.c - project functionality
 *
 * Copyright (C) 2003 Olivier Sessink
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
/*#define DEBUG*/

#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

#include "bluefish.h"
#include "project.h"
#include "stringlist.h"
#include "gui.h"
#include "document.h"
#include "gtk_easy.h"
#include "rcfile.h"

Tbfwin *project_is_open(gchar *filename) {
	GList *tmplist;
	tmplist = g_list_first(main_v->bfwinlist);
	while(tmplist){
		if (BFWIN(tmplist->data)->project 
				&& BFWIN(tmplist->data)->project->filename
				&& strcmp(BFWIN(tmplist->data)->project->filename, filename)==0) {
			return BFWIN(tmplist->data);
		}
		tmplist = g_list_next(tmplist);
	}
	return NULL;
}

static void update_project_filelist(Tbfwin *bfwin, Tproject *prj) {
	GList *tmplist;
	
	free_stringlist(prj->files);
	prj->files = NULL;
	
	tmplist = g_list_first(bfwin->documentlist);
	while(tmplist){
		if (DOCUMENT(tmplist->data)->filename) {
			prj->files = g_list_append(prj->files, g_strdup(DOCUMENT(tmplist->data)->filename));
		}
		tmplist = g_list_next(tmplist);
	}
}

static Tproject *create_new_project(Tbfwin *bfwin) {
	Tproject *prj;
	prj = g_new0(Tproject,1);
	prj->name = g_strdup(_("New project"));
	update_project_filelist(bfwin,prj);
	prj->basedir = g_strdup("");
	prj->webdir = g_strdup("");
	return prj;
}

gboolean project_save(Tbfwin *bfwin, gboolean save_as) {
	gboolean retval;
	DEBUG_MSG("project_save, bfwin=%p, save_as=%d\n",bfwin,save_as);
	if (!bfwin->project) {
		/* there is no project yet, we have to create one */
		bfwin->project= create_new_project(bfwin);
	}
	if (save_as || bfwin->project->filename == NULL) {
		gchar *filename = return_file_w_title(NULL, _("Enter Bluefish project filename"));
		if (!filename) {
			return FALSE;
		}
		bfwin->project->filename = filename;
	}
	DEBUG_MSG("project_save, saving project %p to file %s\n",bfwin->project,bfwin->project->filename);
	retval = rcfile_save_project(bfwin->project, bfwin->project->filename);
	return retval;
}

void project_open_from_file(Tbfwin *bfwin, gchar *fromfilename) {
	Tbfwin *prwin;
	Tproject *prj;
	gboolean retval;

	/* first we test if the project is already open */
	prwin = project_is_open(fromfilename);
	if (prwin != NULL) {
		DEBUG_MSG("project_open_from_file, project is open in bfwin=%p\n",prwin);
		gtk_window_present(GTK_WINDOW(prwin->main_window));
		return;
	}

	prj = g_new(Tproject,1);
	prj->files = NULL;
	retval = rcfile_parse_project(prj, fromfilename);
	if (!retval) {
		DEBUG_MSG("project_open_from_file, failed parsing the project at file %s\n",fromfilename);
		g_free(prj);
		return;
	}
	prj->filename = g_strdup(fromfilename);
	if (bfwin->project == NULL && test_only_empty_doc_left(bfwin->documentlist)) {
		/* we will use this Bluefish window to open the project */
		prwin = bfwin;
		DEBUG_MSG("project_open_from_file, calling docs_new_from_files for existing bfwin=%p\n",prwin);
		docs_new_from_files(prwin, prj->files);
	} else {
		/* we will open a new Bluefish window for this project */
		prwin = gui_new_window(prj->files);
	}
	prwin->project = prj;
}

static void project_open(Tbfwin *bfwin) {
	/* first we ask for a filename */
	gchar *filename = return_file_w_title(NULL, _("Select Bluefish project filename"));
	if (filename) {
		DEBUG_MSG("project_open, for filename %s\n",filename);
		project_open_from_file(bfwin,filename);
		g_free(filename);
	} else {
		DEBUG_MSG("project_open, no filename.., returning\n");
	}
}

gboolean project_save_and_close(Tbfwin *bfwin) {
	if (project_save(bfwin, FALSE)) {
		DEBUG_MSG("project_save_and_close, save returned TRUE\n");
		file_close_all_cb(NULL,bfwin);
		if (test_only_empty_doc_left(bfwin->documentlist)) {
			DEBUG_MSG("project_save_and_close, all documents are closed\n");
			free_stringlist(bfwin->project->files);
			g_free(bfwin->project->filename);
			g_free(bfwin->project->name);
			g_free(bfwin->project->basedir);
			g_free(bfwin->project->webdir);
			g_free(bfwin->project);
			bfwin->project = NULL;
			DEBUG_MSG("project_save_and_close, returning TRUE\n");
			return TRUE;
		}
	}
	DEBUG_MSG("project_save_and_close, returning FALSE\n");
	return FALSE;
}

typedef enum {
	name,
	basedir,
	webdir
} Tprojecteditor_entries;

typedef struct {
	GtkWidget *win;
	Tbfwin *bfwin;
	GtkWidget *entries[3];
} Tprojecteditor;

static void project_edit_destroy_lcb(GtkWidget *widget, Tprojecteditor *pred) {
	DEBUG_MSG("project_edit_destroy_lcb, called for pred=%p\n",pred);
/*	gtk_widget_destroy(pred->win);*/
	pred->bfwin->project->editor = NULL;
	g_free(pred);
}

static void project_edit_cancel_clicked_lcb(GtkWidget *widget, Tprojecteditor *pred) {
	gtk_widget_destroy(pred->win);
}

static void project_edit_ok_clicked_lcb(GtkWidget *widget, Tprojecteditor *pred) {
	Tproject *prj = pred->bfwin->project;
	string_apply(&prj->name, pred->entries[name]);
	string_apply(&prj->basedir, pred->entries[basedir]);
	string_apply(&prj->webdir, pred->entries[webdir]);
	gtk_widget_destroy(pred->win);
}

void project_edit(Tbfwin *bfwin) {
	GtkWidget *vbox, *but, *hbox, *label;
	Tprojecteditor *pred;
	
	if (!bfwin->project) {
		/* there is no project yet, we have to create one */
		bfwin->project= create_new_project(bfwin);
	} else {
		if (bfwin->project->editor) {
			gtk_window_present(GTK_WINDOW(((Tprojecteditor *)bfwin->project->editor)->win));
			return;
		}
	}
	pred = g_new(Tprojecteditor,1);
	pred->bfwin = bfwin;
	bfwin->project->editor = pred;
	
	pred->win = window_full2(_("Project edit"), GTK_WIN_POS_NONE, 5
			, G_CALLBACK(project_edit_destroy_lcb), pred, TRUE, NULL);
	vbox = gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(pred->win),vbox);
	
	label = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	{
		gchar *message;
		message = g_strdup_printf(_("This project contains %d files"), g_list_length(bfwin->project->files));
		gtk_label_set_markup(GTK_LABEL(label), message);
	}

	
	pred->entries[name] = boxed_full_entry(_("Name"), bfwin->project->name,255, vbox);
	pred->entries[basedir] = boxed_full_entry(_("Basedir"), bfwin->project->basedir,255, vbox);
	pred->entries[webdir] = boxed_full_entry(_("Webdir"), bfwin->project->webdir,255, vbox);

	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	but = bf_stock_cancel_button(G_CALLBACK(project_edit_cancel_clicked_lcb), pred);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, TRUE, 0);
	but = bf_stock_ok_button(G_CALLBACK(project_edit_ok_clicked_lcb), pred);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, TRUE, 0);
	gtk_widget_show_all(pred->win);
}

void project_menu_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget) {
	DEBUG_MSG("project_menu_cb, bfwin=%p, callback_action=%d\n",bfwin,callback_action);
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
		project_save_and_close(bfwin);
	break;
	case 5:
		project_edit(bfwin);
	break;
	default:
		DEBUG_MSG("project_menu_cb, no such callback_action %d\n",callback_action);
		exit(54);
	break;
	}
}

