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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*#define DEBUG*/

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
#include <strings.h>  	/* strcasecmp() */
#include "bluefish.h"
#include "bf_lib.h"

/*#include "coloursel.h"  color_but_new() */
#include "stringlist.h"
#include "gtk_easy.h"

#define STRING_MAX_SIZE 4096
#define MAX_ARRAY_LENGTH 20


/************************************************************************/


/************************************************************************/
#ifdef USE_ESTRL_DIALOG
typedef struct {
	GtkWidget *win;
	GtkWidget *ins_upd_entry[MAX_ARRAY_LENGTH];
	GtkWidget *clist;
	gint num_columns;
	gint last_row;
	GList **the_list;
	void (*function)();
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
				, gint column_num, gchar **column_titles, void (*post_dialog_func)()) {
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
	estrl->win = window_full(title, GTK_WIN_POS_MOUSE, 5, G_CALLBACK(estrl_destroy_lcb), estrl, TRUE);

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

	but = bf_generic_mnemonic_button(_(" Add "), G_CALLBACK(estrl_add_clicked_lcb), estrl);
	gtk_box_pack_start(GTK_BOX(vbox2),but , TRUE, TRUE, 0);
	but = bf_generic_mnemonic_button(_(" Update "), G_CALLBACK(estrl_update_clicked_lcb), estrl);
	gtk_box_pack_start(GTK_BOX(vbox2),but , TRUE, TRUE, 0);
	but = bf_generic_mnemonic_button(_(" Delete "), G_CALLBACK(estrl_delete_clicked_lcb), estrl);
	gtk_box_pack_start(GTK_BOX(vbox2),but , TRUE, TRUE, 0);
	but = bf_generic_mnemonic_button(_(" Up "), G_CALLBACK(estrl_up_clicked_lcb), estrl);
	gtk_box_pack_start(GTK_BOX(vbox2),but , TRUE, TRUE, 0);
	but = bf_generic_mnemonic_button(_(" Down "), G_CALLBACK(estrl_down_clicked_lcb), estrl);
	gtk_box_pack_start(GTK_BOX(vbox2),but , TRUE, TRUE, 0);
	but = bf_generic_mnemonic_button(_(" Sort "), G_CALLBACK(estrl_sort_clicked_lcb), estrl);
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
	    gtk_widget_set_size_request(GTK_WIDGET(estrl->win), wx, 400);
	    gtk_widget_set_uposition(estrl->win, 
				     (gdk_screen_width()-wx)/2,
				     (gdk_screen_height() - 400)/2);
	  }
	}
	gtk_widget_show_all(estrl->win);
}
#endif /* USE_ESTRL_DIALOG */


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

/**
 * count_array:
 * @array: #gchar** with the NULL terminated array to count
 *
 * counts the number of entries in a NULL terminated array
 *
 * Return value: #gint with number of entries
 */
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

/**
 * array_to_string:
 * @array: #gchar** with NULL terminated array
 * @delimiter: #gchar with the delimiter character
 *
 * Converts a NULL terminated array to a string with a delimiter
 * between the entries, and some characters backslash escaped
 * like \t, \n, \\ and the delimiter. In Bluefish the : is the most 
 * used delimiter
 *
 * Return value: #gchar* newly allocated string
 */
gchar *array_to_string(gchar **array) {
	if (array) {
		gchar **tmp, *escaped1, *finalstring;
		gint newsize=1;
		DEBUG_MSG("array_to_string, started\n");
		finalstring = g_malloc0(newsize);
		tmp = array;
		while(*tmp) {
			DEBUG_MSG("array_to_string, *tmp = %s\n", *tmp);
			escaped1 = escape_string(*tmp, TRUE);
			newsize += strlen(escaped1)+1;
			finalstring = g_realloc(finalstring, newsize);
			strcat(finalstring, escaped1);
			finalstring[newsize-2] = ':';
			finalstring[newsize-1] = '\0';
			g_free(escaped1);
			tmp++;
		}	
		DEBUG_MSG("array_to_string, finalstring = %s\n", finalstring);
		return finalstring;
	} else {
#ifdef DEBUG
		g_print("array_to_string, array=NULL !!!\n");
		exit(135);
#else
		return g_strdup("");
#endif
	}
}

