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
#include <time.h> /* nanosleep() */

#include "bluefish.h"
#include "gui.h"
#include "document.h" /* file_new_cb() */
#include "gtk_easy.h"
#include "menu.h" /* menu_create_main(), recent_menu_init() */
#include "bf_lib.h" /* get_int_from_string() */
#include "pixmap.h"  /* new_pixmap(); */
#include "undo_redo.h" /* undo_cb() redo_cb() etc. */
#include "snr2.h" /* search_cb, replace_cb */
#include "html_table.h"
#include "html.h"
#include "html2.h"
#include "html_form.h"

typedef struct {
	GtkWidget *main_toolbar_hb;
	GtkWidget *html_toolbar_hb;
	GtkWidget *custom_menu_hb; /* handle box for custom menu */
	GtkWidget *filebrowse_box;
} Thidegui;

typedef struct {
	GtkWidget *undo;
	GtkWidget *redo;
	GtkWidget *html_toolbar_quickbar; /* the quickbar widget */
	GList *html_toolbar_quickbar_children; /* this list is needed to remove widgets from the quickbar */
} Ttoolbar;

typedef struct {
	GtkWidget *window;
	GtkWidget *label;
} Tsplashscreen;

/******************************/
/* global vars for this module */
/******************************/
static Thidegui hidewidgets;
static Ttoolbar toolbarwidgets;
static Tsplashscreen splashscreen;

/**************************/
/* start of the functions */
/**************************/
void gui_toggle_autoindent_cb(gpointer callback_data,guint action,GtkWidget *widget) {
	main_v->props.autoindent = 1 - main_v->props.autoindent;
}

void gui_toggle_hidewidget_cb(gpointer callback_data,guint action,GtkWidget *widget) {
	GtkWidget *handlebox;
	gint *property;
	switch (action) {
	case 0:
		handlebox = hidewidgets.main_toolbar_hb;
		property = &main_v->props.view_main_toolbar;
		if (g_list_length(gtk_container_children(GTK_CONTAINER(handlebox))) == 0) {
			make_main_toolbar(hidewidgets.main_toolbar_hb);
		}
	break;
	case 1:
		handlebox = hidewidgets.html_toolbar_hb;
		property = &main_v->props.view_html_toolbar;
		if (g_list_length(gtk_container_children(GTK_CONTAINER(handlebox))) == 0) {
			make_html_toolbar(hidewidgets.html_toolbar_hb);
		}
	break;
	case 2:
		handlebox = hidewidgets.custom_menu_hb;
		property = &main_v->props.view_custom_menu;
		if (g_list_length(gtk_container_children(GTK_CONTAINER(handlebox))) == 0) {
			make_cust_menubar(hidewidgets.custom_menu_hb);
		}
	break;
	default:
		g_print("gui_toggle_hidewidget_cb should NEVER be called with action %d\n", action);
		exit(1);
	break;
	}

	if (GTK_WIDGET_VISIBLE(handlebox)) {
		*property = 0;
		gtk_widget_hide(handlebox);
	} else {
		*property = 1;
		gtk_widget_show(handlebox);
	}

}
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
		}
	}
	DEBUG_MSG("notebook_changed, done\n");
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

typedef struct {
	gchar *ident;
	void *func;
	gpointer func_data;
	gint pixmaptype;
	gchar *tooltiptext;
} Ttoolbaritem;

typedef struct {
	Ttoolbaritem *tbitem;
	GtkWidget *button;
}Tquickbaritem;

