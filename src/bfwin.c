/*
 * Bluefish HTML Editor
 * bfwin.c
 *
 * Copyright (C) 2002-2013 Olivier Sessink
 * Copyright (C) 2011 James Hayward
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*#define DEBUG*/

#ifdef MAC_INTEGRATION
#include <gtkosxapplication.h>
#include <gtk-mac-menu.h>
#endif

#include <string.h>

#include "config.h"

#include "bfwin.h"
#include "bf_lib.h"
#include "bfwin_uimanager.h"
#include "bookmark.h"
#include "dialog_utils.h"
#include "document.h"
#include "filebrowser2.h"
#include "file_dialogs.h"
#include "gtk_easy.h"			/* widget_set_visible() */
#include "outputbox.h"
#include "pixmap.h"
#include "plugins.h"
#include "project.h"
#include "rcfile.h"
#include "snr3.h"

#ifdef IDENTSTORING
#include "bftextview2_identifier.h"
#endif /* IDENTSTORING */

#ifdef HAVE_LIBENCHANT
#include "bftextview2_spell.h"
#endif /*HAVE_LIBENCHANT*/


enum {
	TARGET_URI_LIST,
	TARGET_STRING
} Tdnd_types;

void
bfwin_fullscreen_toggle(Tbfwin * bfwin, gboolean active)
{
	if (active) {
		gtk_window_fullscreen(GTK_WINDOW(bfwin->main_window));
	} else {
		gtk_window_unfullscreen(GTK_WINDOW(bfwin->main_window));
	}
}

static void
notebook_move(Tbfwin * bfwin, gboolean move_left)
{
	gint curpos, newpos;

	if (!bfwin->current_document)
		return;

	curpos = gtk_notebook_page_num(GTK_NOTEBOOK(bfwin->notebook), bfwin->current_document->vsplit);
	if (curpos != -1) {
#ifdef DEVELOPMENT
		{
			GList *cur;
			cur = g_list_nth(bfwin->documentlist, curpos);
			g_return_if_fail(cur);
		}
#endif
		newpos = curpos + ((move_left) ? -1 : 1);
		DEBUG_MSG("notebook_move, cur=%d, new=%d (num_pages=%d)\n", curpos, newpos,
				  gtk_notebook_get_n_pages(GTK_NOTEBOOK(bfwin->notebook)));
		if (newpos >= 0 && newpos < gtk_notebook_get_n_pages(GTK_NOTEBOOK(bfwin->notebook)))
			gtk_notebook_reorder_child(GTK_NOTEBOOK(bfwin->notebook), bfwin->current_document->vsplit,
									   newpos);
	}
}

void
bfwin_notebook_switch(Tbfwin * bfwin, guint action)
{
	DEBUG_MSG("bfwin_notebook_switch, action=%d\n", action);
	switch (action) {
	case 1:
		gtk_notebook_prev_page(GTK_NOTEBOOK(bfwin->notebook));
		break;
	case 2:
		gtk_notebook_next_page(GTK_NOTEBOOK(bfwin->notebook));
		break;
	case 3:
		gtk_notebook_set_current_page(GTK_NOTEBOOK(bfwin->notebook), 0);
		break;
	case 4:
		gtk_notebook_set_current_page(GTK_NOTEBOOK(bfwin->notebook), -1);
		break;
	case 5:
		notebook_move(bfwin, TRUE);
		break;
	case 6:
		notebook_move(bfwin, FALSE);
		break;
	case 7:
		g_print("bfwin_notebook_switch, should not be called for 'recent'\n");
		break;
	}
}

void
bfwin_set_main_toolbar_visible(Tbfwin * bfwin, gboolean visible, gboolean sync_menu)
{
	if (!bfwin->main_toolbar_hb)
		return;
	if (sync_menu) {
		DEBUG_MSG("bfwin_set_main_toolbar_visible, trying to sync menu\n");
		bfwin_set_menu_toggle_item_from_path(bfwin->uimanager, "/MainMenu/ViewMenu/ViewMainToolbar", visible);
	}
	/*if (gtk_bin_get_child(GTK_BIN(bfwin->main_toolbar_hb)) == NULL)
		bfwin_main_toolbar_init(bfwin);*/

	widget_set_visible(bfwin->main_toolbar_hb, visible);
}

void
bfwin_main_toolbar_show(Tbfwin * bfwin, gboolean active)
{
	bfwin->session->view_main_toolbar = active;
	bfwin_set_main_toolbar_visible(bfwin, active, FALSE);
}

void
bfwin_output_pane_show(Tbfwin * bfwin, gboolean active)
{
	if (active)
		outputbox(bfwin, NULL, 0, 0, 0, NULL);
	else
		outputbox_cleanup(bfwin);
}

static void
side_panel_cleanup(Tbfwin * bfwin)
{
	DEBUG_MSG("side_panel_cleanup called for bfwin %p\n", bfwin);
	bmark_cleanup(bfwin);
	fb2_cleanup(bfwin);
	if (main_v->sidepanel_destroygui) {
		GSList *tmplist = main_v->sidepanel_destroygui;
		while (tmplist) {
			void *(*func) () = tmplist->data;
			DEBUG_MSG("side_panel_cleanup, calling plugin sidepanel_destroygui func %p on bfwin %p\n", tmplist->data, bfwin);
			func(bfwin);
			tmplist = g_slist_next(tmplist);
		}
	}
}

static void
side_panel_notify_position(GObject * object, GParamSpec * pspec, gpointer data)
{
	Tbfwin *bfwin = BFWIN(data);
	gint position;

	g_object_get(object, pspec->name, &position, NULL);
	DEBUG_MSG("side_panel_notify_position, restore_dimensions=%d, new position=%d\n",
			  main_v->props.restore_dimensions, position);
	if (main_v->props.restore_dimensions) {
		if (main_v->props.left_panel_left) {
			main_v->globses.left_panel_width = position;
		} else {
			int w, h;
			gtk_window_get_size(GTK_WINDOW(bfwin->main_window), &w, &h);
			main_v->globses.left_panel_width = w - position;
		}
		DEBUG_MSG("side_panel_notify_position, side_panel_width=%d\n", main_v->globses.left_panel_width);
	}
	if (main_v->props.wrap_on_right_margin) {
		/* all docs need to recalculate the amount of whitespace on the right */
		g_print("side_panel_notify_position, recalc right margin\n");
		bfwin_alldoc_recalc_right_margin(bfwin);
	}
}

static void
side_panel_notebook_switch(GtkNotebook * notebook, gpointer * page, guint page_num, gpointer user_data)
{
	Tbfwin *bfwin = BFWIN(user_data);
	bfwin->session->leftpanel_active_tab = page_num;
}

static GtkWidget *
side_panel_build(Tbfwin * bfwin)
{
	GtkWidget *bmarks;
	GtkWidget *fb2g;

	bfwin->leftpanel_notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(bfwin->leftpanel_notebook), main_v->props.leftpanel_tabposition);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(bfwin->leftpanel_notebook), TRUE);
#if !GTK_CHECK_VERSION(3, 0, 0)
	gtk_notebook_set_tab_hborder(GTK_NOTEBOOK(bfwin->leftpanel_notebook), 0);
	gtk_notebook_set_tab_vborder(GTK_NOTEBOOK(bfwin->leftpanel_notebook), 0);
#else
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(bfwin->leftpanel_notebook), TRUE);
#endif
	gtk_notebook_set_show_border(GTK_NOTEBOOK(bfwin->leftpanel_notebook), FALSE);
	gtk_notebook_popup_enable(GTK_NOTEBOOK(bfwin->leftpanel_notebook));
	DEBUG_MSG("side_panel_build, building side panel for bfwin %p\n", bfwin);
	bmarks = bmark_gui(bfwin);
	fb2g = fb2_init(bfwin);
	fb2_update_settings_from_session(bfwin, NULL);
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(bfwin->leftpanel_notebook), fb2g, new_pixmap(105),
								  gtk_label_new(_("Filebrowser")));
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(bfwin->leftpanel_notebook), bmarks, new_pixmap(104),
								  gtk_label_new(_("Bookmarks")));

	if (main_v->sidepanel_initgui) {
		GSList *tmplist = main_v->sidepanel_initgui;
		while (tmplist) {
			void *(*func) () = tmplist->data;
			DEBUG_MSG("side_panel_build, calling plugin sidepanel_initgui func %p\n", tmplist->data);
			func(bfwin);
			tmplist = g_slist_next(tmplist);
		}
	}

	gtk_widget_show_all(bfwin->leftpanel_notebook);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(bfwin->leftpanel_notebook),
								  bfwin->session->leftpanel_active_tab);
	g_signal_connect(G_OBJECT(bfwin->leftpanel_notebook), "switch-page",
					 G_CALLBACK(side_panel_notebook_switch), bfwin);

	return bfwin->leftpanel_notebook;
}

/**
 * if there is a left panel, this function will rebuild all widgets
 */
void
bfwin_side_panel_rebuild(Tbfwin * bfwin)
{
	if (bfwin->hpane) {
		DEBUG_MSG("bfwin_side_panel_rebuild, destroying widgets for bfwin %p\n", bfwin);
		gtk_widget_destroy(bfwin->leftpanel_notebook);
		side_panel_cleanup(bfwin);
		DEBUG_MSG("bfwin_side_panel_rebuild, re-init\n");
		side_panel_build(bfwin);
#ifdef PLATFORM_DARWIN
		if (main_v->props.left_panel_left) {
			gtk_paned_pack1(GTK_PANED(bfwin->hpane), bfwin->leftpanel_notebook, FALSE, FALSE);
		} else {
			gtk_paned_pack2(GTK_PANED(bfwin->hpane), bfwin->leftpanel_notebook, FALSE, FALSE);
		}
#else
		if (main_v->props.left_panel_left) {
			gtk_paned_pack1(GTK_PANED(bfwin->hpane), bfwin->leftpanel_notebook, TRUE, TRUE);
		} else {
			gtk_paned_pack2(GTK_PANED(bfwin->hpane), bfwin->leftpanel_notebook, TRUE, TRUE);
		}
#endif
		gtk_widget_show_all(bfwin->leftpanel_notebook);
	}
}

