/* Bluefish HTML Editor
 * snippets_gui.c - plugin for snippets sidebar
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
#include "snippets_gui.h"
#include "snippets_load.h"
#include "snippets_leaf_insert.h"
#include "../document.h"
#include "../gtk_easy.h"

static xmlNodePtr snippetview_get_node_at_path(GtkTreePath *path) {
	if (path) {
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(snippets_v.store),&iter,path)) {
			xmlNodePtr cur=NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(snippets_v.store), &iter, 1, &cur, -1);
			return cur;
		}
	}
	return NULL;
}

typedef enum {
	page_type,
	page_branch,
	page_insert,
	page_finished
} Tpagenum;

typedef struct {
	GtkWidget *entry;
	GtkWidget *vbox;
} Tpage1;

static gpointer snippets_build_page1(Tsnippetswin *snw, GtkWidget *dialog_action) {
	GtkWidget *label;
	Tpage1 *p1 = g_new(Tpage1,1);
	p1->vbox = gtk_vbox_new(TRUE,5);
	gtk_container_add(GTK_CONTAINER(dialog_action),p1->vbox);
	label = gtk_label_new(_("Enter the name of the branch:"));
	gtk_box_pack_start(GTK_BOX(p1->vbox),label,TRUE,TRUE,5);
	p1->entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(p1->vbox),p1->entry,TRUE,TRUE,5);
	
	gtk_widget_show_all(p1->vbox);
	return p1;
}

static void snippets_delete_page1(Tsnippetswin *snw, gpointer data) {
	Tpage1 *p1 = (Tpage1 *)data;
	gtk_widget_destroy(p1->vbox);
}

static gint snippets_test_page1(Tsnippetswin *snw, gpointer data) {
	Tpage1 *p1 = (Tpage1 *)data;
	gchar *name;
	GtkTreePath *parentp=NULL;
	xmlNodePtr parent=NULL, child;
	GtkTreeIter piter, citer;	
	/* lets build the branch! */
	name = gtk_editable_get_chars(GTK_EDITABLE(p1->entry),0,-1);
	if (snw->lastclickedpath) {
		parentp = gtk_tree_path_copy(snw->lastclickedpath);
	}
	if (snw->lastclickednode) {
		if (xmlStrEqual(snw->lastclickednode->name, (const xmlChar *)"leaf")) {
			DEBUG_MSG("clicked node was a leaf\n");
			parent = snw->lastclickednode->parent;
			if (parentp) {
				if (!gtk_tree_path_up(parentp)) {
					DEBUG_MSG("could not go up, set parentp NULL\n");
					gtk_tree_path_free(parentp);
					parentp = NULL;
				}
			}
		} else {
			parent = snw->lastclickednode;
			DEBUG_MSG("clicked node was a branch\n");
		}
	} 
	child = xmlNewChild(parent,NULL,(const xmlChar *)"branch",NULL);
	xmlSetProp(child, (const xmlChar *)"title", (const xmlChar *)name);
	/* add this branch to the treestore */

	if ((parentp && gtk_tree_model_get_iter(GTK_TREE_MODEL(snippets_v.store),&piter,parentp)) 
						|| gtk_tree_model_get_iter_first(GTK_TREE_MODEL(snippets_v.store),&piter)) {
		gtk_tree_store_append(snippets_v.store, &citer, &piter);
		gtk_tree_store_set(snippets_v.store, &citer, 0, name,1, child,-1);
	}
	DEBUG_MSG("add branch with title %s\n",name);
	g_free(name);
	snippets_store();
	return page_finished;
}


typedef struct {
	GtkWidget *radio[2];
	GtkWidget *vbox;
} Tpage0;

static gpointer snippets_build_page0(Tsnippetswin *snw, GtkWidget *dialog_action) {
	GtkWidget *label;
	Tpage0 *p0 = g_new(Tpage0,1);
	p0->vbox = gtk_vbox_new(TRUE,5);
	gtk_container_add(GTK_CONTAINER(dialog_action),p0->vbox);
	label = gtk_label_new(_("Select what you want to add"));
	gtk_box_pack_start(GTK_BOX(p0->vbox),label,TRUE,TRUE,5);
	p0->radio[0] = gtk_radio_button_new_with_label(NULL, _("Branch"));
	gtk_box_pack_start(GTK_BOX(p0->vbox),p0->radio[0],TRUE,TRUE,5);
	p0->radio[1] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(p0->radio[0]),_("Insert string"));
	gtk_box_pack_start(GTK_BOX(p0->vbox),p0->radio[1],TRUE,TRUE,5);
	gtk_widget_show_all(p0->vbox);
	return p0;
}

