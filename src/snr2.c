/* Bluefish HTML Editor
 * snr2.c - rewrite of search 'n replace functions
 *
 * Copyright (C) 2000,2001,2002,2003,2004 Olivier Sessink
 * Copyright (C) 2005-2007 James Hayward and Olivier Sessink
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
/*
 * THIS IS THE BACKEND FLOW DIAGRAM
 *
 *                              snr2_run
 *              ________________/ | \  \___________________________
 *             /      / |         |  \                   \  \      \
 *  search_multiple  |  |         |  replace_prompt_all  |   \      \
 *     |            /   |         |         /           /     \      \
 *     |   search_all   |   _____replace_prompt_doc    /       \    replace_all
 *     |     |          |  /          /               /         \     /
 *     |     \   doc_show_result     /  replace_doc_once   replace_doc_multiple
 *     |      \                   __/                  \         /
 *     |  search_doc             /                   replace_backend
 *      \________ \             /  ___________________________/
 *               \ \           /  /
 *                 search_backend
 */
/*****************************************************/

/* #define DEBUG */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>    /* GDK_Return */

#include "config.h"

#include <pcre.h>             /* pcre_compile */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>        /* _before_ regex.h for freeBSD */
#include <regex.h>            /* regcomp() */

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif /* HAVE_STRINGS_H */

#ifndef PCRE_UTF8             /* for compatibility with older libpcre's */
#define PCRE_UTF8 0
#endif /* PCRE_UTF8 */

#include "bluefish.h"
#include "bf_lib.h"
#include "bookmark.h"        /* bmark_add_extern() */
#include "dialog_utils.h"
#include "document.h"        /* doc_replace_text() */
#include "gtk_easy.h"        /* a lot of GUI functions */
#include "gui.h"             /* switch_to_document_by_pointer() */
#include "snr2.h"
#include "stringlist.h"      /* add_to_history_stringlist */
#include "undo_redo.h"       /* doc_unre_new_group */

/* Updates, May 2003, by Ruben Dorta */

typedef enum { string, uppercase, lowercase } Treplace_types;
typedef enum { match_normal, match_word, match_posix, match_perl } Tmatch_types;
typedef enum { beginning, cursor, selection, opened_files } Tplace_types;


typedef struct {
	Tbfwin *bfwin;
	Tdocument *doc;
	Tsearch_result result;
	gint replace;
	gchar *search_pattern;
	gchar *replace_pattern;
	gint unescape;
	gint overlapping_search;
	gint is_case_sens;
	gint bookmark_results;
	Treplace_types replacetype_option;
	Tmatch_types matchtype_option;
	Tplace_types placetype_option;
	gint endpos; /* if placetype==selection, this is used to mark the end of the block */
	gint matches; /* number of matches found */
	gint replaces; /* number of matches replaced */
} Tlast_snr2;

#define LASTSNR2(var) ((Tlast_snr2 *)(var))
/***********************************************************/


void snr2_init(Tbfwin *bfwin) {
	Tlast_snr2 *lsnr2 = g_new0(Tlast_snr2,1);
	lsnr2->bfwin = bfwin;
	bfwin->snr2 = lsnr2;
}

void snr2_cleanup(Tbfwin *bfwin) {
	g_free(bfwin->snr2);
}

static void snr2_doc_remove_highlight(Tdocument *doc) {
	if (doc) {
		GtkTextIter itstart, itend;
		DEBUG_MSG("snr2_doc_remove_highlight, removing tag snr2match from doc %p\n",doc);
		gtk_text_buffer_get_bounds(doc->buffer,&itstart,&itend);
		gtk_text_buffer_remove_tag_by_name(doc->buffer,"snr2match",&itstart,&itend);
	}
}

static void move_window_away_from_cursor(Tdocument *doc, GtkWindow *win, GtkTextIter *iter) {
	GdkRectangle winrect;
	GdkRectangle itrect;

	/* get window coordinates, try to include the decorations */
	gdk_window_get_frame_extents(GTK_WIDGET(win)->window,&winrect);

	doc_get_iter_location(doc, iter, &itrect);	
	DEBUG_MSG("move_window_away_from_cursor, itx=%d-%d,ity=%d-%d, winx=%d-%d, winy=%d-%d\n",itrect.x,itrect.x+itrect.width,itrect.y,itrect.y+itrect.height,winrect.x,winrect.x+winrect.width,winrect.y,winrect.y+winrect.height);
	if (itrect.x+itrect.width > winrect.x && itrect.x < winrect.x+winrect.width 
				&& itrect.y + itrect.height > winrect.y && itrect.y < winrect.y+winrect.height ) {
		if (itrect.y > winrect.height+48) { /* the 48 is there to avoid crashing into a top-menu-bar*/
			DEBUG_MSG("move_window_away_from_cursor, move window up to %d,%d\n",winrect.x,itrect.y-winrect.height);
			gtk_window_move(win,winrect.x,itrect.y-winrect.height-10); /* add pixels 10 spacing */
		} else {
			DEBUG_MSG("move_window_away_from_cursor, move window down to %d,%d\n",winrect.x,itrect.y+itrect.height);
			gtk_window_move(win,winrect.x,itrect.y+itrect.height+10); /* add pixels 10 spqacing */
		}
	}
}

static void snr2_doc_highlight_match(Tdocument *doc, GtkWindow *dialog, gint start, gint end) {
	const gchar * tagname =  "snr2match";
	GtkTextIter itstart, itend;
	GdkRectangle visirect;
	GtkTextIter visi_so, visi_eo;
	GtkTextTag *tag;

	gtk_text_buffer_get_iter_at_offset(doc->buffer, &itstart,start);
	gtk_text_buffer_get_iter_at_offset(doc->buffer, &itend,end);
	
	tag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(doc->buffer), tagname);
	if (!tag) {
		tag = gtk_text_buffer_create_tag(doc->buffer, tagname, "background", "#FF0000", "foreground", "#000000", NULL);
	}  
	gtk_text_buffer_apply_tag(doc->buffer, tag, &itstart, &itend);

	gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(doc->view),&visirect);
	gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(doc->view), &visi_so, visirect.x, visirect.y);
	gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(doc->view), &visi_eo, visirect.x + visirect.width, visirect.y + visirect.height);

	if (!gtk_text_iter_in_range(&itstart,&visi_so,&visi_eo)) {
		gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(doc->view),&itstart,0.0,TRUE,0.5,0.10);
	}
	if (dialog) {
		move_window_away_from_cursor(doc, dialog, &itstart);
	}
}

/* static void reset_last_snr2(Tbfwin *bfwin) {
	if (LASTSNR2(bfwin->snr2)->search_pattern) {
		g_free(LASTSNR2(bfwin->snr2)->search_pattern);
	}
	if (LASTSNR2(bfwin->snr2)->replace_pattern) {
		g_free(LASTSNR2(bfwin->snr2)->replace_pattern);
	}
	memset(LASTSNR2(bfwin->snr2), 0, sizeof(Tlast_snr2));
} */

/***********************************************************/

/**
 * search_backend:
 * @bfwin: #Tbfwin* 
 * @search_pattern: #gchar* to search pattern
 * @matchtype: see #Tmatch_types
 * @is_case_sens: If the search is case sensitive, #gint
 * @buf: #gchar* to the document buffer
 * @byte_offset: #guint where in the buffer the search should start, in bytes, not characters
 * @want_submatches: #gint
 * 
 * Performs an actual search in a supplied buffer (#gchar*, aka string).
 * NOTE: If want_submatches is set, tsearch_result->pmatch should be free`ed by the calling function!
 *
 * Return value: #Tsearch_result, contains both character and byte offsets, 
 * they are absolute values to the start of the provided buffer . Note values for start/end are set to -1 on error.
 **/
Tsearch_result search_backend(Tbfwin *bfwin, gchar *search_pattern, Tmatch_types matchtype, gint is_case_sens, gchar *buf, guint byte_offset, gboolean want_submatches) {
	Tsearch_result returnvalue;
	int (*f) ();
	gint buflen, patlen, match, i;

	returnvalue.start = -1;
	returnvalue.end = -1;
	returnvalue.bstart = -1;
	returnvalue.bend = -1;
	returnvalue.pmatch = NULL;
	returnvalue.nmatch = 0;
	if ((!search_pattern) || (!buf)) {
		DEBUG_MSG("search_backend, search_pattern or buf is NULL\n");
		return returnvalue;
	}
	DEBUG_MSG("search_backend, starting for byte_offset=%u\n",byte_offset);
	if (matchtype == match_posix) {
		/* regex part start */
		regex_t reg_pat;
		regmatch_t *pmatch;
		gint nmatch, retval;

		retval = regcomp(&reg_pat, search_pattern, (is_case_sens ? REG_EXTENDED : REG_EXTENDED | REG_ICASE));
		DEBUG_MSG("search_backend, regcomp retval=%d\n", retval);
		if (retval != 0) {
#define ERRORSTR_SIZE 300
			gchar errorstr[ERRORSTR_SIZE], *errorstr2;
			
			regerror(retval,  &reg_pat, errorstr, ERRORSTR_SIZE);
			errorstr2 = g_strconcat(_("Regular expression error: "), errorstr, NULL);
			message_dialog_new(bfwin->main_window, 
									 GTK_MESSAGE_WARNING, 
							 	 	 GTK_BUTTONS_OK, 
									 _("Search failed"), 
									 errorstr2);
			g_free(errorstr2);
			/* error compiling the search_pattern, returning the default result set,
			which is the 'nothing found' set */
			return returnvalue;
		}
		nmatch = (want_submatches) ? reg_pat.re_nsub+1 : 1;
		DEBUG_MSG("search_backend, expr. contains %d sub search_patterns\n", reg_pat.re_nsub );
		pmatch = g_malloc(nmatch*sizeof(regmatch_t));
		retval = regexec(&reg_pat, buf+byte_offset, nmatch, pmatch, 0);
		DEBUG_MSG("search_backend, regexec retval=%d\n", retval);
		if (retval != 0) {
			returnvalue.bstart = -1;
			returnvalue.bend = -1;
		} else {
			returnvalue.bstart = pmatch[0].rm_so + byte_offset;
			returnvalue.bend = pmatch[0].rm_eo + byte_offset;
		}
#ifdef DEBUG
		{	int i;
			for (i=0;i<nmatch;i++) {
				DEBUG_MSG("search_backend, sub search_pattern %d so=%d, eo=%d\n", i, pmatch[i].rm_so, pmatch[i].rm_eo);
			}
		}
#endif
		regfree(&reg_pat);
		if (want_submatches) {
			returnvalue.pmatch = pmatch;
			returnvalue.nmatch = nmatch;
			/* if want_submatches is set, pmatch should be 
			free`ed by the calling function! */
		} else {
			g_free(pmatch);
		}
		/* regex part end */
	} else if (matchtype == match_perl) {
		pcre *pcre_c;
		const char *err=NULL;
		int erroffset=0;
		int ovector[30];
		gint retval;
		pcre_c = pcre_compile(search_pattern, (is_case_sens ? PCRE_UTF8|PCRE_DOTALL|PCRE_MULTILINE : PCRE_UTF8|PCRE_DOTALL|PCRE_CASELESS|PCRE_MULTILINE),&err,&erroffset,NULL);
		if (err) {
			gchar *errstring;
			errstring = g_strdup_printf(_("Regular expression error: %s at offset %d"), err, erroffset);
			message_dialog_new(bfwin->main_window, 
									 GTK_MESSAGE_WARNING, 
							 	 	 GTK_BUTTONS_OK, 
									 _("Search failed"), 
									 errstring);
			g_free(errstring);
			return returnvalue;/* error compiling the search_pattern, returning the default result set,which is the 'nothing found' set */
		}
		retval = pcre_exec(pcre_c,NULL,buf+byte_offset,strlen(buf+byte_offset),0,0,ovector,30);
		if (retval > 0) {
			returnvalue.bstart = ovector[0] + byte_offset;
			returnvalue.bend = ovector[1] + byte_offset;
		} else {
			returnvalue.bstart = -1;
			returnvalue.bend = -1;
		}
		if (want_submatches) {
			int nmatch,i;
			regmatch_t *pmatch;
			pcre_fullinfo(pcre_c, NULL, PCRE_INFO_CAPTURECOUNT, &nmatch);
			DEBUG_MSG("search_backend, want_submatches=%d, nmatch=%d, retval=%d\n", want_submatches, nmatch, retval);
			pmatch = g_malloc((nmatch+1)*sizeof(regmatch_t));
			for (i=0;i<=nmatch;i++) { /* nmatch==1 means 1 subsearch_pattern, so 2 search_patterns in total*/
				pmatch[i].rm_so = ovector[i*2] + byte_offset;
				pmatch[i].rm_eo = ovector[i*2+1] + byte_offset;
			}
			returnvalue.pmatch = pmatch;
			returnvalue.nmatch = retval;
#ifdef DEBUG
			{	int i;
				for (i=0;i<returnvalue.nmatch;i++) {
					DEBUG_MSG("search_backend, sub search_pattern %d so=%d, eo=%d\n", i, returnvalue.pmatch[i].rm_so, returnvalue.pmatch[i].rm_eo);
				}
			}
#endif
			/* if want_submatches is set, pmatch should be 
			free`ed by the calling function! */
		}
		pcre_free(pcre_c);
	} else {
		/* non regex part start */
		if (!is_case_sens) {
			f = strncasecmp;
		} else {
			f = strncmp;
		}
		buflen = strlen(buf);
		patlen = strlen(search_pattern);
		
		for (i = byte_offset; i <= (buflen - patlen); i++) {
			match = f(&buf[i], search_pattern, patlen);
			if (match == 0) {
				returnvalue.bstart = i;
				returnvalue.bend = i + patlen;
				break;
			}
		}
		/* non regex part end */	
	}
	
	/* if we have a valid result, we now calculate the character offsets for this result */
	if (returnvalue.bstart >= 0 && returnvalue.bend >= 0) {
		if (returnvalue.bstart >= 0) {
			returnvalue.start = utf8_byteoffset_to_charsoffset_cached(buf, returnvalue.bstart);
		}
		if (returnvalue.bend >= 0) {
			returnvalue.end = utf8_byteoffset_to_charsoffset_cached(buf, returnvalue.bend);
		}
		if (want_submatches) {
			/* now convert the submatch start and end to character values */
			int i;
			for (i=0;i<returnvalue.nmatch;i++) {
				returnvalue.pmatch[i].rm_so = utf8_byteoffset_to_charsoffset_cached(buf, returnvalue.pmatch[i].rm_so);
				returnvalue.pmatch[i].rm_eo = utf8_byteoffset_to_charsoffset_cached(buf, returnvalue.pmatch[i].rm_eo);
			}
		}
	} else {
		returnvalue.start = -1;
		returnvalue.end = -1;
		returnvalue.bstart = -1;
		returnvalue.bend = -1;
	}

	DEBUG_MSG("search_backend, returning result.start=%d, result.end=%d, bstart=%d, bend=%d\n", returnvalue.start, returnvalue.end, returnvalue.bstart, returnvalue.bend);
	return returnvalue;
}

