/* Bluefish HTML Editor
 * infb_load.c - plugin for information browser
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

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xmlwriter.h>
#include <string.h>
#include "../stringlist.h"
#include "infb_load.h"
#include "infbrowser.h"
#include "infb_text.h"


gchar *infb_load_refname(gchar *filename) {
	xmlDocPtr doc;
	xmlNodePtr cur;
	gchar *ret;
	
	if (filename == NULL)	return NULL;
	doc = xmlParseFile(filename);
	if (doc==NULL) {
		g_warning(_("Cannot load reference file %s\n"),filename);
		return NULL;
	}	
	cur = xmlDocGetRootElement(doc);	
	if ( xmlStrcmp(cur->name, (const xmlChar *) "ref") != 0  && xmlStrcmp(cur->name, (const xmlChar *) "r") != 0) {
		g_warning(_("Bad root element in reference file %s\nShould be <ref> or <r> and found %s"),filename,cur->name);
		xmlFreeDoc(doc);
		return NULL;
	}
	if (xmlStrcmp(cur->name, (const xmlChar *) "ref")==0)
		ret = (gchar*)xmlGetProp(cur, (const xmlChar *) "name");
	else
		ret = (gchar*)xmlGetProp(cur, (const xmlChar *) "n");	
   xmlFreeDoc(doc);
   return ret;
}

static gboolean reference_file_known(gchar * path)
{
	GList *tmplist = g_list_first(main_v->globses.reference_files);
	while (tmplist) {
		gchar **arr = tmplist->data;
		if (count_array(arr) == 2 && strcmp(arr[1], path) == 0) {
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
	gchar *tofree;
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
				DEBUG_MSG("fref_rescan_dir, adding %s:%s to list\n",tofree,path);
				main_v->globses.reference_files =
					g_list_append(main_v->globses.reference_files,
								  array_from_arglist(tofree, path, NULL));
				g_free(tofree);
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
	xmlNodePtr node,node2;
	xmlDtdPtr dtd;
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
	dtd = xmlCreateIntSubset(infb_v.homeDoc, BAD_CAST "index", BAD_CAST INFB_INDEX_PUBLIC_ID, NULL);
	reflist = g_list_first(main_v->globses.reference_files);
	while (reflist) {
		gchar **tmparray = reflist->data;
		if (count_array(tmparray) == 2) {
			if (access(tmparray[1], R_OK) == 0) {
				node2 = xmlNewChild(node,NULL,BAD_CAST "fileref",BAD_CAST tmparray[1]);
				xmlNewProp(node2,BAD_CAST "name",BAD_CAST tmparray[0]);
				if (access(tmparray[1], W_OK) == 0) {
					xmlNewProp(node2,BAD_CAST "ro",BAD_CAST "0");
				} else {
					xmlNewProp(node2,BAD_CAST "ro",BAD_CAST "1");
				}				   
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


void infb_load_fragments() {
	gchar *userdir = g_strconcat(g_get_home_dir(), "/."PACKAGE"/", NULL);
	const gchar *filename;
	GError *error = NULL;
	GPatternSpec *ps = g_pattern_spec_new("bfrag_*");
	GDir *gd; 
	GtkTreeIter iter;
		
	if ( infb_v.saved_store!=NULL ) {
		gtk_list_store_clear(infb_v.saved_store);
		gd = g_dir_open(userdir, 0, &error);
		filename = g_dir_read_name(gd);
		while (filename) {
			if (g_pattern_match(ps, strlen(filename), filename, NULL) ) {
				gchar *path = g_strconcat(userdir, filename, NULL);
				gchar **arr = g_strsplit(filename, "_", -1);
				if (arr && arr[1]) {
					gtk_list_store_append(infb_v.saved_store, &iter);
					gtk_list_store_set(infb_v.saved_store, &iter, 0, arr[1], 1, path, -1);
				}
				g_strfreev(arr);				
				/*g_free(path);*/
			}
			filename = g_dir_read_name(gd);
		}
		g_dir_close(gd);
		g_pattern_spec_free(ps);		
	} 
	g_free(userdir);
}
