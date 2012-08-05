/* Bluefish HTML Editor
 * blocksync.c - synchronise blocks of text trough all open documents
 *
 * Copyright (C) 2006 Olivier Sessink
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
/*#define DEBUG*/

#include <gtk/gtk.h>
#include <string.h>
#include "config.h"

#include "bluefish.h"
#include "document.h"        /* doc_replace_text() */
#include "gtk_easy.h"        /* a lot of GUI functions */
#include "snr3.h"

typedef enum {
	page_no_selection,
	page_start_marker,
	page_end_marker,
	page_summary
} Tbspage;

typedef struct {
	GtkWidget *dialog;
	GtkWidget *warnlabel;
	Tbspage curpage;
	Tbfwin *bfwin;
	GtkWidget *child;
	GtkWidget *textview;
	gchar *allblock;
	gchar *startmarker;
	gchar *endmarker;
} Tbsdialog;

static gboolean single_occurence(gchar *allcontent, gchar *needle) {
	gchar *first;
	/* the string should be only once in allcontent */
	first = strstr(allcontent, needle);
	if (first) {
		gchar *second = strstr(first+1, needle);
		if (second == NULL) {
			return TRUE;
		}
		DEBUG_MSG("single_occurence, error, needle %s exists twice!, first=%p, second=%p\n",needle,first,second);
	}
	return FALSE;
}

static gchar *pcre_escape_string(const gchar *orig) {
	gint len,i=0,o=0;
	gchar *new;
	
	len = strlen(orig);
	new = g_new(gchar,len*2+1);
	while (i<=len) {
		if (orig[i] == '.' || orig[i] == '?' || orig[i] == '+' || orig[i] == '*'
				|| orig[i] == '(' || orig[i] == ')' || orig[i] == '[' || orig[i] == ']'
				|| orig[i] == '\\' || orig[i] == '^' || orig[i] == '$') {
			new[o] = '\\';
			o++;
		}
		new[o] = orig[i];
		i++;
		o++;
	}
	DEBUG_MSG("pcre_escape_string, orig=%s, new=%s\n",orig,new);
	return new;
}

static void bs_page_no_selection(Tbsdialog *bsdialog) {
	GtkWidget *label, *vbox;
	gchar *tmp1;
	const gchar *stext = _("Select the block to sync to other pages. Include the start-of-block and end-of-block markers in your selection.");	
	
	vbox = gtk_vbox_new(FALSE,6);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(bsdialog->dialog))), vbox, TRUE,TRUE,5);
	if (doc_has_selection(bsdialog->bfwin->current_document)) {
		tmp1 = g_strconcat(stext, " ", _("To proceed with the current selection click forward."), NULL);
	} else {
		tmp1 = g_strdup(stext);
	}
	
	label = gtk_label_new(tmp1);
	g_free(tmp1);
	
	gtk_label_set_use_markup(GTK_LABEL(label),TRUE);
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE,TRUE,5);
	
	gtk_widget_show_all(bsdialog->dialog);
	bsdialog->warnlabel = gtk_label_new("");	
	gtk_box_pack_start(GTK_BOX(vbox), bsdialog->warnlabel, FALSE,TRUE,5);
	
	
	bsdialog->child = vbox;
	bsdialog->curpage = page_no_selection;
}

static void bs_page_start_marker(Tbsdialog *bsdialog, const gchar *text) {
	GtkWidget *vbox, *label, *scrolwin;
	GtkTextBuffer *buffer;
	GtkTextIter start,end;
	GtkTextMark *mark;
	vbox = gtk_vbox_new(FALSE,6);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(bsdialog->dialog))), vbox, TRUE,TRUE,5);
	
	label	= gtk_label_new(_("Below is the block, including start and end marker. Please <b>select the start marker</b> in the text below."));
	gtk_label_set_use_markup(GTK_LABEL(label),TRUE);
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,TRUE,6);
	
	bsdialog->warnlabel = gtk_label_new("");	
	gtk_box_pack_start(GTK_BOX(vbox), bsdialog->warnlabel, FALSE,TRUE,6);	
	
	scrolwin = textview_buffer_in_scrolwin(&bsdialog->textview, -1, -1, text, GTK_WRAP_NONE);
	gtk_box_pack_start(GTK_BOX(vbox),scrolwin,TRUE,TRUE,6);
	gtk_widget_show(bsdialog->textview);
	gtk_widget_show(label);
	gtk_widget_show(scrolwin);
	gtk_widget_show(vbox);
	
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(bsdialog->textview));
	gtk_text_buffer_get_start_iter(buffer,&start);
	end = start;
	gtk_text_iter_forward_to_line_end(&end);
	gtk_text_buffer_select_range(buffer,&start,&end);
	mark = gtk_text_buffer_get_selection_bound(buffer);
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(bsdialog->textview),mark);
	
	bsdialog->child = vbox;
	bsdialog->curpage = page_start_marker;
}

