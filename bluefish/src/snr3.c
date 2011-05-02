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
#include "bfwin.h"
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
	DEBUG_MSG("scroll_to_result, started for s3result %p\n",s3result);
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

static void highlight_run_in_doc(Tsnr3run *s3run, Tdocument *doc) {
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
	guint startingpoint;
	gint offset;
} Toffsetupdate;

static void snr3run_update_offsets(gpointer s3result, gpointer offsetupdate) {
	if (((Tsnr3result *)s3result)->so > ((Toffsetupdate *)offsetupdate)->startingpoint) {
		((Tsnr3result *)s3result)->so += ((Toffsetupdate *)offsetupdate)->offset;
		((Tsnr3result *)s3result)->eo += ((Toffsetupdate *)offsetupdate)->offset;
	}
}

static gchar * 
retrieve_pcre_replace_string(Tsnr3run *s3run, Tsnr3result *s3result)
{
	GError *gerror=NULL;
	GMatchInfo *matchinfo;
	gboolean found;
	gchar *newstr=NULL;
	gchar *buf = doc_get_chars(s3result->doc, s3result->so-10, s3result->eo+10);
	g_print("retrieve_pcre_replace_string, search in %s\n",buf);
	found = g_regex_match_full(s3run->regex,buf,-1, 10,0, &matchinfo, &gerror);
	if (gerror) {
		g_print("retrieve_pcre_replace_string, error %s\n",gerror->message);
		g_error_free(gerror);
	} else if (found) {
		gchar *newstr = g_match_info_expand_references(matchinfo, s3run->replace, &gerror);
		if (gerror) {
			g_print("retrieve_pcre_replace_string, error %s\n",gerror->message);
			g_error_free(gerror);
		} else {
			g_print("retrieve_pcre_replace_string, return %s\n",newstr);
		}
	}
	g_free(buf);
	g_match_info_free(matchinfo);
	return newstr;
}

static Toffsetupdate 
s3result_replace(Tsnr3run *s3run, Tsnr3result *s3result, gint offset)
{
	Toffsetupdate offsetupdate = {0,0};
	if (s3run->replacetype == snr3replace_string) {
		if (s3run->type == snr3type_string) {
			g_print("s3result_replace, replace %d:%d with %s\n", s3result->so+offset, s3result->eo+offset, s3run->replace);
			doc_replace_text_backend(s3result->doc, s3run->replace, s3result->so+offset, s3result->eo+offset);
			offsetupdate.offset = g_utf8_strlen(s3run->replace, -1)-(s3result->eo - s3result->so);
		} else if (s3run->type == snr3type_pcre) {
			gchar *newstr = retrieve_pcre_replace_string(s3run, s3result);
			if (newstr) {
				doc_replace_text_backend(s3result->doc, newstr, s3result->so+offset, s3result->eo+offset);
				offsetupdate.offset = g_utf8_strlen(newstr, -1)-(s3result->eo - s3result->so);
				g_free(newstr);
			}
		}
	} else {
		gchar *tmp1, *tmp2;
		tmp1 = doc_get_chars(s3result->doc, s3result->so+offset, s3result->eo+offset);
		if (s3run->replacetype == snr3replace_upper) 
			tmp2 = g_utf8_strup(tmp1, -1);
		else
			tmp2 = g_utf8_strdown(tmp1, -1);
		doc_replace_text_backend(s3result->doc, tmp2, s3result->so+offset, s3result->eo+offset);
		g_free(tmp1);
		g_free(tmp2);
	}
	offsetupdate.startingpoint = s3result->eo;
	return offsetupdate;
}


static void snr3result_free(gpointer s3result, gpointer s3run) {
	/* BUG TODO: for pcre patterns we should free the 'extra' data */
	DEBUG_MSG("free result %p\n",s3result);
	g_slice_free(Tsnr3result, (Tsnr3result *)s3result);
}


static void
s3run_replace_current(Tsnr3run *s3run)
{
	Toffsetupdate offsetupdate;
	Tsnr3result *s3result=NULL;
	GList *next, *current;
	
	if (s3run->current) {
		current = s3run->current;
		next = s3run->current->next;
	}
	if (!current)
		return;
	s3result = current->data;
	offsetupdate = s3result_replace(s3run, s3result, 0);

	snr3result_free(s3result, s3run);
	g_queue_delete_link(&s3run->results, current);
	s3run->current = next;
	
	if (s3run->current) {
		scroll_to_result(s3run->current->data, NULL);
	}
	if (offsetupdate.offset != 0) {
		/* now re-calculate all the offsets in the results lists!!!!!!!!!!! */
		g_queue_foreach(&s3run->results, snr3run_update_offsets,&offsetupdate);
	}
}

