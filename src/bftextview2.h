/* Bluefish HTML Editor
 * bftextview2.h
 *
 * Copyright (C) 2008,2009,2010,2011,2012,2013 Olivier Sessink
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

============= The widget code =============
- to avoid locking the GUI, it should be able to scan in multiple runs
  - it should be able to mark a region as 'needs scanning'
  - it should be able to resume scanning on any point marked as 'needs scanning'

the widget code, including signal handlers and idle handlers are in bftextview2.c

============== Syntax scanning =============
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


============ The scanned syntax cache ============
- to find where to resume scanning it simply searches for the first position
  that is marked with the needscanning tag 
 - to know which patterns to use we have to know in which context we are. we therefore keep
   a cache of the (context)stack. on each position where the contextstack changes, we make a copy
   of the current state and store it in a sorted balanced tree (a GSequence), sorted by the
   character offset in the text. We store a Tfound structure. A member of the Tfound structure
   is a pointer to the Tfoundcontext stucture which describes the current context.
   - the positions change when text is inserted or deleted, but never their order. a GSequence
     allows us to update the offsets for the stacks without re-sorting the entire tree
 - same holds for the blocks. we keep a blockstack, and we keep a cache of the blockstack in the
   same foundcache as where we keep the contextstack. The member of the Tfound structure that
   describes the state for blocks is the Tfoundblock structure.
   
- when a new block is found, a Tfound structure has a member fblock of type Tfoundblock 
  that points to the new block, and member numblockchange is 1. That Tfoundblock has a 
  pointer to it's parent block.
- when an end-of-block is found, the Tfound structure has again a member fblock that
  points to the popped block, and numblockchange is -1. So to get the active block *after*
  a popped block, you have to look at the parent of the fblock member!!!!
- the Tfound member charoffset_o has the character offset of the end-of-the-end-of-context-match 
  (Tfoundcontext->end_o) or the end-of-the-end-of-block-match (Tfoundblock->end2_o).

The next ascii art shows how blocks are stored in the scancache. This is a special situation
in which a second block starts but does not have an end, so both blocks are popped at 'f'. A 
Tfound structure is saved at offset B, D and F.  

     --------------------Block-1-with-valid-end--------------------
     |  |                                                      |  |
     |  |                        ------Block-without-end-------|--|
     |  |                        |   |                         |  | 
-----a--B------------------------c---D-------------------------e--F---------
     |  |                        |   |                         |  |
     |  |Tfound numblockchange=1 |   |Tfound numblockchange=1  |  |Tfound nublockchange=-2
     |  |end1_o == charoffset_o  |   |end1_o == charoffset_o   |  | end2_o == charoffset_o for the block1
     |                           |                             |
     |start1_o                   |start1_o                     |start2_o for block1


- to paint the margin and detect if we can expand/collapse blocks, we can use this same
  scancache. Along with walking the lines to draw the line numbers we walk the GSequence 
  and see in the Tfound structures if there are new blocks that can be folded.


=========== Scanning with a DFA engine ==============
- the current scanning is based on Deterministic Finite Automata (DFA) just like the 1.1.6
unstable engine (see wikipedia for more info). The 1.1.6 engine alloc's each state in a
separate memory block. This new engine alloc's a large array for all states at once, so you can
simply move trough the array instead of following pointers. 
The array is the array 'table' in structure Tcontext. So each context has it's own 
DFA table. Following the DFA is then as simple as state = table[state][character]; 
where state is just an integer position in the array, and character is the current character 
you're scanning. The array will help to speed up the scanner. I used guint16 because I 
suspect that we never hit the 65500 states for a single context (largest patterns set right 
now is php, 4500 functions use 32000 states). 
When a state has a positive result (it matches something) it has an index number to
an array 'matches' in structure Tscantable which is an array of type Tpattern structure
that has the information for the matched pattern.

- each context has it's own DFA table. The startcontext for each context is always 
position 0 and the identstate is always position 1 in that array

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

Language file loading is done in bftextview2_langmgr.c, the found patterns are added 
to the Tscantable structure and it's members (and compiled into a DFA table) 
in bftextview2_patcompile.c.

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

The Tcontext structure has a startstate for each context and an identifier-state (identstate).
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
for autocompletion we keep a GCompletion in each context (member 'ac' of structure Tcontext). 
This is filled with all the patterns during XML load.

we use a similar scanning engine as above that can tell us where the string that 
the user is typing started, and in which context the curor position is. Once
we know the context we know which GCompletion structure to use, so we can get 
a list of possible completion strings.

The scanning for the context is done in bftextview2_scanner.c, the rest of the autocompletion
code is in bftextview2_autocomp.c

======== Reference information ==========
reference information can be shown in a tooltip above the text and in a side window 
during autocompletion. The reference information is stored in a member of the Tpattern
structure. Each Tcontext structure has a member patternhash that is a hashtable with the 
match as key and the index to the pattern in array Tscantable->matches as value.

For the tooltip we do a short scanning run to find the context and  which pattern is 
actually under the cursor and we do a hash table lookup to find the corresponding
reference information.

========= Spell checker ===========
the spell checker is in bftextview2_spell.c

after the syntax scanning is finished the spell checker is started. Similar to the syntax
scanner it runs in short timeslices such that it won't block the GUI. It scans only in 
certain GtkTextTag's (for example in the GtktextTag for comments and for strings).

======= Storing found function names and such for jump and autocompletion ======

identifiers, such as function names or variable names can be stored for jump and for
autocompletion. Code is in bftextview2_identifier.c

for jump: found functions names are stored in a hashtable 
bfwin->identifier_jump as
key Tbflang-context-name -> value Tdocument-linenumber

for autocompletion they are added to a GCompletion
the GCompletion can be found in hashtable
bfwin->identifier_ac with 
key Tbflang-context -> value GCompletion

identifier_mode="1" means that the following the *following* identifier is to be stored. For example in 
php 'function', and in python 'def' and 'class' (implemented in bluefish 2.0.3).
identifier_mode="2" means that the match itself is to be stored as an identifier, for example in php
the variable '$[a-zA-Z_][a-zA-Z0-9_]*' (implemented in 2.0.4)

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

#define IDENTSTORING

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
	guint8 allsymbols[128];		/* this lookup table holds all symbols for all contexts, and is used to trigger scanning if reduced_scan_triggers is enabled */
	GArray *contexts;			/* dynamic sized array of Tcontext that translates a context number into a rownumber in the DFA table */
	GArray *matches;			/* dynamic sized array of Tpattern */
	GArray *comments;			/* array of Tcomment, has max. 256 entries, we use a guint8 as index */
	GArray *blocks; 			/* array of Tpattern_block with a guint16 as index */
} Tscantable;

