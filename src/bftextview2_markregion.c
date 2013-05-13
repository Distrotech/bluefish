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

/* markregion design:

the markregion code is used to mark which characters of the BluefishTextView need
to be re-scanned for syntax, or for spell checking. Each document has a
separate Tregions structure for syntax scanning and spell checking in BluefishTextView

The Tregions simply point to a head and tail of a double linked list of Tchange elements.

After syntax scanning is complete and spell checking is complete these regions are empty.

So usually the list is very short, and if the list is long (for example after tabs-to-spaces
or another function that does many text changes in one go) it is usually only appended to
because these functions start in the beginning of the text and work towards the end. That's
why a linked list is good enough, and a balanced tree would probably be worse in performance.

The list has two entries for each sequence of characters that need rescanning/spellchecking,
one entry positions the start, the second entry positions the end.
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

#ifdef DEVELOPMENT

void
bftextview2_dump_needscanning(BluefishTextView *btv) {
	gboolean cont=TRUE;
	Tchange *cso, *ceo;
	guint so,eo;
	GtkTextIter start,end;
	g_print("bftextview2_dump_needscanning, started, markregion has head(%d)|tail(%d)\n",btv->scanning.head?CHANGE(btv->scanning.head)->pos:-1
						,btv->scanning.tail?CHANGE(btv->scanning.tail)->pos:-1);
	gtk_text_buffer_get_start_iter(btv->buffer, &start);
	cso = CHANGE(btv->scanning.head);
	while (cont) {
		if (!gtk_text_iter_begins_tag(&start, btv->needscanning)) {
			if (!gtk_text_iter_forward_to_tag_toggle(&start, btv->needscanning)) {
				cont=FALSE;
				if (cso) {
					g_print("ABORT: needscanning has no further region, markregion has cso=%d, ceo=%d\n",cso->pos, cso->next?CHANGE(cso->next)->pos:-1);
					g_assert_not_reached();
				}
				break;
			}
		}
		end = start;
		gtk_text_iter_forward_char(&end);
		if (!gtk_text_iter_ends_tag(&end, btv->needscanning)) {
			if (!gtk_text_iter_forward_to_tag_toggle(&end, btv->needscanning)) {
				g_print("huh2?\n");
			}
		}
		ceo = CHANGE(cso->next);
		so = gtk_text_iter_get_offset(&start);
		eo = gtk_text_iter_get_offset(&end);
		if(so != cso->pos || eo != ceo->pos) {
			g_print("ABORT: needscanning has %u:%u, markregion has %u:%u\n",so,eo,cso->pos,ceo->pos);
			g_assert_not_reached();
		}
		g_print("region %u:%u\n",so,eo);
		start = end;
		cso = ceo->next;
		cont = gtk_text_iter_forward_char(&start);
		if (!cont && cso) {
			g_print("ABORT: buffer has no further characters, markregion has cso=%d, ceo=%d\n",cso->pos, cso->next?CHANGE(cso->next)->pos:-1);
			g_assert_not_reached();
		}
	}
/*	g_print("*****\n");*/
}


