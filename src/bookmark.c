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
/* #define DEBUG */

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
#include "bookmark.h"

enum {
 NAME_COLUMN,
 PTR_COLUMN,
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
	GtkTreeIter iter;  /* for tree view */
	gchar *description;
	gchar *text;
	gchar *name;	
	gint number;
	gint len; /* file length for integrity check - perhaps some hash code is needed */
	gboolean is_temp; /* is this bookmark temporary,... 
								I wonder if that means we should insert all bookmarks
								into the same table, ... keeping different tables
								can be comfortable ... */
} Tbmark;

#define BMARK(var) ((Tbmark *)(var))


typedef struct {
	GtkWidget *tree;
/*	GtkWidget *viewmenu;*/
	GtkWidget *buttons;
	GtkTreeIter temp_branch,perm_branch; /* for easier access */
	Tbfwin *bfwin;
} Tbmark_gui;

#define BMARKGUI(var) ((Tbmark_gui *)(var))

typedef struct {
	GtkTreeStore *store;
	GHashTable *temporary;  
	GHashTable *permanent;
	gint lasttemp;
/*	guchar visible_bmarks;*/
   guchar filename_mode;
	GHashTable *file_iters_temp,*file_iters_perm;
} Tbmark_data;

#define BMARKDATA(var) ((Tbmark_data *)(var))

typedef struct {
	Tdocument *doc;
	gchar *string;
	gpointer ptr;
	gint integer;
} Tforeach_data;

#define BM_FMODE_FULL     1
#define BM_FMODE_HOME     2
#define BM_FMODE_FILE     3

static void bmark_free(gpointer ptr) {
	Tbmark *m;
	
	if (ptr==NULL) return;
	m = BMARK(ptr);
	if (m->doc && m->mark)
	{
		gtk_text_buffer_delete_mark(m->doc->buffer,m->mark);
		m->doc = NULL;
	}	
	g_free(m->filepath);
	g_free(m->text);
	g_free(m->name);
	g_free(m->description);
	g_free(m);
}


static void clean_proc(gpointer key,gpointer value,gpointer user_data) {
  Tbmark *b = BMARK(value);
  Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
	
  if (b  &&  (b->iter.stamp != 0) )
	 {
		 gtk_tree_store_remove(data->store,&(b->iter));		 
		 b->iter.stamp = 0;
	 }	
}

static void clean_fi_proc(gpointer key,gpointer value,gpointer user_data) {
  GtkTreeIter *it = (GtkTreeIter*)value;
  Tbmark_data *data = BMARKDATA(main_v->bmarkdata);   	
  gtk_tree_store_remove(data->store,it);
  g_free(it);		 	
}


static gboolean delmark_proc(gpointer key,gpointer value,gpointer user_data) {
  Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
  Tbmark *b = BMARK(value);
  
  gtk_tree_store_remove(data->store,&(b->iter));	
  bmark_free(b);
  return TRUE;
}


static void bmark_clean_tree(Tbfwin *bfwin) {
	Tbmark_data *data = BMARKDATA(main_v->bmarkdata);

	/* temporary */
	g_hash_table_foreach(data->temporary,clean_proc,NULL);
	/* permanent */
	g_hash_table_foreach(data->permanent,clean_proc,NULL);
	/* file iters */
	g_hash_table_foreach(data->file_iters_temp,clean_fi_proc,NULL);	
	g_hash_table_foreach(data->file_iters_perm,clean_fi_proc,NULL);	
   /* branches */	
	gtk_tree_store_remove(data->store,&(BMARKGUI(bfwin->bmark)->temp_branch));
	gtk_tree_store_remove(data->store,&(BMARKGUI(bfwin->bmark)->perm_branch));
}

/* this is the new loading function ! */
static void load_bmarks_from_list(GList *sourcelist, GHashTable **tab) {
	GList *tmplist = g_list_first(sourcelist);
	while (tmplist) {
		gchar **items = (gchar **) tmplist->data;
		if (items && items[0] && items[1] && items[2] && items[3] && items[4] && items[5]) {
			Tbmark *b;
			b = g_new0(Tbmark,1);
			b->name = g_strdup(items[0]);
			b->description = g_strdup(items[1]);
			b->filepath = g_strdup(items[2]);
			b->offset = atoi(items[3]);
			b->text = g_strdup(items[4]);
			b->len = atoi(items[5]);
			g_hash_table_insert(*tab,g_strdup(b->name),b);
		}
		tmplist = g_list_next(tmplist);
	}
}