/*****************************************************/

/**
 * search_doc:
 * @bfwin: #Tbfwin*
 * @document: a #Tdocument to search
 * @search_pattern: a #gchar* to the search pattern.
 * @matchtype: see #Tmatch_types
 * @is_case_sens: #gint
 * @startpos: #gint offset in document buffer
 * @endpos: #gint offset in document buffer, or -1 to the end
 * @unescape: unescape the pattern string before use
 *
 * Perform search by calling search_backend.
 * Updates last_snr2-values, but doesn't use them -- that is the callers duty.
 *
 * Return value: #Tsearch_result
 **/
Tsearch_result search_doc(Tbfwin *bfwin,Tdocument *document, gchar *search_pattern, Tmatch_types matchtype, gint is_case_sens, gint startpos, gint endpos, gboolean unescape, gboolean want_submatches) {
	gchar *fulltext, *realpat;
	Tsearch_result result;
	
	DEBUG_MSG("search_doc, started on document %p, startpos=%d, endpos=%d\n", document, startpos, endpos);
	utf8_offset_cache_reset();
	fulltext = doc_get_chars(document, startpos, endpos);
	DEBUG_MSG("search_doc, fulltext=%p, search_pattern=%p\n", fulltext, search_pattern);
	if (unescape) {
		realpat = unescape_string(search_pattern, FALSE);
	} else {
		realpat = search_pattern;
	}
	result = search_backend(bfwin,realpat, matchtype, is_case_sens, fulltext, 0, want_submatches);
	if (unescape) {
		g_free(realpat);
	}
	g_free(fulltext);
	if (result.end > 0) {
		DEBUG_MSG("search_doc, received a result (start=%d), adding startpos (%d) to it\n", result.start, startpos);
		result.start += startpos;
		result.end += startpos;
		LASTSNR2(bfwin->snr2)->result.start = result.start;
		LASTSNR2(bfwin->snr2)->result.end = result.end;
		LASTSNR2(bfwin->snr2)->doc = document;
		if (matchtype != match_normal) {
			/* BUG: copy the subpatterns */
			
		}
	} else {
		LASTSNR2(bfwin->snr2)->result.start = -1;
		LASTSNR2(bfwin->snr2)->result.end =  -1;
		LASTSNR2(bfwin->snr2)->doc = document;
	}
	DEBUG_MSG("search_doc, result.start=%d, result.end=%d\n", result.start, result.end);
	return result;
}

/*****************************************************/

/**
 * doc_show_result:
 * @document: a #Tdocument
 * @start: Selection start.
 * @end: Selection end.
 *
 * Focus a document and select the supplied range.
 *
 * Return value: void
 **/
void doc_show_result(Tdocument *document, GtkWindow *window, gint start, gint end) {
	DEBUG_MSG("doc_show_result, select from start=%d to end=%d\n",start, end);
	if (document != BFWIN(document->bfwin)->current_document) {
		switch_to_document_by_pointer(BFWIN(document->bfwin),document);
	}
	/*doc_select_region(document, start, end, TRUE);*/
	snr2_doc_highlight_match(document, window, start,end);
}

/*****************************************************/

/**
 * search_all:
 * @bfwin: #Tbfwin*
 * @search_pattern: #gchar to search pattern
 * @matchtype: see #Tmatch_types
 * is_case_sens: #gint set to 0 or 1.
 *
 * Perform a specified search, spanning all open documents.
 *
 * When called several times, the search continues from where it left off last time.
 * The current 'search-position' is stored in the internal last_snr2 structure.
 *
 * Return value: #Tsearch_all_result
 **/
Tsearch_all_result search_all(Tbfwin *bfwin,gchar *search_pattern, Tmatch_types matchtype, gint is_case_sens, gboolean unescape, gboolean want_submatches) {
	GList *tmplist;
	Tsearch_all_result result_all;

	DEBUG_MSG("search_all, started\n");
	result_all.start = -1;
	result_all.end = -1;
	result_all.doc = NULL;
	
	if (LASTSNR2(bfwin->snr2)->doc) {
		DEBUG_MSG("search_all, continuing previous search at doc %p\n",LASTSNR2(bfwin->snr2)->doc);
		tmplist = g_list_find(bfwin->documentlist, LASTSNR2(bfwin->snr2)->doc);
	} else {
		DEBUG_MSG("search_all, starting at first document\n");
		LASTSNR2(bfwin->snr2)->result.end = 0;
		tmplist = g_list_first(bfwin->documentlist);
	}
	while (tmplist) {
		Tsearch_result result;

		result = search_doc(bfwin,(Tdocument *)tmplist->data, search_pattern, matchtype, is_case_sens, LASTSNR2(bfwin->snr2)->result.end, -1, unescape, want_submatches);
		if (result.end > 0) {
			result_all.start = result.start;
			result_all.end = result.end;
			result_all.doc = tmplist->data;
			DEBUG_MSG("search_all, found!! start=%d, end=%d, doc=%p\n", result.start, result.end, tmplist->data);
			return result_all;
		}
		tmplist = g_list_next(tmplist);
		if (tmplist) {
			LASTSNR2(bfwin->snr2)->result.end = 0;
		}
	}
	DEBUG_MSG("search_all, not found..\n");
	return result_all;
}

/*****************************************************/

static gchar *reg_replace(gchar *replace_pattern, gint offset, Tsearch_result result, Tdocument *doc, gboolean standardescape) {
	Tconvert_table * tct;
	gchar *retval;
	gint i, size;
	DEBUG_MSG("reg_replace, started for pattern='%s',standardescape=%d\n",replace_pattern,standardescape);
	if (result.nmatch <= 10) size = (result.nmatch == 0 ) ? 0 : result.nmatch -1;
	else size= 10;
	DEBUG_MSG("reg_repplace, size=%d\n",size);
	tct = new_convert_table(size, standardescape);
	for (i=0;i<size;i++) {
		tct[i].my_int = i+48;
		tct[i].my_char = doc_get_chars(doc, offset+result.pmatch[i+1].rm_so, offset+result.pmatch[i+1].rm_eo);
	}
	retval = expand_string(replace_pattern, '\\', tct);
	free_convert_table(tct);
	return retval;
}

/*
 * this function will parse the replace string and substitute the \0, \1 etc. with 
 * the subsearch_pattern matches from regexec()
 */
/*static gchar *oldreg_replace(gchar *replace_pattern, gint offset, Tsearch_result result, Tdocument *doc) {
	gchar *tmp1, *newstring;
	gchar *tmpstr1, *tmpstr2, *tmpstr3;
	gboolean escaped=0;
	guint from=0, to=0;

	newstring = g_strdup("");
	tmp1 = replace_pattern;
	while (*tmp1 != '\0') {
		if (escaped) {
			if (*tmp1 == '\\') {
				to++;
				tmpstr1 = newstring;
				tmpstr2 = g_strndup(&replace_pattern[from], to-from);
				newstring = g_strconcat(tmpstr1, tmpstr2, NULL);
				g_free(tmpstr1);
				g_free(tmpstr2);
				from = ++to;
				DEBUG_MSG("reg_replace, escaped backslash, newstring='%s'\n", newstring);			
			} else if ((gint)*tmp1 >= 48 && (gint)*tmp1 <= 57) {
				gint num = (gint)*tmp1 - 48;
				tmpstr1 = newstring;
				tmpstr2 = g_strndup(&replace_pattern[from], to-from);
				DEBUG_MSG("reg_replace, from=%d, to=%d, tmpstr2='%s'\n", from, to, tmpstr2);
				if (result.nmatch >= num+1) {
					DEBUG_MSG("reg_replace, wanted: sub search_pattern %d, offset=%d, so=%d, eo=%d\n", num, offset, result.pmatch[num+1].rm_so, result.pmatch[num+1].rm_eo);
					tmpstr3 = doc_get_chars(doc, offset+result.pmatch[num+1].rm_so, offset+result.pmatch[num+1].rm_eo);
					DEBUG_MSG("reg_replace, subsearch_pattern %d = '%s'\n", num, tmpstr3);
				} else {
					DEBUG_MSG("reg_replace, subsearch_pattern %d does not exist, nmatch=%d\n", num, result.nmatch);
					tmpstr3 = g_strdup("");
				}
				newstring = g_strconcat(tmpstr1, tmpstr2, tmpstr3, NULL);
				g_free(tmpstr1);
				g_free(tmpstr2);
				g_free(tmpstr3);
				to += 2;
				from = to;
				DEBUG_MSG("reg_replace, substituted subsearch_pattern, newstring='%s'\n", newstring);
			} else {
				to += 2;
			}
			escaped = 0;
		} else {
			if (*tmp1 == '\\') {
				escaped = 1;
			} else {
				to++;
			}
		}
		tmp1++;
	}
	tmpstr1 = newstring;
	tmpstr2 = g_strndup(&replace_pattern[from], to-from);
	newstring = g_strconcat(tmpstr1, tmpstr2, NULL);
	g_free(tmpstr1);
	g_free(tmpstr2);
	DEBUG_MSG("reg_replace, end, newstring='%s'\n", newstring);
	return newstring;
}*/

