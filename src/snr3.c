#include <string.h>

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
	GtkTextIter itstart;

	gtk_text_buffer_get_iter_at_offset(DOCUMENT(s3result->doc)->buffer, &itstart, s3result->so);
	gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(DOCUMENT(s3result->doc)->view), &itstart, 0.25, FALSE, 0.5, 0.10);
	if (dialog) {
		move_window_away_from_cursor(doc, dialog, &itstart);
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
	tag = gtk_text_tag_table_lookup(tagtable, "snr2match");
	if (!tag)
		return;
	
	gtk_text_buffer_get_bounds(doc->buffer, &itstart, &itend);
	gtk_text_buffer_remove_tag(doc->buffer, tag, &itstart, &itend);
}

static void highlight_result(Tsnr3result *s3result) {
	GtkTextIter itstart, itend;
	static const gchar *tagname = "snr2match";
	static const GtkTextTag *tag=NULL;
	
	gtk_text_buffer_get_iter_at_offset(s3result->doc->buffer, &itstart, s3result->so);
	gtk_text_buffer_get_iter_at_offset(s3result->doc->buffer, &itend, s3result->eo);

	if (!tag) {
		tag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(doc->buffer), tagname);
		if (!tag) {
			tag =
				gtk_text_buffer_create_tag(doc->buffer, tagname, "background", "#FF0000", "foreground", "#000000",
										   NULL);
		}
	}
	gtk_text_buffer_apply_tag(doc->buffer, tag, &itstart, &itend);
}

static void highlight_run_in_doc(Tsnr3run *s3run, Tdocument *doc) {
	GList *tmplist;
	
	for (tmplist=s3run->results->head;tmplist;tmplist=g_list_next(tmplist)) {
		Tsnr3result *s3result = tmplist->data;
		if (s3result->doc == doc) {
			highlight_result(s3result);
		}
	}
	
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


static void snr3_run_string_in_doc(Tsnr3run *s3run, Tdocument *doc, gint so, gint eo) {
	gchar *buf, *result;
	gint querylen;
	char *(*f) ();
	
	if (s3run->results) {
		g_warning("s3run has results already\n");
	}
	result = buf = doc_get_chars(doc, so, eo);
	
	if (s3run->is_case_sens) {
		f = strstr;
	} else {
		f = strcasestr;
	}
	querylen = g_utf8_strlen(s3run->query);
	do {
		result = f(result, s3run->query);
		if (result) {
			glong char_o = utf8_byteoffset_to_charsoffset_cached(buf, result-buf);
			sn3run_add_result(s3run, char_o+so, char_o+querylen+so, doc, NULL);
		}
	} while (result);
	
	g_free(buf);
}

void snr3_run_go(Tsnr3run *s3run, gboolean forward) {
	GList *next=NULL;
	if (s3run->current) {
		if (forward) {
			next = g_list_next(s3run->current);
		} else {
			next = g_list_previous(s3run->current);
		}
	}
	if (!next) {
		next = (forward ? s3run->results->head : s3run->results->tail;
	}
	if (next) {
		scroll_to_result(next->data);
	}
}

static void snr3_run_string(Tsnr3run *s3run) {
	gint so,eo;
	GList *tmplist;
	switch(s3run->scope) {
		case snr3scope_doc:
			snr3_run_string_in_doc(s3run, s3run->bfwin->current_document, 0, -1);
		break;
		case snr3scope_cursor:
			so = doc_get_cursor_position(s3run->bfwin->current_document);
			snr3_run_string_in_doc(s3run, s3run->bfwin->current_document, so, -1);
		break;
		case snr3scope_selection:
			if (doc_get_selection(bfwin->current_document, &so, &eo)) {
				snr3_run_string_in_doc(s3run, s3run->bfwin->current_document, so, eo);
			} else {
				g_print("Find in selection, but there is no selection\n");
			}
		break;
		case snr3scope_alldocs:
			for (tmplist=g_list_first(bfwin->documentlist);tmplist;tmplist=g_list_next(tmplist)) {
				/* TODO: run in idle loop to avoid blocking the GUI */
				snr3_run_string_in_doc(s3run, DOCUMENT(tmplist->data), 0, -1);
			}
		break;
		case snr3scope_files:
			/* TODO: implement background file loading and saving for search and replace */
		
		break;
	}	
}

static void snr3result_free(Tsnr3result *s3result, Tsnr3run *s3run) {
	/* BUG TODO: for pcre patterns we should free the 'extra' data */
	g_slice_free(Tsnr3result, s3result);
}

void snr3run_free(Tsnr3run *s3run) {
	g_free(s3run->query);
	g_free(s3run->replace);
	g_queue_foreach(&s3run->results, snr3result_free,s3run);
	g_slice_free(Tsnr3run, s3run);
}

gpointer simple_search_run(Tbfwin *bfwin, const gchar *string) {
	Tsnr3run *s3run;
	
	s3run = g_slice_new0(Tsnr3run);
	s3run->bfwin = bfwin;
	s3run->query = g_strdup(string);
	s3run->type = snr3type_string;
	s3run->scope = snr3scope_doc;
	g_queue_init(&s3run->results);
	snr3_run_string(s3run);
	snr_run_go(s3run, TRUE);
	return s3run;
}