static void
markregion_verify_integrity(Tregions *rg)
{
	Tchange *end, *start = rg->head;
	g_print("markregion_verify_integrity, started, head(%d)|tail(%d)\n",rg->head?CHANGE(rg->head)->pos:-1
						,rg->tail?CHANGE(rg->tail)->pos:-1);
	if (!rg->head) {
		if (rg->tail || rg->last) {
			g_print("ERROR: markregion_verify_integrity, !rg->head but there is a tail (%p) or last (%p) !?!?!\n",rg->tail, rg->last);
			g_assert_not_reached();
		}
		return;
	}

/*	g_print("* * * * *\n");*/
	if (CHANGE(rg->head)->prev) {
		g_print("ERROR: markregion_verify_integrity, rg->head has a previous entry !?!?!\n");
		g_assert_not_reached();
	}
	while (start) {
		if (start->is_start ==FALSE) {
			g_print("ERROR: markregion_verify_integrity, entry(%u)->is_start==FALSE but should start a region?!?\n",start->pos);
			g_assert_not_reached();
		}
		end = start->next;
		if (!end) {
			g_print("ERROR: markregion_verify_integrity, entry(%u) does not have an end?!?\n",start->pos);
			g_assert_not_reached();
		}
		if (end->prev != start) {
			g_print("ERROR: markregion_verify_integrity, end(%u)->prev (%p) does not point to start(%u) (%p)?!?\n",end->pos,end->prev,start->pos,start);
			g_assert_not_reached();
		}
/*		g_print("verifying region %u:%u\n",start->pos,end->pos);*/

		if (end->is_start == TRUE) {
			g_print("ERROR: markregion_verify_integrity, entry(%u)->is_start==TRUE but should end a region?!?\n",end->pos);
			g_assert_not_reached();
		}
		if (start->pos >= end->pos) {
			g_print("ERROR: markregion_verify_integrity, start at %u but end at %u ?!?\n",start->pos,end->pos);
			g_assert_not_reached();
		}
		start = end->next;
		if (start) {
			if (start->prev != end) {
				g_print("ERROR: markregion_verify_integrity, start->prev does not point to previous end?!?\n");
				g_assert_not_reached();
			}
			if (end->pos >= start->pos) {
				g_print("ERROR: markregion_verify_integrity, next start at %u but last end at %u ?!?\n",start->pos,end->pos);
				g_assert_not_reached();
			}
		} else {
			if (end != rg->tail) {
				g_print("ERROR: markregion_verify_integrity, rg->tail(%u) does not equal the last entry %u ?!?\n",CHANGE(rg->tail)->pos, end->pos);
				g_assert_not_reached();
			}

		}
	}
/*	g_print("* * * * *\n");*/
}
#endif

static Tchange *
new_change(guint pos, gboolean is_start)
{
	Tchange *change = g_slice_new(Tchange);
	change->next = change->prev = NULL;
	change->pos = pos;
	change->is_start = is_start;
	return change;
}

void markregion_region_done(Tregions *rg, guint end)
{
	Tchange *tmp;
	g_print("markregion_region_done, end=%u\n",end);
	if (!rg->head) {
		return;
	}
	tmp = rg->head;
	while (tmp && tmp->pos <= end) {
		Tchange *toremove = tmp;
		g_print("markregion_region_done, remove change with pos=%u\n",tmp->pos);
		tmp = CHANGE(bf_elist_remove(BF_ELIST(tmp))); /* returns the previous entry if that exists, but
										it does not exist in this case because we remove all entries */
		g_slice_free(Tchange, toremove);
	}
	if (tmp && tmp->is_start == FALSE) {
		/* we cannot begin our list of regions with an end-of-region, so remove it, or prepend a start position in front of it */
		g_print("next change at %u is a end!\n",tmp->pos);
		if (tmp->pos == end) {
			Tchange *toremove = tmp;
			g_print("markregion_region_done, remove change with pos=%u\n",toremove->pos);
			tmp = CHANGE(bf_elist_remove(BF_ELIST(toremove)));
			g_slice_free(Tchange, toremove);
		} else {
			g_print("markregion_region_done, prepend change with end=%u\n",end);
			tmp = CHANGE(bf_elist_prepend(BF_ELIST(tmp), BF_ELIST(new_change(end, TRUE))));
		}
	} else {
#ifdef DEVELOPMENT
		if (tmp) {
			g_print("markregion_region_done, keep %u, is_start=%d\n",tmp->pos,tmp->is_start);
		}
#endif
	}
	rg->head = tmp;
	rg->last = NULL;
	if (tmp == NULL) {
		rg->tail = NULL;
	}
	g_print("markregion_region_done, return, head(%d)|tail(%d)\n",rg->head?CHANGE(rg->head)->pos:-1
						,rg->tail?CHANGE(rg->tail)->pos:-1);
}

