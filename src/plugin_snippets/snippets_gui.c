/* Bluefish HTML Editor
 * snippets_gui.c - plugin for snippets sidebar
 *
 * Copyright (C) 2006-2009 Olivier Sessink
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
#include "snippets_load.h"
#include "snippets_leaf_insert.h"
#include "snippets_leaf_snr.h"
#include "snippets_wizard.h"
#include "snippetsmenu.h"
#include "../document.h"
#include "../gtk_easy.h"

/* GdkPixbuf RGBA C-Source image dump 1-byte-run-length-encoded */

#ifdef __SUNPRO_C
#pragma align 4 (snippet_icon)
#endif
#ifdef __GNUC__
static const guint8 snippet_icon[] __attribute__ ((__aligned__ (4))) = 
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
  "\206\240\235\217\377\205\234\230\212\377\2fcV\377\0\1\0\377\205\0\0\0"
  "\0\222\0\1\0\377\231\0\0\0\0"};

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

static void accelerator_cbdata_free(gpointer data) {
	g_slice_free(Taccelerator_cbdata, data);
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
					hcbdata = g_slice_new(Taccelerator_cbdata);
					hcbdata->snw = snw;
					hcbdata->cur = cur;
					DEBUG_MSG("snippets_connect_accelerators_from_doc, connecting accelerator %s\n",accelerator);
					closure = g_cclosure_new(G_CALLBACK(snippets_accelerator_activated_lcb),hcbdata,(GClosureNotify)accelerator_cbdata_free);
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

static gboolean snippets_delete(xmlNodePtr node, GtkTreePath *path) {
	if (node && path) {
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(snippets_v.store),&iter,path)) {
			/* delete from the treestore */
			DEBUG_MSG("snippets_delete, removing from tree store\n");
			gtk_tree_store_remove(GTK_TREE_STORE(snippets_v.store),&iter);
			/* delete from the xmldoc */
			DEBUG_MSG("snippets_delete, removing from xmldoc\n");
			xmlUnlinkNode(node);
			xmlFreeNode(node);
			return TRUE;
		}
	}
	return FALSE;
}

