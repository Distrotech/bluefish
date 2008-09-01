/* for the design docs see bftextview2.h */
#include "bftextview2.h"
#include "bftextview2_patcompile.h"


Tscantable *bftextview2_scantable_new(GtkTextTag *testtag) {
	Tscantable *st;


	st = g_slice_new0(Tscantable);
	st->table = g_array_sized_new(FALSE,TRUE,sizeof(Ttablerow), 100);
	st->contexts = g_array_sized_new(FALSE,TRUE,sizeof(gint), 3);
	st->matches = g_array_sized_new(FALSE,TRUE,sizeof(Tpattern), 10);

	/* we don't build a automata from patterns right now, because I'm not
	very satisfied with my previous attempt to code that.. 
	we build one by hand just to test scanning.. */ 
	
	/* for testing we are going to scan for a block detected by {} and by () */
	g_array_set_size(st->table,10);
	st->table->len = 10;
	g_array_set_size(st->contexts,2);
	st->matches->len = 2;
	g_array_set_size(st->matches,6);
	st->matches->len = 6;

	g_array_index(st->contexts, gint, 0) = 0;

	g_array_index(st->table, Ttablerow, 0).row['{'] = 1;
	g_array_index(st->table, Ttablerow, 0).row['}'] = 2;
	g_array_index(st->table, Ttablerow, 0).row['('] = 3;
	g_array_index(st->table, Ttablerow, 0).row[')'] = 4;
	g_array_index(st->table, Ttablerow, 0).row['/'] = 5;

	g_array_index(st->table, Ttablerow, 1).match = 0;
	g_array_index(st->matches, Tpattern, 0).message = "{";
	g_array_index(st->matches, Tpattern, 0).starts_block = TRUE;
	g_array_index(st->matches, Tpattern, 0).selftag = testtag;

	g_array_index(st->table, Ttablerow, 2).match = 1;
	g_array_index(st->matches, Tpattern, 1).message = "}";
	g_array_index(st->matches, Tpattern, 1).ends_block = TRUE;
	g_array_index(st->matches, Tpattern, 1).blockstartpattern = 0;
	g_array_index(st->matches, Tpattern, 1).selftag = testtag;
	
	g_array_index(st->table, Ttablerow, 3).match = 2;
	g_array_index(st->matches, Tpattern, 2).message = "(";
	g_array_index(st->matches, Tpattern, 2).starts_block = TRUE;
	g_array_index(st->matches, Tpattern, 2).selftag = testtag;

	g_array_index(st->table, Ttablerow, 4).match = 3;
	g_array_index(st->matches, Tpattern, 3).message = ")";
	g_array_index(st->matches, Tpattern, 3).ends_block = TRUE;
	g_array_index(st->matches, Tpattern, 3).blockstartpattern = 2;
	g_array_index(st->matches, Tpattern, 3).selftag = testtag;

	g_array_index(st->table, Ttablerow, 5).row['*'] = 6;

	g_array_index(st->table, Ttablerow, 6).match = 4;
	g_array_index(st->matches, Tpattern, 4).message = "/*";
	g_array_index(st->matches, Tpattern, 4).starts_block = TRUE;
	g_array_index(st->matches, Tpattern, 4).nextcontext = 1;

	g_array_index(st->contexts, gint, 1) = 7;

	g_array_index(st->table, Ttablerow, 7).row['*'] = 8;
	g_array_index(st->table, Ttablerow, 8).row['/'] = 9;

	g_array_index(st->table, Ttablerow, 9).match = 5;
	g_array_index(st->matches, Tpattern, 5).message = "*/";
	g_array_index(st->matches, Tpattern, 5).ends_block = TRUE;
	g_array_index(st->matches, Tpattern, 5).blockstartpattern = 4;
	g_array_index(st->matches, Tpattern, 5).nextcontext = 0;
	g_array_index(st->matches, Tpattern, 5).blocktag = testtag;

	return st;
}
