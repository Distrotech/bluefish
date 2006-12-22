/* Bluefish HTML Editor
 * snippets_wizard.c - plugin for snippets sidebar
 *
 * Copyright (C) 2006 Olivier Sessink
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
#define DEBUG

#include <string.h>

#include "snippets.h"
#include "snippets_load.h"
#include "snippets_leaf_insert.h"
#include "../gtk_easy.h"

typedef enum {
	page_type, /* 0 */
	page_name,
	page_branch,
	page_insert,
	page_replace,
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

static void add_item_to_tree(GtkTreePath *parentp, gchar *name, gpointer ptr) {
	GtkTreeIter citer;
	if (parentp) {
		GtkTreeIter piter;
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(snippets_v.store),&piter,parentp)) {
			gtk_tree_store_append(snippets_v.store, &citer, &piter);
		} else {
			g_print("hmm weird error!?!\n");
		}
	} else {
		gtk_tree_store_append(snippets_v.store, &citer, NULL);
	}
	gtk_tree_store_set(snippets_v.store, &citer, PIXMAP_COLUMN, NULL, TITLE_COLUMN, name,NODE_COLUMN, ptr,-1);
}

typedef struct {
	GtkWidget *table;
	GtkWidget *entries[10];
	GtkWidget *before_v;
	GtkWidget *after_v;
	GtkTextBuffer *before;
	GtkTextBuffer *after;

} TpageInsert;

static gpointer snippets_build_pageInsert(Tsnipwiz *snwiz, GtkWidget *dialog_action) {
	GtkWidget *scrolwin, *label;
	gint i;
	gchar *tmpstr;
	gchar *beforeval=NULL,*afterval=NULL;
	TpageInsert *p2 = g_new(TpageInsert,1);

	p2->table = gtk_table_new(12, 4, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE (p2->table), 6);
	gtk_table_set_col_spacings(GTK_TABLE (p2->table), 12);
	gtk_box_pack_start(GTK_BOX(dialog_action), p2->table, TRUE, TRUE, 0);
	
	label = gtk_label_new(_("The <i>before</i> text will be inserted before the cursor position or the current selection, the <i>after</i> text will be inserted after the cursor position or the current selection. You may use %0, %1, ...%9 placeholders to ask for values when you activate this item. Give these placeholders an appropriate name on the right."));
	gtk_label_set_use_markup(GTK_LABEL(label),TRUE);
	gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	gtk_table_attach(GTK_TABLE(p2->table),label, 0,3,0,1
					,GTK_FILL,GTK_FILL,0,0);
	
	label = gtk_label_new(_("<i>Before</i> text"));
	gtk_label_set_use_markup(GTK_LABEL(label),TRUE);
	gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
	gtk_table_attach(GTK_TABLE(p2->table),label, 0,1,1,2,GTK_FILL,GTK_FILL,0,0);
	scrolwin = textview_buffer_in_scrolwin(&p2->before_v, -1, -1, NULL, GTK_WRAP_NONE);
	gtk_table_attach(GTK_TABLE(p2->table), scrolwin, /*left*/0,/*right*/1,/*top*/2,/*bottom*/4
				,/*xoptions*/GTK_EXPAND|GTK_FILL,/*yoptions*/GTK_EXPAND|GTK_FILL
				,/*xpadding*/0,/*ypadding*/0);
	p2->before = gtk_text_view_get_buffer(GTK_TEXT_VIEW(p2->before_v));
	
	label = gtk_label_new(_("<i>After</i> text"));
	gtk_label_set_use_markup(GTK_LABEL(label),TRUE);
	gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
	gtk_table_attach(GTK_TABLE(p2->table),label, 0,1,5,6,GTK_FILL,GTK_FILL,0,0);
	scrolwin = textview_buffer_in_scrolwin(&p2->after_v, -1, -1, NULL, GTK_WRAP_NONE);
	gtk_table_attach(GTK_TABLE(p2->table), scrolwin, 0,1,7,11
				,GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,0,0);
	p2->after = gtk_text_view_get_buffer(GTK_TEXT_VIEW(p2->after_v));
	
	for (i = 0; i <  10; i++) {
		tmpstr = g_strdup_printf("%%%d ", i);
		label = gtk_label_new(tmpstr);
		gtk_misc_set_alignment(GTK_MISC(label),1,0.5);
		gtk_table_attach(GTK_TABLE(p2->table),label, 1,2,i+1,i+2
					,GTK_FILL,GTK_FILL,0,0);
		g_free(tmpstr);
		p2->entries[i] = gtk_entry_new();
		gtk_table_attach(GTK_TABLE(p2->table),p2->entries[i], 2,3,i+1,i+2
					,GTK_FILL,GTK_FILL,0,0);
	}
	
	if (snwiz->node) {
		xmlNodePtr cur;
		xmlChar *tmpstr; 
		gint i=0;
		for (cur = snwiz->node->xmlChildrenNode;cur != NULL;cur = cur->next) {
			if (xmlStrEqual(cur->name, (const xmlChar *)"param")) {
				xmlChar *name;
				name = xmlGetProp(cur, (const xmlChar *)"name");
				gtk_entry_set_text(p2->entries[i], (gchar *)name);
				g_free(name);
				i++;
			} else if (xmlStrEqual(cur->name, (const xmlChar *)"before")) {
				tmpstr = xmlNodeListGetString(snippets_v.doc, cur->xmlChildrenNode, 1);
				gtk_text_buffer_set_text(p2->before,(gchar *)tmpstr,-1);
				g_free(tmpstr);
			} else if (xmlStrEqual(cur->name, (const xmlChar *)"after")) {
				tmpstr = xmlNodeListGetString(snippets_v.doc, cur->xmlChildrenNode, 1);
				gtk_text_buffer_set_text(p2->after,(gchar *)tmpstr,-1);
				g_free(tmpstr);
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
	
	/* find the branch to add this leaf to */
	get_parentbranch(snwiz->snw, &parentp, &parentn);
	
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
		/* build a new one */
		childn = xmlNewChild(parentn,NULL,(const xmlChar *)"leaf",NULL);
		xmlSetProp(childn, (const xmlChar *)"type", (const xmlChar *)"insert");
	}
	
	/* build the leaf */
	before = textbuffer_get_all_chars(p2->before);	
	after = textbuffer_get_all_chars(p2->after);		
	xmlSetProp(childn, (const xmlChar *)"title", (const xmlChar *)snwiz->name);
	if (strlen(snwiz->description)) {
		xmlSetProp(childn, (const xmlChar *)"tooltip", (const xmlChar *)snwiz->description);
	}
	
	tmpn = xmlNewChild(childn,NULL,(const xmlChar *)"before",(const xmlChar *)before);
	tmpn = xmlNewChild(childn,NULL,(const xmlChar *)"after",(const xmlChar *)after);
	for (i = 0; i <  10; i++) {
		gchar *tmpstr;
		tmpstr = gtk_editable_get_chars(GTK_EDITABLE(p2->entries[i]),0,-1);
		if (strlen(tmpstr)>0) {
			tmpn = xmlNewChild(childn,NULL,(const xmlChar *)"param", NULL);
			xmlSetProp(tmpn, (const xmlChar *)"name", (const xmlChar *)tmpstr);
		}
	}
	if (!snwiz->node) {
		/* now add this item to the treestore */
		add_item_to_tree(parentp, snwiz->name, childn);
	} else {
		/* TODO: update the name in the treestore */
	
	}
	return page_finished;
}

typedef struct {
	GtkWidget *entry;
	GtkWidget *vbox;
} TpageBranch;

static gpointer snippets_build_pageBranch(Tsnipwiz *snwiz, GtkWidget *dialog_action) {
	GtkWidget *label;
	TpageBranch *p1 = g_new(TpageBranch,1);
	p1->vbox = gtk_vbox_new(TRUE,12);
	gtk_container_add(GTK_CONTAINER(dialog_action),p1->vbox);
	label = gtk_label_new(_("Enter the name of the branch:"));
	gtk_box_pack_start(GTK_BOX(p1->vbox),label,TRUE,TRUE,12);
	p1->entry = gtk_entry_new();
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
	/* lets build the branch! */
	name = gtk_editable_get_chars(GTK_EDITABLE(p1->entry),0,-1);
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
	
	DEBUG_MSG("add branch with title %s\n",name);
	g_free(name);
	snippets_store();
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
			return page_replace;
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
	for (i=0;i<2;i++) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p0->radio[i]))) {
			snwiz->choice = i;
			if (i==0) return page_branch;
			else return page_name;
		}
	}
	return page_type;
}

