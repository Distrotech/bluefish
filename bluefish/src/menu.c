/* Bluefish HTML Editor
 * menu.c - the menu(s)
 *
 * this file has content-type: UTF-8
 * and it is important you keep it UTF-8 !!!
 *
 * Copyright (C) 1998-2000 Olivier Sessink, Chris Mazuc and Roland Steinbach
 * Copyright (C) 2000-2002 Olivier Sessink and Roland Steinbach
 * Copyright (C) 2002-2010 Olivier Sessink
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
#include <gtk/gtk.h>
#include <stdlib.h>			/* atoi */
#include <string.h>			/* strchr() */
#include <gdk/gdkkeysyms.h>

/* #define DEBUG */

#include "bluefish.h"

#ifdef MAC_INTEGRATION
#include <ige-mac-integration.h>
#endif

#include "bf_lib.h"			/* append_string_to_file() */
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
#include "bftextview2.h"
#include "bftextview2_langmgr.h"

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
		doc_reload(bfwin->current_document, NULL, TRUE);
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
		strip_trailing_spaces(CURDOC(bfwin));
	break;
	case 30:
		convert_identing(CURDOC(bfwin), TRUE);
	break;
	case 31:
		convert_identing(CURDOC(bfwin), FALSE);
	break;
	case 32:
		sync_dialog(bfwin);
	break;
	case 33:
		join_lines(CURDOC(bfwin));
	break;
	case 34:
		split_lines(CURDOC(bfwin));
	break;
	case 35:
		toggle_comment(CURDOC(bfwin));
	break;
	case 36:
		convert_to_columns(CURDOC(bfwin));
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
		DEBUG_MSG_C("uh-oh: menu_file_operations_cb, unknown action %d\n",callback_action);
		g_return_if_reached();
	}
}

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

static GtkItemFactoryEntry menu_items[] = {
	{N_("/_File"), NULL, NULL, 0, "<Branch>"},
	{"/File/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/File/_New"), "<control>n", menu_file_operations_cb, 1, "<StockItem>", GTK_STOCK_NEW},
	{N_("/File/New From Template"), NULL, NULL, 0, "<Branch>"},
	{"/File/New From Template/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/File/New _Window"), "<shift><control>n", gui_window_menu_cb, 1, "<Item>"},
	{N_("/File/_Open..."), "<control>O", menu_file_operations_cb, 2, "<StockItem>", GTK_STOCK_OPEN},
	{N_("/File/Open _Recent"), NULL, NULL, 0, "<Branch>"},
	{"/File/Open Recent/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/File/Open Ad_vanced..."), "<shift><control>O", menu_file_operations_cb, 3, "<Item>"},
#ifndef WIN32
#ifndef MAC_INTEGRATION
	{N_("/File/Open _URL..."), NULL, menu_file_operations_cb, 25, "<Item>"},
