/* Bluefish HTML Editor
 * dialog_utils.h - dialog utility functions
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __DIALOG_UTILS_H_
#define __DIALOG_UTILS_H_

#include "config.h"

#ifdef HAVE_ATLEAST_GTK_2_4
void dialog_mnemonic_label_in_table(const gchar *labeltext,
												GtkWidget *m_widget,
												GtkWidget *table,
												guint left_attach, guint right_attach, guint top_attach, guint bottom_attach);
												
GtkWidget * dialog_vbox_labeled(const gchar *labeltext, GtkWidget *box);
GtkWidget * dialog_vbox_labeled_checkbutton(const gchar *labeltext, GtkWidget *checkbutton, GtkWidget *box);

GtkWidget * dialog_table_in_vbox(gint rows, gint cols, 
											gint borderWidth, 
											GtkWidget *box, 
											gboolean expand, gboolean fill, guint padding);
											
GtkWidget * dialog_table_in_vbox_defaults(gint rows, gint cols, gint borderWidth, GtkWidget *box);

void message_dialog_new(GtkWidget *parent, 
								GtkMessageType type, 
								GtkButtonsType button, 
								const gchar *primaryText, const gchar *secondaryText);
								
gint message_dialog_new_multi(GtkWidget *parent, 
										GtkMessageType type, const gchar *buttons[],
										const gchar *primaryText, const gchar *secondaryText);

#endif /* HAVE_ATLEAST_GTK_2_4 */

#endif /* __DIALOG_UTILS_H_ */
