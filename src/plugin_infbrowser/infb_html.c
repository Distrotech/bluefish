/* Bluefish HTML Editor
 * infb_html.c - plugin for information browser
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

#include <stdlib.h>
#include "infb_html.h"
#include "infbrowser.h"
#include "infb_text.h"


int
htmlAutoCloseTag(htmlDocPtr doc, const xmlChar *name, htmlNodePtr elem) {
    /*htmlNodePtr child;

    if (elem == NULL) return(1);
    if (xmlStrEqual(name, elem->name)) return(0);
    if (htmlCheckAutoClose(elem->name, name)) return(1);
    child = elem->children;
    while (child != NULL) {
        if (htmlAutoCloseTag(doc, name, child)) return(1);
	child = child->next;
    }*/
    return(1);
}


xmlChar *infb_html_get_title(xmlDocPtr doc) {
	xmlNodePtr an = NULL,node;
	xmlChar *text;
	
	node = xmlDocGetRootElement(doc);
	an = getnode(doc,BAD_CAST "head/title",node);
	if (an) {
		text = xmlNodeGetContent(an);
		if (text) {
			return text;
		}
	}  
	return NULL;
}

static xmlChar *clean_text(xmlChar *t) {
	gint i,j;
	xmlChar *ret = xmlStrdup(t);
	
	for(i=0;i<xmlStrlen(t);i++) 
		if (t[i]=='\n' || t[i]=='\r' || t[i]=='\t') t[i]=' ';
	j=0;	
	for(i=0;i<xmlStrlen(t);i++) {
		if (t[i]!=' ') {
			ret[j] = t[i];
			j++;
		} 
		else if (ret[j-1]!=' ') {
			ret[j] = t[i];
			j++;		
		}
	}	
	ret[j] = '\0';
	return ret;
	/*BAD_CAST g_strstrip((gchar*)ret);*/	
}

