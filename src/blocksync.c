/* Bluefish HTML Editor
 * blocksync.c - synchronise blocks of text trough all open documents
 *
 * Copyright (C) 2006 Olivier Sessink
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#define DEBUG

#include <gtk/gtk.h>

#include "config.h"

#include "bluefish.h"
#include "document.h"        /* doc_replace_text() */
#include "gtk_easy.h"        /* a lot of GUI functions */
#include "snr2.h"

typedef enum {
	page_no_selection,
	page_start_marker,
	page_end_marker,
	page_summary
} Tbspage;

typedef struct {
	GtkWidget *dialog;
	Tbspage curpage;
	Tbfwin *bfwin;
	GtkWidget *child;
	GtkWidget *textview;
	gchar *allblock;
	gchar *startmarker;
	gchar *endmarker;
} Tbsdialog;

static gboolean single_occurence(gchar *allcontent, gchar *string) {
	/* TODO */
	return TRUE;
}

static void bs_page_no_selection(Tbsdialog *bsdialog) {
	GtkWidget *label = gtk_label_new(_("Select the block to sync to other pages. Include the start-of-block and end-of-block markers in your selection."));
	gtk_label_set_use_markup(GTK_LABEL(label),TRUE);
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(bsdialog->dialog)->vbox), label, TRUE,TRUE,5);
	gtk_widget_show_all(bsdialog->dialog);
	
	bsdialog->child = label;
	bsdialog->curpage = page_no_selection;
}

static void bs_page_start_marker(Tbsdialog *bsdialog, const gchar *text) {
	GtkWidget *vbox, *label, *scrolwin;
	vbox = gtk_vbox_new(FALSE,6);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(bsdialog->dialog)->vbox), vbox, TRUE,TRUE,5);
	
	label	= gtk_label_new(_("Below is the block, including start and end marker. Please <b>select the start marker</b> in the text below."));
	gtk_label_set_use_markup(GTK_LABEL(label),TRUE);
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	gtk_box_pack_start(GTK_BOX(vbox),label,TRUE,TRUE,6);
	
	scrolwin = textview_buffer_in_scrolwin(&bsdialog->textview, 400, 300, text, GTK_WRAP_NONE);
	gtk_box_pack_start(GTK_BOX(vbox),scrolwin,TRUE,TRUE,6);
	gtk_widget_show_all(bsdialog->dialog);
	
	bsdialog->child = vbox;
	bsdialog->curpage = page_start_marker;
}

static void bs_page_end_marker(Tbsdialog *bsdialog, const gchar *text) {
	GtkWidget *vbox, *label, *scrolwin;
	vbox = gtk_vbox_new(FALSE,6);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(bsdialog->dialog)->vbox), vbox, TRUE,TRUE,5);
	
	label	= gtk_label_new(_("Please <b>select the end marker</b> in the text below."));
	gtk_label_set_use_markup(GTK_LABEL(label),TRUE);
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	gtk_box_pack_start(GTK_BOX(vbox),label,TRUE,TRUE,6);
	
	scrolwin = textview_buffer_in_scrolwin(&bsdialog->textview, 400, 300, text, GTK_WRAP_NONE);
	gtk_box_pack_start(GTK_BOX(vbox),scrolwin,TRUE,TRUE,6);
	gtk_widget_show_all(bsdialog->dialog);

	bsdialog->child = vbox;
	bsdialog->curpage = page_end_marker;
}

static void bs_page_summary(Tbsdialog *bsdialog) {
	GtkWidget *label = gtk_label_new(_("<b>Sync this block to all opened documents?</b>"));
	gtk_label_set_use_markup(GTK_LABEL(label),TRUE);
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(bsdialog->dialog)->vbox), label, TRUE,TRUE,5);
	gtk_widget_show_all(bsdialog->dialog);

	bsdialog->child = label;
	bsdialog->curpage = page_summary;
	
}

static void bs_dialog_response_lcb(GtkDialog *dialog, gint response, Tbsdialog *bsdialog) {
	if (bsdialog->curpage == page_no_selection && response == 1) {
		gint start,end;
		if (doc_get_selection(bsdialog->bfwin->current_document,&start,&end)) {
			/* build the new page with the selection text in there */
			bsdialog->allblock = doc_get_chars(bsdialog->bfwin->current_document,start,end);
			gtk_widget_destroy(bsdialog->child);
			bs_page_start_marker(bsdialog, bsdialog->allblock);
		} /* else do nothing */
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
				/* TODO: show warning */
			}
		} /* else do nothing */
	} else if (bsdialog->curpage == page_end_marker && response == 1) {
		GtkTextBuffer *buffer;
		GtkTextIter start,end;
		/* check the textview for the selection */
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(bsdialog->textview));
		if (gtk_text_buffer_get_selection_bounds(buffer,&start,&end)) {
			bsdialog->endmarker = gtk_text_buffer_get_text(buffer,&start,&end,FALSE);
			if (single_occurence(bsdialog->allblock, bsdialog->startmarker)) {
				gtk_widget_destroy(bsdialog->child);
				bs_page_summary(bsdialog);
			} else {
				/* TODO: show warning */
			}
		} /* else do nothing */
	} else if (bsdialog->curpage == page_summary && response == 1) {
		gchar *searchpat;
		/* GO */
		searchpat = g_strconcat(bsdialog->startmarker, ".*", bsdialog->endmarker, NULL);
		/* BUG: what if the start or endmarkers contain special pcre patterns ? */
		snr2_run_extern_replace(bsdialog->bfwin->current_document, searchpat, 3,2,1, bsdialog->allblock,FALSE);
		g_free(searchpat);
		
		/* cleanup */
		gtk_widget_destroy(bsdialog->dialog);
		g_free(bsdialog->startmarker);
		g_free(bsdialog->endmarker);
		g_free(bsdialog->allblock);
		g_free(bsdialog);
	} else {
		DEBUG_MSG("flow broken, response=%d, page=%d\n",response,bsdialog->curpage);
	}
}


void blocksync_dialog(Tbfwin *bfwin) {
	Tbsdialog *bsdialog;
	
	bsdialog = g_new(Tbsdialog,1);
	bsdialog->bfwin = bfwin;
	bsdialog->curpage = page_no_selection;
	bsdialog->dialog = gtk_dialog_new_with_buttons(_("Block synchronisation"),GTK_WINDOW(bfwin->main_window),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
					GTK_STOCK_GO_FORWARD,1,
					NULL);
	gtk_window_set_default_size(GTK_WINDOW(bsdialog->dialog),500,400);

	bs_page_no_selection(bsdialog);
	
	gtk_widget_show_all(bsdialog->dialog);
	g_signal_connect(bsdialog->dialog, "response", G_CALLBACK(bs_dialog_response_lcb), bsdialog);
}



