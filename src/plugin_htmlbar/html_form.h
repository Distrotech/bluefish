/* Bluefish HTML Editor
 * html_form.h - menu/toolbar callback prototypes
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

#ifndef __HTML_FORM_H_
#define __HTML_FORM_H_

#include "../bluefish.h"
#include "html_diag.h"

void formdialog_dialog(Tbfwin * bfwin, Ttagpopup * data);
void textareadialog_dialog(Tbfwin * bfwin, Ttagpopup * data);
void selectdialog_dialog(Tbfwin * bfwin, Ttagpopup * data);
void optiondialog_dialog(Tbfwin * bfwin, Ttagpopup * data);
void optgroupdialog_dialog(Tbfwin * bfwin, Ttagpopup * data);
void inputdialog_dialog(Tbfwin * bfwin, Ttagpopup * data, const gchar * type);
void inputdialog_rpopup(Tbfwin * bfwin, Ttagpopup * data);
void buttondialog_dialog(Tbfwin * bfwin, Ttagpopup * data);

#endif							/* __HTML_FORM_H_ */