static void snipwiz_dialog_response_lcb(GtkDialog *dialog, gint response, Tsnipwiz *snwiz) {
	Tpagenum newpagenum;
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
		case page_replace:
			/* TODO */
		break;
		case page_finished: /* avoid compiler warning */
		break;
	}
	if (snwiz->pagenum != newpagenum) {
		switch (newpagenum) { /* build a new page */
			case page_type:
				snwiz->pagestruct = snippets_build_pageType(snwiz,GTK_DIALOG(snwiz->dialog)->vbox);
			break;
			case page_name:
				snwiz->pagestruct = snippets_build_pageName(snwiz,GTK_DIALOG(snwiz->dialog)->vbox);
			break;
			case page_branch:
				snwiz->pagestruct = snippets_build_pageBranch(snwiz,GTK_DIALOG(snwiz->dialog)->vbox);
			break;
			case page_insert:
				snwiz->pagestruct = snippets_build_pageInsert(snwiz,GTK_DIALOG(snwiz->dialog)->vbox);
			break;
			case page_replace:
				/* TODO */
			break;
			case page_finished:
				gtk_widget_destroy(snwiz->dialog);
				if (snwiz->name) g_free(snwiz->name);
				if (snwiz->description) g_free(snwiz->description);
				g_free(snwiz);
				return;
			break;
		}
		snwiz->pagenum = newpagenum;
	}
}


void snippets_new_item_dialog(Tsnippetswin *snw, xmlNodePtr node) {
	Tsnipwiz *snwiz;
	
	snwiz = g_new0(Tsnipwiz,1);
	snwiz->snw = snw;
	snwiz->node = node;
	snwiz->dialog = gtk_dialog_new_with_buttons(_("New snippet"),GTK_WINDOW(snw->bfwin->main_window),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
					GTK_STOCK_GO_FORWARD,1,
					NULL);
	gtk_window_set_default_size(GTK_WINDOW(snwiz->dialog),500,400);
	g_signal_connect(G_OBJECT(snwiz->dialog), "response", G_CALLBACK(snipwiz_dialog_response_lcb), snwiz);
	if (node) {
		xmlChar *type = xmlGetProp(node, (const xmlChar *)"type");
		if (xmlStrEqual(type, (const xmlChar *)"insert")) {
			snwiz->choice = 1;
		} else if (xmlStrEqual(type, (const xmlChar *)"snr")) {
			snwiz->choice = 2;
		} 
		snwiz->pagestruct = snippets_build_pageName(snwiz,GTK_DIALOG(snwiz->dialog)->vbox);
		snwiz->pagenum = page_name;
	} else {
		snwiz->pagestruct = snippets_build_pageType(snw,GTK_DIALOG(snwiz->dialog)->vbox);
		snwiz->pagenum = page_type;
	}
	gtk_widget_show_all(snwiz->dialog);	
}
