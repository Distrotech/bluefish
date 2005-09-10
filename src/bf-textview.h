/* Bluefish HTML Editor
 *
 *     BfTextView - Bluefish text widget - header file
 * 
 *    Copyright (C) 2005 Oskar Świda swida@aragorn.pb.bialystok.pl
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#ifndef __BF_TEXTVIEW_H__
#define __BF_TEXTVIEW_H__

/**
*	BfLangBlock:
*	@id: block identifier
*	@begin: string which begins this block
*	@end: string which ends this block
*	@group: block group
*	@case_sensitive: are begin and end strings case sensitive
*	@scanned: should we scan text inside the block
*	@foldable: can we fold this block
*	@markup: should we scan markup tags inside block
*
* Represents language block definition read from configuration file. 
*/
typedef struct {
   gchar *id, *begin, *end;
   gchar *group;
   gboolean case_sensitive;
   gboolean scanned;
   gboolean foldable;
   gboolean markup;
   gboolean regexp;
   /*< private >*/      
   gint tabid;
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
typedef struct {
   gchar *text;
   gchar *name;
   gchar *group;
   BfLangBlock *context;
   gboolean regexp;
   /*< private >*/   
   gint id;   
} BfLangToken;

typedef struct {
   /*< private >*/   
   gchar *name, *extensions, *description;
   gboolean case_sensitive;
   gboolean scan_tags;
   gboolean scan_blocks;
   gboolean restricted_tags_only;
   gboolean indent_blocks;
   gunichar as_triggers[256];

   GHashTable *tokens;
   GHashTable *blocks;
   GHashTable *groups;
   GHashTable *blocks_id;
   GHashTable *dfa;
   GList *dfa_tables;
   GHashTable *block_begin_dfa;
   GHashTable *block_end_dfa;
   GArray *line_indent;
   gint **scan_table;
   gint **scan_bb_table;
   gint **scan_be_table;
   gint **scan_tag_table;
   gint tabnum;
	gint bb_tabnum;
	gint be_tabnum;  
	gchar escapes[256]; 
	GHashTable *restricted_tags;
	gint tokennum; /* = BFTV_TOKEN_IDS + 1;*/
	gint blocknum; /* = BFTV_BLOCK_IDS + 1;*/
	BfLangBlock *iblock;
} BfLangConfig;

typedef struct {
	GHashTable *languages; /* table of recognized languages - structures BfLangConfig */
} BfLangManager;

BfLangManager *bf_lang_mgr_new();
gboolean bf_lang_mgr_load_config_list(BfLangManager *mgr,GList *list,gchar *filetype);
gboolean bf_lang_mgr_load_config(BfLangManager *mgr,gchar *filename,gchar *filetype);
BfLangConfig *bf_lang_mgr_get_config(BfLangManager *mgr,gchar *filetype);

typedef struct {
   /*< private >*/      
   BfLangToken *def;
   GtkTextIter itstart, itend;
} TBfToken;

typedef struct {
   /*< private >*/      
   BfLangBlock *def;
   GtkTextIter b_start, b_end;
   GtkTextIter e_start, e_end;
   GtkTextMark *mark_begin, *mark_end;
   gboolean single_line;
   gint b_len,e_len;   
} TBfBlock;

typedef struct {
   /*< private >*/      
	gchar *name;
   GtkTextIter b_start, b_end;
   GtkTextIter e_start, e_end;
} TBfTag;


typedef struct {
   /*< private >*/      
	gchar *name,*value;
   GtkTextIter name_start, name_end;
   GtkTextIter value_start, value_end;
} TBfTagAttr;



enum {
   BFTV_STATE_NORMAL,
   BFTV_STATE_TOKEN,
   BFTV_STATE_DONT_SCAN
};

typedef struct {
   /*< private >*/   
   TBfToken current_token;
   TBfBlock *current_block;
   TBfTag current_tag;
   TBfTagAttr current_attr;
   gint state;
   GQueue block_stack;
   GQueue tag_stack;
} TBfScanner;



#define BF_TYPE_TEXTVIEW	(bf_textview_get_type())
#define BF_TEXTVIEW(obj)	G_TYPE_CHECK_INSTANCE_CAST((obj), bf_textview_get_type(), BfTextView)
#define BF_TEXTVIEW_CONST(obj)	G_TYPE_CHECK_INSTANCE_CAST((obj), bf_textview_get_type(), BfTextView const)
#define BF_TEXTVIEW_CLASS(klass)	G_TYPE_CHECK_CLASS_CAST((klass), bf_textview_get_type(), BfTextViewClass)
#define BF_IS_TEXTVIEW(obj)	G_TYPE_CHECK_INSTANCE_TYPE((obj), bf_textview_get_type ())
#define BF_TEXTVIEW_GET_CLASS(obj)	G_TYPE_INSTANCE_GET_CLASS((obj), bf_textview_get_type(), BfTextViewClass)


typedef struct {
   /*< private >*/      
   gchar *name;
   GdkPixmap *pixmap;
} BfTextViewSymbol;

/*
 * Main object structure
 */
/*#ifndef __TYPEDEF_BF_TEXTVIEW__
#define __TYPEDEF_BF_TEXTVIEW__
typedef struct _BfTextView BfTextView;
#endif*/

typedef struct  {
   GtkTextView __parent__;
   GdkColor block_color;
   GdkColor bkg_color, fg_color;   
   gboolean show_lines;
   gboolean show_blocks;
   gboolean show_symbols;
   gboolean autoscan;
   gboolean color_blocks;
   gint autoscan_lines;
   /*< private >*/   
   gint lw_size_lines, lw_size_blocks, lw_size_sym;   
   GHashTable *symbols;
   GHashTable *symbol_lines;
   GHashTable *languages;
   BfLangConfig *current_lang;
   BfLangBlock *current_context;
   TBfScanner scanner;
   GtkTextTag *folded_tag, *block_tag, *fold_header_tag;
   GtkWidget *fold_menu;
   GList *hltags;
} BfTextView;

/*
 * Class definition
 */
/*typedef struct _BfTextViewClass BfTextViewClass;*/
typedef struct  {
   GtkTextViewClass __parent__;
   /*signal */ 
   void (*token) (BfTextView * self, BfLangToken * tokenDef, GtkTextIter * startIter,
			      GtkTextIter * endIter);
   /*signal */ 
   void (*tag_begin) (BfTextView * self, gchar * tagName, GtkTextIter * startIter,
				  GtkTextIter * endIter);
   /*signal */ 
   void (*tag_attr) (BfTextView * self, gchar * attrName, gchar * attrValue,
				 GtkTextIter * n_startIter, GtkTextIter * n_endIter,
				 GtkTextIter * v_startIter, GtkTextIter * v_endIter);
   /*signal */ 
   void (*tag_end) (BfTextView * self, gchar * tagName,
				GtkTextIter * b_startIter, GtkTextIter * b_endIter,
				GtkTextIter * e_startIter, GtkTextIter * e_endIter);
   /*signal */ 
   void (*tag_misplaced_end) (BfTextView * self, gchar * tagName,
					  GtkTextIter * startIter, GtkTextIter * endIter);
   /*signal */ void (*block_begin) (BfTextView * self, BfLangBlock * blockDef,
				    GtkTextIter * startIter, GtkTextIter * endIter);
   /*signal */ void (*block_end) (BfTextView * self, BfLangBlock * blockDef,
				  GtkTextIter * b_startIter, GtkTextIter * b_endIter,
				  GtkTextIter * e_startIter, GtkTextIter * e_endIter,
				  GtkTextMark * startMark, GtkTextMark * endMark);
   /*signal */ void (*block_misplaced_end) (BfTextView * self, BfLangBlock * blockDef,
					    GtkTextIter * startIter, GtkTextIter * endIter);
} BfTextViewClass;

 /* Public methods  */
GType bf_textview_get_type (void);

/* These are for signals */
void bf_textview_tag_begin (BfTextView * self, gchar * tagName, GtkTextIter * startIter, GtkTextIter * endIter);
void bf_textview_tag_end (BfTextView * self, gchar * tagName, GtkTextIter * b_startIter,
			  GtkTextIter * b_endIter, GtkTextIter * e_startIter, GtkTextIter * e_endIter);
void bf_textview_tag_misplaced_end (BfTextView * self, gchar * tagName, GtkTextIter * startIter, GtkTextIter * endIter);
void bf_textview_tag_attr (BfTextView * self, gchar * attrName, gchar * attrValue,
			   GtkTextIter * n_startIter, GtkTextIter * n_endIter,
			   GtkTextIter * v_startIter, GtkTextIter * v_endIter);
void bf_textview_token (BfTextView * self, BfLangToken * tokenDef, GtkTextIter * startIter, GtkTextIter * endIter);
void bf_textview_block_begin (BfTextView * self, BfLangBlock * blockDef, GtkTextIter * startIter,
			      GtkTextIter * endIter);
void bf_textview_block_end (BfTextView * self, BfLangBlock * blockDef, GtkTextIter * b_startIter,
			    GtkTextIter * b_endIter, GtkTextIter * e_startIter,
			    GtkTextIter * e_endIter, GtkTextMark * startMark, GtkTextMark * endMark);
void bf_textview_block_misplaced_end (BfTextView * self, BfLangBlock * blockDef,
				      GtkTextIter * startIter, GtkTextIter * endIter);

/* the rest */
gboolean bf_textview_add_language (BfTextView * self, gchar * langname, gchar * filename);
void bf_textview_remove_language (BfTextView * self, gchar * langname);
void bf_textview_set_language (BfTextView * self, gchar * langname);
void bf_textview_set_language_ptr (BfTextView * self, BfLangConfig *cfg);

void bf_textview_scan (BfTextView * self);
void bf_textview_scan_area (BfTextView * self, GtkTextIter * start, GtkTextIter * end);
void bf_textview_scan_lines (BfTextView * self, gint start, gint end);

GtkWidget *bf_textview_new (void);
GtkWidget *bf_textview_new_with_buffer (GtkTextBuffer * buffer);

void bf_textview_show_lines (BfTextView * self, gboolean show);

void bf_textview_show_symbols (BfTextView * self, gboolean show);
gboolean bf_textview_add_symbol (BfTextView * self, gchar * name, GdkPixmap * pix);
void bf_textview_remove_symbol (BfTextView * self, gchar * name);
void bf_textview_set_symbol (BfTextView * self, gchar * name, gint line, gboolean set);

void bf_textview_set_autoscan (BfTextView * self, gboolean setauto);
void bf_textview_set_autoscan_lines (BfTextView * self, gint lines);

void bf_textview_show_blocks (BfTextView * self, gboolean show);
void bf_textview_color_blocks (BfTextView * self, gboolean color);
void bf_textview_set_blocks_color (BfTextView * self, gchar * color);
void bf_textview_fold_blocks (BfTextView * self, gboolean fold);
void bf_textview_fold_blocks_area (BfTextView * self, GtkTextIter * start, GtkTextIter * end, gboolean fold);
void bf_textview_fold_blocks_lines (BfTextView * self, gint start, gint end, gboolean fold);

#define BF_GNB_CHAR				0
#define BF_GNB_LINE					1
#define BF_GNB_DOCUMENT	2
TBfBlock *bf_textview_get_nearest_block (BfTextView * self,GtkTextIter *iter,
																					gboolean backward, gint mode, gboolean not_single);

void bf_textview_set_bg_color (BfTextView * self, gchar * color);
void bf_textview_set_fg_color (BfTextView * self, gchar * color);

void bf_textview_register_hltag(BfTextView *self,GtkTextTag *tag);
void bf_textview_unregister_hltag(BfTextView *self,GtkTextTag *tag);
void bf_textview_clear_hltags(BfTextView *self);

#endif