#endif /*ifndef MAC_INTEGRATION*/
#endif /*ifndef WIN32*/
	{N_("/File/Open _From Selection"), NULL, menu_file_operations_cb, 26, "<Item>"},
	{N_("/File/_Insert..."), NULL, file_insert_menucb, 1, "<Item>"},
	{"/File/sep1", NULL, NULL, 0, "<Separator>"},
	{N_("/File/_Save"), "<control>S", menu_file_operations_cb, 5, "<StockItem>", GTK_STOCK_SAVE},
	{N_("/File/Save _As..."), "<shift><control>S", menu_file_operations_cb, 6, "<StockItem>", GTK_STOCK_SAVE_AS},
	{N_("/File/Rena_me..."), "F2", menu_file_operations_cb, 7, "<Item>"},
	{N_("/File/Sav_e All"), NULL, menu_file_operations_cb, 8, "<Item>"},
	{N_("/File/Rever_t to Saved"), NULL, menu_file_operations_cb, 4, "<StockItem>", GTK_STOCK_REVERT_TO_SAVED},
	{"/File/sep3", NULL, NULL, 0, "<Separator>"},
	{N_("/File/_Close"), "<control>w", menu_file_operations_cb, 9, "<StockItem>", GTK_STOCK_CLOSE},
	{N_("/File/Close A_ll"), "<shift><control>w", menu_file_operations_cb, 24, "<Item>"},
	{N_("/File/Close Win_dow"), NULL, gui_window_menu_cb, 2, "<Item>"},
	{"/File/sep2", NULL, NULL, 0, "<Separator>"},
	{N_("/File/Upload \\/ Download"), NULL, menu_file_operations_cb, 32, "<Item>"},
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
	{N_("/Edit/Select _All"), "<control>a", menu_file_operations_cb, 13, "<Item>"},
	{"/Edit/sep3", NULL, NULL, 0, "<Separator>"},
	{N_("/Edit/_Find..."), "<control>f", menu_file_operations_cb, 14, "<StockItem>", GTK_STOCK_FIND},
	{N_("/Edit/Find A_gain"), "<control>g", menu_file_operations_cb, 16, "<Item>"},
	{N_("/Edit/Find From Selection"), "<shift><control>f", menu_file_operations_cb, 27, "<Item>"},
	{N_("/Edit/R_eplace..."), "<control>h", menu_file_operations_cb, 17, "<StockItem>", GTK_STOCK_FIND_AND_REPLACE},
	{N_("/Edit/Replace Agai_n"), "<shift><control>h", menu_file_operations_cb, 19, "<Item>"},
	{"/Edit/sep4", NULL, NULL, 0, "<Separator>"},
	{N_("/Edit/_Indent"), NULL, menu_indent_cb, 2, "<StockItem>", GTK_STOCK_INDENT},
	{N_("/Edit/_Unindent"), NULL, menu_indent_cb, 1, "<StockItem>", GTK_STOCK_UNINDENT},
	{"/Edit/sep6", NULL, NULL, 0, "<Separator>"},
	{N_("/Edit/Save Shortcut _Keys"), NULL, rcfile_save_configfile_menu_cb, 0, "<Item>"},
	{N_("/Edit/Preference_s"), NULL, open_preferences_menu_cb, 0, "<StockItem>", GTK_STOCK_PREFERENCES},
	{N_("/_View"), NULL, NULL, 0, "<Branch>"},
	{"/View/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/View/_Full Screen"), "F11", gui_fullscreen_cb, 1, "<ToggleItem>"},
	{"/View/sep1", NULL, NULL, 0, "<Separator>"},
	{N_("/View/_Main Toolbar"), NULL, gui_toggle_hidewidget_cb, 1, "<ToggleItem>"},
	{N_("/View/_Side Pane"), "F9", gui_toggle_hidewidget_cb, 4, "<ToggleItem>"},
	{N_("/View/_Output Pane"), "<control>F9", gui_toggle_hidewidget_cb, 6, "<ToggleItem>"},
	{N_("/View/Status_bar"), NULL, gui_toggle_hidewidget_cb, 5, "<ToggleItem>"},
	{N_("/_Document"), NULL, NULL, 0, "<Branch>"},
	{"/Document/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/Document/_Auto Indent"), NULL, doc_menu_lcb, 4, "<ToggleItem>"},
	{N_("/Document/Auto _Completion Popup"), NULL, doc_menu_lcb, 3, "<ToggleItem>"},
	{N_("/Document/_Wrap"), NULL, doc_menu_lcb, 1, "<ToggleItem>"},
	{N_("/Document/_Line Numbers"), NULL, doc_menu_lcb, 2, "<ToggleItem>"},
	{N_("/Document/Show _Blocks"), NULL, doc_menu_lcb, 5, "<ToggleItem>"},
	{N_("/Document/_Visible Spacing"), NULL, doc_menu_lcb, 6, "<ToggleItem>"},
	{N_("/Document/Show Right Margin"), NULL, doc_menu_lcb, 14, "<ToggleItem>"},
	{N_("/Document/Highlight block delimiters"), NULL, doc_menu_lcb, 15, "<ToggleItem>"},
#ifdef HAVE_LIBENCHANT
	{N_("/Document/_Spell Check"), NULL, doc_menu_lcb, 13, "<ToggleItem>"},
