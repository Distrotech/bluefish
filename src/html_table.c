/* Bluefish HTML Editor
 * html.c - menu/toolbar callbacks, inserting functions, and other cool stuff 
 *
 * Copyright (C) 
 * 1998 Olivier Sessink and Chris Mazuc
 * 1999-2000 Olivier Sessink
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

#include <gtk/gtk.h>
#include <string.h>

#include "cap.h"  /* cap() */
#include "bluefish.h"
/*#include "coloursel.h"*/ /* colorbut_new() */
#include "html_diag.h"
#include "html_table.h"
#include "html2.h" /* style_but_new */
/*#include "init.h"*/ 	/* positionlist */
#include "gtk_easy.h"
#include "document.h"
#include "stringlist.h"



static void tabledialogok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<TABLE"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->spin[1])), cap("CELLPADDING"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->spin[3])), cap("CELLSPACING"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->spin[4])), cap("BORDER"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(GTK_COMBO(dg->combo[1])->entry)), cap("ALIGN"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(GTK_COMBO(dg->combo[2])->entry)), cap("VALIGN"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(GTK_COMBO(dg->combo[3])->entry)), cap("BGCOLOR"), thestring, NULL);
	thestring = insert_integer_if_spin(dg->spin[2], cap("WIDTH"), thestring, dg->check[1]);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(GTK_COMBO(dg->combo[4])->entry)), cap("CLASS"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[2])), cap("STYLE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(GTK_COMBO(dg->combo[5])->entry)), cap("frame"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(GTK_COMBO(dg->combo[6])->entry)), cap("rules"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[1])), NULL, thestring, NULL);
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


void tabledialog_cb(GtkWidget * widget, gpointer data)
{
	GList *alignlist = NULL, *popuplist;
	GtkWidget *var_but, *dgtable;

	static gchar *tagitems[] = { "cellpadding", "cellspacing", "border"
			, "align", "valign", "bgcolor", "width"
			, "class", "style", "rules", "frame", NULL };
	gchar *tagvalues[12];
	gchar *custom = NULL;
	Thtml_diag *dg;

	dg = html_diag_new(_("Table"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, widget, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 8);

	dg->spin[1] = spinbut_with_value(tagvalues[0], 0, 100, 1.0, 5.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Cell Padding")), 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1, 2, 0, 1);

	dg->spin[3] = spinbut_with_value(tagvalues[1], 0, 100, 1.0, 5.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Cell Spacing")), 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[3], 1, 2, 1, 2);

	dg->combo[4] = combo_with_popdown_sized(tagvalues[7], recent_attribs.classlist, 1, 80);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Class")), 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[4], 1, 2, 2, 3);

	dg->entry[2] = entry_with_text(tagvalues[8], 1024);
	var_but = style_but_new(dg->entry[2], dg->dialog);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Style")), 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 4, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), var_but, 4, 5, 3, 4);

	dg->entry[1] = entry_with_text(custom, 1024);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Custom")), 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 5, 4, 5);

	alignlist = g_list_append(alignlist, "left");
	alignlist = g_list_append(alignlist, "right");
	alignlist = g_list_append(alignlist, "center");
	dg->combo[1] = combo_with_popdown_sized(tagvalues[3], alignlist, 0, 80);
	g_list_free(alignlist);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Align")), 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[1], 3, 5, 0, 1);

  alignlist = NULL;
	alignlist = g_list_insert(alignlist, "top", 0);
	alignlist = g_list_insert(alignlist, "middle", 1);
	alignlist = g_list_insert(alignlist, "bottom", 2);
	alignlist = g_list_insert(alignlist, "texttop", 3);
	alignlist = g_list_insert(alignlist, "baseline", 4);
	alignlist = g_list_insert(alignlist, "absmiddle", 5);
	alignlist = g_list_insert(alignlist, "absbottom", 6);
	dg->combo[2] = combo_with_popdown_sized(tagvalues[4], alignlist, 1, 80);
	g_list_free(alignlist);

	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Vertical Align")), 2, 3, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[2], 3, 5, 1, 2);

	dg->combo[3] = combo_with_popdown_sized(tagvalues[5], recent_attribs.colorlist, 1, 80);
	var_but = color_but_new(GTK_COMBO(dg->combo[3])->entry, dg->dialog);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Background Color")), 2, 3, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[3], 3, 4, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), var_but, 4, 5, 2, 3);

	dg->spin[2] = spinbut_with_value(NULL, 0, 900, 1.0, 5.0);
	dg->check[1] = gtk_check_button_new_with_label("%");
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_(" Width ")), 5, 6, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[2], 6, 7, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 7, 8, 0, 1);
	parse_integer_for_dialog(tagvalues[6], dg->spin[2], NULL, dg->check[1]);

	dg->spin[4] = spinbut_with_value(tagvalues[2], 0, 100, 1.0, 5.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Border")), 5, 6, 1, 2);
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
	dg->combo[5] = combo_with_popdown_sized(tagvalues[10], popuplist, 0, 80);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Frame")), 5, 6, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[5], 6, 8, 2, 3);
	g_list_free(popuplist);

	popuplist = g_list_append(NULL, "");
	popuplist = g_list_append(popuplist, "none");
	popuplist = g_list_append(popuplist, "groups");
	popuplist = g_list_append(popuplist, "rows");
	popuplist = g_list_append(popuplist, "cols");
	popuplist = g_list_append(popuplist, "all");
	dg->combo[6] = combo_with_popdown_sized(tagvalues[9], popuplist, 0, 80);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Rules")), 5, 6, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[6], 6, 8, 3, 4);
	g_list_free(popuplist);

	html_diag_finish(dg, G_CALLBACK(tabledialogok_lcb));
	
	if (custom)	g_free(custom);
}

static void tablerowdialogok_lcb(GtkWidget * widget, Thtml_diag *dg)
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<TR"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(GTK_COMBO(dg->combo[1])->entry)), cap("ALIGN"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(GTK_COMBO(dg->combo[2])->entry)), cap("VALIGN"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(GTK_COMBO(dg->combo[3])->entry)), cap("BGCOLOR"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(GTK_COMBO(dg->combo[4])->entry)), cap("CLASS"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[2])), cap("STYLE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[1])), NULL, thestring, NULL);
	
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


