/*#define DEBUG*/
/*#define SNR3_PROFILING*/

#define _GNU_SOURCE

#include <gtk/gtk.h>
#include <string.h>
#include "config.h"
#include "bluefish.h"
#include "document.h"
#include "bf_lib.h"

#include "snr3.h"

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
	
	g_print("s3run=%p, highlight all results that have doc %p\n",s3run, doc);
	for (tmplist=s3run->results.head;tmplist;tmplist=g_list_next(tmplist)) {
		Tsnr3result *s3result = tmplist->data;
		if (s3result->doc == doc) {
			highlight_result(s3result);
		}
	}
	g_print("highlight_run_in_doc, finished\n");
}

static void sn3run_add_result(Tsnr3run *s3run, gulong so, gulong eo, gpointer doc, gpointer extra) {
	Tsnr3result *s3result;
	s3result = g_slice_new(Tsnr3result);
	s3result->so = so;
	s3result->eo = eo;
	s3result->doc = doc;
	s3result->extra = extra;
	g_queue_push_tail(&s3run->results, s3result);
} 

static void snr3_run_pcre_in_doc(Tsnr3run *s3run, Tdocument *doc, gint so, gint eo) {
	gchar *buf;
	GError *gerror = NULL;
	GRegex *gregex;
	GMatchInfo *match_info = NULL;
	gpointer extra=NULL;
		
	g_print("snr3_run_pcre_in_doc, started for query %s\n",s3run->query);
	if (s3run->results.head) {
		g_warning("s3run has results already\n");
	}
	buf = doc_get_chars(doc, so, eo);
	utf8_offset_cache_reset();
	gregex = g_regex_new(s3run->query,
							 (s3run->is_case_sens ? G_REGEX_MULTILINE | G_REGEX_DOTALL : G_REGEX_CASELESS |
							  G_REGEX_MULTILINE | G_REGEX_DOTALL), G_REGEX_MATCH_NEWLINE_ANY, &gerror);

	g_regex_match_full(gregex, buf, -1, 0, G_REGEX_MATCH_NEWLINE_ANY, &match_info, NULL);
	DEBUG_MSG("snr3_run_pcre_in_doc, start loop\n");
	while (g_match_info_matches(match_info)) {
		gint so, eo;
		g_match_info_fetch_pos(match_info, 0, &so, &eo);
		so = utf8_byteoffset_to_charsoffset_cached(buf, so);
		eo = utf8_byteoffset_to_charsoffset_cached(buf, eo);
		if (s3run->want_submatches) {
			guint num = g_match_info_get_match_count(match_info);
			DEBUG_MSG("got %d submatches\n",num);
			if (num>1) {
				Tsnr3submatches *tmp;
				tmp = g_slice_alloc(num*sizeof(Tsnr3submatches));
				tmp[num].so=0;
				tmp[num].eo=0;
				num--;
				while (num!=0) {
					g_match_info_fetch_pos(match_info, num, &tmp[num].so, &tmp[num].eo);
					tmp[num].so = utf8_byteoffset_to_charsoffset_cached(buf, tmp[num].so);
					tmp[num].eo = utf8_byteoffset_to_charsoffset_cached(buf, tmp[num].eo);
					DEBUG_MSG("submatch %d so=%d, eo=%d\n",num, tmp[num].so, tmp[num].eo);
					num--;
				}
				extra = tmp;
			}
		}
		sn3run_add_result(s3run, so, eo, doc, extra);
		g_match_info_next (match_info, &gerror);
	}
	g_match_info_free(match_info);
	g_regex_unref(gregex);
	if (gerror) {
		g_print("regex error %s\n",gerror->message);
		/*gchar *errstring;
		errstring = g_strdup_printf(_("Regular expression error: %s"), gerror->message);
		message_dialog_new(s3run->bfwin->main_window,
						   GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, _("Search failed"), errstring);
		g_free(errstring);*/
		g_error_free(gerror);
		return;
	}
	g_print("snr3_run_pcre_in_doc, finished with %d results\n",s3run->results.length);
}

