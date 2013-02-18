/* Bluefish HTML Editor
 * snippets_gui.c - plugin for snippets sidebar
 *
 * Copyright (C) 2006-2012 Olivier Sessink
 * Copyright (C) 2011-2012 James Hayward
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

#define _GNU_SOURCE

#include <string.h>
#include <gdk/gdkkeysyms.h>

#include "snippets.h"
#include "snippets_gui.h"
#include "snippets_load.h"
#include "snippets_leaf_insert.h"
#include "snippets_leaf_snr.h"
#include "snippets_wizard.h"
#include "snippetsmenu.h"
#include "../bfwin_uimanager.h"
#include "../document.h"
#include "../gtk_easy.h"
#include "../bf_lib.h"

/* GdkPixbuf RGBA C-Source image dump 1-byte-run-length-encoded */

#ifdef __SUNPRO_C
#pragma align 4 (snippet_icon)
#endif
#ifdef __GNUC__
static const guint8 snippet_icon[] __attribute__ ((__aligned__(4))) =
#else
static const guint8 snippet_icon[] =
#endif
{ ""
		/* Pixbuf magic (0x47646b50) */
		"GdkP"
		/* length: header (24) + pixel_data (1032) */
		"\0\0\4\40"
		/* pixdata_type (0x2010002) */
		"\2\1\0\2"
		/* rowstride (96) */
		"\0\0\0`"
		/* width (24) */
		"\0\0\0\30"
		/* height (24) */
		"\0\0\0\30"
		/* pixel_data: */
	"\235\0\0\0\0\215\0\1\0\377\212\0\0\0\0\3\0\1\0\377\321\323\320\377\361"
		"\363\360\377\210\376\377\374\377\4\371\374\370\377\332\334\331\377]]"
		"X\377\0\1\0\377\211\0\0\0\0\2\0\1\0\377\361\363\360\377\212\376\377\374"
		"\377\4\302\304\301\377\321\323\320\377\324\326\323\377\0\1\0\377\210"
		"\0\0\0\0\1\0\1\0\377\213\376\377\374\377\5\302\304\301\377\335\337\334"
		"\377\361\363\360\377\234\236\233\377\0\1\0\377\203\0\0\0\0\3\220\234"
		"\251\377\213\232\255\377\220\234\251\377\204\213\232\255\377\202\205"
		"\220\235\377\3~\215\237\377\200\213\230\377\256\260\255\377\204\376\377"
		"\374\377\6\256\260\255\377\343\345\342\377\376\377\374\377\335\337\334"
		"\377\212\212\204\377\0\1\0\377\202\0\0\0\0\1\212\225\243\377\203\277"
		"\313\331\377\202\270\314\330\377\204\274\310\325\377\2\205\220\235\377"
		"\332\334\331\377\204\376\377\374\377\12\242\244\241\377\361\363\360\377"
		"\376\377\374\377\361\363\360\377\321\323\320\377\242\244\241\377\0\1"
		"\0\377\0\0\0\0\212\225\243\377\277\313\331\377\204\274\310\325\377\1"
		"\270\314\330\377\203\274\310\325\377\2\205\220\235\377\332\334\331\377"
		"\204\376\377\374\377\1\234\236\233\377\207\0\1\0\377\1\205\220\235\377"
		"\202\274\310\325\377\1\270\314\330\377\203\274\310\325\377\202\377\0"
		"\0\377\3\274\310\325\377\200\213\230\377\324\326\323\377\205\376\377"
		"\374\377\7\335\337\334\377\321\323\320\377\256\260\255\377\234\236\233"
		"\377som\377]]X\377\0\1\0\377\207\205\220\235\377\203\377\0\0\377\2\200"
		"\213\230\377\324\326\323\377\211\376\377\374\377\3\256\260\255\377\212"
		"\212\204\377\0\1\0\377\204\335\337\334\377\1\0\1\0\377\204\332\334\331"
		"\377\202\377\0\0\377\1\321\323\320\377\211\376\377\374\377\3\302\304"
		"\301\377\212\212\204\377\0\1\0\377\204\0\0\0\0\1\0\1\0\377\205\376\377"
		"\374\377\202\377\0\0\377\2\376\377\374\377\377\0\0\377\207\376\377\374"
		"\377\3\321\323\320\377\214\211{\377\0\1\0\377\204\0\0\0\0\1\0\1\0\377"
		"\206\376\377\374\377\203\377\0\0\377\207\376\377\374\377\3\335\337\334"
		"\377\226\226\215\377\0\1\0\377\204\0\0\0\0\1\0\1\0\377\203\376\377\374"
		"\377\3\220\234\251\377\213\232\255\377\220\234\251\377\203\377\0\0\377"
		"\1\213\232\255\377\202\205\220\235\377\7~\215\237\377\200\213\230\377"
		"\256\260\255\377\376\377\374\377\335\337\334\377\243\237\221\377\0\1"
		"\0\377\204\0\0\0\0\1\0\1\0\377\203\376\377\374\377\2\212\225\243\377"
		"\277\313\331\377\204\377\0\0\377\204\274\310\325\377\6\205\220\235\377"
		"\332\334\331\377\376\377\374\377\335\337\334\377\240\235\217\377\0\1"
		"\0\377\204\0\0\0\0\1\0\1\0\377\203\376\377\374\377\2\212\225\243\377"
		"\277\313\331\377\204\274\310\325\377\1\270\314\330\377\203\274\310\325"
		"\377\6\205\220\235\377\332\334\331\377\376\377\374\377\332\334\331\377"
		"\240\235\217\377\0\1\0\377\204\0\0\0\0\1\0\1\0\377\203\376\377\374\377"
		"\1\205\220\235\377\202\274\310\325\377\1\270\314\330\377\206\274\310"
		"\325\377\6\200\213\230\377\324\326\323\377\376\377\374\377\332\334\331"
		"\377\240\235\217\377\0\1\0\377\204\0\0\0\0\1\0\1\0\377\203\376\377\374"
		"\377\211\205\220\235\377\202\200\213\230\377\5\324\326\323\377\376\377"
		"\374\377\324\326\323\377\234\230\212\377\0\1\0\377\204\0\0\0\0\2\0\1"
		"\0\377\371\374\370\377\202\376\377\374\377\205\335\337\334\377\204\332"
		"\334\331\377\202\324\326\323\377\5\321\323\320\377\376\377\374\377\324"
		"\326\323\377\234\230\212\377\0\1\0\377\204\0\0\0\0\2\0\1\0\377\361\363"
		"\360\377\217\376\377\374\377\3\324\326\323\377\234\230\212\377\0\1\0"
		"\377\204\0\0\0\0\2\0\1\0\377\321\323\320\377\211\335\337\334\377\204"
		"\332\334\331\377\202\324\326\323\377\3\321\323\320\377\234\230\212\377"
		"\0\1\0\377\204\0\0\0\0\2\0\1\0\377\226\226\215\377\205\243\237\221\377"
		"\206\240\235\217\377\205\234\230\212\377\2fcV\377\0\1\0\377\205\0\0\0" "\0\222\0\1\0\377\231\0\0\0\0"
};

