
#include <gtk/gtk.h>
#include "bluefish.h"
#include "gtk_easy.h"
#include "gui.h"
#include "document.h"
#include "bookmark.h"

#ifdef BOOKMARKS

enum {
 NAME_COLUMN,
 PTR_COLUMN,
 N_COLUMNS
};


typedef struct {
   GtkTextMark *mark;
   gchar *filepath;
   gint offset;   
   Tdocument *doc; 
   GtkTreeIter iter;  /* for tree view */
   gchar *description;
   gchar *text;
} Tbmark;

#define BMARK(var) ((Tbmark *)(var))


typedef struct {
	GtkWidget *tree;
	GtkWidget *viewmenu;
	GtkWidget *buttons;
	GtkTreeIter temp_branch,perm_branch; /* for easier access */
	Tbfwin *bfwin;
} Tbmark_gui;

#define BMARKGUI(var) ((Tbmark_gui *)(var))

typedef struct {
	GtkTreeStore *store;
	Tbmark *temporary[10];
	GHashTable *permanent;
	gint lasttemp;
} Tbmark_data;

#define BMARKDATA(var) ((Tbmark_data *)(var))

	

void bmark_free(gpointer ptr)
{
	Tbmark *m;
	
	if (ptr==NULL) return;
	m = BMARK(ptr);
	if (m->doc)
	{
	   gtk_text_buffer_delete_mark(m->doc->buffer,m->mark);
	   m->doc = NULL;
	}   
	g_free(m->filepath);
	g_free(m->text);
	g_free(m);
}

void bmark_clean_tree(Tbfwin *bfwin)
{
   gint i;
   Tbmark_data *data = BMARKDATA(main_v->bmarkdata);

   /* temporary */
   for(i=0;i<10;i++)
   {
     if (data->temporary[i])
     {
      gtk_tree_store_remove(data->store,&(data->temporary[i]->iter));   
     } 
   }
   gtk_tree_store_remove(data->store,&(BMARKGUI(bfwin->bmark)->temp_branch));
   gtk_tree_store_remove(data->store,&(BMARKGUI(bfwin->bmark)->perm_branch));
}



void bmark_init(void)
{
   gint i;
   Tbmark_data *data = g_new0(Tbmark_data,1);
   
	data->store = gtk_tree_store_new(N_COLUMNS,G_TYPE_STRING,G_TYPE_POINTER);
	data->permanent = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
	for(i=0;i<10;i++) data->temporary[i] = NULL;
	data->lasttemp = 0;
	main_v->bmarkdata = data;   
}

void bmark_cleanup(Tbfwin *bfwin)
{
   bmark_clean_tree(bfwin);   
	BMARKGUI(bfwin->bmark)->tree = NULL;
	BMARKGUI(bfwin->bmark)->viewmenu = NULL;
	BMARKGUI(bfwin->bmark)->buttons = NULL;
}

static Tbmark *get_current_bmark(Tbfwin *bfwin) {
	GtkTreePath *path;
	GtkTreeViewColumn *col;
	
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(BMARKGUI(bfwin->bmark)->tree), &path, &col);
	if (path != NULL) {
		Tbmark *retval=NULL;
		GValue *val;
		GtkTreeIter iter;
		gtk_tree_model_get_iter(gtk_tree_view_get_model(GTK_TREE_VIEW(BMARKGUI(bfwin->bmark)->tree)), &iter, path);
		gtk_tree_path_free(path);
		val = g_new0(GValue, 1);
		gtk_tree_model_get_value(gtk_tree_view_get_model(GTK_TREE_VIEW(BMARKGUI(bfwin->bmark)->tree)), &iter, 1, val);
		if (G_IS_VALUE(val) && g_value_fits_pointer(val)) {
			retval = BMARK(g_value_peek_pointer(val));
		}
		g_value_unset(val);
		g_free(val);
		return retval;
	}
	return NULL;
}