gboolean
bfwin_side_panel_show_hide_toggle(Tbfwin * bfwin, gboolean first_time, gboolean show, gboolean sync_menu)
{
	if (!bfwin->middlebox || !bfwin->notebook_box)
		return FALSE;

	DEBUG_MSG("bfwin_side_panel_show_hide_toggle, bfwin=%p, first_time=%d, show=%d, sync_menu=%d\n", bfwin,
			  first_time, show, sync_menu);
	if (sync_menu) {
		DEBUG_MSG("bfwin_side_panel_show_hide_toggle, trying to sync menu\n");
		bfwin_set_menu_toggle_item_from_path(bfwin->uimanager, "/MainMenu/ViewMenu/ViewSidePane", show);
	}
	if (!first_time && ((show && bfwin->hpane) || (!show && bfwin->hpane == NULL))) {
		DEBUG_MSG
			("bfwin_side_panel_show_hide_toggle, retrurning!!, show=%d, bfwin->hpane=%p, first_time=%d\n",
			 show, bfwin->hpane, first_time);
		return FALSE;
	}

	if (!first_time) {
		g_object_ref(G_OBJECT(bfwin->notebook_box));
		if (show) {
			gtk_container_remove(GTK_CONTAINER(bfwin->middlebox), bfwin->notebook_box);
		} else {
			gtk_container_remove(GTK_CONTAINER(bfwin->hpane), bfwin->notebook_box);
			DEBUG_MSG("bfwin_side_panel_show_hide_toggle, destroy bfwin->hpane\n");
			gtk_widget_destroy(bfwin->hpane);
			side_panel_cleanup(bfwin);
			bfwin->hpane = NULL;
		}
	}
	if (show) {
#if GTK_CHECK_VERSION(3,0,0)
		bfwin->hpane = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
#else
		bfwin->hpane = gtk_hpaned_new();
#endif
		if (main_v->props.left_panel_left) {
			DEBUG_MSG("set paned position to %d (left)\n", main_v->globses.left_panel_width);
			gtk_paned_set_position(GTK_PANED(bfwin->hpane), main_v->globses.left_panel_width);
		} else {
			int w, h;
			gtk_window_get_size(GTK_WINDOW(bfwin->main_window), &w, &h);
			DEBUG_MSG("set paned position to %d (right)\n", w - main_v->globses.left_panel_width);
			gtk_paned_set_position(GTK_PANED(bfwin->hpane), w - main_v->globses.left_panel_width);
		}
		g_signal_connect(G_OBJECT(bfwin->hpane), "notify::position",
						 G_CALLBACK(side_panel_notify_position), bfwin);
		bfwin->leftpanel_notebook = side_panel_build(bfwin);
/* When building with gtk+3.6.4 for MacOSX there are severeal issues with resizing:
* 1. NSWindow does not have border, so resizing handle when pushed to the edge, gets inactive (whole window is resized insted of notebook).
* 2. Leftpanel_notebook is "pushed" out of screen instead of shrinking; scroll arrows never appear (notebook is set to be scrollable, 
* so they should appear).
* TODO investigate what causes left notebook "push" off-screen instead of cropping and how to set minimum size */			
#ifdef PLATFORM_DARWIN
		if (main_v->props.left_panel_left) { 
			gtk_paned_pack1(GTK_PANED(bfwin->hpane), bfwin->leftpanel_notebook, FALSE, FALSE);
			gtk_paned_pack2(GTK_PANED(bfwin->hpane), bfwin->notebook_box, TRUE, FALSE);
		} else {
			gtk_paned_pack1(GTK_PANED(bfwin->hpane), bfwin->notebook_box, TRUE, FALSE);
			gtk_paned_pack2(GTK_PANED(bfwin->hpane), bfwin->leftpanel_notebook, FALSE, FALSE);
		}
#else
		if (main_v->props.left_panel_left) {
			gtk_paned_pack1(GTK_PANED(bfwin->hpane), bfwin->leftpanel_notebook, FALSE, TRUE);
			gtk_paned_pack2(GTK_PANED(bfwin->hpane), bfwin->notebook_box, TRUE, TRUE);
		} else {
			gtk_paned_pack1(GTK_PANED(bfwin->hpane), bfwin->notebook_box, TRUE, TRUE);
			gtk_paned_pack2(GTK_PANED(bfwin->hpane), bfwin->leftpanel_notebook, FALSE, TRUE);
		}
#endif
		gtk_box_pack_start(GTK_BOX(bfwin->middlebox), bfwin->hpane, TRUE, TRUE, 0);
		gtk_widget_show(bfwin->hpane);
	} else {
		bfwin->hpane = NULL;
		bfwin->leftpanel_notebook = NULL;
		gtk_box_pack_start(GTK_BOX(bfwin->middlebox), bfwin->notebook_box, TRUE, TRUE, 0);
	}
	if (!first_time) {
		g_object_unref(G_OBJECT(bfwin->notebook_box));
	}

	return TRUE;
}

void
bfwin_side_panel_show(Tbfwin * bfwin, gboolean active)
{
	bfwin->session->view_left_panel = active;
	bfwin_side_panel_show_hide_toggle(bfwin, FALSE, active, FALSE);
}

void
bfwin_statusbar_show_hide_toggle(Tbfwin * bfwin, gboolean visible, gboolean sync_menu)
{
	if (!bfwin->statusbar)
		return;
	if (sync_menu)
		bfwin_set_menu_toggle_item_from_path(bfwin->uimanager, "/MainMenu/ViewMenu/ViewStatusbar", visible);

	widget_set_visible(bfwin->statusbar, visible);
	widget_set_visible(bfwin->statusbar_lncol, visible);
	widget_set_visible(bfwin->statusbar_insovr, visible);
	widget_set_visible(bfwin->statusbar_editmode, visible);
}

void
bfwin_statusbar_show(Tbfwin * bfwin, gboolean active)
{
	bfwin->session->view_statusbar = active;
	bfwin_statusbar_show_hide_toggle(bfwin, active, FALSE);
}

void
bfwin_apply_session(Tbfwin * bfwin, Tdocument *active_doc)
{
	bfwin_set_main_toolbar_visible(bfwin, bfwin->session->view_main_toolbar, TRUE);

	if (!bfwin_side_panel_show_hide_toggle(bfwin, FALSE, bfwin->session->view_left_panel, TRUE)) {
		if (bfwin->leftpanel_notebook) {
			gtk_notebook_set_current_page(GTK_NOTEBOOK(bfwin->leftpanel_notebook),
										  bfwin->session->leftpanel_active_tab);
		}
	}

	bfwin_statusbar_show_hide_toggle(bfwin, bfwin->session->view_statusbar, TRUE);

	fb2_update_settings_from_session(bfwin, active_doc);
	bfwin_recent_menu_create(bfwin, TRUE);

	if (bfwin->simplesearch_combo) {
		combobox_empty(bfwin->simplesearch_combo);
		combobox_fill(bfwin->simplesearch_combo, NULL, bfwin->session->searchlist);
	}
	/* force this session in the plugins */
	g_slist_foreach(main_v->plugins, bfplugins_enforce_session, bfwin);
}

void
bfwin_apply_settings(Tbfwin * bfwin)
{
	DEBUG_MSG("bfwin_apply_settings, started\n");
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(bfwin->notebook), main_v->props.document_tabposition);
	/* We don't want to set the tab position if the left panel is hidden */
	if (bfwin->leftpanel_notebook) {
		gtk_notebook_set_tab_pos(GTK_NOTEBOOK(bfwin->leftpanel_notebook),
								 main_v->props.leftpanel_tabposition);

		/* check if the left panel needs to move over to the right */
		if (main_v->props.left_panel_left
			&& bfwin->leftpanel_notebook == gtk_paned_get_child1(GTK_PANED(bfwin->hpane))) {
			DEBUG_MSG("bfwin_apply_settings, left panel is on the right location\n");
		} else {
			bfwin_side_panel_show_hide_toggle(bfwin, FALSE, FALSE, FALSE);
			bfwin_side_panel_show_hide_toggle(bfwin, FALSE, TRUE, FALSE);
		}
	}
}

static gboolean bfwin_window_state_event_idle_cb(gpointer data) {
	bfwin_alldoc_recalc_right_margin(data);
	return FALSE;
}

static gboolean bfwin_window_state_event(GtkWidget * widget, GdkEvent * revent, Tbfwin * bfwin)
{
	if (main_v->props.restore_dimensions) {
		if (gtk_widget_get_visible(bfwin->main_window)) {
			GdkEventWindowState *event = (GdkEventWindowState *) revent;
			DEBUG_MSG
				("bfwin_window_state_event, window state event, state=%d, globses_main_window_w=%d\n",
				 event->new_window_state, main_v->globses.main_window_w);
			if ((event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) && main_v->globses.main_window_w > 0) {
				main_v->globses.main_window_w = -1 * main_v->globses.main_window_w;	/* negative means it is maximized !! */
				DEBUG_MSG("bfwin_window_state_event, maximized!! storing width=%d\n",
						  main_v->globses.main_window_w);
			} else if (!(event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED)
					   && main_v->globses.main_window_w < 0) {
				main_v->globses.main_window_w = -1 * main_v->globses.main_window_w;	/* make it positive again */
				DEBUG_MSG("bfwin_window_state_event, NOT-maximized, storing width=%d\n",
						  main_v->globses.main_window_w);
			}
			if (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) {
				gboolean fullscreen = (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN);
				if (main_v->props.hide_bars_on_fullscreen) {
					widget_set_visible(bfwin->toolbarbox, !fullscreen);
					widget_set_visible(gtk_widget_get_parent(bfwin->statusbar), !fullscreen);
				}
				sync_fullscreen_toggle(bfwin, fullscreen);
			}
		}
	}

	if (main_v->props.wrap_on_right_margin && ((revent->type == GDK_WINDOW_STATE && (((GdkEventWindowState *)revent)->changed_mask & (GDK_WINDOW_STATE_FULLSCREEN | GDK_WINDOW_STATE_MAXIMIZED))))) {
		/* all docs need to recalculate the amount of whitespace on the right, but the width is not yet set,
		so do this in an idle callback */
		g_idle_add(bfwin_window_state_event_idle_cb, bfwin);
	}
	return FALSE;
}

static gboolean
bfwin_configure_event(GtkWidget * widget, GdkEvent * revent, Tbfwin * bfwin)
{
	if (main_v->props.restore_dimensions) {
		GdkEventConfigure *event = (GdkEventConfigure *) revent;
		DEBUG_MSG("bfwin_configure_event, configure event, got event->width=%d, event->height=%d\n",event->width,event->height);
		if (main_v->globses.main_window_w > 0) {
			main_v->globses.main_window_w = event->width;
			main_v->globses.main_window_h = event->height;
			DEBUG_MSG("bfwin_configure_event, not maximized, storing width=%d, height=%d\n",
					  main_v->globses.main_window_w, main_v->globses.main_window_h);
		}
	}
	if (main_v->props.wrap_on_right_margin) {
		/* all docs need to recalculate the amount of whitespace on the right */
		bfwin_alldoc_recalc_right_margin(bfwin);
	}
	return FALSE;
}

