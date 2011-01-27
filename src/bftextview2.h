/* Bluefish HTML Editor
 * bftextview2.h
 *
 * Copyright (C) 2008,2009,2010,2011 Olivier Sessink
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

/*
BFTEXTVIEW2 DESIGN DOCS

requirements: a textwidget with
- syntax highlighting
- block folding
- context-sensitive autocompletion
- for various languages
- fast (doesn't lock the GUI when syntaxt-scanning very large files)

DESIGN:
- to avoid locking the GUI, it should be able to scan in multiple runs
  - it should be able to mark a region as 'needs scanning'
  - it should be able to resume scanning on any point marked as 'needs scanning'

- scanning is done with patterns that scan within a context
 - for e.g. php we start with an html context that scans for html tags and it scans
   for the php-open tag <?php
  - within a html tag we scan the html-attribute context and we scan for the end-of-tag '>'
  - withing <?php we scan for functions/variables and for end-of-php '?>'
- autocompletion is also done based on the context. so for autocompletion we look-up
  the context, and based on the context we know the possibilities
  - for e.g. scanning within the '<img' html tag context we know that 'src' is one of
    the valid attributes
- the context is kept on a stack. once the end-of-context pattern is found (a pattern that has
nextcontext -1), we revert to the previous context

- changed areas are marked with a GtkTextTag called 'needscanning'.
 - a idle function is started (if not running) to do the actual scanning
 - the scanner keeps a timer and stops scanning once a certain time has passed
 - if the scanning is finished the idle function stops

- to find where to resume scanning it simply searches for the first position
  that is marked with this tag and backs-up to the beginning of the line
 - to know which patterns to use we have to know in which context we are. we therefore keep
   a cache of the (context)stack. on each position where to contextstack changes, we make a copy
   of the current stack that we keep in a sorted balanced tree (a GSequence), sorted by the
   character offset in the text.
   - the positions change when text is inserted or deleted, but never their order. a GSequence
     allows us to update the offsets for the stacks without re-sorting the entire tree
 - same holds for the blocks. we keep a blockstack, and we keep a cache of the blockstack in the
   same foundcache as where we keep the contextstack.

- to paint the margin and detect if we can expand/collapse blocks, we can use this same
  scancache. Along with walking the lines we walk the GSequence and see if there are new
  blocks that can be folded.

- the current scanning is based on Deterministic Finite Automata (DFA) just like the 1.1.6
unstable engine (see wikipedia for more info). The 1.1.6 engine alloc's each state in a
separate memory block. This new engine alloc's a large array for all states at once, so you can
simply move trough the array instead of following pointers. Following the DFA is then as simple
as state = table[state][character]; where state is just an integer position in the array, and
character is the current character you're scanning. I hope the array will help to speed up
the scanner. I used guint16 because I suspect that we never hit the 65500 states (largest
patterns set right now is php, 4500 functions use 32000 states)

- DFA table's for multiple contexts are all in the same memory block. Each context has an offset
where it starts. When a match is found, scanning can move to a different context. For example
when <?php is found, we switch to row 123 of the DFA table from which all php functions are scanned.

- Compared to the engine in the 1.0 series the main advantage is that we do only a single scanning
run for all patterns in a given context. The 1.0 scanner does multiple scanning runs for <\?php
and for <[a-z]+>. The new engine scans (<\?php|<[a-z]+>) but knows that both sub-patterns lead
to different results (different color, different context).

========== language parsing from the XML file ==========
- the languages are defined in an XML file. On startup, only the header of that file is parsed,
into a Tbflang struct, which defines the language and the mime types. Only when scanning for
one of these mime-types is requested the rest of the file is parsed (in a separate thread!!!)
and the DFA for this language is created. This saves memory and startup time for languages
that are not used in a certain session.

For parsing we use the libxml2 textreader interface
http://xmlsoft.org/xmlreader.html
It does not need to load the full XML file into memory, it
is a parser that moves through the file while parsing. This
makes it an excellent choice to quickly parse large language
files in order to build the DFA table.

A tag, keyword or patterns may have a class="foo" attribute. This pattern is only
added to the DFA when option "foo" is enabled. This way we can have gtk functions
in the C patterns for those of us that do GTK programming in Bluefish. All
others will have a much smaller DFA table for the C language.

========== Symbols and identifiers in the DFA table ==========
Each context has symbols. Symbols are characters that may start or end a pattern.
Try to highlight for example:
 char *rc_char(char*chara);
 ^^^^          ^^^^
Only two of the four 'char' need to be highlighted. How does the scanner know which
one to highlight? In the above example there are several symbols such as whitespace
, brackets and operators:
 char *rc_char(char*chara);
^    ^^       ^    ^     ^^
see that the occurences of 'char' that should be highlighted are all in between symbols?!

The DFA table has a startstate for each context and an identifier-state (identstate).
In the next example state 0 is the startstate and state 1 the identstate:

|state|| space| a | c | h | r | * | ( | ) | _ | have match ?
|  0  ||   0  | 1 | 2 | 1 | 1 | 0 | 0 | 0 | 1 | no
|  1  ||   0  | 1 | 1 | 1 | 1 | 0 | 0 | 0 | 1 | no
|  2  ||   0  | 1 | 1 | 3 | 1 | 0 | 0 | 0 | 1 | no
|  3  ||   0  | 4 | 1 | 1 | 1 | 0 | 0 | 0 | 1 | no
|  4  ||   0  | 1 | 1 | 1 | 5 | 0 | 0 | 0 | 1 | no
|  5  ||   0  | 1 | 1 | 1 | 1 | 0 | 0 | 0 | 1 | yes! we have: char

now try to parse the bit '(char*' starting at state 0 (the startstate)
( -> state 0 if we find state 0 we check if there is a match.. no
c -> state 2
h -> state 3
a -> state 4
r -> state 5
* -> state 0 if we find state 0 we check if there is a match.. YES!

now try to scan the bit '*rc_char('
* -> state 0 if we find state 0 we check if there is a match.. no
r -> state 1
c -> state 1
_ -> state 1
c -> state 1
h -> state 1
a -> state 1
r -> state 1
( -> state 0 if we find state 0 we check if there is a match.. no

now try to scan the bit ' chara)':
  -> state 0 if we find state 0 we check if there is a match.. no
c -> state 2
h -> state 3
a -> state 4
r -> state 5
a -> state 1
) -> state 0 if we find state 0 we check if there is a match.. no

as you see, the scanner is stuck in state 1 (the identstate) if
either on the start or on the end there is no symbol.

======== Autocompleting patterns =============
for autocompletion we keep a GCompletion in each context. This is filled with 
all the patterns during XML load.

we use a similar scanning engine as above that can tell us where the string that 
the user is typing started, and in which context the curor position is. Once
we know the context we know which GCompletion structure to use, so we can get 
a list of possible completion strings.

======= Storing found function names and such for jump and autocompletion ======

for jump: found functions names are stored in a hashtable 
bfwin->identifier_jump as
key Tbflang-context-name -> value Tdocument-linenumber

for autocompletion they are added to a GCompletion
the GCompletion can be found in hashtable
bfwin->identifier_ac with 
key Tbflang-context -> value GCompletion

======= Split view / slave widget =======

a slave widget is a widget that does not do scanning itself, it doesn't 
have a scanning table, it doesn't have any settings. It relies for all
of these things on the master widget. On a master widget btv->master will
point to itself, on a slave widget btv->master will point to the master.

The slave widget should always be destroyed before the master. 
*/