void tablerowdialog_cb(GtkWidget * widget, gpointer data)
{
	GList *alignlist = NULL;
	GtkWidget *color_but, *var_but;

	static gchar *tagitems[] = { "align", "valign", "bgcolor", "class", "style", NULL };
	gchar *tagvalues[6];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable;

	dg = html_diag_new(_("Table Row"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, widget, dg);

	dgtable = html_diag_table_in_vbox(dg, 4, 5);

	alignlist = g_list_insert(alignlist, "left", 0);
	alignlist = g_list_insert(alignlist, "right", 1);
	alignlist = g_list_insert(alignlist, "center", 2);
	dg->combo[1] = combo_with_popdown_sized(tagvalues[0], alignlist, 0, 90);
	g_list_free(alignlist);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Align")), 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(dg->combo[1]), 1, 2, 0, 1);

  alignlist = NULL;
	alignlist = g_list_insert(alignlist, "top", 0);
	alignlist = g_list_insert(alignlist, "middle", 1);
	alignlist = g_list_insert(alignlist, "bottom", 2);
	alignlist = g_list_insert(alignlist, "texttop", 3);
	alignlist = g_list_insert(alignlist, "baseline", 4);
	alignlist = g_list_insert(alignlist, "absmiddle", 5);
	alignlist = g_list_insert(alignlist, "absbottom", 6);
	dg->combo[2] = combo_with_popdown_sized(tagvalues[1], alignlist, 0, 90);
	g_list_free(alignlist);

	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("VAlign")), 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[2])), 1, 2, 1, 2);

	dg->combo[4] = combo_with_popdown_sized(tagvalues[3], recent_attribs.classlist, 1, 90);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Class")), 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[4], 3, 5, 0, 1);

	dg->combo[3] = combo_with_popdown_sized(tagvalues[2], recent_attribs.colorlist, 1, 90);
	color_but = color_but_new(GTK_COMBO(dg->combo[3])->entry, dg->dialog);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Background Color")), 2, 3, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[3])), 3, 4, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(color_but), 4, 5, 1, 2);

	dg->entry[2] = entry_with_text(tagvalues[4], 1024);
	var_but = style_but_new(dg->entry[2], dg->dialog);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Style")), 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 4, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), var_but, 4, 5, 2, 3);

	dg->entry[1] = entry_with_text(custom, 1024);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Custom")), 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 5, 3, 4);

	html_diag_finish(dg, G_CALLBACK(tablerowdialogok_lcb));

	if (custom)	g_free(custom);
}

static void table_head_and_data_dialogok_lcb(gint type, GtkWidget * widget, Thtml_diag *dg) {

	gchar *thestring, *finalstring;
	
	if (type == 1) {
		thestring = g_strdup(cap("<TD"));
	} else {
		thestring = g_strdup(cap("<TH"));
	}
	thestring = insert_integer_if_spin(dg->spin[1], cap("WIDTH"), thestring, dg->check[2]);
	thestring = insert_integer_if_spin(dg->spin[3], cap("HEIGHT"), thestring, dg->check[3]);
	thestring = insert_string_if_entry(dg->spin[5], cap("COLSPAN"), thestring, NULL);
	thestring = insert_string_if_entry(dg->spin[4], cap("ROWSPAN"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry), cap("ALIGN"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry), cap("VALIGN"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[3])->entry), cap("BGCOLOR"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[4])->entry), cap("CLASS"), thestring, NULL);
	thestring = insert_attr_if_checkbox(dg->check[1], cap("NOWRAP"), thestring);
	thestring = insert_string_if_entry(dg->entry[2], cap("STYLE"), thestring, NULL);
	thestring = insert_string_if_entry(dg->entry[1], NULL, thestring, NULL);
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

