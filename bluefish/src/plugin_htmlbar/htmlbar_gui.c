/* Bluefish HTML Editor
 * htmlbar_gui.c
 *
 * Copyright (C) 2005 Olivier Sessink
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
 
#include <string.h>

#include "htmlbar.h"

#include "../stringlist.h"
#include "../bf_lib.h"
#include "../gtk_easy.h"
#include "htmlbar_gui.h"
#include "html.h"
#include "html2.h"
#include "html_form.h"
#include "html_table.h"
#include "image.h"
#include "quickstart.h"
#include "rpopup.h"
#include "wizards.h"

#include "htmlbar_icons.c"
GtkWidget *htmlbar_pixmap(const guint8 *data) {
	GtkWidget *widget;
	GdkPixbuf *pixbuf;
	pixbuf = gdk_pixbuf_new_from_inline(-1,data,FALSE,NULL);
	widget = gtk_image_new_from_pixbuf(pixbuf);
	g_object_unref(pixbuf);
	return widget;
}

static void menu_html_dialogs_lcb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget) {
	switch (callback_action) {
	case 1:
		body_dialog(bfwin,NULL);
	break;
	case 2:
		DEBUG_MSG("menu_html_dialogs_lcb,calling quickanchor_dialog with bfwin=%p and data=NULL\n",bfwin);
		quickanchor_dialog(bfwin,NULL);
	break;
	case 3:
		email_dialog(bfwin,NULL);
	break;
	case 4:
		quickrule_dialog(bfwin,NULL);
	break;
	case 5:
		font_dialog(bfwin,NULL);
	break;
	case 6:
		basefont_dialog(bfwin,NULL);
	break;
	case 7:
		quicklist_dialog(bfwin,NULL);
	break;
	case 8:
		meta_dialog(bfwin,NULL);
	break;
	case 9:
		embed_dialog(bfwin,NULL);
	break;
	case 10:
		image_insert_dialog(bfwin,NULL);
	break;
	case 11:
		thumbnail_insert_dialog(bfwin);
	break;
	case 12:
		tablewizard_dialog(bfwin);
	break;
	case 13:
		tabledialog_dialog(bfwin,NULL);
	break;
	case 14:
		tablerowdialog_dialog(bfwin,NULL);
	break;
	case 15:
		tableheaddialog_dialog(bfwin,NULL);
	break;
	case 16:
		tabledatadialog_dialog(bfwin,NULL);
	break;
	case 17:
		span_dialog(bfwin,NULL);
	break;
	case 18:
		div_dialog(bfwin,NULL);
	break;
	case 19:
		framewizard_dialog(bfwin);
	break;
	case 20:
		frameset_dialog(bfwin,NULL);
	break;
	case 21:
		frame_dialog(bfwin,NULL);
	break;
	case 22:
		formdialog_dialog(bfwin,NULL);
	break;
	case 23:
		inputdialog_dialog(bfwin, NULL, "submit");
	break;
	case 24:
		inputdialog_dialog(bfwin, NULL, "text");
	break;
	case 25:
		buttondialog_dialog(bfwin, NULL);
	break;
	case 26:
		textareadialog_dialog(bfwin,NULL);
	break;
	case 29:
		selectdialog_dialog(bfwin,NULL);
	break;
	case 30:
		optiondialog_dialog(bfwin,NULL);
	break;
	case 31:
		optgroupdialog_dialog(bfwin,NULL);
	break;
	case 32:
		quickstart_dialog_new(bfwin);
	break;
	case 33:
		inputdialog_dialog(bfwin, NULL, NULL);
	break;
	case 34:
		insert_time_dialog(bfwin);
	break;
	case 35:
		linkdialog_dialog(bfwin,NULL);
	break;
	case 36:
		new_css_dialog(NULL,bfwin);
	break;
	case 37:
		sel_colour_cb(NULL,bfwin);	
	break;
	case 38:
		edit_tag_under_cursor_cb(bfwin);
	break;
	default:
		g_print("menu_file_operations_cb, unknown action, abort!\n");
		exit(123);
	}
}

void htmlbar_view_toolbar(Thtmlbarwin *hbw, gboolean show) {
	if (show) {
		if (hbw->handlebox) {
			gtk_widget_show(hbw->handlebox);
		} else {
			htmlbar_toolbar(hbw);
		}
	} else {
		if (hbw->handlebox) gtk_widget_hide(hbw->handlebox);
	}
}

static void htmlbar_view_lcb(Thtmlbarwin *hbw,guint action,GtkWidget *widget) {
	Thtmlbarsession *hbs;
	hbs = g_hash_table_lookup(htmlbar_v.lookup,hbw->bfwin->session);
	if (hbs) {
		hbs->view_htmlbar = GTK_CHECK_MENU_ITEM(widget)->active;
	} else {
		DEBUG_MSG("htmlbar_view_lcb, ERROR, no htmlbarsession ???\n");
	}
	htmlbar_view_toolbar(hbw, GTK_CHECK_MENU_ITEM(widget)->active);
}

#ifdef ENABLE_NLS
gchar *htmlbar_menu_translate(const gchar * path, gpointer data) {
	return _(path);
}
#endif       

void htmlbar_build_menu(Thtmlbarwin *hbw) {
	GtkItemFactory *ifactory;
	Thtmlbarsession *hbs;
	Tbfwin *bfwin = hbw->bfwin;
	static GtkItemFactoryEntry menu_items[] = {
		{N_("/_Tags"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/_Headings"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/Headings/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/Headings/H_1"), "<control><alt>1", general_html_menu_cb, 18, "<ImageItem>",pixmap_fonth1},
		{N_("/Tags/Headings/H_2"), "<control><alt>2", general_html_menu_cb, 19, "<ImageItem>",pixmap_fonth2},
		{N_("/Tags/Headings/H_3"), "<control><alt>3", general_html_menu_cb, 20, "<ImageItem>",pixmap_fonth3},
		{N_("/Tags/Headings/H_4"), "<control><alt>4", general_html_menu_cb, 21, "<ImageItem>",pixmap_fonth4},
		{N_("/Tags/Headings/H_5"), "<control><alt>5", general_html_menu_cb, 22, "<ImageItem>",pixmap_fonth5},
		{N_("/Tags/Headings/H_6"), "<control><alt>6", general_html_menu_cb, 23, "<ImageItem>",pixmap_fonth6},
		{N_("/Tags/_Special"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/Special/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/Special/_Accent"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/Special/Accent/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/Special/Accent/_Uppercase"), NULL, NULL, 0, "<Branch>"},
		{N_("/Tags/Special/Accent/Uppercase/_A-I"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/Special/Accent/Uppercase/A-I/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/Special/Accent/Uppercase/A-I/A grave À"), NULL, insert_char_cb, 100, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/A-I/A acute Á"), NULL, insert_char_cb, 1, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/A-I/A circumflex Â"), NULL, insert_char_cb, 2, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/A-I/A tilde Ã"), NULL, insert_char_cb, 3, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/A-I/AE ligature Æ"), NULL, insert_char_cb, 4, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/A-I/A diaeresis Ä"), NULL, insert_char_cb, 5, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/A-I/A ring Å"), NULL, insert_char_cb, 6, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/A-I/C cedilla Ç"), NULL, insert_char_cb, 7, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/A-I/E grave È"), NULL, insert_char_cb, 8, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/A-I/E acute É"), NULL, insert_char_cb, 9, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/A-I/E circumflex Ê"), NULL, insert_char_cb, 10, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/A-I/E diaeresis Ë"), NULL, insert_char_cb, 11, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/A-I/I grave Ì"), NULL, insert_char_cb, 12, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/A-I/I acute Í"), NULL, insert_char_cb, 13, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/A-I/I circumflex Î"), NULL, insert_char_cb, 14, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/A-I/I diaeresis Ï"), NULL, insert_char_cb, 15, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/_J-Z"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/Special/Accent/Uppercase/J-Z/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/Special/Accent/Uppercase/J-Z/N tilde Ñ"), NULL, insert_char_cb, 16, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/J-Z/O grave Ò"), NULL, insert_char_cb, 17, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/J-Z/O acute Ó"), NULL, insert_char_cb, 18, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/J-Z/O circumflex Ô"), NULL, insert_char_cb, 19, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/J-Z/OE ligature Œ"), NULL, insert_char_cb, 100, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/J-Z/O tilde Õ"), NULL, insert_char_cb, 20, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/J-Z/O diaeresis Ö"), NULL, insert_char_cb, 21, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/J-Z/O slash Ø"), NULL, insert_char_cb, 22, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/J-Z/U grave Ù"), NULL, insert_char_cb, 23, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/J-Z/U acute Ú"), NULL, insert_char_cb, 24, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/J-Z/U circumflex Û"), NULL, insert_char_cb, 25, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/J-Z/U diaeresis Ü"), NULL, insert_char_cb, 26, "<Item>"},
		{N_("/Tags/Special/Accent/Uppercase/J-Z/Y acute Ý"), NULL, insert_char_cb, 27, "<Item>"},
		{N_("/Tags/Special/Accent/_Lowercase"), NULL, NULL, 0, "<Branch>"},
		{N_("/Tags/Special/Accent/Lowercase/_A-I"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/Special/Accent/Lowercase/A-I/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/Special/Accent/Lowercase/A-I/a grave à"), NULL, insert_char_cb, 28, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/A-I/a acute á"), NULL, insert_char_cb, 29, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/A-I/a circumflex â"), NULL, insert_char_cb, 30, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/A-I/a tilde ã"), NULL, insert_char_cb, 31, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/A-I/a ring å"), NULL, insert_char_cb, 32, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/A-I/ae ligature æ"), NULL, insert_char_cb, 33, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/A-I/a diaeresis ä"), NULL, insert_char_cb, 34, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/A-I/c cedilla ç"), NULL, insert_char_cb, 35, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/A-I/e grave è"), NULL, insert_char_cb, 36, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/A-I/e acute é"), NULL, insert_char_cb, 37, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/A-I/e circumflex ê"), NULL, insert_char_cb, 38, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/A-I/e diaeresis ë"), NULL, insert_char_cb, 39, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/A-I/i grave ì"), NULL, insert_char_cb, 40, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/A-I/i acute í"), NULL, insert_char_cb, 41, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/A-I/i circumflex î"), NULL, insert_char_cb, 42, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/A-I/i diaeresis ï"), NULL, insert_char_cb, 43, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/_J-Z"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/Special/Accent/Lowercase/J-Z/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/Special/Accent/Lowercase/J-Z/n tilde ñ"), NULL, insert_char_cb, 44, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/J-Z/o grave ò"), NULL, insert_char_cb, 45, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/J-Z/o acute ó"), NULL, insert_char_cb,46, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/J-Z/oe ligature œ"), NULL, insert_char_cb,99, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/J-Z/o circumflex ô"), NULL, insert_char_cb, 47, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/J-Z/o tilde õ"), NULL, insert_char_cb, 48, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/J-Z/o diaeresis ö"), NULL, insert_char_cb, 49, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/J-Z/o slash ø"), NULL, insert_char_cb, 50, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/J-Z/u grave ù"), NULL, insert_char_cb, 51, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/J-Z/u acute ú"), NULL, insert_char_cb, 52, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/J-Z/u circumflex û"), NULL, insert_char_cb, 53, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/J-Z/u diaeresis ü"), NULL, insert_char_cb, 54, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/J-Z/y acute ý"), NULL, insert_char_cb, 55, "<Item>"},
		{N_("/Tags/Special/Accent/Lowercase/J-Z/y diaeresis ÿ"), NULL, insert_char_cb, 56, "<Item>"},
		{"/Tags/Special/Accent/sep1", NULL, NULL, 0, "<Separator>"},
		{N_("/Tags/Special/Accent/U_mlaut ¨"), NULL, insert_char_cb, 57, "<Item>"},
		{N_("/Tags/Special/Accent/_Acute ´"), NULL, insert_char_cb, 58, "<Item>"},
		{N_("/Tags/Special/Accent/_Cedilla ¸"), NULL, insert_char_cb, 59, "<Item>"},
		{N_("/Tags/Special/_Currency"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/Special/Currency/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/Special/Currency/_Cent sign ¢"), NULL, insert_char_cb, 60, "<Item>"},
		{N_("/Tags/Special/Currency/_Pound sterling £"), NULL, insert_char_cb, 61, "<Item>"},
		{N_("/Tags/Special/Currency/C_urrency sign ¤"), NULL, insert_char_cb, 62, "<Item>"},
		{N_("/Tags/Special/Currency/_Yen sign ¥"), NULL, insert_char_cb, 63, "<Item>"},
		{N_("/Tags/Special/Currency/_Euro "), NULL, insert_char_cb, 98, "<Item>"},
		{N_("/Tags/Special/_Math-Science"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/Special/Math-Science/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/Special/Math-Science/Logical _not sign ¬"), NULL, insert_char_cb, 64, "<Item>"},
		{N_("/Tags/Special/Math-Science/_Multiplication sign ×"), NULL, insert_char_cb, 65, "<Item>"},
		{N_("/Tags/Special/Math-Science/_Division sign ÷"), "<control><alt>slash", insert_char_cb, 66, "<Item>"},
		{N_("/Tags/Special/Math-Science/_Plus-minus sign ±"), NULL, insert_char_cb, 67, "<Item>"},
		{N_("/Tags/Special/Math-Science/_Less-than sign <"), "<control><alt>comma", insert_char_cb, 68, "<Item>"},
		{N_("/Tags/Special/Math-Science/_Greater-than sign >"), "<control><alt>period", insert_char_cb, 69, "<Item>"},
		{N_("/Tags/Special/Math-Science/Superscript _1 ¹"), NULL, insert_char_cb, 70, "<Item>"},
		{N_("/Tags/Special/Math-Science/Superscript _2 ²"), NULL, insert_char_cb, 71, "<Item>"},
		{N_("/Tags/Special/Math-Science/Superscript _3 ³"), NULL, insert_char_cb, 72, "<Item>"},
		{N_("/Tags/Special/Math-Science/One _quarter ¼"), NULL, insert_char_cb, 73, "<Item>"},
		{N_("/Tags/Special/Math-Science/One _half ½"), NULL, insert_char_cb, 74, "<Item>"},
		{N_("/Tags/Special/Math-Science/_Three quarters ¾"), NULL, insert_char_cb, 75, "<Item>"},
		{N_("/Tags/Special/Math-Science/Deg_ree sign °"), NULL, insert_char_cb, 76, "<Item>"},
		{N_("/Tags/Special/_Non Latin"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/Special/Non Latin/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/Special/Non Latin/_ETH (Icelandic) Ð"), NULL, insert_char_cb, 77, "<Item>"},
		{N_("/Tags/Special/Non Latin/_THORN (Icelandic) Þ"), NULL, insert_char_cb, 78, "<Item>"},
		{N_("/Tags/Special/Non Latin/et_h (Icelandic) ð"), NULL, insert_char_cb, 79, "<Item>"},
		{N_("/Tags/Special/Non Latin/th_orn ÿ"), NULL, insert_char_cb, 80, "<Item>"},
		{N_("/Tags/Special/Non Latin/Sharp _s ß"), NULL, insert_char_cb, 81, "<Item>"},
		{N_("/Tags/Special/Non Latin/_Micro sign µ"), NULL, insert_char_cb, 82, "<Item>"},
		{N_("/Tags/Special/_Other"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/Special/Other/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/Special/Other/_Non-breaking space"), NULL, insert_char_cb, 83, "<Item>"},
		{N_("/Tags/Special/Other/_Section sign §"), NULL, insert_char_cb, 84, "<Item>"},
		{N_("/Tags/Special/Other/_Copyright sign ©"), NULL, insert_char_cb, 85, "<Item>"},
		{N_("/Tags/Special/Other/_Left angle quotes «"), NULL, insert_char_cb, 86, "<Item>"},
		{N_("/Tags/Special/Other/_Right angle quotes »"), NULL, insert_char_cb, 87, "<Item>"},
		{N_("/Tags/Special/Other/Registered _trademark ®"), NULL, insert_char_cb, 88, "<Item>"},
		{N_("/Tags/Special/Other/Inverted _exclamation ¡"), NULL, insert_char_cb, 89, "<Item>"},
		{N_("/Tags/Special/Other/Inverted _question mark ¿"), NULL, insert_char_cb, 90, "<Item>"},
		{N_("/Tags/Special/Other/_Feminine ordinal ª"), NULL, insert_char_cb, 91, "<Item>"},
		{N_("/Tags/Special/Other/_Masculine ordinal º"), NULL, insert_char_cb, 92, "<Item>"},
		{N_("/Tags/Special/Other/_Pilcrow (paragraph sign) ¶"), NULL, insert_char_cb, 93, "<Item>"},
		{N_("/Tags/Special/Other/_Broken bar ¦"), NULL, insert_char_cb, 94, "<Item>"},
		{N_("/Tags/Special/Other/Soft _hypen -"), NULL, insert_char_cb, 95, "<Item>"},
		{N_("/Tags/Special/Other/Spa_cing macron ¯"), NULL, insert_char_cb, 96, "<Item>"},
		{N_("/Tags/Special/Other/Middle _dot ·"), NULL, insert_char_cb, 97, "<Item>"},
		{N_("/Tags/_Format by layout"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/Format by layout/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/Format by layout/_Bold"), "<control><alt>b", general_html_menu_cb, 1, "<ImageItem>",pixmap_bold},
		{N_("/Tags/Format by layout/_Italic"), "<control><alt>i", general_html_menu_cb, 2, "<ImageItem>",pixmap_italic},
		{N_("/Tags/Format by layout/_Underline"), "<control><alt>u", general_html_menu_cb, 3, "<ImageItem>",pixmap_underline},
		{N_("/Tags/Format by layout/_Strikeout"), "<control><alt>s", general_html_menu_cb, 4, "<ImageItem>",pixmap_strikeout},
		{N_("/Tags/Format by layout/Sm_all"), NULL, general_html_menu_cb, 56, "<Item>"},
		{N_("/Tags/Format by layout/Bi_g"), NULL, general_html_menu_cb, 57, "<Item>"},
		{N_("/Tags/F_ormat by context"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/Format by context/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/Format by context/_Strong"), "<control><alt>g", general_html_menu_cb, 16, "<ImageItem>",pixmap_bold},
		{N_("/Tags/Format by context/_Emphasis"), "<control><alt>e", general_html_menu_cb, 17, "<ImageItem>",pixmap_italic},
		{N_("/Tags/Format by context/_Define"), NULL, general_html_menu_cb, 48, "<ImageItem>",pixmap_dfn},
		{N_("/Tags/Format by context/_Code"), NULL, general_html_menu_cb, 49, "<ImageItem>",pixmap_code},
		{N_("/Tags/Format by context/Sa_mple"), NULL, general_html_menu_cb, 50, "<ImageItem>",pixmap_samp},
		{N_("/Tags/Format by context/_Keyboard"), NULL, general_html_menu_cb, 51, "<ImageItem>",pixmap_kbd},
		{N_("/Tags/Format by context/_Variable"), NULL, general_html_menu_cb, 52, "<ImageItem>",pixmap_var},
		{N_("/Tags/Format by context/Ci_tation"), NULL, general_html_menu_cb, 53, "<ImageItem>",pixmap_cite},
		{N_("/Tags/Format by context/_Abbreviation"), NULL, general_html_menu_cb, 54, "<ImageItem>",pixmap_abbr},
		{N_("/Tags/Format by context/Ac_ronym"), NULL, general_html_menu_cb, 55, "<ImageItem>",pixmap_acronym},
		{N_("/Tags/Format _general"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/Format general/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/Format general/_Paragraph"), "<control><alt>p", general_html_menu_cb, 5, "<ImageItem>",pixmap_paragraph},
		{N_("/Tags/Format general/_Break"),  "<control><alt>k", general_html_menu_cb, 6, "<ImageItem>",pixmap_brbreak},
		{N_("/Tags/Format general/Break clear _all"), NULL, general_html_menu_cb, 41, "<ImageItem>",pixmap_breakall},
		{N_("/Tags/Format general/_Non-Breaking Space"), "<control><alt>n", general_html_menu_cb, 7, "<ImageItem>",pixmap_nbsp},
		{"/Tags/Format general/sep1", NULL, NULL, 0, "<Separator>"},
		{N_("/Tags/Format general/Font Si_ze +1"), "<control><alt>equal", general_html_menu_cb, 11, "<ImageItem>",pixmap_fontp1},
		{N_("/Tags/Format general/Font _Size -1"), "<control><alt>minus", general_html_menu_cb, 12, "<ImageItem>",pixmap_fontm1},
		{"/Tags/Format general/sep2", NULL, NULL, 0, "<Separator>"},
		{N_("/Tags/Format general/Preformatted _Text"), "<Control><Alt>f", general_html_menu_cb, 13, "<ImageItem>",pixmap_fontpre},
		{N_("/Tags/Format general/Su_bscript"), NULL, general_html_menu_cb, 14, "<ImageItem>",pixmap_fontsub},
		{N_("/Tags/Format general/Su_perscript"), NULL, general_html_menu_cb, 15, "<ImageItem>",pixmap_fontsuper},
		{"/Tags/Format general/sep3", NULL, NULL, 0, "<Separator>"},
		{N_("/Tags/Format general/_Center"), NULL, general_html_menu_cb, 8, "<ImageItem>",pixmap_center},
		{N_("/Tags/Format general/Align _right"), "<control><Alt>r", general_html_menu_cb, 9, "<ImageItem>",pixmap_right},
		{N_("/Tags/_Table"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/Table/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/Table/_Table"), "<control><alt>t", general_html_menu_cb, 24, "<ImageItem>", pixmap_table2},
		{N_("/Tags/Table/Table _Row"), NULL, general_html_menu_cb, 25, "<ImageItem>", pixmap_table_tr2},
		{N_("/Tags/Table/Table _Header"), NULL, general_html_menu_cb, 26, "<ImageItem>", pixmap_table_th2},
		{N_("/Tags/Table/Table _Data"), NULL, general_html_menu_cb, 27, "<ImageItem>", pixmap_table_td2},
		{N_("/Tags/Table/Table _Caption"), NULL, general_html_menu_cb, 28, "<ImageItem>", pixmap_table_capt},
		{N_("/Tags/_List"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/List/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/List/Unordered _List"), "<control><alt>L", general_html_menu_cb, 33, "<ImageItem>", pixmap_list_ul},
		{N_("/Tags/List/_Ordered List"), "<control><alt>O", general_html_menu_cb, 34, "<ImageItem>", pixmap_list_ol},
		{N_("/Tags/List/List Ite_m"), "<control><alt>M", general_html_menu_cb, 35, "<ImageItem>", pixmap_list_li},
		{N_("/Tags/List/De_finition List"), NULL, general_html_menu_cb, 36, "<ImageItem>", pixmap_list_dl},
		{N_("/Tags/List/Definition _Term"), NULL, general_html_menu_cb, 37, "<ImageItem>", pixmap_list_dt},
		{N_("/Tags/List/_Definition"), NULL, general_html_menu_cb, 38, "<ImageItem>", pixmap_list_dd},
		{N_("/Tags/List/Men_u"), NULL, general_html_menu_cb, 39, "<Item>"},
		{N_("/Tags/Fo_rm"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/Form/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/Form/_Form"), NULL, general_html_menu_cb, 67, "<ImageItem>", pixmap_form},
		{N_("/Tags/Form/_Button"), NULL, general_html_menu_cb, 58, "<Item>"},
		{N_("/Tags/Form/_Input"), NULL, general_html_menu_cb, 65, "<Item>"},
		{N_("/Tags/Form/_Textarea"), NULL, general_html_menu_cb, 66, "<Item>"},
		{N_("/Tags/_Misc"), NULL, NULL, 0, "<Branch>"},
		{"/Tags/Misc/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Tags/Misc/Insert Generator _META-Tag"), NULL, general_html_menu_cb, 47, "<Item>"},
		{N_("/Tags/Misc/_Span"), NULL, general_html_menu_cb, 60, "<Item>"},
		{N_("/Tags/Misc/_Div"), NULL, general_html_menu_cb, 62, "<Item>"},
		{N_("/Tags/Misc/_Link"), NULL, general_html_menu_cb, 61, "<Item>"},
		{N_("/Tags/Misc/S_cript"), NULL, general_html_menu_cb, 59, "<Item>"},
		{N_("/Tags/Misc/_Anchor"), NULL, general_html_menu_cb, 63, "<ImageItem>", pixmap_anchor},
		{N_("/Tags/Misc/_Image"), NULL, general_html_menu_cb, 64, "<ImageItem>", pixmap_image},
		{N_("/Tags/_Comment"), "<control><alt>C", general_html_menu_cb, 10, "<ImageItem>", pixmap_comment},
		{N_("/Dial_ogs"), NULL, NULL, 0, "<Branch>"},
		{"/Dialogs/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Dialogs/_General"), NULL, NULL, 0, "<Branch>"},
		{"/Dialogs/General/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Dialogs/General/_Quickstart..."), "<shift><alt>q", menu_html_dialogs_lcb, 32, "<ImageItem>", pixmap_quick_start},
		{N_("/Dialogs/General/_Body..."), "<shift><alt>B", menu_html_dialogs_lcb, 1, "<ImageItem>", pixmap_body},
		{N_("/Dialogs/General/_Anchor..."), "<shift><alt>a", menu_html_dialogs_lcb, 2, "<ImageItem>", pixmap_anchor},
		{N_("/Dialogs/General/_Email..."), "<shift><alt>e", menu_html_dialogs_lcb, 3, "<Item>"},
		{N_("/Dialogs/General/_Rule..."), "<shift><alt>r", menu_html_dialogs_lcb, 4, "<ImageItem>", pixmap_hrule},
		{N_("/Dialogs/General/_Font..."), "<shift><alt>f", menu_html_dialogs_lcb, 5, "<Item>"},
		{N_("/Dialogs/General/Basef_ont..."), NULL, menu_html_dialogs_lcb, 6, "<Item>"},
		{N_("/Dialogs/General/Quick_list..."), "<shift><alt>L", menu_html_dialogs_lcb, 7, "<ImageItem>", pixmap_list},
		{N_("/Dialogs/General/_Meta..."), "<shift><alt>m", menu_html_dialogs_lcb, 8, "<Item>"},
		{N_("/Dialogs/General/Embe_d..."), NULL, menu_html_dialogs_lcb, 9, "<Item>"},
		{N_("/Dialogs/General/Select _Color..."), NULL, menu_html_dialogs_lcb, 37, "<Item>"},
		{N_("/Dialogs/General/Insert _Time..."), NULL, menu_html_dialogs_lcb, 34, "<Item>"},
		{N_("/Dialogs/General/Insert _Image..."), "<shift><alt>I", menu_html_dialogs_lcb, 10, "<ImageItem>", pixmap_image},
		{N_("/Dialogs/General/Insert T_humbnail..."), "<shift><alt>N", menu_html_dialogs_lcb, 11, "<ImageItem>", pixmap_thumbnail},
		{N_("/Dialogs/_Table"), NULL, NULL, 0, "<Branch>"},
		{"/Dialogs/Table/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Dialogs/Table/Table _Wizard..."), NULL, menu_html_dialogs_lcb, 12, "<ImageItem>", pixmap_tablewhiz},
		{N_("/Dialogs/Table/_Table..."), "<shift><alt>T", menu_html_dialogs_lcb, 13, "<ImageItem>", pixmap_table},
		{N_("/Dialogs/Table/Table _Row..."), NULL, menu_html_dialogs_lcb, 14, "<ImageItem>", pixmap_table_tr},
		{N_("/Dialogs/Table/Table _Head..."), NULL, menu_html_dialogs_lcb, 15, "<ImageItem>", pixmap_table_th},
		{N_("/Dialogs/Table/Table _Data..."), NULL, menu_html_dialogs_lcb, 16, "<ImageItem>", pixmap_table_td},
		{N_("/Dialogs/_CSS"), NULL, NULL, 0, "<Branch>"},
		{"/Dialogs/CSS/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Dialogs/CSS/_Create Style..."), "<shift><alt>S", menu_html_dialogs_lcb, 36, "<ImageItem>",pixmap_cssnewstyle},
		{N_("/Dialogs/CSS/S_pan..."), NULL, menu_html_dialogs_lcb, 17, "<ImageItem>",pixmap_cssspan},
		{N_("/Dialogs/CSS/_Div..."), "<shift><alt>D", menu_html_dialogs_lcb, 18, "<ImageItem>",pixmap_cssdiv},
		{N_("/Dialogs/CSS/_Style..."), NULL, general_html_menu_cb, 42, "<ImageItem>",pixmap_cssstyle},
		{N_("/Dialogs/CSS/_Link to Stylesheet..."), NULL, menu_html_dialogs_lcb, 35, "<Item>"},
		{N_("/Dialogs/_Frame"), NULL, NULL, 0, "<Branch>"},
		{"/Dialogs/Frame/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Dialogs/Frame/Frame _Wizard..."), NULL, menu_html_dialogs_lcb, 19, "<ImageItem>", pixmap_framewhiz},
		{N_("/Dialogs/Frame/Frame_set..."), NULL, menu_html_dialogs_lcb, 20, "<ImageItem>", pixmap_frameset},
		{N_("/Dialogs/Frame/_Frame..."), NULL, menu_html_dialogs_lcb, 21, "<ImageItem>", pixmap_frame},
		{N_("/Dialogs/F_orm"), NULL, NULL, 0, "<Branch>"},
		{"/Dialogs/Form/tearoff1", NULL, NULL, 0, "<Tearoff>"},
		{N_("/Dialogs/Form/F_orm..."), NULL, menu_html_dialogs_lcb, 22, "<ImageItem>", pixmap_form},
		{N_("/Dialogs/Form/_Input..."), NULL, menu_html_dialogs_lcb, 33, "<Item>"},
		{N_("/Dialogs/Form/Input Buttons..."), NULL, menu_html_dialogs_lcb, 23, "<ImageItem>", pixmap_form_submit},
		{N_("/Dialogs/Form/Input Text..."), NULL, menu_html_dialogs_lcb, 24, "<ImageItem>", pixmap_form_text},
		{N_("/Dialogs/Form/Text_area..."), NULL, menu_html_dialogs_lcb, 26, "<ImageItem>", pixmap_form_textarea},
		{N_("/Dialogs/Form/_Select..."), NULL, menu_html_dialogs_lcb, 29, "<ImageItem>", pixmap_form_select},
		{N_("/Dialogs/Form/O_ption..."), NULL, menu_html_dialogs_lcb, 30, "<ImageItem>", pixmap_form_option},
		{N_("/Dialogs/Form/Option _Group..."), NULL, menu_html_dialogs_lcb, 31, "<ImageItem>",pixmap_form_optiongroup },
		{N_("/Dialogs/Form/_Button..."), NULL, menu_html_dialogs_lcb, 25, "<Item>"},
		{"/Dialogs/sep1", NULL, NULL, 0, "<Separator>"},
		{N_("/Dialogs/_Edit tag under cursor..."), "F3", menu_html_dialogs_lcb, 38, "<ImageItem>", pixmap_edit_tag}
	};
	static GtkItemFactoryEntry menu_items1[] = {
		{N_("/View/View _HTML Toolbar"), NULL, htmlbar_view_lcb, 0, "<ToggleItem>"}
	};
	ifactory = gtk_item_factory_from_widget(bfwin->menubar);
#ifdef ENABLE_NLS
	gtk_item_factory_set_translate_func(ifactory, htmlbar_menu_translate, "<bluefishmain>", NULL);
#endif
	gtk_item_factory_create_items(ifactory, sizeof(menu_items) / sizeof(menu_items[0]), menu_items, bfwin);
	gtk_item_factory_create_items(ifactory, sizeof(menu_items1) / sizeof(menu_items1[0]), menu_items1, hbw);

	hbs = g_hash_table_lookup(htmlbar_v.lookup,bfwin->session);
	if (hbs) {
		setup_toggle_item(ifactory, "/View/View HTML Toolbar", hbs->view_htmlbar);
	} else {
		DEBUG_MSG("htmlbar_build_menu, ERROR, no htmlbarsession in hasht %p for session %p!!?!?!?!?\n",htmlbar_v.lookup,bfwin->session);
	}
	
	gtk_widget_show_all(bfwin->menubar);
}

typedef struct {
	const gchar *ident;
	void (*func)();
	const guint8 *data;
	const gchar *tooltiptext;
} Ttoolbaritem;

typedef struct {
	Ttoolbaritem *tbitem;
	GtkWidget *button;
	Tbfwin *bfwin;
}Tquickbaritem;

static void bold_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 1, NULL);
}
static void italic_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 2, NULL);
}
static void paragraph_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 5, NULL);
}
static void break_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 6, NULL);
}
static void breakclear_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 41, NULL);
}
static void nbsp_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 7, NULL);
}
static void center_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 8, NULL);
}
static void rightjustify_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 9, NULL);
}
static void comment_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 10, NULL);
}
static void fontsizeplus1_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 11, NULL);
}
static void fontsizeminus1_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 12, NULL);
}
static void preformatted_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 13, NULL);
}
static void subscript_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 14, NULL);
}
static void superscript_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 15, NULL);
}
static void strong_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 16, NULL);
}
static void emphasis_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 17, NULL);
}
static void heading1_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 18, NULL);
}
static void heading2_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 19, NULL);
}
static void heading3_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 20, NULL);
}
static void heading4_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 21, NULL);
}
static void heading5_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 22, NULL);
}
static void heading6_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 23, NULL);
}
static void table_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 24, NULL);
}
static void tablerow_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 25, NULL);
}
static void tableheader_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 26, NULL);
}
static void tabledata_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 27, NULL);
}
static void tablecaption_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 28, NULL);
}
static void frameset_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 29, NULL);
}
static void frame_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 30, NULL);
}
static void noframes_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 31, NULL);
}
static void target_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 32, NULL);
}
static void unorderedlist_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 33, NULL);
}
static void orderedlist_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 34, NULL);
}
static void listitem_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 35, NULL);
}
static void definitionlist_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 36, NULL);
}
static void definitionterm_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 37, NULL);
}
static void definition_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 38, NULL);
}
static void style_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 42, NULL);
}
static void dfn_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 48, NULL);
}
static void code_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 49, NULL);
}
static void samp_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 50, NULL);
}
static void kbd_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 51, NULL);
}
static void var_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 52, NULL);
}
static void cite_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 53, NULL);
}
static void abbr_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 54, NULL);
}
static void acronym_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	general_html_menu_cb(bfwin, 55, NULL);
}

static void quickstart_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	quickstart_dialog_new(bfwin);
}
static void body_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	body_dialog(bfwin,NULL);
}
static void quickanchor_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	DEBUG_MSG("quickanchor_clicked_lcb, calling quickanchor_dialog with bfwin=%p and data=NULL\n",bfwin);
	quickanchor_dialog(bfwin, NULL);
}
static void quickrule_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	quickrule_dialog(bfwin,NULL);
}
static void email_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	email_dialog(bfwin,NULL);
}
static void fontdialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	font_dialog(bfwin,NULL);
}
static void basefont_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	basefont_dialog(bfwin,NULL);
}
static void framesetdialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	frameset_dialog(bfwin,NULL);
}
static void framedialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	frame_dialog(bfwin,NULL);
}
static void quicklist_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	quicklist_dialog(bfwin,NULL);
}
static void link_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	linkdialog_dialog(bfwin,NULL);
}
static void formdialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	formdialog_dialog(bfwin,NULL);
}
static void inputbuttondialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	inputdialog_dialog(bfwin, NULL, "submit");
}
static void textdialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	inputdialog_dialog(bfwin, NULL, "text");
}
static void hiddendialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	inputdialog_dialog(bfwin, NULL, "hidden");
}
static void textareadialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	textareadialog_dialog(bfwin,NULL);
}
static void radiodialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	inputdialog_dialog(bfwin, NULL, "radio");
}
static void checkdialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	inputdialog_dialog(bfwin, NULL, "checkbox");
}
static void selectdialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	selectdialog_dialog(bfwin,NULL);
}
static void optiondialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	optiondialog_dialog(bfwin,NULL);
}
static void optgroupdialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	optgroupdialog_dialog(bfwin,NULL);
}
static void buttondialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	buttondialog_dialog(bfwin,NULL);
}
static void image_insert_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	image_insert_dialog(bfwin,NULL);
}
static void thumbnail_insert_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	thumbnail_insert_dialog(bfwin);
}
static void multi_thumbnail_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	multi_thumbnail_dialog(bfwin);
}
static void tabledialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	tabledialog_dialog(bfwin,NULL);
}
static void tablerowdialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	tablerowdialog_dialog(bfwin,NULL);
}
static void tableheaddialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	tableheaddialog_dialog(bfwin,NULL);
}
static void tabledatadialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	tabledatadialog_dialog(bfwin,NULL);
}
static void tablewizard_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	tablewizard_dialog(bfwin);
}
static void framewizard_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	framewizard_dialog(bfwin);
}
static void span_dialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	span_dialog(bfwin,NULL);
}
static void div_dialog_clicked_lcb(GtkWidget *widget, Tbfwin *bfwin) {
	div_dialog(bfwin,NULL);
}


static Ttoolbaritem tbi[] = {
	{"quickstart...", quickstart_clicked_lcb,pixmap_quick_start , N_("QuickStart...")},
	{"body...", body_clicked_lcb, pixmap_body, N_("Body...")},
	{"bold", bold_clicked_lcb,  pixmap_bold, N_("Bold")},
	{"italic", italic_clicked_lcb,  pixmap_italic, N_("Italic")},
	{"paragraph", paragraph_clicked_lcb,  pixmap_paragraph, N_("Paragraph")},
	{"break", break_clicked_lcb, pixmap_brbreak, N_("Break")},
	{"breakclear", breakclear_clicked_lcb, pixmap_breakall, N_("Break and clear")},
	{"nbsp", nbsp_clicked_lcb, pixmap_nbsp, N_("Non-breaking space")},
	{"", NULL, 0, NULL}, /* spacing */
	{"anchor...", quickanchor_clicked_lcb,pixmap_anchor , N_("Anchor...")},
	{"rule...", quickrule_clicked_lcb, pixmap_hrule, N_("Rule...")},
	{"center", center_clicked_lcb,  pixmap_center, N_("Center")},
	{"rightjustify", rightjustify_clicked_lcb, pixmap_right, N_("Right Justify")},
	{"comment", comment_clicked_lcb, pixmap_comment , N_("Comment")},
	{"email...", email_clicked_lcb, pixmap_email, N_("E-mail...")},
	{"font...",fontdialog_clicked_lcb , pixmap_font, N_("Font...")},
	{"basefont...", basefont_clicked_lcb, pixmap_font_base, N_("Base Font Size...")},
	{"", NULL, 0, NULL}, /* spacing */
	{"fontsize+1", fontsizeplus1_clicked_lcb, pixmap_fontp1 , N_("Font Size +1")},
	{"fontsize-1", fontsizeminus1_clicked_lcb,  pixmap_fontm1, N_("Font Size -1")},
	{"preformatted", preformatted_clicked_lcb,  pixmap_fontpre, N_("Preformatted Text")},
	{"subscript", subscript_clicked_lcb, pixmap_fontsub, N_("Subscript")},
	{"superscript", superscript_clicked_lcb,  pixmap_fontsuper, N_("Superscript")},
	{"strong", strong_clicked_lcb,  pixmap_bold, N_("Strong")},
	{"emphasis", emphasis_clicked_lcb,  pixmap_italic, N_("Emphasis")},
	{"", NULL, 0, NULL}, /* spacing */
	{"heading1", heading1_clicked_lcb,  pixmap_fonth1, N_("Heading 1")},
	{"heading2", heading2_clicked_lcb,  pixmap_fonth2, N_("Heading 2")},
	{"heading3", heading3_clicked_lcb,  pixmap_fonth3, N_("Heading 3")},
	{"heading4", heading4_clicked_lcb,  pixmap_fonth4, N_("Heading 4")},
	{"heading5", heading5_clicked_lcb,  pixmap_fonth5, N_("Heading 5")},
	{"heading6", heading6_clicked_lcb,  pixmap_fonth6, N_("Heading 6")},
	{"tablewizard...",tablewizard_clicked_lcb, pixmap_tablewhiz, N_("Table Wizard...")},
	{"", NULL, 0, NULL}, /* spacing */
	{"table...",tabledialog_clicked_lcb, pixmap_table, N_("Table...")},
	{"tablerow...", tablerowdialog_clicked_lcb, pixmap_table_tr, N_("Table Row...")},
	{"tableheader...", tableheaddialog_clicked_lcb, pixmap_table_th, N_("Table Header...")},
	{"tabledata...", tabledatadialog_clicked_lcb, pixmap_table_td, N_("Table Data...")},
	{"", NULL, 0, NULL}, /* spacing */
	{"table", table_clicked_lcb,  pixmap_table2, N_("Table")},
	{"tablerow", tablerow_clicked_lcb,  pixmap_table_tr2, N_("Table Row")},
	{"tableheader", tableheader_clicked_lcb,  pixmap_table_th2, N_("Table Header")},
	{"tabledata", tabledata_clicked_lcb,  pixmap_table_td2, N_("Table Data")},
	{"tablecaption", tablecaption_clicked_lcb,  pixmap_table_capt, N_("Table Caption")},
	{"framewizard...",framewizard_clicked_lcb , pixmap_framewhiz, N_("Frame Wizard...")},
	{"", NULL, 0, NULL}, /* spacing */
	{"frameset...", framesetdialog_clicked_lcb,pixmap_frameset , N_("Frameset...")},
	{"frame...",framedialog_clicked_lcb , pixmap_frame, N_("Frame...")},
	{"", NULL, 0, NULL}, /* spacing */
	{"frameset", frameset_clicked_lcb, pixmap_frameset2 , N_("Frameset")},
	{"frame", frame_clicked_lcb,  pixmap_frame2, N_("Frame")},
	{"noframes", noframes_clicked_lcb,  pixmap_frame_no, N_("Noframes")},
	{"target", target_clicked_lcb,  pixmap_frame_base, N_("Target")},
	{"form...",formdialog_clicked_lcb , pixmap_form, N_("Form...")},
	{"inputbutton...", inputbuttondialog_clicked_lcb, pixmap_form_submit, N_("Input Button...")},
	{"text...",textdialog_clicked_lcb , pixmap_form_text, N_("Input Text...")},
	{"hidden...", hiddendialog_clicked_lcb, pixmap_form_hidden, N_("Input Hidden...")},
	{"textarea...",textareadialog_clicked_lcb , pixmap_form_textarea, N_("Textarea...")},
	{"radiobutton...", radiodialog_clicked_lcb, pixmap_form_radio, N_("Input Radio Button...")},
	{"checkbox...", checkdialog_clicked_lcb, pixmap_form_check, N_("Input Check Box...")},
	{"select...", selectdialog_clicked_lcb, pixmap_form_select, N_("Select...")},
	{"option...",optiondialog_clicked_lcb , pixmap_form_option, N_("Option...")},
	{"optiongroup...", optgroupdialog_clicked_lcb, pixmap_form_optiongroup, N_("Option group...")},
	{"button...", buttondialog_clicked_lcb, pixmap_form_submit, N_("Button...")},
	{"quicklist...", quicklist_clicked_lcb, pixmap_list, N_("Quick List...")},
	{"unorderedlist", unorderedlist_clicked_lcb,  pixmap_list_ul, N_("Unordered List")},
	{"orderedlist", orderedlist_clicked_lcb,  pixmap_list_ol, N_("Ordered List")},
	{"listitem", listitem_clicked_lcb,  pixmap_list_li, N_("List Item")},
	{"definitionlist", definitionlist_clicked_lcb,  pixmap_list_dl, N_("Definition List")},
	{"definitionterm", definitionterm_clicked_lcb,  pixmap_list_dt, N_("Definition Term")},
	{"definition", definition_clicked_lcb,  pixmap_list_dd, N_("Definition")},
	{"createstylesheet...", new_css_dialog, pixmap_cssnewstyle, N_("Create stylesheet...")},
	{"span...", span_dialog_clicked_lcb,pixmap_cssspan , N_("Span...")},
	{"div...",div_dialog_clicked_lcb,pixmap_cssdiv , N_("Div...")},
	{"style", style_clicked_lcb, pixmap_cssstyle, N_("Style")},
	{"linkstylesheet...", link_clicked_lcb,pixmap_linkstylesheet , N_("Link to stylesheet...")},
	{"image...", image_insert_clicked_lcb, pixmap_image, N_("Insert image...")},
	{"thumbnail...", thumbnail_insert_clicked_lcb, pixmap_thumbnail, N_("Insert thumbnail...")},
	{"mthumbnail...", multi_thumbnail_clicked_lcb, pixmap_multithumbs, N_("Multi thumbnail...")},
	{"dfn", dfn_clicked_lcb,  pixmap_dfn, N_("Definition")},
	{"code", code_clicked_lcb, pixmap_code, N_("Code")},
	{"samp", samp_clicked_lcb,  pixmap_samp, N_("Sample")},
	{"kbd", kbd_clicked_lcb,  pixmap_kbd, N_("Keyboard")},
	{"var", var_clicked_lcb,  pixmap_var, N_("Variable")},
	{"cite", cite_clicked_lcb,  pixmap_cite, N_("Citation")},
	{"abbr", abbr_clicked_lcb,  pixmap_abbr, N_("Abbreviation")},
	{"acronym", acronym_clicked_lcb, pixmap_acronym, N_("Acronym")},
	{"", NULL, 0, NULL} /* spacing */
};

