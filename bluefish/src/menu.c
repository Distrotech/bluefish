/* Bluefish HTML Editor
 * menu.c - the menu(s)
 *
 * this file has content-type: UTF-8
 * and it is important you keep it UTF-8 !!!
 *
 * Copyright (C) 1998-2000 Olivier Sessink, Chris Mazuc and Roland Steinbach
 * Copyright (C) 2000-2002 Olivier Sessink and Roland Steinbach
 * Copyright (C) 2002-2008 Olivier Sessink
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
#include "encodings_dialog.h"
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
#include "blocksync.h"

#ifdef USE_BFTEXTVIEW2
#include "bftextview2.h"
#include "bftextview2_langmgr.h"
#else
#include "bf-textview.h"
#endif

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
		doc_reload(bfwin->current_document,NULL);
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
		snr_dialog_new(bfwin, 1);
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
	case 28:
		blocksync_dialog(bfwin);
	break;
	case 29:
		strip_trailing_spaces(bfwin->current_document);
	break;
	case 30:
		convert_identing(bfwin->current_document, TRUE);
	break;
	case 31:
		convert_identing(bfwin->current_document, FALSE);
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

static void encodings_dialog_menu_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget) {
	bluefish_encodings_dialog_new (bfwin);
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
#ifndef USE_BFTEXTVIEW2
	case 3:
		bfwin->current_document->autoclosingtag = GTK_CHECK_MENU_ITEM(widget)->active;
		BF_TEXTVIEW(bfwin->current_document->view)->tag_autoclose = GTK_CHECK_MENU_ITEM(widget)->active;
		break;
#endif
	case 4:
		BLUEFISH_TEXT_VIEW(bfwin->current_document->view)->autoindent = GTK_CHECK_MENU_ITEM(widget)->active;
		break;
	case 5:
		bfwin->current_document->blocksstate = GTK_CHECK_MENU_ITEM(widget)->active;
		document_set_show_blocks(bfwin->current_document, bfwin->current_document->blocksstate);
		break;
#ifndef USE_BFTEXTVIEW2
	case 6:
		bfwin->current_document->symstate = GTK_CHECK_MENU_ITEM(widget)->active;
		document_set_show_symbols(bfwin->current_document, bfwin->current_document->symstate);
		break;		
#endif
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
	{N_("/Edit/Replace special/Block sync"), NULL, menu_file_operations_cb, 28, "<Item>"},
	{N_("/Edit/Replace special/Strip trailing whitespace"), NULL, menu_file_operations_cb, 29, "<Item>"},
	{N_("/Edit/Replace special/Convert indenting to tabs"), NULL, menu_file_operations_cb, 30, "<Item>"},
	{N_("/Edit/Replace special/Convert indenting to spaces"), NULL, menu_file_operations_cb, 31, "<Item>"},
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
	{N_("/View/Fullscreen"), "F11", gui_fullscreen_cb, 1, "<ToggleItem>"},
	{"/View/sep1", NULL, NULL, 0, "<Separator>"},
	{N_("/View/View _Main Toolbar"), NULL, gui_toggle_hidewidget_cb, 1, "<ToggleItem>"},
	{N_("/View/View _Sidebar"), "F9", gui_toggle_hidewidget_cb, 4, "<ToggleItem>"},
	{N_("/View/View S_tatusbar"), NULL, gui_toggle_hidewidget_cb, 5, "<ToggleItem>"},
	{N_("/_Document"), NULL, NULL, 0, "<Branch>"},
	{"/Document/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/Document/_Increase Tabsize"), NULL, gui_change_tabsize, 1, "<Item>"},
	{N_("/Document/_Decrease Tabsize"), NULL, gui_change_tabsize, 0, "<Item>"},
	{N_("/Document/_Auto Indent"), NULL, toggle_doc_property, 4, "<ToggleItem>"},
	{"/Document/sep1", NULL, NULL, 0, "<Separator>"},
#ifndef USE_BFTEXTVIEW2
	{N_("/Document/Auto Close H_TML tags"), "<control>T", toggle_doc_property, 3, "<ToggleItem>"},
#endif
	{N_("/Document/_Wrap"), NULL, toggle_doc_property, 1, "<ToggleItem>"},
	{N_("/Document/_Line Numbers"), NULL, toggle_doc_property, 2, "<ToggleItem>"},
	{N_("/Document/Show _blocks"), NULL, toggle_doc_property, 5, "<ToggleItem>"},
#ifndef USE_BFTEXTVIEW2
	{N_("/Document/Show _symbols"), NULL, toggle_doc_property, 6, "<ToggleItem>"},	
#endif
	{"/Document/sep2", NULL, NULL, 0, "<Separator>"},
	{N_("/Document/_Highlight Syntax"), NULL, doc_toggle_highlighting_cb, 1, "<ToggleItem>"},
	{N_("/Document/_Update Highlighting"), "F5", doc_update_highlighting, 0, "<Item>"},
	{"/Document/sep3", NULL, NULL, 0, "<Separator>"},
	{N_("/Document/Document Ty_pe"), NULL, NULL, 0, "<Branch>"},
	{"/Document/Document Type/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/Document/Character _Encoding"), NULL, NULL, 0, "<Branch>"},
	{"/Document/Character Encoding/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{"/Document/Character Encoding/sep4", NULL, NULL, 0, "<Separator>"},
	{"/Document/Character Encoding/_Add or Remove...", NULL, encodings_dialog_menu_cb, 1, "<Item>"},
	{"/Document/sep5", NULL, NULL, 0, "<Separator>"},
#ifdef HAVE_LIBASPELL
	{N_("/Document/Check _Spelling..."), NULL, spell_check_menu_cb, 0, "<StockItem>", GTK_STOCK_SPELL_CHECK},
#endif /* HAVE_LIBASPELL */
	{N_("/Document/_Floating window"), NULL, file_floatingview_menu_cb, 1, "<Item>"},			
	{N_("/Document/Word _Count"), NULL, word_count_cb, 1, "<Item>"},
	{"/Document/sep6", NULL, NULL, 0, "<Separator>"},
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
	{N_("/Go/First bookmark"), NULL, bookmark_menu_cb, 1, "<StockItem>", GTK_STOCK_GOTO_TOP},
	{N_("/Go/Previous bookmark"), NULL, bookmark_menu_cb, 2, "<StockItem>", GTK_STOCK_GO_UP},
	{N_("/Go/Next _bookmark"), "<shift><control>d", bookmark_menu_cb, 3, "<StockItem>", GTK_STOCK_GO_DOWN},
	{N_("/Go/Last bookmark"), NULL, bookmark_menu_cb, 4, "<StockItem>", GTK_STOCK_GOTO_BOTTOM},
	{"/Go/sep3", NULL, NULL, 0, "<Separator>"},
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
	
