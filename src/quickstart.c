/* Bluefish HTML Editor
 * quickstart.c --> quickstart dialog
 *
 * Copyright (C) 2005 James Hayward and Olivier Sessink
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
/*
 * Initial work on a new dialog.
 * A lot more work to do still. some features are still missing
 *
 * DTD's now have an easily readable name
 *
 * meta tags can now be edited by double clicking on them in the list
 *
 * all meta tags displayed in the list are now added to the new quickstart
 * document. the remove button is used to remove tags you don't want 
 *
 * new dialog resizes better then the old one.
 */
 
#define DEBUG

#include <gtk/gtk.h>
#include <string.h>

#include "bluefish.h"
#include "cap.h"
#include "document.h"
#include "gtk_easy.h"
#include "quickstart.h"


enum {
	META_PAGE = 0,
	STYLE_PAGE,
	SCRIPT_PAGE
};

typedef struct {
	GtkWidget *dtd;
	GtkWidget *title;
	GtkWidget *headView;
	GtkWidget *metaView;
	GtkWidget *notebook;
	GtkWidget *stylelinktype;
	GtkWidget *stylehref;
	GtkWidget *stylemedia;
	GtkWidget *stylearea;
	GtkWidget *removeButton;
	Tbfwin *bfwin;
} TQuickStart;

/* Temporarily hard code the DTD's for now.
 * TODO's:
 *
 * Load the DTD's from a file. This would make adding new ones possible 
 * without having to change the code here.
 *
 * Add the ability for the user to add DTD's.
 *
 * Add an option to the preferences and per project options for a default DTD 
 * to be selected when the quick start dialog is opened.
 *
 * other?
 */
static struct {
	gchar const *name;
	gchar const *dtd;
} const dtds[] = {
	{ "HTML 4.01 Strict", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">" },
	{ "HTML 4.01 Transitional", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">" },
	{ "HTML 4.01 Frameset", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN\" \"http://www.w3.org/TR/html4/frameset.dtd\">" },
	{ "XHTML 1.0 Strict", "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"  },
	{ "XHTML 1.0 Transitional", "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">" },
	{ "XHTML 1.0 Frameset", "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Frameset//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd\">" },
};

static void
quickstart_head_selection_changed(GtkTreeSelection *tselection, TQuickStart *qstart) {
	GtkTreeModel *tmodel;
	GtkTreeIter iter;
	guint page = -1;

	tmodel = gtk_tree_view_get_model (GTK_TREE_VIEW (qstart->headView));
	if (gtk_tree_selection_get_selected (tselection, &tmodel, &iter)) {
		gtk_tree_model_get (tmodel, &iter, 1, &page, -1);
		gtk_notebook_set_current_page (GTK_NOTEBOOK (qstart->notebook), page);
	}
}

static void
quickstart_stylelinktype_changed(GtkComboBox *combobox, TQuickStart *qstart)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *type;

	gtk_combo_box_get_active_iter (GTK_COMBO_BOX (qstart->stylelinktype), &iter);
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (qstart->stylelinktype));
	gtk_tree_model_get (model, &iter, 0, &type, -1);
	
	if (strcmp(type, "") == 0) {
		gtk_widget_set_sensitive (qstart->stylehref, FALSE);
		gtk_widget_set_sensitive (qstart->stylemedia, FALSE);
	} else {
		gtk_widget_set_sensitive (qstart->stylehref, TRUE);
		gtk_widget_set_sensitive (qstart->stylemedia, TRUE);	
	}
	g_free (type);
}

static void
quickstart_load_metatags(GtkListStore *lstore) {
	GtkTreeIter iter;
	unsigned int i = 0;

/* Temporarily hard code the meta tag list.
 * TODO's:
 *
 * Load the meta tag's from a file.  
 * 
 * Allow the user to select a list of default tags to be displayed when the 
 * quick start dialog is opened.
 *
 * Allow per project default tags that are displayed when the quick start 
 * dialog is opened
 *
 * other?
 */	
	const gchar *metaTags[] = {
		"meta name=\"generator\" content=\"Bluefish\"",
		"meta name=\"author\" content=\"\"",
		"meta name=\"copyright\" content=\"\"",
		"meta name=\"keywords\" content=\"\"",
		"meta name=\"description\" content=\"\"",
		"meta name=\"ROBOTS\" content=\"NOINDEX, NOFOLLOW\"",
		"meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"",
		"meta http-equiv=\"Content-Type\" content=\"application/xhtml+xml; charset=UTF-8\"",
		"meta http-equiv=\"Content-Style-Type\" content=\"text/css\"",
		"meta http-equiv=\"Expires\" content=\"\"",
		"meta http-equiv=\"refresh\" content=\"5; URL=http://\"",
	};

	for (i = 0; i < G_N_ELEMENTS (metaTags); i++) {
		gtk_list_store_append (lstore, &iter);
		gtk_list_store_set (lstore, &iter, 0, metaTags[i], -1);
	}	
	
}

