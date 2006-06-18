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
#ifdef USE_SCANNER

#include <libgnomeui/libgnomeui.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>

#include <string.h>

#include "filetype.h"
#include "bluefish.h"
#include "bf-textview.h"
#include "document.h"
#include "stringlist.h"
#include "menu.h"

/*void hl_token_slot(BfTextView * view, BfLangToken * tokenDef, GtkTextIter * startIter,
				   GtkTextIter * endIter)
{
	GtkTextTag *tag = NULL;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	GList *lst = g_list_first(main_v->bfwinlist);
	Tdocument *doc = NULL;

	/ * first find current document * /
	while (lst) {
		Tbfwin *bf = (Tbfwin *) lst->data;
		if (bf->current_document->view == GTK_WIDGET(view)) {
			doc = bf->current_document;
			break;
		}
		lst = g_list_next(lst);
	}
	if (doc == NULL)
		return;
	tag = g_hash_table_lookup(doc->hl->hl_token, tokenDef->name);
	if (!tag && tokenDef->group)
		tag = g_hash_table_lookup(doc->hl->hl_group, tokenDef->group);
	if (!tag)
		return;
	gtk_text_buffer_apply_tag(buffer, tag, startIter, endIter);
}

void hl_block_slot(BfTextView * view, BfLangBlock * blockDef, GtkTextIter * b_startIter,
				   GtkTextIter * b_endIter, GtkTextIter * e_startIter, GtkTextIter * e_endIter,
				   GtkTextMark * startMark, GtkTextMark * endMark)
{
	GtkTextTag *tag = NULL;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	GList *lst = g_list_first(main_v->bfwinlist);
	Tdocument *doc = NULL;

	/ * first find current document * /
	while (lst) {
		Tbfwin *bf = (Tbfwin *) lst->data;
		if (bf->current_document->view == GTK_WIDGET(view)) {
			doc = bf->current_document;
			break;
		}
		lst = g_list_next(lst);
	}
	if (doc == NULL)
		return;
	tag = g_hash_table_lookup(doc->hl->hl_block, blockDef->id);
	if (!tag && blockDef->group)
		tag = g_hash_table_lookup(doc->hl->hl_group, blockDef->group);
	if (!tag)
		return;
	gtk_text_buffer_apply_tag(buffer, tag, b_startIter, e_endIter);
}

void hl_tag_begin_slot(BfTextView * view, gchar * tagName, GtkTextIter * startIter,
					   GtkTextIter * endIter)
{
	GtkTextTag *tag = NULL;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	GList *lst = g_list_first(main_v->bfwinlist);
	Tdocument *doc = NULL;

	/ * first find current document * /
	while (lst) {
		Tbfwin *bf = (Tbfwin *) lst->data;
		if (bf->current_document->view == GTK_WIDGET(view)) {
			doc = bf->current_document;
			break;
		}
		lst = g_list_next(lst);
	}
	if (doc == NULL)
		return;
	tag = g_hash_table_lookup(doc->hl->hl_tag, "tag_begin");
	if (!tag)
		return;
	gtk_text_buffer_apply_tag(buffer, tag, startIter, endIter);
}

void hl_tag_end_slot(BfTextView * view, gchar * tagName, GtkTextIter * b_startIter,
					 GtkTextIter * b_endIter, GtkTextIter * e_startIter, GtkTextIter * e_endIter)
{
	GtkTextTag *tag = NULL;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	GList *lst = g_list_first(main_v->bfwinlist);
	Tdocument *doc = NULL;

	/ * first find current document * /
	while (lst) {
		Tbfwin *bf = (Tbfwin *) lst->data;
		if (bf->current_document->view == GTK_WIDGET(view)) {
			doc = bf->current_document;
			break;
		}
		lst = g_list_next(lst);
	}
	if (doc == NULL)
		return;
	tag = g_hash_table_lookup(doc->hl->hl_tag, "tag_end");
	if (!tag)
		return;
	gtk_text_buffer_apply_tag(buffer, tag, e_startIter, e_endIter);
}

void hl_tag_attr_slot(BfTextView * view, gchar * attrName, gchar * attrValue,
					  GtkTextIter * n_startIter, GtkTextIter * n_endIter, GtkTextIter * v_startIter,
					  GtkTextIter * v_endIter)
{
	GtkTextTag *tag = NULL;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	GList *lst = g_list_first(main_v->bfwinlist);
	Tdocument *doc = NULL;

	/ * first find current document * /
	while (lst) {
		Tbfwin *bf = (Tbfwin *) lst->data;
		if (bf->current_document->view == GTK_WIDGET(view)) {
			doc = bf->current_document;
			break;
		}
		lst = g_list_next(lst);
	}
	if (doc == NULL)
		return;
	tag = g_hash_table_lookup(doc->hl->hl_tag, "attr_name");
	if (tag)
		gtk_text_buffer_apply_tag(buffer, tag, n_startIter, n_endIter);
	tag = g_hash_table_lookup(doc->hl->hl_tag, "attr_val");
	if (tag)
		gtk_text_buffer_apply_tag(buffer, tag, v_startIter, v_endIter);
}*/

