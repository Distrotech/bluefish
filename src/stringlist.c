/* Bluefish HTML Editor
 * stringlist.c - functions that deal with stringlists
 *
 * Copyright (C) 1999-2002 Olivier Sessink
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

/*
 * This module deals with the GUI dialog used in bluefish to manage
 * GLists in which each element is composed of several strings,
 * like the lists of syntax highlighting, external filters, external commands,
 * and lately doctypes. For example, the list of doctypes contains a string
 * for the DOCTYPE and a string for the FILENAME of that doctype.
 *
 * When the stringlist is edited, the old one is deleted and a new one is created,
 * so the order can be different from the original.
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>  	/* strspn() */
#include "bluefish.h"
#include "bf_lib.h"

/*#include "coloursel.h"  color_but_new() */
#include "stringlist.h"
#include "gtk_easy.h"

#define STRING_MAX_SIZE 1024
#define MAX_ARRAY_LENGTH 20


/************************************************************************/


/************************************************************************/

typedef struct {
	GtkWidget *win;
	GtkWidget *ins_upd_entry[MAX_ARRAY_LENGTH];
	GtkWidget *clist;
	gint num_columns;
	gint last_row;
	GList **the_list;
	void *(*function) ();
} Testrl;


/******************************************************************************
 * Check if the strings we want to add, already exist in the list
 */
static int is_duplicated(gchar **new_values, Testrl *estrl) {
    guint i, duplicated=0;
    gchar **tmparray, *tmpstring;
    gint row=0, retval=1;

    DEBUG_MSG("stringlist:is_duplicated(%p, %p)\n", new_values, estrl);
    retval = gtk_clist_get_text(GTK_CLIST(estrl->clist), row, 0, &tmpstring);
    while (retval == 1 && !duplicated) {
      tmparray = g_malloc(((estrl->num_columns ? estrl->num_columns : 1)+2)*sizeof(char *));
		for (i = 0; i < (estrl->num_columns ? estrl->num_columns  : 1); i++) {
			gtk_clist_get_text(GTK_CLIST(estrl->clist), row, i, &tmpstring);
			tmparray[i] = g_strdup(tmpstring);
		}
      tmparray[i] = NULL;

      duplicated = 1;
		if (estrl->num_columns == 0) {
			if (strcmp((gchar *) tmparray[0], new_values[0]) != 0) {
				duplicated = 0;
			}
		} else {
			for (i = 0; i < estrl->num_columns; i++) {
				DEBUG_MSG("old_values[%d]='%s'\n", i, tmparray[i]);
				DEBUG_MSG("new_values[%d]='%s'\n", i, new_values[i]);
				if (strcmp(tmparray[i], new_values[i]) != 0) {
					duplicated = 0;
				}
			}
		}
      row++;
      retval = gtk_clist_get_text(GTK_CLIST(estrl->clist), row, 0, &tmpstring);
    }
    DEBUG_MSG("duplicated=%d\n", duplicated);
    return duplicated;
}


static void estrl_destroy_lcb(GtkWidget * widget, GdkEvent *event, Testrl *estrl) {
	window_destroy(estrl->win);
	g_free(estrl);
}

static void estrl_cancel_clicked_lcb(GtkWidget * widget, Testrl *estrl) {
	estrl_destroy_lcb(NULL, NULL, estrl);
}

