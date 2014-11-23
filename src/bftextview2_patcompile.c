/* Bluefish HTML Editor
 * bftextview2_patcompile.c
 *
 * Copyright (C) 2008,2009,2011,2013,2014 Olivier Sessink
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

/*#define ENABLE_PRINT_DFA*/

/* for the design docs see bftextview2.h */
#include <string.h>
#include "bluefish.h"
#include "bftextview2_private.h"
#include "bftextview2_patcompile.h"
#include "bftextview2_langmgr.h"
#include "bf_lib.h"
/*
we do regex pattern support as well.

we don't do everything that pcre or posix regex patterns can
do - these engines have features that cannot be done in a DFA (such as backtracking)

There are several ways in which regex patterns can be simplified:

a(bc)+ == abc(abc)*
[a-z]+ == [a-z][a-z]*
OR reverse:
a(bc)* == (a|a[bc]+)
a[a-z]* == (a|a[a-z]+)

a(bc)? = (a|abc)
a[a-z]? = (a|a[a-z])

a(b|c)d == (abd|acd)

[^a] == [b-z] (and all other ascii characters, for simplification I just use the alphabet)

so at least we need to be able to compile:
the OR construct: (|)
the zero-or-more *
the character list [a-z]

*/

#ifdef ENABLE_PRINT_DFA
void print_DFA_subset(Tscantable * st, gint16 context, char *chars);
#endif

static gint
pointersort_compare(gconstpointer a, gconstpointer b)
{
	return a > b ? -1 : (a == b ? 0 : 1);
}

/* returns a list of tags that are used in this language */
GList *
bftextview2_scantable_rematch_highlights(Tscantable * st, const gchar * lang)
{
	int i = 0;
	GList *retlist = NULL, *tmplist;
	gpointer temp = NULL;
	for (i = 0; i < (st->contexts->len); i++) {
/*		g_print("context %d",i);
		g_print(" has highlight %s\n",g_array_index(st->contexts, Tcontext, i).contexthighlight);*/
		if (g_array_index(st->contexts, Tcontext, i).contexthighlight) {
			g_array_index(st->contexts, Tcontext, i).contexttag =
				langmrg_lookup_tag_highlight(lang, g_array_index(st->contexts, Tcontext, i).contexthighlight);
			if (g_array_index(st->contexts, Tcontext, i).contexttag)
				retlist = g_list_prepend(retlist, g_array_index(st->contexts, Tcontext, i).contexttag);
		}
	}
	for (i = 0; i < (st->matches->len); i++) {
		/*g_print("pattern %d",i);
		   g_print(" (%s)",g_array_index(st->matches, Tpattern, i).pattern);
		   g_print(" has selfhighlight %s and blockhighlight %s\n",g_array_index(st->matches, Tpattern, i).selfhighlight,g_array_index(st->matches, Tpattern, i).blockhighlight); */
		if (g_array_index(st->matches, Tpattern, i).selfhighlight) {
			g_array_index(st->matches, Tpattern, i).selftag =
				langmrg_lookup_tag_highlight(lang, g_array_index(st->matches, Tpattern, i).selfhighlight);
			if (g_array_index(st->matches, Tpattern, i).selftag)
				retlist = g_list_prepend(retlist, g_array_index(st->matches, Tpattern, i).selftag);
			else
				g_print("Possible error in language file, no textstyle found for highlight %s\n",
						g_array_index(st->matches, Tpattern, i).selfhighlight);
		}
		/*if (g_array_index(st->matches, Tpattern, i).blockhighlight) {
			g_array_index(st->matches, Tpattern, i).blocktag =
				langmrg_lookup_tag_highlight(lang, g_array_index(st->matches, Tpattern, i).blockhighlight);
			if (g_array_index(st->matches, Tpattern, i).blocktag)
				retlist = g_list_prepend(retlist, g_array_index(st->matches, Tpattern, i).blocktag);
		}*/
	}
	for (i = 0; i < (st->blocks->len); i++) {
		if (g_array_index(st->blocks, Tpattern_block, i).highlight) {
			g_array_index(st->blocks, Tpattern_block, i).tag = langmrg_lookup_tag_highlight(lang, g_array_index(st->blocks, Tpattern_block, i).highlight);
			if (g_array_index(st->blocks, Tpattern_block, i).tag)
				retlist = g_list_prepend(retlist, g_array_index(st->blocks, Tpattern_block, i).tag);
		}
	}

	/* now remove all duplicate tags */
	tmplist = retlist = g_list_sort(retlist, (GCompareFunc) pointersort_compare);
	while (tmplist) {
		if (tmplist->data == temp) {
			GList *tofree = tmplist;
			tmplist = tmplist->next;
			/* duplicate ! */
			tofree->prev->next = tmplist;
			if (tmplist)
				tmplist->prev = tofree->prev;
			g_list_free_1(tofree);
		} else {
			/* not duplicate */
			temp = tmplist->data;
			tmplist = tmplist->next;
		}
	}

	return retlist;
}

/*static void print_characters(gchar *characters) {
	int i;
	DBG_PATCOMPILE("we have active characters ");
	for (i=0;i<NUMSCANCHARS;i++) {
		if (characters[i]==1)
			DBG_PATCOMPILE("%c ",i);
	}
	DBG_PATCOMPILE("\n");
}*/

static gint
fill_characters_from_range(gchar * input, gchar * characters)
{
	gboolean reverse = 0;
	gint i = 0;
	if (input[i] == '^') {		/* see if it is a NOT pattern */
		reverse = 1;
		memset(characters, 1, NUMSCANCHARS * sizeof(char));
		i++;
	}
	if (input[i] == ']') {		/* ] is actually part of the collection */
		characters['['] = 1;
		i++;
	}
	if (input[i] == '-') {		/* - is actually part of the collection */
		characters['-'] = 1;
		i++;
	}
	while (input[i] != ']') {
		if (input[i] == '-') {	/* range all characters between the previous and the next char */
			gchar j;
			if (input[i + 1] == ']') {	/* - is actually part of the collection */
				characters['-'] = 1;
				i++;
			} else {
				DBG_PATCOMPILE("set characters from %c to %c to %d\n", input[i - 1], input[i + 1],
							   1 - reverse);
				for (j = input[i - 1]; j <= input[i + 1]; j++) {
					characters[(int) j] = 1 - reverse;
				}
				i += 2;
			}
		} else {
			DBG_PATCOMPILE("set characters %c (pos %d) to %d\n", input[i], input[i], 1 - reverse);
			characters[(int) input[i]] = 1 - reverse;
			i++;
		}
	}
	return i;
}