static GtkTextTag *infb_html_copy_tag(GtkTextBuffer *buff,GtkTextTag *tag) {
	GtkTextTag *ret = gtk_text_buffer_create_tag(buff,NULL,NULL);
	GdkColor gdk;	
	gboolean set;
	gchararray arr;
	PangoFontDescription *font_desc;
	gint i;
	GtkJustification *just;
	gdouble d;
	PangoStyle style;
	PangoUnderline und;
	GtkWrapMode wrap;
	
	if (!tag) return ret;
	g_object_get(G_OBJECT(tag),"background-set",&set,NULL);
	if (set) {
		g_object_get(G_OBJECT(tag),"background-gdk",&gdk,NULL);
		g_object_set(G_OBJECT(ret),"background-gdk",&gdk,NULL);
	}
	g_object_get(G_OBJECT(tag),"family-set",&set,NULL);
	if (set) {
		g_object_get(G_OBJECT(tag),"family",&arr,NULL);
		g_object_set(G_OBJECT(ret),"family",arr,NULL);
	}
	g_object_get(G_OBJECT(tag),"font",&arr,NULL);
	g_object_set(G_OBJECT(ret),"font",arr,NULL);
	g_object_get(G_OBJECT(tag),"font-desc",&font_desc,NULL);
	if ( font_desc )
		g_object_set(G_OBJECT(ret),"font-desc",font_desc,NULL);
	g_object_get(G_OBJECT(tag),"foreground-set",&set,NULL);
	if (set) {
		g_object_get(G_OBJECT(tag),"foreground-gdk",&gdk,NULL);
		g_object_set(G_OBJECT(ret),"foreground-gdk",&gdk,NULL);
	}
	g_object_get(G_OBJECT(tag),"indent-set",&set,NULL);
	if (set) {
		g_object_get(G_OBJECT(tag),"indent",&i,NULL);
		g_object_set(G_OBJECT(ret),"indent",i,NULL);
	}
	g_object_get(G_OBJECT(tag),"justification-set",&set,NULL);
	if (set) {
		g_object_get(G_OBJECT(tag),"justification",&just,NULL);
		g_object_set(G_OBJECT(ret),"justification",just,NULL);
	}
	g_object_get(G_OBJECT(tag),"left-margin-set",&set,NULL);
	if (set) {
		g_object_get(G_OBJECT(tag),"left-margin",&i,NULL);
		g_object_set(G_OBJECT(ret),"left-margin",i,NULL);
	}
	g_object_get(G_OBJECT(tag),"right-margin-set",&set,NULL);
	if (set) {
		g_object_get(G_OBJECT(tag),"right-margin",&i,NULL);
		g_object_set(G_OBJECT(ret),"right-margin",i,NULL);
	}	
	g_object_get(G_OBJECT(tag),"paragraph-background-set",&set,NULL);
	if (set) {
		g_object_get(G_OBJECT(tag),"paragraph-background-gdk",&gdk,NULL);
		g_object_set(G_OBJECT(ret),"paragraph-background-gdk",&gdk,NULL);
	}
	g_object_get(G_OBJECT(tag),"scale-set",&set,NULL);
	if (set) {
		g_object_get(G_OBJECT(tag),"scale",&d,NULL);
		g_object_set(G_OBJECT(ret),"scale",d,NULL);
	}
	g_object_get(G_OBJECT(tag),"size-set",&set,NULL);
	if (set) {
		g_object_get(G_OBJECT(tag),"size",&i,NULL);
		g_object_set(G_OBJECT(ret),"size",i,NULL);
	}	
	g_object_get(G_OBJECT(tag),"strikethrough-set",&set,NULL);
	if (set) {
		g_object_get(G_OBJECT(tag),"strikethrough",&set,NULL);
		g_object_set(G_OBJECT(ret),"strikethrough",set,NULL);
	}	
	g_object_get(G_OBJECT(tag),"style-set",&set,NULL);
	if (set) {
		g_object_get(G_OBJECT(tag),"style",&style,NULL);
		g_object_set(G_OBJECT(ret),"style",style,NULL);
	}	
	g_object_get(G_OBJECT(tag),"underline-set",&set,NULL);
	if (set) {
		g_object_get(G_OBJECT(tag),"underline",&und,NULL);
		g_object_set(G_OBJECT(ret),"underline",und,NULL);
	}	
	g_object_get(G_OBJECT(tag),"weight-set",&set,NULL);
	if (set) {
		g_object_get(G_OBJECT(tag),"weight",&i,NULL);
		g_object_set(G_OBJECT(ret),"weight",i,NULL);
	}	
	g_object_get(G_OBJECT(tag),"wrap-mode-set",&set,NULL);
	if (set) {
		g_object_get(G_OBJECT(tag),"wrap-mode",&wrap,NULL);
		g_object_set(G_OBJECT(ret),"wrap-mode",wrap,NULL);
	}	
	return ret;
}

