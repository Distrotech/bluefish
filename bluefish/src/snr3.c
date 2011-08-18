/* Bluefish HTML Editor
 * snr3.c - search and replace
 *
 * Copyright (C) 2011 Olivier Sessink
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
 *

functional design:
- we should have both previous and next
- search should immediately fire, while still entering the text
- search should highlight all results, not only the first
- only on 'activate' (enter) or 'previous' or 'next' we should scroll

technical design:
- search is implemented in an idle loop, such that it will not the block the GUI too much
- the result is a GQueue with items that just have a start and end offset   

- that means that we have to adjust offsets if the buffer changes
- so we need to keep track of any documents that have search results 

- TODO: store the complete set of options in the search history, not only the text strings

- TODO: use a separate highlighting color for different dialogs ???? and then clean the corresponding 
texttag from the tag table??

*/

#define DEBUG
/*#define SNR3_PROFILING*/

#define _GNU_SOURCE

#include <gtk/gtk.h>
#include <string.h>
#include "config.h"
#include "bluefish.h"
#include "document.h"
#include "undo_redo.h"
#include "bf_lib.h"
#include "bookmark.h"
#include "bfwin.h"
#include "stringlist.h"
#include "dialog_utils.h"
#include "gtk_easy.h"
#include "bf_lib.h"
#include "snr3.h"
#include "snr3_files.h"

#ifdef SNR3_PROFILING

typedef struct {
	GTimer *timer;
	gint numresults;
} Tsnr3profiling;

Tsnr3profiling s3profiling= {NULL};

#endif

typedef struct {
	Tdocument *doc;
	Tsnr3run *s3run;
	gboolean update; /* if TRUE, update the current results instead of adding new results */
	guint32 so;
	guint32 eo;
} Truninidle;


static void
move_window_away_from_cursor(Tdocument * doc, GtkWindow * win, GtkTextIter * iter)
{
	GdkRectangle winrect;
	GdkRectangle itrect;

	/* get window coordinates, try to include the decorations */
	gdk_window_get_frame_extents(gtk_widget_get_window(GTK_WIDGET(win)), &winrect);
	doc_get_iter_location(doc, iter, &itrect);
	DEBUG_MSG("move_window_away_from_cursor, itx=%d-%d,ity=%d-%d, winx=%d-%d, winy=%d-%d\n", itrect.x,
			  itrect.x + itrect.width, itrect.y, itrect.y + itrect.height, winrect.x,
			  winrect.x + winrect.width, winrect.y, winrect.y + winrect.height);
	if (itrect.x + itrect.width > winrect.x && itrect.x < winrect.x + winrect.width
		&& itrect.y + itrect.height > winrect.y && itrect.y < winrect.y + winrect.height) {
		if (itrect.y > winrect.height + 48) {	/* the 48 is there to avoid crashing into a top-menu-bar */
			DEBUG_MSG("move_window_away_from_cursor, move window up to %d,%d\n", winrect.x,
					  itrect.y - winrect.height);
			gtk_window_move(win, winrect.x, itrect.y - winrect.height - 10);	/* add pixels 10 spacing */
		} else {
			DEBUG_MSG("move_window_away_from_cursor, move window down to %d,%d\n", winrect.x,
					  itrect.y + itrect.height);
			gtk_window_move(win, winrect.x, itrect.y + itrect.height + 10);	/* add pixels 10 spqacing */
		}
	}
}

static void scroll_to_result(Tsnr3result *s3result, GtkWindow *dialog) {
	GtkTextIter itstart, itend;
	DEBUG_MSG("scroll_to_result, started for s3result %p, dialog=%p\n",s3result, dialog);
	if (BFWIN(DOCUMENT(s3result->doc)->bfwin)->current_document != s3result->doc) {
		DEBUG_MSG("scroll_to_result, switch to document %p\n",s3result->doc);
		bfwin_switch_to_document_by_pointer(DOCUMENT(s3result->doc)->bfwin,s3result->doc);
	}
	
	gtk_text_buffer_get_iter_at_offset(DOCUMENT(s3result->doc)->buffer, &itstart, s3result->so);
	gtk_text_buffer_get_iter_at_offset(DOCUMENT(s3result->doc)->buffer, &itend, s3result->eo);
	gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(DOCUMENT(s3result->doc)->view), &itstart, 0.25, FALSE, 0.5, 0.10);
	gtk_text_buffer_select_range(DOCUMENT(s3result->doc)->buffer,&itstart,&itend);
	
	if (dialog)
		move_window_away_from_cursor(s3result->doc, dialog, &itstart);
	DEBUG_MSG("scroll_to_result, finished for s3result %p\n",s3result);
}

static void
remove_all_highlights_in_doc(Tdocument * doc)
{
	GtkTextTagTable *tagtable;
	GtkTextTag *tag;
	GtkTextIter itstart, itend;

	if (!doc)
		return;
		
	tagtable = gtk_text_buffer_get_tag_table(doc->buffer);
	tag = gtk_text_tag_table_lookup(tagtable, "snr3match");
	if (!tag)
		return;
	
	gtk_text_buffer_get_bounds(doc->buffer, &itstart, &itend);
	gtk_text_buffer_remove_tag(doc->buffer, tag, &itstart, &itend);
}

static void highlight_result(Tsnr3result *s3result) {
	GtkTextIter itstart, itend;
	static const gchar *tagname = "snr3match";
	static GtkTextTag *tag=NULL;
	
	gtk_text_buffer_get_iter_at_offset(DOCUMENT(s3result->doc)->buffer, &itstart, s3result->so);
	gtk_text_buffer_get_iter_at_offset(DOCUMENT(s3result->doc)->buffer, &itend, s3result->eo);

	if (!tag) {
		tag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(DOCUMENT(s3result->doc)->buffer), tagname);
		if (!tag) {
			tag =
				gtk_text_buffer_create_tag(DOCUMENT(s3result->doc)->buffer, tagname, "background", "#FFFF57", "foreground", "#000000",
										   NULL);
		}
	}
	gtk_text_buffer_apply_tag(DOCUMENT(s3result->doc)->buffer, tag, &itstart, &itend);
}

static void
highlight_run_in_doc(Tsnr3run *s3run, Tdocument *doc)
{
	GList *tmplist;
	
	DEBUG_MSG("s3run=%p, highlight all results that have doc %p\n",s3run, doc);
	for (tmplist=s3run->results.head;tmplist;tmplist=g_list_next(tmplist)) {
		Tsnr3result *s3result = tmplist->data;
		if (s3result->doc == doc) {
			highlight_result(s3result);
		}
	}
	DEBUG_MSG("highlight_run_in_doc, finished\n");
}

typedef struct {
	Tdocument *doc;
	guint startingpoint;
	gint offset;
} Toffsetupdate;

static void 
snr3run_update_offsets(Tsnr3run *s3run, Tdocument *doc, guint startpos, gint offset) {
	GList *tmplist = g_list_first(s3run->results.head);
	gint comparepos = (offset > 0) ? startpos : startpos - offset; 
	DEBUG_MSG("snr3run_update_offsets, startpos=%d, offset=%d, comparepos=%d\n",startpos,offset,comparepos);
	while (tmplist) {
		if (doc == ((Tsnr3result *)tmplist->data)->doc 
						&& ((Tsnr3result *)tmplist->data)->eo >= startpos) {
			DEBUG_MSG("evaluate so=%d, eo=%d\n",((Tsnr3result *)tmplist->data)->so,((Tsnr3result *)tmplist->data)->eo);
			if (((Tsnr3result *)tmplist->data)->so >= comparepos) {
				((Tsnr3result *)tmplist->data)->so += offset;
				((Tsnr3result *)tmplist->data)->eo += offset;
				DEBUG_MSG("new so=%d, eo=%d\n",((Tsnr3result *)tmplist->data)->so,((Tsnr3result *)tmplist->data)->eo);
			} else {
				GList *tmplist2;
				DEBUG_MSG("so <= startpos, and eo >= startpos !! delete the result!!\n");
				tmplist2 = tmplist;
				tmplist = g_list_next(tmplist);
				if (tmplist2 == s3run->current)
					s3run->current = tmplist;
				g_slice_free(Tsnr3result, tmplist2->data);
				g_queue_delete_link(&s3run->results, tmplist2);
				continue;
			}
		}
		tmplist = g_list_next(tmplist);
	}
}

static gchar * 
retrieve_pcre_replace_string(Tsnr3run *s3run, Tsnr3result *s3result, GMatchInfo *matchinfo)
{
	GError *gerror=NULL;
	gchar *newstr=NULL;
	gchar *buf = NULL;
	if (!matchinfo) {
		buf =doc_get_chars(s3result->doc, s3result->so-10, s3result->eo+10);
		DEBUG_MSG("retrieve_pcre_replace_string, search in %s\n",buf);
		g_regex_match_full(s3run->regex,buf,-1, 10,0, &matchinfo, &gerror);
		if (gerror) {
			g_print("retrieve_pcre_replace_string, error %s\n",gerror->message);
			g_error_free(gerror);
			g_match_info_free(matchinfo);
			matchinfo=NULL;
		}
	}
	if (matchinfo) {
		newstr = g_match_info_expand_references(matchinfo, s3run->replace, &gerror);
		if (gerror) {
			g_print("retrieve_pcre_replace_string, error %s\n",gerror->message);
			g_error_free(gerror);
		} else {
			DEBUG_MSG("retrieve_pcre_replace_string, return %s\n",newstr);
		}
	}
	if (buf ) {
		g_free(buf);
		if (matchinfo) {
			/* only free matchinfo if we have a buf and created the matcinfo ourselves */
			g_match_info_free(matchinfo);
		}
	}
	return newstr;
}