static void
push_on_stack(GQueue * stack, gint state)
{
	GList *tmplist;
	tmplist = stack->head;
	while (tmplist) {
		if (GPOINTER_TO_INT(tmplist->data) == state)
			return;
		tmplist = tmplist->next;
	}
	g_queue_push_head(stack, GINT_TO_POINTER(state));
}

static void
create_state_tables(Tscantable * st, gint16 context, gchar * characters, gboolean pointtoself,
					GQueue * positions, GQueue * newpositions, gboolean end_is_symbol)
{
	guint c, pos;
	const guint identstate =1;
	gint newstate = -1;			/* if all characters can follow existing states we don't need any newstate
								   and thus newstate==-1 but if one or more characters in one or more states need a new state
								   it will be >0 */
	DBG_PATCOMPILE("create_state_tables, context=%d, started with get_length(positions)=%d states, pointtoself=%d, end_is_symbol=%d\n",
				   (gint)context,g_queue_get_length(positions), pointtoself,end_is_symbol);
	while (g_queue_get_length(positions)) {
		pos = GPOINTER_TO_INT(g_queue_pop_head(positions));
		/*DBG_PATCOMPILE("working on position %d, identstate=%d\n", pos, identstate);*/
		for (c = 0; c < NUMSCANCHARS; c++) {
			if (characters[c] == 1) {
				DBG_PATCOMPILE("running for position %d char %c (=%d)\n",pos,c,c);
				/* there is a bug in the pattern compiler that may compile a buggy language file without a warning:
				if a new keyword is added, and all characters already exist there are no new states created: this function
				will simply follow the existing states. BUT IT DOES NOT CHECK IF THERE ARE MULTIPLE WAYS TO GET TO THESE
				EXISTING STATES. So if those states are created by a regex pattern that allows jumping to these states via
				various other characters, you will have an overlapping pattern WITHOUT WARNING.

				You can avoid this bug by having regex patterns alwats in the end of a context.
				 */


				if (get_tablerow(st, context, pos).row[c] != 0
					&& get_tablerow(st, context, pos).row[c] != identstate) {
					if (get_tablerow(st, context, pos).row[c] == pos) {
						/* this state is a 'morestate', a state that points to itself, for a (regex) pattern such as a+ or a* */
						if (pointtoself) {
							push_on_stack(newpositions,
										  (gint) get_tablerow(st, context, pos).row[c]);
						} else {
							if (newstate == -1) {
								newstate = get_table(st, context)->len;
								DBG_PATCOMPILE("create_state_tables, create newstate %d from morestate %d\n",
											   newstate, pos);
								g_array_set_size(get_table(st,context), get_table(st,context)->len + 1);
								if (get_table(st,context)->len+1 >= G_MAXUINT16) {
									g_print("Critical error in language file: state table overflow!!!!!!!!\n");
								}
								/* pass-on the morestate */
								memcpy(get_tablerow(st, context, newstate).row,
									   get_tablerow(st, context, pos).row,
									   sizeof(guint16[NUMSCANCHARS]));
								get_tablerow(st, context, pos).row[c] = newstate;
								g_queue_push_head(newpositions, GINT_TO_POINTER(newstate));
							} else {
								get_tablerow(st, context, pos).row[c] = newstate;
							}
							/*g_print("*****************************\nBUG: we cannot parse this pattern after a regex pattern yet !!!!!!!!!!!!!\nstate tables will be incorrect!!!\n*****************************\n"); */
						}
					} else {
						if (pointtoself) {	/* perhaps check if we have this state on the stack already */
							push_on_stack(positions, (gint) get_tablerow(st, context, pos).row[c]);
						} else {
							if (get_tablerow(st, context, pos).row[c] > pos) {
								DBG_PATCOMPILE("points to state %d, pushing %d on newpositions\n", (gint) get_tablerow(st, context, pos).row[c], (gint) get_tablerow(st, context, pos).row[c]);
								push_on_stack(newpositions,
											  (gint) get_tablerow(st, context, pos).row[c]);
							} else {	/* this is probably a morestate that has been passed-on several states */
								if (newstate == -1) {
									newstate = get_table(st, context)->len;
									DBG_PATCOMPILE
										("create_state_tables, create newstate %d from morestate %d\n",
										 newstate, pos);
									g_array_set_size(get_table(st,context), get_table(st,context)->len + 1);
									if (get_table(st,context)->len+1 >= G_MAXUINT16) {
										g_print("Critical error in language file: state table overflow!!!!!!!!\n");
									}
									/* pass-on the morestate */
									memcpy(get_tablerow(st, context, newstate).row,
										   get_tablerow(st, context, pos).row,
										   sizeof(guint16[NUMSCANCHARS]));
									get_tablerow(st, context, pos).row[c] = newstate;
									g_queue_push_head(newpositions, GINT_TO_POINTER(newstate));
								} else {
									get_tablerow(st, context, pos).row[c] = newstate;
								}
							}

						}
					}
				} else {
					if (newstate == -1) {
						get_tablerow(st, context, pos).row[c] = newstate = get_table(st,context)->len;
						DBG_PATCOMPILE("create_state_tables, create newstate %d, pointtoself=%d\n", newstate,
									   pointtoself);
						g_array_set_size(get_table(st,context), get_table(st,context)->len + 1);
						if (get_table(st,context)->len+1 >= G_MAXUINT16) {
							g_print("Critical error in language file: state table overflow!!!!!!!!\n");
						}
						if (!end_is_symbol) {
							/* normally this memcpy copies the identstate to the current state such that all symbols still point to the
							   startstate but all non-symbols point to the identstate. Only if the last character ITSELF is a symbol, ALL next
							   characters may point to state 0 and make this a valid match and we don't have to copy the identstate  */
							DBG_PATCOMPILE
								("create_state_tables, newstate %d does not end on a symbol, init all values equal to the identstate %d\n",
								 newstate, 1);
							memcpy(get_tablerow(st,context,newstate).row,
								   get_tablerow(st,context,identstate).row,
								   sizeof(guint16[NUMSCANCHARS]));
						} else {
							DBG_PATCOMPILE
								("create_state_tables, newstate %d ends on a symbol, so init all values with 0\n",
								 newstate);
						}
						g_queue_push_head(newpositions, GINT_TO_POINTER(newstate));
						if (pointtoself) {
							guint d;
							for (d = 0; d < NUMSCANCHARS; d++) {
								if (characters[d] == 1) {
									/*DBG_PATCOMPILE("in newstate %d, character %c points to %d\n",newstate,d,newstate); */
									get_tablerow(st,context,newstate).row[d] = newstate;
								}
							}
						}
					} else {
						get_tablerow(st,context,pos).row[c] = newstate;
						/* nothing to put on the stack, newstate should be on newpositions already if it is not -1 */
					}
				}
			}
		}
	}
	DBG_PATCOMPILE("create_state_tables, done, get_lenght(newpositions)=%d\n", g_queue_get_length(newpositions));
}

