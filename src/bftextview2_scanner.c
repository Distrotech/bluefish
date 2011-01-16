/* Bluefish HTML Editor
 * bftextview2_scanner.c
 *
 * Copyright (C) 2008,2009,2010 Olivier Sessink
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

#define HL_PROFILING
#define DUMP_SCANCACHE

/*#define VALGRIND_PROFILING*/

#ifdef VALGRIND_PROFILING
#include <valgrind/callgrind.h>
#endif

#ifdef HL_PROFILING
#include <unistd.h>
#endif
/* for the design docs see bftextview2.h */
#include "bluefish.h"
#include "bftextview2_scanner.h"
#include "bftextview2_identifier.h"
/* use 
G_SLICE=always-malloc valgrind --tool=memcheck --leak-check=full --num-callers=32 --freelist-vol=100000000 src/bluefish-unstable
to memory-debug this code
*/
#ifdef VALGRIND_PROFILING
#define MAX_CONTINUOUS_SCANNING_INTERVAL 1.0 /* float in seconds */
#else
#define MAX_CONTINUOUS_SCANNING_INTERVAL 0.1 /* float in seconds */
#endif

typedef struct {
	GtkTextIter start;
	GtkTextIter end;
	guint16 patternum;
} Tmatch;

typedef struct {
	Tfoundcontext *curfcontext;
	Tfoundblock *curfblock;
	/*Tfound *curfound;*/ /* items from the cache */
	Tfound *nextfound; /* items from the cache */
	GSequenceIter *siter; /* an iterator to get the next item from the cache */
	GTimer *timer;
	GtkTextIter start; /* start of area to scan */
	GtkTextIter end; /* end of area to scan */
	gint16 context;
	guint8 identmode;
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
	gint found_refcount;
	guint total_runs;
	guint total_chars;
	guint total_time_ms;
} Thl_profiling;

Thl_profiling hl_profiling = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
#endif

guint loops_per_timer=1000; /* a tunable to avoid asking time too often. this is auto-tuned. */ 


#ifdef DUMP_SCANCACHE
void dump_scancache(BluefishTextView *btv) {
	GSequenceIter *siter = g_sequence_get_begin_iter(btv->scancache.foundcaches);
	g_print("\nDUMP SCANCACHE\nwith length %d, document length=%d\n\n",g_sequence_get_length(btv->scancache.foundcaches),gtk_text_buffer_get_char_count(btv->buffer));
	while (siter && !g_sequence_iter_is_end(siter)) {
		Tfound *found = g_sequence_get(siter);
		if (!found)
			break;
		g_print("%3d: %p, fblock %p, fcontext %p\n",found->charoffset_o,found,found->fblock,found->fcontext);
		if (found->numcontextchange != 0) {
			g_print("\tnumcontextchange=%d", found->numcontextchange);
			if (found->fcontext) {
				g_print(",context %d, highlight %s, parent=%p, start %d, end %d"
						,found->fcontext->context
						,g_array_index(btv->bflang->st->contexts,Tcontext,found->fcontext->context).contexthighlight
						,found->fcontext->parentfcontext
						,found->fcontext->start_o
						,found->fcontext->end_o);
			}
			g_print("\n");
		}
		if (found->numblockchange != 0) {
			g_print("\tnumblockchange=%d",found->numblockchange);
			if (found->fblock) {
				g_print(", pattern %d %s, parent=%p, start1 %d, end1 %d, start2 %d, end2 %d"
						,found->fblock->patternum
						,g_array_index(btv->bflang->st->matches,Tpattern, found->fblock->patternum).pattern
						,found->fblock->parentfblock
						,found->fblock->start1_o
						,found->fblock->end1_o
						,found->fblock->start2_o
						,found->fblock->end2_o);
			}
			g_print("\n");
		}
		siter = g_sequence_iter_next(siter);
	}
	g_print("END OF DUMP\n\n");
}
#endif /* DUMP_SCANCACHE */

Tfound *get_foundcache_next(BluefishTextView * btv, GSequenceIter ** siter) {
	DBG_MSG("get_foundcache_next, *siter=%p\n",*siter);
	*siter = g_sequence_iter_next(*siter);
	if (*siter && !g_sequence_iter_is_end(*siter)) {
		return g_sequence_get(*siter);
	}
	return NULL;
}

Tfound *get_foundcache_first(BluefishTextView * btv, GSequenceIter ** retsiter) {
	*retsiter = g_sequence_get_begin_iter(btv->scancache.foundcaches);
	if (*retsiter && !g_sequence_iter_is_end(*retsiter)) {
		return g_sequence_get(*retsiter);
	}
	g_print("get_foundcache_first, return NULL\n");
	return NULL;
}

static gint foundcache_compare_charoffset_o(gconstpointer a,gconstpointer b,gpointer user_data) {
	return ((Tfound *)a)->charoffset_o - ((Tfound *)b)->charoffset_o;
}

Tfound *get_foundcache_at_offset(BluefishTextView * btv, guint offset, GSequenceIter ** retsiter) {
	GSequenceIter* siter;
	Tfound fakefound;
	Tfound *found=NULL;
	fakefound.charoffset_o = offset;
	siter = g_sequence_search(btv->scancache.foundcaches,&fakefound,foundcache_compare_charoffset_o,NULL);
	if (!g_sequence_iter_is_begin(siter)) {
		/* now get the previous position, and get the stack at that position */
		DBG_SCANCACHE("search for offset %d returned iter-position %d (cache length %d)\n",offset,g_sequence_iter_get_position(siter),g_sequence_get_length(btv->scancache.foundcaches));
		siter = g_sequence_iter_prev(siter);
		if (siter && !g_sequence_iter_is_end(siter)) {
			found = g_sequence_get(siter);
			if (retsiter)
				*retsiter = siter;
			DBG_SCANCACHE("found nearest stack %p with charoffset_o %d\n",found,found->charoffset_o);
		} else {
			DBG_SCANCACHE("no siter no cache\n");
		}
	} else if (!g_sequence_iter_is_end(siter)){
		DBG_SCANCACHE("got begin siter\n");
		found = g_sequence_get(siter); /* get the first found */
		if (retsiter)
			*retsiter = siter;
	}
	return found;
}

void found_free_lcb(gpointer data, gpointer btv);

static guint remove_cache_entry(BluefishTextView * btv, Tfound **found, GSequenceIter **siter) {
	Tfound *tmpfound1 = *found;
	GSequenceIter *tmpsiter1 = *siter;
	guint invalidoffset = tmpfound1->charoffset_o;
	gint blockstackcount=0, contextstackcount=0;
	
	*found = get_foundcache_next(btv,siter);

	/* if this entry pops blocks or contexts, mark the ends of those as undefined */
	if (tmpfound1->numblockchange < 0) {
		Tfoundblock *tmpfblock = tmpfound1->fblock;
		DBG_SCANCACHE("remove_cache_entry, mark end of %d fblock's as undefined, fblock=%p\n",tmpfound1->numblockchange,tmpfound1->fblock);
		blockstackcount = tmpfound1->numblockchange;
		while (tmpfblock && blockstackcount < 0) {
			DBG_SCANCACHE("remove_cache_entry, mark end of fblock %p as undefined\n",tmpfblock);
			tmpfblock->start2_o = BF2_OFFSET_UNDEFINED;
			tmpfblock->end2_o = BF2_OFFSET_UNDEFINED;
			tmpfblock = tmpfblock->parentfblock;
			blockstackcount++;
		}
	}
	if (tmpfound1->numcontextchange < 0) {
		Tfoundcontext *tmpfcontext = tmpfound1->fcontext;
		DBG_SCANCACHE("remove_cache_entry, mark end of %d fcontext's as undefined\n",tmpfound1->numcontextchange);
		contextstackcount = tmpfound1->numcontextchange;
		while (tmpfcontext && contextstackcount < 0) {
			DBG_SCANCACHE("remove_cache_entry, mark end of fcontext %p as undefined\n",tmpfcontext);
			tmpfcontext->end_o = BF2_OFFSET_UNDEFINED;
			tmpfcontext = tmpfcontext->parentfcontext;
			contextstackcount++;
		}
	}
	
	blockstackcount= MAX(0, tmpfound1->numblockchange);
	contextstackcount= MAX(0, tmpfound1->numcontextchange);
	/* remove the children */
	DBG_SCANCACHE("remove_cache_entry, remove all children of found %p (numblockchange=%d,numcontextchange=%d) with offset %d, next found in cache=%p\n",tmpfound1,tmpfound1->numblockchange,tmpfound1->numcontextchange,tmpfound1->charoffset_o, *found);
	while (*found && (blockstackcount > 0 || contextstackcount > 0)) {
		Tfound *tmpfound2 = *found;
		GSequenceIter *tmpsiter2 = *siter;
		*found = get_foundcache_next(btv,siter);
		blockstackcount += tmpfound2->numblockchange;
		contextstackcount += tmpfound2->numcontextchange;

		DBG_SCANCACHE("in loop: remove Tfound %p with offset %d from the cache and free, contextstackcount=%d, blockstackcount=%d\n",tmpfound2,tmpfound2->charoffset_o,contextstackcount,blockstackcount);
		invalidoffset = tmpfound2->charoffset_o;
		g_sequence_remove(tmpsiter2);
		found_free_lcb(tmpfound2, btv);
	}
	
	DBG_SCANCACHE("remove_cache_entry, finally remove found %p itself with offset %d\n",tmpfound1,tmpfound1->charoffset_o);
	
	g_sequence_remove(tmpsiter1);
	found_free_lcb(tmpfound1, btv);
	return invalidoffset;
}


