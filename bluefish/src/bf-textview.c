/* Bluefish HTML Editor
 * bf-textview.c - Bluefish text widget
 * 
 * Copyright (C) 2005-2006  Oskar Świda swida@aragorn.pb.bialystok.pl
 * Copyright (C) 2007-2008 Olivier Sessink  
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
 *
 * indenting is done with
 * indent --line-length 100 --k-and-r-style --tab-size 4 -bbo --ignore-newlines bf-textview.c
 */
#define REUSETAGS /* REUSETAGS does not remove texttags from the widget and applying 
							them again if the text was not changed - instead it removes only
							those tags that are not in use anymore, and adds only those tags
							that are not yet set */
#define SKIP_KNOWN_UNCHANGED_BLOCKS /* SKIP_KNOWN_UNCHANGED_BLOCKS tests when a block 
							start is found if a previous scan found a block end and if the block
							end is within the unchanged part of the text, it will skip that entire 
							block and continue scanning after that block*/
/*#define SCANALLTAGVIEWABLE*/ /* SCANALLTAGVIEWABLE was a test to see if it improved speed
							to scan all text but to tag only in the visible area. the performance
							gain is not very high... */
/*#define DEBUG */
/* #define DEBUGSC */ /* DEBUGSC gives specific debugging on internal scanner working PER CHARACTER,
							so this is A LOT OF OUTPUT */
/* #define DEBUGTAGS */ /* DEBUGTAGS gives info about setting and removing tags  */
/*#define HL_PROFILING*/

/*
Typical scanner in compiler is an automata. To implement automata you
need a TABLE. Such a table
is an implementation of function:

    ( state , character ) ------>  next state

where "next state" can be simply next automata state or "finishing
state" which means we are accepting some word.
For example: we want to build scanner recognizing three words "some",
"somebody" and "any"

Sample table

|state|| space| a | b | d | e | m | n | o | s | y |
|  0  ||      | 1 |   |   |   |   |   |   | 3 |   |
|  1  ||      |   |   |   |   |   | 2 |   |   |   |
|  2  ||      |   |   |   |   |   |   |   |   |   |
|  3  ||      |   |   |   |   |   |   | 4 |   |   |
|  4  ||      |   |   |   |   | 5 |   |   |   |   |
|  5  ||      |   |   |   | 6 |   |   |   |   |   |
|  6  ||   *  | * | 7 | * | * | * | * | * | * | * |
|  7  ||      |   |   |   |   |   |   | 8 |   |   |
|  8  ||      |   |   | 9 |   |   |   |   |   |   |
|  9  ||      |   |   |   |   |   |   |   |   | # |

* = 'some'
# = 'somebody'

Let's analyze: given a phrase "Does any some somebody " we can start
scanner.
First state is 0, so:
  (0,D)-> 0 , (0,o)->0, (0,e)->0, (0,s)->3  but (3,space)->0  again
then:
  (0,a)->1 , (1,n)->2, (2,y)->word "any" found

And so on. As you see the case of "some" and "somebody" is different
because these words have the same prefix, so
when in state 5 I have "e" character I'm going to state 6 where the
decision is made. If next character is "b" it means it
could be "somebody", everything else means we have found word "some".

In BfTextView I'm building such table from configuration file (it is a
bit more complex because of regular expressions) and
that is all what I need. Next if I want find any tokens in editor text I
simply set scanner state to 0 and start to iterate through
characters, changing scanner states according the table. If I find
"finishing state" I'm marking a token.

*** Context ***

the situation becomes a little more complex when we take "context" into consideration. 
Within a PHP block for example, we use a different state table. The context is stored 
in self->scanner.current_context which is of type BfLangBlock (wich has a scantable).

*** Blocks ***
when scanning for blocks, a TBfBlock is pushed on the stack scanner.block_stack, so when 
the end of the block is found, this can be related to the start.

once a block is found there are 4 GtkTextMark's added:
1) the start of the block-start-string
2) the end of the  block-start-string
3) the start of the block-end-string
4) the end of the block-end-string
All of these marks will have data "bi" set (with g_object_set_data), and this data is a 
BlockInfo pointer. There are two BlockInfo pointers, one for the end of the block, and for 
the start of the block !!IDEA: can we use just a single BlockInfo pointer ??!!!

 

*** POSSIBLE IMPROVEMENTS ***

currently always the visible area is re-scanned if certain characters are entered.. Much 
better would be to:
1) if we're inside a block that is not scanned (for example a multiline comment): don't scan 
anything except if end-of-block is inserted
2) if we're inside a block that is scanned, just scan for tokens that have to be scanned in 
that context, start at the beginning of that block (anything before that should be alright already), 
and just scan if the end of the block has been just inserted
3) if we're not in any block, start at the end of the last block or token that has been found, continue
til the last just entered character 
*/

#ifdef DEBUGSC
#define DEBUGSC_MSG g_print
#else /* not DEBUGSC */
#ifdef __GNUC__
#define DEBUGSC_MSG(args...)
 /**/
#else/* notdef __GNUC__ */
extern void g_none(gchar *first, ...);
#define DEBUGSC_MSG g_none
#endif /* __GNUC__ */
#endif /* DEBUGSC */

#ifdef DEBUGTAGS
#define DEBUGTAGS_MSG g_print
#else /* not DEBUGTAGS */
#ifdef __GNUC__
#define DEBUGTAGS_MSG(args...)
 /**/
#else/* notdef __GNUC__ */
extern void g_none(gchar *first, ...);
#define DEBUGTAGS_MSG g_none
#endif /* __GNUC__ */
#endif /* DEBUGTAGS */



#include "config.h"

#include "bf-textview.h"
#include "bluefish.h"
#include "textstyle.h"
#include "bf_lib.h"
#include "gtk_easy.h"			/* gdk_color_to_hexstring */

#include <ctype.h>

#include <stdarg.h>
#include <string.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/hash.h>
#include <gdk/gdkkeysyms.h>

#ifdef HL_PROFILING				/* scanning profiling information, interesting for users making a new pattern */
#include <sys/times.h>
#include <unistd.h>
#endif

#ifdef HL_PROFILING
struct tms tms1;
struct tms tms1a;
struct tms tms1b;
struct tms tms1c;
struct tms tms2;
struct tms tms3;
struct tms tms4;
gint numskipped;
#endif							/* HL_PROFILING */

enum {
	BI_START,
	BI_END
};

typedef struct {
	gboolean folded; /* if the block is currently folded */
	gboolean single_line; /* if the block is currently started and ended on the same line (cannot be folded) */
	gpointer data; /* BfLangBlock pointer */
	gchar *tagname; /* for XML/HTML tags */
	GtkTextMark *blockstart_s; /* mark at the begin of the begin-block-string  */
	GtkTextMark *blockstart_e; /* mark at the end of the begin-block-string  */
	GtkTextMark *blockend_s; /* mark at the begin of the end-block-string  */
	GtkTextMark *blockend_e; /* mark at the end of the end-block-string  */
} BlockInfo2;


static char folded_xbm[] = {
	0x02, 0x01
};

static gshort const_true = 1;

static GtkTextViewClass *parent_class = NULL;


/* internal functions */
static gboolean bftv_xml_bool(gchar* text);
static gboolean bf_textview_expose_cb(GtkWidget * widget, GdkEventExpose * event, gpointer doc);
static void bf_textview_insert_text_cb(GtkTextBuffer * textbuffer, GtkTextIter * iter,
									   gchar * string, gint stringlen, BfTextView * view);
static void bf_textview_delete_range_cb(GtkTextBuffer * textbuffer, GtkTextIter * arg1,
										GtkTextIter * arg2, gpointer user_data);
static void bf_textview_delete_range_after_cb(GtkTextBuffer * textbuffer, GtkTextIter * arg1,
											  GtkTextIter * arg2, gpointer user_data);
static void bf_textview_mark_set_cb(GtkTextBuffer * buf, GtkTextIter * arg1, GtkTextMark * arg2,
									gpointer widget);
static gboolean bf_textview_mouse_cb(GtkWidget * widget, GdkEvent * event, gpointer user_data);
inline GtkTextMark *bftv_get_block_at_iter(GtkTextIter * it);
static GtkTextMark *bftv_get_first_block_at_line(BfTextView * view, GtkTextIter * it,
												 gboolean not_single);
/*static GtkTextMark *bftv_get_last_block_at_line(BfTextView *view, GtkTextIter * it);*/
static void bftv_delete_blocks_from_area(BfTextView * view, GtkTextIter * arg1, GtkTextIter * arg2, gboolean remove_tags);
static void bftv_fold(BfTextView * self, GtkTextMark * mark, gboolean move_cursor);
static void bftv_expand_all(GtkWidget * widget, BfTextView * view);
static void bftv_collapse_all(GtkWidget * widget, BfTextView * view);
static void bftv_clear_block_cache(BfTextView * self);
static void bftv_clear_matched_block(BfTextView * self);
static void bf_textview_scan_area(BfTextView * self, GtkTextIter * start, GtkTextIter * end, gboolean apply_hl, gboolean store_string, gboolean force_rescan);

/* internal functions */

static void bf_textview_class_init(BfTextViewClass * c)
{
	parent_class = g_type_class_ref(GTK_TYPE_TEXT_VIEW);
}

static void bf_textview_init(BfTextView * o)
{
	GtkWidget *item;

	o->lw_size_lines = o->lw_size_blocks = o->lw_size_sym = 0;
	o->symbols = g_hash_table_new(g_str_hash, g_str_equal);
	o->symbol_lines = g_hash_table_new(g_direct_hash, g_direct_equal);
	o->lang = NULL;
	g_stpcpy(o->bkg_color, "#FFFFFF");
	g_stpcpy(o->fg_color, "#000000");
	o->fold_menu = gtk_menu_new();
	item = gtk_menu_item_new_with_label(_("Expand all"));
	gtk_menu_append(GTK_MENU(o->fold_menu), item);
	g_signal_connect(GTK_OBJECT(item), "activate", G_CALLBACK(bftv_expand_all), o);
	item = gtk_menu_item_new_with_label(_("Collapse all"));
	gtk_menu_append(GTK_MENU(o->fold_menu), item);
	gtk_widget_show_all(o->fold_menu);
	g_signal_connect(GTK_OBJECT(item), "activate", G_CALLBACK(bftv_collapse_all), o);
	o->highlight = TRUE;
	o->match_blocks = TRUE;
	o->paste_operation = FALSE;
	o->hl_mode = BFTV_HL_MODE_ALL;
	o->show_current_line = main_v->props.view_cline;
	o->tag_autoclose = main_v->props.insert_close_tag;
	o->fbal_cache = g_hash_table_new(g_int_hash, g_int_equal);
	o->lbal_cache = g_hash_table_new(g_int_hash, g_int_equal);
	o->last_matched_block = NULL;
	o->show_rmargin = FALSE;
	o->rmargin_at = 0;
	o->tag_ac_state = FALSE;
	o->scanner.last_string = g_string_new("");
	o->schemas = NULL;
	o->internal_dtd = NULL;

}

GType bf_textview_get_type(void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo info = {
			sizeof(BfTextViewClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) bf_textview_class_init,
			(GClassFinalizeFunc) NULL,
			NULL /* class_data */ ,
			sizeof(BfTextView),
			0 /* n_preallocs */ ,
			(GInstanceInitFunc) bf_textview_init,
			NULL
		};
		type = g_type_register_static(GTK_TYPE_TEXT_VIEW, "BfTextView", &info, (GTypeFlags) 0);
	}
	return type;
}

#define GET_NEW ((BfTextView *)g_object_new(bf_textview_get_type(), NULL))


/**
*	bf_textview_new:
*	Create new BfTextView widget.
* Returns: new BfTextView widget.
*/
GtkWidget *bf_textview_new(void)
{
	BfTextView *o = GET_NEW;
	/*GdkBitmap *bmp; */

	g_signal_connect(G_OBJECT(o), "expose-event", G_CALLBACK(bf_textview_expose_cb), NULL);
	o->markset_signal_id =
		g_signal_connect_after(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "mark-set",
							   G_CALLBACK(bf_textview_mark_set_cb), o);
	o->insert_signal_id =
		g_signal_connect_after(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "insert-text",
							   G_CALLBACK(bf_textview_insert_text_cb), o);
	g_signal_connect(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "delete-range",
					 G_CALLBACK(bf_textview_delete_range_cb), o);
	g_signal_connect_after(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "delete-range",
						   G_CALLBACK(bf_textview_delete_range_after_cb), o);
	g_signal_connect(G_OBJECT(o), "button-press-event", G_CALLBACK(bf_textview_mouse_cb), NULL);
	bf_textview_recolor(o, "#000000", "#FFFFFF");
	return (GtkWidget *) o;
}

/**
*	bf_textview_new_with_buffer:
*	@buffer
*
*	Create new BfTextView widget setting given buffer.
* Returns: new BfTextView widget.
*/
GtkWidget *bf_textview_new_with_buffer(GtkTextBuffer * buffer)
{
	BfTextView *o = GET_NEW;
	/*GdkBitmap *bmp; */
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(o), buffer);

	g_signal_connect(G_OBJECT(o), "expose-event", G_CALLBACK(bf_textview_expose_cb), NULL);
	o->markset_signal_id =
		g_signal_connect_after(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "mark-set",
							   G_CALLBACK(bf_textview_mark_set_cb), o);
	o->insert_signal_id =
		g_signal_connect_after(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "insert-text",
							   G_CALLBACK(bf_textview_insert_text_cb), o);
	g_signal_connect(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "delete-range",
					 G_CALLBACK(bf_textview_delete_range_cb), o);
	g_signal_connect_after(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "delete-range",
						   G_CALLBACK(bf_textview_delete_range_after_cb), o);
	g_signal_connect(G_OBJECT(o), "button-press-event", G_CALLBACK(bf_textview_mouse_cb), NULL);
	bf_textview_recolor(o, "#000000", "#FFFFFF");
	return (GtkWidget *) o;
}

/* ---------------------    EVENTS ---------------------------- */
static gboolean bf_textview_mouse_cb(GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
	GdkWindow *win = gtk_text_view_get_window(GTK_TEXT_VIEW(widget),
											  GTK_TEXT_WINDOW_LEFT);
	gint x, y;
	GtkTextIter it;
	GtkTextMark *block_mark = NULL;
	gint pt_lines, pt_sym, pt_blocks;
	DEBUG_MSG("bf_textview_mouse_cb, started\n");

	if (win != event->button.window)
		return FALSE;

	pt_lines = 2;
	if (BF_TEXTVIEW(widget)->show_lines)
		pt_sym = BF_TEXTVIEW(widget)->lw_size_lines + 2;
	else
		pt_sym = 2;
	pt_blocks = 2;
	if (BF_TEXTVIEW(widget)->show_lines)
		pt_blocks = BF_TEXTVIEW(widget)->lw_size_lines + pt_blocks;
	if (BF_TEXTVIEW(widget)->show_symbols)
		pt_blocks = BF_TEXTVIEW(widget)->lw_size_sym + pt_blocks;

	if (event->button.button == 1) {
		if (event->button.x >= pt_blocks) {
			gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT, 0,
												  event->button.y, &x, &y);
			gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(widget), &it, y, &x);
			block_mark = bftv_get_first_block_at_line(BF_TEXTVIEW(widget), &it, TRUE);
			if (block_mark)
				bftv_fold(BF_TEXTVIEW(widget), block_mark, TRUE);
			return TRUE;
		}
	} else if (event->button.button == 3) {
		if (event->button.x >= pt_blocks) {
			gtk_menu_popup(GTK_MENU(BF_TEXTVIEW(widget)->fold_menu), NULL, NULL, NULL, NULL, 1,
						   gtk_get_current_event_time());
			return TRUE;
		}
	}
	return FALSE;
}

static void bf_textview_get_iters_at_blockinfo(GtkTextBuffer *buf, BlockInfo2 *bi2, GtkTextIter *it1, GtkTextIter *it2, GtkTextIter *it3, GtkTextIter *it4) {
	gtk_text_buffer_get_iter_at_mark(buf, it1, bi2->blockstart_s);
	gtk_text_buffer_get_iter_at_mark(buf, it2, bi2->blockstart_e);
	gtk_text_buffer_get_iter_at_mark(buf, it3, bi2->blockend_s);
	gtk_text_buffer_get_iter_at_mark(buf, it4, bi2->blockend_e);	
}


static void bf_textview_mark_set_cb(GtkTextBuffer * buf, GtkTextIter * location, GtkTextMark * arg2,
									gpointer widget)
{
	GtkTextIter it1, it2;
	
	GtkTextMark *block = NULL;

	if (arg2 && gtk_text_buffer_get_insert(buf) == arg2) {
		BlockInfo2 *bi2;
		GtkTextIter it3, it4;
		/* insert set which means cursor position changed */

		block = bftv_get_block_at_iter(location);
		DEBUG_MSG("bf_textview_mark_set_cb, got block %p, last_matched_block=%p\n",block,BF_TEXTVIEW(widget)->last_matched_block);

		if (BF_TEXTVIEW(widget)->last_matched_block) {
			/* remove the tags on the previously highlighted block */

			bi2 = (BlockInfo2 *) g_object_get_data(G_OBJECT(BF_TEXTVIEW(widget)->last_matched_block), "bi2");
			bf_textview_get_iters_at_blockinfo(buf, bi2, &it1, &it2, &it3, &it4);
			DEBUG_TEXTTAG_MSG("removing tag %p (%s:%d)\n",main_v->lang_mgr->internal_tags[IT_BLOCK_MATCH],__FILE__,__LINE__);
			gtk_text_buffer_remove_tag(buf, main_v->lang_mgr->internal_tags[IT_BLOCK_MATCH],&it1,&it2);
			gtk_text_buffer_remove_tag(buf, main_v->lang_mgr->internal_tags[IT_BLOCK_MATCH],&it3,&it4);
		}

		if (block) {
			bi2 = (BlockInfo2 *) g_object_get_data(G_OBJECT(block), "bi2");
			if (!bi2->folded) {
				bf_textview_get_iters_at_blockinfo(buf, bi2, &it1, &it2, &it3, &it4);
				DEBUG_TEXTTAG_MSG("apply tag %p (%s:%d)\n",main_v->lang_mgr->internal_tags[IT_BLOCK_MATCH],__FILE__,__LINE__);
				gtk_text_buffer_apply_tag(buf, main_v->lang_mgr->internal_tags[IT_BLOCK_MATCH], &it1,&it2);
				gtk_text_buffer_apply_tag(buf, main_v->lang_mgr->internal_tags[IT_BLOCK_MATCH], &it3,&it4);
			}
			BF_TEXTVIEW(widget)->last_matched_block = block;
		}
	} else if (arg2 && arg2 == gtk_text_buffer_get_selection_bound(buf)) {
		gtk_text_buffer_get_iter_at_mark(buf, &it1, gtk_text_buffer_get_insert(buf));
		gtk_text_buffer_get_iter_at_mark(buf, &it2, arg2);
		gtk_text_buffer_remove_tag(buf, main_v->lang_mgr->internal_tags[IT_BLOCK_MATCH], &it1, &it2);
	}
}

