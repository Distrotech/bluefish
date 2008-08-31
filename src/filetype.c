/* Bluefish HTML Editor
 * filetype.c
 *
 * Copyright (C) 2005,2006 Olivier Sessink
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
 * indenting is done with
 * indent --line-length 100 --k-and-r-style --tab-size 4 -bbo --ignore-newlines filetype.c
 */

/* #define DEBUG */

#include "config.h"

#include <string.h>
#include "textstyle.h"
#include "filetype.h"
#include "bluefish.h"
#include "bf-textview.h"
#include "document.h"
#include "stringlist.h"
#include "menu.h"

#ifndef HAVE_ATLEAST_GIO_2_18
char *g_content_type_from_mime_type (const char *type) {
	if (type)
		return g_strdup(type);
	return NULL;
}
#endif

/**
 * icon_for_mime_type:
 * @mime_type: a MIME type
 * @size_hint: the size the caller plans to display the icon at
 *
 * Tries to find an icon representing @mime_type that will display
 * nicely at @size_hint by @size_hint pixels. The returned icon
 * may or may not actually be that size.
 *
 * Return value: a pixbuf, which the caller must unref when it is done
 **/

#if !GTK_CHECK_VERSION(2,14,0)
GdkPixbuf *get_pixbuf_for_gicon(GIcon *icon) {
	static GtkIconTheme *it=NULL;
	GdkPixbuf *pixbuf=NULL;
	
	if (!it) 
		it = gtk_icon_theme_get_default();
	
	if (G_IS_THEMED_ICON(icon)) {
		gchar **tmp;
		g_object_get(icon, "names", &tmp, NULL);
		if (tmp && tmp[0]) {
			GtkIconInfo *ii;
			ii = gtk_icon_theme_choose_icon(it, (const gchar **)tmp, 12, GTK_ICON_LOOKUP_GENERIC_FALLBACK);
			if (ii) {
				pixbuf = gtk_icon_info_load_icon(ii, NULL);
			}
			g_strfreev (tmp);
		}
	}
	return pixbuf;
}
#endif

GdkPixbuf *get_icon_for_mime_type (const char *mime_type) {
	
	GIcon *icon;
	gchar *conttype;
	GdkPixbuf *pixbuf = NULL;

	conttype = g_content_type_from_mime_type(mime_type);
	icon = g_content_type_get_icon(conttype);
	if (icon) {
		pixbuf = get_pixbuf_for_gicon(icon);
		g_object_unref(icon);
	} 
	g_free(conttype);
	return pixbuf;
}

static Tfiletype *filetype_new(const char *mime_type, BfLangConfig *cfg) {
	Tfiletype *filetype;
	/*const gchar *description;*/
	filetype = g_new(Tfiletype, 1);
	DEBUG_MSG("building filetype for %s at %p\n",mime_type,filetype);
	/* BUG: should use contenttype here, not mime_type*/
	filetype->type = g_content_type_get_description(mime_type);
	filetype->icon = get_icon_for_mime_type(mime_type);
	filetype->mime_type = g_strdup(mime_type);

	filetype->cfg = cfg;
	return filetype;
}

/* scans for all *.bflang files in a directory and parses them */
static void filetype_scan_langfiles(const gchar * dir) {
	const gchar *filename;
	GError *error = NULL;
	GPatternSpec *ps = g_pattern_spec_new("*.bflang"); 
	GDir *gd = g_dir_open(dir, 0, &error);
	textstyle_build_lookup_table();
	if (!error) {
		DEBUG_MSG("filetype_scan_langfiles, scanning %s\n",dir);
		filename = g_dir_read_name(gd);
		while (filename) {
			if (g_pattern_match(ps, strlen(filename), filename, NULL)) {
				gchar *path = g_strconcat(dir, filename, NULL);
				DEBUG_MSG("filetype_scan_langfiles, loading %s in dir %s -> %s\n",filename,dir,path);
				bf_lang_mgr_load_config(path);
				g_free(path);
			}
			filename = g_dir_read_name(gd);
		}
		g_dir_close(gd);
	}
	g_pattern_spec_free(ps);
	textstyle_cleanup_lookup_table();
}

/* retrieves a Tfiletype for the given mime-type, if none is found, a 
new filetype is created */
Tfiletype *get_filetype_for_mime_type(const gchar *mime_type) {
	
	if (!mime_type) return NULL; 
	else {
		Tfiletype *ft;
		ft = g_hash_table_lookup(main_v->filetypetable, mime_type);
		if (ft == NULL) {
			DEBUG_MSG("get_filetype_for_mime_type, %s is not in hashtable, creating new filetype %p\n",mime_type,ft);
			ft = filetype_new(mime_type, NULL);
			g_hash_table_replace(main_v->filetypetable, ft->mime_type, ft);
			main_v->filetypelist = g_list_prepend(main_v->filetypelist, ft);
		}
		return ft;
	}
}

