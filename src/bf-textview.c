/**
 *		 Bluefish HTML Editor  -   BfTextView - Bluefish text widget
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

#include "bf-textview.h"
#include "bluefish.h"
#include <stdarg.h>
#include <string.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>


static void bf_textview_init (BfTextView * o);
static void bf_textview_class_init (BfTextViewClass * c);
static gboolean bf_textview_expose_cb (GtkWidget * widget, GdkEventExpose * event, gpointer doc);
/*static void bf_textview_changed_cb (GtkTextBuffer * textbuffer, gpointer user_data);*/
static gboolean bf_textview_mouse_cb (GtkWidget * widget, GdkEvent * event, gpointer user_data);
static void bf_textview_delete_range_cb (GtkTextBuffer * textbuffer,
					 GtkTextIter * arg1, GtkTextIter * arg2, gpointer user_data);
static void bftv_delete_blocks_from_area (BfTextView *view, GtkTextIter * arg1, GtkTextIter * arg2 );
static void bf_textview_insert_text_cb (GtkTextBuffer * textbuffer,
					GtkTextIter * arg1, gchar * arg2, gint arg3, gpointer user_data);
static void bftv_fold (GtkTextMark * mark, gboolean move_cursor);

/* --- Auxiliary closures ---- */
typedef void (*___Sig1) (BfTextView *, gpointer, gpointer, gpointer, gpointer);

static void
___marshal_Sig1 (GClosure * closure,
		 GValue * return_value G_GNUC_UNUSED,
		 guint n_param_values,
		 const GValue * param_values, gpointer invocation_hint G_GNUC_UNUSED, gpointer marshal_data)
{
   register ___Sig1 callback;
   register GCClosure *cc = (GCClosure *) closure;
   register gpointer data1, data2;

   g_return_if_fail (n_param_values == 4);

   if (G_CCLOSURE_SWAP_DATA (closure)) {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
   }
   else {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
   }

   callback = (___Sig1) (marshal_data != NULL ? marshal_data : cc->callback);

   callback ((BfTextView *) data1,
	     (gpointer) g_value_get_pointer (param_values + 1),
	     (gpointer) g_value_get_pointer (param_values + 2),
	     (gpointer) g_value_get_pointer (param_values + 3), data2);
}

typedef void (*___Sig2) (BfTextView *, gpointer, gpointer, gpointer, gpointer, gpointer, gpointer, gpointer, gpointer);

static void
___marshal_Sig2 (GClosure * closure,
		 GValue * return_value G_GNUC_UNUSED,
		 guint n_param_values,
		 const GValue * param_values, gpointer invocation_hint G_GNUC_UNUSED, gpointer marshal_data)
{
   register ___Sig2 callback;
   register GCClosure *cc = (GCClosure *) closure;
   register gpointer data1, data2;

   g_return_if_fail (n_param_values == 8);

   if (G_CCLOSURE_SWAP_DATA (closure)) {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
   }
   else {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
   }

   callback = (___Sig2) (marshal_data != NULL ? marshal_data : cc->callback);

   callback ((BfTextView *) data1,
	     (gpointer) g_value_get_pointer (param_values + 1),
	     (gpointer) g_value_get_pointer (param_values + 2),
	     (gpointer) g_value_get_pointer (param_values + 3),
	     (gpointer) g_value_get_pointer (param_values + 4),
	     (gpointer) g_value_get_pointer (param_values + 5),
	     (gpointer) g_value_get_pointer (param_values + 6),
	     (gpointer) g_value_get_pointer (param_values + 7), data2);
}

typedef void (*___Sig3) (BfTextView *, gpointer, gpointer, gpointer, gpointer, gpointer, gpointer, gpointer);

static void
___marshal_Sig3 (GClosure * closure,
		 GValue * return_value G_GNUC_UNUSED,
		 guint n_param_values,
		 const GValue * param_values, gpointer invocation_hint G_GNUC_UNUSED, gpointer marshal_data)
{
   register ___Sig3 callback;
   register GCClosure *cc = (GCClosure *) closure;
   register gpointer data1, data2;

   g_return_if_fail (n_param_values == 7);

   if (G_CCLOSURE_SWAP_DATA (closure)) {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
   }
   else {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
   }

   callback = (___Sig3) (marshal_data != NULL ? marshal_data : cc->callback);

   callback ((BfTextView *) data1,
	     (gpointer) g_value_get_pointer (param_values + 1),
	     (gpointer) g_value_get_pointer (param_values + 2),
	     (gpointer) g_value_get_pointer (param_values + 3),
	     (gpointer) g_value_get_pointer (param_values + 4),
	     (gpointer) g_value_get_pointer (param_values + 5),
	     (gpointer) g_value_get_pointer (param_values + 6), data2);
}

typedef void (*___Sig4) (BfTextView *, gpointer, gpointer, gpointer, gpointer, gpointer, gpointer);

static void
___marshal_Sig4 (GClosure * closure,
		 GValue * return_value G_GNUC_UNUSED,
		 guint n_param_values,
		 const GValue * param_values, gpointer invocation_hint G_GNUC_UNUSED, gpointer marshal_data)
{
   register ___Sig4 callback;
   register GCClosure *cc = (GCClosure *) closure;
   register gpointer data1, data2;

   g_return_if_fail (n_param_values == 6);

   if (G_CCLOSURE_SWAP_DATA (closure)) {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
   }
   else {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
   }

   callback = (___Sig4) (marshal_data != NULL ? marshal_data : cc->callback);

   callback ((BfTextView *) data1,
	     (gpointer) g_value_get_pointer (param_values + 1),
	     (gpointer) g_value_get_pointer (param_values + 2),
	     (gpointer) g_value_get_pointer (param_values + 3),
	     (gpointer) g_value_get_pointer (param_values + 4),
	     (gpointer) g_value_get_pointer (param_values + 5), data2);
}



enum {
   TOKEN_SIGNAL,
   BLOCK_BEGIN_SIGNAL,
   BLOCK_END_SIGNAL,
   BLOCK_MISPLACED_END_SIGNAL,
   NUMBER_SIGNAL,
   TAG_BEGIN_SIGNAL,
   TAG_ATTR_SIGNAL,
   TAG_END_SIGNAL,
   TAG_MISPLACED_END_SIGNAL,
   LAST_SIGNAL
};

#define BFTV_TOKEN_IDS		10000
#define BFTV_BLOCK_IDS		10000
#define BFTV_TAG_IDS			8000
#define BFTV_TAG_STATE_NUM 	16
#define BFTV_TAG_IDS_BEGIN			8001
#define BFTV_TAG_IDS_BEGIN_COMPLETED		8002
#define BFTV_TAG_IDS_END		8003
#define BFTV_TAG_IDS_EMPTY	8004

#define BFTV_UTF8_RANGE	2048


static guint object_signals[LAST_SIGNAL] = { 0 };
static GtkTextViewClass *parent_class = NULL;

GType bf_textview_get_type (void)
{
   static GType type = 0;

   if (type == 0) {
      static const GTypeInfo info = {
	 sizeof (BfTextViewClass),
	 (GBaseInitFunc) NULL,
	 (GBaseFinalizeFunc) NULL,
	 (GClassInitFunc) bf_textview_class_init,
	 (GClassFinalizeFunc) NULL,
	 NULL /* class_data */ ,
	 sizeof (BfTextView),
	 0 /* n_preallocs */ ,
	 (GInstanceInitFunc) bf_textview_init,
	 NULL
      };

      type = g_type_register_static (GTK_TYPE_TEXT_VIEW, "BfTextView", &info, (GTypeFlags) 0);
   }
   return type;
}

#define GET_NEW ((BfTextView *)g_object_new(bf_textview_get_type(), NULL))

static void bftv_expand_all(GtkWidget * widget, BfTextView *view) {
	GtkTextIter it;
	bf_textview_fold_blocks(view,FALSE);
	gtk_text_buffer_get_start_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)),&it);
	gtk_text_buffer_place_cursor (gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)), &it);	
	gtk_widget_queue_draw(GTK_WIDGET(view));
}

static void bftv_collapse_all(GtkWidget * widget, BfTextView *view) {
	GtkTextIter it;
	bf_textview_fold_blocks(view,TRUE);
	gtk_text_buffer_get_start_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)),&it);
	gtk_text_buffer_place_cursor (gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)), &it);		
	gtk_widget_queue_draw(GTK_WIDGET(view));
}


static void bf_textview_init (BfTextView * o)
{
	GtkWidget *item;
	
   o->lw_size_lines = o->lw_size_blocks = o->lw_size_sym = 0;
   o->symbols = g_hash_table_new (g_str_hash, g_str_equal);
   o->symbol_lines = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
   o->languages = g_hash_table_new (g_str_hash, g_str_equal);
   o->current_lang = NULL;
   o->autoscan = FALSE;
   o->autoscan_lines = 0;
   o->color_blocks = FALSE;
   g_queue_is_empty (&(o->scanner.block_stack));
   gdk_color_parse ("#F7F3D2", &o->block_color);
   gdk_color_parse ("#FFFFFF", &o->bkg_color);
   gdk_color_parse ("#000000", &o->fg_color);
   o->fold_menu = gtk_menu_new();
	item = gtk_menu_item_new_with_label(_("Expand all"));
	gtk_menu_append(GTK_MENU(o->fold_menu),item);
	g_signal_connect(GTK_OBJECT(item), "activate", G_CALLBACK(bftv_expand_all),o);
	item = gtk_menu_item_new_with_label(_("Collapse all"));
	gtk_menu_append(GTK_MENU(o->fold_menu),item);
	gtk_widget_show_all(o->fold_menu);
	g_signal_connect(GTK_OBJECT(item), "activate", G_CALLBACK(bftv_collapse_all),o);   
	o->hltags = NULL;
}


static void bf_textview_class_init (BfTextViewClass * c)
{
   GObjectClass *g_object_class = (GObjectClass *) c;

   parent_class = g_type_class_ref (GTK_TYPE_TEXT_VIEW);

   object_signals[TAG_BEGIN_SIGNAL] =
      g_signal_new ("tag-begin", G_TYPE_FROM_CLASS (g_object_class),
		    (GSignalFlags) (G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION),
		    G_STRUCT_OFFSET (BfTextViewClass, tag_begin), NULL, NULL,
		    ___marshal_Sig1, G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER);
   object_signals[TAG_ATTR_SIGNAL] =
      g_signal_new ("tag-attr", G_TYPE_FROM_CLASS (g_object_class),
		    (GSignalFlags) (G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION),
		    G_STRUCT_OFFSET (BfTextViewClass, tag_attr), NULL, NULL,
		    ___marshal_Sig3, G_TYPE_NONE, 6, G_TYPE_POINTER,
		    G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER);
   object_signals[TAG_END_SIGNAL] =
      g_signal_new ("tag-end", G_TYPE_FROM_CLASS (g_object_class),
		    (GSignalFlags) (G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION),
		    G_STRUCT_OFFSET (BfTextViewClass, tag_end), NULL, NULL,
		    ___marshal_Sig4, G_TYPE_NONE, 5, G_TYPE_POINTER,
		    G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER);
   object_signals[TAG_MISPLACED_END_SIGNAL] =
      g_signal_new ("tag-misplaced-end", G_TYPE_FROM_CLASS (g_object_class),
		    (GSignalFlags) (G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION),
		    G_STRUCT_OFFSET (BfTextViewClass, tag_misplaced_end),
		    NULL, NULL, ___marshal_Sig1, G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER);
   object_signals[TOKEN_SIGNAL] =
      g_signal_new ("token", G_TYPE_FROM_CLASS (g_object_class),
		    (GSignalFlags) (G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION),
		    G_STRUCT_OFFSET (BfTextViewClass, token), NULL, NULL,
		    ___marshal_Sig1, G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER);
   object_signals[BLOCK_BEGIN_SIGNAL] =
      g_signal_new ("block-begin", G_TYPE_FROM_CLASS (g_object_class),
		    (GSignalFlags) (G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION),
		    G_STRUCT_OFFSET (BfTextViewClass, block_begin), NULL,
		    NULL, ___marshal_Sig1, G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER);
   object_signals[BLOCK_END_SIGNAL] =
      g_signal_new ("block-end", G_TYPE_FROM_CLASS (g_object_class),
		    (GSignalFlags) (G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION),
		    G_STRUCT_OFFSET (BfTextViewClass, block_end), NULL, NULL,
		    ___marshal_Sig2, G_TYPE_NONE, 7, G_TYPE_POINTER,
		    G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER);
   object_signals[BLOCK_MISPLACED_END_SIGNAL] =
      g_signal_new ("block-misplaced-end", G_TYPE_FROM_CLASS (g_object_class),
		    (GSignalFlags) (G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION),
		    G_STRUCT_OFFSET (BfTextViewClass, block_misplaced_end),
		    NULL, NULL, ___marshal_Sig1, G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER);

   c->tag_begin = NULL;
   c->tag_end = NULL;
   c->tag_misplaced_end = NULL;
   c->tag_attr = NULL;
   c->token = NULL;
   c->block_begin = NULL;
   c->block_end = NULL;
   c->block_misplaced_end = NULL;
}


/* --------   SIGNALS ------------- */


/**
*	bf_textview_tag_begin:
*	@self:  BfTextView widget emitting signal
*	@tagName:  name of the tag (Warning! This string is freed after emit, so remember to copy if you want to use it later!)
* @startIter:  textbuffer iterator (#GtkTextIter) which begins tag start
* @endIter:  textbuffer iterator (#GtkTextIter) which ends tag start
*
*	This method represents signal "tag-begin" which is emitted when scanner encounters tag begin string (for example <html>) .
* Simply define method with identical parameters as below and connect it using #g_signal_connect.
*/

void bf_textview_tag_begin (BfTextView * self, gchar * tagName, GtkTextIter * startIter, GtkTextIter * endIter)
{
   GValue ___param_values[4];
   GValue ___return_val;
   memset (&___return_val, 0, sizeof (___return_val));
   memset (&___param_values, 0, sizeof (___param_values));
   g_return_if_fail (self != NULL);
   g_return_if_fail (BF_IS_TEXTVIEW (self));

   ___param_values[0].g_type = 0;
   g_value_init (&___param_values[0], G_TYPE_FROM_INSTANCE (self));
   g_value_set_instance (&___param_values[0], (gpointer) self);

   ___param_values[1].g_type = 0;
   g_value_init (&___param_values[1], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[1], (gpointer) tagName);

   ___param_values[2].g_type = 0;
   g_value_init (&___param_values[2], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[2], (gpointer) startIter);

   ___param_values[3].g_type = 0;
   g_value_init (&___param_values[3], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[3], (gpointer) endIter);


   g_signal_emitv (___param_values, object_signals[TAG_BEGIN_SIGNAL], 0 /* detail */ , &___return_val);

   g_value_unset (&___param_values[0]);
   g_value_unset (&___param_values[1]);
   g_value_unset (&___param_values[2]);
   g_value_unset (&___param_values[3]);
}

/**
*	bf_textview_tag_attr:
*	@self:  BfTextView widget emitting signal
*	@attrName:  name of the attribute (Warning! This string is freed after emit, so remember to copy if you want to use it later!)
*	@attrValue:  value of the attribute (Warning! This string is freed after emit, so remember to copy if you want to use it later!)
* @n_startIter:  textbuffer iterator (#GtkTextIter) which begins attribute name
* @n_endIter:  textbuffer iterator (#GtkTextIter) which ends attribute name
* @v_startIter:  textbuffer iterator (#GtkTextIter) which begins attribute value
* @v_endIter:  textbuffer iterator (#GtkTextIter) which ends attribute value
*
*	This method represents signal "tag-attr" which is emitted when scanner encounters tag attribute .
* Simply define method with identical parameters as below and connect it using #g_signal_connect.
*/
void
bf_textview_tag_attr (BfTextView * self, gchar * attrName, gchar * attrValue,
		      GtkTextIter * n_startIter, GtkTextIter * n_endIter,
		      GtkTextIter * v_startIter, GtkTextIter * v_endIter)
{
   GValue ___param_values[7];
   GValue ___return_val;
   memset (&___return_val, 0, sizeof (___return_val));
   memset (&___param_values, 0, sizeof (___param_values));
   g_return_if_fail (self != NULL);
   g_return_if_fail (BF_IS_TEXTVIEW (self));

   ___param_values[0].g_type = 0;
   g_value_init (&___param_values[0], G_TYPE_FROM_INSTANCE (self));
   g_value_set_instance (&___param_values[0], (gpointer) self);

   ___param_values[1].g_type = 0;
   g_value_init (&___param_values[1], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[1], (gpointer) attrName);

   ___param_values[2].g_type = 0;
   g_value_init (&___param_values[2], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[2], (gpointer) attrValue);

   ___param_values[3].g_type = 0;
   g_value_init (&___param_values[3], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[3], (gpointer) n_startIter);

   ___param_values[4].g_type = 0;
   g_value_init (&___param_values[4], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[4], (gpointer) n_endIter);

   ___param_values[5].g_type = 0;
   g_value_init (&___param_values[5], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[5], (gpointer) v_startIter);

   ___param_values[6].g_type = 0;
   g_value_init (&___param_values[6], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[6], (gpointer) v_endIter);


   g_signal_emitv (___param_values, object_signals[TAG_ATTR_SIGNAL], 0 /* detail */ , &___return_val);

   g_value_unset (&___param_values[0]);
   g_value_unset (&___param_values[1]);
   g_value_unset (&___param_values[2]);
   g_value_unset (&___param_values[3]);
   g_value_unset (&___param_values[4]);
   g_value_unset (&___param_values[5]);
   g_value_unset (&___param_values[6]);
}

/**
*	bf_textview_tag_end:
*	@self:  BfTextView widget emitting signal
*	@tagName:  name of the tag (Warning! This string is freed after emit, so remember to copy if you want to use it later!)
* @b_startIter:  textbuffer iterator (#GtkTextIter) which begins tag start
* @b_endIter:  textbuffer iterator (#GtkTextIter) which ends tag start
* @e_startIter:  textbuffer iterator (#GtkTextIter) which begins tag end
* @e_endIter:  textbuffer iterator (#GtkTextIter) which ends tag end
*
*	This method represents signal "tag-end" which is emitted when scanner encounters tag end .
* Simply define method with identical parameters as below and connect it using #g_signal_connect.
*/
void
bf_textview_tag_end (BfTextView * self, gchar * tagName,
		     GtkTextIter * b_startIter, GtkTextIter * b_endIter,
		     GtkTextIter * e_startIter, GtkTextIter * e_endIter)
{
   GValue ___param_values[6];
   GValue ___return_val;
   memset (&___return_val, 0, sizeof (___return_val));
   memset (&___param_values, 0, sizeof (___param_values));
   g_return_if_fail (self != NULL);
   g_return_if_fail (BF_IS_TEXTVIEW (self));

   ___param_values[0].g_type = 0;
   g_value_init (&___param_values[0], G_TYPE_FROM_INSTANCE (self));
   g_value_set_instance (&___param_values[0], (gpointer) self);

   ___param_values[1].g_type = 0;
   g_value_init (&___param_values[1], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[1], (gpointer) tagName);

   ___param_values[2].g_type = 0;
   g_value_init (&___param_values[2], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[2], (gpointer) b_startIter);

   ___param_values[3].g_type = 0;
   g_value_init (&___param_values[3], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[3], (gpointer) b_endIter);

   ___param_values[4].g_type = 0;
   g_value_init (&___param_values[4], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[4], (gpointer) e_startIter);

   ___param_values[5].g_type = 0;
   g_value_init (&___param_values[5], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[5], (gpointer) e_endIter);

   g_signal_emitv (___param_values, object_signals[TAG_END_SIGNAL], 0 /* detail */ , &___return_val);

   g_value_unset (&___param_values[0]);
   g_value_unset (&___param_values[1]);
   g_value_unset (&___param_values[2]);
   g_value_unset (&___param_values[3]);
   g_value_unset (&___param_values[4]);
   g_value_unset (&___param_values[5]);
}