static gboolean bf_textview_expose_cb(GtkWidget * widget, GdkEventExpose * event, gpointer doc)
{
	GdkWindow *left_win;
	GdkRectangle rect;
	GtkTextIter l_start, l_end, it;
	GtkTextMark *block_mark;
	gint l_top1, numlines, w, l_top2, i, w2;
	PangoLayout *l;
	gchar *pomstr = NULL;
	gint pt_lines, pt_sym, pt_blocks, currline = 0;
	GdkGC *gc = NULL;
	gpointer aux;
	GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));


	DEBUG_MSG("bf_textview_expose_cb, started\n");
	if (BF_TEXTVIEW(widget)->need_rescan) {
		DEBUG_MSG("bf_textview_expose_cb, need rescan!\n");
		bf_textview_scan(BF_TEXTVIEW(widget));
	}

	left_win = gtk_text_view_get_window(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_LEFT);

	if (left_win != event->window) {
		if (event->window == gtk_text_view_get_window(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT)) {
			if (BF_TEXTVIEW(widget)->show_current_line) {
				/* current line highlighting - thanks gtksourceview team :) */
				gtk_text_buffer_get_iter_at_mark(buf, &it, gtk_text_buffer_get_insert(buf));
				gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(widget), &rect);
				gtk_text_view_get_line_yrange(GTK_TEXT_VIEW(widget), &it, &w, &w2);
				gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT,
													  rect.x, rect.y, &rect.x, &rect.y);
				gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT,
													  0, w, NULL, &w);
				gdk_draw_rectangle(event->window, widget->style->bg_gc[GTK_WIDGET_STATE(widget)],
								   TRUE, rect.x, w, rect.width, w2);
			}
			if (BF_TEXTVIEW(widget)->show_rmargin) {
				l = gtk_widget_create_pango_layout(widget, "x");
				pango_layout_get_pixel_size(l, &w, NULL);
				gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(widget), &rect);
				gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT,
													  rect.x, rect.y, NULL, &rect.y);
				gdk_draw_line(event->window, widget->style->bg_gc[GTK_WIDGET_STATE(widget)],
							  w * BF_TEXTVIEW(widget)->rmargin_at, rect.y,
							  w * BF_TEXTVIEW(widget)->rmargin_at, rect.y + rect.height);
				g_object_unref(G_OBJECT(l));
			}
		}
		return FALSE;
	}

	if (!BF_TEXTVIEW(widget)->show_lines && !BF_TEXTVIEW(widget)->show_symbols
		&& !BF_TEXTVIEW(widget)->show_blocks)
		return FALSE;


	gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(widget), &rect);
	gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(widget), &l_start, rect.y, &l_top1);
	gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(widget), &l_end, rect.y + rect.height, &l_top2);
	l = gtk_widget_create_pango_layout(widget, "");
	if (BF_TEXTVIEW(widget)->show_lines) {
		numlines = gtk_text_buffer_get_line_count(gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget)));
		pomstr = g_strdup_printf("%d", MAX(99, numlines));
		pango_layout_set_text(l, pomstr, -1);
		g_free(pomstr);
		pango_layout_get_pixel_size(l, &w, NULL);
		BF_TEXTVIEW(widget)->lw_size_lines = w + 4;
		gtk_text_buffer_get_iter_at_mark(buf, &it, gtk_text_buffer_get_insert(buf));
		currline = gtk_text_iter_get_line(&it);
	}
	gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_LEFT,
										 BF_TEXTVIEW(widget)->lw_size_lines +
										 BF_TEXTVIEW(widget)->lw_size_blocks +
										 BF_TEXTVIEW(widget)->lw_size_sym);
	pt_lines = 2;
	if (BF_TEXTVIEW(widget)->show_lines)
		pt_sym = BF_TEXTVIEW(widget)->lw_size_lines + 2;
	else
		pt_sym = 2;
	pt_blocks = 2;
	if (BF_TEXTVIEW(widget)->show_lines)
		pt_blocks = BF_TEXTVIEW(widget)->lw_size_lines + pt_blocks;
	if (BF_TEXTVIEW(widget)->show_symbols)
		pt_blocks = BF_TEXTVIEW(widget)->lw_size_sym + pt_blocks;

	it = l_start;
	for (i = gtk_text_iter_get_line(&l_start); i <= gtk_text_iter_get_line(&l_end); i++) {
		gtk_text_iter_set_line(&it, i);
		gtk_text_view_get_line_yrange(GTK_TEXT_VIEW(widget), &it, &w, NULL);
		gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_LEFT, 0, w,
											  NULL, &w);
		if (BF_TEXTVIEW(widget)->show_lines) {	/* show line numbers */
			/*DEBUG_MSG("checking for folded tag %p\n", BF_TEXTVIEW(widget)->folded_tag); */
			DEBUG_TEXTTAG_MSG("check for tag %p (%s:%d)\n",main_v->lang_mgr->internal_tags[IT_FOLDED],__FILE__,__LINE__);
			if (!gtk_text_iter_has_tag(&it, main_v->lang_mgr->internal_tags[IT_FOLDED])) {
				if (currline == i)
					pomstr = g_strdup_printf("<b>%d</b>", 1 + i);	/* line numbers should start at 1 */
				else
					pomstr = g_strdup_printf("%d", 1 + i);
				pango_layout_set_markup(l, pomstr, -1);
				gdk_draw_layout(GDK_DRAWABLE(left_win),
								widget->style->text_gc[GTK_WIDGET_STATE(widget)], pt_lines, w, l);
				g_free(pomstr);
			}
		}
		if (BF_TEXTVIEW(widget)->show_symbols) {	/* show symbols */
			DEBUG_TEXTTAG_MSG("check tag %p (%s:%d)\n",main_v->lang_mgr->internal_tags[IT_FOLDED],__FILE__,__LINE__);
			if (!gtk_text_iter_has_tag(&it, main_v->lang_mgr->internal_tags[IT_FOLDED])) {
				GSList *lst3;
				gc = gdk_gc_new(GDK_DRAWABLE(left_win));
				lst3 = gtk_text_iter_get_marks(&it);
				aux = NULL;
				while (lst3) {
					aux = g_hash_table_lookup(BF_TEXTVIEW(widget)->symbol_lines, lst3->data);
					if (aux)
						break;
					lst3 = g_slist_next(lst3);
				}
				g_slist_free(lst3);
				if (aux) {
					gdk_pixbuf_render_to_drawable(((BfTextViewSymbol *) aux)->pixmap,
												  GDK_DRAWABLE(left_win), gc, 0, 0, pt_sym, w + 2,
												  10, 10, GDK_RGB_DITHER_NORMAL, 0, 0);
				}
				g_object_unref(gc);
			}
		}
		if (BF_TEXTVIEW(widget)->show_blocks) {	/* show block markers */
			BlockInfo2 *bi=NULL;
			block_mark = bftv_get_first_block_at_line(BF_TEXTVIEW(widget), &it, TRUE);
			if (block_mark) {
				bi = (BlockInfo2 *) g_object_get_data(G_OBJECT(block_mark), "bi2");
			}
			DEBUG_TEXTTAG_MSG("check tag %p (%s:%d)\n",main_v->lang_mgr->internal_tags[IT_FOLDED],__FILE__,__LINE__);
			if (bi && !gtk_text_iter_has_tag(&it, main_v->lang_mgr->internal_tags[IT_FOLDED])) {

				gtk_text_view_get_line_yrange(GTK_TEXT_VIEW(widget), &it, &w2, NULL);
				gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_LEFT,
													  0, w2, NULL, &w2);
				if (block_mark == bi->blockstart_s) {
					gdk_draw_rectangle(GDK_DRAWABLE(left_win),
									   widget->style->base_gc[GTK_WIDGET_STATE(widget)], TRUE,
									   pt_blocks, w + 2, 10, 10);
					gdk_draw_rectangle(GDK_DRAWABLE(left_win),
									   widget->style->fg_gc[GTK_WIDGET_STATE(widget)], FALSE,
									   pt_blocks, w + 2, 10, 10);
					if (bi->folded) {
						gdk_draw_line(GDK_DRAWABLE(left_win),
									  widget->style->fg_gc[GTK_WIDGET_STATE(widget)], pt_blocks + 3,
									  w + 7, pt_blocks + 7, w + 7);
						gdk_draw_line(GDK_DRAWABLE(left_win),
									  widget->style->fg_gc[GTK_WIDGET_STATE(widget)], pt_blocks + 5,
									  w + 5, pt_blocks + 5, w + 9);
					} else
						gdk_draw_line(GDK_DRAWABLE(left_win),
									  widget->style->fg_gc[GTK_WIDGET_STATE(widget)], pt_blocks + 3,
									  w + 7, pt_blocks + 7, w + 7);
				} else {		/* block end */
					gdk_draw_line(GDK_DRAWABLE(left_win),
								  widget->style->fg_gc[GTK_WIDGET_STATE(widget)], pt_blocks + 5, w,
								  pt_blocks + 5, w + 6);
					gdk_draw_line(GDK_DRAWABLE(left_win),
								  widget->style->fg_gc[GTK_WIDGET_STATE(widget)], pt_blocks + 5,
								  w + 6, pt_blocks + 10, w + 6);
				}
			} else {			/* not block begin or end, but perhaps inside */
				/*DEBUG_MSG("checking if we're in tag %p\n",
						  main_v->lang_mgr->internal_tags[IT_BLOCK]);*/
				DEBUG_TEXTTAG_MSG("check tag %p (%s:%d)\n",main_v->lang_mgr->internal_tags[IT_BLOCK],__FILE__,__LINE__);
				if (gtk_text_iter_has_tag(&it, main_v->lang_mgr->internal_tags[IT_BLOCK])
					&& !gtk_text_iter_has_tag(&it, main_v->lang_mgr->internal_tags[IT_FOLDED])) {
					gdk_draw_line(GDK_DRAWABLE(left_win),
								  widget->style->fg_gc[GTK_WIDGET_STATE(widget)], pt_blocks + 5, w,
								  pt_blocks + 5, w + 16);
				}
			}
		}
		/* block markers */
	}							/* end of lines loop */
	g_object_unref(G_OBJECT(l));
	return TRUE;
}

static void bf_textview_insert_text_cb(GtkTextBuffer * textbuffer, GtkTextIter * iter,
									   gchar * string, gint stringlen, BfTextView * view)
{
	gint len;
	gboolean trigger = FALSE;
	gchar *p = string;

	DEBUG_MSG("bf_textview_insert_text_cb, started, string=\"%s\", stringlen=%d\n", string,
			  stringlen);
	if (!view->lang)
		return;
	view->delete_rescan = FALSE;
	if (GTK_WIDGET_VISIBLE(view)) {
		/* now find out if this insert should trigger a rescan */
		glong uslen = g_utf8_strlen(string, stringlen);
		len = 0;
		while (len < uslen) {
			if (view->lang->as_triggers[(gint) * p] == 1) {
				trigger = TRUE;
				break;
			}
			len++;
			p = g_utf8_next_char(p);
		}
		if (!trigger)
			return;
		if (stringlen == 1 && *string == '>')
			view->tag_ac_state = TRUE;
		else
			view->tag_ac_state = FALSE;

#if defined SCANALLTAGVIEWABLE || defined SKIP_KNOWN_UNCHANGED_BLOCKS
		if (view->need_rescan) 
			bf_textview_scan(view);
		else
			bf_textview_scan_visible(view);
#else
		if (view->hl_mode == BFTV_HL_MODE_ALL || view->need_rescan) {
			bf_textview_scan(view);
		} else {
			bf_textview_scan_visible(view);
		}
#endif
	} else {
		DEBUG_MSG("bf_textview_insert_text_cb, postpone the scanning, setting need_rescan\n");
		view->need_rescan = TRUE;
	}
}

static void bf_textview_delete_range_cb(GtkTextBuffer * textbuffer, GtkTextIter * arg1,
										GtkTextIter * arg2, gpointer user_data)
{
	BfTextView *view = BF_TEXTVIEW(user_data);
	gboolean trigger = FALSE;
	gint len, pomlen;
	gchar *p, *pomstr;

	view->delete_rescan = FALSE;
	DEBUG_MSG("bf_textview_delete_range_cb, started\n");
	if (!view->lang)
		return;
	bftv_clear_matched_block(view);
	bftv_clear_block_cache(view);
	bftv_delete_blocks_from_area(view, arg1, arg2, TRUE);
	gtk_text_buffer_remove_all_tags(textbuffer, arg1, arg2);
	p = pomstr = gtk_text_buffer_get_text(textbuffer, arg1, arg2, TRUE);
	len = 0;
	pomlen = g_utf8_strlen(pomstr, -1);

	while (len < pomlen) {
		if (view->lang->as_triggers[(gint) * p] == 1) {
			trigger = TRUE;
			break;
		}
		len++;
		p = g_utf8_next_char(p);
	}

	g_free(pomstr);
	if (trigger)
		view->delete_rescan = TRUE;
}

static void bf_textview_delete_range_after_cb(GtkTextBuffer * textbuffer, GtkTextIter * arg1,
											  GtkTextIter * arg2, gpointer user_data)
{
	BfTextView *view = BF_TEXTVIEW(user_data);

	DEBUG_MSG("bf_textview_delete_range_cb, started\n");

	if (!view->lang)
		return;
	if (view->delay_rescan)
		return;
	if (GTK_WIDGET_VISIBLE(view) && view->delete_rescan) {
#ifdef SCANALLTAGVIEWABLE
		if (view->need_rescan) {
			bf_textview_scan(view);
			view->delete_rescan = FALSE;
		} else {
			bf_textview_scan_visible(view);
			view->delete_rescan = FALSE;
		}
#else
		if (view->hl_mode == BFTV_HL_MODE_ALL || view->need_rescan) {
			bf_textview_scan(view);
			view->delete_rescan = FALSE;
		} else {
			bf_textview_scan_visible(view);
			view->delete_rescan = FALSE;
		}
#endif
	} else if (view->delete_rescan) {
		view->need_rescan = TRUE;
	}
}


/* ---------------------    /EVENTS ---------------------------- */

/* ---------------------    UTILITY FUNCTIONS  ---------------------------- */
inline GtkTextMark *bftv_get_block_at_iter(GtkTextIter * it)
{
	GSList *tmp, *lst = gtk_text_iter_get_marks(it);
	gpointer ptr = NULL;
	tmp = lst;

	while (tmp) {
		ptr = g_object_get_data(G_OBJECT(tmp->data), "bi2");
		if (ptr) {
			GtkTextMark *mark = GTK_TEXT_MARK(tmp->data);
			g_slist_free(lst);
			return mark;
		}
		tmp = g_slist_next(tmp);
	}
	g_slist_free(lst);
	return NULL;
}

/* what does this function exactly do ? 

it is called from many functions, such as the expose_event_cb and used to find if there should be
a block marker on the side.. 

this function should be pretty fast if possible !!!!!!!!

*/
static GtkTextMark *bftv_get_first_block_at_line(BfTextView * view, GtkTextIter * it,
												 gboolean not_single)
{
	GtkTextIter it2, it3;
	GtkTextMark *mark = NULL;
	gint *ln = NULL;
	gpointer ptr = NULL;

	if (!it)
		return NULL;
	ln = g_new0(gint, 1);
	*ln = gtk_text_iter_get_line(it);
	ptr = g_hash_table_lookup(view->fbal_cache, ln);
	if (ptr) {
		g_free(ln);
		return GTK_TEXT_MARK(ptr);
	}
	it2 = it3 = *it;
	gtk_text_iter_set_line(&it2, gtk_text_iter_get_line(it));
	gtk_text_iter_forward_to_line_end(&it3);
	gtk_text_iter_backward_char(&it3);

	while (gtk_text_iter_compare(&it2, &it3) <= 0
		   && gtk_text_iter_get_line(&it2) == gtk_text_iter_get_line(it)) {
		mark = bftv_get_block_at_iter(&it2);
		if (mark) {
			BlockInfo2 *bi;
			bi = (BlockInfo2 *) g_object_get_data(G_OBJECT(mark), "bi2");
			if (not_single) {
				if (!bi->single_line) {
					g_hash_table_insert(view->fbal_cache, ln, mark);
					return mark;
				}
			} else {
				g_hash_table_insert(view->fbal_cache, ln, mark);
				return mark;
			}
		}
		if (!gtk_text_iter_forward_char(&it2))
			break;
	}
	g_free(ln);
	return NULL;
}