static Ttoolbaritem tbi[] = {
	{"quickstart...", quickstart_cb, NULL,100 , N_("QuickStart...")},
	{"body...", body_cb, NULL, 101, N_("Body...")},
	{"bold", general_html_cb, GINT_TO_POINTER(1), 102, N_("Bold")},
	{"italic", general_html_cb, GINT_TO_POINTER(2), 103, N_("Italic")},
	{"paragraph", general_html_cb, GINT_TO_POINTER(5), 106, N_("Paragraph")},
	{"break", general_html_cb, GINT_TO_POINTER(6), 107, N_("Break")},
	{"breakclear", general_html_cb, GINT_TO_POINTER(41), 108, N_("Break and clear")},
	{"nbsp", general_html_cb, GINT_TO_POINTER(7), 110, N_("Non-breaking space")},
	{"", NULL, NULL, 0, NULL}, /* spacing */
	{"anchor...", quickanchor_cb, NULL,111 , N_("Anchor...")},
	{"rule...", quickrule_cb, NULL, 112, N_("Rule...")},
	{"center", general_html_cb, GINT_TO_POINTER(8), 114, N_("Center")},
	{"rightjustify", general_html_cb, GINT_TO_POINTER(9), 115, N_("Right Justify")},
	{"comment", general_html_cb, GINT_TO_POINTER(10),116 , N_("Comment")},
	{"email...", email_cb, NULL, 117, N_("E-Mail...")},
	{"font...",fontdialog_cb , NULL, 118, N_("Font...")},
	{"basefont...", basefont_cb, NULL, 119, N_("Base Font Size...")},
	{"", NULL, NULL, 0, NULL}, /* spacing */
	{"fontsize+1", general_html_cb, GINT_TO_POINTER(11),120 , N_("Font Size +1")},
	{"fontsize-1", general_html_cb, GINT_TO_POINTER(12), 121, N_("Font Size -1")},
	{"preformatted", general_html_cb, GINT_TO_POINTER(13), 122, N_("Preformatted Text")},
	{"subscript", general_html_cb, GINT_TO_POINTER(14), 123, N_("Subscript")},
	{"superscript", general_html_cb, GINT_TO_POINTER(15), 124, N_("Superscript")},
	{"strong", general_html_cb, GINT_TO_POINTER(16), 102, N_("Strong")},
	{"emphasis", general_html_cb, GINT_TO_POINTER(17), 103, N_("Emphasis")},
	{"", NULL, NULL, 0, NULL}, /* spacing */
	{"heading1", general_html_cb, GINT_TO_POINTER(18), 127, N_("Heading 1")},
	{"heading2", general_html_cb, GINT_TO_POINTER(19), 128, N_("Heading 2")},
	{"heading3", general_html_cb, GINT_TO_POINTER(20), 129, N_("Heading 3")},
	{"heading4", general_html_cb, GINT_TO_POINTER(21), 130, N_("Heading 4")},
	{"heading5", general_html_cb, GINT_TO_POINTER(22), 131, N_("Heading 5")},
	{"heading6", general_html_cb, GINT_TO_POINTER(23), 132, N_("Heading 6")},
	{"", NULL, NULL, 0, NULL}, /* spacing */
	{"table...",tabledialog_cb, NULL, 134, N_("Table...")},
	{"tablerow...", tablerowdialog_cb, NULL, 135, N_("Table Row...")},
	{"tableheader...", tableheaddialog_cb, NULL, 136, N_("Table Header...")},
	{"tabledata...", tabledatadialog_cb, NULL, 137, N_("Table Data...")},
	{"", NULL, NULL, 0, NULL}, /* spacing */
	{"table", general_html_cb, GINT_TO_POINTER(24), 138, N_("Table")},
	{"tablerow", general_html_cb, GINT_TO_POINTER(25), 139, N_("Table Row")},
	{"tableheader", general_html_cb, GINT_TO_POINTER(26), 140, N_("Table Header")},
	{"tabledata", general_html_cb, GINT_TO_POINTER(27), 141, N_("Table Data")},
	{"tablecaption", general_html_cb, GINT_TO_POINTER(28), 142, N_("Table Caption")},
	{"", NULL, NULL, 0, NULL}, /* spacing */
/*	{"framewizard...",framewizard , NULL, 143, N_("Frame Wizard...")},*/
	{"frameset...", framesetdialog_cb, NULL,144 , N_("Frameset...")},
	{"frame...",framedialog_cb , NULL, 145, N_("Frame...")},
	{"", NULL, NULL, 0, NULL}, /* spacing */
	{"frameset", general_html_cb, GINT_TO_POINTER(29),146 , N_("Frameset")},
	{"frame", general_html_cb, GINT_TO_POINTER(30), 147, N_("Frame")},
	{"noframes", general_html_cb, GINT_TO_POINTER(31), 148, N_("Noframes")},
	{"target", general_html_cb, GINT_TO_POINTER(32), 149, N_("Target")},
	{"form...",formdialog_cb , NULL, 150, N_("Form...")},
	{"inputbutton...", buttondialog_cb, NULL, 151, N_("Input button...")},
	{"text...",textdialog_cb , NULL, 153, N_("Text...")},
	{"hidden...", hiddendialog_cb, NULL, 154, N_("Hidden...")},
	{"textarea...",textareadialog_cb , NULL, 155, N_("Textarea...")},
	{"radiobutton...", radiodialog_cb, NULL, 156, N_("Radio Button...")},
	{"checkbox...", checkdialog_cb, NULL, 157, N_("Check Box...")},
	{"select...", selectdialog_cb, NULL, 158, N_("Select...")},
	{"option...",optiondialog_cb , NULL, 417, N_("Option...")},
	{"optiongroup...", optgroupdialog_cb, NULL, 300, N_("Option group...")},
	{"quicklist...", quicklist_cb, NULL, 159, N_("Quick List...")},
	{"unorderedlist", general_html_cb, GINT_TO_POINTER(33), 160, N_("Unordered List")},
	{"orderedlist", general_html_cb, GINT_TO_POINTER(34), 161, N_("Ordered List")},
	{"listitem", general_html_cb, GINT_TO_POINTER(35), 162, N_("List Item")},
	{"definitionlist", general_html_cb, GINT_TO_POINTER(36), 163, N_("Definition List")},
	{"definitionterm", general_html_cb, GINT_TO_POINTER(37), 164, N_("Definition Term")},
	{"definition", general_html_cb, GINT_TO_POINTER(38), 165, N_("Definition")},
	{"createstylesheet...", new_css_dialog, NULL, 180, N_("Create stylesheet...")},
	{"Span...", span_dialog, NULL,181 , N_("Span...")},
	{"Div...",div_dialog , NULL,182 , N_("Div...")},
	{"Style", general_html_cb, GINT_TO_POINTER(42), 184, N_("Style")},
	{"Link to stylesheet...", link_cb, NULL,189 , N_("Link to stylesheet...")},
/*	{"", general_html_cb, GINT_TO_POINTER(), , N_("")},
	{"", general_html_cb, GINT_TO_POINTER(), , N_("")},

	{"", , NULL, , N_("")},
	{"", , NULL, , N_("")},
	{"", , NULL, , },
	{"", NULL, NULL, 0, NULL},*/
	{"", NULL, NULL, 0, NULL} /* spacing */
};