static void html_toolbar_remove_from_quickbar_lcb(GtkMenuItem *menuitem, Ttoolbaritem *tbitem) {
	GList *bfwlist;
	htmlbar_v.quickbar_items = remove_from_stringlist(htmlbar_v.quickbar_items, tbitem->ident);
	bfwlist = g_list_first(main_v->bfwinlist);
	while (bfwlist) {
		Tquickbaritem *qbi=NULL;
		Tbfwin *bfwin = BFWIN(bfwlist->data);
		GList *tmplist  = g_list_first(bfwin->toolbar_quickbar_children);
		while (tmplist) {
			qbi = tmplist->data;
			if (qbi->tbitem == tbitem) {
				break;
			}
			tmplist = g_list_next(tmplist);
		}
		if (qbi) {
			gtk_widget_hide(qbi->button);
			gtk_widget_destroy(qbi->button);
			g_free(qbi);
		}
		bfwlist = g_list_next(bfwlist);
	}
}

/* return the position in the quickbar GList of a tool bar item */
gint get_quickbar_item_position(Ttoolbaritem *tbitem) {
	GList *tmplist, *winlist = g_list_first(main_v->bfwinlist);
	gint pos = 0;
	tmplist  = g_list_first(BFWIN(winlist->data)->toolbar_quickbar_children);

	while (tmplist) {
		Tquickbaritem *qbi;
		qbi = tmplist->data;
		if (qbi->tbitem == tbitem) {
			DEBUG_MSG("get_quickbar_item_position, quickbar item is at %d\n", pos);
			return pos;
		}
		pos += 1;
		tmplist = g_list_next(tmplist);
	}
	DEBUG_MSG("get_quickbar_item_position, not found, return -1!\n");
	return -1;
}