static void
quickstart_meta_selection_changed(GtkTreeSelection *tselection, TQuickStart *qstart) {
	GtkTreeModel *tmodel;
	GtkTreeIter iter;
	
	tmodel = gtk_tree_view_get_model (GTK_TREE_VIEW (qstart->metaView));
	if (gtk_tree_selection_get_selected (tselection, &tmodel, &iter)) {
		gtk_widget_set_sensitive (qstart->removeButton, TRUE);	
	} else {
		gtk_widget_set_sensitive (qstart->removeButton, FALSE);
	}
}

static void
quickstart_meta_add_clicked(GtkWidget *widget, TQuickStart *qstart) {
	GtkWidget *dialog;
	/* FIXME: We can't add any new tags yet */
	dialog = gtk_message_dialog_new (GTK_WINDOW (qstart->bfwin->main_window),
															  GTK_DIALOG_DESTROY_WITH_PARENT,
															  GTK_MESSAGE_INFO,
															  GTK_BUTTONS_CLOSE,
															  "%s",
															  "Feature not implemented yet");
 	gtk_dialog_run (GTK_DIALOG (dialog));
 	gtk_widget_destroy (dialog);
}

static void
quickstart_meta_remove_clicked(GtkWidget *widget, TQuickStart *qstart) {
	GtkTreeModel *tmodel;
	GtkTreeIter iter;
	GtkTreeSelection *tselection;

	tselection = gtk_tree_view_get_selection (GTK_TREE_VIEW (qstart->metaView));
	tmodel = gtk_tree_view_get_model (GTK_TREE_VIEW (qstart->metaView));
	if (gtk_tree_selection_get_selected (tselection, &tmodel, &iter)) {
		gtk_list_store_remove (GTK_LIST_STORE (tmodel), &iter);
	}
}

static void
quickstart_meta_cell_edited(GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, TQuickStart *qstart) {
	GtkTreeModel *tmodel;
	GtkTreeIter iter;
	GtkTreePath *path;

	tmodel = gtk_tree_view_get_model (GTK_TREE_VIEW (qstart->metaView));
	path = gtk_tree_path_new_from_string (path_string);
	
	if (gtk_tree_model_get_iter (tmodel, &iter, path)) {
		gtk_list_store_set (GTK_LIST_STORE (tmodel), &iter, 0, new_text, -1);
	}
	
	gtk_tree_path_free (path);
}

