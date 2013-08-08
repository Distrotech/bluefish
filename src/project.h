/* Bluefish HTML Editor
 * project.h - project prototypes
 *
 * Copyright (C) 2003-2011 Olivier Sessink
 * Copyright (C) 2011 James Hayward
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

#ifndef __PROJECT_H_
#define __PROJECT_H_

void project_edit(Tbfwin * bfwin);
void project_new(Tbfwin * bfwin);
void project_open(Tbfwin * bfwin);
gint project_open_from_file(Tbfwin * bfwin, GFile * fromuri);

gboolean project_save(Tbfwin * bfwin, gboolean save_as);
void project_save_and_close(Tbfwin * bfwin);
void project_save_and_mark_closed(Tbfwin * bfwin);
gboolean project_final_close(Tbfwin * bfwin, gboolean close_win);

void set_project_menu_actions(Tbfwin * bfwin, gboolean win_has_project);

#endif							/* __PROJECT_H_  */