static void snippets_export_response_lcb(GtkDialog *dialog,gint response,Tsnippetswin *snw) {
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

static void snippets_import_response_lcb(GtkDialog *dialog,gint response,Tsnippetswin *snw) {
	if (response == GTK_RESPONSE_ACCEPT) {
		gchar *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		if (snw->lastclickednode) {
			/* get the branch, not the node */
			if (xmlStrEqual(snw->lastclickednode->name, (const xmlChar *)"leaf")) {
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

static void snip_rpopup_rpopup_action_lcb(Tsnippetswin *snw,guint callback_action, GtkWidget *widget) {
	DEBUG_MSG("snip_rpopup_rpopup_action_lcb, called with action %d and widget %p\n",callback_action,widget);
	switch (callback_action) {
	case 1: /* edit */
		if (snw->lastclickednode) {
			snippets_new_item_dialog(snw,snw->lastclickednode);
		}
	break;
	case 3: /* new */
		{
		snippets_new_item_dialog(snw,NULL);
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
				g_idle_add(snippets_store_lcb, NULL);
				g_free(accel);
			}
		}
	break;
	case 4:
		if (snippets_delete(snw->lastclickednode, snw->lastclickedpath)) {
			snw->lastclickednode = NULL;
			gtk_tree_path_free(snw->lastclickedpath);
			snw->lastclickedpath = NULL;
			g_idle_add(snippets_store_lcb, NULL);
		}
	break;
	case 5:
		gtk_tree_view_expand_all(GTK_TREE_VIEW(snw->view));
	break;
	case 6:
		gtk_tree_view_collapse_all(GTK_TREE_VIEW(snw->view));
	break;
	case 7:
		/* export */
		{
			GtkWidget *dialog;
			dialog = file_chooser_dialog(snw->bfwin, _("Snippet export filename"), GTK_FILE_CHOOSER_ACTION_SAVE, NULL, TRUE, FALSE, "snippets", FALSE);
			g_signal_connect(dialog, "response", G_CALLBACK(snippets_export_response_lcb), snw);
			gtk_widget_show_all(dialog);
		}
	break;
	case 8:
		/* import */
		{
			GtkWidget *dialog;
			dialog = file_chooser_dialog(snw->bfwin, _("Snippet import filename"), GTK_FILE_CHOOSER_ACTION_OPEN, NULL, TRUE, FALSE, "snippets", FALSE);
			g_signal_connect(dialog, "response", G_CALLBACK(snippets_import_response_lcb), snw);
			gtk_widget_show_all(dialog);
		}

	break;
	}
}

static void snippetsmenu_cb(gpointer user_data, gpointer data) {
	xmlNodePtr cur=data;
	Tsnippetswin *snw=user_data;
	if (snw && cur && xmlStrEqual(cur->name, (const xmlChar *)"leaf")) {
		snippet_activate_leaf(snw, cur);
	}
}

void snippets_show_as_menu(Tsnippetswin *snw, gboolean enable) {
	if (enable) {
		if (!snw->snippetsmenu) {
			gint width = gdk_screen_get_width(gtk_window_get_screen(GTK_WINDOW(snw->bfwin->main_window)));
			snw->snippetsmenu = snippets_menu_new(width);
			gtk_box_pack_start(GTK_BOX(snw->bfwin->toolbarbox), snw->snippetsmenu, FALSE, FALSE, 0);
			gtk_widget_show(snw->snippetsmenu); /* show is required such that the size requests are accurate */
			snippets_menu_set_model((SnippetsMenu *)snw->snippetsmenu, (GtkTreeModel *)snippets_v.store, snippetsmenu_cb, snw, TITLE_COLUMN, NODE_COLUMN);
		} else 
			gtk_widget_show(snw->snippetsmenu);
	} else if (snw->snippetsmenu) {
		gtk_widget_hide(snw->snippetsmenu);
	}
	setup_toggle_item(gtk_item_factory_from_widget(snw->bfwin->menubar), "/View/Snippets Menu", enable);
}

void snip_rpopup_toggle_cb(Tsnippetswin *snw,guint action,GtkWidget *widget) {
	Tsnippetssession *sns = snippets_get_session(snw->bfwin->session);
	sns->show_as_menu = GTK_CHECK_MENU_ITEM(widget)->active;
	snippets_show_as_menu(snw, sns->show_as_menu);
}
static GtkItemFactoryEntry snip_rpopup_menu_entries[] = {
	{ N_("/_New snippet"),		NULL,	snip_rpopup_rpopup_action_lcb,		3,	"<Item>" },
	{ N_("/_Edit snippet"),		NULL,	snip_rpopup_rpopup_action_lcb,		1,	"<Item>" },
	{ N_("/_Delete snippet"),		NULL,	snip_rpopup_rpopup_action_lcb,		4,	"<Item>" },
	{ N_("/Delete branch"),		NULL,	snip_rpopup_rpopup_action_lcb,		4,	"<Item>" },
	{ "/sep1",						NULL,	NULL,									0,	"<Separator>" },
	{ N_("/Set snippet _accelerator"),			NULL,	snip_rpopup_rpopup_action_lcb,	2,	"<Item>" },
	{ "/sep2",						NULL,	NULL,									0,	"<Separator>" },
	{ N_("/Expand all"),			NULL,	snip_rpopup_rpopup_action_lcb,	5,	"<Item>" },
	{ N_("/Collapse all"),			NULL,	snip_rpopup_rpopup_action_lcb,	6,	"<Item>" },
	{ "/sep3",						NULL,	NULL,									0,	"<Separator>" },
	{ N_("/Show as menu"),			NULL,	snip_rpopup_toggle_cb,	6,	"<ToggleItem>" },
	{ "/sep4",						NULL,	NULL,									0,	"<Separator>" },
	{ N_("/Export"),			NULL,	snip_rpopup_rpopup_action_lcb,	7,	"<Item>" },
	{ N_("/Import"),			NULL,	snip_rpopup_rpopup_action_lcb,	8,	"<Item>" }
};

#ifdef ENABLE_NLS
static gchar *snippets_menu_translate(const gchar * path, gpointer data) {
	return _(path);
}
#endif       

static GtkWidget *snip_rpopup_create_menu(Tsnippetswin *snw, xmlNodePtr cur) {
	GtkWidget *menu;
	GtkItemFactory *menumaker;
	gint state=0; /* 0= nothing clicked, 1=branch, 2\=leaf */
	Tsnippetssession *sns = snippets_get_session(snw->bfwin->session);
	menumaker = gtk_item_factory_new(GTK_TYPE_MENU, "<snippets_rpopup>", NULL);
#ifdef ENABLE_NLS
	gtk_item_factory_set_translate_func(menumaker,snippets_menu_translate,"<snippets_rpopup>",NULL);
#endif
	gtk_item_factory_create_items(menumaker, sizeof(snip_rpopup_menu_entries)/sizeof(GtkItemFactoryEntry), snip_rpopup_menu_entries, snw);
	menu = gtk_item_factory_get_widget(menumaker, "<snippets_rpopup>");
	
	if (snw->lastclickednode) {
		if (xmlStrEqual(snw->lastclickednode->name, (const xmlChar *)"leaf")) {
			state = 2;
		} else {
			state = 1;
		}
	}
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(menumaker, "/Show as menu")),sns->show_as_menu);

	gtk_widget_set_sensitive(gtk_item_factory_get_widget(menumaker, "/Edit snippet"), (state!=0));
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(menumaker, "/Delete snippet"), (state==2));
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(menumaker, "/Set snippet accelerator"), (state==2));
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(menumaker, "/New snippet"), (state!=2));
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(menumaker, "/Delete branch"), (state==1));
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(menumaker, "/Export"), (state!=0));
	return menu;
}

