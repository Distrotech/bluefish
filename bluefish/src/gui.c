/* Bluefish HTML Editor
 * gui.c - the main GUI
 *
 * Copyright (C) 2002-2006 Olivier Sessink
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
#include <stdlib.h>    /* exit() on Solaris */
#include <string.h>    /* strchr() */
#include <time.h>      /* nanosleep() */
#include <unistd.h>    /* exit() */

#include "config.h"

#include "bluefish.h"
#include "gui.h"
#include "bf_lib.h"        /* get_int_from_string() */
#include "bookmark.h"
#include "document.h"      /* file_new_cb() */
#include "filebrowser.h"
#include "fref.h"
#include "gtk_easy.h"
#include "html.h"
#include "html2.h"
#include "html_form.h"
#include "html_table.h"
#include "image.h"
#include "menu.h"          /* menu_create_main(), recent_menu_init() */
#include "outputbox.h"     /* init_outputbox() */
#include "pixmap.h"        /* new_pixmap(); */
#include "preferences.h"   /* open_preferences_cb */
#include "project.h"
#include "snr2.h"          /* search_cb, replace_cb */
#include "stringlist.h"
#include "undo_redo.h"     /* undo_cb() redo_cb() etc. */
#include "wizards.h"

#ifdef HAVE_ATLEAST_GTK_2_4
#include "quickstart.h"
#endif /* HAVE_ATLEAST_GTK_2_4 */

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
 **/
void notebook_hide(Tbfwin *bfwin) {
	gtk_widget_hide (bfwin->notebook);
	gtk_widget_show (bfwin->notebook_fake);
}

/**
 * notebook_show:
 *
 * Shows the notebook, after a notebook_hide() has been called.
 *
 * Return value: void
 **/
void notebook_show(Tbfwin *bfwin) {
	gtk_widget_hide (bfwin->notebook_fake);
	gtk_widget_show (bfwin->notebook);	
}

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

gboolean switch_to_document_by_filename(Tbfwin *bfwin,gchar *filename) {
	gint index;

	index = documentlist_return_index_from_filename(bfwin->documentlist,filename);
	return switch_to_document_by_index(bfwin,index);
}


static void notebook_switch_page_lcb(GtkWidget *notebook,GtkNotebookPage *page,gint page_num,Tbfwin *bfwin) {
	DEBUG_MSG("notebook_switch_page_lcb, page=%d\n", page_num);
	notebook_changed(bfwin,page_num);
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
	}
}

static void left_panel_notify_position_lcb(GObject *object,GParamSpec *pspec,gpointer data){
	gint position;
	g_object_get(object, pspec->name, &position, NULL);
	DEBUG_MSG("left_panel_notify_position_lcb, new position=%d\n", position);
	if (main_v->props.restore_dimensions) {
		if (main_v->props.left_panel_left) {
			main_v->props.left_panel_width = position;
		} else {
			main_v->props.left_panel_width = main_v->globses.main_window_w - position;
		}
	}
}

GtkWidget *left_panel_build(Tbfwin *bfwin) {
	GtkWidget *fileb;
	GtkWidget *left_notebook = gtk_notebook_new();
	GtkWidget *fref;
	GtkWidget *bmarks;
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(left_notebook),main_v->props.leftpanel_tabposition);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(left_notebook), TRUE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(left_notebook), FALSE);
	gtk_notebook_set_tab_hborder(GTK_NOTEBOOK(left_notebook), 0);
	gtk_notebook_set_tab_vborder(GTK_NOTEBOOK(left_notebook), 0);
	gtk_notebook_popup_enable(GTK_NOTEBOOK(left_notebook));
	fileb = filebrowser_init(bfwin);
	fref = fref_gui(bfwin);
	bmarks = bmark_gui(bfwin);
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(left_notebook),fileb,new_pixmap(105),gtk_label_new(_("Filebrowser")));
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(left_notebook),fref,new_pixmap(106),gtk_label_new(_("Function reference")));
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(left_notebook),bmarks,new_pixmap(104),gtk_label_new(_("Bookmarks")));
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
		filebrowser_cleanup(bfwin);
		fref_cleanup(bfwin);
		bmark_cleanup(bfwin);
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
			filebrowser_cleanup(bfwin);
			fref_cleanup(bfwin);
			bmark_cleanup(bfwin); 
		}
	}
	if (show) {
		bfwin->hpane = gtk_hpaned_new();
		if (main_v->props.left_panel_left) {
			DEBUG_MSG("set paned position to %d (left)\n",main_v->props.left_panel_width);
			gtk_paned_set_position(GTK_PANED(bfwin->hpane), main_v->props.left_panel_width);
		} else {
			DEBUG_MSG("set paned position to %d (right)\n",main_v->globses.main_window_w - main_v->props.left_panel_width);
			gtk_paned_set_position(GTK_PANED(bfwin->hpane), main_v->globses.main_window_w - main_v->props.left_panel_width);
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
}

typedef struct {
	const gchar *ident;
	void (*func)();
	gint pixmaptype;
	const gchar *tooltiptext;
} Ttoolbaritem;

typedef struct {
	Ttoolbaritem *tbitem;
	GtkWidget *button;
	Tbfwin *bfwin;
}Tquickbaritem;

static void bold_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 1, NULL);
}
static void italic_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 2, NULL);
}
static void paragraph_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 5, NULL);
}
static void break_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 6, NULL);
}
static void breakclear_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 41, NULL);
}
static void nbsp_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 7, NULL);
}
static void center_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 8, NULL);
}
static void rightjustify_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 9, NULL);
}
static void comment_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 10, NULL);
}
static void fontsizeplus1_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 11, NULL);
}
static void fontsizeminus1_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 12, NULL);
}
static void preformatted_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 13, NULL);
}
static void subscript_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 14, NULL);
}
static void superscript_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 15, NULL);
}
static void strong_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 16, NULL);
}
static void emphasis_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 17, NULL);
}
static void heading1_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 18, NULL);
}
static void heading2_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 19, NULL);
}
static void heading3_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 20, NULL);
}
static void heading4_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 21, NULL);
}
static void heading5_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 22, NULL);
}
static void heading6_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 23, NULL);
}
static void table_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 24, NULL);
}
static void tablerow_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 25, NULL);
}
static void tableheader_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 26, NULL);
}
static void tabledata_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 27, NULL);
}
static void tablecaption_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 28, NULL);
}
static void frameset_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 29, NULL);
}
static void frame_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 30, NULL);
}
static void noframes_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 31, NULL);
}
static void target_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 32, NULL);
}
static void unorderedlist_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 33, NULL);
}
static void orderedlist_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 34, NULL);
}
static void listitem_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 35, NULL);
}
static void definitionlist_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 36, NULL);
}
static void definitionterm_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 37, NULL);
}
static void definition_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 38, NULL);
}
static void style_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 42, NULL);
}
static void dfn_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 48, NULL);
}
static void code_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 49, NULL);
}
static void samp_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 50, NULL);
}
static void kbd_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 51, NULL);
}
static void var_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 52, NULL);
}
static void cite_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 53, NULL);
}
static void abbr_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 54, NULL);
}
static void acronym_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 55, NULL);
}

