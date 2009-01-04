/* Bluefish HTML Editor
 * bftextview2.c
 *
 * Copyright (C) 2008-2009 Olivier Sessink
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

/* for the design docs see bftextview2.h */

#include <gdk/gdkkeysyms.h>
#include <math.h> /* log10() */
#include <string.h> /* strlen() */

#include "bftextview2.h"

#ifdef IN_BLUEFISH
#include "bluefish.h"
#include "bookmark.h"
#else
#include "testapp.h"
#endif


#include "bftextview2_scanner.h"
#include "bftextview2_patcompile.h"
#include "bftextview2_autocomp.h"
#include "bftextview2_langmgr.h"

#define USER_IDLE_EVENT_INTERVAL 480 /* milliseconds */

G_DEFINE_TYPE(BluefishTextView, bluefish_text_view, GTK_TYPE_TEXT_VIEW)

static gboolean bftextview2_user_idle_timer(gpointer data)
{
	BluefishTextView *btv = data;
	guint elapsed = (guint) (1000.0 * g_timer_elapsed(btv->user_idle_timer, NULL));
	if ((elapsed + 20) >= USER_IDLE_EVENT_INTERVAL) {	/* avoid delaying again for less than 20 milliseconds */
		DBG_AUTOCOMP("bftextview2_user_idle_timer, user is > %d milliseconds idle, autocomp=%d, mode=%d\n", elapsed,btv->autocomplete, main_v->props.autocomp_popup_mode);
#ifdef IN_BLUEFISH
		if (btv->autocomplete && main_v->props.autocomp_popup_mode == 1)
#else
		if (btv->autocomplete && autocomp_popup_mode == 1)
#endif
			autocomp_run(btv,FALSE);
		btv->user_idle = 0;
		return FALSE;
	} else {
		guint nextcheck;
		nextcheck = USER_IDLE_EVENT_INTERVAL - elapsed;
		DBG_MSG
			("bftextview2_user_idle_timer, not yet elapsed (%d milliseconds idle), rechecking in %d milliseconds\n",
			 elapsed, nextcheck);
		btv->user_idle = g_timeout_add(nextcheck, bftextview2_user_idle_timer, btv);
		return FALSE;
	}
}

static void bftextview2_reset_user_idle_timer(BluefishTextView * btv)
{
	gboolean need_timeout_func;
	DBG_DELAYSCANNING("timer reset\n");
	g_timer_start(btv->user_idle_timer);
#ifdef IN_BLUEFISH
	need_timeout_func = ((main_v->props.autocomp_popup_mode==1&&btv->autocomp)||main_v->props.delay_full_scan);
#else
	need_timeout_func = ((autocomp_popup_mode==1&&btv->autocomp)||delay_full_scan);
#endif
	if (btv->user_idle == 0 && need_timeout_func) {
		btv->user_idle = g_timeout_add(USER_IDLE_EVENT_INTERVAL, bftextview2_user_idle_timer, btv);
		DBG_MSG("started user_idle timeout with event source %d and timeout %d\n", btv->user_idle,
				USER_IDLE_EVENT_INTERVAL);
	}
}

static void bftextview2_set_margin_size(BluefishTextView * btv);
static gboolean bftextview2_scanner_idle(gpointer data);
static gboolean bftextview2_scanner_timeout(gpointer data);

static gboolean bftextview2_scanner_scan(BluefishTextView *btv, gboolean in_idle)
{
	if (btv->bflang) {
#ifdef IN_BLUEFISH
		if (main_v->props.delay_full_scan)
#else
		if (delay_full_scan)
#endif
		 {
			guint elapsed = (guint) (1000.0 * g_timer_elapsed(btv->user_idle_timer, NULL));
			DBG_DELAYSCANNING("%d milliseconds elapsed since last user action\n",elapsed);
			if ((elapsed + 20) >= main_v->props.delay_scan_time) { /* delay scan time has passed ! */
				DBG_DELAYSCANNING("idle, call scan for everything\n");
				if (!bftextview2_run_scanner(btv, NULL)) {
					/* finished scanning, make sure we are not called again */
					DBG_DELAYSCANNING("finished scanning (idle=%d)\n",in_idle);
					if (in_idle && btv->scanner_delayed) {
						g_source_remove(btv->scanner_delayed);
					} else if (!in_idle && btv->scanner_idle) {
						g_source_remove(btv->scanner_idle);
					}
					btv->scanner_delayed = 0;
					btv->scanner_idle = 0;
					bftextview2_set_margin_size(btv);
					return FALSE;
				}
				if (!in_idle) {
					/* don't call timeout function again (run in idle) */
					if (!btv->scanner_idle) {
						DBG_DELAYSCANNING("schedule scan again in idle time\n");
						btv->scanner_idle = g_idle_add(bftextview2_scanner_idle, btv);
					} else {
						DBG_DELAYSCANNING("scan in idle is already scheduled\n");
					}
					btv->scanner_delayed = 0;
					return FALSE; 
				}
				return TRUE; /* call idle function again */
			} else {
				/* user has not been idle, only scan visible area */
				GtkTextIter endvisible;
				GdkRectangle rect;
				/* get visible area and only scan the visible area */
				gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(btv), &rect);
				gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(btv), &endvisible, rect.y + rect.height, NULL);
				DBG_DELAYSCANNING("not idle, call scan for visible area\n");
				if (!bftextview2_run_scanner(btv, &endvisible)) {
					DBG_DELAYSCANNING("finished scanning\n");
					if (in_idle && btv->scanner_delayed) {
						g_source_remove(btv->scanner_delayed);
					} else if (!in_idle && btv->scanner_idle) {
						g_source_remove(btv->scanner_idle);
					}
					btv->scanner_delayed = 0;
					btv->scanner_idle = 0;
					bftextview2_set_margin_size(btv);
					return FALSE;
				}
				if (in_idle) {
					/* don't call idle function again, but call timeout function */
					if (!btv->scanner_delayed) {
						DBG_DELAYSCANNING("schedule delayed scanning\n");
						btv->scanner_delayed = g_timeout_add(main_v->props.delay_scan_time, bftextview2_scanner_timeout, btv);
					} else {
						DBG_DELAYSCANNING("delayed scanning already scheduled\n");
					}
					btv->scanner_idle = 0;
					return FALSE;
				}
				return TRUE;/* call timeout function again */
			}
		} else {
			DBG_SIGNALS("bftextview2_scanner_idle, running scanner idle function\n");
			if (!bftextview2_run_scanner(btv, NULL)) {
				btv->scanner_idle = 0;
				DBG_SIGNALS("bftextview2_scanner_idle, stopping scanner idle function\n");
				bftextview2_set_margin_size(btv);
				return FALSE;
			}
		}
	}
	return TRUE;
}

