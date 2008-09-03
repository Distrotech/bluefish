/* for the design docs see bftextview2.h */

#include "bftextview2.h"
#include "bftextview2_scanner.h"
#include "bftextview2_patcompile.h"
#include "bftextview2_autocomp.h"

static gboolean bftextview2_user_idle_timer(gpointer data) {
	Tbftextview2 * bt2 = data;
	guint elapsed = (guint)(1000.0 * g_timer_elapsed(bt2->user_idle_timer,NULL));
	if (elapsed >= USER_IDLE_EVENT_INTERVAL) {
		g_print("bftextview2_user_idle_timer, user is > %d milliseconds idle!!!\n",elapsed);
		autocomp_run(bt2);
		bt2->user_idle = 0;
		return FALSE;
	} else {
		guint nextcheck;
		nextcheck = USER_IDLE_EVENT_INTERVAL - elapsed;
		g_print("bftextview2_user_idle_timer, not yet elapsed (%d milliseconds idle), rechecking in %d milliseconds\n",elapsed, nextcheck);  
		bt2->user_idle = g_timeout_add(nextcheck, bftextview2_user_idle_timer, bt2);
		return FALSE;
	}
}
static void bftextview2_reset_user_idle_timer(Tbftextview2 * bt2) {
	g_timer_start(bt2->user_idle_timer);
	if (bt2->user_idle == 0) {
		bt2->user_idle = g_timeout_add(USER_IDLE_EVENT_INTERVAL, bftextview2_user_idle_timer, bt2);
		g_print("started user_idle timeout with event source %d and timeout %d\n",bt2->user_idle,USER_IDLE_EVENT_INTERVAL);
	}
}

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
	bftextview2_reset_user_idle_timer(bt2);
}

static void bftextview2_get_iters_at_foundblock(GtkTextBuffer *buffer, Tfoundblock *fblock
	,GtkTextIter *it1, GtkTextIter *it2, GtkTextIter *it3, GtkTextIter *it4) {
	gtk_text_buffer_get_iter_at_mark(buffer, it1, fblock->start1);
	gtk_text_buffer_get_iter_at_mark(buffer, it2, fblock->end1);
	gtk_text_buffer_get_iter_at_mark(buffer, it3, fblock->start2);
	gtk_text_buffer_get_iter_at_mark(buffer, it4, fblock->end2);
}

static Tfoundblock *bftextview2_get_block_at_iter(GtkTextIter * it)
{
	GSList *tmp, *lst = gtk_text_iter_get_marks(it);
	gpointer ptr = NULL;
	tmp = lst;

	while (tmp) {
		ptr = g_object_get_data(G_OBJECT(tmp->data), "block");
		if (ptr) {
			g_slist_free(lst);
			return ptr;
		}
		tmp = g_slist_next(tmp);
	}
	g_slist_free(lst);
	return NULL;
}

static void bftextview2_mark_set_lcb(GtkTextBuffer *buffer, GtkTextIter *location, GtkTextMark * arg2,
									gpointer widget) {
	if (arg2 && gtk_text_buffer_get_insert(buffer) == arg2) {
		GtkTextIter it1,it2;
		Tfoundblock *fblock = bftextview2_get_block_at_iter(location);
		gtk_text_buffer_get_bounds(buffer,&it1,&it2);
		gtk_text_buffer_remove_tag_by_name(buffer, "blockmatch", &it1,&it2);
		if (fblock) {
			GtkTextIter it3,it4;
			if (fblock->start2) {
				g_print("found a block to highlight the start and end\n");
				bftextview2_get_iters_at_foundblock(buffer, fblock, &it1, &it2, &it3, &it4);
				gtk_text_buffer_apply_tag_by_name(buffer, "blockmatch", &it1,&it2);
				gtk_text_buffer_apply_tag_by_name(buffer, "blockmatch", &it3,&it4);
			} else {
				g_print("block has no end - no matching\n");
			}
		}
	}
	bftextview2_reset_user_idle_timer((Tbftextview2 *)widget);
}

/* *************************************************************** */
/* widget stuff below */
/* *************************************************************** */
static GtkTextViewClass *parent_class = NULL;

static void bftextview2_class_init(Tbftextview2Class * c)
{
	parent_class = g_type_class_ref(GTK_TYPE_TEXT_VIEW);
}

static void bftextview2_init(Tbftextview2 * bt2)
{
	bt2->user_idle_timer = g_timer_new();
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
	g_signal_connect_after(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "mark-set",
							   G_CALLBACK(bftextview2_mark_set_lcb), o);
	font_desc = pango_font_description_from_string("Monospace 10");
	gtk_widget_modify_font(GTK_WIDGET(o), font_desc);
	pango_font_description_free(font_desc);

	return (GtkWidget *) o;
}

GtkWidget *bftextview2_new_with_buffer(GtkTextBuffer * buffer)
{
	Tbftextview2 *o = (Tbftextview2 *)bftextview2_new();
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(o), buffer);
	g_signal_connect_after(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "insert-text",
						   G_CALLBACK(bftextview2_insert_text_lcb), o);
	g_signal_connect_after(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "mark-set",
							   G_CALLBACK(bftextview2_mark_set_lcb), o);
	return (GtkWidget *) o;
}