/*
gboolean filetype_clear_tags(gpointer key,gpointer value,gpointer data) {
   gtk_text_tag_table_remove(highlight.tagtable,GTK_TEXT_TAG(value));
	return TRUE;
}*/
#ifdef GNOMEVFSINT

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
static GdkPixbuf *get_icon_for_mime_type (const char *mime_type) {
	static GtkIconTheme *icon_theme = NULL;
	char *icon_name;
	GdkPixbuf *pixbuf = NULL;

	/* Try the icon theme. (GNOME 2.2 or Sun GNOME 2.0).
	 * This will also look in GNOME VFS.
	 */
	
	if (!icon_theme)
		icon_theme = gtk_icon_theme_get_default();	
	
	icon_name = gnome_icon_lookup (icon_theme, NULL, NULL, NULL, NULL,
				       mime_type, 0, NULL);
	if (icon_name) {
		GError *error=NULL;
		DEBUG_MSG("get_icon_for_mime_type, got %s for %s\n",icon_name,mime_type);
		pixbuf = gtk_icon_theme_load_icon (icon_theme, icon_name, main_v->props.filebrowser_icon_size, GTK_ICON_LOOKUP_USE_BUILTIN, &error);
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

static Tfiletype *filetype_new(const char *mime_type, BfLangConfig *cfg) {
	Tfiletype *filetype;
	const gchar *description;
	filetype = g_new(Tfiletype, 1);
	DEBUG_MSG("building filetype for %s\n",mime_type);
	description = gnome_vfs_mime_get_description(mime_type);
	filetype->type = g_strdup(description?description:"");
	filetype->mime_type = g_strdup(mime_type);
	filetype->icon = get_icon_for_mime_type(mime_type);
	filetype->cfg = cfg;
	return filetype;
}
#endif

/* scans for all *.bflang files in a directory and parses them */
static void filetype_scan_langfiles(const gchar * dir) {
	const gchar *filename;
	GError *error = NULL;
	GPatternSpec *ps = g_pattern_spec_new("*.bflang"); 
	GDir *gd = g_dir_open(dir, 0, &error);
	filename = g_dir_read_name(gd);
	while (filename) {
		if (g_pattern_match(ps, strlen(filename), filename, NULL)) {
			gchar *path = g_strconcat(dir, filename, NULL);
			bf_lang_mgr_load_config(path);
			g_free(path);
		}
		filename = g_dir_read_name(gd);
	}
	g_dir_close(gd);
	g_pattern_spec_free(ps);
}

/* retrieves a Tfiletype for the given mime-type, if none is found, a 
new filetype is created */
Tfiletype *get_filetype_for_mime_type(const gchar *mime_type) {
	Tfiletype *ft;
	ft = g_hash_table_lookup(main_v->filetypetable, mime_type);
	if (!ft) {
		ft = filetype_new(mime_type, NULL);
		g_hash_table_replace(main_v->filetypetable, (const gpointer) mime_type, ft);
	}
	return ft;
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

	/* first remove the menu widgets, and delete the filetype structs */
	DEBUG_MSG("filetype_highlighting_rebuild, testing for filetypelist existance\n");
	filetype_menus_empty();
	tmplist = g_list_first(main_v->filetypelist);
	while (tmplist) {
		filetype = (Tfiletype *) tmplist->data;
		g_free(filetype->type);
#ifdef GNOMEVFSINT
		g_free(filetype->mime_type);
#else
		g_strfreev(filetype->extensions);
		g_free(filetype->update_chars);
		g_free(filetype->content_regex);
#endif
		if (filetype->icon) {
			g_object_unref(filetype->icon);
		}
		/* the highlightpatterns are freed separately, see below */
		g_free(filetype);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(main_v->filetypelist);
	main_v->filetypelist = NULL;

	DEBUG_MSG("filetype_highlighting_rebuild, rebuilding the filetype list\n");
	/* now rebuild the filetype list */
#ifdef GNOMEVFSINT
	/* parse language files */
	filetype_scan_langfiles(PKGDATADIR);
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
#else
	tmplist = g_list_first(main_v->props.filetypes);
	while (tmplist) {
		gint arrcount;
		gchar **strarr;
		strarr = (gchar **) tmplist->data;
		arrcount = count_array(strarr);
		if (arrcount == 8) {
			filetype = g_new(Tfiletype, 1);
			filetype->type = g_strdup(strarr[0]);
			filetype->editable = (strarr[4][0] != '0');
			filetype->content_regex = g_strdup(strarr[5]);
			filetype->autoclosingtag = atoi(strarr[6]);
			DEBUG_MSG("extensions for %s loaded from %s\n", strarr[0], strarr[1]);
			filetype->extensions = g_strsplit(strarr[1], ":", 127);
			filetype->update_chars = g_strdup(strarr[2]);
			if (strlen(strarr[3])) {
				GError *error = NULL;
				filetype->icon = gdk_pixbuf_new_from_file(strarr[3], &error);
				if (error != NULL) {
					/* Report error to user, and free error */
					g_print("ERROR while loading icon: %s\n", error->message);
					g_error_free(error);
					filetype->icon = NULL;
				}
			} else {
				filetype->icon = NULL;
			}
			/* load configuration */
			filetype->language_file = g_strdup(strarr[7]);
			if (filetype->language_file && filetype->language_file[0] != '\0') {
				gint i = 0;
				DEBUG_MSG("filetype_highlighting_rebuild, get config for %s\n",filetype->type);
				filetype->cfg = bf_lang_mgr_get_config(main_v->lang_mgr, filetype->language_file);
				if (!filetype->cfg) {
					DEBUG_MSG("filetype_highlighting_rebuild, loading %s(%p) from %s\n",filetype->type,filetype,filetype->language_file);
					filetype->cfg = bf_lang_mgr_load_config(main_v->lang_mgr, filetype->language_file);
				}
				if (filetype->cfg) {
					gchar *p = filetype->update_chars;
					i = 0;
					while (i < g_utf8_strlen(filetype->update_chars, -1)) {
						filetype->cfg->as_triggers[(gint) * p] = 1;
						i++;
						p = g_utf8_next_char(p);
					}
				}
			} else {
				filetype->cfg = NULL;
			}
			filetype->highlightlist = NULL;
/*			filetype->hl_block = g_hash_table_new(g_str_hash, g_str_equal);
			filetype->hl_token = g_hash_table_new(g_str_hash, g_str_equal);
			filetype->hl_tag = g_hash_table_new(g_str_hash, g_str_equal);
			filetype->hl_group = g_hash_table_new(g_str_hash, g_str_equal);*/
			
/*			filetype->tag_begin = get_tag_for_scanner_style(filetype->type,"m","tag_begin");
			filetype->tag_end = get_tag_for_scanner_style(filetype->type,"m","tag_end");
			filetype->attr_name = get_tag_for_scanner_style(filetype->type,"m","attr_name");
			filetype->attr_val = get_tag_for_scanner_style(filetype->type,"m","attr_val");*/
			
			main_v->filetypelist = g_list_append(main_v->filetypelist, filetype);
/*			tmplist2 = g_list_first(main_v->props.highlight_patterns);
			while (tmplist2) {
				gchar **strarr;
				strarr = (gchar **) tmplist2->data;
				if (strcmp(filetype->type, strarr[0]) == 0) {
					create_style(filetype, strarr[1], strarr[2], strarr[3], strarr[4],
								 atoi(strarr[5]), atoi(strarr[6]));
				}
				tmplist2 = g_list_next(tmplist2);
			}*/

		}

		tmplist = g_list_next(tmplist);
	}
#endif

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



#endif							/* SCANNER */