/** 
 * foundcache_update_offsets
 * 
 * startpos is the lowest position 
 *  so on insert it is the point _after_ which the insert will be (and offset is a positive number) 
 *  on delete it is the point _after_ which the delete area starts (and offset is a negative number)
*/
void foundcache_update_offsets(BluefishTextView * btv, guint startpos, gint offset) {
	Tfound *found;
	GSequenceIter *siter;
	if (offset==0)
		return;
	DBG_SCANCACHE("foundcache_update_offsets, update with offset %d starting at startpos %d, cache length=%d\n",offset,startpos,g_sequence_get_length(btv->scancache.foundcaches) );
	
	found = get_foundcache_at_offset(btv, startpos, &siter);
	if (found)
		DBG_SCANCACHE("foundcache_update_offsets, got found %p with offset %d\n",found, found->charoffset_o);
	else
		DBG_SCANCACHE("foundcache_update_offsets, got found %p\n",found);

	/* first update all block ends and context ends that are on the stack */
	if (found) {
		Tfoundcontext *tmpfcontext;
		Tfoundblock *tmpfblock;
		DBG_SCANCACHE("foundcache_update_offsets, handle first found %p with offset %d, complete stack fcontext %p fblock %p\n",found, found->charoffset_o, found->fcontext, found->fblock);
		/* for the first found, we have to update the end-offsets for all contexts/blocks on the stack */
		tmpfcontext = found->fcontext;
		while(tmpfcontext) {
			DBG_SCANCACHE("foundcache_update_offsets, fcontext on stack=%p, start_o=%d end_o=%d\n",tmpfcontext, tmpfcontext->start_o, tmpfcontext->end_o);
			if (tmpfcontext->end_o > startpos && tmpfcontext->end_o != BF2_OFFSET_UNDEFINED) {
				DBG_SCANCACHE("foundcache_update_offsets, update fcontext %p end from %d to %d\n",tmpfcontext, tmpfcontext->end_o, tmpfcontext->end_o+offset);
				tmpfcontext->end_o += offset;
			}
			tmpfcontext = (Tfoundcontext *)tmpfcontext->parentfcontext;
		}

		tmpfblock = found->fblock;
		while(tmpfblock) {
			DBG_SCANCACHE("foundcache_update_offsets, fblock on stack=%p, start1_o=%d end2_o=%d\n",tmpfblock, tmpfblock->start1_o, tmpfblock->end2_o);
			if (tmpfblock->start2_o > startpos && tmpfblock->start2_o != BF2_OFFSET_UNDEFINED) {
				DBG_SCANCACHE("foundcache_update_offsets, update fblock %p with start1_o=%d and start2_o=%d to start2_o=%d\n",tmpfblock, tmpfblock->start1_o,tmpfblock->start2_o, tmpfblock->start2_o+offset);
				tmpfblock->start2_o += offset;
			}
			if (tmpfblock->end2_o > startpos && tmpfblock->end2_o != BF2_OFFSET_UNDEFINED)
				tmpfblock->end2_o += offset;
			
			tmpfblock = (Tfoundblock *)tmpfblock->parentfblock;
		}
	}

	if (offset < 0) {
		while (found && found->charoffset_o <= startpos-offset) {
			if (found->charoffset_o > startpos) {
				remove_cache_entry(btv, &found, &siter);
			} else {
				found = get_foundcache_next(btv, &siter);
			}
		}		
	} else {
		while(found && found->charoffset_o <= startpos) {
			DBG_SCANCACHE("foundcache_update_offsets, now search for found > startpos, try found %p with charoffset %d\n",found,found->charoffset_o);
			found = get_foundcache_next(btv, &siter);
		}
	}
	while (found) {
		DBG_SCANCACHE("foundcache_update_offsets, about to update found %p with charoffset %d to %d, fcontext %p and fblock %p\n",found,found->charoffset_o,found->charoffset_o+offset, found->fcontext, found->fblock);
		/* for all further founds, we only handle the pushedblock and pushedcontext */
		if (IS_FOUNDMODE_CONTEXTPUSH(found)) {
			DBG_SCANCACHE("foundcache_update_offsets, contextpush %p update from %d:%d to %d:%d\n"
						,found->fcontext
						,found->fcontext->start_o,found->fcontext->end_o
						,found->fcontext->start_o+offset,found->fcontext->end_o+offset);
			if (found->fcontext->start_o != BF2_OFFSET_UNDEFINED)
				found->fcontext->start_o += offset;
			if (found->fcontext->end_o != BF2_OFFSET_UNDEFINED)
				found->fcontext->end_o += offset;
		}
		if (IS_FOUNDMODE_BLOCKPUSH(found)) {
			DBG_SCANCACHE("foundcache_update_offsets, blockpush %p update from %d:%d to %d:%d\n"
						,found->fblock
						,found->fblock->start1_o,found->fblock->end2_o
						,found->fblock->start1_o+offset,found->fblock->end2_o+offset);
			if (found->fblock->start1_o != BF2_OFFSET_UNDEFINED)
				found->fblock->start1_o += offset;
			if (found->fblock->end1_o != BF2_OFFSET_UNDEFINED)
				found->fblock->end1_o += offset;
			if (found->fblock->start2_o != BF2_OFFSET_UNDEFINED)
				found->fblock->start2_o += offset;
			if (found->fblock->end2_o != BF2_OFFSET_UNDEFINED)
				found->fblock->end2_o += offset;
		}
		/*g_print("startpos=%d, offset=%d, found=%p, update charoffset_o from %d to %d\n",startpos,offset, found,found->charoffset_o,found->charoffset_o+offset);*/
		found->charoffset_o += offset;
		found = get_foundcache_next(btv, &siter);
	}
#ifdef DUMP_SCANCACHE
	dump_scancache(btv);
#endif
}

void found_free_lcb(gpointer data, gpointer btv) {
	Tfound *found = data;
	DBG_SCANCACHE("found_free_lcb, destroy %p\n",found);
	if (IS_FOUNDMODE_BLOCKPUSH(found)) {
#ifdef HL_PROFILING
		hl_profiling.fblock_refcount--;
#endif
		g_slice_free(Tfoundblock, found->fblock);
	}
	if (IS_FOUNDMODE_CONTEXTPUSH(found)) {
#ifdef HL_PROFILING
		hl_profiling.fcontext_refcount--;
#endif
		g_slice_free(Tfoundcontext, found->fcontext);
	}
#ifdef HL_PROFILING
	hl_profiling.found_refcount--;
#endif
	g_slice_free(Tfound,found);
}

static void remove_all_highlighting_in_area(BluefishTextView *btv, GtkTextIter *start, GtkTextIter *end) {
	GList *tmplist = g_list_first(btv->bflang->tags);
	while (tmplist) {
		gtk_text_buffer_remove_tag(btv->buffer, (GtkTextTag *)tmplist->data, start, end);
		tmplist = g_list_next(tmplist);
	}
}

