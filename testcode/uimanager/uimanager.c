/* test-ui-mananager.c : Simple tutorial for GtkUIManager
 * Copyright (C) 2005 Ryan McDougall.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

gcc -o uimanager uimanager.c -O2 -Wall `pkg-config --libs --cflags gtk+-2.0`


 */

#include <gtk/gtk.h>

#include "inline_icons.c"


static void set_action_toggle_wo_activate(GtkActionGroup *actiongroup, const gchar *actionname, gboolean value) {
	GtkAction *action = gtk_action_group_get_action(actiongroup, actionname);
	gtk_action_block_activate(action);
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), value);
	gtk_action_unblock_activate(action);
}

/*static void doc_sync_menu() {
	set_action_toggle_wo_activate(action_group, TRUE);
}
*/
/* Create callbacks that implement our Actions */
static void rescan_syntax_action(GtkAction * action, gpointer user_data)
{
	g_print("rescan syntax\n");
}

static void auto_indent_action(GtkAction * action, gpointer user_data)
{
	g_print("auto indent = %d\n",gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));
}

static void quit_action(GtkAction * action, gpointer user_data)
{
	gtk_main_quit();
}

static void new_window();

static void new_window_action(GtkAction * action, gpointer user_data)
{
	new_window();
}

/* Create a list of entries which are passed to the Action constructor. 
 * This is a huge convenience over building Actions by hand. */
static GtkActionEntry entries[] = {
	{"FileMenuAction", NULL, "_File"},	/* name, stock id, label */
	{"DocumentMenuAction", NULL, "_Document"},
	{"LanguageModeMenuAction", NULL, "_Language Mode"},
	
	{"NewWindowAction", NULL, "New _Window", "<shift><control>n", "Create new window", G_CALLBACK(new_window_action)},


	{"RescanSyntaxAction", NULL,
	 "Rescan _syntax", "F5",
	 "Re-scan the language syntax",
	 G_CALLBACK(rescan_syntax_action)},

	{"QuitAction", GTK_STOCK_QUIT,
	 "_Quit", "<control>Q",
	 "Quit",
	 G_CALLBACK(quit_action)}
};

static guint n_entries = G_N_ELEMENTS(entries);

static GtkToggleActionEntry toggleactions[] = {
	{"AutoIndentAction", NULL,	/* name, stock id */
	 "Auto _Indent", NULL,	/* label, accelerator */
	 "Enable automatic indenting",	/* tooltip */
	 G_CALLBACK(auto_indent_action), 0}
};

static void anchor_action(GtkAction * action, gpointer user_data)
{
	g_print("anchor\n");
}


static GtkActionEntry htmlentries[] = {
	{"DialogsAction", NULL, "_Dialogs"},
	{"AnchorAction", "view_in_browser", "_Anchor", NULL, "Anchor / hyperlink",
	 G_CALLBACK(anchor_action)}
};

static const gchar * htmlui = "<ui>"
	"<menubar name='MainMenu'>"
	"<placeholder name='PluginPlaceholder'>"
	"<menu name='Dialogs' action='DialogsAction'>"
		"<menuitem name='Anchor' action='AnchorAction' />"
	"</menu>"
	"</placeholder>"
	"</menubar>"
	"<toolbar name='HTMLToolbar' action='HTMLToolBarAction'>"
	"<placeholder name='ToolItems'>"
	"<separator />"
	"<toolitem name='Anchor' action='AnchorAction'/>"
	"<separator />"
	"</placeholder>"
	"</toolbar>"
"</ui>";

static void plugin_gui(GtkUIManager *menu_manager) {
	GtkActionGroup *htmlactiongroup;	/* Packing group for our Actions */
	GError *error;				/* For reporting exceptions or errors */
	
	htmlactiongroup = gtk_action_group_new("HTMLActions");
	gtk_action_group_add_actions(htmlactiongroup, htmlentries,
								 G_N_ELEMENTS(htmlentries), NULL);
	gtk_ui_manager_insert_action_group(menu_manager, htmlactiongroup, 0);
/*	gtk_ui_manager_add_ui_from_file(menu_manager, "htmlui.xml", &error);*/
	gtk_ui_manager_add_ui_from_string(menu_manager, htmlui, -1, &error);
	if (error) {
		g_message("building html menus failed: %s", error->message);
		g_error_free(error);
		error = NULL;
	}

	
} 

