/* Bluefish HTML Editor
 * infb_load.c - plugin for information browser
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
#include <libxml/xpath.h>
#include <libxml/xmlwriter.h>
#include <string.h>
#include "../stringlist.h"
#include "infb_load.h"
#include "infbrowser.h"
#include "infb_text.h"
#include "infb_docbook.h"
#include "infb_html.h"


gchar **infb_load_refname(gchar *filename) {
	xmlDocPtr doc;
	xmlNodePtr cur;
	gchar **ret = g_new0(gchar*,4);
	
	if (filename == NULL)	return NULL;
	doc = xmlReadFile(filename,NULL,XML_PARSE_RECOVER | XML_PARSE_NOENT | XML_PARSE_NOBLANKS | XML_PARSE_XINCLUDE);
	if (doc==NULL) {
		g_warning(_("Cannot load reference file %s\n"),filename);
		g_strfreev(ret);
		return NULL;
	}	
	cur = xmlDocGetRootElement(doc);
	if (!cur) {
		g_strfreev(ret);
		return NULL;
	}
	if (xmlStrcmp(cur->name, BAD_CAST "ref")==0)
	{
		ret[0] = (gchar*)xmlGetProp(cur, BAD_CAST "name");
		ret[1] = (gchar*)xmlGetProp(cur, BAD_CAST "type");
		if (!ret[1]) ret[1] = g_strdup("fref");
		ret[2] = (gchar*)xmlGetProp(cur, BAD_CAST "description");
		if (!ret[2]) ret[2] = g_strdup("");
	}	
	else if (xmlStrcmp(cur->name, BAD_CAST "book")==0) /* docbook? */
	{
		xmlChar *text = infb_db_get_title(doc,FALSE,NULL);
		if (text) {
				ret[0] = g_strdup((gchar*)text);
				xmlFree(text);
		}		
		else
				ret[0] = g_strdup((gchar*)cur->name);
		ret[1] = g_strdup("docbook");
		ret[2] = g_strdup("");		
	}
	else if (xmlStrcmp(cur->name, BAD_CAST "html")==0) /* html? */
	{
		xmlErrorPtr err = xmlGetLastError();
		xmlChar *text;
		if (err) {
			/* try to reload */
				xmlFreeDoc(doc);
      		doc = htmlParseFile(filename,NULL);
      }
  		if (!doc) {
			g_strfreev(ret);
			return NULL;
  		}	
  		else {
			text = infb_html_get_title(doc);
			if (text) {
				ret[0] = g_strdup((gchar*)text);
				xmlFree(text);
			}		
			else
				ret[0] = g_strdup((gchar*)cur->name);
			ret[1] = g_strdup("html");	
		}
		ret[2] = g_strdup("");				
	}			
	else {
		g_strfreev(ret);
		return NULL;
	}	
   if (doc) xmlFreeDoc(doc);   
   return ret;
}

static gboolean reference_file_known(gchar * path)
{
	GList *tmplist = g_list_first(main_v->globses.reference_files);
	while (tmplist) {
		gchar **arr = tmplist->data;
		if (g_strv_length(arr) == 4 && strcmp(arr[1], path) == 0) {
			return TRUE;
		}
		tmplist = g_list_next(tmplist);
	}
	return FALSE;
}


void infb_rescan_dir(const gchar * dir)
{
	/* BUG: deprecated reference files are never removed from the list !! */
	const gchar *filename;
	GError *error = NULL;
	gchar **tofree;
	GPatternSpec *ps = g_pattern_spec_new("bflib_*.xml"); 
	GPatternSpec *ps2 = g_pattern_spec_new("bflib_*.xml.gz"); /* also xml.gz files */
	GDir *gd = g_dir_open(dir, 0, &error);
	filename = g_dir_read_name(gd);
	while (filename) {
		if (g_pattern_match(ps, strlen(filename), filename, NULL) ||
		    g_pattern_match(ps2, strlen(filename), filename, NULL)) {
			gchar *path = g_strconcat(dir, filename, NULL);
			DEBUG_MSG("fref_rescan_dir, filename %s has a match!\n", filename);
			if (!reference_file_known(path)) {
				tofree = infb_load_refname(path);
				DEBUG_MSG("fref_rescan_dir, adding %s:%s to list\n",*tofree,path);
				if (tofree) {
					main_v->globses.reference_files =
						g_list_append(main_v->globses.reference_files,
										  array_from_arglist(tofree[0], path,tofree[1],tofree[2], NULL));
					g_strfreev(tofree);
				}
			}
			g_free(path);
		}
		filename = g_dir_read_name(gd);
	}
	g_dir_close(gd);
	g_pattern_spec_free(ps);
	g_pattern_spec_free(ps2);
}