/**
 * replace_backend:
 * @bfwin: #Tbfwin with the window from this/these documents
 * @search_pattern: #gchar* to search pattern
 * @matchtype: See #Tmatch_types
 * @is_case_sens: #gint
 * @buf: #ghar* to buffer
 * @replace_pattern: The replace pattern.
 * @doc: the #Tdocument
 * @offset: The difference between the buffer and the text widget because of previous replace actions, so the first char in buf is actually number offset in the text widget.
 * @replacetype: see #Treplace_types
 * @replacelen: #gint*, set to -1 to calculate this automatically.
 * 
 * This will perform a search and the actual replacement of data in the buffer.
 * Note that *buf is probably an offset in the documents buffer.
 * 
 * Return value: #Tsearch_result
 **/
Tsearch_result replace_backend(Tbfwin *bfwin,gchar *search_pattern, Tmatch_types matchtype, gint is_case_sens
			, gchar *buf, guint byte_offset, gchar *replace_pattern, Tdocument *doc, gint offset, Treplace_types replacetype
			, gint *replacelen, gboolean unescape) {
/* the offset in this function is the difference between the buffer and the text widget because of previous replace 
actions, so the first char in buf is actually number offset in the text widget */
/* replacelen -1 means there is no replacelen known yet, so we have to calculate it */
	Tsearch_result result;
	gchar *tmpstr=NULL, *realpat;
	
	if (unescape) {
		realpat = unescape_string(search_pattern,FALSE);
		DEBUG_MSG("replace_backend, realpat='%s'\n",realpat);
	} else {
		realpat = search_pattern;
	}
	result = search_backend(bfwin,realpat, matchtype, is_case_sens, buf, byte_offset, (matchtype != match_normal));
	if (unescape) {
		DEBUG_MSG("replace_backend, free-ing realpat\n");
		g_free(realpat);
	}
	DEBUG_MSG("replace_backend, offset=%d, result.start=%d, result.end=%d\n", offset, result.start, result.end);
	if (result.end > 0) {
		gchar *tofree;
		switch (replacetype) {
		case string:
			if (matchtype == match_normal) {
				if (unescape) {
					DEBUG_MSG("replace_backend, replace_pattern='%s'\n",replace_pattern);
					tmpstr = unescape_string(replace_pattern, FALSE);
				} else {
					tmpstr = g_strdup(replace_pattern);
				}
			} else {
				tmpstr = reg_replace(replace_pattern, offset, result, doc, unescape);
			}
			DEBUG_MSG("replace_backend, tmpstr='%s'\n", tmpstr);
		break;
		case uppercase:
			tmpstr = g_strndup(&buf[result.bstart], result.bend - result.bstart);
			tofree = tmpstr;
			tmpstr = g_utf8_strup(tmpstr, -1);
			g_free(tofree);
		break;
		case lowercase:
			tmpstr = g_strndup(&buf[result.bstart], result.bend - result.bstart);
			tofree = tmpstr;
			tmpstr = g_utf8_strdown(tmpstr, -1);
			g_free(tofree);
		break;
		}
#ifdef DEVELOPMENT
		{
		gchar *replacing;
		replacing = doc_get_chars(doc, result.start + offset, result.end + offset);
		DEBUG_MSG("replace_backend, replacing %s (%d characters) with %s (%d characters), starting at character %d\n"
				, replacing, (result.end - result.start), tmpstr, (gint)g_utf8_strlen(tmpstr, -1), offset+result.start);
		if (replacetype ==uppercase || replacetype==lowercase) {
			gchar *tmp1, *tmp2;
			tmp1 = g_utf8_strdown(replacing,-1);
			tmp2 = g_utf8_strdown(tmpstr,-1);
			if (g_utf8_strlen(tmpstr, -1) != g_utf8_strlen(replacing, -1) || strcmp(tmp1,tmp2)!=0) {
				g_print("you found a bug: uppercase/lowercase replace found a different string\n");
				exit(155);
			}
			g_free(tmp1);
			g_free(tmp2);
		}
		g_free(replacing);
		}
#endif
		/* doc_replace_text_backend needs CHARACTER positions, not bytes !! */
		doc_replace_text_backend(doc, tmpstr, result.start + offset, result.end + offset);
		if (*replacelen == -1) {
			*replacelen = g_utf8_strlen(tmpstr, -1);
		}
		g_free(tmpstr);
	}
	if (matchtype == match_posix) {
		g_free(result.pmatch);
		result.pmatch = NULL;
	}
	return result;
}

/*****************************************************/

/**
 * replace_doc_once:
 * @search_pattern: #gchar* to search pattern
 * @matchtype: see #Tmatch_types
 * @is_case_sens: #gint
 * @startpos: #gint offset in document.
 * @endpos: #gint where to stop replacing. Set to -1 to cover the entire buffer.
 * @replace_pattern: #gchar* to replace pattern
 * @doc: a #Tdocument* to work on
 * @replacetype: see #Treplace_types
 *
 * Performs a single replace with the selected parameters, by calling replace_backend().
 * Calls doc_unre_new_group(doc) to make this action undoable.
 * Updates the internal last_snr2 struct, but the _caller_ is responsible for honouring this data.
 * 
 * Return value: #Tsearch_result
 **/
Tsearch_result replace_doc_once(Tbfwin *bfwin,gchar *search_pattern, Tmatch_types matchtype, gint is_case_sens, gint startpos, gint endpos, gchar *replace_pattern, Tdocument *doc, Treplace_types replacetype, gboolean unescape) {
/* endpos -1 means do till end */
	gchar *fulltext;
	gint replacelen = 0; /* replacelen -1 means there is no replacelen known yet
								, but we don't need one, so we set it to 0 */
	Tsearch_result result;

	doc_unre_new_group(doc);
	utf8_offset_cache_reset();
	fulltext = doc_get_chars(doc, startpos, endpos);
	result = replace_backend(bfwin,search_pattern, matchtype, is_case_sens, fulltext, 0, replace_pattern, doc, startpos, replacetype, &replacelen, unescape);
	if ( result.end > 0) {
		LASTSNR2(bfwin->snr2)->result.start = result.start + startpos;
		LASTSNR2(bfwin->snr2)->result.end = result.end + startpos;
		LASTSNR2(bfwin->snr2)->doc = doc;
	} else {
		LASTSNR2(bfwin->snr2)->result.start = -1;
		LASTSNR2(bfwin->snr2)->result.end = -1;
		LASTSNR2(bfwin->snr2)->doc = doc;
	}
	g_free(fulltext);

	doc_unre_new_group(doc);

	return result;
}
 
/*****************************************************/

/**
 * replace_doc_multiple:
 * @search_pattern: #gchar* to search pattern
 * @matchtype: see #Tmatch_types
 * @is_case_sens: #gint
 * @startpos: #gint where to start in characters
 * @endpos: #gint where to stop replacing. Set to -1 to cover the entire buffer.
 * @replace_pattern: #gchar* to replace pattern
 * @doc: a #Tdocument* to work on
 * @replacetype: see #Treplace_types
 * 
 * Performs a replace on all occurences of the pattern in the supplied document.
 * The doc's buffer will be modified.
 *
 * last_snr2 is reset with .start = .end = -1, and .doc = doc.
 * 
 * Return value: void
 **/
static gint replace_doc_multiple(Tbfwin *bfwin,const gchar *search_pattern, Tmatch_types matchtype, gint is_case_sens, gint startpos, gint endpos, const gchar *replace_pattern, Tdocument *doc, Treplace_types replacetype, gboolean unescape, guint unre_action_id) {
/* endpos -1 means do till end */
	gint count=0;
	gchar *fulltext, *realpats, *realpatr;
	gboolean realunesc;
	Tsearch_result result;
	gint buf_b_offset=0; /* the position in bytes where to start the next search in our temporary buffer */
	gint offset=startpos; /* the number of characters after which our temporary buffer starts */
	gint replacelen; /* replacelen -1 means there is no replacelen known yet */
	doc_unre_new_group_action_id(doc,unre_action_id);

	DEBUG_MSG("replace_doc_multiple, STARTED, startpos=%d, endpos=%d, document=%p\n", startpos, endpos, doc);
	if (matchtype == match_normal || replacetype != string) {
		/* the replace string has a fixed length if it is not regex, or it is not type string
		 in this case we can also do the unescaping in this function */
		if (unescape) {
			realpats = unescape_string(search_pattern, FALSE);
			realpatr = unescape_string(replace_pattern, FALSE);
			DEBUG_MSG("replace_doc_multiple, unescaped patterns, realpats='%s', realpatr='%s'\n",realpats, realpatr);
		} else {
			realpats = (gchar *)search_pattern;
			realpatr = (gchar *)replace_pattern;
		}
		replacelen = g_utf8_strlen(realpatr,-1);
		realunesc = FALSE;
	} else {
		replacelen=-1;
		realpats = (gchar *)search_pattern;
		realpatr = (gchar *)replace_pattern;
		realunesc = unescape;
	}
	fulltext = doc_get_chars(doc, startpos, endpos);
	utf8_offset_cache_reset();
	result = replace_backend(bfwin,realpats, matchtype, is_case_sens, fulltext, 0, realpatr, doc, offset, replacetype, &replacelen, realunesc);
	while (result.end > 0) {
		/* the 'buf_b_offset' is the location in the buffer, measured in bytes, where the next search has to start 
		 *
		 * 'offset' is the number of characters after which our temporary buffer starts
		 *
		 */
		if (replacetype == string) {
			offset += replacelen - (result.end - result.start);
		}
		if (LASTSNR2(bfwin->snr2)->overlapping_search || result.start == result.end) {
			buf_b_offset = result.bstart + 1;
			/* BUG: if the first following character is a multibute character, we should perhaps 
			add 2 or 3 to result.bstart */
		} else {
			buf_b_offset = result.bend;
		}
		if (matchtype != match_normal && replacetype == string) {
			/* all regex replaces can have different replace lengths, so they have to be re-calculated */
			replacelen = -1;
		}
		result = replace_backend(bfwin,realpats, matchtype, is_case_sens, fulltext, buf_b_offset, realpatr
						, doc, offset, replacetype, &replacelen, realunesc);
		count++;
		DEBUG_MSG("replace_doc_multiple, after replace_backend, offset=%d, buf_b_offset=%d, result.start=%d, result.end=%d\n", offset, buf_b_offset, result.start, result.end);
	}
	if (unescape && (matchtype == match_normal || replacetype != string)) {
		DEBUG_MSG("replace_doc_multiple, free-ing realpats and realpatr\n");
		g_free(realpats);
		g_free(realpatr);
	}

	doc_unre_new_group_action_id(doc,0);

	LASTSNR2(bfwin->snr2)->result.start = -1;
	LASTSNR2(bfwin->snr2)->result.end = -1;
	LASTSNR2(bfwin->snr2)->doc = doc;
	g_free(fulltext);
	return count;
}

