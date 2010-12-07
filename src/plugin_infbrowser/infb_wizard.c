/* Bluefish HTML Editor
 * infb_wizard.c - plugin for information browser
 *
 * Copyright (C) 2007 Oskar Świda
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

/* #define DEBUG */

#include <string.h>
#include <stdio.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlwriter.h>
#include <glib/gstdio.h>
#include "infb_wizard.h"
#include "infbrowser.h"
#include "infb_load.h"
#include "infb_text.h"

#define ENTRY_TYPE_NONE			0
#define ENTRY_TYPE_DTD_LOCAL	1
#define ENTRY_TYPE_DTD_REMOTE	2

#define LAST_PAGE 3;

typedef enum {
	infbw_type,					/* ==0 */
	infbw_info,
	infbw_dtd_file,
	infbw_dtd_remote,
	infbw_num_pages
} Tinfbw_pagenum;

typedef struct {
	gint type;
	gchar *name;
	gchar *desc;
	gchar *uri;
	Tbfwin *bfwin;
	GtkWidget *dialog;
	Tinfbw_pagenum current_page;
	gpointer current_pagepointer;
} TentryData;


static void infbw_save_entry(TentryData * data)
{
	gchar *fname, *pstr;
	xmlDocPtr doc;
	xmlNodePtr node;
	FILE *ff;

	if (!data)
		return;
	pstr = g_strdup(data->name);
	fname =
		g_strconcat(g_get_home_dir(), "/." PACKAGE "/bflib_",
					g_strcanon(pstr,
							   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
							   '_'), ".xml", NULL);

	doc = xmlNewDoc(BAD_CAST "1.0");
	node = xmlNewNode(NULL, BAD_CAST "ref");
	xmlDocSetRootElement(doc, node);
	xmlNewProp(node, BAD_CAST "name", BAD_CAST data->name);
	if (data->desc)
		xmlNewProp(node, BAD_CAST "description", BAD_CAST data->desc);
	xmlNewProp(node, BAD_CAST "type", BAD_CAST "dtd");
	xmlNewProp(node, BAD_CAST "uri", BAD_CAST data->uri);
	ff = fopen(fname, "w");
	if (ff) {
		xmlDocDump(ff, doc);
		fclose(ff);
		infb_reload_home(data->bfwin);
	}
	g_free(fname);
	g_free(pstr);

}


static void infbw_type_set(GtkToggleButton * btn, gpointer data)
{
	TentryData *d = (TentryData *) data;
	gpointer ptr = g_object_get_data(G_OBJECT(btn), "type");
	if (ptr) {
		if (strcmp((gchar *) ptr, "dtd_local") == 0)
			d->type = ENTRY_TYPE_DTD_LOCAL;
		else if (strcmp((gchar *) ptr, "dtd_remote") == 0)
			d->type = ENTRY_TYPE_DTD_REMOTE;
		else
			d->type = ENTRY_TYPE_NONE;
	}
}


static GtkWidget *create_page_type(GtkWidget * dlg, TentryData * edata)
{
	GtkWidget *box, *btn;
	GSList *btn_group = NULL;

	box = gtk_vbox_new(FALSE, 0);
	btn = gtk_radio_button_new_with_mnemonic(NULL, _("Local DTD file"));
	g_object_set_data(G_OBJECT(btn), "type", "dtd_local");
	gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 2);
	gtk_radio_button_set_group(GTK_RADIO_BUTTON(btn), btn_group);
	btn_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(btn));
	g_signal_connect(G_OBJECT(btn), "toggled", G_CALLBACK(infbw_type_set), edata);
	edata->type = ENTRY_TYPE_DTD_LOCAL;

	btn = gtk_radio_button_new_with_mnemonic(NULL, _("Remote DTD file"));
	g_object_set_data(G_OBJECT(btn), "type", "dtd_remote");
	gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 2);
	gtk_radio_button_set_group(GTK_RADIO_BUTTON(btn), btn_group);
	btn_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(btn));
	g_signal_connect(G_OBJECT(btn), "toggled", G_CALLBACK(infbw_type_set), edata);

	gtk_widget_show_all(box);
	return box;
}

