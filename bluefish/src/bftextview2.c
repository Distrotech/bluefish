/* Bluefish HTML Editor
 * bftextview2.c
 *
 * Copyright (C) 2008-2010 Olivier Sessink
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

/* for the design docs see bftextview2.h */

#include <gdk/gdkkeysyms.h>
#include <math.h>				/* log10() */
#include <string.h>				/* strlen() */

#include "bftextview2.h"

#include "bluefish.h"
#include "bf_lib.h"
#include "bookmark.h"
#include "document.h"

#include "bftextview2_scanner.h"
#include "bftextview2_patcompile.h"
#include "bftextview2_autocomp.h"
#include "bftextview2_langmgr.h"
#ifdef HAVE_LIBENCHANT
#include "bftextview2_spell.h"
#endif

#define USER_IDLE_EVENT_INTERVAL 480	/* milliseconds */
/*
G_PRIORITY_HIGH -100 			Use this for high priority event sources. It is not used within GLib or GTK+.
G_PRIORITY_DEFAULT 0 			Use this for default priority event sources. In GLib this priority is used when adding 
										timeout functions with g_timeout_add(). In GDK this priority is used for events from the X server.
G_PRIORITY_HIGH_IDLE 100 		Use this for high priority idle functions. GTK+ uses G_PRIORITY_HIGH_IDLE + 10 for resizing 
										operations, and G_PRIORITY_HIGH_IDLE + 20 for redrawing operations. (This is done to ensure 
										that any pending resizes are processed before any pending redraws, so that widgets are not 
										redrawn twice unnecessarily.)
G_PRIORITY_DEFAULT_IDLE 200 	Use this for default priority idle functions. In GLib this priority is used when adding idle 
										functions with g_idle_add().
G_PRIORITY_LOW 300
*/
#define SCANNING_IDLE_PRIORITY -50
/* a newly loaded language file generates a priority 122 event to notice all documents to be rescanned.
to make sure that we don't scan or spellcheck a file that will be scanned again we do timeout
scanning in a lower priority timeout  */
#define SCANNING_IDLE_AFTER_TIMEOUT_PRIORITY 125	/* a higher priority makes bluefish go greyed-out (it will not redraw if required while the loop is running)
													   and a much lower priority (tried 250) will first draw all textstyles on screen before the
													   next burst of scanning is done */

G_DEFINE_TYPE(BluefishTextView, bluefish_text_view, GTK_TYPE_TEXT_VIEW)

static GdkColor st_whitespace_color;
static GdkColor st_cline_color;

/****************************** utility functions ******************************/

static inline gboolean is_symbol(BluefishTextView * btv, gint contextnum, gunichar uc)
{
	if (G_UNLIKELY(uc > 127))
		return FALSE;
	return (g_array_index
			(btv->bflang->st->table, Ttablerow,
			 g_array_index(btv->bflang->st->contexts, Tcontext,
						   contextnum).identstate).row[uc] != g_array_index(btv->bflang->st->contexts,
																			Tcontext, contextnum).identstate);
}

gchar *bf_get_identifier_at_iter(BluefishTextView * btv, GtkTextIter * iter, gint * contextnum)
{
	GQueue *contextstack;
	GtkTextIter so, eo;

	so = eo = *iter;
	if (!btv->bflang || !btv->bflang->st) {
		while (gtk_text_iter_backward_char(&so) && !g_unichar_isspace(gtk_text_iter_get_char(&so))) {
		};
		gtk_text_iter_forward_char(&so);
		while (!g_unichar_isspace(gtk_text_iter_get_char(&eo)) && gtk_text_iter_forward_char(&eo)) {
		};

		return gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv)), &so, &eo, TRUE);
	}
	contextstack = get_contextstack_at_position(btv, iter);
	if (g_queue_get_length(contextstack) > 0)
		*contextnum = GPOINTER_TO_INT(g_queue_peek_head(contextstack));
	else
		*contextnum = 1;

	while (gtk_text_iter_backward_char(&so) && !is_symbol(btv, *contextnum, gtk_text_iter_get_char(&so))) {
	};
	gtk_text_iter_forward_char(&so);
	while (gtk_text_iter_forward_char(&eo) && !is_symbol(btv, *contextnum, gtk_text_iter_get_char(&eo))) {
	};
	return gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv)), &so, &eo, TRUE);
}

gboolean bf_text_iter_line_start_of_text(GtkTextIter * iter, GtkTextIter * realend)
{
	gtk_text_iter_set_line_offset(iter, 0);
	*realend = *iter;
	while (g_unichar_isspace(gtk_text_iter_get_char(iter))) {
		if (gtk_text_iter_ends_line(iter))
			return FALSE;
		gtk_text_iter_forward_char(iter);
	}
	return TRUE;
}

gboolean bf_text_iter_line_end_of_text(GtkTextIter * iter, GtkTextIter * realend)
{
	if (!gtk_text_iter_ends_line(iter))
		gtk_text_iter_forward_to_line_end(iter);
	if (gtk_text_iter_starts_line(iter))
		return FALSE;
	*realend = *iter;
	if (gtk_text_iter_is_end(iter))
		gtk_text_iter_backward_char(iter);
	while (g_unichar_isspace(gtk_text_iter_get_char(iter))) {
		if (gtk_text_iter_starts_line(iter))
			return FALSE;
		gtk_text_iter_backward_char(iter);
	}
	if (!gtk_text_iter_ends_line(iter) && !g_unichar_isspace(gtk_text_iter_get_char(iter)))
		gtk_text_iter_forward_char(iter);
	return TRUE;
}

/*************************** end of utility functions **************************/
static gboolean bftextview2_user_idle_timer(gpointer data)
{
	BluefishTextView *btv = data;
	guint elapsed = (guint) (1000.0 * g_timer_elapsed(btv->user_idle_timer, NULL));
	if ((elapsed + 20) >= USER_IDLE_EVENT_INTERVAL) {	/* avoid delaying again for less than 20 milliseconds */
		DBG_AUTOCOMP("bftextview2_user_idle_timer, user is > %d milliseconds idle, autocomp=%d, mode=%d\n",
					 elapsed, BLUEFISH_TEXT_VIEW(btv->master)->auto_complete, main_v->props.autocomp_popup_mode);
		if (BLUEFISH_TEXT_VIEW(btv->master)->auto_complete && btv->needs_autocomp && main_v->props.autocomp_popup_mode == 0) {
			autocomp_run(btv, FALSE);
			DBG_AUTOCOMP("bftextview2_user_idle_timer, set needs_autocomp to FALSE\n");
			btv->needs_autocomp = FALSE;
		}
		btv->user_idle = 0;
		return FALSE;
	} else {
		guint nextcheck;
		nextcheck = USER_IDLE_EVENT_INTERVAL - elapsed;
		DBG_AUTOCOMP
			("bftextview2_user_idle_timer, not yet elapsed (%d milliseconds idle), rechecking in %d milliseconds\n",
			 elapsed, nextcheck);
		btv->user_idle = g_timeout_add(nextcheck, bftextview2_user_idle_timer, btv);
		return FALSE;
	}
}

static void bftextview2_reset_user_idle_timer(BluefishTextView * btv)
{
	DBG_AUTOCOMP("bftextview2_reset_user_idle_timer: timer reset\n");
	g_timer_start(btv->user_idle_timer);	/* the timer is used both for delayed scanning AND for the delayed popup */
	if (btv->user_idle == 0 && main_v->props.autocomp_popup_mode == 0 /*&&btv->autocomp */ ) {
		btv->user_idle = g_timeout_add(USER_IDLE_EVENT_INTERVAL, bftextview2_user_idle_timer, btv);
		DBG_AUTOCOMP("started user_idle timeout with event source %d and timeout %d\n", btv->user_idle,
					 USER_IDLE_EVENT_INTERVAL);
	}
}

static void bftextview2_set_margin_size(BluefishTextView * btv);
static gboolean bftextview2_scanner_idle(gpointer data);
static gboolean bftextview2_scanner_timeout(gpointer data);

static gboolean bftextview2_scanner_scan(BluefishTextView * btv, gboolean in_idle)
{
	if (!btv->bflang) {
		if (in_idle)
			btv->scanner_idle = 0;
		else
			btv->scanner_delayed = 0;
		return FALSE;
	}
	if (!btv->bflang->st
#ifdef HAVE_LIBENCHANT
		&& !btv->spell_check
#endif
		) {
		if (in_idle)
			btv->scanner_idle = 0;
		else
			btv->scanner_delayed = 0;
		return FALSE;
	}
	if (main_v->props.delay_full_scan) {
		guint elapsed = (guint) (1000.0 * g_timer_elapsed(btv->user_idle_timer, NULL));
		DBG_DELAYSCANNING("%d milliseconds elapsed since last user action\n", elapsed);
		if ((elapsed + 20) >= main_v->props.delay_scan_time) {	/* delay scan time has passed ! */
			DBG_DELAYSCANNING("idle, call scan for everything\n");
			if (!bftextview2_run_scanner(btv, NULL)
#ifdef HAVE_LIBENCHANT
				&& !bftextview2_run_spellcheck(btv)
#endif
				) {
				/* finished scanning, make sure we are not called again */
				DBG_DELAYSCANNING("finished scanning (idle=%d)\n", in_idle);
				if (in_idle && btv->scanner_delayed) {
					g_source_remove(btv->scanner_delayed);
				} else if (!in_idle && btv->scanner_idle) {
					g_source_remove(btv->scanner_idle);
				}
				btv->scanner_delayed = 0;
				btv->scanner_idle = 0;
				bftextview2_set_margin_size(btv);
				DBG_DELAYSCANNING
					("return FALSE, scanning was finished (in_idle=%d, scanned_delayed=%d, scanner_idle=%d)\n",
					 in_idle, btv->scanner_delayed, btv->scanner_idle);
				return FALSE;
			}
			if (!in_idle) {
				/* don't call timeout function again (run in idle) */
				if (!btv->scanner_idle) {
					DBG_DELAYSCANNING("schedule scan again in idle time with priority %d\n",
									  SCANNING_IDLE_AFTER_TIMEOUT_PRIORITY);
					btv->scanner_idle =
						g_idle_add_full(SCANNING_IDLE_AFTER_TIMEOUT_PRIORITY, bftextview2_scanner_idle, btv,
										NULL);
				} else {
					DBG_DELAYSCANNING("scan in idle is already scheduled\n");
				}
				btv->scanner_delayed = 0;
				DBG_DELAYSCANNING
					("return FALSE for timeout callback (in_idle=%d, scanned_delayed=%d, scanner_idle=%d)\n",
					 in_idle, btv->scanner_delayed, btv->scanner_idle);
				return FALSE;
			}
			DBG_DELAYSCANNING("return TRUE, call idle callback again (scanned_delayed=%d, scanner_idle=%d)\n",
							  btv->scanner_delayed, btv->scanner_idle);
			return TRUE;		/* call idle function again */
		} else {				/* user is not idle, only scan visible area */
			GtkTextIter endvisible;
			GdkRectangle rect;
			/* get visible area and only scan the visible area */
			gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(btv), &rect);
			gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(btv), &endvisible, rect.y + rect.height, NULL);
			gtk_text_iter_forward_to_line_end(&endvisible);
			DBG_DELAYSCANNING("not idle, call scan for visible area\n");
			/* hmm spell checking should always be delayed, right? we should
			   rewrite this bit such that we always schedule spell checking in 
			   the timeout function */
			if (!bftextview2_run_scanner(btv, &endvisible)
#ifdef HAVE_LIBENCHANT
				&& !btv->spell_check
#endif
				) {
				DBG_DELAYSCANNING("finished scanning, remove callbacks\n");
				if (in_idle && btv->scanner_delayed) {
					g_source_remove(btv->scanner_delayed);
				} else if (!in_idle && btv->scanner_idle) {
					g_source_remove(btv->scanner_idle);
				}
				btv->scanner_delayed = 0;
				btv->scanner_idle = 0;
				bftextview2_set_margin_size(btv);
				DBG_DELAYSCANNING
					("scanning was finished, so return FALSE (scanned_delayed=%d, scanner_idle=%d)\n",
					 btv->scanner_delayed, btv->scanner_idle);
				return FALSE;
			}
			if (in_idle) {
				/* don't call idle function again, but call timeout function */
				if (!btv->scanner_delayed) {
					DBG_DELAYSCANNING("schedule delayed scanning\n");
					btv->scanner_delayed =
						g_timeout_add(main_v->props.delay_scan_time, bftextview2_scanner_timeout, btv);
				} else {
					DBG_DELAYSCANNING("delayed scanning already scheduled\n");
				}
				btv->scanner_idle = 0;
				DBG_DELAYSCANNING
					("return FALSE for this idle (in_idle=%d) callback (scanned_delayed=%d, scanner_idle=%d)\n",
					 in_idle, btv->scanner_delayed, btv->scanner_idle);
				return FALSE;
			}
			DBG_DELAYSCANNING
				("return TRUE, call timeout again (in_idle=%d, scanned_delayed=%d, scanner_idle=%d)\n",
				 in_idle, btv->scanner_delayed, btv->scanner_idle);
			return TRUE;		/* call timeout function again */
		}
	} else {					/* no delayed scanning, run everything in the idle callback */
		DBG_SIGNALS("bftextview2_scanner_idle, running scanner idle function\n");
		if (!bftextview2_run_scanner(btv, NULL)
#ifdef HAVE_LIBENCHANT
			&& !bftextview2_run_spellcheck(btv)
#endif
			) {
			btv->scanner_idle = 0;
			DBG_SIGNALS("bftextview2_scanner_idle, stopping scanner idle function\n");
			bftextview2_set_margin_size(btv);
			return FALSE;
		}
	}
	DBG_DELAYSCANNING("end of function, return TRUE\n");
	return TRUE;				/* call me again */
}