static void quickstart_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
#ifdef HAVE_ATLEAST_GTK_2_4
	quickstart_dialog_new(bfwin);
#else
	quickstart_dialog(bfwin,NULL);
#endif
}
static void body_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	body_dialog(bfwin,NULL);
}
static void quickanchor_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	DEBUG_MSG("quickanchor_clicked_lcb, calling quickanchor_dialog with bfwin=%p and data=NULL\n",bfwin);
	quickanchor_dialog(bfwin, NULL);
}
static void quickrule_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	quickrule_dialog(bfwin,NULL);
}
static void email_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	email_dialog(bfwin,NULL);
}
static void fontdialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	font_dialog(bfwin,NULL);
}
static void basefont_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	basefont_dialog(bfwin,NULL);
}
static void framesetdialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	frameset_dialog(bfwin,NULL);
}
static void framedialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	frame_dialog(bfwin,NULL);
}
static void quicklist_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	quicklist_dialog(bfwin,NULL);
}
static void link_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	linkdialog_dialog(bfwin,NULL,linkdialog_mode_css);
}
static void formdialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	formdialog_dialog(bfwin,NULL);
}
static void inputbuttondialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	inputdialog_dialog(bfwin, NULL, "submit");
}
static void textdialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	inputdialog_dialog(bfwin, NULL, "text");
}
static void hiddendialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	inputdialog_dialog(bfwin, NULL, "hidden");
}
static void textareadialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	textareadialog_dialog(bfwin,NULL);
}
static void radiodialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	inputdialog_dialog(bfwin, NULL, "radio");
}
static void checkdialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	inputdialog_dialog(bfwin, NULL, "checkbox");
}
static void selectdialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	selectdialog_dialog(bfwin,NULL);
}
static void optiondialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	optiondialog_dialog(bfwin,NULL);
}
static void optgroupdialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	optgroupdialog_dialog(bfwin,NULL);
}
static void buttondialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	buttondialog_dialog(bfwin,NULL);
}
static void image_insert_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	image_insert_dialog(bfwin,NULL);
}
static void thumbnail_insert_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	thumbnail_insert_dialog(bfwin);
}
static void multi_thumbnail_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	multi_thumbnail_dialog(bfwin);
}
static void tabledialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	tabledialog_dialog(bfwin,NULL);
}
static void tablerowdialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	tablerowdialog_dialog(bfwin,NULL);
}
static void tableheaddialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	tableheaddialog_dialog(bfwin,NULL);
}
static void tabledatadialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	tabledatadialog_dialog(bfwin,NULL);
}
static void tablewizard_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	tablewizard_dialog(bfwin);
}
static void framewizard_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	framewizard_dialog(bfwin);
}
static void span_dialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	span_dialog(bfwin,NULL);
}
static void div_dialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	div_dialog(bfwin,NULL);
}


