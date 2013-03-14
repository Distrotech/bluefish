/* Bluefish HTML Editor
 * html_table.c - table dialoges
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * Copyright (C) 1999-2010 Olivier Sessink
 * Copyright (C) 2011 James Hayward
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

#include <string.h>

#include "html_table.h"
#include "htmlbar.h"
#include "cap.h"
#include "html2.h"				/* style_but_new */
#include "html_diag.h"
#include "../dialog_utils.h"
#include "../document.h"
#include "../gtk_easy.h"
#include "../stringlist.h"

static void
table_border_clicked_lcb(GtkWidget * checkbutton, Thtml_diag * dg)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton))){
		gtk_widget_set_sensitive(dg->spin[4], FALSE);
		gtk_widget_set_sensitive(dg->check[2], FALSE);
	} else {
		gtk_widget_set_sensitive(dg->spin[4], TRUE);
		gtk_widget_set_sensitive(dg->check[2], TRUE);
	}
}

static void
tabledialogok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<TABLE"));
	thestring = insert_if_spin(dg->spin[1], cap("CELLPADDING"), thestring,
							   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[4])));							   
	thestring = insert_if_spin(dg->spin[3], cap("CELLSPACING"), thestring,
							   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[5])));	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[3]))) {
		thestring = insert_attr_if_checkbox(dg->check[3],
			main_v->props.xhtml == 1 ? cap("BORDER=\"border\"") : cap("BORDER"), thestring);
	} else {	thestring =	insert_if_spin(dg->spin[4], cap("BORDER"), thestring,
							   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[2])));
	}						   
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[1]), cap("ALIGN"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[3]), cap("BGCOLOR"), thestring, NULL);
	thestring =	insert_if_spin(dg->spin[2], cap("WIDTH"), thestring,
							   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[1])));
	thestring =	insert_string_if_entry(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(dg->combo[4]))), cap("CLASS"), 
								thestring, NULL);
	thestring =	insert_string_if_entry(GTK_ENTRY(dg->entry[3]), cap("ID"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[2]), cap("STYLE"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[5]), cap("FRAME"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[6]), cap("RULES"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[1]), NULL, thestring, NULL);
	finalstring = g_strconcat(thestring, ">", NULL);
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</TABLE>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}

	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}


