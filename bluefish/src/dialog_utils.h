/* Bluefish HTML Editor
 * dialog_utils.h
 *
 * Copyright (C) 2005 James Hayward and Olivier Sessink
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __DIALOG_UTILS_H_
#define __DIALOG_UTILS_H_

GtkWidget * dialog_label_with_alignment(const gchar *labeltext, GtkWidget *box);
GtkWidget * dialog_table_in_vbox(gint rows, gint cols, gint borderWidth, GtkWidget *box, gboolean expand, gboolean fill, guint padding);
GtkWidget * dialog_table_in_vbox_defaults(gint rows, gint cols, gint borderWidth, GtkWidget *box);

#endif /* __DIALOG_UTILS_H_ */
