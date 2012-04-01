/* Bluefish HTML Editor
 * html.h - menu/toolbar callback prototypes
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * Copyright (C) 1999-2001 Olivier Sessink
 * Copyright (C) 2011 James Hayward
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

#ifndef __HTML_H_
#define __HTML_H_

#include "html_diag.h"			/* Ttagpopup */

typedef enum {
	linkdialog_mode_default,
	linkdialog_mode_css
} Tlinkdialog_mode;

typedef enum {
	BF_A_ACUTE_LOWER = 1,
	BF_A_CIRC_LOWER,
	BF_A_ELIG_LOWER,
	BF_A_GRAVE_LOWER,
	BF_A_RING_LOWER,
	BF_A_TILDE_LOWER,
	BF_A_UML_LOWER,
	BF_C_CEDIL_LOWER,
	BF_E_ACUTE_LOWER,
	BF_E_CIRC_LOWER,
	BF_E_GRAVE_LOWER,
	BF_E_UML_LOWER,
	BF_I_ACUTE_LOWER,
	BF_I_CIRC_LOWER,
	BF_I_GRAVE_LOWER,
	BF_I_UML_LOWER,
	BF_N_TILDE_LOWER,
	BF_O_ACUTE_LOWER,
	BF_O_CIRC_LOWER,
	BF_O_ELIG_LOWER,
	BF_O_GRAVE_LOWER,
	BF_O_SLASH_LOWER,
	BF_O_TILDE_LOWER,
	BF_O_UML_LOWER,
	BF_U_ACUTE_LOWER,
	BF_U_CIRC_LOWER,
	BF_U_GRAVE_LOWER,
	BF_U_UML_LOWER,
	BF_Y_ACUTE_LOWER,
	BF_Y_UML_LOWER,
	BF_A_ACUTE_UPPER,
	BF_A_CIRC_UPPER,
	BF_A_ELIG_UPPER,
	BF_A_GRAVE_UPPER,
	BF_A_RING_UPPER,
	BF_A_TILDE_UPPER,
	BF_A_UML_UPPER,
	BF_C_CEDIL_UPPER,
	BF_E_ACUTE_UPPER,
	BF_E_CIRC_UPPER,
	BF_E_GRAVE_UPPER,
	BF_E_UML_UPPER,
	BF_I_ACUTE_UPPER,
	BF_I_CIRC_UPPER,
	BF_I_GRAVE_UPPER,
	BF_I_UML_UPPER,
	BF_N_TILDE_UPPER,
	BF_O_ACUTE_UPPER,
	BF_O_CIRC_UPPER,
	BF_O_ELIG_UPPER,
	BF_O_GRAVE_UPPER,
	BF_O_SLASH_UPPER,
	BF_O_TILDE_UPPER,
	BF_O_UML_UPPER,
	BF_U_ACUTE_UPPER,
	BF_U_CIRC_UPPER,
	BF_U_GRAVE_UPPER,
	BF_U_UML_UPPER,
	BF_Y_ACUTE_UPPER,
	BF_ACUTE,
	BF_CEDILLA,
	BF_UMLAUT,
	BF_CENT,
	BF_CURRENCY,
	BF_EURO,
	BF_POUND,
	BF_YEN,
	BF_LOGICAL_NOT,
	BF_DIVISION,
	BF_TIMES,
	BF_PLUS_MINUS,
	BF_LESS_THAN,
	BF_GREATER_THAN,
	BF_SUPERSCRIPT_1,
	BF_SUPERSCRIPT_2,
	BF_SUPERSCRIPT_3,
	BF_FRAC_QUARTER,
	BF_FRAC_HALF,
	BF_FRAC_THREE_QUARTER,
	BF_DEGREE,
	BF_ETH_LOWER,
	BF_ETH_UPPER,
	BF_THORN_LOWER,
	BF_THORN_UPPER,
	BF_SZLIG,
	BF_MIRCO,
	BF_NBSP,
	BF_SECTION,
	BF_COPYRIGHT,
	BF_LEFT_ANGLE,
	BF_RIGHT_ANGLE,
	BF_TRADEMARK,
	BF_INVERTED_EXCLAMATION,
	BF_INVERTED_QUESTION_MARK,
	BF_ORDINAL_F,
	BF_ORDINAL_M,
	BF_PARAGRAPH,
	BF_BROKEN_BAR,
	BF_SOFT_HYPHEN,
	BF_SPACING_MACRON,
	BF_MIDDOT
} TBFSpecialChars;

void htmlbar_insert_special_char(Tbfwin * bfwin, TBFSpecialChars spchar);

void general_html_menu_cb(Tbfwin * bfwin, guint callback_action, GtkWidget * widget);

void insert_time_dialog(Tbfwin * bfwin);
void quickanchor_dialog(Tbfwin * bfwin, Ttagpopup * data);
void block_tag_edit_cb(gint type, GtkWidget * widget, Tbfwin * bfwin);
void p_dialog(Tbfwin * bfwin, Ttagpopup * data);
void div_dialog(Tbfwin * bfwin, Ttagpopup * data);
void columns_dialog(Tbfwin * bfwin, Ttagpopup * data);
void span_dialog(Tbfwin * bfwin, Ttagpopup * data);
void h1_dialog(Tbfwin * bfwin, Ttagpopup * data);
void h2_dialog(Tbfwin * bfwin, Ttagpopup * data);
void h3_dialog(Tbfwin * bfwin, Ttagpopup * data);
void h4_dialog(Tbfwin * bfwin, Ttagpopup * data);
void h5_dialog(Tbfwin * bfwin, Ttagpopup * data);
void h6_dialog(Tbfwin * bfwin, Ttagpopup * data);
void quickrule_dialog(Tbfwin * bfwin, Ttagpopup * data);
void body_dialog(Tbfwin * bfwin, Ttagpopup * data);
void meta_dialog(Tbfwin * bfwin, Ttagpopup * data);
void font_dialog(Tbfwin * bfwin, Ttagpopup * data);
void basefont_dialog(Tbfwin * bfwin, Ttagpopup * data);
void email_dialog(Tbfwin * bfwin, Ttagpopup * data);
void quicklist_dialog(Tbfwin * bfwin, Ttagpopup * data);
void frameset_dialog(Tbfwin * bfwin, Ttagpopup * data);
void frame_dialog(Tbfwin * bfwin, Ttagpopup * data);
void audio_dialog(Tbfwin * bfwin, Ttagpopup * data);
void embed_dialog(Tbfwin * bfwin, Ttagpopup * data);
void script_dialog(Tbfwin * bfwin, Ttagpopup * data);
void linkdialog_dialog(Tbfwin * bfwin, Ttagpopup * data, Tlinkdialog_mode mode);
void audio_dialog(Tbfwin * bfwin, Ttagpopup * data);
void video_dialog(Tbfwin * bfwin, Ttagpopup * data);
void canvas_dialog(Tbfwin * bfwin, Ttagpopup * data);
void html5time_dialog(Tbfwin * bfwin, Ttagpopup * data);


#endif							/* __HTML_H_ */
