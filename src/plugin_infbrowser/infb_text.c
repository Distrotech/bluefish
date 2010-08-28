/* Bluefish HTML Editor
 * infb_text.c - plugin for information browser
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

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/hash.h>
#include <libxml/xmlschemas.h>
#include <libxml/schemasInternals.h>
#include <libxml/tree.h>
#include <string.h>
#include "infb_text.h"
#include "infbrowser.h"
#include "infb_docbook.h"
#include "infb_html.h"


void infb_set_current_type(xmlDocPtr doc) {
	xmlChar *txt;
	xmlNodePtr root;
	infb_v.currentType = INFB_DOCTYPE_UNKNOWN;
	root = xmlDocGetRootElement(doc);
	if ( root && xmlStrcmp(root->name,BAD_CAST "ref")== 0 )
	{
		txt = xmlGetProp(root,BAD_CAST "type");
		if (!txt) infb_v.currentType = INFB_DOCTYPE_FREF2;
		else {
			if ( xmlStrcmp(txt,BAD_CAST "dtd")== 0 )
				infb_v.currentType = INFB_DOCTYPE_DTD;
			else if ( xmlStrcmp(txt,BAD_CAST "index")== 0 )
				infb_v.currentType = INFB_DOCTYPE_INDEX;
			else 
				infb_v.currentType = INFB_DOCTYPE_FREF2;					
			xmlFree(txt);
		}
	}	
	else if ( root && xmlStrcmp(root->name,BAD_CAST "book")== 0 ) 
		infb_v.currentType = INFB_DOCTYPE_DOCBOOK;
	else if ( root && xmlStrcmp(root->name,BAD_CAST "html")== 0 ) 
		infb_v.currentType = INFB_DOCTYPE_HTML;
		
} 

xmlXPathObjectPtr getnodeset (xmlDocPtr doc, xmlChar *xpath,xmlNodePtr start){
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;
	context = xmlXPathNewContext(doc);
	if (context == NULL) return NULL;
	if (start!=NULL)
		context->node = start;
	else 
		context->node = xmlDocGetRootElement(doc);	
	result = xmlXPathEvalExpression(xpath, context);
	xmlXPathFreeContext(context);
	if (result == NULL) return NULL;
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
		xmlXPathFreeObject(result);
   	return NULL;
	}
	return result;
}

xmlNodePtr getnode (xmlDocPtr doc, xmlChar *xpath,xmlNodePtr start){
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;
	xmlNodePtr ret=NULL;
	
	context = xmlXPathNewContext(doc);
	if (context == NULL) return NULL;
	if (start!=NULL)
		context->node = start;
	else
		context->node = xmlDocGetRootElement(doc);
	result = xmlXPathEvalExpression(xpath, context);
	xmlXPathFreeContext(context);
	if (result == NULL) return NULL;
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
		xmlXPathFreeObject(result);
   	return NULL;
	}
	if ( result->nodesetval->nodeNr > 0 )
	{
		ret = result->nodesetval->nodeTab[0];
		xmlXPathFreeObject(result);
	}
	return ret;
}

gint getcount (xmlDocPtr doc, xmlChar *xpath,xmlNodePtr start) {
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;
	gint ret=0;
	
	context = xmlXPathNewContext(doc);
	if (context == NULL) return 0;
	if (start!=NULL)
		context->node = start;
	else
		context->node = xmlDocGetRootElement(doc);
	result = xmlXPathEvalExpression(BAD_CAST xpath, context);
	xmlXPathFreeContext(context);
	if (result == NULL) return 0;
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
		xmlXPathFreeObject(result);
   	return 0;
	}	
	ret = result->nodesetval->nodeNr;
	xmlXPathFreeObject(result);
	return ret;
}


void infb_insert_message(GtkTextView *view, xmlChar *text) {
	GtkTextTag *tag;
	GtkTextIter iter,it1,it2;
	GtkTextBuffer *buff = gtk_text_view_get_buffer(view);	
	
	gtk_text_buffer_get_bounds(buff,&it1,&it2);
	gtk_text_buffer_remove_all_tags (buff,&it1,&it2);
	gtk_text_buffer_delete(buff,&it1,&it2);
	gtk_text_buffer_insert_at_cursor(buff,"\n\n",2);	
	tag = gtk_text_buffer_create_tag(buff,NULL,INFB_STYLE_MSG,NULL);
	gtk_text_buffer_get_iter_at_mark (buff,&iter,gtk_text_buffer_get_insert(buff));
	gtk_text_buffer_insert_with_tags(buff,&iter,(const gchar*)text,xmlStrlen(text),tag,NULL);
	gtk_widget_queue_draw(GTK_WIDGET(view));
	gdk_window_process_all_updates();
		
}

void infb_insert_error(GtkTextView *view, xmlChar *text) {
	GtkTextTag *tag;
	GtkTextIter iter,it1,it2;
	GtkTextBuffer *buff = gtk_text_view_get_buffer(view);	
	
	gtk_text_buffer_get_bounds(buff,&it1,&it2);
	gtk_text_buffer_remove_all_tags (buff,&it1,&it2);
	gtk_text_buffer_delete(buff,&it1,&it2);	
	gtk_text_buffer_insert_at_cursor(buff,"\n\n",2);
	tag = gtk_text_buffer_create_tag(buff,NULL,INFB_STYLE_ERR,NULL);
	gtk_text_buffer_get_iter_at_mark (buff,&iter,gtk_text_buffer_get_insert(buff));
	gtk_text_buffer_insert_with_tags(buff,&iter,(const gchar*)text,xmlStrlen(text),tag,NULL);
	gtk_widget_queue_draw(GTK_WIDGET(view));
	gdk_window_process_all_updates();
		
}


void infb_insert_text_tag(GtkTextBuffer *buff, xmlChar *text, GtkTextTag *tag, gboolean eol) {
	GtkTextIter iter;
	if (!text || !tag) return;
	gtk_text_buffer_get_iter_at_mark (buff,&iter,gtk_text_buffer_get_insert(buff));
	gtk_text_buffer_insert_with_tags(buff,&iter,(const gchar*)text,xmlStrlen(text),tag,NULL);
	if (eol)
		gtk_text_buffer_insert_at_cursor(buff,"\n",1);
}

void infb_insert_text(GtkTextBuffer *buff, xmlChar *text, gint type, gboolean eol) {
	GtkTextTag *tag;
	GtkTextIter iter;
	if (!text) return;
	switch (type) {
		case INFB_TT_SMALL:
				tag = gtk_text_buffer_create_tag(buff,NULL,INFB_STYLE_SMALL,NULL);
				gtk_text_buffer_get_iter_at_mark (buff,&iter,gtk_text_buffer_get_insert(buff));
				gtk_text_buffer_insert_with_tags(buff,&iter,(const gchar*)text,xmlStrlen(text),tag,NULL);
		break;
		case INFB_TT_BOLD:
				tag = gtk_text_buffer_create_tag(buff,NULL,INFB_STYLE_BOLD,NULL);
				gtk_text_buffer_get_iter_at_mark (buff,&iter,gtk_text_buffer_get_insert(buff));
				gtk_text_buffer_insert_with_tags(buff,&iter,(const gchar*)text,xmlStrlen(text),tag,NULL);
		break;				
		case INFB_TT_ITALIC:
				tag = gtk_text_buffer_create_tag(buff,NULL,INFB_STYLE_ITALIC,NULL);
				gtk_text_buffer_get_iter_at_mark (buff,&iter,gtk_text_buffer_get_insert(buff));
				gtk_text_buffer_insert_with_tags(buff,&iter,(const gchar*)text,xmlStrlen(text),tag,NULL);
		break;				
		case INFB_TT_TITLE:
				tag = gtk_text_buffer_create_tag(buff,NULL,INFB_STYLE_TITLE,NULL);
				gtk_text_buffer_get_iter_at_mark (buff,&iter,gtk_text_buffer_get_insert(buff));
				gtk_text_buffer_insert_with_tags(buff,&iter,(const gchar*)text,xmlStrlen(text),tag,NULL);
		break;				
		case INFB_TT_DESC:
				tag = gtk_text_buffer_create_tag(buff,NULL,INFB_STYLE_DESC,NULL);
				gtk_text_buffer_get_iter_at_mark (buff,&iter,gtk_text_buffer_get_insert(buff));
				gtk_text_buffer_insert_with_tags(buff,&iter,(const gchar*)text,xmlStrlen(text),tag,NULL);
		break;				
		case INFB_TT_SECTION:
				tag = gtk_text_buffer_create_tag(buff,NULL,INFB_STYLE_SECTION,NULL);
				gtk_text_buffer_get_iter_at_mark (buff,&iter,gtk_text_buffer_get_insert(buff));
				gtk_text_buffer_insert_with_tags(buff,&iter,(const gchar*)text,xmlStrlen(text),tag,NULL);
		break;										
		default:
			gtk_text_buffer_insert_at_cursor(buff,(const gchar*)text,xmlStrlen(text));			
	}	
	if (eol)
		gtk_text_buffer_insert_at_cursor(buff,"\n",1);
}


void infb_insert_icon(GtkTextView *view, GtkWidget *icon, gchar *prepend) {
	GtkTextChildAnchor *anchor;
	GtkTextIter iter;
	GtkTextBuffer *buff = gtk_text_view_get_buffer(view);
	if ( prepend )
		gtk_text_buffer_insert_at_cursor(buff,prepend,-1);	
	gtk_text_buffer_get_iter_at_mark (buff,&iter,gtk_text_buffer_get_insert(buff));
	anchor = gtk_text_buffer_create_child_anchor (buff, &iter);
	gtk_text_buffer_insert_at_cursor(buff," ",1);
	gtk_text_view_add_child_at_anchor (view,icon,anchor);
	gtk_widget_show_all(icon);									
}

void infb_insert_anchor(GtkTextView *view, xmlChar *text) {
	GtkTextIter iter;
	GtkTextBuffer *buff = gtk_text_view_get_buffer(view);
	if (!text) return;
	gtk_text_buffer_get_iter_at_mark (buff,&iter,gtk_text_buffer_get_insert(buff));
	gtk_text_buffer_create_mark(buff,(gchar*)text,&iter,TRUE);
}

void infb_insert_widget(GtkTextView *view, GtkWidget *widget,gint size) {
	GtkTextChildAnchor *anchor;
	GtkTextBuffer *buff = gtk_text_view_get_buffer(view);
	GtkTextIter iter;
	GdkColor gc;
	
	gdk_color_parse("#FF0000",&gc);
	gtk_widget_modify_bg(widget,GTK_STATE_NORMAL,&gc);
	
/*	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 2, 2);*/
	if (size>0)
		gtk_widget_set_size_request(widget,size,-1);
	/*gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
	gtk_label_set_line_wrap(GTK_LABEL(label),PANGO_WRAP_CHAR);*/ 
	gtk_text_buffer_get_iter_at_mark (buff,&iter,gtk_text_buffer_get_insert(buff));
	anchor = gtk_text_buffer_create_child_anchor (buff, &iter);
	gtk_text_view_add_child_at_anchor (view,widget,anchor);
	gtk_widget_show_all(widget);									
}