/* called *before the widget is destroyed, this function will destroy the widget */
void
bfwin_destroy_and_cleanup(Tbfwin *bfwin)
{
	GList *tmplist;
	DEBUG_MSG("bfwin_destroy_and_cleanup, started for %p\n", bfwin);

		/* all documents have to be freed for this window */
	tmplist = g_list_first(bfwin->documentlist);
	DEBUG_MSG("bfwin_cleanup, have %d documents in window %p\n", g_list_length(bfwin->documentlist),
			  bfwin);
	while (tmplist) {
		DEBUG_MSG("bfwin_cleanup closing doc=%p\n", tmplist->data);
		doc_destroy(DOCUMENT(tmplist->data), TRUE);
		/* no this is not an indefinite loop, because the documents remove themselves
		   from the documentlist, we remove the top document untill there are no documents
		   left */
		tmplist = g_list_first(bfwin->documentlist);
	}

	g_signal_handler_disconnect(bfwin->notebook, bfwin->notebook_switch_signal);
	if (bfwin->update_searchhistory_idle_id != 0) {
		g_source_remove(bfwin->update_searchhistory_idle_id);
	}

	gtk_widget_destroy(bfwin->main_window);
	DEBUG_MSG("bfwin_destroy_and_cleanup, main_window destroyed for bfwin %p\n", bfwin);

	/* the widgets are destroyed now, don't touch any widgets any more from now on */

	bfwin->statusbar = NULL;	/* make sure no new statusbar messages have to be popped */

	if (bfwin->statusbar_pop_id != 0) {
		g_source_remove(bfwin->statusbar_pop_id);
		bfwin->statusbar_pop_id = 0;
	}

#ifdef HAVE_LIBENCHANT
	unload_spell_dictionary(bfwin);
#endif
	DEBUG_MSG("bfwin_cleanup called for bfwin %p\n",bfwin);
	side_panel_cleanup(bfwin);
	outputbox_cleanup(bfwin);

	if (bfwin->notebook_changed_doc_activate_id != 0) {
		g_source_remove(bfwin->notebook_changed_doc_activate_id);
	}

	DEBUG_MSG("bfwin_cleanup, unref static actiongroups\n");
	g_object_unref(G_OBJECT(bfwin->uimanager));
	/*g_object_unref(G_OBJECT(bfwin->globalGroup));
	g_object_unref(G_OBJECT(bfwin->documentGroup));
	g_object_unref(G_OBJECT(bfwin->editGroup));
	g_object_unref(G_OBJECT(bfwin->findReplaceGroup));
	g_object_unref(G_OBJECT(bfwin->projectGroup));
	g_object_unref(G_OBJECT(bfwin->undoGroup));
	g_object_unref(G_OBJECT(bfwin->bookmarkGroup));
	g_object_unref(G_OBJECT(bfwin->filebrowserGroup)); invalid unref according to valgrind */

	DEBUG_MSG("unref dynamic templates actiongroups\n");
	g_object_unref(G_OBJECT(bfwin->templates_group));
	DEBUG_MSG("unref dynamic lang_mode actiongroups\n");
	g_object_unref(G_OBJECT(bfwin->lang_mode_group));
	DEBUG_MSG("unref dynamic commands actiongroups\n");
	g_object_unref(G_OBJECT(bfwin->commands_group));
	DEBUG_MSG("unref dynamicfilters  actiongroups\n");
	g_object_unref(G_OBJECT(bfwin->filters_group));
	DEBUG_MSG("unref dynamic outputbox actiongroups\n");
	g_object_unref(G_OBJECT(bfwin->outputbox_group));
	DEBUG_MSG("unref dynamicencodings  actiongroups\n");
	g_object_unref(G_OBJECT(bfwin->encodings_group));
	if (bfwin->fb2_filters_group) {
		DEBUG_MSG("unref dynamic fb2_filters actiongroups\n");
		g_object_unref(G_OBJECT(bfwin->fb2_filters_group));
	}
	DEBUG_MSG("bfwin_cleanup, finished unref actiongroups\n");
#ifdef IDENTSTORING
	bftextview2_identifier_hash_destroy(bfwin);
#endif

	DEBUG_MSG("bfwin_cleanup, going to free bfwin %p\n", bfwin);
	g_free(bfwin);
}

gboolean
bfwin_delete_event(GtkWidget * widget, GdkEvent * event, Tbfwin * bfwin)
{
	/*
	   If you return FALSE in the "delete_event" signal handler GTK will emit the "destroy" signal.
	   Returning TRUE means you handled the event, and it should not be further propagated

	   we always handle it, so we always return TRUE
	 */
	DEBUG_MSG("bfwin_delete_event, started for bfwin %p\n", bfwin);
	if (!bfwin->documentlist) {
		bfwin_destroy_and_cleanup(bfwin);
		return TRUE;
	}

	if (have_modified_documents(bfwin->documentlist)) {
		Tclose_mode retval = multiple_files_modified_dialog(bfwin);

		switch (retval) {
		case close_mode_per_file:
			DEBUG_MSG("bfwin_delete_event, per file\n");
			project_save_and_mark_closed(bfwin);
			if (choose_per_file(bfwin, TRUE)) {
				DEBUG_MSG("bfwin_delete_event, all saved or all closed, return TRUE\n");
				return TRUE;
			} else {
				DEBUG_MSG("bfwin_delete_event, cancelled!\n");
				if (bfwin->project)
					bfwin->project->close = FALSE;
				return TRUE;
			}
			break;
		case close_mode_close_all:
			DEBUG_MSG("bfwin_delete_event, close all\n");
			if (bfwin->project) {
				project_save_and_mark_closed(bfwin);
			}
			doc_close_multiple_backend(bfwin, TRUE, retval);
			/* the last document that closes should close the window, so return TRUE */
			return TRUE; /* this had FALSE the comment above sais it should return TRUE ??? */
			break;
		case close_mode_cancel:
			DEBUG_MSG("bfwin_delete_event, cancel\n");
			return TRUE;
			break;
		case close_mode_save_all:
		default:
			/* save all and close */
			project_save_and_mark_closed(bfwin);
			DEBUG_MSG("bfwin_delete_event, save all\n");
			doc_save_all_close(bfwin);
			return TRUE;
			break;
		}
	} else {
		DEBUG_MSG("bfwin_delete_event, nothing modified, close all\n");
		project_save_and_mark_closed(bfwin);
		doc_close_multiple_backend(bfwin, TRUE, close_mode_close_all);
		/* the last document that closes should close the window, so return TRUE */
		return TRUE;
	}

}
#ifdef MAC_INTEGRATION
gboolean
bfwin_osx_terminate_event(GtkWidget * widget, GdkEvent * event, Tbfwin * bfwin)
{
	/*
	   On OSX we need this simplified function since showing dialog window on NSApplicationBlockTermination callback
	   causes bluefish to crash. Probably this is bug in gtk-mac-integration. So, we save all modified documents
	   by default without asking if user really wants to do it.
	 */

	DEBUG_MSG("bfwin_osx_terminate_event, started for bfwin %p\n", bfwin);
	if (!bfwin->documentlist) {
		g_print("bfwin_osx_terminate_event, document list is empty\n");
		bfwin_destroy_and_cleanup(bfwin);
		return TRUE;
	}

	if (have_modified_documents(bfwin->documentlist)) {
			DEBUG_MSG("bfwin_osx_terminate_event, have modified documents\n");
			if (bfwin->project && bfwin->project->uri) {
			project_save_and_mark_closed(bfwin);
			}
			doc_save_all_close(bfwin);
	} else {
		DEBUG_MSG("bfwin_osx_terminate_event, nothing modified, close all\n");
		if (bfwin->project && bfwin->project->uri) {
		project_save_and_mark_closed(bfwin);
		}
		doc_close_multiple_backend(bfwin, TRUE, close_mode_close_all);
	}
	return TRUE;
}
#endif

void
bfwin_gotoline_from_clipboard(Tbfwin * bfwin)
{
	gchar *string;
	GtkClipboard *cb;
	DEBUG_MSG("bfwin_gotoline_from_clipboard, called!\n");
	if (!gtk_widget_get_visible(bfwin->gotoline_frame))
		gtk_widget_show(bfwin->gotoline_frame);

	cb = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	string = gtk_clipboard_wait_for_text(cb);
	if (string) {
		gtk_entry_set_text(GTK_ENTRY(bfwin->gotoline_entry),string);
		gtk_editable_select_region(GTK_EDITABLE(bfwin->gotoline_entry),0,-1);
		g_free(string);
	}
}

void
bfwin_gotoline_frame_show(Tbfwin * bfwin)
{
	if (!gtk_widget_get_visible(bfwin->gotoline_frame))
		gtk_widget_show(bfwin->gotoline_frame);

	gtk_widget_grab_focus(bfwin->gotoline_entry);
}

static void simplesearch_combo_entry_activated(gpointer widget, Tbfwin * bfwin);

void
bfwin_simplesearch_show(Tbfwin *bfwin)
{
	GtkTextIter itstart,itend;
	if (!gtk_widget_get_visible(bfwin->gotoline_frame))
		gtk_widget_show(bfwin->gotoline_frame);

	/* see if there is a selection within a single line */
	if (gtk_text_buffer_get_selection_bounds(bfwin->current_document->buffer, &itstart, &itend)) {
		if (gtk_text_iter_get_line(&itstart)==gtk_text_iter_get_line(&itend)) {
			gchar *tmpstr = gtk_text_buffer_get_text(bfwin->current_document->buffer,&itstart,&itend,TRUE);
			gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(bfwin->simplesearch_combo))),tmpstr);
			g_free(tmpstr);
			/* TODO: mark the current selection as the 'current' search result */
		}
	}

	simplesearch_combo_entry_activated(bfwin->simplesearch_regex, bfwin);
	gtk_widget_grab_focus(bfwin->simplesearch_combo);
	gtk_editable_select_region(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(bfwin->simplesearch_combo))),0,-1);
}

void
bfwin_simplesearch_from_clipboard(Tbfwin *bfwin)
{
	gchar *string;
	GtkClipboard *cb;
	DEBUG_MSG("bfwin_simplesearch_from_clipboard, called!\n");
	if (!gtk_widget_get_visible(bfwin->gotoline_frame))
		gtk_widget_show(bfwin->gotoline_frame);

	cb = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	string = gtk_clipboard_wait_for_text(cb);
	if (string) {
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(bfwin->simplesearch_combo))),string);
		gtk_editable_select_region(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(bfwin->simplesearch_combo))),0,-1);
		/* TODO: mark the current selection as the 'current' search result */
		g_free(string);
	}
	simplesearch_combo_entry_activated(bfwin->simplesearch_regex, bfwin);
	gtk_widget_grab_focus(bfwin->simplesearch_combo);
}