/*
static void print_blockstack(BluefishTextView * btv, Tscanning *scanning) {
	GList *tmplist;
	Tfoundblock *fblock;
	g_print("blockstack:");
	for (tmplist=scanning->blockstack->tail;tmplist;tmplist=tmplist->prev) {
		fblock = tmplist->data;
		g_print(" %s",g_array_index(btv->bflang->st->matches, Tpattern, fblock->patternum).pattern);
	}
	g_print("\n");
}*/

static Tfoundblock *pop_blocks(gint numchange, Tfoundblock *curblock) {
	gint num=numchange;
	Tfoundblock *fblock = curblock;
	while (num < 0 && fblock) {
		fblock = (Tfoundblock *)fblock->parentfblock;
		num++;
	}
	return fblock;
}

static inline Tfoundblock *found_start_of_block(BluefishTextView * btv,Tmatch *match, Tscanning *scanning) {
	Tfoundblock *fblock;
	DBG_BLOCKMATCH("found_start_of_block, put block for pattern %d (%s) on blockstack\n",match->patternum,g_array_index(btv->bflang->st->matches,Tpattern,match->patternum).pattern);
#ifdef HL_PROFILING
	hl_profiling.numblockstart++;
	hl_profiling.fblock_refcount++;
#endif
	fblock = g_slice_new0(Tfoundblock);
	fblock->start1_o = gtk_text_iter_get_offset(&match->start);
	fblock->end1_o = gtk_text_iter_get_offset(&match->end);
	/*g_print("found blockstart with start_1 %d end1 %d\n",fblock->start1_o,fblock->end1_o);*/
	fblock->start2_o = BF2_OFFSET_UNDEFINED;
	fblock->end2_o = BF2_OFFSET_UNDEFINED;
	fblock->patternum = match->patternum;
	fblock->parentfblock = scanning->curfblock;
	DBG_BLOCKMATCH("found_start_of_block, new block at %p with parent %p\n",fblock, fblock->parentfblock);
	scanning->curfblock = fblock;
	return fblock;
}

static gboolean enlarge_scanning_region(BluefishTextView * btv, Tscanning *scanning, guint offset);

static inline Tfoundblock *found_end_of_block(BluefishTextView * btv,Tmatch *match, Tscanning *scanning, Tpattern *pat, gint *numblockchange) {
	Tfoundblock *retfblock, *fblock=scanning->curfblock;
	GtkTextIter iter;
	DBG_BLOCKMATCH("found_end_of_block(), blockstartpattern %d, curfblock=%p\n",pat->blockstartpattern, scanning->curfblock);

	if (G_UNLIKELY(!scanning->curfblock))
		return NULL;
	
	retfblock = scanning->curfblock;
#ifdef HL_PROFILING
	hl_profiling.numblockend++;
#endif
	while (fblock && fblock->patternum != pat->blockstartpattern && pat->blockstartpattern != -1) {
		DBG_BLOCKMATCH("pop fblock %p with patternum %d and parent %p\n", fblock, fblock->patternum, fblock->parentfblock);
		fblock = (Tfoundblock *)fblock->parentfblock;
		(*numblockchange) --;
	}

	if (G_UNLIKELY(!fblock)) {
		*numblockchange = 0;
		DBG_BLOCKMATCH("no matching start-of-block found\n");
		return NULL;
	}

	DBG_BLOCKMATCH("found the matching start of the block fblock %p with patternum %d and parent %p\n",fblock, fblock->patternum, fblock->parentfblock);
	if (G_UNLIKELY(fblock->start2_o != BF2_OFFSET_UNDEFINED && fblock->start2_o)) {
		Tfound *ifound;
		GSequenceIter *isiter=NULL, *cursiter;
		DBG_SCANCACHE("found_end_of_block, block has an end already?!? invalidate and enlarge region to previous end at end2_o %d\n",fblock->end2_o);
		/* this block was previously larger, so now we have to invalidate the previous
		end of block in the cache */
		enlarge_scanning_region(btv, scanning, fblock->end2_o);
		ifound = get_foundcache_at_offset(btv, fblock->end2_o, &isiter);
		if (ifound && ifound->charoffset_o == fblock->end2_o && isiter) {
			DBG_SCANCACHE("found_end_of_block, invalidate ifound=%p at offset %d\n",ifound,ifound->charoffset_o);
			cursiter = scanning->siter;
			if (scanning->siter) {
				scanning->nextfound = get_foundcache_next(btv,&isiter);
			}
			g_sequence_foreach_range(cursiter,isiter,found_free_lcb,btv);
			g_sequence_remove_range(cursiter,isiter);
		}
	}
	fblock->start2_o = gtk_text_iter_get_offset(&match->start);
	fblock->end2_o = gtk_text_iter_get_offset(&match->end);
	
	gtk_text_buffer_get_iter_at_offset(btv->buffer,&iter,fblock->end1_o);
	if (G_UNLIKELY(pat->blocktag)) {
		gtk_text_buffer_apply_tag(btv->buffer,pat->blocktag, &iter, &match->start);
	}
	if ((gtk_text_iter_get_line(&iter)+1) < gtk_text_iter_get_line(&match->start)) {
		fblock->foldable = TRUE;
	}

	scanning->curfblock = fblock->parentfblock;
	(*numblockchange) --;
	return retfblock;
}

static Tfoundcontext *pop_contexts(gint numchange, Tfoundcontext *curcontext) {
	gint num=numchange;
	Tfoundcontext *fcontext = curcontext;
	while (num < 0 && fcontext) {
		fcontext = (Tfoundcontext *)fcontext->parentfcontext;
		num++;
	}
	return fcontext;
}

static Tfoundcontext *pop_and_apply_contexts(BluefishTextView * btv, gint numchange, Tfoundcontext *curcontext, GtkTextIter *matchstart) {
	gint num=numchange;
	guint offset = gtk_text_iter_get_offset(matchstart);
	Tfoundcontext *fcontext = curcontext;
	while (num < 0 && fcontext) { /* pop, but don't pop if there is nothing to pop (because of an error in the language file) */
		DBG_SCANNING("pop_and_apply_contexts, end context %d at pos %d, has tag %p and parent %p\n",fcontext->context, gtk_text_iter_get_offset(matchstart), g_array_index(btv->bflang->st->contexts,Tcontext,fcontext->context).contexttag, fcontext->parentfcontext);
		fcontext->end_o = offset;
		if (g_array_index(btv->bflang->st->contexts,Tcontext,fcontext->context).contexttag) {
			GtkTextIter iter;
			gtk_text_buffer_get_iter_at_offset(btv->buffer,&iter,fcontext->start_o);
			gtk_text_buffer_apply_tag(btv->buffer,g_array_index(btv->bflang->st->contexts,Tcontext,fcontext->context).contexttag, &iter, matchstart);
		}
		fcontext = (Tfoundcontext *)fcontext->parentfcontext;
		num++;
	}
	return fcontext;
}

static inline Tfoundcontext *found_context_change(BluefishTextView * btv,Tmatch *match, Tscanning *scanning, Tpattern *pat) {
	/* check if we change up or down the stack */
	if (pat->nextcontext < 0) {
		Tfoundcontext *retcontext = scanning->curfcontext;
#ifdef HL_PROFILING
		hl_profiling.numcontextend++;
#endif
		DBG_SCANNING("found_context_change, should pop %d contexts, curfcontext=%p\n",(-1*pat->nextcontext),scanning->curfcontext);
		scanning->curfcontext = pop_and_apply_contexts(btv, pat->nextcontext, scanning->curfcontext, &match->start);
		scanning->context = scanning->curfcontext ? scanning->curfcontext->context : 1;
		return retcontext;
	} else {
		Tfoundcontext *fcontext;
#ifdef HL_PROFILING
		hl_profiling.numcontextstart++;
		hl_profiling.fcontext_refcount++;
#endif
		fcontext = g_slice_new0(Tfoundcontext);
		fcontext->start_o = gtk_text_iter_get_offset(&match->end);
		fcontext->end_o = BF2_OFFSET_UNDEFINED;
		fcontext->parentfcontext = scanning->curfcontext;
		DBG_SCANNING("found_context_change, new fcontext %p with context %d onto the stack, parent=%p\n",fcontext, pat->nextcontext, fcontext->parentfcontext);
		scanning->curfcontext = fcontext;
		scanning->context = fcontext->context = pat->nextcontext;
		return fcontext;
	}
}