static void html_toolbar_remove_from_quickbar_lcb(GtkMenuItem *menuitem, Ttoolbaritem *tbitem) {
	GList *tmplist;
	Tquickbaritem *qbi;
	main_v->props.quickbar_items = remove_from_stringlist(main_v->props.quickbar_items, tbitem->ident);
	tmplist = g_list_first(toolbarwidgets.html_toolbar_quickbar_children);
	while (tmplist) {
		qbi = tmplist->data;
		if (qbi->tbitem == tbitem) {
			break;
		}
		tmplist = g_list_next(tmplist);
	}
	gtk_widget_hide(qbi->button);
	gtk_widget_destroy(qbi->button);
	g_free(qbi);
}

static gboolean html_toolbar_quickbar_item_button_press_lcb(GtkWidget *widget,GdkEventButton *bevent,Ttoolbaritem *tbitem) {
	if (bevent->button == 3) {
		GtkWidget *menu = gtk_menu_new ();
		GtkWidget *menuitem = gtk_menu_item_new_with_label(_("Remove from quickbar"));
		DEBUG_MSG("popup for tbitem %p\n", tbitem);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(html_toolbar_remove_from_quickbar_lcb), tbitem);
		gtk_menu_append(GTK_MENU(menu), menuitem);
		gtk_widget_show_all (menu);
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
			  NULL, widget, 0, gtk_get_current_event_time ());
		return TRUE;
	}
	return FALSE;
}