static gboolean bftextview2_scanner_idle(gpointer data) {
	DBG_DELAYSCANNING("bftextview2_scanner_idle\n");
	return bftextview2_scanner_scan((BluefishTextView *)data, TRUE);
}
static gboolean bftextview2_scanner_timeout(gpointer data) {
	DBG_DELAYSCANNING("bftextview2_scanner_timeout\n");
	return bftextview2_scanner_scan((BluefishTextView *)data, FALSE);
}

void bftextview2_schedule_scanning(BluefishTextView * btv) {
	if (btv->enable_scanner && btv->bflang && btv->bflang->st && btv->scanner_idle == 0) {
		DBG_MSG("bftextview2_schedule_scanning, scheduling scanning function\n");
		DBG_DELAYSCANNING("scheduling scanning in idle function\n");
		/* G_PRIORITY_LOW IS 300 and G_PRIORITY_DEFAULT_IDLE is 200 */
		btv->scanner_idle = g_idle_add_full(250,bftextview2_scanner_idle, btv,NULL);
	}
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

	if (BLUEFISH_TEXT_VIEW(widget)->bflang && BLUEFISH_TEXT_VIEW(widget)->bflang->st && arg2 && gtk_text_buffer_get_insert(buffer) == arg2) {
		GtkTextIter it1, it2;
		Tfoundblock *fblock = bftextview2_get_block_at_iter(location);
		gtk_text_buffer_get_bounds(buffer, &it1, &it2);
		DBG_SIGNALS("bftextview2_mark_set_lcb, 'insert' set at %d\n",gtk_text_iter_get_offset(location));
		gtk_text_buffer_remove_tag(buffer, BLUEFISH_TEXT_VIEW(widget)->blockmatch, &it1, &it2);
		if (fblock) {
			GtkTextIter it3, it4;
			if (fblock->start2) {
				bftextview2_get_iters_at_foundblock(buffer, fblock, &it1, &it2, &it3, &it4);
				DBG_MSG("found a block to highlight the start (%d:%d) and end (%d:%d)\n",gtk_text_iter_get_offset(&it1),gtk_text_iter_get_offset(&it2),gtk_text_iter_get_offset(&it3),gtk_text_iter_get_offset(&it4));
				gtk_text_buffer_apply_tag(buffer, BLUEFISH_TEXT_VIEW(widget)->blockmatch, &it1, &it2);
				gtk_text_buffer_apply_tag(buffer, BLUEFISH_TEXT_VIEW(widget)->blockmatch, &it3, &it4);
			} else {
				DBG_MSG("block has no end - no matching\n");
			}
		}
		
		if (BLUEFISH_TEXT_VIEW(widget)->autocomp) {
			autocomp_stop(BLUEFISH_TEXT_VIEW(widget));
		}
		bftextview2_reset_user_idle_timer(BLUEFISH_TEXT_VIEW(widget));
	}
}

static void bftextview2_set_margin_size(BluefishTextView * btv)
{
	/* TODO: this should be calculated based on the number of lines in the text,
	   whether or not we have bookmarks, and whether or not we have block folding */
	gint lines,count;
	if (btv->linenumbers) {
		PangoLayout *panlay;
		lines = gtk_text_buffer_get_line_count(gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv)));
		if (btv->margin_pixels_per_char==0) {
			panlay = gtk_widget_create_pango_layout(GTK_WIDGET(btv), "");
			pango_layout_set_text(panlay, "4", -1);
			pango_layout_get_pixel_size(panlay, &btv->margin_pixels_per_char, NULL);
			g_object_unref(G_OBJECT(panlay));
		}
		if (lines >= 100)
			count = 1+log10(lines);
		else
			count=2;
		btv->margin_pixels_chars = 4 + count * btv->margin_pixels_per_char;
	} else {
		btv->margin_pixels_chars = 0;
	}
	if (btv->showblocks && g_sequence_get_length(btv->scancache.stackcaches)>0) {
		btv->margin_pixels_block = 12;
	} else {
		btv->margin_pixels_block = 0;
	}
	if (btv->showsymbols) {
		btv->margin_pixels_symbol = 12;
	} else {
		btv->margin_pixels_symbol = 0;
	}
	/*g_print("lines=%d,count=%d,pixels_per_char=%d\n",lines,count,btv->margin_pixels_per_char);*/
	gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(btv), GTK_TEXT_WINDOW_LEFT, btv->margin_pixels_chars+btv->margin_pixels_block+btv->margin_pixels_symbol);
}

static gboolean char_in_allsymbols(BluefishTextView * btv, gunichar uc) {
	if (uc > 127)
		return FALSE;
	if (btv->bflang && btv->bflang->st)
		return btv->bflang->st->allsymbols[uc];
	return FALSE;
}

