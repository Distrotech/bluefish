/* Bluefish HTML Editor
 * autocomp.c - autocompletion system
 *
 * Copyright (C) 2006  Oskar Świda oskar.swida@gmail.com
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


#include "autocomp.h"
#include <gdk/gdkkeysyms.h>
#include <string.h>

static gint ac_paint(GtkWidget *win)
{
  if (!win) return FALSE;
  gtk_paint_flat_box (win->style, win->window,
                      GTK_STATE_NORMAL, GTK_SHADOW_ETCHED_IN,
                      NULL, win, "acomp",
                      0, 0, -1, -1);  
  gtk_paint_shadow (win->style, win->window,
                      GTK_STATE_NORMAL, GTK_SHADOW_ETCHED_IN,
                      NULL, win, "acomp",
                      0, 0, -1, -1);                        
  return FALSE;                      
}

static gboolean  ac_press(GtkWidget *widget,GdkEventKey *event,gpointer user_data)
{
	GtkDialog *dlg = GTK_DIALOG(user_data);
	switch ( event->keyval )
	{
		case GDK_Return:gtk_dialog_response(dlg,GTK_RESPONSE_OK);break;
		case GDK_Escape:gtk_dialog_response(dlg,GTK_RESPONSE_CANCEL);break;
	}
	return FALSE;	
}

/* Creates ac window  */
static GtkWidget *ac_create_window(GtkTreeView *tree, GtkListStore *store)
{
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	GtkWidget *scroll;
	GtkWidget *win;
	PangoFontDescription *fontdesc;

	gtk_tree_view_set_headers_visible(tree, FALSE);	
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", cell, "markup", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);	
   win = gtk_dialog_new ();
   gtk_widget_set_app_paintable (win, TRUE);
   gtk_window_set_resizable (GTK_WINDOW(win), FALSE);
   gtk_container_set_border_width (GTK_CONTAINER (win), 0); 
   g_signal_connect_swapped(GTK_WINDOW(win),"expose-event",G_CALLBACK(ac_paint),win);
   gtk_window_set_position (GTK_WINDOW(win), GTK_WIN_POS_MOUSE);
   gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(tree));
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(win)->vbox),scroll,TRUE,TRUE,0);
   gtk_widget_show_all(scroll);   
	gtk_window_set_decorated(GTK_WINDOW(win),FALSE);
   gtk_dialog_set_has_separator (GTK_DIALOG(win),FALSE);
   g_signal_connect(G_OBJECT(win),"key-press-event",G_CALLBACK(ac_press),win);
	fontdesc = pango_font_description_from_string("Sans 8");
	gtk_widget_modify_font(GTK_WIDGET(tree), fontdesc);
   pango_font_description_free (fontdesc);
   gtk_widget_hide(GTK_DIALOG(win)->action_area);
   return win;     
}

Tautocomp *ac_init()
{
	Tautocomp *ret = g_new0(Tautocomp,1);
	ret->store	= gtk_list_store_new(1,G_TYPE_STRING);
	ret->tree = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(ret->store)));
	ret->window = ac_create_window(ret->tree,ret->store);
	ret->gc = g_completion_new(NULL);	
	ret->lang_lists = g_hash_table_new(g_str_hash,g_str_equal);
	ret->dtd_lists = g_hash_table_new(g_str_hash,g_str_equal);
	return ret;
}

 

/* Runs autocomp window with given list and prefix (string from document at cursor position )
	Returns: only string which has been inserted into document
	PERFORMS INSERTION! 
*/
gchar *ac_run(Tautocomp *ac, GList *strings, gchar *prefix, GtkTextView *view) 
{
	GtkTextIter it,it3;
	GtkTextBuffer *buf = gtk_text_view_get_buffer(view);
	GdkRectangle rect;
	gint x,y,w,h,len=0;	
	GtkTreeIter it2;
	GList *lst,*items;
	gchar *tofree;
	GtkTreeSelection *selection; 
	PangoLayout *l;
	GtkTreeModel *model;
	
	if ( !view || !ac || strcmp(prefix,"")==0 ) return NULL;	
	GdkScreen *screen = gtk_widget_get_screen(GTK_WIDGET(view));
	gtk_text_buffer_get_iter_at_mark(buf,&it,gtk_text_buffer_get_insert(buf));
	gtk_text_view_get_iter_location(view,&it,&rect);
	gtk_text_view_buffer_to_window_coords(view, GTK_TEXT_WINDOW_TEXT, rect.x, rect.y,&rect.x, &rect.y);
	gdk_window_get_origin(gtk_text_view_get_window(view,GTK_TEXT_WINDOW_TEXT),&x,&y);
	l = gtk_widget_create_pango_layout(GTK_WIDGET(ac->tree), "x");
	pango_layout_get_pixel_size(l, &w, NULL);
	it3 = it;
	gtk_text_iter_set_line(&it3,gtk_text_iter_get_line(&it));
	g_completion_clear_items(ac->gc);
	g_completion_add_items(ac->gc,strings);
	items = g_completion_complete(ac->gc,prefix,NULL);
	if ( items && g_list_length(items)>0 )
	{
		gtk_list_store_clear(ac->store);
		lst = items;
		while ( lst )
		{
			gtk_list_store_append(ac->store,&it2);
			if ( strlen((gchar*)lst->data) > len )
				len = strlen((gchar*)lst->data);
			gtk_list_store_set(ac->store,&it2,0,(gchar*)lst->data,-1);				
			lst = g_list_next(lst);
		}
		w = MAX(len*w+10,40);
		h=100;
		gtk_widget_set_size_request(ac->window,w,h);	
		x += rect.x;
		y = y + rect.y + rect.height;
		if ( x + w > gdk_screen_get_width(screen)	 )
			x = gdk_screen_get_width(screen)	- x - w;
		if ( y + h > gdk_screen_get_height(screen)	 )
			y = gdk_screen_get_height(screen)	- y - h;			 					
		gtk_window_move(GTK_WINDOW(ac->window),x ,y);
		if ( gtk_dialog_run(GTK_DIALOG(ac->window)) ==GTK_RESPONSE_OK )
		{
			selection = gtk_tree_view_get_selection(ac->tree);
			if ( selection && gtk_tree_selection_get_selected(selection,&model,&it2))
			{
				gtk_tree_model_get(model,&it2,0,&tofree,-1);
				tofree = tofree+strlen(prefix);
				if ( strcmp(tofree,"")!=0 )
				{
					gtk_text_buffer_insert_at_cursor(buf,tofree,strlen(tofree));
				}
			}
		}
		gtk_widget_hide(ac->window);
	}	
	return tofree;
}

