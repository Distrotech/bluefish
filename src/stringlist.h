/* Bluefish HTML Editor
 * stringlist.h - functions that deal with stringlists
 *
 * Copyright (C) 1999 Olivier Sessink and Hylke van der Schaaf
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

#ifndef __STRINGLIST_H_
#define __STRINGLIST_H_

#include <glib.h>

void estrl_dialog(GList **which_list, gchar *title, gint what_list
				, gint column_num, gchar **column_titles, void (*post_dialog_func)());
GList *duplicate_stringlist(GList *list, gint dup_data);
gint free_stringlist(GList * which_list);
GList *get_stringlist(gchar * filename, GList * which_list);
gboolean put_stringlist_limited(gchar * filename, GList * which_list, gint maxentries);
gboolean put_stringlist(gchar * filename, GList * which_list);
gint count_array(gchar **array);
gint free_arraylist(GList * which_list);
gchar **duplicate_stringarray(gchar **array);
GList *duplicate_arraylist(GList *arraylist);

/* removes a string from a stringlist if it is the same */
GList *remove_from_stringlist(GList *which_list, const gchar * string);
GList *add_to_history_stringlist(GList *which_list, const gchar *string);
/* adds a string to a stringlist if it is not yet in there */
GList *add_to_stringlist(GList * which_list, const gchar * string);
gchar *stringlist_to_string(GList *stringlist, gchar *delimiter);
#endif							/* __STRINGLIST_H_ */