/***************************************************************************
 * ok button clicked
 *
 * Creates a new list with the elements of the dialog and replaces the old list
 * with the new one
 * At the end calls the post_func passed to the creator of the dialog and
 * destroys the dialog
*/
static void estrl_ok_clicked_lcb(GtkWidget * widget, Testrl *estrl) {

	guint i;
	gchar *tmpstring;
	gchar **tmparray;
	gint row=0, retval=1;
	GList *the_list=NULL, *orig_list;

	retval = gtk_clist_get_text(GTK_CLIST(estrl->clist), row, 0, &tmpstring);
	while (retval == 1) {
		tmparray = g_malloc(((estrl->num_columns ? estrl->num_columns : 1)+2)*sizeof(char *));
		for (i = 0; i < (estrl->num_columns ? estrl->num_columns  : 1); i++) {
			gtk_clist_get_text(GTK_CLIST(estrl->clist), row, i, &tmpstring);
			tmparray[i] = g_strdup(tmpstring);
		}
		tmparray[i] = NULL;
		if (estrl->num_columns == 0) {
			the_list = g_list_append(the_list, tmparray[0]);
			g_free(tmparray);
		} else {
			the_list = g_list_append(the_list, tmparray);
		}
		row++;
		retval = gtk_clist_get_text(GTK_CLIST(estrl->clist), row, 0, &tmpstring);
	}
	DEBUG_MSG("estrl_ok_clicked_lcb, going to add %p to the list\n", the_list);
	orig_list = *estrl->the_list;
	*estrl->the_list = the_list;

	if (estrl->num_columns) {
	        free_arraylist(orig_list);
	} else {
		free_stringlist(orig_list);
	}

	if (estrl->function)
		estrl->function();

	estrl_destroy_lcb(NULL, NULL, estrl);
}

static void estrl_select_row_lcb(GtkWidget * widget, gint row, gint column, GdkEventButton * event, Testrl *estrl){
	gchar *text;
	guint i;

	for (i = 0; i < ( estrl->num_columns ? estrl->num_columns : 1); i++) {
		gtk_clist_get_text(GTK_CLIST(estrl->clist), row, i, &text);
		gtk_entry_set_text(GTK_ENTRY(estrl->ins_upd_entry[i]), text);
	}
	estrl->last_row = row;
}

static void estrl_unselect_row_lcb(GtkWidget * widget, gint row, gint column, GdkEventButton * event, Testrl *estrl) {
	estrl->last_row = -1;
}

static void estrl_up_clicked_lcb(GtkWidget * widget, Testrl *estrl) {
	if (estrl->last_row >= 1) {
		gtk_clist_swap_rows(GTK_CLIST(estrl->clist), estrl->last_row, estrl->last_row - 1);
		estrl->last_row--;
	}
}

static void estrl_down_clicked_lcb(GtkWidget * widget, Testrl *estrl) {
	if (estrl->last_row < GTK_CLIST(estrl->clist)->rows - 1) {
		gtk_clist_swap_rows(GTK_CLIST(estrl->clist), estrl->last_row, estrl->last_row + 1);
		estrl->last_row++;
	}
}

/******************************************************************************
 * Avoid adding an empty element to the stringlist
 * Avoid adding a duplicated element to the stringlist
 */
static void estrl_add_clicked_lcb(GtkWidget * widget, Testrl *estrl) {
   gchar **tmplistentry;
	guint i, len;

    tmplistentry = g_malloc(((estrl->num_columns ? estrl->num_columns : 1)+2)*sizeof(char *));
    for (len = i = 0; i < (estrl->num_columns ? estrl->num_columns  : 1); i++) {
		tmplistentry[i] = gtk_editable_get_chars(GTK_EDITABLE(estrl->ins_upd_entry[i]),0,-1);
		len += strlen(tmplistentry[i]);
		DEBUG_MSG("tmplistentry[%d]='%s'\n", i, tmplistentry[i]);
    }
    tmplistentry[i]=NULL;
    if(len) {
		if(!is_duplicated(tmplistentry, estrl)) {
			gtk_clist_append(GTK_CLIST(estrl->clist), tmplistentry);
			/* does this introduce a memory leak?
			I'm not going to look at it since we need to get rid of the clist anyway 
			(gtk deprecated)
			*/
		} else {
			g_free(tmplistentry);
		}
	} else {
		g_free(tmplistentry);
	}
}

static void estrl_update_clicked_lcb(GtkWidget * widget, Testrl *estrl) {
	guint i;

	for (i = 0; i < (estrl->num_columns ? estrl->num_columns  : 1); i++) {
		gtk_clist_set_text(GTK_CLIST(estrl->clist)
					   , estrl->last_row, i, gtk_entry_get_text(GTK_ENTRY(estrl->ins_upd_entry[i])));
	}
}

static void estrl_delete_clicked_lcb(GtkWidget * widget, Testrl *estrl) {
	gtk_clist_remove(GTK_CLIST(estrl->clist), estrl->last_row);
}