static Ttoolbaritem tbi[] = {
	{"quickstart...", quickstart_clicked_lcb,13 , N_("QuickStart...")},
	{"body...", body_clicked_lcb, 14, N_("Body...")},
	{"bold", bold_clicked_lcb,  15, N_("Bold")},
	{"italic", italic_clicked_lcb,  16, N_("Italic")},
	{"paragraph", paragraph_clicked_lcb,  19, N_("Paragraph")},
	{"break", break_clicked_lcb,  20, N_("Break")},
	{"breakclear", breakclear_clicked_lcb,  21, N_("Break and clear")},
	{"nbsp", nbsp_clicked_lcb,  22, N_("Non-breaking space")},
	{"", NULL, 0, NULL}, /* spacing */
	{"anchor...", quickanchor_clicked_lcb,23 , N_("Anchor...")},
	{"rule...", quickrule_clicked_lcb, 24, N_("Rule...")},
	{"center", center_clicked_lcb,  28, N_("Center")},
	{"rightjustify", rightjustify_clicked_lcb,  29, N_("Right Justify")},
	{"comment", comment_clicked_lcb, 30 , N_("Comment")},
	{"email...", email_clicked_lcb, 31, N_("E-mail...")},
	{"font...",fontdialog_clicked_lcb , 32, N_("Font...")},
	{"basefont...", basefont_clicked_lcb, 33, N_("Base Font Size...")},
	{"", NULL, 0, NULL}, /* spacing */
	{"fontsize+1", fontsizeplus1_clicked_lcb, 34 , N_("Font Size +1")},
	{"fontsize-1", fontsizeminus1_clicked_lcb,  35, N_("Font Size -1")},
	{"preformatted", preformatted_clicked_lcb,  36, N_("Preformatted Text")},
	{"subscript", subscript_clicked_lcb,  37, N_("Subscript")},
	{"superscript", superscript_clicked_lcb,  38, N_("Superscript")},
	{"strong", strong_clicked_lcb,  15, N_("Strong")},
	{"emphasis", emphasis_clicked_lcb,  16, N_("Emphasis")},
	{"", NULL, 0, NULL}, /* spacing */
	{"heading1", heading1_clicked_lcb,  39, N_("Heading 1")},
	{"heading2", heading2_clicked_lcb,  40, N_("Heading 2")},
	{"heading3", heading3_clicked_lcb,  41, N_("Heading 3")},
	{"heading4", heading4_clicked_lcb,  42, N_("Heading 4")},
	{"heading5", heading5_clicked_lcb,  43, N_("Heading 5")},
	{"heading6", heading6_clicked_lcb,  44, N_("Heading 6")},
	{"tablewizard...",tablewizard_clicked_lcb, 45, N_("Table Wizard...")},
	{"", NULL, 0, NULL}, /* spacing */
	{"table...",tabledialog_clicked_lcb, 46, N_("Table...")},
	{"tablerow...", tablerowdialog_clicked_lcb, 47, N_("Table Row...")},
	{"tableheader...", tableheaddialog_clicked_lcb, 48, N_("Table Header...")},
	{"tabledata...", tabledatadialog_clicked_lcb, 49, N_("Table Data...")},
	{"", NULL, 0, NULL}, /* spacing */
	{"table", table_clicked_lcb,  50, N_("Table")},
	{"tablerow", tablerow_clicked_lcb,  51, N_("Table Row")},
	{"tableheader", tableheader_clicked_lcb,  52, N_("Table Header")},
	{"tabledata", tabledata_clicked_lcb,  53, N_("Table Data")},
	{"tablecaption", tablecaption_clicked_lcb,  54, N_("Table Caption")},
	{"framewizard...",framewizard_clicked_lcb , 55, N_("Frame Wizard...")},
	{"", NULL, 0, NULL}, /* spacing */
	{"frameset...", framesetdialog_clicked_lcb,56 , N_("Frameset...")},
	{"frame...",framedialog_clicked_lcb , 57, N_("Frame...")},
	{"", NULL, 0, NULL}, /* spacing */
	{"frameset", frameset_clicked_lcb, 58 , N_("Frameset")},
	{"frame", frame_clicked_lcb,  59, N_("Frame")},
	{"noframes", noframes_clicked_lcb,  60, N_("Noframes")},
	{"target", target_clicked_lcb,  61, N_("Target")},
	{"form...",formdialog_clicked_lcb , 62, N_("Form...")},
	{"inputbutton...", inputbuttondialog_clicked_lcb, 63, N_("Input Button...")},
	{"text...",textdialog_clicked_lcb , 65, N_("Input Text...")},
	{"hidden...", hiddendialog_clicked_lcb, 66, N_("Input Hidden...")},
	{"textarea...",textareadialog_clicked_lcb , 67, N_("Textarea...")},
	{"radiobutton...", radiodialog_clicked_lcb, 68, N_("Input Radio Button...")},
	{"checkbox...", checkdialog_clicked_lcb, 69, N_("Input Check Box...")},
	{"select...", selectdialog_clicked_lcb, 70, N_("Select...")},
	{"option...",optiondialog_clicked_lcb , 71, N_("Option...")},
	{"optiongroup...", optgroupdialog_clicked_lcb, 72, N_("Option group...")},
	{"button...", buttondialog_clicked_lcb, 63, N_("Button...")},
	{"quicklist...", quicklist_clicked_lcb, 73, N_("Quick List...")},
	{"unorderedlist", unorderedlist_clicked_lcb,  74, N_("Unordered List")},
	{"orderedlist", orderedlist_clicked_lcb,  75, N_("Ordered List")},
	{"listitem", listitem_clicked_lcb,  76, N_("List Item")},
	{"definitionlist", definitionlist_clicked_lcb,  77, N_("Definition List")},
	{"definitionterm", definitionterm_clicked_lcb,  78, N_("Definition Term")},
	{"definition", definition_clicked_lcb,  79, N_("Definition")},
	{"createstylesheet...", new_css_dialog, 85, N_("Create stylesheet...")},
	{"span...", span_dialog_clicked_lcb,86 , N_("Span...")},
	{"div...",div_dialog_clicked_lcb,87 , N_("Div...")},
	{"style", style_clicked_lcb, 89, N_("Style")},
	{"linkstylesheet...", link_clicked_lcb,90 , N_("Link to stylesheet...")},
	{"image...", image_insert_clicked_lcb, 25, N_("Insert image...")},
	{"thumbnail...", thumbnail_insert_clicked_lcb, 26, N_("Insert thumbnail...")},
	{"mthumbnail...", multi_thumbnail_clicked_lcb, 27, N_("Multi thumbnail...")},
	{"dfn", dfn_clicked_lcb,  93, N_("Definition")},
	{"code", code_clicked_lcb,  94, N_("Code")},
	{"samp", samp_clicked_lcb,  95, N_("Sample")},
	{"kbd", kbd_clicked_lcb,  96, N_("Keyboard")},
	{"var", var_clicked_lcb,  97, N_("Variable")},
	{"cite", cite_clicked_lcb,  98, N_("Citation")},
	{"abbr", abbr_clicked_lcb,  99, N_("Abbreviation")},
	{"acronym", acronym_clicked_lcb,  100, N_("Acronym")},
	{"", NULL, 0, NULL} /* spacing */
};

