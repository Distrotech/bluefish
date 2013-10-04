/* compile with
gcc -o filetreemodel filetreemodel.c main.c `pkg-config --cflags --libs gtk+-3.0`
*/

#include "filetreemodel.h"
#include <stdlib.h>

void fill_model(FileTreemodel * ftm)
{
	gint i=0;
	GFile *uri; 
/*	const gchar *firstnames[] = { "Joe", "Jane", "William", "Hannibal", "Timothy", "Gargamel", NULL };
	const gchar *surnames[] = { "Grokowich", "Twitch", "Borheimer", "Bork", NULL };
	const gchar **fname, **sname;

	for (sname = surnames; *sname != NULL; sname++) {
		for (fname = firstnames; *fname != NULL; fname++) {
			gchar *name = g_strdup_printf("%s %s", *fname, *sname);

			filetreemodel_append_record(ftm, name,"folder");

			g_free(name);
		}
	}*/
	const gchar *paths[] = {"/home/olivier/svnbluefish", "/home2/olivier/", "/home/olivier/tmp", NULL};
	while (paths[i]) {
		uri = g_file_new_for_path(paths[i]);
		filetreemodel_build_dir(ftm, uri);
		g_object_unref(uri);
		i++;
	}
	uri = g_file_new_for_path("/home/olivier/tmp");
	/*fb2_fill_dir_async(ftm, NULL, uri);*/
	g_object_unref(uri);
	
}

GtkWidget *create_view_and_model(void)
{
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	FileTreemodel *FileTreemodel;
	GtkWidget *view;

	FileTreemodel = filetreemodel_new();
	fill_model(FileTreemodel);

	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(FileTreemodel));

	g_object_unref(FileTreemodel);	/* destroy store automatically with view */

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();

	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", filetreemodel_COL_NAME);
	gtk_tree_view_column_set_title(col, "Name");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", filetreemodel_COL_ICON_NAME);
	gtk_tree_view_column_set_title(col, "Icon name");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	return view;
}

int main(int argc, char **argv)
{
	GtkWidget *window, *view, *scrollwin;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 200, 400);
	g_signal_connect(window, "delete_event", gtk_main_quit, NULL);

	scrollwin = gtk_scrolled_window_new(NULL, NULL);

	view = create_view_and_model();

	gtk_container_add(GTK_CONTAINER(scrollwin), view);
	gtk_container_add(GTK_CONTAINER(window), scrollwin);

	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}
