/* Bluefish HTML Editor
 * html_diag.c - general functions to create HTML dialogs
 *
 * Copyright (C) 2000-2002 Olivier Sessink
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
#include <string.h>	/* strrchr */
#include <stdlib.h> /* strtod */
/* #define DEBUG */

#include "bluefish.h"
#include "html2.h" /* style_but_new() */
#include "html_diag.h" /* myself */
#include "gtk_easy.h" /* window_full() */
#include "bf_lib.h"
#include "stringlist.h" /* add_to_stringlist */
#include "document.h" /* doc_save_selection */
#include "cap.h"

Trecent_attribs recent_attribs;
/*****************************************/
/********** DIALOG FUNCTIONS *************/
/*****************************************/

void html_diag_destroy_cb(GtkWidget * widget, Thtml_diag *dg) {
	dg->tobedestroyed = TRUE;
	DEBUG_MSG("html_diag_destroy_cb, widget=%p, dg=%p, dg->dialog=%p\n",widget,dg,dg->dialog);
	if (dg->mark_ins) {
		gtk_text_buffer_delete_mark(dg->doc->buffer,dg->mark_ins);
		gtk_text_buffer_delete_mark(dg->doc->buffer,dg->mark_sel);
	}
	window_destroy(dg->dialog);
	DEBUG_MSG("html_diag_destroy_cb, about to free dg=%p\n",dg);
	g_free(dg);
}

void html_diag_cancel_clicked_cb(GtkWidget *widget, gpointer data) {
	html_diag_destroy_cb(NULL, data);
}