static void 
quickstart_response_lcb(GtkDialog *dialog, gint response, TQuickStart *qstart) {
	DEBUG_MSG("quickstart_response_lcb() started\n");
	if (response == GTK_RESPONSE_ACCEPT) {
		GtkTreeModel *model;
		GtkTreeIter iter;		
		gchar *tmpstr, *tmpstr2, *name, *xmlstr, *titlestr, *endstr, *finalstr, *metatag;
		gchar *dtdstr = NULL;
		GString *metastr, *stylestr;
		unsigned int i = 0;
		
		gtk_combo_box_get_active_iter (GTK_COMBO_BOX (qstart->dtd), &iter);
		model = gtk_combo_box_get_model (GTK_COMBO_BOX (qstart->dtd));
		gtk_tree_model_get (model, &iter, 0, &name, -1);
			
		if (strstr(name, "XHTML")) {
			xmlstr = g_strconcat ("<?xml version=\"1.0\" encoding=\"", main_v->props.newfile_default_encoding, "\"?>\n", NULL);
			tmpstr = g_strdup_printf ("%shttp://www.w3.org/1999/xhtml%sen%sen\">\n%s\n", cap("<HTML XMLNS=\""), cap("\" XML:LANG=\""), cap("\" LANG=\""), cap("<HEAD>"));
			endstr = g_strdup ("/>\n");
		} else {
			xmlstr = g_strdup ("");
			tmpstr = g_strdup_printf ("%s\n", cap("<HTML>\n<HEAD>"));
			endstr = g_strdup (">\n");
		}
		for (i = 0; i < G_N_ELEMENTS (dtds); i++) {
			if (strcmp(name, dtds[i].name) == 0) {
				dtdstr = g_strconcat (dtds[i].dtd, "\n", NULL);
			}
		}
		g_free (name);
		
		titlestr = g_strconcat (cap("<TITLE>"), gtk_entry_get_text (GTK_ENTRY (qstart->title)), cap("</TITLE>\n"), NULL);
		
		metastr = g_string_new ("");
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (qstart->metaView));
		if (gtk_tree_model_get_iter_first (model, &iter)) {
			do {
				gtk_tree_model_get (model, &iter, 0, &metatag, -1);
				tmpstr2 = g_strconcat ("<", metatag, endstr, NULL);
				g_free (metatag);				
				metastr = g_string_append (metastr, tmpstr2);
				g_free (tmpstr2);
			} while (gtk_tree_model_iter_next (model, &iter));
		}
		
		stylestr = g_string_new ("");
		gtk_combo_box_get_active_iter (GTK_COMBO_BOX (qstart->stylelinktype), &iter);
		model = gtk_combo_box_get_model (GTK_COMBO_BOX (qstart->stylelinktype));
		gtk_tree_model_get (model, &iter, 0, &name, -1);
		
		if (strcmp(name, "") != 0) {
			gchar *stylehref, *stylemedia;
			
			stylehref = gtk_editable_get_chars (GTK_EDITABLE (GTK_BIN (qstart->stylehref)->child), 0, -1);
			stylemedia = gtk_editable_get_chars (GTK_EDITABLE (qstart->stylemedia), 0, -1);
			
			if (strcmp(name, "Linked") == 0) {
				stylestr = g_string_append (stylestr, "<link rel=stylesheet type=\"text/css\" ");
				if (strlen(stylemedia) > 0) {
					tmpstr2 = g_strdup_printf ("media=\"%s\" href=\"%s\">\n", stylemedia, stylehref);
				} else {
					tmpstr2 = g_strdup_printf ("href=\"%s\">\n", stylehref);
				}
			} else {
				stylestr = g_string_append (stylestr, "@import url(");
				if (strlen(stylemedia) > 0) {
					tmpstr2 = g_strdup_printf ("%s) %s;\n", stylehref, stylemedia);
				} else {
					tmpstr2 = g_strdup_printf ("%s);\n", stylehref);
				}	
			}
			stylestr = g_string_append (stylestr, tmpstr2);

			g_free (tmpstr2);
			g_free (stylehref);
			g_free (stylemedia);	
		}
		g_free (name);
		
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (qstart->stylearea))) {
			tmpstr2 = g_strdup ("<style>\n\n</style>\n");
		} else {
			tmpstr2 = g_strdup ("");
		}
		
		finalstr = g_strconcat (xmlstr, dtdstr, tmpstr, titlestr, metastr->str, stylestr->str, tmpstr2, cap("</HEAD>\n<BODY>\n"), NULL);
		
		g_free (xmlstr);
		g_free (dtdstr);
		g_free (tmpstr);
		g_free (titlestr);
		g_string_free (metastr, TRUE);
		g_string_free (stylestr, TRUE);
		g_free (tmpstr2);
		
		/* FIXME: the old quick start dialog added the code to the current document.
		 * I think it would be better to open a new document and add the code to it 
		 * or possibly add an option "open is new document" or something similar
		 */
		doc_insert_two_strings(qstart->bfwin->current_document, finalstr, cap("\n</BODY>\n</HTML>"));	
		g_free (finalstr);
	}
	
	g_free (qstart);
	gtk_widget_destroy (GTK_WIDGET (dialog));
	DEBUG_MSG("quickstart_response_lcb() finished\n");
}

