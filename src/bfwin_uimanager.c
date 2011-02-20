/*
 * Bluefish HTML Editor
 * bfwin_uimanager.c
 *
 * Copyright (C) 2011 James Hayward and Olivier Sessink
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


#include "bfwin_uimanager.h"


/* For testing purposes */
#define MAIN_MENU_UI "ui/bluefish_menu_ui.xml"


static const GtkActionEntry top_level_menus[] = {
  { "FileMenu", NULL, N_("_File") },
  { "NewFromTemplate", NULL, N_("New From Template") },
  { "FileOpenRecent", NULL, N_("Open _Recent") },
  { "EditMenu", NULL, N_("_Edit") },
  { "ViewMenu", NULL, N_("_View") },
  { "DocumentMenu", NULL, N_("_Document") },
  { "DocumentTabSize", NULL, N_("_Tab Size") },
  { "DocumentFontSize", NULL, N_("_Font Size") },
  { "DocumentLangMode", NULL, N_("Language _Mode") },
  { "DocumentEncoding", NULL, N_("Character _Encoding") },
  { "GoMenu", NULL, N_("_Go") },
  { "ProjectMenu", NULL, N_("_Project") },
  { "ProjectOpenRecent", NULL, N_("Open _Recent") },
  { "ToolsMenu", NULL, N_("_Tools") },
  { "ToolsCommands", NULL, N_("_Commands") },
  { "ToolsFilters", NULL, N_("_Filters") },
  { "ToolsOutputBox", NULL, N_("_Outputbox") }
};