static void sn3run_add_result(Tsnr3run *s3run, gulong so, gulong eo, gpointer doc) {
	Tsnr3result *s3result;
	s3result = g_slice_new(Tsnr3result);
	s3result->so = so;
	s3result->eo = eo;
	s3result->doc = doc;
	g_queue_push_tail(&s3run->results, s3result);
} 


static gboolean snr3_run_pcre_loop(Tsnr3run *s3run) {
	gint loop=0;
	GTimer *timer;
	gboolean cont=TRUE;
	static guint loops_per_timer = 10;
	GMatchInfo *match_info = NULL;
	GError *gerror = NULL;
	
	DEBUG_MSG("snr3_run_pcre_loop, started, regex=%p\n",s3run->regex);
	/* reconstruct where we are searching */
	g_regex_match_full(s3run->regex, s3run->curbuf, -1, s3run->curoffset, G_REGEX_MATCH_NEWLINE_ANY, &match_info, NULL);
	
	timer = g_timer_new();
	while (cont && (loop % loops_per_timer != 0
				 || g_timer_elapsed(timer, NULL) < MAX_CONTINUOUS_SEARCH_INTERVAL)) {
		gint so, eo;
		g_match_info_fetch_pos(match_info, 0, &so, &eo);
		so = utf8_byteoffset_to_charsoffset_cached(s3run->curbuf, so);
		eo = utf8_byteoffset_to_charsoffset_cached(s3run->curbuf, eo);
		sn3run_add_result(s3run, so, eo, s3run->curdoc);
		
		s3run->curoffset = eo;
		
		g_match_info_next (match_info, &gerror);
		if (gerror) {
			g_print("snr3_run_pcre_loop, match error %s\n",gerror->message);
			g_error_free(gerror);
		}
		loop++;
		cont = g_match_info_matches(match_info);
	}
	DEBUG_MSG("snr3_run_pcre_loop, did %d loops in %f seconds\n",loop, g_timer_elapsed(timer, NULL));
	g_timer_destroy(timer);
	g_match_info_free(match_info);
	
	if (cont) {
		loops_per_timer = MAX(loop / 10, 10);
		DEBUG_MSG("snr3_run_pcre_loop, continue the search another time\n");
		return TRUE; /* call me again */
	}
	DEBUG_MSG("snr3_run_pcre_loop, finished document %p\n",s3run->curdoc);
	g_free(s3run->curbuf);
	
	if (s3run->scope == snr3scope_alldocs) {
		/* look if there is a next doc */
		GList *tmplist;
		tmplist = g_list_find(s3run->bfwin->documentlist, s3run->curdoc);
		tmplist = g_list_next(tmplist);
		if (tmplist) {
			s3run->curdoc = DOCUMENT(tmplist->data);
			s3run->curbuf = doc_get_chars(s3run->curdoc, 0, -1);
			s3run->curoffset = 0;
			return TRUE;
		}
	}
	
	DEBUG_MSG("no more work, run the callback\n");
	s3run->callback(s3run);
	return FALSE;
}

static void snr3_run_pcre_in_doc(Tsnr3run *s3run, Tdocument *doc, gint so, gint eo) {
	s3run->curdoc = doc;
	s3run->curbuf = doc_get_chars(doc, so, eo);
	s3run->so = so;
	s3run->eo = eo;
	utf8_offset_cache_reset();
	s3run->idle_id = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,(GSourceFunc)snr3_run_pcre_loop,s3run,NULL);
}

