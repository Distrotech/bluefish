/* Bluefish HTML Editor
 * menu.c - the home of the pulldowns
 *
 * Copyright (C) 1998-2002 Olivier Sessink, Chris Mazuc and Roland Steinbach
 * 
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
#include <gtk/gtk.h>
#include <stdlib.h> /* atoi */
#include <string.h> /* strchr() */



#include "bluefish.h"
#include "document.h"			/* file_open etc. */
#include "highlight.h" /* hl_update_current_document_cb */
#include "menu.h" /* my own .h file */
#include "undo_redo.h" /* undo_cb() redo_cb() etc. */
#include "snr2.h" /* search_cb, replace_cb */
#include "gui.h" /* go_to_line_win_cb */
#include "stringlist.h" 	/* free_stringlist() */
#include "bf_lib.h"  /* append_string_to_file() */
#include "gtk_easy.h" /* window_full, bf_stock_ok_button */

static GtkItemFactoryEntry menu_items[] = {
	{N_("/_File"), NULL, NULL, 0, "<Branch>"},
	{N_("/File/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/File/_New"), "F8", file_new_cb, 0, NULL},
	{N_("/File/_Open..."), "<control>O", file_open_cb, 0, NULL},
	{N_("/File/Open advanced..."), NULL, file_open_cb, 1, NULL},
	{N_("/File/Open recent"), NULL, NULL, 0, "<Branch>"},
	{N_("/File/Open recent/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/File/Revert to saved"), NULL, file_revert_to_saved_cb, 0, NULL},
	{N_("/File/Insert..."), NULL, file_insert_cb, 0, NULL},
	{N_("/File/sep2"), NULL, NULL, 0, "<Separator>"},
	{N_("/File/_Save"), "<control>S", file_save_cb, 0, NULL},
	{N_("/File/Save _as..."), NULL, file_save_as_cb, 0, NULL},
	{N_("/File/Move to..."), NULL, file_move_to_cb, 0, NULL},
	{N_("/File/Save all"), NULL, file_save_all_cb, 0, NULL},
	{N_("/File/sep3"), NULL, NULL, 0, "<Separator>"},
	{N_("/File/Close"), NULL, file_close_cb, 0, NULL},
	{N_("/File/Close all"), NULL, file_close_all_cb, 0, NULL},
	{N_("/File/sep5"), NULL, NULL, 0, "<Separator>"},
	{N_("/File/Quit"), "<control>Q", bluefish_exit_request, 0, NULL},
	{N_("/_Edit"), NULL, NULL, 0, "<Branch>"},
	{N_("/Edit/Tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Edit/Cut"), "<control>x", edit_cut_cb, 0, NULL},
	{N_("/Edit/Copy"), "<control>c", edit_copy_cb, 0, NULL},
	{N_("/Edit/Paste"), "<control>v", edit_paste_cb, 0, NULL},
	{N_("/Edit/Select all"), NULL, edit_select_all_cb, 0, NULL},
	{N_("/Edit/sep3"), NULL, NULL, 0, "<Separator>"},
	{N_("/Edit/Find..."), "F6", search_cb, 0, NULL},
	{N_("/Edit/New find..."), NULL, new_search_cb, 0, NULL},
	{N_("/Edit/Find again"), "<shift>F6", search_again_cb, 0, NULL},
	{N_("/Edit/Replace..."), "F7", replace_cb, 0, NULL},
	{N_("/Edit/New replace..."), NULL, new_replace_cb, 0, NULL},
	{N_("/Edit/Replace again"), "<shift>F7", replace_again_cb, 0, NULL},
	{N_("/Edit/sep4"), NULL, NULL, 0, "<Separator>"},
	{N_("/Edit/Undo"), "<control>z", undo_cb, 0, NULL},
	{N_("/Edit/Redo"), "<control>r", redo_cb, 0, NULL},
	{N_("/Edit/Undo all"), NULL, undo_all_cb, 0, NULL},
	{N_("/Edit/Redo all"), NULL, redo_all_cb, 0, NULL},
	{N_("/Edit/sep5"), NULL, NULL, 0, "<Separator>"},
	{N_("/Edit/Goto line"), NULL, go_to_line_win_cb, 0, NULL},
	{N_("/Edit/Goto selection"), NULL, go_to_line_from_selection_cb, 0, NULL},
	{N_("/_Tags"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/_Dialogs"), NULL, NULL, 0, "<Branch>"},
	{N_("/Dialogs/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/E_xternal"), NULL, NULL, 0, "<Branch>"},
	{N_("/External/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/External/Filters"), NULL, NULL, 0, "<Branch>"},
	{N_("/External/Filters/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/External/Commands"), NULL, NULL, 0, "<Branch>"},
	{N_("/External/Commands/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/External/sep1"), NULL, NULL, 0, "<Separator>"},
	{N_("/_Options"), NULL, NULL, 0, "<Branch>"},
	{N_("/Options/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Options/Current document"), NULL, NULL, 0, "<Branch>"},
	{N_("/Options/Current document/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Options/Current document/Highlight syntax"), NULL, doc_toggle_highlighting_cb, 0, "<ToggleItem>"},
	{N_("/Options/Current document/Update highlighting"), "F5", doc_update_highlighting, 0, NULL},
	{N_("/Options/View Main toolbar"), NULL, gui_toggle_hidewidget_cb, 0, "<ToggleItem>"},
	{N_("/Options/View HTML toolbar"), NULL, gui_toggle_hidewidget_cb, 1, "<ToggleItem>"},
	{N_("/Options/View Custom menu"), NULL, gui_toggle_hidewidget_cb, 2, "<ToggleItem>"}
};


/* 
 * menu factory crap, thanks to the gtk tutorial for this
 * both the 1.0 and the 1.2 code is directly from the tutorial
 */
void menu_create_main(GtkWidget *vbox)
{
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	gint nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
	accel_group = gtk_accel_group_new();
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);
#ifdef ENABLE_NLS
	gtk_item_factory_set_translate_func(item_factory, menu_translate, "<main>", NULL);
#endif
	gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);
	gtk_window_add_accel_group(GTK_WINDOW(main_v->main_window), accel_group);
	main_v->menubar = gtk_item_factory_get_widget(item_factory, "<main>");
	gtk_box_pack_start(GTK_BOX(vbox), main_v->menubar, FALSE, TRUE, 0);
	gtk_widget_show(main_v->menubar);

	setup_toggle_item(item_factory, "/Options/View Main toolbar", main_v->props.view_main_toolbar);
	setup_toggle_item(item_factory, "/Options/View HTML toolbar", main_v->props.view_html_toolbar);
	setup_toggle_item(item_factory, "/Options/View Custom menu", main_v->props.view_custom_menu);
/*	setup_toggle_item(item_factory, "/Options/View Filebrowser", main_v->props.v_filebrowser);
	setup_toggle_item(item_factory, "/View/Highlight syntax", main_v->props.cont_highlight_update);*/
}
/************************************************/
/* generic functions for dynamic created menu's */
/************************************************/

