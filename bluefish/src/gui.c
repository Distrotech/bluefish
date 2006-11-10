/* Bluefish HTML Editor
 * gui.c - the main GUI
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
#include <time.h>		/* nanosleep() */
#include <string.h>	/* strchr() */
#include <unistd.h>	/* exit() */
#include <stdlib.h>	/* exit() on Solaris */

#include "config.h"

#include "bluefish.h"
#include "bf_lib.h"			/* get_int_from_string() */
#include "bookmark.h"
#include "document.h"		/* file_new_cb() */
/* #include "filebrowser.h" */
#include "filebrowser2.h"
#include "file_dialogs.h"
#include "fref2.h"
#include "gtk_easy.h"
#include "gui.h"
#include "menu.h"				/* menu_create_main(), recent_menu_init() */
#include "outputbox.h"		/* init_outputbox() */
#include "pixmap.h"			/* new_pixmap(); */
#include "preferences.h"	/* open_preferences_cb */
#include "project.h"
#include "snr2.h"				/* search_cb, replace_cb */
#include "stringlist.h"
#include "undo_redo.h"		/* undo_cb() redo_cb() etc. */
#include "plugins.h"

#ifdef HAVE_LIBASPELL
#include "bfspell.h"
#endif /* HAVE_LIBASPELL */

typedef struct {
	GtkWidget *window;
	GtkWidget *label;
} Tsplashscreen;

/******************************/
/* global vars for this module */
/******************************/
#ifndef NOSPLASH
static Tsplashscreen splashscreen;
#endif
/**************************/
/* start of the functions */
/**************************/

/**
 * notebook_hide:
 *
 * Hides the notebook. Used when loading a large amount of files, to avoid
 * slowing things down. Shows a "Stand by..." label instead..
 *
 * Return value: void
 ** /
void notebook_hide(Tbfwin *bfwin) {
	gtk_widget_hide (bfwin->notebook);
	gtk_widget_show (bfwin->notebook_fake);
}

/ **
 * notebook_show:
 *
 * Shows the notebook, after a notebook_hide() has been called.
 *
 * Return value: void
 ** /
void notebook_show(Tbfwin *bfwin) {
	gtk_widget_hide (bfwin->notebook_fake);
	gtk_widget_show (bfwin->notebook);	
}*/

void notebook_changed(Tbfwin *bfwin, gint newpage) {
	gint cur = newpage;
	gint doclistlen;
	DEBUG_MSG("notebook_changed, doclistlen=%d, newpage=%d, notebook_curpage=%d, last_notebook_page=%d, curdoc=%p\n"
			,g_list_length(bfwin->documentlist)
			,newpage
			,gtk_notebook_get_current_page(GTK_NOTEBOOK(bfwin->notebook))
			,bfwin->last_notebook_page
			,bfwin->current_document
			);
	if (newpage == -1) {
		/* this returns -1 if there is no current page */
		cur = gtk_notebook_get_current_page(GTK_NOTEBOOK(bfwin->notebook));
	}
	if ((bfwin->last_notebook_page == cur) 
		&& (bfwin->current_document != NULL)
		&& (bfwin->current_document == g_list_nth_data(bfwin->documentlist, cur))) {
		DEBUG_MSG("notebook_changed, NOT CHANGED cur=%d, documentlist[cur]==current_document (=%p), RETURNING\n",cur,bfwin->current_document);
		return;
	}
	doclistlen = g_list_length(bfwin->documentlist);
	if (cur == -1) {
		if (doclistlen > 0) {
			DEBUG_MSG("notebook_changed, WEIRD 1 cur=%d, but doclistlen=%d RETURNING\n",cur,doclistlen);
			bfwin->last_notebook_page = -2;
			return;
		}
	}
	if (doclistlen == 0) {
		DEBUG_MSG("notebook_changed, doclistlen=%d, before doc_new()!\n",doclistlen);
		if (bfwin->project && bfwin->project->close) project_save_and_close(bfwin, FALSE);
		bfwin->current_document = doc_new(bfwin,TRUE);
		bfwin->last_notebook_page = 1;
		DEBUG_MSG("notebook_changed, after doc_new(), returning\n");
		return;
	}
	/* if the documentlist has length 1, cur should not be larger then 0, if 2, cur should not be larger then 1, etc.*/
	if (cur >= doclistlen) {
		DEBUG_MSG("notebook_changed, DOCALREADYCLOSED, cur=%d, doclistlen=%d, RETURNING\n", cur, doclistlen);
		bfwin->last_notebook_page = -2;
		return;
	}
	bfwin->current_document = g_list_nth_data(bfwin->documentlist, cur);
	if (bfwin->current_document == NULL) {
		DEBUG_MSG("notebook_changed, WEIRD 2, doclist[%d] == NULL, RETURNING\n",cur);
		return;
	}
	DEBUG_MSG("notebook_changed, current_document=%p\n",bfwin->current_document);
	bfwin->last_notebook_page = cur;
	/* now we flush the queue first, so that we don't call doc_activate 
	on _this_ document if the user has another close click in the queue */
	flush_queue();

	doc_activate(bfwin->current_document);
/*	bmark_adjust_visible(bfwin);*/
	DEBUG_MSG("notebook_changed, finished\n");
}

gboolean switch_to_document_by_index(Tbfwin *bfwin,gint index) {
	if (index >= 0) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(bfwin->notebook), (index));
/*		notebook_changed();*/
		return TRUE;
	}
	return FALSE;
}

gboolean switch_to_document_by_pointer(Tbfwin *bfwin,Tdocument *document) {
	gint index;

	index = g_list_index(bfwin->documentlist, document);
	return switch_to_document_by_index(bfwin,index);
}

gboolean switch_to_document_by_uri(Tbfwin *bfwin,GnomeVFSURI *uri) {
	gint index;

	index = documentlist_return_index_from_uri(bfwin->documentlist,uri);
	return switch_to_document_by_index(bfwin,index);
}

void bfwin_docs_not_complete(Tbfwin *bfwin, gboolean increase) {
	if (increase) bfwin->num_docs_not_completed++;
	else bfwin->num_docs_not_completed--;
	DEBUG_MSG("bfwin_docs_not_complete, increase=%d, num=%d\n",increase,bfwin->num_docs_not_completed);
}

