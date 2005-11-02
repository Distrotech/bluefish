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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __HTML_H_
#define __HTML_H_

#include "html_diag.h" /* Ttagpopup */

typedef enum {
        linkdialog_mode_default,
        linkdialog_mode_css
} Tlinkdialog_mode;

void insert_char_cb(Tbfwin* bfwin,guint callback_action, GtkWidget *widget);
void general_html_menu_cb(Tbfwin* bfwin,guint callback_action, GtkWidget *widget);

void insert_time_dialog(Tbfwin *bfwin);
void quickanchor_dialog(Tbfwin *bfwin, Ttagpopup *data);
void block_tag_edit_cb(gint type, GtkWidget *widget, Tbfwin *bfwin);
void p_dialog(Tbfwin *bfwin, Ttagpopup *data);
void div_dialog(Tbfwin *bfwin, Ttagpopup *data);
void span_dialog(Tbfwin *bfwin, Ttagpopup *data);
void h1_dialog(Tbfwin *bfwin, Ttagpopup *data);
void h2_dialog(Tbfwin *bfwin, Ttagpopup *data);
void h3_dialog(Tbfwin *bfwin, Ttagpopup *data);
void h4_dialog(Tbfwin *bfwin, Ttagpopup *data);
void h5_dialog(Tbfwin *bfwin, Ttagpopup *data);
void h6_dialog(Tbfwin *bfwin, Ttagpopup *data);
void quickrule_dialog(Tbfwin *bfwin, Ttagpopup *data);
void body_dialog(Tbfwin *bfwin, Ttagpopup *data);
void meta_dialog(Tbfwin *bfwin, Ttagpopup *data);
void font_dialog(Tbfwin *bfwin, Ttagpopup *data);
void basefont_dialog(Tbfwin *bfwin, Ttagpopup *data);
void email_dialog(Tbfwin *bfwin, Ttagpopup *data);
void quicklist_dialog(Tbfwin *bfwin, Ttagpopup *data);
void frameset_dialog(Tbfwin *bfwin, Ttagpopup *data);
void frame_dialog(Tbfwin *bfwin, Ttagpopup *data);
void embed_dialog(Tbfwin *bfwin, Ttagpopup *data);
void script_dialog(Tbfwin *bfwin, Ttagpopup *data);
void linkdialog_dialog(Tbfwin *bfwin, Ttagpopup *data, Tlinkdialog_mode mode);

#ifndef HAVE_ATLEAST_GTK_2_4
void quickstart_dialog(Tbfwin *bfwin, Ttagpopup *data);
#endif /* HAVE_ATLEAST_GTK_2_4 */

#endif							/* __HTML_H_ */
