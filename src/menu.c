/* Copyright (C) 1998-2002 Olivier Sessink, Chris Mazuc and Roland Steinbach
 * 
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
#include <gtk/gtk.h>
#include <stdlib.h> /* atoi */
#include <string.h> /* strchr() */

#include "bluefish.h"
#include "document.h"			/* file_open etc. */
#include "highlight.h" /* doc_highlight_full */
#include "menu.h" /* my own .h file */
#include "undo_redo.h" /* undo_cb() redo_cb() etc. */
#include "snr2.h" /* search_cb, replace_cb */
#include "gui.h" /* go_to_line_win_cb */
#include "stringlist.h" 	/* free_stringlist() */
#include "bf_lib.h"  /* append_string_to_file() */
#include "gtk_easy.h" /* window_full, bf_stock_ok_button */
#include "preferences.h" /* open_preferences_menu_cb */
#include "html.h"
#include "html2.h"
#include "html_table.h"
#include "html_form.h"
#include "wizards.h"
#include "image.h"
#include "rcfile.h" /* rcfile_save_configfile_menu_cb */

#include "outputbox.h" /* temporary */

typedef struct {
	GList *outputbox_menu;
	GList *external_menu;
	GList *recent_files;
	GList *encodings;
	GtkWidget *cmenu;
	GList *cmenu_entries;
} Tmenus;
static Tmenus menus = {NULL,NULL,NULL,NULL,NULL,NULL};

