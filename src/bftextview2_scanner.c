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
/*#define MINIMAL_REFCOUNTING*/
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
	Tfound *curfound; /* items from the cache */
	Tfound *nextfound; /* items from the cache */
	GSequenceIter *siter; /* an iterator to get the next item from the cache */
	GTimer *timer;
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
	gint num_marks;
	guint total_runs;
	guint total_chars;
	guint total_marks;
	guint total_time_ms;
} Thl_profiling;

Thl_profiling hl_profiling = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#endif

guint loops_per_timer=1000; /* a tunable to avoid asking time too often. this is auto-tuned. */ 

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
			DBG_SCANCACHE("no siter no stack\n");
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
	DBG_SCANCACHE("foundcache_update_offsets, update offset %d starting at startpos %d, cache length=%d\n",offset,startpos,g_sequence_get_length(btv->scancache.foundcaches) );
	found = get_foundcache_at_offset(btv, startpos, &siter);
	if (found)
		DBG_SCANCACHE("foundcache_update_offsets, got found %p with offset %d, foundmode %d\n",found, found->charoffset_o, found->foundmode);
	else
		DBG_SCANCACHE("foundcache_update_offsets, got found %p\n",found);
	if (offset < 0) {
		while (found && found->charoffset_o < startpos-offset) {
			if (found->charoffset_o > startpos) {
				GSequenceIter *tmpsiter = siter;
				Tfound *tmpfound=found;
				found = get_foundcache_next(btv, &siter);
				DBG_SCANCACHE("foundcache_update_offsets, remove found %p with charoffset_o=%d\n",found,found->charoffset_o);
				g_sequence_remove(tmpsiter);
				found_free_lcb(tmpfound, btv);
			} else {
				found = get_foundcache_next(btv, &siter);
			}
		}		
	}
	if (found && found->charoffset_o < startpos) {
		Tfoundcontext *tmpfcontext;
		Tfoundblock *tmpfblock;
		DBG_SCANCACHE("foundcache_update_offsets, handle first found %p with offset %d, foundmode %d, complete stack\n",found, found->charoffset_o, found->foundmode);
		/* for the first found, we have to update the end-offsets for all contexts/blocks on the stack */

		tmpfcontext = found->fcontext;
		while(tmpfcontext) {
			DBG_SCANCACHE("fcontext on stack=%p, start_o=%d end_o=%d\n",tmpfcontext, tmpfcontext->start_o, tmpfcontext->end_o);
			if (tmpfcontext->end_o != BF2_OFFSET_UNDEFINED) {
				DBG_SCANCACHE("update fcontext %p end from %d to %d\n",tmpfcontext, tmpfcontext->end_o, tmpfcontext->end_o+offset);
				tmpfcontext->end_o += offset;
			}
			tmpfcontext = (Tfoundcontext *)tmpfcontext->parentfcontext;
		}

		tmpfblock = found->fblock;
		while(tmpfblock) {
			DBG_SCANCACHE("fblock on stack=%p, start1_o=%d end2_o=%d\n",tmpfblock, tmpfblock->start1_o, tmpfblock->end2_o);
			if (tmpfblock->start2_o != BF2_OFFSET_UNDEFINED) {
				DBG_SCANCACHE("update fblock %p with start1_o=%d and start2_o=%d to start2_o=%d\n",tmpfblock, tmpfblock->start1_o,tmpfblock->start2_o, tmpfblock->start2_o+offset);
				tmpfblock->start2_o += offset;
			}
			if (tmpfblock->end2_o != BF2_OFFSET_UNDEFINED)
				tmpfblock->end2_o += offset;
			
			tmpfblock = (Tfoundblock *)tmpfblock->parentfblock;
		}
		
		DBG_SCANCACHE("foundcache_update_offsets, handled first found %p, requesting next\n",found);
		/*this offset is *before* 'position' found->charoffset_o += offset;*/
		found = get_foundcache_next(btv, &siter);
	}
	while (found) {
		DBG_SCANCACHE("foundcache_update_offsets, about to update found %p with charoffset %d, mode %d fcontext %p and fblock %p\n",found,found->charoffset_o,found->foundmode, found->fcontext, found->fblock);
		/* for all further founds, we only handle the pushedblock and pushedcontext */
		if (IS_FOUNDMODE_CONTEXTPUSH(found->foundmode)) {
			DBG_SCANCACHE("mode contextpush %p with start_o=%d and end_o=%d\n",found->fcontext,found->fcontext->start_o,found->fcontext->end_o);
			if (found->fcontext->start_o != BF2_OFFSET_UNDEFINED)
				found->fcontext->start_o += offset;
			if (found->fcontext->end_o != BF2_OFFSET_UNDEFINED)
				found->fcontext->end_o += offset;
		}
		if (IS_FOUNDMODE_BLOCKPUSH(found->foundmode)) {
			DBG_SCANCACHE("mode blockpush %p with start1_o=%d and end2_o=%d\n",found->fblock,found->fblock->start1_o,found->fblock->end2_o);
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
}

void found_free_lcb(gpointer data, gpointer btv) {
	Tfound *found = data;

	if (IS_FOUNDMODE_BLOCKPUSH(found->foundmode)) {
#ifdef HL_PROFILING
		hl_profiling.fblock_refcount--;
#endif
		g_slice_free(Tfoundblock, found->fblock);
	}
	if (IS_FOUNDMODE_CONTEXTPUSH(found->foundmode)) {
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

static inline Tfoundblock *found_start_of_block(BluefishTextView * btv,const Tmatch match, Tscanning *scanning) {
	Tfoundblock *fblock;
	DBG_BLOCKMATCH("found_start_of_block, put block for pattern %d (%s) on blockstack\n",match.patternum,g_array_index(btv->bflang->st->matches,Tpattern,match.patternum).pattern);
#ifdef HL_PROFILING
	hl_profiling.numblockstart++;
	hl_profiling.fblock_refcount++;
#endif
	fblock = g_slice_new0(Tfoundblock);
	fblock->start1_o = gtk_text_iter_get_offset(&match.start);
	fblock->end1_o = gtk_text_iter_get_offset(&match.end);
	/*g_print("found blockstart with start_1 %d end1 %d\n",fblock->start1_o,fblock->end1_o);*/
	fblock->start2_o = BF2_OFFSET_UNDEFINED;
	fblock->end2_o = BF2_OFFSET_UNDEFINED;
	fblock->patternum = match.patternum;
	fblock->parentfblock = scanning->curfblock;
	DBG_BLOCKMATCH("found_start_of_block, new block at %p with parent %p\n",fblock, fblock->parentfblock);
	scanning->curfblock = fblock;
	return fblock;
}

static inline Tfoundblock *found_end_of_block(BluefishTextView * btv,const Tmatch match, Tscanning *scanning, Tpattern *pat) {
	Tfoundblock *fblock=scanning->curfblock;
	GtkTextIter iter;
	DBG_BLOCKMATCH("found end of block with blockstartpattern %d, curfblock=%p\n",pat->blockstartpattern, scanning->curfblock);

	if (G_UNLIKELY(!scanning->curfblock))
		return NULL;
	
#ifdef HL_PROFILING
	hl_profiling.numblockend++;
#endif
	while (fblock && fblock->patternum != pat->blockstartpattern && pat->blockstartpattern != -1) {
		DBG_BLOCKMATCH("pop fblock %p with patternum %d and parent %p\n", fblock, fblock->patternum, fblock->parentfblock);
		fblock = (Tfoundblock *)fblock->parentfblock;
	}

	if (G_UNLIKELY(!fblock)) {
		DBG_BLOCKMATCH("no matching start-of-block found\n");
		return NULL;
	}

	DBG_BLOCKMATCH("found the matching start of the block fblock %p with patternum %d and parent %p\n",fblock, fblock->patternum, fblock->parentfblock);
	fblock->start2_o = gtk_text_iter_get_offset(&match.start);
	fblock->end2_o = gtk_text_iter_get_offset(&match.end);
	
	gtk_text_buffer_get_iter_at_offset(btv->buffer,&iter,fblock->end1_o);
	if (pat->blocktag) {
		gtk_text_buffer_apply_tag(btv->buffer,pat->blocktag, &iter, &match.start);
	}
	if ((gtk_text_iter_get_line(&iter)+1) < gtk_text_iter_get_line(&match.start)) {
		fblock->foldable = TRUE;
	}

	scanning->curfblock = fblock->parentfblock;
	return scanning->curfblock;
}

static inline Tfoundcontext *found_context_change(BluefishTextView * btv,const Tmatch match, Tscanning *scanning, Tpattern *pat) {
	Tfoundcontext *fcontext=scanning->curfcontext;
	/* check if we change up or down the stack */
	if (pat->nextcontext < 0) {
		gint num = -1 * pat->nextcontext;
		guint matchstart = gtk_text_iter_get_offset(&match.start);
#ifdef HL_PROFILING
		hl_profiling.numcontextend++;
#endif
		/* pop, but don't pop if there is nothing to pop (because of an error in the language file) */
		DBG_SCANNING("found_context_change, should pop %d contexts, fcontext=%p\n",num,fcontext);
		while (num > 0 && fcontext) {
			DBG_SCANNING("found_context_change, end context %d at pos %d, has tag %p and parent %p\n",fcontext->context, matchstart, g_array_index(btv->bflang->st->contexts,Tcontext,fcontext->context).contexttag, fcontext->parentfcontext);
			fcontext->end_o = matchstart;
			if (g_array_index(btv->bflang->st->contexts,Tcontext,fcontext->context).contexttag) {
				GtkTextIter iter;
				gtk_text_buffer_get_iter_at_offset(btv->buffer,&iter,fcontext->start_o);
				gtk_text_buffer_apply_tag(btv->buffer,g_array_index(btv->bflang->st->contexts,Tcontext,fcontext->context).contexttag, &iter, &match.start);
			}
			fcontext = (Tfoundcontext *)fcontext->parentfcontext;
			num--;
		}
		DBG_SCANNING("found_context_change, return (current) fcontext=%p\n",fcontext);
		scanning->curfcontext = fcontext;
		return fcontext;
	} else {
#ifdef HL_PROFILING
		hl_profiling.numcontextstart++;
		hl_profiling.fcontext_refcount++;
#endif
		fcontext = g_slice_new0(Tfoundcontext);
		fcontext->start_o = gtk_text_iter_get_offset(&match.end);
		fcontext->end_o = BF2_OFFSET_UNDEFINED;
		fcontext->context = pat->nextcontext;
		fcontext->parentfcontext = scanning->curfcontext;
		DBG_SCANNING("found_context_change, new fcontext %p with context %d onto the stack, parent=%p\n",fcontext, pat->nextcontext, fcontext->parentfcontext);
		scanning->curfcontext = fcontext;
		return fcontext;
	}
}

static inline gboolean cached_found_is_valid(BluefishTextView * btv, Tmatch *match, Tscanning *scanning) {
	Tpattern pat = g_array_index(btv->bflang->st->matches,Tpattern, match->patternum);
	if (IS_FOUNDMODE_BLOCKPUSH(scanning->nextfound->foundmode) && (
				!pat.starts_block
				|| scanning->nextfound->fblock->patternum != match->patternum
				))
		return FALSE;
	if (IS_FOUNDMODE_BLOCKPOP(scanning->nextfound->foundmode) && !pat.ends_block)
		return FALSE;
	if (IS_FOUNDMODE_CONTEXTPUSH(scanning->nextfound->foundmode) && (
				pat.nextcontext <= 0 
				|| pat.nextcontext != scanning->context
				|| scanning->nextfound->fcontext->context != pat.nextcontext
				))
		return FALSE;
	if (IS_FOUNDMODE_CONTEXTPOP(scanning->nextfound->foundmode) 
				&& (pat.nextcontext >= 0 || pat.nextcontext != scanning->context))
		return FALSE;
	return TRUE;
}

static void remove_invalid_cache(BluefishTextView * btv, Tmatch *match, Tscanning *scanning) {
	guint blockstackcount=0, contextstackcount=0;
	
	Tfound *tmpfound = scanning->nextfound;
	GSequenceIter *tmpsiter = scanning->siter;
	/* TODO:
	if this entry pushes a context or block on the stack, we should remove entries that 
	refer to this fcontext or fblock from the cache too, else we crash/segfault
	
	if the entry has an offset < match->end we also have to remove this and every entry 
	up to the current offset */
	DBG_SCANNING("cache item %p is NO LONGER valid\n",scanning->nextfound);
	scanning->nextfound = get_foundcache_next(btv,&scanning->siter);
	if (IS_FOUNDMODE_BLOCKPOP(tmpfound->foundmode) ) {
		/* TODO: the *previous* fblock is popped here, and this one is invalid...
		now we should set the *previous* fblock end to 'unknown'  */
	}
	if (IS_FOUNDMODE_CONTEXTPOP(tmpfound->foundmode) ) {
		/* TODO: the *previous* fcontext is popped here, and this one is invalid...
		now we should set the *previous* fcontext end to 'unknown'  */
	}
	if (IS_FOUNDMODE_BLOCKPUSH(tmpfound->foundmode) )
		blockstackcount=1;
	if (IS_FOUNDMODE_CONTEXTPUSH(tmpfound->foundmode) )
		contextstackcount=1;
	
	DBG_SCANCACHE("remove Tfound %p from the cache and free\n",tmpfound);
	g_sequence_remove(tmpsiter);
	found_free_lcb(tmpfound, btv);
	
	while (blockstackcount > 0 && contextstackcount > 0) {
		tmpfound = scanning->nextfound;
		tmpsiter = scanning->siter;
		scanning->nextfound = get_foundcache_next(btv,&scanning->siter);
		if (IS_FOUNDMODE_BLOCKPOP(tmpfound->foundmode) ) {
			/* TODO: the *previous* fblock is popped here, and this one is invalid...
			now we should set the *previous* fblock end to 'unknown'  */
			blockstackcount--;
		}
		if (IS_FOUNDMODE_CONTEXTPOP(tmpfound->foundmode) ) {
			/* TODO: the *previous* fcontext is popped here, and this one is invalid...
			now we should set the *previous* fcontext end to 'unknown'  */
			contextstackcount--;
		}
		if (blockstackcount > 0 && IS_FOUNDMODE_BLOCKPUSH(tmpfound->foundmode) )
			blockstackcount++;
		if (contextstackcount > 0 && IS_FOUNDMODE_CONTEXTPUSH(tmpfound->foundmode) )
			contextstackcount++;
		DBG_SCANCACHE("in loop: remove Tfound %p from the cache and free, contextstackcount=%d, blockstackcount=%d\n",tmpfound,contextstackcount,blockstackcount);
		g_sequence_remove(tmpsiter);
		found_free_lcb(tmpfound, btv);
	}
}


static inline int found_match(BluefishTextView * btv, const Tmatch match, Tscanning *scanning)
{
	Tfoundblock *fblock=NULL;
	Tfoundcontext *fcontext=NULL;
	guint8 mode=0;
	gint retcontext=scanning->context;
	guint match_end_o;
	Tfound *found;
	Tpattern pat = g_array_index(btv->bflang->st->matches,Tpattern, match.patternum);
	DBG_SCANNING("found_match for pattern %d %s at charoffset %d, starts_block=%d,ends_block=%d, nextcontext=%d (current=%d)\n",match.patternum,pat.pattern, gtk_text_iter_get_offset(&match.start),pat.starts_block,pat.ends_block,pat.nextcontext,scanning->context);
/*	DBG_MSG("pattern no. %d (%s) matches (%d:%d) --> nextcontext=%d\n", match.patternum, scantable.matches[match.patternum].message,
			gtk_text_iter_get_offset(&match.start), gtk_text_iter_get_offset(&match.end), scantable.matches[match.patternum].nextcontext);*/
#ifdef IDENTSTORING
	scanning->identmode = pat.identmode;
#endif /* IDENTSTORING */

	match_end_o = gtk_text_iter_get_offset(&match.end);

	if (pat.selftag) {
		DBG_SCANNING("found_match, apply tag %p from %d to %d\n",pat.selftag,gtk_text_iter_get_offset(&match.start),gtk_text_iter_get_offset(&match.end));
		gtk_text_buffer_apply_tag(btv->buffer,pat.selftag, &match.start, &match.end);
	}
	if (!pat.starts_block && !pat.ends_block && (pat.nextcontext == 0 || pat.nextcontext == scanning->context))
		return scanning->context;
	
	/* There are three situations comparing the current scan to the cached results:
	1: the cache entry has an offset lower than the current offset or equal but a different patternum and
		is thus not valid anymore. That means that the region that needs scanning should be enlarged up 
		to the fcontext or fblock end from the cache.
	2: the cache entry has the same offset and the same patternum and is thus valid, we only highlight and 
		store nothing in the cache
	3: the cache entry has a higher offset -> do nothing with the cached one 
	*/
	if (scanning->nextfound) {
		/* TODO: this should be a loop, if we remove an entry because our current entry > the removed entry, 
		perhaps we have to remove the next entry too ??? */
		if (scanning->nextfound->charoffset_o > match_end_o) {
			DBG_SCANNING("next item in the cache (offset %d) is not relevant yet (offset now %d)\n",scanning->nextfound->charoffset_o, match_end_o);
		} else if (scanning->nextfound->charoffset_o == match_end_o && cached_found_is_valid(btv, &match, scanning)){
			DBG_SCANNING("cache item at offset %d and mode %d is still valid\n",scanning->nextfound->charoffset_o, scanning->nextfound->foundmode);
			return scanning->nextfound->fcontext->context;
		} else {
			remove_invalid_cache(btv, &match, scanning);
		}
	}
	
	if (pat.starts_block) {
		fblock = found_start_of_block(btv, match, scanning);
		SET_FOUNDMODE_BLOCKPUSH(mode);
	} else if (pat.ends_block) {
		fblock = found_end_of_block(btv, match, scanning, &pat);
		SET_FOUNDMODE_BLOCKPOP(mode);
	}

	if (pat.nextcontext != 0 && pat.nextcontext != scanning->context) {
		fcontext = found_context_change(btv, match, scanning, &pat);
		if (pat.nextcontext < 0) {
			SET_FOUNDMODE_CONTEXTPOP(mode);
		} else {
			SET_FOUNDMODE_CONTEXTPUSH(mode);			
		}
		retcontext = (fcontext ? fcontext->context : 1);
	}
	found = g_slice_new0(Tfound);
#ifdef HL_PROFILING
	hl_profiling.found_refcount++;
#endif
	found->foundmode = mode;
	found->fblock = scanning->curfblock;
	found->fcontext = scanning->curfcontext;
	found->charoffset_o = match_end_o;
	DBG_SCANCACHE("found_match, put found %p in the cache at charoffset_o %d with mode %d fblock %p fcontext %p\n",found,found->charoffset_o,found->foundmode, found->fblock,found->fcontext);
	g_sequence_insert_sorted(btv->scancache.foundcaches,found,foundcache_compare_charoffset_o,NULL);
	return retcontext;
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
		scanning->curfound = found;
		scanning->curfblock = found->fblock;
		scanning->curfcontext = found->fcontext;
		if (scanning->curfcontext) {
			scanning->context = scanning->curfcontext->context;
		} else {
			scanning->context = 1;
		}
		scanning->nextfound = get_foundcache_next(btv, &scanning->siter);
		DBG_SCANNING("reconstruct_stack, found at offset %d, curfblock=%p, curfcontext=%p, context=%d\n", found->charoffset_o, scanning->curfblock, scanning->curfcontext, scanning->context);
		return found->charoffset_o;
	} else {
		DBG_SCANNING("nothing to reconstruct\n");
		scanning->curfcontext = NULL;
		scanning->curfblock = NULL;
		scanning->context = 1;
		scanning->curfound=NULL;
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
static void remove_all_highlighting_in_area(BluefishTextView *btv, GtkTextIter *start, GtkTextIter *end) {
	GList *tmplist = g_list_first(btv->bflang->tags);
	while (tmplist) {
		gtk_text_buffer_remove_tag(btv->buffer, (GtkTextTag *)tmplist->data, start, end);
		tmplist = g_list_next(tmplist);
	}
}

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
	GtkTextIter start, end, iter, orig_end;
	GtkTextIter mstart;
	/*GArray *matchstack;*/
	Tscanning scanning;
	guint pos = 0, newpos, reconstruction_o;
	gboolean normal_run=TRUE, last_character_run=FALSE;
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

	if (!bftextview2_find_region2scan(btv, btv->buffer, &start, &end)) {
		DBG_MSG("nothing to scan here.. return FALSE\n");
#ifdef VALGRIND_PROFILING
		CALLGRIND_STOP_INSTRUMENTATION;
#endif /* VALGRIND_PROFILING */
		return FALSE;
	}
	/* start timer */
	scanning.timer = g_timer_new();

	orig_end = end;
	if (visible_end) {
		/* make sure that we only scan up to visible_end and no further */
		if (gtk_text_iter_compare(&start,visible_end)>0) {
			DBG_DELAYSCANNING("start of region that needs scanning is beyond visible_end, return TRUE\n");
			g_timer_destroy(scanning.timer);
#ifdef VALGRIND_PROFILING
			CALLGRIND_STOP_INSTRUMENTATION;
#endif /* VALGRIND_PROFILING */
			return TRUE;
		}
		if (gtk_text_iter_compare(&end,visible_end)>0) {
			DBG_DELAYSCANNING("end of region that needs scanning (%d) is beyond visible_end (%d), reset end\n",gtk_text_iter_get_offset(&end),gtk_text_iter_get_offset(visible_end));
			end = *visible_end;
		}
	}
#ifdef HL_PROFILING
	startpos = gtk_text_iter_get_offset(&start);
	stage1 = g_timer_elapsed(scanning.timer,NULL);
#endif
	iter = mstart = start;
	if (gtk_text_iter_is_start(&start)) {
		scanning.siter = g_sequence_get_begin_iter(btv->scancache.foundcaches);
		scanning.curfound = NULL;
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
		gtk_text_iter_set_line_offset(&start,0);
		DBG_SCANNING("compare possible start positions %d and %d\n",gtk_text_iter_get_offset(&start),gtk_text_iter_get_offset(&iter));
		if (gtk_text_iter_compare(&iter,&start)>0)
			mstart = start = iter;
		else
			iter = mstart = start;
	}
	if (!gtk_text_iter_is_end(&end)) {
		/* the end position should be the largest of the end of the line and the 'end' iter */
		gtk_text_iter_forward_to_line_end(&iter);
		gtk_text_iter_forward_char(&iter);
		if (gtk_text_iter_compare(&iter,&end)>=0)
			end = iter;
		iter = start;
	}
	DBG_SCANNING("scanning from %d to %d\n",gtk_text_iter_get_offset(&start),gtk_text_iter_get_offset(&end));
	btv->scancache.valid_cache_offset = gtk_text_iter_get_offset(&start);
#ifdef HL_PROFILING
	stage2= g_timer_elapsed(scanning.timer,NULL);
#endif
	if (btv->needremovetags) {
		remove_all_highlighting_in_area(btv, &start, &end);
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
		DBG_SCANNING("scanning %d %c in pos %d..",gtk_text_iter_get_offset(&iter),uc,pos);
		newpos = g_array_index(btv->bflang->st->table, Ttablerow, pos).row[uc];
		DBG_SCANNING(" got newpos %d\n",newpos);
		if (G_UNLIKELY(newpos == 0 || uc == '\0')) {
			if (G_UNLIKELY(g_array_index(btv->bflang->st->table,Ttablerow, pos).match)) {
				Tmatch match;
				match.patternum = g_array_index(btv->bflang->st->table,Ttablerow, pos).match;
				match.start = mstart;
				match.end = iter;
				DBG_SCANNING("we have a match from pos %d to %d\n", gtk_text_iter_get_offset(&match.start),gtk_text_iter_get_offset(&match.end));
				scanning.context = found_match(btv, match,&scanning);
				DBG_SCANNING("after match context=%d\n",scanning.context);
			}
#ifdef IDENTSTORING
			else if (G_UNLIKELY(scanning.identmode != 0 && pos == g_array_index(btv->bflang->st->contexts,Tcontext,scanning.context).identstate)) {
				/* ignore if the cursor is within the range, because it could be that the user is still typing the name */
				if (G_LIKELY(!gtk_text_iter_in_range(&itcursor, &mstart, &iter) 
								&& !gtk_text_iter_equal(&itcursor, &mstart) 
								&& !gtk_text_iter_equal(&itcursor, &iter))) {
					found_identifier(btv, &mstart, &iter, scanning.context, scanning.identmode);
					scanning.identmode = 0;
				}
			}
#endif /* IDENTSTORING */
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
		normal_run = !gtk_text_iter_equal(&iter, &end);
		if (G_UNLIKELY(!normal_run)) {
			/* only if last_character_run is FALSE and normal_run is FALSE we set last_character run to TRUE */
			last_character_run = 1 - last_character_run;
		}
	} while ((normal_run || last_character_run) && (loop%loops_per_timer!=0 || g_timer_elapsed(scanning.timer,NULL)<MAX_CONTINUOUS_SCANNING_INTERVAL));
	DBG_SCANNING("scanned from %d to position %d, (end=%d, orig_end=%d) which took %f microseconds, loops_per_timer=%d\n",gtk_text_iter_get_offset(&start),gtk_text_iter_get_offset(&iter),gtk_text_iter_get_offset(&end),gtk_text_iter_get_offset(&orig_end),g_timer_elapsed(scanning.timer,NULL),loops_per_timer);
	gtk_text_buffer_remove_tag(btv->buffer, btv->needscanning, &start , &iter);
	
	/* because we do not yet have an algorithm to find out where our previous scanning runs are still valid
	we have to re-scan all the text up to the end */
	gtk_text_buffer_apply_tag(btv->buffer,btv->needscanning,&iter,&orig_end);
	/*g_array_free(matchstack,TRUE);*/
#ifdef HL_PROFILING
	stage4 = g_timer_elapsed(scanning.timer,NULL);
	hl_profiling.total_runs++;
	hl_profiling.total_marks += hl_profiling.num_marks;
	hl_profiling.total_chars += hl_profiling.numchars;
	hl_profiling.total_time_ms += (gint)(1000.0*stage4); 
	g_print("scanning run %d (%d ms): %d, %d, %d, %d; from %d-%d, loops=%d,chars=%d,blocks %d/%d (%d) contexts %d/%d (%d) scancache %d, marks %d\n"
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
		,hl_profiling.num_marks
		);
	g_print("memory scancache %d(%dKb+%dKb) found %d(%dKb) fcontext %d(%dKb)\n"
		,hl_profiling.found_refcount,(gint)(hl_profiling.found_refcount*sizeof(Tfound)/1024.0),(gint)(hl_profiling.found_refcount*40/1024.0)
		,hl_profiling.fblock_refcount,(gint)(hl_profiling.fblock_refcount*sizeof(Tfoundblock)/1024.0)
		,hl_profiling.fcontext_refcount,(gint)(hl_profiling.fcontext_refcount*sizeof(Tfoundcontext)/1024.0)
		);
	g_print("average %d chars/s %d chars/run, %d marks/s, %d marks/run\n"
			,(guint)(1000.0*hl_profiling.total_chars / hl_profiling.total_time_ms )
			,(guint)(1.0*hl_profiling.total_chars / hl_profiling.total_runs )
			,(guint)(1000.0*hl_profiling.total_marks / hl_profiling.total_time_ms )
			,(guint)(1.0*hl_profiling.total_marks / hl_profiling.total_runs )
			);
#endif
	/* tune the loops_per_timer, try to have 10 timer checks per loop, so we have around 10% deviation from the set interval */
	if (normal_run)
		loops_per_timer = MAX(loop/10,200);
	g_timer_destroy(scanning.timer);

#ifdef VALGRIND_PROFILING
	CALLGRIND_STOP_INSTRUMENTATION;
#endif /* VALGRIND_PROFILING */

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
		DBG_SCANNING("cleanup_scanner, no sit1, no cleanup ??\n");
	}
#ifdef HL_PROFILING
	g_print("cleanup_scanner, num_marks=%d, fblock_refcount=%d,fcontext_refcount=%d,found_refcount=%d\n",hl_profiling.num_marks,hl_profiling.fblock_refcount,hl_profiling.fcontext_refcount,hl_profiling.found_refcount);
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