static gboolean bftextview2_scanner_idle(gpointer data)
{
	DBG_MSG("bftextview2_scanner_idle callback started\n");
	if (!((BluefishTextView *) data)->enable_scanner)
		return FALSE;
	return bftextview2_scanner_scan((BluefishTextView *) data, TRUE);
}

static gboolean bftextview2_scanner_timeout(gpointer data)
{
	if (!((BluefishTextView *) data)->enable_scanner)
		return FALSE;
	DBG_DELAYSCANNING("bftextview2_scanner_timeout callback started\n");
	return bftextview2_scanner_scan((BluefishTextView *) data, FALSE);
}

void bftextview2_schedule_scanning(BluefishTextView * btv)
{
	DBG_MSG("bftextview2_schedule_scanning, enable=%d, bflang=%p,scanner_idle=%d\n", BLUEFISH_TEXT_VIEW(btv->master)->enable_scanner,
			BLUEFISH_TEXT_VIEW(btv->master)->bflang, BLUEFISH_TEXT_VIEW(btv->master)->scanner_idle);
	if (BLUEFISH_TEXT_VIEW(btv->master)->enable_scanner && BLUEFISH_TEXT_VIEW(btv->master)->bflang && BLUEFISH_TEXT_VIEW(btv->master)->scanner_idle == 0) {
		DBG_DELAYSCANNING("scheduling scanning in idle function with priority %d\n", SCANNING_IDLE_PRIORITY);
		BLUEFISH_TEXT_VIEW(btv->master)->scanner_idle = g_idle_add_full(SCANNING_IDLE_PRIORITY, bftextview2_scanner_idle, btv->master, NULL);
	}
}

static void bftextview2_get_iters_at_foundblock(GtkTextBuffer * buffer, Tfoundblock * fblock,
												GtkTextIter * it1, GtkTextIter * it2,
												GtkTextIter * it3, GtkTextIter * it4)
{
	gtk_text_buffer_get_iter_at_offset(buffer, it1, fblock->start1_o);
	gtk_text_buffer_get_iter_at_offset(buffer, it2, fblock->end1_o);
	gtk_text_buffer_get_iter_at_offset(buffer, it3, fblock->start2_o);
	gtk_text_buffer_get_iter_at_offset(buffer, it4, fblock->end2_o);
}

static inline Tfoundblock *bftextview2_get_block_at_offset(BluefishTextView * btv, guint offset)
{
	Tfoundstack *fstack;
	GSequenceIter *siter;
	fstack = get_stackcache_at_offset(btv, offset, &siter);
	while (fstack) {
		/*g_print("check fstack %p with charoffset_o=%d\n",fstack,fstack->charoffset_o); */
		if (fstack->pushedblock
			&& (fstack->pushedblock->start1_o == offset || fstack->pushedblock->end1_o == offset)) {
			return fstack->pushedblock;
		} else if (fstack->poppedblock
				   && (fstack->poppedblock->start2_o == offset || fstack->poppedblock->end2_o == offset)) {
			return fstack->poppedblock;
		}
		if (fstack->charoffset_o > offset)
			break;
		fstack = get_stackcache_next(btv, &siter);
	}
	return NULL;
}

static gboolean mark_set_idle_lcb(gpointer widget)
{
	BluefishTextView *btv = widget;

	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
	GtkTextIter it1, it2, location;
	Tfoundblock *fblock;

	gtk_text_buffer_get_iter_at_mark(buffer, &location, gtk_text_buffer_get_insert(buffer));
	fblock = bftextview2_get_block_at_offset(btv, gtk_text_iter_get_offset(&location));
	if (btv->showing_blockmatch) {
		gtk_text_buffer_get_bounds(buffer, &it1, &it2);
		gtk_text_buffer_remove_tag(buffer, BLUEFISH_TEXT_VIEW(widget)->blockmatch, &it1, &it2);
		btv->showing_blockmatch = FALSE;
	}
	DBG_SIGNALS("mark_set_idle_lcb, 'insert' set at %d\n", gtk_text_iter_get_offset(&location));
	if (fblock) {
		GtkTextIter it3, it4;
		if (fblock->start2_o != BF2_OFFSET_UNDEFINED) {
			bftextview2_get_iters_at_foundblock(buffer, fblock, &it1, &it2, &it3, &it4);
			DBG_MSG("mark_set_idle_lcb, found a block to highlight the start (%d:%d) and end (%d:%d)\n",
					gtk_text_iter_get_offset(&it1), gtk_text_iter_get_offset(&it2),
					gtk_text_iter_get_offset(&it3), gtk_text_iter_get_offset(&it4));
			gtk_text_buffer_apply_tag(buffer, btv->blockmatch, &it1, &it2);
			gtk_text_buffer_apply_tag(buffer, btv->blockmatch, &it3, &it4);
			btv->showing_blockmatch = TRUE;
		} else {
			DBG_MSG("mark_set_idle_lcb, block has no end - no matching\n");
		}
	}

	btv->mark_set_idle = 0;
	return FALSE;
}

/* this function slows down scrolling when you hold the cursor pressed, because 
it is called for every cursor position change. This function is therefore
an ideal candidate for speed optimization */
static void bftextview2_mark_set_lcb(GtkTextBuffer * buffer, GtkTextIter * location,
									 GtkTextMark * arg2, gpointer widget)
{
	BluefishTextView *btv = BLUEFISH_TEXT_VIEW(widget);
	DBG_SIGNALS("bftextview2_mark_set_lcb\n");
	if (arg2 && gtk_text_buffer_get_insert(buffer) == arg2) {
		
		if (btv->autocomp)
			autocomp_stop(btv);

		if (btv->show_mbhl)
			btv->needs_blockmatch = TRUE;

		if (btv->bflang && btv->bflang->st) {
			if (btv->user_idle) {
				g_source_remove(btv->user_idle);
				btv->user_idle = 0;
			}
		}
	}
}

static void bftextview2_set_margin_size(BluefishTextView * btv)
{
	/* TODO: this should be calculated based on the number of lines in the text,
	   whether or not we have bookmarks, and whether or not we have block folding */
	gint lines, count, newsize;
	DBG_MSG("bftextview2_set_margin_size, called for %p\n",btv);
	if (BLUEFISH_TEXT_VIEW(btv->master)->show_line_numbers) {
		PangoLayout *panlay;
		lines = gtk_text_buffer_get_line_count(gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv)));
		if (BLUEFISH_TEXT_VIEW(btv->master)->margin_pixels_per_char == 0) {
			panlay = gtk_widget_create_pango_layout(GTK_WIDGET(btv), "");
			pango_layout_set_text(panlay, "4", -1);
			pango_layout_get_pixel_size(panlay, &btv->margin_pixels_per_char, NULL);
			g_object_unref(G_OBJECT(panlay));
		}
		if (lines >= 100)
			count = 1 + log10(lines);
		else
			count = 2;
		BLUEFISH_TEXT_VIEW(btv->master)->margin_pixels_chars = 4 + count * BLUEFISH_TEXT_VIEW(btv->master)->margin_pixels_per_char;
	} else {
		BLUEFISH_TEXT_VIEW(btv->master)->margin_pixels_chars = 0;
	}
	if (BLUEFISH_TEXT_VIEW(btv->master)->show_blocks && g_sequence_get_length(BLUEFISH_TEXT_VIEW(btv->master)->scancache.stackcaches) > 0) {
		BLUEFISH_TEXT_VIEW(btv->master)->margin_pixels_block = 12;
	} else {
		BLUEFISH_TEXT_VIEW(btv->master)->margin_pixels_block = 0;
	}
	if (BLUEFISH_TEXT_VIEW(btv->master)->showsymbols) {
		BLUEFISH_TEXT_VIEW(btv->master)->margin_pixels_symbol = 12;
	} else {
		BLUEFISH_TEXT_VIEW(btv->master)->margin_pixels_symbol = 0;
	}
	/*g_print("lines=%d,count=%d,pixels_per_char=%d\n",lines,count,btv->margin_pixels_per_char); */
	newsize = BLUEFISH_TEXT_VIEW(btv->master)->margin_pixels_chars 
										 + BLUEFISH_TEXT_VIEW(btv->master)->margin_pixels_block 
										 + BLUEFISH_TEXT_VIEW(btv->master)->margin_pixels_symbol;
	gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(btv), GTK_TEXT_WINDOW_LEFT,newsize);
	if (btv->slave)
		gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(btv->slave), GTK_TEXT_WINDOW_LEFT,newsize);
}

static inline gboolean char_in_allsymbols(BluefishTextView * btv, gunichar uc)
{
	if (uc > 127)
		return FALSE;
	if (!btv->bflang)
		return FALSE;
	if (btv->bflang->st)
		return btv->bflang->st->allsymbols[uc];
	else
		return (uc == ' ' || uc == '\t' || uc == '\n');
	return FALSE;
}

static void bftextview2_insert_text_lcb(GtkTextBuffer * buffer, GtkTextIter * iter, gchar * string,
										gint stringlen, BluefishTextView * btv)
{
	DBG_SIGNALS("bftextview2_insert_text_lcb, btv=%p, master=%p\n",btv,btv->master);
	if (btv==btv->master)
		stackcache_update_offsets(BLUEFISH_TEXT_VIEW(btv->master), gtk_text_iter_get_offset(iter), g_utf8_strlen(string, stringlen));
}

