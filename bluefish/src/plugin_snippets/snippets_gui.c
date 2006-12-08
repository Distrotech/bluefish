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
#include "snippets_leaf_snr.h"
#include "../document.h"
#include "../gtk_easy.h"

static xmlNodePtr snippetview_get_node_at_path(GtkTreePath *path) {
	if (path) {
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(snippets_v.store),&iter,path)) {
			xmlNodePtr cur=NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(snippets_v.store), &iter, NODE_COLUMN, &cur, -1);
			return cur;
		}
	}
	return NULL;
}

static void snippet_activate_leaf(Tsnippetswin *snw, xmlNodePtr cur) {
	xmlChar *type = xmlGetProp(cur, (const xmlChar *)"type");
	if (!type) {
		DEBUG_MSG("snippet_activate_leaf, no type\n");
		return;
	}
	if (xmlStrEqual(type, (const xmlChar *)"insert")) {
		snippets_activate_leaf_insert(snw, cur);
	} else if (xmlStrEqual(type, (const xmlChar *)"snr")) {
		snippets_activate_leaf_snr(snw, cur);
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
			if (event->button==3) { /* right mouse button clicked */
				GtkWidget *menu;
				menu = snip_rpopup_create_menu(snw, NULL);
				gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);	
			}
		}
	}
	return FALSE; /* pass the event on */
}

static gchar* snippets_treetip_lcb(gconstpointer bfwin, gconstpointer tree, gint x, gint y) {
	GtkTreePath *path;	
	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree), x, y, &path, NULL, NULL, NULL)) {
		xmlNodePtr cur = snippetview_get_node_at_path(path);
		DEBUG_MSG("snippets_treetip_lcb, found node %p for path %p\n",cur,path); 
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
	
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(snw->view), FALSE);
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column,renderer
			,"pixbuf",PIXMAP_COLUMN
			,"pixbuf_expander_closed",PIXMAP_COLUMN
			,"pixbuf_expander_open",PIXMAP_COLUMN
			,NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column,renderer,"text", TITLE_COLUMN,NULL);
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