static void tabledatadialogok_lcb(GtkWidget * widget, Thtml_diag *dg) {
	table_head_and_data_dialogok_lcb(1, widget, dg);
}

static void tableheaddialogok_lcb(GtkWidget * widget, Thtml_diag *dg) {
	table_head_and_data_dialogok_lcb(0, widget, dg);
}

static void table_head_and_data_dialog_cb(gint type, GtkWidget * widget, gpointer data) {
	GList *alignlist = NULL;

	static gchar *tagitems[] = { "width", "align", "colspan"
				, "height", "valign", "rowspan"
				, "bgcolor", "nowrap", "class"
				, "style", NULL };
	gchar *tagvalues[11];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable, *var_but;

	if (type == 1) {
		dg = html_diag_new(_("Table Data"));
	} else {
		dg = html_diag_new(_("Table Header"));
	}
	
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, widget, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 7);

	alignlist = g_list_insert(alignlist, "left", 0);
	alignlist = g_list_insert(alignlist, "right", 1);
	alignlist = g_list_insert(alignlist, "center", 2);
	alignlist = g_list_insert(alignlist, "justify", 3);
	dg->combo[1] = combo_with_popdown_sized(tagvalues[1], alignlist, 1, 80);
	g_list_free(alignlist);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Align")), 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[1])), 1, 2, 0, 1);

  alignlist = NULL;
	alignlist = g_list_insert(alignlist, "top", 0);
	alignlist = g_list_insert(alignlist, "middle", 1);
	alignlist = g_list_insert(alignlist, "bottom", 2);
	alignlist = g_list_insert(alignlist, "texttop", 3);
	alignlist = g_list_insert(alignlist, "baseline", 4);
	alignlist = g_list_insert(alignlist, "absmiddle", 5);
	alignlist = g_list_insert(alignlist, "absbottom", 6);
	dg->combo[2] = combo_with_popdown_sized(tagvalues[4], alignlist, 1, 90);
	g_list_free(alignlist);

	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("VAlign")), 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[2])), 1, 2, 1, 2);

	dg->combo[4] = combo_with_popdown_sized(tagvalues[8], recent_attribs.classlist, 1, 80);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Class")), 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[4])), 1, 2, 2, 3);

	dg->entry[2] = entry_with_text(tagvalues[9], 512);
	var_but = style_but_new(dg->entry[2], dg->dialog);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Style")), 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 6, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), var_but, 6, 7, 3, 4);

	dg->entry[1] = entry_with_text(custom, 1024);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Custom")), 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 7, 4, 5);

	dg->spin[5] = spinbut_with_value(tagvalues[2], 0, 10, 1.0, 2.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Col Span")), 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[5], 3, 4, 0, 1);

	dg->spin[4] = spinbut_with_value(tagvalues[5], 0, 100, 1.0, 5.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Row Span")), 2, 3, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[4], 3, 4, 1, 2);

	dg->check[1] = gtk_check_button_new();
	parse_existence_for_dialog(tagvalues[7], dg->check[1]);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("No Wrap")), 2, 3, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 3, 4, 2, 3);
	
	dg->spin[1] = spinbut_with_value(NULL, 0, 100, 1.0, 5.0);
	dg->check[2] = gtk_check_button_new_with_label("%");
	parse_integer_for_dialog(tagvalues[0], dg->spin[1], NULL, dg->check[2]);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Width")), 4, 5, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 5, 6, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[2], 6, 7, 0, 1);

	dg->spin[3] = spinbut_with_value(NULL, 0, 100, 1.0, 5.0);
	dg->check[3] = gtk_check_button_new_with_label("%");
	parse_integer_for_dialog(tagvalues[3], dg->spin[3], NULL, dg->check[3]);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Height")), 4, 5, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[3], 5, 6, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[3], 6, 7, 1, 2);

	dg->combo[3] = combo_with_popdown_sized(tagvalues[6], recent_attribs.colorlist, 1, 80);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Background Color")), 4, 5, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[3])), 5, 6, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(color_but_new(GTK_COMBO(dg->combo[3])->entry, dg->dialog)), 6, 7, 2, 3);

	if (type == 1) {
		html_diag_finish(dg, G_CALLBACK(tabledatadialogok_lcb));
	} else {
		html_diag_finish(dg, G_CALLBACK(tableheaddialogok_lcb));
	}

	if (custom)	g_free(custom);
}

void tabledatadialog_cb(GtkWidget * widget, gpointer data) {
	table_head_and_data_dialog_cb(1, widget, data);
}

void tableheaddialog_cb(GtkWidget * widget, gpointer data) {
	table_head_and_data_dialog_cb(0, widget, data);
}