Thtml_diag *html_diag_new(Tbfwin *bfwin, gchar *title) {
	Thtml_diag *dg;
	
	dg = g_malloc(sizeof(Thtml_diag));
	dg->tobedestroyed = FALSE;
	DEBUG_MSG("html_diag_new, dg=%p\n",dg);
	dg->dialog = window_full2(title, GTK_WIN_POS_MOUSE
		, 12,G_CALLBACK(html_diag_destroy_cb), dg, TRUE,  bfwin ? bfwin->main_window : NULL);
	gtk_window_set_type_hint(GTK_WINDOW(dg->dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_role(GTK_WINDOW(dg->dialog), "html_dialog");
	dg->vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(dg->dialog), dg->vbox);
#ifdef DEBUG
	{
		Tdocument *doc = bfwin->current_document;
		GtkTextIter iter1, iter2;
		GtkTextMark* mark;
		mark = gtk_text_buffer_get_mark(doc->buffer,"insert");
		gtk_text_buffer_get_iter_at_mark(doc->buffer,&iter1,mark);
		mark = gtk_text_buffer_get_mark(doc->buffer,"selection_bound");
		gtk_text_buffer_get_iter_at_mark(doc->buffer,&iter2,mark);
		g_print("html_diag_new, current marks: insert=%d, selection=%d\n",gtk_text_iter_get_offset(&iter1),gtk_text_iter_get_offset(&iter2));
	}
#endif
	{
		if (!gtk_text_buffer_get_mark(bfwin->current_document->buffer,"diag_ins")) {
			GtkTextIter iter;
			GtkTextMark* mark = gtk_text_buffer_get_mark(bfwin->current_document->buffer,"insert");
			gtk_text_buffer_get_iter_at_mark(bfwin->current_document->buffer,&iter,mark);
			dg->mark_ins = gtk_text_buffer_create_mark(bfwin->current_document->buffer,"diag_ins",&iter,TRUE);
			mark = gtk_text_buffer_get_mark(bfwin->current_document->buffer,"selection_bound");
			gtk_text_buffer_get_iter_at_mark(bfwin->current_document->buffer,&iter,mark);
			dg->mark_sel = gtk_text_buffer_create_mark(bfwin->current_document->buffer,"diag_sel",&iter,TRUE);
		} else {
			dg->mark_ins = dg->mark_sel = NULL;
		}
	}
	dg->range.pos = -1;
	dg->range.end = -1;
	if (main_v->props.transient_htdialogs) {
		/* must be set before realizing */
		DEBUG_MSG("html_diag_finish, setting dg->dialog=%p transient!\n", dg->dialog);
		gtk_window_set_transient_for(GTK_WINDOW(dg->dialog), GTK_WINDOW(bfwin->main_window));
	}

	gtk_widget_realize(dg->dialog);
	dg->bfwin = bfwin;
	dg->doc = bfwin->current_document;
	return dg;
}

GtkWidget *html_diag_table_in_vbox(Thtml_diag *dg, gint rows, gint cols) {
	GtkWidget *returnwidget = gtk_table_new(rows, cols, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(returnwidget), 12);
	gtk_table_set_col_spacings(GTK_TABLE(returnwidget), 12);
	gtk_box_pack_start(GTK_BOX(dg->vbox), returnwidget, FALSE, FALSE, 0);
	return returnwidget;
}

void html_diag_finish(Thtml_diag *dg, GtkSignalFunc ok_func) {
	GtkWidget *hbox;

	gtk_box_pack_start(GTK_BOX(dg->vbox), gtk_hseparator_new(), FALSE, FALSE, 12);
	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 12);
	
	dg->obut = bf_stock_ok_button(ok_func, dg);
	dg->cbut = bf_stock_cancel_button(G_CALLBACK(html_diag_cancel_clicked_cb), dg);

	gtk_box_pack_start(GTK_BOX(hbox),dg->cbut , FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox),dg->obut , FALSE, FALSE, 0);
	gtk_window_set_default(GTK_WINDOW(dg->dialog), dg->obut);

	gtk_box_pack_start(GTK_BOX(dg->vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show_all(GTK_WIDGET(dg->dialog));
}

/*************************************************/
/********** HTML -> DIALOG FUNCTIONS *************/
/*************************************************/

void parse_html_for_dialogvalues(gchar *dialogitems[], gchar *dialogvalues[]
		, gchar **custom, Ttagpopup *data) {
	gint count=0;
	gchar *customnew;
	GList *tmplist;
	gboolean found = FALSE;

	*custom = g_strdup("");
	tmplist = g_list_first(data->taglist);
	DEBUG_MSG("fill_dialogvalues, data, and no sending widget!\n");
	while (tmplist) {
		count = 0;
		found = FALSE;
		while (dialogitems[count]) {
			if (strcmp(((Ttagitem *) tmplist->data)->item, dialogitems[count]) == 0) {
				DEBUG_MSG("fill_dialogvalues, string found!, dialogitems[%d]=%s\n", count, dialogitems[count]);
				dialogvalues[count] = ((Ttagitem *) tmplist->data)->value;
				DEBUG_MSG("fill_dialogvalues, *dialogvalues[%d]=%s\n", count, dialogvalues[count]);
				found = TRUE;
			}
			count++;
		}
		if (!found) {
			customnew = g_strjoin(NULL, *custom, " ", ((Ttagitem *) tmplist->data)->item, NULL);
			if (*custom) g_free(*custom);
			*custom = customnew;
			if (((Ttagitem *) tmplist->data)->value) {
				customnew = g_strjoin(NULL, *custom, "=\"", ((Ttagitem *) tmplist->data)->value, "\"", NULL);
				if (*custom) g_free(*custom);
				*custom = customnew;
			}
		}
		tmplist = g_list_next(tmplist);
	}
}

void fill_dialogvalues(gchar *dialogitems[], gchar *dialogvalues[]
		, gchar **custom, Ttagpopup *data, Thtml_diag *diag) {

	gint count=0;
	
	while (dialogitems[count]) {
		dialogvalues[count] = NULL;
		count++;
	}

	/* when there is NO sending_widget, but there is data, the function 
	 * is called by the right-click-opup, so data will contain info about
	 * the position of the tag */

	if (data) {
		parse_html_for_dialogvalues(dialogitems, dialogvalues, custom, data);
		diag->range.pos = data->pos;
		diag->range.end = data->end;
	} else {
		DEBUG_MSG("fill_dialogvalues, no data, or a sending widget\n");
		diag->range.pos = -1;
		diag->range.end = -1;
	}
}

void parse_existence_for_dialog(gchar * valuestring, GtkWidget * checkbox)
{

	if (valuestring) {
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(checkbox), TRUE);
		DEBUG_MSG("parse_existence_for_dialog, checkbox set YES, valuestring=%p\n", valuestring);
	} else {
		DEBUG_MSG("parse_existence_for_dialog, NO checkbox set, valuestring=%p\n", valuestring);
	}
}