static GtkWidget *remove_menuitem_in_list_by_label(gchar *labelstring, GList **menuitemlist) {
	GList *tmplist;
	gpointer tmp;

	tmplist = g_list_first(*menuitemlist);
	while (tmplist) {
		DEBUG_MSG("remove_recent_entry, tmplist=%p, data=%p\n", tmplist, tmplist->data);
		DEBUG_MSG("remove_recent_entry, tmplist->data=%s\n",GTK_LABEL(GTK_BIN(tmplist->data)->child)->label);
		if(!strcmp(GTK_LABEL(GTK_BIN(tmplist->data)->child)->label, labelstring)) {
			tmp = tmplist->data;
			*menuitemlist = g_list_remove(*menuitemlist, tmplist->data);
			DEBUG_MSG("remove_recent_entry, returning %p\n", tmp);
			return tmp;
		}
		tmplist = g_list_next(tmplist);
	}
	return NULL;
}

/* the result of this function can be added to the menuitem-list */
static GtkWidget *create_menuitem(gchar *menubasepath, gchar *label, GCallback callback, gint menu_insert_offset) {
	GtkWidget *tmp, *menu;
	GtkItemFactory *factory;

	/* add it to main_v->menubar */
	factory = gtk_item_factory_from_widget(main_v->menubar);
	menu = gtk_item_factory_get_widget(factory, menubasepath);

	tmp = gtk_menu_item_new_with_label(label);
	g_signal_connect(G_OBJECT(tmp), "activate",
					G_CALLBACK(callback), (gpointer) GTK_LABEL(GTK_BIN(tmp)->child)->label);

	gtk_widget_show(tmp);
	gtk_menu_insert(GTK_MENU(menu), tmp, menu_insert_offset);
	return tmp;
}


