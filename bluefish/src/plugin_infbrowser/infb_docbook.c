/* Bluefish HTML Editor
 * infb_docbook.c - plugin for information browser
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

#include "infb_docbook.h"
#include "infb_text.h"

xmlChar *infb_db_get_title(xmlDocPtr doc,gboolean subtitle,xmlNodePtr root) {
	xmlNodePtr an = NULL,node;
	gchar *path[3];
	gint i=0;
	xmlChar *text;
	
	if (!root)
		node = xmlDocGetRootElement(doc);
	else
		node = root;	
	if (subtitle) {
		path[0] = "info/subtitle"; /* v5 */
		path[1] = "bookinfo/subtitle"; /* v4 */
		path[2] = "subtitle"; /* inside book */
	} else {
		path[0] = "info/title"; /* v5 */
		path[1] = "bookinfo/title"; /* v4 */
		path[2] = "title"; /* inside book */	
	}	
	i = 1;
	an = getnode(doc,BAD_CAST path[0],node);
	while (!an && i<3) {
		an = getnode(doc,BAD_CAST path[i],node);
		i++;
	}	
	if (an) {
		text = xmlNodeGetContent(an);
		if (text) {
			return text;
		}
	}  
	return NULL;
}

xmlNodePtr infb_db_get_info_node(xmlDocPtr doc,xmlNodePtr root) {
	xmlNodePtr an = NULL;
	an = getnode(doc,BAD_CAST "info",root); /* v5 */
	if (!an) an = getnode(doc,BAD_CAST "bookinfo",root); /* v4 */
	return an;
}

static GList *infb_db_get_authors(xmlDocPtr doc,xmlNodePtr node) {
	GList *ret=NULL;
	xmlNodePtr auxn;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr result;
	xmlChar *text;
	gchar *str=NULL,*tofree;
	gint i; 

	auxn = getnode(doc,BAD_CAST "author",node);
	if (auxn) {
		auxn = getnode(doc,BAD_CAST "personname/firstname",node); /* v5 */
		if (!auxn)
			auxn = getnode(doc,BAD_CAST "firstname",node); /* v4 */	
		if (auxn) {
			text = xmlNodeGetContent(auxn);
			str = g_strdup((gchar*)text);
			xmlFree(text);
		}
		auxn = getnode(doc,BAD_CAST "personname/surname",node); /* v5 */
		if (!auxn)
			auxn = getnode(doc,BAD_CAST "surname",node); /* v4 */	
		if (auxn) {
			text = xmlNodeGetContent(auxn);
			if ( str ) {
				tofree = str;
				str = g_strconcat(str,(gchar*)text,NULL);
				g_free(tofree);
			} 
			else
				str = g_strdup((gchar*)text);
			xmlFree(text);
		}
		if (str) 
			ret = g_list_append(ret,str);			
	}
	else {
		result = getnodeset(doc, BAD_CAST "authorgroup/author",node);
		if (result) {
			nodeset = result->nodesetval;
			for(i=0;i<nodeset->nodeNr;i++) {
				auxn = getnode(doc,BAD_CAST "personname/firstname",nodeset->nodeTab[i]); /* v5 */
				if (!auxn)
					auxn = getnode(doc,BAD_CAST "firstname",nodeset->nodeTab[i]); /* v4 */	
				if (auxn) {
					text = xmlNodeGetContent(auxn);
					str = g_strdup((gchar*)text);
					xmlFree(text);
				}
				auxn = getnode(doc,BAD_CAST "personname/surname",nodeset->nodeTab[i]); /* v5 */
				if (!auxn)
					auxn = getnode(doc,BAD_CAST "surname",nodeset->nodeTab[i]); /* v4 */	
				if (auxn) {
					text = xmlNodeGetContent(auxn);
					if ( str ) {
						tofree = str;
						str = g_strconcat(str,(gchar*)text,NULL);
						g_free(tofree);
					} 
					else
						str = g_strdup((gchar*)text);
					xmlFree(text);
				}
				if (str) 
					ret = g_list_append(ret,str);								
			} /* set */
			xmlXPathFreeObject(result);
		}
	}
	return ret;
}


void infb_db_prepare_info(GtkTextView *view,xmlDocPtr doc, xmlNodePtr node) {
	GtkTextBuffer *buff = gtk_text_view_get_buffer(view);
	/*xmlChar *text;*/
	GList *lst=NULL,*p;
	
	if (!node) return;
	infb_insert_section(buff,BAD_CAST "Authors");
	lst = infb_db_get_authors(doc,node);
	p = lst;
	while (p) {
		infb_insert_line(buff,BAD_CAST p->data,NULL);
		p = g_list_next(p);
	}
}


void infb_db_format_element(GtkTextView *view,xmlDocPtr doc, xmlNodePtr node) {
	GtkTextBuffer *buff = gtk_text_view_get_buffer(view);
	xmlChar *text;
	 
	if (xmlStrcmp(node->name,BAD_CAST "command")==0 ||
		 xmlStrcmp(node->name,BAD_CAST "option")==0 ||
		 xmlStrcmp(node->name,BAD_CAST "type")==0 ||
		 xmlStrcmp(node->name,BAD_CAST "emphasis")==0 ) {
		text = xmlNodeGetContent(node);
		if (text) {
			infb_insert_italic(buff,text);
			xmlFree(text);
		}
	}
	else 	if (xmlStrcmp(node->name,BAD_CAST "application")==0 ||
		 		 xmlStrcmp(node->name,BAD_CAST "userinput")==0 ) 
	{
		text = xmlNodeGetContent(node);
		if (text) {
			infb_insert_bold(buff,text);
			xmlFree(text);
		}
	}
	else if (xmlStrcmp(node->name,BAD_CAST "title")!=0 &&
				xmlStrcmp(node->name,BAD_CAST "subtitle")!=0 &&
				xmlStrcmp(node->name,BAD_CAST "refpurpose")!=0 &&	
				xmlStrcmp(node->name,BAD_CAST "refname")!=0 ) {
		text = xmlNodeGetContent(node);
		if (text) {
			infb_insert_text(buff,text);
			xmlFree(text);
		}
	}
}