static gboolean snr3_run_string_loop(Tsnr3run *s3run) {
	gint querylen;
	char *(*f) ();
	gint loop=0;
	static guint loops_per_timer=10;
	gchar *result;
	GTimer *timer;

	timer = g_timer_new();

	if (s3run->is_case_sens) {
		f = strstr;
	} else {
		f = strcasestr;
	}
	querylen = g_utf8_strlen(s3run->query, -1);
	
	/* now reconstruct the last scan offset */
	result = s3run->curbuf + utf8_charoffset_to_byteoffset_cached(s3run->curbuf, s3run->curoffset);

	do {
		result = f(result, s3run->query);
		if (result) {
			glong char_o = utf8_byteoffset_to_charsoffset_cached(s3run->curbuf, (result-s3run->curbuf));
			sn3run_add_result(s3run, char_o+s3run->so, char_o+querylen+s3run->so, s3run->curdoc);
			s3run->curoffset = char_o+querylen+s3run->so;
			
#ifdef SNR3_PROFILING
			s3profiling.numresults++;
#endif
			result++;
			loop++;
		}		
	} while (result && (loop % loops_per_timer != 0
				 || g_timer_elapsed(timer, NULL) < MAX_CONTINUOUS_SEARCH_INTERVAL));
	DEBUG_MSG("did %d loops in %f seconds\n",loop, g_timer_elapsed(timer, NULL));
	g_timer_destroy(timer);

	if (result) {
		return TRUE;
	}
	
	g_free(s3run->curbuf);
	
	if (s3run->scope == snr3scope_alldocs) {
		/* look if there is a next doc */
		GList *tmplist;
		tmplist = g_list_find(s3run->bfwin->documentlist, s3run->curdoc);
		tmplist = g_list_next(tmplist);
		if (tmplist) {
			g_print("continue with document %p\n",tmplist->data);
			s3run->curdoc = DOCUMENT(tmplist->data);
			utf8_offset_cache_reset();
			s3run->curbuf = doc_get_chars(s3run->curdoc, 0, -1);
			s3run->curoffset = 0;
			return TRUE;
		}
	}
	
	DEBUG_MSG("no more work, run the callback\n");
	s3run->callback(s3run);
#ifdef SNR3_PROFILING
	g_print("search %ld bytes with %d results run took %f s, %f bytes/s\n",(glong)strlen(buf),s3profiling.numresults,g_timer_elapsed(s3profiling.timer, NULL),strlen(buf)/g_timer_elapsed(s3profiling.timer, NULL));
	g_timer_destroy(s3profiling.timer);
#endif

	DEBUG_MSG("snr3_run_string_in_doc, finished with %d results\n",s3run->results.length);
	
	return FALSE;
	
}


static void snr3_run_string_in_doc(Tsnr3run *s3run, Tdocument *doc, gint so, gint eo) {
	DEBUG_MSG("snr3_run_string_in_doc, doc=%p, s3run=%p, started for query %s\n",doc, s3run, s3run->query);
#ifdef SNR3_PROFILING
	s3profiling.timer = g_timer_new();
	s3profiling.numresults=0;
#endif

	s3run->curdoc = doc;
	s3run->curbuf = doc_get_chars(doc, so, eo);
	s3run->so = so;
	s3run->eo = eo;
	utf8_offset_cache_reset();
	s3run->idle_id = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,(GSourceFunc)snr3_run_string_loop,s3run,NULL);	
}

void snr3_run_go(Tsnr3run *s3run, gboolean forward) {
	GList *next=NULL;
	DEBUG_MSG("snr3_run_go, s3run=%p\n",s3run);
	if (s3run->current) {
		if (forward) {
			next = g_list_next(s3run->current);
		} else {
			next = g_list_previous(s3run->current);
		}
	}
	if (!next) {
		g_print("no 'next' (current=%p)\n", s3run->current);
		next = forward ? s3run->results.head : s3run->results.tail;
		DEBUG_MSG("snr3_run_go, no next (current=%p), new next=%p\n",s3run->current,next);
	}
	DEBUG_MSG("scroll to result %p\n",next);
	if (next) {
		s3run->current = next;
		scroll_to_result(next->data, NULL);
	}
}

