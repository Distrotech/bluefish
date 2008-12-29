/* Bluefish HTML Editor
 * bftextview2_scanner.c
 *
 * Copyright (C) 2008 Olivier Sessink
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
#define HL_PROFILING
#ifdef HL_PROFILING
#include <sys/times.h>
#include <unistd.h>
#endif
/* for the design docs see bftextview2.h */
#include "bftextview2_scanner.h"

#define MAX_CONTINUOUS_SCANNING_INTERVAL 0.1 /* float in seconds */

typedef struct {
	GtkTextIter start;
	GtkTextIter end;
	guint16 patternum;
} Tmatch;

typedef struct {
	GQueue *contextstack;
	GQueue *blockstack;
	GTimer *timer;
	gint16 context;
} Tscanning;
#ifdef HL_PROFILING
typedef struct {
	gint longest_contextstack;
	gint longest_blockstack;
	gint numcontextstart;
	gint numcontextend;
	gint numblockstart;
	gint numblockend;
	gint numchars;
	gint numloops;
	gint fblock_refcount;
	gint fcontext_refcount;
	gint fstack_refcount;
} Thl_profiling;

Thl_profiling hl_profiling = {0,0,0,0,0,0,0,0,0,0,0};
#endif

guint loops_per_timer=1000; /* a tunable to avoid asking time too often. this is auto-tuned. */ 

/* sort function for the stackcache GSequence structure */
static gint stackcache_compare_charoffset(gconstpointer a,gconstpointer b,gpointer user_data) {
	return ((Tfoundstack *)a)->charoffset - ((Tfoundstack *)b)->charoffset;
}

Tfoundstack *get_stackcache_next(BluefishTextView * btv, GSequenceIter ** siter) {
	DBG_MSG("get_stackcache_next, *siter=%p\n",*siter);
	*siter = g_sequence_iter_next(*siter);
	if (*siter && !g_sequence_iter_is_end(*siter)) {
		return g_sequence_get(*siter);
	}
	return NULL;
}

Tfoundstack *get_stackcache_first(BluefishTextView * btv, GSequenceIter ** retsiter) {
	*retsiter = g_sequence_get_begin_iter(btv->scancache.stackcaches);
	if (*retsiter && !g_sequence_iter_is_end(*retsiter)) {
		return g_sequence_get(*retsiter);
	}
	return NULL;
}

Tfoundstack *get_stackcache_at_position(BluefishTextView * btv, GtkTextIter *position, GSequenceIter ** retsiter) {
	GSequenceIter* siter;
	Tfoundstack fakefstack;
	Tfoundstack *fstack=NULL;
	fakefstack.charoffset = gtk_text_iter_get_offset(position);
	DBG_SCANCACHE("get_stackcache_at_position for position %d\n",fakefstack.charoffset);
	siter = g_sequence_search(btv->scancache.stackcaches,&fakefstack,stackcache_compare_charoffset,NULL);
	if (!g_sequence_iter_is_begin(siter)) {

		/* now get the previous position, and get the stack at that position */
		DBG_SCANCACHE("search returned iter at position %d (cache length %d)\n",g_sequence_iter_get_position(siter),g_sequence_get_length(btv->scancache.stackcaches));
		siter = g_sequence_iter_prev(siter);
		DBG_SCANCACHE("prev returned iter at position %d (cache length %d)\n",g_sequence_iter_get_position(siter),g_sequence_get_length(btv->scancache.stackcaches));
		if (siter && !g_sequence_iter_is_end(siter)) {
			fstack = g_sequence_get(siter);
			if (retsiter)
				*retsiter = siter;
			DBG_SCANCACHE("found nearest stack %p with charoffset %d\n",fstack,fstack->charoffset);
		} else {
			DBG_SCANCACHE("no siter no stack\n");
		}
	} else
		DBG_SCANCACHE("got begin iter..\n");
	return fstack;
}

static void foundstack_update_positions(GtkTextBuffer *buffer, Tfoundstack *fstack) {

	GtkTextMark *mark=NULL;
	if (fstack->pushedblock)
		mark = fstack->pushedblock->end1;
		/* previously this had fstack->pushedblock->start1 but this caused a problem. if a fstack has BOTH context
		AND block, and you need to restore the correct context, the context that should have been started at
		fstack->pushedcontext->start is accidently started at fstack->pushedblock->start1.  */
	else if (fstack->poppedblock)
		mark = fstack->poppedblock->end2;
	else if (fstack->pushedcontext)
		mark = fstack->pushedcontext->start;
	else if (fstack->poppedcontext)
		mark = fstack->poppedcontext->end;
	if (mark) {
		GtkTextIter it1;
		gtk_text_buffer_get_iter_at_mark(buffer,&it1,mark);
		fstack->charoffset = gtk_text_iter_get_offset(&it1);
		fstack->line = gtk_text_iter_get_line(&it1);
	}
}

static void scancache_update_all_foreach(gpointer data, gpointer user_data) {
	foundstack_update_positions(GTK_TEXT_BUFFER(user_data), (Tfoundstack *)data);
}