static void snr3_run_string_in_doc(Tsnr3run *s3run, Tdocument *doc, gint so, gint eo) {
	gchar *buf, *result;
	gint querylen;
	char *(*f) ();
	g_print("snr3_run_string_in_doc, s3run=%p, started for query %s\n",s3run, s3run->query);
#ifdef SNR3_PROFILING
	s3profiling.timer = g_timer_new();
	s3profiling.numresults=0;
#endif
	if (s3run->results.head) {
		g_warning("s3run has results already\n");
	}
	result = buf = doc_get_chars(doc, so, eo);
	utf8_offset_cache_reset();
	if (s3run->is_case_sens) {
		f = strstr;
	} else {
		f = strcasestr;
	}
	querylen = g_utf8_strlen(s3run->query, -1);
	do {
		result = f(result, s3run->query);
		if (result) {
			glong char_o = utf8_byteoffset_to_charsoffset_cached(buf, (result-buf));
			sn3run_add_result(s3run, char_o+so, char_o+querylen+so, doc, NULL);
#ifdef SNR3_PROFILING
			s3profiling.numresults++;
#endif
			result++;
		}		
	} while (result);
	

#ifdef SNR3_PROFILING
	g_print("search %ld bytes with %d results run took %f s, %f bytes/s\n",(glong)strlen(buf),s3profiling.numresults,g_timer_elapsed(s3profiling.timer, NULL),strlen(buf)/g_timer_elapsed(s3profiling.timer, NULL));
	g_timer_destroy(s3profiling.timer);
#endif
	g_free(buf);
	g_print("snr3_run_string_in_doc, finished with %d results\n",s3run->results.length);
}

void snr3_run_go(Tsnr3run *s3run, gboolean forward) {
	GList *next=NULL;
	g_print("snr3_run_go, s3run=%p\n",s3run);
	if (s3run->current) {
		if (forward) {
			next = g_list_next(s3run->current);
		} else {
			next = g_list_previous(s3run->current);
		}
	}
	if (!next) {
		next = forward ? s3run->results.head : s3run->results.tail;
	}
	g_print("scroll to result %p\n",next);
	if (next) {
		s3run->current = next;
		scroll_to_result(next->data, NULL);
	}
}


static void snr3_run(Tsnr3run *s3run) {
	gint so,eo;
	GList *tmplist;
	g_print("snr3_run, s3run=%p\n",s3run);
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
			for (tmplist=g_list_first(s3run->bfwin->documentlist);tmplist;tmplist=g_list_next(tmplist)) {
				/* TODO: run in idle loop to avoid blocking the GUI */
				if (s3run->type == snr3type_string) 
					snr3_run_string_in_doc(s3run, s3run->bfwin->current_document, 0, -1);
				else if (s3run->type == snr3type_pcre)
					snr3_run_pcre_in_doc(s3run, s3run->bfwin->current_document, 0, -1);
			}
		break;
		case snr3scope_files:
			/* TODO: implement background file loading and saving for search and replace */
			g_print("TODO: implement background file loading for replace\n");
		break;
	}
}


static void snr3result_free(gpointer s3result, gpointer s3run) {
	/* BUG TODO: for pcre patterns we should free the 'extra' data */
	g_print("free result %p\n",s3result);
	g_slice_free(Tsnr3result, (Tsnr3result *)s3result);
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

gpointer simple_search_run(Tbfwin *bfwin, const gchar *string) {
	Tsnr3run *s3run;
	
	s3run = g_slice_new0(Tsnr3run);
	s3run->bfwin = bfwin;
	/*s3run->query = g_regex_escape_string(string,-1);
	s3run->type = snr3type_pcre;*/
	s3run->query = g_strdup(string);
	DEBUG_MSG("snr3run at %p, query at %p\n",s3run, s3run->query);
	s3run->type = snr3type_string;
	s3run->scope = snr3scope_doc;
	g_queue_init(&s3run->results);
	snr3_run(s3run);
	highlight_run_in_doc(s3run, bfwin->current_document);
	snr3_run_go(s3run, TRUE);
	return s3run;
}