/*static GtkTextMark *bftv_get_last_block_at_line(BfTextView *view, GtkTextIter * it)
{
	GtkTextIter it2, it3;
	GtkTextMark *mark = NULL, *mark2 = NULL;
	gint *ln = NULL;
	gpointer ptr = NULL;

if (!it)	return NULL;
ln = g_new0(gint,1);
*ln = gtk_text_iter_get_line(it);
ptr = g_hash_table_lookup(view->lbal_cache,ln);	
if ( ptr )
{
	g_free(ln);
	return GTK_TEXT_MARK(ptr);
}
	
it2 = it3 = *it;
gtk_text_iter_set_line(&it2, gtk_text_iter_get_line(it));
gtk_text_iter_forward_to_line_end(&it3);
gtk_text_iter_backward_char(&it3);

while (gtk_text_iter_compare(&it2, &it3) <= 0
	   && gtk_text_iter_get_line(&it2) == gtk_text_iter_get_line(it)) 
{
	mark = bftv_get_block_at_iter(&it2);
	if (mark) 
	{
		mark2 = mark;
	}
	if (!gtk_text_iter_forward_char(&it2))
		break;
}
if (mark2) g_hash_table_insert(view->fbal_cache,ln,mark2);
else g_free(ln);
return mark2;
}*/
static gint offset_for_mark(GtkTextBuffer *buf, GtkTextMark *mark) {
	GtkTextIter it;
	gtk_text_buffer_get_iter_at_mark(buf,&it,mark);
	return gtk_text_iter_get_offset(&it);
}

static void blockinfo_free(GtkTextBuffer *buf, BlockInfo2 *bi) {
	
	if (bi->tagname) g_free(bi->tagname);
	/*g_print("blockinfo_free at %d:%d - %d%d\n"
			,offset_for_mark(buf,bi->blockstart_s),offset_for_mark(buf,bi->blockstart_e)
			,offset_for_mark(buf,bi->blockend_s),offset_for_mark(buf,bi->blockend_e)
				);*/
	gtk_text_buffer_delete_mark(buf, bi->blockstart_s);
	gtk_text_buffer_delete_mark(buf, bi->blockstart_e);
	gtk_text_buffer_delete_mark(buf, bi->blockend_s);
	gtk_text_buffer_delete_mark(buf, bi->blockend_e);
	g_slice_free(BlockInfo2,bi); 
}

static void blockinfo_free_bymark(GtkTextBuffer *buf, GtkTextMark *mark) {
	BlockInfo2 *bi;
	bi = g_object_get_data(G_OBJECT(mark), "bi2");
	if (bi) {
		blockinfo_free(buf, bi);
	}
}

/* this function takes quite a part of the cpu time when working with very large files. what 
does this function do?

it is called when preparing an area for scanning (so before the scanning) and it is called in the 
'delete_text' callback  

it seems to loop over the region between startit and endit, for every character it looks if there is a 
mark with a non-folded block, and then remove the marks, BlockInfo and tags 
 */
static void bftv_delete_blocks_from_area(BfTextView * view, GtkTextIter * startit, GtkTextIter * endit, gboolean remove_tags)
{
	GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	GtkTextIter it;

	if (!view->lang || !view->lang->scan_blocks)
		return;
	it = *startit;
	while (!gtk_text_iter_equal(&it, endit)) {
		GtkTextMark *mark;
		mark = bftv_get_block_at_iter(&it);
		if (mark) {
			BlockInfo2 *bi;
			bi = g_object_get_data(G_OBJECT(mark), "bi2");
			if (bi && !bi->folded) {
				GtkTextIter it1,it2;
				gtk_text_buffer_get_iter_at_mark(buf, &it1, bi->blockstart_s);
				gtk_text_buffer_get_iter_at_mark(buf, &it2, bi->blockend_e);
				if (remove_tags) {
					DEBUG_TEXTTAG_MSG("remove tag %p (%s:%d)\n",main_v->lang_mgr->internal_tags[IT_BLOCK],__FILE__,__LINE__);
					gtk_text_buffer_remove_tag(buf,main_v->lang_mgr->internal_tags[IT_BLOCK],&it1, &it2);
				}
				blockinfo_free(buf, bi);
			}
		}
		gtk_text_iter_forward_char(&it);
	}
}

static void bftv_fold(BfTextView * self, GtkTextMark * mark, gboolean move_cursor)
{
	GtkTextBuffer *buf = gtk_text_mark_get_buffer(mark);
	BlockInfo2 *bi = (BlockInfo2 *) g_object_get_data(G_OBJECT(mark), "bi2");
	GtkTextIter it1, it2, it3, it4;
	
	if (!bi || bi->single_line || bi->blockstart_s != mark) {
		DEBUG_TEXTTAG_MSG("BUG in bftv_fold? perhaps bi->blockstart_e(%p) == mark(%p)\n",bi->blockstart_e,mark);
		return;
	}
	bi->folded = bi->folded ? FALSE : TRUE;
	bf_textview_get_iters_at_blockinfo(buf, bi, &it1, &it2, &it3, &it4);
	gtk_text_iter_set_line_offset(&it1,0);
	gtk_text_iter_set_line_offset(&it3,0);
	if (bi->folded) {
		gtk_text_buffer_apply_tag_by_name(buf, "_folded_", &it2, &it3);
		gtk_text_buffer_apply_tag_by_name(buf, "_fold_header_", &it1, &it2);
		gtk_text_buffer_apply_tag_by_name(buf, "_fold_header_", &it3, &it4);
		gtk_text_iter_forward_char(&it4);
		gtk_text_buffer_place_cursor(buf, &it4);
	} else {
		gtk_text_buffer_remove_tag_by_name(buf, "_fold_header_", &it1, &it4);
		gtk_text_buffer_remove_tag_by_name(buf, "_folded_", &it1, &it4);
		gtk_text_iter_forward_line(&it2);
		gtk_text_iter_backward_line(&it3);
		bf_textview_fold_blocks_area(self, &it2, &it3, FALSE);
	}
}

static void bftv_expand_all(GtkWidget * widget, BfTextView * view)
{
	GtkTextIter it;
	bf_textview_fold_blocks(view, FALSE);
	gtk_text_buffer_get_start_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)), &it);
	gtk_text_buffer_place_cursor(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)), &it);
	gtk_widget_queue_draw(GTK_WIDGET(view));
}

static void bftv_collapse_all(GtkWidget * widget, BfTextView * view)
{
	GtkTextIter it;
	bf_textview_fold_blocks(view, TRUE);
	gtk_text_buffer_get_start_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)), &it);
	gtk_text_buffer_place_cursor(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)), &it);
	gtk_widget_queue_draw(GTK_WIDGET(view));
}
/*
GtkTextMark *bf_textview_get_nearest_block(BfTextView * self, GtkTextIter * iter, gboolean backward,
										   gint mode, gboolean not_single)
{
	GtkTextMark *mark = bftv_get_block_at_iter(iter);
	GtkTextIter it = *iter, endit;
	gboolean found = FALSE;
	BlockInfo *bi;

	endit = it;
	switch (mode) {
	case BF_GNB_HERE:
		return mark;
	case BF_GNB_LINE:
		if (backward)
			gtk_text_iter_set_line(&endit, gtk_text_iter_get_line(iter));
		else
			gtk_text_iter_forward_to_line_end(&endit);
		break;
	case BF_GNB_DOC:
		if (backward)
			gtk_text_iter_set_offset(&endit, 0);
		else
			gtk_text_iter_forward_to_end(&endit);
		break;
	case BF_GNB_CHAR:
		if (backward)
			gtk_text_iter_backward_char(&endit);
		else
			gtk_text_iter_forward_char(&endit);
		break;
	}

	if (backward) {
		gboolean cont = TRUE;
		while (!found && !gtk_text_iter_compare(&it, &endit) >= 0 && cont) {
			cont = gtk_text_iter_backward_char(&it);
			mark = bftv_get_block_at_iter(&it);
			if (mark) {
				bi = (BlockInfo *) g_object_get_data(G_OBJECT(mark), "bi");
				if (not_single) {
					if (!bi->single_line)
						found = TRUE;
				} else
					found = TRUE;
			}
		}
	} else {
		gboolean cont = TRUE;
		while (!found && !gtk_text_iter_compare(&it, &endit) <= 0 && cont) {
			cont = gtk_text_iter_forward_char(&it);
			mark = bftv_get_block_at_iter(&it);
			if (mark) {
				bi = (BlockInfo *) g_object_get_data(G_OBJECT(mark), "bi");
				if (not_single) {
					if (!bi->single_line)
						found = TRUE;
				} else
					found = TRUE;
			}
		}
	}
	return mark;
}*/
/*
GtkTextMark *bf_textview_get_nearest_block_of_type(BfTextView * self, BfLangBlock * block,
												   GtkTextIter * iter, gboolean backward, gint mode,
												   gboolean not_single)
{
	GtkTextMark *mark = NULL, *prev_mark = NULL;
	GtkTextIter it;
	BlockInfo *bi = NULL;

	mark = bf_textview_get_nearest_block(self, iter, backward, mode, not_single);
	if (mark) {
		gtk_text_buffer_get_iter_at_mark(gtk_text_iter_get_buffer(iter), &it, mark);
		bi = (BlockInfo *) g_object_get_data(G_OBJECT(mark), "bi");
	}
	if (mark && bi && bi->data == block)
		return mark;
	while (prev_mark != mark) {
		prev_mark = mark;
		bi = NULL;
		mark = bf_textview_get_nearest_block(self, &it, backward, mode, not_single);
		if (mark) {
			gtk_text_buffer_get_iter_at_mark(gtk_text_iter_get_buffer(iter), &it, mark);
			bi = (BlockInfo *) g_object_get_data(G_OBJECT(mark), "bi");
		}
		if (mark && bi && bi->data == block)
			return mark;
	}
	return NULL;
}
*/

/* ---------------------    /UTILITY FUNCTIONS  ---------------------------- */

/* ---------------------    LANGUAGE CONFIG ---------------------------- */
#ifdef __GNUC__
__inline__ 
#endif
static gboolean bftv_xml_bool(gchar* text)
{
	if (text == NULL || text[0] == '0' || strcasecmp(text,"true") != 0)
		return FALSE;
	return TRUE;
}

/* this function adds every call a block or a token to the scanning table. 
It is called for every token and every block during the loading of the language files.

This function takes a lot of time during startup  
 */
static void bftv_scantable_insert(BfState * scantable, guint8 type, gpointer data,
								  BfLangConfig * cfg)
{
	BfLangToken *token = NULL;
	BfLangBlock *block = NULL;
	gchar *ptr = NULL;
	gunichar *ptr2;
	BfState *current_state = scantable;
	glong size;
	gboolean regexp = TRUE;
	GList *pstates = NULL;

	switch (type) {
	case ST_TOKEN:
		token = (BfLangToken *) data;
		ptr = token->text;
		regexp = token->regexp;
		break;
	case ST_BLOCK_BEGIN:
		block = (BfLangBlock *) data;
		ptr = block->begin;
		regexp = block->regexp;
		break;
	case ST_BLOCK_END:
		block = (BfLangBlock *) data;
		ptr = block->end;
		regexp = block->regexp;
		break;
	}


	if (!ptr || !xmlCheckUTF8((xmlChar *)ptr))
		return;
	if (cfg->case_sensitive || (type == ST_TOKEN && token && token->type == TT_FAKE)) {
		ptr2 = g_utf8_to_ucs4_fast((const gchar *) ptr, -1, &size);
	} else {
		gchar *tmp = g_utf8_strup((const gchar *) ptr, -1);
		ptr2 = g_utf8_to_ucs4_fast(tmp, -1, &size);
		g_free(tmp);
	}
	if (!regexp) {
		if (xmlUTF8Strlen((xmlChar *)ptr) > cfg->max_token_length)
			cfg->max_token_length = xmlUTF8Strlen((xmlChar *)ptr);
	}


	pstates = g_list_append(pstates, scantable);
	

	if (!regexp) { /* a static string */
		gunichar *input = ptr2;
		gshort i;
		while (*input != '\0') {/* main loop, loops for every character of every token in the language file !!!! */
			BfState *st = (BfState *) current_state->tv[(gint8) *input];
			if (st != NULL) {
				if (st->type == ST_TRANSIT) {
					current_state = current_state->tv[(gint8) *input];
				} else {
					gshort m;
					gboolean found = FALSE;
					for (m = 0; m < BFTV_SCAN_RANGE; m++) { /* this loop runs 127 times for every character for every block or token... 
									but what does this loop exactly do ?? */
						if (m != (gchar) *input && st->tv[m] == st->tv[(gint8) *input])
							found = TRUE;
							break;
					}
					current_state->tv[(gint8) *input] = g_slice_new0(BfState);
					current_state = current_state->tv[(gint8) *input];
					current_state->type = ST_TRANSIT;
					cfg->num_states++;
					if (!found)
						g_slice_free(BfState, st);
				}
			} else { /* st == NULL */
				current_state->tv[(gint8) *input] = g_slice_new0(BfState);
				current_state = current_state->tv[(gint8) *input];
				current_state->type = ST_TRANSIT;
				cfg->num_states++;
			}
			input++;
		}
		
		BfState *s = g_slice_new0(BfState);
		s->type = type;
		s->data = data;
		cfg->num_states++;
		for (i = 0; i < BFTV_SCAN_RANGE; i++)
			if (!current_state->tv[i])
				current_state->tv[i] = s;

	} else { /* it is a list of characters (regex style) */
		gint i=0, k, m;
		gunichar *inp3;
		gboolean reverse_set, found;
		GList  *states = NULL, *auxlst;
		gint counter = 0;
		gboolean charset[BFTV_SCAN_RANGE], done = FALSE;
		BfState *z;

		gunichar *input = ptr2;
			
		while (i < size) {/* main loop, loops for every character of every token in the language file !!!! */
			/* DETERMINE SPECIFIED CHARACTER SET */
			reverse_set = FALSE;
			/* IMPROVEMENT: the next for loop can be done with a single 'memset()' call I think */
			/*for (m = 0; m < BFTV_SCAN_RANGE; m++)
				charset[m] = FALSE;*/
			memset(charset, FALSE, BFTV_SCAN_RANGE*sizeof(gboolean));
			if (*input == '[') {
				k = i + 1;
				inp3 = input + 1;
				if (*inp3 == '^') {
					reverse_set = TRUE;
					inp3++;
					k++;
				}
				while (k < size && *inp3 != ']') {
					if (g_unichar_isalnum(*inp3) && *(inp3 + 1) == '-' && g_unichar_isalnum(*(inp3 + 2))) {	/* range */
						for (m = (gint) (*inp3); m <= (gint) (*(inp3 + 2)); m++)
							charset[m] = TRUE;
						inp3 = inp3 + 2;
						k += 2;
					} else {	/* char */
						if (*inp3 == '\\') {
							inp3++;
							k++;
						}
						charset[(gint) * inp3] = TRUE;
					}
					inp3++;
					k++;
				}				/* while k<size */
				i = k;
				input = inp3;
				if (reverse_set) {
					for (m = 0; m < BFTV_SCAN_RANGE; m++)
						charset[m] = !charset[m];
				}
			} /* *inp == '[' */
			else {
				charset[(gint8) *input] = TRUE;
			}
			/* CREATE STATES */
			g_list_free(states);
			states = NULL;
			done = FALSE;
			z = g_slice_new0(BfState);
			z->type = ST_TRANSIT;

			if (i < size - 1 && (*(input + 1) == '?' || *(input + 1) == '*')) {
				auxlst = g_list_first(pstates);
				while (auxlst) {
					states = g_list_append(states, auxlst->data);
					auxlst = g_list_next(auxlst);
				}
			}
			auxlst = g_list_first(pstates);
			while (auxlst) {
				BfState *st = (BfState *) auxlst->data;
				for (m = 0; m < BFTV_SCAN_RANGE; m++) {
					if (charset[m]) {
						if (st->tv[m] == NULL) {
							st->tv[m] = z;
							if (!g_list_find(states, z))
								states = g_list_append(states, z);
						} else {
							if (((BfState *) (st->tv[m]))->type == ST_TRANSIT) {
								if (!g_list_find(states, st->tv[m])) {
									states = g_list_append(states, st->tv[m]);
								}
							} else {
								found = FALSE;
								for (k = 0; k < BFTV_SCAN_RANGE; k++) {
									if (k != m && st->tv[k] == st->tv[m]) {
										found = TRUE;
										break;
									}
								}
								if (!found)
									g_slice_free(BfState, st->tv[m]);
								st->tv[m] = z;
							}
						}
					}			/* charset */
				}				/* for */
				auxlst = g_list_next(auxlst);
			}					/* auxlst */

			if (i < size - 1 && (*(input + 1) == '*' || *(input + 1) == '+')) {
				for (m = 0; m < BFTV_SCAN_RANGE; m++) {
					if (charset[m]) {
						auxlst = g_list_first(states);
						while (auxlst) {
							BfState *st = (BfState *) auxlst->data;
							if (!st->tv[m])
								st->tv[m] = z;
							auxlst = g_list_next(auxlst);
						}
					}
				}
			}

			if (i == size - 1 || (i == size - 2 && *(input + 1) == '+')
				|| (i == size - 2 && *(input + 1) == '*') || (i == size - 2 && *(input + 1) == '?')) {
				BfState *final;
				auxlst = g_list_first(states);
				final = g_slice_new0(BfState);
				final->data = data;
				final->type = type;
				while (auxlst) {
					BfState *st = (BfState *) auxlst->data;
					for (m = 0; m < BFTV_SCAN_RANGE; m++) {
						if (st->tv[m] == NULL) {
							st->tv[m] = final;
						}
					}
					auxlst = g_list_next(auxlst);
				}
			}

			if (i < size - 1 && (*(input + 1) == '?' || *(input + 1) == '*' || *(input + 1) == '+')) {
				input++;
				i++;
			}
			g_list_free(pstates);
			pstates = NULL;
			auxlst = g_list_first(states);
			while (auxlst) {
				pstates = g_list_append(pstates, auxlst->data);
				auxlst = g_list_next(auxlst);
			}
			counter++;
			if (i < size)
				input++;
			i++;
		}
		if (counter > cfg->max_token_length)
			cfg->max_token_length = counter;
	}	
	g_free(ptr2);
}								/* bftv_scantable_insert */