/**
*	bf_textview_tag_misplaced_end:
*	@self:  BfTextView widget emitting signal
*	@tagName:  name of the tag (Warning! This string is freed after emit, so remember to copy if you want to use it later!)
* @startIter:  textbuffer iterator (#GtkTextIter) which begins tag end
* @endIter:  textbuffer iterator (#GtkTextIter) which ends tag end
*
*	This method represents signal "tag-misplaced-end" which is emitted when scanner encounters tag end but cannot find proper tag start.
* Simply define method with identical parameters as below and connect it using #g_signal_connect.
*/
void bf_textview_tag_misplaced_end (BfTextView * self, gchar * tagName, GtkTextIter * startIter, GtkTextIter * endIter)
{
   GValue ___param_values[4];
   GValue ___return_val;
   memset (&___return_val, 0, sizeof (___return_val));
   memset (&___param_values, 0, sizeof (___param_values));
   g_return_if_fail (self != NULL);
   g_return_if_fail (BF_IS_TEXTVIEW (self));

   ___param_values[0].g_type = 0;
   g_value_init (&___param_values[0], G_TYPE_FROM_INSTANCE (self));
   g_value_set_instance (&___param_values[0], (gpointer) self);

   ___param_values[1].g_type = 0;
   g_value_init (&___param_values[1], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[1], (gpointer) tagName);

   ___param_values[2].g_type = 0;
   g_value_init (&___param_values[2], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[2], (gpointer) startIter);

   ___param_values[3].g_type = 0;
   g_value_init (&___param_values[3], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[3], (gpointer) endIter);


   g_signal_emitv (___param_values, object_signals[TAG_MISPLACED_END_SIGNAL], 0 /* detail */ , &___return_val);

   g_value_unset (&___param_values[0]);
   g_value_unset (&___param_values[1]);
   g_value_unset (&___param_values[2]);
   g_value_unset (&___param_values[3]);

}

/**
*	bf_textview_token:
*	@self:  BfTextView widget emitting signal
*	@tokenDef:  #BfLangToken structure describing language token found
* @startIter:  textbuffer iterator (#GtkTextIter) which begins token
* @endIter:  textbuffer iterator (#GtkTextIter) which ends token
*
*	This method represents signal "token" which is emitted when scanner encounters token defined in configuration file.
* Simply define method with identical parameters as below and connect it using #g_signal_connect.
*/
void bf_textview_token (BfTextView * self, BfLangToken * tokenDef, GtkTextIter * startIter, GtkTextIter * endIter)
{
   GValue ___param_values[4];
   GValue ___return_val;
   memset (&___return_val, 0, sizeof (___return_val));
   memset (&___param_values, 0, sizeof (___param_values));
   g_return_if_fail (self != NULL);
   g_return_if_fail (BF_IS_TEXTVIEW (self));

   ___param_values[0].g_type = 0;
   g_value_init (&___param_values[0], G_TYPE_FROM_INSTANCE (self));
   g_value_set_instance (&___param_values[0], (gpointer) self);

   ___param_values[1].g_type = 0;
   g_value_init (&___param_values[1], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[1], (gpointer) tokenDef);

   ___param_values[2].g_type = 0;
   g_value_init (&___param_values[2], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[2], (gpointer) startIter);

   ___param_values[3].g_type = 0;
   g_value_init (&___param_values[3], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[3], (gpointer) endIter);

   g_signal_emitv (___param_values, object_signals[TOKEN_SIGNAL], 0 /* detail */ , &___return_val);

   g_value_unset (&___param_values[0]);
   g_value_unset (&___param_values[1]);
   g_value_unset (&___param_values[2]);
   g_value_unset (&___param_values[3]);
}

/**
*	bf_textview_block_begin:
*	@self:  BfTextView widget emitting signal
*	@blockDef:  #BfLangBlock structure describing language block
* @startIter:  textbuffer iterator (#GtkTextIter) which begins block string
* @endIter:  textbuffer iterator (#GtkTextIter) which ends block string
*
*	This method represents signal "block-begin" which is emitted when scanner encounters begin of block defined in configuration file.
* Simply define method with identical parameters as below and connect it using #g_signal_connect.
*/
void bf_textview_block_begin (BfTextView * self, BfLangBlock * blockDef, GtkTextIter * startIter, GtkTextIter * endIter)
{
   GValue ___param_values[4];
   GValue ___return_val;
   memset (&___return_val, 0, sizeof (___return_val));
   memset (&___param_values, 0, sizeof (___param_values));
   g_return_if_fail (self != NULL);
   g_return_if_fail (BF_IS_TEXTVIEW (self));

   ___param_values[0].g_type = 0;
   g_value_init (&___param_values[0], G_TYPE_FROM_INSTANCE (self));
   g_value_set_instance (&___param_values[0], (gpointer) self);

   ___param_values[1].g_type = 0;
   g_value_init (&___param_values[1], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[1], (gpointer) blockDef);

   ___param_values[2].g_type = 0;
   g_value_init (&___param_values[2], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[2], (gpointer) startIter);

   ___param_values[3].g_type = 0;
   g_value_init (&___param_values[3], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[3], (gpointer) endIter);


   g_signal_emitv (___param_values, object_signals[BLOCK_BEGIN_SIGNAL], 0 /* detail */ , &___return_val);

   g_value_unset (&___param_values[0]);
   g_value_unset (&___param_values[1]);
   g_value_unset (&___param_values[2]);
   g_value_unset (&___param_values[3]);

}

/**
*	bf_textview_block_end:
*	@self:  BfTextView widget emitting signal
*	@blockDef:  #BfLangBlock structure describing language block
* @b_startIter:  textbuffer iterator (#GtkTextIter) which begins block starting string
* @b_endIter:  textbuffer iterator (#GtkTextIter) which ends block starting string
* @e_startIter:  textbuffer iterator (#GtkTextIter) which begins block ending string
* @e_endIter:  textbuffer iterator (#GtkTextIter) which ends block ending string
* @startMark:  textbuffer mark (#GtkTextMark) allocated at the block beginning (marks positions are preserved...)
* @endMark:  textbuffer mark (#GtkTextMark) allocated at the block end
*
*	This method represents signal "block-end" which is emitted when scanner encounters end of block defined in configuration file.
* Simply define method with identical parameters as below and connect it using #g_signal_connect.
*/
void
bf_textview_block_end (BfTextView * self, BfLangBlock * blockDef,
		       GtkTextIter * b_startIter, GtkTextIter * b_endIter,
		       GtkTextIter * e_startIter, GtkTextIter * e_endIter,
		       GtkTextMark * startMark, GtkTextMark * endMark)
{
   GValue ___param_values[8];
   GValue ___return_val;
   memset (&___return_val, 0, sizeof (___return_val));
   memset (&___param_values, 0, sizeof (___param_values));
   g_return_if_fail (self != NULL);
   g_return_if_fail (BF_IS_TEXTVIEW (self));

   ___param_values[0].g_type = 0;
   g_value_init (&___param_values[0], G_TYPE_FROM_INSTANCE (self));
   g_value_set_instance (&___param_values[0], (gpointer) self);

   ___param_values[1].g_type = 0;
   g_value_init (&___param_values[1], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[1], (gpointer) blockDef);

   ___param_values[2].g_type = 0;
   g_value_init (&___param_values[2], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[2], (gpointer) b_startIter);

   ___param_values[3].g_type = 0;
   g_value_init (&___param_values[3], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[3], (gpointer) b_endIter);

   ___param_values[4].g_type = 0;
   g_value_init (&___param_values[4], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[4], (gpointer) e_startIter);

   ___param_values[5].g_type = 0;
   g_value_init (&___param_values[5], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[5], (gpointer) e_endIter);

   ___param_values[6].g_type = 0;
   g_value_init (&___param_values[6], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[6], (gpointer) startMark);

   ___param_values[7].g_type = 0;
   g_value_init (&___param_values[7], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[7], (gpointer) endMark);


   g_signal_emitv (___param_values, object_signals[BLOCK_END_SIGNAL], 0 /* detail */ , &___return_val);

   g_value_unset (&___param_values[0]);
   g_value_unset (&___param_values[1]);
   g_value_unset (&___param_values[2]);
   g_value_unset (&___param_values[3]);
   g_value_unset (&___param_values[4]);
   g_value_unset (&___param_values[5]);
   g_value_unset (&___param_values[6]);
   g_value_unset (&___param_values[7]);

}

/**
*	bf_textview_block_misplaced_end:
*	@self:  BfTextView widget emitting signal
*	@blockDef:  #BfLangBlock structure describing language block
* @startIter:  textbuffer iterator (#GtkTextIter) which begins block ending string
* @endIter:  textbuffer iterator (#GtkTextIter) which ends block ending string
*
*	This method represents signal "block-misplaced-end" which is emitted when scanner encounters end of block but cannot find proper beginning.
* Simply define method with identical parameters as below and connect it using #g_signal_connect.
*/
void
bf_textview_block_misplaced_end (BfTextView * self, BfLangBlock * blockDef,
				 GtkTextIter * startIter, GtkTextIter * endIter)
{
   GValue ___param_values[4];
   GValue ___return_val;
   memset (&___return_val, 0, sizeof (___return_val));
   memset (&___param_values, 0, sizeof (___param_values));
   g_return_if_fail (self != NULL);
   g_return_if_fail (BF_IS_TEXTVIEW (self));

   ___param_values[0].g_type = 0;
   g_value_init (&___param_values[0], G_TYPE_FROM_INSTANCE (self));
   g_value_set_instance (&___param_values[0], (gpointer) self);

   ___param_values[1].g_type = 0;
   g_value_init (&___param_values[1], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[1], (gpointer) blockDef);

   ___param_values[2].g_type = 0;
   g_value_init (&___param_values[2], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[2], (gpointer) startIter);

   ___param_values[3].g_type = 0;
   g_value_init (&___param_values[3], G_TYPE_POINTER);
   g_value_set_pointer (&___param_values[3], (gpointer) endIter);

   g_signal_emitv (___param_values, object_signals[BLOCK_MISPLACED_END_SIGNAL], 0 /* detail */ , &___return_val);

   g_value_unset (&___param_values[0]);
   g_value_unset (&___param_values[1]);
   g_value_unset (&___param_values[2]);
   g_value_unset (&___param_values[3]);
}

static void bftv_update_iters(BfTextView *view, TBfBlock *b) {
	GtkTextIter it;
	GtkTextBuffer *buff = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	gint i;
	
	if ( !b ) return;
	gtk_text_buffer_get_iter_at_mark(buff,&it,b->mark_begin);
	b->b_start = it;
	for(i=0;i<b->b_len;i++)
		gtk_text_iter_forward_char(&it);
	b->b_end = it;
	gtk_text_buffer_get_iter_at_mark(buff,&it,b->mark_end);
	b->e_start = it;
	for(i=0;i<b->e_len;i++)
		gtk_text_iter_forward_char(&it);
	b->e_end = it;		
}


GtkTextMark *bftv_get_block_at_iter (GtkTextIter * it)
{
   GSList *lst = gtk_text_iter_get_marks (it);
   GtkTextMark *block_mark = NULL;
   gchar *pomstr = NULL;

   while (lst && lst->data) {
      pomstr = g_object_get_data (G_OBJECT (lst->data), "type");
      if (pomstr && (strcmp (pomstr, "block_begin") == 0 || strcmp (pomstr, "block_end") == 0)) {
	 block_mark = (GtkTextMark *) lst->data;
	 break;
      }
      lst = g_slist_next (lst);
   }
   g_slist_free (lst);
   return block_mark;
}


GtkTextMark *bftv_get_first_block_at_line (GtkTextIter * it, gboolean not_single)
{
   GtkTextIter it2, it3;
   GtkTextMark *mark = NULL;
   TBfBlock *bb;

   if (!it)
      return NULL;
   it2 = it3 = *it;
   gtk_text_iter_set_line (&it2, gtk_text_iter_get_line (it));
   gtk_text_iter_forward_to_line_end (&it3);
   gtk_text_iter_backward_char (&it3);

   while (gtk_text_iter_compare (&it2, &it3) <= 0 && gtk_text_iter_get_line (&it2) == gtk_text_iter_get_line (it)) {
      mark = bftv_get_block_at_iter (&it2);
      if (mark) {
      	if ( not_single ) {
      		bb = (TBfBlock*)g_object_get_data(G_OBJECT(mark),"pointer");
      		if ( !bb->single_line ) return mark;
      	} else  return mark;
      }
      if (!gtk_text_iter_forward_char (&it2))
	 break;
   }
   return NULL;
}

GtkTextMark *bftv_get_last_block_at_line (GtkTextIter * it)
{
   GtkTextIter it2, it3;
   GtkTextMark *mark = NULL, *mark2 = NULL;

   if (!it)
      return NULL;
   it2 = it3 = *it;
   gtk_text_iter_set_line (&it2, gtk_text_iter_get_line (it));
   gtk_text_iter_forward_to_line_end (&it3);
   gtk_text_iter_backward_char (&it3);

   while (gtk_text_iter_compare (&it2, &it3) <= 0 && gtk_text_iter_get_line (&it2) == gtk_text_iter_get_line (it)) {
      mark = bftv_get_block_at_iter (&it2);
      if (mark) {
	 mark2 = mark;
      }
      if (!gtk_text_iter_forward_char (&it2))
	 break;
   }
   return mark2;
}




/* ------------------------    SCANNING ---------------------------- */


/**
*	bf_textview_scan:
*	@self:  BfTextView widget 
*
*	Scan whole buffer text.
*/
void bf_textview_scan (BfTextView * self)
{
   GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self));
   GtkTextIter its, ite;

   g_return_if_fail (self != NULL);
   g_return_if_fail (BF_IS_TEXTVIEW (self));
   g_return_if_fail (buf != NULL);
   gtk_text_buffer_get_start_iter (buf, &its);
   gtk_text_buffer_get_end_iter (buf, &ite);
   if ( gtk_text_iter_equal(&its,&ite) ) return;  
   bf_textview_scan_area (self, &its, &ite);
}

static gint bftv_prepare_indent_blocks(BfTextView *view, gint start) {
	gint currline,lev;
	TBfBlock *bb;
	GtkTextIter it1,it2;
	GtkTextMark *mark=NULL,*mark2=NULL;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	BfLangConfig *cfg = view->current_lang;
	
	lev = g_array_index(cfg->line_indent,gint,start+1);
	currline = start+1;
	while ( g_array_index(cfg->line_indent,gint,currline) >= lev && currline<cfg->line_indent->len ) {
		if ( g_array_index(cfg->line_indent,gint,currline) > lev )
			currline = bftv_prepare_indent_blocks(view,currline-1);		
		currline++;
	}	
	bb = g_new0(TBfBlock,1);
	bb->def = cfg->iblock;
	gtk_text_buffer_get_iter_at_line(buffer,&it1,start);
	gtk_text_buffer_get_iter_at_line(buffer,&it2,currline-1);
	bb->b_start = it1;bb->e_start = it2;
	gtk_text_iter_forward_to_line_end(&it1);
	gtk_text_iter_forward_to_line_end(&it2);
	bb->b_end = it1;bb->e_end = it2;
	bb->single_line = FALSE;
	if (!bftv_get_block_at_iter (&(bb->b_start))) {
		   mark = gtk_text_buffer_create_mark (buffer, NULL, &(bb->b_start), FALSE);
			g_object_set_data (G_OBJECT (mark), "type", "block_begin");
			g_object_set_data (G_OBJECT (mark), "folded", "false");
			g_object_set_data (G_OBJECT (mark), "pointer", bb);
			bb->mark_begin = mark;
			bb->b_len = g_utf8_strlen(gtk_text_iter_get_slice(&bb->b_start,&bb->b_end),-1)-1;
			mark2 = gtk_text_buffer_create_mark (buffer, NULL, &(bb->e_start), FALSE);
			g_object_set_data (G_OBJECT (mark2), "type", "block_end");
			g_object_set_data (G_OBJECT (mark2), "folded", "false");
			g_object_set_data (G_OBJECT (mark2), "pointer", bb);
			bb->e_len = g_utf8_strlen(gtk_text_iter_get_slice(&bb->e_start,&bb->e_end),-1);
			bb->mark_end = mark2;
			gtk_text_buffer_apply_tag_by_name (buffer, "_block_", &(bb->b_start), &(bb->e_end));			
	}
	bf_textview_block_end (view, NULL,	&bb->b_start, &bb->b_end, &bb->e_start, &bb->e_end, mark, mark2);	/* EMIT SIGNAL */	
	return currline-1;	
}

