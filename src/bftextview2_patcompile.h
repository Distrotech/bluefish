/* Bluefish HTML Editor
 * bftextview2_patcompile.h
 *
 * Copyright (C) 2008,2009,2010,2013,2014 Olivier Sessink
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

/* for the design docs see bftextview2.h */
#ifndef _BFTEXTVIEW2_PATCOMPILE_H_
#define _BFTEXTVIEW2_PATCOMPILE_H_

#include "bftextview2.h"
GList *bftextview2_scantable_rematch_highlights(Tscantable * st, const gchar * lang);
gint16 new_context(Tscantable * st, guint expected_size, const gchar * lang, const gchar * symbols, const gchar * contexthighlight,
			gboolean autocomplete_case_insens, gboolean default_spellcheck, gboolean dump_dfa_run);
void match_set_nextcontext(Tscantable * st, guint16 matchnum, guint16 nextcontext);
void match_autocomplete_reference(Tscantable * st, guint16 matchnum, guint16 context);
void match_add_autocomp_item(Tscantable * st, guint16 matchnum, const gchar * autocomplete_string,
						const gchar * autocomplete_append, guint8 autocomplete_backup_cursor,
						guint8 trigger_new_autocomp_popup,
						const gchar *refname, gint relation, gint mode);
void match_set_reference(Tscantable * st, guint16 matchnum, const gchar * reference);
void compile_existing_match(Tscantable * st, guint16 matchnum, gint16 context, gpointer ldb);
void pattern_set_condition(Tscantable * st, guint16 matchnum, gchar *refname, gint relation, gint mode);
void
pattern_set_blockmatch(Tscantable * st, guint16 matchnum,
							gboolean starts_block,
							gboolean ends_block,
							guint blockstartpattern,
							const gchar *blockhighlight,
							const gchar *blockname,
							gboolean foldable);

void
pattern_set_runtime_properties(Tscantable * st, guint16 matchnum,
								const gchar * selfhighlight,
								gint16 nextcontext,
								gboolean tagclose_from_blockstack,
								gboolean stretch_blockstart,
								guint8 identmode,
								gboolean identjump,
								gboolean identautocomp);

guint16
add_pattern_to_scanning_table(Tscantable * st, const gchar * pattern,
								gboolean is_regex,
								gboolean case_insens,
								gint16 context, gpointer ldb);


/*guint16 add_keyword_to_scanning_table(Tscantable * st, gchar * pattern, const gchar * lang,
							  const gchar * selfhighlight, const gchar * blockhighlight, gboolean is_regex,
							  gboolean case_insens, gint16 context, gint16 nextcontext, gboolean starts_block,
							  gboolean ends_block, guint blockstartpattern,
							  gboolean tagclose_from_blockstack, gboolean stretch_blockstart,
							  guint8 identmode, gboolean identjump, gboolean identautocomp);*/
void print_DFA(Tscantable * st, gint16 context, char start, char end);
Tscantable *scantable_new(guint size_table, guint size_matches, guint size_contexts);
Tscantable *bftextview2_scantable_new(GtkTextBuffer * buffer);

void
print_DFA_subset(Tscantable * st, gint16 context, char *chars);

#endif