void
tabledialog_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	GList *alignlist = NULL, *popuplist = NULL;
	GtkWidget *var_but, *dgtable;

	static gchar *tagitems[] =
		{ "cellpadding", "cellspacing", "border", "align", "bgcolor", "width", "class", "style",
"rules", "frame", "id", NULL
	};
	gchar *tagvalues[13];
	gchar *custom = NULL;
	Thtml_diag *dg;

	dg = html_diag_new(bfwin, _("Table"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 8);

	dg->spin[1] = spinbut_with_value(NULL, 0, 100, 1.0, 5.0);
	dg->check[4] = gtk_check_button_new_with_label("%");
	dialog_mnemonic_label_in_table(_("<span color=\"#006000\">Cell _Padding:</span>"), 
				dg->spin[1], dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[4], 2, 3, 0, 1);
	/*g_print("cell padding='%s'\n",tagvalues[0]);*/
	parse_integer_for_dialog(tagvalues[0], dg->spin[1], NULL, dg->check[4]);

	dg->spin[3] = spinbut_with_value(NULL, 0, 100, 1.0, 5.0);
	dg->check[5] = gtk_check_button_new_with_label("%");
	dialog_mnemonic_label_in_table(_("<span color=\"#006000\">C_ell Spacing:</span>"), 
				dg->spin[3], dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[3], 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[5], 2, 3, 1, 2);
	parse_integer_for_dialog(tagvalues[1], dg->spin[3], NULL, dg->check[5]);

	dg->entry[3] = dialog_entry_in_table(tagvalues[10], dgtable, 4, 5, 1, 2);
	dialog_mnemonic_label_in_table(_("_Id:"), dg->entry[3], dgtable, 3, 4, 1, 2);

	dg->combo[4] = html_diag_combobox_with_popdown_sized(tagvalues[6], bfwin->session->classlist, 1, 90);
	dialog_mnemonic_label_in_table(_("Cl_ass:"), dg->combo[4], dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[4], 1, 2, 2, 3);

	dg->entry[2] = dialog_entry_in_table(tagvalues[7], dgtable, 1, 4, 3, 4);
	dialog_mnemonic_label_in_table(_("St_yle:"), dg->entry[2], dgtable, 0, 1, 3, 4);
	var_but = style_but_new(dg->entry[2], dg->dialog);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), var_but, 4, 5, 3, 4);

	dg->entry[1] = dialog_entry_in_table(custom, dgtable, 1, 5, 4, 5);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dg->entry[1], dgtable, 0, 1, 4, 5);

	alignlist = g_list_append(NULL, "");
	alignlist = g_list_append(alignlist, "left");
	alignlist = g_list_append(alignlist, "right");
	alignlist = g_list_append(alignlist, "center");
	dg->combo[1] = html_diag_combobox_with_popdown_sized(tagvalues[3], alignlist, 0, 90);
	g_list_free(alignlist);
	dialog_mnemonic_label_in_table(_("<span color=\"#006000\">_Align:</span>"), dg->combo[1], dgtable, 3, 4, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[1], 4, 5, 0, 1);

	dg->combo[3] = html_diag_combobox_with_popdown_sized(tagvalues[4], bfwin->session->colorlist, 1, 90);
	var_but = color_but_new(gtk_bin_get_child(GTK_BIN(dg->combo[3])), dg->dialog);
	dialog_mnemonic_label_in_table(_("<span color=\"red\">_bgcolor:</span>"), dg->combo[3], dgtable, 2, 3, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[3], 3, 4, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), var_but, 4, 5, 2, 3);

	dg->spin[2] = spinbut_with_value(NULL, 0, 10000, 1.0, 5.0);
	dg->check[1] = gtk_check_button_new_with_label("%");
	dialog_mnemonic_label_in_table(_("<span color=\"#006000\">_Width:</span>"), dg->spin[2], dgtable, 5, 6, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[2], 6, 7, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 7, 8, 0, 1);
	parse_integer_for_dialog(tagvalues[5], dg->spin[2], NULL, dg->check[1]);

	dg->spin[4] = spinbut_with_value(NULL, 0, 100, 1.0, 5.0);
	dg->check[2] = gtk_check_button_new_with_label("%");
	dialog_mnemonic_label_in_table(_("<span color=\"#006000\">Bo_rder:</span>"), dg->spin[4], dgtable, 5, 6, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[4], 6, 7, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[2], 7, 8, 1, 2);
	parse_integer_for_dialog(tagvalues[2], dg->spin[4], NULL, dg->check[2]);

	dg->check[3] = gtk_check_button_new();
	dialog_mnemonic_label_in_table(_("<span color=\"#A36A00\">_Border:</span>"), dg->check[3], dgtable, 5,6, 2,3);
	g_signal_connect(dg->check[3], "clicked", G_CALLBACK(table_border_clicked_lcb), dg);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[3], 6,7, 2,3);	

	popuplist = g_list_append(NULL, "");
	popuplist = g_list_append(popuplist, "void");
	popuplist = g_list_append(popuplist, "above");
	popuplist = g_list_append(popuplist, "below");
	popuplist = g_list_append(popuplist, "hsides");
	popuplist = g_list_append(popuplist, "vsides");
	popuplist = g_list_append(popuplist, "lhs");
	popuplist = g_list_append(popuplist, "rhs");
	popuplist = g_list_append(popuplist, "box");
	popuplist = g_list_append(popuplist, "border");
	dg->combo[5] = html_diag_combobox_with_popdown_sized(tagvalues[9], popuplist, 0, 90);
	dialog_mnemonic_label_in_table(_("<span color=\"#006000\">_Frame:</span>"), dg->combo[5], dgtable, 5, 6, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[5], 6, 8, 3, 4);
	g_list_free(popuplist);

	popuplist = g_list_append(NULL, "");
	popuplist = g_list_append(popuplist, "none");
	popuplist = g_list_append(popuplist, "groups");
	popuplist = g_list_append(popuplist, "rows");
	popuplist = g_list_append(popuplist, "cols");
	popuplist = g_list_append(popuplist, "all");
	dg->combo[6] = html_diag_combobox_with_popdown_sized(tagvalues[8], popuplist, 0, 90);
	dialog_mnemonic_label_in_table(_("<span color=\"#006000\">R_ules:</span>"), dg->combo[6], dgtable, 5, 6, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[6], 6, 8, 4, 5);
	g_list_free(popuplist);

	html_diag_finish(dg, G_CALLBACK(tabledialogok_lcb));

	if (custom)
		g_free(custom);
}