const gchar *get_mimetype_for_uri(GFile *uri, GFileInfo *finfo, gboolean fast) {
	GFileInfo *rfinfo;
	const gchar *mime;
	const gchar *attrib = fast ? "standard::fast-content-type":"standard::content-type";
	if (finfo && g_file_info_has_attribute(finfo, attrib)) {
		rfinfo = finfo;
		g_object_ref(rfinfo);
	} else {
		GError *error=NULL;
		rfinfo = g_file_query_info(uri,attrib
				,G_FILE_QUERY_INFO_NONE,NULL,&error);
	}
	mime = g_file_info_get_attribute_string(rfinfo, attrib);
	g_object_unref(rfinfo);
	return mime;
}
Tfiletype *get_filetype_for_uri(GFile *uri, GFileInfo *finfo, gboolean fast) {
	const gchar *mimetype = get_mimetype_for_uri(uri, finfo, fast);
	if (mimetype) {
		return get_filetype_for_mime_type(mimetype);
	}
	return NULL;
}


/*
 * This is modifed function for scanner environment.
 * if gui_errors is set, we can send a popup with an error message,
 * else (during startup) we use g_print()
 */
void filetype_highlighting_rebuild(gboolean gui_errors)
{
	GList *tmplist;
	GList *alldoclist;
	Tfiletype *filetype;
	
	if (main_v->filetypetable == NULL) {
		main_v->filetypetable = g_hash_table_new(g_str_hash,g_str_equal);
	}
	
	alldoclist = return_allwindows_documentlist();
	/* remove filetypes from documents, but to reconnect them 
	   again after the rebuild, we temporary put a string with 
	   the filetype on that pointer */
	if (alldoclist) {
		tmplist = g_list_first(alldoclist);
		while (tmplist) {
			Tdocument *thisdoc = (Tdocument *) tmplist->data;
			if (thisdoc->hl) {
				DEBUG_MSG("doc %p has type %p named %s\n", thisdoc, thisdoc->hl, thisdoc->hl->type);
				DEBUG_MSG("disconnected document %p from filetype %s\n", thisdoc,
						  thisdoc->hl->type);
				thisdoc->hl = (gpointer) g_strdup(thisdoc->hl->type);
			}
			tmplist = g_list_next(tmplist);
		}
	}
	DEBUG_MSG("filetype_highlighting_rebuild, rebuilding the filetype list\n");
	/* now rebuild the filetype list */
	/* parse language files */
	filetype_scan_langfiles(PKGDATADIR"/bflang/");
	{
		gchar *userdir = g_strconcat(g_get_home_dir(), "/."PACKAGE"/", NULL);
		filetype_scan_langfiles(userdir);
		g_free(userdir);
	}
	/* build all mime types */
/*	filetype = filetype_new("text/plain", NULL);
	main_v->filetypelist = g_list_append(main_v->filetypelist, filetype);
	g_hash_table_replace(main_v->filetypetable, "text/plain", filetype);*/
	tmplist = g_list_first(main_v->lang_mgr->languages);
	while (tmplist) { /* loop all language configs */
		GList *tmplist2;
		BfLangConfig *cfg = tmplist->data;
		tmplist2 = g_list_first(cfg->mimetypes);
		while (tmplist2) { /* loop all mime-types in this language config */
			gpointer tmp; /* test if this mime-type is already known */
			tmp = g_hash_table_lookup(main_v->filetypetable, (gchar *)tmplist2->data);
			if (!tmp) { /* not known: register this Tfiletype for this mime-type */
				filetype = filetype_new(tmplist2->data, cfg);
				main_v->filetypelist = g_list_append(main_v->filetypelist, filetype);
				g_hash_table_replace(main_v->filetypetable, (gchar *)tmplist2->data, filetype);
			}
			tmplist2 = g_list_next(tmplist2);
		}
		tmplist = g_list_next(tmplist);
	}

	/* now we have finished the rebuilding of the filetypes, we 
	   have to connect all the documents with their filetypes again, we 
	   stored the name of the filetype temporary in the place of the Tfiletype,
	   undo that now */
	if (alldoclist) {
		tmplist = g_list_first(alldoclist);
		while (tmplist) {
			if (((Tdocument *) tmplist->data)->hl) {
				gchar *tmpstr = (gchar *) ((Tdocument *) tmplist->data)->hl;
				((Tdocument *) tmplist->data)->hl = get_filetype_by_name(tmpstr);
				DEBUG_MSG("reconnecting document %p to filetype %s\n", tmplist->data, tmpstr);
				g_free(tmpstr);
				((Tdocument *) tmplist->data)->need_highlighting = TRUE;
			}
			tmplist = g_list_next(tmplist);
		}
	}
	g_list_free(alldoclist);
}