static gpointer bftv_make_entity(xmlDocPtr doc, xmlNodePtr node, BfLangConfig * cfg, gint type,
								 gchar * group, const gchar * groupstyle, gchar * text)
{
	gchar *tmps, *tmps2;
	gpointer ptr;

	if (text != NULL && text[0] == '\0')
		return NULL;
	switch (type) {
	case ST_TOKEN:
		{
			BfLangToken *t;
			if (text == NULL)
				tmps = (gchar *)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
			else
				tmps = text;
			t = g_slice_new0(BfLangToken);
			t->type = TT_NORMAL;
			t->group = group;
			tmps2 = (gchar *)xmlGetProp(node, (const xmlChar *) "regexp");
			t->regexp = bftv_xml_bool(tmps2);
			if (tmps2)
				xmlFree(tmps2);
			tmps2 = (gchar *)xmlGetProp(node, (const xmlChar *) "name");
			if (tmps2 && text == NULL) {
				t->name = tmps2;
			} else {
				t->name = tmps;
				if (tmps2)
					xmlFree(tmps2);
			}
			t->text = tmps;
			tmps2 = (gchar *)xmlGetProp(node, (const xmlChar *) "context");
			if (tmps2) {
				ptr = g_hash_table_lookup(cfg->blocks, tmps2);
				if (!ptr)
					g_warning("Token (%s) context defined as %s but such a block does not exists.",
							  t->text, tmps2);
				t->context = (BfLangBlock *) ptr;
				xmlFree(tmps2);
			} else
				t->context = NULL;

			tmps2 = (gchar *)xmlGetProp(node, (const xmlChar *) "defaultstyle");
			t->tag =
				get_tag_for_scanner_style((gchar *) cfg->name, "t", (gchar *) t->name,
										  (gchar *) tmps2);
			if (!t->tag) {
				t->tag =
					get_tag_for_scanner_style((gchar *) cfg->name, "g", (gchar *) t->group,
											  (gchar *) groupstyle);
			}
			if (tmps2)
				xmlFree(tmps2);
			if (t->context)
				bftv_scantable_insert(&t->context->scan_table, ST_TOKEN, t, cfg);
			else
				bftv_scantable_insert(&cfg->scan_table, ST_TOKEN, t, cfg);
			g_hash_table_insert(cfg->tokens, &t->name, t);
			return t;
		}
		break;
	case ST_BLOCK_BEGIN:
		tmps = (gchar *)xmlGetProp(node, (const xmlChar *) "id");
		if (tmps) {
			BfLangBlock *b = g_slice_new0(BfLangBlock);
			b->type = BT_NORMAL;
			b->name = tmps;
			b->group = group;
			b->begin = (gchar *)xmlGetProp(node, (const xmlChar *) "begin");
			b->end = (gchar *)xmlGetProp(node, (const xmlChar *) "end");
			tmps2 = (gchar *)xmlGetProp(node, (const xmlChar *) "scanned");
			b->scanned = bftv_xml_bool(tmps2);
			if (tmps2)
				xmlFree(tmps2);
			tmps2 = (gchar *)xmlGetProp(node, (const xmlChar *) "markup");
			b->markup = bftv_xml_bool(tmps2);
			if (tmps2)
				xmlFree(tmps2);
			tmps2 = (gchar *)xmlGetProp(node, (const xmlChar *) "foldable");
			b->foldable = bftv_xml_bool(tmps2);
			if (tmps2)
				xmlFree(tmps2);
			tmps2 = (gchar *)xmlGetProp(node, (const xmlChar *) "case");
			b->cs = bftv_xml_bool(tmps2);
			if (tmps2)
				xmlFree(tmps2);
			tmps2 = (gchar *)xmlGetProp(node, (const xmlChar *) "regexp");
			b->regexp = bftv_xml_bool(tmps2);
			if (tmps2)
				xmlFree(tmps2);
			tmps2 = (gchar *)xmlGetProp(node, (const xmlChar *) "context");
			if (tmps2) {
				ptr = g_hash_table_lookup(cfg->blocks, tmps2);
				if (!ptr)
					g_warning("Block (%s) context defined as %s but such a block does not exists.",
							  b->name, tmps2);
				b->context = ptr;
				xmlFree(tmps2);
			} else
				b->context = NULL;
			tmps2 = (gchar *)xmlGetProp(node, (const xmlChar *) "defaultstyle");
			b->tag =
				get_tag_for_scanner_style((gchar *) cfg->name, "b", (gchar *) b->name,
										  (gchar *) tmps2);
			if (!b->tag) {
				b->tag =
					get_tag_for_scanner_style((gchar *) cfg->name, "g", (gchar *) b->group,
											  (gchar *) groupstyle);
			}
			if (tmps2)
				xmlFree(tmps2);

			g_hash_table_insert(cfg->blocks, tmps, b);
			bftv_scantable_insert(&b->scan_table, ST_BLOCK_END, b, cfg);

			if (b->context) {
				bftv_scantable_insert(&((BfLangBlock *) b->context)->scan_table, ST_BLOCK_BEGIN, b,
									  cfg);
			} else {
				bftv_scantable_insert(&cfg->scan_table, ST_BLOCK_BEGIN, b, cfg);
			}
			return b;
		}
		break;
	}
	return NULL;
}


static void bftv_make_block_list(gpointer key, gpointer val, gpointer data)
{
	GList **lst = (GList **) data;
	*lst = g_list_append(*lst, val);
}

static void bftv_update_default(gpointer key, gpointer val, gpointer data)
{
	BfLangConfig *cfg = (BfLangConfig *) data;
	BfLangBlock *b = (BfLangBlock *) val;
	gint i;
	if (!b->scanned)
		return;
	for (i = 0; i < BFTV_SCAN_RANGE; i++)
		if (!b->scan_table.tv[i])
			b->scan_table.tv[i] = cfg->scan_table.tv[i];
}

static void bftv_update_tables(BfLangConfig * cfg, BfLangBlock * b)
{
	GList *lst = cfg->block_list;
	gint i;
	while (lst) {
		BfLangBlock *bb = (BfLangBlock *) lst->data;
		if (bb->scanned && bb->context == b) {
			for (i = 0; i < BFTV_SCAN_RANGE; i++)
				if (!bb->scan_table.tv[i])
					bb->scan_table.tv[i] = b->scan_table.tv[i];
			bftv_update_tables(cfg, bb);
		}
		lst = g_list_next(lst);
	}
}

