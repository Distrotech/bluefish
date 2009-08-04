#include <gtk/gtk.h>
#include <gio/gio.h>
#include "snippetsmenu.h"

GtkWidget *snippetsmenu;
GtkTreeStore *treestore;
gint i=0;

void on_window_destroy(GtkWidget * widget, gpointer data)
{
	gtk_main_quit();
}

void on_delete_clicked(GtkWidget * button, gpointer data)
{
	GtkTreeIter iter2;
	if (gtk_tree_model_get_iter_from_string (treestore,&iter2,"0:0")) {
		gtk_tree_store_remove(treestore, &iter2);
	} else if (gtk_tree_model_get_iter_from_string (treestore,&iter2,"0")) {
		gtk_tree_store_remove(treestore, &iter2);
	}
}

void on_change_clicked(GtkWidget * button, gpointer data)
{
	GtkTreeIter iter2;
	gchar *name;
	name = g_strdup_printf("changed%d",i);
	if (gtk_tree_model_get_iter_from_string (treestore,&iter2,"0:0")) {
		gtk_tree_store_set(treestore, &iter2, 0, name, 1, (void *)i, -1);
	} else if (gtk_tree_model_get_iter_from_string (treestore,&iter2,"0")) {
		gtk_tree_store_set(treestore, &iter2, 0, name, 1, (void *)i, -1);
	}
	i++;
	g_free(name);
}
void on_button_clicked(GtkWidget * button, gpointer data)
{
	GtkTreeIter iter, iter2;
	gchar *name;
	name = g_strdup_printf("item%d",i);
	g_print("on_button_clicked, insert %s\n",name);
	if (gtk_tree_model_get_iter_from_string (treestore,&iter2,"1:2")) {
		gtk_tree_store_prepend(treestore, &iter, &iter2);
	} else if (gtk_tree_model_get_iter_from_string (treestore,&iter2,"1")) {
		gtk_tree_store_prepend(treestore, &iter, &iter2);
	} else {
		gtk_tree_store_prepend(treestore, &iter, NULL);
	}
	gtk_tree_store_set(treestore, &iter, 0, name, 1, (void *)i, -1);
	i++;
	g_free(name);
}

int main(int argc, char *argv[])
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *button;

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
	treestore = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
	snippets_menu_set_model((SnippetsMenu *)snippetsmenu, (GtkTreeModel *)treestore);
	/* Create a close button. */
	button = gtk_button_new_with_label("insert");
	gtk_box_pack_start(GTK_BOX(vbox), button, 0, 0, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_button_clicked), NULL);

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