static gboolean nextcache_valid(Tscanning *scanning) {
	if (!scanning->nextfound)
		return FALSE;
	if (scanning->nextfound->numblockchange <= 0 && scanning->nextfound->fblock != scanning->curfblock) {
		DBG_SCANCACHE("nextcache_valid, next found %p pops fblock=%p, current fblock=%p, return FALSE\n",scanning->nextfound,scanning->nextfound->fblock,scanning->curfblock);
		return FALSE;
	}
	if (scanning->nextfound->numcontextchange <= 0 && scanning->nextfound->fcontext != scanning->curfcontext) {
		DBG_SCANCACHE("nextcache_valid, next found %p pops fcontext=%p, current fcontext=%p, return FALSE\n",scanning->nextfound,scanning->nextfound->fcontext,scanning->curfcontext);
		return FALSE;
	}
	if (scanning->nextfound->numcontextchange > 0 && (
			!scanning->nextfound->fcontext
			|| scanning->nextfound->fcontext->parentfcontext != scanning->curfcontext)) {
		DBG_SCANCACHE("nextcache_valid, next found %p doesn't push context on top of current fcontext %p, return FALSE\n",scanning->nextfound,scanning->curfcontext);
		return FALSE;
	}
	if (scanning->nextfound->numblockchange > 0&& (
			!scanning->nextfound->fblock
			|| scanning->nextfound->fblock->parentfblock != scanning->curfblock)) {
		DBG_SCANCACHE("nextcache_valid, next found %p doesn't push block on top of current fblock %p, return FALSE\n",scanning->nextfound,scanning->curfblock);
		return FALSE;
	}
	DBG_SCANCACHE("nextcache_valid, next found %p with offset %d seems valid\n",scanning->nextfound, scanning->nextfound->charoffset_o);
	return TRUE;
}


static inline gboolean cached_found_is_valid(BluefishTextView * btv, Tmatch *match, Tscanning *scanning) {
	Tpattern pat = g_array_index(btv->bflang->st->matches,Tpattern, match->patternum);

	if (!scanning->nextfound)
		return FALSE;
	
	DBG_SCANCACHE("cached_found_is_valid, testing %p\n",scanning->nextfound);
	
	if (!nextcache_valid(scanning))
		return FALSE;
	
	g_print("with numcontextchange=%d, numblockchange=%d\n"
			,scanning->nextfound->numcontextchange
			,scanning->nextfound->numblockchange);
	if (IS_FOUNDMODE_BLOCKPUSH(scanning->nextfound) && (
				!pat.starts_block
				|| scanning->nextfound->fblock->patternum != match->patternum
				)) {
		DBG_SCANCACHE("cached_found_is_valid, cached entry %p does not push the same block\n",scanning->nextfound);
		DBG_SCANCACHE("pat.startsblock=%d, nextfound->fblock->patternum=%d,match->patternum=%d,nextfound->fblock->parentfblock=%p,scanning->curfblock=%p\n"
					,pat.starts_block
					,scanning->nextfound->fblock->patternum, match->patternum
					,scanning->nextfound->fblock->parentfblock, scanning->curfblock);
		return FALSE;
	}
	if (IS_FOUNDMODE_BLOCKPOP(scanning->nextfound) && !pat.ends_block) {
		/* TODO: add more checks for popped blocks */
		DBG_SCANCACHE("cached_found_is_valid, cached entry %p does not pop a block\n",scanning->nextfound);
		return FALSE;
	}
	if (IS_FOUNDMODE_CONTEXTPUSH(scanning->nextfound) && (
				pat.nextcontext <= 0 
				/*|| pat.nextcontext != scanning->context*/
				|| scanning->nextfound->fcontext->context != pat.nextcontext
				)) {
		DBG_SCANCACHE("cached_found_is_valid, cached entry %p does not push the same context\n",scanning->nextfound);
		DBG_SCANCACHE("cached pat.nextcontext=%d, fcontext->context=%d, current context=%d\n"
					,pat.nextcontext
					,scanning->nextfound->fcontext->context
					,scanning->context
					);
		return FALSE;
	}
	if ((scanning->nextfound->numcontextchange < 0) && ( 
				pat.nextcontext != scanning->nextfound->numcontextchange
				)) {
		/* TODO: use more checks for popped contexts */
		DBG_SCANCACHE("cached_found_is_valid, cached entry %p does not pop the same context\n",scanning->nextfound);
		DBG_SCANCACHE("cached numcontextchange=%d, pat.nextcontext=%d\n"
					,scanning->nextfound->numcontextchange
					,pat.nextcontext
					);
		return FALSE;
	}
	return TRUE;
}

/* called from found_match() if an entry is in the cache, but the scanner continued in the text beyond
this offset (and thus this is outdated), or this entry is invalid for some reason */
static guint remove_invalid_cache(BluefishTextView * btv, guint match_end_o, Tscanning *scanning) {
	guint invalidoffset;
	
	DBG_SCANNING("remove_invalid_cache, cache item %p is NO LONGER valid\n",scanning->nextfound);
	if (scanning->nextfound->numblockchange < 0) {
		gint i=scanning->nextfound->numblockchange;
		Tfoundblock *tmpfblock = scanning->nextfound->fblock;
		while (i<0 && tmpfblock) {
			if (tmpfblock->end2_o >= match_end_o) {
				DBG_SCANNING("setting end of fblock %p as undefined\n",tmpfblock);
				/* only set the end of this fblock to invalid if it originally pointed to my 
				offset, because it is possibly already valid again and set to a smaller offset by a new scanning run */
				tmpfblock->start2_o = BF2_OFFSET_UNDEFINED;
				tmpfblock->end2_o = BF2_OFFSET_UNDEFINED;
			}
			tmpfblock = tmpfblock->parentfblock;
			i++;
		}
	}
	if (scanning->nextfound->numcontextchange < 0) {
		gint i=scanning->nextfound->numcontextchange;
		Tfoundcontext *tmpfcontext = scanning->nextfound->fcontext;
		while (i<0 && tmpfcontext) {
			/* TODO: what if the scanner ended this block already at an earlier offset?? 
			how to detect that situation? */
			DBG_SCANNING("setting end of fcontext %p as undefined\n",tmpfcontext);
			tmpfcontext->end_o = BF2_OFFSET_UNDEFINED;
			tmpfcontext = tmpfcontext->parentfcontext;
			i++;
		}
	}
	do {
		invalidoffset = remove_cache_entry(btv,&scanning->nextfound, &scanning->siter);
	} while(scanning->nextfound && scanning->nextfound->charoffset_o < match_end_o);
	
	return invalidoffset;
}

static gboolean enlarge_scanning_region_to_iter(BluefishTextView * btv, Tscanning *scanning, GtkTextIter *iter) {
	if (gtk_text_iter_compare(iter,&scanning->end)>0) {
		DBG_SCANCACHE("enlarge_scanning_region to offset %d\n",gtk_text_iter_get_offset(iter));
		remove_all_highlighting_in_area(btv, &scanning->end, iter);
		scanning->end = *iter;
		return TRUE;
	}
	return FALSE;
}
static gboolean enlarge_scanning_region(BluefishTextView * btv, Tscanning *scanning, guint offset) {
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_offset(btv->buffer, &iter, offset);
	return enlarge_scanning_region_to_iter(btv,scanning,&iter);
}

