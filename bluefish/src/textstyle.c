/* Bluefish HTML Editor
 * textstyle.c - color styles used for syntax highlighting
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

/* #define DEBUG */

#include <glib.h>
#include <gtk/gtk.h>
#include <string.h> /* strlen */
#include <stdlib.h> /* atoi */
#include "config.h"
#include "bluefish.h"
#include "textstyle.h"
#include "stringlist.h"
typedef struct {
	GtkTextTagTable *tagtable;
	GHashTable *lookup_table;
} Ttextstyle;

static Ttextstyle textstyle = {NULL,NULL};

/* these hash functions hash the first 3 strings in a gchar ** */
static gboolean arr3_equal (gconstpointer v1,gconstpointer v2)
{
	const gchar **arr1 = (const gchar **)v1;
	const gchar **arr2 = (const gchar **)v2;
  
	return ((strcmp ((char *)arr1[0], (char *)arr2[0]) == 0 )&&
			(strcmp ((char *)arr1[1], (char *)arr2[1]) == 0) &&
			(strcmp ((char *)arr1[2], (char *)arr2[2]) == 0));
}
static guint arr3_hash(gconstpointer v)
{
  /* modified after g_str_hash */
  	const signed char **tmp = (const signed char **)v;
	const signed char *p;
	guint32 h = *tmp[0];
	if (h) {
		gshort i=0;
		while (i<3) {
			p = tmp[i];
			for (p += 1; *p != '\0'; p++)
				h = (h << 5) - h + *p;
			i++;
		}
	}
	return h;
}


void textstyle_build_lookup_table(void) {
	GList *tmplist;
	textstyle.lookup_table = g_hash_table_new(arr3_hash,arr3_equal);
	for (tmplist = g_list_first(main_v->props.syntax_styles);tmplist;tmplist=g_list_next(tmplist)) {
		const gchar **tmp = tmplist->data;
		g_hash_table_insert(textstyle.lookup_table,tmp,tmp[3]);
	}
}

void textstyle_cleanup_lookup_table(void) {
	g_hash_table_unref(textstyle.lookup_table);
	textstyle.lookup_table = NULL;
}

static GtkTextTag *textstyle_compile(gchar **arr) {
	GtkTextTag *tag;
	gint weight, style;

	tag = gtk_text_tag_new(arr[0]);
	if (strlen(arr[1])) {
		g_object_set(tag, "foreground", arr[1], NULL);
	}
	if (strlen(arr[2])) {
		g_object_set(tag, "background", arr[2], NULL);
	}
	weight = atoi(arr[3]);
	if (weight > 0) {
		if (1 == weight) {
			g_object_set(tag, "weight", PANGO_WEIGHT_NORMAL, NULL);
		} else {
			g_object_set(tag, "weight", PANGO_WEIGHT_BOLD, NULL);
		}
	}
	style = atoi(arr[4]);
	if (style > 0) {
		if (1 == style) {
			g_object_set(tag, "style", PANGO_STYLE_NORMAL, NULL);
		} else {
			g_object_set(tag, "style", PANGO_STYLE_ITALIC, NULL);
		}
	}
	return tag;
}

static void addtolist_lcb(GtkTextTag *tag,gpointer data) {
	*((GList **)data) = g_list_append(*((GList **)data),tag);
}

