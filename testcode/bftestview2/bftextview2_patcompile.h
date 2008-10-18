/* for the design docs see bftextview2.h */
#ifndef _BFTEXTVIEW2_PATCOMPILE_H_
#define _BFTEXTVIEW2_PATCOMPILE_H_

#include "bftextview2.h"

guint16 new_context(Tscantable *st, gchar *symbols, GtkTextTag *contexttag, gboolean autocomplete_case_insens);
void match_set_nextcontext(Tscantable *st, guint16 matchnum, guint16 nextcontext);
guint16 add_keyword_to_scanning_table(Tscantable *st, gchar *keyword, gboolean is_regex,gboolean case_insens, GtkTextTag *selftag, guint context, guint nextcontext
				, gboolean starts_block, gboolean ends_block, guint blockstartpattern
				, GtkTextTag *blocktag,gboolean add_to_ac, gchar *append_to_ac, gchar *reference);
void print_DFA(Tscantable *st, char start, char end);
void print_scantable_stats(Tscantable *st);
Tscantable *scantable_new();
Tscantable *bftextview2_scantable_new(GtkTextBuffer *buffer);

#endif