#endif
	{"/Document/sep1", NULL, NULL, 0, "<Separator>"},
	{N_("/Document/_Tab Size"), NULL, NULL, 0, "<Branch>"},
	{"/Document/Tab Size/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/Document/Tab Size/_Increase"), NULL, doc_menu_lcb, 10, "<Item>"},
	{N_("/Document/Tab Size/_Decrease"), NULL, doc_menu_lcb, 11, "<Item>"},
	{"/Document/Tab Size/sep", NULL, NULL, 0, "<Separator>"},
	{N_("/Document/Tab Size/_Reset"), NULL, doc_menu_lcb, 12, "<Item>"},
	{N_("/Document/_Font Size"), NULL, NULL, 0, "<Branch>"},
	{"/Document/Font Size/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/Document/Font Size/_Increase"), NULL, doc_menu_lcb, 7, "<Item>"},
	{N_("/Document/Font Size/_Decrease"), NULL, doc_menu_lcb, 8, "<Item>"},
	{"/Document/Font Size/sep", NULL, NULL, 0, "<Separator>"},
	{N_("/Document/Font Size/_Reset"), NULL, doc_menu_lcb, 9, "<Item>"},
	{"/Document/sep2", NULL, NULL, 0, "<Separator>"},
	{N_("/Document/_Highlight Syntax"), NULL, doc_toggle_highlighting_cb, 1, "<ToggleItem>"},
#ifdef HAVE_LIBENCHANT
	{N_("/Document/_Rescan Syntax & Spelling"), "F5", doc_update_highlighting, 0, "<Item>"},
#else
	{N_("/Document/_Rescan Syntax"), "F5", doc_update_highlighting, 0, "<Item>"},