/*******************************************************************/
/*               Open Recent menu handling                         */
/*******************************************************************/
/* the only required header */
static GtkWidget *create_recent_entry(char *filename);
/*******************************************************************/

static GtkWidget *remove_recent_entry(gchar *filename) {
	GList *tmplist;
	gpointer tmp;

	if(strcmp(filename, "last") ==0) {
		tmplist = g_list_first(main_v->recent_files);
		tmp = tmplist->data;
		DEBUG_MSG("remove_recent_entry, remove last entry\n");
		main_v->recent_files = g_list_remove(main_v->recent_files, tmplist->data);
		return tmp;
	}	else {
		return remove_menuitem_in_list_by_label(filename, &main_v->recent_files);
	}
}

/* open_recent_file
 * This function should be called when a menu from the Open Recent list
 * has been selected. */
static void open_recent_file_cb(GtkWidget *widget, gchar *filename) {
	DEBUG_MSG("open_recent_file_cb, started, filename is %s\n", filename);

	/* Now, let's check if that file still exists and is readable, before loading it */
	if (!file_exists_and_readable(filename)) {
		return;
	}

	/* Open the file that was asked */
	doc_new_with_file(filename, FALSE);
	DEBUG_MSG("open_recent_file_cb, document %s opened\n", filename);

	add_to_recent_list(filename, 0);
	return;
}

/* create_recent_entry
 * This function builds the gtkitemfactoryentry and inserts it at the
 * main_v->menubar. Furthermore, it returns a pointer to it, so that
 * this pointer can be added in the main_v->recent_files list */
static GtkWidget *create_recent_entry(gchar *filename) {
	GtkWidget *tmp;

	tmp = remove_recent_entry(filename);
	if (tmp) {
		gtk_widget_hide(tmp);
		gtk_widget_destroy(tmp);
	}
	return  create_menuitem(N_("<main>/File/Open recent")
		, filename, G_CALLBACK(open_recent_file_cb)
		, 1);
}


/* recent_menu_init()
 * Gets the list of documents from .bluefish/recentlist and inserts
 * it at the File-->Open Recent menu. If the file doesn't exist (probably
 * because this is the first time Bluefish is running) then a menu
 * item telling that no recent files exist will appear */

