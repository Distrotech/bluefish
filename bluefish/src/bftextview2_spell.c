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
#include "bluefish.h"

static EnchantBroker *eb;

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

static gboolean run_spellcheck(void) {
	
	
}

static void bftextview2_spell_init(void) {
	eb = enchant_broker_init();

	
}

static void bftextview2_spell_cleanup(void) {
	enchant_broker_free(eb);
	eb=NULL;
}

#endif /*HAVE_ENCHANT*/