static void
snr3_run(Tsnr3run *s3run, void (*callback)(void *))
{
	gint so,eo;
	GList *tmplist;
	DEBUG_MSG("snr3_run, s3run=%p, scope=%d, query=%s\n",s3run, s3run->scope, s3run->query);

	if (s3run->query == NULL || s3run->query[0]=='\0') {
		g_print("no, query, no s3run\n");
		return;
	}
	s3run->callback = callback;
	
	switch(s3run->scope) {
		case snr3scope_doc:
			if (s3run->type == snr3type_string) 
				snr3_run_string_in_doc(s3run, s3run->bfwin->current_document, 0, -1);
			else if (s3run->type == snr3type_pcre)
				snr3_run_pcre_in_doc(s3run, s3run->bfwin->current_document, 0, -1);
		break;
		case snr3scope_cursor:
			so = doc_get_cursor_position(s3run->bfwin->current_document);
			if (s3run->type == snr3type_string) 
				snr3_run_string_in_doc(s3run, s3run->bfwin->current_document, so, -1);
			else if (s3run->type == snr3type_pcre)
				snr3_run_pcre_in_doc(s3run, s3run->bfwin->current_document, so, -1);
		break;
		case snr3scope_selection:
			if (doc_get_selection(s3run->bfwin->current_document, &so, &eo)) {
				if (s3run->type == snr3type_string) 
					snr3_run_string_in_doc(s3run, s3run->bfwin->current_document, so, eo);
				else if (s3run->type == snr3type_pcre)
					snr3_run_pcre_in_doc(s3run, s3run->bfwin->current_document, so, eo);
			} else {
				g_print("Find in selection, but there is no selection\n");
			}
		break;
		case snr3scope_alldocs:
			tmplist=g_list_first(s3run->bfwin->documentlist);
			if (s3run->type == snr3type_string) 
				snr3_run_string_in_doc(s3run, tmplist->data, 0, -1);
			else if (s3run->type == snr3type_pcre)
				snr3_run_pcre_in_doc(s3run, tmplist->data, 0, -1);
		break;
		case snr3scope_files:
			s3run->basedir = g_file_new_for_path("/tmp/");
			s3run->recursive = TRUE;
			s3run->filepattern = g_strdup("*.txt");
			snr3_run_in_files(s3run, callback);
		break;
	}
}

void snr3run_free(Tsnr3run *s3run) {
	DEBUG_MSG("snr3run_free, started for %p\n",s3run);
	DEBUG_MSG("snr3run_free, query at %p\n",s3run->query);
	g_free(s3run->query);
	g_free(s3run->replace);
	remove_all_highlights_in_doc(s3run->bfwin->current_document);
	
	g_queue_foreach(&s3run->results, snr3result_free,s3run);
	g_slice_free(Tsnr3run, s3run);
}

static void activate_simple_search(void *data) {
	Tsnr3run *s3run=data;
	g_print("activate_simple_search\n");
	highlight_run_in_doc(s3run, s3run->bfwin->current_document);
	snr3_run_go(s3run, TRUE);
}

static Tsnr3run *
snr3run_new(Tbfwin *bfwin, gpointer dialog) 
{
	Tsnr3run *s3run;
	s3run = g_slice_new0(Tsnr3run);
	s3run->bfwin = bfwin;
	s3run->dialog = dialog;
	g_queue_init(&s3run->results);
	return s3run;	
} 


static void snr3run_multiset(Tsnr3run *s3run, 
				const gchar *query, const gchar *replace, 
				Tsnr3type type, Tsnr3replace replacetype,
				Tsnr3scope scope) {
	g_free(s3run->query);
	s3run->query = g_strdup(query);
	g_free(s3run->replace);
	s3run->replace = g_strdup(replace);
	g_print("snr3run_init, query=%s, replace=%s, type=%d, replacetype=%d, scope=%d\n",query, replace, type,replacetype, scope);
	s3run->type = type;
	s3run->replacetype = replacetype;
	s3run->scope = scope;
	/* TODO: potential memory leak below, what is there are resulkts on the queue ? */
	g_queue_init(&s3run->results);
	
} 



gpointer simple_search_run(Tbfwin *bfwin, const gchar *string) {
	Tsnr3run *s3run;
	
	s3run = snr3run_new(bfwin, NULL);
	snr3run_multiset(s3run, string, NULL, snr3type_string,snr3replace_string,snr3scope_doc);
	DEBUG_MSG("snr3run at %p, query at %p\n",s3run, s3run->query);
	snr3_run(s3run, activate_simple_search);
	return s3run;
}

static void dialog_changed_run_ready_cb(gpointer data) {
	Tsnr3run *s3run=data;
	g_print("dialog_changed_run_ready_cb, finished with %d results\n",g_queue_get_length(&s3run->results));
	highlight_run_in_doc(s3run, s3run->bfwin->current_document);
	if (s3run->dialog) {
		TSNRWin *snrwin = s3run->dialog;
		gchar *tmp = g_strdup_printf("<i>found %d results</i>\n",g_queue_get_length(&s3run->results));
		gtk_label_set_markup(GTK_LABEL(snrwin->searchfeedback),tmp);
		gtk_widget_show(snrwin->searchfeedback);
		g_free(tmp);
	}
}

