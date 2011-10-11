/* Bluefish HTML Editor
 * dialog_utils.h - dialog utility functions
 *
 * Copyright (C) 2005-2011 James Hayward and Olivier Sessink
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

#ifndef __DIALOG_UTILS_H_
#define __DIALOG_UTILS_H_

GtkWidget *bluefish_small_close_button_new(void);

GtkWidget *dialog_stock_button_new_in_table(const gchar * stockID, GtkWidget * table, guint left_attach,
											guint right_attach, guint top_attach, guint bottom_attach);

GtkWidget *dialog_button_new_in_table(const gchar * labeltext, GtkWidget * table, guint left_attach,
									  guint right_attach, guint top_attach, guint bottom_attach);

GtkWidget *dialog_button_new_with_image(const gchar * labeltext,const gchar * stockid,
							GCallback func, gpointer func_data, gboolean force_image, gboolean mnemonic);

GtkWidget *
dialog_button_new_with_image_in_table(const gchar * labeltext, const gchar * stockID,
										GCallback func, gpointer func_data, gboolean force_image, gboolean mnemonic,
										GtkWidget * table, guint left_attach,
										guint right_attach, guint top_attach, guint bottom_attach);

GtkWidget *dialog_check_button_new(const gchar * labeltext, gint active);

GtkWidget *dialog_check_button_in_table(const gchar * labeltext, gint active, GtkWidget * table,
										guint left_attach, guint right_attach, guint top_attach,
										guint bottom_attach);

GtkWidget *dialog_color_button_in_table(const gchar * color, const gchar * title, GtkWidget * table,
										guint left_attach, guint right_attach, guint top_attach,
										guint bottom_attach);

GtkWidget *dialog_combo_box_text_from_list(const GList * options, const gchar * value);

GtkWidget *dialog_combo_box_text_labeled_from_list(const GList * options, const gchar * value,
												   const gchar * labeltext, GtkWidget * box, guint padding);

GtkWidget *dialog_combo_box_text_from_list_in_table(const GList * options, const gchar * value,
													GtkWidget * table, guint left_attach, guint right_attach,
													guint top_attach, guint bottom_attach);

GtkWidget *dialog_combo_box_text_new(const gchar ** options, gint index);

GtkWidget *dialog_combo_box_text_in_table(const gchar ** options, gint index, GtkWidget * table,
										  guint left_attach, guint right_attach, guint top_attach,
										  guint bottom_attach);

GtkWidget *dialog_combo_box_text_labeled(const gchar * labeltext, const gchar ** options, gint index,
										 GtkWidget * box, guint padding);

GtkWidget *dialog_entry_labeled(const gchar * text, const gchar * labeltext, GtkWidget * box, guint padding);

GtkWidget *dialog_entry_in_table(const gchar * text, GtkWidget * table, guint left_attach, guint right_attach,
								 guint top_attach, guint bottom_attach);
GtkWidget *
dialog_mnemonic_label_new(const gchar * labeltext, GtkWidget * m_widget);
GtkWidget *dialog_mnemonic_label_in_table(const gchar * labeltext, GtkWidget * m_widget, GtkWidget * table,
									guint left_attach, guint right_attach, guint top_attach,
									guint bottom_attach);

GtkWidget *dialog_radio_button_in_table(GSList * group, const gchar * label, GtkWidget * table,
										guint left_attach, guint right_attach, guint top_attach,
										guint bottom_attach);

GtkWidget *dialog_radio_button_from_widget_in_table(GtkRadioButton * radio_group_member, const gchar * label,
													GtkWidget * table, guint left_attach, guint right_attach,
													guint top_attach, guint bottom_attach);

GtkWidget *dialog_spin_button_new(gfloat lower, gfloat upper, const gint value);

GtkWidget *dialog_spin_button_in_table(gfloat lower, gfloat upper, const gint value, GtkWidget * table,
									   guint left_attach, guint right_attach, guint top_attach,
									   guint bottom_attach);

GtkWidget *dialog_spin_button_labeled(gfloat lower, gfloat upper, const gint value, const gchar * labeltext,
									  GtkWidget * box, guint padding);

GtkWidget *dialog_vbox_new(GtkWidget * box);

GtkWidget *dialog_label_new(const gchar * labeltext, gfloat xalign, gfloat yalign, GtkWidget * box,
							guint padding);

GtkWidget *dialog_vbox_labeled(const gchar * labeltext, GtkWidget * box);

GtkWidget *dialog_vbox_labeled_checkbutton(const gchar * labeltext, GtkWidget * checkbutton, GtkWidget * box);

GtkWidget *dialog_table_in_vbox(gint rows, gint cols, gint borderWidth, GtkWidget * box, gboolean expand,
								gboolean fill, guint padding);

GtkWidget *dialog_table_in_vbox_defaults(gint rows, gint cols, gint borderWidth, GtkWidget * box);

void message_dialog_new(GtkWidget * parent, GtkMessageType type, GtkButtonsType button,
						const gchar * primaryText, const gchar * secondaryText);

gint message_dialog_new_multi(GtkWidget * parent, GtkMessageType type, const gchar * buttons[],
							  const gchar * primaryText, const gchar * secondaryText);

#endif							/* __DIALOG_UTILS_H_ */
