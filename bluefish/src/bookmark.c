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
#include "stringlist.h"
#include "bookmark.h"

/*
bookmarks will be loaded and saved to an arraylist (see stringlist.c). This 
is a double linked list (GList *) with pointers to string arrays (gchar **).

To have the GUI work with them, we convert those arrays (gchar **) into a 
Tbmark struct. This struct will ahve a pointer to the array (gchar **) so 
on change it can directly change the array as well, without any need to 
look it up in the list.

For the GUI, we store everything in a Gtktreestore. The treestore will have a 
pointer to a string with the name, and it will also have a pointer to the 
Tbmark. When the user clicks in the Gtktreeview widget, we can get 
immediately a pointer to the Tbmark, and that has the Gtktextmark, so that 
is very easy, and very fast!

But now we have one problem: all normal windows do share the same bookmarks list. 
So it is probably the most logical to have them store the same Gtktreestore as 
well. The best way is to have the project functions create/destroy the 
gtktreestore when they convert a window (Tbfwin) into a project window.
*/

enum {
	NAME_COLUMN,				/* bookmark name */
	PTR_COLUMN,					/* bookmark pointer */
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
	GtkTreeIter iter;			/* for tree view */
	gchar *description;
	gchar *text;
	gchar *name;
	gint len;					/* file length for integrity check - perhaps some hash code is needed */
	gboolean is_temp;
	gchar **strarr;				/* this is a pointer to the location where this bookmark is stored in the sessionlist,
								   so we can immediately change it _in_ the list */
} Tbmark;
#define BMARK(var) ((Tbmark *)(var))

typedef struct {
	Tdocument *bevent_doc;		/* last button event document */
	gint bevent_charoffset;		/* last button event location */
} Tbmarkdata;
#define BMARKDATA(var) ((Tbmarkdata *)(var))

#define BM_FMODE_FULL     0
#define BM_FMODE_HOME     1
#define BM_FMODE_FILE     2

/* Free bookmark structure */
static void bmark_free(gpointer ptr)
{
	Tbmark *m;
	if (ptr == NULL)
		return;
	m = BMARK(ptr);
	if (m->doc && m->mark) {
		DEBUG_MSG("bmark_free, deleting mark %p\n",m->mark);
		gtk_text_buffer_delete_mark(m->doc->buffer, m->mark);
		m->doc = NULL;
	}
#ifdef DEBUG
	if (m->strarr) {
		DEBUG_MSG("bmark_free, NOT GOOD, strarr should be NULL here...\n");
	}
#endif
	g_free(m->filepath);
	g_free(m->text);
	g_free(m->name);
	g_free(m->description);
	g_free(m);
}

static void bmark_update_offset_from_textmark(Tbmark *b) {
	if (b->doc && b->mark) {
		GtkTextIter it;
		gtk_text_buffer_get_iter_at_mark(b->doc->buffer, &it, b->mark);
		b->offset = gtk_text_iter_get_offset(&it);
	}
}

/* this function re-uses the b->strarr if possible, otherwise it will create a new one and
append it to the list */
static void bmark_store(Tbfwin * bfwin, Tbmark * b)
{
	gchar **strarr;
	if (b->is_temp) {
		DEBUG_MSG("bmark_store, called for temp bookmark %p ?!?! weird!!!! returning\n", b);
		return;
	}

	/* if there is a strarr already, we only update the fields, else we append a new one */
	if (b->strarr == NULL) {
		DEBUG_MSG("bmark_store, creating new strarr for bookmark %p\n",b);
		strarr = g_malloc0(sizeof(gchar *) * 7);
		DEBUG_MSG("name=%s, description=%s, filepath=%s, text=%s\n", b->name, b->description, b->filepath, b->text);
		strarr[2] = g_strdup(b->filepath);
		strarr[4] = g_strdup(b->text);
	} else {
		DEBUG_MSG("bmark_store, bookmark %p has strarr at %p\n",b,b->strarr);
		strarr = b->strarr;
		/* free the ones we are going to update */
		g_free(strarr[0]);
		g_free(strarr[1]);
		g_free(strarr[3]);
		g_free(strarr[5]);
	}
	strarr[0] = g_strdup(b->name);
	strarr[1] = g_strdup(b->description);
#ifdef HAVE_GNOME_VFS
	if (b->doc)
		b->len = b->doc->fileinfo->size;
#else
	if (b->doc)
		b->len = b->doc->statbuf.st_size;
#endif
	strarr[3] = g_strdup_printf("%d", b->offset);
	DEBUG_MSG("bmark_store, offset string=%s, offset int=%d\n",strarr[3],b->offset);
	strarr[5] = g_strdup_printf("%d", b->len);
	DEBUG_MSG("bmark_store, arracount=%d\n",count_array(strarr));
	if (b->strarr == NULL) {
		bfwin->session->bmarks = g_list_append(bfwin->session->bmarks, strarr);
		DEBUG_MSG("added new (previously unstored) bookmark to session list, list length=%d\n",
				  g_list_length(bfwin->session->bmarks));
		b->strarr = strarr;
	}
}