/***************************************************************************/
/***************************** GUI *****************************************/
/***************************************************************************/

enum {
	SNR_RESPONSE_FIND = 0,
	SNR_RESPONSE_REPLACE,
	SNR_RESPONSE_REPLACE_ALL
};

static void
snr3_cleanup(void *data)
{
	Tsnr3run *s3run=data;
	DEBUG_MSG("snr3_cleanup %p\n",s3run);
	g_free(s3run->query);
	g_free(s3run->replace);
	if (s3run->basedir)
		g_object_unref(s3run->basedir);
	g_free(s3run->filepattern);
	
	g_slice_free(Tsnr3run, s3run);
}

static void
snr3run_resultcleanup(Tsnr3run *s3run) 
{
	GList *tmplist;
	for (tmplist=g_list_first(s3run->results.head);tmplist;tmplist=g_list_next(tmplist)) {
		g_slice_free(Tsnr3result, tmplist->data);
	}
	g_queue_clear(&s3run->results);
}

static gboolean compile_regex(TSNRWin *snrwin, Tsnr3run *s3run, const gchar *query) {
	GError *gerror = NULL;
	s3run->regex = g_regex_new(query,
					 (s3run->is_case_sens ? G_REGEX_MULTILINE | G_REGEX_DOTALL : G_REGEX_CASELESS |
					  G_REGEX_MULTILINE | G_REGEX_DOTALL), G_REGEX_MATCH_NEWLINE_ANY, &gerror);
	if (gerror) {
		gchar *message = g_markup_printf_escaped("<span foreground=\"red\">%s</span>", gerror->message);
		g_print("compile_regex, regex error %s\n",gerror->message);
		gtk_label_set_markup(GTK_LABEL(snrwin->searchfeedback), message);
		gtk_widget_show(snrwin->searchfeedback);
		g_free(message);
		g_error_free(gerror);
		return FALSE;
	}
	g_print("compile_regex, return TRUE\n");
	return TRUE;
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
	gint type, replacetype, scope;
	gboolean is_case_sens;
	gint retval=0;
	
	query = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(snrwin->search))));
	replace = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(snrwin->replace))));
	type = gtk_combo_box_get_active(GTK_COMBO_BOX(snrwin->searchType));
	replacetype = gtk_combo_box_get_active(GTK_COMBO_BOX(snrwin->replaceType));
	scope = gtk_combo_box_get_active(GTK_COMBO_BOX(snrwin->scope));
	is_case_sens = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(snrwin->matchCase));

	if (is_case_sens != s3run->is_case_sens) {
		g_print("set is_case_sens %d\n",is_case_sens);
		s3run->is_case_sens = is_case_sens;
		retval |= 1;
	}
	if (replacetype != s3run->replacetype) {
		g_print("set replacetype %d\n",replacetype);
		s3run->replacetype = replacetype;
		retval |= 2;
	}
	if (scope != s3run->scope) {
		g_print("set scope %d\n",scope);
		s3run->scope = scope;
		retval |= 1;
	}
	if (type != s3run->type) {
		g_print("set type %d\n",type);
		s3run->type = type;
		retval |= 1;
	}

	if (g_strcmp0(s3run->query, query)!=0) {
		g_free(s3run->query);
		s3run->query = NULL;
		g_print("set query %s\n",query);
		s3run->query = g_strdup(query);
		retval |= 1;
	}
	
	if (g_strcmp0(s3run->replace, replace)!=0) {
		g_free(s3run->replace);
		g_print("set replace %s\n",replace);
		s3run->replace = g_strdup(replace);
		retval |= 2;
		g_print("retval=%d\n",retval);
	}
	
	if ((retval & 1) != 0) {
		if (s3run->regex) {
			g_regex_unref(s3run->regex);
			s3run->regex = NULL;
		}
		
		if (type == snr3type_pcre) {
			if (!compile_regex(snrwin, s3run, query)) {
				g_free(s3run->query);
				s3run->query = NULL; /* mark query as unusable */
				return -1;
			}
		}
		
		
		snr3run_resultcleanup(s3run);
	}
	gtk_widget_hide(snrwin->searchfeedback);
	g_print("snr3run_init_from_gui, return %d\n", retval);
	return retval;
}