static Toffsetupdate 
s3result_replace(Tsnr3run *s3run, Tsnr3result *s3result, GMatchInfo *matchinfo)
{
	Toffsetupdate offsetupdate = {NULL,0,0};
	s3run->in_replace=TRUE;
	if (s3run->replacetype == snr3replace_string) {
		if (s3run->type == snr3type_string) {
			DEBUG_MSG("s3result_replace, replace %d:%d with %s\n", s3result->so, s3result->eo, s3run->replace);
			doc_replace_text_backend(s3result->doc, s3run->replace, s3result->so, s3result->eo);
			offsetupdate.offset = g_utf8_strlen(s3run->replace, -1)-(s3result->eo - s3result->so);
		} else if (s3run->type == snr3type_pcre) {
			gchar *newstr = retrieve_pcre_replace_string(s3run, s3result, matchinfo);
			if (newstr) {
				DEBUG_MSG("s3result_replace, replace from  %d:%d\n",s3result->so,s3result->eo);
				doc_replace_text_backend(s3result->doc, newstr, s3result->so, s3result->eo);
				DEBUG_MSG("s3result_replace, old len=%d, new len=%d, change offset with %d\n",(s3result->eo - s3result->so), (gint)g_utf8_strlen(newstr, -1), (gint)g_utf8_strlen(newstr, -1)-(s3result->eo - s3result->so));
				offsetupdate.offset = g_utf8_strlen(newstr, -1)-(s3result->eo - s3result->so);
				g_free(newstr);
			}
		}
	} else {
		gchar *tmp1, *tmp2;
		tmp1 = doc_get_chars(s3result->doc, s3result->so, s3result->eo);
		if (s3run->replacetype == snr3replace_upper) 
			tmp2 = g_utf8_strup(tmp1, -1);
		else
			tmp2 = g_utf8_strdown(tmp1, -1);
		doc_replace_text_backend(s3result->doc, tmp2, s3result->so, s3result->eo);
		g_free(tmp1);
		g_free(tmp2);
	}
	offsetupdate.startingpoint = s3result->eo;
	offsetupdate.doc = s3result->doc;
	s3run->in_replace=FALSE;
	return offsetupdate;
}


static void snr3result_free(gpointer s3result, gpointer s3run) {
	DEBUG_MSG("free result %p\n",s3result);
	g_slice_free(Tsnr3result, (Tsnr3result *)s3result);
}


static void
s3run_replace_current(Tsnr3run *s3run)
{
	Toffsetupdate offsetupdate;
	Tsnr3result *s3result=NULL;
	GList *next=NULL, *current=NULL;
	DEBUG_MSG("s3run_replace_current, s3run->current=%p\n",s3run->current);
	if (s3run->current) {
		current = s3run->current;
		next = s3run->current->next;
	}
	if (!current)
		return;
	s3result = current->data;
	offsetupdate = s3result_replace(s3run, s3result, NULL);

	snr3result_free(s3result, s3run);
	DEBUG_MSG("s3run_replace_current, the result is free'ed, now delete the link %p\n",current);
	g_queue_delete_link(&s3run->results, current);
	s3run->current = next;
	
	if (s3run->current) {
		scroll_to_result(s3run->current->data, GTK_WINDOW(((TSNRWin *)s3run->dialog)->dialog));
	}
	if (offsetupdate.offset != 0) {
		/* now re-calculate all the offsets in the results lists!!!!!!!!!!! */
		snr3run_update_offsets(s3run, offsetupdate.doc, offsetupdate.startingpoint, offsetupdate.offset);
	}
}

static Tsnr3result * sn3run_add_result(Tsnr3run *s3run, gulong so, gulong eo, gpointer doc) {
	Tsnr3result *s3result;
	s3result = g_slice_new(Tsnr3result);
	s3result->so = so;
	s3result->eo = eo;
	s3result->doc = doc;
	g_queue_push_tail(&s3run->results, s3result);
	return s3result;
}

static gboolean
backend_pcre_loop(Tsnr3run *s3run, gboolean indefinitely) {
	static guint loops_per_timer = 10;
	gint loop=0;
	GMatchInfo *match_info = NULL;
	GError *gerror = NULL;
	gboolean cont;
	/* reconstruct where we are searching */
	DEBUG_MSG("backend_pcre_loop, reconstruct scanning at curbuf %p curposition %d (byte %d)\n",s3run->curbuf, s3run->curposition, utf8_charoffset_to_byteoffset_cached(s3run->curbuf, s3run->curposition));
	cont = g_regex_match_full(s3run->regex, s3run->curbuf, -1, utf8_charoffset_to_byteoffset_cached(s3run->curbuf, s3run->curposition), G_REGEX_MATCH_NEWLINE_ANY, &match_info, &gerror);
	if (gerror) {
		g_warning("regex matching error: %s\n",gerror->message);
		g_error_free(gerror);
		gerror=NULL;
	}
	GTimer *timer = g_timer_new();
	while (cont && (indefinitely || loop % loops_per_timer != 0
				 || g_timer_elapsed(timer, NULL) < MAX_CONTINUOUS_SEARCH_INTERVAL)) {
		gint bso, beo, so, eo;
		Tsnr3result *s3result;
		g_match_info_fetch_pos(match_info, 0, &bso, &beo);
		so = utf8_byteoffset_to_charsoffset_cached(s3run->curbuf, bso);
		eo = utf8_byteoffset_to_charsoffset_cached(s3run->curbuf, beo);
		s3result = sn3run_add_result(s3run, so+s3run->curoffset+s3run->so, eo+s3run->curoffset+s3run->so, s3run->curdoc);
		DBG_MSG("backend_pcre_loop, found result at bso %d, so %d, s3run->so=%d, s3run->curoffse=%d --> %s\n",bso,so,s3run->so,s3run->curoffset,s3result->so);
		if (s3run->replaceall) {
			DEBUG_MSG("backend_pcre_loop, found in buffer at %d:%d, replace at %d:%d (curoffset=%d, s3run->so=%d)\n", so, eo, s3result->so, s3result->eo,s3run->curoffset, s3run->so);
			Toffsetupdate offsetupdate = s3result_replace(s3run, s3result, match_info);
			s3run->curoffset += offsetupdate.offset;
			DEBUG_MSG("backend_pcre_loop, new offset %d\n",s3run->curoffset);
		}
		
		s3run->curposition = eo;
		
		cont = g_match_info_next (match_info, &gerror);
		if (gerror) {
			g_print("snr3_run_pcre_loop, match error %s\n",gerror->message);
			g_error_free(gerror);
			break;
		}
		loop++;
	}
	DEBUG_MSG("did %d loops in %f seconds\n",loop, g_timer_elapsed(timer, NULL));
	g_timer_destroy(timer);
	g_match_info_free(match_info);
	if (cont)
		loops_per_timer = (loops_per_timer + MAX(loop / 10, 10))/2;
	return cont;
}

static gboolean
backend_string_loop(Tsnr3run *s3run, gboolean indefinitely)
{
	GTimer *timer = g_timer_new();
	gint querylen;
	char *(*f) ();
	gint loop=0;
	static guint loops_per_timer=10;
	gchar *result;
	if (s3run->is_case_sens) {
		f = strstr;
	} else {
		f = strcasestr;
	}
	querylen = g_utf8_strlen(s3run->query, -1);
	/* now reconstruct the last scan offset */
	result = s3run->curbuf + utf8_charoffset_to_byteoffset_cached(s3run->curbuf, s3run->curposition);

	do {
		result = f(result, s3run->query);
		if (result) {
			Tsnr3result *s3result;
			glong char_o = utf8_byteoffset_to_charsoffset_cached(s3run->curbuf, (result-s3run->curbuf));
			DEBUG_MSG("snr3_run_string_loop, add result %d:%d, replaceall=%d\n", (gint)char_o+s3run->so, (gint)char_o+querylen+s3run->so, s3run->replaceall);
			s3result = sn3run_add_result(s3run, char_o+s3run->so+s3run->curoffset, char_o+querylen+s3run->so+s3run->curoffset, s3run->curdoc);
			if (s3run->replaceall) {
				DEBUG_MSG("snr3_run_string_loop, replace %d:%d\n", (gint)char_o+s3run->so, (gint)char_o+querylen+s3run->so);
				Toffsetupdate offsetupdate = s3result_replace(s3run, s3result, NULL);
				s3run->curoffset += offsetupdate.offset;
			}
			s3run->curposition = char_o+querylen+s3run->so;
			result++;
			loop++;
		}		
	} while (result && (indefinitely || loop % loops_per_timer != 0
				 || g_timer_elapsed(timer, NULL) < MAX_CONTINUOUS_SEARCH_INTERVAL));
	DEBUG_MSG("did %d loops in %f seconds\n",loop, g_timer_elapsed(timer, NULL));
	g_timer_destroy(timer);

	if (result) {
		loops_per_timer = (loops_per_timer + MAX(loop / 10, 10))/2;
		return TRUE;
	}
	return FALSE;
}