static void bftextview2_insert_text_after_lcb(GtkTextBuffer * buffer, GtkTextIter * iter, gchar * string,
											  gint stringlen, BluefishTextView * btv)
{
	GtkTextIter start, end;
	/*gint start_offset; */
	DBG_SIGNALS("bftextview2_insert_text_after_lcb, btv=%p, master=%p, stringlen=%d\n", btv, btv->master, stringlen);

	if (BLUEFISH_TEXT_VIEW(btv->master)->enable_scanner && btv->needs_autocomp && BLUEFISH_TEXT_VIEW(btv->master)->auto_complete && stringlen == 1
		&& (btv->autocomp || main_v->props.autocomp_popup_mode != 0)) {
		DBG_AUTOCOMP("bftextview2_insert_text_after_lcb: call autocomp_run\n");
		autocomp_run(btv, FALSE);
		DBG_AUTOCOMP("bftextview2_insert_text_after_lcb, set needs_autocomp to FALSE\n");
		btv->needs_autocomp = FALSE;
	}

	bftextview2_reset_user_idle_timer(btv);
	bftextview2_set_margin_size(BLUEFISH_TEXT_VIEW(btv->master));

	if (btv!=btv->master)
		return;

	if (!main_v->props.reduced_scan_triggers || stringlen > 1
		|| (stringlen == 1 && char_in_allsymbols(btv, string[0]))) {
		bftextview2_schedule_scanning(btv);
	}
	/* mark the text that is changed */
	end = start = *iter;
	gtk_text_iter_backward_chars(&start, stringlen);

	DBG_SIGNALS("bftextview2_insert_text_after_lcb: mark text from %d to %d as needscanning %p\n",
				gtk_text_iter_get_offset(&start), gtk_text_iter_get_offset(iter), btv->needscanning);
	gtk_text_buffer_apply_tag(buffer, btv->needscanning, &start, iter);
	btv->needremovetags = TRUE;
	/*start_offset = gtk_text_iter_get_offset(&start); */

#ifdef HAVE_LIBENCHANT
	DBG_SPELL("bftextview2_insert_text_after_lcb, mark area from %d to %d with tag 'needspellcheck' %p\n",
			  gtk_text_iter_get_offset(&start), gtk_text_iter_get_offset(iter), btv->needspellcheck);
	gtk_text_buffer_apply_tag(buffer, btv->needspellcheck, &start, &end);
#endif							/*HAVE_LIBENCHANT */

}

/*static void print_fstack(Tfoundstack * fstack)
{
	DBG_MARGIN("got fstack %p for next position", fstack);
	if (fstack) {
		DBG_MARGIN(" with line %d and charoffset %d and %d blocks", fstack->line, fstack->charoffset,
				g_queue_get_length(fstack->blockstack));
	}
	DBG_MARGIN("\n");
}*/

static inline void paint_margin_expand(BluefishTextView * btv, cairo_t * cr, gint w, gint height)
{
	gdk_cairo_set_source_color(cr, &gtk_widget_get_style(GTK_WIDGET(btv))->base[gtk_widget_get_state(GTK_WIDGET(btv))]);
	cairo_rectangle(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 2, w + (height / 2) - 3, 7, 7);
	cairo_fill(cr);
	gdk_cairo_set_source_color(cr, &gtk_widget_get_style(GTK_WIDGET(btv))->fg[gtk_widget_get_state(GTK_WIDGET(btv))]);
	cairo_rectangle(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 1.5, w + (height / 2) - 3.5, 8,
					8);
	cairo_move_to(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 5.5, w + (height / 2) + 5);
	cairo_line_to(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 5.5, w + height + 0.5);
	cairo_move_to(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 3, w + (height / 2) + 0.5);
	cairo_line_to(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 8, w + (height / 2) + 0.5);
	cairo_stroke(cr);
}

static inline void paint_margin_collapse(BluefishTextView * btv, cairo_t * cr, gint w, gint height)
{
	gdk_cairo_set_source_color(cr, &gtk_widget_get_style(GTK_WIDGET(btv))->base[gtk_widget_get_state(GTK_WIDGET(btv))]);
	cairo_rectangle(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 2, w + (height / 2) - 3, 7, 7);
	cairo_fill(cr);
	gdk_cairo_set_source_color(cr, &gtk_widget_get_style(GTK_WIDGET(btv))->fg[gtk_widget_get_state(GTK_WIDGET(btv))]);
	cairo_rectangle(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 1.5, w + (height / 2) - 3.5, 8,
					8);
	cairo_move_to(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 5.5, w + (height / 2) - 2);
	cairo_line_to(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 5.5, w + (height / 2) + 3);
	cairo_move_to(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 5.5, w + (height / 2) + 5);
	cairo_line_to(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 5.5, w + height + 0.5);
	cairo_move_to(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 3, w + (height / 2) + 0.5);
	cairo_line_to(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 8, w + (height / 2) + 0.5);
	cairo_stroke(cr);
}

static inline void paint_margin_blockend(BluefishTextView * btv, cairo_t * cr, gint w, gint height)
{
	/*gdk_draw_line(GDK_DRAWABLE(event->window),GTK_WIDGET(btv)->style->fg_gc[gtk_widget_get_state(GTK_WIDGET(btv))],btv->margin_pixels_chars+btv->margin_pixels_symbol+5, w, btv->margin_pixels_chars+btv->margin_pixels_symbol+5, w + (height/2));
	   gdk_draw_line(GDK_DRAWABLE(event->window),GTK_WIDGET(btv)->style->fg_gc[gtk_widget_get_state(GTK_WIDGET(btv))],btv->margin_pixels_chars+btv->margin_pixels_symbol+5, w+(height/2), btv->margin_pixels_chars+btv->margin_pixels_symbol+8, w + (height/2)); */
	cairo_move_to(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 5.5, w);
	cairo_line_to(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 5.5, w + (height / 2) + 0.5);
	cairo_move_to(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 5.5, w + (height / 2) + 0.5);
	cairo_line_to(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 8.5, w + (height / 2) + 0.5);
	cairo_stroke(cr);
}

static inline void paint_margin_line(BluefishTextView * btv, cairo_t * cr, gint w, gint height)
{
/*	gdk_draw_line(GDK_DRAWABLE(event->window),GTK_WIDGET(btv)->style->fg_gc[gtk_widget_get_state(GTK_WIDGET(btv))],btv->margin_pixels_chars+btv->margin_pixels_symbol+5, w, btv->margin_pixels_chars+btv->margin_pixels_symbol+5, w + height);*/
	cairo_move_to(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 5.5, w);
	cairo_line_to(cr, btv->margin_pixels_chars + btv->margin_pixels_symbol + 5.5, w + height);
	cairo_stroke(cr);
}

static inline void paint_margin_symbol(BluefishTextView * btv, cairo_t * cr, gint w, gint height)
{
	cairo_rectangle(cr, btv->margin_pixels_chars + 2, w + (height / 2) - 4, 8, 8);
	cairo_fill(cr);
}

static gint get_num_foldable_blocks(Tfoundstack * fstack)
{
	guint count = 0;
	GList *tmplist = fstack->blockstack->head;
	while (tmplist) {
		if (((Tfoundblock *) (tmplist->data))->foldable)
			count++;
		tmplist = g_list_next(tmplist);
	}
	return count;
}

static inline void paint_margin(BluefishTextView * btv, GdkEventExpose * event, GtkTextIter * startvisible,
								GtkTextIter * endvisible)
{
	Tfoundstack *fstack = NULL;
	BluefishTextView *master=btv->master;
	GSequenceIter *siter = NULL;
	guint num_blocks;
	gint cursor_line = -1;
	GtkTextIter it;
	GtkTextTag *folded;
	gint i;
	PangoLayout *panlay;
	gpointer bmark;
	gint bmarkline = -1;
	cairo_t *cr;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
	DBG_MSG("paint_margin called for %p\n",btv);
	cr = gdk_cairo_create(event->window);
	cairo_set_line_width(cr, 1);
	gdk_cairo_set_source_color(cr, &gtk_widget_get_style(GTK_WIDGET(btv))->fg[gtk_widget_get_state(GTK_WIDGET(btv))]);

	if (master->show_line_numbers) {
		GtkTextIter cursorit;
		gtk_text_buffer_get_iter_at_mark(buffer, &cursorit,
										 gtk_text_buffer_get_insert(buffer));
		cursor_line = gtk_text_iter_get_line(&cursorit);
	}

	/* to see how many blocks are active here */
	if (G_UNLIKELY(gtk_text_iter_is_start(startvisible) && (g_sequence_get_length(master->scancache.stackcaches) != 0))) {
		siter = g_sequence_get_begin_iter(master->scancache.stackcaches);
		if (!g_sequence_iter_is_end(siter)) {
			fstack = g_sequence_get(siter);
		}
		num_blocks = 0;
		DBG_MARGIN("EXPOSE: start at begin, set num_blocks %d, fstack=%p\n", num_blocks, fstack);
	} else {
		fstack = get_stackcache_at_offset(master, gtk_text_iter_get_offset(startvisible), &siter);
		if (fstack) {
			num_blocks = get_num_foldable_blocks(fstack);
		} else {
			DBG_MARGIN("EXPOSE: no fstack for position %d, siter=%p\n",
					   gtk_text_iter_get_offset(startvisible), siter);
			num_blocks = 0;
		}
	}
	/* in the case that the *first* fstack is relevant, we don't need
	   the 'next' fstack */
	if (!fstack || fstack->charoffset_o < gtk_text_iter_get_offset(startvisible)) {
		DBG_MARGIN("get next fstack..\n");
		if (siter)
			fstack = get_stackcache_next(master, &siter);
	}
	/*DBG_MARGIN("first fstack ");
	   print_fstack(fstack); */

	it = *startvisible;
	panlay = gtk_widget_create_pango_layout(GTK_WIDGET(btv), "x");

	folded =	gtk_text_tag_table_lookup(langmgr_get_tagtable(),"_folded_");
	if (master->showsymbols) {
		bmarkline = bmark_margin_get_initial_bookmark((Tdocument *) master->doc, startvisible, &bmark);
	}

	for (i = gtk_text_iter_get_line(startvisible); i <= gtk_text_iter_get_line(endvisible); i++) {
		gint w, height;
		gchar *string;


		gtk_text_iter_set_line(&it, i);

		if (G_UNLIKELY(gtk_text_iter_has_tag(&it, folded))) {
			DBG_FOLD("line %d is hidden\n", i);
		} else {
			gtk_text_view_get_line_yrange(GTK_TEXT_VIEW(btv), &it, &w, &height);
			gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(btv), GTK_TEXT_WINDOW_LEFT, 0, w, NULL, &w);

			/* line numbers */
			if (master->show_line_numbers) {
				if (i == cursor_line)
					string = g_strdup_printf("<b>%d</b>", 1 + i);
				else
					string = g_strdup_printf("%d", 1 + i);
				pango_layout_set_markup(panlay, string, -1);
				cairo_move_to(cr, 2, w);
				pango_cairo_show_layout(cr, panlay);
				g_free(string);
			}
			/* symbols */
			if (master->showsymbols && bmarkline != -1) {
				while (bmarkline != -1 && bmarkline < i) {
					bmarkline = bmark_margin_get_next_bookmark((Tdocument *) master->doc, &bmark);
				}
				if (G_UNLIKELY(bmarkline == i)) {
					paint_margin_symbol(master, cr, w, height);
				}
			}
			/* block folding.
			   - to find out if we need an expander/collapse, we need to see if there is a pushedblock on the
			   blockstack which has 'foldable' for any stackcache that is on this line.
			   - to find if we need an end-of-block we need to see if there is a poppedblock on this line
			   which has 'foldable'
			   - to find out if we need a line or nothing we need to know the number of expanded blocks on the stack
			 */
			if (master->show_blocks) {
				GtkTextIter nextline;
				guint nextline_o, curline_o;
				curline_o = gtk_text_iter_get_offset(&it);
				nextline = it;
				if (!gtk_text_iter_ends_line(&nextline)) {
					gtk_text_iter_forward_to_line_end(&nextline);
				}
				nextline_o = gtk_text_iter_get_offset(&nextline);
				while (fstack) {
					guint fstackpos = fstack->charoffset_o;
					if (fstack->pushedblock) {
						/* on a pushedblock we should look where the block match start, charoffset_o is the end of the 
						   match, so multiline patterns are drawn on the wrong line */
						fstackpos = fstack->pushedblock->start1_o;
					}
					/*DBG_FOLD("search block at line %d, curline_o=%d, nextline_o=%d\n",i,curline_o,nextline_o); */
					if (fstackpos > nextline_o) {
						if (num_blocks > 0) {
							paint_margin_line(master, cr, w, height);
						}
						break;
					}
					if (fstackpos <= nextline_o && fstackpos >= curline_o) {
						if (fstack->pushedblock && fstack->pushedblock->foldable) {
							if (fstack->pushedblock->folded)
								paint_margin_collapse(master, cr, w, height);
							else
								paint_margin_expand(master, cr, w, height);

							num_blocks = get_num_foldable_blocks(fstack);
							break;
						} else if (fstack->poppedblock && fstack->poppedblock->foldable) {
							paint_margin_blockend(master, cr, w, height);
							num_blocks = get_num_foldable_blocks(fstack);
							break;
						}
					}
					fstack = get_stackcache_next(master, &siter);
				}
			}
		}
	}
	cairo_destroy(cr);
	g_object_unref(G_OBJECT(panlay));
}