typedef struct {
	GSequence *foundcaches;		/* a sorted structure of Tfound for
								   each position where the stack changes so we can restart scanning
								   on any location */
} Tscancache;
/********************************/
/* language manager */
/********************************/
typedef struct {
	gchar *name;
	GList *mimetypes;
	GList *tags;				/* all tags used for highlighting in this language. we use this list when 
								   we want to remove all tags and want to re-highlight */
	gchar *filename;			/* the .bflang2 file */
	Tscantable *st;				/* NULL or complete */
	gchar *smartindentchars;
	gchar *smartoutdentchars;
#ifdef HAVE_LIBENCHANT
	gboolean default_spellcheck;
	gboolean spell_decode_entities;
#endif
	gboolean no_st;				/* no scantable, for Text, don't try to load the scantable if st=NULL */
	gboolean parsing;			/* set to TRUE when a thread is parsing the scantable already */
	gboolean in_menu; /* set to TRUE to show this language in the menu */ 
	gint size_table;
	gint size_contexts;
	gint size_matches;
} Tbflang;

#define BFLANG(var)  ((Tbflang *)var)

/* Color Configuation data */
typedef enum {
	BTV_COLOR_ED_BG,
	BTV_COLOR_ED_FG,
	BTV_COLOR_WHITESPACE,
	BTV_COLOR_CURRENT_LINE,
	BTV_COLOR_RIGHT_MARGIN,
	BTV_COLOR_CURSOR,
	BTV_COLOR_SELECTION,
	BTV_COLOR_CURSOR_HIGHLIGHT,
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
	gpointer master;			/* points usually to self, but in the case of a slave widget 
								   (two widgets showing the same buffer it will point to the master widget) */
	gpointer slave;				/* usually NULL, but might point to a slave widget */
	Tbflang *bflang;			/* Tbflang */
	gpointer doc;				/* Tdocument */
	GtkTextBuffer *buffer;
	GtkTextTag *needscanning;
#ifdef HAVE_LIBENCHANT
	GtkTextTag *needspellcheck;
#endif							/*HAVE_LIBENCHANT */
	GtkTextTag *blockmatch;
	GtkTextTag *cursortag;
	Tscancache scancache;

	guint scanner_immediate; /* event ID for the high priority scanning run */
	guint scanner_idle;			/* event ID for the idle function that handles the scanning. 0 if no idle function is running */
	guint scanner_delayed;		/* event ID for the timeout function that handles the delayed scanning. 0 if no timeout function is running */
	GTimer *user_idle_timer;
	guint user_idle;			/* event ID for the timed function that handles user idle events such as autocompletion popups */
	guint mark_set_idle;		/* event ID for the mark_set idle function that avoids showing matching block bounds while 
								   you hold the arrow key to scroll quickly */
	gulong insert_text_id;
	gulong insert_text_after_id;
	gulong mark_set_id;
	gulong delete_range_id;
	gulong delete_range_after_id;

	gpointer autocomp;			/* a Tacwin* with the current autocompletion window */
	gboolean needs_autocomp;	/* a state of the widget, autocomplete is needed on user keyboard actions */
	gboolean needs_blockmatch;	/* a state of the widget, if the cursor position was changed */
	gboolean key_press_inserted_char;	/* FALSE if the key press was used by autocomplete popup, or simply not in our widget */
	gdouble button_press_line; /* line location of the button press, used in the release */
	/*gboolean key_press_was_autocomplete;  a state of the widget, if the last keypress was handled by the autocomplete popup window */
	gboolean showing_blockmatch;	/* a state of the widget if we are currently showing a blockmatch */
	gboolean insert_was_auto_indent;	/* a state of the widget if the last keypress (enter) caused 
										   autoindent (so we should unindent on a closing bracket */
	guint needremovetags;	/* after we have removed all old highlighting, we set this to G_MAXUINT32, or to the 
									offset up to the point where we removed the old highlighting. but after a change that
									needs highlighting we set this to the offset of the change. */
	/* next three are used for margin painting */
	gint margin_pixels_per_char;
	gint margin_pixels_chars;
	gint margin_pixels_block;
	gint margin_pixels_symbol;

	/* following options are simple true/false settings */
	gboolean enable_scanner;	/* only run scanner when TRUE, this is FALSE if the document is in the background for example */
	gboolean auto_indent;
	gboolean auto_complete;
	gboolean show_line_numbers;
	gboolean show_blocks;
	gboolean showsymbols;
	gboolean visible_spacing;
	gboolean show_right_margin;
	gboolean show_mbhl;			/* show matching block highlighting */
#ifdef HAVE_LIBENCHANT
	gboolean spell_check;
#endif
};