static void
gotoline_entry_changed(GtkEditable * editable, Tbfwin * bfwin)
{
	gchar *linestr;
	gint linenum;
	guint linecount;
	GtkTextIter iter;

	if (!bfwin->current_document)
		return;
	if (!gtk_widget_get_visible(bfwin->gotoline_frame))
		return;

	DEBUG_MSG("gotoline_entry_changed, called!\n");
	linestr = gtk_editable_get_chars(editable, 0, -1);
	linenum = get_int_from_string(linestr);
	g_free(linestr);

	linecount = gtk_text_buffer_get_line_count(GTK_TEXT_BUFFER(bfwin->current_document->buffer));

	if (linenum == -1)
		gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(bfwin->current_document->buffer), &iter);
	else if (linenum >= linecount)
		gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(bfwin->current_document->buffer), &iter);
	else
		gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(bfwin->current_document->buffer), &iter,
										 linenum - 1);

	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(bfwin->current_document->buffer), &iter);
	doc_scroll_to_cursor(bfwin->current_document);
}

static void
gotoline_entry_insert_text(GtkEditable * editable, gchar * text, gint length, gint * position, gpointer data)
{
	gunichar c;
	const gchar *p;
	const gchar *end;
	const gchar *next;

	p = text;
	end = text + length;

	if (p == end)
		return;

	while (p != end) {
		next = g_utf8_next_char(p);

		c = g_utf8_get_char(p);

		if (!g_unichar_isdigit(c)) {
			g_signal_stop_emission_by_name(editable, "insert_text");
			break;
		}

		p = next;
	}
}

void
bfwin_gotoline_search_bar_close(Tbfwin *bfwin, gboolean clean_entry_only)
{
	if (!clean_entry_only) {
		DEBUG_MSG("bfwin_gotoline_search_bar_close, called!\n");
		gtk_widget_hide(bfwin->gotoline_frame);

		if (bfwin->simplesearch_snr3run) {
			DEBUG_MSG("free simple search run %p\n", bfwin->simplesearch_snr3run);
			snr3run_free(bfwin->simplesearch_snr3run, TRUE);
			bfwin->simplesearch_snr3run = NULL;
		}
	}
	/*gtk_editable_delete_text(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(bfwin->simplesearch_combo))), 0, -1);*/
	g_signal_handlers_block_matched(bfwin->gotoline_entry,
									G_SIGNAL_MATCH_FUNC, 0, 0, NULL, gotoline_entry_changed, NULL);

	gtk_editable_delete_text(GTK_EDITABLE(bfwin->gotoline_entry), 0, -1);

	g_signal_handlers_unblock_matched(bfwin->gotoline_entry,
									  G_SIGNAL_MATCH_FUNC, 0, 0, NULL, gotoline_entry_changed, NULL);
}

static void
gotoline_close_button_clicked(GtkButton * button, Tbfwin * bfwin)
{
	DEBUG_MSG("gotoline_close_button_clicked, called!\n");
	bfwin_gotoline_search_bar_close(bfwin, FALSE);
	if (bfwin->current_document)
		gtk_widget_grab_focus(bfwin->current_document->view);
}

static gboolean
simpleasearch_update_search_history(gpointer data)
{
	DEBUG_MSG("simpleasearch_update_search_history, called!\n");
	if (BFWIN(data)->simplesearch_combo) {
		const gchar *tmpstr = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(BFWIN(data)->simplesearch_combo))));
		DEBUG_MSG("currenbt string=%s\n",tmpstr);
		combobox_empty(BFWIN(data)->simplesearch_combo);
		combobox_fill(BFWIN(data)->simplesearch_combo, tmpstr, BFWIN(data)->session->searchlist);
		BFWIN(data)->update_searchhistory_idle_id = 0;
	}
	return FALSE;
}

static gboolean
simplesearch_start(Tbfwin *bfwin, gboolean allow_single_char_search) {
	const gchar *tmpstr;
	gboolean retval=FALSE;

	if (!bfwin->current_document)
		return FALSE;
	tmpstr = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(bfwin->simplesearch_combo))));
	if (bfwin->simplesearch_snr3run) {
		DEBUG_MSG("free simple search run %p\n", bfwin->simplesearch_snr3run);
		snr3run_free(bfwin->simplesearch_snr3run, TRUE);
		bfwin->simplesearch_snr3run=NULL;
	}
	if (tmpstr && tmpstr[0]!='\0' && (allow_single_char_search || tmpstr[1] != '\0')) {
		gpointer before;
		bfwin->session->ssearch_regex = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(bfwin->simplesearch_regex));
		bfwin->session->ssearch_casesens = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(bfwin->simplesearch_casesens));
		bfwin->session->ssearch_dotmatchall = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(bfwin->simplesearch_dotmatchall));
		bfwin->session->ssearch_unescape = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(bfwin->simplesearch_unescape));
		g_free(bfwin->session->ssearch_text);
		bfwin->session->ssearch_text = g_strdup(tmpstr);
		DEBUG_MSG("start simple search run with %s\n",tmpstr);
		before = bfwin->session->searchlist;
		bfwin->simplesearch_snr3run = simple_search_run(bfwin, tmpstr
				, bfwin->session->ssearch_regex ? snr3type_pcre :snr3type_string
				, bfwin->session->ssearch_casesens
				, bfwin->session->ssearch_dotmatchall
				, bfwin->session->ssearch_unescape);
		if (before != bfwin->session->searchlist) {
			/* update the combo with the new history */
			if (bfwin->update_searchhistory_idle_id) {
				g_source_remove(bfwin->update_searchhistory_idle_id);
			}
			bfwin->update_searchhistory_idle_id = g_timeout_add_seconds(4, simpleasearch_update_search_history,bfwin);
		}
		retval = TRUE;
	}
	return retval;
}

static void
simplesearch_forward_clicked(GtkButton * button, Tbfwin * bfwin)
{
	if (!bfwin->simplesearch_snr3run && !simplesearch_start(bfwin, TRUE))
		return;
	snr3_run_go(bfwin->simplesearch_snr3run, TRUE);
}

static void
simplesearch_back_clicked(GtkButton * button, Tbfwin * bfwin)
{
	if (!bfwin->simplesearch_snr3run && !simplesearch_start(bfwin, TRUE))
		return;
	snr3_run_go(bfwin->simplesearch_snr3run, FALSE);
}

static void
simplesearch_combo_entry_changed(gpointer widget, Tbfwin * bfwin)
{
	if (!gtk_widget_get_visible(bfwin->gotoline_frame))
		return;
	DEBUG_MSG("simplesearch_combo_entry_changed, call simplesearch_start()\n");
	simplesearch_start(bfwin, FALSE);
}

static void
simplesearch_combo_entry_activated(gpointer widget, Tbfwin * bfwin)
{
	if (!gtk_widget_get_visible(bfwin->gotoline_frame))
		return;
	DEBUG_MSG("simplesearch_combo_entry_activated, call simplesearch_start()\n");
	simplesearch_forward_clicked(NULL, bfwin);
}

static void
simplesearch_option_toggled(gpointer widget, Tbfwin * bfwin)
{
	DEBUG_MSG("simplesearch_option_toggled, call simplesearch_start()\n");
	simplesearch_start(bfwin, FALSE);
	if (widget == bfwin->simplesearch_regex) {
		gboolean regex = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(bfwin->simplesearch_regex));
		gtk_widget_set_sensitive(bfwin->simplesearch_dotmatchall, regex);
		gtk_widget_set_sensitive(bfwin->simplesearch_unescape, !regex);
	}
}

static void
simplesearch_advanced_clicked(GtkButton * button, Tbfwin * bfwin)
{
	const gchar *tmpstr = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(bfwin->simplesearch_combo))));
	snr3_advanced_dialog(bfwin, tmpstr?tmpstr:"");
	gotoline_close_button_clicked(NULL, bfwin);
}

static gboolean
gotoline_entries_key_press_event(GtkWidget * widget, GdkEventKey * event, Tbfwin * bfwin)
{
	if (event->keyval == GDK_Escape) {
		gotoline_close_button_clicked(NULL, bfwin);
		return TRUE;
	}
	if (event->keyval == GDK_Return && widget == bfwin->gotoline_entry) {
		gotoline_entry_changed(GTK_EDITABLE(widget), bfwin);
		/* the close button clicked function automatically switches the focus to the text widget */
		gotoline_close_button_clicked(NULL, bfwin);
		return TRUE;
	}
	return FALSE;
}
static void
gotoline_frame_create(Tbfwin * bfwin)
{
	GtkWidget *button, *hbox;
	GtkWidget *menu;

	bfwin->gotoline_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(bfwin->gotoline_frame), GTK_SHADOW_NONE);
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 2);
	gtk_container_add(GTK_CONTAINER(bfwin->gotoline_frame), hbox);

	button = bluefish_small_close_button_new();
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 6);
	g_signal_connect(button, "clicked", G_CALLBACK(gotoline_close_button_clicked), bfwin);

	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Goto Line:")), FALSE, FALSE, 0);
	bfwin->gotoline_entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), bfwin->gotoline_entry, FALSE, FALSE, 0);
	g_signal_connect(bfwin->gotoline_entry, "changed", G_CALLBACK(gotoline_entry_changed), bfwin);
	g_signal_connect(bfwin->gotoline_entry, "insert-text", G_CALLBACK(gotoline_entry_insert_text), NULL);
	g_signal_connect(G_OBJECT(bfwin->gotoline_entry), "key-press-event", G_CALLBACK(gotoline_entries_key_press_event), bfwin);
#if (GTK_CHECK_VERSION(3,0,0))
	gtk_box_pack_start(GTK_BOX(hbox), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, 6);