static void
merge_queues(GQueue * target, GQueue * src)
{
	while (g_queue_get_length(src)) {
		DBG_PATCOMPILE("merge queue, push state %d to queue\n", GPOINTER_TO_INT(g_queue_peek_head(src)));
		g_queue_push_head(target, g_queue_pop_head(src));
	}
	DBG_PATCOMPILE("merge queue, target queue has length %d \n", g_queue_get_length(target));
}

static GQueue *process_regex_part(Tscantable * st, gchar * regexpart, gint16 context,
								  gboolean caseinsensitive, GQueue * inputpositions,
								  gboolean regexpart_ends_regex);

static GQueue *
run_subpatterns(Tscantable * st, gchar * regexpart, gint16 context, gboolean caseinsensitive,
				GQueue * inputpositions, gint * regexpartpos, gboolean regexpart_ends_regex)
{
	gint j = 0;
	gchar *target;
	GQueue *mergednewpositions = g_queue_new();
	target = g_strdup(&regexpart[*regexpartpos]);	/* a very easy way to make target a buffer long enough to hold any subpattern */

	while (regexpart[*regexpartpos] != '\0') {
		DBG_PATCOMPILE("run_subpatterns, regexpart[%d]=%c\n", *regexpartpos, regexpart[*regexpartpos]);
		if (regexpart[*regexpartpos] == '\\') {
			if (regexpart[*regexpartpos + 1] == '|' || regexpart[*regexpartpos + 1] == ')') {
				target[j] = regexpart[*regexpartpos + 1];
				*regexpartpos = *regexpartpos + 1;
				j++;
			} else {
				target[j] = regexpart[*regexpartpos];
				j++;
			}
		} else if ((regexpart[*regexpartpos] == '|' || regexpart[*regexpartpos] == ')')) {
			/* found a subpattern */
			GQueue *newpositions;
			target[j] = '\0';
			DBG_PATCOMPILE("at regexpartpos=%d found SUBPATTERN %s\n", *regexpartpos, target);
			newpositions =
				process_regex_part(st, target, context, caseinsensitive, inputpositions,
								   regexpart_ends_regex);
			merge_queues(mergednewpositions, newpositions);
			g_queue_free(newpositions);
			j = 0;

			if (regexpart[*regexpartpos] == ')') {
				g_free(target);
				return mergednewpositions;
			}
		} else {
			target[j] = regexpart[*regexpartpos];
			j++;
		}
		*regexpartpos = *regexpartpos + 1;
	}
	g_free(target);
	return mergednewpositions;
}

