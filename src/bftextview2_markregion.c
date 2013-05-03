/* Bluefish HTML Editor
 * bftextview2_markregion.c
 *
 * Copyright (C) 2013 Olivier Sessink
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

#include "bluefish.h"
#include "bf_lib.h"
#include "bftextview2_markregion.h"

#ifdef MARKREGION

typedef struct {
	BF_ELIST_HEAD;
	guint32 pos;
	guint32 is_start; /* a guint8 would have been good enough, but 
								both on 32bit or 64 bit systems that doesn't affect 
								the size of Tchange, and this is faster on 32bit 
								systems */
} Tchange;
#define CHANGE(var) ((Tchange *)var)

static Tchange *
new_change(guint pos, gboolean is_start)
{
	Tchange *change = g_slice_new(Tchange);
	change->next = change->prev = NULL;
	change->pos = pos;
	change->is_start = is_start;
	return change;
}

static void
insert_start_and_end(Tregions *rg, guint start, guint end)
{
	Tchange *tmp = rg->head;
	/* insert start */
	if (tmp->pos > start) {
		rg->head = bf_elist_prepend(rg->head, new_change(start, TRUE));
		tmp = rg->head;
	} else {
		while (tmp && tmp->pos < start) {
			tmp = tmp->next;
		}
		/* if tmp->is_start == FALSE we don't need to insert a new start, because
		the previous start is valid */
		if (tmp->is_start == TRUE) {
			tmp = (Tchange *)bf_elist_prepend(BF_ELIST(tmp), new_change(start, TRUE));
		} else {
			tmp = tmp->prev;
		}
	}
	/* tmp now points to the start position, continue to the end position */
	while (tmp && tmp->pos < end) {
		Tchange *toremove = tmp;
		tmp = CHANGE(bf_elist_remove(BF_ELIST(toremove))); /* returns the previous entry */
		g_slice_free(Tchange, toremove);
		tmp = tmp->next;
	}
	if (!tmp) {
		rg->tail = bf_elist_append(BF_ELIST(rg->tail), BF_ELIST(new_change(start, FALSE)));
		return;
	}

	/* if tmp->is_start == FALSE we do not have to do anything:
		the already existing region ends on or beyond the region we are ending now, so use
		the existing end and merge them together */
	if (tmp->is_start == TRUE) {
		if (tmp->pos == end) {
			Tchange *toremove = tmp;
			/* the end of the current region starts the next region, simply remove
			the start of the next region so they form a continuous region */
			bf_elist_remove(BF_ELIST(toremove));
			g_slice_free(Tchange, toremove);
		} else {
			bf_elist_prepend(BF_ELIST(tmp), BF_ELIST(new_change(start, FALSE)));
		}
	}
}

void
mark_region_changed(Tregions *rg, guint start, guint end)
{
	if (!rg->head) {
		rg->head = new_change(start, TRUE);
		rg->tail = bf_elist_append(BF_ELIST(rg->head), BF_ELIST(new_change(start, FALSE)));
		return;
	}

	if (CHANGE(rg->tail)->pos < start) {
		rg->tail = bf_elist_append(BF_ELIST(rg->tail), BF_ELIST(new_change(start, TRUE)));
		rg->tail = bf_elist_append(BF_ELIST(rg->tail), BF_ELIST(new_change(start, FALSE)));
		return;
	} else if (CHANGE(rg->head)->pos > end){
		rg->head = bf_elist_prepend(BF_ELIST(rg->head), BF_ELIST(new_change(start, FALSE)));
		rg->head = bf_elist_prepend(BF_ELIST(rg->head), BF_ELIST(new_change(start, TRUE)));
		return;
	}

	insert_start_and_end(rg, start, end);
}

void mark_region_done(Tregions *rg, guint end)
{
	Tchange *tmp;
	if (!rg->head) {
		return;
	}
	tmp = rg->head;
	while (tmp && tmp->pos < end) {
		tmp = CHANGE(bf_elist_remove(BF_ELIST(tmp))); /* returns the previous entry if that exists, but
										it does not exist in this case because we remove all entries */
	}
	if (tmp && tmp->is_start == FALSE) {
		if (tmp->pos == end) {
			Tchange *toremove = tmp;
			tmp = CHANGE(bf_elist_remove(BF_ELIST(toremove)));
			g_slice_free(Tchange, toremove);
		} else {
			tmp = CHANGE(bf_elist_prepend(BF_ELIST(tmp), BF_ELIST(new_change(end, TRUE))));
		}
	}
	rg->head = tmp;
	if (tmp == NULL) {
		rg->tail = NULL;
	}
}

gpointer
update_offset(Tregions *rg, gpointer cur, guint start , gint offset, guint nextpos)
{
	if (cur == NULL) {
		cur = rg->head;
		while (cur && CHANGE(cur)->pos < start) {
			cur = CHANGE(cur)->next;
		}
	}
	while (cur && CHANGE(cur)->pos+offset < nextpos) {
		CHANGE(cur)->pos += offset;
		cur = CHANGE(cur)->next;
	}
	return cur;
}
#endif

