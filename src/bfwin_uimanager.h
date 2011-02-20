/*
 * Bluefish HTML Editor
 * bfwin_uimanager.h
 *
 * Copyright (C) 2011 James Hayward and Olivier Sessink
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

#ifndef BFWIN_UIMANAGER_H_
#define BFWIN_UIMANAGER_H_

#include "bluefish.h"


void main_menu_create(Tbfwin *bfwin, GtkWidget *vbox);
void setup_menu_toggle_item(GtkActionGroup *action_group, const gchar *action_name, gboolean is_active);


#endif /* BFWIN_UIMANAGER_H_ */
