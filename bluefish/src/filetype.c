/* Bluefish HTML Editor
 * filetype.c
 *
 * Copyright (C) 2005 Olivier Sessink
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
#define DEBUG

#include "config.h"
#ifdef USE_SCANNER

#include <string.h>

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


/*
 * This is modifed function for scanner environment.
 * if gui_errors is set, we can send a popup with an error message,
 * else (during startup) we use g_print()
 */
void filetype_highlighting_rebuild(gboolean gui_errors)
{
	GList *tmplist;
	GList *alldoclist;

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
		Tfiletype *filetype = (Tfiletype *) tmplist->data;
		g_free(filetype->type);
		g_strfreev(filetype->extensions);
		g_free(filetype->update_chars);
		if (filetype->icon) {
			g_object_unref(filetype->icon);
		}
		g_free(filetype->content_regex);
/*		g_hash_table_foreach_remove(filetype->hl_block,filetype_clear_tags,NULL);
		g_hash_table_foreach_remove(filetype->hl_token,filetype_clear_tags,NULL);
		g_hash_table_foreach_remove(filetype->hl_tag,filetype_clear_tags,NULL);
		g_hash_table_foreach_remove(filetype->hl_group,filetype_clear_tags,NULL);*/
		/* the highlightpatterns are freed separately, see below */
		g_free(filetype);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(main_v->filetypelist);
	main_v->filetypelist = NULL;

	DEBUG_MSG("filetype_highlighting_rebuild, rebuilding the filetype list\n");
	/* now rebuild the filetype list */
	tmplist = g_list_first(main_v->props.filetypes);
	while (tmplist) {
		gint arrcount;
		gchar **strarr;
		Tfiletype *filetype;
		strarr = (gchar **) tmplist->data;
		arrcount = count_array(strarr);
		if (arrcount == 8) {
			filetype = g_new(Tfiletype, 1);
			filetype->editable = (strarr[4][0] != '0');
			filetype->content_regex = g_strdup(strarr[5]);
			filetype->type = g_strdup(strarr[0]);
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
				filetype->cfg = bf_lang_mgr_get_config(main_v->lang_mgr, filetype->type);
				if (!filetype->cfg) {
					DEBUG_MSG("filetype_highlighting_rebuild, loading %s(%p) from %s\n",filetype->type,filetype,filetype->language_file);
					filetype->cfg = bf_lang_mgr_load_config(main_v->lang_mgr, filetype->language_file, filetype);
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
