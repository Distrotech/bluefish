/* Bluefish HTML Editor
 * menu.c - the menu(s)
 *
 * this file has content-type: UTF-8
 * and it is important you keep it UTF-8 !!!
 *
 * Copyright (C) 1998-2000 Olivier Sessink, Chris Mazuc and Roland Steinbach
 * Copyright (C) 2000-2002 Olivier Sessink and Roland Steinbach
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
#include <gtk/gtk.h>
#include <stdlib.h>			/* atoi */
#include <string.h>			/* strchr() */
#include <gdk/gdkkeysyms.h>

/* #define DEBUG */

#include "bluefish.h"
#include "bf_lib.h"			/* append_string_to_file() */
#include "bfspell.h"
#include "bookmark.h"
#include "dialog_utils.h"
#include "document.h"
#include "file_dialogs.h"
#include "gtk_easy.h"            /* window_full, bf_stock_ok_button */
#include "gui.h"                 /* go_to_line_win_cb */
#include "menu.h"                /* my own .h file */
#include "pixmap.h"
#include "preferences.h"         /* open_preferences_menu_cb */
#include "project.h"
#include "rcfile.h"              /* rcfile_save_configfile_menu_cb */
#include "snr2.h"                /* search_cb, replace_cb */
#include "stringlist.h"          /* free_stringlist() */
#include "undo_redo.h"           /* undo_cb() redo_cb() etc. */
#include "external_commands.h"
#include "outputbox.h"           /* temporary */
#include "bf-textview.h"

#ifdef HAVE_PYTHON
#include "embed_python.h"
#endif

/*
The callback for an ItemFactory entry can take two forms. If callback_action is zero, it is of the following form:
void callback(void)
otherwise it is of the form:
void callback( gpointer callback_data,guint callback_action, GtkWidget *widget)
callback_data is a pointer to an arbitrary piece of data and is set during the call to gtk_item_factory_create_items().

we want to pass the Tbfwin* so we should never use a callback_action of zero
*/
static void menu_file_operations_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget) {
	switch(callback_action) {
	case 1:
		file_new_cb(NULL,bfwin);
	break;
	case 2:
		file_open_cb(NULL,bfwin);
	break;
	case 3:
		file_open_advanced_cb(NULL,bfwin);
	break;
	case 4:
		doc_reload(bfwin->current_document);
	break;
	case 5:
		file_save_cb(NULL, bfwin);
	break;
	case 6:
		file_save_as_cb(NULL, bfwin);
	break;
	case 7:
		file_move_to_cb(NULL, bfwin);
	break;
	case 8:
		file_save_all_cb(NULL, bfwin);
	break;
	case 9:
		file_close_cb(NULL, bfwin);
	break;
	case 10:
		edit_cut_cb(NULL, bfwin);
	break;
	case 11:
		edit_copy_cb(NULL, bfwin);
	break;
	case 12:
		edit_paste_cb(NULL, bfwin);
	break;
	case 13:
		edit_select_all_cb(NULL, bfwin);
	break;
	case 14:
		search_cb(NULL, bfwin);
	break;
	case 16:
		search_again_cb(NULL, bfwin);
	break;
	case 17:
#ifdef SNRDIALOG2
		snr_dialog_new(bfwin, 1);
#else
		replace_cb(NULL, bfwin);
#endif
	break;
	case 19:
		replace_again_cb(NULL, bfwin);
	break;
	case 20:
		undo_cb(NULL, bfwin);
	break;
	case 21:
		redo_cb(NULL, bfwin);
	break;
	case 22:
		undo_all_cb(NULL, bfwin);
	break;
	case 23:
		redo_all_cb(NULL, bfwin);
	break;
	case 24:
		file_close_all_cb(NULL,bfwin);
	break;
	case 25:
		file_open_url_cb(NULL, bfwin);
	break;
	case 26:
		file_open_from_selection(bfwin);
	break;
	case 27:
		search_from_selection(bfwin);
	break;
#ifdef HAVE_PYTHON
	case 99:
		{
			GtkWidget *dialog = file_chooser_dialog(bfwin, "for testing: choose python file", GTK_FILE_CHOOSER_ACTION_OPEN, NULL, TRUE, FALSE, NULL, FALSE);
			if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
				gchar *file;
				file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
				if (file) pythonRun(bfwin, file);
			}
			gtk_widget_destroy (dialog);
		}
	break;
#endif
	default:
		g_print("menu_file_operations_cb, unknown action, abort!\n");
		exit(123);
	}
}
#ifdef HAVE_LIBASPELL
static void spell_check_menu_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget) {
	spell_check_cb(NULL, bfwin);
}
#endif /* HAVE_LIBASPELL */

static void menu_bmark_operations_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget) {
	switch(callback_action) {
	case 1:
	   bmark_add(bfwin);
	break;
/*	case 2:
	   bmark_add_perm(bfwin);
	break;*/
	default:
			g_print("Bmark action no. %d\n",callback_action);
	}
}

static void toggle_doc_property(Tbfwin *bfwin,guint callback_action, GtkWidget *widget) {
	switch(callback_action) {
	case 1:
		bfwin->current_document->wrapstate = GTK_CHECK_MENU_ITEM(widget)->active;
		doc_set_wrap(bfwin->current_document);
		break;
	case 2:
		bfwin->current_document->linenumberstate = GTK_CHECK_MENU_ITEM(widget)->active;
		document_set_line_numbers(bfwin->current_document, bfwin->current_document->linenumberstate);
		break;
	case 3:
		bfwin->current_document->autoclosingtag = GTK_CHECK_MENU_ITEM(widget)->active;
		BF_TEXTVIEW(bfwin->current_document->view)->tag_autoclose = GTK_CHECK_MENU_ITEM(widget)->active;
		break;
	case 4:
		main_v->props.autoindent = GTK_CHECK_MENU_ITEM(widget)->active;
		break;
	case 5:
		bfwin->current_document->blocksstate = GTK_CHECK_MENU_ITEM(widget)->active;
		document_set_show_blocks(bfwin->current_document, bfwin->current_document->blocksstate);
		break;
	case 6:
		bfwin->current_document->symstate = GTK_CHECK_MENU_ITEM(widget)->active;
		document_set_show_symbols(bfwin->current_document, bfwin->current_document->symstate);
		break;		
	}
}

static GtkItemFactoryEntry menu_items[] = {
	{N_("/_File"), NULL, NULL, 0, "<Branch>"},
	{"/File/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/File/_New"), "<control>n", menu_file_operations_cb, 1, "<StockItem>", GTK_STOCK_NEW},
	{N_("/File/New _Window"), "<shift><control>n", gui_window_menu_cb, 1, "<Item>"},	
	{N_("/File/_Open..."), "<control>O", menu_file_operations_cb, 2, "<StockItem>", GTK_STOCK_OPEN},
	{N_("/File/Open _recent"), NULL, NULL, 0, "<Branch>"},
	{"/File/Open recent/tearoff1", NULL, NULL, 0, "<Tearoff>"},	
	{N_("/File/Open Ad_vanced..."), "<shift><control>O", menu_file_operations_cb, 3, "<Item>"},
	{N_("/File/Open _URL..."), NULL, menu_file_operations_cb, 25, "<Item>"},
	{N_("/File/Open _from selection"), NULL, menu_file_operations_cb, 26, "<Item>"},
	{"/File/sep1", NULL, NULL, 0, "<Separator>"},
	{N_("/File/_Save"), "<control>S", menu_file_operations_cb, 5, "<StockItem>", GTK_STOCK_SAVE},
	{N_("/File/Save _As..."), "<shift><control>S", menu_file_operations_cb, 6, "<StockItem>", GTK_STOCK_SAVE_AS},
	{N_("/File/Sav_e All"), NULL, menu_file_operations_cb, 8, "<Item>"},
	{N_("/File/Rever_t to Saved"), NULL, menu_file_operations_cb, 4, "<StockItem>", GTK_STOCK_REVERT_TO_SAVED},
	{"/File/sep2", NULL, NULL, 0, "<Separator>"},	
	{N_("/File/_Insert..."), NULL, file_insert_menucb, 1, "<Item>"},	
	{N_("/File/Rena_me..."), "F2", menu_file_operations_cb, 7, "<Item>"},
	{"/File/sep3", NULL, NULL, 0, "<Separator>"},
	{N_("/File/_Close"), "<control>w", menu_file_operations_cb, 9, "<StockItem>", GTK_STOCK_CLOSE},
	{N_("/File/Close A_ll"), "<shift><control>w", menu_file_operations_cb, 24, "<Item>"},
	{N_("/File/Close Win_dow"), NULL, gui_window_menu_cb, 2, "<Item>"},
	{"/File/sep4", NULL, NULL, 0, "<Separator>"},
	{N_("/File/_Quit"), "<control>Q", bluefish_exit_request, 0, "<StockItem>", GTK_STOCK_QUIT},
#ifdef HAVE_PYTHON
	{"/File/sep5", NULL, NULL, 0, "<Separator>"},
	{N_("/File/Run Python Script"), NULL, menu_file_operations_cb, 99, "<Item>"},
