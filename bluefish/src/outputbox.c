/* Bluefish HTML Editor
 * outputbox.c - the output box
 *
 * Copyright (C) 2002-2006 Olivier Sessink
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
#include <pcre.h>
#include <stdlib.h>
#include <string.h> /* strlen() */
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#include "bluefish.h"
#include "outputbox.h" /* myself */
#include "bf_lib.h"
#include "document.h"
#include "file_dialogs.h"
#include "gtk_easy.h"
#include "gui.h" /* statusbar_message() */
#include "stringlist.h"
#include "pixmap.h"
#include "external_commands.h"

#define NUM_MATCH 30

typedef struct {
	gchar *command;
	gchar *pattern;
	gchar *docuri;
	gint file_subpat;
	gint line_subpat;
	gint output_subpat;
	gboolean show_all_output;
	
	pcre *pcre_c;
	pcre_extra *pcre_s;
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
	gint lineval=-1;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(ob->lstore),&iter,path);
	gtk_tree_model_get(GTK_TREE_MODEL(ob->lstore), &iter, 0,&file,1,&line, -1);
	DEBUG_MSG("ob_lview_row_activated_lcb, file=%s, line=%s\n",file,line);
	if (line && strlen(line)) {
		lineval = atoi(line);
	}
	if (file && strlen(file)) {
		doc_new_from_input(ob->bfwin, file, FALSE,FALSE, lineval);
	} else {
		doc_select_line(ob->bfwin->current_document, lineval, TRUE);
	}
	g_free(line);
	g_free(file);
}

static void output_box_close_clicked_lcb(GtkWidget *widget, Toutputbox *ob) {
	DEBUG_MSG("output_box_close_clicked_lcb for %p\n",ob);
	outputbox_cleanup(ob->bfwin);
}

static Toutputbox *init_output_box(Tbfwin *bfwin) {
	GtkTreeViewColumn *column;
	GtkWidget *scrolwin;
	GtkCellRenderer *renderer;
	GtkWidget *vbox2, *but, *image;
	Toutputbox *ob;
	
	ob = g_new0(Toutputbox,1);
	DEBUG_MSG("init_output_box, created %p\n",ob);
	bfwin->outputbox = ob;
	ob->bfwin = bfwin;
	ob->def = NULL;	

	ob->hbox = gtk_hbox_new(FALSE,0);
	gtk_paned_add2(GTK_PANED(bfwin->vpane), ob->hbox);
	gtk_paned_set_position(GTK_PANED(bfwin->vpane),(gint)(bfwin->vpane->allocation.height * 0.8));

	ob->lstore = gtk_list_store_new (3,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
	ob->lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ob->lstore));
	g_object_unref(ob->lstore);/* the view widget now holds the only reference, if the view is destroyed, the model will be destroyed */

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

	vbox2 = gtk_vbox_new(FALSE,0);
	but = gtk_button_new();
	image = new_pixmap(4);
	gtk_widget_show(image);
	gtk_container_add(GTK_CONTAINER(but), image);
	gtk_container_set_border_width(GTK_CONTAINER(but), 0);
	gtk_widget_set_size_request(but, 16,16);
	g_signal_connect(G_OBJECT(but), "clicked", G_CALLBACK(output_box_close_clicked_lcb), ob);
	gtk_box_pack_start(GTK_BOX(ob->hbox), vbox2, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), but, FALSE, FALSE, 0);

	gtk_widget_show_all(ob->hbox);
	return ob;
}