static xmlNodePtr
snippetview_get_node_at_path(GtkTreePath * path)
{
	if (path) {
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(snippets_v.store), &iter, path)) {
			xmlNodePtr cur = NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(snippets_v.store), &iter, NODE_COLUMN, &cur, -1);
			return cur;
		}
	}
	return NULL;
}

static void
snippet_activate_leaf(Tsnippetswin * snw, xmlNodePtr cur)
{
	xmlChar *type = xmlGetProp(cur, (const xmlChar *) "type");
	if (!type) {
		DEBUG_MSG("snippet_activate_leaf, no type\n");
		return;
	}
	if (xmlStrEqual(type, (const xmlChar *) "insert")) {
		snippets_activate_leaf_insert(snw, cur);
	} else if (xmlStrEqual(type, (const xmlChar *) "snr")) {
		snippets_activate_leaf_snr(snw, cur);
	} else {
		DEBUG_MSG("snippet_activate_leaf, unknown type %s\n", (const gchar *) type);
	}
	xmlFree(type);
}

typedef struct {
	Tsnippetswin *snw;
	xmlNodePtr cur;
} Taccelerator_cbdata;

gboolean
snippets_accelerator_activated_lcb(GtkAccelGroup * accel_group, GObject * acceleratable, guint keyval,
								   GdkModifierType modifier, gpointer data)
{
	Taccelerator_cbdata *hcbdata = (Taccelerator_cbdata *) data;
	snippet_activate_leaf(hcbdata->snw, hcbdata->cur);
	return TRUE;
}

static void
accelerator_cbdata_free(gpointer data)
{
	DEBUG_MSG("accelerator_cbdata_free, free %p\n", data);
	g_slice_free(Taccelerator_cbdata, data);
}

static void
snippets_connect_accelerators_from_doc(Tsnippetswin * snw, xmlNodePtr cur, GtkAccelGroup * accel_group)
{
	DEBUG_MSG("snippets_connect_accelerators_from_doc\n");
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((xmlStrEqual(cur->name, (const xmlChar *) "branch"))) {
			snippets_connect_accelerators_from_doc(snw, cur, accel_group);
		} else if ((xmlStrEqual(cur->name, (const xmlChar *) "leaf"))) {
			xmlChar *accelerator;
			/* if there are shortcut keys defined for this leaf we should register them */
			accelerator = xmlGetProp(cur, (const xmlChar *) "accelerator");
			if (accelerator) {
				guint key;
				GdkModifierType mod;
				gtk_accelerator_parse((const gchar *) accelerator, &key, &mod);
				if (key != 0 && (mod != 0 || (key >= GDK_KEY_F1 && key <= GDK_KEY_F12)) && gtk_accelerator_valid(key, mod)) {
					GClosure *closure;
					Taccelerator_cbdata *hcbdata;
					hcbdata = g_slice_new(Taccelerator_cbdata);
					hcbdata->snw = snw;
					hcbdata->cur = cur;
					DEBUG_MSG
						("snippets_connect_accelerators_from_doc, connecting accelerator %s to data %p, and add to accel_group %p\n",
						 accelerator, hcbdata, accel_group);
					closure =
						g_cclosure_new(G_CALLBACK(snippets_accelerator_activated_lcb), hcbdata,
									   (GClosureNotify) accelerator_cbdata_free);
					gtk_accel_group_connect(accel_group, key, mod, GTK_ACCEL_VISIBLE, closure);
					g_object_watch_closure(G_OBJECT(snw->bfwin->main_window), closure);
				} else {
					g_print("Invalid shortcut key %s found in snippets library\n", accelerator);
				}
				xmlFree(accelerator);
			}
		}
		cur = cur->next;
	}
}