static gboolean
snr3_run_loop_idle_func(Truninidle *rii)
{
	Tsnr3run *s3run=rii->s3run;
	gboolean cont;
	DEBUG_MSG("snr3_run_loop_idle_func, next loop\n");
	if (s3run->type == snr3type_string)
		cont = backend_string_loop(s3run, FALSE);
	else 
		cont = backend_pcre_loop(s3run, FALSE);

	if (cont)		
		return TRUE;

	/* now see if this document added any results to the resultset, and increment the number of 
	documents with results if so */
	if (s3run->results.tail) {
		if (S3RESULT(s3run->results.tail->data)->doc == rii->doc)
			s3run->resultnumdoc++;
	}

	g_free(s3run->curbuf);
	s3run->curbuf=NULL;
	s3run->idle_id = 0;
	queue_worker_ready(&s3run->idlequeue);
	snr3run_unrun(s3run); /* unrun currently only calls the callback if the refcount is zero, we can run that from here too ??? */
	g_slice_free(Truninidle, rii);
	return FALSE;
}

void
snr3_queue_run(gpointer data) {
	Truninidle *rii=data;
	DEBUG_MSG("snr3_queue_run, s3run=%p, doc=%p\n",rii->s3run, rii->doc);

	if (rii->update) {
		GList *tmplist=rii->s3run->results.head;
		guint32 newso=0;
		/* we start not at the requested change offset, but we use the last valid search result end as start offset */
		while (tmplist) {
			if (rii->doc == ((Tsnr3result *)tmplist->data)->doc 
							&& ((Tsnr3result *)tmplist->data)->so >= rii->so) {
				GList *tmplist2;
				DEBUG_MSG("snr3_queue_run, so >= update so!! delete the result %d:%d!!\n",((Tsnr3result *)tmplist->data)->so, ((Tsnr3result *)tmplist->data)->eo);
				tmplist2 = tmplist;
				tmplist = g_list_next(tmplist);
				if (tmplist2 == rii->s3run->current)
					rii->s3run->current = g_list_previous(tmplist2);
				g_slice_free(Tsnr3result, tmplist2->data);
				g_queue_delete_link(&rii->s3run->results, tmplist2);
			} else {
				newso = ((Tsnr3result *)tmplist->data)->eo;
				tmplist = g_list_next(tmplist);
			}
		}
		rii->so = newso;
		DEBUG_MSG("snr3_queue_run, update starting at %d\n",newso);
	}

	rii->s3run->curdoc = rii->doc;
	rii->s3run->curoffset=0;
	rii->s3run->curposition=0;
	rii->s3run->curbuf = doc_get_chars(rii->doc, rii->so, rii->eo);
	rii->s3run->so = rii->so;
	rii->s3run->eo = rii->eo;
	DEBUG_MSG("snr3_queue_run, run doc %p, curbuf %p (%d:%d)\n",rii->doc, rii->s3run->curbuf, rii->so, rii->eo);
	utf8_offset_cache_reset();
	if (rii->s3run->replaceall) {
		if (rii->s3run->scope == snr3scope_alldocs || rii->s3run->scope == snr3scope_files) {
			doc_unre_new_group_action_id(rii->doc, rii->s3run->unre_action_id);
		} else {
			doc_unre_new_group(rii->doc);
		}
	}
	
	rii->s3run->idle_id = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,(GSourceFunc)snr3_run_loop_idle_func,rii,NULL);	
}

static void
update_callback(gpointer data)
{
	Tsnr3run *s3run=data;
	remove_all_highlights_in_doc(s3run->bfwin->current_document);
	highlight_run_in_doc(s3run, s3run->bfwin->current_document);
}

void
snr3_run_in_doc(Tsnr3run *s3run, Tdocument *doc, gint so, gint eo, gboolean update) {
	Truninidle *rii;
	
	rii = g_slice_new(Truninidle);
	rii->doc = doc;
	rii->s3run=s3run;
	g_atomic_int_inc(&s3run->runcount);
	rii->so = so;
	rii->eo = eo;
	rii->update = update;
	if (update) {
		rii->s3run->callback = update_callback;
	}
	
	DEBUG_MSG("push doc %p on the idlequeue, len=%d\n",doc,s3run->idlequeue.q.length);
	queue_push(&s3run->idlequeue, rii);
}

void
snr3_run_go(Tsnr3run *s3run, gboolean forward) {
	GList *next=NULL;
	DEBUG_MSG("snr3_run_go, s3run=%p\n",s3run);
	if (s3run->current) {
	DEBUG_MSG("snr3_run_go, s3run->current=%p\n",s3run->current);
		if (forward) {
			next = g_list_next(s3run->current);
		} else {
			next = g_list_previous(s3run->current);
		}
	} 
	
	if (!next) {
		DEBUG_MSG("no 'next' (current=%p)\n", s3run->current);
		next = forward ? s3run->results.head : s3run->results.tail;
		DEBUG_MSG("snr3_run_go, no next (current=%p), new next=%p\n",s3run->current,next);
	}
	
	if (!s3run->current) {
		GtkTextBuffer *buffer;
		GtkTextIter iter;
		GList *tmplist;
		gint cursorpos;
		/* use the cursor position to find the next or previous */
		buffer = BLUEFISH_TEXT_VIEW(s3run->bfwin->current_document->view)->buffer;
		gtk_text_buffer_get_iter_at_mark(buffer,&iter, gtk_text_buffer_get_insert(buffer));
		cursorpos = gtk_text_iter_get_offset(&iter);
		DEBUG_MSG("cursorpos at %d\n",cursorpos);
		for (tmplist=g_list_first(s3run->results.head);tmplist;tmplist=g_list_next(tmplist)) {
			if (((Tsnr3result *)tmplist->data)->doc == s3run->bfwin->current_document) {
				if ((((Tsnr3result *)tmplist->data)->so == cursorpos)) {
					next = tmplist;
					break;
				} else if ((((Tsnr3result *)tmplist->data)->so < cursorpos)) {
					if (!forward)
						next = tmplist;
				} else if ((((Tsnr3result *)tmplist->data)->so > cursorpos)) {
					if (forward)
						next = tmplist;
					break;
				}
			}
		}
	}
	DEBUG_MSG("scroll to result %p\n",next);
	if (next) {
		s3run->current = next;
		scroll_to_result(next->data, s3run->dialog ? GTK_WINDOW(((TSNRWin *)s3run->dialog)->dialog) : NULL);
	}
}

void
snr3_run(Tsnr3run *s3run, TSNRWin *snrwin, Tdocument *doc, void (*callback)(void *))
{
	gint so,eo;
	GList *tmplist;
	DEBUG_MSG("snr3_run, s3run=%p, scope=%d, query=%s\n",s3run, s3run->scope, s3run->query);

	if (s3run->query == NULL || s3run->query[0]=='\0') {
		DEBUG_MSG("no query (%p), no s3run\n",s3run->query);
		return;
	}
	s3run->callback = callback;
	if (snrwin) {
		if (s3run->replaceall)
			gtk_label_set_markup(GTK_LABEL(snrwin->searchfeedback),_("<i>Replace started</i>"));
		else
			gtk_label_set_markup(GTK_LABEL(snrwin->searchfeedback),_("<i>Search started</i>"));
		gtk_widget_show(snrwin->searchfeedback);
	}
	
	switch(s3run->scope) {
		case snr3scope_doc:
			snr3_run_in_doc(s3run, doc, 0, -1, FALSE);
		break;
		case snr3scope_cursor:
			so = doc_get_cursor_position(doc);
			snr3_run_in_doc(s3run, doc, so, -1, FALSE);
		break;
		case snr3scope_selection:
			if (doc_get_selection(doc, &so, &eo)) {
				snr3_run_in_doc(s3run, doc, so, eo, FALSE);
			} else {
				gtk_label_set_markup(GTK_LABEL(snrwin->searchfeedback),_("<span foreground=\"red\">No selection, aborted search</span>"));
			}
		break;
		case snr3scope_alldocs:
			for (tmplist=g_list_first(s3run->bfwin->documentlist);tmplist;tmplist=g_list_next(tmplist)) {
				snr3_run_in_doc(s3run, tmplist->data, 0, -1, FALSE);
			}
		break;
		case snr3scope_files:
			if (s3run->replaceall) {
				DEBUG_MSG("scope files, run with filepattern %s\n", s3run->filepattern);
				s3run->recursive = TRUE;
				snr3_run_in_files(s3run);
			}
		break;
	}
}

