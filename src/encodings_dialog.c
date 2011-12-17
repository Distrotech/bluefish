/* Bluefish HTML Editor
 * encodings_dialog.c
 *
 * Copyright (C) 2008-2011 James Hayward and Olivier Sessink
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


#include "encodings_dialog.h"
#include "bfwin_uimanager.h"
#include "dialog_utils.h"
#include "document.h"
#include "stringlist.h"


#define BLUEFISH_ENCODINGS_DIALOG_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), BLUEFISH_TYPE_ENCODINGS_DIALOG, BluefishEncodingsDialogPrivate))


struct _BluefishEncodingsDialogPrivate {
	GtkWidget *addButton;
	GtkWidget *removeButton;

	GtkWidget *encAvailView;
	GtkWidget *encInMenuView;

	GtkListStore *encStore;

	Tbfwin *bfwin;
};

enum {
	PROP_0,
	PROP_BFWIN
};

G_DEFINE_TYPE(BluefishEncodingsDialog, bluefish_encodings_dialog, GTK_TYPE_DIALOG)


enum {
	COLUMN_DESCRIPTION,
	COLUMN_ENCODING,
	COLUMN_IN_MENU,
	COLUMN_CAN_REMOVE,
	NUM_COLUMNS
};


static void
bluefish_encodings_dialog_finalize(GObject * object)
{
	BluefishEncodingsDialog *dialog = BLUEFISH_ENCODINGS_DIALOG(object);

	g_object_unref(dialog->priv->encStore);

	G_OBJECT_CLASS(bluefish_encodings_dialog_parent_class)->finalize(object);
}

static void
bluefish_encodings_dialog_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
	BluefishEncodingsDialog *dialog = BLUEFISH_ENCODINGS_DIALOG(object);

	switch (prop_id) {
	case PROP_BFWIN:
		g_value_set_pointer(value, dialog->priv->bfwin);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void
bluefish_encodings_dialog_set_property(GObject * object,
									   guint prop_id, const GValue * value, GParamSpec * pspec)
{
	BluefishEncodingsDialog *dialog = BLUEFISH_ENCODINGS_DIALOG(object);

	switch (prop_id) {
	case PROP_BFWIN:
		dialog->priv->bfwin = g_value_get_pointer(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static GList *
get_encoding_list_for_all_open_documents(void)
{
	GList *opendocs = return_allwindows_documentlist();
	GList *encodings = NULL;

	opendocs = g_list_first(opendocs);
	while (opendocs) {
		Tdocument *doc = opendocs->data;
		encodings = add_to_stringlist(encodings, doc->encoding);

		opendocs = g_list_next(opendocs);
	}

	return (encodings);
}

static void
enc_avail_view_selection_changed(GtkTreeSelection * selection, BluefishEncodingsDialog * dialog)
{
	if (gtk_tree_selection_count_selected_rows(selection) > 0) {
		gtk_widget_set_sensitive(dialog->priv->addButton, TRUE);
	} else {
		gtk_widget_set_sensitive(dialog->priv->addButton, FALSE);
	}
}

static void
enc_in_menu_view_selection_changed(GtkTreeSelection * selection, BluefishEncodingsDialog * dialog)
{
	if (gtk_tree_selection_count_selected_rows(selection) > 0) {
		gboolean canRemove = TRUE;
		GtkTreeIter iter;
		GtkTreeModel *model;

		model = gtk_tree_view_get_model(GTK_TREE_VIEW(dialog->priv->encInMenuView));
		if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
			gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, COLUMN_CAN_REMOVE, &canRemove, -1);
		}

		gtk_widget_set_sensitive(dialog->priv->removeButton, canRemove);
	} else {
		gtk_widget_set_sensitive(dialog->priv->removeButton, FALSE);
	}
}

static void
enc_refilter(BluefishEncodingsDialog * dialog)
{
	GtkTreeModel *filter;

	filter = gtk_tree_view_get_model(GTK_TREE_VIEW(dialog->priv->encAvailView));
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(filter));

	filter = gtk_tree_view_get_model(GTK_TREE_VIEW(dialog->priv->encInMenuView));
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(filter));
}

static void
enc_add_button_clicked(GtkButton * button, BluefishEncodingsDialog * dialog)
{
	GtkTreeIter childIter, filterIter;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(dialog->priv->encAvailView));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dialog->priv->encAvailView));

	if (gtk_tree_selection_get_selected(selection, &model, &filterIter)) {
		gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(model),
														 &childIter, &filterIter);

		gtk_list_store_set(dialog->priv->encStore, &childIter, COLUMN_IN_MENU, TRUE, -1);
		enc_refilter(dialog);
	}
}

static void
enc_remove_button_clicked(GtkButton * button, BluefishEncodingsDialog * dialog)
{
	GtkTreeIter childIter, filterIter;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(dialog->priv->encInMenuView));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dialog->priv->encInMenuView));

	if (gtk_tree_selection_get_selected(selection, &model, &filterIter)) {
		gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(model),
														 &childIter, &filterIter);

		gtk_list_store_set(dialog->priv->encStore, &childIter, COLUMN_IN_MENU, FALSE, -1);
		enc_refilter(dialog);
	}
}

static void
encodings_dialog_response_lcb(GtkDialog * dialog, gint response, BluefishEncodingsDialog * encDialog)
{
	GtkTreeIter iter;
	gboolean valid;
	GList *tmplist = NULL;
	GList *bfwinlist = NULL;

	gtk_widget_hide(GTK_WIDGET(dialog));

	free_arraylist(main_v->globses.encodings);

	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(encDialog->priv->encStore), &iter);

	while (valid) {
		gchar *desc, *enc;
		gboolean inMenu = FALSE;

		gtk_tree_model_get(GTK_TREE_MODEL(encDialog->priv->encStore), &iter,
						   COLUMN_DESCRIPTION, &desc, COLUMN_ENCODING, &enc, COLUMN_IN_MENU, &inMenu, -1);
		tmplist = g_list_append(tmplist, array_from_arglist(desc, enc, inMenu ? "1" : "0", NULL));
		g_free(desc);
		g_free(enc);

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(encDialog->priv->encStore), &iter);
	}

	main_v->globses.encodings = tmplist;

	bfwinlist = g_list_first(main_v->bfwinlist);
	while (bfwinlist) {
		bfwin_encodings_menu_create(BFWIN(bfwinlist->data));
		bfwinlist = g_list_next(bfwinlist);
	}

	if (encDialog->priv->bfwin->current_document)
		bfwin_encoding_set_wo_activate(encDialog->priv->bfwin,
									   encDialog->priv->bfwin->current_document->encoding);

	gtk_widget_destroy(GTK_WIDGET(dialog));
}

static gboolean
visible_func(GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
	gboolean inMenu = FALSE;
	gboolean retval;

	gtk_tree_model_get(model, iter, COLUMN_IN_MENU, &inMenu, -1);

	if (g_ascii_strcasecmp((gchar *) data, "ENC_AVAIL_MODEL"))
		retval = inMenu;
	else
		retval = !inMenu;
	return (retval);
}

static GObject *
bluefish_encodings_dialog_create(GType type,
								 guint n_construct_properties, GObjectConstructParam * construct_properties)
{
	BluefishEncodingsDialogClass *klass =
		BLUEFISH_ENCODINGS_DIALOG_CLASS(g_type_class_peek(BLUEFISH_TYPE_ENCODINGS_DIALOG));
	GObjectClass *parent_class = G_OBJECT_CLASS(g_type_class_peek_parent(klass));
	GObject *obj = parent_class->constructor(type,
											 n_construct_properties,
											 construct_properties);

	BluefishEncodingsDialog *dialog = BLUEFISH_ENCODINGS_DIALOG(obj);

	GtkWidget *alignment, *hbox, *label, *scrolwin, *vbox;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeIter iter;
	GtkTreeModel *encAvailModel, *encInMenuModel;
	GtkTreeSelection *selection;
	GList *encodings, *node;

	gtk_window_set_default_size(GTK_WINDOW(dialog), -1, 370);

	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

	g_signal_connect(dialog, "response", G_CALLBACK(encodings_dialog_response_lcb), dialog);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox, TRUE, TRUE, 8);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 8);

	label = gtk_label_new_with_mnemonic(_("_Available encodings:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolwin), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), scrolwin, TRUE, TRUE, 0);

	dialog->priv->encStore = gtk_list_store_new(NUM_COLUMNS,
												G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);

	encodings = get_encoding_list_for_all_open_documents();

	for (node = main_v->globses.encodings; node != NULL; node = node->next) {
		GStrv arr = (GStrv) node->data;
		if (g_strv_length(arr) == 3) {
			gboolean canRemove = TRUE;

			if (g_ascii_strcasecmp(arr[1], "UTF-8") == 0
				|| g_list_find_custom(encodings, arr[1], (GCompareFunc) g_ascii_strcasecmp) != NULL)
				canRemove = FALSE;

			gtk_list_store_append(dialog->priv->encStore, &iter);
			gtk_list_store_set(dialog->priv->encStore, &iter,
							   COLUMN_DESCRIPTION, arr[0],
							   COLUMN_ENCODING, arr[1],
							   COLUMN_IN_MENU, (arr[2][0] == '1' ? TRUE : FALSE),
							   COLUMN_CAN_REMOVE, canRemove, -1);
		}
	}

	encAvailModel = gtk_tree_model_filter_new(GTK_TREE_MODEL(dialog->priv->encStore), NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(encAvailModel),
										   (GtkTreeModelFilterVisibleFunc) visible_func,
										   "ENC_AVAIL_MODEL", NULL);

	dialog->priv->encAvailView = gtk_tree_view_new_with_model(encAvailModel);
	gtk_container_add(GTK_CONTAINER(scrolwin), dialog->priv->encAvailView);
	g_object_unref(encAvailModel);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), dialog->priv->encAvailView);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dialog->priv->encAvailView));
	g_signal_connect(selection, "changed", G_CALLBACK(enc_avail_view_selection_changed), dialog);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Description"),
													  renderer, "text", COLUMN_DESCRIPTION, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(dialog->priv->encAvailView), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Encoding"), renderer, "text", COLUMN_ENCODING, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(dialog->priv->encAvailView), column);

	alignment = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 0);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(alignment), vbox);

	dialog->priv->addButton = dialog_button_new_with_image(NULL, GTK_STOCK_GO_FORWARD, G_CALLBACK(enc_add_button_clicked), dialog, TRUE, FALSE);
	gtk_button_set_focus_on_click(GTK_BUTTON(dialog->priv->addButton), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), dialog->priv->addButton, FALSE, FALSE, 0);
	gtk_widget_set_sensitive(dialog->priv->addButton, FALSE);

	dialog->priv->removeButton = dialog_button_new_with_image(NULL, GTK_STOCK_GO_BACK, G_CALLBACK(enc_remove_button_clicked), dialog, TRUE, FALSE);
	gtk_button_set_focus_on_click(GTK_BUTTON(dialog->priv->removeButton), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), dialog->priv->removeButton, FALSE, FALSE, 0);
	gtk_widget_set_sensitive(dialog->priv->removeButton, FALSE);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 8);

	label = gtk_label_new_with_mnemonic(_("Encodings in _menu:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolwin), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), scrolwin, TRUE, TRUE, 0);

	encInMenuModel = gtk_tree_model_filter_new(GTK_TREE_MODEL(dialog->priv->encStore), NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(encInMenuModel),
										   (GtkTreeModelFilterVisibleFunc) visible_func,
										   "ENC_IN_MENU_MODEL", NULL);

	dialog->priv->encInMenuView = gtk_tree_view_new_with_model(encInMenuModel);
	gtk_container_add(GTK_CONTAINER(scrolwin), dialog->priv->encInMenuView);
	g_object_unref(encInMenuModel);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), dialog->priv->encInMenuView);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dialog->priv->encInMenuView));
	g_signal_connect(selection, "changed", G_CALLBACK(enc_in_menu_view_selection_changed), dialog);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Description"),
													  renderer, "text", COLUMN_DESCRIPTION, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(dialog->priv->encInMenuView), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Encoding"), renderer, "text", COLUMN_ENCODING, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(dialog->priv->encInMenuView), column);

	return (obj);
}

static void
bluefish_encodings_dialog_class_init(BluefishEncodingsDialogClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->constructor = bluefish_encodings_dialog_create;
	object_class->finalize = bluefish_encodings_dialog_finalize;

	object_class->get_property = bluefish_encodings_dialog_get_property;
	object_class->set_property = bluefish_encodings_dialog_set_property;

	g_type_class_add_private(object_class, sizeof(BluefishEncodingsDialogPrivate));

	g_object_class_install_property(object_class,
									PROP_BFWIN,
									g_param_spec_pointer("bfwin",
														 "bfwin",
														 "The encodings dialog bfwin",
														 G_PARAM_READWRITE |
														 G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
}

static void
bluefish_encodings_dialog_init(BluefishEncodingsDialog * dialog)
{
	dialog->priv = BLUEFISH_ENCODINGS_DIALOG_GET_PRIVATE(dialog);
	g_return_if_fail(dialog->priv != NULL);
}

void
bluefish_encodings_dialog_new(Tbfwin * bfwin)
{
	BluefishEncodingsDialog *dialog;

	dialog = g_object_new(BLUEFISH_TYPE_ENCODINGS_DIALOG,
						  "bfwin", bfwin,
						  "destroy-with-parent", TRUE,
#if !GTK_CHECK_VERSION(3,0,0)
						  "has-separator", FALSE,
#endif
						  "title", _("Add or remove document encodings"),
						  "transient-for", bfwin->main_window, NULL);

	g_return_if_fail(dialog != NULL);

	gtk_widget_show_all(GTK_WIDGET(dialog));
}