#define ARRAYBLOCKSIZE 6
#define BUFBLOCKSIZE 60
/**
 * string_to_array:
 * @string: #gchar* with the string to convert
 * @delimiter: #gchar with the delimiter character
 *
 * breaks the string apart into a NULL terminated array
 * using the delimiter character. \t, \\ and \n are also unescaped
 * to their original characters
 *
 * Return value: #gchar** newly allocated NULL terminated array
 */
gchar **string_to_array(gchar *string) {
	gchar **array;
	gchar *tmpchar, *tmpchar2;
	gchar *newstring;
	gint count=0;
	gint newstringsize;
	gint arraycount=0, arraysize;

	newstringsize = BUFBLOCKSIZE;
	newstring = g_malloc(newstringsize * sizeof(char));
	
	arraysize = ARRAYBLOCKSIZE;
	array = g_malloc(arraysize * sizeof(char *));
	DEBUG_MSG("string_to_array, started, array=%p\n", array);	
	
	tmpchar = string;
	while (*tmpchar != '\0') {
		DEBUG_MSG("string_to_array, count=%d, newstring(%p)\n", count, newstring);
		if (*tmpchar == '\\') {
			tmpchar2 = tmpchar+1;
			switch (*tmpchar2) {
			case '\0':
				newstring[count] = '\\';
			break;
			case '\\':
				newstring[count] = '\\';
				tmpchar++;
			break;
			case 'n':
				newstring[count] = '\n';
				tmpchar++;
			break;
			case 't':
				newstring[count] = '\t';
				tmpchar++;
			break;
			case ':':
				newstring[count] = ':';
				tmpchar++;
			break;
			default:
				DEBUG_MSG("string_to_array, weird, an unescaped backslash ?\n");
				newstring[count] = '\\';
			break;
			}
		} else if (*tmpchar == ':') {
			newstring[count] = '\0';  /* end of the current newstring */
			DEBUG_MSG("string_to_array, newstring(%p)=%s\n", newstring, newstring);
			array[arraycount] = g_strdup(newstring);
			DEBUG_MSG("string_to_array, found delimiter, arraycount=%d, result(%p)=%s\n",arraycount, array[arraycount], array[arraycount]);
			arraycount++;
			if (arraycount == arraysize-2) { /* we need 1 spare entry in the array */
				arraysize += ARRAYBLOCKSIZE;  /* and arraysize starts at 1, arraycount at 0 */
				DEBUG_MSG("string_to_array, arraycount=%d, about to increase arraysize to %d, sizeof(array(%p))=%d\n", arraycount, arraysize, array, sizeof(&array));
				array = g_realloc(array, arraysize * sizeof(char *));
				DEBUG_MSG("string_to_array, arraysize=%d, array(%p), sizeof(array)=%d\n", arraysize, array, sizeof(&array));
			}
			count = -1;
		} else {
			newstring[count] = *tmpchar;
		}
		tmpchar++;
		count++;
		if (count == newstringsize-2) {
			newstringsize += BUFBLOCKSIZE;
			DEBUG_MSG("string_to_array, about to increase newstring(%p) to %d bytes\n", newstring, newstringsize);
			newstring = g_realloc(newstring, newstringsize * sizeof(char));
			DEBUG_MSG("string_to_array, newstringsize=%d, sizeof(newstring(%p))=%d\n", newstringsize, newstring, sizeof(newstring));
		}
	}
	
	if (count > 0) {
		newstring[count] = '\0';
		array[arraycount] = g_strdup(newstring);
		DEBUG_MSG("string_to_array, last array entry, arraycount=%d, result(%p)=%s\n",arraycount, array[arraycount],array[arraycount]);
	} else {
		array[arraycount] = NULL;
	}
	array[arraycount+1] = NULL; /* since we have 1 spare entry in the array this is safe to do*/
	DEBUG_MSG("string_to_array, returning %p\n", array);
	g_free(newstring);
	return array;
}		

/**
 * array_from_arglist:
 * @...: #gchar* with the first string, NULL terminated
 *
 * builds a NULL terminated array from the argumentlist to this function
 *
 * Return value: #gchar** with the array
 */
