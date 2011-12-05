/* Bluefish HTML Editor
 * charmap_gui.c - the charmap panel
 *
 * Copyright (C) 2009-2010 Olivier Sessink
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
/*#define DEBUG*/

#include "charmap.h"
#include "charmap_gui.h"
#include "../document.h"
#include "../gtk_easy.h"
#include <gucharmap/gucharmap.h>

#ifdef HAVE_LIBGUCHARMAP
#include <gucharmap/gucharmap-block-chapters-model.h>
#endif

#ifdef __SUNPRO_C
#pragma align 4 (charmap_icon)
#endif
#ifdef __GNUC__
static const guint8 charmap_icon[] __attribute__ ((__aligned__ (4))) = 
#else
static const guint8 charmap_icon[] = 
#endif
{ ""
  /* Pixbuf magic (0x47646b50) */
  "GdkP"
  /* length: header (24) + pixel_data (800) */
  "\0\0\3""8"
  /* pixdata_type (0x2010001) */
  "\2\1\0\1"
  /* rowstride (72) */
  "\0\0\0H"
  /* width (24) */
  "\0\0\0\30"
  /* height (24) */
  "\0\0\0\30"
  /* pixel_data: */
  "\213\374\376\373\1\246\250\245\216\374\376\373\2gif\246\250\245\204\374"
  "\376\373\4\316\320\315HJG\374\376\373\246\250\245\202\374\376\373\5\316"
  "\320\315HJG\24\25\23HJG\316\320\315\207\374\376\373\2\216\217\215.0-"
  "\204\374\376\373\202gif\2\374\376\373\246\250\245\202\374\376\373\5."
  "0-gif\374\376\373\216\217\215\24\25\23\210\374\376\373\2\24\25\23gif"
  "\203\216\217\215\7\24\25\23\316\320\315\374\376\373\246\250\245\374\376"
  "\373\316\320\315\24\25\23\203\374\376\373\2gif\316\320\315\207\374\376"
  "\373\2HJG.0-\202\216\217\215\2HJG.0-\202\374\376\373\4\246\250\245\374"
  "\376\373\246\250\245.0-\214\374\376\373\2\246\250\245\24\25\23\202\374"
  "\376\373\2HJG\216\217\215\202\374\376\373\4\246\250\245\374\376\373\216"
  "\217\215HJG\215\374\376\373\5\24\25\23\316\320\315\374\376\373\24\25"
  "\23\316\320\315\202\374\376\373\4\246\250\245\374\376\373\216\217\215"
  ".0-\215\374\376\373\202gif\2\216\217\215HJG\203\374\376\373\4\246\250"
  "\245\374\376\373\316\320\315\24\25\23\203\374\376\373\2\246\250\245\316"
  "\320\315\210\374\376\373\4\316\320\315\24\25\23.0-\246\250\245\203\374"
  "\376\373\1\246\250\245\202\374\376\373\5\24\25\23\246\250\245\374\376"
  "\373\316\320\315\24\25\23\212\374\376\373\2HJG\24\25\23\204\374\376\373"
  "\1\246\250\245\202\374\376\373\5\246\250\245\24\25\23.0-\24\25\23\246"
  "\250\245\220\374\376\373\1\246\250\245\204\374\376\373\1\316\320\315"
  "\207\374\376\373\230\246\250\245\213\374\376\373\1\246\250\245\216\374"
  "\376\373\210gif\2\374\376\373\246\250\245\203\374\376\373\1\316\320\315"
  "\202\216\217\215\203gif\1\316\320\315\204\374\376\373\2gif.0-\204\216"
  "\217\215\4.0-gif\374\376\373\246\250\245\202\374\376\373\3\216\217\215"
  "\24\25\23gif\204\216\217\215\1\316\320\315\204\374\376\373\2\316\320"
  "\315\24\25\23\204\374\376\373\7\24\25\23\316\320\315\374\376\373\246"
  "\250\245\374\376\373\246\250\245.0-\214\374\376\373\2.0-\246\250\245"
  "\202\374\376\373\2\246\250\245.0-\202\374\376\373\4\246\250\245\374\376"
  "\373HJG\246\250\245\214\374\376\373\2\216\217\215HJG\202\374\376\373"
  "\2HJG\216\217\215\202\374\376\373\3\246\250\245\374\376\373\24\25\23"
  "\207gif\1\316\320\315\206\374\376\373\1\24\25\23\202\316\320\315\1\24"
  "\25\23\203\374\376\373\3\246\250\245\374\376\373\24\25\23\207\246\250"
  "\245\1\316\320\315\206\374\376\373\202gif\2\216\217\215HJG\203\374\376"
  "\373\4\246\250\245\374\376\373gif\216\217\215\215\374\376\373\1\316\320"
  "\315\202\24\25\23\1\316\320\315\203\374\376\373\1\246\250\245\202\374"
  "\376\373\2\24\25\23\216\217\215\215\374\376\373\202.0-\204\374\376\373"
  "\1\246\250\245\202\374\376\373\3\316\320\315HJG\24\25\23\204.0-\1\246"
  "\250\245\215\374\376\373\1\246\250\245\205\374\376\373\204\316\320\315"
  "\216\374\376\373\1\246\250\245\214\374\376\373"};

void charmap_sidepanel_destroygui(Tbfwin *bfwin) {
	g_hash_table_remove(charmap_v.lookup,bfwin);
}

static void insert_unichar(Tcharmapwin *cm, gunichar wc) {
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cm->entities))) {
		gchar *buf = g_strdup_printf("&#%d;",wc);
		DEBUG_MSG("charmap_plugin, clicked %s\n",buf);
		doc_insert_two_strings(cm->bfwin->current_document, buf, NULL);
		g_free(buf);
	} else {
		gchar ubuf[7];
		ubuf[g_unichar_to_utf8 (wc, ubuf)] = '\0';
		DEBUG_MSG("charmap_plugin, clicked %s\n",ubuf);
		doc_insert_two_strings(cm->bfwin->current_document, ubuf, NULL);
	}
}

