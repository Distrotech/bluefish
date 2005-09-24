/* Bluefish HTML Editor
 * outputbox.c - the output box
 *
 * Copyright (C) 2002-2005 Olivier Sessink
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
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#include "bluefish.h"
#include "outputbox.h" /* myself */
#include "bf_lib.h"
#include "document.h"
#include "file_dialogs.h"
#include "gtk_easy.h"
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
	regmatch_t pmatch[NUM_MATCH];
	regex_t preg;

/*	pid_t process;
	GIOChannel* channel;*/
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
	Toutputbox *ob = data;
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
			g_free(filename);
			g_free(addtolist);
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
}

static void outputbox_def_cleanup(Toutputbox *ob, gboolean do_shutdown) {
/*	GError *error=NULL;
	gint status;*/
	DEBUG_MSG("outputbox_def_cleanup, started\n");
/*	if (do_shutdown) {
		g_io_channel_shutdown(ob->def->channel,FALSE,&error);
		if (error) {
			DEBUG_MSG("outputbox_def_cleanup, error shutting down channel: %s\n", error->message);
			g_error_free(error);
		}
	}
	g_io_channel_close(ob->def->channel);
	kill(ob->def->process, SIGKILL);
	DEBUG_MSG("outputbox_def_cleanup, will now waitpid() for the child\n");
	waitpid(ob->def->process, &status, 0);
	g_io_channel_unref(ob->def->channel);*/
	g_free(ob->def->pattern);
	regfree(&ob->def->preg);
	g_free(ob->def->command);
	g_free(ob->def->docuri);
	g_free(ob->def);
	ob->def = NULL;
	DEBUG_MSG("outputbox_def_cleanup, finished\n");
}

/*static void parse_for_lines(Toutputbox *ob) {
	gint pos=0, size;
	gchar *newpos=NULL;
	if (!ob->def->buf) return;
	
	newpos = strchr(ob->def->buf+pos, '\n');
	while (newpos != NULL) {
		*newpos = '\0';
		fill_output_box(ob, ob->def->buf+pos);
		pos = (newpos - ob->def->buf + 1);
		newpos = strchr(ob->def->buf+pos, '\n');
	}
	size = strlen(ob->def->buf+pos)+1;
	DEBUG_MSG("parse_for_lines, remaining size=%d\n",size);
	if (size > 1) {
		memmove(ob->def->buf,ob->def->buf+pos,size);
	} else {
		g_free(ob->def->buf);
		ob->def->buf = NULL;
	}
	DEBUG_MSG("parse_for_lines, remaining is %s\n",ob->def->buf);
}*/

/* static gboolean io_watch_lcb(GIOChannel *source,GIOCondition condition,gpointer data) {
	Toutputbox *ob = data;
	DEBUG_MSG("io_watch_lcb, called with condition %d\n",condition);
	if (condition & G_IO_IN) {
		gchar *buf=NULL;
		gsize buflen=0,termpos=0;
		GError *error=NULL;
		GIOStatus status = g_io_channel_read_line(source,&buf,&buflen,&termpos,&error);
		while (status == G_IO_STATUS_NORMAL && buflen > 0) {
			if (termpos < buflen) buf[termpos] = '\0';
			fill_output_box(ob, buf);
			g_free(buf);
			status = g_io_channel_read_line(source,&buf,&buflen,&termpos,&error);
		}
	}
	if (condition & G_IO_OUT) {
		DEBUG_MSG("io_watch_lcb, condition %d G_IO_OUT not handled\n",condition);
	}
	if (condition & G_IO_PRI) {
		DEBUG_MSG("io_watch_lcb, condition %d G_IO_PRI not handled\n",condition);
	}
	if (condition & G_IO_ERR) {
		DEBUG_MSG("io_watch_lcb, condition %d G_IO_ERR not handled\n",condition);
	}
	if (condition & G_IO_HUP) {
		DEBUG_MSG("io_watch_lcb, condition %d G_IO_HUP\n",condition);
		/ * cleanup !!!!!!!!! * /
		outputbox_def_cleanup(ob, FALSE);
		return FALSE;
	}
	if (condition & G_IO_NVAL) {
		DEBUG_MSG("io_watch_lcb, condition %d G_IO_NVAL not handled\n",condition);
	}
	return TRUE;
}*/
/*
static void start_command(Toutputbox *ob, const gchar *command) {
	int fd[2];
	/ * 0 is for reading, 1 is for writing * /
	if (pipe(fd) != 0) {
		g_print("some error happened ??\n");
		return;
	}
	if ((ob->def->process = fork()) == 0) {
		close(fd[0]);
		dup2(fd[1],STDOUT_FILENO);
		close(fd[1]); / * dup makes a duplicate, so we can close the original * /
		dup2(STDOUT_FILENO,STDERR_FILENO);
		/ * close(fd[1]); dup makes a duplicate, so we can close the original * /
		execlp("/bin/sh", "sh", "-c", command, NULL);
		exit(127);
	}
	close(fd[1]);
	ob->def->channel = g_io_channel_unix_new(fd[0]);
	g_io_channel_set_flags(ob->def->channel,G_IO_FLAG_NONBLOCK,NULL);
	g_io_add_watch(ob->def->channel, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP,io_watch_lcb,ob);
}*/
/*
static void run_command(Toutputbox *ob) {
	Tconvert_table *table, *tmpt;
	gchar *command1;
	
	if (!ob->bfwin->current_document->uri) {
		/ * cannot (yet) use nameless files * /
		return;
	}
	
	table = tmpt = g_new(Tconvert_table, 2);
	tmpt->my_int = 's';
	if (gnome_vfs_uri_is_local(ob->bfwin->current_document->uri)) {
		tmpt->my_char = g_strdup(gnome_vfs_uri_get_path(ob->bfwin->current_document->uri));
	} else {
		tmpt->my_char = gnome_vfs_uri_to_string(ob->bfwin->current_document->uri,GNOME_VFS_URI_HIDE_PASSWORD);
	}
	tmpt++;
	tmpt->my_char = NULL;
	command1 = replace_string_printflike(ob->def->command, table);
	g_free(table);
	DEBUG_MSG("run_command, should run %s now\n", command1);
	if (gnome_vfs_uri_is_local(ob->bfwin->current_document->uri)) {
		gchar *tmpstring = g_path_get_dirname(gnome_vfs_uri_get_path(ob->bfwin->current_document->uri));
		chdir(tmpstring);
		g_free(tmpstring);
	}

	start_command(ob,command1);
	g_free(command1);
}*/

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
	ob->def = g_new0(Toutput_def,1);
	ob->def->pattern = g_strdup(pattern);
	if (bfwin->current_document->uri) {
		ob->def->docuri = gnome_vfs_uri_to_string(bfwin->current_document->uri,GNOME_VFS_URI_HIDE_PASSWORD);
	}
	ob->def->file_subpat = file_subpat;
	ob->def->line_subpat = line_subpat;
	ob->def->output_subpat = output_subpat;
	regcomp(&ob->def->preg,ob->def->pattern, REG_EXTENDED);
	gtk_list_store_clear(GTK_LIST_STORE(ob->lstore));
	ob->def->show_all_output = show_all_output;
	outputbox_command(bfwin, command);
/*
	run_command(ob);
*/
}