void recent_menu_init() {
	gchar *recentfile;
	GList *filenames=NULL, *tmp, *tmp2, *newlist=NULL;
	gint recent_file_count=0;

	DEBUG_MSG("open_recent_list, started\n");
	recentfile = g_strconcat(g_get_home_dir(), "/.bluefish/recentlist", NULL);
	filenames = get_stringlist(recentfile, filenames);
	if (!filenames) {
		filenames = add_to_stringlist(main_v->recent_files, _("(none)"));
	}

	tmp = g_list_last(filenames);
	while (tmp && recent_file_count <= main_v->props.max_recent_files) {
		gint already_found=0;
		tmp2 = g_list_first(newlist);
		while (tmp2) {
			if (strcmp(tmp->data, tmp2->data)==0) {
				already_found=1;
			}
			tmp2 = g_list_next(tmp2);
		}
		if (!already_found) {
			recent_file_count++;
			DEBUG_MSG("open_recent_list, added %s to top of list\n", (gchar *)tmp->data);
			newlist = g_list_prepend(newlist, tmp->data);
		}
		already_found = 0;
		tmp = g_list_previous(tmp);
	}

	tmp2 = g_list_first(newlist);
	while (tmp2) {
		main_v->recent_files  = g_list_append(main_v->recent_files, create_recent_entry(tmp2->data));
		tmp2 = g_list_next(tmp2);
	}

	put_stringlist(recentfile, newlist);
	g_list_free(newlist);
	free_stringlist(filenames);

	g_free(recentfile);
}

/* Add_to_recent_list
 * This should be called when a new file is opened, i.e. from
 * file_open_cb, it adds a new entry which also appears in the
 * menu bar, and (if nessecary) deletes the last entry */
void add_to_recent_list(gchar *filename, gint closed_file) {
	DEBUG_MSG("add_to_recent_list, started for %s\n", filename);
	if (closed_file) {
		GtkWidget *tmp;

		/* First of all, create the entry and insert it at the list*/
		main_v->recent_files = g_list_append(main_v->recent_files,
								create_recent_entry(filename));

		DEBUG_MSG("add_to_recent_list, inserted item in menu\n");
		if(g_list_length(main_v->recent_files) > main_v->props.max_recent_files) {
			tmp = remove_recent_entry("last");
			DEBUG_MSG("add_to_recent_list, list too long, entry %s to be deleted\n", GTK_LABEL(GTK_BIN(tmp)->child)->label);
			gtk_widget_hide(tmp);
			gtk_widget_destroy(tmp);
		}
/*		flush_queue();*/

	} else {
		gchar *tmpfilename, *recentfile;
		recentfile = g_strconcat(g_get_home_dir(), "/.bluefish/recentlist", NULL);
		/* save the new list */
		tmpfilename = g_strconcat(filename, "\n", NULL);
		DEBUG_MSG("add_to_recent_list, trying to append to %s\n", recentfile);
		append_string_to_file(recentfile, tmpfilename);
		g_free(recentfile);
		g_free(tmpfilename);
	}
}

/*****************/
/* Browsers!!    */
/*****************/

static void browser_lcb(GtkWidget *widget, gchar *name) {
	GList *tmplist = g_list_first(main_v->props.browsers);
	while (tmplist) {
		gchar **arr = tmplist->data;
		if (strcmp(name, arr[0])==0) {
			DEBUG_MSG("browser_lcb, should start %s now\n", arr[1]);
		}
		tmplist = g_list_next(tmplist);
	}
}

void browsers_menu_init() {
	GList *tmplist = g_list_first(main_v->props.browsers);
	while (tmplist) {
		gchar **arr = tmplist->data;
		/*  arr[0] = name
		 *  arr[1] = command
		 */
		if (count_array(arr)==2) {
			main_v->browsers = g_list_append(main_v->browsers
					, create_menuitem(N_("<main>/External")
						, arr[0], G_CALLBACK(browser_lcb), 6)
					);
		}
		tmplist = g_list_next(tmplist);
	}
}

/***************/
/* custom menu */
/***************/
#define MAX_TEXT_ENTRY 10
typedef struct {
	GtkWidget *dialog;
	GtkWidget *textentry[MAX_TEXT_ENTRY];
	gint type;
	gchar **array;
} Tcust_con_struc;

typedef struct {
	GtkItemFactoryEntry entry;
	gchar **array;
} Tcmenu_entry;

