/* Bluefish HTML Editor
 * html_form.h - menu/toolbar callback prototypes
 *
 * Copyright (C) 2000 Olivier Sessink
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

#ifndef __HTML_FORM_H_
#define __HTML_FORM_H_

void formdialog_cb(GtkWidget * widget, gpointer data);
void textareadialog_cb(GtkWidget * widget, gpointer data);
void textdialog_cb(GtkWidget * widget, gpointer data);
void buttondialog_cb(GtkWidget * widget, gpointer data);
void hiddendialog_cb(GtkWidget * widget, gpointer data);
void radiodialog_cb(GtkWidget * widget, gpointer data);
void checkdialog_cb(GtkWidget * widget, gpointer data);
void selectdialog_cb(GtkWidget * widget, gpointer data);
void optiondialog_cb(GtkWidget * widget, gpointer data);
void optgroupdialog_cb(GtkWidget * widget, gpointer data);
#endif /* __HTML_FORM_H_ */
