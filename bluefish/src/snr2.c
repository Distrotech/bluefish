/* Bluefish HTML Editor
 * snr2.c - rewrite of search 'n replace functions
 *
 * Copyright (C) 2000-2002 Olivier Sessink
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * THIS IS THE BACKEND FLOW DIAGRAM
 *
 *                         snr2_run
 *         ________________/ | \  \___________________________
 *        /        |         |  \                   \  \      \
 *       /         |         |  replace_prompt_all  |   \      \
 *      /          |         |         /           /     \      \
 *   search_all    |   _____replace_prompt_doc    /       \    replace_all
 *     \           |  /          /               /         \     /
 *      \   doc_show_result     /  replace_doc_once   replace_doc_multiple
 *       \                   __/                  \         /
 *     search_doc           /                   replace_backend
 *           \             / ___________________________/
 *            \           / /
 *             search_backend
 */
/*****************************************************/
/* #define DEBUG */

#include <gtk/gtk.h>

#include "config.h"

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif /* HAVE_STRINGS_H */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>  /* _before_ regex.h for freeBSD */
#include <regex.h> 				/* regcomp() */
#include <pcre.h> 	/* pcre_compile */
#include <gdk/gdkkeysyms.h> /* GDK_Return */

#include "bluefish.h"
#include "bf_lib.h"
#include "undo_redo.h" /* doc_unre_new_group */
#include "document.h"			/* doc_replace_text() */
#include "gui.h" /* switch_to_document_by_pointer() */
#include "gtk_easy.h"         /* a lot of GUI functions */
#include "snr2.h"
#include "highlight.h" /* doc_highlight_full() */

/* Note:
    With multibyte text support a byte-offset
    of a string can not be used as a character-offset of GtkText.
    The member "start" and "end" are character-offset.
    The member "bstart" and "bend" are byte-offset in the source
    buffer.
    These values are set in search_backend().
    In single-byte text mode,
        start == bstart, end == bend .

    Iwasa, updated 2002 by Olivier
*/

typedef enum { string, upcase, lowcase }replacetypes;
enum Tmatchtypes { match_normal, match_regex, match_pcre };

typedef struct {
	gint start;
	gint end;
	Tdocument *doc;
} Tsearch_all_result;

typedef struct {
	Tdocument *doc;
	Tsearch_result result;
	gchar *pattern;
	gint region_from_beginning;
	gint region_from_cursor;
	gint region_selection;
	gint region_all_open_files;
	enum Tmatchtypes matchtype;
	gint is_case_sens;
	gint overlapping_search;
	gint replace;
	gchar *replace_string;
	gint prompt_before_replacing;
	gint replace_once;
	gint replacetype_string;
	gint replacetype_upcase;
	gint replacetype_lowcase;
} Tlast_snr2;

typedef struct {
	gboolean is_advanced;
	GtkWidget *advanced_button;
	GtkWidget *whereframe;
	GtkWidget *matchingframe;
	GtkWidget *repoptbox;
	GtkWidget *patscrolbox;
	GtkWidget *repscrolbox;
	
	GtkWidget *window;
	GtkWidget *patterne;
	GtkWidget *patternv;
	GtkWidget *region_from_beginning;
	GtkWidget *region_from_cursor;
	GtkWidget *region_selection;
	GtkWidget *region_all_open_files;
	GtkWidget *is_normal;
	GtkWidget *is_regex;
	GtkWidget *is_pcre;
	GtkWidget *is_case_sens;
	GtkWidget *overlapping_search;
	gint replace;
	GtkWidget *subpat_help;
	GtkWidget *replacee;
	GtkWidget *replacev;
	GtkWidget *prompt_before_replacing;
	GtkWidget *replace_once;
	GtkWidget *replacetype_string;
	GtkWidget *replacetype_upcase;
	GtkWidget *replacetype_lowcase;
} Tsnr2_win;

/***********************************************************/

static Tlast_snr2 last_snr2;


void snr2_run(void);

/***********************************************************/

static void reset_last_snr2(void) {
	if (last_snr2.pattern) {
		g_free(last_snr2.pattern);
	}
	if (last_snr2.replace_string) {
		g_free(last_snr2.replace_string);
	}
	memset(&last_snr2, 0, sizeof(Tlast_snr2));
	last_snr2.replacetype_string = 1;
}

/***********************************************************/

