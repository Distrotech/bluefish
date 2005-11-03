/* Bluefish HTML Editor
 * html_form.c - html form dialogs
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * Copyright (C) 1999-2005 Olivier Sessink
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* #define DEBUG */
#include <gtk/gtk.h>

#include <string.h>

#include "cap.h"
#include "bluefish.h"
#include "html2.h" /* style_but_new() */
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
	tmp = gtk_editable_get_chars(GTK_EDITABLE(dg->php_var_ins.name), 0, -1);

	if (strlen(tmp)) {
		gchar *tmp2=NULL;
		gchar *tmp3=NULL;
		switch (dg->php_var_ins.type) {
		case PHPFORM_TYPE_TEXT:
			tmp2 = g_strdup_printf("<?php if (isset($_POST['%s'])) { echo $%s; } ?>",tmp,tmp);
		break;
		case PHPFORM_TYPE_RADIO:
			tmp3 = gtk_editable_get_chars(GTK_EDITABLE(dg->php_var_ins.val), 0, -1);
			if (strlen(tmp3)) {
				if (main_v->props.xhtml == 1) {
					tmp2 = g_strdup_printf("<?php if ($_POST['%s']==\"%s\") { echo 'checked=\\\"checked\\\"'; } ?>",tmp,tmp3);
				} else {
					tmp2 = g_strdup_printf("<?php if ($_POST['%s']==\"%s\") { echo 'checked'; } ?>",tmp,tmp3);
				}
				g_free(tmp3);
			}
		break;
		case PHPFORM_TYPE_CHECK:
			if (main_v->props.xhtml == 1) {
				tmp2 = g_strdup_printf("<?php if (isset($_POST['%s'])) { echo 'checked=\\\"checked\\\"'; } ?>",tmp);
			} else {
				tmp2 = g_strdup_printf("<?php if (isset($_POST['%s'])) { echo 'checked'; } ?>",tmp);
			}
		break;
		}
		if (tmp2) {
			gtk_entry_set_text(GTK_ENTRY(dg->php_var_ins.dest), tmp2);
			g_free(tmp2);
		}
	}
	g_free(tmp);
}

GtkWidget *php_var_but(Thtml_diag *dg, GtkWidget *name, GtkWidget *val) {
 	GtkWidget *pixmap,*returnwid;
	dg->php_var_ins.name = name;
	dg->php_var_ins.val = val;
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
	
	methodlist = g_list_append(methodlist, cap("GET"));
	methodlist = g_list_append(methodlist, cap("POST"));
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
/*
static void textok_lcb(GtkWidget * widget, Thtml_diag *dg)
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<INPUT TYPE=\"TEXT\""));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[1])), cap("NAME"), thestring, NULL);
	thestring = insert_integer_if_spin(dg->spin[1], cap("SIZE"), thestring, FALSE, 0);
	thestring = insert_integer_if_spin(dg->spin[2], cap("MAXLENGTH"), thestring, FALSE, 0);
/ *	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->spin[1])), cap("SIZE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->spin[2])), cap("MAXLENGTH"), thestring, NULL);* /
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

	varbut = php_var_but(dg->entry[1],NULL, dg->entry[2], dg, PHPFORM_TYPE_TEXT);
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
	dg->combo[1] = combo_with_popdown((tagvalues[2] == NULL || tagvalues[2][0] == '\0') ? "submit" : tagvalues[2], tmplist, 0);
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
	static gchar *tagitems[] = { "name", "value", NULL };
	gchar *tagvalues[3];
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
	thestring = insert_attr_if_checkbox(dg->check[1], main_v->props.xhtml == 1 ? cap("CHECKED=\"checked\"") : cap("CHECKED"), thestring);
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
	static gchar *tagitems[] = { "name", "value", "checked", NULL };
	gchar *tagvalues[4];
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

	varbut = php_var_but(dg->entry[1], dg->entry[2], dg->entry[3], dg, PHPFORM_TYPE_RADIO);
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
	thestring = insert_attr_if_checkbox(dg->check[1], main_v->props.xhtml == 1 ? cap("CHECKED=\"checked\"") : cap("CHECKED"), thestring);
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
	static gchar *tagitems[] = { "name", "value", "checked", NULL };
	gchar *tagvalues[4];
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

	varbut = php_var_but(dg->entry[1],NULL, dg->entry[3], dg, PHPFORM_TYPE_CHECK);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), varbut, 9, 10, 3, 4);

	html_diag_finish(dg, G_CALLBACK(checkdialogok_lcb));

	if (custom)	g_free(custom);
}
*/
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

	dg->spin[1] = spinbut_with_value(tagvalues[1], 0, 1000, 1.0, 5.0);
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

static void inputdialogok_lcb(GtkWidget * widget,Thtml_diag *dg) {
	gchar *thestring, *finalstring;
	const char *text;
	text = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(dg->combo[0])->entry));
	thestring = g_strdup(cap("<INPUT"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[0])->entry), cap("TYPE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[0]), cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[1]), cap("VALUE"), thestring, NULL);
	if (strcmp(text, "radio")==0 || strcmp(text, "checkbox")==0) {
		thestring = insert_attr_if_checkbox(dg->check[0], main_v->props.xhtml == 1 ? cap("CHECKED=\"checked\"") : cap("CHECKED"), thestring);
	}
	if (strcmp(text, "hidden")!=0) {
		thestring = insert_integer_if_spin(dg->spin[0], cap("SIZE"), thestring, FALSE, 0);
	}
	if (strcmp(text, "text")==0 || strcmp(text, "passwd")==0) {
		thestring = insert_integer_if_spin(dg->spin[1], cap("MAXLENGTH"), thestring, FALSE, 0);
	}
	if (strcmp(text, "file")==0) {
		thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[2]), cap("ACCEPT"), thestring, NULL);
	}
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->attrwidget[0])->entry), cap("CLASS"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->attrwidget[1]), cap("ID"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->attrwidget[2]), cap("STYLE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[3]), cap("ONFOCUS"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[4]), cap("ONBLUR"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[5]), cap("ONSELECT"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[6]), cap("ONCHANGE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[7]), NULL, thestring, NULL);
	finalstring = g_strconcat(thestring, (main_v->props.xhtml == 1) ? " />" : ">", NULL);
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, NULL);
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