/* this function can be called recursively for subpatterns. It gets all current valid states in
inputpositions, and returns all valid outputstates.

regexpart_ends_regex is a boolean that is used to set the state for the last character of the regex.
if the last character of a pattern is a symbol, the states are different. because this function is
called recursively this is only relevant for the first call.
BUG: this is not true, for a pattern like <(a|b) the a and b are both end-patterns, but they are handled in a
subpattern


*/
static GQueue *
process_regex_part(Tscantable * st, gchar * regexpart, gint16 context, gboolean caseinsensitive,
				   GQueue * inputpositions, gboolean regexpart_ends_regex)
{
	gboolean escaped = FALSE;
	gint i = 0;
	char characters[NUMSCANCHARS];
	GQueue *newpositions, *positions, *tmp;

	positions = g_queue_copy(inputpositions);
	newpositions = g_queue_new();
	DBG_PATCOMPILE("process_regex_part, running part %s\n", regexpart);
	while (1) {
		memset(&characters, 0, NUMSCANCHARS * sizeof(char));
		escaped = 0;
		DBG_PATCOMPILE("start of loop, regexpart[%d]=%c, have %d positions\n", i, regexpart[i],
					   g_queue_get_length(positions));
		if (regexpart[i] == '\0') {	/* end of pattern */
			DBG_PATCOMPILE("end of pattern, positions(%p) has %d entries\n", positions,
						   g_queue_get_length(positions));
			g_queue_free(newpositions);
			return positions;
		} else {
			if (!escaped && regexpart[i] == '\\') {
				DBG_PATCOMPILE("found \\ at position %d:next pattern is escaped\n", i);
				escaped = TRUE;
				i++;
			}
			if (!escaped && regexpart[i] == '(') {
				gchar *tmp;
				gboolean subpattern_ends_regex = FALSE;
				/* a subpattern */
				DBG_PATCOMPILE("found subpatern start at %d\n", i);
				i++;
				tmp = strchr(&regexpart[i], ')');
				/* BUG: this code doesn't handle an escaped \) sequence */
				if (tmp) {
					tmp++;
					if (*tmp == '\0')
						subpattern_ends_regex = TRUE;
				}
				newpositions =
					run_subpatterns(st, regexpart, context, caseinsensitive, positions, &i,
									subpattern_ends_regex);
				DBG_PATCOMPILE("end of subpatern at %d (%c)\n", i, regexpart[i]);
				/* BUG: if the subpattern is followed by an operator foo(bla)? this is not handled by the code !!!!!!!! */
			} else {
				gboolean end_is_symbol = FALSE;
				if (!escaped) {
					switch (regexpart[i]) {
					case '.':
						memset(&characters, 1, NUMSCANCHARS * sizeof(char));
						break;
					case '[':
						DBG_PATCOMPILE("found range at i=%d, fill characters\n", i);
						i += fill_characters_from_range(&regexpart[i + 1], characters) + 1;
						break;
					default:
						characters[(int) regexpart[i]] = 1;
						break;
					}
				} else {		/* escaped */
					DBG_PATCOMPILE("character %c was escaped, adding to characters\n", regexpart[i]);
					characters[(int) regexpart[i]] = 1;
				}
				/* handle case insensitiveness */
				if (caseinsensitive) {
					gint j;
					for (j = 'a'; j <= 'z'; j++) {
						if (characters[j] == 1)
							characters[j - 32] = 1;
						if (characters[j - 32] == 1)
							characters[j] = 1;
					}
				}
				/* BUG: the following code misses a lot of cases where the next state could be a possible
				   end-state already. For example <%[=@]? does not work */
				/* this code used to be
				if (regexpart_ends_regex && regexpart[i] != '\0' && regexpart[i + 1] == '\0') {
				but that was limiting for certain situations
				*/
				if (regexpart_ends_regex) {
					gboolean only_symbols = TRUE;
					/* check if the last character of the regex is a symbol, if so the last state should not
					   refer to the identstate for all non-symbols */
					gint j;
					for (j = 0; j < NUMSCANCHARS; j++) {
						if (characters[j] == 1
							&& !character_is_symbol(st, context, j)) {
							only_symbols = FALSE;
							break;
						}
					}
					if (only_symbols) {
						end_is_symbol = TRUE;
					}
				}
				DBG_PATCOMPILE("i=%d, testing i+1  (%c) for operator\n", i, regexpart[i + 1]);
				/*print_characters(characters); */
				/* see if there is an operator */
				if (regexpart[i] == '\0') {
					DBG_PATCOMPILE("no operator, end of (sub)pattern\n");
					create_state_tables(st, context, characters, FALSE, positions, newpositions,
										end_is_symbol);
				} else if (regexpart[i + 1] == '+') {
					DBG_PATCOMPILE("handling + operator\n");
					create_state_tables(st, context, characters, TRUE, positions, newpositions,
										end_is_symbol);
					i++;
				} else if (regexpart[i + 1] == '*') {
					GQueue *tmp = g_queue_copy(positions);
					DBG_PATCOMPILE("handling * operator\n");
					create_state_tables(st, context, characters, TRUE, positions, newpositions,
										end_is_symbol);
					merge_queues(newpositions, tmp);
					g_queue_free(tmp);
					i++;
				} else if (regexpart[i + 1] == '?') {
					GQueue *tmp = g_queue_copy(positions);
					DBG_PATCOMPILE("handling ? operator\n");
					create_state_tables(st, context, characters, FALSE, positions, newpositions,
										end_is_symbol);
					merge_queues(newpositions, tmp);
					g_queue_free(tmp);
					i++;
				} else {
					DBG_PATCOMPILE("no operator\n");
					create_state_tables(st, context, characters, FALSE, positions, newpositions,
										end_is_symbol);
				}
			}
			g_queue_clear(positions);
			tmp = positions;
			positions = newpositions;
			newpositions = tmp;
		}
		i++;
	}
	/* cannot get here */
}

static void
compile_limitedregex_to_DFA(Tscantable * st, gchar * input, gboolean caseinsensitive, guint16 matchnum,
							gint16 context, gpointer ldb)
{
	GQueue *positions, *newpositions;
	gchar *lregex;
	gint p;
	positions = g_queue_new();

	if (caseinsensitive) {
		/* make complete string lowercase */
		lregex = g_ascii_strdown(input, -1);
	} else {
		lregex = g_strdup(input);
	}

	g_queue_push_head(positions,
					  GINT_TO_POINTER((gint) 0));
	DBG_PATCOMPILE("lregex=%s, positionstack has length %d\n", lregex, g_queue_get_length(positions));
	newpositions = process_regex_part(st, lregex, context, caseinsensitive, positions, TRUE);
	/*compile_limitedregex_to_DFA_backend(st,lregex,context,caseinsensitive,&positions); */
	DBG_PATCOMPILE("after compiling positionstack has length %d\n", g_queue_get_length(positions));
	while ((g_queue_get_length(newpositions))) {
		p = GPOINTER_TO_INT(g_queue_pop_head(newpositions));
		DBG_PATCOMPILE("mark state %d as possible end-state\n", p);
		if (get_tablerow(st, context, p).match != 0
			&& get_tablerow(st, context, p).match != matchnum) {
			gchar *dbstring = ldb_stack_string(ldb);
			g_print("Error in language file %s: patterns %s and %s in context %d overlap each other\n", dbstring, input,
					g_array_index(st->matches, Tpattern,
								  get_tablerow(st, context, p).match).pattern, context);
			g_free(dbstring);
		} else {
			get_tablerow(st, context, p).match = matchnum;
		}
	}
	g_queue_free(positions);
	g_queue_free(newpositions);
	g_free(lregex);

#ifdef ENABLE_PRINT_DFA
	if (context == 1) {
		print_DFA_subset(st, context, "a=1; \n\t");
	}
#endif
}