#endif
	{N_("/_Edit"), NULL, NULL, 0, "<Branch>"},
	{"/Edit/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/Edit/_Undo"), "<control>z", menu_file_operations_cb, 20, "<StockItem>", GTK_STOCK_UNDO},
	{N_("/Edit/_Redo"), "<shift><control>z", menu_file_operations_cb, 21, "<StockItem>", GTK_STOCK_REDO},
	{N_("/Edit/Undo All"), NULL, menu_file_operations_cb, 22, "<Item>"},
	{N_("/Edit/Redo All"), NULL, menu_file_operations_cb, 23, "<Item>"},
	{"/Edit/sep1", NULL, NULL, 0, "<Separator>"},	
	{N_("/Edit/Cu_t"), "<control>x", menu_file_operations_cb, 10, "<StockItem>", GTK_STOCK_CUT},
	{N_("/Edit/_Copy"), "<control>c", menu_file_operations_cb, 11, "<StockItem>", GTK_STOCK_COPY},
	{N_("/Edit/_Paste"), "<control>v", menu_file_operations_cb, 12, "<StockItem>", GTK_STOCK_PASTE},
	{"/Edit/sep2", NULL, NULL, 0, "<Separator>"},
	{N_("/Edit/Select _All"), NULL, menu_file_operations_cb, 13, "<Item>"},
	{"/Edit/sep3", NULL, NULL, 0, "<Separator>"},
	{N_("/Edit/_Find..."), "<control>f", menu_file_operations_cb, 14, "<StockItem>", GTK_STOCK_FIND},
	{N_("/Edit/Find A_gain"), "<control>g", menu_file_operations_cb, 16, "<Item>"},
	{N_("/Edit/Find from selection"), NULL, menu_file_operations_cb, 27, "<Item>"},
	{N_("/Edit/R_eplace..."), "<control>h", menu_file_operations_cb, 17, "<StockItem>", GTK_STOCK_FIND_AND_REPLACE},
	{N_("/Edit/Replace Agai_n"), "<shift><control>h", menu_file_operations_cb, 19, "<Item>"},
	{N_("/Edit/Replace special"), NULL, NULL, 0, "<Branch>"},
	{"/Edit/Replace special/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{"/Edit/sep4", NULL, NULL, 0, "<Separator>"},
	{N_("/Edit/S_hift Right"), NULL, menu_indent_cb, 2, "<StockItem>", GTK_STOCK_INDENT},
	{N_("/Edit/Shift _Left"), NULL, menu_indent_cb, 1, "<StockItem>", GTK_STOCK_UNINDENT},
	{"/Edit/sep5", NULL, NULL, 0, "<Separator>"},
	{N_("/Edit/Add _Bookmark"), "<control>d", menu_bmark_operations_cb, 1, "<Item>"},	
	{"/Edit/sep6", NULL, NULL, 0, "<Separator>"},
	{N_("/Edit/Save Sett_ings"), NULL, rcfile_save_configfile_menu_cb, 0, "<Item>"},
	{N_("/Edit/Save Shortcut _Keys"), NULL, rcfile_save_configfile_menu_cb, 3, "<Item>"},	
	{N_("/Edit/Preference_s"), NULL, open_preferences_menu_cb, 0, "<StockItem>", GTK_STOCK_PREFERENCES},
	{N_("/_View"), NULL, NULL, 0, "<Branch>"},
	{"/View/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/View/View _Main Toolbar"), NULL, gui_toggle_hidewidget_cb, 1, "<ToggleItem>"},
	{N_("/View/View _Custom Menu"), NULL, gui_toggle_hidewidget_cb, 3, "<ToggleItem>"},
	{N_("/View/View _Sidebar"), "F9", gui_toggle_hidewidget_cb, 4, "<ToggleItem>"},
	{N_("/_Document"), NULL, NULL, 0, "<Branch>"},
	{"/Document/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/Document/_Increase Tabsize"), NULL, gui_change_tabsize, 1, "<Item>"},
	{N_("/Document/_Decrease Tabsize"), NULL, gui_change_tabsize, 0, "<Item>"},
	{N_("/Document/_Auto Indent"), NULL, toggle_doc_property, 4, "<ToggleItem>"},
	{"/Document/sep1", NULL, NULL, 0, "<Separator>"},
	{N_("/Document/Auto Close H_TML tags"), "<control>T", toggle_doc_property, 3, "<ToggleItem>"},
	{N_("/Document/_Wrap"), NULL, toggle_doc_property, 1, "<ToggleItem>"},
	{N_("/Document/_Line Numbers"), NULL, toggle_doc_property, 2, "<ToggleItem>"},
	{N_("/Document/Show _blocks"), NULL, toggle_doc_property, 5, "<ToggleItem>"},
	{N_("/Document/Show _symbols"), NULL, toggle_doc_property, 6, "<ToggleItem>"},	
	{"/Document/sep2", NULL, NULL, 0, "<Separator>"},
	{N_("/Document/_Highlight Syntax"), NULL, doc_toggle_highlighting_cb, 1, "<ToggleItem>"},
	{N_("/Document/_Update Highlighting"), "F5", doc_update_highlighting, 0, "<Item>"},
	{"/Document/sep3", NULL, NULL, 0, "<Separator>"},
	{N_("/Document/Document Ty_pe"), NULL, NULL, 0, "<Branch>"},
	{"/Document/Document Type/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/Document/Character _Encoding"), NULL, NULL, 0, "<Branch>"},
	{"/Document/Character Encoding/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{"/Document/sep4", NULL, NULL, 0, "<Separator>"},
#ifdef HAVE_LIBASPELL
	{N_("/Document/Check _Spelling..."), NULL, spell_check_menu_cb, 0, "<StockItem>", GTK_STOCK_SPELL_CHECK},
#endif /* HAVE_LIBASPELL */
	{N_("/Document/_Floating window"), NULL, file_floatingview_menu_cb, 1, "<Item>"},			
	{N_("/Document/Word _Count"), NULL, word_count_cb, 1, "<Item>"},
	{"/Document/sep5", NULL, NULL, 0, "<Separator>"},
	{N_("/Document/Move left"), NULL, gui_notebook_switch, 5, "<Item>"},
	{N_("/Document/Move right"), NULL, gui_notebook_switch, 6, "<Item>"},
	{N_("/_Go"), NULL, NULL, 0, "<Branch>"},
	{"/Go/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/Go/_Previous document"), NULL, gui_notebook_switch, 1, "<StockItem>", GTK_STOCK_GO_BACK},
	{N_("/Go/_Next document"), NULL, gui_notebook_switch, 2, "<StockItem>", GTK_STOCK_GO_FORWARD},
	{"/Go/sep1", NULL, NULL, 0, "<Separator>"},
	{N_("/Go/_First document"), NULL, gui_notebook_switch, 3, "<StockItem>", GTK_STOCK_GOTO_FIRST},
	{N_("/Go/L_ast document"), NULL, gui_notebook_switch, 4, "<StockItem>", GTK_STOCK_GOTO_LAST},
	{"/Go/sep2", NULL, NULL, 0, "<Separator>"},	
	{N_("/Go/Goto _Line"), "<control>l", go_to_line_win_cb, 1, "<StockItem>", GTK_STOCK_JUMP_TO},
	{N_("/Go/Goto _Selection"), NULL, go_to_line_from_selection_cb, 1, "<Item>"},
	{N_("/_Project"), NULL, NULL, 0, "<Branch>"},
	{"/Project/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/Project/_New Project"), NULL, project_menu_cb, 6, "<Item>"},
	{N_("/Project/_Open Project..."), NULL, project_menu_cb, 1, "<Item>"},
	{N_("/Project/Open _recent"), NULL, NULL, 0, "<Branch>"},		
	{"/Project/Open recent/tearoff1", NULL, NULL, 0, "<Tearoff>"},	
	{"/Project/sep1", NULL, NULL, 0, "<Separator>"},
	{N_("/Project/_Save"), NULL, project_menu_cb, 2, "<Item>"},
	{N_("/Project/Save _as..."), NULL, project_menu_cb, 3, "<Item>"},
	{N_("/Project/Save & _close"), NULL, project_menu_cb, 4, "<Item>"},
	{"/Project/sep2", NULL, NULL, 0, "<Separator>"},
	{N_("/Project/E_dit Project Options..."), NULL, project_menu_cb, 5, "<StockItem>", GTK_STOCK_PREFERENCES},
	{N_("/E_xternal"), NULL, NULL, 0, "<Branch>"},
	{"/External/tearoff1", NULL, NULL, 0, "<Tearoff>"}, 
/*	{N_("/External/_Commands"), NULL, NULL, 0, "<Branch>"},
	{"/External/Commands/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/External/_Outputbox"), NULL, NULL, 0, "<Branch>"},
	{"/External/Outputbox/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{"/External/sep1", NULL, NULL, 0, "<Separator>"},*/
};

#ifdef ENABLE_NLS
gchar *menu_translate(const gchar * path, gpointer data) {
	gchar *retval;
/*	static gchar *menupath = NULL;
	if (menupath) g_free(menupath);
	menupath = g_strdup(path);
	if ((strstr(path, "/tearoff1") != NULL) 
			|| (strstr(path, "/---") != NULL) 
			|| (strstr(path, "/sep1") != NULL)) {
		DEBUG_MSG("menu_translate, nogettext returning %s for %s\n", menupath, path);
		return menupath;
	}*/
	retval = gettext(path);
	return retval;
}
#endif       

/************************************************/
/* generic functions for dynamic created menu's */
/************************************************/

typedef struct {
	Tbfwin *bfwin;
	GtkWidget *menuitem;
	gpointer data;
	gulong signal_id;
} Tbfw_dynmenu;
#define BFW_DYNMENU(var) ((Tbfw_dynmenu *)(var))

static Tbfw_dynmenu *find_bfw_dynmenu_by_data_in_list(GList *thelist, gpointer data) {
	GList *tmplist = g_list_first(thelist);
	while (tmplist) {
		if (BFW_DYNMENU(tmplist->data)->data == data) return BFW_DYNMENU(tmplist->data);
		tmplist = g_list_next(tmplist);
	}
	return NULL;
}

/*
NOT IN USE
static Tbfw_dynmenu *find_bfw_dynmenu_by_label_in_list(GList *itemlist, gchar *label) {
	GList *tmplist;

	tmplist = g_list_first(itemlist);
	while (tmplist) {
		if(!strcmp(GTK_LABEL(GTK_BIN(BFW_DYNMENU(tmplist->data)->menuitem)->child)->label, label)) {
			return BFW_DYNMENU(tmplist->data);
		}
		tmplist = g_list_next(tmplist);
	}
	return NULL;
}
*/

static GtkWidget *remove_menuitem_in_list_by_label(const gchar *labelstring, GList **menuitemlist) {
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
static GtkWidget *create_dynamic_menuitem(Tbfwin *bfwin, gchar *menubasepath, const gchar *label, GCallback callback, gpointer data, gint menu_insert_offset) {
	GtkWidget *tmp, *menu;
	GtkItemFactory *factory;

	/* add it to bfwin->menubar */
	factory = gtk_item_factory_from_widget(bfwin->menubar);
	menu = gtk_item_factory_get_widget(factory, menubasepath);
	DEBUG_MSG("create_dynamic_menuitem, menubar=%p, menu=%p basepath=%s, label=%s\n", bfwin->menubar, menu, menubasepath,label);
	if (menu != NULL) {
		tmp = gtk_menu_item_new_with_label(label);
		g_signal_connect(G_OBJECT(tmp), "activate",callback, data);

		gtk_widget_show(tmp);
		if (menu_insert_offset == -1) {
			gtk_menu_shell_append(GTK_MENU_SHELL(menu),tmp);
		} else {
			gtk_menu_shell_insert(GTK_MENU_SHELL(menu),tmp,menu_insert_offset);
		}
		return tmp;
	} else {
		DEBUG_MSG("create_dynamic_menuitem, NO MENU FOR BASEPATH %s\n", menubasepath);
		return NULL;
	}
}

static void create_parent_and_tearoff(gchar *menupath, GtkItemFactory *ifactory) {
	char *basepath;
	GtkWidget *widg=NULL;
	GtkItemFactoryEntry entry;

	basepath = g_strndup(menupath, (strlen(menupath) - strlen(strrchr(menupath, '/'))));
	DEBUG_MSG("create_parent_and_tearoff, basepath=%s for menupath=%s\n", basepath, menupath);
	widg = gtk_item_factory_get_widget(ifactory, basepath);
	if (!widg) {
		DEBUG_MSG("create_parent_and_tearoff, no widget found for %s, will create it\n", basepath);
		create_parent_and_tearoff(basepath, ifactory);
		entry.path = g_strconcat(basepath, "/tearoff1", NULL);
		entry.accelerator = NULL;
		entry.callback = NULL;
		entry.callback_action = 0;
		entry.item_type = "<Tearoff>";
		gtk_item_factory_create_item(ifactory, &entry, NULL, 2);
		g_free(entry.path);
	}
	g_free(basepath);
}	

static void menu_current_document_type_change(GtkMenuItem *menuitem,Tbfw_dynmenu *bdm) {
	DEBUG_MSG("menu_current_document_type_change, started for hlset %p\n", bdm->data);
	if (GTK_CHECK_MENU_ITEM(menuitem)->active) {
		if (doc_set_filetype(bdm->bfwin->current_document, bdm->data)) {
		} else {
			menu_current_document_set_toggle_wo_activate(bdm->bfwin,bdm->bfwin->current_document->hl, NULL);
		}
	}
	doc_set_statusbar_editmode_encoding(bdm->bfwin->current_document);
	DEBUG_MSG("menu_current_document_type_change, finished\n");
}

void filetype_menus_empty() {
	GList *tmplist = g_list_first(main_v->bfwinlist);
	while (tmplist) {
		Tbfwin *bfwin = BFWIN(tmplist->data);
		GList *tmplist2 = g_list_first(bfwin->menu_filetypes);
		while (tmplist2) {
			Tbfw_dynmenu *bdm = BFW_DYNMENU(tmplist2->data);
			g_signal_handler_disconnect(bdm->menuitem,bdm->signal_id);
			gtk_widget_destroy(bdm->menuitem);
			g_free(bdm);
			tmplist2 = g_list_next(tmplist2);
		}
		tmplist = g_list_next(tmplist);
	}
}

void filetype_menu_rebuild(Tbfwin *bfwin,GtkItemFactory *item_factory) {
	GSList *group=NULL;
	GtkWidget *parent_menu;
	GList *tmplist = g_list_last(main_v->filetypelist);
	if (!item_factory) {
		item_factory = gtk_item_factory_from_widget(bfwin->menubar);
	}
	DEBUG_MSG("filetype_menu_rebuild, adding filetypes in menu\n");
	bfwin->menu_filetypes = NULL;
	parent_menu = gtk_item_factory_get_widget(item_factory, N_("/Document/Document Type"));
	while (tmplist) {
		Tfiletype *filetype = (Tfiletype *)tmplist->data;
		if (filetype->cfg) {
			Tbfw_dynmenu *bdm = g_new(Tbfw_dynmenu,1);
			bdm->data = filetype;
			bdm->bfwin = bfwin;
			bdm->menuitem = gtk_radio_menu_item_new_with_label(group, filetype->type);
			bdm->signal_id = g_signal_connect(G_OBJECT(bdm->menuitem), "activate",G_CALLBACK(menu_current_document_type_change), (gpointer) bdm);
			gtk_widget_show(bdm->menuitem);
			gtk_menu_insert(GTK_MENU(parent_menu), bdm->menuitem, 1);
			group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(bdm->menuitem));
			bfwin->menu_filetypes = g_list_append(bfwin->menu_filetypes, bdm);
		}
		tmplist = g_list_previous(tmplist);
	}
}

