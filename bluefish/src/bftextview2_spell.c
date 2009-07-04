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
}

static gboolean run_spellcheck(BluefishTextView * btv) {
	GtkTextIter so,eo,iter;
	GtkTextBuffer *buffer;
	Tfoundstack *fstack;
	GSequenceIter *siter=NULL;
	GTimer *timer;
	gint loops=0;
	gint loops_per_timer=100;
	guint16 contextnum=1;
	gboolean cont=TRUE;
	
	buffer = 
	if (!bftextview2_find_region2spellcheck(btv, buffer, &so, &eo));
		return FALSE;

	timer = g_timer_new();
	
	fstack = get_stackcache_at_position(btv, &so, &siter); 
	contextnum = g_queue_get_length(fstack->contextstack) ? GPOINTER_TO_INT(g_queue_peek_head(fstack->contextstack)): 1;
	need_spellcheck = g_array_index(btv->bflang->st->contexts,Tcontext, contextnum).need_spellcheck;
	
	do {
		if (need_spellcheck) {
			/* move to the next word, but check in the meantime that we're not entering a context that doesn't need spellchecking */
			
			
		} else {
			/* move in the B-tree to the next context that needs spellchecking */
			
		}
		loops++;
	} while (loop%loops_per_timer!=0 || g_timer_elapsed(timer,NULL)<MAX_CONTINUOUS_SPELLCHECK_INTERVAL);
	
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
