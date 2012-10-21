
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

static void
vcs_commit_dialog_lcb(GtkDialog *dialog,gint response,gpointer user_data)
{
	Tvcs_gui *vg = user_data;
	if (response == GTK_RESPONSE_ACCEPT) {
		GList *list=NULL;
		GtkTextIter si, ei;
		GtkTextBuffer *buffer;
		gchar *message;
		GtkTreeIter iter;
		gboolean cont;
		/*build the list */
		cont = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(vg->filestore),&iter);
		while(cont) {
			gboolean enabled;
			gtk_tree_model_get(GTK_TREE_MODEL(vg->filestore),&iter,0, &enabled, -1);
			if (enabled) {
				gchar *tmpstr;
				gtk_tree_model_get(GTK_TREE_MODEL(vg->filestore),&iter,1, &tmpstr, -1);
				list = g_list_prepend(list, tmpstr);
			}
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(vg->filestore), &iter);
		}
		buffer = gtk_text_view_get_buffer(vg->tview);
		gtk_text_buffer_get_bounds(buffer,&si,&ei);
		message = gtk_text_buffer_get_text(buffer, &si, &ei, TRUE);
		vcs_commit_real(vg->bfwin, vg->vs, list, message);
		g_list_foreach(list, (GFunc) g_free, NULL);
		g_list_free(list);
		g_free(message);
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_slice_free(Tvcs_gui, vg);
}

static void
active_edited_lcb(GtkCellRendererToggle * cellrenderertoggle, gchar * path, Tvcs_gui *vg)
{
	GtkTreeIter iter;
	GtkTreePath *tpath;
	tpath = gtk_tree_path_new_from_string(path);
	if (tpath && gtk_tree_model_get_iter(GTK_TREE_MODEL(vg->filestore), &iter, tpath)) {
		gtk_list_store_set(GTK_LIST_STORE(vg->filestore), &iter, 0, !gtk_cell_renderer_toggle_get_active(cellrenderertoggle), -1);
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

	vg->fileview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(vg->filestore)));
	renderer = gtk_cell_renderer_toggle_new();
	column = gtk_tree_view_column_new_with_attributes(_("Include"), renderer, "active", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(vg->fileview), column);
	g_object_set(G_OBJECT(renderer), "activatable", TRUE, NULL);
	g_signal_connect(G_OBJECT(renderer), "toggled", G_CALLBACK(active_edited_lcb), vg);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Filename"), renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(vg->fileview), column);

	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), GTK_WIDGET(vg->fileview));

	gtk_box_pack_start(GTK_BOX(vbox), scrolwin, TRUE, TRUE, 4);
	gtk_box_pack_start(GTK_BOX(vbox), gtk_vseparator_new(), FALSE, FALSE, 4);

	vg->tview = GTK_TEXT_VIEW(gtk_text_view_new());
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), GTK_WIDGET(vg->tview));
	gtk_box_pack_start(GTK_BOX(vbox), scrolwin, TRUE, TRUE, 4);

	vcs_status(bfwin, vs, vg);
	gtk_window_set_default_size(GTK_WINDOW(vg->win), 300, 300);
	gtk_widget_show_all(vg->win);
	g_signal_connect(vg->win, "response", G_CALLBACK(vcs_commit_dialog_lcb), vg);
}
