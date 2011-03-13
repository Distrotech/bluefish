/* Bluefish HTML Editor
 * html_table.h - menu/toolbar callback prototypes
 *
 * Copyright (C) 2000-2011 Olivier Sessink
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

#ifndef __HTML_TABLE_H_
#define __HTML_TABLE_H_

#include "../bluefish.h"
#include "html_diag.h"

void tabledialog_dialog(Tbfwin * bfwin, Ttagpopup * data);
void tablerowdialog_dialog(Tbfwin * bfwin, Ttagpopup * data);
void tableheaddialog_dialog(Tbfwin * bfwin, Ttagpopup * data);
void tabledatadialog_dialog(Tbfwin * bfwin, Ttagpopup * data);

#endif							/* __HTML_TABLE_H_ */