static void scancache_update_all_positions(BluefishTextView *btv, GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end) {

	if (start && end) {
		GSequenceIter *siter;
		Tfoundstack *fstack;
		gint end_offset = gtk_text_iter_get_offset(end);
		fstack = get_stackcache_at_position(btv, start, &siter);
		if (fstack) {
			gboolean cont=TRUE;
			while (cont) {
				siter = g_sequence_iter_next(siter);
				if (siter && !g_sequence_iter_is_end(siter)) {
					fstack = g_sequence_get(siter);
					foundstack_update_positions(buffer, fstack);
					cont = (fstack->charoffset <= end_offset);
				} else {
					cont = FALSE;
				}
			}
		}
		btv->scancache.stackcache_need_update_charoffset = end_offset;
	} else {
		/* loop over all items in the scancache and update the offsets */
		g_sequence_foreach(btv->scancache.stackcaches,scancache_update_all_foreach,buffer);
		btv->scancache.stackcache_need_update_charoffset = -1;
	}
	DBG_MSG("done updating stackcaches offsets\n");
}
static void foundblock_unref(Tfoundblock *fblock, GtkTextBuffer *buffer) {
	DBG_FBLOCKREFCOUNT("unref fblock %p;",fblock);
	fblock->refcount--;
	DBG_FBLOCKREFCOUNT(" refcount is %d\n",fblock->refcount);
	if (fblock->refcount == 0) {
		/* remove marks */
		DBG_FBLOCKREFCOUNT("UNREF->cleanup foundblock %p\n",fblock);
		gtk_text_buffer_delete_mark(buffer,fblock->start1);
		gtk_text_buffer_delete_mark(buffer,fblock->end1);
		if (fblock->start2 && fblock->end2) {
			gtk_text_buffer_delete_mark(buffer,fblock->start2);
			gtk_text_buffer_delete_mark(buffer,fblock->end2);
		}
		g_slice_free(Tfoundblock,fblock);
#ifdef HL_PROFILING
		hl_profiling.fblock_refcount--;
#endif
	}
}
static void foundcontext_unref(Tfoundcontext *fcontext, GtkTextBuffer *buffer) {
	fcontext->refcount--;
	DBG_FCONTEXTREFCOUNT("unref: refcount for fcontext %p is %d\n",fcontext,fcontext->refcount);
	if (fcontext->refcount == 0) {
		/* remove marks */
		DBG_FCONTEXTREFCOUNT("unref: cleanup foundcontext %p\n",fcontext);
		gtk_text_buffer_delete_mark(buffer,fcontext->start);
		gtk_text_buffer_delete_mark(buffer,fcontext->end);
		g_slice_free(Tfoundcontext,fcontext);
#ifdef HL_PROFILING
		hl_profiling.fcontext_refcount--;
#endif

	}
}
static void foundcontext_foreach_unref_lcb(gpointer data,gpointer user_data) {
	if (data)
		foundcontext_unref(data,gtk_text_view_get_buffer(user_data));
}
static void foundblock_foreach_unref_lcb(gpointer data,gpointer user_data) {
	if (data)
		foundblock_unref(data,gtk_text_view_get_buffer(user_data));
}
void foundstack_free_lcb(gpointer data, gpointer btv) {
	Tfoundstack *fstack = data;
	/* unref all contexts and blocks */
	DBG_FBLOCKREFCOUNT("removing fstack %p, calling foundblock_foreach_unref_lcb\n",fstack);
	g_queue_foreach(fstack->blockstack,foundblock_foreach_unref_lcb,btv);
	g_queue_foreach(fstack->contextstack,foundcontext_foreach_unref_lcb,btv);
	if (fstack->pushedblock)
		foundblock_unref(fstack->pushedblock, GTK_TEXT_VIEW(btv)->buffer);
	if (fstack->poppedblock)
		foundblock_unref(fstack->poppedblock, GTK_TEXT_VIEW(btv)->buffer);
	
	g_slice_free(Tfoundstack,fstack);
#ifdef HL_PROFILING
	hl_profiling.fstack_refcount--;
#endif
}
static void foundcontext_foreach_ref_lcb(gpointer data,gpointer user_data) {
	if (data) {
		((Tfoundcontext *)data)->refcount++;
		DBG_FCONTEXTREFCOUNT("foreach ref: refcount for fcontext %p is %d\n",data,((Tfoundcontext *)data)->refcount);
	}
}
static void foundblock_foreach_ref_lcb(gpointer data,gpointer user_data) {
	if (data) {
		((Tfoundblock *)data)->refcount++;
		DBG_FBLOCKREFCOUNT("foreach ref fblock %p; refcount is %d\n",((Tfoundblock *)data),((Tfoundblock *)data)->refcount);
	}
}
static void add_to_scancache(BluefishTextView * btv,GtkTextBuffer *buffer,Tscanning *scanning, Tfoundblock *fblock, Tfoundcontext *fcontext) {
	Tfoundstack *fstack;

	fstack = g_slice_new0(Tfoundstack);
#ifdef HL_PROFILING
	hl_profiling.fstack_refcount++;
#endif
	fstack->contextstack = g_queue_copy(scanning->contextstack);
	fstack->blockstack = g_queue_copy(scanning->blockstack);
	DBG_FBLOCKREFCOUNT("creating new fstack %p, calling foundblock_foreach_ref_lcb\n",fstack);
	g_queue_foreach(fstack->blockstack,foundblock_foreach_ref_lcb,NULL);
	g_queue_foreach(fstack->contextstack,foundcontext_foreach_ref_lcb,NULL);

	if (fblock) {
		fblock->refcount++;
		DBG_FBLOCKREFCOUNT("adding fblock %p to pushed/popped, refcount=%d\n",fblock,fblock->refcount);
		if (fblock == g_queue_peek_head(fstack->blockstack)) {
			fstack->pushedblock = fblock;
		} else {
			fstack->poppedblock = fblock;
		}
	}
	if (fcontext) {
		if (fcontext == g_queue_peek_head(fstack->contextstack))
			fstack->pushedcontext = fcontext;
		else
			fstack->poppedcontext = fcontext;
	}
	foundstack_update_positions(buffer, fstack);
	DBG_SCANCACHE("add_to_scancache, put fstack %p in the cache at charoffset %d / line %d\n",fstack,fstack->charoffset,fstack->line);
	g_sequence_insert_sorted(btv->scancache.stackcaches,fstack,stackcache_compare_charoffset,NULL);
}