static void
snr3_cancel_run(Tsnr3run *s3run) {
	DEBUG_MSG("snr3_cancel_run, started for %p\n",s3run);
	if (s3run->idle_id) {
		DEBUG_MSG("remove idle_id %d\n", s3run->idle_id);
		g_source_remove(s3run->idle_id);
		/* TODO: BUG: MEMLEAK: the Truninidle is not free'ed now !?!?!?! */
		s3run->idle_id=0;
		s3run->curdoc=NULL;
	}
	if (s3run->changed_idle_id) {
		g_source_remove(s3run->changed_idle_id);
		s3run->changed_idle_id=0;
	}
	if (s3run->scope == snr3scope_files) {
		snr3_run_in_files_cancel(s3run);
	}
}

static void
snr3run_resultcleanup(Tsnr3run *s3run) 
{
	GList *tmplist;
	for (tmplist=g_list_first(s3run->results.head);tmplist;tmplist=g_list_next(tmplist)) {
		g_slice_free(Tsnr3result, tmplist->data);
	}
	g_queue_clear(&s3run->results);
	s3run->current=NULL;
	s3run->curposition=0;
	s3run->curoffset=0;
	s3run->resultnumdoc=0;
}

/* called from bfwin.c for simplesearch */
void
snr3run_free(Tsnr3run *s3run) {
	DEBUG_MSG("snr3run_free, started for %p\n",s3run);
	snr3_cancel_run(s3run);
	if (s3run->curbuf)
		g_free(s3run->curbuf);
	bfwin_current_document_change_remove_by_data(s3run->bfwin, s3run);
	bfwin_document_insert_text_remove_by_data(s3run->bfwin, s3run);
	bfwin_document_delete_range_remove_by_data(s3run->bfwin, s3run);
	bfwin_document_destroy_remove_by_data(s3run->bfwin, s3run);
	DEBUG_MSG("snr3run_free, query at %p\n",s3run->query);
	g_free(s3run->query);
	if (s3run->regex)
		g_regex_unref(s3run->regex);
	DEBUG_MSG("snr3run_free, replace\n");
	g_free(s3run->replace);
	g_free(s3run->filepattern);
	DEBUG_MSG("snr3run_free, basedir\n");
	if (s3run->basedir)
		g_object_unref(s3run->basedir);
	DEBUG_MSG("snr3run_free, remove all highlights\n");
	remove_all_highlights_in_doc(s3run->bfwin->current_document);
	DEBUG_MSG("snr3run_free, resultcleanup\n");
	snr3run_resultcleanup(s3run);
	g_slice_free(Tsnr3run, s3run);
}

void snr3run_bookmark_all(Tsnr3run *s3run) {
	GList *tmpl;
	
	for (tmpl=g_list_first(s3run->results.head);tmpl;tmpl=g_list_next(tmpl)) {
		Tsnr3result *s3result = tmpl->data;
		gchar *text = doc_get_chars(s3result->doc, s3result->so, s3result->eo);
		bmark_add_extern(s3result->doc, s3result->so, s3run->query, text, !main_v->globses.bookmarks_default_store);
		g_free(text);
	}
}

static void
replace_all_ready(void *data) {
	Tsnr3run *s3run=data;
	s3run->replaceall=FALSE;
	s3run->unre_action_id = 0;
	s3run->curdoc = NULL;
	if (s3run->dialog) {
		gchar *tmp;
		gtk_widget_set_sensitive(((TSNRWin *)s3run->dialog)->replaceButton, TRUE);
		gtk_widget_set_sensitive(((TSNRWin *)s3run->dialog)->findButton, TRUE);
		gtk_widget_set_sensitive(((TSNRWin *)s3run->dialog)->backButton, TRUE);
		gtk_widget_set_sensitive(((TSNRWin *)s3run->dialog)->replaceAllButton, TRUE);
		gtk_widget_set_sensitive(((TSNRWin *)s3run->dialog)->bookmarkButton, TRUE);
		tmp = g_strdup_printf(_("<i>Replaced %d entries</i>"), g_queue_get_length(&s3run->results));
		gtk_label_set_markup(GTK_LABEL(((TSNRWin *)s3run->dialog)->searchfeedback),tmp);
		g_free(tmp);
	}
}

static void
highlight_simple_search(void *data) {
	Tsnr3run *s3run=data;
	highlight_run_in_doc(s3run, s3run->bfwin->current_document);	
}


static void
snr3_curdocchanged_cb(Tbfwin *bfwin, Tdocument *olddoc, Tdocument *newdoc, gpointer data) {
	Tsnr3run *s3run=data;
	if (olddoc)
		remove_all_highlights_in_doc(olddoc);
	if (newdoc) {
		if (s3run->dialog) {
			highlight_run_in_doc(s3run, newdoc);
		} else { /* simple search */
			snr3run_resultcleanup(s3run);
			snr3_run(s3run, NULL, newdoc, highlight_simple_search);
		}
	}
}

static gboolean
changed_idle_cb(gpointer data)
{
	Truninidle *rii = data;
	DEBUG_MSG("startupdate_idle_cb\n");
	snr3_run_in_doc(rii->s3run, rii->doc, rii->so, rii->eo, rii->update);
	/*g_slice_free(Truninidle, rii); handled bythe idle function destroy */
	rii->s3run->changed_idle_id = 0;
	return FALSE;
}

static void
changed_destroy_cb(gpointer data)
{
	DEBUG_MSG("startupdate_destroy_cb\n");
	g_slice_free(Truninidle, data);
}

static void
handle_changed_in_snr3doc(Tsnr3run *s3run, Tdocument *doc, gint pos, gint len) {
	Toffsetupdate offsetupdate = {doc,pos,len};
	Truninidle *rii;
	if (s3run->in_replace) {
		return;
	}
	/* BUG: TODO: update s3run->so and s3run->eo also ! */
	
	DEBUG_MSG("handle_change_in_snr3doc, doc=%p, pos=%d, len=%d\n",doc,pos,len);
	snr3run_update_offsets(s3run, offsetupdate.doc, offsetupdate.startingpoint, offsetupdate.offset);
	
	/* notice that this function is called BEFORE the actual change in the document
	so we CANNOT call a function that will get a buffer from the textview widget or something like
	that, we should register an idle callback to do something like that */
	
	rii = g_slice_new(Truninidle);
	rii->s3run = s3run;
	rii->update = TRUE;
	rii->so = pos;
	if (s3run->curposition < pos) {
		rii->so = s3run->curposition;
	}
	rii->doc = doc;
	rii->eo = -1;
	/* TODO: BUG: what if the region is not all of the document, but just a part of it (a selection ?? */
	if (s3run->curdoc == doc) {
		snr3_cancel_run(s3run);
		if (s3run->callback != update_callback) {
			rii->update = FALSE;
			rii->eo = s3run->eo;
		}
		/* and we'll restart it again */
	}
	s3run->changed_idle_id = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE+40,changed_idle_cb,rii,changed_destroy_cb);
}

static void
snr3_docinsertext_cb(Tdocument *doc, const gchar *string, GtkTextIter * iter, gint pos, gint len, gint clen, gpointer data)
{
	handle_changed_in_snr3doc((Tsnr3run *)data, doc, pos, clen);
}

static void
snr3_docdeleterange_cb(Tdocument *doc, GtkTextIter * itstart, gint start, GtkTextIter * itend, gint end, const gchar *string, gpointer data)
{
	handle_changed_in_snr3doc((Tsnr3run *)data, doc, start, start-end);
}

static void
snr3_docdestroy_cb(Tdocument *doc, gpointer data)
{
	Tsnr3run *s3run = data;
	/* see if this is the current document of an ongoing search, if so, cancel the search */
	DEBUG_MSG("snr3_docdestroy_cb, doc=%p, s3run=%p\n",doc,s3run);
	if (s3run->curdoc == doc || s3run->scope == snr3scope_alldocs) {
		snr3_cancel_run(s3run);
	}
	/* remove any existing search results for this doc */
	GList *tmplist=g_list_first(s3run->results.head);
	while(tmplist) {
		GList *next = tmplist->next;
		Tsnr3result *s3result=tmplist->data;
		if(s3result->doc == doc) {
			snr3result_free(s3result, s3run);
			g_queue_delete_link(&s3run->results, tmplist);
		}
		tmplist = next;
	}
}

static Tsnr3run *
snr3run_new(Tbfwin *bfwin, gpointer dialog) 
{
	Tsnr3run *s3run;
	s3run = g_slice_new0(Tsnr3run);
	s3run->bfwin = bfwin;
	s3run->dialog = dialog;
	g_queue_init(&s3run->results);
	queue_init_full(&s3run->idlequeue, 1, FALSE, FALSE, snr3_queue_run);
	bfwin_current_document_change_register(bfwin, snr3_curdocchanged_cb, s3run);
	bfwin_document_insert_text_register(bfwin, snr3_docinsertext_cb, s3run);
	bfwin_document_delete_range_register(bfwin, snr3_docdeleterange_cb, s3run);
	bfwin_document_destroy_register(bfwin, snr3_docdestroy_cb, s3run);
	return s3run;
} 

void snr3run_unrun(Tsnr3run *s3run) {
	if (g_atomic_int_dec_and_test(&s3run->runcount)) {
		DEBUG_MSG("runcount 0, run the callback\n");
		s3run->callback(s3run);
		DEBUG_MSG("runcount 0, after the callback\n");
	}
}