void infb_html_fill_node(GtkTextView *view,xmlDocPtr doc,xmlNodePtr node,
								  gint level,GtkTextTag *tag,gboolean preserve_spaces) {
	xmlNodePtr auxn;
	GtkTextBuffer *buff = gtk_text_view_get_buffer(view);
	xmlChar *text;
	GtkTextTag *t=NULL;
	
	if (xmlStrcmp(node->name,BAD_CAST "body")==0 ||
		 xmlStrcmp(node->name,BAD_CAST "html")==0 ||
		 xmlStrcmp(node->name,BAD_CAST "span")==0 ||
		 xmlStrcmp(node->name,BAD_CAST "table")==0 ||
		 xmlStrcmp(node->name,BAD_CAST "tbody")==0 
		 ) {

		auxn = node->xmlChildrenNode;
		while (auxn) {
			infb_html_fill_node(view,doc,auxn,level+1,tag,preserve_spaces);				
			auxn = auxn->next;
		}		
	}
	else if (xmlStrcmp(node->name,BAD_CAST "div")==0 ||
				xmlStrcmp(node->name,BAD_CAST "p")==0 ||
				xmlStrcmp(node->name,BAD_CAST "dt")==0 ||
				xmlStrcmp(node->name,BAD_CAST "dd")==0 ||
				 xmlStrcmp(node->name,BAD_CAST "ul")==0 ||
				 xmlStrcmp(node->name,BAD_CAST "dl")==0 ||
		 		xmlStrcmp(node->name,BAD_CAST "ol")==0
				
			   ) {
		auxn = node->xmlChildrenNode;
		while (auxn) {
			infb_html_fill_node(view,doc,auxn,level+1,tag,preserve_spaces);				
			auxn = auxn->next;
		}
		infb_insert_text(buff,BAD_CAST "",INFB_TT_NONE,TRUE);		
	}
	else if (xmlStrcmp(node->name,BAD_CAST "li")==0 ) {
		infb_insert_text(buff,BAD_CAST "•",INFB_TT_NONE,FALSE);
		auxn = node->xmlChildrenNode;
		while (auxn) {
			infb_html_fill_node(view,doc,auxn,level+1,tag,preserve_spaces);				
			auxn = auxn->next;
		}
		infb_insert_text(buff,BAD_CAST "",INFB_TT_NONE,TRUE);		
	}	
	else if (xmlStrcmp(node->name,BAD_CAST "em")==0 ||
				xmlStrcmp(node->name,BAD_CAST "i")==0 ||
				xmlStrcmp(node->name,BAD_CAST "abbr")==0 ||
				xmlStrcmp(node->name,BAD_CAST "code")==0	) {
		t = infb_html_copy_tag(buff,tag);
		g_object_set(G_OBJECT(t),"style",PANGO_STYLE_ITALIC,NULL);
		auxn = node->xmlChildrenNode;
		while (auxn) {
			infb_html_fill_node(view,doc,auxn,level,t,TRUE);				
			auxn = auxn->next;
		}	
	}
	else if (xmlStrcmp(node->name,BAD_CAST "b")==0 ||
				xmlStrcmp(node->name,BAD_CAST "strong")==0 
				) {
		t = infb_html_copy_tag(buff,tag);
		g_object_set(G_OBJECT(t),"weight",PANGO_WEIGHT_SEMIBOLD,NULL);
		auxn = node->xmlChildrenNode;
		while (auxn) {
			infb_html_fill_node(view,doc,auxn,level,t,TRUE);				
			auxn = auxn->next;
		}	
	}
	else if (xmlStrcmp(node->name,BAD_CAST "a")==0) { /* link */
		xmlChar *lt,*tt;
		GdkColor gc;
		gboolean doit=FALSE;

		t = infb_html_copy_tag(buff,tag);		
		lt = xmlGetProp(node,BAD_CAST "href");
		if ( lt ) {
			if (!g_str_has_prefix ((gchar*)lt,"http://")) {
				if (xmlStrcmp(doc->URL,lt)!=0 && lt[0]!='/') {
					gchar *dir = g_path_get_dirname((gchar*)doc->URL);
					gchar **arr = g_strsplit((gchar*)lt,"#",-1);
					gchar *ss = g_strconcat(dir,"/",arr[0],NULL);
					if (arr[1] && (xmlStrcmp(doc->URL,BAD_CAST arr[0])==0 || xmlStrcmp(BAD_CAST arr[0],BAD_CAST "")==0) ) 
					{
						g_object_set_data(G_OBJECT(t),"type",&infb_v.nt_localref);
						g_object_set_data(G_OBJECT(t),"node",g_strdup(arr[1]));
						doit = TRUE;
					} else {
						g_object_set_data(G_OBJECT(t),"type",&infb_v.nt_fileref);					
						g_object_set_data(G_OBJECT(t),"file",g_strdup(ss));
						doit = TRUE;
					}	
					g_strfreev(arr);
					g_free(dir);
					g_free(ss);
				} else
				{
					g_object_set_data(G_OBJECT(t),"type",&infb_v.nt_fileref);
					g_object_set_data(G_OBJECT(t),"file",g_strdup((gchar*)lt));
					doit = TRUE;
				}	
			} else {
				g_object_set_data(G_OBJECT(t),"type",&infb_v.nt_fileref); 
				g_object_set_data(G_OBJECT(t),"file",g_strdup((gchar*)lt));				
				doit = TRUE;
			}	
			if (doit) {				
				g_object_set(G_OBJECT(t),"underline",PANGO_UNDERLINE_SINGLE,NULL);
				
				gdk_color_parse("#1E1ECE",&gc);
				g_object_set(G_OBJECT(t),"foreground-gdk",&gc,NULL);
				tt = xmlGetProp(node,BAD_CAST "title");
				if (tt) {
					g_object_set_data(G_OBJECT(t),"tip",g_strdup((gchar*)tt));
					xmlFree(tt);
				}
			}	
			xmlFree(lt);
		} else {
			lt = xmlGetProp(node,BAD_CAST "name");
			if (lt) {
				infb_insert_anchor(view,lt);
				xmlFree(lt);
			}
		}
		auxn = node->xmlChildrenNode;
		while (auxn) {
			infb_html_fill_node(view,doc,auxn,level,t,TRUE);				
			auxn = auxn->next;
		}	
	}																			
	else if (xmlStrcmp(node->name,BAD_CAST "pre")==0) {
		auxn = node->xmlChildrenNode;
		while (auxn) {
			infb_html_fill_node(view,doc,auxn,level+1,tag,TRUE);				
			auxn = auxn->next;
		}
		infb_insert_text(buff,BAD_CAST "",INFB_TT_NONE,TRUE);		
	}			
	else if (xmlStrcmp(node->name,BAD_CAST "br")==0) {
		infb_insert_text(buff,BAD_CAST "",INFB_TT_NONE,TRUE);
	}
	else if (xmlStrcmp(node->name,BAD_CAST "hr")==0) {
		GdkColor gc;
		t = infb_html_copy_tag(buff,tag);
		gdk_color_parse("#EEEEEE",&gc);
		g_object_set(G_OBJECT(t),"underline",PANGO_UNDERLINE_SINGLE,NULL);
		g_object_set(G_OBJECT(t),"justification",GTK_JUSTIFY_CENTER,NULL);		
		infb_insert_text_tag(buff,BAD_CAST "                          ",t,TRUE);   
	}	
	else if (xmlStrcmp(node->name,BAD_CAST "h5")==0 ||
				xmlStrcmp(node->name,BAD_CAST "h4")==0 ||
				xmlStrcmp(node->name,BAD_CAST "h3")==0 ||
				xmlStrcmp(node->name,BAD_CAST "h2")==0 ||
				xmlStrcmp(node->name,BAD_CAST "h1")==0
 				) { 		 		
		GtkStyle *s = gtk_widget_get_style(GTK_WIDGET(view));
		t = infb_html_copy_tag(buff,tag);
		g_object_set(G_OBJECT(t),"size",pango_font_description_get_size(s->font_desc)+
						(6-g_ascii_digit_value(node->name[1]))*PANGO_SCALE,NULL);
		g_object_set(G_OBJECT(t),"weight",PANGO_WEIGHT_SEMIBOLD,NULL);
		auxn = node->xmlChildrenNode;
		while (auxn) {
			infb_html_fill_node(view,doc,auxn,level+1,t,preserve_spaces);				
			auxn = auxn->next;
		}				
		infb_insert_text(buff,BAD_CAST "\n",INFB_TT_NONE,TRUE);
	}
	else if (xmlStrcmp(node->name,BAD_CAST "tr")==0 ) {				
		auxn = node->xmlChildrenNode;	
		while (auxn) {
			if (xmlStrcmp(auxn->name,BAD_CAST "td")==0
				 || xmlStrcmp(auxn->name,BAD_CAST "th")==0 ) {
					xmlNodePtr auxn2 = auxn->xmlChildrenNode;
					while (auxn2) { 				 
						infb_html_fill_node(view,doc,auxn2,level,tag,preserve_spaces);
						infb_insert_text(buff,BAD_CAST " ",INFB_TT_NONE,FALSE);
						auxn2 = auxn2->next;	
					}	
				}							
			auxn = auxn->next;
		}		
		infb_insert_text(buff,BAD_CAST "",INFB_TT_NONE,TRUE);		
	}
	else if (xmlStrcmp(node->name,BAD_CAST "title")==0 ||
				xmlStrcmp(node->name,BAD_CAST "link")==0 ||
				xmlStrcmp(node->name,BAD_CAST "meta")==0 ||
				xmlStrcmp(node->name,BAD_CAST "head")==0  
				) {
		/* nothing to parse */
	}		
	else {
		text = xmlNodeGetContent(node);
		if (text && (node->type==XML_ELEMENT_NODE || node->type==XML_TEXT_NODE) ) {
			if (!preserve_spaces) {
				text = clean_text(text);
			}	
			if (tag) {
				infb_insert_text_tag(buff,text,tag,FALSE);
			}
			else {
				infb_insert_text(buff,text,INFB_TT_NONE,FALSE);
			}	
				xmlFree(text);
		}	
	}
}