static void print_blockstack(BluefishTextView * btv, Tscanning *scanning) {
	GList *tmplist;
	Tfoundblock *fblock;
	g_print("blockstack:");
	for (tmplist=scanning->blockstack->tail;tmplist;tmplist=tmplist->prev) {
		fblock = tmplist->data;
		g_print(" %s",g_array_index(btv->bflang->st->matches, Tpattern, fblock->patternum).pattern);
	}
	g_print("\n");
}

static Tfoundblock *found_start_of_block(BluefishTextView * btv,GtkTextBuffer *buffer, Tmatch match, Tscanning *scanning) {
	if (scanning->blockstack->length > 100) {
		/* if a file has thousands of blockstarts this results in thousands of Tfoundblock structures, but 
		worse: also thousands of copies of the blockstack in the scancache --> 1000 * 0.5 * 1000 queue elements.
		to avoid this we return NULL here if the blockstack is > 100. If we return NULL here
		there will be no addition to the scancache either.  */
		DBG_BLOCKMATCH("blockstack length > 100 ** IGNORING BLOCK **\n");
		return NULL;
	} else {
		Tfoundblock *fblock;
		DBG_BLOCKMATCH("put block for pattern %d (%s) on blockstack\n",match.patternum,g_array_index(btv->bflang->st->matches,Tpattern,match.patternum).pattern);
#ifdef HL_PROFILING
		hl_profiling.numblockstart++;
#endif
		fblock = g_slice_new0(Tfoundblock);
		DBG_FBLOCKREFCOUNT("created new fblock with refcount 1 at %p\n",fblock);
#ifdef HL_PROFILING
		hl_profiling.fblock_refcount++;
#endif
		fblock->start1 = gtk_text_buffer_create_mark(buffer,NULL,&match.start,FALSE);
		fblock->end1 = gtk_text_buffer_create_mark(buffer,NULL,&match.end,TRUE);
		fblock->patternum = match.patternum;
		g_object_set_data(G_OBJECT(fblock->start1), "block", fblock);
		g_object_set_data(G_OBJECT(fblock->end1), "block", fblock);
		g_queue_push_head(scanning->blockstack,fblock);
		/*print_blockstack(btv,scanning);*/
		fblock->refcount++;
		return fblock;
	}	
}

static Tfoundblock *found_end_of_block(BluefishTextView * btv,GtkTextBuffer *buffer, Tmatch match, Tscanning *scanning, Tpattern *pat) {
	Tfoundblock *fblock=NULL;
	DBG_BLOCKMATCH("found end of block with blockstartpattern %d\n",pat->blockstartpattern);
#ifdef HL_PROFILING
	hl_profiling.numblockend++;
#endif
	do {
		if (fblock) {
			foundblock_unref(fblock, buffer);
		}
		fblock = g_queue_pop_head(scanning->blockstack);
		if (fblock) {
			DBG_BLOCKMATCH("popped block for pattern %d (%s) from blockstack\n",fblock->patternum, g_array_index(btv->bflang->st->matches,Tpattern,fblock->patternum).pattern);
		}
		/* if patternum == -1 this means we should end the last started block
		else we pop until we have the right startpattern */
	} while (fblock && fblock->patternum != pat->blockstartpattern && pat->blockstartpattern != -1);
	/*print_blockstack(btv,scanning);*/
	if (fblock) {
		GtkTextIter iter;
		DBG_BLOCKMATCH("found the matching start of the block\n");
		fblock->start2 = gtk_text_buffer_create_mark(buffer,NULL,&match.start,FALSE);
		fblock->end2 = gtk_text_buffer_create_mark(buffer,NULL,&match.end,TRUE);
		g_object_set_data(G_OBJECT(fblock->start2), "block", fblock);
		g_object_set_data(G_OBJECT(fblock->end2), "block", fblock);
		gtk_text_buffer_get_iter_at_mark(buffer,&iter,fblock->end1);
		if (pat->blocktag) {
			gtk_text_buffer_apply_tag(buffer,pat->blocktag, &iter, &match.start);
		}
		if ((gtk_text_iter_get_line(&iter)+1) < gtk_text_iter_get_line(&match.start)) {
			fblock->foldable = TRUE;
		}
		return fblock;
	} else {
		DBG_BLOCKMATCH("no matching start-of-block found\n");
	}
	return NULL;
}

