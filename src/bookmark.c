/* Bluefish HTML Editor - bookmarks
 *
 * Copyright (C) 2003 Oskar Swida
 * modifications (C) 2004 Olivier Sessink
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
#define DEBUG

#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "bluefish.h"
#include "gtk_easy.h"
#include "gui.h"
#include "document.h"
#include "stringlist.h"
#include "bookmark.h"

enum {
 NAME_COLUMN,  /* bookmark name */
 PTR_COLUMN,   /* bookmark pointer */ 
 N_COLUMNS
};

enum {
	BMARK_ADD_PERM_DIALOG,
	BMARK_RENAME_TEMP_DIALOG,
	BMARK_RENAME_PERM_DIALOG
};

typedef struct {
	GtkTextMark *mark;
	gchar *filepath;
	gint offset;	
	Tdocument *doc; 
	GtkTreeIter iter;         /* for tree view */
	gchar *description;
	gchar *text;
	gchar *name;	
	gint len;                 /* file length for integrity check - perhaps some hash code is needed */
	gboolean is_temp; 
	gchar **strarr;          /* this is a pointer to the location where this bookmark is stored in the sessionlist,
							          so we can immediately change it _in_ the list */
} Tbmark;

#define BMARK(var) ((Tbmark *)(var))

#define BM_FMODE_FULL     0
#define BM_FMODE_HOME     1
#define BM_FMODE_FILE     2


/* Free bookmark structure */
static void bmark_free(gpointer ptr) {
	Tbmark *m;
	if (ptr==NULL) return;
	m = BMARK(ptr);
	if (m->doc && m->mark) {
		gtk_text_buffer_delete_mark(m->doc->buffer,m->mark);
		m->doc = NULL;
	}
	g_free(m->filepath);
	g_free(m->text);
	g_free(m->name);
	g_free(m->description);
	g_free(m);
}

/* this function re-uses the b->strarr if possible, otherwise it will create a new one and
append it to the list */
static void bmark_store(Tbfwin *bfwin, Tbmark *b) {
   gchar **strarr;
   
   /* if there is a strarr already, we only update the fields, else we append a new one */
   if (b->strarr == NULL) {
	   strarr = g_malloc0(sizeof(gchar*)*7);
      strarr[0] = g_strdup(b->name);
	   strarr[1] = g_strdup(b->description);
	   strarr[2] = g_strdup(b->filepath);
	   strarr[4] = g_strdup(b->text);
	} else {
		strarr = b->strarr;
		/* free the ones we are going to update */
		g_free(strarr[3]);
		g_free(strarr[5]);
	}

#ifdef HAVE_GNOME_VFS	
   if (b->doc)	b->len = b->doc->fileinfo->size;
#else
   if (b->doc)	b->len = b->doc->statbuf.st_size;
#endif
   
   strarr[3] = g_strdup_printf("%d",b->offset);
   strarr[5] = g_strdup_printf("%d",b->len);
   if (b->strarr == NULL) {
   	DEBUG_MSG("added new (previously unstored) bookmark to session list, list length=%d\n",g_list_length(bfwin->session->bmarks));
	   bfwin->session->bmarks = g_list_append(bfwin->session->bmarks,strarr);
	   b->strarr = strarr;
   }
}

static void bmark_remove(Tbfwin *bfwin, Tbmark *b) {
  if (bfwin->session->bmarks == NULL || b->strarr == NULL ) return;
  g_list_remove(bfwin->session->bmarks,b->strarr);
}