gboolean   menu_autocomp_run(GtkAccelGroup *accel_group,GObject *acceleratable,
                                             guint keyval,GdkModifierType modifier,gpointer data)
{
	Tbfwin *bfwin = BFWIN(data);
	bf_textview_autocomp_show(BF_TEXTVIEW(bfwin->current_document->view));
	return TRUE;
}    

/* 
 * menu factory crap, thanks to the gtk tutorial for this
 * both the 1.0 and the 1.2 code is directly from the tutorial
 */
void menu_create_main(Tbfwin *bfwin, GtkWidget *vbox) {
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	gint nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
	accel_group = gtk_accel_group_new();
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<bluefishmain>", accel_group);
#ifdef ENABLE_NLS
	gtk_item_factory_set_translate_func(item_factory, menu_translate, "<bluefishmain>", NULL);
#endif
	gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, bfwin);
	gtk_window_add_accel_group(GTK_WINDOW(bfwin->main_window), accel_group);
	bfwin->menubar = gtk_item_factory_get_widget(item_factory, "<bluefishmain>");
	gtk_box_pack_start(GTK_BOX(vbox), bfwin->menubar, FALSE, TRUE, 0);
	gtk_accel_map_add_entry("<bluefishmain>/Edit/Shift Right", GDK_period, GDK_CONTROL_MASK);
	gtk_accel_map_add_entry("<bluefishmain>/Edit/Shift Left", GDK_comma, GDK_CONTROL_MASK);
	gtk_accel_map_add_entry("<bluefishmain>/Go/Previous document", GDK_Page_Up, GDK_CONTROL_MASK);
	gtk_accel_map_add_entry("<bluefishmain>/Go/Next document", GDK_Page_Down, GDK_CONTROL_MASK);
	gtk_accel_map_add_entry("<bluefishmain>/Go/First document", GDK_Page_Up, GDK_SHIFT_MASK | GDK_CONTROL_MASK);
	gtk_accel_map_add_entry("<bluefishmain>/Go/Last document", GDK_Page_Down, GDK_SHIFT_MASK | GDK_CONTROL_MASK);
	gtk_widget_show(bfwin->menubar);
	setup_toggle_item(item_factory, "/View/View Main Toolbar", bfwin->session->view_main_toolbar);
	setup_toggle_item(item_factory, "/View/View Custom Menu", bfwin->session->view_custom_menu);
	setup_toggle_item(item_factory, "/View/View Sidebar", bfwin->session->view_left_panel);
	setup_toggle_item(item_factory, "/Document/Auto Indent", main_v->props.autoindent);
	set_project_menu_widgets(bfwin, FALSE);
	filetype_menu_rebuild(bfwin, item_factory);
	{
		guint key;	
		GdkModifierType mod;
		main_v->autocompletion->closure = g_cclosure_new(G_CALLBACK(menu_autocomp_run),bfwin,NULL);
		main_v->autocompletion->group = accel_group;
		gtk_accelerator_parse(main_v->props.autocomp_key,&key,&mod);
		gtk_accel_group_connect(main_v->autocompletion->group,key,mod,GTK_ACCEL_VISIBLE, main_v->autocompletion->closure);
		
	}
}


/*************************************************************/
/*               Output Box handling                         */
/*************************************************************/
/*static GtkWidget *dynamic_menu_append_spacing(Tbfwin *bfwin, gchar *basepath) {
	GtkItemFactory *factory;
	GtkWidget *menu, *menuitem;
	factory = gtk_item_factory_from_widget(bfwin->menubar);
	menu = gtk_item_factory_get_widget(factory, basepath);
	menuitem = gtk_menu_item_new();
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),menuitem);
	return menuitem;
}*/

static void menu_outputbox_lcb(GtkMenuItem *menuitem,Tbfw_dynmenu *bdm) {
	gchar **arr = (gchar **)bdm->data;
	outputbox(bdm->bfwin,arr[1], atoi(arr[2]), atoi(arr[3]), atoi(arr[4]), arr[5], (arr[6][0]=='1'));
}

/*******************************************************************/
/*               Open Recent menu handling                         */
/*******************************************************************/
/* the only required header */
static GtkWidget *create_recent_entry(Tbfwin *bfwin, const gchar *filename, gboolean is_project, gboolean check_for_duplicates);
/*******************************************************************/

static GtkWidget *remove_recent_entry(Tbfwin *bfwin, const gchar *filename, gboolean is_project) {
	GList *tmplist;
	GList **worklist;
	gpointer tmp;

	worklist = (is_project) ? &bfwin->menu_recent_projects : &bfwin->menu_recent_files;

	if(strcmp(filename, "last") ==0) {
		tmplist = g_list_first(*worklist);
		if (tmplist) {
			tmp = tmplist->data;
			DEBUG_MSG("remove_recent_entry, remove last entry\n");
			*worklist = g_list_remove(*worklist, tmplist->data);
			return tmp;
		} else {
			DEBUG_MSG("remove_recent_entry, worklist contained no items, returning NULL\n");
			return NULL;
		}
	}	else {
		return remove_menuitem_in_list_by_label(filename, worklist);
	}
}

static void open_recent_project_cb(GtkWidget *widget, Tbfwin *bfwin) {
	gchar *filename = GTK_LABEL(GTK_BIN(widget)->child)->label;
	DEBUG_MSG("open_recent_project_cb, started, filename is %s\n", filename);
	project_open_from_file(bfwin, filename);
	add_to_recent_list(bfwin,filename, 0, TRUE);
}

/* open_recent_file
 * This function should be called when a menu from the Open Recent list
 * has been selected. */
static void open_recent_file_cb(GtkWidget *widget, Tbfwin *bfwin) {
	GnomeVFSURI *uri;
	uri = gnome_vfs_uri_new(GTK_LABEL(GTK_BIN(widget)->child)->label);
	DEBUG_MSG("open_recent_file_cb, started, filename is %s\n", GTK_LABEL(GTK_BIN(widget)->child)->label);
	doc_new_from_uri(bfwin, uri, NULL, FALSE, FALSE, -1, -1);
	gnome_vfs_uri_unref(uri);
	add_to_recent_list(bfwin,GTK_LABEL(GTK_BIN(widget)->child)->label, 0, FALSE);
}

/* create_recent_entry
 * This function builds the gtkitemfactoryentry and inserts it at the
 * bfwin->menubar. Furthermore, it returns a pointer to it, so that
 * this pointer can be added in the main_v->recent_files list */
static GtkWidget *create_recent_entry(Tbfwin *bfwin, const gchar *filename, gboolean is_project, gboolean check_for_duplicates) {
	GtkWidget *tmp;

	if (check_for_duplicates) {
		tmp = remove_recent_entry(bfwin,filename,is_project);
		if (tmp) {
			gtk_widget_hide(tmp);
			gtk_widget_destroy(tmp);
		}
	}
	if (is_project) {
		return  create_dynamic_menuitem(bfwin,N_("/Project/Open recent")
			, filename, G_CALLBACK(open_recent_project_cb), bfwin
			, 1);
	} else {
		return  create_dynamic_menuitem(bfwin,N_("/File/Open recent")
			, filename, G_CALLBACK(open_recent_file_cb), bfwin
			, 1);
	}
}

GList *recent_menu_from_list(Tbfwin *bfwin, GList *startat, gboolean is_project) {
	GList *retlist=NULL, *tmplist=startat;
	while (tmplist) {
		DEBUG_MSG("recent_menu_init, adding recent project %s\n",(gchar *)tmplist->data);
		retlist = g_list_append(retlist, create_recent_entry(bfwin,tmplist->data,is_project,FALSE));
		tmplist = g_list_next(tmplist);
	}
	return retlist;
}

/* void recent_menu_from_file(Tbfwin *bfwin, gchar *file_name, gboolean is_project) {
	gchar *filename;
	GList *inputlist, *recentfiles=NULL, *tmplist, **worklist;
	gint num;
	worklist = (is_project) ? &bfwin->menu_recent_projects : &bfwin->menu_recent_files;
	/ * empty any existing menu * /
	tmplist = g_list_first(*worklist);
	while (tmplist) {
		gtk_widget_destroy(tmplist->data);
		tmplist = g_list_next(tmplist);
	}
	
	filename = g_strconcat(g_get_home_dir(), file_name, NULL);
	inputlist = get_stringlist(filename, NULL);
	/ * the last entry in inputlist is the most recent file * /
	tmplist = g_list_first(inputlist);
	while (tmplist) {
		recentfiles = add_to_history_stringlist(recentfiles, (gchar *)tmplist->data, TRUE);
		tmplist = g_list_next(tmplist);
	}
	free_stringlist(inputlist);
	num = g_list_length(recentfiles) - main_v->props.max_recent_files;
	*worklist = recent_menu_from_list(bfwin, g_list_nth(recentfiles, (num > 0)?num:0), is_project);
	put_stringlist_limited(filename, recentfiles, main_v->props.max_recent_files);
	free_stringlist(recentfiles);
	g_free(filename);
}*/

/* recent_menu_init()
 * Gets the list of documents from .bluefish/recentlist and inserts
 * it at the File-->Open Recent menu. If the file doesn't exist (probably
 * because this is the first time Bluefish is running) then a menu
 * item telling that no recent files exist will appear */
void recent_menu_init(Tbfwin *bfwin) {
/*	recent_menu_from_file(bfwin, "/."PACKAGE"/recentlist", FALSE);
	recent_menu_from_file(bfwin, "/."PACKAGE"/recentprojects", TRUE);*/
	recent_menu_from_list(bfwin, bfwin->session->recent_files, FALSE);
	recent_menu_from_list(bfwin, main_v->globses.recent_projects, TRUE);
}

/* when a project is opened, the recent menu should show the recent files
from that project */
void recent_menu_init_project(Tbfwin *bfwin) {
	gint num;
	GList *tmplist = g_list_first(bfwin->menu_recent_files);
	while (tmplist) {
		gtk_widget_destroy(tmplist->data);
		tmplist = g_list_next(tmplist);
	}
	num = g_list_length(bfwin->session->recent_files) - main_v->props.max_recent_files;
	bfwin->menu_recent_files = recent_menu_from_list(bfwin, g_list_nth(bfwin->session->recent_files, (num > 0)?num:0), FALSE);
}

/* Add_to_recent_list
 * This should be called when a new file is opened, i.e. from
 * file_open_cb, it adds a new entry which also appears in the
 * menu bar, and (if nessecary) deletes the last entry */
void add_to_recent_list(Tbfwin *bfwin,gchar *filename, gint closed_file, gboolean is_project) {
	DEBUG_MSG("add_to_recent_list, started for %s\n", filename);
	if (closed_file) {
		GList *tmplist = g_list_first(main_v->bfwinlist);
		while (tmplist) {
			Tbfwin *curbfwin = BFWIN(tmplist->data);
			if (!curbfwin->project || curbfwin == bfwin || is_project) {
				GtkWidget *tmp;
				GList **worklist;
				worklist = (is_project) ? &curbfwin->menu_recent_projects : &curbfwin->menu_recent_files;
				
				/* First of all, create the entry and insert it at the list*/
				*worklist = g_list_append(*worklist,create_recent_entry(curbfwin,filename,is_project,TRUE));
	
				DEBUG_MSG("add_to_recent_list, inserted item in menu\n");
				if(g_list_length(*worklist) > main_v->props.max_recent_files) {
					tmp = remove_recent_entry(bfwin,"last",is_project);
					if (tmp) {
						DEBUG_MSG("add_to_recent_list, list too long, entry %s to be deleted\n", GTK_LABEL(GTK_BIN(tmp)->child)->label);
						gtk_widget_hide(tmp);
						gtk_widget_destroy(tmp);
					}
				}
			}
			tmplist = g_list_next(tmplist);
		}
	}
	if (is_project) {
		main_v->globses.recent_projects = add_to_history_stringlist(main_v->globses.recent_projects, filename, FALSE,TRUE);
	} else {
		bfwin->session->recent_files = add_to_history_stringlist(bfwin->session->recent_files, filename, FALSE,TRUE);
	}


/*	} else {
		/ * once we get rid of the other ways to store recent files this will be the only line we still need * /
		if (is_project) {
			main_v->globses.recent_projects = add_to_history_stringlist(main_v->globses.recent_projects, filename, TRUE);
		} else {
			bfwin->session->recent_files = add_to_history_stringlist(bfwin->session->recent_files, filename, TRUE);
		}
		DEBUG_MSG("add_to_recent_list, added to session recent_files, length=%d\n",g_list_length(bfwin->session->recent_files));
		if (bfwin->project) {
			/ * we do nothing when the file is opened, since opened files are anyway opened again in a project * /
		} else {
			gchar *tmpfilename, *recentfile;
			recentfile = g_strconcat(g_get_home_dir(), (is_project) ? "/."PACKAGE"/recentprojects" : "/."PACKAGE"/recentlist", NULL);
			/ * save the new list * /
			tmpfilename = g_strconcat(filename, "\n", NULL);
			DEBUG_MSG("add_to_recent_list, trying to append to %s\n", recentfile);
			append_string_to_file(recentfile, tmpfilename);
			g_free(recentfile);
			g_free(tmpfilename);
		}
		* /
	} */
}
/*****************/
/* Windows !!    */
/*****************/