static void
tablerowdialogok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<TR"));
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[1]), cap("ALIGN"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[2]), cap("VALIGN"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[3]), cap("BGCOLOR"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[4]), cap("CLASS"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[2]), cap("STYLE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[1]), NULL, thestring, NULL);

	finalstring = g_strconcat(thestring, ">", NULL);
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</TR>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}


void
tablerowdialog_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	GList *alignlist = NULL;
	GtkWidget *color_but, *var_but;

	static gchar *tagitems[] = { "align", "valign", "bgcolor", "class", "style", NULL };
	gchar *tagvalues[6];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable;

	dg = html_diag_new(bfwin, _("Table Row"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 4, 5);

	alignlist = g_list_append(NULL, "");
	alignlist = g_list_insert(alignlist, "left", 0);
	alignlist = g_list_insert(alignlist, "right", 1);
	alignlist = g_list_insert(alignlist, "center", 2);
	dg->combo[1] = html_diag_combobox_with_popdown_sized(tagvalues[0], alignlist, 0, 90);
	g_list_free(alignlist);
	dialog_mnemonic_label_in_table(_("<span color=\"#006000\">_Align:</span>"), dg->combo[1], dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(dg->combo[1]), 1, 2, 0, 1);

	alignlist = g_list_append(NULL, "");
	alignlist = g_list_insert(alignlist, "top", 0);
	alignlist = g_list_insert(alignlist, "middle", 1);
	alignlist = g_list_insert(alignlist, "bottom", 2);
	alignlist = g_list_insert(alignlist, "baseline", 3);
	dg->combo[2] = html_diag_combobox_with_popdown_sized(tagvalues[1], alignlist, 0, 90);
	g_list_free(alignlist);

	dialog_mnemonic_label_in_table(_("<span color=\"#006000\">_Valign:</span>"), dg->combo[2], dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[2])), 1, 2, 1, 2);

	dg->combo[4] = html_diag_combobox_with_popdown_sized(tagvalues[3], bfwin->session->classlist, 1, 90);
	dialog_mnemonic_label_in_table(_("Cl_ass:"), dg->combo[4], dgtable, 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[4], 3, 5, 0, 1);

	dg->combo[3] = html_diag_combobox_with_popdown_sized(tagvalues[2], bfwin->session->colorlist, 1, 90);
	color_but = color_but_new(gtk_bin_get_child(GTK_BIN(dg->combo[3])), dg->dialog);
	dialog_mnemonic_label_in_table(_("<span color=\"red\">_bgcolor:</span>"), dg->combo[3], dgtable, 2, 3, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[3])), 3, 4, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(color_but), 4, 5, 1, 2);

	dg->entry[2] = dialog_entry_in_table(tagvalues[4], dgtable, 1, 4, 2, 3);
	dialog_mnemonic_label_in_table(_("St_yle:"), dg->entry[2], dgtable, 0, 1, 2, 3);
	var_but = style_but_new(dg->entry[2], dg->dialog);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), var_but, 4, 5, 2, 3);

	dg->entry[1] = dialog_entry_in_table(custom, dgtable, 1, 5, 3, 4);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dg->entry[1], dgtable, 0, 1, 3, 4);

	html_diag_finish(dg, G_CALLBACK(tablerowdialogok_lcb));

	if (custom)
		g_free(custom);
}

