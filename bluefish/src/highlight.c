/* Bluefish HTML Editor
 * highlight2.c - the syntax highlighting with perl compatible regular expressions
 *
 * Copyright (C) 2002 Olivier Sessink
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
 *
 *
 * indenting is done with
 * indent --line-length 100 --k-and-r-style --tab-size 4 -bbo --ignore-newlines highlight.c
 */
/*#define HL_TIMING
#define HL_DEBUG
#define DEBUG*/

#ifdef HL_TIMING
#include <sys/times.h>
#include <unistd.h>
#endif

#include <gtk/gtk.h>
#include <sys/types.h>			/* _before_ regex.h for freeBSD */
#include <pcre.h>				/* pcre_*() */
#include <string.h>				/* strerror() */
#include <stdlib.h>				/* atoi() */

#include "bluefish.h"
#include "bf_lib.h"				/* filename_test_extensions() */
#include "rcfile.h"				/* array_from_arglist() */
#include "stringlist.h" 	/* count_array() */
#include "menu.h" 			/* menu_current_document_type_set_active_wo_activate */
#include "document.h" /* doc_get_chars() */
#include "highlight.h"


#define NUM_SUBMATCHES 30 /* should be a multiple of three for pcre_exec(), and at maximum 2/3 of this size can really be used for substrings */

typedef struct {
	pcre *pcre;
	pcre_extra *pcre_e;
} Treg;

typedef struct {
	gchar *name;
	GtkTextTag *tag;
} Thlstyle;

typedef struct {
	Treg reg1;
	Treg reg2;
	gint numsub;
	gint mode;
	GList *childs;
	GtkTextTag *tag;
} Tpattern;

typedef struct {
	gchar *filetype;
	gchar *name;
	gchar *parentmatch;
	pcre *pcre;
	Tpattern *pat;
} Tmetapattern; /* mostly used during compiling since it contains some more meta-info about the pattern */

typedef struct {
	GtkTextTagTable *tagtable; /* this one should ultimately move to Tfiletype, so every set would have it's own tagtable, but there is currently no way to switch a document to a new tagtable */
	GList *highlight_filetypes; /* contains all filetypes that have a highlight pattern */
	GList *all_highlight_patterns; /* contains Tmetapattern, not Tpattern !! */
} Thighlight;

typedef struct {
	int ovector[NUM_SUBMATCHES];
	gboolean is_match;
	Tpattern *pat;
} Tpatmatch;

/***************************************************************
how it works:

-types:
	1 - a start pattern and an end pattern
	2 - a pattern that defines the complete match
	3 - a sub-pattern , this must be a child pattern for type 2

-matching
   all top-level patterns are matched, a matching table is formed
	with all the start positions for every type, the lowest start 
	position is the pattern where we start.
	for this pattern we crate a new matching table with all the start 
	values AND the end value for the parent pattern. If the parent 
	pattern is type 2 or 3 this value is fixed, and no pattern or 
	child-pattern can go beyond that value, if the parent is type 1 
	the end value is re-evaluated once another pattern has gone beyond
	it's value.
	Once a match is found, the text is tagged with the right style, and
	


***************************************************************/

/*********************************/
/* global vars for this module   */
/*********************************/
static Thighlight highlight;

/*********************************/
/* debugging                     */
/*********************************/
#ifdef DEBUG

static void print_meta_for_pattern(Tpattern *pat) {
	GList *tmplist = g_list_first(highlight.all_highlight_patterns);
	while (tmplist) {
		if (((Tmetapattern *)tmplist->data)->pat == pat) {
			g_print("pattern %p: filetype=%s, name=%s, num_childs=%d\n", pat, ((Tmetapattern *)tmplist->data)->filetype, ((Tmetapattern *)tmplist->data)->name, g_list_length(pat->childs));
			return;
		}
		tmplist = g_list_next(tmplist);
	}
}

static void print_meta_for_tag(GtkTextTag *tag) {
	GList *tmplist = g_list_first(highlight.all_highlight_patterns);
	while (tmplist) {
		if (((Tmetapattern *)tmplist->data)->pat->tag == tag) {
			g_print("tag %p: filetype=%s, name=%s, num_childs=%d\n", tag, ((Tmetapattern *)tmplist->data)->filetype, ((Tmetapattern *)tmplist->data)->name, g_list_length(((Tmetapattern *)tmplist->data)->pat->childs));
			return;
		}
		tmplist = g_list_next(tmplist);
	}
}

#endif /* DEBUG */
#ifdef HL_TIMING
typedef struct {
	struct tms tms1;
	struct tms tms2;
	glong total_ms;
	gint numtimes;
}Ttiming;
#define TIMING_TEXTBUF 0
#define TIMING_PCRE_EXEC 1
#define TIMING_TOTAL 2
#define TIMING_UTF8 3
#define TIMING_UTF8_INV 4
#define TIMING_TEXTBUF_ITER 5
#define TIMING_TEXTBUF_TAG 6
#define TIMING_LINE_HIGHLIGHTING 7
#define TIMING_NUM 8
static Ttiming timing[TIMING_NUM];
static void timing_init() {
	gint i;
	for (i=0;i<TIMING_NUM;i++){
		timing[i].total_ms = 0;
		timing[i].numtimes = 0;
	}
}
static void timing_start(gint id) {
	times(&timing[id].tms1);
}
static void timing_stop(gint id) {
	times(&timing[id].tms2);
	timing[id].numtimes++;
	timing[id].total_ms += (int) (double) ((timing[id].tms2.tms_utime - timing[id].tms1.tms_utime) * 1000 / sysconf(_SC_CLK_TCK));
}
#endif /* HL_TIMING */


/*********************************/
/* initializing the highlighting */
/*********************************/

static int pcre_compile_options(gint case_insens) {
	if (case_insens) {
		return PCRE_CASELESS|PCRE_MULTILINE;
	} else {
		return PCRE_MULTILINE;
	}
}