static void html_toolbar_remove_from_quickbar_lcb(GtkMenuItem *menuitem, Ttoolbaritem *tbitem) {
	GList *bfwlist;
	main_v->globses.quickbar_items = remove_from_stringlist(main_v->globses.quickbar_items, tbitem->ident);
	bfwlist = g_list_first(main_v->bfwinlist);
	while (bfwlist) {
		Tquickbaritem *qbi=NULL;
		Tbfwin *bfwin = BFWIN(bfwlist->data);
		GList *tmplist  = g_list_first(bfwin->toolbar_quickbar_children);
		while (tmplist) {
			qbi = tmplist->data;
			if (qbi->tbitem == tbitem) {
				break;
			}
			tmplist = g_list_next(tmplist);
		}
		if (qbi) {
			gtk_widget_hide(qbi->button);
			gtk_widget_destroy(qbi->button);
			g_free(qbi);
		}
		bfwlist = g_list_next(bfwlist);
	}
}

/* return the position in the quickbar GList of a tool bar item */
gint get_quickbar_item_position(Ttoolbaritem *tbitem) {
	GList *tmplist, *winlist = g_list_first(main_v->bfwinlist);
	gint pos = 0;
	tmplist  = g_list_first(BFWIN(winlist->data)->toolbar_quickbar_children);

	while (tmplist) {
		Tquickbaritem *qbi;
		qbi = tmplist->data;
		if (qbi->tbitem == tbitem) {
			DEBUG_MSG("get_quickbar_item_position, quickbar item is at %d\n", pos);
			return pos;
		}
		pos += 1;
		tmplist = g_list_next(tmplist);
	}
	DEBUG_MSG("get_quickbar_item_position, not found, return -1!\n");
	return -1;
}

static gboolean html_toolbar_quickbar_item_button_press_lcb(GtkWidget *widget,GdkEventButton *bevent,Ttoolbaritem *tbitem);

static void html_toolbar_quickbar_switch(Ttoolbaritem *tbitem, gboolean moveright) {
	gint pos;

	/* first we look for the current location of this item */	
	pos = get_quickbar_item_position(tbitem);
	DEBUG_MSG("html_toolbar_quickbar_switch, found item %s at pos=%d, config list length=%d\n",tbitem->ident,pos,g_list_length(main_v->globses.quickbar_items));
	if (pos >= 0) {
		GList *tmp1, *tmp2, *tmplist;
		/* then we move this item to the new place in the config list */
		tmp1 = g_list_nth(main_v->globses.quickbar_items, pos);
		if (!tmp1) {
			DEBUG_MSG("html_toolbar_quickbar_switch, nothing found in config list at pos=%d, RETURN\n",pos);
			return;
		}
		tmp2 = (moveright) ? g_list_next(tmp1) : g_list_previous(tmp1);
		if (!tmp2) {
			DEBUG_MSG("html_toolbar_quickbar_switch, no tmp2 found, moveright=%d, list length=%d RETURN\n",moveright,g_list_length(main_v->globses.quickbar_items));
			return;
		}
		list_switch_order(tmp1, tmp2);
		DEBUG_MSG("html_toolbar_quickbar_switch, after list_switch_order, list length=%d\n",g_list_length(main_v->globses.quickbar_items));
		
		/* now switch the widgets in every window that has a quickbar */
		tmplist = g_list_first(main_v->bfwinlist);
		while (tmplist) {
			Tbfwin *bfwin = BFWIN(tmplist->data);
			DEBUG_MSG("html_toolbar_quickbar_switch, checking bfwin=%p\n",bfwin);
			if (bfwin->toolbar_quickbar && bfwin->toolbar_quickbar_children) {
				Tquickbaritem *qbi1;
				DEBUG_MSG("retrieving quickbaritem from pos %d\n",pos);
				tmp1 = g_list_nth(bfwin->toolbar_quickbar_children, pos);
				tmp2 = (moveright) ? g_list_next(tmp1) : g_list_previous(tmp1);
				qbi1 = (Tquickbaritem *)tmp1->data;
				list_switch_order(tmp1, tmp2);
				DEBUG_MSG("html_toolbar_quickbar_switch, quickbaritem %p has tbitem %s\n",qbi1, qbi1->tbitem->ident);
				/* I don't know why the next bit of code doesn't work... bug in gtk? */
				/*g_object_ref(G_OBJECT(qbi1->button));
				DEBUG_MSG("html_toolbar_quickbar_switch, detaching widget!, button=%p, button->parent=%p, toolbar=%p\n",qbi1->button,qbi1->button->parent,bfwin->toolbar_quickbar);
				gtk_container_remove(GTK_CONTAINER(bfwin->toolbar_quickbar),qbi1->button);
				DEBUG_MSG("html_toolbar_quickbar_switch, attaching widget at pos %d\n",(moveright)?pos+1:pos-1);
				gtk_toolbar_insert_widget(GTK_TOOLBAR(bfwin->toolbar_quickbar),qbi1->button,
							_(qbi1->tbitem->tooltiptext),"",(moveright)?pos+1:pos-1);
				g_object_unref(G_OBJECT(qbi1->button));
				*/
				/* this workaround bit of code removes (and thus destroys) the button and creates a new identical button on the new location */
				DEBUG_MSG("html_toolbar_quickbar_switch, about to destroy button,(toolbar has %d children)\n",g_list_length(GTK_TOOLBAR(bfwin->toolbar_quickbar)->children));
				gtk_container_remove(GTK_CONTAINER(bfwin->toolbar_quickbar),qbi1->button);
				DEBUG_MSG("html_toolbar_quickbar_switch, inserting widget at pos %d (toolbar has %d children)\n",(moveright)?pos+1:pos-1, g_list_length(GTK_TOOLBAR(bfwin->toolbar_quickbar)->children));
				qbi1->button = gtk_toolbar_insert_item(GTK_TOOLBAR(bfwin->toolbar_quickbar), NULL, _(qbi1->tbitem->tooltiptext),
							"", new_pixmap(qbi1->tbitem->pixmaptype), G_CALLBACK(qbi1->tbitem->func), bfwin, (moveright)?pos+1:pos-1);
				g_signal_connect(qbi1->button, "button-press-event", G_CALLBACK(html_toolbar_quickbar_item_button_press_lcb), tbitem);
				gtk_widget_show(qbi1->button);
			}
			tmplist = g_list_next(tmplist);
		}
		
	}
}

