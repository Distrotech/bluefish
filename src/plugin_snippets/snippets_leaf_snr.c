/* Bluefish HTML Editor
 * snippets_leaf_snr.c - plugin for snippets sidebar
 *
 * Copyright (C) 2006-2011 Olivier Sessink
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
#include "../bf_lib.h"
#include "../dialog_utils.h"
#include "../gtk_easy.h"
#include "../snr3.h"

Tsnr3scope snippets_snr_region_from_char(const xmlChar *region) {
	if (region) {
		switch (region[0]) {  /* beginning, cursor, selection, allopenfiles */
			case 'c':
				return snr3scope_cursor;
			break;
			case 's':
				return snr3scope_selection;
			break;
			case 'a':
				return snr3scope_alldocs;
			break;
			case 'b':
			default:
				return snr3scope_doc;
			break;
		}
	} 
	return snr3scope_doc;
}

Tsnr3type snippets_snr_matchtype_from_char(const xmlChar *matchtype) {
	/* since snr2 dropped support for posix, we try to run them as pcre. */
	if (matchtype && (xmlStrEqual(matchtype, (const xmlChar *)"posix")
						||xmlStrEqual(matchtype, (const xmlChar *)"perl")))
		return snr3type_pcre;
	return snr3type_string;
}

static void snippets_snr_run_from_strings(Tdocument *doc, const xmlChar *searchpat,const xmlChar *region, 
					const xmlChar *matchtype,const xmlChar *casesens, const xmlChar *replacepat, const xmlChar *useescapechars) {
	gint casesensnum,unescape;
	Tsnr3scope scope;
	Tsnr3type type;
	
	scope = snippets_snr_region_from_char(region);
	type = snippets_snr_matchtype_from_char(matchtype);
	casesensnum = (casesens && casesens[0] == '1');
	unescape = (useescapechars && useescapechars[0]=='1');
	/* snr3_run_extern_replace
	 * doc: a #Tdocument
	 * search_pattern: #gchar* to search pattern
	 * scope: #gint, 0 = region_from_beginning, 1 = region_from_cursor, 2 = region_selection, 3 = region_all_open_files
	 * matchtype: #gint, 0 = normal, 1 = posix, 2 = perl
	 * is_case_sens: #gint
	 * replace_pattern: #gchar* to replace pattern.
	 * unescape: #gint
	 * dotmatchall: gboolean
	 * */
	DEBUG_MSG("snippets_snr_run_from_strings, useescapechars=%s, unescape=%d\n",useescapechars,unescape);
	snr3_run_extern_replace(doc, (gchar *)searchpat, 
						scope,type, casesensnum, (gchar *)replacepat,unescape, FALSE);
}

typedef struct {
	GtkWidget *dialog;
	GtkWidget *textentry[10];
	Tsnippetswin *snw;
	xmlNodePtr leaf;
} Tsnippet_insert_dialog;

