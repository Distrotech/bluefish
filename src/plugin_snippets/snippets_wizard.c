/* Bluefish HTML Editor
 * snippets_wizard.c - plugin for snippets sidebar
 *
 * Copyright (C) 2006,2009-2011 Olivier Sessink
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
/*#define DEBUG*/

#include <string.h>

#include "snippets.h"
#include "snippets_load.h"
#include "snippets_leaf_insert.h"
#include "snippets_leaf_snr.h"
#include "../gtk_easy.h"
#include "../dialog_utils.h"

typedef enum {
	page_type, /* 0 */
	page_name,
	page_branch,
	page_insert,
	page_snr,
	page_finished
} Tpagenum;

typedef struct {
	Tsnippetswin *snw;
	GtkWidget *dialog;
	gint choice;
	gchar *name;
	gchar *description;
	gpointer pagestruct;
	Tpagenum pagenum;
	xmlNodePtr node;
} Tsnipwiz;

static void get_parentbranch(Tsnippetswin *snw, GtkTreePath **parentp, xmlNodePtr *parentn) {
	if (snw->lastclickedpath) {
		*parentp = gtk_tree_path_copy(snw->lastclickedpath);
	} else {
		*parentp = NULL;
	}
	if (snw->lastclickednode) {
		if (xmlStrEqual(snw->lastclickednode->name, (const xmlChar *)"leaf")) {
			*parentn = snw->lastclickednode->parent;
			if (*parentp) {
				if (!gtk_tree_path_up(*parentp)) {
					gtk_tree_path_free(*parentp);
					*parentp = NULL;
				}
			}
		} else {
			*parentn = snw->lastclickednode;
		}
	} else { /* parent must be the root element <snippets> */
		*parentn = xmlDocGetRootElement(snippets_v.doc);
	}
} 

static void add_item_to_tree(GtkTreePath *parentp, gint pixmaptype, const gchar *name, gpointer ptr) {
	GtkTreeIter citer;
	if (parentp) {
		GtkTreeIter piter;
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(snippets_v.store),&piter,parentp)) {
			gtk_tree_store_append(GTK_TREE_STORE(snippets_v.store), &citer, &piter);
			snippets_fill_tree_item_from_node(&citer, ptr);
		} else {
			g_print("hmm weird error!?!\n");
		}
	} else {
		gtk_tree_store_append(GTK_TREE_STORE(snippets_v.store), &citer, NULL);
		snippets_fill_tree_item_from_node(&citer, ptr);
	}
}

static void update_name_in_tree(Tsnippetswin *snw, const gchar *name) {
	if (snw->lastclickedpath) {
		GtkTreeIter citer;
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(snippets_v.store),&citer,snw->lastclickedpath)) {
			gtk_tree_store_set(snippets_v.store, &citer, TITLE_COLUMN, name,-1);
		}
	}
}

typedef struct {
	GtkWidget *table;
	GtkWidget *entries[6];
	GtkWidget *matchtype;
	GtkWidget *scope;
	GtkWidget *casesens;
	GtkWidget *escapechars;
	GtkWidget *searchpat;
	GtkWidget *replace;
} TpageSnr;