static GtkItemFactoryEntry menu_items[] = {
	{N_("/_File"), NULL, NULL, 0, "<Branch>"},
	{N_("/File/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/File/_New"), "F8", file_new_cb, 0, NULL},
	{N_("/File/_Open..."), "<control>O", file_open_cb, 0, NULL},
	{N_("/File/Open advanced..."), NULL, file_open_cb, 1, NULL},
	{N_("/File/Open r_ecent"), NULL, NULL, 0, "<Branch>"},
	{N_("/File/Open recent/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/File/_Revert to saved"), NULL, file_revert_to_saved_cb, 0, NULL},
	{N_("/File/_Insert..."), NULL, file_insert_cb, 0, NULL},
	{N_("/File/sep2"), NULL, NULL, 0, "<Separator>"},
	{N_("/File/_Save"), "<control>S", file_save_cb, 0, NULL},
	{N_("/File/Save _as..."), NULL, file_save_as_cb, 0, NULL},
	{N_("/File/_Move to..."), NULL, file_move_to_cb, 0, NULL},
	{N_("/File/Sa_ve all"), NULL, file_save_all_cb, 0, NULL},
	{N_("/File/sep3"), NULL, NULL, 0, "<Separator>"},
	{N_("/File/_Close"), "<control>w", file_close_cb, 0, NULL},
	{N_("/File/Close all"), NULL, file_close_all_cb, 0, NULL},
	{N_("/File/sep5"), NULL, NULL, 0, "<Separator>"},
	{N_("/File/_Quit"), "<control>Q", bluefish_exit_request, 0, NULL},
	{N_("/_Edit"), NULL, NULL, 0, "<Branch>"},
	{N_("/Edit/Tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Edit/Cu_t"), "<control>x", edit_cut_cb, 0, NULL},
	{N_("/Edit/_Copy"), "<control>c", edit_copy_cb, 0, NULL},
	{N_("/Edit/_Paste"), "<control>v", edit_paste_cb, 0, NULL},
	{N_("/Edit/Select _all"), "<control>a", edit_select_all_cb, 0, NULL},
	{N_("/Edit/sep3"), NULL, NULL, 0, "<Separator>"},
	{N_("/Edit/_Find..."), "F6", search_cb, 0, NULL},
	{N_("/Edit/Ne_w find..."), NULL, new_search_cb, 0, NULL},
	{N_("/Edit/Find agai_n"), "<shift>F6", search_again_cb, 0, NULL},
	{N_("/Edit/R_eplace..."), "F7", replace_cb, 0, NULL},
	{N_("/Edit/New re_place..."), NULL, new_replace_cb, 0, NULL},
	{N_("/Edit/Replace a_gain"), "<shift>F7", replace_again_cb, 0, NULL},
	{N_("/Edit/Replace _special"), NULL, NULL, 0, "<Branch>"},
	{N_("/Edit/Replace special/Tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Edit/Replace special/_ascii to HTML entities"), NULL, doc_convert_asciichars_in_selection, 1, NULL},
	{N_("/Edit/Replace special/_iso8859 to HTML entities"), NULL, doc_convert_asciichars_in_selection, 2, NULL},
	{N_("/Edit/Replace special/_both types to HTML entities"), NULL, doc_convert_asciichars_in_selection, 3, NULL},
	{N_("/Edit/sep4"), NULL, NULL, 0, "<Separator>"},
	{N_("/Edit/_Undo"), "<control>z", undo_cb, 0, NULL},
	{N_("/Edit/_Redo"), "<control>r", redo_cb, 0, NULL},
	{N_("/Edit/Undo all"), NULL, undo_all_cb, 0, NULL},
	{N_("/Edit/Redo all"), NULL, redo_all_cb, 0, NULL},
	{N_("/Edit/sep5"), NULL, NULL, 0, "<Separator>"},
	{N_("/Edit/_Goto line"), NULL, go_to_line_win_cb, 0, NULL},
	{N_("/Edit/G_oto selection"), NULL, go_to_line_from_selection_cb, 0, NULL},
	{N_("/_Tags"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Tearoff1"), NULL, NULL, 0, "<Tearoff>"},
/*	{N_("/Tags/Repeat last"), "F4", repeat_last_insert_cb, 0, NULL},*/
	{N_("/Tags/_Headings"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Headings/Tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Tags/Headings/H_1"), "<alt>F1", general_html_cb, 18, NULL},
	{N_("/Tags/Headings/H_2"), "<alt>F2", general_html_cb, 19, NULL},
	{N_("/Tags/Headings/H_3"), "<alt>F3", general_html_cb, 20, NULL},
	{N_("/Tags/Headings/H_4"), "<alt>F4", general_html_cb, 21, NULL},
	{N_("/Tags/Headings/H_5"), "<alt>F5", general_html_cb, 22, NULL},
	{N_("/Tags/Headings/H_6"), "<alt>F6", general_html_cb, 23, NULL},
	{N_("/Tags/_Special"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Special/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Tags/Special/_Accent"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Special/Accent/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Tags/Special/Accent/_Uppercase"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Special/Accent/Uppercase/_A-I"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Special/Accent/Uppercase/A-I/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Tags/Special/Accent/Uppercase/A-I/A grave"), NULL, insert_char_cb, 0, NULL},
	{N_("/Tags/Special/Accent/Uppercase/A-I/A acute"), NULL, insert_char_cb, 1, NULL},
	{N_("/Tags/Special/Accent/Uppercase/A-I/A circumflex"), NULL, insert_char_cb, 2, NULL},
	{N_("/Tags/Special/Accent/Uppercase/A-I/A tilde"), NULL, insert_char_cb, 3, NULL},
	{N_("/Tags/Special/Accent/Uppercase/A-I/AE ligature"), NULL, insert_char_cb, 4, NULL},
	{N_("/Tags/Special/Accent/Uppercase/A-I/A diaeresis"), NULL, insert_char_cb, 5, NULL},
	{N_("/Tags/Special/Accent/Uppercase/A-I/A ring"), NULL, insert_char_cb, 6, NULL},
	{N_("/Tags/Special/Accent/Uppercase/A-I/C cedilla"), NULL, insert_char_cb, 7, NULL},
	{N_("/Tags/Special/Accent/Uppercase/A-I/E grave"), NULL, insert_char_cb, 8, NULL},
	{N_("/Tags/Special/Accent/Uppercase/A-I/E acute"), NULL, insert_char_cb, 9, NULL},
	{N_("/Tags/Special/Accent/Uppercase/A-I/E circumflex"), NULL, insert_char_cb, 10, NULL},
	{N_("/Tags/Special/Accent/Uppercase/A-I/E diaeresis"), NULL, insert_char_cb, 11, NULL},
	{N_("/Tags/Special/Accent/Uppercase/A-I/I grave"), NULL, insert_char_cb, 12, NULL},
	{N_("/Tags/Special/Accent/Uppercase/A-I/I acute"), NULL, insert_char_cb, 13, NULL},
	{N_("/Tags/Special/Accent/Uppercase/A-I/I circumflex"), NULL, insert_char_cb, 14, NULL},
	{N_("/Tags/Special/Accent/Uppercase/A-I/I diaeresis"), NULL, insert_char_cb, 15, NULL},
	{N_("/Tags/Special/Accent/Uppercase/_J-Z"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Special/Accent/Uppercase/J-Z/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Tags/Special/Accent/Uppercase/J-Z/N tilde"), NULL, insert_char_cb, 16, NULL},
	{N_("/Tags/Special/Accent/Uppercase/J-Z/O grave"), NULL, insert_char_cb, 17, NULL},
	{N_("/Tags/Special/Accent/Uppercase/J-Z/O acute"), NULL, insert_char_cb, 18, NULL},
	{N_("/Tags/Special/Accent/Uppercase/J-Z/O circumflex"), NULL, insert_char_cb, 19, NULL},
	{N_("/Tags/Special/Accent/Uppercase/J-Z/OE ligature"), NULL, insert_char_cb, 100, NULL},
	{N_("/Tags/Special/Accent/Uppercase/J-Z/O tilde"), NULL, insert_char_cb, 20, NULL},
	{N_("/Tags/Special/Accent/Uppercase/J-Z/O diaeresis"), NULL, insert_char_cb, 21, NULL},
	{N_("/Tags/Special/Accent/Uppercase/J-Z/O slash"), NULL, insert_char_cb, 22, NULL},
	{N_("/Tags/Special/Accent/Uppercase/J-Z/U grave"), NULL, insert_char_cb, 23, NULL},
	{N_("/Tags/Special/Accent/Uppercase/J-Z/U acute"), NULL, insert_char_cb, 24, NULL},
	{N_("/Tags/Special/Accent/Uppercase/J-Z/U circumflex"), NULL, insert_char_cb, 25, NULL},
	{N_("/Tags/Special/Accent/Uppercase/J-Z/U diaeresis"), NULL,
	 insert_char_cb, 26, NULL},
	{N_("/Tags/Special/Accent/Uppercase/J-Z/Y acute"), NULL, insert_char_cb,
	 27, NULL},
	{N_("/Tags/Special/Accent/_Lowercase"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Special/Accent/Lowercase/_A-I"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Special/Accent/Lowercase/A-I/tearoff1"), NULL, NULL, 0,
	 "<Tearoff>"},
	{N_("/Tags/Special/Accent/Lowercase/A-I/a grave"), NULL, insert_char_cb,
	 28, NULL},
	{N_("/Tags/Special/Accent/Lowercase/A-I/a acute"), NULL, insert_char_cb,
	 29, NULL},
	{N_("/Tags/Special/Accent/Lowercase/A-I/a circumflex"), NULL,
	 insert_char_cb, 30, NULL},
	{N_("/Tags/Special/Accent/Lowercase/A-I/a tilde"), NULL, insert_char_cb,
	 31, NULL},
	{N_("/Tags/Special/Accent/Lowercase/A-I/a ring"), NULL, insert_char_cb,
	 32, NULL},
	{N_("/Tags/Special/Accent/Lowercase/A-I/ae ligature"), NULL,
	 insert_char_cb, 33, NULL},
	{N_("/Tags/Special/Accent/Lowercase/A-I/a diaeresis"), NULL,
	 insert_char_cb, 34, NULL},
	{N_("/Tags/Special/Accent/Lowercase/A-I/c cedilla"), NULL, insert_char_cb,
	 35, NULL},
	{N_("/Tags/Special/Accent/Lowercase/A-I/e grave"), NULL, insert_char_cb,
	 36, NULL},
	{N_("/Tags/Special/Accent/Lowercase/A-I/e acute"), NULL, insert_char_cb,
	 37, NULL},
	{N_("/Tags/Special/Accent/Lowercase/A-I/e circumflex"), NULL,
	 insert_char_cb, 38, NULL},
	{N_("/Tags/Special/Accent/Lowercase/A-I/e diaeresis"), NULL,
	 insert_char_cb, 39, NULL},
	{N_("/Tags/Special/Accent/Lowercase/A-I/i grave"), NULL, insert_char_cb,
	 40, NULL},
	{N_("/Tags/Special/Accent/Lowercase/A-I/i acute"), NULL, insert_char_cb,
	 41, NULL},
	{N_("/Tags/Special/Accent/Lowercase/A-I/i circumflex"), NULL,
	 insert_char_cb, 42, NULL},
	{N_("/Tags/Special/Accent/Lowercase/A-I/i diaeresis"), NULL,
	 insert_char_cb, 43, NULL},
	{N_("/Tags/Special/Accent/Lowercase/_J-Z"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Special/Accent/Lowercase/J-Z/tearoff1"), NULL, NULL, 0,
	 "<Tearoff>"},
	{N_("/Tags/Special/Accent/Lowercase/J-Z/n tilde"), NULL, insert_char_cb,
	 44, NULL},
	{N_("/Tags/Special/Accent/Lowercase/J-Z/o grave"), NULL, insert_char_cb,
	 45, NULL},
	{N_("/Tags/Special/Accent/Lowercase/J-Z/o acute"), NULL, insert_char_cb,46, NULL},
	{N_("/Tags/Special/Accent/Lowercase/J-Z/oe ligature"), NULL, insert_char_cb,99, NULL},
	{N_("/Tags/Special/Accent/Lowercase/J-Z/o circumflex"), NULL,
	 insert_char_cb, 47, NULL},
	{N_("/Tags/Special/Accent/Lowercase/J-Z/o tilde"), NULL, insert_char_cb,
	 48, NULL},
	{N_("/Tags/Special/Accent/Lowercase/J-Z/o diaeresis"), NULL,
	 insert_char_cb, 49, NULL},
	{N_("/Tags/Special/Accent/Lowercase/J-Z/o slash"), NULL, insert_char_cb,
	 50, NULL},
	{N_("/Tags/Special/Accent/Lowercase/J-Z/u grave"), NULL, insert_char_cb,
	 51, NULL},
	{N_("/Tags/Special/Accent/Lowercase/J-Z/u acute"), NULL, insert_char_cb,
	 52, NULL},
	{N_("/Tags/Special/Accent/Lowercase/J-Z/u circumflex"), NULL,
	 insert_char_cb, 53, NULL},
	{N_("/Tags/Special/Accent/Lowercase/J-Z/u diaeresis"), NULL,
	 insert_char_cb, 54, NULL},
	{N_("/Tags/Special/Accent/Lowercase/J-Z/y acute"), NULL, insert_char_cb,
	 55, NULL},
	{N_("/Tags/Special/Accent/Lowercase/J-Z/y diaeresis"), NULL,
	 insert_char_cb, 56, NULL},
	{N_("/Tags/Special/Accent/separator"), NULL, NULL, 0, "<Separator>"},
	{N_("/Tags/Special/Accent/U_mlaut"), NULL, insert_char_cb, 57, NULL},
	{N_("/Tags/Special/Accent/_Acute"), NULL, insert_char_cb, 58, NULL},
	{N_("/Tags/Special/Accent/_Cedilla"), NULL, insert_char_cb, 59, NULL},
	{N_("/Tags/Special/_Currency"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Special/Currency/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Tags/Special/Currency/_Cent sign"), NULL, insert_char_cb,
	 60, NULL},
	{N_("/Tags/Special/Currency/_Pound sterling"), NULL, insert_char_cb,
	 61, NULL},
	{N_("/Tags/Special/Currency/C_urrency sign"), NULL, insert_char_cb,
	 62, NULL},
	{N_("/Tags/Special/Currency/_Yen sign"), NULL, insert_char_cb, 63, NULL},
	{N_("/Tags/Special/Currency/_Euro"), NULL, insert_char_cb, 98, NULL},
	{N_("/Tags/Special/_Math-Science"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Special/Math-Science/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Tags/Special/Math-Science/Logical _not sign"), "<alt>1", insert_char_cb,
	 64, NULL},
	{N_("/Tags/Special/Math-Science/_Multiplication sign"), "<alt>8",
	 insert_char_cb, 65, NULL},
	{N_("/Tags/Special/Math-Science/_Division sign"), "<alt>slash", insert_char_cb,
	 66, NULL},
	{N_("/Tags/Special/Math-Science/_Plus-minus sign"), NULL, insert_char_cb,
	 67, NULL},
	{N_("/Tags/Special/Math-Science/_Less-than sign"), "<alt>comma", insert_char_cb,
	 68, NULL},
	{N_("/Tags/Special/Math-Science/_Greater-than sign"), "<alt>period", insert_char_cb,
	 69, NULL},
	{N_("/Tags/Special/Math-Science/Superscript _1"), NULL, insert_char_cb,
	 70, NULL},
	{N_("/Tags/Special/Math-Science/Superscript _2"), NULL, insert_char_cb,
	 71, NULL},
	{N_("/Tags/Special/Math-Science/Superscript _3"), NULL, insert_char_cb,
	 72, NULL},
	{N_("/Tags/Special/Math-Science/One _quarter"), NULL, insert_char_cb,
	 73, NULL},
	{N_("/Tags/Special/Math-Science/One _half"), NULL, insert_char_cb,
	 74, NULL},
	{N_("/Tags/Special/Math-Science/_Three quarters"), NULL, insert_char_cb,
	 75, NULL},
	{N_("/Tags/Special/Math-Science/Deg_ree sign"), NULL, insert_char_cb,
	 76, NULL},
	{N_("/Tags/Special/_Non Latin"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Special/Non Latin/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Tags/Special/Non Latin/_ETH (Icelandic)"), NULL, insert_char_cb,
	 77, NULL},
	{N_("/Tags/Special/Non Latin/_THORN (Icelandic)"), NULL, insert_char_cb,
	 78, NULL},
	{N_("/Tags/Special/Non Latin/et_h (Icelandic)"), NULL, insert_char_cb,
	 79, NULL},
	{N_("/Tags/Special/Non Latin/th_orn"), NULL, insert_char_cb, 80, NULL},
	{N_("/Tags/Special/Non Latin/Sharp _s"), NULL, insert_char_cb, 81, NULL},
	{N_("/Tags/Special/Non Latin/_Micro sign"), NULL, insert_char_cb,
	 82, NULL},
	{N_("/Tags/Special/_Other"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Special/Other/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Tags/Special/Other/_Non-breaking space"), NULL, insert_char_cb,
	 83, NULL},
	{N_("/Tags/Special/Other/_Section sign"), NULL, insert_char_cb,
	 84, NULL},
	{N_("/Tags/Special/Other/_Copyright sign"), NULL, insert_char_cb,
	 85, NULL},
	{N_("/Tags/Special/Other/_Left angle quotes"), NULL, insert_char_cb,
	 86, NULL},
	{N_("/Tags/Special/Other/_Right angle quotes"), NULL, insert_char_cb,
	 87, NULL},
	{N_("/Tags/Special/Other/Registered _trademark"), NULL, insert_char_cb,
	 88, NULL},
	{N_("/Tags/Special/Other/Inverted _exclamation"), NULL, insert_char_cb,
	 89, NULL},
	{N_("/Tags/Special/Other/Inverted _question mark"), NULL, insert_char_cb,
	 90, NULL},
	{N_("/Tags/Special/Other/_Feminine ordinal"), NULL, insert_char_cb,
	 91, NULL},
	{N_("/Tags/Special/Other/_Masculine ordinal"), NULL, insert_char_cb,
	 92, NULL},
	{N_("/Tags/Special/Other/_Pilcrow (paragraph sign)"), NULL, insert_char_cb,
	 93, NULL},
	{N_("/Tags/Special/Other/_Broken bar"), NULL, insert_char_cb, 94, NULL},
	{N_("/Tags/Special/Other/Soft _hypen"), "<alt>minus", insert_char_cb, 95, NULL},
	{N_("/Tags/Special/Other/Spa_cing macron"), NULL, insert_char_cb,
	 96, NULL},
	{N_("/Tags/Special/Other/Middle _dot"), NULL, insert_char_cb,
	 97, NULL},
	{N_("/Tags/_Format by layout"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Format by layout/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Tags/Format by layout/_Bold"), "<alt>v", general_html_cb, 1, NULL},
	{N_("/Tags/Format by layout/_Italic"), "<alt>i", general_html_cb, 2, NULL},
	{N_("/Tags/Format by layout/_Underline"), "<alt>u", general_html_cb, 3, NULL},
	{N_("/Tags/Format by layout/_Strikeout"), "<alt>s", general_html_cb, 4, NULL},	
	{N_("/Tags/F_ormat by context"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Format by context/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Tags/Format by context/_Strong"), NULL, general_html_cb, 16, NULL},
	{N_("/Tags/Format by context/_Emphasis"), NULL, general_html_cb, 17, NULL},
	{N_("/Tags/Format by context/_Define"), NULL, general_html_cb, 48, NULL},
	{N_("/Tags/Format by context/_Code"), NULL, general_html_cb, 49, NULL},
	{N_("/Tags/Format by context/Sa_mple"), NULL, general_html_cb, 50, NULL},
	{N_("/Tags/Format by context/_Keyboard"), NULL, general_html_cb, 51, NULL},
	{N_("/Tags/Format by context/_Variable"), NULL, general_html_cb, 52, NULL},
	{N_("/Tags/Format by context/Ci_tation"), NULL, general_html_cb, 53, NULL},
	{N_("/Tags/Format by context/_Abbreviation"), NULL, general_html_cb, 54, NULL},
	{N_("/Tags/Format by context/Ac_ronym"), NULL, general_html_cb, 55, NULL},
	{N_("/Tags/Format _general"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Format general/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Tags/Format general/_Paragraph"), "<alt>P", general_html_cb, 5, NULL},
	{N_("/Tags/Format general/_Break"),  "<Control><Alt>r", general_html_cb, 6, NULL},
	{N_("/Tags/Format general/Break clear _all"), NULL, general_html_cb, 41, NULL},
	{N_("/Tags/Format general/_Non-Breaking Space"), "<alt>n", general_html_cb, 7, NULL},
	{N_("/Tags/Format general/<separator>"), NULL, NULL, 0, "<Separator>"},
	{N_("/Tags/Format general/Font Si_ze +1"), "<control>equal", general_html_cb, 11, NULL},
	{N_("/Tags/Format general/Font _Size -1"), "<control>minus", general_html_cb, 12, NULL},
	{N_("/Tags/Format general/<separator>"), NULL, NULL, 0, "<Separator>"},
	{N_("/Tags/Format general/Preformatted _Text"), "<Control><Alt>p", general_html_cb, 13, NULL},
	{N_("/Tags/Format general/Su_bscript"), "<alt><shift>s", general_html_cb, 14, NULL},
	{N_("/Tags/Format general/Su_perscript"), "<alt><control>s", general_html_cb, 15, NULL},
	{N_("/Tags/Format general/<separator>"), NULL, NULL, 0, "<Separator>"},
	{N_("/Tags/Format general/_Center"), "<alt>c", general_html_cb, 8, NULL},
	{N_("/Tags/Format general/Align _right"), "<Alt>r", general_html_cb, 9, NULL},
	{N_("/Tags/_Table"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/Table/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Tags/Table/_Table"), NULL, general_html_cb, 24, NULL},
	{N_("/Tags/Table/Table_row"), NULL, general_html_cb, 25, NULL},
	{N_("/Tags/Table/Table_header"), NULL, general_html_cb, 26, NULL},
	{N_("/Tags/Table/Table_data"), NULL, general_html_cb, 27, NULL},
	{N_("/Tags/Table/Table_caption"), NULL, general_html_cb, 28, NULL},
	{N_("/Tags/_List"), NULL, NULL, 0, "<Branch>"},
	{N_("/Tags/List/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Tags/List/_Unordered list"), NULL, general_html_cb, 33, NULL},
	{N_("/Tags/List/_Ordered list"), NULL, general_html_cb, 34, NULL},
	{N_("/Tags/List/List _item"), "<alt>l", general_html_cb, 35, NULL},
	{N_("/Tags/List/Definition _list"), NULL, general_html_cb, 36, NULL},
	{N_("/Tags/List/Definition _term"), NULL, general_html_cb, 37, NULL},
	{N_("/Tags/List/_Definition"), NULL, general_html_cb, 38, NULL},
	{N_("/Tags/List/_Menu"), NULL, general_html_cb, 39, NULL},
	{N_("/Tags/_Insert Generator META-Tag"), NULL, general_html_cb, 47, NULL},
	{N_("/Tags/_Comment"), NULL, general_html_cb, 10, NULL},
	{N_("/Dialo_gs"), NULL, NULL, 0, "<Branch>"},
	{N_("/Dialogs/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Dialogs/_General"), NULL, NULL, 0, "<Branch>"},
	{N_("/Dialogs/General/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Dialogs/General/_Quickstart..."), "<alt>q", quickstart_cb, 0, NULL},
/*	{N_("/Dialogs/General/DTD"), NULL, dtd_cb, 0, NULL},
	{N_("/Dialogs/General/Head"), NULL, head_cb, 0, NULL}, */
	{N_("/Dialogs/General/_Body..."), NULL, body_cb, 0, NULL},
	{N_("/Dialogs/General/_Anchor..."), "<alt>a", quickanchor_cb, 0, NULL},
	{N_("/Dialogs/General/_Email..."), NULL, email_cb, 0, NULL},
	{N_("/Dialogs/General/_Rule..."), "<alt>h", quickrule_cb, 0, NULL},
	{N_("/Dialogs/General/_Font..."), NULL, fontdialog_cb, 0, NULL},
	{N_("/Dialogs/General/Basef_ont..."), NULL, basefont_cb, 0, NULL},
	{N_("/Dialogs/General/Quick_list..."), NULL, quicklist_cb, 0, NULL},
	{N_("/Dialogs/General/_Meta..."), "<alt>m", meta_cb, 0, NULL},
	{N_("/Dialogs/General/Embe_d..."), NULL, embed_cb, 0, NULL},
	{N_("/Dialogs/General/Select _colour..."), NULL, sel_colour_cb, 0, NULL},
	{N_("/Dialogs/General/Insert _time..."), NULL, insert_time_cb, 0, NULL},
	{N_("/Dialogs/General/Insert _image..."), "<control>i", image_insert_dialog_cb, 0, NULL},
	{N_("/Dialogs/General/Insert t_humbnail..."), "<control>t", thumbnail_insert_dialog_cb, 0, NULL},
	{N_("/Dialogs/_Table"), NULL, NULL, 0, "<Branch>"},
	{N_("/Dialogs/Table/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Dialogs/Table/Table _wizard..."), NULL, tablewizard, 0, NULL},
	{N_("/Dialogs/Table/_Table..."), NULL, tabledialog_cb, 0, NULL},
	{N_("/Dialogs/Table/Table_row..."), NULL, tablerowdialog_cb, 0, NULL},
	{N_("/Dialogs/Table/Table_head..."), NULL, tableheaddialog_cb, 0, NULL},
	{N_("/Dialogs/Table/Table_data..."), NULL, tabledatadialog_cb, 0, NULL},
	{N_("/Dialogs/_CSS"), NULL, NULL, 0, "<Branch>"},
	{N_("/Dialogs/CSS/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Dialogs/CSS/_Create style..."), NULL, new_css_dialog, 0, NULL},
	{N_("/Dialogs/CSS/S_pan..."), NULL, span_dialog, 0, NULL},
	{N_("/Dialogs/CSS/_Div..."), NULL, div_dialog, 0, NULL},
	{N_("/Dialogs/CSS/_Style..."), NULL, general_html_cb, 42, NULL},
	{N_("/Dialogs/CSS/_Link to stylesheet..."), NULL, link_cb, 0, NULL},
	{N_("/Dialogs/_Frame"), NULL, NULL, 0, "<Branch>"},
	{N_("/Dialogs/Frame/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Dialogs/Frame/Frame _wizard..."), NULL, framewizard, 0, NULL},
	{N_("/Dialogs/Frame/Frame_set..."), NULL, framesetdialog_cb, 0, NULL},
	{N_("/Dialogs/Frame/_Frame..."), NULL, framedialog_cb, 0, NULL},
	{N_("/Dialogs/F_orm"), NULL, NULL, 0, "<Branch>"},
	{N_("/Dialogs/Form/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Dialogs/Form/F_orm..."), NULL, formdialog_cb, 0, NULL},
	{N_("/Dialogs/Form/_Input button..."), NULL, buttondialog_cb, 0, NULL},
	{N_("/Dialogs/Form/_Text..."), NULL, textdialog_cb, 0, NULL},
	{N_("/Dialogs/Form/_Hidden..."), NULL, hiddendialog_cb, 0, NULL},
	{N_("/Dialogs/Form/Text_area..."), NULL, textareadialog_cb, 0, NULL},
	{N_("/Dialogs/Form/Radio _buttons..."), NULL, radiodialog_cb, 0, NULL},
	{N_("/Dialogs/Form/_Check buttons..."), NULL, checkdialog_cb, 0, NULL},
	{N_("/Dialogs/Form/_Select..."), NULL, selectdialog_cb, 0, NULL},
	{N_("/Dialogs/Form/O_ption..."), NULL, optiondialog_cb, 0, NULL},
	{N_("/Dialogs/Form/Option _group..."), NULL, optgroupdialog_cb, 0, NULL},
/*	{N_("/Dialogs/Javascript"), NULL, NULL, 0, "<Branch>"},
	{N_("/Dialogs/Javascript/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Dialogs/Javascript/Mouseover Script"), NULL, mouseover_script_cb, 0, NULL},
	{N_("/Dialogs/Javascript/Netscape 4 Resize Bugfix Script"), NULL, ns47_bugfix_script_cb, 0, NULL},
	{N_("/Dialogs/Javascript/Image Preload Script"), NULL, imagepreload_script_cb, 0, NULL},
	{N_("/Dialogs/_WML"), NULL, NULL, 0, "<Branch>"},
	{N_("/Dialogs/WML/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Dialogs/WML/Standard Document"), NULL, general_wml_cb, 6, NULL},
	{N_("/Dialogs/WML/Card..."), NULL, carddialog_cb, 0, NULL},
	{N_("/Dialogs/WML/Postfield..."), NULL, postfielddialog_cb, 0, NULL},
	{N_("/Dialogs/WML/Go..."), NULL, godialog_cb, 0, NULL},
	{N_("/Dialogs/WML/Do..."), NULL, dodialog_cb, 0, NULL},
	{N_("/Dialogs/WML/Anchor..."), NULL, anchordialog_cb, 0, NULL},
	{N_("/Dialogs/WML/Access..."), NULL, accessdialog_cb, 0, NULL},
	{N_("/Dialogs/WML/sep11"), NULL, NULL, 0, "<Separator>"},
	{N_("/Dialogs/WML/Paragraph"), NULL, general_wml_cb, 1, NULL},
	{N_("/Dialogs/WML/Line Break"), NULL, general_wml_cb, 2, NULL},
	{N_("/Dialogs/WML/Italic"), NULL, general_wml_cb, 3, NULL},
	{N_("/Dialogs/WML/Non-Breaking Space"), NULL, general_wml_cb, 4, NULL},
	{N_("/Dialogs/WML/Bold"), NULL, general_wml_cb, 5, NULL},
	{N_("/Dialogs/WML/sep12"), NULL, NULL, 0, "<Separator>"},
	{N_("/Dialogs/WML/Prev"), NULL, general_wml_cb, 7, NULL},
	{N_("/Dialogs/WML/Refresh"), NULL, general_wml_cb, 8, NULL},
	{N_("/Dialogs/WML/Noop"), NULL, general_wml_cb, 9, NULL},
	{N_("/Dialogs/WML/sep13"), NULL, NULL, 0, "<Separator>"},
	{N_("/Dialogs/WML/Set Variable..."), NULL, vardialog_cb, 0, NULL},*/
	{N_("/E_xternal"), NULL, NULL, 0, "<Branch>"},
	{N_("/External/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
/*	{N_("/External/_Commands"), NULL, NULL, 0, "<Branch>"},
	{N_("/External/Commands/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/External/_Outputbox"), NULL, NULL, 0, "<Branch>"},
	{N_("/External/Outputbox/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/External/sep1"), NULL, NULL, 0, "<Separator>"},*/
	{N_("/_Document"), NULL, NULL, 0, "<Branch>"},
	{N_("/Document/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Document/_Type"), NULL, NULL, 0, "<Branch>"},
	{N_("/Document/Type/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Document/_Encoding"), NULL, NULL, 0, "<Branch>"},
	{N_("/Document/Encoding/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Document/_Highlight syntax"), NULL, doc_toggle_highlighting_cb, 0, "<ToggleItem>"},
	{N_("/Document/_Wrap"), NULL, doc_toggle_wrap_cb, 0, "<ToggleItem>"},
	{N_("/Document/_Line numbers"), NULL, doc_toggle_linenumbers_cb, 0, "<ToggleItem>"},
	{N_("/Document/sep1"), NULL, NULL, 0, "<Separator>"},
	{N_("/Document/_Update highlighting"), "F5", doc_update_highlighting, 0, NULL},
	{N_("/Document/_Next"), "F1", gui_notebook_switch, 1, NULL},
	{N_("/Document/_Previous"), "F2", gui_notebook_switch, 0, NULL},
	{N_("/Document/sep1"), NULL, NULL, 0, "<Separator>"},
	{N_("/Document/_Word Count"), NULL, word_count_cb, 0, NULL},
	{N_("/_Options"), NULL, NULL, 0, "<Branch>"},
	{N_("/Options/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Options/_Preferences"), NULL, open_preferences_menu_cb, 0, NULL},
	{N_("/Options/_Display"), NULL, NULL, 0, "<Branch>"},
	{N_("/Options/Display/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Options/Display/View _Main toolbar"), NULL, gui_toggle_hidewidget_cb, 0, "<ToggleItem>"},
	{N_("/Options/Display/View _HTML toolbar"), NULL, gui_toggle_hidewidget_cb, 1, "<ToggleItem>"},
	{N_("/Options/Display/View _Custom menu"), NULL, gui_toggle_hidewidget_cb, 2, "<ToggleItem>"},
	{N_("/Options/Display/View _Left panel"), NULL, gui_toggle_hidewidget_cb, 3, "<ToggleItem>"},
/*	{N_("/Options/sep1"), NULL, NULL, 0, "<Separator>"},*/
	{N_("/Options/_Tabsize"), NULL, NULL, 0, "<Branch>"},
	{N_("/Options/Tabsize/tearoff1"), NULL, NULL, 0, "<Tearoff>"},
	{N_("/Options/Tabsize/_Increase tabsize"), NULL, gui_change_tabsize, 1, NULL},
	{N_("/Options/Tabsize/_Decrease tabsize"), NULL, gui_change_tabsize, 0, NULL},
	{N_("/Options/sep2"), NULL, NULL, 0, "<Separator>"},
	{N_("/Options/_Auto indent"), NULL, gui_toggle_autoindent_cb, 0, "<ToggleItem>"},
	{N_("/Options/sep2"), NULL, NULL, 0, "<Separator>"},
	{N_("/Options/_Save settings"), NULL, rcfile_save_configfile_menu_cb, 0, NULL},
	{N_("/Options/Save shortcut _keys"), NULL, rcfile_save_configfile_menu_cb, 3, NULL}
};

#ifdef ENABLE_NLS
gchar *menu_translate(const gchar * path, gpointer data) {
	gchar *retval;
/*	static gchar *menupath = NULL;
	if (menupath) g_free(menupath);
	menupath = g_strdup(path);
	if ((strstr(path, "/tearoff1") != NULL) 
			|| (strstr(path, "/---") != NULL) 
			|| (strstr(path, "/sep1") != NULL)) {
		DEBUG_MSG("menu_translate, nogettext returning %s for %s\n", menupath, path);
		return menupath;
	}*/
	retval = gettext(path);
	DEBUG_MSG("menu_translate, returning %s for %s\n", retval, path);
	return retval;
}
#endif       

/************************************************/
/* generic functions for dynamic created menu's */
/************************************************/

static GtkWidget *find_menuitem_in_list_by_label(GList *itemlist, gchar *label) {
	GList *tmplist;

	tmplist = g_list_first(itemlist);
	while (tmplist) {
		if(!strcmp(GTK_LABEL(GTK_BIN(tmplist->data)->child)->label, label)) {
			return GTK_WIDGET(tmplist->data);
		}
		tmplist = g_list_next(tmplist);
	}
	return NULL;
}

static GtkWidget *remove_menuitem_in_list_by_label(gchar *labelstring, GList **menuitemlist) {
	GList *tmplist;
	gpointer tmp;

	tmplist = g_list_first(*menuitemlist);
	while (tmplist) {
		DEBUG_MSG("remove_recent_entry, tmplist=%p, data=%p\n", tmplist, tmplist->data);
		DEBUG_MSG("remove_recent_entry, tmplist->data=%s\n",GTK_LABEL(GTK_BIN(tmplist->data)->child)->label);
		if(!strcmp(GTK_LABEL(GTK_BIN(tmplist->data)->child)->label, labelstring)) {
			tmp = tmplist->data;
			*menuitemlist = g_list_remove(*menuitemlist, tmplist->data);
			DEBUG_MSG("remove_recent_entry, returning %p\n", tmp);
			return tmp;
		}
		tmplist = g_list_next(tmplist);
	}
	return NULL;
}

/* the result of this function can be added to the menuitem-list */
static GtkWidget *create_dynamic_menuitem(gchar *menubasepath, gchar *label, GCallback callback, gpointer data, gint menu_insert_offset) {
	GtkWidget *tmp, *menu;
	GtkItemFactory *factory;

	/* add it to main_v->menubar */
	factory = gtk_item_factory_from_widget(main_v->menubar);
	menu = gtk_item_factory_get_widget(factory, menubasepath);
	DEBUG_MSG("create_dynamic_menuitem, menubar=%p, menu=%p basepath=%s, label=%s\n", main_v->menubar, menu, menubasepath,label);
	if (menu != NULL) {
		tmp = gtk_menu_item_new_with_label(label);
		g_signal_connect(G_OBJECT(tmp), "activate",callback, data);

		gtk_widget_show(tmp);
		if (menu_insert_offset == -1) {
			gtk_menu_shell_append(GTK_MENU_SHELL(menu),tmp);
		} else {
			gtk_menu_shell_insert(GTK_MENU_SHELL(menu),tmp,menu_insert_offset);
		}
		return tmp;
	} else {
		DEBUG_MSG("create_dynamic_menuitem, NO MENU FOR BASEPATH %s\n", menubasepath);
		return NULL;
	}
}

static void create_parent_and_tearoff(gchar *menupath, GtkItemFactory *ifactory) {
	char *basepath;
	GtkWidget *widg=NULL;
	GtkItemFactoryEntry entry;

	basepath = g_strndup(menupath, (strlen(menupath) - strlen(strrchr(menupath, '/'))));
	DEBUG_MSG("create_parent_and_tearoff, basepath=%s for menupath=%s\n", basepath, menupath);
	widg = gtk_item_factory_get_widget(ifactory, basepath);
	if (!widg) {
		DEBUG_MSG("create_parent_and_tearoff, no widget found for %s, will create it\n", basepath);
		create_parent_and_tearoff(basepath, ifactory);
		entry.path = g_strconcat(basepath, "/tearoff1", NULL);
		entry.accelerator = NULL;
		entry.callback = NULL;
		entry.callback_action = 0;
		entry.item_type = "<Tearoff>";
		gtk_item_factory_create_item(ifactory, &entry, NULL, 2);
		g_free(entry.path);
	}
}	

static void menu_current_document_type_change(GtkMenuItem *menuitem,Tfiletype *hlset) {
	DEBUG_MSG("menu_current_document_type_change, started for hlset %p\n", hlset);
	if (GTK_CHECK_MENU_ITEM(menuitem)->active) {
		if (hl_set_highlighting_type(main_v->current_document, hlset)) {
			doc_highlight_full(main_v->current_document);
		} else {
			menu_current_document_set_toggle_wo_activate(main_v->current_document->hl, NULL);
		}
	}
	DEBUG_MSG("menu_current_document_type_change, finished\n");
}

void menu_current_document_set_toggle_wo_activate(Tfiletype *hlset, gchar *encoding) {
	if (hlset && !GTK_CHECK_MENU_ITEM(hlset->menuitem)->active) {
		DEBUG_MSG("setting widget from hlset %p active\n", main_v->current_document->hl);
		g_signal_handler_disconnect(G_OBJECT(hlset->menuitem),hlset->menuitem_activate_id);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (hlset->menuitem), TRUE);
		hlset->menuitem_activate_id = g_signal_connect(G_OBJECT(hlset->menuitem), "activate",G_CALLBACK(menu_current_document_type_change), (gpointer) hlset);
	}
#ifdef DEBUG
	 else {
	 	DEBUG_MSG("widget from hlset %p is already active!!\n", main_v->current_document->hl);
	 }
#endif
	if (encoding) {
		GtkWidget *menuitem = find_menuitem_in_list_by_label(menus.encodings, encoding);
		if (menuitem) {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem),TRUE);
		}
	}
}

void filetype_menu_destroy(Tfiletype *filetype) {
	if (filetype->menuitem) {
		g_signal_handler_disconnect(filetype->menuitem,filetype->menuitem_activate_id);
		gtk_widget_destroy(filetype->menuitem);
		filetype->menuitem = NULL;
	}
}

void filetype_menu_rebuild(GtkItemFactory *item_factory) {
	GSList *group=NULL;
	GtkWidget *parent_menu;
	GList *tmplist = g_list_first(main_v->filetypelist);
	if (!item_factory) {
		item_factory = gtk_item_factory_from_widget(main_v->menubar);
	}
	DEBUG_MSG("filetype_menu_rebuild, adding filetypes in menu\n");
	parent_menu = gtk_item_factory_get_widget(item_factory, N_("/Document/Type"));
	while (tmplist) {
		Tfiletype *filetype = (Tfiletype *)tmplist->data;
#ifdef DEBUG
		if (filetype->menuitem) {
			DEBUG_MSG("filetype_menu_rebuild, %s has a menuitem already!!!\n", filetype->type);
		}
#endif
		filetype->menuitem = gtk_radio_menu_item_new_with_label(group, filetype->type);
		filetype->menuitem_activate_id = g_signal_connect(G_OBJECT(filetype->menuitem), "activate",G_CALLBACK(menu_current_document_type_change), (gpointer) filetype);
		gtk_widget_show(filetype->menuitem);
		gtk_menu_insert(GTK_MENU(parent_menu), filetype->menuitem, 1);
		group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM(filetype->menuitem));
		tmplist = g_list_next(tmplist);
	}
}

/* 
 * menu factory crap, thanks to the gtk tutorial for this
 * both the 1.0 and the 1.2 code is directly from the tutorial
 */
void menu_create_main(GtkWidget *vbox)
{
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	gint nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
	accel_group = gtk_accel_group_new();
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<bluefishmain>", accel_group);
#ifdef ENABLE_NLS
	gtk_item_factory_set_translate_func(item_factory, menu_translate, "<bluefishmain>", NULL);
#endif
	gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);
	gtk_window_add_accel_group(GTK_WINDOW(main_v->main_window), accel_group);
	main_v->menubar = gtk_item_factory_get_widget(item_factory, "<bluefishmain>");
	gtk_box_pack_start(GTK_BOX(vbox), main_v->menubar, FALSE, TRUE, 0);
	gtk_widget_show(main_v->menubar);

	setup_toggle_item(item_factory, N_("/Options/Display/View Main toolbar"), main_v->props.view_main_toolbar);
	setup_toggle_item(item_factory, N_("/Options/Display/View HTML toolbar"), main_v->props.view_html_toolbar);
	setup_toggle_item(item_factory, N_("/Options/Display/View Custom menu"), main_v->props.view_custom_menu);
	setup_toggle_item(item_factory, N_("/Options/Display/View Left panel"), main_v->props.view_left_panel);
	setup_toggle_item(item_factory, N_("/Options/Auto indent"), main_v->props.autoindent);

	filetype_menu_rebuild(item_factory);
	
	if (main_v->props.ext_browsers_in_submenu) {
		create_parent_and_tearoff(N_("/External/Browsers/"), item_factory);
	}
	if (main_v->props.ext_commands_in_submenu) {
		create_parent_and_tearoff(N_("/External/Commands/"), item_factory);
	}
	if (main_v->props.ext_outputbox_in_submenu) {
		create_parent_and_tearoff(N_("/External/Outputbox/"), item_factory);
	}
}


/*************************************************************/
/*               Output Box handling                         */
/*************************************************************/
static void menu_outputbox_lcb(GtkMenuItem *menuitem,gchar **arr) {
	outputbox(arr[1], atoi(arr[2]), atoi(arr[3]), atoi(arr[4]), arr[5], (arr[6][0]=='1'));
}

static GtkWidget *dynamic_menu_append_spacing(gchar *basepath) {
	GtkItemFactory *factory;
	GtkWidget *menu, *menuitem;
	factory = gtk_item_factory_from_widget(main_v->menubar);
	menu = gtk_item_factory_get_widget(factory, basepath);
	menuitem = gtk_menu_item_new();
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),menuitem);
	return menuitem;
}

void menu_outputbox_rebuild() {
	GList *tmplist;
	
	if (menus.outputbox_menu) {
		tmplist = g_list_first(menus.outputbox_menu);
		while (tmplist) {
			gtk_widget_destroy(GTK_WIDGET(tmplist->data));
			tmplist = g_list_next(tmplist);
		}
		g_list_free(menus.outputbox_menu);
		menus.outputbox_menu = NULL;
	}
	if (!main_v->props.ext_outputbox_in_submenu) {
		menus.outputbox_menu = g_list_append(menus.outputbox_menu, dynamic_menu_append_spacing(N_("/External")));
	}
	
	tmplist = g_list_first(main_v->props.outputbox);
	while (tmplist) {
		gchar **arr = tmplist->data;
		/* outputbox(gchar *pattern, gint file_subpat, gint line_subpat, gint output_subpat, gchar *command, gboolean show_all_output)
		 * arr[0] = name
		 * arr[1] = pattern
		 * arr[2] = file subpattern
		 * arr[3] = line subpattern
		 * arr[4] = output subpattern
		 * arr[5] = command
		 * arr[6] = show_all_output
		 */
		if (count_array(arr)==7) {
			gchar *tmp1;
			if (main_v->props.ext_outputbox_in_submenu) {
				tmp1 = N_("/External/Outputbox");
			} else {
				tmp1 = N_("/External");
			}
			menus.outputbox_menu = g_list_append(menus.outputbox_menu
					,create_dynamic_menuitem(tmp1,arr[0],G_CALLBACK(menu_outputbox_lcb),(gpointer)arr,-1));
		}
		tmplist = g_list_next(tmplist);
	}
}
/*******************************************************************/
/*               Open Recent menu handling                         */
/*******************************************************************/
/* the only required header */
static GtkWidget *create_recent_entry(char *filename);
/*******************************************************************/

static GtkWidget *remove_recent_entry(gchar *filename) {
	GList *tmplist;
	gpointer tmp;

	if(strcmp(filename, "last") ==0) {
		tmplist = g_list_first(menus.recent_files);
		tmp = tmplist->data;
		DEBUG_MSG("remove_recent_entry, remove last entry\n");
		menus.recent_files = g_list_remove(menus.recent_files, tmplist->data);
		return tmp;
	}	else {
		return remove_menuitem_in_list_by_label(filename, &menus.recent_files);
	}
}

/* open_recent_file
 * This function should be called when a menu from the Open Recent list
 * has been selected. */
static void open_recent_file_cb(GtkWidget *widget, gpointer nothing) {
	gchar *filename = GTK_LABEL(GTK_BIN(widget)->child)->label;
	DEBUG_MSG("open_recent_file_cb, started, filename is %s\n", filename);

	statusbar_message(_("Loading file(s)..."),2000);
	flush_queue();
	if (!doc_new_with_file(filename, FALSE)) {
		gchar *message = g_strconcat(_("Could not open file\n"), filename, NULL);
		error_dialog("Bluefish error", message);
		g_free(message);
		return;
	}
	DEBUG_MSG("open_recent_file_cb, document %s opened\n", filename);
	add_to_recent_list(filename, 0);
	return;
}

/* create_recent_entry
 * This function builds the gtkitemfactoryentry and inserts it at the
 * main_v->menubar. Furthermore, it returns a pointer to it, so that
 * this pointer can be added in the main_v->recent_files list */
static GtkWidget *create_recent_entry(gchar *filename) {
	GtkWidget *tmp;

	tmp = remove_recent_entry(filename);
	if (tmp) {
		gtk_widget_hide(tmp);
		gtk_widget_destroy(tmp);
	}
	return  create_dynamic_menuitem(N_("/File/Open recent")
		, filename, G_CALLBACK(open_recent_file_cb), NULL
		, 1);
}


/* recent_menu_init()
 * Gets the list of documents from .bluefish/recentlist and inserts
 * it at the File-->Open Recent menu. If the file doesn't exist (probably
 * because this is the first time Bluefish is running) then a menu
 * item telling that no recent files exist will appear */

void recent_menu_init() {
	gchar *recentfile;
	GList *filenames=NULL, *tmp, *tmp2, *newlist=NULL;
	gint recent_file_count=0;

	DEBUG_MSG("open_recent_list, started\n");
	recentfile = g_strconcat(g_get_home_dir(), "/.bluefish/recentlist", NULL);
	filenames = get_stringlist(recentfile, filenames);
	if (!filenames) {
		filenames = add_to_stringlist(menus.recent_files, _("(none)"));
	}

	tmp = g_list_last(filenames);
	while (tmp && recent_file_count <= main_v->props.max_recent_files) {
		gint already_found=0;
		tmp2 = g_list_first(newlist);
		while (tmp2) {
			if (strcmp(tmp->data, tmp2->data)==0) {
				already_found=1;
			}
			tmp2 = g_list_next(tmp2);
		}
		if (!already_found) {
			recent_file_count++;
			DEBUG_MSG("open_recent_list, added %s to top of list\n", (gchar *)tmp->data);
			newlist = g_list_prepend(newlist, tmp->data);
		}
		already_found = 0;
		tmp = g_list_previous(tmp);
	}

	tmp2 = g_list_first(newlist);
	while (tmp2) {
		menus.recent_files  = g_list_append(menus.recent_files, create_recent_entry(tmp2->data));
		tmp2 = g_list_next(tmp2);
	}

	put_stringlist(recentfile, newlist);
	g_list_free(newlist);
	free_stringlist(filenames);

	g_free(recentfile);
}

/* Add_to_recent_list
 * This should be called when a new file is opened, i.e. from
 * file_open_cb, it adds a new entry which also appears in the
 * menu bar, and (if nessecary) deletes the last entry */
void add_to_recent_list(gchar *filename, gint closed_file) {
	DEBUG_MSG("add_to_recent_list, started for %s\n", filename);
	if (closed_file) {
		GtkWidget *tmp;

		/* First of all, create the entry and insert it at the list*/
		menus.recent_files = g_list_append(menus.recent_files,
								create_recent_entry(filename));

		DEBUG_MSG("add_to_recent_list, inserted item in menu\n");
		if(g_list_length(menus.recent_files) > main_v->props.max_recent_files) {
			tmp = remove_recent_entry("last");
			DEBUG_MSG("add_to_recent_list, list too long, entry %s to be deleted\n", GTK_LABEL(GTK_BIN(tmp)->child)->label);
			gtk_widget_hide(tmp);
			gtk_widget_destroy(tmp);
		}
/*		flush_queue();*/

	} else {
		gchar *tmpfilename, *recentfile;
		recentfile = g_strconcat(g_get_home_dir(), "/.bluefish/recentlist", NULL);
		/* save the new list */
		tmpfilename = g_strconcat(filename, "\n", NULL);
		DEBUG_MSG("add_to_recent_list, trying to append to %s\n", recentfile);
		append_string_to_file(recentfile, tmpfilename);
		g_free(recentfile);
		g_free(tmpfilename);
	}
}

/*****************/
/* Browsers!!    */
/*****************/

static void view_in_browser(gchar *browser) {
	if (main_v->current_document->filename) {
		Tconvert_table *table, *tmpt;
		gchar *command;
		table = tmpt = g_new(Tconvert_table, 2);
		tmpt->my_int = 's';
		tmpt->my_char = main_v->current_document->filename;
		tmpt++;
		tmpt->my_char = NULL;
		command = replace_string_printflike(browser, table);
		g_free(table);
		DEBUG_MSG("view_in_browser, should start %s now\n", command);
		system(command);
		g_free(command);
	} else {
		error_dialog(_("Bluefish error: no filename"), _("Could not view file in browser, the file does not yet have a name\n"));
	}
}

void browser_toolbar_cb(GtkWidget *widget, gpointer data) {
	GList *tmplist = g_list_first(main_v->props.browsers);
	if (tmplist && tmplist->data) {
		gchar **arr = tmplist->data;
		DEBUG_MSG("first browser in main_v->props.browsers(%p) is %s with command %s\n", main_v->props.browsers, arr[0], arr[1]);
		view_in_browser(arr[1]);
	}
}

static void browser_lcb(GtkWidget *widget, gchar **arr) {
	if (!main_v->current_document->filename) {
		file_save_cb(NULL, NULL);
	}
	view_in_browser(arr[1]);
}
static void external_command_lcb(GtkWidget *widget, gchar **arr) {
	gchar *secure_tempname = NULL;
	gboolean need_s=FALSE, need_f=FALSE;

	file_save_cb(NULL, NULL);
	if (!main_v->current_document->filename) {
		return;
	}
	change_dir(main_v->current_document->filename);
	/* now check if
	 * %s - we need a filename 
	 * %f - output filename that we need to read after the command has finished (filter)
	 */
	need_f = (int)strstr(arr[1], "%f");
	need_s = (int)strstr(arr[1], "%s");
	if (need_f || need_s) {
		gchar *command;
		Tconvert_table *table, *tmpt;
		table = tmpt = g_new(Tconvert_table, 3);
		if (need_s) {
			if (!main_v->current_document->filename) {
				file_save_cb(NULL, NULL);
				if (!main_v->current_document->filename) {
					g_free(table);
					return;
				}
			}
			DEBUG_MSG("adding 's' to table\n");
			tmpt->my_int = 's';
			tmpt->my_char = main_v->current_document->filename;
			tmpt++;
		}
		if (need_f) {
			secure_tempname = create_secure_dir_return_filename();
			DEBUG_MSG("adding 'f' to table\n");
			tmpt->my_int = 'f';
			tmpt->my_char = secure_tempname;
			tmpt++;
		}
		tmpt->my_char = NULL;
		command = replace_string_printflike(arr[1], table);
		g_free(table);
		system(command);
		g_free(command);
		if (need_f) {
			gint end;
			gchar *buf = NULL;
			gboolean suc6;
			/* empty textbox and fill from file secure_tempname */
			end = doc_get_max_offset(main_v->current_document);
			suc6 = g_file_get_contents(secure_tempname, &buf, NULL, NULL);
			if (suc6 && buf) {
				if (strlen(buf)) {
					doc_replace_text(main_v->current_document, buf, 0, end);
				}
				g_free(buf);
			}
			remove_secure_dir_and_filename(secure_tempname);
		}
	} else {
		DEBUG_MSG("external_command_lcb, about to start %s\n", arr[1]);
		system(arr[1]);
	}
}
void external_menu_rebuild() {
	GList *tmplist;
	if (menus.external_menu) {
		tmplist = g_list_first(menus.external_menu);
		while (tmplist) {
			gtk_widget_destroy(tmplist->data);
			tmplist = g_list_next(tmplist);
		}
		g_list_free(menus.external_menu);
		menus.external_menu = NULL;
	}


	if (!main_v->props.ext_browsers_in_submenu) {
		menus.external_menu = g_list_append(menus.external_menu
					,dynamic_menu_append_spacing(N_("/External")));
	}
	tmplist = g_list_first(main_v->props.browsers);
	while (tmplist) {
		gchar **arr = tmplist->data;
		/*  arr[0] = name
		 *  arr[1] = command
		 */
		if (count_array(arr)==2) {
			gchar *tmp1;
			if (main_v->props.ext_browsers_in_submenu) {
				tmp1 = N_("/External/Browsers");
			} else {
				tmp1 = N_("/External");
			}
			DEBUG_MSG("Adding browser %s with command %s to the menu\n", arr[0], arr[1]);
			menus.external_menu = g_list_append(menus.external_menu
					, create_dynamic_menuitem(tmp1,arr[0],G_CALLBACK(browser_lcb),arr,-1));
		}
#ifdef DEBUG
		else {
			DEBUG_MSG("need count=2 for browser menu! %p has count %d\n", arr, count_array(arr));
		}
#endif
		tmplist = g_list_next(tmplist);
	}
	
	if (!main_v->props.ext_commands_in_submenu) {
		menus.external_menu = g_list_append(menus.external_menu
					,dynamic_menu_append_spacing(N_("/External")));
	}
	
	tmplist = g_list_first(main_v->props.external_commands);
	while (tmplist) {
		gchar **arr = tmplist->data;
		/*  arr[0] = name
		 *  arr[1] = command
		 */
		if (count_array(arr)==2) {
			gchar *tmp1;
			if (main_v->props.ext_commands_in_submenu) {
				tmp1 = N_("/External/Commands");
			} else {
				tmp1 = N_("/External");
			}		
			menus.external_menu = g_list_append(menus.external_menu
					, create_dynamic_menuitem(tmp1,arr[0],G_CALLBACK(external_command_lcb),arr,-1));
		}
		tmplist = g_list_next(tmplist);
	}
}

static void menu_current_document_encoding_change(GtkMenuItem *menuitem,gchar *encoding) {
	if (GTK_CHECK_MENU_ITEM(menuitem)->active) {
		if (encoding && (!main_v->current_document->encoding || strcmp(encoding,main_v->current_document->encoding)!=0)) {
			if (main_v->current_document->encoding) {
				g_free(main_v->current_document->encoding);
			}
			main_v->current_document->encoding = g_strdup(encoding);
			if (main_v->props.auto_set_encoding_meta) {
				update_encoding_meta_in_file(main_v->current_document, main_v->current_document->encoding);
			}
			DEBUG_MSG("menu_current_document_encoding_change, set to %s\n", encoding);
		}
	}
}

void encoding_menu_rebuild() {
	GSList *group=NULL;
	GtkWidget *parent_menu;
	GList *tmplist;
	if (menus.encodings) {
		tmplist = g_list_first(menus.encodings);
		while (tmplist) {
			gtk_widget_destroy(GTK_WIDGET(tmplist->data));
			tmplist = g_list_next(tmplist);
		}
		g_list_free(menus.encodings);
		menus.encodings = NULL;
	}
	tmplist = g_list_last(main_v->props.encodings);
	parent_menu = gtk_item_factory_get_widget(gtk_item_factory_from_widget(main_v->menubar), N_("/Document/Encoding"));
	while (tmplist) {
		gchar **strarr = (gchar **)tmplist->data;
		if (count_array(strarr)==2) {
			GtkWidget *menuitem = gtk_radio_menu_item_new_with_label(group, strarr[0]);
			g_signal_connect(G_OBJECT(menuitem), "activate",G_CALLBACK(menu_current_document_encoding_change), (gpointer) strarr[1]);
			gtk_widget_show(menuitem);
			gtk_menu_insert(GTK_MENU(parent_menu), menuitem, 1);
			group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));
			menus.encodings = g_list_append(menus.encodings, menuitem);
		}
		tmplist = g_list_previous(tmplist);
	}
	
}

/***************/
/* custom menu */
/***************/
#define MAX_TEXT_ENTRY 10
typedef struct {
	GtkWidget *dialog;
	GtkWidget *textentry[MAX_TEXT_ENTRY];
	gint type;
	gchar **array;
} Tcust_con_struc;

typedef struct {
	GtkItemFactoryEntry entry;
	gchar **array;
} Tcmenu_entry;

/*
Tcust_con_struc->array[0] = title / menupath
Tcust_con_struc->array[1] = type:
										0 = custom dialog
										1 = custom search and replace

** for dialog **
Tcust_con_struc->array[2] = formatstring before, containing %0, %1... that should be replaced by the 
				values from the dialog
Tcust_con_struc->array[3] = formatstring after
Tcust_con_struc->array[4] = number of variables from the dialog
Tcust_con_struc->array[5..] = the description of those variables

** for search and replace **
Tcust_con_struc->array[2] = search pattern, containing %0 etc.
Tcust_con_struc->array[3] = replace pattern, containing %0 etc.
Tcust_con_struc->array[4] = replace where:
										0 = from beginning
										1 = from cursor
										2 = selection (selection required)
										3 = all open documents
										4 = ask
Tcust_con_struc->array[5] = replace type:
										0 = normal
										1 = regular expression
Tcust_con_struc->array[6] = case sensitivity:
										0 = no
										1 = yes
Tcust_con_struc->array[7] = number of variables from the dialog
Tcust_con_struc->array[8..] = the description of those variables
*/

static void cust_con_struc_dialog_destroy_lcb(GtkWidget *widget, GdkEvent *event,  Tcust_con_struc *ccs) {
	window_destroy(ccs->dialog);
	g_free(ccs);
}

static void cust_con_struc_dialog_cancel_lcb(GtkWidget *widget, gpointer data) {
	cust_con_struc_dialog_destroy_lcb(NULL, NULL, data);
}

static void cust_con_struc_dialog_ok_lcb(GtkWidget *widget, Tcust_con_struc *ccs) {
	Tconvert_table *table, *tmpt;
	gint num_vars, i;

	DEBUG_MSG("cust_con_struc_dialog_ok_lcb, ccs at %p\n", ccs);
	DEBUG_MSG("cust_con_struc_dialog_ok_lcb, array at %p, &array[0]=%p\n", ccs->array, &ccs->array[0]);
	DEBUG_MSG("cust_con_struc_dialog_ok_lcb, array[0] at %p, *array=%p\n", ccs->array[0], *ccs->array);
	if (ccs->type == 0) {
		gchar *before=NULL, *after=NULL;
		num_vars = atoi(ccs->array[4]);
		DEBUG_MSG("cust_con_struc_dialog_ok_lcb, num_vars=%d, ccs->array[3]=%s\n", num_vars, ccs->array[3]);
		table = tmpt = g_new(Tconvert_table, num_vars+1);
		for (i=0; i<num_vars; i++) {
			DEBUG_MSG("cust_con_struc_dialog_ok_lcb, tmpt=%p, i=%d\n", tmpt, i);
			tmpt->my_int = 48 + i;
			tmpt->my_char = gtk_editable_get_chars(GTK_EDITABLE(ccs->textentry[i]), 0, -1);
			tmpt++;
		}
		DEBUG_MSG("cust_con_struc_dialog_ok_lcb, setting tmpt(%p) to NULL\n", tmpt);
		tmpt->my_char = NULL;

		if (strlen(ccs->array[2])) {
			DEBUG_MSG("cust_con_struc_dialog_ok_lcb, ccs->array[2]=%s\n",ccs->array[2] );
			before = replace_string_printflike(ccs->array[2], table);
		}
		if (strlen(ccs->array[3])) {
			after = replace_string_printflike(ccs->array[3], table);
		}
		doc_insert_two_strings(main_v->current_document, before, after);
		tmpt = table;
		while (tmpt->my_char) {
			DEBUG_MSG("cust_con_struc_dialog_ok_lcb, tmpt=%p, about to free(%p) %s\n", tmpt, tmpt->my_char, tmpt->my_char);
			g_free(tmpt->my_char);
			tmpt++;
		}
		g_free(table);

		if (before) {
			g_free(before);
		}
		if (after) {
			g_free(after);
		}
	} else {
		gchar *search=NULL, *replace=NULL;
		num_vars = atoi(ccs->array[7]);
		table = tmpt = g_new(Tconvert_table, num_vars+1);
		for (i=0; i<num_vars; i++) {
			tmpt->my_int = 48 + i;
			tmpt->my_char = gtk_editable_get_chars(GTK_EDITABLE(ccs->textentry[i]), 0, -1);
			tmpt++;
		}
		tmpt->my_char = NULL;
		if (strlen(ccs->array[2])) {
			DEBUG_MSG("cust_con_struc_dialog_ok_lcb, ccs->array[2]=%s\n",ccs->array[2] );
			search = replace_string_printflike(ccs->array[2], table);
		}
		if (strlen(ccs->array[3])) {
			replace = replace_string_printflike(ccs->array[3], table);
		} else {
			replace = g_strdup("");
		}
		snr2_run_extern_replace(search, atoi(ccs->array[4]),
				atoi(ccs->array[5]), atoi(ccs->array[6]), replace, TRUE);
		
		tmpt = table;
		while (tmpt->my_char) {
			g_free(tmpt->my_char);
			tmpt++;
		}
		g_free(table);
		
		if (search) {
			g_free(search);
		}
		if (replace) {
			g_free(replace);
		} 
	}
	cust_con_struc_dialog_cancel_lcb(NULL, ccs);
}

static void cust_con_struc_dialog(gchar **array, gint type) {
	Tcust_con_struc *ccs;
	GtkWidget *vbox, *hbox, *okb, *cancb;
	gint i, num_vars;

	ccs = g_malloc(sizeof(Tcust_con_struc));
	ccs->type = type;
	DEBUG_MSG("cust_con_struc_dialog_cb, ccs at %p\n", ccs);
	ccs->array = array;
	DEBUG_MSG("cust_con_struc_dialog_cb, array at %p, &array[0]=%p\n", ccs->array, &ccs->array[0]);
	DEBUG_MSG("cust_con_struc_dialog_cb, array[0] at %p, *array=%p\n", ccs->array[0], *ccs->array);
	ccs->dialog = window_full(ccs->array[0], GTK_WIN_POS_MOUSE,  
			5, G_CALLBACK(cust_con_struc_dialog_cancel_lcb), ccs, TRUE);
	vbox = gtk_vbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(ccs->dialog), vbox);
	DEBUG_MSG("cust_con_struc_dialog_cb, ccs->array[0]=%s\n", ccs->array[0]);
	
	if (type == 0) {
		num_vars = atoi(ccs->array[4]);
	} else {
		num_vars = atoi(ccs->array[7]);
	}
	DEBUG_MSG("cust_con_struc_dialog_cb, num_vars=%d\n", num_vars);

	for (i=0; i<num_vars; i++) {
		hbox = gtk_hbox_new(FALSE, 0);
		if (type ==0) {
			gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(ccs->array[i+5]), TRUE, TRUE, 2);
		} else {
			gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(ccs->array[i+8]), TRUE, TRUE, 2);
		}
		ccs->textentry[i] = gtk_entry_new();
		DEBUG_MSG("cust_con_struc_dialog_cb, textentry[%d]=%p\n", i, ccs->textentry[i]);
		gtk_box_pack_start(GTK_BOX(hbox), ccs->textentry[i], TRUE, TRUE, 0);		
		gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	}

	hbox = gtk_hbox_new(TRUE, 0);
	okb = bf_stock_ok_button(G_CALLBACK(cust_con_struc_dialog_ok_lcb), ccs);
	cancb = bf_stock_cancel_button(G_CALLBACK(cust_con_struc_dialog_cancel_lcb), ccs);
	gtk_box_pack_start(GTK_BOX(hbox), okb, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), cancb, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

	gtk_widget_grab_focus(ccs->textentry[0]);
	gtk_widget_show_all(ccs->dialog);
}


static void cust_menu_lcb(GtkWidget * widget, gpointer data) {
	Tcmenu_entry *cmentry;

	cmentry = (Tcmenu_entry *) g_list_nth_data(menus.cmenu_entries, GPOINTER_TO_INT(data));
	if (strcmp(cmentry->array[1], "0") ==0) {
		DEBUG_MSG("cust_menu_lcb, a custom dialog, array[4]=%s\n", cmentry->array[4]);
		if (atoi(cmentry->array[4]) > 0) {
		     cust_con_struc_dialog(cmentry->array, 0);
		} else {
		     doc_insert_two_strings(main_v->current_document, cmentry->array[2],cmentry->array[3]);
		}
	} else {
		DEBUG_MSG("cust_menu_lcb, a custom search and replace!, cmentry->array[7]=%s\n", cmentry->array[7]);
		if (strcmp(cmentry->array[4], "2")==0 && !doc_has_selection(main_v->current_document)) {
			error_dialog(_("Bluefish error"), _("This custom search and replace requires a selection"));
			return;
		}
		if (atoi(cmentry->array[7]) > 0) {
			cust_con_struc_dialog(cmentry->array, 1);
		} else {
		     snr2_run_extern_replace(cmentry->array[2], atoi(cmentry->array[4]),
							atoi(cmentry->array[5]), atoi(cmentry->array[6]), cmentry->array[3],TRUE);
		}
	}
}

static void fill_cust_menubar() {
	GtkItemFactory *ifactory;
	gint count;
	gchar **splittedstring;
	GList *tmplist;
	Tcmenu_entry *cmentry;

	ifactory = gtk_item_factory_from_widget(menus.cmenu);

	tmplist = g_list_first(menus.cmenu_entries);
	while (tmplist) {
		cmentry = (Tcmenu_entry *) tmplist->data;
		gtk_item_factory_delete_entry(ifactory, &cmentry->entry);
		DEBUG_MSG("fill_cust_menubar, removed entry.path=%s\n", cmentry->entry.path);
		g_free(cmentry->entry.path);
		g_free(cmentry);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(menus.cmenu_entries);
	menus.cmenu_entries = NULL;

	count = 0;
	tmplist = g_list_first(main_v->props.cust_menu);
	while (tmplist) {
		gint count2;
		splittedstring = (gchar **) tmplist->data;
		count2 = count_array(splittedstring);
		
		DEBUG_MSG("fill_cust_menubar, splittedstring[0]='%s', splittedstring[1]='%s'\n", splittedstring[0], splittedstring[1]);
		if (count2 < 5) {
			DEBUG_MSG("fill_cust_menubar, array count < 5, (count2=%d) this is invalid\n", count2);
#ifdef DEBUGARRAYS
			debug_array(splittedstring);
#endif
			tmplist = g_list_next(tmplist);
			continue;
		}
		if ((strcmp(splittedstring[1] , "1")==0) && (count2 < 8)) {
			DEBUG_MSG("fill_cust_menubar, type 1, array count < 8, this is invalid\n");
			tmplist = g_list_next(tmplist);
			continue;
		}
		
		cmentry = g_malloc0(sizeof(Tcmenu_entry));
		cmentry->entry.path = g_strdup(splittedstring[0]);
		DEBUG_MSG("fill_cust_menubar, entry.path=%s, count=%d\n", cmentry->entry.path, count);
		cmentry->entry.callback = cust_menu_lcb;
		cmentry->entry.callback_action = count;
		cmentry->array = splittedstring;
		create_parent_and_tearoff(cmentry->entry.path, ifactory);
		gtk_item_factory_create_item(ifactory, &cmentry->entry, GINT_TO_POINTER(count), 2);
		menus.cmenu_entries = g_list_append(menus.cmenu_entries, cmentry);
		count++;
		tmplist = g_list_next(tmplist);
	}
}
/* function declaration needed here */
void cmenu_editor(GtkWidget *widget, gpointer data);

void make_cust_menubar(GtkWidget *cust_handle_box)
{
	static GtkItemFactoryEntry cust_menu[] = {
		{N_("/_Custom menu"), NULL, NULL, 0, "<Branch>"},
		{N_("/Custom menu/sep"), NULL, NULL, 0, "<Tearoff>"},
		{N_("/Custom menu/Edit custom menu..."), NULL, cmenu_editor, 0, NULL}/*,
		{N_("/Custom menu/Reset custom menu"), NULL, reset_cust_menu_list_cb, 0, NULL}*/
	};
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	gint nmenu_items = sizeof(cust_menu) / sizeof(cust_menu[0]);

	DEBUG_MSG("make_cust_menubar, started\n");

	/* this should only happen once !!!!!!!!!! */
	accel_group = gtk_accel_group_new();
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<bluefishcustom>", accel_group);
#ifdef ENABLE_NLS
	gtk_item_factory_set_translate_func(item_factory, menu_translate, "<bluefishcustom>", NULL);
#endif
	gtk_item_factory_create_items(item_factory, nmenu_items, cust_menu, NULL);
	gtk_window_add_accel_group(GTK_WINDOW(main_v->main_window), accel_group);

	menus.cmenu = gtk_item_factory_get_widget(item_factory, "<bluefishcustom>");
	gtk_container_add(GTK_CONTAINER(cust_handle_box), menus.cmenu);
	gtk_widget_show(menus.cmenu);

	fill_cust_menubar();

	DEBUG_MSG("make_cust_menubar, finished\n");
}

/*******************************************************************/
/*               Custom menu editor                                */
/*******************************************************************/
typedef struct {
	GtkWidget *win;
	GtkWidget *type[2];
	GtkListStore *lstore;
	GtkWidget *lview;
	GtkWidget *label1;
	GtkWidget *label2;
	GtkWidget *menupath;
/*	GtkWidget *befv;*/
	GtkTextBuffer *befb;
/*	GtkWidget *aftv;*/
	GtkTextBuffer *aftb;
	GtkWidget *num;
	gchar **lastarray;
	GtkWidget *dynvbox;
	GtkWidget *hboxes[MAX_TEXT_ENTRY];
	GtkWidget *descriptions[MAX_TEXT_ENTRY];
	GtkWidget *csnr_box;
	GtkWidget *region;
	GtkWidget *matching;
	GtkWidget *is_case_sens;
	GList *worklist;
} Tcmenu_editor;

static void cme_destroy_lcb(GtkWidget *widget, GdkEvent *event, Tcmenu_editor* cme) {
	window_destroy(cme->win);
	free_arraylist(cme->worklist);
	g_free(cme);
}

static void cme_close_lcb(GtkWidget *widget, gpointer data) {
	cme_destroy_lcb(NULL, NULL, data);
}

static void cme_ok_lcb(GtkWidget *widget, Tcmenu_editor *cme) {
	GList *tmplist;

	tmplist = main_v->props.cust_menu;
	main_v->props.cust_menu = cme->worklist;
	cme->worklist = tmplist;
	cme_destroy_lcb(NULL, NULL, cme);
	fill_cust_menubar();
}

static void cme_create_entries(Tcmenu_editor *cme, gint num) {
	gint i;

	for (i = 0; i < MAX_TEXT_ENTRY ; i++) {
		if (i < num) {
			gtk_widget_show(cme->hboxes[i]);
		} else {
			gtk_widget_hide(cme->hboxes[i]); 
		}
	}
}

static void cme_lview_selection_changed(GtkTreeSelection *selection, Tcmenu_editor *cme) {
	GtkTreeIter iter;
	GtkTreeModel *model;
	if (gtk_tree_selection_get_selected (selection,&model,&iter)) {
		GList *tmplist;
		gchar *selected_value;
		gchar **tmparr;
		gint num=0, i;
		gint type=0;

		gtk_tree_model_get(model, &iter, 0, &selected_value, -1);
		tmplist = g_list_first(cme->worklist);
		while(tmplist) {
			tmparr = (gchar **)tmplist->data;
			if (strcmp(tmparr[0], selected_value)==0) {
				cme->lastarray = (gchar **)tmplist->data;
			}
			tmplist = g_list_next(tmplist);
		}
		DEBUG_MSG("cme_clist_select_lcb, lastarray=%p, lastarray[0]=%s\n", cme->lastarray, cme->lastarray[0]);

		i = count_array(cme->lastarray);
		if (i<5) {
			DEBUG_MSG("cme_clist_select_lcb, invalid array count! (<5)\n");
			cme->lastarray = NULL;
			return;
		}
		if (strcmp(cme->lastarray[1], "0")==0) {
			type = 0;
			DEBUG_MSG("cme_clist_select_lcb, type=%d\n", type);
		}
		if (strcmp(cme->lastarray[1], "1")==0) {
			type = 1;
			if (i < 8) {
				DEBUG_MSG("cme_clist_select_lcb, invalid array count (<8 type=1)!\n");
				cme->lastarray = NULL;
				return;
			}
			DEBUG_MSG("cme_clist_select_lcb, type=%d\n", type);
		}
		if (type > 1) {
			DEBUG_MSG("cme_clist_select_lcb, invalid type! (type=%d)\n", type);
			cme->lastarray = NULL;
			return;
		}

		DEBUG_MSG("cme_clist_select_lcb, cme->lastarray[0]=%s, [i]='%s'\n", cme->lastarray[0], cme->lastarray[1]);
		gtk_entry_set_text(GTK_ENTRY(cme->menupath), cme->lastarray[0]);

		DEBUG_MSG("cme_clist_select_lcb, cme->lastarray[2]='%s'\n", cme->lastarray[2]);
		gtk_text_buffer_set_text(cme->befb, cme->lastarray[2], -1);

		DEBUG_MSG("cme_clist_select_lcb, cme->lastarray[3]='%s'\n", cme->lastarray[3]);
		gtk_text_buffer_set_text(cme->aftb, cme->lastarray[3], -1);

		DEBUG_MSG("cme_clist_select_lcb, type=%d\n", type);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cme->type[type]), TRUE);
		if (type == 0) {
			DEBUG_MSG("cme_clist_select_lcb, type=0, custom dialog\n");
			gtk_widget_hide(cme->csnr_box);
		
			num = atoi(cme->lastarray[4]);
			DEBUG_MSG("cme_clist_select_lcb, num=%d\n", num);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(cme->num), num);
	
			cme_create_entries(cme, num);
			DEBUG_MSG("cme_clist_select_lcb, %d entries created\n", num);
			for (i = 0 ; i < num; i++) {
				gtk_entry_set_text(GTK_ENTRY(cme->descriptions[i]), cme->lastarray[i+5]);
			}
			for (i = num ; i < MAX_TEXT_ENTRY; i++) {
				gtk_entry_set_text(GTK_ENTRY(cme->descriptions[i]), "");
			}
		}
		if (type == 1) {
			static Tconvert_table table1[] = {{0, "0"}, {1, "1"}, {0, NULL}};
			static Tconvert_table table2[] = {{0, N_("in current document")}, {1, N_("from cursor")}, {2, N_("in selection")}, {3, N_("in all open documents")}, {0,NULL}};
			static Tconvert_table table3[] = {{0, N_("normal")}, {1, N_("posix regular expresions")}, {2, N_("perl regular expresions")}, {0, NULL}};
			gint converti;
			gchar *convertc;
			DEBUG_MSG("cme_clist_select_lcb, type=1, custom search and replace\n");
			gtk_widget_show(cme->csnr_box);
			DEBUG_MSG("cme_clist_select_lcb, cme->lastarray[5]=%s\n", cme->lastarray[5]);
			
			gtk_editable_delete_text(GTK_EDITABLE(GTK_COMBO(cme->matching)->entry), 0, -1);
			converti = atoi(cme->lastarray[5]);
			convertc = table_convert_int2char(table3, converti);
			if (convertc) {
				gint pos=0;
				gtk_editable_insert_text(GTK_EDITABLE(GTK_COMBO(cme->matching)->entry), convertc, strlen(convertc), &pos);
			}

			DEBUG_MSG("cme_clist_select_lcb, cme->lastarray[6]=%s\n", cme->lastarray[6]);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cme->is_case_sens), table_convert_char2int(table1, cme->lastarray[6]));
			
			gtk_editable_delete_text(GTK_EDITABLE(GTK_COMBO(cme->region)->entry), 0, -1);
			converti = atoi(cme->lastarray[4]);
			convertc = table_convert_int2char(table2, converti);
			if (convertc) {
				gint pos=0;
				gtk_editable_insert_text(GTK_EDITABLE(GTK_COMBO(cme->region)->entry), convertc, strlen(convertc), &pos);
			}

			num = atoi(cme->lastarray[7]);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(cme->num), num);
	
			cme_create_entries(cme, num);
			for (i = 0 ; i < num; i++) {
				gtk_entry_set_text(GTK_ENTRY(cme->descriptions[i]), cme->lastarray[i+8]);
			}
			for (i = num ; i < MAX_TEXT_ENTRY; i++) {
				gtk_entry_set_text(GTK_ENTRY(cme->descriptions[i]), "");
			}
		}
		DEBUG_MSG("cme_clist_select_lcb, finished\n");
	} else {
		gint i;
		gtk_entry_set_text(GTK_ENTRY(cme->menupath), "");
		{
			GtkTextIter itstart, itend;
			gtk_text_buffer_get_bounds(cme->befb,&itstart,&itend);
			gtk_text_buffer_delete(cme->befb,&itstart,&itend);
			gtk_text_buffer_get_bounds(cme->aftb,&itstart,&itend);
			gtk_text_buffer_delete(cme->aftb,&itstart,&itend);
		}
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(cme->num), 0);
		for (i = 0 ; i < MAX_TEXT_ENTRY; i++) {
			gtk_entry_set_text(GTK_ENTRY(cme->descriptions[i]), "");
		}
		cme->lastarray = NULL;
		DEBUG_MSG("cme_clist_unselect_lcb, lastarray=%p\n", cme->lastarray);
	}
}

static void cme_spin_changed_lcb(GtkWidget *widget, Tcmenu_editor *cme) {
	cme_create_entries(cme, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(cme->num)));
}

static void cme_type_changed_lcb(GtkWidget *widget, Tcmenu_editor *cme) {
	if (GTK_TOGGLE_BUTTON(cme->type[1])->active) {
		DEBUG_MSG("cme_clist_select_lcb, type[1] is active\n");
		gtk_widget_show(cme->csnr_box);
		gtk_label_set_text(GTK_LABEL(cme->label1), _("Search pattern"));
		gtk_label_set_text(GTK_LABEL(cme->label2), _("Replace string"));
	} else {
		gtk_widget_hide(cme->csnr_box);
		gtk_label_set_text(GTK_LABEL(cme->label1), _("Formatstring before"));
		gtk_label_set_text(GTK_LABEL(cme->label2), _("Formatstring after"));
	}
}

static gchar **cme_create_array(Tcmenu_editor *cme) {
	gchar **newarray;
	gint num, i, type;
	
	gtk_spin_button_update(GTK_SPIN_BUTTON(cme->num));
	num  = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(cme->num));
	DEBUG_MSG("cme_create_array, num=%d\n", num);
	if (GTK_TOGGLE_BUTTON(cme->type[1])->active) {
		type = 1;
	} else {
		type = 0;
	}
	if (type == 0) {
		newarray = g_malloc0((num+6) * sizeof(char *));
	} else {
		newarray = g_malloc0((num+9) * sizeof(char *));
	}
	newarray[0] = gtk_editable_get_chars(GTK_EDITABLE(cme->menupath), 0, -1);
	{
		gboolean invalid=FALSE;
		GList *tmplist = g_list_first(cme->worklist);
		while (tmplist) {
			gchar **tmparr = (gchar **)tmplist->data;
			if (strcmp(tmparr[0],newarray[0])==0) {
				error_dialog(_("Bluefish error"), _("the menupath you want to add exists already"));
				invalid = TRUE;
				break;
			}
			tmplist = g_list_next(tmplist);
		}
		if (newarray[0][0] != '/') {
			DEBUG_MSG("cme_create_array, menupath does not start with slash, returning NULL\n");
			error_dialog(_("Bluefish error"), _("the menupath should start with a / character"));
			invalid = TRUE;
		}
		if (invalid) {
			g_free(newarray[0]);
			g_free(newarray);
			return (NULL);
		}
	}
	if (type == 0) {
		newarray[1] = g_strdup("0");

		newarray[4] = gtk_editable_get_chars(GTK_EDITABLE(cme->num), 0, -1);
		DEBUG_MSG("cme_create_array, newarray[4]=%s\n", newarray[4]);
		for (i = 0 ; i < num; i++) {
			DEBUG_MSG("cme_create_array, adding descriptions[%d] to newarray[%d]\n", i, i+5);
			newarray[5+i] = gtk_editable_get_chars(GTK_EDITABLE(cme->descriptions[i]), 0, -1);
		}
		DEBUG_MSG("cme_create_array, setting newarray[%d] to NULL\n",i+5);
		newarray[5+i] = NULL;
	} else {
		static Tconvert_table table2[] = {{0, N_("in current document")}, {1, N_("from cursor")}, {2, N_("in selection")}, {3, N_("in all open documents")}, {0,NULL}};
		static Tconvert_table table3[] = {{0, N_("normal")}, {1, N_("posix regular expresions")}, {2, N_("perl regular expresions")}, {0, NULL}};
		gint converti;
		gchar *convertc;
		newarray[1] = g_strdup("1");
		convertc = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(cme->region)->entry), 0, -1);
		converti = table_convert_char2int(table2, convertc);
		g_free(convertc);
		newarray[4] = g_strdup_printf("%d", converti);

		convertc = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(cme->matching)->entry), 0, -1);
		converti = table_convert_char2int(table3, convertc);
		g_free(convertc);
		newarray[5] = g_strdup_printf("%d", converti);
		
		newarray[6] = g_strdup_printf("%d", GTK_TOGGLE_BUTTON(cme->is_case_sens)->active);
	
		newarray[7] = gtk_editable_get_chars(GTK_EDITABLE(cme->num), 0, -1);
		DEBUG_MSG("cme_create_array, newarray[7]=%s\n", newarray[7]);
		for (i = 0 ; i < num; i++) {
			DEBUG_MSG("cme_create_array, adding descriptions[%d] to newarray[%d]\n", i, i+8);
			newarray[8+i] = gtk_editable_get_chars(GTK_EDITABLE(cme->descriptions[i]), 0, -1);
		}
		DEBUG_MSG("cme_create_array, setting newarray[%d] to NULL\n",i+8);
		newarray[8+i] = NULL;
	}
	{
		GtkTextIter itstart, itend;
		gtk_text_buffer_get_bounds(cme->befb,&itstart,&itend);
		newarray[2] = gtk_text_buffer_get_text(cme->befb,&itstart,&itend, FALSE);
		gtk_text_buffer_get_bounds(cme->aftb,&itstart,&itend);
		newarray[3] = gtk_text_buffer_get_text(cme->aftb,&itstart,&itend, FALSE);
	}

	return newarray;
}