static gboolean html_toolbar_quickbar_item_button_press_lcb(GtkWidget *widget,GdkEventButton *bevent,Ttoolbaritem *tbitem);

static void html_toolbar_quickbar_switch(Ttoolbaritem *tbitem, gboolean moveright) {
	gint pos;

	/* first we look for the current location of this item */	
	pos = get_quickbar_item_position(tbitem);
	DEBUG_MSG("html_toolbar_quickbar_switch, found item %s at pos=%d, config list length=%d\n",tbitem->ident,pos,g_list_length(htmlbar_v.quickbar_items));
	if (pos >= 0) {
		GList *tmp1, *tmp2, *tmplist;
		/* then we move this item to the new place in the config list */
		tmp1 = g_list_nth(htmlbar_v.quickbar_items, pos);
		if (!tmp1) {
			DEBUG_MSG("html_toolbar_quickbar_switch, nothing found in config list at pos=%d, RETURN\n",pos);
			return;
		}
		tmp2 = (moveright) ? g_list_next(tmp1) : g_list_previous(tmp1);
		if (!tmp2) {
			DEBUG_MSG("html_toolbar_quickbar_switch, no tmp2 found, moveright=%d, list length=%d RETURN\n",moveright,g_list_length(htmlbar_v.quickbar_items));
			return;
		}
		list_switch_order(tmp1, tmp2);
		DEBUG_MSG("html_toolbar_quickbar_switch, after list_switch_order, list length=%d\n",g_list_length(htmlbar_v.quickbar_items));
		
		/* now switch the widgets in every window that has a quickbar */
		tmplist = g_list_first(main_v->bfwinlist);
		while (tmplist) {
			Tbfwin *bfwin = BFWIN(tmplist->data);
			DEBUG_MSG("html_toolbar_quickbar_switch, checking bfwin=%p\n",bfwin);
			if (bfwin->toolbar_quickbar && bfwin->toolbar_quickbar_children) {
				Tquickbaritem *qbi1;
				DEBUG_MSG("retrieving quickbaritem from pos %d\n",pos);
				tmp1 = g_list_nth(bfwin->toolbar_quickbar_children, pos);
				tmp2 = (moveright) ? g_list_next(tmp1) : g_list_previous(tmp1);
				qbi1 = (Tquickbaritem *)tmp1->data;
				list_switch_order(tmp1, tmp2);
				DEBUG_MSG("html_toolbar_quickbar_switch, quickbaritem %p has tbitem %s\n",qbi1, qbi1->tbitem->ident);
				/* I don't know why the next bit of code doesn't work... bug in gtk? */
				/*g_object_ref(G_OBJECT(qbi1->button));
				DEBUG_MSG("html_toolbar_quickbar_switch, detaching widget!, button=%p, button->parent=%p, toolbar=%p\n",qbi1->button,qbi1->button->parent,bfwin->toolbar_quickbar);
				gtk_container_remove(GTK_CONTAINER(bfwin->toolbar_quickbar),qbi1->button);
				DEBUG_MSG("html_toolbar_quickbar_switch, attaching widget at pos %d\n",(moveright)?pos+1:pos-1);
				gtk_toolbar_insert_widget(GTK_TOOLBAR(bfwin->toolbar_quickbar),qbi1->button,
							_(qbi1->tbitem->tooltiptext),"",(moveright)?pos+1:pos-1);
				g_object_unref(G_OBJECT(qbi1->button));
				*/
				/* this workaround bit of code removes (and thus destroys) the button and creates a new identical button on the new location */
				DEBUG_MSG("html_toolbar_quickbar_switch, about to destroy button,(toolbar has %d children)\n",g_list_length(GTK_TOOLBAR(bfwin->toolbar_quickbar)->children));
				gtk_container_remove(GTK_CONTAINER(bfwin->toolbar_quickbar),qbi1->button);
				DEBUG_MSG("html_toolbar_quickbar_switch, inserting widget at pos %d (toolbar has %d children)\n",(moveright)?pos+1:pos-1, g_list_length(GTK_TOOLBAR(bfwin->toolbar_quickbar)->children));
				qbi1->button = gtk_toolbar_insert_item(GTK_TOOLBAR(bfwin->toolbar_quickbar), NULL, _(qbi1->tbitem->tooltiptext),
							"", htmlbar_pixmap(qbi1->tbitem->data), G_CALLBACK(qbi1->tbitem->func), bfwin, (moveright)?pos+1:pos-1);
				g_signal_connect(qbi1->button, "button-press-event", G_CALLBACK(html_toolbar_quickbar_item_button_press_lcb), tbitem);
				gtk_widget_show(qbi1->button);
			}
			tmplist = g_list_next(tmplist);
		}
		
	}
}