/* when a users want to save the project, it's good to have updated bookmarks
so this function will update all arrays (strarr**)
 */
void bmark_store_all(Tbfwin *bfwin) {
	/* we loop over all filename iters, and only for the ones that are opened
	 we loop over the children (the ones that are not open cannot be changed) */
	GtkTreeIter fileit;
	gboolean cont;

	cont = gtk_tree_model_iter_children(GTK_TREE_MODEL(BFWIN(bfwin)->bookmarkstore), &fileit,NULL);
	while (cont) {
		Tdocument *doc = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(BFWIN(bfwin)->bookmarkstore), &fileit, PTR_COLUMN,&doc, -1);
		if (doc) {
			/* the document is open, so the offsets could be changed, store all permanent */
			GtkTreeIter bmit;
			gboolean cont2 = gtk_tree_model_iter_children(GTK_TREE_MODEL(BFWIN(bfwin)->bookmarkstore), &bmit,&fileit);
			DEBUG_MSG("bmark_store_all, storing bookmarks for %s\n",doc->filename);
			while (cont2) {
				Tbmark *bmark;
				gtk_tree_model_get(GTK_TREE_MODEL(BFWIN(bfwin)->bookmarkstore), &bmit, PTR_COLUMN,&bmark, -1);
				if (!bmark->is_temp) {
					bmark_update_offset_from_textmark(bmark);
					bmark_store(bfwin, bmark);
				}
				cont2 = gtk_tree_model_iter_next(GTK_TREE_MODEL(BFWIN(bfwin)->bookmarkstore), &bmit);
			}
		} else {
			DEBUG_MSG("doc not set, so not open...\n");
		}
		cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(BFWIN(bfwin)->bookmarkstore), &fileit);
	} /* cont */
}

/* removes the bookmark from the session, removed the b->strarr pointer and frees it */
static void bmark_unstore(Tbfwin * bfwin, Tbmark * b)
{
	if (bfwin->session->bmarks == NULL || b->strarr == NULL)
		return;
	DEBUG_MSG("bmark_remove, removing bookmark %p from sessionlist\n",b);
	bfwin->session->bmarks = g_list_remove(bfwin->session->bmarks, b->strarr);
	g_strfreev(b->strarr);
	b->strarr = NULL;
}

/* get bookmark from pointer column */
static Tbmark *get_current_bmark(Tbfwin * bfwin)
{
	if (bfwin->bmark) {
		GtkTreePath *path;
		GtkTreeViewColumn *col;
		gtk_tree_view_get_cursor(bfwin->bmark, &path, &col);
		if (path != NULL) {
			Tbmark *retval = NULL;
			GtkTreeIter iter;
			gtk_tree_model_get_iter(gtk_tree_view_get_model(bfwin->bmark), &iter, path);
			gtk_tree_model_get(gtk_tree_view_get_model(bfwin->bmark),&iter,1, &retval, -1);
			gtk_tree_path_free(path);
			DEBUG_MSG("get_current_bmark, returning %p\n",retval);
			return retval;
		}
	}
	return NULL;
}
/*
void bmark_name_entry_changed(GtkEntry * entry, GtkDialog * dialog)
{
	const gchar *string;

	string = gtk_entry_get_text(GTK_ENTRY(entry));
	if (strlen(string) <= 0)
		gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, FALSE);
	else
		gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, TRUE);
}*/

void bmark_add_rename_dialog(Tbfwin * bfwin, gchar * dialogtitle)
{
	GtkWidget *dlg, *name, *desc, *button, *table, *istemp;
	gint result;
/*	GtkTextMark *im;
	gchar *pstr;
	GtkTextIter it, sit, eit;*/
	Tbmark *m = get_current_bmark(bfwin);
	if (!m) return;

	dlg =
		gtk_dialog_new_with_buttons(dialogtitle, GTK_WINDOW(bfwin->main_window), GTK_DIALOG_MODAL,
									GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	button = gtk_button_new_from_stock(GTK_STOCK_OK);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_dialog_add_action_widget(GTK_DIALOG(dlg), button, GTK_RESPONSE_OK);
/*	gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);*/
	table = gtk_table_new(2, 3, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), table, FALSE, FALSE, 12);

	name = entry_with_text(m->name, 200);
	gtk_entry_set_activates_default(GTK_ENTRY(name), TRUE);
	bf_mnemonic_label_tad_with_alignment(_("_Name:"), name, 0, 0.5, table, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), name, 1, 2, 0, 1);