static Tfoundcontext *found_context_change(BluefishTextView * btv,GtkTextBuffer *buffer, Tmatch match, Tscanning *scanning, Tpattern *pat) {
	Tfoundcontext *fcontext=NULL;
	/* check if we change up or down the stack */
	if (pat->nextcontext < 0) {
		gint num = -1 * pat->nextcontext;
#ifdef HL_PROFILING
		hl_profiling.numcontextend++;
#endif
		/* pop, but don't pop if there is nothing to pop (because of an error in the language file) */
		while (num > 0 && scanning->contextstack->head) {
			fcontext = g_queue_pop_head(scanning->contextstack);
			DBG_SCANNING("popped %p, stack len now %d\n",fcontext,g_queue_get_length(scanning->contextstack));
			DBG_SCANNING("found_context_change, popped context %d from the stack, stack len %d\n",fcontext->context,g_queue_get_length(scanning->contextstack));
			fcontext->end = gtk_text_buffer_create_mark(buffer,NULL,&match.start,FALSE);
			if (g_array_index(btv->bflang->st->contexts,Tcontext,fcontext->context).contexttag) {
				GtkTextIter iter;
				gtk_text_buffer_get_iter_at_mark(buffer,&iter,fcontext->start);
				gtk_text_buffer_apply_tag(buffer,g_array_index(btv->bflang->st->contexts,Tcontext,fcontext->context).contexttag, &iter, &match.start);
			}
			foundcontext_unref(fcontext, buffer);
			num--;
		}
		return fcontext;
	} else {
#ifdef HL_PROFILING
		hl_profiling.numcontextstart++;
#endif
		fcontext = g_slice_new0(Tfoundcontext);
#ifdef HL_PROFILING
		hl_profiling.fcontext_refcount++;
#endif
		fcontext->start = gtk_text_buffer_create_mark(buffer,NULL,&match.end,FALSE);
		fcontext->context = pat->nextcontext;
		g_queue_push_head(scanning->contextstack, fcontext);
		fcontext->refcount++;
		DBG_FCONTEXTREFCOUNT("refcount for fcontext %p is %d\n",fcontext,fcontext->refcount);
		DBG_SCANNING("found_context_change, pushed nextcontext %d onto the stack, stack len %d\n",pat->nextcontext,g_queue_get_length(scanning->contextstack));
		return fcontext;
	}
}

static int found_match(BluefishTextView * btv, Tmatch match, Tscanning *scanning)
{
	GtkTextBuffer *buffer;
	Tfoundblock *fblock=NULL;
	Tfoundcontext *fcontext=NULL;
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
	Tpattern pat = g_array_index(btv->bflang->st->matches,Tpattern, match.patternum);
	DBG_SCANNING("found_match for pattern %d %s at charoffset %d, starts_block=%d,ends_block=%d, nextcontext=%d (current=%d)\n",match.patternum,pat.pattern, gtk_text_iter_get_offset(&match.start),pat.starts_block,pat.ends_block,pat.nextcontext,scanning->context);
/*	DBG_MSG("pattern no. %d (%s) matches (%d:%d) --> nextcontext=%d\n", match.patternum, scantable.matches[match.patternum].message,
			gtk_text_iter_get_offset(&match.start), gtk_text_iter_get_offset(&match.end), scantable.matches[match.patternum].nextcontext);*/

	if (pat.selftag) {
		DBG_SCANNING("apply tag %p from %d to %d\n",pat.selftag,gtk_text_iter_get_offset(&match.start),gtk_text_iter_get_offset(&match.end));
		gtk_text_buffer_apply_tag(buffer,pat.selftag, &match.start, &match.end);
	}

	if (pat.starts_block) {
		fblock = found_start_of_block(btv, buffer, match, scanning);
	}
	if (pat.ends_block) {
		fblock = found_end_of_block(btv, buffer, match, scanning, &pat);
	}

	if (pat.nextcontext != 0 && pat.nextcontext != scanning->context) {
		fcontext = found_context_change(btv, buffer, match, scanning, &pat);
	}
	if (fblock || fcontext) {
		add_to_scancache(btv,buffer,scanning, fblock,fcontext);
		if (fblock) {
			foundblock_unref(fblock,buffer);
		}
	}
	if (pat.nextcontext < 0) {
		if (g_queue_get_length(scanning->contextstack)) {
			fcontext = g_queue_peek_head(scanning->contextstack);
			DBG_SCANNING("new context %d\n",fcontext->context);
			return fcontext->context;
		}
		DBG_SCANNING("return context 0\n");
		return 0;
	}
#ifdef HL_PROFILING
	if (g_queue_get_length(scanning->blockstack) > hl_profiling.longest_blockstack) 
		hl_profiling.longest_blockstack = g_queue_get_length(scanning->blockstack);
	if (g_queue_get_length(scanning->contextstack) > hl_profiling.longest_contextstack) 
		hl_profiling.longest_contextstack = g_queue_get_length(scanning->contextstack);

#endif
	if (pat.nextcontext == 0)
		return scanning->context;
	else
		return pat.nextcontext;
}

static gboolean bftextview2_find_region2scan(BluefishTextView * btv, GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end) {
	/* first find a region that needs scanning */
	gtk_text_buffer_get_start_iter(buffer, start);
	if (!gtk_text_iter_begins_tag(start,btv->needscanning) ) {
		if (!gtk_text_iter_forward_to_tag_toggle(start,btv->needscanning)) {
			/* nothing to scan */
			return FALSE;
		}
	}
	/* find the end of the region */
	*end = *start;
	gtk_text_iter_forward_char(end);
	if (!gtk_text_iter_ends_tag(end,btv->needscanning)) {
		if (!gtk_text_iter_forward_to_tag_toggle(end,btv->needscanning)) {
			DBG_MSG("BUG: we should never get here\n");
			return FALSE;
		}
	}
	/* now move start to the beginning of the line and end to the end of the line */
	gtk_text_iter_set_line_offset(start,0);
	gtk_text_iter_forward_to_line_end(end);
	gtk_text_iter_forward_char(end);
	return TRUE;
}

