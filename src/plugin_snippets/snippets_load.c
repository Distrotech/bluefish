/* Bluefish HTML Editor
 * snippets_load.c - plugin for snippets sidebar
 *
 * Copyright (C) 2006-2010 Olivier Sessink
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
/*
the XML format is not yet fully designed
I'm starting the programming to get a feel for libXML

we'll have two main types of items in the xml document: branches and leaves
the branches are just placeholders, for leaves and for other branches
leaves are of some type:
- python script
- search and replace action
- custom insert
- custom dialog

<?xml version="1.0"?>
<snippets>
<branch title="html">
<branch title="form">
<leaf title="html form" type="insert" tooltip="html form" accelerator="&lt;Control&gt;F1">
<param name="action"/>
<param name="method"/>
<before>&lt;form method="%1" action="%0"&gt;</before>
<after>&lt;/form&gt;</after>
</leaf>
<leaf title="input type text" type="insert" tooltip="input type is text">
<param name="name"/>
<param name="value"/>
<before>&lt;input type="text" name="%0" value="%1" /&gt;</before>
</leaf>
<leaf title="input type submit" type="insert" tooltip="input type is submit" accelerator="&lt;Control&gt;F1">
<param name="name"/>
<param name="value"/>
<before>&lt;input type="submit" name="%0" value="%1" /&gt;</before>
</leaf>
</branch>
</branch>
</snippets>

*/
/* #define DEBUG */

#include <string.h>

#include "snippets.h"
#include "snippets_load.h"
#include "snippets_gui.h"
#include "../config.h"
#include "../bf_lib.h"

/* GdkPixbuf RGBA C-Source image dump */

