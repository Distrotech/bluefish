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
#include "bftextview2_private.h"
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
		the previous start is valid
		if there is no tmp we append to the tail */
		if (!tmp) {
			rg->tail = tmp = CHANGE(bf_elist_append(BF_ELIST(rg->tail), BF_ELIST(new_change(start, TRUE))));
		} else if (tmp->is_start == TRUE) {
			tmp = (Tchange *)bf_elist_prepend(BF_ELIST(tmp), new_change(start, TRUE));
		} else {
			tmp = tmp->prev;
		}
	}
	/* tmp now points to the start position, continue to the end position */
	while (tmp && tmp->pos < end) {
		Tchange *toremove = tmp;
		tmp = CHANGE(bf_elist_remove(BF_ELIST(toremove))); /* returns the previous entry if there is one */
		g_slice_free(Tchange, toremove);
		tmp = tmp->next;
	}
	if (!tmp) {
		rg->tail = bf_elist_append(BF_ELIST(rg->tail), BF_ELIST(new_change(end, FALSE)));
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
			bf_elist_prepend(BF_ELIST(tmp), BF_ELIST(new_change(end, FALSE)));
		}
	}
}

void
mark_region_changed_real(Tregions *rg, guint start, guint end)
{
	g_print("mark_region_changed, %u:%u\n",start,end);
	if (!rg->head) {
		rg->head = new_change(start, TRUE);
		rg->tail = bf_elist_append(BF_ELIST(rg->head), BF_ELIST(new_change(end, FALSE)));
		return;
	}

	if (CHANGE(rg->tail)->pos < start) {
		rg->tail = bf_elist_append(BF_ELIST(rg->tail), BF_ELIST(new_change(start, TRUE)));
		rg->tail = bf_elist_append(BF_ELIST(rg->tail), BF_ELIST(new_change(end, FALSE)));
		return;
	} else if (CHANGE(rg->head)->pos > end){
		rg->head = bf_elist_prepend(BF_ELIST(rg->head), BF_ELIST(new_change(end, FALSE)));
		rg->head = bf_elist_prepend(BF_ELIST(rg->head), BF_ELIST(new_change(start, TRUE)));
		return;
	}

	insert_start_and_end(rg, start, end);
}

void mark_region_done(Tregions *rg, guint end)
{
	Tchange *tmp;
	g_print("mark_region_done, end=%u\n",end);
	if (!rg->head) {
		return;
	}
	tmp = rg->head;
	while (tmp && tmp->pos <= end) {
		g_print("mark_region_done, remove change with pos=%u\n",tmp->pos);
		tmp = CHANGE(bf_elist_remove(BF_ELIST(tmp))); /* returns the previous entry if that exists, but
										it does not exist in this case because we remove all entries */
	}
	if (tmp && tmp->is_start == FALSE) {
		g_print("next change at %u is a end!\n",tmp->pos);
		if (tmp->pos == end) {
			Tchange *toremove = tmp;
			g_print("mark_region_done, remove change with pos=%u\n",toremove->pos);
			tmp = CHANGE(bf_elist_remove(BF_ELIST(toremove)));
			g_slice_free(Tchange, toremove);
		} else {
			g_print("mark_region_done, prepend change with end=%u\n",end);
			tmp = CHANGE(bf_elist_prepend(BF_ELIST(tmp), BF_ELIST(new_change(end, TRUE))));
		}
	}
	rg->head = tmp;
	if (tmp == NULL) {
		rg->tail = NULL;
	}
}

gpointer
update_offset_real(Tregions *rg, gpointer cur, guint start , gint offset, guint nextpos)
{
	g_print("update_offset, start=%u, offset=%d, nextpos=%u\n",start,offset,nextpos);
	if (cur == NULL) {
		cur = rg->head;
		while (cur && CHANGE(cur)->pos <= start) {
			cur = CHANGE(cur)->next;
		}
	}
	g_print("update_offset, start at cur->pos=%u\n",cur ? CHANGE(cur)->pos : -1);
	while (cur && CHANGE(cur)->pos+offset < nextpos) {
		g_print("update_offset, update cur->pos=%u to %u\n",CHANGE(cur)->pos, CHANGE(cur)->pos + offset);
		CHANGE(cur)->pos += offset;
		cur = CHANGE(cur)->next;
	}
	return cur;
}

typedef enum {
	cache_changed,
	cache_offset
} Tcachetype;

typedef struct {
	BF_ELIST_HEAD;
	Tcachetype type;
	gint val1;
	gint val2;
} Tcache;

static guint
cache_get_nextpos(Tcache *startch)
{
	Tcache *ch = startch->next;
	while (ch) {
		if (ch->type == cache_offset) {
			return ch->val1;
		}
		ch = ch->next;
	}
	return BF2_OFFSET_UNDEFINED;
}

static gboolean
process_cache(Tregions *rg)
{
	Tcache *ch, *nextch;
	gpointer cur=NULL;
	gint handleoffset=0;
	if (!rg->cachehead)
		return FALSE;

	ch = rg->cachehead;
	while (ch) {
		if (ch->type == cache_changed) {
			mark_region_changed_real(rg, ch->val1, ch->val2);
		} else if (ch->type == cache_offset) {
			guint nextpos = cache_get_nextpos(ch);
			handleoffset += ch->val2;
			cur = update_offset_real(rg, cur, ch->val1 , handleoffset, nextpos);
		} else {
#ifdef DEVELOPMENT
			g_assert_not_reached();
#endif
		}
		nextch = ch->next;
		g_slice_free(Tcache, ch);
		ch = nextch;
	}
	rg->cachehead = rg->cachetail = NULL;
	return TRUE;
}

