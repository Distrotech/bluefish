/* Bluefish HTML Editor
 * outputbox.c - the output box
 *
 * Copyright (C) 2002 Olivier Sessink
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
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h> /* strlen() */

#include "bluefish.h"
#include "outputbox.h" /* myself */
#include "bf_lib.h"
#include "document.h"
#include "gtk_easy.h"
#include "stringlist.h"
#include "pixmap.h"

#define NUM_MATCH 30

typedef struct {
	gchar *command;
	gchar *pattern;
	gint file_subpat;
	gint line_subpat;
	gint output_subpat;
	gboolean show_all_output;
	regmatch_t pmatch[NUM_MATCH];
	regex_t preg;
} Toutput_def;

typedef struct {
	GtkListStore *lstore;
	GtkWidget *lview;
	GtkWidget *hbox;
	Toutput_def *def;
	Tbfwin *bfwin;
} Toutputbox;
#define OUTPUTBOX(var) ((Toutputbox *)(var))

static void ob_lview_row_activated_lcb(GtkTreeView *tree, GtkTreePath *path,GtkTreeViewColumn *column, Toutputbox *ob) {
	GtkTreeIter iter;
	gchar *file, *line;
	gint lineval;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(ob->lstore),&iter,path);
	gtk_tree_model_get(GTK_TREE_MODEL(ob->lstore), &iter, 0,&file,1,&line, -1);
	DEBUG_MSG("ob_lview_row_activated_lcb, file=%s, line=%s\n",file,line);
	if (file && strlen(file)) {
		doc_new_with_file(ob->bfwin,file,FALSE,FALSE);
	}
	if (line && strlen(line)) {
		lineval = atoi(line);
		flush_queue();
		doc_select_line(ob->bfwin->current_document, lineval, TRUE);
	}
	g_free(line);
	g_free(file);
}

static void output_box_close_clicked_lcb(GtkWidget *widget, Toutputbox *ob) {
	gtk_widget_hide(ob->hbox);
}

void init_output_box(Tbfwin *bfwin, GtkWidget *vbox) {
	Toutputbox *ob = g_new0(Toutputbox,1);
	bfwin->outputbox = ob;
	ob->bfwin = bfwin;

	ob->def = NULL;	
	ob->lstore = gtk_list_store_new (3,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
	ob->lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ob->lstore));
	{
		GtkTreeViewColumn *column;
		GtkWidget *scrolwin;
		GtkCellRenderer *renderer;
		
		ob->hbox = gtk_hbox_new(FALSE,0);
		gtk_box_pack_start(GTK_BOX(vbox), ob->hbox, FALSE, FALSE, 2);
		
		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes ("Filename", renderer,"text",0,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(ob->lview), column);
		column = gtk_tree_view_column_new_with_attributes ("Line", renderer,"text",1,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(ob->lview), column);
		column = gtk_tree_view_column_new_with_attributes ("Output", renderer,"text",2,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(ob->lview), column);

		scrolwin = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
		gtk_container_add(GTK_CONTAINER(scrolwin), ob->lview);
		gtk_widget_set_size_request(scrolwin, 150, 150);
		gtk_box_pack_start(GTK_BOX(ob->hbox), scrolwin, TRUE, TRUE, 0);
		g_signal_connect(G_OBJECT(ob->lview), "row-activated",G_CALLBACK(ob_lview_row_activated_lcb),ob);
	}
	{
		GtkWidget *vbox2 = gtk_vbox_new(FALSE,0);
		GtkWidget *but = gtk_button_new();
		GtkWidget *image = new_pixmap(4);
		gtk_widget_show(image);
		gtk_container_add(GTK_CONTAINER(but), image);
		gtk_container_set_border_width(GTK_CONTAINER(but), 0);
		gtk_widget_set_size_request(but, 16,16);
		g_signal_connect(G_OBJECT(but), "clicked", G_CALLBACK(output_box_close_clicked_lcb), ob);
		gtk_box_pack_start(GTK_BOX(ob->hbox), vbox2, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox2), but, FALSE, FALSE, 0);
	}
}