static void html_toolbar_quickbar_move_left_lcb(GtkMenuItem *menuitem, Ttoolbaritem *tbitem) {
	html_toolbar_quickbar_switch(tbitem, FALSE);
}

static void html_toolbar_quickbar_move_right_lcb(GtkMenuItem *menuitem, Ttoolbaritem *tbitem) {
	html_toolbar_quickbar_switch(tbitem, TRUE);
}

static gboolean html_toolbar_quickbar_item_button_press_lcb(GtkWidget *widget,GdkEventButton *bevent,Ttoolbaritem *tbitem) {
	if (bevent->button == 3) {
		GtkWidget *menu = gtk_menu_new ();
		GtkWidget *menuitem = gtk_menu_item_new_with_label(_("Remove from quickbar"));
		GtkWidget *itemleft = gtk_menu_item_new_with_label(_("Move Left"));
		GtkWidget *itemright = gtk_menu_item_new_with_label(_("Move Right"));
		GtkWidget *sep = gtk_separator_menu_item_new();
		DEBUG_MSG("popup for tbitem %p\n", tbitem);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(html_toolbar_remove_from_quickbar_lcb), tbitem);
		g_signal_connect(G_OBJECT(itemleft), "activate", G_CALLBACK(html_toolbar_quickbar_move_left_lcb), tbitem);
		g_signal_connect(G_OBJECT(itemright), "activate", G_CALLBACK(html_toolbar_quickbar_move_right_lcb), tbitem);
		gtk_menu_append(GTK_MENU(menu), itemleft);
		gtk_menu_append(GTK_MENU(menu), itemright);
		gtk_menu_append(GTK_MENU(menu), sep);
		gtk_menu_append(GTK_MENU(menu), menuitem);
		gtk_widget_show_all (menu);
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
			  NULL, widget, 0, gtk_get_current_event_time ());
		return TRUE;
	}
	return FALSE;
}

