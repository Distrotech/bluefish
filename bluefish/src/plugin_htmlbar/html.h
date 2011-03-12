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
	A_ACUTE_LOWER = 1,
	A_CIRC_LOWER,
	A_ELIG_LOWER,
	A_GRAVE_LOWER,
	A_RING_LOWER,
	A_TILDE_LOWER,
	A_UML_LOWER,
	C_CEDIL_LOWER,
	E_ACUTE_LOWER,
	E_CIRC_LOWER,
	E_GRAVE_LOWER,
	E_UML_LOWER,
	I_ACUTE_LOWER,
	I_CIRC_LOWER,
	I_GRAVE_LOWER,
	I_UML_LOWER,
	N_TILDE_LOWER,
	O_ACUTE_LOWER,
	O_CIRC_LOWER,
	O_ELIG_LOWER,
	O_GRAVE_LOWER,
	O_SLASH_LOWER,
	O_TILDE_LOWER,
	O_UML_LOWER,
	U_ACUTE_LOWER,
	U_CIRC_LOWER,
	U_GRAVE_LOWER,
	U_UML_LOWER,
	Y_ACUTE_LOWER,
	Y_UML_LOWER,
	A_ACUTE_UPPER,
	A_CIRC_UPPER,
	A_ELIG_UPPER,
	A_GRAVE_UPPER,
	A_RING_UPPER,
	A_TILDE_UPPER,
	A_UML_UPPER,
	C_CEDIL_UPPER,
	E_ACUTE_UPPER,
	E_CIRC_UPPER,
	E_GRAVE_UPPER,
	E_UML_UPPER,
	I_ACUTE_UPPER,
	I_CIRC_UPPER,
	I_GRAVE_UPPER,
	I_UML_UPPER,
	N_TILDE_UPPER,
	O_ACUTE_UPPER,
	O_CIRC_UPPER,
	O_ELIG_UPPER,
	O_GRAVE_UPPER,
	O_SLASH_UPPER,
	O_TILDE_UPPER,
	O_UML_UPPER,
	U_ACUTE_UPPER,
	U_CIRC_UPPER,
	U_GRAVE_UPPER,
	U_UML_UPPER,
	Y_ACUTE_UPPER,
	ACUTE,
	CEDILLA,
	UMLAUT,
	CENT,
	CURRENCY,
	EURO,
	POUND,
	YEN,
	LOGICAL_NOT,
	DIVISION,
	TIMES,
	PLUS_MINUS,
	LESS_THAN,
	GREATER_THAN,
	SUPERSCRIPT_1,
	SUPERSCRIPT_2,
	SUPERSCRIPT_3,
	FRAC_QUARTER,
	FRAC_HALF,
	FRAC_THREE_QUARTER,
	DEGREE,
	ETH_LOWER,
	ETH_UPPER,
	THORN_LOWER,
	THORN_UPPER,
	SZLIG,
	MIRCO,
	NBSP,
	SECTION,
	COPYRIGHT,
	LEFT_ANGLE,
	RIGHT_ANGLE,
	TRADEMARK,
	INVERTED_EXCLAMATION,
	INVERTED_QUESTION_MARK,
	ORDINAL_F,
	ORDINAL_M,
	PARAGRAPH,
	BROKEN_BAR,
	SOFT_HYPHEN,
	SPACING_MACRON,
	MIDDOT
} TBFSpecialChars;

void htmlbar_insert_special_char(Tbfwin * bfwin, TBFSpecialChars spchar);

void general_html_menu_cb(Tbfwin * bfwin, guint callback_action, GtkWidget * widget);

void insert_time_dialog(Tbfwin * bfwin);
void quickanchor_dialog(Tbfwin * bfwin, Ttagpopup * data);
void block_tag_edit_cb(gint type, GtkWidget * widget, Tbfwin * bfwin);
void p_dialog(Tbfwin * bfwin, Ttagpopup * data);
void div_dialog(Tbfwin * bfwin, Ttagpopup * data);
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
void embed_dialog(Tbfwin * bfwin, Ttagpopup * data);
void script_dialog(Tbfwin * bfwin, Ttagpopup * data);
void linkdialog_dialog(Tbfwin * bfwin, Ttagpopup * data, Tlinkdialog_mode mode);


#endif							/* __HTML_H_ */
