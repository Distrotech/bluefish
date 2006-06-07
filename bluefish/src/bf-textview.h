/* Bluefish HTML Editor
 * bf-textview.h - Bluefish text widget
 *
 * Copyright (C) 2005-2006  Oskar Świda swida@aragorn.pb.bialystok.pl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#ifndef __BF_TEXTVIEW_H__
#define __BF_TEXTVIEW_H__

#define BFTV_SCAN_RANGE	128

enum {
	ST_NONE,
	ST_TRANSIT,
	ST_TOKEN,
	ST_BLOCK_BEGIN,
	ST_BLOCK_END,
	ST_TAG_START_BEGIN,
	ST_TAG_START_END,
	ST_TAG_STOP,
	ST_ATTR_NAME,
	ST_ATTR_VALUE
};

typedef struct {
  guint8 type;		
  gpointer tv[BFTV_SCAN_RANGE];	
  gpointer data;		
} BfState;

/**
*	BfLangBlock:
*	@name: block identifier
*	@begin: string which begins this block
*	@end: string which ends this block
*	@group: block group
*	@regexp: is this regexp block
*	@scanned, foldable, cs,markup
*	@tag: tag used for highlighting		
*  @context: block context
* Represents language block definition read from configuration file. 
*/


enum {
	BT_NORMAL,
	BT_TAG_BEGIN
};

typedef struct {
	guchar *name, *begin, *end;
	guchar *group;
	gboolean regexp, scanned, foldable,cs,markup;
	guint8 type;
	GtkTextTag *tag;
	gpointer context;
	BfState scan_table;
} BfLangBlock;

/**
*	BfLangToken:
*	@text: token text
*	@name: optional token name - this is for tokens defined as regular expressions
*	@group: token group
*	@context: pointer to #BfLangBlock structure representing token context (if token context is different than NULL, 
*		this token will be recognized only inside that block).
*	@regexp: is this token regular expression
*
* Represents language token definition read from configuration file. 
*/

enum {	
	TT_NORMAL,
	TT_TAG_END,
	TT_ATTR,
	TT_ATTR2,
	TT_DOCTYPE,
	TT_XMLSCHEMA,
	TT_FAKE
}; 

typedef struct {
	guchar *text;
	guchar *name;
	guchar *group;
	BfLangBlock *context;
	gboolean regexp;
	guint8 type;
	GtkTextTag *tag;
} BfLangToken;

/**
*	BfLangConfig:
*
* Represents language configuration file. 
*/



typedef struct {
	guchar *name, *description;
	gboolean case_sensitive;
	gboolean scan_tags;
	gboolean scan_blocks;
	gboolean schema_aware;
	gunichar as_triggers[BFTV_SCAN_RANGE];
	gchar escapes[BFTV_SCAN_RANGE];
	GHashTable *tokens;
	GHashTable *blocks;
	GHashTable *groups;
	BfState scan_table;

	GtkTextTag *tag_begin;
	GtkTextTag *tag_end;
	GtkTextTag *attr_name;
	GtkTextTag *attr_val;
	GHashTable *dont_autoclose;
	
	gint num_states;
	GList *block_list;	
	gint max_token_length;
	
} BfLangConfig;

typedef struct {
	GHashTable *languages;		/* table of recognized languages - structures BfLangConfig */
} BfLangManager;

BfLangManager *bf_lang_mgr_new();
BfLangConfig *bf_lang_mgr_load_config(BfLangManager * mgr, const gchar * filename);
BfLangConfig *bf_lang_mgr_get_config(BfLangManager * mgr, const gchar * filename);

typedef struct {
	BfLangBlock *def;
	gchar *tagname; /* optionally for tags */
	GtkTextIter b_start, b_end;
	GtkTextIter e_start, e_end;
} TBfBlock;


typedef struct {
	BfLangBlock *current_context;
	GQueue block_stack;
	GQueue tag_stack;
	GString *last_string;	
	gchar *last_tagname;
} TBfScanner;



#define BF_TYPE_TEXTVIEW	(bf_textview_get_type())
#define BF_TEXTVIEW(obj)	G_TYPE_CHECK_INSTANCE_CAST((obj), bf_textview_get_type(), BfTextView)
#define BF_TEXTVIEW_CONST(obj)	G_TYPE_CHECK_INSTANCE_CAST((obj), bf_textview_get_type(), BfTextView const)
#define BF_TEXTVIEW_CLASS(klass)	G_TYPE_CHECK_CLASS_CAST((klass), bf_textview_get_type(), BfTextViewClass)
#define BF_IS_TEXTVIEW(obj)	G_TYPE_CHECK_INSTANCE_TYPE((obj), bf_textview_get_type ())
#define BF_TEXTVIEW_GET_CLASS(obj)	G_TYPE_INSTANCE_GET_CLASS((obj), bf_textview_get_type(), BfTextViewClass)


typedef struct {
	gchar *name;
	GdkPixbuf *pixmap;
} BfTextViewSymbol;

/*
 * Main object structure
 */