static void html_toolbar_add_to_quickbar_lcb(GtkMenuItem *menuitem, Ttoolbaritem *tbitem) {
	GList *tmplist = g_list_first(main_v->bfwinlist);
	DEBUG_MSG("adding tbitem %p to quickbars\n", tbitem);
	htmlbar_v.quickbar_items = add_to_stringlist(htmlbar_v.quickbar_items, tbitem->ident);
	while (tmplist) {
		Tquickbaritem *qbi;
		Tbfwin *bfwin = BFWIN(tmplist->data);
		qbi = g_new(Tquickbaritem,1);
		qbi->button = gtk_toolbar_append_item(GTK_TOOLBAR(bfwin->toolbar_quickbar), NULL, _(tbitem->tooltiptext),
							"", htmlbar_pixmap(tbitem->data), G_CALLBACK(tbitem->func), bfwin);
		g_signal_connect(qbi->button, "button-press-event", G_CALLBACK(html_toolbar_quickbar_item_button_press_lcb), tbitem);
		gtk_widget_show(qbi->button);
		qbi->tbitem = tbitem;
		bfwin->toolbar_quickbar_children = g_list_append(bfwin->toolbar_quickbar_children, qbi);
		tmplist = g_list_next(tmplist);
	}
}

static gboolean html_toolbar_item_button_press_lcb(GtkWidget *widget,GdkEventButton *bevent,Ttoolbaritem *tbitem) {
	DEBUG_MSG("html_toolbar_item_button_press_lcb, button=%d\n", bevent->button);
	if (bevent->button == 3) {
		GtkWidget *menu = gtk_menu_new ();
		GtkWidget *menuitem = gtk_menu_item_new_with_label(_("Add to quickbar"));
		DEBUG_MSG("popup for tbitem %p\n", tbitem);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(html_toolbar_add_to_quickbar_lcb), tbitem);
		gtk_menu_append(GTK_MENU(menu), menuitem);
		gtk_widget_show_all (menu);
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
			  NULL, widget, 0, gtk_get_current_event_time ());
		return TRUE;
	}
	return FALSE;
}