static void load_bmarks_from_file(gchar *filename,GHashTable **tab) {
	Tbmark *b;
	gchar *content;
	gchar **lines,**items;
	gint i;
	
	if (g_file_get_contents(filename,&content,NULL,NULL)) {
	  lines = g_strsplit(content,"\n",0);
	  i=0;
	  while (lines[i]) {
		 items = g_strsplit(lines[i],":",0);
		 if ( items && items[0] && items[1] && items[2] && items[3] && items[4] && items[5])
		 {
			 b = g_new0(Tbmark,1);
			 b->name = g_strdup(items[0]);
			 b->description = g_strdup(items[1]);
			 b->filepath = g_strdup(items[2]);
			 b->offset = atoi(items[3]);
			 b->text = g_strdup(items[4]);
			 b->len = atoi(items[5]);
			 g_hash_table_insert(*tab,g_strdup(b->name),b);
		  }  
		 g_strfreev(items);		  
		 i++;		  
	  }
	  g_strfreev(lines);
	}
}

static void set_offset_proc(gpointer key,gpointer value,gpointer user_data) {
	Tbmark *b = BMARK(value);
	GtkTextIter it;
	if (b->doc && b->mark) {
	  gtk_text_buffer_get_iter_at_mark(b->doc->buffer,&it,b->mark);
	  b->offset =  gtk_text_iter_get_offset(&it);				 
	}
}

static void save_bmark_proc(gpointer key,gpointer value,gpointer user_data) {
	Tbmark *b = BMARK(value);
	int *fd = (int*)user_data;
	gchar *pstr;

	/* you have to update offset BEFORE calling this */	
	/* setting length */
	/* can we not use the filesize as stored in the Tdocument, it is also used if the 
	document is modified on disk and stuff like that - Olivier */
	/* we can, but where is filesize in Tdocument ? - Oskar */
	if (b->doc)	b->len = gtk_text_buffer_get_char_count(b->doc->buffer);
	
	pstr = g_strdup_printf("%s:%s:%s:%d:%s:%d\n",b->name,b->description,b->filepath,b->offset,b->text,b->len);
	write(*fd,pstr,strlen(pstr)); 
}

static void store_mark(Tbmark *mark,gchar *oldname) {
	gchar *fname = g_strconcat(g_get_home_dir(), "/.bluefish/bookmarks", NULL);
	GHashTable *tab;
	gchar  *pstr;
	int fd;
	GtkTextIter it;
	tab = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,bmark_free);
	load_bmarks_from_file(fname,&tab);
	if ( oldname!=NULL && g_hash_table_lookup(tab,oldname) ) {
		g_hash_table_remove(tab,oldname);  
	}  
	pstr = g_strdup(mark->name);
	gtk_text_buffer_get_iter_at_mark(mark->doc->buffer,&it,mark->mark);
	mark->offset =  gtk_text_iter_get_offset(&it);			 
	g_hash_table_insert(tab,pstr,mark); 
	
	/* if we would change the hash table into a GList we can use the functions from 
	rcfile.c to load and store the bookmarks - Olivier */
	fd = open(fname,O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd) {
		g_hash_table_foreach(tab,save_bmark_proc,&fd);
		close(fd);
	}
	g_hash_table_steal(tab,mark->name);
	g_free(pstr);
	g_free(fname);
	g_hash_table_destroy(tab);
}


void bmark_save_all(void) {
  gint fd;
  gchar *fname = g_strconcat(g_get_home_dir(), "/.bluefish/bookmarks", NULL);
  Tbmark_data *data = BMARKDATA(main_v->bmarkdata); 
  
  fd = open(fname,O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  g_hash_table_foreach(data->permanent,set_offset_proc,NULL);
  g_hash_table_foreach(data->permanent,save_bmark_proc,&fd);
  close(fd);
  
}

void bmark_init(void) {
	Tbmark_data *data = g_new0(Tbmark_data,1);
	gchar *fname = g_strconcat(g_get_home_dir(), "/.bluefish/bookmarks", NULL);
	
	data->store = gtk_tree_store_new(N_COLUMNS,G_TYPE_STRING,G_TYPE_POINTER);
	data->permanent = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,NULL);
	data->temporary = g_hash_table_new_full(g_int_hash,g_int_equal,g_free,NULL);
	data->file_iters_temp = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,NULL);
   data->file_iters_perm = g_hash_table_new_full(g_str_hash,g_str_equal,g_free,NULL);	
	data->lasttemp = 0;
	data->filename_mode = BM_FMODE_FILE;
/*	data->visible_bmarks = BM_VIS_ALL;*/
	main_v->bmarkdata = data;	
	load_bmarks_from_file(fname,&data->permanent);
	g_free(fname);
}

