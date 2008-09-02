/* for the design docs see bftextview2.h */
#include "bftextview2.h"
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
} Tscanning;

static void foundstack_update_positions(GtkTextBuffer *buffer, Tfoundstack *fstack) {
	GtkTextIter it1;
	if (fstack->mark) {
		gtk_text_buffer_get_iter_at_mark(buffer,&it1,fstack->mark);
		fstack->charoffset = gtk_text_iter_get_offset(&it1);
		fstack->line = gtk_text_iter_get_line(&it1);
	}
}

/* sort function for the stackcache GSequence structure */
static gint stackcache_compare_charoffset(gconstpointer a,gconstpointer b,gpointer user_data) {
	return ((Tfoundstack *)a)->charoffset - ((Tfoundstack *)b)->charoffset; 
}

static void add_to_scancache(Tbftextview2 * bt2,GtkTextBuffer *buffer,Tscanning *scanning, GtkTextMark *where) {
	Tfoundstack *fstack;

	fstack = g_slice_new0(Tfoundstack);
	fstack->contextstack = g_queue_copy(scanning->contextstack);
	fstack->blockstack = g_queue_copy(scanning->blockstack);
	fstack->mark = where;
	foundstack_update_positions(buffer, fstack);
	g_print("add_to_scancache, put the stacks in the cache\n");
	g_sequence_insert_sorted(bt2->scancache.stackcaches,fstack,stackcache_compare_charoffset,NULL);
}

static GtkTextMark *found_start_of_block(Tbftextview2 * bt2,GtkTextBuffer *buffer, Tmatch match, Tscanning *scanning) {
	Tfoundblock *fblock;
	g_print("put block with type %d on blockstack\n",match.patternum);
		
	fblock = g_slice_new0(Tfoundblock);
	fblock->start1 = gtk_text_buffer_create_mark(buffer,NULL,&match.start,FALSE);
	fblock->end1 = gtk_text_buffer_create_mark(buffer,NULL,&match.end,TRUE);
	fblock->patternum = match.patternum;
	g_object_set_data(G_OBJECT(fblock->start1), "block", fblock);
	g_object_set_data(G_OBJECT(fblock->end1), "block", fblock);
	g_queue_push_head(scanning->blockstack,fblock);
	return fblock->end1;
}

static GtkTextMark *found_end_of_block(Tbftextview2 * bt2,GtkTextBuffer *buffer, Tmatch match, Tscanning *scanning, Tpattern *pat) {
	Tfoundblock *fblock=NULL;
	g_print("found end of block that matches start of block pattern %d\n",pat->blockstartpattern); 
	do {
/*		since a pointer to the Tfoundblock is *also* in the cache, we should add a refcount for memory-management
		or something like that */ 
/*		if (fblock) {
			gtk_text_buffer_delete_mark(buffer,fblock->start1);
			gtk_text_buffer_delete_mark(buffer,fblock->end1);
			g_slice_free(Tfoundblock,fblock);
		}*/
		fblock = g_queue_pop_head(scanning->blockstack);
		if (fblock)
			g_print("popped block for pattern %d from blockstack\n",fblock->patternum);
	} while (fblock && fblock->patternum != pat->blockstartpattern);
	if (fblock) {
		g_print("found the matching start of the block\n");
		fblock->start2 = gtk_text_buffer_create_mark(buffer,NULL,&match.start,FALSE);
		fblock->end2 = gtk_text_buffer_create_mark(buffer,NULL,&match.end,TRUE);
		g_object_set_data(G_OBJECT(fblock->start2), "block", fblock);
		g_object_set_data(G_OBJECT(fblock->end2), "block", fblock);
		if (pat->blocktag) {
			GtkTextIter iter;
			g_print("apply blocktag\n");
			gtk_text_buffer_get_iter_at_mark(buffer,&iter,fblock->end1);
			gtk_text_buffer_apply_tag(buffer,pat->blocktag, &iter, &match.start);
		}
		return fblock->end2;
	} else {
		g_print("no matching start-of-block found\n");
	}
	return NULL;
}