static void estrl_sort_clicked_lcb(GtkWidget * widget, Testrl *estrl) {
	gtk_clist_set_auto_sort(GTK_CLIST(estrl->clist), 1);
}

/****************************************************************************
 * Creates the dialog to edit a stringlist
 *
 * Parameters:
 *  which_list      .- the stringlist which we want to edit
 *  title           .- the title of the dialog
 *  what_list       .- a bit more of information about the kind of strings
 *  column_num      .- number of strings in each element of the stringlist
 *                     equals to the number of columns in the dialog
 *                     if column_num=0, it is assumed column_num=1
 *  column_titles   .- title of the columns in the dialog
 *  post_dialog_func.- function to call after editing to process the modified
 *                     stringlist
*/
void estrl_dialog(GList **which_list, gchar *title, gint what_list
				, gint column_num, gchar **column_titles, void *post_dialog_func) {
	Testrl *estrl;
	GList *tmplist;
	GtkWidget *but_var=NULL, *hbox, *scrolwin, *vbox2, *vbox;
	GtkWidget *table2, *but;
	gint i;
	gchar *tmplistentry[1];


	estrl = g_malloc(sizeof(Testrl));
        if( column_num > MAX_ARRAY_LENGTH )
              column_num = MAX_ARRAY_LENGTH;
	estrl->num_columns = column_num;
	estrl->the_list = which_list;
	estrl->function = post_dialog_func;
	estrl->win = window_full(title, GTK_WIN_POS_MOUSE, 5, G_CALLBACK(estrl_destroy_lcb), estrl);

	vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(estrl->win), vbox);
	gtk_widget_realize(estrl->win);

	table2 = gtk_table_new(column_num ? column_num : 1, 3, 0);
	for (i = 0; i < (column_num ? column_num : 1); i++) {
		estrl->ins_upd_entry[i] = gtk_entry_new_with_max_length(950/(column_num ? column_num : 1));
		if (column_titles) {
			gtk_table_attach_defaults(GTK_TABLE(table2), gtk_label_new(column_titles[i]), 0, 1, i, i+1);
		}
		gtk_table_attach_defaults(GTK_TABLE(table2), estrl->ins_upd_entry[i], 1, 2, i, i+1);
	}
	gtk_box_pack_start(GTK_BOX(vbox), table2, FALSE, FALSE, 0);

	switch (what_list) {
	case 1:
		/* files */
		but_var = file_but_new(estrl->ins_upd_entry[0], estrl->win, 1);
		break;
	case 2:
		/* color */
#ifdef NOTYET
		but_var = color_but_new(estrl->ins_upd_entry[0], estrl->win);
#endif
		break;
	case 4:
		/* url */
		but_var = file_but_new(estrl->ins_upd_entry[0], estrl->win, 0);
		break;
	case 7:
		/* external filters and commands */

		break;
	case 8:
		/* syntax highlighting patterns */
#ifdef NOTYET
		but_var = color_but_new(estrl->ins_upd_entry[0], estrl->win);
#endif
		break;

#ifdef PARSEDTD
	case 9:
                /* doctypes */
		but_var = file_but_new(estrl->ins_upd_entry[1], estrl->win, 0);
		break;
#endif

	default:
		/* targets(5), fonts(3) meta_cb (6) */
		break;
	}
	if (but_var) {
		gtk_table_attach_defaults(GTK_TABLE(table2), but_var, 2, 3, 0, 1);
		GTK_WIDGET_SET_FLAGS(but_var, GTK_CAN_DEFAULT);
	}

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(hbox), scrolwin, TRUE, TRUE, 0);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 0);

	if (column_num == 0) {
		estrl->clist = gtk_clist_new(1);
	} else {
		if (column_titles != NULL) {
			estrl->clist = gtk_clist_new_with_titles(column_num, column_titles);
		} else {
			estrl->clist = gtk_clist_new(column_num);
		}
	}

	gtk_signal_connect(GTK_OBJECT(estrl->clist), "select_row", GTK_SIGNAL_FUNC(estrl_select_row_lcb), estrl);
	gtk_signal_connect(GTK_OBJECT(estrl->clist), "unselect_row", GTK_SIGNAL_FUNC(estrl_unselect_row_lcb), estrl);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolwin), estrl->clist);
	gtk_clist_set_reorderable(GTK_CLIST(estrl->clist), TRUE);
	gtk_clist_freeze(GTK_CLIST(estrl->clist));

	tmplist = g_list_first(*which_list);
	if (column_num == 0) {
		while (tmplist != NULL) {
			tmplistentry[0] = (gchar *) tmplist->data;
			gtk_clist_append(GTK_CLIST(estrl->clist), tmplistentry);
			tmplist = g_list_next(tmplist);
		}
	} else {
		while (tmplist != NULL) {
			gtk_clist_append(GTK_CLIST(estrl->clist), (gchar **) tmplist->data);
			tmplist = g_list_next(tmplist);
		}
	}
	gtk_clist_thaw(GTK_CLIST(estrl->clist));

	but = bf_stock_button(_(" Add "), G_CALLBACK(estrl_add_clicked_lcb), estrl);
	gtk_box_pack_start(GTK_BOX(vbox2),but , TRUE, TRUE, 0);
	but = bf_stock_button(_(" Update "), G_CALLBACK(estrl_update_clicked_lcb), estrl);
	gtk_box_pack_start(GTK_BOX(vbox2),but , TRUE, TRUE, 0);
	but = bf_stock_button(_(" Delete "), G_CALLBACK(estrl_delete_clicked_lcb), estrl);
	gtk_box_pack_start(GTK_BOX(vbox2),but , TRUE, TRUE, 0);
	but = bf_stock_button(_(" Up "), G_CALLBACK(estrl_up_clicked_lcb), estrl);
	gtk_box_pack_start(GTK_BOX(vbox2),but , TRUE, TRUE, 0);
	but = bf_stock_button(_(" Down "), G_CALLBACK(estrl_down_clicked_lcb), estrl);
	gtk_box_pack_start(GTK_BOX(vbox2),but , TRUE, TRUE, 0);
	but = bf_stock_button(_(" Sort "), G_CALLBACK(estrl_sort_clicked_lcb), estrl);
	gtk_box_pack_start(GTK_BOX(vbox2),but , TRUE, TRUE, 0);

	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 1);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	but = bf_stock_ok_button(G_CALLBACK(estrl_ok_clicked_lcb), estrl);
	gtk_box_pack_start(GTK_BOX(hbox), but, TRUE, TRUE, 0);
	gtk_window_set_default(GTK_WINDOW(estrl->win), but);

	but = bf_stock_cancel_button(G_CALLBACK(estrl_cancel_clicked_lcb), estrl);
	gtk_box_pack_start(GTK_BOX(hbox), but, TRUE, TRUE, 0);

	/* Adjust the size and position of the dialog */
	{
	  int wx=100, optx;
	  optx = gtk_clist_optimal_column_width(GTK_CLIST(estrl->clist),0);
	  if( optx < 100 )
	    optx = 100;
	  wx += optx + gtk_clist_optimal_column_width(GTK_CLIST(estrl->clist), 1);
	  if( wx < gdk_screen_width() ) {
	    gtk_clist_set_column_width(GTK_CLIST(estrl->clist), 0, optx );
	    gtk_widget_set_usize(GTK_WIDGET(estrl->win), wx, 400);
	    gtk_widget_set_uposition(estrl->win, 
				     (gdk_screen_width()-wx)/2,
				     (gdk_screen_height() - 400)/2);
	  }
	}
	gtk_widget_show_all(estrl->win);
}



