/* Bluefish HTML Editor
 * menu.h - uhh, duh.
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * Copyright (C) 1999-2002 Olivier Sessink
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

#ifndef __MENU_H_
#define __MENU_H_

void menu_create_main(GtkWidget *vbox);
void add_to_recent_list(gchar *filename, gint closed_file);
void recent_menu_init();
#endif							/* __MENU_H_ */