/*
Tcust_con_struc->array[0] = title / menupath
Tcust_con_struc->array[1] = type:
										0 = custom dialog
										1 = custom search and replace

** for dialog **
Tcust_con_struc->array[2] = formatstring before, containing %0, %1... that should be replaced by the 
				values from the dialog
Tcust_con_struc->array[3] = formatstring after
Tcust_con_struc->array[4] = number of variables from the dialog
Tcust_con_struc->array[5..] = the description of those variables

** for search and replace **
Tcust_con_struc->array[2] = search pattern, containing %0 etc.
Tcust_con_struc->array[3] = replace pattern, containing %0 etc.
Tcust_con_struc->array[4] = replace where:
										0 = from beginning
										1 = from cursor
										2 = selection (selection required)
										3 = all open documents
										4 = ask
Tcust_con_struc->array[5] = replace type:
										0 = normal
										1 = regular expression
Tcust_con_struc->array[6] = case sensitivity:
										0 = no
										1 = yes
Tcust_con_struc->array[7] = number of variables from the dialog
Tcust_con_struc->array[8..] = the description of those variables
*/


static gchar *replace_with(gchar numc, gchar **array) {
	gint numi, i;

	numi = ((int) numc) -48;
	for (i=0; array[i] != NULL; i++) {
		DEBUG_MSG("replace_with, i=%d, searching for numi=%d\n", i, numi);
		if (i == numi) {
			DEBUG_MSG("replace_with, returning %d= %s\n", i, array[i]);
			return array[i];
		}
	}
	return NULL;
}


static gchar *replace_string(gchar *in_string, gchar **array) {
	gchar *tmp3;
	gchar *found, *search_in, *freestr;
	gchar *newstring, *tmpnewstring, *begin, *replace_w;

	newstring = g_strdup("");
	freestr = search_in = begin = g_strdup(in_string);
	while ( (search_in) && (found = strchr(search_in, '%')) ) {
		DEBUG_MSG("replace_string, found=%s, search_in=%s\n", found, search_in);
		tmp3 = found;
		DEBUG_MSG("replace_string, char=%c\n", *tmp3);
		tmp3++;
		DEBUG_MSG("replace_string, char=%c\n", *tmp3);
		if ((replace_w = replace_with(*tmp3, array))) {
			*found = '\0';
			tmpnewstring = newstring;
			newstring = g_strconcat(tmpnewstring, begin, replace_w, NULL);
			DEBUG_MSG("replace_string, newstring=%s\n", newstring);
			g_free(tmpnewstring);
			begin = ++tmp3;
			search_in = tmp3;
		} else if (*tmp3 == '\0') {
			DEBUG_MSG("replace_string, *tmp3 = \\0, found end of string\n");
			search_in = NULL;
		} else {
			search_in = ++found;
			DEBUG_MSG("replace_string, search_in=%s\n", search_in);
		}
	}
	tmpnewstring = newstring;
	newstring = g_strconcat(tmpnewstring, begin,NULL);
	DEBUG_MSG("replace_string, newstring=%s\n", newstring);
	g_free(tmpnewstring);
	
	g_free(freestr);
	return newstring;
}

static void cust_con_struc_dialog_destroy_lcb(GtkWidget *widget, GdkEvent *event,  Tcust_con_struc *ccs) {
	window_destroy(ccs->dialog);
	g_free(ccs);
}

static void cust_con_struc_dialog_cancel_lcb(GtkWidget *widget, gpointer data) {
	cust_con_struc_dialog_destroy_lcb(NULL, NULL, data);
}