static void html_toolbar_quickbar_move_left_lcb(GtkMenuItem *menuitem, Ttoolbaritem *tbitem) {
	html_toolbar_quickbar_switch(tbitem, FALSE);
/*	GList *winlist = g_list_first(main_v->bfwinlist); 
	DEBUG_MSG("moving tbitem %p left on quickbars\n", tbitem);
	while (winlist) {
		GList *tmplist;
		gint pos;
		Tbfwin *bfwin = BFWIN(winlist->data);
		Tquickbaritem *qbi;

		pos = get_quickbar_item_position(tbitem);
		tmplist = g_list_nth(bfwin->toolbar_quickbar_children, pos);
		qbi = tmplist->data;

		if (qbi) {
			if (pos) { / * pos > 0 * /
				gtk_widget_destroy(qbi->button);
			}
			g_free(qbi);
		}
		
		if (pos) {
			GList *other;
			qbi = g_new(Tquickbaritem,1);
			qbi->button = gtk_toolbar_insert_item(GTK_TOOLBAR(bfwin->toolbar_quickbar), NULL, _(tbitem->tooltiptext),
							"", new_pixmap(tbitem->pixmaptype), G_CALLBACK(tbitem->func), bfwin, pos-1);
			g_signal_connect(qbi->button, "button-press-event", G_CALLBACK(html_toolbar_quickbar_item_button_press_lcb), tbitem);
			gtk_widget_show(qbi->button);
			qbi->tbitem = tbitem;
			
			other = g_list_previous(tmplist);
			list_switch_order(tmplist, other);
			
			tmplist = g_list_nth(main_v->globses.quickbar_items, pos);
			other = g_list_previous(tmplist);
			list_switch_order(tmplist, other);
			g_list_free(tmplist);
			g_list_free(other);
		}
		winlist = g_list_next(winlist);
	}
	 we don't create this list here so we don't need to free it!!
	g_list_free(winlist);*/
}

static void html_toolbar_quickbar_move_right_lcb(GtkMenuItem *menuitem, Ttoolbaritem *tbitem) {
	html_toolbar_quickbar_switch(tbitem, TRUE);
/*	GList *winlist = g_list_first(main_v->bfwinlist); 
	DEBUG_MSG("moving tbitem %p right on quickbars\n", tbitem);
	while (winlist) {
		gint pos;
		GList *tmplist, *other;
		Tbfwin *bfwin = BFWIN(winlist->data);
		Tquickbaritem *qbi;
		pos = get_quickbar_item_position(tbitem);
		tmplist = g_list_nth(bfwin->toolbar_quickbar_children, pos);
		qbi = tmplist->data;

		if (qbi) {
			gtk_widget_hide(qbi->button);
			gtk_widget_destroy(qbi->button);
			g_free(qbi);
		}
		
		qbi = g_new(Tquickbaritem,1);
		qbi->button = gtk_toolbar_insert_item(GTK_TOOLBAR(bfwin->toolbar_quickbar), NULL, _(tbitem->tooltiptext),
							"", new_pixmap(tbitem->pixmaptype), G_CALLBACK(tbitem->func), bfwin, pos+1);
		g_signal_connect(qbi->button, "button-press-event", G_CALLBACK(html_toolbar_quickbar_item_button_press_lcb), tbitem);
		gtk_widget_show(qbi->button);
		qbi->tbitem = tbitem;
		
		other = g_list_next(tmplist);
		list_switch_order(tmplist, other);
			
		tmplist = g_list_nth(main_v->globses.quickbar_items, pos);
		other = g_list_next(tmplist);
		list_switch_order(tmplist, other);
		g_list_free(tmplist);
		g_list_free(other);
		
		winlist = g_list_next(winlist);
	}
	/ * g_list_free(winlist); */
}

static gboolean html_toolbar_quickbar_item_button_press_lcb(GtkWidget *widget,GdkEventButton *bevent,Ttoolbaritem *tbitem) {
	if (bevent->button == 3) {
		GtkWidget *menu = gtk_menu_new ();
		GtkWidget *menuitem = gtk_menu_item_new_with_label(_("Remove from quickbar"));
		GtkWidget *itemleft = gtk_menu_item_new_with_label(_("Move Left"));
		GtkWidget *itemright = gtk_menu_item_new_with_label(_("Move Right"));
		GtkWidget *sep = gtk_separator_menu_item_new();
		DEBUG_MSG("popup for tbitem %p\n", tbitem);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(html_toolbar_remove_from_quickbar_lcb), tbitem);
		g_signal_connect(G_OBJECT(itemleft), "activate", G_CALLBACK(html_toolbar_quickbar_move_left_lcb), tbitem);
		g_signal_connect(G_OBJECT(itemright), "activate", G_CALLBACK(html_toolbar_quickbar_move_right_lcb), tbitem);
		gtk_menu_append(GTK_MENU(menu), itemleft);
		gtk_menu_append(GTK_MENU(menu), itemright);
		gtk_menu_append(GTK_MENU(menu), sep);
		gtk_menu_append(GTK_MENU(menu), menuitem);
		gtk_widget_show_all (menu);
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
			  NULL, widget, 0, gtk_get_current_event_time ());
		return TRUE;
	}
	return FALSE;
}