/*****************************************************/

/**
 * replace_all:
 * @bfwin: #Tbfwin* the window with the documents to act upon
 * @search_pattern: #gchar* to search pattern
 * @matchtype: see #Tmatch_types
 * @is_case_sens: #gint
 * @replace_pattern: #gchar* to replace pattern
 * @replacetype: see #Treplace_types
 * @unescape: #gboolean if the pattern needs unescaping
 * 
 * Perform a replace_doc_multiple() with supplied data on all open documents.
 * This will replace all occurences of search_pattern in all documents.
 * 
 * Return value: void
 **/
static gint replace_all(Tbfwin *bfwin,const gchar *search_pattern, Tmatch_types matchtype, gint is_case_sens, const gchar *replace_pattern, Treplace_types replacetype, gboolean unescape) {
	GList *tmplist;
	gint count=0;
	guint unre_action_id = new_unre_action_id();
	tmplist = g_list_first(bfwin->documentlist);
	while (tmplist) {
		count += replace_doc_multiple(bfwin,search_pattern, matchtype, is_case_sens, 0, -1, replace_pattern, (Tdocument *)tmplist->data, replacetype, unescape, unre_action_id);
		tmplist = g_list_next(tmplist);
	}
	return count;
}

/**
 * replace_current_match:
 *
 *
 * for new dialog: 
 *
 * Return value: TRUE if found and replaced the match
 */
static gboolean replace_current_match(Tbfwin *bfwin) {
	GtkTextIter itstart,itend;
	GtkTextTag *tag;
	gtk_text_buffer_get_iter_at_offset(bfwin->current_document->buffer,&itstart,LASTSNR2(bfwin->snr2)->result.start);
	gtk_text_buffer_get_iter_at_offset(bfwin->current_document->buffer,&itend,LASTSNR2(bfwin->snr2)->result.end);
	tag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(bfwin->current_document->buffer),"snr2match");
	if (gtk_text_iter_toggles_tag(&itstart, tag) && gtk_text_iter_toggles_tag(&itend, tag)) {
		gchar *tmpstr;
		gint lenadded;
		if (LASTSNR2(bfwin->snr2)->replacetype_option==string) {
			tmpstr = g_strdup(LASTSNR2(bfwin->snr2)->replace_pattern);
			/* if it was a regex replace we need to do the sub-search_pattern matching */
			tmpstr = reg_replace(tmpstr, 0, LASTSNR2(bfwin->snr2)->result, bfwin->current_document, LASTSNR2(bfwin->snr2)->unescape);
		} else if (LASTSNR2(bfwin->snr2)->replacetype_option==uppercase) {
			gchar *tofree;
			tmpstr = doc_get_chars(bfwin->current_document, LASTSNR2(bfwin->snr2)->result.start ,LASTSNR2(bfwin->snr2)->result.end);
			tofree = tmpstr;
			tmpstr = g_utf8_strup(tmpstr, -1);
			g_free(tofree);
		} else {
			gchar *tofree;
			tmpstr = doc_get_chars(bfwin->current_document, LASTSNR2(bfwin->snr2)->result.start ,LASTSNR2(bfwin->snr2)->result.end);
			tofree = tmpstr;
			tmpstr = g_utf8_strdown(tmpstr, -1);
			g_free(tofree);
		}
		/* avoid new highlighting at this stage, so call the backend directly instead of the frontend function
		this because the highlighting interferes with the selection
		the better solution is to have the highlighting handle the selection better, 
		the problem starts in document.c in get_positions() because the selection is not saved there
		I don't know why the selection is gray, but that's basically the reason why it doesn't save the selection
		 */

		doc_unre_new_group(bfwin->current_document);
		doc_replace_text_backend(bfwin->current_document, tmpstr, LASTSNR2(bfwin->snr2)->result.start,LASTSNR2(bfwin->snr2)->result.end);
		doc_unre_new_group(bfwin->current_document);
		doc_set_modified(bfwin->current_document, 1);

		lenadded = strlen(tmpstr) - (LASTSNR2(bfwin->snr2)->result.end - LASTSNR2(bfwin->snr2)->result.start);
		DEBUG_MSG("lenadded=%d (streln=%d, end-start=%d)\n",lenadded,strlen(tmpstr),(LASTSNR2(bfwin->snr2)->result.end - LASTSNR2(bfwin->snr2)->result.start));
		g_free(tmpstr);
		/* empty the last match */
		if (LASTSNR2(bfwin->snr2)->result.pmatch) {
			g_free(LASTSNR2(bfwin->snr2)->result.pmatch);
			LASTSNR2(bfwin->snr2)->result.pmatch = NULL;
		}
		if (!LASTSNR2(bfwin->snr2)->overlapping_search && lenadded > 0) {
			LASTSNR2(bfwin->snr2)->result.end += lenadded;
		}
		if (LASTSNR2(bfwin->snr2)->placetype_option == selection) {
			LASTSNR2(bfwin->snr2)->endpos += lenadded;
		}
		return TRUE;
	}
	return FALSE;
}

/* this function can do multiple searches, only used for bookmarking and counting matches, it
returns the number of matches */
static gint search_doc_multiple_backend(Tbfwin *bfwin,Tdocument *document, gchar *search_pattern, Tmatch_types matchtype, gint is_case_sens, gint startpos, gint endpos, gboolean unescape) {
	gchar *fulltext, *realpat;
	gint buf_byte_offset=0, count=0;
	Tsearch_result result;
	fulltext = doc_get_chars(document, startpos, endpos);
	utf8_offset_cache_reset();
	if (unescape) {
		realpat = unescape_string(search_pattern, FALSE);
	} else {
		realpat = search_pattern;
	}
	result = search_backend(bfwin,realpat, matchtype, is_case_sens, fulltext, 0, FALSE);
	while (result.end > 0) {
		if (LASTSNR2(bfwin->snr2)->bookmark_results) {
			gchar *text = doc_get_chars(document, startpos+result.start, startpos+result.end);
			DEBUG_MSG("search_doc_multiple_backend, adding bookmark '%s' at %d\n", text, result.start);
			bmark_add_extern(document, startpos+result.start, search_pattern, text, !main_v->globses.bookmarks_default_store);
			g_free(text);
		}
		count++;
		if (LASTSNR2(bfwin->snr2)->overlapping_search) {
			buf_byte_offset = result.bstart + 1;
		} else {
			buf_byte_offset = result.bend;
		}
		result = search_backend(bfwin,realpat, matchtype, is_case_sens, fulltext, buf_byte_offset, FALSE);
	}
	if (unescape) {
		g_free(realpat);
	}
	g_free(fulltext);
	return count;
}

/**
 * search_multiple:
 * @bfwin: #Tbfwin *
 * @startat: #gint
 *
 * will search all, return the count, and if requested bookmark the matches
 * 
 */
static gint search_multiple(Tbfwin *bfwin, gint startpos, gint endpos) {
	gint count=0;
	DEBUG_MSG("search_multiple, started\n");
	if (LASTSNR2(bfwin->snr2)->placetype_option==opened_files) {
		GList *tmplist = g_list_first(bfwin->documentlist);
		while (tmplist) {
			count += search_doc_multiple_backend(bfwin,DOCUMENT(tmplist->data), LASTSNR2(bfwin->snr2)->search_pattern, LASTSNR2(bfwin->snr2)->matchtype_option, LASTSNR2(bfwin->snr2)->is_case_sens, 0, -1, LASTSNR2(bfwin->snr2)->unescape);
			tmplist = g_list_next(tmplist);
		}
	} else {
		count = search_doc_multiple_backend(bfwin,DOCUMENT(bfwin->current_document), LASTSNR2(bfwin->snr2)->search_pattern, LASTSNR2(bfwin->snr2)->matchtype_option, LASTSNR2(bfwin->snr2)->is_case_sens, startpos, endpos, LASTSNR2(bfwin->snr2)->unescape);
	}
	DEBUG_MSG("search_multiple, done\n");
	return count;
}

static Tsearch_result search_single_and_show(Tbfwin *bfwin, GtkWindow *dialog, gint startpos, gint endpos, gboolean want_submatches) {
	Tsearch_result result = {0,0,0,0,NULL,0};
	if (LASTSNR2(bfwin->snr2)->placetype_option==opened_files) {
		Tsearch_all_result result_all;
		snr2_doc_remove_highlight(LASTSNR2(bfwin->snr2)->doc);
		result_all = search_all(bfwin,LASTSNR2(bfwin->snr2)->search_pattern, LASTSNR2(bfwin->snr2)->matchtype_option, LASTSNR2(bfwin->snr2)->is_case_sens, LASTSNR2(bfwin->snr2)->unescape, want_submatches);
		result.start = result_all.start;
		result.end = result_all.end;
		if (result_all.end > 0) {
			doc_show_result(result_all.doc, dialog, result_all.start, result_all.end);
			if (bfwin->current_document->uri && LASTSNR2(bfwin->snr2)->bookmark_results) {
				gchar *text = doc_get_chars(result_all.doc, result_all.start, result_all.end);
				DEBUG_MSG("search_single_and_show, adding bookmark '%s' at %d\n", text, result_all.start);
				bmark_add_extern(result_all.doc, result_all.start, LASTSNR2(bfwin->snr2)->search_pattern, text, !main_v->globses.bookmarks_default_store);
				g_free(text);
			}
		}
		return result;
	} else {
		DEBUG_MSG("search_single_and_show, startpos=%d, endpos=%d\n",startpos,endpos);
		snr2_doc_remove_highlight(bfwin->current_document);
		result = search_doc(bfwin,bfwin->current_document, LASTSNR2(bfwin->snr2)->search_pattern, LASTSNR2(bfwin->snr2)->matchtype_option, LASTSNR2(bfwin->snr2)->is_case_sens, startpos, endpos, LASTSNR2(bfwin->snr2)->unescape, want_submatches);
		if (result.end > 0) {
			doc_show_result(bfwin->current_document, dialog, result.start, result.end);
			if (bfwin->current_document->uri && LASTSNR2(bfwin->snr2)->bookmark_results) {
				gchar *text = doc_get_chars(bfwin->current_document, result.start, result.end);
				DEBUG_MSG("search_single_and_show, adding bookmark '%s' at %d\n", text, result.start);
				bmark_add_extern(bfwin->current_document, result.start, LASTSNR2(bfwin->snr2)->search_pattern, text, !main_v->globses.bookmarks_default_store);
				g_free(text);
			}

		} 
		return result;
	}
}

/*****************************************************/

/**
 * snr2_run_extern_replace:
 * @doc: a #Tdocument
 * @search_pattern: #gchar* to search pattern
 * @region: #gint, 0 = region_from_beginning, 1 = region_from_cursor, 2 = region_selection, 3 = region_all_open_files
 * @matchtype: #gint, 0 = normal, 1 = posix, 2 = perl
 * @is_case_sens: #gint
 * @replace_pattern: #gchar* to replace pattern.
 * @ unescape: #gboolean whether the patterns needs to be unescaped 
 * Return value: void
 **/