static void
snr3run_multiset(Tsnr3run *s3run, 
				const gchar *query, const gchar *replace, 
				Tsnr3type type, Tsnr3replace replacetype,
				Tsnr3scope scope) {
	g_free(s3run->query);
	s3run->query = g_strdup(query);
	g_free(s3run->replace);
	s3run->replace = g_strdup(replace);
	DEBUG_MSG("snr3run_init, query=%s, replace=%s, type=%d, replacetype=%d, scope=%d\n",query, replace, type,replacetype, scope);
	s3run->type = type;
	s3run->replacetype = replacetype;
	s3run->scope = scope;
	/* TODO: potential memory leak below, what if there are results on the queue ? */
	g_queue_init(&s3run->results);
	
} 
/******************************************************/
/*********** start of simple search *******************/
/******************************************************/

static void
activate_simple_search(void *data) {
	Tsnr3run *s3run=data;
	DEBUG_MSG("activate_simple_search, s3run=%p\n", s3run);
	s3run->curdoc = NULL;
	highlight_run_in_doc(s3run, s3run->bfwin->current_document);
	snr3_run_go(s3run, TRUE);
}

gpointer simple_search_run(Tbfwin *bfwin, const gchar *string) {
	Tsnr3run *s3run;
	
	s3run = snr3run_new(bfwin, NULL);
	snr3run_multiset(s3run, string, NULL, snr3type_string,snr3replace_string,snr3scope_doc);
	DEBUG_MSG("simple_search_run, snr3run at %p, query at %p\n",s3run, s3run->query);
	snr3_run(s3run, NULL, bfwin->current_document, activate_simple_search);
	return s3run;
}

void
simple_search_next(Tbfwin *bfwin)
{
	if (bfwin->simplesearch_snr3run) {
		snr3_run_go(((Tsnr3run *)bfwin->simplesearch_snr3run), TRUE);
	}
}
/******************************************************/
/*********** end of simple search *********************/
/******************************************************/

static void
dialog_changed_run_ready_cb(gpointer data) {
	Tsnr3run *s3run=data;
	DEBUG_MSG("dialog_changed_run_ready_cb, finished with %d results\n",g_queue_get_length(&s3run->results));
	s3run->curdoc = NULL;
	highlight_run_in_doc(s3run, s3run->bfwin->current_document);
	if (s3run->dialog) {
		TSNRWin *snrwin = s3run->dialog;
		gchar *tmp;
		if (s3run->resultnumdoc > 1) {
			tmp = g_strdup_printf(_("<i>Found %d results in %d documents</i>"),g_queue_get_length(&s3run->results), s3run->resultnumdoc);
		} else {
			tmp = g_strdup_printf(_("<i>Found %d results</i>"),g_queue_get_length(&s3run->results));
		}
		gtk_label_set_markup(GTK_LABEL(snrwin->searchfeedback),tmp);
		gtk_widget_show(snrwin->searchfeedback);
		g_free(tmp);
	}
}

/***************************************************************************/
/***************************** GUI *****************************************/
/***************************************************************************/

static gboolean compile_regex(Tsnr3run *s3run) {
	GError *gerror = NULL;
	gint options = G_REGEX_MULTILINE;
	if (!s3run->is_case_sens)
		options |= G_REGEX_CASELESS;
	if (s3run->dotmatchall)
		options |= G_REGEX_DOTALL;
	
	s3run->regex = g_regex_new(s3run->query, options, G_REGEX_MATCH_NEWLINE_ANY, &gerror);
	if (gerror) {
		if (s3run->dialog) {
			gchar *message = g_markup_printf_escaped("<span foreground=\"red\">%s</span>", gerror->message);
			DEBUG_MSG("compile_regex, regex error %s\n",gerror->message);
			gtk_label_set_markup(GTK_LABEL(((TSNRWin *)s3run->dialog)->searchfeedback), message);
			gtk_widget_show(((TSNRWin *)s3run->dialog)->searchfeedback);
			g_free(message);
		}
		g_error_free(gerror);
		return FALSE;
	}
	DEBUG_MSG("compile_regex, return TRUE\n");
	return TRUE;
}

static gint update_snr3run(Tsnr3run *s3run) {
	if (s3run->regex) {
		g_regex_unref(s3run->regex);
		s3run->regex = NULL;
	}
	if (s3run->type == snr3type_pcre) {
		if (!compile_regex(s3run)) {
			g_free(s3run->query);
			s3run->query = NULL; /* mark query as unusable */
			return -1;
		}
	}
	return 1;
}

/*
returns -1 on error
returns a bitfield with:
 - on 0, nothing changed in the GUI
 - first bit: search options have changed
 - second bit: replace options have changed


*/
static gint
snr3run_init_from_gui(TSNRWin *snrwin, Tsnr3run *s3run)
{
	const gchar *query, *replace;
	gint type, replacetype, scope, dotmatchall, escapechars;
	gboolean is_case_sens;
	gint retval=0;
	GFile *basedir;
	const gchar *filepattern;
	DEBUG_MSG("s3run->in_replace=%d\n",s3run->in_replace);
	if (s3run->replaceall)
		return -1;
	
	query = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(snrwin->search))));
	replace = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(snrwin->replace))));
	type = gtk_combo_box_get_active(GTK_COMBO_BOX(snrwin->searchType));
	replacetype = gtk_combo_box_get_active(GTK_COMBO_BOX(snrwin->replaceType));
	scope = gtk_combo_box_get_active(GTK_COMBO_BOX(snrwin->scope));
	is_case_sens = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(snrwin->matchCase));
	dotmatchall = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(snrwin->dotmatchall));
	escapechars = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(snrwin->escapeChars));
	
	basedir = g_file_new_for_commandline_arg(gtk_entry_get_text(GTK_ENTRY(snrwin->basedir)));
	filepattern = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(snrwin->filepattern))));

	if (is_case_sens != s3run->is_case_sens) {
		snr3_cancel_run(s3run);
		DEBUG_MSG("set is_case_sens %d\n",is_case_sens);
		s3run->is_case_sens = is_case_sens;
		retval |= 1;
	}
	if (dotmatchall != s3run->dotmatchall) {
		snr3_cancel_run(s3run);
		s3run->dotmatchall = dotmatchall;
		retval |= 1;
	}
	if (replacetype != s3run->replacetype) {
		DEBUG_MSG("set replacetype %d\n",replacetype);
		s3run->replacetype = replacetype;
		retval |= 2;
	}
	if (scope != s3run->scope) {
		snr3_cancel_run(s3run);
		DEBUG_MSG("set scope %d\n",scope);
		s3run->scope = scope;
		retval |= 1;
	}
	if (type != s3run->type) {
		snr3_cancel_run(s3run);
		DEBUG_MSG("set type %d\n",type);
		s3run->type = type;
		retval |= 1;
	}

	if (g_strcmp0(s3run->query, query)!=0) {
		snr3_cancel_run(s3run);
		g_free(s3run->query);
		s3run->query = NULL;
		DEBUG_MSG("set query %s\n",query);
		s3run->query = g_strdup(query);
		retval |= 1;
	}
	
	if (g_strcmp0(s3run->replace, replace)!=0) {
		g_free(s3run->replace);
		DEBUG_MSG("set replace %s\n",replace);
		s3run->replace = g_strdup(replace);
		retval |= 2;
		DEBUG_MSG("retval=%d\n",retval);
	}
	
	if (g_strcmp0(s3run->filepattern, filepattern)!=0) {
		snr3_cancel_run(s3run);
		g_free(s3run->filepattern);
		s3run->filepattern = g_strdup(filepattern);
		DEBUG_MSG("filepattern =%s\n",filepattern);
		retval |= 1;
	}
	if (!s3run->basedir || !g_file_equal(s3run->basedir, basedir)) {
		snr3_cancel_run(s3run);
		if (!s3run->basedir)
			g_object_unref(s3run->basedir);
		s3run->basedir = basedir;
		retval |= 1;
	} else {
		g_object_unref(basedir);
	}
	
	if ((retval & 1) != 0) {
		if (update_snr3run(s3run)==-1)
			return -1;
		remove_all_highlights_in_doc(snrwin->bfwin->current_document);
		snr3run_resultcleanup(s3run);
	}
	/*gtk_widget_hide(snrwin->searchfeedback);*/
	
	if (retval != 0) {
		DEBUG_MSG("set session type %d, replacetype %d, scope %d\n",type,replacetype,scope);
		snrwin->bfwin->session->snr3_type = type;
		snrwin->bfwin->session->snr3_replacetype = replacetype;
		snrwin->bfwin->session->snr3_scope = scope;
		snrwin->bfwin->session->snr3_casesens = is_case_sens;
		snrwin->bfwin->session->snr3_escape_chars = escapechars;
		snrwin->bfwin->session->snr3_dotmatchall = dotmatchall;
	}
	
	DEBUG_MSG("snr3run_init_from_gui, return %d\n", retval);
	return retval;
}