void parse_integer_for_dialog(gchar * valuestring, GtkWidget * spin, GtkWidget * entry, GtkWidget * checkbox)
{

	gchar *tmp;
	gint value = 0;
	gboolean found = FALSE;
	gchar *sign = NULL;
	gboolean percentage = FALSE;

	DEBUG_MSG("parse_integer_for_dialog, started, valuestring=%s\n", valuestring);
	if (!valuestring) {
		if (spin)
			gtk_entry_set_text(GTK_ENTRY(GTK_SPIN_BUTTON(spin)), "");
		if (entry)
			gtk_entry_set_text(GTK_ENTRY(entry), "");
		return;
	}
	tmp = strrchr(valuestring, '-');
	if (tmp) {
		DEBUG_MSG("parse_integer_for_dialog, found a -!, tmp=%s\n", tmp);
		value = strtod(++tmp, NULL);
		sign = "-";
		found = TRUE;
		DEBUG_MSG("parse_integer_for_dialog, value=%d\n", value);
	}
	tmp = strrchr(valuestring, '+');
	if (tmp) {
		DEBUG_MSG("parse_integer_for_dialog, found a +!, tmp=%s\n", tmp);
		value = strtod(++tmp, NULL);
		sign = "+";
		found = TRUE;
		DEBUG_MSG("parse_integer_for_dialog, value=%d\n", value);
	}
	tmp = strchr(valuestring, '%');
	if (tmp) {
		DEBUG_MSG("parse_integer_for_dialog, found a percentage!\n");
		valuestring = trunc_on_char(valuestring, '%');
		value = strtod(valuestring, NULL);
		percentage = TRUE;
		found = TRUE;
		DEBUG_MSG("parse_integer_for_dialog, value=%d\n", value);
	}
	if (!found) {
		value = strtod(valuestring, NULL);
	}
	if (spin) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), value);
	}

	if (entry) {
		if (sign) {
			gtk_entry_set_text(GTK_ENTRY(entry), sign);
		} else {
			gtk_entry_set_text(GTK_ENTRY(entry), "");
		}
	}

	if (checkbox) {
		DEBUG_MSG("parse_integer_for_dialog, checkbox set\n");
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(checkbox), percentage);
	} else {
		DEBUG_MSG("parse_integer_for_dialog, NO checkbox set\n");
	}
}

/*************************************************/
/********** DIALOG -> HTML FUNCTIONS *************/
/*************************************************/

gchar *insert_string_if_entry(GtkWidget * entry, gchar * itemname, gchar * string2add2, gchar *defaultvalue)
{

	gchar *tempstring=NULL;
	gchar *entryvalue=NULL, *value=NULL;

	if (!entry) {
		value = defaultvalue;
	} else {
		entryvalue = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
		if (strlen(entryvalue)) {
			value = entryvalue;
		} else{
			value = defaultvalue;
		} 
	}
	if (value) {
		if (itemname) {
			tempstring = g_strdup_printf("%s %s=\"%s\"", string2add2, itemname, value);
		} else {
			tempstring = g_strdup_printf("%s %s", string2add2, value);
		}
		g_free(string2add2);
		string2add2 = tempstring;
	}
	if (entryvalue) {
		g_free(entryvalue);
	}
	return string2add2;
}

