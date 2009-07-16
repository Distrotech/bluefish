/* Bluefish HTML Editor
 * snippets_leaf_insert.c - plugin for snippets sidebar
 *
 * Copyright (C) 2006,2009 Olivier Sessink
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

#include <string.h>

#include "snippets.h"
#include "snippets_gui.h"
#include "../document.h"
#include "../gtk_easy.h"
#include "../bf_lib.h"

typedef struct {
	GtkWidget *dialog;
	GtkWidget *textentry[10];
	Tsnippetswin *snw;
	xmlNodePtr leaf;
} Tsnippet_insert_dialog;

static int snippets_insert_num_params(xmlNodePtr parent) {
	int num=0;
	xmlNodePtr cur = parent->xmlChildrenNode;
	while (cur != NULL) {
		if (xmlStrEqual(cur->name, (const xmlChar *)"param")) {
			num++;
		}
		cur = cur->next;
	}
	return num;
}

static void snippets_insert_dialog(Tsnippetswin *snw, xmlNodePtr leaf, gint num_vars) {
	Tsnippet_insert_dialog *sid;
	xmlChar *title;
	xmlChar *before=NULL;
	xmlChar *after=NULL;
	xmlNodePtr cur;
	int i=0;
	GtkWidget *table, *label;
	gchar *tmpstr;
	
	title = xmlGetProp(leaf, (const xmlChar *)"title");
	sid = g_new0(Tsnippet_insert_dialog,1);
	DEBUG_MSG("snippets_insert_dialog, started for %s\n",(gchar *)title);
	sid->dialog = gtk_dialog_new_with_buttons((gchar *)title,GTK_WINDOW(snw->bfwin->main_window),
							GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,
							GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
							NULL);
	xmlFree(title);
	gtk_dialog_set_default_response (GTK_DIALOG(sid->dialog),GTK_RESPONSE_ACCEPT);
	gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(sid->dialog)->vbox),6);
	table = gtk_table_new(num_vars+1, 3, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	for (cur = leaf->xmlChildrenNode;cur != NULL;cur = cur->next) {
		if (xmlStrEqual(cur->name, (const xmlChar *)"param")) {
			xmlChar *name, *is_file;
			gchar *final_name;
			name = xmlGetProp(cur, (const xmlChar *)"name");
			is_file = xmlGetProp(cur, (const xmlChar *)"is_file");
			final_name = g_markup_escape_text((gchar *)name,-1);
			sid->textentry[i] = gtk_entry_new();
			gtk_entry_set_activates_default(GTK_ENTRY(sid->textentry[i]),TRUE);
			bf_mnemonic_label_tad_with_alignment(final_name, sid->textentry[i], 0, 0.5, table, 0, 1, i+1, i+2);
			if (is_file && is_file[0]=='1') {
				GtkWidget *but;
				gtk_table_attach(GTK_TABLE (table), sid->textentry[i], 1, 2, i+1, i+2, GTK_EXPAND|GTK_FILL, GTK_SHRINK, 0, 0);
				but = file_but_new2(sid->textentry[i], "", snw->bfwin,GTK_FILE_CHOOSER_ACTION_OPEN);
				gtk_table_attach(GTK_TABLE (table), but, 2, 3, i+1, i+2, GTK_FILL, GTK_SHRINK, 0, 0);
			} else {
				gtk_table_attach(GTK_TABLE (table), sid->textentry[i], 1, 3, i+1, i+2, GTK_EXPAND|GTK_FILL, GTK_SHRINK, 0, 0);
			}
			xmlFree(name);
			g_free(final_name);
			i++;
		} else if (xmlStrEqual(cur->name, (const xmlChar *)"before")) {
			before = xmlNodeListGetString(snippets_v.doc, cur->xmlChildrenNode, 1);
		} else if (xmlStrEqual(cur->name, (const xmlChar *)"after")) {
			after = xmlNodeListGetString(snippets_v.doc, cur->xmlChildrenNode, 1);
		}
	}
	if (before && after) {
		tmpstr = g_strconcat((gchar *)before,_("[cursor position or selection]"),(gchar *)after,NULL);
	} else if (before) {
		tmpstr = g_strdup((gchar *)before);
	} else if (after) {
		tmpstr = g_strdup((gchar *)after);
	} else {
		tmpstr = g_strdup("An error has occurred with this item");
	}
	label = gtk_label_new(tmpstr);
	g_free(tmpstr);
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	gtk_table_attach(GTK_TABLE (table), label, 0, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	
	sid->textentry[i] = NULL;
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(sid->dialog)->vbox),table);
	
	gtk_widget_show_all(sid->dialog);
	
	
	if (gtk_dialog_run(GTK_DIALOG(sid->dialog))==GTK_RESPONSE_ACCEPT) {
		Tconvert_table *ctable;
		gchar *before_final=NULL, *after_final=NULL;
		DEBUG_MSG("snippets_insert_dialog, building convert table, num_vars=%d\n",num_vars);
		ctable = g_new(Tconvert_table,num_vars+2);
		for (i=0;i<num_vars && sid->textentry[i]!=NULL;i++) {
			ctable[i].my_int = 48 + i; /* 48 for the ascii number */
			ctable[i].my_char = gtk_editable_get_chars(GTK_EDITABLE(sid->textentry[i]), 0, -1);
		}
		ctable[i].my_int = '%';
		ctable[i].my_char = g_strdup("%");
		ctable[i+1].my_char = NULL;
		
		if (before) {
			before_final = replace_string_printflike((gchar *)before, ctable);
			xmlFree(before);
		}
		if (after) {
			after_final = replace_string_printflike((gchar *)after, ctable);
			xmlFree(after);
		}
		free_convert_table(ctable);
		doc_insert_two_strings(snw->bfwin->current_document, before_final, after_final);
		doc_scroll_to_cursor(snw->bfwin->current_document);
	}
	gtk_widget_destroy(sid->dialog);
	g_free(sid);
}


void snippets_activate_leaf_insert(Tsnippetswin *snw, xmlNodePtr parent) {

	gint num_vars = snippets_insert_num_params(parent);
	DEBUG_MSG("snippets_activate_leaf_insert, num_vars=%d\n",num_vars);
	if (num_vars == 0) {
		xmlChar *before=NULL;
		xmlChar *after=NULL;
		xmlNodePtr cur = parent->xmlChildrenNode;
		while (cur != NULL && (before == NULL || after == NULL)) {
			if (xmlStrEqual(cur->name, (const xmlChar *)"before")) {
				before = xmlNodeListGetString(snippets_v.doc, cur->xmlChildrenNode, 1);
			} else if (xmlStrEqual(cur->name, (const xmlChar *)"after")) {
				after = xmlNodeListGetString(snippets_v.doc, cur->xmlChildrenNode, 1);
			}
			cur = cur->next;
		}
		
		if (before || after) {
			doc_insert_two_strings(snw->bfwin->current_document, (const gchar *)before, (const gchar *)after);
			if (before) xmlFree(before);
			if (after) xmlFree(after);
		}
	} else {
		snippets_insert_dialog(snw, parent, num_vars);
	}
}