static void foundblock_foreach_clear_end_lcb(gpointer data,gpointer user_data) {
	Tfoundblock *fblock=data;
	if (fblock) {
		if (fblock->start2 && fblock->end2) {
			GtkTextIter iter;/* for debugging */
			gtk_text_buffer_get_iter_at_mark(GTK_TEXT_BUFFER(user_data), &iter, fblock->start1);
			DBG_SCANNING("clear end for block that starts at %d",gtk_text_iter_get_offset(&iter));
			gtk_text_buffer_get_iter_at_mark(GTK_TEXT_BUFFER(user_data), &iter, fblock->start2);
			DBG_SCANNING(" and the end starts at %d\n",gtk_text_iter_get_offset(&iter));
			gtk_text_buffer_delete_mark(GTK_TEXT_BUFFER(user_data),fblock->start2);
			gtk_text_buffer_delete_mark(GTK_TEXT_BUFFER(user_data),fblock->end2);
			fblock->start2 = NULL;
			fblock->end2 = NULL;
		}
		fblock->foldable = FALSE;
	}
}

static void reconstruct_stack(BluefishTextView * btv, GtkTextBuffer *buffer, GtkTextIter *position, Tscanning *scanning) {
	Tfoundstack *fstack=NULL;
	DBG_SCANNING("reconstruct_stack at position %d\n",gtk_text_iter_get_offset(position));
	fstack = get_stackcache_at_position(btv,position,NULL);
	if (fstack) {
		Tfoundcontext *fcontext;
		scanning->contextstack = g_queue_copy(fstack->contextstack);
		fcontext = g_queue_peek_head(scanning->contextstack);
		if (fcontext)
			scanning->context = fcontext->context;
		scanning->blockstack = g_queue_copy(fstack->blockstack);

		g_queue_foreach(scanning->blockstack,foundblock_foreach_ref_lcb,NULL);
		g_queue_foreach(scanning->contextstack,foundcontext_foreach_ref_lcb,NULL);
		DBG_SCANNING("stack from the cache, contextstack has len %d, blockstack has len %d, context=%d\n",g_queue_get_length(scanning->contextstack),g_queue_get_length(scanning->blockstack),scanning->context);

		g_queue_foreach(scanning->blockstack,foundblock_foreach_clear_end_lcb,buffer);
	} else {
		DBG_SCANNING("empty stack\n");
		scanning->contextstack =  g_queue_new();
		scanning->blockstack =  g_queue_new();
	}
}

/*
static void remove_old_matches_at_iter(BluefishTextView *btv, GtkTextBuffer *buffer, GtkTextIter *iter) {
	GSList *toggles, *tmplist;
	/ * remove any toggled tags * /
	toggles = tmplist = gtk_text_iter_get_toggled_tags(iter,FALSE);
	while (tmplist) {
		GtkTextIter tmpit;
		tmpit = *iter;
		gtk_text_iter_forward_to_tag_toggle(&tmpit,tmplist->data);
		DBG_MSG("%s:%d, removing tag %p from %d to %d\n",__FILE__,__LINE__,tmplist->data,gtk_text_iter_get_offset(iter),gtk_text_iter_get_offset(&tmpit));
		gtk_text_buffer_remove_tag(buffer,tmplist->data,iter,&tmpit);
		tmplist = g_slist_next(tmplist);
	}
	g_slist_free(toggles);
	/ * TODO see if there are any old blockstack or context changes * /

}*/

static void remove_old_scan_results(BluefishTextView *btv, GtkTextBuffer *buffer, GtkTextIter *fromhere) {
	GtkTextIter end;
	GSequenceIter *sit1,*sit2;
	Tfoundstack fakefstack;

	gtk_text_buffer_get_end_iter(buffer,&end);
	DBG_SCANCACHE("remove_old_scan_results: remove tags from charoffset %d to %d\n",gtk_text_iter_get_offset(fromhere),gtk_text_iter_get_offset(&end));
	gtk_text_buffer_remove_all_tags(buffer,fromhere,&end);

	fakefstack.charoffset = gtk_text_iter_get_offset(fromhere);
	sit1 = g_sequence_search(btv->scancache.stackcaches,&fakefstack,stackcache_compare_charoffset,NULL);
	if (sit1 && !g_sequence_iter_is_end(sit1)) {
		sit2 = g_sequence_get_end_iter(btv->scancache.stackcaches);
		DBG_SCANCACHE("sit1=%p, sit2=%p\n",sit1,sit2);
		DBG_SCANCACHE("remove_old_scan_results: remove stackcache entries %d to %d\n",g_sequence_iter_get_position(sit1),g_sequence_iter_get_position(sit2));
		g_sequence_foreach_range(sit1,sit2,foundstack_free_lcb,btv);
		g_sequence_remove_range(sit1,sit2);
	} else{
		DBG_SCANCACHE("no sit1, no cleanup ??\n");
	}
}