static void compile_pattern(gchar *filetype, gchar *name, gint case_insens
			, gchar *pat1, gchar *pat2, gint mode, gchar *parentmatch
			, gchar *fgcolor, gchar *bgcolor, gint weight, gint style) {
	/*
	 * test if the pattern is correct 
	 */
	if (!name || strlen(name) == 0) {
		g_print("error compiling nameless pattern: name is not set\n");
		return;
	}
	switch (mode) {
	case 1:
		if (!(pat1 && pat2 && strlen(pat1) && strlen(pat2))) {
			g_print("error compiling pattern '%s' for mode 1: some pattern(s) missing\n", name);
			return;
		}
		break;
	case 2:
		if (!(pat1 && strlen(pat1))) {
			g_print("error compiling pattern '%s' for mode 2: pattern missing\n", name);
			return;
		}
		break;
	case 3:
		if (!(pat1 && strlen(pat1) && atoi(pat1) > 0 && atoi(pat1) < NUM_SUBMATCHES)) {
			g_print("error compiling pattern '%s' for mode 3: sub-pattern number missing, too large or incorrect\n", name);
			return;
		}
		break;
	default:
		g_print("error compiling pattern '%s', mode %d unknown\n", name, mode);
		return;
		break;
	}

	{
		Tmetapattern *mpat;
		Tpattern *pat = g_new0(Tpattern, 1);
		pat->mode = mode;
		if (mode == 1 || mode ==2) {
			const char *err=NULL;
			int erroffset=0;
			DEBUG_MSG("compiling pat1 '%s'\n", pat1);
			pat->reg1.pcre = pcre_compile(pat1, pcre_compile_options(case_insens),&err,&erroffset,NULL);
			if (err) {
				g_print("error compiling pattern %s offset %d\n", err, erroffset);
			}
			DEBUG_MSG("result: pat->reg1.pcre=%p\n", pat->reg1.pcre);
			pat->reg1.pcre_e = pcre_study(pat->reg1.pcre,0,&err);
			if (err) {
				g_print("error studying pattern %s\n", err);
			}
		}
		if (mode == 1) {
			const char *err=NULL;
			int erroffset=0;
			DEBUG_MSG("compiling pat2 '%s'\n", pat2);
			pat->reg2.pcre = pcre_compile(pat2, pcre_compile_options(case_insens),&err,&erroffset,NULL);
			if (err) {
				g_print("error compiling pattern %s offset %d\n", err, erroffset);
			}
			DEBUG_MSG("result: pat->reg2.pcre=%p\n", pat->reg2.pcre);
			pat->reg2.pcre_e = pcre_study(pat->reg2.pcre,0,&err);
			if (err) {
				g_print("error studying pattern %s\n", err);
			}
		}
		if (mode == 3) {
			pat->numsub = atoi(pat1);
		}

		pat->tag = gtk_text_tag_new(NULL);
		if (strlen(fgcolor)) {
			g_object_set(pat->tag, "foreground", fgcolor, NULL);
		}
		if (strlen(bgcolor)) {
			g_object_set(pat->tag, "background", bgcolor, NULL);
		}
		if (weight > 0) {
			if (1 == weight) {
				g_object_set(pat->tag, "weight", PANGO_WEIGHT_NORMAL, NULL);
			} else {
				g_object_set(pat->tag, "weight", PANGO_WEIGHT_BOLD, NULL);
			}
		}
		if (style > 0) {
			if (1 == style) {
				g_object_set(pat->tag, "style", PANGO_STYLE_NORMAL, NULL);
			} else {
				g_object_set(pat->tag, "style", PANGO_STYLE_ITALIC, NULL);
			}
		}
		gtk_text_tag_table_add(highlight.tagtable, pat->tag);
		/* from the documentation:
			When adding a tag to a tag table, it will be assigned the highest priority in the table by 
			default; so normally the precedence of a set of tags is the order in which they were added 
			to the table.
			so the order of the styles in the list will be the order in the tagtable */
		
		mpat = g_new(Tmetapattern, 1);
		mpat->pat = pat;			
		mpat->name = g_strdup(name);
		if (strlen(parentmatch)) {
			mpat->parentmatch = g_strdup(parentmatch);
		} else {
			mpat->parentmatch = g_strdup("^top$");
		}
		mpat->filetype = g_strdup(filetype);
		highlight.all_highlight_patterns = g_list_append(highlight.all_highlight_patterns, mpat);
	}
}

static void add_patterns_to_parent(GList **list, gchar *filetype, gchar *name) {
	GList *tmplist = g_list_first(highlight.all_highlight_patterns);
	while (tmplist) {
		Tmetapattern *mpat = (Tmetapattern *)tmplist->data;
		if (strcmp(filetype, mpat->filetype)==0) {
			int ovector[9];
			if (pcre_exec(mpat->pcre,NULL, name,strlen(name),0,0,ovector,9) > 0) {
				*list = g_list_append(*list, mpat->pat);
			}
		}
		tmplist = g_list_next(tmplist);
	}
}

void hl_init() {
	GList *tmplist;
	/* init main_v->filetypelist, the first set is the defaultset */
	highlight.highlight_filetypes = NULL;
	highlight.all_highlight_patterns = NULL; 
	highlight.tagtable = gtk_text_tag_table_new();

	/* start by initializing the types, they should come from the configfile */
	tmplist = g_list_first(main_v->props.filetypes);
	while (tmplist) {
		gchar **strarr;
		Tfiletype *filetype;
		strarr = (gchar **) tmplist->data;
		if (count_array(strarr) == 4) {
			filetype = g_new(Tfiletype, 1);
			filetype->type = g_strdup(strarr[0]);
			filetype->extensions = g_strsplit(strarr[1], ":", 127);
			filetype->update_chars = g_strdup(strarr[2]);
			if (strlen(strarr[3])){
				GError *error=NULL;
				filetype->icon = gdk_pixbuf_new_from_file(strarr[3], &error);
				if (error != NULL) {
					/* Report error to user, and free error */
					g_print("ERROR while loading icon: %s\n", error->message);
					g_error_free(error);
					filetype->icon = NULL;
				}
			} else {
				filetype->icon = NULL;
			}
			filetype->highlightlist = NULL;
			main_v->filetypelist = g_list_append(main_v->filetypelist, filetype);
		}
#ifdef DEBUG
		else {
			DEBUG_MSG("hl_init, filetype needs 4 params in array\n");
		}
#endif
		tmplist = g_list_next(tmplist);
	}
	
	/* now compile the patterns in metapatterns, they should come from the configfile */
	tmplist = g_list_first(main_v->props.highlight_patterns);
	while (tmplist) {
		gchar **strarr;
		strarr = (gchar **) tmplist->data;
		if (count_array(strarr) == 11) {
			compile_pattern(strarr[0], strarr[1], atoi(strarr[2]), strarr[3], strarr[4], atoi(strarr[5]), strarr[6]
				, strarr[7], strarr[8], atoi(strarr[9]), atoi(strarr[10]));
		}
#ifdef DEBUG		
		else {
			g_print("hl_init, pattern needs 11 parameters in array\n");
		}
#endif
		tmplist = g_list_next(tmplist);
	}

	/* now start adding the patterns to the right filetype and the right pattern using the meta-info */
	
	/* first compile the parentmatch pattern */
	tmplist = g_list_first(highlight.all_highlight_patterns);
	while (tmplist) {
		const char *err=NULL;
		int erroffset=0;
		Tmetapattern *mpat = (Tmetapattern *)tmplist->data;
		mpat->pcre = pcre_compile(mpat->parentmatch, 0, &err, &erroffset,NULL);
		if (err) {
			g_print("error compiling parentmatch %s at %d\n", err, erroffset);
		}
		tmplist = g_list_next(tmplist);
	}
	/* now match the top-level patterns */
	tmplist = g_list_first(main_v->filetypelist);
	while (tmplist) {
		Tfiletype *filetype = (Tfiletype *)tmplist->data;
		add_patterns_to_parent(&filetype->highlightlist, filetype->type, "top");
		tmplist = g_list_next(tmplist);
	}
	/* now match the rest of the patterns */
	tmplist = g_list_first(highlight.all_highlight_patterns);
	while (tmplist) {
		Tmetapattern *mpat = (Tmetapattern *)tmplist->data;
		add_patterns_to_parent(&mpat->pat->childs, mpat->filetype, mpat->name);
		tmplist = g_list_next(tmplist);
	}
	/* free the parentmatch pattern now */
	tmplist = g_list_first(highlight.all_highlight_patterns);
	while (tmplist) {
		Tmetapattern *mpat = (Tmetapattern *)tmplist->data;
		pcre_free(mpat->pcre);
		tmplist = g_list_next(tmplist);
	}
	
	/* now link the filetypes with highlight patterns to a new list */
	tmplist = g_list_first(main_v->filetypelist);
	while (tmplist) {
		if (((Tfiletype *)tmplist->data)->highlightlist) {
			highlight.highlight_filetypes = g_list_append(highlight.highlight_filetypes, tmplist->data);
		}
		tmplist = g_list_next(tmplist);
	}
}

