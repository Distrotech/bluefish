#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "bluefish.h"

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
	{101, pixmap_close_small},
	{102, pixmap_view_in_browser},
 	{104, pixmap_bookmarks},
	{105, pixmap_filetree},
	{106, pixmap_fref},
	{107, pixmap_frinfo},
	{108, pixmap_frdialog},
	{109, pixmap_frsearch},
	{110, pixmap_indent},
	{111, pixmap_unindent},
	{112, pixmap_open_small},
	{150,pixmap_prefseditor},
	{151,pixmap_prefsexternal},
	{152,pixmap_prefsfiles},
	{153,pixmap_prefsfiletypes},
	{154,pixmap_prefshtml},
	{155,pixmap_prefsimages},
	{156,pixmap_prefsinterface},
	{157,pixmap_prefsoutputparser},
	{158,pixmap_prefssyntax},
	{159, pixmap_nothing},
	{160, pixmap_frhide},
	{161, pixmap_frdialog2},
	{162, pixmap_frlibrary},
	{170, pixmap_frlibrary_ro},	
	{163, pixmap_frgroup},
	{164, pixmap_frnote},
	{165, pixmap_frfunction},
	{166, pixmap_frtag},
	{167, pixmap_frvar},
	{168, pixmap_frcss},
	{169, pixmap_frsnippet}
};

static Tstockpixmap tsp[] = {
	{1000, GTK_STOCK_FIND},
	{1001, GTK_STOCK_FIND_AND_REPLACE},
	{1002, GTK_STOCK_PREFERENCES},
	{1003, GTK_STOCK_GO_BACK},
	{1004,GTK_STOCK_GO_FORWARD}	
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

void set_default_icon(void) {
	gtk_window_set_default_icon_list(g_list_append(NULL, gdk_pixbuf_new_from_inline(-1,pixmap_bluefish_icon1,FALSE,NULL)));
}