/*	g_signal_connect(G_OBJECT(name), "changed", G_CALLBACK(bmark_name_entry_changed), dlg);*/
	desc = entry_with_text(m->description, 200);
	gtk_entry_set_activates_default(GTK_ENTRY(desc), TRUE);
	bf_mnemonic_label_tad_with_alignment(_("_Description:"), desc, 0, 0.5, table, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), desc, 1, 2, 1, 2);
	istemp = checkbut_with_value(_("Temporary"), m->is_temp);
	gtk_table_attach_defaults(GTK_TABLE(table), istemp, 0, 2, 2, 3);
	
	gtk_window_set_default(GTK_WINDOW(dlg), button);

	gtk_widget_show_all(dlg);
	result = gtk_dialog_run(GTK_DIALOG(dlg));

	if (result == GTK_RESPONSE_OK) {
		gchar *tmpstr;
		/* check if name exists */
/*		if (bmark_name_exists
			(DOCUMENT(bfwin->current_document), gtk_entry_get_text(GTK_ENTRY(name)))) {
			pstr =
				g_strdup_printf(_("You already have a bookmark named %s!"),
								gtk_entry_get_text(GTK_ENTRY(name)));
			if (dialogtype == BMARK_ADD_PERM_DIALOG) {
				info_dialog(bfwin->main_window, dialogtitle, pstr);
			} else {
				info_dialog(bfwin->main_window, dialogtitle, pstr);
			}
			g_free(pstr);
			gtk_widget_destroy(dlg);
			bmark_add_rename_dialog(bfwin, dialogtitle, dialogtype);
			return;
		} else */
/*		if (dialogtype == BMARK_ADD_PERM_DIALOG) {
			m = g_new0(Tbmark, 1);
			m->doc = DOCUMENT(bfwin->current_document);
			if (!m->doc)
				return;
			im = gtk_text_buffer_get_insert(m->doc->buffer);
			gtk_text_buffer_get_iter_at_mark(m->doc->buffer, &it, im);
			m->mark =
				gtk_text_buffer_create_mark(m->doc->buffer,
											g_strdup(gtk_entry_get_text(GTK_ENTRY(name))), &it,
											TRUE);
			m->filepath = g_strdup(m->doc->filename);
			m->is_temp = FALSE;
			m->name = g_strdup(gtk_entry_get_text(GTK_ENTRY(name)));
			m->description = g_strdup(gtk_entry_get_text(GTK_ENTRY(desc)));
			sit = eit = it;
			gtk_text_iter_forward_to_line_end(&eit);
			gtk_text_iter_forward_chars(&sit, 10);
			if (!gtk_text_iter_in_range(&sit, &it, &eit))
				sit = eit;
			m->text = g_strdup(gtk_text_iter_get_slice(&it, &sit));

			bmark_get_iter_at_position(bfwin, m);
			gtk_tree_store_set(bfwin->bookmarkstore, &m->iter, NAME_COLUMN,
							   g_strdup_printf("[%s] --> %s", gtk_entry_get_text(GTK_ENTRY(name)),
											   m->text), PTR_COLUMN, m, -1);
			gtk_tree_view_expand_all(bfwin->bmark);
			gtk_widget_grab_focus(bfwin->current_document->view);
			bmark_store(bfwin, m);
		} else {*/
			g_free(m->name);
			m->name = g_strdup(gtk_entry_get_text(GTK_ENTRY(name)));
			g_free(m->description);
			m->description = g_strdup(gtk_entry_get_text(GTK_ENTRY(desc)));
			m->is_temp = GTK_TOGGLE_BUTTON(istemp)->active;
			tmpstr = g_strdup_printf("[%s] --> %s", m->name, m->text);
			gtk_tree_store_set(bfwin->bookmarkstore, &m->iter, NAME_COLUMN,
							   tmpstr,-1);
			g_free(tmpstr);
			if (m->is_temp) {
				if (m->strarr) {
					/* hmm previously this was not a temporary bookmark */
					bmark_unstore(bfwin, m);
				}
			} else {
				bmark_store(bfwin, m);
			}
	/*	} */
	}
	gtk_widget_destroy(dlg);
}

static void bmark_popup_menu_goto_lcb(GtkWidget * widget, gpointer user_data)
{
	Tbmark *b;
	GtkTextIter it;

	if (!user_data)
		return;
	b = get_current_bmark(BFWIN(user_data));
	if (!b)
		return;
	if (b->filepath && !b->doc) {
		/* check if that document _is_ open */
		Tdocument *tmpdoc;
		GList *doclist = return_allwindows_documentlist();
		tmpdoc = documentlist_return_document_from_filename(doclist, b->filepath);
		g_list_free(doclist);
		if (tmpdoc == NULL) {
			if (!g_file_test(b->filepath, G_FILE_TEST_EXISTS)) {
				gchar *string = g_strdup_printf(_("Could not find the file \"%s\"."), b->filepath);
				error_dialog(BFWIN(user_data)->main_window, string,
							 _("This bookmark is set in a file that no longer exists."));
				g_free(string);
				return;
			}
			tmpdoc = doc_new_with_file(BFWIN(user_data), b->filepath, FALSE, TRUE);
		}
		/* now I have to check all bookmarks */
		bmark_set_for_doc(tmpdoc);
	}

	if (b->doc) {
		gtk_text_buffer_get_iter_at_mark(b->doc->buffer, &it, b->mark);
		gtk_text_buffer_place_cursor(b->doc->buffer, &it);
		gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(b->doc->view), b->mark);
		if (b->doc != BFWIN(user_data)->current_document)
			switch_to_document_by_pointer(BFWIN(user_data), b->doc);
		gtk_widget_grab_focus(b->doc->view);
	}
}