static void cme_add_lcb(GtkWidget *widget, Tcmenu_editor *cme) {
	gchar **newarray;
	newarray = cme_create_array(cme);
	if (newarray != NULL){
	   GtkTreeIter iter;
		GtkTreeSelection *gtsel;
		cme->worklist = g_list_append(cme->worklist, newarray);
	   gtk_list_store_append(GTK_LIST_STORE(cme->lstore),&iter);
		gtk_list_store_set(GTK_LIST_STORE(cme->lstore),&iter,0,newarray[0], -1);
		cme->lastarray = newarray;
		gtsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(cme->lview));
		gtk_tree_selection_select_iter(gtsel,&iter);
	}
}

static void cme_update_lcb(GtkWidget *widget, Tcmenu_editor *cme) {
	GList *tmplist;
	gchar **newarray;
	gint row;

	if (cme->lastarray == NULL) {
		cme_add_lcb(NULL, cme);
		return;
	}

	newarray = cme_create_array(cme);
	if (newarray) {

		row = g_list_index(cme->worklist, cme->lastarray);
		tmplist = g_list_nth(cme->worklist, row);
		g_strfreev(cme->lastarray);
		DEBUG_MSG ("cme_update_lcb, row=%d\n", row);

		tmplist->data = newarray;
		{
			GtkTreeIter iter;
			gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(cme->lstore),&iter,NULL,row);
			gtk_list_store_set(GTK_LIST_STORE(cme->lstore),&iter,0,newarray[0], -1);
		}

		cme->lastarray = g_list_nth_data(cme->worklist, row);
	} else {
		DEBUG_MSG ("cme_update_lcb, no new array, cancelled\n");
	}
	DEBUG_MSG ("cme_update_lcb finished\n");
}

