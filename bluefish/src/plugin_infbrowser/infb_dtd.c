/* Bluefish HTML Editor
 * infb_dtd.c - plugin for information browser
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

#include "infb_dtd.h"

static gchar *infb_dtd_str_content(xmlElementContentPtr ct,gchar *sofar) {
	gchar *ret = sofar,*tofree;
	if (ct==NULL) return ret;
	switch (ct->type) {
		case XML_ELEMENT_CONTENT_PCDATA:
			tofree = ret;
			ret = g_strconcat(ret,"PCDATA",NULL);
			if (tofree) g_free(tofree);
		break;
		case XML_ELEMENT_CONTENT_ELEMENT:
			tofree = ret;
			ret = g_strconcat(ret,"<",ct->name,">",NULL);
			if (tofree) g_free(tofree);
		break; 	
		case XML_ELEMENT_CONTENT_SEQ:
			if ( ct->c1 )
				ret = infb_dtd_str_content(ct->c1,ret);
			if ( ct->c2 ) {
				tofree = ret;
				ret = g_strconcat(ret,"\n",NULL);
				if (tofree) g_free(tofree);
				ret = infb_dtd_str_content(ct->c2,ret);
			}										
		break; 		 
		case XML_ELEMENT_CONTENT_OR:			
			if ( ct->c1 )
				ret = infb_dtd_str_content(ct->c1,ret);
			if ( ct->c2 ) {
				tofree = ret;
				ret = g_strconcat(ret,"|",NULL);
				if (tofree) g_free(tofree);
				ret = infb_dtd_str_content(ct->c2,ret);
			}										
		break; 			
		default:break;
	}
	switch (ct->ocur) {
		case XML_ELEMENT_CONTENT_OPT:
			tofree = ret;
			ret = g_strconcat(ret,"(optional)",NULL);
			if (tofree) g_free(tofree);
		break; 	
		case XML_ELEMENT_CONTENT_MULT:
			tofree = ret;
			ret = g_strconcat(ret,"(zero or more)",NULL);
			if (tofree) g_free(tofree);
		break; 		 
		case XML_ELEMENT_CONTENT_PLUS:
			tofree = ret;
			ret = g_strconcat(ret,"(one or more)",NULL);
			if (tofree) g_free(tofree);
		break;
		default:break; 			
	}
		
	return ret;
}

static void infb_dtd_element(void *payload, void *data, xmlChar *name) {
	xmlDocPtr doc = (xmlDocPtr)data;
	xmlElementPtr el = (xmlElementPtr)payload;
	xmlAttributePtr attr = el->attributes, ptr=NULL;
	xmlNodePtr root = xmlDocGetRootElement(doc);
	xmlNodePtr node,node2,node3,txt,enode;
	gchar *pstr,*tofree;
	
	enode = xmlNewNode(NULL,BAD_CAST "element");
	xmlNewProp(enode, BAD_CAST "kind", BAD_CAST "tag");
	xmlNewProp(enode, BAD_CAST "name", name);
	xmlAddChild(root,enode);
	node2 = xmlNewNode(NULL,BAD_CAST "properties");
	xmlAddChild(enode,node2);
	ptr = attr;
	while (ptr) {
		node = xmlNewNode(NULL,BAD_CAST "property");
		xmlNewProp(node,BAD_CAST "kind",BAD_CAST "attribute");
		xmlNewProp(node,BAD_CAST "name",ptr->name);
		switch (ptr->atype) {
			case XML_ATTRIBUTE_CDATA:xmlNewProp(node,BAD_CAST "type",BAD_CAST "CDATA");break;
			case XML_ATTRIBUTE_ID:xmlNewProp(node,BAD_CAST "type",BAD_CAST "ID");break;
			case XML_ATTRIBUTE_IDREF:xmlNewProp(node,BAD_CAST "type",BAD_CAST "IDREF");break;
			case XML_ATTRIBUTE_IDREFS:xmlNewProp(node,BAD_CAST "type",BAD_CAST "IDREFS");break;
			case XML_ATTRIBUTE_ENTITY:xmlNewProp(node,BAD_CAST "type",BAD_CAST "ENTITY");break;
			case XML_ATTRIBUTE_ENTITIES:xmlNewProp(node,BAD_CAST "type",BAD_CAST "ENTITIES");break;
			case XML_ATTRIBUTE_NMTOKEN:xmlNewProp(node,BAD_CAST "type",BAD_CAST "NMTOKEN");break;
			case XML_ATTRIBUTE_NMTOKENS:xmlNewProp(node,BAD_CAST "type",BAD_CAST "NMTOKENS");break;
			case XML_ATTRIBUTE_ENUMERATION:xmlNewProp(node,BAD_CAST "type",BAD_CAST "ENUMERATION");break;
			case XML_ATTRIBUTE_NOTATION:xmlNewProp(node,BAD_CAST "type",BAD_CAST "NOTATION");break;
		}
		pstr=NULL;
		switch (ptr->def) {
			case XML_ATTRIBUTE_REQUIRED: pstr = g_strdup("Default value: REQUIRED");break;
			case XML_ATTRIBUTE_IMPLIED: pstr = g_strdup("Default value: IMPLIED");break;
			case XML_ATTRIBUTE_FIXED: pstr = g_strdup("Default value: FIXED");break;
			default:break;
		}
		if (ptr->defaultValue) {
			if (pstr) {
				tofree = pstr;
				pstr = g_strconcat(pstr," (",ptr->defaultValue,")",NULL);
				g_free(tofree);
			}	
			else pstr = g_strconcat("Default value: ",ptr->defaultValue,NULL);
		}
		if (pstr) {
			node3 = xmlNewNode(NULL,BAD_CAST "description");
			txt = xmlNewText(BAD_CAST pstr);
			xmlAddChild(node3,txt);
			xmlAddChild(node,node3);	 
		  	g_free(pstr);
		 } 
		xmlAddChild(node2,node);
		ptr = ptr->nexth;
	}
	pstr = NULL;
	switch(el->etype) {
		case XML_ELEMENT_TYPE_EMPTY:pstr=g_strdup("Content type: EMPTY");break;
		case XML_ELEMENT_TYPE_ANY:pstr=g_strdup("Content type: ANY");break;
		case XML_ELEMENT_TYPE_MIXED:pstr=g_strdup("Content type: MIXED");break;
		case XML_ELEMENT_TYPE_ELEMENT:pstr=g_strdup("Content type: ELEMENT");break;
		default:break;
	}	
	if (pstr) {
		node3 = xmlNewNode(NULL,BAD_CAST "note");
		xmlNewProp(node3,BAD_CAST "title",BAD_CAST pstr);
		g_free(pstr);
		pstr = g_strdup("");
		pstr = infb_dtd_str_content(el->content,pstr);
		if (pstr) {
			txt = xmlNewText(BAD_CAST pstr);
			xmlAddChild(node3,txt);
		}	
		xmlAddChild(enode,node3);
	}
			
}

void infb_convert_dtd(xmlDocPtr ref) {
	xmlNodePtr node;
	xmlChar *text;
	xmlDtdPtr dtd;
	if (ref==NULL) return;
	node = xmlDocGetRootElement(ref);
	if (xmlStrcmp(node->name,BAD_CAST "ref")!=0) return;
	text = xmlGetProp(node,BAD_CAST "type");
	if (text) {
		if (xmlStrcmp(text,BAD_CAST "dtd")!=0) { xmlFree(text); return; }
		xmlFree(text);
		text = xmlGetProp(node,BAD_CAST "uri");
		if (!text) return;
		dtd = xmlParseDTD(text,text);
		xmlFree(text);
		if (dtd) {
			xmlSetProp(node,BAD_CAST "type",BAD_CAST "fref2");
			xmlHashScan((xmlHashTablePtr)(dtd->elements),infb_dtd_element,ref);
			xmlFreeDtd(dtd);
		}		
	}	
}


