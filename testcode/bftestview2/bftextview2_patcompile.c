/* for the design docs see bftextview2.h */
#include <string.h>
#include "bftextview2_patcompile.h"

static guint new_context(Tscantable *st) {
	guint num;
	
	num = st->contexts->len;
	st->contexts->len++;
	g_array_set_size(st->contexts,st->contexts->len+1);
	g_array_index(st->contexts, Tcontext, num).startstate = st->table->len;

	st->table->len++;
	g_array_set_size(st->table,st->table->len);
	
	g_print("new context %d has startstate %d\n",num, g_array_index(st->contexts, Tcontext, num).startstate);	
	return num;
}

static guint new_match(Tscantable *st, gchar *keyword, GtkTextTag *selftag, guint context, guint nextcontext
				, gboolean starts_block, gboolean ends_block, guint blockstartpattern
				, GtkTextTag *blocktag,gboolean add_to_ac, gchar *reference) {
	guint matchnum;
/* add the match */
	
	matchnum = st->matches->len;
	DBG_PATCOMPILE("new match %s with matchnum %d\n",keyword,matchnum);
	st->matches->len++;
	g_array_set_size(st->matches,st->matches->len+1);

	g_array_index(st->matches, Tpattern, matchnum).message = g_strdup(keyword);
	g_array_index(st->matches, Tpattern, matchnum).ends_block = ends_block;
	g_array_index(st->matches, Tpattern, matchnum).starts_block = starts_block;
	g_array_index(st->matches, Tpattern, matchnum).blockstartpattern = blockstartpattern;
	g_array_index(st->matches, Tpattern, matchnum).selftag = selftag;
	g_array_index(st->matches, Tpattern, matchnum).blocktag = blocktag;
	g_array_index(st->matches, Tpattern, matchnum).nextcontext = nextcontext;
	
		/* do autocompletion and reference */
	if (add_to_ac) {
		GList *list;
		if (!g_array_index(st->contexts, Tcontext, context).ac) {
			DBG_PATCOMPILE("create g_completion for context %d\n",context);
			g_array_index(st->contexts, Tcontext, context).ac = g_completion_new(NULL);
		}
		list = g_list_prepend(NULL, keyword);
		g_completion_add_items(g_array_index(st->contexts, Tcontext, context).ac, list);
		g_list_free(list);
		
		if (reference) {
			if (!g_array_index(st->contexts, Tcontext, context).reference) {
				DBG_PATCOMPILE("create hashtable for context %d\n",context);
				g_array_index(st->contexts, Tcontext, context).reference = g_hash_table_new(g_str_hash,g_str_equal);
			}
			g_hash_table_insert(g_array_index(st->contexts, Tcontext, context).reference,keyword,reference);
		}
	}
	
	return matchnum;
}
/* this function cannot do any regex style patterns 
just full keywords */
static void compile_keyword_to_DFA(Tscantable *st, gchar *keyword, guint matchnum, guint context) {

	gint i,len,pos=0;

	/* compile the keyword into the DFA */
	pos = g_array_index(st->contexts, Tcontext, context).startstate;
	len = strlen(keyword);
	for (i=0;i<=len;i++) {
		int c = keyword[i];
		if (c == '\0') {
			g_array_index(st->table, Ttablerow, pos).match = matchnum;
		} else {
			if (g_array_index(st->table, Ttablerow, pos).row[c] != 0) {
				pos = g_array_index(st->table, Ttablerow, pos).row[c];
			} else {
				pos = g_array_index(st->table, Ttablerow, pos).row[c] = st->table->len;
				st->table->len++;
				g_array_set_size(st->table,st->table->len+1);
			}
		}
	}
}

guint add_keyword_to_scanning_table(Tscantable *st, gchar *keyword, GtkTextTag *selftag, guint context, guint nextcontext
				, gboolean starts_block, gboolean ends_block, guint blockstartpattern
				, GtkTextTag *blocktag,gboolean add_to_ac, gchar *reference)  {
	guint matchnum;
	
	matchnum = new_match(st, keyword, selftag, context, nextcontext, starts_block, ends_block, blockstartpattern
				, blocktag,add_to_ac, reference);
	compile_keyword_to_DFA(st, keyword, matchnum, context);
	return matchnum;
}

