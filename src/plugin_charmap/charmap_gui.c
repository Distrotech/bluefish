#include "charmap.h"
#include "charmap_gui.h"

#include <gucharmap/gucharmap.h> 

void charmap_sidepanel_destroygui(Tbfwin *bfwin) {
	
}

static void charmap_charmap_activate_lcb(GucharmapTable  *chartable,gunichar wc,gpointer data) {
	Tcharmap *cm = data;
	gchar ubuf[7];

	ubuf[g_unichar_to_utf8 (wc, ubuf)] = '\0';
	g_print("charmap_plugin, clicked %s\n",ubuf);
	doc_insert_two_strings(cm->bfwin->current_document, ubuf, NULL);
}

static void chaptersv_changed_lcb(GtkComboBox *combo, gpointer data) {
	Tcharmap *cm = data;
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(combo, &iter)) {
		gchar *name;
		GtkTreeModel *model = gtk_combo_box_get_model(combo);
		gtk_tree_model_get(model, &iter, 0, &name, -1);
		GucharmapCodepointList * gcpl;
		gcpl = gucharmap_script_codepoint_list_new();
		gucharmap_script_codepoint_list_set_script(gcpl,name);
		gucharmap_table_set_codepoint_list(cm->gcm,gcpl);
		g_free(name);
	}
}

void charmap_sidepanel_initgui(Tbfwin *bfwin) {
	Tcharmap *cm;
	GdkPixbuf *pixbuf;
	GtkWidget *image;
	GtkWidget *scrolwin, *vbox;
	GtkWidget *chapters;
	GtkTreeModel *model;
	GucharmapCodepointList * gcpl;
	GtkCellRenderer *renderer;
	
	vbox = gtk_vbox_new(FALSE,4);

	cm = g_new0(Tcharmap,1);
	cm->bfwin = bfwin;
/*	g_hash_table_insert(snippets_v.lookup,bfwin,cm);*/


	model = gucharmap_script_chapters_model_new();	
	/*cm->chaptersv = gucharmap_chapters_new_with_model(model);*/
	cm->chaptersv = gtk_combo_box_new_with_model(model);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cm->chaptersv),renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cm->chaptersv), renderer, "text", 0, NULL);
	g_signal_connect(G_OBJECT(cm->chaptersv), "changed",G_CALLBACK(chaptersv_changed_lcb),cm);
	g_object_unref(model); 
	gtk_box_pack_start(GTK_BOX(vbox),cm->chaptersv,FALSE,TRUE,4);
	
	
	cm->gcm = gucharmap_table_new();
	gcpl = gucharmap_script_codepoint_list_new();
	gucharmap_script_codepoint_list_set_script(gcpl,"Nko");
	gucharmap_table_set_codepoint_list(cm->gcm,gcpl);
	g_signal_connect(cm->gcm, "activate", G_CALLBACK(charmap_charmap_activate_lcb), cm);
	
	/* these lines create a full gucharmap with vertical pane inside the sidebar
	this is not a nice GUI	
	chapters = gucharmap_script_chapters_new (); 
	cm->gcm = gucharmap_charmap_new(chapters);
	g_signal_connect (GUCHARMAP_CHARMAP(cm->gcm)->chartable, "activate", G_CALLBACK(charmap_charmap_activate_lcb), cm);
	*/
	
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(scrolwin, cm->gcm);
	/*gtk_container_add(GTK_CONTAINER(scrolwin), cm->gcm);*/
	gtk_box_pack_start(GTK_BOX(vbox),scrolwin,TRUE,TRUE,4);
	
	gtk_notebook_insert_page_menu(GTK_NOTEBOOK(bfwin->leftpanel_notebook),vbox,gtk_label_new(_("charmap")),gtk_label_new(_("charmap")),2);

	

}