static gpointer snippets_build_pageSnr(Tsnipwiz *snwiz, GtkWidget *dialog_action) {
	const gchar *scope[] = {
		N_("Entire document"),
		N_("Forward from cursor position"),
		N_("Selection"),
		N_("All open files"),
	};

	const gchar *matchPattern[] = {
		N_("Normal"),
		N_("PERL")
	};
	GtkWidget *label;
	gint i;
	TpageSnr *p = g_new(TpageSnr,1);

	p->table = gtk_table_new(8, 4, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE (p->table), 6);
	gtk_table_set_col_spacings(GTK_TABLE (p->table), 12);
	gtk_box_pack_start(GTK_BOX(dialog_action), p->table, TRUE, TRUE, 0);
	
	label = gtk_label_new(_("Specify a search and a replace pattern. You may use %0, %1, ...%5 placeholders to ask for values when you activate this item. Give these placeholders an appropriate name on the right. (Please use %% if you need literal % in your string!)"));
	/*gtk_label_set_use_markup(GTK_LABEL(label),TRUE);*/
#if GTK_CHECK_VERSION(3,2,0)
	gtk_label_set_width_chars(GTK_LABEL(label),50);
#endif
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	gtk_table_attach(GTK_TABLE(p->table),label, 0,4,0,1
					,GTK_FILL,GTK_FILL,0,0);
	
	p->searchpat = gtk_entry_new();
	dialog_mnemonic_label_in_table(_("_Search for: "), p->searchpat, p->table, 0, 1, 1, 2);
	gtk_table_attach(GTK_TABLE(p->table),p->searchpat, 1,4,1,2,GTK_FILL|GTK_EXPAND,GTK_FILL,0,0);
	
	p->replace = gtk_entry_new();
	dialog_mnemonic_label_in_table(_("Replace _with: "), p->replace, p->table, 0, 1, 2, 3);
	gtk_table_attach(GTK_TABLE(p->table),p->replace, 1,4,2,3,GTK_FILL|GTK_EXPAND,GTK_FILL,0,0);
	
	p->scope = gtk_combo_box_text_new();
	for (i = 0; i < G_N_ELEMENTS(scope); i++) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(p->scope), _(scope[i]));
	}
	dialog_mnemonic_label_in_table(_("Sco_pe: "), p->scope, p->table, 0, 1, 3, 4);
	gtk_table_attach(GTK_TABLE(p->table),p->scope, 1,2,3,4,GTK_FILL|GTK_EXPAND,GTK_SHRINK,0,0);
	
	p->matchtype = gtk_combo_box_text_new();
	for (i = 0; i < G_N_ELEMENTS(matchPattern); i++) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(p->matchtype), _(matchPattern[i]));
	}
	dialog_mnemonic_label_in_table(_("Match Patter_n: "), p->matchtype, p->table, 0, 1, 4, 5);
	gtk_table_attach(GTK_TABLE(p->table), p->matchtype, 1, 2, 4, 5, GTK_EXPAND|GTK_FILL,GTK_SHRINK, 0, 0);
	
	p->casesens = gtk_check_button_new_with_mnemonic(_("Case sensitive _matching"));
	gtk_widget_set_tooltip_text(p->casesens,_("Only match if case (upper/lower) is identical."));
	gtk_table_attach(GTK_TABLE(p->table), p->casesens, 0, 2, 5, 6, GTK_EXPAND|GTK_FILL,GTK_SHRINK, 0, 0);
	
	p->escapechars = gtk_check_button_new_with_mnemonic(_("_Use escape chars"));
	gtk_widget_set_tooltip_text(p->escapechars,_("Unescape backslash escaped characters such as \\n, \\t etc."));
	gtk_table_attach(GTK_TABLE(p->table), p->escapechars, 0, 2, 6, 7, GTK_EXPAND|GTK_FILL,GTK_SHRINK, 0, 0);
	
	for (i = 0; i <  6; i++) {
		gchar *tmpstr;
		tmpstr = g_strdup_printf("%%%d ", i);
		label = gtk_label_new(tmpstr);
		gtk_misc_set_alignment(GTK_MISC(label),1,0.5);
		gtk_table_attach(GTK_TABLE(p->table),label, 2,3,i+3,i+4
					,GTK_FILL,GTK_FILL,0,0);
		g_free(tmpstr);
		p->entries[i] = gtk_entry_new();
		gtk_table_attach(GTK_TABLE(p->table),p->entries[i], 3,4,i+3,i+4
					,GTK_FILL,GTK_FILL,0,0);
	}
	
	if (snwiz->node) {
		xmlNodePtr cur;
		xmlChar *tmpstr;
		xmlChar *region,*matchtype,*casesens,*escapechars;
		gint i=0;
		for (cur = snwiz->node->xmlChildrenNode;cur != NULL;cur = cur->next) {
			if (i < 6 && xmlStrEqual(cur->name, (const xmlChar *)"param")) {
				xmlChar *name;
				name = xmlGetProp(cur, (const xmlChar *)"name");
				gtk_entry_set_text(GTK_ENTRY(p->entries[i]), (gchar *)name);
				g_free(name);
				i++;
			} else if (xmlStrEqual(cur->name, (const xmlChar *)"searchpat")) {
				tmpstr = xmlNodeListGetString(snippets_v.doc, cur->xmlChildrenNode, 1);
				DEBUG_MSG("searchpattern: %s\n",tmpstr);
				gtk_entry_set_text(GTK_ENTRY(p->searchpat),(gchar *)tmpstr);
				g_free(tmpstr);
			} else if (xmlStrEqual(cur->name, (const xmlChar *)"replacepat")) {
				tmpstr = xmlNodeListGetString(snippets_v.doc, cur->xmlChildrenNode, 1);
				DEBUG_MSG("replacepattern: %s\n",tmpstr);
				gtk_entry_set_text(GTK_ENTRY(p->replace),(gchar *)tmpstr);
				g_free(tmpstr);
			}
		}
		region = xmlGetProp(snwiz->node, (const xmlChar *)"region");
		matchtype = xmlGetProp(snwiz->node, (const xmlChar *)"matchtype");
		casesens = xmlGetProp(snwiz->node, (const xmlChar *)"casesens");
		escapechars = xmlGetProp(snwiz->node, (const xmlChar *)"escapechars");
		gtk_combo_box_set_active(GTK_COMBO_BOX(p->scope),snippets_snr_region_from_char(region));
		gtk_combo_box_set_active(GTK_COMBO_BOX(p->matchtype),snippets_snr_matchtype_from_char(matchtype));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->casesens), (casesens && casesens[0] == '1'));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->escapechars), (escapechars && escapechars[0] == '1'));
	} else {
		gtk_combo_box_set_active(GTK_COMBO_BOX(p->scope),0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(p->matchtype),0);
	}
	
	gtk_widget_show_all(p->table);
	return p;
}
static void snippets_delete_pageSnr(Tsnipwiz *snwiz, gpointer data) {
	TpageSnr *p = (TpageSnr *)data;
	gtk_widget_destroy(p->table);
	g_free(p);
}