/* if visible_end is set (not NULL) we will scan only the visible area and nothing else.
this can be used to delay scanning everything until the editor is idle for several milliseconds */
gboolean bftextview2_run_scanner(BluefishTextView * btv, GtkTextIter *visible_end)
{
	GtkTextBuffer *buffer;
	GtkTextIter start, end, iter;
	GtkTextIter mstart;
	/*GArray *matchstack;*/
	Tscanning scanning;
	guint pos = 0, newpos;
	gboolean normal_run=TRUE, last_character_run=FALSE;
	gint loop=0;
#ifdef HL_PROFILING
	gdouble stage1;
	gdouble stage2;
	gdouble stage3;
	gdouble stage4,stage5;
	hl_profiling.longest_contextstack=0;
	hl_profiling.longest_blockstack=0;
	hl_profiling.numcontextstart=0;
	hl_profiling.numcontextend=0;
	hl_profiling.numblockstart=0;
	hl_profiling.numblockend=0;
	hl_profiling.numchars=0;
	hl_profiling.numloops=0;
#endif

		/* start timer */
	scanning.timer = g_timer_new();

	scanning.context = 1;
	DBG_MSG("bftextview2_run_scanner for btv %p..\n",btv);
	if (!btv->bflang->st) {
		DBG_MSG("no scantable, nothing to scan, returning...\n");
		return FALSE;
	}

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));

	if (!bftextview2_find_region2scan(btv, buffer, &start, &end)) {
		DBG_MSG("nothing to scan here.. update the offsets in the stackcache\n");
		
		DBG_SCANCACHE("scancache length %d\n", g_sequence_get_length(btv->scancache.stackcaches));
		scancache_update_all_positions(btv,buffer,NULL,NULL);
#ifdef HL_PROFILING
		stage1 = g_timer_elapsed(scanning.timer,NULL);
		g_print("update scancache offsets timing: %f us\n",stage1);
#endif
		/* after the offsets have been updated there is really nothing to do for
		the idle function so we return FALSE */
		return FALSE;
	}
	if (visible_end) {
		/* check such that we only scan up to vend */
		if (gtk_text_iter_compare(&start,visible_end)>0) {
			DBG_DELAYSCANNING("start of region that needs scanning is beyond visible_end, return TRUE\n");
			return TRUE;
		}
		if (gtk_text_iter_compare(&end,visible_end)>0) {
			DBG_DELAYSCANNING("end of region that needs scanning is beyond visible_end, reset end\n");
			end = *visible_end;
		}

	}

	DBG_SCANNING("scanning from %d to %d\n",gtk_text_iter_get_offset(&start),gtk_text_iter_get_offset(&end));
	if (btv->scancache.stackcache_need_update_charoffset != -1 && btv->scancache.stackcache_need_update_charoffset <= gtk_text_iter_get_offset(&end)) {
		gtk_text_buffer_get_iter_at_offset(buffer, &iter, btv->scancache.stackcache_need_update_charoffset);
		scancache_update_all_positions(btv, buffer, &iter, &end);
	}
#ifdef HL_PROFILING
	stage1 = g_timer_elapsed(scanning.timer,NULL);
#endif
	iter = mstart = start;
	if (gtk_text_iter_is_start(&start)) {
		scanning.contextstack = g_queue_new();
		scanning.blockstack = g_queue_new();
		/*siter = g_sequence_iter_first(btv->scancache.stackcaches);*/
	} else {
		/* reconstruct the context stack and the block stack */
		reconstruct_stack(btv, buffer, &iter, &scanning);
		pos = g_array_index(btv->bflang->st->contexts,Tcontext,scanning.context).startstate;
		DBG_SCANNING("reconstructed stacks, context=%d, startstate=%d\n",scanning.context,pos);
	}
#ifdef HL_PROFILING
	stage2= g_timer_elapsed(scanning.timer,NULL);
#endif
	/* TODO: when rescanning text that has been scanned before we need to remove
	invalid tags and blocks. right now we remove all, but most are likely
	still valid. 
	This function takes a lot of time!!!!!!!!!! */
	remove_old_scan_results(btv, buffer, &start);
	/* because we remove all to the end we have to rescan to the end (I know this
	is stupid, should become smarter in the future )*/
#ifdef HL_PROFILING
	stage3 = g_timer_elapsed(scanning.timer,NULL);