static void html_toolbar_add_to_quickbar_lcb(GtkMenuItem *menuitem, Ttoolbaritem *tbitem) {
	GList *tmplist = g_list_first(main_v->bfwinlist);
	DEBUG_MSG("adding tbitem %p to quickbars\n", tbitem);
	main_v->globses.quickbar_items = add_to_stringlist(main_v->globses.quickbar_items, tbitem->ident);
	while (tmplist) {
		Tquickbaritem *qbi;
		Tbfwin *bfwin = BFWIN(tmplist->data);
		qbi = g_new(Tquickbaritem,1);
		qbi->button = gtk_toolbar_append_item(GTK_TOOLBAR(bfwin->toolbar_quickbar), NULL, _(tbitem->tooltiptext),
							"", new_pixmap(tbitem->pixmaptype), G_CALLBACK(tbitem->func), bfwin);
		g_signal_connect(qbi->button, "button-press-event", G_CALLBACK(html_toolbar_quickbar_item_button_press_lcb), tbitem);
		gtk_widget_show(qbi->button);
		qbi->tbitem = tbitem;
		bfwin->toolbar_quickbar_children = g_list_append(bfwin->toolbar_quickbar_children, qbi);
		tmplist = g_list_next(tmplist);
	}
}

static gboolean html_toolbar_item_button_press_lcb(GtkWidget *widget,GdkEventButton *bevent,Ttoolbaritem *tbitem) {
	DEBUG_MSG("html_toolbar_item_button_press_lcb, button=%d\n", bevent->button);
	if (bevent->button == 3) {
		GtkWidget *menu = gtk_menu_new ();
		GtkWidget *menuitem = gtk_menu_item_new_with_label(_("Add to quickbar"));
		DEBUG_MSG("popup for tbitem %p\n", tbitem);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(html_toolbar_add_to_quickbar_lcb), tbitem);
		gtk_menu_append(GTK_MENU(menu), menuitem);
		gtk_widget_show_all (menu);
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
			  NULL, widget, 0, gtk_get_current_event_time ());
		return TRUE;
	}
	return FALSE;
}

static void html_toolbar_add_items(Tbfwin *bfwin, GtkWidget *html_toolbar, Ttoolbaritem *tbi, gint from, gint to) {
	gint i;
	GtkWidget *item;
	for (i=from;i<=to;i++) {
		if (tbi[i].func == NULL) {
			gtk_toolbar_append_space(GTK_TOOLBAR(html_toolbar));
		} else {
			item = gtk_toolbar_append_item(GTK_TOOLBAR(html_toolbar), NULL, _(tbi[i].tooltiptext),
						NULL, new_pixmap(tbi[i].pixmaptype), G_CALLBACK(tbi[i].func), bfwin);
			g_signal_connect(item, "button-press-event", G_CALLBACK(html_toolbar_item_button_press_lcb), &tbi[i]);
			DEBUG_MSG("adding tbitem %p to html_toolbar\n", &tbi[i]);
		}
	}
}

static void html_toolbar_add_items_to_submenu(Tbfwin *bfwin, GtkWidget *html_toolbar, Ttoolbaritem *tbi, gint from, gint to, gchar *menulabel, gint menupix) {
	GtkWidget *menu_bar, *sub_menu, *menu_item, *pixmap;
	GtkTooltips *tooltips;
	gint i;
	
	tooltips = gtk_tooltips_new();	
	sub_menu = gtk_menu_new();
	for (i=from;i<=to;i++) {
		if (tbi[i].func == NULL) {
			gtk_toolbar_append_space(GTK_TOOLBAR(html_toolbar));
		} else {
			menu_item = gtk_menu_item_new();
			gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), menu_item, _(tbi[i].tooltiptext), NULL);
			gtk_container_add(GTK_CONTAINER(menu_item), new_pixmap(tbi[i].pixmaptype));
			g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(tbi[i].func), bfwin);
			g_signal_connect(menu_item, "button-press-event", G_CALLBACK(html_toolbar_item_button_press_lcb), &tbi[i]);
			gtk_menu_append(GTK_MENU(sub_menu), menu_item);
		}
	}
	menu_bar = gtk_menu_bar_new();
	menu_item = gtk_menu_item_new();
	gtk_container_add(GTK_CONTAINER(menu_item),new_pixmap(menupix));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), sub_menu);
	gtk_menu_bar_append(GTK_MENU_BAR(menu_bar), menu_item);
	pixmap = new_pixmap(menupix);
	gtk_toolbar_append_element(GTK_TOOLBAR(html_toolbar),GTK_TOOLBAR_CHILD_WIDGET,menu_bar,NULL,menulabel,NULL,pixmap,NULL,NULL);
	gtk_object_sink(GTK_OBJECT(pixmap));
}