static void
update_offset(Tchange *start, gint offset)
{
	if (offset == 0)
		return;
	while (start) {
		start->pos += offset;
		start = start->next;
	}
}

static Tchange *
find_prev_or_equal_position(Tregions *rg, guint position)
{
	if (!rg->last) {
		if (position - CHANGE(rg->head)->pos < CHANGE(rg->tail)->pos - position) {
			rg->last = rg->head;
		} else {
			rg->last = rg->tail;
		}
	}
#ifdef DEVELOPMENT
	if (!rg->last) {
		g_print("ABORT: find_prev_or_equal_position, rg->last==NULL ?!?!?!\n");
		g_assert_not_reached();
	}
#endif
	if (CHANGE(rg->last)->pos == position)
		return CHANGE(rg->last);

	if (CHANGE(rg->last)->pos > position) {
/*		g_print("find_prev_or_equal_position, current position %u is beyond the requested position %u\n",CHANGE(rg->last)->pos,position);*/
		while (rg->last && CHANGE(rg->last)->pos > position) {
/*			g_print("backward start %p(%u) to %p\n",CHANGE(rg->last),CHANGE(rg->last)->pos,CHANGE(rg->last)->prev);*/
			rg->last = CHANGE(rg->last)->prev;
		}
	} else {
		Tchange *next = CHANGE(rg->last)->next;
/*		g_print("find_prev_or_equal_position, current position %u is before the requested position %u\n",CHANGE(rg->last)->pos,position);*/
		while (next && next->pos <= position) {
/*			g_print("forward start %p(%u) to %p(%u), next->next=%p\n",CHANGE(rg->last),CHANGE(rg->last)->pos,next,next->pos,next->next);*/
			rg->last = next;
			next = next->next;
		}
	}
	g_print("find_prev_or_equal_position, requested position %u, returning change->pos=%d\n",position,rg->last?CHANGE(rg->last)->pos:-1);
	return CHANGE(rg->last);
}

static gboolean
markregion_handle_generic(Tregions *rg, guint markstart, guint markend, guint comparepos, gint offset)
{
	if (!rg->head) {
		/* empty region, just append the start and end */
		g_print("markregion_handle_generic, empty, just add the first entries\n");
		rg->head = new_change(markstart, TRUE);
		rg->last = rg->tail = bf_elist_append(BF_ELIST(rg->head), BF_ELIST(new_change(markend, FALSE)));
		return TRUE;
	}
	if (CHANGE(rg->tail)->pos < markstart) {
		g_print("markregion_handle_generic, markstart (%u) beyond the end (%u), append new entries\n", markstart, CHANGE(rg->tail)->pos);
		/* the new region is beyond the end */
		rg->last = rg->tail = bf_elist_append(BF_ELIST(rg->tail), BF_ELIST(new_change(markstart, TRUE)));
		rg->tail = bf_elist_append(BF_ELIST(rg->tail), BF_ELIST(new_change(markend, FALSE)));
		return TRUE;
	}
	if (CHANGE(rg->head)->pos > comparepos) {
		Tchange *oldhead = rg->head;
		g_print("markregion_handle_generic, comparepos (%u) before the head (%u), prepend new entries and update offsets\n",comparepos,CHANGE(rg->head)->pos);
		/* the first region is before the current start */
		rg->last = rg->head = bf_elist_prepend(BF_ELIST(rg->head), BF_ELIST(new_change(markend, FALSE)));
		rg->head = bf_elist_prepend(BF_ELIST(rg->head), BF_ELIST(new_change(markstart, TRUE)));
		update_offset(oldhead, offset);
		return TRUE;
	}
	return FALSE;
}

