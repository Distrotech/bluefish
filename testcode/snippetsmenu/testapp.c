#include <gtk/gtk.h>
#include <gio/gio.h>
#include "snippetsmenu.h"

GtkWidget *snippetsmenu;
GtkTreeStore *treestore;
gint i=0;
enum {
	PIXMAP_COLUMN,
	TITLE_COLUMN,
	NODE_COLUMN,
	NUM_COLUMNS
};
void on_window_destroy(GtkWidget * widget, gpointer data)
{
	gtk_main_quit();
}

void on_delete_clicked(GtkWidget * button, gpointer data)
{
	GtkTreeIter iter2;
	if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(treestore),&iter2,"0:0")) {
		gtk_tree_store_remove(treestore, &iter2);
	} else if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(treestore),&iter2,"0")) {
		gtk_tree_store_remove(treestore, &iter2);
	}
}

void on_change_clicked(GtkWidget * button, gpointer data)
{
	GtkTreeIter iter2;
	gchar *name;
	name = g_strdup_printf("changed%d",i);
	if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(treestore),&iter2,"0:0")) {
		gtk_tree_store_set(treestore, &iter2, TITLE_COLUMN, name, NODE_COLUMN, (void *)i, -1);
	} else if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(treestore),&iter2,"0")) {
		gtk_tree_store_set(treestore, &iter2, TITLE_COLUMN, name, NODE_COLUMN, (void *)i, -1);
	}
	i++;
	g_free(name);
}
void on_insert_clicked(GtkWidget * button, gpointer data)
{
	GtkTreeIter iter, iter2;
	gchar *name;
	name = g_strdup_printf("item%d",i);
	g_print("on_insert_clicked, insert %s\n",name);
	if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(treestore),&iter2,"1:2")) {
		gtk_tree_store_prepend(treestore, &iter, &iter2);
	} else if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(treestore),&iter2,"1")) {
		gtk_tree_store_prepend(treestore, &iter, &iter2);
	} else {
		gtk_tree_store_prepend(treestore, &iter, NULL);
	}
	gtk_tree_store_set(treestore, &iter, TITLE_COLUMN, name, NODE_COLUMN, (void *)i, -1);
	i++;
	g_free(name);
}

int main(int argc, char *argv[])
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *button, *treeview;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkWidget *scrolwin;
	GtkTargetEntry dndtargets[] = {{"BLUEFISH_SNIPPET", GTK_TARGET_SAME_APP, 0}};
	gtk_init(&argc, &argv);

	/* Create a Window. */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Menu widget test");

	/* Set a decent default size for the window. */
	gtk_window_set_default_size(GTK_WINDOW(window), 500, 300);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_window_destroy), NULL);

	vbox = gtk_vbox_new(FALSE, 2);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	/* create menu */
	snippetsmenu = snippets_menu_new();
	gtk_box_pack_start(GTK_BOX(vbox), snippetsmenu, 0, 0, 0);
	treestore = gtk_tree_store_new(NUM_COLUMNS, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_POINTER);
	snippets_menu_set_model((SnippetsMenu *)snippetsmenu, (GtkTreeModel *)treestore, TITLE_COLUMN, NODE_COLUMN);
	
	
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(treestore));
	column = gtk_tree_view_column_new();
/*	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column,renderer
			,"pixbuf",PIXMAP_COLUMN
			,"pixbuf_expander_closed",PIXMAP_COLUMN
			,"pixbuf_expander_open",PIXMAP_COLUMN
			,NULL);
*/	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column,renderer,"text", TITLE_COLUMN,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(treeview), GDK_BUTTON1_MASK, dndtargets,1, GDK_ACTION_MOVE);
	gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(treeview), dndtargets, 1, GDK_ACTION_MOVE);
	/*g_signal_connect(G_OBJECT(treeview), "drag-data-received",G_CALLBACK(snippetview_drag_data_received_lcb), snw);
	g_signal_connect(G_OBJECT(treeview), "drag-data-get",G_CALLBACK(snippetview_drag_data_get_lcb), snw);*/
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), treeview);
	gtk_box_pack_start(GTK_BOX(vbox), scrolwin, TRUE, TRUE, 0);
	
	/* Create a close button. */
	button = gtk_button_new_with_label("insert");
	gtk_box_pack_start(GTK_BOX(vbox), button, 0, 0, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_insert_clicked), NULL);

	button = gtk_button_new_with_label("delete");
	gtk_box_pack_start(GTK_BOX(vbox), button, 0, 0, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_delete_clicked), NULL);

	button = gtk_button_new_with_label("change");
	gtk_box_pack_start(GTK_BOX(vbox), button, 0, 0, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_change_clicked), NULL);

	
	gtk_widget_show_all(window);

	gtk_main();
	return 0;
}
