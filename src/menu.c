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

#include "bluefish.h"
#include "document.h"			/* file_open etc. */
#include "menu.h" /* my own .h file */
#include "undo_redo.h" /* undo_cb() redo_cb() etc. */
#include "snr2.h" /* search_cb, replace_cb */
#include "gui.h" /* go_to_line_win_cb */
#include "stringlist.h" 	/* free_stringlist() */
#include "bf_lib.h"  /* append_string_to_file() */

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

/*	setup_toggle_item(item_factory, "/Options/View Main toolbar", main_v->props.v_main_tb);
	setup_toggle_item(item_factory, "/Options/View HTML toolbar", main_v->props.v_html_tb);
	setup_toggle_item(item_factory, "/Options/View Custom toolbar", main_v->props.v_custom_tb);
	setup_toggle_item(item_factory, "/Options/View Filebrowser", main_v->props.v_filebrowser);
	setup_toggle_item(item_factory, "/View/Highlight syntax", main_v->props.cont_highlight_update);*/
}


/*******************************************************************/
/*               Open Recent menu handling                         */
/*******************************************************************/
/* the only required header */
static GtkWidget *create_recent_entry(char *filename);
/*******************************************************************/


/* This function was nessecary I'm afraid, if you find a way
 * that it is not needed for open_recent_file_cb, please
 * send the patch. This also removes the last entry if
 * filename == 'last' */
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
		tmplist = g_list_first(main_v->recent_files);
		while (tmplist) {
			DEBUG_MSG("remove_recent_entry, tmplist=%p, data=%p\n", tmplist, tmplist->data);
			DEBUG_MSG("remove_recent_entry, tmplist->data=%s\n",GTK_LABEL(GTK_BIN(tmplist->data)->child)->label);
			if(!strcmp(GTK_LABEL(GTK_BIN(tmplist->data)->child)->label, filename)) {
				tmp = tmplist->data;
				main_v->recent_files = g_list_remove(main_v->recent_files, tmplist->data);
				DEBUG_MSG("remove_recent_entry, returning %p\n", tmp);
				return tmp;
			}
			tmplist = g_list_next(tmplist);
		}
	}
	return NULL;
}

/* open_recent_file
 * This function should be called when a menu from the Open Recent list
 * has been selected. */

static void open_recent_file_cb(gchar *data) {
	gchar *filename=data;

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
static GtkWidget *create_recent_entry(char *filename) {
	GtkWidget *tmp, *menu;
	GtkItemFactory *factory;

	tmp = remove_recent_entry(filename);
	if (tmp) {
		gtk_widget_hide(tmp);
		gtk_widget_destroy(tmp);
	}

	/* add it to main_v->menubar */
	factory = gtk_item_factory_from_widget(main_v->menubar);
	menu = gtk_item_factory_get_widget(factory, N_("<main>/File/Open recent"));

	tmp = gtk_menu_item_new_with_label(filename);
	gtk_signal_connect_object(GTK_OBJECT(tmp), "activate",
					GTK_SIGNAL_FUNC(open_recent_file_cb), (gpointer) GTK_LABEL(GTK_BIN(tmp)->child)->label);
/*	gtk_signal_connect_object(GTK_OBJECT(tmp), "activate",
					GTK_SIGNAL_FUNC(open_recent_file_cb), (gpointer) g_strdup("Hello World")); */

	gtk_widget_show(tmp);
	gtk_menu_insert(GTK_MENU(menu), tmp, 1);

/*	flush_queue();*/

	/* return it */
	return tmp;
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
		recentfile = g_strconcat(getenv("HOME"), "/.bluefish/recentlist", NULL);
		/* save the new list */
		tmpfilename = g_strconcat(filename, "\n", NULL);
		DEBUG_MSG("add_to_recent_list, trying to append to %s\n", recentfile);
		append_string_to_file(recentfile, tmpfilename);
		g_free(recentfile);
		g_free(tmpfilename);
	}
}

/*******************************************************************/
/*           End of Open Recent menu handling                      */
/*******************************************************************/