static void fill_output_box(Toutputbox *ob, GList *source) {
	GList *tmplist;
	
	gtk_list_store_clear(GTK_LIST_STORE(ob->lstore));
	
	tmplist = g_list_first(source);
	while (tmplist) {
		gchar *string = (gchar *)tmplist->data;
		GtkTreeIter iter;
		if (regexec(&ob->def->preg,string,NUM_MATCH,ob->def->pmatch,0)==0) {
			/* we have a valid line */
			gchar *filename,*line,*output;
			filename=line=output=NULL;
			gtk_list_store_append(GTK_LIST_STORE(ob->lstore), &iter);
			if (ob->def->file_subpat >= 0 && ob->def->pmatch[ob->def->file_subpat].rm_so >=0) {
				DEBUG_MSG("fill_output_box, filename from %d to %d\n", ob->def->pmatch[ob->def->file_subpat].rm_so ,ob->def->pmatch[ob->def->file_subpat].rm_eo);
				filename=g_strndup(&string[ob->def->pmatch[ob->def->file_subpat].rm_so], ob->def->pmatch[ob->def->file_subpat].rm_eo - ob->def->pmatch[ob->def->file_subpat].rm_so);
			}
			if (ob->def->line_subpat >= 0&& ob->def->pmatch[ob->def->line_subpat].rm_so >=0) {
				DEBUG_MSG("fill_output_box, line from %d to %d\n", ob->def->pmatch[ob->def->line_subpat].rm_so ,ob->def->pmatch[ob->def->line_subpat].rm_eo);
				line=g_strndup(&string[ob->def->pmatch[ob->def->line_subpat].rm_so], ob->def->pmatch[ob->def->line_subpat].rm_eo - ob->def->pmatch[ob->def->line_subpat].rm_so);
			}
			if (ob->def->output_subpat >= 0&& ob->def->pmatch[ob->def->output_subpat].rm_so >=0) {
				DEBUG_MSG("fill_output_box, output from %d to %d\n", ob->def->pmatch[ob->def->output_subpat].rm_so ,ob->def->pmatch[ob->def->output_subpat].rm_eo);
				output=g_strndup(&string[ob->def->pmatch[ob->def->output_subpat].rm_so], ob->def->pmatch[ob->def->output_subpat].rm_eo - ob->def->pmatch[ob->def->output_subpat].rm_so);
			}
			if (filename) {
				gchar *fullpath;
				/* create_full_path uses the current directory of no basedir is set */
				fullpath = create_full_path(filename, NULL);
				gtk_list_store_set(GTK_LIST_STORE(ob->lstore), &iter,0,fullpath,-1);
				g_free(filename);
				g_free(fullpath);
			}
			if (line) {
				gtk_list_store_set(GTK_LIST_STORE(ob->lstore), &iter,1,line, -1);
				g_free(line);
			}
			if (output) {
				gtk_list_store_set(GTK_LIST_STORE(ob->lstore), &iter,2,output, -1);
				g_free(output);
			}
		} else if (ob->def->show_all_output) {
			gtk_list_store_append(GTK_LIST_STORE(ob->lstore), &iter);
			gtk_list_store_set(GTK_LIST_STORE(ob->lstore), &iter,2,string, -1);
		}
		tmplist = g_list_next(tmplist);
	}
}

static GList *run_command(Toutputbox *ob) {
	GList *retlist;
	Tconvert_table *table, *tmpt;
	gchar *command1, *command2, *tmpfile;
	file_save_cb(NULL, ob->bfwin);
	if (!ob->bfwin->current_document->filename) {
		/* if the usder clicked cancel at file_save -> return */
		return NULL;
	}
	table = tmpt = g_new(Tconvert_table, 2);
	tmpt->my_int = 's';
	tmpt->my_char = ob->bfwin->current_document->filename;
	tmpt++;
	tmpt->my_char = NULL;
	command1 = replace_string_printflike(ob->def->command, table);
	g_free(table);
	tmpfile = create_secure_dir_return_filename();
	command2 = g_strconcat(command1, " > ", tmpfile, " 2>&1", NULL);
	DEBUG_MSG("run_command, should run %s now\n", command2);
	{
		gchar *tmpstring = g_path_get_dirname(ob->bfwin->current_document->filename);
		chdir(tmpstring);
		g_free(tmpstring);
	}
	system(command2);
	retlist = get_stringlist(tmpfile, NULL);
	remove_secure_dir_and_filename(tmpfile);
	g_free(command1);
	g_free(command2);
	g_free(tmpfile);
	return retlist;
}

void outputbox(Tbfwin *bfwin,gchar *pattern, gint file_subpat, gint line_subpat, gint output_subpat, gchar *command, gboolean show_all_output) {
	GList *olist;
	Toutputbox *ob = OUTPUTBOX(bfwin->outputbox);
	ob->def = g_new(Toutput_def,1);
	ob->def->pattern = pattern;
	ob->def->file_subpat = file_subpat;
	ob->def->line_subpat = line_subpat;
	ob->def->output_subpat = output_subpat;
	regcomp(&ob->def->preg,ob->def->pattern, REG_EXTENDED);
	ob->def->command = command;
	olist = run_command(ob);
	fill_output_box(ob,olist);
	gtk_widget_show_all(ob->hbox);
	free_stringlist(olist);
	g_free(ob->def);
	ob->def = NULL;
}