static void notebook_switch_page_lcb(GtkWidget *notebook,GtkNotebookPage *page,gint page_num,Tbfwin *bfwin) {
	DEBUG_MSG("notebook_switch_page_lcb, page=%d\n", page_num);
	notebook_changed(bfwin,page_num);
}

void gui_notebook_move(Tbfwin *bfwin, gboolean move_left) {
	GtkWidget *tmp;
	
	tmp = gtk_widget_get_parent(bfwin->current_document->view);
	DEBUG_MSG("gui_notebook_move, found parent %p\n",tmp);
	if (tmp) {
		gint curpos, newpos;
		GList *cur;
		curpos = gtk_notebook_page_num(GTK_NOTEBOOK(bfwin->notebook),tmp);
		cur = g_list_nth(bfwin->documentlist, curpos);
#ifdef DEVELOPMENT
		if (!cur) exit(1);
#endif
		newpos = curpos + ((move_left) ? -1 : 1);
		DEBUG_MSG("gui_notebook_move, cur=%d, new=%d (num_pages=%d)\n",curpos,newpos,gtk_notebook_get_n_pages(GTK_NOTEBOOK(bfwin->notebook)));
		if (newpos >= 0 && newpos < gtk_notebook_get_n_pages(GTK_NOTEBOOK(bfwin->notebook))) {
			GList *moveto;
			gtk_notebook_reorder_child(GTK_NOTEBOOK(bfwin->notebook),tmp,newpos);
			moveto = (move_left) ? g_list_previous(cur) : g_list_next(cur);
#ifdef DEVELOPMENT
			if (!moveto) exit(1);
#endif
			pointer_switch_addresses(&cur->data, &moveto->data);
		}
	}
}

void gui_notebook_switch(Tbfwin *bfwin,guint action,GtkWidget *widget) {
	switch (action) {
		case 1: gtk_notebook_prev_page(GTK_NOTEBOOK(bfwin->notebook));
		break;
		case 2: gtk_notebook_next_page(GTK_NOTEBOOK(bfwin->notebook));
		break;
		case 3: gtk_notebook_set_current_page(GTK_NOTEBOOK(bfwin->notebook), 0);
		break;
		case 4: gtk_notebook_set_current_page(GTK_NOTEBOOK(bfwin->notebook), -1);
		break;
		case 5: gui_notebook_move(bfwin, TRUE);
		break;
		case 6: gui_notebook_move(bfwin, FALSE);
		break;
	}
}

static void left_panel_notify_position_lcb(GObject *object,GParamSpec *pspec,gpointer data){
	gint position;
	g_object_get(object, pspec->name, &position, NULL);
	DEBUG_MSG("left_panel_notify_position_lcb, restore_dimensions=%d, new position=%d\n",main_v->props.restore_dimensions, position);
	if (main_v->props.restore_dimensions) {
		if (main_v->props.left_panel_left) {
			main_v->globses.left_panel_width = position;
		} else {
			main_v->globses.left_panel_width = main_v->globses.main_window_w - position;
		}
	}
}

GtkWidget *left_panel_build(Tbfwin *bfwin) {
/*	GtkWidget *fileb;*/
	GtkWidget *left_notebook = gtk_notebook_new();
	GtkWidget *fref;
	GtkWidget *bmarks;
	GtkWidget *fb2g;
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(left_notebook),main_v->props.leftpanel_tabposition);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(left_notebook), TRUE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(left_notebook), FALSE);
	gtk_notebook_set_tab_hborder(GTK_NOTEBOOK(left_notebook), 0);
	gtk_notebook_set_tab_vborder(GTK_NOTEBOOK(left_notebook), 0);
	gtk_notebook_popup_enable(GTK_NOTEBOOK(left_notebook));
	DEBUG_MSG("left_panel_build, building left panel for bfwin %p\n",bfwin);
/*	fileb = filebrowser_init(bfwin);*/
	fref = fref_gui(bfwin);
	bmarks = bmark_gui(bfwin);
	fb2g = fb2_init(bfwin);
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(left_notebook),fb2g,new_pixmap(105),gtk_label_new(_("Filebrowser")));
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(left_notebook),bmarks,new_pixmap(104),gtk_label_new(_("Bookmarks")));
/*	gtk_notebook_append_page_menu(GTK_NOTEBOOK(left_notebook),fileb,new_pixmap(105),gtk_label_new(_("Filebrowser")));*/
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(left_notebook),fref,new_pixmap(106),gtk_label_new(_("Function reference")));

	gtk_widget_show_all(left_notebook);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(left_notebook),0);
	return left_notebook;
}

/**
 * if there is a left panel, this function will rebuild all widgets
 */
void left_panel_rebuild(Tbfwin *bfwin) {
	if (bfwin->hpane) {
		DEBUG_MSG("left_panel_rebuild, destroying widgets\n");
		gtk_widget_destroy(bfwin->leftpanel_notebook);
		DEBUG_MSG("left_panel_rebuild, cleanup\n");
/*		filebrowser_cleanup(bfwin);*/
		fref_cleanup(bfwin);
		bmark_cleanup(bfwin);
		fb2_cleanup(bfwin);
		DEBUG_MSG("left_panel_rebuild, re-init\n");
		bfwin->leftpanel_notebook = left_panel_build(bfwin);
		if (main_v->props.left_panel_left) {
			gtk_paned_add1(GTK_PANED(bfwin->hpane), bfwin->leftpanel_notebook);
		} else {
			gtk_paned_add2(GTK_PANED(bfwin->hpane), bfwin->leftpanel_notebook);
		}
		gtk_widget_show_all(bfwin->leftpanel_notebook);
	}
}