void make_html_toolbar(Tbfwin *bfwin) {
/*
 * every item with a ... in the tooltip brings up a dialog, every item
 * without the ... in the tooltip will directly insert html code
 *
 */
	GtkWidget *html_toolbar, *html_notebook;

	DEBUG_MSG("make_html_toolbar, started\n");
	html_notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(html_notebook), GTK_POS_TOP);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(html_notebook), TRUE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(html_notebook), TRUE);
	gtk_container_add(GTK_CONTAINER(bfwin->html_toolbar_hb), html_notebook);

	bfwin->toolbar_quickbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(bfwin->toolbar_quickbar), GTK_TOOLBAR_ICONS);
	DEBUG_MSG("make_html_toolbar, creating quickbar\n");
	{
		GList *tmplist;
		gint i, numitems=(sizeof(tbi)/sizeof(Ttoolbaritem));
		tmplist = g_list_first(main_v->globses.quickbar_items);
		while (tmplist) {
			gchar *tmpstr = tmplist->data;
			DEBUG_MSG("make_html_toolbar, searching for %s\n", tmpstr);
			for (i=0;i<numitems;i++) {
				if (strcmp(tbi[i].ident, tmpstr)==0) {
					Tquickbaritem *qbi = g_new(Tquickbaritem,1);
					qbi->button = gtk_toolbar_append_item(GTK_TOOLBAR(bfwin->toolbar_quickbar), NULL, _(tbi[i].tooltiptext),
						"", new_pixmap(tbi[i].pixmaptype), G_CALLBACK(tbi[i].func), bfwin);
					g_signal_connect(qbi->button, "button-press-event", G_CALLBACK(html_toolbar_quickbar_item_button_press_lcb), &tbi[i]);
					qbi->tbitem = &tbi[i];
					bfwin->toolbar_quickbar_children = g_list_append(bfwin->toolbar_quickbar_children, qbi);
					DEBUG_MSG("make_html_toolbar, appended %s as quickbaritem %p to quickbar %p\n",qbi->tbitem->ident, qbi,bfwin->toolbar_quickbar);
					break;
				}
			}
			tmplist = g_list_next(tmplist);
		}
	}
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), bfwin->toolbar_quickbar, gtk_label_new(_(" Quick bar ")));

	html_toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(html_toolbar), GTK_TOOLBAR_ICONS);
	html_toolbar_add_items(bfwin,html_toolbar, tbi, 0, 14);
	html_toolbar_add_items_to_submenu(bfwin,html_toolbar, tbi, 26, 31, _("Heading"), 91);
	html_toolbar_add_items(bfwin,html_toolbar, tbi, 76, 78);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" Standard bar ")));

	html_toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(html_toolbar), GTK_TOOLBAR_ICONS);
	html_toolbar_add_items(bfwin,html_toolbar, tbi, 15, 25);
	html_toolbar_add_items_to_submenu(bfwin,html_toolbar, tbi, 79, 86, _("Context formatting"), 103);
	html_toolbar_add_items(bfwin,html_toolbar, tbi, 25, 31);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" Fonts ")));

	html_toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(html_toolbar), GTK_TOOLBAR_ICONS);
	html_toolbar_add_items(bfwin,html_toolbar, tbi, 32, 43);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" Tables ")));

	html_toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(html_toolbar), GTK_TOOLBAR_ICONS);
	html_toolbar_add_items(bfwin,html_toolbar, tbi, 44, 52);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" Frames ")));

	html_toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(html_toolbar), GTK_TOOLBAR_ICONS);
	html_toolbar_add_items(bfwin,html_toolbar, tbi, 53, 63);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" Forms ")));

	html_toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(html_toolbar), GTK_TOOLBAR_ICONS);
	html_toolbar_add_items(bfwin,html_toolbar, tbi, 64, 70);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" List ")));

	html_toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(html_toolbar), GTK_TOOLBAR_ICONS);
	html_toolbar_add_items(bfwin,html_toolbar, tbi, 71, 73);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" CSS ")));

	gtk_widget_show_all(html_notebook);
	
	if (main_v->globses.quickbar_items == NULL) {
		gtk_notebook_set_current_page (GTK_NOTEBOOK (html_notebook), 1);
	}
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
#ifdef HAVE_ATLEAST_GTK_2_4
   gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_UNINDENT, _("Shift Left"), 
                        "", G_CALLBACK (doc_unindent_lcb), bfwin, -1);
   gtk_toolbar_insert_stock (GTK_TOOLBAR( toolbar), GTK_STOCK_INDENT, _("Shift Right"),
                        "", G_CALLBACK (doc_indent_lcb), bfwin, -1);
#else
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Shift Left"),
                        "", new_pixmap(111), G_CALLBACK(doc_unindent_lcb), bfwin);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Shift Right"),
                        "", new_pixmap(110), G_CALLBACK(doc_indent_lcb), bfwin);
#endif
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
	setup_toggle_item(tmp1,("/Document/Highlight Syntax"), (doc->highlightstate && doc->hl->highlightlist != NULL));
	/*gtk_widget_set_sensitive(gtk_item_factory_get_widget(tmp1,_("/Document/Highlight Syntax")), (doc->hl->highlightlist != NULL));*/
	gui_set_undo_redo_widgets(doc->bfwin, doc_has_undo_list(doc), doc_has_redo_list(doc));
	setup_toggle_item(gtk_item_factory_from_widget(BFWIN(doc->bfwin)->menubar),"/Document/Wrap", doc->wrapstate);
	setup_toggle_item(gtk_item_factory_from_widget(BFWIN(doc->bfwin)->menubar),"/Document/Line Numbers", doc->linenumberstate);
	setup_toggle_item(gtk_item_factory_from_widget(BFWIN(doc->bfwin)->menubar),"/Document/Auto Close HTML tags", doc->autoclosingtag);
	menu_current_document_set_toggle_wo_activate(BFWIN(doc->bfwin),doc->hl, doc->encoding);
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
	gchar *filename, *url;
	int mode = 0;
	gint url_is_local;

	if ((data->length == 0) || (data->format != 8) || ((info != TARGET_STRING) && (info != TARGET_URI_LIST))) {
		DEBUG_MSG("on_drag_data_cb, currently unknown DnD object, need to do string comparision\n");
		gtk_drag_finish(context, FALSE, TRUE, time);
		return;
	}

	/* netscape sends URL's labelled as string */
	if (info == TARGET_STRING) {
		gchar *stringdata = g_strndup((gchar *)data->data, data->length);
		if (strchr(stringdata, ':')) {
			DEBUG_MSG("on_drag_data_cb, TARGET_STRING contains :, so it's probably an URL\n");
			info = TARGET_URI_LIST;
		}
		g_free(stringdata);
	}

	/* construct both filename and url from the given data */
	if (info == TARGET_STRING) {
		filename = g_strndup((gchar *)data->data, data->length);
		filename = trunc_on_char(trunc_on_char(filename, '\n'), '\r');
		url = g_strconcat("file:", filename, NULL);
		url_is_local = 1;
		DEBUG_MSG("on_drag_data_cb, TARGET_STRING, url=%s\n",url);
	} else { /* TARGET_UTI_LIST*/
		gchar *tmp2;
		gint len;

		url = g_strndup((gchar *)data->data, data->length);
		url = trunc_on_char(trunc_on_char(url, '\n'), '\r');
		if (strncmp(url, "file://", 7) == 0) {
			filename = g_strdup(url+7);
			url_is_local = 1;
			DEBUG_MSG("on_drag_data_cb, TARGET_URI_LIST, url=%s, filename=%s\n",url,filename);
		} else {
			len = strlen(url);
			tmp2 = strrchr(url, '#');
			if (tmp2) {
				len -= strlen(tmp2);
			}
			filename = g_strndup(url, len);
			url_is_local = 0;
			DEBUG_MSG("on_drag_data_cb, TARGET_URI_LIST, url=%s\n",url);
		}
	}
	
	DEBUG_MSG("on_drag_data_cb, filename='%s', url='%s'\n", filename, url);
	if (g_str_has_suffix (url_is_local ? filename : url, ".bfproject")) {
	    project_open_from_file(bfwin, url_is_local ? filename : url);
	} else {
	    doc_new_with_file(bfwin,url_is_local ? filename : url, FALSE, FALSE);
	}

	gtk_drag_finish(context, TRUE, (mode == GDK_ACTION_COPY), time);
	g_free(filename);
	g_free(url);
}

