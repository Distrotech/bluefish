/* for the design docs see bftextview2.h */

#include "bftextview2.h"
#include "bftextview2_scanner.h"

static gboolean bftextview2_scanner_idle(gpointer data) {
	Tbftextview2 * bt2 = data;
	if (!bftextview2_run_scanner(bt2)) {
		bt2->scanner_idle = 0;
		return FALSE;
	}
	return TRUE;
}

static void bftextview2_insert_text_lcb(GtkTextBuffer * buffer, GtkTextIter * iter,
										gchar * string, gint stringlen, Tbftextview2 * bt2)
{
	GtkTextIter start;
	g_print("bftextview2_insert_text_lcb, stringlen=%d\n",stringlen);
	if (bt2->scanner_idle == 0) {
		bt2->scanner_idle = g_idle_add(bftextview2_scanner_idle,bt2);
	}
	
	/* mark the text that is changed */
	start = *iter;
	gtk_text_iter_backward_chars(&start,stringlen);
	/* always scan the whole line */
	gtk_text_iter_set_line_offset(&start, 0);
	gtk_text_iter_forward_to_line_end(iter);
	gtk_text_buffer_apply_tag_by_name(buffer,"needscanning",&start,iter);
	g_print("mark text from %d to %d as needscanning\n",gtk_text_iter_get_offset(&start),gtk_text_iter_get_offset(iter));
	
}

/* widget stuff below */


static GtkTextViewClass *parent_class = NULL;

static void bftextview2_class_init(Tbftextview2Class * c)
{
	parent_class = g_type_class_ref(GTK_TYPE_TEXT_VIEW);
}

static void bftextview2_init(Tbftextview2 * bt2)
{
	bt2->timer = g_timer_new();
	bt2->scancache.stackcaches = g_sequence_new(NULL);

}

GType bftextview2_get_type(void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo info = {
			sizeof(Tbftextview2Class),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) bftextview2_class_init,
			(GClassFinalizeFunc) NULL,
			NULL /* class_data */ ,
			sizeof(Tbftextview2),
			0 /* n_preallocs */ ,
			(GInstanceInitFunc) bftextview2_init,
			NULL
		};
		type = g_type_register_static(GTK_TYPE_TEXT_VIEW, "bftextview2", &info, (GTypeFlags) 0);
	}
	return type;
}

GtkWidget *bftextview2_new(void)
{
	PangoFontDescription *font_desc;
	Tbftextview2 *o = ((Tbftextview2 *) g_object_new(bftextview2_get_type(), NULL));
	
	/*g_signal_connect(G_OBJECT(o), "expose-event", G_CALLBACK(bftextview2_expose_lcb), NULL);*/
	g_signal_connect_after(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "insert-text",
						   G_CALLBACK(bftextview2_insert_text_lcb), o);
	font_desc = pango_font_description_from_string("Courier 11");
	gtk_widget_modify_font(o, font_desc);
	pango_font_description_free(font_desc);

	return (GtkWidget *) o;
}

GtkWidget *bftextview2_new_with_buffer(GtkTextBuffer * buffer)
{
	Tbftextview2 *o = bftextview2_new();
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(o), buffer);
	g_signal_connect_after(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "insert-text",
						   G_CALLBACK(bftextview2_insert_text_lcb), o);
	return (GtkWidget *) o;
}