struct _BluefishTextViewClass {
	GtkTextViewClass parent_class;
};

GType bluefish_text_view_get_type(void);

gboolean bluefish_text_view_get_active_block_boundaries(BluefishTextView *btv, guint location, gboolean innerblock, GtkTextIter *so, GtkTextIter *eo);
gboolean bluefish_text_view_get_auto_complete(BluefishTextView * btv);
void bluefish_text_view_set_auto_complete(BluefishTextView * btv, gboolean enable);
gboolean bluefish_text_view_get_auto_indent(BluefishTextView * btv);
void bluefish_text_view_set_auto_indent(BluefishTextView * btv, gboolean enable);
void bftextview2_init_globals(void);
void bluefish_text_view_set_colors(BluefishTextView * btv, gchar * const *colors);
void bluefish_text_view_select_language(BluefishTextView * btv, const gchar * mime, const gchar * filename);
gboolean bluefish_text_view_get_show_blocks(BluefishTextView * btv);
void bluefish_text_view_set_show_blocks(BluefishTextView * btv, gboolean show);
void bluefish_text_view_set_show_symbols_redraw(BluefishTextView * btv, gboolean show);
gboolean bluefish_text_view_get_show_line_numbers(BluefishTextView * btv);
void bluefish_text_view_set_show_line_numbers(BluefishTextView * btv, gboolean show);
gboolean bluefish_text_view_get_show_visible_spacing(BluefishTextView * btv);
void bluefish_text_view_set_show_visible_spacing(BluefishTextView * btv, gboolean show);
gboolean bluefish_text_view_get_show_right_margin(BluefishTextView * btv);
void bluefish_text_view_set_show_right_margin(BluefishTextView * btv, gboolean show);
void bluefish_text_view_set_font(BluefishTextView *btv, PangoFontDescription *font_desc);
gboolean bluefish_text_view_get_show_mbhl(BluefishTextView * btv);
void bluefish_text_view_set_show_mbhl(BluefishTextView * btv, gboolean show);
#ifdef HAVE_LIBENCHANT
void bluefish_text_view_set_spell_check(BluefishTextView * btv, gboolean spell_check);
#endif
void bluefish_text_view_scan_cleanup(BluefishTextView * btv);
void bluefish_text_view_rescan(BluefishTextView * btv);
void bftextview2_schedule_scanning(BluefishTextView * btv);
gboolean bluefish_text_view_in_comment(BluefishTextView * btv, GtkTextIter * its, GtkTextIter * ite);
Tcomment *bluefish_text_view_get_comment(BluefishTextView * btv, GtkTextIter * it,
										 Tcomment_type preferred_type);
void bluefish_text_view_multiset(BluefishTextView * btv, gpointer doc, gint view_line_numbers,
							gint view_blocks, gint autoindent, gint autocomplete, gint show_mbhl, gint enable_scanner);

GtkWidget *bftextview2_new(void);
GtkWidget *bftextview2_new_with_buffer(GtkTextBuffer * buffer);
GtkWidget *bftextview2_new_slave(BluefishTextView * master);

#endif