/* whitespace macro. Possibly include: '/n', 8206-8207, maybe others */
#define BTV_ISWS(c) ( \
  ((c) == '\t') || \
  ((c) == ' ') || \
  ((c) == 160) || \
  ((c) == 8239) || \
  ((c) == 12288) || \
  ((c) >= 8192 && (c) <= 8205) \
)
/*
main_v->props.visible_ws_mode:
0 = All
1 = All except spaces
2 = All trailing
3 = All except non-trailing spaces 
*/
static inline void paint_spaces(BluefishTextView * btv, GdkEventExpose * event, GtkTextIter * startvisible,
								GtkTextIter * endvisible)
{
	GtkTextIter iter;
	cairo_t *cr;
	gunichar uc;
	gboolean trailing = FALSE;

	cr = gdk_cairo_create(event->window);
	cairo_set_line_width(cr, 1.0);
	gdk_cairo_set_source_color(cr, &st_whitespace_color);

	iter = *endvisible;
	if (!gtk_text_iter_ends_line(&iter))
		gtk_text_iter_forward_to_line_end(&iter);

	while (!gtk_text_iter_equal(&iter, startvisible)) {	/* equal is faster than compare */
		GdkRectangle rect;
		gint x, y;
		uc = gtk_text_iter_get_char(&iter);
		if (G_UNLIKELY(BTV_ISWS(uc))) {
			gtk_text_view_get_iter_location(GTK_TEXT_VIEW(btv), &iter, &rect);
			gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(btv), GTK_TEXT_WINDOW_TEXT, rect.x,
												  rect.y + rect.height / 1.5, &x, &y);
			if (uc == '\t' && (trailing || main_v->props.visible_ws_mode != 2)) {
				/* draw tab */
				cairo_move_to(cr, x + 3.5, y - 2.5);
				cairo_rel_line_to(cr, 0, 3);
				cairo_rel_line_to(cr, rect.width - 6, 0);
				cairo_rel_line_to(cr, 0, -3);
			} else if ((uc == 160 || uc == 8239) && (trailing || main_v->props.visible_ws_mode != 2)) {
				/* draw nbsp (8239= narrow-nbsp) */
				cairo_move_to(cr, x + 1, y - 0.5);
				cairo_rel_line_to(cr, rect.width - 2, 0);
			} else if (main_v->props.visible_ws_mode == 0 || (main_v->props.visible_ws_mode != 1 && trailing)) {
				/* draw space */
				x += rect.width / 2;
				cairo_move_to(cr, x, y);
				cairo_arc(cr, x, y, 0.75, 0, 2 * M_PI);
			}
		} else if (G_UNLIKELY(uc != '\n' && uc != '\r')) {
			trailing = FALSE;
		}

		if (G_UNLIKELY(gtk_text_iter_starts_line(&iter))) {
			trailing = TRUE;
		}
		gtk_text_iter_backward_char(&iter);
	}
	cairo_stroke(cr);
	cairo_destroy(cr);
}

static gboolean bluefish_text_view_expose_event(GtkWidget * widget, GdkEventExpose * event)
{
	BluefishTextView *btv = BLUEFISH_TEXT_VIEW(widget);
	BluefishTextView *master = BLUEFISH_TEXT_VIEW(btv->master);
	gboolean event_handled = FALSE;

	if (event->window == gtk_text_view_get_window(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_LEFT)) {
		GtkTextIter startvisible, endvisible;
		GdkRectangle rect;
		DBG_SIGNALS("bluefish_text_view_expose_event, GTK_TEXT_WINDOW_LEFT\n");

		gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(widget), &rect);
		gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(widget), &startvisible, rect.y, NULL);
		gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(widget), &endvisible, rect.y + rect.height, NULL);

		paint_margin(btv, event, &startvisible, &endvisible);
		event_handled = TRUE;
	} else {
		if (gtk_widget_is_sensitive(GTK_WIDGET(btv))
			&& (event->window == gtk_text_view_get_window(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT))
			&& (BFWIN(DOCUMENT(master->doc)->bfwin)->session->view_cline)) {
			GdkRectangle rect;
			gint w, w2;
			cairo_t *context;
			GtkTextIter it;
			GtkTextBuffer *buffer;
			DBG_SIGNALS("bluefish_text_view_expose_event, current line highlighting\n");
			buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
			gtk_text_buffer_get_iter_at_mark(buffer, &it, gtk_text_buffer_get_insert(buffer));
			gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(widget), &rect);
			gtk_text_view_get_line_yrange(GTK_TEXT_VIEW(widget), &it, &w, &w2);
			gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT,
												  rect.x, rect.y, &rect.x, &rect.y);
			gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT,
												  0, w, NULL, &w);
			context = gdk_cairo_create(event->window);
			gdk_cairo_set_source_color(context, &st_cline_color);
			cairo_set_line_width(context, 1);
			cairo_rectangle(context, rect.x + .5, w + .5, rect.width - 1, w2 - 1);
			cairo_stroke_preserve(context);
			cairo_fill(context);
			cairo_destroy(context);
		}

		if (GTK_WIDGET_CLASS(bluefish_text_view_parent_class)->expose_event)
			event_handled = GTK_WIDGET_CLASS(bluefish_text_view_parent_class)->expose_event(widget, event);

		if (event->window == gtk_text_view_get_window(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT)) {
			if (master->visible_spacing) {
				GtkTextIter startvisible, endvisible;
				GdkRectangle rect;
				DBG_SIGNALS("bluefish_text_view_expose_event, paint visible spacing\n");

				gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(widget), &rect);
				gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(widget), &startvisible, rect.y, NULL);
				gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(widget), &endvisible, rect.y + rect.height, NULL);
				paint_spaces(btv, event, &startvisible, &endvisible);
			}

			if (master->show_right_margin) {
				GdkRectangle rect, rect2;
				cairo_t *cr;
				guint pix = master->margin_pixels_per_char * main_v->props.right_margin_pos;
				gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(widget), &rect);
				rect2 = rect;
				gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT, rect.x,
													  rect.y, &rect2.x, &rect2.y);
				cr = gdk_cairo_create(gtk_text_view_get_window(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT));
				cairo_set_line_width(cr, 1.0);	/* 1.0 looks the best, smaller gives a half-transparent color */
				cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
				cairo_clip(cr);
				cairo_move_to(cr, pix, rect2.y);
				cairo_line_to(cr, pix, rect2.y + rect2.height);
				cairo_stroke(cr);
				cairo_destroy(cr);
			}
		}
	}
	return event_handled;
}

static void bftextview2_delete_range_lcb(GtkTextBuffer * buffer, GtkTextIter * obegin,
										 GtkTextIter * oend, gpointer user_data)
{
	gint loop;
	GtkTextIter begin = *obegin, end = *oend;
	BluefishTextView *btv = user_data;
	DBG_SIGNALS("bftextview2_delete_range_lcb\n");
	
	if (btv->master != btv) {
		bftextview2_reset_user_idle_timer(btv);
		return;
	}
	
	loop = gtk_text_iter_get_offset(obegin);	/* re-use the loop variable */
	DBG_SCANCACHE("bftextview2_delete_range_lcb, delete from %d to %d\n", gtk_text_iter_get_offset(obegin),
				  gtk_text_iter_get_offset(oend));
	stackcache_update_offsets(BLUEFISH_TEXT_VIEW(btv->master), loop, loop - gtk_text_iter_get_offset(oend));

	/* mark the surroundings of the text that will be deleted */

	/* the 'word start' algorithm of pango becomes very slow in a situation where 
	   the file is filled with funny unicode characters such as 'box' symbol characters. 
	   This happens in search and replace with many replaces (this function is called for
	   each replace).  
	   I have to see why this is. We could also mark from the beginning of the line, but that
	   would be excessive on very long lines...... what is best?? */
	loop = 0;
	while (loop < 32 && gtk_text_iter_backward_char(&begin)
		   && !g_unichar_isspace(gtk_text_iter_get_char(&begin)))
		loop++;
	loop = 0;
	while (loop < 32 && gtk_text_iter_forward_char(&end) && !g_unichar_isspace(gtk_text_iter_get_char(&end)))
		loop++;
	/*gtk_text_iter_backward_word_start(&begin);
	   gtk_text_iter_forward_word_end(&end); */
	gtk_text_buffer_apply_tag(buffer, btv->needscanning, &begin, &end);
	btv->needremovetags = TRUE;
	DBG_SIGNALS("mark text from %d to %d as needscanning\n", gtk_text_iter_get_offset(&begin),
				gtk_text_iter_get_offset(&end));
#ifdef HAVE_LIBENCHANT
	gtk_text_buffer_apply_tag(buffer, btv->needspellcheck, &begin, &end);
	DBG_SPELL("mark text from %d to %d as needspellcheck\n", gtk_text_iter_get_offset(&begin),
			  gtk_text_iter_get_offset(&end));
#endif							/*HAVE_LIBENCHANT */
	bftextview2_reset_user_idle_timer(btv);
	bftextview2_schedule_scanning(btv);
}

static void bftextview2_delete_range_after_lcb(GtkTextBuffer * buffer, GtkTextIter * obegin,
											   GtkTextIter * oend, gpointer user_data)
{
	BluefishTextView *btv = user_data;
	DBG_SIGNALS("bftextview2_delete_range_after_lcb, btv=%p, master=%p, needs_autocomp=%d\n",btv, btv->master, btv->needs_autocomp);
	if (BLUEFISH_TEXT_VIEW(btv->master)->enable_scanner && btv->needs_autocomp && BLUEFISH_TEXT_VIEW(btv->master)->auto_complete
		&& (btv->autocomp || main_v->props.autocomp_popup_mode != 0)) {
		autocomp_run(btv, FALSE);
		DBG_AUTOCOMP("bftextview2_delete_range_after_lcb, set needs_autocomp to FALSE\n");
		btv->needs_autocomp = FALSE;
	}
}