static void bmark_check_remove(Tbfwin *bfwin,Tbmark *b)
{
   gpointer ptr=NULL;
   gboolean re = FALSE;

   gtk_tree_store_remove(bfwin->bookmarkstore, &(b->iter));  	     
   if (bfwin->bmark_files) 
   {
      ptr = g_hash_table_lookup(bfwin->bmark_files,b->filepath);
      if (ptr!=NULL) 
       {
          if ( !gtk_tree_model_iter_has_child(GTK_TREE_MODEL(bfwin->bookmarkstore), (GtkTreeIter*)ptr))
            re = TRUE;
       }   
   }
   if (re)    
   {
  	   gtk_tree_store_remove(bfwin->bookmarkstore,(GtkTreeIter*)ptr );   
  	   g_hash_table_remove(bfwin->bmark_files,b->filepath);
  	   g_free(ptr);  	   
  	   if (b->doc)
  	      b->doc->bmark_parent = NULL;
  	}   
}

static void bmark_popup_menu_del_lcb(GtkWidget * widget, gpointer user_data)
{
	Tbmark *b;
	gint ret;
	gchar *pstr;
	gchar *btns[] = { GTK_STOCK_NO, GTK_STOCK_YES, NULL };

	if (!user_data)
		return;
	b = get_current_bmark(BFWIN(user_data));
	if (!b)
		return;
	/* check if it is temp mark */
	if (b->is_temp) {
		/* gtk_tree_store_remove(BFWIN(user_data)->bookmarkstore, &(b->iter)); */
		bmark_check_remove(BFWIN(user_data),b); /* check  if we should remove a filename too */	
		/* bmark_unstore(BFWIN(user_data), b); */
		bmark_free(b);
	} else {
		pstr = g_strdup_printf(_("Do you really want to delete %s?"), b->name);
		ret =
			multi_query_dialog(BFWIN(user_data)->main_window, _("Delete permanent bookmark."), pstr,
							   0, 0, btns);
		g_free(pstr);
		if (ret == 0)
			return;
		bmark_check_remove(BFWIN(user_data),b); /* check  if we should remove a filename too */	
		bmark_unstore(BFWIN(user_data), b);
		bmark_free(b);
		
	}
	gtk_widget_grab_focus(BFWIN(user_data)->current_document->view);
}

static void bmark_popup_menu_rename_lcb(GtkWidget * widget, Tbfwin * bfwin)
{
	Tbmark *m = get_current_bmark(bfwin);
	if (!m) return;
	bmark_add_rename_dialog(bfwin, _("Edit bookmark"));
}

static GtkWidget *bmark_popup_menu(Tbfwin * bfwin, gpointer data)
{
	GtkWidget *menu, *menu_item;

	menu = gtk_menu_new();
	menu_item = gtk_menu_item_new_with_label(_("Goto bookmark"));
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(bmark_popup_menu_goto_lcb),
					 bfwin);
	gtk_menu_append(GTK_MENU(menu), menu_item);

	menu_item = gtk_separator_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), menu_item);

	menu_item = gtk_menu_item_new_with_label(_("Edit"));
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(bmark_popup_menu_rename_lcb),
					 bfwin);
	gtk_menu_append(GTK_MENU(menu), menu_item);
	menu_item = gtk_menu_item_new_with_label(_("Delete"));
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(bmark_popup_menu_del_lcb),
					 bfwin);
	gtk_menu_append(GTK_MENU(menu), menu_item);

	gtk_widget_show_all(menu);
	g_signal_connect_after(G_OBJECT(menu), "destroy", G_CALLBACK(destroy_disposable_menu_cb), menu);

	return menu;
}