static void language_mode_changed(GtkWidget *widget, gpointer user_data) {
	g_print("language_mode_changed, widget=%p\n",widget);
}


static void new_window() {
	GList *langlist, *tmplist;
	GSList *group;
	GtkUIManager *menu_manager;	/* The magic widget! */
	GtkActionGroup *action_group;	/* Packing group for our Actions */
	GError *error;				/* For reporting exceptions or errors */
	GtkWidget *window;			/* The main window */
	GtkWidget *menu_box;		/* Packing box for the menu and toolbars */
	GtkWidget *menubar;			/* The actual menubar */
	GtkWidget *toolbar;			/* The actual toolbar */

	/* Create our objects */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	menu_box = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), menu_box);

	menu_manager = gtk_ui_manager_new();
	action_group = gtk_action_group_new("TestActions");
	gtk_action_group_set_translation_domain(action_group, "blah");
	gtk_action_group_add_actions(action_group, entries, n_entries, NULL);
	gtk_action_group_add_toggle_actions(action_group, toggleactions, G_N_ELEMENTS(toggleactions), NULL);
	gtk_ui_manager_insert_action_group(menu_manager, action_group, 0);

	/* Read in the UI from our XML file */
	error = NULL;
	gtk_ui_manager_add_ui_from_file(menu_manager, "mainui.xml", &error);
	if (error) {
		g_message("building menus failed: %s", error->message);
		g_error_free(error);
		error = NULL;
	}
	/* add a dynamic radiogroup */
	langlist = g_list_prepend(NULL, "Generic XML");
	langlist = g_list_prepend(langlist, "HTML");
	langlist = g_list_prepend(langlist, "XHTML");
	
	tmplist = g_list_first(langlist);
	group = NULL;
	while (tmplist) {
		gchar *actionname=g_strconcat((gchar *)tmplist->data, "Action", NULL);
		GtkRadioAction * raction = gtk_radio_action_new(actionname,
                                                         (gchar *)tmplist->data,
                                                         NULL,
                                                         NULL,
                                                         0);
		gtk_radio_action_set_group(raction, group);
		g_signal_connect(raction, "changed", G_CALLBACK(language_mode_changed), NULL);
		gtk_action_group_add_action(action_group, GTK_ACTION(raction));
		gtk_ui_manager_add_ui(menu_manager, gtk_ui_manager_new_merge_id(menu_manager)
					, "/MainMenu/DocumentMenu/LanguageModeMenu"
					, (gchar *)tmplist->data
					, actionname
					, GTK_UI_MANAGER_MENUITEM
					, FALSE);
		group = gtk_radio_action_get_group(raction);
		tmplist = g_list_next(tmplist);
	}

	plugin_gui(menu_manager);

	/* Connect up important signals */
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	gtk_ui_manager_set_add_tearoffs(menu_manager, TRUE);

	/* Get the menubar and the toolbar and put them in the vertical packing box */
	menubar = gtk_ui_manager_get_widget(menu_manager, "/MainMenu");
	gtk_box_pack_start(GTK_BOX(menu_box), menubar, FALSE, FALSE, 0);
	toolbar = gtk_ui_manager_get_widget(menu_manager, "/MainToolbar");
	gtk_box_pack_start(GTK_BOX(menu_box), toolbar, FALSE, FALSE, 0);
/*	toolbar = gtk_ui_manager_get_widget(menu_manager, "/HTMLToolbar");
	gtk_box_pack_start(GTK_BOX(menu_box), toolbar, FALSE, FALSE, 0);
*/	/* Make sure that the accelerators work */
	gtk_window_add_accel_group(GTK_WINDOW(window),
							   gtk_ui_manager_get_accel_group
							   (menu_manager));

	/* Show the window and run the main loop, we're done! */
	gtk_widget_show_all(window);
	
	
}


/*---------------------------------------------------------------------------*/

int main(int argc, char **argv)
{
	GdkPixbuf *pixbuf;
	/* Always the first step is to start up GTK+ itself */
	gtk_init(&argc, &argv);

	pixbuf = gdk_pixbuf_new_from_inline(-1,pixmap_view_in_browser,FALSE,NULL);
	gtk_icon_theme_add_builtin_icon("view_in_browser",24,pixbuf);
	
	new_window();
	gtk_main();

	return 0;
}
