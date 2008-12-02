/* Bluefish HTML Editor
 * htmlbar.c - plugin for html toolbar
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

#include <string.h>
#include "htmlbar.h"
#include <gmodule.h>
#include "../plugins.h"
#include "../rcfile.h"
#include "../document.h"
#include "../gtk_easy.h"
#include "htmlbar_gui.h"
#include "rpopup.h"
Thtmlbar htmlbar_v;

static void htmlbar_doc_view_populate_popup(GtkTextView *textview,GtkMenu *menu,Tdocument *doc) {
	GtkWidget *menuitem;
	DEBUG_MSG("htmlbar_doc_view_populate_popup, called\n");

	menuitem = gtk_image_menu_item_new_with_label(_("Edit color"));
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));
	if (rpopup_doc_located_color(doc)) { 	 
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(rpopup_edit_color_cb), doc); 	 
	} else { 	 
		gtk_widget_set_sensitive(menuitem, FALSE); 	 
	} 	 

	menuitem = gtk_image_menu_item_new_with_label(_("Edit tag")); 	 
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),htmlbar_pixmap(pixmap_edit_tag));
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem)); 	 
	if (rpopup_doc_located_tag(doc)) { 	 
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(rpopup_edit_tag_cb), doc); 	 
	} else { 	 
		gtk_widget_set_sensitive(menuitem, FALSE); 	 
	}

/*	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(gtk_menu_item_new()));

	menuitem = gtk_image_menu_item_new_with_label(_("Plugin test"));
	g_signal_connect(menuitem, "activate", G_CALLBACK(search_cb), doc->bfwin);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem)); */
}
static void htmlbar_doc_view_button_press(GtkWidget *widget,GdkEventButton *bevent, Tdocument *doc) {
	DEBUG_MSG("htmlbar_doc_view_button_press, called\n");
	if (bevent->button == 3) {
		GtkTextIter iter;
		doc_get_iter_at_bevent(doc, bevent, &iter);
		rpopup_bevent_in_html_code(doc, &iter); 	 
	}
}

static void htmlbar_init(void) {
#ifdef ENABLE_NLS
	DEBUG_MSG("htmlbar_init, gettext domain-name=%s\n",PACKAGE"_plugin_htmlbar");
	bindtextdomain(PACKAGE"_plugin_htmlbar", LOCALEDIR);
	bind_textdomain_codeset(PACKAGE"_plugin_htmlbar", "UTF-8");
#endif
	htmlbar_v.quickbar_items = NULL;
	htmlbar_v.lookup = g_hash_table_new_full(NULL /* == g_direct_hash() */,
					NULL /* == g_direct_equal() */,
					NULL,NULL);
	DEBUG_MSG("htmlbar_init, will add doc_view_button_press %p\n",htmlbar_doc_view_button_press);
	main_v->doc_view_populate_popup_cbs = g_slist_prepend(main_v->doc_view_populate_popup_cbs,htmlbar_doc_view_populate_popup);
	main_v->doc_view_button_press_cbs = g_slist_prepend(main_v->doc_view_button_press_cbs,htmlbar_doc_view_button_press);
	DEBUG_MSG("htmlbar_init, finished\n");
}
static void htmlbar_initgui(Tbfwin* bfwin) {
	Thtmlbarwin *hbw;
	Thtmlbarsession *hbs;

	hbw = g_new0(Thtmlbarwin,1);
	hbw->bfwin = bfwin;
	g_hash_table_insert(htmlbar_v.lookup,bfwin,hbw);
	DEBUG_MSG("htmlbar_initgui, adding hbw %p to hashtable %p with key %p\n",hbw,htmlbar_v.lookup,bfwin);
	hbs = g_hash_table_lookup(htmlbar_v.lookup,bfwin->session);
	if (!hbs) {
		hbs = g_new0(Thtmlbarsession,1);
		hbs->view_htmlbar = TRUE;
		DEBUG_MSG("htmlbar_initgui, adding hbs %p to hashtable %p with key %p\n",hbs,htmlbar_v.lookup,bfwin->session);
		g_hash_table_insert(htmlbar_v.lookup,bfwin->session,hbs);
	}
	DEBUG_MSG("htmlbar_initgui, started, will call htmlbar_build_menu\n");
	htmlbar_build_menu(hbw);
	htmlbar_view_toolbar(hbw, hbs->view_htmlbar);
	DEBUG_MSG("htmlbar_initgui, finished\n");
}
static void htmlbar_enforce_session(Tbfwin* bfwin) {
	Thtmlbarsession *hbs;
	Thtmlbarwin *hbw;
	DEBUG_MSG("htmlbar_enforce_session, started for session %p\n",bfwin->session);
	hbs = g_hash_table_lookup(htmlbar_v.lookup,bfwin->session);
	hbw = g_hash_table_lookup(htmlbar_v.lookup,bfwin);
	if (hbs && hbw) {
		htmlbar_view_toolbar(hbw, hbs->view_htmlbar);
	}
}
static void htmlbar_cleanup(void) {
	GList *tmplist = g_list_first(gtk_window_list_toplevels());
	while (tmplist) {
		if (GTK_IS_WIDGET(tmplist->data)) {
			const gchar *role= gtk_window_get_role ((GtkWindow*)tmplist->data);
			if (role && strcmp(role,"html_dialog") ==0) {
				gtk_widget_hide(GTK_WIDGET(tmplist->data));
				window_destroy(GTK_WIDGET(tmplist->data));
			}
		}
		tmplist = g_list_next(tmplist);
	}
}