#endif
	if (!visible_end)
		gtk_text_iter_forward_to_end(&end);
	else
		end = *visible_end;

	do {
		gunichar uc;
		loop++;
#ifdef HL_PROFILING
		hl_profiling.numloops++;
#endif
		if (last_character_run) {
			uc = '\0';
		} else {
			uc = gtk_text_iter_get_char(&iter);
			if (uc > 128) {
				/* multibyte characters cannot be matched by the engine. character
				1 in ascii is "SOH (start of heading)". we need this to support a
				pattern like [^#]* .  */
				uc = 1;
			}
		}
		DBG_SCANNING("scanning %d %c in pos %d..",gtk_text_iter_get_offset(&iter),uc,pos);
		newpos = g_array_index(btv->bflang->st->table, Ttablerow, pos).row[uc];
		DBG_SCANNING(" got newpos %d\n",newpos);
		if (newpos == 0 || uc == '\0') {
			if (g_array_index(btv->bflang->st->table,Ttablerow, pos).match) {
				Tmatch match;
				match.patternum = g_array_index(btv->bflang->st->table,Ttablerow, pos).match;
				match.start = mstart;
				match.end = iter;
				DBG_SCANNING("we have a match from pos %d to %d\n", gtk_text_iter_get_offset(&match.start),gtk_text_iter_get_offset(&match.end));
				scanning.context = found_match(btv, match,&scanning);
				DBG_SCANNING("after match context=%d\n",scanning.context);
			}
			if (gtk_text_iter_equal(&mstart,&iter)) {
				gtk_text_iter_forward_char(&iter);
#ifdef HL_PROFILING
				hl_profiling.numchars++;
#endif
			}
			mstart = iter;
			newpos = g_array_index(btv->bflang->st->contexts,Tcontext,scanning.context).startstate;
		} else {
			gtk_text_iter_forward_char(&iter);
#ifdef HL_PROFILING
			hl_profiling.numchars++;
#endif
		}
		pos = newpos;
		normal_run = !gtk_text_iter_equal(&iter, &end);
		if (!normal_run)
			/* only if last_character_run is FALSE and normal_run is FALSE we set last_character run to TRUE */
			last_character_run = 1 - last_character_run;
	} while ((normal_run || last_character_run) && (loop%loops_per_timer!=0 || g_timer_elapsed(scanning.timer,NULL)<MAX_CONTINUOUS_SCANNING_INTERVAL));
	DBG_SCANNING("scanned up to position %d, (end=%d) which took %f microseconds\n",gtk_text_iter_get_offset(&iter),gtk_text_iter_get_offset(&end),g_timer_elapsed(scanning.timer,NULL));
	gtk_text_buffer_apply_tag(buffer,btv->needscanning,&iter,&end);

	/*g_array_free(matchstack,TRUE);*/
#ifdef HL_PROFILING
	stage4 = g_timer_elapsed(scanning.timer,NULL);
	g_print("timing for this %d ms scanning run: %d, %d, %d, %d; loops=%d,chars=%d,blocks %d/%d (%d) contexts %d/%d (%d) scancache %d refcs %d,%d,%d\n"
		,(gint)(1000*stage4)
		,(gint)(1000*stage1)
		,(gint)(1000*stage2-stage1)
		,(gint)(1000*stage3-stage2)
		,(gint)(1000*stage4-stage3)
		,hl_profiling.numloops,hl_profiling.numchars
		,hl_profiling.numblockstart,hl_profiling.numblockend,hl_profiling.longest_blockstack
		,hl_profiling.numcontextstart,hl_profiling.numcontextend,hl_profiling.longest_contextstack
		,g_sequence_get_length(btv->scancache.stackcaches)
		,hl_profiling.fblock_refcount,hl_profiling.fcontext_refcount,hl_profiling.fstack_refcount
		);
#endif
	/* tune the loops_per_timer, try to have 10 timer checks per loop, so we have around 10% deviation from the set interval */
	if (normal_run)
		loops_per_timer = MAX(loop/10,100);
	g_timer_destroy(scanning.timer);
	return TRUE; /* even if we finished scanning the next call should update the scancache */
}

static GQueue *get_contextstack_at_position(BluefishTextView * btv, GtkTextIter *position) {
	Tfoundstack *fstack;
	GQueue *retqueue = g_queue_new();
	fstack = get_stackcache_at_position(btv,position,NULL);
	if (fstack) {
		GList *tmplist;

		tmplist = fstack->contextstack->head;
		while (tmplist) {
			gint cont = ((Tfoundcontext *)tmplist->data)->context;
			g_queue_push_tail(retqueue, GINT_TO_POINTER(cont));
			tmplist = g_list_next(tmplist);
		}
	}
	return retqueue;
}

void scan_for_autocomp_prefix(BluefishTextView *btv,GtkTextIter *mstart,GtkTextIter *cursorpos,gint *contextnum) {
	GtkTextIter iter;
	guint16 pos,newpos;
	GQueue *contextstack;
	/* get the current context */
	iter = *mstart;

	contextstack = get_contextstack_at_position(btv, &iter);
	if (g_queue_get_length(contextstack)>0)
		*contextnum = GPOINTER_TO_INT(g_queue_peek_head(contextstack));
	else
		*contextnum = 1;
	pos = g_array_index(btv->bflang->st->contexts,Tcontext, *contextnum).startstate;
	DBG_AUTOCOMP("start scanning at offset %d with context %d and position %d\n",gtk_text_iter_get_offset(&iter),*contextnum,pos);
	while (!gtk_text_iter_equal(&iter, cursorpos)) {
		gunichar uc;
		uc = gtk_text_iter_get_char(&iter);
		if (uc > 128) {
			newpos = 0;
		} else {
			DBG_AUTOCOMP("scanning %c\n",uc);
			newpos = g_array_index(btv->bflang->st->table, Ttablerow, pos).row[uc];
		}
		if (newpos == 0 || uc == '\0') {
			DBG_AUTOCOMP("newpos=%d...\n",newpos);
			if (g_array_index(btv->bflang->st->table,Ttablerow, pos).match) {
				if (g_array_index(btv->bflang->st->matches,Tpattern, g_array_index(btv->bflang->st->table,Ttablerow, pos).match).nextcontext < 0) {
					gint num  = g_array_index(btv->bflang->st->matches,Tpattern, g_array_index(btv->bflang->st->table,Ttablerow, pos).match).nextcontext;
					while (num != 0) {
						g_queue_pop_head(contextstack);
						num++;
					}
					*contextnum = GPOINTER_TO_INT(g_queue_peek_head(contextstack));
				} else if (g_array_index(btv->bflang->st->matches,Tpattern, g_array_index(btv->bflang->st->table,Ttablerow, pos).match).nextcontext > 0) {
					DBG_AUTOCOMP("previous pos=%d had a match with a context change!\n",pos);
					*contextnum = g_array_index(btv->bflang->st->matches,Tpattern, g_array_index(btv->bflang->st->table,Ttablerow, pos).match).nextcontext;
					g_queue_push_head(contextstack, GINT_TO_POINTER(*contextnum));
				}
				DBG_AUTOCOMP("found match %d, new context is %d\n",g_array_index(btv->bflang->st->table,Ttablerow, pos).match,*contextnum);
			}
			if (gtk_text_iter_equal(mstart,&iter)) {
				gtk_text_iter_forward_char(&iter);
			}
			*mstart = iter;
			newpos = g_array_index(btv->bflang->st->contexts,Tcontext, *contextnum).startstate;
		} else {
			gtk_text_iter_forward_char(&iter);
		}
		pos = newpos;
	}
}

