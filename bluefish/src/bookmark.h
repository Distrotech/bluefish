/* Bluefish HTML Editor - bookmarks
 *
 * Copyright (C) 2003 Oskar Swida
 * modifications (C) 2004 Olivier Sessink
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
#ifndef __BOOKMARK_H__
#define __BOOKMARK_H__

GtkWidget *bmark_gui(Tbfwin *bfwin); /* used in gui.c */
void bmark_init(void); /* only used once */
void bmark_cleanup(Tbfwin *bfwin);

GHashTable *bmark_get_lines(Tdocument *doc,gboolean temp);
void bmark_clean_for_doc(Tdocument *doc); /* set bookmark's doc to NULL when closing file */ 
void bmark_set_for_doc(Tdocument *doc); /* set bookmark's doc to proper doc when opening file */ 

void bmark_add_temp(Tbfwin *bfwin);
void bmark_del_all_temp(Tbfwin *bfwin,gboolean ask);
void bmark_add_perm(Tbfwin *bfwin);
void bmark_del_all_perm(Tbfwin *bfwin,gboolean ask);
void bmark_add(Tbfwin *bfwin);
void bmark_del_all(Tbfwin *bfwin);

void bmark_save_all(Tbfwin *bfwin);
void bmark_check_length(Tbfwin *bfwin,Tdocument *doc);
void bmark_reload(Tbfwin *bfwin);

void bmark_name_entry_changed(GtkEntry *entry, GtkDialog *dialog);
void bmark_add_rename_dialog(Tbfwin *bfwin, gchar *dialogtitle, gint dialogtype);

#endif /* __BOOKMARK_H__ */
