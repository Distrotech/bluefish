/* for the design docs see bftextview2.h */

#include <gtk-2.0/gdk/gdkevents.h>
#include <gtk-2.0/gtk/gtkwidget.h>

#include "bftextview2.h"
#include "bftextview2_scanner.h"
#include "bftextview2_patcompile.h"
#include "bftextview2_autocomp.h"


G_DEFINE_TYPE(BluefishTextView, bluefish_text_view, GTK_TYPE_TEXT_VIEW)

static gboolean bftextview2_user_idle_timer(gpointer data)
{
	BluefishTextView *bt2 = data;
	guint elapsed = (guint) (1000.0 * g_timer_elapsed(bt2->user_idle_timer, NULL));
	if (elapsed + 10 >= USER_IDLE_EVENT_INTERVAL) {	/* avoid delaying for less than 10 milliseconds */
		DBG_MSG("bftextview2_user_idle_timer, user is > %d milliseconds idle!!!\n", elapsed);
		autocomp_run(bt2);
		bt2->user_idle = 0;
		return FALSE;
	} else {
		guint nextcheck;
		nextcheck = USER_IDLE_EVENT_INTERVAL - elapsed;
		DBG_MSG
			("bftextview2_user_idle_timer, not yet elapsed (%d milliseconds idle), rechecking in %d milliseconds\n",
			 elapsed, nextcheck);
		bt2->user_idle = g_timeout_add(nextcheck, bftextview2_user_idle_timer, bt2);
		return FALSE;
	}
}
static void bftextview2_reset_user_idle_timer(BluefishTextView * bt2)
{
	g_timer_start(bt2->user_idle_timer);
	if (bt2->user_idle == 0) {
		bt2->user_idle = g_timeout_add(USER_IDLE_EVENT_INTERVAL, bftextview2_user_idle_timer, bt2);
		DBG_MSG("started user_idle timeout with event source %d and timeout %d\n", bt2->user_idle,
				USER_IDLE_EVENT_INTERVAL);
	}
}

static gboolean bftextview2_scanner_idle(gpointer data)
{
	BluefishTextView *bt2 = data;
	if (!bftextview2_run_scanner(bt2)) {
		bt2->scanner_idle = 0;
		return FALSE;
	}
	return TRUE;
}

static void bftextview2_insert_text_lcb(GtkTextBuffer * buffer, GtkTextIter * iter, gchar * string,
										gint stringlen, BluefishTextView * bt2)
{
	GtkTextIter start;
	gint start_offset;
	DBG_MSG("bftextview2_insert_text_lcb, stringlen=%d\n", stringlen);
	if (bt2->scanner_idle == 0) {
		bt2->scanner_idle = g_idle_add(bftextview2_scanner_idle, bt2);
	}

	/* mark the text that is changed */
	start = *iter;
	gtk_text_iter_backward_chars(&start, stringlen);

	gtk_text_buffer_apply_tag_by_name(buffer, "needscanning", &start, iter);
	DBG_MSG("mark text from %d to %d as needscanning\n", gtk_text_iter_get_offset(&start),
			gtk_text_iter_get_offset(iter));
	start_offset = gtk_text_iter_get_offset(&start);
	if (bt2->scancache.stackcache_need_update_charoffset == -1
		|| bt2->scancache.stackcache_need_update_charoffset > start_offset) {
		bt2->scancache.stackcache_need_update_charoffset = start_offset;
	}
	bftextview2_reset_user_idle_timer(bt2);
}

static void bftextview2_get_iters_at_foundblock(GtkTextBuffer * buffer, Tfoundblock * fblock,
												GtkTextIter * it1, GtkTextIter * it2,
												GtkTextIter * it3, GtkTextIter * it4)
{
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

static void bftextview2_mark_set_lcb(GtkTextBuffer * buffer, GtkTextIter * location,
									 GtkTextMark * arg2, gpointer widget)
{
	if (arg2 && gtk_text_buffer_get_insert(buffer) == arg2) {
		GtkTextIter it1, it2;
		Tfoundblock *fblock = bftextview2_get_block_at_iter(location);
		gtk_text_buffer_get_bounds(buffer, &it1, &it2);
		gtk_text_buffer_remove_tag_by_name(buffer, "blockmatch", &it1, &it2);
		if (fblock) {
			GtkTextIter it3, it4;
			if (fblock->start2) {
				DBG_MSG("found a block to highlight the start and end\n");
				bftextview2_get_iters_at_foundblock(buffer, fblock, &it1, &it2, &it3, &it4);
				gtk_text_buffer_apply_tag_by_name(buffer, "blockmatch", &it1, &it2);
				gtk_text_buffer_apply_tag_by_name(buffer, "blockmatch", &it3, &it4);
			} else {
				DBG_MSG("block has no end - no matching\n");
			}
		}
	}
	bftextview2_reset_user_idle_timer(BLUEFISH_TEXT_VIEW(widget));
}

static void bftextview2_set_margin_size(BluefishTextView * bt2)
{
	/* TODO: this should be calculated based on the number of lines in the text, 
	   whether or not we have bookmarks, and whether or not we have block folding */
	gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(bt2), GTK_TEXT_WINDOW_LEFT, 35);
}

