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
#include "gui.h"
#include "document.h" /* file_new_cb() */
#include "gtk_easy.h"
#include "menu.h" /* menu_create_main() */

void notebook_changed(gint newpage)
{
	gint cur;

	if (newpage == -1) {
		/* This one is called when you click on the notebook
	   it _should_ be called also when you use the keys to change the page */
		cur = (gint) gtk_notebook_get_current_page(GTK_NOTEBOOK(main_v->notebook));
	} else {
		cur = newpage;
	}
	DEBUG_MSG("notebook_changed, main_v is at %p\n", main_v);

	if ((main_v->last_notebook_page == cur) 
		&& (main_v->current_document != NULL)
		&& (main_v->current_document == g_list_nth_data(main_v->documentlist, cur))) {
		DEBUG_MSG("notebook_changed, it didn't change to a new document (cur=%d, current_document=%p)\n", cur, main_v->current_document);
		return;
	} else {
		if (cur == -1) {
			DEBUG_MSG("notebook_changed, cur=-1, calling file_new_cb()\n");
			file_new_cb(NULL, NULL);
			main_v->last_notebook_page = 0;
		} else {
			DEBUG_MSG("notebook_changed, it did really change to a new document, cur=%d\n", cur);
			main_v->last_notebook_page = cur;
			main_v->current_document = g_list_nth_data(main_v->documentlist, cur);
#ifdef DEBUG
			DEBUG_MSG("notebook_changed, finished, main_v->current_document=%p\n", main_v->current_document);
			g_assert(main_v->current_document);
#endif
			doc_activate(main_v->current_document);
			DEBUG_MSG("notebook_changed, setting up toggle item\n");
/*			setup_toggle_item(gtk_item_factory_from_widget(main_v->menubar), N_("/View/Highlight syntax"),
							  main_v->current_document->highlightstate);*/
		}
	}
}

gboolean switch_to_document_by_index(gint index) {
	if (index >= 0) {
		gtk_notebook_set_page(GTK_NOTEBOOK(main_v->notebook), (index));
/*		notebook_changed();*/
		return TRUE;
	}
	return FALSE;
}

gboolean switch_to_document_by_pointer(Tdocument *document) {
	gint index;

	index = g_list_index(main_v->documentlist, document);
	return switch_to_document_by_index(index);
}

gboolean switch_to_document_by_filename(gchar *filename) {
	gint index;

	index = documentlist_return_index_from_filename(filename);
	return switch_to_document_by_index(index);
}


static void notebook_switch_page_lcb(GtkWidget *notebook,GtkNotebookPage *page,gint page_num,gpointer user_data) {
	DEBUG_MSG("notebook_switch_page_lcb, page=%d\n", page_num);
	notebook_changed(page_num);
}

static void main_window_delete_lcb(GtkWidget *wid, gpointer data) {
	bluefish_exit_request();
}

void filebrowser_hide(gint first_time) {
/*	if (!first_time) {
		gtk_widget_ref(main_v->notebook);
		gtk_container_remove(GTK_CONTAINER(main_v->filebrowser.hpane), main_v->notebook);
		gtk_widget_destroy(main_v->filebrowser.hpane);
	}*/
	main_v->hpane = NULL;

	gtk_box_pack_start(GTK_BOX(main_v->middlebox), main_v->notebook, TRUE, TRUE, 0);
/*	if (!first_time) {
		gtk_widget_unref(main_v->notebook);
	}*/
}

void filebrowser_show(gint first_time) {
/*	GtkWidget *noteb;

	if (!first_time) {
		gtk_widget_ref(main_v->notebook);
		gtk_container_remove(GTK_CONTAINER(main_v->middlebox), main_v->notebook);
	}*/

	main_v->hpane = gtk_hpaned_new();
	gtk_paned_set_gutter_size(GTK_PANED(main_v->hpane), 8);
/*	
	noteb = make_filelist();
	
	gtk_paned_add1(GTK_PANED(main_v->filebrowser.hpane), noteb);*/
	gtk_paned_add2(GTK_PANED(main_v->hpane), main_v->notebook);
	gtk_box_pack_start(GTK_BOX(main_v->middlebox), main_v->hpane, TRUE, TRUE, 0);

	gtk_widget_show(main_v->hpane);
/*
	if (!first_time) {
		gtk_widget_unref(main_v->notebook);
	}*/
}

void gui_notebook_bind_signals() {
	main_v->notebook_switch_signal = g_signal_connect_after(G_OBJECT(main_v->notebook),"switch-page",G_CALLBACK(notebook_switch_page_lcb), NULL);
}

void gui_notebook_unbind_signals() {
if (main_v->notebook_switch_signal != 0) {
		g_signal_handler_disconnect(G_OBJECT(main_v->notebook),main_v->notebook_switch_signal);
		main_v->notebook_switch_signal = 0;
	}
}

void gui_create_main(GList *filenames) {
	GtkWidget *vbox;
	main_v->main_window = window_full(CURRENT_VERSION_NAME, GTK_WIN_POS_CENTER, 0, G_CALLBACK(main_window_delete_lcb), NULL);
	gtk_window_set_wmclass(GTK_WINDOW(main_v->main_window), "Bluefish", "bluefish");

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(main_v->main_window), vbox);
	gtk_widget_show(vbox);

	gtk_window_set_policy(GTK_WINDOW(main_v->main_window), FALSE, FALSE, FALSE);
	gtk_widget_set_usize(GTK_WIDGET(main_v->main_window), 600, 600);
	gtk_window_set_policy(GTK_WINDOW(main_v->main_window), TRUE, TRUE, FALSE);

	/* first a menubar */
	menu_create_main(vbox);

	/* then the toolbars */
	
	/* then the work area */
	main_v->middlebox = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), main_v->middlebox, TRUE, TRUE, 0);
	gtk_widget_show(main_v->middlebox);


	/* notebook with the text widget in there */
	main_v->notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(main_v->notebook),GTK_POS_BOTTOM);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(main_v->notebook), TRUE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(main_v->notebook), TRUE);
	gtk_notebook_set_tab_border(GTK_NOTEBOOK(main_v->notebook), 1);
	gtk_notebook_popup_enable(GTK_NOTEBOOK(main_v->notebook));
	if (filenames) {
		docs_new_from_files(filenames);
	} else {
		/* no files to open, open an empty doc */
		file_new_cb(NULL, NULL);	
	}

	/* We have to know when the notebook changes */
	gui_notebook_bind_signals();

	gtk_notebook_set_page(GTK_NOTEBOOK(main_v->notebook), 0);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(main_v->notebook), TRUE);
	gtk_widget_show(main_v->notebook);

	filebrowser_hide(1);

	/* finally the statusbar */
	
	/* show all */
	DEBUG_MSG("gui_create_main, before show_all\n");
	gtk_widget_show_all(main_v->main_window);
}

void statusbar_message(gchar *message, gint time) {
	/* show the message on the statusbar */
}
