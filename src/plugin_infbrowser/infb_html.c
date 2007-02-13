/* Bluefish HTML Editor
 * infb_html.c - plugin for information browser
 *
 * Copyright (C) 2007 Oskar Świda
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

#include "infb_html.h"
#include "infbrowser.h"
#include "infb_text.h"

xmlChar *infb_html_get_title(xmlDocPtr doc) {
	xmlNodePtr an = NULL,node;
	xmlChar *text;
	
	node = xmlDocGetRootElement(doc);
	an = getnode(doc,BAD_CAST "head/title",node);
	g_print("html title %p\n",an);
	if (an) {
		text = xmlNodeGetContent(an);
		if (text) {
			return text;
		}
	}  
	return NULL;
}



void infb_html_fill_node(GtkTextView *view,xmlDocPtr doc,xmlNodePtr node,gint level) {
	xmlNodePtr auxn;
	GtkTextBuffer *buff = gtk_text_view_get_buffer(view);
	xmlChar *text;
	
	if (xmlStrcmp(node->name,BAD_CAST "body")==0) {
		auxn = node->xmlChildrenNode;
		while (auxn) {
			infb_html_fill_node(view,doc,auxn,level+1);				
			auxn = auxn->next;
		}		
	}
	else if (xmlStrcmp(node->name,BAD_CAST "div")==0) {
		auxn = node->xmlChildrenNode;
		while (auxn) {
			infb_html_fill_node(view,doc,auxn,level+1);				
			auxn = auxn->next;
		}
		infb_insert_text(buff,BAD_CAST "",INFB_TT_NONE,TRUE);		
	}	
	else if (xmlStrcmp(node->name,BAD_CAST "br")==0) {
		infb_insert_text(buff,BAD_CAST "",INFB_TT_NONE,TRUE);
	}
	else if (xmlStrcmp(node->name,BAD_CAST "tr")==0) {
		auxn = node->xmlChildrenNode;
		while (auxn) {
			if (xmlStrcmp(auxn->name,BAD_CAST "td")==0) {
				text = xmlNodeGetContent(auxn);
				if (text) {
					infb_insert_label(view,text);
					xmlFree(text);
				}	
			}					
			auxn = auxn->next;
		}			
		infb_insert_text(buff,BAD_CAST "",INFB_TT_NONE,TRUE);
	}	
	else {
		text = xmlNodeGetContent(node);
		if (text) {
			infb_insert_text(buff,text,INFB_TT_NONE,FALSE);
			xmlFree(text);
		}	
	}
}