/**************************************/
/* end of initialisation code         */
/**************************************/

Tfiletype *hl_get_highlightset_by_type(gchar * type)
{
	GList *tmplist;
	tmplist = g_list_first(main_v->filetypelist);
	while (tmplist) {
#ifdef DEBUG
		if (!tmplist || !tmplist->data || !((Tfiletype *) tmplist->data)->type) {
			DEBUG_MSG("hl_get_highlightset_by_type is perhaps not yet finished??\n");
			exit(1);
		}
#endif							/* DEBUG */
		if (strcmp(((Tfiletype *) tmplist->data)->type, type) == 0) {
			return (Tfiletype *) tmplist->data;
		}
		tmplist = g_list_next(tmplist);
	}
	return NULL;
}

Tfiletype *hl_get_highlightset_by_filename(gchar * filename)
{
	GList *tmplist;
	/* find set for this filetype */
	if (filename) {
		tmplist = g_list_first(highlight.highlight_filetypes);
		while (tmplist) {
			if (filename_test_extensions(((Tfiletype *) tmplist->data)->extensions, filename)) {
				return (Tfiletype *) tmplist->data;
			}
			tmplist = g_list_next(tmplist);
		}
	}
	/* if none found return first set (is default set) */
	tmplist = g_list_first(highlight.highlight_filetypes);
	if (!tmplist) {
		DEBUG_MSG("hl_get_highlightset_by_filename, no highlightsets?\n");
		return NULL;
	} else {
		return (Tfiletype *) tmplist->data;
	}
}

/*****************************/
/* applying the highlighting */
/*****************************/

static void patmatch_rematch(gboolean is_parentmatch, Tpatmatch *patmatch, gint offset, gchar *buf, gint length, Tpatmatch *parentmatch) {
	if (is_parentmatch) {
#ifdef HL_TIMING
		timing_start(TIMING_PCRE_EXEC);
#endif
		patmatch->is_match = pcre_exec(patmatch->pat->reg2.pcre, patmatch->pat->reg2.pcre_e, buf, length, offset, 0, patmatch->ovector, NUM_SUBMATCHES);
#ifdef HL_TIMING
		timing_stop(TIMING_PCRE_EXEC);
#endif
	} else {
		if (patmatch->pat->mode == 3) {
			patmatch->ovector[0] = parentmatch->ovector[patmatch->pat->numsub*2];
			patmatch->ovector[1] = parentmatch->ovector[patmatch->pat->numsub*2+1];
			patmatch->is_match = TRUE;
		} else {
#ifdef HL_TIMING
			timing_start(TIMING_PCRE_EXEC);
#endif
			patmatch->is_match = pcre_exec(patmatch->pat->reg1.pcre, patmatch->pat->reg1.pcre_e, buf, length, offset, 0, patmatch->ovector, NUM_SUBMATCHES);
#ifdef HL_TIMING
			timing_stop(TIMING_PCRE_EXEC);
#endif
		}
	}
}

static void applystyle(Tdocument *doc, gchar *buf, guint buf_char_offset, gint so, gint eo, Tpattern *pat) {
	GtkTextIter itstart, itend;
	gint istart, iend;
	guint char_start, char_end, byte_char_diff_start;

	DEBUG_MSG("applystyle, coloring from so=%d to eo=%d\n", so, eo);

#ifdef HL_TIMING
	timing_start(TIMING_UTF8);
#endif
	char_start = utf8_byteoffset_to_charsoffset_cached(buf, so);
	char_end = utf8_byteoffset_to_charsoffset_cached(buf, eo);
	byte_char_diff_start = so-char_start;
#ifdef HL_TIMING
	timing_stop(TIMING_UTF8);
#endif
	istart = char_start+buf_char_offset;
	iend = char_end+buf_char_offset;
#ifdef HL_DEBUG
	DEBUG_MSG("applystyle, byte_char_diff=%d\n", byte_char_diff_start);
	DEBUG_MSG("applystyle, coloring from %d to %d\n", istart, iend);
#endif
#ifdef HL_TIMING
	timing_start(TIMING_TEXTBUF);
#endif

#ifdef HL_TIMING
	timing_start(TIMING_TEXTBUF_ITER);
#endif
	gtk_text_buffer_get_iter_at_offset(doc->buffer, &itstart, istart);
	gtk_text_buffer_get_iter_at_offset(doc->buffer, &itend, iend);
#ifdef HL_TIMING
	timing_stop(TIMING_TEXTBUF_ITER);
#endif
#ifdef HL_TIMING
	timing_start(TIMING_TEXTBUF_TAG);
#endif
	gtk_text_buffer_apply_tag(doc->buffer, pat->tag, &itstart, &itend);
#ifdef HL_TIMING
	timing_stop(TIMING_TEXTBUF_TAG);
#endif
#ifdef HL_TIMING
	timing_stop(TIMING_TEXTBUF);
#endif
}