static GtkWidget *
quickstart_meta_page_create(TQuickStart *qstart)
{
	GtkWidget *hbox, *scrolwin, *button, *bbox;
	GtkListStore  *metaStore;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	hbox = gtk_hbox_new (FALSE, 6);
	
	scrolwin = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolwin), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request (scrolwin, 450, 200);
	gtk_box_pack_start (GTK_BOX (hbox), scrolwin, TRUE, TRUE, 0);
	
	metaStore = gtk_list_store_new (1, G_TYPE_STRING);
	quickstart_load_metatags(metaStore);

	qstart->metaView = gtk_tree_view_new ();
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (qstart->metaView), FALSE);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (qstart->metaView));
	g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (quickstart_meta_selection_changed), qstart);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	gtk_container_add (GTK_CONTAINER (scrolwin), qstart->metaView);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "editable", TRUE, NULL);
	g_signal_connect (renderer, "edited", G_CALLBACK (quickstart_meta_cell_edited), qstart);
	column = gtk_tree_view_column_new_with_attributes ("Meta Tag", renderer, "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (qstart->metaView), column);
	gtk_tree_view_set_model (GTK_TREE_VIEW (qstart->metaView), GTK_TREE_MODEL (metaStore));
	g_object_unref (metaStore);

	bbox = gtk_vbutton_box_new ();
	gtk_box_set_spacing (GTK_BOX (bbox), 6);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_START);
	button = gtk_button_new_from_stock (GTK_STOCK_ADD);
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (quickstart_meta_add_clicked), qstart);
	gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);
	qstart->removeButton = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
	g_signal_connect (G_OBJECT (qstart->removeButton), "clicked", G_CALLBACK (quickstart_meta_remove_clicked), qstart);
	gtk_box_pack_start (GTK_BOX (bbox), qstart->removeButton, FALSE, FALSE, 0);
	gtk_widget_set_sensitive (qstart->removeButton, FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), bbox, FALSE, FALSE, 0);
	
	return hbox;
}

/* TODO: create functions to reduce the duplicate code */
static GtkWidget *
quickstart_style_page_create(TQuickStart *qstart)
{
	GtkWidget *frame, *vbox, *vbox2, *hbox, *table, *label, *alignment;
	GtkListStore *history;
	unsigned int i = 0;

	const gchar *type[] = {
		"",
		"Linked",
		"Imported",
	};

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	vbox = gtk_vbox_new (FALSE, 12);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), _("<b>External style sheet</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);	
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	
	alignment = gtk_alignment_new (0, 0, 1, 1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 6);
	gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);	
	vbox2 = gtk_vbox_new (FALSE, 12);
	gtk_container_add (GTK_CONTAINER (alignment), vbox2);
	
	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
	
	qstart->stylelinktype = gtk_combo_box_new_text ();
	for (i = 0; i < G_N_ELEMENTS (type); i++) {
		gtk_combo_box_append_text (GTK_COMBO_BOX (qstart->stylelinktype), type[i]);
	}
	g_signal_connect (G_OBJECT (qstart->stylelinktype), "changed", G_CALLBACK (quickstart_stylelinktype_changed), qstart);
	label = gtk_label_new_with_mnemonic (_("_Type:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), qstart->stylelinktype);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), qstart->stylelinktype, FALSE, FALSE, 0);

	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 12);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
	
	/* TODO: this should be session history list */	
	history = gtk_list_store_new (1, G_TYPE_STRING);
	
	qstart->stylehref = gtk_combo_box_entry_new_with_model (GTK_TREE_MODEL (history), 0);
	g_object_unref (history);
	bf_mnemonic_label_tad_with_alignment(_("_Href:"), qstart->stylehref, 0, 0.5, table, 0, 1, 0, 1);
	gtk_table_attach (GTK_TABLE (table), qstart->stylehref, 1, 2, 0, 1, GTK_EXPAND|GTK_FILL, GTK_SHRINK, 0, 0);
	
	qstart->stylemedia = gtk_entry_new ();
	bf_mnemonic_label_tad_with_alignment(_("_Media:"), qstart->stylemedia, 0, 0.5, table, 0, 1, 1, 2);
	gtk_table_attach (GTK_TABLE (table), qstart->stylemedia, 1, 2, 1, 2, GTK_EXPAND|GTK_FILL, GTK_SHRINK, 0, 0);	

	/* TODO: add an option to place content in the style area
	 * Possibly from a code snippet library
	 */
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Style area</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);	
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	
	alignment = gtk_alignment_new (0, 0, 1, 1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 6);
	gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);	
	vbox2 = gtk_vbox_new (FALSE, 12);
	gtk_container_add (GTK_CONTAINER (alignment), vbox2);
	
	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
	qstart->stylearea = gtk_check_button_new_with_mnemonic (_("_Create empty style area"));
	gtk_box_pack_start (GTK_BOX (hbox), qstart->stylearea, FALSE, FALSE, 0);	

	gtk_combo_box_set_active (GTK_COMBO_BOX (qstart->stylelinktype), 0);

	return frame;
}