static gboolean
snrwin_focus_out_event_cb(GtkWidget *widget,GdkEventFocus *event,gpointer data)
{
	TSNRWin *snrwin=data;
	gint guichange;
	if (!snrwin->s3run) {
		snrwin->s3run = snr3run_new(snrwin->bfwin, snrwin);
	}
	guichange = snr3run_init_from_gui(snrwin, snrwin->s3run);
	g_print("search_focus_out_event_cb, guichange=%d\n",guichange);
	if ((guichange & 1) != 0) {
		g_print("search_focus_out_event_cb, run snr3_run\n");
		snr3_run(snrwin->s3run, dialog_changed_run_ready_cb);
	}
	return FALSE;
}
static void
snr3_advanced_response(GtkDialog * dialog, gint response, TSNRWin * snrwin)
{
	Tsnr3run *s3run = snrwin->s3run;
	gint guichange;
	if (!snrwin->s3run) {
		snrwin->s3run = snr3run_new(snrwin->bfwin, snrwin);
	}
	guichange = snr3run_init_from_gui(snrwin, snrwin->s3run);
	g_print("snr3_advanced_response, response=%d, guichange=%d\n",response,guichange);
	if (guichange == -1)
		return;

	switch(response) {
		case SNR_RESPONSE_FIND:
			g_print("current=%p\n",snrwin->s3run->current);
			if ((guichange & 1) != 0) {
				g_print("guichange=%d, call snr3_run\n",guichange);
				snr3_run(snrwin->s3run, activate_simple_search);
			} else if (snrwin->s3run->results.length) {
				g_print("guichange=%d, call snr3_run_go\n",guichange);
				snr3_run_go(s3run, TRUE);
			}
		break;
		case SNR_RESPONSE_REPLACE:
			s3run_replace_current(snrwin->s3run);
			doc_unre_new_group(snrwin->bfwin->current_document);
			if ((guichange == 0) && s3run->current) {
				scroll_to_result(s3run->current->data, NULL);
			}
		break;
		case SNR_RESPONSE_REPLACE_ALL:
			g_print("TODO: implement replace all\n");
			/*snr3_run(s3run, snr3_cleanup);*/
		break;
		case GTK_RESPONSE_CLOSE:
			gtk_widget_destroy(GTK_WIDGET(dialog));
		break;
	}

}

static void
snr_combobox_changed(GtkComboBox * combobox, TSNRWin * snrwin)
{
	gint value;
	if (combobox == GTK_COMBO_BOX(snrwin->replaceType)) {
		value = gtk_combo_box_get_active(GTK_COMBO_BOX(snrwin->replaceType));
		gtk_widget_set_sensitive(snrwin->replace, (value == snr3type_string));
	} else if (combobox == GTK_COMBO_BOX(snrwin->scope)) {
		value = gtk_combo_box_get_active(GTK_COMBO_BOX(snrwin->scope));
		if (value == snr3scope_files) {
			gtk_widget_show_all(snrwin->fileshbox);
		} else {
			gtk_widget_hide(snrwin->fileshbox);
		}
	}
}

void 
snr3_advanced_dialog(Tbfwin * bfwin)
{
	TSNRWin *snrwin;
	GtkWidget *table, *vbox, *button, *widget;
	gint currentrow=0;
	GtkListStore *history, *lstore;
	GList *list;
	GtkTreeIter iter;
	/*GtkTextIter start, end; */
	unsigned int i = 0;

	const gchar *scope[] = {
		N_("Entire document"),
		N_("Forward from cursor position"),
		N_("Selection"),
		N_("All open documents"),
		N_("Files on disk")
	};

	const gchar *matchPattern[] = {
		N_("Normal"),
		N_("PERL style regular expression"),
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
	/*g_signal_connect_after(G_OBJECT(snrwin->dialog), "focus-in-event", G_CALLBACK(snr_focus_in_lcb), snrwin);*/
	
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(snrwin->dialog));
	
	history = gtk_list_store_new(1, G_TYPE_STRING);
	list = g_list_last(bfwin->session->searchlist);
	while (list) {
		DEBUG_MSG("snr_dialog_real: adding search history %s\n", (gchar *) list->data);
		gtk_list_store_append(history, &iter);
		gtk_list_store_set(history, &iter, 0, list->data, -1);
		list = g_list_previous(list);
	}
	snrwin->search = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(history), 0);
	/* this kills the primary selection, which is annoying if you want to 
	   search/replace within the selection  */
	/*if (bfwin->session->searchlist)
	   gtk_combo_box_set_active(GTK_COMBO_BOX(snrwin->search), 0); */
	g_object_unref(history);
	widget = dialog_mnemonic_label_new(_("<b>_Search for</b>"), snrwin->search);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), snrwin->search, TRUE, TRUE, 2);
	g_signal_connect(gtk_bin_get_child(GTK_BIN(snrwin->search)), "focus-out-event", G_CALLBACK(snrwin_focus_out_event_cb), snrwin);
