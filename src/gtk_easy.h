/* Bluefish HTML Editor
 *
 * Copyright (C) 1999-2002 Olivier Sessink
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

#ifndef __GTK_EASY_H_
#define __GTK_EASY_H_

/* GtkWindowPosition can be 
GTK_WIN_POS_NONE
GTK_WIN_POS_CENTER
GTK_WIN_POS_MOUSE */

typedef enum { none, file, font } Textra_but;

void error_dialog(gchar * window_title, gchar * error_string);
void setup_toggle_item(GtkItemFactory * ifactory, gchar * path, gint state);
void string_apply(gchar ** config_var, GtkWidget * entry);
GtkWidget *combo_with_popdown(gchar * setstring, GList * which_list, gint editable);
GtkWidget *boxed_combo_with_popdown(gchar * setstring, GList * which_list, gint editable, GtkWidget *box);
GtkWidget *combo_with_popdown_sized(gchar * setstring, GList * which_list, gint editable, gint width);
GtkWidget *entry_with_text(gchar * setstring, gint max_lenght);
GtkWidget *boxed_entry_with_text(gchar * setstring, gint max_lenght, GtkWidget *box);
GtkWidget *boxed_full_entry(gchar * labeltext, gchar * setstring,gint max_lenght, GtkWidget * box);
GtkWidget *checkbut_with_value(gchar *labeltext, gint which_config_int);
GtkWidget *boxed_checkbut_with_value(gchar *labeltext, gint which_config_int, GtkWidget * box);
GtkWidget *radiobut_with_value(gchar *labeltext, gint enabled, GtkRadioButton *prevbut);
GtkWidget *boxed_radiobut_with_value(gchar *labeltext, gint enabled, GtkRadioButton *prevbut, GtkWidget *box);
GtkWidget *spinbut_with_value(gchar *value, gfloat lower, gfloat upper, gfloat step_increment, gfloat page_increment);
GtkWidget *window_with_title(gchar * title, GtkWindowPosition position, gint borderwidth);
GtkWidget *window_full(gchar * title, GtkWindowPosition position, gint borderwidth, GCallback close_func,
					   gpointer close_data);
void window_destroy(GtkWidget * windowname);
void window_close_by_widget_cb(GtkWidget * widget, gpointer data);
void window_close_by_data_cb(GtkWidget * widget, gpointer data);
/* 
GtkWidget *apply_font_style(GtkWidget * this_widget, gchar * fontstring);
*/
GtkWidget *bf_stock_button(const gchar * Text, GCallback func, gpointer func_data);
GtkWidget *bf_stock_ok_button(GCallback func, gpointer func_data);
GtkWidget *bf_stock_cancel_button(GCallback func, gpointer func_data);
GtkWidget *file_but_new(GtkWidget * which_entry, GtkWidget * win, gint full_pathname);
gint multi_button_dialog(gchar *title, gint defval, gchar *label, gchar **buttons);

gchar *return_file_w_title(gchar * setfile, gchar *title);
gchar *return_file(gchar * setfile);
GList *return_files_w_title(gchar * setfile, gchar *title);
GList *return_files(gchar * setfile);
#endif