void left_panel_show_hide_toggle(Tbfwin *bfwin,gboolean first_time, gboolean show, gboolean sync_menu) {
	DEBUG_MSG("left_panel_show_hide_toggle, bfwin=%p, first_time=%d, show=%d, sync_menu=%d\n",bfwin,first_time,show,sync_menu);
	if (sync_menu) {
		DEBUG_MSG("left_panel_show_hide_toggle, trying to sync menu\n");
		setup_toggle_item_from_widget(bfwin->menubar, "/View/View Sidebar", show);
	}
	if (!first_time && ((show && bfwin->hpane) || (!show && bfwin->hpane == NULL))) {
		DEBUG_MSG("left_panel_show_hide_toggle, retrurning!!, show=%d, bfwin->hpane=%p, first_time=%d\n",show,bfwin->hpane,first_time);
		return;
	} 

	if (!first_time) {
		gtk_widget_ref(bfwin->notebook_box);
		if (show) {
			gtk_container_remove(GTK_CONTAINER(bfwin->middlebox), bfwin->notebook_box);
		} else {
			gtk_container_remove(GTK_CONTAINER(bfwin->hpane), bfwin->notebook_box);
			gtk_widget_destroy(bfwin->hpane);
			/*filebrowser_cleanup(bfwin);*/
			fb2_cleanup(bfwin);
			fref_cleanup(bfwin);
			bmark_cleanup(bfwin); 
		}
	}
	if (show) {
		bfwin->hpane = gtk_hpaned_new();
		if (main_v->props.left_panel_left) {
			DEBUG_MSG("set paned position to %d (left)\n",main_v->globses.left_panel_width);
			gtk_paned_set_position(GTK_PANED(bfwin->hpane), main_v->globses.left_panel_width);
		} else {
			DEBUG_MSG("set paned position to %d (right)\n",main_v->globses.main_window_w - main_v->globses.left_panel_width);
			gtk_paned_set_position(GTK_PANED(bfwin->hpane), main_v->globses.main_window_w - main_v->globses.left_panel_width);
		}
		g_signal_connect(G_OBJECT(bfwin->hpane),"notify::position",G_CALLBACK(left_panel_notify_position_lcb), NULL);
		bfwin->leftpanel_notebook = left_panel_build(bfwin);
		gtk_paned_add1(GTK_PANED(bfwin->hpane), main_v->props.left_panel_left ? bfwin->leftpanel_notebook : bfwin->notebook_box);
		gtk_paned_add2(GTK_PANED(bfwin->hpane), main_v->props.left_panel_left ? bfwin->notebook_box : bfwin->leftpanel_notebook);
		gtk_box_pack_start(GTK_BOX(bfwin->middlebox), bfwin->hpane, TRUE, TRUE, 0);
		gtk_widget_show(bfwin->hpane);
	} else {
		bfwin->hpane = NULL;
		bfwin->leftpanel_notebook = NULL;
		gtk_box_pack_start(GTK_BOX(bfwin->middlebox), bfwin->notebook_box, TRUE, TRUE, 0);
	}
	if (!first_time) {
		gtk_widget_unref(bfwin->notebook_box);
	}
}

void gui_set_title(Tbfwin *bfwin, Tdocument *doc) {
	gchar *title, *prfilepart;
	const gchar *tablabel;
	tablabel = gtk_label_get_text(GTK_LABEL(doc->tab_label));
	if (bfwin->project) {
		prfilepart = g_strconcat(bfwin->project->name," - ",tablabel,NULL);
	} else {
		prfilepart = g_strdup(tablabel);
	}
	title = g_strconcat(prfilepart, " - Bluefish "VERSION,NULL);
	gtk_window_set_title(GTK_WINDOW(bfwin->main_window),title);
	/*rename_window_entry_in_all_windows(bfwin, title);*/
	g_free(title);
	g_free(prfilepart);
}

static void doc_indent_lcb(GtkWidget *wid,Tbfwin *bfwin) {
	if (bfwin->current_document) {
		doc_indent_selection(bfwin->current_document,0);
	}
}
static void doc_unindent_lcb(GtkWidget *wid,Tbfwin *bfwin) {
	if (bfwin->current_document) {
		doc_indent_selection(bfwin->current_document,1);
	}
}

void make_main_toolbar(Tbfwin *bfwin) {
	GtkWidget *toolbar = gtk_toolbar_new ();
	DEBUG_MSG("make_main_toolbar, started\n");
	gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_icon_size (GTK_TOOLBAR (toolbar), GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_container_add (GTK_CONTAINER(bfwin->main_toolbar_hb), toolbar);

	gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_NEW, _("New"),
								"", G_CALLBACK(file_new_cb), bfwin, -1);
	gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_OPEN, _("Open..."),
								"", G_CALLBACK(file_open_cb), bfwin, -1);
	gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_SAVE, _("Save"),
								"", G_CALLBACK(file_save_cb), bfwin, -1);
	gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_SAVE_AS, _("Save As..."),	
								"", G_CALLBACK(file_save_as_cb), bfwin, -1);
	gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_CLOSE, _("Close"),	
								"", G_CALLBACK(file_close_cb), bfwin, -1);		
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
	gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_CUT, _("Cut"),	
								"", G_CALLBACK(edit_cut_cb), bfwin, -1);
	gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_COPY, _("Copy"),	
								"", G_CALLBACK(edit_copy_cb), bfwin, -1);
	gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_PASTE, _("Paste"),	
								"", G_CALLBACK(edit_paste_cb), bfwin, -1);
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));								
	gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_FIND, _("Find..."),	
								"", G_CALLBACK(search_cb), bfwin, -1);
	gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_FIND_AND_REPLACE, _("Replace..."),	
								"", G_CALLBACK(replace_cb), bfwin, -1);
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));								
	bfwin->toolbar_undo = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_UNDO, _("Undo"),	
								"", G_CALLBACK(undo_cb), bfwin, -1);
	bfwin->toolbar_redo = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_REDO, _("Redo"),	
								"", G_CALLBACK(redo_cb), bfwin, -1);
	gtk_toolbar_append_space (GTK_TOOLBAR(toolbar));
	gtk_toolbar_insert_stock (GTK_TOOLBAR(toolbar), GTK_STOCK_UNINDENT, _("Shift Left"), 
	                     "", G_CALLBACK(doc_unindent_lcb), bfwin, -1);
	gtk_toolbar_insert_stock (GTK_TOOLBAR(toolbar), GTK_STOCK_INDENT, _("Shift Right"),
                        "", G_CALLBACK(doc_indent_lcb), bfwin, -1);