/* applylevel(Tdocument * doc, gchar * buf, guint buf_char_offset, gint offset, gint length, Tpatmatch *parentmatch, GList *childs_list)
 * doc: the document
 * buf: the buffer with all characters to apply the highlighting to
 * buf_char_offset: the character offset of the first char of the buffer 
 * offset: the byte offset where we start in the buffer
 * length: the length we apply the highlighting on in bytes
 * parentmatch: if there is a parent with mode 1 we have to search for the end together with it's children
 * childs_list: a list of Tpattern that needs to be applied in this region
 */
static void applylevel(Tdocument * doc, gchar * buf, guint buf_char_offset, gint offset, gint length, Tpatmatch *parentmatch, GList *childs_list) {
	gint parent_mode1_start=offset;
	gint parent_mode1_offset=offset;
	if (parentmatch && parentmatch->pat->mode == 1) {
		/* before any patmatch_rematch, this has the end of the start pattern */
		parent_mode1_offset = parentmatch->ovector[1];
	}
	DEBUG_MSG("applylevel, started with offset=%d, length=%d\n", offset, length);
	if (!childs_list) {
		/* if the parent is mode 1 we still need to find the end for the parent */
		if (parentmatch && parentmatch->pat->mode == 1) {
			patmatch_rematch(TRUE, parentmatch, offset > parent_mode1_offset ? offset : parent_mode1_offset, buf, length, parentmatch);
			if (parentmatch->is_match<1) {
				DEBUG_MSG("no childs list, mode 1 parent has no match, matching to length %d\n", length);
				parentmatch->ovector[1] = length;
				parentmatch->is_match = 1;
			}
			applystyle(doc, buf,buf_char_offset, offset, parentmatch->ovector[1], parentmatch->pat);
			DEBUG_MSG("no childs list, mode 1 parent is applied, returning\n");
		} else {
			DEBUG_MSG("no childs list, no mode 1 parent, returning\n");
		}
		return;
	} else {
		gint numpats, i;
		Tpatmatch *patmatch;
		/* create the patmatch structures for this level */
		GList *tmplist;
		gint lowest_pm = -2; /* -2 means no lowest match, -1 means the parents ending is the lowest match, 0... means some pattern is the lowest match */
			i = 0;
			numpats = g_list_length(childs_list);
			patmatch = g_new(Tpatmatch, numpats + 1);
			tmplist = g_list_first(childs_list);
			while (tmplist) {
				patmatch[i].pat = (Tpattern *) tmplist->data;
				/* match all patmatches for the first time and directly search for the lowest match */
				patmatch_rematch(FALSE, &patmatch[i], offset, buf, length, parentmatch);
				if ((patmatch[i].is_match > 0) && (lowest_pm == -2 || (patmatch[lowest_pm].ovector[0] > patmatch[i].ovector[0]))) {
					lowest_pm = i;
				}
#ifdef DEBUG
				DEBUG_MSG("lowest_pm=%d, patmatch[%d].is_match=%d, start=%d, pattern=",lowest_pm,i,patmatch[i].is_match, patmatch[i].ovector[0]);
				print_meta_for_pattern(patmatch[i].pat);
#endif
				tmplist = g_list_next(tmplist);
				i++;
			}
			if (parentmatch && parentmatch->pat->mode == 1) {
				/* the end of the parent pattern needs matching too */
				DEBUG_MSG("matching the parent with offset %d\n", offset > parent_mode1_start ? offset : parent_mode1_start);
				patmatch_rematch(TRUE, parentmatch, offset > parent_mode1_offset ? offset : parent_mode1_offset, buf, length, parentmatch);
				if (!parentmatch->is_match) {
					DEBUG_MSG("mode 1 parent has no match, matching to length %d\n", length);
					parentmatch->ovector[1] = length;
					parentmatch->ovector[0] = length;
					parentmatch->is_match = 1;
				}
				if (parentmatch->is_match && (lowest_pm == -2 || (patmatch[lowest_pm].ovector[0] > parentmatch->ovector[0]))) {
					lowest_pm = -1;
				}
				DEBUG_MSG("lowest_pm=%d, parentmatch->is_match=%d, start=%d\n",lowest_pm,parentmatch->is_match, parentmatch->ovector[0]);
			}
			
			/* apply the lowest match */
			while (lowest_pm > -2) {
				if (lowest_pm == -1) {
#ifdef DEBUG
					DEBUG_MSG("parent is the match!! ");
					print_meta_for_pattern(parentmatch->pat);
					DEBUG_MSG("offset=%d, lowest_pm=%d with ovector[0]=%d, ovector[1]=%d\n", offset, lowest_pm, parentmatch->ovector[0],parentmatch->ovector[1]);
#endif
					/* apply the parent, and return from this level */
					applystyle(doc, buf,buf_char_offset, parent_mode1_start, parentmatch->ovector[1], parentmatch->pat);
					lowest_pm = -2; /* makes us return */
				} else {
#ifdef DEBUG
					print_meta_for_pattern(patmatch[lowest_pm].pat);
					DEBUG_MSG("a childs is the match, offset=%d, lowest_pm=%d with start=%d and ovector[1]=%d\n", offset, lowest_pm, patmatch[lowest_pm].ovector[0],patmatch[lowest_pm].ovector[1]);
#endif
					switch (patmatch[lowest_pm].pat->mode) {
					case 1:
						/* if mode==1 the style is applied within the applylevel for the children because the end is not yet 
						known, the end is set in ovector[1] after applylevel for the children is finished */
						applylevel(doc, buf,buf_char_offset, patmatch[lowest_pm].ovector[0], length, &patmatch[lowest_pm], patmatch[lowest_pm].pat->childs);
						offset = patmatch[lowest_pm].ovector[1];
					break;
					case 2:
						applystyle(doc, buf,buf_char_offset, patmatch[lowest_pm].ovector[0], patmatch[lowest_pm].ovector[1], patmatch[lowest_pm].pat);
						applylevel(doc, buf,buf_char_offset, patmatch[lowest_pm].ovector[0], patmatch[lowest_pm].ovector[1], &patmatch[lowest_pm], patmatch[lowest_pm].pat->childs);
						offset = patmatch[lowest_pm].ovector[1];
					break;
					case 3:
						applystyle(doc, buf,buf_char_offset, patmatch[lowest_pm].ovector[0], patmatch[lowest_pm].ovector[1], patmatch[lowest_pm].pat);
						applylevel(doc, buf,buf_char_offset, patmatch[lowest_pm].ovector[0], patmatch[lowest_pm].ovector[1], &patmatch[lowest_pm], patmatch[lowest_pm].pat->childs);
						offset = patmatch[lowest_pm].ovector[1];
					break;
					default:
						/* unknown mode, cannot pass the pattern-compile-stage??? */
#ifdef DEBUG
						g_print("applylevel, unknown mode, cannot pass the pattern-compile-stage???\n");
						exit(2);
#endif
					break;
					}
					
					/* rematch the patterns that have a startpoint < offset */
					lowest_pm = -2;
					for (i = 0; i < numpats; i++) {
						if (patmatch[i].ovector[0] < offset) {
							if (patmatch[i].pat->mode == 3) {
								patmatch[i].is_match = FALSE; /* mode 3 types can only match as first match */
							} else {
								patmatch_rematch(FALSE, &patmatch[i], offset, buf, length, parentmatch);
								DEBUG_MSG("rematch: cur lowest_pm=%d, patmatch[%d].is_match=%d\n",lowest_pm,i,patmatch[i].is_match);
							}
						}
						if ((patmatch[i].is_match>0) && (lowest_pm == -2 || (patmatch[lowest_pm].ovector[0] > patmatch[i].ovector[0]))) {
							lowest_pm = i;
						}
					}
					if (parentmatch && parentmatch->pat->mode == 1) {
						/* the end of the parent pattern needs matching too */
						if (parentmatch->ovector[0] < offset) {
							patmatch_rematch(TRUE, parentmatch, offset, buf, length, parentmatch);
						}
						if (!parentmatch->is_match) {
							DEBUG_MSG("mode 1 parent has no match, matching to length %d\n", length);
							parentmatch->ovector[1] = length;
							parentmatch->ovector[0] = length;
							parentmatch->is_match = 1;
						}
						if (lowest_pm == -2 || (patmatch[lowest_pm].ovector[0] > parentmatch->ovector[0])) {
							lowest_pm = -1;
						}
					}
#ifdef DEBUG
					if (lowest_pm == -1) {
						DEBUG_MSG("lowest_match %d has start %d\n", lowest_pm, parentmatch->ovector[0]);
					} else {
						DEBUG_MSG("lowest_match %d has start %d\n", lowest_pm, patmatch[lowest_pm].ovector[0]);
					}
#endif
				}
		}
		g_free(patmatch);
	}
}

