/* Bluefish HTML Editor
 * html_form.c - html form dialogs
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

#include "cap.h"
#include "bluefish.h"
#include "html_diag.h" /* html_diag functions */
#include "html_form.h" /* myself */
#include "stringlist.h" /* add to stringlist */
#include "pixmap.h" /* new_pixmap() */
#include "gtk_easy.h"  /* entry_with_text and stuff */
#include "document.h" /* insert_dbl_text */

/*************************************************/
/**********   PHP BUTTON FUNCTIONS   *************/
/*************************************************/

#define PHPFORM_TYPE_TEXT 0
#define PHPFORM_TYPE_RADIO 1
#define PHPFORM_TYPE_CHECK 2

static void php_var_insert_cb(GtkWidget *widget, Thtml_diag *dg) {
	gchar *tmp;
	tmp = gtk_editable_get_chars(GTK_EDITABLE(dg->php_var_ins.src), 0, -1);
	if (strlen(tmp)) {
		gchar *tmp2=NULL;
		switch (dg->php_var_ins.type) {
		case PHPFORM_TYPE_TEXT:
			tmp2 = g_strdup_printf("<?php if (isset($%s)) { echo $%s; } ?>",tmp,tmp);
		break;
		case PHPFORM_TYPE_RADIO:
			tmp2 = g_strdup_printf("<?php if (isset($%s)) { echo 'selected'; } ?>",tmp);
		break;
		case PHPFORM_TYPE_CHECK:
			tmp2 = g_strdup_printf("<?php if (isset($%s)) { echo 'selected'; } ?>",tmp);
		break;
		}
		if (tmp2) {
			gtk_entry_set_text(GTK_ENTRY(dg->php_var_ins.dest), tmp2);
			g_free(tmp2);
		}
	}
	g_free(tmp);
}

GtkWidget *php_var_but(GtkWidget *src, GtkWidget *dest, Thtml_diag *dg, gint type) {
	GtkWidget *pixmap,*returnwid;

	dg->php_var_ins.src = src;
	dg->php_var_ins.dest = dest;
	dg->php_var_ins.type = type;
	returnwid = gtk_button_new();
	pixmap = new_pixmap(84);
	gtk_widget_show(pixmap);
	gtk_container_add(GTK_CONTAINER(returnwid), pixmap);
	gtk_signal_connect(GTK_OBJECT(returnwid), "clicked", G_CALLBACK(php_var_insert_cb), dg);
	gtk_widget_show(returnwid);
	return returnwid;
}


/************************************************************************/
/************************************************************************/
/****************                                  **********************/
/****************  BELOW ARE ALL DIALOGS MIGRATED  **********************/
/****************      TO HTML_DIAG FUNCTIONS      **********************/
/****************                                  **********************/
/************************************************************************/
/************************************************************************/

static void formok_lcb(GtkWidget * widget, Thtml_diag *dg) {
	Tbfwin *bfwin = dg->bfwin;
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<FORM"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[4])->entry), cap("ACTION"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry), cap("METHOD"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry), cap("ENCTYPE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[3])->entry), cap("TARGET"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[2]), NULL, thestring, NULL);
	finalstring = g_strconcat(thestring, ">", NULL);
	g_free(thestring);

	bfwin->session->targetlist = add_entry_to_stringlist(bfwin->session->targetlist, GTK_WIDGET(GTK_COMBO(dg->combo[3])->entry));
	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</FORM>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}

	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}