/*
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Print..."), "",
							new_pixmap(015), G_CALLBACK(file_print_cb), NULL);*/
#ifdef HAVE_LIBASPELL
	gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_SPELL_CHECK, _("Spellcheck..."),
							"", G_CALLBACK(spell_check_cb), bfwin, -1);
#endif /* HAVE_LIBASPELL */

	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL,
							_("View in browser"), "",
							new_pixmap(102), G_CALLBACK(browser_toolbar_cb), bfwin);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_PREFERENCES, _("Preferences..."),	
								"", G_CALLBACK(open_preferences_cb), NULL, -1);	

	gtk_widget_show_all(toolbar);
}

void gui_set_undo_redo_widgets(Tbfwin *bfwin, gboolean undo, gboolean redo) {
	if (GTK_WIDGET_VISIBLE(bfwin->main_toolbar_hb)) {
		gtk_widget_set_sensitive(bfwin->toolbar_redo, redo);
		gtk_widget_set_sensitive(bfwin->toolbar_undo, undo);
	}
	
	menuitem_set_sensitive(bfwin->menubar, "/Edit/Undo", undo);
	menuitem_set_sensitive(bfwin->menubar, "/Edit/Undo All", undo);
	menuitem_set_sensitive(bfwin->menubar, "/Edit/Redo", redo);
	menuitem_set_sensitive(bfwin->menubar, "/Edit/Redo All", redo);
}

void gui_set_document_widgets(Tdocument *doc) {
	GtkItemFactory *tmp1 = gtk_item_factory_from_widget(BFWIN(doc->bfwin)->menubar);

	setup_toggle_item(tmp1,("/Document/Highlight Syntax"), BF_TEXTVIEW(doc->view)->highlight);			
	gui_set_undo_redo_widgets(doc->bfwin, doc_has_undo_list(doc), doc_has_redo_list(doc));
	setup_toggle_item(gtk_item_factory_from_widget(BFWIN(doc->bfwin)->menubar),"/Document/Wrap", doc->wrapstate);
	setup_toggle_item(gtk_item_factory_from_widget(BFWIN(doc->bfwin)->menubar),"/Document/Line Numbers", doc->linenumberstate);
	setup_toggle_item(gtk_item_factory_from_widget(BFWIN(doc->bfwin)->menubar),"/Document/Auto Close HTML tags", doc->autoclosingtag);
	setup_toggle_item(gtk_item_factory_from_widget(BFWIN(doc->bfwin)->menubar),"/Document/Show blocks", doc->blocksstate);
	setup_toggle_item(gtk_item_factory_from_widget(BFWIN(doc->bfwin)->menubar),"/Document/Show symbols", doc->symstate);
	
/*#ifndef USE_SCANNER	why did we not set the encoding and filetype with the scanner enabled????*/
	menu_current_document_set_toggle_wo_activate(BFWIN(doc->bfwin),doc->hl, doc->encoding);
/*#endif	*/

	/* we should also disable certain menu's if the document is readonly */
	menuitem_set_sensitive(BFWIN(doc->bfwin)->menubar, "/File/Save", !doc->readonly);
	menuitem_set_sensitive(BFWIN(doc->bfwin)->menubar, "/File/Insert...", !doc->readonly);
	menuitem_set_sensitive(BFWIN(doc->bfwin)->menubar, "/File/Rename...", !doc->readonly);
	
	menuitem_set_sensitive(BFWIN(doc->bfwin)->menubar, "/Edit/Cut", !doc->readonly);
	menuitem_set_sensitive(BFWIN(doc->bfwin)->menubar, "/Edit/Paste", !doc->readonly);
	menuitem_set_sensitive(BFWIN(doc->bfwin)->menubar, "/Edit/Replace...", !doc->readonly);
	menuitem_set_sensitive(BFWIN(doc->bfwin)->menubar, "/Edit/Replace Again", !doc->readonly);
	menuitem_set_sensitive(BFWIN(doc->bfwin)->menubar, "/Edit/Shift Right", !doc->readonly);
	menuitem_set_sensitive(BFWIN(doc->bfwin)->menubar, "/Edit/Shift Left", !doc->readonly);

}

void gui_notebook_bind_signals(Tbfwin *bfwin) {
	bfwin->notebook_switch_signal = g_signal_connect_after(G_OBJECT(bfwin->notebook),"switch-page",G_CALLBACK(notebook_switch_page_lcb), bfwin);
}

void gui_notebook_unbind_signals(Tbfwin *bfwin) {
if (bfwin->notebook_switch_signal != 0) {
		g_signal_handler_disconnect(G_OBJECT(bfwin->notebook),bfwin->notebook_switch_signal);
		bfwin->notebook_switch_signal = 0;
	}
}

static gboolean gui_main_window_configure_event_lcb(GtkWidget *widget,GdkEvent *revent,Tbfwin *bfwin) {
	if (main_v->props.restore_dimensions) {
		if (revent->type == GDK_CONFIGURE) {
			GdkEventConfigure *event = (GdkEventConfigure *)revent;
			if (main_v->globses.main_window_w > 0 ) {
				main_v->globses.main_window_w = event->width;
				main_v->globses.main_window_h = event->height;
				DEBUG_MSG("gui_main_window_configure_event_lcb, width=%d, height=%d\n",main_v->globses.main_window_w,main_v->globses.main_window_h);
			}
		} else if (revent->type == GDK_WINDOW_STATE) {
			GdkEventWindowState *event = (GdkEventWindowState *)revent;
			if (event->new_window_state == GDK_WINDOW_STATE_MAXIMIZED && main_v->globses.main_window_w > 0) {
				main_v->globses.main_window_w = -1 * main_v->globses.main_window_w; /* negative means it is maximized !! */
				DEBUG_MSG("gui_main_window_configure_event_lcb, maximized!! width=%d\n",main_v->globses.main_window_w);
			} else if (event->new_window_state != GDK_WINDOW_STATE_MAXIMIZED && main_v->globses.main_window_w < 0) {
				main_v->globses.main_window_w = -1 * main_v->globses.main_window_w; /* make it positive again */
				DEBUG_MSG("gui_main_window_configure_event_lcb, NOT-maximized, width=%d\n",main_v->globses.main_window_w);
			}
		}
	}
	return FALSE;
}

