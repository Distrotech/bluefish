
#include "vcs.h"

typedef struct {
	Tbfwin *bfwin;
	Tvcssession *vs;
	GtkWidget *win;

	GtkListStore *filestore;
	GtkTreeView *fileview;

	GtkTextView *tview;

} Tvcs_gui;

void
vcs_gui_update_status(gpointer data, GList *list)
{
	Tvcs_gui *vg=data;
	GtkTreeIter iter;
	GList *tmplist;

	for (tmplist=list;tmplist;tmplist=tmplist->next) {
		Tvcsfilestatus *vs = tmplist->data;
		if (vs->status == vcsstatus_modified) {
			gtk_list_store_append(GTK_LIST_STORE(vg->filestore), &iter);
			gtk_list_store_set(GTK_LIST_STORE(vg->filestore), &iter, 0, TRUE, 1, vs->filename, -1);
		}
	}

}


void
vcs_commit_dialog(Tbfwin *bfwin, Tvcssession *vs)
{
	Tvcs_gui *vg;
	GtkWidget *vbox;
	GtkTreeViewColumn *column;
	GtkWidget *scrolwin;
	GtkCellRenderer *renderer;

	vg = g_slice_new(Tvcs_gui);
	vg->bfwin = bfwin;
	vg->vs = vs;
	vg->win = gtk_dialog_new_with_buttons(_("Commit"), GTK_WINDOW(bfwin->main_window),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,
				GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
				NULL);
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(vg->win));

	vg->filestore = gtk_list_store_new(2, G_TYPE_BOOLEAN, G_TYPE_STRING);

	vg->fileview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(vg->filestore));
	renderer = gtk_cell_renderer_toggle_new();
	column = gtk_tree_view_column_new_with_attributes(_("Include"), renderer, "active", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(vg->fileview), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Filename"), renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(vg->fileview), column);

	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), GTK_WIDGET(vg->fileview));

	gtk_box_pack_start(GTK_BOX(vbox), scrolwin, TRUE, TRUE, 4);
	gtk_box_pack_start(GTK_BOX(vbox), gtk_vseparator_new(), TRUE, FALSE, 4);

	vg->tview = gtk_text_view_new();
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), GTK_WIDGET(vg->tview));
	gtk_box_pack_start(GTK_BOX(vbox), scrolwin, TRUE, TRUE, 4);

	vcs_status(bfwin, vs, vg);

	gtk_widget_show_all(vg->win);
}