void formdialog_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	GList *methodlist = NULL;
	GList *enctypelist = NULL;

	static gchar *tagitems[] = { "action", "method", "enctype", "target", NULL };
	gchar *tagvalues[5];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable;

	dg = html_diag_new(bfwin,_("Form"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 4, 10);

	bfwin->session->urllist = add_to_stringlist(bfwin->session->urllist, "<?php echo $SCRIPT_NAME ?>");
	bfwin->session->urllist = add_to_stringlist(bfwin->session->urllist, "'.$SCRIPT_NAME.'");
	{
	GList *rel_link_list, *tmplist;
	rel_link_list = NULL /*generate_relative_doc_list()*/;
	tmplist = duplicate_stringlist(bfwin->session->urllist, 1);
	rel_link_list = g_list_concat(tmplist, rel_link_list);

	dg->combo[4] = combo_with_popdown(tagvalues[0], rel_link_list, 1);

	free_stringlist(rel_link_list);
	}
	
	bf_mnemonic_label_tad_with_alignment(_("_Action:"), dg->combo[4], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[4], 1, 8, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), file_but_new(GTK_WIDGET(GTK_COMBO(dg->combo[4])->entry), 0, bfwin), 8, 10, 0, 1);
	
	methodlist = g_list_append(methodlist, "GET");
	methodlist = g_list_append(methodlist, "POST");
	dg->combo[1] = combo_with_popdown(tagvalues[1], methodlist, 1);
	g_list_free(methodlist);
	bf_mnemonic_label_tad_with_alignment(_("Metho_d:"), dg->combo[1], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[1])), 1, 2, 1, 2);

	/* I need to add more types */
	enctypelist = g_list_append(enctypelist, "application/x-www-form-urlencoded");
	enctypelist = g_list_append(enctypelist, "multipart/form-data");
	enctypelist = g_list_append(enctypelist, "text/plain");
	dg->combo[2] = combo_with_popdown(tagvalues[2], enctypelist, 1);
	g_list_free(enctypelist);
	bf_mnemonic_label_tad_with_alignment(_("_Enctype:"), dg->combo[2], 0, 0.5, dgtable, 3, 4, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(dg->combo[2]), 4, 10, 1, 2);

	bfwin->session->targetlist = add_to_stringlist(bfwin->session->targetlist, "_top");
	bfwin->session->targetlist = add_to_stringlist(bfwin->session->targetlist, "_blank");
	bfwin->session->targetlist = add_to_stringlist(bfwin->session->targetlist, "_parent");
	dg->combo[3] = combo_with_popdown(tagvalues[3], bfwin->session->targetlist, 1);
	bf_mnemonic_label_tad_with_alignment(_("_Target:"), dg->combo[3], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[3])), 1, 10, 2, 3);

	dg->entry[2] = entry_with_text(custom, 1024);
	bf_mnemonic_label_tad_with_alignment(_("Custo_m:"), dg->entry[2], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 12, 3, 4);

	html_diag_finish(dg, G_CALLBACK(formok_lcb));

	if (custom)	g_free(custom);

}

static void textareaok_lcb(GtkWidget * widget, Thtml_diag *dg)
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<TEXTAREA"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[1])), cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->spin[1])), cap("ROWS"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->spin[2])), cap("COLS"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[2])), NULL, thestring, NULL);
	finalstring = g_strconcat(thestring, ">", NULL);
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</TEXTAREA>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}

	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void textareadialog_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	static gchar *tagitems[] = { "name", "value", "rows", "cols", NULL };
	gchar *tagvalues[5];
	gchar *custom = NULL;
	GtkWidget *dgtable;
	Thtml_diag *dg;

	dg = html_diag_new(bfwin,_("Text area"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 4, 10);

	dg->entry[1] = entry_with_text(tagvalues[0], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Name:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 10, 0, 1);

	dg->spin[1] = spinbut_with_value(tagvalues[2], 0, 500, 1.0, 5.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1, 2, 1, 2);
	bf_mnemonic_label_tad_with_alignment(_("_Rows:"), dg->spin[1], 0, 0.5, dgtable, 0, 1, 1, 2);

	dg->spin[2] = spinbut_with_value(tagvalues[3], 0, 500, 1.0, 5.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[2], 1, 2, 2, 3);
	bf_mnemonic_label_tad_with_alignment(_("Co_ls:"), dg->spin[2], 0, 0.5, dgtable, 0, 1, 2, 3);

	dg->entry[2] = entry_with_text(custom, 1024);
	bf_mnemonic_label_tad_with_alignment(_("Custo_m:"), dg->entry[2], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 10, 3, 4);

	html_diag_finish(dg, G_CALLBACK(textareaok_lcb));

	if (custom)	g_free(custom);
}

static void textok_lcb(GtkWidget * widget, Thtml_diag *dg)
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<INPUT TYPE=\"TEXT\""));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[1])), cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->spin[1])), cap("SIZE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->spin[2])), cap("MAXLENGTH"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[2])), cap("VALUE"), thestring, NULL);	
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[3])), NULL, thestring, NULL);
	if (main_v->props.xhtml == 1) {
		finalstring = g_strconcat(thestring, " />", NULL);
	} else {
		finalstring = g_strconcat(thestring, ">", NULL);
	}
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, NULL);
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}


