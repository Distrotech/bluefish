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

static xmlNodePtr snippetview_get_path_if_leaf(GtkTreePath *path) {
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter(GTK_TREE_MODEL(snippets_v.store),&iter,path)) {
		xmlNodePtr cur;
		gtk_tree_model_get(GTK_TREE_MODEL(snippets_v.store), &iter, 1, &cur, -1);
		if (xmlStrcmp(cur->name, (const xmlChar *)"leaf")==0) {
			DEBUG_MSG("snippetview_path_is_leaf, return TRUE, found a leaf!\n");
			return cur;
		}
		DEBUG_MSG("snippetview_path_is_leaf, iter is not a leaf!\n");
	}
	DEBUG_MSG("snippetview_path_is_leaf, not a leaf or no iter\n");
	return NULL;
}

static gboolean snippetview_button_press_lcb(GtkWidget *widget, GdkEventButton *event, Tsnippetswin *snw) {
	if (event->button==1 && event->type == GDK_2BUTTON_PRESS) { /* active it! */
		GtkTreePath *path;
		xmlNodePtr cur;
		gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(snw->view), event->x, event->y, &path, NULL, NULL, NULL);
		cur = snippetview_get_path_if_leaf(path); 
		if (cur) {
			DEBUG_MSG("snippetview_button_press_lcb, found leaf, activate!\n");
			/* based on the type of leaf we call different actions */
		}
	}
	return FALSE; /* pass the event on */
}

typedef struct {
	Tsnippetswin *snw;
	xmlNodePtr cur;
} Thotkey_cbdata;

gboolean testfunc(GtkAccelGroup *accel_group,GObject *acceleratable,guint keyval,GdkModifierType modifier,gpointer data) {
	Thotkey_cbdata *hcbdata = (Thotkey_cbdata *)data;
	xmlChar *title = xmlGetProp(hcbdata->cur, (const xmlChar *)"title");
	if (title) {
		DEBUG_MSG("testfunc called for %s\n",(const gchar *)title);
	}
		
	return TRUE;
}

static void snippets_connect_hotkeys_from_doc(Tsnippetswin *snw, xmlNodePtr cur, GtkAccelGroup *accel_group) {
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"branch"))) {
			snippets_connect_hotkeys_from_doc(snw, cur, accel_group);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *)"leaf"))) {
			xmlChar *hotkey;
			/* if there are shortcut keys defined for this leaf we should register them */
			hotkey = xmlGetProp(cur, (const xmlChar *)"hotkey");
			if (hotkey) {
				guint key;	
				GdkModifierType mod;
				gtk_accelerator_parse((const gchar *)hotkey,&key,&mod);
				if (key!=0 && mod!=0 && gtk_accelerator_valid(key,mod)) {
					GClosure *closure;
					Thotkey_cbdata *hcbdata;
					hcbdata = g_new(Thotkey_cbdata,1);
					hcbdata->snw = snw;
					hcbdata->cur = cur;
					DEBUG_MSG("snippets_connect_hotkeys_from_doc, connecting hotkey %s\n",hotkey);
					closure = g_cclosure_new(G_CALLBACK(testfunc),hcbdata,g_free);
					gtk_accel_group_connect(accel_group,key,mod,GTK_ACCEL_VISIBLE, closure);
				}
			}
		}
		cur = cur->next;
	}
}

void snippets_sidepanel_initgui(Tbfwin *bfwin) {
	Tsnippetswin *snw;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkAccelGroup *accel_group;

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
		
	/* now parse the hotkey, and make it active for this item */
	accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(snw->bfwin->main_window), accel_group);
	DEBUG_MSG("snippets_sidepanel_initgui, connect hotkeys\n");
	if (snippets_v.doc) {
		xmlNodePtr cur = xmlDocGetRootElement(snippets_v.doc);
		if (cur) {
			snippets_connect_hotkeys_from_doc(snw, cur, accel_group);
		}
	}
	DEBUG_MSG("snippets_sidepanel_initgui, finished\n");
}

void snippets_sidepanel_destroygui(Tbfwin *bfwin) {
	/* the widget is auto destroyed, and there is nothing more to destroy */
}
