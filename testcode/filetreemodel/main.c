/* compile with
gcc -g -Wall -o filetreemodel filetreemodel.c main.c `pkg-config --cflags --libs gtk+-3.0`

G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind --tool=memcheck --num-callers=32 ./filetreemodel

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
	const gchar *paths[] = {"/home/olivier/svnbluefish", "/home2/olivier/", "file://home/olivier/tmp", "sftp://olivier.sessink.nl/var/www", NULL};
	while (paths[i]) {
		uri = g_file_new_for_commandline_arg(paths[i]);
		filetreemodel_build_dir(ftm, uri);
		g_object_unref(uri);
		i++;
	}
	/*uri = g_file_new_for_path("/home/olivier/tmp");*/
	uri = g_file_new_for_commandline_arg("/home/olivier/tmp");
	/*filetreemodel_refresh_dir_async(ftm, NULL, uri);*/
	/*path = path_for_uri(ftm, uri);
	g_print("expand to path %p\n",path);
	if (path) {
		gtk_tree_view_expand_to_path(view,path);
		gtk_tree_path_free(path);
	}*/
	g_object_unref(uri);
}

static void
view_row_expanded(GtkTreeView * tree, GtkTreeIter * iter,
					   GtkTreePath * path, FileTreemodel *ftm)
{
	UriRecord *record=NULL;
	gtk_tree_model_get(GTK_TREE_MODEL(ftm), iter, filetreemodel_COL_RECORD, &record, -1);
	filetreemodel_refresh_dir_async(ftm, NULL, record->uri);
}

static void
view_selection_changed(GtkTreeSelection * treeselection, FileTreemodel *ftm)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	/* Get the current selected row and the model. */
	if (treeselection && gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		UriRecord *record=NULL;
		gtk_tree_model_get(model, &iter, filetreemodel_COL_RECORD, &record, -1);
		if (record->uri && (g_file_info_get_file_type(record->finfo)==G_FILE_TYPE_DIRECTORY)) {
			filetreemodel_refresh_dir_async(ftm, NULL, record->uri);
		}
	}
}

GtkWidget *create_view_and_model(void)
{
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	FileTreemodel *FileTreemodel;
	GtkWidget *view;
	GtkTreeSelection *dirselection;

	FileTreemodel = filetreemodel_new();
	fill_model(FileTreemodel);

	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(FileTreemodel));

	g_object_unref(FileTreemodel);	/* destroy store automatically with view */

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_set_attributes(col, renderer,
										"icon-name", filetreemodel_COL_ICON_NAME,
										/*"pixbuf_expander_closed", PIXMAP_COLUMN,
										"pixbuf_expander_open", PIXMAP_COLUMN,*/ NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", filetreemodel_COL_NAME, "weight", filetreemodel_COL_WEIGHT, NULL);
	g_object_set(G_OBJECT(renderer), "editable", FALSE, "weight-set", TRUE, NULL);	/* Not editable. */
	/*gtk_tree_view_column_set_title(col, "Name");*/
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	/*renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", filetreemodel_COL_ICON_NAME);
	gtk_tree_view_column_set_title(col, "Icon name");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);*/

	dirselection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	g_signal_connect(G_OBJECT(dirselection), "changed", G_CALLBACK(view_selection_changed), FileTreemodel);
	g_signal_connect(G_OBJECT(view), "row-expanded", G_CALLBACK(view_row_expanded), FileTreemodel);

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
	gtk_widget_destroy(window);
	return 0;
}
