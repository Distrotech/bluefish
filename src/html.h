/* Bluefish HTML Editor
 * html.h - menu/toolbar callback prototypes
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
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

#ifndef __HTML_H_
#define __HTML_H_
void general_html_cb(GtkWidget * widget, gpointer data);
void insert_time_cb(GtkWidget * widget, gpointer data);
void quickanchor_cb(GtkWidget * widget, gpointer data);
void block_tag_edit_cb(gint type, GtkWidget *widget, gpointer data);
void div_dialog(GtkWidget *widget, gpointer data);
void span_dialog(GtkWidget * widget, gpointer data);
void quickrule_cb(GtkWidget * widget, gpointer data);
void quickstart_cb(GtkWidget * widget, gpointer data);
void body_cb(GtkWidget * widget, gpointer data);
void meta_cb(GtkWidget * widget, gpointer data);
void fontdialog_cb(GtkWidget *widget, gpointer data);
void basefont_cb(GtkWidget *widget, gpointer data);
void email_cb(GtkWidget * widget, gpointer data);
void quicklist_cb(GtkWidget * widget, gpointer data);
void framesetdialog_cb(GtkWidget * widget, gpointer data);
void framedialog_cb(GtkWidget * widget, gpointer data);
void embed_cb(GtkWidget * widget, gpointer data);
void script_cb(GtkWidget * widget, gpointer data);
void link_cb(GtkWidget * widget, gpointer data);
#endif							/* __HTML_H_ */
