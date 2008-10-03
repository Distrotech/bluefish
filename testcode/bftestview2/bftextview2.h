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
   character offset in the text.
   - the positions change when text is inserted or deleted, but never their order. a GSequence 
     allows us to update the offsets for the stacks without re-sorting the entire tree
 - same holds for the blocks. we keep a blockstack, and we keep a cache of the blockstack in the 
   same stackcache as where we keep the contextstack.

- to paint the margin and detect if we can expand/collapse blocks, we can use this same 
  scancache. Along with walking the lines we walk the GSequence and see if there are new 
  blocks that can be folded. 

- the current scanning is based on Deterministic Finite Automata (DFA) just like the current 
unstable engine (see wikipedia for more info). The unstable engine alloc's each state in a 
separate memory block. This engine alloc's a large array for all states at once, so you can 
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

- the languages are defined in an XML file. On startup, only the header of that file is parsed,
into a Tbflang struct, which defines the language and the mime types. Only when scanning for 
one of these mime-types is requested the rest of the file is parsed (in a separate thread!!!) 
and the DFA for this language is created. This saves memory and startup time for languages 
that are not used in a certain session.
*/

#ifndef _BFTEXTVIEW2_H_
#define _BFTEXTVIEW2_H_

#include <gtk/gtk.h>

#define DBG_NONE(args...)
 /**/

#define DBG_MSG DBG_NONE
#define DBG_SCANCACHE DBG_NONE
#define DBG_REFCOUNT DBG_NONE
#define DBG_PATCOMPILE DBG_NONE
#define DBG_SIGNALS DBG_NONE
#define DBG_AUTOCOMP DBG_NONE
#define DBG_SCANNING DBG_NONE
#define DBG_FOLD DBG_NONE
#define DBG_MARGIN DBG_NONE
#define DBG_PARSING DBG_NONE

#define NUMSCANCHARS 127 /* 128 is ascii, but the last character is never scanned (DEL)
		and the Ttablerow has one more 16bit value. By setting this to 127 instead of 128
		we don't need padding to align the Ttablerow in memory 
		(Ttablerow = (127+1)*16=2048 bits or 256 bytes) */

#define USER_IDLE_EVENT_INTERVAL 480 /* milliseconds */

#define MAX_CONTINUOUS_SCANNING_INTERVAL 0.1 /* float in seconds */ 

/*****************************************************************/
/* building the automata and autocompletion cache */
/*****************************************************************/

typedef struct {
	GCompletion* ac; /* autocompletion items in this context */
	GHashTable *reference; /* reference help for each autocompletion item */
	GtkTextTag *contexttag; /* if the context area itself needs some kind of style (strings!) */
	guint16 startstate; /* refers to the row number in scantable->table that is the start state for this context */
	guint16 identstate; /* refers to the row number in scantable->table that is the identifier-state 
					for this context. The identifier state is a state that refers to itself for all characters
					except the characters (symbols) thay may be the begin or end of an identifier such
					as whitespace, ();[]{}*+-/ etc. */
	
} Tcontext;

typedef struct {
	char *message; /* for debugging */
	GtkTextTag *selftag; /* the tag used to highlight this pattern */
	GtkTextTag *blocktag; /* if this pattern ends a context or a block, we can highlight 
	the region within the start and end pattern with this tag */
	guint16 blockstartpattern; /* the number of the pattern that may start this block */
	guint16 nextcontext; /* 0, or if this pattern starts a new context the number of the contect */
	guint8 starts_block; /* wether or not this pattern may start a block */
	guint8 ends_block; /* wether or not this pattern may end a block */
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

typedef struct {
	GArray *table; /* dynamic sized array of Ttablerow: the DFA table */
	GArray *contexts; /* dynamic sized array of Tcontext that translates a context number into a rownumber in the DFA table */
	GArray *matches; /* dynamic sized array of Tpattern */
} Tscantable;

/*****************************************************************/
/* scanning the text and caching the results */
/*****************************************************************/
typedef struct {
	GtkTextMark *start1;
	GtkTextMark *end1;
	GtkTextMark *start2;
	GtkTextMark *end2;
	guint16 patternum;
	guint16 refcount; /* free on 0 */
	guint8 folded;
	guint8 foldable; /* perhaps on a single line ? */
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
	guint16 context;
	guint16 refcount; /* free on 0 */
} Tfoundcontext; /* once a start-of-context is found start is set 
						and the Tfoundcontext is added to the GtkTextMark as "context"
						and the Tfoundcontext is added to the current contextstack.
						A copy of the current contextstack is copied into Tscancache
						so we can later on find what the current contextstack looks like.
						once the end-of-context is found, end is set 
						and the Tfoundcontext is added to this GtkTextMark as "context"
						The Tfoundcontext is popped from the current stack, and a new copy
						of the stack is copied into Tscancache */ 

typedef struct {
	GQueue *contextstack; /* a stack of Tfoundcontext */
	GQueue *blockstack;  /* a stack of Tfoundblock */
	Tfoundcontext *poppedcontext;
	Tfoundcontext *pushedcontext;
	Tfoundblock *poppedblock;
	Tfoundblock *pushedblock;
	guint charoffset; /* the stackcaches (see below in Tscancache) is sorted on this offset */
	guint line; /* a line that starts a block should be very quick to find (during the expose event) 
						because we need to draw a collapse icon in the margin. because the stackcaches are 
						sorted by charoffset they are automatically also sorted by line. finding the first 
						visible block should be easy, and then we can iterare over the stackcaches until 
						we're out of the visible area */
} Tfoundstack;

typedef struct {
	GSequence* stackcaches; /* a sorted structure of Tfoundstack for 
				each position where the stack changes so we can restart scanning 
				on any location */
	gint stackcache_need_update_charoffset; /* from this character offset and further there
				have been changes in the buffer so the caches need updating */
} Tscancache;
/********************************/
/* language manager */
/********************************/
typedef struct {
	gchar *name;
	GList *mimetypes;
	GList *langoptions; /* all options that can be enabled/disabled for this language */
	GList *setoptions; /* all options that are enabled */
	gchar *filename; /* the .bflang2 file */
	Tscantable *st; /* NULL or complete */
	gboolean parsing; /* set to TRUE when a thread is parsing the scantable already */
} Tbflang;


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
/*	Tscantable *scantable;*/
	Tbflang *bflang; /* Tbflang */
	Tscancache scancache;
	guint scanner_idle; /* event ID for the idle function that handles the scanning. 0 if no idle function is running */
	GTimer *user_idle_timer;
	guint user_idle; /* event ID for the timed function that handles user idle events such as autocompletion popups */
	gpointer autocomp; /* a Tacwin* with the current autocompletion window */
	gint margin_pixels_per_char;
	gint margin_pixels_chars;
	gint margin_pixels_block;	
};

struct _BluefishTextViewClass {
	GtkTextViewClass parent_class;
};

void bluefish_text_view_rescan(BluefishTextView * btv);
void bluefish_text_view_set_mimetype(BluefishTextView * btv, const gchar *mime);
GType bluefish_text_view_get_type (void);

GtkWidget * bftextview2_new(void);
GtkWidget * bftextview2_new_with_buffer(GtkTextBuffer * buffer);

#endif