Tsearch_result search_backend(gchar *pattern, gint matchtype, gint is_case_sens, gchar *buf, gboolean want_submatches) {
	Tsearch_result returnvalue;
	int (*f) ();
	gint buflen, patlen, match, i;

	returnvalue.start = -1;
	returnvalue.end = -1;
	returnvalue.bstart = -1;
	returnvalue.bend = -1;
	returnvalue.pmatch = NULL;
	returnvalue.nmatch = 0;
	if ((!pattern) || (!buf)) {
		DEBUG_MSG("search_backend, pattern or buf is NULL\n");
		return returnvalue;
	}
	
	if (matchtype == match_regex) {
		/* regex part start */
		regex_t reg_pat;
		regmatch_t *pmatch;
		gint nmatch, retval;

		retval = regcomp(&reg_pat, pattern, (is_case_sens ? REG_EXTENDED : REG_EXTENDED | REG_ICASE));
		DEBUG_MSG("search_backend, regcomp retval=%d\n", retval);
		if (retval != 0) {
#define ERRORSTR_SIZE 300
			gchar errorstr[ERRORSTR_SIZE], *errorstr2;
			
			regerror(retval,  &reg_pat, errorstr, ERRORSTR_SIZE);
			errorstr2 = g_strconcat(_("Regular expression error: "), errorstr, NULL);
			error_dialog(_("Bluefish warning: regular expression error"), errorstr2);
			g_free(errorstr2);
			/* error compiling the pattern, returning the default result set,
			which is the 'nothing found' set */
			return returnvalue;
		}
		nmatch = (want_submatches) ? reg_pat.re_nsub+1 : 1;
		DEBUG_MSG("search_backend, expr. contains %d sub patterns\n", reg_pat.re_nsub );
		pmatch = g_malloc(nmatch*sizeof(regmatch_t));
		retval = regexec(&reg_pat, buf, nmatch, pmatch, 0);
		DEBUG_MSG("search_backend, regexec retval=%d\n", retval);
		if (retval != 0) {
			pmatch[0].rm_so = -1;
			pmatch[0].rm_eo = -1;
		}
#ifdef DEBUG
		{	int i;
			for (i=0;i<nmatch;i++) {
				DEBUG_MSG("search_backend, sub pattern %d so=%d, eo=%d\n", i, pmatch[i].rm_so, pmatch[i].rm_eo);
			}
		}
#endif
		returnvalue.bstart = pmatch[0].rm_so;
		returnvalue.bend = pmatch[0].rm_eo;
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
	} else if (matchtype == match_pcre) {
		pcre *pcre_c;
		const char *err=NULL;
		int erroffset=0;
		int ovector[30];
		gint retval;
		pcre_c = pcre_compile(pattern, (is_case_sens ? PCRE_DOTALL|PCRE_MULTILINE : PCRE_DOTALL|PCRE_CASELESS|PCRE_MULTILINE),&err,&erroffset,NULL);
		if (err) {
			gchar *errstring;
			errstring = g_strdup_printf(_("Regular expression error: %s at offset %d"), err, erroffset);
			error_dialog(_("Bluefish warning: regular expression error"), errstring);
			g_free(errstring);
			return returnvalue;/* error compiling the pattern, returning the default result set,which is the 'nothing found' set */
		}
		retval = pcre_exec(pcre_c,NULL,buf,strlen(buf),0,0,ovector,30);
		if (retval > 0) {
			returnvalue.bstart = ovector[0];
			returnvalue.bend = ovector[1];
		} else {
			returnvalue.bstart = -1;
			returnvalue.bend = -1;
		}
		if (want_submatches) {
			int nmatch,i;
			regmatch_t *pmatch;
			pcre_fullinfo(pcre_c, NULL, PCRE_INFO_CAPTURECOUNT, &nmatch);
			DEBUG_MSG("search_backend, nmatch=%d, retval=%d\n", nmatch, retval);
			pmatch = g_malloc((nmatch+1)*sizeof(regmatch_t));
			for (i=0;i<=nmatch;i++) { /* nmatch==1 means 1 subpattern, so 2 patterns in total*/
				pmatch[i].rm_so = ovector[i*2];
				pmatch[i].rm_eo = ovector[i*2+1];
			}
			returnvalue.pmatch = pmatch;
			returnvalue.nmatch = retval;
#ifdef DEBUG
			{	int i;
				for (i=0;i<returnvalue.nmatch;i++) {
					DEBUG_MSG("search_backend, sub pattern %d so=%d, eo=%d\n", i, returnvalue.pmatch[i].rm_so, returnvalue.pmatch[i].rm_eo);
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
		patlen = strlen(pattern);
		
		for (i = 0; i <= (buflen - patlen); i++) {
			match = f(&buf[i], pattern, patlen);
			if (match == 0) {
				returnvalue.bstart = i;
				returnvalue.bend = i + patlen;
				break;
			}
		}
		/* non regex part end */	
	}
	
	/* for multibyte characters */
	if (returnvalue.start >= 0 && returnvalue.end >= 0) {
		utf8_offset_cache_reset();
		if (returnvalue.bstart >= 0) {
			returnvalue.start = utf8_byteoffset_to_charsoffset_cached(buf, returnvalue.bstart);
		}
		if (returnvalue.bend >= 0) {
			returnvalue.end = utf8_byteoffset_to_charsoffset_cached(buf, returnvalue.bend);
		}
		if (want_submatches) {
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

	DEBUG_MSG("search_backend, returning result.start=%d, result.end=%d\n", returnvalue.start, returnvalue.end);
	return returnvalue;
}

/*****************************************************/

Tsearch_result search_doc(Tdocument *document, gchar *pattern, gint matchtype, gint is_case_sens, gint startpos) {
	gchar *fulltext;
	Tsearch_result result;
	
	DEBUG_MSG("search_doc, started on document %p, startpos=%d\n", document, startpos);
	fulltext = doc_get_chars(document, startpos, -1);
	DEBUG_MSG("search_doc, fulltext=%p, pattern=%p\n", fulltext, pattern);
	result = search_backend(pattern, matchtype, is_case_sens, fulltext, FALSE);
	g_free(fulltext);
	if (result.end > 0) {
		DEBUG_MSG("search_doc, received a result (start=%d), adding startpos (%d) to it\n", result.start, startpos);
		result.start += startpos;
		result.end += startpos;
		last_snr2.result.start = result.start;
		last_snr2.result.end = result.end;
		last_snr2.doc = document;
	} else {
		last_snr2.result.start = -1;
		last_snr2.result.end =  -1;
		last_snr2.doc = document;
	}
	DEBUG_MSG("search_doc, result.start=%d, result.end=%d\n", result.start, result.end);
	return result;
}

/*****************************************************/

void doc_show_result(Tdocument *document, gint start, gint end) {
	DEBUG_MSG("doc_show_result, select from start=%d to end=%d\n",start, end);
	if (document != main_v->current_document) {
		switch_to_document_by_pointer(document);
	}
	doc_select_region(document, start, end, TRUE);
}

/*****************************************************/

Tsearch_all_result search_all(gchar *pattern, gint matchtype, gint is_case_sens) {
	GList *tmplist;
	Tsearch_all_result result_all;
	Tsearch_result result;

	DEBUG_MSG("search_all, started\n");
	result_all.start = -1;
	result_all.end = -1;
	result_all.doc = NULL;
	
	if (last_snr2.doc) {
		tmplist = g_list_find(main_v->documentlist, last_snr2.doc);
	} else {
		last_snr2.result.end = 0;
		tmplist = g_list_first(main_v->documentlist);
	}
	while (tmplist) {
		result = search_doc((Tdocument *)tmplist->data, pattern, matchtype, is_case_sens, last_snr2.result.end);
		if (result.end > 0) {
			result_all.start = result.start;
			result_all.end = result.end;
			result_all.doc = tmplist->data;
			DEBUG_MSG("search_all, found!! start=%d, end=%d, doc=%p\n", result.start, result.end, tmplist->data);
			return result_all;
		}
		tmplist = g_list_next(tmplist);
		if (tmplist) {
			last_snr2.result.end = 0;
		}
	}
	DEBUG_MSG("search_all, not found..\n");
	return result_all;
}

/*****************************************************/
/*
this function will parse the replace string and substitute the \0, \1 etc. with 
the subpattern matches from regexec()
*/
static gchar *reg_replace(gchar *replace_string, gint offset, Tsearch_result result, Tdocument *doc) {
	gchar *tmp1, *newstring;
	gchar *tmpstr1, *tmpstr2, *tmpstr3;
	gboolean escaped=0;
	guint from=0, to=0;

	newstring = g_strdup("");
	tmp1 = replace_string;
	while (*tmp1 != '\0') {
		if (escaped) {
			if (*tmp1 == '\\') {
				to++;
				tmpstr1 = newstring;
				tmpstr2 = g_strndup(&replace_string[from], to-from);
				newstring = g_strconcat(tmpstr1, tmpstr2, NULL);
				g_free(tmpstr1);
				g_free(tmpstr2);
				from = ++to;
				DEBUG_MSG("reg_replace, escaped backslash, newstring='%s'\n", newstring);			
			} else if ((gint)*tmp1 >= 48 && (gint)*tmp1 <= 57) {
				gint num = (gint)*tmp1 - 48;
				tmpstr1 = newstring;
				tmpstr2 = g_strndup(&replace_string[from], to-from);
				DEBUG_MSG("reg_replace, from=%d, to=%d, tmpstr2='%s'\n", from, to, tmpstr2);
				if (result.nmatch >= num+1) {
					DEBUG_MSG("reg_replace, wanted: sub pattern %d, offset=%d, so=%d, eo=%d\n", num, offset, result.pmatch[num+1].rm_so, result.pmatch[num+1].rm_eo);
					tmpstr3 = doc_get_chars(doc, offset+result.pmatch[num+1].rm_so, offset+result.pmatch[num+1].rm_eo);
					DEBUG_MSG("reg_replace, subpattern %d = '%s'\n", num, tmpstr3);
				} else {
					DEBUG_MSG("reg_replace, subpattern %d does not exist, nmatch=%d\n", num, result.nmatch);
					tmpstr3 = g_strdup("");
				}
				newstring = g_strconcat(tmpstr1, tmpstr2, tmpstr3, NULL);
				g_free(tmpstr1);
				g_free(tmpstr2);
				g_free(tmpstr3);
				to += 2;
				from = to;
				DEBUG_MSG("reg_replace, substituted subpattern, newstring='%s'\n", newstring);
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
	tmpstr2 = g_strndup(&replace_string[from], to-from);
	newstring = g_strconcat(tmpstr1, tmpstr2, NULL);
	g_free(tmpstr1);
	g_free(tmpstr2);
	DEBUG_MSG("reg_replace, end, newstring='%s'\n", newstring);
	return newstring;
}

Tsearch_result replace_backend(gchar *pattern, gint matchtype, gint is_case_sens
			, gchar *buf, gchar *replace_string, Tdocument *doc, gint offset, replacetypes replacetype
			, gint use_mb, gint *replacelen) {
/* the offset in this function is the difference between the buffer and the text widget because of previous replace 
actions, so the first char in buf is actually number offset in the text widget */
/* replacelen -1 means there is no replacelen known yet, so we have to calculate it */
	Tsearch_result result;
	gchar *tmpstr=NULL;
	
	result = search_backend(pattern, matchtype, is_case_sens, buf, (matchtype != match_normal));
	DEBUG_MSG("replace_backend, offset=%d, result.start=%d, result.end=%d\n", offset, result.start, result.end);
	if (result.end > 0) {
		switch (replacetype) {
		case string:
			if (matchtype == match_normal) {
				tmpstr = g_strdup(replace_string);
			} else {
				tmpstr = reg_replace(replace_string, offset, result, doc);
			}
			DEBUG_MSG("replace_backend, tmpstr='%s'\n", tmpstr);
		break;
		case upcase:
			tmpstr = g_strndup(&buf[result.bstart], result.bend - result.bstart);
			g_strup(tmpstr);
		break;
		case lowcase:
			tmpstr = g_strndup(&buf[result.bstart], result.bend - result.bstart);
			g_strdown(tmpstr);
		break;
		}
		DEBUG_MSG("replace_backend, len=%d, offset=%d, start=%d, end=%d, document=%p\n", result.end - result.start, offset, result.start + offset, result.end + offset, doc);
		doc_replace_text_backend(doc, tmpstr, result.start + offset, result.end + offset);
		if (*replacelen == -1) {
			*replacelen = g_utf8_strlen(tmpstr, -1);
		}
		g_free(tmpstr);
	}
	if (matchtype == match_regex) {
		g_free(result.pmatch);
		result.pmatch = NULL;
	}
	return result;
}

/*****************************************************/

Tsearch_result replace_doc_once(gchar *pattern, gint matchtype, gint is_case_sens, gint startpos, gint endpos, gchar *replace_string, Tdocument *doc, replacetypes replacetype) {
/* endpos -1 means do till end */
	gchar *fulltext;
	gint replacelen = 0; /* replacelen -1 means there is no replacelen known yet
								, but we don't need one, so we set it to 0 */
	Tsearch_result result;

	doc_unre_new_group(doc);
	fulltext = doc_get_chars(doc, startpos, endpos);
	result = replace_backend(pattern, matchtype, is_case_sens, fulltext, replace_string, doc, startpos, replacetype, FALSE, &replacelen);
	if ( result.end > 0) {
		last_snr2.result.start = result.start + startpos;
		last_snr2.result.end = result.end + startpos;
		last_snr2.doc = doc;
	} else {
		last_snr2.result.start = -1;
		last_snr2.result.end = -1;
		last_snr2.doc = doc;
	}
	g_free(fulltext);

	doc_unre_new_group(doc);

	return result;
}
 
/*****************************************************/

void replace_doc_multiple(gchar *pattern, gint matchtype, gint is_case_sens, gint startpos, gint endpos, gchar *replace_string, Tdocument *doc, replacetypes replacetype) {
/* endpos -1 means do till end */
	gchar *fulltext;
	Tsearch_result result;
	gint in_buf_offset=0;
	gint buf_text_offset=startpos;
	gint replacelen=-1; /* replacelen -1 means there is no replacelen known yet */


	doc_unre_new_group(doc);

	DEBUG_MSG("replace_doc_multiple, startpos=%d, endpos=%d\n", startpos, endpos);
	if (matchtype == match_normal || replacetype != string) {
		/* the replace string has a fixed length if it is not regex, or it is not type string */
/*		if (GTK_TEXT(doc->textbox)->use_wchar) {
			replacelen = wchar_len(replace_string, -1);
		} else {*/
			replacelen = strlen(replace_string);
/*		}*/
	}
	fulltext = doc_get_chars(doc, startpos, endpos);
	result = replace_backend(pattern, matchtype, is_case_sens, fulltext, replace_string, doc, buf_text_offset, replacetype, FALSE, &replacelen);
	while (result.end > 0) {
		if (replacetype == string) {
			buf_text_offset += replacelen - (result.end - result.start);
		}
		if (last_snr2.overlapping_search) {
/*			if (GTK_TEXT(doc->textbox)->use_wchar) {
				onechar = gtk_editable_get_chars(GTK_EDITABLE(doc->textbox), startpos + result.start, startpos + result.start + 1);
				if (onechar) {
					in_buf_offset += result.bstart + strlen(onechar);
				} else {
					in_buf_offset += result.bstart + 1;
				}
				g_free(onechar);
			} else {*/
				in_buf_offset += result.bstart + 1;
/*			}*/
			buf_text_offset += result.start + 1;
		} else {
			in_buf_offset += result.bend;
			buf_text_offset += result.end;
		}
		DEBUG_MSG("replace_doc_multiple, after first search, buf_text_offset=%d, in_buf_offset=%d\n", buf_text_offset, in_buf_offset);
		if (matchtype != match_normal && replacetype == string) {
			/* all regex replaces can have different replace lengths, so they have to be re-calculated */
			replacelen = -1;
		}
		result = replace_backend(pattern, matchtype, is_case_sens, &fulltext[in_buf_offset], replace_string, doc, buf_text_offset, replacetype, FALSE, &replacelen);

		DEBUG_MSG("replace_doc_multiple, 1- buf_text_offset=%d, in_buf_offset=%d, result.start=%d, result.end=%d\n", buf_text_offset, in_buf_offset, result.start, result.end);
	}

	doc_unre_new_group(doc);

	last_snr2.result.start = -1;
	last_snr2.result.end = -1;
	last_snr2.doc = doc;
	g_free(fulltext);
}

/*****************************************************/

void replace_all(gchar *pattern, gint matchtype, gint is_case_sens, gchar *replace_string, replacetypes replacetype) {
	GList *tmplist;

	tmplist = g_list_first(main_v->documentlist);
	while (tmplist) {
		replace_doc_multiple(pattern, matchtype, is_case_sens, 0, -1, replace_string, (Tdocument *)tmplist->data, replacetype);
		tmplist = g_list_next(tmplist);
	}
}

/*****************************************************/

static void replace_prompt_dialog_ok_lcb(GtkWidget *widget, gpointer data) {
	gchar *tmpstr;
	gint sel_start_pos, sel_end_pos;

	window_close_by_widget_cb(widget, data);
	
	doc_get_selection(main_v->current_document, &sel_start_pos, &sel_end_pos);
		if ((sel_start_pos == last_snr2.result.start) &&
					(sel_end_pos == last_snr2.result.end)) {

			if (last_snr2.replacetype_string) {
				tmpstr = g_strdup(last_snr2.replace_string);
				/* if it was a regex replace we need to do the sub-pattern matching */
				tmpstr = reg_replace(tmpstr, 0, last_snr2.result, main_v->current_document);
				
			} else if (last_snr2.replacetype_upcase) {
				tmpstr = doc_get_chars(main_v->current_document, last_snr2.result.start ,last_snr2.result.end);
				g_strup(tmpstr);
			} else {
				tmpstr = doc_get_chars(main_v->current_document, last_snr2.result.start ,last_snr2.result.end);
				g_strdown(tmpstr);
			}
			/* avoid new highlighting at this stage, so call the backend directly instead of the frontend function
			this because the highlighting interferes with the selection
			the better solution is to have the highlighting handle the selection better, 
			the problem starts in document.c in get_positions() because the selection is not saved there
			I don't know why the selection is gray, but that's basically the reason why it doesn't save the selection
			 */
			doc_unre_new_group(main_v->current_document);
			doc_replace_text_backend(main_v->current_document, tmpstr, last_snr2.result.start,last_snr2.result.end);
			doc_unre_new_group(main_v->current_document);
			doc_set_modified(main_v->current_document, 1);
			
			g_free(tmpstr);
			if (last_snr2.result.pmatch) {
				g_free(last_snr2.result.pmatch);
				last_snr2.result.pmatch = NULL;
			}
			if (!last_snr2.replace_once) {
				snr2_run();
			}
		}
#ifdef DEBUG
		 else {
			g_print("replace_prompt_dialog_ok_lcb, selection != result, not replacing!!\n");
		}
#endif /* DEBUG */
}

static void replace_prompt_dialog_all_lcb(GtkWidget *widget, gpointer data) {
	window_close_by_widget_cb(widget, data);
	last_snr2.prompt_before_replacing = 0;
	last_snr2.result.start--;
	last_snr2.result.end = last_snr2.result.start;
	if (last_snr2.result.pmatch) {
		g_free(last_snr2.result.pmatch);
		last_snr2.result.pmatch = NULL;
	}
	snr2_run();
}

void replace_prompt_dialog() {
	GtkWidget *win, *vbox, *hbox;
	GtkWidget *butok, *butcancel, *butall;

	DEBUG_MSG("replace_prompt_dialog, start\n");
	win = window_full(_("Confirm replace"), GTK_WIN_POS_MOUSE, 5, G_CALLBACK(window_close_by_widget_cb), NULL, TRUE);
	vbox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(win), vbox);
	gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(_("Are you sure you want to replace this?")), FALSE, FALSE,3);
	hbox = gtk_hbox_new(TRUE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE,3);
	butok = bf_stock_ok_button(G_CALLBACK(replace_prompt_dialog_ok_lcb), win);
	butcancel = bf_stock_cancel_button(G_CALLBACK(window_close_by_widget_cb), NULL);
	butall = bf_stock_button(_("All"), G_CALLBACK(replace_prompt_dialog_all_lcb), win);
	gtk_box_pack_start(GTK_BOX(hbox), butok, TRUE, TRUE,3);
	gtk_box_pack_start(GTK_BOX(hbox), butcancel, TRUE, TRUE,3);
	gtk_box_pack_start(GTK_BOX(hbox), butall, TRUE, TRUE,3);
	gtk_widget_show_all(win);
	DEBUG_MSG("replace_prompt_dialog, end\n");
}

gint replace_prompt_doc(gchar *pattern, gint matchtype, gint is_case_sens, gint startpos, gint endpos, gchar *replace_string, Tdocument *doc) {
/* endpos -1 means do till end , returns if the document still had a match*/
	gchar *fulltext;
	Tsearch_result result;

	if (last_snr2.result.pmatch) {
		g_free(last_snr2.result.pmatch);
		last_snr2.result.pmatch = NULL;
	}
	fulltext = doc_get_chars(doc, startpos, endpos);
	result = search_backend(pattern, matchtype, is_case_sens, fulltext, 1);
	last_snr2.doc = doc;
	g_free(fulltext);
	DEBUG_MSG("replace_prompt_doc, result.end=%d\n", result.end);
	if (result.end > 0) {
		gint i;
		last_snr2.result.start = result.start + startpos;
		last_snr2.result.end = result.end + startpos;
		last_snr2.result.nmatch = result.nmatch;
		last_snr2.result.pmatch = result.pmatch;
		for (i=0;i<result.nmatch;i++) {
			DEBUG_MSG("replace_prompt_doc, adding offset %d to subpattern %d\n", startpos, i);
			last_snr2.result.pmatch[i].rm_so += startpos;
			last_snr2.result.pmatch[i].rm_eo += startpos;
		}
		doc_show_result(doc, result.start + startpos, result.end + startpos);
		replace_prompt_dialog();
		return 1;
	} else {
		g_free(result.pmatch);
		return 0;
	}
}

void replace_prompt_all(gchar *pattern, gint matchtype, gint is_case_sens, gchar *replace_string) {
	GList *tmplist;
	gint retvalue;
	Tdocument *tmpdoc;

	if (last_snr2.doc) {
		tmpdoc = last_snr2.doc;
	} else {
		tmplist = g_list_first(main_v->documentlist);
		tmpdoc = (Tdocument *)tmplist->data;
	}
	retvalue = replace_prompt_doc(pattern, matchtype, is_case_sens, 0, -1, replace_string, tmpdoc);
	while (retvalue == 0) {
		tmplist = g_list_find(main_v->documentlist, last_snr2.doc);
		tmplist = g_list_next(tmplist);
		if (tmplist) {
			retvalue = replace_prompt_doc(pattern, matchtype, is_case_sens, 0, -1, replace_string, (Tdocument *)tmplist->data);
		} else {
			retvalue = 1;
		}
	}
}

/*****************************************************/

void snr2_run(void) {
	gint startpos, endpos;
	Tsearch_result result;
	Tsearch_all_result result_all;
	replacetypes replacetype;

	if (last_snr2.result.pmatch) {
		g_free(last_snr2.result.pmatch);
		last_snr2.result.pmatch = NULL;
	}

	/* should be more stuff here */
	if (last_snr2.region_from_beginning) {
		startpos = 0;
		endpos = -1;
	} else if (last_snr2.region_from_cursor) {
		startpos = doc_get_cursor_position(main_v->current_document);
		endpos = -1;
	} else if (last_snr2.region_selection) {
		if (!doc_get_selection(main_v->current_document,&startpos,&endpos)) {
			/* what to do if there was no selection ?*/
			DEBUG_MSG("snr2_run, no selection found, returning\n");
			return;
		}
		DEBUG_MSG("snr2_run, from selection: startpos=%d, endpos=%d\n", startpos, endpos);
	}
	if (last_snr2.doc == main_v->current_document) {
		if (last_snr2.result.end > 0) {
			if (last_snr2.overlapping_search) {
				startpos = last_snr2.result.start + 1;
			} else {
				startpos = last_snr2.result.end;
			}
		}
		DEBUG_MSG("snr2_run, last_snr2.result.end=%d, startpos=%d\n", last_snr2.result.end, startpos);
	}	
	if (last_snr2.replace) {
		if (last_snr2.replacetype_string) {
			replacetype = string;
		} else if (last_snr2.replacetype_upcase) {
			replacetype = upcase;
		} else {
			replacetype = lowcase;
		}
	
		if (last_snr2.prompt_before_replacing) {
			if (last_snr2.region_all_open_files) {
				replace_prompt_all(last_snr2.pattern,last_snr2.matchtype, last_snr2.is_case_sens, last_snr2.replace_string);
			} else {
				replace_prompt_doc(last_snr2.pattern, last_snr2.matchtype, last_snr2.is_case_sens, startpos, endpos, last_snr2.replace_string, main_v->current_document);
			}
		} else {
			if (last_snr2.region_all_open_files) {
				replace_all(last_snr2.pattern, last_snr2.matchtype, last_snr2.is_case_sens, last_snr2.replace_string, replacetype);
			} else if (last_snr2.replace_once) {
				replace_doc_once(last_snr2.pattern, last_snr2.matchtype, last_snr2.is_case_sens, startpos, endpos, last_snr2.replace_string, main_v->current_document, replacetype);
			} else {
				replace_doc_multiple(last_snr2.pattern, last_snr2.matchtype, last_snr2.is_case_sens, startpos, endpos, last_snr2.replace_string, main_v->current_document, replacetype);
			}		
		}
	} else { /* find, not replace */
		if (last_snr2.region_all_open_files) {
			DEBUG_MSG("snr2dialog_ok_lcb, search = all\n");
			result_all = search_all(last_snr2.pattern, last_snr2.matchtype, last_snr2.is_case_sens);
			DEBUG_MSG("snr2dialog_ok_lcb, result_all.doc=%p\n",result_all.doc);
			if (result_all.end > 0) {
				doc_show_result(result_all.doc, result_all.start, result_all.end);
			} else {
				error_dialog(_("Bluefish message"), _("Search: no match found"));
			}
		} else {
			result = search_doc(main_v->current_document, last_snr2.pattern, last_snr2.matchtype, last_snr2.is_case_sens, startpos);
			if (result.end > 0) {
				doc_show_result(main_v->current_document, result.start, result.end);	
			} else {
				error_dialog(_("Bluefish message"), _("Search: no match found"));
			}
		}
	}
	/* if highlighting is needed for this document do this now !! */
	if (main_v->current_document->need_highlighting && main_v->current_document->highlightstate) {
		doc_highlight_full(main_v->current_document);
	}
}

/* void snr2_run_extern_replace
arguments are translated to last_snr2 like this:
pattern = patten
region,  0 = region_from_beginning, 
			1 = region_from_cursor, 
			2 = region_selection,
			3 = region_all_open_files
matchtype = matchtype, 0=normal, 1=posix, 2=pcre
is_case_sens = is_case_sens
overlapping_search is off
replace = 1
replace_string = replace_string
prompt_before_replacing = off
replace_once = off
replacetype_string = 1
replacetype_upcase = 0
replacetype_lowcase = 0
*/
void snr2_run_extern_replace(gchar *pattern, gint region,
							gint matchtype, gint is_case_sens, gchar *replace_string,
							gboolean store_as_last_snr2) {
	gchar *pattern_bck = last_snr2.pattern, *replace_string_bck = last_snr2.pattern;
	Tlast_snr2 last_snr2_bck = last_snr2;
	if (!pattern || !replace_string || !strlen(pattern)) {
		DEBUG_MSG("snr2_run_extern, returning, non-valid arguments\n");
		return;
	}
	last_snr2.pattern = g_strdup(pattern);
	last_snr2.region_from_beginning = region == 0 ? 1: 0;
	last_snr2.region_from_cursor = region == 1 ? 1: 0;
	last_snr2.region_selection = region == 2 ? 1: 0;
	last_snr2.region_all_open_files = region == 3 ? 1: 0;
	last_snr2.matchtype = matchtype;
 	last_snr2.is_case_sens = is_case_sens;
 	last_snr2.overlapping_search = 0;
	last_snr2.replace = 1;
	last_snr2.replace_string = g_strdup(replace_string);
 	last_snr2.prompt_before_replacing = 0;
 	last_snr2.replace_once = 0;
	last_snr2.replacetype_string = 1;
	last_snr2.replacetype_upcase = 0;
	last_snr2.replacetype_lowcase = 0;

	snr2_run();
	if (store_as_last_snr2) {
		g_free(pattern_bck);
		g_free(replace_string_bck);
	} else {
		g_free(last_snr2.pattern);
		g_free(last_snr2.replace_string);
		last_snr2 = last_snr2_bck;
	}
}

Tsearch_result doc_search_run_extern(gchar *pattern, gint matchtype, gint is_case_sens) {
	return search_doc(main_v->current_document, pattern, matchtype, is_case_sens, 0);
} 

/*****************************************************/

static void snr2dialog_destroy_lcb(GtkWidget *widget, GdkEvent *event, gpointer data) {
	DEBUG_MSG("snr2dialog_destroy_lcb, started, about to call window_destroy\n");
	window_destroy(((Tsnr2_win *)data)->window);
	DEBUG_MSG("snr2dialog_destroy_lcb, about to free data %p\n", data);
	g_free(data);
	DEBUG_MSG("snr2dialog_destroy_lcb, done\n");
}

static void snr2dialog_cancel_lcb(GtkWidget *widget, gpointer data) {
	snr2dialog_destroy_lcb(NULL, NULL, data);
}

/*****************************************************/

static void snr2dialog_ok_lcb(GtkWidget *widget, Tsnr2_win *data) {
	if (last_snr2.pattern) {
		g_free(last_snr2.pattern);
		last_snr2.pattern = NULL;
	}
	if (last_snr2.replace_string) {
		g_free(last_snr2.replace_string);
		last_snr2.replace_string = NULL;
	}
	if (data->is_advanced){
		GtkTextIter itstart, itend;
		GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data->patternv));
		gtk_text_buffer_get_bounds(buf,&itstart,&itend);
		last_snr2.pattern = gtk_text_buffer_get_text(buf,&itstart,&itend, FALSE);
	} else {
		last_snr2.pattern = gtk_editable_get_chars(GTK_EDITABLE(data->patterne), 0, -1);
	}
	last_snr2.region_from_beginning = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->region_from_beginning));
	last_snr2.region_from_cursor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->region_from_cursor));
	last_snr2.region_selection = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->region_selection));
	last_snr2.region_all_open_files = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->region_all_open_files));
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->is_regex))) {
		last_snr2.matchtype = match_regex;
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->is_pcre))){
		last_snr2.matchtype = match_pcre;
	} else {
		last_snr2.matchtype = match_normal;
	}
 	last_snr2.is_case_sens = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->is_case_sens));
 	last_snr2.overlapping_search = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->overlapping_search));
	if (data->replace) {
		last_snr2.replace = 1;
		if (data->is_advanced){
			GtkTextIter itstart, itend;
			GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data->replacev));
			gtk_text_buffer_get_bounds(buf,&itstart,&itend);
			last_snr2.replace_string = gtk_text_buffer_get_text(buf,&itstart,&itend, FALSE);
		} else {
			last_snr2.replace_string = gtk_editable_get_chars(GTK_EDITABLE(data->replacee), 0, -1);
		}
	 	last_snr2.prompt_before_replacing = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->prompt_before_replacing));
	 	last_snr2.replace_once = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->replace_once));
		last_snr2.replacetype_string = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->replacetype_string)));
		last_snr2.replacetype_upcase = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->replacetype_upcase)));
		last_snr2.replacetype_lowcase = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->replacetype_lowcase)));
	} else {
		last_snr2.replace = 0;
	}
	window_destroy(data->window);
	g_free(data);

	snr2_run();
}