static gboolean bluefish_text_view_key_press_event(GtkWidget * widget, GdkEventKey * kevent)
{
	gboolean retval;
	BluefishTextView *btv = BLUEFISH_TEXT_VIEW(widget);
	DBG_SIGNALS("bluefish_text_view_key_press_event, keyval=%d\n", kevent->keyval);
	if (btv->autocomp) {
		if (acwin_check_keypress(btv, kevent)) {
			btv->key_press_inserted_char = FALSE;
			return TRUE;
		}
	}
	if (BLUEFISH_TEXT_VIEW(btv->master)->enable_scanner && (kevent->state & main_v->autocomp_accel_mods) && kevent->keyval == main_v->autocomp_accel_key) {
		/* <ctrl><space> manually opens the auto completion */
		autocomp_run(btv, TRUE);
		return TRUE;
	}
	DBG_AUTOCOMP("bluefish_text_view_key_press_event, set needs_autocomp to TRUE\n");
	btv->needs_autocomp = TRUE;

	if (main_v->props.editor_smart_cursor && !(kevent->state & GDK_CONTROL_MASK)
		&& ((kevent->keyval == GDK_Home) || (kevent->keyval == GDK_KP_Home) || (kevent->keyval == GDK_End)
			|| (kevent->keyval == GDK_KP_End))) {
		GtkTextMark *imark;
		gboolean ret;
		GtkTextIter iter, currentpos, linestart;
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));


		imark = gtk_text_buffer_get_insert(buffer);
		gtk_text_buffer_get_iter_at_mark(buffer, &currentpos, imark);
		iter = currentpos;

		if ((kevent->keyval == GDK_Home) || (kevent->keyval == GDK_KP_Home)) {
			/*gtk_text_iter_backward_cursor_positions(&iter, gtk_text_iter_get_line_offset(&iter));
			   linestart = iter;

			   while (g_unichar_isspace(gtk_text_iter_get_char (&iter)) && !gtk_text_iter_ends_line(&iter))
			   gtk_text_iter_forward_char (&iter); */
			ret = bf_text_iter_line_start_of_text(&iter, &linestart);
		} else {				/* (kevent->keyval == GDK_End) || (kevent->keyval == GDK_KP_End) */
			/*if (!gtk_text_iter_ends_line(&iter))
			   gtk_text_iter_forward_to_line_end(&iter);
			   linestart = iter;
			   if (gtk_text_iter_is_end (&iter) && !gtk_text_iter_starts_line (&iter))
			   gtk_text_iter_backward_char(&iter);
			   while (g_unichar_isspace (gtk_text_iter_get_char (&iter)) && !gtk_text_iter_starts_line (&iter))
			   gtk_text_iter_backward_char(&iter);
			   if ((!gtk_text_iter_starts_line (&iter) || !gtk_text_iter_ends_line (&iter)) && !g_unichar_isspace (gtk_text_iter_get_char (&iter)))
			   gtk_text_iter_forward_char(&iter); */
			ret = bf_text_iter_line_end_of_text(&iter, &linestart);
		}
		if (ret) {
			if (gtk_text_iter_compare(&currentpos, &iter) == 0)
				iter = linestart;
			if (kevent->state & GDK_SHIFT_MASK)
				gtk_text_buffer_move_mark(buffer, imark, &iter);
			else
				gtk_text_buffer_place_cursor(buffer, &iter);
			gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(btv), gtk_text_buffer_get_insert(buffer));
			return TRUE;
		}
	}
	if (main_v->props.editor_tab_indent_sel
		&& (kevent->keyval == GDK_Tab || kevent->keyval == GDK_KP_Tab || kevent->keyval == GDK_ISO_Left_Tab)
		&& (!(kevent->state & GDK_CONTROL_MASK))) {	/* shift-tab is also known as GDK_ISO_Left_Tab */
		GtkTextIter so, eo;
		gboolean have_selection;
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
		have_selection = gtk_text_buffer_get_selection_bounds(buffer, &so, &eo);
		if (have_selection) {
			GtkTextIter eol1, eol2, sol1, sol2;
			if (kevent->state & GDK_SHIFT_MASK) {
				/* unindent block */
				doc_indent_selection(btv->doc, TRUE);
				return TRUE;
			}
			if (gtk_text_iter_get_line(&so) != gtk_text_iter_get_line(&eo)) {
				doc_indent_selection(btv->doc, FALSE);
				return TRUE;
			}
			/* if the start and end are *around* the text (so either at the start or end or
			   in the indenting) we indent */
			eol1 = eo;
			sol2 = so;
			if (bf_text_iter_line_end_of_text(&eol1, &eol2) && bf_text_iter_line_start_of_text(&sol2, &sol1)) {
				if ((gtk_text_iter_equal(&so, &sol1) || gtk_text_iter_equal(&so, &sol2)
					 || gtk_text_iter_in_range(&so, &sol1, &sol2))
					&& (gtk_text_iter_equal(&eo, &eol1) || gtk_text_iter_equal(&eo, &eol2)
						|| gtk_text_iter_in_range(&eo, &eol1, &eol2))) {
					doc_indent_selection(btv->doc, FALSE);
					return TRUE;
				}
			}
		}
	}
	if (kevent->keyval == GDK_Tab && main_v->props.editor_indent_wspaces) {
		GtkTextBuffer *buffer = buffer;
		GtkTextMark *imark;
		GtkTextIter iter;
		gchar *string;
		gint numchars;
		/* replace the tab with spaces if the user wants that. 
		   However, some users want the tab key to arrive at the next tab stop. so if the tab width is 
		   4 and there are two characters already, bluefish should insert only 2 characters */
		string = bf_str_repeat(" ", BFWIN(DOCUMENT(btv->doc)->bfwin)->session->editor_tab_width);
		imark = gtk_text_buffer_get_insert(buffer);
		gtk_text_buffer_get_iter_at_mark(buffer, &iter, imark);
		numchars =
			BFWIN(DOCUMENT(btv->doc)->bfwin)->session->editor_tab_width -
			(gtk_text_iter_get_line_offset(&iter) %
			 BFWIN(DOCUMENT(btv->doc)->bfwin)->session->editor_tab_width);
		gtk_text_buffer_insert(buffer, &iter, string, numchars);
		g_free(string);
		return TRUE;
	}

	retval = GTK_WIDGET_CLASS(bluefish_text_view_parent_class)->key_press_event(widget, kevent);
	if (retval) {
		DBG_SIGNALS("parent handled the event, set key_press_inserted_char to TRUE\n");
		btv->key_press_inserted_char = TRUE;
	}
	return retval;
}

static void bftextview2_block_toggle_fold(BluefishTextView * btv, Tfoundblock * fblock)
{

	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
	GtkTextIter it1, it2, it3, it4;

	bftextview2_get_iters_at_foundblock(buffer, fblock, &it1, &it2, &it3, &it4);
	if (main_v->props.block_folding_mode == 1 && !gtk_text_iter_ends_line(&it2)
		&& !gtk_text_iter_starts_line(&it2)) {
		gtk_text_iter_forward_to_line_end(&it2);
	}
	if (gtk_text_iter_ends_line(&it4)) {
		gtk_text_iter_forward_line(&it4);
	}

	if (fblock->folded) {
		gtk_text_buffer_remove_tag_by_name(buffer, "foldheader", &it1, &it2);
		if (main_v->props.block_folding_mode == 0) {
			gtk_text_buffer_remove_tag_by_name(buffer, "_folded_", &it2, &it3);
			gtk_text_buffer_remove_tag_by_name(buffer, "foldheader", &it3, &it4);
		} else if (main_v->props.block_folding_mode == 1) {
			gtk_text_buffer_remove_tag_by_name(buffer, "_folded_", &it2, &it4);
		}
	} else {
		gtk_text_buffer_apply_tag_by_name(buffer, "foldheader", &it1, &it2);
		if (main_v->props.block_folding_mode == 0) {
			gtk_text_buffer_apply_tag_by_name(buffer, "_folded_", &it2, &it3);
			gtk_text_buffer_apply_tag_by_name(buffer, "foldheader", &it3, &it4);
		} else if (main_v->props.block_folding_mode == 1) {
			gtk_text_buffer_apply_tag_by_name(buffer, "_folded_", &it2, &it4);
		}
	}
	fblock->folded = (!fblock->folded);
}

static void bftextview2_toggle_fold(BluefishTextView * btv, GtkTextIter * iter)
{
	Tfoundstack *fstack;
	GSequenceIter *siter;
	GtkTextIter tmpiter;
	guint offset, nextline_o;

	if (!btv->bflang)
		return;
	tmpiter = *iter;
	gtk_text_iter_set_line_offset(&tmpiter, 0);
	offset = gtk_text_iter_get_offset(&tmpiter);
	if (!gtk_text_iter_ends_line(&tmpiter)) {
		gtk_text_iter_forward_to_line_end(&tmpiter);
	}
	nextline_o = gtk_text_iter_get_offset(&tmpiter);
	/* returns the fstack PRIOR to iter, or the fstack excactly at iter,
	   but this fails if the iter is the start of the buffer */
	fstack = get_stackcache_at_offset(btv, offset, &siter);
	if (!fstack) {
		/* is this 'if' block still required? I think get_stackcache_at_offset() now returns the first iter already */
		DBG_FOLD("no fstack, retrieve first iter\n");
		fstack = get_stackcache_first(btv, &siter);
	}
	while (fstack && fstack->charoffset_o < nextline_o) {
		if (fstack->pushedblock && fstack->pushedblock->foldable && fstack->pushedblock->start1_o >= offset)
			break;
		fstack = get_stackcache_next(btv, &siter);	/* should be the first fstack AFTER iter */
	}
	/*while (fstack && (fstack->charoffset_o < offset || !fstack->pushedblock || !fstack->pushedblock->foldable)) {
	   fstack = get_stackcache_next(btv, &siter); / * should be the first fstack AFTER iter * /
	   if (fstack && fstack->pushedblock && fstack->pushedblock->foldable)
	   break;
	   } */
	if (fstack && fstack->pushedblock && fstack->pushedblock->start1_o >= offset
		&& fstack->pushedblock->start1_o <= nextline_o && fstack->pushedblock->foldable) {
		DBG_FOLD("toggle fold on fstack=%p\n", fstack);
		bftextview2_block_toggle_fold(btv, fstack->pushedblock);
	}
}

static void bftextview2_collapse_expand_all_toggle(BluefishTextView * btv, gboolean collapse)
{
	GSequenceIter *siter = NULL;
	Tfoundstack *fstack;
	fstack = get_stackcache_first(btv, &siter);
	while (fstack) {
		if (fstack->pushedblock && fstack->pushedblock->foldable && fstack->pushedblock->folded != collapse)
			bftextview2_block_toggle_fold(btv, fstack->pushedblock);
		fstack = get_stackcache_next(btv, &siter);
	}
}

static void bftextview2_collapse_all_lcb(GtkMenuItem * mitem, BluefishTextView * btv)
{
	bftextview2_collapse_expand_all_toggle(btv, TRUE);
}

static void bftextview2_expand_all_lcb(GtkMenuItem * mitem, BluefishTextView * btv)
{
	bftextview2_collapse_expand_all_toggle(btv, FALSE);
}

static GtkWidget *bftextview2_fold_menu(BluefishTextView * btv)
{
	GtkWidget *mitem, *menu = gtk_menu_new();
	mitem = gtk_menu_item_new_with_label(_("Collapse all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
	g_signal_connect(G_OBJECT(mitem), "activate", G_CALLBACK(bftextview2_collapse_all_lcb), btv);
	mitem = gtk_menu_item_new_with_label(_("Expand all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
	g_signal_connect(G_OBJECT(mitem), "activate", G_CALLBACK(bftextview2_expand_all_lcb), btv);
	gtk_widget_show_all(menu);
	/* only required for submenu's that have a radioitem ????? g_signal_connect(G_OBJECT(menu), "destroy", destroy_disposable_menu_cb, menu); */
	return menu;
}

static void bftextview2_get_iter_at_bevent(BluefishTextView *btv, GdkEventButton *bevent, GtkTextIter *iter) {
	gint xpos, ypos;
	GtkTextWindowType wintype;

	wintype = gtk_text_view_get_window_type(GTK_TEXT_VIEW(btv),
			gtk_widget_get_window(GTK_WIDGET(btv)));
	gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(btv), wintype,bevent->x, bevent->y,
					  &xpos, &ypos);
	xpos += gtk_text_view_get_border_window_size(GTK_TEXT_VIEW(btv),GTK_TEXT_WINDOW_LEFT);
	gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(btv), iter, xpos, ypos);
}


static gboolean bluefish_text_view_button_press_event(GtkWidget * widget, GdkEventButton * event)
{
	BluefishTextView *btv = BLUEFISH_TEXT_VIEW(widget);
	BluefishTextView *master = BLUEFISH_TEXT_VIEW(btv->master);

	if (event->window == gtk_text_view_get_window(GTK_TEXT_VIEW(btv), GTK_TEXT_WINDOW_LEFT)) {
		DBG_SIGNALS("bluefish_text_view_button_press_event\n");
		if (event->button == 1) {
			gint x, y;
			GtkTextIter it;

			gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(btv), GTK_TEXT_WINDOW_TEXT, 0, event->y, &x,
												  &y);
			gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(widget), &it, y, &x);

			if (event->type == GDK_2BUTTON_PRESS && (event->x > (master->margin_pixels_chars))
				&& (event->x < (master->margin_pixels_chars + master->margin_pixels_symbol))) {
				GdkRegion *region;
				bmark_toggle(btv->doc, gtk_text_iter_get_offset(&it), NULL, NULL);
				/* redraw margin */
				region = gdk_drawable_get_clip_region(event->window);
				gdk_window_invalidate_region(event->window, region, FALSE);
				gdk_region_destroy(region);

				return TRUE;
			}
			if (btv->show_blocks && (event->x > (master->margin_pixels_chars + master->margin_pixels_symbol))) {	/* get the offset that equals the folding area */

				gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(widget), &it, y, &x);
				DBG_FOLD("fold/unfold at offset %d (line %d)\n", gtk_text_iter_get_offset(&it),
						 gtk_text_iter_get_line(&it));
				bftextview2_toggle_fold(btv, &it);
				return TRUE;
			}
			if (event->x < master->margin_pixels_chars) {
				if (!gtk_text_iter_ends_line(&it)) {
					GtkTextIter it2;
					/* select line */
					it2 = it;
					gtk_text_iter_forward_to_line_end(&it2);
					DBG_MSG("select from %d to %d\n", gtk_text_iter_get_offset(&it),
							gtk_text_iter_get_offset(&it2));
					gtk_text_buffer_select_range(gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv)), &it, &it2);
				}
				return TRUE;
			}
		} else if (event->button == 3 && btv->show_blocks && (event->x > master->margin_pixels_chars)) {
			gtk_menu_popup(GTK_MENU(bftextview2_fold_menu(btv)), NULL, NULL, NULL, NULL, event->button,
						   event->time);
			return TRUE;
		}
	}
	if (event->button == 1) {
		btv->needs_blockmatch = TRUE;
		if (!btv->mark_set_idle)
			btv->mark_set_idle = g_idle_add_full(G_PRIORITY_HIGH_IDLE, mark_set_idle_lcb, btv->master, NULL);

	} else if (event->button == 3) {
		GtkTextIter iter;
		/* store the location of the right mouse button click for menu items like 'edit tag'
		   or 'edit color' */
		bftextview2_get_iter_at_bevent(btv, event, &iter);
		main_v->bevent_doc = master->doc;
		main_v->bevent_charoffset = gtk_text_iter_get_offset(&iter);
	}
	/* here we ask any plugins to do any processing */
	if (main_v->doc_view_button_press_cbs) {
		GSList *tmplist = main_v->doc_view_button_press_cbs;
		while (tmplist) {
			void *(*func) () = tmplist->data;
			DEBUG_MSG("doc_view_button_press_lcb, calling plugin func %p\n", tmplist->data);
			func(widget, event, (Tdocument *) master->doc);
			tmplist = g_slist_next(tmplist);
		}
	}

	return GTK_WIDGET_CLASS(bluefish_text_view_parent_class)->button_press_event(widget, event);
}