/* this function cannot do any regex style patterns
just full keywords */
static void
compile_keyword_to_DFA(Tscantable * st, const gchar * keyword, guint16 matchnum, gint16 context,
					   gboolean case_insens, gpointer ldb)
{
	gint i, len;
	GQueue *positions, *newpositions;
	gchar *pattern;
	gchar characters[NUMSCANCHARS];
	gboolean end_is_symbol;

	positions = g_queue_new();
	newpositions = g_queue_new();

	/* compile the keyword into the DFA */
	g_queue_push_head(positions,
					  GINT_TO_POINTER((gint) 0));

	if (!keyword) {
		g_warning("compile_keyword_to_DFA, called without keyword!\n");
		return;
	}

	if (case_insens) {
		/* make complete string lowercase */
		pattern = g_ascii_strdown(keyword, -1);
	} else {
		pattern = g_strdup(keyword);
	}

	DBG_PATCOMPILE("in context %d we start with position %d\n", (gint)context, GPOINTER_TO_INT(g_queue_peek_head(positions)));
	len = strlen(pattern);

	end_is_symbol =
		character_is_symbol(st, context, (gint) pattern[len - 1]);

	for (i = 0; i <= len; i++) {
		/*GQueue *tmp; */
		int c = pattern[i];
		memset(&characters, 0, NUMSCANCHARS * sizeof(char));
		if (c == '\0') {
			while ((g_queue_get_length(positions))) {
				gint p;
				p = GPOINTER_TO_INT(g_queue_pop_head(positions));
				DBG_PATCOMPILE("mark state %d as possible end-state\n", p);
				if (get_tablerow(st, context, p).match != 0
					&& get_tablerow(st, context, p).match != matchnum) {
					gchar *dbstring = ldb_stack_string(ldb);
					g_print("Error in language file %s: patterns %s and %s in context %d overlap each other\n",
							dbstring, keyword, g_array_index(st->matches, Tpattern,
												   get_tablerow(st, context, p).match).pattern,
							context);
					g_free(dbstring);
				}
				get_tablerow(st, context, p).match = matchnum;
			}
		} else {
			characters[c] = 1;
			if (case_insens && c >= 97 && c <= 122)
				characters[c - 32] = 1;
			create_state_tables(st, context, characters, FALSE, positions, newpositions, end_is_symbol);
		}
		g_queue_clear(positions);
		pointer_switch_addresses((void *) &positions, (void *) &newpositions);
		/*tmp = positions;
		   positions = newpositions;
		   newpositions = tmp; */
	}
	g_queue_clear(positions);
	g_queue_clear(newpositions);
	g_queue_free(positions);
	g_queue_free(newpositions);
	g_free(pattern);
}

gint16
new_context(Tscantable * st, guint expected_size, const gchar * lang, const gchar * symbols, const gchar * contexthighlight,
			gboolean autocomplete_case_insens, gboolean default_spellcheck, gboolean dump_dfa_run)
{
	gint16 context;
	gint i;
	const gchar *tmp;
	GArray *tmptable;

	context = st->contexts->len;
	g_array_set_size(st->contexts, st->contexts->len + 1);

	g_array_index(st->contexts, Tcontext, context).autocomplete_case_insens = autocomplete_case_insens;
	g_array_index(st->contexts, Tcontext, context).default_spellcheck = default_spellcheck;
	g_array_index(st->contexts, Tcontext, context).dump_dfa_run = dump_dfa_run;
	g_array_index(st->contexts, Tcontext, context).contexthighlight = (gchar *) contexthighlight;
	tmptable = g_array_index(st->contexts, Tcontext, context).table = g_array_sized_new(TRUE, TRUE, sizeof(Ttablerow), expected_size);
	g_array_set_size(g_array_index(st->contexts, Tcontext, context).table, 2); /* first two states are the startstate (0) and the identstate (1) */
	/* identstate refers to itself for all characters except the symbols. we cannot use memset
	   because an guint16 occupies 2 bytes */
	for (i = 0; i < NUMSCANCHARS; i++)
		g_array_index(tmptable, Ttablerow, 1).row[i] = 1;

	/* 0, \0 or NULL is always a symbol */
	g_array_index(tmptable, Ttablerow, 1).row[0] = 0;
	tmp = symbols;
	while (*tmp) {
		/*g_print("mark %c as symbol\n",*tmp); */
		g_array_index(tmptable, Ttablerow, 1).row[(int) *tmp] = 0;

		if (st->allsymbols[(int) *tmp] == 0)
			st->allsymbols[(int) *tmp] = 1;

		tmp++;
	}
	/* now copy the identstate to the startstate, so every symbol in the startstate will point to the identstate */
	memcpy(g_array_index(tmptable, Ttablerow, 0).row,
		   g_array_index(tmptable, Ttablerow, 1).row, sizeof(guint16[NUMSCANCHARS]));

	return context;
}

void
match_set_nextcontext(Tscantable * st, guint16 matchnum, guint16 nextcontext)
{
	DBG_PATCOMPILE("match_set_nextcontext, set match %d to have nextcontext %d\n", matchnum, nextcontext);
	g_array_index(st->matches, Tpattern, matchnum).nextcontext = nextcontext;
}