static BfLangConfig *bftv_load_config(const gchar * filename)
{
	xmlDocPtr doc;
	xmlNodePtr cur, cur2;
	gchar *tmps, *tmps2 = NULL, *tmps3 = NULL, *tmps4;
	BfLangConfig *cfg = NULL;
	GList *lst, *lst2, *lst3;
	gint i;

	g_return_val_if_fail(filename != NULL, (BfLangConfig *) 0);

#ifdef HL_PROFILING
	struct tms tms1;
	struct tms tms2;
	glong tot_ms;
	times(&tms1);
#endif
	
	xmlLineNumbersDefault(1);
	DEBUG_MSG("bftv_load_config, loading %s\n", filename);
	doc = xmlReadFile(filename, "UTF-8", XML_PARSE_RECOVER | XML_PARSE_NOENT);
	cur = xmlDocGetRootElement(doc);
	if (xmlStrcmp(cur->name, (const xmlChar *) "bflang") == 0) {
		cfg = g_slice_new0(BfLangConfig);
		cfg->name = (gchar *)xmlGetProp(cur, (const xmlChar *) "name");
		cfg->description = (gchar *)xmlGetProp(cur, (const xmlChar *) "description");
		cfg->blocks = g_hash_table_new(g_str_hash, g_str_equal);
		cfg->tokens = g_hash_table_new(g_str_hash, g_str_equal);
		cfg->groups = g_hash_table_new(g_str_hash, g_str_equal);
		cfg->tag_begin =
			get_tag_for_scanner_style((gchar *) cfg->name, "m", "tag_begin", "tag begin");
		cfg->tag_end = get_tag_for_scanner_style((gchar *) cfg->name, "m", "tag_end", "tag end");
		cfg->attr_name =
			get_tag_for_scanner_style((gchar *) cfg->name, "m", "attr_name", "attribute");
		cfg->attr_val =
			get_tag_for_scanner_style((gchar *) cfg->name, "m", "attr_val", "attribute value");
		cfg->dont_autoclose = g_hash_table_new(g_str_hash, g_str_equal);
		for (i = 0; i < BFTV_SCAN_RANGE; i++) {
			cfg->as_triggers[i] = 1;
			cfg->escapes[i] = 0;
		}
		cur = cur->xmlChildrenNode;
		while (cur != NULL) {
			if (xmlStrcmp(cur->name, (const xmlChar *) "mimetypes") == 0) {
				cur2 = cur->xmlChildrenNode;
				while (cur2 != NULL) {
					if (xmlStrcmp(cur2->name, (const xmlChar *) "mimetype") == 0) {
						tmps = (gchar *)xmlGetProp(cur2, (const xmlChar *) "type");
						DEBUG_MSG("found mime-type %s\n", tmps);
						if (tmps)
							cfg->mimetypes = g_list_append(cfg->mimetypes, tmps);
					}
					cur2 = cur2->next;
				}
			} /* /mimetypes */
			else if (xmlStrcmp(cur->name, (const xmlChar *) "options") == 0) {
				cur2 = cur->xmlChildrenNode;
				while (cur2 != NULL) {
					if (xmlStrcmp(cur2->name, (const xmlChar *) "option") == 0) {
						tmps = (gchar *)xmlGetProp(cur2, (const xmlChar *) "name");
						tmps2 = (gchar *)xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
						if (tmps) {
							if (strcmp(tmps, "case-sensitive") == 0)
								cfg->case_sensitive = bftv_xml_bool(tmps2);
							else if (strcmp(tmps, "scan-markup-tags") == 0)
								cfg->scan_tags = bftv_xml_bool(tmps2);
							else if (strcmp(tmps, "scan-blocks") == 0)
								cfg->scan_blocks = bftv_xml_bool(tmps2);
							else if (strcmp(tmps, "schema-aware") == 0)
								cfg->schema_aware = bftv_xml_bool(tmps2);
							else if (strcmp(tmps, "autoclose-exclude") == 0) {
								gchar **arr = NULL;
								gint i = 0;
								arr = g_strsplit((const gchar *) tmps2, ",", -1);
								while (arr[i]) {
									g_hash_table_insert(cfg->dont_autoclose, g_strdup(arr[i]),
														&const_true);
									i++;
								}
								g_strfreev(arr);
							} else if (strcmp(tmps, "auto-scan-triggers") == 0) {
								const gchar *p = tmps2;
								for (i = 0; i < BFTV_SCAN_RANGE; i++) {
									cfg->as_triggers[i] = 0;
								}
								i = 0;
								while (i < xmlUTF8Strlen((xmlChar *)tmps2)) {
									p = (gchar *)xmlUTF8Strpos((xmlChar *)tmps2, i);
									cfg->as_triggers[(gint) * p] = 1;
									i++;
									/* p = xmlUTF8Strpos(tmps2, i++); does not take last character */
								}
							} else if (strcmp(tmps,  "escape-characters") == 0) {
								const gchar *p = tmps2;
								i = 0;
								while (i < xmlUTF8Strlen((xmlChar *)tmps2)) {
									p = (gchar *)xmlUTF8Strpos((xmlChar *)tmps2, i);
									cfg->escapes[(gint) * p] = 1;
									i++;
								}
							}
							if (tmps)
								xmlFree(tmps);
						}
						if (tmps2)
							xmlFree(tmps2);
					}
					cur2 = cur2->next;
				}				/* end of cur2 */
			} /* options */
			else if (xmlStrcmp(cur->name, (const xmlChar *) "block-group") == 0) {	/* blocks  */
				tmps3 = (gchar *)xmlGetProp(cur, (const xmlChar *) "id");
				tmps4 = (gchar *)xmlGetProp(cur, (const xmlChar *) "defaultstyle");
				g_hash_table_insert(cfg->groups, g_strdup(tmps3), "b");
				cur2 = cur->xmlChildrenNode;
				while (cur2 != NULL) {
					if (strcmp((gchar *)cur2->name, "block") == 0)
						bftv_make_entity(doc, cur2, cfg, ST_BLOCK_BEGIN, tmps3, tmps4, NULL);
					cur2 = cur2->next;
				}				/* while */
				xmlFree(tmps4);
			} else if (strcmp((gchar *)cur->name, "token-group") == 0) {	/* tokens  */
				tmps3 = (gchar *)xmlGetProp(cur, (const xmlChar *) "id");
				tmps4 = (gchar *)xmlGetProp(cur, (const xmlChar *) "defaultstyle");
				g_hash_table_insert(cfg->groups, g_strdup(tmps3), "t");
				cur2 = cur->xmlChildrenNode;
				while (cur2 != NULL) {
					if (strcmp((gchar *)cur2->name, "token") == 0) {
						bftv_make_entity(doc, cur2, cfg, ST_TOKEN, tmps3, tmps4, NULL);
					} else if (strcmp((gchar *)cur2->name, "token-list") == 0) {
						tmps2 = (gchar *)xmlGetProp(cur2, (const xmlChar *) "separator");
						if (tmps2) {
							gchar **arr;
							tmps = (gchar *)xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
							if (tmps) {
								arr = g_strsplit((const gchar *) tmps, (const gchar *) tmps2, -1);
								xmlFree(tmps2);
								if (arr) {
									gint i = 0;
									while (arr[i] != NULL) {
										bftv_make_entity(doc, cur2, cfg, ST_TOKEN, tmps3, tmps4,
														 (gchar *) g_strdup(arr[i]));
										i++;
									}	/* while */
									g_strfreev(arr);
								}
								xmlFree(tmps);
							}
						}
					}			/* token-list */
					cur2 = cur2->next;
				}				/* while cur2 */
				xmlFree(tmps4);
			} /* token group */
			else if (xmlStrcmp(cur->name, (const xmlChar *) "block") == 0) {	/* block  * without  * a  * group  */
				bftv_make_entity(doc, cur, cfg, ST_BLOCK_BEGIN, NULL, NULL, NULL);
			} /* block without a group */
			else if (xmlStrcmp(cur->name, (const xmlChar *) "token") == 0) {
				bftv_make_entity(doc, cur, cfg, ST_TOKEN, NULL, NULL, NULL);
			} /* token without a group */
			else if (xmlStrcmp(cur->name, (const xmlChar *) "token-list") == 0) {	/* token  * list  * without  * a  * group  */
				tmps2 = (gchar *)xmlGetProp(cur, (const xmlChar *) "separator");
				if (tmps2) {
					gchar **arr;
					tmps = (gchar *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
					arr = g_strsplit((const gchar *) tmps, (const gchar *) tmps2, -1);
					xmlFree(tmps2);
					if (arr) {
						gint i = 0;
						while (arr[i] != NULL) {
							bftv_make_entity(doc, cur, cfg, ST_TOKEN, NULL, NULL,
											 (gchar *) g_strdup(arr[i]));
							i++;
						}
						g_strfreev(arr);
					}
					if (tmps)
						xmlFree(tmps);
				}
			}					/* token-list without a group */
			cur = cur->next;
		}						/* while cur */

/* fill all blocks with tokens without context ! */
		cfg->block_list = NULL;
		g_hash_table_foreach(cfg->blocks, bftv_make_block_list, &cfg->block_list);
		g_hash_table_foreach(cfg->blocks, bftv_update_default, cfg);
		lst = cfg->block_list;
		while (lst) {
			BfLangBlock *bbb = (BfLangBlock *) lst->data;
			if (!bbb->context)
				bftv_update_tables(cfg, bbb);
			lst = g_list_next(lst);
		}

		lst2 = NULL;
		g_hash_table_foreach(cfg->tokens, bftv_make_block_list, &lst2);
		lst = lst2;
		lst3 = NULL;
		while (lst) {
			BfLangToken *t = (BfLangToken *) lst->data;
			if (!t->regexp) {
				lst3 = g_list_append(lst3, g_strstrip((gchar *) (t->text)));
			}
			lst = g_list_next(lst);
		}
		ac_add_lang_list(main_v->autocompletion, (gchar *) cfg->name, lst3);


		if (cfg->scan_tags) {
			BfLangBlock *b;
			BfLangToken *t = g_slice_new0(BfLangToken);
			t->group = NULL;
			t->regexp = TRUE;
			t->name = g_strdup("_tag_end_");
			t->text = g_strdup("</[a-zA-Z:_][a-zA-Z0-9:_-.]*[ ]*>");
			t->context = NULL;
			t->type = TT_TAG_END;
			bftv_scantable_insert(&cfg->scan_table, ST_TOKEN, t, cfg);
			g_hash_table_insert(cfg->tokens, &t->name, t);

			b = g_slice_new0(BfLangBlock);
			b->name = g_strdup("_tag_begin_");
			b->group = NULL;
			b->regexp = TRUE;
			b->begin = g_strdup("<[a-zA-Z:_][a-zA-Z0-9:_-.]*[ ]*");
			b->end = g_strdup("/?>");
			b->scanned = TRUE;
			b->foldable = FALSE;
			b->cs = FALSE;
			b->type = BT_TAG_BEGIN;
			b->markup = FALSE;
			g_hash_table_insert(cfg->blocks, b->name, b);
			bftv_scantable_insert(&cfg->scan_table, ST_BLOCK_BEGIN, b, cfg);
			bftv_scantable_insert(&b->scan_table, ST_BLOCK_END, b, cfg);



			t = g_slice_new0(BfLangToken);
			t->group = NULL;
			t->regexp = TRUE;
			t->name = g_strdup("_attr2_");
			t->type = TT_ATTR2;
			t->text = g_strdup("[a-zA-Z-:]+=\"[^\"]*\"");
			t->context = b;
			bftv_scantable_insert(&b->scan_table, ST_TOKEN, t, cfg);
			g_hash_table_insert(cfg->tokens, &t->name, t);

			t = g_slice_new0(BfLangToken);
			t->group = NULL;
			t->regexp = TRUE;
			t->name = g_strdup("_attr_");
			t->type = TT_ATTR;
			t->text = g_strdup("[a-zA-Z-:]+=[^\" ><]+");
			t->context = b;
			bftv_scantable_insert(&b->scan_table, ST_TOKEN, t, cfg);
			g_hash_table_insert(cfg->tokens, &t->name, t);


/*		t = g_new0(BfLangToken, 1);
		t->group = NULL;
		t->regexp = TRUE;
		t->name = xmlCharStrdup("_attr_tag_begin_end_");
		t->spec_type = 4;
		t->text = xmlCharStrdup("/?>");
		t->context = b;
		bftv_put_into_dfa(cfg->tag_dfa, cfg, t, BFTV_DFA_TYPE_TOKEN, TRUE);
		g_hash_table_insert(cfg->tokens, &t->tabid, t);
*/

		}

		if (cfg->schema_aware) {
			BfLangToken *t;
			BfLangBlock *b = g_slice_new0(BfLangBlock);
			b->name = g_strdup("_doctype_internal_");
			b->group = NULL;
			b->regexp = TRUE;
			b->begin = g_strdup("<!DOCTYPE[ a-zA-Z]+[\\[]");
			b->end = g_strdup("[\\]]>");
			b->scanned = FALSE;
			b->foldable = TRUE;
			b->cs = FALSE;
			b->type = BT_DOCTYPE_INT;
			b->markup = FALSE;
			g_hash_table_insert(cfg->blocks, b->name, b);
			bftv_scantable_insert(&cfg->scan_table, ST_BLOCK_BEGIN, b, cfg);
			bftv_scantable_insert(&b->scan_table, ST_BLOCK_END, b, cfg);

			t = g_slice_new0(BfLangToken);
			t->group = NULL;
			t->regexp = TRUE;
			t->name = g_strdup("_doctype_");
			t->text = g_strdup("<!DOCTYPE[^>]*>");
			t->context = NULL;
			t->type = TT_DOCTYPE;
			bftv_scantable_insert(&cfg->scan_table, ST_TOKEN, t, cfg);
			g_hash_table_insert(cfg->tokens, &t->name, t);


		}

		{						/* FAKE IDENTIFIER - Lookahead symbol workaround  */
			gunichar c;
			gchar *pstr, *tofree, *pstr2;
			gchar out[6];
			BfLangToken *t;

			pstr = g_strdup("");
			for (c = 0; c < BFTV_SCAN_RANGE; c++) {
				if (g_unichar_isalpha(c) || (gchar) c == '_') {
					tofree = pstr;
					for (i = 0; i < 6; i++)
						out[i] = '\0';
					g_unichar_to_utf8(c, out);
					pstr = g_strdup_printf("%s%s", pstr, out);
					g_free(tofree);
				}
			}
			pstr2 = g_strdup("");
			for (c = 0; c < BFTV_SCAN_RANGE; c++) {
				if (g_unichar_isalnum(c) || (gchar) c == '_') {
					tofree = pstr2;
					for (i = 0; i < 6; i++)
						out[i] = '\0';
					g_unichar_to_utf8(c, out);
					pstr2 = g_strdup_printf("%s%s", pstr2, out);
					g_free(tofree);
				}
			}
			tofree = pstr;
			pstr = g_strdup_printf("[%s]", pstr);
			g_free(tofree);
			for (i = 0; i < cfg->max_token_length + 1; i++) {
				tofree = pstr;
				pstr = g_strdup_printf("%s[%s]", pstr, pstr2);
				g_free(tofree);
			}

			tofree = pstr;
			pstr = g_strdup_printf("%s[%s]*", pstr, pstr2);
			g_free(tofree);
			g_free(pstr2);
			t = g_slice_new0(BfLangToken);
			t->group = NULL;
			t->regexp = TRUE;
			t->name = g_strdup("_fake_ident_");
			t->text = (gchar *) pstr;
			t->context = NULL;
			t->type = TT_FAKE;
			bftv_scantable_insert(&cfg->scan_table, ST_TOKEN, t, cfg);
			g_hash_table_insert(cfg->tokens, &t->name, t);
			lst = g_list_first(cfg->block_list);
			while (lst) {
				BfLangBlock *bbb = (BfLangBlock *) lst->data;
				bftv_scantable_insert(&bbb->scan_table, ST_TOKEN, t, cfg);
				lst = g_list_next(lst);
			}

		}

/* remove markup */
		lst = cfg->block_list;
		while (lst) {
			BfLangBlock *bbb = (BfLangBlock *) lst->data;
			if (!bbb->markup) {
				if (bbb->scan_table.tv[(gint) '<'] == cfg->scan_table.tv[(gint) '<'])
					bbb->scan_table.tv[(gint) '<'] = NULL;
			}
			lst = g_list_next(lst);
		}

	}
	if (doc)
		xmlFreeDoc(doc);
#ifdef HL_PROFILING
	times(&tms2);
	tot_ms = (glong) (double) ((tms2.tms_utime - tms1.tms_utime) * 1000 / sysconf(_SC_CLK_TCK));
	g_print("bftv_load_config(%s) took %ld ms\n", filename, tot_ms);
	g_print("NUMBER OF STATES: %d (%s) - table size = %d, max token len: %d\n", cfg->num_states,
			cfg->name, cfg->num_states * sizeof(BfState), cfg->max_token_length);
#endif
	return cfg;
}


/* ---------------------     /LANGUAGE CONFIG ---------------------------- */



/* --------------------   LANGUAGE MANAGER -------------------------------*/
static void bf_lang_mgr_build_internal_tags(BfLangManager * mgr)
{
	mgr->internal_tags[IT_BLOCK_MATCH] = textstyle_get("_block_match_");
	if (mgr->internal_tags[IT_BLOCK_MATCH] == NULL) {
		mgr->internal_tags[IT_BLOCK_MATCH] = gtk_text_tag_new("_block_match_");
		gtk_text_tag_table_add(textstyle_return_tagtable(), mgr->internal_tags[IT_BLOCK_MATCH]);
		g_object_set(mgr->internal_tags[IT_FOLD_HEADER], "background", "#F7F3D2", NULL);
	}
	mgr->internal_tags[IT_BLOCK] = textstyle_get("_block_");
	if (mgr->internal_tags[IT_BLOCK] == NULL) {
		mgr->internal_tags[IT_BLOCK] = gtk_text_tag_new("_block_");
		gtk_text_tag_table_add(textstyle_return_tagtable(), mgr->internal_tags[IT_BLOCK]);
	}
	mgr->internal_tags[IT_FOLDED] = textstyle_get("_folded_");
	if (mgr->internal_tags[IT_FOLDED] == NULL) {
		mgr->internal_tags[IT_FOLDED] = gtk_text_tag_new("_folded_");
		gtk_text_tag_table_add(textstyle_return_tagtable(), mgr->internal_tags[IT_FOLDED]);
		g_object_set(mgr->internal_tags[IT_FOLDED], "editable", FALSE, "invisible", TRUE, NULL);
	}
	mgr->internal_tags[IT_FOLD_HEADER] = textstyle_get("_fold_header_");
	if (mgr->internal_tags[IT_FOLD_HEADER] == NULL) {
		GdkBitmap *bmp;
		bmp = gdk_bitmap_create_from_data(NULL, folded_xbm, 2, 2);
		mgr->internal_tags[IT_FOLD_HEADER] = gtk_text_tag_new("_fold_header_");
		gtk_text_tag_table_add(textstyle_return_tagtable(), mgr->internal_tags[IT_FOLD_HEADER]);
		g_object_set(mgr->internal_tags[IT_FOLD_HEADER], "editable", FALSE, "background", "#F7F3D2",
					 "foreground-stipple", bmp, NULL);
		g_object_unref(bmp);
	}
	DEBUG_TEXTTAG_MSG("new tags are IT_BLOCK_MATCH:%p, IT_BLOCK:%p, IT_FOLDED:%p, IT_FOLD_HEADER:%p\n"
			,mgr->internal_tags[IT_BLOCK_MATCH]
			,mgr->internal_tags[IT_BLOCK]
			,mgr->internal_tags[IT_FOLDED]
			,mgr->internal_tags[IT_FOLD_HEADER]
			);
}

BfLangManager *bf_lang_mgr_new()
{
	BfLangManager *ret = g_slice_new0(BfLangManager);
	ret->languages = NULL;
	bf_lang_mgr_build_internal_tags(ret);
	return ret;
}

BfLangConfig *bf_lang_mgr_load_config(const gchar * filename)
{
	BfLangConfig *cfg = NULL;

	cfg = bftv_load_config(filename);
	if (cfg != NULL) {
		DEBUG_MSG("bf_lang_mgr_load_config, adding %s to hashtable\n", filename);
		/* hmm can we add them by mime-type instead of filename ? */
		main_v->lang_mgr->languages = g_list_prepend(main_v->lang_mgr->languages, cfg);
	}
	return (cfg);
}

/* I would like a function based on mime types.. 
BfLangConfig *bf_lang_mgr_get_config(BfLangManager * mgr, const gchar * filename)
{
	return g_hash_table_lookup(mgr->languages, filename);
}*/

static void bftv_ins_key(gpointer key, gpointer value, gpointer udata)
{
	GList **lst = (GList **) udata;
	*lst = g_list_append(*lst, key);
}


GList *bf_lang_get_groups(BfLangConfig * cfg)
{
	GList *lst = NULL;
	g_hash_table_foreach(cfg->groups, bftv_ins_key, &lst);
	return lst;
}

typedef struct {
	GList **list;
	gchar *grpcrit;
} Thf;

static void bftv_ins_block(gpointer key, gpointer value, gpointer udata)
{
	Thf *d = (Thf *) udata;
	BfLangBlock *t = (BfLangBlock *) value;
	if (d->grpcrit == NULL && t->group == NULL) {
		if (strcmp(t->name, "_tag_begin_") != 0)
			*(d->list) = g_list_append(*(d->list), t->name);
	} else if (d->grpcrit != NULL && t->group != NULL && strcmp(t->group, d->grpcrit) == 0) {
		if (strcmp(t->name, "_tag_begin_") != 0)
			*(d->list) = g_list_append(*(d->list), t->name);
	}
}

/* If you want to get list of "unaligned" blocks 
*  i.e. blocks which are defined without a group - 
*  just use NULL for group param 
*/
GList *bf_lang_get_blocks_for_group(BfLangConfig * cfg, gchar * group)
{
	GList *lst = NULL;
	Thf *hf = g_new0(Thf, 1);

	hf->list = &lst;
	hf->grpcrit = group;
	g_hash_table_foreach(cfg->blocks, bftv_ins_block, hf);
	g_free(hf);
	return lst;
}


static void bftv_ins_token(gpointer key, gpointer value, gpointer udata)
{
	Thf *d = (Thf *) udata;
	BfLangToken *t = (BfLangToken *) value;
	if (d->grpcrit == NULL && t->group == NULL) {
		if (strcmp(t->name,  "_tag_end_") != 0
			&& strcmp(t->name,  "_attr_") != 0
			&& strcmp(t->name,  "_attr2_") != 0
			&& strcmp(t->name,  "_attr_tag_begin_end_") != 0
			&& strcmp(t->name,  "_fake_ident_") != 0)
			*(d->list) = g_list_append(*(d->list), t->name);
	} else if (d->grpcrit != NULL && t->group != NULL && strcmp(t->group, d->grpcrit) == 0) {
		if (strcmp(t->name,  "_tag_end_") != 0
			&& strcmp(t->name,  "_attr_") != 0
			&& strcmp(t->name,  "_attr2_") != 0
			&& strcmp(t->name,  "_attr_tag_begin_end_") != 0
			&& strcmp(t->name,  "_fake_ident_") != 0)
			*(d->list) = g_list_append(*(d->list), t->name);
	}
}

/* If you want to get list of "unaligned" tokens 
*  i.e. tokens which are defined without a group - 
*  just use NULL for group param 
*/
GList *bf_lang_get_tokens_for_group(BfLangConfig * cfg, gchar * group)
{
	GList *lst = NULL;
	Thf *hf = g_new0(Thf, 1);

	hf->list = &lst;
	hf->grpcrit = group;
	g_hash_table_foreach(cfg->tokens, bftv_ins_token, hf);
	g_free(hf);
	return lst;
}

gboolean bf_lang_needs_tags(BfLangConfig * cfg)
{
	return cfg->scan_tags;
}

static void bflang_retag_token(gpointer key, gpointer value, gpointer udata)
{
	BfLangToken *t = (BfLangToken *) value;
	BfLangConfig *cfg = (BfLangConfig *) udata;
	t->tag = get_tag_for_scanner_style((gchar *) cfg->name, "t", (gchar *) t->name, NULL);
	if (!t->tag) {
		t->tag = get_tag_for_scanner_style((gchar *) cfg->name, "g", (gchar *) t->group, NULL);
	}
}

static void bflang_retag_block(gpointer key, gpointer value, gpointer udata)
{
	BfLangBlock *b = (BfLangBlock *) value;
	BfLangConfig *cfg = (BfLangConfig *) udata;
	b->tag = get_tag_for_scanner_style((gchar *) cfg->name, "b", (gchar *) b->name, NULL);
	if (!b->tag) {
		b->tag = get_tag_for_scanner_style((gchar *) cfg->name, "g", (gchar *) b->group, NULL);
	}
}

static void bf_lang_retag(gpointer value, gpointer udata)
{
	BfLangConfig *cfg = (BfLangConfig *) value;

	textstyle_build_lookup_table();
	
	g_hash_table_foreach(cfg->tokens, bflang_retag_token, cfg);
	g_hash_table_foreach(cfg->blocks, bflang_retag_block, cfg);
	cfg->tag_begin = get_tag_for_scanner_style((gchar *) cfg->name, "m", "tag_begin", NULL);
	cfg->tag_end = get_tag_for_scanner_style((gchar *) cfg->name, "m", "tag_end", NULL);
	cfg->attr_name = get_tag_for_scanner_style((gchar *) cfg->name, "m", "attr_name", NULL);
	cfg->attr_val = get_tag_for_scanner_style((gchar *) cfg->name, "m", "attr_val", NULL);

	textstyle_cleanup_lookup_table();
}

void bf_lang_mgr_retag(void)
{

	g_list_foreach(main_v->lang_mgr->languages, bf_lang_retag, NULL);
	/* we should perhaps also retag some of the internal tags such as _block_match_ ?
	   these tags are stored in each document, so we have to rescan all documents
	   to set them again */
	bf_lang_mgr_build_internal_tags(main_v->lang_mgr);
}

/* -------------------- /LANGUAGE MANAGER  -------------------------------*/

/* -------------------- FOLDING  -------------------------------*/
void bf_textview_fold_blocks(BfTextView * self, gboolean fold)
{
	GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self));
	GtkTextIter its, ite;

	g_return_if_fail(self != NULL);
	g_return_if_fail(BF_IS_TEXTVIEW(self));
	gtk_text_buffer_get_start_iter(buf, &its);
	gtk_text_buffer_get_end_iter(buf, &ite);
	bf_textview_fold_blocks_area(self, &its, &ite, fold);
}


void bf_textview_fold_blocks_area(BfTextView * self, GtkTextIter * start, GtkTextIter * end,
								  gboolean fold)
{
	GtkTextMark *mark;
	GtkTextIter it;
	gint i;

	g_return_if_fail(self != NULL);
	g_return_if_fail(BF_IS_TEXTVIEW(self));
	it = *start;
	for (i = gtk_text_iter_get_line(start); i <= gtk_text_iter_get_line(end); i++) {
		gtk_text_iter_set_line(&it, i);
		mark = bftv_get_first_block_at_line(self, &it, TRUE);
		if (mark) {
			BlockInfo2 *bi = (BlockInfo2 *) g_object_get_data(G_OBJECT(mark), "bi2");
			if (bi && bi->blockstart_s == mark) {
				bi->folded = !fold;
				bftv_fold(self, mark, FALSE);
			}
		}
	}
}

/* -------------------- /FOLDING  -------------------------------*/

/* -------------------- SCANNING  -------------------------------*/


void bf_textview_scan(BfTextView * self)
{
	if (!self->delay_rescan) {
		GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self));
		GtkTextIter its, ite;

		g_return_if_fail(self != NULL);
		g_return_if_fail(BF_IS_TEXTVIEW(self));
		g_return_if_fail(buf != NULL);
		gtk_text_buffer_get_start_iter(buf, &its);
		gtk_text_buffer_get_end_iter(buf, &ite);
		if (gtk_text_iter_equal(&its, &ite))
			return;
		self->need_rescan = FALSE;
		bf_textview_scan_area(self, &its, &ite, TRUE, FALSE, TRUE);
	} else {
		self->need_rescan = TRUE;
	}
}

void bf_textview_scan_visible(BfTextView * self)
{
	if (!self->delay_rescan) {
		GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self));
		GtkTextIter its, ite;
		GdkRectangle rect;
		GtkTextIter l_start, l_end;

		g_return_if_fail(self != NULL);
		g_return_if_fail(BF_IS_TEXTVIEW(self));
		g_return_if_fail(buf != NULL);
		gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(self), &rect);
		gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(self), &l_start, rect.y, NULL);
		gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(self), &l_end, rect.y + rect.height, NULL);
		its = l_start;
		ite = l_end;
		gtk_text_iter_forward_to_line_end(&ite);
		DEBUGSC_MSG("bf_textview_scan_visible, from %d (line %d) to %d (line %d)\n"
				,gtk_text_iter_get_offset(&its),gtk_text_iter_get_line(&its)
				,gtk_text_iter_get_offset(&ite),gtk_text_iter_get_line(&ite));
		bf_textview_scan_area(self, &its, &ite, TRUE, FALSE, FALSE);
	} else {
		self->need_rescan = TRUE;
	}
}