static void matchingtype_toggled_lcb(GtkToggleButton *togglebutton,Tsnr2_win *snr2win){
	DEBUG_MSG("matchingtype_toggled_lcb, started\n");
	if (GTK_TOGGLE_BUTTON(snr2win->is_normal)->active) {
		gtk_widget_hide(snr2win->subpat_help);
	} else {
		gtk_widget_show(snr2win->subpat_help);
	}
}
static void snr2dialog_replacetype_toggled(GtkWidget *widget, Tsnr2_win *snr2win) {
/*	gtk_text_view_set_editable(GTK_TEXT_VIEW(snr2win->replacev), GTK_TOGGLE_BUTTON(snr2win->replacetype_string)->active);*/
	gtk_widget_set_sensitive(snr2win->replacev, GTK_TOGGLE_BUTTON(snr2win->replacetype_string)->active);
}

static gboolean patternv_key_press_event_lcb(GtkWidget *widget,GdkEventKey *event,Tsnr2_win *snr2win) {
	if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_Return)) {
		snr2dialog_ok_lcb(NULL, snr2win);
		return TRUE;
	}
	return FALSE;
}

static void snr2dialog_advanced_button_clicked(GtkWidget *button, Tsnr2_win *snr2win) {
	if (button) {
		snr2win->is_advanced = 1 - snr2win->is_advanced;
	} else {
		/* if button==NULL we are called by the snr2win() function, so we should not toggle */
		
	}
	if (snr2win->is_advanced) {
		const gchar *tmp;
		gtk_button_set_label(GTK_BUTTON(snr2win->advanced_button), _("Hide advanced <<"));
		gtk_widget_show(snr2win->matchingframe);
		gtk_widget_show(snr2win->whereframe);
		gtk_widget_show(snr2win->overlapping_search);
		gtk_widget_show(snr2win->patscrolbox);
		gtk_widget_hide(snr2win->patterne);
		tmp = gtk_entry_get_text(GTK_ENTRY(snr2win->patterne));
		gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(snr2win->patternv)), tmp, -1);
		if (snr2win->replace) {
			gtk_widget_show(snr2win->repoptbox);
			gtk_widget_show(snr2win->repscrolbox);
			gtk_widget_hide(snr2win->replacee);
			tmp = gtk_entry_get_text(GTK_ENTRY(snr2win->replacee));
			gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(snr2win->replacev)), tmp, -1);
		}
	} else {
		gchar *tmp;
		gtk_button_set_label(GTK_BUTTON(snr2win->advanced_button), _("Show advanced >>"));
		gtk_widget_hide(snr2win->matchingframe);
		gtk_widget_hide(snr2win->whereframe);
		gtk_widget_hide(snr2win->overlapping_search);
		gtk_widget_hide(snr2win->patscrolbox);
		gtk_widget_show(snr2win->patterne);
		{
			GtkTextIter itstart, itend;
			GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(snr2win->patternv));
			gtk_text_buffer_get_bounds(buf,&itstart,&itend);
			tmp = gtk_text_buffer_get_text(buf,&itstart,&itend, FALSE);
			gtk_entry_set_text(GTK_ENTRY(snr2win->patterne),tmp);
			g_free(tmp);
		}
		if (snr2win->replace) {
			gtk_widget_hide(snr2win->repoptbox);
			gtk_widget_hide(snr2win->repscrolbox);
			gtk_widget_show(snr2win->replacee);
			{
				GtkTextIter itstart, itend;
				GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(snr2win->replacev));
				gtk_text_buffer_get_bounds(buf,&itstart,&itend);
				tmp = gtk_text_buffer_get_text(buf,&itstart,&itend, FALSE);
				gtk_entry_set_text(GTK_ENTRY(snr2win->replacee),tmp);
				g_free(tmp);
			}
		}
		
		
		/* resize the window to its smallest size */
		gtk_window_resize(GTK_WINDOW(snr2win->window),20,20);
	}
}