void textdialog_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	GtkWidget *varbut;
	static gchar *tagitems[] = { "name", "value", "size", "maxlength", "type", NULL };
	gchar *tagvalues[6];
	gchar *custom = NULL;
	GtkWidget *dgtable;
	Thtml_diag *dg;

	dg = html_diag_new(bfwin,_("Text"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 10);

	dg->entry[1] = entry_with_text(tagvalues[0], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Name:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 10, 0, 1);

	dg->entry[2] = entry_with_text(tagvalues[1], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Value:"), dg->entry[2], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 9, 1, 2);

	varbut = php_var_but(dg->entry[1], dg->entry[2], dg, PHPFORM_TYPE_TEXT);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), varbut, 9, 10, 1, 2);

	dg->spin[1] = spinbut_with_value(tagvalues[2], 0, 500, 1.0, 5.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1, 2, 2, 3);
	bf_mnemonic_label_tad_with_alignment(_("Si_ze:"), dg->spin[1], 0, 0.5, dgtable, 0, 1, 2, 3);

	dg->spin[2] = spinbut_with_value(tagvalues[3], 0, 500, 1.0, 5.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[2], 1, 2, 3, 4);
	bf_mnemonic_label_tad_with_alignment(_("Max _Length:"), dg->spin[2], 0, 0.5, dgtable, 0, 1, 3, 4);

	dg->entry[3] = entry_with_text(custom, 1024);
	bf_mnemonic_label_tad_with_alignment(_("Custo_m:"), dg->entry[3], 0, 0.5, dgtable, 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[3], 1, 10, 4, 5);

	html_diag_finish(dg, G_CALLBACK(textok_lcb));

	if (custom)	g_free(custom);
}

static void buttondialogok_lcb(GtkWidget * widget, Thtml_diag *dg)
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<INPUT"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[1])), cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[2])), cap("VALUE"), thestring, NULL);	
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(GTK_COMBO(dg->combo[1])->entry)), cap("TYPE"), thestring, NULL);	
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[3])), NULL, thestring, NULL);

	if (main_v->props.xhtml == 1) {
		finalstring = g_strconcat(thestring, " />", NULL);
	} else {
		finalstring = g_strconcat(thestring, ">", NULL);
	}
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, NULL);
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void buttondialog_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	static gchar *tagitems[] = { "name", "value", "type",  NULL };
	gchar *tagvalues[4];
	gchar *custom = NULL;
	GtkWidget *dgtable;
	Thtml_diag *dg;
	GList *tmplist=NULL;

	dg = html_diag_new(bfwin,_("Button"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 10);

	dg->entry[1] = entry_with_text(tagvalues[0], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Name:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 10, 0, 1);

	dg->entry[2] = entry_with_text(tagvalues[1], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Value:"), dg->entry[2], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 9, 1, 2);

	tmplist = g_list_append(tmplist, "submit");
	tmplist = g_list_append(tmplist, "reset");
	tmplist = g_list_append(tmplist, "button");
	dg->combo[1] = combo_with_popdown(tagvalues[2], tmplist, 0);
	g_list_free(tmplist);
	bf_mnemonic_label_tad_with_alignment(_("_Type:"), dg->combo[1], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(dg->combo[1]), 1, 9, 2, 3);

	dg->entry[3] = entry_with_text(custom, 256);
	bf_mnemonic_label_tad_with_alignment(_("Custo_m:"), dg->entry[3], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[3], 1, 9, 3, 4);

	html_diag_finish(dg, G_CALLBACK(buttondialogok_lcb));

	if (custom)	g_free(custom);
}

static void hiddenok_lcb(GtkWidget * widget, Thtml_diag *dg)
{
	gchar *thestring, *finalstring;
	thestring = g_strdup(cap("<INPUT TYPE=\"HIDDEN\""));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[1])), cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[2])), cap("VALUE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[3])), NULL, thestring, NULL);
	if (main_v->props.xhtml == 1) {
		finalstring = g_strconcat(thestring," />", NULL);
	} else {
		finalstring = g_strconcat(thestring,">", NULL);
	}
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, NULL);
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}