/* get bookmark from pointer column */
static Tbmark *get_current_bmark(Tbfwin *bfwin) {
	GtkTreePath *path;
	GtkTreeViewColumn *col;
	
	gtk_tree_view_get_cursor(bfwin->bmark, &path, &col);
	if (path != NULL) {
		Tbmark *retval=NULL;
		GValue *val;
		GtkTreeIter iter;
		gtk_tree_model_get_iter(gtk_tree_view_get_model(bfwin->bmark), &iter, path);
		gtk_tree_path_free(path);
		val = g_new0(GValue, 1);
		gtk_tree_model_get_value(gtk_tree_view_get_model(bfwin->bmark), &iter, 1, val);
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
	Tbmark *b;
	GtkTextIter it;

	if (!user_data) return;
	b = get_current_bmark(BFWIN(user_data));
	if (!b) return;
	if (b->filepath && !b->doc) {
		/* check if that document _is_ open */
		Tdocument *tmpdoc;
		GList *doclist = return_allwindows_documentlist();
		tmpdoc = documentlist_return_document_from_filename(doclist, b->filepath);
		g_list_free(doclist);
		if (tmpdoc == NULL) {
			if (!g_file_test(b->filepath,G_FILE_TEST_EXISTS)) {
				gchar *string = g_strdup_printf (_("Could not find the file \"%s\"."), b->filepath);
				error_dialog(BFWIN(user_data)->main_window, string, _("This bookmark is set in a file that no longer exists."));
				g_free (string);
				return;
			}
			tmpdoc = doc_new_with_file(BFWIN(user_data),b->filepath,TRUE,TRUE);
		}
		/* now I have to check all bookmarks */
		bmark_set_for_doc(tmpdoc);
	} 
	
	if (b->doc)	{
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
	gint ret;
	gchar *pstr;
	gchar *btns[]={GTK_STOCK_NO,GTK_STOCK_YES,NULL};	
		
	if (!user_data) return;
	b = get_current_bmark(BFWIN(user_data));
	if (!b) return;
	/* check if it is temp mark */
	if (b->is_temp)
	{
			gtk_tree_store_remove(BFWIN(user_data)->bookmarkstore,&(b->iter));	
  		   bmark_remove(BFWIN(user_data),b);			
			bmark_free(b);
	}	
	else
	{	
		pstr = g_strdup_printf(_("Do you really want to remove %s?"),b->name);
		ret = multi_query_dialog(BFWIN(user_data)->main_window,_("Delete permanent bookmark."), pstr, 0, 0, btns);
		g_free(pstr);
		if (ret==0) return; 
		gtk_tree_store_remove(BFWIN(user_data)->bookmarkstore,&(b->iter));	
		bmark_remove(BFWIN(user_data),b);
		bmark_free(b);
	}
	gtk_widget_grab_focus(BFWIN(user_data)->current_document->view);
}

static void bmark_popup_menu_rename_lcb(GtkWidget *widget, Tbfwin *bfwin) 
{
	Tbmark *m;

	m = get_current_bmark(bfwin);
	if (!m) return;
	
	if (!m->is_temp) {
			bmark_add_rename_dialog(bfwin, _("Rename permanent bookmark"), BMARK_RENAME_PERM_DIALOG);
	} else {
			bmark_add_rename_dialog(bfwin, _("Rename temporary bookmark"), BMARK_RENAME_TEMP_DIALOG);
	}
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
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(bmark_popup_menu_rename_lcb), bfwin);
	gtk_menu_append(GTK_MENU(menu), menu_item);
	menu_item = gtk_menu_item_new_with_label(_("Delete"));
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(bmark_popup_menu_del_lcb), bfwin);
	gtk_menu_append(GTK_MENU(menu), menu_item);
	
	gtk_widget_show_all(menu);
	g_signal_connect_after(G_OBJECT(menu), "destroy", G_CALLBACK(destroy_disposable_menu_cb), menu);

	return menu;
}


/* right mouse click */
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



/* Initialize bookmarks gui for window */

GtkWidget *bmark_gui(Tbfwin *bfwin) {
	GtkWidget *vbox;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	
	/* Tree Store is in bfwin->bookmarkstore 
      Tree View is in bfwin->bmark	
	*/
	vbox = gtk_vbox_new(FALSE,1);	
	bfwin->bmark = gtk_tree_view_new_with_model(GTK_TREE_MODEL(bfwin->bookmarkstore));
	cell = gtk_cell_renderer_text_new();
	column =	gtk_tree_view_column_new_with_attributes("", cell, "text",
												 NAME_COLUMN, NULL);  
	gtk_tree_view_column_set_sizing (column,GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(bfwin->bmark), column);
	gtk_widget_show_all(bfwin->bmark);  
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(bfwin->bmark), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox),bfwin->bmark,TRUE,TRUE,0);
	g_signal_connect(G_OBJECT(bfwin->bmark), "button-press-event",G_CALLBACK(bmark_event_mouseclick), bfwin);
	gtk_tree_view_expand_all(bfwin->bmark);  
	return vbox;
}