static void
add_to_cache(Tregions *rg, Tcachetype type, gint val1, gint val2)
{
	Tcache *ch = g_slice_new(Tcache);
	ch->prev = ch->next = NULL;
	ch->val1 = val1;
	ch->val2 = val2;
	ch->type = type;
	rg->cachetail = bf_elist_append(rg->cachetail, ch);
	if (!rg->cachehead)
		rg->cachehead = rg->cachetail;
}

void
mark_region_changed(Tregions *rg, guint start, guint end)
{
	add_to_cache(rg, cache_changed, start, end);
}

void
update_offset(Tregions *rg, guint start , gint offset)
{
	add_to_cache(rg, cache_offset, start, offset);
}

static void
update_offset(Tchange *start, gint offset)
{
	while (start) {
		start->pos += offset;
		start = start->next;
	}
}

void
add_region(Tregions *rg, guint markstart, guint markend, guint pos, gint offset)
{

	g_print("add_region, %u:%u, from pos=%u update offset with %d\n",markstart,markend,pos,offset);
	if (!rg->head) {
		/* empty region, just append the start and end */
		rg->head = new_change(markstart, TRUE);
		rg->tail = bf_elist_append(BF_ELIST(rg->head), BF_ELIST(new_change(markend, FALSE)));
		return;
	}
	if (CHANGE(rg->tail)->pos < markstart) {
		/* the new region is beyond the end */
		rg->tail = bf_elist_append(BF_ELIST(rg->tail), BF_ELIST(new_change(markstart, TRUE)));
		rg->tail = bf_elist_append(BF_ELIST(rg->tail), BF_ELIST(new_change(markend, FALSE)));
		return;
	}
	if (CHANGE(rg->head)->pos > markend) {
		/* the first region is before the current start */
		Tchange *oldhead = rg->head;

		/* if the offset is negative, we might have to remove a few entries */
		while (rg->head->pos < pos && ) {

		}
		update_offset(oldhead, offset);



		rg->head = bf_elist_prepend(BF_ELIST(rg->head), BF_ELIST(new_change(markend, FALSE)));
		rg->head = bf_elist_prepend(BF_ELIST(rg->head), BF_ELIST(new_change(markstart, TRUE)));
		return;
	}
	if (CHANGE(rg->tail)->pos <= markend) {
		/* the new region ends beyond the end, but starts before */

		/* first find the place where to start it */
		while (rg->tail && CHANGE(rg->tail)->pos > markstart) {
			rg->tail = bf_elist_remove(rg->tail); /* returns 'change->prev' if there is one */
		}
		if (CHANGE(rg->tail)->is_start != TRUE) {
			if (CHANGE(rg->tail)->pos == markstart) {
				rg->tail = bf_elist_remove(CHANGE(rg->tail));
			} else {
				rg->tail = bf_elist_append(BF_ELIST(rg->tail), BF_ELIST(new_change(markstart, TRUE)));
			}
		}
		rg->tail = bf_elist_append(BF_ELIST(rg->tail), BF_ELIST(new_change(markend, FALSE)));
		return;
	}

	if (CHANGE(rg->head)->pos > markstart) {
		/* the new region begins beyond the begin, but ends after */
		/* first find the place where to end it */
		while (rg->head && CHANGE(rg->head)->pos < markend) {
			rg->head = bf_elist_remove(rg->head); /* returns 'change->prev' if there is one, but there is none, so it will return 'next' */
		}
		if (CHANGE(rg->head)->is_start == TRUE) {
			if (CHANGE(rg->head)->pos == markend) {
				rg->head = bf_elist_remove(CHANGE(rg->head));
			} else {
				rg->head = bf_elist_prepend(BF_ELIST(rg->head), BF_ELIST(new_change(markend, FALSE)));
			}
		}
		rg->head = bf_elist_prepend(BF_ELIST(rg->head), BF_ELIST(new_change(markstart, TRUE)));
		return;
	}

	/* the most complex case: it somewhere in the middle of existing regions */

}


gpointer
get_region(Tregions *rg, gpointer cur, guint *start, guint *end)
{
	if (process_cache(rg)) {
		cur = NULL;
	}

	if (cur == NULL) {
		if (rg->head==NULL) {
			*start = BF2_OFFSET_UNDEFINED;
			*end = BF2_OFFSET_UNDEFINED;
			return NULL;
		}
		cur = rg->head;
	}

#ifdef DEVELOPMENT
	if (!CHANGE(cur)->is_start) {
		g_print("ABORT: get_region is called, and cur is not a start of region\n");
		g_assert_not_reached();
	}
#endif
	*start = CHANGE(cur)->pos;
	cur = CHANGE(cur)->next;
#ifdef DEVELOPMENT
	if (!cur) {
		g_print("ABORT: get_region is called, and next cur does not exist\n");
		g_assert_not_reached();
	}
	if (CHANGE(cur)->is_start) {
		g_print("ABORT: get_region is called, and next cur is a start of region\n");
		g_assert_not_reached();
	}
#endif
	*end = CHANGE(cur)->pos;
	cur = CHANGE(cur)->next;
	return cur;
}


#endif

