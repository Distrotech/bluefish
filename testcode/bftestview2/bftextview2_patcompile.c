/* for the design docs see bftextview2.h */
#include "bftextview2.h"
#include "bftextview2_patcompile.h"


Tscantable *bftextview2_scantable_new(void) {
	Tscantable *st;

	Ttablerow tr;
	Tpatter pat;
	gint context;
	
	st = g_slice_new0(Tscantable);
	st->table = g_array_sized_new(FALSE,TRUE,sizeof(Ttablerow), 100);
	st->context = g_array_sized_new(FALSE,TRUE,sizeof(gint), 3);
	st->matches = g_array_sized_new(FALSE,TRUE,sizeof(Tpattern), 10);

	/* we don't build a automata from patterns right now, because I'm not
	very satisfied with my previous attempt to code that.. 
	we build one by hand just to test scanning.. */ 
	
	/* for testing we are going to scan for a block detected by {} and by () */
	g_array_set_size(st->table,8);
	st->table->len = 8;
	g_array_index(st->table, Ttablerow, 0).row['{'] = 1;
	g_array_index(st->table, Ttablerow, 0).row['}'] = 2;
	g_array_index(st->table, Ttablerow, 0).row['('] = 3;
	g_array_index(st->table, Ttablerow, 0).row[')'] = 4;
	g_array_index(st->table, Ttablerow, 1).match = 0;
	g_array_index(st->table, Ttablerow, 2).match = 1;
	g_array_index(st->table, Ttablerow, 3).match = 2;
	g_array_index(st->table, Ttablerow, 4).match = 3;
	
	g_array_set_size(st->matches,4);
	st->matches->len = 4;
	g_array_index(st->table, Tpattern, 0).message = "{";
	g_array_index(st->table, Tpattern, 0).starts_block = TRUE;
	g_array_index(st->table, Tpattern, 1).message = "}";
	g_array_index(st->table, Tpattern, 1).ends_block = TRUE;
	g_array_index(st->table, Tpattern, 1).blockstartpattern = 0;
	g_array_index(st->table, Tpattern, 2).message = "(";
	g_array_index(st->table, Tpattern, 2).starts_block = TRUE;
	g_array_index(st->table, Tpattern, 3).message = ")";
	g_array_index(st->table, Tpattern, 3).ends_block = TRUE;
	g_array_index(st->table, Tpattern, 3).blockstartpattern = 0;
	
	return st;
}