/* determine bookmark's location in the tree and  insert - result GtkTreeIter is stored in m->iter */
static void bmark_get_iter_at_position(Tbfwin *bfwin, Tbmark *m) {
   GtkTreeIter *parent;
   
   if ( m->doc == NULL || m->doc->bmark_parent == NULL ) 
   {
    	parent = g_new0(GtkTreeIter,1);
		gtk_tree_store_append(bfwin->bookmarkstore,parent,NULL);
		switch (main_v->props.bookmarks_filename_mode) {
			case BM_FMODE_FULL:
            gtk_tree_store_set(bfwin->bookmarkstore, parent, NAME_COLUMN, m->filepath,PTR_COLUMN, NULL, -1);
            break;
			case BM_FMODE_HOME: /* todo */
				if (bfwin->project != NULL) {
					gchar *pstr = m->filepath;
					pstr += strlen(bfwin->project->basedir);
					gtk_tree_store_set(bfwin->bookmarkstore, parent, NAME_COLUMN, pstr,PTR_COLUMN, NULL,  -1);
				} else {
					gtk_tree_store_set(bfwin->bookmarkstore, parent, NAME_COLUMN, g_path_get_basename(m->filepath),PTR_COLUMN, NULL, -1);  
				}
            break;
			case BM_FMODE_FILE:
				gtk_tree_store_set(bfwin->bookmarkstore, parent, NAME_COLUMN, g_path_get_basename(m->filepath),PTR_COLUMN, NULL,  -1);
			break;                   
		}  
		if (m->doc != NULL)  m->doc->bmark_parent = parent;
   }
   else 
    parent = m->doc->bmark_parent;
   
	DEBUG_MSG("bmark_get_iter_at_position, sorting=%d\n",main_v->props.bookmarks_sort);
	if (main_v->props.bookmarks_sort) {
		GtkTreeIter tmpiter;
		gboolean cont;
		cont = gtk_tree_model_iter_children(GTK_TREE_MODEL(bfwin->bookmarkstore),&tmpiter,parent);
		while (cont) {
			Tbmark *tmpm=NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(bfwin->bookmarkstore), &tmpiter, PTR_COLUMN, &tmpm, -1);
			if (tmpm) {
				gint val = strcmp(m->filepath, tmpm->filepath);
				if (val == 0) {
					DEBUG_MSG("bmark_get_iter_at_position, comparing two iters\n");
					if (m->offset > tmpm->offset) {
						gtk_tree_store_insert_before(bfwin->bookmarkstore,&m->iter,parent,&tmpiter);
						return;
					}
				} else if (val < 0){
					/* different file */
					gtk_tree_store_insert_before(bfwin->bookmarkstore,&m->iter,parent,&tmpiter);
					return;
				}
			}
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(bfwin->bookmarkstore),&tmpiter);
		}
	}
	gtk_tree_store_append(bfwin->bookmarkstore, &m->iter, parent);
}


static gboolean bmark_name_exists(Tdocument *doc,const gchar *name)
{
  GtkTreeIter tmpiter;
  gboolean cont;

  if (doc->bmark_parent == NULL) return FALSE;
  cont = gtk_tree_model_iter_children(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore),&tmpiter,doc->bmark_parent);
  while (cont) {
			Tbmark *mark=NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &tmpiter, PTR_COLUMN, &mark, -1);
			if (mark) {
			   if (strcmp(mark->name,name) == 0) return TRUE;
			}
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore),&tmpiter);
  } /* cont */
  return FALSE;
}




/*---------------------- GLOBAL ----------------------------------*/
void bmark_init()
{
  main_v->bookmarkstore = gtk_tree_store_new(N_COLUMNS,G_TYPE_STRING,G_TYPE_POINTER);  
}


/* this function will load the bookmarks
 * from bfwin->session->bmarks and parse
 * them into treestore bfwin->bookmarkstore
 */