static void snippets_delete_page0(Tsnippetswin *snw, gpointer data) {
	Tpage0 *p0 = (Tpage0 *)data;
	gtk_widget_destroy(p0->vbox);
}

static gint snippets_test_page0(Tsnippetswin *snw, gpointer data) {
	Tpage0 *p0 = (Tpage0 *)data;
	int i;
	for (i=0;i<2;i++) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p0->radio[i]))) {
			return i+1; /* branch =1, insert=2 */
		}
	}
	return page_type;
}

static void snippets_new_item_dialog(Tsnippetswin *snw) {
	GtkWidget* dialog;
	gboolean cont=TRUE;
	gint response;
	gpointer pagestruct;
	Tpagenum pagenum = page_type, newpagenum = page_type; 
	
	dialog = gtk_dialog_new_with_buttons(_("New snippet"),GTK_WINDOW(snw->bfwin->main_window),
					GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
					GTK_STOCK_GO_FORWARD,1,
					NULL);
	pagestruct = snippets_build_page0(snw,GTK_DIALOG(dialog)->vbox);
	gtk_widget_show_all(dialog);
	while (cont) {
		response = gtk_dialog_run(dialog);
		if (response == GTK_RESPONSE_REJECT) {
			cont = FALSE;
			break;
		} 
		switch (pagenum) { /* test the current results */
			case page_type:
				newpagenum = snippets_test_page0(snw,pagestruct);
				if (newpagenum != page_type) {
					snippets_delete_page0(snw,pagestruct);
				}
			break;
			case page_branch:
				newpagenum = snippets_test_page1(snw,pagestruct);
				if (newpagenum != page_type) {
					snippets_delete_page1(snw,pagestruct);
				}
			break;
			case page_insert:
			break;
		}
		if (pagenum != newpagenum) {
			switch (newpagenum) { /* build a new page */
				case page_type:
					pagestruct = snippets_build_page0(snw,GTK_DIALOG(dialog)->vbox);
				break;
				case page_branch:
					pagestruct = snippets_build_page1(snw,GTK_DIALOG(dialog)->vbox);
				break;
				case page_insert:
				break;
				case page_finished:
					cont = FALSE;
				break;
			}
			pagenum = newpagenum;
		}
	}
	gtk_widget_destroy(dialog);
}

static void snippet_activate_leaf(Tsnippetswin *snw, xmlNodePtr cur) {
	xmlChar *type = xmlGetProp(cur, (const xmlChar *)"type");
	if (!type) {
		DEBUG_MSG("snippet_activate_leaf, no type\n");
		return;
	}
	if (xmlStrEqual(type, (const xmlChar *)"insert")) {
		snippets_activate_leaf_insert(snw, cur);
	} else {
		DEBUG_MSG("snippet_activate_leaf, unknown type %s\n",(const gchar *)type);
	}
	xmlFree(type);
}

typedef struct {
	Tsnippetswin *snw;
	xmlNodePtr cur;
} Taccelerator_cbdata;

gboolean snippets_accelerator_activated_lcb(GtkAccelGroup *accel_group,GObject *acceleratable,guint keyval,GdkModifierType modifier,gpointer data) {
	Taccelerator_cbdata *hcbdata = (Taccelerator_cbdata *)data;
	snippet_activate_leaf(hcbdata->snw, hcbdata->cur);
	return TRUE;
}

