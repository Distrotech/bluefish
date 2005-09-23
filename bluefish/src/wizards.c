/* Bluefish HTML Editor
 * wizards.c - much magic is contained within
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * Copyright (C) 1999-2002 Olivier Sessink
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

#include <gtk/gtk.h>

#include "bluefish.h"
#include "cap.h"
#include "wizards.h" 	/* myself */
#include "gtk_easy.h" /* spinbutwithvalue and stuff*/
#include "bf_lib.h"  /* bf_str_repeat() */
#include "html_diag.h"
#include "document.h"			/* doc_insert_two_strings() */

static void table_wizard_ok_lcb(GtkWidget * widget, Thtml_diag *dg) {

	gint rows, cols;
	gchar *tablerowstart, *rowdata, *tablerow, *tablecontent, *finalstring;
	/* gtk forces the value of one if nothing is entered */
	rows = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dg->spin[1]));
	cols = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dg->spin[2]));

	if (GTK_TOGGLE_BUTTON(dg->check[1])->active) {
		if (GTK_TOGGLE_BUTTON(dg->check[2])->active) {
			tablerowstart = cap("\t<TR>");
		} else {
			tablerowstart = cap("<TR>");
		}
		rowdata = bf_str_repeat(cap("<TD></TD>"), cols);
	} else {
		tablerowstart = cap("<TR>\n");
		if (GTK_TOGGLE_BUTTON(dg->check[2])->active) {
			rowdata = bf_str_repeat(cap("\t<TD></TD>\n"), cols);
		} else {
			rowdata = bf_str_repeat(cap("<TD></TD>\n"), cols);
		}
	}
	tablerow = g_strconcat(tablerowstart, rowdata, cap("</TR>\n"), NULL);
	g_free(rowdata);
	tablecontent = bf_str_repeat(tablerow, rows);
	g_free(tablerow);
	finalstring = g_strconcat(cap("<TABLE>\n"), tablecontent, cap("</TABLE>"), NULL);
	g_free(tablecontent);

	doc_insert_two_strings(dg->bfwin->current_document, finalstring, NULL);
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void tablewizard_dialog(Tbfwin *bfwin) {
	GtkWidget *dgtable;
	Thtml_diag *dg;
	dg = html_diag_new(bfwin,_("Table Wizard"));

	dgtable = gtk_table_new(4, 5, 0);
	gtk_table_set_row_spacings(GTK_TABLE(dgtable), 6);
	gtk_table_set_col_spacings(GTK_TABLE(dgtable), 12);
	gtk_box_pack_start(GTK_BOX(dg->vbox), dgtable, FALSE, FALSE, 0);

	dg->spin[1] = spinbut_with_value("1", 1, 100, 1.0, 5.0);
	bf_mnemonic_label_tad_with_alignment(_("Number of _rows:"), dg->spin[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1, 5, 0, 1);

	dg->spin[2] = spinbut_with_value("1", 1, 100, 1.0, 5.0);
	bf_mnemonic_label_tad_with_alignment(_("Number of colu_mns:"), dg->spin[2], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[2], 1, 5, 1, 2);

	dg->check[1] = gtk_check_button_new();
	bf_mnemonic_label_tad_with_alignment(_("Table rows on one _line:"), dg->check[1], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 1, 2, 2, 3);

	dg->check[2] = gtk_check_button_new();
	bf_mnemonic_label_tad_with_alignment(_("_Indent table code:"), dg->check[2], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[2], 1, 5, 3, 4);

	html_diag_finish(dg, G_CALLBACK(table_wizard_ok_lcb));
}

static void frame_wizard_ok_lcb(GtkWidget * widget, Thtml_diag *dg) {
	gchar *title, *thestring, *frameset, *frames, *sizes, *dtd, *finalstring;
	gint i, count;
	gboolean need_comma=FALSE;

	if (GTK_TOGGLE_BUTTON(dg->check[0])->active) {
		dtd = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN\"\n\t\"http://www.w3.org/TR/html4/frameset.dtd\">";
	} else {
		dtd = "";
	}
	title = gtk_editable_get_chars(GTK_EDITABLE(dg->entry[12]), 0, -1);
	thestring = g_strconcat(dtd
			, cap("<HTML>\n<HEAD>\n<TITLE>")
			, title
			, cap("</TITLE>\n</HEAD>\n"), NULL);
	g_free(title);
	
	count = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dg->spin[1]));
	frames = g_strdup("");
	sizes = g_strdup("");
	for (i = 0; i < count ; i++) {
		gchar *tmpstr, *size, *name, *source;
		size = gtk_editable_get_chars(GTK_EDITABLE(dg->entry[i]), 0, -1);
		name = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(dg->combo[i])->entry), 0, -1);
		source = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(dg->combo[i+5])->entry), 0, -1);
		if (main_v->props.xhtml == 1) {
			tmpstr = g_strconcat(frames, cap("<FRAME NAME=\""), name, cap("\" SRC=\""), source, "\" />\n", NULL);
		} else {
			tmpstr = g_strconcat(frames, cap("<FRAME NAME=\""), name, cap("\" SRC=\""), source, "\">\n", NULL);
		}
		g_free(frames);
		frames = tmpstr;

		if (need_comma) {
			tmpstr = g_strconcat(sizes, ",", size, NULL);
		} else {
			tmpstr = g_strconcat(sizes, size, NULL);
		}
		g_free(sizes);
		sizes = tmpstr;

		need_comma = TRUE;

		g_free(size);
		g_free(name);
		g_free(source);
	}
	
	if (GTK_TOGGLE_BUTTON(dg->radio[1])->active) {
		frameset = g_strconcat(cap("<FRAMESET COLS=\""), sizes, "\">\n", NULL);
	} else {
		frameset = g_strconcat(cap("<FRAMESET ROWS=\""), sizes, "\">\n", NULL);
	}

	finalstring = g_strconcat(thestring, frameset, frames, cap("<NOFRAMES>\n\n</NOFRAMES>\n</FRAMESET>\n</HTML>"), NULL);
	g_free(sizes);
	g_free(frameset);
	g_free(frames);
	g_free(thestring);
	doc_insert_two_strings(dg->bfwin->current_document, finalstring, NULL);
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

