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


#define BF_RECENT_FILE_GROUP		"bluefish-recent-file"
#define BF_RECENT_PROJECT_GROUP		"bluefish-recent-project"

void sync_fullscreen_toggle(Tbfwin *bfwin, gboolean is_fullscreen);

void bfwin_main_ui_init(Tbfwin * bfwin, GtkWidget * vbox);
void bfwin_set_undo_redo_actions(Tbfwin * bfwin, gboolean undo, gboolean redo);
void bfwin_set_cutcopypaste_actions(Tbfwin * bfwin, gboolean enabled);
void bfwin_set_document_menu_items(Tdocument * doc);

void bfwin_action_set_sensitive(GtkUIManager * manager, const gchar * path, gboolean sensitive);

void bfwin_set_menu_toggle_item(GtkActionGroup * action_group, const gchar * action_name, gboolean is_active);
void bfwin_set_menu_toggle_item_from_path(GtkUIManager * manager, const gchar * path, gboolean is_active);

void bfwin_encoding_set_wo_activate(Tbfwin * bfwin, const gchar * encoding);
void bfwin_lang_mode_set_wo_activate(Tbfwin * bfwin, Tbflang * bflang);

void bfwin_commands_menu_create(Tbfwin * bfwin);
void bfwin_encodings_menu_create(Tbfwin * bfwin);
void bfwin_filters_menu_create(Tbfwin * bfwin);
void bfwin_outputbox_menu_create(Tbfwin * bfwin);
void lang_mode_menu_create(Tbfwin * bfwin);

/*void bfwin_recent_menu_add(Tbfwin * bfwin, GFile * file, GFileInfo * finfo, gboolean is_project);*/
void bfwin_recent_menu_add(Tbfwin *bfwin, gboolean project, const gchar *curi);
void bfwin_recent_menu_remove(Tbfwin *bfwin, gboolean project, const gchar *curi);
void bfwin_recent_menu_create(Tbfwin *bfwin, gboolean only_update_session);
void bfwin_templates_menu_create(Tbfwin * bfwin);

void sync_fullscreen_toggle(Tbfwin *bfwin, gboolean is_fullscreen);

#endif							/* BFWIN_UIMANAGER_H_ */