#ifndef _BFTEXTVIEW2_H_
#define _BFTEXTVIEW2_H_

#include <gtk/gtk.h>
#include "config.h"

#if defined(__GNUC__) || (defined(__SUNPRO_C) && __SUNPRO_C > 0x580)
#define DBG_NONE(args...)
 /**/
#else                                                   /* notdef __GNUC__ || __SUNPRO_C */
extern void g_none(char * first, ...);
#define DBG_NONE g_none
#endif  
 /**/

#define IDENTSTORING

#define BF2_OFFSET_UNDEFINED G_MAXUINT32

#define DBG_MSG DBG_NONE
#define DBG_SCANCACHE DBG_NONE
#define DBG_SCANNING DBG_NONE
#define DBG_BLOCKMATCH DBG_NONE
#define DBG_FCONTEXTREFCOUNT DBG_NONE
#define DBG_FBLOCKREFCOUNT DBG_NONE
#define DBG_PATCOMPILE DBG_NONE
#define DBG_SIGNALS DBG_NONE
#define DBG_AUTOCOMP DBG_NONE
#define DBG_DELAYSCANNING DBG_NONE
#define DBG_FOLD DBG_NONE
#define DBG_MARGIN DBG_NONE
#define DBG_PARSING DBG_NONE
#define DBG_TOOLTIP DBG_NONE
#define DBG_SPELL DBG_NONE
#define DBG_IDENTIFIER DBG_NONE

