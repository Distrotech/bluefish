/* Bluefish HTML Editor
 * outputbox.h - the output box
 *
 * Copyright (C) 2005-2009 Olivier Sessink
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

#ifndef __OUTPUTBOX_H_
#define __OUTPUTBOX_H_

/*void init_output_box(Tbfwin *bfwin,GtkWidget *vbox);*/
void outputbox_add_line(Tbfwin *bfwin, const gchar *uri, gint line, const gchar *message);
void fill_output_box(gpointer data, gchar *string);
void outputbox(Tbfwin *bfwin,gchar *pattern, gint file_subpat, gint line_subpat, gint output_subpat, gchar *command);
void outputbox_cleanup(Tbfwin *bfwin);

#endif /* __OUTPUTBOX_H */