void snr2_run_extern_replace(Tdocument *doc, const gchar *search_pattern, gint region,
							gint matchtype, gint is_case_sens, const gchar *replace_pattern,
							gboolean unescape) {
	DEBUG_MSG("snr2_run_extern_replace, search_pattern=%s, replace_pattern=%s, unescape=%d\n",search_pattern,replace_pattern,unescape);
	
	if (region == 3) { /* in all open files */
		replace_all(BFWIN(doc->bfwin),search_pattern
					, matchtype
					, is_case_sens
					, replace_pattern
					, string
					, unescape);
	} else {
		gint startpos=0,endpos=-1;
		if (region == 1) {
			startpos = doc_get_cursor_position(doc);
		} else if (region == 2){
			if (!doc_get_selection(doc,&startpos,&endpos)) {		
				return;
			}
		}
		replace_doc_multiple(BFWIN(doc->bfwin),search_pattern
					, matchtype
					, is_case_sens
					, startpos, endpos, replace_pattern
					, BFWIN(doc->bfwin)->current_document
					, string
					, unescape, 0 /* unre_id */);	
	}
}

/**
 * doc_search_run_extern:
 * @doc: a #Tdocument
 * @search_pattern: #gchar search pattern
 * @matchtype: #gint, 0 = normal, 1 = posix, 2 = perl
 * @is_case_sens: #gint, case sensitive pattern?
 *
 * Frontend for search_doc, calling it with supplied arguments and startpos = 0.
 * 
 * Return value: #Tsearch_result_doc
 **/
Tsearch_result doc_search_run_extern(Tdocument *doc, gchar *search_pattern, gint matchtype, gint is_case_sens) {
	return search_doc(BFWIN(doc->bfwin),doc, search_pattern, matchtype, is_case_sens, 0, -1, FALSE, FALSE);
} 

/*****************************************************/

#ifdef NOTPORTEDYET

static gint do_filename_curfile_replace(gchar *fulltext, Tsearch_result result, gint offset, gchar *olddirname, gchar *newfilename, gint changelen, Tdocument *doc) {
/* returns the change in the lenght of the buffer compared to the actual document text */
	gchar *possible_filename;
	gchar *olddirfile;
	gint len;
	gchar *replacestring;
	gint change_lenght = changelen;

	possible_filename = g_strndup(&fulltext[offset + result.bstart +1], result.bend - result.bstart -2 );
	DEBUG_MSG("do_filename_change_replace, possible_filename=%s\n", possible_filename);
	len = strlen(possible_filename) + strlen(olddirname) + 2;
	olddirfile = strncat(strncat(strncpy(g_malloc(len), olddirname, len), "/", len), possible_filename, len);

	/* code to check if this is a file */
	if (file_exists_and_readable(olddirfile)) {
		DEBUG_MSG("do_filename_change_replace, olddirfile=%s does exist!!\n", olddirfile);
		/* code to create replace_pattern */
		olddirfile = most_efficient_filename(olddirfile);
		DEBUG_MSG("do_filename_change_replace, updated olddirfile=%s \n", olddirfile);
		replacestring = create_relative_link_to(newfilename, olddirfile);
		DEBUG_MSG("do_filename_change_replace, replacestring=%s, newfilename=%s\n", replacestring, newfilename);
		/* code to actual replace it */
		doc_replace_text(doc, replacestring, result.start + offset + 1 + changelen, result.end + offset + changelen -1);	
		change_lenght = strlen(replacestring) - strlen(possible_filename) + changelen;
		DEBUG_MSG("do_filename_change_replace, replacestring=%s, possible_filename=%s\n", replacestring, possible_filename);
		DEBUG_MSG("do_filename_change_replace, change_lenght=%d\n",change_lenght );		
		g_free(replacestring); 
	} else {
		DEBUG_MSG("do_filename_change_replace, olddirfile=%s does NOT exist\n", olddirfile);  
	}
	g_free(possible_filename);
	g_free(olddirfile);
	return change_lenght;
}

static gint do_filename_otherfile_replace(gchar *fulltext, Tsearch_result result, gint offset, gchar *oldfilename, gchar *newfilename, gint changelen, Tdocument *doc) {
	gchar *possible_filename, *replacestring, *foundstring;
	gint change_length = changelen;
	gchar *eff_my_filename;

	foundstring = g_strndup(&fulltext[offset + result.bstart +1], result.bend - result.bstart -2 );
	possible_filename = most_efficient_filename(create_full_path(foundstring, doc->filename));
	g_free(foundstring);
	DEBUG_MSG("do_filename_otherfile_replace, posname=%s\n", possible_filename);
	DEBUG_MSG("do_filename_otherfile_replace, newname=%s\n", oldfilename);
	if (strcmp(possible_filename, oldfilename) == 0) {
		eff_my_filename = most_efficient_filename(g_strdup(doc->filename));
		replacestring = create_relative_link_to(eff_my_filename, newfilename);
		doc_replace_text(doc, replacestring, result.start + offset + 1 + changelen, result.end + offset + changelen -1);
		change_length = strlen(replacestring) - strlen(possible_filename) + changelen;		
		g_free(eff_my_filename);
		g_free(replacestring);
	}
	g_free(possible_filename);
	return change_length;
}

/**
 * update_filenames_in_file:
 * @doc: a #Tdocument
 * @oldfilename: Filename to change from.
 * @newfilename: Filename to change to.
 * @doc_has_newfilename: If *doc (blabla?)
 * 
 * 
 *
 * Return value: void
 **/
void update_filenames_in_file(Tdocument *doc, gchar *oldfilename, gchar *newfilename, gint doc_has_newfilename) {
	gchar *fulltext;
	Tsearch_result result;
	gint cur_offset, changelen=0;
	gchar *search_pattern;
	Tmatch_types matchtype;
	gint is_case_sens;
	gchar *olddirname=NULL;

	if ((oldfilename == NULL)|| (newfilename == NULL)) {
		return;
	}
	search_pattern = "\"[^\"]+\"";
	matchtype = match_posix;
 	is_case_sens = 0;
	cur_offset = 0;

	if (doc_has_newfilename) {
			olddirname = g_path_get_dirname(oldfilename);
	}

	fulltext = doc_get_chars(doc, 0, -1);
	utf8_offset_cache_reset();
	result = search_backend(search_pattern, matchtype, is_case_sens, fulltext, 0);
	while (result.end > 0) {
		if (doc_has_newfilename) {
			changelen = do_filename_curfile_replace(fulltext, result, cur_offset, olddirname, newfilename, changelen, doc);
		} else {
			changelen = do_filename_otherfile_replace(fulltext, result, cur_offset, oldfilename, newfilename, changelen, doc);
		}
		cur_offset += result.bstart +1;
		result = search_backend(search_pattern, matchtype, is_case_sens, &fulltext[cur_offset], 0);
	}
	g_free(fulltext);
	if (doc_has_newfilename) {
		g_free(olddirname);
	}
}
#endif /* NOTPORTEDYET */

/**
 * update_encoding_meta_in_file:
 * @doc: a #Tdocument*
 * @encoding: #gchar*, The documents character encoding
 *
 * Update the HTML meta encoding tags for the supplied document.
 *
 * Return value: void
 **/
void update_encoding_meta_in_file(Tdocument *doc, gchar *encoding) {
	if (encoding) {
		Tbfwin *bfwin = BFWIN(doc->bfwin);
		Tlast_snr2 last_snr2_bck = *LASTSNR2(bfwin->snr2);
		gchar *last_search_pattern_bck = g_strdup(LASTSNR2(bfwin->snr2)->search_pattern);
		gchar *search_pattern, *fulltext;
		Tsearch_result result;
		/* first find if there is a meta encoding tag already */
		search_pattern = "<meta[ \t\n]http-equiv[ \t\n]*=[ \t\n]*\"content-type\"[ \t\n]+content[ \t\n]*=[ \t\n]*\"([^;]*);[ \t\n]*charset=[a-z0-9-]*\"[ \t\n]*(/?)>";
		fulltext = doc_get_chars(doc, 0, -1);
		utf8_offset_cache_reset();
		result = search_backend(bfwin,search_pattern, match_posix, 0, fulltext, 0, 1);
		if (result.end > 0) {
			gchar *replacestring, *type, *xhtmlend;
			DEBUG_MSG("update_encoding_meta_in_file, we have a match, nmatch=%d\n",result.nmatch);
			if (result.nmatch > 2) {
				type = g_strndup(fulltext+result.pmatch[1].rm_so, result.pmatch[1].rm_eo - result.pmatch[1].rm_so);
				xhtmlend = g_strndup(fulltext+result.pmatch[2].rm_so, result.pmatch[2].rm_eo - result.pmatch[2].rm_so);
				DEBUG_MSG("update_encoding_meta_in_file, type=%s (bstart=%d, bend=%d, so[1]=%d, eo[1]=%d)\n",type,result.bstart,result.bend,result.pmatch[1].rm_so,result.pmatch[1].rm_eo);
			} else {
				type = g_strdup("text/html");
				xhtmlend = g_strdup( main_v->props.xhtml ? "/" : "");
			}
			replacestring = g_strconcat("<meta http-equiv=\"content-type\" content=\"",type,"; charset=",encoding,"\" ",xhtmlend,">", NULL);
			DEBUG_MSG("update_encoding_meta_in_file, 1: we have a match\n");
			doc_replace_text(doc, replacestring, result.start, result.end);
			g_free(replacestring);
			g_free(type);
			g_free(xhtmlend);
		} else {
			DEBUG_MSG("update_encoding_meta_in_file, 1: NO match\n");
			/* now search for <head>, we can append it to this tag */
			search_pattern = "<head>";
			result = search_backend(bfwin,search_pattern, match_posix, 0, fulltext, 0, 0);
			if (result.end > 0) {
				gchar *replacestring = g_strconcat("<head>\n<meta http-equiv=\"content-type\" content=\"text/html; charset=",encoding,"\">", NULL);
				DEBUG_MSG("update_encoding_meta_in_file, 2: we have a match\n");
				doc_replace_text(doc, replacestring, result.start, result.end);
				g_free(replacestring);
			} else {
				DEBUG_MSG("update_encoding_meta_in_file, 2: NO match\n");
			}
		}
		g_free(fulltext);
		g_free(LASTSNR2(bfwin->snr2)->search_pattern);
		*LASTSNR2(bfwin->snr2) = last_snr2_bck;
		LASTSNR2(bfwin->snr2)->search_pattern = last_search_pattern_bck;
	}
}

static void setup_new_snr2(Tbfwin *bfwin, const gchar *search_pattern, gboolean unescape, 
		gboolean is_case_sens, gboolean overlapping_search,
		gboolean bookmark, Tplace_types place_type, Tmatch_types match_type, Treplace_types replace_type,
		gboolean replace, const gchar *replace_pattern) {
	if (LASTSNR2(bfwin->snr2)->search_pattern) {
		g_free(LASTSNR2(bfwin->snr2)->search_pattern);
		LASTSNR2(bfwin->snr2)->search_pattern = NULL;
	}
	if (LASTSNR2(bfwin->snr2)->replace_pattern) {
		g_free(LASTSNR2(bfwin->snr2)->replace_pattern);
		LASTSNR2(bfwin->snr2)->replace_pattern = NULL;
	}
	LASTSNR2(bfwin->snr2)->search_pattern = g_strdup(search_pattern);
	bfwin->session->searchlist = add_to_history_stringlist(bfwin->session->searchlist,LASTSNR2(bfwin->snr2)->search_pattern,TRUE,TRUE);
	LASTSNR2(bfwin->snr2)->unescape = unescape;
 	LASTSNR2(bfwin->snr2)->is_case_sens = is_case_sens;
 	LASTSNR2(bfwin->snr2)->overlapping_search = overlapping_search;
	LASTSNR2(bfwin->snr2)->replace = replace;
	LASTSNR2(bfwin->snr2)->placetype_option = place_type;
	LASTSNR2(bfwin->snr2)->matchtype_option = match_type;
	LASTSNR2(bfwin->snr2)->replacetype_option = replace_type;
	LASTSNR2(bfwin->snr2)->endpos = -1;
	if (replace_pattern) {
		LASTSNR2(bfwin->snr2)->replace_pattern = g_strdup(replace_pattern);
		bfwin->session->replacelist = add_to_history_stringlist(bfwin->session->replacelist,LASTSNR2(bfwin->snr2)->replace_pattern,TRUE,TRUE);
	}
	LASTSNR2(bfwin->snr2)->bookmark_results = bookmark;
	LASTSNR2(bfwin->snr2)->matches = 0;
	LASTSNR2(bfwin->snr2)->replaces = 0;
	LASTSNR2(bfwin->snr2)->doc = NULL;
}


