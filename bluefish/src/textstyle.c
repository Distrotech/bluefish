/* Bluefish HTML Editor
 *
 * textstyle - color styles used for syntax highlighting
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#define DEBUG

#include <glib.h>
#include <gtk/gtk.h>
#include <string.h> /* strlen */
#include <stdlib.h> /* atoi */

#include "bluefish.h"
#include "textstyle.h"

typedef struct {
	GtkTextTagTable *tagtable;
} Ttextstyle;

static Ttextstyle textstyle = {NULL};

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
	if (textstyle.tagtable) {
		/* cleanup the existing tags */
		gtk_text_tag_table_foreach(textstyle.tagtable,addtolist_lcb,&tmplist);
		for (tmplist=g_list_first(tmplist);tmplist!=NULL;tmplist=tmplist->next) {
			DEBUG_MSG("textstyle_rebuild, removing tag %p from tagtable %p\n",tmplist->data,textstyle.tagtable);
			gtk_text_tag_table_remove(textstyle.tagtable,tmplist->data);
		}
		g_list_free(tmplist);
	} else {
		textstyle.tagtable = gtk_text_tag_table_new();
		DEBUG_MSG("textstyle_rebuild, tagtable at %p\n",textstyle.tagtable);
	}

	for (tmplist=g_list_first(main_v->props.textstyles);tmplist!=NULL;tmplist=tmplist->next) {
		gchar **arr = tmplist->data;
		GtkTextTag *tag;
		tag = textstyle_compile(arr);
		DEBUG_MSG("textstyle_rebuild, adding tag %p to tagtable %p\n",tag,textstyle.tagtable);
		gtk_text_tag_table_add(textstyle.tagtable, tag);
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