#ifdef __SUNPRO_C
#pragma align 4 (pixmap_snr)
#endif
#ifdef __GNUC__
static const guint8 pixmap_snr[] __attribute__ ((__aligned__ (4))) = 
#else
static const guint8 pixmap_snr[] = 
#endif
{ ""
  /* Pixbuf magic (0x47646b50) */
  "GdkP"
  /* length: header (24) + pixel_data (900) */
  "\0\0\3\234"
  /* pixdata_type (0x1010002) */
  "\1\1\0\2"
  /* rowstride (60) */
  "\0\0\0<"
  /* width (15) */
  "\0\0\0\17"
  /* height (15) */
  "\0\0\0\17"
  /* pixel_data: */
  "cla\0wto\377NMK\377VWS\377VWS\377VWS\377VWS\377VWS\377VWS\377VWS\377"
  "\223\224\220\377cla\0cla\0cla\0cla\0cla\0NMK\377\361\363\360\377\373"
  "\375\372\377\373\375\372\377\361\363\360\377\373\375\372\377\373\375"
  "\372\377\373\375\372\377\305\310\303\377\245\245\242\377wto\377cla\0"
  "cla\0cla\0cla\0VWS\377\373\375\372\377\324\325\320\377wto\377VWS\377"
  "__Z\377\223\224\220\377\361\363\360\377\305\310\303\377\361\363\360\377"
  "\223\224\220\377jic\377cla\0cla\0cla\0NMK\377\373\375\372\377wto\377"
  "\242\243\224\377\261\263\235\377\324\325\320\377\210\206\204\377\245"
  "\245\242\377\264\266\261\377\334\336\333\377\324\325\320\377\210\206"
  "\204\377\210\206\204\377cla\0cla\0VWS\377\373\375\372\377NMK\377\242"
  "\243\224\377\261\263\235\377\324\325\320\377\242\243\224\377\210\206"
  "\204\377\245\245\242\377__Z\377NMK\377\23\23\20\377j[\34\377bWG\377c"
  "la\0NMK\377\373\375\372\377jic\377\245\245\242\377\261\263\235\377\261"
  "\263\235\377\203\200s\377\210\206\204\377\334\336\333\377\334\336\333"
  "\377\223\224\220\377\234\212*\377\305\235;\377:4$\377cla\0NMK\377\245"
  "\245\242\377\37\40\36\377jic\377__Z\377ebU\377__Z\377\245\245\242\377"
  "\334\336\333\377\245\245\242\377\234\212*\377\305\235;\377($\24\377_"
  "_Z\377cla\0.-+\377\23\23\20\377.-+\377\245\245\242\377\210\206\204\377"
  "\223\224\220\377\245\245\242\377\305\310\303\377\223\224\220\377\234"
  "\212*\377\264\2143\377:4$\377;;5\377wto\377\245\245\242\377\23\23\20"
  "\377\37\40\36\377\210\206\204\377\305\310\303\377\324\325\320\377\305"
  "\310\303\377\324\325\320\377\223\224\220\377\264\2143\377\305\235;\377"
  ":4$\377\210\206\204\377__Z\377\210\206\204\377\210\206\204\377\37\40"
  "\36\377\223\224\220\377\264\266\261\377\324\325\320\377\334\336\333\377"
  "\341\344\340\377\245\245\242\377\207t/\377\264\2143\377:4$\377\223\224"
  "\220\377\264\266\261\377__Z\377\210\206\204\377cla\0NMK\377\305\310\303"
  "\377\324\325\320\377\334\336\333\377\361\363\360\377\341\344\340\377"
  "__Z\377G;%\377:4$\377\210\206\204\377\264\266\261\377\324\325\320\377"
  "jic\377\210\206\204\377cla\0NMK\377\334\336\333\377\334\336\333\377\341"
  "\344\340\377\334\336\333\377\334\336\333\377;;5\377jic\377\245\245\242"
  "\377\264\266\261\377\324\325\320\377\324\325\320\377jic\377\210\206\204"
  "\377cla\0NMK\377\334\336\333\377\334\336\333\377\341\344\340\377\334"
  "\336\333\377\324\325\320\377\264\266\261\377\245\245\242\377\305\310"
  "\303\377\324\325\320\377\324\325\320\377\324\325\320\377jic\377\210\206"
  "\204\377cla\0NMK\377jic\377wto\377wto\377wto\377wto\377jic\377jic\377"
  "jic\377wto\377wto\377wto\377;;5\377\223\224\220\377cla\0cla\0\210\206"
  "\204\377\210\206\204\377\210\206\204\377\210\206\204\377\210\206\204"
  "\377\210\206\204\377\210\206\204\377\210\206\204\377\210\206\204\377"
  "\210\206\204\377\210\206\204\377\223\224\220\377cla\0"};


/* GdkPixbuf RGBA C-Source image dump */