static void inputdialog_typecombo_activate_lcb(GtkList *list, Thtml_diag *dg) {
	/* hmm this function should check if the window is being destroyed... */
	if (!dg->tobedestroyed) {
		const gchar *text = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(dg->combo[0])->entry));
		DEBUG_MSG("inputdialog_typecombo_activate_lcb, text=%s\n",text);
		gtk_widget_set_sensitive(dg->check[0], (strcmp(text, "radio")==0 || strcmp(text, "checkbox")==0));
		gtk_widget_set_sensitive(dg->spin[0], (strcmp(text, "hidden")!=0));
		gtk_widget_set_sensitive(dg->spin[1], (strcmp(text, "text")==0 || strcmp(text, "passwd")==0));
		gtk_widget_set_sensitive(dg->entry[2], (strcmp(text, "file")==0));
		gtk_widget_set_sensitive(dg->phpbutton, (strcmp(text, "radio")==0 || strcmp(text, "checkbox")==0) || (strcmp(text, "text")==0));
		if (strcmp(text, "text")==0) {
			dg->php_var_ins.dest = dg->entry[1];
			dg->php_var_ins.type = PHPFORM_TYPE_TEXT;
		} else if (strcmp(text, "radio")==0) {
			dg->php_var_ins.dest = dg->entry[7];
			dg->php_var_ins.type = PHPFORM_TYPE_RADIO;
		} else if (strcmp(text, "checkbox")==0) {
			dg->php_var_ins.dest = dg->entry[7];
			dg->php_var_ins.type = PHPFORM_TYPE_CHECK;
		}
	}
}