void bmark_cleanup(Tbfwin *bfwin) {
	bmark_clean_tree(bfwin);	
	BMARKGUI(bfwin->bmark)->tree = NULL;
/*	BMARKGUI(bfwin->bmark)->viewmenu = NULL;*/
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


void docset_proc(gpointer key,gpointer value,gpointer user_data)
{
  GtkTextIter it;
  Tbmark *b = BMARK(value);
  Tforeach_data *dat = (Tforeach_data*)(user_data);

  if ( b && b->filepath && !b->doc && (g_ascii_strcasecmp(dat->string,b->filepath)==0) )
		{
			b->doc = dat->doc;
			gtk_text_buffer_get_iter_at_offset(b->doc->buffer,&it,b->offset);
			b->mark = gtk_text_buffer_create_mark(b->doc->buffer,b->name,&it,TRUE); 
		}	 
}

void bmark_name_entry_changed(GtkEntry * entry, GtkDialog* dialog)
{
	const gchar *string;
	
	string = gtk_entry_get_text (GTK_ENTRY (entry));
	
	if (strlen (string) <= 0)
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_OK, FALSE);
	else
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_OK, TRUE);
}


/* Determine bookmark location */

static void bmark_get_iter_at_position(GtkTreeStore *store, Tbmark *m,GtkTreeIter *branch) {
   GtkTreeIter *parent;
   Tbmark_data *data = BMARKDATA(main_v->bmarkdata); 	   
   gpointer ptr;
   GtkTreeIter *it;
   
	if (m->is_temp)
	   ptr = g_hash_table_lookup(data->file_iters_temp,m->filepath);
	else
	   ptr = g_hash_table_lookup(data->file_iters_perm,m->filepath);   
   if ( ptr != NULL )
   {
      parent = (GtkTreeIter*)ptr;
   }
   else
   {
     it = g_new0(GtkTreeIter,1);
     gtk_tree_store_append(store,it,branch);
     switch (data->filename_mode)
     {
       case BM_FMODE_FULL:
            gtk_tree_store_set(store, it, NAME_COLUMN, g_strdup(m->filepath),PTR_COLUMN, NULL, -1);
            break;
       case BM_FMODE_HOME:
            gtk_tree_store_set(store, it, NAME_COLUMN, g_strdup(m->filepath),PTR_COLUMN, NULL, -1);
            break;
       case BM_FMODE_FILE:
            gtk_tree_store_set(store, it, NAME_COLUMN, g_strdup(g_path_get_basename(m->filepath)),PTR_COLUMN, NULL, -1);
            break;                   
     }  
     if (m->is_temp)
        g_hash_table_insert(data->file_iters_temp, g_strdup(m->filepath),it);
     else
        g_hash_table_insert(data->file_iters_perm, g_strdup(m->filepath),it);   
     parent = it;
   }
	DEBUG_MSG("bmark_get_iter_at_position, sorting=%d\n",main_v->props.bookmarks_sort);
	if (main_v->props.bookmarks_sort) {
		GtkTreeIter tmpiter;
		gboolean cont;
		/* we have to loop trough all bookmarks, we insert before 
		the first one that has a line number > then the new bookmark has */
		cont = gtk_tree_model_iter_children(GTK_TREE_MODEL(store),&tmpiter,parent);
		while (cont) {
			Tbmark *tmpm=NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(store), &tmpiter, PTR_COLUMN, &tmpm, -1);
			if (tmpm) {
			   /* not docs, because documents can be closed, filepaths rather ... Oskar */
				gint val = strcmp(m->filepath, tmpm->filepath);
				if (val == 0) {
					GtkTextIter textit1, textit2;
					/* same file, now check the offset */
					/*  the same problem - lets check offset  ... Oskar*/
				  /*	gtk_text_buffer_get_iter_at_mark(m->doc->buffer,&textit1,m->mark);
					gtk_text_buffer_get_iter_at_mark(tmpm->doc->buffer,&textit2,tmpm->mark);*/
					DEBUG_MSG("bmark_get_iter_at_position, comparing two iters\n");
					if (/*gtk_text_iter_compare(&textit1,&textit2)*/m->offset > tmpm->offset) {
						/* insert !!!!! */
						gtk_tree_store_insert_before(store,&m->iter,parent,&tmpiter);
						return;
					}
				} else if (val < 0){
					/* different file */
					gtk_tree_store_insert_before(store,&m->iter,parent,&tmpiter);
					return;
				}
			}
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(store),&tmpiter);
		}
	}
	gtk_tree_store_append(store, &m->iter, parent);
}