void doc_remove_highlighting(Tdocument * doc)
{
	GtkTextIter itstart, itend;
	gtk_text_buffer_get_bounds(doc->buffer, &itstart, &itend);
	gtk_text_buffer_remove_all_tags(doc->buffer, &itstart, &itend);
}

void hl_reset_highlighting_type(Tdocument * doc, gchar * newfilename)
{
	Tfiletype *filetype = hl_get_highlightset_by_filename(newfilename);
	if (filetype && filetype->highlightlist) {
		if (filetype != doc->hl) {
			doc_remove_highlighting(doc);
			doc->hl = filetype;
			doc->need_highlighting = TRUE;
			menu_current_document_type_set_active_wo_activate(filetype);
		}
	}
}

gboolean hl_set_highlighting_type(Tdocument * doc, Tfiletype *filetype) {
	if (filetype && filetype->highlightlist) {
		if (filetype != doc->hl) {
			doc_remove_highlighting(doc);
			doc->hl = filetype;
			doc->need_highlighting = TRUE;
			menu_current_document_type_set_active_wo_activate(filetype);
		}
		return TRUE;
	} else {
		return FALSE;
	}
}

void doc_highlight_full(Tdocument * doc) {
	if (!doc->hl) {
		return;
	} else {
		gchar *buf;
		guint charcount = gtk_text_buffer_get_char_count(doc->buffer);
		doc_remove_highlighting(doc);
#ifdef HL_TIMING
		timing_init();
#endif

#ifdef HL_TIMING
		timing_start(TIMING_TOTAL);
#endif
		utf8_offset_cache_reset();
		buf = doc_get_chars(doc, 0, charcount);
#ifdef DEBUG
		g_assert(doc);
		g_assert(doc->hl);
		g_assert(buf);
#endif
		applylevel(doc,buf,0,0,strlen(buf),NULL,doc->hl->highlightlist);
		g_free(buf);
#ifdef HL_TIMING
		timing_stop(TIMING_TOTAL);
		g_print("doc_highlight_full done, %ld ms total, %ld ms tagging (%dX), %ld ms matching (%dX)\n",timing[TIMING_TOTAL].total_ms, timing[TIMING_TEXTBUF].total_ms, timing[TIMING_TEXTBUF].numtimes, timing[TIMING_PCRE_EXEC].total_ms, timing[TIMING_PCRE_EXEC].numtimes);
		g_print("%ld ms utf8-shit (%dX), %ld ms utf8-invalidate (%dX)\n", timing[TIMING_UTF8].total_ms, timing[TIMING_UTF8].numtimes, timing[TIMING_UTF8_INV].total_ms, timing[TIMING_UTF8_INV].numtimes);
		g_print("%ld ms setting iters, %ld ms setting tags\n", timing[TIMING_TEXTBUF_ITER].total_ms, timing[TIMING_TEXTBUF_TAG].total_ms);
#endif
		doc->need_highlighting = FALSE;
	}
}

static void remove_tag_by_list_in_region(Tdocument * doc, GList * patlist, GtkTextIter * itstart, GtkTextIter * itend)
{
	GList *tmplist = g_list_first(patlist);
	DEBUG_MSG("remove_tag_by_list_in_region, started on list %p\n", patlist);
	/* remove all tags that are children of patlist */
	while (tmplist) {
		DEBUG_MSG("remove_tag_by_list_in_region, removing tags for pattern %p\n", tmplist->data);
		gtk_text_buffer_remove_tag(doc->buffer, ((Tpattern *) tmplist->data)->tag, itstart, itend);
		if (((Tpattern *) tmplist->data)->childs) {
			remove_tag_by_list_in_region(doc, ((Tpattern *) tmplist->data)->childs, itstart, itend);
		}
		tmplist = g_list_next(tmplist);
	}
}