static void snippets_connect_accelerators_from_doc(Tsnippetswin *snw, xmlNodePtr cur, GtkAccelGroup *accel_group) {
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((xmlStrEqual(cur->name, (const xmlChar *)"branch"))) {
			snippets_connect_accelerators_from_doc(snw, cur, accel_group);
		} else if ((xmlStrEqual(cur->name, (const xmlChar *)"leaf"))) {
			xmlChar *accelerator;
			/* if there are shortcut keys defined for this leaf we should register them */
			accelerator = xmlGetProp(cur, (const xmlChar *)"accelerator");
			if (accelerator) {
				guint key;	
				GdkModifierType mod;
				gtk_accelerator_parse((const gchar *)accelerator,&key,&mod);
				if (key!=0 && mod!=0 && gtk_accelerator_valid(key,mod)) {
					GClosure *closure;
					Taccelerator_cbdata *hcbdata;
					hcbdata = g_new(Taccelerator_cbdata,1);
					hcbdata->snw = snw;
					hcbdata->cur = cur;
					DEBUG_MSG("snippets_connect_accelerators_from_doc, connecting accelerator %s\n",accelerator);
					closure = g_cclosure_new(G_CALLBACK(snippets_accelerator_activated_lcb),hcbdata,g_free);
					gtk_accel_group_connect(accel_group,key,mod,GTK_ACCEL_VISIBLE, closure);
				}
				xmlFree(accelerator);
			}
		}
		cur = cur->next;
	}
}
static void snippets_window_rebuild_accelerators_lcb(gpointer key,gpointer value,gpointer user_data) {
	Tsnippetswin *snw = (Tsnippetswin *)value;
	gtk_window_remove_accel_group(GTK_WINDOW(snw->bfwin->main_window),snw->accel_group);
	g_object_unref(G_OBJECT(snw->accel_group));
	
	snw->accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(snw->bfwin->main_window), snw->accel_group);
	if (snippets_v.doc) {
		xmlNodePtr cur = xmlDocGetRootElement(snippets_v.doc);
		if (cur) {
			snippets_connect_accelerators_from_doc(snw, cur, snw->accel_group);
		}
	}
}

static void snippets_rebuild_accelerators(void) {
	/* loop over all windows and rebuild the accelerators */
	 g_hash_table_foreach(snippets_v.lookup,snippets_window_rebuild_accelerators_lcb,NULL);
}

static void snip_rpopup_rpopup_action_lcb(Tsnippetswin *snw,guint callback_action, GtkWidget *widget) {
	DEBUG_MSG("snip_rpopup_rpopup_action_lcb, called with action %d and widget %p\n",callback_action,widget);
	switch (callback_action) {
	case 1: /* edit */
		DEBUG_MSG("edit not yet implemented\n");
	break;
	case 3: /* new */
		{
		snippets_new_item_dialog(snw);
		}
	break;
	case 2: /* set accelerator */
		if (snw->lastclickednode && xmlStrEqual(snw->lastclickednode->name, (const xmlChar *)"leaf")) {
			gchar *accel = ask_accelerator_dialog(_("Set accelerator key"));
			if (accel) {
				if (accel[0]=='\0') {
					xmlAttrPtr prop = xmlHasProp(snw->lastclickednode, (const xmlChar *)"accelerator");
					if (prop) xmlRemoveProp(prop);
				} else {
					xmlSetProp(snw->lastclickednode, (const xmlChar *)"accelerator", (const xmlChar *)accel);
				}
				snippets_rebuild_accelerators();
				snippets_store();
				g_free(accel);
			}
		}
	break;
	
	}
}

static GtkItemFactoryEntry snip_rpopup_menu_entries[] = {
	{ N_("/_Edit"),		NULL,	snip_rpopup_rpopup_action_lcb,		1,	"<Item>" },
	{ N_("/_New"),		NULL,	snip_rpopup_rpopup_action_lcb,		3,	"<Item>" },
	{ "/sep1",						NULL,	NULL,									0,	"<Separator>" },
	{ N_("/Set _accelerator"),			NULL,	snip_rpopup_rpopup_action_lcb,	2,	"<Item>" }
};

#ifdef ENABLE_NLS
static gchar *snippets_menu_translate(const gchar * path, gpointer data) {
	return _(path);
}
#endif       

static GtkWidget *snip_rpopup_create_menu(Tsnippetswin *snw, xmlNodePtr cur) {
	GtkWidget *menu;
	GtkItemFactory *menumaker;

	menumaker = gtk_item_factory_new(GTK_TYPE_MENU, "<snippets_rpopup>", NULL);
#ifdef ENABLE_NLS
	gtk_item_factory_set_translate_func(menumaker,snippets_menu_translate,"<snippets_rpopup>",NULL);
#endif
	gtk_item_factory_create_items(menumaker, sizeof(snip_rpopup_menu_entries)/sizeof(GtkItemFactoryEntry), snip_rpopup_menu_entries, snw);
	menu = gtk_item_factory_get_widget(menumaker, "<snippets_rpopup>");

	return menu;
}

