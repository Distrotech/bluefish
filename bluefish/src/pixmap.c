#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "../inline_images/toolbar_icons.c"
#include "../inline_images/general_icons.c"

GtkWidget *new_pixmap(gint type) {
	const guint8 *data;

	switch (type) {
	case 000:
		data = new;
	break;
	case 001:
		data = open;
	break;
	case 002:
		data = save;
	break;
	case 003:
		data = save_as;
	break;
	case 004:
		data = close;
	break;
	case 005:
		data = copy;
	break;
	case 006:
		data = cut;
	break;
	case 007:
		data = paste;
	break;
	case 010:
		data = search;
		break;
	case 011:
		data = search_replace;
		break;
	case 012:
		data = undo;
		break;
	case 013:
		data = redo;
		break;
	case 100:
		data = quick_start;
		break;
	case 101:
		data = body;
		break;
	case 102:
		data = bold;
		break;
	case 103:
		data = italic;
		break;
	case 104:
		data = underline;
		break;
	case 105:
		data = strikeout;
		break;
	case 106:
		data = paragraph;
		break;
	case 107:
		data = brbreak;
		break;
	case 108:
		data = breakall;
		break;
	case 110:
		data = nbsp;
		break;
	case 111:
		data = anchor;
		break;
	case 112:
		data = hrule;
		break;
	case 113:
		data = image;
		break;
	case 114:
		data = center;
		break;
	case 115:
		data = right;
		break;
	case 116:
		data = comment;
		break;
	case 117:
		data = email;
		break;
	case 118:
		data = font;
		break;
	case 119:
		data = font_base;
		break;
	case 120:
		data = fontp1;
		break;
	case 121:
		data = fontm1;
		break;
	case 122:
		data = fontpre;
		break;
	case 123:
		data = fontsub;
		break;
	case 124:
		data = fontsuper;
		break;
	case 125:
		data = thumbnail;
		break;
	case 127:
		data = fonth1;
		break;
	case 128:
		data = fonth2;
		break;
	case 129:
		data = fonth3;
		break;
	case 130:
		data = fonth4;
		break;
	case 131:
		data = fonth5;
		break;
	case 132:
		data = fonth6;
		break;
	case 133:
		data = tablewhiz;
		break;
	case 134:
		data = table;
		break;
	case 135:
		data= table_tr;
		break;
	case 136:
		data = table_th;
		break;
	case 137:
		data = table_td;
		break;
	case 138:
		data = table2;
		break;
	case 139:
		data = table_tr2;
		break;
	case 140:
		data = table_th2;
		break;
	case 141:
		data = table_td2;
		break;
	case 142:
		data = table_capt;
		break;
/*	case 176: replaced by stock button
		data = color_select_small;
		break;*/
	case 192:
		data = css_small;
		break;
	case 205:
		data = close_small;
		break;
	default:
		g_print("new_pixmap, unknown pixmap type %d requested\n", type);
		exit(1);
		break;
	}
	{
		GdkPixbuf *pixbuf;
		GtkWidget *widget;

		pixbuf = gdk_pixbuf_new_from_inline(-1,data,FALSE,NULL);
		widget = gtk_image_new_from_pixbuf(pixbuf);
		g_object_unref(pixbuf);
		return widget;
	}
}
