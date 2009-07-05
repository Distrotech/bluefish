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

#include "bluefish.h"

#ifdef HAVE_ENCHANT
#include <enchant/enchant.h>

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

static void spellcheck_word(BluefishTextView * btv, GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end) {
	gchar *tocheck;
	
	tocheck = gtk_text_buffer_get_text(buffer, start,end, FALSE);
	if (!ed) {
		g_print("spellcheck_word, should check word %s\n",tocheck);
		g_free(tocheck);
		return;
	}

	if (enchant_dict_check(ed, tocheck, g_utf8_strlen(tocheck,NULL)) != 0) {
		gtk_text_buffer_apply_tag_by_name(buffer, "_spellerror_", start, end);
	}
	g_free(tocheck);
}

static inline gint foundstack_needspellcheck(BluefishTextView * btv, Tfoundstack *fstack) {
	guint16 contextnum = g_queue_get_length(fstack->contextstack) ? GPOINTER_TO_INT(g_queue_peek_head(fstack->contextstack)): 1;
	return g_array_index(btv->bflang->st->contexts,Tcontext, contextnum).needspellcheck;
}

/* handle apostrophe in word gracefully */ 
static inline gboolean text_iter_forward_real_word_end(GtkTextIter *it) {
	GtkTextIter iter;
	if (!gtk_text_iter_forward_word_end(it))
		return FALSE;
	if (gtk_text_iter_get_char(it) != '\'')
		return TRUE;
	iter = *it;
	if (gtk_text_iter_forward_char(&iter)) {
		if (g_unichar_isalpha(gtk_text_iter_get_char(&iter))) {
			return (gtk_text_iter_forward_word_end(it));
		}
	}
	return TRUE;
}

static gboolean run_spellcheck(BluefishTextView * btv) {
	GtkTextIter so,eo,iter;
	GtkTextBuffer *buffer;
	Tfoundstack *fstack, *nextfstack;
	GSequenceIter *siter=NULL;
	GTimer *timer;
	gint loop=0, loops_per_timer=100;
	guint eo_offset;
	gboolean cont=TRUE, use_default_context=FALSE;
	
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
	if (!bftextview2_find_region2spellcheck(btv, buffer, &so, &eo));
		return FALSE;

	timer = g_timer_new();
	iter=so;
	eo_offset = gtk_text_iter_get_offset(&eo);
	do {
		GtkTextIter eo2;
		gboolean cont2=TRUE;
		/**** find the start within this region of a context that needs spellcheck */
		fstack = get_stackcache_at_position(btv, &iter, &siter);
		if (fstack || !g_array_index(btv->bflang->st->contexts,Tcontext, 1).needspellcheck) {
			if(!fstack) {
				fstack = get_stackcache_first(btv, &siter);
				if (fstack->charoffset > eo_offset) {
					/* nothing to do ! */
					cont=FALSE;
					cont2=FALSE;
					iter=eo;
				}
			}
			while (fstack && fstack->charoffset < eo_offset && !foundstack_needspellcheck(btv,fstack)) {
				fstack = get_stackcache_next(btv, &siter);
			}
			
			if (fstack && fstack->charoffset < eo_offset) {
				guint iter_offset;
				/* advance the iter to the charoffset of fstack, but keep at the same place if the iter is already further */
				if (fstack->charoffset > gtk_text_iter_get_offset(&iter)) {
					gtk_text_iter_set_offset(&iter, fstack->charoffset);
				}
			}
		} else { /* no fstack and the default context needs spellcheck */
			iter=so;
			fstack = get_stackcache_first(btv, &siter);
		}

		/***** now find the end of this context to spellcheck */
		if (cont && cont2) {
			while(fstack && fstack->charoffset < eo_offset && foundstack_needspellcheck(btv,fstack)) {
				fstack = get_stackcache_next(btv, &siter);
			}
			if (fstack && fstack->charoffset < eo_offset) {
				/* set eo2 to the end of the context(s) that needs spellcheck */
				gtk_text_iter_set_offset(&eo2, fstack->charoffset);
			} else {
				/* no next, set end iter to eo */
				eo2=eo;
			}
		}
		
		while (cont2 && (loop%loops_per_timer!=0 || g_timer_elapsed(timer,NULL)<MAX_CONTINUOUS_SPELLCHECK_INTERVAL)) { /* loop from iter to eo2 */
			GtkTextIter wordstart;

			loop++;
			gtk_text_iter_forward_word_start(&iter);
			wordstart=iter;
			/* move to the next word as long as the we don't hit eo2 */
			if (text_iter_forward_real_word_end(&iter)) {
				/* check word */
				spellcheck_word(btv, buffer, &wordstart, &iter);
			}
			if (gtk_text_iter_compare(&iter, &eo2) <= 0)
				cont2 = FALSE;
			 
		}

		if (gtk_text_iter_compare(&iter, &eo) <= 0) /* TODO: check the order of the compare items */
			cont = FALSE;
			
	} while (cont && (loop%loops_per_timer!=0 || g_timer_elapsed(timer,NULL)<MAX_CONTINUOUS_SPELLCHECK_INTERVAL));
	
	gtk_text_buffer_remove_tag(buffer, btv->needspellcheck, &so , &iter);

	g_timer_destroy(timer);
	
	return TRUE;
}

static void bftextview2_spell_init(void) {
	eb = enchant_broker_init();
	ed = enchant_broker_request_dict (eb, "NL");
	if (!ed) {
		g_warning("dictionary does not exist!\n");
	}

}

static void bftextview2_spell_cleanup(void) {
	enchant_broker_free(eb);
	eb=NULL;
}

#endif /*HAVE_ENCHANT*/