static void bftextview2_insert_text_after_lcb(GtkTextBuffer * buffer, GtkTextIter * iter, gchar * string,
										gint stringlen, BluefishTextView * btv)
{
	GtkTextIter start;
	gint start_offset;
	DBG_SIGNALS("bftextview2_insert_text_lcb, stringlen=%d\n", stringlen);
#ifdef IN_BLUEFISH
	if (!main_v->props.reduced_scan_triggers || stringlen > 1 || (stringlen==1 && char_in_allsymbols(btv, string[0]))) {
		bftextview2_schedule_scanning(btv);
	}
#else
	if (!reduced_scan_triggers || stringlen > 1 || (stringlen==1 && char_in_allsymbols(btv, string[0]))) {
		bftextview2_schedule_scanning(btv);
	}
#endif
	/* mark the text that is changed */
	start = *iter;
	gtk_text_iter_backward_chars(&start, stringlen);

	gtk_text_buffer_apply_tag(buffer, btv->needscanning, &start, iter);
	DBG_SIGNALS("mark text from %d to %d as needscanning\n", gtk_text_iter_get_offset(&start),
			gtk_text_iter_get_offset(iter));
	start_offset = gtk_text_iter_get_offset(&start);
	if (btv->scancache.stackcache_need_update_charoffset == -1
		|| btv->scancache.stackcache_need_update_charoffset > start_offset) {
		btv->scancache.stackcache_need_update_charoffset = start_offset;
	}
#ifdef IN_BLUEFISH
	if (btv->enable_scanner && btv->autocomplete && (btv->autocomp || main_v->props.autocomp_popup_mode == 2)) {
		autocomp_run(btv,FALSE);
	}
#else
	if (btv->enable_scanner && btv->autocomplete && (btv->autocomp || autocomp_popup_mode == 2)) {
		autocomp_run(btv,FALSE);
	}
#endif
	bftextview2_reset_user_idle_timer(btv);
	bftextview2_set_margin_size(btv);
}

static void print_fstack(Tfoundstack * fstack)
{
	DBG_MARGIN("got fstack %p for next position", fstack);
	if (fstack)
		DBG_MARGIN(" with line %d and charoffset %d and %d blocks", fstack->line, fstack->charoffset,
				g_queue_get_length(fstack->blockstack));
	DBG_MARGIN("\n");
}

static void paint_margin_expand(BluefishTextView *btv,GdkEventExpose * event,gint w,gint height) {
/*	gtk_paint_box(GTK_WIDGET(btv)->style, event->window, GTK_STATE_ACTIVE,
				  GTK_SHADOW_OUT, NULL, GTK_WIDGET(btv), NULL, 21, w + (height / 2 - 4), 9, 9);
	gtk_paint_hline(GTK_WIDGET(btv)->style, event->window, GTK_WIDGET_STATE(btv), NULL,
					GTK_WIDGET(btv), NULL, 23, 28, w + (height / 2));
	gtk_paint_vline(GTK_WIDGET(btv)->style, event->window, GTK_WIDGET_STATE(btv), NULL,
					GTK_WIDGET(btv), NULL, w + (height / 2) + 4, w + height, 25);*/
	gdk_draw_rectangle(GDK_DRAWABLE(event->window),GTK_WIDGET(btv)->style->base_gc[GTK_WIDGET_STATE(btv)], TRUE,btv->margin_pixels_chars+btv->margin_pixels_symbol+1, w + (height / 2) - 4, 8,8);
	gdk_draw_rectangle(GDK_DRAWABLE(event->window),GTK_WIDGET(btv)->style->fg_gc[GTK_WIDGET_STATE(btv)], FALSE,btv->margin_pixels_chars+btv->margin_pixels_symbol+1, w + (height / 2) - 4, 8,8);
	gdk_draw_line(GDK_DRAWABLE(event->window),GTK_WIDGET(btv)->style->fg_gc[GTK_WIDGET_STATE(btv)],btv->margin_pixels_chars+btv->margin_pixels_symbol+5,w + (height / 2) + 5,btv->margin_pixels_chars+btv->margin_pixels_symbol+5, w + height);
	gdk_draw_line(GDK_DRAWABLE(event->window),GTK_WIDGET(btv)->style->fg_gc[GTK_WIDGET_STATE(btv)],btv->margin_pixels_chars+btv->margin_pixels_symbol+3,w + (height / 2), btv->margin_pixels_chars+btv->margin_pixels_symbol+7,w + (height / 2));
}

static void paint_margin_collapse(BluefishTextView *btv,GdkEventExpose * event,gint w,gint height) {
/*	gtk_paint_box(GTK_WIDGET(btv)->style, event->window, GTK_WIDGET_STATE(btv),
				  GTK_SHADOW_OUT, NULL, GTK_WIDGET(btv), NULL, 21, w + (height / 2 - 4), 9, 9);
	gtk_paint_hline(GTK_WIDGET(btv)->style, event->window, GTK_WIDGET_STATE(btv), NULL,
					GTK_WIDGET(btv), NULL, 23, 28, w + (height / 2));
	gtk_paint_vline(GTK_WIDGET(btv)->style, event->window, GTK_WIDGET_STATE(btv), NULL,
					GTK_WIDGET(btv), NULL, w + (height / 2) -2, w + (height / 2) +2, 25);
	gtk_paint_vline(GTK_WIDGET(btv)->style, event->window, GTK_WIDGET_STATE(btv), NULL,
					GTK_WIDGET(btv), NULL, w + (height / 2) + 4, w + height, 25);*/
	gdk_draw_rectangle(GDK_DRAWABLE(event->window),GTK_WIDGET(btv)->style->base_gc[GTK_WIDGET_STATE(btv)], TRUE,btv->margin_pixels_chars+btv->margin_pixels_symbol+1, w + (height / 2) - 4, 8,8);
	gdk_draw_rectangle(GDK_DRAWABLE(event->window),GTK_WIDGET(btv)->style->fg_gc[GTK_WIDGET_STATE(btv)], FALSE,btv->margin_pixels_chars+btv->margin_pixels_symbol+1, w + (height / 2) - 4, 8,8);
	gdk_draw_line(GDK_DRAWABLE(event->window),GTK_WIDGET(btv)->style->fg_gc[GTK_WIDGET_STATE(btv)],btv->margin_pixels_chars+btv->margin_pixels_symbol+5,w + (height / 2)-2,btv->margin_pixels_chars+btv->margin_pixels_symbol+5, w + (height / 2)+2);
	gdk_draw_line(GDK_DRAWABLE(event->window),GTK_WIDGET(btv)->style->fg_gc[GTK_WIDGET_STATE(btv)],btv->margin_pixels_chars+btv->margin_pixels_symbol+5,w + (height / 2) + 5,btv->margin_pixels_chars+btv->margin_pixels_symbol+5, w + height);
	gdk_draw_line(GDK_DRAWABLE(event->window),GTK_WIDGET(btv)->style->fg_gc[GTK_WIDGET_STATE(btv)],btv->margin_pixels_chars+btv->margin_pixels_symbol+3,w + (height / 2), btv->margin_pixels_chars+btv->margin_pixels_symbol+7,w + (height / 2));
}