#define NUMSCANCHARS 127 /* 128 is ascii, but the last character is never scanned (DEL)
		and the Ttablerow has one more 16bit value. By setting this to 127 instead of 128
		we dont need padding to align the Ttablerow in memory
		(Ttablerow = (127+1)*16=2048 bits or 256 bytes) */

/*****************************************************************/
/* building the automata and autocompletion cache */
/*****************************************************************/

#define COMMENT_INDEX_INHERIT 255
#define COMMENT_INDEX_NONE 254

typedef struct {
	gboolean autocomplete_case_insens;
	GCompletion* ac; /* autocompletion items in this context */
	/*GHashTable *reference;*/ /* reference help for each autocompletion item */
	GHashTable *patternhash; /* a hash table where the pattern and its autocompletion string are the keys, and an integer to the ID of the pattern is the value */
	GtkTextTag *contexttag; /* if the context area itself needs some kind of style (to implement a string context for example) */
	gchar *contexthighlight; /* the string that has the id for the highlight */
	guint16 startstate; /* refers to the row number in scantable->table that is the start state for this context */
	guint16 identstate; /* refers to the row number in scantable->table that is the identifier-state
					for this context. The identifier state is a state that refers to itself for all characters
					except the characters (symbols) thay may be the begin or end of an identifier such
					as whitespace, ();[]{}*+-/ etc. */
	guint8 has_tagclose_from_blockstack; /* this context has xml end patterns that need autoclosing for generix xml tags, based on the tag that is on top of the blockstack */
	guint8 comment_block; /* block comment index in array scantable->comments 
				or COMMENT_INDEX_INHERIT (which means inherit) 
				or COMMENT_INDEX_NONE if there is no block comment  */
	guint8 comment_line; /* index in array scantable->comments for line comments; see comment_block */
/*#ifdef HAVE_LIBENCHANT_OLD
	guint8 needspellcheck;
#endif / *HAVE_LIBENCHANT*/
} Tcontext;

typedef struct {
	gchar *autocomplete_string;
	guint8 autocomplete_backup_cursor; /* number of characters to backup the cursor after autocompletion (max 256) */
	 /* guint8 autocomplete;whether or not this pattern should be added to the autocompletion; stored in the Tpattern so we can re-use it in another context */
} Tpattern_autocomplete;

typedef struct {
	GtkTextTag *selftag; /* the tag used to highlight this pattern */
	GtkTextTag *blocktag; /* if this pattern ends a context or a block, we can highlight
	the region within the start and end pattern with this tag */
	gchar *reference; /* the reference data, or NULL. may be inserted in hash tables for multiple keys in multiple contexts */
	gchar *pattern; /* the pattern itself. stored in the Tpattern so we can re-use it in another context */
	GSList *autocomp_items;
	gchar *selfhighlight; /* a string with the highlight for this pattern. used when re-linking highlights and textstyles 
							if the user changed any of these in the preferences */
	gchar *blockhighlight; /* a string for the highlight corresponding to the  blocktag */

	gint16 blockstartpattern; /* the number of the pattern that may start this block, or -1 to end the last started block */
	gint16 nextcontext; /* 0, or if this pattern starts a new context the number of the context, or -1 or -2 etc.
			to pop a context of the stack */
	/* TODO:  all following guint8 entries are booleans that can be combined into a single 8 bits integer with a bitmask */ 
	guint8 starts_block; /* wether or not this pattern may start a block */
	guint8 ends_block; /* wether or not this pattern may end a block */
	guint8 case_insens;
	guint8 is_regex;
	guint8 tagclose_from_blockstack; /* this is a generix xml close tag that needs the blockstack to autoclose */
#ifdef IDENTSTORING
	guint8 identmode;
#endif /* IDENTSTORING */
	/*gboolean may_fold;  not yet used */
	/*gboolean highlight_other_end; not yet used */
} Tpattern;