#ifdef HL_PROFILING
/* this function is here for debugging purposes */
static void bftv_dump_location_info(gint line, GtkTextBuffer * buffer, GtkTextIter * it)
{
	GSList *ss;
	ss = gtk_text_iter_get_marks(it);
	while (ss) {
		GtkTextMark *m = (GtkTextMark *) ss->data;
		g_print("bftv_dump_location_info, called from line %d, location %d has mark %p (%s)\n",
				line, gtk_text_iter_get_offset(it), m, gtk_text_mark_get_name(m));
		ss = g_slist_next(ss);
	}
	g_slist_free(ss);
}
#endif							/* HL_DEBUG */

typedef struct {
	GtkTextBuffer *buffer;
	GtkTextIter *start;
	GtkTextIter *end;
} Trts;
/*
static void bftv_remove_t_tag(gpointer key, gpointer value, gpointer data)
{
	BfLangToken *t = (BfLangToken *) value;
	Trts *s = (Trts *) data;
	if (t->tag)
		gtk_text_buffer_remove_tag(s->buffer, t->tag, s->start, s->end);
}

static void bftv_remove_b_tag(gpointer key, gpointer value, gpointer data)
{
	BfLangBlock *b = (BfLangBlock *) value;
	Trts *s = (Trts *) data;
	if (b->tag)
		gtk_text_buffer_remove_tag(s->buffer, b->tag, s->start, s->end);
}*/

static gboolean bftv_remove_cache_item(gpointer key, gpointer value, gpointer data)
{
	g_free((gint *) key);
	return TRUE;
}

static void bftv_clear_block_cache(BfTextView * self)
{
	g_hash_table_foreach_remove(self->fbal_cache, bftv_remove_cache_item, NULL);
	g_hash_table_foreach_remove(self->lbal_cache, bftv_remove_cache_item, NULL);
}

static void bftv_clear_matched_block(BfTextView * self)
{
	GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self));
	DEBUG_MSG("bftv_clear_matched_block, started\n");
	if (self->last_matched_block) {
		BlockInfo2 *bi = NULL;
		GtkTextIter it1, it2, it3, it4;
		bi = (BlockInfo2 *) g_object_get_data(G_OBJECT(self->last_matched_block), "bi2");
		bf_textview_get_iters_at_blockinfo(buf, bi, &it1, &it2, &it3, &it4);
		DEBUG_TEXTTAG_MSG("REMOVE tag %p (%s:%d)\n",main_v->lang_mgr->internal_tags[IT_BLOCK_MATCH],__FILE__,__LINE__);
		gtk_text_buffer_remove_tag(buf, main_v->lang_mgr->internal_tags[IT_BLOCK_MATCH], &it1, &it2);
		gtk_text_buffer_remove_tag(buf, main_v->lang_mgr->internal_tags[IT_BLOCK_MATCH], &it3, &it4);
	}
	self->last_matched_block = NULL;
}

static void bf_textview_add_block(BfTextView * self, GtkTextBuffer *buf, gchar *tagname, GtkTextIter *blockstart_s, GtkTextIter *blockstart_e, GtkTextIter *blockend_s, GtkTextIter *blockend_e, gboolean single_line, gpointer data) {
	BlockInfo2 *bi2;
	GtkTextMark *mark,*mark2,*mark3,*mark4;
	mark = gtk_text_buffer_create_mark(buf, NULL, blockstart_s, FALSE);
	mark2 = gtk_text_buffer_create_mark(buf, NULL, blockstart_e, FALSE);
	mark3 = gtk_text_buffer_create_mark(buf, NULL, blockend_s, FALSE);
	mark4 = gtk_text_buffer_create_mark(buf, NULL, blockend_e, FALSE);
	bi2 = g_slice_new(BlockInfo2);
	bi2->folded = FALSE;
	bi2->tagname = g_strdup(tagname);
	bi2->blockstart_s = mark;
	bi2->blockstart_e = mark2;
	bi2->blockend_s = mark3;
	bi2->blockend_e = mark4;
	bi2->single_line = single_line;
	bi2->data = data;
	g_object_set_data(G_OBJECT(mark), "bi2", bi2);
	g_object_set_data(G_OBJECT(mark3), "bi2", bi2);
	/*g_print("%s:%d, marks with bi2 set at %d and %d\n",__FILE__,__LINE__,gtk_text_iter_get_offset(blockstart_s),gtk_text_iter_get_offset(blockend_s));*/
}

#ifdef REUSETAGS

/*static void debug_tags(GtkTextIter *it) {
	GSList *toggles = gtk_text_iter_get_toggled_tags(it,FALSE);
	while (toggles) {
		g_print("toggles tag %p\n",toggles->data);
		toggles = g_slist_next(toggles);
	}
	toggles = gtk_text_iter_get_tags(it);
	while (toggles) {
		g_print("has tag %p\n",toggles->data);
		toggles = g_slist_next(toggles);
	}
}*/

static void bf_textview_check_or_apply_tag(GtkTextBuffer *buf,GtkTextTag *tag,GtkTextIter *its, GtkTextIter *ita) {
	if (!gtk_text_iter_toggles_tag(its, tag) || !gtk_text_iter_toggles_tag(ita, tag)) {
		/*g_print("%s:%d, set tag %p from %d to %d\n",__FILE__,__LINE__,tag,gtk_text_iter_get_offset(its),gtk_text_iter_get_offset(ita));*/
		/*debug_tags(its);*/
		gtk_text_buffer_apply_tag(buf, tag, its, ita);
	} /*else {
		g_print("bf_textview_check_or_apply_tag, re-use existing tag %p\n",tag);
	}*/
}
#else
#define bf_textview_check_or_apply_tag gtk_text_buffer_apply_tag
#endif

/* this is called just once */
#ifdef __GNUC__
__inline__ 
#endif
static void bf_textview_scan_state_type_st_token(BfTextView * self, GtkTextBuffer *buf, BfState * current_state, GtkTextIter *its, GtkTextIter *ita, gboolean apply_hl)
{
	GtkTextIter pit;
	BfLangToken *t = (BfLangToken *) current_state->data;
	DEBUGSC_MSG("%s:%d token type %d from %d to %d\n",__FILE__,__LINE__,t->type,gtk_text_iter_get_offset(its),gtk_text_iter_get_offset(ita));
	switch (t->type) {
	case TT_NORMAL:
		if (self->highlight && t->tag && apply_hl) {
			DEBUG_TEXTTAG_MSG("apply tag %p (%s:%d)\n",t->tag,__FILE__,__LINE__);
			bf_textview_check_or_apply_tag(buf, t->tag, its, ita);
		}
		break;
	case TT_TAG_END:
		{
			gchar *txt = gtk_text_buffer_get_text(buf, its, ita, FALSE);
			gchar *pc = txt + 2;
			gchar **arr = g_strsplit(pc, ">", -1);
			gchar **arr2 = g_strsplit(arr[0], " ", -1);
			if (!g_queue_is_empty(&(self->scanner.tag_stack))) {
				TBfBlock *bf;
				/* Have to get tags from stack, because HTML allows not closed tags */
				bf = NULL;
				while (!g_queue_is_empty(&(self->scanner.tag_stack))) {
					bf = g_queue_pop_head(&(self->scanner.tag_stack));
					if (bf && strcmp(bf->tagname, arr2[0]) == 0)
						break;
					g_free(bf->tagname);
					g_slice_free(TBfBlock,bf);
					bf = NULL;
				}
				if (bf) {
					GtkTextMark *mark3;
					gboolean do_mark = TRUE;
					
					mark3 = bftv_get_block_at_iter(&bf->b_start);

					if (mark3) {
						BlockInfo2 *bi2;
						bi2 = (BlockInfo2 *) g_object_get_data(G_OBJECT(mark3), "bi2");
						if (bi2 && bi2->tagname && strcmp(bi2->tagname, arr2[0]) == 0)
							do_mark = FALSE;
					}
					if (do_mark) {
						bf_textview_add_block(self,buf,arr2[0], &bf->b_start, &bf->b_end, its, ita
									, (gtk_text_iter_get_line(&bf->b_start) == gtk_text_iter_get_line(its)), NULL);
					}
					if (apply_hl) {
						DEBUG_TEXTTAG_MSG("apply tag %p (%s:%d)\n",main_v->lang_mgr->internal_tags[IT_BLOCK],__FILE__,__LINE__);
						bf_textview_check_or_apply_tag(buf, main_v->lang_mgr->internal_tags[IT_BLOCK],
												  &bf->b_end, its);
					}
					g_free(bf->tagname);
					g_slice_free(TBfBlock,bf);
				}
			}					/* queue empty */
			g_strfreev(arr);
			g_strfreev(arr2);
			g_free(txt);
			if (self->highlight) {
				GtkTextTag *tag;
				tag = self->lang->tag_end;
				if (tag && apply_hl) {
					DEBUG_TEXTTAG_MSG("apply tag %p (%s:%d)\n",tag,__FILE__,__LINE__);
					bf_textview_check_or_apply_tag(buf, tag, its, ita);
				}
			}
		}						/* end of tag_end */
		break;
	case TT_ATTR2:
	case TT_ATTR:
		{
			gchar *txt = gtk_text_buffer_get_text(buf, its, ita, FALSE);
			gchar **arr = g_strsplit(txt, "=", -1);
			gchar *attrname = NULL, *attrval = NULL;
			pit = *its;
			gtk_text_iter_forward_chars(&pit, g_utf8_strlen(arr[0], -1));
			g_strfreev(arr);
			g_free(txt);
			if (self->lang && self->lang->schema_aware) {	/* XMLSchema */
				attrname = gtk_text_buffer_get_text(buf, its, &pit, FALSE);
				attrval = gtk_text_buffer_get_text(buf, &pit, ita, FALSE);
				if (g_str_has_suffix(attrname, "schemaLocation")) {
					gchar **arr = g_strsplit(attrval, " ", -1);
					gchar *sname = NULL;
					if (arr[1]) {
						arr[1][g_utf8_strlen(arr[1], -1) - 1] = '\0';
						sname = ac_add_xmlschema_list(main_v->autocompletion, arr[1]);
						if (sname)
							self->schemas = g_list_append(self->schemas, sname);
					}
					g_strfreev(arr);
				}
				g_free(attrname);
				g_free(attrval);
			}
			if (self->highlight) {
				GtkTextTag *tag;
				tag = self->lang->attr_name;
				if (tag && apply_hl) {
					DEBUG_TEXTTAG_MSG("apply tag %p (%s:%d)\n",tag,__FILE__,__LINE__);
					bf_textview_check_or_apply_tag(buf, tag, its, &pit);
				}
				tag = self->lang->attr_val;
				if (tag && apply_hl) {
					DEBUG_TEXTTAG_MSG("apply tag %p (%s:%d)\n",tag,__FILE__,__LINE__);
					bf_textview_check_or_apply_tag(buf, tag, &pit, ita);
				}
			}
		}
		break;
	case TT_DOCTYPE:
		{
			gchar *txt = gtk_text_buffer_get_text(buf, its, ita, FALSE);
			gchar *sname = ac_add_dtd_list(main_v->autocompletion, txt, FALSE, NULL);
			if (sname)
				self->schemas = g_list_append(self->schemas, sname);
			g_free(txt);
		}
		break;
	case TT_FAKE:
		break;
	}
}
/* this is called just once */
#ifdef __GNUC__
__inline__ 
#endif
static BfState *bf_textview_scan_state_type_st_block_begin(BfTextView * self, GtkTextBuffer *buf, BfState * current_state, GtkTextIter *its, GtkTextIter *ita, gboolean apply_hl)
{
	TBfBlock *bf;
	BfLangBlock *tmp = (BfLangBlock *) current_state->data;
	bf = g_slice_new0(TBfBlock);
	bf->def = tmp;
	bf->b_start = *its;
	bf->b_end = *ita;
	DEBUGSC_MSG("%s:%d block %s begin from %d to %d\n",__FILE__,__LINE__,tmp->name,gtk_text_iter_get_offset(its),gtk_text_iter_get_offset(ita));
	g_queue_push_head(&(self->scanner.block_stack), bf);
	self->scanner.current_context = tmp;
	if (tmp->type == BT_TAG_BEGIN) {
		GtkTextIter pit;
		pit = *its;
		gtk_text_iter_forward_char(&pit);
		bf->tagname = gtk_text_buffer_get_text(buf, &pit, ita, FALSE);
		bf->tagname = g_strstrip(bf->tagname);
		self->scanner.last_tagname = g_strdup(bf->tagname);
	} else {
		if (self->scanner.last_tagname)
			g_free(self->scanner.last_tagname);
		self->scanner.last_tagname = NULL;
		bf->tagname = NULL;
	}
	return &tmp->scan_table;
}

/* this is called just once */
#ifdef __GNUC__
__inline__ 
#endif
static BfState *bf_textview_scan_state_type_st_block_end(BfTextView * self, GtkTextBuffer *buf, BfState * current_state, GtkTextIter *its, GtkTextIter *ita, gboolean apply_hl)
{
	TBfBlock *bf;
	BfLangBlock *tmp = (BfLangBlock *) current_state->data;
	DEBUGSC_MSG("%s:%d block %s end from %d to %d\n",__FILE__,__LINE__,tmp->name, gtk_text_iter_get_offset(its),gtk_text_iter_get_offset(ita));
	bf = g_queue_peek_head(&(self->scanner.block_stack));
	if (bf && bf->def == tmp) {
		TBfBlock *aux;
		g_queue_pop_head(&(self->scanner.block_stack));
		aux = (TBfBlock *) g_queue_peek_head(&(self->scanner.block_stack));
		if (aux)
			self->scanner.current_context = aux->def;
		else
			self->scanner.current_context = NULL;
		if (self->scanner.current_context)
			current_state = &self->scanner.current_context->scan_table;
		else
			current_state = &self->lang->scan_table;
		switch (tmp->type) {
		case BT_TAG_BEGIN:
			{
				GtkTextIter pit;
				TBfBlock *bf_2;
				gboolean just_ended = FALSE;
				if (self->scanner.last_tagname)
					g_free(self->scanner.last_tagname);
				self->scanner.last_tagname = NULL;
				bf_2 = g_slice_new0(TBfBlock);
				bf_2->tagname = g_strdup(bf->tagname);
				bf_2->b_start = bf->b_start;
				bf_2->b_end = *ita;
				g_queue_push_head(&(self->scanner.tag_stack), bf_2);
				pit = *ita;
				while (gtk_text_iter_get_char(&pit) != '>')
					gtk_text_iter_backward_char(&pit);
				gtk_text_iter_backward_char(&pit);
				if (gtk_text_iter_get_char(&pit) == '/')
					just_ended = TRUE;
				if (self->highlight) {
					GtkTextTag *tag;
					tag = self->lang->tag_begin;
					if (tag && apply_hl) {
						DEBUG_TEXTTAG_MSG("apply tag %p (%s:%d)\n",tag,__FILE__,__LINE__);
						bf_textview_check_or_apply_tag(buf, tag, &bf->b_start, ita);
					}
				}
				/* TAG autoclose */
				if (self->tag_autoclose && !self->delete_rescan && !self->paste_operation
					&& self->tag_ac_state && !just_ended
					&& g_hash_table_lookup(self->lang->dont_autoclose, bf_2->tagname) == NULL) {
					GtkTextIter it9;
					gtk_text_buffer_get_iter_at_mark(buf, &it9, gtk_text_buffer_get_insert(buf));
					if (gtk_text_iter_equal(&it9, ita)) {
						gchar *pp;
						if (main_v->props.close_tag_newline)
							pp = g_strconcat("\n</", bf_2->tagname, ">", NULL);
						else
							pp = g_strconcat("</", bf_2->tagname, ">", NULL);
						self->tag_ac_state = FALSE;
						/* Clear stacks */
						while (!g_queue_is_empty(&self->scanner.block_stack)) {
							bf = (TBfBlock *) g_queue_pop_head(&self->scanner.block_stack);
							if (bf->tagname)
								g_free(bf->tagname);
							g_slice_free(TBfBlock,bf);
						}
						while (!g_queue_is_empty(&self->scanner.tag_stack)) {
							bf = (TBfBlock *) g_queue_pop_head(&self->scanner.tag_stack);
							if (bf->tagname)
								g_free(bf->tagname);
							g_slice_free(TBfBlock,bf);
						}
						DEBUG_MSG("about to add the closing tag, this insert will trigger a new scan\n");
						gtk_text_buffer_insert(buf, ita, pp, -1);
						gtk_text_buffer_get_iter_at_mark(buf, &it9,
														 gtk_text_buffer_get_insert(buf));
						gtk_text_iter_backward_chars(&it9, strlen(pp));
						gtk_text_buffer_place_cursor(buf, &it9);
						g_free(pp);
						g_signal_handler_unblock(buf, self->markset_signal_id);
						/* BUG, THIS HAS TO STOP THE SCANNING RUN COMPLETELY */
						/* lets try: set ita to end of file to stop scanning */
						
						gtk_text_iter_forward_to_end(ita);
						return NULL;	/*I have to return from scan, because it has been performed after latest insert */
					}
				}
			}
			break;
		case BT_DOCTYPE_INT:
			{
				gchar *txt = gtk_text_buffer_get_text(buf, &bf->b_start, ita, FALSE);
				ac_add_dtd_list(main_v->autocompletion, txt, TRUE, &self->internal_dtd);
				g_free(txt);
			}
			/* not breaking here - I want next code */
		default:
			{
				GtkTextMark *mark3;
				gboolean do_mark = TRUE;
				
				mark3 = bftv_get_block_at_iter(&bf->b_start);
				if (mark3) {
					BlockInfo2 *bi2;
					bi2 = (BlockInfo2 *) g_object_get_data(G_OBJECT(mark3), "bi2");
					if (bi2 && bi2->blockstart_s == mark3 && bi2->data == tmp)
						do_mark = FALSE;
				}
				if (do_mark) {
					bf_textview_add_block(self,buf,NULL, &bf->b_start, &bf->b_end, its, ita
						, (gtk_text_iter_get_line(&bf->b_start) == gtk_text_iter_get_line(its)), tmp);
				}
				if (apply_hl) {
					/* we only need to apply IT_BLOCK on the top-level block detected. all blocks within 
					the top level will be inside an area in which IT_BLOCK is set already */
					if (g_queue_is_empty(&self->scanner.block_stack)) {
						DEBUG_TEXTTAG_MSG("apply tag %p (%s:%d)\n",main_v->lang_mgr->internal_tags[IT_BLOCK],__FILE__,__LINE__);
						DEBUGSC_MSG("%s:%d set IT_BLOCK from %d to %d\n",__FILE__,__LINE__,gtk_text_iter_get_offset(&bf->b_end),gtk_text_iter_get_offset(its));
						bf_textview_check_or_apply_tag(buf, main_v->lang_mgr->internal_tags[IT_BLOCK],
											  &bf->b_end, its);
					}
					if (self->highlight && tmp->tag) {
						DEBUG_TEXTTAG_MSG("apply tag %p (%s:%d)\n",tmp->tag,__FILE__,__LINE__);
						bf_textview_check_or_apply_tag(buf, tmp->tag, &bf->b_start, ita);
					}
				}
			}					/* default */
		}						/* switch */
		if (bf->tagname)
			g_free(bf->tagname);
		g_slice_free(TBfBlock,bf);
	} /* if bf */
	else {
		current_state = NULL;
	}
	return current_state;
}