static void cust_con_struc_dialog_ok_lcb(GtkWidget *widget, Tcust_con_struc *ccs) {
	gchar **dialogarray;
	gint num_vars, i;

	DEBUG_MSG("cust_con_struc_dialog_ok_lcb, ccs at %p\n", ccs);
	DEBUG_MSG("cust_con_struc_dialog_ok_lcb, array at %p, &array[0]=%p\n", ccs->array, &ccs->array[0]);
	DEBUG_MSG("cust_con_struc_dialog_ok_lcb, array[0] at %p, *array=%p\n", ccs->array[0], *ccs->array);
	if (ccs->type == 0) {
		gchar *before=NULL, *after=NULL;
		num_vars = atoi(ccs->array[4]);
		DEBUG_MSG("cust_con_struc_dialog_ok_lcb, num_vars=%d, ccs->array[3]=%s\n", num_vars, ccs->array[3]);
		dialogarray = g_malloc((num_vars+1) * sizeof(char *));
		for (i=0; i<num_vars; i++) {
			dialogarray[i] = gtk_editable_get_chars(GTK_EDITABLE(ccs->textentry[i]), 0, -1);
			DEBUG_MSG("cust_con_struc_dialog_ok_lcb, i=%d, dialogarray[i]=%s\n", i, dialogarray[i]);
		}
		dialogarray[i] = NULL;
		DEBUG_MSG("cust_con_struc_dialog_ok_lcb, i=%d, dialogarray[i]=%p\n", i, dialogarray[i]);

		if (strlen(ccs->array[2])) {
			DEBUG_MSG("cust_con_struc_dialog_ok_lcb, ccs->array[2]=%s\n",ccs->array[2] );
			before = replace_string(ccs->array[2], dialogarray);
		}
		if (strlen(ccs->array[3])) {
			after = replace_string(ccs->array[3], dialogarray);
		}
		doc_insert_two_strings(main_v->current_document, before, after);
		g_strfreev(dialogarray);

		if (before) {
			g_free(before);
		}
		if (after) {
			g_free(after);
		}
	} else {
		gchar *search=NULL, *replace=NULL;
		num_vars = atoi(ccs->array[7]);
		dialogarray = g_malloc((num_vars+1) * sizeof(char *));
		for (i=0; i<num_vars; i++) {
			dialogarray[i] = gtk_editable_get_chars(GTK_EDITABLE(ccs->textentry[i]), 0, -1);
			DEBUG_MSG("cust_con_struc_dialog_ok_lcb, i=%d, dialogarray[i]=%s\n", i, dialogarray[i]);
		}
		dialogarray[i] = NULL;
		if (strlen(ccs->array[2])) {
			DEBUG_MSG("cust_con_struc_dialog_ok_lcb, ccs->array[2]=%s\n",ccs->array[2] );
			search = replace_string(ccs->array[2], dialogarray);
		}
		if (strlen(ccs->array[3])) {
			replace = replace_string(ccs->array[3], dialogarray);
		} else {
			replace = g_strdup("");
		}
		snr2_run_extern_replace(search, atoi(ccs->array[4]),
				atoi(ccs->array[5]), atoi(ccs->array[6]), replace);
		g_strfreev(dialogarray);
		if (search) {
			g_free(search);
		}
		if (replace) {
			g_free(replace);
		} 
	}
	cust_con_struc_dialog_cancel_lcb(NULL, ccs);
}