void hiddendialog_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	static gchar *tagitems[] = { "name", "value", "type", NULL };
	gchar *tagvalues[4];
	gchar *custom = NULL;

	GtkWidget *dgtable;
	Thtml_diag *dg;

	dg = html_diag_new(bfwin,_("Hidden"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 10);
	dg->entry[1] = entry_with_text(tagvalues[0], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Name:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 10, 0, 1);

	dg->entry[2] = entry_with_text(tagvalues[1], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Value:"), dg->entry[2], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 10, 1, 2);

	dg->entry[3] = entry_with_text(custom, 1024);
	bf_mnemonic_label_tad_with_alignment(_("Custo_m:"), dg->entry[3], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[3], 1, 10, 2, 3);

	html_diag_finish(dg, G_CALLBACK(hiddenok_lcb));

	if (custom)	g_free(custom);
}

static void radiodialogok_lcb(GtkWidget * widget, Thtml_diag *dg)
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<INPUT TYPE=\"RADIO\""));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[1])), cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[2])), cap("VALUE"), thestring, NULL);
	thestring = insert_attr_if_checkbox(dg->check[1], cap("CHECKED"), thestring);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[3])), NULL, thestring, NULL);
	if (main_v->props.xhtml == 1) {
		finalstring = g_strconcat(thestring," />", NULL);
	} else {
		finalstring = g_strconcat(thestring,">", NULL);
	}
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, NULL);
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}


void radiodialog_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	static gchar *tagitems[] = { "name", "value", "checked", "type", NULL };
	gchar *tagvalues[5];
	gchar *custom = NULL;

	GtkWidget *dgtable, *varbut;
	Thtml_diag *dg;

	dg = html_diag_new(bfwin,_("Radio Button"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 10);
	dg->entry[1] = entry_with_text(tagvalues[0], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Name:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 10, 0, 1);

	dg->entry[2] = entry_with_text(tagvalues[1], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Value:"), dg->entry[2], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 10, 1, 2);

	dg->check[1] = gtk_check_button_new();
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 1, 2, 2, 3);
	bf_mnemonic_label_tad_with_alignment(_("Chec_ked:"), dg->check[1], 0, 0.5, dgtable, 0, 1, 2, 3);
	parse_existence_for_dialog(tagvalues[2], dg->check[1]);

	dg->entry[3] = entry_with_text(custom, 1024);
	bf_mnemonic_label_tad_with_alignment(_("Custo_m:"), dg->entry[3], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[3], 1, 9, 3, 4);

	varbut = php_var_but(dg->entry[1], dg->entry[3], dg, PHPFORM_TYPE_RADIO);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), varbut, 9, 10, 3, 4);

	html_diag_finish(dg, G_CALLBACK(radiodialogok_lcb));

	if (custom)	g_free(custom);
}

static void checkdialogok_lcb(GtkWidget * widget,Thtml_diag *dg )
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<INPUT TYPE=\"CHECKBOX\""));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[1])), cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[2])), cap("VALUE"), thestring, NULL);
	thestring = insert_attr_if_checkbox(dg->check[1], cap("CHECKED"), thestring);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[3])), NULL, thestring, NULL);
	if (main_v->props.xhtml == 1) {
		finalstring = g_strconcat(thestring," />", NULL);
	} else {
		finalstring = g_strconcat(thestring,">", NULL);
	}
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, NULL);
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void checkdialog_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	static gchar *tagitems[] = { "name", "value", "checked", "type", NULL };
	gchar *tagvalues[5];
	gchar *custom = NULL;
	GtkWidget *dgtable, *varbut;
	Thtml_diag *dg;

	dg = html_diag_new(bfwin,_("Check Box"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 10);
	
	dg->entry[1] = entry_with_text(tagvalues[0], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Name:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 10, 0, 1);

	dg->entry[2] = entry_with_text(tagvalues[1], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Value:"), dg->entry[2], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 10, 1, 2);

	dg->check[1] = gtk_check_button_new();
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 1, 2, 2, 3);
	bf_mnemonic_label_tad_with_alignment(_("Chec_ked:"), dg->check[1], 0, 0.5, dgtable, 0, 1, 2, 3);
	parse_existence_for_dialog(tagvalues[2], dg->check[1]);

	dg->entry[3] = entry_with_text(custom, 1024);
	bf_mnemonic_label_tad_with_alignment(_("_Custo_m:"), dg->entry[3], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[3], 1, 9, 3, 4);

	varbut = php_var_but(dg->entry[1], dg->entry[3], dg, PHPFORM_TYPE_CHECK);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), varbut, 9, 10, 3, 4);

	html_diag_finish(dg, G_CALLBACK(checkdialogok_lcb));

	if (custom)	g_free(custom);
}