void
snippets_rebuild_accelerators(void)
{
	/* loop over all windows and rebuild the accelerators */
	GList *tmplist;
	DEBUG_MSG("snippets_rebuild_accelerators\n");
	for (tmplist = g_list_first(main_v->bfwinlist); tmplist; tmplist = tmplist->next) {
		Tbfwin *bfwin = tmplist->data;
		Tsnippetswin *snw = g_hash_table_lookup(snippets_v.lookup, bfwin);
		if (!snw)
			continue;
		DEBUG_MSG("remove accel_group from bfwin %p\n",bfwin);
		gtk_window_remove_accel_group(GTK_WINDOW(bfwin->main_window), snw->accel_group);
		g_object_unref(G_OBJECT(snw->accel_group));

		snw->accel_group = gtk_accel_group_new();
		DEBUG_MSG("add new accel_group %p to bfwin %p\n",snw->accel_group,bfwin);
		gtk_window_add_accel_group(GTK_WINDOW(bfwin->main_window), snw->accel_group);
		if (snippets_v.doc) {
			xmlNodePtr cur = xmlDocGetRootElement(snippets_v.doc);
			if (cur) {
				snippets_connect_accelerators_from_doc(snw, cur, snw->accel_group);
			}
		}
	}
}

static gboolean
snippets_delete(xmlNodePtr node, GtkTreePath * path)
{
	if (node && path) {
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(snippets_v.store), &iter, path)) {
			/* delete from the treestore */
			DEBUG_MSG("snippets_delete, removing from tree store\n");
			gtk_tree_store_remove(GTK_TREE_STORE(snippets_v.store), &iter);
			/* delete from the xmldoc */
			DEBUG_MSG("snippets_delete, removing from xmldoc\n");
			xmlUnlinkNode(node);
			xmlFreeNode(node);
			return TRUE;
		}
	}
	return FALSE;
}

static void
snippetsmenu_cb(gpointer user_data, gpointer data)
{
	xmlNodePtr cur = data;
	Tsnippetswin *snw = user_data;
	if (snw && cur && xmlStrEqual(cur->name, (const xmlChar *) "leaf")) {
		snippet_activate_leaf(snw, cur);
	}
}

void
snippets_show_as_menu(Tsnippetswin * snw, gboolean enable)
{
	if (enable) {
		if (!snw->snippetsmenu) {
			gint width = gdk_screen_get_width(gtk_window_get_screen(GTK_WINDOW(snw->bfwin->main_window)));
			snw->snippetsmenu = snippets_menu_new(width);
			gtk_box_pack_start(GTK_BOX(snw->bfwin->toolbarbox), snw->snippetsmenu, FALSE, FALSE, 0);
			gtk_widget_show(snw->snippetsmenu);	/* show is required such that the size requests are accurate */
			snippets_menu_set_model((SnippetsMenu *) snw->snippetsmenu, (GtkTreeModel *) snippets_v.store,
									snippetsmenu_cb, snw, TITLE_COLUMN, NODE_COLUMN);
		} else
			gtk_widget_show(snw->snippetsmenu);
	} else if (snw->snippetsmenu) {
		gtk_widget_hide(snw->snippetsmenu);
	}

	bfwin_set_menu_toggle_item_from_path(snw->bfwin->uimanager, "/MainMenu/ViewMenu/ViewSnippetsMenu",
										 enable);
}

static void
popup_menu_create(Tsnippetswin * snw, xmlNodePtr cur, GdkEventButton * event)
{
	Tbfwin *bfwin = snw->bfwin;
	Tsnippetssession *sns = snippets_get_session(bfwin->session);
	GtkWidget *menu;
	gint state = 0;				/* 0= nothing clicked, 1=branch, 2\=leaf */

	menu = gtk_ui_manager_get_widget(bfwin->uimanager, "/SnippetsMenu");

	if (snw->lastclickednode) {
		if (xmlStrEqual(snw->lastclickednode->name, (const xmlChar *) "leaf")) {
			state = 2;
		} else {
			state = 1;
		}
	}

	bfwin_set_menu_toggle_item_from_path(bfwin->uimanager, "/SnippetsMenu/ShowAsMenu", sns->show_as_menu);

	bfwin_action_set_sensitive(bfwin->uimanager, "/SnippetsMenu/NewSnippet", (state != 2));
	bfwin_action_set_sensitive(bfwin->uimanager, "/SnippetsMenu/EditSnippet", (state != 0));
	bfwin_action_set_sensitive(bfwin->uimanager, "/SnippetsMenu/DeleteSnippet", (state == 2));
	bfwin_action_set_sensitive(bfwin->uimanager, "/SnippetsMenu/SetAccelerator", (state == 2));
	bfwin_action_set_sensitive(bfwin->uimanager, "/SnippetsMenu/DeleteBranch", (state == 1));
	bfwin_action_set_sensitive(bfwin->uimanager, "/SnippetsMenu/Export", (state != 0));

	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
}