static gchar *get_prevline_indenting(GtkTextBuffer * buffer, GtkTextIter * itend, gchar * lastchar)
{
	gchar *string;
	gchar *indenting;
	gint stringlen;

	GtkTextIter itstart;

	itstart = *itend;
	/* set to the beginning of the previous line */
	gtk_text_iter_backward_line(&itstart);
	gtk_text_iter_set_line_index(&itstart, 0);
	string = gtk_text_buffer_get_text(buffer, &itstart, itend, TRUE);
	if (!string)
		return NULL;

	stringlen = strlen(string);
	if (stringlen > 1) {
		*lastchar = string[stringlen - 2];
	}
	/* now count the indenting in this string */
	indenting = string;
	while (*indenting == '\t' || *indenting == ' ') {
		indenting++;
	}
	/* ending search, non-whitespace found, so terminate at this position */
	*indenting = '\0';
	return string;
}

static gboolean bluefish_text_view_key_release_event(GtkWidget * widget, GdkEventKey * kevent)
{
	BluefishTextView *btv = BLUEFISH_TEXT_VIEW(widget);
	gboolean prev_insert_was_auto_indent = btv->insert_was_auto_indent;
	btv->insert_was_auto_indent = FALSE;

	DBG_SIGNALS("bluefish_text_view_key_release_event for %p, keyval=%d\n", widget, kevent->keyval);

	if (!btv->mark_set_idle && btv->needs_blockmatch)
		btv->mark_set_idle = g_idle_add_full(G_PRIORITY_HIGH_IDLE, mark_set_idle_lcb, btv->master, NULL);

	/* sometimes we receive a release event for a key that was not pressed in the textview widget!
	   for example if you use the keyboard to navigate the menu, and press enter to activate an item, a 
	   key release event is received in the textview widget.... so we have to check that ! */
	if (!btv->key_press_inserted_char)
		return GTK_WIDGET_CLASS(bluefish_text_view_parent_class)->key_release_event(widget, kevent);
	btv->key_press_inserted_char = FALSE;	/* after the check we set this to FALSE */

	if (!BLUEFISH_TEXT_VIEW(btv->master)->auto_indent)
		return GTK_WIDGET_CLASS(bluefish_text_view_parent_class)->key_release_event(widget, kevent);
	/*g_print("bluefish_text_view_key_release_event, working on keyval %d\n",kevent->keyval); */
	if ((kevent->keyval == GDK_Return || kevent->keyval == GDK_KP_Enter)
		&& !((kevent->state & GDK_SHIFT_MASK) || (kevent->state & GDK_CONTROL_MASK)
			 || (kevent->state & GDK_MOD1_MASK))) {
		gchar *string;
		gchar lastchar = '\0';
		GtkTextIter itend;
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
		gtk_text_buffer_get_iter_at_mark(buffer, &itend, gtk_text_buffer_get_insert(buffer));
		string = get_prevline_indenting(buffer, &itend, &lastchar);
		/*g_print("bluefish_text_view_key_release_event, previoud indenting '%s'\n",string); */
		if (string) {
			/*g_print("bluefish_text_view_key_release_event, lastchar=%c, smartindentchars=%s\n",lastchar, btv->bflang->smartindentchars); */
			if (lastchar != '\0' && main_v->props.smartindent
				&& btv->bflang && btv->bflang->smartindentchars
				&& strchr(btv->bflang->smartindentchars, lastchar) != NULL) {
				gchar *tmp, *tmp2;
				if (main_v->props.editor_indent_wspaces)
					tmp2 = bf_str_repeat(" ", BFWIN(DOCUMENT(btv->doc)->bfwin)->session->editor_tab_width);
				else
					tmp2 = g_strdup("	");
				tmp = g_strconcat(string, tmp2, NULL);
				g_free(string);
				g_free(tmp2);
				string = tmp;
			}
			if (string && string[0] != '\0') {
				gboolean in_paste = DOCUMENT(BLUEFISH_TEXT_VIEW(btv->master)->doc)->in_paste_operation;
				/*g_print("bluefish_text_view_key_release_event, autoindent, insert indenting\n"); */
				/* a dirty trick: if in_paste_operation is set, there will be no call 
				   for doc_unre_new_group when indenting is inserted */
				if (!in_paste)
					DOCUMENT(BLUEFISH_TEXT_VIEW(btv->master)->doc)->in_paste_operation = TRUE;
				gtk_text_buffer_insert(buffer, &itend, string, -1);
				if (!in_paste)
					DOCUMENT(BLUEFISH_TEXT_VIEW(btv->master)->doc)->in_paste_operation = FALSE;
				btv->insert_was_auto_indent = TRUE;
			}
			g_free(string);
		}
	} else if (main_v->props.smartindent && prev_insert_was_auto_indent
			   && btv->bflang && btv->bflang->smartoutdentchars
			   && strchr(btv->bflang->smartoutdentchars, kevent->keyval) != NULL) {
		GtkTextIter itend, itstart;
		gunichar uc;
		/* reduce the indenting one level back */
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
		gtk_text_buffer_get_iter_at_mark(buffer, &itend, gtk_text_buffer_get_insert(buffer));
		gtk_text_iter_backward_char(&itend);
		itstart = itend;
		gtk_text_iter_backward_char(&itstart);
		uc = gtk_text_iter_get_char(&itstart);
		/*g_print("found indenting char '%c'\n",uc); */
		if (uc == '\t') {
			gtk_text_buffer_delete(buffer, &itstart, &itend);
		} else if (uc == ' ') {
			int i = 1;
			/* if a space was the previous char, we need N spaces to unindent */
			while (uc == ' ' && i < BFWIN(DOCUMENT(btv->doc)->bfwin)->session->editor_tab_width) {
				gtk_text_iter_backward_char(&itstart);
				uc = gtk_text_iter_get_char(&itstart);
				i++;
			}
			gtk_text_buffer_delete(buffer, &itstart, &itend);
		}
	}
	return GTK_WIDGET_CLASS(bluefish_text_view_parent_class)->key_release_event(widget, kevent);
}

/* called for example by doc_reload() */
void bluefish_text_view_scan_cleanup(BluefishTextView * btv)
{
	cleanup_scanner(btv);
}

void bluefish_text_view_rescan(BluefishTextView * btv)
{
	DBG_MSG("bluefish_text_view_rescan, btv=%p, lang=%p\n", btv, btv->bflang);
	cleanup_scanner(btv->master);
	if (BLUEFISH_TEXT_VIEW(btv->master)->bflang) {
		GtkTextIter start, end;
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv->master));
		gtk_text_buffer_get_bounds(buffer, &start, &end);
		gtk_text_buffer_apply_tag(buffer, BLUEFISH_TEXT_VIEW(btv->master)->needscanning, &start, &end);
		btv->needremovetags = TRUE;
#ifdef HAVE_LIBENCHANT
		DBG_SPELL("bluefish_text_view_rescan, mark all with needspellcheck\n");
		gtk_text_buffer_apply_tag(buffer, BLUEFISH_TEXT_VIEW(btv->master)->needspellcheck, &start, &end);
#endif							/*HAVE_LIBENCHANT */
		bftextview2_schedule_scanning(btv);
	}
}

	/* returns TRUE if 
	   there is a selection and a comment start and end is inside the selection
	   OR no selection and cursor is inside a comment */
gboolean bluefish_text_view_in_comment(BluefishTextView * btv, GtkTextIter * its, GtkTextIter * ite)
{
	GtkTextIter tmpits, tmpite;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
	GtkTextTag *comment_tag = gtk_text_tag_table_lookup(langmgr_get_tagtable(), "comment");
	if (gtk_text_buffer_get_selection_bounds(buffer, &tmpits, &tmpite)) {
		*its = tmpits;
		*ite = tmpite;
		/* first test if the selection starts the tag */
		if ((gtk_text_iter_begins_tag(&tmpits, comment_tag) && gtk_text_iter_ends_tag(&tmpite, comment_tag))
			|| (gtk_text_iter_begins_tag(&tmpite, comment_tag) && gtk_text_iter_ends_tag(&tmpits, comment_tag))
			) {
			DEBUG_MSG("bluefish_text_view_in_comment, selection toggles comment, return TRUE\n");
			return TRUE;
		}
		DEBUG_MSG("bluefish_text_view_in_comment, selection does NOT toggle comment, return FALSE\n");
		return FALSE;
	} else {
		gboolean retval;
		gtk_text_buffer_get_iter_at_mark(buffer, &tmpits,
										 gtk_text_buffer_get_insert(buffer));
		/* for line comments the \n is usually not part of the comment anymore, so move back one char */
		if (gtk_text_iter_ends_line(&tmpits))
			gtk_text_iter_backward_char(&tmpits);
		retval = gtk_text_iter_has_tag(&tmpits, comment_tag);
		*ite = *its = tmpits;
		if (retval) {
			gtk_text_iter_forward_to_tag_toggle(ite, comment_tag);
			gtk_text_iter_backward_to_tag_toggle(its, comment_tag);
			return TRUE;
		}
	}
	return FALSE;
}