void fill_output_box(gpointer data, gchar *string) {
	GtkTreeIter iter;
	gboolean needscroll=FALSE;
	int ovector[30];
	int nummatches;
	Toutputbox *ob = data;
	
	nummatches = pcre_exec(ob->def->pcre_c, ob->def->pcre_s,string, strlen(string), 0,0, ovector, 30);
	if (nummatches > 0) {
		/* we have a valid line */
		const char *filename,*line,*output;
		filename=line=output=NULL;
		gtk_list_store_append(GTK_LIST_STORE(ob->lstore), &iter);
		needscroll=TRUE;
		if (ob->def->file_subpat >= 0 && ovector[ob->def->file_subpat*2] >=0) {
			pcre_get_substring(string,ovector,nummatches,ob->def->file_subpat,&filename);
/*			DEBUG_MSG("fill_output_box, filename from %d to %d\n", ob->def->pmatch[ob->def->file_subpat].rm_so ,ob->def->pmatch[ob->def->file_subpat].rm_eo);*/
		}
		if (ob->def->line_subpat >= 0&& ovector[ob->def->line_subpat*2] >=0) {
			pcre_get_substring(string,ovector,nummatches,ob->def->line_subpat,&line);
/*			DEBUG_MSG("fill_output_box, line from %d to %d\n", ob->def->pmatch[ob->def->line_subpat].rm_so ,ob->def->pmatch[ob->def->line_subpat].rm_eo);*/
		}
		if (ob->def->output_subpat >= 0&& ovector[ob->def->output_subpat*2] >=0) {
			pcre_get_substring(string,ovector,nummatches,ob->def->output_subpat,&output);
/*			DEBUG_MSG("fill_output_box, output from %d to %d\n", ob->def->pmatch[ob->def->output_subpat].rm_so ,ob->def->pmatch[ob->def->output_subpat].rm_eo);*/
		}
		DEBUG_MSG("fill_output_box, filename=%s, line=%s, output=%s\n",filename,line,output);
		if (filename) {
			gchar *addtolist;
			if (filename[0] == '/') {
				addtolist = gnome_vfs_make_uri_from_input(filename);
			} else if (strchr(filename,':')==NULL) {
				addtolist = gnome_vfs_uri_make_full_from_relative(ob->def->docuri,filename);
			} else {
				addtolist = gnome_vfs_make_uri_canonical_strip_fragment(filename);
			}
			gtk_list_store_set(GTK_LIST_STORE(ob->lstore), &iter,0,addtolist,-1);
			g_free((gchar *)filename);
			g_free(addtolist);
		}
		if (line) {
			gtk_list_store_set(GTK_LIST_STORE(ob->lstore), &iter,1,line, -1);
			g_free((gchar *)line);
		}
		if (output) {
			gtk_list_store_set(GTK_LIST_STORE(ob->lstore), &iter,2,output, -1);
			g_free((gchar *)output);
		}
	} else if (ob->def->show_all_output) {
		needscroll=TRUE;
		gtk_list_store_append(GTK_LIST_STORE(ob->lstore), &iter);
		gtk_list_store_set(GTK_LIST_STORE(ob->lstore), &iter,2,string, -1);
	}
	if (needscroll) {
		GtkAdjustment* vadj;
		
		vadj = gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(ob->lview));
		gtk_adjustment_set_value(vadj, vadj->upper);		
		
	}
}

static void outputbox_def_cleanup(Toutputbox *ob, gboolean do_shutdown) {
	DEBUG_MSG("outputbox_def_cleanup, started\n");
	g_free(ob->def->pattern);
	pcre_free(ob->def->pcre_c);
	pcre_free(ob->def->pcre_s);
	g_free(ob->def->command);
	g_free(ob->def->docuri);
	g_free(ob->def);
	ob->def = NULL;
	DEBUG_MSG("outputbox_def_cleanup, finished\n");
}

void outputbox_cleanup(Tbfwin *bfwin) {
	DEBUG_MSG("outputbox_cleanup, called for bfwin %p, outputbox=%p\n",bfwin,bfwin->outputbox);
	if (bfwin->outputbox) {
		if (OUTPUTBOX(bfwin->outputbox)->def) {
			outputbox_def_cleanup(OUTPUTBOX(bfwin->outputbox), TRUE);
		}
		gtk_widget_destroy(OUTPUTBOX(bfwin->outputbox)->hbox);
		g_free(bfwin->outputbox);
		bfwin->outputbox = NULL;
	}
}

void outputbox(Tbfwin *bfwin,gchar *pattern, gint file_subpat, gint line_subpat, gint output_subpat, gchar *command, gboolean show_all_output) {
	Toutputbox *ob;
	const char *errptr;
	gint erroffset;
	DEBUG_MSG("outputbox, bfwin->outputbox=%p\n",bfwin->outputbox);
	if (bfwin->outputbox) {
		ob = OUTPUTBOX(bfwin->outputbox);
	} else {
		ob = init_output_box(bfwin);
	}
	if (ob->def) {
		/* we have a command still running !!! we have to cancel that first! */
		outputbox_def_cleanup(ob, TRUE);
	}
	gtk_list_store_clear(GTK_LIST_STORE(ob->lstore));
	
	ob->def = g_new0(Toutput_def,1);
	ob->def->pattern = g_strdup(pattern);
	if (bfwin->current_document->uri) {
		ob->def->docuri = gnome_vfs_uri_to_string(bfwin->current_document->uri,GNOME_VFS_URI_HIDE_PASSWORD);
	}
	ob->def->file_subpat = file_subpat;
	ob->def->line_subpat = line_subpat;
	ob->def->output_subpat = output_subpat;
	
	
	ob->def->show_all_output = show_all_output;
	
	ob->def->pcre_c = pcre_compile(ob->def->pattern, PCRE_UTF8,&errptr,&erroffset,NULL);
	if (ob->def->pcre_c == NULL) {
		gchar *tmpstr = g_strdup_printf(_("failed to compile outputbox pattern %s\n"),ob->def->pattern);
		statusbar_message(bfwin,tmpstr,4000);
		g_free(tmpstr);
		return;
	}
	ob->def->pcre_s = pcre_study(ob->def->pcre_c, 0,&errptr);
	
	outputbox_command(bfwin, command);
}