void bmark_reload(Tbfwin *bfwin)
{
	GList *tmplist = g_list_first(bfwin->session->bmarks);
	while (tmplist) {
	  	gchar **items = (gchar **) tmplist->data;
	  	gchar *ptr = NULL;
		if (items && count_array(items)==6) {
			Tbmark *b;
			b = g_new0(Tbmark,1);
			b->name = g_strdup(items[0]);
			b->description = g_strdup(items[1]);
			b->filepath = g_strdup(items[2]);
			b->offset = atoi(items[3]);
			b->text = g_strdup(items[4]);
			b->len = atoi(items[5]);
			b->strarr = items;
			bmark_get_iter_at_position(bfwin,b);
			ptr = g_strdup_printf("[%s] --> %s",b->name,b->text);
	      gtk_tree_store_set(bfwin->bookmarkstore, &(b->iter), NAME_COLUMN,ptr,PTR_COLUMN, b, -1);		
			g_free(ptr);
		}
		tmplist = g_list_next(tmplist);
	}
}


void bmark_set_store(Tbfwin *bfwin)
{
  if (bfwin->bookmarkstore)
  {
    gtk_tree_view_set_model(bfwin->bmark,GTK_TREE_MODEL(bfwin->bookmarkstore));
  }
}

void bmark_clean_for_doc(Tdocument *doc)
{
  GtkTreeIter tmpiter;
  GtkTextIter it;
  gboolean cont;

  if (doc->bmark_parent == NULL) return;
  cont = gtk_tree_model_iter_children(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore),&tmpiter,doc->bmark_parent);
  while (cont) {
			Tbmark *b=NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &tmpiter, PTR_COLUMN, &b, -1);
			if (b) {
		        gtk_text_buffer_get_iter_at_mark(doc->buffer,&it,b->mark);
		        b->offset =  gtk_text_iter_get_offset(&it);		
		        gtk_text_buffer_delete_mark(doc->buffer,b->mark);	  
		        b->mark = NULL;
		        b->doc = NULL;
			}
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore),&tmpiter);
  } /* cont */
  g_free(doc->bmark_parent);
}

/*
 * this function will check is this document needs any bookmarks, and set the
 * doc->bmark_parent if needed
 */
void bmark_set_for_doc(Tdocument *doc)
{
  GtkTreeIter tmpiter;
  GtkTextIter it;
  gboolean cont;
  cont = gtk_tree_model_iter_children(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore),&tmpiter,NULL);
  while (cont) {
			GtkTreeIter child;
			if (gtk_tree_model_iter_children(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore),&child,&tmpiter))
			{
			  Tbmark *mark=NULL;
			  gtk_tree_model_get(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &child, PTR_COLUMN, &mark, -1);
			  if (mark) {
			     if ( strcmp(mark->filepath,doc->filename) == 0 )
			     { /* this is it */
			        gboolean cont2;
			        mark->doc = doc;
		           gtk_text_buffer_get_iter_at_offset(doc->buffer,&it,mark->offset);
		           mark->mark = gtk_text_buffer_create_mark(doc->buffer,mark->name,&it,TRUE); 
			        cont2 = gtk_tree_model_iter_next(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore),&child);
			        while (cont2) 
			        {
			          mark = NULL;
			          gtk_tree_model_get(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &child, PTR_COLUMN, &mark, -1);
			          if (mark)
			          {
			              mark->doc = doc;
		                 gtk_text_buffer_get_iter_at_offset(doc->buffer,&it,mark->offset);
		                 mark->mark = gtk_text_buffer_create_mark(doc->buffer,mark->name,&it,TRUE); 
			          }
			          cont2 = gtk_tree_model_iter_next(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore),&child);
			        }
			        doc->bmark_parent = g_memdup(&tmpiter,sizeof(GtkTreeIter));
			        break;
			     }
			  }
			}
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore),&tmpiter);
  } /* cont */  
}


GHashTable *bmark_get_lines(Tdocument *doc,gboolean temp) {
  GHashTable *ret;
  GtkTreeIter tmpiter;
  GtkTextIter it;
  gboolean cont;
  int *iaux;

  ret = g_hash_table_new_full(g_int_hash,g_int_equal,g_free,g_free);
  if (doc->bmark_parent == NULL) return ret;
  cont = gtk_tree_model_iter_children(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore),&tmpiter,doc->bmark_parent);
  while (cont) {
			Tbmark *mark=NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &tmpiter, PTR_COLUMN, &mark, -1);
			if (mark && mark->mark && mark->doc==doc && mark->is_temp == temp) {
		     iaux = g_new0(gint,1);
		     gtk_text_buffer_get_iter_at_mark(doc->buffer,&it,mark->mark);
		     *iaux = gtk_text_iter_get_line(&it);
		     g_hash_table_insert(ret,iaux,g_strdup("+"));			 
			}
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore),&tmpiter);
  } /* cont */ 
  return ret;
}


