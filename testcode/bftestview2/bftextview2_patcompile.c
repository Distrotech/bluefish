/* for the design docs see bftextview2.h */
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
	st->reference = g_hash_table_new(g_str_hash,g_str_equal);

	/* we don't build a automata from patterns right now, because I'm not
	very satisfied with my previous attempt to code that.. 
	we build one by hand just to test scanning.. */ 
	
	/* for testing we are going to scan for a block detected by {} and by ()
	and we do C comments, and we scan for the keyword void */
#define CSTYLEMATCHING
#ifdef CSTYLEMATCHING
	g_array_set_size(st->table,14);
	st->table->len = 14;
	g_array_set_size(st->contexts,2);
	st->matches->len = 2;
	g_array_set_size(st->matches,8);
	st->matches->len = 8; /* match 0 is not used */

	g_array_index(st->contexts, Tcontext, 0).startstate = 0;
	g_array_index(st->contexts, Tcontext, 0).ac = g_completion_new(NULL);
	{
		GList *list = g_list_prepend(NULL, "void"); 
		g_completion_add_items(g_array_index(st->contexts, Tcontext, 0).ac, list);
		g_list_free(list);
		g_hash_table_insert(st->reference,"void","A function without return value returns <b>void</b>. An argument list for a function taking no arguments is also <b>void</b>. The only variable that can be declared with type void is a pointer.");
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
/*#define HTMLSTYLEMATCHING*/
#ifdef HTMLSTYLEMATCHING
	g_array_set_size(st->table,39);
	st->table->len = 39;
	g_array_set_size(st->contexts,4);
	st->contexts->len = 4;
	g_array_set_size(st->matches,12);
	st->matches->len = 12; /* match 0 is not used */

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

	/* <html */
	g_array_index(st->table, Ttablerow, 1).row['h'] = 8;
	g_array_index(st->table, Ttablerow, 8).row['t'] = 9;
	g_array_index(st->table, Ttablerow, 9).row['m'] = 10;
	g_array_index(st->table, Ttablerow, 10).row['l'] = 11;
	g_array_index(st->table, Ttablerow, 11).match = 4;
	g_array_index(st->matches, Tpattern, 4).message = "<html";
	g_array_index(st->matches, Tpattern, 4).starts_block = TRUE;
	g_array_index(st->matches, Tpattern, 4).selftag = braces;
	
	/* </html> */
	g_array_index(st->table, Ttablerow, 1).row['/'] = 12;
	g_array_index(st->table, Ttablerow, 12).row['h'] = 13;
	g_array_index(st->table, Ttablerow, 13).row['t'] = 14;
	g_array_index(st->table, Ttablerow, 14).row['m'] = 15;
	g_array_index(st->table, Ttablerow, 15).row['l'] = 16;
	g_array_index(st->table, Ttablerow, 16).row['>'] = 17;
	g_array_index(st->table, Ttablerow, 17).match = 5;
	g_array_index(st->matches, Tpattern, 5).message = "</html>";
	g_array_index(st->matches, Tpattern, 5).ends_block = TRUE;
	g_array_index(st->matches, Tpattern, 5).selftag = braces;
	g_array_index(st->matches, Tpattern, 5).blockstartpattern = 4;
	
	/* <body */ 
	g_array_index(st->table, Ttablerow, 1).row['b'] = 18;
	g_array_index(st->table, Ttablerow, 18).row['o'] = 19;
	g_array_index(st->table, Ttablerow, 19).row['d'] = 20;
	g_array_index(st->table, Ttablerow, 20).row['y'] = 21;
	g_array_index(st->table, Ttablerow, 21).row['>'] = 22;
	g_array_index(st->table, Ttablerow, 22).match = 6;
	g_array_index(st->matches, Tpattern, 6).message = "<body";
	g_array_index(st->matches, Tpattern, 6).starts_block = TRUE;
	g_array_index(st->matches, Tpattern, 6).selftag = braces;

	/* </body> starts with a / on state 12 */
	g_array_index(st->table, Ttablerow, 12).row['b'] = 23;
	g_array_index(st->table, Ttablerow, 23).row['o'] = 24;
	g_array_index(st->table, Ttablerow, 24).row['d'] = 25;
	g_array_index(st->table, Ttablerow, 25).row['y'] = 26;
	g_array_index(st->table, Ttablerow, 26).row['>'] = 27;
	g_array_index(st->table, Ttablerow, 27).match = 7;
	g_array_index(st->matches, Tpattern, 7).message = "</body>";
	g_array_index(st->matches, Tpattern, 7).ends_block = TRUE;
	g_array_index(st->matches, Tpattern, 7).selftag = braces;
	g_array_index(st->matches, Tpattern, 7).blockstartpattern = 6;

	/* context for comment --> */
	g_array_index(st->contexts, Tcontext, 1).startstate = 28;
	g_array_index(st->table, Ttablerow, 28).row['-'] = 29;
	g_array_index(st->table, Ttablerow, 29).row['-'] = 30;
	g_array_index(st->table, Ttablerow, 30).row['>'] = 31;
	g_array_index(st->table, Ttablerow, 31).match = 8;
	g_array_index(st->matches, Tpattern, 8).message = "-->";
	g_array_index(st->matches, Tpattern, 8).nextcontext = 0;
	g_array_index(st->matches, Tpattern, 8).ends_block = TRUE;
	g_array_index(st->matches, Tpattern, 8).blockstartpattern = 1;
	g_array_index(st->matches, Tpattern, 8).blocktag = comment;
	g_array_index(st->matches, Tpattern, 8).selftag = comment;

	/* closure > in context for <img */
	g_array_index(st->contexts, Tcontext, 2).startstate = 32;
	g_array_index(st->contexts, Tcontext, 2).ac = g_completion_new(NULL);
	{
		GList *list = NULL;
		list = g_list_prepend(list, "src=");
		list = g_list_prepend(list, "width=");
		list = g_list_prepend(list, "height=");
		g_completion_add_items(g_array_index(st->contexts, Tcontext, 2).ac, list);
		g_list_free(list);
	}
	g_array_index(st->table, Ttablerow, 32).row['>'] = 33;
	g_array_index(st->table, Ttablerow, 33).match = 9;
	g_array_index(st->matches, Tpattern, 9).message = ">";
	g_array_index(st->matches, Tpattern, 9).nextcontext = 0;
	g_array_index(st->matches, Tpattern, 9).selftag = braces;
	/* src= in context for <img */
	g_array_index(st->table, Ttablerow, 32).row['s'] = 33;
	g_array_index(st->table, Ttablerow, 33).row['r'] = 34;
	g_array_index(st->table, Ttablerow, 34).row['c'] = 35;
	g_array_index(st->table, Ttablerow, 35).row['='] = 36;
	g_array_index(st->table, Ttablerow, 35).row[' '] = 35;
	g_array_index(st->table, Ttablerow, 35).row['\t'] = 35;
	g_array_index(st->table, Ttablerow, 35).row['\n'] = 35;
	g_array_index(st->table, Ttablerow, 35).row['\r'] = 35;
	g_array_index(st->table, Ttablerow, 36).match = 10;
	g_array_index(st->matches, Tpattern, 10).message = ">";
	g_array_index(st->matches, Tpattern, 10).nextcontext = 2;
	g_array_index(st->matches, Tpattern, 10).selftag = storage;
	/* closure > in context for <i */
	g_array_index(st->contexts, Tcontext, 3).startstate = 37;
	g_array_index(st->table, Ttablerow, 37).row['>'] = 38;
	g_array_index(st->table, Ttablerow, 38).match = 11;
	g_array_index(st->matches, Tpattern, 11).message = ">";
	g_array_index(st->matches, Tpattern, 11).nextcontext = 0;
	g_array_index(st->matches, Tpattern, 11).selftag = braces;
	
	
#endif /* #ifdef HTMLSTYLEMATCHING */
	return st;
}