Tcomment *bluefish_text_view_get_comment(BluefishTextView * btv, GtkTextIter * it,
										 Tcomment_type preferred_type)
{
	/* get the context, and then retrieve the preferred comment type for that context */
	GQueue *contextstack;
	GList *tmplist;
	guint16 contextnum = 0;

	if (!btv->bflang)
		return NULL;

	if (!btv->bflang->st)
		return NULL;

	if (btv->master != btv)
		g_warning("bluefish_text_view_get_comment should only be called for the master widget\n");

	contextstack = get_contextstack_at_position(btv, it);
/*	g_print("bluefish_text_view_get_comment, got contextstack %p with len %d\n",contextstack,contextstack->length);*/
	if (!contextstack)
		return NULL;

	for (tmplist = g_list_first(contextstack->head); contextnum != 1; tmplist = g_list_next(tmplist)) {
		guint8 line, block;
		if (tmplist) {
			contextnum = GPOINTER_TO_INT(tmplist->data);
		} else {
			contextnum = 1;
		}
/*		g_print("bluefish_text_view_get_comment, contextnum=%d\n",contextnum);*/
		line = g_array_index(btv->bflang->st->contexts, Tcontext, contextnum).comment_line;
		block = g_array_index(btv->bflang->st->contexts, Tcontext, contextnum).comment_block;
/*		g_print("bluefish_text_view_get_comment, comment_line=%d, comment_block=%d\n",line,block);*/
		if (line == COMMENT_INDEX_NONE && block == COMMENT_INDEX_NONE)
			return NULL;

		if ((line == COMMENT_INDEX_INHERIT && block == COMMENT_INDEX_INHERIT)
			|| (line == COMMENT_INDEX_INHERIT && block == COMMENT_INDEX_NONE)
			|| (line == COMMENT_INDEX_NONE && block == COMMENT_INDEX_INHERIT)
			)
			continue;

		if (preferred_type == block) {
			if (block == COMMENT_INDEX_NONE) {
				return &g_array_index(btv->bflang->st->comments, Tcomment, line);
			} else if (block == COMMENT_INDEX_INHERIT) {
				continue;
			} else {
				return &g_array_index(btv->bflang->st->comments, Tcomment, block);
			}
		} else {
			if (line == COMMENT_INDEX_NONE) {
				return &g_array_index(btv->bflang->st->comments, Tcomment, block);
			} else if (line == COMMENT_INDEX_INHERIT) {
				continue;
			} else {
				return &g_array_index(btv->bflang->st->comments, Tcomment, line);
			}
		}
	}
	return NULL;
}


gboolean bluefish_text_view_get_auto_complete(BluefishTextView * btv)
{
	return (BLUEFISH_TEXT_VIEW(btv->master)->auto_complete);
}

void bluefish_text_view_set_auto_complete(BluefishTextView * btv, gboolean enable)
{
	g_return_if_fail(btv != NULL);

	if (enable == BLUEFISH_TEXT_VIEW(btv->master)->auto_complete) {
		return;
	}

	BLUEFISH_TEXT_VIEW(btv->master)->auto_complete = enable;
}

gboolean bluefish_text_view_get_auto_indent(BluefishTextView * btv)
{
	return (BLUEFISH_TEXT_VIEW(btv->master)->auto_indent);
}

void bluefish_text_view_set_auto_indent(BluefishTextView * btv, gboolean enable)
{
	g_return_if_fail(btv != NULL);

	if (enable == BLUEFISH_TEXT_VIEW(btv->master)->auto_indent) {
		return;
	}

	BLUEFISH_TEXT_VIEW(btv->master)->auto_indent = enable;
}

static void bftextview2_parse_static_colors(void)
{
	if (!
		(main_v->props.btv_color_str[BTV_COLOR_CURRENT_LINE]
		 && gdk_color_parse(main_v->props.btv_color_str[BTV_COLOR_CURRENT_LINE], &st_cline_color))) {
		gdk_color_parse("#e0e0e0", &st_cline_color);
	}
	if (!
		(main_v->props.btv_color_str[BTV_COLOR_WHITESPACE]
		 && gdk_color_parse(main_v->props.btv_color_str[BTV_COLOR_WHITESPACE], &st_whitespace_color))) {
		gdk_color_parse("#ff0000", &st_whitespace_color);
	}
	if (main_v->props.btv_color_str[BTV_COLOR_CURSOR] != NULL
		&& main_v->props.btv_color_str[BTV_COLOR_CURSOR][0] != '\0') {
		gchar *tmp;
		tmp = g_strconcat("style \"bluefish-cursor\" {"
						  "GtkTextView::cursor-color = \"",
						  main_v->props.btv_color_str[BTV_COLOR_CURSOR],
						  "\"}" "class \"GtkTextView\" style \"bluefish-cursor\"", NULL);
		gtk_rc_parse_string(tmp);
		g_free(tmp);
	}
}

void bftextview2_init_globals(void) {
	bftextview2_parse_static_colors();
	if (main_v->props.autocomp_accel_string && main_v->props.autocomp_accel_string[0] != '\0') { 
		gtk_accelerator_parse(main_v->props.autocomp_accel_string,&main_v->autocomp_accel_key,&main_v->autocomp_accel_mods);
		if (gtk_accelerator_valid(main_v->autocomp_accel_key,main_v->autocomp_accel_mods)) {
			return;
		}
		g_warning("%s is not a valid shortcut key combination\n",main_v->props.autocomp_accel_string);
	}
	main_v->autocomp_accel_key = ' ';
	main_v->autocomp_accel_mods = GDK_CONTROL_MASK;
}

void bluefish_text_view_set_colors(BluefishTextView * btv, gchar * const *colors)
{
	GdkColor color;
	DEBUG_MSG("bluefish_text_view_set_colors, started for %p\n",btv);
	if (colors[BTV_COLOR_ED_BG] && gdk_color_parse(colors[BTV_COLOR_ED_BG], &color)) {
		gtk_widget_modify_base(GTK_WIDGET(btv), GTK_STATE_NORMAL, &color);
		if (btv->slave)
			gtk_widget_modify_base(GTK_WIDGET(btv->slave), GTK_STATE_NORMAL, &color);
	}
	if (colors[BTV_COLOR_ED_FG] && gdk_color_parse(colors[BTV_COLOR_ED_FG], &color)) {
		gtk_widget_modify_text(GTK_WIDGET(btv), GTK_STATE_NORMAL, &color);
		if (btv->slave)
			gtk_widget_modify_text(GTK_WIDGET(btv->slave), GTK_STATE_NORMAL, &color);
	}
	
}

void bluefish_text_view_set_mimetype(BluefishTextView * btv, const gchar * mime)
{
	GtkTextIter start, end;
	GtkTextBuffer *buffer;
	Tbflang *bflang = langmgr_get_bflang_for_mimetype(mime);
	BluefishTextView *master=btv->master;
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(master));
	/* remove all highlighting */
	cleanup_scanner(master);
	DBG_MSG("bluefish_text_view_set_mimetype, found bflang %p for mimetype %s\n", bflang, mime);
	if (bflang) {
		/* set new language */
		master->bflang = bflang;
		/* restart scanning */
		gtk_text_buffer_get_bounds(buffer, &start, &end);
		gtk_text_buffer_apply_tag(buffer, master->needscanning, &start, &end);
		btv->needremovetags = TRUE;
#ifdef HAVE_LIBENCHANT
		gtk_text_buffer_apply_tag(buffer, master->needspellcheck, &start, &end);
#endif
		bftextview2_schedule_scanning(master);
	} else {
		master->bflang = NULL;
	}
	DBG_MSG("bluefish_text_view_set_mimetype, done for %p\n",btv);
}

gboolean bluefish_text_view_get_show_blocks(BluefishTextView * btv)
{
	return (btv->show_blocks);
}

void bluefish_text_view_set_show_blocks(BluefishTextView * btv, gboolean show)
{
	g_return_if_fail(btv != NULL);

	if (show == btv->show_blocks) {
		return;
	}

	btv->show_blocks = show;
	bftextview2_set_margin_size(btv);
	gtk_widget_queue_draw(GTK_WIDGET(btv));
	if (btv->slave)
		gtk_widget_queue_draw(GTK_WIDGET(btv->slave));
}

gboolean bluefish_text_view_get_show_line_numbers(BluefishTextView * btv)
{
	return (btv->show_line_numbers);
}

void bluefish_text_view_set_show_line_numbers(BluefishTextView * btv, gboolean show)
{
	g_return_if_fail(btv != NULL);

	if (show == btv->show_line_numbers) {
		return;
	}

	btv->show_line_numbers = show;
	bftextview2_set_margin_size(btv);
	gtk_widget_queue_draw(GTK_WIDGET(btv));
	if (btv->slave)
		gtk_widget_queue_draw(GTK_WIDGET(btv->slave));
}

gboolean bluefish_text_view_get_show_visible_spacing(BluefishTextView * btv)
{
	return (btv->visible_spacing);
}

void bluefish_text_view_set_show_visible_spacing(BluefishTextView * btv, gboolean show)
{
	g_return_if_fail(btv != NULL);

	if (show == btv->visible_spacing) {
		return;
	}

	btv->visible_spacing = show;
	gtk_widget_queue_draw(GTK_WIDGET(btv));
	if (btv->slave)
		gtk_widget_queue_draw(GTK_WIDGET(btv->slave));
}

gboolean bluefish_text_view_get_show_right_margin(BluefishTextView * btv)
{
	return (btv->show_right_margin);
}

void bluefish_text_view_set_show_right_margin(BluefishTextView * btv, gboolean show)
{
	g_return_if_fail(btv != NULL);

	if (show == btv->show_right_margin) {
		return;
	}

	btv->show_right_margin = show;
	gtk_widget_queue_draw(GTK_WIDGET(btv));
	if (btv->slave)
		gtk_widget_queue_draw(GTK_WIDGET(btv->slave));
}

gboolean bluefish_text_view_get_show_mbhl(BluefishTextView * btv)
{
	return (btv->show_mbhl);
}

void bluefish_text_view_set_show_mbhl(BluefishTextView * btv, gboolean show)
{
	g_return_if_fail(btv != NULL);

	if (show == btv->show_mbhl) {
		return;
	}

	btv->show_mbhl = show;
	gtk_widget_queue_draw(GTK_WIDGET(btv));
	if (btv->slave)
		gtk_widget_queue_draw(GTK_WIDGET(btv->slave));
}

#ifdef HAVE_LIBENCHANT
void bluefish_text_view_set_spell_check(BluefishTextView * btv, gboolean spell_check)
{
	GtkTextIter start, end;
	GtkTextBuffer *buffer;

	g_return_if_fail(btv != NULL);

	if (btv->spell_check == spell_check) {
		return;
	}

	btv->spell_check = spell_check;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
	gtk_text_buffer_get_bounds(buffer, &start, &end);

	if (btv->spell_check) {
		gtk_text_buffer_apply_tag(buffer, btv->needspellcheck, &start, &end);
		bftextview2_schedule_scanning(btv->master);
	} else {
		gtk_text_buffer_remove_tag_by_name(buffer, "_spellerror_", &start, &end);
	}
}
#endif

