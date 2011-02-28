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
#include "bfwin.h"


void bfwin_main_menu_init(Tbfwin * bfwin, GtkWidget * vbox);
void bfwin_main_toolbar_init(Tbfwin * bfwin);

void bfwin_set_undo_redo_actions(Tbfwin * bfwin, gboolean undo, gboolean redo);
void bfwin_set_document_menu_items(Tdocument * doc);

void bfwin_action_set_sensitive(GtkUIManager * manager, const gchar * path, gboolean sensitive);

void setup_menu_toggle_item(GtkActionGroup * action_group, const gchar * action_name, gboolean is_active);
void setup_menu_toggle_item_from_path(GtkUIManager * manager, const gchar * path, gboolean is_active);


#endif							/* BFWIN_UIMANAGER_H_ */