static void remove_all_window_entries_in_window(Tbfwin *menubfwin) {
	GList *tmplist = g_list_first(menubfwin->menu_windows);
	DEBUG_MSG("removing all window entries in menubfwin %p\n",menubfwin);
	while (tmplist) {
		Tbfw_dynmenu *bdm = BFW_DYNMENU(tmplist->data);
		/*g_signal_handler_disconnect(bdm->menuitem,bdm->signal_id);*/
		DEBUG_MSG("remove_all_window_entries_in_window, destroy menuitem=%p\n",bdm->menuitem);
		gtk_widget_destroy(bdm->menuitem);
		g_free(bdm);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(menubfwin->menu_windows);
	menubfwin->menu_windows = NULL;
}
static void remove_window_entry_from_window(Tbfwin *menubfwin, Tbfwin *tobfwin) {
	Tbfw_dynmenu *bdm = find_bfw_dynmenu_by_data_in_list(menubfwin->menu_windows, tobfwin);
	DEBUG_MSG("remove_window_entry_from_window, menuwin=%p, found bdm=%p\n",menubfwin,bdm);
	if (bdm) {
		/*g_signal_handler_disconnect(bdm->menuitem,bdm->signal_id);*/
		DEBUG_MSG("remove_window_entry_from_window, destroy menuitem=%p\n",bdm->menuitem);
		gtk_widget_destroy(bdm->menuitem);
		menubfwin->menu_windows = g_list_remove(menubfwin->menu_windows,bdm);
		g_free(bdm);
		
	}
}
static void rename_window_entry_from_window(Tbfwin *menubfwin, Tbfwin *tobfwin, gchar *newtitle) {
	Tbfw_dynmenu *bdm = find_bfw_dynmenu_by_data_in_list(menubfwin->menu_windows, tobfwin);
	DEBUG_MSG("rename_window_entry_from_window, menubfwin=%p, found bdm=%p\n",menubfwin,bdm);
	if (bdm) {
		GtkWidget *label = gtk_bin_get_child(GTK_BIN(bdm->menuitem));
		DEBUG_MSG("rename_window_entry_from_window, setting label to have title %s\n",newtitle);
		gtk_label_set_text(GTK_LABEL(label), newtitle);
	}
}	
static void menu_window_lcb(GtkWidget *widget, Tbfw_dynmenu *bdm) {
	gtk_window_present(GTK_WINDOW(BFWIN(bdm->data)->main_window));
}
static void add_window_entry(Tbfwin *menubfwin, Tbfwin *tobfwin) {
	const gchar *winname;
	Tbfw_dynmenu *bdm = g_new(Tbfw_dynmenu,1);
	bdm->bfwin = menubfwin;
	bdm->data = tobfwin;
	winname = gtk_window_get_title(GTK_WINDOW(tobfwin->main_window));
	DEBUG_MSG("add_window_entry, menubfwin=%p, bdm=%p with title %s\n",menubfwin,bdm,winname);
	bdm->menuitem = create_dynamic_menuitem(menubfwin,_("/Windows"),winname,G_CALLBACK(menu_window_lcb),(gpointer)bdm,-1);
	DEBUG_MSG("add_window_entry, menuitem=%p\n",bdm->menuitem);
	menubfwin->menu_windows = g_list_append(menubfwin->menu_windows, bdm);
}
void add_window_entry_to_all_windows(Tbfwin *tobfwin) {
	GList *tmplist = g_list_first(main_v->bfwinlist);
	while (tmplist) {
		if (tmplist->data != tobfwin) {
			add_window_entry(BFWIN(tmplist->data), tobfwin);
		}
		tmplist = g_list_next(tmplist);
	}
}
void add_allwindows_entries_to_window(Tbfwin *menubfwin) {
	GList *tmplist = g_list_first(main_v->bfwinlist);
	while (tmplist) {
		if (tmplist->data != menubfwin) {
			add_window_entry(menubfwin, BFWIN(tmplist->data));
		}
		tmplist = g_list_next(tmplist);
	}
}	
void remove_window_entry_from_all_windows(Tbfwin *tobfwin) {
	GList *tmplist = g_list_first(main_v->bfwinlist);
	while (tmplist) {
		remove_window_entry_from_window(BFWIN(tmplist->data), tobfwin);
		tmplist = g_list_next(tmplist);
	}
	remove_all_window_entries_in_window(tobfwin);
}
void rename_window_entry_in_all_windows(Tbfwin *tobfwin, gchar *newtitle) {
	GList *tmplist = g_list_first(main_v->bfwinlist);
	while (tmplist) {
		rename_window_entry_from_window(BFWIN(tmplist->data), tobfwin, newtitle);
		tmplist = g_list_next(tmplist);
	}
}

/*****************/
/* Browsers!!    */
/*****************/
/* static void view_in_browser(Tbfwin *bfwin, gchar *browser) {
	if (bfwin->current_document->uri) {
		Tconvert_table *table, *tmpt;
		gchar *command;
		gchar *curi;
		
		curi = gnome_vfs_uri_to_string(bfwin->current_document->uri,GNOME_VFS_URI_HIDE_PASSWORD);
		table = tmpt = g_new(Tconvert_table, 2);
		tmpt->my_int = 's';
		if (bfwin->project && bfwin->project->webdir 
				&& bfwin->project->basedir && strlen(bfwin->project->webdir)>2
				&& strlen(bfwin->project->basedir)>2 
				&& strncmp(curi, bfwin->project->basedir, strlen(bfwin->project->basedir))==0
				) {
			tmpt->my_char = g_strconcat(bfwin->project->webdir, &curi[strlen(bfwin->project->basedir)], NULL);
		} else {
			tmpt->my_char = g_strdup(curi);
		}
		tmpt++;
		tmpt->my_char = NULL;
		command = replace_string_printflike(browser, table);
		g_free(table->my_char);
		g_free(table);
		DEBUG_MSG("view_in_browser, should start %s now\n", command);
		system(command);
		g_free(command);
		g_free(curi);
	} else {
		message_dialog_new(bfwin->main_window, 
							 	 GTK_MESSAGE_ERROR, 
							 	 GTK_BUTTONS_CLOSE, 
								 _("Could not view file in browser."), 
								 _("File has not been saved\n"));
	}
}*/

void browser_toolbar_cb(GtkWidget *widget, Tbfwin *bfwin) {
    GList *tmplist = g_list_first(main_v->props.external_command);
    if (tmplist && tmplist->data) {
        while (tmplist) {
            gchar **arr = tmplist->data;
            if (arr[2][0] == '1') { /* This is the default browser */
                external_command(bfwin, arr[1]);
                return;
            }
            tmplist = g_list_next(tmplist);
        }
    }
    
    message_dialog_new(bfwin->main_window, 
                       GTK_MESSAGE_INFO,
                       GTK_BUTTONS_CLOSE,
                       _("No default browser found"),
                       _("You can set a default browser by going to:\n" 
                         "Edit->Preferences->External Commands"));
}

/*static void browser_lcb(GtkWidget *widget, Tbfw_dynmenu *bdm) {
	gchar **arr = (gchar **)bdm->data;
/ *	if (!bdm->bfwin->current_document->uri || bdm->bfwin->current_document->modified) {
		file_save_cb(NULL, bdm->bfwin);
	} * /
	view_in_browser(bdm->bfwin,arr[1]);
}*/
static void external_filter_lcb(GtkWidget *widget, Tbfw_dynmenu *bdm) {
	gchar **arr = (gchar **)bdm->data;
	gint begin=0,end=-1;
	/* if we have a selection, and the filter can be used on a selection,
	 we should ask if it should be the complete file or the selection */
	 
	if (operatable_on_selection(arr[1]) && (doc_has_selection(bdm->bfwin->current_document))) {
		GtkWidget *dialog;
		dialog = gtk_message_dialog_new(GTK_WINDOW(bdm->bfwin->main_window),GTK_DIALOG_DESTROY_WITH_PARENT,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("Operate filter only on selection?"));
		
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) {
			doc_get_selection(bdm->bfwin->current_document,&begin,&end);
			g_print("get selection\n");
		}
		g_print("external_filter_lcb, begin=%d, end=%d\n",begin,end);
		gtk_widget_destroy(dialog);
	} 
	DEBUG_MSG("external_filter_lcb, calling external_command for %s\n",arr[1]);
	filter_command(bdm->bfwin, arr[1],begin,end);
}

static void external_command_lcb(GtkWidget *widget, Tbfw_dynmenu *bdm) {
	gchar **arr = (gchar **)bdm->data;
	external_command(bdm->bfwin, arr[1]);
}
/**
 * external_menu_rebuild:
 * @bfwin: #Tbfwin*
 *
 * rebuild the browsers, external commands and outputbox menu's
 *
 * Return value: void
 */