static void
snrwin_guichange(TSNRWin *snrwin)
{
	gint guichange;
	if (!snrwin->s3run) {
		snrwin->s3run = snr3run_new(snrwin->bfwin, snrwin);
		DEBUG_MSG("snrwin_guichange, new s3run at %p\n",snrwin->s3run);
	}
	guichange = snr3run_init_from_gui(snrwin, snrwin->s3run);
	DEBUG_MSG("snrwin_guichange, guichange=%d\n",guichange);
	if (guichange != -1 && (guichange & 1) != 0) {
		DEBUG_MSG("snrwin_guichange, run snr3_run %p\n",snrwin->s3run);
		snr3_run(snrwin->s3run, snrwin, snrwin->s3run->bfwin->current_document, dialog_changed_run_ready_cb);
	}
}

static gboolean
snrwin_focus_out_event_cb(GtkWidget *widget,GdkEventFocus *event,gpointer data)
{
	snrwin_guichange((TSNRWin *)data);
	return FALSE;
}
static void
snr3_advanced_response(GtkDialog * dialog, gint response, TSNRWin * snrwin)
{
	Tsnr3run *s3run = snrwin->s3run;
	gint guichange;
	if (response == GTK_RESPONSE_CLOSE || response == GTK_RESPONSE_DELETE_EVENT) {
		/* freeing is done in the destroy callback */
		DEBUG_MSG("snr3_advanced_response, destroy widget\n");
		gtk_widget_destroy(GTK_WIDGET(dialog));
		return;
	}

	if (snrwin->s3run && snrwin->s3run->scope != snr3scope_files 
							&& snrwin->s3run->scope != snr3scope_alldocs
							&& snrwin->s3run->results.head
							&& S3RESULT(snrwin->s3run->results.head->data)->doc != snrwin->bfwin->current_document) {
		DEBUG_MSG("restart the search on the new active document\n");
		snr3run_resultcleanup(snrwin->s3run);
		snr3_run(snrwin->s3run, snrwin, snrwin->s3run->bfwin->current_document, dialog_changed_run_ready_cb);
		return;
	}

	if (!snrwin->s3run) {
		snrwin->s3run = snr3run_new(snrwin->bfwin, snrwin);
	}
	
	guichange = snr3run_init_from_gui(snrwin, snrwin->s3run);
	DEBUG_MSG("snr3_advanced_response, response=%d, guichange=%d\n",response,guichange);
	if (guichange == -1)
		return;

	snrwin->bfwin->session->searchlist = add_to_history_stringlist(snrwin->bfwin->session->searchlist, s3run->query, FALSE,TRUE);
	if (s3run->replace && s3run->replace[0] != '\0')
		snrwin->bfwin->session->replacelist = add_to_history_stringlist(snrwin->bfwin->session->replacelist, s3run->replace,FALSE, TRUE);
	if (s3run->scope == snr3scope_files && s3run->filepattern && s3run->filepattern[0] != '\0')
		snrwin->bfwin->session->filegloblist = add_to_history_stringlist(snrwin->bfwin->session->filegloblist, s3run->filepattern,FALSE, TRUE);
	
	switch(response) {
		case SNR_RESPONSE_FIND:
			if ((guichange & 1) != 0) {
				DEBUG_MSG("guichange=%d, call snr3_run\n",guichange);
				snr3_run(s3run, snrwin, s3run->bfwin->current_document, activate_simple_search);
			} else if (snrwin->s3run->results.length) {
				DEBUG_MSG("guichange=%d, call snr3_run_go\n",guichange);
				snr3_run_go(s3run, TRUE);
			}
		break;
		case SNR_RESPONSE_BACK:
			if ((guichange & 1) != 0) {
				DEBUG_MSG("guichange=%d, call snr3_run\n",guichange);
				snr3_run(snrwin->s3run, snrwin, snrwin->s3run->bfwin->current_document, activate_simple_search);
			} else if (snrwin->s3run->results.length) {
				DEBUG_MSG("guichange=%d, call snr3_run_go\n",guichange);
				snr3_run_go(s3run, FALSE);
			}		
		break;
		case SNR_RESPONSE_REPLACE:
			s3run_replace_current(snrwin->s3run);
			doc_unre_new_group(snrwin->bfwin->current_document);
			if ((guichange == 0) && s3run->current) {
				scroll_to_result(s3run->current->data, GTK_WINDOW(((TSNRWin *)s3run->dialog)->dialog));
			}
		break;
		case SNR_RESPONSE_BOOKMARK_ALL:
			snr3run_bookmark_all(snrwin->s3run);
		break;
		case SNR_RESPONSE_REPLACE_ALL:
			snr3_cancel_run(s3run);
			s3run->replaceall=TRUE;
			snr3run_resultcleanup(s3run);
			s3run->unre_action_id = new_unre_action_id();
			
			gtk_widget_set_sensitive(snrwin->replaceButton, FALSE);
			gtk_widget_set_sensitive(snrwin->findButton, FALSE);
			gtk_widget_set_sensitive(snrwin->backButton, FALSE);
			gtk_widget_set_sensitive(snrwin->replaceAllButton, FALSE);
			gtk_widget_set_sensitive(snrwin->bookmarkButton, FALSE);
			
			snr3_run(s3run, snrwin, s3run->bfwin->current_document, replace_all_ready);
		break;
	}

}

static inline void widget_set_show(GtkWidget *widget, gboolean show) {
	if (show)
		gtk_widget_show(widget);
	else
		gtk_widget_hide(widget);
}

static void snr_dialog_show_widgets(TSNRWin * snrwin) {
	Tsnr3type searchtype;
	Tsnr3scope scope;
	Tsnr3replace replacetype;
	
	scope = gtk_combo_box_get_active(GTK_COMBO_BOX(snrwin->scope));
	searchtype = gtk_combo_box_get_active(GTK_COMBO_BOX(snrwin->searchType));
	replacetype = gtk_combo_box_get_active(GTK_COMBO_BOX(snrwin->replaceType));

	DEBUG_MSG("snr_dialog_show_widgets, scope=%d, searchtype=%d, replacetype=%d\n",scope,searchtype,replacetype);

	widget_set_show(snrwin->filepattern, (scope == snr3scope_files));
	widget_set_show(snrwin->filepatternL, (scope == snr3scope_files));
	widget_set_show(snrwin->basedir, (scope == snr3scope_files));
	widget_set_show(snrwin->basedirL, (scope == snr3scope_files));
	widget_set_show(snrwin->basedirB, (scope == snr3scope_files));
	widget_set_show(snrwin->replaceType, (searchtype == snr3type_pcre));
	widget_set_show(snrwin->replaceTypeL, (searchtype == snr3type_pcre));
	widget_set_show(snrwin->replace, (searchtype != snr3type_pcre || replacetype == snr3replace_string));
	widget_set_show(snrwin->escapeChars, (searchtype == snr3type_string));
	widget_set_show(snrwin->dotmatchall, (searchtype == snr3type_pcre));
	
	widget_set_show(snrwin->replaceButton, (scope != snr3scope_files));
	widget_set_show(snrwin->backButton, (scope != snr3scope_files));
	widget_set_show(snrwin->findButton, (scope != snr3scope_files));
	widget_set_show(snrwin->bookmarkButton, (scope != snr3scope_files));
}

static void
snr_combobox_changed(GtkComboBox * combobox, TSNRWin * snrwin)
{
	DEBUG_MSG("snr_combobox_changed, combobox=%p\n",combobox);
	snr_dialog_show_widgets(snrwin);
	snrwin_guichange(snrwin);
}

static void
snr_combo_activate_lcb(GtkEntry * entry, gpointer data)
{
	snrwin_guichange((TSNRWin *) data);
}

static void
snr_option_toggled(GtkToggleButton * togglebutton, gpointer data)
{
	snrwin_guichange((TSNRWin *) data);
}

static void
snr3win_destroy_cb(GtkObject *object, gpointer user_data) 
{
	TSNRWin *snrwin=user_data;
	DEBUG_MSG("snr3win_destroy_cb, user_data=%p\n",user_data);
	snr3run_free(snrwin->s3run);
	g_slice_free(TSNRWin, snrwin);
}