/*	g_signal_connect(snrwin->search, "changed", G_CALLBACK(snr_comboboxentry_changed), snrwin);
	g_signal_connect(snrwin->search, "realize", G_CALLBACK(realize_combo_set_tooltip),
					 _("The pattern to look for"));
	g_signal_connect(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(snrwin->search))), "activate",
					 G_CALLBACK(snr_combo_activate_lcb), snrwin);*/
	snrwin->searchfeedback = gtk_label_new(NULL);
	gtk_label_set_line_wrap(GTK_LABEL(snrwin->searchfeedback), TRUE);
	gtk_misc_set_alignment(GTK_MISC(snrwin->searchfeedback),0,0);
	gtk_box_pack_start(GTK_BOX(vbox), snrwin->searchfeedback, TRUE, TRUE, 2);

	history = gtk_list_store_new(1, G_TYPE_STRING);
	list = g_list_last(bfwin->session->replacelist);
	while (list) {
		DEBUG_MSG("snr_dialog_real: adding replace history %s\n", (gchar *) list->data);
		gtk_list_store_append(history, &iter);
		gtk_list_store_set(history, &iter, 0, list->data, -1);
		list = g_list_previous(list);
	}
	snrwin->replace = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(history), 0);
	g_signal_connect(gtk_bin_get_child(GTK_BIN(snrwin->replace)), "focus-out-event", G_CALLBACK(snrwin_focus_out_event_cb), snrwin);
	g_object_unref(history);
	widget = dialog_mnemonic_label_new(_("<b>Replace _with</b>"), snrwin->replace);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), snrwin->replace, TRUE, TRUE, 2);
/*	g_signal_connect(snrwin->replace, "changed", G_CALLBACK(snr_comboboxentry_changed), snrwin);
	g_signal_connect(snrwin->replace, "realize", G_CALLBACK(realize_combo_set_tooltip),
					 _("Replace matching text with"));
	g_signal_connect(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(snrwin->replace))), "activate",
					 G_CALLBACK(snr_combo_activate_lcb), snrwin);
*/

	table =
		dialog_table_in_vbox(3, 3, 6, vbox, TRUE,
							 TRUE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	currentrow=0;

	dialog_mnemonic_label_in_table(_("<b>Options</b>"), NULL, table, 0, 1, currentrow, currentrow+1);
	
	currentrow++;

	snrwin->searchType = gtk_combo_box_new_text();
	for (i = 0; i < G_N_ELEMENTS(matchPattern); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(snrwin->searchType), _(matchPattern[i]));
	}
	dialog_mnemonic_label_in_table(_("Match Patter_n: "), snrwin->searchType, table, 0, 1, currentrow, currentrow+1);
	gtk_table_attach(GTK_TABLE(table), snrwin->searchType, 1, 3, currentrow, currentrow+1, GTK_EXPAND | GTK_FILL,
					 GTK_SHRINK, 0, 0);