void external_menu_rebuild(Tbfwin *bfwin) {
	GList *tmplist;
	
	/* first cleanup all menu's */
	tmplist = g_list_first(bfwin->menu_external);
	while (tmplist) {
		Tbfw_dynmenu *bdm = (Tbfw_dynmenu *)tmplist->data;
		DEBUG_MSG("external_menu_rebuild,destroying,bfwin=%p,bdm=%p,menuitem=%p\n",bfwin,bdm,bdm->menuitem);
		gtk_widget_destroy(bdm->menuitem);
		g_free(bdm);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(bfwin->menu_external);
	bfwin->menu_external = NULL;

	tmplist = g_list_first(bfwin->menu_outputbox);
	while (tmplist) {
		gtk_widget_destroy(BFW_DYNMENU(tmplist->data)->menuitem);
		g_free(BFW_DYNMENU(tmplist->data));
		tmplist = g_list_next(tmplist);
	}
	g_list_free(bfwin->menu_outputbox);
	bfwin->menu_outputbox = NULL;
	
	create_parent_and_tearoff(N_("/External/Outputbox/"), gtk_item_factory_from_widget(bfwin->menubar));
	create_parent_and_tearoff(N_("/External/Commands/"), gtk_item_factory_from_widget(bfwin->menubar));
	create_parent_and_tearoff(N_("/External/Filters/"), gtk_item_factory_from_widget(bfwin->menubar));

	tmplist = g_list_first(main_v->props.external_filter);
	while (tmplist) {
		gchar **arr = tmplist->data;
		/*  arr[0] = name
		 *  arr[1] = command
		 */
		if (count_array(arr)==2) {
			Tbfw_dynmenu *bdm = g_new(Tbfw_dynmenu,1);
			bdm->bfwin = bfwin;
			bdm->data = arr;
			DEBUG_MSG("external_menu_rebuild,Adding browser %s with command %s to the menu\n", arr[0], arr[1]);
			bdm->menuitem = create_dynamic_menuitem(bfwin,N_("/External/Filters"),arr[0],G_CALLBACK(external_filter_lcb),bdm,-1);
			DEBUG_MSG("external_menu_rebuild,creating,bfwin=%p,bdm=%p,menuitem=%p\n",bfwin,bdm,bdm->menuitem);
			bfwin->menu_external = g_list_append(bfwin->menu_external, bdm);
		}
#ifdef DEBUG
		else {
			DEBUG_MSG("need count=2 for browser menu! %p has count %d\n", arr, count_array(arr));
		}
#endif
		tmplist = g_list_next(tmplist);
	}
	
	tmplist = g_list_first(main_v->props.external_command);
	while (tmplist) {
		gchar **arr = tmplist->data;
		/*  arr[0] = name
		 *  arr[1] = command
		 *  arr[2] = is_default_browser
		 */
		if (count_array(arr)==3) {
			gchar *tmp1;
			Tbfw_dynmenu *bdm = g_new(Tbfw_dynmenu,1);
			if (arr[2][0] == '1') {
				tmp1 = N_("/External");
			} else {
				tmp1 = N_("/External/Commands");
			}
			bdm->bfwin = bfwin;
			bdm->data = arr;
			bdm->menuitem = create_dynamic_menuitem(bfwin,tmp1,arr[0],G_CALLBACK(external_command_lcb),bdm,-1);
			bfwin->menu_external = g_list_append(bfwin->menu_external, bdm);
		}
		tmplist = g_list_next(tmplist);
	}

	/* the outputbox */	

	tmplist = g_list_first(main_v->props.external_outputbox);
	while (tmplist) {
		gchar **arr = tmplist->data;
		/* outputbox(gchar *pattern, gint file_subpat, gint line_subpat, gint output_subpat, gchar *command, gboolean show_all_output)
		 * arr[0] = name
		 * arr[1] = pattern
		 * arr[2] = file subpattern
		 * arr[3] = line subpattern
		 * arr[4] = output subpattern
		 * arr[5] = command
		 * arr[6] = show_all_output
		 */
		if (count_array(arr)==7) {
			Tbfw_dynmenu *bdm = g_new(Tbfw_dynmenu,1);
			bdm->data = arr;
			bdm->bfwin = bfwin;
			bdm->menuitem = create_dynamic_menuitem(bfwin,N_("/External/Outputbox"),arr[0],G_CALLBACK(menu_outputbox_lcb),(gpointer)bdm,-1);
			bfwin->menu_outputbox = g_list_append(bfwin->menu_outputbox,bdm);
		}
		tmplist = g_list_next(tmplist);
	}
}

static void menu_current_document_encoding_change(GtkMenuItem *menuitem,Tbfw_dynmenu *bdm) {
	if (GTK_CHECK_MENU_ITEM(menuitem)->active) {
		gchar *encoding = (gchar *)bdm->data;
		Tbfwin *bfwin = bdm->bfwin;
		DEBUG_MSG("menu_current_document_encoding_change, encoding=%s\n",encoding);
		if (encoding) {
			if ((!bfwin->current_document->encoding || strcmp(encoding,bfwin->current_document->encoding)!=0)) {
				if (bfwin->current_document->encoding) g_free(bfwin->current_document->encoding);
				bfwin->current_document->encoding = g_strdup(encoding);
				if (main_v->props.auto_set_encoding_meta) {
					update_encoding_meta_in_file(bfwin->current_document, bfwin->current_document->encoding);
				}
				doc_set_statusbar_editmode_encoding(bfwin->current_document);
				DEBUG_MSG("menu_current_document_encoding_change, set to %s\n", encoding);
			}
			if (bfwin->session->encoding) g_free(bfwin->session->encoding);
			bfwin->session->encoding = g_strdup(encoding);
			DEBUG_MSG("menu_current_document_encoding_change, session encoding now is %s\n",bfwin->session->encoding);
		}
	}
}

void encoding_menu_rebuild(Tbfwin *bfwin) {
	GSList *group=NULL;
	GtkWidget *parent_menu;
	GList *tmplist;
	tmplist = g_list_first(bfwin->menu_encodings);
	while (tmplist) {
		Tbfw_dynmenu *bdm = tmplist->data;
		gtk_widget_destroy(GTK_WIDGET(bdm->menuitem));
		g_free(bdm);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(bfwin->menu_encodings);
	bfwin->menu_encodings = NULL;

	tmplist = g_list_last(main_v->props.encodings);
	parent_menu = gtk_item_factory_get_widget(gtk_item_factory_from_widget(bfwin->menubar), N_("/Document/Character Encoding"));
	while (tmplist) {
		gchar **strarr = (gchar **)tmplist->data;
		if (count_array(strarr)==2) {
			Tbfw_dynmenu *bdm = g_new(Tbfw_dynmenu,1);
			bdm->menuitem = gtk_radio_menu_item_new_with_label(group, strarr[0]);
			bdm->data = strarr[1];
			bdm->bfwin = bfwin;
			g_signal_connect(G_OBJECT(bdm->menuitem), "activate",G_CALLBACK(menu_current_document_encoding_change), (gpointer) bdm);
			gtk_widget_show(bdm->menuitem);
			gtk_menu_insert(GTK_MENU(parent_menu), bdm->menuitem, 1);
			group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(bdm->menuitem));
			bfwin->menu_encodings = g_list_append(bfwin->menu_encodings, bdm);
		}
		tmplist = g_list_previous(tmplist);
	}
}

void menu_current_document_set_toggle_wo_activate(Tbfwin *bfwin, Tfiletype *filetype, gchar *encoding) {
	Tbfw_dynmenu *bdm = find_bfw_dynmenu_by_data_in_list(bfwin->menu_filetypes, filetype);
	if (bdm && filetype && bdm->menuitem && !GTK_CHECK_MENU_ITEM(bdm->menuitem)->active) {
		DEBUG_MSG("setting widget from hlset %p active\n", bfwin->current_document->hl);
		g_signal_handler_disconnect(G_OBJECT(bdm->menuitem),bdm->signal_id);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(bdm->menuitem), TRUE);
		bdm->signal_id = g_signal_connect(G_OBJECT(bdm->menuitem), "activate",G_CALLBACK(menu_current_document_type_change), (gpointer) bdm);
	}
#ifdef DEBUG
	 else {
	 	DEBUG_MSG("widget from filetype %p is already active, or filetype does not have a widget!!\n", bfwin->current_document->hl);
	 }
#endif
	if (encoding) {
		GList *tmplist;
		tmplist = g_list_first(main_v->props.encodings);
		while (tmplist) {
			gchar **tmparr = (gchar **)tmplist->data;
			if (strcmp(tmparr[1], encoding)==0) {
				Tbfw_dynmenu *bdm = find_bfw_dynmenu_by_data_in_list(bfwin->menu_encodings, tmparr[1]);
				if (bdm) {
					g_signal_handlers_block_matched(G_OBJECT(bdm->menuitem), G_SIGNAL_MATCH_FUNC,
							0, 0, NULL, menu_current_document_encoding_change, NULL);
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(bdm->menuitem),TRUE);
					g_signal_handlers_unblock_matched(G_OBJECT(bdm->menuitem), G_SIGNAL_MATCH_FUNC,
							0, 0, NULL, menu_current_document_encoding_change, NULL);
				}
				break;
			}
			tmplist = g_list_next(tmplist);	
		}
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
	Tbfwin *bfwin;
} Tcust_con_struc;

typedef struct {
	GtkItemFactoryEntry entry;
	gint type;
	gchar **array;
	Tbfwin *bfwin;
} Tcmenu_entry;

/*
instead of having one list where both insert and replace types have their
place, I changed that to 2 arraylists:
main_v->props.cmenu_insert
main_v->props.cmenu_replace

** for insert **
array[0] = title / menupath
array[1] = formatstring before, containing %0, %1... that should be replaced by the 
				values from the dialog
array[2] = formatstring after
array[3] = number of variables from the dialog
array[4..] = the description of those variables

** for replace **
array[0] = title / menupath
array[1] = search pattern, containing %0 etc.
array[2] = replace pattern, containing %0 etc.
array[3] = replace where:
							0 = from beginning
							1 = from cursor
							2 = selection (selection required)
							3 = all open documents
							4 = ask
array[4] = replace type:
							0 = normal
							1 = regular expression
array[5] = case sensitivity:
							0 = no
							1 = yes
array[6] = number of variables from the dialog
array[7..] = the description of those variables
*/

static void cust_con_struc_dialog_destroy_lcb(GtkWidget *widget, Tcust_con_struc *ccs) {
	window_destroy(ccs->dialog);
	g_free(ccs);
}

static void cust_con_struc_dialog_cancel_lcb(GtkWidget *widget, gpointer data) {
	cust_con_struc_dialog_destroy_lcb(NULL, data);
}

static void cust_con_struc_dialog_ok_lcb(GtkWidget *widget, Tcust_con_struc *ccs) {
	Tconvert_table *table, *tmpt;
	gint num_vars, i;

	DEBUG_MSG("cust_con_struc_dialog_ok_lcb, ccs at %p\n", ccs);
	DEBUG_MSG("cust_con_struc_dialog_ok_lcb, array at %p, &array[0]=%p\n", ccs->array, &ccs->array[0]);
	DEBUG_MSG("cust_con_struc_dialog_ok_lcb, array[0] at %p, *array=%p\n", ccs->array[0], *ccs->array);
	if (ccs->type == 0) {
		gchar *before=NULL, *after=NULL;
		num_vars = atoi(ccs->array[3]);
		DEBUG_MSG("cust_con_struc_dialog_ok_lcb, num_vars=%d, ccs->array[3]=%s\n", num_vars, ccs->array[3]);
		table = tmpt = g_new(Tconvert_table, num_vars+2);
		tmpt->my_int = '%';
		tmpt->my_char = g_strdup("%");
		tmpt++;
		for (i=0; i<num_vars; i++) {
			DEBUG_MSG("cust_con_struc_dialog_ok_lcb, tmpt=%p, i=%d\n", tmpt, i);
			tmpt->my_int = 48 + i;
			tmpt->my_char = gtk_editable_get_chars(GTK_EDITABLE(ccs->textentry[i]), 0, -1);
			tmpt++;
		}
		DEBUG_MSG("cust_con_struc_dialog_ok_lcb, setting tmpt(%p) to NULL\n", tmpt);
		tmpt->my_char = NULL;

		if (strlen(ccs->array[1])) {
			DEBUG_MSG("cust_con_struc_dialog_ok_lcb, ccs->array[1]=%s\n",ccs->array[1] );
			before = replace_string_printflike(ccs->array[1], table);
		}
		if (strlen(ccs->array[2])) {
			after = replace_string_printflike(ccs->array[2], table);
		}
		doc_insert_two_strings(ccs->bfwin->current_document, before, after);
		doc_scroll_to_cursor(ccs->bfwin->current_document);
		
		tmpt = table;
		while (tmpt->my_char) {
			DEBUG_MSG("cust_con_struc_dialog_ok_lcb, tmpt=%p, about to free(%p) %s\n", tmpt, tmpt->my_char, tmpt->my_char);
			g_free(tmpt->my_char);
			tmpt++;
		}
		g_free(table);

		if (before) {
			g_free(before);
		}
		if (after) {
			g_free(after);
		}
	} else {
		gchar *search=NULL, *replace=NULL;
		num_vars = atoi(ccs->array[6]);
		table = tmpt = g_new(Tconvert_table, num_vars+1);
		for (i=0; i<num_vars; i++) {
			tmpt->my_int = 48 + i;
			tmpt->my_char = gtk_editable_get_chars(GTK_EDITABLE(ccs->textentry[i]), 0, -1);
			tmpt++;
		}
		tmpt->my_char = NULL;
		if (strlen(ccs->array[1])) {
			DEBUG_MSG("cust_con_struc_dialog_ok_lcb, ccs->array[1]=%s\n",ccs->array[1] );
			search = replace_string_printflike(ccs->array[1], table);
		}
		if (strlen(ccs->array[2])) {
			replace = replace_string_printflike(ccs->array[2], table);
		} else {
			replace = g_strdup("");
		}
		snr2_run_extern_replace(ccs->bfwin->current_document, search, atoi(ccs->array[3]),
				atoi(ccs->array[4]), atoi(ccs->array[5]), replace, TRUE);
		
		tmpt = table;
		while (tmpt->my_char) {
			g_free(tmpt->my_char);
			tmpt++;
		}
		g_free(table);
		
		if (search) {
			g_free(search);
		}
		if (replace) {
			g_free(replace);
		} 
	}
	cust_con_struc_dialog_cancel_lcb(NULL, ccs);
}

