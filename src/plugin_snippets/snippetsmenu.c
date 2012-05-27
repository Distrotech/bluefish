/* Bluefish HTML Editor
 * snippetsmenu.c - menubar that represents the treemodel as menu
 *
 * Copyright (C) 2009 Olivier Sessink
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*#define DEBUG*/
#include <gtk/gtk.h>
#include "../bluefish.h"
#include "snippetsmenu.h"

#if !GTK_CHECK_VERSION(2,22,0)
/* this is an ugly hack to expose the treepath internals to this widget */
/*typedef struct {
	gint depth;
	gint *indices;	
} TreePath;

#define TREEPATH(var) ((TreePath *)(var))
*/
static gint *
gtk_tree_path_get_indices_with_depth(GtkTreePath *path, gint *depth)
{
/*	*depth = TREEPATH(path)->depth;
	DEBUG_MSG("get_indices_with_depth, return depth %d and indices %p\n",*depth, TREEPATH(depth)->indices);
	if (*depth > 0)
		DEBUG_MSG("indices[0]=%d\n",TREEPATH(depth)->indices[0]);
	return TREEPATH(depth)->indices;*/
	*depth = gtk_tree_path_get_depth(path);
	return gtk_tree_path_get_indices(path);
}
#endif

static GtkMenuItem *menushell_nth_child(GtkMenuShell *menushell, guint n) {
	GtkMenuItem *menuitem;
	GList *list = gtk_container_get_children(GTK_CONTAINER(menushell));
	menuitem = g_list_nth_data(list, n);
	g_list_free(list);
	/*g_print("menushell_nth_child, return child %p for n=%d\n",menuitem,n);*/
	return menuitem;
}


static GtkMenuItem *menuitem_from_path(SnippetsMenu *sm, GtkTreePath *path) {
	gint i;
	GtkMenuItem *mitem=NULL;
	GtkMenuShell *mshell = (GtkMenuShell *)sm;
	gint *indices, depth;
	if (!path) {
		DEBUG_MSG("menuitem_from_path, path=NULL, return NULL\n");
		return NULL;
	}
	indices = gtk_tree_path_get_indices_with_depth(path, &depth);
	DEBUG_MSG("menuitem_from_path, depth=%d\n",depth);
	for (i=0;mshell && i<depth;i++) {
		/* all menu's have a tearoff entry as first entry, except the main menu */
		mitem = menushell_nth_child(mshell, i==0?indices[i]:indices[i]+1);
		if (mitem)
			mshell = (GtkMenuShell *)gtk_menu_item_get_submenu(mitem);
		else 
			mshell = NULL;
	}
	DEBUG_MSG("indices[%d]=%d mitem=%p, mshell=%p\n",i,indices[i],mitem, mshell);
	/*g_print("return mitem=%p with label %s\n", mitem, gtk_label_get_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(mitem)))));*/
	return mitem;
}

static void snippets_menu_row_inserted(GtkTreeModel * tree_model,
									   GtkTreePath * path, GtkTreeIter * iter, gpointer user_data)
{
	SnippetsMenu *sm = user_data;
	GtkMenuItem *item, *newitem;
	GtkTreePath *parent;
	gint *indices, depth;
	DEBUG_MSG("row inserted, sm=%p, path=%s\n", sm, gtk_tree_path_to_string(path));
	parent = gtk_tree_path_copy(path);
	indices = gtk_tree_path_get_indices_with_depth(path, &depth);
	if (!gtk_tree_path_up(parent) || gtk_tree_path_get_depth(parent)==0) {
		GtkRequisition req;
		/* main menu entry ! */
#if GTK_CHECK_VERSION(3,0,0)
		gtk_widget_get_preferred_size((GtkWidget *) sm, &req, NULL);
#else
		gtk_widget_size_request((GtkWidget *)sm,&req);
#endif
		DEBUG_MSG("have %d pixels in use, %d available\n",req.width, sm->maxwidth);
		if (req.width < (sm->maxwidth-100)) { /* reserve at least 100 pixels for any new entry */
			if (!indices)
				g_warning("indices should not be NULL!!!!!!!!!\n");
			newitem = (GtkMenuItem *) gtk_menu_item_new_with_label("");
			DEBUG_MSG("insert menu item %p at position %d\n",newitem, indices[0]);
			gtk_menu_shell_insert((GtkMenuShell *)sm, (GtkWidget *)newitem, indices[0]);
			gtk_widget_show((GtkWidget *)newitem);
		}
	} else {
		GtkMenuShell *mshell;
		item = menuitem_from_path(sm, parent);
		if (item) {
			mshell = (GtkMenuShell *)gtk_menu_item_get_submenu(item);
			DEBUG_MSG("row inserted, item=%p, mshell=%p\n",item, mshell);
			if (!mshell) {
				GtkMenuItem *tearoff;
				mshell = (GtkMenuShell *)gtk_menu_new();
				DEBUG_MSG("append mshell %p to item %p\n",mshell, item);
				gtk_menu_item_set_submenu(item, (GtkWidget *)mshell);
				tearoff = (GtkMenuItem *) gtk_tearoff_menu_item_new();
				gtk_menu_shell_insert((GtkMenuShell *)mshell, (GtkWidget *)tearoff, 0);
				gtk_widget_show((GtkWidget *)tearoff);
			}
			DEBUG_MSG("row inserted, insert in mshell=%p at position %d\n",mshell, indices[depth-1]+1);
			newitem = (GtkMenuItem *) gtk_menu_item_new_with_label("");
			/* add 1 to the index number for the tearoff item */
			gtk_menu_shell_insert((GtkMenuShell *)mshell, (GtkWidget *)newitem, indices[depth-1]+1);
			gtk_widget_show((GtkWidget *)newitem);
		}
	}
	gtk_tree_path_free(parent);
}