void bmark_add_rename_dialog(Tbfwin *bfwin, gchar *dialogtitle, gint dialogtype)
{
	GtkWidget *dlg, *name, *desc, *button, *table;
	GtkTextMark *im;	
	gint result;
	gpointer ptr;
	gchar *pstr;
	GtkTextIter it, sit, eit;
	Tbmark *m = NULL;
	Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
	Tbmark_gui *gui = BMARKGUI(bfwin->bmark); 
	 
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
		ptr = g_hash_table_lookup(data->permanent,gtk_entry_get_text(GTK_ENTRY(name)));
		if (ptr) {
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
			g_hash_table_insert(data->permanent, g_strdup(gtk_entry_get_text(GTK_ENTRY(name))), m);

			bmark_get_iter_at_position(data->store,m,&(gui->perm_branch));

			gtk_tree_store_set(data->store, &m->iter, NAME_COLUMN, g_strdup_printf("[%s] --> %s", gtk_entry_get_text(GTK_ENTRY(name)), m->text),PTR_COLUMN, m, -1);
			gtk_tree_view_expand_all(GTK_TREE_VIEW(gui->tree));
			gtk_widget_grab_focus(bfwin->current_document->view);	
			store_mark(m, NULL);
		} else {
			pstr = g_strdup(m->name);
			g_free(m->name);
			m->name = g_strdup(gtk_entry_get_text(GTK_ENTRY(name)));
			g_free(m->description);
			m->description = g_strdup(gtk_entry_get_text(GTK_ENTRY(desc)));
			if (m->is_temp) {
				m->is_temp = FALSE;
				g_hash_table_remove(data->temporary, &(m->number));
				gtk_tree_store_remove(data->store, &m->iter);
				g_hash_table_insert(data->permanent, g_strdup(m->name), m);
				
				bmark_get_iter_at_position(data->store, m, &(gui->perm_branch));
	 			/* gtk_tree_store_append(data->store, &m->iter, &(gui->perm_branch)); */
	 			gtk_tree_store_set(data->store, &m->iter, NAME_COLUMN, g_strdup_printf("[%s] --> %s",m->name,m->text),PTR_COLUMN, m, -1);		 
			}
			gtk_tree_store_set(data->store, &m->iter, NAME_COLUMN, g_strdup_printf("[%s] --> %s",m->name,m->text),-1);
			store_mark(m, pstr);
			g_free(pstr);			
		}
	}
	gtk_widget_destroy(dlg);		
}