void
markregion_insert(Tregions *rg, guint markstart, guint markend)
{
	gint offset = markend-markstart;
#ifdef DEVELOPMENT
	markregion_verify_integrity(rg);
	if (markstart > markend) {
		g_print("ABORT: markregion_insert, markstart > markend ?!?!?!\n");
		g_assert_not_reached();
	}
#endif
	g_print("markregion_insert, markstart=%u, markend=%u, offset=%d\n",markstart,markend,offset);
	if (markstart == markend)
		return;

	if (markregion_handle_generic(rg, markstart, markend, markstart, offset))
		return;

	/* insert somewhere within the existing regions */
	rg->last = find_prev_or_equal_position(rg, markstart);
	if (!rg->last && CHANGE(rg->head)->pos > markstart) {
		rg->head = rg->last = bf_elist_prepend(BF_ELIST(rg->head), BF_ELIST(new_change(markstart, TRUE)));
	}
	if (CHANGE(rg->last)->is_start == TRUE) {
		/* only update the offset */
		g_print("update offset, starting at %u\n",CHANGE(CHANGE(rg->last)->next)->pos);
		update_offset(CHANGE(rg->last)->next, offset);
		return;
	}
	if (CHANGE(rg->last)->pos == markstart) {
		Tchange *toremove;
		/* out current start previously ended a region, merge the old and new regions together */
		toremove = rg->last;
 		rg->last = bf_elist_remove(toremove); /* returns 'change->prev' if there is one */
 		g_slice_free(Tchange,toremove);
		rg->last = bf_elist_append(BF_ELIST(rg->last), BF_ELIST(new_change(markend, FALSE)));
		if (CHANGE(rg->last)->next) {
			update_offset(CHANGE(rg->last)->next, offset);
		} else {
			rg->tail = rg->last;
		}
		return;
	}
	if (CHANGE(rg->last)->pos < markstart) {
		rg->last = bf_elist_append(BF_ELIST(rg->last), BF_ELIST(new_change(markstart, TRUE)));
		rg->last = bf_elist_append(BF_ELIST(rg->last), BF_ELIST(new_change(markend, FALSE)));
		update_offset(CHANGE(rg->last)->next, offset);
		return;
	}
#ifdef DEVELOPMENT
	g_print("ABORT: markregion_insert, we should never get to the end of this function\n");
	g_assert_not_reached();
#endif
}