static void print_fstack(Tfoundstack * fstack)
{
	DBG_MSG("got fstack %p for next position", fstack);
	if (fstack)
		DBG_MSG(" with line %d and charoffset %d and %d blocks", fstack->line, fstack->charoffset,
				g_queue_get_length(fstack->blockstack));
	DBG_MSG("\n");
}

static void paint_margin(BluefishTextView *btv,GdkEventExpose * event, GtkTextIter * startvisible, GtkTextIter * endvisible)
{
	Tfoundstack *fstack = NULL;
	GSequenceIter *siter;
	guint num_blocks;
	GtkTextIter it;
	gint i;
	PangoLayout *panlay;

	/* to see how many blocks are active here */
	if (gtk_text_iter_is_start(startvisible)) {
		siter = g_sequence_get_begin_iter(btv->scancache.stackcaches);
		if (!g_sequence_iter_is_end(siter)) {
			fstack = g_sequence_get(siter);
		}
		num_blocks = 0;
		DBG_MSG("EXPOSE: start at begin, set num_blocks %d, fstack=%p\n", num_blocks, fstack);
	} else {
		fstack = get_stackcache_at_position(btv, startvisible, &siter);
		if (fstack) {
			num_blocks = g_queue_get_length(fstack->blockstack);
			DBG_MSG
				("EXPOSE: got fstack %p with line %d and charoffset %d and num_blocks %d for start position %d\n",
				 fstack, fstack->line, fstack->charoffset, num_blocks,
				 gtk_text_iter_get_offset(startvisible));
		} else {
			num_blocks = 0;
		}
	}
	/* in the case that the *first* fstack is relevant, we don't need 
	   the 'next' fstack */
	if (!fstack || fstack->charoffset < gtk_text_iter_get_offset(startvisible)) {
		DBG_MSG("get next fstack..\n");
		fstack = get_stackcache_next(btv, &siter);
	}
	DBG_MSG("first fstack ");
	print_fstack(fstack);


	it = *startvisible;
	panlay = gtk_widget_create_pango_layout(GTK_WIDGET(btv), "x");
	for (i = gtk_text_iter_get_line(startvisible); i <= gtk_text_iter_get_line(endvisible); i++) {
		gint w, height;
		gchar *string;

		gtk_text_iter_set_line(&it, i);
		gtk_text_view_get_line_yrange(GTK_TEXT_VIEW(btv), &it, &w, &height);
		gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(btv), GTK_TEXT_WINDOW_LEFT, 0, w,
											  NULL, &w);

		/* line numbers */
		string = g_strdup_printf("%d", 1 + i);
		pango_layout_set_markup(panlay, string, -1);
		gtk_paint_layout(GTK_WIDGET(btv)->style, event->window, GTK_WIDGET_STATE(btv), FALSE, NULL,
						 GTK_WIDGET(btv), NULL, 2, w, panlay);
		g_free(string);

		/* block folding */
		if (fstack && fstack->line == i) {	/* the Tfoundstack is on this line, expand or collapse ? */
			gint tmp = g_queue_get_length(fstack->blockstack);
			if (tmp > num_blocks) {
				DBG_MSG
					("fstack num_blocks=%d, old num_blocks=%d, expander (box from x 21, y %d) on line %d (for the user line %d)\n",
					 tmp, num_blocks, w + (height / 2 - 4), i, i + 1);
				gtk_paint_box(GTK_WIDGET(btv)->style, event->window, GTK_WIDGET_STATE(btv),
							  GTK_SHADOW_OUT, NULL, GTK_WIDGET(btv), NULL, 21, w + (height / 2 - 4), 9, 9);
				gtk_paint_hline(GTK_WIDGET(btv)->style, event->window, GTK_WIDGET_STATE(btv), NULL,
								GTK_WIDGET(btv), NULL, 23, 28, w + (height / 2));
				gtk_paint_vline(GTK_WIDGET(btv)->style, event->window, GTK_WIDGET_STATE(btv), NULL,
								GTK_WIDGET(btv), NULL, w + (height / 2) + 4, w + height, 25);

				/*gtk_paint_box(GTK_WIDGET(btv)->style,event->window,GTK_WIDGET_STATE(btv),GTK_SHADOW_NONE,
				   NULL,btv,"bg",20,w,8,8); */
				num_blocks = g_queue_get_length(fstack->blockstack);
			} else if (tmp < num_blocks) {
				DBG_MSG("end of block\n");
				gtk_paint_vline(GTK_WIDGET(btv)->style, event->window, GTK_WIDGET_STATE(btv), NULL,
								GTK_WIDGET(btv), NULL, w, w + (height / 2), 25);
				gtk_paint_hline(GTK_WIDGET(btv)->style, event->window, GTK_WIDGET_STATE(btv), NULL,
								GTK_WIDGET(btv), NULL, 25, 29, w + (height / 2));
				num_blocks = g_queue_get_length(fstack->blockstack);
			} else {
				DBG_MSG("no blockstack change, fstack has %d, num_blocks=%d, draw line\n", tmp,
						num_blocks);
				gtk_paint_vline(GTK_WIDGET(btv)->style, event->window, GTK_WIDGET_STATE(btv), NULL,
								GTK_WIDGET(btv), NULL, w, w + height, 25);
			}
			do {
				fstack = get_stackcache_next(btv, &siter);
				print_fstack(fstack);
			} while (fstack && fstack->line == i);
		} else {				/* not on this line, draw line  or nothing ? */
			if (num_blocks > 0) {
				DBG_MSG("draw line (line from x 25 y %d to %d) on line %d (for the user line %d)\n",
						w, w + height, i, i + 1);
				/* draw line */
				gtk_paint_vline(GTK_WIDGET(btv)->style, event->window, GTK_WIDGET_STATE(btv), NULL,
								GTK_WIDGET(btv), NULL, w, w + height, 25);
			}
		}
	}
	g_object_unref(G_OBJECT(panlay));
}

