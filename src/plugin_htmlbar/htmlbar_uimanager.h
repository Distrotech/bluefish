/*
 * Bluefish HTML Editor
 * htmlbar_uimanager.h
 *
 * Copyright (C) 2005-2011 Olivier Sessink
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

#ifndef HTMLBAR_UIMANAGER_H_
#define HTMLBAR_UIMANAGER_H_

#include "htmlbar.h"

void htmlbar_load_ui(Thtmlbarwin * hbw);
void htmlbar_menu_create(Thtmlbarwin *hbw);
GtkWidget *htmlbar_toolbar_create(Thtmlbarwin *hbw, Thtmlbarsession *hbs);

void htmlbar_register_stock_icons(void);

void htmlbar_toolbar_show(Thtmlbarwin *hbw, Thtmlbarsession *hbs, gboolean show);


#endif /* HTMLBAR_UIMANAGER_H_ */