static void infbw_file_chosen(GtkFileChooser * chooser, gpointer user_data)
{
	TentryData *edata = (TentryData *) user_data;
	gchar *fname;

	fname = gtk_file_chooser_get_filename(chooser);
	if (fname && *fname) {
		gtk_dialog_set_response_sensitive(GTK_DIALOG(edata->dialog), 1, TRUE);
		edata->uri = fname;
	} else {
		gtk_dialog_set_response_sensitive(GTK_DIALOG(edata->dialog), 1, TRUE);
		if (edata->uri) {
			g_free(edata->uri);
			edata->uri = NULL;
		}
	}
}

static GtkWidget *create_page_dtd_file(GtkWidget * dlg, TentryData * edata)
{
	GtkWidget *box, *label, *btn, *box2;

	box = gtk_hbox_new(FALSE, 0);
	box2 = gtk_vbox_new(FALSE, 15);
	gtk_box_pack_start(GTK_BOX(box2), box, FALSE, FALSE, 5);

	label = gtk_label_new(_("Choose file"));
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 2);

	btn = gtk_file_chooser_button_new(_("Local DTD file"), GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_box_pack_start(GTK_BOX(box), btn, TRUE, TRUE, 5);
	g_signal_connect(G_OBJECT(btn), "update-preview", G_CALLBACK(infbw_file_chosen), edata);

	gtk_widget_show_all(box2);

	return box2;
}

static void infbw_name_changed(GtkWidget * widget, gpointer user_data)
{
	TentryData *edata = (TentryData *) user_data;
	const gchar *text;
	text = gtk_entry_get_text(GTK_ENTRY(widget));
	if (text && *text) {
		gtk_dialog_set_response_sensitive(GTK_DIALOG(edata->dialog), 1, TRUE);
		edata->name = g_strdup(text);
	} else {
		gtk_dialog_set_response_sensitive(GTK_DIALOG(edata->dialog), 1, FALSE);
		if (edata->name) {
			g_free(edata->name);
			edata->name = NULL;
		}
	}
}

static void infbw_desc_changed(GtkWidget * widget, gpointer user_data)
{
	TentryData *edata = (TentryData *) user_data;
	const gchar *text;
	text = gtk_entry_get_text(GTK_ENTRY(widget));
	if (text && *text) {
		edata->desc = g_strdup(text);
	} else {
		if (edata->desc) {
			g_free(edata->desc);
			edata->name = NULL;
		}
	}
}

static GtkWidget *create_page_info(GtkWidget * dlg, TentryData * edata)
{
	GtkWidget *box, *label, *entry, *box2;

	box = gtk_vbox_new(FALSE, 0);
	box2 = gtk_hbox_new(TRUE, 5);
	gtk_box_pack_start(GTK_BOX(box2), box, TRUE, TRUE, 5);

	label = gtk_label_new(_("Entry name"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 2, 2);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(box), entry, TRUE, TRUE, 5);
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(infbw_name_changed), edata);

	label = gtk_label_new(_("Description"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 2, 2);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(box), entry, TRUE, TRUE, 5);
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(infbw_desc_changed), edata);

	gtk_widget_show_all(box2);
	return box2;
}

static void infbw_uri_changed(GtkWidget * widget, gpointer user_data)
{
	TentryData *edata = (TentryData *) user_data;
	const gchar *text;
	text = gtk_entry_get_text(GTK_ENTRY(widget));
	if (text && *text) {
		edata->uri = g_strdup(text);
	} else {
		if (edata->uri) {
			g_free(edata->uri);
			edata->uri = NULL;
		}
	}
}

static GtkWidget *create_page_dtd_remote(GtkWidget * dlg, TentryData * edata)
{
	GtkWidget *box, *label, *entry, *box2;

	box = gtk_vbox_new(FALSE, 2);
	box2 = gtk_hbox_new(TRUE, 5);
	gtk_box_pack_start(GTK_BOX(box2), box, TRUE, TRUE, 5);

	label = gtk_label_new(_("DTD file URI"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 2, 2);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 2);

	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 5);
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(infbw_uri_changed), edata);

	gtk_widget_show_all(box2);
	return box;
}