/*	g_signal_connect(snrwin->searchType, "changed", G_CALLBACK(snr_combobox_changed), snrwin);
	g_signal_connect(snrwin->searchType, "realize", G_CALLBACK(realize_combo_set_tooltip),
					 _("How to interpret the pattern."));
*/
	currentrow++;

	snrwin->replaceType = gtk_combo_box_new_text();
	for (i = 0; i < G_N_ELEMENTS(replaceType); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(snrwin->replaceType), _(replaceType[i]));
	}
	dialog_mnemonic_label_in_table(_("Replace T_ype: "), snrwin->replaceType, table, 0, 1, currentrow, currentrow+1);
	gtk_table_attach(GTK_TABLE(table), snrwin->replaceType, 1, 3, currentrow, currentrow+1, GTK_EXPAND | GTK_FILL,
					 GTK_SHRINK, 0, 0);
	/*g_signal_connect(snrwin->replaceType, "changed", G_CALLBACK(snr_combobox_changed), snrwin);
	g_signal_connect(snrwin->replaceType, "realize", G_CALLBACK(realize_combo_set_tooltip),
					 _("What to replace with."));*/
	
	currentrow++;
	
	snrwin->scope = gtk_combo_box_new_text();
	for (i = 0; i < G_N_ELEMENTS(scope); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(snrwin->scope), _(scope[i]));
	}
	dialog_mnemonic_label_in_table(_("Sco_pe: "), snrwin->scope, table, 0, 1, currentrow, currentrow+1);
	gtk_table_attach(GTK_TABLE(table), snrwin->scope, 1, 3, currentrow, currentrow+1,GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	g_signal_connect(snrwin->scope, "changed", G_CALLBACK(snr_combobox_changed), snrwin);
	/*g_signal_connect(snrwin->scope, "realize", G_CALLBACK(realize_combo_set_tooltip),
					 _("Where to look for the pattern."));*/
	
	currentrow++;
	
/*	gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new("File selection options:"), TRUE, TRUE, 2);*/
	snrwin->fileshbox = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), snrwin->fileshbox, TRUE, TRUE, 2);
	/* add a basedir and file pattern widget here */
	lstore = gtk_list_store_new(1, G_TYPE_STRING);
	/*for (i = 0; i < G_N_ELEMENTS(fileExts); i++) {
		gtk_list_store_append(GTK_LIST_STORE(lstore), &iter);
		gtk_list_store_set(GTK_LIST_STORE(lstore), &iter, 0, fileExts[i], -1);
	};*/
	snrwin->filepattern = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(lstore), 0);
	g_object_unref(lstore);
	gtk_box_pack_start(GTK_BOX(snrwin->fileshbox), snrwin->filepattern, TRUE, TRUE, 2);
	snrwin->basedir = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(snrwin->fileshbox), snrwin->basedir, TRUE, TRUE, 2);
	button = dialog_button_new_with_image(NULL, -1, GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU);
	gtk_box_pack_start(GTK_BOX(snrwin->fileshbox), button, TRUE, TRUE, 2);

	table =
		dialog_table_in_vbox(2, 4, 6, vbox, TRUE,
							 TRUE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	currentrow=0;
	
	snrwin->matchCase = dialog_check_button_in_table(_("Case sensitive _matching"), FALSE, table,
										0, 2, currentrow, currentrow+1);
	/*g_signal_connect(snrwin->matchCase, "toggled", G_CALLBACK(snr_option_toggled), snrwin);*/
	gtk_widget_set_tooltip_text(snrwin->matchCase, _("Only match if case (upper/lower) is identical."));

	snrwin->escapeChars = dialog_check_button_in_table(_("Pattern contains escape-se_quences"), FALSE, table,
										2, 4, currentrow, currentrow+1);
	/*g_signal_connect(snrwin->escapeChars, "toggled", G_CALLBACK(snr_option_toggled), snrwin);*/
	gtk_widget_set_tooltip_text(snrwin->escapeChars,
								_("Pattern contains backslash escaped characters such as \\n \\t etc."));

	currentrow++;


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
	snrwin->findButton = gtk_dialog_add_button(GTK_DIALOG(snrwin->dialog), GTK_STOCK_FIND, SNR_RESPONSE_FIND);
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

	gtk_combo_box_set_active(GTK_COMBO_BOX(snrwin->scope), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(snrwin->searchType), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(snrwin->replaceType), 0);
	gtk_dialog_set_default_response(GTK_DIALOG(snrwin->dialog), SNR_RESPONSE_FIND);
	DEBUG_MSG("snr_dialog_real: display the dialog\n");
	gtk_widget_show(snrwin->dialog);

/*	if ((bfwin->session->snr_position_x >= 0) && (bfwin->session->snr_position_y >= 0) &&
		(bfwin->session->snr_position_x < (gdk_screen_width() - 50))
		&& (bfwin->session->snr_position_y < (gdk_screen_height() - 50))) {
		gtk_window_move(GTK_WINDOW(snrwin->dialog), bfwin->session->snr_position_x,
						bfwin->session->snr_position_y);
	}
*/

}