#else
	gtk_box_pack_start(GTK_BOX(hbox), gtk_vseparator_new(), FALSE, FALSE, 6);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Find:")), FALSE, FALSE, 0);
	DEBUG_MSG("ssearch_text=%s\n",bfwin->session->ssearch_text);
	bfwin->simplesearch_combo = combobox_with_popdown(bfwin->session->ssearch_text, bfwin->session->searchlist, TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), bfwin->simplesearch_combo, FALSE, FALSE, 0);
	button = (GtkWidget *)gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(simplesearch_back_clicked), bfwin);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	button = (GtkWidget *)gtk_menu_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(simplesearch_forward_clicked), bfwin);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	menu = gtk_menu_new();
	bfwin->simplesearch_regex = gtk_check_menu_item_new_with_label(_("Regular expression (pcre)"));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(bfwin->simplesearch_regex), bfwin->session->ssearch_regex);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(bfwin->simplesearch_regex));
	bfwin->simplesearch_casesens = gtk_check_menu_item_new_with_label(_("Case sensitive"));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(bfwin->simplesearch_casesens), bfwin->session->ssearch_casesens);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(bfwin->simplesearch_casesens));
	bfwin->simplesearch_dotmatchall = gtk_check_menu_item_new_with_label(_("Dot character matches newlines"));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(bfwin->simplesearch_dotmatchall), bfwin->session->ssearch_dotmatchall);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(bfwin->simplesearch_dotmatchall));
	bfwin->simplesearch_unescape = gtk_check_menu_item_new_with_label(_("Pattern contains backslash escaped sequences"));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(bfwin->simplesearch_unescape), bfwin->session->ssearch_unescape);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(bfwin->simplesearch_unescape));
	gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(button), GTK_WIDGET(menu));
	gtk_widget_show_all(GTK_WIDGET(menu));
	g_signal_connect(gtk_bin_get_child(GTK_BIN(bfwin->simplesearch_combo)), "key-press-event", G_CALLBACK(gotoline_entries_key_press_event), bfwin);
	g_signal_connect(gtk_bin_get_child(GTK_BIN(bfwin->simplesearch_combo)), "changed", G_CALLBACK(simplesearch_combo_entry_changed), bfwin);
	g_signal_connect(gtk_bin_get_child(GTK_BIN(bfwin->simplesearch_combo)), "activate", G_CALLBACK(simplesearch_combo_entry_activated), bfwin);
	g_signal_connect(bfwin->simplesearch_regex, "toggled", G_CALLBACK(simplesearch_option_toggled), bfwin);
	g_signal_connect(bfwin->simplesearch_casesens, "toggled", G_CALLBACK(simplesearch_option_toggled), bfwin);
	g_signal_connect(bfwin->simplesearch_dotmatchall, "toggled", G_CALLBACK(simplesearch_option_toggled), bfwin);
	g_signal_connect(bfwin->simplesearch_unescape, "toggled", G_CALLBACK(simplesearch_option_toggled), bfwin);
	button = (GtkWidget *)gtk_tool_button_new(NULL,_("Advanced"));
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(simplesearch_advanced_clicked),bfwin);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(bfwin->notebook_box), bfwin->gotoline_frame, FALSE, FALSE, 2);
	gtk_widget_show_all(hbox);
}

static void
bfwin_destroy_event(GtkWidget * widget, Tbfwin * bfwin)
{
	DEBUG_MSG("bfwin_destroy_event, started for bfwin=%p, first cleanup any project (%p)\n", bfwin,
			  bfwin->project);
	if (bfwin->project) {
		project_final_close(bfwin, TRUE);
		bfwin->session = NULL;
	}
	main_v->bfwinlist = g_list_remove(main_v->bfwinlist, bfwin);
	DEBUG_MSG("bfwin_destroy_event, bfwin(%p) is removed from bfwinlist\n", bfwin);
#ifndef MAC_INTEGRATION
	if (NULL == main_v->bfwinlist) {
		rcfile_save_global_session();
		gtk_main_quit();
	}
#else
	if (NULL == main_v->bfwinlist) {
		if (main_v->osx_status == 0 ) {
			main_v->osx_status = 2;
			bfwin_window_new();
			return;
		}
		if (main_v->osx_status == 1 ) {
			rcfile_save_global_session();
			gtk_main_quit();
			return;
		}
	}
#endif
}

static void
bfwin_on_drag_data(GtkWidget * widget, GdkDragContext * context, gint x, gint y,
				   GtkSelectionData * data, guint info, guint time, Tbfwin * bfwin)
{
	int mode = 0;
	gchar *stringdata;

	if ((gtk_selection_data_get_length(data) == 0) || (gtk_selection_data_get_format(data) != 8)
		|| ((info != TARGET_STRING) && (info != TARGET_URI_LIST))) {
		DEBUG_MSG("bfwin_on_drag_data, currently unknown DnD object, need to do string comparision\n");
		gtk_drag_finish(context, FALSE, TRUE, time);
		return;
	}
	stringdata = g_strndup((gchar *) gtk_selection_data_get_data(data), gtk_selection_data_get_length(data));
	if (strchr(stringdata, '\n')) {
		gchar **arr, **tmp;
		tmp = arr = g_strsplit(stringdata, "\n", -1);
		while (*tmp) {
			gchar *stripped = g_strstrip(*tmp);
			if (stripped[0] != '\0') {
				doc_new_from_input(bfwin, stripped, FALSE, FALSE, -1);
			}
			tmp++;
		}
		g_strfreev(arr);
	} else {
		doc_new_from_input(bfwin, stringdata, FALSE, FALSE, -1);
	}
	g_free(stringdata);
	gtk_drag_finish(context, TRUE, (mode == GDK_ACTION_COPY), time);
}

static void
notebook_switch_by_key(GtkWidget * widget, gpointer data)
{
	DEBUG_MSG("notebook_switch_by_key, gtk_notebook_set_current_page\n");
	gtk_notebook_set_current_page(GTK_NOTEBOOK(widget), GPOINTER_TO_INT(data) - 1);
}

void
notebook_unbind_tab_signals(Tbfwin * bfwin)
{
	g_signal_handlers_disconnect_matched(bfwin->notebook, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, notebook_switch_by_key, NULL);
}

void
notebook_bind_tab_signals(Tbfwin * bfwin)
{
	DEBUG_MSG("notebook_bind_signals, connect <Alt>X trigered events\n");
	g_signal_connect(G_OBJECT(bfwin->notebook), "tab-last", G_CALLBACK(notebook_switch_by_key),
					 (gpointer) - 1);
	g_signal_connect(G_OBJECT(bfwin->notebook), "tab-first", G_CALLBACK(notebook_switch_by_key),
					 (gpointer) 1);
	g_signal_connect(G_OBJECT(bfwin->notebook), "tab2", G_CALLBACK(notebook_switch_by_key), (gpointer) 2);
	g_signal_connect(G_OBJECT(bfwin->notebook), "tab3", G_CALLBACK(notebook_switch_by_key), (gpointer) 3);
	g_signal_connect(G_OBJECT(bfwin->notebook), "tab4", G_CALLBACK(notebook_switch_by_key), (gpointer) 4);
	g_signal_connect(G_OBJECT(bfwin->notebook), "tab5", G_CALLBACK(notebook_switch_by_key), (gpointer) 5);
	g_signal_connect(G_OBJECT(bfwin->notebook), "tab6", G_CALLBACK(notebook_switch_by_key), (gpointer) 6);
	g_signal_connect(G_OBJECT(bfwin->notebook), "tab7", G_CALLBACK(notebook_switch_by_key), (gpointer) 7);
	g_signal_connect(G_OBJECT(bfwin->notebook), "tab8", G_CALLBACK(notebook_switch_by_key), (gpointer) 8);
	g_signal_connect(G_OBJECT(bfwin->notebook), "tab9", G_CALLBACK(notebook_switch_by_key), (gpointer) 9);
}