static void html_toolbar_add_to_quickbar_lcb(GtkMenuItem *menuitem, Ttoolbaritem *tbitem) {
	GtkWidget *item;
	Tquickbaritem *qbi;
	DEBUG_MSG("adding tbitem %p to quickbar\n", tbitem);
	main_v->props.quickbar_items = add_to_stringlist(main_v->props.quickbar_items, tbitem->ident);
	qbi = g_new(Tquickbaritem,1);
	qbi->button = gtk_toolbar_append_item(GTK_TOOLBAR(toolbarwidgets.html_toolbar_quickbar), NULL, _(tbitem->tooltiptext),
						"", new_pixmap(tbitem->pixmaptype), G_CALLBACK(tbitem->func), tbitem->func_data);
	g_signal_connect(qbi->button, "button-press-event", html_toolbar_quickbar_item_button_press_lcb, tbitem);
	gtk_widget_show(qbi->button);
	qbi->tbitem = tbitem;
	toolbarwidgets.html_toolbar_quickbar_children = g_list_append(toolbarwidgets.html_toolbar_quickbar_children, qbi);
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

static void html_toolbar_add_items(GtkWidget *html_toolbar, Ttoolbaritem *tbi, gint from, gint to) {
	gint i;
	GtkWidget *item;
	for (i=from;i<=to;i++) {
		if (tbi[i].func == NULL) {
			gtk_toolbar_append_space(GTK_TOOLBAR(html_toolbar));
		} else {
			item = gtk_toolbar_append_item(GTK_TOOLBAR(html_toolbar), NULL, _(tbi[i].tooltiptext),
						"", new_pixmap(tbi[i].pixmaptype), G_CALLBACK(tbi[i].func), tbi[i].func_data);
			g_signal_connect(item, "button-press-event", html_toolbar_item_button_press_lcb, &tbi[i]);
			DEBUG_MSG("adding tbitem %p to html_toolbar\n", &tbi[i]);
		}
	}
}

void make_html_toolbar(GtkWidget *handlebox) {
/*
 * every item with a ... in the tooltip brings up a dialog, every item
 * without the ... in the tooltip will directly insert html code
 *
 */
	GtkWidget *html_toolbar, *html_notebook;
	GtkWidget *menu_bar;	/* for construction of a dropdown menu */
	GtkWidget *menu_item, *sub_menu;

	DEBUG_MSG("make_html_toolbar, started\n");
	html_notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(html_notebook), GTK_POS_TOP);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(html_notebook), TRUE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(html_notebook), TRUE);
	gtk_notebook_set_page(GTK_NOTEBOOK(html_notebook), 0);
	gtk_container_add(GTK_CONTAINER(handlebox), html_notebook);

	toolbarwidgets.html_toolbar_quickbar = gtk_toolbar_new();
	DEBUG_MSG("make_html_toolbar, creating quickbar\n");
	{
		GList *tmplist;
		gint i, numitems=(sizeof(tbi)/sizeof(Ttoolbaritem));
		tmplist = g_list_first(main_v->props.quickbar_items);
		while (tmplist) {
			gchar *tmpstr = tmplist->data;
			DEBUG_MSG("make_html_toolbar, searching for %s\n", tmpstr);
			for (i=0;i<numitems;i++) {
				if (strcmp(tbi[i].ident, tmpstr)==0) {
					Tquickbaritem *qbi = g_new(Tquickbaritem,1);
					qbi->button = gtk_toolbar_append_item(GTK_TOOLBAR(toolbarwidgets.html_toolbar_quickbar), NULL, _(tbi[i].tooltiptext),
						"", new_pixmap(tbi[i].pixmaptype), G_CALLBACK(tbi[i].func), tbi[i].func_data);
					g_signal_connect(qbi->button, "button-press-event", html_toolbar_quickbar_item_button_press_lcb, &tbi[i]);
					qbi->tbitem = &tbi[i];
					toolbarwidgets.html_toolbar_quickbar_children = g_list_append(toolbarwidgets.html_toolbar_quickbar_children, qbi);
					break;
				}
			}
			tmplist = g_list_next(tmplist);
		}
	}
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), toolbarwidgets.html_toolbar_quickbar, gtk_label_new(_(" Quick bar ")));

	html_toolbar = gtk_toolbar_new();
	html_toolbar_add_items(html_toolbar, tbi, 0, 14);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" Standard bar ")));

	html_toolbar = gtk_toolbar_new();
	html_toolbar_add_items(html_toolbar, tbi, 15, 31);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" Fonts ")));

	html_toolbar = gtk_toolbar_new();
	html_toolbar_add_items(html_toolbar, tbi, 32, 42);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" Tables ")));

	html_toolbar = gtk_toolbar_new();
	html_toolbar_add_items(html_toolbar, tbi, 43, 50);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" Frames ")));

	html_toolbar = gtk_toolbar_new();
	html_toolbar_add_items(html_toolbar, tbi, 51, 60);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" Forms ")));

	html_toolbar = gtk_toolbar_new();
	html_toolbar_add_items(html_toolbar, tbi, 61, 67);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" List ")));

	html_toolbar = gtk_toolbar_new();
	html_toolbar_add_items(html_toolbar, tbi, 68, 73);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" CSS ")));


	gtk_widget_show_all(html_notebook);
}