static void 
snr3_advanced_dialog_backend(Tbfwin * bfwin, const gchar *findtext, Tsnr3scope s3scope)
{
	TSNRWin *snrwin;
	GtkWidget *table, *vbox, *entry;
	gint currentrow=0;
	GtkListStore *history, *lstore;
	GList *list, *tmplist;
	GtkTreeIter iter;
	/*GtkTextIter start, end; */
	unsigned int i = 0;

	const gchar *scope[] = {
		N_("Active document"),
		N_("Forward from cursor position"),
		N_("Current selection"),
		N_("All open documents"),
		N_("Files on disk")
	};

	const gchar *matchPattern[] = {
		N_("Normal"),
		N_("Regular expression (pcre)"),
	};

	const gchar *replaceType[] = {
		N_("Normal"),
		N_("Uppercase"),
		N_("Lowercase"),
	};

	snrwin = g_slice_new0(TSNRWin);
	snrwin->bfwin = bfwin;

	snrwin->dialog =
		gtk_dialog_new_with_buttons(_("Find and Replace"), GTK_WINDOW(bfwin->main_window),
									GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
/*	gtk_window_set_resizable(GTK_WINDOW(snrwin->dialog), FALSE);
	gtk_dialog_set_has_separator(GTK_DIALOG(snrwin->dialog), FALSE);*/
	window_delete_on_escape(GTK_WINDOW(snrwin->dialog));
	g_signal_connect(G_OBJECT(snrwin->dialog), "response", G_CALLBACK(snr3_advanced_response), snrwin);
	g_signal_connect(G_OBJECT(snrwin->dialog), "destroy", G_CALLBACK(snr3win_destroy_cb), snrwin);
	/*g_signal_connect_after(G_OBJECT(snrwin->dialog), "focus-in-event", G_CALLBACK(snr_focus_in_lcb), snrwin);*/
	
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(snrwin->dialog));

	table =
		dialog_table_in_vbox(3, 4, 6/*borderwidth*/, vbox, TRUE,
							 TRUE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	currentrow=0;

	
	history = gtk_list_store_new(1, G_TYPE_STRING);
	list = g_list_last(bfwin->session->searchlist);
	while (list) {
		DEBUG_MSG("snr_dialog_real: adding search history %s\n", (gchar *) list->data);
		gtk_list_store_append(history, &iter);
		gtk_list_store_set(history, &iter, 0, list->data, -1);
		list = g_list_previous(list);
	}
	snrwin->search = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(history), 0);
	if (findtext) {
		entry = gtk_bin_get_child(GTK_BIN(snrwin->search));
		gtk_entry_set_text(GTK_ENTRY(entry), findtext);
	}
	/* this kills the primary selection, which is annoying if you want to 
	   search/replace within the selection  */
	/*if (bfwin->session->searchlist)
	   gtk_combo_box_set_active(GTK_COMBO_BOX(snrwin->search), 0); */
	g_object_unref(history);
	dialog_mnemonic_label_in_table(_("<b>_Search for</b>"), snrwin->search, table, 0, 1, currentrow, currentrow+1);
	gtk_table_attach(GTK_TABLE(table), snrwin->search, 1, 4, currentrow, currentrow+1, GTK_EXPAND | GTK_FILL,
					 GTK_SHRINK, 0, 0);
	currentrow++;

/*	g_signal_connect(snrwin->search, "changed", G_CALLBACK(snr_comboboxentry_changed), snrwin);
	g_signal_connect(snrwin->search, "realize", G_CALLBACK(realize_combo_set_tooltip),
					 _("The pattern to look for"));
	g_signal_connect(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(snrwin->search))), "activate",
					 G_CALLBACK(snr_combo_activate_lcb), snrwin);*/

	snrwin->searchfeedback = gtk_label_new(NULL);
	gtk_label_set_line_wrap(GTK_LABEL(snrwin->searchfeedback), TRUE);
	gtk_misc_set_alignment(GTK_MISC(snrwin->searchfeedback),0,0);
	gtk_table_attach(GTK_TABLE(table), snrwin->searchfeedback, 1, 4, currentrow, currentrow+1, GTK_EXPAND | GTK_FILL,
					 GTK_SHRINK, 0, 0);
	gtk_table_set_row_spacing(GTK_TABLE(table), currentrow, 0);
	currentrow++;
	
	history = gtk_list_store_new(1, G_TYPE_STRING);
	list = g_list_last(bfwin->session->replacelist);
	while (list) {
		DEBUG_MSG("snr_dialog_real: adding replace history %s\n", (gchar *) list->data);
		gtk_list_store_append(history, &iter);
		gtk_list_store_set(history, &iter, 0, list->data, -1);
		list = g_list_previous(list);
	}
	snrwin->replace = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(history), 0);
	g_object_unref(history);
	dialog_mnemonic_label_in_table(_("<b>Replace _with</b>"), snrwin->replace, table, 0, 1, currentrow, currentrow+1);
	gtk_table_attach(GTK_TABLE(table), snrwin->replace, 1, 4, currentrow, currentrow+1, GTK_EXPAND | GTK_FILL,
					 GTK_SHRINK, 0, 0);
	currentrow++;