static void cme_delete_lcb(GtkWidget *widget, Tcmenu_editor *cme) {
	gint row;
	gchar **array;
	
	if (cme->lastarray) {
		array = cme->lastarray;
		DEBUG_MSG("cme_delete_lcb, delete lastarray %p\n", array);
		row = g_list_index(cme->worklist, array);
		DEBUG_MSG("cme_delete_lcb, g_list_length=%d\n", g_list_length(cme->worklist));
		cme->worklist = g_list_remove(cme->worklist, array);
		DEBUG_MSG("cme_delete_lcb, row=%d, g_list_length=%d\n", row, g_list_length(cme->worklist));
		{
			GtkTreeIter iter;
			gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(cme->lstore),&iter,NULL,row);
			gtk_list_store_remove(GTK_LIST_STORE(cme->lstore),&iter);
		}
		g_strfreev(array);
		cme->lastarray = NULL;
	} else {
		DEBUG_MSG("cme_delete_lcb, lastarray=NULL, nothing to delete\n");
	}
}

gint menu_entry_sort(gchar ** a,gchar ** b) {
	return strcmp(a[0],b[0]);
}

void cmenu_editor(GtkWidget *widget, gpointer data) {
	Tcmenu_editor *cme;
	GtkWidget *hbox, *vbox,*frame, *vbox2, *but, *hbox2;
	GList *tmplist, *popuplist;
	gchar **splittedstring;
	gint i;
	gchar *tmpstr;
	
	cme = g_malloc0(sizeof(Tcmenu_editor));
	DEBUG_MSG("cmenu_editor, cme is at %p\n", cme);
	cme->win = window_full(_("Edit custom menu"), GTK_WIN_POS_NONE
			, 5, G_CALLBACK(cme_destroy_lcb), cme, TRUE);

	vbox = gtk_vbox_new(FALSE, 2);
	gtk_container_add(GTK_CONTAINER(cme->win), vbox);
	/* upper area */
	hbox = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Menu path")), FALSE, TRUE, 0);
	cme->menupath = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox),cme->menupath , TRUE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

	/* clist & type area */
	vbox2 = gtk_vbox_new(FALSE, 0);	
	gtk_box_pack_start(GTK_BOX(hbox), vbox2, TRUE, TRUE, 0);
	{
		GtkWidget *scrolwin;
		GtkTreeViewColumn *column;
		GtkTreeSelection *select;
	   GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();

		cme->lstore = gtk_list_store_new (1, G_TYPE_STRING);
		cme->lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(cme->lstore));

		column = gtk_tree_view_column_new_with_attributes ("Menu path", renderer,"text", 0,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(cme->lview), column);

		scrolwin = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

		DEBUG_MSG("cmenu_editor, created lstore and lview\n");
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolwin), cme->lview);
		gtk_widget_set_usize(scrolwin, 180, 250);
		gtk_box_pack_start(GTK_BOX(vbox2), scrolwin, TRUE, TRUE, 0);
		
		select = gtk_tree_view_get_selection(GTK_TREE_VIEW(cme->lview));
		g_signal_connect(G_OBJECT(select), "changed",G_CALLBACK(cme_lview_selection_changed),cme);
	}

	cme->type[0] = gtk_radio_button_new_with_label(NULL, _("custom dialog"));
	gtk_box_pack_start(GTK_BOX(vbox2), cme->type[0], FALSE, TRUE, 0);
	cme->type[1] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(cme->type[0]), _("custom search and replace"));
	gtk_box_pack_start(GTK_BOX(vbox2), cme->type[1], FALSE, TRUE, 0);

	g_signal_connect(GTK_OBJECT(cme->type[0]), "toggled", G_CALLBACK(cme_type_changed_lcb), cme);

	/* dynamic entries area */
	vbox2 = gtk_vbox_new(FALSE, 0);	
	gtk_box_pack_start(GTK_BOX(hbox), vbox2, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox2), gtk_label_new(_("Number of variables")), FALSE, FALSE, 0);		
	cme->num = spinbut_with_value(NULL, 0, MAX_TEXT_ENTRY, 1,1);
	g_signal_connect(GTK_OBJECT(cme->num), "changed", G_CALLBACK(cme_spin_changed_lcb), cme);
	gtk_box_pack_start(GTK_BOX(vbox2),cme->num , FALSE, FALSE, 0);
	
	frame = gtk_frame_new(_("Variables"));
	gtk_box_pack_end(GTK_BOX(vbox2), frame, TRUE, TRUE, 0);
	cme->dynvbox = gtk_vbox_new(FALSE, 0);	
	gtk_container_add(GTK_CONTAINER(frame), cme->dynvbox);
	for (i = 0; i <  MAX_TEXT_ENTRY; i++) {
		cme->hboxes[i] = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(cme->dynvbox), cme->hboxes[i], FALSE, FALSE, 0);
		tmpstr = g_strdup_printf("%%%d", i);
		gtk_box_pack_start(GTK_BOX(cme->hboxes[i]), gtk_label_new(tmpstr), FALSE, FALSE, 0);
		g_free(tmpstr);
		cme->descriptions[i] = gtk_entry_new();
		gtk_box_pack_start(GTK_BOX(cme->hboxes[i]), cme->descriptions[i], TRUE, TRUE, 0);
	}

	/* lower area */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

	/* before and after text area */
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox2, TRUE, TRUE, 0);
	
	/* csnr area */
	cme->csnr_box = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), cme->csnr_box, FALSE, TRUE, 0);

	hbox2 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(cme->csnr_box), hbox2, FALSE, TRUE, 0);
	
	gtk_box_pack_start(GTK_BOX(hbox2),gtk_label_new(_("Replace")), FALSE, FALSE, 0);
	popuplist = g_list_append(NULL, _("in current document"));
	popuplist = g_list_append(popuplist, _("from cursor"));
	popuplist = g_list_append(popuplist, _("in selection"));
	popuplist = g_list_append(popuplist, _("in all open documents"));
	cme->region = combo_with_popdown(NULL, popuplist, 0);
	g_list_free(popuplist);
	gtk_box_pack_start(GTK_BOX(hbox2),cme->region , TRUE, TRUE, 3);

	hbox2 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(cme->csnr_box), hbox2, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox2),gtk_label_new(_("Matching")), FALSE, FALSE, 0);
	popuplist = g_list_append(NULL, _("normal"));
	popuplist = g_list_append(popuplist, _("posix regular expresions"));
	popuplist = g_list_append(popuplist, _("perl regular expresions"));
	cme->matching = combo_with_popdown(NULL, popuplist, 0);
	g_list_free(popuplist);
	gtk_box_pack_start(GTK_BOX(hbox2),cme->matching , TRUE, TRUE, 3);
	
	cme->is_case_sens = boxed_checkbut_with_value(_("case sensitive"), 0, cme->csnr_box);

	{
		GtkWidget *scrolwin, *textview;
		cme->label1 = gtk_label_new("");
		gtk_box_pack_start(GTK_BOX(vbox2), cme->label1, FALSE, FALSE, 0);

		scrolwin = textview_buffer_in_scrolwin(&textview, 280, 50, NULL, GTK_WRAP_NONE);
		cme->befb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
		gtk_box_pack_start(GTK_BOX(vbox2), scrolwin, TRUE, TRUE, 0);

		cme->label2 = gtk_label_new("");
		gtk_box_pack_start(GTK_BOX(vbox2), cme->label2, FALSE, FALSE, 0);
		
		scrolwin = textview_buffer_in_scrolwin(&textview, 280, 50, NULL, GTK_WRAP_NONE);
		cme->aftb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
		gtk_box_pack_start(GTK_BOX(vbox2), scrolwin, TRUE, TRUE, 0);
	}
	/* button area */
	vbox2 = gtk_vbox_new(FALSE, 0);	
	gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, TRUE, 5);
	but = bf_gtkstock_button(GTK_STOCK_ADD, G_CALLBACK(cme_add_lcb), cme);
