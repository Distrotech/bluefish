/* Bluefish HTML Editor
 * gui.c - the main GUI
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <gtk/gtk.h>


#include "bluefish.h"
#include "document.h" /* file_new_cb() */

void main_window_delete_lcd(GtkWidget *wid, gpointer data) {

}

void filebrowser_hide(gint first_time) {
	if (!first_time) {
		gtk_widget_ref(main_v->notebook);
		gtk_container_remove(GTK_CONTAINER(main_v->filebrowser.hpane), main_v->notebook);
		gtk_widget_destroy(main_v->filebrowser.hpane);
	}
	main_v->filebrowser.hpane = NULL;
	DEBUG_MSG("filebrowser_hide, middlebox=%p, notebook=%p\n", main_v->middlebox, main_v->notebook);
	gtk_box_pack_start(GTK_BOX(main_v->middlebox), main_v->notebook, TRUE, TRUE, 0);
	if (!first_time) {
		gtk_widget_unref(main_v->notebook);
	}
}

void filebrowser_show(gint first_time) {
	GtkWidget *noteb;

	if (!first_time) {
		gtk_widget_ref(main_v->notebook);
		gtk_container_remove(GTK_CONTAINER(main_v->middlebox), main_v->notebook);
	}

	main_v->filebrowser.hpane = gtk_hpaned_new();
	gtk_paned_set_gutter_size(GTK_PANED(main_v->filebrowser.hpane), 8);
	
	noteb = make_filelist();
	
	gtk_paned_add1(GTK_PANED(main_v->filebrowser.hpane), noteb);
	gtk_paned_add2(GTK_PANED(main_v->filebrowser.hpane), main_v->notebook);
	gtk_box_pack_start(GTK_BOX(main_v->middlebox), main_v->filebrowser.hpane, TRUE, TRUE, 0);

	DEBUG_MSG("filebrowser_show, middlebox=%p, notebook=%p\n", main_v->middlebox, main_v->notebook);

	gtk_widget_show(main_v->filebrowser.hpane);

	if (!first_time) {
		gtk_widget_unref(main_v->notebook);
	}
}

void gui_create_main() {
	main_v->main_window = window_full(CURRENT_VERSION_NAME, GTK_WIN_POS_CENTER, GTK_WINDOW_TOPLEVEL, 0, main_window_delete_lcd, NULL);
	gtk_window_set_wmclass(GTK_WINDOW(main_v->main_window), "Bluefish", "bluefish");

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(main_v->main_window), vbox);
	gtk_widget_show(vbox);

	/* first a menubar */
	
	/* then the toolbars */
	
	/* then the work area */
	main_v->middlebox = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), main_v->middlebox, TRUE, TRUE, 0);
	gtk_widget_show(main_v->middlebox);


	/* notebook with the text widget in there */
	main_v->notebook = gtk_notebook_new();
	notebook_set_tab_pos(main_v->props.cfg_tab_pos);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(main_v->notebook), TRUE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(main_v->notebook), TRUE);
	gtk_notebook_set_tab_border(GTK_NOTEBOOK(main_v->notebook), 1);

	/* add the first document to this notebook */
	file_new_cb(NULL, NULL);

	/* We have to know when the notebook changes */
	gtk_signal_connect(GTK_OBJECT(main_v->notebook), "button_release_event", notebook_changed, NULL);
	gtk_signal_connect_after(GTK_OBJECT(main_v->notebook), "key_press_event", notebook_changed, NULL);

	gtk_notebook_set_page(GTK_NOTEBOOK(main_v->notebook), 0);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(main_v->notebook), TRUE);
	gtk_widget_show(main_v->notebook);

	/* finally the statusbar */
	
	/* show all */
	
	gtk_widget_show_all(main_v->main_window);
}

void statusbar_message(gchar *message, gint time) {
	/* show the message on the statusbar */
}
