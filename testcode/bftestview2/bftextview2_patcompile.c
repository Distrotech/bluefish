/* for the design docs see bftextview2.h */
#include "bftextview2.h"
#include "bftextview2_patcompile.h"


Tscantable *bftextview2_scantable_new(GtkTextBuffer *buffer) {
	Tscantable *st;

	GtkTextTag *braces, *comment, *storage;
	
	braces = gtk_text_buffer_create_tag(buffer,"braces","weight", PANGO_WEIGHT_BOLD,"foreground","darkblue",NULL);
	comment = gtk_text_buffer_create_tag(buffer,"comment","style", PANGO_STYLE_ITALIC,"foreground", "grey", NULL);
	storage = gtk_text_buffer_create_tag(buffer,"storage","weight", PANGO_WEIGHT_BOLD,"foreground", "darkred", NULL);
	

	st = g_slice_new0(Tscantable);
	st->table = g_array_sized_new(FALSE,TRUE,sizeof(Ttablerow), 100);
	st->contexts = g_array_sized_new(FALSE,TRUE,sizeof(Tcontext), 3);
	st->matches = g_array_sized_new(FALSE,TRUE,sizeof(Tpattern), 10);

	/* we don't build a automata from patterns right now, because I'm not
	very satisfied with my previous attempt to code that.. 
	we build one by hand just to test scanning.. */ 
	
	/* for testing we are going to scan for a block detected by {} and by ()
	and we do C comments, and we scan for the keyword void */
#ifdef CSTYLEMATCHING
	g_array_set_size(st->table,14);
	st->table->len = 14;
	g_array_set_size(st->contexts,2);
	st->matches->len = 2;
	g_array_set_size(st->matches,8);
	st->matches->len = 8; / * match 0 is not used * /

	g_array_index(st->contexts, Tcontext, 0).startstate = 0;
	g_array_index(st->contexts, Tcontext, 0).ac = g_completion_new(NULL);
	{
		GList *list = g_list_prepend(NULL, "void"); 
		g_completion_add_items(g_array_index(st->contexts, Tcontext, 0).ac, list);
		g_list_free(list);
	}

	g_array_index(st->table, Ttablerow, 0).row['{'] = 1;
	g_array_index(st->table, Ttablerow, 0).row['}'] = 2;
	g_array_index(st->table, Ttablerow, 0).row['('] = 3;
	g_array_index(st->table, Ttablerow, 0).row[')'] = 4;
	g_array_index(st->table, Ttablerow, 0).row['/'] = 5;
	g_array_index(st->table, Ttablerow, 0).row['v'] = 10;

	g_array_index(st->table, Ttablerow, 1).match = 6;
	g_array_index(st->matches, Tpattern, 6).message = "{";
	g_array_index(st->matches, Tpattern, 6).starts_block = TRUE;
	g_array_index(st->matches, Tpattern, 6).selftag = braces;

	g_array_index(st->table, Ttablerow, 2).match = 1;
	g_array_index(st->matches, Tpattern, 1).message = "}";
	g_array_index(st->matches, Tpattern, 1).ends_block = TRUE;
	g_array_index(st->matches, Tpattern, 1).blockstartpattern = 6;
	g_array_index(st->matches, Tpattern, 1).selftag = braces;
	
	g_array_index(st->table, Ttablerow, 3).match = 2;
	g_array_index(st->matches, Tpattern, 2).message = "(";
	g_array_index(st->matches, Tpattern, 2).starts_block = TRUE;
	g_array_index(st->matches, Tpattern, 2).selftag = braces;

	g_array_index(st->table, Ttablerow, 4).match = 3;
	g_array_index(st->matches, Tpattern, 3).message = ")";
	g_array_index(st->matches, Tpattern, 3).ends_block = TRUE;
	g_array_index(st->matches, Tpattern, 3).blockstartpattern = 2;
	g_array_index(st->matches, Tpattern, 3).selftag = braces;

	g_array_index(st->table, Ttablerow, 5).row['*'] = 6;

	g_array_index(st->table, Ttablerow, 6).match = 4;
	g_array_index(st->matches, Tpattern, 4).message = "/*";
	g_array_index(st->matches, Tpattern, 4).starts_block = TRUE;
	g_array_index(st->matches, Tpattern, 4).nextcontext = 1;

	g_array_index(st->matches, Tpattern, 4).selftag = comment;
	
	g_array_index(st->contexts, Tcontext, 1).startstate = 7;

	g_array_index(st->table, Ttablerow, 7).row['*'] = 8;
	g_array_index(st->table, Ttablerow, 8).row['/'] = 9;

	g_array_index(st->table, Ttablerow, 9).match = 5;
	g_array_index(st->matches, Tpattern, 5).message = "*/";
	g_array_index(st->matches, Tpattern, 5).ends_block = TRUE;
	g_array_index(st->matches, Tpattern, 5).blockstartpattern = 4;
	g_array_index(st->matches, Tpattern, 5).nextcontext = 0;
	g_array_index(st->matches, Tpattern, 5).blocktag = comment;
	g_array_index(st->matches, Tpattern, 5).selftag = comment;

	g_array_index(st->table, Ttablerow, 10).row['o'] = 11;
	g_array_index(st->table, Ttablerow, 11).row['i'] = 12;
	g_array_index(st->table, Ttablerow, 12).row['d'] = 13;
	
	g_array_index(st->table, Ttablerow, 13).match = 7;
	g_array_index(st->matches, Tpattern, 7).message = "void";
	g_array_index(st->matches, Tpattern, 7).selftag = storage;
#endif /* #ifdef CSTYLEMATCHING */
/* for more testing we try some html tags */
#define HTMLSTYLEMATCHING
#ifdef HTMLSTYLEMATCHING
	g_array_set_size(st->table,20);
	st->table->len = 20;
	g_array_set_size(st->contexts,4);
	st->contexts->len = 4;
	g_array_set_size(st->matches,8);
	st->matches->len = 8; /* match 0 is not used */

	g_array_index(st->contexts, Tcontext, 0).startstate = 0;
	g_array_index(st->contexts, Tcontext, 0).ac = g_completion_new(NULL);
	{
		GList *list = NULL;
		list = g_list_prepend(list, "<img");
		list = g_list_prepend(list, "<i");
		list = g_list_prepend(list, "<!--");
		g_completion_add_items(g_array_index(st->contexts, Tcontext, 0).ac, list);
		g_list_free(list);
	}

	g_array_index(st->table, Ttablerow, 0).row['<'] = 1;
	
	/* comment <!-- */
	g_array_index(st->table, Ttablerow, 1).row['!'] = 2;
	g_array_index(st->table, Ttablerow, 2).row['-'] = 3;
	g_array_index(st->table, Ttablerow, 3).row['-'] = 4;
	g_array_index(st->table, Ttablerow, 4).match = 1;
	g_array_index(st->matches, Tpattern, 1).message = "<!--";
	g_array_index(st->matches, Tpattern, 1).starts_block = TRUE;
	g_array_index(st->matches, Tpattern, 1).selftag = comment;
	g_array_index(st->matches, Tpattern, 1).nextcontext = 1;
	
	/* <img */	
	g_array_index(st->table, Ttablerow, 1).row['i'] = 5;
	g_array_index(st->table, Ttablerow, 5).row['m'] = 6;
	g_array_index(st->table, Ttablerow, 6).row['g'] = 7;
	g_array_index(st->table, Ttablerow, 7).match = 2;
	g_array_index(st->matches, Tpattern, 2).message = "<img";
	g_array_index(st->matches, Tpattern, 2).selftag = braces;
	g_array_index(st->matches, Tpattern, 2).nextcontext = 2;

	/* <i */
	g_array_index(st->table, Ttablerow, 5).match = 3;
	g_array_index(st->matches, Tpattern, 3).message = "<i";
	g_array_index(st->matches, Tpattern, 3).selftag = braces;
	g_array_index(st->matches, Tpattern, 3).nextcontext = 3;

	/* context for comment --> */
	g_array_index(st->contexts, Tcontext, 1).startstate = 8;
	g_array_index(st->table, Ttablerow, 8).row['-'] = 9;
	g_array_index(st->table, Ttablerow, 9).row['-'] = 10;
	g_array_index(st->table, Ttablerow, 10).row['>'] = 11;
	g_array_index(st->table, Ttablerow, 11).match = 4;
	g_array_index(st->matches, Tpattern, 4).message = "-->";
	g_array_index(st->matches, Tpattern, 4).nextcontext = 0;
	g_array_index(st->matches, Tpattern, 4).ends_block = TRUE;
	g_array_index(st->matches, Tpattern, 4).blockstartpattern = 1;
	g_array_index(st->matches, Tpattern, 4).blocktag = comment;
	g_array_index(st->matches, Tpattern, 4).selftag = comment;

	/* closure > in context for <img */
	g_array_index(st->contexts, Tcontext, 2).startstate = 12;
	g_array_index(st->contexts, Tcontext, 2).ac = g_completion_new(NULL);
	{
		GList *list = NULL;
		list = g_list_prepend(list, "src=");
		list = g_list_prepend(list, "width=");
		list = g_list_prepend(list, "height=");
		g_completion_add_items(g_array_index(st->contexts, Tcontext, 2).ac, list);
		g_list_free(list);
	}
	g_array_index(st->table, Ttablerow, 12).row['>'] = 13;
	g_array_index(st->table, Ttablerow, 13).match = 5;
	g_array_index(st->matches, Tpattern, 5).message = ">";
	g_array_index(st->matches, Tpattern, 5).nextcontext = 0;
	g_array_index(st->matches, Tpattern, 5).selftag = braces;
	/* src= in context for <img */
	g_array_index(st->table, Ttablerow, 12).row['s'] = 14;
	g_array_index(st->table, Ttablerow, 14).row['r'] = 15;
	g_array_index(st->table, Ttablerow, 15).row['c'] = 16;
	g_array_index(st->table, Ttablerow, 16).row['='] = 17;
	g_array_index(st->table, Ttablerow, 16).row[' '] = 16;
	g_array_index(st->table, Ttablerow, 16).row['\t'] = 16;
	g_array_index(st->table, Ttablerow, 16).row['\n'] = 16;
	g_array_index(st->table, Ttablerow, 16).row['\r'] = 16;
	g_array_index(st->table, Ttablerow, 17).match = 6;
	g_array_index(st->matches, Tpattern, 6).message = ">";
	g_array_index(st->matches, Tpattern, 6).nextcontext = 2;
	g_array_index(st->matches, Tpattern, 6).selftag = storage;
	/* closure > in context for <i */
	g_array_index(st->contexts, Tcontext, 3).startstate = 18;
	g_array_index(st->table, Ttablerow, 18).row['>'] = 19;
	g_array_index(st->table, Ttablerow, 19).match = 7;
	g_array_index(st->matches, Tpattern, 7).message = ">";
	g_array_index(st->matches, Tpattern, 7).nextcontext = 0;
	g_array_index(st->matches, Tpattern, 7).selftag = braces;
	
	
#endif /* #ifdef HTMLSTYLEMATCHING */
	return st;
}
