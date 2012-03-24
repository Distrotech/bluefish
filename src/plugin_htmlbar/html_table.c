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
tabledialogok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<TABLE"));
	thestring = insert_string_if_entry(GTK_ENTRY(dg->spin[1]), cap("CELLPADDING"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->spin[3]), cap("CELLSPACING"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->spin[4]), cap("BORDER"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[1]), cap("ALIGN"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[2]), cap("VALIGN"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[3]), cap("BGCOLOR"), thestring, NULL);
	thestring =
		insert_integer_if_spin(dg->spin[2], cap("WIDTH"), thestring,
							   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[1])), 0);
	thestring =
		insert_string_if_entry(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(dg->combo[4]))), cap("CLASS"), thestring,
							   NULL);
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
	GList *alignlist = NULL, *popuplist;
	GtkWidget *var_but, *dgtable;

	static gchar *tagitems[] =
		{ "cellpadding", "cellspacing", "border", "align", "valign", "bgcolor", "width", "class", "style",
"rules", "frame", NULL
	};
	gchar *tagvalues[12];
	gchar *custom = NULL;
	Thtml_diag *dg;

	dg = html_diag_new(bfwin, _("Table"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 8);

	dg->spin[1] = spinbut_with_value(tagvalues[0], 0, 100, 1.0, 5.0);
	dialog_mnemonic_label_in_table(_("Cell _Padding:"), dg->spin[1], dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1, 2, 0, 1);

	dg->spin[3] = spinbut_with_value(tagvalues[1], 0, 100, 1.0, 5.0);
	dialog_mnemonic_label_in_table(_("C_ell Spacing:"), dg->spin[3], dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[3], 1, 2, 1, 2);

	dg->combo[4] = combobox_with_popdown_sized(tagvalues[7], bfwin->session->classlist, 1, 80);
	dialog_mnemonic_label_in_table(_("Cl_ass:"), dg->combo[4], dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[4], 1, 2, 2, 3);

	dg->entry[2] = dialog_entry_in_table(tagvalues[8], dgtable, 1, 4, 3, 4);
	dialog_mnemonic_label_in_table(_("St_yle:"), dg->entry[2], dgtable, 0, 1, 3, 4);
	var_but = style_but_new(dg->entry[2], dg->dialog);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), var_but, 4, 5, 3, 4);

	dg->entry[1] = dialog_entry_in_table(custom, dgtable, 1, 5, 4, 5);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dg->entry[1], dgtable, 0, 1, 4, 5);

	alignlist = g_list_append(alignlist, "left");
	alignlist = g_list_append(alignlist, "right");
	alignlist = g_list_append(alignlist, "center");
	dg->combo[1] = combobox_with_popdown_sized(tagvalues[3], alignlist, 0, 80);
	g_list_free(alignlist);
	dialog_mnemonic_label_in_table(_("Ali_gn:"), dg->combo[1], dgtable, 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[1], 3, 5, 0, 1);

	alignlist = NULL;
	alignlist = g_list_insert(alignlist, "top", 0);
	alignlist = g_list_insert(alignlist, "middle", 1);
	alignlist = g_list_insert(alignlist, "bottom", 2);
	alignlist = g_list_insert(alignlist, "texttop", 3);
	alignlist = g_list_insert(alignlist, "baseline", 4);
	alignlist = g_list_insert(alignlist, "absmiddle", 5);
	alignlist = g_list_insert(alignlist, "absbottom", 6);
	dg->combo[2] = combobox_with_popdown_sized(tagvalues[4], alignlist, 1, 80);
	g_list_free(alignlist);

	dialog_mnemonic_label_in_table(_("_Valign:"), dg->combo[2], dgtable, 2, 3, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[2], 3, 5, 1, 2);

	dg->combo[3] = combobox_with_popdown_sized(tagvalues[5], bfwin->session->colorlist, 1, 80);
	var_but = color_but_new(gtk_bin_get_child(GTK_BIN(dg->combo[3])), dg->dialog);
	dialog_mnemonic_label_in_table(_("Backgrou_nd Color:"), dg->combo[3], dgtable, 2, 3, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[3], 3, 4, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), var_but, 4, 5, 2, 3);

	dg->spin[2] = spinbut_with_value(NULL, 0, 10000, 1.0, 5.0);
	dg->check[1] = gtk_check_button_new_with_label("%");
	dialog_mnemonic_label_in_table(_("_Width:"), dg->spin[2], dgtable, 5, 6, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[2], 6, 7, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 7, 8, 0, 1);
	parse_integer_for_dialog(tagvalues[6], dg->spin[2], NULL, dg->check[1]);

	dg->spin[4] = spinbut_with_value(tagvalues[2], 0, 100, 1.0, 5.0);
	dialog_mnemonic_label_in_table(_("Bo_rder:"), dg->spin[4], dgtable, 5, 6, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[4], 6, 8, 1, 2);

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
	dg->combo[5] = combobox_with_popdown_sized(tagvalues[10], popuplist, 0, 80);
	dialog_mnemonic_label_in_table(_("_Frame:"), dg->combo[5], dgtable, 5, 6, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[5], 6, 8, 2, 3);
	g_list_free(popuplist);

	popuplist = g_list_append(NULL, "");
	popuplist = g_list_append(popuplist, "none");
	popuplist = g_list_append(popuplist, "groups");
	popuplist = g_list_append(popuplist, "rows");
	popuplist = g_list_append(popuplist, "cols");
	popuplist = g_list_append(popuplist, "all");
	dg->combo[6] = combobox_with_popdown_sized(tagvalues[9], popuplist, 0, 80);
	dialog_mnemonic_label_in_table(_("R_ules:"), dg->combo[6], dgtable, 5, 6, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[6], 6, 8, 3, 4);
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

	alignlist = g_list_insert(alignlist, "left", 0);
	alignlist = g_list_insert(alignlist, "right", 1);
	alignlist = g_list_insert(alignlist, "center", 2);
	dg->combo[1] = combobox_with_popdown_sized(tagvalues[0], alignlist, 0, 90);
	g_list_free(alignlist);
	dialog_mnemonic_label_in_table(_("Ali_gn:"), dg->combo[1], dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(dg->combo[1]), 1, 2, 0, 1);

	alignlist = NULL;
	alignlist = g_list_insert(alignlist, "top", 0);
	alignlist = g_list_insert(alignlist, "middle", 1);
	alignlist = g_list_insert(alignlist, "bottom", 2);
	alignlist = g_list_insert(alignlist, "texttop", 3);
	alignlist = g_list_insert(alignlist, "baseline", 4);
	alignlist = g_list_insert(alignlist, "absmiddle", 5);
	alignlist = g_list_insert(alignlist, "absbottom", 6);
	dg->combo[2] = combobox_with_popdown_sized(tagvalues[1], alignlist, 0, 90);
	g_list_free(alignlist);

	dialog_mnemonic_label_in_table(_("_Valign:"), dg->combo[2], dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[2])), 1, 2, 1, 2);

	dg->combo[4] = combobox_with_popdown_sized(tagvalues[3], bfwin->session->classlist, 1, 90);
	dialog_mnemonic_label_in_table(_("Cl_ass:"), dg->combo[4], dgtable, 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[4], 3, 5, 0, 1);

	dg->combo[3] = combobox_with_popdown_sized(tagvalues[2], bfwin->session->colorlist, 1, 90);
	color_but = color_but_new(gtk_bin_get_child(GTK_BIN(dg->combo[3])), dg->dialog);
	dialog_mnemonic_label_in_table(_("Backgrou_nd Color:"), dg->combo[3], dgtable, 2, 3, 1, 2);
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
	}
	thestring =
		insert_integer_if_spin(dg->spin[1], cap("WIDTH"), thestring,
							   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[2])), 0);
	thestring =
		insert_integer_if_spin(dg->spin[3], cap("HEIGHT"), thestring,
							   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[3])), 0);
	thestring = insert_integer_if_spin(dg->spin[5], cap("COLSPAN"), thestring, FALSE, 0);
	thestring = insert_integer_if_spin(dg->spin[4], cap("ROWSPAN"), thestring, FALSE, 0);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[1]), cap("ALIGN"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[2]), cap("VALIGN"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[3]), cap("BGCOLOR"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[4]), cap("CLASS"), thestring, NULL);
	thestring =
		insert_attr_if_checkbox(dg->check[1],
								main_v->props.xhtml == 1 ? cap("NOWRAP=\"nowrap\"") : cap("NOWRAP"),
								thestring);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[2]), cap("STYLE"), thestring, NULL);
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
NULL
	};
	gchar *tagvalues[11];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable, *var_but;

	if (type == 1) {
		dg = html_diag_new(bfwin, _("Table Data"));
	} else {
		dg = html_diag_new(bfwin, _("Table Header"));
	}

	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 7);

	alignlist = g_list_insert(alignlist, "left", 0);
	alignlist = g_list_insert(alignlist, "right", 1);
	alignlist = g_list_insert(alignlist, "center", 2);
	alignlist = g_list_insert(alignlist, "justify", 3);
	dg->combo[1] = combobox_with_popdown_sized(tagvalues[1], alignlist, 1, 80);
	g_list_free(alignlist);
	dialog_mnemonic_label_in_table(_("Ali_gn:"), dg->combo[1], dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[1])), 1, 2, 0, 1);

	alignlist = NULL;
	alignlist = g_list_insert(alignlist, "top", 0);
	alignlist = g_list_insert(alignlist, "middle", 1);
	alignlist = g_list_insert(alignlist, "bottom", 2);
	alignlist = g_list_insert(alignlist, "texttop", 3);
	alignlist = g_list_insert(alignlist, "baseline", 4);
	alignlist = g_list_insert(alignlist, "absmiddle", 5);
	alignlist = g_list_insert(alignlist, "absbottom", 6);
	dg->combo[2] = combobox_with_popdown_sized(tagvalues[4], alignlist, 1, 90);
	g_list_free(alignlist);

	dialog_mnemonic_label_in_table(_("_Valign:"), dg->combo[2], dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[2])), 1, 2, 1, 2);

	dg->combo[4] = combobox_with_popdown_sized(tagvalues[8], bfwin->session->classlist, 1, 80);
	dialog_mnemonic_label_in_table(_("Cl_ass:"), dg->combo[4], dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[4])), 1, 2, 2, 3);

	dg->entry[2] = dialog_entry_in_table(tagvalues[9], dgtable, 1, 6, 3, 4);
	dialog_mnemonic_label_in_table(_("St_yle:"), dg->entry[2], dgtable, 0, 1, 3, 4);
	var_but = style_but_new(dg->entry[2], dg->dialog);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), var_but, 6, 7, 3, 4);

	dg->entry[1] = dialog_entry_in_table(custom, dgtable, 1, 7, 4, 5);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dg->entry[1], dgtable, 0, 1, 4, 5);

	dg->spin[5] = spinbut_with_value(tagvalues[2], 0, 100, 1.0, 2.0);
	dialog_mnemonic_label_in_table(_("Co_l Span:"), dg->spin[5], dgtable, 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[5], 3, 4, 0, 1);

	dg->spin[4] = spinbut_with_value(tagvalues[5], 0, 100, 1.0, 5.0);
	dialog_mnemonic_label_in_table(_("_Row Span:"), dg->spin[4], dgtable, 2, 3, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[4], 3, 4, 1, 2);

	dg->check[1] = gtk_check_button_new();
	parse_existence_for_dialog(tagvalues[7], dg->check[1]);
	dialog_mnemonic_label_in_table(_("No Wra_p:"), dg->check[1], dgtable, 2, 3, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 3, 4, 2, 3);

	dg->spin[1] = spinbut_with_value(NULL, 0, 10000, 1.0, 5.0);
	dg->check[2] = gtk_check_button_new_with_label("%");
	parse_integer_for_dialog(tagvalues[0], dg->spin[1], NULL, dg->check[2]);
	dialog_mnemonic_label_in_table(_("_Width:"), dg->spin[1], dgtable, 4, 5, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 5, 6, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[2], 6, 7, 0, 1);

	dg->spin[3] = spinbut_with_value(NULL, 0, 10000, 1.0, 5.0);
	dg->check[3] = gtk_check_button_new_with_label("%");
	parse_integer_for_dialog(tagvalues[3], dg->spin[3], NULL, dg->check[3]);
	dialog_mnemonic_label_in_table(_("_Height:"), dg->spin[3], dgtable, 4, 5, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[3], 5, 6, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[3], 6, 7, 1, 2);

	dg->combo[3] = combobox_with_popdown_sized(tagvalues[6], bfwin->session->colorlist, 1, 80);
	dialog_mnemonic_label_in_table(_("Backgrou_nd Color:"), dg->combo[3], dgtable, 4, 5, 2, 3);
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