/*	g_signal_connect(snrwin->replace, "changed", G_CALLBACK(snr_comboboxentry_changed), snrwin);
	g_signal_connect(snrwin->replace, "realize", G_CALLBACK(realize_combo_set_tooltip),
					 _("Replace matching text with"));
	g_signal_connect(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(snrwin->replace))), "activate",
					 G_CALLBACK(snr_combo_activate_lcb), snrwin);
*/

	dialog_mnemonic_label_in_table(_("<b>Options</b>"), NULL, table, 0, 1, currentrow, currentrow+1);
	
	currentrow++;

	snrwin->searchType = gtk_combo_box_new_text();
	for (i = 0; i < G_N_ELEMENTS(matchPattern); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(snrwin->searchType), _(matchPattern[i]));
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(snrwin->searchType), bfwin->session->snr3_type);

	dialog_mnemonic_label_in_table(_("Match Patter_n: "), snrwin->searchType, table, 0, 1, currentrow, currentrow+1);
	gtk_table_attach(GTK_TABLE(table), snrwin->searchType, 1, 4, currentrow, currentrow+1, GTK_EXPAND | GTK_FILL,
					 GTK_SHRINK, 0, 0);
	/*g_signal_connect(snrwin->searchType, "realize", G_CALLBACK(realize_combo_set_tooltip),
					 _("How to interpret the pattern."));
*/
	currentrow++;

	snrwin->replaceType = gtk_combo_box_new_text();
	for (i = 0; i < G_N_ELEMENTS(replaceType); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(snrwin->replaceType), _(replaceType[i]));
	}
	snrwin->replaceTypeL = dialog_mnemonic_label_in_table(_("Replace T_ype: "), snrwin->replaceType, table, 0, 1, currentrow, currentrow+1);
	gtk_table_attach(GTK_TABLE(table), snrwin->replaceType, 1, 4, currentrow, currentrow+1, GTK_EXPAND | GTK_FILL,
					 GTK_SHRINK, 0, 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(snrwin->replaceType), bfwin->session->snr3_replacetype);
	/*g_signal_connect(snrwin->replaceType, "realize", G_CALLBACK(realize_combo_set_tooltip),
					 _("What to replace with."));*/
	
	currentrow++;
	
	snrwin->scope = gtk_combo_box_new_text();
	for (i = 0; i < G_N_ELEMENTS(scope); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(snrwin->scope), _(scope[i]));
	}
	dialog_mnemonic_label_in_table(_("Sco_pe: "), snrwin->scope, table, 0, 1, currentrow, currentrow+1);
	gtk_table_attach(GTK_TABLE(table), snrwin->scope, 1, 4, currentrow, currentrow+1,GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	/*g_signal_connect(snrwin->scope, "realize", G_CALLBACK(realize_combo_set_tooltip),
					 _("Where to look for the pattern."));*/
	gtk_combo_box_set_active(GTK_COMBO_BOX(snrwin->scope), s3scope);
	
	currentrow++;
	/* add a basedir and file pattern widget here */
	lstore = gtk_list_store_new(1, G_TYPE_STRING);
	for (tmplist = g_list_first(bfwin->session->filegloblist); tmplist; tmplist = g_list_next(tmplist)) {
		gtk_list_store_append(GTK_LIST_STORE(lstore), &iter);
		gtk_list_store_set(GTK_LIST_STORE(lstore), &iter, 0, tmplist->data, -1);
	}
	snrwin->filepattern = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(lstore), 0);
	g_object_unref(lstore);
	snrwin->filepatternL = dialog_mnemonic_label_in_table(_("Filename pattern: "), snrwin->filepattern, table, 0, 1, currentrow, currentrow+1);
	gtk_table_attach(GTK_TABLE(table), snrwin->filepattern, 1, 4, currentrow, currentrow+1,GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	
	currentrow++;
	
	snrwin->basedir = gtk_entry_new();
	snrwin->basedirL = dialog_mnemonic_label_in_table(_("Basedir: "), snrwin->basedir, table, 0, 1, currentrow, currentrow+1);

	if (bfwin->current_document && bfwin->current_document->uri) {
		GFile *parent = g_file_get_parent(bfwin->current_document->uri);
		gchar *tmp = g_file_get_uri(parent);
		gtk_entry_set_text(GTK_ENTRY(snrwin->basedir), tmp);
		g_object_unref(parent);
		g_free(tmp);
	}
	gtk_table_attach(GTK_TABLE(table), snrwin->basedir, 1, 3, currentrow, currentrow+1,GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	snrwin->basedirB = dialog_button_new_with_image(NULL, -1, GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU);
	gtk_table_attach(GTK_TABLE(table), snrwin->basedirB, 3, 4, currentrow, currentrow+1,GTK_FILL, GTK_SHRINK, 0, 0);

	currentrow++;

	snrwin->matchCase = dialog_check_button_in_table(_("Case sensitive _matching"), bfwin->session->snr3_casesens, table,
										0, 4, currentrow, currentrow+1);
	gtk_widget_set_tooltip_text(snrwin->matchCase, _("Only match if case (upper/lower) is identical."));

	currentrow++;

	snrwin->escapeChars = dialog_check_button_in_table(_("Pattern contains escape-se_quences"), bfwin->session->snr3_escape_chars, table,
										0, 4, currentrow, currentrow+1);
	gtk_widget_set_tooltip_text(snrwin->escapeChars,
								_("Pattern contains backslash escaped characters such as \\n \\t etc."));

	currentrow++;

	snrwin->dotmatchall = dialog_check_button_in_table(_("Dot character in regex pattern matches newlines"), bfwin->session->snr3_dotmatchall, table,
										0, 4, currentrow, currentrow+1);
	gtk_widget_set_tooltip_text(snrwin->dotmatchall,
								_("The . character will match everything including newlines, use for multiline matching."));


/*	snrwin->overlappingMatches = gtk_check_button_new_with_mnemonic(_("Allow o_verlapping matches"));
	gtk_box_pack_start(GTK_BOX(vbox2), snrwin->overlappingMatches, FALSE, FALSE, 0);
	g_signal_connect(snrwin->overlappingMatches, "toggled", G_CALLBACK(snr_option_toggled), snrwin);
	gtk_widget_set_tooltip_text(snrwin->overlappingMatches,
								_("After a match is found, start next search within that match."));


	snrwin->select_match = gtk_check_button_new_with_mnemonic(_("Select matc_hes"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(snrwin->select_match), main_v->globses.snr_select_match);
	gtk_box_pack_start(GTK_BOX(vbox2), snrwin->select_match, FALSE, FALSE, 0);
	gtk_widget_set_tooltip_text(snrwin->select_match,
								_("Select the matching text instead of just highlighting it"));

	snrwin->bookmarks = gtk_check_button_new_with_mnemonic(_("_Bookmark matches"));
	gtk_box_pack_start(GTK_BOX(vbox2), snrwin->bookmarks, FALSE, FALSE, 0);
	gtk_widget_set_tooltip_text(snrwin->bookmarks, _("Create a bookmark for each match"));
*/
	gtk_dialog_add_button(GTK_DIALOG(snrwin->dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	snrwin->replaceAllButton =
			gtk_dialog_add_button(GTK_DIALOG(snrwin->dialog), _("Replace _All"), SNR_RESPONSE_REPLACE_ALL);
	snrwin->replaceButton =
			gtk_dialog_add_button(GTK_DIALOG(snrwin->dialog), _("_Replace"), SNR_RESPONSE_REPLACE);
	snrwin->backButton = gtk_dialog_add_button(GTK_DIALOG(snrwin->dialog), GTK_STOCK_GO_BACK, SNR_RESPONSE_BACK);
	snrwin->findButton = gtk_dialog_add_button(GTK_DIALOG(snrwin->dialog), GTK_STOCK_GO_FORWARD, SNR_RESPONSE_FIND);
	
	snrwin->bookmarkButton = gtk_dialog_add_button(GTK_DIALOG(snrwin->dialog), _("_Bookmark all"), SNR_RESPONSE_BOOKMARK_ALL);
	/*gtk_dialog_set_response_sensitive(GTK_DIALOG(snrwin->dialog), SNR_RESPONSE_FIND, FALSE); */
	/*snr_comboboxentry_changed(GTK_COMBO_BOX_ENTRY(snrwin->search), snrwin);*/
	gtk_widget_show_all(GTK_WIDGET(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(snrwin->dialog)))));
	/* this kills the primary selection, which is annoying if you want to 
	   search/replace within the selection  */
/*    if (gtk_text_buffer_get_selection_bounds(bfwin->current_document->buffer, &start, &end)) {
        gchar * buffer = gtk_text_buffer_get_text(bfwin->current_document->buffer, &start, &end, FALSE);

        if (strchr(buffer, '\n') == NULL) {
            gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(snrwin->search))), buffer);
            gtk_editable_select_region(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(snrwin->search))), 0, -1);
        }
        if (buffer)    g_free(buffer);
    }*/

	gtk_dialog_set_default_response(GTK_DIALOG(snrwin->dialog), SNR_RESPONSE_FIND);
	DEBUG_MSG("snr_dialog_real: display the dialog\n");
	
	g_signal_connect(gtk_bin_get_child(GTK_BIN(snrwin->search)), "focus-out-event", G_CALLBACK(snrwin_focus_out_event_cb), snrwin);
	g_signal_connect(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(snrwin->search))), "activate",G_CALLBACK(snr_combo_activate_lcb), snrwin);
	g_signal_connect(gtk_bin_get_child(GTK_BIN(snrwin->replace)), "focus-out-event", G_CALLBACK(snrwin_focus_out_event_cb), snrwin);
	g_signal_connect(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(snrwin->replace))), "activate",G_CALLBACK(snr_combo_activate_lcb), snrwin);
	g_signal_connect(snrwin->searchType, "changed", G_CALLBACK(snr_combobox_changed), snrwin);
	g_signal_connect(snrwin->replaceType, "changed", G_CALLBACK(snr_combobox_changed), snrwin);
	g_signal_connect(snrwin->scope, "changed", G_CALLBACK(snr_combobox_changed), snrwin);
	g_signal_connect(snrwin->matchCase, "toggled", G_CALLBACK(snr_option_toggled), snrwin);
	g_signal_connect(snrwin->escapeChars, "toggled", G_CALLBACK(snr_option_toggled), snrwin);	
	g_signal_connect(snrwin->escapeChars, "toggled", G_CALLBACK(snr_option_toggled), snrwin);
	
		
	snr_dialog_show_widgets(snrwin);

	gtk_widget_show(snrwin->dialog);
	
/*	if ((bfwin->session->snr_position_x >= 0) && (bfwin->session->snr_position_y >= 0) &&
		(bfwin->session->snr_position_x < (gdk_screen_width() - 50))
		&& (bfwin->session->snr_position_y < (gdk_screen_height() - 50))) {
		gtk_window_move(GTK_WINDOW(snrwin->dialog), bfwin->session->snr_position_x,
						bfwin->session->snr_position_y);
	}
*/

}

void
snr3_advanced_dialog(Tbfwin * bfwin, const gchar *findtext)
{
	GtkTextIter so, eo;
	Tdocument *doc = bfwin->current_document;
	if (!doc)
		return;
	if (gtk_text_buffer_get_selection_bounds(GTK_TEXT_BUFFER(doc->buffer),&so,&eo) ) {
		/* check if it is a multiline selection */
		if (gtk_text_iter_get_line(&so)==gtk_text_iter_get_line(&eo)) {
			gchar *tmp = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(doc->buffer), &so, &eo, TRUE);
			snr3_advanced_dialog_backend(bfwin, findtext?findtext:tmp, bfwin->session->snr3_scope);
			g_free(tmp);
		} else {
			snr3_advanced_dialog_backend(bfwin, findtext, snr3scope_selection);
		}
	} else {
		snr3_advanced_dialog_backend(bfwin, findtext, bfwin->session->snr3_scope);
	}
}

static void
extern_doc_backend(Tsnr3run * s3run, Tdocument *doc, guint so, guint eo)
{
	s3run->curoffset=0;
	s3run->curposition=0;
	s3run->curdoc = doc;
	s3run->curbuf = doc_get_chars(doc, so, eo);
	utf8_offset_cache_reset();
	s3run->so = so;
	s3run->eo = eo;
	if (s3run->type == snr3type_pcre) {
		backend_pcre_loop(s3run, TRUE);
	} else if (s3run->type == snr3type_string) {
		backend_string_loop(s3run, TRUE);
	}
	g_free(s3run->curbuf);
	s3run->curbuf = NULL;
}

void
snr3_run_extern_replace(Tdocument * doc, const gchar * search_pattern, Tsnr3scope scope,
							 Tsnr3type type, gboolean is_case_sens, const gchar * replace_pattern,
							 gboolean unescape) 
{
	gint so,eo;
	GList *tmplist;
	Tsnr3run * s3run = snr3run_new(doc->bfwin, NULL);
	snr3run_multiset(s3run, search_pattern, NULL, type,snr3replace_string,scope);
	s3run->replace = g_strdup(replace_pattern);
	update_snr3run(s3run);
	s3run->replaceall = TRUE;
	
	switch(s3run->scope) {
		case snr3scope_doc:
			extern_doc_backend(s3run, doc, 0, -1);
		break;
		case snr3scope_cursor:
			so = doc_get_cursor_position(doc);
			extern_doc_backend(s3run, doc, so, -1);
		break;
		case snr3scope_selection:
			if (doc_get_selection(doc, &so, &eo)) {
				extern_doc_backend(s3run, doc, so, eo);
			} else {
				DEBUG_MSG("Find in selection, but there is no selection\n");
			}
		break;
		case snr3scope_alldocs:
			for (tmplist=g_list_first(s3run->bfwin->documentlist);tmplist;tmplist=g_list_next(tmplist)) {
				extern_doc_backend(s3run, tmplist->data, 0, -1);
			}
		break;
		case snr3scope_files:
			g_warning("snr3_run_extern_replace does not support replace in files\n");
		break;
	}
	snr3run_free(s3run);
}

