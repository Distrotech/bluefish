#ifndef _BFTEXTVIEW_H_
#define _BFTEXTVIEW_H_

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
	char *message;
	int nextcontext;
	GtkTextTag *selftag;
	gboolean starts_block;
	gboolean ends_block;
	GtkTextTag *blocktag;
	int patternum blockstartpattern;
} Tpattern;

typedef struct {
	unsigned int row[NUMSCANCHARS];	/* contains for each character the number of the next state */
	unsigned int match;			/* 0 == no match, refers to the index number in array 'matches' */
} Ttablerow;

typedef struct {
	int nextnewpos;
	int nextnewmatch;
	GArray *table;
	GArray *matches;
	GArray *contexts;
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
	GQueue *stack; /* a Tfoundcontext or Tfoundblock stack */
	Tstacktype type;
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
	guint scanner_idle; /* 0 if no idle function is running */
	GTimer *timer;
} Tbftextview2;

typedef struct {
	GtkTextViewClass __parent__;
} Tbftextview2Class;

extern Tscantable scantable;

#endif