void bmark_add_temp(Tbfwin *bfwin)
{
	Tbmark *m;
	GtkTextMark *im;
	GtkTextIter it,eit,sit;
	gint ret;
	gboolean has_mark,cont;
   GtkTreeIter tmpiter;
	
	/* check for nameless document */
	if (!DOCUMENT(bfwin->current_document)->filename) {
		error_dialog(bfwin->main_window,_("Add temporary bookmark"),_("Cannot add bookmarks in nameless files.\nPlease, save the file first."));
		return;
	}

	/* check for existing bookmark in this place */
	im = gtk_text_buffer_get_insert(DOCUMENT(bfwin->current_document)->buffer);
	gtk_text_buffer_get_iter_at_mark(DOCUMENT(bfwin->current_document)->buffer,&it,im);	
	ret = gtk_text_iter_get_offset(&it);
	has_mark = FALSE;
   cont = gtk_tree_model_iter_children(GTK_TREE_MODEL(bfwin->bookmarkstore),&tmpiter,DOCUMENT(bfwin->current_document)->bmark_parent);
   while (cont) {
			m=NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(bfwin->bookmarkstore), &tmpiter, PTR_COLUMN, &m, -1);
			if (m && m->mark) {
		     	gtk_text_buffer_get_iter_at_mark(DOCUMENT(bfwin->current_document)->buffer,&eit,m->mark);	
		     	if ( gtk_text_iter_get_offset(&eit) == ret )
		     	   {
		     	      has_mark = TRUE;
		     	      break;
		     	   } 
			}
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(bfwin->bookmarkstore),&tmpiter);
   } /* cont */ 
	
	if (has_mark) {
		info_dialog(bfwin->main_window,_("Add temporary bookmark"),_("You already have a bookmark here!"));
		return;
	}


   /* create bookmark */	
	m = g_new0(Tbmark,1);	
	m->doc = DOCUMENT(bfwin->current_document);
	im = gtk_text_buffer_get_insert(m->doc->buffer);
	gtk_text_buffer_get_iter_at_mark(m->doc->buffer,&it,im);
	m->mark = gtk_text_buffer_create_mark(m->doc->buffer,"temp",&it,TRUE);
	m->filepath = g_strdup(m->doc->filename);
	m->is_temp = TRUE;
	m->name = g_strdup_printf("temp");
	sit=eit=it;
	gtk_text_iter_forward_to_line_end(&eit);
	gtk_text_iter_forward_chars(&sit,10);
	if (!gtk_text_iter_in_range(&sit,&it,&eit)) 
		sit = eit;
	m->text = g_strdup(gtk_text_iter_get_slice(&it,&sit));
	/* insert into tree */
	bmark_get_iter_at_position(bfwin,m);
	gtk_tree_store_set(bfwin->bookmarkstore, &m->iter, NAME_COLUMN,g_strdup_printf("[temp] --> %s",m->text),PTR_COLUMN, m, -1);
	gtk_tree_view_expand_all(bfwin->bmark);
	gtk_widget_grab_focus(bfwin->current_document->view);		
	/* and store */
	bmark_store(bfwin,m);
}

void bmark_add_perm(Tbfwin *bfwin)
{
	/* check for nameless document */
	if (!DOCUMENT(bfwin->current_document)->filename)
	{
		error_dialog(bfwin->main_window,_("Add permanent bookmark"),_("Cannot add bookmarks in nameless files.\nPlease, save the file first."));
		return;
	}
	bmark_add_rename_dialog(bfwin, _("Add permanent bookmark"), BMARK_ADD_PERM_DIALOG);
}

void bmark_add(Tbfwin *bfwin)
{
  if (main_v->props.bookmarks_default_store)
     bmark_add_perm(bfwin);
  else
     bmark_add_temp(bfwin);     
}