static void cust_con_struc_dialog(Tbfwin *bfwin, gchar **array, gint type) {
	Tcust_con_struc *ccs;
   GtkWidget *vbox, *table, *hbox, *okb, *cancb;
	gint i, num_vars;

	ccs = g_malloc(sizeof(Tcust_con_struc));
	ccs->type = type;
	ccs->bfwin = bfwin;
	DEBUG_MSG("cust_con_struc_dialog_cb, ccs at %p\n", ccs);
	ccs->array = array;
	DEBUG_MSG("cust_con_struc_dialog_cb, array at %p, &array[0]=%p\n", ccs->array, &ccs->array[0]);
	DEBUG_MSG("cust_con_struc_dialog_cb, array[0] at %p, *array=%p\n", ccs->array[0], *ccs->array);
	ccs->dialog = window_full2(ccs->array[0], GTK_WIN_POS_MOUSE,  
			5, G_CALLBACK(cust_con_struc_dialog_destroy_lcb), ccs, TRUE, NULL);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(ccs->dialog), vbox);
	DEBUG_MSG("cust_con_struc_dialog_cb, ccs->array[0]=%s\n", ccs->array[0]);
	
	if (type == 0) {
		num_vars = atoi(ccs->array[3]);
	} else {
		num_vars = atoi(ccs->array[6]);
	}
	DEBUG_MSG("cust_con_struc_dialog_cb, num_vars=%d\n", num_vars);

   table = gtk_table_new (num_vars, 2, FALSE);
   gtk_table_set_row_spacings (GTK_TABLE (table), 12);
   gtk_table_set_col_spacings (GTK_TABLE (table), 12);
   gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

   for (i = 0; i < num_vars; i++) {
      gchar *labelTxt = NULL;
      
      if (type == 0) {
         labelTxt = g_strconcat (ccs->array[i+4], ": ", NULL);
      } else {
         labelTxt = g_strconcat (ccs->array[i+7], ": ", NULL); 
      }
      
      ccs->textentry[i] = gtk_entry_new ();
		DEBUG_MSG("cust_con_struc_dialog_cb, textentry[%d]=%p\n", i, ccs->textentry[i]);
		bf_mnemonic_label_tad_with_alignment(labelTxt, ccs->textentry[i], 0, 0.5, table, 0, 1, i, i+1);
		gtk_table_attach (GTK_TABLE (table), ccs->textentry[i], 1, 2, i, i+1, GTK_EXPAND|GTK_FILL, GTK_SHRINK, 0, 0);
		
		g_free (labelTxt);
   }
	
	gtk_box_pack_start(GTK_BOX(vbox), gtk_hseparator_new(), FALSE, FALSE, 12);
	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 12);
	okb = bf_stock_ok_button(G_CALLBACK(cust_con_struc_dialog_ok_lcb), ccs);
	cancb = bf_stock_cancel_button(G_CALLBACK(cust_con_struc_dialog_cancel_lcb), ccs);
	gtk_box_pack_start(GTK_BOX(hbox),cancb , FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox),okb , FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	gtk_widget_grab_focus(ccs->textentry[0]);
	gtk_widget_grab_default(okb);
	gtk_widget_show_all(ccs->dialog);
}


static void cust_menu_lcb(Tcmenu_entry *cmentry,guint callback_action,GtkWidget *widget) {
	if (cmentry->type == 0) {
		DEBUG_MSG("cust_menu_lcb, a custom insert, array[3]=%s\n", cmentry->array[3]);
		if (atoi(cmentry->array[3]) > 0) {
		     cust_con_struc_dialog(cmentry->bfwin,cmentry->array, 0);
		} else {
		     doc_insert_two_strings(cmentry->bfwin->current_document, cmentry->array[1],cmentry->array[2]);
		}
	} else {
		DEBUG_MSG("cust_menu_lcb, a custom replace!, cmentry->array[6]=%s\n", cmentry->array[6]);
		if (strcmp(cmentry->array[3], "2")==0 && !doc_has_selection(cmentry->bfwin->current_document)) {
			message_dialog_new(cmentry->bfwin->main_window, 
							 	 	 GTK_MESSAGE_WARNING, 
							 	 	 GTK_BUTTONS_OK, 
									 _("This custom search and replace requires a selection"), 
									 NULL);
			return;
		}
		if (atoi(cmentry->array[6]) > 0) {
			cust_con_struc_dialog(cmentry->bfwin,cmentry->array, 1);
		} else {
		     snr2_run_extern_replace(cmentry->bfwin->current_document,cmentry->array[1], atoi(cmentry->array[3]),
							atoi(cmentry->array[4]), atoi(cmentry->array[5]), cmentry->array[2],TRUE);
		}
	}
}

static Tcmenu_entry *create_cmentry(Tbfwin *bfwin,const gchar *menupath, gint count, gchar **array, GtkItemFactory *ifactory, gint type) {
	Tcmenu_entry *cmentry = g_malloc0(sizeof(Tcmenu_entry));
	cmentry->bfwin = bfwin;
	cmentry->entry.path = g_strdup(menupath);
	DEBUG_MSG("create_cmentry, entry.path=%s, count=%d\n", cmentry->entry.path, count);
	cmentry->entry.callback = cust_menu_lcb;
	cmentry->entry.callback_action = count;
	cmentry->array = array;
	cmentry->type = type;
	create_parent_and_tearoff(cmentry->entry.path, ifactory);
	gtk_item_factory_create_item(ifactory, &cmentry->entry, cmentry, 1);
	return cmentry;
}

static void fill_cust_menubar(Tbfwin *bfwin) {
	GtkItemFactory *ifactory;
	gint count;
	gchar **splittedstring;
	GList *tmplist;
	Tcmenu_entry *cmentry;

	ifactory = gtk_item_factory_from_widget(bfwin->menu_cmenu);

	tmplist = g_list_first(bfwin->menu_cmenu_entries);
	while (tmplist) {
		cmentry = (Tcmenu_entry *) tmplist->data;
		gtk_item_factory_delete_entry(ifactory, &cmentry->entry);
		DEBUG_MSG("fill_cust_menubar, removed entry.path=%s\n", cmentry->entry.path);
		g_free(cmentry->entry.path);
		g_free(cmentry);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(bfwin->menu_cmenu_entries);
	bfwin->menu_cmenu_entries = NULL;

	count = 0;
	tmplist = g_list_first(main_v->props.cmenu_insert);
	while (tmplist) {
		gint count2;
		splittedstring = (gchar **) tmplist->data;
		count2 = count_array(splittedstring);
		if (count2 >= 4) {
			cmentry = create_cmentry(bfwin,splittedstring[0], count, splittedstring, ifactory, 0);
			bfwin->menu_cmenu_entries = g_list_append(bfwin->menu_cmenu_entries, cmentry);
		}
		count++;
		tmplist = g_list_next(tmplist);
	}
	tmplist = g_list_first(main_v->props.cmenu_replace);
	while (tmplist) {
		gint count2;
		splittedstring = (gchar **) tmplist->data;
		count2 = count_array(splittedstring);
		if (count2 >= 4) {
			cmentry = create_cmentry(bfwin,splittedstring[0], count, splittedstring, ifactory, 1);
			bfwin->menu_cmenu_entries = g_list_append(bfwin->menu_cmenu_entries, cmentry);
		}
		count++;
		tmplist = g_list_next(tmplist);
	}
}

static void cmenu_reset_lcb(Tbfwin *bfwin,guint callback_action,GtkWidget *widget) {
	GList *tmplist;
	rcfile_parse_custom_menu(TRUE, FALSE);
	tmplist = g_list_first(main_v->bfwinlist);
	while (tmplist) {
		fill_cust_menubar(BFWIN(tmplist->data));
		tmplist = g_list_next(tmplist);
	}
}
static void cmenu_load_new_lcb(Tbfwin *bfwin,guint callback_action,GtkWidget *widget) {
	GList *tmplist;
	rcfile_parse_custom_menu(FALSE, TRUE);
	tmplist = g_list_first(main_v->bfwinlist);
	while (tmplist) {
		fill_cust_menubar(BFWIN(tmplist->data));
		tmplist = g_list_next(tmplist);
	}
}
/* function declaration needed here */
void cmenu_editor(Tbfwin *bfwin,guint callback_action,GtkWidget *widget);

void make_cust_menubar(Tbfwin *bfwin, GtkWidget *cust_handle_box) {
	static GtkItemFactoryEntry cust_menu[] = {
		{N_("/_Custom menu"), NULL, NULL, 0, "<Branch>"},
		{"/Custom menu/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Custom menu/Edit custom menu..."), NULL, cmenu_editor, 0, NULL},
		{N_("/Custom menu/Reset"), NULL, cmenu_reset_lcb, 0, NULL},
		{N_("/Custom menu/Load new"), NULL, cmenu_load_new_lcb, 0, NULL}
	};
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	gint nmenu_items = sizeof(cust_menu) / sizeof(cust_menu[0]);

	DEBUG_MSG("make_cust_menubar, started\n");

	/* this should only happen once !!!!!!!!!! */
	accel_group = gtk_accel_group_new();
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<bluefishcustom>", accel_group);
#ifdef ENABLE_NLS
	gtk_item_factory_set_translate_func(item_factory, menu_translate, "<bluefishcustom>", NULL);
#endif
	gtk_item_factory_create_items(item_factory, nmenu_items, cust_menu, bfwin);
	gtk_window_add_accel_group(GTK_WINDOW(bfwin->main_window), accel_group);

	bfwin->menu_cmenu = gtk_item_factory_get_widget(item_factory, "<bluefishcustom>");
	gtk_container_add(GTK_CONTAINER(bfwin->custom_menu_hb), bfwin->menu_cmenu);
	gtk_widget_show(bfwin->menu_cmenu);

	fill_cust_menubar(bfwin);

	DEBUG_MSG("make_cust_menubar, finished\n");
}

/*******************************************************************/
/*               Custom menu editor                                */
/*******************************************************************/
typedef struct {
	GtkWidget *win;
	GtkWidget *type[2];
	GtkListStore *lstore;
	GtkWidget *lview;
	GtkWidget *label1;
	GtkWidget *label2;
	GtkWidget *menupath;
/*	GtkWidget *befv;*/
	GtkTextBuffer *befb;
/*	GtkWidget *aftv;*/
	GtkTextBuffer *aftb;
	GtkWidget *num;
	gchar **lastarray;
	GtkWidget *dynvbox;
	GtkWidget *hboxes[MAX_TEXT_ENTRY];
	GtkWidget *descriptions[MAX_TEXT_ENTRY];
	GtkWidget *csnr_box;
	GtkWidget *region;
	GtkWidget *matching;
	GtkWidget *is_case_sens;
/*	GList *worklist;*/
	GList *worklist_insert;
	GList *worklist_replace;
	Tbfwin *bfwin;
} Tcmenu_editor;

static void cme_destroy_lcb(GtkWidget *widget, Tcmenu_editor* cme) {
	window_destroy(cme->win);
	free_arraylist(cme->worklist_insert);
	free_arraylist(cme->worklist_replace);
	g_free(cme);
}

static void cme_close_lcb(GtkWidget *widget, gpointer data) {
	cme_destroy_lcb(NULL, data);
}

static void cme_ok_lcb(GtkWidget *widget, Tcmenu_editor *cme) {
	GList *tmplist;
	DEBUG_MSG("cme_ok_lcb, start cmenu_insert=%p, worklist_insert=%p\n",main_v->props.cmenu_insert, cme->worklist_insert);
	pointer_switch_addresses((gpointer)&main_v->props.cmenu_insert, (gpointer)&cme->worklist_insert);
	DEBUG_MSG("cme_ok_lcb, after cmenu_insert=%p, worklist_insert=%p\n",main_v->props.cmenu_insert, cme->worklist_insert);
	pointer_switch_addresses((gpointer)&main_v->props.cmenu_replace, (gpointer)&cme->worklist_replace);
	cme_destroy_lcb(NULL, cme);
	tmplist = g_list_first(main_v->bfwinlist);
	while (tmplist) {
		fill_cust_menubar(BFWIN(tmplist->data));
		tmplist = g_list_next(tmplist);
	}
	rcfile_save_custom_menu();
}

static void cme_create_entries(Tcmenu_editor *cme, gint num) {
	gint i;

	for (i = 0; i < MAX_TEXT_ENTRY ; i++) {
		if (i < num) {
			gtk_widget_show(cme->hboxes[i]);
		} else {
			gtk_widget_hide(cme->hboxes[i]); 
		}
	}
}

static gboolean cme_iter_at_pointer(GtkTreeIter *iter, gpointer pointer, Tcmenu_editor *cme) {
	gpointer tmp;
	gboolean cont;
	cont = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(cme->lstore),iter);
	while (cont) {
		gtk_tree_model_get(GTK_TREE_MODEL(cme->lstore),iter,2,&tmp,-1);
		if (pointer == tmp) {
			return TRUE;
		}
		cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(cme->lstore),iter);
	}
	return FALSE;
}