static void bmark_popup_menu_goto_lcb(GtkWidget *widget,gpointer user_data) 
{
	Tbmark *b;
	Tdocument *dd;
	GtkTextIter it;
	gint ret;
	Tforeach_data *fa;
	Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
	gchar *btns[]={GTK_STOCK_NO,GTK_STOCK_YES,NULL};
	

	if (!user_data) return;
	b = get_current_bmark(BFWIN(user_data));
	if (!b) return;
	if (b->filepath && !b->doc)
	{
		if (!g_file_test(b->filepath,G_FILE_TEST_EXISTS))
		  {
		  	gchar *string = g_strdup_printf (_("Could not find the file \"%s\"."), b->filepath);
 			  error_dialog(BFWIN(user_data)->main_window, string, _("This bookmark is set in a file that no longer exists."));
 			  g_free (string);
 			  return;
		  }	
		ret = multi_query_dialog(BFWIN(user_data)->main_window,_("Bookmarked file is closed"), _("Do you want to open it?"), 1, 0, btns);	
		if (ret==0) return;
		dd = doc_new_with_file(BFWIN(user_data),b->filepath,TRUE,TRUE);
		/* now I have to check all bookmarks */
		fa = g_new0(Tforeach_data,1);
		fa->doc = dd;
		fa->string = b->filepath;
		g_hash_table_foreach(data->temporary,docset_proc,fa);
		g_hash_table_foreach(data->permanent,docset_proc,fa);
		g_free(fa);
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
	gint ret;
	gchar *pstr;
	Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
	gchar *btns[]={GTK_STOCK_NO,GTK_STOCK_YES,NULL};	
		
	if (!user_data) return;
	b = get_current_bmark(BFWIN(user_data));
	if (!b) return;
	/* check if it is temp mark */
	if (b->is_temp)
	{
			gtk_tree_store_remove(data->store,&(b->iter));	
			g_hash_table_remove(data->temporary,&b->number);
			bmark_free(b);
	}	
	else
	{	
		pstr = g_strdup_printf(_("Do you really want to remove %s?"),b->name);
		ret = multi_query_dialog(BFWIN(user_data)->main_window,_("Delete permanent bookmark."), pstr, 0, 0, btns);
		g_free(pstr);
		if (ret==0) return; 
		gtk_tree_store_remove(data->store,&(b->iter));	
		g_hash_table_remove(data->permanent,b->name);
		bmark_free(b);
	}
	bmark_save_all();
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

/* static void popup_menu_out(GtkWidget *widget,gpointer user_data) {
  Tbmark_gui *gui = BMARKGUI(BFWIN(user_data)->bmark);		
  tree_tips_set_enabled(gui->tips,TRUE);		  
} */

static GtkWidget *bmark_popup_menu(Tbfwin *bfwin, gpointer data) {
	GtkWidget *menu, *menu_item;
	
/*	tree_tips_set_enabled(BMARKGUI(bfwin->bmark)->tips,FALSE);*/
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
	/*menu_item = gtk_menu_item_new_with_label(_("Move"));
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(bmark_popup_menu_del_lcb), bfwin);
	gtk_menu_append(GTK_MENU(menu), menu_item);*/
	
	gtk_widget_show_all(menu);
	g_signal_connect_after(G_OBJECT(menu), "destroy", G_CALLBACK(destroy_disposable_menu_cb), menu);
/*	g_signal_connect(G_OBJECT(menu), "hide", G_CALLBACK(popup_menu_out), bfwin);*/

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

void restore_proc(gpointer key,gpointer value,gpointer data){
	Tbmark *b = BMARK(value);
	Tbfwin *bfwin = BFWIN(data);
	Tbmark_data *bd = BMARKDATA(main_v->bmarkdata);
	bmark_get_iter_at_position(bd->store, b, (b->is_temp) ? &(BMARKGUI(bfwin->bmark)->temp_branch) : &(BMARKGUI(bfwin->bmark)->perm_branch));
	gtk_tree_store_set(bd->store, &(b->iter), NAME_COLUMN,g_strdup_printf("[%s] --> %s",b->name,
								b->text),PTR_COLUMN, b, -1);		
}


/*
gchar* aa(gconstpointer win,gconstpointer tree,gint x, gint y)
{
  Tbfwin *bfwin = BFWIN(win);
  Tbmark_gui *gui = BMARKGUI(bfwin->bmark);  
  Tbmark *b;
  GtkTreePath *path;
  gchar *pstr;
  
  if ( !gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(gui->tree),x,y,&path,NULL,NULL,NULL) )
	  return NULL;
 

	b = NULL;
	if (path != NULL) {
		GValue *val;
		GtkTreeIter iter;
		gtk_tree_model_get_iter(gtk_tree_view_get_model(GTK_TREE_VIEW(BMARKGUI(bfwin->bmark)->tree)), &iter, path);
		gtk_tree_path_free(path);
		val = g_new0(GValue, 1);
		gtk_tree_model_get_value(gtk_tree_view_get_model(GTK_TREE_VIEW(BMARKGUI(bfwin->bmark)->tree)), &iter, 1, val);
		if (G_IS_VALUE(val) && g_value_fits_pointer(val)) {
			b = BMARK(g_value_peek_pointer(val));
		}
		g_value_unset(val);
		g_free(val);
	}
	
  if (!b) return NULL;

  if (b->is_temp)
  {
	 if (b->doc)
		 pstr = g_strdup_printf("<span style='italic' color='#768BEA'>Temporary</span>\nNumber: %d\nFile: %s",b->number,b->filepath);
	 else
		 pstr = g_strdup_printf("<span style='italic' color='#768BEA'>Temporary</span>\nNumber: %d\nFile: %s\n<span weight='bold'>File closed</span>",b->number,b->filepath);		 
  } 
  else
  {
	 if (b->doc)
		 pstr = g_strdup_printf("<span style='italic' color='#62CB7F'>Permanent</span>\nName: %s\nFile: %s",b->name,b->filepath);  
	 else
		 pstr = g_strdup_printf("<span style='italic' color='#62CB7F'>Permanent</span>\nNumber: %d\nFile: %s\n<span weight='bold'>File closed</span>",b->number,b->filepath);						
  } 
  return pstr;
} */


/*
void adjust_proc(gpointer key,gpointer value,gpointer user_data)
{
	Tbmark *b = BMARK(value);
	Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
	Tbfwin *bfwin = BFWIN(user_data);
	gboolean add=FALSE;
	
	switch(data->visible_bmarks)
	{
	  case BM_VIS_ALL:
		 add = TRUE;
	  break;
	  case BM_VIS_FILES:
		 if (b->doc)
			add = TRUE;		  
	  break;
	  case BM_VIS_ACTUAL:
		 if (b->doc && b->doc==bfwin->current_document)
			 add = TRUE;
	  break;	  
	}
	
	if (add) {
		bmark_get_iter_at_position(data->store, b, (b->is_temp) ? &(BMARKGUI(bfwin->bmark)->temp_branch) : &(BMARKGUI(bfwin->bmark)->perm_branch));
		gtk_tree_store_set(data->store, &(b->iter), NAME_COLUMN,g_strdup_printf("[%s] --> %s",b->name,
								b->text),PTR_COLUMN, b, -1);			
	}	
}

void bmark_adjust_visible(Tbfwin *bfwin) {
	if (bfwin->bmark) {
		Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
		g_hash_table_foreach(data->temporary,clean_proc,bfwin);
		g_hash_table_foreach(data->permanent,clean_proc,bfwin);
		g_hash_table_foreach(data->temporary,adjust_proc,bfwin);	
		g_hash_table_foreach(data->permanent,adjust_proc,bfwin);	
		gtk_tree_view_expand_all(GTK_TREE_VIEW(BMARKGUI(bfwin->bmark)->tree));
	}
}

static void bmark_visible_1_lcb(GtkWidget *widget,gpointer user_data) {
	Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
	data->visible_bmarks = BM_VIS_ALL;
	bmark_adjust_visible(user_data);  
}

static void bmark_visible_2_lcb(GtkWidget *widget,gpointer user_data) {
	Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
	data->visible_bmarks = BM_VIS_FILES;
	bmark_adjust_visible(user_data);  
}

static void bmark_visible_3_lcb(GtkWidget *widget,gpointer user_data) {
	Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
	data->visible_bmarks = BM_VIS_ACTUAL;
	bmark_adjust_visible(user_data);  
} */

GtkWidget *bmark_gui(Tbfwin *bfwin) {
	GtkWidget *vbox,*mi,*menu;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	Tbmark_data *bd = BMARKDATA(main_v->bmarkdata);
	
	bfwin->bmark = g_new(Tbmark_gui,1);
	vbox = gtk_vbox_new(FALSE,1);
	
/*	BMARKGUI(bfwin->bmark)->viewmenu = gtk_option_menu_new();
	menu = gtk_menu_new();
	mi = gtk_menu_item_new_with_label(_("Show all bookmarks"));
	g_signal_connect(GTK_OBJECT(mi), "activate", G_CALLBACK(bmark_visible_1_lcb), bfwin);			
	gtk_menu_append(GTK_MENU(menu), mi);	
	mi = gtk_menu_item_new_with_label(_("Show bookmarks for opened files"));
	g_signal_connect(GTK_OBJECT(mi), "activate", G_CALLBACK(bmark_visible_2_lcb), bfwin);
	gtk_menu_append(GTK_MENU(menu), mi);	
	mi = gtk_menu_item_new_with_label(_("Show bookmarks for active file"));
	g_signal_connect(GTK_OBJECT(mi), "activate", G_CALLBACK(bmark_visible_3_lcb), bfwin);
	gtk_menu_append(GTK_MENU(menu), mi);	
	gtk_option_menu_set_menu(GTK_OPTION_MENU(BMARKGUI(bfwin->bmark)->viewmenu),menu);
*/	
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
/*	gtk_box_pack_start(GTK_BOX(vbox),BMARKGUI(bfwin->bmark)->viewmenu,FALSE,TRUE,0);*/
	gtk_box_pack_start(GTK_BOX(vbox),BMARKGUI(bfwin->bmark)->tree,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(vbox),BMARKGUI(bfwin->bmark)->buttons,FALSE,TRUE,0);	
	
	g_signal_connect(G_OBJECT(BMARKGUI(bfwin->bmark)->tree), "button-press-event",G_CALLBACK(bmark_event_mouseclick), bfwin);
	
	/* restore bookmarks if it was after left panel hide */
	g_hash_table_foreach(bd->temporary,restore_proc,bfwin);
	g_hash_table_foreach(bd->permanent,restore_proc,bfwin);
	
	gtk_tree_view_expand_all(GTK_TREE_VIEW(BMARKGUI(bfwin->bmark)->tree));  
/*	BMARKGUI(bfwin->bmark)->tips = tree_tips_new_full(bfwin,GTK_TREE_VIEW(BMARKGUI(bfwin->bmark)->tree),aa);
	tree_tips_set_show_interval(BMARKGUI(bfwin->bmark)->tips,1000);
	tree_tips_set_hide_interval(BMARKGUI(bfwin->bmark)->tips,2000);*/
	
	return vbox;
}



void check_line_proc(gpointer key,gpointer value, gpointer user_data)
{
  Tbmark *b = BMARK(value);
  Tforeach_data *dat = (Tforeach_data*)(user_data);
  gint *iaux;
  GtkTextIter it;
  
  if (b && b->mark && b->doc==dat->doc)
  {
		iaux = g_new0(gint,1);
		gtk_text_buffer_get_iter_at_mark(b->doc->buffer,&it,b->mark);
		*iaux = gtk_text_iter_get_line(&it);
		g_hash_table_insert((GHashTable*)dat->ptr,iaux,g_strdup("+"));			 
  }
}

GHashTable *bmark_get_lines(Tdocument *doc,gboolean temp)
{
  Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
  GHashTable *ret;
  Tforeach_data *dat;
  
  
  ret = g_hash_table_new_full(g_int_hash,g_int_equal,g_free,g_free);
  dat = g_new0(Tforeach_data,1);
  dat->ptr = ret;
  dat->doc = doc;

  if (temp)
	  g_hash_table_foreach(data->temporary,check_line_proc,dat);
  else
	  g_hash_table_foreach(data->permanent,check_line_proc,dat);
  g_free(dat);
  return ret;
}


void docclean_proc(gpointer key,gpointer value,gpointer user_data)
{
	GtkTextIter it;
	Tbmark *b = BMARK(value);
	Tdocument *doc = DOCUMENT(user_data);

	if ( b && b->doc == doc )
	  {
		  gtk_text_buffer_get_iter_at_mark(b->doc->buffer,&it,b->mark);
		  b->offset =  gtk_text_iter_get_offset(&it);		
		  gtk_text_buffer_delete_mark(b->doc->buffer,b->mark);	  
		  b->mark = NULL;
		  b->doc = NULL;
	  }		
}

void bmark_clean_for_doc(Tdocument *doc)
{
	Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
	
	/* clean temporary */
	g_hash_table_foreach(data->temporary,docclean_proc,doc);
	/* clean permanent */
	g_hash_table_foreach(data->permanent,docclean_proc,doc);
}


void bmark_set_for_doc(Tdocument *doc)
{
	Tforeach_data *fa;
	Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
	
	fa = g_new0(Tforeach_data,1);
	fa->doc = doc;
	fa->string = doc->filename;
	g_hash_table_foreach(data->temporary,docset_proc,fa);	
	g_hash_table_foreach(data->permanent,docset_proc,fa);
	g_free(fa);
}




void exists_proc(gpointer key,gpointer value,gpointer user_data)
{
	GtkTextIter it;
	Tbmark *b = BMARK(value);
	Tforeach_data *dat = (Tforeach_data*)(user_data);
	
	if (b && b->mark && b->doc==DOCUMENT(BFWIN(dat->ptr)->current_document))
	  {
		 gtk_text_buffer_get_iter_at_mark(DOCUMENT(BFWIN(dat->ptr)->current_document)->buffer,&it,b->mark);	
		 if ( dat->integer == gtk_text_iter_get_offset(&it) )
			 dat->string = g_strdup("yes");
	  } 
}

void bmark_add_temp(Tbfwin *bfwin) {
	Tbmark *m;
	Tforeach_data *fa;
	GtkTextMark *im;
	GtkTextIter it,eit,sit;
	gint ret,ffree,i,*iaux;
	gboolean has_mark;
	gchar *btns[]={GTK_STOCK_NO,GTK_STOCK_YES,NULL};
	gchar *pomstr;
	gpointer ptr;
	Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
	Tbmark_gui *gui = BMARKGUI(bfwin->bmark);
	
	/* check for nameless document */
	if (!DOCUMENT(bfwin->current_document)->filename)
	{
		error_dialog(bfwin->main_window,_("Add temporary bookmark"),_("Cannot add bookmarks in nameless files.\nPlease, save the file first."));
		return;
	}

	/* check for existing bookmark in this place */
	im = gtk_text_buffer_get_insert(DOCUMENT(bfwin->current_document)->buffer);
	gtk_text_buffer_get_iter_at_mark(DOCUMENT(bfwin->current_document)->buffer,&it,im);	
	ret = gtk_text_iter_get_offset(&it);
	has_mark = FALSE;
	
	fa = g_new0(Tforeach_data,1);	
	fa->ptr = bfwin;
	fa->string = NULL;
	fa->integer = ret;
	g_hash_table_foreach(data->temporary,exists_proc,fa);
	if (fa->string) {
		has_mark = TRUE;
		g_free(fa->string);
	}	
	g_free(fa);	
	
	if (has_mark) {
		info_dialog(bfwin->main_window,_("Add temporary bookmark"),_("You already have a bookmark here!"));
		return;
	}

	/* find free number - why only 10 bookmarks ??? */
	/*
	ffree = -1;
	for(i=0;i<10;i++)	{
	  if (!g_hash_table_lookup(data->temporary,&i)) {
			ffree = i;
			break;
		 }
	}	
		
	if (ffree==-1)	{		 
		 pomstr = g_strdup_printf(_("You can overwrite next temp bookmark (number %d). Overwrite?"),data->lasttemp+1);
		 ret = multi_query_dialog(bfwin->main_window,_("Maximum number of temp bookmarks assigned"), pomstr, 0, 0, btns);
		 g_free(pomstr);
		 if (ret==0) return;
		 ffree = data->lasttemp;
		 data->lasttemp++;		 
		 if (data->lasttemp == 10) data->lasttemp = 0;			 
	} */
	ffree = data->lasttemp;
	data->lasttemp++;
	
	ptr = g_hash_table_lookup(data->temporary,&ffree);
	if ( ptr ) {
			gtk_tree_store_remove(data->store,&(BMARK(ptr)->iter));
			g_hash_table_remove(data->temporary,&ffree);
			bmark_free(ptr);			
	 }	
	m = g_new0(Tbmark,1);	
	m->doc = DOCUMENT(bfwin->current_document);
	if (!m->doc) return;
	im = gtk_text_buffer_get_insert(m->doc->buffer);
	gtk_text_buffer_get_iter_at_mark(m->doc->buffer,&it,im);
	m->mark = gtk_text_buffer_create_mark(m->doc->buffer,g_strdup_printf("%d",ffree),&it,TRUE);
	m->filepath = g_strdup(m->doc->filename);
	m->is_temp = TRUE;
	m->number = ffree;
	m->name = g_strdup_printf("%d",ffree+1);
	iaux = g_new0(gint,1);
	*iaux = ffree;
	g_hash_table_insert(data->temporary,iaux,m);
	/* add to the tree */
	sit=eit=it;
	gtk_text_iter_forward_to_line_end(&eit);
	gtk_text_iter_forward_chars(&sit,10);
	if (!gtk_text_iter_in_range(&sit,&it,&eit)) 
		sit = eit;
	m->text = g_strdup(gtk_text_iter_get_slice(&it,&sit));
	bmark_get_iter_at_position(data->store, m, &(gui->temp_branch));
/*	gtk_tree_store_insert(data->store, &m->iter, &(gui->temp_branch),ffree);*/
	gtk_tree_store_set(data->store, &m->iter, NAME_COLUMN,g_strdup_printf("[%d] --> %s",ffree+1,m->text),PTR_COLUMN, m, -1);
	gtk_tree_view_expand_all(GTK_TREE_VIEW(gui->tree));
	gtk_widget_grab_focus(bfwin->current_document->view);		
}




void bmark_del_all_temp(Tbfwin *bfwin)
{
	gint ret;
	Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
	gchar *btns[]={GTK_STOCK_NO,GTK_STOCK_YES,NULL};	
	
	ret = multi_query_dialog(bfwin->main_window,_("Delete all temporary bookmarks."), _("Are you sure?"), 0, 0, btns);
	if (ret==0) return;
	g_hash_table_foreach_remove(data->temporary,delmark_proc,NULL);
}

void bmark_del_all_perm(Tbfwin *bfwin)
{
	gint ret;
	Tbmark_data *data = BMARKDATA(main_v->bmarkdata);
	gchar *btns[]={GTK_STOCK_NO,GTK_STOCK_YES,NULL};	
	
	ret = multi_query_dialog(bfwin->main_window,_("Delete all permanent bookmarks."), _("Are you REALLY sure?"), 0, 0, btns);
	if (ret==0) return;
	g_hash_table_foreach_remove(data->permanent,delmark_proc,NULL);
	bmark_save_all();
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

void check_len_proc(gpointer key,gpointer value,gpointer user_data)
{
  Tbmark *b = BMARK(value);
  Tforeach_data *fa = (Tforeach_data*)user_data;
	
 if (b->doc && b->filepath && (b->doc == fa->doc) )
 { 
  if (b->len != gtk_text_buffer_get_char_count(b->doc->buffer))
  {
	  b->len = gtk_text_buffer_get_char_count(b->doc->buffer);
	  fa->integer=1;
  }	  
 }
}


void bmark_check_length(Tbfwin *bfwin,Tdocument *doc)
{
  gchar *pstr;
  Tforeach_data *fa;
  Tbmark_data *data = BMARKDATA(main_v->bmarkdata);

  fa = g_new0(Tforeach_data,1);
  fa->doc = doc;
  fa->integer=0;
  g_hash_table_foreach(data->permanent,check_len_proc,fa);
  if (fa->integer>0)
  {
	  pstr = g_strdup_printf(_("Character count changed in file\n %s."),doc->filename);
	  warning_dialog(bfwin->main_window,pstr, _("Bookmarks positions can be incorrect."));
	  g_free(pstr);
  }
  g_free(fa);	
}
 




