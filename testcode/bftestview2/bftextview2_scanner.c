#include "bftextview2.h"

typedef struct {
	unsigned int patternum;
	GtkTextIter start;
	GtkTextIter end;
} Tmatch;

typedef struct {
	GQueue *contextstack;
	GQueue *blockstack;
} Tscanning;

static void found_start_of_block(GtkTextBuffer *buffer, Tmatch match, Tscanning *scanning) {
	Tfoundblock *fblock;
	g_print("put on blockstack\n");
		
	fblock = g_slice_new0(Tfoundblock);
	fblock->start1 = gtk_text_buffer_create_mark(buffer,NULL,&match.start,FALSE);
	fblock->end1 = gtk_text_buffer_create_mark(buffer,NULL,&match.end,TRUE);
	fblock->patternum = match.patternum;
	g_object_set_data(fblock->start1, "block", fblock);
	g_object_set_data(fblock->end1, "block", fblock);
	g_queue_push_head(scanning->blockstack,fblock);
}

static void found_end_of_block(GtkTextBuffer *buffer, Tmatch match, Tscanning *scanning) {
	Tfoundblock *fblock=NULL;
	do {
		if (fblock) {
			gtk_text_buffer_delete_mark(buffer,fblock->start1);
			gtk_text_buffer_delete_mark(buffer,fblock->end1);
			g_slice_free(Tfoundblock,fblock);
		}
		g_print("pop from blockstack\n");
		fblock = g_queue_pop_head(scanning->blockstack);
	} while (fblock && fblock.patternnum == g_array_index(bt2->scantable.matches,Tpattern, match.patternum).blockstartpattern);
	if (fblock) {
		g_print("found the start of the block\n");
		fblock->start2 = gtk_text_buffer_create_mark(buffer,NULL,&match.start,FALSE);
		fblock->end2 = gtk_text_buffer_create_mark(buffer,NULL,&match.end,TRUE);
		g_object_set_data(fblock->start2, "block", fblock);
		g_object_set_data(fblock->end2, "block", fblock);
	}
	
}

static int found_match(Tbftextview2 * bt2, Tmatch match, Tscanning *scanning)
{
	GtkTextBuffer *buffer;
	buffer = gtk_text_view_get_buffer(bt2);

/*	g_print("pattern no. %d (%s) matches (%d:%d) --> nextcontext=%d\n", match.patternum, scantable.matches[match.patternum].message,
			gtk_text_iter_get_offset(&match.start), gtk_text_iter_get_offset(&match.end), scantable.matches[match.patternum].nextcontext);*/

	if (g_array_index(bt2->scantable.matches,Tpattern, match.patternum).selftag)
		gtk_text_buffer_apply_tag(buffer,scantable.matches[match.patternum].selftag, &match.start, &match.end);
	
	if (g_array_index(bt2->scantable.matches,Tpattern, match.patternum).starts_block) {
		found_start_of_block(buffer, match, scanning);
	}
	if (g_array_index(bt2->scantable.matches,Tpattern, match.patternum).ends_block) {
		found_end_of_block(buffer, match, scanning);
	}
	
	return scantable.matches[match.patternum].nextcontext;
}

static gboolean bftextview2_find_region2scan(GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end) {
	GtkTextTag *scantag;
	/* first find a region that needs scanning */
	scantag = langmrg_get_tag("needscanning");
	gtk_text_buffer_get_bounds(buffer, start, end);
	if (!gtk_text_iter_begins_tag(start,scantag) ) {
		if (!gtk_text_iter_forward_to_tag_toggle(start,scantag)) {
			/* nothing to scan */
			return FALSE;
		}
	}
	if (!gtk_text_iter_ends_tag(end,scantag)) {
		if (!gtk_text_iter_backward_to_tag_toggle(end,scantag)) {
			g_print("BUG: we should never get here\n");
			return FALSE;
		}
	}
	return TRUE;
}

gboolean bftextview2_run_scanner(Tbftextview2 * bt2)
{
	GtkTextBuffer *buffer;
	GtkTextIter start, end, iter;
	GtkTextIter mstart;
	GArray *matchstack;
	Tscanning scanning;
	unsigned int pos = 0, newpos;
	int context = 0;

	Tmatch matchstack[32];

	if (!bt2->scantable) {
		g_print("no scantable, nothing to scan, returning...\n");
		return;
	}
	
	buffer = gtk_text_view_get_buffer(bt2);
	
	if (!bftextview2_find_region2scan(buffer, &start, &end)) {
		g_print("nothing to scan here.. move along\n");
		return;
	}
	/* start timer */
	g_timer_start(bt2->timer);
	

	iter = mstart = start;
	g_print("got bounds from %d to %d\n",gtk_text_iter_get_offset(&start),gtk_text_iter_get_offset(&end));
	
	
	if (gtk_text_iter_is_start(&start)) {
		contextstack = g_queue_new();
		blockstack = g_queue_new();
	} else {
		/* reconstruct the context stack and the block stack */
	
	}
	matchstack = g_array_sized_new(FALSE,TRUE,sizeof(Tmatch),10);
	do {
		gunichar uc;

		uc = gtk_text_iter_get_char(&iter);
		if (uc > 128) {
			newpos = 0;
		} else {
			/*g_print("scanning %c\n",uc); */
			newpos = g_array_index(bt2->scantable.table, Ttablerow, pos).row[uc];
		}
		if (bt2->scantable.table[newpos].match != 0) {
			Tmatch match;
			g_print("possible match at matchstackpos=%d\n",matchstackpos);
			match.patternum = g_array_index(bt2->scantable.table,Ttablerow, newpos).match;
			match.start = mstart;
			match.end = iter;
			gtk_text_iter_forward_char(&match.end);
			g_array_append_val(matchstack,match);
		}
		if (newpos == 0 || uc == '\0') {
			if (matchstack->len > 0) {
				g_print("we have a match at matchstackpos=%d\n",matchstack->len-1);
				context = found_match(bt2, g_array_index(matchstack,Tmatch,matchstack->len - 1),&scanning);
				matchstack->len = 0;
			}
			newpos = bt2->scantable.contexttable[context];
			/*g_print("reset matchstack and mstart iter\n");*/
			mstart = iter;
			gtk_text_iter_forward_char(&mstart);
		}
		pos = newpos;
		gtk_text_iter_forward_char(&iter);
	} while (!gtk_text_iter_equal(&iter, &end) && g_timer_elapsed(bt2->timer,NULL)<0.002);
	g_print("scanned up to position %d, which took %f microseconds\n",gtk_text_iter_get_offset(&iter),g_timer_elapsed(bt2->timer,NULL));
	gtk_text_buffer_remove_tag_by_name(buffer,"needscanning",&start,&iter);
	if (!gtk_text_iter_equal(&iter, &end)) {
		return FALSE;
	}
	return TRUE;
}