static gboolean bftextview2_expose_event_lcb(GtkWidget * widget, GdkEventExpose * event,
											 gpointer data)
{
	BluefishTextView *btv = data;
	if (event->window == gtk_text_view_get_window(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_LEFT)) {
		GtkTextIter startvisible, endvisible;
		GdkRectangle rect;

		gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(widget), &rect);
		gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(widget), &startvisible, rect.y, NULL);
		gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(widget), &endvisible, rect.y + rect.height, NULL);

		paint_margin(btv,event, &startvisible, &endvisible);
		return TRUE;
	}
	return FALSE;
}
static void bftextview2_delete_range_lcb(GtkTextBuffer * textbuffer, GtkTextIter * begin,
										 GtkTextIter * end, gpointer user_data)
{
	DBG_MSG("bftextview2_delete_range_lcb\n");
}

static void bftextview2_delete_range_after_lcb(GtkTextBuffer * textbuffer, GtkTextIter * begin,
											   GtkTextIter * end, gpointer user_data)
{
	DBG_MSG("bftextview2_delete_range_after_lcb\n");
}

/* *************************************************************** */
/* widget stuff below */
/* *************************************************************** */

static void bluefish_text_view_finalize(GObject * object)
{
	G_OBJECT_CLASS(bluefish_text_view_parent_class)->finalize(object);
}

static GObject *bluefish_text_view_create(GType type, guint n_construct_properties,
										  GObjectConstructParam * construct_properties)
{
	BluefishTextViewClass *klass =
		BLUEFISH_TEXT_VIEW_CLASS(g_type_class_peek(BLUEFISH_TYPE_TEXT_VIEW));
	GObjectClass *parent_class = G_OBJECT_CLASS(g_type_class_peek_parent(klass));
	GObject *obj = parent_class->constructor(type,
											 n_construct_properties,
											 construct_properties);

	/* This constructor is not needed right now */

	return (obj);
}

static void bluefish_text_view_class_init(BluefishTextViewClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->constructor = bluefish_text_view_create;
	object_class->finalize = bluefish_text_view_finalize;
}

static void bluefish_text_view_init(BluefishTextView * textview)
{
	PangoFontDescription *font_desc;

	textview->user_idle_timer = g_timer_new();
	textview->scancache.stackcaches = g_sequence_new(NULL);

	font_desc = pango_font_description_from_string("Monospace 10");
	gtk_widget_modify_font(GTK_WIDGET(textview), font_desc);
	pango_font_description_free(font_desc);
	bftextview2_set_margin_size(textview);
}

GtkWidget *bftextview2_new(void)
{
	BluefishTextView *textview = (BluefishTextView *) g_object_new(BLUEFISH_TYPE_TEXT_VIEW, NULL);

	g_return_val_if_fail(textview != NULL, NULL);

	return GTK_WIDGET(textview);
}

GtkWidget *bftextview2_new_with_buffer(GtkTextBuffer * buffer)
{
	BluefishTextView *textview = (BluefishTextView *) bftextview2_new();

	g_return_val_if_fail(textview != NULL, NULL);
	/* If for some reason we cannot create the text view, we return NULL.
	 * We don't want to call *set_buffer() on a NULL value.
	 */

	gtk_text_view_set_buffer(GTK_TEXT_VIEW(textview), buffer);

	g_signal_connect_after(G_OBJECT(buffer), "insert-text", G_CALLBACK(bftextview2_insert_text_lcb),
						   textview);
	g_signal_connect_after(G_OBJECT(buffer), "mark-set", G_CALLBACK(bftextview2_mark_set_lcb),
						   textview);
	g_signal_connect(G_OBJECT(textview), "expose-event", G_CALLBACK(bftextview2_expose_event_lcb),
					 textview);
	g_signal_connect(G_OBJECT(buffer), "delete-range", G_CALLBACK(bftextview2_delete_range_lcb),
					 textview);
	g_signal_connect_after(G_OBJECT(buffer), "delete-range",
						   G_CALLBACK(bftextview2_delete_range_after_lcb), textview);
	return GTK_WIDGET(textview);
}
