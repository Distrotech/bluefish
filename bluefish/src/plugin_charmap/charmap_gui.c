#include "charmap.h"
#include "charmap_gui.h"

#include <gucharmap/gucharmap.h> 

void charmap_sidepanel_destroygui(Tbfwin *bfwin) {
	
}
void charmap_sidepanel_initgui(Tbfwin *bfwin) {
	Tcharmap *cm;
	GdkPixbuf *pixbuf;
	GtkWidget *image;
	GtkWidget *scrolwin;

	cm = g_new0(Tcharmap,1);
	cm->bfwin = bfwin;
/*	g_hash_table_insert(snippets_v.lookup,bfwin,cm);*/	
	
	cm->gcm = gucharmap_table_new();
	
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), cm->gcm);
	gtk_notebook_insert_page_menu(GTK_NOTEBOOK(bfwin->leftpanel_notebook),scrolwin,gtk_label_new(_("charmap")),gtk_label_new(_("charmap")),2);

}

