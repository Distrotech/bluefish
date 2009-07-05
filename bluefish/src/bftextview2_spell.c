/* Bluefish HTML Editor
 * bftextview2_spell.c
 *
 * Copyright (C) 2009 Olivier Sessink
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
#ifdef HAVE_ENCHANT
#include <enchant.h>

#include "bluefish.h"

#define MAX_CONTINUOUS_SPELLCHECK_INTERVAL 0.1 /* float in seconds */

static EnchantBroker *eb;
static EnchantDict *ed;


static gboolean bftextview2_find_region2spellcheck(BluefishTextView * btv, GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end) {
	/* first find a region that needs a spellcheck */
	gtk_text_buffer_get_start_iter(buffer, start);
	if (!gtk_text_iter_begins_tag(start,btv->needspellcheck) ) {
		if (!gtk_text_iter_forward_to_tag_toggle(start,btv->needspellcheck)) {
			/* nothing to spellcheck */
			return FALSE;
		}
	}
	/* find the end of the region */
	*end = *start;
	gtk_text_iter_forward_char(end);
	if (!gtk_text_iter_ends_tag(end,btv->needspellcheck)) {
		if (!gtk_text_iter_forward_to_tag_toggle(end,btv->needspellcheck)) {
			DBG_MSG("BUG: we should never get here\n");
			return FALSE;
		}
	}
	return TRUE;
}

static void word_check(BluefishTextView * btv, GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end) {
	gchar *tocheck;
	
	tocheck = gtk_text_buffer_get_text(buffer, start,end);
	if (enchant_dict_check(ed, tocheck, strlen(tocheck)) != 0) {
		gtk_text_buffer_apply_tag_by_name(buffer, "_spellerror_", start, end);
	}
	g_free(tocheck);
}

static inline gint foundstack_need_spellcheck(BluefishTextView * btv, Tfoundstack *fstack) {
	guint16 contextnum;
	contextnum = g_queue_get_length(fstack->contextstack) ? GPOINTER_TO_INT(g_queue_peek_head(fstack->contextstack)): 1;
	return g_array_index(btv->bflang->st->contexts,Tcontext, contextnum).need_spellcheck;
}

static gboolean run_spellcheck(BluefishTextView * btv) {
	GtkTextIter so,eo,iter;
	GtkTextBuffer *buffer;
	Tfoundstack *fstack, *nextfstack;
	GSequenceIter *siter=NULL;
	GTimer *timer;
	gint loops=0;
	gint loops_per_timer=100;
	gboolean cont=TRUE;
	
	buffer = /*TODO*/
	if (!bftextview2_find_region2spellcheck(btv, buffer, &so, &eo));
		return FALSE;

	timer = g_timer_new();
	
	fstack = get_stackcache_at_position(btv, &so, &siter);
	need_spellcheck = foundstack_need_spellcheck(btv,fstack);
	nextfstack = get_stackcache_next(btv, &siter);
	do {
		if (need_spellcheck) {
			/* move to the next word, but check in the meantime that we're not entering a context that doesn't need spellchecking */
			
			nextfstack && nextfstack->charoffset;
			
		} else {
			/* move in the B-tree to the next context that needs spellchecking */
			fstack = nextfstack;
			nextfstack = get_stackcache_next(btv, &siter);
			if (fstack) {
				need_spellcheck = foundstack_need_spellcheck(btv,fstack);
				if (need_spellcheck) {
					/* set the iter so we can continue spellchecking */
					gtk_text_buffer_iter_at_offset(buffer,&iter,fstack->charoffset);
				}
			} else {
				/* we're finished spellchecking, set the iter to the end */
				iter=eo;
				cont=0;
			}
		}
		loops++;
		if (gtk_text_iter_compare(&iter, &so) <= 0) { /* TODO: check the order of the compare items */
			cont=0;
		}
	} while (cont && (loop%loops_per_timer!=0 || g_timer_elapsed(timer,NULL)<MAX_CONTINUOUS_SPELLCHECK_INTERVAL));
	
	gtk_text_buffer_remove_tag(buffer, btv->needspellcheck, &so , &iter);

	g_timer_destroy(scanning.timer);
	
	return TRUE;
}

static void bftextview2_spell_init(void) {
	eb = enchant_broker_init();

	
}

static void bftextview2_spell_cleanup(void) {
	enchant_broker_free(eb);
	eb=NULL;
}

#endif /*HAVE_ENCHANT*/