static void
table_head_and_data_dialogok_lcb(gint type, GtkWidget * widget, Thtml_diag * dg)
{

	gchar *thestring, *finalstring;

	if (type == 1) {
		thestring = g_strdup(cap("<TD"));
	} else {
		thestring = g_strdup(cap("<TH"));
		thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[5]), cap("SCOPE"), thestring, NULL);
	}
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[3]), cap("HEADERS"), thestring, NULL);
	thestring = insert_integer_if_spin(dg->spin[5], cap("COLSPAN"), thestring, FALSE, 0);
	thestring = insert_integer_if_spin(dg->spin[4], cap("ROWSPAN"), thestring, FALSE, 0);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[4]), cap("CLASS"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[2]), cap("STYLE"), thestring, NULL);
	thestring =	insert_integer_if_spin(dg->spin[1], cap("WIDTH"), thestring,
							gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[2])), 0);
	thestring =	insert_integer_if_spin(dg->spin[3], cap("HEIGHT"), thestring,
							gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[3])), 0);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[1]), cap("ALIGN"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[2]), cap("VALIGN"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[3]), cap("BGCOLOR"), thestring, NULL);
	thestring = insert_attr_if_checkbox(dg->check[1],
							main_v->props.xhtml == 1 ? cap("NOWRAP=\"nowrap\"") : cap("NOWRAP"), thestring);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[1]), NULL, thestring, NULL);
	finalstring = g_strconcat(thestring, ">", NULL);
	g_free(thestring);

	if (dg->range.end == -1) {
		if (type == 1) {
			doc_insert_two_strings(dg->doc, finalstring, cap("</TD>"));
		} else {
			doc_insert_two_strings(dg->doc, finalstring, cap("</TH>"));
		}
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

static void
tabledatadialogok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	table_head_and_data_dialogok_lcb(1, widget, dg);
}

static void
tableheaddialogok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	table_head_and_data_dialogok_lcb(0, widget, dg);
}