#ifdef __SUNPRO_C
#pragma align 4 (pixmap_insert)
#endif
#ifdef __GNUC__
static const guint8 pixmap_insert[] __attribute__ ((__aligned__ (4))) = 
#else
static const guint8 pixmap_insert[] = 
#endif
{ ""
  /* Pixbuf magic (0x47646b50) */
  "GdkP"
  /* length: header (24) + pixel_data (840) */
  "\0\0\3`"
  /* pixdata_type (0x1010002) */
  "\1\1\0\2"
  /* rowstride (56) */
  "\0\0\0""8"
  /* width (14) */
  "\0\0\0\16"
  /* height (15) */
  "\0\0\0\17"
  /* pixel_data: */
  "WXU\377PRO\377WXU\377WXU\377WXU\377WXU\377WXU\377WXU\377WXU\377\234\234"
  "\231\377_`[\0_`[\0_`[\0_`[\0PRO\377\372\374\371\377\372\374\371\377\372"
  "\374\371\377\372\374\371\377\372\374\371\377\372\374\371\377\372\374"
  "\371\377\272\272\267\377\234\234\231\377\222\221\214\377_`[\0_`[\0_`"
  "[\0WXU\377\372\374\371\377\372\374\371\377\363\365\362\377\372\374\371"
  "\377\363\365\362\377\372\374\371\377\363\365\362\377\324\325\322\377"
  "\351\353\350\377}}{\377\212\206\204\377_`[\0_`[\0WXU\377\372\374\371"
  "\377\363\365\362\377\372\374\371\377\372\374\371\377\372\374\371\377"
  "\363\365\362\377\351\353\350\377\272\272\267\377\337\341\336\377\313"
  "\314\310\377}}{\377\234\234\231\377_`[\0WXU\377\372\374\371\377\363\365"
  "\362\377\363\365\362\377\372\374\371\377\372\374\371\377\372\374\371"
  "\377\351\353\350\377\212\206\204\377WXU\377GHC\377\35\33\17\377j[\34"
  "\377pnf\377WXU\377\372\374\371\377\363\365\362\377\372\374\371\377\372"
  "\374\371\377\363\365\362\377\363\365\362\377\363\365\362\377\351\353"
  "\350\377\324\325\322\377\203\200r\377\260\231,\377\255\2117\377@<0\377"
  "WXU\377\372\374\371\377\372\374\371\377\363\365\362\377\372\374\371\377"
  "\363\365\362\377\363\365\362\377\363\365\362\377\351\353\350\377\222"
  "\221\214\377\260\231,\377\255\2117\377\35\33\17\377\212\206\204\377W"
  "XU\377\363\365\362\377\363\365\362\377\363\365\362\377\363\365\362\377"
  "\351\353\350\377\363\365\362\377\351\353\350\377\217\214~\377\260\231"
  ",\377\237x2\377@<0\377,-+\377\222\221\214\377PRO\377\363\365\362\377"
  "\363\365\362\377\363\365\362\377\363\365\362\377\363\365\362\377\351"
  "\353\350\377\212\206\204\377\263\222+\377\255\2117\377@<0\377\234\234"
  "\231\377GHC\377\253\254\251\377WXU\377\351\353\350\377\363\365\362\377"
  "\351\353\350\377\363\365\362\377\351\353\350\377\234\234\231\377\255"
  "\2117\377\237x2\377@<0\377\272\272\267\377\313\314\310\377PRO\377\253"
  "\254\251\377WXU\377\351\353\350\377\351\353\350\377\351\353\350\377\351"
  "\353\350\377\324\325\322\377LI=\377K>%\377@<0\377\234\234\231\377\324"
  "\325\322\377\313\314\310\377SPI\377\253\254\251\377WXU\377\351\353\350"
  "\377\351\353\350\377\351\353\350\377\351\353\350\377\324\325\322\377"
  "796\377\212\206\204\377\253\254\251\377\313\314\310\377\324\325\322\377"
  "\313\314\310\377SPI\377\253\254\251\377PRO\377\351\353\350\377\351\353"
  "\350\377\351\353\350\377\351\353\350\377\324\325\322\377\313\314\310"
  "\377\313\314\310\377\324\325\322\377\324\325\322\377\324\325\322\377"
  "\313\314\310\377SPI\377\253\254\251\377GHC\377pnf\377pnf\377pnf\377p"
  "nf\377pnf\377pnf\377pnf\377pnf\377pnf\377pnf\377pnf\377796\377\272\272"
  "\267\377_`[\0\212\206\204\377\212\206\204\377\212\206\204\377\212\206"
  "\204\377\212\206\204\377\212\206\204\377\212\206\204\377\212\206\204"
  "\377\212\206\204\377\212\206\204\377\212\206\204\377\234\234\231\377"
  "_`[\0"};

static void walk_tree(xmlNodePtr cur, GtkTreeIter *parent);

