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
 	{0, new}, 
/* 	{1, open}, */
/* 	{2, save}, */
/* 	{3, save_as}, */
 	{4, close}, 
/* 	{5, copy}, */
/* 	{6, cut}, */
/* 	{7, paste}, */
/* 	{8, search}, */
/*	{9, search_replace}, */
/* 	{10, undo}, */
/* 	{11, redo}, */
/* 	{12, preferences}, */
	{13, quick_start},
	{14, body},
	{15, bold},
	{16, italic},
	{17, underline},
	{19, paragraph},
	{20, brbreak},
	{21, breakall},
	{22, nbsp},
	{23, anchor},
	{24, hrule},
	{25, image},
	{26, thumbnail},
	{27, multithumbs}, /*  multi thumbnail image ?? */
	{28, center},
	{29, right},
	{30, comment},
	{31, email},
	{32, font},
	{33, font_base},
	{34, fontp1},
	{35, fontm1},
	{36, fontpre},
	{37, fontsub},
	{38, fontsuper},
	{39, fonth1},
	{40, fonth2},
	{41, fonth3},
	{42, fonth4},
	{43, fonth5},
	{44, fonth6},
	{45, tablewhiz},
	{46, table},
	{47, table_tr},
	{48, table_th},
	{49, table_td},
	{50, table2},
	{51, table_tr2},
	{52, table_th2},
	{53, table_td2},
	{54, table_capt},
	{55, framewhiz},
	{56, frameset},
	{57, frame},
	{58, frameset2},
	{59, frame2},
	{60, frame_no},
	{61, frame_base},
	{62, form},
	{63, form_submit},
/*	{64, form_reset},*/
	{65, form_text},
	{66, form_hidden},
	{67, form_textarea},
	{68, form_radio},
	{69, form_check},
	{70, form_select},
	{71, form_option},
	{72, form_optiongroup},
	{73, list},
	{74, list_ul},
	{75, list_ol},
	{76, list_li},
	{77, list_dl},
	{78, list_dt},
	{79, list_dd},
/*	{80, list_menu},*/
	{81, meta},
	{82, embed},
	{83, color_select},
	{84, php3}, 
	{85, cssnewstyle},
	{86, cssspan},
	{87, cssdiv},
/*	{88, cssclass},*/
	{89, cssstyle},
	{90, linkstylesheet},
	{91, headings},
	{92, css_small},
	{93, dfn},
	{94, code},
	{95, samp},
	{96, kbd},
	{97, var},
	{98, cite},
	{99, abbr},
	{100, acronym},
	{101, close_small},
	{102, view_in_browser},
	{103, context}, /* context dropdown image */
 	{104, bookmarks},
	{105, filetree},
	{106, fref},
	{107, frinfo},
	{108, frdialog},
	{109, frsearch},
	{110, indent},
	{111, unindent},
	{112, open_small},
	{113, edit_tag},
	{150,prefseditor},
	{151,prefsexternal},
	{152,prefsfiles},
	{153,prefsfiletypes},
	{154,prefshtml},
	{155,prefsimages},
	{156,prefsinterface},
	{157,prefsoutputparser},
	{158,prefssyntax},
	{159, nothing}
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
	retlist = g_list_append(retlist, gdk_pixbuf_new_from_inline(-1,bluefish_icon1,FALSE,NULL));
	return retlist;
}
