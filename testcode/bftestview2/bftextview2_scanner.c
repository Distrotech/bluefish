/* for the design docs see bftextview2.h */
#include "bftextview2_scanner.h"

typedef struct {
	unsigned int patternum;
	GtkTextIter start;
	GtkTextIter end;
} Tmatch;

typedef struct {
	GQueue *contextstack;
	GQueue *blockstack;
	gint context;
	GTimer *timer;
} Tscanning;

/* sort function for the stackcache GSequence structure */
static gint stackcache_compare_charoffset(gconstpointer a,gconstpointer b,gpointer user_data) {
	return ((Tfoundstack *)a)->charoffset - ((Tfoundstack *)b)->charoffset; 
}

Tfoundstack *get_stackcache_next(BluefishTextView * bt2, GSequenceIter ** siter) {
	*siter = g_sequence_iter_next(*siter);
	if (*siter && !g_sequence_iter_is_end(*siter)) {
		return g_sequence_get(*siter);
	}
	return NULL;
}

Tfoundstack *get_stackcache_at_position(BluefishTextView * bt2, GtkTextIter *position, GSequenceIter ** retsiter) {
	GSequenceIter* siter;
	Tfoundstack fakefstack;
	Tfoundstack *fstack=NULL;
	fakefstack.charoffset = gtk_text_iter_get_offset(position); 
	DBG_SCANCACHE("get_stackcache_at_position for position %d\n",fakefstack.charoffset);
	siter = g_sequence_search(bt2->scancache.stackcaches,&fakefstack,stackcache_compare_charoffset,NULL);
	if (!g_sequence_iter_is_begin(siter)) {
		
		/* now get the previous position, and get the stack at that position */
		DBG_SCANCACHE("search returned iter at position %d (cache length %d)\n",g_sequence_iter_get_position(siter),g_sequence_get_length(bt2->scancache.stackcaches));
		siter = g_sequence_iter_prev(siter);
		DBG_SCANCACHE("prev returned iter at position %d (cache length %d)\n",g_sequence_iter_get_position(siter),g_sequence_get_length(bt2->scancache.stackcaches));
		if (siter && !g_sequence_iter_is_end(siter)) {
			fstack = g_sequence_get(siter);
			if (retsiter)
				*retsiter = siter;
			DBG_SCANCACHE("found nearest stack %p with charoffset %d\n",fstack,fstack->charoffset);
		} else {
			DBG_SCANCACHE("no siter no stack\n");
		}
	}
	return fstack;
}

static void foundstack_update_positions(GtkTextBuffer *buffer, Tfoundstack *fstack) {
	GtkTextIter it1;
	if (fstack->mark) {
		gtk_text_buffer_get_iter_at_mark(buffer,&it1,fstack->mark);
		fstack->charoffset = gtk_text_iter_get_offset(&it1);
		fstack->line = gtk_text_iter_get_line(&it1);
	}
}

static void scancache_update_all_foreach(gpointer data, gpointer user_data) {
	foundstack_update_positions(GTK_TEXT_BUFFER(user_data), (Tfoundstack *)data);
} 