void snippets_fill_tree_item_from_node(GtkTreeIter *iter, xmlNodePtr node) {
	gchar *title;
	gboolean recursive=FALSE;
	GdkPixbuf* pixmap=NULL;
	DEBUG_MSG("snippets_fill_tree_item_from_node, got node with type %s\n",node->name);
	title = (gchar *)xmlGetProp(node, (const xmlChar *)"title");
	DEBUG_MSG("snippets_fill_tree_item_from_node, node has title %s\n",title);
	if ((xmlStrEqual(node->name, (const xmlChar *)"branch"))) {
		pixmap = NULL;
		recursive = TRUE;
	} else /*if ((xmlStrEqual(cur->name, (const xmlChar *)"leaf")))*/ {
		xmlChar *type;
		type = xmlGetProp(node, (const xmlChar *)"type");
		if (xmlStrEqual(type, (const xmlChar *)"insert")) {
			pixmap = gdk_pixbuf_new_from_inline(-1, pixmap_insert, FALSE, NULL);
		} else if (xmlStrEqual(type, (const xmlChar *)"snr")) {
			pixmap = gdk_pixbuf_new_from_inline(-1, pixmap_snr, FALSE, NULL);
		}
		xmlFree(type);
	}
	gtk_tree_store_set(snippets_v.store, iter, PIXMAP_COLUMN, pixmap, TITLE_COLUMN, title,NODE_COLUMN, node,-1);
	if (pixmap) {
		g_object_unref(pixmap);
	}
	xmlFree(title);	
	if (recursive) {
		walk_tree(node, iter);
	}
	
}

static void walk_tree(xmlNodePtr cur, GtkTreeIter *parent) {
	DEBUG_MSG("walk_tree, started for children of %s\n",cur->name);
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		GtkTreeIter iter;
		DEBUG_MSG("walk_tree, found child with type %s\n",cur->name);
		if (xmlStrEqual(cur->name, (const xmlChar *)"branch") || xmlStrEqual(cur->name, (const xmlChar *)"leaf")) {
			/* do not add type text (all the xml spacing is type text )! */ 
			gtk_tree_store_append(snippets_v.store, &iter, parent);
			snippets_fill_tree_item_from_node(&iter, cur);
		}
		cur = cur->next;
	}
}

static gboolean snippets_load_finished_lcb(gpointer data) {
	xmlDocPtr doc = (xmlDocPtr)data;
	xmlNodePtr cur=NULL;
	DEBUG_MSG("snippets_load_finished_lcb, doc=%p, starting to load xml data into treestore\n", doc);
	if (doc) {
		cur = xmlDocGetRootElement(doc);
		if (cur) {
			if (xmlStrEqual(cur->name, (const xmlChar *) "snippets")) {
				snippets_v.doc = doc;
				walk_tree(cur, NULL);
				snippets_rebuild_accelerators();
				DEBUG_MSG("snippets_load_finished_lcb, finished walking tree\n");
				return FALSE;
			}
		}
		xmlFreeDoc(doc);
		doc = NULL;
	}
	if (doc == NULL) {
		snippets_v.doc = xmlNewDoc((const xmlChar *)"1.0");
		cur = xmlNewDocNode(snippets_v.doc,NULL, (const xmlChar *)"snippets",NULL);
		xmlDocSetRootElement(snippets_v.doc, cur);
		/* DEBUG_MSG("snippets_load_finished_lcb, loading from cmenu %p and %p\n",main_v->props.cmenu_insert, main_v->props.cmenu_replace); */
		/* now check if there is a custom menu  configuration. If there is one, parse it and build a xml tree */
/*		if (snippets_convert_cmenu(cur)) {
			walk_tree(cur, NULL);
		}*/
	}
	DEBUG_MSG("snippets_load_finished_lcb, finished empty tree\n");
	return FALSE;
}

static gchar *get_snipfile(gboolean forload) {
	GFile *uri;
	gchar *filename;
	uri = user_bfdir("snippets");
	filename = g_file_get_path(uri);
	g_object_unref(uri);
	if (forload) {
		/* if it does not exist, return PKGDATADIR"/snippets" so we start with the default file */
		uri = return_first_existing_filename(filename, PKGDATADIR"/snippets","data/snippets","../data/snippets",NULL);	
		g_free(filename);
		if (!uri) {
			return NULL;
		}
		filename = g_file_get_path(uri);
		g_object_unref(uri);
	}
	return filename;
}