gboolean scan_for_tooltip(BluefishTextView *btv,GtkTextIter *mstart,GtkTextIter *position,gint *contextnum) {
	GtkTextIter iter,end;
	guint16 pos,newpos;
	gboolean retthismatch=FALSE;
	GQueue *contextstack;
	/* get the current context */
	iter = *mstart;

	contextstack = get_contextstack_at_position(btv, &iter);
	if (g_queue_get_length(contextstack)>0)
		*contextnum = GPOINTER_TO_INT(g_queue_peek_head(contextstack));
	else
		*contextnum = 1;
	pos = g_array_index(btv->bflang->st->contexts,Tcontext, *contextnum).startstate;

	gtk_text_buffer_get_end_iter(GTK_TEXT_VIEW(btv)->buffer,&end);
	DBG_MSG("start scanning at offset %d with context %d and position %d\n",gtk_text_iter_get_offset(&iter),*contextnum,pos);
	while (!gtk_text_iter_equal(&iter, &end)) {
		gunichar uc;
		uc = gtk_text_iter_get_char(&iter);
		if (uc > 128) {
			newpos = 0;
		} else {
			DBG_MSG("scanning %c\n",uc);
			newpos = g_array_index(btv->bflang->st->table, Ttablerow, pos).row[uc];
		}
		if (newpos == 0 || uc == '\0') {
			DBG_MSG("newpos=%d...\n",newpos);
			if (g_array_index(btv->bflang->st->table,Ttablerow, pos).match) {
				DBG_MSG("found match %d, retthismatch=%d\n",g_array_index(btv->bflang->st->table,Ttablerow, pos).match,retthismatch);
				if (retthismatch) {
					*position = iter;
					return TRUE;
				}
				if (g_array_index(btv->bflang->st->matches,Tpattern, g_array_index(btv->bflang->st->table,Ttablerow, pos).match).nextcontext < 0) {
					gint num  = g_array_index(btv->bflang->st->matches,Tpattern, g_array_index(btv->bflang->st->table,Ttablerow, pos).match).nextcontext;
					while (num != 0) {
						*contextnum = GPOINTER_TO_INT(g_queue_pop_head(contextstack));
						num++;
					}
				} else if (g_array_index(btv->bflang->st->matches,Tpattern, g_array_index(btv->bflang->st->table,Ttablerow, pos).match).nextcontext > 0) {
					DBG_MSG("previous pos=%d had a match with a context change!\n",pos);
					*contextnum = g_array_index(btv->bflang->st->matches,Tpattern, g_array_index(btv->bflang->st->table,Ttablerow, pos).match).nextcontext;
					g_queue_push_head(contextstack, GINT_TO_POINTER(*contextnum));
				}
			}
			if (gtk_text_iter_equal(mstart,&iter)) {
				gtk_text_iter_forward_char(&iter);
			}
			*mstart = iter;
			newpos = g_array_index(btv->bflang->st->contexts,Tcontext, *contextnum).startstate;
		} else {
			gtk_text_iter_forward_char(&iter);
		}
		pos = newpos;
		if (gtk_text_iter_equal(&iter, position)) {
			DBG_MSG("at position...\n");
			if (gtk_text_iter_equal(&iter, mstart))
				return FALSE;
			retthismatch = TRUE;
		}
	}
	return FALSE;
}


void cleanup_scanner(BluefishTextView *btv) {
	GtkTextIter begin,end;
	GtkTextBuffer *buffer;
	GSequenceIter *sit1,*sit2;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
	gtk_text_buffer_get_bounds(buffer,&begin,&end);
	gtk_text_buffer_remove_all_tags(buffer,&begin,&end);

	sit1 = g_sequence_get_begin_iter(btv->scancache.stackcaches);
	if (sit1 && !g_sequence_iter_is_end(sit1)) {
		sit2 = g_sequence_get_end_iter(btv->scancache.stackcaches);
		g_sequence_foreach_range(sit1,sit2,foundstack_free_lcb,btv);
		g_sequence_remove_range(sit1,sit2);
	} else{
		DBG_SCANNING("cleanup_scanner, no sit1, no cleanup ??\n");
	}
}