/*    I wonder if this function is needed ... Oskar */


void bmark_del_all(Tbfwin *bfwin,gboolean ask) {}
/*
{
	gint ret;
   gchar *btns[]={GTK_STOCK_NO,GTK_STOCK_YES,NULL};	
   GtkTreeIter tmpiter;
   gboolean cont;
   Tdocument *doc = bfwin->current_document
	
	
	if (ask)	{
	  ret = multi_query_dialog(bfwin->main_window,_("Delete all permanent bookmarks."), _("Are you REALLY sure?"), 0, 0, btns);
	  if (ret==0) return;
	}  

  if (doc->bmark_parent == NULL) return;
  ret = g_hash_table_new_full(g_int_hash,g_int_equal,g_free,g_free);
  cont = gtk_tree_model_iter_children(GTK_TREE_MODEL(doc->bfwin->bookmarkstore),&tmpiter,doc->bmark_parent);
  while (cont) {
			Tbmark *mark=NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(doc->bfwin->bookmarkstore), &tmpiter, PTR_COLUMN, &mark, -1);
			if (mark && mark->mark && mark->doc==doc && mark->is_temp == temp) {
		     iaux = g_new0(gint,1);
		     gtk_text_buffer_get_iter_at_mark(b->doc->buffer,&it,b->mark);
		     *iaux = gtk_text_iter_get_line(&it);
		     g_hash_table_insert(ret,iaux,g_strdup("+"));			 
			}
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(doc->bfwin->bookmarkstore),&tmpiter);
  } 
	
	
			gtk_tree_store_remove(BFWIN(user_data)->bookmarkstore,&(b->iter));	
  		   bmark_remove(BFWIN(user_data),b);			
			bmark_free(b);
	
} */



void bmark_check_length(Tbfwin *bfwin,Tdocument *doc)
{
  gchar *pstr;
  GtkTreeIter tmpiter;
  gboolean cont;
  glong size;

  if (!doc || !doc->bmark_parent) return;
  
  cont = gtk_tree_model_iter_children(GTK_TREE_MODEL(bfwin->bookmarkstore),&tmpiter,doc->bmark_parent);
  while (cont) {
			Tbmark *mark=NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &tmpiter, PTR_COLUMN, &mark, -1);
			if (mark) {
			
#ifdef HAVE_GNOME_VFS	
             size = doc->fileinfo->size;
#else
             size = doc->statbuf.st_size;
#endif
             if  (mark->len != size)
             {
	             pstr = g_strdup_printf(_("Character count changed in file\n %s."),doc->filename);
	             warning_dialog(bfwin->main_window,pstr, _("Bookmarks positions can be incorrect."));
	             g_free(pstr);
                return;
             }
			}
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(bfwin->bookmarkstore),&tmpiter);
  } 
}



void bmark_name_entry_changed(GtkEntry * entry, GtkDialog* dialog) {
	const gchar *string;
	
	string = gtk_entry_get_text (GTK_ENTRY (entry));
	if (strlen (string) <= 0)
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_OK, FALSE);
	else
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_OK, TRUE);
}

