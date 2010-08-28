/* Bluefish HTML Editor
 * infb_docbook.c - plugin for information browser
 *
 * Copyright (C) 2007 Oskar Świda
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

#include "infb_docbook.h"
#include "infb_text.h"


xmlChar *infb_db_get_title(xmlDocPtr doc,gboolean subtitle,xmlNodePtr root) {
	xmlNodePtr an = NULL,node;
	gchar *path[4];
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
		path[3] = "refnamediv/refname"; /* refentry */
	} else {
		path[0] = "info/title"; /* v5 */
		path[1] = "bookinfo/title"; /* v4 */
		path[2] = "title"; /* inside book */	
		path[3] = "refnamediv/refname"; /* refentry */
	}	
	i = 1;
	an = getnode(doc,BAD_CAST path[0],node);
	while (!an && i<4) {
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
	infb_insert_text(buff,BAD_CAST "Authors",INFB_TT_SECTION,TRUE);
	lst = infb_db_get_authors(doc,node);
	p = lst;
	while (p) {
		infb_insert_text(buff,BAD_CAST p->data,INFB_TT_NONE,TRUE);
		p = g_list_next(p);
	}
}


void infb_db_format_element(GtkTextView *view,xmlDocPtr doc, xmlNodePtr node) {
	GtkTextBuffer *buff = gtk_text_view_get_buffer(view);
	xmlChar *text;
	 
	if (xmlStrcmp(node->name,BAD_CAST "command")==0 ||
		 xmlStrcmp(node->name,BAD_CAST "option")==0 ||
		 xmlStrcmp(node->name,BAD_CAST "type")==0 ||
		 xmlStrcmp(node->name,BAD_CAST "function")==0 ||
		 xmlStrcmp(node->name,BAD_CAST "emphasis")==0 ) {
		text = xmlNodeGetContent(node);
		if (text) {
			infb_insert_text(buff,text,INFB_TT_ITALIC,FALSE);
			xmlFree(text);
		}
	}
	else 	if (xmlStrcmp(node->name,BAD_CAST "application")==0 ||
				 xmlStrcmp(node->name,BAD_CAST "primary")==0 ||
				 xmlStrcmp(node->name,BAD_CAST "parameter")==0 ||
		 		 xmlStrcmp(node->name,BAD_CAST "userinput")==0 ) 
	{
		text = xmlNodeGetContent(node);
		if (text) {
			infb_insert_text(buff,text,INFB_TT_BOLD,FALSE);
			xmlFree(text);
		}
	}
	else 	if (xmlStrcmp(node->name,BAD_CAST "programlisting")==0 ) 
	{
		text = xmlNodeGetContent(node);
		if (text) {
			infb_insert_text(buff,text,INFB_TT_SMALL,TRUE);
			xmlFree(text);
		}
	}
	else if ( (node->type==XML_ELEMENT_NODE || node->type==XML_TEXT_NODE) &&
				xmlStrcmp(node->name,BAD_CAST "title")!=0 &&
				xmlStrcmp(node->name,BAD_CAST "subtitle")!=0 &&
				xmlStrcmp(node->name,BAD_CAST "refpurpose")!=0 &&	
				xmlStrcmp(node->name,BAD_CAST "refname")!=0 ) {
		text = xmlNodeGetContent(node);
		if (text) {
			infb_insert_text(buff,text,INFB_TT_NONE,FALSE);
			xmlFree(text);
		}
	}
}