static void paint_margin_blockend(BluefishTextView *btv,GdkEventExpose * event,gint w,gint height) {
	gdk_draw_line(GDK_DRAWABLE(event->window),GTK_WIDGET(btv)->style->fg_gc[GTK_WIDGET_STATE(btv)],btv->margin_pixels_chars+btv->margin_pixels_symbol+5, w, btv->margin_pixels_chars+btv->margin_pixels_symbol+5, w + (height/2));
	gdk_draw_line(GDK_DRAWABLE(event->window),GTK_WIDGET(btv)->style->fg_gc[GTK_WIDGET_STATE(btv)],btv->margin_pixels_chars+btv->margin_pixels_symbol+5, w+(height/2), btv->margin_pixels_chars+btv->margin_pixels_symbol+8, w + (height/2));
/*	gtk_paint_vline(GTK_WIDGET(btv)->style, event->window, GTK_WIDGET_STATE(btv), NULL,
					GTK_WIDGET(btv), NULL, w, w + (height / 2), 25);
	gtk_paint_hline(GTK_WIDGET(btv)->style, event->window, GTK_WIDGET_STATE(btv), NULL,
					GTK_WIDGET(btv), NULL, 25, 29, w + (height / 2));*/
}

static void paint_margin_line(BluefishTextView *btv,GdkEventExpose * event,gint w,gint height) {
	gdk_draw_line(GDK_DRAWABLE(event->window),GTK_WIDGET(btv)->style->fg_gc[GTK_WIDGET_STATE(btv)],btv->margin_pixels_chars+btv->margin_pixels_symbol+5, w, btv->margin_pixels_chars+btv->margin_pixels_symbol+5, w + height);
	/*gtk_paint_vline(GTK_WIDGET(btv)->style, event->window, GTK_WIDGET_STATE(btv), NULL,
					GTK_WIDGET(btv), NULL, w, w + height, 25);*/
}

static void paint_margin_symbol(BluefishTextView *btv,GdkEventExpose * event,gint w,gint height) {
	gdk_draw_rectangle(GDK_DRAWABLE(event->window),GTK_WIDGET(btv)->style->fg_gc[GTK_WIDGET_STATE(btv)], TRUE,btv->margin_pixels_chars+2, w + (height / 2) - 4, 8,8);
}

static gint get_num_foldable_blocks(Tfoundstack *fstack) {
	guint count=0;
	GList *tmplist = fstack->blockstack->head;
	while (tmplist) {
		if ( ((Tfoundblock *)(tmplist->data))->foldable)
			count++;
		tmplist = g_list_next(tmplist);
	}
	return count;
}

/*
static Tfoundstack *get_next_foldable_block(BluefishTextView *btv,GSequenceIter *siter, guint line) {
	Tfoundstack *fstack = g_sequence_get(siter);
	while (fstack) {
		if (fstack->line > line)
			return NULL;
		if (fstack->line == line && fstack->pushedblock && fstack->pushedblock->foldable)
			return fstack;
		fstack = get_stackcache_next(btv, &siter);
	}
	return NULL;
}*/