static inline int found_match(BluefishTextView * btv, Tmatch *match, Tscanning *scanning)
{
	Tfoundblock *fblock=scanning->curfblock;
	Tfoundcontext *fcontext=scanning->curfcontext;
	guint match_end_o;
	Tfound *found;
	GtkTextIter iter;
	Tpattern pat = g_array_index(btv->bflang->st->matches,Tpattern, match->patternum);
	DBG_SCANNING("found_match for pattern %d %s at charoffset %d, starts_block=%d,ends_block=%d, nextcontext=%d (current=%d)\n",match->patternum,pat.pattern, gtk_text_iter_get_offset(&match->start),pat.starts_block,pat.ends_block,pat.nextcontext,scanning->context);
/*	DBG_MSG("pattern no. %d (%s) matches (%d:%d) --> nextcontext=%d\n", match->patternum, scantable.matches[match->patternum].message,
			gtk_text_iter_get_offset(&match->start), gtk_text_iter_get_offset(&match->end), scantable.matches[match->patternum].nextcontext);*/
#ifdef IDENTSTORING
	scanning->identmode = pat.identmode;
#endif /* IDENTSTORING */

	match_end_o = gtk_text_iter_get_offset(&match->end);

	if (pat.selftag) {
		DBG_SCANNING("found_match, apply tag %p from %d to %d\n",pat.selftag,gtk_text_iter_get_offset(&match->start),gtk_text_iter_get_offset(&match->end));
		gtk_text_buffer_apply_tag(btv->buffer,pat.selftag, &match->start, &match->end);
	}
	if (!pat.starts_block && !pat.ends_block && (pat.nextcontext == 0 || pat.nextcontext == scanning->context))
		return scanning->context;
	
	/* There are three situations comparing the current scan to the cached results:
	1: the cache entry has an offset lower than the current offset or equal but a different patternum and
		is thus not valid anymore. That means that the region that needs scanning should be enlarged up 
		to the fcontext or fblock end from the cache.
	2: the cache entry has the same offset and the same patternum and is thus valid, we only highlight and 
		store nothing in the cache
	3: the cache entry has a higher offset -> do nothing with the cached one, 
		and enlarge the area to scan because we have a context change or new block that
		previously didn't exist 
	*/
	if (scanning->nextfound) {
		DBG_SCANCACHE("found_match, testing nextfound %p\n",scanning->nextfound);
		if (scanning->nextfound->charoffset_o > match_end_o) {
			DBG_SCANCACHE("found_match, next item in the cache (offset %d) is not relevant yet (offset now %d), set scanning end to %d\n",scanning->nextfound->charoffset_o, match_end_o,scanning->nextfound->charoffset_o);
			enlarge_scanning_region(btv, scanning, scanning->nextfound->charoffset_o);
		} else if (scanning->nextfound->charoffset_o == match_end_o && cached_found_is_valid(btv, match, scanning)){
			Tfoundcontext *tmpfcontext = scanning->curfcontext;
			gint context;
			DBG_SCANCACHE("found_match, cache item at offset %d is still valid\n",scanning->nextfound->charoffset_o);
			if (scanning->nextfound->numcontextchange >= 0) {
				context = scanning->nextfound->fcontext ? scanning->nextfound->fcontext->context : 1;
			} else if (pat.nextcontext < 0) {
				tmpfcontext = pop_and_apply_contexts(btv, pat.nextcontext, scanning->curfcontext, &match->start);
				Tfoundcontext *tmpfcontext2 = pop_contexts(pat.nextcontext, scanning->nextfound->fcontext);
				context = tmpfcontext ? tmpfcontext->context : 1;
				if (tmpfcontext != tmpfcontext2) {
					g_warning("found_match, ERROR: popped context from cache does not equal popped context from current scan\n");
				}
			}
			
			scanning->curfblock = pop_blocks(scanning->nextfound->numblockchange, fblock);
			scanning->curfcontext = tmpfcontext;
			scanning->nextfound = get_foundcache_next(btv,&scanning->siter);
			return context;
		} else { /* either a smaller offset, or invalid */
			guint invalidoffset;
			DBG_SCANCACHE("found_match, found %p with offset %d will be removed\n",scanning->nextfound, scanning->nextfound->charoffset_o);
			invalidoffset = remove_invalid_cache(btv, match_end_o, scanning);
			enlarge_scanning_region(btv, scanning, invalidoffset);
		}
	}
	/* TODO: in all cases: if we find a previously unknown match and there is no nextfound we
	have to scan to the end of the text */
	if (!scanning->nextfound) {
		gtk_text_buffer_get_end_iter(btv->buffer, &iter);
		enlarge_scanning_region_to_iter(btv,scanning,&iter);
	}

	found = g_slice_new0(Tfound);
#ifdef HL_PROFILING
	hl_profiling.found_refcount++;
#endif
	if (pat.starts_block) {
		fblock = found_start_of_block(btv, match, scanning);
		found->numblockchange = 1;
	} else if (pat.ends_block) {
		gint numblockchange=0;
		fblock = found_end_of_block(btv, match, scanning, &pat, &numblockchange);
		found->numblockchange = numblockchange;
	}

	if (pat.nextcontext != 0 && pat.nextcontext != scanning->context) {
		fcontext = found_context_change(btv, match, scanning, &pat);
		found->numcontextchange = pat.nextcontext <= 0 ?  pat.nextcontext : 1;
	} else {
		found->numcontextchange = 0;
	}
	found->fblock = fblock;
	found->fcontext = fcontext;
	found->charoffset_o = match_end_o;
	DBG_SCANCACHE("found_match, put found %p in the cache at charoffset_o %d fblock %p fcontext %p\n",found,found->charoffset_o,found->fblock,found->fcontext);
	g_sequence_insert_sorted(btv->scancache.foundcaches,found,foundcache_compare_charoffset_o,NULL);
	return scanning->context;
}