enum {
	BFTV_HL_MODE_ALL,
	BFTV_HL_MODE_VISIBLE
};

enum {
	IT_FOLDED,
	IT_FOLD_HEADER,
	IT_BLOCK_MATCH,
	IT_BLOCK,
	IT_NUM_OF_TAGS	
};

typedef struct {
	GtkTextView __parent__;
	gchar bkg_color[8], fg_color[8];
	gboolean show_lines;
	gboolean show_blocks;
	gboolean show_symbols;
	gboolean show_rmargin;
	gboolean highlight;			/* TRUE if highlighting should be performed */
	gboolean match_blocks;		/* TRUE if matching block begin/end should be shown */
	gboolean show_current_line;	/* TRUE if current line should be shown */
	gboolean tag_autoclose;           /* TRUE if tags should be automatically closed */
	gint hl_mode;				/* highlighting mode */
	gint rmargin_at;
	gint lw_size_lines, lw_size_blocks, lw_size_sym;
	GHashTable *symbols;
	GHashTable *symbol_lines;
	BfLangConfig *lang;
	TBfScanner scanner;
	GtkTextTag *internal_tags[IT_NUM_OF_TAGS];
	GtkWidget *fold_menu;
	gboolean need_rescan; /* this is set to true if the buffer is changed, but the
						 widget is not visible to the user, the scanning is then postponed until 
						 the widget is visible to the user */
	gulong insert_signal_id; /* needed for blocking */
	gulong markset_signal_id; /* needed for blocking */							 
	GHashTable *fbal_cache; /* first block at line - cache */
	GHashTable *lbal_cache; /* last block at line - cache */
	gboolean delete_rescan; /* indicates if rescan is done from delete operation */
	GtkTextMark *last_matched_block; /* last matched block */
	gboolean paste_operation; /* indicates if we perform paste */
	gboolean tag_ac_state; /* TRUE if we should perform tag autoclosing - i.e. after inserting '>' sign only */
	GList *schemas;
} BfTextView;

/*
 * Class definition
 */

typedef struct {
	GtkTextViewClass __parent__;
} BfTextViewClass;

 /* Public methods  */
GType bf_textview_get_type(void);

void bf_textview_set_language_ptr(BfTextView * self, BfLangConfig * cfg);

void bf_textview_scan(BfTextView * self);
void bf_textview_scan_area(BfTextView * self, GtkTextIter * start, GtkTextIter * end, gboolean apply_hl);
void bf_textview_scan_visible(BfTextView * self);

GtkWidget *bf_textview_new(void);
GtkWidget *bf_textview_new_with_buffer(GtkTextBuffer * buffer);

void bf_textview_set_show_lines(BfTextView * self, gboolean show);
void bf_textview_set_show_symbols(BfTextView * self, gboolean show);
void bf_textview_set_show_rmargin(BfTextView * self, gboolean show, gint column);
void bf_textview_set_show_blocks(BfTextView * self, gboolean show);
void bf_textview_set_highlight(BfTextView * self, gboolean on);
void bf_textview_set_match_blocks(BfTextView * self, gboolean on);

gboolean bf_textview_add_symbol(BfTextView * self, gchar * name, GdkPixbuf * pix);
void bf_textview_remove_symbol(BfTextView * self, gchar * name);
void bf_textview_set_symbol(BfTextView * self, gchar * name, gint line, gboolean set);


void bf_textview_fold_blocks(BfTextView * self, gboolean fold);
void bf_textview_fold_blocks_area(BfTextView * self, GtkTextIter * start, GtkTextIter * end,
								  gboolean fold);


enum {
	BF_GNB_CHAR,
	BF_GNB_LINE,
	BF_GNB_DOC,
	BF_GNB_HERE
};

GtkTextMark *bf_textview_get_nearest_block(BfTextView * self, GtkTextIter * iter, gboolean backward,
										   gint mode, gboolean not_single);
GtkTextMark *bf_textview_get_nearest_block_of_type(BfTextView * self, BfLangBlock * block,
												   GtkTextIter * iter, gboolean backward, gint mode,
												   gboolean not_single);


void bf_textview_recolor(BfTextView *view, gchar *fg_color, gchar *bg_color );

/* these functions return the names of groups, blocks and tokens that are defined 
for a certain language config. They are used in the preferences panel to build
the GUI to set textstyles */
GList *bf_lang_get_groups(BfLangConfig * cfg);
GList *bf_lang_get_blocks_for_group(BfLangConfig * cfg, guchar * group);
GList *bf_lang_get_tokens_for_group(BfLangConfig * cfg, guchar * group);
gboolean bf_lang_needs_tags(BfLangConfig * cfg);
void bf_lang_mgr_retag(void);

void bf_textview_autocomp_show(BfTextView *self); 

/* For backward compatibility only - will be removed */
void bf_textview_show_lines(BfTextView * self, gboolean show);
void bf_textview_show_symbols(BfTextView * self, gboolean show);
void bf_textview_show_rmargin(BfTextView * self, gboolean show, gint column);
void bf_textview_show_blocks(BfTextView * self, gboolean show);


#endif