static void html_toolbar_add_items(Tbfwin *bfwin, GtkWidget *html_toolbar, Ttoolbaritem *tbi, gint from, gint to) {
	gint i;
	GtkWidget *item;
	for (i=from;i<=to;i++) {
		if (tbi[i].func == NULL) {
			gtk_toolbar_append_space(GTK_TOOLBAR(html_toolbar));
		} else {
			item = gtk_toolbar_append_item(GTK_TOOLBAR(html_toolbar), NULL, _(tbi[i].tooltiptext),
						NULL, htmlbar_pixmap(tbi[i].data), G_CALLBACK(tbi[i].func), bfwin);
			g_signal_connect(item, "button-press-event", G_CALLBACK(html_toolbar_item_button_press_lcb), &tbi[i]);
/*			DEBUG_MSG("adding tbitem %p to html_toolbar\n", &tbi[i]);*/
		}
	}
}

static void html_toolbar_add_items_to_submenu(Tbfwin *bfwin, GtkWidget *html_toolbar, Ttoolbaritem *tbi, gint from, gint to, gchar *menulabel, const guint8 *menupix) {
	GtkWidget *menu_bar, *sub_menu, *menu_item, *pixmap;
	GtkTooltips *tooltips;
	gint i;
	
	tooltips = gtk_tooltips_new();	
	sub_menu = gtk_menu_new();
	for (i=from;i<=to;i++) {
		if (tbi[i].func == NULL) {
			gtk_toolbar_append_space(GTK_TOOLBAR(html_toolbar));
		} else {
			menu_item = gtk_menu_item_new();
			gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), menu_item, _(tbi[i].tooltiptext), NULL);
			gtk_container_add(GTK_CONTAINER(menu_item), htmlbar_pixmap(tbi[i].data));
			g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(tbi[i].func), bfwin);
			g_signal_connect(menu_item, "button-press-event", G_CALLBACK(html_toolbar_item_button_press_lcb), &tbi[i]);
			gtk_menu_append(GTK_MENU(sub_menu), menu_item);
		}
	}
	gtk_object_sink(GTK_OBJECT(tooltips));
	/* BUG: should we decrement the reference count of 'tooltips' after the loop? 
	valgrind thinks we have a memory leak here */
	menu_bar = gtk_menu_bar_new();
	menu_item = gtk_menu_item_new();
	gtk_container_add(GTK_CONTAINER(menu_item),htmlbar_pixmap(menupix));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), sub_menu);
	gtk_menu_bar_append(GTK_MENU_BAR(menu_bar), menu_item);
	pixmap = htmlbar_pixmap(menupix);
	gtk_toolbar_append_element(GTK_TOOLBAR(html_toolbar),GTK_TOOLBAR_CHILD_WIDGET,menu_bar,NULL,menulabel,NULL,pixmap,NULL,NULL);
	gtk_object_sink(GTK_OBJECT(pixmap));
}