static void
table_head_and_data_dialog_cb(gint type, Tbfwin * bfwin, Ttagpopup * data)
{
	GList *alignlist = NULL;

	static gchar *tagitems[] =
		{ "width", "align", "colspan", "height", "valign", "rowspan", "bgcolor", "nowrap", "class", "style", 
		"headers", "scope", NULL
	};
	gchar *tagvalues[12];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable, *var_but;

	if (type == 1) {
		dg = html_diag_new(bfwin, _("Table Data"));
	} else {
		dg = html_diag_new(bfwin, _("Table Header"));
	}

	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 6, 7);

	alignlist = g_list_append(NULL, "");
	alignlist = g_list_insert(alignlist, "left", 0);
	alignlist = g_list_insert(alignlist, "right", 1);
	alignlist = g_list_insert(alignlist, "center", 2);
	alignlist = g_list_insert(alignlist, "justify", 3);
	dg->combo[1] = html_diag_combobox_with_popdown_sized(tagvalues[1], alignlist, 0, 90);
	g_list_free(alignlist);
	dialog_mnemonic_label_in_table(_("<span color=\"#006000\">_Align:</span>"), dg->combo[1], dgtable, 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[1])), 3, 4, 0, 1);

	alignlist = g_list_append(NULL, "");
	alignlist = g_list_insert(alignlist, "top", 0);
	alignlist = g_list_insert(alignlist, "middle", 1);
	alignlist = g_list_insert(alignlist, "bottom", 2);
	alignlist = g_list_insert(alignlist, "baseline", 3);
	dg->combo[2] = html_diag_combobox_with_popdown_sized(tagvalues[4], alignlist, 0, 90);
	g_list_free(alignlist);

	dialog_mnemonic_label_in_table(_("<span color=\"#006000\">_Valign:</span>"), dg->combo[2], dgtable, 2, 3, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[2])), 3, 4, 1, 2);

	dg->combo[4] = html_diag_combobox_with_popdown_sized(tagvalues[8], bfwin->session->classlist, 1, 90);
	dialog_mnemonic_label_in_table(_("Cl_ass:"), dg->combo[4], dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[4])), 1, 2, 2, 3);

	dg->entry[3] = dialog_entry_in_table(tagvalues[10], dgtable, 1, 4, 3, 4);
	dialog_mnemonic_label_in_table(_("<span color=\"#A36A00\">_Headers:</span>"), dg->entry[3], dgtable, 0, 1, 3, 4);
	gtk_widget_set_tooltip_text(dg->entry[3],_("Set of space-separated IDs of th elements."));

	if (type == 0) {
		alignlist = g_list_append(NULL, "");
		alignlist = g_list_insert(alignlist, "row", 0);
		alignlist = g_list_insert(alignlist, "col", 1);
		alignlist = g_list_insert(alignlist, "rowgroup", 2);
		alignlist = g_list_insert(alignlist, "colgroup", 3);
		alignlist = g_list_insert(alignlist, "auto", 4);		
		dg->combo[5] = html_diag_combobox_with_popdown_sized(tagvalues[11], alignlist, 0, 90);
		g_list_free(alignlist);
		dialog_mnemonic_label_in_table(_("<span color=\"#A36A00\">_Scope:</span>"), dg->combo[5], dgtable, 4, 5, 3, 4);
		gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[5])), 5, 6, 3, 4);
		gtk_widget_set_tooltip_text(dg->combo[5],_("the header cell applies to cells in the same:"));
	}

	dg->entry[2] = dialog_entry_in_table(tagvalues[9], dgtable, 1, 6, 4, 5);
	dialog_mnemonic_label_in_table(_("St_yle:"), dg->entry[2], dgtable, 0, 1, 4, 5);
	var_but = style_but_new(dg->entry[2], dg->dialog);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), var_but, 6, 7, 4, 5);

	dg->entry[1] = dialog_entry_in_table(custom, dgtable, 1, 7, 5,6);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dg->entry[1], dgtable, 0, 1, 5, 6);

	dg->spin[5] = spinbut_with_value(tagvalues[2], 0, 100, 1.0, 2.0);
	dialog_mnemonic_label_in_table(_("Co_l Span:"), dg->spin[5], dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[5], 1, 2, 0, 1);

	dg->spin[4] = spinbut_with_value(tagvalues[5], 0, 100, 1.0, 5.0);
	dialog_mnemonic_label_in_table(_("_Row Span:"), dg->spin[4], dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[4], 1, 2, 1, 2);

	dg->check[1] = gtk_check_button_new();
	parse_existence_for_dialog(tagvalues[7], dg->check[1]);
	dialog_mnemonic_label_in_table(_("<span color=\"red\">No _Wrap:</span>"), dg->check[1], dgtable, 2, 3, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 3, 4, 2, 3);

	dg->spin[1] = spinbut_with_value(NULL, 0, 10000, 1.0, 5.0);
	dg->check[2] = gtk_check_button_new_with_label("%");
	parse_integer_for_dialog(tagvalues[0], dg->spin[1], NULL, dg->check[2]);
	dialog_mnemonic_label_in_table(_("<span color=\"red\">_Width:</span>"), dg->spin[1], dgtable, 4, 5, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 5, 6, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[2], 6, 7, 0, 1);

	dg->spin[3] = spinbut_with_value(NULL, 0, 10000, 1.0, 5.0);
	dg->check[3] = gtk_check_button_new_with_label("%");
	parse_integer_for_dialog(tagvalues[3], dg->spin[3], NULL, dg->check[3]);
	dialog_mnemonic_label_in_table(_("<span color=\"red\">_Height:</span>"), dg->spin[3], dgtable, 4, 5, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[3], 5, 6, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[3], 6, 7, 1, 2);

	dg->combo[3] = html_diag_combobox_with_popdown_sized(tagvalues[6], bfwin->session->colorlist, 1, 90);
	dialog_mnemonic_label_in_table(_("<span color=\"red\">_bgcolor:</span>"), dg->combo[3], dgtable, 4, 5, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[3])), 5, 6, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable),
							  GTK_WIDGET(color_but_new(gtk_bin_get_child(GTK_BIN(dg->combo[3])), dg->dialog)),
							  6, 7, 2, 3);

	if (type == 1) {
		html_diag_finish(dg, G_CALLBACK(tabledatadialogok_lcb));
	} else {
		html_diag_finish(dg, G_CALLBACK(tableheaddialogok_lcb));
	}

	if (custom)
		g_free(custom);
}

void
tabledatadialog_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	table_head_and_data_dialog_cb(1, bfwin, data);
}

void
tableheaddialog_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	table_head_and_data_dialog_cb(0, bfwin, data);
}