static void remove_tags_starting_at_iter(GtkTextBuffer *buf, GtkTextIter *it) {
	GSList *toggles, *tmplist;
	tmplist = toggles = gtk_text_iter_get_toggled_tags(it,TRUE);
	while (tmplist) {
		GtkTextIter tmpit;
		tmpit = *it;
		gtk_text_iter_forward_to_tag_toggle(&tmpit,tmplist->data);
		DEBUGSC_MSG("%s:%d, removing tag %p from %d to %d\n",__FILE__,__LINE__,tmplist->data,gtk_text_iter_get_offset(it),gtk_text_iter_get_offset(&tmpit));
		gtk_text_buffer_remove_tag(buf,tmplist->data,it,&tmpit);
		tmplist = g_slist_next(tmplist);
	}
	g_slist_free(toggles);
}
#ifdef SKIP_KNOWN_UNCHANGED_BLOCKS
static gboolean block_unchanged_and_known(GtkTextBuffer *buf, GtkTextIter *it, GtkTextIter *changestart, GtkTextIter *changeend) {		
	if (gtk_text_iter_compare(it,changestart)>0 && gtk_text_iter_compare(it,changeend)<0) {
		/* the block starts after the start of the changed area but before the end of the changed area */
		DEBUGSC_MSG("no skipping, iter at %d (line %d) is between %d (line %d) and %d (line %d)\n"
					,gtk_text_iter_get_offset(it),gtk_text_iter_get_line(it)
					,gtk_text_iter_get_offset(changestart),gtk_text_iter_get_line(changestart)
					,gtk_text_iter_get_offset(changeend),gtk_text_iter_get_line(changeend));
	} else {
		/* the block starts before the start of the changed area or after the end */
		GtkTextMark *mark;
		mark = bftv_get_block_at_iter(it);
		if (mark) {
			BlockInfo2 *bi;
			bi = g_object_get_data(G_OBJECT(mark), "bi2");
			if (bi) {
				/* GtkTextIter it1; */
				GtkTextIter it2;
				
				/*gtk_text_buffer_get_iter_at_mark(buf, &it1, bi->blockstart_s);*/
				gtk_text_buffer_get_iter_at_mark(buf, &it2, bi->blockend_e);
				if ((gtk_text_iter_compare(it,changestart)<0 && gtk_text_iter_compare(&it2,changestart) < 0)
							 || (gtk_text_iter_compare(it,changeend)>0 && gtk_text_iter_compare(&it2,changeend)>0)) {
					/* the block ends before the start of the changed area: skip */
					DEBUGSC_MSG("block_unchanged_and_known, skipping block from %d (line %d) to %d (line %d)\n", gtk_text_iter_get_offset(it),gtk_text_iter_get_line(it),gtk_text_iter_get_offset(&it2),gtk_text_iter_get_line(&it2));
					/* the block is finished before the first changed text, so we can skip to the end of the block with scanning */
#ifdef HL_PROFILING
					numskipped += (gtk_text_iter_get_offset(&it2)-gtk_text_iter_get_offset(it)-1);
#endif
					*it = it2;
					gtk_text_iter_backward_char(it);
					return TRUE;
				} else DEBUGSC_MSG("blocks %d to %d is within the changed area %d to %d\n",
								gtk_text_iter_get_offset(it),gtk_text_iter_get_offset(&it2),
								gtk_text_iter_get_offset(changestart),gtk_text_iter_get_offset(changeend));
				
			} else DEBUGSC_MSG("no existing blockinfo\n");
		} else DEBUGSC_MSG(__FILE__", no existing mark at %d (line %d)\n",gtk_text_iter_get_offset(it),gtk_text_iter_get_line(it));
	}
	return FALSE;
}
#endif

#define NEWREMOVETAGS
/* this function takes most of the CPU time in bluefish */
static void bf_textview_scan_area(BfTextView * self, GtkTextIter * startarg, GtkTextIter * endarg,
						   gboolean apply_hl, gboolean store_string, gboolean force_rehighlight)
{
	GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self));
	GtkTextIter its, ita;
	GtkTextIter start,end;
	BfState *current_state;
#if defined SCANALLTAGVIEWABLE || defined SKIP_KNOWN_UNCHANGED_BLOCKS
	GtkTextIter startvisible,endvisible;
#endif
#ifdef HL_PROFILING
	gint numchars=0, numloop=0;
	numskipped=0;
#endif
	DEBUG_MSG("bf_textview_scan_area, started from %d to %d\n",gtk_text_iter_get_offset(start),gtk_text_iter_get_offset(end));
	g_return_if_fail(self != NULL);
	g_return_if_fail(BF_IS_TEXTVIEW(self));
	if (!self->lang || self->paste_operation)
		return;
	if (self->delay_rescan)
		return;

#if defined SCANALLTAGVIEWABLE || defined SKIP_KNOWN_UNCHANGED_BLOCKS
	startvisible = *startarg;
	endvisible = *endarg;
	gtk_text_buffer_get_bounds(buf,&start,&end);
	its = ita = start; 
#else
	its = ita = start = *startarg;
	end = *endarg;
#endif

	self->scanner.current_context = NULL;
	current_state = &self->lang->scan_table;

#ifdef HL_PROFILING
	times(&tms1);
#endif

	g_signal_handler_block(buf, self->markset_signal_id);

	while (g_queue_pop_head(&(self->scanner.block_stack)) != NULL) {
	};
	while (g_queue_pop_head(&(self->scanner.tag_stack)) != NULL) {
	};
#ifdef HL_PROFILING
	times(&tms1a);
#endif
	bftv_clear_block_cache(self);
	bftv_clear_matched_block(self);
#ifdef HL_PROFILING
	times(&tms1b);
#endif

	if (apply_hl) {
#ifdef SKIP_KNOWN_UNCHANGED_BLOCKS
		if (force_rehighlight) {
			gtk_text_buffer_remove_all_tags(buf,&start,&end);
		}
#endif /* SKIP_KNOWN_UNCHANGED_BLOCKS */
#ifdef HL_PROFILING
	times(&tms1c);
#endif
#ifdef REUSETAGS
	/* do not remove any tags yet */
#else
#ifdef NEWREMOVETAGS
		/* We now simply remove ALL tags from the buffer. This means that we also remove tags 
		that have special meaning (inside a block) and tags that are set by other code..
		It could be that we have to remove tags more specifically, and leave some of the tags 
		in the buffer. But removing all tags in one go is so much more faster that 
		I'm going to see if we can keep it like that */

		/* BUG: if there are other (non-scanner-related) tokens in this block they are removed too !!! */
		gtk_text_buffer_remove_all_tags(buf,&start,&end);
		DEBUGTAGS_MSG("%s:%d, removing all tags\n",__FILE__,__LINE__);
#else
		
		if (self->lang->tag_begin) {
			DEBUG_TEXTTAG_MSG("apply tag %p (%s:%d)\n",self->lang->tag_begin,__FILE__,__LINE__);
			gtk_text_buffer_remove_tag(buf, self->lang->tag_begin, start, end);
		}
		if (self->lang->tag_end) {
			DEBUG_TEXTTAG_MSG("apply tag %p (%s:%d)\n",self->lang->tag_end,__FILE__,__LINE__);
			gtk_text_buffer_remove_tag(buf, self->lang->tag_end, start, end);
		}
		if (self->lang->attr_name) {
			DEBUG_TEXTTAG_MSG("apply tag %p (%s:%d)\n",self->lang->attr_name,__FILE__,__LINE__);
			gtk_text_buffer_remove_tag(buf, self->lang->attr_name, start, end);
		}
		if (self->lang->attr_val) {
			DEBUG_TEXTTAG_MSG("apply tag %p (%s:%d)\n",self->lang->attr_val,__FILE__,__LINE__);
			gtk_text_buffer_remove_tag(buf, self->lang->attr_val, start, end);*/
		}

		/* these next two take TOO MUCH TIME because they are called for each token, but many tokens
		actually have the same tag.... */
		rts.buffer = buf;
		rts.start = start;
		rts.end = end;
		g_hash_table_foreach(self->lang->tokens, bftv_remove_t_tag, &rts);
		g_hash_table_foreach(self->lang->blocks, bftv_remove_b_tag, &rts);
#endif
#endif 
	}
#ifdef HL_PROFILING
	times(&tms2);
#endif
/* THIS IS THE NEW MAIN LOOP */
	{
		gboolean last_loop = FALSE;
		gboolean rescan_character;
		gboolean block_found = FALSE;
#ifdef SCANALLTAGVIEWABLE
		const gboolean end_is_buffer_end = TRUE;
		gboolean in_visible_area;
#else
		gboolean end_is_buffer_end = gtk_text_iter_is_end(&end);
#endif 
		while (!last_loop) {
			gunichar c;
#ifdef HL_PROFILING
			numchars++;
#endif
			c = gtk_text_iter_get_char(&ita);
			/*g_print("%s:%d got character %c at position %d\n",__FILE__, __LINE__,c,gtk_text_iter_get_offset(&ita));*/
			if (c == 0 || (!end_is_buffer_end && gtk_text_iter_equal(&ita, &end))) {
				last_loop = TRUE;
			} 
#ifdef SCANALLTAGVIEWABLE		
			in_visible_area = gtk_text_iter_in_range(&ita,&startvisible,&endvisible);
#endif 			
			if (force_rehighlight || gtk_text_iter_in_range(&ita,&startvisible,&endvisible)) {
				GtkTextMark *mark = bftv_get_block_at_iter(&ita);
				if (mark)
					blockinfo_free_bymark(buf, mark);
			}
			if (store_string) {
				DEBUG_MSG("store string! append character %c\n",c);
				/* the next line can probably be improved a bit for speed  */
				if (!g_unichar_isgraph(c)) {
					self->scanner.last_string = g_string_assign(self->scanner.last_string, "");
				} else if (!block_found) {
					g_string_append_printf(self->scanner.last_string, "%c", c); /* this is slowwwww because it is called VERY often */
				}
			}
			if (last_loop) {
				/* This is a trick. Character of code 3(ETX) is not printable, so will not appear in the text,
				   but automata has entries for it, so if at the end of text is token - it will 
				   be recognized */
				c = 3;
			} else {
				if ((gint) c > BFTV_SCAN_RANGE) {
					c = 0;
				}
				while (self->lang->escapes[(gint) c] && c != 0 ) {	/* remove escapes */
					gtk_text_iter_forward_chars(&ita, 2);
					c = gtk_text_iter_get_char(&ita);
				}						/* remove escapes */
			}
	
			/* #####################     RECOGNIZE TOKENS AND BLOCKS ###################*/
			rescan_character = FALSE;
			do {
#ifdef HL_PROFILING
				numloop++;
#endif
				block_found = FALSE;
				if (!current_state) {
					if (self->scanner.current_context) {
						current_state = &self->scanner.current_context->scan_table;
					} else
						current_state = &self->lang->scan_table;
				}
				if (self->lang->case_sensitive)
					current_state = current_state->tv[(gint8) c];
				else
					current_state = current_state->tv[(gint8) toupper(c)];
				
				if (current_state) {
					DEBUGSC_MSG("%s:%d, new state after character %d(%c) has type %d\n",__FILE__,__LINE__,gtk_text_iter_get_offset(&ita),c,current_state->type);
					if (current_state->type == ST_TRANSIT) {
#ifdef REUSETAGS
#ifdef SCANALLTAGVIEWABLE		
						if (in_visible_area && !rescan_character && !gtk_text_iter_equal(&its,&ita)) {
							remove_tags_starting_at_iter(buf, &ita);
						}
#else
						if (!force_rehighlight && !rescan_character && !gtk_text_iter_equal(&its,&ita)) {
							/*g_print("%s:%d calling remove_tags_starting_at_iter, position %d\n",__FILE__, __LINE__,gtk_text_iter_get_offset(&ita));*/
							remove_tags_starting_at_iter(buf, &ita);
						}
#endif
#endif
						gtk_text_iter_forward_char(&ita);
						rescan_character = FALSE;
					} else {
						DEBUGSC_MSG(__FILE__":%d found type %d at pos %d\n", __LINE__,current_state->type,gtk_text_iter_get_offset(&ita));
						switch (current_state->type) {
						case ST_TOKEN:
#ifdef SCANALLTAGVIEWABLE
							bf_textview_scan_state_type_st_token(self, buf, current_state, &its, &ita, (apply_hl && in_visible_area));
#else
							bf_textview_scan_state_type_st_token(self, buf, current_state, &its, &ita, apply_hl);
#endif
							current_state = NULL;
							break;			/* token */
						case ST_BLOCK_BEGIN:
#ifdef SKIP_KNOWN_UNCHANGED_BLOCKS
							/* the mark with bi2 is set at the start of the block, so ita and not its */
							if (!force_rehighlight && block_unchanged_and_known(buf, &its, &startvisible,&endvisible)) {
								ita = its;
								/* keep same scanning state */
								TBfBlock *aux = (TBfBlock *) g_queue_peek_head(&(self->scanner.block_stack));
#ifdef DEBUGSC								
								if (aux && aux->def) g_print("set context %s\n",aux->def->name);
#endif
								if (aux)
									self->scanner.current_context = aux->def;
								else
									self->scanner.current_context = NULL;
								if (self->scanner.current_context)
									current_state = &self->scanner.current_context->scan_table;
								else
									current_state = &self->lang->scan_table;					
							} else {
								current_state = bf_textview_scan_state_type_st_block_begin(self, buf, current_state, &its, &ita, apply_hl);
								block_found = TRUE;
							}						
#else
#ifdef SCANALLTAGVIEWABLE
							current_state = bf_textview_scan_state_type_st_block_begin(self, buf, current_state, &its, &ita, (apply_hl && in_visible_area));
#else
							current_state = bf_textview_scan_state_type_st_block_begin(self, buf, current_state, &its, &ita, apply_hl);
#endif
							block_found = TRUE;
#endif
							break;
						case ST_BLOCK_END:
#ifdef SCANALLTAGVIEWABLE
							current_state = bf_textview_scan_state_type_st_block_end(self, buf, current_state, &its, &ita, (apply_hl && in_visible_area));
#else
							current_state = bf_textview_scan_state_type_st_block_end(self, buf, current_state, &its, &ita, apply_hl);
#endif
							if (current_state == NULL && gtk_text_iter_is_end(&ita)) {
								/* auto closing a tag automatically triggers a new scan */
								return;
							}
							block_found = TRUE;
							break;
						}
						its = ita;
						/* why don't we use gtk_text_iter_forward_char(&ita); here? --> the highlightig doesn't work anymore
						we run the loop another time on the same position ?? what kind of magic is
						in the automata that required us to re-run the loop? 
						==> because a pattern [any]* or + requires a character that matches the NOT condition
						to stop the matching on the previous position !!!! */
						rescan_character = TRUE;
						DEBUGSC_MSG("%s:%d, rescan_character=%d\n",__FILE__,__LINE__,rescan_character);
					}
				} else {	/* current_state is NULL */
					DEBUGSC_MSG("%s:%d, current_state=NULL after character %d(%c)\n",__FILE__,__LINE__,gtk_text_iter_get_offset(&ita),c);
#ifdef SCANALLTAGVIEWABLE	
					if (in_visible_area && !rescan_character) {
						/*g_print("%s:%d calling remove_tags_starting_at_iter, position %d\n",__FILE__, __LINE__,gtk_text_iter_get_offset(&ita));*/
						remove_tags_starting_at_iter(buf, &ita);
						if (!gtk_text_iter_equal(&its,&ita)) {
							/*g_print("%s:%d calling remove_tags_starting_at_iter, position %d\n",__FILE__, __LINE__,gtk_text_iter_get_offset(&its));*/
							remove_tags_starting_at_iter(buf, &its);
						}
					}
#else
					if (!rescan_character && !force_rehighlight) {
						DEBUGTAGS_MSG("%s:%d calling remove_tags_starting_at_iter, position %d\n",__FILE__, __LINE__,gtk_text_iter_get_offset(&ita));
						remove_tags_starting_at_iter(buf, &ita);
						if (!gtk_text_iter_equal(&its,&ita)) {
							DEBUGTAGS_MSG("%s:%d calling remove_tags_starting_at_iter, position %d\n",__FILE__, __LINE__,gtk_text_iter_get_offset(&its));
							remove_tags_starting_at_iter(buf, &its);
						}
					} else {
						DEBUGTAGS_MSG("%s:%d rescan_character=TRUE --> not removing tags at %d\n",__FILE__, __LINE__,gtk_text_iter_get_offset(&its));
					}

#endif					
					if (rescan_character) {
						gtk_text_iter_forward_char(&ita);
						rescan_character = FALSE;
					} else {
						rescan_character = TRUE;
					}
					its = ita;
				}
			} while (rescan_character);
		}	/*main loop */
	}