void infb_insert_fileref(GtkTextBuffer *buff, xmlChar *text, xmlChar *fname,xmlChar *desc) {
	GtkTextTag *tag;
	GtkTextIter iter;
	if (!text) return;
	gtk_text_buffer_get_iter_at_mark (buff,&iter,gtk_text_buffer_get_insert(buff));	
	tag = gtk_text_buffer_create_tag(buff,NULL,INFB_STYLE_FILEREF,NULL);
	g_object_set_data (G_OBJECT (tag), "type", &infb_v.nt_fileref);
	g_object_set_data (G_OBJECT (tag), "file", g_strdup((const gchar*)fname));				
	if ( desc && xmlStrcmp(desc,BAD_CAST"")!=0 )
		g_object_set_data (G_OBJECT (tag), "tip", g_strdup((gchar*)desc));
	gtk_text_buffer_insert_with_tags(buff,&iter,(const gchar*)text,xmlStrlen(text),tag,NULL);
	gtk_text_buffer_insert_at_cursor(buff,"\n",1);
}


void infb_insert_node(GtkTextBuffer *buff, xmlChar *text, xmlNodePtr node,gboolean endline) {
	GtkTextTag *tag;
	GtkTextIter iter;
	if (!text) return;
	gtk_text_buffer_get_iter_at_mark (buff,&iter,gtk_text_buffer_get_insert(buff));	
	tag = gtk_text_buffer_create_tag(buff,NULL,INFB_STYLE_NODE,NULL);
	g_object_set_data (G_OBJECT (tag), "type", &infb_v.nt_node);
	g_object_set_data (G_OBJECT (tag), "node", node);					
	gtk_text_buffer_insert_with_tags(buff,&iter,(const gchar*)text,xmlStrlen(text),tag,NULL);
	if (endline)
		gtk_text_buffer_insert_at_cursor(buff,"\n",1);
}