static void snr2dialog(gint is_replace, gint is_new_search) {

	Tsnr2_win *snr2win;
	GtkWidget *vbox, *hbox, *vbox2, *vbox3, *frame, *but;
	gchar *tmptext;

	snr2win = g_malloc(sizeof(Tsnr2_win));
	snr2win->is_advanced = main_v->props.default_advanced_snr;
	if (is_replace) {
		tmptext = _("Replace");
		snr2win->replace = 1;
	} else {
		tmptext = _("Search");
		snr2win->replace = 0;
	}
	if (is_new_search) {
		reset_last_snr2();
	} else {
		last_snr2.result.start = -1;
		last_snr2.result.end = -1;
		last_snr2.doc = NULL;
	}
	snr2win->window = window_full(tmptext, GTK_WIN_POS_MOUSE, 5, G_CALLBACK(snr2dialog_destroy_lcb), snr2win, TRUE);
	gtk_window_set_role(GTK_WINDOW(snr2win->window), "snr");
	vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(snr2win->window), vbox);
	
	hbox = gtk_hbox_new(FALSE,0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Find: ")), FALSE, FALSE, 0);
	snr2win->patterne = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), snr2win->patterne, TRUE, TRUE, 0);
	{
		snr2win->patscrolbox = textview_buffer_in_scrolwin(&snr2win->patternv, 300, 50, last_snr2.pattern, GTK_WRAP_NONE);
		gtk_box_pack_start(GTK_BOX(hbox), snr2win->patscrolbox, TRUE, TRUE, 0);
		g_signal_connect(G_OBJECT(snr2win->patternv), "key_press_event", G_CALLBACK(patternv_key_press_event_lcb), snr2win);
	}

	snr2win->whereframe = gtk_frame_new(_("Where"));
	gtk_box_pack_start(GTK_BOX(vbox), snr2win->whereframe, FALSE, FALSE, 0);
	vbox2 = gtk_vbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(snr2win->whereframe), vbox2);
	snr2win->region_from_beginning = boxed_radiobut_with_value(_("Start at beginning of document"), last_snr2.region_from_beginning, NULL, vbox2);
	snr2win->region_from_cursor = boxed_radiobut_with_value(_("Start at cursor position"), last_snr2.region_from_cursor, GTK_RADIO_BUTTON(snr2win->region_from_beginning),  vbox2);
	snr2win->region_selection = boxed_radiobut_with_value(_("In selection"), last_snr2.region_selection, GTK_RADIO_BUTTON(snr2win->region_from_cursor), vbox2);
	snr2win->region_all_open_files = boxed_radiobut_with_value(_("In all open files"), last_snr2.region_all_open_files, GTK_RADIO_BUTTON(snr2win->region_selection), vbox2);
	
	snr2win->matchingframe = gtk_frame_new(_("Matching"));
	gtk_box_pack_start(GTK_BOX(vbox), snr2win->matchingframe, FALSE, FALSE, 0);
	vbox2 = gtk_vbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(snr2win->matchingframe), vbox2);
	
	snr2win->is_normal = boxed_radiobut_with_value(_("Normal matching"), (last_snr2.matchtype == match_normal), NULL, vbox2);
	snr2win->is_regex = boxed_radiobut_with_value(_("Use posix regular expression matching"),(last_snr2.matchtype == match_regex),GTK_RADIO_BUTTON(snr2win->is_normal), vbox2);
	snr2win->is_pcre = boxed_radiobut_with_value(_("Use perl compatible regular expression matching"),(last_snr2.matchtype == match_pcre),GTK_RADIO_BUTTON(snr2win->is_normal),vbox2);
	if (is_replace) {
		g_signal_connect(G_OBJECT(snr2win->is_normal), "toggled", G_CALLBACK(matchingtype_toggled_lcb), snr2win);
	}
	
	snr2win->is_case_sens = boxed_checkbut_with_value(_("Case sensitive"), last_snr2.is_case_sens, vbox);
	snr2win->overlapping_search = boxed_checkbut_with_value(_("Overlapping searches"), last_snr2.overlapping_search, vbox);
	
	if (is_replace) {
		frame = gtk_frame_new(_("Replace"));
		gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
		vbox2 = gtk_vbox_new(FALSE, 3);
		gtk_container_add(GTK_CONTAINER(frame), vbox2);
		snr2win->subpat_help = gtk_label_new(_("\\0 refers to the first subpattern, \\1 to the second etc."));
		gtk_box_pack_start(GTK_BOX(vbox2), snr2win->subpat_help, FALSE, TRUE, 0);
		hbox = gtk_hbox_new(FALSE,0);
		gtk_box_pack_start(GTK_BOX(vbox2), hbox, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Replace with: ")), FALSE, FALSE, 0);
		snr2win->replacee = gtk_entry_new();
		gtk_box_pack_start(GTK_BOX(hbox), snr2win->replacee, TRUE, TRUE, 0);
		{
			snr2win->repscrolbox = textview_buffer_in_scrolwin(&snr2win->replacev, 300, 50, last_snr2.replace_string, GTK_WRAP_NONE);
			gtk_box_pack_start(GTK_BOX(hbox), snr2win->repscrolbox, TRUE, TRUE, 0);
			g_signal_connect(G_OBJECT(snr2win->replacev), "key_press_event", G_CALLBACK(patternv_key_press_event_lcb), snr2win);
		}
		if (last_snr2.replacetype_upcase || last_snr2.replacetype_lowcase) {
/*			gtk_text_view_set_editable(GTK_TEXT_VIEW(snr2win->replacev), FALSE);*/
			gtk_widget_set_sensitive(snr2win->replacev, FALSE);
		}
		snr2win->repoptbox = gtk_hbox_new(TRUE,3);
		gtk_box_pack_start(GTK_BOX(vbox2), snr2win->repoptbox, FALSE, TRUE, 3);

		vbox3 = gtk_vbox_new(TRUE,3);
		gtk_box_pack_start(GTK_BOX(snr2win->repoptbox), vbox3, FALSE, TRUE, 3);	
		
		snr2win->replacetype_string = boxed_radiobut_with_value(_("Replace text"), last_snr2.replacetype_string, NULL, vbox3);
		snr2win->replacetype_upcase = boxed_radiobut_with_value(_("Replace uppercase"), last_snr2.replacetype_upcase, GTK_RADIO_BUTTON(snr2win->replacetype_string), vbox3);
		snr2win->replacetype_lowcase = boxed_radiobut_with_value(_("Replace lowercase"), last_snr2.replacetype_lowcase, GTK_RADIO_BUTTON(snr2win->replacetype_string), vbox3);

		vbox3 = gtk_vbox_new(TRUE,3);
		gtk_box_pack_start(GTK_BOX(snr2win->repoptbox), vbox3, FALSE, TRUE, 3);	
		snr2win->prompt_before_replacing = boxed_checkbut_with_value(_("Prompt before replacing"), last_snr2.prompt_before_replacing, vbox3);
		snr2win->replace_once = boxed_checkbut_with_value(_("Replace once"), last_snr2.replace_once, vbox3);
		g_signal_connect(G_OBJECT(snr2win->replacetype_string), "toggled", 
				G_CALLBACK(snr2dialog_replacetype_toggled), snr2win);
	}
	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 1);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	snr2win->advanced_button = gtk_button_new_with_label(_("Show advanced >>"));
	g_signal_connect(G_OBJECT(snr2win->advanced_button), "clicked", G_CALLBACK(snr2dialog_advanced_button_clicked), snr2win);
	gtk_box_pack_start(GTK_BOX(hbox), snr2win->advanced_button, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(hbox), bf_stock_cancel_button(G_CALLBACK(snr2dialog_cancel_lcb), snr2win), FALSE, TRUE, 0);
	but = bf_stock_ok_button(G_CALLBACK(snr2dialog_ok_lcb), snr2win);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, TRUE, 0);
	gtk_window_set_default(GTK_WINDOW(snr2win->window), but);

	
	/* now select all text */
	if (snr2win->is_advanced) {
		GtkTextIter start,end;
		GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(snr2win->patternv));
		gtk_text_buffer_get_bounds(buffer,&start,&end);
		gtk_text_buffer_move_mark_by_name(buffer,"insert",&start);
		gtk_text_buffer_move_mark_by_name(buffer,"selection_bound",&end);
		gtk_widget_grab_focus(snr2win->patternv);
	} else {
		gtk_widget_grab_focus(snr2win->patterne);
	}
	gtk_widget_show_all(vbox);

	snr2dialog_advanced_button_clicked(NULL, snr2win);
	gtk_widget_show(snr2win->window);		
	if (is_replace) {
		matchingtype_toggled_lcb(NULL, snr2win);
	}
}