/* right mouse click */
static gboolean bmark_event_mouseclick(GtkWidget * widget, GdkEventButton * event, Tbfwin * bfwin)
{
	GtkTreePath *path;
	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(bfwin->bmark), event->x, event->y, &path, NULL, NULL, NULL)) {
		if (path) {
			gint depth = gtk_tree_path_get_depth(path);
			gtk_tree_path_free(path);
			if (depth==2) {
				if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {	/* right mouse click */
					gtk_menu_popup(GTK_MENU(bmark_popup_menu(bfwin, NULL)), NULL, NULL, NULL, NULL,
								   event->button, event->time);
				} else if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {	/* double click  */
					bmark_popup_menu_goto_lcb(NULL, bfwin);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}



/* Initialize bookmarks gui for window */

GtkWidget *bmark_gui(Tbfwin * bfwin)
{
	GtkWidget *vbox;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	DEBUG_MSG("bmark_gui, building gui for bfwin=%p\n",bfwin);
	/* Tree Store is in bfwin->bookmarkstore 
	   Tree View is in bfwin->bmark 
	 */
	vbox = gtk_vbox_new(FALSE, 1);
	bfwin->bmark = gtk_tree_view_new_with_model(GTK_TREE_MODEL(bfwin->bookmarkstore));
	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", cell, "text", NAME_COLUMN, NULL);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(bfwin->bmark), column);
	gtk_widget_show_all(bfwin->bmark);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(bfwin->bmark), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), bfwin->bmark, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(bfwin->bmark), "button-press-event",
					 G_CALLBACK(bmark_event_mouseclick), bfwin);
	gtk_tree_view_expand_all(bfwin->bmark);
	return vbox;
}




/* determine bookmark's location in the tree and  insert - result GtkTreeIter is stored in m->iter */
static void bmark_get_iter_at_position(Tbfwin * bfwin, Tbmark * m)
{
	GtkTreeIter *parent;
	gpointer ptr;
	DEBUG_MSG("bmark_get_iter_at_position, started for filepath=%s\n",m->filepath);
	if (!bfwin->bmark_files) {
		DEBUG_MSG("bmark_get_iter_at_position, creating hashtable for bfwin=%p\n",bfwin);
		bfwin->bmark_files = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	}
	ptr = g_hash_table_lookup(bfwin->bmark_files, m->filepath);
	if (ptr == NULL) {			/* closed document or bookmarks never set */
		gchar *title = NULL;
		parent = g_new0(GtkTreeIter, 1);
		gtk_tree_store_append(bfwin->bookmarkstore, parent, NULL);
		switch (main_v->props.bookmarks_filename_mode) {
		case BM_FMODE_FULL:
			g_strdup(m->filepath);
			break;
		case BM_FMODE_HOME:	/* todo */
			if (bfwin->project != NULL && bfwin->project->basedir && strlen(bfwin->project->basedir)) {
				gint baselen = strlen(bfwin->project->basedir);
				if (strncmp(m->filepath, bfwin->project->basedir, baselen)==0) {
					title = g_strdup(m->filepath + baselen);
				}
			}
			break;
/*		case BM_FMODE_FILE:
			title = g_path_get_basename(m->filepath);
			break;*/
		}
		if (title == NULL) {
			title = g_path_get_basename(m->filepath);
		}
		gtk_tree_store_set(bfwin->bookmarkstore, parent, NAME_COLUMN, title
							   , PTR_COLUMN, m->doc, -1);
		if (m->doc != NULL) {
			DEBUG_MSG("bmark_get_iter_at_position, setting parent iter %p for doc%p\n",parent,m->doc);
			m->doc->bmark_parent = parent;
		}
		DEBUG_MSG("bmark_get_iter_at_position, appending parent %p in hashtable for filepath=%s\n",parent, m->filepath);
		/* the hash table frees the key, but not the value, on destroy */
		g_hash_table_insert(bfwin->bmark_files, g_strdup(m->filepath), parent);
	} else
		parent = (GtkTreeIter *) ptr;

	DEBUG_MSG("bmark_get_iter_at_position, sorting=%d\n", main_v->props.bookmarks_sort);
	if (main_v->props.bookmarks_sort) {
		GtkTreeIter tmpiter;
		gboolean cont;
		cont = gtk_tree_model_iter_children(GTK_TREE_MODEL(bfwin->bookmarkstore), &tmpiter, parent);
		while (cont) {
			Tbmark *tmpm = NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(bfwin->bookmarkstore), &tmpiter, PTR_COLUMN, &tmpm,
							   -1);
			if (tmpm) {
				gint val = strcmp(m->filepath, tmpm->filepath);
				if (val == 0) {
					DEBUG_MSG("bmark_get_iter_at_position, there is already a bookmark for this file, comparing two iters\n");
					if (m->offset < tmpm->offset) {
						gtk_tree_store_insert_before(bfwin->bookmarkstore, &m->iter, parent,
													 &tmpiter);
						return;
					}
				} else if (val < 0) {
					/* different file */
					DEBUG_MSG("bmark_get_iter_at_position, no bookmark for this file yet, creating\n");
					gtk_tree_store_insert_before(bfwin->bookmarkstore, &m->iter, parent, &tmpiter);
					return;
				}
			}
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(bfwin->bookmarkstore), &tmpiter);
		}
	}
	gtk_tree_store_append(bfwin->bookmarkstore, &m->iter, parent);
}

/*
because we don't store them in a hashtable anymore, it's no problem to have 
bookmarks with the same name
static gboolean bmark_name_exists(Tdocument * doc, const gchar * name)
{
	GtkTreeIter tmpiter;
	gboolean cont;

	if (doc->bmark_parent == NULL)
		return FALSE;
	cont =
		gtk_tree_model_iter_children(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &tmpiter,
									 doc->bmark_parent);
	while (cont) {
		Tbmark *mark = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &tmpiter, PTR_COLUMN,
						   &mark, -1);
		if (mark) {
			if (strcmp(mark->name, name) == 0)
				return TRUE;
		}
		cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &tmpiter);
	}							
	return FALSE;
}
*/



/*
 * this function should create the global
 * main_v->bookmarkstore
 */
void bmark_init()
{
	main_v->bookmarkstore = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
	DEBUG_MSG("bmark_init, created main_v->bookmarkstore at %p\n", main_v->bookmarkstore);
	main_v->bmarkdata = g_new0(Tbmarkdata, 1);
}


/* this function will load the bookmarks
 * from bfwin->session->bmarks and parse
 * them into treestore bfwin->bookmarkstore
 *
 * this function should ALSO check all dcouments that are
 * opened (bfwin->documentlist) if they have bookmarks !!
 */