/************************************************************************/

#ifdef DEBUG
void debug_array(gchar **array) {
	gint count=0;
	gchar **tmpchar=array;
	
	if (!tmpchar) {
		DEBUG_MSG("debug_array, no array!?!?\n");
	}

	while (*tmpchar != NULL) {
		count++;
		DEBUG_MSG("debug_array, tmpchar(%p), count=%d, contains(%p) %s\n", tmpchar, count, *tmpchar, *tmpchar);
		tmpchar++;
	}
}
#endif

gint count_array(gchar **array) {
	gint count=0;
	gchar **tmpchar=array;
	
	if (!tmpchar) {
		DEBUG_MSG("count_array, no array!?!?\n");
		return 0;
	}

	while (*tmpchar != NULL) {
		count++;
		tmpchar++;
	}
	return count;
}

GList *duplicate_stringlist(GList *list, gint dup_data) {
	GList *retlist=NULL;
	if (list) {
		GList *tmplist;
		tmplist = g_list_first(list);
		while (tmplist) {
			if (tmplist->data) {
				gchar *data;
				if (dup_data) {
					data = g_strdup((gchar *)tmplist->data);
				} else {
					data = (gchar *)tmplist->data;
				}
				retlist = g_list_append(retlist, data);
			}
			tmplist = g_list_next(tmplist);
		}
	}
	return retlist;
}