#endif
	{N_("/Document/Language _Mode"), NULL, NULL, 0, "<Branch>"},
	{"/Document/Language Mode/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{"/Document/sep3", NULL, NULL, 0, "<Separator>"},
	{N_("/Document/Character _Encoding"), NULL, NULL, 0, "<Branch>"},
	{"/Document/Character Encoding/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{"/Document/Character Encoding/sep4", NULL, NULL, 0, "<Separator>"},
	{N_("/Document/Character Encoding/_Add or Remove..."), NULL, encodings_dialog_menu_cb, 1, "<Item>"},
	{"/Document/sep5", NULL, NULL, 0, "<Separator>"},
	{N_("/Document/Add Boo_kmark"), "<control>k", menu_bmark_operations_cb, 1, "<Item>"},
	{N_("/Document/Floati_ng Window"), NULL, file_floatingview_menu_cb, 1, "<Item>"},
	{"/Document/sep6", NULL, NULL, 0, "<Separator>"},
	{N_("/Document/Move Left"), NULL, gui_notebook_switch, 5, "<Item>"},
	{N_("/Document/Move Right"), NULL, gui_notebook_switch, 6, "<Item>"},
	{N_("/_Go"), NULL, NULL, 0, "<Branch>"},
	{"/Go/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/Go/_Previous Document"), NULL, gui_notebook_switch, 1, "<StockItem>", GTK_STOCK_GO_BACK},
	{N_("/Go/_Next Document"), NULL, gui_notebook_switch, 2, "<StockItem>", GTK_STOCK_GO_FORWARD},
	{N_("/Go/_First Document"), NULL, gui_notebook_switch, 3, "<StockItem>", GTK_STOCK_GOTO_FIRST},
	{N_("/Go/L_ast Document"), NULL, gui_notebook_switch, 4, "<StockItem>", GTK_STOCK_GOTO_LAST},
	{"/Go/sep2", NULL, NULL, 0, "<Separator>"},
	{N_("/Go/F_irst Bookmark"), NULL, bookmark_menu_cb, 1, "<StockItem>", GTK_STOCK_GOTO_TOP},
	{N_("/Go/P_revious Bookmark"), NULL, bookmark_menu_cb, 2, "<StockItem>", GTK_STOCK_GO_UP},
	{N_("/Go/N_ext Bookmark"), NULL, bookmark_menu_cb, 3, "<StockItem>", GTK_STOCK_GO_DOWN},
	{N_("/Go/Last _Bookmark"), NULL, bookmark_menu_cb, 4, "<StockItem>", GTK_STOCK_GOTO_BOTTOM},
	{"/Go/sep3", NULL, NULL, 0, "<Separator>"},
	{N_("/Go/Goto _Line"), "<control>l", gui_gotoline_frame_show, 1, "<StockItem>", GTK_STOCK_JUMP_TO},
	{N_("/Go/Goto line number in _selection"), NULL, go_to_line_from_selection_cb, 1, "<Item>"},
	{N_("/Go/Jump to reference"), "<control>j", doc_menu_lcb, 16, "<Item>"},
	{N_("/_Project"), NULL, NULL, 0, "<Branch>"},
	{"/Project/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/Project/_New Project"), NULL, project_menu_cb, 6, "<Item>"},
	{N_("/Project/_Open Project..."), NULL, project_menu_cb, 1, "<Item>"},
	{N_("/Project/Open _Recent"), NULL, NULL, 0, "<Branch>"},
	{"/Project/Open Recent/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{"/Project/sep1", NULL, NULL, 0, "<Separator>"},
	{N_("/Project/_Save"), NULL, project_menu_cb, 2, "<Item>"},
	{N_("/Project/Save _as..."), NULL, project_menu_cb, 3, "<Item>"},
	{N_("/Project/Save & _close"), NULL, project_menu_cb, 4, "<Item>"},
	{"/Project/sep2", NULL, NULL, 0, "<Separator>"},
	{N_("/Project/E_dit Project Options..."), NULL, project_menu_cb, 5, "<StockItem>", GTK_STOCK_PREFERENCES},
	{N_("/_Tools"), NULL, NULL, 0, "<Branch>"},
	{"/Tools/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{"/Tools/sep1", NULL, NULL, 0, "<Separator>"},
	{N_("/Tools/_Commands"), NULL, NULL, 0, "<Branch>"},
	{"/Tools/Commands/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/Tools/_Outputbox"), NULL, NULL, 0, "<Branch>"},
	{"/Tools/Outputbox/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{N_("/Tools/_Filters"), NULL, NULL, 0, "<Branch>"},
	{"/Tools/Filters/tearoff1", NULL, NULL, 0, "<Tearoff>"},
	{"/Tools/sep4", NULL, NULL, 0, "<Separator>"},
	{N_("/Tools/_Synchronize text block"), NULL, menu_file_operations_cb, 28, "<Item>"},
	{N_("/Tools/Toggle _Comment"), "<shift><control>c", menu_file_operations_cb, 35, "<Item>"},
	{N_("/Tools/_Word Count"), NULL, word_count_cb, 1, "<Item>"},
	{"/Tools/sep2", NULL, NULL, 0, "<Separator>"},
	{N_("/Tools/Strip T_railing Whitespace"), NULL, menu_file_operations_cb, 29, "<Item>"},
	{N_("/Tools/_Join lines"), NULL, menu_file_operations_cb, 33, "<Item>"},
	{N_("/Tools/Sp_lit lines"), NULL, menu_file_operations_cb, 34, "<Item>"},
	{N_("/Tools/Indenting To T_abs"), NULL, menu_file_operations_cb, 30, "<Item>"},
	{N_("/Tools/Indenting To S_paces"), NULL, menu_file_operations_cb, 31, "<Item>"},
	{N_("/Tools/Merge Lines Into Col_umns"), NULL, menu_file_operations_cb, 36, "<Item>"},
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
static GtkWidget *create_dynamic_menuitem(Tbfwin *bfwin, const gchar *menubasepath, const gchar *label, GCallback callback, gpointer data, gint menu_insert_offset) {
	GtkWidget *tmp, *menu;
	GtkItemFactory *factory;

	/* add it to bfwin->menubar */
	factory = gtk_item_factory_from_widget(bfwin->menubar);
	menu = gtk_item_factory_get_widget(factory, menubasepath);
	DEBUG_MSG("create_dynamic_menuitem, menubar=%p, menu=%p, basepath=%s, label=%s\n", bfwin->menubar, menu, menubasepath, label);
	if (!menu)
		return NULL;
	if (label) {
		tmp = gtk_menu_item_new_with_label(label);
		g_signal_connect(G_OBJECT(tmp), "activate",callback, data);
	} else { /* separator */
		tmp = gtk_menu_item_new();
	}
	gtk_widget_show(tmp);
	if (menu_insert_offset == -1) {
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),tmp);
	} else {
		gtk_menu_shell_insert(GTK_MENU_SHELL(menu),tmp,menu_insert_offset);
	}
	return tmp;
}
/*
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
}	*/