void htmlbar_toolbar(Thtmlbarwin *hbw) {
/*
 * every item with a ... in the tooltip brings up a dialog, every item
 * without the ... in the tooltip will directly insert html code
 *
 */
	GtkWidget *html_toolbar, *html_notebook;
	Tbfwin *bfwin = hbw->bfwin;

	DEBUG_MSG("htmlbar_toolbar, started for %p\n",hbw);
	html_notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(html_notebook), GTK_POS_TOP);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(html_notebook), TRUE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(html_notebook), TRUE);
	hbw->handlebox = gtk_handle_box_new();
	gtk_container_add(GTK_CONTAINER(hbw->handlebox), html_notebook);
	gtk_box_pack_start(GTK_BOX(bfwin->toolbarbox), hbw->handlebox, FALSE, FALSE, 0);

	bfwin->toolbar_quickbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(bfwin->toolbar_quickbar), GTK_TOOLBAR_ICONS);
	DEBUG_MSG("htmlbar_toolbar, creating quickbar, quickbar_items=%p\n",htmlbar_v.quickbar_items);
	{
		GList *tmplist;
		gint i, numitems=(sizeof(tbi)/sizeof(Ttoolbaritem));
		tmplist = g_list_first(htmlbar_v.quickbar_items);
		while (tmplist) {
			gchar *tmpstr = tmplist->data;
			DEBUG_MSG("htmlbar_toolbarr, searching for %s\n", tmpstr);
			for (i=0;i<numitems;i++) {
				if (strcmp(tbi[i].ident, tmpstr)==0) {
					Tquickbaritem *qbi = g_new(Tquickbaritem,1);
					qbi->button = gtk_toolbar_append_item(GTK_TOOLBAR(bfwin->toolbar_quickbar), NULL, _(tbi[i].tooltiptext),
						"", htmlbar_pixmap(tbi[i].data), G_CALLBACK(tbi[i].func), bfwin);
					g_signal_connect(qbi->button, "button-press-event", G_CALLBACK(html_toolbar_quickbar_item_button_press_lcb), &tbi[i]);
					qbi->tbitem = &tbi[i];
					bfwin->toolbar_quickbar_children = g_list_append(bfwin->toolbar_quickbar_children, qbi);
					DEBUG_MSG("htmlbar_toolbar, appended %s as quickbaritem %p to quickbar %p\n",qbi->tbitem->ident, qbi,bfwin->toolbar_quickbar);
					break;
				}
			}
			tmplist = g_list_next(tmplist);
		}
	}
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), bfwin->toolbar_quickbar, gtk_label_new(_(" Quick bar ")));

	html_toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(html_toolbar), GTK_TOOLBAR_ICONS);
	html_toolbar_add_items(bfwin,html_toolbar, tbi, 0, 14);
	html_toolbar_add_items_to_submenu(bfwin,html_toolbar, tbi, 26, 31, _("Heading"), pixmap_headings);
	html_toolbar_add_items(bfwin,html_toolbar, tbi, 76, 78);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" Standard bar ")));

	html_toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(html_toolbar), GTK_TOOLBAR_ICONS);
	html_toolbar_add_items(bfwin,html_toolbar, tbi, 15, 25);
	html_toolbar_add_items_to_submenu(bfwin,html_toolbar, tbi, 79, 86, _("Context formatting"), pixmap_context);
	html_toolbar_add_items(bfwin,html_toolbar, tbi, 25, 31);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" Fonts ")));

	html_toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(html_toolbar), GTK_TOOLBAR_ICONS);
	html_toolbar_add_items(bfwin,html_toolbar, tbi, 32, 43);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" Tables ")));

	html_toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(html_toolbar), GTK_TOOLBAR_ICONS);
	html_toolbar_add_items(bfwin,html_toolbar, tbi, 44, 52);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" Frames ")));

	html_toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(html_toolbar), GTK_TOOLBAR_ICONS);
	html_toolbar_add_items(bfwin,html_toolbar, tbi, 53, 63);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" Forms ")));

	html_toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(html_toolbar), GTK_TOOLBAR_ICONS);
	html_toolbar_add_items(bfwin,html_toolbar, tbi, 64, 70);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" List ")));

	html_toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(html_toolbar), GTK_TOOLBAR_ICONS);
	html_toolbar_add_items(bfwin,html_toolbar, tbi, 71, 73);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), html_toolbar, gtk_label_new(_(" CSS ")));
	DEBUG_MSG("quickbar_items=%p\n",htmlbar_v.quickbar_items);

	gtk_widget_show_all(hbw->handlebox);
	if (htmlbar_v.quickbar_items == NULL) {
		DEBUG_MSG("set notebook to page 1\n");
		gtk_notebook_set_current_page(GTK_NOTEBOOK(html_notebook),1);
	}

}
