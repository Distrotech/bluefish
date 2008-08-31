#include "bftextview2.h"

typedef struct {
	unsigned int patternum;
	GtkTextIter start;
	GtkTextIter end;
} Tmatch;


static int found_match(Tbftextview2 * bt2, Tmatch match)
{
	GtkTextBuffer *buffer;

	buffer = gtk_text_view_get_buffer(bt2);

	g_print("pattern no. %d (%s) matches (%d:%d) --> nextcontext=%d\n", match.patternum, scantable.matches[match.patternum].message,
			gtk_text_iter_get_offset(&match.start), gtk_text_iter_get_offset(&match.end), scantable.matches[match.patternum].nextcontext);

	if (bt2->scantable.matches[match.patternum].selftag)
		gtk_text_buffer_apply_tag(buffer,scantable.matches[match.patternum].selftag, &match.start, &match.end);
		
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
	GQueue *contextstack;
	GQueue *blockstack;
	GArray *matchstack;

	unsigned int pos = 0, newpos, matchstackpos=0;
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
			matchstackpos++;
		}
		if (newpos == 0 || uc == '\0') {
			if (matchstackpos > 0) {
				g_print("we have a match at matchstackpos=%d\n",matchstackpos-1);
				context = found_match(bt2, g_array_index(matchstack,Tmatch,matchstackpos - 1));
				matchstackpos = 0;
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