static gint snippets_test_pageSnr(Tsnipwiz *snwiz, gpointer data) {
	TpageSnr *p = (TpageSnr *)data;
	GtkTreePath *parentp;
	xmlNodePtr parentn, childn, tmpn;
	gchar *tmp="",*search,*replace;
	gint i;
	gint scope,casesens,matchtype,escapechars;

	scope = gtk_combo_box_get_active(GTK_COMBO_BOX(p->scope));
	matchtype = gtk_combo_box_get_active(GTK_COMBO_BOX(p->matchtype));
	casesens = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->casesens));
	escapechars = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->escapechars));
	
	if (snwiz->node) {
		xmlNodePtr cur;
		/* clean the old one */
		childn = snwiz->node;
		cur = childn->xmlChildrenNode;
		while (cur) {
			xmlUnlinkNode(cur);
			xmlFreeNode(cur);
			cur = childn->xmlChildrenNode;
		}
	} else {
		/* find the branch to add this leaf to */
		get_parentbranch(snwiz->snw, &parentp, &parentn);
		/* build a new one */
		childn = xmlNewChild(parentn,NULL,(const xmlChar *)"leaf",NULL);
		xmlSetProp(childn, (const xmlChar *)"type", (const xmlChar *)"snr");
	}
	
	/* build the leaf */
	xmlSetProp(childn, (const xmlChar *)"title", (const xmlChar *)snwiz->name);
	if (snwiz->description) {
		xmlSetProp(childn, (const xmlChar *)"tooltip", (const xmlChar *)snwiz->description);
	}
	
	switch (scope) {
		case 1:
			tmp = "c";
		break;
		case 2:
			tmp = "s";
		break;
		case 3:
			tmp = "a";
		break;
		case 0:
			tmp = "b";
		break;
	}
	xmlSetProp(childn, (const xmlChar *)"region", (const xmlChar *) tmp);
	switch (matchtype) {
		case 0:
			tmp = "normal";
		break;
		case 1:
			tmp = "perl";
		break;
	}
	xmlSetProp(childn, (const xmlChar *)"matchtype", (const xmlChar *) tmp);
	
	xmlSetProp(childn, (const xmlChar *)"casesens", casesens ? (const xmlChar *)"1": (const xmlChar *)"0");
	xmlSetProp(childn, (const xmlChar *)"escapechars", escapechars ? (const xmlChar *)"1": (const xmlChar *)"0");
	
	search = gtk_editable_get_chars(GTK_EDITABLE(p->searchpat),0,-1);
	replace = gtk_editable_get_chars(GTK_EDITABLE(p->replace),0,-1);
	xmlNewTextChild(childn,NULL,(const xmlChar *)"searchpat",(const xmlChar *)search);
	xmlNewTextChild(childn,NULL,(const xmlChar *)"replacepat",(const xmlChar *)replace);
	for (i = 0; i <  6; i++) {
		gchar *tmpstr;
		tmpstr = gtk_editable_get_chars(GTK_EDITABLE(p->entries[i]),0,-1);
		if (strlen(tmpstr)>0) {
			tmpn = xmlNewChild(childn,NULL,(const xmlChar *)"param", NULL);
			xmlSetProp(tmpn, (const xmlChar *)"name", (const xmlChar *)tmpstr);
		}
	}
	if (!snwiz->node) {
		/* now add this item to the treestore */
		add_item_to_tree(parentp, pixmap_type_snr, snwiz->name, childn);
	} else {
		update_name_in_tree(snwiz->snw, snwiz->name);
	}
	return page_finished;
}

