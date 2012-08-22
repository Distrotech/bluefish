/* Bluefish HTML Editor
 * rcfile.h - functions to load the config
 *
 * Copyright (C) 2001-2008 Olivier Sessink
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __RCFILE_H_
#define __RCFILE_H_

#define BLUEFISH_DEFINED_DISABLED "0"
#define BLUEFISH_DEFINED_ENABLED "1"
#define USER_DEFINED_DISABLED "2"
#define USER_DEFINED_ENABLED "3"

GHashTable *make_config_list_item(GHashTable * config_list, void *pointer_to_var, unsigned char type_of_var, gchar * name_of_var, gint len);

GList *update_outputbox(GList * current, gboolean overwrite);
GList *update_filters(GList * current, gboolean overwrite);
GList *update_commands(GList * current, gboolean overwrite);

void rcfile_parse_main(void);
void rcfile_parse_highlighting(void);
void rcfile_check_directory(void);
void rcfile_save_accelerators(void);
void rcfile_load_accelerators(gboolean defaultmap);
gint rcfile_save_main(void);

gboolean rcfile_parse_project(Tproject *project, GFile *file);
gboolean rcfile_save_project(Tproject *project, GFile *file);
gboolean rcfile_save_encodings(void);
gboolean rcfile_save_global_session(void);
gboolean rcfile_parse_global_session(void);
void load_templates(void);
#endif /* __RCFILE_H_ */