static gboolean
snippetview_button_press_lcb(GtkWidget * widget, GdkEventButton * event, Tsnippetswin * snw)
{
	if (snippets_v.doc) {
		if (event->button == 3 || (event->button == 1 && event->type == GDK_2BUTTON_PRESS)) {
			xmlNodePtr cur;
			GtkTreePath *path;
			gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(snw->view), event->x, event->y, &path, NULL, NULL,
										  NULL);
			cur = snippetview_get_node_at_path(path);
			if (cur) {
				snw->lastclickednode = cur;
				if (snw->lastclickedpath)
					gtk_tree_path_free(snw->lastclickedpath);
				snw->lastclickedpath = path;
				if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {	/* left mouse button double-clicked */
					if (xmlStrEqual(cur->name, (const xmlChar *) "leaf")) {
						snippet_activate_leaf(snw, cur);
						return TRUE;
					}
				} else if (event->button == 3) {	/* right mouse button clicked */
					popup_menu_create(snw, cur, event);
				}
			} else {
				snw->lastclickednode = NULL;
				if (snw->lastclickedpath)
					gtk_tree_path_free(snw->lastclickedpath);
				snw->lastclickedpath = NULL;
				gtk_tree_path_free(path);
				if (event->button == 3) {	/* right mouse button clicked */
					popup_menu_create(snw, NULL, event);
				}
			}
		}
	}
	return FALSE;				/* pass the event on */
}

static gchar *
snippets_gen_treetip_string(xmlNodePtr cur)
{
	if (cur && xmlStrEqual(cur->name, (const xmlChar *) "leaf")) {
		xmlChar *tooltip, *accelerator;
		gchar *tooltip2 = NULL, *accelerator2 = NULL;
		tooltip = xmlGetProp(cur, (const xmlChar *) "tooltip");
		accelerator = xmlGetProp(cur, (const xmlChar *) "accelerator");
		if (tooltip && tooltip[0]!='\0') {
			tooltip2 = g_markup_escape_text((gchar *) tooltip, -1);
			xmlFree(tooltip);
		} else {
			xmlChar *type = xmlGetProp(cur, (const xmlChar *) "type");
			if (type && xmlStrEqual(type, (const xmlChar *) "insert")) {
				gchar *tmp = snippets_tooltip_from_insert_content(cur);
				if (tmp) {
					tooltip2 = g_markup_escape_text(tmp, -1);
					g_free(tmp);
				}
			}					/*else if (type && xmlStrEqual(type, (const xmlChar *)"snr")) {
								   TODO
								   } */
			if (type)
				xmlFree(type);
		}
		if (accelerator) {
			accelerator2 = g_markup_escape_text((gchar *) accelerator, -1);
			xmlFree(accelerator);
		}
		if (tooltip2 && !accelerator2) {
			return tooltip2;
		} else if (accelerator2 && !tooltip2) {
			return accelerator2;
		} else if (tooltip2 && accelerator2) {
			gchar *tmp;
			tmp = g_strconcat(tooltip2, "\n", accelerator2, NULL);
			g_free(tooltip2);
			g_free(accelerator2);
			return tmp;
		}
	}
	return NULL;
}

static gboolean
snippets_treetip_lcb(GtkWidget * widget, gint x, gint y, gboolean keyboard_tip, GtkTooltip * tooltipwidget,
					 gpointer user_data)
{
	Tsnippetswin *snw = user_data;
	if (snippets_v.doc) {
		GtkTreePath *path;
		if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(snw->view), x, y, &path, NULL, NULL, NULL)) {
			gchar *tooltip;
			xmlNodePtr cur = snippetview_get_node_at_path(path);
			DEBUG_MSG("snippets_treetip_lcb, found node %p for path %p\n", cur, path);
			tooltip = snippets_gen_treetip_string(cur);
			if (tooltip) {
				gtk_tooltip_set_markup(tooltipwidget, tooltip);
				g_free(tooltip);
				gtk_tree_path_free(path);
				return TRUE;
			}
			gtk_tree_path_free(path);
		} else {
			gchar *arr[] = {
				N_("Click the right mouse button to add, edit or delete snippets."),
				N_("Use drag and drop to re-order snippets"),
				N_("To exchange snippets with others use import or export")
			};
			gtk_tooltip_set_text(tooltipwidget, _(arr[g_random_int_range(0, 3)]));
			return TRUE;
		}
	}
	return FALSE;
}