static void bmark_popup_menu_goto_lcb(GtkWidget *widget,gpointer user_data) 
{
	Tbmark *b,*baux;
	Tdocument *dd;
	GtkTextIter it;
	gint ret;
	gchar *pomstr;
	Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
   gchar *btns[]={GTK_STOCK_NO,GTK_STOCK_YES,NULL};
   
   

	
	if (!user_data) return;
   b = get_current_bmark(BFWIN(user_data));
   if (!b) return;
   if (b->filepath && !b->doc)
   {
      ret = multi_query_dialog(BFWIN(user_data)->main_window,_("Bookmarked file is closed"), _("Do you want to open it?"), 1, 0, btns);   
      if (ret==0) return;
      dd = doc_new_with_file(BFWIN(user_data),b->filepath,TRUE,TRUE);
/*      gtk_text_buffer_get_iter_at_offset(b->doc->buffer,&it,b->offset);
      b->mark = gtk_text_buffer_create_mark(b->doc->buffer,"",&it,TRUE); */
      /* now I have to check all bookmarks */
      for(ret=0;ret<10;ret++)
      {
        baux = data->temporary[ret];
        if ( baux && baux->filepath && !baux->doc && (g_ascii_strcasecmp(baux->filepath,b->filepath)==0) )
        {
           pomstr = g_strdup_printf("%d",ret);
           baux->doc = dd;
           gtk_text_buffer_get_iter_at_offset(baux->doc->buffer,&it,baux->offset);
           baux->mark = gtk_text_buffer_create_mark(baux->doc->buffer,pomstr,&it,TRUE); 
           g_free(pomstr);
        }
      }
   } 
   
   if (b->doc)
   {
      gtk_text_buffer_get_iter_at_mark(b->doc->buffer,&it,b->mark);
      gtk_text_buffer_place_cursor(b->doc->buffer,&it);
      gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(b->doc->view),b->mark);		
      if (b->doc != BFWIN(user_data)->current_document) switch_to_document_by_pointer(BFWIN(user_data),b->doc);    
      gtk_widget_grab_focus(b->doc->view);
   }  

}


static void bmark_popup_menu_del_lcb(GtkWidget *widget,gpointer user_data) 
{
	Tbmark *b;
	gint i;
	Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
	
	if (!user_data) return;
   b = get_current_bmark(BFWIN(user_data));
   if (!b) return;
   /* check if it is temp mark */
   for(i=0;i<10;i++)
      if (data->temporary[i]==b) break;
   if (i<10)
   {
      gtk_tree_store_remove(data->store,&(b->iter));   
      bmark_free(b);
      data->temporary[i] = NULL;
   }   
   gtk_widget_grab_focus(BFWIN(user_data)->current_document->view);
}


static GtkWidget *bmark_popup_menu(Tbfwin *bfwin, gpointer data) {
	GtkWidget *menu, *menu_item;

	menu = gtk_menu_new();
	menu_item = gtk_menu_item_new_with_label(_("Goto bookmark"));
   g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(bmark_popup_menu_goto_lcb), bfwin);		
   gtk_menu_append(GTK_MENU(menu), menu_item);
	menu_item = gtk_separator_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), menu_item);
	
	menu_item = gtk_menu_item_new_with_label(_("Rename"));
	gtk_menu_append(GTK_MENU(menu), menu_item);
	menu_item = gtk_menu_item_new_with_label(_("Delete"));
   g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(bmark_popup_menu_del_lcb), bfwin);		
	gtk_menu_append(GTK_MENU(menu), menu_item);
	
	gtk_widget_show_all(menu);
	g_signal_connect_after(G_OBJECT(menu), "destroy", G_CALLBACK(destroy_disposable_menu_cb), menu);
	return menu;
}


static gboolean bmark_event_mouseclick(GtkWidget *widget,GdkEventButton *event,Tbfwin *bfwin) {
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {	/* right mouse click */
		gtk_menu_popup(GTK_MENU(bmark_popup_menu(bfwin,NULL)), NULL, NULL, NULL, NULL, event->button, event->time);
	}
	else if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {	/* double click  */
  	   bmark_popup_menu_goto_lcb(NULL,bfwin);
  	   return TRUE;
	}
   return FALSE;
}


