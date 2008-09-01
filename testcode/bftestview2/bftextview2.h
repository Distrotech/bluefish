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
- the context is kept on a stack. once the end-of-context pattern is found, we revert to
  the previous context

- changed areas are marked with a GtkTextTag called 'needscanning'.
 - a idle function is started (if not running) to do the actual scanning
 - the scanner keeps a timer and stops scanning once a certain time has passed
 - if the scanning is finished the idle function stops

- to find where to resume scanning it simply searches for the first position 
  that is marked with this tag and backs-up to the beginning of the line   
 - to know which patterns to use we have to know in which context we are. we therefore keep 
   a cache of the (context)stack. on each position where to contextstack changes, we make a copy 
   of the current stack that we keep in a sorted balanced tree (a GSequence), sorted by the 
   position in the text.
   - the positions change when text is inserted or deleted, but never their order. a GSequence 
     allows us to update the offsets for the stacks without re-sorting the entire tree
 - same holds for the blocks. we keep a blockstack, and we keep a cache of the blockstack in the 
   same stackcache as where we keep the contextstack.

- the current scanner is a DFA (deterministic finit automata) but it can be done with regex as well

*/

#ifndef _BFTEXTVIEW2_H_
#define _BFTEXTVIEW2_H_

#include <gtk/gtk.h>

#define NUMSCANCHARS 127
#define MAXSCANSTATES 1000
#define MAXMATCHES 10
#define NUMCONTEXTS 10

/* building the automata */

typedef struct {
	GCompletion* ac;

} Tcontext;

typedef struct {
	char *message; /* for debugging */
	int nextcontext; /* 0, or if this pattern starts a new context the number of the contect */
	GtkTextTag *selftag; /* the tag used to highlight this pattern */
	gboolean starts_block; /* wheter or not this pattern may start a block */
	gboolean ends_block; /* wheter or not this pattern may end a block */
	GtkTextTag *blocktag; /* if this pattern ends a context or a block, we can highlight 
	the region within the start and end pattern with this tag */
	int blockstartpattern; /* the number of the pattern that may start this block */
} Tpattern;

typedef struct {
	unsigned int row[NUMSCANCHARS];	/* contains for each character the number of the next state */
	unsigned int match;			/* 0 == no match, refers to the index number in array 'matches' */
} Ttablerow; /* a row in the DFA */

typedef struct {
	int nextnewpos;
	int nextnewmatch;
	GArray *table; /* dynamic sized array of Ttablerow: the DFA table */
	GArray *contexts; /* dynamic sized array that translates a context number into a rownumber in the DFA table */
	GArray *matches; /* dynamic sized array of Tpattern */
} Tscantable;


/* scanning the text */
typedef struct {
	GtkTextMark *start1;
	GtkTextMark *end1;
	GtkTextMark *start2;
	GtkTextMark *end2;
	gint patternum;
} Tfoundblock; /* once a start-of-block is found start1 and end1 are set 
						and the Tfoundblock is added to the GtkTextMark's as "block"
						and the Tfoundblock is added to the current blockstack.
						A copy of the current blockstack is copied into Tscancache
						so we can later on find what the current blockstack looks like.
						once the end-of-block is found, start2 and end2 are set 
						and the Tfoundblock is added to these GtkTextMark's as "block"
						The Tfoundblock is popped from the current stack, and a new copy
						of the stack is copied into Tscancache */

typedef struct {
	GtkTextMark *start;
	GtkTextMark *end;
	gint contextnum;
} Tfoundcontext; /* once a start-of-context is found start is set 
						and the Tfoundcontext is added to the GtkTextMark as "context"
						and the Tfoundcontext is added to the current contextstack.
						A copy of the current contextstack is copied into Tscancache
						so we can later on find what the current contextstack looks like.
						once the end-of-context is found, end is set 
						and the Tfoundcontext is added to this GtkTextMark as "context"
						The Tfoundcontext is popped from the current stack, and a new copy
						of the stack is copied into Tscancache */ 

typedef enum {
	CONTEXTSTACK,
	BLOCKSTACK
} Tstacktype;

typedef struct {
	GQueue *stack; /* a stack of Tfoundcontext or Tfoundblock stack */
	Tstacktype type;
	guint charoffset; /* the stackcaches (see below in Tscancache) is sorted on this offset */
	guint line; /* a line that starts a block should be very quick to find (during the expose event) 
						because we need to draw a collapse icon in the margin. because the stackcaches are 
						sorted by charoffset they are automatically also sorted by line. finding the first 
						visible block should be easy, and then we can iterare over the stackcaches until 
						we're out of the visible area */
} Tfoundstack;

typedef struct {
	GSequence* stackcaches; /* a sorted cache of all blocks and context stacks for 
							each position where the stack changes so we can restart scanning 
							on any location */
} Tscancache;

/* stuff for the widget */

typedef struct {
	GtkTextView __parent__;
	Tscantable *scantable;
	Tscancache scancache;
	guint scanner_idle; /* event ID for the idle function that handles the scanning. 0 if no idle function is running */
	GTimer *timer;
} Tbftextview2;

typedef struct {
	GtkTextViewClass __parent__;
} Tbftextview2Class;

extern Tscantable scantable;

#endif