static void
snippetview_drag_data_get_lcb(GtkWidget * widget, GdkDragContext * ctx, GtkSelectionData * data, guint info,
							  guint time, gpointer user_data)
{
	if (gtk_selection_data_get_target(data) == gdk_atom_intern("BLUEFISH_SNIPPET", FALSE)) {
		GtkTreePath *path;
		GtkTreeIter iter;
		GtkTreeSelection *selection;
		GtkTreeModel *model;
		gchar *strpath;

		DEBUG_MSG("snippetview_drag_data_get_lcb, started\n");

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
		if (!gtk_tree_selection_get_selected(selection, &model, &iter))
			return;
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(snippets_v.store), &iter);
		strpath = gtk_tree_path_to_string(path);
		gtk_selection_data_set(data, gtk_selection_data_get_target(data), 8, (guchar *) strpath,
							   strlen(strpath));
		DEBUG_MSG("snippetview_drag_data_get_lcb, set path %p (%s)\n", strpath, strpath);
		gtk_tree_path_free(path);
	}
}

static void
snippetview_drag_data_received_lcb(GtkWidget * widget, GdkDragContext * context, guint x, guint y,
								   GtkSelectionData * sd, guint info, guint time, gpointer user_data)
{
	g_signal_stop_emission_by_name(widget, "drag_data_received");
	DEBUG_MSG("snippetview_drag_data_received_lcb \n");
	if (gtk_selection_data_get_target(sd) == gdk_atom_intern("BLUEFISH_SNIPPET", FALSE)
		&& gtk_selection_data_get_data(sd)) {
		GtkTreePath *destpath = NULL, *srcpath;
		GtkTreeViewDropPosition position;

		srcpath = gtk_tree_path_new_from_string((gchar *) gtk_selection_data_get_data(sd));

		if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget), x, y, &destpath, &position)) {
			GtkTreeIter srciter, destiter, newiter, parentiter;
			xmlNodePtr srcnode, destnode;

			if (0 == gtk_tree_path_compare(srcpath, destpath) || gtk_tree_path_is_ancestor(srcpath, destpath)) {
				DEBUG_MSG("source and destination combination invalid\n");
				gtk_drag_finish(context, FALSE, TRUE, time);
				return;
			}

			if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(snippets_v.store), &srciter, srcpath)) {
				DEBUG_MSG("snippetview_drag_data_received_lcb, error, no iter for srcpath\n");
				gtk_drag_finish(context, FALSE, TRUE, time);
				return;
			}
			gtk_tree_model_get(GTK_TREE_MODEL(snippets_v.store), &srciter, NODE_COLUMN, &srcnode, -1);
			if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(snippets_v.store), &destiter, destpath)) {
				DEBUG_MSG("snippetview_drag_data_received_lcb, error, no iter for destpath\n");
				gtk_drag_finish(context, FALSE, TRUE, time);
				return;
			}
			gtk_tree_model_get(GTK_TREE_MODEL(snippets_v.store), &destiter, NODE_COLUMN, &destnode, -1);
			if (position == GTK_TREE_VIEW_DROP_AFTER || position == GTK_TREE_VIEW_DROP_BEFORE
				|| xmlStrEqual(destnode->name, (const xmlChar *) "leaf")) {
				DEBUG_MSG("snippetview_drag_data_received_lcb, drop location is a leaf\n");
				switch (position) {
				case GTK_TREE_VIEW_DROP_AFTER:
				case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
					srcnode = xmlAddNextSibling(destnode, srcnode);
					if (srcnode) {
						gtk_tree_store_remove(GTK_TREE_STORE(snippets_v.store), &srciter);
						if (gtk_tree_model_iter_parent
							(GTK_TREE_MODEL(snippets_v.store), &parentiter, &destiter)) {
							gtk_tree_store_insert_after(GTK_TREE_STORE(snippets_v.store), &newiter,
														&parentiter, &destiter);
						} else {
							gtk_tree_store_insert_after(GTK_TREE_STORE(snippets_v.store), &newiter, NULL,
														&destiter);
						}
						snippets_fill_tree_item_from_node(&newiter, srcnode);
					}
					break;
				case GTK_TREE_VIEW_DROP_BEFORE:
				case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
					srcnode = xmlAddPrevSibling(destnode, srcnode);
					if (srcnode) {
						gtk_tree_store_remove(GTK_TREE_STORE(snippets_v.store), &srciter);
						if (gtk_tree_model_iter_parent
							(GTK_TREE_MODEL(snippets_v.store), &parentiter, &destiter)) {
							gtk_tree_store_insert_before(GTK_TREE_STORE(snippets_v.store), &newiter,
														 &parentiter, &destiter);
						} else {
							gtk_tree_store_insert_before(GTK_TREE_STORE(snippets_v.store), &newiter, NULL,
														 &destiter);
						}
						snippets_fill_tree_item_from_node(&newiter, srcnode);
					}
					break;
				default:
					return;
				}
			} else {			/* branch */
				/*xmlNode *orig;
				   orig = srcnode; */
				DEBUG_MSG("snippetview_drag_data_received_lcb, drop location is a branch\n");
				xmlUnlinkNode(srcnode);
				srcnode = xmlAddChild(destnode, srcnode);
				if (srcnode) {
					/*xmlUnlinkNode(orig);
					   xmlFreeNode(orig); */
					gtk_tree_store_remove(GTK_TREE_STORE(snippets_v.store), &srciter);
					gtk_tree_store_append(GTK_TREE_STORE(snippets_v.store), &newiter, &destiter);
					snippets_fill_tree_item_from_node(&newiter, srcnode);
				}
			}
			g_idle_add(snippets_store_lcb, NULL);
			gtk_drag_finish(context, TRUE, TRUE, time);
			return;
		}
	}
	gtk_drag_finish(context, FALSE, TRUE, time);
}