void infb_db_fill_node(GtkTextView *view,xmlDocPtr doc,xmlNodePtr node,gint level) {
	xmlNodePtr an;
	GtkTextBuffer *buff = gtk_text_view_get_buffer(view);
	xmlChar *text;
	xmlNodePtr auxn;
	gchar *levstr=NULL;
	if ( level > 0 ) 
		levstr = g_strnfill(2*level,' ');
	else
		levstr = "";
			
	if ( xmlStrcmp(node->name,BAD_CAST "book") ==0 /*||
				  xmlStrcmp(node->name,BAD_CAST "part") ==0*/	) {
				text = infb_db_get_title(doc,FALSE,node);
				if (text) {
					infb_insert_text(buff,text,INFB_TT_TITLE,TRUE);
					xmlFree(text);
				}
				text = infb_db_get_title(doc,TRUE,node);
				if (text) {
					infb_insert_text(buff,text,INFB_TT_DESC,TRUE);
					xmlFree(text);
				}
				
				auxn = node->xmlChildrenNode;
				while (auxn) {
					infb_db_fill_node(view,doc,auxn,level+1);				
					auxn = auxn->next;
				}				
									
			}
			else if ( xmlStrcmp(node->name,BAD_CAST "bookinfo") ==0 || 
						 xmlStrcmp(node->name,BAD_CAST "info") ==0) 
			{
			  if (level==0)
				infb_db_prepare_info(view,doc,node);
			  else { 
					infb_insert_icon(view,gtk_image_new_from_stock(GTK_STOCK_EDIT,GTK_ICON_SIZE_MENU),levstr);
					infb_insert_node(buff,BAD_CAST _("Info"),node,TRUE);											
			  } 		
			}
			else if ( xmlStrcmp(node->name,BAD_CAST "itemizedlist") ==0 ||
						 xmlStrcmp(node->name,BAD_CAST "orderedlist") ==0 ||
						 xmlStrcmp(node->name,BAD_CAST "segmentedlist") ==0 ||
						 xmlStrcmp(node->name,BAD_CAST "procedure") ==0 ) {
				auxn = node->xmlChildrenNode;
				while (auxn) {
					infb_insert_text(buff,BAD_CAST "•",INFB_TT_NONE,FALSE);
					infb_db_fill_node(view,doc,auxn,level+1);
					auxn = auxn->next;
				}				
			}			
			else  if ( xmlStrcmp(node->name,BAD_CAST "simpara") ==0 ) {
				text = xmlNodeGetContent(node);
				if (text) {
					infb_insert_text(buff,text,INFB_TT_NONE,TRUE);
					xmlFree(text);
				}
			}
			else  if ( xmlStrcmp(node->name,BAD_CAST "formalpara") ==0  ||
						  xmlStrcmp(node->name,BAD_CAST "term") ==0  ||
						  xmlStrcmp(node->name,BAD_CAST "indexterm") ==0  ||
						  xmlStrcmp(node->name,BAD_CAST "synopsis") ==0 ||	
						  xmlStrcmp(node->name,BAD_CAST "listitem") ==0  ||
						  xmlStrcmp(node->name,BAD_CAST "seglistitem") ==0  ||
						  xmlStrcmp(node->name,BAD_CAST "seg") ==0  ||
						  xmlStrcmp(node->name,BAD_CAST "varlistentry") ==0  ||					  			   	
						  xmlStrcmp(node->name,BAD_CAST "para") ==0) {
				an = getnode(doc,BAD_CAST "title",node);
				if (an) {
					text = xmlNodeGetContent(an);
					if (text) {
						infb_insert_text(buff,text,INFB_TT_SECTION,TRUE);
						xmlFree(text);
					}
				}
				auxn = node->xmlChildrenNode;
				while ( auxn ) {
					infb_db_fill_node(view,doc,auxn,level+1);						
					auxn = auxn->next;
				} 							
				infb_insert_text(buff,BAD_CAST "",INFB_TT_NONE,TRUE);
			}			
			else  if ( xmlStrcmp(node->name,BAD_CAST "refentry") ==0 ) {
				if (level==0) 
				{
					an = getnode(doc,BAD_CAST "refnamediv/refname",node);
					if (an) {
						text = xmlNodeGetContent(an);
						if (text) {
							infb_insert_text(buff,text,INFB_TT_TITLE,TRUE);
							xmlFree(text);
						}
					}
					an = getnode(doc,BAD_CAST "refnamediv/refpurpose",node);
					if (an) {
						text = xmlNodeGetContent(an);
						if (text) {
							infb_insert_text(buff,text,INFB_TT_DESC,TRUE);
							xmlFree(text);
						}
					}
					auxn = node->xmlChildrenNode;
					while ( auxn ) {
						infb_db_fill_node(view,doc,auxn,level+1);						
						auxn = auxn->next;
					} 						
				} 
				else
				{
			 		an = getnode(doc,BAD_CAST "refnamediv/refname",node);
					if (an) {
						text = xmlNodeGetContent(an);
						if (text) {
							infb_insert_icon(view,gtk_image_new_from_stock(GTK_STOCK_ABOUT,GTK_ICON_SIZE_MENU),NULL);
							infb_insert_node(buff,text,node,TRUE);
							xmlFree(text);
						}
					}				
				}
			}
			else  if ( xmlStrcmp(node->name,BAD_CAST "link") ==0 ) {
				gchar *pstr;
				xmlChar *text2;
				text = xmlGetProp(node,BAD_CAST "linkend");
				if (text) {
					pstr = g_strdup_printf("//refentry[@id=\"%s\"]",(gchar*)text);
					auxn = getnode(doc,BAD_CAST pstr,NULL);
					if (auxn) {
						text2 = xmlNodeGetContent(node);
						if (text2) {
							infb_insert_node(buff,text2,auxn,FALSE);
							xmlFree(text2);
						} 
					} else 
					{
						text2 = xmlNodeGetContent(node);
						if (text2) {
							infb_insert_text(buff,text2,INFB_TT_NONE,FALSE);
							xmlFree(text2);
						}	
					}	
					xmlFree(text);
					g_free(pstr);
				}
			}
			else  if ( xmlStrcmp(node->name,BAD_CAST "sect1") ==0 ||
						  xmlStrcmp(node->name,BAD_CAST "sect2") ==0	||
						  xmlStrcmp(node->name,BAD_CAST "sect3") ==0 ||
						  xmlStrcmp(node->name,BAD_CAST "sect4") ==0 ||
						  xmlStrcmp(node->name,BAD_CAST "sect5") ==0 ||
						  xmlStrcmp(node->name,BAD_CAST "section") ==0 ||
						  xmlStrcmp(node->name,BAD_CAST "refsect3") ==0 ||
						  xmlStrcmp(node->name,BAD_CAST "refsect2") ==0 ||
						  xmlStrcmp(node->name,BAD_CAST "refsection") ==0 ||
						  xmlStrcmp(node->name,BAD_CAST "refsynopsisdiv") ==0 ||
						  xmlStrcmp(node->name,BAD_CAST "refsect1") ==0 ||
						  xmlStrcmp(node->name,BAD_CAST "chapter") ==0 ||
						  xmlStrcmp(node->name,BAD_CAST "article") ==0 ||	
						  xmlStrcmp(node->name,BAD_CAST "preface") ==0 ||
						  xmlStrcmp(node->name,BAD_CAST "abstract") ==0 ||
						  xmlStrcmp(node->name,BAD_CAST "appendix") ==0 ||
						  xmlStrcmp(node->name,BAD_CAST "partintro") ==0 ||
						  xmlStrcmp(node->name,BAD_CAST "part") ==0 ||
						  xmlStrcmp(node->name,BAD_CAST "step") ==0 ||
						  xmlStrcmp(node->name,BAD_CAST "note") ==0 ||
						  xmlStrcmp(node->name,BAD_CAST "variablelist") ==0  						   
						) 
			{
			 if ( level == 0 ) {
				an = getnode(doc,BAD_CAST "child::title",node);
				if (an) {
					text = xmlNodeGetContent(an);
					if (text) {
						infb_insert_text(buff,text,INFB_TT_TITLE,TRUE);
						xmlFree(text);
					}
				}
				an = getnode(doc,BAD_CAST "child::subtitle",node);
				if (an) {
					text = xmlNodeGetContent(an);
					if (text) {
						infb_insert_text(buff,text,INFB_TT_DESC,TRUE);
						xmlFree(text);
					}
				}				
				auxn = node->xmlChildrenNode;
				while ( auxn ) {
					infb_db_fill_node(view,doc,auxn,level+1);
					auxn = auxn->next;
				} 												
			 }
			 else {
			 		an = getnode(doc,BAD_CAST "title",node);
					if (an) {
						text = xmlNodeGetContent(an);
						if (text) {
							infb_insert_icon(view,gtk_image_new_from_stock(GTK_STOCK_ABOUT,GTK_ICON_SIZE_MENU),levstr);
							infb_insert_node(buff,text,node,TRUE);
							xmlFree(text);
						} else {
							infb_insert_icon(view,gtk_image_new_from_stock(GTK_STOCK_ABOUT,GTK_ICON_SIZE_MENU),levstr);
							infb_insert_node(buff,BAD_CAST node->name,node,TRUE);					
						}
					}
			  }
			 } /*sections */
			 else  if ( xmlStrcmp(node->name,BAD_CAST "refmeta") ==0 ||
			 				xmlStrcmp(node->name,BAD_CAST "glossary") ==0 ||
			 				xmlStrcmp(node->name,BAD_CAST "refnamediv") ==0 
			 				) {
			 	/* not shown */
			 }
			 else { /* other elements */
			 	infb_db_format_element(view,doc,node);
			 }						 
}