Tscantable *bftextview2_scantable_new(GtkTextBuffer *buffer) {
	Tscantable *st;
	gint i,context1,context2,match1;
	GtkTextTag *braces, *comment, *storage, *keyword, *string, *variable, *function;
	
	braces = gtk_text_buffer_create_tag(buffer,"braces","weight", PANGO_WEIGHT_BOLD,"foreground","darkblue",NULL);
	comment = gtk_text_buffer_create_tag(buffer,"comment","style", PANGO_STYLE_ITALIC,"foreground", "grey", NULL);
	storage = gtk_text_buffer_create_tag(buffer,"storage","weight", PANGO_WEIGHT_BOLD,"foreground", "darkred", NULL);
	keyword = gtk_text_buffer_create_tag(buffer,"keyword","weight", PANGO_WEIGHT_BOLD,"foreground", "black", NULL);
	string = gtk_text_buffer_create_tag(buffer,"string","foreground", "darkgreen", NULL);
	variable = gtk_text_buffer_create_tag(buffer,"variable","foreground", "blue", NULL);
	function = gtk_text_buffer_create_tag(buffer,"function","foreground", "purple", NULL);

	st = g_slice_new0(Tscantable);
	st->table = g_array_sized_new(TRUE,TRUE,sizeof(Ttablerow), 160);
	st->contexts = g_array_sized_new(TRUE,TRUE,sizeof(Tcontext), 3);
	st->matches = g_array_sized_new(TRUE,TRUE,sizeof(Tpattern), 10);
	st->matches->len = 1; /* match 0 eans no match */
	
/*#define DFA_COMPILING*/ 
#ifdef DFA_COMPILING
	context1 = new_context(st);
	add_keyword_to_scanning_table(st, "void", storage, context1, context1
				, FALSE, FALSE, 0, NULL,TRUE
				, "A function without return value returns <b>void</b>. An argument list for a function taking no arguments is also <b>void</b>. The only variable that can be declared with type void is a pointer.");
	add_keyword_to_scanning_table(st, "int", storage, context1, context1
				, FALSE, FALSE, 0, NULL,TRUE
				, "Integer bla bla");
	add_keyword_to_scanning_table(st, "char", storage, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "storage type bla bla");
	add_keyword_to_scanning_table(st, "gchar", storage, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "storage type bla bla");
	add_keyword_to_scanning_table(st, "gint", storage, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "storage type bla bla");
	add_keyword_to_scanning_table(st, "guint", storage, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "storage type bla bla");
	add_keyword_to_scanning_table(st, "gpointer", storage, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "storage type bla bla");
	add_keyword_to_scanning_table(st, "gboolean", storage, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "storage type bla bla");
	add_keyword_to_scanning_table(st, "GList", storage, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "storage type bla bla");
	add_keyword_to_scanning_table(st, "GFile", storage, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "storage type bla bla");
	add_keyword_to_scanning_table(st, "GtkWidget", storage, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "storage type bla bla");
	add_keyword_to_scanning_table(st, "GtkTextView", storage, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "storage type bla bla");
	add_keyword_to_scanning_table(st, "GtkTextBuffer", storage, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "storage type bla bla");
	add_keyword_to_scanning_table(st, "GtkLabel", storage, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "storage type bla bla");
	add_keyword_to_scanning_table(st, "GtkTextIter", storage, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "storage type bla bla");
	add_keyword_to_scanning_table(st, "GtkTextMark", storage, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "storage type bla bla");
	add_keyword_to_scanning_table(st, "GtkTreeStore", storage, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "storage type bla bla");
	add_keyword_to_scanning_table(st, "GtkListStore", storage, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "storage type bla bla");
	add_keyword_to_scanning_table(st, "GtkTreeModel", storage, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "storage type bla bla");
	
	add_keyword_to_scanning_table(st, "NULL", variable, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "empty pointer value bla bla");
	
	add_keyword_to_scanning_table(st, "if", keyword, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "if bla");
	add_keyword_to_scanning_table(st, "else", keyword, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "else bla bla");
	add_keyword_to_scanning_table(st, "return", keyword, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "return bla bla");
	add_keyword_to_scanning_table(st, "static", keyword, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "static bla bla");
	add_keyword_to_scanning_table(st, "const", keyword, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "const bla bla");
	add_keyword_to_scanning_table(st, "sizeof", keyword, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "sizeof bla bla");
	add_keyword_to_scanning_table(st, "switch", keyword, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "switch bla bla");
	add_keyword_to_scanning_table(st, "case", keyword, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "case bla bla");
	add_keyword_to_scanning_table(st, "break", keyword, context1, context1, FALSE, FALSE, 0, NULL,TRUE, "break bla bla");
	match1 = add_keyword_to_scanning_table(st, "{", braces, context1, context1, TRUE, FALSE, 0, NULL,FALSE,NULL);
	add_keyword_to_scanning_table(st, "}", braces, context1, context1, FALSE, TRUE, match1, NULL,FALSE,NULL);
	match1 = add_keyword_to_scanning_table(st, "(", braces, context1, context1, TRUE, FALSE, 0, NULL,FALSE,NULL);
	add_keyword_to_scanning_table(st, ")", braces, context1, context1, FALSE, TRUE, match1, NULL,FALSE,NULL);
	match1 = add_keyword_to_scanning_table(st, "[", braces, context1, context1, TRUE, FALSE, 0, NULL,FALSE,NULL);
	add_keyword_to_scanning_table(st, "]", braces, context1, context1, FALSE, TRUE, match1, NULL,FALSE,NULL);
	context2 = new_context(st);
	match1 = add_keyword_to_scanning_table(st, "\"", string, context1, context2, TRUE, FALSE, 0, NULL,FALSE,NULL);
	add_keyword_to_scanning_table(st, "\\\"", string, context2, context2, FALSE, FALSE, 0, string,FALSE,NULL);
	add_keyword_to_scanning_table(st, "\"", string, context2, context1, FALSE, TRUE, match1, string,FALSE,NULL);
	context2 = new_context(st);
	match1 = add_keyword_to_scanning_table(st, "/*", comment, context1, context2, TRUE, FALSE, 0, NULL,FALSE,NULL);
	add_keyword_to_scanning_table(st, "*/", comment, context2, context1, FALSE, TRUE, match1, comment,FALSE,NULL);

	g_print("we have %d states in %d contexts for %d matches\n",st->table->len,st->contexts->len,st->matches->len);
#endif

#define PHP_COMPILING
#ifdef PHP_COMPILING
	{
		guint context0, contexttag, contextphp, contextstring;
		guint match;
		context0= new_context(st);

		contexttag = new_context(st);
		match = add_keyword_to_scanning_table(st, "<html", storage, context0, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, ">", storage, contexttag, context0, FALSE, FALSE, 0, NULL,FALSE,NULL);
		
		contexttag = new_context(st);
		match = add_keyword_to_scanning_table(st, "<body", storage, context0, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, ">", storage, contexttag, context0, FALSE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "onLoad", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "style", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "class", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		contextstring = new_context(st);
		add_keyword_to_scanning_table(st, "\"", string, contexttag, contextstring, FALSE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "\"", string, contextstring, contexttag, FALSE, FALSE, 0, string,FALSE,NULL);
		
		contexttag = new_context(st);
		match = add_keyword_to_scanning_table(st, "<p", storage, context0, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, ">", storage, contexttag, context0, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "style", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "id", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "class", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		contextstring = new_context(st);
		add_keyword_to_scanning_table(st, "\"", string, contexttag, contextstring, FALSE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "\"", string, contextstring, contexttag, FALSE, FALSE, 0, string,FALSE,NULL);
		
		contexttag = new_context(st);
		match = add_keyword_to_scanning_table(st, "<b", storage, context0, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, ">", storage, contexttag, context0, FALSE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "style", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "id", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "class", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		contextstring = new_context(st);
		add_keyword_to_scanning_table(st, "\"", string, contexttag, contextstring, FALSE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "\"", string, contextstring, contexttag, FALSE, FALSE, 0, string,FALSE,NULL);

		
		contexttag = new_context(st);
		match = add_keyword_to_scanning_table(st, "<i", storage, context0, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, ">", storage, contexttag, context0, FALSE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "style", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "id", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "class", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		contextstring = new_context(st);
		add_keyword_to_scanning_table(st, "\"", string, contexttag, contextstring, FALSE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "\"", string, contextstring, contexttag, FALSE, FALSE, 0, string,FALSE,NULL);
		
		contexttag = new_context(st);
		match = add_keyword_to_scanning_table(st, "<img", storage, context0, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, ">", storage, contexttag, context0, FALSE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "style", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "id", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "src", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "width", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "height", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "alt", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "class", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		contextstring = new_context(st);
		add_keyword_to_scanning_table(st, "\"", string, contexttag, contextstring, FALSE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "\"", string, contextstring, contexttag, FALSE, FALSE, 0, string,FALSE,NULL);
		
		contexttag = new_context(st);
		match = add_keyword_to_scanning_table(st, "<div", storage, context0, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, ">", storage, contexttag, context0, FALSE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "style", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "id", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "class", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		contextstring = new_context(st);
		add_keyword_to_scanning_table(st, "\"", string, contexttag, contextstring, FALSE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "\"", string, contextstring, contexttag, FALSE, FALSE, 0, string,FALSE,NULL);
		
		contexttag = new_context(st);
		match = add_keyword_to_scanning_table(st, "<script", storage, context0, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, ">", storage, contexttag, context0, FALSE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "style", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "id", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "class", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "src", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "href", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		contextstring = new_context(st);
		add_keyword_to_scanning_table(st, "\"", string, contexttag, contextstring, FALSE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "\"", string, contextstring, contexttag, FALSE, FALSE, 0, string,FALSE,NULL);

		contexttag = new_context(st);
		match = add_keyword_to_scanning_table(st, "<a", storage, context0, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, ">", storage, contexttag, context0, FALSE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "style", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "id", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "class", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "href", keyword, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,NULL);
		contextstring = new_context(st);
		add_keyword_to_scanning_table(st, "\"", string, contexttag, contextstring, FALSE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "\"", string, contextstring, contexttag, FALSE, FALSE, 0, string,FALSE,NULL);

		contexttag = new_context(st);
		match = add_keyword_to_scanning_table(st, "<!--", comment, context0, contexttag, TRUE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "-->", comment, contexttag, context0, FALSE, TRUE, match, comment,FALSE,NULL);

		contextphp = new_context(st);
		match = add_keyword_to_scanning_table(st, "<?php", variable, context0, contextphp, TRUE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "?>", variable, contextphp, context0, FALSE, TRUE, match, NULL,FALSE,NULL);

		add_keyword_to_scanning_table(st, "if", keyword, contextphp, contextphp, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "return", keyword, contextphp, contextphp, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "function", keyword, contextphp, contextphp, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "else", keyword, contextphp, contextphp, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "case", keyword, contextphp, contextphp, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "switch", keyword, contextphp, contextphp, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "break", keyword, contextphp, contextphp, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "echo", keyword, contextphp, contextphp, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "class", keyword, contextphp, contextphp, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "var", keyword, contextphp, contextphp, FALSE, FALSE, 0, NULL,TRUE,NULL);

		contextstring = new_context(st);
		add_keyword_to_scanning_table(st, "\"", string, contextphp, contextstring, FALSE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "\"", string, contextstring, contextphp, FALSE, FALSE, 0, string,FALSE,NULL);
		add_keyword_to_scanning_table(st, "\\\"", string, contextstring, contextstring, FALSE, FALSE, 0, NULL,FALSE,NULL);

		contextstring = new_context(st);
		match = add_keyword_to_scanning_table(st, "/*", comment, contextphp, contextstring, TRUE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "*/", comment, contextstring, contextphp, FALSE, TRUE, match, comment,FALSE,NULL);

		contextstring = new_context(st);
		add_keyword_to_scanning_table(st, "'", string, contextphp, contextstring, FALSE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "'", string, contextstring, contextphp, FALSE, FALSE, 0, string,FALSE,NULL);
		add_keyword_to_scanning_table(st, "\\'", string, contextstring, contextstring, FALSE, FALSE, 0, NULL,FALSE,NULL);
		
		match = add_keyword_to_scanning_table(st, "{", keyword, contextphp, contextphp, TRUE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "}", keyword, contextphp, contextphp, FALSE, TRUE, match, NULL,FALSE,NULL);
		match = add_keyword_to_scanning_table(st, "[", keyword, contextphp, contextphp, TRUE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, "]", keyword, contextphp, contextphp, FALSE, TRUE, match, NULL,FALSE,NULL);
		match = add_keyword_to_scanning_table(st, "(", keyword, contextphp, contextphp, TRUE, FALSE, 0, NULL,FALSE,NULL);
		add_keyword_to_scanning_table(st, ")", keyword, contextphp, contextphp, FALSE, TRUE, match, NULL,FALSE,NULL);
		
		add_keyword_to_scanning_table(st, "mysql_connect", function, contextphp, contextphp, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "mysql_query", function, contextphp, contextphp, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "mysql_num_rows", function, contextphp, contextphp, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "mysql_fetch_row", function, contextphp, contextphp, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "mysql_fetch_array", function, contextphp, contextphp, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "mysql_destroy", function, contextphp, contextphp, FALSE, FALSE, 0, NULL,TRUE,NULL);
		add_keyword_to_scanning_table(st, "mysql_close", function, contextphp, contextphp, FALSE, FALSE, 0, NULL,TRUE,NULL);
		
	}
#endif
	/* we don't build a automata from regex patterns right now, because I'm not
	very satisfied with my previous attempt to code that.. 
	we build one by hand just to test scanning.. */ 
	
	/* for testing we are going to scan for a block detected by {} and by ()
	and we do C comments, and we scan for the keyword void */
/*#define CSTYLEMATCHING*/
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
		g_array_index(st->contexts, Tcontext, 0).reference = g_hash_table_new(g_str_hash,g_str_equal);
		g_hash_table_insert(g_array_index(st->contexts, Tcontext, 0).reference,"void","A function without return value returns <b>void</b>. An argument list for a function taking no arguments is also <b>void</b>. The only variable that can be declared with type void is a pointer.");
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
/* #define PHPSTYLEMATCHING not finished !!! */
#ifdef PHPSTYLEMATCHING
	g_array_index(st->table, Ttablerow, 0).row['<'] = 1;
	
	/* <?php */
	g_array_index(st->table, Ttablerow, 1).row['?'] = 2;
	g_array_index(st->table, Ttablerow, 2).row['p'] = 3;
	g_array_index(st->table, Ttablerow, 3).row['h'] = 4;
	g_array_index(st->table, Ttablerow, 4).row['p'] = 5;
	g_array_index(st->table, Ttablerow, 5).match = 1;
	g_array_index(st->matches, Tpattern, 1).message = "<?php";
	g_array_index(st->matches, Tpattern, 1).nextcontext = 1;
	g_array_index(st->matches, Tpattern, 1).selftag = braces;
	g_array_index(st->matches, Tpattern, 1).starts_block = TRUE;
	/* <p */
	g_array_index(st->table, Ttablerow, 1).row['p'] = 6;
	g_array_index(st->table, Ttablerow, 6).match = 2;
	g_array_index(st->matches, Tpattern, 2).message = "<p";
	g_array_index(st->matches, Tpattern, 2).nextcontext = 2;
	g_array_index(st->matches, Tpattern, 2).selftag = storage;
	/* <!-- */
	g_array_index(st->table, Ttablerow, 1).row['!'] = 7;
	g_array_index(st->table, Ttablerow, 7).row['-'] = 8;
	g_array_index(st->table, Ttablerow, 8).row['-'] = 9;
	g_array_index(st->table, Ttablerow, 9).match = 3;
	g_array_index(st->matches, Tpattern, 3).message = "<!--";
	g_array_index(st->matches, Tpattern, 3).nextcontext = 3;
	g_array_index(st->matches, Tpattern, 3).selftag = comment;
	g_array_index(st->matches, Tpattern, 3).starts_block = TRUE;
	/* php context, ?> */
	g_array_index(st->contexts, Tcontext, 1).startstate = 10;
	g_array_index(st->table, Ttablerow, 10).row['?'] = 11;
	g_array_index(st->table, Ttablerow, 11).row['>'] = 12;
	g_array_index(st->table, Ttablerow, 12).match = 4;
	g_array_index(st->matches, Tpattern, 4).message = "?>";
	g_array_index(st->matches, Tpattern, 4).nextcontext = 1;
	g_array_index(st->matches, Tpattern, 4).selftag = braces;
	g_array_index(st->matches, Tpattern, 6).ends_block = TRUE;
	g_array_index(st->matches, Tpattern, 6).blockstartpattern = 1;
	/* php context, { */
	g_array_index(st->table, Ttablerow, 10).row['{'] = 13;
	g_array_index(st->table, Ttablerow, 13).match = 5;
	g_array_index(st->matches, Tpattern, 5).message = "{";
	g_array_index(st->matches, Tpattern, 5).nextcontext = 1;
	g_array_index(st->matches, Tpattern, 5).selftag = braces;
	g_array_index(st->matches, Tpattern, 5).starts_block = TRUE;
	/* php context, } */
	g_array_index(st->table, Ttablerow, 10).row['}'] = 14;
	g_array_index(st->table, Ttablerow, 14).match = 6;
	g_array_index(st->matches, Tpattern, 6).message = "}";
	g_array_index(st->matches, Tpattern, 6).nextcontext = 1;
	g_array_index(st->matches, Tpattern, 6).selftag = braces;
	g_array_index(st->matches, Tpattern, 6).ends_block = TRUE;
	g_array_index(st->matches, Tpattern, 6).blockstartpattern = 5;

	/* php context, ( */
	g_array_index(st->table, Ttablerow, 10).row['('] = 15;
	g_array_index(st->table, Ttablerow, 15).match = 7;
	g_array_index(st->matches, Tpattern, 7).message = "(";
	g_array_index(st->matches, Tpattern, 7).nextcontext = 1;
	g_array_index(st->matches, Tpattern, 7).selftag = braces;
	g_array_index(st->matches, Tpattern, 7).starts_block = TRUE;
	/* php context, ) */
	g_array_index(st->table, Ttablerow, 10).row[')'] = 16;
	g_array_index(st->table, Ttablerow, 16).match = 8;
	g_array_index(st->matches, Tpattern, 8).message = ")";
	g_array_index(st->matches, Tpattern, 8).nextcontext = 1;
	g_array_index(st->matches, Tpattern, 8).selftag = braces;
	g_array_index(st->matches, Tpattern, 8).ends_block = TRUE;
	g_array_index(st->matches, Tpattern, 8).blockstartpattern = 7;

	/* php context, [ */
	g_array_index(st->table, Ttablerow, 10).row['['] = 17;
	g_array_index(st->table, Ttablerow, 17).match = 9;
	g_array_index(st->matches, Tpattern, 9).message = "[";
	g_array_index(st->matches, Tpattern, 9).nextcontext = 1;
	g_array_index(st->matches, Tpattern, 9).selftag = braces;
	g_array_index(st->matches, Tpattern, 9).starts_block = TRUE;
	/* php context, ] */
	g_array_index(st->table, Ttablerow, 10).row[']'] = 18;
	g_array_index(st->table, Ttablerow, 18).match = 10;
	g_array_index(st->matches, Tpattern, 10).message = "]";
	g_array_index(st->matches, Tpattern, 10).nextcontext = 1;
	g_array_index(st->matches, Tpattern, 10).selftag = braces;
	g_array_index(st->matches, Tpattern, 8).ends_block = TRUE;
	g_array_index(st->matches, Tpattern, 8).blockstartpattern = 9;

	/* php context $[a-zA-Z]+ */
	g_array_index(st->table, Ttablerow, 10).row['$'] = 19;
	for (i='a';i<='z';i++) {
		g_array_index(st->table, Ttablerow, 19).row[i] = 20;
		g_array_index(st->table, Ttablerow, 20).row[i] = 20;
	}
	for (i='A';i<='Z';i++) {
		g_array_index(st->table, Ttablerow, 19).row[i] = 20;
		g_array_index(st->table, Ttablerow, 20).row[i] = 20;
	}
	g_array_index(st->table, Ttablerow, 20).match = 11;
	g_array_index(st->matches, Tpattern, 10).message = "$[a-z]+";
	g_array_index(st->matches, Tpattern, 10).nextcontext = 1;
	g_array_index(st->matches, Tpattern, 10).selftag = storage;
	
	/* php context C comment */
	g_array_index(st->table, Ttablerow, 10).row['/'] = 21;
	g_array_index(st->table, Ttablerow, 21).row['*'] = 22;
	g_array_index(st->table, Ttablerow, 22).match = 11;
	g_array_index(st->matches, Tpattern, 11).message = "/*";
	g_array_index(st->matches, Tpattern, 11).nextcontext = 4;
	g_array_index(st->matches, Tpattern, 11).selftag = comment;
	g_array_index(st->matches, Tpattern, 11).starts_block = TRUE;
	/* php context C++ or shell comment (//|#)[^\n]*\n */
	g_array_index(st->table, Ttablerow, 10).row['/'] = 23;
	g_array_index(st->table, Ttablerow, 10).row['#'] = 24;
	g_array_index(st->table, Ttablerow, 23).row['/'] = 24;
	for (i=0;i<127;i++) {
		g_array_index(st->table, Ttablerow, 24).row[i] = 24;
	}
	g_array_index(st->table, Ttablerow, 24).row['\n'] = 25;
	g_array_index(st->table, Ttablerow, 25).match = 12;
	g_array_index(st->matches, Tpattern, 12).message = " (//|#)[^\\n]*\\n";
	g_array_index(st->matches, Tpattern, 12).nextcontext = 1;
	g_array_index(st->matches, Tpattern, 12).selftag = comment;
	
	/* <p end > */
	g_array_index(st->contexts, Tcontext, 2).startstate = 26;
	g_array_index(st->table, Ttablerow, 26).row['>'] = 27;
	g_array_index(st->table, Ttablerow, 27).match = 13;
	g_array_index(st->matches, Tpattern, 13).message = ">";
	g_array_index(st->matches, Tpattern, 13).nextcontext = 0;
	g_array_index(st->matches, Tpattern, 13).selftag = storage;
	
	/* <!-- end --> */
	g_array_index(st->contexts, Tcontext, 3).startstate = 28;
	g_array_index(st->table, Ttablerow, 28).row['-'] = 29;
	g_array_index(st->table, Ttablerow, 29).row['-'] = 30;
	g_array_index(st->table, Ttablerow, 30).row['>'] = 31;
	g_array_index(st->table, Ttablerow, 31).match = 14;
	g_array_index(st->matches, Tpattern, 14).message = "-->";
	g_array_index(st->matches, Tpattern, 14).nextcontext = 0;
	g_array_index(st->matches, Tpattern, 14).selftag = comment;
	g_array_index(st->matches, Tpattern, 14).ends_block = TRUE;
	g_array_index(st->matches, Tpattern, 14).blockstartpattern = 3;
	/* php, C style comment end * / */
	g_array_index(st->contexts, Tcontext, 4).startstate = 32;
	g_array_index(st->table, Ttablerow, 32).row['*'] = 33;
	g_array_index(st->table, Ttablerow, 33).row['/'] = 34;
	g_array_index(st->table, Ttablerow, 34).match = 15;
	g_array_index(st->matches, Tpattern, 15).message = "*/";
	g_array_index(st->matches, Tpattern, 15).nextcontext = 1;
	g_array_index(st->matches, Tpattern, 15).selftag = comment;
	g_array_index(st->matches, Tpattern, 15).blocktag = comment;
	g_array_index(st->matches, Tpattern, 15).ends_block = TRUE;
	g_array_index(st->matches, Tpattern, 15).blockstartpattern = 11;
	
#endif /* PHPSTYLEMATCHING */
	return st;
}
