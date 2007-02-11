/* Bluefish HTML Editor
 * infb_load.h - plugin for information browser
 *
 * Copyright (C) 2007 Oskar Świda
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

#ifndef INFB_LOAD_H
#define INFB_LOAD_H
#include "../bluefish.h"
#include <gtk/gtk.h>

void infb_load(void);
void infb_load_fragments();
void infb_reload_home(Tbfwin *win);
GList *infb_user_files();

#endif /* INFB_LOAD_H */