static gboolean snippetview_button_press_lcb(GtkWidget *widget, GdkEventButton *event, Tsnippetswin *snw) {
	if (snippets_v.doc) {
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
	}
	return FALSE; /* pass the event on */
}

static gboolean snippets_treetip_lcb(GtkWidget *widget,gint x,gint y,gboolean keyboard_tip, GtkTooltip *tooltipwidget, gpointer user_data) {
	Tsnippetswin *snw = user_data;
	if (snippets_v.doc) {
		GtkTreePath *path;	
		if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(snw->view), x, y, &path, NULL, NULL, NULL)) {
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
				if (tooltip2 && !accelerator2) {
					gtk_tooltip_set_markup(tooltipwidget, tooltip2);
					g_free(tooltip2);
					return TRUE;
				} else if (accelerator2 && !tooltip2) {
					gtk_tooltip_set_markup(tooltipwidget, accelerator2);
					g_free(accelerator2);
					return TRUE;
				} else if (tooltip2 && accelerator2) {
					gchar *tmp;
					tmp = g_strconcat(tooltip2, "\n", accelerator2, NULL);
					gtk_tooltip_set_markup(tooltipwidget, tmp);
					g_free(tooltip2);
					g_free(accelerator2);
					g_free(tmp);
					return TRUE;
				}
			}
			gtk_tree_path_free(path);
		} else {
			gchar *arr[] = {
				N_("Click the right mouse button to add, edit or delete snippets."),
				N_("Use drag and drop to re-order snippets"),
				N_("To exchange snippets with others use import or export")
			};
			gtk_tooltip_set_text(tooltipwidget, _(arr[g_random_int_range(0,3)]));
		}
	}
	return TRUE;
}

static void snippetview_drag_data_get_lcb(GtkWidget *widget, GdkDragContext *ctx,GtkSelectionData *data, guint info, guint time,gpointer user_data)  {
	if (data->target == gdk_atom_intern("BLUEFISH_SNIPPET", FALSE)) {
		GtkTreePath *path;
		GtkTreeIter iter;
		GtkTreeSelection *selection;
		GtkTreeModel *model;
		gchar *strpath;
		
		DEBUG_MSG("snippetview_drag_data_get_lcb, started\n");

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
		if (!gtk_tree_selection_get_selected(selection, &model, &iter)) return;		
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(snippets_v.store),&iter);
		strpath = gtk_tree_path_to_string(path);
		gtk_selection_data_set(data, data->target, 8, (guchar *) strpath, strlen(strpath));
		DEBUG_MSG("snippetview_drag_data_get_lcb, set path %p (%s)\n",strpath,strpath);
		gtk_tree_path_free(path);
	}
}