/*	but = bf_stock_button(_("Add"), G_CALLBACK(cme_add_lcb), cme);*/
	gtk_box_pack_start(GTK_BOX(vbox2), but, FALSE, FALSE, 5);
	but = bf_gtkstock_button(GTK_STOCK_APPLY, G_CALLBACK(cme_update_lcb), cme);
/*	but = bf_stock_button(_("Update"), G_CALLBACK(cme_update_lcb), cme);*/
	gtk_box_pack_start(GTK_BOX(vbox2), but, FALSE, FALSE, 5);
	but = bf_gtkstock_button(GTK_STOCK_DELETE, G_CALLBACK(cme_delete_lcb), cme);
/*	but = bf_stock_button(_("Delete"), G_CALLBACK(cme_delete_lcb), cme);*/
	gtk_box_pack_start(GTK_BOX(vbox2), but, FALSE, FALSE, 5);

	/* frame for cancel/ok buttons */
	frame = gtk_frame_new(NULL);
	gtk_box_pack_end(GTK_BOX(vbox2), frame, FALSE, FALSE, 0);
	hbox = gtk_hbox_new(TRUE, 1);
	gtk_container_add(GTK_CONTAINER(frame), hbox);
	but = bf_stock_ok_button(G_CALLBACK(cme_ok_lcb), cme);
	gtk_window_set_default(GTK_WINDOW(cme->win), but);
	gtk_box_pack_start(GTK_BOX(hbox), but, TRUE, TRUE, 0);
	
	but = bf_stock_cancel_button(G_CALLBACK(cme_close_lcb), cme);
	gtk_box_pack_start(GTK_BOX(hbox), but, TRUE, TRUE, 0);
	/* ready !! */
	cme->worklist = duplicate_arraylist(main_v->props.cust_menu);
	cme->worklist = g_list_sort(cme->worklist, (GCompareFunc)menu_entry_sort);
	tmplist = g_list_first(cme->worklist);
	while (tmplist) {
		GtkTreeIter iter;
		splittedstring = (gchar **) tmplist->data;
		if (count_array(splittedstring) >= 5) {
			DEBUG_MSG("cmenu_editor, adding '%s'\n", splittedstring[0]);
			gtk_list_store_append(GTK_LIST_STORE(cme->lstore), &iter);
			gtk_list_store_set(GTK_LIST_STORE(cme->lstore), &iter, 0, splittedstring[0], -1);
		} else {
			DEBUG_MSG("cmenu_editor, NOT adding '%s', not a valid array_count (%d)\n", splittedstring[0], count_array(splittedstring));
		}
		tmplist = g_list_next(tmplist);
	}
	DEBUG_MSG("cmenu_editor, worklist length=%d\n", g_list_length(cme->worklist));
	gtk_widget_show_all(cme->win);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cme->type[0]), TRUE);
	cme_type_changed_lcb(NULL, cme);
}

/*************************************************************************/