static void paint_margin(BluefishTextView *btv,GdkEventExpose * event, GtkTextIter * startvisible, GtkTextIter * endvisible)
{
	Tfoundstack *fstack = NULL;
	GSequenceIter *siter=NULL;
	guint num_blocks;
	GtkTextIter it;
	GtkTextTag *folded;
	gint i;
	PangoLayout *panlay;
	gpointer bmark;
	gint bmarkline=-1;

	/* to see how many blocks are active here */
	if (gtk_text_iter_is_start(startvisible) && (g_sequence_get_length (btv->scancache.stackcaches) != 0)) {
		siter = g_sequence_get_begin_iter(btv->scancache.stackcaches);
		if (!g_sequence_iter_is_end(siter)) {
			fstack = g_sequence_get(siter);
		}
		num_blocks = 0;
		DBG_MARGIN("EXPOSE: start at begin, set num_blocks %d, fstack=%p\n", num_blocks, fstack);
	} else {
		fstack = get_stackcache_at_position(btv, startvisible, &siter);
		if (fstack) {
			num_blocks = get_num_foldable_blocks(fstack);
			DBG_MARGIN
				("EXPOSE: got fstack %p with line %d and charoffset %d and num_blocks %d for start position %d\n",
				 fstack, fstack->line, fstack->charoffset, num_blocks,
				 gtk_text_iter_get_offset(startvisible));
		} else {
			DBG_MARGIN("EXPOSE: no fstack for position %d, siter=%p\n",gtk_text_iter_get_offset(startvisible),siter);
			num_blocks = 0;
		}
	}
	/* in the case that the *first* fstack is relevant, we don't need
	   the 'next' fstack */
	if (!fstack || fstack->charoffset < gtk_text_iter_get_offset(startvisible)) {
		DBG_MARGIN("get next fstack..\n");
		if (siter)
			fstack = get_stackcache_next(btv, &siter);
	}
	/*DBG_MARGIN("first fstack ");
	print_fstack(fstack);*/

	it = *startvisible;
	panlay = gtk_widget_create_pango_layout(GTK_WIDGET(btv), "x");

	folded = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv))),"_folded_");
	if (btv->showsymbols) {
		bmarkline = bmark_margin_get_initial_bookmark((Tdocument *)btv->doc, startvisible, &bmark);
	}

	for (i = gtk_text_iter_get_line(startvisible); i <= gtk_text_iter_get_line(endvisible); i++) {
		gint w, height;
		gchar *string;

		gtk_text_iter_set_line(&it, i);

		if (gtk_text_iter_has_tag(&it, folded)) {
			DBG_FOLD("line %d is hidden\n",i);
		} else {
			gtk_text_view_get_line_yrange(GTK_TEXT_VIEW(btv), &it, &w, &height);
			gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(btv), GTK_TEXT_WINDOW_LEFT, 0, w,
												  NULL, &w);

			/* line numbers */
			if (btv->linenumbers) {
				string = g_strdup_printf("%d", 1 + i);
				pango_layout_set_markup(panlay, string, -1);
				gtk_paint_layout(GTK_WIDGET(btv)->style, event->window, GTK_WIDGET_STATE(btv), FALSE, NULL,
								 GTK_WIDGET(btv), NULL, 2, w, panlay);
				g_free(string);
			}
			/* symbols */
			if (btv->showsymbols && bmarkline != -1) {
				while (bmarkline != -1 && bmarkline < i) {
					bmarkline = bmark_margin_get_next_bookmark((Tdocument *)btv->doc, &bmark);
				}
				if (bmarkline == i) {
					paint_margin_symbol(btv,event,w,height);
				}
			}
			/* block folding.
			- to find out if we need an expander/collapse, we need to see if there is a pushedblock on the
			blockstack which has 'foldable' for any stackcache that is on this line.
			- to find if we need an end-of-block we need to see if there is a poppedblock on this line
			which has 'foldable'
			- to find out if we need a line or nothing we need to know the number of expanded blocks on the stack
			 */
			if (btv->showblocks) {
				while (fstack) {
					if (fstack->line > i) {
						DBG_FOLD("found fstack for line %d, num_blocks=%d..\n",fstack->line,num_blocks);
						if (num_blocks > 0) {
							paint_margin_line(btv,event,w,height);
						}
						break;
					}
					if (fstack->line == i) {
						DBG_FOLD("found fstack %p (charoffset %d) for current line %d, pushedblock=%p,poppedblock=%p\n",fstack, fstack->charoffset, fstack->line,fstack->pushedblock,fstack->poppedblock);
						if (fstack->pushedblock && fstack->pushedblock->foldable) {
							if (fstack->pushedblock->folded)
								paint_margin_collapse(btv,event,w,height);
							else
								paint_margin_expand(btv,event,w,height);

							num_blocks = get_num_foldable_blocks(fstack);
							break;
						} else if (fstack->poppedblock && fstack->poppedblock->foldable) {
							paint_margin_blockend(btv,event,w,height);
							num_blocks = get_num_foldable_blocks(fstack);
							break;
						}
					}
					fstack = get_stackcache_next(btv, &siter);
				}
			}
		}
	}
	g_object_unref(G_OBJECT(panlay));
}

static gboolean bluefish_text_view_expose_event(GtkWidget * widget, GdkEventExpose * event)
{
	BluefishTextView *btv = BLUEFISH_TEXT_VIEW (widget);
	gboolean event_handled = FALSE;
	DBG_SIGNALS("bluefish_text_view_expose_event\n");

	/* Optimally this should only scan the visible area and only if necessary */
	if (event->window == gtk_text_view_get_window(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT))
		bftextview2_schedule_scanning(btv);
	
	if (event->window == gtk_text_view_get_window(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_LEFT)) {
		GtkTextIter startvisible, endvisible;
		GdkRectangle rect;

		gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(widget), &rect);
		gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(widget), &startvisible, rect.y, NULL);
		gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(widget), &endvisible, rect.y + rect.height, NULL);

		paint_margin(btv, event, &startvisible, &endvisible);
		event_handled = TRUE;
	} else {
		 if (GTK_WIDGET_IS_SENSITIVE(btv) &&
				 (event->window == gtk_text_view_get_window(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT))) {
			/* current line highlighting */
			GdkRectangle rect;
			gint w,w2;
			GtkTextIter it;
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
			gtk_text_buffer_get_iter_at_mark(buffer, &it, gtk_text_buffer_get_insert(buffer));
			gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(widget), &rect);
			gtk_text_view_get_line_yrange(GTK_TEXT_VIEW(widget), &it, &w, &w2);
			gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT,
														  rect.x, rect.y, &rect.x, &rect.y);
			gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT,
													  0, w, NULL, &w);
			gdk_draw_rectangle(event->window, widget->style->bg_gc[GTK_WIDGET_STATE(widget)],
									   TRUE, rect.x, w, rect.width, w2);
		}
		
		if (GTK_WIDGET_CLASS(bluefish_text_view_parent_class)->expose_event)
 			event_handled = GTK_WIDGET_CLASS(bluefish_text_view_parent_class)->expose_event (widget, event); 			
	}
	
	return event_handled;
}