void bmark_reload(Tbfwin * bfwin)
{
	GList *tmplist = g_list_first(bfwin->session->bmarks);
	DEBUG_MSG("bmark_reload for bfwin %p\n",bfwin);
	if (bfwin->bmark_files != NULL)
		g_hash_table_destroy(bfwin->bmark_files);
	bfwin->bmark_files = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	while (tmplist) {
		gchar **items = (gchar **) tmplist->data;
		if (items && count_array(items) == 6) {
			gchar *ptr;
			Tbmark *b;
			b = g_new0(Tbmark, 1);
			b->name = g_strdup(items[0]);
			b->description = g_strdup(items[1]);
			b->filepath = g_strdup(items[2]);
			b->offset = atoi(items[3]);
			b->text = g_strdup(items[4]);
			b->len = atoi(items[5]);
			b->strarr = items;
			bmark_get_iter_at_position(bfwin, b);
			ptr = g_strdup_printf("[%s] --> %s", b->name, b->text);
			gtk_tree_store_set(bfwin->bookmarkstore, &(b->iter), NAME_COLUMN, ptr, PTR_COLUMN, b,
							   -1);
			g_free(ptr);
		}
		tmplist = g_list_next(tmplist);
	}
	
	tmplist = g_list_first(bfwin->documentlist);
	while (tmplist) {
		DEBUG_MSG("bmark_reload, calling bmark_set_for_doc for doc=%p\n",tmplist->data);
		bmark_set_for_doc(DOCUMENT(tmplist->data));
		tmplist = g_list_next(tmplist);
	}
}

/*
 * this function will simply call
 * gtk_tree_view_set_model() to connect the treeview
 * to the new treestore, used in unloading and
 * loading of projects
 */
void bmark_set_store(Tbfwin * bfwin)
{
	DEBUG_MSG("bmark_set_store set store %p for bfwin %p\n",bfwin->bookmarkstore,bfwin);
	if (bfwin->bookmarkstore && bfwin->bmark) {
		gtk_tree_view_set_model(bfwin->bmark, GTK_TREE_MODEL(bfwin->bookmarkstore));
	}
}

void bmark_clean_for_doc(Tdocument * doc)
{
	GtkTreeIter tmpiter;
	gboolean cont;

	if (doc->bmark_parent == NULL)
		return;
	DEBUG_MSG("bmark_clean_for_doc, getting children for parent_iter=%p\n",doc->bmark_parent);
	cont =
		gtk_tree_model_iter_children(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &tmpiter,
									 doc->bmark_parent);
	while (cont) {
		Tbmark *b = NULL;
		DEBUG_MSG("bmark_clean_for_doc, getting bookmark for first child\n");
		gtk_tree_model_get(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &tmpiter, PTR_COLUMN,
						   &b, -1);
		if (b) {
			bmark_update_offset_from_textmark(b);
			DEBUG_MSG("bmark_clean_for_doc, bookmark=%p, new offset=%d\n",b,b->offset);
			gtk_text_buffer_delete_mark(doc->buffer, b->mark);
			b->mark = NULL;
			b->doc = NULL;
			if (!b->is_temp) {
				bmark_store(doc->bfwin, b);
			}
		}
		cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &tmpiter);
	}							/* cont */
	/* now unset the Tdocument* in the second column */
	DEBUG_MSG("unsetting and freeing parent_iter %p for doc%p\n",doc->bmark_parent,doc);
	gtk_tree_store_set(GTK_TREE_STORE(BFWIN(doc->bfwin)->bookmarkstore), doc->bmark_parent, PTR_COLUMN, NULL, -1);
	g_free(doc->bmark_parent);
	doc->bmark_parent = NULL;
}

/*
 * this function will check is this document needs any bookmarks, and set the
 * doc->bmark_parent if needed
 */