gchar *ac_run_lang(Tautocomp *ac, gchar *prefix, gchar *name, GtkTextView *view)
{
	gpointer ptr = g_hash_table_lookup(ac->lang_lists, name);
	gchar *ret=NULL;
	if ( ptr )
	{
		ret = ac_run(ac, (GList*)ptr, prefix, view);
	}
	return ret;
}

gchar *ac_run_dtd(Tautocomp *ac, gchar *prefix, gchar *name, GtkTextView *view)
{
	gpointer ptr = g_hash_table_lookup(ac->dtd_lists, name);
	gchar *ret=NULL;
	if ( ptr )
	{
		ret = ac_run(ac, (GList*)ptr, prefix, view);
	}
	return ret;
}


void ac_add_lang_list(Tautocomp *ac, gchar *name, GList *strings)
{
	g_hash_table_replace(ac->lang_lists, name, strings);
}

void ac_add_dtd_list(Tautocomp *ac, gchar *name, GList *strings)
{
	g_hash_table_replace(ac->dtd_lists, name, strings);
}


static gboolean  ac_key_choice_press  (GtkWidget *widget, GdkEventKey *event, gpointer user_data)
 {
 	GtkDialog *dlg = GTK_DIALOG(user_data);
 	if (!g_unichar_isalnum((gunichar)event->keyval) 
 		&& event->keyval!=GDK_Escape
 		&& !g_unichar_isspace((gunichar)event->keyval)
 		) return FALSE;
 	switch (event->keyval)
 	{
 		case GDK_Escape:gtk_dialog_response(dlg,GTK_RESPONSE_CANCEL);break;
 		default:
 			if ( !(event->state & GDK_SHIFT_MASK)
 					&& !(event->state & GDK_CONTROL_MASK) 
 					&& !(event->state & GDK_MOD1_MASK) )
 			gtk_dialog_response(dlg,GTK_RESPONSE_CANCEL);
 			else
 			{
 				g_object_set_data(G_OBJECT(dlg),"keyname",gtk_accelerator_name(event->keyval,event->state));
 				gtk_dialog_response(dlg,GTK_RESPONSE_OK);
 			}
 		 break;
 	}
 	return FALSE;
 }

/* Shows a dialog which allows to select accelerator key for autocompletion */
gchar *ac_key_choice()
{
  GtkWidget *dialog1;
  GtkWidget *dialog_vbox1;
  GtkWidget *vbox1;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *dialog_action_area1;

  dialog1 = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog1), "Autocompletion accelerator");
  gtk_window_set_position (GTK_WINDOW (dialog1), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (dialog1), TRUE);
  /*gtk_window_set_decorated (GTK_WINDOW (dialog1), FALSE);*/
  gtk_window_set_type_hint (GTK_WINDOW (dialog1), GDK_WINDOW_TYPE_HINT_DIALOG);
  gtk_dialog_set_has_separator (GTK_DIALOG (dialog1), FALSE);

  dialog_vbox1 = GTK_DIALOG (dialog1)->vbox;
  gtk_widget_show (dialog_vbox1);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), vbox1, FALSE, FALSE, 0);

  label1 = gtk_label_new ("<b>Keystroke choice </b>");
  gtk_widget_show (label1);
  gtk_box_pack_start (GTK_BOX (vbox1), label1, FALSE, FALSE, 0);
  gtk_label_set_use_markup (GTK_LABEL (label1), TRUE);
  gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_CENTER);
  gtk_misc_set_padding (GTK_MISC (label1), 2, 2);

  label2 = gtk_label_new ("\nPress requested key combination.\nPlease use Ctrl, Shift, Alt key with any other key.\n<i>Esc to cancel.</i>");
  gtk_widget_show (label2);
  gtk_box_pack_start (GTK_BOX (vbox1), label2, FALSE, FALSE, 0);
  gtk_label_set_use_markup (GTK_LABEL (label2), TRUE);
  gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_CENTER);
  gtk_misc_set_padding (GTK_MISC (label2), 2, 2);

  dialog_action_area1 = GTK_DIALOG (dialog1)->action_area;
  gtk_widget_show (dialog_action_area1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);
  g_signal_connect(G_OBJECT(dialog1),"key-press-event",G_CALLBACK(ac_key_choice_press),dialog1);
  if ( gtk_dialog_run(GTK_DIALOG(dialog1)) == GTK_RESPONSE_OK )
  {
  		gpointer ptr = g_object_get_data(G_OBJECT(dialog1),"keyname");
  		gtk_widget_destroy(dialog1);
  		if (ptr) return (gchar*)ptr;
  		else return NULL;	
  }  
  gtk_widget_destroy(dialog1);
  return NULL;
} 