static gboolean snippetview_button_press_lcb(GtkWidget *widget, GdkEventButton *event, Tsnippetswin *snw) {
	if (event->button==3 || (event->button==1 && event->type == GDK_2BUTTON_PRESS)) {
		xmlNodePtr cur;
		GtkTreePath *path;
		gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(snw->view), event->x, event->y, &path, NULL, NULL, NULL);
		cur = snippetview_get_node_at_path(path);
		if (cur) {
			snw->lastclickednode = cur;
			if (snw->lastclickedpath) gtk_tree_path_free(snw->lastclickedpath);   
			snw->lastclickedpath = path;
			if (event->button==1 && event->type == GDK_2BUTTON_PRESS) { /* left mouse button double-clicked */
				if (xmlStrEqual(cur->name, (const xmlChar *)"leaf")) {
					snippet_activate_leaf(snw, cur);
				}
			} else if (event->button==3) { /* right mouse button clicked */
				GtkWidget *menu;
				menu = snip_rpopup_create_menu(snw, cur);
				gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);	
			}
		} else {
			snw->lastclickednode = NULL;
			if (snw->lastclickedpath) gtk_tree_path_free(snw->lastclickedpath);   
			snw->lastclickedpath = NULL;
			gtk_tree_path_free(path);
		}
	}
	return FALSE; /* pass the event on */
}

static gchar* snippets_treetip_lcb(gconstpointer bfwin, gconstpointer tree, gint x, gint y) {
	GtkTreePath *path;
	xmlNodePtr cur;
	gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree), x, y, &path, NULL, NULL, NULL);
	cur = snippetview_get_node_at_path(path); 
	if (cur && xmlStrEqual(cur->name, (const xmlChar *)"leaf")) {
		xmlChar *tooltip, *accelerator;
		gchar *tooltip2=NULL, *accelerator2=NULL;
		tooltip = xmlGetProp(cur, (const xmlChar *)"tooltip");
		accelerator = xmlGetProp(cur, (const xmlChar *)"accelerator");
		if (tooltip) {
			tooltip2 = g_markup_escape_text((gchar *)tooltip,-1);
			xmlFree(tooltip);
		}
		if (accelerator) {
			accelerator2 = g_markup_escape_text((gchar *)accelerator,-1);
			xmlFree(accelerator);
		}
		if (tooltip && !accelerator) {
			return tooltip2;
		} else if (accelerator && !tooltip) {
			return accelerator2;
		} else if (tooltip && accelerator) {
			gchar *tmp;
			tmp = g_strconcat(tooltip2, "\n", accelerator2, NULL);
			g_free(tooltip2);
			g_free(accelerator2);
			return tmp;
		} else {
			return NULL;
		}
	}
	return NULL;
}

void snippets_sidepanel_initgui(Tbfwin *bfwin) {
	Tsnippetswin *snw;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	DEBUG_MSG("snippets_sidepanel_initgui, bfwin=%p\n",bfwin);
	
	snw = g_new0(Tsnippetswin,1);
	snw->bfwin = bfwin;
	g_hash_table_insert(snippets_v.lookup,bfwin,snw);	
	DEBUG_MSG("snippets_sidepanel_initgui, snw=%p, store=%p\n",snw,snippets_v.store);
	snw->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(snippets_v.store));
	
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Title",renderer,"text", 0,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(snw->view), column);
	
	g_signal_connect(G_OBJECT(snw->view), "button_press_event",G_CALLBACK(snippetview_button_press_lcb),snw);
	
	gtk_notebook_append_page(GTK_NOTEBOOK(bfwin->leftpanel_notebook),snw->view,gtk_label_new(_("snippets")));
	
	snw->ttips = tree_tips_new_full(snw->bfwin,snw->view,snippets_treetip_lcb);
	
	/* now parse the accelerator, and make it active for this item */
	snw->accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(snw->bfwin->main_window), snw->accel_group);
	DEBUG_MSG("snippets_sidepanel_initgui, connect accelerators\n");
	if (snippets_v.doc) {
		xmlNodePtr cur = xmlDocGetRootElement(snippets_v.doc);
		if (cur) {
			snippets_connect_accelerators_from_doc(snw, cur, snw->accel_group);
		}
	}
	DEBUG_MSG("snippets_sidepanel_initgui, finished\n");
}

void snippets_sidepanel_destroygui(Tbfwin *bfwin) {
	Tsnippetswin *snw;
	/* the widget is auto destroyed, and there is nothing more to destroy */
	snw = g_hash_table_lookup(snippets_v.lookup,bfwin);
	if (snw) {
		tree_tips_destroy(snw->ttips);
		gtk_window_remove_accel_group(GTK_WINDOW(snw->bfwin->main_window),snw->accel_group);
		g_object_unref(G_OBJECT(snw->accel_group));
	}	
}