static GtkTextMark *found_context_change(Tbftextview2 * bt2,GtkTextBuffer *buffer, Tmatch match, Tscanning *scanning, Tpattern *pat) {
	Tfoundcontext *fcontext;
	/* check if we change up or down the stack */
	if (pat->nextcontext < scanning->context) {
		/* pop */
		fcontext = g_queue_pop_head(scanning->contextstack);
		fcontext->end = gtk_text_buffer_create_mark(buffer,NULL,&match.start,FALSE);
		g_print("found_context_change, popped context %d from the stack, stack len %d\n",fcontext->context,g_queue_get_length(scanning->contextstack));
		return fcontext->end;
	} else {
		fcontext = g_slice_new0(Tfoundcontext);
		fcontext->start = gtk_text_buffer_create_mark(buffer,NULL,&match.end,FALSE);
		fcontext->context = pat->nextcontext;
		g_queue_push_head(scanning->contextstack, fcontext);
		g_print("found_context_change, pushed context %d onto the stack, stack len %d\n",pat->nextcontext,g_queue_get_length(scanning->contextstack));
		return fcontext->start;
	}
}

static int found_match(Tbftextview2 * bt2, Tmatch match, Tscanning *scanning)
{
	GtkTextBuffer *buffer;
	GtkTextMark *where;
	gboolean changesstack=FALSE;
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(bt2));
	Tpattern pat = g_array_index(bt2->scantable->matches,Tpattern, match.patternum);
	g_print("found_match for pattern %d %s at charoffset %d\n",match.patternum,pat.message, gtk_text_iter_get_offset(&match.start));
/*	g_print("pattern no. %d (%s) matches (%d:%d) --> nextcontext=%d\n", match.patternum, scantable.matches[match.patternum].message,
			gtk_text_iter_get_offset(&match.start), gtk_text_iter_get_offset(&match.end), scantable.matches[match.patternum].nextcontext);*/

	if (pat.selftag)
		gtk_text_buffer_apply_tag(buffer,pat.selftag, &match.start, &match.end);
	g_print("found_match for pattern %d\n",match.patternum);
	if (pat.starts_block) {
		where = found_start_of_block(bt2, buffer, match, scanning);
		changesstack = TRUE;
	}
	if (pat.ends_block) {
		where = found_end_of_block(bt2, buffer, match, scanning, &pat);
		changesstack = TRUE;
	}

	if (pat.nextcontext != scanning->context) {
		where = found_context_change(bt2, buffer, match, scanning, &pat);
		changesstack = TRUE;
	}
	if (changesstack) {
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
			g_print("BUG: we should never get here\n");
			return FALSE;
		}
	}
	/* now move start to the beginning of the line and end to the end of the line */
	gtk_text_iter_set_line_offset(start,0);
	gtk_text_iter_forward_to_line_end(end);
	gtk_text_iter_forward_char(end);
	return TRUE;
}

void reconstruct_stack(Tbftextview2 * bt2, GtkTextBuffer *buffer, GtkTextIter *position, Tscanning *scanning) {
	GSequenceIter* siter;
	Tfoundstack fakefstack;
	Tfoundstack *fstack=NULL;
	fakefstack.charoffset = gtk_text_iter_get_offset(position); 
	g_print("reconstruct_block_stack for position %d\n",fakefstack.charoffset);
	siter = g_sequence_search(bt2->scancache.stackcaches,&fakefstack,stackcache_compare_charoffset,NULL);
	if (!g_sequence_iter_is_begin(siter)) {
		
		/* now get the previous position, and get the stack at that position */
		g_print("search returned iter at position %d (cache length %d)\n",g_sequence_iter_get_position(siter),g_sequence_get_length(bt2->scancache.stackcaches));
		siter = g_sequence_iter_prev(siter);
		g_print("prev returned iter at position %d (cache length %d)\n",g_sequence_iter_get_position(siter),g_sequence_get_length(bt2->scancache.stackcaches));
		if (siter && !g_sequence_iter_is_end(siter)) {
			fstack = g_sequence_get(siter);
			g_print("found a stack with charoffset %d\n",fstack->charoffset);
		} else {
			fstack = NULL;
			g_print("no siter no stack\n");
		}
	}
	if (fstack) {
		Tfoundcontext *fcontext;
		scanning->contextstack = g_queue_copy(fstack->contextstack);
		fcontext = g_queue_peek_head(scanning->contextstack);
		if (fcontext)
			scanning->context = fcontext->context;
		scanning->blockstack =  g_queue_copy(fstack->blockstack);
		g_print("stack from the cache, contextstack has len %d, blockstack has len %d, context=%d\n",g_queue_get_length(scanning->contextstack),g_queue_get_length(scanning->blockstack),scanning->context);
	} else {
		g_print("empty stack\n");
		scanning->contextstack =  g_queue_new();
		scanning->blockstack =  g_queue_new();
		
	}
}

