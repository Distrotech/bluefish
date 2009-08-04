#include <gtk/gtk.h>
#include "snippetsmenu.h"

static GtkMenuItem *menushell_nth_child(GtkMenuShell *menushell, guint n) {
	GtkMenuItem *menuitem;
	GList *list = gtk_container_get_children(GTK_CONTAINER(menushell));
	menuitem = g_list_nth_data(list, n);
	g_list_free(list);
	g_print("menushell_nth_child, return child %p for n=%d\n",menuitem,n);
	return menuitem;
}

/* this is an ugly hack to expose the treepath internals to this widget */
typedef struct {
	gint depth;
	gint *indices;	
} TreePath;

#define TREEPATH(var) ((TreePath *)(var))



static GtkMenuItem *menuitem_from_path(SnippetsMenu *sm, GtkTreePath *path) {
	gint i;
	GtkMenuItem *mitem=NULL;
	GtkMenuShell *mshell = (GtkMenuShell *)sm;
	if (!path) {
		g_print("menuitem_from_path, path=NULL, return\n");
		return NULL;
	}
	g_print("menuitem_from_path, depth=%d\n",TREEPATH(path)->depth);
	for (i=0;mshell && i<TREEPATH(path)->depth;i++) {
		mitem = menushell_nth_child(mshell, TREEPATH(path)->indices[i]);
		if (mitem) {
			mshell = (GtkMenuShell *)gtk_menu_item_get_submenu(mitem);
		}
		g_print("indices[%d]=%d mitem=%p, mshell=%p\n",i,TREEPATH(path)->indices[i],mitem, mshell);
	}
	/*g_print("return mitem=%p with label %s\n", mitem, gtk_label_get_text(GTK_LABEL(GTK_BIN(mitem)->child)));*/
	return mitem;
}

static void snippets_menu_row_inserted(GtkTreeModel * tree_model,
									   GtkTreePath * path, GtkTreeIter * iter, gpointer user_data)
{
	SnippetsMenu *sm = user_data;
	GtkMenuItem *item;
	GtkTreePath *parent;
	g_print("row inserted, path=%s\n", gtk_tree_path_to_string(path));
	parent = gtk_tree_path_copy(path);
	if (!gtk_tree_path_up(parent) || gtk_tree_path_get_depth(parent)==0) {
		/* main menu entry ! */
		gtk_menu_shell_insert((GtkMenuShell *)sm, gtk_menu_item_new(), TREEPATH(path)->indices[0]);
	} else {
		GtkMenuShell *mshell;
		item = menuitem_from_path(sm, parent);
		mshell = (GtkMenuShell *)gtk_menu_item_get_submenu(item);
		g_print("row inserted, item=%p, mshell=%p\n",item, mshell);
		if (!mshell) {
			mshell = (GtkMenuShell *)gtk_menu_new();
			g_print("append mshell %p to item %p\n",mshell, item);
			gtk_menu_item_set_submenu(item, (GtkWidget *)mshell);
		}
		g_print("row inserted, insert in mshell=%p at position %d\n",mshell, TREEPATH(path)->indices[TREEPATH(path)->depth-1]);
		gtk_menu_shell_insert((GtkMenuShell *)mshell, gtk_menu_item_new(), TREEPATH(path)->indices[TREEPATH(path)->depth-1]);		
	}
	gtk_tree_path_free(parent);
	
	
/*	gtk_menu_shell_append((GtkMenuShell *) sm, gtk_menu_item_new_with_label("blajaja"));*/
	gtk_widget_show_all((GtkWidget *)sm);
}

static void snippets_menu_rows_reordered(GtkTreeModel * tree_model,
										 GtkTreePath * path,
										 GtkTreeIter * iter, gint *neworder, gpointer user_data)
{
	/* an array of integers mapping the current position of each child to its 
	old position before the re-ordering, i.e. neworder[newpos] = oldpos. */
	
	g_print("TODO rows reordered\n");
}

static void snippets_menu_row_has_child_toggled(GtkTreeModel * tree_model,
												GtkTreePath * path, GtkTreeIter * iter, gpointer user_data)
{
	g_print("todo? or nothing todo? row has child toggled\n");
}

static void snippets_menu_row_deleted(GtkTreeModel * tree_model, GtkTreePath * path, gpointer user_data)
{
	SnippetsMenu *sm = user_data;
	GtkMenuItem *mitem;
	g_print("row deleted\n");
	mitem = menuitem_from_path(sm, path);
	if (mitem) {
		gtk_widget_destroy((GtkWidget *)mitem);
	} else {
		g_print("row deleted, no mitem for path %s\n",gtk_tree_path_to_string(path));	
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
	g_print("row changed\n");
	mitem = menuitem_from_path(sm, path);
	if (mitem) {
		gchar *name=NULL;
		gpointer pointer;
		Tsmdata *smdata;
		gtk_tree_model_get(tree_model, iter, sm->name_column, &name, sm->data_column, &pointer, -1);
		g_print("row changed got name %s pointer %p\n",name, pointer);
		if (!GTK_BIN(mitem)->child) {
			gtk_container_add(GTK_CONTAINER(mitem), gtk_label_new(name));
			gtk_widget_show_all((GtkWidget *)mitem);
			
			
		} else {
			g_signal_handlers_disconnect_matched(mitem, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, menuitem_activate, NULL);
			gtk_label_set_text(GTK_LABEL(GTK_BIN(mitem)->child),name);
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
		g_print("row changed, no mitem for path %s\n",gtk_tree_path_to_string(path));	
	}
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
}


/************ widget stuff *****************/

G_DEFINE_TYPE(SnippetsMenu, snippets_menu, GTK_TYPE_MENU_BAR)

static void snippets_menu_finalize(GObject * object)
{
	g_print("finalize\n");
}

static void snippets_menu_class_init(SnippetsMenuClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	/*GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);*/
	object_class->finalize = snippets_menu_finalize;
}

static void snippets_menu_init(SnippetsMenu * sm)
{
/*	gtk_menu_shell_append((GtkMenuShell *)sm,gtk_menu_item_new_with_label("bla"));*/


}



GtkWidget *snippets_menu_new(void)
{
	SnippetsMenu *sm = (SnippetsMenu *) g_object_new(SNIPPETS_TYPE_MENU, NULL);
	g_return_val_if_fail(sm != NULL, NULL);
	return GTK_WIDGET(sm);
}