static void cust_con_struc_dialog(gchar **array, gint type) {
	Tcust_con_struc *ccs;
	GtkWidget *vbox, *hbox, *okb, *cancb;
	gint i, num_vars;

	ccs = g_malloc(sizeof(Tcust_con_struc));
	ccs->type = type;
	DEBUG_MSG("cust_con_struc_dialog_cb, ccs at %p\n", ccs);
	ccs->array = array;
	DEBUG_MSG("cust_con_struc_dialog_cb, array at %p, &array[0]=%p\n", ccs->array, &ccs->array[0]);
	DEBUG_MSG("cust_con_struc_dialog_cb, array[0] at %p, *array=%p\n", ccs->array[0], *ccs->array);
	ccs->dialog = window_full(ccs->array[0], GTK_WIN_POS_MOUSE,  
			5, G_CALLBACK(cust_con_struc_dialog_cancel_lcb), ccs);
	vbox = gtk_vbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(ccs->dialog), vbox);
	DEBUG_MSG("cust_con_struc_dialog_cb, ccs->array[0]=%s\n", ccs->array[0]);
	
	if (type == 0) {
		num_vars = atoi(ccs->array[4]);
	} else {
		num_vars = atoi(ccs->array[7]);
	}
	DEBUG_MSG("cust_con_struc_dialog_cb, num_vars=%d\n", num_vars);

	for (i=0; i<num_vars; i++) {
		hbox = gtk_hbox_new(FALSE, 0);
		if (type ==0) {
			gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(ccs->array[i+5]), TRUE, TRUE, 2);
		} else {
			gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(ccs->array[i+8]), TRUE, TRUE, 2);
		}
		ccs->textentry[i] = gtk_entry_new();
		DEBUG_MSG("cust_con_struc_dialog_cb, textentry[%d]=%p\n", i, ccs->textentry[i]);
		gtk_box_pack_start(GTK_BOX(hbox), ccs->textentry[i], TRUE, TRUE, 0);		
		gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	}

	hbox = gtk_hbox_new(TRUE, 0);
	okb = bf_stock_ok_button(G_CALLBACK(cust_con_struc_dialog_ok_lcb), ccs);
	cancb = bf_stock_cancel_button(G_CALLBACK(cust_con_struc_dialog_cancel_lcb), ccs);
	gtk_box_pack_start(GTK_BOX(hbox), okb, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), cancb, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

	gtk_widget_grab_focus(ccs->textentry[0]);
	gtk_widget_show_all(ccs->dialog);
}


static void cust_menu_lcb(GtkWidget * widget, gpointer data) {
	Tcmenu_entry *cmentry;

	cmentry = (Tcmenu_entry *) g_list_nth_data(main_v->cmenu_entries, GPOINTER_TO_INT(data));
	if (strcmp(cmentry->array[1], "0") ==0) {
		DEBUG_MSG("cust_menu_lcb, a custom dialog, array[4]=%s\n", cmentry->array[4]);
		if (atoi(cmentry->array[4]) > 0) {
		     cust_con_struc_dialog(cmentry->array, 0);
		} else {
		     doc_insert_two_strings(main_v->current_document, cmentry->array[2],cmentry->array[3]);
		}
	} else {
		DEBUG_MSG("cust_menu_lcb, a custom search and replace!, cmentry->array[7]=%s\n", cmentry->array[7]);
		if (strcmp(cmentry->array[4], "2")==0 && !doc_has_selection(main_v->current_document)) {
			error_dialog(_("Bluefish error"), _("This custom search and replace requires a selection"));
			return;
		}
		if (atoi(cmentry->array[7]) > 0) {
			cust_con_struc_dialog(cmentry->array, 1);
		} else {
		     snr2_run_extern_replace(cmentry->array[2], atoi(cmentry->array[4]),
							atoi(cmentry->array[5]), atoi(cmentry->array[6]), cmentry->array[3]);
		}
	}
}

static void create_parent_and_tearoff(gchar *menupath, GtkItemFactory *ifactory) {
	char *basepath;
	GtkWidget *widg=NULL;
	GtkItemFactoryEntry entry;

	basepath = g_strndup(menupath, (strlen(menupath) - strlen(strrchr(menupath, '/'))));
	DEBUG_MSG("create_parent_and_tearoff, basepath=%s\n", basepath);
	widg = gtk_item_factory_get_widget(ifactory, basepath);
	if (!widg) {
		create_parent_and_tearoff(basepath, ifactory);
		entry.path = g_strconcat(basepath, "/sep", NULL);
		entry.accelerator = NULL;
		entry.callback = NULL;
		entry.callback_action = 0;
		entry.item_type = "<Tearoff>";
		gtk_item_factory_create_item(ifactory, &entry, NULL, 2);
		g_free(entry.path);
	}
}