static void bs_page_end_marker(Tbsdialog *bsdialog, const gchar *text) {
	GtkWidget *vbox, *label, *scrolwin;
	GtkTextBuffer *buffer;
	GtkTextIter start,end;
	GtkTextMark *mark;
	vbox = gtk_vbox_new(FALSE,6);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(bsdialog->dialog))), vbox, TRUE,TRUE,5);
	
	label	= gtk_label_new(_("Please <b>select the end marker</b> in the text below."));
	gtk_label_set_use_markup(GTK_LABEL(label),TRUE);
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,TRUE,6);
	
	bsdialog->warnlabel = gtk_label_new("");	
	gtk_box_pack_start(GTK_BOX(vbox), bsdialog->warnlabel, FALSE,TRUE,6);
	
	scrolwin = textview_buffer_in_scrolwin(&bsdialog->textview, -1, -1, text, GTK_WRAP_NONE);
	gtk_box_pack_start(GTK_BOX(vbox),scrolwin,TRUE,TRUE,6);
	gtk_widget_show(bsdialog->textview);
	gtk_widget_show(label);
	gtk_widget_show(scrolwin);
	gtk_widget_show(vbox);
	
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(bsdialog->textview));
	gtk_text_buffer_get_end_iter(buffer,&end);
	start = end;
	gtk_text_iter_set_line_offset(&start,0);
	if (gtk_text_iter_equal(&start,&end)) {
		/* if the last line is just a newline, we select the line before */
		gtk_text_iter_backward_line(&start);
		gtk_text_iter_set_line_offset(&start,0);
	}
	gtk_text_buffer_select_range(buffer,&start,&end);
	mark = gtk_text_buffer_get_selection_bound(buffer);
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(bsdialog->textview),mark);
	
	bsdialog->child = vbox;
	bsdialog->curpage = page_end_marker;
}

static void bs_page_summary(Tbsdialog *bsdialog) {
	gchar *tmp, *tmp1, *tmp2;
	GtkWidget *label;

	tmp1 = g_markup_escape_text(bsdialog->startmarker,-1);
	tmp2 = g_markup_escape_text(bsdialog->endmarker,-1);
	tmp = g_strdup_printf(_("Will replace the text between <i>%s</i> and <i>%s</i> in all documents opened in this window.\n<b>Proceed?</b>"), tmp1,tmp2);
	label = gtk_label_new(tmp);
	g_free(tmp);
	g_free(tmp1);
	g_free(tmp2);
	gtk_label_set_use_markup(GTK_LABEL(label),TRUE);
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(bsdialog->dialog))), label, TRUE,TRUE,5);
	gtk_widget_show_all(bsdialog->dialog);

	bsdialog->child = label;
	bsdialog->curpage = page_summary;
}

static void bsdialog_cleanup(Tbsdialog *bsdialog) {
	gtk_widget_destroy(bsdialog->dialog);
	if (bsdialog->startmarker) g_free(bsdialog->startmarker);
	if (bsdialog->endmarker) g_free(bsdialog->endmarker);
	if (bsdialog->allblock) g_free(bsdialog->allblock);
	g_free(bsdialog);
}