void bmark_set_for_doc(Tdocument * doc)
{
	GtkTreeIter tmpiter;
	GtkTextIter it;
	gboolean cont;
	DEBUG_MSG("bmark_set_for_doc, doc=%p\n",doc);
	if (!doc->filename) {
		DEBUG_MSG("bmark_set_for_doc, an unnamed document cannot have bookmarks, returning\n");
		return;
	}
	if (!BFWIN(doc->bfwin)->bmark) {
		DEBUG_MSG("bmark_set_for_doc, no leftpanel, not implemented yet!!\n");
		return;
	}
	if (doc->bmark_parent) {
		DEBUG_MSG("this document (%p) already has a bmark_parent (%p) why is this function called?\n",doc,doc->bmark_parent);
		return;
	}
	
	cont =
		gtk_tree_model_iter_children(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &tmpiter,
									 NULL);
	while (cont) {
		GtkTreeIter child;
		if (gtk_tree_model_iter_children
			(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &child, &tmpiter)) {
			Tbmark *mark = NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &child, PTR_COLUMN,
							   &mark, -1);
			if (mark) {
				if (strcmp(mark->filepath, doc->filename) == 0) {	/* this is it */
					gboolean cont2;
					DEBUG_MSG("bmark_set_for_doc, we found a bookmark for document %s at offset=%d!\n",doc->filename,mark->offset);
					/* we will now first set the Tdocument * into the second column of the parent */
					gtk_tree_store_set(GTK_TREE_STORE(BFWIN(doc->bfwin)->bookmarkstore), &tmpiter, PTR_COLUMN, doc, -1);
					
					mark->doc = doc;
					gtk_text_buffer_get_iter_at_offset(doc->buffer, &it, mark->offset);
					mark->mark = gtk_text_buffer_create_mark(doc->buffer, NULL, &it, TRUE);
					cont2 =
						gtk_tree_model_iter_next(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore),
												 &child);
					while (cont2) {
						mark = NULL;
						gtk_tree_model_get(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &child,
										   PTR_COLUMN, &mark, -1);
						if (mark) {
							mark->doc = doc;
							DEBUG_MSG("bmark_set_for_doc, next bookmark at offset=%d!\n",mark->offset);
							gtk_text_buffer_get_iter_at_offset(doc->buffer, &it, mark->offset);
							mark->mark =
								gtk_text_buffer_create_mark(doc->buffer, NULL, &it, TRUE);
						}
						cont2 =
							gtk_tree_model_iter_next(GTK_TREE_MODEL
													 (BFWIN(doc->bfwin)->bookmarkstore), &child);
					}
					doc->bmark_parent = g_memdup(&tmpiter, sizeof(GtkTreeIter));
					DEBUG_MSG("bmark_set_for_doc, added parent_iter %p to doc %p\n",doc->bmark_parent,doc);
					break;
				}
			}
		}
		cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &tmpiter);
	}							/* cont */
}

/* this function is called VERY OFTEN (might be 20X per second!!!!) by document.c
so we obviously need to keep this function VERY FAST 

the function will return NULL if no bookmarks for this document are 
known (this is faster then looking in an empty hash table)
*/
GHashTable *bmark_get_bookmarked_lines(Tdocument * doc, GtkTextIter *fromit, GtkTextIter *toit) {
	if (doc->bmark_parent && BFWIN(doc->bfwin)->bmark) {
		gboolean cont;
		GtkTreeIter tmpiter;

		GHashTable *ret = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, g_free);
		cont = gtk_tree_model_iter_children(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), 
									&tmpiter, doc->bmark_parent);
		while (cont) {
			Tbmark *mark;
			gtk_tree_model_get(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &tmpiter, PTR_COLUMN,
							   &mark, -1);
			if (mark && mark->mark) {
				GtkTextIter it;
				gint *iaux;
				gtk_text_buffer_get_iter_at_mark(doc->buffer, &it, mark->mark);
				if (gtk_text_iter_in_range(&it,fromit,toit)) {
					iaux = g_new(gint, 1);
					*iaux = gtk_text_iter_get_line(&it);
					g_hash_table_insert(ret, iaux, g_strdup(mark->is_temp ? "1" : "0"));
				}
			}
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &tmpiter);
		} /* cont */
		return ret;
	}
	return NULL;
}

void bmark_add_backend(Tbfwin *bfwin, const gchar *name, gint offset, gboolean is_temp) {
	Tbmark *m;
	GtkTextIter it, eit, sit;
	DEBUG_MSG("bmark_add_backend, adding bookmark at offset=%d for bfwin=%p\n",offset,bfwin);
	/* create bookmark */
	m = g_new0(Tbmark, 1);
	m->doc = DOCUMENT(bfwin->current_document);
	m->offset = offset;
	gtk_text_buffer_get_iter_at_offset(m->doc->buffer,&it,offset);
	m->mark = gtk_text_buffer_create_mark(m->doc->buffer, NULL, &it, TRUE);
	m->filepath = g_strdup(m->doc->filename);
	m->is_temp = is_temp;
	m->name = g_strdup(name);
	m->description = g_strdup("");
	sit = eit = it;
	gtk_text_iter_forward_to_line_end(&eit);
	gtk_text_iter_forward_chars(&sit, 10);
	if (!gtk_text_iter_in_range(&sit, &it, &eit))
		sit = eit;
	m->text = gtk_text_iter_get_text(&it, &sit);
	/* insert into tree */
	bmark_get_iter_at_position(bfwin, m);
	gtk_tree_store_set(bfwin->bookmarkstore, &m->iter, NAME_COLUMN,
					   g_strdup_printf("[%s] --> %s", m->name, m->text), PTR_COLUMN, m, -1);
	gtk_tree_view_expand_all(bfwin->bmark);
	gtk_widget_grab_focus(bfwin->current_document->view);
	/* and store */
	if (!m->is_temp) {
		bmark_store(bfwin, m);
	}
}