/**
*	bf_textview_scan_area:
*	@self:  BfTextView widget 
* @start: textbuffer iterator at start of area
* @end: textbuffer iterator at end of area
*
*	Scan buffer area - this is main scanning function, the rest of "scans" uses this method.
*/
void bf_textview_scan_area (BfTextView * self, GtkTextIter * start, GtkTextIter * end)
{
   GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self));
   GtkTextIter its, ita, pit, previt;
   gunichar c;
   gint pos, pos2,iaux,lev;
   gpointer ptr = NULL, ptr2 = NULL;
   GHashTable *ctab = NULL;
   gboolean block_found = FALSE,tag_found=FALSE, regexp_found = FALSE;
   GtkTextMark *mark, *mark2;
   gboolean magic = FALSE, st = FALSE;
   gint currstate = 0;
   TBfTag *tmp_tag=NULL;
   GList *lst=NULL;
   
   g_return_if_fail (self != NULL);
   g_return_if_fail (self->current_lang != NULL);
   g_return_if_fail (BF_IS_TEXTVIEW (self));

   its = *start;
   ita = *start;
   self->current_context = NULL;
   self->scanner.state = BFTV_STATE_NORMAL;
   ptr = ptr2 = NULL;
   ctab = self->current_lang->dfa;
   self->scanner.current_block = NULL;
   pos = pos2 = 0;
   currstate = 0;
   while (!g_queue_is_empty (&(self->scanner.block_stack)))
      g_queue_pop_head (&(self->scanner.block_stack));
   while (!g_queue_is_empty (&(self->scanner.tag_stack)))
      g_queue_pop_head (&(self->scanner.tag_stack));
      
	bftv_delete_blocks_from_area(self,start,end);
	lst = g_list_first(self->hltags);
	while ( lst ) {
		gtk_text_buffer_remove_tag(buf,GTK_TEXT_TAG(lst->data),start,end);
		lst = g_list_next(lst);
	}

   while (gtk_text_iter_compare (&ita, end) <= 0) {

      if (gtk_text_iter_compare (&ita, end) == 0 && !magic)
	       magic = TRUE;
      else {
	       if (gtk_text_iter_compare (&ita, end) == 0 && magic)
	        break;
      }
      c = gtk_text_iter_get_char (&ita);

		/* LINE INDENT */
		if ( self->current_lang->indent_blocks && c=='\n') {
			iaux = gtk_text_iter_get_line(&ita);
			pit = ita;
			gtk_text_iter_set_line(&pit,iaux);
			lev = 0;
			while ( (gtk_text_iter_get_char(&pit)==' ' || gtk_text_iter_get_char(&pit)=='\t') && gtk_text_iter_get_char(&pit)!='\n' ) {
				lev++;
				gtk_text_iter_forward_char(&pit);
			}
			if (self->current_lang->line_indent->len>iaux)
				self->current_lang->line_indent = g_array_remove_index(self->current_lang->line_indent,iaux);
			g_array_insert_val(self->current_lang->line_indent,iaux,lev);
		}

		/* ESCAPED CHARACTERS */
		while ( self->current_lang->escapes[(gint)c] )
		{
			gtk_text_iter_forward_char(&ita);
			gtk_text_iter_forward_char(&ita);
			c = gtk_text_iter_get_char (&ita);
		}

      /* MARKUP TAGS */
      if (!self->scanner.current_block)
	    st = self->current_lang->scan_tags;
      else
	    st = self->current_lang->scan_tags && self->scanner.current_block->def->markup;

      if (st && self->scanner.state != BFTV_STATE_DONT_SCAN &&
	        self->scanner.state != BFTV_STATE_TOKEN && 
	        (gint) c < 256) 
	   {
			
	   	if (self->current_lang->case_sensitive)
	    		currstate = self->current_lang->scan_tag_table[(gint) c][currstate];
	   	else
	    		currstate = self->current_lang->scan_tag_table[(gint) g_unichar_toupper (c)][currstate];

	 		if (currstate > 0) {
	 			tag_found = FALSE;	 			
	 			previt = ita;
				while (gtk_text_iter_compare (&ita, end) <= 0 && currstate > 0)	    	
				{
					 switch ( currstate ) {
					 	case 1: /* tag start/end */
	       				pit = ita;
	       				gtk_text_iter_forward_char (&pit);
	       				if (gtk_text_iter_get_char (&pit) == '/')	  {
	       				   self->scanner.current_tag.e_start = ita;
	       				}   
	       				else	  self->scanner.current_tag.b_start = ita;
					 	break;
					 	case 2: /* tag start/end continued */
					 		if ( !self->scanner.current_tag.name )
					 			self->scanner.current_tag.name = g_strdup_printf("%c",c);
					 		else {
					 			gchar *tofree = self->scanner.current_tag.name;
					 			self->scanner.current_tag.name = g_strdup_printf("%s%c",self->scanner.current_tag.name,c);
					 			g_free(tofree);
					 		}						 	
					 	break;
					 	case 3: /* tag end continued */
					 	 if ( c!='/' ) {
					 		if ( !self->scanner.current_tag.name )
					 			self->scanner.current_tag.name = g_strdup_printf("%c",c);
					 		else {
					 			gchar *tofree = self->scanner.current_tag.name;
					 			self->scanner.current_tag.name = g_strdup_printf("%s%c",self->scanner.current_tag.name,c);
					 			g_free(tofree);
					 		}						 						 	
					 	 }	
					 	break;
	    				case 6:		 /* attribute name */	    					
					 		if ( !self->scanner.current_attr.name ) {
					 			self->scanner.current_attr.name_start = ita;
					 			self->scanner.current_attr.name = g_strdup_printf("%c",c);
					 		}	
					 		else {
					 			gchar *tofree = self->scanner.current_attr.name;
					 			self->scanner.current_attr.name = g_strdup_printf("%s%c",self->scanner.current_attr.name,c);
					 			g_free(tofree);
					 		}						 		    					
	      			break;
	      			case 7: /* attribute name end */
	      				self->scanner.current_attr.name_end = ita;
	      			break;
	    				case 8:		/* attr value start */
	    				case 13:
	    				 if ( c!= '"' ) {	    					
					 		if ( !self->scanner.current_attr.value ) {
					 			self->scanner.current_attr.value_start = ita;
					 			self->scanner.current_attr.value = g_strdup_printf("%c",c);
					 		}	
					 		else {
					 			gchar *tofree = self->scanner.current_attr.value;
					 			self->scanner.current_attr.value = g_strdup_printf("%s%c",self->scanner.current_attr.value,c);
					 			g_free(tofree);
					 		}
					 	 }							 		    						    					
					 	break;
				      case 9:		 /* attribute end */
				      	self->scanner.current_attr.value_end = ita;
				      	if ( self->current_lang->restricted_tags_only ) {
				      		ptr = g_hash_table_lookup(self->current_lang->restricted_tags,self->scanner.current_tag.name);
				      		if ( ptr ) {
				      			if ( g_hash_table_lookup((GHashTable*)ptr,self->scanner.current_attr.name)  ){
					       				bf_textview_tag_attr (self, 
					       					self->scanner.current_attr.name,
					       					self->scanner.current_attr.value,
					       					&self->scanner.current_attr.name_start,&self->scanner.current_attr.name_end,
					       					&self->scanner.current_attr.value_start,&self->scanner.current_attr.value_end); /* EMIT SIGNAL */				      			
				      			}
				      		}
				      	} 
				      	else
	       				bf_textview_tag_attr (self, 
	       					self->scanner.current_attr.name,
	       					self->scanner.current_attr.value,
	       					&self->scanner.current_attr.name_start,&self->scanner.current_attr.name_end,
	       					&self->scanner.current_attr.value_start,&self->scanner.current_attr.value_end); /* EMIT SIGNAL */
	       					g_free(self->scanner.current_attr.name);
	       					self->scanner.current_attr.name = NULL;
	       					g_free(self->scanner.current_attr.value);
	       					self->scanner.current_attr.value = NULL;
				      break;
	    				case 15:		 /* attribute end s.v.  */
				      	self->scanner.current_attr.value_end = ita;
				      	if ( self->current_lang->restricted_tags_only ) {
				      		ptr = g_hash_table_lookup(self->current_lang->restricted_tags,self->scanner.current_tag.name);
				      		if ( ptr ) {
				      			if ( g_hash_table_lookup((GHashTable*)ptr,self->scanner.current_attr.name)  ){
				       				bf_textview_tag_attr (self, 
				       					self->scanner.current_attr.name,
				       					self->scanner.current_attr.value,
				       					&self->scanner.current_attr.name_start,&self->scanner.current_attr.name_end,
				       					&self->scanner.current_attr.value_start,&self->scanner.current_attr.value_end); /* EMIT SIGNAL */				      			
				      			}
				      		}
				      	}
				      	else
	       				bf_textview_tag_attr (self, 
	       					self->scanner.current_attr.name,
	       					self->scanner.current_attr.value,
	       					&self->scanner.current_attr.name_start,&self->scanner.current_attr.name_end,
	       					&self->scanner.current_attr.value_start,&self->scanner.current_attr.value_end); /* EMIT SIGNAL */
	       					g_free(self->scanner.current_attr.name);
	       					self->scanner.current_attr.name = NULL;
	       					g_free(self->scanner.current_attr.value);
	       					self->scanner.current_attr.value = NULL;
	       				gtk_text_iter_backward_char (&ita);
	       			break;
	    				case BFTV_TAG_IDS_BEGIN_COMPLETED:
	       				self->scanner.current_tag.b_end = ita;
	       				if ( self->current_lang->restricted_tags_only ) {
	       					ptr = g_hash_table_lookup(self->current_lang->restricted_tags,self->scanner.current_tag.name);
	       					if ( ptr )
		       					bf_textview_tag_begin (self, self->scanner.current_tag.name, 
		       						&self->scanner.current_tag.b_start, &ita);	 /* EMIT SIGNAL */	       					
	       				} 
	       				else
	       					bf_textview_tag_begin (self, self->scanner.current_tag.name, 
	       						&self->scanner.current_tag.b_start, &ita);	 /* EMIT SIGNAL */
	       				currstate = 0;
	       				tmp_tag = g_new0(TBfTag,1);
	       				tmp_tag->name = g_strdup(self->scanner.current_tag.name);
	       				tmp_tag->b_start = self->scanner.current_tag.b_start;
	       				tmp_tag->b_end = self->scanner.current_tag.b_end;
	       				g_queue_push_head (&(self->scanner.tag_stack), tmp_tag);
	       				g_free(self->scanner.current_tag.name);
	       				self->scanner.current_tag.name = NULL;
	      				tag_found = TRUE;
	      			break;
	    				case BFTV_TAG_IDS_EMPTY:
	       				self->scanner.current_tag.b_end = ita;
	       				self->scanner.current_tag.e_end = ita;
	       				if ( self->current_lang->restricted_tags_only ) {
	       					ptr = g_hash_table_lookup(self->current_lang->restricted_tags,self->scanner.current_tag.name);
	       					if ( ptr ) {
			       				bf_textview_tag_begin (self, self->scanner.current_tag.name, 
			       					&self->scanner.current_tag.b_start, &ita);	 /* EMIT SIGNAL */
			       				bf_textview_tag_end (self, self->scanner.current_tag.name, 
				       				&self->scanner.current_tag.b_start, &self->scanner.current_tag.b_end, 
				       				&self->scanner.current_tag.e_start, &self->scanner.current_tag.e_end);	 /* EMIT SIGNAL */
		       				}		
	       				} 
	       				else
							{	       				
		       				bf_textview_tag_begin (self, self->scanner.current_tag.name, 
		       					&self->scanner.current_tag.b_start, &ita);	 /* EMIT SIGNAL */
		       				bf_textview_tag_end (self, self->scanner.current_tag.name, 
			       				&self->scanner.current_tag.b_start, &self->scanner.current_tag.b_end, 
			       				&self->scanner.current_tag.e_start, &self->scanner.current_tag.e_end);	 /* EMIT SIGNAL */
			       		}		
	       				g_free(self->scanner.current_tag.name);
	       				self->scanner.current_tag.name = NULL;
	       				currstate = 0;
	       				tag_found = TRUE;
	       			break;
	    				case BFTV_TAG_IDS_END:
	       				ptr = NULL;
	       				self->scanner.current_tag.e_end = ita;
	       				ptr = g_queue_peek_head (&(self->scanner.tag_stack));
	       				if (ptr && 
	       				    strcmp (((TBfTag *) ptr)->name, self->scanner.current_tag.name ) == 0) {
	       				    	self->scanner.current_tag.b_start = ((TBfTag *) ptr)->b_start;
	       				    	self->scanner.current_tag.b_end = ((TBfTag *) ptr)->b_end;
			       				if ( self->current_lang->restricted_tags_only ) {
			       					ptr = g_hash_table_lookup(self->current_lang->restricted_tags,self->scanner.current_tag.name);
			       					if ( ptr ) {
						  						bf_textview_tag_end (self, 
						  							self->scanner.current_tag.name, 
						  							&self->scanner.current_tag.b_start, &self->scanner.current_tag.b_end, 
						  							&self->scanner.current_tag.e_start, &self->scanner.current_tag.e_end);	 /* EMIT SIGNAL */			       					
				       				}		
			       				} 
			       				else	       				    
				  						bf_textview_tag_end (self, 
				  							self->scanner.current_tag.name, 
				  							&self->scanner.current_tag.b_start, &self->scanner.current_tag.b_end, 
				  							&self->scanner.current_tag.e_start, &self->scanner.current_tag.e_end);	 /* EMIT SIGNAL */
		  						ptr = g_queue_pop_head (&(self->scanner.tag_stack));
		  						if ( ptr ) {
			  						g_free(((TBfTag*)ptr)->name);
			  						g_free (ptr);
			  					}	
		  						g_free(self->scanner.current_tag.name);
		  						self->scanner.current_tag.name = NULL;
	       				}
	       				else {
			       				if ( self->current_lang->restricted_tags_only ) {
			       					ptr = g_hash_table_lookup(self->current_lang->restricted_tags,self->scanner.current_tag.name);
			       					if ( ptr ) {
					  						bf_textview_tag_misplaced_end (self, 
					  							self->scanner.current_tag.name, 
					  							&self->scanner.current_tag.e_start, &ita);	  /* EMIT SIGNAL  */
				       				}		
			       				} 
			       				else	       				    	       				
				  						bf_textview_tag_misplaced_end (self, 
				  							self->scanner.current_tag.name, 
				  							&self->scanner.current_tag.e_start, &ita);	  /* EMIT SIGNAL  */
		  						/* try to dig for proper one */
		  						gboolean ff = FALSE;
		  						while (!g_queue_is_empty(&self->scanner.tag_stack))	{
		  							ptr = g_queue_pop_head (&(self->scanner.tag_stack));
		  							if ( strcmp(((TBfTag*)ptr)->name,self->scanner.current_tag.name) == 0 ) { ff=TRUE;  break; }
		  							if ( ptr ) {
			  							g_free(((TBfTag*)ptr)->name);
			  							g_free(ptr);
			  						}	
		  						}
		  						if ( ff ) {
		  							self->scanner.current_tag.b_start = ((TBfTag*)ptr)->b_start;
		  							self->scanner.current_tag.b_end = ((TBfTag*)ptr)->b_end;
			       				if ( self->current_lang->restricted_tags_only ) {
			       					ptr = g_hash_table_lookup(self->current_lang->restricted_tags,self->scanner.current_tag.name);
			       					if ( ptr ) {
					  						bf_textview_tag_end (self, 
					  							self->scanner.current_tag.name, 
					  							&self->scanner.current_tag.b_start, &self->scanner.current_tag.b_end, 
					  							&self->scanner.current_tag.e_start, &self->scanner.current_tag.e_end);	 /* EMIT SIGNAL */		  							
				       				}		
			       				} 
			       				else	       				    	       						  						
				  						bf_textview_tag_end (self, 
				  							self->scanner.current_tag.name, 
				  							&self->scanner.current_tag.b_start, &self->scanner.current_tag.b_end, 
				  							&self->scanner.current_tag.e_start, &self->scanner.current_tag.e_end);	 /* EMIT SIGNAL */		  							
			  						g_free(((TBfTag*)ptr)->name);
			  						g_free(ptr);			
		  						}
		  						g_free(self->scanner.current_tag.name);	
		  						self->scanner.current_tag.name = NULL;
	       				}
	       				currstate = 0;
	       				tag_found = TRUE;
	       			break;
					 	
					 } /* switch */
					 if ( tag_found ) break;
			       if  (!gtk_text_iter_forward_char (&ita)) { currstate=0; break; }
			       c = gtk_text_iter_get_char (&ita);
			   	 if (self->current_lang->case_sensitive)
	    				currstate = self->current_lang->scan_tag_table[(gint) c][currstate];
	   			else
	    				currstate = self->current_lang->scan_tag_table[(gint) g_unichar_toupper (c)][currstate];
				} /* while */
				if ( !tag_found ) {
					currstate = 0;
					ita = previt;
					c = gtk_text_iter_get_char (&ita);
				} else continue;
	 } /* state>0 */
  }
  
  
      /* BLOCKS */
  if (self->current_lang->scan_blocks
	  		&& self->scanner.state != BFTV_STATE_TOKEN 
	  		&& (gint) c < 256) {

		  		
	 /* BLOCKS BEGINS */
	 if ( self->scanner.state != BFTV_STATE_DONT_SCAN ) {
	 currstate = self->current_lang->scan_bb_table[(gint) c][currstate];
	 if (currstate > 0) {
	    block_found = FALSE;
	    previt = ita;
	    while (gtk_text_iter_compare (&ita, end) <= 0 && currstate > 0) {
	       if (currstate > BFTV_BLOCK_IDS) {
	       	BfLangBlock *tmp = (BfLangBlock *) g_hash_table_lookup (self->current_lang->blocks_id, &currstate);
	       	if ( self->scanner.current_block != NULL && self->scanner.current_block->def == tmp 
	       		  && strcmp(self->scanner.current_block->def->begin,self->scanner.current_block->def->end)==0) {
	       		/* probably this is the end of block */
	       		block_found = FALSE;
	       		break;
	       	}
		  		self->scanner.current_block = g_new0 (TBfBlock, 1);
		  		self->scanner.current_block->def = (BfLangBlock *) g_hash_table_lookup (self->current_lang->blocks_id, &currstate);
		 		self->scanner.current_block->b_start = its;
		  		self->scanner.current_block->b_end = ita;
		  		gtk_text_iter_forward_char (&(self->scanner.current_block->b_end));
		  		bf_textview_block_begin (self, self->scanner.current_block->def, &its, &ita);	/* EMIT SIGNAL */
		  		self->current_context = self->scanner.current_block->def;
		  		g_queue_push_head (&(self->scanner.block_stack), self->scanner.current_block);
		  		block_found = TRUE;
		  		if (!self->current_context->scanned)
		     		self->scanner.state = BFTV_STATE_DONT_SCAN;
		  		break;
	       }
	       gtk_text_iter_forward_char (&ita);
	       c = gtk_text_iter_get_char (&ita);
	       if ((gint) c < 256)
		  			currstate = self->current_lang->scan_bb_table[(gint) c][currstate];
	       else
		  			currstate = 0;
	    }			/* while */
	    if (block_found) {
	       currstate = 0;
	       its = ita;
	       continue;
	    } else { ita = previt; c = gtk_text_iter_get_char (&ita); currstate=0; }
	 }
    } /* dont scan */
	
	 /* BLOCKS ENDS */
	 currstate = self->current_lang->scan_be_table[(gint) c][currstate];
	 if (currstate > 0) {
	    block_found = FALSE;
	    previt = ita;
	    while (gtk_text_iter_compare (&ita, end) <= 0 && currstate > 0) {
	       if (currstate > BFTV_BLOCK_IDS) {
				  TBfBlock *bb = NULL;
				  if (!g_queue_is_empty (&(self->scanner.block_stack)))    
				  	bb = (TBfBlock *)g_queue_peek_head (&(self->scanner.block_stack));
		  		  ptr2 = g_hash_table_lookup (self->current_lang->blocks_id, &currstate);
		  		  if (self->scanner.current_block != NULL && bb && bb->def == ptr2 ) {
		      		g_queue_pop_head (&(self->scanner.block_stack));
		     			self->scanner.current_block->e_start = its;
		     			self->scanner.current_block->e_end = ita;
		     			if (gtk_text_iter_get_line(&(self->scanner.current_block->b_start)) == gtk_text_iter_get_line (&ita))
							self->scanner.current_block->single_line = TRUE;
		     			else
							self->scanner.current_block->single_line = FALSE;
		     			block_found = TRUE;
		    			 mark = mark2 = NULL;
		     			if (self->current_lang->scan_blocks && self->scanner.current_block->def->foldable) {
							if (!bftv_get_block_at_iter (&(self->scanner.current_block->b_start))) {
							   mark = gtk_text_buffer_create_mark (buf, NULL, &(self->scanner.current_block->b_start), FALSE);
			  					g_object_set_data (G_OBJECT (mark), "type", "block_begin");
			   				g_object_set_data (G_OBJECT (mark), "folded", "false");
			   				g_object_set_data (G_OBJECT (mark), "pointer", self->scanner.current_block);
			   				self->scanner.current_block->mark_begin = mark;
			   				self->scanner.current_block->b_len = g_utf8_strlen(gtk_text_iter_get_slice(
			   				&self->scanner.current_block->b_start,&self->scanner.current_block->b_end),-1)-1;
			   				mark2 = gtk_text_buffer_create_mark (buf, NULL, &its, FALSE);
			   				g_object_set_data (G_OBJECT (mark2), "type", "block_end");
			   				g_object_set_data (G_OBJECT (mark2), "folded", "false");
			   				g_object_set_data (G_OBJECT (mark2), "pointer", self->scanner.current_block);
			   				self->scanner.current_block->e_len = g_utf8_strlen(gtk_text_iter_get_slice(
			   				&self->scanner.current_block->e_start,&self->scanner.current_block->e_end),-1);

			   				self->scanner.current_block->mark_end = mark2;
			   				gtk_text_buffer_apply_tag_by_name (buf, "_block_", &(self->scanner.current_block->b_start), &ita);
				  		}	/* it means we already have mark here  - do not remove it! */
		    		}		    		
		   		 bf_textview_block_end (self, self->scanner.current_block->def, 
		    				&self->scanner.current_block->b_start, &self->scanner.current_block->b_end, &its, &ita, mark, mark2);	/* EMIT SIGNAL */
		     		 if (!g_queue_is_empty (&(self->scanner.block_stack)))	bb = (TBfBlock *) g_queue_peek_head (&(self->scanner.block_stack));
		     		else	bb = NULL;
		   	}
		   	else {
		     		bf_textview_block_misplaced_end (self, 
		       		(BfLangBlock *) g_hash_table_lookup (self->current_lang->blocks_id, &currstate), &its, &ita);	/*EMIT SIGNAL */
		   	}
		   if (bb)
		     self->current_context = bb->def;
		   else
		     self->current_context = NULL;
		   if (self->current_context == NULL || self->current_context->scanned)
		     self->scanner.state = BFTV_STATE_NORMAL;
		   self->scanner.current_block = bb;
		   break;
	     }
	       gtk_text_iter_forward_char (&ita);
	       c = gtk_text_iter_get_char (&ita);
	       if ((gint) c < 256)
		  			currstate = self->current_lang->scan_be_table[(gint) c][currstate];
	       else
		  			currstate = 0;
	    } /* while */
	    if (block_found) {
	       currstate = 0;
	       its = ita;
	       continue;
	    } else { ita=previt; c=gtk_text_iter_get_char (&ita); currstate=0; }
	}
 } /* BLOCKs */	

      /* TOKENS */
    if (self->scanner.state != BFTV_STATE_DONT_SCAN && (gint) c < 256 ) {
    regexp_found = FALSE;	
    
	 if (self->current_lang->case_sensitive)
	    currstate = self->current_lang->scan_table[(gint) c][currstate];
	 else
	    currstate = self->current_lang->scan_table[(gint) g_unichar_toupper (c)][currstate];
	 if (currstate > 0) {
	    self->scanner.state = BFTV_STATE_TOKEN;
	    if (currstate > BFTV_TOKEN_IDS) {
	    	 
	       self->scanner.current_token.def =
		  (BfLangToken *) g_hash_table_lookup (self->current_lang->tokens, &currstate);
	       if ( self->scanner.current_token.def->context == self->current_context || self->scanner.current_token.def->context == NULL) {	/* only in proper context */
			  self->scanner.current_token.itstart = its;
			  self->scanner.current_token.itend = ita;
			  { /* emit only if true token found */
			  		GtkTextIter ait1=its,ait2=ita;
			  		gboolean bok=FALSE,eok=FALSE;
			  		gunichar c;
			  		if ( gtk_text_iter_starts_line(&ait1) ) 
			  		  bok = TRUE;
			  		else {
			  			gtk_text_iter_backward_char(&ait1);
			  			c = gtk_text_iter_get_char (&ait1);
			  			if (!g_unichar_isalnum(c) && c!='_')
			  				bok=TRUE;
			  		}	
			  		if ( gtk_text_iter_ends_line(&ait2) ) 
			  		  eok = TRUE;
			  		else {
			  			/*gtk_text_iter_forward_char(&ait2);*/
			  			c = gtk_text_iter_get_char (&ait2);
			  			if (!g_unichar_isalnum(c) && c!='_')
			  				eok=TRUE;
			  		}	
			  		if ( bok && eok )
				  		bf_textview_token (self, self->scanner.current_token.def, &its, &ita);	/* EMIT SIGNAL */
			  }	
			  if ( self->scanner.current_token.def->regexp ) 
			  		regexp_found = TRUE;
	       }
	       its = ita;
	       gtk_text_iter_forward_char (&its);
	       currstate = 0;
	       self->scanner.state = BFTV_STATE_NORMAL;
	    }			/* has token */
	 }
	 else {
	    its = ita;
	    gtk_text_iter_forward_char (&its);
	    self->scanner.state = BFTV_STATE_NORMAL;
	 }
      }				/* DONT_SCAN */
      else {
	 its = ita;
	 gtk_text_iter_forward_char (&its);
      }
      if ( !regexp_found ) 
	      gtk_text_iter_forward_char (&ita);
   }				/* while */
   
	if ( self->current_lang->indent_blocks) {
		for(iaux=0;iaux<self->current_lang->line_indent->len;iaux++) {
			if ( iaux+1 < self->current_lang->line_indent->len ) {
				if ( g_array_index(self->current_lang->line_indent,gint,iaux) < g_array_index(self->current_lang->line_indent,gint,iaux+1) )
					iaux = bftv_prepare_indent_blocks(self,iaux);
			}
		}
   }
}