enum {
TARGET_URI_LIST,
TARGET_STRING
} Tdnd_types;
static void main_win_on_drag_data_lcb(GtkWidget * widget, GdkDragContext * context
			, gint x, gint y, GtkSelectionData * data
			, guint info, guint time, Tbfwin *bfwin) {
	int mode = 0;
	gchar *stringdata;

	if ((data->length == 0) || (data->format != 8) || ((info != TARGET_STRING) && (info != TARGET_URI_LIST))) {
		DEBUG_MSG("on_drag_data_cb, currently unknown DnD object, need to do string comparision\n");
		gtk_drag_finish(context, FALSE, TRUE, time);
		return;
	}
	stringdata = g_strndup((gchar *)data->data, data->length);
	doc_new_from_input(bfwin, stringdata, FALSE, FALSE, -1);
	gtk_drag_finish(context, TRUE, (mode == GDK_ACTION_COPY), time);
}

static gboolean gui_periodic_check_lcb(gpointer data) {
	Tbfwin *bfwin = data;
	if (bfwin->current_document) {
		doc_start_modified_check(bfwin->current_document);
	}
	return TRUE;
}

void gui_bfwin_periodic_check(Tbfwin *bfwin, gboolean enabled) {
	if (enabled) {
		if (!bfwin->periodic_check_id) bfwin->periodic_check_id = g_timeout_add_full(G_PRIORITY_LOW,15000,gui_periodic_check_lcb,bfwin,NULL);
	} else {
		if (bfwin->periodic_check_id) g_source_remove(bfwin->periodic_check_id);
	}
}

void gui_apply_settings(Tbfwin *bfwin) {
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(bfwin->notebook),main_v->props.document_tabposition);
	/* We don't want to set the tab position if the left panel is hidden */
	if (bfwin->leftpanel_notebook) {
		gtk_notebook_set_tab_pos(GTK_NOTEBOOK(bfwin->leftpanel_notebook),main_v->props.leftpanel_tabposition);

		/* check if the left panel needs to move over to the right */
		if (main_v->props.left_panel_left && bfwin->leftpanel_notebook == GTK_PANED(bfwin->hpane)->child1) {
			DEBUG_MSG("gui_apply_settings, left panel is on the right location\n");
		} else {
			left_panel_show_hide_toggle(bfwin,FALSE, FALSE, FALSE);
			left_panel_show_hide_toggle(bfwin,FALSE, TRUE, FALSE);
		}
	}
	gui_bfwin_periodic_check(bfwin, main_v->props.do_periodic_check);
}

static void gui_bfwin_cleanup(Tbfwin *bfwin) {
	GList *tmplist;
	/* call all cleanup functions here */
	/*remove_window_entry_from_all_windows(bfwin);*/
	
	/* all documents have to be freed for this window */
	tmplist = g_list_first(bfwin->documentlist);
	DEBUG_MSG("gui_bfwin_cleanup, have %d documents in window %p\n",g_list_length(bfwin->documentlist),bfwin);
	while (tmplist) {
		DEBUG_MSG("gui_bfwin_cleanup closing doc=%p\n",tmplist->data);
		doc_destroy(DOCUMENT(tmplist->data), TRUE);
		/* no this is not an indefinite loop, because the documents remove themselves
		from the documentlist, we remove the top document untill there are no documents
		left */
		tmplist = g_list_first(bfwin->documentlist);
	}
	gui_bfwin_periodic_check(bfwin,FALSE);
	fb2_cleanup(bfwin);
	bmark_cleanup(bfwin);
	outputbox_cleanup(bfwin);
	snr2_cleanup(bfwin);
}

void main_window_destroy_lcb(GtkWidget *widget,Tbfwin *bfwin) {
	DEBUG_MSG("main_window_destroy_lcb, started for bfwin=%p, first cleanup any project (%p)\n",bfwin,bfwin->project);
	if (bfwin->project) {
		project_save_and_close(bfwin, TRUE);
	}
	
	DEBUG_MSG("main_window_destroy_lcb, will hide the window now\n");
	gtk_widget_hide(bfwin->main_window);
	
	main_v->bfwinlist = g_list_remove(main_v->bfwinlist, bfwin);
	DEBUG_MSG("main_window_destroy_lcb, bfwin(%p) is removed from bfwinlist\n",bfwin);
	gui_bfwin_cleanup(bfwin);
	DEBUG_MSG("main_window_destroy_lcb, will destroy the window now\n");
	gtk_widget_destroy(bfwin->main_window);
	g_free(bfwin);
	DEBUG_MSG("main_window_destroy_lcb, bfwin is free'ed\n");
	if (NULL == main_v->bfwinlist) {
		bluefish_exit_request();
	}
}
gboolean main_window_delete_event_lcb(GtkWidget *widget,GdkEvent *event,Tbfwin *bfwin) {
	/* If you return FALSE in the "delete_event" signal handler,
	 * GTK will emit the "destroy" signal. Returning TRUE means
	 * you don't want the window to be destroyed.
	 * This is useful for popping up 'are you sure you want to quit?'
	 * type dialogs. */
	DEBUG_MSG("main_window_delete_event_lcb, started\n");
	if (bfwin->project) {
		project_save_and_close(bfwin, TRUE);
		return (bfwin->documentlist != NULL); /* the last document that closes should close the window, so return TRUE */
	} else {
		if (bfwin->documentlist) {
			doc_close_multiple_backend(bfwin, TRUE);
			/* the last document that closes should close the window, so return TRUE */
			return TRUE;
		}
	}
	return FALSE;
}