void gui_bfwin_cleanup(Tbfwin *bfwin) {
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
	
	bmark_cleanup(bfwin);
}

void main_window_destroy_lcb(GtkWidget *widget,Tbfwin *bfwin) {
	DEBUG_MSG("main_window_destroy_lcb, started\n");
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
		return !project_save_and_close(bfwin);
	} else {
		if (bfwin->documentlist && test_docs_modified(bfwin->documentlist)) {
			DEBUG_MSG("main_window_delete_event_lcb, we have changed documents!\n");
			bfwin_close_all_documents(bfwin, TRUE);
			if (bfwin->documentlist && test_docs_modified(bfwin->documentlist)) {
				DEBUG_MSG("main_window_delete_event_lcb, we STILL have changed documents!?!\n");
				/* if there are still documents modified we should cancel the closing */
				return TRUE;
			}
		} else {
			/* no changed documents, but there might be changed bookmarks */
			bmark_store_all(bfwin);
		}
	}
	return FALSE;
}

void gui_create_main(Tbfwin *bfwin, GList *filenames) {
	GtkWidget *vbox;
	DEBUG_MSG("gui_create_main, bfwin=%p, bfwin->bookmarkstore=%p\n",bfwin,bfwin->bookmarkstore);
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

	/* first a menubar */
	DEBUG_MSG("gui_create_main, starting menu_create_main\n");
	menu_create_main(bfwin, vbox);
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
		gtk_box_pack_start(GTK_BOX(vbox), bfwin->main_toolbar_hb, FALSE, FALSE, 0);
		bfwin->html_toolbar_hb = gtk_handle_box_new();
		gtk_box_pack_start(GTK_BOX(vbox), bfwin->html_toolbar_hb, FALSE, FALSE, 0);
		bfwin->custom_menu_hb = gtk_handle_box_new();
		gtk_box_pack_start(GTK_BOX(vbox), bfwin->custom_menu_hb, FALSE, FALSE, 0);

		if (bfwin->session->view_main_toolbar) {
			make_main_toolbar(bfwin);
			gtk_widget_show(bfwin->main_toolbar_hb);
		}
		if (bfwin->session->view_html_toolbar) {
			make_html_toolbar(bfwin);
			gtk_widget_show(bfwin->html_toolbar_hb);
		}
		if (bfwin->session->view_custom_menu) {
			make_cust_menubar(bfwin,bfwin->custom_menu_hb);
			gtk_widget_show(bfwin->custom_menu_hb);
		}
	}
	
	/* then the work area */
	bfwin->middlebox = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), bfwin->middlebox, TRUE, TRUE, 0);
	gtk_widget_show(bfwin->middlebox);

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

	/* output_box */
	init_output_box(bfwin, vbox);

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
	/* We have to know when the notebook changes */
	gui_notebook_bind_signals(bfwin);
	
	/* everything is ready - we can start loading documents */
	/* start to open an empty doc */
	file_new_cb(NULL, bfwin);
	if (filenames) {
		DEBUG_MSG("gui_create_main, we have filenames, load them\n");
		docs_new_from_files(bfwin,filenames,(bfwin->project != NULL));
	}

	gtk_notebook_set_current_page(GTK_NOTEBOOK(bfwin->notebook), 0);
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
}

void gui_show_main(Tbfwin *bfwin) {
	/* show all */
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

void gui_set_html_toolbar_visible(Tbfwin *bfwin, gboolean visible, gboolean sync_menu) {
	if (sync_menu) {
		DEBUG_MSG("gui_set_html_toolbar_visible, trying to sync menu\n");
		setup_toggle_item_from_widget(bfwin->menubar, "/View/View HTML Toolbar", visible);
	}
	if (gtk_container_children(GTK_CONTAINER(bfwin->html_toolbar_hb)) == NULL) {
		make_html_toolbar(bfwin);
	}
	widget_set_visible(bfwin->html_toolbar_hb,visible);
}
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
	case 2:
		bfwin->session->view_html_toolbar = active;
		gui_set_html_toolbar_visible(bfwin, active, FALSE);
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
	bfwin->bmark_files = NULL; /* Oskar */
	if (project) {
		bfwin->project = project;
		bfwin->session = project->session;
		bfwin->bookmarkstore = project->bookmarkstore;
		DEBUG_MSG("gui_new_window, bfwin=%p, bfwin->bookmarkstore=%p (from project %p)\n",bfwin,bfwin->bookmarkstore,project);
	} else {
		bfwin->session = main_v->session;
		bfwin->bookmarkstore = main_v->bookmarkstore;
		DEBUG_MSG("gui_new_window, bfwin=%p, bfwin->bookmarkstore=%p (from main_v)\n",bfwin,bfwin->bookmarkstore);
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