gint free_stringlist(GList * which_list)
{
	GList *tmplist;

	DEBUG_MSG("free_stringlist, started\n");

	tmplist = g_list_first(which_list);
	while (tmplist != NULL) {
		DEBUG_MSG("free_stringlist, tmplist->data(%p)\n", tmplist->data);
		g_free(tmplist->data);
		tmplist = g_list_next(tmplist);
	}
	DEBUG_MSG("free_stringlist, strings free-ed, about to free list itself\n");
	g_list_free(which_list);
	which_list = NULL;
	return 1;
}

gint free_arraylist(GList * which_list)
{
	GList *tmplist;
#ifdef DEBUGGING
	gchar **tmpchar;
#endif
	DEBUG_MSG("free_arraylist, started\n");

	tmplist = g_list_first(which_list);
	while (tmplist != NULL) {

#ifdef DEBUGGING
		DEBUG_MSG("free_arraylist, starting list %p with data %p\n", tmplist, tmplist->data);
		tmpchar = (gchar **)tmplist->data;
		while (*tmpchar) {
			DEBUG_MSG("free_arraylist, freeing %p\n", *tmpchar);
			DEBUG_MSG("free_arraylist, containing %s\n", *tmpchar);
			g_free(*tmpchar);
			tmpchar++;
		}
		DEBUG_MSG("free_arraylist, content freed\n");
		g_free((gchar **)tmplist->data);
		DEBUG_MSG("free_arraylist, array freed\n");
#else
		g_strfreev((gchar **)tmplist->data);
#endif
		tmplist = g_list_next(tmplist);
	}
	g_list_free(which_list);
	which_list = NULL;
	return 1;
}

gchar **duplicate_stringarray(gchar **array) {
	gchar **newchar;
	gint i;

	newchar = g_malloc0((count_array(array)+1)*sizeof(gchar *));
	for (i=0; array[i] != NULL ; i++) {
		newchar[i] = g_strdup(array[i]);
	}
	return newchar;
}

GList *duplicate_arraylist(GList *arraylist) {
	GList *tmplist;
	GList *newlist=NULL;

	tmplist = g_list_first(arraylist);
	while (tmplist != NULL) {
		newlist = g_list_append(newlist, duplicate_stringarray((gchar **)tmplist->data));
		tmplist = g_list_next(tmplist);
	}
	return newlist;
}


/*****************************************************************************
 * gets a stringlist from a file
 */
GList *get_stringlist(gchar * filename, GList * which_list)
{
	char *tempstr, *tmpbuf;
	FILE *fd;

	DEBUG_MSG("get_stringlist, started with filename=%s\n", filename);
	if (filename == NULL) {
		return NULL;
	}
	tmpbuf = g_malloc(STRING_MAX_SIZE);
	if (file_exists_and_readable(filename)) {
		DEBUG_MSG("get_stringlist, opening %s\n", filename);
		fd = fopen(filename, "r");
		if (fd == NULL) {
			return NULL;
		}
		while (fgets(tmpbuf, STRING_MAX_SIZE, fd) != NULL) {
			tmpbuf = trunc_on_char(tmpbuf, '\n');
			tempstr = g_strdup(tmpbuf);
			DEBUG_MSG("get_stringlist, adding string \"%s\" to the list=%p\n", tempstr, which_list);
			which_list = g_list_append(which_list, tempstr);
		}
		fclose(fd);
	}
	g_free(tmpbuf);
	return which_list;
}

