/* Bluefish HTML Editor
 * snippets_convert_cmenu.c - convert custom menu
 *
 * Copyright (C) 2007 Olivier Sessink
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

#include "snippets.h"
#include "../stringlist.h"
#include "../rcfile.h"
#include "snippets_convert_cmenu.h"

static xmlNodePtr get_child_by_title(xmlNodePtr node, const xmlChar *title) {
	xmlNodePtr cur;
	cur = node->xmlChildrenNode;
	while (cur != NULL) {
		xmlChar *tmp = xmlGetProp(cur, (const xmlChar *)"title");
		if (xmlStrEqual(tmp, title)) {
			return cur;		
		}
		cur = cur->next;
	}
	return NULL;
}

static xmlNodePtr create_path(xmlNodePtr rootnode, const gchar *path) {
	gchar **splitted, **tmp;
	xmlNodePtr cur=rootnode, tmpnode;
	gboolean nodemightexist = TRUE;
	/* split the path */
	DEBUG_MSG("create_path, started with '%s'\n",path);
	tmp = splitted = g_strsplit(path,"/",-1);
	while (*tmp != NULL) {
		if (*tmp[0] != '\0') {
			gchar *title = g_strdup(*tmp);
			title = strip_any_whitespace(title);
			if (nodemightexist) {
				DEBUG_MSG("create_path, searching for %s\n",title);
				tmpnode = get_child_by_title(cur, (const xmlChar *)title);
				if (tmpnode) {
					cur = tmpnode;
				} else {
					nodemightexist = FALSE;
				}
			} 
			if (! nodemightexist) {
				if (*(tmp+1)!=NULL) {
					DEBUG_MSG("create_path, adding branch %s to xml tree\n",title);
					tmpnode = xmlNewChild(cur,NULL,(const xmlChar *)"branch",NULL);	
					xmlSetProp(tmpnode, (const xmlChar *)"title", (const xmlChar *)title);
				} else {
					DEBUG_MSG("create_path, adding leaf %s to xml tree\n",title);
					tmpnode = xmlNewChild(cur,NULL,(const xmlChar *)"leaf",NULL);	
					xmlSetProp(tmpnode, (const xmlChar *)"title", (const xmlChar *)title);
				}
				cur = tmpnode;
			}
			g_free(title);
		}
		tmp++;
	}
	g_strfreev(splitted);
	return cur;
}
/*
instead of having one list where both insert and replace types have their
place, I changed that to 2 arraylists:
main_v->props.cmenu_insert
main_v->props.cmenu_replace

** for insert **
array[0] = title / menupath
array[1] = formatstring before, containing %0, %1... that should be replaced by the 
				values from the dialog
array[2] = formatstring after
array[3] = number of variables from the dialog
array[4..] = the description of those variables

** for replace **
array[0] = title / menupath
array[1] = search pattern, containing %0 etc.
array[2] = replace pattern, containing %0 etc.
array[3] = replace where:
							0 = from beginning
							1 = from cursor
							2 = selection (selection required)
							3 = all open documents
							4 = ask
array[4] = replace type:
							0 = normal
							1 = regular expression
array[5] = case sensitivity:
							0 = no
							1 = yes
array[6] = number of variables from the dialog
array[7..] = the description of those variables
*/

static void add_params(xmlNodePtr node, gint num, const gchar **arr) {
	int i;
	DEBUG_MSG("add_params, adding %d params\n",num);
	for (i=0;i<num;i++) {
		xmlNodePtr tmpn;
		if (!arr[i]) {
			DEBUG_MSG("add_params, param %d does not exist???\n",i);
			return;
		}
		DEBUG_MSG("add_params, adding %s\n",arr[i]);
		tmpn = xmlNewChild(node,NULL,(const xmlChar *)"param", NULL);
		xmlSetProp(tmpn, (const xmlChar *)"name", (const xmlChar *)arr[i]);
	}
}

static void fill_leaf_insert(xmlNodePtr node, const gchar **arr) {
	gint num;
	xmlNodePtr tmpn;
	xmlSetProp(node, (const xmlChar *)"type", (const xmlChar *)"insert");
	tmpn = xmlNewTextChild(node,NULL,(const xmlChar *)"before",(const xmlChar *)arr[1]);
	tmpn = xmlNewTextChild(node,NULL,(const xmlChar *)"after",(const xmlChar *)arr[2]);
	
	num = atoi(arr[3]);
	add_params(node, num, arr+4);
}

static void fill_leaf_snr(xmlNodePtr node, const gchar **arr) {
	gint num;
	gchar *tmp;
	xmlNodePtr tmpn;
	xmlSetProp(node, (const xmlChar *)"type", (const xmlChar *)"snr");
	tmpn = xmlNewTextChild(node,NULL,(const xmlChar *)"searchpat",(const xmlChar *)arr[1]);
	tmpn = xmlNewTextChild(node,NULL,(const xmlChar *)"replacepat",(const xmlChar *)arr[2]);
	switch (atoi(arr[3])) {
		case 1:
			tmp = "c";
		break;
		case 2:
			tmp = "s";
		break;
		case 3:
			tmp = "a";
		break;
		default:
			tmp = "b";
		break;
	}
	xmlSetProp(node, (const xmlChar *)"region", (const xmlChar *) tmp);
	switch (atoi(arr[4])) {
		case 1:
			tmp = "posix";
		break;
		case 2:
			tmp = "perl";
		break;
		default:
			tmp = "normal";
		break;

	}	
	xmlSetProp(node, (const xmlChar *)"matchtype", (const xmlChar *) tmp);
	xmlSetProp(node, (const xmlChar *)"casesens", (arr[5] && arr[5][0] =='1' ) ? (const xmlChar *)"1": (const xmlChar *)"0");
	xmlSetProp(node, (const xmlChar *)"escapechars", (const xmlChar *)"0");
	
	num = atoi(arr[6]);
	add_params(node, num, arr+7);
}

gboolean snippets_convert_cmenu(xmlNodePtr rootnode) {
	GList *tmplist;
	const gchar **splittedstring;
	gint count2;

	rcfile_parse_custom_menu();
	
	if (!main_v->props.cmenu_insert && !main_v->props.cmenu_replace) {
		return FALSE;
	}

	tmplist = g_list_first(main_v->props.cmenu_insert);
	while (tmplist) {
		splittedstring = (const gchar **) tmplist->data;
		count2 = count_array((gchar **)splittedstring);
		if (count2 >= 4) {
			xmlNodePtr node;
			node = create_path(rootnode, splittedstring[0]);
			fill_leaf_insert(node, splittedstring);
		}
		tmplist = g_list_next(tmplist);
	}
	
	tmplist = g_list_first(main_v->props.cmenu_replace);
	while (tmplist) {
		splittedstring = (const gchar **) tmplist->data;
		count2 = count_array((gchar **)splittedstring);
		if (count2 >= 4) {
			xmlNodePtr node;
			node = create_path(rootnode, splittedstring[0]);
			fill_leaf_snr(node, splittedstring);
		}
		tmplist = g_list_next(tmplist);
	}
	free_arraylist(main_v->props.cmenu_insert);
	free_arraylist(main_v->props.cmenu_replace);
	main_v->props.cmenu_insert = NULL;
	main_v->props.cmenu_replace = NULL;
	return TRUE;
}