void infb_insert_group(GtkTextView *view, xmlChar *text, xmlNodePtr node) {
	GtkTextTag *tag;
	GtkTextIter iter;
	GtkTextBuffer *buff = gtk_text_view_get_buffer(view);
	xmlChar *text2;
	if (!text) return;
	/* text2 doesn't appear to be used. Can it be removed? */
	text2 = xmlGetProp(node,BAD_CAST "expanded");
	infb_insert_icon(view,gtk_image_new_from_stock(GTK_STOCK_DIRECTORY,GTK_ICON_SIZE_MENU),NULL);		
	gtk_text_buffer_get_iter_at_mark (buff,&iter,gtk_text_buffer_get_insert(buff));	
	tag = gtk_text_buffer_create_tag(buff,NULL,INFB_STYLE_GROUP,NULL);
	g_object_set_data (G_OBJECT (tag), "type", &infb_v.nt_group);
	g_object_set_data (G_OBJECT (tag), "node", node);					
	gtk_text_buffer_insert_with_tags(buff,&iter,(const gchar*)text,xmlStrlen(text),tag,NULL);
	gtk_text_buffer_insert_at_cursor(buff,"\n",1);
	xmlFree(text2);
}



static void infb_fill_node(GtkTextView *view,xmlDocPtr doc,xmlNodePtr node,gint level) {
	GtkTextBuffer *buff;
	gchar *levstr=NULL;
	xmlChar *text,*text2;
	xmlNodePtr auxn;
		
 	if ( view==NULL || doc==NULL || node==NULL ) return;
 	buff = gtk_text_view_get_buffer(view);
	if ( level > 0 ) 
		levstr = g_strnfill(2*level,' ');
	else
		levstr = "";	
	switch ( infb_v.currentType ) { 
		case INFB_DOCTYPE_INDEX:
			if ( xmlStrcmp(node->name,BAD_CAST "ref") ==0 ) {  /* reference index */
				text = xmlGetProp(node,BAD_CAST "name");
				if ( text ) {
						infb_insert_text(buff,text,INFB_TT_TITLE,TRUE);
						xmlFree(text);
						infb_insert_text(buff,BAD_CAST "",INFB_TT_NONE,TRUE);
				}
				auxn = node->xmlChildrenNode;
				while ( auxn ) {
					infb_fill_node(view,doc,auxn,level+1);
					auxn = auxn->next;
				}
			} /* reference index */	
			else	if ( xmlStrcmp(node->name,BAD_CAST "group") ==0 ) { /* fileref group  - this will link child */
				text = xmlGetProp(node,BAD_CAST "name");
				text2 = xmlGetProp(node,BAD_CAST "expanded");
				if ( text ) {					
					infb_insert_group(view,text,node);																					
					xmlFree(text);
				}						
				if ( text2 && xmlStrcmp(text2,BAD_CAST "1")==0 ) {
					auxn = node->xmlChildrenNode;
					while ( auxn ) {
						text = xmlGetProp(auxn,BAD_CAST "name");
						if ( text ) {
							xmlChar *text3,*text4;
							infb_insert_icon(view,gtk_image_new_from_stock(GTK_STOCK_FILE,GTK_ICON_SIZE_MENU),"  ");
							text4 = xmlNodeGetContent(auxn);
							text3 = xmlGetProp(auxn,BAD_CAST "description");
							infb_insert_fileref(buff,text,text4,text3);					
							xmlFree(text);
							xmlFree(text4);
						}	
						auxn = auxn->next;
					}
					xmlFree(text2);
				}										
			} /* fileref */				
		break; /* index document */
/**************************  DOCBOOK file ******************************/		
		case INFB_DOCTYPE_DOCBOOK:  /* DOCBOOK file */
			infb_db_fill_node(view,doc,node,level);
		break;
/**************************  DOCBOOK file END ******************************/
/**************************  HTML file ******************************/		
		case INFB_DOCTYPE_HTML: {
			infb_html_fill_node(view,doc,node,level,NULL,FALSE);
		}	
		break;
/**************************  HTML file END ******************************/	
	
		default: /* fref2 document */
			if ( xmlStrcmp(node->name,BAD_CAST "ref") ==0 ) { /* reference top */
				text = xmlGetProp(node,BAD_CAST "name");
				if ( text ) {
					infb_insert_text(buff,text,INFB_TT_TITLE,TRUE);
					xmlFree(text);
				}
				text = xmlGetProp(node,BAD_CAST "description");
				if ( text ) {
					infb_insert_text(buff,text,INFB_TT_DESC,TRUE);
					xmlFree(text);
				}
				
				auxn = node->xmlChildrenNode;
				while ( auxn ) {
					infb_fill_node(view,doc,auxn,level+1);
					auxn = auxn->next;
				}
			}
			else	if ( xmlStrcmp(node->name,BAD_CAST "note") ==0 ) { /* note */
				if ( level == 0 ) {
					text = xmlGetProp(node,BAD_CAST "title");
					if ( text ) {
						infb_insert_text(buff,text,INFB_TT_TITLE,TRUE);
						xmlFree(text);
					}	
					text = xmlNodeGetContent(node);
					if (text) {
						infb_insert_text(buff,text,INFB_TT_NONE,TRUE);
						xmlFree(text);
					}	
				}
				else 
				{
					text = xmlGetProp(node,BAD_CAST "title");
					if ( text ) {
						infb_insert_icon(view,gtk_image_new_from_stock(GTK_STOCK_EDIT,GTK_ICON_SIZE_MENU),levstr);
						infb_insert_node(buff,text,node,TRUE);											
						xmlFree(text);
					}
				}		
			} /* note */
			else	if ( xmlStrcmp(node->name,BAD_CAST "search_result") ==0 ) { /* search result */
				if ( level == 0 ) {
					text = xmlGetProp(node,BAD_CAST "title");
					if ( text ) {
						infb_insert_text(buff,text,INFB_TT_TITLE,TRUE);
						xmlFree(text);
					}
					auxn = node->xmlChildrenNode;
					while ( auxn ) {
						infb_fill_node(view,doc,auxn,level+1);
						auxn = auxn->next;
					}
				}
				else 
				{
					text = xmlGetProp(node,BAD_CAST "title");
					if ( text ) {
						infb_insert_icon(view,gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,GTK_ICON_SIZE_MENU),levstr);
						infb_insert_node(buff,text,node,TRUE);											
						xmlFree(text);
					}
				}		
			} /* search result */			
			else	if ( xmlStrcmp(node->name,BAD_CAST "group") ==0 ) { /* group */
				text = xmlGetProp(node,BAD_CAST "name");
				text2 = xmlGetProp(node,BAD_CAST "expanded");
				if ( text ) {
					if (level>0) gtk_text_buffer_insert_at_cursor(buff,levstr,2*level);
					infb_insert_group(view,text,node);																					
					xmlFree(text);
				}						
				if ( text2 && xmlStrcmp(text2,BAD_CAST "1")==0 ) {
					auxn = node->xmlChildrenNode;
					while ( auxn ) {
						infb_fill_node(view,doc,auxn,level+1);
						auxn = auxn->next;
					}
				}			
				if (text2) xmlFree(text2);
			} /* group */
/* ************************** ELEMENT **************************************/			
			else	if ( xmlStrcmp(node->name,BAD_CAST "element") ==0 ) { /* element */
			  if ( level == 0 ){
			  		xmlNodeSetPtr nodeset,nodeset2;
					xmlXPathObjectPtr result,result2;
					gint i,j;
					gchar *str;
					xmlNodePtr an,an2;
					gboolean attr_section=FALSE, param_section = FALSE, ret_section=FALSE;
					xmlChar *text3;
											  
					text = xmlGetProp(node,BAD_CAST "name");
					infb_insert_text(buff,text,INFB_TT_TITLE,TRUE);
					xmlFree(text);					
					
					result = getnodeset(doc,BAD_CAST "child::description",node);
					if (result) {
							nodeset = result->nodesetval;
							if ( nodeset->nodeNr > 0 ) {
								text = xmlNodeGetContent(nodeset->nodeTab[0]);
								infb_insert_text(buff,text,INFB_TT_DESC,TRUE);
								xmlFree(text);
							}
							xmlXPathFreeObject (result);
					}
					
					/* PROPLISTS */
					result = getnodeset(doc,BAD_CAST "child::properties/proplist",node);
					if (result && result->nodesetval->nodeNr > 0) {
							nodeset = result->nodesetval;
							for (i=0;i< nodeset->nodeNr;i++  ) {
								text = xmlGetProp(nodeset->nodeTab[i],BAD_CAST "ref");
								if (text) {
									str = g_strdup_printf("//def[@deftype='proplist' and @id='%s']",text);
									an = getnode(doc,BAD_CAST str,NULL);
									g_free(str);
									xmlFree(text);									
									if (an) {
										/* attributes */
										result2 = getnodeset(doc,BAD_CAST "child::property[@kind='attribute']",an);
										if (result2 && result2->nodesetval->nodeNr > 0) {
												if (!attr_section) {
													infb_insert_text(buff,BAD_CAST "Attributes",INFB_TT_SECTION,TRUE);
													attr_section = TRUE;
												}	
												nodeset2 = result2->nodesetval;
												for (j=0;j< nodeset2->nodeNr;j++  ) {
													text = xmlGetProp(nodeset2->nodeTab[j],BAD_CAST "name");
													text2 = xmlGetProp(nodeset2->nodeTab[j],BAD_CAST "type");
													if (text) {
														if ( text2 && xmlStrcmp(text2,BAD_CAST "")!=0) 
															str = g_strconcat((gchar*)text," (",(gchar*)text2,")",NULL);
														else 
															str = g_strdup((gchar*)text);
														infb_insert_icon(view,gtk_image_new_from_stock(GTK_STOCK_ABOUT,GTK_ICON_SIZE_MENU),NULL);											
														infb_insert_text(buff,BAD_CAST str,INFB_TT_NONE,TRUE);
														g_free(str);
														xmlFree(text);
													}	
													if (text2) xmlFree(text2);
													an2 = getnode(doc,BAD_CAST "child::description",nodeset2->nodeTab[j]);
													if ( an2 ) {
														text = xmlNodeGetContent(an2);
														if (text) {
															infb_insert_text(buff,text,INFB_TT_SMALL,TRUE);
															xmlFree(text);
														}	
													}
												}
												xmlXPathFreeObject (result2);
										} /* attributes */
										/* parameters */
										result2 = getnodeset(doc,BAD_CAST "child::property[@kind='parameter']",an);
										if (result2 && result2->nodesetval->nodeNr > 0) {
												if (!param_section) {
													infb_insert_text(buff,BAD_CAST "Parameters",INFB_TT_SECTION,TRUE);
													param_section = TRUE;
												}	
												nodeset2 = result2->nodesetval;
												for (j=0;j< nodeset2->nodeNr;j++  ) {
													text = xmlGetProp(nodeset2->nodeTab[j],BAD_CAST "name");
													text2 = xmlGetProp(nodeset2->nodeTab[j],BAD_CAST "type");
													if (text) {
														if ( text2 && xmlStrcmp(text2,BAD_CAST "")!=0) 
															str = g_strconcat((gchar*)text," (",(gchar*)text2,")",NULL);
														else 
															str = g_strdup((gchar*)text);			
														infb_insert_icon(view,gtk_image_new_from_stock(GTK_STOCK_ABOUT,GTK_ICON_SIZE_MENU),NULL);								
														infb_insert_text(buff,BAD_CAST str,INFB_TT_NONE,TRUE);
														g_free(str);
														xmlFree(text);
													}	
													if (text2) xmlFree(text2);
													an2 = getnode(doc,BAD_CAST "child::description",nodeset2->nodeTab[j]);
													if ( an2 ) {
														text = xmlNodeGetContent(an2);
														if (text) {
															infb_insert_text(buff,text,INFB_TT_SMALL,TRUE);
															xmlFree(text);
														}	
													}
												}
												xmlXPathFreeObject (result2);
										} /* parameters */
										
									} /* proplist found */
								}
							}						
					} /* proplist */
					if (result)	xmlXPathFreeObject(result);
					
					/* SINGLE PROPERTY REF */
					result = getnodeset(doc,BAD_CAST "child::properties/property[@ref]",node);
					if (result && result->nodesetval->nodeNr > 0) {
							nodeset = result->nodesetval;
							for (i=0;i< nodeset->nodeNr;i++  ) {
								text = xmlGetProp(nodeset->nodeTab[i],BAD_CAST "ref");
								if (text) {
									str = g_strdup_printf("//def[@deftype='property' and @id='%s']",text);
									an = getnode(doc,BAD_CAST str,NULL);
									g_free(str);
									xmlFree(text);		
									text = xmlGetProp(an,BAD_CAST "kind");
									if (text) {
										if (xmlStrcmp(text,BAD_CAST "attribute")==0) {
												if (!attr_section) {
													infb_insert_text(buff,BAD_CAST "Attributes",INFB_TT_SECTION,TRUE);
													attr_section = TRUE;
												}	
												text2 = xmlGetProp(an,BAD_CAST "name");
												text3 = xmlGetProp(an,BAD_CAST "type");
												if (text2) {
														if ( text3 && xmlStrcmp(text3,BAD_CAST "")!=0) 
															str = g_strconcat((gchar*)text2," (",(gchar*)text3,")",NULL);
														else 
															str = g_strdup((gchar*)text2);		
														infb_insert_icon(view,gtk_image_new_from_stock(GTK_STOCK_ABOUT,GTK_ICON_SIZE_MENU),NULL);									
														infb_insert_text(buff,BAD_CAST str,INFB_TT_NONE,TRUE);
														g_free(str);
														xmlFree(text2);
													}	
													if (text3) xmlFree(text3);
													an2 = getnode(doc,BAD_CAST "child::description",an);
													if ( an2 ) {
														text2 = xmlNodeGetContent(an2);
														if (text2) {
															infb_insert_text(buff,text2,INFB_TT_SMALL,TRUE);
															xmlFree(text2);
														}	
													}
										} /* attribute */
										else if (xmlStrcmp(text,BAD_CAST "parameter")==0) {
												if (!param_section) {
													infb_insert_text(buff,BAD_CAST "Parameters",INFB_TT_SECTION,TRUE);
													param_section = TRUE;
												}	
												text2 = xmlGetProp(an,BAD_CAST "name");
												text3 = xmlGetProp(an,BAD_CAST "type");
												if (text2) {
														if ( text3 && xmlStrcmp(text3,BAD_CAST "")!=0) 
															str = g_strconcat((gchar*)text2," (",(gchar*)text3,")",NULL);
														else 
															str = g_strdup((gchar*)text2);		
														infb_insert_icon(view,gtk_image_new_from_stock(GTK_STOCK_ABOUT,GTK_ICON_SIZE_MENU),NULL);									
														infb_insert_text(buff,BAD_CAST str,INFB_TT_NONE,TRUE);
														g_free(str);
														xmlFree(text2);
													}	
													if (text3) xmlFree(text3);
													an2 = getnode(doc,BAD_CAST "child::description",an);
													if ( an2 ) {
														text2 = xmlNodeGetContent(an2);
														if (text2) {
															infb_insert_text(buff,text2,INFB_TT_SMALL,TRUE);
															xmlFree(text2);
														}	
													}
										} /* parameter */
										else if (xmlStrcmp(text,BAD_CAST "return")==0) {
												if (!ret_section) {
													text2 = xmlGetProp(an,BAD_CAST "type");
													if ( text2 ) {
														str = g_strconcat("Returned type: ",(gchar*)text2,NULL);
														infb_insert_text(buff,BAD_CAST str,INFB_TT_SECTION,TRUE);
														g_free(str);
														ret_section = TRUE;
														xmlFree(text2);
														infb_insert_text(buff,BAD_CAST "",INFB_TT_NONE,TRUE);
													}	
												}	
										} /* return */										
																				
									}							
								}
							}			
					} /* single property */											
					if (result)	xmlXPathFreeObject(result);
					
					/* ATTRIBUTES */
					result = getnodeset(doc,BAD_CAST "child::properties/property[@kind='attribute']",node);
					if (result && result->nodesetval->nodeNr > 0) {
							if (!attr_section) {
								infb_insert_text(buff,BAD_CAST "Attributes",INFB_TT_SECTION,TRUE);
								attr_section = TRUE;	
							}	
							nodeset = result->nodesetval;
							for (i=0;i< nodeset->nodeNr;i++  ) {
								text = xmlGetProp(nodeset->nodeTab[i],BAD_CAST "name");
								text2 = xmlGetProp(nodeset->nodeTab[i],BAD_CAST "type");
								if ( text2 && xmlStrcmp(text2,BAD_CAST "")!=0) 
									str = g_strconcat((gchar*)text," (",(gchar*)text2,")",NULL);
								else 
									str = g_strdup((gchar*)text);
								infb_insert_icon(view,gtk_image_new_from_stock(GTK_STOCK_ABOUT,GTK_ICON_SIZE_MENU),NULL);											
								infb_insert_text(buff,BAD_CAST str,INFB_TT_NONE,TRUE);
								g_free(str);
								xmlFree(text);
								if (text2) xmlFree(text2);
								an = getnode(doc,BAD_CAST "child::description",nodeset->nodeTab[i]);
								if ( an ) {
									text = xmlNodeGetContent(an);
									infb_insert_text(buff,text,INFB_TT_SMALL,TRUE);
									xmlFree(text);
								}
							}
					} /* attributes */
					if (result)	xmlXPathFreeObject(result);
					
					/* PARAMETERS */
					result = getnodeset(doc,BAD_CAST "child::properties/property[@kind='parameter']",node);
					if (result && result->nodesetval->nodeNr > 0) {
							if (!param_section) {
								infb_insert_text(buff,BAD_CAST "Parameters",INFB_TT_SECTION,TRUE);
								param_section = TRUE;	
							}	
							nodeset = result->nodesetval;
							for (i=0;i< nodeset->nodeNr;i++  ) {
								text = xmlGetProp(nodeset->nodeTab[i],BAD_CAST "name");
								text2 = xmlGetProp(nodeset->nodeTab[i],BAD_CAST "type");
								if ( text2 && xmlStrcmp(text2,BAD_CAST "")!=0) 
									str = g_strconcat((gchar*)text," (",(gchar*)text2,")",NULL);
								else 
									str = g_strdup((gchar*)text);			
								infb_insert_icon(view,gtk_image_new_from_stock(GTK_STOCK_ABOUT,GTK_ICON_SIZE_MENU),NULL);								
								infb_insert_text(buff,BAD_CAST str,INFB_TT_NONE,TRUE);
								g_free(str);
								xmlFree(text);
								if (text2) xmlFree(text2);
								an = getnode(doc,BAD_CAST "child::description",nodeset->nodeTab[i]);
								if ( an ) {
									text = xmlNodeGetContent(an);
									infb_insert_text(buff,text,INFB_TT_SMALL,TRUE);
									xmlFree(text);
								}
							}
					} /* parameters */
					if (result)	xmlXPathFreeObject(result);
					
					/* RETURN */
					an = getnode(doc,BAD_CAST "child::properties/property[@kind='return']",node);
					if ( an ) {
						if (!ret_section) {
							text2 = xmlGetProp(an,BAD_CAST "type");
							if ( text2 ) {
								str = g_strconcat("Returned type: ",(gchar*)text2,NULL);
								infb_insert_text(buff,BAD_CAST str,INFB_TT_SECTION,TRUE);
								g_free(str);
								ret_section = TRUE;
								xmlFree(text2);
								infb_insert_text(buff,BAD_CAST "",INFB_TT_NONE,TRUE);
							}	
						}						
					} /* return */
					/* EXAMPLES */
					result = getnodeset(doc,BAD_CAST "child::properties/property[@kind='example']",node);
					if (result && result->nodesetval->nodeNr > 0) {							
							nodeset = result->nodesetval;
							for (i=0;i< nodeset->nodeNr;i++  ) {
								infb_insert_text(buff,BAD_CAST "Example",INFB_TT_SECTION,TRUE);
								an = getnode(doc,BAD_CAST "child::description",nodeset->nodeTab[i]);
								if ( an ) {
									text = xmlNodeGetContent(an);
									infb_insert_text(buff,text,INFB_TT_SMALL,TRUE);
									xmlFree(text);
								}
							}
					} /* examples */
					if (result)	xmlXPathFreeObject(result);
					
					/* NOTES */
					result = getnodeset(doc,BAD_CAST "child::note",node);
					if (result && result->nodesetval->nodeNr > 0) {							
							nodeset = result->nodesetval;
							for (i=0;i< nodeset->nodeNr;i++  ) {
								text = xmlGetProp(nodeset->nodeTab[i],BAD_CAST "title");
								infb_insert_text(buff,text,INFB_TT_SECTION,TRUE);
								xmlFree(text);
								text = xmlNodeGetContent(nodeset->nodeTab[i]);
								infb_insert_text(buff,text,INFB_TT_SMALL,TRUE);
								xmlFree(text);
							}
					} /* notes */
					if (result)	xmlXPathFreeObject(result);					
			  }
			  else {
				text = xmlGetProp(node,BAD_CAST "name");
				if ( text ) {
					infb_insert_icon(view,gtk_image_new_from_stock(GTK_STOCK_ABOUT,GTK_ICON_SIZE_MENU),levstr);
					infb_insert_node(buff,text,node,TRUE);					
					xmlFree(text);
				}
			  } 			
			} /* element */
		
		break;
	} /* switch */ 
	if ( level > 0 ) g_free(levstr);	
}



