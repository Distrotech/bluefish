/* Bluefish HTML Editor
 * project.h - project prototypes
 *
 * Copyright (C) 2003 Olivier Sessink
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

#ifndef __PROJECT_H_
#define __PROJECT_H_ 

/* #define DEBUG */
gboolean project_save_and_close(Tbfwin *bfwin);
void project_open_from_file(Tbfwin *bfwin, gchar *fromfilename);
void set_project_menu_widgets(Tbfwin *bfwin, gboolean win_has_project);
void project_menu_cb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget);

#endif /* __PROJECT_H_  */