void
match_autocomplete_reference(Tscantable * st, guint16 matchnum, guint16 context)
{
	gint pattern_id = matchnum;
	if (!g_array_index(st->contexts, Tcontext, context).patternhash) {
		g_array_index(st->contexts, Tcontext, context).patternhash =
			g_hash_table_new(g_str_hash, g_str_equal);
	}
	g_hash_table_insert(g_array_index(st->contexts, Tcontext, context).patternhash,
						g_array_index(st->matches, Tpattern, matchnum).pattern, GINT_TO_POINTER(pattern_id));

	if (g_array_index(st->matches, Tpattern, matchnum).tagclose_from_blockstack
		&& !g_array_index(st->contexts, Tcontext, context).has_tagclose_from_blockstack) {
		DBG_AUTOCOMP("pattern %d in context %d has tagclose from blockstack!\n", matchnum, context);
		g_array_index(st->contexts, Tcontext, context).has_tagclose_from_blockstack = 1;
	}

/*	if (g_array_index(st->matches, Tpattern, matchnum).reference && !g_array_index(st->matches, Tpattern, matchnum).is_regex) {
		if (!g_array_index(st->contexts, Tcontext, context).reference) {
			DBG_PATCOMPILE("create hashtable for context %d\n",context);
			g_array_index(st->contexts, Tcontext, context).reference = g_hash_table_new(g_str_hash,g_str_equal);
		}
		g_hash_table_insert(g_array_index(st->contexts, Tcontext, context).reference,g_array_index(st->matches, Tpattern, matchnum).pattern,g_array_index(st->matches, Tpattern, matchnum).reference);
	}*/
	if (g_array_index(st->matches, Tpattern, matchnum).autocomp_items) {
		GSList *tmpslist = g_array_index(st->matches, Tpattern, matchnum).autocomp_items;
		GList *list = NULL;
		if (!g_array_index(st->contexts, Tcontext, context).ac) {
			DBG_PATCOMPILE("create g_completion for context %d\n", context);
			g_array_index(st->contexts, Tcontext, context).ac = g_completion_new(NULL);
			if (g_array_index(st->contexts, Tcontext, context).autocomplete_case_insens)
				g_completion_set_compare(g_array_index(st->contexts, Tcontext, context).ac, strncasecmp);
		}

		while (tmpslist) {
			Tpattern_autocomplete *pac = tmpslist->data;
			list = g_list_prepend(list, pac->autocomplete_string);
			g_hash_table_insert(g_array_index(st->contexts, Tcontext, context).patternhash,
								pac->autocomplete_string, GINT_TO_POINTER(pattern_id));
			tmpslist = g_slist_next(tmpslist);
		}
		g_completion_add_items(g_array_index(st->contexts, Tcontext, context).ac, list);
		g_list_free(list);
	}
#ifdef OLD_AUTOCOMP
	if (g_array_index(st->matches, Tpattern, matchnum).autocomplete) {
		GList *list;
		gchar *tmp;
		if (!g_array_index(st->contexts, Tcontext, context).ac) {
			DBG_PATCOMPILE("create g_completion for context %d\n", context);
			g_array_index(st->contexts, Tcontext, context).ac = g_completion_new(NULL);
			if (g_array_index(st->contexts, Tcontext, context).autocomplete_case_insens)
				g_completion_set_compare(g_array_index(st->contexts, Tcontext, context).ac, strncasecmp);
		}
		if (g_array_index(st->matches, Tpattern, matchnum).autocomplete_string) {
			tmp = g_array_index(st->matches, Tpattern, matchnum).autocomplete_string;
		} else {
			tmp = g_array_index(st->matches, Tpattern, matchnum).pattern;
		}

		list = g_list_prepend(NULL, tmp);
		g_completion_add_items(g_array_index(st->contexts, Tcontext, context).ac, list);
		DBG_AUTOCOMP("adding %s to GCompletion\n", (gchar *) list->data);
		g_list_free(list);
		if (g_array_index(st->matches, Tpattern, matchnum).autocomplete_string) {
			/*if (g_array_index(st->matches, Tpattern, matchnum).reference) {
			   g_hash_table_insert(g_array_index(st->contexts, Tcontext, context).reference,g_array_index(st->matches, Tpattern, matchnum).autocomplete_string,g_array_index(st->matches, Tpattern, matchnum).reference);
			   } */
			g_hash_table_insert(g_array_index(st->contexts, Tcontext, context).patternhash,
								g_array_index(st->matches, Tpattern, matchnum).autocomplete_string,
								GINT_TO_POINTER(pattern_id));
		}
	}
#endif
}


void
match_add_autocomp_item(Tscantable * st, guint16 matchnum, const gchar * autocomplete_string,
						const gchar * autocomplete_append, guint8 autocomplete_backup_cursor)
{
	Tpattern_autocomplete *pac = g_slice_new(Tpattern_autocomplete);
	if (autocomplete_string) {
		pac->autocomplete_string = g_strdup(autocomplete_string);
	} else if (autocomplete_append) {
		pac->autocomplete_string =
			g_strconcat(g_array_index(st->matches, Tpattern, matchnum).pattern, autocomplete_append, NULL);
	} else {
		pac->autocomplete_string = g_array_index(st->matches, Tpattern, matchnum).pattern;
	}

#ifdef DEVELOPMENT
	GSList *slist;
	slist=g_array_index(st->matches, Tpattern, matchnum).autocomp_items;
	while (slist) {
		Tpattern_autocomplete *tpac = slist->data;
		if (g_strcmp0(pac->autocomplete_string, tpac->autocomplete_string)==0) {
			g_print("Warning: string %s exists as autocomplete string for pattern %d\n",pac->autocomplete_string,matchnum);
		}
		slist = g_slist_next(slist);
	}
#endif
	pac->autocomplete_backup_cursor = autocomplete_backup_cursor;
	g_array_index(st->matches, Tpattern, matchnum).autocomp_items =
		g_slist_prepend(g_array_index(st->matches, Tpattern, matchnum).autocomp_items, pac);
}

void
match_set_reference(Tscantable * st, guint16 matchnum, const gchar * reference)
{
	if (reference)
		g_array_index(st->matches, Tpattern, matchnum).reference = g_strdup(reference);
}

void
compile_existing_match(Tscantable * st, guint16 matchnum, gint16 context, gpointer ldb)
{
	DBG_PATCOMPILE("compile existing match %d (%s) in context %d\n", matchnum,
				   g_array_index(st->matches, Tpattern, matchnum).pattern, context);
	if (g_array_index(st->matches, Tpattern, matchnum).is_regex) {
		compile_limitedregex_to_DFA(st, g_array_index(st->matches, Tpattern, matchnum).pattern,
									g_array_index(st->matches, Tpattern, matchnum).case_insens, matchnum,
									context, ldb);
	} else {
		compile_keyword_to_DFA(st, g_array_index(st->matches, Tpattern, matchnum).pattern, matchnum, context,
							   g_array_index(st->matches, Tpattern, matchnum).case_insens, ldb);
	}
	match_autocomplete_reference(st, matchnum, context);
}
#ifdef CONDITIONALPATTERN