typedef struct {
	guint16 row[NUMSCANCHARS];	/* contains for each character the number of the next state
						because we use a 16bit unsigned number we can support only 65535 states
						at maximum!!!!!!! but we use half the size of the scanning table, which
						hopefully helps to keep the scanning table in the L2 cache of the CPU */
	guint16 match;			/* 0 == no match, refers to the index number in array 'matches' */
} Ttablerow; /* a row in the DFA, right now exactly 256 bytes */

typedef enum {
	comment_type_block,
	comment_type_line
} Tcomment_type;

typedef struct {
	gchar *so;
	gchar *eo;
	Tcomment_type type;
} Tcomment;

typedef struct {
	guint8 allsymbols[128]; /* this lookup table holds all symbols for all contexts, and is used to trigger scanning if reduced_scan_triggers is enabled */
	GArray *table; /* dynamic sized array of Ttablerow: the DFA table, max 65.... entries, we use a guint16 as index */
	GArray *contexts; /* dynamic sized array of Tcontext that translates a context number into a rownumber in the DFA table */
	GArray *matches; /* dynamic sized array of Tpattern */
	GArray *comments; /* Tcomment, has max. 256 entries, we use a guint8 as index */
} Tscantable;

/*****************************************************************/
/* scanning the text and caching the results */
/*****************************************************************/
typedef struct {
	gpointer parentfblock;
	guint32 start1_o;
	guint32 end1_o;
	guint32 start2_o;
	guint32 end2_o;
	gint16 patternum; /* which pattern (number of the array element in scantable->matches) */
	guint8 folded;
	guint8 foldable; /* FALSE on a single line */
} Tfoundblock; /* Once a start-of-block is found start1 and end1 are set
							and the Tfoundblock is added to the foundcache.
							The previous foundblock is set as parentfblock
							so we can later on find what the current blockstack looks like.
						Once the end-of-block is found, start2 and end2 are set
							The Tfoundblock is popped as current block, and the parent
							is active again. This is also put on the foundcache
							
						on 64bit this type has size 8+4+4+4+4+2+1+1 + 4 padding = 32 bytes
						on 32bit this type has size 4+4+4+4+4+2+1+1 NO padding = 24 bytes
							*/

typedef struct {
	gpointer parentfcontext;
	guint32 start_o;
	guint32 end_o;
	gint16 context; /* number of the element in scantable->contexts */
} Tfoundcontext; /* Once a start-of-context is found start is set
						and the Tfoundcontext is added to the current foundcache.
						The previous fcontext is set in parentfcontext
						so we can later on find what the current contextstack looks like.
						once the end-of-context is found, end is set
						The Tfoundcontext is popped from the current stack and
						this entry is also added to the foundcache
						
						on 64bit this type has size 8+4+4+2 + 6 padding = 24 bytes
						on 32bit this type has size 4+4+4+2 + 2 padding = 16 bytes
						*/

typedef struct {
	Tfoundcontext *fcontext; /* if numcontextchange == 0 this points to the current active context
				if numcontextchange > 0 this points to the pushed context, which also happens to be the current context
	 			if numcontextchange < 0 this points to the top of the stack at this position, to get the current position 
	 			you'll have to pop N items (where N is -1 * numcontextchange). */
	Tfoundblock *fblock; /* if numblockchange == 0 this points to the current active block
				if numblockchange > 0 this points to the pushed block, which also happens to be the current block
	 			if numblockchange < 0 this points to the top of the stack at this position, to get the current position 
	 			you'll have to pop N items (where N is -1 * numblockchange). */ 
	guint32 charoffset_o;
	gint8 numcontextchange; /* 0 means no change, 1 means 1 push, -2 means 2 popped etc. */
	gint8 numblockchange;
} Tfound; /*
						on 64bit this type has size 8+8+4+1+1 + 2 padding = 24 bytes
						on 32bit this type has size 4+4+4+1+1 + 2 padding = 16 bytes
						*/