static Tpattern *find_pattern_by_tag(GList * parentlist, GtkTextTag * tag)
{
	GList *tmplist;

	tmplist = g_list_first(parentlist);
	while (tmplist) {
		if (((Tpattern *) tmplist->data)->tag == tag) {
			return (Tpattern *) tmplist->data;
		}
		tmplist = g_list_next(tmplist);
	}
	return NULL;
}

void doc_highlight_line(Tdocument * doc)
{
	gint so, eo;
	Tpattern *pat = NULL;
	GList *patternlist = doc->hl->highlightlist;
#ifdef HL_TIMING
	timing_init();
	timing_start(TIMING_LINE_HIGHLIGHTING);
#endif
	{
		GtkTextIter itstart, itend, itsearch;
		GSList *taglist = NULL, *slist;
		GtkTextMark *mark = gtk_text_buffer_get_insert(doc->buffer);
		gtk_text_buffer_get_iter_at_mark(doc->buffer, &itstart, mark);
		/* move to the beginning of the line */
		gtk_text_iter_set_line_offset(&itstart, 0);
		if (main_v->props.highlight_num_lines_count) {
			gtk_text_iter_backward_lines(&itstart, main_v->props.highlight_num_lines_count);
		}
#ifdef DEBUG
		DEBUG_MSG("doc_highlight_line, itstart is at %d\n", gtk_text_iter_get_offset(&itstart));
#endif
		gtk_text_buffer_get_iter_at_mark(doc->buffer, &itend, mark);
/*		gtk_text_iter_forward_to_line_end(&itend);
		gtk_text_iter_set_line_offset(&itend, 0);*/
		if (main_v->props.highlight_num_lines_count) {
			gtk_text_iter_forward_lines(&itend, main_v->props.highlight_num_lines_count);
		}
		if (gtk_text_iter_forward_to_line_end(&itend)) {
			gtk_text_iter_forward_char(&itend);
		}
#ifdef DEBUG
		DEBUG_MSG("doc_highlight_line, itend is at %d\n", gtk_text_iter_get_offset(&itend));
#endif
		if (gtk_text_iter_get_offset(&itstart) == gtk_text_iter_get_offset(&itend)) {
			DEBUG_MSG("doc_highlight_line start end end are the same, cannot highlight!!\n");
			return;
		}

		/* get all the tags that itstart is in */
		taglist = gtk_text_iter_get_tags(&itstart);
		/* find for every tag if it ends _after_ itend or not */
		itsearch = itstart;
		slist = taglist;
		while (slist && slist->data) {
			gboolean tag_found;
#ifdef DEBUG
			{
				Tpattern *testpat;
				gchar *test;
				testpat = find_pattern_by_tag(patternlist, GTK_TEXT_TAG(slist->data));
				if (testpat) {
					DEBUG_MSG("doc_highlight_line, tag %p goes with ", testpat->tag);
					print_meta_for_pattern(testpat);
				} else {
					DEBUG_MSG("doc_highlight_line, tag doesn't have parent?? ");
					print_meta_for_tag(GTK_TEXT_TAG(slist->data));
					exit(9);
				}
				test = gtk_text_buffer_get_text(doc->buffer, &itstart, &itend,FALSE);
				DEBUG_MSG("doc_highlight_line, current string='%s'\n", test);
				g_free(test);
			}
#endif
			/* if the tags ends at itstart there is no need to search forward to the end */
			if (!gtk_text_iter_ends_tag(&itstart, GTK_TEXT_TAG(slist->data))) {
				DEBUG_MSG("doc_highlight_line, looking for tag %p from so=%d to eo=%d\n", slist->data,
						  gtk_text_iter_get_offset(&itstart), gtk_text_iter_get_offset(&itend));
				tag_found = gtk_text_iter_forward_to_tag_toggle(&itsearch, GTK_TEXT_TAG(slist->data));
				DEBUG_MSG("doc_highlight_line, tag %p ends at itsearch=%d\n", slist->data,gtk_text_iter_get_offset(&itsearch));
				if (gtk_text_iter_compare(&itsearch, &itend) > 0) {
					/* both the start and endpoint are within this 
					   tag --> pattern matching can start with this
					   subpattern */
						pat = find_pattern_by_tag(patternlist, GTK_TEXT_TAG(slist->data));
					DEBUG_MSG("found pattern %p with tag %p and childs %p\n", pat, pat->tag,pat->childs);
					if (pat && (pat->mode == 1)) {
						gchar *string;
						int ovector[NUM_SUBMATCHES];
						/* but first we do a quick test if the parent-pattern is indeed still valid */
						string = gtk_text_buffer_get_text(doc->buffer, &itstart, &itend, FALSE);
						if (0 < pcre_exec(pat->reg2.pcre, pat->reg2.pcre_e, string, strlen(string), 0, 0, ovector, NUM_SUBMATCHES)) {
							/* the current line does not have the start of the tag or the end of the tag, but now 
							   it does have a match on the end pattern --> so the pattern should be invalidated */
							pat = NULL;
							DEBUG_MSG("doc_highlight_line, a match of the endpattern is found on this line '%s', the pattern is invalidated\n",string);
							itend = itsearch;
							gtk_text_iter_backward_to_tag_toggle(&itstart, slist->data);
						}
						g_free(string);
					}
					if (pat) {
						/* what happens if patternlist = NULL ?
						   that means we are inside a match without any subpatterns, but 
						   perhaps the subpattern should be invalidated... hmm..
						 */
						patternlist = pat->childs;
#ifdef DEBUG
						DEBUG_MSG("doc_highlight_line, going to use patternlist %p from pattern ", patternlist);
						print_meta_for_pattern(pat);
#endif
					}
				} else {
					/* this tag stops somewhere in the middle of the line, move 
					   itstart to the beginning of this tag, 
						there is also no need anymore to look further in slist, we have to start with this patternlist */
					if (gtk_text_iter_begins_tag(&itstart, GTK_TEXT_TAG(slist->data))) {
						DEBUG_MSG("doc_highlight_line, itstart at %d is already at the beginning of tag %p\n",gtk_text_iter_get_offset(&itstart), slist->data);
					} else {
						DEBUG_MSG("doc_highlight_line, move itstart from %d to beginning of tag %p\n",
								  gtk_text_iter_get_offset(&itstart), slist->data);
						gtk_text_iter_backward_to_tag_toggle(&itstart, GTK_TEXT_TAG(slist->data));
						DEBUG_MSG("doc_highlight_line, itstart is set back to %d\n", gtk_text_iter_get_offset(&itstart));
					}
					slist = g_slist_last(slist);
				}
			}
			itsearch = itstart;
			slist = g_slist_next(slist);
		}
		g_slist_free(taglist);

		/* get all the tags that itend is in */
		taglist = gtk_text_iter_get_tags(&itend);
		/* find for every tag if it startss _before_ itstart or not */
		itsearch = itend;
		slist = taglist;
		while (slist && slist->data) {
			gboolean tag_found;
					/* if the tags starts at itend there is no need to search backward to the start */
			if (!gtk_text_iter_begins_tag(&itend, GTK_TEXT_TAG(slist->data))) {
				DEBUG_MSG("doc_highlight_line, (2) looking for tag %p from so=%d to eo=%d, itsearch=%d\n", slist->data,
						  gtk_text_iter_get_offset(&itstart), gtk_text_iter_get_offset(&itend), gtk_text_iter_get_offset(&itsearch));
				tag_found = gtk_text_iter_backward_to_tag_toggle(&itsearch, GTK_TEXT_TAG(slist->data));
				if (!tag_found) {
					DEBUG_MSG("doc_highlight_line, very weird situation, the tag is ended but it doesn't start ??, itsearch now is at %d\n", gtk_text_iter_get_offset(&itsearch));
					/* itsearch is now at offset 0, so we do a forward search to find the first start */
					tag_found = gtk_text_iter_forward_to_tag_toggle(&itsearch, GTK_TEXT_TAG(slist->data));
					DEBUG_MSG("a forward search to the tag finds %d\n", gtk_text_iter_get_offset(&itsearch));
					if (!tag_found) {
						g_print("doc_highlight_line, a tag that is started is nowhere to be found ??? BUG!\n");
						exit(123);
					}
				} 
				DEBUG_MSG("doc_highlight_line, tag %p starts at itsearch=%d\n", slist->data,gtk_text_iter_get_offset(&itsearch));
				if (gtk_text_iter_compare(&itsearch, &itstart) <= 0) {
					/* both the start and endpoint are within this 
					   tag --> pattern matching can start with this
					   subpattern,	since we did run the same algorithm for itstart we can skip a bit now */
				} else {
					/* this tag starts somewhere in the middle of the line, move 
					   itend to the end of this tag */
					if (gtk_text_iter_ends_tag(&itend, GTK_TEXT_TAG(slist->data))) {
						DEBUG_MSG("doc_highlight_line, itend at %d is already at the end of tag %p\n",
								  gtk_text_iter_get_offset(&itend), slist->data);
					} else {
						DEBUG_MSG("doc_highlight_line, move itend from %d to end of tag %p\n",gtk_text_iter_get_offset(&itend), slist->data);
						gtk_text_iter_forward_to_tag_toggle(&itend, GTK_TEXT_TAG(slist->data));
						DEBUG_MSG("doc_highlight_line, itend is set forward to %d\n", gtk_text_iter_get_offset(&itend));
					}
				}
			}
			itsearch = itend;
			slist = g_slist_next(slist);
		}
		
		
		/* this function removes some specific tags from the region */
		remove_tag_by_list_in_region(doc, patternlist, &itstart, &itend);
		so = gtk_text_iter_get_offset(&itstart);
		eo = gtk_text_iter_get_offset(&itend);
	}
#ifdef HL_TIMING
		timing_stop(TIMING_LINE_HIGHLIGHTING);
#endif
	if (patternlist) {
		gchar *buf;
		DEBUG_MSG("doc_highlight_line from so=%d to eo=%d\n", so, eo);
#ifdef HL_TIMING
		timing_start(TIMING_TOTAL);
#endif
		buf = doc_get_chars(doc, so, eo);
		applylevel(doc,buf,so,0,strlen(buf),NULL,patternlist);
		g_free(buf);
#ifdef HL_TIMING
		timing_stop(TIMING_TOTAL);
		g_print("doc_highlight_line done, %ld ms total, %ld ms line_highlighting\n",timing[TIMING_TOTAL].total_ms, timing[TIMING_LINE_HIGHLIGHTING].total_ms);
#endif
	} else {
		DEBUG_MSG("doc_highlight_line, patternlist=NULL so we don't need highlighting\n");
	}
	doc->need_highlighting = FALSE;
}

