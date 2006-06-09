/* Bluefish HTML Editor
 * bf-textview.c - Bluefish text widget
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
 *
 * indenting is done with
 * indent --line-length 100 --k-and-r-style --tab-size 4 -bbo --ignore-newlines bf-textview.c
 */

/* #define DEBUG */

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

*/

#include "config.h"
#ifdef USE_SCANNER

#include "bf-textview.h"
#include "bluefish.h"
#include "textstyle.h"
#include "bf_lib.h"
#include "gtk_easy.h"			/* gdk_color_to_hexstring */


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
struct tms tms2;
#endif							/* HL_PROFILING */

enum {
	BI_START,
	BI_END
};

typedef struct {
	guint8 type;
	gboolean folded;
	gchar *tagname;
	GtkTextMark *ref, *refb1, *refb2, *refe1, *refe2;
	gboolean single_line;
	gpointer data;
} BlockInfo;

static char folded_xbm[] = {
	0x02, 0x01
};

static gshort const_true = 1;

static GtkTextViewClass *parent_class = NULL;


/* internal functions */
static gboolean bftv_xml_bool(xmlChar * text);
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
static void bftv_delete_blocks_from_area(BfTextView * view, GtkTextIter * arg1, GtkTextIter * arg2);
static void bftv_fold(BfTextView * self, GtkTextMark * mark, gboolean move_cursor);
static void bftv_expand_all(GtkWidget * widget, BfTextView * view);
static void bftv_collapse_all(GtkWidget * widget, BfTextView * view);
static void bftv_clear_block_cache(BfTextView * self);
static void bftv_clear_matched_block(BfTextView * self);
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
	o->hl_mode = BFTV_HL_MODE_VISIBLE;
	o->show_current_line = main_v->props.view_cline;
	o->tag_autoclose = FALSE;
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
	GdkBitmap *bmp;

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
	o->internal_tags[IT_FOLDED] =
		gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table
								  (gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "_folded_");
	if (!o->internal_tags[IT_FOLDED])
		o->internal_tags[IT_FOLDED] =
			gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o)), "_folded_",
									   "editable", FALSE, "invisible", TRUE, NULL);
	o->internal_tags[IT_FOLD_HEADER] =
		gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table
								  (gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "_fold_header_");
	if (!o->internal_tags[IT_FOLD_HEADER]) {
		bmp = gdk_bitmap_create_from_data(NULL, folded_xbm, 2, 2);
		o->internal_tags[IT_FOLD_HEADER] =
			gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o)), "_fold_header_",
									   "editable", FALSE, "background", "#F7F3D2",
									   "foreground-stipple", bmp, NULL);
		g_object_unref(bmp);
	}
	o->internal_tags[IT_BLOCK_MATCH] =
		gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table
								  (gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "_block_match_");
	if (!o->internal_tags[IT_BLOCK_MATCH])
		o->internal_tags[IT_BLOCK_MATCH] =
			gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o)), "_block_match_",
									   "background", "#F7F3D2", NULL);
	o->internal_tags[IT_BLOCK] =
		gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table
								  (gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "_block_");
	if (!o->internal_tags[IT_BLOCK])
		o->internal_tags[IT_BLOCK] =
			gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o)), "_block_", NULL);
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
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(o), buffer);
	GdkBitmap *bmp;

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
	o->internal_tags[IT_FOLDED] =
		gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table
								  (gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "_folded_");
	if (!o->internal_tags[IT_FOLDED])
		o->internal_tags[IT_FOLDED] =
			gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o)), "_folded_",
									   "editable", FALSE, "invisible", TRUE, NULL);
	o->internal_tags[IT_FOLD_HEADER] =
		gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table
								  (gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "_fold_header_");
	if (!o->internal_tags[IT_FOLD_HEADER]) {
		bmp = gdk_bitmap_create_from_data(NULL, folded_xbm, 2, 2);
		o->internal_tags[IT_FOLD_HEADER] =
			gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o)), "_fold_header_",
									   "editable", FALSE, "background", "#F7F3D2",
									   "foreground-stipple", bmp, NULL);
		g_object_unref(bmp);
	}
	o->internal_tags[IT_BLOCK_MATCH] =
		gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table
								  (gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "_block_match_");
	if (!o->internal_tags[IT_BLOCK_MATCH])
		o->internal_tags[IT_BLOCK_MATCH] =
			gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o)), "_block_match_",
									   "background", "#F7F3D2", NULL);
	o->internal_tags[IT_BLOCK] =
		gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table
								  (gtk_text_view_get_buffer(GTK_TEXT_VIEW(o))), "_block_");
	if (!o->internal_tags[IT_BLOCK])
		o->internal_tags[IT_BLOCK] =
			gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(o)), "_block_", NULL);
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

static void bf_textview_mark_set_cb(GtkTextBuffer * buf, GtkTextIter * arg1, GtkTextMark * arg2,
									gpointer widget)
{
	GtkTextIter it, it2;
	GtkTextMark *block = NULL;
	BlockInfo *bi;

	if (arg2 && gtk_text_buffer_get_insert(buf) == arg2) {
		/* insert set which means cursor position changed */
		if (BF_TEXTVIEW(widget)->last_matched_block) {
			block = BF_TEXTVIEW(widget)->last_matched_block;
			bi = (BlockInfo *) g_object_get_data(G_OBJECT(block), "bi");
			gtk_text_buffer_get_iter_at_mark(buf, &it, block);
			gtk_text_buffer_get_iter_at_mark(buf, &it2, bi->ref);
			gtk_text_buffer_remove_tag(buf, BF_TEXTVIEW(widget)->internal_tags[IT_BLOCK_MATCH], &it,
									   &it2);
			if (bi->type == BI_START) {
				gtk_text_buffer_get_iter_at_mark(buf, &it, bi->refe1);
				gtk_text_buffer_get_iter_at_mark(buf, &it2, bi->refe2);
			} else if (bi->type == BI_END) {
				gtk_text_buffer_get_iter_at_mark(buf, &it, bi->refb1);
				gtk_text_buffer_get_iter_at_mark(buf, &it2, bi->refb2);
			}
			gtk_text_buffer_remove_tag(buf, BF_TEXTVIEW(widget)->internal_tags[IT_BLOCK_MATCH], &it,
									   &it2);
			block = NULL;
		}
		gtk_text_buffer_get_iter_at_mark(buf, &it, gtk_text_buffer_get_insert(buf));
		block = bftv_get_block_at_iter(&it);
		if (block)
			bi = (BlockInfo *) g_object_get_data(G_OBJECT(block), "bi");
		else
			bi = NULL;
		if (block && bi->folded)
			block = NULL;
		if (block) {
			gtk_text_buffer_get_iter_at_mark(buf, &it, block);
			gtk_text_buffer_get_iter_at_mark(buf, &it2, bi->ref);
			gtk_text_buffer_apply_tag(buf, BF_TEXTVIEW(widget)->internal_tags[IT_BLOCK_MATCH], &it,
									  &it2);
			if (bi->type == BI_START) {
				gtk_text_buffer_get_iter_at_mark(buf, &it, bi->refe1);
				gtk_text_buffer_get_iter_at_mark(buf, &it2, bi->refe2);
			} else if (bi->type == BI_END) {
				gtk_text_buffer_get_iter_at_mark(buf, &it, bi->refb1);
				gtk_text_buffer_get_iter_at_mark(buf, &it2, bi->refb2);
			}
			gtk_text_buffer_apply_tag(buf, BF_TEXTVIEW(widget)->internal_tags[IT_BLOCK_MATCH], &it,
									  &it2);
			BF_TEXTVIEW(widget)->last_matched_block = block;
		}
	} else if (arg2 && arg2 == gtk_text_buffer_get_selection_bound(buf)) {
		gtk_text_buffer_get_iter_at_mark(buf, &it, gtk_text_buffer_get_insert(buf));
		gtk_text_buffer_get_iter_at_mark(buf, &it2, arg2);
		gtk_text_buffer_remove_tag(buf, BF_TEXTVIEW(widget)->internal_tags[IT_BLOCK_MATCH], &it,
								   &it2);
	}
}