void bmark_add(Tbfwin * bfwin)
{
	GtkTreeIter tmpiter;
	GtkTextMark *im;
	GtkTextIter it;
	gint offset;
	/* check for unnamed document */
	if (!DOCUMENT(bfwin->current_document)->filename) {
		error_dialog(bfwin->main_window, _("Add bookmark"),
					 _("Cannot add bookmarks in unnamed files."));
					 /*\nPlease save the file first. A Save button in this dialog would be cool -- Alastair*/
		return;
	}
	/* if the left panel is disabled, we simply should add the bookmark to the list, and do nothing else */
	if (bfwin->bmark == NULL) {
		DEBUG_MSG("no left panel, this is not implemented yet\n");
	} else {
		im = gtk_text_buffer_get_insert(DOCUMENT(bfwin->current_document)->buffer);
		gtk_text_buffer_get_iter_at_mark(DOCUMENT(bfwin->current_document)->buffer, &it, im);
		offset = gtk_text_iter_get_offset(&it);
	
		/* check for existing bookmark in this place */
		if (DOCUMENT(bfwin->current_document)->bmark_parent) {
			gboolean has_mark, cont;
			has_mark = FALSE;
			cont =
				gtk_tree_model_iter_children(GTK_TREE_MODEL(bfwin->bookmarkstore), &tmpiter,
											 DOCUMENT(bfwin->current_document)->bmark_parent);
			while (cont) {
				Tbmark *m = NULL;
				gtk_tree_model_get(GTK_TREE_MODEL(bfwin->bookmarkstore), &tmpiter, PTR_COLUMN, &m, -1);
				if (m && m->mark) {
					GtkTextIter eit;
					gtk_text_buffer_get_iter_at_mark(DOCUMENT(bfwin->current_document)->buffer, &eit,
													 m->mark);
					if (gtk_text_iter_get_offset(&eit) == offset) {
						has_mark = TRUE;
						break;
					}
				}
				cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(bfwin->bookmarkstore), &tmpiter);
			}							/* cont */
			if (has_mark) {
				info_dialog(bfwin->main_window, _("Add bookmark"),
							_("You already have a bookmark here!"));
				return;
			}
		}
	
		bmark_add_backend(bfwin, _("default"), offset, !main_v->props.bookmarks_default_store);
	}
}

/*void bmark_add_perm(Tbfwin * bfwin)
{
	/ * check for nameless document * /
	if (!DOCUMENT(bfwin->current_document)->filename) {
		error_dialog(bfwin->main_window, _("Add permanent bookmark"),
					 _("Cannot add bookmarks in nameless files.\nPlease, save the file first."));
		return;
	}
	bmark_add_rename_dialog(bfwin, _("Add permanent bookmark"), BMARK_ADD_PERM_DIALOG);
}*/

void bmark_store_bevent_location(Tdocument * doc, gint charoffset)
{
	DEBUG_MSG("bmark_store_bevent_location, storing offset=%d for doc=%p\n",charoffset,doc);
	BMARKDATA(main_v->bmarkdata)->bevent_doc = doc;
	BMARKDATA(main_v->bmarkdata)->bevent_charoffset = charoffset;
}

void bmark_add_at_bevent(Tdocument *doc) 
{
	if (BMARKDATA(main_v->bmarkdata)->bevent_doc == doc) {
		gint offset = BMARKDATA(main_v->bmarkdata)->bevent_charoffset;
		/* we have the location */
		if (BFWIN(doc->bfwin)->bmark == NULL) {
			DEBUG_MSG("adding bookmarks without left panel is not implemented yet\n");
		} else {
			bmark_add_backend(doc->bfwin, _("temp"), offset, !main_v->props.bookmarks_default_store);
		}
	}
}

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



void bmark_check_length(Tbfwin * bfwin, Tdocument * doc)
{
	gchar *pstr;
	GtkTreeIter tmpiter;
	gboolean cont;
	glong size;
	DEBUG_MSG("bmark_check_length, started\n");
	if (!doc || !doc->bmark_parent)
		return;

	cont =
		gtk_tree_model_iter_children(GTK_TREE_MODEL(bfwin->bookmarkstore), &tmpiter,
									 doc->bmark_parent);
	while (cont) {
		Tbmark *mark = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bookmarkstore), &tmpiter, PTR_COLUMN,
						   &mark, -1);
		if (mark) {
#ifdef HAVE_GNOME_VFS
			size = doc->fileinfo->size;
#else
			size = doc->statbuf.st_size;
#endif
			DEBUG_MSG("bmark_check_length, bmark has %d, file has %ld\n",mark->len, size);
			if (mark->len != size) {
				pstr = g_strdup_printf(_("Character count changed in file\n %s."), doc->filename);
				warning_dialog(bfwin->main_window, pstr,
							   _("Bookmarks positions could be incorrect."));
				g_free(pstr);
				return;
			}
		} else {
			DEBUG_MSG("bmark_check_length, NOT GOOD no mark in the treestore??\n");
		}
		
		cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(bfwin->bookmarkstore), &tmpiter);
	}
}

void bmark_cleanup(Tbfwin * bfwin)
{
	DEBUG_MSG("bmark_cleanup, cleanup for bfwin=%p\n",bfwin);
/* we are not destroying a store, so let's not destroy tree positions, it seems, they 
   are recovered from store - Oskar */	
/*	if (bfwin->bmark_files) g_hash_table_destroy(bfwin->bmark_files);
	bfwin->bmark_files = NULL; */ 

	bfwin->bmark = NULL;
}