/* this function gives the flow through the pages */
static gint infbw_next_page(gint current_page, gpointer data)
{
	TentryData *edata = data;
	switch (current_page) {
	case 1:
		switch (edata->type) {
		case ENTRY_TYPE_DTD_LOCAL:
			return 2;
		case ENTRY_TYPE_DTD_REMOTE:
			return 3;
		default:
			return -1;
		}
		break;
	case 2:
	case 3:
		infbw_save_entry(edata);
		return -1;
		break;
	default:
		if (current_page + 1 < infbw_num_pages)
			return current_page + 1;
		else
			return -1;
	}
}

static void infbw_dialog_response_lcb(GtkDialog * dialog, gint response, gpointer data)
{
	TentryData *edata = data;
	gint newpage;
	newpage = infbw_next_page(edata->current_page, edata);
	DEBUG_MSG("infbw_dialog_response_lcb, response==%d,newpage=%d\n",response,newpage);
	if (newpage == -1) infbw_save_entry(edata);
	if (response == GTK_RESPONSE_REJECT || newpage == -1) {
		
		gtk_widget_destroy(edata->dialog);
		if (edata->name)
			g_free(edata->name);
		if (edata->desc)
			g_free(edata->desc);
		if (edata->uri)
			g_free(edata->uri);
		g_free(edata);
		DEBUG_MSG("infbw_dialog_response_lcb, dialog and data destroyed, returning\n");
		return;
	}

	if (newpage != edata->current_page) {
		GtkWidget *vbox = gtk_dialog_get_content_area(GTK_DIALOG(edata->dialog));
		gtk_container_remove(GTK_CONTAINER(vbox), edata->current_pagepointer);
		switch (newpage) {
		case infbw_type:
			edata->current_pagepointer  = create_page_type(edata->dialog, edata); 
		break;
		case infbw_info:
			edata->current_pagepointer  = create_page_info(edata->dialog, edata);
			break;
		case infbw_dtd_file:
			edata->current_pagepointer  = create_page_dtd_file(edata->dialog, edata);
			break;
		case infbw_dtd_remote:
			edata->current_pagepointer  = create_page_dtd_remote(edata->dialog, edata);
			break;
		}
		gtk_container_add(GTK_CONTAINER(vbox), edata->current_pagepointer);
		edata->current_page = newpage;
	}
	gtk_widget_show_all(edata->dialog);
}

void infb_add_item(Tbfwin * bfwin)
{
	TentryData *edata = g_new0(TentryData, 1);
	edata->bfwin = bfwin;
	edata->dialog =
		gtk_dialog_new_with_buttons(_("New info item"), GTK_WINDOW(edata->bfwin->main_window),
									GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL,
									GTK_RESPONSE_REJECT, GTK_STOCK_GO_FORWARD, 1, NULL);

	edata->current_pagepointer = create_page_type(edata->dialog, edata);	
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(edata->dialog))), edata->current_pagepointer);
	edata->current_page = infbw_type;
	gtk_widget_show_all(edata->dialog);
	g_signal_connect(G_OBJECT(edata->dialog), "response", G_CALLBACK(infbw_dialog_response_lcb),
					 edata);
}


/*****************************************************************/

typedef enum {
	infbwdel_select,			/* ==0 */
	infbwdel_confirm,
	infbwdel_num_pages
} Tinfbwdel_pagenum;

typedef struct {
	Tbfwin *bfwin;
	GtkWidget *dialog;
	gchar *uri;
	gchar *name;
	gpointer pages[infbw_num_pages];
	Tinfbw_pagenum current_page;
	gpointer current_pagepointer;
} TdelData;

static void infbw_selected_lcb(GtkTreeView * view, gpointer user_data)
{
	TdelData *ddata = user_data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *val, *val2;
	GtkTreeSelection *sel = gtk_tree_view_get_selection(view);

	if (sel && gtk_tree_selection_get_selected(sel, &model, &iter)) {
		gtk_tree_model_get(model, &iter, 1, &val, 0, &val2, -1);
		ddata->uri = g_strdup(val);
		ddata->name = g_strdup(val2);
		gtk_dialog_set_response_sensitive(GTK_DIALOG(ddata->dialog), 1, TRUE);
	} else {
		if (ddata->uri) {
			g_free(ddata->uri);
			ddata->uri = NULL;
		}
		if (ddata->name) {
			g_free(ddata->name);
			ddata->name = NULL;
		}
		gtk_dialog_set_response_sensitive(GTK_DIALOG(ddata->dialog), 1, FALSE);
	}
}

