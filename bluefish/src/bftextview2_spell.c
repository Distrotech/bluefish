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

#ifdef HAVE_LIBENCHANT
#ifdef HAVE_ENCHANT_ENCHANT_H
#include <enchant/enchant.h>
#else /* HAVE_ENCHANT_ENCHANT_H */
#include <enchant.h>
#endif /* HAVE_ENCHANT_ENCHANT_H */

#include "bftextview2_scanner.h"

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
	g_print("spellcheck_word, check word %s\n",tocheck);
	if (enchant_dict_check(ed, tocheck, g_utf8_strlen(tocheck,-1)) != 0) {
		g_print("%s *not* spelled correctly!\n", tocheck);
		gtk_text_buffer_apply_tag_by_name(buffer, "_spellerror_", start, end);
	} else {
		g_print("%s spelled correctly!\n", tocheck);
	}
	g_free(tocheck);
}

static gint foundstack_needspellcheck(BluefishTextView * btv, Tfoundstack *fstack) {
	guint16 contextnum;
	if (g_queue_get_length(fstack->contextstack))
		contextnum = ((Tfoundcontext *)g_queue_peek_head(fstack->contextstack))->context;
	else
		contextnum = 1;
	g_print("context %d has spellcheck=%d\n",contextnum, g_array_index(btv->bflang->st->contexts,Tcontext, contextnum).needspellcheck);
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

gboolean bftextview2_run_spellcheck(BluefishTextView * btv) {
	GtkTextIter so,eo,iter;
	GtkTextBuffer *buffer;
	Tfoundstack *fstack;
	GSequenceIter *siter=NULL;
	GTimer *timer;
	gint loop=0, loops_per_timer=100;
	guint eo_offset;
	gboolean cont=TRUE;
	
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
	if (!bftextview2_find_region2spellcheck(btv, buffer, &so, &eo)) {
		g_print("nothing to spellcheck...\n");
		return FALSE;
	}
	g_print("loop1 from %d to %d\n",gtk_text_iter_get_offset(&so),gtk_text_iter_get_offset(&eo));
	timer = g_timer_new();
	iter=so;
	eo_offset = gtk_text_iter_get_offset(&eo);
	do {
		GtkTextIter eo2=eo;
		gboolean cont2=TRUE;
		/**** find the start within this region of a context that needs spellcheck */
		fstack = get_stackcache_at_position(btv, &iter, &siter);
		if (fstack || !g_array_index(btv->bflang->st->contexts,Tcontext, 1).needspellcheck) {
			if(!fstack) {
				fstack = get_stackcache_first(btv, &siter);
				if (fstack->charoffset > eo_offset) {
					/* nothing to do ! */
					g_print("next starting fstack beyond eo (1), we're finished\n");
					cont=FALSE;
					cont2=FALSE;
					iter=eo;
				}
			}
			while (fstack && fstack->charoffset < eo_offset && !foundstack_needspellcheck(btv,fstack)) {
				fstack = get_stackcache_next(btv, &siter);
			}
			
			if (fstack && fstack->charoffset < eo_offset) {
				/* advance the iter to the charoffset of fstack, but keep at the same place if the iter is already further */
				if (fstack->charoffset > gtk_text_iter_get_offset(&iter)) {
					gtk_text_iter_set_offset(&iter, fstack->charoffset);
					g_print("advance iter to %d\n",gtk_text_iter_get_offset(&iter));
				}
			} else {
				g_print("next starting fstack beyond eo (2), we're finished\n");
				cont=FALSE;
				cont2=FALSE;
				iter=eo;
			}
		} else { /* no fstack and the default context needs spellcheck */
			g_print("default context, keep iter at %d\n",gtk_text_iter_get_offset(&iter));
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
				g_print("set eo2 to %d\n",gtk_text_iter_get_offset(&eo2));
			} else {
				/* no next, set end iter to eo */
				eo2=eo;
				g_print("set eo2 to eo at %d\n",gtk_text_iter_get_offset(&eo2));
			}
		}
		g_print("loop2 from %d to %d\n",gtk_text_iter_get_offset(&iter),gtk_text_iter_get_offset(&eo2));
		while (cont2 && (loop%loops_per_timer!=0 || g_timer_elapsed(timer,NULL)<MAX_CONTINUOUS_SPELLCHECK_INTERVAL)) { /* loop from iter to eo2 */
			loop++;
			if (gtk_text_iter_forward_word_end(&iter) && gtk_text_iter_compare(&iter, &eo2) <= 0) {
				GtkTextIter wordstart;
				gtk_text_iter_backward_word_start(&iter);

				wordstart=iter;
				/* move to the next word as long as the we don't hit eo2 */
				if (text_iter_forward_real_word_end(&iter)) {
					/* check word */
					spellcheck_word(btv, buffer, &wordstart, &iter);
				}
			} else {
				g_print("no word start within region\n");
				iter=eo2;
				cont2=FALSE;
			}
			if (cont2 && gtk_text_iter_compare(&iter, &eo2) >= 0)
				cont2 = FALSE;
		}

		if (gtk_text_iter_compare(&iter, &eo) >= 0) /* TODO: check the order of the compare items */
			cont = FALSE;
			
	} while (cont && (loop%loops_per_timer!=0 || g_timer_elapsed(timer,NULL)<MAX_CONTINUOUS_SPELLCHECK_INTERVAL));
	
	gtk_text_buffer_remove_tag(buffer, btv->needspellcheck, &so , &iter);

	g_timer_destroy(timer);
	
	return TRUE;
}

void bftextview2_spell_init(void) {
	ed=NULL;
	eb = enchant_broker_init();
	if (!eb) {
		g_warning("could not initialize spell checking engine\n");
		return;
	}
	if (enchant_broker_dict_exists(eb,"nl"))
		ed = enchant_broker_request_dict(eb, "nl");
	else {
		g_warning("dictionary does not exist!\n");
	}

}

void bftextview2_spell_cleanup(void) {
	enchant_broker_free(eb);
	eb=NULL;
}

#endif /*HAVE_LIBENCHANT*/