void textstyle_rebuild(void) {
	GList *tmplist=NULL;
	DEBUG_MSG("textstyle_rebuild, started\n");
	if (textstyle.tagtable) {
		/* cleanup the existing tags */
		gtk_text_tag_table_foreach(textstyle.tagtable,addtolist_lcb,&tmplist);
		for (tmplist=g_list_first(tmplist);tmplist!=NULL;tmplist=tmplist->next) {
/*			GtkTextTag *tag = tmplist->data;
			if (strcmp(tag->name, "_folded_") !=0 && strcmp(tag->name, "_fold_header_") !=0 && strcmp(tag->name, "_block_") !=0 && strcmp(tag->name, "_block_match_") !=0) {*/
				DEBUG_MSG("textstyle_rebuild, removing tag %p from tagtable %p\n",tmplist->data,textstyle.tagtable);
				gtk_text_tag_table_remove(textstyle.tagtable,tmplist->data);
/*			} else {
				DEBUG_MSG("textstyle_rebuild, not removing %s\n",tag->name);
			}*/
		}
		g_list_free(tmplist);
	} else {
		textstyle.tagtable = gtk_text_tag_table_new();
		DEBUG_MSG("textstyle_rebuild, tagtable at %p\n",textstyle.tagtable);
	}
	/* we want the first entry in the list to have the highest priority, so we'll
	start with the last textstyle, and end with the first one, because the last GtkTextTag has
	the highest priority */
	for (tmplist=g_list_last(main_v->props.textstyles);tmplist!=NULL;tmplist=tmplist->prev) {
		gchar **arr = tmplist->data;
		GtkTextTag *tag;
		tag = textstyle_compile(arr);
		DEBUG_MSG("textstyle_rebuild, adding tag %p to tagtable %p\n",tag,textstyle.tagtable);
		gtk_text_tag_table_add(textstyle.tagtable, tag);
		g_object_unref(tag);
	}
}

GtkTextTag *textstyle_get(const gchar *name) {
	return gtk_text_tag_table_lookup(textstyle.tagtable, name);
}

GtkWidget* textstyle_combo_box(GList *configstyles) {
	GtkWidget *retwid;
	GList *tmplist;
	
	retwid = gtk_combo_box_new_text();
	for (tmplist=g_list_first(configstyles);tmplist!=NULL;tmplist=tmplist->next) {
		gchar **arr = tmplist->data;
		gtk_combo_box_append_text(GTK_COMBO_BOX(retwid), arr[0]);
	}
	return retwid;
}
GtkTextTagTable *textstyle_return_tagtable(void) {
	return textstyle.tagtable;
}

static gchar *get_tagname_for_scanner_style(const gchar *filetype,const gchar *type,const gchar *name) {
	const gchar *arr[] = {filetype, type, name, NULL};
	if (textstyle.lookup_table) {
		return g_hash_table_lookup(textstyle.lookup_table, arr);
	} else {
		gchar **tmparr = arraylist_value_exists(main_v->props.syntax_styles, arr, 3, TRUE);
		g_print("get_tagname_for_scanner_style: NO LOOKUP TABLE, SLOWWWWW!!!!!\n");
		if (tmparr)
			return tmparr[3];
	}
	return NULL;	
}

GtkTextTag *get_tag_for_scanner_style(const gchar *filetype,const gchar *type,const gchar *name, const gchar *defaultstyle) {
	GtkTextTag *tag=NULL;

	const gchar *tagname=get_tagname_for_scanner_style(filetype,type,name);	

	if (tagname) {
		/* the style for this element is set in the config */
		if (tagname[0] != '\0') {
			tag = textstyle_get(tagname);
		}
		DEBUG_MSG("get_tag_for_scanner_style(%s:%s:%s) return tag %p for textstyle %s\n",filetype,type,name,tag,tagname);
		return tag;
	} else if (defaultstyle && defaultstyle[0] != '\0'){
		/* try the default style */
		tag = textstyle_get(defaultstyle);
		if (tag) {
			gchar **arr2 = array_from_arglist(filetype,type,name,defaultstyle,NULL);
			main_v->props.syntax_styles = g_list_append(main_v->props.syntax_styles,arr2);
		}
		DEBUG_MSG("get_tag_for_scanner_style, return default style %s for %s:%s:%s\n",defaultstyle,filetype,type,name);
		return tag;
	}
	DEBUG_MSG("no config found for %s:%s:%s\n",filetype,type,name);
	return NULL;
}