#define IS_FOUNDMODE_CONTEXTPUSH(i)   (i->numcontextchange > 0)
#define IS_FOUNDMODE_CONTEXTPOP(i)   (i->numcontextchange < 0)
#define IS_FOUNDMODE_BLOCKPUSH(i)   (i->numblockchange >0)
#define IS_FOUNDMODE_BLOCKPOP(i)   (i->numblockchange < 0)

typedef struct {
	GSequence* foundcaches; /* a sorted structure of Tfound for
				each position where the stack changes so we can restart scanning
				on any location */
	guint32 valid_cache_offset; /* from 0 to this offset the cache is considered valid. after this 
						position the cache could be outdated */ 
} Tscancache;
/********************************/
/* language manager */
/********************************/
typedef struct {
	gchar *name;
	GList *mimetypes;
	/*GList *langoptions;*/ /* all options that can be enabled/disabled for this language and their default value (0 or 1) */
	/*GList *setoptions;*/ /* all options that are enabled have value '1' in this hashtable */
	GList *tags; /* all tags used for highlighting in this language. we use this list when 
						we want to remove all tags and want to re-highlight */
	gchar *filename; /* the .bflang2 file */
	Tscantable *st; /* NULL or complete */
	gchar *smartindentchars;
	gchar *smartoutdentchars;
#ifdef HAVE_LIBENCHANT
	gboolean default_spellcheck;
	gboolean spell_decode_entities;
#endif
	gboolean no_st; /* no scantable, for Text, don't try to load the scantable if st=NULL */
	gboolean parsing; /* set to TRUE when a thread is parsing the scantable already */
	gint size_table;
	gint size_contexts;
	gint size_matches;
} Tbflang;


/* Color Configuation data */
typedef enum {
	BTV_COLOR_ED_BG,
	BTV_COLOR_ED_FG,
	BTV_COLOR_WHITESPACE,
	BTV_COLOR_CURRENT_LINE,
	BTV_COLOR_RIGHT_MARGIN,
	BTV_COLOR_CURSOR,
/*	BTV_COLOR_SEARCH_BG,
	BTV_COLOR_SEARCH_FG,*/
	BTV_COLOR_COUNT
} Tbtv_colors;


/*****************************************************************/
/* stuff for the widget */
/*****************************************************************/

#define BLUEFISH_TYPE_TEXT_VIEW            (bluefish_text_view_get_type ())
#define BLUEFISH_TEXT_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BLUEFISH_TYPE_TEXT_VIEW, BluefishTextView))
#define BLUEFISH_TEXT_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BLUEFISH_TYPE_TEXT_VIEW, BluefishTextViewClass))
#define BLUEFISH_IS_TEXT_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BLUEFISH_TYPE_TEXT_VIEW))
#define BLUEFISH_IS_TEXT_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), BLUEFISH_TYPE_TEXT_VIEW))
#define BLUEFISH_TEXT_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BLUEFISH_TYPE_TEXT_VIEW, BluefishTextViewClass))

typedef struct _BluefishTextView BluefishTextView;
typedef struct _BluefishTextViewClass BluefishTextViewClass;

struct _BluefishTextView {
	GtkTextView parent;
	gpointer master; /* points usually to self, but in the case of a slave widget 
					(two widgets showing the same buffer it will point to the master widget) */
	gpointer slave; /* usually NULL, but might point to a slave widget */
	Tbflang *bflang; /* Tbflang */
	gpointer doc; /* Tdocument */
	GtkTextBuffer *buffer;
	GtkTextTag *needscanning;
#ifdef HAVE_LIBENCHANT
	GtkTextTag *needspellcheck;
#endif /*HAVE_LIBENCHANT*/
	GtkTextTag *blockmatch;
	Tscancache scancache;
	
	guint scanner_idle; /* event ID for the idle function that handles the scanning. 0 if no idle function is running */
	guint scanner_delayed; /* event ID for the timeout function that handles the delayed scanning. 0 if no timeout function is running */
	GTimer *user_idle_timer;
	guint user_idle; /* event ID for the timed function that handles user idle events such as autocompletion popups */
	guint mark_set_idle; /* event ID for the mark_set idle function that avoids showing matching block bounds while 
								you hold the arrow key to scroll quickly */
	gulong insert_text_id;
	gulong insert_text_after_id;
	gulong mark_set_id;
	gulong delete_range_id;
	gulong delete_range_after_id;
	