/**
*	bf_textview_scan_lines:
*	@self:  BfTextView widget 
* @start: start line
* @end: end line
*
*	Scan buffer from line "start" to line "end". 
*/
void bf_textview_scan_lines (BfTextView * self, gint start, gint end)
{
   GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self));
   GtkTextIter its, ite, itp;
   GtkTextMark *mark = NULL;
   gchar *pomstr;
   TBfBlock *bb;
   gint i;

   g_return_if_fail (self != NULL);
   g_return_if_fail (BF_IS_TEXTVIEW (self));
   g_return_if_fail (buf != NULL);
   gtk_text_buffer_get_iter_at_line (buf, &its, start);
   gtk_text_buffer_get_iter_at_line (buf, &ite, end);
   gtk_text_iter_forward_to_line_end (&ite);
   /* expand to blocks  begin and end */
   if (self->current_lang->scan_blocks) {
      itp = its;
      for (i = start; i <= end; i++) {
	 gtk_text_iter_set_line (&itp, i);
	 mark = bftv_get_first_block_at_line (&itp,TRUE);
	 if (mark) {
	    pomstr = g_object_get_data (G_OBJECT (mark), "type");
	    if (strcmp (pomstr, "block_end") == 0) {
	       bb = (TBfBlock *) g_object_get_data (G_OBJECT (mark), "pointer");
	       gtk_text_buffer_get_iter_at_mark (buf, &its, bb->mark_begin);
	    }
	    break;
	 }
      }				/* for */
      itp = ite;
      for (i = end; i >= start; i--) {
	 gtk_text_iter_set_line (&itp, i);
	 mark = bftv_get_last_block_at_line (&itp);
	 if (mark) {
	    pomstr = g_object_get_data (G_OBJECT (mark), "type");
	    if (strcmp (pomstr, "block_begin") == 0) {
	       bb = (TBfBlock *) g_object_get_data (G_OBJECT (mark), "pointer");
	       gtk_text_buffer_get_iter_at_mark (buf, &ite, bb->mark_end);
	    }
	    break;
	 }
      }				/* for */
   }
   bf_textview_scan_area (self, &its, &ite);
}


/* -----------------------  LANGUAGE CONFIGURATION -------------------- */

static gboolean bftv_xml_bool (xmlChar * text)
{
   if (text == NULL)
      return FALSE;
   if (xmlStrcmp (text, (const xmlChar *) "true") == 0 || xmlStrcmp (text, (const xmlChar *) "TRUE") == 0 || xmlStrcmp (text, (const xmlChar *) "1") == 0) {
      return TRUE;
   }
   return FALSE;
}


static gpointer bftv_find_regexp_hash (GList * lst, gchar * key)
{
   GList *pom = g_list_first (lst);
   gpointer ptr;
   while (pom) {
      ptr = g_hash_table_lookup ((GHashTable *) lst->data, key);
      if (ptr)
	 return ptr;
      pom = g_list_next (pom);
   }
   return NULL;
}

static void bftv_insert_regexp_hash (GList * lst, gchar * key, gpointer value, gboolean guarded)
{
   GList *pom = g_list_first (lst);
   gpointer ptr;
   while (pom) {
      ptr = g_hash_table_lookup ((GHashTable *) pom->data, key);
      if (guarded) {
	 if (!ptr) {
	    g_hash_table_insert ((GHashTable *) pom->data, g_strdup (key), value);
	 }
      }
      else {
	 g_hash_table_insert ((GHashTable *) pom->data, g_strdup (key), value);
      }
      pom = g_list_next (pom);
   }
}



static void bftv_put_into_block_dfa (BfLangConfig * cfg, BfLangBlock * b)
{
   gint i, size, j, k, m;
   gchar *ptr, *inp, *inp2, *inp3;
   gpointer p;
   GList *regexp_tables = NULL, *pom_regexp_tables = NULL, *as_tables = NULL;
   GList *lst = NULL;   
   GHashTable *curr_table = NULL, *pom_table = NULL;


b->tabid = cfg->blocknum;
cfg->blocknum++;
   
inp2 = g_strdup (b->begin);
inp = inp2;
size = g_utf8_strlen (b->begin, -1);
i = 0;
j = 0;
if (b->regexp)
   regexp_tables = g_list_append (regexp_tables, cfg->block_begin_dfa);
else
   curr_table = cfg->block_begin_dfa;


while (i < size) {
	if ( !b->regexp) {
			ptr = g_strdup_printf ("%d%c", (int) j, *inp);
		   p = NULL;
		   p = g_hash_table_lookup (curr_table, ptr);
		   if (p == NULL) {
				 pom_table = g_hash_table_new (g_str_hash, g_str_equal);
				 cfg->bb_tabnum++;
				 g_hash_table_insert (pom_table, "number", g_strdup_printf ("%d", cfg->bb_tabnum));
				 g_hash_table_replace (curr_table, g_strdup (ptr), pom_table);
				 curr_table = pom_table;
		   }
		   else {
				 curr_table = (GHashTable *) p;
		   }
		   if (i == size - 1) {
				 g_hash_table_replace (curr_table, g_strdup ("_final_"), b);
		   }
		   g_free (ptr);
   } /* not regexp begin */
   else { /*regexp begin */
			 if (*inp == '[') {
				    k = i + 1;
				    inp3 = inp + 1;
				    lst = NULL;
				    while (k < size && *inp3 != ']') {
				       if (g_unichar_isalnum (*inp3) && *(inp3 + 1) == '-' && g_unichar_isalnum (*(inp3 + 2))) {
					  for (m = (gint) (*inp3); m <= (gint) (*(inp3 + 2)); m++) {
					     ptr = g_strdup_printf ("%d%c", j, m);
					     if (!g_list_find (lst, ptr))
						lst = g_list_append (lst, ptr);
					  }
					  inp3 = inp3 + 2;
					  k += 2;
				       }
				       else {
					  			lst = g_list_append (lst, g_strdup_printf ("%d%c", j, *inp3));
				       }
				       inp3++;
				       k++;
				    }
				    i = k;
				    inp = inp3;
				 }
				 else {
				    lst = NULL;
				    lst = g_list_append (lst, g_strdup_printf ("%d%c", j, *inp));
				 }
				 lst = g_list_first (lst);
				 pom_regexp_tables = NULL;
				 pom_table = NULL;
				 as_tables = regexp_tables;
				 if (*(inp + 1) == '?' || *(inp + 1) == '*') {
				    GList *lst2 = g_list_first (regexp_tables);
				    while (lst2) {
				       if (!g_list_find (pom_regexp_tables, lst2->data)) {
					  			pom_regexp_tables = g_list_append (pom_regexp_tables, lst2->data);
				       }
				       lst2 = g_list_next (lst2);
				    }
				    if (*(inp + 1) == '?') {
				       i++;
				       inp++;
				    }
				 }			/* optional modifier */
				 while (lst) {
				    p = bftv_find_regexp_hash (regexp_tables, lst->data);
				    if (p == NULL) {	/* table not found */
				    	 if (!pom_table) {
							  pom_table = g_hash_table_new (g_str_hash, g_str_equal);
							  cfg->bb_tabnum++;
							  g_hash_table_insert (pom_table, "number", g_strdup_printf ("%d", cfg->bb_tabnum));
							  pom_regexp_tables = g_list_append (pom_regexp_tables, pom_table);
							  cfg->dfa_tables = g_list_append (cfg->dfa_tables, pom_table);
				       }
				       bftv_insert_regexp_hash (regexp_tables, lst->data, pom_table, TRUE);
				    }
				    else {		/* just follow */
				       pom_regexp_tables = g_list_append (pom_regexp_tables, p);
				    }
				    if (*(inp + 1) == '+' || *(inp + 1) == '*') {
				       bftv_insert_regexp_hash (pom_regexp_tables, lst->data, pom_table, TRUE);
				    }			/* one/zero or more modifier */
			
				    lst = g_list_next (lst);
				 }
				 if (i == size - 1 || (i == size - 2 && *(inp + 1) == '+')
				     || (i == size - 2 && *(inp + 1) == '*'))
				    bftv_insert_regexp_hash (pom_regexp_tables, "_final_", b, TRUE);
			
				 if (*(inp + 1) == '+' || *(inp + 1) == '*') {
				    i++;
				    inp++;
				 }
				 if (regexp_tables)
				    g_list_free (regexp_tables);
				 regexp_tables = pom_regexp_tables;
   
   } /* regexp begin */
   if (i<size) inp++;
   i++;
   j++;
}				/* while  for begin */
g_free (inp2);

inp2 = g_strdup (b->end);
inp = inp2;
regexp_tables = NULL;pom_regexp_tables = NULL;as_tables = NULL;
lst = NULL; pom_table = NULL;
if (b->regexp)
   regexp_tables = g_list_append (regexp_tables, cfg->block_end_dfa);
else
   curr_table = cfg->block_end_dfa;

size = g_utf8_strlen (b->end, -1);
i = 0;
j = 0;
while (i < size) {
	if ( !b->regexp ) {
		   ptr = g_strdup_printf ("%d%c", (int) j, *inp);
		   p = NULL;
		   p = g_hash_table_lookup (curr_table, ptr);
		   if (p == NULL) {
				 pom_table = g_hash_table_new (g_str_hash, g_str_equal);
				 cfg->be_tabnum++;
				 g_hash_table_insert (pom_table, "number", g_strdup_printf ("%d", cfg->be_tabnum));
				 g_hash_table_replace (curr_table, g_strdup (ptr), pom_table);
				 curr_table = pom_table;
		   }
		   else {
				 curr_table = (GHashTable *) p;
		   }
		   if (i == size - 1) {
				 g_hash_table_replace (curr_table, g_strdup ("_final_"), b);
		   }
		   g_free (ptr);
   } /* not regexp end */
   else /* regexp end */
   {
			 if (*inp == '[') {
				    k = i + 1;
				    inp3 = inp + 1;
				    lst = NULL;
				    while (k < size && *inp3 != ']') {
				       if (g_unichar_isalnum (*inp3) && *(inp3 + 1) == '-' && g_unichar_isalnum (*(inp3 + 2))) {
					  for (m = (gint) (*inp3); m <= (gint) (*(inp3 + 2)); m++) {
					     ptr = g_strdup_printf ("%d%c", j, m);
					     if (!g_list_find (lst, ptr))
						lst = g_list_append (lst, ptr);
					  }
					  inp3 = inp3 + 2;
					  k += 2;
				       }
				       else {
					  			lst = g_list_append (lst, g_strdup_printf ("%d%c", j, *inp3));
				       }
				       inp3++;
				       k++;
				    }
				    i = k;
				    inp = inp3;
				 }
				 else {
				    lst = NULL;
				    lst = g_list_append (lst, g_strdup_printf ("%d%c", j, *inp));
				 }
				 lst = g_list_first (lst);
				 pom_regexp_tables = NULL;
				 pom_table = NULL;
				 as_tables = regexp_tables;
				 if (*(inp + 1) == '?' || *(inp + 1) == '*') {
				    GList *lst2 = g_list_first (regexp_tables);
				    while (lst2) {
				       if (!g_list_find (pom_regexp_tables, lst2->data)) {
					  			pom_regexp_tables = g_list_append (pom_regexp_tables, lst2->data);
				       }
				       lst2 = g_list_next (lst2);
				    }
				    if (*(inp + 1) == '?') {
				       i++;
				       inp++;
				    }
				 }			/* optional modifier */
				 while (lst) {
				    p = bftv_find_regexp_hash (regexp_tables, lst->data);
				    if (p == NULL) {	/* table not found */
				    	 if (!pom_table) {
							  pom_table = g_hash_table_new (g_str_hash, g_str_equal);
							  cfg->be_tabnum++;
							  g_hash_table_insert (pom_table, "number", g_strdup_printf ("%d", cfg->be_tabnum));
							  pom_regexp_tables = g_list_append (pom_regexp_tables, pom_table);
							  cfg->dfa_tables = g_list_append (cfg->dfa_tables, pom_table);
				       }
				       bftv_insert_regexp_hash (regexp_tables, lst->data, pom_table, TRUE);
				    }
				    else {		/* just follow */
				       pom_regexp_tables = g_list_append (pom_regexp_tables, p);
				    }
				    if (*(inp + 1) == '+' || *(inp + 1) == '*') {
				       bftv_insert_regexp_hash (pom_regexp_tables, lst->data, pom_table, TRUE);
				    }			/* one/zero or more modifier */
			
				    lst = g_list_next (lst);
				 }
				 if (i == size - 1 || (i == size - 2 && *(inp + 1) == '+')
				     || (i == size - 2 && *(inp + 1) == '*'))
				    bftv_insert_regexp_hash (pom_regexp_tables, "_final_", b, TRUE);
			
				 if (*(inp + 1) == '+' || *(inp + 1) == '*') {
				    i++;
				    inp++;
				 }
				 if (regexp_tables)
				    g_list_free (regexp_tables);
				 regexp_tables = pom_regexp_tables;
   
   } /* regexp end */
   inp++;
   i++;
   j++;
}				/* while  for end*/
g_free (inp2);
}



