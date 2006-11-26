/* Bluefish HTML Editor
 * snippets_load.c - plugin for snippets sidebar
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
#define DEBUG

#include <string.h>

#include "snippets.h"
#include "snippets_load.h"
#include "../bf_lib.h"

static void add_tree_item(GtkTreeIter *parent, GtkTreeIter *child, const gchar *name, gpointer ptr) {
	DEBUG_MSG("add_tree_item, adding %s\n",name);
	gtk_tree_store_append(snippets_v.store, child, parent);
	gtk_tree_store_set(snippets_v.store, child, 0, name,1, ptr,-1);
}

static void walk_tree(xmlNodePtr cur, GtkTreeIter *parent) {
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		xmlChar *title;
		GtkTreeIter iter;
		if ((xmlStrEqual(cur->name, (const xmlChar *)"branch"))) {
			title = xmlGetProp(cur, (const xmlChar *)"title");
			add_tree_item(parent, &iter, (const gchar *)title, cur);
			walk_tree(cur, &iter);
		} else if ((xmlStrEqual(cur->name, (const xmlChar *)"leaf"))) {
			title = xmlGetProp(cur, (const xmlChar *)"title");
			add_tree_item(parent, &iter, (const gchar *)title, cur);
		}
		cur = cur->next;
	}
}

void snippets_load(const gchar *filename) {
	xmlNodePtr cur=NULL;
	DEBUG_MSG("snippets_load, filename=%s\n",filename);
	snippets_v.doc = xmlParseFile(filename);
	
	if (snippets_v.doc == NULL ) {
		DEBUG_MSG("snippets_load, parse error\n");
		return;
	}

	cur = xmlDocGetRootElement(snippets_v.doc);
	if (cur == NULL) {
		DEBUG_MSG("snippets_load, empty document\n");
		xmlFreeDoc(snippets_v.doc);
		snippets_v.doc = NULL;
		return;
	}
	if (!xmlStrEqual(cur->name, (const xmlChar *) "snippets")) {
		DEBUG_MSG("snippets_load, wrong type of document, root is called %s\n",cur->name);
		xmlFreeDoc(snippets_v.doc);
		snippets_v.doc = NULL;
		return;
	}
	
	walk_tree(cur, NULL);
}

void snippets_store(void) {
	gchar *snipfile = user_bfdir("snippets");
	xmlSaveFormatFile(snipfile, snippets_v.doc, 1);
	g_free(snipfile);
}
