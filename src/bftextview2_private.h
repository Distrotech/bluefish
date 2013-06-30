/* Bluefish HTML Editor
 * bftextview2_private.h
 *
 * Copyright (C) 2012 Olivier Sessink
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

#ifndef _BFTEXTVIEW2_PRIVATE_H_
#define _BFTEXTVIEW2_PRIVATE_H_


#if defined(__GNUC__) || (defined(__SUNPRO_C) && __SUNPRO_C > 0x580)
#define DBG_NONE(args...)
 /**/
#else							/* notdef __GNUC__ || __SUNPRO_C */
extern void g_none(char *first, ...);
#define DBG_NONE g_none
#endif
 /**/

#define CURRENT_BFLANG2_VERSION "2.0"

#define BF_OFFSET_UNDEFINED G_MAXINT32
#define BF_POSITION_UNDEFINED G_MAXINT32

/*#define DUMP_SCANCACHE*/
/*#define CHECK_CONSISTENCY*/

#define DBG_MSG DBG_NONE
#define DBG_SCANCACHE DBG_NONE
#define DBG_SCANNING DBG_NONE
#define DBG_BLOCKMATCH DBG_NONE
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
#define DBG_MARKREGION DBG_NONE

#ifdef UPDATE_OFFSET_DELAYED
#include "bf_lib.h"
typedef struct {
	BF_ELIST_HEAD;
	guint32 startpos;
	gint32 offset;
} Toffsetupdate;
#define OFFSETUPDATE(var) ((Toffsetupdate *)var)
#endif /*UPDATE_OFFSET_DELAYED*/

#define NUMSCANCHARS 127		/* 128 is ascii, but the last character is never scanned (DEL)
								   and the Ttablerow has one more 16bit value. By setting this to 127 instead of 128
								   we dont need padding to align the Ttablerow in memory
								   (Ttablerow = (127+1)*16=2048 bits or 256 bytes) */
/*****************************************************************/
/* building the automata and autocompletion cache */
/*****************************************************************/
#define COMMENT_INDEX_INHERIT 255
#define COMMENT_INDEX_NONE 254
	typedef struct {
	GArray *table; /* a pointer to the DFA table for this context */
	GCompletion *ac;			/* autocompletion items in this context */
	/*GHashTable *reference; *//* reference help for each autocompletion item */
	GHashTable *patternhash;	/* a hash table where the pattern and its autocompletion string are the keys, and an integer to the ID of the pattern is the value */
	GtkTextTag *contexttag;		/* if the context area itself needs some kind of style (to implement a string context for example) */
	gchar *contexthighlight;	/* the string that has the id for the highlight */
	/*guint16 startstate;*/			/* refers to the row number in scantable->table that is the start state for this context */
	/*guint16 identstate;*/			/* refers to the row number in scantable->table that is the identifier-state
								   for this context. The identifier state is a state that refers to itself for all characters
								   except the characters (symbols) thay may be the begin or end of an identifier such
								   as whitespace, ();[]{}*+-/ etc. */
	guint8 has_tagclose_from_blockstack;	/* this context has xml end patterns that need autoclosing for generix xml tags, based on the tag that is on top of the blockstack */
	guint8 comment_block;		/* block comment index in array scantable->comments
								   or COMMENT_INDEX_INHERIT (which means inherit)
								   or COMMENT_INDEX_NONE if there is no block comment  */
	guint8 comment_line;		/* index in array scantable->comments for line comments; see comment_block */
	guint8 autocomplete_case_insens;
	guint8 default_spellcheck;
} Tcontext;

typedef struct {
	gchar *autocomplete_string;
	guint8 autocomplete_backup_cursor;	/* number of characters to backup the cursor after autocompletion (max 256) */
	/* guint8 autocomplete;whether or not this pattern should be added to the autocompletion; stored in the Tpattern so we can re-use it in another context */
} Tpattern_autocomplete;

typedef struct {
	gchar *name;
	GtkTextTag *tag;		/* if this pattern ends a context or a block, we can highlight
								   the region within the start and end pattern with this tag */
	gchar *highlight;		/* a string for the highlight corresponding to the  blocktag */
	gboolean foldable;
} Tpattern_block;