/*****************************************************/

void search_cb(GtkWidget *widget, gpointer data) {
	snr2dialog(0, 0);
}

void new_search_cb(GtkWidget *widget, gpointer data) {
	snr2dialog(0, 1);
}

void search_again_cb(GtkWidget *widget, gpointer data) {
	snr2_run();	
}

void replace_again_cb(GtkWidget *widget, gpointer data) {
	snr2_run();
}

void replace_cb(GtkWidget *widget, gpointer data) {
	snr2dialog(1, 0);
}

void new_replace_cb(GtkWidget *widget, gpointer data) {
	snr2dialog(1, 1);
}

/*****************************************************/

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
		/* code to create replace_string */
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
/*		DEBUG_MSG("do_filename_change_replace, olddirfile=%s does NOT exist\n", olddirfile);  */
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

void update_filenames_in_file(Tdocument *doc, gchar *oldfilename, gchar *newfilename, gint doc_has_newfilename) {
	gchar *fulltext;
	Tsearch_result result;
	gint cur_offset, changelen=0;
	gchar *pattern;
	gint matchtype, is_case_sens;
	gchar *olddirname=NULL;

	if ((oldfilename == NULL)|| (newfilename == NULL)) {
		return;
	}
	pattern = "\"[^\"]+\"";
	matchtype = match_regex;
 	is_case_sens = 0;
	cur_offset = 0;

	if (doc_has_newfilename) {
			olddirname = g_dirname(oldfilename);
	}

	fulltext = doc_get_chars(doc, 0, -1);
	result = search_backend(pattern, matchtype, is_case_sens, fulltext, 0);
	while (result.end > 0) {
		if (doc_has_newfilename) {
			changelen = do_filename_curfile_replace(fulltext, result, cur_offset, olddirname, newfilename, changelen, doc);
		} else {
			changelen = do_filename_otherfile_replace(fulltext, result, cur_offset, oldfilename, newfilename, changelen, doc);
		}
		cur_offset += result.bstart +1;
		result = search_backend(pattern, matchtype, is_case_sens, &fulltext[cur_offset], 0);
	}
	g_free(fulltext);
	if (doc_has_newfilename) {
		g_free(olddirname);
	}
}

