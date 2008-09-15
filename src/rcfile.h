/* Bluefish HTML Editor
 * rcfile.h - functions to load the config
 *
 * Copyright (C) 2001-2008 Olivier Sessink
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

#ifndef __RCFILE_H_
#define __RCFILE_H_

GList *make_config_list_item(GList * config_list, void *pointer_to_var, unsigned char type_of_var, gchar * name_of_var, gint len);
void rcfile_parse_main(void);
void rcfile_parse_highlighting(void);
void rcfile_parse_custom_menu(void);
void rcfile_check_directory(void);
void rcfile_save_configfile_menu_cb(gpointer callback_data,guint action,GtkWidget *widget);

gint rcfile_save_main(void);
gint rcfile_save_custom_menu(void);
gboolean rcfile_parse_project(Tproject *project, gchar *filename);
gboolean rcfile_save_project(Tproject *project, gchar *filename);
gboolean rcfile_save_encodings(void);
gboolean rcfile_save_global_session(void);
gboolean rcfile_parse_global_session(void);
#endif /* __RCFILE_H_ */
