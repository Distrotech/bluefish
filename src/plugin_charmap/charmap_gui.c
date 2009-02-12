/* Bluefish HTML Editor
 * charmap_gui.c - the charmap panel
 *
 * Copyright (C) 2009 Olivier Sessink
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

#include "charmap.h"
#include "charmap_gui.h"

#include <gucharmap/gucharmap.h> 

void charmap_sidepanel_destroygui(Tbfwin *bfwin) {
	
}
#ifdef HAVE_LIBGUCHARMAP_2
static void charmap_charmap_activate_lcb(GucharmapChartable *gucharmapchartable,gpointer data) {
	Tcharmap *cm = data;
	gchar ubuf[7];
	gunichar wc;
	wc = gucharmap_chartable_get_active_character(gucharmapchartable);
	ubuf[g_unichar_to_utf8 (wc, ubuf)] = '\0';
	g_print("charmap_plugin, clicked %s\n",ubuf);
	doc_insert_two_strings(cm->bfwin->current_document, ubuf, NULL);
}

#endif

#ifdef HAVE_LIBGUCHARMAP
static void charmap_charmap_activate_lcb(GtkWidget *chartable,gunichar wc,gpointer data) {
	Tcharmap *cm = data;
	gchar ubuf[7];

	ubuf[g_unichar_to_utf8 (wc, ubuf)] = '\0';
	g_print("charmap_plugin, clicked %s\n",ubuf);
	doc_insert_two_strings(cm->bfwin->current_document, ubuf, NULL);
}
#endif

static void chaptersv_changed_lcb(GtkComboBox *combo, gpointer data) {
	Tcharmap *cm = data;
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(combo, &iter)) {
		gchar *name;
		GucharmapCodepointList * gcpl;
		GtkTreeModel *model;
		
		model = gtk_combo_box_get_model(combo);
		gcpl = gucharmap_chapters_model_get_codepoint_list(model,&iter);
/*		gtk_tree_model_get(model, &iter, 0, &name, -1);
		gcpl = gucharmap_script_codepoint_list_new();
		gucharmap_script_codepoint_list_set_script(gcpl,name);*/
#ifdef HAVE_LIBGUCHARMAP		
		gucharmap_table_set_codepoint_list(cm->gcm,gcpl);
#endif
#ifdef HAVE_LIBGUCHARMAP_2
		gucharmap_chartable_set_codepoint_list (cm->gcm,gcpl);
#endif 
/*		g_free(name);*/
	}
}

void charmap_sidepanel_initgui(Tbfwin *bfwin) {
	Tcharmap *cm;
	GdkPixbuf *pixbuf;
	GtkWidget *image;
	GtkWidget *scrolwin, *vbox;
	GtkWidget *chapters;
	GtkTreeModel *model;
	/*GucharmapCodepointList * gcpl;*/
	GtkCellRenderer *renderer;
	
	vbox = gtk_vbox_new(FALSE,4);

	cm = g_new0(Tcharmap,1);
	cm->bfwin = bfwin;
/*	g_hash_table_insert(snippets_v.lookup,bfwin,cm);*/

	model = gucharmap_block_chapters_model_new();
/*	model = gucharmap_script_chapters_model_new();*/	
	cm->chaptersv = gtk_combo_box_new_with_model(model);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cm->chaptersv),renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cm->chaptersv), renderer, "text", 0, NULL);
	g_signal_connect(G_OBJECT(cm->chaptersv), "changed",G_CALLBACK(chaptersv_changed_lcb),cm);
	g_object_unref(model); 
	gtk_box_pack_start(GTK_BOX(vbox),cm->chaptersv,FALSE,TRUE,2);
#ifdef HAVE_LIBGUCHARMAP
	cm->gcm = gucharmap_table_new();
#endif
#ifdef HAVE_LIBGUCHARMAP_2
	cm->gcm = gucharmap_chartable_new();
#endif 
/*	gcpl = gucharmap_script_codepoint_list_new();
	gucharmap_script_codepoint_list_set_script(gcpl,"Nko");
	gucharmap_table_set_codepoint_list(cm->gcm,gcpl);*/
	g_signal_connect(cm->gcm, "activate", G_CALLBACK(charmap_charmap_activate_lcb), cm);
	gtk_combo_box_set_active(cm->chaptersv,0);
	/* these lines create a full gucharmap with vertical pane inside the sidebar
	this is not a nice GUI	
	chapters = gucharmap_script_chapters_new (); 
	cm->gcm = gucharmap_charmap_new(chapters);
	g_signal_connect (GUCHARMAP_CHARMAP(cm->gcm)->chartable, "activate", G_CALLBACK(charmap_charmap_activate_lcb), cm);
	*/
	
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#ifdef HAVE_LIBGUCHARMAP
	gtk_scrolled_window_add_with_viewport(scrolwin, cm->gcm);
#endif
#ifdef HAVE_LIBGUCHARMAP_2
	gtk_container_add(GTK_CONTAINER(scrolwin), cm->gcm);
#endif
	gtk_box_pack_start(GTK_BOX(vbox),scrolwin,TRUE,TRUE,4);
	
	gtk_notebook_insert_page_menu(GTK_NOTEBOOK(bfwin->leftpanel_notebook),vbox,gtk_label_new(_("charmap")),gtk_label_new(_("charmap")),2);

	

}