/***************** NEW DIALOG ************************************/
typedef struct {
	gint dialogType;
	GtkWidget *dialog;
	GtkWidget *expander;
	GtkWidget *search;
	GtkWidget *replace;
	GtkWidget *scope;
	GtkWidget *countlabel;
	GtkWidget *warninglabel;
	GtkWidget *matchPattern;
	GtkWidget *replaceType;
	GtkWidget *overlappingMatches;
	GtkWidget *matchCase;
	GtkWidget *escapeChars;
	GtkWidget *bookmarks;
	GtkWidget *findButton;
	GtkWidget *findAllButton;
	GtkWidget *replaceButton;
	GtkWidget *replaceAllButton;
	Tbfwin *bfwin;
} TSNRWin;

enum {
	SNR_RESPONSE_FIND = 0,
	SNR_RESPONSE_REPLACE,
	SNR_RESPONSE_REPLACE_ALL,
	SNR_RESPONSE_FIND_ALL
};

static gboolean snr_focus_in_lcb(GtkWidget *widget, GdkEventFocus *event, TSNRWin * snrwin)
{
    if (snrwin->bfwin->current_document->uri) {
        gtk_widget_set_sensitive (snrwin->bookmarks, TRUE);
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (snrwin->bookmarks))) {
            LASTSNR2(snrwin->bfwin->snr2)->bookmark_results = TRUE;
        }
    } else {
        LASTSNR2(snrwin->bfwin->snr2)->bookmark_results = FALSE;
        gtk_widget_set_sensitive (snrwin->bookmarks, FALSE);
    }
       
    return FALSE;
}

/* the following two functions are a trick to get a tooltip working on top of a gtkcombobox */
static void set_combo_tooltip(GtkWidget *widget, gpointer data)
{
	if (GTK_IS_BUTTON (widget)) gtk_tooltips_set_tip(main_v->tooltips, widget,(gchar *)data, NULL);
}
static void realize_combo_set_tooltip(GtkWidget *combo, gpointer data)
{
	gtk_container_forall(GTK_CONTAINER(combo),set_combo_tooltip,data);
}

static void snr_update_count_label(TSNRWin * snrwin) {
	gchar *text;
	if (snrwin->dialogType == BF_REPLACE_DIALOG) {
		gchar *text1, *text2;
		text1 = g_strdup_printf(ngettext("Found %d matches, ","Found %d matches, ", LASTSNR2(snrwin->bfwin->snr2)->matches), LASTSNR2(snrwin->bfwin->snr2)->matches);
		text2 = g_strdup_printf(ngettext("replaced %d","replaced %d", LASTSNR2(snrwin->bfwin->snr2)->replaces), LASTSNR2(snrwin->bfwin->snr2)->replaces);
		text = g_strconcat(text1,text2, NULL);
		g_free(text1);
		g_free(text2);
	} else {
		text = g_strdup_printf(ngettext("Found %d matches","Found %d matches",
					LASTSNR2(snrwin->bfwin->snr2)->matches),
					LASTSNR2(snrwin->bfwin->snr2)->matches);
	}
	gtk_label_set_markup(GTK_LABEL(snrwin->countlabel), text);
	g_free(text);
}

static void snr_combo_changed_reset_options(TSNRWin * snrwin)
{
	gint scope;
	DEBUG_MSG("snr_combo_changed, called\n");
	
	/* on any change we reset the current set of options */
	LASTSNR2(snrwin->bfwin->snr2)->result.start = -1;
	LASTSNR2(snrwin->bfwin->snr2)->matches = 0;
	LASTSNR2(snrwin->bfwin->snr2)->replaces = 0;
	scope = gtk_combo_box_get_active(GTK_COMBO_BOX(snrwin->scope));
	if (scope == opened_files && snrwin->bfwin->num_docs_not_completed > 0) {
		DEBUG_MSG("snr_combo_changed, num docs not completed is %d\n",snrwin->bfwin->num_docs_not_completed);
		/* display warning that not all documents have yet finished loading */
		gtk_label_set_markup(GTK_LABEL(snrwin->warninglabel), _("<span foreground=\"red\" weight=\"bold\">Not all documents are loaded yet</span>"));
		gtk_widget_show(snrwin->warninglabel);
		/* BUG: when do we hide this label? */
	} else {
		snr_update_count_label(snrwin);
	}
}

static void snr_combobox_changed(GtkComboBox * combobox, TSNRWin * snrwin)
{
    if (snrwin->dialogType == BF_REPLACE_DIALOG && combobox == GTK_COMBO_BOX(snrwin->replaceType)) {
	    int replacetype = gtk_combo_box_get_active(GTK_COMBO_BOX(snrwin->replaceType));
	    gtk_widget_set_sensitive(snrwin->replace, (replacetype == string));
	}
	
	snr_combo_changed_reset_options(snrwin);
}

static void snr_comboboxentry_changed(GtkComboBoxEntry * comboboxentry, TSNRWin * snrwin)
{
    if (comboboxentry == GTK_COMBO_BOX_ENTRY(snrwin->search)) {
    	if (strlen(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(snrwin->search)->child))) > 0) {
    		gtk_widget_set_sensitive(snrwin->findButton, TRUE);
    		if (snrwin->dialogType == BF_REPLACE_DIALOG) {
    			gtk_widget_set_sensitive(snrwin->replaceAllButton, TRUE);
    		} else {
    			gtk_widget_set_sensitive(snrwin->findAllButton, TRUE);
    		}
    	} else {
    		gtk_widget_set_sensitive(snrwin->findButton, FALSE);
    		if (snrwin->dialogType == BF_REPLACE_DIALOG) {
    			gtk_widget_set_sensitive(snrwin->replaceAllButton, FALSE);
    			gtk_widget_set_sensitive(snrwin->replaceButton, FALSE);
    		} else {
    			gtk_widget_set_sensitive(snrwin->findAllButton, FALSE);
    		}
    	}
	}
	
	snr_combo_changed_reset_options(snrwin);
}

static void snr_option_toggled(GtkToggleButton *togglebutton,gpointer user_data) 
{
	TSNRWin *snrwin = (TSNRWin *)user_data;
	
	LASTSNR2(snrwin->bfwin->snr2)->result.start = -1;
	LASTSNR2(snrwin->bfwin->snr2)->matches = 0;
	LASTSNR2(snrwin->bfwin->snr2)->replaces = 0;
	snr_update_count_label(snrwin);
}

static void snr_dialog_destroy(TSNRWin * snrwin)
{
	DEBUG_MSG("snr_dialog_destroy, called\n");
	snr2_doc_remove_highlight(LASTSNR2(snrwin->bfwin->snr2)->doc);
	gtk_widget_destroy(snrwin->dialog);
	g_free(snrwin);
}

static void snr_dialog_save_position(TSNRWin * snrwin)
{
	gint x, y;
	
	snrwin->bfwin->session->snr_is_expanded =
				gtk_expander_get_expanded(GTK_EXPANDER(snrwin->expander));
	
	gtk_window_get_position(GTK_WINDOW(snrwin->dialog), &x, &y);
	
	if (x < 0) x=0;
	if (y < 0) y=0;
	
	snrwin->bfwin->session->snr_position_x = x;
	snrwin->bfwin->session->snr_position_y = y;
}