static void selectdialogok_lcb(GtkWidget * widget, Thtml_diag *dg)
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<SELECT"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[1])), cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->spin[1])), cap("SIZE"), thestring, NULL);
	thestring = insert_attr_if_checkbox(dg->check[1], cap("MULTIPLE"), thestring);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[2])), NULL, thestring, NULL);
	finalstring = g_strconcat(thestring, ">", NULL);
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</SELECT>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void selectdialog_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	static gchar *tagitems[] = { "name", "size", "multiple", NULL };
	gchar *tagvalues[4];
	gchar *custom = NULL;
	GtkWidget *dgtable;
	Thtml_diag *dg;

	dg = html_diag_new(bfwin,_("Select"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 10);

	dg->entry[1] = entry_with_text(tagvalues[0], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Name:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 10, 0, 1);

	dg->spin[1] = spinbut_with_value(tagvalues[1], 0, 100, 1.0, 5.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1, 2, 1, 2);
	bf_mnemonic_label_tad_with_alignment(_("Si_ze:"), dg->spin[1], 0, 0.5, dgtable, 0, 1, 1, 2);

	dg->check[1] = gtk_check_button_new();
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 1, 2, 2, 3);
	bf_mnemonic_label_tad_with_alignment(_("Multiple _Selections:"), dg->check[1], 0, 0.5, dgtable, 0, 1, 2, 3);
	parse_existence_for_dialog(tagvalues[2], dg->check[1]);

	dg->entry[2] = entry_with_text(custom, 1024);
	bf_mnemonic_label_tad_with_alignment(_("Custo_m:"), dg->entry[2], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 10, 3, 4);

	html_diag_finish(dg, G_CALLBACK(selectdialogok_lcb));

	if (custom)	g_free(custom);
}

static void optiondialogok_lcb(GtkWidget * widget,Thtml_diag *dg )
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<OPTION"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[1])), cap("VALUE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[2])), cap("LABEL"), thestring, NULL);
	thestring = insert_attr_if_checkbox(dg->check[1], cap("SELECTED"), thestring);
	finalstring = g_strconcat(thestring, ">", NULL);
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</OPTION>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void optiondialog_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	static gchar *tagitems[] = { "value", "label", "selected", NULL };
	gchar *tagvalues[4];
	gchar *custom = NULL;
	GtkWidget *dgtable;
	Thtml_diag *dg;

	dg = html_diag_new(bfwin,_("Option"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 3, 10);

	dg->entry[1] = entry_with_text(tagvalues[0], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Value:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 10, 0, 1);

	dg->entry[2] = entry_with_text(tagvalues[2], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Label:"), dg->entry[2], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 10, 1, 2);

	dg->check[1] = gtk_check_button_new();
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 1, 2, 2, 3);
	bf_mnemonic_label_tad_with_alignment(_("_Selected:"), dg->check[1], 0, 0.5, dgtable, 0, 1, 2, 3);
	parse_existence_for_dialog(tagvalues[2], dg->check[1]);

	html_diag_finish(dg, G_CALLBACK(optiondialogok_lcb));

	if (custom)	g_free(custom);
}

static void optgroupdialogok_lcb(GtkWidget * widget,Thtml_diag *dg )
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<OPTGROUP"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[1])), cap("LABEL"), thestring, NULL);
	finalstring = g_strconcat(thestring, ">", NULL);
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</OPTGROUP>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void optgroupdialog_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	static gchar *tagitems[] = { "label", NULL };
	gchar *tagvalues[2];
	gchar *custom = NULL;
	GtkWidget *dgtable;
	Thtml_diag *dg;

	dg = html_diag_new(bfwin,_("Option group"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 10);
	dg->entry[1] = entry_with_text(tagvalues[0], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Label:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 10, 0, 1);

	html_diag_finish(dg, G_CALLBACK(optgroupdialogok_lcb));

	if (custom)	g_free(custom);
}