static void bftv_put_into_dfa (GHashTable * dfa, BfLangToken * token, BfLangConfig * cfg)
{
   gint i, size, j, k, m;
   gchar *ptr, *inp, *inp2, *inp3;
   gpointer p;
   GHashTable *curr_table = NULL, *pom_table = NULL;
   GList *regexp_tables = NULL, *pom_regexp_tables = NULL, *as_tables = NULL;
   GList *lst = NULL;

   token->id = cfg->tokennum;
   cfg->tokennum++;
   if (cfg->case_sensitive)
      inp2 = g_strdup (token->text);
   else
      inp2 = g_utf8_strup (token->text, -1);
   inp = inp2;
   if (token->regexp)
      regexp_tables = g_list_append (regexp_tables, dfa);
   else
      curr_table = dfa;

   size = g_utf8_strlen (token->text, -1);
   i = 0;
   j = 0;
   while (i < size) {
    if (!token->regexp) {
	 ptr = g_strdup_printf ("%d%c", j, *inp);
	 p = NULL;
	 p = g_hash_table_lookup (curr_table, ptr);
	 if (p == NULL) {	/* table not found */
	    pom_table = g_hash_table_new (g_str_hash, g_str_equal);
	    cfg->tabnum++;
	    g_hash_table_insert (pom_table, "number", g_strdup_printf ("%d", cfg->tabnum));
	    g_hash_table_replace (curr_table, g_strdup (ptr), pom_table);
	    curr_table = pom_table;
	    cfg->dfa_tables = g_list_append (cfg->dfa_tables, pom_table);
	 }
	 else {			/* just follow */
	    curr_table = (GHashTable *) p;
	 }
		 if (i == size - 1)
		    g_hash_table_replace (curr_table, g_strdup ("_final_"), token);
		 g_free (ptr);
     }
     else {			/* REGULAR EXPR */
				 
				 if (*inp == '[') {
				    k = i + 1;
				    inp3 = inp + 1;
				    lst = NULL;
				    while (k < size && *inp3 != ']') {
				       if (g_unichar_isalnum (*inp3) && *(inp3 + 1) == '-' && g_unichar_isalnum (*(inp3 + 2))) {
					  for (m = (gint) (*inp3); m <= (gint) (*(inp3 + 2)); m++) {
					     ptr = g_strdup_printf ("%d%c", j, m);
					     if (!g_list_find (lst, ptr))
						lst = g_list_append (lst, ptr);
					  }
					  inp3 = inp3 + 2;
					  k += 2;
				       }
				       else {
				       		if (*inp3=='\\') { inp3++;k++;}
					  			lst = g_list_append (lst, g_strdup_printf ("%d%c", j, *inp3));
				       }
				       inp3++;
				       k++;
				    }
				    i = k;
				    inp = inp3;
				 }
				 else {
				    lst = NULL;
				    if (*inp=='\\') { inp++;i++;}
				    lst = g_list_append (lst, g_strdup_printf ("%d%c", j, *inp));
				 }
				 lst = g_list_first (lst);
				 pom_regexp_tables = NULL;
				 pom_table = NULL;
				 as_tables = regexp_tables;
				 if (*(inp + 1) == '?' || *(inp + 1) == '*') {
				    GList *lst2 = g_list_first (regexp_tables);
				    while (lst2) {
				       if (!g_list_find (pom_regexp_tables, lst2->data)) {
					  		pom_regexp_tables = g_list_append (pom_regexp_tables, lst2->data);
				       }
				       lst2 = g_list_next (lst2);
				    }
				    if (*(inp + 1) == '?') {
				       i++;
				       inp++;
				    }
				 }			/* optional modifier */
				 while (lst) {
				    p = bftv_find_regexp_hash (regexp_tables, lst->data);
				    if (p == NULL) {	/* table not found */
				       if (!pom_table) {
							  pom_table = g_hash_table_new (g_str_hash, g_str_equal);
							  cfg->tabnum++;
							  g_hash_table_insert (pom_table, "number", g_strdup_printf ("%d", cfg->tabnum));
							  pom_regexp_tables = g_list_append (pom_regexp_tables, pom_table);
							  cfg->dfa_tables = g_list_append (cfg->dfa_tables, pom_table);
				       }
				       bftv_insert_regexp_hash (regexp_tables, lst->data, pom_table, TRUE);
				    }
				    else {		/* just follow */
				       pom_regexp_tables = g_list_append (pom_regexp_tables, p);
				    }
				    if (*(inp + 1) == '+' || *(inp + 1) == '*') {
				       bftv_insert_regexp_hash (pom_regexp_tables, lst->data, pom_table, TRUE);
				    }			/* one/zero or more modifier */
			
				    lst = g_list_next (lst);
				 }
				 if (i == size - 1 || (i == size - 2 && *(inp + 1) == '+')
				     || (i == size - 2 && *(inp + 1) == '*'))
				    bftv_insert_regexp_hash (pom_regexp_tables, "_final_", token, TRUE);
			
				 if (*(inp + 1) == '+' || *(inp + 1) == '*') {
				    i++;
				    inp++;
				 }
				 if (regexp_tables)
				    g_list_free (regexp_tables);
				 regexp_tables = pom_regexp_tables;
      }				/* end of REGEXP */
      if (i < size) inp++;
      i++;
      j++;
   }				/* while */
   g_free (inp2);
}


#define S_TABLE_ERR		-1
#define S_TABLE_FINAL	-2

typedef struct {
   gint size;
   GHashTable *bkp;
   GHashTable *current;
} Tbf_cnt;


typedef struct {
   gint **table;
   gint number;
   GHashTable *current, *bkp;
} Tbf_bb;

static void bftv_dfa_build (gpointer key, gpointer value, gpointer data)
{
   Tbf_bb *bb = (Tbf_bb *) data;
   gchar *pomstr = NULL;
   gint len, idx, prev;
   BfLangToken *t;

   if (strcmp ((gchar *) key, "_final_") != 0
       && strcmp ((gchar *) key, "number") != 0 && strcmp ((gchar *) key, "made") != 0) {
      pomstr = (gchar *) key;
      len = g_utf8_strlen (pomstr, -1);
      idx = (gint) pomstr[len - 1];
      pomstr = g_hash_table_lookup ((GHashTable *) value, "number");
      bb->table[idx][bb->number] = atoi (pomstr);
      prev = bb->number;
      bb->number = atoi (pomstr);
      if ((bb->current != (GHashTable *) value)
	  && !g_hash_table_lookup ((GHashTable *) value, "made")) {
			 bb->bkp = bb->current;
			 bb->current = (GHashTable *) value;
			 g_hash_table_foreach ((GHashTable *) value, bftv_dfa_build, bb);
			 g_hash_table_insert ((GHashTable *) value, "made", "1");
			 bb->current = bb->bkp;
      }
      bb->number = prev;
   }
   else if (strcmp ((gchar *) key, "_final_") == 0) {
      t = (BfLangToken *) value;
      for (len = 0; len < 256; len++)
			 if (bb->table[len][bb->number] == 0 /*&& !g_ascii_isalnum((gchar)len) && (gchar)len!='_'*/)
			    bb->table[len][bb->number] = t->id;
       
   }

}


static gint **bftv_make_scan_table (GHashTable * dfa, BfLangConfig *cfg)
{
   gint **table;
   gint i, j;
   Tbf_bb *bb = g_new0 (Tbf_bb, 1);
   gchar *pomstr;

   table = g_new0 (gint *, 256);
   for (i = 0; i < 256; i++) {
      table[i] = g_new0 (gint, cfg->tabnum + 1);
      for (j = 0; j < cfg->tabnum + 1; j++)
	 table[i][j] = 0;
   }
   bb->table = table;
   pomstr = g_hash_table_lookup (dfa, "number");
   bb->number = atoi (pomstr);
   bb->current = dfa;
   bb->bkp = NULL;
   g_hash_table_foreach (dfa, bftv_dfa_build, bb);
   g_free (bb);
   return table;
}

static void bftv_dfa_build_b (gpointer key, gpointer value, gpointer data)
{
   Tbf_bb *bb = (Tbf_bb *) data;
   gchar *pomstr = NULL;
   gint len, idx, prev;
   BfLangBlock *t;

   if (strcmp ((gchar *) key, "_final_") != 0
       && strcmp ((gchar *) key, "number") != 0 && strcmp ((gchar *) key, "made") != 0) {
      pomstr = (gchar *) key;
      len = g_utf8_strlen (pomstr, -1);
      idx = (gint) pomstr[len - 1];
      pomstr = g_hash_table_lookup ((GHashTable *) value, "number");
      bb->table[idx][bb->number] = atoi (pomstr);
      prev = bb->number;
      bb->number = atoi (pomstr);
      if ((bb->current != (GHashTable *) value)
	  && !g_hash_table_lookup ((GHashTable *) value, "made")) {
			 bb->bkp = bb->current;
			 bb->current = (GHashTable *) value;
			 g_hash_table_foreach ((GHashTable *) value, bftv_dfa_build_b, bb);
			 g_hash_table_insert ((GHashTable *) value, "made", "1");
			 bb->current = bb->bkp;
      }
      bb->number = prev;
   }
   else if (strcmp ((gchar *) key, "_final_") == 0) {
      t = (BfLangBlock *) value;
      for (len = 0; len < 256; len++)
			 if (bb->table[len][bb->number] == 0)
			    bb->table[len][bb->number] = t->tabid;
   }

/*
   Tbf_bb *bb = (Tbf_bb *) data;
   gchar *pomstr = NULL;
   gint len, idx, prev;
   BfLangBlock *t;
	g_print("build %s %p\n",key);
   if (strcmp ((gchar *) key, "_final_") != 0 && strcmp ((gchar *) key, "number") != 0 
       && strcmp ((gchar *) key, "made") != 0) {
      pomstr = (gchar *) key;
      len = g_utf8_strlen (pomstr, -1);
      idx = (gint) pomstr[len - 1];
      pomstr = g_hash_table_lookup ((GHashTable *) value, "number");
      bb->table[idx][bb->number] = atoi (pomstr);
      prev = bb->number;
      bb->number = atoi (pomstr);
      g_hash_table_foreach ((GHashTable *) value, bftv_dfa_build_b, bb);
      g_hash_table_insert ((GHashTable *) value, "made", "1");
      bb->number = prev;
   }
   else if (strcmp ((gchar *) key, "_final_") == 0) {
      t = (BfLangBlock *) value;
      for (len = 0; len < 256; len++)
	 if (bb->table[len][bb->number] == 0)
	    bb->table[len][bb->number] = t->tabid;
   } 
*/   
}

#define BT_BEGIN 0
#define BT_END 1


static gint **bftv_make_bscan_table (GHashTable * dfa, gint btype, BfLangConfig *cfg)
{
   gint **table = NULL;
   gint size = 0, i, j;
   Tbf_bb *bb = g_new0 (Tbf_bb, 1);
   gchar *pomstr;

   switch (btype) {
   case BT_BEGIN:
      size = cfg->bb_tabnum + 1;
      break;
   case BT_END:
      size = cfg->be_tabnum + 1;
      break;
   }
   table = g_new0 (gint *, 256);
   for (i = 0; i < 256; i++) {
      table[i] = g_new0 (gint, size);
      for (j = 0; j < size; j++)
	 table[i][j] = 0;
   }
   bb->table = table;
   pomstr = g_hash_table_lookup (dfa, "number");
   bb->number = atoi (pomstr);
   g_hash_table_foreach (dfa, bftv_dfa_build_b, bb);
   g_free (bb);
   return table;
}

static gint **bftv_make_tscan_table ()
{
   gint **table = NULL;
   gint i, j;
   table = g_new0 (gint *, BFTV_UTF8_RANGE);
   for (i = 0; i < BFTV_UTF8_RANGE; i++) {
      table[i] = g_new0 (gint, BFTV_TAG_STATE_NUM);
      for (j = 0; j < BFTV_TAG_STATE_NUM; j++)
	 table[i][j] = 0;
   }
   table[(gint) '<'][0] = 1;
   for (i = (gint) 'a'; i <= (gint) 'z'; i++)
      table[i][1] = 2;
   for (i = (gint) 'A'; i <= (gint) 'Z'; i++)
      table[i][1] = 2;
   table[(gint) '/'][1] = 3;
   for (i = (gint) 'a'; i <= (gint) 'z'; i++)
      table[i][2] = 2;
   for (i = (gint) 'A'; i <= (gint) 'Z'; i++)
      table[i][2] = 2;
   table[(gint) '>'][2] = 4;
   table[(gint) ' '][2] = 5;
   table[(gint) '\t'][2] = 5;
   table[(gint) '\n'][2] = 5;
   table[(gint) '/'][2] = 10;
   for (i = (gint) 'a'; i <= (gint) 'z'; i++)
      table[i][3] = 3;
   for (i = (gint) 'A'; i <= (gint) 'Z'; i++)
      table[i][3] = 3;
   table[(gint) '>'][3] = 12;
   for (i = 0; i < BFTV_UTF8_RANGE; i++)
      table[i][4] = BFTV_TAG_IDS_BEGIN_COMPLETED;
   for (i = (gint) 'a'; i <= (gint) 'z'; i++)
      table[i][5] = 6;
   for (i = (gint) 'A'; i <= (gint) 'Z'; i++)
      table[i][5] = 6;
   table[(gint) '>'][5] = 4;
   table[(gint) ' '][5] = 5;
   table[(gint) '\t'][5] = 5;
   table[(gint) '\n'][5] = 5;
   table[(gint) '/'][5] = 10;
   for (i = (gint) 'a'; i <= (gint) 'z'; i++)
      table[i][6] = 6;
   for (i = (gint) 'A'; i <= (gint) 'Z'; i++)
      table[i][6] = 6;
   for (i = (gint) '0'; i <= (gint) '9'; i++)
      table[i][6] = 6;
   table[(gint)'-'][6] = 6;   
   table[(gint)'_'][6] = 6; 
   table[(gint) '='][6] = 7;
   for (i = (gint) 'a'; i <= (gint) 'z'; i++)
      table[i][7] = 13;
   for (i = (gint) 'A'; i <= (gint) 'Z'; i++)
      table[i][7] = 13;
   for (i = (gint) '0'; i <= (gint) '9'; i++)
      table[i][7] = 13;
      
   table[(gint) '"'][7] = 8;
   for (i = 0; i < BFTV_UTF8_RANGE; i++)
      table[i][8] = 8;
   table[(gint) '"'][8] = 9;
/*   table[(gint) '>'][8] = 0;*/
   table[(gint) ' '][9] = 14;
   table[(gint) '\t'][9] = 14;
   table[(gint) '\n'][9] = 14;
   table[(gint) '/'][9] = 10;
   table[(gint) '>'][9] = 4;
   table[(gint) '>'][10] = 11;
   for (i = 0; i < BFTV_UTF8_RANGE; i++)
      table[i][11] = BFTV_TAG_IDS_EMPTY;
   for (i = 0; i < BFTV_UTF8_RANGE; i++)
      table[i][12] = BFTV_TAG_IDS_END;
   for (i = (gint) 'a'; i <= (gint) 'z'; i++)
      table[i][13] = 13;
   for (i = (gint) 'A'; i <= (gint) 'Z'; i++)
      table[i][13] = 13;
   for (i = (gint) '0'; i <= (gint) '9'; i++)
      table[i][13] = 13;
      
   table[(gint) '>'][13] = 15;
   table[(gint) ' '][13] = 15;
   table[(gint) '\t'][13] = 15;
   table[(gint) '\n'][13] = 15;
   for (i = (gint) 'a'; i <= (gint) 'z'; i++)
      table[i][14] = 6;
   for (i = (gint) 'A'; i <= (gint) 'Z'; i++)
      table[i][14] = 6;
   table[(gint) '>'][14] = 4;
   table[(gint) '/'][14] = 10;
   table[(gint) ' '][14] = 14;
   table[(gint) '\t'][14] = 14;
   table[(gint) '\n'][14] = 14;
   for (i = (gint) 'a'; i <= (gint) 'z'; i++)
      table[i][15] = 6;
   for (i = (gint) 'A'; i <= (gint) 'Z'; i++)
      table[i][15] = 6;
   table[(gint) '>'][15] = 4;
   table[(gint) '/'][15] = 10;
   table[(gint) ' '][15] = 14;
   table[(gint) '\t'][15] = 14;
   table[(gint) '\n'][15] = 14;

   return table;
}


static void bftv_free_dfa (BfLangConfig * cfg)
{
   GList *lst = g_list_first (cfg->dfa_tables);
   while (lst) {
      g_hash_table_destroy ((GHashTable *) lst->data);
      lst = g_list_next (lst);
   }
   g_hash_table_destroy (cfg->dfa);
}