static gboolean
snippets_search(GtkTreeModel *model,gint column,const gchar *key,GtkTreeIter *iter,gpointer search_data)
{
	xmlNodePtr node;
	gchar *tmp=NULL, *title=NULL;
	gboolean retval = TRUE;

	gtk_tree_model_get(model, iter, NODE_COLUMN, &node, TITLE_COLUMN, &title, -1);
	if (title && strcasestr(title, key)!= NULL) {
		retval = FALSE;
	}
	g_free(title);

	if (node) {
		xmlChar *type = xmlGetProp(node, (const xmlChar *) "type");
		if (type && xmlStrEqual(type, (const xmlChar *) "insert")) {
			tmp = snippets_tooltip_from_insert_content(node);
		}					/*else if (type && xmlStrEqual(type, (const xmlChar *)"snr")) {
		   TODO
	   } */
		if (type)
			xmlFree(type);
		if (tmp && strcasestr(tmp, key)!= NULL) {
			retval = FALSE;
		}
		g_free(tmp);
	}
	return retval;
}

void
snippets_sidepanel_initgui(Tbfwin * bfwin)
{
	Tsnippetswin *snw;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GdkPixbuf *pixbuf;
	GtkWidget *image, *entry, *vbox;
	GtkWidget *scrolwin;
/*	GtkTreeSelection *selection;*/
	GtkTargetEntry bfsnippet[] = { {"BLUEFISH_SNIPPET", GTK_TARGET_SAME_APP, 0} };

	DEBUG_MSG("snippets_sidepanel_initgui, bfwin=%p\n", bfwin);

	snw = snippets_get_win(bfwin);
	DEBUG_MSG("snippets_sidepanel_initgui, snw=%p, store=%p\n", snw, snippets_v.store);
	vbox = gtk_vbox_new(FALSE, 1);

	entry = gtk_entry_new();
	gtk_entry_set_icon_from_stock(GTK_ENTRY(entry), GTK_ENTRY_ICON_PRIMARY, GTK_STOCK_FIND);
	gtk_entry_set_icon_activatable(GTK_ENTRY(entry), GTK_ENTRY_ICON_PRIMARY, TRUE);
	/*g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(bmark_search_changed), bfwin); */
	gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, TRUE, 0);


	snw->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(snippets_v.store));
/*	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(snw->view));
	gtk_tree_selection_set_mode(selection,GTK_SELECTION_NONE);*/

	gtk_tree_view_set_search_entry(GTK_TREE_VIEW(snw->view), GTK_ENTRY(entry));
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(snw->view), snippets_search, snw, NULL);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(snw->view), FALSE);
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "pixbuf", PIXMAP_COLUMN, "pixbuf_expander_closed",
										PIXMAP_COLUMN, "pixbuf_expander_open", PIXMAP_COLUMN, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", TITLE_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(snw->view), column);

	g_signal_connect(G_OBJECT(snw->view), "button_press_event", G_CALLBACK(snippetview_button_press_lcb),
					 snw);

	gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(snw->view), GDK_BUTTON1_MASK, bfsnippet, 1,
										   GDK_ACTION_MOVE);
	gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(snw->view), bfsnippet, 1, GDK_ACTION_MOVE);
	g_signal_connect(G_OBJECT(snw->view), "drag-data-received",
					 G_CALLBACK(snippetview_drag_data_received_lcb), snw);
	g_signal_connect(G_OBJECT(snw->view), "drag-data-get", G_CALLBACK(snippetview_drag_data_get_lcb), snw);

	pixbuf = gdk_pixbuf_new_from_inline(-1, snippet_icon, FALSE, NULL);
	image = gtk_image_new_from_pixbuf(pixbuf);
	g_object_unref(pixbuf);
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), snw->view);
	gtk_box_pack_start(GTK_BOX(vbox), scrolwin, TRUE, TRUE, 1);
	gtk_notebook_insert_page_menu(GTK_NOTEBOOK(bfwin->leftpanel_notebook), vbox, image,
								  gtk_label_new(_("snippets")), 2);
	g_object_set(snw->view, "has-tooltip", TRUE, NULL);
	g_signal_connect(snw->view, "query-tooltip", G_CALLBACK(snippets_treetip_lcb), snw);

	DEBUG_MSG("snippets_sidepanel_initgui, finished\n");
}

