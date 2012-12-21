/* Bluefish HTML Editor
 * snippets_leaf_insert.c - plugin for snippets sidebar
 *
 * Copyright (C) 2006,2010 Olivier Sessink
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

#include "snippets.h"
#include "snippets_gui.h"
#include "../dialog_utils.h"
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

static gchar *snippets_strings2ui(const gchar *before, gint beforelen, const gchar *after, gint afterlen) {
	gchar *retval;
	gchar *tmpb=NULL, *tmpa=NULL;

	if (beforelen > 30) {
		gchar *tmp = g_strndup((gchar *)before, 30);
		tmpb = g_strconcat(tmp, " etc. etc.",NULL);
		g_free(tmp);
	}
	if (afterlen > 30) { 
		gchar *tmp = g_strndup((gchar *)after, 30);
		tmpa = g_strconcat(tmp, " etc. etc.",NULL);
		g_free(tmp);
	}
	if (before && after) {
		retval = g_strconcat((gchar *)(tmpb?tmpb:before),_("[cursor position or selection]"),(gchar *)(tmpa?tmpa:after),NULL);
	} else if (before) {
		retval = g_strdup((gchar *)(tmpb?tmpb:before));
	} else if (after) {
		retval = g_strdup((gchar *)(tmpa?tmpa:after));
	} else {
		retval = g_strdup("An error has occurred with this item");
	}
	g_free(tmpa);
	g_free(tmpb);
	return retval;
}

gchar *snippets_tooltip_from_insert_content(xmlNodePtr leaf) {
	xmlNodePtr cur;
	gchar *tmpstr;
	gchar *before=NULL, *after=NULL;
	gint beforelen=0,afterlen=0;
	for (cur = leaf->xmlChildrenNode;cur != NULL;cur = cur->next) {
		if (xmlStrEqual(cur->name, (const xmlChar *)"before")) {
			before = (gchar *)xmlNodeListGetString(snippets_v.doc, cur->xmlChildrenNode, 1);
			beforelen = before?strlen((gchar *)before):0;
		} else if (xmlStrEqual(cur->name, (const xmlChar *)"after")) {
			after = (gchar *)xmlNodeListGetString(snippets_v.doc, cur->xmlChildrenNode, 1);
			afterlen = after?strlen((gchar *)after):0;
		}
	}
	tmpstr = snippets_strings2ui(before, beforelen, after, afterlen);
	xmlFree(before);
	xmlFree(after);
	return tmpstr;
}

static void snippets_insert_dialog(Tsnippetswin *snw, xmlNodePtr leaf, gint num_vars) {
	Tsnippet_insert_dialog *sid;
	xmlChar *title;
	gchar *before=NULL, *after=NULL;
	xmlNodePtr cur;
	int i=0;
	GtkWidget *table, *label, *vbox;
	gchar *tmpstr;
	gint beforelen=0,afterlen=0;
	
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
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(sid->dialog));
	gtk_box_set_spacing(GTK_BOX(vbox),6);
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
			dialog_mnemonic_label_in_table(final_name, sid->textentry[i], table, 0, 1, i+1, i+2);
			if (is_file && is_file[0]=='1') {
				GtkWidget *but;
				gtk_table_attach(GTK_TABLE (table), sid->textentry[i], 1, 2, i+1, i+2, GTK_EXPAND|GTK_FILL, GTK_SHRINK, 0, 0);
				but = file_but_new2(sid->textentry[i], 0, snw->bfwin,GTK_FILE_CHOOSER_ACTION_OPEN);
				gtk_table_attach(GTK_TABLE (table), but, 2, 3, i+1, i+2, GTK_FILL, GTK_SHRINK, 0, 0);
			} else {
				gtk_table_attach(GTK_TABLE (table), sid->textentry[i], 1, 3, i+1, i+2, GTK_EXPAND|GTK_FILL, GTK_SHRINK, 0, 0);
			}
			xmlFree(name);
			g_free(final_name);
			i++;
		} else if (xmlStrEqual(cur->name, (const xmlChar *)"before")) {
			before = (gchar *)xmlNodeListGetString(snippets_v.doc, cur->xmlChildrenNode, 1);
			beforelen = before?strlen((gchar *)before):0;
		} else if (xmlStrEqual(cur->name, (const xmlChar *)"after")) {
			after = (gchar *)xmlNodeListGetString(snippets_v.doc, cur->xmlChildrenNode, 1);
			afterlen = after?strlen((gchar *)after):0;
		}
	}
	tmpstr = snippets_strings2ui(before, beforelen, after, afterlen);
	label = gtk_label_new(tmpstr);
	g_free(tmpstr);
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	gtk_table_attach(GTK_TABLE (table), label, 0, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	
	sid->textentry[i] = NULL;
	gtk_container_add(GTK_CONTAINER(vbox),table);
	
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

static gchar *
convert_noargs(const gchar *string)
{
	Tconvert_table *ctable;
	gchar *newstring;
	ctable = g_new(Tconvert_table,2);
	ctable[0].my_int = '%';
	ctable[0].my_char = g_strdup("%");
	ctable[1].my_char = NULL;
	newstring = replace_string_printflike((gchar *)string, ctable);
	free_convert_table(ctable);
	return newstring;
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
			gchar *newbefore=NULL, *newafter=NULL;
			if (after)
				newafter = convert_noargs((const gchar *)after);
			if (before)
				newbefore = convert_noargs((const gchar *)before);
			doc_insert_two_strings(snw->bfwin->current_document, newbefore, newafter);
			if (before) xmlFree(before);
			if (after) xmlFree(after);
		}
	} else {
		snippets_insert_dialog(snw, parent, num_vars);
	}
}