static void menu_current_document_type_change(GtkMenuItem *menuitem,Tbfw_dynmenu *bdm) {
	DEBUG_MSG("menu_current_document_type_change, started for bflang %p\n", bdm->data);
	if (GTK_CHECK_MENU_ITEM(menuitem)->active) {
		doc_set_mimetype(bdm->bfwin->current_document, ((Tbflang *)bdm->data)->mimetypes->data);
	}
	DEBUG_MSG("menu_current_document_type_change, finished\n");
}

void free_bfw_dynmenu_list(GList *list) {
	GList *tmplist = g_list_first(list);
	while (tmplist) {
		Tbfw_dynmenu *bdm = BFW_DYNMENU(tmplist->data);
		if (bdm->signal_id!=0)
			g_signal_handler_disconnect(bdm->menuitem,bdm->signal_id);
		gtk_widget_destroy(bdm->menuitem);
		g_slice_free(Tbfw_dynmenu,bdm);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(list);
}

static void menu_template_lcb(GtkMenuItem *menuitem,Tbfw_dynmenu *bdm) {
	gchar **arr = bdm->data;
	GFile *uri = g_file_new_for_commandline_arg(arr[1]);
	doc_new_with_template(bdm->bfwin, uri, FALSE);
	g_object_unref(uri);
}

void template_menu_rebuild(Tbfwin *bfwin,GtkItemFactory *item_factory) {
	GtkWidget *parent_menu;
	GList *tmplist;
	if (!item_factory) {
		item_factory = gtk_item_factory_from_widget(bfwin->menubar);
	}
	free_bfw_dynmenu_list(bfwin->menu_templates);
	DEBUG_MSG("template_menu_rebuild, adding filetypes in menu\n");
	bfwin->menu_templates = NULL;
	parent_menu = gtk_item_factory_get_widget(item_factory, N_("/File/New From Template"));
	tmplist = g_list_last(main_v->props.templates);
	while (tmplist) {
		gchar **arr = (gchar **)tmplist->data;
		if (arr && arr[0] && arr[1]) {
			Tbfw_dynmenu *bdm = g_slice_new(Tbfw_dynmenu);
			bdm->data = arr;
			bdm->bfwin = bfwin;
			bdm->menuitem = gtk_menu_item_new_with_label(arr[0]);
			bdm->signal_id = g_signal_connect(G_OBJECT(bdm->menuitem), "activate",G_CALLBACK(menu_template_lcb), (gpointer) bdm);
			gtk_widget_show(bdm->menuitem);
			gtk_menu_insert(GTK_MENU(parent_menu), bdm->menuitem, 1);
			bfwin->menu_templates = g_list_prepend(bfwin->menu_templates, bdm);
		}
		tmplist = g_list_previous(tmplist);
	}
/*#ifdef MAC_INTEGRATION
	ige_mac_menu_sync(GTK_MENU_SHELL(BFWIN(bfwin)->menubar));
#endif*/
}

void filetype_menu_rebuild(Tbfwin *bfwin,GtkItemFactory *item_factory) {
	GSList *group=NULL;
	GtkWidget *parent_menu;
	GList *tmplist, *list;
	if (!item_factory) {
		item_factory = gtk_item_factory_from_widget(bfwin->menubar);
	}
	DEBUG_MSG("filetype_menu_rebuild, adding filetypes in menu\n");
	bfwin->menu_filetypes = NULL;
	parent_menu = gtk_item_factory_get_widget(item_factory, N_("/Document/Language Mode"));
	list = tmplist = g_list_last(langmgr_get_languages());
	while (tmplist) {
		Tbflang *bflang = (Tbflang *)tmplist->data;
		Tbfw_dynmenu *bdm = g_slice_new(Tbfw_dynmenu);
		bdm->data = bflang;
		bdm->bfwin = bfwin;
		bdm->menuitem = gtk_radio_menu_item_new_with_label(group, bflang->name);
		bdm->signal_id = g_signal_connect(G_OBJECT(bdm->menuitem), "activate",G_CALLBACK(menu_current_document_type_change), (gpointer) bdm);
		gtk_widget_show(bdm->menuitem);
		gtk_menu_insert(GTK_MENU(parent_menu), bdm->menuitem, 1);
		group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(bdm->menuitem));
		bfwin->menu_filetypes = g_list_prepend(bfwin->menu_filetypes, bdm);
		tmplist = g_list_previous(tmplist);
	}
	g_list_free(list);
/*#ifdef MAC_INTEGRATION
	ige_mac_menu_sync(GTK_MENU_SHELL(BFWIN(bfwin)->menubar));
#endif*/

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
	gtk_accel_map_add_entry("<bluefishmain>/Edit/Indent", GDK_period, GDK_CONTROL_MASK);
	gtk_accel_map_add_entry("<bluefishmain>/Edit/Unindent", GDK_comma, GDK_CONTROL_MASK);
	gtk_accel_map_add_entry("<bluefishmain>/Go/Previous Document", GDK_Page_Up, GDK_CONTROL_MASK);
	gtk_accel_map_add_entry("<bluefishmain>/Go/Next Document", GDK_Page_Down, GDK_CONTROL_MASK);
	gtk_accel_map_add_entry("<bluefishmain>/Go/First Document", GDK_Page_Up, GDK_SHIFT_MASK | GDK_CONTROL_MASK);
	gtk_accel_map_add_entry("<bluefishmain>/Go/Last Document", GDK_Page_Down, GDK_SHIFT_MASK | GDK_CONTROL_MASK);
	gtk_widget_show(bfwin->menubar);
	setup_toggle_item(item_factory, "/View/Main Toolbar", bfwin->session->view_main_toolbar);
	setup_toggle_item(item_factory, "/View/Side Pane", bfwin->session->view_left_panel);
	setup_toggle_item(item_factory, "/View/Statusbar", bfwin->session->view_statusbar);
	setup_toggle_item(item_factory, "/Document/Auto Indent", bfwin->session->autoindent);
	set_project_menu_widgets(bfwin, FALSE);
	filetype_menu_rebuild(bfwin, item_factory);
	template_menu_rebuild(bfwin, item_factory);
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
	outputbox(bdm->bfwin,arr[1], atoi(arr[2]), atoi(arr[3]), atoi(arr[4]), arr[5]);
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
	GFile *uri = g_file_new_for_uri(GTK_LABEL(GTK_BIN(widget)->child)->label);
	if (uri) {
		project_open_from_file(bfwin, uri);
		add_to_recent_list(bfwin,uri, 0, TRUE);
		g_object_unref(uri);
	}
}