static BfLangConfig *bftv_load_config (gchar *filename, gboolean reuse, BfLangConfig *rcfg)
{
   xmlDocPtr doc;
   xmlNodePtr cur, cur2;
   xmlChar *tmps, *tmps2 = NULL, *tmps3 = NULL;
   BfLangConfig *cfg = NULL;
   gpointer ptr = NULL;
   gint i;
   gboolean options_set = FALSE;

   g_return_val_if_fail (filename != NULL, (BfLangConfig *) 0);
   xmlLineNumbersDefault (1);
  	doc = xmlReadFile(filename,"UTF-8",XML_PARSE_RECOVER | XML_PARSE_NOENT);
   cur = xmlDocGetRootElement (doc);
   if ( reuse && rcfg!=NULL )
   	options_set = TRUE;
   
   
   if (xmlStrcmp (cur->name, (const xmlChar *) "bflang") == 0) {
   if ( !reuse || rcfg==NULL) {
      cfg = g_new0 (BfLangConfig, 1);
      cfg->name = xmlGetProp (cur, (const xmlChar *) "name");
      cfg->description = xmlGetProp (cur, (const xmlChar *) "description");
      cfg->blocks = g_hash_table_new (g_str_hash, g_str_equal);
      cfg->blocks_id = g_hash_table_new (g_int_hash, g_int_equal);
      cfg->tokens = g_hash_table_new (g_int_hash, g_int_equal);
      cfg->groups = g_hash_table_new (g_int_hash, g_int_equal);
      cfg->dfa = g_hash_table_new (g_str_hash, g_str_equal);
      cfg->dfa_tables = NULL;
		cfg->restricted_tags = g_hash_table_new (g_int_hash, g_int_equal);
		cfg->line_indent = g_array_new(TRUE,TRUE,sizeof(gint));
		
      cfg->tabnum = 0;
      g_hash_table_insert (cfg->dfa, "number", "0");
      
      cfg->tokennum = BFTV_TOKEN_IDS + 1;
      cfg->bb_tabnum = 0;
      cfg->be_tabnum = 0;
      cfg->blocknum = BFTV_BLOCK_IDS + 1;
      
      cfg->block_begin_dfa = g_hash_table_new (g_str_hash, g_str_equal);
      g_hash_table_insert (cfg->block_begin_dfa, "number", "0");
      cfg->block_end_dfa = g_hash_table_new (g_str_hash, g_str_equal);
      g_hash_table_insert (cfg->block_end_dfa, "number", "0");
      for (i = 0; i < 256; i++) {
       cfg->as_triggers[i] = 0;
       cfg->escapes[i] = 0;
      } 
    } else cfg = rcfg;
      
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
	 if (xmlStrcmp (cur->name, (const xmlChar *) "options") == 0) {
	 	 
	    cur2 = cur->xmlChildrenNode;
	    
	    while (cur2 != NULL) {
	       if (xmlStrcmp (cur2->name, (const xmlChar *) "option") == 0 && !options_set) {
				  tmps = xmlGetProp (cur2, (const xmlChar *) "name");
				  tmps2 = xmlNodeListGetString (doc, cur2->xmlChildrenNode, 1);
				  if (tmps) {
				     if (xmlStrcmp (tmps, (const xmlChar *) "case-sensitive") == 0 && !options_set)
						cfg->case_sensitive = bftv_xml_bool (tmps2);
				     else if (xmlStrcmp (tmps, (const xmlChar *) "scan-markup-tags") == 0 && !options_set)
						cfg->scan_tags = bftv_xml_bool (tmps2);
				     else if (xmlStrcmp (tmps, (const xmlChar *) "scan-blocks") == 0 && !options_set)
						cfg->scan_blocks = bftv_xml_bool (tmps2);
				     else if (xmlStrcmp (tmps, (const xmlChar *) "restricted-tags-only") == 0 && !options_set)
						cfg->restricted_tags_only = bftv_xml_bool (tmps2);						
				     else if (xmlStrcmp (tmps, (const xmlChar *) "indent-blocks") == 0 && !options_set)
						cfg->indent_blocks = bftv_xml_bool (tmps2);												
				     else if (xmlStrcmp (tmps, (const xmlChar *) "extensions") == 0 && !options_set)
						cfg->extensions = g_strdup (tmps2);
				     else if (xmlStrcmp (tmps, (const xmlChar *) "auto-scan-triggers") == 0 && !options_set) {
							gchar *p = tmps2;
							i = 0;
							while (i < g_utf8_strlen (tmps2, -1)) {
							   cfg->as_triggers[(gint) * p] = 1;
							   i++;
							   p = g_utf8_next_char (p);
							}
					  }
				     else if (xmlStrcmp (tmps, (const xmlChar *) "escape-characters") == 0 && !options_set) {
							gchar *p = tmps2;
							i = 0;
							while (i < g_utf8_strlen (tmps2, -1)) {
							   cfg->escapes[(gint) * p] = 1;
							   i++;
							   p = g_utf8_next_char (p);
							}
					 }			 
				 	 if (tmps) xmlFree (tmps);
			  }
			  if (tmps2)
				     xmlFree (tmps2);
			 }
	  		 cur2 = cur2->next;
	  }			/* end of cur2 */
	   options_set = TRUE;		
	 }
	 else if (xmlStrcmp (cur->name, (const xmlChar *) "block-group") == 0) {	/* blocks */
	    tmps3 = xmlGetProp (cur, (const xmlChar *) "id");
	    g_hash_table_replace(cfg->groups,g_strdup(tmps3),"b");
	    cur2 = cur->xmlChildrenNode;
	    while (cur2 != NULL) {
	       if (xmlStrcmp (cur2->name, (const xmlChar *) "block") == 0) {
				  tmps = xmlGetProp (cur2, (const xmlChar *) "id");
				  if (tmps) {
				     ptr = g_hash_table_lookup (cfg->blocks, tmps);
				     if (!ptr) {
							BfLangBlock *b = g_new0 (BfLangBlock, 1);
							b->id = tmps;
							b->group = tmps3;
							b->begin = xmlGetProp (cur2, (const xmlChar *) "begin");
							b->end = xmlGetProp (cur2, (const xmlChar *) "end");
							tmps2 = xmlGetProp (cur2, (const xmlChar *) "scanned");
							b->scanned = bftv_xml_bool (tmps2);			
							if (tmps2)  xmlFree (tmps2);
							tmps2 = xmlGetProp (cur2, (const xmlChar *) "markup");
							b->markup = bftv_xml_bool (tmps2);
							if (tmps2) xmlFree (tmps2);
							tmps2 = xmlGetProp (cur2, (const xmlChar *) "foldable");
							b->foldable = bftv_xml_bool (tmps2);
							if (tmps2) xmlFree (tmps2);
							tmps2 = xmlGetProp (cur2, (const xmlChar *) "case");
							b->case_sensitive = bftv_xml_bool (tmps2);
							if (tmps2)  xmlFree (tmps2);
							tmps2 = xmlGetProp (cur2, (const xmlChar *) "regexp");
							b->regexp = bftv_xml_bool (tmps2);
							if (tmps2)  xmlFree (tmps2);
							
							g_hash_table_insert (cfg->blocks, tmps, b);
							bftv_put_into_block_dfa (cfg, b);
							g_hash_table_insert (cfg->blocks_id, &b->tabid, b);
			     }
			     else {
							g_warning ("Block named %s already exists!", tmps);
							xmlFree (tmps);
			     }
			  }
	       } /* block */
	       cur2 = cur2->next;
	    }
	 }
	 else if (xmlStrcmp (cur->name, (const xmlChar *) "token-group") == 0) {	/* tokens */
	    tmps3 = xmlGetProp (cur, (const xmlChar *) "id");
	    g_hash_table_insert(cfg->groups,g_strdup(tmps3),"t");
	    cur2 = cur->xmlChildrenNode;
	    while (cur2 != NULL) {
	       if (xmlStrcmp (cur2->name, (const xmlChar *) "token") == 0) {
					  tmps = xmlNodeListGetString (doc, cur2->xmlChildrenNode, 1);
					  ptr = g_hash_table_lookup (cfg->tokens, tmps);
					  if (!ptr) {
					     BfLangToken *t = g_new0 (BfLangToken, 1);
					     t->group = tmps3;
					     tmps2 = xmlGetProp (cur2, (const xmlChar *) "regexp");
					     t->regexp = bftv_xml_bool (tmps2);
					     if (tmps2)
						xmlFree (tmps2);
					     tmps2 = xmlGetProp (cur2, (const xmlChar *) "name");
					     if (tmps2)
						t->name = tmps2;
					     else
						t->name = tmps;
					     t->text = tmps;
					     tmps2 = xmlGetProp (cur2, (const xmlChar *) "context");
					     if (tmps2) {
						ptr = g_hash_table_lookup (cfg->blocks, tmps2);
						if (!ptr)
						   g_warning
						      ("Token (%s) context defined as %s but such a block does not exists.", t->text, tmps2);
								t->context = (BfLangBlock *) ptr;
								xmlFree (tmps2);
					     }
					     else
								t->context = NULL;
					     bftv_put_into_dfa (cfg->dfa, t, cfg);
					     g_hash_table_insert (cfg->tokens, &t->id, t);
		 	 }
		  	 else {
				        g_warning ("Token %s already exists.", tmps);
					     xmlFree (tmps);
		  	  }
	       }		/* token */
	       else if (xmlStrcmp (cur2->name, (const xmlChar *) "token-list") == 0) {
					  tmps3 = xmlGetProp (cur2, (const xmlChar *) "separator");
					  if (tmps3) {
					     tmps = xmlNodeListGetString (doc, cur2->xmlChildrenNode, 1);
					     gchar **arr = g_strsplit (tmps, tmps3, -1);
					     xmlFree (tmps3);
					     if (arr) {
						gint i = 0;
						tmps3 = xmlGetProp (cur, (const xmlChar *) "id");
						while (arr[i] != NULL) {
						   ptr = g_hash_table_lookup (cfg->tokens, arr[i]);
						   if (!ptr) {
						      BfLangToken *t = g_new0 (BfLangToken, 1);
						      t->group = tmps3;
						      tmps2 = xmlGetProp (cur2, (const xmlChar *) "regexp");
						      t->regexp = bftv_xml_bool (tmps2);
						      if (tmps2)
							 xmlFree (tmps2);
/*						      tmps2 = xmlGetProp (cur2, "name");
						      if (tmps2)
							 t->name = tmps2;
						      else
							   t->name = tmps;*/
						      t->text = g_strdup (arr[i]);
						      t->name = t->text;
						      tmps2 = xmlGetProp (cur2, (const xmlChar *) "context");
						      t->context = NULL;
						      if (tmps2) {
									 ptr = g_hash_table_lookup (cfg->blocks, tmps2);
									 if (!ptr)
									    g_warning("Token (%s) context defined as %s but such a block does not exists.",
											t->text, tmps2);
									 if (ptr)		
										 t->context = (BfLangBlock *) ptr;
									 xmlFree (tmps2);
						      }
						      bftv_put_into_dfa (cfg->dfa, t, cfg);
						      g_hash_table_insert (cfg->tokens, &t->id, t);
						   }
						   else {
						      g_warning ("Token %s already exists.", arr[i]);
						   }
						   i++;
						}
						g_strfreev (arr);
					     }
					     xmlFree (tmps);
					  }
       }		/* token-list */
	       cur2 = cur2->next;
	    }			/* while cur2 */
	 }
	 else if (xmlStrcmp (cur->name, (const xmlChar *) "block") == 0) { /* block without a group */
				  tmps = xmlGetProp (cur, (const xmlChar *) "id");
				  if (tmps) {
				     ptr = g_hash_table_lookup (cfg->blocks, tmps);
				     if (!ptr) {
							BfLangBlock *b = g_new0 (BfLangBlock, 1);
							b->id = tmps;
							b->group = NULL;
							b->begin = xmlGetProp (cur, (const xmlChar *) "begin");
							b->end = xmlGetProp (cur, (const xmlChar *) "end");
							tmps2 = xmlGetProp (cur, (const xmlChar *) "scanned");
							b->scanned = bftv_xml_bool (tmps2);			
							if (tmps2)  xmlFree (tmps2);
							tmps2 = xmlGetProp (cur, (const xmlChar *) "markup");
							b->markup = bftv_xml_bool (tmps2);
							if (tmps2) xmlFree (tmps2);
							tmps2 = xmlGetProp (cur, (const xmlChar *) "foldable");
							b->foldable = bftv_xml_bool (tmps2);
							if (tmps2) xmlFree (tmps2);
							tmps2 = xmlGetProp (cur, (const xmlChar *) "case");
							b->case_sensitive = bftv_xml_bool (tmps2);
							if (tmps2)  xmlFree (tmps2);
							tmps2 = xmlGetProp (cur, (const xmlChar *) "regexp");
							b->regexp = bftv_xml_bool (tmps2);
							if (tmps2)  xmlFree (tmps2);
							
							g_hash_table_insert (cfg->blocks, tmps, b);
							bftv_put_into_block_dfa (cfg, b);
							g_hash_table_insert (cfg->blocks_id, &b->tabid, b);
			     }
			     else {
							g_warning ("Block named %s already exists!", tmps);
							xmlFree (tmps);
			     }
			  }
	 } /* block without a group */
	 else if (xmlStrcmp (cur->name, (const xmlChar *) "token") == 0) {
					  tmps = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
					  ptr = g_hash_table_lookup (cfg->tokens, tmps);
					  if (!ptr) {
					     BfLangToken *t = g_new0 (BfLangToken, 1);
					     t->group = NULL;
					     tmps2 = xmlGetProp (cur, (const xmlChar *) "regexp");
					     t->regexp = bftv_xml_bool (tmps2);
					     if (tmps2)
						xmlFree (tmps2);
					     tmps2 = xmlGetProp (cur, (const xmlChar *) "name");
					     if (tmps2)
						t->name = tmps2;
					     else
						t->name = tmps;
					     t->text = tmps;
					     tmps2 = xmlGetProp (cur, (const xmlChar *) "context");
					     if (tmps2) {
						ptr = g_hash_table_lookup (cfg->blocks, tmps2);
						if (!ptr)
						   g_warning
						      ("Token (%s) context defined as %s but such a block does not exists.", t->text, tmps2);
								t->context = (BfLangBlock *) ptr;
								xmlFree (tmps2);
					     }
					     else
								t->context = NULL;
					     bftv_put_into_dfa (cfg->dfa, t, cfg);
					     g_hash_table_insert (cfg->tokens, &t->id, t);
		 	 }
		  	 else {
				        g_warning ("Token %s already exists.", tmps);
					     xmlFree (tmps);
		  	  }
	  }		/* token  without a group */	
      else if (xmlStrcmp (cur->name, (const xmlChar *) "token-list") == 0) { /* token list without a group */
					  tmps3 = xmlGetProp (cur, (const xmlChar *) "separator");
					  if (tmps3) {
					     tmps = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
					     gchar **arr = g_strsplit (tmps, tmps3, -1);
					     xmlFree (tmps3);
					     if (arr) {
						gint i = 0;
						while (arr[i] != NULL) {
						   ptr = g_hash_table_lookup (cfg->tokens, arr[i]);
						   if (!ptr) {
						      BfLangToken *t = g_new0 (BfLangToken, 1);
						      t->group = NULL;
						      tmps2 = xmlGetProp (cur, (const xmlChar *) "regexp");
						      t->regexp = bftv_xml_bool (tmps2);
						      if (tmps2)
							 xmlFree (tmps2);
/*						      tmps2 = xmlGetProp (cur, "name");
						      if (tmps2)
							 t->name = tmps2;
						      else
							   t->name = tmps;*/
						      t->text = g_strdup (arr[i]);
						      t->name = t->text;
						      tmps2 = xmlGetProp (cur, (const xmlChar *) "context");
						      t->context = NULL;
						      if (tmps2) {
									 ptr = g_hash_table_lookup (cfg->blocks, tmps2);
									 if (!ptr)
									    g_warning("Token (%s) context defined as %s but such a block does not exists.",
											t->text, tmps2);
									 if (ptr)		
										 t->context = (BfLangBlock *) ptr;
									 xmlFree (tmps2);
						      }
						      bftv_put_into_dfa (cfg->dfa, t, cfg);
						      g_hash_table_insert (cfg->tokens, &t->id, t);
						   }
						   else {
						      g_warning ("Token %s already exists.", arr[i]);
						   }
						   i++;
						}
						g_strfreev (arr);
					     }
					     xmlFree (tmps);
					  }
     }		/* token-list  without a group */	   
  	 else if (xmlStrcmp (cur->name, (const xmlChar *) "restricted-tags") == 0) {	/* restricted tags */
	    cur2 = cur->xmlChildrenNode;
	    while (cur2 != NULL) {
			if (xmlStrcmp (cur2->name, (const xmlChar *) "tag") == 0) {	
				tmps = xmlGetProp(cur2, (const xmlChar *) "name");
				if ( tmps ) {
					GHashTable *h = g_hash_table_new(g_str_hash,g_str_equal);
					gchar **arr;
					gint i=0;
					g_hash_table_insert(cfg->restricted_tags,g_strdup(tmps),h);
					xmlFree(tmps);
					tmps = xmlGetProp(cur2, (const xmlChar *) "attributes");
					arr = g_strsplit (tmps, ",", -1);
					while ( arr[i] != NULL ) {
						g_hash_table_insert(h,g_strdup(arr[i]),"1");
						i++;
					}
					xmlFree(tmps);
					g_strfreev (arr);
				}
			}
	    	cur2 = cur2->next;
		 } /* while cur2 */     	 	
  	 } /* restricted tags */
	 
	 
	 cur = cur->next;
      }				/* while cur */
   }
   if (cfg->indent_blocks) {
      	cfg->iblock = g_new0(BfLangBlock,1);
      	cfg->iblock->id = "Indent block";
      	cfg->iblock->begin = cfg->iblock->end = g_strdup(" ");
      	cfg->iblock->group = NULL;		
      	cfg->iblock->tabid = cfg->blocknum;
      	cfg->blocknum++;
			g_hash_table_insert (cfg->blocks, cfg->iblock->id, cfg->iblock);
			g_hash_table_insert (cfg->blocks_id, &cfg->iblock->tabid, cfg->iblock);      	
      }
   
   
   if (doc)
      xmlFreeDoc (doc);
   return cfg;
}

static void bftv_make_config_tables(BfLangConfig *cfg) {
/*	gint i,j;*/

	if (!cfg) return;
   cfg->scan_table = bftv_make_scan_table (cfg->dfa,cfg);
   cfg->scan_bb_table = bftv_make_bscan_table (cfg->block_begin_dfa, BT_BEGIN,cfg);
   cfg->scan_be_table = bftv_make_bscan_table (cfg->block_end_dfa, BT_END,cfg);
   if (cfg->scan_tags)
      cfg->scan_tag_table = bftv_make_tscan_table ();
   bftv_free_dfa (cfg);
/*  	g_print("st   ");   
   for(j=32;j<100;j++)
   	g_print("%c  ",j);
   g_print("\n");	
   for(i=0;i<cfg->tabnum+1;i++) {
   	g_print("%.2d   ",i);   
   	for(j=32;j<100;j++)
   		g_print("%d  ",cfg->scan_table[j][i]);
   	g_print("\n");	
   }*/
}

static gboolean bftv_free_config_del_block (gpointer key, gpointer value, gpointer data)
{
   BfLangBlock *b = (BfLangBlock *) value;
   xmlFree (b->id);
   xmlFree (b->begin);
   xmlFree (b->end);
   /* group is not freed!!!! */
   g_free (b);
   return TRUE;
}

static gboolean bftv_free_config_del_token (gpointer key, gpointer value, gpointer data)
{
   BfLangToken *b = (BfLangToken *) value;
   xmlFree (b->text);
   /* group is not freed!!!! */
   g_free (b);
   return TRUE;
}


static void bftv_free_config (BfLangConfig * cfg)
{
   if (!cfg)
      return;
   xmlFree (cfg->name);
   xmlFree (cfg->description);
   g_free (cfg->extensions);
   g_hash_table_foreach_remove (cfg->blocks, bftv_free_config_del_block, NULL);
   g_hash_table_foreach_remove (cfg->tokens, bftv_free_config_del_token, NULL);
   g_free (cfg);
}


/**
*	bf_textview_add_language:
*	@self:  BfTextView widget 
* @langname: name of language
* @filename: configuration file
*
*	Add language to BfTextView language pool. This widget can load configurations for multiple languages. Each language
* is configured by separated file and has different scanner. When you want to switch to specific language - use #bf_textview_set_language.
*
* Returns: FALSE if error (for example cannot load config or language with that name already exists)
*/
gboolean bf_textview_add_language (BfTextView * self, gchar * langname, gchar * filename)
{
   BfLangConfig *cfg = NULL;
   gpointer ptr;

   g_return_val_if_fail (self != NULL, FALSE);
   g_return_val_if_fail (BF_IS_TEXTVIEW (self), FALSE);
   ptr = g_hash_table_lookup (self->languages, langname);
   if (ptr)
      return FALSE;
   cfg = bftv_load_config (filename,FALSE,NULL);
   bftv_make_config_tables(cfg);
   if (cfg) {
      g_hash_table_insert (self->languages, langname, cfg);
      return TRUE;
   }
   return FALSE;
}

/**
*	bf_textview_remove_language:
*	@self:  BfTextView widget 
* @langname: name of language
*
*	Remove language from the pool.
*
*/
void bf_textview_remove_language (BfTextView * self, gchar * langname)
{
   gpointer ptr = NULL;

   g_return_if_fail (self != NULL);
   g_return_if_fail (BF_IS_TEXTVIEW (self));
   ptr = g_hash_table_lookup (self->languages, langname);
   if (ptr) {
      if (self->current_lang == ptr)
	 self->current_lang = NULL;
      bftv_free_config ((BfLangConfig *) ptr);
      g_hash_table_remove (self->languages, langname);
   }
}

/**
*	bf_textview_set_language:
*	@self:  BfTextView widget 
* @langname: name of language
*
*	Set current language. The scanner for that language will be used in "scan" methods.
*
*/
void bf_textview_set_language (BfTextView * self, gchar * langname)
{
   gpointer ptr = NULL;

   g_return_if_fail (self != NULL);
   g_return_if_fail (BF_IS_TEXTVIEW (self));
   ptr = g_hash_table_lookup (self->languages, langname);
   if (ptr) {
      self->current_lang = (BfLangConfig *) ptr;
   }
}

void bf_textview_set_language_ptr (BfTextView * self, BfLangConfig *cfg) {
	self->current_lang = cfg;
	if (cfg)
		bf_textview_scan(self);
}

/* -------------------------  BLOCK FOLDING ----------------------- */

/**
*	bf_textview_fold_blocks:
*	@self:  BfTextView widget 
* @fold: if TRUE blocks should be folded, FALSE means unfold.
*
*	Fold/unfold all blocks in buffer.
*
*/
void bf_textview_fold_blocks (BfTextView * self, gboolean fold)
{
   GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self));
   GtkTextIter its, ite;

   g_return_if_fail (self != NULL);
   g_return_if_fail (BF_IS_TEXTVIEW (self));
   gtk_text_buffer_get_start_iter (buf, &its);
   gtk_text_buffer_get_end_iter (buf, &ite);
   bf_textview_fold_blocks_area (self, &its, &ite, fold);
}