static void cme_lview_selection_changed(GtkTreeSelection *selection, Tcmenu_editor *cme) {
	GtkTreeIter iter;
	GtkTreeModel *model;
	if (gtk_tree_selection_get_selected (selection,&model,&iter)) {
		gint num=0, i;
		gint type=0;

		gtk_tree_model_get(model, &iter, 1, &type, 2, &cme->lastarray, -1);
		
		DEBUG_MSG("cme_clist_select_lcb, lastarray=%p, lastarray[0]=%s, type=%d\n", cme->lastarray, cme->lastarray[0], type);

		DEBUG_MSG("cme_clist_select_lcb, cme->lastarray[0]=%s, [i]='%s'\n", cme->lastarray[0], cme->lastarray[1]);
		gtk_entry_set_text(GTK_ENTRY(cme->menupath), cme->lastarray[0]);

		DEBUG_MSG("cme_clist_select_lcb, cme->lastarray[1]='%s'\n", cme->lastarray[1]);
		gtk_text_buffer_set_text(cme->befb, cme->lastarray[1], -1);

		DEBUG_MSG("cme_clist_select_lcb, cme->lastarray[2]='%s'\n", cme->lastarray[2]);
		gtk_text_buffer_set_text(cme->aftb, cme->lastarray[2], -1);

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cme->type[type]), TRUE);
		if (type == 0) {
			DEBUG_MSG("cme_clist_select_lcb, type=0, custom dialog\n");
			gtk_widget_hide(cme->csnr_box);
		
			num = atoi(cme->lastarray[3]);
			DEBUG_MSG("cme_clist_select_lcb, num=%d\n", num);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(cme->num), num);
	
			cme_create_entries(cme, num);
			DEBUG_MSG("cme_clist_select_lcb, %d entries created\n", num);
			for (i = 0 ; i < num; i++) {
				gtk_entry_set_text(GTK_ENTRY(cme->descriptions[i]), cme->lastarray[i+4]);
			}
			for (i = num ; i < MAX_TEXT_ENTRY; i++) {
				gtk_entry_set_text(GTK_ENTRY(cme->descriptions[i]), "");
			}
		} else if (type == 1) {
			static Tconvert_table table1[] = {{0, "0"}, {1, "1"}, {0, NULL}};
/*			static Tconvert_table table2[] = {{0, N_("in current document")}, {1, N_("from cursor")}, {2, N_("in selection")}, {3, N_("in all open documents")}, {0,NULL}};
			static Tconvert_table table3[] = {{0, N_("normal")}, {1, N_("posix regular expressions")}, {2, N_("perl regular expressions")}, {0, NULL}};*/
			gint converti;
			/*gchar *convertc;*/
			DEBUG_MSG("cme_clist_select_lcb, type=1, custom search and replace\n");
			gtk_widget_show(cme->csnr_box);
			DEBUG_MSG("cme_clist_select_lcb, cme->lastarray[4]=%s\n", cme->lastarray[4]);
			
			/*gtk_editable_delete_text(GTK_EDITABLE(GTK_COMBO(cme->matching)->entry), 0, -1);
			converti = atoi(cme->lastarray[4]);
			convertc = table_convert_int2char(table3, converti);
			if (convertc) {
				gint pos=0;
				gtk_editable_insert_text(GTK_EDITABLE(GTK_COMBO(cme->matching)->entry), convertc, strlen(convertc), &pos);
			}*/
			converti = atoi(cme->lastarray[4]);
			gtk_option_menu_set_history(GTK_OPTION_MENU(cme->matching),converti);

			DEBUG_MSG("cme_clist_select_lcb, cme->lastarray[5]=%s\n", cme->lastarray[5]);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cme->is_case_sens), table_convert_char2int(table1, cme->lastarray[5], tcc2i_full_match));
			
			/*gtk_editable_delete_text(GTK_EDITABLE(GTK_COMBO(cme->region)->entry), 0, -1);
			converti = atoi(cme->lastarray[3]);
			convertc = table_convert_int2char(table2, converti);
			if (convertc) {
				gint pos=0;
				gtk_editable_insert_text(GTK_EDITABLE(GTK_COMBO(cme->region)->entry), convertc, strlen(convertc), &pos);
			}*/
			converti = atoi(cme->lastarray[3]);
			gtk_option_menu_set_history(GTK_OPTION_MENU(cme->region),converti);

			num = atoi(cme->lastarray[6]);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(cme->num), num);
	
			cme_create_entries(cme, num);
			for (i = 0 ; i < num; i++) {
				gtk_entry_set_text(GTK_ENTRY(cme->descriptions[i]), cme->lastarray[i+7]);
			}
			for (i = num ; i < MAX_TEXT_ENTRY; i++) {
				gtk_entry_set_text(GTK_ENTRY(cme->descriptions[i]), "");
			}
		}
		DEBUG_MSG("cme_clist_select_lcb, finished\n");
	} else {
		gint i;
		gtk_entry_set_text(GTK_ENTRY(cme->menupath), "");
		{
			GtkTextIter itstart, itend;
			gtk_text_buffer_get_bounds(cme->befb,&itstart,&itend);
			gtk_text_buffer_delete(cme->befb,&itstart,&itend);
			gtk_text_buffer_get_bounds(cme->aftb,&itstart,&itend);
			gtk_text_buffer_delete(cme->aftb,&itstart,&itend);
		}
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(cme->num), 0);
		for (i = 0 ; i < MAX_TEXT_ENTRY; i++) {
			gtk_entry_set_text(GTK_ENTRY(cme->descriptions[i]), "");
		}
		cme->lastarray = NULL;
		DEBUG_MSG("cme_clist_unselect_lcb, lastarray=%p\n", cme->lastarray);
	}
}

static void cme_spin_changed_lcb(GtkWidget *widget, Tcmenu_editor *cme) {
	cme_create_entries(cme, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(cme->num)));
}

static void cme_type_changed_lcb(GtkWidget *widget, Tcmenu_editor *cme) {
	if (GTK_TOGGLE_BUTTON(cme->type[1])->active) {
		DEBUG_MSG("cme_clist_select_lcb, type[1] is active\n");
		gtk_widget_show(cme->csnr_box);
		gtk_label_set_text(GTK_LABEL(cme->label1), _("Search Pattern"));
		gtk_label_set_text(GTK_LABEL(cme->label2), _("Replace String"));
	} else {
		gtk_widget_hide(cme->csnr_box);
		gtk_label_set_text(GTK_LABEL(cme->label1), _("Formatstring Before"));
		gtk_label_set_text(GTK_LABEL(cme->label2), _("Formatstring After"));
	}
}

static gchar **cme_create_array(Tcmenu_editor *cme, gboolean is_update) {
	gchar **newarray;
	gint num, i, type;
	
	gtk_spin_button_update(GTK_SPIN_BUTTON(cme->num));
	num  = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(cme->num));
	DEBUG_MSG("cme_create_array, num=%d\n", num);
	if (GTK_TOGGLE_BUTTON(cme->type[1])->active) {
		type = 1;
	} else {
		type = 0;
	}
	if (type == 0) {
		newarray = g_malloc0((num+5) * sizeof(char *));
	} else {
		newarray = g_malloc0((num+8) * sizeof(char *));
	}
	DEBUG_MSG("cme_create_array, newarray at %p\n",newarray);
	newarray[0] = gtk_editable_get_chars(GTK_EDITABLE(cme->menupath), 0, -1);
	{
		gboolean invalid=is_update;
		GList *tmplist = g_list_first(cme->worklist_insert);
		while (tmplist) {
			gchar **tmparr = (gchar **)tmplist->data;
			if (strcmp(tmparr[0],newarray[0])==0) {
				/* if it is an update they path should exist already, else is should not */
				invalid = (!is_update);
				break;
			}
			tmplist = g_list_next(tmplist);
		}
		tmplist = g_list_first(cme->worklist_replace);
		while (tmplist) {
			gchar **tmparr = (gchar **)tmplist->data;
			if (strcmp(tmparr[0],newarray[0])==0) {
				/* if it is an update they path should exist already, else is should not */
				invalid = (!is_update);
				break;
			}
			tmplist = g_list_next(tmplist);
		}
		if (invalid) {
			if (is_update) {
				message_dialog_new(cme->bfwin->main_window, 
										 GTK_MESSAGE_WARNING, 
							 	 	 	 GTK_BUTTONS_OK, 
									 	 _("The menupath you want to update does not exist yet"), 
									 	 _("Try 'add' instead."));
			} else {
				message_dialog_new(cme->bfwin->main_window, 
										 GTK_MESSAGE_WARNING, 
							 	 	 	 GTK_BUTTONS_OK, 
									 	 _("The menupath you want to add already exists."), 
									 	 NULL);
			}
		}
		if (newarray[0][0] != '/') {
			DEBUG_MSG("cme_create_array, menupath does not start with slash, returning NULL\n");
			message_dialog_new(cme->bfwin->main_window, 
									 GTK_MESSAGE_WARNING, 
							 	 	 GTK_BUTTONS_OK, 
									 _("The menupath should start with a / character"), 
									 NULL);
			invalid = TRUE;
		}
		if (invalid) {
			g_free(newarray[0]);
			g_free(newarray);
			return (NULL);
		}
	}
	if (type == 0) {
		newarray[3] = gtk_editable_get_chars(GTK_EDITABLE(cme->num), 0, -1);
		for (i = 0 ; i < num; i++) {
			DEBUG_MSG("cme_create_array, adding descriptions[%d] to newarray[%d]\n", i, i+4);
			newarray[4+i] = gtk_editable_get_chars(GTK_EDITABLE(cme->descriptions[i]), 0, -1);
		}
		DEBUG_MSG("cme_create_array, setting newarray[%d] to NULL\n",i+4);
		newarray[4+i] = NULL;
	} else {
/*		static Tconvert_table table2[] = {{0, N_("in current document")}, {1, N_("from cursor")}, {2, N_("in selection")}, {3, N_("in all open documents")}, {0,NULL}};
		static Tconvert_table table3[] = {{0, N_("normal")}, {1, N_("posix regular expressions")}, {2, N_("perl regular expressions")}, {0, NULL}};*/
		gint converti;
/*		gchar *convertc;
		convertc = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(cme->region)->entry), 0, -1);
		converti = table_convert_char2int(table2, convertc, tcc2i_full_match_gettext);
		g_free(convertc);*/
		converti = gtk_option_menu_get_history(GTK_OPTION_MENU(cme->region));
		newarray[3] = g_strdup_printf("%d", converti);

		/*convertc = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(cme->matching)->entry), 0, -1);
		g_free(convertc);*/
		converti = gtk_option_menu_get_history(GTK_OPTION_MENU(cme->matching));
		newarray[4] = g_strdup_printf("%d", converti);
		
		newarray[5] = g_strdup_printf("%d", GTK_TOGGLE_BUTTON(cme->is_case_sens)->active);
	
		newarray[6] = gtk_editable_get_chars(GTK_EDITABLE(cme->num), 0, -1);
		DEBUG_MSG("cme_create_array, newarray[6]=%s\n", newarray[6]);
		for (i = 0 ; i < num; i++) {
			DEBUG_MSG("cme_create_array, adding descriptions[%d] to newarray[%d]\n", i, i+7);
			newarray[7+i] = gtk_editable_get_chars(GTK_EDITABLE(cme->descriptions[i]), 0, -1);
		}
		DEBUG_MSG("cme_create_array, setting newarray[%d] to NULL\n",i+7);
		newarray[7+i] = NULL;
	}
	{
		GtkTextIter itstart, itend;
		gtk_text_buffer_get_bounds(cme->befb,&itstart,&itend);
		newarray[1] = gtk_text_buffer_get_text(cme->befb,&itstart,&itend, FALSE);
		gtk_text_buffer_get_bounds(cme->aftb,&itstart,&itend);
		newarray[2] = gtk_text_buffer_get_text(cme->aftb,&itstart,&itend, FALSE);
	}

	return newarray;
}

static void cme_add_lcb(GtkWidget *widget, Tcmenu_editor *cme) {
	gchar **newarray;
	newarray = cme_create_array(cme, FALSE);
	if (newarray != NULL){
		GtkTreeIter iter;
		GtkTreeSelection *gtsel;
		gint type = GTK_TOGGLE_BUTTON(cme->type[1])->active;
		DEBUG_MSG("cme_add_lcb, adding %p with type %d\n",newarray,type);
		if (type == 0) {
			cme->worklist_insert = g_list_append(cme->worklist_insert, newarray);
		} else {
			cme->worklist_replace = g_list_append(cme->worklist_replace, newarray);
		}
		gtk_list_store_append(GTK_LIST_STORE(cme->lstore),&iter);
		gtk_list_store_set(GTK_LIST_STORE(cme->lstore),&iter,0,newarray[0],1,type,2,newarray,-1);
		cme->lastarray = newarray;
		gtsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(cme->lview));
		gtk_tree_selection_select_iter(gtsel,&iter);
	}
}