gboolean bftextview2_run_scanner(Tbftextview2 * bt2)
{
	GtkTextBuffer *buffer;
	GtkTextIter start, end, iter;
	GtkTextIter mstart;
	GArray *matchstack;
	Tscanning scanning;
	guint pos = 0, newpos;
	
	scanning.context = 0;

	if (!bt2->scantable) {
		g_print("no scantable, nothing to scan, returning...\n");
		return FALSE;
	}
	
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(bt2));
	
	if (!bftextview2_find_region2scan(buffer, &start, &end)) {
		g_print("nothing to scan here.. move along\n");
		return FALSE;
	}
	/* start timer */
	g_timer_start(bt2->timer);
	

	iter = mstart = start;
	g_print("got bounds from %d to %d\n",gtk_text_iter_get_offset(&start),gtk_text_iter_get_offset(&end));
	
	if (gtk_text_iter_is_start(&start)) {
		scanning.contextstack = g_queue_new();
		scanning.blockstack = g_queue_new();
	} else {
		/* reconstruct the context stack and the block stack */
		reconstruct_stack(bt2, buffer, &iter, &scanning);
		pos = g_array_index(bt2->scantable->contexts,gint,scanning.context);
	}
	matchstack = g_array_sized_new(FALSE,TRUE,sizeof(Tmatch),10);
	do {
		gunichar uc;

		uc = gtk_text_iter_get_char(&iter);
		if (uc > 128) {
			newpos = 0;
		} else {
			/*g_print("scanning %c in pos %d..",uc,pos);*/ 
			newpos = g_array_index(bt2->scantable->table, Ttablerow, pos).row[uc];
			/*g_print(" got newpos %d\n",newpos);*/
		}
		if (g_array_index(bt2->scantable->table, Ttablerow, newpos).match != 0) {
			Tmatch match;
			match.patternum = g_array_index(bt2->scantable->table,Ttablerow, newpos).match;
			match.start = mstart;
			match.end = iter;
			gtk_text_iter_forward_char(&match.end);
			g_array_append_val(matchstack,match);
		}
		if (newpos == 0 || uc == '\0') {
			if (matchstack->len > 0) {
				g_print("we have a match at matchstackpos=%d\n",matchstack->len-1);
				scanning.context = found_match(bt2, g_array_index(matchstack,Tmatch,matchstack->len - 1),&scanning);
				matchstack->len = 0;
				
				/* we found a match, so the pattern is finished, so 
				the current character was not part of that pattern, and thus
				the current character might be the start of the next pattern.
				we have to reverse the iter one char  */
				gtk_text_iter_backward_char(&iter);
			}
			newpos = g_array_index(bt2->scantable->contexts,gint,scanning.context);
			/*g_print("reset matchstack and mstart iter\n");*/
			mstart = iter;
			gtk_text_iter_forward_char(&mstart);
		}
		pos = newpos;
		gtk_text_iter_forward_char(&iter);
	} while (!gtk_text_iter_equal(&iter, &end) && g_timer_elapsed(bt2->timer,NULL)<0.1);
	g_print("scanned up to position %d, which took %f microseconds\n",gtk_text_iter_get_offset(&iter),g_timer_elapsed(bt2->timer,NULL));
	gtk_text_buffer_remove_tag_by_name(buffer,"needscanning",&start,&iter);
	if (!gtk_text_iter_equal(&iter, &end)) {
		return TRUE; /* need again scanning */
	}
	return FALSE; /* finished scanning */
}