static int snippets_snr_num_params(xmlNodePtr parent) {
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

static void snippets_snr_dialog(Tsnippetswin *snw, xmlNodePtr leaf, gint num_vars) {
	Tsnippet_insert_dialog *sid;
	xmlChar *title;
	xmlChar *searchpat=NULL;
	xmlChar *replacepat=NULL;
	xmlNodePtr cur;
	int i=0;
	GtkWidget *table, *label, *vbox;
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
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(sid->dialog));
	gtk_box_set_spacing(GTK_BOX(vbox),6);
	table = gtk_table_new(num_vars+1, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	for (cur = leaf->xmlChildrenNode;cur != NULL;cur = cur->next) {
		if (xmlStrEqual(cur->name, (const xmlChar *)"param")) {
			xmlChar *name;
			gchar *final_name;
			name =  xmlGetProp(cur, (const xmlChar *)"name");
			final_name = g_markup_escape_text((gchar *)name,-1);
			sid->textentry[i] = gtk_entry_new();
			gtk_entry_set_activates_default(GTK_ENTRY(sid->textentry[i]),TRUE);
			dialog_mnemonic_label_in_table(final_name, sid->textentry[i], table, 0, 1, i+1, i+2);
			gtk_table_attach(GTK_TABLE (table), sid->textentry[i], 1, 2, i+1, i+2, GTK_EXPAND|GTK_FILL, GTK_SHRINK, 0, 0);
			xmlFree(name);
			g_free(final_name);
			i++;
		} else if (xmlStrEqual(cur->name, (const xmlChar *)"searchpat")) {
			searchpat = xmlNodeListGetString(snippets_v.doc, cur->xmlChildrenNode, 1);
		} else if (xmlStrEqual(cur->name, (const xmlChar *)"replacepat")) {
			replacepat = xmlNodeListGetString(snippets_v.doc, cur->xmlChildrenNode, 1);
			if (!replacepat)
				replacepat = (guchar *) g_strdup("");
		}
	}
	if (!searchpat) {
		g_print("Empty search string\n");
		return;
	} 
	tmpstr = g_strconcat(_("Search for: '"), searchpat,_("', replace with: '"),replacepat,"'",NULL);
	label = gtk_label_new(tmpstr);
	g_free(tmpstr);
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	gtk_table_attach(GTK_TABLE (table), label, 0, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	
	sid->textentry[i] = NULL;
	gtk_container_add(GTK_CONTAINER(vbox),table);
	
	gtk_widget_show_all(sid->dialog);
	
	
	if (gtk_dialog_run(GTK_DIALOG(sid->dialog))==GTK_RESPONSE_ACCEPT) {
		xmlChar *region=NULL, *matchtype=NULL, *casesens=NULL, *escapechars=NULL;
		Tconvert_table *ctable;
		gchar *search_final=NULL, *replace_final=NULL;
		DEBUG_MSG("snippets_insert_dialog, building convert table, num_vars=%d\n",num_vars);
		ctable = g_new(Tconvert_table,num_vars+2);
		for (i=0;i<num_vars && sid->textentry[i]!=NULL;i++) {
			ctable[i].my_int = 48 + i; /* 48 for the ascii number */
			ctable[i].my_char = gtk_editable_get_chars(GTK_EDITABLE(sid->textentry[i]), 0, -1);
		}
		ctable[i].my_int = '%';
		ctable[i].my_char = g_strdup("%");
		ctable[i+1].my_char = NULL;
		
		search_final = replace_string_printflike((gchar *)searchpat, ctable);
		xmlFree(searchpat);
		if (replacepat) {
			replace_final = replace_string_printflike((gchar *)replacepat, ctable);
			xmlFree(replacepat);
		} else {
			replace_final = g_strdup("");
		}
		
		free_convert_table(ctable);
		
		region = xmlGetProp(leaf, (const xmlChar *)"region");
		matchtype = xmlGetProp(leaf, (const xmlChar *)"matchtype");
		casesens = xmlGetProp(leaf, (const xmlChar *)"casesens");
		escapechars = xmlGetProp(leaf, (const xmlChar *)"escapechars");
		snippets_snr_run_from_strings(snw->bfwin->current_document, (const xmlChar *)search_final,region,matchtype,casesens, (const xmlChar *)replace_final, escapechars);
		g_free(replace_final);
	}
	gtk_widget_destroy(sid->dialog);
	g_free(sid);
}

void snippets_activate_leaf_snr(Tsnippetswin *snw, xmlNodePtr parent) {
	gint num_vars = snippets_snr_num_params(parent);
	DEBUG_MSG("snippets_activate_leaf_snr, num_vars=%d\n",num_vars);
	if (num_vars == 0) {
		xmlChar *searchpat=NULL;
		xmlChar *replacepat=NULL;
		xmlChar *region=NULL, *matchtype=NULL, *casesens=NULL, *escapechars=NULL;
		
		xmlNodePtr cur = parent->xmlChildrenNode;
		while (cur != NULL && (searchpat == NULL || replacepat == NULL)) {
			if (xmlStrEqual(cur->name, (const xmlChar *)"searchpat")) {
				searchpat = xmlNodeListGetString(snippets_v.doc, cur->xmlChildrenNode, 1);
			} else if (xmlStrEqual(cur->name, (const xmlChar *)"replacepat")) {
				replacepat = xmlNodeListGetString(snippets_v.doc, cur->xmlChildrenNode, 1);
				if (!replacepat)
					replacepat= (guchar *) g_strdup("");
			}
			cur = cur->next;
		}
		region = xmlGetProp(parent, (const xmlChar *)"region");
		matchtype = xmlGetProp(parent, (const xmlChar *)"matchtype");
		casesens = xmlGetProp(parent, (const xmlChar *)"casesens");
		escapechars = xmlGetProp(parent, (const xmlChar *)"escapechars");
		DEBUG_MSG("snippets_activate_leaf_snr, region=%s,matchtype=%s,casesens=%s,escapechars=%s\n",region,matchtype,casesens,escapechars);
		snippets_snr_run_from_strings(snw->bfwin->current_document, searchpat,region,matchtype,casesens, replacepat,escapechars);
	} else {
		snippets_snr_dialog(snw, parent, num_vars);
	}
}