void infb_fill_doc(Tbfwin *bfwin,xmlNodePtr root) {
	GtkTextView *view;
	gpointer auxp;
	GtkTextBuffer *buff;
	GtkTextIter it1,it2;
	xmlNodePtr node;
	
	auxp = g_hash_table_lookup(infb_v.windows,bfwin);
	if ( !auxp ) return;
	view = GTK_TEXT_VIEW(((Tinfbwin*)auxp)->view);
	if ( infb_v.currentDoc == NULL || view==NULL ) return;

	
	if ( infb_v.currentDoc != infb_v.homeDoc ) {
		/*gtk_widget_set_sensitive(((Tinfbwin*)auxp)->btn_add,FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(((Tinfbwin*)auxp)->btn_del),FALSE);*/
		gtk_widget_set_sensitive(GTK_WIDGET(((Tinfbwin*)auxp)->btn_home),TRUE);
		gtk_widget_set_sensitive(((Tinfbwin*)auxp)->sentry,TRUE);
	} else
	{
		/*gtk_widget_set_sensitive(((Tinfbwin*)auxp)->btn_add,TRUE);
		gtk_widget_set_sensitive(((Tinfbwin*)auxp)->btn_del,TRUE);*/
		gtk_widget_set_sensitive(GTK_WIDGET(((Tinfbwin*)auxp)->btn_home),FALSE);
		gtk_widget_set_sensitive(((Tinfbwin*)auxp)->sentry,FALSE);
	}

	 	
	buff = gtk_text_view_get_buffer(view);
	gtk_text_buffer_get_bounds(buff,&it1,&it2);
	gtk_text_buffer_remove_all_tags (buff,&it1,&it2);
	gtk_text_buffer_delete(buff,&it1,&it2);
	
	infb_set_current_type(infb_v.currentDoc);
		
	if ( infb_v.currentType == INFB_DOCTYPE_UNKNOWN ) {
		infb_insert_error(view,BAD_CAST _("Unknown document type"));
		return;
	}	
	if ( root == NULL ){
		node = xmlDocGetRootElement(infb_v.currentDoc);
		infb_v.currentNode = node;
		infb_fill_node(view,infb_v.currentDoc,node,0);
	} else 
	{
		infb_v.currentNode = root;	
		infb_fill_node(view,infb_v.currentDoc,root,0);
	}
	gtk_widget_set_sensitive(GTK_WIDGET(((Tinfbwin*)auxp)->btn_up),
									 (infb_v.currentNode->parent != NULL && 
									  (void*)infb_v.currentNode->parent != (void*)infb_v.currentNode->doc ) );
	node = xmlDocGetRootElement(infb_v.currentDoc);
	gtk_widget_set_sensitive(GTK_WIDGET(((Tinfbwin*)auxp)->btn_idx),(node!=infb_v.currentNode));
						  
}