typedef struct {
	GtkWidget *table;
	GtkWidget *entries[10];
	GtkWidget *is_file[10];
	GtkWidget *before_v;
	GtkWidget *after_v;
	GtkTextBuffer *before;
	GtkTextBuffer *after;
} TpageInsert;

static gpointer snippets_build_pageInsert(Tsnipwiz *snwiz, GtkWidget *dialog_action) {
	GtkWidget *scrolwin, *label;
	gint i;
	gchar *tmpstr;
	TpageInsert *p2 = g_new(TpageInsert,1);

	p2->table = gtk_table_new(13, 5, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE (p2->table), 6);
	gtk_table_set_col_spacings(GTK_TABLE (p2->table), 12);
	gtk_box_pack_start(GTK_BOX(dialog_action), p2->table, TRUE, TRUE, 0);
	
	label = gtk_label_new(_("The <i>before</i> text will be inserted before the cursor position or the current selection, the <i>after</i> text will be inserted after the cursor position or the current selection. You may use %0, %1, ...%9 placeholders to ask for values when you activate this item. Give these placeholders an appropriate name on the right. (Please use %% if you need literal % in your string!)"));
	gtk_label_set_use_markup(GTK_LABEL(label),TRUE);
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
#if GTK_CHECK_VERSION(3,2,0)
	gtk_label_set_width_chars(GTK_LABEL(label),50);
#endif
	gtk_table_attach(GTK_TABLE(p2->table),label, 0,5,0,1
					,GTK_FILL|GTK_EXPAND,GTK_FILL,0,0);
	
	label = gtk_label_new(_("<i>Before</i> text"));
	gtk_label_set_use_markup(GTK_LABEL(label),TRUE);
	gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
	gtk_table_attach(GTK_TABLE(p2->table),label, 0,1,1,2,GTK_FILL,GTK_FILL,0,0);
	scrolwin = textview_buffer_in_scrolwin(&p2->before_v, -1, -1, NULL, GTK_WRAP_NONE);
	gtk_table_attach(GTK_TABLE(p2->table), scrolwin, /*left*/0,/*right*/1,/*top*/2,/*bottom*/6
				,/*xoptions*/GTK_EXPAND|GTK_FILL,/*yoptions*/GTK_EXPAND|GTK_FILL
				,/*xpadding*/0,/*ypadding*/0);
	p2->before = gtk_text_view_get_buffer(GTK_TEXT_VIEW(p2->before_v));
	
	label = gtk_label_new(_("<i>After</i> text"));
	gtk_label_set_use_markup(GTK_LABEL(label),TRUE);
	gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
	gtk_table_attach(GTK_TABLE(p2->table),label, 0,1,6,7,GTK_FILL,GTK_FILL,0,0);
	scrolwin = textview_buffer_in_scrolwin(&p2->after_v, -1, -1, NULL, GTK_WRAP_NONE);
	gtk_table_attach(GTK_TABLE(p2->table), scrolwin, 0,1,7,11
				,GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,0,0);
	p2->after = gtk_text_view_get_buffer(GTK_TEXT_VIEW(p2->after_v));
	gtk_table_attach(GTK_TABLE(p2->table),gtk_label_new(_("Number")), 1,2,1,2
					,GTK_FILL,GTK_FILL,0,0);
	gtk_table_attach(GTK_TABLE(p2->table),gtk_label_new(_("Name")), 2,3,1,2
					,GTK_FILL,GTK_FILL,0,0);
	gtk_table_attach(GTK_TABLE(p2->table),gtk_label_new(_("Is file path")), 3,4,1,2
					,GTK_FILL,GTK_FILL,0,0);
	for (i = 0; i <  10; i++) {
		tmpstr = g_strdup_printf("%%%d ", i);
		label = gtk_label_new(tmpstr);
		gtk_misc_set_alignment(GTK_MISC(label),1,0.5);
		gtk_table_attach(GTK_TABLE(p2->table),label, 1,2,i+2,i+3
					,GTK_FILL,GTK_FILL,0,0);
		g_free(tmpstr);
		p2->entries[i] = gtk_entry_new();
		gtk_table_attach(GTK_TABLE(p2->table),p2->entries[i], 2,3,i+2,i+3
					,GTK_FILL,GTK_FILL,0,0);
		p2->is_file[i] = gtk_check_button_new();
		gtk_table_attach(GTK_TABLE(p2->table),p2->is_file[i], 3,4,i+2,i+3
					,GTK_SHRINK,GTK_SHRINK,0,0);
	}
	
	if (snwiz->node) {
		xmlNodePtr cur;
		xmlChar *tmpstr; 
		gint i=0;
		for (cur = snwiz->node->xmlChildrenNode;cur != NULL;cur = cur->next) {
			if (i<10 && xmlStrEqual(cur->name, (const xmlChar *)"param")) {
				xmlChar *name, *is_file;
				name = xmlGetProp(cur, (const xmlChar *)"name");
				is_file = xmlGetProp(cur, (const xmlChar *)"is_file");
				gtk_entry_set_text(GTK_ENTRY(p2->entries[i]), (gchar *)name);
				if (is_file&&is_file[0]=='1')
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p2->is_file[i]),1);
				g_free(name);
				g_free(is_file);
				i++;
			} else if (xmlStrEqual(cur->name, (const xmlChar *)"before")) {
				tmpstr = xmlNodeListGetString(snippets_v.doc, cur->xmlChildrenNode, 1);
				if (tmpstr) {
					gtk_text_buffer_set_text(p2->before,(gchar *)tmpstr,-1);
					g_free(tmpstr);
				}
			} else if (xmlStrEqual(cur->name, (const xmlChar *)"after")) {
				tmpstr = xmlNodeListGetString(snippets_v.doc, cur->xmlChildrenNode, 1);
				if (tmpstr) {
					gtk_text_buffer_set_text(p2->after,(gchar *)tmpstr,-1);
					g_free(tmpstr);
				}
			}
		}	
	}
	gtk_widget_show_all(p2->table);
	return p2;
}