GtkWidget *bmark_gui(Tbfwin *bfwin)
{
   GtkWidget *vbox,*mi,*menu;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	gint i;
   Tbmark_data *bd = BMARKDATA(main_v->bmarkdata);
   
   
   bfwin->bmark = g_new(Tbmark_gui,1);
   vbox = gtk_vbox_new(FALSE,1);
   BMARKGUI(bfwin->bmark)->viewmenu = gtk_option_menu_new();
   menu = gtk_menu_new();
   mi = gtk_menu_item_new_with_label(_("Show all bookmarks"));
   gtk_menu_append(GTK_MENU(menu), mi);   
   mi = gtk_menu_item_new_with_label(_("Show bookmarks for project files"));
   gtk_menu_append(GTK_MENU(menu), mi);   
   mi = gtk_menu_item_new_with_label(_("Show bookmarks for opened files"));
   gtk_menu_append(GTK_MENU(menu), mi);   
   mi = gtk_menu_item_new_with_label(_("Show bookmarks for active file"));
   gtk_menu_append(GTK_MENU(menu), mi);   
   gtk_option_menu_set_menu(GTK_OPTION_MENU(BMARKGUI(bfwin->bmark)->viewmenu),menu);

   BMARKGUI(bfwin->bmark)->tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(bd->store));
   gtk_tree_store_append(bd->store, &(BMARKGUI(bfwin->bmark)->temp_branch), NULL);
	gtk_tree_store_set(bd->store, &(BMARKGUI(bfwin->bmark)->temp_branch), NAME_COLUMN,_("Temporary"), PTR_COLUMN,NULL, -1);
   gtk_tree_store_append(bd->store, &(BMARKGUI(bfwin->bmark)->perm_branch), NULL);
	gtk_tree_store_set(bd->store, &(BMARKGUI(bfwin->bmark)->perm_branch), NAME_COLUMN,_("Permanent"), PTR_COLUMN,NULL, -1);
	cell = gtk_cell_renderer_text_new();
	column =	gtk_tree_view_column_new_with_attributes("", cell, "text",
												 NAME_COLUMN, NULL);  
   gtk_tree_view_column_set_sizing (column,GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(BMARKGUI(bfwin->bmark)->tree), column);
	gtk_widget_show_all(BMARKGUI(bfwin->bmark)->tree);  
   gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(BMARKGUI(bfwin->bmark)->tree), FALSE);
   BMARKGUI(bfwin->bmark)->buttons = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(vbox),BMARKGUI(bfwin->bmark)->viewmenu,FALSE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(vbox),BMARKGUI(bfwin->bmark)->tree,TRUE,TRUE,0);
   gtk_box_pack_start(GTK_BOX(vbox),BMARKGUI(bfwin->bmark)->buttons,FALSE,TRUE,0);	
   
 	g_signal_connect(G_OBJECT(BMARKGUI(bfwin->bmark)->tree), "button-press-event",
					 G_CALLBACK(bmark_event_mouseclick), bfwin);

   /* restore bookmarks if it was after left panel hide */
   for (i=0;i<10;i++)
   {
     if (bd->temporary[i])
     {
		 gtk_tree_store_insert(bd->store, &(bd->temporary[i]->iter), &(BMARKGUI(bfwin->bmark)->temp_branch),i+1);
		 gtk_tree_store_set(bd->store, &(bd->temporary[i]->iter), NAME_COLUMN,g_strdup_printf("[%d] --> %s",i+1,
							   bd->temporary[i]->text),PTR_COLUMN, bd->temporary[i], -1);      
	  }	
   }
   gtk_tree_view_expand_all(GTK_TREE_VIEW(BMARKGUI(bfwin->bmark)->tree));
   
   return vbox;
}


GHashTable *bmark_get_temp_lines(Tdocument *doc)
{
  Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
  gint i,*iaux;
  Tbmark *m;
  GHashTable *ret;
  GtkTextIter it;
  
  ret = g_hash_table_new_full(g_int_hash,g_int_equal,g_free,g_free);
  for(i=0;i<10;i++)
  {
    m = data->temporary[i];
    if (m && m->doc==doc)
    {
      iaux = g_new0(gint,1);
      gtk_text_buffer_get_iter_at_mark(m->doc->buffer,&it,m->mark);
      *iaux = gtk_text_iter_get_line(&it);
      g_hash_table_insert(ret,iaux,g_strdup("+"));      
    }
  }
  return ret;
}

void bmark_clean_for_doc(Tdocument *doc)
{
   gint i;
   Tbmark *b;
   GtkTextIter it;
   Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
   
   /* clean temporary */
   for(i=0;i<10;i++)
   {
     b = data->temporary[i];
     if ( b && b->doc == doc )
     {
        gtk_text_buffer_get_iter_at_mark(b->doc->buffer,&it,b->mark);
        b->offset =  gtk_text_iter_get_offset(&it);      
        gtk_text_buffer_delete_mark(b->doc->buffer,b->mark);     
        b->mark = NULL;
        b->doc = NULL;
     }   
   }
}

