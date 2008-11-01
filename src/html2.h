/* Bluefish HTML Editor
 * html2.h - menu/toolbar callbacks, inserting functions, and other cool stuff 
 * otherwise html.c is getting so long ;-)
 *
 * Copyright (C) 1999 Olivier Sessink
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

#ifndef __HTML2_H_
#define __HTML2_H_
void insert_class(GtkWidget * widget, gpointer data);

/* void insert_in_stylesheet(GtkWidget * widget, gpointer data); */

GtkWidget *style_but_new(GtkWidget * which_entry, GtkWidget * win);
GtkWidget *style_but_new_for_wizard(GtkWidget * textview);
void new_css_dialog(GtkWidget *wid,Tbfwin *bfwin);
void edit_color_dialog(Tdocument *doc, gchar *color, gint startpos, gint endpos);
void sel_colour_cb(GtkWidget *widget, Tbfwin *bfwin);
GtkWidget *color_but_new(GtkWidget * which_entry, GtkWidget * win);

#endif /* __HTML2_H_ */