static void snippets_delete_pageInsert(Tsnipwiz *snwiz, gpointer data) {
	TpageInsert *p2 = (TpageInsert *)data;
	gtk_widget_destroy(p2->table);
	g_free(p2);
}

static gint snippets_test_pageInsert(Tsnipwiz *snwiz, gpointer data) {
	TpageInsert *p2 = (TpageInsert *)data;
	GtkTreePath *parentp;
	xmlNodePtr parentn, childn, tmpn;
	gchar *before, *after;
	gint i;
	
	if (snwiz->node) {
		xmlNodePtr cur;
		/* clean the old one */
		childn = snwiz->node;
		cur = childn->xmlChildrenNode;
		while (cur) {
			xmlUnlinkNode(cur);
			xmlFreeNode(cur);
			cur = childn->xmlChildrenNode;
		}
	} else {
		/* find the branch to add this leaf to */
		get_parentbranch(snwiz->snw, &parentp, &parentn);
		/* build a new one */
		childn = xmlNewChild(parentn,NULL,(const xmlChar *)"leaf",NULL);
		xmlSetProp(childn, (const xmlChar *)"type", (const xmlChar *)"insert");
	}
	
	/* build the leaf */
	before = textbuffer_get_all_chars(p2->before);	
	after = textbuffer_get_all_chars(p2->after);		
	xmlSetProp(childn, (const xmlChar *)"title", (const xmlChar *)snwiz->name);
	if (snwiz->description) {
		xmlSetProp(childn, (const xmlChar *)"tooltip", (const xmlChar *)snwiz->description);
	}
	
	xmlNewTextChild(childn,NULL,(const xmlChar *)"before",(const xmlChar *)before);
	xmlNewTextChild(childn,NULL,(const xmlChar *)"after",(const xmlChar *)after);
	for (i = 0; i <  10; i++) {
		gchar *tmpstr;
		tmpstr = gtk_editable_get_chars(GTK_EDITABLE(p2->entries[i]),0,-1);
		if (strlen(tmpstr)>0) {
			tmpn = xmlNewChild(childn,NULL,(const xmlChar *)"param", NULL);
			xmlSetProp(tmpn, (const xmlChar *)"name", (const xmlChar *)tmpstr);
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->is_file[i]))) {
				xmlSetProp(tmpn, (const xmlChar *)"is_file", (const xmlChar *)"1");
			}
		}
	}
	if (!snwiz->node) {
		/* now add this item to the treestore */
		add_item_to_tree(parentp, pixmap_type_insert, snwiz->name, childn);
	} else {
		update_name_in_tree(snwiz->snw, snwiz->name);
	}
	return page_finished;
}

typedef struct {
	GtkWidget *entry;
	GtkWidget *vbox;
} TpageBranch;