gchar *insert_integer_if_spin(GtkWidget * spin, gchar * itemname, gchar * string2add2, gboolean ispercentage, gint dontinsertonvalue) {
	gint val = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
	if (val != dontinsertonvalue) {
		gchar *tempstring;
		if (ispercentage) {
			tempstring = g_strdup_printf("%s %s=\"%d%%\"", string2add2, itemname, val);
		} else {
			tempstring = g_strdup_printf("%s %s=\"%d\"", string2add2, itemname, val);
		}
		g_free(string2add2);
		string2add2 = tempstring;
	}
	return string2add2;
}

gchar *insert_attr_if_checkbox(GtkWidget * checkbox, gchar * itemname, gchar *string2add2) {

	gchar *tempstring;
	
	if (GTK_TOGGLE_BUTTON(checkbox)->active) {
		tempstring = g_strdup_printf("%s %s",string2add2, itemname);
		g_free(string2add2);
		string2add2 = tempstring;
	}
	return string2add2;
}

gchar *format_entry_into_string(GtkEntry * entry, gchar * formatstring) {
	gchar * tempstring;
	
	if (strlen(gtk_entry_get_text(GTK_ENTRY(entry)))) {
		tempstring = g_strdup_printf(formatstring, gtk_entry_get_text(GTK_ENTRY(entry)));
	} else {
		tempstring = g_strdup("");
	}
	return tempstring;

}

GList *add_entry_to_stringlist(GList *which_list, GtkWidget *entry) {
	if (entry) {
		gchar *temp = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
		which_list = add_to_stringlist(which_list, temp);
		g_free(temp);
	}
	return which_list;
}

GtkWidget *generic_table_inside_notebookframe(GtkWidget *notebook, const gchar *title, gint rows, gint cols) {
	GtkWidget *dgtable, *frame = bf_generic_frame_new(NULL, GTK_SHADOW_NONE, 12);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, gtk_label_new(title));
	dgtable = gtk_table_new(rows, cols, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(dgtable), 6);
	gtk_table_set_col_spacings(GTK_TABLE(dgtable), 12);
	gtk_container_add(GTK_CONTAINER(frame), dgtable);
	return dgtable;
}

void generic_class_id_style_section(Thtml_diag *dg, gint firstattrwidget, GtkWidget *dgtable, gint firstrowintable, gchar **tagvalues, gint firsttagvalue) {
	GtkWidget *but;

	dg->attrwidget[firstattrwidget] = combo_with_popdown(tagvalues[firsttagvalue], dg->bfwin->session->classlist, 1);
	bf_mnemonic_label_tad_with_alignment(_("Cl_ass:"), dg->attrwidget[firstattrwidget], 0, 0.5, dgtable, 0, 1, firstrowintable+0, firstrowintable+1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->attrwidget[firstattrwidget], 1, 3, firstrowintable+0, firstrowintable+1);

	dg->attrwidget[firstattrwidget+1] = entry_with_text(tagvalues[firsttagvalue+1], 256);
	bf_mnemonic_label_tad_with_alignment(_("_ID:"), dg->attrwidget[firstattrwidget+1], 0, 0.5, dgtable, 0, 1, firstrowintable+1, firstrowintable+2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->attrwidget[firstattrwidget+1], 1, 3, firstrowintable+1, firstrowintable+2);

	dg->attrwidget[firstattrwidget+2] = entry_with_text(tagvalues[firsttagvalue+2], 256);
	bf_mnemonic_label_tad_with_alignment(_("St_yle:"), dg->attrwidget[firstattrwidget+2], 0, 0.5, dgtable, 0, 1, firstrowintable+2, firstrowintable+3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->attrwidget[firstattrwidget+2], 1, 2, firstrowintable+2, firstrowintable+3);
	but = style_but_new(dg->attrwidget[firstattrwidget+2], dg->dialog);
	gtk_table_attach(GTK_TABLE(dgtable), but, 2, 3, firstrowintable+2, firstrowintable+3, GTK_SHRINK, GTK_SHRINK, 0, 0);
}