void
notebook_set_tab_accels(Tbfwin * bfwin)
{
	static gsize create_signals_init = 0;
	GtkAccelGroup *tab_accels;

	if (g_once_init_enter(&create_signals_init)) {
		DEBUG_MSG("notebook_set_tab_accels, g_signal_new for <Alt>X triggered events\n");
		g_signal_new("tab-last", gtk_widget_get_type(), G_SIGNAL_ACTION, 0, NULL, NULL,
					 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
		g_signal_new("tab-first", gtk_widget_get_type(), G_SIGNAL_ACTION, 0, NULL, NULL,
					 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
		g_signal_new("tab2", gtk_widget_get_type(), G_SIGNAL_ACTION, 0, NULL, NULL,
					 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
		g_signal_new("tab3", gtk_widget_get_type(), G_SIGNAL_ACTION, 0, NULL, NULL,
					 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
		g_signal_new("tab4", gtk_widget_get_type(), G_SIGNAL_ACTION, 0, NULL, NULL,
					 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
		g_signal_new("tab5", gtk_widget_get_type(), G_SIGNAL_ACTION, 0, NULL, NULL,
					 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
		g_signal_new("tab6", gtk_widget_get_type(), G_SIGNAL_ACTION, 0, NULL, NULL,
					 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
		g_signal_new("tab7", gtk_widget_get_type(), G_SIGNAL_ACTION, 0, NULL, NULL,
					 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
		g_signal_new("tab8", gtk_widget_get_type(), G_SIGNAL_ACTION, 0, NULL, NULL,
					 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
		g_signal_new("tab9", gtk_widget_get_type(), G_SIGNAL_ACTION, 0, NULL, NULL,
					 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
		g_once_init_leave(&create_signals_init, TRUE);
	}

	tab_accels = gtk_accel_group_new();
	DEBUG_MSG("notebook_set_tab_accels, gtk_window_add_accel_group\n");
	gtk_window_add_accel_group(GTK_WINDOW(bfwin->main_window), tab_accels);
	DEBUG_MSG("notebook_set_tab_accels, gtk_widget_add_accelerator\n");
	gtk_widget_add_accelerator(bfwin->notebook, "tab-last", tab_accels, GDK_0, GDK_MOD1_MASK,
							   GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(bfwin->notebook, "tab-first", tab_accels, GDK_1, GDK_MOD1_MASK,
							   GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(bfwin->notebook, "tab2", tab_accels, GDK_2, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(bfwin->notebook, "tab3", tab_accels, GDK_3, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(bfwin->notebook, "tab4", tab_accels, GDK_4, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(bfwin->notebook, "tab5", tab_accels, GDK_5, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(bfwin->notebook, "tab6", tab_accels, GDK_6, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(bfwin->notebook, "tab7", tab_accels, GDK_7, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(bfwin->notebook, "tab8", tab_accels, GDK_8, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(bfwin->notebook, "tab9", tab_accels, GDK_9, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
}

static gboolean
notebook_changed_activate_current_document(gpointer data)
{
	DEBUG_SIG("notebook_changed_activate_current_document, priority=%d\n",NOTEBOOKCHANGED_DOCACTIVATE_PRIORITY);
	DEBUG_MSG("notebook_changed_activate_current_document, current_document=%p\n",BFWIN(data)->current_document);
	if (BFWIN(data)->current_document)
		doc_activate(BFWIN(data)->current_document);

	BFWIN(data)->notebook_changed_doc_activate_id = 0;
	DEBUG_MSG("notebook_changed_doc_activate_id=%d\n",BFWIN(data)->notebook_changed_doc_activate_id);
	return FALSE;
}

void
bfwin_notebook_block_signals(Tbfwin * bfwin)
{
	g_signal_handler_block(G_OBJECT(bfwin->notebook), bfwin->notebook_switch_signal);
}

void
bfwin_notebook_unblock_signals(Tbfwin * bfwin)
{
	g_signal_handler_unblock(G_OBJECT(bfwin->notebook), bfwin->notebook_switch_signal);
}

static void
bfwin_current_document_changed_notify(Tbfwin *bfwin, Tdocument *olddoc, Tdocument *newdoc) {
	GSList *tmpslist;
	for (tmpslist=bfwin->curdoc_changed;tmpslist;tmpslist=g_slist_next(tmpslist)) {
		Tcallback *cb=tmpslist->data;
		DEBUG_MSG("bfwin_current_document_changed_notify, call %p, data=%p, bfwin=%p, olddoc=%p, newdoc=%p\n",cb->func,cb->data, bfwin,olddoc,newdoc);
		((CurdocChangedCallback)cb->func)(bfwin, olddoc, newdoc, cb->data);
	}
}

void
bfwin_current_document_change_register(Tbfwin *bfwin, CurdocChangedCallback func, gpointer data) {
	callback_register(&bfwin->curdoc_changed, func, data);
}

void
bfwin_current_document_change_remove_by_data(Tbfwin *bfwin, gpointer data) {
	callback_remove_by_data(&bfwin->curdoc_changed, data);
}

void
bfwin_current_document_change_remove_all(Tbfwin *bfwin) {
	callback_remove_all(&bfwin->curdoc_changed);
}

void
bfwin_document_insert_text_register(Tbfwin *bfwin, DocInsertTextCallback func, gpointer data) {
	callback_register(&bfwin->doc_insert_text, func, data);
}

void
bfwin_document_insert_text_remove_by_data(Tbfwin *bfwin, gpointer data) {
	callback_remove_by_data(&bfwin->doc_insert_text, data);
}

void
bfwin_document_delete_range_register(Tbfwin *bfwin, DocDeleteRangeCallback func, gpointer data) {
	callback_register(&bfwin->doc_delete_range, func, data);
}

void
bfwin_document_delete_range_remove_by_data(Tbfwin *bfwin, gpointer data) {
	callback_remove_by_data(&bfwin->doc_delete_range, data);
}

void
bfwin_document_destroy_register(Tbfwin *bfwin, DocDestroyCallback func, gpointer data)
{
	callback_register(&bfwin->doc_destroy, func, data);
}
void
bfwin_document_destroy_remove_by_data(Tbfwin *bfwin, gpointer data)
{
	callback_remove_by_data(&bfwin->doc_destroy, data);
}

void
bfwin_notebook_changed(Tbfwin * bfwin, gint newpage)
{
	gint cur = newpage;
	gint doclistlen;
	Tdocument *prev_document;
	DEBUG_MSG
		("bfwin_notebook_changed, doclistlen=%d, newpage=%d, notebook_curpage=%d, last_notebook_page=%d, curdoc=%p\n",
		 g_list_length(bfwin->documentlist)
		 , newpage, gtk_notebook_get_current_page(GTK_NOTEBOOK(bfwin->notebook))
		 , bfwin->last_notebook_page, bfwin->current_document);
	if (newpage == -1) {
		/* this returns -1 if there is no current page */
		cur = gtk_notebook_get_current_page(GTK_NOTEBOOK(bfwin->notebook));
	}
	if ((bfwin->last_notebook_page == cur)
		&& (bfwin->current_document != NULL)
		&& (bfwin->current_document == g_list_nth_data(bfwin->documentlist, cur))) {
		if (bfwin->project && bfwin->project->close)
			project_final_close(bfwin, FALSE);
		DEBUG_MSG
			("bfwin_notebook_changed, NOT CHANGED cur=%d, documentlist[cur]==current_document (=%p), RETURNING\n",
			 cur, bfwin->current_document);
		return;
	}
	doclistlen = g_list_length(bfwin->documentlist);
	if (cur == -1) {
		if (doclistlen > 0) {
			DEBUG_MSG("bfwin_notebook_changed, WEIRD 1 cur=%d, but doclistlen=%d RETURNING\n", cur,
					  doclistlen);
			bfwin->last_notebook_page = -2;
			return;
		}
	}
	if (doclistlen == 0) {
		DEBUG_MSG("bfwin_notebook_changed, doclistlen=%d, before doc_new()!\n", doclistlen);
		if (bfwin->project && bfwin->project->close)
			project_final_close(bfwin, FALSE);
		bfwin->current_document = doc_new(bfwin, TRUE);
		bfwin_current_document_changed_notify(bfwin, NULL, bfwin->current_document);
		bfwin->last_notebook_page = 1;
		DEBUG_MSG("bfwin_notebook_changed, after doc_new(), returning\n");
		return;
	}
	/* if the documentlist has length 1, cur should not be larger then 0, if 2, cur should not be larger then 1, etc. */
	if (cur >= doclistlen) {
		DEBUG_MSG("bfwin_notebook_changed, DOCALREADYCLOSED, cur=%d, doclistlen=%d, RETURNING\n", cur,
				  doclistlen);
		bfwin->last_notebook_page = -2;
		return;
	}
	if (bfwin->current_document) {
		/*g_print("notebook_changed, set enable_scanner to FALSE for doc %p\n",bfwin->current_document); */
		BLUEFISH_TEXT_VIEW(bfwin->current_document->view)->enable_scanner = FALSE;
	}
	prev_document = bfwin->current_document;
	bfwin->current_document = g_list_nth_data(bfwin->documentlist, cur);
	if (bfwin->current_document == NULL) {
		DEBUG_MSG("bfwin_notebook_changed, WEIRD 2, doclist[%d] == NULL, RETURNING\n", cur);
		return;
	}
	bfwin_current_document_changed_notify(bfwin, prev_document, bfwin->current_document);
	
	bfwin->last_notebook_page = cur;
	DEBUG_MSG("bfwin_notebook_changed, current_document=%p, idle callback for doc activate = %d\n",
			  bfwin->current_document, bfwin->notebook_changed_doc_activate_id);
	/* slightly lower than default priority so we make sure any events are handled first */
	if (bfwin->notebook_changed_doc_activate_id == 0) {
		bfwin->notebook_changed_doc_activate_id =
			g_idle_add_full(NOTEBOOKCHANGED_DOCACTIVATE_PRIORITY, notebook_changed_activate_current_document,
							bfwin, NULL);
	}
}

/*
child : 	the child GtkWidget affected
page_num : 	the new page number for child
*/
static void
notebook_reordered(GtkNotebook * notebook, GtkWidget * child, guint page_num, gpointer user_data)
{
	Tbfwin *bfwin = BFWIN(user_data);
	Tdocument *doc = NULL;
	GList *tmplist = g_list_first(bfwin->documentlist);

	DEBUG_MSG("notebook_reordered, started\n");
	/* look where this child (the GtkVPaned) is in the documentlist */
	while (tmplist) {
		if (DOCUMENT(tmplist->data)->vsplit == child) {
			doc = DOCUMENT(tmplist->data);
			break;
		}
		tmplist = g_list_next(tmplist);
	}
	if (!doc)
		return;
	bfwin->documentlist = g_list_remove(bfwin->documentlist, doc);
	DEBUG_MSG("notebook_reordered, moving doc %p to position %d in the documentlist\n", doc, page_num);
	bfwin->documentlist = g_list_insert(bfwin->documentlist, doc, page_num);
	DEBUG_MSG("notebook_reordered, done\n");
}

/* Restores tab scrolling feature for gtk+3 builds */
#if GTK_CHECK_VERSION (3, 0, 0)
static gboolean
notebook_scroll_event_lcb (GtkNotebook * notebook, GdkEventScroll *event, gpointer user_data)
{
	DEBUG_MSG("notebook_scroll_event, started for notebook=%p\n", notebook);

	/* Some sanity check, which is required when document does not have scroll bar and scroll-events are not consumed by textview*/
	GtkWidget *child, *event_widget;
	child = gtk_notebook_get_nth_page (notebook, gtk_notebook_get_current_page (notebook));
	if (child == NULL)
		return FALSE;
	event_widget = gtk_get_event_widget ((GdkEvent *) event);
	if (event_widget == NULL || event_widget == child || gtk_widget_is_ancestor (event_widget, child))
		return FALSE; 

	switch (event->direction) {
		case GDK_SCROLL_RIGHT:
		case GDK_SCROLL_DOWN:
			gtk_notebook_next_page (notebook);
			break;
		case GDK_SCROLL_LEFT:
		case GDK_SCROLL_UP:
			gtk_notebook_prev_page (notebook);
			break;
		default:
			break;
	}

	return TRUE;
}
#endif

static void
notebook_switch_page(GtkWidget * notebook, gpointer * page, gint page_num, Tbfwin * bfwin)
{
	DEBUG_MSG("notebook_switch_page_lcb, page=%d\n", page_num);
	bfwin_notebook_changed(bfwin, page_num);
}

static void
notebook_connect_signals(Tbfwin *bfwin)
{
	/* We have to know when the notebook changes */
	if (bfwin->notebook_switch_signal == 0) {
	bfwin->notebook_switch_signal =
		g_signal_connect_after(G_OBJECT(bfwin->notebook), "switch-page", G_CALLBACK(notebook_switch_page),
							   bfwin);
	}
}

static void
notebook_disconnect_signals(Tbfwin *bfwin)
{
	if (bfwin->notebook_switch_signal > 0) {
		g_signal_handler_disconnect(bfwin->notebook, bfwin->notebook_switch_signal);
		bfwin->notebook_switch_signal = 0;
	}
}

static gboolean
bfwin_key_press_event(GtkWidget *widget,GdkEventKey  *kevent,gpointer   user_data)
{
	/*g_print("bfwin_key_press_event, key=%d, state=%d\n",kevent->keyval,kevent->state);*/
	if (kevent->keyval == GDK_Tab && (kevent->state & GDK_CONTROL_MASK)) {
		gint i;
		Tdocument *doc;
		GList *tmplist;
		DEBUG_MSG("bfwin_key_press_event, control tab pressed\n");
		/* switch to the next recent document, without activating it */
		i = gtk_notebook_get_current_page(GTK_NOTEBOOK(BFWIN(user_data)->notebook));
		/* we cannot use the bfwin->current_document because if we tab multiple documents
		with control-tab the current notebook page and the current document might be out
		of sync */
		doc = g_list_nth_data(BFWIN(user_data)->documentlist, i);
		if (!doc)
			return FALSE;
		tmplist = g_list_next(doc->recentpos);
		if (!tmplist)
			return FALSE;
		i = g_list_index(BFWIN(user_data)->documentlist, tmplist->data);
		if (i==-1)
			return FALSE;
		DEBUG_MSG("bfwin_key_press_event, set notebook page %d\n", i);
		notebook_disconnect_signals(BFWIN(user_data));
		gtk_notebook_set_current_page(GTK_NOTEBOOK(BFWIN(user_data)->notebook), i);
		return TRUE;
	}
	/*if (kevent->keyval == GDK_Control_L || kevent->keyval == GDK_Control_R) {
		g_print("control pressed\n");
	}*/
	return FALSE;
}

static gboolean
bfwin_key_release_event(GtkWidget *widget, GdkEventKey  *kevent, gpointer   user_data)
{
	/*g_print("bfwin_key_release_event, key=%d, state=%d\n",kevent->keyval,kevent->state);*/
	/*if (kevent->keyval == GDK_Tab && kevent->state & GDK_CONTROL_MASK) {
		g_print("control tab released\n");
	}*/
	if (kevent->keyval == GDK_Control_L || kevent->keyval == GDK_Control_R) {
		/* if we did switch to a recent document without activating it, we should activate it now */
		DEBUG_MSG("bfwin_key_release_event, control released\n");
		if (BFWIN(user_data)->notebook_switch_signal == 0) {
			gint i;
			notebook_connect_signals(BFWIN(user_data));
			i = gtk_notebook_get_current_page(GTK_NOTEBOOK(BFWIN(user_data)->notebook));
			DEBUG_MSG("bfwin_key_release_event, connect signals, and call bfwin_notebook_changed(%p, %d)\n",user_data,i);
			bfwin_notebook_changed(BFWIN(user_data), i);
		}
	}
	return FALSE;
}

void
bfwin_create_main(Tbfwin * bfwin)
{
	GtkWidget *vbox;
	DEBUG_MSG("bfwin_create_main, bfwin=%p, main_window_w=%d\n", bfwin, main_v->globses.main_window_w);

#ifdef IDENTSTORING
	bftextview2_identifier_hash_init(bfwin);
#endif							/* IDENTSTORING */

	bfwin->main_window =
		window_full2(_("New Bluefish Window"), GTK_WIN_POS_CENTER, 0, G_CALLBACK(bfwin_destroy_event),
					 bfwin, FALSE, NULL);
	gtk_window_set_role(GTK_WINDOW(bfwin->main_window), "bluefish");
#ifndef MAC_INTEGRATION
	gtk_widget_realize(bfwin->main_window); /* Causes half baked window entry to appear on OSX WIndow menu. Probably this is not required also on other systems */
#endif
	if (!main_v->props.leave_to_window_manager) {
		if (main_v->globses.main_window_w > 0) {
			gtk_window_set_default_size(GTK_WINDOW(bfwin->main_window), main_v->globses.main_window_w,
										main_v->globses.main_window_h);
		} else {
			gtk_window_set_default_size(GTK_WINDOW(bfwin->main_window), main_v->globses.main_window_w * -1,
										main_v->globses.main_window_h);
			gtk_window_maximize(GTK_WINDOW(bfwin->main_window));
		}
	}
	g_signal_connect(G_OBJECT(bfwin->main_window), "delete_event", G_CALLBACK(bfwin_delete_event), bfwin);
	g_signal_connect(G_OBJECT(bfwin->main_window), "configure-event",
					 G_CALLBACK(bfwin_configure_event), bfwin);
	g_signal_connect(G_OBJECT(bfwin->main_window), "window-state-event",
					 G_CALLBACK(bfwin_window_state_event), bfwin);
	g_signal_connect(G_OBJECT(bfwin->main_window), "key-press-event", G_CALLBACK(bfwin_key_press_event), bfwin);
	g_signal_connect(G_OBJECT(bfwin->main_window), "key-release-event", G_CALLBACK(bfwin_key_release_event), bfwin);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(bfwin->main_window), vbox);
	gtk_widget_show(vbox);
#if GTK_CHECK_VERSION(3,4,0)
	bfwin->toolbarbox = gtk_grid_new();
	gtk_orientable_set_orientation(GTK_ORIENTABLE(bfwin->toolbarbox), GTK_ORIENTATION_VERTICAL);
	gtk_box_pack_start(GTK_BOX(vbox), bfwin->toolbarbox, FALSE, FALSE, 0);
	gtk_widget_show(bfwin->toolbarbox);
#else
	bfwin->toolbarbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), bfwin->toolbarbox, FALSE, FALSE, 0);
	gtk_widget_show(bfwin->toolbarbox);
#endif
	/* first a menubar */
	bfwin->uimanager = gtk_ui_manager_new();
#if !GTK_CHECK_VERSION(3,4,0)
	gtk_ui_manager_set_add_tearoffs(bfwin->uimanager, TRUE);
#endif
	bfwin_main_ui_init(bfwin, bfwin->toolbarbox);

	/* then the toolbars */
	DEBUG_MSG("bfwin_create_main, creating handles for all menu/toolbars\n");

	if (bfwin->session->view_main_toolbar) {
		/*bfwin_main_toolbar_init(bfwin);*/
		gtk_widget_show(bfwin->main_toolbar_hb);
	}

	/* the area for the middlebox and the outputbox */
#if GTK_CHECK_VERSION(3,0,0)
	bfwin->vpane = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
#else
	bfwin->vpane = gtk_vpaned_new();
#endif
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
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(bfwin->notebook), main_v->props.document_tabposition);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(bfwin->notebook), TRUE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(bfwin->notebook), FALSE);
#if !GTK_CHECK_VERSION(3, 0, 0)
	gtk_notebook_set_tab_border(GTK_NOTEBOOK(bfwin->notebook), 0);
#endif
	gtk_notebook_popup_enable(GTK_NOTEBOOK(bfwin->notebook));

	/* Add notebook and its fake friend to their common hbox. */
	bfwin->notebook_box = gtk_vbox_new(FALSE, 0);
	gotoline_frame_create(bfwin);
	gtk_box_pack_start(GTK_BOX(bfwin->notebook_box), bfwin->notebook, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(bfwin->notebook_box), bfwin->notebook_fake, TRUE, TRUE, 0);

	/* here we ask any plugins to build there gui, BEFORE WE START THE SIDEPANEL */
	g_slist_foreach(main_v->plugins, bfplugins_gui, bfwin);

	/* output_box * /
	   init_output_box(bfwin, vbox); */
	bfwin->outputbox = NULL;

	bfwin_side_panel_show_hide_toggle(bfwin, TRUE, (bfwin->session->view_left_panel), FALSE);

	/* finally the statusbar */
	{
		GtkWidget *hbox;
		gint onecharwidth;
		hbox = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
		bfwin->statusbar = gtk_statusbar_new();
		gtk_box_pack_start(GTK_BOX(hbox), bfwin->statusbar, TRUE, TRUE, 0);
		bfwin->statusbar_lncol = gtk_statusbar_new();
		gtk_box_pack_start(GTK_BOX(hbox), bfwin->statusbar_lncol, FALSE, FALSE, 0);
		/* I hope the 'w' is an average width character, the characters are usually not monospaced so these values are just averages that look good on most translations too */
		onecharwidth = widget_get_string_size(bfwin->statusbar_lncol, "x");
		gtk_widget_set_size_request(GTK_WIDGET(bfwin->statusbar_lncol), onecharwidth * 30, -1);
		bfwin->statusbar_insovr = gtk_statusbar_new();
		gtk_box_pack_start(GTK_BOX(hbox), bfwin->statusbar_insovr, FALSE, FALSE, 0);
		gtk_widget_set_size_request(GTK_WIDGET(bfwin->statusbar_insovr), onecharwidth * 6, -1);
		bfwin->statusbar_editmode = gtk_statusbar_new();
		gtk_box_pack_start(GTK_BOX(hbox), bfwin->statusbar_editmode, FALSE, FALSE, 0);
		gtk_widget_set_size_request(GTK_WIDGET(bfwin->statusbar_editmode), onecharwidth * 35, -1);
#if GTK_CHECK_VERSION(3, 0, 0)
		gtk_window_set_has_resize_grip(GTK_WINDOW(bfwin->main_window), TRUE);
#else
		gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(bfwin->statusbar_lncol), FALSE);
		gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(bfwin->statusbar_insovr), FALSE);
		gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(bfwin->statusbar), FALSE);
#endif /* gtk3 */
		if (bfwin->session->view_statusbar)
			gtk_widget_show_all(hbox);
		else
			gtk_widget_show(hbox);
	}
	notebook_connect_signals(bfwin);
	g_signal_connect(G_OBJECT(bfwin->notebook), "page-reordered", G_CALLBACK(notebook_reordered), bfwin);
#if GTK_CHECK_VERSION (3, 0, 0) /* Restores tab scrolling feature for gtk+3 builds */
	gtk_widget_add_events (bfwin->notebook, GDK_SCROLL_MASK);
	g_signal_connect (G_OBJECT(bfwin->notebook), "scroll-event", G_CALLBACK (notebook_scroll_event_lcb), bfwin);
#endif

	if (main_v->props.switch_tabs_by_altx) {
		notebook_set_tab_accels(bfwin);
		notebook_bind_tab_signals(bfwin);
	}
	/* everything is ready - we can start loading documents */
	/* start to open an empty doc */
	file_new_cb(NULL, bfwin);

	gtk_notebook_set_scrollable(GTK_NOTEBOOK(bfwin->notebook), TRUE);
	/* don't use show_all since some widgets are and should be hidden */
	gtk_widget_show(bfwin->notebook);
	gtk_widget_show(bfwin->notebook_box);

	{
		/* drag n drop support */
		const GtkTargetEntry drag_dest_types[] = {
			{"text/uri-list", 0, TARGET_URI_LIST},
			{"STRING", 0, TARGET_STRING},
		};
		gtk_drag_dest_set(bfwin->main_window, (GTK_DEST_DEFAULT_ALL)
						  , drag_dest_types, 2, (GDK_ACTION_DEFAULT | GDK_ACTION_COPY | GDK_ACTION_MOVE |
												 GDK_ACTION_LINK | GDK_ACTION_PRIVATE | GDK_ACTION_ASK));
		g_signal_connect(G_OBJECT(bfwin->main_window), "drag_data_received",
						 G_CALLBACK(bfwin_on_drag_data), bfwin);
	}
}

#ifdef MAC_INTEGRATION
/* first move all accelerators away from <control> to <command>, and then in a second run
move the <alt> accelerators to <control> (alt doesn't work on osx) */
static void
osx_accel_map_foreach_controltometa_lcb(gpointer data, const gchar * accel_path, guint accel_key,
										GdkModifierType accel_mods, gboolean changed)
{
	if (accel_mods & GDK_CONTROL_MASK) {
		accel_mods &= (accel_mods & GDK_MOD1_MASK) ? ~GDK_MOD1_MASK : ~GDK_CONTROL_MASK;
		accel_mods |= GDK_META_MASK;
		if (!gtk_accel_map_change_entry(accel_path, accel_key, accel_mods, FALSE)) {
			g_print("controltometa, could not change accelerator %s\n", accel_path);
		}
	}
}

static void
osx_accel_map_foreach_mod1tocontrol_lcb(gpointer data, const gchar * accel_path, guint accel_key,
										GdkModifierType accel_mods, gboolean changed)
{
	if (accel_mods & GDK_MOD1_MASK) {
		accel_mods &= ~GDK_MOD1_MASK;
		accel_mods |= GDK_CONTROL_MASK;
		if (!gtk_accel_map_change_entry(accel_path, accel_key, accel_mods, FALSE)) {
			g_print("mod1tocontrol, could not change accelerator %s\n", accel_path);
		}
	}
}
#endif

void
bfwin_show_main(Tbfwin * bfwin)
{
#ifdef MAC_INTEGRATION
	GtkWidget *menuitem;
	GtkosxApplication *theApp = g_object_new(GTKOSX_TYPE_APPLICATION, NULL);
	gtk_widget_hide(bfwin->menubar);
	gtkosx_application_set_menu_bar(theApp, GTK_MENU_SHELL(bfwin->menubar));
	DEBUG_MSG("hide gtk menubar, set gtkosxapplication menubar\n");
/*This arrangement gives more mackish ordering of menu. TODO Window menu does not track opened toplevels correctly; see bug #701571; for 2.2.5 temporal hack applied to the gdk/quartz/gdkeventloop-quartz.c  */
	gint menupos = 0;
	if (bfwin_action_group_is_available(bfwin->uimanager, "AboutActions")) { /*Since it is plugin, user can disable it, and this leads to bf crash */
		menuitem = gtk_ui_manager_get_widget(bfwin->uimanager, "/MainMenu/HelpMenu");
		gtkosx_application_set_help_menu (theApp, menuitem);
		menuitem = gtk_ui_manager_get_widget(bfwin->uimanager, "/MainMenu/HelpMenu/HelpAbout");
		gtkosx_application_insert_app_menu_item(theApp, menuitem, 0);
		gtkosx_application_insert_app_menu_item (theApp, g_object_ref(gtk_separator_menu_item_new ()), 1);
		menupos = 2;
	}
	gtkosx_application_set_window_menu (theApp, NULL);

	menuitem = gtk_ui_manager_get_widget(bfwin->uimanager, "/MainMenu/EditMenu/EditPreferences");
	gtkosx_application_insert_app_menu_item(theApp, menuitem, menupos);

	menuitem = gtk_ui_manager_get_widget(bfwin->uimanager, "/MainMenu/FileMenu/FileQuit");
	gtk_widget_hide(menuitem);

	gtkosx_application_set_use_quartz_accelerators (theApp, FALSE); /* If set to TRUE then widgets does not respond to bindings like Cmd+X|C|V TODO: find the way to make MacOSX shortcuts Cmd+M and Cmd+H work; for 2.2.5 hack applied to gdk/quartz/gdkkeys-quartz.c see bug #711019*/

	if (main_v->osx_status == 0 && g_list_length(main_v->bfwinlist) == 1) { /* Accelarators should be moved just once, at the startup */
		DEBUG_MSG("bfwin_show_main, configuring accelerators for OSX\n");
		gtk_accel_map_foreach_unfiltered(NULL, osx_accel_map_foreach_controltometa_lcb);
		gtk_accel_map_foreach_unfiltered(NULL, osx_accel_map_foreach_mod1tocontrol_lcb);
	}

/*
IgeMacMenuGroup *group;
	gtk_widget_hide(bfwin->menubar);

	ige_mac_menu_set_menu_bar(GTK_MENU_SHELL(bfwin->menubar));

	ifactory = gtk_item_factory_from_widget(bfwin->menubar);
	menuitem = gtk_item_factory_get_widget(ifactory, _("/File/Quit"));
	ige_mac_menu_set_quit_menu_item(GTK_MENU_ITEM(menuitem));

	group = ige_mac_menu_add_app_menu_group ();
	ige_mac_menu_add_app_menu_item(group,GTK_MENU_ITEM(menuitem),NULL);

	menuitem = gtk_item_factory_get_widget(ifactory, _("/Edit/Preferences"));
	group = ige_mac_menu_add_app_menu_group();
	ige_mac_menu_add_app_menu_item(group,GTK_MENU_ITEM(menuitem),NULL);
	ige_mac_menu_set_global_key_handler_enabled(TRUE);*/

	/* MACTODO: add focus in and focus out event so we can sync the menu
	   when we switch to a different bluefish window */

	   if (main_v->osx_status == 2) {

	   	bfwin_action_groups_set_sensitive(bfwin, FALSE);

		return;
	   }
#endif
	DEBUG_MSG("bfwin_show_main, before show\n");
	gtk_widget_show(bfwin->main_window);
	/* don't use show_all since some widgets are and should be hidden */
}

/***********************/
/* statusbar functions */
/***********************/

typedef struct {
	gint message_id;
	Tbfwin *bfwin;
} Tstatusbar_remove;

static
 gboolean
statusbar_pop(gpointer data)
{
	if (BFWIN(data)->statusbar)
		gtk_statusbar_pop(GTK_STATUSBAR(BFWIN(data)->statusbar), 0);

	BFWIN(data)->statusbar_pop_id = 0;

	return FALSE;
}

void
bfwin_statusbar_message(Tbfwin * bfwin, const gchar * message, gint seconds)
{
	if (bfwin->statusbar) {
		if (bfwin->statusbar_pop_id != 0) {
			g_source_remove(bfwin->statusbar_pop_id);
			gtk_statusbar_pop(GTK_STATUSBAR(bfwin->statusbar), 0);
		}
		gtk_statusbar_push(GTK_STATUSBAR(bfwin->statusbar), 0, message);
		bfwin->statusbar_pop_id = g_timeout_add_seconds(seconds, statusbar_pop, bfwin);
	}
}

void
bfwin_all_statusbar_message(const gchar * message, gint seconds)
{
	GList *tmplist = g_list_first(main_v->bfwinlist);
	while (tmplist) {
		bfwin_statusbar_message(BFWIN(tmplist->data), message, seconds);
		tmplist = g_list_next(tmplist);
	}
}

Tbfwin *
bfwin_window_new_with_project(Tproject * project)
{
	Tbfwin *bfwin = g_new0(Tbfwin, 1);

	g_return_val_if_fail(bfwin != NULL, NULL);
	g_return_val_if_fail(project != NULL, NULL);

	bfwin->project = project;
	bfwin->session = project->session;
	bfwin->bmarkdata = project->bmarkdata;
	DEBUG_MSG("bfwin_window_new_with_project, bfwin=%p, bfwin->session=%p, (from project %p)\n", bfwin,
			  bfwin->session, project);

	bfwin_create_main(bfwin);
	main_v->bfwinlist = g_list_append(main_v->bfwinlist, bfwin);
	bfwin_show_main(bfwin);

	return bfwin;
}

Tbfwin *
bfwin_window_new(void)
{
	Tbfwin *bfwin = g_new0(Tbfwin, 1);

	g_return_val_if_fail(bfwin != NULL, NULL);

	bfwin->session = main_v->session;
	bfwin->bmarkdata = main_v->bmarkdata;
	DEBUG_MSG("bfwin_window_new, bfwin=%p, (from main_v)\n", bfwin);

	bfwin_create_main(bfwin);
	main_v->bfwinlist = g_list_append(main_v->bfwinlist, bfwin);
	bfwin_show_main(bfwin);

	return bfwin;
}

void
bfwin_window_close(Tbfwin * bfwin)
{
	bfwin_delete_event(NULL, NULL, bfwin);
}

gboolean
bfwin_has_doc(Tbfwin * bfwin, Tdocument * doc)
{
	return (g_list_index(bfwin->documentlist, doc) >= 0);
}

void
bfwin_docs_not_complete(Tbfwin * bfwin, gboolean increase)
{
	if (increase)
		bfwin->num_docs_not_completed++;
	else
		bfwin->num_docs_not_completed--;
	DEBUG_MSG("bfwin_docs_not_complete, increase=%d, num=%d\n", increase, bfwin->num_docs_not_completed);
}

void
bfwin_set_title(Tbfwin * bfwin, Tdocument * doc, gint num_modified_change)
{
	gchar *title, *prfilepart;
	gint modified_docs;
	const gchar *tablabel;

	if (!doc)
		return;

	modified_docs = have_modified_documents(bfwin->documentlist);
	if (num_modified_change != 0 && (modified_docs > 1 || (modified_docs == 0 && num_modified_change != -1)
									 || (modified_docs == 1 && num_modified_change != 1))) {
		/* no title change */
		return;
	}
	tablabel = gtk_label_get_text(GTK_LABEL(doc->tab_label));
	if (doc->uri) {
		gchar *curi;
		if (g_file_is_native(doc->uri)) {
			curi = g_file_get_parse_name(doc->uri);
		} else {
			curi = g_file_get_uri(doc->uri);
		}
		if (bfwin->project) {
			prfilepart = g_strconcat(bfwin->project->name, " - ", tablabel, " (", curi, ")", NULL);
		} else {
			prfilepart = g_strconcat(tablabel, " (", curi, ")", NULL);
		}
		g_free(curi);
	} else {
		if (bfwin->project) {
			prfilepart = g_strconcat(bfwin->project->name, " - ", tablabel, NULL);
		} else {
			prfilepart = g_strconcat(tablabel, NULL);
		}
	}
	if (doc->modified) {
		title = g_strconcat("* ", prfilepart, " - Bluefish " VERSION, NULL);
	} else {
		title = g_strconcat(prfilepart, " - Bluefish " VERSION, NULL);
	}
	if (main_v->props.max_window_title > 0 && strlen(title) > main_v->props.max_window_title)
		title[main_v->props.max_window_title] = '\0';
	gtk_window_set_title(GTK_WINDOW(bfwin->main_window), title);
	/*rename_window_entry_in_all_windows(bfwin, title); */
	g_free(title);
	g_free(prfilepart);
}

/* use -1 to switch to the last page */
gboolean
bfwin_switch_to_document_by_index(Tbfwin * bfwin, gint i)
{
	DEBUG_MSG("bfwin_switch_to_document_by_index, index=%d\n", i);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(bfwin->notebook), i);
	return TRUE;
}

gboolean
bfwin_switch_to_document_by_pointer(Tbfwin * bfwin, Tdocument * document)
{
	gint i = g_list_index(bfwin->documentlist, document);
	if (i==-1)
		return FALSE;
	return bfwin_switch_to_document_by_index(bfwin, i);
}