static gpointer snippets_build_pageBranch(Tsnipwiz *snwiz, GtkWidget *dialog_action) {
	GtkWidget *label;
	gchar *namestr=NULL;
	TpageBranch *p1 = g_new(TpageBranch,1);
	if (snwiz->node) {	/* we're editing a node! */
		namestr = (gchar *)xmlGetProp(snwiz->node, (const xmlChar *)"title");
	}
	p1->vbox = gtk_vbox_new(TRUE,12);
	gtk_container_add(GTK_CONTAINER(dialog_action),p1->vbox);
	label = gtk_label_new(_("Enter the name of the branch:"));
	gtk_box_pack_start(GTK_BOX(p1->vbox),label,TRUE,TRUE,12);
	p1->entry = gtk_entry_new();
	if (namestr) {
		gtk_entry_set_text((GtkEntry *)p1->entry,namestr);
	}
	gtk_box_pack_start(GTK_BOX(p1->vbox),p1->entry,TRUE,TRUE,12);
	gtk_widget_show_all(p1->vbox);
	return p1;
}

static void snippets_delete_pageBranch(Tsnipwiz *snwiz, gpointer data) {
	TpageBranch *p1 = (TpageBranch *)data;
	gtk_widget_destroy(p1->vbox);
}

static gint snippets_test_pageBranch(Tsnipwiz *snwiz, gpointer data) {
	TpageBranch *p1 = (TpageBranch *)data;
	gchar *name;
	GtkTreePath *parentp=NULL;
	xmlNodePtr parent=NULL, child;
	GtkTreeIter piter, citer;	
	
	name = gtk_editable_get_chars(GTK_EDITABLE(p1->entry),0,-1);
	
	if (snwiz->node) {
		xmlSetProp(snwiz->node, (const xmlChar *)"title", (const xmlChar *)name);
		update_name_in_tree(snwiz->snw, name);
	} else {
		/* lets build the branch! */
		if (snwiz->snw->lastclickedpath) {
			parentp = gtk_tree_path_copy(snwiz->snw->lastclickedpath);
		}
		if (snwiz->snw->lastclickednode) {
			if (xmlStrEqual(snwiz->snw->lastclickednode->name, (const xmlChar *)"leaf")) {
				DEBUG_MSG("clicked node was a leaf\n");
				parent = snwiz->snw->lastclickednode->parent;
				if (parentp) {
					if (!gtk_tree_path_up(parentp)) {
						DEBUG_MSG("could not go up, set parentp NULL\n");
						gtk_tree_path_free(parentp);
						parentp = NULL;
					}
				}
			} else {
				parent = snwiz->snw->lastclickednode;
				DEBUG_MSG("clicked node was a branch\n");
			}
		} else { /* parent must be the root element <snippets> */
			parent = xmlDocGetRootElement(snippets_v.doc);
		}
		DEBUG_MSG("snippets_test_pageBranch, xml-adding %s to parent %s\n",name,(gchar *)parent->name); 
		child = xmlNewChild(parent,NULL,(const xmlChar *)"branch",NULL);	
		xmlSetProp(child, (const xmlChar *)"title", (const xmlChar *)name);
		/* add this branch to the treestore */
		DEBUG_MSG("snippets_test_pageBranch, store-adding %s to parentp=%p\n",name,parentp);
		if (parentp) {
			if (gtk_tree_model_get_iter(GTK_TREE_MODEL(snippets_v.store),&piter,parentp)) {
				gtk_tree_store_append(snippets_v.store, &citer, &piter);
			} else {
				g_print("hmm weird error!?!\n");
			}
		} else {
			gtk_tree_store_append(snippets_v.store, &citer, NULL);
		}
		gtk_tree_store_set(snippets_v.store, &citer, PIXMAP_COLUMN,NULL,TITLE_COLUMN, name,NODE_COLUMN, child,-1);
	}
	
	DEBUG_MSG("add branch with title %s\n",name);
	g_free(name);
	g_idle_add(snippets_store_lcb, NULL);
	return page_finished;
}

typedef struct {
	GtkWidget *name;
	GtkWidget *description;
	GtkWidget *vbox;
} TpageName;