static void snr_response_lcb(GtkDialog * dialog, gint response, TSNRWin * snrwin)
{
	const gchar *search_pattern, *replace_pattern=NULL;
	gint ret, startpos=0,endpos=-1;
	Tbfwin *bfwin=snrwin->bfwin;
	gint matchtype, replacetype=-1;
	gint scope = gtk_combo_box_get_active(GTK_COMBO_BOX(snrwin->scope));
	
	matchtype = gtk_combo_box_get_active(GTK_COMBO_BOX(snrwin->matchPattern));
	search_pattern = gtk_entry_get_text(GTK_ENTRY(GTK_BIN(snrwin->search)->child)); 
				/* gtk_combo_box_get_active_text(GTK_COMBO_BOX(snrwin->search));*/
	DEBUG_MSG("snr_response_lcb, scope=%d, dialogtype=%d, response=%d\n",scope,snrwin->dialogType,response);

	DEBUG_MSG("snr_response_lcb, search_pattern=%s\n",search_pattern);
	if (snrwin->dialogType == BF_REPLACE_DIALOG) {
		replace_pattern = gtk_entry_get_text(GTK_ENTRY(GTK_BIN(snrwin->replace)->child)); 
					/* gtk_combo_box_get_active_text(GTK_COMBO_BOX(snrwin->replace));*/
		replacetype = gtk_combo_box_get_active(GTK_COMBO_BOX(snrwin->replaceType));
	}
	
	/* cleanup any leftovers of previous run */
	if (response != SNR_RESPONSE_REPLACE && LASTSNR2(bfwin->snr2)->result.pmatch) {
		g_free(LASTSNR2(bfwin->snr2)->result.pmatch);
		LASTSNR2(bfwin->snr2)->result.pmatch = NULL;
	}
	if (response != GTK_RESPONSE_CLOSE) {
		gtk_widget_hide(snrwin->warninglabel);
	
		/* now check if we started with the current option set already, if so we can continue it, if 
		not we should setup a new set of options*/
		if (LASTSNR2(snrwin->bfwin->snr2)->result.start == -1) {
			DEBUG_MSG("result.start == -1: setup new search\n");
			/* snrwin->replaceType */
			setup_new_snr2(snrwin->bfwin, search_pattern, GTK_TOGGLE_BUTTON(snrwin->escapeChars)->active, 
					GTK_TOGGLE_BUTTON(snrwin->matchCase)->active, GTK_TOGGLE_BUTTON(snrwin->overlappingMatches)->active,
					GTK_TOGGLE_BUTTON(snrwin->bookmarks)->active, scope, matchtype, replacetype, (snrwin->dialogType == BF_REPLACE_DIALOG), replace_pattern);
			if (LASTSNR2(bfwin->snr2)->placetype_option==beginning) {
				startpos = 0;
				endpos = -1;
			} else if (LASTSNR2(bfwin->snr2)->placetype_option==cursor) {
				startpos = doc_get_cursor_position(bfwin->current_document);
				endpos = -1;
			} else if (LASTSNR2(bfwin->snr2)->placetype_option==selection) {
				if (!doc_get_selection(bfwin->current_document,&startpos,&endpos)) {
					/* BUG: what to do if there was no selection ?*/
					DEBUG_MSG("snr2_run, no selection found, returning\n");
					gtk_label_set_markup(GTK_LABEL(snrwin->warninglabel),_("<span foreground=\"red\" weight=\"bold\">No selection. Please select a block of text first.</span>"));
					gtk_widget_show(snrwin->warninglabel);
					return;
				}
				LASTSNR2(bfwin->snr2)->endpos = endpos;
				DEBUG_MSG("snr2_run, from selection: startpos=%d, endpos=%d\n", startpos, endpos);
			}
		} else {
			if (LASTSNR2(bfwin->snr2)->doc == bfwin->current_document) {
				if (LASTSNR2(bfwin->snr2)->result.end > 0) {
				    if (response != SNR_RESPONSE_REPLACE_ALL) {
					    if (LASTSNR2(bfwin->snr2)->overlapping_search) {
						    startpos = LASTSNR2(bfwin->snr2)->result.start + 1;
					    } else {
						    startpos = LASTSNR2(bfwin->snr2)->result.end;
					    }
				    } else {
				        startpos = LASTSNR2(bfwin->snr2)->result.start;
				    }
				}
			}
			if (LASTSNR2(bfwin->snr2)->placetype_option==selection) {
				endpos = LASTSNR2(bfwin->snr2)->endpos;
			}
			DEBUG_MSG("snr2_run, continue search, startpos=%d, endpos=%d\n", startpos, endpos);
		}
		DEBUG_MSG("snr_response_lcb, startpos=%d, result.start=%d\n",startpos,LASTSNR2(snrwin->bfwin->snr2)->result.start);
	}
	switch (response) {
	case SNR_RESPONSE_FIND:
	{
		Tsearch_result result;
		result = search_single_and_show(snrwin->bfwin, GTK_WINDOW(snrwin->dialog), startpos, endpos, (snrwin->dialogType == BF_REPLACE_DIALOG));
		if (result.end >0) {
			LASTSNR2(snrwin->bfwin->snr2)->matches++;
			DEBUG_MSG("snr_response_lcb, found match, update count label\n");
			snr_update_count_label(snrwin);
		} else {
			DEBUG_MSG("snr_response_lcb, nothing found, showing warning label\n");
			gtk_label_set_markup(GTK_LABEL(snrwin->warninglabel),_("<span foreground=\"red\" weight=\"bold\">No more matches found, next search will continue at the beginning.</span>"));
			gtk_widget_show(snrwin->warninglabel);
		}
		if (snrwin->dialogType == BF_REPLACE_DIALOG) {
			if (result.end >0) {
				/*LASTSNR2(snrwin->bfwin->snr2)->replace = TRUE;*/
				gtk_widget_set_sensitive(snrwin->replaceButton, TRUE);
				gtk_widget_grab_focus(snrwin->replaceButton);
				gtk_dialog_set_default_response(GTK_DIALOG(snrwin->dialog),SNR_RESPONSE_REPLACE);
				LASTSNR2(bfwin->snr2)->result = result;
				DEBUG_MSG("snr_response_lcb, pmatch=%p, nmatch=%d\n",LASTSNR2(bfwin->snr2)->result.pmatch, LASTSNR2(bfwin->snr2)->result.nmatch);
			} else {
				gtk_widget_set_sensitive(snrwin->replaceButton, FALSE);
			}
		}
	}
	break;
	case SNR_RESPONSE_REPLACE:
		if (replace_current_match(snrwin->bfwin)) {
			LASTSNR2(snrwin->bfwin->snr2)->replaces++;
			snr_update_count_label(snrwin);
			/* now run another find */
			gtk_widget_set_sensitive(snrwin->replaceButton, FALSE);
			gtk_dialog_response(GTK_DIALOG(snrwin->dialog),SNR_RESPONSE_FIND);
		}
	break;
	case SNR_RESPONSE_REPLACE_ALL:
		if (LASTSNR2(bfwin->snr2)->placetype_option== opened_files) {
			ret = replace_all(snrwin->bfwin,LASTSNR2(snrwin->bfwin->snr2)->search_pattern, LASTSNR2(snrwin->bfwin->snr2)->matchtype_option, LASTSNR2(snrwin->bfwin->snr2)->is_case_sens, LASTSNR2(snrwin->bfwin->snr2)->replace_pattern, LASTSNR2(snrwin->bfwin->snr2)->replacetype_option, LASTSNR2(snrwin->bfwin->snr2)->unescape);
		} else {
			ret = replace_doc_multiple(snrwin->bfwin,LASTSNR2(snrwin->bfwin->snr2)->search_pattern, LASTSNR2(snrwin->bfwin->snr2)->matchtype_option, LASTSNR2(snrwin->bfwin->snr2)->is_case_sens, startpos, endpos, LASTSNR2(snrwin->bfwin->snr2)->replace_pattern, snrwin->bfwin->current_document, LASTSNR2(snrwin->bfwin->snr2)->replacetype_option, LASTSNR2(snrwin->bfwin->snr2)->unescape, 0);
		}
		/* now update the count widget in the dialog */
		LASTSNR2(snrwin->bfwin->snr2)->matches += ret;
		LASTSNR2(snrwin->bfwin->snr2)->replaces += ret;
		snr_update_count_label(snrwin);		
	break;
	case SNR_RESPONSE_FIND_ALL:
		ret = search_multiple(snrwin->bfwin, startpos, endpos);
		/* hmm should we add the found matches to the already known value, or just set it to the found value? */
		LASTSNR2(snrwin->bfwin->snr2)->matches = ret;
		snr_update_count_label(snrwin);
	break;
	case GTK_RESPONSE_CLOSE:
		snr_dialog_save_position(snrwin);
		snr_dialog_destroy(snrwin);
	break;
	default:
		DEBUG_MSG("response %d is not handled yet: destroy dialog\n", response);
		snr_dialog_destroy(snrwin);
	break;
	}
/*	g_free(search_pattern);
	g_free(replace_pattern);*/
}

static void snr_combo_activate_lcb(GtkEntry *entry,gpointer user_data) {
	TSNRWin *snrwin = (TSNRWin *)user_data;
	DEBUG_MSG("snr_combo_activate_lcb, called\n");
	gtk_widget_grab_focus(snrwin->findButton);
	gtk_dialog_response(GTK_DIALOG(snrwin->dialog),SNR_RESPONSE_FIND);
}