static GtkWidget *create_page_select(GtkWidget * dlg, TdelData * ddata)
{
	GtkWidget *box, *label, *entry, *box2;
	GtkWidget *scrolledwindow1;
	GtkListStore *store;
	GtkCellRenderer *rend = gtk_cell_renderer_text_new();
	GtkTreeIter iter;
	GList *list = NULL, *lpt;

	box = gtk_vbox_new(FALSE, 2);
	box2 = gtk_hbox_new(TRUE, 5);
	gtk_box_pack_start(GTK_BOX(box2), box, TRUE, TRUE, 5);

	label = gtk_label_new(_("Entries you can delete"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 2, 2);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 2);

	scrolledwindow1 = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow1), GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(box), scrolledwindow1, FALSE, FALSE, 5);

	store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	list = infb_user_files();
	lpt = list;
	while (lpt) {
		gchar **arr = g_strsplit(lpt->data, ",", -1);
		if (arr[0] && arr[1]) {
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 0, arr[0], 1, arr[1], -1);
		}
		lpt = g_list_next(lpt);
	}
	entry = gtk_tree_view_new();
	gtk_tree_view_set_model(GTK_TREE_VIEW(entry), GTK_TREE_MODEL(store));
	gtk_container_add(GTK_CONTAINER(scrolledwindow1), entry);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(entry), FALSE);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(entry), 0, "", rend, "text", 0, NULL);
	g_signal_connect(G_OBJECT(entry), "cursor-changed", G_CALLBACK(infbw_selected_lcb), ddata);

	gtk_widget_show_all(box2);
	return box2;
}

static GtkWidget *create_page_select_confirm(GtkWidget * dlg, TdelData * ddata)
{
	GtkWidget *box, *label, *box2;
	gchar *pstr;

	box = gtk_vbox_new(FALSE, 2);
	box2 = gtk_hbox_new(TRUE, 5);
	gtk_box_pack_start(GTK_BOX(box2), box, TRUE, TRUE, 5);

	label = gtk_label_new("");
	pstr = g_strconcat("<b>Entry name:</b>", ddata->name, NULL);
	gtk_label_set_markup(GTK_LABEL(label), pstr);
	g_free(pstr);

	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 2, 2);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 2);


	label = gtk_label_new("");
	pstr = g_strconcat("<b>Entry file:</b>", ddata->uri, NULL);
	gtk_label_set_markup(GTK_LABEL(label), pstr);
	g_free(pstr);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 2, 2);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 2);

	gtk_widget_show_all(box2);
	return box2;
}


static void infbwdel_dialog_response_lcb(GtkDialog * dialog, gint response, gpointer data)
{
	TdelData *ddata = data;
	gint newpage;
	if (response == GTK_RESPONSE_REJECT) {
		gtk_widget_destroy(ddata->dialog);
		g_free(ddata);
		return;
	} else if (ddata->current_page == infbwdel_confirm) {
		g_remove(ddata->uri);
		infb_reload_home(ddata->bfwin);
		gtk_widget_destroy(ddata->dialog);
		g_free(ddata);
		return;
	}
	newpage = ddata->current_page + 1;
	if (newpage != ddata->current_page) {
		GtkWidget *vbox = gtk_dialog_get_content_area(GTK_DIALOG(ddata->dialog));
		gtk_container_remove(GTK_CONTAINER(vbox), ddata->current_pagepointer);
		if (newpage == infbwdel_select) {
			ddata->current_pagepointer = create_page_select_confirm(ddata->dialog, ddata);
		}
		gtk_container_add(GTK_CONTAINER(vbox), ddata->current_pagepointer);
		ddata->current_page = newpage;
	}
	gtk_widget_show_all(ddata->dialog);
}

void infb_del_item(Tbfwin * bfwin)
{
	TdelData *ddata = g_new0(TdelData, 1);
	ddata->bfwin = bfwin;

	ddata->current_pagepointer = create_page_select(ddata->dialog, ddata);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(ddata->dialog))), ddata->current_pagepointer);
	gtk_widget_show_all(ddata->dialog);
	g_signal_connect(G_OBJECT(ddata->dialog), "response", G_CALLBACK(infbwdel_dialog_response_lcb),
					 ddata);
}