static gpointer snippets_build_pageName(Tsnipwiz *snwiz, GtkWidget *dialog_action) {
	GtkWidget *label, *scrolwin;
	gchar *namestr=NULL, *descstr=NULL;
	TpageName *p = g_new(TpageName,1);

	if (snwiz->node) {	/* we're editing a node! */
		namestr = (gchar *)xmlGetProp(snwiz->node, (const xmlChar *)"title");
		descstr = (gchar *)xmlGetProp(snwiz->node, (const xmlChar *)"tooltip");
	}

	p->vbox = gtk_vbox_new(FALSE,12);
	gtk_container_add(GTK_CONTAINER(dialog_action),p->vbox);
	label = gtk_label_new(_("Name of the new item:"));
	gtk_box_pack_start(GTK_BOX(p->vbox),label,TRUE,FALSE,12);
	p->name = gtk_entry_new();
	if (namestr) gtk_entry_set_text(GTK_ENTRY(p->name),namestr);
	gtk_box_pack_start(GTK_BOX(p->vbox),p->name,TRUE,FALSE,12);
	label = gtk_label_new(_("Description:"));
	gtk_box_pack_start(GTK_BOX(p->vbox),label,TRUE,FALSE,12);
	scrolwin = textview_buffer_in_scrolwin(&p->description, -1, -1, descstr, GTK_WRAP_NONE);
	gtk_box_pack_start(GTK_BOX(p->vbox),scrolwin,TRUE,TRUE,12);
	
	gtk_widget_show_all(p->vbox);
	g_free(namestr);
	g_free(descstr);
	return p;
}

static void snippets_delete_pageName(Tsnipwiz *snwiz, gpointer data) {
	TpageName *p = (TpageName *)data;
	gtk_widget_destroy(p->vbox);
	g_free(p);
}

static gint snippets_test_pageName(Tsnipwiz *snwiz, gpointer data) {
	TpageName *p = (TpageName *)data;
	gchar *name, *desc;

	name = gtk_editable_get_chars(GTK_EDITABLE(p->name),0,-1);
	desc = textbuffer_get_all_chars(gtk_text_view_get_buffer(GTK_TEXT_VIEW(p->description)));
	if (strlen(name)) {
		snwiz->name = name;
		snwiz->description = desc;
		if (snwiz->choice == 1) {
			return page_insert;
		} else {
			return page_snr;
		}
	} else {
		/* warn */
	
		g_free(name);
		g_free(desc);
		return page_name;
	}
}

typedef struct {
	GtkWidget *radio[3];
	GtkWidget *vbox;
} TpageType;

static gpointer snippets_build_pageType(Tsnipwiz *snwiz, GtkWidget *dialog_action) {
	GtkWidget *label;
	TpageType *p0 = g_new(TpageType,1);
	p0->vbox = gtk_vbox_new(FALSE,12);
	gtk_container_add(GTK_CONTAINER(dialog_action),p0->vbox);
	label = gtk_label_new(_("Select what you want to add"));
	gtk_box_pack_start(GTK_BOX(p0->vbox),label,TRUE,TRUE,12);
	p0->radio[0] = gtk_radio_button_new_with_label(NULL, _("Branch"));
	gtk_box_pack_start(GTK_BOX(p0->vbox),p0->radio[0],TRUE,TRUE,12);
	p0->radio[1] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(p0->radio[0]),_("Insert string"));
	gtk_box_pack_start(GTK_BOX(p0->vbox),p0->radio[1],TRUE,TRUE,12);
	p0->radio[2] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(p0->radio[0]),_("Search and replace pattern"));
	gtk_box_pack_start(GTK_BOX(p0->vbox),p0->radio[2],TRUE,TRUE,12);
	gtk_widget_show_all(p0->vbox);
	DEBUG_MSG("snippets_build_pageType, created pageType at %p\n",p0);
	return p0;
}

static void snippets_delete_pageType(Tsnipwiz *snwiz, gpointer data) {
	TpageType *p0 = (TpageType *)data;
	DEBUG_MSG("snippets_delete_pageType, destroy pageType at %p\n",p0);
	gtk_widget_destroy(p0->vbox);
	g_free(p0);
}

static gint snippets_test_pageType(Tsnipwiz *snwiz, gpointer data) {
	TpageType *p0 = (TpageType *)data;
	int i;
	DEBUG_MSG("snippets_delete_pageType, test pageType at %p\n",p0);
	for (i=0;i<3;i++) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p0->radio[i]))) {
			snwiz->choice = i;
			if (i==0) return page_branch;
			else return page_name;
		}
	}
	return page_type;
}

