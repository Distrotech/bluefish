/* Bluefish HTML Editor
 * highlight.c - the syntax highlighting
 *
 * Copyright (C) 2002 Olivier Sessink
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
#include <gtk/gtk.h>


#include "bluefish.h"

/*********************************/
/* initializing the highlighting */
/*********************************/

Thighlightset *hl_get_highlightset_by_type(gchar *type) {
	GList *tmplist;
	tmplist = g_list_first(main_v->hlsetlist);
	while (tmplist) {
#ifdef DEBUG
		if (!tmplist || !tmplist->data || !((Thighlightset *)tmplist->data)->type) {
			g_print("hl_get_highlightset_by_type is perhaps not yet finished??\n");
			exit(1);
		}
#endif /* DEBUG */
		if (strcmp(((Thighlightset *)tmplist->data)->type, type)==0) {
			return (Thighlightset *)tmplist->data;
		}
	}
	return NULL;
}


void hl_init() {
	GList *tmplist;
	/* init main_v->hlsetlist, the first set is the defaultset */
	
	/* start by initializing the types, they should come from the configfile */
	tmplist = g_list_first(some_config_file_var_with_types);
	while (tmplist) {
		gchar *strarr[];
		Thighlightset *hlset = g_new0(Thighlightset, 1);
		strarr = tmplist->data;
#ifdef DEBUG
		if (!strarr || !strarr[0] || !strarr[1]) {
			g_print("hl_init is perhaps not yet finished??\n");
			exit(1);
		}
#endif /* DEBUG */
		hlset->type = g_strdup(strarr[0]);
		hlset->extensions = g_strdup(strarr[1]);
		
		main_v->hlsetlist = g_list_append(main_v->hlsetlist, hlset);
		tmplist = g_list_next(tmplist);
	}
	
	/* now start adding all the patterns */
	
	tmplist = g_list_first(some_config_file_var_with_patterns);
	while (tmplist) {
		Thighlightset *hlset;
		gchar *strarr[];
		strarr = tmplist->data;

		hlset = hl_get_highlightset_by_type(strarr[0]);
		hl_compile_pattern(hlset, strarr[1], strarr[2], strarr[3], .....);
	}

}

Thighlightset *hl_get_highlightset_by_filename(gchar *filename) {
	GList *tmplist;
	/* find set for this filetype */
	if (filename) {
	
	
	}
	/* if none found return first set (is default set) */
	tmplist = g_list_first(main_v->hlsetlist);
	return (Thighlightset *)tmplist->data;
}





/*****************************/
/* applying the highlighting */
/*****************************/