void infb_load(void) {
	xmlNodePtr node,node2,n_fref,n_dtd,n_html;
	GList *reflist;
	gchar *userdir = g_strconcat(g_get_home_dir(), "/."PACKAGE"/", NULL);

	if ( infb_v.homeDoc != NULL ) xmlFreeDoc(infb_v.homeDoc);
	infb_rescan_dir(PKGDATADIR "/bflib/");
	infb_rescan_dir(userdir);
	g_free(userdir);
	infb_v.homeDoc = xmlNewDoc(BAD_CAST "1.0");	
	node = xmlNewDocNode(infb_v.homeDoc,NULL,BAD_CAST "ref",NULL);
	xmlNewProp(node,BAD_CAST "name",BAD_CAST _("Documentation entries"));
	xmlNewProp(node,BAD_CAST "type",BAD_CAST "index");
	xmlDocSetRootElement(infb_v.homeDoc,node);
	n_fref = xmlNewChild(node,NULL,BAD_CAST "group",BAD_CAST "");
	xmlNewProp(n_fref,BAD_CAST "name",BAD_CAST _("References"));
	n_dtd = xmlNewChild(node,NULL,BAD_CAST "group",BAD_CAST "");
	xmlNewProp(n_dtd,BAD_CAST "name",BAD_CAST _("DTD"));
	n_html = xmlNewChild(node,NULL,BAD_CAST "group",BAD_CAST "");
	xmlNewProp(n_html,BAD_CAST "name",BAD_CAST _("Web pages"));
		
	reflist = g_list_first(main_v->globses.reference_files);
	while (reflist) {
		gchar **tmparray = reflist->data;
		if (g_strv_length(tmparray) == 4) {
			if (access(tmparray[1], R_OK) == 0) {
				if (strcmp(tmparray[2],"dtd")==0)
					node2 = xmlNewChild(n_dtd,NULL,BAD_CAST "fileref",BAD_CAST tmparray[1]);
				else if (strcmp(tmparray[2],"http")==0)
					node2 = xmlNewChild(n_html,NULL,BAD_CAST "fileref",BAD_CAST tmparray[1]);
				else
						node2 = xmlNewChild(n_fref,NULL,BAD_CAST "fileref",BAD_CAST tmparray[1]);		
				xmlNewProp(node2,BAD_CAST "name",BAD_CAST tmparray[0]);
				xmlNewProp(node2,BAD_CAST "type",BAD_CAST tmparray[2]);
				xmlNewProp(node2,BAD_CAST "description",BAD_CAST tmparray[3]);
			}
		} 
		reflist = g_list_next(reflist);
	}
	/*xmlDocDump(stdout,infb_v.homeDoc);*/
}

void infb_reload_home(Tbfwin *win) {
	infb_load();
	infb_v.currentDoc = infb_v.homeDoc;
	infb_fill_doc(win,NULL);	 
}

GList *infb_user_files() {
	GList *ret = NULL;
	xmlXPathObjectPtr result;
	gint i;
	xmlChar *text;
	gchar *str;
	gchar *userdir = g_strconcat(g_get_home_dir(), "/."PACKAGE"/", NULL); 
	
	
	
	if ( !infb_v.homeDoc ) return NULL;
	result = getnodeset (infb_v.homeDoc, BAD_CAST "//fileref",NULL);
	if (result) {
		for(i=0;i<result->nodesetval->nodeNr;i++) {
			text = xmlNodeGetContent(result->nodesetval->nodeTab[i]);
			if (g_str_has_prefix ((gchar*)text,userdir)) {
				str = g_strconcat((gchar*)xmlGetProp(result->nodesetval->nodeTab[i],BAD_CAST "name"),",",(gchar*)text,NULL);
				ret = g_list_append(ret,str);
			}	
		}
		xmlXPathFreeObject (result);
		g_free(userdir);
		return ret;
	}
	g_free(userdir);
	return NULL;
}


static void infb_fragment_activated(GtkMenuItem *menuitem,gpointer data) {
	xmlDocPtr doc;
	gpointer ptr;
	
	ptr = g_object_get_data(G_OBJECT(menuitem),"file");
	if ( !data || !ptr ) return;
	doc = xmlParseFile((gchar*)ptr);
	if (doc) {
		infb_v.currentDoc = doc;
		infb_v.currentNode = NULL;
		infb_fill_doc(BFWIN(data),NULL);
	}
}

void infb_load_fragments(Tinfbwin *win) {
	gchar *userdir = g_strconcat(g_get_home_dir(), "/."PACKAGE"/", NULL);
	const gchar *filename;
	GError *error = NULL;
	GPatternSpec *ps = g_pattern_spec_new("bfrag_*");
	GDir *gd; 
	GtkWidget *menu,*item;
	gint pos=0;

		
	menu = gtk_menu_tool_button_get_menu(GTK_MENU_TOOL_BUTTON(win->saved));
	if ( menu ) gtk_widget_destroy(menu);
	menu = gtk_menu_new();	
	gd = g_dir_open(userdir, 0, &error);
	filename = g_dir_read_name(gd);
	while (filename) {
		if (g_pattern_match(ps, strlen(filename), filename, NULL) ) {
			gchar *path = g_strconcat(userdir, filename, NULL);
			gchar **arr = g_strsplit(filename, "_", -1);
			if (arr && arr[1]) {
				item = gtk_menu_item_new_with_label(arr[1]);
				g_object_set_data(G_OBJECT(item),"file",path); 
				g_signal_connect(G_OBJECT(item),"activate",G_CALLBACK(infb_fragment_activated),win->bfwin);
				gtk_menu_shell_insert(GTK_MENU_SHELL(menu),item,pos);
				pos++;
			}
			g_strfreev(arr);				
		}
		filename = g_dir_read_name(gd);
	}
	g_dir_close(gd);
	g_pattern_spec_free(ps);		
	g_free(userdir);
	gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(win->saved),menu);
	gtk_widget_show_all(menu);
}