static const GtkActionEntry global_actions[] = {
	{ "FileNew", GTK_STOCK_NEW, N_("_New"), "<control>N", N_("New file"), NULL },
	{ "FileNewWindow", NULL, N_("New _Window"), "<shift><control>N", N_("New window"), NULL },
	{ "FileOpen", GTK_STOCK_OPEN, N_("_Open..."), "<control>O", N_("Open file"), NULL },
	{ "FileOpenAdvanced", NULL, N_("Open Ad_vanced..."), "<shift><control>O", N_("Open advanced"), NULL },
#ifndef WIN32
#ifndef MAC_INTEGRATION
	{ "FileOpenUrl", NULL, N_("Open _URL..."), NULL, N_("Open URL"), NULL },
#endif /* MAC_INTEGRATION */
#endif /* WIN32 */
	{ "FileCloseWindow", NULL, N_("Close Win_dow"), NULL, N_("Close window"), NULL },
	{ "FileInsert", NULL, N_("_Insert..."), NULL, N_("Insert file"), NULL },
	{ "FileQuit", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q", N_("Quit Bluefish"), NULL },
	{ "EditIndent", GTK_STOCK_INDENT, N_("_Indent"), NULL, N_("Indent"), NULL },
	{ "EditUnindent", GTK_STOCK_UNINDENT, N_("_Unindent"), NULL, N_("Unindent"), NULL },
	{ "EditPreferences", GTK_STOCK_PREFERENCES, N_("Preference_s..."), NULL, N_("Edit Preferences"), NULL },
	{ "EditSelectAll", NULL, N_("Select _All"), "<control>A", N_("Select all"), NULL },
	{ "DocAddBookmark", NULL, N_("Add Boo_kmark"), "<control>k", N_("Add bookmark"), NULL },
	{ "DocEncodingAddRemove", NULL, N_("_Add or Remove..."), NULL, N_("Add or remove character encoding"), NULL },
	{ "DocFloatingWindow", NULL, N_("Floati_ng Window"), NULL, N_("Open current document in floating window"), NULL },
	{ "DocFontSizeIncrease", NULL, N_("_Increase"), NULL, N_("Increase document font size"), NULL },
	{ "DocFontSizeDecrease", NULL, N_("_Decrease"), NULL, N_("Decrease document font size"), NULL },
	{ "DocFontSizeReset", NULL, N_("_Reset"), NULL, N_("Reset document font size"), NULL },
	{ "DocTabSizeIncrease", NULL, N_("_Increase"), NULL, N_("Increase document tab size"), NULL },
	{ "DocTabSizeDecrease", NULL, N_("_Decrease"), NULL, N_("Decrease document tab size"), NULL },
	{ "DocTabSizeReset", NULL, N_("_Reset"), NULL, N_("Reset document tab size"), NULL },
#ifdef HAVE_LIBENCHANT
	{ "DocumentRescan", NULL, N_("_Rescan Syntax & Spelling"), "F5", N_("Rescan document highlighting"), NULL },
#else
	{ "DocumentRescan", NULL, N_("_Rescan Syntax"), "F5", N_("Rescan document highlighting"), NULL },
#endif
	{ "ProjectNew", NULL, N_("_New Project..."), NULL, N_("New project"), NULL },
	{ "ProjectOpen", NULL, N_("_Open Project..."), NULL, N_("Open project"), NULL },
	{ "SynchTextBlock", NULL, N_("_Synchronize Text Block"), NULL, N_("Synchronize text block"), NULL },
	{ "ToggleComment", NULL, N_("Toggle _Comment"), "<shift><control>C", N_("Toggle comment"), NULL },
	{ "WordCount", NULL, N_("_Word Count"), NULL, N_("Word count"), NULL },
	{ "IndentingToSpaces", NULL, N_("Indenting To S_paces"), NULL, N_("Indenting to spaces"), NULL },
	{ "IndentingToTabs", NULL, N_("Indenting To T_abs"), NULL, N_("Indenting to tabs"), NULL },
	{ "JoinLines", NULL, N_("_Join Lines Together"), NULL, N_("Join lines together"), NULL },
	{ "SplitLines", NULL, N_("Sp_lit Lines on Right Margin"), NULL, N_("Split lines on right margin"), NULL },
	{ "MergeLinesIntoColumns", NULL, N_("Merge Lines Into Col_umns"), NULL, N_("Merge lines into columns"), NULL },
	{ "RewrapLines", NULL, N_("Rewrap _Lines"), NULL, N_("Rewrap lines"), NULL },
	{ "StripTrailingWhitespace", NULL, N_("Strip T_railing Whitespace"), NULL, N_("Strip trailing whitespace"), NULL }
};

static const GtkToggleActionEntry global_toggle_actions[] = {
	{ "ViewFullScreen", GTK_STOCK_FULLSCREEN, N_("_Full Screen"), "F11", N_("Full screen"), NULL, FALSE },
	{ "ViewMainToolbar", NULL, N_("_Main Toolbar"), NULL, N_("Show main toolbar"), NULL, TRUE },
	{ "ViewSidePane", NULL, N_("_Side Pane"), "F9", N_("Show side pane"), NULL, TRUE },
	{ "ViewOutputPane", NULL, N_("_Output Pane"), "<control>F9", N_("Show output pane"), NULL, FALSE },
	{ "ViewStatusbar", NULL, N_("Status_bar"), NULL, N_("Show statusbar"), NULL, TRUE },
	{ "AutoCompletion", NULL, N_("Auto _Completion Popup"), NULL, N_("Show auto completion popup"), NULL, TRUE },
	{ "AutoIndent", NULL, N_("_Auto Indent"), NULL, N_("Auto indent"), NULL, TRUE },
	{ "HighlightBlockDelimiters", NULL, N_("Highlight Block Delimiters"), NULL, N_("Highlight block delimiters"), NULL, TRUE },
	{ "HighlightSyntax", NULL, N_("_Highlight Syntax"), NULL, N_("Highlight syntax"), NULL, TRUE },
	{ "ShowBlocks", NULL, N_("Show _Blocks"), NULL, N_("Show blocks"), NULL, TRUE },
	{ "ShowLineNumbers", NULL, N_("Show _Line Numbers"), NULL, N_("Show line numbers"), NULL, TRUE },
	{ "ShowRightMargin", NULL, N_("Show Right Margin"), NULL, N_("Show right margin"), NULL, FALSE },
	{ "ShowSplitView", NULL, N_("Show Split View"), NULL, N_("Show split view"), NULL, FALSE },
	{ "ShowVisibleSpacing", NULL, N_("Show _Visible Spacing"), NULL, N_("Show visible spacing"), NULL, FALSE },
#ifdef HAVE_LIBENCHANT
	{ "SpellCheck", NULL, N_("_Spell Check"), NULL, N_("Spell check"), NULL, TRUE },
#endif
	{ "WrapText", NULL, N_("_Wrap Text"), NULL, N_("Wrap text"), NULL, FALSE }
};

static const GtkActionEntry document_actions[] = {
	{ "FileClose", GTK_STOCK_CLOSE, N_("_Close"), "<control>W", N_("Close current file"), NULL },
	{ "FileCloseAll", NULL, N_("Close A_ll"), "<shift><control>W", N_("Close all files"), NULL },
	{ "FileSave", GTK_STOCK_SAVE, N_("_Save"), "<control>S", N_("Save current file"), NULL },
	{ "FileSaveAs", GTK_STOCK_SAVE_AS, N_("Save _As..."), "<shift><control>S", N_("Save file as"), NULL },
	{ "FileSaveAll", NULL, N_("Sav_e All"), NULL, N_("Save all files"), NULL },
	{ "FileRevert", GTK_STOCK_REVERT_TO_SAVED, N_("Rever_t to Saved"), NULL, N_("Reload current file"), NULL },
	{ "FileRename", NULL, N_("Rena_me..."), "F2", N_("Rename file"), NULL },
	{ "BookmarkFirst", GTK_STOCK_GOTO_TOP, N_("F_irst Bookmark"), NULL, N_("Goto first bookmark"), NULL },
	{ "BookmarkLast", GTK_STOCK_GOTO_BOTTOM, N_("_Last Bookmark"), NULL, N_("Goto last bookmark"), NULL },
	{ "BookmarkNext", GTK_STOCK_GO_DOWN, N_("N_ext Bookmark"), "<shift><control>K", N_("Goto next bookmark"), NULL },
	{ "BookmarkPrevious", GTK_STOCK_GO_UP, N_("P_revious Bookmark"), "<shift><control>J", N_("Goto previous document"), NULL },
	{ "DocMoveLeft", NULL, N_("Move Tab Left"), NULL, N_("Move current tab left"), NULL },
	{ "DocMoveRight", NULL, N_("Move Tab Right"), NULL, N_("Move current tab right"), NULL },
	{ "DocFirst", GTK_STOCK_GOTO_FIRST, N_("_First Document"), NULL, N_("Goto first document"), NULL },
	{ "DocLast", GTK_STOCK_GOTO_LAST, N_("L_ast Document"), NULL, N_("Goto last document"), NULL },
	{ "DocNext", GTK_STOCK_GO_FORWARD, N_("_Next Document"), NULL, N_("Goto next document"), NULL },
	{ "DocPrevious", GTK_STOCK_GO_BACK, N_("_Previous Document"), NULL, N_("Goto previous document"), NULL },
	{ "GotoLine", NULL, N_("Goto _Line"), "<control>L", N_("Goto line"), NULL },
	{ "GotoLineSelection", NULL, N_("Goto Line Number in _Selection"), "<shift><control>L", N_("Goto line number in selection"), NULL },
	{ "JumpToReference", NULL, N_("Jump to Reference"), "<control>J", N_("Jump to reference"), NULL }
};

static const GtkActionEntry edit_actions[] = {
	{ "Cut", GTK_STOCK_CUT, N_("Cu_t"), "<control>X", N_("Cut"), NULL },
	{ "Copy", GTK_STOCK_COPY, N_("_Copy"), "<control>C", N_("Copy"), NULL },
	{ "Paste", GTK_STOCK_PASTE, N_("_Paste"), "<control>V", N_("Paste"), NULL }
};

static const GtkActionEntry find_replace_actions[] = {
	{ "Find", GTK_STOCK_FIND, N_("_Find..."), "<control>F", N_("Find"), NULL },
	{ "FindAgain", NULL, N_("Find A_gain"), "<control>G", N_("Find again"), NULL },
	{ "FindSelection", NULL, N_("Find From Selection"), "<shift><control>F", N_("Find from selection"), NULL },
	{ "Replace", GTK_STOCK_FIND_AND_REPLACE, N_("R_eplace..."), "<control>H", N_("Replace"), NULL },
	{ "ReplaceAgain", NULL, N_("Replace Agai_n"), "<shift><control>H", N_("Replace again"), NULL }
};

static const GtkActionEntry project_actions[] = {
	{ "ProjectClose", NULL, N_("Save & _close"), NULL, N_("Save & close project"), NULL },
	{ "ProjectSave", NULL, N_("_Save"), NULL, N_("Save project"), NULL },
	{ "ProjectSaveAs", NULL, N_("Save _as..."), NULL, N_("Save project as"), NULL },
	{ "ProjectEditOptions", NULL, N_("E_dit Project Options..."), NULL, N_("Edit project options"), NULL },
};

static const GtkActionEntry undo_redo_actions[] = {
	{ "Undo", GTK_STOCK_UNDO, N_("_Undo"), "<control>Z", N_("Undo"), NULL },
	{ "UndoAll", NULL, N_("Undo All"), NULL, N_("Undo All"), NULL },
	{ "Redo", GTK_STOCK_REDO, N_("_Redo"), "<shift><control>Z", N_("Redo"), NULL },
	{ "RedoAll", NULL, N_("Redo All"), NULL, N_("Redo All"), NULL }
};

void
main_menu_create(Tbfwin * bfwin, GtkWidget * vbox)
{
	GtkActionGroup *action_group;
	GtkUIManager *manager;
	GtkWidget *menubar;

	GError *error = NULL;

	manager = bfwin->uimanager;

	gtk_window_add_accel_group(GTK_WINDOW(bfwin->main_window), gtk_ui_manager_get_accel_group(manager));

	action_group = gtk_action_group_new("topLevelMenus");
	gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(action_group, top_level_menus, G_N_ELEMENTS(top_level_menus),
								 bfwin->main_window);
	gtk_ui_manager_insert_action_group(manager, action_group, 0);
	g_object_unref(action_group);

	action_group = gtk_action_group_new("GlobalActions");
	gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(action_group, global_actions, G_N_ELEMENTS(global_actions),
								 bfwin->main_window);
	gtk_action_group_add_toggle_actions(action_group, global_toggle_actions,
										G_N_ELEMENTS(global_toggle_actions), bfwin->main_window);
	gtk_ui_manager_insert_action_group(manager, action_group, 0);
	g_object_unref(action_group);
	bfwin->globalGroup = action_group;

	action_group = gtk_action_group_new("DocumentActions");
	gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(action_group, document_actions, G_N_ELEMENTS(document_actions),
								 bfwin->main_window);
	gtk_ui_manager_insert_action_group(manager, action_group, 0);
	gtk_action_group_set_sensitive(action_group, FALSE);
	g_object_unref(action_group);
	bfwin->documentGroup = action_group;

	action_group = gtk_action_group_new("EditActions");
	gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(action_group, edit_actions, G_N_ELEMENTS(edit_actions), bfwin->main_window);
	gtk_ui_manager_insert_action_group(manager, action_group, 0);
	gtk_action_group_set_sensitive(action_group, FALSE);
	g_object_unref(action_group);
	bfwin->editGroup = action_group;

	action_group = gtk_action_group_new("FindReplaceActions");
	gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(action_group, find_replace_actions, G_N_ELEMENTS(find_replace_actions),
								 bfwin->main_window);
	gtk_ui_manager_insert_action_group(manager, action_group, 0);
	gtk_action_group_set_sensitive(action_group, FALSE);
	g_object_unref(action_group);
	bfwin->findReplaceGroup = action_group;

	action_group = gtk_action_group_new("UndoRedoActions");
	gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(action_group, undo_redo_actions, G_N_ELEMENTS(undo_redo_actions),
								 bfwin->main_window);
	gtk_ui_manager_insert_action_group(manager, action_group, 0);
	gtk_action_group_set_sensitive(action_group, FALSE);
	g_object_unref(action_group);
	bfwin->undoGroup = action_group;

	action_group = gtk_action_group_new("ProjectActions");
	gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(action_group, project_actions, G_N_ELEMENTS(project_actions),
								 bfwin->main_window);
	gtk_ui_manager_insert_action_group(manager, action_group, 0);
	gtk_action_group_set_sensitive(action_group, FALSE);
	g_object_unref(action_group);
	bfwin->projectGroup = action_group;

	gtk_ui_manager_add_ui_from_file(manager, MAIN_MENU_UI, &error);
	if (error != NULL) {
		g_warning("building main menu failed: %s", error->message);
		g_error_free(error);
	}

	menubar = gtk_ui_manager_get_widget(manager, "/MainMenu");
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
	gtk_widget_show(menubar);
}

void
setup_menu_toggle_item(GtkActionGroup *action_group, const gchar *action_name, gboolean is_active)
{
	GtkAction *action = gtk_action_group_get_action(action_group, action_name);
	if (!action) {
		g_print("Cannot set-up menu widget %s\n", action_name);
		return;
	}

	if ((gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action))) != is_active)
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), is_active);
}