void
markregion_delete(Tregions *rg, guint markstart, guint markend, gint offset)
{
	gint comparepos = markend-offset;
	Tchange *oldlast;

#ifdef DEVELOPMENT
	markregion_verify_integrity(rg);
	if (markstart > markend) {
		g_print("ABORT: markregion_insert, markstart > markend ?!?!?!\n");
		g_assert_not_reached();
	}
#endif
	g_print("markregion_delete, %u:%u, update offset with %d, comparepos=%d. head(%d)|tail(%d)\n",markstart,markend,offset,comparepos
						,rg->head?CHANGE(rg->head)->pos:-1
						,rg->tail?CHANGE(rg->tail)->pos:-1);
	if (markstart == 0 && markend == 0 && rg->tail) {
		rg->last = rg->head;
		while(rg->last && CHANGE(rg->last)->pos <= comparepos) {
			Tchange *toremove = CHANGE(rg->last);
			rg->last = CHANGE(rg->last)->next;
			bf_elist_remove(BF_ELIST(toremove));
			g_print("markregion_delete, remove pos=%u, because it is lower than %d\n",toremove->pos,comparepos);
			g_slice_free(Tchange, toremove);
		}
		rg->head = rg->last;
		if (!rg->last) {
			rg->tail = NULL;
		}
	}
	if (markstart == markend) {
		 return;
	}

	if (markregion_handle_generic(rg, markstart, markend, comparepos, offset))
		return;

	rg->last = find_prev_or_equal_position(rg, markstart);
	if (rg->last == NULL) {
		/* starts before head */
		g_print("markregion_delete, prepend start %u to head (%u)\n",markstart,rg->head?CHANGE(rg->head)->pos:-1);
		rg->last = rg->head = bf_elist_prepend(BF_ELIST(rg->head), BF_ELIST(new_change(markstart, TRUE)));
	}

	oldlast = rg->last;
	rg->last = CHANGE(rg->last)->next;
	/* oldlast has he position _before_ the deleted area */
	if (CHANGE(oldlast)->is_start == FALSE) {
		if (oldlast->pos == markstart) {
			Tchange *toremove = oldlast;
			/* previous region ends at our start, merge the previous and this region together */
			oldlast = CHANGE(bf_elist_remove(BF_ELIST(oldlast))); /* returns 'change->prev' if there is one */
			g_slice_free(Tchange,toremove);
		} else {
			/* previous region ended before our start, start a new region */
			oldlast = CHANGE(bf_elist_append(BF_ELIST(oldlast), BF_ELIST(new_change(markstart, TRUE))));
		}
	}
	/*now remove entries in the deleted area */
	while(rg->last && CHANGE(rg->last)->pos < comparepos) {
		Tchange *toremove = CHANGE(rg->last);
		rg->last = CHANGE(rg->last)->next;
		bf_elist_remove(BF_ELIST(toremove));
/*		g_print("markregion_delete, remove pos=%u, because it is lower than %d\n",toremove->pos,comparepos);*/
		g_slice_free(Tchange, toremove);
	}
	/* after the delete loop rg->last points to a position equal or beyond comparepos, or NULL if there was no following position */
	if (!rg->last) {
		rg->tail = rg->last = CHANGE(bf_elist_append(BF_ELIST(oldlast), BF_ELIST(new_change(markend, FALSE))));
		return;
	}

	if (rg->last && CHANGE(rg->last)->pos == comparepos && CHANGE(rg->last)->is_start == TRUE) {
		/* if rg->last equals comparepos and is a start, we can remove it to merge the regions. if it is a start,
		there should also be an end, so removing this entry should not delete the tail! */
		Tchange *toremove = CHANGE(rg->last);
		rg->last = CHANGE(rg->last)->next;
#ifdef DEVELOPMENT
		g_assert(rg->last);
#endif
		bf_elist_remove(BF_ELIST(toremove));
		g_print("markregion_delete, remove pos=%u, because it equals %d and starts the next region (merge them)\n",toremove->pos,comparepos);
		g_slice_free(Tchange, toremove);
		update_offset(rg->last, offset);
		return;
	}
	/*g_print("rg->last(%u) is_start=%d, oldlast(%u) is_start=%d\n",CHANGE(rg->last)->pos,CHANGE(rg->last)->is_start,oldlast->pos,oldlast->is_start);*/
	if (CHANGE(rg->last)->is_start == TRUE) {
		bf_elist_prepend(BF_ELIST(rg->last), BF_ELIST(new_change(markend, FALSE)));
	}
	update_offset(rg->last, offset);
}

void
markregion_nochange(Tregions *rg, guint markstart, guint markend)
{
	g_print("markregion_nochange, markstart=%u, markend=%u\n",markstart,markend);
	markregion_delete(rg, markstart, markend, 0);
}

gpointer
markregion_get_region(Tregions *rg, gpointer cur, guint *start, guint *end)
{
#ifdef DEVELOPMENT
	markregion_verify_integrity(rg);
#endif
	g_print("markregion_get_region, cur->pos(%d), head(%d)|tail(%d)\n",cur?CHANGE(cur)->pos:-1
						,rg->head?CHANGE(rg->head)->pos:-1
						,rg->tail?CHANGE(rg->tail)->pos:-1);
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
	g_print("markregion_get_region, start=%u,end=%u,cur->pos(%d)\n",*start,*end,cur?CHANGE(cur)->pos:-1);
	return cur;
}

#endif