static void htmlbar_cleanup_gui(Tbfwin *bfwin) {
	/* BUG: clean the keys and structures in the hashtable */
}

static GHashTable *htmlbar_register_globses_config(GHashTable *configlist) {
	DEBUG_MSG("htmlbar_register_globses_config, started\n");
/*	configlist = make_config_list_item(configlist, &htmlbar_v.view_htmlbar, 'i', "view_htmlbar", 0);*/
	configlist = make_config_list_item(configlist, &htmlbar_v.quickbar_items, 'l', "htmlbar_quickbar:", 0);
	return configlist;
}
static GHashTable *htmlbar_register_session_config(GHashTable *configlist, Tsessionvars *session) {
	Thtmlbarsession *hbs;
	DEBUG_MSG("htmlbar_register_session_config, started for %p\n",session);
	hbs = g_hash_table_lookup(htmlbar_v.lookup,session);
	if (!hbs) {
		hbs = g_new0(Thtmlbarsession,1);
		hbs->view_htmlbar = TRUE;
		g_hash_table_insert(htmlbar_v.lookup,session,hbs);
		DEBUG_MSG("htmlbar_register_session_config, adding hbs %p to hashtable %p with key %p\n",hbs,htmlbar_v.lookup,session);
	}
	configlist = make_config_list_item(configlist, &hbs->view_htmlbar, 'i', "htmlbar_view:", 0);
	return configlist;
}

static TBluefishPlugin bfplugin = {
	"htmlbar",
	BFPLUGIN_VERSION,
	sizeof(Tdocument),
	sizeof(Tsessionvars),
	sizeof(Tglobalsession),
	sizeof(Tbfwin),
	sizeof(Tproject),
	sizeof(Tmain),
	NULL, /* private */
	htmlbar_init, /* init */
	htmlbar_initgui, 
	htmlbar_enforce_session,
	htmlbar_cleanup,
	htmlbar_cleanup_gui,
	htmlbar_register_globses_config,
	htmlbar_register_session_config,
	NULL, /* binary compatibility */
	NULL,
	NULL,
	NULL
};

G_MODULE_EXPORT TBluefishPlugin *getplugin(void) {
	return &bfplugin;
}