static void snipwiz_dialog_response_lcb(GtkDialog *dialog, gint response, Tsnipwiz *snwiz) {
	Tpagenum newpagenum=page_type;
	DEBUG_MSG("snipwiz_dialog_response_lcb, response=%d\n",response);
	if (response == GTK_RESPONSE_REJECT) {
		gtk_widget_destroy(snwiz->dialog);
		g_free(snwiz);
		return;
	}
	switch (snwiz->pagenum) { /* test the current results */
		case page_type:
			newpagenum = snippets_test_pageType(snwiz,snwiz->pagestruct);
			if (newpagenum != page_type) {
				snippets_delete_pageType(snwiz,snwiz->pagestruct);
			}
		break;
		case page_name:
			newpagenum = snippets_test_pageName(snwiz,snwiz->pagestruct);
			if (newpagenum != page_name) {
				snippets_delete_pageName(snwiz,snwiz->pagestruct);
			}
		break;
		case page_branch:
			newpagenum = snippets_test_pageBranch(snwiz,snwiz->pagestruct);
			if (newpagenum != page_branch) {
				snippets_delete_pageBranch(snwiz,snwiz->pagestruct);
			}
		break;
		case page_insert:
			newpagenum = snippets_test_pageInsert(snwiz,snwiz->pagestruct);
			if (newpagenum != page_insert) {
				snippets_delete_pageInsert(snwiz,snwiz->pagestruct);
			}
		break;
		case page_snr:
			newpagenum = snippets_test_pageSnr(snwiz,snwiz->pagestruct);
			if (newpagenum != page_snr) {
				snippets_delete_pageSnr(snwiz,snwiz->pagestruct);
			}
		break;
		case page_finished: /* avoid compiler warning */
		break;
	}
	if (snwiz->pagenum != newpagenum) {
		GtkWidget *vbox = gtk_dialog_get_content_area(GTK_DIALOG(snwiz->dialog));
		switch (newpagenum) { /* build a new page */
			case page_type:
				snwiz->pagestruct = snippets_build_pageType(snwiz, vbox);
			break;
			case page_name:
				snwiz->pagestruct = snippets_build_pageName(snwiz, vbox);
			break;
			case page_branch:
				snwiz->pagestruct = snippets_build_pageBranch(snwiz, vbox);
			break;
			case page_insert:
				snwiz->pagestruct = snippets_build_pageInsert(snwiz, vbox);
			break;
			case page_snr:
				snwiz->pagestruct = snippets_build_pageSnr(snwiz, vbox);
			break;
			case page_finished:
				gtk_widget_destroy(snwiz->dialog);
				if (snwiz->name) g_free(snwiz->name);
				if (snwiz->description) g_free(snwiz->description);
				g_free(snwiz);
				g_idle_add(snippets_store_lcb, NULL);
				return;
			break;
		}
		snwiz->pagenum = newpagenum;
	}
}


void snippets_new_item_dialog(Tsnippetswin *snw, xmlNodePtr node) {
	Tsnipwiz *snwiz;
	GtkWidget *vbox;
	
	snwiz = g_new0(Tsnipwiz,1);
	snwiz->snw = snw;
	snwiz->node = node;
	snwiz->dialog = gtk_dialog_new_with_buttons(node? _("Edit snippet") :_("New snippet"),GTK_WINDOW(snw->bfwin->main_window),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
					GTK_STOCK_GO_FORWARD,1,
					NULL);
	gtk_window_set_default_size(GTK_WINDOW(snwiz->dialog),500,400);
	g_signal_connect(G_OBJECT(snwiz->dialog), "response", G_CALLBACK(snipwiz_dialog_response_lcb), snwiz);
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(snwiz->dialog));
	if (node) {
		if (xmlStrEqual(snw->lastclickednode->name, (const xmlChar *)"leaf")) {
			xmlChar *type = xmlGetProp(node, (const xmlChar *)"type");
			if (xmlStrEqual(type, (const xmlChar *)"insert")) {
				snwiz->choice = 1;
			} else if (xmlStrEqual(type, (const xmlChar *)"snr")) {
				snwiz->choice = 2;
			} 
			snwiz->pagestruct = snippets_build_pageName(snwiz, vbox);
			snwiz->pagenum = page_name;
		} else {
			snwiz->pagestruct = snippets_build_pageBranch(snwiz, vbox);
			snwiz->pagenum = page_branch;
		}
	} else {
		if (!snw->lastclickednode) {
			snwiz->pagestruct = snippets_build_pageBranch(snwiz, vbox);
			snwiz->pagenum = page_branch;
		} else {
			snwiz->pagestruct = snippets_build_pageType(snwiz, vbox);
			snwiz->pagenum = page_type;
		}
	}
	gtk_widget_show_all(snwiz->dialog);	
}