static void
popup_menu_collapse_all(GtkAction * action, gpointer user_data)
{
	gtk_tree_view_collapse_all(GTK_TREE_VIEW(((Tsnippetswin *) user_data)->view));
}

static void
popup_menu_delete(GtkAction * action, gpointer user_data)
{
	Tsnippetswin *snw = (Tsnippetswin *) user_data;

	if (snippets_delete(snw->lastclickednode, snw->lastclickedpath)) {
		snw->lastclickednode = NULL;
		gtk_tree_path_free(snw->lastclickedpath);
		snw->lastclickedpath = NULL;
		g_idle_add(snippets_store_lcb, NULL);
	}
}

static void
popup_menu_edit_snippet(GtkAction * action, gpointer user_data)
{
	Tsnippetswin *snw = (Tsnippetswin *) user_data;

	if (snw->lastclickednode) {
		snippets_new_item_dialog(snw, snw->lastclickednode);
	}
}

static void
popup_menu_expand_all(GtkAction * action, gpointer user_data)
{
	gtk_tree_view_expand_all(GTK_TREE_VIEW(((Tsnippetswin *) user_data)->view));
}

static void
snippets_export_dialog_response(GtkDialog * dialog, gint response, Tsnippetswin * snw)
{
	if (response == GTK_RESPONSE_ACCEPT) {
		gchar *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		if (snw->lastclickednode) {
			snippets_export_node(snw->lastclickednode, filename);
		}
		g_free(filename);
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void
popup_menu_export_snippet(GtkAction * action, gpointer user_data)
{
	Tsnippetswin *snw = (Tsnippetswin *) user_data;
	GtkWidget *dialog;

	dialog =
		file_chooser_dialog(snw->bfwin, _("Snippet export filename"), GTK_FILE_CHOOSER_ACTION_SAVE,
							NULL, TRUE, FALSE, "snippets", FALSE);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
	g_signal_connect(dialog, "response", G_CALLBACK(snippets_export_dialog_response), snw);
	gtk_widget_show_all(dialog);
}

static void
snippets_import_dialog_response(GtkDialog * dialog, gint response, Tsnippetswin * snw)
{
	if (response == GTK_RESPONSE_ACCEPT) {
		gchar *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		if (snw->lastclickednode) {
			/* get the branch, not the node */
			if (xmlStrEqual(snw->lastclickednode->name, (const xmlChar *) "leaf")) {
				snippets_import_node(snw->lastclickednode->parent, filename);
			} else {
				snippets_import_node(snw->lastclickednode, filename);
			}
		} else {
			/* give the root-node */
			xmlNodePtr root;
			root = xmlDocGetRootElement(snippets_v.doc);
			snippets_import_node(root, filename);
		}
		g_free(filename);
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void
popup_menu_import_snippet(GtkAction * action, gpointer user_data)
{
	Tsnippetswin *snw = (Tsnippetswin *) user_data;
	GtkWidget *dialog;

	dialog =
		file_chooser_dialog(snw->bfwin, _("Snippet import filename"), GTK_FILE_CHOOSER_ACTION_OPEN,
							NULL, TRUE, FALSE, "snippets", FALSE);
	g_signal_connect(dialog, "response", G_CALLBACK(snippets_import_dialog_response), snw);
	gtk_widget_show_all(dialog);
}

static void
popup_menu_new_snippet(GtkAction * action, gpointer user_data)
{
	snippets_new_item_dialog((Tsnippetswin *) user_data, NULL);
}

static void
popup_menu_set_accelerator(GtkAction * action, gpointer user_data)
{
	Tsnippetswin *snw = (Tsnippetswin *) user_data;

	if (snw->lastclickednode && xmlStrEqual(snw->lastclickednode->name, (const xmlChar *) "leaf")) {
		gchar *accel = ask_accelerator_dialog(_("Set accelerator key"));
		if (accel) {
			if (accel[0] == '\0') {
				xmlAttrPtr prop = xmlHasProp(snw->lastclickednode, (const xmlChar *) "accelerator");
				if (prop)
					xmlRemoveProp(prop);
			} else {
				xmlSetProp(snw->lastclickednode, (const xmlChar *) "accelerator", (const xmlChar *) accel);
			}
			DEBUG_MSG("popup_menu_set_accelerator, call snippets_rebuild_accelerators()\n");
			snippets_rebuild_accelerators();
			g_idle_add(snippets_store_lcb, NULL);
			g_free(accel);
		}
	}
}

static void
popup_menu_show_as_menu(GtkAction * action, gpointer user_data)
{
	Tsnippetswin *snw = (Tsnippetswin *) user_data;
	Tsnippetssession *sns = snippets_get_session(snw->bfwin->session);

	sns->show_as_menu = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	snippets_show_as_menu(snw, sns->show_as_menu);
}

static const gchar *snippets_plugin_ui =
	"<ui>"
	"  <menubar name='MainMenu'>"
	"    <menu action='ViewMenu'>"
	"      <menuitem action='ViewSnippetsMenu'/>"
	"    </menu>"
	"  </menubar>"
	"</ui>";

static const gchar *snippets_popup_menu_ui =
	"<ui>"
	"  <popup action='SnippetsMenu'>"
	"    <menuitem action='NewSnippet'/>"
	"    <menuitem action='EditSnippet'/>"
	"    <menuitem action='DeleteSnippet'/>"
	"    <menuitem action='DeleteBranch'/>"
	"    <separator/>"
	"    <menuitem action='SetAccelerator'/>"
	"    <separator/>"
	"    <menuitem action='ExpandAll'/>"
	"    <menuitem action='CollapseAll'/>"
	"    <separator/>"
	"    <menuitem action='ShowAsMenu'/>"
	"    <separator/>"
	"    <menuitem action='Export'/>"
	"    <menuitem action='Import'/>"
	"  </popup>"
	"</ui>";

static const GtkActionEntry snippets_actions[] = {
	{"SnippetsMenu", NULL, N_("Snippets menu")},
	{"NewSnippet", NULL, N_("_New Snippet"), NULL, N_("New snippet"), G_CALLBACK(popup_menu_new_snippet)},
	{"EditSnippet", NULL, N_("_Edit Snippet"), NULL, N_("Edit snippet"), G_CALLBACK(popup_menu_edit_snippet)},
	{"DeleteSnippet", NULL, N_("_Delete Snippet"), NULL, N_("Delete snippet"), G_CALLBACK(popup_menu_delete)},
	{"DeleteBranch", NULL, N_("Delete _Branch"), NULL, N_("Delete branch"), G_CALLBACK(popup_menu_delete)},
	{"SetAccelerator", NULL, N_("Set Snippet _Accelerator"), NULL, N_("Set snippet accelerator"),
	 G_CALLBACK(popup_menu_set_accelerator)},
	{"ExpandAll", NULL, N_("Ex_pand All"), NULL, N_("Expand all"), G_CALLBACK(popup_menu_expand_all)},
	{"CollapseAll", NULL, N_("_Collapse All"), NULL, N_("Collapse all"), G_CALLBACK(popup_menu_collapse_all)},
	{"Export", NULL, N_("E_xport"), NULL, N_("Export snippets"), G_CALLBACK(popup_menu_export_snippet)},
	{"Import", NULL, N_("I_mport"), NULL, N_("Import snippets"), G_CALLBACK(popup_menu_import_snippet)}
};

static const GtkToggleActionEntry snippets_toggle_actions[] = {
	{"ShowAsMenu", NULL, N_("_Show as menu"), NULL, N_("Show snippets menu"),
	 G_CALLBACK(popup_menu_show_as_menu)},
	{"ViewSnippetsMenu", NULL, N_("S_nippets Menu"), NULL, NULL, G_CALLBACK(popup_menu_show_as_menu), TRUE},
};

void
snippets_create_gui(Tbfwin * bfwin)
{
	Tsnippetswin *snw = snippets_get_win(bfwin);
	Tsnippetssession *sns = snippets_get_session(bfwin->session);

	GtkActionGroup *action_group;
	GError *error = NULL;

	if (!snw || !sns)
		return;
	DEBUG_MSG("snippets_create_gui, bfwin=%p\n",bfwin);
	action_group = gtk_action_group_new("SnippetsActions");
	gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE "_plugin_snippets");
	gtk_action_group_add_actions(action_group, snippets_actions, G_N_ELEMENTS(snippets_actions), snw);
	gtk_action_group_add_toggle_actions(action_group, snippets_toggle_actions,
										G_N_ELEMENTS(snippets_toggle_actions), snw);
	gtk_ui_manager_insert_action_group(bfwin->uimanager, action_group, 0);
	g_object_unref(action_group);

	gtk_ui_manager_add_ui_from_string(bfwin->uimanager, snippets_plugin_ui, -1, &error);
	if (error != NULL) {
		g_warning("building snippets plugin ui failed: %s", error->message);
		g_error_free(error);
	}

	gtk_ui_manager_add_ui_from_string(bfwin->uimanager, snippets_popup_menu_ui, -1, &error);
	if (error != NULL) {
		g_warning("building snippets plugin popup menu failed: %s", error->message);
		g_error_free(error);
	}
	/* now parse the accelerator, and make it active for this item */
	snw->accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(bfwin->main_window), snw->accel_group);
	DEBUG_MSG("snippets_create_gui, connect accelerators to bfwin %p\n",bfwin);
	if (snippets_v.doc) {
		xmlNodePtr cur = xmlDocGetRootElement(snippets_v.doc);
		if (cur) {
			snippets_connect_accelerators_from_doc(snw, cur, snw->accel_group);
		}
	}

	if (sns->show_as_menu) {
		snippets_show_as_menu(snw, TRUE);
	}
}