void
pattern_set_condition(Tscantable * st, guint16 matchnum, gchar *refname, gint relation, gint mode)
{
	guint16 condnum = st->conditions->len;
	DBG_PARSING("pattern_set_condition, condnum=%d, called with refname %s for matchnum %d\n",condnum,refname,matchnum);
	g_array_set_size(st->conditions, st->conditions->len + 1);
	if (mode < 1 || mode > 4) {
		g_warning("Error in language file: condition mode %d is not defined\n",mode);
		return;
	}
	g_array_index(st->conditions, Tpattern_condition, condnum).refname = g_strdup(refname);
	g_array_index(st->conditions, Tpattern_condition, condnum).parentrelation = relation;
	g_array_index(st->conditions, Tpattern_condition, condnum).relationtype = mode;
	DBG_PARSING("pattern %d (%s) now has condition %d\n",matchnum,g_array_index(st->matches, Tpattern, matchnum).pattern,condnum);
	g_array_index(st->matches, Tpattern, matchnum).condition = condnum;
}
#endif /*CONDITIONALPATTERN*/
void
pattern_set_blockmatch(Tscantable * st, guint16 matchnum,
							gboolean starts_block,
							gboolean ends_block,
							guint blockstartpattern,
							const gchar *blockhighlight,
							const gchar *blockname,
							gboolean foldable)
{
	if (starts_block == TRUE && ends_block == TRUE) {
		g_warning("Error in language file or Bluefish bug: pattern %s both starts and ends a block\n",
						g_array_index(st->matches, Tpattern, matchnum).pattern);
	}
	if (blockname && !starts_block) {
		g_warning("Error in language file or Bluefish bug: block_name %s can only be set on a block start\n", blockname);
	}

	if (starts_block) {
		guint16 blocknum = 0;
		if (blockname || blockhighlight) { /* only create a block if we need it */
			blocknum = st->blocks->len;
			g_array_set_size(st->blocks, st->blocks->len + 1);
			g_array_index(st->blocks, Tpattern_block, blocknum).highlight = (gchar *) blockhighlight;
			g_array_index(st->blocks, Tpattern_block, blocknum).name = (gchar *) blockname;
			g_array_index(st->blocks, Tpattern_block, blocknum).foldable = foldable;
			g_array_index(st->blocks, Tpattern_block, blocknum).tag = NULL;
		}
		g_array_index(st->matches, Tpattern, matchnum).starts_block = 1;
		g_array_index(st->matches, Tpattern, matchnum).block = blocknum;
	} else if (ends_block) {
		g_array_index(st->matches, Tpattern, matchnum).ends_block = 1;
	}
	g_array_index(st->matches, Tpattern, matchnum).blockstartpattern = blockstartpattern;
}

void
pattern_set_runtime_properties(Tscantable * st, guint16 matchnum,
								const gchar * selfhighlight,
								gint16 nextcontext,
								gboolean tagclose_from_blockstack,
								gboolean stretch_blockstart,
								guint8 identmode,
								gboolean identjump,
								gboolean identautocomp)
{
	g_array_index(st->matches, Tpattern, matchnum).selfhighlight = (gchar *) selfhighlight;
	g_array_index(st->matches, Tpattern, matchnum).nextcontext = nextcontext;
	g_array_index(st->matches, Tpattern, matchnum).tagclose_from_blockstack = tagclose_from_blockstack;
	g_array_index(st->matches, Tpattern, matchnum).stretch_blockstart = stretch_blockstart;
	if (identjump || identautocomp) {
		g_array_index(st->matches, Tpattern, matchnum).identmode = identmode;
		g_array_index(st->matches, Tpattern, matchnum).identaction = (identjump?1:0) | (identautocomp?2:0);
	}
}

guint16
add_pattern_to_scanning_table(Tscantable * st, const gchar * pattern,
								gboolean is_regex,
								gboolean case_insens,
								gint16 context, gpointer ldb)
{
	guint16 matchnum;
	if (!pattern || pattern[0] == '\0') {
		g_warning("corrupt language file: found empty pattern. Results are undefined\n");
		return 0;
	}
	matchnum = st->matches->len;
	if (matchnum == G_MAXUINT16) {
		g_warning("Language file has too many patterns, this will very likely result in a crash!\n");
	}
	g_array_set_size(st->matches, st->matches->len + 1);
	g_array_index(st->matches, Tpattern, matchnum).pattern = g_strdup(pattern);
	g_array_index(st->matches, Tpattern, matchnum).case_insens = case_insens;
	g_array_index(st->matches, Tpattern, matchnum).is_regex = is_regex;
	DBG_PATCOMPILE("add_pattern_to_scanning_table,pattern=%s for context=%d got matchnum %d\n",pattern, context, matchnum);
	if (is_regex) {
		compile_limitedregex_to_DFA(st, g_array_index(st->matches, Tpattern, matchnum).pattern, case_insens, matchnum, context, ldb);
	} else {
		compile_keyword_to_DFA(st, g_array_index(st->matches, Tpattern, matchnum).pattern, matchnum, context, case_insens, ldb);
	}
	/*if (g_strcmp0(pattern, "rem ")==0 || g_strcmp0(pattern, "\\.?[a-zA-Z][a-zA-Z_0-9]*[\\$%]?")==0) {
		print_DFA_subset(st, context, "rem var");
	}*/
	return matchnum;
}


