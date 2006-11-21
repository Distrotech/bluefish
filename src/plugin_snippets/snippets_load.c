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

I'll start implementing just the branches to get a feel for libXML, 
I want to parse something like this:
<snippets>
<branch title="myname">
	<branch title="child of myname">

	</branch> 
</branch> 
<branch title="myname2">

</branch> 
</snippets>
*/
#define DEBUG

#include <string.h>

#include "snippets.h"
#include "snippets_load.h"

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
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"branch"))) {
			title = xmlGetProp(cur, (const xmlChar *)"title");
			add_tree_item(parent, &iter, (const gchar *)title, cur);
			walk_tree(cur, &iter);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar *)"leaf"))) {
			title = xmlGetProp(cur, (const xmlChar *)"title");
			add_tree_item(parent, &iter, (const gchar *)title, cur);
		}
		cur = cur->next;
	}
}

void snippets_load(const gchar *filename) {
	xmlNodePtr cur=NULL;

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
	if (xmlStrcmp(cur->name, (const xmlChar *) "snippets")) {
		DEBUG_MSG("snippets_load, wrong type of document, root is called %s\n",cur->name);
		xmlFreeDoc(snippets_v.doc);
		snippets_v.doc = NULL;
		return;
	}
	
	walk_tree(cur, NULL);
}

