/* Bluefish HTML Editor
 * highlight.c - the syntax highlighting
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
 */
#include <gtk/gtk.h>
#include <sys/types.h>
#include <regex.h> /* regexec() */
#include <string.h> /* strerror() */
#include <stdlib.h> /* atoi() */

#include "bluefish.h"

#include "highlight.h"

#define NUM_SUBMATCHES 20

typedef struct {
	gint so;
	gint eo;
	regmatch_t par_pmatch[NUM_SUBMATCHES];
	gboolean is_match;
} Tpatmatch;


/*********************************/
/* initializing the highlighting */
/*********************************/


static void pattern_free(Tpattern * pat)
{
	g_free(pat->name);
	g_free(pat->spat1);
	regfree(&pat->rpat1);
	if (pat->spat2) {
		g_free(pat->spat2);
		regfree(&pat->rpat2);
	}
	if (pat->spat3) {
		g_free(pat->spat3);
		regfree(&pat->rpat3);
	}
	g_free(pat);
}

static gboolean add_pattern_to_name(GList *patternlist, gchar *parentname,Tpattern * pat)
{
	Tpattern *parpat;
	gboolean resval = FALSE;
	GList *tmplist;

	tmplist = g_list_first(patternlist);
	while (tmplist) {
		if (((Tpattern *) tmplist->data)->childs) {
			gboolean tmpresval;

			tmpresval =
				add_pattern_to_name(((Tpattern *) tmplist->data)->childs,
									parentname, pat);
			if (!resval && tmpresval)
				resval = TRUE;
		}
		if (strcmp
			((gchar *) ((Tpattern *) tmplist->data)->name,
			 parentname) == 0) {
			parpat = (Tpattern *) tmplist->data;
			parpat->childs = g_list_append(parpat->childs, pat);
			resval = TRUE;
			if (pat->mode == 3) {
				parpat->need_subpatterns = TRUE;
			}
		}
		tmplist = g_list_next(tmplist);
	}
	return resval;
}