#ifdef USE_BFTEXTVIEW2
	DEBUG_MSG("menu_current_document_type_change, started for bflang %p\n", bdm->data);
	if (GTK_CHECK_MENU_ITEM(menuitem)->active) {
		bluefish_text_view_set_mimetype(BLUEFISH_TEXT_VIEW(bdm->bfwin->current_document->view), ((Tbflang *)bdm->data)->mimetypes->data);
	}

#else
	DEBUG_MSG("menu_current_document_type_change, started for hlset %p\n", bdm->data);
	if (GTK_CHECK_MENU_ITEM(menuitem)->active) {
		if (doc_set_filetype(bdm->bfwin->current_document, bdm->data)) {
		} else {
			menu_current_document_set_toggle_wo_activate(bdm->bfwin,bdm->bfwin->current_document->hl, NULL);
		}
	}
#endif
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
	GList *tmplist;
	if (!item_factory) {
		item_factory = gtk_item_factory_from_widget(bfwin->menubar);
	}
	DEBUG_MSG("filetype_menu_rebuild, adding filetypes in menu\n");
	bfwin->menu_filetypes = NULL;
	parent_menu = gtk_item_factory_get_widget(item_factory, N_("/Document/Document Type"));
#ifdef USE_BFTEXTVIEW2
	tmplist = g_list_first(langmgr_get_languages());
	while (tmplist) {
		Tbflang *bflang = (Tbflang *)tmplist->data;
		Tbfw_dynmenu *bdm = g_new(Tbfw_dynmenu,1);
		bdm->data = bflang;
		bdm->bfwin = bfwin;
		bdm->menuitem = gtk_radio_menu_item_new_with_label(group, bflang->name);
		bdm->signal_id = g_signal_connect(G_OBJECT(bdm->menuitem), "activate",G_CALLBACK(menu_current_document_type_change), (gpointer) bdm);
		gtk_widget_show(bdm->menuitem);
		gtk_menu_insert(GTK_MENU(parent_menu), bdm->menuitem, 1);
		group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(bdm->menuitem));
		bfwin->menu_filetypes = g_list_append(bfwin->menu_filetypes, bdm);
		tmplist = g_list_next(tmplist);
	}
#else
	tmplist = g_list_last(main_v->filetypelist);
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
#endif
}
#ifndef USE_BFTEXTVIEW2
gboolean   menu_autocomp_run(GtkAccelGroup *accel_group,GObject *acceleratable,
                                             guint keyval,GdkModifierType modifier,gpointer data)
{
	Tbfwin *bfwin = BFWIN(data);
	bf_textview_autocomp_show(BF_TEXTVIEW(bfwin->current_document->view));
	return TRUE;
}
#endif 

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
	setup_toggle_item(item_factory, "/View/View Sidebar", bfwin->session->view_left_panel);
	setup_toggle_item(item_factory, "/Document/Auto Indent", main_v->props.autoindent);
	set_project_menu_widgets(bfwin, FALSE);
	filetype_menu_rebuild(bfwin, item_factory);
#ifndef USE_BFTEXTVIEW2
	{
		guint key;	
		GdkModifierType mod;
		main_v->autocompletion->closure = g_cclosure_new(G_CALLBACK(menu_autocomp_run),bfwin,NULL);
		main_v->autocompletion->group = accel_group;
		gtk_accelerator_parse(main_v->props.autocomp_key,&key,&mod);
		gtk_accel_group_connect(main_v->autocompletion->group,key,mod,GTK_ACCEL_VISIBLE, main_v->autocompletion->closure);
		
	}