void inputdialog_dialog(Tbfwin *bfwin, Ttagpopup *data, const gchar *type) {
	static gchar *tagitems[] = { "type", "name", "value", "checked", "size", "maxlength", "accept", "onfocus", "onblur", "onselect", "onchange", "class", "id", "style", NULL };
	gchar *tagvalues[15];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *noteb, *dgtable;
	
	dg = html_diag_new(bfwin,_("Input"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);
	noteb = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(dg->vbox), noteb, FALSE, FALSE, 0);

	dgtable = generic_table_inside_notebookframe(noteb, _("Attributes"), 10, 3);

	{
		GList *poplist;
		const gchar *type2 = (type) ? type : "text";
		poplist = list_from_arglist(FALSE, "text", "password", "checkbox", "radio", "submit", "reset", "file", "hidden", "image", "button", NULL);
		dg->combo[0] = combo_with_popdown(tagvalues[0] ? tagvalues[0] : type2, poplist, 0);
		g_list_free(poplist);
	}
	bf_mnemonic_label_tad_with_alignment(_("_Type:"), dg->combo[0], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[0])), 1, 3, 0, 1);

	g_signal_connect(G_OBJECT(GTK_COMBO(dg->combo[0])->list), "selection-changed", G_CALLBACK(inputdialog_typecombo_activate_lcb), dg);

	dg->entry[0] = entry_with_text(tagvalues[1], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Name:"), dg->entry[0], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[0], 1, 3, 1, 2);
	
	dg->entry[1] = entry_with_text(tagvalues[2], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Value:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 3, 2, 3);
	
	dg->check[0] = gtk_check_button_new();
	bf_mnemonic_label_tad_with_alignment(_("_Checked:"), dg->check[0], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[0], 1, 3, 3, 4);
	parse_existence_for_dialog(tagvalues[3], dg->check[0]);
	
	dg->spin[0] = spinbut_with_value(tagvalues[4], 0, 1000, 1.0, 5.0);
	bf_mnemonic_label_tad_with_alignment(_("Si_ze:"), dg->spin[0], 0, 0.5, dgtable, 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[0], 1, 2, 4, 5);

	dg->spin[1] = spinbut_with_value(tagvalues[5], 0, 10000, 1.0, 5.0);
	bf_mnemonic_label_tad_with_alignment(_("Max _Length:"), dg->spin[1], 0, 0.5, dgtable, 0, 1, 5, 6);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1, 2, 5, 6);

	dg->entry[2] = entry_with_text(tagvalues[6], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Accept:"), dg->entry[2], 0, 0.5, dgtable, 0, 1, 6, 7);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 3, 6, 7);
	
	dg->entry[7] = entry_with_text(custom, 256);
	bf_mnemonic_label_tad_with_alignment(_("C_ustom:"), dg->entry[7], 0, 0.5, dgtable, 0, 1, 7, 8);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[7], 1, 2, 7, 8);
	dg->phpbutton = php_var_but(dg, dg->entry[0], dg->entry[1]);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->phpbutton, 2, 3, 7, 8);

	dgtable = generic_table_inside_notebookframe(noteb, _("Style"), 4, 3);
	
	generic_class_id_style_section(dg, 0, dgtable, 0, tagvalues, 11);

	dgtable = generic_table_inside_notebookframe(noteb, _("Events"), 10, 5);
	
	dg->entry[3] = entry_with_text(tagvalues[7], 256);
	bf_mnemonic_label_tad_with_alignment(_("On_Focus:"), dg->entry[3], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[3], 1, 3, 0, 1);
	
	dg->entry[4] = entry_with_text(tagvalues[8], 256);
	bf_mnemonic_label_tad_with_alignment(_("On_Blur:"), dg->entry[4], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[4], 1, 3, 1, 2);

	dg->entry[5] = entry_with_text(tagvalues[9], 256);
	bf_mnemonic_label_tad_with_alignment(_("On_Select:"), dg->entry[5], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[5], 1, 3, 2, 3);

	dg->entry[6] = entry_with_text(tagvalues[10], 256);
	bf_mnemonic_label_tad_with_alignment(_("On_Change:"), dg->entry[6], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[6], 1, 3, 3, 4);
	
	inputdialog_typecombo_activate_lcb(NULL, dg);
	html_diag_finish(dg, G_CALLBACK(inputdialogok_lcb));
	if (custom)	g_free(custom);
}

void inputdialog_rpopup(Tbfwin *bfwin, Ttagpopup *data) {
	inputdialog_dialog(bfwin, data, NULL);
}

static void buttondialogok_lcb(GtkWidget * widget, Thtml_diag *dg) {
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<BUTTON"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[1])), cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[2])), cap("VALUE"), thestring, NULL);	
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(GTK_COMBO(dg->combo[1])->entry)), cap("TYPE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[3])), NULL, thestring, NULL);
	finalstring = g_strconcat(thestring, ">", NULL);
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</BUTTON>"));
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
	tmplist = list_from_arglist(FALSE, "", "submit", "reset", "button", NULL);
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