static gboolean bluefish_text_view_query_tooltip(GtkWidget * widget, gint x, gint y, gboolean keyboard_tip,
												 GtkTooltip * tooltip)
{
	BluefishTextView *btv = BLUEFISH_TEXT_VIEW(widget);
	BluefishTextView *master = BLUEFISH_TEXT_VIEW(btv->master);
	if (master->bflang && master->bflang->st && master->enable_scanner && master->scanner_idle == 0
		&& main_v->props.show_tooltip_reference) {
		GtkTextIter iter, mstart;
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(master));
		gint contextnum;
		/* get position */
		if (keyboard_tip) {
			gint offset;
			g_object_get(buffer, "cursor-position", &offset, NULL);
			gtk_text_buffer_get_iter_at_offset(buffer, &iter, offset);
		} else {
			gint bx, by, trailing;
			/*g_print("get iter at mouse position %d %d\n",x,y); */
			gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(btv), GTK_TEXT_WINDOW_TEXT,
												  x - (btv->margin_pixels_chars + btv->margin_pixels_block +
													   btv->margin_pixels_symbol), y, &bx, &by);
			if (bx < 0)
				return FALSE;
			/* if I don't do this check, I get the following error during 'Collapse all'
			   (bluefish-unstable:3866): Gtk-WARNING **: /build/buildd/gtk+2.0-2.16.1/gtk/gtktextbtree.c:4017: byte index off the end of the line
			   Gtk-ERROR **: Byte index 533 is off the end of the line aborting...

			   #0  0xb80bf430 in __kernel_vsyscall ()
			   #1  0xb761f6d0 in raise () from /lib/tls/i686/cmov/libc.so.6
			   #2  0xb7621098 in abort () from /lib/tls/i686/cmov/libc.so.6
			   #3  0xb77b3eac in IA__g_logv (log_domain=0xb7fcba77 "Gtk", 
			   log_level=G_LOG_LEVEL_ERROR, 
			   format=0xb8076f74 "Byte index %d is off the end of the line", 
			   args1=0xbfcdc06c "\206\002")
			   at /build/buildd/glib2.0-2.20.1/glib/gmessages.c:506
			   #4  0xb77b3ee6 in IA__g_log (log_domain=0xb7fcba77 "Gtk", 
			   log_level=G_LOG_LEVEL_ERROR, 
			   format=0xb8076f74 "Byte index %d is off the end of the line")
			   at /build/buildd/glib2.0-2.20.1/glib/gmessages.c:526
			   #5  0xb7ed4ee7 in iter_set_from_byte_offset (iter=0xbfcdc0c4, line=0x9b1cbb8, 
			   byte_offset=646) at /build/buildd/gtk+2.0-2.16.1/gtk/gtktextiter.c:110
			   #6  0xb7ed967f in IA__gtk_text_iter_set_visible_line_index (iter=0xbfcdc1e4, 
			   byte_on_line=4) at /build/buildd/gtk+2.0-2.16.1/gtk/gtktextiter.c:3906
			   #7  0xb7edc8c0 in line_display_index_to_iter (layout=0x9711728, 
			   display=0x9c30618, iter=0xbfcdc1e4, index=69, trailing=0)
			   at /build/buildd/gtk+2.0-2.16.1/gtk/gtktextlayout.c:2549
			   #8  0xb7ee099c in IA__gtk_text_layout_get_iter_at_position (layout=0x9711728, 
			   target_iter=0xbfcdc1e4, trailing=0xbfcdc21c, x=-5, y=<value optimized out>)
			   at /build/buildd/gtk+2.0-2.16.1/gtk/gtktextlayout.c:2670
			   #9  0x080646cd in bluefish_text_view_query_tooltip (widget=0x9783050, x=47, 
			   y=88, keyboard_tip=0, tooltip=0x97d0a38) at bftextview2.c:1187

			   I guess this is a race condition, during collapse all a lot of text is made hidden
			   and for the tooltip we request an iter somewhere in the text that is becoming hidden
			 */
			/*g_print("get iter at buffer position %d %d\n",bx,by); */
			/* gtk 2.14 cannot handle a NULL instead of &trailing. so although the docs tell
			   that if you don't need it you can pass NULL, we will not do so. */
			gtk_text_view_get_iter_at_position(GTK_TEXT_VIEW(btv), &iter, &trailing, bx, by);
			/*g_print("done\n"); */
		}
		mstart = iter;
		gtk_text_iter_set_line_offset(&mstart, 0);
		gtk_text_iter_forward_char(&iter);
		DBG_TOOLTIP("scan for tooltip: start at %d, position=%d...\n", gtk_text_iter_get_offset(&mstart),
					gtk_text_iter_get_offset(&iter));
		if (scan_for_tooltip(master, &mstart, &iter, &contextnum)) {
			DBG_TOOLTIP("we have a match in context %d, has_patternhash=%d\n", contextnum,
						(g_array_index(master->bflang->st->contexts, Tcontext, contextnum).patternhash != NULL));
			if (g_array_index(master->bflang->st->contexts, Tcontext, contextnum).patternhash) {
				gint pattern_id;
				gchar *key = gtk_text_buffer_get_text(buffer, &mstart, &iter, TRUE);
				pattern_id =
					GPOINTER_TO_INT(g_hash_table_lookup
									(g_array_index(master->bflang->st->contexts, Tcontext, contextnum).
									 patternhash, key));
				if (pattern_id && g_array_index(master->bflang->st->matches, Tpattern, pattern_id).reference) {
					DBG_TOOLTIP("key=%s, value=%s\n", key,
								g_array_index(master->bflang->st->matches, Tpattern, pattern_id).reference);
					gtk_tooltip_set_markup(tooltip,
										   g_array_index(master->bflang->st->matches, Tpattern,
														 pattern_id).reference);
					g_free(key);
					return TRUE;
				}
				g_free(key);
			}
		}
	}

	return FALSE;
}

static gboolean bluefish_text_view_focus_out_event(GtkWidget * widget, GdkEventFocus * event)
{
	if (BLUEFISH_TEXT_VIEW(widget)->autocomp) {
		autocomp_stop(BLUEFISH_TEXT_VIEW(widget));
	}
	return GTK_WIDGET_CLASS(bluefish_text_view_parent_class)->focus_out_event(widget, event);
}

static void bluefish_text_view_finalize(GObject * object)
{
	BluefishTextView *btv;
	BluefishTextViewClass *klass;

	g_return_if_fail(object != NULL);
	DBG_MSG("destroy BluefishTextView %p\n", object);
	btv = BLUEFISH_TEXT_VIEW(object);
	if (btv->master != btv && BLUEFISH_TEXT_VIEW(btv->master)->slave == btv) {
		BLUEFISH_TEXT_VIEW(btv->master)->slave = NULL;
	}
	
	if (btv->scanner_delayed) {
		g_source_remove(btv->scanner_delayed);
		btv->scanner_delayed = 0;
	}
	if (btv->scanner_idle) {
		g_source_remove(btv->scanner_idle);
		btv->scanner_idle = 0;
	}
	if (btv->user_idle) {
		g_source_remove(btv->user_idle);
		btv->user_idle = 0;
	}
	if (btv->autocomp) {
		autocomp_stop(btv);
	}
	if (btv->scancache.stackcaches) {
		scancache_destroy(btv);
	}
	if (btv->user_idle_timer) {
		g_timer_destroy(btv->user_idle_timer);
		btv->user_idle_timer = NULL;
	}
	if (btv->buffer) {
		DBG_MSG("bluefish_text_view_finalize %p, disconnect signals from buffer %p\n",btv,btv->buffer);
		if (btv->insert_text_id)
			g_signal_handler_disconnect(btv->buffer, btv->insert_text_id);
		if (btv->insert_text_after_id)
			g_signal_handler_disconnect(btv->buffer, btv->insert_text_after_id);
		if (btv->mark_set_id)
			g_signal_handler_disconnect(btv->buffer, btv->mark_set_id);
		if (btv->delete_range_id)
			g_signal_handler_disconnect(btv->buffer, btv->delete_range_id);
		if (btv->delete_range_after_id)
			g_signal_handler_disconnect(btv->buffer, btv->delete_range_after_id);
	}
	btv->bflang = NULL;
	btv->enable_scanner = FALSE;
	klass = gtk_type_class(gtk_widget_get_type());
	if (G_OBJECT_CLASS(klass)->finalize) {
		(*G_OBJECT_CLASS(klass)->finalize) (object);
	}
}

/* *************************************************************** */
/* widget stuff below */
/* *************************************************************** */
/*
static void bluefish_text_view_finalize(GObject * object)
{
	G_OBJECT_CLASS(bluefish_text_view_parent_class)->finalize(object);
}*/
/*
static GObject *bluefish_text_view_create(GType type, guint n_construct_properties,
										  GObjectConstructParam * construct_properties)
{
	BluefishTextViewClass *klass =
		BLUEFISH_TEXT_VIEW_CLASS(g_type_class_peek(BLUEFISH_TYPE_TEXT_VIEW));
	GObjectClass *parent_class = G_OBJECT_CLASS(g_type_class_peek_parent(klass));
	GObject *obj = parent_class->constructor(type,
											 n_construct_properties,
											 construct_properties);

	/ * This constructor is not needed right now * /

	return (obj);
}*/

static void bluefish_text_view_class_init(BluefishTextViewClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

/*	object_class->constructor = bluefish_text_view_create;*/
	object_class->finalize = bluefish_text_view_finalize;

	widget_class->button_press_event = bluefish_text_view_button_press_event;
	widget_class->expose_event = bluefish_text_view_expose_event;
	widget_class->key_press_event = bluefish_text_view_key_press_event;
	widget_class->key_release_event = bluefish_text_view_key_release_event;
	widget_class->query_tooltip = bluefish_text_view_query_tooltip;
	widget_class->focus_out_event = bluefish_text_view_focus_out_event;
}

void bluefish_text_view_multiset(BluefishTextView * btv, gpointer doc, gint view_line_numbers,
								 gint view_blocks, gint autoindent, gint autocomplete, gint show_mbhl)
{
	BLUEFISH_TEXT_VIEW(btv->master)->doc = doc;
	BLUEFISH_TEXT_VIEW(btv->master)->show_line_numbers = view_line_numbers;
	BLUEFISH_TEXT_VIEW(btv->master)->show_blocks = view_blocks;
	BLUEFISH_TEXT_VIEW(btv->master)->auto_indent = autoindent;
	BLUEFISH_TEXT_VIEW(btv->master)->auto_complete = autocomplete;
	BLUEFISH_TEXT_VIEW(btv->master)->show_mbhl = show_mbhl;
}

static void bluefish_text_view_init(BluefishTextView * textview)
{
	GtkTextTagTable *ttt;
/*	PangoFontDescription *font_desc;*/
	textview->user_idle_timer = g_timer_new();
	textview->scancache.stackcaches = g_sequence_new(NULL);
	bluefish_text_view_set_colors(textview, main_v->props.btv_color_str);
	textview->showsymbols = TRUE;
	ttt = langmgr_get_tagtable();
	textview->needscanning = gtk_text_tag_table_lookup(ttt, "_needscanning_");
#ifdef HAVE_LIBENCHANT
	textview->needspellcheck = gtk_text_tag_table_lookup(ttt, "_needspellcheck_");
#endif							/*HAVE_LIBENCHANT */
	textview->blockmatch = gtk_text_tag_table_lookup(ttt, "blockmatch");
	textview->enable_scanner = FALSE;
	/*font_desc = pango_font_description_from_string("Monospace 10");
	   gtk_widget_modify_font(GTK_WIDGET(textview), font_desc);
	   pango_font_description_free(font_desc); */
}

GtkWidget *bftextview2_new(void)
{
	BluefishTextView *textview = g_object_new(BLUEFISH_TYPE_TEXT_VIEW,
											  "has-tooltip", TRUE, NULL);

	g_return_val_if_fail(textview != NULL, NULL);
	textview->master = textview;
	textview->slave = NULL;
	return GTK_WIDGET(textview);
}

GtkWidget *bftextview2_new_with_buffer(GtkTextBuffer * buffer)
{
	BluefishTextView *textview = (BluefishTextView *) bftextview2_new();

	g_return_val_if_fail(textview != NULL, NULL);
	DBG_MSG("bftextview2_new_with_buffer, textview=%p, buffer=%p\n",textview,buffer);
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(textview), buffer);
	textview->buffer = buffer;
	textview->insert_text_id = g_signal_connect(G_OBJECT(buffer), "insert-text", G_CALLBACK(bftextview2_insert_text_lcb), textview);
	textview->insert_text_after_id = g_signal_connect_after(G_OBJECT(buffer), "insert-text", G_CALLBACK(bftextview2_insert_text_after_lcb),
						   textview);
	textview->mark_set_id = g_signal_connect_after(G_OBJECT(buffer), "mark-set", G_CALLBACK(bftextview2_mark_set_lcb), textview);
	textview->delete_range_id = g_signal_connect(G_OBJECT(buffer), "delete-range", G_CALLBACK(bftextview2_delete_range_lcb), textview);
	textview->delete_range_after_id = g_signal_connect_after(G_OBJECT(buffer), "delete-range", G_CALLBACK(bftextview2_delete_range_after_lcb),
						   textview);
	return GTK_WIDGET(textview);
}

GtkWidget *bftextview2_new_slave(BluefishTextView *master) {
	BluefishTextView *textview;
	GtkTextBuffer *buffer;
	
	g_return_val_if_fail(master != NULL, NULL);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(master));
	DBG_MSG("bftextview2_new_slave, master=%p, buffer=%p\n",master,buffer);
	textview = (BluefishTextView *) bftextview2_new_with_buffer(buffer);
	g_return_val_if_fail(textview != NULL, NULL);
	
	textview->master = master;
	master->slave = textview;
	DBG_MSG("bftextview2_new_slave, created slave %p for master %p\n",textview, master);
	gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(textview), GTK_TEXT_WINDOW_LEFT,
										 BLUEFISH_TEXT_VIEW(master)->margin_pixels_chars 
										 + BLUEFISH_TEXT_VIEW(master)->margin_pixels_block 
										 + BLUEFISH_TEXT_VIEW(master)->margin_pixels_symbol);
	return GTK_WIDGET(textview);
}
