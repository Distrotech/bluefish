#include "charmap.h"
#include "charmap_gui.h"

#include <gucharmap/gucharmap.h> 

void charmap_sidepanel_destroygui(Tbfwin *bfwin) {
	
}

static void charmap_charmap_activate_lcb(GucharmapTable  *chartable,gunichar wc,gpointer data) {
	Tcharmap *cm = data;
	gchar ubuf[7];

	ubuf[g_unichar_to_utf8 (wc, ubuf)] = '\0';
	g_print("insert character %s\n",ubuf);
}

void charmap_sidepanel_initgui(Tbfwin *bfwin) {
	Tcharmap *cm;
	GdkPixbuf *pixbuf;
	GtkWidget *image;
	GtkWidget *scrolwin, *vbox;
	GtkWidget *chapters;
	GtkTreeModel *model;

	vbox = gtk_vbox_new(FALSE,4);

	cm = g_new0(Tcharmap,1);
	cm->bfwin = bfwin;
/*	g_hash_table_insert(snippets_v.lookup,bfwin,cm);*/

	chapters = gucharmap_script_chapters_new ();

/*
	model = gucharmap_script_chapters_model_new();	
	cm->chaptersv = gucharmap_chapters_new_with_model(model);
	g_object_unref(model); 
	
	gtk_box_pack_start(GTK_BOX(vbox),cm->chaptersv,TRUE,TRUE,4);*/
	 
	cm->gcm = gucharmap_charmap_new(chapters);
	
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), cm->gcm);
	gtk_box_pack_start(GTK_BOX(vbox),scrolwin,TRUE,TRUE,4);
	
	gtk_notebook_insert_page_menu(GTK_NOTEBOOK(bfwin->leftpanel_notebook),vbox,gtk_label_new(_("charmap")),gtk_label_new(_("charmap")),2);

	g_signal_connect (GUCHARMAP_CHARMAP(cm->gcm)->chartable, "activate", G_CALLBACK(charmap_charmap_activate_lcb), cm);

}