static gboolean bf_textview_expose_cb(GtkWidget * widget, GdkEventExpose * event, gpointer doc)
{
	GdkWindow *left_win;
	GdkRectangle rect;
	GtkTextIter l_start, l_end, it, it2;
	GtkTextMark *block_mark;
	gint l_top1, numlines, w, l_top2, i, w2;
	PangoLayout *l;
	gchar *pomstr = NULL;
	gint pt_lines, pt_sym, pt_blocks, currline = 0;
	GdkGC *gc = NULL;
	gpointer aux;
	GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
	BlockInfo *bi;

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
			if (!gtk_text_iter_has_tag(&it, BF_TEXTVIEW(widget)->internal_tags[IT_FOLDED])) {
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

			if (!gtk_text_iter_has_tag(&it, BF_TEXTVIEW(widget)->internal_tags[IT_FOLDED])) {
				gc = gdk_gc_new(GDK_DRAWABLE(left_win));
				GSList *lst3 = gtk_text_iter_get_marks(&it);
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
			GtkTextMark *mark_begin = NULL, *mark_end = NULL;
			block_mark = bftv_get_first_block_at_line(BF_TEXTVIEW(widget), &it, TRUE);
			if (block_mark) {
				bi = (BlockInfo *) g_object_get_data(G_OBJECT(block_mark), "bi");
				mark_begin = block_mark;
				mark_end = bi->ref;
			} else
				bi = NULL;
			if (block_mark && bi && mark_end && mark_begin
				&& !gtk_text_iter_has_tag(&it, BF_TEXTVIEW(widget)->internal_tags[IT_FOLDED])) {
				gtk_text_buffer_get_iter_at_mark(buf, &it2, mark_end);
				gtk_text_view_get_line_yrange(GTK_TEXT_VIEW(widget), &it2, &w2, NULL);
				gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_LEFT,
													  0, w2, NULL, &w2);
				if (bi->type == BI_START) {
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
				DEBUG_MSG("checking if we're in tag %p\n",
						  BF_TEXTVIEW(widget)->internal_tags[IT_BLOCK]);
				if (gtk_text_iter_has_tag(&it, BF_TEXTVIEW(widget)->internal_tags[IT_BLOCK])
					&& !gtk_text_iter_has_tag(&it, BF_TEXTVIEW(widget)->internal_tags[IT_FOLDED])) {
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
		len = 0;
		while (len < g_utf8_strlen(string, stringlen)) {
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
		if (view->hl_mode == BFTV_HL_MODE_ALL || view->need_rescan) {
			bf_textview_scan(view);
		} else {
			bf_textview_scan_visible(view);
		}
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
	bftv_delete_blocks_from_area(view, arg1, arg2);
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
	if (GTK_WIDGET_VISIBLE(view) && view->delete_rescan) {
		if (view->hl_mode == BFTV_HL_MODE_ALL || view->need_rescan) {
			bf_textview_scan(view);
			view->delete_rescan = FALSE;
		} else {
			bf_textview_scan_visible(view);
			view->delete_rescan = FALSE;
		}
	} else if (view->delete_rescan) {
		view->need_rescan = TRUE;
	}
}


/* ---------------------    /EVENTS ---------------------------- */

/* ---------------------    UTILITY FUNCTIONS  ---------------------------- */
inline GtkTextMark *bftv_get_block_at_iter(GtkTextIter * it)
{
	GSList *lst = gtk_text_iter_get_marks(it);
	gpointer ptr = NULL;

	while (lst) {
		ptr = g_object_get_data(G_OBJECT(lst->data), "bi");
		if (ptr /* && (ptr == &tid_block_start || ptr == &tid_block_end) */ ) {
			return GTK_TEXT_MARK(lst->data);
		}
		lst = g_slist_next(lst);
	}
	g_slist_free(lst);
	return NULL;
}

static GtkTextMark *bftv_get_first_block_at_line(BfTextView * view, GtkTextIter * it,
												 gboolean not_single)
{
	GtkTextIter it2, it3;
	GtkTextMark *mark = NULL;
	gint *ln = NULL;
	gpointer ptr = NULL;
	BlockInfo *bi;


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
			bi = (BlockInfo *) g_object_get_data(G_OBJECT(mark), "bi");
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

static void bftv_delete_blocks_from_area(BfTextView * view, GtkTextIter * arg1, GtkTextIter * arg2)
{
	GtkTextBuffer *textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	GtkTextMark *mark = NULL, *mark2 = NULL, *mark3, *mark4;
	gboolean deleted = FALSE;
	GtkTextIter it, it2, it3;
	BlockInfo *bi, *bi2, *bi3, *bi4;

	if (!view->lang || !view->lang->scan_blocks)
		return;
	it = *arg1;
	while (gtk_text_iter_compare(&it, arg2) <= 0) {
		mark = bftv_get_block_at_iter(&it);
		if (mark) {
			bi = g_object_get_data(G_OBJECT(mark), "bi");
			if (bi && !bi->folded) {
				if (bi->type == BI_START) {
					mark2 = bi->ref;
					mark3 = bi->refe1;
					mark4 = bi->refe2;
					bi2 = g_object_get_data(G_OBJECT(bi->ref), "bi");
					bi3 = g_object_get_data(G_OBJECT(bi->refe1), "bi");
					bi4 = g_object_get_data(G_OBJECT(bi->refe2), "bi");
					gtk_text_buffer_get_iter_at_mark(textbuffer, &it2, mark);
					gtk_text_buffer_get_iter_at_mark(textbuffer, &it3, bi->refe2);
					if (gtk_text_iter_in_range(&it2, arg1, arg2)
						&& gtk_text_iter_in_range(&it3, arg1, arg2)) {
						if (!gtk_text_iter_has_tag(&it2, view->internal_tags[IT_BLOCK])) {
							gtk_text_buffer_get_iter_at_mark(textbuffer, &it2, mark2);
							gtk_text_buffer_get_iter_at_mark(textbuffer, &it3, mark3);
							gtk_text_buffer_remove_tag(textbuffer, view->internal_tags[IT_BLOCK],
													   &it2, &it3);
						}
						if (bi->tagname)
							g_free(bi->tagname);
						if (bi2 && bi2->tagname)
							g_free(bi2->tagname);
						if (bi3 && bi3->tagname)
							g_free(bi3->tagname);
						if (bi4 && bi4->tagname)
							g_free(bi4->tagname);
						gtk_text_buffer_delete_mark(textbuffer, mark);
						gtk_text_buffer_delete_mark(textbuffer, bi->ref);
						gtk_text_buffer_delete_mark(textbuffer, bi->refe1);
						gtk_text_buffer_delete_mark(textbuffer, bi->refe2);
						g_free(bi);
						if (bi2)
							g_free(bi2);
						if (bi3)
							g_free(bi3);
						if (bi4)
							g_free(bi4);
					}
				}
				deleted = TRUE;
			}					/* folded */
		}
		gtk_text_iter_forward_char(&it);
		if (gtk_text_iter_is_end(&it))
			break;
	}
}

static void bftv_fold(BfTextView * self, GtkTextMark * mark, gboolean move_cursor)
{
	GtkTextBuffer *buf = gtk_text_mark_get_buffer(mark);
	BlockInfo *bi = (BlockInfo *) g_object_get_data(G_OBJECT(mark), "bi"), *bi2;
	gboolean f = FALSE;
	GtkTextIter it1, it2, it3, it4, it5;

	if (!bi || bi->single_line || bi->type != BI_START)
		return;
	if (bi->folded) {
		bi->folded = FALSE;
		bi2 = (BlockInfo *) g_object_get_data(G_OBJECT(bi->refe1), "bi");
		if (bi2)
			bi2->folded = FALSE;
		f = FALSE;
	} else {
		bi->folded = TRUE;
		bi2 = (BlockInfo *) g_object_get_data(G_OBJECT(bi->refe1), "bi");
		if (bi2)
			bi2->folded = TRUE;

		f = TRUE;
	}
	gtk_text_buffer_get_iter_at_mark(buf, &it1, mark);
	gtk_text_iter_set_line(&it1, gtk_text_iter_get_line(&it1));
	gtk_text_buffer_get_iter_at_mark(buf, &it2, GTK_TEXT_MARK(bi->ref));
	gtk_text_buffer_get_iter_at_mark(buf, &it3, GTK_TEXT_MARK(bi->refe1));
	gtk_text_iter_set_line_offset(&it3, 0);
	gtk_text_buffer_get_iter_at_mark(buf, &it4, GTK_TEXT_MARK(bi->refe2));
	if (f) {
		if (move_cursor) {
			it5 = it4;
			gtk_text_iter_forward_char(&it5);
			gtk_text_buffer_place_cursor(buf, &it5);
		}
		gtk_text_buffer_apply_tag_by_name(buf, "_folded_", &it2, &it3);
		gtk_text_buffer_apply_tag_by_name(buf, "_fold_header_", &it1, &it2);
		gtk_text_buffer_apply_tag_by_name(buf, "_fold_header_", &it3, &it4);
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
		}						/* while */
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
		}						/* while */
	}
	return mark;
}

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


/* ---------------------    /UTILITY FUNCTIONS  ---------------------------- */

/* ---------------------    LANGUAGE CONFIG ---------------------------- */
static gboolean bftv_xml_bool(xmlChar * text)
{
	if (text == NULL)
		return FALSE;
	if (xmlStrcmp(text, (const xmlChar *) "true") == 0
		|| xmlStrcmp(text, (const xmlChar *) "TRUE") == 0
		|| xmlStrcmp(text, (const xmlChar *) "1") == 0) {
		return TRUE;
	}
	return FALSE;
}


static void bftv_scantable_insert(BfState * scantable, guint8 type, gpointer data,
								  BfLangConfig * cfg)
{
	BfLangToken *token = NULL;
	BfLangBlock *block = NULL;
	BfState *current_state = scantable;
	guchar *ptr = NULL;
	gunichar *ptr2, *input, *inp3;
	gint i, j, k, m;
	glong size;
	gint counter = 0;
	gboolean regexp = TRUE, reverse_set, found;
	GList *pstates = NULL, *states = NULL, *auxlst;
	gboolean charset[BFTV_SCAN_RANGE], done = FALSE;
	BfState *z;


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


	if (!ptr || !xmlCheckUTF8(ptr))
		return;
	if (cfg->case_sensitive || (type == ST_TOKEN && token && token->type == TT_FAKE)) {
		ptr2 = g_utf8_to_ucs4_fast(ptr, -1, &size);
	} else {
		gchar *tmp = g_utf8_strup(ptr, -1);
		ptr2 = g_utf8_to_ucs4_fast(tmp, -1, &size);
		g_free(tmp);
	}
	if (!regexp) {
		if (xmlUTF8Strlen(ptr) > cfg->max_token_length)
			cfg->max_token_length = xmlUTF8Strlen(ptr);
	}


	pstates = g_list_append(pstates, scantable);
	input = ptr2;
	i = 0;

	while (i < size) {			/* main loop */
		if (!regexp) {
			BfState *st = (BfState *) current_state->tv[(gint) * input];
			if (st != NULL && st->type == ST_TRANSIT) {
				current_state = current_state->tv[(gint) * input];
			} else if (st != NULL) {
				found = FALSE;
				for (m = 0; m < BFTV_SCAN_RANGE; m++) {
					if (m != (gint) * input && st->tv[m] == st->tv[(gint) * input])
						found = TRUE;
				}
				current_state->tv[(gint) * input] = g_new0(BfState, 1);
				current_state = current_state->tv[(gint) * input];
				current_state->type = ST_TRANSIT;
				cfg->num_states++;
				if (!found)
					g_free(st);
			} else {
				current_state->tv[(gint) * input] = g_new0(BfState, 1);
				current_state = current_state->tv[(gint) * input];
				current_state->type = ST_TRANSIT;
				cfg->num_states++;
			}
		} else {				/* REGULAR */

			/* DETERMINE SPECIFIED CHARACTER SET */
			reverse_set = FALSE;
			for (m = 0; m < BFTV_SCAN_RANGE; m++)
				charset[m] = FALSE;
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
				charset[(gint) * input] = TRUE;
			}
			/* CREATE STATES */
			g_list_free(states);
			states = NULL;
			done = FALSE;
			z = g_new0(BfState, 1);
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
									g_free((BfState *) st->tv[m]);
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
				auxlst = g_list_first(states);
				BfState *final = g_new0(BfState, 1);
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

		}						/* REGULAR END */
		if (regexp)
			counter++;
		if (i < size)
			input++;
		i++;
	}							/* main loop */

	if (!regexp) {
		BfState *s = g_new0(BfState, 1);
		s->type = type;
		s->data = data;
		cfg->num_states++;
		for (j = 0; j < BFTV_SCAN_RANGE; j++)
			if (!current_state->tv[j])
				current_state->tv[j] = s;
	} else {
		if (counter > cfg->max_token_length)
			cfg->max_token_length = counter;
	}


}								/* bftv_scantable_insert */


static gpointer bftv_make_entity(xmlDocPtr doc, xmlNodePtr node, BfLangConfig * cfg, gint type,
								 guchar * group, const guchar * groupstyle, guchar * text)
{
	xmlChar *tmps, *tmps2;
	gpointer ptr;
	
	if ( text!=NULL && strcmp(text,"")==0 ) return NULL;
	switch (type) {
	case ST_TOKEN:
		if (text == NULL)
			tmps = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
		else
			tmps = text;
		BfLangToken *t = g_new0(BfLangToken, 1);
		t->type = TT_NORMAL;
		t->group = group;
		tmps2 = xmlGetProp(node, (const xmlChar *) "regexp");
		t->regexp = bftv_xml_bool(tmps2);
		if (tmps2)
			xmlFree(tmps2);
		tmps2 = xmlGetProp(node, (const xmlChar *) "name");
		if (tmps2 && text == NULL) {
			t->name = tmps2;
		} else {
			t->name = tmps;
			if (tmps2)
				xmlFree(tmps2);
		}
		t->text = tmps;
		tmps2 = xmlGetProp(node, (const xmlChar *) "context");
		if (tmps2) {
			ptr = g_hash_table_lookup(cfg->blocks, tmps2);
			if (!ptr)
				g_warning("Token (%s) context defined as %s but such a block does not exists.",
						  t->text, tmps2);
			t->context = (BfLangBlock *) ptr;
			xmlFree(tmps2);
		} else
			t->context = NULL;

		tmps2 = xmlGetProp(node, (const xmlChar *) "defaultstyle");
		t->tag =
			get_tag_for_scanner_style((gchar *) cfg->name, "t", (gchar *) t->name, (gchar *) tmps2);
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
		break;
	case ST_BLOCK_BEGIN:
		tmps = xmlGetProp(node, (const xmlChar *) "id");
		if (tmps) {
			BfLangBlock *b = g_new0(BfLangBlock, 1);
			b->type = BT_NORMAL;
			b->name = tmps;
			b->group = group;
			b->begin = xmlGetProp(node, (const xmlChar *) "begin");
			b->end = xmlGetProp(node, (const xmlChar *) "end");
			tmps2 = xmlGetProp(node, (const xmlChar *) "scanned");
			b->scanned = bftv_xml_bool(tmps2);
			if (tmps2)
				xmlFree(tmps2);
			tmps2 = xmlGetProp(node, (const xmlChar *) "markup");
			b->markup = bftv_xml_bool(tmps2);
			if (tmps2)
				xmlFree(tmps2);
			tmps2 = xmlGetProp(node, (const xmlChar *) "foldable");
			b->foldable = bftv_xml_bool(tmps2);
			if (tmps2)
				xmlFree(tmps2);
			tmps2 = xmlGetProp(node, (const xmlChar *) "case");
			b->cs = bftv_xml_bool(tmps2);
			if (tmps2)
				xmlFree(tmps2);
			tmps2 = xmlGetProp(node, (const xmlChar *) "regexp");
			b->regexp = bftv_xml_bool(tmps2);
			if (tmps2)
				xmlFree(tmps2);
			tmps2 = xmlGetProp(node, (const xmlChar *) "context");
			if (tmps2) {
				ptr = g_hash_table_lookup(cfg->blocks, tmps2);
				if (!ptr)
					g_warning("Block (%s) context defined as %s but such a block does not exists.",
							  b->name, tmps2);
				b->context = ptr;
				xmlFree(tmps2);
			} else
				b->context = NULL;
			tmps2 = xmlGetProp(node, (const xmlChar *) "defaultstyle");
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

static BfLangConfig *bftv_load_config(gchar * filename, const gchar * filetype_name)
{
	xmlDocPtr doc;
	xmlNodePtr cur, cur2;
	xmlChar *tmps, *tmps2 = NULL, *tmps3 = NULL, *tmps4;
	BfLangConfig *cfg = NULL;
	GList *lst, *lst2, *lst3;
	gint i;

	g_return_val_if_fail(filename != NULL, (BfLangConfig *) 0);
	xmlLineNumbersDefault(1);
	doc = xmlReadFile(filename, "UTF-8", XML_PARSE_RECOVER | XML_PARSE_NOENT);
	cur = xmlDocGetRootElement(doc);
	if (xmlStrcmp(cur->name, (const xmlChar *) "bflang") == 0) {
		cfg = g_new0(BfLangConfig, 1);
		cfg->name = xmlGetProp(cur, (const xmlChar *) "name");
		cfg->description = xmlGetProp(cur, (const xmlChar *) "description");
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
			cfg->as_triggers[i] = 0;
			cfg->escapes[i] = 0;
		}
		cur = cur->xmlChildrenNode;
		while (cur != NULL) {
			if (xmlStrcmp(cur->name, (const xmlChar *) "options") == 0) {
				cur2 = cur->xmlChildrenNode;
				while (cur2 != NULL) {
					if (xmlStrcmp(cur2->name, (const xmlChar *) "option") == 0) {
						tmps = xmlGetProp(cur2, (const xmlChar *) "name");
						tmps2 = xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
						if (tmps) {
							if (xmlStrcmp(tmps, (const xmlChar *) "case-sensitive") == 0)
								cfg->case_sensitive = bftv_xml_bool(tmps2);
							else if (xmlStrcmp(tmps, (const xmlChar *) "scan-markup-tags") == 0)
								cfg->scan_tags = bftv_xml_bool(tmps2);								
							else if (xmlStrcmp(tmps, (const xmlChar *) "scan-blocks") == 0)
								cfg->scan_blocks = bftv_xml_bool(tmps2);
							else if (xmlStrcmp(tmps, (const xmlChar *) "schema-aware") == 0)
								cfg->schema_aware = bftv_xml_bool(tmps2);								
							else if (xmlStrcmp(tmps, (const xmlChar *) "autoclose-exclude") == 0) {
								gchar **arr = NULL;
								gint i = 0;
								arr = g_strsplit(tmps2, ",", -1);
								while (arr[i]) {
									g_hash_table_insert(cfg->dont_autoclose, g_strdup(arr[i]),
														&const_true);
									i++;
								}
								g_strfreev(arr);
							} else if (xmlStrcmp(tmps, (const xmlChar *) "auto-scan-triggers") == 0) {
								const guchar *p = tmps2;
								i = 0;
								while (i < xmlUTF8Strlen(tmps2)) {
									cfg->as_triggers[(gint) * p] = 1;
									p = xmlUTF8Strpos(tmps2, i++);
								}
							} else if (xmlStrcmp(tmps, (const xmlChar *) "escape-characters") == 0) {
								const guchar *p = tmps2;
								i = 0;
								while (i < xmlUTF8Strlen(tmps2)) {
									cfg->escapes[(gint) * p] = 1;
									p = xmlUTF8Strpos(tmps2, i++);
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
				tmps3 = xmlGetProp(cur, (const xmlChar *) "id");
				tmps4 = xmlGetProp(cur, (const xmlChar *) "defaultstyle");
				g_hash_table_insert(cfg->groups, xmlStrdup(tmps3), "b");
				cur2 = cur->xmlChildrenNode;
				while (cur2 != NULL) {
					if (xmlStrcmp(cur2->name, (const xmlChar *) "block") == 0)
						bftv_make_entity(doc, cur2, cfg, ST_BLOCK_BEGIN, tmps3, tmps4, NULL);
					cur2 = cur2->next;
				}				/* while */
				xmlFree(tmps4);
			} else if (xmlStrcmp(cur->name, (const xmlChar *) "token-group") == 0) {	/* tokens  */
				tmps3 = xmlGetProp(cur, (const xmlChar *) "id");
				tmps4 = xmlGetProp(cur, (const xmlChar *) "defaultstyle");
				g_hash_table_insert(cfg->groups, xmlStrdup(tmps3), "t");
				cur2 = cur->xmlChildrenNode;
				while (cur2 != NULL) {
					if (xmlStrcmp(cur2->name, (const xmlChar *) "token") == 0) {
						bftv_make_entity(doc, cur2, cfg, ST_TOKEN, tmps3, tmps4, NULL);
					} else if (xmlStrcmp(cur2->name, (const xmlChar *) "token-list") == 0) {
						tmps2 = xmlGetProp(cur2, (const xmlChar *) "separator");
						if (tmps2) {
							tmps = xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
							gchar **arr = g_strsplit(tmps, tmps2, -1);
							xmlFree(tmps2);
							if (arr) {
								gint i = 0;
								while (arr[i] != NULL) {
									bftv_make_entity(doc, cur2, cfg, ST_TOKEN, tmps3, tmps4,
													 g_strdup(arr[i]));
									i++;
								}	/* while */
								g_strfreev(arr);
							}
							if (tmps)
								xmlFree(tmps);
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
				tmps2 = xmlGetProp(cur, (const xmlChar *) "separator");
				if (tmps2) {
					tmps = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
					gchar **arr = g_strsplit(tmps, tmps2, -1);
					xmlFree(tmps2);
					if (arr) {
						gint i = 0;
						while (arr[i] != NULL) {
							bftv_make_entity(doc, cur, cfg, ST_TOKEN, NULL, NULL, g_strdup(arr[i]));
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
				lst3 = g_list_append(lst3, g_strstrip(t->text));
			}
			lst = g_list_next(lst);
		}
		ac_add_lang_list(main_v->autocompletion, cfg->name, lst3);


		if (cfg->scan_tags) {
			BfLangToken *t = g_new0(BfLangToken, 1);
			t->group = NULL;
			t->regexp = TRUE;
			t->name = xmlCharStrdup("_tag_end_");
			t->text = xmlCharStrdup("</[a-zA-Z:_][a-zA-Z0-9:_-.]*[ ]*>");
			t->context = NULL;
			t->type = TT_TAG_END;
			bftv_scantable_insert(&cfg->scan_table, ST_TOKEN, t, cfg);
			g_hash_table_insert(cfg->tokens, &t->name, t);

			BfLangBlock *b = g_new0(BfLangBlock, 1);
			b->name = xmlCharStrdup("_tag_begin_");
			b->group = NULL;
			b->regexp = TRUE;
			b->begin = xmlCharStrdup("<[a-zA-Z:_][a-zA-Z0-9:_-.]*[ ]*");
			b->end = xmlCharStrdup("/?>");
			b->scanned = TRUE;
			b->foldable = FALSE;
			b->cs = FALSE;
			b->type = BT_TAG_BEGIN;
			b->markup = FALSE;
			g_hash_table_insert(cfg->blocks, b->name, b);
			bftv_scantable_insert(&cfg->scan_table, ST_BLOCK_BEGIN, b, cfg);
			bftv_scantable_insert(&b->scan_table, ST_BLOCK_END, b, cfg);

		

			t = g_new0(BfLangToken, 1);
			t->group = NULL;
			t->regexp = TRUE;
			t->name = xmlCharStrdup("_attr2_");
			t->type = TT_ATTR2;
			t->text = xmlCharStrdup("[a-zA-Z-:]+=\"[^\"]*\"");
			t->context = b;
			bftv_scantable_insert(&b->scan_table, ST_TOKEN, t, cfg);
			g_hash_table_insert(cfg->tokens, &t->name, t);

			t = g_new0(BfLangToken, 1);
			t->group = NULL;
			t->regexp = TRUE;
			t->name = xmlCharStrdup("_attr_");
			t->type = TT_ATTR;
			t->text = xmlCharStrdup("[a-zA-Z-:]+=[^\" ><]+");
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

		if ( cfg->schema_aware )
		{
		
			BfLangBlock *b = g_new0(BfLangBlock, 1);
			b->name = xmlCharStrdup("_doctype_internal_");
			b->group = NULL;
			b->regexp = TRUE;
			b->begin = xmlCharStrdup("<!DOCTYPE[ a-zA-Z]+[\\[]");
			b->end = xmlCharStrdup("[\\]]>");
			b->scanned = FALSE;
			b->foldable = TRUE;
			b->cs = FALSE;
			b->type = BT_DOCTYPE_INT;
			b->markup = FALSE;
			g_hash_table_insert(cfg->blocks, b->name, b);
			bftv_scantable_insert(&cfg->scan_table, ST_BLOCK_BEGIN, b, cfg);
			bftv_scantable_insert(&b->scan_table, ST_BLOCK_END, b, cfg);
					
			BfLangToken *t = g_new0(BfLangToken, 1);
			t->group = NULL;
			t->regexp = TRUE;
			t->name = xmlCharStrdup("_doctype_");
			t->text = xmlCharStrdup("<!DOCTYPE[^>]*>");
			t->context = NULL;
			t->type = TT_DOCTYPE;
			bftv_scantable_insert(&cfg->scan_table, ST_TOKEN, t, cfg);
			g_hash_table_insert(cfg->tokens, &t->name, t);
			
					
		}		

		{						/* FAKE IDENTIFIER - Lookahead symbol workaround  */
			gunichar c;
			gchar *pstr, *tofree, *pstr2;
			gchar out[6];

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
			BfLangToken *t = g_new0(BfLangToken, 1);
			t->group = NULL;
			t->regexp = TRUE;
			t->name = xmlCharStrdup("_fake_ident_");
			t->text = pstr;
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
	g_print("NUMBER OF STATES: %d (%s) - table size = %d, max token len: %d\n", cfg->num_states,
			cfg->name, cfg->num_states * sizeof(BfState), cfg->max_token_length);
#endif
	return cfg;
}


/* ---------------------     /LANGUAGE CONFIG ---------------------------- */



/* --------------------   LANGUAGE MANAGER -------------------------------*/
BfLangManager *bf_lang_mgr_new()
{
	BfLangManager *ret = g_new0(BfLangManager, 1);
	ret->languages = g_hash_table_new(g_str_hash, g_str_equal);
	return ret;
}

BfLangConfig *bf_lang_mgr_load_config(BfLangManager * mgr, const gchar * filename)
{
	gchar *fname, *fname1, *fname2;
	BfLangConfig *cfg = NULL;

	fname1 = g_strconcat(PKGDATADIR, filename, NULL);
	fname2 = g_strconcat(g_get_home_dir(), "/." PACKAGE "/", filename, NULL);
	fname = return_first_existing_filename(fname1, fname2, NULL);
	if (fname) {
		cfg = bftv_load_config(fname, filename);
		if (cfg != NULL) {
			DEBUG_MSG("bf_lang_mgr_load_config, adding %s to hashtable\n", filename);
			/* hmm can we add them by mime-type instead of filename ? */
			g_hash_table_replace(mgr->languages, (gpointer) filename, cfg);
			/* 
			now this is the place where we add the config to the mime-type hashtable... TODO
			*/
#ifdef GNOMEVFSINT
			g_prtint("bf_lang_mgr_load_config, TODO, add to mime-type table\n");
#endif
		}
		g_free(fname);
	}
	g_free(fname1);
	g_free(fname2);
	return (cfg);
}

/* I would like a function based on mime types.. */
BfLangConfig *bf_lang_mgr_get_config(BfLangManager * mgr, const gchar * filename)
{
	return g_hash_table_lookup(mgr->languages, filename);
}

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
	guchar *grpcrit;
} Thf;

static void bftv_ins_block(gpointer key, gpointer value, gpointer udata)
{
	Thf *d = (Thf *) udata;
	BfLangBlock *t = (BfLangBlock *) value;
	if (d->grpcrit == NULL && t->group == NULL) {
		if (xmlStrcmp(t->name, (const xmlChar *) "_tag_begin_") != 0)
			*(d->list) = g_list_append(*(d->list), t->name);
	} else if (d->grpcrit != NULL && t->group != NULL && xmlStrcmp(t->group, d->grpcrit) == 0) {
		if (xmlStrcmp(t->name, (const xmlChar *) "_tag_begin_") != 0)
			*(d->list) = g_list_append(*(d->list), t->name);
	}
}

/* If you want to get list of "unaligned" blocks 
*  i.e. blocks which are defined without a group - 
*  just use NULL for group param 
*/
GList *bf_lang_get_blocks_for_group(BfLangConfig * cfg, guchar * group)
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
		if (xmlStrcmp(t->name, (const xmlChar *) "_tag_end_") != 0
			&& xmlStrcmp(t->name, (const xmlChar *) "_attr_") != 0
			&& xmlStrcmp(t->name, (const xmlChar *) "_attr2_") != 0
			&& xmlStrcmp(t->name, (const xmlChar *) "_attr_tag_begin_end_") != 0
			&& xmlStrcmp(t->name, (const xmlChar *) "_fake_ident_") != 0)
			*(d->list) = g_list_append(*(d->list), t->name);
	} else if (d->grpcrit != NULL && t->group != NULL && xmlStrcmp(t->group, d->grpcrit) == 0) {
		if (xmlStrcmp(t->name, (const xmlChar *) "_tag_end_") != 0
			&& xmlStrcmp(t->name, (const xmlChar *) "_attr_") != 0
			&& xmlStrcmp(t->name, (const xmlChar *) "_attr2_") != 0
			&& xmlStrcmp(t->name, (const xmlChar *) "_attr_tag_begin_end_") != 0
			&& xmlStrcmp(t->name, (const xmlChar *) "_fake_ident_") != 0)
			*(d->list) = g_list_append(*(d->list), t->name);
	}
}

/* If you want to get list of "unaligned" tokens 
*  i.e. tokens which are defined without a group - 
*  just use NULL for group param 
*/
GList *bf_lang_get_tokens_for_group(BfLangConfig * cfg, guchar * group)
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

static void bf_lang_retag(gpointer key, gpointer value, gpointer udata)
{
	BfLangConfig *cfg = (BfLangConfig *) value;
	g_hash_table_foreach(cfg->tokens, bflang_retag_token, cfg);
	g_hash_table_foreach(cfg->blocks, bflang_retag_block, cfg);
	cfg->tag_begin = get_tag_for_scanner_style((gchar *) cfg->name, "m", "tag_begin", NULL);
	cfg->tag_end = get_tag_for_scanner_style((gchar *) cfg->name, "m", "tag_end", NULL);
	cfg->attr_name = get_tag_for_scanner_style((gchar *) cfg->name, "m", "attr_name", NULL);
	cfg->attr_val = get_tag_for_scanner_style((gchar *) cfg->name, "m", "attr_val", NULL);

	/* we should perhaps also retag some of the internal tags such as _block_match_ ? */
}

void bf_lang_mgr_retag(void)
{
	g_hash_table_foreach(main_v->lang_mgr->languages, bf_lang_retag, NULL);
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
	BlockInfo *bi = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(BF_IS_TEXTVIEW(self));
	it = *start;
	for (i = gtk_text_iter_get_line(start); i <= gtk_text_iter_get_line(end); i++) {
		gtk_text_iter_set_line(&it, i);
		mark = bftv_get_first_block_at_line(self, &it, TRUE);
		if (mark) {
			bi = (BlockInfo *) g_object_get_data(G_OBJECT(mark), "bi");
			if (bi && bi->type == BI_START) {
				if (fold)
					bi->folded = FALSE;
				else
					bi->folded = TRUE;
				bftv_fold(self, mark, FALSE);
			}
		}
	}
}

/* -------------------- /FOLDING  -------------------------------*/

/* -------------------- SCANNING  -------------------------------*/


void bf_textview_scan(BfTextView * self)
{
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
	bf_textview_scan_area(self, &its, &ite, TRUE);
}

void bf_textview_scan_visible(BfTextView * self)
{
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
	bf_textview_scan_area(self, &its, &ite, TRUE);
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
}

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
	BlockInfo *bi = NULL;
	GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self));
	DEBUG_MSG("bftv_clear_matched_block, started\n");
	if (self->last_matched_block) {
		GtkTextIter it, it2;
		bi = (BlockInfo *) g_object_get_data(G_OBJECT(self->last_matched_block), "bi");
		gtk_text_buffer_get_iter_at_mark(buf, &it, self->last_matched_block);
		gtk_text_buffer_get_iter_at_mark(buf, &it2, bi->ref);
		gtk_text_buffer_remove_tag(buf, self->internal_tags[IT_BLOCK_MATCH], &it, &it2);
		if (bi->type == BI_START) {
			gtk_text_buffer_get_iter_at_mark(buf, &it, bi->refe1);
			gtk_text_buffer_get_iter_at_mark(buf, &it2, bi->refe2);
		} else if (bi->type == BI_END) {
			gtk_text_buffer_get_iter_at_mark(buf, &it, bi->refb1);
			gtk_text_buffer_get_iter_at_mark(buf, &it2, bi->refb2);
		}
		gtk_text_buffer_remove_tag(buf, self->internal_tags[IT_BLOCK_MATCH], &it, &it2);
	}
	self->last_matched_block = NULL;
}

void bf_textview_scan_area(BfTextView * self, GtkTextIter * start, GtkTextIter * end,
						   gboolean apply_hl)
{
	GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self));
	GtkTextIter its, ita, pit;
	gunichar c;
	gboolean block_found = FALSE, token_found = FALSE, recognizing = FALSE;
	GtkTextMark *mark, *mark2;
	gshort magic = 0;
	TBfBlock *bf = NULL;
	GtkTextTag *tag = NULL;
	BfState *current_state;
	Trts rts;
	BlockInfo *bi = NULL;

	DEBUG_MSG("bf_textview_scan_area, started\n");
	g_return_if_fail(self != NULL);
	g_return_if_fail(BF_IS_TEXTVIEW(self));
	if (!self->lang || self->paste_operation)
		return;
	its = *start;
	ita = *start;
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
	bftv_clear_block_cache(self);
	bftv_clear_matched_block(self);
	if (apply_hl) {
		bftv_delete_blocks_from_area(self, start, end);
		if (self->lang->tag_begin)
			gtk_text_buffer_remove_tag(buf, self->lang->tag_begin, start, end);
		if (self->lang->tag_end)
			gtk_text_buffer_remove_tag(buf, self->lang->tag_end, start, end);
		if (self->lang->attr_name)
			gtk_text_buffer_remove_tag(buf, self->lang->attr_name, start, end);
		if (self->lang->attr_val)
			gtk_text_buffer_remove_tag(buf, self->lang->attr_val, start, end);
		rts.buffer = buf;
		rts.start = start;
		rts.end = end;
		g_hash_table_foreach(self->lang->tokens, bftv_remove_t_tag, &rts);
		g_hash_table_foreach(self->lang->blocks, bftv_remove_b_tag, &rts);
	}

	magic = 0;
	while (gtk_text_iter_compare(&ita, end) <= 0) {	/* main loop */
		DEBUG_MSG("bf_textview_scan_area, in main loop, ita at %d, end at %d\n", gtk_text_iter_get_offset(&ita),gtk_text_iter_get_offset(end));
		if (gtk_text_iter_equal(&ita, end)) {
			magic++;
			if (magic >= 2)
				break;
		}
		c = gtk_text_iter_get_char(&ita);
		if (!g_unichar_isgraph(c) ) {
			self->scanner.last_string = g_string_assign(self->scanner.last_string, "");
		} else if (!block_found) {
			g_string_append_printf(self->scanner.last_string, "%c", c);
		}

		/* This is a trick. Character of code 3(ETX) is not printable, so will not appear in the text,
		   but automata has entries for it, so if at the end of text is token - it will 
		   be recognized */
		if (magic == 1)
			c = 3;
		if ((gint) c < 0 || (gint) c > BFTV_SCAN_RANGE) {
			c = 0;
			recognizing = FALSE;
		}
		while (gtk_text_iter_compare(&ita, end) <= 0 && self->lang->escapes[(gint) c]) {	/* remove escapes */
			gtk_text_iter_forward_char(&ita);
			gtk_text_iter_forward_char(&ita);
			if (magic == 0)
				c = gtk_text_iter_get_char(&ita);
		}						/* remove escapes */

/* #####################     RECOGNIZE TOKENS AND BLOCKS ###################*/
		token_found = FALSE;
		block_found = FALSE;
		if (!current_state) {
			if (self->scanner.current_context) {
				current_state = &self->scanner.current_context->scan_table;
			} else
				current_state = &self->lang->scan_table;
		}
		if (self->lang->case_sensitive)
			current_state = current_state->tv[(gint) c];
		else
			current_state = current_state->tv[(gint) g_unichar_toupper(c)];

		if (current_state) {
			recognizing = TRUE;
			switch (current_state->type) {
			case ST_TOKEN:
				{
					BfLangToken *t = (BfLangToken *) current_state->data;
					switch (t->type) {
					case TT_NORMAL:
						if (self->highlight && t->tag && apply_hl)
							gtk_text_buffer_apply_tag(buf, t->tag, &its, &ita);
						break;
					case TT_TAG_END:
						{
							gchar *txt = gtk_text_buffer_get_text(buf, &its, &ita, FALSE);
							gchar *pc = txt + 2;
							gchar **arr = g_strsplit(pc, ">", -1);
							gchar **arr2 = g_strsplit(arr[0], " ", -1);
							if (!g_queue_is_empty(&(self->scanner.tag_stack))) {
								/* Have to get tags from stack, because HTML allows not closed tags */
								bf = NULL;
								while (!g_queue_is_empty(&(self->scanner.tag_stack))) {
									bf = g_queue_pop_head(&(self->scanner.tag_stack));
									if (bf && strcmp(bf->tagname, arr2[0]) == 0)
										break;
									g_free(bf->tagname);
									g_free(bf);
									bf = NULL;
								}
								if (bf) {
									mark = mark2 = NULL;
									GtkTextMark *mark3, *mark4;
									gboolean do_mark = TRUE;

									mark3 = bftv_get_block_at_iter(&bf->b_start);

									if (mark3) {
										bi = (BlockInfo *) g_object_get_data(G_OBJECT(mark3), "bi");
										if (bi->type == BI_START
											&& strcmp(bi->tagname, arr2[0]) == 0)
											do_mark = FALSE;
									}
									if (do_mark) {
										mark =
											gtk_text_buffer_create_mark(buf, NULL, &bf->b_start,
																		FALSE);
										mark2 =
											gtk_text_buffer_create_mark(buf, NULL, &bf->b_end,
																		FALSE);
										mark3 = gtk_text_buffer_create_mark(buf, NULL, &its, FALSE);
										mark4 = gtk_text_buffer_create_mark(buf, NULL, &ita, FALSE);
										bi = g_new0(BlockInfo, 1);
										bi->type = BI_START;
										bi->folded = FALSE;
										bi->tagname = g_strdup(arr2[0]);
										bi->ref = mark2;
										bi->refe1 = mark3;
										bi->refe2 = mark4;
										g_object_set_data(G_OBJECT(mark), "bi", bi);
										BlockInfo *bi2 = g_new0(BlockInfo, 1);
										bi2->type = BI_END;
										bi2->folded = FALSE;
										bi2->tagname = g_strdup(arr2[0]);
										bi2->ref = mark4;
										bi2->refb1 = mark;
										bi2->refb2 = mark2;
										g_object_set_data(G_OBJECT(mark3), "bi", bi2);
										if (gtk_text_iter_get_line(&bf->b_start) ==
											gtk_text_iter_get_line(&its)) {
											bi->single_line = bi2->single_line = TRUE;
										} else {
											bi->single_line = bi2->single_line = FALSE;
										}
									}
									if (apply_hl)
										gtk_text_buffer_apply_tag(buf,
																  self->internal_tags[IT_BLOCK],
																  &bf->b_end, &its);
									g_free(bf->tagname);
									g_free(bf);
								}
							}	/* queue empty */
							g_strfreev(arr);
							g_strfreev(arr2);
							g_free(txt);
							if (self->highlight) {
								tag = self->lang->tag_end;
								if (tag && apply_hl)
									gtk_text_buffer_apply_tag(buf, tag, &its, &ita);
							}
						}		/* end of tag_end */
						break;
					case TT_ATTR2:
					case TT_ATTR:
						{
							gchar *txt = gtk_text_buffer_get_text(buf, &its, &ita, FALSE);
							gchar **arr = g_strsplit(txt, "=", -1);
							gchar *attrname=NULL,*attrval=NULL;
							pit = its;
							gtk_text_iter_forward_chars(&pit, g_utf8_strlen(arr[0], -1));
							g_strfreev(arr);
							g_free(txt);
							if (self->lang && self->lang->schema_aware ) /* XMLSchema */								
							{							
								attrname = gtk_text_buffer_get_text(buf, &its, &pit, FALSE);
								attrval = gtk_text_buffer_get_text(buf, &pit, &ita, FALSE);
								if ( g_str_has_suffix(attrname,"schemaLocation") ) 
								{
									gchar **arr = g_strsplit(attrval, " ", -1);
									gchar *sname = NULL;
									if ( arr[1] )
									{									
										arr[1][g_utf8_strlen(arr[1],-1)-1]='\0';															
										sname = ac_add_xmlschema_list(main_v->autocompletion,arr[1]);
										if ( sname )
											self->schemas = g_list_append(self->schemas, sname );
									}										
									g_strfreev(arr);
								}	
								g_free(attrname);
								g_free(attrval);
							}
							if (self->highlight) {
								tag = self->lang->attr_name;
								if (tag && apply_hl)
									gtk_text_buffer_apply_tag(buf, tag, &its, &pit);
								tag = self->lang->attr_val;
								if (tag && apply_hl)
									gtk_text_buffer_apply_tag(buf, tag, &pit, &ita);
							}
						}
						break;
					case TT_DOCTYPE:
						{
							gchar *txt = gtk_text_buffer_get_text(buf, &its, &ita, FALSE);
							gchar *sname = ac_add_dtd_list(main_v->autocompletion,txt,FALSE,NULL);
							if ( sname )
								self->schemas = g_list_append(self->schemas, sname );
							g_free(txt);					
						}	
						break;
					case TT_FAKE:
						break;
					}
					token_found = TRUE;
					current_state = NULL;
					its = ita;
				}
				break;			/* token */
			case ST_BLOCK_BEGIN:
				{
					BfLangBlock *tmp = (BfLangBlock *) current_state->data;
					bf = g_new0(TBfBlock, 1);
					bf->def = tmp;
					bf->b_start = its;
					bf->b_end = ita;
					g_queue_push_head(&(self->scanner.block_stack), bf);
					self->scanner.current_context = tmp;
					if (tmp->type == BT_TAG_BEGIN) {
						pit = its;
						gtk_text_iter_forward_char(&pit);
						bf->tagname = gtk_text_buffer_get_text(buf, &pit, &ita, FALSE);
						bf->tagname = g_strstrip(bf->tagname);
						self->scanner.last_tagname = g_strdup(bf->tagname);
					} else
					{
						if ( self->scanner.last_tagname )
							g_free(self->scanner.last_tagname);
						self->scanner.last_tagname=NULL;
						bf->tagname = NULL;
					}	
					current_state = &tmp->scan_table;
					block_found = TRUE;
					its = ita;
				}
				break;
			case ST_BLOCK_END:
				{
					BfLangBlock *tmp = (BfLangBlock *) current_state->data;
					bf = g_queue_peek_head(&(self->scanner.block_stack));
					if (bf && bf->def == tmp) {
						g_queue_pop_head(&(self->scanner.block_stack));
						TBfBlock *aux =
							(TBfBlock *) g_queue_peek_head(&(self->scanner.block_stack));
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
								gboolean just_ended = FALSE;
								if (self->scanner.last_tagname)
									g_free(self->scanner.last_tagname);
								self->scanner.last_tagname = NULL;
								TBfBlock *bf_2 = g_new0(TBfBlock, 1);
								bf_2->tagname = g_strdup(bf->tagname);
								bf_2->b_start = bf->b_start;
								bf_2->b_end = ita;
								g_queue_push_head(&(self->scanner.tag_stack), bf_2);
								pit = ita;
								while (gtk_text_iter_get_char(&pit) != '>')
									gtk_text_iter_backward_char(&pit);
								gtk_text_iter_backward_char(&pit);
								if (gtk_text_iter_get_char(&pit) == '/')
									just_ended = TRUE;
								if (self->highlight) {
									tag = self->lang->tag_begin;
									if (tag && apply_hl)
										gtk_text_buffer_apply_tag(buf, tag, &bf->b_start, &ita);
								}
								/* TAG autoclose */
								if (self->tag_autoclose && !self->delete_rescan
									&& !self->paste_operation && self->tag_ac_state && !just_ended
									&& g_hash_table_lookup(self->lang->dont_autoclose,
														   bf_2->tagname) == NULL) {
									GtkTextIter it9;
									gtk_text_buffer_get_iter_at_mark(buf, &it9,
																	 gtk_text_buffer_get_insert
																	 (buf));
									if (gtk_text_iter_equal(&it9, &ita)) {
										gchar *pp = g_strjoin("", "</", bf_2->tagname, ">", NULL);
										self->tag_ac_state = FALSE;
										/* Clear stacks */
										while (!g_queue_is_empty(&self->scanner.block_stack)) {
											bf = (TBfBlock *) g_queue_pop_head(&self->scanner.
																			   block_stack);
											if (bf->tagname)
												g_free(bf->tagname);
											g_free(bf);
										}
										while (!g_queue_is_empty(&self->scanner.tag_stack)) {
											bf = (TBfBlock *) g_queue_pop_head(&self->scanner.
																			   tag_stack);
											if (bf->tagname)
												g_free(bf->tagname);
											g_free(bf);
										}
										gtk_text_buffer_insert(buf, &ita, pp,
															   g_utf8_strlen(pp, -1));
										gtk_text_buffer_get_iter_at_mark(buf, &it9,
																		 gtk_text_buffer_get_insert
																		 (buf));
										gtk_text_iter_backward_chars(&it9, strlen(pp));
										gtk_text_buffer_place_cursor(buf, &it9);
										g_free(pp);
										g_signal_handler_unblock(buf, self->markset_signal_id);
										return;	/*I have to return from scan, because it has been performed after latest insert */
									}
								}
							}
							break;
						case BT_DOCTYPE_INT:
							{
								gchar *txt = gtk_text_buffer_get_text(buf, &bf->b_start, &ita, FALSE);
								ac_add_dtd_list(main_v->autocompletion,txt,TRUE,&self->internal_dtd);
								g_free(txt);					
							}	
							/* not breaking here - I want next code */	
						default:
							{
								mark = mark2 = NULL;
								GtkTextMark *mark3, *mark4;
								gboolean do_mark = TRUE;

								mark3 = bftv_get_block_at_iter(&bf->b_start);
								if (mark3) {
									bi = (BlockInfo *) g_object_get_data(G_OBJECT(mark3), "bi");
									if (bi->type == BI_START && bi->data == tmp)
										do_mark = FALSE;
								}
								if (do_mark) {
									mark =
										gtk_text_buffer_create_mark(buf, NULL, &bf->b_start, FALSE);
									mark2 =
										gtk_text_buffer_create_mark(buf, NULL, &bf->b_end, FALSE);
									mark3 = gtk_text_buffer_create_mark(buf, NULL, &its, FALSE);
									mark4 = gtk_text_buffer_create_mark(buf, NULL, &ita, FALSE);
									bi = g_new0(BlockInfo, 1);
									bi->type = BI_START;
									bi->folded = FALSE;
									bi->data = tmp;
									bi->ref = mark2;
									bi->refe1 = mark3;
									bi->refe2 = mark4;
									g_object_set_data(G_OBJECT(mark), "bi", bi);
									BlockInfo *bi2 = g_new0(BlockInfo, 1);
									bi2->type = BI_END;
									bi2->folded = FALSE;
									bi2->data = tmp;
									bi2->ref = mark4;
									bi2->refb1 = mark;
									bi2->refb2 = mark2;
									g_object_set_data(G_OBJECT(mark3), "bi", bi2);
									if (gtk_text_iter_get_line(&bf->b_start) ==
										gtk_text_iter_get_line(&its)) {
										bi->single_line = bi2->single_line = TRUE;
									} else {
										bi->single_line = bi2->single_line = FALSE;
									}
								}
								if (apply_hl)
									gtk_text_buffer_apply_tag(buf, self->internal_tags[IT_BLOCK],
															  &bf->b_end, &its);
								if (self->highlight) {
									if (tmp->tag && apply_hl)
										gtk_text_buffer_apply_tag(buf, tmp->tag, &bf->b_start,
																  &ita);
								}
							}	/* default */
						}		/* switch */
						if (bf->tagname)
							g_free(bf->tagname);
						g_free(bf);
					} /* if bf */
					else {
						current_state = NULL;
					}
					block_found = TRUE;
					its = ita;
				}
				break;
			case ST_TRANSIT:
				break;
			}
		} else {				/* current_state is NULL */

			its = ita;
			gtk_text_iter_forward_char(&its);
			if (recognizing) {
				gtk_text_iter_backward_char(&its);
				gtk_text_iter_backward_char(&ita);
				recognizing = FALSE;
			}
		}
		DEBUG_MSG("bf_textview_scan_area, token_found=%d, block_found=%d\n",token_found,block_found);
		if (!token_found && !block_found)
			gtk_text_iter_forward_char(&ita);

	}							/*main loop */

/* Clear stacks */
	while (!g_queue_is_empty(&self->scanner.block_stack)) {
		bf = (TBfBlock *) g_queue_pop_head(&self->scanner.block_stack);
		if (bf->tagname)
			g_free(bf->tagname);
		g_free(bf);
	}
	while (!g_queue_is_empty(&self->scanner.tag_stack)) {
		bf = (TBfBlock *) g_queue_pop_head(&self->scanner.tag_stack);
		if (bf->tagname)
			g_free(bf->tagname);
		g_free(bf);
	}

#ifdef HL_PROFILING
	{
		glong tot_ms = 0, marks = 0, allmarks = 0, tags = 0;
		GSList *ss = NULL;
		times(&tms2);
		tot_ms = (glong) (double) ((tms2.tms_utime - tms1.tms_utime) * 1000 / sysconf(_SC_CLK_TCK));
		g_print("PROFILING: total time %ld ms\n", tot_ms);
		gtk_text_buffer_get_bounds(buf, &its, &ita);
		pit = its;
		while (gtk_text_iter_compare(&pit, &ita) < 0) {
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
		g_print("Total number of existing marks: %ld, all marks: %ld total number of tags: %ld\n",
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

gboolean bf_textview_add_symbol(BfTextView * self, gchar * name, GdkPixbuf * pix)
{
	gpointer ptr = g_hash_table_lookup(self->symbols, name);
	if (ptr)
		return FALSE;
	BfTextViewSymbol *sym = g_new0(BfTextViewSymbol, 1);
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
		g_free((BfTextViewSymbol *) ptr);
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
	gchar *str;
	GdkColor c1, c2, c3, c4;
	GtkRcStyle *style;

	g_stpcpy(view->fg_color, fg_color);
	g_stpcpy(view->bkg_color, bg_color);
	gdk_color_parse(fg_color, &c1);
	gdk_color_parse(bg_color, &c2);
	gdk_color_dl(&c1, &c3, 0.07);
	gdk_color_dl(&c2, &c4, 0.07);
	str =
		g_strdup_printf
		("style \"bfish\" {\nGtkWidget::cursor_color=\"%s\"\nbase[NORMAL]=\"%s\"\nbase[ACTIVE]=\"%s\"\ntext[NORMAL]=\"%s\"\ntext[ACTIVE]=\"%s\"\nfg[NORMAL]=\"%s\"\nfg[ACTIVE]=\"%s\"\nbg[NORMAL]=\"%s\"\nbg[ACTIVE]=\"%s\"\n}\nclass \"BfTextView\" style \"bfish\"",
		 view->fg_color, view->bkg_color, view->bkg_color, view->fg_color, view->fg_color,
		 gdk_color_to_hexstring(&c1, FALSE), gdk_color_to_hexstring(&c3, FALSE),
		 gdk_color_to_hexstring(&c4, FALSE), gdk_color_to_hexstring(&c4, FALSE));
	gtk_rc_parse_string(str);
	g_free(str);
	style = gtk_widget_get_modifier_style(GTK_WIDGET(view));
	gtk_widget_modify_style(GTK_WIDGET(view), style);
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
	if ( !self->lang ) return;
	gtk_text_buffer_get_iter_at_mark(buf, &it, gtk_text_buffer_get_insert(buf));
	it3 = it;
	gtk_text_iter_set_line(&it3, gtk_text_iter_get_line(&it));
	gtk_text_iter_backward_char(&it);
	self->scanner.last_string = g_string_assign(self->scanner.last_string, "");
	bf_textview_scan_area(self, &it3, &it, FALSE);
	if ( self->lang && self->lang->schema_aware )
	{
		if ( self->scanner.last_string->str[0] == '<' )
			ac_run_schema(main_v->autocompletion, self->scanner.last_string->str+1, self->schemas,
				self->internal_dtd,GTK_TEXT_VIEW(self),NULL);
		else if (self->scanner.current_context && self->scanner.current_context->type == BT_TAG_BEGIN )
		{
			ac_run_tag_attributes(main_v->autocompletion, self->scanner.last_tagname,self->scanner.last_string->str, 
					self->schemas,self->internal_dtd,GTK_TEXT_VIEW(self),"=\"\"");
		}
		else
			ac_run_lang(main_v->autocompletion, self->scanner.last_string->str, self->lang->name,
				GTK_TEXT_VIEW(self),NULL);		
	}
	else
	{
		ac_run_lang(main_v->autocompletion, self->scanner.last_string->str, self->lang->name,
				GTK_TEXT_VIEW(self),NULL);
	}			
}


#endif							/* USE_SCANNER */