	gpointer autocomp; /* a Tacwin* with the current autocompletion window */
	gboolean needs_autocomp; /* a state of the widget, autocomplete is needed on user keyboard actions */
	gboolean needs_blockmatch; /* a state of the widget, if the cursor position was changed */
	gboolean key_press_inserted_char; /* FALSE if the key press was used by autocomplete popup, or simply not in our widget */
	/*gboolean key_press_was_autocomplete;  a state of the widget, if the last keypress was handled by the autocomplete popup window */
	gboolean showing_blockmatch; /* a state of the widget if we are currently showing a blockmatch */
	gboolean insert_was_auto_indent; /* a state of the widget if the last keypress (enter) caused 
													autoindent (so we should unindent on a closing bracket */
	gboolean needremovetags; /* after we have removed all old highlighting, we set this to FALSE
										but after a change that needs highlighting we set this to TRUE again */

	/* next three are used for margin painting */
	gint margin_pixels_per_char;
	gint margin_pixels_chars;
	gint margin_pixels_block;
	gint margin_pixels_symbol;
	
	/* following options are simple true/false settings */
	gboolean enable_scanner; /* only run scanner when TRUE, this is FALSE if the document is in the background for example */
	gboolean auto_indent;
	gboolean auto_complete;
	gboolean show_line_numbers;
	gboolean show_blocks;
	gboolean showsymbols;
	gboolean visible_spacing;
	gboolean show_right_margin;
	gboolean show_mbhl; /* show matching block highlighting */
#ifdef HAVE_LIBENCHANT
	gboolean spell_check;
#endif
};

struct _BluefishTextViewClass {
	GtkTextViewClass parent_class;
};

GType bluefish_text_view_get_type (void);


gboolean bluefish_text_view_get_auto_complete(BluefishTextView * btv);
void bluefish_text_view_set_auto_complete(BluefishTextView * btv, gboolean enable);
gboolean bluefish_text_view_get_auto_indent(BluefishTextView * btv);
void bluefish_text_view_set_auto_indent(BluefishTextView * btv, gboolean enable);
void bftextview2_init_globals(void);
void bluefish_text_view_set_colors(BluefishTextView * btv, gchar * const *colors);
void bluefish_text_view_set_mimetype(BluefishTextView * btv, const gchar *mime);
gboolean bluefish_text_view_get_show_blocks(BluefishTextView * btv);
void bluefish_text_view_set_show_blocks(BluefishTextView * btv, gboolean show);
gboolean bluefish_text_view_get_show_line_numbers(BluefishTextView * btv);
void bluefish_text_view_set_show_line_numbers(BluefishTextView * btv, gboolean show);
gboolean bluefish_text_view_get_show_visible_spacing(BluefishTextView * btv);
void bluefish_text_view_set_show_visible_spacing(BluefishTextView * btv, gboolean show);
gboolean bluefish_text_view_get_show_right_margin(BluefishTextView * btv);
void bluefish_text_view_set_show_right_margin(BluefishTextView * btv, gboolean show);
gboolean bluefish_text_view_get_show_mbhl(BluefishTextView * btv);
void bluefish_text_view_set_show_mbhl(BluefishTextView * btv, gboolean show);
#ifdef HAVE_LIBENCHANT
void bluefish_text_view_set_spell_check(BluefishTextView * btv, gboolean spell_check);
#endif
void bluefish_text_view_scan_cleanup(BluefishTextView * btv);
void bluefish_text_view_rescan(BluefishTextView * btv);
void bftextview2_schedule_scanning(BluefishTextView * btv);
gboolean bluefish_text_view_in_comment(BluefishTextView * btv, GtkTextIter *its, GtkTextIter *ite);
Tcomment *bluefish_text_view_get_comment(BluefishTextView *btv, GtkTextIter *it, Tcomment_type preferred_type);
void bluefish_text_view_multiset(BluefishTextView *btv
			, gpointer doc
			, gint view_line_numbers
			, gint view_blocks
			, gint autoindent
			, gint autocomplete
			, gint show_mbhl);

GtkWidget * bftextview2_new(void);
GtkWidget * bftextview2_new_with_buffer(GtkTextBuffer * buffer);
GtkWidget *bftextview2_new_slave(BluefishTextView *master);

#endif