void bmark_add_rename_dialog(Tbfwin *bfwin, gchar *dialogtitle, gint dialogtype)
{
	GtkWidget *dlg, *name, *desc, *button, *table;
	GtkTextMark *im;	
	gint result;
	gchar *pstr;
	GtkTextIter it, sit, eit;
	Tbmark *m = NULL;
	 
	if (dialogtype != BMARK_ADD_PERM_DIALOG) {
		m = get_current_bmark(bfwin);
			if (!m) return;
	}
	
	dlg = gtk_dialog_new_with_buttons(dialogtitle,GTK_WINDOW(bfwin->main_window),GTK_DIALOG_MODAL,
			GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,NULL);
	button = gtk_button_new_from_stock(GTK_STOCK_OK);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_dialog_add_action_widget(GTK_DIALOG(dlg), button, GTK_RESPONSE_OK);	
	gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);						
	table = gtk_table_new(2, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE (table), 12);
	gtk_table_set_row_spacings(GTK_TABLE (table), 6);		
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dlg)->vbox), table, FALSE, FALSE, 12);
	
 	name = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(name), TRUE);
	bf_mnemonic_label_tad_with_alignment(_("_Name:"), name, 0, 0.5, table, 0, 1, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE(table), name, 1, 2, 0, 1);
	g_signal_connect (G_OBJECT (name), "changed", G_CALLBACK (bmark_name_entry_changed), dlg);
	desc = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(desc), TRUE);
 	bf_mnemonic_label_tad_with_alignment(_("_Description:"), desc, 0, 0.5, table, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), desc, 1, 2, 1, 2);
	gtk_window_set_default(GTK_WINDOW(dlg), button);
	
	if (m != NULL && m->is_temp == FALSE) {
		gtk_entry_set_text(GTK_ENTRY(name),m->name);		
		gtk_entry_set_text(GTK_ENTRY(desc),m->description);
	}
	
	gtk_widget_show_all(dlg);
	result = gtk_dialog_run(GTK_DIALOG(dlg));	  
	
	if (result == GTK_RESPONSE_OK) {		
		/* check if name exists */
		if (bmark_name_exists(DOCUMENT(bfwin->current_document),gtk_entry_get_text(GTK_ENTRY(name)))) {
			pstr = g_strdup_printf(_("You already have a bookmark named %s!"), gtk_entry_get_text(GTK_ENTRY(name)));
			if (dialogtype == BMARK_ADD_PERM_DIALOG) {
				info_dialog(bfwin->main_window, dialogtitle, pstr);		
			} else {
				info_dialog(bfwin->main_window, dialogtitle, pstr);
			}
			g_free(pstr);
			gtk_widget_destroy(dlg);
			bmark_add_rename_dialog(bfwin, dialogtitle, dialogtype);
			return;
		} else if (dialogtype == BMARK_ADD_PERM_DIALOG) {
			m = g_new0(Tbmark,1);	
			m->doc = DOCUMENT(bfwin->current_document);
			if (!m->doc) return;
			im = gtk_text_buffer_get_insert(m->doc->buffer);
			gtk_text_buffer_get_iter_at_mark(m->doc->buffer, &it, im);
			m->mark = gtk_text_buffer_create_mark(m->doc->buffer, g_strdup(gtk_entry_get_text(GTK_ENTRY(name))), &it, TRUE);
			m->filepath = g_strdup(m->doc->filename);
			m->is_temp = FALSE;			
			m->name = g_strdup(gtk_entry_get_text(GTK_ENTRY(name)));
			m->description = g_strdup(gtk_entry_get_text(GTK_ENTRY(desc)));
			sit=eit=it;
			gtk_text_iter_forward_to_line_end(&eit);
			gtk_text_iter_forward_chars(&sit, 10);
			if (!gtk_text_iter_in_range(&sit, &it, &eit)) sit = eit;
			m->text = g_strdup(gtk_text_iter_get_slice(&it, &sit));	
				
			bmark_get_iter_at_position(bfwin,m);
			gtk_tree_store_set(bfwin->bookmarkstore, &m->iter, NAME_COLUMN, g_strdup_printf("[%s] --> %s", gtk_entry_get_text(GTK_ENTRY(name)), m->text),PTR_COLUMN, m, -1);
			gtk_tree_view_expand_all(bfwin->bmark);
			gtk_widget_grab_focus(bfwin->current_document->view);	
			bmark_store(bfwin,m);
		} else {
			pstr = g_strdup(m->name);
			g_free(m->name);
			m->name = g_strdup(gtk_entry_get_text(GTK_ENTRY(name)));
			g_free(m->description);
			m->description = g_strdup(gtk_entry_get_text(GTK_ENTRY(desc)));
			if (m->is_temp) {
				m->is_temp = FALSE;
	 			gtk_tree_store_set(bfwin->bookmarkstore, &m->iter, NAME_COLUMN, g_strdup_printf("[%s] --> %s",m->name,m->text),PTR_COLUMN, m, -1);		 
			}
			gtk_tree_store_set(bfwin->bookmarkstore, &m->iter, NAME_COLUMN, g_strdup_printf("[%s] --> %s",m->name,m->text),-1);
			bmark_store(bfwin,m);
			g_free(pstr);			
		}
	}
	gtk_widget_destroy(dlg);		
}

void bmark_cleanup(Tbfwin *bfwin)
{
  /* I don't know if this is needed */
  
  
}