void update_encoding_meta_in_file(Tdocument *doc, gchar *encoding) {
	if (encoding) {
		Tlast_snr2 last_snr2_bck = last_snr2;
		gchar *last_pattern_bck = g_strdup(last_snr2.pattern);
		gchar *pattern, *fulltext;
		Tsearch_result result;
		/* first find if there is a meta encoding tag already */
		pattern = "<meta[ \t\n]http-equiv[ \t\n]*=[ \t\n]*\"content-type\"[ \t\n]+content[ \t\n]*=[ \t\n]*\"text/html;[ \t\n]*charset=[a-z0-9-]+\"[ \t\n]*>";
		fulltext = doc_get_chars(doc, 0, -1);
		result = search_backend(pattern, match_regex, 0, fulltext, 0);
		if (result.end > 0) {
			gchar *replacestring = g_strconcat("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=",encoding,"\">", NULL);
			DEBUG_MSG("update_encoding_meta_in_file, 1: we have a match\n");
			doc_replace_text(doc, replacestring, result.start, result.end);
			g_free(replacestring);
		} else {
			DEBUG_MSG("update_encoding_meta_in_file, 1: NO match\n");
			/* now search for <head>, we can append it to this tag */
			pattern = "<head>";
			result = search_backend(pattern, match_regex, 0, fulltext, 0);
			if (result.end > 0) {
				gchar *replacestring = g_strconcat("<head>\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=",encoding,"\">", NULL);
				DEBUG_MSG("update_encoding_meta_in_file, 2: we have a match\n");
				doc_replace_text(doc, replacestring, result.start, result.end);
				g_free(replacestring);
			} else {
				DEBUG_MSG("update_encoding_meta_in_file, 2: NO match\n");
			}
		}
		g_free(fulltext);
		g_free(last_snr2.pattern);
		last_snr2 = last_snr2_bck;
		last_snr2.pattern = last_pattern_bck;
	}
}