/*static void snippets_menu_rows_reordered(GtkTreeModel * tree_model,
										 GtkTreePath * path,
										 GtkTreeIter * iter, gint *neworder, gpointer user_data)
{
	/ * an array of integers mapping the current position of each child to its 
	old position before the re-ordering, i.e. neworder[newpos] = oldpos. * /
	
	DEBUG_MSG("TODO rows reordered\n");
}

static void snippets_menu_row_has_child_toggled(GtkTreeModel * tree_model,
												GtkTreePath * path, GtkTreeIter * iter, gpointer user_data)
{
	DEBUG_MSG("todo? or nothing todo? row has child toggled\n");
}
*/
static void snippets_menu_row_deleted(GtkTreeModel * tree_model, GtkTreePath * path, gpointer user_data)
{
	SnippetsMenu *sm = user_data;
	GtkMenuItem *mitem;
	DEBUG_MSG("row deleted, sm=%p\n",sm);
	mitem = menuitem_from_path(sm, path);
	if (mitem) {
		gtk_widget_destroy((GtkWidget *)mitem);
	} else {
		DEBUG_MSG("row deleted, no mitem for path %s\n",gtk_tree_path_to_string(path));	
	}
}

typedef struct {
	SnippetsMenu *sm;
	gpointer pointer;
} Tsmdata;

static void menuitem_activate(GtkMenuItem *mitem, gpointer user_data) {
	Tsmdata *smdata=user_data;
	smdata->sm->callback(smdata->sm->user_data, smdata->pointer);
}

static void smdata_free(gpointer smdata) {
	g_slice_free(Tsmdata, smdata);
}

static void snippets_menu_row_changed(GtkTreeModel * tree_model,
									  GtkTreePath * path, GtkTreeIter * iter, gpointer user_data)
{
	SnippetsMenu *sm = user_data;
	GtkMenuItem *mitem;
	DEBUG_MSG("snippets_menu_row_changed, sm=%p\n",sm);
	mitem = menuitem_from_path(sm, path);
	if (mitem) {
		gchar *name=NULL;
		gpointer pointer;
		Tsmdata *smdata;
		gtk_tree_model_get(tree_model, iter, sm->name_column, &name, sm->data_column, &pointer, -1);
		if (gtk_bin_get_child(GTK_BIN(mitem))) {
			g_signal_handlers_disconnect_matched(mitem, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, menuitem_activate, NULL);
			gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(mitem))),name);
			g_free(name);
		}
		smdata = g_object_get_data(G_OBJECT(mitem),"smdata");
		if (!smdata) {
			smdata = g_slice_new(Tsmdata);
			smdata->sm = sm;
			g_object_weak_ref(G_OBJECT(mitem),(GWeakNotify) smdata_free,smdata);
			g_object_set_data(G_OBJECT(mitem),"smdata",smdata);
		}
		smdata->pointer = pointer;
		g_signal_connect(mitem, "activate", G_CALLBACK(menuitem_activate), smdata);
	} else {
		DEBUG_MSG("row changed, no mitem for path %s\n",gtk_tree_path_to_string(path));	
	}
}

gboolean snippets_menu_set_model_foreach(GtkTreeModel *model,GtkTreePath *path,GtkTreeIter *iter,gpointer data) {
	DEBUG_MSG("snippets_menu_set_model_foreach, sm=%p\n",data);
	snippets_menu_row_inserted(model,path,iter, data);
	snippets_menu_row_changed(model,path, iter, data);
	return FALSE;
}
void snippets_menu_set_model(SnippetsMenu * sm, GtkTreeModel * model, SnippetMenuCallback callback, gpointer user_data, gint name_column, gint data_column)
{
	sm->name_column = name_column;
	sm->data_column = data_column;
	sm->callback = callback;
	sm->user_data = user_data;
	g_signal_connect(model, "row-changed", G_CALLBACK(snippets_menu_row_changed), sm);
	g_signal_connect(model, "row-deleted", G_CALLBACK(snippets_menu_row_deleted), sm);
	/*g_signal_connect(model, "row-has-child-toggled", G_CALLBACK(snippets_menu_row_has_child_toggled), sm);
	g_signal_connect(model, "rows-reordered", G_CALLBACK(snippets_menu_rows_reordered), sm);*/
	g_signal_connect(model, "row-inserted", G_CALLBACK(snippets_menu_row_inserted), sm);
	
	gtk_tree_model_foreach(model, snippets_menu_set_model_foreach, sm);
}


/************ widget stuff *****************/

G_DEFINE_TYPE(SnippetsMenu, snippets_menu, GTK_TYPE_MENU_BAR)

static void snippets_menu_finalize(GObject * object)
{
	DEBUG_MSG("snippets_menu_finalize, finalize\n");
	if (G_OBJECT_CLASS(snippets_menu_parent_class)->finalize)
		G_OBJECT_CLASS(snippets_menu_parent_class)->finalize(object);
}

static void snippets_menu_class_init(SnippetsMenuClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	/*GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);*/
	object_class->finalize = snippets_menu_finalize;
}

static void snippets_menu_init(SnippetsMenu * sm)
{

}

GtkWidget *snippets_menu_new(gint maxwidth)
{
	SnippetsMenu *sm = (SnippetsMenu *) g_object_new(SNIPPETS_TYPE_MENU, NULL);
	DEBUG_MSG("snippets_menu_new, sm=%p\n",sm);
	g_return_val_if_fail(sm != NULL, NULL);
	sm->maxwidth = maxwidth;
	return GTK_WIDGET(sm);
}