/**
*	bf_textview_fold_blocks_area:
*	@self:  BfTextView widget 
*	@start: textbuffer iterator denoting area start
*	@end: textbuffer iterator denoting area end
* @fold: if TRUE blocks should be folded, FALSE means unfold.
*
*	Fold/unfold all blocks in buffer area specified by "start" and "end" iterators.
*
*/
void bf_textview_fold_blocks_area (BfTextView * self, GtkTextIter * start, GtkTextIter * end, gboolean fold)
{
   GtkTextMark *mark;
   GtkTextIter it;
   gint i;
   gchar *pomstr;

   g_return_if_fail (self != NULL);
   g_return_if_fail (BF_IS_TEXTVIEW (self));
   it = *start;
   for (i = gtk_text_iter_get_line (start); i <= gtk_text_iter_get_line (end); i++) {
      gtk_text_iter_set_line (&it, i);
      mark = bftv_get_first_block_at_line (&it,TRUE);
      if (mark) {
	 pomstr = g_object_get_data (G_OBJECT (mark), "type");
	 if (strcmp (pomstr, "block_begin") == 0) {
	    if (fold)
	       g_object_set_data (G_OBJECT (mark), "folded", "false");
	    else
	       g_object_set_data (G_OBJECT (mark), "folded", "true");
	    bftv_fold (mark,FALSE);
	 }
      }
   }
}

/**
*	bf_textview_fold_blocks_lines:
*	@self:  BfTextView widget 
*	@start: beginning line
*	@end: ending line
* @fold: if TRUE blocks should be folded, FALSE means unfold.
*
*	Fold/unfold all blocks in buffer area specified by "start" and "end" lines.
*
*/
void bf_textview_fold_blocks_lines (BfTextView * self, gint start, gint end, gboolean fold)
{
   GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self));
   GtkTextIter its, ite;

   g_return_if_fail (self != NULL);
   g_return_if_fail (BF_IS_TEXTVIEW (self));
   gtk_text_buffer_get_iter_at_line (buf, &its, start);
   gtk_text_buffer_get_iter_at_line (buf, &ite, end);
   gtk_text_iter_forward_to_line_end (&ite);
   bf_textview_fold_blocks_area (self, &its, &ite, fold);
}

/* --------------------------------  CREATING THE VIEW ------------------------- */

/**
*	bf_textview_new:
*
*	Create new BfTextView widget.
*
* Returns: new BfTextView widget.
*/
GtkWidget *bf_textview_new (void)
{
	BfTextView *o = GET_NEW;

   g_signal_connect (G_OBJECT (o), "expose-event", G_CALLBACK (bf_textview_expose_cb), NULL);
   g_signal_connect_after (G_OBJECT
			   (gtk_text_view_get_buffer (GTK_TEXT_VIEW (o))),
			   "insert-text", G_CALLBACK (bf_textview_insert_text_cb), o);
/*    g_signal_connect (G_OBJECT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (o))),
		     "delete-range", G_CALLBACK (bf_textview_delete_range_before_cb), o);*/
   g_signal_connect_after (G_OBJECT
			   (gtk_text_view_get_buffer (GTK_TEXT_VIEW (o))),
			   "delete-range", G_CALLBACK (bf_textview_delete_range_cb), o);
   g_signal_connect (G_OBJECT (o), "button-press-event", G_CALLBACK (bf_textview_mouse_cb), NULL);
   o->folded_tag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(gtk_text_view_get_buffer
				  (GTK_TEXT_VIEW (o))),"_folded_");
	if ( !o->folded_tag )			  
	   o->folded_tag =
   	   gtk_text_buffer_create_tag (gtk_text_view_get_buffer
					  (GTK_TEXT_VIEW (o)), "_folded_", "editable", FALSE, "invisible", TRUE, NULL);
   o->fold_header_tag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(gtk_text_view_get_buffer
				  (GTK_TEXT_VIEW (o))),"_fold_header_");
	if ( !o->fold_header_tag )				  
	   o->fold_header_tag =
   	   gtk_text_buffer_create_tag (gtk_text_view_get_buffer
					  (GTK_TEXT_VIEW (o)), "_fold_header_",
					  "editable", FALSE, "background-gdk", &o->bkg_color, NULL);
   o->block_tag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(gtk_text_view_get_buffer
				  (GTK_TEXT_VIEW (o))),"_block_");
	if ( !o->block_tag )				  
	   o->block_tag =
   	   gtk_text_buffer_create_tag (gtk_text_view_get_buffer
					  (GTK_TEXT_VIEW (o)), "_block_", "background-gdk", &o->bkg_color, NULL);
   gtk_widget_modify_base (GTK_WIDGET (o), GTK_WIDGET (o)->state, &o->bkg_color);
   gtk_widget_modify_text (GTK_WIDGET (o), GTK_WIDGET (o)->state, &o->fg_color);

   return (GtkWidget *) o;
}

/**
*	bf_textview_new_with_buffer:
*	@buffer
*
*	Create new BfTextView widget setting given buffer.
*
* Returns: new BfTextView widget.
*/
GtkWidget *bf_textview_new_with_buffer (GtkTextBuffer * buffer) 
{
	BfTextView *o = GET_NEW;
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(o),buffer);
	GdkColor col;
	
   g_signal_connect (G_OBJECT (o), "expose-event", G_CALLBACK (bf_textview_expose_cb), NULL);
   g_signal_connect_after (G_OBJECT
			   (gtk_text_view_get_buffer (GTK_TEXT_VIEW (o))),
			   "insert-text", G_CALLBACK (bf_textview_insert_text_cb), o);
/*   g_signal_connect (G_OBJECT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (o))),
		     "delete-range", G_CALLBACK (bf_textview_delete_range_before_cb), o);*/
   g_signal_connect_after (G_OBJECT
			   (gtk_text_view_get_buffer (GTK_TEXT_VIEW (o))),
			   "delete-range", G_CALLBACK (bf_textview_delete_range_cb), o);
   g_signal_connect (G_OBJECT (o), "button-press-event", G_CALLBACK (bf_textview_mouse_cb), NULL);
   o->folded_tag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(gtk_text_view_get_buffer
				  (GTK_TEXT_VIEW (o))),"_folded_");   
	if ( !o->folded_tag )			  
	   o->folded_tag =
   	   gtk_text_buffer_create_tag (gtk_text_view_get_buffer
					  (GTK_TEXT_VIEW (o)), "_folded_", "editable", FALSE, "invisible", TRUE, NULL);
	gdk_color_parse ("#F7F3D2", &col);
   o->fold_header_tag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(gtk_text_view_get_buffer
				  (GTK_TEXT_VIEW (o))),"_fold_header_");
	if ( !o->fold_header_tag )
	   o->fold_header_tag =
   	   gtk_text_buffer_create_tag (gtk_text_view_get_buffer
					  (GTK_TEXT_VIEW (o)), "_fold_header_",
					  "editable", FALSE, "background-gdk", &col, NULL);
   o->block_tag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(gtk_text_view_get_buffer
				  (GTK_TEXT_VIEW (o))),"_block_");					  
	if ( !o->block_tag )			  
	   o->block_tag =
   	   gtk_text_buffer_create_tag (gtk_text_view_get_buffer
					  (GTK_TEXT_VIEW (o)), "_block_", "background-gdk", &o->bkg_color, NULL);
   gtk_widget_modify_base (GTK_WIDGET (o), GTK_WIDGET (o)->state, &o->bkg_color);
   gtk_widget_modify_text (GTK_WIDGET (o), GTK_WIDGET (o)->state, &o->fg_color);   
	
	return (GtkWidget*)o;
}


/* --------------------------------- EVENTS ---------------------------------------*/

static gboolean bf_textview_expose_cb (GtkWidget * widget, GdkEventExpose * event, gpointer doc)
{
   GdkWindow *left_win;
   GdkRectangle rect;
   GtkTextIter l_start, l_end, it, it2;
   GtkTextMark *block_mark;
   TBfBlock *blockptr;
   gint l_top1, numlines, w, l_top2, i, w2;
   PangoLayout *l;
   gchar *pomstr = NULL;
   gint pt_lines, pt_sym, pt_blocks;
   GdkGC *gc;
   gpointer aux;
   GdkColor cwhite, cblack;
   GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
   gboolean block_folded;

   left_win = gtk_text_view_get_window (GTK_TEXT_VIEW (widget), GTK_TEXT_WINDOW_LEFT);
   if (left_win != event->window)
      return FALSE;
   if (!BF_TEXTVIEW (widget)->show_lines && !BF_TEXTVIEW (widget)->show_symbols && !BF_TEXTVIEW (widget)->show_blocks)
      return FALSE;

   gc = gdk_gc_new (GDK_DRAWABLE (left_win));
   gdk_color_parse ("#FFFFFF", &cwhite);
   gdk_color_parse ("#000000", &cblack);
   gtk_text_view_get_visible_rect (GTK_TEXT_VIEW (widget), &rect);
   gtk_text_view_get_line_at_y (GTK_TEXT_VIEW (widget), &l_start, rect.y, &l_top1);
   gtk_text_view_get_line_at_y (GTK_TEXT_VIEW (widget), &l_end, rect.y + rect.height, &l_top2);
   l = gtk_widget_create_pango_layout (widget, "");
   if (BF_TEXTVIEW (widget)->show_lines) {
      numlines = gtk_text_buffer_get_line_count (gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget)));
      pomstr = g_strdup_printf ("%d", MAX (99, numlines));
      pango_layout_set_text (l, pomstr, -1);
      g_free (pomstr);
      pango_layout_get_pixel_size (l, &w, NULL);
      BF_TEXTVIEW (widget)->lw_size_lines = w + 4;
   }
   gtk_text_view_set_border_window_size (GTK_TEXT_VIEW (widget),
					 GTK_TEXT_WINDOW_LEFT,
					 BF_TEXTVIEW (widget)->lw_size_lines +
					 BF_TEXTVIEW (widget)->lw_size_blocks + BF_TEXTVIEW (widget)->lw_size_sym);
   pt_lines = 2;
   if (BF_TEXTVIEW (widget)->show_lines)
      pt_sym = BF_TEXTVIEW (widget)->lw_size_lines + 2;
   else
      pt_sym = 2;
   pt_blocks = 2;
   if (BF_TEXTVIEW (widget)->show_lines)
      pt_blocks = BF_TEXTVIEW (widget)->lw_size_lines + pt_blocks;
   if (BF_TEXTVIEW (widget)->show_symbols)
      pt_blocks = BF_TEXTVIEW (widget)->lw_size_sym + pt_blocks;


   it = l_start;
   for (i = gtk_text_iter_get_line (&l_start); i <= gtk_text_iter_get_line (&l_end); i++) {
      gtk_text_iter_set_line (&it, i);
      gtk_text_view_get_line_yrange (GTK_TEXT_VIEW (widget), &it, &w, NULL);
      gtk_text_view_buffer_to_window_coords (GTK_TEXT_VIEW (widget), GTK_TEXT_WINDOW_LEFT, 0, w, NULL, &w);

      if (BF_TEXTVIEW (widget)->show_lines) {	/* show line numbers */
	 if (!gtk_text_iter_has_tag (&it, BF_TEXTVIEW (widget)->folded_tag)) {
	    pomstr = g_strdup_printf ("<span foreground=\"#7E7979\">%d</span>", i + 1);
	    pango_layout_set_markup (l, pomstr, -1);
	    gtk_paint_layout (widget->style, left_win,
			      GTK_WIDGET_STATE (widget), FALSE, NULL, widget, NULL, pt_lines, w, l);
	    g_free (pomstr);
	 }
      }
      if (BF_TEXTVIEW (widget)->show_symbols) {	/* show symbols */
	 if (!gtk_text_iter_has_tag (&it, BF_TEXTVIEW (widget)->folded_tag)) {
	    pomstr = g_strdup_printf ("%d", i + 1);
	    aux = g_hash_table_lookup (BF_TEXTVIEW (widget)->symbol_lines, pomstr);
	    if (aux) {
	       gdk_draw_drawable (GDK_DRAWABLE (left_win), gc,
				  GDK_DRAWABLE (((BfTextViewSymbol *) aux)->pixmap), 0, 0, pt_sym, w + 2, 10, 10);
	    }
	    g_free (pomstr);
	 }
      }
      if (BF_TEXTVIEW (widget)->show_blocks) {	/* show block markers */
	 block_mark = bftv_get_first_block_at_line (&it,TRUE);
	 if (block_mark)
	    blockptr = g_object_get_data (G_OBJECT (block_mark), "pointer");
	 else
	    blockptr = NULL;
	 if (blockptr) {
	    pomstr = g_object_get_data (G_OBJECT (block_mark), "folded");
	    if (strcmp (pomstr, "true") == 0)
	       block_folded = TRUE;
	    else
	       block_folded = FALSE;
	    pomstr = g_object_get_data (G_OBJECT (block_mark), "type");
	 }
	 else {
	    block_folded = FALSE;
	 }

	 if (blockptr && !blockptr->single_line && blockptr->mark_end && blockptr->mark_begin
	 		&& !gtk_text_iter_has_tag (&it, BF_TEXTVIEW (widget)->folded_tag)    ) {
	    gtk_text_buffer_get_iter_at_mark (buf, &it2, blockptr->mark_end);
	    gtk_text_view_get_line_yrange (GTK_TEXT_VIEW (widget), &it2, &w2, NULL);
	    gtk_text_view_buffer_to_window_coords (GTK_TEXT_VIEW (widget), GTK_TEXT_WINDOW_LEFT, 0, w2, NULL, &w2);
	    if (strcmp (pomstr, "block_begin") == 0) {
	       gdk_gc_set_rgb_fg_color (gc, &cwhite);
	       gdk_draw_rectangle (GDK_DRAWABLE (left_win), gc, TRUE, pt_blocks, w + 2, 10, 10);
	       gdk_gc_set_rgb_fg_color (gc, &cblack);
	       gdk_draw_rectangle (GDK_DRAWABLE (left_win), gc, FALSE, pt_blocks, w + 2, 10, 10);
	     
	       if (block_folded) {
		        gdk_draw_line (GDK_DRAWABLE (left_win), gc, pt_blocks + 3, w + 7, pt_blocks + 7, w + 7);
		        gdk_draw_line (GDK_DRAWABLE (left_win), gc, pt_blocks + 5, w + 5, pt_blocks + 5, w + 9);
	       }
	       else
		        gdk_draw_line (GDK_DRAWABLE (left_win), gc, pt_blocks + 3, w + 7, pt_blocks + 7, w + 7);
	    }
	    else {		/* block end */
	       gdk_draw_line (GDK_DRAWABLE (left_win), gc, pt_blocks + 5, w, pt_blocks + 5, w + 6);
	       gdk_draw_line (GDK_DRAWABLE (left_win), gc, pt_blocks + 5, w + 6, pt_blocks + 10, w + 6);
	    }
	 }
	 else {			/* not block begin or end, but perhaps inside */
	    if (gtk_text_iter_has_tag (&it, BF_TEXTVIEW (widget)->block_tag) && 
	        !gtk_text_iter_has_tag (&it, BF_TEXTVIEW (widget)->folded_tag)) {
	       gdk_draw_line (GDK_DRAWABLE (left_win), gc, pt_blocks + 5, w, pt_blocks + 5, w + 16);
	    }
	 }
      }

   }				/* end of lines loop */

   g_object_unref (G_OBJECT (l));
   return TRUE;
}

static void
bf_textview_insert_text_cb (GtkTextBuffer * textbuffer, GtkTextIter * arg1, gchar * arg2, gint arg3, gpointer user_data)
{
   BfTextView *view = BF_TEXTVIEW (user_data);
   gint l1, l2, len;
   gboolean trigger = FALSE;
   gchar *p = arg2;

   if (!view->autoscan || !view->current_lang)
      return;
   len = 0;
   while (len < g_utf8_strlen (arg2, -1)) {
      if (view->current_lang->as_triggers[(gint) * p] == 1) {
	 trigger = TRUE;
	 break;
      }
      len++;
      p = g_utf8_next_char (p);
   }
   if (!trigger)
      return;
   if (view->autoscan_lines <= 0) {
      bf_textview_scan (view);
      gtk_widget_queue_draw (GTK_WIDGET (user_data));
   }
   else {
      l1 = l2 = gtk_text_iter_get_line (arg1);
      l1 -= view->autoscan_lines;
      l2 += view->autoscan_lines;
      if (l1 < 0)
	 l1 = 0;
      if (l2 >= gtk_text_buffer_get_line_count (textbuffer))
	 l2 = gtk_text_buffer_get_line_count (textbuffer) - 1;
      bf_textview_scan_lines (view, l1, l2);
      gtk_widget_queue_draw (GTK_WIDGET (user_data));
   }
}

/*static void bf_textview_changed_cb (GtkTextBuffer * textbuffer, gpointer user_data)
{
}*/

static void bftv_fold (GtkTextMark * mark, gboolean move_cursor)
{
   GtkTextBuffer *buf = gtk_text_mark_get_buffer (mark);
   gchar *pomstr = g_object_get_data (G_OBJECT (mark), "folded");
   gboolean f = FALSE;
   GtkTextIter it1, it2, it3, it4;
   TBfBlock *block = (TBfBlock *) g_object_get_data (G_OBJECT (mark), "pointer");
   gint i;

   if (!pomstr)
      return;
   if (block->single_line || !block->mark_end)
      return;
   if (strcmp (pomstr, "true") == 0) {
      g_object_set_data (G_OBJECT (block->mark_begin), "folded", "false");
      g_object_set_data (G_OBJECT (block->mark_end), "folded", "false");
      f = FALSE;
   }
   else {
      g_object_set_data (G_OBJECT (block->mark_begin), "folded", "true");
      g_object_set_data (G_OBJECT (block->mark_end), "folded", "true");
      f = TRUE;
   }
   gtk_text_buffer_get_iter_at_mark (buf, &it1, block->mark_begin);
   gtk_text_iter_set_line (&it1, gtk_text_iter_get_line (&it1));
   gtk_text_buffer_get_iter_at_mark (buf, &it2, block->mark_end);
   /*gtk_text_iter_forward_char (&it2);*/
   it3 = it1;
   gtk_text_iter_forward_to_line_end (&it3);
   if (f) {
   	if ( move_cursor ) {
	      it4 = it2;
   	   gtk_text_iter_forward_char (&it4);
   	   gtk_text_buffer_place_cursor (buf, &it4);
   	}   
      gtk_text_buffer_apply_tag_by_name (buf, "_folded_", &it3, &it2);
      gtk_text_buffer_apply_tag_by_name (buf, "_fold_header_", &it1, &it3);
      it4 = it2;
      for(i=0;i<g_utf8_strlen(block->def->end,-1);i++)
	      gtk_text_iter_forward_char (&it4);
	   gtk_text_buffer_apply_tag_by_name (buf, "_fold_header_", &it2, &it4);   
   }
   else {
      gtk_text_buffer_remove_tag_by_name (buf, "_fold_header_", &it1, &it3);
      gtk_text_buffer_remove_tag_by_name (buf, "_folded_", &it3, &it2);
      it4 = it2;
      for(i=0;i<g_utf8_strlen(block->def->end,-1);i++)
	      gtk_text_iter_forward_char (&it4);
	   gtk_text_buffer_remove_tag_by_name (buf, "_fold_header_", &it2, &it4);         
   }
}