void gui_create_main(Tbfwin *bfwin, GList *filenames) {
	GtkWidget *vbox;
	GList *tmplist;
	DEBUG_MSG("gui_create_main, bfwin=%p\n",bfwin);
	bfwin->main_window = window_full2(_("New Bluefish Window"), GTK_WIN_POS_CENTER, 0, G_CALLBACK(main_window_destroy_lcb), bfwin, FALSE, NULL);
	gtk_window_set_role(GTK_WINDOW(bfwin->main_window), "bluefish");
	if (main_v->globses.main_window_w > 0) {
		gtk_window_set_default_size(GTK_WINDOW(bfwin->main_window), main_v->globses.main_window_w, main_v->globses.main_window_h);
	} else {
		gtk_window_set_default_size(GTK_WINDOW(bfwin->main_window), main_v->globses.main_window_w * -1, main_v->globses.main_window_h);
		gtk_window_maximize(GTK_WINDOW(bfwin->main_window));
	}
	g_signal_connect(G_OBJECT(bfwin->main_window), "delete_event", G_CALLBACK(main_window_delete_event_lcb), bfwin);
	g_signal_connect(G_OBJECT(bfwin->main_window), "configure-event", G_CALLBACK(gui_main_window_configure_event_lcb), bfwin);
	g_signal_connect(G_OBJECT(bfwin->main_window), "window-state-event", G_CALLBACK(gui_main_window_configure_event_lcb), bfwin);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(bfwin->main_window), vbox);
	gtk_widget_show(vbox);
	
	bfwin->toolbarbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), bfwin->toolbarbox, FALSE, FALSE, 0);
	gtk_widget_show(bfwin->toolbarbox);

	/* first a menubar */
	DEBUG_MSG("gui_create_main, starting menu_create_main\n");
	menu_create_main(bfwin, bfwin->toolbarbox);
	DEBUG_MSG("gui_create_main, starting recent_menu\n");
	recent_menu_init(bfwin);
	DEBUG_MSG("gui_create_main, starting external-encoding_menu\n");
	external_menu_rebuild(bfwin);
	encoding_menu_rebuild(bfwin);
	snr2_init(bfwin);
/*	add_window_entry_to_all_windows(bfwin);
	add_allwindows_entries_to_window(bfwin);*/
	/* then the toolbars */
	{
		DEBUG_MSG("gui_create_main, creating handles for all menu/toolbars\n");
		bfwin->main_toolbar_hb = gtk_handle_box_new();
		gtk_box_pack_start(GTK_BOX(bfwin->toolbarbox), bfwin->main_toolbar_hb, FALSE, FALSE, 0);
		bfwin->html_toolbar_hb = gtk_handle_box_new();
		gtk_box_pack_start(GTK_BOX(bfwin->toolbarbox), bfwin->html_toolbar_hb, FALSE, FALSE, 0);
		bfwin->custom_menu_hb = gtk_handle_box_new();
		gtk_box_pack_start(GTK_BOX(bfwin->toolbarbox), bfwin->custom_menu_hb, FALSE, FALSE, 0);

		if (bfwin->session->view_main_toolbar) {
			make_main_toolbar(bfwin);
			gtk_widget_show(bfwin->main_toolbar_hb);
		}
		if (bfwin->session->view_custom_menu) {
			make_cust_menubar(bfwin,bfwin->custom_menu_hb);
			gtk_widget_show(bfwin->custom_menu_hb);
		}
	}

	
	/* the area for the middlebox and the outputbox */
	bfwin->vpane = gtk_vpaned_new();
	gtk_box_pack_start(GTK_BOX(vbox), bfwin->vpane, TRUE, TRUE, 0);
	/* then the area for left panel and */
	bfwin->middlebox = gtk_hbox_new(TRUE, 0);
	gtk_paned_add1(GTK_PANED(bfwin->vpane), bfwin->middlebox);
	gtk_widget_show(bfwin->middlebox);
	gtk_widget_show(bfwin->vpane);
	
	/* Fake-label (for notebook_hide() and _show() ;) */
	bfwin->notebook_fake = gtk_label_new(_("Stand by..."));
	
	/* notebook with the text widget in there */
	bfwin->notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(bfwin->notebook),main_v->props.document_tabposition);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(bfwin->notebook), TRUE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(bfwin->notebook), FALSE);
	gtk_notebook_set_tab_hborder(GTK_NOTEBOOK(bfwin->notebook), 0);
	gtk_notebook_set_tab_vborder(GTK_NOTEBOOK(bfwin->notebook), 0);
	gtk_notebook_popup_enable(GTK_NOTEBOOK(bfwin->notebook));

	/* Add notebook and its fake friend to their common hbox. */
	bfwin->notebook_box = gtk_hbox_new (TRUE, 0);
	gtk_box_pack_start(GTK_BOX(bfwin->notebook_box), bfwin->notebook, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(bfwin->notebook_box), bfwin->notebook_fake, TRUE, TRUE, 0);

	/* output_box * /
	init_output_box(bfwin, vbox); */
	bfwin->outputbox = NULL;

	left_panel_show_hide_toggle(bfwin,TRUE, (bfwin->session->view_left_panel), FALSE);

	/* finally the statusbar */
	{
		GtkWidget *hbox;
		gint onecharwidth;
		hbox = gtk_hbox_new(FALSE,0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
		bfwin->statusbar = gtk_statusbar_new();
		gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(bfwin->statusbar), FALSE);
		gtk_box_pack_start(GTK_BOX(hbox), bfwin->statusbar, TRUE, TRUE, 0);
		bfwin->statusbar_lncol = gtk_statusbar_new();
		gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(bfwin->statusbar_lncol), FALSE);
		gtk_box_pack_start(GTK_BOX(hbox), bfwin->statusbar_lncol, FALSE, FALSE, 0);
		/* I hope the 'w' is an average width character */
		onecharwidth = widget_get_string_size(bfwin->statusbar_lncol, "w");
		gtk_widget_set_size_request(GTK_WIDGET(bfwin->statusbar_lncol), 17*onecharwidth, -1);
		bfwin->statusbar_insovr = gtk_statusbar_new();
		gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(bfwin->statusbar_insovr), FALSE);
		gtk_box_pack_start(GTK_BOX(hbox), bfwin->statusbar_insovr, FALSE, FALSE, 0);
		gtk_widget_set_size_request(GTK_WIDGET(bfwin->statusbar_insovr), onecharwidth * 4, -1);
		bfwin->statusbar_editmode = gtk_statusbar_new();
		gtk_box_pack_start(GTK_BOX(hbox), bfwin->statusbar_editmode, FALSE, FALSE, 0);
		gtk_widget_set_size_request(GTK_WIDGET(bfwin->statusbar_editmode), onecharwidth * 25, -1);
		gtk_widget_show_all(hbox);
	}
	
	/* here we ask any plugins to build there gui */
	g_slist_foreach(main_v->plugins, bfplugins_gui, bfwin);

	/* We have to know when the notebook changes */
	gui_notebook_bind_signals(bfwin);
	
	/* everything is ready - we can start loading documents */
	/* start to open an empty doc */
	file_new_cb(NULL, bfwin);
	if (filenames) {
		tmplist = g_list_last(filenames);
		bfwin->focus_next_new_doc = TRUE;
		while (tmplist) {
			doc_new_from_input(bfwin, tmplist->data, FALSE, (bfwin->project != NULL), -1);
			tmplist = g_list_previous(tmplist);
		}
		DEBUG_MSG("gui_create_main, we have filenames, load them\n");
	}