void hl_reset_to_default()
{
	gchar **arr;
	/* the further down, the higher the priority of the tag */
	arr = array_from_arglist("c", "string", "0", "\"", "\"", "1", "", "#009900", "", "0", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("c", "string-escape", "0", "\\\\.", "", "2", "^string$", "#009900", "", "0", "0",NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("c", "preprocessor", "0", "#(include|define|if|ifdef|else|endif).*$", "", "2", "","#000099", "", "0", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("c", "comment", "0", "/\\*", "\\*/", "1", "^(top|preprocessor)$", "#AAAAAA", "", "1", "2", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("c", "keywords", "0", "\\b(return|goto|if|else|case|default|switch|break|continue|while|do|for|sizeof)\\b","", "2", "", "#000000", "", "2", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr =	array_from_arglist("c", "storage-keywords", "0", "\\b(const|extern|auto|register|static|unsigned|signed|volatile|char|double|float|int|long|short|void|typedef|struct|union|enum|FILE|gint|gchar|GList|GtkWidget|gpointer|guint|gboolean)\\b","", "2", "", "#990000", "", "2", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("c", "braces", "0", "[{()}]", "",  "2", "", "#000000", "", "2", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("c", "character", "0", "'", "'",  "1", "", "#009900", "", "0", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);

	arr = array_from_arglist("php", "html", "1", "<((/)?[a-z0-9]+)", ">",  "1", "", "#000077", "", "0", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("php", "html-tag", "1", "1", "", "3", "^html$", "#550044", "", "2", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr =	array_from_arglist("php", "html-tag-table", "1", "^(/)?(table|td|tr|tbody)$", "",  "2", "^html-tag$", "#5005AA", "", "0", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr =	array_from_arglist("php", "html-tag-special", "1", "^(/)?(img|a)$", "",  "2", "^html-tag$", "#BB0540", "", "0","0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("php", "html-attrib", "1", "([a-z]*=)(\"[^\"]*\")", "",  "2", "^html$", "", "", "0", "0",NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("php", "html-attrib-sub2", "1", "2", "",  "3", "^html-attrib$", "#009900", "", "0", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("php", "specialchar", "1", "&[^;]*;", "", "2", "", "#999999", "", "2", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("php", "comment", "0", "<!--", "-->",  "1", "", "#AAAAAA", "", "1", "2", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("php", "php", "1", "<\\?php", "\\?>",  "1", "^(top|html|html-attrib-sub2)$", "#0000FF", "", "0", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("php", "php-keywords", "0","\\b(return|goto|if|else|case|default|switch|break|continue|while|do|for|global|var|class|function|new)\\b","",  "2", "^php$", "#000000", "", "2", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("php", "php-braces", "0", "[{()}]", "", "2", "^php$", "#000000", "", "2", "0",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("php", "php-string-double", "0", "\"", "\"",  "1", "^php$", "#009900", "", "1", "1", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("php", "php-string-double-escape", "0", "\\\\.", "", "2", "^php-string-double$", "#009900", "", "1", "1",NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("php", "php-var", "1", "\\$[a-z_][][a-z0-9>_$-]*", "",  "2", "^php$", "#CC0000", "", "2", "0",NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("php", "php-var-specialchars", "0", "(\\[|\\]|->)", "",  "2", "^php-var$", "#0000CC", "", "0","0",NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("php", "php-string-single", "0", "'", "'",  "1", "^php$", "#009900", "", "1", "1", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("php", "php-comment-C", "0", "/\\*", "\\*/",  "1", "^php$", "#7777AA", "", "1", "2", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr =array_from_arglist("php", "php-comment-C++", "0", "//.*$", "",  "2", "^php$", "#7777AA", "", "1", "2", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);

	arr = array_from_arglist("html", "html", "1", "<((/)?[a-z0-9]+)", ">", "1", "", "#0000EE", "", "0", "0",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("html", "html-tag", "1", "1", "",  "3", "^html$", "#000077", "", "2", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("html", "html-attrib", "1", "([a-z]*=)(\"[^\"]*\")", "", "2", "^html$", "", "", "0", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("html", "html-attrib-sub2", "1", "2", "", "3", "^html-attrib$", "#009900", "", "0", "0",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("html", "specialchar", "1", "&[^;]*;", "", "2", "", "#999999", "", "2", "0",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("html", "comment", "0", "<!--", "-->", "1", "", "#AAAAAA", "", "1", "2",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("html", "doctype", "1", "<![a-z0-9]+", "[^?-]>",  "1", "", "#bb8800", "", "0", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);

	arr = array_from_arglist("xml", "tag", "1", "<((/)?[a-z0-9]+)", "[^?-]>",  "1", "", "#0000EE", "", "0", "0",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("xml", "tag-name", "1", "1", "", "3", "^tag$", "#000077", "", "2", "0",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("xml", "tag-attrib", "1", "([a-z]*=)(\"[^\"]*\")", "",  "2", "^tag$", "", "", "0", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("xml", "tag-attrib-sub2", "1", "2", "",  "3", "^tag-attrib$", "#009900", "", "0", "0",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("xml", "specialchar", "1", "&[^;]*;", "",  "2", "", "#999999", "", "2", "0",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("xml", "comment", "0", "<!--", "-->",  "1", "", "#AAAAAA", "", "1", "2",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("xml", "doctype", "1", "<![a-z0-9]+", "[^?-]>",  "1", "", "#bb8800", "", "0", "0",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);

	arr = array_from_arglist("java", "comment-C-style", "0", "/\\*", "\\*/", "1", "", "#AAAAAA", "", "1", "2",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("java", "comment-C++-style", "0", "//.*$", "", "2", "", "#AAAAAA", "", "1", "2",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("java", "string", "0", "\"", "\"", "1", "", "#009900", "", "0", "0",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("java", "string-escape", "0", "\\\\.", "", "2", "^string$", "#009900", "", "0", "0",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr =	array_from_arglist("java", "include", "0", "^(import|package).*$", "",  "2", "", "#000099", "", "0", "0",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr =	array_from_arglist("java", "keywords", "0", "\\b(new|super|return|goto|if|else|case|default|switch|break|continue|while|do|for|catch|throw|finally|try|class)\\b","",  "2", "", "#000000", "", "2", "0",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("java", "special_values", "0", "\\b(false|null|true)\\b", "", "2", "", "#5caeee", "", "0", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("java", "modifiers", "0", "\\b(abstract|final|native|private|protected|public|static|transient|synchronized|volatile|extends|implements)\\b","", "2", "", "#990000", "", "2", "0",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("java", "this", "0", "\\bthis\\.", "",  "2", "", "#000099", "", "2", "0", NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("java", "primitive-types", "0", "\\b(void|double|boolean|int)\\b", "",  "2", "", "#880088", "", "2","0",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("java", "braces", "0", "[{()}]", "", "", "2", "", "#000000", "", "2", "0",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	arr = array_from_arglist("java", "character", "0", "'", "'", "1", "", "#009900", "", "0", "0",  NULL);
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns, arr);
	
	/* python support */
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns,array_from_arglist("python", "keywords", "0", "\\b(and|assert|break|continue|def|del|elif|else|except|exec|finally|for|if|in|is|not|or|pass|print|raise|return|try|while)\\b","",  "2", "", "#000000", "", "2", "0",  NULL));
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns,array_from_arglist("python", "string-double", "0", "\"", "\"", "1", "", "#009900", "", "0", "0",  NULL));
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns,array_from_arglist("python", "string-double-escape", "0", "\\\\.", "", "2", "^string-double$", "#009900", "", "0", "0",  NULL));
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns,array_from_arglist("python", "string-single", "0", "'", "'", "1", "", "#009900", "", "0", "0",  NULL));
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns,array_from_arglist("python", "string-single-escape", "0", "\\\\.", "", "2", "^string-single$", "#009900", "", "0", "0",  NULL));
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns,array_from_arglist("python", "special_values", "0", "\\b(None)\\b", "", "2", "", "#5caeee", "", "0", "0", NULL));
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns,array_from_arglist("python", "include", "0", "^(import|from).*$", "",  "2", "", "#000099", "", "0", "0",  NULL));
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns,array_from_arglist("python", "comment", "0", "#.*$", "", "2", "", "#AAAAAA", "", "1", "2",  NULL));
	main_v->props.highlight_patterns = g_list_append(main_v->props.highlight_patterns,array_from_arglist("python", "braces", "0", "[{()}\\[\\]]", "", "2", "", "#000000", "", "2", "0",  NULL));
}

GtkTextTagTable *highlight_return_tagtable() {
	return highlight.tagtable;
}