typedef struct {
	GtkTextTag *selftag;		/* the tag used to highlight this pattern */
	/*GtkTextTag *blocktag;*/		/* if this pattern ends a context or a block, we can highlight
								   the region within the start and end pattern with this tag */
	gchar *reference;			/* the reference data, or NULL. may be inserted in hash tables for multiple keys in multiple contexts */
	gchar *pattern;				/* the pattern itself. stored in the Tpattern so we can re-use it in another context */
	GSList *autocomp_items;
	gchar *selfhighlight;		/* a string with the highlight for this pattern. used when re-linking highlights and textstyles
								   if the user changed any of these in the preferences */
	/*gchar *blockhighlight;*/		/* a string for the highlight corresponding to the  blocktag */
	guint16 block;			/* this is 0 for most blocks, only blocks that need a tag have this set */
	gint16 blockstartpattern;	/* the number of the pattern that may start this block, or -1 to end the last started block */
	gint16 nextcontext;			/* 0, or if this pattern starts a new context the number of the context, or -1 or -2 etc.
								   to pop a context of the stack */
#ifdef IDENTSTORING
	guint8 identaction :2; /* 2 bits, first bit is add to jump hashtable, second bit is autocomplete */
#endif
	/* we use 1 bit integers here because these values are all booleans */
	guint8 starts_block :1;		/* wether or not this pattern may start a block */
	guint8 ends_block :1;			/* wether or not this pattern may end a block */
	guint8 tagclose_from_blockstack :1;	/* this is a generix xml close tag that needs the blockstack to autoclose */
	guint8 stretch_blockstart :1; /* the end of this match is the new end-of-blockstart, used for HTML/XML tags */
	guint8 case_insens :1;
	guint8 is_regex :1;
#ifdef IDENTSTORING
	guint8 identmode :1;
#endif							/* IDENTSTORING */
} Tpattern;
/*
32bit size = 5*32 + 3*16 + 1*2 + 8*1 = 217 + 7 padding = 28 bytes
64bit size = 5*64 + 3*16 + 1*2 + 8*1 = 361 + 7 padding = 48 bytes
*/

typedef struct {
	guint16 row[NUMSCANCHARS];	/* contains for each character the number of the next state
								   because we use a 16bit unsigned number we can support only 65535 states
								   at maximum!!!!!!! but we use half the size of the scanning table, which
								   hopefully helps to keep the scanning table in the L2 cache of the CPU */
	guint16 match;				/* 0 == no match, refers to the index number in array 'matches' */
} Ttablerow;					/* a row in the DFA, right now exactly 256 bytes */


#define character_is_symbol(st,context,c) (g_array_index((GArray *)g_array_index(st->contexts, Tcontext, context).table, Ttablerow, 1).row[c] != 1)

#define get_table(scantable, context) ((GArray *)g_array_index(scantable->contexts, Tcontext, context).table)

#define get_tablerow(scantable, context, curstate) (g_array_index(g_array_index(scantable->contexts, Tcontext, context).table, Ttablerow, curstate))

/*****************************************************************/
/* scanning the text and caching the results */
/*****************************************************************/
typedef struct {
	gpointer parentfblock;
	guint32 start1_o;
	guint32 end1_o;
	guint32 start2_o;
	guint32 end2_o;
	gint16 patternum;			/* which pattern (number of the array element in scantable->matches) */
	guint8 folded;
	guint8 foldable;			/* FALSE on a single line */
} Tfoundblock;					/* Once a start-of-block is found start1 and end1 are set
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
	gint16 context;				/* number of the element in scantable->contexts */
} Tfoundcontext;				/* Once a start-of-context is found start is set
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
	Tfoundcontext *fcontext;	/* if numcontextchange == 0 this points to the current active context
								   if numcontextchange > 0 this points to the pushed context, which also happens to be the current context
								   if numcontextchange < 0 this points to the top of the stack at this position, to get the current position
								   you'll have to pop N items (where N is -1 * numcontextchange). */
	Tfoundblock *fblock;		/* if numblockchange == 0 this points to the current active block
								   if numblockchange > 0 this points to the pushed block, which also happens to be the current block
								   if numblockchange < 0 this points to the top of the stack at this position, to get the current position
								   you'll have to pop N items (where N is -1 * numblockchange). */
	guint32 charoffset_o;
	gint16 numblockchange;		/* there are files that have > 127 pops in a single position
								   for example html files that don't close paragrahs or tablerows */
	gint8 numcontextchange;		/* 0 means no change, 1 means 1 push, -2 means 2 popped etc. */
} Tfound;						/*
								   on 64bit this type has size 8+8+4+2+1 + 1 padding = 24 bytes
								   on 32bit this type has size 4+4+4+2+1 + 1 padding = 16 bytes
								 */

#define IS_FOUNDMODE_CONTEXTPUSH(i)   (i->numcontextchange > 0)
#define IS_FOUNDMODE_CONTEXTPOP(i)   (i->numcontextchange < 0)
#define IS_FOUNDMODE_BLOCKPUSH(i)   (i->numblockchange ==1)
#define IS_FOUNDMODE_BLOCKPOP(i)   (i->numblockchange < 0)

#endif /* _BFTEXTVIEW2_PRIVATE_H_ */