/*	gtk_notebook_set_page(GTK_NOTEBOOK(bfwin->notebook), 0);*/
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(bfwin->notebook), TRUE);
	/* don't use show_all since some widgets are and should be hidden */
	gtk_widget_show(bfwin->notebook);
	gtk_widget_show (bfwin->notebook_box);

	{
		/* drag n drop support */
		const GtkTargetEntry drag_dest_types[] = {
			{"text/uri-list", 0, TARGET_URI_LIST },
			{"STRING", 0, TARGET_STRING},
		};
		gtk_drag_dest_set(bfwin->main_window, (GTK_DEST_DEFAULT_ALL)
				,drag_dest_types, 2
				,(GDK_ACTION_DEFAULT | GDK_ACTION_COPY | GDK_ACTION_MOVE |
				GDK_ACTION_LINK | GDK_ACTION_PRIVATE | GDK_ACTION_ASK));
		g_signal_connect(G_OBJECT(bfwin->main_window), "drag_data_received", G_CALLBACK(main_win_on_drag_data_lcb), bfwin);
	}
	if (main_v->props.do_periodic_check) {
		gui_bfwin_periodic_check(bfwin,TRUE);
	}
}

void gui_show_main(Tbfwin *bfwin) {
	DEBUG_MSG("gui_show_main, before show\n");
	/* don't use show_all since some widgets are and should be hidden */
	gtk_widget_show(bfwin->main_window);
	flush_queue();
	doc_scroll_to_cursor(bfwin->current_document);
/*	if ((bfwin->project && bfwin->project->view_left_panel) || (!bfwin->project && main_v->props.view_left_panel)) {
		filebrowser_scroll_initial(bfwin);
	}*/
}
/***********************/
/* statusbar functions */
/***********************/
typedef struct {
	gint message_id;
	Tbfwin *bfwin;
} Tstatusbar_remove;

static gint statusbar_remove(gpointer sr) {
	gtk_statusbar_remove(GTK_STATUSBAR(((Tstatusbar_remove *)sr)->bfwin->statusbar), 0, ((Tstatusbar_remove *)sr)->message_id);
	g_free(sr);
	return FALSE;
}

void statusbar_message(Tbfwin *bfwin,gchar *message, gint time) {
	if (bfwin->statusbar) {
		Tstatusbar_remove *sr = g_new(Tstatusbar_remove,1);
		sr->bfwin = bfwin;
		sr->message_id = gtk_statusbar_push(GTK_STATUSBAR(bfwin->statusbar), 0, message);
		gtk_timeout_add(time, statusbar_remove, sr);
	}
}


/***********************/
/* GOTO line functions */
/***********************/

typedef struct {
	GtkWidget *win;
	GtkWidget *entry;
	GtkWidget *check;
	Tbfwin *bfwin;
} Tgotoline;

static void tgl_destroy_lcb(GtkWidget * widget, Tgotoline *tgl) {
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
		doc_select_line(tgl->bfwin->current_document, linenum, TRUE);
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
		tgl_destroy_lcb(NULL, tgl);
	}

}

static void tgl_fromsel_clicked_lcb(GtkWidget * widget, Tgotoline *tgl) {
	gchar *string;
	GtkClipboard* cb;

	cb = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	string = gtk_clipboard_wait_for_text(cb);
	if (string) {
		gtk_entry_set_text(GTK_ENTRY(tgl->entry), string);
	}
	tgl_ok_clicked_lcb(widget, tgl);
}

static void tgl_cancel_clicked_lcb(GtkWidget *widget, gpointer data) {
	tgl_destroy_lcb(NULL, data);
}

void tgl_enter_lcb (GtkWidget *widget, gpointer ud) {
     Tgotoline *tgl;
     tgl = ud;
     tgl_ok_clicked_lcb (widget, tgl);
}

void go_to_line_win_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget) {
	Tgotoline *tgl;
	GtkWidget *but1, *vbox, *hbox, *label;
	
	tgl = g_new(Tgotoline, 1);
	tgl->bfwin = bfwin;
	tgl->win = window_full(_("Goto line"), GTK_WIN_POS_MOUSE
						  ,12, G_CALLBACK(tgl_destroy_lcb), tgl, TRUE);
	vbox = gtk_vbox_new(FALSE, 12);
	gtk_container_add(GTK_CONTAINER(tgl->win), vbox);

	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic(_("_Line number:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	tgl->entry = boxed_entry_with_text(NULL, 20, hbox);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), tgl->entry); /* mnemonic for label */

	but1 = bf_generic_mnemonic_button(_("From _selection"), G_CALLBACK(tgl_fromsel_clicked_lcb), tgl);
	gtk_box_pack_start(GTK_BOX(hbox), but1, FALSE, FALSE, 0);

	tgl->check = boxed_checkbut_with_value(_("Keep _dialog"), 0, vbox);
	
	gtk_box_pack_start(GTK_BOX(vbox), gtk_hseparator_new(), TRUE, TRUE, 0);
	
	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 12);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);	
	
	but1 = bf_stock_cancel_button(G_CALLBACK(tgl_cancel_clicked_lcb), tgl);
	gtk_box_pack_start(GTK_BOX(hbox), but1, FALSE, FALSE, 0);
	gtk_widget_grab_focus (tgl->entry);

	but1 = bf_stock_ok_button(G_CALLBACK(tgl_ok_clicked_lcb), tgl);
	gtk_box_pack_start(GTK_BOX(hbox), but1, FALSE, FALSE, 0);
	gtk_window_set_default(GTK_WINDOW(tgl->win), but1);

	gtk_widget_show_all(tgl->win);
}