static void hl_compile_pattern(Thighlightset *hlset, gchar * name,
						gint case_insens, gchar * pat1, gchar * pat2,
						gchar * pat3, gint mode, gchar * parent,
						gchar * fgcolor, gchar * bgcolor, gint weight,
						gint style)
{
	/*
	 * test if the pattern is correct 
	 */
	g_print("compile_patterns, testing name=%s\n", name);
	if (!name || strlen(name) == 0) {
		g_print("error compiling nameless pattern: name is not set\n");
	}
	switch (mode) {
	case 0:
		if (!(pat1 && pat2 && pat3 && strlen(pat1) && strlen(pat2)
			  && strlen(pat3))) {
			g_print
				("error compiling pattern %s for mode 0: some pattern(s) missing\n",
				 name);
			return;
		}
		break;
	case 1:
		if (!(pat1 && pat2 && strlen(pat1) && strlen(pat2))) {
			g_print
				("error compiling pattern %s for mode 1: some pattern(s) missing\n",
				 name);
			return;
		}
		break;
	case 2:
		if (!(pat1 && strlen(pat1))) {
			g_print
				("error compiling pattern %s for mode 2: pattern missing\n",
				 name);
			return;
		}
		break;
	case 3:
		if (!
			(pat1 && strlen(pat1) && atoi(pat1) > 0
			 && atoi(pat1) < NUM_SUBMATCHES)) {
			g_print
				("error compiling pattern %s for mode 3: sub-pattern number missing or incorrect\n",
				 name);
			return;
		}
		break;
	default:
		g_print("error compiling pattern %s: mode %d unknown\n", name,
				mode);
		return;
		break;
	}

	{
		Tpattern *pat;
		gint res;

		g_print("compile_patterns, compiling name=%s\n", name);

		pat = g_malloc0(sizeof(Tpattern));
		switch (mode) {
		case 0:
			pat->spat3 = g_strdup(pat3);
		case 1:
			pat->spat2 = g_strdup(pat2);
		case 2:
		case 3:
			pat->spat1 = g_strdup(pat1);
		default:
			pat->name = g_strdup(name);
			pat->mode = mode;
			g_print("compile_patterns, done copying\n");
			break;
		}
		switch (mode) {
		case 0:
			if (!case_insens) {
				res = regcomp(&pat->rpat3, pat3, REG_EXTENDED);
			} else {
				res = regcomp(&pat->rpat3, pat3, REG_EXTENDED | REG_ICASE);
			}
			if (0 != res) {
				g_print
					("error compiling 3rd pattern %s: %s because %s\n",
					 name, pat3, strerror(res));
				g_free(pat->spat1);
				g_free(pat->spat2);
				g_free(pat->spat3);
				g_free(pat->name);
				g_free(pat);
				return;
			}
		case 1:
			if (!case_insens) {
				res = regcomp(&pat->rpat2, pat2, REG_EXTENDED);
			} else {
				res = regcomp(&pat->rpat2, pat2, REG_EXTENDED | REG_ICASE);
			}
			if (0 != res) {
				g_print
					("error compiling 2nd pattern %s: %s because %s\n",
					 name, pat2, strerror(res));
				if (mode == 0) {
					g_free(pat->spat3);
					regfree(&pat->rpat3);
				}
				g_free(pat->spat1);
				g_free(pat->spat2);
				g_free(pat->name);
				g_free(pat);
				return;
			}
		case 2:
			if (!case_insens) {
				res = regcomp(&pat->rpat1, pat1, REG_EXTENDED);
			} else {
				res = regcomp(&pat->rpat1, pat1, REG_EXTENDED | REG_ICASE);
			}
			if (0 != res) {
				g_print
					("error compiling 1st pattern %s: %s because %s\\n",
					 name, pat1, strerror(res));
				switch (mode) {
				case 0:
					regfree(&pat->rpat3);
					g_free(pat->spat3);
				case 1:
					regfree(&pat->rpat2);
					g_free(pat->spat2);
				default:
					break;
				}
				g_free(pat->spat1);
				g_free(pat->name);
				g_free(pat);
				return;
			}
			break;
		case 3:
			{
				gint num;

				num = atoi(pat1);
				if (num <= 0 || num > NUM_SUBMATCHES) {
					g_free(pat->spat1);
					g_free(pat->name);
					g_free(pat);
					return;
				}
			}
			break;
		default:
			break;
		}
		pat->tag = gtk_text_tag_new(NULL);
		if (fgcolor) {
			g_object_set(pat->tag, "foreground", fgcolor, NULL);
		}
		if (bgcolor) {
			g_object_set(pat->tag, "background", bgcolor, NULL);
		}
		if (weight > 0) {
			if (1 == weight) {
				g_object_set(pat->tag, "weight", PANGO_WEIGHT_NORMAL,
							 NULL);
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
		gtk_text_tag_table_add(main_v->tagtable, pat->tag);
		/*
		 * now add the pattern to the hierarchy 
		 */

		if (parent && strlen(parent)) {
			if (!add_pattern_to_name(hlset->highlightlist, parent, pat)) {
				g_print("could NOT add child %s to parent %s\n", name,
						parent);
				pattern_free(pat);
			}
		} else {
			hlset->highlightlist = g_list_append(hlset->highlightlist, pat);
		}
	}
}


Thighlightset *hl_get_highlightset_by_type(gchar *type) {
	GList *tmplist;
	tmplist = g_list_first(main_v->hlsetlist);
	while (tmplist) {
#ifdef DEBUG
		if (!tmplist || !tmplist->data || !((Thighlightset *)tmplist->data)->type) {
			g_print("hl_get_highlightset_by_type is perhaps not yet finished??\n");
			exit(1);
		}
#endif /* DEBUG */
		if (strcmp(((Thighlightset *)tmplist->data)->type, type)==0) {
			return (Thighlightset *)tmplist->data;
		}
	}
	return NULL;
}


void hl_init() {
	GList *tmplist;
	/* init main_v->hlsetlist, the first set is the defaultset */
	
	/* start by initializing the types, they should come from the configfile */
	tmplist = g_list_first(main_v->props.filetypes);
	while (tmplist) {
		gchar **strarr;
		Thighlightset *hlset = g_new0(Thighlightset, 1);
		strarr = (gchar **)tmplist->data;
#ifdef DEBUG
		if (!strarr || !strarr[0] || !strarr[1]) {
			g_print("hl_init is perhaps not yet finished??\n");
			exit(1);
		}
#endif /* DEBUG */
		hlset->type = g_strdup(strarr[0]);
		hlset->extensions = g_strsplit(strarr[1], ":", 20);
		
		main_v->hlsetlist = g_list_append(main_v->hlsetlist, hlset);
		tmplist = g_list_next(tmplist);
	}
	
	/* now start adding all the patterns */
	
	tmplist = g_list_first(main_v->props.highlight_patterns);
	while (tmplist) {
		Thighlightset *hlset;
		gchar **strarr;
		strarr = (gchar **)tmplist->data;

		hlset = hl_get_highlightset_by_type(strarr[0]);
		hl_compile_pattern(hlset, strarr[1], atoi(strarr[2]), strarr[3], strarr[4]
			, strarr[5], atoi(strarr[6]), strarr[7], strarr[8], strarr[9]
			, atoi(strarr[10]), atoi(strarr[11]));
	}

}

Thighlightset *hl_get_highlightset_by_filename(gchar *filename) {
	GList *tmplist;
	/* find set for this filetype */
	if (filename) {
		/* DO SOMETHING */
	
	}
	/* if none found return first set (is default set) */
	tmplist = g_list_first(main_v->hlsetlist);
	if (!tmplist) {
		DEBUG_MSG("hl_get_highlightset_by_filename, no highlightsets?\n");
		return NULL;
	} else {
		return (Thighlightset *)tmplist->data;
	}
}





/*****************************/
/* applying the highlighting */
/*****************************/

static void applylist(Tdocument * doc, GList * list, gint start, gint end,
			   regmatch_t par_pmatch[])
{
	if (list) {
		gchar *string;
		GList *tmplist;

		{
			GtkTextIter itstart, itend;

			gtk_text_buffer_get_iter_at_offset(doc->buffer, &itstart,
											   start);
			gtk_text_buffer_get_iter_at_offset(doc->buffer, &itend, end);

			string =
				gtk_text_buffer_get_text(doc->buffer, &itstart, &itend,
										 FALSE);
#ifdef HL_DEBUG
			g_print("len van string =%d\n", strlen(string));
#endif
		}

		tmplist = g_list_first(list);
		while (tmplist) {
			Tpatmatch patmatch;
			Tpattern *pat = (Tpattern *) tmplist->data;
			gint offset = 0;

			patmatch.is_match = TRUE;

			while (patmatch.is_match) {
				patmatch.is_match = FALSE;
#ifdef HL_DEBUG
				g_print
					("applylist, pattern=%s (mode %d), start=%d, offset=%d, end=%d\n",
					 pat->name, pat->mode, start, offset, end);
#endif
				switch (pat->mode) {
				case 0:

					break;
				case 1:
					{
						gint res1, res2;

						if (pat->need_subpatterns) {
							regmatch_t pmatch[NUM_SUBMATCHES];

							res1 =
								regexec(&pat->rpat1, &string[offset],
										NUM_SUBMATCHES, pmatch, 0);
							if (0 == res1) {
								gint i = 1;

								patmatch.so =
									pmatch[0].rm_so + start + offset;
								while (pmatch[i].rm_so > -1
									   && i < NUM_SUBMATCHES) {
									patmatch.par_pmatch[i].rm_so =
										pmatch[i].rm_so - pmatch[0].rm_so;
									patmatch.par_pmatch[i].rm_eo =
										pmatch[i].rm_eo - pmatch[0].rm_so;
#ifdef HL_DEBUG
									g_print
										("set subpat %d, so=%d, eo=%d\n",
										 i,
										 patmatch.
										 par_pmatch[i].rm_so,
										 patmatch.par_pmatch[i].rm_eo);
#endif
									i++;
								}
#ifdef HL_DEBUG
								g_print("set subpat %d, so=-1, eo=-1\n",
										i);
#endif
								patmatch.par_pmatch[i].rm_so = -1;
								patmatch.par_pmatch[i].rm_eo = -1;
							}
#ifdef HL_DEBUG
//                      g_print("welsub, res1=%d, patmatch.so=%d\n", res1, patmatch.so);
#endif
						} else {
							regmatch_t pmatch[1];

							res1 =
								regexec(&pat->rpat1, &string[offset], 1,
										pmatch, 0);
							if (0 == res1) {
								patmatch.so =
									pmatch[0].rm_so + start + offset;
							}
#ifdef HL_DEBUG
//                      g_print("nosub res1=%d, pmatch[0].rm_so=%d, patmatch.so=%d\n", res1, pmatch[0].rm_so,patmatch.so);
#endif
						}
						if (0 == res1) {
							regmatch_t pmatch[1];

							res2 =
								regexec(&pat->rpat2,
										&string[patmatch.so + 1 - start],
										1, pmatch, 0);
							if (0 == res2) {
								patmatch.eo =
									pmatch[0].rm_eo + patmatch.so + 1;
								patmatch.is_match = TRUE;
							}
						}
					}
					break;
				case 2:
					{
						gint res1;

						if (pat->need_subpatterns) {
							regmatch_t pmatch[NUM_SUBMATCHES];

							res1 =
								regexec(&pat->rpat1, &string[offset],
										NUM_SUBMATCHES, pmatch, 0);
							if (0 == res1) {
								gint i = 1;

								patmatch.so =
									pmatch[0].rm_so + start + offset;
								patmatch.eo =
									pmatch[0].rm_eo + start + offset;
								while (pmatch[i].rm_so > -1
									   && i < NUM_SUBMATCHES) {
									patmatch.par_pmatch[i].rm_so =
										pmatch[i].rm_so - pmatch[0].rm_so;
									patmatch.par_pmatch[i].rm_eo =
										pmatch[i].rm_eo - pmatch[0].rm_so;
#ifdef HL_DEBUG
									g_print
										("set subpat %d, so=%d, eo=%d\n",
										 i,
										 patmatch.
										 par_pmatch[i].rm_so,
										 patmatch.par_pmatch[i].rm_eo);
#endif
									i++;
								}
#ifdef HL_DEBUG
								g_print("set subpat %d, so=-1, eo=-1\n",
										i);
#endif
								patmatch.par_pmatch[i].rm_so = -1;
								patmatch.par_pmatch[i].rm_eo = -1;
							}
						} else {
							regmatch_t pmatch[1];

							res1 =
								regexec(&pat->rpat1, &string[offset], 1,
										pmatch, 0);
							if (0 == res1) {
								patmatch.so =
									pmatch[0].rm_so + start + offset;
								patmatch.eo =
									pmatch[0].rm_eo + start + offset;
							}
						}
						patmatch.is_match = (0 == res1);
					}
					break;
				case 3:
					if (offset == 0) {
						gint num = atoi(pat->spat1);

						patmatch.so = par_pmatch[num].rm_so + start;
						patmatch.eo = par_pmatch[num].rm_eo + start;
						if (par_pmatch[num].rm_so > -1) {
							patmatch.is_match = TRUE;
						}
					}
					break;
				}
#ifdef HL_DEBUG
				g_print("has match=%d (from %d to %d)\n",
						patmatch.is_match, patmatch.so, patmatch.eo);
#endif
				if (patmatch.is_match) {
					GtkTextIter itstart, itend;
#ifdef HL_DEBUG
					g_print("coloring so=%d, eo=%d to tag %d\n",
							patmatch.so, patmatch.eo, pat->style);
#endif
					gtk_text_buffer_get_iter_at_offset(doc->buffer,
													   &itstart,
													   patmatch.so);
					gtk_text_buffer_get_iter_at_offset(doc->buffer, &itend,
													   patmatch.eo);
#ifdef HL_DEBUG
					{
						gchar *tmp = gtk_text_buffer_get_text(doc->buffer,
															  &itstart,
															  &itend,
															  FALSE);

						g_print("coloring '%s'\n", tmp);
						g_free(tmp);
					}
#endif
					gtk_text_buffer_apply_tag(doc->buffer, pat->tag,
											  &itstart, &itend);

					if (pat->childs) {
						applylist(doc, pat->childs, patmatch.so,
								  patmatch.eo, patmatch.par_pmatch);
					}
				}
				offset = patmatch.eo - start;
			}
			tmplist = g_list_next(tmplist);
		}
		g_free(string);
	}
}


static void doc_remove_highlighting(Tdocument *doc) {
	GtkTextIter itstart, itend;
	gtk_text_buffer_get_bounds(doc->buffer,&itstart,&itend);
	gtk_text_buffer_remove_all_tags(doc->buffer, &itstart, &itend);
	doc->hl = NULL;
}

void hl_reset_highlighting_type(Tdocument *doc, gchar *newfilename) {


}

void doc_highlight_full(Tdocument *doc) {
	gint so = 0, eo = gtk_text_buffer_get_char_count(doc->buffer);
	{
		GtkTextIter itstart, itend;
/*		gtk_text_buffer_get_iter_at_offset(doc->buffer, &itstart, so);
		gtk_text_buffer_get_iter_at_offset(doc->buffer, &itend, eo);*/
		gtk_text_buffer_get_start_iter(doc->buffer, &itstart);
		gtk_text_buffer_get_end_iter(doc->buffer, &itend);
		gtk_text_buffer_remove_all_tags(doc->buffer, &itstart, &itend);
	}
	applylist(doc, doc->hl->highlightlist, so, eo, NULL);
}

void doc_highlight_line(Tdocument * doc) {

}