gchar **array_from_arglist(const gchar *string1, ...) {
	gint numargs=1;
	va_list args;
	gchar *s;
	gchar **retval, **index;

	va_start (args, string1);
	s = va_arg (args, gchar*);
	while (s) {
		numargs++;
		s = va_arg (args, gchar*);
	}
	va_end (args);
	DEBUG_MSG("array_from_arglist, numargs=%d\n", numargs);

	index = retval = g_new(gchar *, numargs + 1);
	*index = g_strdup(string1);

	va_start (args, string1);
	s = va_arg (args, gchar*);
	while (s) {
		index++;
		*index = g_strdup(s);
		s = va_arg (args, gchar*);
	}
	va_end (args);
	index++;
	*index = NULL;
	return retval;
}
/**
 * array_from_arglist:
 * @allocate_strings: #gboolean if the strings should be newly allocated
 * @...: #gchar* with the first string, NULL terminated
 *
 * builds a GList with strings (a stringlist), copied by reference or by content
 *
 * Return value: GList *
 */
GList *list_from_arglist(gboolean allocate_strings, ...) {
	GList *retval=NULL;
	va_list args;
	gchar *s;
	va_start(args, allocate_strings);
	s = va_arg(args, gchar*);
	while (s) {
		retval = g_list_append(retval, s);
		s = va_arg (args, gchar*);
	}
	va_end (args);
	return retval;
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
GList *get_list(const gchar * filename, GList * which_list, gboolean is_arraylist) {
	DEBUG_MSG("get_stringlist, started with filename=%s\n", filename);
	if (filename == NULL) {
		return NULL;
	}
	if (file_exists_and_readable(filename)) {
		FILE *fd;
		gchar *tmpbuf;
		DEBUG_MSG("get_stringlist, opening %s\n", filename);
		fd = fopen(filename, "r");
		if (fd == NULL) {
			return NULL;
		}
		tmpbuf = g_malloc(STRING_MAX_SIZE);
		while (fgets(tmpbuf, STRING_MAX_SIZE, fd) != NULL) {
			gchar *tempstr;
			tmpbuf = trunc_on_char(tmpbuf, '\n');
			tempstr = g_strdup(tmpbuf);
			if (is_arraylist) {
				gchar **temparr = string_to_array(tempstr);
				which_list = g_list_append(which_list, temparr);
				g_free(tempstr);
			} else {
				DEBUG_MSG("get_list, adding string \"%s\" to the stringlist=%p\n", tempstr, which_list);
				which_list = g_list_append(which_list, tempstr);
			}
		}
		fclose(fd);
		g_free(tmpbuf);
	}
	return which_list;
}
#ifdef __GNUC__
__inline__ 
#endif
GList *get_stringlist(const gchar * filename, GList * which_list) {
	return get_list(filename,which_list,FALSE);
}

/**
 * put_stringlist_limited:
 * @filename: #gchar* with the filename to store the list
 * @which_list: #GList* with the list to store
 * @maxentries: #gint only the LAST maxentries of the list will be stored
 *
 * stores the LAST maxentries entries of list which_list in file filename
 * if maxentries is 0 all entries will be stored
 *
 * Return value: #gboolean TRUE on success, FALSE on failure
 */
gboolean put_stringlist_limited(gchar * filename, GList * which_list, gint maxentries) {
	FILE *fd;
	GList *tmplist;

	DEBUG_MSG("put_stringlist_limited, started with filename=%s\n", filename);
	{
		gchar *backupfilename = g_strconcat(filename, main_v->props.backup_filestring,NULL);
		file_copy(filename, backupfilename);
		g_free(backupfilename);
	}

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

static void unlink_before(GList *tmplist) {
	GList *prev = tmplist->prev;
	if (prev) {
		prev->next = NULL;
	}
	tmplist->prev = NULL;
}

GList *limit_stringlist(GList *which_list, gint num_entries, gboolean keep_end) {
	GList *retlist, *freelist;
	if (keep_end) {
		gint len;
		freelist = g_list_first(which_list);
		len = g_list_length(freelist);
		if (len <= num_entries) return which_list;
		retlist = g_list_nth(freelist, len - num_entries);
		unlink_before(retlist);
	} else {
		retlist = g_list_first(which_list);
		freelist = g_list_nth(retlist, num_entries);
		if (freelist) unlink_before(freelist);
	}
	if (freelist) free_stringlist(freelist);
	return retlist;
}

/**
 * add_to_history_stringlist:
 * @which_list: #GList* the list to add to
 * @string: #const gchar* with the string to add
 * @recent_on_top: #gboolean, TRUE if the most recent entry is the one on top
 * @move_if_exists: #gboolean, if TRUE do move existing entries to the most recent part
 *
 * adds string to the stringlist which_list
 *
 * if string exists in this list already and move_if_exists is TRUE, 
 * it will be moved to the most recent part of the list (which is the 
 * end or the beginning based on the value of recent_on_top
 *
 * Return value: GList* with the modified list
 */
GList *add_to_history_stringlist(GList *which_list, const gchar *string, gboolean recent_on_top, gboolean move_if_exists) {
	if (string && strlen(string) ) {
		GList *tmplist = g_list_first(which_list);
		while (tmplist) {
			if (strcmp((gchar *) tmplist->data, string) == 0) {
				/* move this entry to the end */
				if (move_if_exists) {
					DEBUG_MSG("add_to_history_stringlist, entry %s exists, moving!\n", string);
					which_list = g_list_remove_link(which_list, tmplist);
					if (recent_on_top) {
						return g_list_concat(tmplist, which_list);
					} else {
						return g_list_concat(which_list, tmplist);
					}
				} else {
					return which_list;
				}
			}
			tmplist = g_list_next(tmplist);
		}
		/* if we arrive here the string was not yet in the list */
		DEBUG_MSG("add_to_history_stringlist, adding new entry %s\n",string);
		if (recent_on_top) {
			which_list = g_list_prepend(which_list, g_strdup(string));
		} else {
			which_list = g_list_append(which_list, g_strdup(string));
		}
	}
	return which_list;
}

/**
 * add_to_stringlist:
 * @which_list: a #GList * to add to
 * @string: a #const gchar * item you want to add to the list
 * 
 * this function will check if a string with same content exists already
 * and if not it will add it to the list, it returns the new list pointer
 * 
 * Return value: the new GList *
 **/
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
/**
 * stringlist_to_string:
 * @stringlist: a #GList * to convert
 * @delimiter: a #const gchar * item with the delimiter
 * 
 * this function will convert a stringlist (GList that contains 
 * only \0 terminated gchar* elements) to a string, putting the 
 * delimiter inbetween all elements;
 * 
 * Return value: the gchar *
 **/
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
/**
 * array_n_strings_identical:
 * @array1: #gchar**
 * @array2: #gchar**
 * @case_sensitive: #gboolean 
 * @testlevel: #gint
 *
 * tests the first testlevel strings in the arrays if they are identical
 * returns the first strcmp() value that is not 0, or 0 if all
 * strings up to testlevel are identical.
 *
 * if BOTH array end before testlevel is reached, 0 is returned
 * if ONE array ends before the other, -1 or 1 is returned
 *
 * Return value: #gint
 */
gint array_n_strings_identical(gchar **array1, gchar **array2, gboolean case_sensitive, gint testlevel) {
	gint i=0, res=0;
	while (i<testlevel && res==0) {
		/*  array1[i]==array2[i] will only happen when they are both NULL	*/
		if (array1[i] == NULL || array2[i] == NULL) {
			/* NULL - NULL = 0
			 * NULL - ptr = negative
			 * ptr - NULL = positive */
			return (gint)(array1[i] - array2[i]);
		} else if (case_sensitive) {
			res = strcmp(array1[i],array2[i]);
		} else /*if (!case_sensitive)*/ {
			res = strcasecmp(array1[i],array2[i]);
		}
		if (res !=0) return res;
		i++;
	}
	return res;
}
/**
 * arraylist_delete_identical:
 * @thelist: #GList*
 * @compare: #gchar**
 * @testlevel: #gint
 * @case_sensitive: #gboolean
 *
 * Deletes all arrays from 'arraylist' that are, up to testlevel, identical to array 'compare'
 *
 * Return value: the new list
 */
GList *arraylist_delete_identical(GList *thelist, gchar **compare, gint testlevel, gboolean case_sensitive) {
	GList *tmplist = g_list_first(thelist);
	while (tmplist) {
		GList *nextlist = g_list_next(tmplist);
		gchar **tmparr = tmplist->data;
		if (array_n_strings_identical(compare, tmparr, case_sensitive, testlevel)==0) {
			DEBUG_MSG("arraylist_delete_identical, %s and %s are identical, will delete %p from list\n",tmparr[0],compare[0], tmplist);
			thelist = g_list_delete_link(thelist, tmplist);
			DEBUG_MSG("arraylist_delete_identical, free array %p (%s)\n",tmparr,tmparr[0]);
			g_strfreev(tmparr);
		}
		tmplist = nextlist;
	}
	return thelist;
}

/**
 * arraylist_append_identical_from_list:
 * @thelist: #GList*
 * @source: #GList*
 * @compare: #gchar**
 * @testlevel: #gint
 * @case_sensitive: #gboolean
 *
 * compares every array in 'source' with 'compare', and if it is identical up to 'testlevel', it will
 * add the array to 'thelist'
 *
 * Return value: the new arraylist
 */
GList *arraylist_append_identical_from_list(GList *thelist, GList *source, gchar **compare, gint testlevel, gboolean case_sensitive) {
	GList *tmplist = g_list_first(source);
	while (tmplist) {
		gchar **tmparr = tmplist->data;
		if (array_n_strings_identical(compare, tmparr, case_sensitive, testlevel)==0) {
			thelist = g_list_append(thelist, duplicate_stringarray(tmparr));
		}
		tmplist = g_list_next(tmplist);
	}
	return thelist;
}

/**
 * arraylist_append_identical_from_file:
 * @thelist: #GList*
 * @sourcefilename: #const gchar*
 * @compare: #gchar**
 * @testlevel: #gint
 * @case_sensitive: #gboolean
 *
 * compares every array read from 'sourcefilename' with 'compare', and if it is identical up to 'testlevel', it will
 * add the array to 'thelist'
 *
 * Return value: the new arraylist
 */
GList *arraylist_append_identical_from_file(GList *thelist, const gchar *sourcefilename, gchar **compare, gint testlevel, gboolean case_sensitive) {
	GList *sourcelist = get_list(sourcefilename,NULL,TRUE);
	thelist = arraylist_append_identical_from_list(thelist, sourcelist, compare, testlevel, case_sensitive);
	free_arraylist(sourcelist);
	return thelist;
}

/**
 * arraylist_value_exists:
 * @arraylist: #GList*
 * @value: #gchar**
 * @testlevel: #gint
 * @case_sensitive: #gboolean whether the test should be case sensitive
 *
 * tests for the occurence of an array with identical content as value
 * in arraylist. It will only test the first testlevel strings in the array
 * so if you want the test to check for a 100% identical array that number 
 * should be high (9999 or so)
 *
 * Return value: #gboolean 
 */
gboolean arraylist_value_exists(GList *arraylist, gchar **value, gint testlevel, gboolean case_sensitive) {
	GList *tmplist = g_list_first(arraylist);
	while (tmplist) {
		gchar **tmparr = tmplist->data;
		if (array_n_strings_identical(value, tmparr, case_sensitive, testlevel)==0) {
			return TRUE;
		}
		tmplist = g_list_next(tmplist);
	}
	return FALSE;
}
/**
 * arraylist_load_new_identifiers_from_list:
 * @mylist: #GList*
 * @fromlist: #GList*
 * @uniquelevel: #gint
 *
 * compares every entry in fromlist 
 * with all entries in list mylist. Comparision is done uniquelevel deep
 * by function arraylist_value_exists()
 * those arrays that do _not_ match are appended to mylist which is returned
 * at the end of the function
 *
 * Return value: #GList*
 */
GList *arraylist_load_new_identifiers_from_list(GList *mylist, GList *deflist, gint uniquelevel) {
	GList *tmplist = g_list_first(deflist);
	while (tmplist) {
		gchar **tmparr = tmplist->data;
		if (count_array(tmparr) >= uniquelevel) {
			if (!arraylist_value_exists(mylist, tmparr, uniquelevel, TRUE)) {
				DEBUG_MSG("arraylist_load_new_identifiers, adding %s to thelist\n",tmparr[0]);
				mylist = g_list_append(mylist, duplicate_stringarray(tmparr));
			}
		}
		tmplist = g_list_next(tmplist);
	}
	return mylist;
}
/**
 * arraylist_load_new_identifiers_from_file:
 * @mylist: #GList*
 * @fromfilename: #gchar*
 * @uniquelevel: #gint
 *
 * loads an arraylist from fromfilename and compares every entry 
 * with all entries in list mylist. Comparision is done uniquelevel deep
 * by function arraylist_value_exists()
 * those arrays that do _not_ match are appended to mylist which is returned
 * at the end of the function
 *
 * Return value: #GList*
 */
GList *arraylist_load_new_identifiers_from_file(GList *mylist, const gchar *fromfilename, gint uniquelevel) {
	GList *deflist = get_list(fromfilename,NULL,TRUE);
	mylist = arraylist_load_new_identifiers_from_list(mylist, deflist, uniquelevel);
	free_arraylist(deflist);
	return mylist;	
}