static gpointer snippets_load_async(gpointer data) {
	gchar *filename = get_snipfile(TRUE);
	xmlDocPtr doc;
	if (!filename) {
		snippets_load_finished_lcb(NULL);
		return NULL;
	}
	DEBUG_MSG("snippets_load, filename=%s\n",filename);
	doc = xmlParseFile(filename);
	g_free(filename);
	g_idle_add_full(G_PRIORITY_LOW, snippets_load_finished_lcb, doc, NULL);	
	return NULL;
}

static gboolean snippets_load_low_priority(gpointer data) {
	g_thread_create(snippets_load_async, NULL, FALSE, NULL);
/*	g_print("snippets_load_low_priority\n");*/
	return FALSE;
}

void snippets_load(void) {
	/*g_thread_create(snippets_load_async, NULL, FALSE, NULL);*/
	g_idle_add_full(G_PRIORITY_LOW, snippets_load_low_priority, NULL, NULL);
}

gboolean snippets_store_lcb(gpointer data) {
	DEBUG_SIG("snippets_store_lcb, priority=%d\n",G_PRIORITY_DEFAULT_IDLE)
	DEBUG_MSG("snippets_store_lcb, started\n");
	if (snippets_v.doc) {
		gchar *snipfile = get_snipfile(FALSE);
		xmlSaveFormatFile(snipfile, snippets_v.doc, 1);
		g_free(snipfile);
	}
	return FALSE;
}

void reload_tree_from_doc(void) {
	if (snippets_v.doc) {
		xmlNodePtr cur = xmlDocGetRootElement(snippets_v.doc);
		if (cur) {
			if (xmlStrEqual(cur->name, (const xmlChar *) "snippets")) {
				/* the document is correct, first empty the treestore */
				gtk_tree_store_clear(snippets_v.store);
				
				/* now reload the tree */
				walk_tree(cur, NULL);
			}
		}
	}
}

void snippets_export_node(xmlNodePtr node, const gchar *filename) {
	xmlNodePtr newnode=NULL, cur=NULL;
	xmlDocPtr newdoc; 
	
	newdoc = xmlNewDoc((const xmlChar *)"1.0");
	cur = xmlNewDocNode(newdoc,NULL, (const xmlChar *)"snippets",NULL);
	xmlDocSetRootElement(newdoc, cur);
	
	newnode = xmlDocCopyNode(node, newdoc, TRUE);
	DEBUG_MSG("snippets_export_node, cur=%p, newdoc=%p, newnode=%p\n",cur,newdoc,newnode);
	xmlAddChild(cur, newnode);

	xmlSaveFormatFile(filename, newdoc, 1);
	xmlFreeDoc(newdoc);
}

typedef struct {
	gchar *filename;
	xmlDocPtr doc;
	xmlNodePtr parent;
} Tsnippets_import;

static gboolean snippets_import_load_finished_lcb(gpointer data) {
	Tsnippets_import *si = (Tsnippets_import *)data;

	if (si->doc) {
		xmlNodePtr cur;
		cur = xmlDocGetRootElement(si->doc);
		if (cur) {
			if (xmlStrEqual(cur->name, (const xmlChar *) "snippets")) {
				xmlNodePtr newnode;
				/* now import everything below */
				newnode = xmlDocCopyNodeList(snippets_v.doc, cur->children);
				xmlAddChildList(si->parent,newnode);
				reload_tree_from_doc();
				g_idle_add(snippets_store_lcb, NULL);
			}
		}
		xmlFreeDoc(si->doc);
	}
	g_free(si->filename);
	g_free(si);
	return FALSE;
}

static gpointer snippets_import_async(gpointer data) {
	Tsnippets_import *si = (Tsnippets_import *)data;
	
	si->doc = xmlParseFile(si->filename);
	g_idle_add_full(G_PRIORITY_LOW, snippets_import_load_finished_lcb, si, NULL);	
	return NULL;
}

void snippets_import_node(xmlNodePtr branch, const gchar *filename) {
	Tsnippets_import *si;
	
	si = g_new(Tsnippets_import,1);
	si->filename = g_strdup(filename);
	si->parent = branch;
	g_thread_create(snippets_import_async, si, FALSE, NULL);
}