static void scancache_update_all_positions(BluefishTextView *bt2, GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end) {
	
	if (start && end) {
		GSequenceIter *siter;
		Tfoundstack *fstack;
		gint end_offset = gtk_text_iter_get_offset(end);
		fstack = get_stackcache_at_position(bt2, start, &siter);
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
		bt2->scancache.stackcache_need_update_charoffset = end_offset;
	} else {
		/* loop over all items in the scancache and update the offsets */
		g_sequence_foreach(bt2->scancache.stackcaches,scancache_update_all_foreach,buffer);
		bt2->scancache.stackcache_need_update_charoffset = -1;
	}
	DBG_MSG("done updating stackcaches offsets\n");
}
static void foundblock_unref(Tfoundblock *fblock, GtkTextBuffer *buffer) {
	fblock->refcount--;
	if (fblock->refcount == 0) {
		/* remove marks */
		DBG_SCANCACHE("UNREF->cleanup foundblock %p\n",fblock);
		gtk_text_buffer_delete_mark(buffer,fblock->start1);
		gtk_text_buffer_delete_mark(buffer,fblock->end1);
		if (fblock->start2 && fblock->end2) {
			gtk_text_buffer_delete_mark(buffer,fblock->start2);
			gtk_text_buffer_delete_mark(buffer,fblock->end2);
		}
		g_slice_free(Tfoundblock,fblock);
	}
}
static void foundcontext_unref(Tfoundcontext *fcontext, GtkTextBuffer *buffer) {
	fcontext->refcount--;
	DBG_REFCOUNT("unref: refcount for fcontext %p is %d\n",fcontext,fcontext->refcount);
	if (fcontext->refcount == 0) {
		/* remove marks */
		DBG_REFCOUNT("unref: cleanup foundcontext %p\n",fcontext);
		gtk_text_buffer_delete_mark(buffer,fcontext->start);
		gtk_text_buffer_delete_mark(buffer,fcontext->end);
		g_slice_free(Tfoundcontext,fcontext);
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
void foundstack_free_lcb(gpointer data, gpointer bt2) {
	Tfoundstack *fstack = data;
	/* unref all contexts and blocks */
	g_queue_foreach(fstack->blockstack,foundblock_foreach_unref_lcb,bt2);
	g_queue_foreach(fstack->contextstack,foundcontext_foreach_unref_lcb,bt2);
	g_slice_free(Tfoundstack,fstack);
}
static void foundcontext_foreach_ref_lcb(gpointer data,gpointer user_data) {
	if (data) {
		((Tfoundcontext *)data)->refcount++; 
		DBG_REFCOUNT("foreach ref: refcount for fcontext %p is %d\n",data,((Tfoundcontext *)data)->refcount);
	}
}
static void foundblock_foreach_ref_lcb(gpointer data,gpointer user_data) {
	if (data) {
		((Tfoundblock *)data)->refcount++;
	}
}
static void add_to_scancache(BluefishTextView * bt2,GtkTextBuffer *buffer,Tscanning *scanning, GtkTextMark *where) {
	Tfoundstack *fstack;

	fstack = g_slice_new0(Tfoundstack);
	fstack->contextstack = g_queue_copy(scanning->contextstack);
	fstack->blockstack = g_queue_copy(scanning->blockstack);
	g_queue_foreach(fstack->blockstack,foundblock_foreach_ref_lcb,NULL);
	g_queue_foreach(fstack->contextstack,foundcontext_foreach_ref_lcb,NULL);
		
	fstack->mark = where;
	foundstack_update_positions(buffer, fstack);
	DBG_SCANCACHE("add_to_scancache, put the stacks in the cache at charoffset %d / line %d\n",fstack->charoffset,fstack->line);
	g_sequence_insert_sorted(bt2->scancache.stackcaches,fstack,stackcache_compare_charoffset,NULL);
}

static GtkTextMark *found_start_of_block(BluefishTextView * bt2,GtkTextBuffer *buffer, Tmatch match, Tscanning *scanning) {
	Tfoundblock *fblock;
	DBG_MSG("put block with type %d on blockstack\n",match.patternum);
		
	fblock = g_slice_new0(Tfoundblock);
	fblock->start1 = gtk_text_buffer_create_mark(buffer,NULL,&match.start,FALSE);
	fblock->end1 = gtk_text_buffer_create_mark(buffer,NULL,&match.end,TRUE);
	fblock->patternum = match.patternum;
	g_object_set_data(G_OBJECT(fblock->start1), "block", fblock);
	g_object_set_data(G_OBJECT(fblock->end1), "block", fblock);
	g_queue_push_head(scanning->blockstack,fblock);
	fblock->refcount++;
	return fblock->end1;
}

static GtkTextMark *found_end_of_block(BluefishTextView * bt2,GtkTextBuffer *buffer, Tmatch match, Tscanning *scanning, Tpattern *pat) {
	Tfoundblock *fblock=NULL;
	DBG_MSG("found end of block that matches start of block pattern %d\n",pat->blockstartpattern); 
	do {
		if (fblock) {
			foundblock_unref(fblock, buffer);
		}
		fblock = g_queue_pop_head(scanning->blockstack);
		if (fblock) {
			DBG_MSG("popped block for pattern %d from blockstack\n",fblock->patternum);
		}
	} while (fblock && fblock->patternum != pat->blockstartpattern);
	if (fblock) {
		DBG_MSG("found the matching start of the block\n");
		fblock->start2 = gtk_text_buffer_create_mark(buffer,NULL,&match.start,FALSE);
		fblock->end2 = gtk_text_buffer_create_mark(buffer,NULL,&match.end,TRUE);
		g_object_set_data(G_OBJECT(fblock->start2), "block", fblock);
		g_object_set_data(G_OBJECT(fblock->end2), "block", fblock);
		if (pat->blocktag) {
			GtkTextIter iter;
			DBG_MSG("apply blocktag\n");
			gtk_text_buffer_get_iter_at_mark(buffer,&iter,fblock->end1);
			gtk_text_buffer_apply_tag(buffer,pat->blocktag, &iter, &match.start);
		}
		return fblock->end2;
	} else {
		DBG_MSG("no matching start-of-block found\n");
	}
	return NULL;
}

static GtkTextMark *found_context_change(BluefishTextView * bt2,GtkTextBuffer *buffer, Tmatch match, Tscanning *scanning, Tpattern *pat) {
	Tfoundcontext *fcontext;
	/* check if we change up or down the stack */
	if (pat->nextcontext < scanning->context) {
		/* pop */
		fcontext = g_queue_pop_head(scanning->contextstack);
		fcontext->end = gtk_text_buffer_create_mark(buffer,NULL,&match.start,FALSE);
		DBG_MSG("found_context_change, popped context %d from the stack, stack len %d\n",fcontext->context,g_queue_get_length(scanning->contextstack));
		if (g_array_index(bt2->scantable->contexts,Tcontext,fcontext->context).contexttag) {
			GtkTextIter iter;
			gtk_text_buffer_get_iter_at_mark(buffer,&iter,fcontext->start);
			gtk_text_buffer_apply_tag(buffer,g_array_index(bt2->scantable->contexts,Tcontext,fcontext->context).contexttag, &iter, &match.start);
		}
		foundcontext_unref(fcontext, buffer);
		return fcontext->end;
	} else {
		fcontext = g_slice_new0(Tfoundcontext);
		fcontext->start = gtk_text_buffer_create_mark(buffer,NULL,&match.end,FALSE);
		fcontext->context = pat->nextcontext;
		g_queue_push_head(scanning->contextstack, fcontext);
		fcontext->refcount++;
		DBG_REFCOUNT("refcount for fcontext %p is %d\n",fcontext,fcontext->refcount);
		DBG_MSG("found_context_change, pushed context %d onto the stack, stack len %d\n",pat->nextcontext,g_queue_get_length(scanning->contextstack));
		return fcontext->start;
	}
}

static int found_match(BluefishTextView * bt2, Tmatch match, Tscanning *scanning)
{
	GtkTextBuffer *buffer;
	GtkTextMark *where=NULL;
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(bt2));
	Tpattern pat = g_array_index(bt2->scantable->matches,Tpattern, match.patternum);
	DBG_SCANNING("found_match for pattern %d %s at charoffset %d\n",match.patternum,pat.message, gtk_text_iter_get_offset(&match.start));
/*	DBG_MSG("pattern no. %d (%s) matches (%d:%d) --> nextcontext=%d\n", match.patternum, scantable.matches[match.patternum].message,
			gtk_text_iter_get_offset(&match.start), gtk_text_iter_get_offset(&match.end), scantable.matches[match.patternum].nextcontext);*/

	if (pat.selftag)
		gtk_text_buffer_apply_tag(buffer,pat.selftag, &match.start, &match.end);
	DBG_MSG("found_match for pattern %d\n",match.patternum);
	if (pat.starts_block) {
		where = found_start_of_block(bt2, buffer, match, scanning);
	}
	if (pat.ends_block) {
		where = found_end_of_block(bt2, buffer, match, scanning, &pat);
	}

	if (pat.nextcontext != scanning->context) {
		where = found_context_change(bt2, buffer, match, scanning, &pat);
	}
	if (where) {
		add_to_scancache(bt2,buffer,scanning, where);
	}
	
	return pat.nextcontext;
}

static gboolean bftextview2_find_region2scan(GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end) {
	GtkTextTag *scantag;
	/* first find a region that needs scanning */
	scantag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(buffer),"needscanning");
	gtk_text_buffer_get_start_iter(buffer, start);
	if (!gtk_text_iter_begins_tag(start,scantag) ) {
		if (!gtk_text_iter_forward_to_tag_toggle(start,scantag)) {
			/* nothing to scan */
			return FALSE;
		}
	}
	/* find the end of the region */
	*end = *start;
	gtk_text_iter_forward_char(end);
	if (!gtk_text_iter_ends_tag(end,scantag)) {
		if (!gtk_text_iter_forward_to_tag_toggle(end,scantag)) {
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

static void reconstruct_stack(BluefishTextView * bt2, GtkTextBuffer *buffer, GtkTextIter *position, Tscanning *scanning) {
	Tfoundstack *fstack=NULL;
	fstack = get_stackcache_at_position(bt2,position,NULL);
	if (fstack) {
		Tfoundcontext *fcontext;
		scanning->contextstack = g_queue_copy(fstack->contextstack);
		fcontext = g_queue_peek_head(scanning->contextstack);
		if (fcontext)
			scanning->context = fcontext->context;
		scanning->blockstack = g_queue_copy(fstack->blockstack);

		g_queue_foreach(scanning->blockstack,foundblock_foreach_ref_lcb,NULL);
		g_queue_foreach(scanning->contextstack,foundcontext_foreach_ref_lcb,NULL);
		DBG_MSG("stack from the cache, contextstack has len %d, blockstack has len %d, context=%d\n",g_queue_get_length(scanning->contextstack),g_queue_get_length(scanning->blockstack),scanning->context);
	} else {
		DBG_MSG("empty stack\n");
		scanning->contextstack =  g_queue_new();
		scanning->blockstack =  g_queue_new();
		
	}
}
#ifdef OLD
/* returns a context number and changes 'position' such that it points to the position
where this context was found */
guint16 get_context_and_startposition(BluefishTextView * bt2, GtkTextIter *position) {
	Tfoundstack *fstack;
	fstack = get_stackcache_at_position(bt2,position,NULL);
	if (fstack) {
		Tfoundcontext *fcontext;
		fcontext = g_queue_peek_head(fstack->contextstack);
		if (fcontext) {
			DBG_MSG("found context %d\n",fcontext->context);
			gtk_text_buffer_get_iter_at_mark(gtk_text_view_get_buffer(bt2),position,fstack->mark);
			return fcontext->context;
		} else 
			DBG_MSG("no context on stack, return context 0\n");
	} else 
		DBG_MSG("no stack, no context, return context 0\n");
	return 0;
}
#endif
guint16 get_context_at_position(BluefishTextView * bt2, GtkTextIter *position) {
	Tfoundstack *fstack;
	fstack = get_stackcache_at_position(bt2,position,NULL);
	if (fstack) {
		Tfoundcontext *fcontext;
		fcontext = g_queue_peek_head(fstack->contextstack);
		if (fcontext) {
			DBG_MSG("found context %d\n",fcontext->context);
			return fcontext->context;
		} else 
			DBG_MSG("no context on stack, return context 0\n");
	} else 
		DBG_MSG("no stack, no context, return context 0\n");
	return 0;
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
		g_print("no sit1, no cleanup ??\n");
	}
}

gboolean bftextview2_run_scanner(BluefishTextView * bt2)
{
	GtkTextBuffer *buffer;
	GtkTextIter start, end, iter;
	GtkTextIter mstart;
	/*GArray *matchstack;*/
	Tscanning scanning;
	guint pos = 0, newpos;
	
	scanning.context = 0;

	if (!bt2->scantable) {
		DBG_MSG("no scantable, nothing to scan, returning...\n");
		return FALSE;
	}
	
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(bt2));
	
	if (!bftextview2_find_region2scan(buffer, &start, &end)) {
		DBG_MSG("nothing to scan here.. update the offsets in the stackcache\n");
		scancache_update_all_positions(bt2,buffer,NULL,NULL);
		/* after the offsets have been updated there is really nothing to do for
		the idle function so we return FALSE */
		return FALSE;
	}
	/* start timer */
	scanning.timer = g_timer_new();
		
	
	g_print("scanning from %d to %d\n",gtk_text_iter_get_offset(&start),gtk_text_iter_get_offset(&end));

	if (bt2->scancache.stackcache_need_update_charoffset != -1 && bt2->scancache.stackcache_need_update_charoffset <= gtk_text_iter_get_offset(&end)) {
		DBG_MSG("need to update the scancache offsets for the region we are about to scan\n");
		gtk_text_buffer_get_iter_at_offset(buffer, &iter, bt2->scancache.stackcache_need_update_charoffset);
		scancache_update_all_positions(bt2, buffer, &iter, &end); 
	}
	iter = mstart = start;
	if (gtk_text_iter_is_start(&start)) {
		scanning.contextstack = g_queue_new();
		scanning.blockstack = g_queue_new();
		/*siter = g_sequence_iter_first(bt2->scancache.stackcaches);*/
	} else {
		/* reconstruct the context stack and the block stack */
		reconstruct_stack(bt2, buffer, &iter, &scanning);
		pos = g_array_index(bt2->scantable->contexts,Tcontext,scanning.context).startstate;
	}
	/*matchstack = g_array_sized_new(FALSE,TRUE,sizeof(Tmatch),10);*/
	/* TODO: when rescanning text that has been scanned before we need to remove 
	invalid tags and blocks. right now we remove all, but most are likely 
	still valid */
	remove_old_scan_results(bt2, buffer, &start);
	/* because we remove all to the end we have to rescan to the end (I know this 
	is stupid, should become smarter in the future )*/
	gtk_text_iter_forward_to_end(&end);
	do {
		gunichar uc;

		uc = gtk_text_iter_get_char(&iter);
		if (uc > 128) {
			newpos = 0;
		} else {
			DBG_SCANNING("scanning %d %c in pos %d..",gtk_text_iter_get_offset(&iter),uc,pos); 
			newpos = g_array_index(bt2->scantable->table, Ttablerow, pos).row[uc];
			DBG_SCANNING(" got newpos %d\n",newpos);
		}
		if (newpos == 0 || uc == '\0') {
			if (g_array_index(bt2->scantable->table,Ttablerow, pos).match) {
				Tmatch match;
				match.patternum = g_array_index(bt2->scantable->table,Ttablerow, pos).match;
				match.start = mstart;
				match.end = iter;
				DBG_SCANNING("we have a match from pos %d to %d\n", gtk_text_iter_get_offset(&match.start),gtk_text_iter_get_offset(&match.end));
				scanning.context = found_match(bt2, match,&scanning);
			}
			if (gtk_text_iter_equal(&mstart,&iter)) {
				gtk_text_iter_forward_char(&iter);
			}
			mstart = iter;
			newpos = g_array_index(bt2->scantable->contexts,Tcontext,scanning.context).startstate;		
		} else {
			gtk_text_iter_forward_char(&iter);
		}
		pos = newpos;
	} while (!gtk_text_iter_equal(&iter, &end) && g_timer_elapsed(scanning.timer,NULL)<MAX_CONTINUOUS_SCANNING_INTERVAL);
	DBG_MSG("scanned up to position %d, which took %f microseconds\n",gtk_text_iter_get_offset(&iter),g_timer_elapsed(scanning.timer,NULL));
	gtk_text_buffer_apply_tag_by_name(buffer,"needscanning",&iter,&end);

	g_timer_destroy(scanning.timer);
	/*g_array_free(matchstack,TRUE);*/

	return TRUE; /* even if we finished scanning the next call should update the scancache */
}
#ifdef OLD
/* this function starts at start, scans up to iter, and the location of the 
possible match start will be in 'start'. */
void scan_for_prefix_start(BluefishTextView *btv, guint16 contextnum, GtkTextIter *start, GtkTextIter *cursor) {
	guint16 pos,newpos;
	GtkTextIter iter = *start;
	pos = g_array_index(btv->scantable->contexts,Tcontext, contextnum).startstate;
	DBG_AUTOCOMP("scanning for prefix from %d to %d\n",gtk_text_iter_get_offset(&iter),gtk_text_iter_get_offset(cursor));
	while (!gtk_text_iter_equal(&iter, cursor)) {
		gunichar uc;

		uc = gtk_text_iter_get_char(&iter);
		
		if (uc > 128) {
			newpos = 0;
		} else {
			DBG_AUTOCOMP("scanning at position %d char %c\n",gtk_text_iter_get_offset(&iter),uc);
			newpos = g_array_index(btv->scantable->table, Ttablerow, pos).row[uc];
		}
		if (newpos == 0 || uc == '\0') {
			if (g_array_index(btv->scantable->table,Ttablerow, pos).match) {
				contextnum = g_array_index(btv->scantable->matches,Tpattern, g_array_index(btv->scantable->table,Ttablerow, pos).match).nextcontext;
			}
			if (gtk_text_iter_equal(start,&iter)) {
				gtk_text_iter_forward_char(&iter);
			}
			*start = iter;
			newpos = g_array_index(btv->scantable->contexts,Tcontext, contextnum).startstate;
		} else {
			gtk_text_iter_forward_char(&iter);
		}
		pos = newpos;
	}
}
#endif
void scan_for_autocomp_prefix(BluefishTextView *btv,GtkTextIter *mstart,GtkTextIter *cursorpos,guint16 *contextnum) {
	GtkTextIter iter;
	guint16 pos,newpos;
	/* get the current context */
	iter = *mstart;
	*contextnum = get_context_at_position(btv, &iter);
	pos = g_array_index(btv->scantable->contexts,Tcontext, *contextnum).startstate;
	while (!gtk_text_iter_equal(&iter, cursorpos)) {
		gunichar uc;
		uc = gtk_text_iter_get_char(&iter);
		if (uc > 128) {
			newpos = 0;
		} else {
			newpos = g_array_index(btv->scantable->table, Ttablerow, pos).row[uc];
		}
		if (newpos == 0 || uc == '\0') {
			if (g_array_index(btv->scantable->table,Ttablerow, pos).match) {
				*contextnum = g_array_index(btv->scantable->matches,Tpattern, g_array_index(btv->scantable->table,Ttablerow, pos).match).nextcontext;
			}
			if (gtk_text_iter_equal(mstart,&iter)) {
				gtk_text_iter_forward_char(&iter);
			}
			*mstart = iter;
			newpos = g_array_index(btv->scantable->contexts,Tcontext, *contextnum).startstate;
		} else {
			gtk_text_iter_forward_char(&iter);
		}
		pos = newpos;		
	}
}