static TSNRWin *snr_dialog_real(Tbfwin * bfwin, gint dialogType)
{
	TSNRWin *snrwin;
	GtkWidget *table, *vbox, *vbox2;
	gchar *title;
	gint numrows;
	GtkListStore *history;
	GList *list;
	GtkTreeIter iter;
	GtkTextIter start, end;
	unsigned int i = 0;

	const gchar *scope[] = {
		N_("Entire document"),
		N_("Forward from cursor position"),
		N_("Selection"),
		N_("All open files"),
	};

	const gchar *matchPattern[] = {
		N_("Normal"),
		N_("Entire word only"),
		N_("POSIX"),
		N_("PERL"),
	};

	const gchar *replaceType[] = {
		N_("Normal"),
		N_("Uppercase"),
		N_("Lowercase"),
	};

	snrwin = g_new(TSNRWin, 1);
	snrwin->bfwin = bfwin;
	snrwin->dialogType = dialogType;

	if (dialogType == BF_FIND_DIALOG) {
		title = g_strdup(_("Find"));
		numrows = 3;
	} else {
		title = g_strdup(_("Replace"));
		numrows = 4;
	}

	snrwin->dialog =
		gtk_dialog_new_with_buttons(title, GTK_WINDOW(bfwin->main_window),
									GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
	gtk_window_set_resizable(GTK_WINDOW(snrwin->dialog), FALSE);
	gtk_dialog_set_has_separator(GTK_DIALOG(snrwin->dialog), FALSE);
	window_delete_on_escape(GTK_WINDOW(snrwin->dialog));
	g_signal_connect(G_OBJECT(snrwin->dialog), "response", G_CALLBACK(snr_response_lcb), snrwin);
	g_signal_connect_after(G_OBJECT(snrwin->dialog), "focus-in-event", G_CALLBACK(snr_focus_in_lcb), snrwin);
	g_free(title);

	table = dialog_table_in_vbox(numrows, 2, 6, GTK_DIALOG(snrwin->dialog)->vbox, FALSE, FALSE, 0);

	history = gtk_list_store_new(1, G_TYPE_STRING);
	list = g_list_first(bfwin->session->searchlist);
	while (list) {
		DEBUG_MSG("snr_dialog_real: adding search history %s\n",(gchar *)list->data);
		gtk_list_store_append(history, &iter);
		gtk_list_store_set(history, &iter, 0, list->data, -1);
		list = g_list_next(list);
	}
    
	snrwin->search = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(history), 0);
	g_object_unref(history);
	dialog_mnemonic_label_in_table(_("_Search for: "), snrwin->search, table, 0, 1, 0, 1);
	gtk_table_attach(GTK_TABLE(table), snrwin->search, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL,
					 GTK_SHRINK, 0, 0);
	g_signal_connect(snrwin->search, "changed", G_CALLBACK(snr_comboboxentry_changed), snrwin);
	g_signal_connect(snrwin->search, "realize",G_CALLBACK(realize_combo_set_tooltip), _("The pattern to look for"));
	g_signal_connect(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(snrwin->search))), "activate",G_CALLBACK(snr_combo_activate_lcb), snrwin);	

	if (dialogType == BF_REPLACE_DIALOG) {
		history = gtk_list_store_new(1, G_TYPE_STRING);
		list = g_list_first(bfwin->session->replacelist);
		while (list) {
			DEBUG_MSG("snr_dialog_real: adding replace history %s\n",(gchar *)list->data);
			gtk_list_store_append(history, &iter);
			gtk_list_store_set(history, &iter, 0, list->data, -1);
			list = g_list_next(list);
		}

		snrwin->replace = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(history), 0);
		g_object_unref(history);
		dialog_mnemonic_label_in_table(_("Replace _with: "), snrwin->replace, table, 0, 1, 1, 2);
		gtk_table_attach(GTK_TABLE(table), snrwin->replace, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL,
						 GTK_SHRINK, 0, 0);
		g_signal_connect(snrwin->replace, "changed", G_CALLBACK(snr_comboboxentry_changed), snrwin);
		g_signal_connect(snrwin->replace, "realize",G_CALLBACK(realize_combo_set_tooltip), _("Replace matching text with"));
		g_signal_connect(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(snrwin->replace))), "activate",G_CALLBACK(snr_combo_activate_lcb), snrwin);
	}
	
	snrwin->scope = gtk_combo_box_new_text();
	for (i = 0; i < G_N_ELEMENTS(scope); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(snrwin->scope), scope[i]);
	}
	dialog_mnemonic_label_in_table(_("Sco_pe: "), snrwin->scope, table, 0, 1, numrows-2, numrows-1);
	gtk_table_attach(GTK_TABLE(table), snrwin->scope, 1, 2, numrows-2, numrows-1,
					 GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	g_signal_connect(snrwin->scope, "changed", G_CALLBACK(snr_combobox_changed), snrwin);
	g_signal_connect(snrwin->scope, "realize",G_CALLBACK(realize_combo_set_tooltip), _("Where to look for the pattern."));

	vbox2 = gtk_vbox_new(FALSE,0);
	gtk_table_attach(GTK_TABLE(table), vbox2, 1, 2, numrows - 1, numrows,
					 GTK_FILL, GTK_FILL, 0, 0);

	snrwin->countlabel = gtk_label_new((dialogType == BF_REPLACE_DIALOG)?_("Found 0 matches, replaced 0"):_("Found 0 matches"));
	gtk_misc_set_alignment(GTK_MISC(snrwin->countlabel), 0.0f, 0.5f);
	g_object_set(snrwin->countlabel, "single-line-mode", TRUE, NULL);
	gtk_box_pack_start(GTK_BOX(vbox2),snrwin->countlabel,TRUE,TRUE,0);
	
	snrwin->warninglabel = gtk_label_new(NULL);
	gtk_label_set_line_wrap(GTK_LABEL(snrwin->warninglabel),TRUE);
	gtk_misc_set_alignment(GTK_MISC(snrwin->warninglabel), 0.0f, 0.5f);
	gtk_box_pack_start(GTK_BOX(vbox2),snrwin->warninglabel,TRUE,TRUE,0);
	
	snrwin->expander = gtk_expander_new_with_mnemonic(_("<b>_Options</b>"));
	gtk_expander_set_use_markup(GTK_EXPANDER(snrwin->expander), TRUE);
	gtk_expander_set_spacing(GTK_EXPANDER(snrwin->expander), 6);
	gtk_expander_set_expanded(GTK_EXPANDER(snrwin->expander), bfwin->session->snr_is_expanded);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(snrwin->dialog)->vbox), snrwin->expander, FALSE, FALSE, 12);
	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(snrwin->expander), vbox);

	vbox2 = dialog_vbox_new(vbox);
	table = dialog_table_in_vbox_defaults(numrows - 1, 2, 0, vbox2);

	snrwin->matchPattern = gtk_combo_box_new_text();
	for (i = 0; i < G_N_ELEMENTS(matchPattern); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(snrwin->matchPattern), matchPattern[i]);
	}
	dialog_mnemonic_label_in_table(_("Match Patter_n: "), snrwin->matchPattern, table, 0, 1, 0, 1);
	gtk_table_attach(GTK_TABLE(table), snrwin->matchPattern, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL,
					 GTK_SHRINK, 0, 0);
	g_signal_connect(snrwin->matchPattern, "changed", G_CALLBACK(snr_combobox_changed), snrwin);
	g_signal_connect(snrwin->matchPattern, "realize",G_CALLBACK(realize_combo_set_tooltip), _("How to interpret the pattern."));
	
	if (dialogType == BF_REPLACE_DIALOG) {
		snrwin->replaceType = gtk_combo_box_new_text();
		for (i = 0; i < G_N_ELEMENTS(replaceType); i++) {
			gtk_combo_box_append_text(GTK_COMBO_BOX(snrwin->replaceType), replaceType[i]);
		}
		dialog_mnemonic_label_in_table(_("Replace T_ype: "), snrwin->replaceType, table, 0, 1, 1,
									   2);
		gtk_table_attach(GTK_TABLE(table), snrwin->replaceType, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL,
						 GTK_SHRINK, 0, 0);
		g_signal_connect(snrwin->replaceType, "changed", G_CALLBACK(snr_combobox_changed), snrwin);
		g_signal_connect(snrwin->replaceType, "realize",G_CALLBACK(realize_combo_set_tooltip), _("What to replace with."));
	}

	snrwin->overlappingMatches =
		gtk_check_button_new_with_mnemonic(_("Allow o_verlapping matches"));
	gtk_box_pack_start(GTK_BOX(vbox2), snrwin->overlappingMatches, FALSE, FALSE, 0);
	g_signal_connect(snrwin->overlappingMatches, "toggled", G_CALLBACK(snr_option_toggled), snrwin);
	gtk_tooltips_set_tip(main_v->tooltips,snrwin->overlappingMatches,_("After a match is found, start next search within that match."),NULL);

	snrwin->matchCase = gtk_check_button_new_with_mnemonic(_("Case sensitive _matching"));
	gtk_box_pack_start(GTK_BOX(vbox2), snrwin->matchCase, FALSE, FALSE, 0);
	g_signal_connect(snrwin->matchCase, "toggled", G_CALLBACK(snr_option_toggled), snrwin);
	gtk_tooltips_set_tip(main_v->tooltips,snrwin->matchCase,_("Only match if case (upper/lower) is identical."),NULL);

	snrwin->escapeChars = gtk_check_button_new_with_mnemonic(_("_Use escape chars"));
	gtk_box_pack_start(GTK_BOX(vbox2), snrwin->escapeChars, FALSE, FALSE, 0);
	g_signal_connect(snrwin->escapeChars, "toggled", G_CALLBACK(snr_option_toggled), snrwin);
	gtk_tooltips_set_tip(main_v->tooltips,snrwin->escapeChars,_("Unescape backslash escaped characters such as \\n, \\t etc."),NULL);
	
	snrwin->bookmarks = gtk_check_button_new_with_mnemonic(_("_Bookmark matches"));
	gtk_box_pack_start(GTK_BOX(vbox2), snrwin->bookmarks, FALSE, FALSE, 0);
	g_signal_connect(snrwin->bookmarks, "toggled", G_CALLBACK(snr_option_toggled), snrwin);
	gtk_tooltips_set_tip(main_v->tooltips,snrwin->bookmarks,_("Create a bookmark for each match"),NULL);

	gtk_dialog_add_button(GTK_DIALOG(snrwin->dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	if (dialogType == BF_REPLACE_DIALOG) {
		snrwin->replaceAllButton =
			gtk_dialog_add_button(GTK_DIALOG(snrwin->dialog), _("Replace _All"), SNR_RESPONSE_REPLACE_ALL);
		gtk_dialog_set_response_sensitive(GTK_DIALOG(snrwin->dialog), SNR_RESPONSE_REPLACE_ALL, FALSE);
		snrwin->replaceButton =
			gtk_dialog_add_button(GTK_DIALOG(snrwin->dialog), _("_Replace"), SNR_RESPONSE_REPLACE);
		gtk_dialog_set_response_sensitive(GTK_DIALOG(snrwin->dialog), SNR_RESPONSE_REPLACE, FALSE);
	} else {
		snrwin->findAllButton = gtk_dialog_add_button(GTK_DIALOG(snrwin->dialog), _("Find _All"), SNR_RESPONSE_FIND_ALL);
		gtk_dialog_set_response_sensitive(GTK_DIALOG(snrwin->dialog), SNR_RESPONSE_FIND_ALL, FALSE);
	}
	snrwin->findButton =
		gtk_dialog_add_button(GTK_DIALOG(snrwin->dialog), GTK_STOCK_FIND, SNR_RESPONSE_FIND);
	gtk_dialog_set_response_sensitive(GTK_DIALOG(snrwin->dialog), SNR_RESPONSE_FIND, FALSE);

	gtk_widget_show_all(GTK_WIDGET(GTK_BOX(GTK_DIALOG(snrwin->dialog)->vbox)));
	gtk_widget_hide(snrwin->warninglabel);

    if (gtk_text_buffer_get_selection_bounds(bfwin->current_document->buffer, &start, &end)) {
        gchar * buffer = gtk_text_buffer_get_text(bfwin->current_document->buffer, &start, &end, FALSE);
        
        if (strchr(buffer, '\n') == NULL) {
            gtk_entry_set_text(GTK_ENTRY(GTK_BIN(snrwin->search)->child), buffer);
            gtk_editable_select_region(GTK_EDITABLE(GTK_BIN(snrwin->search)->child), 0, -1);
        }
        if (buffer)    g_free(buffer);
    }
	
	gtk_combo_box_set_active(GTK_COMBO_BOX(snrwin->scope), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(snrwin->matchPattern), 0);
	if (dialogType == BF_REPLACE_DIALOG) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(snrwin->replaceType), 0);
	}
	gtk_dialog_set_default_response(GTK_DIALOG(snrwin->dialog),SNR_RESPONSE_FIND);
	DEBUG_MSG("snr_dialog_real: display the dialog\n");
	gtk_widget_show(snrwin->dialog);
	
	if ((bfwin->session->snr_position_x >= 0) && (bfwin->session->snr_position_y >= 0) &&
		(bfwin->session->snr_position_x < (gdk_screen_width()-50)) && (bfwin->session->snr_position_y < (gdk_screen_height()-50)))
	{
		gtk_window_move (GTK_WINDOW(snrwin->dialog), bfwin->session->snr_position_x, bfwin->session->snr_position_y);
	}
	
	return snrwin;
}

void snr_dialog_new(Tbfwin * bfwin, gint dialogType) {
	snr_dialog_real(bfwin, dialogType);
}

void search_from_selection(Tbfwin *bfwin) {
	gchar *string;
	GtkClipboard* cb;

	cb = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	string = gtk_clipboard_wait_for_text(cb);
	if (string) {
		GtkTreeModel* history;
		GtkTreeIter iter;
/*		setup_new_snr2(bfwin, string, FALSE, FALSE, FALSE,FALSE, beginning,FALSE, NULL);*/
		TSNRWin *snrwin = snr_dialog_real(bfwin, BF_FIND_DIALOG);
		history = gtk_combo_box_get_model(GTK_COMBO_BOX(snrwin->search));
		gtk_list_store_append(GTK_LIST_STORE(history), &iter);
		gtk_list_store_set(GTK_LIST_STORE(history), &iter, 0, string, -1);
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(snrwin->search),&iter);
		gtk_dialog_response(GTK_DIALOG(snrwin->dialog),SNR_RESPONSE_FIND);
	}
}

/**
 * search_cb:
 * @widget: unused #GtkWidget*
 * @data: unused #gpointer
 * 
 * Show the search-dialog.
 * 
 * Return value: void
 **/
void search_cb(GtkWidget *widget, Tbfwin *bfwin) {
	snr_dialog_new(bfwin, BF_FIND_DIALOG);
}

/**
 * replace_cb:
 * @widget: unused #GtkWidget*
 * @data: unused #gpointer
 * 
 * Show replace dialog.
 * 
 * Return value: void
 **/ 
void replace_cb(GtkWidget *widget, Tbfwin *bfwin) {
	snr_dialog_new(bfwin, BF_REPLACE_DIALOG);
}


/**
 * search_again_cb:
 * @widget: unused #GtkWidget*
 * @data: unused #gpointer
 * 
 * Repeat last search, if any.
 * 
 * Return value: void
 **/ 
void search_again_cb(GtkWidget *widget, Tbfwin *bfwin) {
	gint startpos=0,endpos=-1;
	Tsearch_result result;

	if (LASTSNR2(bfwin->snr2)->doc == bfwin->current_document) {
		if (LASTSNR2(bfwin->snr2)->result.end > 0) {
			if (LASTSNR2(bfwin->snr2)->overlapping_search) {
				startpos = LASTSNR2(bfwin->snr2)->result.start + 1;
			} else {
				startpos = LASTSNR2(bfwin->snr2)->result.end;
			}
		}
	}	
	
	result = search_single_and_show(bfwin, NULL, startpos, endpos, FALSE);
	if (result.end > 0) {
		LASTSNR2(bfwin->snr2)->matches++;
	} else {
		TSNRWin *snrwin = snr_dialog_real(bfwin, BF_FIND_DIALOG);
		gtk_label_set_markup(GTK_LABEL(snrwin->warninglabel),_("<span foreground=\"red\" weight=\"bold\">No more matches found, next search will continue at the beginning.</span>"));
		gtk_widget_show(snrwin->warninglabel);
	}	
}

/**
 * replace_again_cb:
 * @widget: unused #GtkWidget*
 * @data: unused #gpointer
 * 
 * Repeat last replace, if any.
 * 
 * Return value: void
 **/ 
void replace_again_cb(GtkWidget *widget, Tbfwin *bfwin) {
	gint startpos=0,endpos=-1;
	Tsearch_result sret;
	if (LASTSNR2(bfwin->snr2)->doc == bfwin->current_document) {
		if (LASTSNR2(bfwin->snr2)->result.end > 0) {
			if (LASTSNR2(bfwin->snr2)->overlapping_search) {
				startpos = LASTSNR2(bfwin->snr2)->result.start + 1;
			} else {
				startpos = LASTSNR2(bfwin->snr2)->result.end;
			}
		}
	}
	sret = replace_doc_once(bfwin,LASTSNR2(bfwin->snr2)->search_pattern, LASTSNR2(bfwin->snr2)->matchtype_option, LASTSNR2(bfwin->snr2)->is_case_sens, startpos, endpos, LASTSNR2(bfwin->snr2)->replace_pattern
			, bfwin->current_document, LASTSNR2(bfwin->snr2)->replacetype_option, LASTSNR2(bfwin->snr2)->unescape);
}