void make_main_toolbar(GtkWidget *handlebox) {
	GtkWidget *toolbar = gtk_toolbar_new ();
	DEBUG_MSG("make_main_toolbar, started\n");
	gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
	gtk_container_add (GTK_CONTAINER (handlebox), toolbar);

	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("New"), "",
							new_pixmap(000), G_CALLBACK(file_new_cb), NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Open..."), "",
							new_pixmap(001), G_CALLBACK(file_open_cb), NULL);

	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Save"), "",
							new_pixmap(002), G_CALLBACK(file_save_cb), NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Save As..."), "",
							new_pixmap(003), G_CALLBACK(file_save_as_cb), NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Close"), "",
							new_pixmap(004), G_CALLBACK(file_close_cb), NULL);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Copy"), "",
							new_pixmap(005), G_CALLBACK(edit_copy_cb), NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Cut"), "",
							new_pixmap(006), G_CALLBACK(edit_cut_cb), NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Paste"), "",
							new_pixmap(007), G_CALLBACK(edit_paste_cb), NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Search..."), "",
							new_pixmap(010), G_CALLBACK(search_cb), NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL,_("Search and Replace..."), "", 
							new_pixmap(011), G_CALLBACK(replace_cb), NULL);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	toolbarwidgets.undo = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Undo"), "",
							new_pixmap(012), G_CALLBACK(undo_cb), NULL);
	toolbarwidgets.redo = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Redo"), "",
							new_pixmap(013), G_CALLBACK(redo_cb), NULL);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
/*	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Configure..."),
							"", new_pixmap(014), G_CALLBACK(configure_cb), NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Print..."), "",
							new_pixmap(015), G_CALLBACK(file_print_cb), NULL);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL, _("Spellcheck..."),
							"", new_pixmap(016), G_CALLBACK(spell_check_cb), NULL);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), NULL,
							_("View in Netscape"), "",
							new_pixmap(170), G_CALLBACK(view_in_netscape_cb), NULL);*/
	gtk_widget_show_all(toolbar);
}

void gui_set_undo_redo_widgets(gboolean undo, gboolean redo) {
	if (main_v->props.view_main_toolbar) {
		gtk_widget_set_sensitive(toolbarwidgets.redo, redo);
		gtk_widget_set_sensitive(toolbarwidgets.undo, undo);
	}
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(gtk_item_factory_from_widget(main_v->menubar), N_("/Edit/Undo")), undo);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(gtk_item_factory_from_widget(main_v->menubar), N_("/Edit/Undo all")), undo);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(gtk_item_factory_from_widget(main_v->menubar), N_("/Edit/Redo")), redo);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(gtk_item_factory_from_widget(main_v->menubar), N_("/Edit/Redo all")), redo);
}