#endif
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
	GFile *uri;
	uri = g_file_new_for_commandline_arg(GTK_LABEL(GTK_BIN(widget)->child)->label);
	DEBUG_MSG("open_recent_file_cb, started, filename is %s\n", GTK_LABEL(GTK_BIN(widget)->child)->label);
	doc_new_from_uri(bfwin, uri, NULL, FALSE, FALSE, -1, -1);
	g_object_unref(uri);
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

typedef struct {
	Tselectionsave *selsave;
	Tbfw_dynmenu *bdm;
} Tfilterdialog;

static void external_filter_dialog_response_lcb(GtkWidget *widget,gint response_id, gpointer data) {
	Tfilterdialog *fd= data;
	gchar **arr = (gchar **)fd->bdm->data;
	gint begin=0,end=-1;
	gtk_widget_destroy(widget);
	doc_restore_selection(fd->selsave, TRUE); /* the restore will also free the Tselectionsave */
	if (response_id == GTK_RESPONSE_YES) {
		doc_get_selection(fd->bdm->bfwin->current_document,&begin,&end);
	}
	filter_command(fd->bdm->bfwin, arr[1],begin,end);
	g_slice_free(Tfilterdialog,fd);
}

static void external_filter_lcb(GtkWidget *widget, Tbfw_dynmenu *bdm) {
	gchar **arr = (gchar **)bdm->data;
	gint begin=0,end=-1;
	/* if we have a selection, and the filter can be used on a selection,
	 we should ask if it should be the complete file or the selection */
	 
	if (operatable_on_selection(arr[1]) && (doc_has_selection(bdm->bfwin->current_document))) {
		GtkWidget *dialog,*vbox;
		Tfilterdialog *fd;
		fd = g_slice_new(Tfilterdialog);
		fd->bdm = bdm;
		
		fd->selsave = doc_save_selection(bdm->bfwin->current_document);
		dialog = gtk_message_dialog_new(GTK_WINDOW(bdm->bfwin->main_window),GTK_DIALOG_DESTROY_WITH_PARENT,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,_("Operate filter only on selection?"));
		g_signal_connect(dialog,"response",G_CALLBACK(external_filter_dialog_response_lcb),fd);
		gtk_widget_show_all(dialog);
	} else {
		DEBUG_MSG("external_filter_lcb, calling external_command for %s\n",arr[1]);
		filter_command(bdm->bfwin, arr[1],begin,end);
	}
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
			DEBUG_MSG("external_menu_rebuild,Adding filter %s with command %s to the menu\n", arr[0], arr[1]);
			bdm->menuitem = create_dynamic_menuitem(bfwin,N_("/External/Filters"),arr[0],G_CALLBACK(external_filter_lcb),bdm,-1);
			DEBUG_MSG("external_menu_rebuild,creating,bfwin=%p,bdm=%p,menuitem=%p\n",bfwin,bdm,bdm->menuitem);
			bfwin->menu_external = g_list_append(bfwin->menu_external, bdm);
		} else {
			DEBUG_MSG("external_menu_rebuild, CORRUPT ENTRY IN external_filter; array count =%d\n",count_array(arr));
		}
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
			DEBUG_MSG("external_menu_rebuild,Adding command %s with command %s (is default browser=%d) to the menu\n", arr[0], arr[1], (arr[2][0] == '1'));
			bdm->menuitem = create_dynamic_menuitem(bfwin,tmp1,arr[0],G_CALLBACK(external_command_lcb),bdm,-1);
			bfwin->menu_external = g_list_append(bfwin->menu_external, bdm);
		} else {
			DEBUG_MSG("external_menu_rebuild, CORRUPT ENTRY IN external_command; array count =%d\n",count_array(arr));
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
			DEBUG_MSG("external_menu_rebuild,Adding outputbox %s with command %s to the menu\n", arr[0], arr[5]);
			bdm->menuitem = create_dynamic_menuitem(bfwin,N_("/External/Outputbox"),arr[0],G_CALLBACK(menu_outputbox_lcb),(gpointer)bdm,-1);
			bfwin->menu_outputbox = g_list_append(bfwin->menu_outputbox,bdm);
		} else {
			DEBUG_MSG("external_menu_rebuild, CORRUPT ENTRY IN external_outputbox; array count =%d\n",count_array(arr));
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
		if (count_array(strarr)==3 && (g_ascii_strcasecmp (strarr[2], "TRUE") == 0)) {
			Tbfw_dynmenu *bdm = g_new(Tbfw_dynmenu,1);
			gchar *label = g_strdup_printf ("%s (%s)", strarr[0], strarr[1]);
			bdm->menuitem = gtk_radio_menu_item_new_with_label(group, label);
			g_free (label);
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

void menu_current_document_set_toggle_wo_activate(Tbfwin *bfwin, gpointer filetype, gchar *encoding) {
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


/*************************************************************************/