static void snippetview_drag_data_received_lcb(GtkWidget *widget, GdkDragContext *context,guint x, guint y, GtkSelectionData *sd,guint info, guint time, gpointer user_data) {
	g_signal_stop_emission_by_name(widget, "drag_data_received");
	DEBUG_MSG("snippetview_drag_data_received_lcb received node %p (%s)\n",sd->data, sd->data);
	if (sd->target == gdk_atom_intern("BLUEFISH_SNIPPET", FALSE) && sd->data) {
		GtkTreePath *destpath = NULL, *srcpath;
		GtkTreeViewDropPosition position;
		
		srcpath = gtk_tree_path_new_from_string((gchar *)sd->data);
		
		if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget), x, y,&destpath, &position)) {
			GtkTreeIter srciter, destiter, newiter, parentiter;
			xmlNodePtr srcnode, destnode;
			
			if (0==gtk_tree_path_compare(srcpath,destpath) || gtk_tree_path_is_ancestor(srcpath, destpath)) {
				DEBUG_MSG("source and destination combination invalid\n");
				gtk_drag_finish(context, FALSE, TRUE, time);
				return;
			}
			
			if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(snippets_v.store),&srciter,srcpath)) {
				DEBUG_MSG("snippetview_drag_data_received_lcb, error, no iter for srcpath\n");
				gtk_drag_finish(context, FALSE, TRUE, time);
				return;
			}
			gtk_tree_model_get(GTK_TREE_MODEL(snippets_v.store),&srciter,NODE_COLUMN,&srcnode,-1);
			if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(snippets_v.store), &destiter, destpath)) {
				DEBUG_MSG("snippetview_drag_data_received_lcb, error, no iter for destpath\n");
				gtk_drag_finish(context, FALSE, TRUE, time);
				return;
			}
			gtk_tree_model_get(GTK_TREE_MODEL(snippets_v.store),&destiter,NODE_COLUMN,&destnode,-1);
			if (position == GTK_TREE_VIEW_DROP_AFTER|| position == GTK_TREE_VIEW_DROP_BEFORE
						|| xmlStrEqual(destnode->name, (const xmlChar *)"leaf")) {
				DEBUG_MSG("snippetview_drag_data_received_lcb, drop location is a leaf\n");
				switch (position) {
					case GTK_TREE_VIEW_DROP_AFTER:
					case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
						srcnode = xmlAddNextSibling(destnode, srcnode);
						if (srcnode) {
							gtk_tree_store_remove(GTK_TREE_STORE(snippets_v.store),&srciter);
							if (gtk_tree_model_iter_parent(GTK_TREE_MODEL(snippets_v.store), &parentiter, &destiter)) {
								gtk_tree_store_insert_after(GTK_TREE_STORE(snippets_v.store), &newiter, &parentiter, &destiter);
							} else {
								gtk_tree_store_insert_after(GTK_TREE_STORE(snippets_v.store), &newiter, NULL, &destiter);
							}
							snippets_fill_tree_item_from_node(&newiter, srcnode);
						}
						break;
					case GTK_TREE_VIEW_DROP_BEFORE:
					case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
						srcnode = xmlAddPrevSibling(destnode, srcnode);
						if (srcnode) {
							gtk_tree_store_remove(GTK_TREE_STORE(snippets_v.store),&srciter);
							if (gtk_tree_model_iter_parent(GTK_TREE_MODEL(snippets_v.store), &parentiter, &destiter)) {
								gtk_tree_store_insert_before(GTK_TREE_STORE(snippets_v.store), &newiter, &parentiter, &destiter);
							} else {
								gtk_tree_store_insert_before(GTK_TREE_STORE(snippets_v.store), &newiter, NULL, &destiter);
							}
							snippets_fill_tree_item_from_node(&newiter, srcnode);
						}
						break;
					default:
						return;
				}				
			} else { /* branch */
				/*xmlNode *orig;
				orig = srcnode;*/
				DEBUG_MSG("snippetview_drag_data_received_lcb, drop location is a branch\n");
				xmlUnlinkNode(srcnode);
				srcnode = xmlAddChild(destnode, srcnode);
				if (srcnode) {
					/*xmlUnlinkNode(orig);
					xmlFreeNode(orig);*/
					gtk_tree_store_remove(GTK_TREE_STORE(snippets_v.store),&srciter);
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

void snippets_sidepanel_initgui(Tbfwin *bfwin) {
	Tsnippetswin *snw;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GdkPixbuf *pixbuf;
	GtkWidget *image;
	GtkWidget *scrolwin;
/*	GtkTreeSelection *selection;*/
	GtkTargetEntry bfsnippet[] = {{"BLUEFISH_SNIPPET", GTK_TARGET_SAME_APP, 0}};
	
	DEBUG_MSG("snippets_sidepanel_initgui, bfwin=%p\n",bfwin);
	
	snw = g_new0(Tsnippetswin,1);
	snw->bfwin = bfwin;
	g_hash_table_insert(snippets_v.lookup,bfwin,snw);	
	DEBUG_MSG("snippets_sidepanel_initgui, snw=%p, store=%p\n",snw,snippets_v.store);
	snw->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(snippets_v.store));
/*	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(snw->view));
	gtk_tree_selection_set_mode(selection,GTK_SELECTION_NONE);*/
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

	gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(snw->view), GDK_BUTTON1_MASK, bfsnippet,1, GDK_ACTION_MOVE);
	gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(snw->view), bfsnippet, 1, GDK_ACTION_MOVE);
	g_signal_connect(G_OBJECT(snw->view), "drag-data-received",G_CALLBACK(snippetview_drag_data_received_lcb), snw);
	g_signal_connect(G_OBJECT(snw->view), "drag-data-get",G_CALLBACK(snippetview_drag_data_get_lcb), snw);

	pixbuf = gdk_pixbuf_new_from_inline(-1,snippet_icon,FALSE,NULL);
	image = gtk_image_new_from_pixbuf(pixbuf);
	g_object_unref(pixbuf);
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), snw->view);
	gtk_notebook_insert_page_menu(GTK_NOTEBOOK(bfwin->leftpanel_notebook),scrolwin,image,gtk_label_new(_("snippets")),2);
	g_object_set(snw->view, "has-tooltip", TRUE, NULL);
	g_signal_connect(snw->view, "query-tooltip",G_CALLBACK(snippets_treetip_lcb), snw);
	/*snw->ttips = tree_tips_new_full(snw->bfwin,GTK_TREE_VIEW(snw->view),snippets_treetip_lcb);*/
	
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
		/*tree_tips_destroy(snw->ttips);*/
		gtk_window_remove_accel_group(GTK_WINDOW(snw->bfwin->main_window),snw->accel_group);
		g_object_unref(G_OBJECT(snw->accel_group));
	}	
}

void snippets_create_gui(Tbfwin *bfwin) {
	static GtkItemFactoryEntry menu_items[] = {
		{N_("/View/Snippets Menu"), NULL, snip_rpopup_toggle_cb, 0, "<ToggleItem>"}
	};
	Tsnippetswin *snw;
	GtkItemFactory *ifactory;
	Tsnippetssession *sns = snippets_get_session(bfwin->session);
	snw = g_hash_table_lookup(snippets_v.lookup,bfwin);

	ifactory = gtk_item_factory_from_widget(bfwin->menubar);
#ifdef ENABLE_NLS
	gtk_item_factory_set_translate_func(ifactory, snippets_menu_translate, "<bluefishmain>", NULL);
#endif
	gtk_item_factory_create_items(ifactory, sizeof(menu_items) / sizeof(menu_items[0]), menu_items, snw);

	if (sns->show_as_menu) {
		snippets_show_as_menu(snw, TRUE);
	}
	gtk_widget_show_all(bfwin->menubar);
	
}