#ifdef HAVE_LIBGUCHARMAP_2
static void charmap_charmap_activate_lcb(GucharmapChartable *gucharmapchartable,gpointer data) {
	insert_unichar((Tcharmapwin *)data, gucharmap_chartable_get_active_character(gucharmapchartable));
}
#endif
#ifdef HAVE_LIBGUCHARMAP
static void charmap_charmap_activate_lcb(GtkWidget *chartable,gunichar wc,gpointer data) {
	insert_unichar((Tcharmapwin *)data, wc);
}
#endif

static void chaptersv_changed_lcb(GtkComboBox *combo, gpointer data) {
	Tcharmapwin *cm = data;
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(combo, &iter)) {
		GucharmapCodepointList * gcpl;
		GtkTreeModel *model;
		Tcharmapsession *cms;
		
		model = gtk_combo_box_get_model(combo);
		gcpl = gucharmap_chapters_model_get_codepoint_list((GucharmapChaptersModel *)model,&iter);
/*		gtk_tree_model_get(model, &iter, 0, &name, -1);
		gcpl = gucharmap_script_codepoint_list_new();
		gucharmap_script_codepoint_list_set_script(gcpl,name);*/
#ifdef HAVE_LIBGUCHARMAP		
		gucharmap_table_set_codepoint_list((GucharmapTable *)cm->gcm,gcpl);
#endif
#ifdef HAVE_LIBGUCHARMAP_2
		gucharmap_chartable_set_codepoint_list ((GucharmapChartable *)cm->gcm,gcpl);
#endif 
/*		g_free(name);*/
		cms = get_charmap_session(cm->bfwin->session);
		cms->charmap_block = gtk_combo_box_get_active(combo);
		/*g_print("save block %d\n",cms->charmap_block);*/
	}
}

void charmap_sidepanel_initgui(Tbfwin *bfwin) {
	Tcharmapwin *cm;
	Tcharmapsession *cms;
	GdkPixbuf *pixbuf;
	GtkWidget *image;
	GtkWidget *scrolwin, *vbox;
	GtkCellRenderer *renderer;
#ifdef HAVE_LIBGUCHARMAP_2
	PangoFontDescription *fontdesc;
#endif
 
	vbox = gtk_vbox_new(FALSE,4);

	cm = g_new0(Tcharmapwin,1);
	cm->bfwin = bfwin;
	DEBUG_MSG("created cm=%p for bfwin %p\n",cm,bfwin);
	g_hash_table_insert(charmap_v.lookup,bfwin,cm);
	cms = get_charmap_session(bfwin->session);
	
	if (!charmap_v.model)
		charmap_v.model = GTK_TREE_MODEL(gucharmap_block_chapters_model_new());
/*	charmap_v.model = gucharmap_script_chapters_model_new();*/	
	cm->chaptersv = gtk_combo_box_new_with_model(charmap_v.model);
	renderer = gtk_cell_renderer_text_new();
#if GTK_CHECK_VERSION(3,0,0)
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_MIDDLE, NULL);
#endif
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cm->chaptersv),renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cm->chaptersv), renderer, "text", 0, NULL);
	g_signal_connect(G_OBJECT(cm->chaptersv), "changed",G_CALLBACK(chaptersv_changed_lcb),cm);
 
	gtk_box_pack_start(GTK_BOX(vbox),cm->chaptersv,FALSE,TRUE,2);
#ifdef HAVE_LIBGUCHARMAP
	cm->gcm = gucharmap_table_new();
	gucharmap_table_set_font((GucharmapTable *)cm->gcm, "sans 12");
#endif
#ifdef HAVE_LIBGUCHARMAP_2
	cm->gcm = gucharmap_chartable_new();
	fontdesc = pango_font_description_from_string("sans 12");
	gucharmap_chartable_set_font_desc((GucharmapChartable *)cm->gcm, fontdesc);
	pango_font_description_free(fontdesc); 
#endif 
	g_signal_connect(cm->gcm, "activate", G_CALLBACK(charmap_charmap_activate_lcb), cm);
	/*g_print("activate block %d\n",cms->charmap_block);*/
	gtk_combo_box_set_active(GTK_COMBO_BOX(cm->chaptersv),cms->charmap_block);
	

	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#ifdef HAVE_LIBGUCHARMAP
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolwin), cm->gcm);
#endif
#ifdef HAVE_LIBGUCHARMAP_2
	gtk_container_add(GTK_CONTAINER(scrolwin), cm->gcm);
#endif
	gtk_box_pack_start(GTK_BOX(vbox),scrolwin,TRUE,TRUE,4);
	
	cm->entities = boxed_checkbut_with_value(_("Insert as HTML entity"), 0, vbox);
#if GTK_CHECK_VERSION(3,0,0)
	gtk_label_set_ellipsize(GTK_LABEL(gtk_bin_get_child(GTK_BIN(cm->entities))), PANGO_ELLIPSIZE_MIDDLE);
#endif
	/*gtk_box_pack_start(GTK_BOX(vbox),cm->entities,FALSE,TRUE,4);*/
	
	pixbuf = gdk_pixbuf_new_from_inline(-1,charmap_icon,FALSE,NULL);
	image = gtk_image_new_from_pixbuf(pixbuf);
	g_object_unref(pixbuf);
	
	gtk_notebook_insert_page_menu(GTK_NOTEBOOK(bfwin->leftpanel_notebook),vbox,image,gtk_label_new(_("charmap")),2);
}

