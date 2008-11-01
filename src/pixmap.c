/* Bluefish HTML Editor
 * pixmap.c
 *
 * Copyright (C) 2002 Oliver Sessink
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

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "../inline_images/toolbar_icons.c"
#include "../inline_images/general_icons.c"

typedef struct {
	const guint id;
	const guint8 *data;
} Tpixmap;

typedef struct {
	const guint id;
	const gchar *stock_id;
} Tstockpixmap;

static Tpixmap tp[] = {
 	{0, pixmap_new}, 
/* 	{1, open}, */
/* 	{2, save}, */
/* 	{3, save_as}, */
 	{4, pixmap_close}, 
/* 	{5, copy}, */
/* 	{6, cut}, */
/* 	{7, paste}, */
/* 	{8, search}, */
/*	{9, search_replace}, */
/* 	{10, undo}, */
/* 	{11, redo}, */
/* 	{12, preferences}, */
	{13, pixmap_quick_start},
	{14, pixmap_body},
	{15, pixmap_bold},
	{16, pixmap_italic},
	{17, pixmap_underline},
	{19, pixmap_paragraph},
	{20, pixmap_brbreak},
	{21, pixmap_breakall},
	{22, pixmap_nbsp},
	{23, pixmap_anchor},
	{24, pixmap_hrule},
	{25, pixmap_image},
	{26, pixmap_thumbnail},
	{27, pixmap_multithumbs}, /*  multi thumbnail image ?? */
	{28, pixmap_center},
	{29, pixmap_right},
	{30, pixmap_comment},
	{31, pixmap_email},
	{32, pixmap_font},
	{33, pixmap_font_base},
	{34, pixmap_fontp1},
	{35, pixmap_fontm1},
	{36, pixmap_fontpre},
	{37, pixmap_fontsub},
	{38, pixmap_fontsuper},
	{39, pixmap_fonth1},
	{40, pixmap_fonth2},
	{41, pixmap_fonth3},
	{42, pixmap_fonth4},
	{43, pixmap_fonth5},
	{44, pixmap_fonth6},
	{45, pixmap_tablewhiz},
	{46, pixmap_table},
	{47, pixmap_table_tr},
	{48, pixmap_table_th},
	{49, pixmap_table_td},
	{50, pixmap_table2},
	{51, pixmap_table_tr2},
	{52, pixmap_table_th2},
	{53, pixmap_table_td2},
	{54, pixmap_table_capt},
	{55, pixmap_framewhiz},
	{56, pixmap_frameset},
	{57, pixmap_frame},
	{58, pixmap_frameset2},
	{59, pixmap_frame2},
	{60, pixmap_frame_no},
	{61, pixmap_frame_base},
	{62, pixmap_form},
	{63, pixmap_form_submit},
/*	{64, form_reset},*/
	{65, pixmap_form_text},
	{66, pixmap_form_hidden},
	{67, pixmap_form_textarea},
	{68, pixmap_form_radio},
	{69, pixmap_form_check},
	{70, pixmap_form_select},
	{71, pixmap_form_option},
	{72, pixmap_form_optiongroup},
	{73, pixmap_list},
	{74, pixmap_list_ul},
	{75, pixmap_list_ol},
	{76, pixmap_list_li},
	{77, pixmap_list_dl},
	{78, pixmap_list_dt},
	{79, pixmap_list_dd},
/*	{80, list_menu},*/
	{81, pixmap_meta},
	{82, pixmap_embed},
	{83, pixmap_color_select},
	{84, pixmap_php3}, 
	{85, pixmap_cssnewstyle},
	{86, pixmap_cssspan},
	{87, pixmap_cssdiv},
/*	{88, cssclass},*/
	{89, pixmap_cssstyle},
	{90, pixmap_linkstylesheet},
	{91, pixmap_headings},
	{92, pixmap_css_small},
	{93, pixmap_dfn},
	{94, pixmap_code},
	{95, pixmap_samp},
	{96, pixmap_kbd},
	{97, pixmap_var},
	{98, pixmap_cite},
	{99, pixmap_abbr},
	{100, pixmap_acronym},
	{101, pixmap_close_small},
	{102, pixmap_view_in_browser},
	{103, pixmap_context}, /* context dropdown image */
 	{104, pixmap_bookmarks},
	{105, pixmap_filetree},
	{106, pixmap_fref},
	{107, pixmap_frinfo},
	{108, pixmap_frdialog},
	{109, pixmap_frsearch},
	{110, pixmap_indent},
	{111, pixmap_unindent},
	{112, pixmap_open_small},
	{113, pixmap_edit_tag},
	{150,pixmap_prefseditor},
	{151,pixmap_prefsexternal},
	{152,pixmap_prefsfiles},
	{153,pixmap_prefsfiletypes},
	{154,pixmap_prefshtml},
	{155,pixmap_prefsimages},
	{156,pixmap_prefsinterface},
	{157,pixmap_prefsoutputparser},
	{158,pixmap_prefssyntax},
	{159, pixmap_nothing}
};

static Tstockpixmap tsp[] = {
	{1000, GTK_STOCK_FIND},
	{1001, GTK_STOCK_FIND_AND_REPLACE}
};

GtkWidget *new_pixmap(gint type) {
	GtkWidget *widget;
	gint i;
	if (type < 1000) {
		GdkPixbuf *pixbuf;
		for (i=0;i<(sizeof(tp)/sizeof(Tpixmap));i++) {
			if (tp[i].id == type) {
				break;
			}
		}
		if (i >= sizeof(tp)/sizeof(Tpixmap)) {
			g_print("new_pixmap, requested unknown type %d\n", type);
			i = sizeof(tp)/sizeof(Tpixmap)-1;
		}
		pixbuf = gdk_pixbuf_new_from_inline(-1,tp[i].data,FALSE,NULL);
		widget = gtk_image_new_from_pixbuf(pixbuf);
		g_object_unref(pixbuf);
	} else {
		for (i=0;i<(sizeof(tsp)/sizeof(Tstockpixmap));i++) {
			if (tsp[i].id == type) {
				break;
			}
		}
		if (i >= sizeof(tp)/sizeof(Tpixmap)) {
			g_print("new_pixmap, requested unknown type %d\n", type);
			i = sizeof(tp)/sizeof(Tpixmap)-1;
		}
		widget = gtk_image_new_from_stock(tsp[i].stock_id,GTK_ICON_SIZE_BUTTON);
	}
	return widget;
}

GList *default_icon_list() {
	GList *retlist = NULL;
	retlist = g_list_append(retlist, gdk_pixbuf_new_from_inline(-1,pixmap_bluefish_icon1,FALSE,NULL));
	return retlist;
}