static void bs_dialog_response_lcb(GtkDialog *dialog, gint response, Tbsdialog *bsdialog) {
	if (bsdialog->curpage == page_no_selection && response == 1) {
		gint start,end;
		if (doc_get_selection(bsdialog->bfwin->current_document,&start,&end)) {
			/* build the new page with the selection text in there */
			bsdialog->allblock = doc_get_chars(bsdialog->bfwin->current_document,start,end);
			gtk_widget_destroy(bsdialog->child);
			bs_page_start_marker(bsdialog, bsdialog->allblock);
		} else {
			gtk_label_set_markup(GTK_LABEL(bsdialog->warnlabel), _("<span foreground=\"red\" weight=\"bold\">Please select the block of text to synchronize.</span>"));
			gtk_widget_show(bsdialog->warnlabel);
		}
	} else if (bsdialog->curpage == page_start_marker && response == 1) {
		GtkTextBuffer *buffer;
		GtkTextIter start,end;
		/* check the textview for the selection */
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(bsdialog->textview));
		if (gtk_text_buffer_get_selection_bounds(buffer,&start,&end)) {
			bsdialog->startmarker = gtk_text_buffer_get_text(buffer,&start,&end,FALSE);
			if (single_occurence(bsdialog->allblock, bsdialog->startmarker)) {
				gtk_widget_destroy(bsdialog->child);
				bs_page_end_marker(bsdialog, bsdialog->allblock);
			} else {
				gtk_label_set_markup(GTK_LABEL(bsdialog->warnlabel), _("<span foreground=\"red\" weight=\"bold\">This marker exists more than once in the block.</span>"));
				gtk_widget_show(bsdialog->warnlabel);
				g_free(bsdialog->startmarker);
				bsdialog->startmarker = NULL;
			}
		} else {
			gtk_label_set_markup(GTK_LABEL(bsdialog->warnlabel), _("<span foreground=\"red\" weight=\"bold\">Select the text that marks the start of the block.</span>"));
			gtk_widget_show(bsdialog->warnlabel);
		}
	} else if (bsdialog->curpage == page_end_marker && response == 1) {
		GtkTextBuffer *buffer;
		GtkTextIter start,end;
		/* check the textview for the selection */
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(bsdialog->textview));
		if (gtk_text_buffer_get_selection_bounds(buffer,&start,&end)) {
			bsdialog->endmarker = gtk_text_buffer_get_text(buffer,&start,&end,FALSE);
			if (single_occurence(bsdialog->allblock, bsdialog->endmarker)) {
				gtk_widget_destroy(bsdialog->child);
				bs_page_summary(bsdialog);
			} else {
				gtk_label_set_markup(GTK_LABEL(bsdialog->warnlabel), _("<span foreground=\"red\" weight=\"bold\">This marker exists more than once in the block.</span>"));
				gtk_widget_show(bsdialog->warnlabel);
				g_free(bsdialog->endmarker);
				bsdialog->endmarker = NULL;
			}
		}  else {
			gtk_label_set_markup(GTK_LABEL(bsdialog->warnlabel), _("<span foreground=\"red\" weight=\"bold\">Select the text that marks the end of the block.</span>"));
			gtk_widget_show(bsdialog->warnlabel);
		}
	} else if (bsdialog->curpage == page_summary && response == 1) {
		gchar *searchpat, *tmp1, *tmp2;
		/* GO */
		tmp1 = pcre_escape_string(bsdialog->startmarker);
		tmp2 = pcre_escape_string(bsdialog->endmarker);
		searchpat = g_strconcat(tmp1, ".*?",tmp2 , NULL);
		DEBUG_MSG("searchpat=%s\n",searchpat);
		g_free(tmp1);
		g_free(tmp2);
		snr3_run_extern_replace(bsdialog->bfwin->current_document, searchpat, snr3scope_alldocs,snr3type_pcre,TRUE, bsdialog->allblock,FALSE, TRUE);
		g_free(searchpat);
		
		/* cleanup */
		bsdialog_cleanup(bsdialog);
	} else if (response == -2 || response == -4){
		bsdialog_cleanup(bsdialog);
	} else{
		DEBUG_MSG("flow broken, response=%d, page=%d\n",response,bsdialog->curpage);
	}
}

void blocksync_dialog(Tbfwin *bfwin) {
	Tbsdialog *bsdialog;
	
	bsdialog = g_new0(Tbsdialog,1);
	bsdialog->bfwin = bfwin;
	bsdialog->curpage = page_no_selection;
	bsdialog->dialog = gtk_dialog_new_with_buttons(_("Synchronize text block"),GTK_WINDOW(bfwin->main_window),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
					GTK_STOCK_GO_FORWARD,1,
					NULL);
	gtk_window_set_default_size(GTK_WINDOW(bsdialog->dialog),400,300);

	bs_page_no_selection(bsdialog);
	
	gtk_widget_show_all(bsdialog->dialog);
	g_signal_connect(bsdialog->dialog, "response", G_CALLBACK(bs_dialog_response_lcb), bsdialog);
}
