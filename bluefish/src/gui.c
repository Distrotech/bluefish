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
#include "bf_lib.h" /* get_int_from_string() */

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
	{
	GtkWidget *hbox;
	hbox = gtk_hbox_new(FALSE,0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	main_v->statuslabel = gtk_label_new(_(" line    0 "));
	gtk_box_pack_start(GTK_BOX(hbox), main_v->statuslabel, FALSE, FALSE, 0);
	main_v->statusbar = gtk_statusbar_new();
	gtk_box_pack_start(GTK_BOX(hbox), main_v->statusbar, TRUE, TRUE, 0);

	}
	/* show all */
	DEBUG_MSG("gui_create_main, before show_all\n");
	gtk_widget_show_all(main_v->main_window);
}
/***********************/
/* statusbar functions */
/***********************/

static gint statusbar_remove(gpointer message_id)
{
	gtk_statusbar_remove(GTK_STATUSBAR(main_v->statusbar), 0, GPOINTER_TO_INT(message_id));
	return 0;
}

void statusbar_message(gchar * message, gint time)
{
	if (main_v->statusbar) {
		gint count;

		count = gtk_statusbar_push(GTK_STATUSBAR(main_v->statusbar), 0, message);
		gtk_timeout_add(time, statusbar_remove, GINT_TO_POINTER(count));
	}
}


/***********************/
/* GOTO line functions */
/***********************/

typedef struct {
	GtkWidget *win;
	GtkWidget *entry;
	GtkWidget *check;
} Tgotoline;

static void tgl_destroy_lcb(GtkWidget * widget, GdkEvent *event,  Tgotoline *tgl) {
	window_destroy(tgl->win);
	g_free(tgl);
}

static void tgl_ok_clicked_lcb(GtkWidget * widget, Tgotoline *tgl)
{
	gchar *linestr;
	gint linenum;

	linestr = gtk_editable_get_chars(GTK_EDITABLE(tgl->entry), 0, -1);
	linenum = get_int_from_string(linestr);
	DEBUG_MSG("tgl_ok_clicked_lcb, going to line %d (linestr=%s)\n", linenum, linestr);
	g_free(linestr);
	
	if (linenum > 0) {
		doc_select_line(main_v->current_document, linenum, TRUE);
	}

	if (GTK_TOGGLE_BUTTON(tgl->check)->active) {
		if (linenum > 0) {
			gchar *new_text;
			gint position=0;
			gtk_editable_delete_text (GTK_EDITABLE(tgl->entry), 0, -1);
			new_text = g_strdup_printf("%d", linenum);
			gtk_editable_insert_text(GTK_EDITABLE(tgl->entry),new_text,strlen(new_text),&position);
			g_free(new_text);
		}
	} else {
		tgl_destroy_lcb(NULL, NULL, tgl);
	}

}

static void tgl_fromsel_clicked_lcb(GtkWidget * widget, Tgotoline *tgl) {
	gchar *string;
	GtkClipboard* cb;

	cb = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	string = gtk_clipboard_wait_for_text(cb);
	if (string) {
		gtk_entry_set_text (GTK_ENTRY(tgl->entry), string);
	}
	tgl_ok_clicked_lcb(widget, tgl);
}

static void tgl_cancel_clicked_lcb(GtkWidget *widget, gpointer data) {
	tgl_destroy_lcb(NULL, NULL, data);
}

void tgl_enter_lcb (GtkWidget *widget, gpointer ud) {
     Tgotoline *tgl;
     tgl = ud;
     tgl_ok_clicked_lcb (widget, tgl);
}

void go_to_line_win_cb(GtkWidget * widget, gpointer data)
{
	Tgotoline *tgl;
	GtkWidget *but1, *vbox, *hbox;
	
	tgl = g_new(Tgotoline, 1);
	tgl->win = window_full(_("Goto line"), GTK_WIN_POS_MOUSE
						  ,5, G_CALLBACK(tgl_destroy_lcb), tgl);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(tgl->win), vbox);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Line number: ")), FALSE, FALSE, 0);
	tgl->entry = boxed_entry_with_text(NULL, 20, hbox);

	but1 = bf_stock_button(_("From selection"), G_CALLBACK(tgl_fromsel_clicked_lcb), tgl);
	gtk_box_pack_start(GTK_BOX(hbox), but1, FALSE, FALSE, 0);

	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 1);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	tgl->check = boxed_checkbut_with_value(_("Keep dialog"), 0, hbox);
	
	but1 = bf_stock_ok_button(G_CALLBACK(tgl_ok_clicked_lcb), tgl);
	gtk_box_pack_start(GTK_BOX(hbox), but1, FALSE, FALSE, 0);
	gtk_window_set_default(GTK_WINDOW(tgl->win), but1);

	but1 = bf_stock_cancel_button(G_CALLBACK(tgl_cancel_clicked_lcb), tgl);
	gtk_box_pack_start(GTK_BOX(hbox), but1, FALSE, FALSE, 0);
	gtk_widget_grab_focus (tgl->entry);
        g_signal_connect (G_OBJECT (tgl->entry), "activate", G_CALLBACK(tgl_enter_lcb),
                            (gpointer *) tgl);
	gtk_widget_show_all(tgl->win);
}