static gboolean bftextview2_find_region2scan(BluefishTextView * btv, GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end) {
	/* first find a region that needs scanning */
	gtk_text_buffer_get_start_iter(buffer, start);
	if (!gtk_text_iter_begins_tag(start,btv->needscanning) ) {
		if (!gtk_text_iter_forward_to_tag_toggle(start,btv->needscanning)) {
			/* nothing to scan */
			DBG_DELAYSCANNING("nothing to scan..\n");
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
	DBG_SCANNING("first region that needs scanning runs from %d to %d\n",gtk_text_iter_get_offset(start),gtk_text_iter_get_offset(end));
	/* now move start to the beginning of the line and end to the end of the line */
	/*gtk_text_iter_set_line_offset(start,0);
	DBG_SCANNING("set startposition to beginning of line, offset is now %d\n",gtk_text_iter_get_offset(start));
	gtk_text_iter_forward_to_line_end(end);
	gtk_text_iter_forward_char(end);*/
	return TRUE;
}

static guint reconstruct_scanning(BluefishTextView * btv, GtkTextIter *position, Tscanning *scanning) {
	Tfound *found;
	DBG_SCANNING("reconstruct_scanning at position %d\n",gtk_text_iter_get_offset(position));
	found = get_foundcache_at_offset(btv, gtk_text_iter_get_offset(position), &scanning->siter);
	DBG_SCANCACHE("reconstruct_stack, got found %p to reconstruct stack at position %d\n",found,gtk_text_iter_get_offset(position));
	if (G_LIKELY(found)) {
		if (found->numcontextchange < 0) {
			scanning->curfcontext = pop_contexts(found->numcontextchange, found->fcontext); 
		} else {
			scanning->curfcontext = found->fcontext;
		}
		if (found->numblockchange < 0) {
			scanning->curfblock = pop_blocks(found->numblockchange, found->fblock); 
		} else {
			scanning->curfblock = found->fblock;
		}
		scanning->context = (scanning->curfcontext) ? scanning->curfcontext->context : 1;

		scanning->nextfound = get_foundcache_next(btv, &scanning->siter);
		DBG_SCANNING("reconstruct_stack, found at offset %d, curfblock=%p, curfcontext=%p, context=%d\n", found->charoffset_o, scanning->curfblock, scanning->curfcontext, scanning->context);
		return found->charoffset_o;
	} else {
		DBG_SCANNING("nothing to reconstruct\n");
		scanning->curfcontext = NULL;
		scanning->curfblock = NULL;
		scanning->context = 1;
		scanning->nextfound=get_foundcache_first(btv,&scanning->siter);
		DBG_SCANNING("reconstruct_scanning, nextfound=%p\n",scanning->nextfound);
		return 0;
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

static void remove_old_scan_results(BluefishTextView *btv, GtkTextIter *fromhere) {
	GtkTextIter end;
	GSequenceIter *sit1,*sit2;
	Tfound fakefound;

	gtk_text_buffer_get_end_iter(btv->buffer,&end);
	DBG_SCANCACHE("remove_old_scan_results: remove tags from charoffset %d to %d\n",gtk_text_iter_get_offset(fromhere),gtk_text_iter_get_offset(&end));
	remove_all_highlighting_in_area(btv, fromhere, &end);
	fakefound.charoffset_o = gtk_text_iter_get_offset(fromhere);
	sit1 = g_sequence_search(btv->scancache.foundcaches,&fakefound,foundcache_compare_charoffset_o,NULL);
	if (sit1 && !g_sequence_iter_is_end(sit1)) {
		sit2 = g_sequence_get_end_iter(btv->scancache.foundcaches);
		DBG_SCANCACHE("sit1=%p, sit2=%p\n",sit1,sit2);
		DBG_SCANCACHE("remove_old_scan_results: remove foundcache entries %d to %d\n",g_sequence_iter_get_position(sit1),g_sequence_iter_get_position(sit2));
		g_sequence_foreach_range(sit1,sit2,found_free_lcb,btv);
		g_sequence_remove_range(sit1,sit2);
	} else{
		DBG_SCANCACHE("no sit1, no cleanup ??\n");
	}
}

/* if visible_end is set (not NULL) we will scan only the visible area and nothing else.
this can be used to delay scanning everything until the editor is idle for several milliseconds */
gboolean bftextview2_run_scanner(BluefishTextView * btv, GtkTextIter *visible_end)
{
	GtkTextIter iter, orig_end;
	GtkTextIter mstart;
	/*GArray *matchstack;*/
	Tscanning scanning;
	guint pos = 0, newpos, reconstruction_o;
	gboolean end_of_region=FALSE, last_character_run=FALSE, continue_loop=TRUE;
	gint loop=0;
#ifdef IDENTSTORING
	GtkTextIter itcursor;
#endif
#ifdef HL_PROFILING
	guint startpos;
	gdouble stage1=0;
	gdouble stage2;
	gdouble stage3;
	gdouble stage4;
	hl_profiling.longest_contextstack=0;
	hl_profiling.longest_blockstack=0;
	hl_profiling.numcontextstart=0;
	hl_profiling.numcontextend=0;
	hl_profiling.numblockstart=0;
	hl_profiling.numblockend=0;
	hl_profiling.numchars=0;
	hl_profiling.numloops=0;
#endif

	scanning.context = 1;
#ifdef IDENTSTORING
	scanning.identmode = 0;
#endif /* IDENTSTORING */

	DBG_MSG("bftextview2_run_scanner for btv %p..\n",btv);
	if (!btv->bflang->st) {
		DBG_MSG("no scantable, nothing to scan, returning...\n");
		return FALSE;
	}
#ifdef VALGRIND_PROFILING
	CALLGRIND_START_INSTRUMENTATION;
#endif /* VALGRIND_PROFILING */	

	if (!bftextview2_find_region2scan(btv, btv->buffer, &scanning.start, &scanning.end)) {
		DBG_MSG("nothing to scan here.. return FALSE\n");
#ifdef VALGRIND_PROFILING
		CALLGRIND_STOP_INSTRUMENTATION;
#endif /* VALGRIND_PROFILING */
		return FALSE;
	}
	/* start timer */
	scanning.timer = g_timer_new();

	orig_end = scanning.end;
	if (visible_end) {
		/* make sure that we only scan up to visible_end and no further */
		if (gtk_text_iter_compare(&scanning.start,visible_end)>0) {
			DBG_DELAYSCANNING("start of region that needs scanning is beyond visible_end, return TRUE\n");
			g_timer_destroy(scanning.timer);
#ifdef VALGRIND_PROFILING
			CALLGRIND_STOP_INSTRUMENTATION;
#endif /* VALGRIND_PROFILING */
			return TRUE;
		}
		if (gtk_text_iter_compare(&scanning.end,visible_end)>0) {
			DBG_DELAYSCANNING("end of region that needs scanning (%d) is beyond visible_end (%d), reset end\n",gtk_text_iter_get_offset(&scanning.end),gtk_text_iter_get_offset(visible_end));
			scanning.end = *visible_end;
		}
	}
#ifdef HL_PROFILING
	startpos = gtk_text_iter_get_offset(&scanning.start);
	stage1 = g_timer_elapsed(scanning.timer,NULL);
#endif
	iter = mstart = scanning.start;
	if (gtk_text_iter_is_start(&scanning.start)) {
		scanning.siter = g_sequence_get_begin_iter(btv->scancache.foundcaches);
		scanning.nextfound = NULL;
		scanning.curfcontext = NULL;
		scanning.curfblock = NULL;
		reconstruction_o = 0;
	} else {
		/* reconstruct the context stack and the block stack */
		reconstruction_o = reconstruct_scanning(btv, &iter, &scanning);
		pos = g_array_index(btv->bflang->st->contexts,Tcontext,scanning.context).startstate;
		DBG_SCANNING("reconstructed stacks, context=%d, startstate=%d\n",scanning.context,pos);
		/* now move the start position either to the start of the line, or to the position 
		where the stack was reconstructed, the largest offset */
		gtk_text_buffer_get_iter_at_offset(btv->buffer, &iter, reconstruction_o);
		gtk_text_iter_set_line_offset(&scanning.start,0);
		DBG_SCANNING("compare possible start positions %d and %d\n",gtk_text_iter_get_offset(&scanning.start),gtk_text_iter_get_offset(&iter));
		if (gtk_text_iter_compare(&iter,&scanning.start)>0)
			mstart = scanning.start = iter;
		else
			iter = mstart = scanning.start;
	}
	if (!gtk_text_iter_is_end(&scanning.end)) {
		/* the end position should be the largest of the end of the line and the 'end' iter */
		gtk_text_iter_forward_to_line_end(&iter);
		gtk_text_iter_forward_char(&iter);
		if (gtk_text_iter_compare(&iter,&scanning.end)>=0)
			scanning.end = iter;
		iter = scanning.start;
	}
	DBG_SCANNING("scanning from %d to %d\n",gtk_text_iter_get_offset(&scanning.start),gtk_text_iter_get_offset(&scanning.end));
	btv->scancache.valid_cache_offset = gtk_text_iter_get_offset(&scanning.start);
#ifdef HL_PROFILING
	stage2= g_timer_elapsed(scanning.timer,NULL);
#endif
	if (btv->needremovetags) {
		remove_all_highlighting_in_area(btv, &scanning.start, &scanning.end);
/*		remove_old_scan_results(btv, buffer, &start);*/
		btv->needremovetags = FALSE;
	}
#ifdef HL_PROFILING
	stage3 = g_timer_elapsed(scanning.timer,NULL);
#endif
/*	if (!visible_end)
		gtk_text_iter_forward_to_end(&end);
	else
		end = *visible_end;*/
#ifdef IDENTSTORING
	gtk_text_buffer_get_iter_at_mark(btv->buffer, &itcursor, gtk_text_buffer_get_insert(btv->buffer));
#endif
	do {
		gunichar uc;
		loop++;
#ifdef HL_PROFILING
		hl_profiling.numloops++;
#endif
		if (G_UNLIKELY(last_character_run)) {
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
		DBG_SCANNING("scanning offset %d pos %d %c ",gtk_text_iter_get_offset(&iter),uc,pos);
		newpos = g_array_index(btv->bflang->st->table, Ttablerow, pos).row[uc];
		DBG_SCANNING("(context=%d).. got newpos %d\n",scanning.context,newpos);
		if (G_UNLIKELY(newpos == 0 || uc == '\0')) {
			if (G_UNLIKELY(g_array_index(btv->bflang->st->table,Ttablerow, pos).match)) {
				Tmatch match;
				match.patternum = g_array_index(btv->bflang->st->table,Ttablerow, pos).match;
				match.start = mstart;
				match.end = iter;
				DBG_SCANNING("we have a match from pos %d to %d\n", gtk_text_iter_get_offset(&match.start),gtk_text_iter_get_offset(&match.end));
				scanning.context = found_match(btv, &match,&scanning);
				DBG_SCANNING("after match context=%d\n",scanning.context);
			} else {
				if G_UNLIKELY((uc=='\0' && scanning.nextfound && scanning.nextfound->charoffset_o == gtk_text_iter_get_offset(&iter))) {
					guint invalidoffset;
					/* scanning->nextfound is invalid! remove from cache */
					invalidoffset = remove_invalid_cache(btv, gtk_text_iter_get_offset(&iter), &scanning);
					if (enlarge_scanning_region(btv, &scanning, invalidoffset))
						last_character_run = FALSE;
				}
#ifdef IDENTSTORING
				if (G_UNLIKELY(scanning.identmode != 0 && pos == g_array_index(btv->bflang->st->contexts,Tcontext,scanning.context).identstate)) {
					/* ignore if the cursor is within the range, because it could be that the user is still typing the name */
					if (G_LIKELY(!gtk_text_iter_in_range(&itcursor, &mstart, &iter) 
									&& !gtk_text_iter_equal(&itcursor, &mstart) 
									&& !gtk_text_iter_equal(&itcursor, &iter))) {
						found_identifier(btv, &mstart, &iter, scanning.context, scanning.identmode);
						scanning.identmode = 0;
					}
				}
#endif /* IDENTSTORING */
			}
			g_print("last_character_run=%d, scanning.nextfound=%p\n",last_character_run, scanning.nextfound);
			if (G_UNLIKELY(last_character_run && scanning.nextfound && !nextcache_valid(&scanning))) {
				guint invalidoffset;
				/* see if nextfound has a valid context and block stack, if not we enlarge the scanning area */
				DBG_SCANNING("last_character_run, but nextfound %p is INVALID!\n",scanning.nextfound);
				invalidoffset = remove_invalid_cache(btv, 0, &scanning);
				enlarge_scanning_region(btv, &scanning, invalidoffset);
				/* TODO: do we need to rescan this position with the real uc instead of uc='\0' ?? */
			}
			
			if (G_LIKELY(gtk_text_iter_equal(&mstart,&iter) && !last_character_run)) {
				gtk_text_iter_forward_char(&iter);
#ifdef HL_PROFILING
				hl_profiling.numchars++;
#endif
			}
			mstart = iter;
			newpos = g_array_index(btv->bflang->st->contexts,Tcontext,scanning.context).startstate;
		} else if (G_LIKELY(!last_character_run)){
			gtk_text_iter_forward_char(&iter);
#ifdef HL_PROFILING
			hl_profiling.numchars++;
#endif
		}
		pos = newpos;
		end_of_region = gtk_text_iter_equal(&iter, &scanning.end);
		if (G_UNLIKELY(end_of_region || last_character_run)) {
			last_character_run = !last_character_run;
		}
		continue_loop = (!end_of_region || last_character_run);
		DBG_SCANNING("continue_loop=%d, end_of_region=%d, last_character_run=%d\n",continue_loop,end_of_region,last_character_run);
	} while (continue_loop && (loop%loops_per_timer!=0 || g_timer_elapsed(scanning.timer,NULL)<MAX_CONTINUOUS_SCANNING_INTERVAL));
	DBG_SCANNING("scanned from %d to position %d, (end=%d, orig_end=%d) which took %f microseconds, loops_per_timer=%d\n",gtk_text_iter_get_offset(&scanning.start),gtk_text_iter_get_offset(&iter),gtk_text_iter_get_offset(&scanning.end),gtk_text_iter_get_offset(&orig_end),g_timer_elapsed(scanning.timer,NULL),loops_per_timer);
	/* TODO: if we end the scan within a context that has a tag, we have to apply the contexttag */
	if (!gtk_text_iter_is_end(&scanning.end) && scanning.curfcontext) {
		if (g_array_index(btv->bflang->st->contexts,Tcontext,scanning.curfcontext->context).contexttag) {
			GtkTextIter iter2;
			gtk_text_buffer_get_iter_at_offset(btv->buffer,&iter2,scanning.curfcontext->start_o);
			gtk_text_buffer_apply_tag(btv->buffer,g_array_index(btv->bflang->st->contexts,Tcontext,scanning.curfcontext->context).contexttag, &iter, &iter2);
		}
	} 
	
	gtk_text_buffer_remove_tag(btv->buffer, btv->needscanning, &scanning.start , &iter);
	gtk_text_buffer_apply_tag(btv->buffer,btv->needscanning,&iter,&scanning.end);
	
#ifdef HL_PROFILING
	stage4 = g_timer_elapsed(scanning.timer,NULL);
	hl_profiling.total_runs++;
	hl_profiling.total_chars += hl_profiling.numchars;
	hl_profiling.total_time_ms += (gint)(1000.0*stage4); 
	g_print("scanning run %d (%d ms): %d, %d, %d, %d; from %d-%d, loops=%d,chars=%d,blocks %d/%d (%d) contexts %d/%d (%d) scancache %d\n"
		,hl_profiling.total_runs
		,(gint)(1000.0*stage4)
		,(gint)(1000.0*stage1)
		,(gint)(1000.0*stage2-stage1)
		,(gint)(1000.0*stage3-stage2)
		,(gint)(1000.0*stage4-stage3)
		,startpos,gtk_text_iter_get_offset(&iter)
		,hl_profiling.numloops,hl_profiling.numchars
		,hl_profiling.numblockstart,hl_profiling.numblockend,hl_profiling.longest_blockstack
		,hl_profiling.numcontextstart,hl_profiling.numcontextend,hl_profiling.longest_contextstack
		,g_sequence_get_length(btv->scancache.foundcaches)
		);
	g_print("memory scancache %d(%dKb+%dKb) found %d(%dKb) fcontext %d(%dKb) = %dKb\n"
		,hl_profiling.found_refcount,(gint)(hl_profiling.found_refcount*sizeof(Tfound)/1024.0),(gint)(hl_profiling.found_refcount*40/1024.0)
		,hl_profiling.fblock_refcount,(gint)(hl_profiling.fblock_refcount*sizeof(Tfoundblock)/1024.0)
		,hl_profiling.fcontext_refcount,(gint)(hl_profiling.fcontext_refcount*sizeof(Tfoundcontext)/1024.0)
		,(gint)((hl_profiling.found_refcount*sizeof(Tfound)+hl_profiling.found_refcount*5*sizeof(gpointer)+hl_profiling.fblock_refcount*sizeof(Tfoundblock)+hl_profiling.fcontext_refcount*sizeof(Tfoundcontext))/1024.0)
		);
	g_print("average %d chars/s %d chars/run\n"
			,(guint)(1000.0*hl_profiling.total_chars / hl_profiling.total_time_ms )
			,(guint)(1.0*hl_profiling.total_chars / hl_profiling.total_runs )
			);
#endif
	/* tune the loops_per_timer, try to have 10 timer checks per loop, so we have around 10% deviation from the set interval */
	if (!end_of_region)
		loops_per_timer = MAX(loop/10,200);
	g_timer_destroy(scanning.timer);

#ifdef VALGRIND_PROFILING
	CALLGRIND_STOP_INSTRUMENTATION;
#endif /* VALGRIND_PROFILING */

#ifdef DUMP_SCANCACHE
	dump_scancache(btv);
#endif

	DBG_MSG("cleaned scanning run, finished this run\n");
	return !gtk_text_iter_is_end(&iter); 
}

GQueue *get_contextstack_at_position(BluefishTextView * btv, GtkTextIter *position) {
	Tfound *found;
	GQueue *retqueue = g_queue_new();
	found = get_foundcache_at_offset(btv, gtk_text_iter_get_offset(position), NULL);
	if (found) {
		Tfoundcontext *tmpfcontext = found->fcontext;
		while (tmpfcontext) {
			gint context = tmpfcontext->context;
			g_queue_push_tail(retqueue, GINT_TO_POINTER(context));
			tmpfcontext = (Tfoundcontext *)tmpfcontext->parentfcontext;
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
	*contextnum = g_queue_get_length(contextstack) ? GPOINTER_TO_INT(g_queue_peek_head(contextstack)): 1;
	pos = g_array_index(btv->bflang->st->contexts,Tcontext, *contextnum).startstate;
	DBG_AUTOCOMP("start scanning at offset %d with context %d and position %d\n",gtk_text_iter_get_offset(&iter),*contextnum,pos);
	while (!gtk_text_iter_equal(&iter, cursorpos)) {
		gunichar uc;
		uc = gtk_text_iter_get_char(&iter);
		if (G_UNLIKELY(uc > 128)) {
				/* multibyte characters cannot be matched by the engine. character
				1 in ascii is "SOH (start of heading)". we need this to support a
				pattern like [^#]* .  */
			uc = 1;
		}
		DBG_AUTOCOMP("scanning %c\n",uc);
		newpos = g_array_index(btv->bflang->st->table, Ttablerow, pos).row[uc];
		if (G_UNLIKELY(newpos == 0 || uc == '\0')) {
			DBG_AUTOCOMP("newpos=%d...\n",newpos);
			if (G_UNLIKELY(g_array_index(btv->bflang->st->table,Ttablerow, pos).match)) {
				if (g_array_index(btv->bflang->st->matches,Tpattern, g_array_index(btv->bflang->st->table,Ttablerow, pos).match).nextcontext < 0) {
					gint num  = g_array_index(btv->bflang->st->matches,Tpattern, g_array_index(btv->bflang->st->table,Ttablerow, pos).match).nextcontext;
					while (num != 0) {
						g_queue_pop_head(contextstack);
						num++;
					}
					*contextnum = g_queue_get_length(contextstack) ? GPOINTER_TO_INT(g_queue_peek_head(contextstack)): 1;
				} else if (g_array_index(btv->bflang->st->matches,Tpattern, g_array_index(btv->bflang->st->table,Ttablerow, pos).match).nextcontext > 0) {
					DBG_AUTOCOMP("previous pos=%d had a match with a context change!\n",pos);
					*contextnum = g_array_index(btv->bflang->st->matches,Tpattern, g_array_index(btv->bflang->st->table,Ttablerow, pos).match).nextcontext;
					g_queue_push_head(contextstack, GINT_TO_POINTER(*contextnum));
				}
				DBG_AUTOCOMP("found match %d, new context is %d\n",g_array_index(btv->bflang->st->table,Ttablerow, pos).match,*contextnum);
			}
			if (G_LIKELY(gtk_text_iter_equal(mstart,&iter))) {
				gtk_text_iter_forward_char(&iter);
			}
			*mstart = iter;
			newpos = g_array_index(btv->bflang->st->contexts,Tcontext, *contextnum).startstate;
		} else {
			gtk_text_iter_forward_char(&iter);
		}
		pos = newpos;
	}
	g_queue_free(contextstack);
	DBG_AUTOCOMP("scan_for_autocomp_prefix, return mstart at %d, cursor at %d, context %d\n",gtk_text_iter_get_offset(mstart),gtk_text_iter_get_offset(cursorpos),*contextnum);
}

gboolean scan_for_tooltip(BluefishTextView *btv,GtkTextIter *mstart,GtkTextIter *position,gint *contextnum) {
	GtkTextIter iter,end;
	guint16 pos,newpos;
	gboolean retthismatch=FALSE;
	GQueue *contextstack;
	/* get the current context */
	iter = *mstart;

	contextstack = get_contextstack_at_position(btv, &iter);
	*contextnum = g_queue_get_length(contextstack) ? GPOINTER_TO_INT(g_queue_peek_head(contextstack)): 1;
	pos = g_array_index(btv->bflang->st->contexts,Tcontext, *contextnum).startstate;

	gtk_text_buffer_get_end_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv)),&end);
	DBG_TOOLTIP("start scanning at offset %d with context %d and position %d\n",gtk_text_iter_get_offset(&iter),*contextnum,pos);
	while (!gtk_text_iter_equal(&iter, &end)) {
		gunichar uc;
		uc = gtk_text_iter_get_char(&iter);
		if (G_UNLIKELY(uc > 128)) {
			newpos = 0;
		} else {
			DBG_TOOLTIP("scanning %c\n",uc);
			newpos = g_array_index(btv->bflang->st->table, Ttablerow, pos).row[uc];
		}
		if (G_UNLIKELY(newpos == 0 || uc == '\0')) {
			DBG_TOOLTIP("newpos=%d...\n",newpos);
			if (G_UNLIKELY(g_array_index(btv->bflang->st->table,Ttablerow, pos).match)) {
				DBG_MSG("found match %d, retthismatch=%d\n",g_array_index(btv->bflang->st->table,Ttablerow, pos).match,retthismatch);
				if (retthismatch) {
					*position = iter;
					g_queue_free(contextstack);
					DBG_TOOLTIP("return TRUE, mstart %d position %d\n",gtk_text_iter_get_offset(mstart),gtk_text_iter_get_offset(position));
					return TRUE;
				}
				if (g_array_index(btv->bflang->st->matches,Tpattern, g_array_index(btv->bflang->st->table,Ttablerow, pos).match).nextcontext < 0) {
					gint num  = g_array_index(btv->bflang->st->matches,Tpattern, g_array_index(btv->bflang->st->table,Ttablerow, pos).match).nextcontext;
					while (num != 0) {
						g_queue_pop_head(contextstack);
						num++;
					}
					*contextnum = g_queue_get_length(contextstack) ? GPOINTER_TO_INT(g_queue_peek_head(contextstack)): 1;
					DBG_TOOLTIP("previous pos=%d had a match that popped the context to %d!\n",pos,*contextnum);
				} else if (g_array_index(btv->bflang->st->matches,Tpattern, g_array_index(btv->bflang->st->table,Ttablerow, pos).match).nextcontext > 0) {
					*contextnum = g_array_index(btv->bflang->st->matches,Tpattern, g_array_index(btv->bflang->st->table,Ttablerow, pos).match).nextcontext;
					DBG_TOOLTIP("previous pos=%d had a match that pushed the context to %d!\n",pos,*contextnum);
					g_queue_push_head(contextstack, GINT_TO_POINTER(*contextnum));
				}
			} else if (retthismatch) {
				g_queue_free(contextstack);
				return FALSE;
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
			DBG_TOOLTIP("at cursor position..., scanning in context %d, pos %d (identstate=%d)\n",*contextnum,pos,g_array_index(btv->bflang->st->contexts,Tcontext, *contextnum).identstate);
			if (gtk_text_iter_equal(&iter, mstart) || (pos==g_array_index(btv->bflang->st->contexts,Tcontext, *contextnum).identstate)) {
				g_queue_free(contextstack);
				return FALSE;
			}
			retthismatch = TRUE;
		}
	}
	g_queue_free(contextstack);
	return FALSE;
}

void cleanup_scanner(BluefishTextView *btv) {
	GtkTextIter begin,end;
	GSequenceIter *sit1,*sit2;

	gtk_text_buffer_get_bounds(btv->buffer,&begin,&end);
	gtk_text_buffer_remove_all_tags(btv->buffer,&begin,&end);

	g_sequence_foreach(btv->scancache.foundcaches,found_free_lcb,btv);
	sit1 = g_sequence_get_begin_iter(btv->scancache.foundcaches);
	if (sit1 && !g_sequence_iter_is_end(sit1)) {
		sit2 = g_sequence_get_end_iter(btv->scancache.foundcaches);
		/*g_sequence_foreach_range(sit1,sit2,foundstack_free_lcb,btv);*/
		g_sequence_remove_range(sit1,sit2);
	} else{
		/*DBG_SCANNING("cleanup_scanner, no sit1, no cleanup ??\n");*/
	}
#ifdef HL_PROFILING
	g_print("cleanup_scanner, memory scancache %d(%dKb+%dKb) found %d(%dKb) fcontext %d(%dKb) = %dKb\n"
		,hl_profiling.found_refcount,(gint)(hl_profiling.found_refcount*sizeof(Tfound)/1024.0),(gint)(hl_profiling.found_refcount*40/1024.0)
		,hl_profiling.fblock_refcount,(gint)(hl_profiling.fblock_refcount*sizeof(Tfoundblock)/1024.0)
		,hl_profiling.fcontext_refcount,(gint)(hl_profiling.fcontext_refcount*sizeof(Tfoundcontext)/1024.0)
		,(gint)((hl_profiling.found_refcount*sizeof(Tfound)+hl_profiling.found_refcount*5*sizeof(gpointer)+hl_profiling.fblock_refcount*sizeof(Tfoundblock)+hl_profiling.fcontext_refcount*sizeof(Tfoundcontext))/1024.0)
		);

#endif
#ifdef IDENTSTORING
	bftextview2_identifier_hash_remove_doc(DOCUMENT(btv->doc)->bfwin, btv->doc);
#endif /* IDENTSTORING */

}

void scancache_destroy(BluefishTextView *btv) {
	g_sequence_foreach(btv->scancache.foundcaches,found_free_lcb,btv);
	g_sequence_free(btv->scancache.foundcaches);
	btv->scancache.foundcaches = NULL;
}
