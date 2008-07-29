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
#ifdef HAVE_ATLEAST_GIO_2_16
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
			ii = gtk_icon_theme_choose_icon(it, tmp, 12, GTK_ICON_LOOKUP_GENERIC_FALLBACK);
			if (ii) {
				pixbuf = gtk_icon_info_load_icon(ii, NULL);
			}
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

#else
GdkPixbuf *get_icon_for_mime_type (const char *mime_type) {
	static GtkIconTheme *icon_theme = NULL;
	gchar *icon_name = NULL;
	GdkPixbuf *pixbuf = NULL;

	/* Try the icon theme. (GNOME 2.2 or Sun GNOME 2.0).
	 * This will also look in GNOME VFS.
	 */
	
	if (!icon_theme)
		icon_theme = gtk_icon_theme_get_default();
		/*icon_theme = gtk_icon_theme_get_for_screen(widget));*/	

#ifdef HAVE_LIBGNOMEUI_LIBGNOMEUI_H
	icon_name = gnome_icon_lookup (icon_theme, NULL, NULL, NULL, NULL,
				mime_type, 0, NULL);
#else /* HAVE_LIBGNOMEUI_LIBGNOMEUI_H */
	if (mime_type != NULL) {
		gchar *mime_type_without_slashes, *tmp;

		mime_type_without_slashes = g_strdup (mime_type);
		while ((tmp = strchr(mime_type_without_slashes, '/')) != NULL)
			*tmp = '-';
		icon_name = g_strdup (mime_type_without_slashes);

		/* TODO: add gnome_vfs_mime_get_icon () call too? */
		if (!gtk_icon_theme_has_icon (icon_theme, icon_name))
			icon_name = g_strconcat ("gnome-mime-", icon_name, NULL);
			if (!gtk_icon_theme_has_icon (icon_theme, icon_name))
				icon_name = NULL;

		g_free (mime_type_without_slashes);
	}
#endif /* HAVE_LIBGNOMEUI_LIBGNOMEUI_H */
	if (!icon_name) {
		/* fall back to the default icon */
		if (strncmp(mime_type,"x-directory",11)==0) {
			icon_name = g_strdup(GTK_STOCK_DIRECTORY);
		} else {
			icon_name = g_strdup(GTK_STOCK_FILE);
		}
	}
	if (icon_name) {
		GError *error=NULL;
		DEBUG_MSG("get_icon_for_mime_type, got %s for %s\n",icon_name,mime_type);
		pixbuf = gtk_icon_theme_load_icon(icon_theme, icon_name, main_v->props.filebrowser_icon_size, GTK_ICON_LOOKUP_USE_BUILTIN, &error);
		if (!pixbuf) {
    		g_warning ("Couldn't load icon: %s", error->message);
    		g_error_free (error);
		}
		g_free (icon_name);
	} else {
		return NULL; /* perhaps we shopuld return some default icon ? */
	}
	return pixbuf;
}
#endif /* HAVE_ATLEAST_GIO_2_16 */

static Tfiletype *filetype_new(const char *mime_type, BfLangConfig *cfg) {
	Tfiletype *filetype;
	const gchar *description;
	filetype = g_new(Tfiletype, 1);
	DEBUG_MSG("building filetype for %s at %p\n",mime_type,filetype);
#ifdef HAVE_ATLEAST_GIO_2_16
	/* BUG: should use contenttype here, not mime_type*/
	filetype->type = g_content_type_get_description(mime_type);
#else /* no HAVE_ATLEAST_GIO_2_16  */
	description = gnome_vfs_mime_get_description(mime_type);
	filetype->type = g_strdup(description?description:"");
#endif /* else HAVE_ATLEAST_GIO_2_16 */
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

#ifdef HAVE_ATLEAST_GIO_2_16
gchar *get_mimetype_for_uri(GFile *uri, GFileInfo *finfo, gboolean fast) {
	GFileInfo *rfinfo;
	gchar *mime;
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

#else /* no HAVE_ATLEAST_GIO_2_16  */
const gchar *get_mimetype_for_uri(GnomeVFSURI *uri, gboolean fast) {
	GnomeVFSFileInfo *info;
	GnomeVFSResult res;
	const gchar *retval=NULL;
	
	info = gnome_vfs_file_info_new();
	DEBUG_MSG("get_mimetype_for_uri, uri(%p)=%s\n",uri,gnome_vfs_uri_extract_short_name(uri));
	res = gnome_vfs_get_file_info_uri(uri,info,fast?GNOME_VFS_FILE_INFO_GET_MIME_TYPE|GNOME_VFS_FILE_INFO_FOLLOW_LINKS|GNOME_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE:GNOME_VFS_FILE_INFO_GET_MIME_TYPE|GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	DEBUG_MSG("get_mimetype_for_uri, res=%d\n",res);
	if (res == GNOME_VFS_OK) {
		retval = gnome_vfs_file_info_get_mime_type(info);
	}
	gnome_vfs_file_info_unref(info);
	return retval;
}
Tfiletype *get_filetype_for_uri(GnomeVFSURI *uri, gboolean fast) {
	const gchar *mimetype = get_mimetype_for_uri(uri, fast);
	if (mimetype) {
		return get_filetype_for_mime_type(mimetype);
	}
	return NULL;
}
#endif /* else HAVE_ATLEAST_GIO_2_16 */


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
