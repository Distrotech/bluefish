#include <gtk/gtk.h>
#include <string.h> /* strcasecmp() */

/* #define DEBUG */

#include "bluefish.h"
#include "bf_lib.h"
#include "document.h"
#include "html_diag.h"
#include "html.h"
#include "html2.h"
#include "html_table.h"
#include "html_form.h"
#include "image.h"

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
	
	DEBUG_MSG("doc_bevent_in_html_tag, xpos=%d, ypos=%d\n", xpos, ypos);
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
		gtfound = gtk_text_iter_backward_find_char(&gtiter,(GtkTextCharPredicate)iter_char_search_lcb,
                                             &iht,NULL);
		/* perhaps we should limit the search fto 50 charcters or so */
		iht.findchar = 60;/* 60 = < */
		iht.prevchar = 10; /* \n */
		iht.ignore_if_prevchar = 63 /* ? */;
		ltfound = gtk_text_iter_backward_find_char(&ltiter,(GtkTextCharPredicate)iter_char_search_lcb,
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
			gtfound = gtk_text_iter_forward_find_char(&gtiter,(GtkTextCharPredicate)iter_char_search_lcb,
                                             &iht,NULL);
			/* perhaps we should limit the search fto 50 charcters or so */
			iht.findchar = 60;/* 60 = < */
			iht.prevchar = 10; /* \n */
			iht.ignore_if_prevchar = 0;
			ltfound = gtk_text_iter_forward_find_char(&ltiter,(GtkTextCharPredicate)iter_char_search_lcb,
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

static void input_tag_splitter(gpointer nothing, gpointer data)
{
	Ttagpopup *tag_popup;
	GList *tmplist;
	gint counter = 0;
	gchar *itemvalue = NULL;

	tag_popup = (Ttagpopup *) data;
	DEBUG_MSG("input_tag_splitter, started\n");

	tmplist = g_list_first(tag_popup->taglist);
	while (tmplist) {
		if (strcasecmp((gchar *) ((Ttagitem *) (tmplist->data))->item, "type") == 0) {
			DEBUG_MSG("input_tag_splitter, found!! tmplist->data->item=%s, counter=%d\n", (gchar *) ((Ttagitem *) (tmplist->data))->item,
					  counter);
			itemvalue = g_strdup((gchar *) ((Ttagitem *) (tmplist->data))->value);
			break;
		} else {
			DEBUG_MSG("input_tag_splitter, nope.., tmplist->data->item=%s, counter=%d\n", (gchar *) ((Ttagitem *) (tmplist->data))->item,
					  counter);
		}
		counter++;
		tmplist = g_list_next(tmplist);
	}
	if (tmplist) {
		g_strdown(itemvalue);
		DEBUG_MSG("input_tag_splitter, itemvalue=%s, counter=%d\n", itemvalue, counter);
		if (strcmp(itemvalue, "submit") == 0) {
			buttondialog_dialog(NULL, data);
		} else if (strcmp(itemvalue, "reset") == 0) {
			buttondialog_dialog(NULL, data);
		} else if (strcmp(itemvalue, "button") == 0) {
			buttondialog_dialog(NULL, data);
		} else if (strcmp(itemvalue, "text") == 0) {
			textdialog_dialog(NULL, data);
		} else if (strcmp(itemvalue, "hidden") == 0) {
			hiddendialog_dialog(NULL, data);
		} else if (strcmp(itemvalue, "radio") == 0) {
			radiodialog_dialog(NULL, data);
		} else if (strcmp(itemvalue, "checkbox") == 0) {
			checkdialog_dialog(NULL, data);
		}
		g_free(itemvalue);
	} else {
		DEBUG_MSG("input_tag_splitter, not found ?\n");
	}
}


typedef struct {
	gchar *tag;
	void (*func) ();
}Ttagdialog;

static void parse_tagstring(Tbfwin *bfwin, gchar * tagstring, gint pos, gint end)
{
	GList *tmplist = NULL;
	gchar *tmpstring, *item, *value;
	gint count, prevtag, item_value_delimiter;
	Ttagitem *tag_item;
	Ttagpopup *tag_popup;
	gboolean in_quote, has_quotes;
	Ttagdialog tagdia[] = {
		{"body", G_CALLBACK(body_dialog)},
		{"a", G_CALLBACK(quickanchor_dialog)},
		{"p", G_CALLBACK(p_dialog)},
		{"div", G_CALLBACK(div_dialog)},
		{"span", G_CALLBACK(span_dialog)},
		{"h1", G_CALLBACK(h1_dialog)},
		{"h2", G_CALLBACK(h2_dialog)},
		{"h3", G_CALLBACK(h3_dialog)},
		{"h4", G_CALLBACK(h4_dialog)},
		{"h5", G_CALLBACK(h5_dialog)},
		{"h6", G_CALLBACK(h6_dialog)},
		{"hr", G_CALLBACK(quickrule_dialog)},
		{"font", G_CALLBACK(font_dialog)},
		{"basefont", G_CALLBACK(basefont_dialog)},
		{"table", G_CALLBACK(tabledialog_dialog)},
		{"tr", G_CALLBACK(tablerowdialog_dialog)},
		{"th", G_CALLBACK(tableheaddialog_dialog)},
		{"td", G_CALLBACK(tabledatadialog_dialog)},
		{"frameset", G_CALLBACK(frameset_dialog)},
		{"frame",G_CALLBACK(frame_dialog)},
		{"form",G_CALLBACK(formdialog_dialog)},
		{"textarea",G_CALLBACK(textareadialog_dialog)},
		{"select",G_CALLBACK(selectdialog_dialog)},
		{"option",G_CALLBACK(optiondialog_dialog)},
		{"meta",G_CALLBACK(meta_dialog)},
		{"img",G_CALLBACK(image_insert_dialog)}
	};

	DEBUG_MSG("parse_tagstring, started, tagstring=%s\n", tagstring);

	/* parsing the values from this tag */
	tmpstring = g_strdup(tagstring);
	strip_any_whitespace(tmpstring);
	item_value_delimiter = prevtag = count = 0;
	has_quotes = in_quote = FALSE;
	while (tmpstring[count] != '\0') {
		/* extra spacing like between the name = value can be ignored */
		if (g_ascii_isspace((gchar)tmpstring[count])) {
			gint count2 = count+1;
			/* search for the next character, is that a = or a " then we will ignore this space */
			while (tmpstring[count2] != '\0') {
				if (g_ascii_isspace((gchar)tmpstring[count2]) || tmpstring[count2]=='\n') {
					count2++;
				} else {
					if (tmpstring[count2] == '=' || tmpstring[count2] == '"') {
						DEBUG_MSG("found ignorable spaces, increasing count from %d to %d\n", count, count2);
						count = count2;
					}
					break;
				}
			}
		}
		DEBUG_MSG("tmpstring[%d]=%c\n",count,tmpstring[count]);
		/* spaces (delimiters) are allowed within quotes, so we have to keep track of quotes */
		if (tmpstring[count] == '"') {
			has_quotes = TRUE;
			if (in_quote) {
				in_quote = FALSE;
			} else {
				in_quote = TRUE;
			}
		}
		/* to split the item and the value we have to keep track of '=' characters */
		if (tmpstring[count] == '=' && !in_quote) {
			item_value_delimiter = count;
		}
		/* it is a delimiter if it a space (or tab, newline), outside a quote or the last character of the string */
		if ((g_ascii_isspace((gchar)tmpstring[count]) && (in_quote == FALSE)) || (tmpstring[count + 1] == '\0')) {
			if (prevtag == (count - 1)) {
				DEBUG_MSG("parse_tagstring, two spaces!\n");
				prevtag = count;
			} else if (prevtag == 0) {
				DEBUG_MSG("parse_tagstring, this is the name of the tag itself\n");
				prevtag = count;
			} else {
				DEBUG_MSG("parse_tagstring, making split, count=%d, prevtag=%d\n", count, prevtag);
				if (item_value_delimiter > prevtag) {
					item = g_strndup(&tmpstring[prevtag + 1], item_value_delimiter - prevtag - 1);
					DEBUG_MSG("item from %d to %d=%s\n", prevtag+1, item_value_delimiter - prevtag - 1, item);
					if (has_quotes == TRUE) {
						gchar *tmp;
						tmp = g_strndup(&tmpstring[item_value_delimiter + 1], count - item_value_delimiter - 1);
						g_strstrip(tmp);
						value = g_strndup(&tmp[1], strlen(tmp)-1);
						g_free(tmp);
						value = trunc_on_char(value, '"');
						DEBUG_MSG("value from %d to %d=%s", item_value_delimiter + 2, count - item_value_delimiter - 2, value);
					} else {
						value = g_strndup(&tmpstring[item_value_delimiter + 1], count - item_value_delimiter);
						g_strstrip(value);
					}
				} else {
					item = g_strndup(&tmpstring[prevtag + 1], count - prevtag);
					value = g_strdup("");
				}
				g_strdown(item);
				g_strstrip(item);
				tag_item = g_malloc(sizeof(Ttagitem));
				tag_item->item = item;
				tag_item->value = value;
				tmplist = g_list_append(tmplist, tag_item);
				DEBUG_MSG("parse_tagstring, item=%s with value=%s appended to list %p\n", item, value, tmplist);
				prevtag = count;
				has_quotes = FALSE;
			}
		}
		count++;
	}
	g_free(tmpstring);

	tag_popup = g_malloc(sizeof(Ttagpopup));
	tag_popup->taglist = tmplist;
	tag_popup->pos = pos;
	tag_popup->end = end;
	DEBUG_MSG("parse_tagstring, tag_popup->pos=%d, tag_popup->end=%d\n", tag_popup->pos, tag_popup->end);

	tmpstring = g_strdup(tagstring);
	tmpstring = trunc_on_char(tmpstring, ' ');
	g_strdown(tmpstring);
	/* identifying which tag we have */
	DEBUG_MSG("parse_tagstring, searching for dialog for %s\n", tmpstring);
	
	{
	gint i, numitems = (sizeof(tagdia)/sizeof(Ttagdialog));
		for (i=0;i<numitems;i++) {
			if (strcmp(tmpstring, tagdia[i].tag) == 0) {
				tagdia[i].func(bfwin, tag_popup);
				break;
			}
		}
	}
/*	if (strcmp(tmpstring, "card") == 0) {
		carddialog_cb(NULL, tag_popup);
	} else
	if (strcmp(tmpstring, "postfield") == 0) {
		postfielddialog_cb(NULL, tag_popup);
	} else
	if (strcmp(tmpstring, "setvar") == 0) {
		vardialog_cb(NULL, tag_popup);
	} else
	if (strcmp(tmpstring, "go") == 0) {
		godialog_cb(NULL, tag_popup);
	} else
	if (strcmp(tmpstring, "do") == 0) {
		dodialog_cb(NULL, tag_popup);
	} else
	if (strcmp(tmpstring, "anchor") == 0) {
		anchordialog_cb(NULL, tag_popup);
	} else
	if (strcmp(tmpstring, "access") == 0) {
		accessdialog_cb(NULL, tag_popup);
	} else
	if (strcmp(tmpstring, "script") == 0) {
		script_cb(NULL, tag_popup);
	} else
	if (strcmp(tmpstring, "link") == 0) {
		link_cb(NULL, tag_popup);
	} else*/
	if (strcmp(tmpstring, "input") == 0) {
		DEBUG_MSG("parse_tagstring, identified as INPUT tag, splitting tag!\n");
		input_tag_splitter(NULL, tag_popup);
	}
	tmplist = g_list_first(tmplist);
	while (tmplist) {
		g_free(((Ttagitem *) tmplist->data)->item);
		g_free(((Ttagitem *) tmplist->data)->value);
		g_free(tmplist->data);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(tmplist);
	g_free(tag_popup);
	g_free(tmpstring);
}


void rpopup_edit_tag_cb(GtkMenuItem *menuitem,Tdocument *doc) {
	if (rec_tag.doc == doc && rec_tag.found_tag) {
		gchar *text;
		DEBUG_MSG("rpopup_edit_tag_cb, get text from %d to %d\n", rec_tag.tag_so, rec_tag.tag_eo);
		/* we do not need the < and > chars so we cut those */
		text = doc_get_chars(doc, rec_tag.tag_so+1, rec_tag.tag_eo - 1);
		DEBUG_MSG("rpopup_edit_tag_cb, about to parse %s\n", text);
		parse_tagstring(doc->bfwin, text, rec_tag.tag_so, rec_tag.tag_eo);
		g_free(text);
	} else {
		DEBUG_MSG("rpopup_edit_tag_cb, no tag search known!!\n");
	}
}