void gui_set_widgets(gboolean undo, gboolean redo, gboolean wrap, gboolean highlight, Thighlightset *hl) {
	gui_set_undo_redo_widgets(undo, redo);
	setup_toggle_item(gtk_item_factory_from_widget(main_v->menubar),N_("/Options/Current document/Highlight syntax"), highlight);
	setup_toggle_item(gtk_item_factory_from_widget(main_v->menubar),N_("/Options/Current document/Wrap"), wrap);
	menu_current_document_type_set_active_wo_activate(hl);
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
	recent_menu_init();
	external_menu_init();

	/* then the toolbars */
	{
		hidewidgets.main_toolbar_hb = gtk_handle_box_new();
		gtk_box_pack_start(GTK_BOX(vbox), hidewidgets.main_toolbar_hb, FALSE, FALSE, 0);
		hidewidgets.html_toolbar_hb = gtk_handle_box_new();
		gtk_box_pack_start(GTK_BOX(vbox), hidewidgets.html_toolbar_hb, FALSE, FALSE, 0);
		hidewidgets.custom_menu_hb = gtk_handle_box_new();
		gtk_box_pack_start(GTK_BOX(vbox), hidewidgets.custom_menu_hb, FALSE, FALSE, 0);

		if (main_v->props.view_main_toolbar) {
			make_main_toolbar(hidewidgets.main_toolbar_hb);
			gtk_widget_show(hidewidgets.main_toolbar_hb);
		}
		if (main_v->props.view_html_toolbar) {
			make_html_toolbar(hidewidgets.html_toolbar_hb);
			gtk_widget_show(hidewidgets.html_toolbar_hb);
		}
		if (main_v->props.view_custom_menu) {
			make_cust_menubar(hidewidgets.custom_menu_hb);
			gtk_widget_show(hidewidgets.custom_menu_hb);
		}
	}
	
	/* then the work area */
	main_v->middlebox = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), main_v->middlebox, TRUE, TRUE, 0);
	gtk_widget_show(main_v->middlebox);


	/* notebook with the text widget in there */
	main_v->notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(main_v->notebook),GTK_POS_BOTTOM);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(main_v->notebook), TRUE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(main_v->notebook), FALSE);
	gtk_notebook_set_tab_hborder(GTK_NOTEBOOK(main_v->notebook), 0);
	gtk_notebook_set_tab_vborder(GTK_NOTEBOOK(main_v->notebook), 0);
	gtk_notebook_popup_enable(GTK_NOTEBOOK(main_v->notebook));

	filebrowser_hide(1);

	/* finally the statusbar */
	{
	GtkWidget *hbox;
	hbox = gtk_hbox_new(FALSE,0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	main_v->statuslabel = gtk_label_new(_(" line    1 "));
	gtk_widget_show(main_v->statuslabel);
	gtk_box_pack_start(GTK_BOX(hbox), main_v->statuslabel, FALSE, FALSE, 0);
	main_v->statusbar = gtk_statusbar_new();
	gtk_widget_show(main_v->statusbar);
	gtk_box_pack_start(GTK_BOX(hbox), main_v->statusbar, TRUE, TRUE, 0);
	}
	
	/* everything is ready - we can start loading documents */
	/* start to open an empty doc */
	file_new_cb(NULL, NULL);
	if (filenames) {
		docs_new_from_files(filenames);
	}
	/* We have to know when the notebook changes */
	gui_notebook_bind_signals();

	gtk_notebook_set_page(GTK_NOTEBOOK(main_v->notebook), 0);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(main_v->notebook), TRUE);
	gtk_widget_show(main_v->notebook);
	/* don't use show_all since some widgets are and should be hidden */
}

void gui_show_main() {
	/* show all */
	DEBUG_MSG("gui_show_main, before show\n");
	/* don't use show_all since some widgets are and should be hidden */
	gtk_widget_show(main_v->main_window);
	flush_queue();
	doc_scroll_to_cursor(main_v->current_document);
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

void go_to_line_from_selection_cb(GtkWidget * widget, gpointer data) {
	gchar *string;
	GtkClipboard* cb;
	gint linenum;

	cb = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	string = gtk_clipboard_wait_for_text(cb);
	if (string) {
		linenum = get_int_from_string(string);
		if (linenum > 0) {
			doc_select_line(main_v->current_document, linenum, TRUE);
		}
	}
}

#ifndef NOSPLASH

void splash_screen_set_label(gchar *label) {
	static struct timespec const req = { 0, 100000000};
	static struct timespec rem;
	gtk_label_set(GTK_LABEL(splashscreen.label),label);
	flush_queue();
	nanosleep(&req, &rem);
}

GtkWidget *start_splash_screen() {
	static struct timespec const req = { 0, 300000000};
	static struct timespec rem;
	GtkWidget *image, *vbox;
	GdkColor color;

	splashscreen.window = window_with_title(CURRENT_VERSION_NAME, GTK_WIN_POS_CENTER_ALWAYS, 0);
	gtk_window_set_decorated(GTK_WINDOW(splashscreen.window), FALSE);
	gtk_window_set_wmclass(GTK_WINDOW(splashscreen.window), "Bluefish", "splash");
	gtk_window_set_resizable(GTK_WINDOW(splashscreen.window),FALSE);
	gtk_widget_set_usize(splashscreen.window, 340, 400);
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
		GdkPixbuf* pixbuf= gdk_pixbuf_new_from_file(BLUEFISH_SPLASH_FILENAME,NULL);
		if (pixbuf) {
			image = gtk_image_new_from_pixbuf(pixbuf);
			gtk_box_pack_end(GTK_BOX(vbox), image, FALSE, FALSE, 0);
			g_object_unref(pixbuf);
			gtk_widget_show(image);
		}
	}

	gtk_widget_show(splashscreen.window);
	flush_queue();
	DEBUG_MSG("start_splash_screen, should be visible\n");
	nanosleep(&req, &rem);
	return splashscreen.window;
}
#endif /* #ifndef NOSPLASH */