void go_to_line_from_selection_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget) {
	gchar *string;
	GtkClipboard* cb;
	gint linenum;

	cb = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	string = gtk_clipboard_wait_for_text(cb);
	if (string) {
		linenum = get_int_from_string(string);
		if (linenum > 0) {
			doc_select_line(bfwin->current_document, linenum, TRUE);
		}
	}
}

#ifndef NOSPLASH

void splash_screen_set_label(gchar *label) {
	static struct timespec const req = { 0, 10000000};
#ifdef DEBUG
	g_print("Setting splash label to %s\n", label);
#endif
	gtk_label_set(GTK_LABEL(splashscreen.label),label);
	flush_queue();
	nanosleep(&req, NULL);
}

GtkWidget *start_splash_screen() {
	static struct timespec const req = { 0, 100000000};
	GtkWidget *image, *vbox;
	GdkColor color;

	splashscreen.window = window_with_title(CURRENT_VERSION_NAME, GTK_WIN_POS_CENTER_ALWAYS, 0);
	gtk_window_set_decorated(GTK_WINDOW(splashscreen.window), FALSE);
	gtk_window_set_role(GTK_WINDOW(splashscreen.window), "splash");
	gtk_window_set_resizable(GTK_WINDOW(splashscreen.window),FALSE);
	color.red = 65535;
	color.blue = 65535;
	color.green = 65535;
	gtk_widget_modify_bg(splashscreen.window, GTK_STATE_NORMAL,&color);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(splashscreen.window), vbox);
	gtk_widget_show(vbox);
	splashscreen.label = gtk_label_new(_("starting bluefish"));
	gtk_box_pack_end(GTK_BOX(vbox),splashscreen.label , FALSE, FALSE, 0);
	gtk_widget_show(splashscreen.label);
	{
		GError *error=NULL;
		GdkPixbuf* pixbuf= gdk_pixbuf_new_from_file(BLUEFISH_SPLASH_FILENAME,&error);
		if (error) {
			g_print("ERROR while loading splash screen: %s\n", error->message);
			g_error_free(error);
		} else if (pixbuf) {
			image = gtk_image_new_from_pixbuf(pixbuf);
			gtk_box_pack_end(GTK_BOX(vbox), image, FALSE, FALSE, 0);
			g_object_unref(pixbuf);
			gtk_widget_show(image);
		}
	}

	gtk_widget_show(splashscreen.window);
	flush_queue();
	DEBUG_MSG("start_splash_screen, should be visible\n");
	nanosleep(&req, NULL);
	return splashscreen.window;
}
#endif /* #ifndef NOSPLASH */
void gui_set_main_toolbar_visible(Tbfwin *bfwin, gboolean visible, gboolean sync_menu) {
	if (sync_menu) {
		DEBUG_MSG("gui_set_main_toolbar_visible, trying to sync menu\n");
		setup_toggle_item_from_widget(bfwin->menubar, "/View/View Main Toolbar", visible);
	}
	if (gtk_container_children(GTK_CONTAINER(bfwin->main_toolbar_hb)) == NULL) {
		make_main_toolbar(bfwin);
	}
	widget_set_visible(bfwin->main_toolbar_hb,visible);
}
void gui_set_custom_menu_visible(Tbfwin *bfwin, gboolean visible, gboolean sync_menu) {
	if (sync_menu) {
		DEBUG_MSG("gui_set_custom_menu_visible, trying to sync menu\n");
		setup_toggle_item_from_widget(bfwin->menubar, "/View/View Custom Menu", visible);
	}
	if (gtk_container_children(GTK_CONTAINER(bfwin->custom_menu_hb)) == NULL) {
		make_cust_menubar(bfwin,bfwin->custom_menu_hb);
	}
	widget_set_visible(bfwin->custom_menu_hb,visible);
}

void gui_toggle_hidewidget_cb(Tbfwin *bfwin,guint action,GtkWidget *widget) {
	gboolean active = GTK_CHECK_MENU_ITEM(widget)->active;
	DEBUG_MSG("gui_toggle_hidewidget_cb, action=%d, active=%d\n",action,active);
	switch (action) {
	case 1:
		bfwin->session->view_main_toolbar = active;
		gui_set_main_toolbar_visible(bfwin, active, FALSE);
	break;
	case 3:
		bfwin->session->view_custom_menu = active;
		gui_set_custom_menu_visible(bfwin, active, FALSE);
	break;
	case 4:
		bfwin->session->view_left_panel = active;
		left_panel_show_hide_toggle(bfwin,FALSE, active, FALSE);
	break;
	default:
		g_print("gui_toggle_hidewidget_cb should NEVER be called with action %d\n", action);
		exit(1);
	break;
	}
}

Tbfwin *gui_new_window(GList *filenames, Tproject *project) {
	Tbfwin *bfwin = g_new0(Tbfwin,1);
	if (project) {
		bfwin->project = project;
		bfwin->session = project->session;
		bfwin->bmarkdata = project->bmarkdata;
		DEBUG_MSG("gui_new_window, bfwin=%p, bfwin->session=%p, (from project %p)\n",bfwin,bfwin->session,project);
	} else {
		bfwin->session = main_v->session;
		bfwin->bmarkdata = main_v->bmarkdata;
		DEBUG_MSG("gui_new_window, bfwin=%p, (from main_v)\n",bfwin);
	}
	gui_create_main(bfwin,filenames);
	/* never comment this out again Jim! */
	main_v->bfwinlist = g_list_append(main_v->bfwinlist, bfwin);
	gui_show_main(bfwin);
	return bfwin;
}

void gui_window_menu_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget) {
	switch (callback_action) {
		case 1:
			gui_new_window(NULL, NULL);
		break;
		case 2: /* close the window */
			if (main_window_delete_event_lcb(NULL, NULL, bfwin) == FALSE) {
				gtk_widget_destroy(bfwin->main_window);
			}
		break;
	}
}
