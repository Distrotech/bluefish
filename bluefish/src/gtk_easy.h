/* Bluefish HTML Editor
 * gtk_easy.h
 *
 * Copyright (C) 1999-2011 Olivier Sessink
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

#ifndef __GTK_EASY_H_
#define __GTK_EASY_H_

#define FILE_CHOOSER_USE_VFS(dialog) (g_object_set_data(G_OBJECT(dialog),"GnomeFileSelectorEnableVFS",GINT_TO_POINTER(1)))

/* GtkWindowPosition can be 
GTK_WIN_POS_NONE
GTK_WIN_POS_CENTER
GTK_WIN_POS_MOUSE */

typedef enum { none, file, font } Textra_but;

void flush_queue(void);
gint widget_get_string_size(GtkWidget * widget, gchar * string);
void widget_set_visible(GtkWidget * widget, gboolean visible);

void string_apply(gchar ** config_var, GtkWidget * entry);
void integer_apply(gint * config_var, GtkWidget * widget, gboolean is_checkbox);
void button_apply(gchar ** config_var, GtkWidget * entry);

GtkWidget *combobox_with_popdown_sized(const gchar * setstring, const GList * which_list, gint editable,
									   gint width);
GtkWidget *boxed_combobox_with_popdown(const gchar * setstring, const GList * which_list, gboolean editable,
									   GtkWidget * box);
void combobox_empty(GtkWidget *combobox);
void combobox_fill(GtkWidget *combobox, const gchar * setstring, const GList * which_list);
GtkWidget *combobox_with_popdown(const gchar * setstring, const GList * which_list, gboolean editable);

GtkWidget *checkbut_with_value(gchar * labeltext, gint which_config_int);
GtkWidget *boxed_checkbut_with_value(gchar * labeltext, gint which_config_int, GtkWidget * box);

GtkWidget *radiobut_with_value(gchar * labeltext, gint enabled, GtkRadioButton * prevbut);
GtkWidget *boxed_radiobut_with_value(gchar * labeltext, gint enabled, GtkRadioButton * prevbut,
									 GtkWidget * box);
GtkWidget *spinbut_with_value(gchar * value, gfloat lower, gfloat upper, gfloat step_increment,
							  gfloat page_increment);

GtkWidget *window_with_title(const gchar * title, GtkWindowPosition position, gint borderwidth);

void window_delete_on_escape(GtkWindow * window);
#define window_full(title,position,borderwidth,close_func,close_data,delete_on_escape) window_full2(title,position,borderwidth,close_func,close_data,delete_on_escape,NULL)

GtkWidget *window_full2(const gchar * title, GtkWindowPosition position, gint borderwidth,
						GCallback close_func, gpointer close_data, gboolean delete_on_escape,
						GtkWidget * transientforparent);

GtkWidget *textview_buffer_in_scrolwin(GtkWidget ** textview, gint width, gint height, const gchar * contents,
									   GtkWrapMode wrapmode);
gchar *textbuffer_get_all_chars(GtkTextBuffer * buffer);
void window_destroy(GtkWidget * windowname);
void window_close_by_widget_cb(GtkWidget * widget, gpointer data);
void window_close_by_data_cb(GtkWidget * widget, gpointer data);
GtkWidget *apply_font_style(GtkWidget * this_widget, gchar * fontstring);

#define bf_stock_ok_button(func, data) dialog_button_new_with_image(NULL,GTK_STOCK_OK,func, data, FALSE, FALSE)
#define bf_stock_cancel_button(func, data) dialog_button_new_with_image(NULL,GTK_STOCK_CANCEL,func, data, FALSE, FALSE)

GtkWidget *bf_generic_frame_new(const gchar * label, GtkShadowType shadowtype, gint borderwidth);

GtkWidget *bf_label_with_markup(const gchar * labeltext);
void bf_label_tad_with_markup(const gchar * labeltext, gfloat xalign, gfloat yalign,
							  GtkWidget * table, guint left_attach, guint right_attach, guint top_attach,
							  guint bottom_attach);
gchar *run_file_select_dialog(GtkWindow *parent, const gchar *setfile, const gchar *relativeto, GtkFileChooserAction chooseraction);
GtkWidget *file_but_new2(GtkWidget * which_entry, gint full_pathname, Tbfwin * bfwin,
						 GtkFileChooserAction chooseraction);
#define file_but_new(entry,path,bfwin) file_but_new2(entry,path,bfwin,GTK_FILE_CHOOSER_ACTION_OPEN)
GtkWidget *file_chooser_dialog(Tbfwin * bfwin, const gchar * title, GtkFileChooserAction action,
							   const gchar * set, gboolean localonly, gboolean multiple, const gchar * filter,
							   gboolean show_encoding);

gchar *ask_accelerator_dialog(const gchar * title);
GtkWidget *accelerator_button(const gchar * accel);
GtkWidget *boxed_accelerator_button(const gchar * labeltext, const gchar * accel, GtkWidget * box);

gchar *gdk_color_to_hexstring(GdkColor * color, gboolean websafe);
GtkWidget *color_but_new2(GtkWidget * entry);

#endif