void bmark_add_temp(Tbfwin *bfwin)
{
   Tbmark *m;
   GtkTextMark *im;
   GtkTextIter it,eit,sit;
   gint ret,ffree,i;
   GSList *lst;
   gboolean has_mark;
   gchar *btns[]={GTK_STOCK_NO,GTK_STOCK_YES,NULL};
   gchar *pomstr;
   Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
   Tbmark_gui *gui = BMARKGUI(bfwin->bmark);

   
   /* check for existing bookmark in this place */
   im = gtk_text_buffer_get_insert(DOCUMENT(bfwin->current_document)->buffer);
   gtk_text_buffer_get_iter_at_mark(DOCUMENT(bfwin->current_document)->buffer,&it,im);   
   ret = gtk_text_iter_get_offset(&it);
   has_mark = FALSE;
   
   for(i=0;i<10;i++)
   {
     m = data->temporary[i];
     if (m && m->mark && m->doc==DOCUMENT(bfwin->current_document))
     {
       gtk_text_buffer_get_iter_at_mark(DOCUMENT(bfwin->current_document)->buffer,&it,m->mark);   
       if ( ret == gtk_text_iter_get_offset(&it) )
          { has_mark=TRUE;break;}       
     } 
   }
   
   if (has_mark)
   {
      info_dialog(bfwin->main_window,_("Add temporary bookmark"),_("You already have a bookmark here!"));
      return;
   }

   /* find free number */
   ffree = -1;
   for(i=0;i<10;i++)
      if (data->temporary[i]==NULL)
         {
            ffree = i;
            break;
         }
   if (ffree==-1)
   {       
       pomstr = g_strdup_printf(_("You can overwrite next temp bookmark (number %d). Overwrite?"),data->lasttemp+1);
       ret = multi_query_dialog(bfwin->main_window,_("Maximum number of temp bookmarks assigned"), pomstr, 0, 0, btns);
       g_free(pomstr);
       if (ret==0) return;
       ffree = data->lasttemp;
       data->lasttemp++;       
       if (data->lasttemp == 10) data->lasttemp = 0;          
   }
   if (data->temporary[ffree]!=NULL)
    {
         gtk_tree_store_remove(data->store,&(data->temporary[ffree]->iter));
         bmark_free(data->temporary[ffree]);
    }   
   
      m = g_new0(Tbmark,1);	
      m->doc = DOCUMENT(bfwin->current_document);
      if (!m->doc) return;
      im = gtk_text_buffer_get_insert(m->doc->buffer);
      gtk_text_buffer_get_iter_at_mark(m->doc->buffer,&it,im);
      m->mark = gtk_text_buffer_create_mark(m->doc->buffer,g_strdup_printf("%d",ffree),&it,TRUE);
      m->filepath = g_strdup(m->doc->filename);
      data->temporary[ffree] = m;
      /* add to the tree */
      sit=eit=it;
      gtk_text_iter_forward_to_line_end(&eit);
      gtk_text_iter_forward_chars(&sit,10);
      if (!gtk_text_iter_in_range(&sit,&it,&eit)) 
         sit = eit;
      m->text = g_strdup(gtk_text_iter_get_slice(&it,&sit));   
		gtk_tree_store_insert(data->store, &m->iter, &(gui->temp_branch),ffree);
		gtk_tree_store_set(data->store, &m->iter, NAME_COLUMN,g_strdup_printf("[%d] --> %s",ffree+1,m->text),PTR_COLUMN, m, -1);      						   
		gtk_tree_view_expand_all(GTK_TREE_VIEW(gui->tree));
      gtk_widget_grab_focus(bfwin->current_document->view);		
}



void bmark_del_all_temp(Tbfwin *bfwin)
{
   gint ret,i;
   Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
   gchar *btns[]={GTK_STOCK_NO,GTK_STOCK_YES,NULL};   
   
   ret = multi_query_dialog(bfwin->main_window,_("Delete all temporary bookmarks."), _("Are you sure?"), 0, 0, btns);
   if (ret==0) return;
   for(i=0;i<10;i++)
   {
     if (data->temporary[i])
     {
      gtk_tree_store_remove(data->store,&(data->temporary[i]->iter));   
      bmark_free(data->temporary[i]);
      data->temporary[i] = NULL;   
     } 
      data->lasttemp=0;
   }
}

void bmark_add_perm(Tbfwin *bfwin)
{
   GtkWidget *dlg,*name,*desc,*button,*table;
   gint result;
   
   dlg = gtk_dialog_new_with_buttons(_("Add permanent bookmark "),
   										GTK_WINDOW(bfwin->main_window),
										GTK_DIALOG_MODAL,
										GTK_STOCK_OK,
				                        GTK_RESPONSE_OK,
				                        NULL);
	
	button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_dialog_add_action_widget(GTK_DIALOG(dlg), button, GTK_RESPONSE_CANCEL);									
	table = gtk_table_new(2, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE (table), 12);
	gtk_table_set_row_spacings(GTK_TABLE (table), 6);		
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dlg)->vbox), table, FALSE, FALSE, 12);
	
 	name = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(name), TRUE);
	bf_mnemonic_label_tad_with_alignment(_("_Name:"), name, 0, 0.5, table, 0, 1, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE(table), name, 1, 2, 0, 1);
	desc = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(desc), TRUE);
 	bf_mnemonic_label_tad_with_alignment(_("_Description:"), desc, 0, 0.5, table, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), desc, 1, 2, 1, 2);
	gtk_window_set_default(GTK_WINDOW(dlg), button);
	gtk_widget_show_all(dlg);
	result = gtk_dialog_run(GTK_DIALOG(dlg));     
    gtk_widget_destroy(dlg);
}


#endif /* BOOKMARKS */