static void bftextview2_delete_range_lcb(GtkTextBuffer * buffer, GtkTextIter * obegin,
										 GtkTextIter * oend, gpointer user_data)
{
	guint start_offset;
	GtkTextIter begin=*obegin,end=*oend;
	BluefishTextView *btv=user_data;
	DBG_SIGNALS("bftextview2_delete_range_lcb\n");

	/* mark the surroundings of the ext that will be deleted */
	gtk_text_iter_backward_word_start(&begin);
	gtk_text_iter_forward_word_end(&end);
	gtk_text_buffer_apply_tag(buffer, btv->needscanning, &begin, &end);
	DBG_SIGNALS("mark text from %d to %d as needscanning\n", gtk_text_iter_get_offset(&begin),
			gtk_text_iter_get_offset(&end));
	start_offset = gtk_text_iter_get_offset(&begin);
	if (btv->scancache.stackcache_need_update_charoffset == -1
		|| btv->scancache.stackcache_need_update_charoffset > start_offset) {
		btv->scancache.stackcache_need_update_charoffset = start_offset;
	}

	bftextview2_reset_user_idle_timer(btv);
	bftextview2_schedule_scanning(btv);
}
static void bftextview2_delete_range_after_lcb(GtkTextBuffer * buffer, GtkTextIter * obegin,
										 GtkTextIter * oend, gpointer user_data)
{
	BluefishTextView *btv=user_data;
	if (btv->enable_scanner && btv->autocomplete && (btv->autocomp || main_v->props.autocomp_popup_mode == 2)) {
		autocomp_run(btv,FALSE);
	}
}

static gboolean bluefish_text_view_key_press_event(GtkWidget * widget, GdkEventKey * kevent) {
	BluefishTextView *btv = BLUEFISH_TEXT_VIEW (widget);
	
	if (btv->autocomp) {
		if (acwin_check_keypress(btv, kevent)) {
			btv->key_press_was_autocomplete = TRUE;
			return TRUE;
		}
	}
	btv->key_press_was_autocomplete = FALSE;
	if (btv->enable_scanner && btv->autocomplete && (kevent->state & GDK_CONTROL_MASK) && kevent->keyval == ' ') {
		autocomp_run(btv,TRUE);
		return TRUE;
	}
#ifdef IN_BLUEFISH
	if (main_v->props.editor_smart_cursor && !(kevent->state & GDK_CONTROL_MASK) && ((kevent->keyval == GDK_Home) || (kevent->keyval == GDK_KP_Home) || (kevent->keyval == GDK_End) || (kevent->keyval == GDK_KP_End)))
#else
	if (smart_cursor && !(kevent->state & GDK_CONTROL_MASK) && ((kevent->keyval == GDK_Home) || (kevent->keyval == GDK_KP_Home) || (kevent->keyval == GDK_End) || (kevent->keyval == GDK_KP_End)))
#endif
	{
		GtkTextMark* imark;
		GtkTextIter iter, currentpos, linestart;
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));

		imark = gtk_text_buffer_get_insert(buffer);
		gtk_text_buffer_get_iter_at_mark(buffer, &currentpos, imark);
		iter = currentpos;

		if ((kevent->keyval == GDK_Home) || (kevent->keyval == GDK_KP_Home)) {
			gtk_text_iter_backward_cursor_positions(&iter, gtk_text_iter_get_line_offset(&iter));
			linestart = iter;

			while (g_unichar_isspace(gtk_text_iter_get_char (&iter)) && !gtk_text_iter_ends_line(&iter))
				gtk_text_iter_forward_char (&iter);
		} else { /* (kevent->keyval == GDK_End) || (kevent->keyval == GDK_KP_End) */
			if (!gtk_text_iter_ends_line(&iter))
				gtk_text_iter_forward_to_line_end(&iter);
			linestart = iter;
			if (gtk_text_iter_is_end (&iter) && !gtk_text_iter_starts_line (&iter))
				gtk_text_iter_backward_char(&iter);
			while (g_unichar_isspace (gtk_text_iter_get_char (&iter)) && !gtk_text_iter_starts_line (&iter))
				gtk_text_iter_backward_char(&iter);
			if ((!gtk_text_iter_starts_line (&iter) || !gtk_text_iter_ends_line (&iter)) && !g_unichar_isspace (gtk_text_iter_get_char (&iter)))
				gtk_text_iter_forward_char(&iter);
		}
		if (gtk_text_iter_compare(&currentpos, &iter) == 0)
			iter = linestart;
		if (kevent->state & GDK_SHIFT_MASK)
			gtk_text_buffer_move_mark(buffer, imark, &iter);
		else
			gtk_text_buffer_place_cursor(buffer, &iter);
		gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(btv), gtk_text_buffer_get_insert(buffer));
		return TRUE;
	}

	return GTK_WIDGET_CLASS(bluefish_text_view_parent_class)->key_press_event (widget, kevent);
}