/* open_recent_file
 * This function should be called when a menu from the Open Recent list
 * has been selected. */
static void open_recent_file_cb(GtkWidget *widget, Tbfwin *bfwin) {
	GFile *uri = g_file_new_for_uri(GTK_LABEL(GTK_BIN(widget)->child)->label);
	if (uri) {
		doc_new_from_uri(bfwin, uri, NULL, FALSE, FALSE, -1, -1);
		add_to_recent_list(bfwin,uri, 0, FALSE);
		g_object_unref(uri);
	}
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
		return  create_dynamic_menuitem(bfwin,"/Project/Open Recent"
			, filename, G_CALLBACK(open_recent_project_cb), bfwin
			, 1);
	} else {
		return  create_dynamic_menuitem(bfwin,"/File/Open Recent"
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
/*#ifdef MAC_INTEGRATION
	ige_mac_menu_sync(GTK_MENU_SHELL(BFWIN(bfwin)->menubar));
#endif*/
	return retlist;
}

/* recent_menu_init()
 * Gets the list of documents from .bluefish/recentlist and inserts
 * it at the File-->Open Recent menu. If the file doesn't exist (probably
 * because this is the first time Bluefish is running) then a menu
 * item telling that no recent files exist will appear */
void recent_menu_init(Tbfwin *bfwin) {
/*	recent_menu_from_file(bfwin, "/."PACKAGE"/recentlist", FALSE);
	recent_menu_from_file(bfwin, "/."PACKAGE"/recentprojects", TRUE);*/
	bfwin->menu_recent_files = recent_menu_from_list(bfwin, bfwin->session->recent_files, FALSE);
	bfwin->menu_recent_projects = recent_menu_from_list(bfwin, main_v->globses.recent_projects, TRUE);
}

/* when a project is opened, the recent menu should show the recent files
from that project */
void recent_menu_init_project(Tbfwin *bfwin) {
	/*gint num;*/
	GList *tmplist = g_list_first(bfwin->menu_recent_files);
	while (tmplist) {
		gtk_widget_destroy(tmplist->data);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(bfwin->menu_recent_files);
	bfwin->menu_recent_files = recent_menu_from_list(bfwin, bfwin->session->recent_files, FALSE);
}

static void register_recent(gchar *curi, gboolean is_project) {
	if (main_v->props.register_recent_mode==0)
		return;
	if(is_project) {
		gtk_recent_manager_add_item(main_v->recentm, curi);
		return;
	}
	if (main_v->props.register_recent_mode==1) {
		gtk_recent_manager_add_item(main_v->recentm, curi);
	}
}
/* Add_to_recent_list
 * This should be called when a new file is opened, i.e. from
 * file_open_cb, it adds a new entry which also appears in the
 * menu bar, and (if nessecary) deletes the last entry */
void add_to_recent_list(Tbfwin *bfwin, GFile *file, gint closed_file, gboolean is_project) {
	gchar *filename = g_file_get_uri(file);
	register_recent(filename,is_project);
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
	g_free(filename);
/*#ifdef MAC_INTEGRATION
	ige_mac_menu_sync(GTK_MENU_SHELL(BFWIN(bfwin)->menubar));
#endif*/
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
	if (response_id == 1) {
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
		GtkWidget *dialog, *but;
		Tfilterdialog *fd;
		fd = g_slice_new(Tfilterdialog);
		fd->bdm = bdm;

		fd->selsave = doc_save_selection(bdm->bfwin->current_document);
		/* TODO: this dialog is not very convenient, hitting enter should choose 'selection' */
		dialog = gtk_message_dialog_new(GTK_WINDOW(bdm->bfwin->main_window),GTK_DIALOG_DESTROY_WITH_PARENT,GTK_MESSAGE_QUESTION,GTK_BUTTONS_NONE,_("Text area to filter"));
		gtk_dialog_add_button(GTK_DIALOG(dialog), _("All text"), 0);
		but = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Selected text only"), 1);
		gtk_widget_grab_default(but);
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
	free_bfw_dynmenu_list(bfwin->menu_external);
	bfwin->menu_external = NULL;

	free_bfw_dynmenu_list(bfwin->menu_outputbox);
	bfwin->menu_outputbox = NULL;

	/*create_parent_and_tearoff(N_("/Tools/Outputbox/"), gtk_item_factory_from_widget(bfwin->menubar));
	create_parent_and_tearoff(N_("/Tools/Commands/"), gtk_item_factory_from_widget(bfwin->menubar));
	create_parent_and_tearoff(N_("/Tools/Filters/"), gtk_item_factory_from_widget(bfwin->menubar));*/

	tmplist = g_list_first(main_v->props.external_filter);
	while (tmplist) {
		gchar **arr = tmplist->data;
		/*  arr[0] = name
		 *  arr[1] = command
		 */
		if (count_array(arr)==2) {
			Tbfw_dynmenu *bdm = g_slice_new(Tbfw_dynmenu);
			bdm->bfwin = bfwin;
			bdm->data = arr;
			bdm->signal_id=0;
			DEBUG_MSG("external_menu_rebuild,Adding filter %s with command %s to the menu\n", arr[0], arr[1]);
			bdm->menuitem = create_dynamic_menuitem(bfwin,"/Tools/Filters",arr[0],G_CALLBACK(external_filter_lcb),bdm,-1);
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
			Tbfw_dynmenu *bdm = g_slice_new(Tbfw_dynmenu);
			bdm->bfwin = bfwin;
			bdm->data = arr;
			bdm->signal_id=0;
			if (arr[2][0] == '1') {
				bdm->menuitem = create_dynamic_menuitem(bfwin,"/Tools",arr[0],G_CALLBACK(external_command_lcb),bdm,1);
			} else {
				bdm->menuitem = create_dynamic_menuitem(bfwin,"/Tools/Commands",arr[0],G_CALLBACK(external_command_lcb),bdm,-1);
			}
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
		if (count_array(arr)==6) {
			Tbfw_dynmenu *bdm = g_slice_new(Tbfw_dynmenu);
			bdm->data = arr;
			bdm->bfwin = bfwin;
			bdm->signal_id=0;
			DEBUG_MSG("external_menu_rebuild,Adding outputbox %s with command %s to the menu\n", arr[0], arr[5]);
			bdm->menuitem = create_dynamic_menuitem(bfwin,"/Tools/Outputbox",arr[0],G_CALLBACK(menu_outputbox_lcb),(gpointer)bdm,-1);
			bfwin->menu_outputbox = g_list_append(bfwin->menu_outputbox,bdm);
		} else {
			DEBUG_MSG("external_menu_rebuild, CORRUPT ENTRY IN external_outputbox; array count =%d\n",count_array(arr));
		}
		tmplist = g_list_next(tmplist);
	}
/*#ifdef MAC_INTEGRATION
	ige_mac_menu_sync(GTK_MENU_SHELL(BFWIN(bfwin)->menubar));
#endif*/
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
	
	free_bfw_dynmenu_list(bfwin->menu_encodings);
	bfwin->menu_encodings = NULL;

	tmplist = g_list_last(main_v->globses.encodings);
	parent_menu = gtk_item_factory_get_widget(gtk_item_factory_from_widget(bfwin->menubar), N_("/Document/Character Encoding"));
	while (tmplist) {
		gchar **strarr = (gchar **)tmplist->data;
		if (count_array(strarr)==3 && strarr[2][0]=='1') {
			Tbfw_dynmenu *bdm = g_slice_new(Tbfw_dynmenu);
			gchar *label = g_strdup_printf ("%s (%s)", strarr[0], strarr[1]);
			/*g_print("add encoding %s:%s:%s to the menu\n",strarr[0],strarr[1],strarr[2]);*/
			bdm->menuitem = gtk_radio_menu_item_new_with_label(group, label);
			g_free (label);
			bdm->data = strarr[1];
			bdm->bfwin = bfwin;
			bdm->signal_id=0;
			g_signal_connect(G_OBJECT(bdm->menuitem), "activate",G_CALLBACK(menu_current_document_encoding_change), (gpointer) bdm);
			gtk_widget_show(bdm->menuitem);
			gtk_menu_insert(GTK_MENU(parent_menu), bdm->menuitem, 1);
			group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(bdm->menuitem));
			bfwin->menu_encodings = g_list_append(bfwin->menu_encodings, bdm);
		}
		tmplist = g_list_previous(tmplist);
	}
/*#ifdef MAC_INTEGRATION
	ige_mac_menu_sync(GTK_MENU_SHELL(BFWIN(bfwin)->menubar));
#endif*/
}

void menu_current_document_set_toggle_wo_activate(Tbfwin *bfwin, gpointer filetype, gchar *encoding) {
	Tbfw_dynmenu *bdm = find_bfw_dynmenu_by_data_in_list(bfwin->menu_filetypes, filetype);
	if (bdm && filetype && bdm->menuitem && !GTK_CHECK_MENU_ITEM(bdm->menuitem)->active) {
		g_signal_handler_disconnect(G_OBJECT(bdm->menuitem),bdm->signal_id);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(bdm->menuitem), TRUE);
		bdm->signal_id = g_signal_connect(G_OBJECT(bdm->menuitem), "activate",G_CALLBACK(menu_current_document_type_change), (gpointer) bdm);
	}

	if (encoding) {
		GList *tmplist;
		tmplist = g_list_first(main_v->globses.encodings);
		while (tmplist) {
			gchar **tmparr = (gchar **)tmplist->data;
			if (g_ascii_strcasecmp(tmparr[1], encoding)==0) {
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