static void fill_cust_menubar() {
	GtkItemFactory *ifactory;
	gint count;
	gchar **splittedstring;
	GList *tmplist;
	Tcmenu_entry *cmentry;

	ifactory = gtk_item_factory_from_widget(main_v->cmenu);

	tmplist = g_list_first(main_v->cmenu_entries);
	while (tmplist) {
		cmentry = (Tcmenu_entry *) tmplist->data;
		gtk_item_factory_delete_entry(ifactory, &cmentry->entry);
		DEBUG_MSG("fill_cust_menubar, removed entry.path=%s\n", cmentry->entry.path);
		g_free(cmentry->entry.path);
		g_free(cmentry);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(main_v->cmenu_entries);
	main_v->cmenu_entries = NULL;

	count = 0;
	tmplist = g_list_first(main_v->props.cust_menu);
	while (tmplist) {
		gint count2;
		splittedstring = (gchar **) tmplist->data;
		count2 = count_array(splittedstring);
		
		/* this is for backwards compatibility with bluefish < 0.7 */
		if (count2 < 4) {
			tmplist = g_list_next(tmplist);
			continue;
		}
		DEBUG_MSG("fill_cust_menubar, splittedstring[1]='%s'\n", splittedstring[1]);

		if (count2 < 5) {
			DEBUG_MSG("fill_cust_menubar, array count < 4, this is invalid\n");
			tmplist = g_list_next(tmplist);
			continue;
		}
		if ((strcmp(splittedstring[1] , "1")==0) && (count2 < 8)) {
			DEBUG_MSG("fill_cust_menubar, type 1, array count < 8, this is invalid\n");
			tmplist = g_list_next(tmplist);
			continue;
		}
		
		cmentry = g_malloc0(sizeof(Tcmenu_entry));
		cmentry->entry.path = g_strdup(splittedstring[0]);
		DEBUG_MSG("fill_cust_menubar, entry.path=%s, count=%d\n", cmentry->entry.path, count);
		cmentry->entry.callback = cust_menu_lcb;
		cmentry->entry.callback_action = count;
		cmentry->array = splittedstring;
		create_parent_and_tearoff(cmentry->entry.path, ifactory);
		gtk_item_factory_create_item(ifactory, &cmentry->entry, GINT_TO_POINTER(count), 2);
		main_v->cmenu_entries = g_list_append(main_v->cmenu_entries, cmentry);
		count++;
		tmplist = g_list_next(tmplist);
	}
}

void make_cust_menubar(GtkWidget *cust_handle_box)
{
	static GtkItemFactoryEntry cust_menu[] = {
		{N_("/_Custom menu"), NULL, NULL, 0, "<Branch>"},
		{N_("/Custom menu/sep"), NULL, NULL, 0, "<Tearoff>"}/*,
		{N_("/Custom menu/Edit custom menu..."), NULL, cmenu_editor, 0, NULL},
		{N_("/Custom menu/Reset custom menu"), NULL, reset_cust_menu_list_cb, 0, NULL}*/
	};
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	gint nmenu_items = sizeof(cust_menu) / sizeof(cust_menu[0]);

	DEBUG_MSG("make_cust_menubar, started\n");

	/* this should only happen once !!!!!!!!!! */
	accel_group = gtk_accel_group_new();
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);
#ifdef ENABLE_NLS
	gtk_item_factory_set_translate_func(item_factory, menu_translate, "<main>", NULL);
#endif
	gtk_item_factory_create_items(item_factory, nmenu_items, cust_menu, NULL);
	gtk_window_add_accel_group(GTK_WINDOW(main_v->main_window), accel_group);

	main_v->cmenu = gtk_item_factory_get_widget(item_factory, "<main>");
	gtk_container_add(GTK_CONTAINER(cust_handle_box), main_v->cmenu);
	gtk_widget_show(main_v->cmenu);

	fill_cust_menubar();

	DEBUG_MSG("make_cust_menubar, finished\n");
}