static void bftextview2_toggle_fold(BluefishTextView *btv, GtkTextIter *iter) {
	Tfoundstack *fstack;
	GSequenceIter *siter;
	gint line;

	if (!btv->bflang)
		return;

	line = gtk_text_iter_get_line(iter);
	 /* returns the fstack PRIOR to iter, or the fstack excactly at iter,
	 but this fails if the iter is the start of the buffer */
	fstack = get_stackcache_at_position(btv, iter, &siter);
	if (!fstack) {
		DBG_FOLD("no fstack, retrieve first iter\n");
		fstack = get_stackcache_first(btv, &siter);
	}
	while (fstack && (fstack->line < line || !fstack->pushedblock || !fstack->pushedblock->foldable)) {
		fstack = get_stackcache_next(btv, &siter); /* should be the first fstack AFTER iter */
		if (fstack && fstack->pushedblock && fstack->pushedblock->foldable)
			break;
	}
	if (fstack && fstack->line == line && fstack->pushedblock && fstack->pushedblock->foldable) {
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
		GtkTextIter it1,it2,it3,it4;
		DBG_FOLD("got fstack=%p on line %d\n",fstack,fstack->line);
		bftextview2_get_iters_at_foundblock(buffer, fstack->pushedblock, &it1, &it2, &it3, &it4);
		if (main_v->props.block_folding_mode == 1 && !gtk_text_iter_ends_line(&it2)) {
			gtk_text_iter_forward_to_line_end(&it2);
		}
		if (gtk_text_iter_ends_line(&it4)) {
			gtk_text_iter_forward_line(&it4);
		}
		if (fstack->pushedblock->folded) {
			DBG_FOLD("expand fstack with line %d\n",fstack->line);
			gtk_text_buffer_remove_tag_by_name(buffer, "foldheader", &it1, &it2);
			if (main_v->props.block_folding_mode==0) {
				gtk_text_buffer_remove_tag_by_name(buffer, "_folded_", &it2, &it3);
				gtk_text_buffer_remove_tag_by_name(buffer, "foldheader", &it3, &it4);
			} else if (main_v->props.block_folding_mode==1) {
				gtk_text_buffer_remove_tag_by_name(buffer, "_folded_", &it2, &it4);
			}
			fstack->pushedblock->folded=FALSE;
		} else {
			DBG_FOLD("collapse fstack with line %d\n",fstack->line);
			gtk_text_buffer_apply_tag_by_name(buffer, "foldheader", &it1, &it2);
			if (main_v->props.block_folding_mode==0) {
				gtk_text_buffer_apply_tag_by_name(buffer, "_folded_", &it2, &it3);
				gtk_text_buffer_apply_tag_by_name(buffer, "foldheader", &it3, &it4);
			} else if (main_v->props.block_folding_mode==1) {
				gtk_text_buffer_apply_tag_by_name(buffer, "_folded_", &it2, &it4);
			}
			fstack->pushedblock->folded=TRUE;
		}
	}
}

static gboolean bluefish_text_view_button_press_event(GtkWidget * widget, GdkEventButton * event) {
	BluefishTextView *btv = BLUEFISH_TEXT_VIEW (widget);

	if (event->window == gtk_text_view_get_window(GTK_TEXT_VIEW(btv),GTK_TEXT_WINDOW_LEFT)) {
		DBG_SIGNALS("bluefish_text_view_button_press_event\n");
		if (event->button == 1) {
			gint x, y;
			GtkTextIter it;
			if (event->x >= btv->margin_pixels_chars) { /* get the offset that equals the folding area */
				gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(btv), GTK_TEXT_WINDOW_TEXT, 0,
													  event->y, &x, &y);
				gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(widget), &it, y, &x);
				DBG_FOLD("fold/unfold at offset %d (line %d)\n",gtk_text_iter_get_offset(&it),gtk_text_iter_get_line(&it));
				bftextview2_toggle_fold(btv, &it);
				/*block_mark = bftv_get_first_block_at_line(BF_TEXTVIEW(widget), &it, TRUE);
				if (block_mark)
					bftv_fold(BF_TEXTVIEW(widget), block_mark, TRUE);*/
				return TRUE;
			} else if (event->x < btv->margin_pixels_chars) {
				GtkTextIter it2;
				/* select line */
				gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(btv), GTK_TEXT_WINDOW_TEXT, 0,
													  event->y, &x, &y);
				gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(widget), &it, y, &x);
				it2 = it;
				gtk_text_iter_forward_to_line_end(&it2);
				g_print("select from %d to %d\n",gtk_text_iter_get_offset(&it),gtk_text_iter_get_offset(&it2));
				gtk_text_buffer_select_range(gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv)),&it,&it2);
				return TRUE;
			}
		}
	}/* else if (event->button.button == 3) {
		if (event->button.x >= pt_blocks) {
			gtk_menu_popup(GTK_MENU(BF_TEXTVIEW(widget)->fold_menu), NULL, NULL, NULL, NULL, 1,
						   gtk_get_current_event_time());
			return TRUE;
		}
	}*/
	return GTK_WIDGET_CLASS(bluefish_text_view_parent_class)->button_press_event (widget, event);
}

static gboolean bftextview2_key_release_lcb(GtkWidget *widget,GdkEventKey *kevent,gpointer user_data) {
	BluefishTextView *btv=user_data;
	if (!btv->key_press_was_autocomplete && (kevent->keyval == GDK_Return || kevent->keyval == GDK_KP_Enter) && !(kevent->state & GDK_SHIFT_MASK || kevent->state & GDK_CONTROL_MASK || kevent->state & GDK_MOD1_MASK)) {
		if (btv->autoindent) {
			gchar *string, *indenting;
			GtkTextMark* imark;
			GtkTextIter itstart, itend;
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
			imark = gtk_text_buffer_get_insert(buffer);
			gtk_text_buffer_get_iter_at_mark(buffer,&itend,imark);
			itstart = itend;
			/* set to the beginning of the previous line */
			gtk_text_iter_backward_line(&itstart);
			gtk_text_iter_set_line_index(&itstart, 0);
			string = gtk_text_buffer_get_text(buffer,&itstart,&itend,TRUE);
			if (string) {
				/* now count the indenting in this string */
				indenting = string;
				while (*indenting == '\t' || *indenting == ' ') {
					indenting++;
				}
				/* ending search, non-whitespace found, so terminate at this position */
				*indenting = '\0';
				if (strlen(string)) {
					gtk_text_buffer_insert(buffer,&itend,string,-1);
				}
				g_free(string);
			}
		}
	}
	return FALSE; /* we didn't handle all of the event */
}

void bluefish_text_view_rescan(BluefishTextView * btv) {
	cleanup_scanner(btv);
	if (btv->bflang) {
		GtkTextIter start,end;
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
		gtk_text_buffer_get_bounds(buffer,&start,&end);
		gtk_text_buffer_apply_tag(buffer, btv->needscanning, &start, &end);
		bftextview2_schedule_scanning(btv);
	}
}

