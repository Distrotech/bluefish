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

static GtkItemFactoryEntry menu_items[] = {
	{N_("/_File"), NULL, NULL, 0, "<Branch>"},
	{N_("/File/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/File/_New"), "F8", file_new_cb, 0, NULL},
	{N_("/File/_Open..."), "<control>O", file_open_cb, 0, NULL},
	{N_("/File/Open advanced..."), NULL, file_open_cb, 1, NULL},
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
	{N_("/Edit/sep5"), NULL, NULL, 0, "<Separator>"}
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