/*guint16
add_keyword_to_scanning_table(Tscantable * st, gchar * pattern, const gchar * lang,
							  const gchar * selfhighlight, const gchar * blockhighlight, gboolean is_regex,
							  gboolean case_insens, gint16 context, gint16 nextcontext, gboolean starts_block,
							  gboolean ends_block, guint blockstartpattern,
							  gboolean tagclose_from_blockstack, gboolean stretch_blockstart,
							  guint8 identmode, gboolean identjump, gboolean identautocomp)
{
	guint16 matchnum;

	if (!pattern || pattern[0] == '\0') {
		g_print("CORRUPT LANGUAGE FILE: empty pattern/tag/keyword\n");
		return 0;
	}

	if (context == nextcontext)
		DBG_PATCOMPILE("context=nextcontext=%d for %s\n", context, pattern);

	matchnum = st->matches->len;
	DBG_BLOCKMATCH("new match %s at matchnum %d has blockstartpattern %d and nextcontext %d\n", pattern,
				   matchnum, blockstartpattern, nextcontext);
	g_array_set_size(st->matches, st->matches->len + 1);

	g_array_index(st->matches, Tpattern, matchnum).pattern = g_strdup(pattern);
	g_array_index(st->matches, Tpattern, matchnum).ends_block = ends_block;
	g_array_index(st->matches, Tpattern, matchnum).starts_block = starts_block;
	g_array_index(st->matches, Tpattern, matchnum).blockstartpattern = blockstartpattern;
	g_array_index(st->matches, Tpattern, matchnum).blockhighlight = (gchar *) blockhighlight;
	g_array_index(st->matches, Tpattern, matchnum).nextcontext = nextcontext;
	g_array_index(st->matches, Tpattern, matchnum).case_insens = case_insens;
	g_array_index(st->matches, Tpattern, matchnum).is_regex = is_regex;
	g_array_index(st->matches, Tpattern, matchnum).selfhighlight = (gchar *) selfhighlight;

	g_array_index(st->matches, Tpattern, matchnum).tagclose_from_blockstack = tagclose_from_blockstack;
	g_array_index(st->matches, Tpattern, matchnum).stretch_blockstart = stretch_blockstart;
#ifdef IDENTSTORING
	if (identjump || identautocomp) {
		g_array_index(st->matches, Tpattern, matchnum).identmode = identmode;
		g_array_index(st->matches, Tpattern, matchnum).identaction = (identjump?1:0) | (identautocomp?2:0);
	}
#endif
	DBG_PATCOMPILE
		("add_keyword_to_scanning_table,pattern=%s,starts_block=%d,ends_block=%d,blockstartpattern=%d, context=%d,nextcontext=%d and got matchnum %d\n",
		 pattern, starts_block, ends_block, blockstartpattern, context, nextcontext, matchnum);

	if (is_regex) {
		compile_limitedregex_to_DFA(st, pattern, case_insens, matchnum, context);
	} else {
		compile_keyword_to_DFA(st, pattern, matchnum, context, case_insens);
	}
	/ *print_DFA(st, 'a', 'z'); * /
	return matchnum;
}*/

void
print_DFA_subset(Tscantable * st, gint16 context, char *chars)
{
	gint i, j, len;
	len = strlen(chars);
	g_print("***************** print subset of DFA table for context %d\n",(gint)context);
	g_print("       ");
	for (j = 0; j < len; j++) {
		switch(chars[j]) {
			case '\n':
				g_print(" \\n   ");
				break;
			case '\r':
				g_print(" \\r   ");
				break;
			case '\t':
				g_print(" \\t   ");
				break;
			default:
				g_print(" '%c' ", chars[j]);
		}
	}
	g_print(": match\n");
	for (i = 0; i < g_array_index(st->contexts, Tcontext, context).table->len; i++) {
		g_print("%4d: ", i);
		for (j = 0; j < len; j++) {
			g_print("%4d ", g_array_index(g_array_index(st->contexts, Tcontext, context).table, Ttablerow, i).row[(gshort) chars[j]]);
		}
		g_print(" : %4d", g_array_index(g_array_index(st->contexts, Tcontext, context).table, Ttablerow, i).match);
		if (i==0) {
			g_print(" 	this is the startstate");
		} else if (i==1) {
			g_print(" 	this is the identstate");
		}
		if (g_array_index(g_array_index(st->contexts, Tcontext, context).table, Ttablerow, i).match > 0) {
			g_print(" %s",
					g_array_index(st->matches, Tpattern,
								  g_array_index(g_array_index(st->contexts, Tcontext, context).table, Ttablerow, i).match).pattern);
			if (g_array_index(st->matches, Tpattern, g_array_index(g_array_index(st->contexts, Tcontext, context).table, Ttablerow, i).match).nextcontext
				> 0) {
				g_print(" 	--> goto context %d",
						g_array_index(st->matches, Tpattern,
									  g_array_index(g_array_index(st->contexts, Tcontext, context).table, Ttablerow, i).match).nextcontext);
			} else
				if (g_array_index
					(st->matches, Tpattern, g_array_index(g_array_index(st->contexts, Tcontext, context).table, Ttablerow, i).match).nextcontext < 0) {
				g_print(" 	--> pop context: %d",
						g_array_index(st->matches, Tpattern,
									  g_array_index(g_array_index(st->contexts, Tcontext, context).table, Ttablerow, i).match).nextcontext);
			}
		}
		g_print("\n");
	}
	g_print("*****************\n");
}
#ifdef ENABLE_PRINT_DFA
void
print_DFA(Tscantable * st, gint16 context, char start, char end)
{
	gint i, j;
	g_print("    ");
	for (j = start; j <= end; j++) {
		g_print("  %c ", j);
	}
	g_print(": match\n");
	for (i = 0; i < g_array_index(st->contexts, Tcontext, context).table->len; i++) {
		g_print("%3d: ", i);
		for (j = start; j <= end; j++) {
			g_print("%3d ", g_array_index(g_array_index(st->contexts, Tcontext, context).table, Ttablerow, i).row[j]);
		}
		g_print(": %3d (%s)\n", g_array_index(g_array_index(st->contexts, Tcontext, context).table, Ttablerow, i).match,
					g_array_index(st->matches, Tpattern,g_array_index(g_array_index(st->contexts, Tcontext, context).table, Ttablerow, i).match).pattern);
	}

}
#endif /* ENABLE_PRINT_DFA */

Tscantable *
scantable_new(guint size_table, guint size_matches, guint size_contexts)
{
	Tscantable *st;
	st = g_slice_new0(Tscantable);
	DBG_PARSING("scantable_new, initial size table %d, contexts %d, matches %d\n", size_table, size_matches,
				size_contexts);
	st->contexts = g_array_sized_new(TRUE, TRUE, sizeof(Tcontext), size_contexts);
	st->matches = g_array_sized_new(TRUE, TRUE, sizeof(Tpattern), size_matches);
	st->comments = g_array_sized_new(TRUE, FALSE, sizeof(Tcomment), 8);
	st->blocks = g_array_sized_new(TRUE, FALSE, sizeof(Tpattern_block), 8);
	st->conditions = g_array_sized_new(TRUE, FALSE, sizeof(Tpattern_block), 8);
	st->matches->len = 1;		/* match 0 means no match */
	st->contexts->len = 1;		/* a match with nextcontext 0 means no context change, so we cannot use context 0 */
	st->blocks->len = 1;			/* block 0 means no block */
	st->conditions->len = 1;
	return st;
}

