#include "htmlbar.h"
#include "../plugins.h"
#include "../rcfile.h"
#include "htmlbar_gui.h"

Thtmlbar htmlbar_v;

static void htmlbar_doc_view_populate_popup(GtkTextView *textview,GtkMenu *menu,Tdocument *doc) {
	GtkWidget *menuitem;
	DEBUG_MSG("htmlbar_doc_view_populate_popup, called\n");

	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(gtk_menu_item_new()));

	menuitem = gtk_image_menu_item_new_with_label(_("Plugin test"));
/*	g_signal_connect(menuitem, "activate", G_CALLBACK(search_cb), doc->bfwin);*/
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));
}
static void htmlbar_doc_view_button_press(GtkWidget *widget,GdkEventButton *bevent, Tdocument *doc) {
	DEBUG_MSG("htmlbar_doc_view_button_press, called\n");
}

static void htmlbar_init(void) {
#ifdef ENABLE_NLS
	DEBUG_MSG("htmlbar_init, gettext domain-name=%s\n",PACKAGE"_plugin_htmlbar");
	bindtextdomain(PACKAGE"_plugin_htmlbar", LOCALEDIR);
	bind_textdomain_codeset(PACKAGE"_plugin_htmlbar", "UTF-8");
#endif
	htmlbar_v.quickbar_items = NULL;
	htmlbar_v.view_htmlbar = TRUE;
	htmlbar_v.lookup = g_hash_table_new_full(NULL /* == g_direct_hash() */,
					NULL /* == g_direct_equal() */,
					g_free,NULL);
	DEBUG_MSG("htmlbar_init, will add doc_view_button_press %p\n",htmlbar_doc_view_button_press);
	main_v->doc_view_populate_popup_cbs = g_slist_prepend(main_v->doc_view_populate_popup_cbs,htmlbar_doc_view_populate_popup);
	main_v->doc_view_button_press_cbs = g_slist_prepend(main_v->doc_view_button_press_cbs,htmlbar_doc_view_button_press);
	DEBUG_MSG("htmlbar_init, finished\n");
}
static void htmlbar_initgui(Tbfwin* bfwin) {
	Thtmlbarwin *hbw;
	gpointer *key;

	key = g_new(gpointer,1);
	hbw = g_new0(Thtmlbarwin,1);
	hbw->bfwin = bfwin;
	*key = bfwin;
	g_hash_table_insert(htmlbar_v.lookup,key,hbw);
	DEBUG_MSG("htmlbar_initgui, started, will call htmlbar_build_menu\n");
	htmlbar_build_menu(hbw);
	if (htmlbar_v.view_htmlbar) {
		htmlbar_toolbar(hbw);
	}
	DEBUG_MSG("htmlbar_initgui, finished\n");
}
static void htmlbar_cleanup(void) {

}
static void htmlbar_cleanup_gui(Tbfwin *bfwin) {

}

static GList *htmlbar_register_globses_config(GList *configlist) {
/*	configlist = make_config_list_item(configlist, &htmlbar_v.view_htmlbar, 'i', "view_htmlbar", 0);*/
	return configlist;
}
static GList *htmlbar_register_session_config(GList *configlist) {
	configlist = make_config_list_item(configlist, &htmlbar_v.view_htmlbar, 'i', "htmlbar_view:", 0);
	configlist = make_config_list_item(configlist, &htmlbar_v.quickbar_items, 'l', "htmlbar_quickbar:", 0);
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
	htmlbar_cleanup,
	htmlbar_cleanup_gui,
	htmlbar_register_globses_config,
	htmlbar_register_session_config,
	NULL, /* binary compatibility */
	NULL,
	NULL,
	NULL
};

TBluefishPlugin *getplugin(void) {
	return &bfplugin;
}