static GtkWidget *
quickstart_script_page_create(TQuickStart *qstart)
{
	GtkWidget *hbox;

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new ("script page"), TRUE, TRUE, 0);
	return hbox;
}

void 
quickstart_dialog_new(Tbfwin *bfwin) {
	TQuickStart *qstart;
	GtkWidget *dialog, *table, *frame, *page;
	GtkListStore *headStore;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	unsigned int i = 0;
	
	const gchar *headstr[] = {
		"Meta",
		"Style",
		"Script",
	};
	
	qstart = g_new (TQuickStart, 1);
	qstart->bfwin = bfwin;
	
	dialog = gtk_dialog_new_with_buttons (_("Quick Start"), GTK_WINDOW (bfwin->main_window),
													  GTK_DIALOG_DESTROY_WITH_PARENT,
													  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
													  GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
													  NULL);	  
	g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (quickstart_response_lcb), qstart);

	table = gtk_table_new (5, 3, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 6);
	gtk_table_set_row_spacings (GTK_TABLE (table), 12);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table, TRUE, TRUE, 0);
	
	qstart->dtd = gtk_combo_box_new_text ();
	for (i = 0; i < G_N_ELEMENTS (dtds); i++) {
		gtk_combo_box_append_text (GTK_COMBO_BOX (qstart->dtd), dtds[i].name);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (qstart->dtd), 0);
	bf_mnemonic_label_tad_with_alignment(_("_DTD:"), qstart->dtd, 0, 0.5, table, 0, 1, 0, 1);
	gtk_table_attach (GTK_TABLE (table), qstart->dtd, 1, 2, 0, 1, GTK_FILL, GTK_SHRINK, 0, 0);
	
	qstart->title = gtk_entry_new ();
	bf_mnemonic_label_tad_with_alignment(_("_Title:"), qstart->title, 0, 0.5, table, 0, 1, 1, 2);
	gtk_table_attach (GTK_TABLE (table), qstart->title, 1, 2, 1, 2, GTK_FILL, GTK_SHRINK, 0, 0);

	headStore = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_UINT);
	for (i = 0; i < G_N_ELEMENTS (headstr); i++) {
		gtk_list_store_append (headStore, &iter);
		gtk_list_store_set (headStore, &iter, 0, headstr[i], 1, i, -1);
	}

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	qstart->headView = gtk_tree_view_new ();
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (qstart->headView), FALSE);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (qstart->headView));
	g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (quickstart_head_selection_changed), qstart);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	gtk_container_add (GTK_CONTAINER (frame), qstart->headView);
	gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 3, 4, GTK_SHRINK, GTK_FILL, 0, 0);	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Head", renderer, "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (qstart->headView), column);
	gtk_tree_view_set_model (GTK_TREE_VIEW (qstart->headView), GTK_TREE_MODEL (headStore));
	g_object_unref (headStore);
	bf_mnemonic_label_tad_with_alignment(_("_Head:"), qstart->headView, 0, 0, table, 0, 1, 2, 3);
	
	qstart->notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (qstart->notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (qstart->notebook), FALSE);
	gtk_table_attach_defaults (GTK_TABLE (table), qstart->notebook, 1, 3, 3, 4); 
	
	page = quickstart_meta_page_create(qstart);
	gtk_notebook_append_page (GTK_NOTEBOOK (qstart->notebook), page, NULL);
	page = quickstart_style_page_create(qstart);
	gtk_notebook_append_page (GTK_NOTEBOOK (qstart->notebook), page, NULL);
	page = quickstart_script_page_create(qstart);
	gtk_notebook_append_page (GTK_NOTEBOOK (qstart->notebook), page, NULL);	
	
	gtk_widget_show_all (dialog);
}