static gboolean bf_textview_mouse_cb (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
   GdkWindow *win = gtk_text_view_get_window (GTK_TEXT_VIEW (widget), GTK_TEXT_WINDOW_LEFT);
   gint x, y;
   GtkTextIter it;
   GtkTextMark *block_mark = NULL;
   gint pt_lines,pt_sym,pt_blocks;

   if (win != event->button.window)
      return FALSE;
      
      pt_lines = 2;
   	if (BF_TEXTVIEW (widget)->show_lines)
      	pt_sym = BF_TEXTVIEW (widget)->lw_size_lines + 2;
   	else
      	pt_sym = 2;
   	pt_blocks = 2;
   	if (BF_TEXTVIEW (widget)->show_lines)
      	pt_blocks = BF_TEXTVIEW (widget)->lw_size_lines + pt_blocks;
	   if (BF_TEXTVIEW (widget)->show_symbols)
   	   pt_blocks = BF_TEXTVIEW (widget)->lw_size_sym + pt_blocks;
      
   if ( event->button.button == 1 ) {   
		if ( event->button.x >= pt_blocks ) {
		   gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (widget), GTK_TEXT_WINDOW_TEXT, 0, event->button.y, &x, &y);
   		gtk_text_view_get_line_at_y (GTK_TEXT_VIEW (widget), &it, y, &x);
   		block_mark = bftv_get_first_block_at_line (&it,TRUE);
   		if (block_mark)
   		   bftv_fold (block_mark,TRUE);
   		return TRUE;   
   	}	   
   }
   else if ( event->button.button == 3 ) {
   	if ( event->button.x >= pt_blocks ) {
	 		gtk_menu_popup(GTK_MENU(BF_TEXTVIEW(widget)->fold_menu),
	 											NULL,NULL,NULL,NULL,1,gtk_get_current_event_time()); 
   		return TRUE;
   	}
   }	  
   return FALSE;
}

static void bftv_delete_blocks_from_area (BfTextView *view, GtkTextIter * arg1, GtkTextIter * arg2 )
{
   GtkTextBuffer *textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
   GtkTextMark *mark = NULL;
   gboolean deleted = FALSE;
   GtkTextIter it, it2, it3;
   TBfBlock *bb;
   gchar *p, *p2;

   if (!view->current_lang || !view->current_lang->scan_blocks)
      return;
   it = *arg1;
   while (gtk_text_iter_compare (&it, arg2) <= 0) {
      mark = bftv_get_block_at_iter (&it);
      if (mark) {
	 p = g_object_get_data (G_OBJECT (mark), "type");
	 p2 = g_object_get_data (G_OBJECT (mark), "folded");
	 bb = (TBfBlock *) g_object_get_data (G_OBJECT (mark), "pointer");
	 if (strcmp (p2, "true") != 0) {
	    if (strcmp (p, "block_begin") == 0) {
	       gtk_text_buffer_get_iter_at_mark (textbuffer, &it2, bb->mark_begin);
	       gtk_text_buffer_get_iter_at_mark (textbuffer, &it3, bb->mark_end);
	       gtk_text_iter_forward_char (&it3);
	       gtk_text_buffer_remove_tag_by_name (textbuffer, "_block_", &it2, &it3);
	       gtk_text_buffer_delete_mark (textbuffer, bb->mark_end);
	    }
	    else if (strcmp (p, "block_end") == 0) {
	       gtk_text_buffer_get_iter_at_mark (textbuffer, &it2, bb->mark_begin);
	       gtk_text_buffer_get_iter_at_mark (textbuffer, &it3, bb->mark_end);
	       gtk_text_iter_forward_char (&it3);
	       gtk_text_buffer_remove_tag_by_name (textbuffer, "_block_", &it2, &it3);
	       gtk_text_buffer_delete_mark (textbuffer, bb->mark_begin);
	       bb->mark_end = NULL;
	    }
	    gtk_text_buffer_delete_mark (textbuffer, mark);
	    deleted = TRUE;
	 }			/* folded */
      }
      gtk_text_iter_forward_char (&it);
      if (gtk_text_iter_is_end (&it))
	 break;
   }
}

static void
bf_textview_delete_range_cb (GtkTextBuffer * textbuffer, GtkTextIter * arg1, GtkTextIter * arg2, gpointer user_data)
{
   BfTextView *view = BF_TEXTVIEW (user_data);
   gint l1, l2;
   gboolean trigger = FALSE;
   gint len;
   gchar *p, *pomstr;


   if (view->autoscan && view->current_lang) {
      p = pomstr = gtk_text_buffer_get_text (textbuffer, arg1, arg2, TRUE);
      len = 0;
      while (len < g_utf8_strlen (pomstr, -1)) {
	 if (view->current_lang->as_triggers[(gint) * p] == 1) {
	    trigger = TRUE;
	    break;
	 }
	 len++;
	 p = g_utf8_next_char (p);
      }
      if (trigger) {
	 if (view->autoscan_lines <= 0) {
	    bf_textview_scan (view);
	    gtk_widget_queue_draw (GTK_WIDGET (user_data));
	 }
	 else {
	    l1 = l2 = gtk_text_iter_get_line (arg1);
	    l1 -= view->autoscan_lines;
	    l2 += view->autoscan_lines;
	    if (l1 < 0)
	       l1 = 0;
	    if (l2 >= gtk_text_buffer_get_line_count (textbuffer))
	       l2 = gtk_text_buffer_get_line_count (textbuffer) - 1;
	    bf_textview_scan_lines (view, l1, l2);
	    gtk_widget_queue_draw (GTK_WIDGET (user_data));
	 }
      }
   }
}

/* ----------------------------   MISC --------------------------------------------- */

/**
*	bf_textview_show_lines:
*	@self:  BfTextView widget 
* @show: if TRUE line numbers will be shown, FALSE means line numbers are hidden.
*
*	Show/hide line numbers at the left side of widget.
*
*/
void bf_textview_show_lines (BfTextView * self, gboolean show)
{
   PangoLayout *l = gtk_widget_create_pango_layout (GTK_WIDGET (self), "");
   gint numlines = gtk_text_buffer_get_line_count (gtk_text_view_get_buffer (GTK_TEXT_VIEW (self)));
   gchar *pomstr = g_strdup_printf ("%d", MAX (99, numlines));
   gint w;

   pango_layout_set_text (l, pomstr, -1);
   g_free (pomstr);
   pango_layout_get_pixel_size (l, &w, NULL);
   g_object_unref (G_OBJECT (l));

   self->show_lines = show;
   if (show) {
      self->lw_size_lines = w + 4;
   }
   else {
      self->lw_size_lines = 0;
   }
   gtk_text_view_set_border_window_size (GTK_TEXT_VIEW (self),
					 GTK_TEXT_WINDOW_LEFT,
					 self->lw_size_lines + self->lw_size_blocks + self->lw_size_sym);
}


/**
*	bf_textview_show_symbols:
*	@self:  BfTextView widget 
* @show: if TRUE symbols will be shown, FALSE means symbols are hidden.
*
*	Show/hide line symbols at the left side of widget.
*	Symbols are any other marks you want to show in BfTextView lines,
*	they can be for example: bookmarks
*
*/
void bf_textview_show_symbols (BfTextView * self, gboolean show)
{
   self->show_symbols = show;
   if (show) {
      self->lw_size_sym = 14;
   }
   else {
      self->lw_size_sym = 0;
   }
   gtk_text_view_set_border_window_size (GTK_TEXT_VIEW (self),
					 GTK_TEXT_WINDOW_LEFT,
					 self->lw_size_lines + self->lw_size_blocks + self->lw_size_sym);
   gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
*	bf_textview_add_symbol:
*	@self:  BfTextView widget 
* @name: name of the symbol - this symbol will be recognized by given name in symbol database.
* @pix:  pixmap to show for this symbol
*
*	Add symbol to BfTextView symbols database.
*
* Returns: TRUE if symbol added or FALSE if widget cannot add symbol (symbol of that name already exists).	
*/
gboolean bf_textview_add_symbol (BfTextView * self, gchar * name, GdkPixmap * pix)
{
   gpointer ptr = g_hash_table_lookup (self->symbols, name);
   if (ptr)
      return FALSE;
   BfTextViewSymbol *sym = g_new0 (BfTextViewSymbol, 1);
   sym->name = g_strdup (name);
   sym->pixmap = pix;
   g_hash_table_insert (self->symbols, name, sym);
   return TRUE;
}


static gboolean bftv_hash_remove (gpointer key, gpointer val, gpointer udata)
{
   BfTextViewSymbol *sym = (BfTextViewSymbol *) val;
   if (strcmp (sym->name, (gchar *) udata) == 0)
      return TRUE;
   return FALSE;
}

/**
*	bf_textview_remove_symbol:
*	@self:  BfTextView widget 
* @name: name of the symbol.
*
*	Remove symbol from BfTextView symbols database.
*	
*/
void bf_textview_remove_symbol (BfTextView * self, gchar * name)
{
   gpointer ptr = g_hash_table_lookup (self->symbols, name);
   if (ptr) {
      g_hash_table_foreach_remove (self->symbol_lines, bftv_hash_remove, name);
      g_hash_table_remove (self->symbols, name);
      g_free (((BfTextViewSymbol *) ptr)->name);
      g_free ((BfTextView *) ptr);
      gtk_widget_queue_draw (GTK_WIDGET (self));
   }
}

/**
*	bf_textview_set_symbol:
*	@self:  BfTextView widget 
* @name: name of the symbol.
* @line: line number in which symbol should be set
* @set: if TRUE - symbol should be shown, FALSE - hidden.
*
*	Set symbol visibility for a given line.
*	
*/
void bf_textview_set_symbol (BfTextView * self, gchar * name, gint line, gboolean set)
{
   gpointer ptr = g_hash_table_lookup (self->symbols, name);
   if (ptr) {
      if (set)
	 g_hash_table_replace (self->symbol_lines, g_strdup_printf ("%d", line), ptr);
      else {
	 g_hash_table_remove (self->symbol_lines, g_strdup_printf ("%d", line));
      }
      gtk_widget_queue_draw (GTK_WIDGET (self));
   }
}

/**
*	bf_textview_show_blocks:
*	@self:  BfTextView widget 
* @show: TRUE - blocks are shown, FALSE - hidden.
*
*	Switch blocks visibility on/off.
*	
*/
void bf_textview_show_blocks (BfTextView * self, gboolean show)
{
   self->show_blocks = show;
   if (show) {
      self->lw_size_blocks = 15;
   }
   else {
      self->lw_size_blocks = 0;
   }
   gtk_text_view_set_border_window_size (GTK_TEXT_VIEW (self),
					 GTK_TEXT_WINDOW_LEFT,
					 self->lw_size_lines + self->lw_size_blocks + self->lw_size_sym);
}

/**
*	bf_textview_set_autoscan:
*	@self:  BfTextView widget 
* @setauto: s.e.
*
*	Switch autoscanning feature. Autoscan is triggered by chars from "auto-scan-triggers" 
* option in configuration file.
*	
*/
void bf_textview_set_autoscan (BfTextView * self, gboolean setauto)
{
   self->autoscan = setauto;
}

/**
*	bf_textview_set_autoscan_lines:
*	@self:  BfTextView widget 
* @lines: - number of lines forward and backward to scan.
*
*	Set number of lines which should be scanned from current cursor position (backward and forward). 
* 	Value of  0 in @lines means "whole buffer".
*/
void bf_textview_set_autoscan_lines (BfTextView * self, gint lines)
{
   self->autoscan_lines = lines;
}

/**
*	bf_textview_color_blocks:
*	@self:  BfTextView widget 
* @color: - TRUE/FALSE for block coloring.
*
*	Set whether scanned blocks should have different background color than BfTextView widget. 
* 	
*/
void bf_textview_color_blocks (BfTextView * self, gboolean color)
{
   self->color_blocks = color;
   if (self->color_blocks) {
      g_object_set (G_OBJECT (self->block_tag), "background-gdk", &(self->block_color), NULL);
      g_object_set (G_OBJECT (self->fold_header_tag), "background-gdk", &(self->block_color), NULL);
   }
   else {
      g_object_set (G_OBJECT (self->block_tag), "background-gdk", &(self->bkg_color), NULL);
      g_object_set (G_OBJECT (self->fold_header_tag), "background-gdk", &(self->bkg_color), NULL);
   }

}

/**
*	bf_textview_set_blocks_color:
*	@self:  BfTextView widget 
* @color: - hexadecimal (HTML) representation of a color.
*
*	Set block background color. 
* 	
*/
void bf_textview_set_blocks_color (BfTextView * self, gchar * color)
{
   gdk_color_parse (color, &self->block_color);
   g_object_set (G_OBJECT (self->block_tag), "background-gdk", &(self->block_color), NULL);
   g_object_set (G_OBJECT (self->fold_header_tag), "background-gdk", &(self->block_color), NULL);
}

/**
*	bf_textview_set_bg_color:
*	@self:  BfTextView widget 
* @color: - hexadecimal (HTML) representation of a color.
*
*	Set BfTextView background color. 
* 	
*/
void bf_textview_set_bg_color (BfTextView * self, gchar * color)
{
   gdk_color_parse (color, &self->bkg_color);
   gtk_widget_modify_base (GTK_WIDGET (self), GTK_WIDGET (self)->state, &self->bkg_color);
}

/**
*	bf_textview_set_fg_color:
*	@self:  BfTextView widget 
* @color: - hexadecimal (HTML) representation of a color.
*
*	Set BfTextView foreground (text) color. 
* 	
*/
void bf_textview_set_fg_color (BfTextView * self, gchar * color)
{
   gdk_color_parse (color, &self->fg_color);
   gtk_widget_modify_text (GTK_WIDGET (self), GTK_WIDGET (self)->state, &self->fg_color);
}



/**
*	bf_textview_get_nearest_block:
*	@self:  BfTextView widget 
* @iter: - GtkTextIter indicating begin of a search.
*	@backward - if TRUE search is performed backward else forward
*	@mode - how much should we search - look at BF_GNB defines in header file
*	Find block nearest to a given iterator. 
* 	
* Returns: @TBfBlock structure or NULL if block not found.
*/
TBfBlock *bf_textview_get_nearest_block (BfTextView * self,GtkTextIter *iter, gboolean backward, gint mode, gboolean not_single) {
	GtkTextMark *mark = bftv_get_block_at_iter(iter);
	GtkTextIter it = *iter, endit;
	gchar *pomstr="";
	gpointer ptr;
	TBfBlock *bb=NULL;
	gboolean found=FALSE;
	
	if ( mark )
		pomstr = g_object_get_data (G_OBJECT (mark), "type");
	endit = it;	
	switch ( mode ) {
		case BF_GNB_LINE:
			if ( backward )
				gtk_text_iter_set_line(&endit,gtk_text_iter_get_line(iter));
			else
				gtk_text_iter_forward_to_line_end(&endit);			
		break;
		case BF_GNB_DOCUMENT:
			if (backward)
				gtk_text_iter_set_offset(&endit,0);
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

	if ( backward ) {	
		while ( !found && !gtk_text_iter_compare(&it,&endit)>=0 ) {
			gtk_text_iter_backward_char(&it);
			mark = bftv_get_block_at_iter(&it);
			if (mark) {
				ptr = g_object_get_data (G_OBJECT (mark), "pointer");
				if ( ptr ) {
					if ( not_single ) {
						bb = (TBfBlock*)ptr;
						if ( !bb->single_line ) found = TRUE;
					} else found=TRUE;
				}
			}	
		}
	} else { /* forward */
		while ( !found && !gtk_text_iter_compare(&it,&endit)<=0 ) {
			gtk_text_iter_forward_char(&it);
			mark = bftv_get_block_at_iter(&it);
			if (mark) {
				ptr = g_object_get_data (G_OBJECT (mark), "pointer");
				if ( ptr ) {
					if ( not_single ) {
						bb = (TBfBlock*)ptr;
						if ( !bb->single_line ) found = TRUE;
					} else found=TRUE;
				}
			}	
		}
	}

	
	if ( mark ) {
		bb = (TBfBlock*)g_object_get_data (G_OBJECT (mark), "pointer");
		bftv_update_iters(self,bb);
		return bb;
	}
	return NULL;
}


void bf_textview_register_hltag(BfTextView *self,GtkTextTag *tag) {
	self->hltags = g_list_append(self->hltags,tag);
}

void bf_textview_unregister_hltag(BfTextView *self,GtkTextTag *tag) {
	self->hltags = g_list_remove_all(self->hltags,tag);
}

void bf_textview_clear_hltags(BfTextView *self) {
	g_list_free(self->hltags);
	self->hltags = NULL;
}


/*
*	---------------------------------------------------------------------------------------------------------------
*										LANG MANAGER
*  ---------------------------------------------------------------------------------------------------------------
*/

BfLangManager *bf_lang_mgr_new() {
	BfLangManager *ret = g_new0(BfLangManager,1);
	ret->languages = g_hash_table_new(g_str_hash,g_str_equal);
	return ret;
}

gboolean bf_lang_mgr_load_config(BfLangManager *mgr,gchar *filename,gchar *filetype) {
	gchar *fname=NULL;
	gchar *userdir = g_strconcat(g_get_home_dir(), "/."PACKAGE"/", NULL);
	gboolean found;
	BfLangConfig *cfg=NULL;

	found = FALSE;
	fname = g_strconcat(PKGDATADIR,"/",filename,NULL);
	if (!g_file_test(fname,G_FILE_TEST_EXISTS)) {
			g_free(fname);
			fname = g_strconcat(userdir,filename,NULL);
			if ( g_file_test(fname,G_FILE_TEST_EXISTS) ) 
				found=TRUE;
		} else found=TRUE;		
	if (found) 
			cfg = bftv_load_config(fname,FALSE,cfg);
	else
			g_warning("BfLangManager: cannot load file %s",filename);					
	g_free(fname);	
	if ( cfg != NULL ) {
		bftv_make_config_tables(cfg);
		g_hash_table_replace(mgr->languages,filetype,cfg);
		return TRUE;
	}
	return FALSE;	
}

gboolean bf_lang_mgr_load_config_list(BfLangManager *mgr,GList *list,gchar *filetype) {
	GList *l = g_list_first(list);
	gchar *fname=NULL;
	gchar *userdir = g_strconcat(g_get_home_dir(), "/."PACKAGE"/", NULL);
	gboolean found;
	BfLangConfig *cfg=NULL;
	
	while ( l ) {
		found = FALSE;
		fname = g_strconcat(PKGDATADIR,"/",l->data,NULL);
		if (!g_file_test(fname,G_FILE_TEST_EXISTS)) {
			g_free(fname);
			fname = g_strconcat(userdir,l->data,NULL);
			if ( g_file_test(fname,G_FILE_TEST_EXISTS) ) 
				found=TRUE;
		} else found=TRUE;		
		if (found) 
			cfg = bftv_load_config(fname,TRUE,cfg);
		else
			g_warning("BfLangManager: cannot load file %s",(gchar*)l->data);					
		g_free(fname);	
		l = g_list_next(l);
	}
	if ( cfg != NULL ) {
		bftv_make_config_tables(cfg);
		g_hash_table_replace(mgr->languages,filetype,cfg);
		return TRUE;
	}
	return FALSE;
}

BfLangConfig *bf_lang_mgr_get_config(BfLangManager *mgr,gchar *filetype) {
	if ( !filetype ) return NULL;
	return g_hash_table_lookup(mgr->languages,filetype);
}