static void frame_wizard_num_changed(GtkWidget *widget, Thtml_diag *dg) {
	gint num,i;
	num = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dg->spin[1]));
	for (i = 0; i < MAX_FRAMES_IN_FRAMEWIZARD ; i++) {
		if (i < num) {
			gtk_widget_set_sensitive(dg->clist[i], TRUE);
		} else {
			gtk_widget_set_sensitive(dg->clist[i], FALSE); 
		}
	}
}

void framewizard_dialog(Tbfwin *bfwin) {
	GtkWidget *dgtable, *frame, *vbox, *label;
	Thtml_diag *dg;
	gint i;

	dg = html_diag_new(bfwin,_("Frame Wizard"));

	dgtable = gtk_table_new(4, 12, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(dgtable), 6);
	gtk_table_set_col_spacings(GTK_TABLE(dgtable), 12);
	frame = gtk_aspect_frame_new(NULL, 0, 0.5, 0, TRUE);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_box_pack_start(GTK_BOX(dg->vbox), frame, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), dgtable);
	
	dg->entry[12] = entry_with_text(NULL, 256);
	bf_mnemonic_label_tad_with_alignment(_("_Title:"), dg->entry[12], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[12], 1, 12, 0, 1);

	dg->check[0] = gtk_check_button_new();
	bf_mnemonic_label_tad_with_alignment(_("Use _DTD:"), dg->check[0], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[0], 1, 2, 1, 2);

	label = gtk_label_new(_("Orientation:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), label, 0, 1, 2, 3);
	dg->radio[1] = radiobut_with_value(_("_Horizontal"), 0, NULL);
	dg->radio[2] = radiobut_with_value(_("_Vertical"), 0, GTK_RADIO_BUTTON(dg->radio[1]));
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->radio[1], 1, 2, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->radio[2], 2, 3, 2, 3);

	dg->spin[1] = spinbut_with_value("2", 1, MAX_FRAMES_IN_FRAMEWIZARD, 1.0, 2.0);
	bf_mnemonic_label_tad_with_alignment(_("Number of _Frames:"), dg->spin[1], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1, 2, 3, 4);
	g_signal_connect(G_OBJECT(dg->spin[1]), "changed", G_CALLBACK(frame_wizard_num_changed), dg);
	frame = gtk_frame_new(_("Frame's"));
	gtk_container_set_border_width(GTK_CONTAINER(frame), 6);
	gtk_box_pack_start(GTK_BOX(dg->vbox), frame, FALSE, FALSE, 0);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	for (i = 0; i < MAX_FRAMES_IN_FRAMEWIZARD ; i++) {
		GtkWidget *file_but;
	/* since there are no clists in this dialog we can use it freely :) */
		dg->clist[i] = gtk_hbox_new(FALSE, 6);
		gtk_box_pack_start(GTK_BOX(vbox), dg->clist[i], FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(dg->clist[i]), gtk_label_new(_("Name:")), TRUE, TRUE, 0);
		dg->combo[i] = boxed_combo_with_popdown(NULL, bfwin->session->targetlist, 1, dg->clist[i]);
		gtk_box_pack_start(GTK_BOX(dg->clist[i]), gtk_label_new(_("Source:")), TRUE, TRUE, 0);
		dg->combo[i+5] = boxed_combo_with_popdown(NULL, bfwin->session->urllist, 1, dg->clist[i]);
		file_but = file_but_new(GTK_WIDGET(GTK_COMBO(dg->combo[i+5])->entry), 0, bfwin);
		gtk_box_pack_start(GTK_BOX(dg->clist[i]), file_but, FALSE, FALSE, 0);		
		gtk_box_pack_start(GTK_BOX(dg->clist[i]), gtk_label_new(_("Size:")), TRUE, TRUE, 0);
		dg->entry[i] = boxed_entry_with_text(NULL, 100, dg->clist[i]);

	}
	html_diag_finish(dg, G_CALLBACK(frame_wizard_ok_lcb));
	frame_wizard_num_changed(NULL, dg);
}


