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
	case 205:
		data = close_small;
	break;
	default:
		g_print("new_pixmap, unknown pixmap type requested\n");
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