/*****************************************************************************
 * stores a stringlist into a file
 */

gboolean put_stringlist_limited(gchar * filename, GList * which_list, gint maxentries) {
	FILE *fd;
	GList *tmplist;

	DEBUG_MSG("put_stringlist_limited, started with filename=%s\n", filename);

	DEBUG_MSG("put_stringlist_limited, opening %s for saving list(%p)\n", filename, which_list);
	fd = fopen(filename, "w");
	if (fd == NULL) {
		return FALSE;
	}
	if (maxentries > 0) {
		gint count;
		count = g_list_length(which_list) - maxentries;
		tmplist = g_list_nth(which_list, (count<0) ? 0 : count);
	} else {
		tmplist = g_list_first(which_list);
	}
	while (tmplist) {
		gchar *tmpstr = g_strndup((char *) tmplist->data, STRING_MAX_SIZE - 1);
		DEBUG_MSG("put_stringlist_limited, tmplist(%p), adding string(%p)=%s (strlen=%d)the file\n", tmplist, tmpstr, tmpstr, strlen(tmpstr));
		fputs(tmpstr, fd);
		g_free(tmpstr);
		fputs("\n", fd);
		tmplist = g_list_next(tmplist);
	}
	fclose(fd);
	DEBUG_MSG("put_stringlist_limited, finished, filedescriptor closed\n");
	return TRUE;
}

gboolean put_stringlist(gchar * filename, GList * which_list) {
	return put_stringlist_limited(filename,which_list, -1);
}

GList *remove_from_stringlist(GList *which_list, const gchar * string) {
	if (string && strlen(string) ) {
		GList *tmplist = g_list_first(which_list);
		while (tmplist) {
			if (strcmp((gchar *) tmplist->data, string) == 0) {
				DEBUG_MSG("remove_from_stringlist, removing '%s' (%p)\n", (gchar *)tmplist->data, tmplist->data);
				g_free(tmplist->data);
				return g_list_remove(which_list, tmplist->data);
			}
			tmplist = g_list_next(tmplist);
		}
	}
	return which_list;
}

GList *add_to_history_stringlist(GList *which_list, const gchar *string) {
	if (string && strlen(string) ) {
		GList *tmplist = g_list_first(which_list);
		while (tmplist) {
			if (strcmp((gchar *) tmplist->data, string) == 0) {
				/* move this entry to the end */
				DEBUG_MSG("add_to_history_stringlist, entry exists, moving to the end\n");
				which_list = g_list_remove_link(which_list, tmplist);
				return g_list_concat(which_list, tmplist);
			}
			tmplist = g_list_next(tmplist);
		}
		/* if we arrive here the string was not yet in the list */
		DEBUG_MSG("add_to_history_stringlist, appending new entry\n");
		which_list = g_list_append(which_list, g_strdup(string));
	}
	return which_list;
}

/* designed for adding strings to colorlist, urllist, fontlist and targetlist */
GList *add_to_stringlist(GList * which_list, const gchar * string) {
	if (string && strlen(string) ) {
		GList *tmplist = g_list_first(which_list);
		while (tmplist) {
			if (strcmp((gchar *) tmplist->data, string) == 0) {
				DEBUG_MSG("add_to_stringlist, strings are the same, don't add!!\n");
				return which_list;
			}
			tmplist = g_list_next(tmplist);
		}
		/* if we arrive here the string was not yet in the list */
		which_list = g_list_append(which_list, g_strdup(string));
	}
	return which_list;
}

gchar *stringlist_to_string(GList *stringlist, gchar *delimiter) {
	gchar *string, *tmp;
	GList *tmplist;
	string = g_strdup("");
	tmp = string;
	tmplist = g_list_first(stringlist);
	while (tmplist) {
		string = g_strconcat(tmp, (gchar *) tmplist->data, delimiter, NULL);
		g_free(tmp);
		tmp = string;
		tmplist = g_list_next(tmplist);
	}
	return string;
}