void bluefish_text_view_set_mimetype(BluefishTextView * btv, const gchar *mime) {
	GtkTextIter start,end;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
	Tbflang *bflang = langmgr_get_bflang_for_mimetype(mime);
	/* remove all highlighting */
	cleanup_scanner(btv);
	if (bflang) {
		/* set new language */
		btv->bflang = bflang;
		/* restart scanning */
		gtk_text_buffer_get_bounds(buffer,&start,&end);
		gtk_text_buffer_apply_tag(buffer, btv->needscanning, &start, &end);
		bftextview2_schedule_scanning(btv);
	} else {
		btv->bflang = NULL;
	}
}

static gboolean bluefish_text_view_query_tooltip(GtkWidget *widget, gint x, gint y, gboolean keyboard_tip, GtkTooltip *tooltip) {
	BluefishTextView *btv = BLUEFISH_TEXT_VIEW (widget);	
	
	if (btv->bflang && btv->bflang->st && btv->enable_scanner && btv->scanner_idle==0) {
		GtkTextIter iter,mstart;
		gint contextnum;
		/* get position */
		if (keyboard_tip) {
			gint offset;
			g_object_get(GTK_TEXT_VIEW(btv)->buffer,"cursor-position", &offset, NULL);
			gtk_text_buffer_get_iter_at_offset(GTK_TEXT_VIEW(btv)->buffer, &iter, offset);
		} else {
			gint bx, by, trailing;
			gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(btv), GTK_TEXT_WINDOW_TEXT,x-(btv->margin_pixels_chars+btv->margin_pixels_block), y, &bx, &by);
			gtk_text_view_get_iter_at_position(GTK_TEXT_VIEW(btv), &iter, &trailing, bx, by);
		}
		mstart=iter;
		gtk_text_iter_set_line_offset(&mstart,0);
		gtk_text_iter_forward_char(&iter);
		DBG_MSG("scan for tooltip: start at %d, position=%d...\n",gtk_text_iter_get_offset(&mstart),gtk_text_iter_get_offset(&iter));
		if (scan_for_tooltip(btv,&mstart,&iter,&contextnum)) {
			DBG_MSG("we have a match in context %d\n",contextnum);
			if (g_array_index(btv->bflang->st->contexts,Tcontext, contextnum).reference) {
				gchar *value, *key = gtk_text_buffer_get_text(GTK_TEXT_VIEW(btv)->buffer,&mstart,&iter,TRUE);
				value = g_hash_table_lookup(g_array_index(btv->bflang->st->contexts,Tcontext, contextnum).reference, key);
				DBG_MSG("key=%s, value=%s\n",key,value);
				if (value) {
					gtk_tooltip_set_markup(tooltip, value);
					g_free(key);
					return TRUE;
				}
				g_free(key);
			}
		}
	}

	return FALSE;
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
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	object_class->constructor = bluefish_text_view_create;
	object_class->finalize = bluefish_text_view_finalize;
	
	widget_class->button_press_event = bluefish_text_view_button_press_event;
	widget_class->expose_event = bluefish_text_view_expose_event;
	widget_class->key_press_event = bluefish_text_view_key_press_event;
	widget_class->query_tooltip = bluefish_text_view_query_tooltip;
}

static void bluefish_text_view_init(BluefishTextView * textview)
{
/*	PangoFontDescription *font_desc;*/
	textview->user_idle_timer = g_timer_new();
	textview->scancache.stackcaches = g_sequence_new(NULL);
#ifdef IN_BLUEFISH
	if (main_v->props.editor_bg) {
		GdkColor color;
		gdk_color_parse(main_v->props.editor_bg, &color);
		gtk_widget_modify_base(GTK_WIDGET(textview),GTK_STATE_NORMAL, &color);
	}
	textview->linenumbers = main_v->props.view_line_numbers;
	textview->showblocks = main_v->props.view_blocks;
	textview->autoindent = main_v->props.autoindent;
	textview->showsymbols=TRUE;
#else
	if (background_color) {
		GdkColor color;
		gdk_color_parse(background_color, &color);
		gtk_widget_modify_base(GTK_WIDGET(textview),GTK_STATE_NORMAL, &color);
	}
#endif
	textview->needscanning = gtk_text_tag_table_lookup(langmgr_get_tagtable(),"_needscanning_");
	textview->blockmatch = gtk_text_tag_table_lookup(langmgr_get_tagtable(),"blockmatch");
	textview->enable_scanner=FALSE;
	textview->autocomplete=TRUE;	
	/*font_desc = pango_font_description_from_string("Monospace 10");
	gtk_widget_modify_font(GTK_WIDGET(textview), font_desc);
	pango_font_description_free(font_desc);*/
	bftextview2_set_margin_size(textview);
}

GtkWidget *bftextview2_new(void)
{
	BluefishTextView *textview = g_object_new(BLUEFISH_TYPE_TEXT_VIEW,
																						"has-tooltip", TRUE,
																						NULL);

	g_return_val_if_fail(textview != NULL, NULL);

	return GTK_WIDGET(textview);
}

GtkWidget *bftextview2_new_with_buffer(GtkTextBuffer * buffer)
{
	BluefishTextView *textview = (BluefishTextView *) bftextview2_new();

	g_return_val_if_fail(textview != NULL, NULL);

	gtk_text_view_set_buffer(GTK_TEXT_VIEW(textview), buffer);

	g_signal_connect_after(G_OBJECT(buffer), "insert-text", G_CALLBACK(bftextview2_insert_text_after_lcb),textview);
	g_signal_connect_after(G_OBJECT(buffer), "mark-set", G_CALLBACK(bftextview2_mark_set_lcb),textview);
	g_signal_connect(G_OBJECT(buffer), "delete-range", G_CALLBACK(bftextview2_delete_range_lcb),textview);
	g_signal_connect_after(G_OBJECT(buffer), "delete-range", G_CALLBACK(bftextview2_delete_range_after_lcb),textview);
	g_signal_connect_after(G_OBJECT(textview), "key-release-event", G_CALLBACK(bftextview2_key_release_lcb), textview);

	return GTK_WIDGET(textview);
}