#ifdef HL_PROFILING
	times(&tms3);
#endif

/* Clear stacks */
	while (!g_queue_is_empty(&self->scanner.block_stack)) {
		TBfBlock *bf = (TBfBlock *) g_queue_pop_head(&self->scanner.block_stack);
		if (bf->tagname)
			g_free(bf->tagname);
		g_slice_free(TBfBlock,bf);
	}
	while (!g_queue_is_empty(&self->scanner.tag_stack)) {
		TBfBlock *bf = (TBfBlock *) g_queue_pop_head(&self->scanner.tag_stack);
		if (bf->tagname)
			g_free(bf->tagname);
		g_slice_free(TBfBlock,bf);
	}

#ifdef HL_PROFILING
	{
		gint totalchars;
		glong tot_ms=0,first_stage_ms = 0, loop_ms=0,finalizing_ms=0;
		glong t1a_ms=0,t1ab_ms=0,t1bc_ms=0,t1c2_ms=0;
		gint marks = 0, allmarks = 0, tags = 0;
		GSList *ss = NULL;
		GtkTextIter pit;
		times(&tms4);
		totalchars = gtk_text_buffer_get_char_count(buf);
		tot_ms = (glong) (double) ((tms4.tms_utime - tms1.tms_utime) * 1000 / sysconf(_SC_CLK_TCK));
		first_stage_ms = (glong) (double) ((tms2.tms_utime - tms1.tms_utime) * 1000 / sysconf(_SC_CLK_TCK));
		loop_ms = (glong) (double) ((tms3.tms_utime - tms2.tms_utime) * 1000 / sysconf(_SC_CLK_TCK));
		finalizing_ms = (glong) (double) ((tms4.tms_utime - tms3.tms_utime) * 1000 / sysconf(_SC_CLK_TCK));
		t1a_ms =  (glong) (double) ((tms1a.tms_utime - tms1.tms_utime) * 1000 / sysconf(_SC_CLK_TCK));
		t1ab_ms =  (glong) (double) ((tms1b.tms_utime - tms1a.tms_utime) * 1000 / sysconf(_SC_CLK_TCK));
		t1bc_ms =  (glong) (double) ((tms1c.tms_utime - tms1b.tms_utime) * 1000 / sysconf(_SC_CLK_TCK));
		t1c2_ms =  (glong) (double) ((tms2.tms_utime - tms1c.tms_utime) * 1000 / sysconf(_SC_CLK_TCK));
		g_print("bf_textview_scan_area, PROFILING\n%d total characters, retrieved %d characters (%d%%), looped %d times, skipped %d characters (%d%%)\n", totalchars,numchars,(int)(100*numchars/totalchars),numloop,numskipped,(int)(100*numskipped/totalchars));
		g_print("total time: %ld ms\n", tot_ms);
		
		g_print("   preparing : %ld ms\n", first_stage_ms);
		g_print("   prep 1 : %ld ms\n", t1a_ms);
		g_print("   prep 2 : %ld ms\n", t1ab_ms);
		g_print("   prep 3 : %ld ms\n", t1bc_ms);
		g_print("   prep 4 : %ld ms\n", t1c2_ms);
		g_print("   loop      : %ld ms\n", loop_ms);
		g_print("   finalizing: %ld ms\n", finalizing_ms);
		
		its = start;
		ita = end;
		pit = its;
		while (!gtk_text_iter_equal(&pit, &ita)) {
			ss = gtk_text_iter_get_marks(&pit);
			while (ss) {
				allmarks++;
				if (!gtk_text_mark_get_deleted(ss->data)) {
					marks++;
				}
				ss = g_slist_next(ss);
			}
			g_slist_free(ss);
			ss = gtk_text_iter_get_toggled_tags(&pit, TRUE);
			while (ss) {
				tags++;
				ss = g_slist_next(ss);
			}
			g_slist_free(ss);
			gtk_text_iter_forward_char(&pit);
		}
		g_print("In area: total number of existing marks: %d, all marks: %d total number of tags: %d\n",
				marks, allmarks, tags);
	}
#endif
	g_signal_handler_unblock(buf, self->markset_signal_id);
}

/* -------------------- /SCANNING  -------------------------------*/

/* -------------------- MISC  -------------------------------*/
void bf_textview_set_language_ptr(BfTextView * self, BfLangConfig * cfg)
{
	self->lang = cfg;
	if (cfg)
		bf_textview_scan(self);
}

void bf_textview_set_show_lines(BfTextView * self, gboolean show)
{
	PangoLayout *l = gtk_widget_create_pango_layout(GTK_WIDGET(self), "");
	gint numlines = gtk_text_buffer_get_line_count(gtk_text_view_get_buffer(GTK_TEXT_VIEW(self)));
	gchar *pomstr = g_strdup_printf("%d", MAX(99, numlines));
	gint w;

	pango_layout_set_text(l, pomstr, -1);
	g_free(pomstr);
	pango_layout_get_pixel_size(l, &w, NULL);
	g_object_unref(G_OBJECT(l));
	self->show_lines = show;
	if (show) {
		self->lw_size_lines = w + 4;
	} else {
		self->lw_size_lines = 0;
	}
	gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(self), GTK_TEXT_WINDOW_LEFT,
										 self->lw_size_lines + self->lw_size_blocks +
										 self->lw_size_sym);
}

void bf_textview_set_show_symbols(BfTextView * self, gboolean show)
{
	self->show_symbols = show;
	if (show) {
		self->lw_size_sym = 14;
	} else {
		self->lw_size_sym = 0;
	}
	gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(self), GTK_TEXT_WINDOW_LEFT,
										 self->lw_size_lines + self->lw_size_blocks +
										 self->lw_size_sym);
	gtk_widget_queue_draw(GTK_WIDGET(self));
}

void bf_textview_set_show_blocks(BfTextView * self, gboolean show)
{
	self->show_blocks = show;
	if (show) {
		self->lw_size_blocks = 15;
	} else {
		self->lw_size_blocks = 0;
	}
	gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(self), GTK_TEXT_WINDOW_LEFT,
										 self->lw_size_lines + self->lw_size_blocks +
										 self->lw_size_sym);
}

void bf_textview_set_show_rmargin(BfTextView * self, gboolean show, gint column)
{
	self->show_rmargin = show;
	if (column > 0)
		self->rmargin_at = column;
}

void bf_textview_set_highlight(BfTextView * self, gboolean on)
{
	self->highlight = on;
}

void bf_textview_set_match_blocks(BfTextView * self, gboolean on)
{
	self->match_blocks = on;
}

void bf_textview_set_delay_rescan(BfTextView * self, gboolean on)
{
	self->delay_rescan = on;
}

gboolean bf_textview_add_symbol(BfTextView * self, gchar * name, GdkPixbuf * pix)
{
	BfTextViewSymbol *sym;
	gpointer ptr = g_hash_table_lookup(self->symbols, name);
	if (ptr)
		return FALSE;
	sym = g_slice_new0(BfTextViewSymbol);
	sym->name = g_strdup(name);
	sym->pixmap = gdk_pixbuf_scale_simple(pix, 10, 10, GDK_INTERP_BILINEAR);
	g_hash_table_insert(self->symbols, name, sym);
	return TRUE;
}

static gboolean bftv_hash_remove(gpointer key, gpointer val, gpointer udata)
{
	BfTextViewSymbol *sym = (BfTextViewSymbol *) val;
	if (strcmp(sym->name, (gchar *) udata) == 0)
		return TRUE;
	return FALSE;
}

void bf_textview_remove_symbol(BfTextView * self, gchar * name)
{
	gpointer ptr = g_hash_table_lookup(self->symbols, name);
	if (ptr) {
		g_hash_table_foreach_remove(self->symbol_lines, bftv_hash_remove, name);
		g_hash_table_remove(self->symbols, name);
		g_free(((BfTextViewSymbol *) ptr)->name);
		g_free(((BfTextViewSymbol *) ptr)->pixmap);
		g_slice_free(BfTextViewSymbol, ptr);
		gtk_widget_queue_draw(GTK_WIDGET(self));
	}
}

void bf_textview_set_symbol(BfTextView * self, gchar * name, gint line, gboolean set)
{
	gpointer ptr = g_hash_table_lookup(self->symbols, name);
	GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self));
	GtkTextIter it;
	GtkTextMark *mark;
	GSList *lst;

	if (!ptr)
		return;
	gtk_text_buffer_get_iter_at_line(buf, &it, line);
	if (set) {
		mark = gtk_text_buffer_create_mark(buf, NULL, &it, TRUE);
		g_hash_table_insert(self->symbol_lines, mark, ptr);
	} else {
		mark = NULL;
		lst = gtk_text_iter_get_marks(&it);
		while (lst) {
			if (g_hash_table_lookup(self->symbol_lines, lst->data)) {
				mark = GTK_TEXT_MARK(lst->data);
				break;
			}
			lst = g_slist_next(lst);
		}
		g_slist_free(lst);
		if (mark)
			g_hash_table_remove(self->symbol_lines, mark);
	}
	gtk_widget_queue_draw(GTK_WIDGET(self));
}

/* These functions are taken stright from GTK+ sources (www.gtk.org), because I need to
compute color shading for line highlight and left window  */

static void rgb_to_hls(gdouble * r, gdouble * g, gdouble * b)
{
	gdouble min;
	gdouble max;
	gdouble red;
	gdouble green;
	gdouble blue;
	gdouble h, l, s;
	gdouble delta;

	red = *r;
	green = *g;
	blue = *b;

	if (red > green) {
		if (red > blue)
			max = red;
		else
			max = blue;

		if (green < blue)
			min = green;
		else
			min = blue;
	} else {
		if (green > blue)
			max = green;
		else
			max = blue;

		if (red < blue)
			min = red;
		else
			min = blue;
	}

	l = (max + min) / 2;
	s = 0;
	h = 0;

	if (max != min) {
		if (l <= 0.5)
			s = (max - min) / (max + min);
		else
			s = (max - min) / (2 - max - min);

		delta = max - min;
		if (red == max)
			h = (green - blue) / delta;
		else if (green == max)
			h = 2 + (blue - red) / delta;
		else if (blue == max)
			h = 4 + (red - green) / delta;

		h *= 60;
		if (h < 0.0)
			h += 360;
	}

	*r = h;
	*g = l;
	*b = s;
}

static void hls_to_rgb(gdouble * h, gdouble * l, gdouble * s)
{
	gdouble hue;
	gdouble lightness;
	gdouble saturation;
	gdouble m1, m2;
	gdouble r, g, b;

	lightness = *l;
	saturation = *s;

	if (lightness <= 0.5)
		m2 = lightness * (1 + saturation);
	else
		m2 = lightness + saturation - lightness * saturation;
	m1 = 2 * lightness - m2;

	if (saturation == 0) {
		*h = lightness;
		*l = lightness;
		*s = lightness;
	} else {
		hue = *h + 120;
		while (hue > 360)
			hue -= 360;
		while (hue < 0)
			hue += 360;

		if (hue < 60)
			r = m1 + (m2 - m1) * hue / 60;
		else if (hue < 180)
			r = m2;
		else if (hue < 240)
			r = m1 + (m2 - m1) * (240 - hue) / 60;
		else
			r = m1;

		hue = *h;
		while (hue > 360)
			hue -= 360;
		while (hue < 0)
			hue += 360;

		if (hue < 60)
			g = m1 + (m2 - m1) * hue / 60;
		else if (hue < 180)
			g = m2;
		else if (hue < 240)
			g = m1 + (m2 - m1) * (240 - hue) / 60;
		else
			g = m1;

		hue = *h - 120;
		while (hue > 360)
			hue -= 360;
		while (hue < 0)
			hue += 360;

		if (hue < 60)
			b = m1 + (m2 - m1) * hue / 60;
		else if (hue < 180)
			b = m2;
		else if (hue < 240)
			b = m1 + (m2 - m1) * (240 - hue) / 60;
		else
			b = m1;

		*h = r;
		*l = g;
		*s = b;
	}
}

/* modified function to darken/lighten given color */
static void gdk_color_dl(GdkColor * a, GdkColor * b, gdouble k)
{
	gdouble red;
	gdouble green;
	gdouble blue;

	red = (gdouble) a->red / 65535.0;
	green = (gdouble) a->green / 65535.0;
	blue = (gdouble) a->blue / 65535.0;
	rgb_to_hls(&red, &green, &blue);
	if (green > 0.5)
		green -= k;
	else
		green += k;
	hls_to_rgb(&red, &green, &blue);
	b->red = red * 65535.0;
	b->green = green * 65535.0;
	b->blue = blue * 65535.0;
}


void bf_textview_recolor(BfTextView * view, gchar * fg_color, gchar * bg_color)
{
	gchar *str, *fgn, *fga, *bgn, *bga;
	GdkColor c1, c2, c3, c4;
	GtkRcStyle *style;

	g_stpcpy(view->fg_color, fg_color);
	g_stpcpy(view->bkg_color, bg_color);
	gdk_color_parse(fg_color, &c1);
	gdk_color_parse(bg_color, &c2);
	gdk_color_dl(&c1, &c3, 0.07);
	gdk_color_dl(&c2, &c4, 0.07);
	fgn = gdk_color_to_hexstring(&c1, FALSE);
	fga = gdk_color_to_hexstring(&c3, FALSE);
	bgn = gdk_color_to_hexstring(&c4, FALSE);
	bga = gdk_color_to_hexstring(&c4, FALSE);

	str =
		g_strdup_printf
		("style \"bfish\" {\nGtkWidget::cursor_color=\"%s\"\nbase[NORMAL]=\"%s\"\nbase[ACTIVE]=\"%s\"\ntext[NORMAL]=\"%s\"\ntext[ACTIVE]=\"%s\"\nfg[NORMAL]=\"%s\"\nfg[ACTIVE]=\"%s\"\nbg[NORMAL]=\"%s\"\nbg[ACTIVE]=\"%s\"\n}\nclass \"BfTextView\" style \"bfish\"",
		 view->fg_color, view->bkg_color, view->bkg_color, view->fg_color, view->fg_color, fgn, fga,
		 bgn, bga);

	gtk_rc_parse_string(str);
	style = gtk_widget_get_modifier_style(GTK_WIDGET(view));
	gtk_widget_modify_style(GTK_WIDGET(view), style);

	g_free(fgn);
	g_free(fga);
	g_free(bgn);
	g_free(bga);
	g_free(str);
}

/* -------------------- /MISC  -------------------------------*/

void bf_textview_show_lines(BfTextView * self, gboolean show)
{
	bf_textview_set_show_lines(self, show);
}

void bf_textview_show_symbols(BfTextView * self, gboolean show)
{
	bf_textview_set_show_symbols(self, show);
}

void bf_textview_show_rmargin(BfTextView * self, gboolean show, gint column)
{
	bf_textview_set_show_rmargin(self, show, column);
}

void bf_textview_show_blocks(BfTextView * self, gboolean show)
{
	bf_textview_set_show_blocks(self, show);
}

/*-----------------------      Autocompletion  ---------------------------------------*/

void bf_textview_autocomp_show(BfTextView * self)
{
	GtkTextIter it, it3;
	GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self));
	if (!self->lang)
		return;
	gtk_text_buffer_get_iter_at_mark(buf, &it, gtk_text_buffer_get_insert(buf));
	it3 = it;
	gtk_text_iter_set_line(&it3, gtk_text_iter_get_line(&it));
	gtk_text_iter_backward_char(&it);
	self->scanner.last_string = g_string_assign(self->scanner.last_string, "");
	DEBUG_MSG("bf_textview_autocomp_show, calling scan_area()\n");
	bf_textview_scan_area(self, &it3, &it, FALSE, TRUE, FALSE);
	DEBUG_MSG("bf_textview_autocomp_show, last_string=%s\n",self->scanner.last_string->str);
	if (self->lang && self->lang->schema_aware) {
		DEBUG_MSG("bf_textview_autocomp_show, running in 'schema_aware' mode\n");
		if (self->scanner.last_string->str[0] == '<')
			ac_run_schema(main_v->autocompletion, self->scanner.last_string->str + 1, self->schemas,
						  self->internal_dtd, GTK_TEXT_VIEW(self), NULL);
		else if (self->scanner.current_context
				 && self->scanner.current_context->type == BT_TAG_BEGIN) {
			ac_run_tag_attributes(main_v->autocompletion, self->scanner.last_tagname,
								  self->scanner.last_string->str, self->schemas, self->internal_dtd,
								  GTK_TEXT_VIEW(self), "=\"\"");
		} else {
			if (self->lang->case_sensitive)
				ac_run_lang(main_v->autocompletion, self->scanner.last_string->str,
							(gchar *) (self->lang->name), GTK_TEXT_VIEW(self), NULL);
			else {
				gchar *upper =
					g_utf8_strup(self->scanner.last_string->str, self->scanner.last_string->len);
				ac_run_lang(main_v->autocompletion, upper, (gchar *) (self->lang->name),
							GTK_TEXT_VIEW(self), NULL);
				g_free(upper);
			}
		}
	} else {
		DEBUG_MSG("bf_textview_autocomp_show, not 'schema_aware', calling ac_run_lang()\n");
		if (self->lang->case_sensitive)
			ac_run_lang(main_v->autocompletion, self->scanner.last_string->str,
						(gchar *) (self->lang->name), GTK_TEXT_VIEW(self), NULL);
		else {
			gchar *upper =
				g_utf8_strup(self->scanner.last_string->str, self->scanner.last_string->len);
			ac_run_lang(main_v->autocompletion, upper, (gchar *) (self->lang->name),
						GTK_TEXT_VIEW(self), NULL);
			g_free(upper);
		}
	}
}