static void cme_update_lcb(GtkWidget *widget, Tcmenu_editor *cme) {
	gchar **newarray;
	if (cme->lastarray == NULL) {
		cme_add_lcb(NULL, cme);
		return;
	}
	newarray = cme_create_array(cme, TRUE);
	if (newarray) {
		GtkTreeIter iter;
		if (cme_iter_at_pointer(&iter, cme->lastarray, cme)) {
			gint newtype, oldtype;
			gtk_tree_model_get(GTK_TREE_MODEL(cme->lstore),&iter,1,&oldtype,-1);
			newtype = GTK_TOGGLE_BUTTON(cme->type[1])->active;
			gtk_list_store_set(GTK_LIST_STORE(cme->lstore),&iter,0,newarray[0],1,newtype,2,newarray,-1);
			if (oldtype == 0) {
				if (newtype == 1) {
					cme->worklist_insert = g_list_remove(cme->worklist_insert, cme->lastarray);
					cme->worklist_replace = g_list_append(cme->worklist_replace, newarray);
				} else {
					GList *tmplist = g_list_find(cme->worklist_insert,cme->lastarray);
					tmplist->data = newarray;
				}
			} else if (oldtype == 1) {
				if (newtype == 0) {
					cme->worklist_replace = g_list_remove(cme->worklist_replace, cme->lastarray);
					cme->worklist_insert = g_list_append(cme->worklist_insert, newarray);
				} else {
					GList *tmplist = g_list_find(cme->worklist_replace,cme->lastarray);
					tmplist->data = newarray;
				}
			}
		} else {
			DEBUG_MSG("cme_update_lcb, cannot find iter for pointer %p\n",cme->lastarray);
		}
		g_strfreev(cme->lastarray);
		cme->lastarray = newarray;
	} else {
		DEBUG_MSG ("cme_update_lcb, no new array, cancelled\n");
	}
	DEBUG_MSG ("cme_update_lcb finished\n");
}

static void cme_delete_lcb(GtkWidget *widget, Tcmenu_editor *cme) {
	if (cme->lastarray) {
		GtkTreeIter iter;
		if (cme_iter_at_pointer(&iter, cme->lastarray, cme)) {
			gint type;
			DEBUG_MSG("cme_delete_lcb, removing from listmodel\n");
			gtk_tree_model_get(GTK_TREE_MODEL(cme->lstore),&iter,1,&type,-1);
			if (type == 0) {
				cme->worklist_insert = g_list_remove(cme->worklist_insert, cme->lastarray);
			} else if (type == 1) {
				cme->worklist_replace = g_list_remove(cme->worklist_replace, cme->lastarray);
			} else {
				DEBUG_MSG("NOT removed from lists, type=%d ???\n",type);
			}
			if (type == 1 || type == 0) {
				g_strfreev(cme->lastarray);
			}
			/* removing the iter will call cme_clist_unselect_lcb which will set the lastarray to NULL
			therefore we will do this as last action */
			gtk_list_store_remove(GTK_LIST_STORE(cme->lstore),&iter);
		} else {
			DEBUG_MSG("NOT REMOVED, no iter can be found for pointer %p?!?\n",cme->lastarray);
		}
		DEBUG_MSG("cme_delete_lcb, setting lastarray NULL\n");
		cme->lastarray = NULL;
	} else {
		DEBUG_MSG("cme_delete_lcb, lastarray=NULL, nothing to delete\n");
	}
}

gint menu_entry_sort(gchar ** a,gchar ** b) {
	return strcmp(a[0],b[0]);
}

void cmenu_editor(Tbfwin *bfwin,guint callback_action,GtkWidget *widget) {
	Tcmenu_editor *cme;
	GtkWidget *hbox, *vbox, *frame, *vbox2, *vbox3, *hbox2, *label, *toolbar;
	GList *tmplist;
	gint i;
	gchar *tmpstr;
	
	cme = g_malloc0(sizeof(Tcmenu_editor));
	cme->bfwin = bfwin;
	DEBUG_MSG("cmenu_editor, cme is at %p\n", cme);
	cme->win = window_full2(_("Custom Menu Editor"), GTK_WIN_POS_CENTER, 
							0, G_CALLBACK(cme_destroy_lcb), cme, TRUE, bfwin->main_window);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(cme->win), vbox);
	
	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	
	gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_ADD, _("Add New Menu Entry"),
								NULL, G_CALLBACK(cme_add_lcb), cme, -1);
	gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_APPLY, _("Apply Changes"),
								NULL, G_CALLBACK(cme_update_lcb), cme, -1);
	gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_DELETE, _("Delete Menu Entry"),
								NULL, G_CALLBACK(cme_delete_lcb), cme, -1);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_CLOSE, _("Close Discards Changes"),
								NULL, G_CALLBACK(cme_close_lcb), cme, -1);
	gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_SAVE, _("Save Changes and Exit"),
								NULL, G_CALLBACK(cme_ok_lcb), cme, -1);
	
	vbox2 = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2), 6);
	gtk_box_pack_start(GTK_BOX(vbox), vbox2, TRUE, TRUE, 0);							
	/* upper area */
	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, TRUE, 6);

	label = gtk_label_new_with_mnemonic(_("_Menu Path:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	cme->menupath = gtk_entry_new();
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), cme->menupath);
	gtk_box_pack_start(GTK_BOX(hbox),cme->menupath , TRUE, TRUE, 0);
	
	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, TRUE, TRUE, 6);

	/* clist & type area */
	vbox3 = gtk_vbox_new(FALSE, 12);	
	gtk_box_pack_start(GTK_BOX(hbox), vbox3, TRUE, TRUE, 0);
	{
		GtkWidget *scrolwin;
		GtkTreeViewColumn *column;
		GtkTreeSelection *select;
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();

		cme->lstore = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_POINTER);
		cme->lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(cme->lstore));

		column = gtk_tree_view_column_new_with_attributes (_("Menu path"), renderer,"text", 0,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(cme->lview), column);

		scrolwin = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

		DEBUG_MSG("cmenu_editor, created lstore and lview\n");
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolwin), cme->lview);
		gtk_widget_set_size_request(scrolwin, 180, 250);
		gtk_box_pack_start(GTK_BOX(vbox3), scrolwin, TRUE, TRUE, 0);
		
		select = gtk_tree_view_get_selection(GTK_TREE_VIEW(cme->lview));
		g_signal_connect(G_OBJECT(select), "changed",G_CALLBACK(cme_lview_selection_changed),cme);
	}

	/* dynamic entries area */
	vbox3 = gtk_vbox_new(FALSE, 0);	
	gtk_box_pack_start(GTK_BOX(hbox), vbox3, TRUE, TRUE, 0);

	hbox2 = gtk_hbox_new(FALSE, 12); 
	label = gtk_label_new_with_mnemonic(_("Number of _Variables:"));
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);		
	cme->num = spinbut_with_value(NULL, 0, MAX_TEXT_ENTRY, 1,1);
	g_signal_connect(GTK_OBJECT(cme->num), "changed", G_CALLBACK(cme_spin_changed_lcb), cme);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), cme->num);
	gtk_box_pack_start(GTK_BOX(hbox2),cme->num , FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox3), hbox2, FALSE, FALSE, 0);
	
	frame = gtk_frame_new(_("Variables"));
	gtk_box_pack_end(GTK_BOX(vbox3), frame, TRUE, TRUE, 0);
	cme->dynvbox = gtk_vbox_new(FALSE, 2);	
	gtk_container_add(GTK_CONTAINER(frame), cme->dynvbox);
	for (i = 0; i <  MAX_TEXT_ENTRY; i++) {
		cme->hboxes[i] = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(cme->dynvbox), cme->hboxes[i], FALSE, FALSE, 0);
		tmpstr = g_strdup_printf("%%%d: ", i);
		gtk_box_pack_start(GTK_BOX(cme->hboxes[i]), gtk_label_new(tmpstr), FALSE, FALSE, 0);
		g_free(tmpstr);
		cme->descriptions[i] = gtk_entry_new();
		gtk_box_pack_start(GTK_BOX(cme->hboxes[i]), cme->descriptions[i], TRUE, TRUE, 0);
	}

	/* lower area */
	/* before and after text area */
	vbox3 = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox), vbox3, TRUE, TRUE, 0);
	
	cme->type[0] = gtk_radio_button_new_with_mnemonic(NULL, _("Custom Dialo_g"));
	gtk_box_pack_start(GTK_BOX(vbox3), cme->type[0], FALSE, TRUE, 0);
	cme->type[1] = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(cme->type[0]), _("Custom Replace"));
	gtk_box_pack_start(GTK_BOX(vbox3), cme->type[1], FALSE, TRUE, 0);

	g_signal_connect(GTK_OBJECT(cme->type[0]), "toggled", G_CALLBACK(cme_type_changed_lcb), cme);
	
	/* csnr area */
	cme->csnr_box = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox3), cme->csnr_box, FALSE, TRUE, 12);

	hbox2 = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(cme->csnr_box), hbox2, FALSE, TRUE, 0);
	
	label = gtk_label_new_with_mnemonic(_("_Replace:"));
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);
	{
		gchar *whereoptions[] = {N_("in current document"),N_("from cursor"),N_("in selection"),N_("in all open documents"), NULL};
		cme->region = optionmenu_with_value(whereoptions, 0);
	}
	gtk_box_pack_start(GTK_BOX(hbox2),cme->region , TRUE, TRUE, 3);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), cme->region);

	hbox2 = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(cme->csnr_box), hbox2, TRUE, TRUE, 0);
	label = gtk_label_new_with_mnemonic(_("Matc_hing:"));
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);
	{
		gchar *matchactions[] = {N_("normal"), N_("posix regular expressions"),	N_("perl regular expressions"), NULL};
		cme->matching = optionmenu_with_value(matchactions, 0);
	}
	gtk_box_pack_start(GTK_BOX(hbox2),cme->matching , TRUE, TRUE, 3);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), cme->matching);
	
	cme->is_case_sens = boxed_checkbut_with_value(_("Case Se_nsitive"), 0, cme->csnr_box);

	{
		GtkWidget *scrolwin, *textview;
		cme->label1 = gtk_label_new_with_mnemonic("");
		gtk_box_pack_start(GTK_BOX(vbox3), cme->label1, FALSE, FALSE, 0);

		scrolwin = textview_buffer_in_scrolwin(&textview, -1, -1, NULL, GTK_WRAP_NONE);
		cme->befb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
		gtk_box_pack_start(GTK_BOX(vbox3), scrolwin, TRUE, TRUE, 0);

		cme->label2 = gtk_label_new_with_mnemonic("");
		gtk_box_pack_start(GTK_BOX(vbox3), cme->label2, FALSE, FALSE, 0);
		
		scrolwin = textview_buffer_in_scrolwin(&textview, -1, -1, NULL, GTK_WRAP_NONE);
		cme->aftb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
		gtk_box_pack_start(GTK_BOX(vbox3), scrolwin, TRUE, TRUE, 0);
	}
	
	/* ready !! */
	cme->worklist_insert = duplicate_arraylist(main_v->props.cmenu_insert);
	cme->worklist_replace = duplicate_arraylist(main_v->props.cmenu_replace);
	cme->worklist_insert = g_list_sort(cme->worklist_insert, (GCompareFunc)menu_entry_sort);
	cme->worklist_replace = g_list_sort(cme->worklist_replace, (GCompareFunc)menu_entry_sort);
	{
		GtkTreeIter iter;
		tmplist = g_list_first(cme->worklist_insert);
		while (tmplist) {
			DEBUG_MSG("cmenu_editor, adding type 0 '%s'\n", *(gchar **)tmplist->data);
			gtk_list_store_append(GTK_LIST_STORE(cme->lstore), &iter);
			gtk_list_store_set(GTK_LIST_STORE(cme->lstore), &iter, 0, *(gchar **)tmplist->data, 1, 0, 2, tmplist->data, -1); /* the name , the type, the pointer */
			tmplist = g_list_next(tmplist);
		}
		tmplist = g_list_first(cme->worklist_replace);
		while (tmplist) {
			DEBUG_MSG("cmenu_editor, adding type 1 '%s'\n", *(gchar **)tmplist->data);
			gtk_list_store_append(GTK_LIST_STORE(cme->lstore), &iter);
			gtk_list_store_set(GTK_LIST_STORE(cme->lstore), &iter, 0, *(gchar **)tmplist->data, 1, 1, 2, tmplist->data, -1); /* the name , the type, the pointer */
			tmplist = g_list_next(tmplist);
		}
	}
	gtk_widget_show_all(cme->win);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cme->type[0]), TRUE);
	cme_type_changed_lcb(NULL, cme);
}

/*************************************************************************/
