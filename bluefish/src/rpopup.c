#include <gtk/gtk.h>

#include "bluefish.h"

typedef struct {
	guint32 findchar;
	guint32 prevchar;
	guint32 ignore_if_prevchar;
} Tin_html_tag;

typedef struct {
	Tdocument *doc;
	gint tag_so;
	gint tag_eo;
	gboolean found_tag;
} Trecent_tag;

/********************************/
/* a global var for this module */
/********************************/

Trecent_tag rec_tag;

/*****************/
/* the functions */
/*****************/

static gboolean iter_char_search_lcb(gunichar ch,Tin_html_tag *iht){
/* TODO: a </tag> should not count as editable tag!!! */

	if (ch==iht->findchar && iht->prevchar != iht->ignore_if_prevchar) {
		return TRUE;
	}
	iht->prevchar = ch;
	return FALSE;
}

/* TODO: a </tag> should not count as editable tag!!! */
gboolean doc_bevent_in_html_tag(Tdocument *doc, GdkEventButton *bevent) {
	gint xpos, ypos;
	GtkTextWindowType wintype;
	GtkTextIter iter;

	wintype = gtk_text_view_get_window_type(GTK_TEXT_VIEW(doc->view), doc->view->window);
	gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(doc->view), wintype,bevent->x, bevent->y,
					  &xpos, &ypos);

	rec_tag.tag_so = rec_tag.tag_eo = -1;
	rec_tag.doc = doc;
	
	g_print("doc_bevent_in_html_tag, xpos=%d, ypos=%d\n", xpos, ypos);
	gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(doc->view), &iter, xpos, ypos);
	{
		GtkTextIter gtiter, ltiter;
		gboolean ltfound, gtfound;
		Tin_html_tag iht;
		gtiter = ltiter = iter;

		/* backward search for tag start */
		iht.findchar = 62;/* 62 = > */
		iht.prevchar = 10; /* \n */
		iht.ignore_if_prevchar = 0;
		gtfound = gtk_text_iter_backward_find_char(&gtiter,iter_char_search_lcb,
                                             &iht,NULL);
		/* perhaps we should limit the search fto 50 charcters or so */
		iht.findchar = 60;/* 60 = < */
		iht.prevchar = 10; /* \n */
		iht.ignore_if_prevchar = 63 /* ? */;
		ltfound = gtk_text_iter_backward_find_char(&ltiter,iter_char_search_lcb,
                                             &iht,NULL);
		if ((ltfound && gtfound && gtk_text_iter_compare(&ltiter,&gtiter) > 0)
				|| (ltfound && !gtfound)) {
			rec_tag.tag_so = gtk_text_iter_get_offset(&ltiter);
			DEBUG_MSG("a tag is started on the left side at %d\n",rec_tag.tag_so);
		} else {
			DEBUG_MSG("no tag start found on the left side\n");
		}

		if (rec_tag.tag_so >=0) {
			/* forward search for end tag */
			iht.findchar = 62;/* 62 = > */
			iht.prevchar = 10; /* \n */
			iht.ignore_if_prevchar = 63 /* ? */;
			gtfound = gtk_text_iter_forward_find_char(&gtiter,iter_char_search_lcb,
                                             &iht,NULL);
			/* perhaps we should limit the search fto 50 charcters or so */
			iht.findchar = 60;/* 60 = < */
			iht.prevchar = 10; /* \n */
			iht.ignore_if_prevchar = 0;
			ltfound = gtk_text_iter_forward_find_char(&ltiter,iter_char_search_lcb,
                                             &iht,NULL);
			if ((ltfound && gtfound && gtk_text_iter_compare(&ltiter,&gtiter) > 0)
				|| (gtfound && !ltfound)) {
				rec_tag.tag_eo = gtk_text_iter_get_offset(&gtiter)+1;
				DEBUG_MSG("a tag is ended on the right side at %d\n",rec_tag.tag_eo);
				rec_tag.found_tag = TRUE;
				return TRUE;
			} else {
				DEBUG_MSG("no tag end found on the right side\n");
			}
		}
	}
	return FALSE;
}

void rpopup_edit_tag_cb(GtkMenuItem *menuitem,Tdocument *doc) {
	if (rec_tag.doc == doc && rec_tag.found_tag) {
		gchar *text;
		DEBUG_MSG("rpopup_edit_tag_cb, get text from %d to %d\n", rec_tag.tag_so, rec_tag.tag_eo - rec_tag.tag_so);
		text = doc_get_chars(doc, rec_tag.tag_so, rec_tag.tag_eo - rec_tag.tag_so);
		DEBUG_MSG("rpopup_edit_tag_cb, about to parse %s\n", text);
		
		g_free(text);
	} else {
		DEBUG_MSG("rpopup_edit_tag_cb, no tag search known!!\n");
	}
}
