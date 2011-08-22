/*
 * Bluefish HTML Editor
 * htmlbar_uimanager.c
 *
 * Copyright (C) 2005-2011 Olivier Sessink
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


#include "htmlbar_uimanager.h"
#include "../bluefish.h"
#include "../bfwin_uimanager.h"
#include "../document.h"
#include "cap.h"
#include "html.h"
#include "html2.h"
#include "htmlbar_stock_icons.h"
#include "html_form.h"
#include "html_table.h"
#include "image.h"
#include "image_dialog.h"
#include "rpopup.h"
#include "quickstart.h"
#include "wizards.h"


/*#define DEBUG*/

#define HTMLBAR_PIXMAP_DIR PKGDATADIR"/plugins/htmlbar/pixmaps/"
#define HTMLBAR_MENU_UI	   PKGDATADIR"/plugins/htmlbar/ui/htmlbar_menu_ui.xml"
#define HTMLBAR_TOOLBAR_UI PKGDATADIR"/plugins/htmlbar/ui/htmlbar_toolbar_ui.xml"

#include "pixmaps/htmlbar_icons.c"

/* For testing purposes */
/*#define HTMLBAR_MENU_UI	"src/plugin_htmlbar/ui/htmlbar_menu_ui.xml"
#define HTMLBAR_TOOLBAR_UI	"src/plugin_htmlbar/ui/htmlbar_toolbar_ui.xml"*/


static void
htmlbar_edit_tag_under_cursor(GtkAction * action, gpointer user_data)
{
	edit_tag_under_cursor_cb(BFWIN(user_data));
}

static void
htmlbar_dialog_anchor(GtkAction * action, gpointer user_data)
{
	quickanchor_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_basefont(GtkAction * action, gpointer user_data)
{
	basefont_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_body(GtkAction * action, gpointer user_data)
{
	body_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_color(GtkAction * action, gpointer user_data)
{
	sel_colour_cb(NULL, BFWIN(user_data));
}

static void
htmlbar_dialog_div(GtkAction * action, gpointer user_data)
{
	div_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_email(GtkAction * action, gpointer user_data)
{
	email_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_embed(GtkAction * action, gpointer user_data)
{
	embed_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_font(GtkAction * action, gpointer user_data)
{
	font_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_form(GtkAction * action, gpointer user_data)
{
	formdialog_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_form_button(GtkAction * action, gpointer user_data)
{
	buttondialog_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_form_check_box(GtkAction * action, gpointer user_data)
{
	inputdialog_dialog(BFWIN(user_data), NULL, "checkbox");
}

static void
htmlbar_dialog_form_hidden(GtkAction * action, gpointer user_data)
{
	inputdialog_dialog(BFWIN(user_data), NULL, "hidden");
}

static void
htmlbar_dialog_form_input(GtkAction * action, gpointer user_data)
{
	inputdialog_dialog(BFWIN(user_data), NULL, NULL);
}

static void
htmlbar_dialog_form_input_buttons(GtkAction * action, gpointer user_data)
{
	inputdialog_dialog(BFWIN(user_data), NULL, "submit");
}

static void
htmlbar_dialog_form_input_text(GtkAction * action, gpointer user_data)
{
	inputdialog_dialog(BFWIN(user_data), NULL, "text");
}

static void
htmlbar_dialog_form_option(GtkAction * action, gpointer user_data)
{
	optiondialog_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_form_option_group(GtkAction * action, gpointer user_data)
{
	optgroupdialog_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_form_radio_button(GtkAction * action, gpointer user_data)
{
	inputdialog_dialog(BFWIN(user_data), NULL, "radio");
}

static void
htmlbar_dialog_form_select(GtkAction * action, gpointer user_data)
{
	selectdialog_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_form_textarea(GtkAction * action, gpointer user_data)
{
	textareadialog_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_frame(GtkAction * action, gpointer user_data)
{
	frame_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_frameset(GtkAction * action, gpointer user_data)
{
	frameset_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_frame_wizard(GtkAction * action, gpointer user_data)
{
	framewizard_dialog(BFWIN(user_data));
}

static void
htmlbar_dialog_image(GtkAction * action, gpointer user_data)
{
	bluefish_image_dialog_new(BFWIN(user_data));
}

static void
htmlbar_dialog_link_header(GtkAction * action, gpointer user_data)
{
	linkdialog_dialog(BFWIN(user_data), NULL, linkdialog_mode_default);
}

static void
htmlbar_dialog_meta(GtkAction * action, gpointer user_data)
{
	meta_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_multi_thumbnail(GtkAction * action, gpointer user_data)
{
	multi_thumbnail_dialog(BFWIN(user_data));
}

static void
htmlbar_dialog_rule(GtkAction * action, gpointer user_data)
{
	quickrule_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_span(GtkAction * action, gpointer user_data)
{
	span_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_style(GtkAction * action, gpointer user_data)
{
	gchar *tmp = g_strdup_printf("<%s=\"text/css\"><!--\n", cap("STYLE TYPE"));
	doc_insert_two_strings(BFWIN(user_data)->current_document, tmp, cap("\n--></STYLE>"));
	g_free(tmp);
}

static void
htmlbar_dialog_style_create(GtkAction * action, gpointer user_data)
{
	new_css_dialog(NULL, BFWIN(user_data));
}

static void
htmlbar_dialog_style_link_to(GtkAction * action, gpointer user_data)
{
	linkdialog_dialog(BFWIN(user_data), NULL, linkdialog_mode_css);
}

static void
htmlbar_dialog_table(GtkAction * action, gpointer user_data)
{
	tabledialog_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_table_data(GtkAction * action, gpointer user_data)
{
	tabledatadialog_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_table_head(GtkAction * action, gpointer user_data)
{
	tableheaddialog_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_table_row(GtkAction * action, gpointer user_data)
{
	tablerowdialog_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_table_wizard(GtkAction * action, gpointer user_data)
{
	tablewizard_dialog(BFWIN(user_data));
}

static void
htmlbar_dialog_thumbnail(GtkAction * action, gpointer user_data)
{
	thumbnail_insert_dialog(BFWIN(user_data));
}

static void
htmlbar_dialog_time(GtkAction * action, gpointer user_data)
{
	insert_time_dialog(BFWIN(user_data));
}

static void
htmlbar_dialog_quicklist(GtkAction * action, gpointer user_data)
{
	quicklist_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_quickstart(GtkAction * action, gpointer user_data)
{
	quickstart_dialog_new(BFWIN(user_data));
}

static void
htmlbar_insert_html_comment(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, "<!-- ", " -->");
}

static void
htmlbar_insert_abbr_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<ABBR>"), cap("</ABBR>"));
}

static void
htmlbar_insert_acronym_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<ACRONYM>"), cap("</ACRONYM>"));
}

static void
htmlbar_insert_align_right_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document,
						   main_v->props.xhtml ==
						   1 ? cap("<DIV STYLE=\"text-align: right\">") : cap("<DIV ALIGN=\"RIGHT\">"),
						   cap("</DIV>"));
}

static void
htmlbar_insert_anchor_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<A>"), cap("</A>"));
}

static void
htmlbar_insert_big_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<BIG>"), cap("</BIG>"));
}

static void
htmlbar_insert_bold_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<B>"), cap("</B>"));
}

static void
htmlbar_insert_break_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document,
						   main_v->props.xhtml == 1 ? cap("<br />") : cap("<br>"), NULL);
}

static void
htmlbar_insert_center_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document,
						   main_v->props.xhtml ==
						   1 ? cap("<DIV STYLE=\"text-align: center\">") : cap("<DIV ALIGN=\"CENTER\">"),
						   cap("</DIV>"));
}

static void
htmlbar_insert_citation_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<CITE>"), cap("</CITE>"));
}

static void
htmlbar_insert_clearall_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document,
						   main_v->props.xhtml == 1 ? cap("<BR CLEAR=\"ALL\" />") : cap("<BR CLEAR=\"ALL\">"),
						   NULL);
}

static void
htmlbar_insert_code_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<CODE>"), cap("</CODE>"));
}

static void
htmlbar_insert_define_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<DFN>"), cap("</DFN>"));
}

static void
htmlbar_insert_definition_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<DD>"), cap("</DD>"));
}

static void
htmlbar_insert_definition_list_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<DL>\n\t<DT></DT>\n\t<DD></DD>\n</DL>"),
						   "\n");
}

static void
htmlbar_insert_definition_term_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<DT>"), cap("</DT>"));
}

static void
htmlbar_insert_delete_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<DEL>"), cap("</DEL>"));
}

static void
htmlbar_insert_div_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<DIV>"), cap("</DIV>"));
}

static void
htmlbar_insert_emphasis_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<EM>"), cap("</EM>"));
}

static void
htmlbar_insert_font_minus_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document,
						   main_v->props.xhtml ==
						   1 ? cap("<SPAN STYLE=\"font-size: larger;\">") : cap("<FONT SIZE=\"+1\">")
						   , main_v->props.xhtml == 1 ? cap("</SPAN>") : cap("</FONT>"));
}

static void
htmlbar_insert_font_plus_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document,
						   main_v->props.xhtml ==
						   1 ? cap("<SPAN STYLE=\"font-size: smaller;\">") : cap("<FONT SIZE=\"-1\">")
						   , main_v->props.xhtml == 1 ? cap("</SPAN>") : cap("</FONT>"));
}

static void
htmlbar_insert_form_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<FORM>"), cap("</FORM>"));
}

static void
htmlbar_insert_form_button_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<BUTTON>"), cap("</BUTTON>"));
}

static void
htmlbar_insert_form_input_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document,
						   main_v->props.xhtml == 1 ? cap("<INPUT />") : cap("<INPUT>"), NULL);
}

static void
htmlbar_insert_form_textarea_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<TEXTAREA>"), cap("</TEXTAREA>"));
}

static void
htmlbar_insert_frame_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document,
						   main_v->props.xhtml == 1 ? cap("<FRAME />") : cap("<FRAME>"), NULL);
}

static void
htmlbar_insert_frameset_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<FRAMESET>"), cap("</FRAMESET>"));
}

static void
htmlbar_insert_generator_meta_tag(GtkAction * action, gpointer user_data)
{
	gchar *tmp = g_strconcat(cap("<META NAME=\"Generator\" CONTENT=\""), "Bluefish ", VERSION,
							 " http://bluefish.openoffice.nl/",
							 main_v->props.xhtml == 1 ? "\" />\n" : "\">\n", NULL);
	doc_insert_two_strings(BFWIN(user_data)->current_document, tmp, NULL);
	g_free(tmp);
}

static void
htmlbar_insert_h1_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<H1>"), cap("</H1>"));
}

static void
htmlbar_insert_h2_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<H2>"), cap("</H2>"));
}

static void
htmlbar_insert_h3_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<H3>"), cap("</H3>"));
}

static void
htmlbar_insert_h4_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<H4>"), cap("</H4>"));
}

static void
htmlbar_insert_h5_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<H5>"), cap("</H5>"));
}

static void
htmlbar_insert_h6_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<H6>"), cap("</H6>"));
}

static void
htmlbar_insert_image_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document,
						   main_v->props.xhtml == 1 ? cap("<IMG />") : cap("<IMG>"), NULL);
}

static void
htmlbar_insert_insert_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<INS>"), cap("</INS>"));
}

static void
htmlbar_insert_italic_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<I>"), cap("</I>"));
}

static void
htmlbar_insert_keyboard_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<KBD>"), cap("</KBD>"));
}

static void
htmlbar_insert_link_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document,
						   main_v->props.xhtml == 1 ? cap("<LINK />") : cap("<LINK>"), NULL);
}

static void
htmlbar_insert_list_item_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<LI>"), cap("</LI>"));
}

static void
htmlbar_insert_list_ordered_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<OL>"), cap("</OL>"));
}

static void
htmlbar_insert_list_unordered_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<UL>"), cap("</UL>"));
}

static void
htmlbar_insert_menu_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<MENU>"), cap("</MENU>"));
}

static void
htmlbar_insert_nbsp_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, "&nbsp;", NULL);
}

static void
htmlbar_insert_noframes_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<NOFRAMES>"), cap("</NOFRAMES>"));
}

static void
htmlbar_insert_paragraph_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<P>"), cap("</P>"));
}

static void
htmlbar_insert_pre_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<PRE>"), cap("</PRE>"));
}

static void
htmlbar_insert_sample_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<SAMP>"), cap("</SAMP>"));
}

static void
htmlbar_insert_script_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<SCRIPT>"), cap("</SCRIPT>"));
}

static void
htmlbar_insert_small_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<SMALL>"), cap("</SMALL>"));
}

static void
htmlbar_insert_span_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<SPAN>"), cap("</SPAN>"));
}

static void
htmlbar_insert_strikeout_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<STRIKE>"), cap("</STRIKE>"));
}

static void
htmlbar_insert_strong_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<STRONG>"), cap("</STRONG>"));
}

static void
htmlbar_insert_subscript_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<SUB>"), cap("</SUB>"));
}

static void
htmlbar_insert_superscript_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<SUP>"), cap("</SUP>"));
}

static void
htmlbar_insert_table_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<TABLE>"), cap("</TABLE>"));
}

static void
htmlbar_insert_table_caption_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<CAPTION>"), cap("</CAPTION>"));
}

static void
htmlbar_insert_table_data_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<TD>"), cap("</TD>"));
}

static void
htmlbar_insert_table_header_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<TH>"), cap("</TH>"));
}

static void
htmlbar_insert_table_row_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<TR>"), cap("</TR>"));
}

static void
htmlbar_insert_target_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document,
						   main_v->props.xhtml ==
						   1 ? cap("<BASE TARGET=\"\" />") : cap("<BASE TARGET=\"\">"), NULL);
}

static void
htmlbar_insert_underline_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document,
						   main_v->props.xhtml ==
						   1 ? cap("<SPAN STYLE=\"text-decoration: underline;\">") : cap("<U>")
						   , main_v->props.xhtml == 1 ? cap("</SPAN>") : cap("</U>"));
}

static void
htmlbar_insert_variable_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<VAR>"), cap("</VAR>"));
}

static void
htmlbar_insert_acute(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_ACUTE);
}

static void
htmlbar_insert_cedilla(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_CEDILLA);
}

static void
htmlbar_insert_umlaut(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_UMLAUT);
}

static void
htmlbar_insert_aacute(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_A_ACUTE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_A_ACUTE_LOWER);
}

static void
htmlbar_insert_acirc(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_A_CIRC_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_A_CIRC_LOWER);
}

static void
htmlbar_insert_aelig(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_A_ELIG_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_A_ELIG_LOWER);
}

static void
htmlbar_insert_agrave(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_A_GRAVE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_A_GRAVE_LOWER);
}

static void
htmlbar_insert_aring(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_A_RING_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_A_RING_LOWER);
}

static void
htmlbar_insert_atilde(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_A_TILDE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_A_TILDE_LOWER);
}

static void
htmlbar_insert_auml(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_A_UML_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_A_UML_LOWER);
}

static void
htmlbar_insert_ccedil(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_C_CEDIL_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_C_CEDIL_LOWER);
}

static void
htmlbar_insert_eacute(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_E_ACUTE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_E_ACUTE_LOWER);
}

static void
htmlbar_insert_ecirc(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_E_CIRC_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_E_CIRC_LOWER);
}

static void
htmlbar_insert_egrave(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_E_GRAVE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_E_GRAVE_LOWER);
}

static void
htmlbar_insert_euml(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_E_UML_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_E_UML_LOWER);
}

static void
htmlbar_insert_iacute(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_I_ACUTE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_I_ACUTE_LOWER);
}

static void
htmlbar_insert_icirc(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_I_CIRC_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_I_CIRC_LOWER);
}

static void
htmlbar_insert_igrave(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_I_GRAVE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_I_GRAVE_LOWER);
}

static void
htmlbar_insert_iuml(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_I_UML_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_I_UML_LOWER);
}

static void
htmlbar_insert_ntilde(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_N_TILDE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_N_TILDE_LOWER);
}

static void
htmlbar_insert_oacute(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_O_ACUTE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_O_ACUTE_LOWER);
}

static void
htmlbar_insert_ocirc(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_O_CIRC_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_O_CIRC_LOWER);
}

static void
htmlbar_insert_oelig(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_O_ELIG_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_O_ELIG_LOWER);
}

static void
htmlbar_insert_ograve(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_O_GRAVE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_O_GRAVE_LOWER);
}

static void
htmlbar_insert_oslash(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_O_SLASH_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_O_SLASH_LOWER);
}

static void
htmlbar_insert_otilde(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_O_TILDE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_O_TILDE_LOWER);
}

static void
htmlbar_insert_ouml(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_O_UML_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_O_UML_LOWER);
}

static void
htmlbar_insert_uacute(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_U_ACUTE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_U_ACUTE_LOWER);
}

static void
htmlbar_insert_ucirc(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_U_CIRC_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_U_CIRC_LOWER);
}

static void
htmlbar_insert_ugrave(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_U_GRAVE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_U_GRAVE_LOWER);
}

static void
htmlbar_insert_uuml(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_U_UML_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_U_UML_LOWER);
}

static void
htmlbar_insert_yacute(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_Y_ACUTE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_Y_ACUTE_LOWER);
}

static void
htmlbar_insert_yuml(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_Y_UML_LOWER);
}

static void
htmlbar_insert_cent_sign(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_CENT);
}

static void
htmlbar_insert_currency(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_CURRENCY);
}

static void
htmlbar_insert_euro(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_EURO);
}

static void
htmlbar_insert_pound(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_POUND);
}

static void
htmlbar_insert_yen(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_YEN);
}

static void
htmlbar_insert_division(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_DIVISION);
}

static void
htmlbar_insert_logical_not(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_LOGICAL_NOT);
}

static void
htmlbar_insert_times(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_TIMES);
}

static void
htmlbar_insert_plus_minus(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_PLUS_MINUS);
}

static void
htmlbar_insert_less_than(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_LESS_THAN);
}

static void
htmlbar_insert_greater_than(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_GREATER_THAN);
}

static void
htmlbar_insert_superscript1(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_SUPERSCRIPT_1);
}

static void
htmlbar_insert_superscript2(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_SUPERSCRIPT_2);
}

static void
htmlbar_insert_superscript3(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_SUPERSCRIPT_3);
}

static void
htmlbar_insert_one_quarter(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_FRAC_QUARTER);
}

static void
htmlbar_insert_one_half(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_FRAC_HALF);
}

static void
htmlbar_insert_three_quarter(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_FRAC_THREE_QUARTER);
}

static void
htmlbar_insert_degree(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_DEGREE);
}

static void
htmlbar_insert_eth(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_ETH_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_ETH_LOWER);
}

static void
htmlbar_insert_thorn(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_THORN_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_THORN_LOWER);
}

static void
htmlbar_insert_sharp(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_SZLIG);
}

static void
htmlbar_insert_micro_sign(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_MIRCO);
}

static void
htmlbar_insert_nbsp(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_NBSP);
}

static void
htmlbar_insert_section_sign(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_SECTION);
}

static void
htmlbar_insert_copyright(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_COPYRIGHT);
}

static void
htmlbar_insert_left_angle(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_LEFT_ANGLE);
}

static void
htmlbar_insert_right_angle(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_RIGHT_ANGLE);
}

static void
htmlbar_insert_trademark(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_TRADEMARK);
}

static void
htmlbar_insert_inverted_exclamation(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_INVERTED_EXCLAMATION);
}

static void
htmlbar_insert_inverted_question_mark(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_INVERTED_QUESTION_MARK);
}

static void
htmlbar_insert_ordinal(GtkAction * action, gpointer user_data)
{
	if (g_str_has_suffix(gtk_action_get_name(action), "F"))
		htmlbar_insert_special_char(BFWIN(user_data), BF_ORDINAL_F);
	else
		htmlbar_insert_special_char(BFWIN(user_data), BF_ORDINAL_M);
}

static void
htmlbar_insert_paragraph(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_PARAGRAPH);
}

static void
htmlbar_insert_broken_bar(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_BROKEN_BAR);
}

static void
htmlbar_insert_soft_hyphen(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_SOFT_HYPHEN);
}

static void
htmlbar_insert_spacing_macron(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_SPACING_MACRON);
}

static void
htmlbar_insert_middot(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_MIDDOT);
}

void
htmlbar_toolbar_show(Thtmlbarwin * hbw, gboolean show)
{
	if (show) {
		if (hbw->handlebox)
			gtk_widget_show(hbw->handlebox);
		else
			htmlbar_toolbar_create(hbw);
	} else {
		if (hbw->handlebox)
			gtk_widget_hide(hbw->handlebox);
	}
}

static void
htmlbar_toolbar_show_toogle(GtkAction * action, gpointer user_data)
{
	Thtmlbarwin *hbw = (Thtmlbarwin *) user_data;
	Thtmlbarsession *hbs;

	hbs = g_hash_table_lookup(htmlbar_v.lookup, hbw->bfwin->session);
	if (hbs) {
		hbs->view_htmlbar = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	} else {
		DEBUG_MSG("htmlbar_toolbar_show_toogle, ERROR, no htmlbarsession ???\n");
	}

	htmlbar_toolbar_show(hbw, gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));
}

static const GtkActionEntry htmlbar_actions[] = {
	{"TagsMenu", NULL, N_("T_ags")},
	{"HeadingsMenu", NULL, N_("_Headings")},
	{"HeadingsH1", BF_STOCK_FONTH1, N_("H_1"), "<control><alt>1", N_("Insert H1 tag"),
	 G_CALLBACK(htmlbar_insert_h1_tag)},
	{"HeadingsH2", BF_STOCK_FONTH2, N_("H_2"), "<control><alt>2", N_("Insert H2 tag"),
	 G_CALLBACK(htmlbar_insert_h2_tag)},
	{"HeadingsH3", BF_STOCK_FONTH3, N_("H_3"), "<control><alt>3", N_("Insert H3 tag"),
	 G_CALLBACK(htmlbar_insert_h3_tag)},
	{"HeadingsH4", BF_STOCK_FONTH4, N_("H_4"), "<control><alt>4", N_("Insert H4 tag"),
	 G_CALLBACK(htmlbar_insert_h4_tag)},
	{"HeadingsH5", BF_STOCK_FONTH5, N_("H_5"), "<control><alt>5", N_("Insert H5 tag"),
	 G_CALLBACK(htmlbar_insert_h5_tag)},
	{"HeadingsH6", BF_STOCK_FONTH6, N_("H_6"), "<control><alt>6", N_("Insert H6 tag"),
	 G_CALLBACK(htmlbar_insert_h6_tag)},
	{"SpecialMenu", NULL, N_("_Special")},
	{"AccentMenu", NULL, N_("_Accent")},
	{"UppercaseMenu", NULL, N_("_Uppercase")},
	{"UppercaseAIMenu", NULL, N_("_A-I")},
	{"UppercaseAgrave", NULL, N_("A grave À"), NULL, NULL, G_CALLBACK(htmlbar_insert_agrave)},
	{"UppercaseAacute", NULL, N_("A acute Á"), NULL, NULL, G_CALLBACK(htmlbar_insert_aacute)},
	{"UppercaseAcircumflex", NULL, N_("A circumflex Â"), NULL, NULL, G_CALLBACK(htmlbar_insert_acirc)},
	{"UppercaseAtilde", NULL, N_("A tilde Ã"), NULL, NULL, G_CALLBACK(htmlbar_insert_atilde)},
	{"UppercaseAEligature", NULL, N_("AE ligature Æ"), NULL, NULL, G_CALLBACK(htmlbar_insert_aelig)},
	{"UppercaseAdiaeresis", NULL, N_("A diaeresis Ä"), NULL, NULL, G_CALLBACK(htmlbar_insert_auml)},
	{"UppercaseAring", NULL, N_("A ring Å"), NULL, NULL, G_CALLBACK(htmlbar_insert_aring)},
	{"UppercaseCcedilla", NULL, N_("C cedilla Ç"), NULL, NULL, G_CALLBACK(htmlbar_insert_ccedil)},
	{"UppercaseEgrave", NULL, N_("E grave È"), NULL, NULL, G_CALLBACK(htmlbar_insert_egrave)},
	{"UppercaseEacute", NULL, N_("E acute É"), NULL, NULL, G_CALLBACK(htmlbar_insert_eacute)},
	{"UppercaseEcircumflex", NULL, N_("E circumflex Ê"), NULL, NULL, G_CALLBACK(htmlbar_insert_ecirc)},
	{"UppercaseEdiaeresis", NULL, N_("E diaeresis Ë"), NULL, NULL, G_CALLBACK(htmlbar_insert_euml)},
	{"UppercaseIgrave", NULL, N_("I grave Ì"), NULL, NULL, G_CALLBACK(htmlbar_insert_igrave)},
	{"UppercaseIacute", NULL, N_("I acute Í"), NULL, NULL, G_CALLBACK(htmlbar_insert_iacute)},
	{"UppercaseIcircumflex", NULL, N_("I circumflex Î"), NULL, NULL, G_CALLBACK(htmlbar_insert_icirc)},
	{"UppercaseIdiaeresis", NULL, N_("I diaeresis Ï"), NULL, NULL, G_CALLBACK(htmlbar_insert_iuml)},
	{"UppercaseJZMenu", NULL, N_("_J-Z")},
	{"UppercaseNtilde", NULL, N_("N tilde Ñ"), NULL, NULL, G_CALLBACK(htmlbar_insert_ntilde)},
	{"UppercaseOgrave", NULL, N_("O grave Ò"), NULL, NULL, G_CALLBACK(htmlbar_insert_ograve)},
	{"UppercaseOacute", NULL, N_("O acute Ó"), NULL, NULL, G_CALLBACK(htmlbar_insert_oacute)},
	{"UppercaseOcircumflex", NULL, N_("O circumflex Ô"), NULL, NULL, G_CALLBACK(htmlbar_insert_ocirc)},
	{"UppercaseOEligature", NULL, N_("OE ligature Œ"), NULL, NULL, G_CALLBACK(htmlbar_insert_oelig)},
	{"UppercaseOtilde", NULL, N_("O tilde Õ"), NULL, NULL, G_CALLBACK(htmlbar_insert_otilde)},
	{"UppercaseOdiaeresis", NULL, N_("O diaeresis Ö"), NULL, NULL, G_CALLBACK(htmlbar_insert_ouml)},
	{"UppercaseOslash", NULL, N_("O slash Ø"), NULL, NULL, G_CALLBACK(htmlbar_insert_oslash)},
	{"UppercaseUgrave", NULL, N_("U grave Ù"), NULL, NULL, G_CALLBACK(htmlbar_insert_ugrave)},
	{"UppercaseUacute", NULL, N_("U acute Ú"), NULL, NULL, G_CALLBACK(htmlbar_insert_uacute)},
	{"UppercaseUcircumflex", NULL, N_("U circumflex Û"), NULL, NULL, G_CALLBACK(htmlbar_insert_ucirc)},
	{"UppercaseUdiaeresis", NULL, N_("U diaeresis Ü"), NULL, NULL, G_CALLBACK(htmlbar_insert_uuml)},
	{"UppercaseYacute", NULL, N_("Y acute Ý"), NULL, NULL, G_CALLBACK(htmlbar_insert_yacute)},
	{"LowercaseMenu", NULL, N_("_Lowercase")},
	{"LowercaseAIMenu", NULL, N_("_A-I")},
	{"LowercaseAgrave", NULL, N_("a grave à"), NULL, NULL, G_CALLBACK(htmlbar_insert_agrave)},
	{"LowercaseAacute", NULL, N_("a acute á"), NULL, NULL, G_CALLBACK(htmlbar_insert_aacute)},
	{"LowercaseAcircumflex", NULL, N_("a circumflex â"), NULL, NULL, G_CALLBACK(htmlbar_insert_acirc)},
	{"LowercaseAtilde", NULL, N_("a tilde ã"), NULL, NULL, G_CALLBACK(htmlbar_insert_atilde)},
	{"LowercaseAring", NULL, N_("a ring å"), NULL, NULL, G_CALLBACK(htmlbar_insert_aring)},
	{"LowercaseAEligature", NULL, N_("ae ligature æ"), NULL, NULL, G_CALLBACK(htmlbar_insert_aelig)},
	{"LowercaseAdiaeresis", NULL, N_("a diaeresis ä"), NULL, NULL, G_CALLBACK(htmlbar_insert_auml)},
	{"LowercaseCcedilla", NULL, N_("c cedilla ç"), NULL, NULL, G_CALLBACK(htmlbar_insert_ccedil)},
	{"LowercaseEgrave", NULL, N_("e grave è"), NULL, NULL, G_CALLBACK(htmlbar_insert_egrave)},
	{"LowercaseEacute", NULL, N_("e acute é"), NULL, NULL, G_CALLBACK(htmlbar_insert_eacute)},
	{"LowercaseEcircumflex", NULL, N_("e circumflex ê"), NULL, NULL, G_CALLBACK(htmlbar_insert_ecirc)},
	{"LowercaseEdiaeresis", NULL, N_("e diaeresis ë"), NULL, NULL, G_CALLBACK(htmlbar_insert_euml)},
	{"LowercaseIgrave", NULL, N_("i grave ì"), NULL, NULL, G_CALLBACK(htmlbar_insert_igrave)},
	{"LowercaseIacute", NULL, N_("i acute í"), NULL, NULL, G_CALLBACK(htmlbar_insert_iacute)},
	{"LowercaseIcircumflex", NULL, N_("i circumflex î"), NULL, NULL, G_CALLBACK(htmlbar_insert_icirc)},
	{"LowercaseIdiaeresis", NULL, N_("i diaeresis ï"), NULL, NULL, G_CALLBACK(htmlbar_insert_iuml)},
	{"LowercaseJZMenu", NULL, N_("_J-Z")},
	{"LowercaseNtilde", NULL, N_("n tilde ñ"), NULL, NULL, G_CALLBACK(htmlbar_insert_ntilde)},
	{"LowercaseOgrave", NULL, N_("o grave ò"), NULL, NULL, G_CALLBACK(htmlbar_insert_ograve)},
	{"LowercaseOacute", NULL, N_("o acute ó"), NULL, NULL, G_CALLBACK(htmlbar_insert_oacute)},
	{"LowercaseOEligature", NULL, N_("oe ligature œ"), NULL, NULL, G_CALLBACK(htmlbar_insert_oelig)},
	{"LowercaseOcircumflex", NULL, N_("o circumflex ô"), NULL, NULL, G_CALLBACK(htmlbar_insert_ocirc)},
	{"LowercaseOtilde", NULL, N_("o tilde õ"), NULL, NULL, G_CALLBACK(htmlbar_insert_otilde)},
	{"LowercaseOdiaeresis", NULL, N_("o diaeresis ö"), NULL, NULL, G_CALLBACK(htmlbar_insert_ouml)},
	{"LowercaseOslash", NULL, N_("o slash ø"), NULL, NULL, G_CALLBACK(htmlbar_insert_oslash)},
	{"LowercaseUgrave", NULL, N_("u grave ù"), NULL, NULL, G_CALLBACK(htmlbar_insert_ugrave)},
	{"LowercaseUacute", NULL, N_("u acute ú"), NULL, NULL, G_CALLBACK(htmlbar_insert_uacute)},
	{"LowercaseUcircumflex", NULL, N_("u circumflex û"), NULL, NULL, G_CALLBACK(htmlbar_insert_ucirc)},
	{"LowercaseUdiaeresis", NULL, N_("u diaeresis ü"), NULL, NULL, G_CALLBACK(htmlbar_insert_uuml)},
	{"LowercaseYacute", NULL, N_("y acute ý"), NULL, NULL, G_CALLBACK(htmlbar_insert_yacute)},
	{"LowercaseYdiaeresis", NULL, N_("y diaeresis ÿ"), NULL, NULL, G_CALLBACK(htmlbar_insert_yuml)},
	{"Acute", NULL, N_("_Acute ´"), NULL, NULL, G_CALLBACK(htmlbar_insert_acute)},
	{"Cedilla", NULL, N_("_Cedilla ¸"), NULL, NULL, G_CALLBACK(htmlbar_insert_cedilla)},
	{"Umlaut", NULL, N_("U_mlaut ¨"), NULL, NULL, G_CALLBACK(htmlbar_insert_umlaut)},
	{"CurrencyMenu", NULL, N_("_Currency")},
	{"CentSign", NULL, N_("_Cent sign ¢"), NULL, NULL, G_CALLBACK(htmlbar_insert_cent_sign)},
	{"PoundSterling", NULL, N_("_Pound sterling £"), NULL, NULL, G_CALLBACK(htmlbar_insert_pound)},
	{"CurrencySign", NULL, N_("C_urrency sign ¤"), NULL, NULL, G_CALLBACK(htmlbar_insert_currency)},
	{"YenSign", NULL, N_("_Yen sign ¥"), NULL, NULL, G_CALLBACK(htmlbar_insert_yen)},
	{"Euro", NULL, N_("_Euro €"), NULL, NULL, G_CALLBACK(htmlbar_insert_euro)},
	{"MathScienceMenu", NULL, N_("_Math-Science")},
	{"LogicalNot", NULL, N_("Logical _not sign ¬"), NULL, NULL, G_CALLBACK(htmlbar_insert_logical_not)},
	{"Multiplication", NULL, N_("_Multiplication sign ×"), NULL, NULL, G_CALLBACK(htmlbar_insert_times)},
	{"Division", NULL, N_("_Division sign ÷"), "<control><alt>slash", NULL,
	 G_CALLBACK(htmlbar_insert_division)},
	{"PlusMinus", NULL, N_("_Plus-minus sign ±"), NULL, NULL, G_CALLBACK(htmlbar_insert_plus_minus)},
	{"LessThan", NULL, N_("_Less-than sign <"), "<control><alt>comma", NULL,
	 G_CALLBACK(htmlbar_insert_less_than)},
	{"GreaterThan", NULL, N_("_Greater-than sign >"), "<control><alt>period", NULL,
	 G_CALLBACK(htmlbar_insert_greater_than)},
	{"Superscript1", NULL, N_("Superscript _1 ¹"), NULL, NULL, G_CALLBACK(htmlbar_insert_superscript1)},
	{"Superscript2", NULL, N_("Superscript _2 ²"), NULL, NULL, G_CALLBACK(htmlbar_insert_superscript2)},
	{"Superscript3", NULL, N_("Superscript _3 ³"), NULL, NULL, G_CALLBACK(htmlbar_insert_superscript3)},
	{"OneQuarter", NULL, N_("One _quarter ¼"), NULL, NULL, G_CALLBACK(htmlbar_insert_one_quarter)},
	{"OneHalf", NULL, N_("One _half ½"), NULL, NULL, G_CALLBACK(htmlbar_insert_one_half)},
	{"ThreeQuarters", NULL, N_("_Three quarters ¾"), NULL, NULL,
	 G_CALLBACK(htmlbar_insert_three_quarter)},
	{"DegreeSign", NULL, N_("Deg_ree sign °"), NULL, NULL, G_CALLBACK(htmlbar_insert_degree)},
	{"NonLatinMenu", NULL, N_("_Non Latin")},
	{"UppercaseETH", NULL, N_("_ETH (Icelandic) Ð"), NULL, NULL, G_CALLBACK(htmlbar_insert_eth)},
	{"UppercaseTHORN", NULL, N_("_THORN (Icelandic) Þ"), NULL, NULL, G_CALLBACK(htmlbar_insert_thorn)},
	{"LowercaseETH", NULL, N_("et_h (Icelandic) ð"), NULL, NULL, G_CALLBACK(htmlbar_insert_eth)},
	{"LowercaseTHORN", NULL, N_("th_orn ÿ"), NULL, NULL, G_CALLBACK(htmlbar_insert_thorn)},
	{"NonLatinSharp", NULL, N_("Sharp _s ß"), NULL, NULL, G_CALLBACK(htmlbar_insert_sharp)},
	{"NonLatinMicroSign", NULL, N_("_Micro sign µ"), NULL, NULL, G_CALLBACK(htmlbar_insert_micro_sign)},
	{"SpecialOtherMenu", NULL, N_("_Other")},
	{"NBSP", NULL, N_("_Non-breaking space"), NULL, NULL, G_CALLBACK(htmlbar_insert_nbsp)},
	{"SectionSign", NULL, N_("_Section sign §"), NULL, NULL, G_CALLBACK(htmlbar_insert_section_sign)},
	{"Copyright", NULL, N_("_Copyright sign ©"), NULL, NULL, G_CALLBACK(htmlbar_insert_copyright)},
	{"LeftAngleQuote", NULL, N_("_Left angle quotes «"), NULL, NULL,
	 G_CALLBACK(htmlbar_insert_left_angle)},
	{"RightAngleQuote", NULL, N_("_Right angle quotes »"), NULL, NULL,
	 G_CALLBACK(htmlbar_insert_right_angle)},
	{"Trademark", NULL, N_("Registered _trademark ®"), NULL, NULL, G_CALLBACK(htmlbar_insert_trademark)},
	{"InvertedExclamation", NULL, N_("Inverted _exclamation ¡"), NULL, NULL,
	 G_CALLBACK(htmlbar_insert_inverted_exclamation)},
	{"InvertedQuestionMark", NULL, N_("Inverted _question mark ¿"), NULL, NULL,
	 G_CALLBACK(htmlbar_insert_inverted_question_mark)},
	{"FeminineOrdinal", NULL, N_("_Feminine ordinal ª"), NULL, NULL, G_CALLBACK(htmlbar_insert_ordinal)},
	{"MasculineOrdinal", NULL, N_("_Masculine ordinal º"), NULL, NULL,
	 G_CALLBACK(htmlbar_insert_ordinal)},
	{"Pilcrow", NULL, N_("_Pilcrow (paragraph sign) ¶"), NULL, NULL,
	 G_CALLBACK(htmlbar_insert_paragraph)},
	{"BrokenBar", NULL, N_("_Broken bar ¦"), NULL, NULL, G_CALLBACK(htmlbar_insert_broken_bar)},
	{"SoftHyphen", NULL, N_("Soft _hyphen -"), NULL, NULL, G_CALLBACK(htmlbar_insert_soft_hyphen)},
	{"SpacingMacron", NULL, N_("Spa_cing macron ¯"), NULL, NULL,
	 G_CALLBACK(htmlbar_insert_spacing_macron)},
	{"MidDot", NULL, N_("Middle _dot ·"), NULL, NULL, G_CALLBACK(htmlbar_insert_middot)},
	{"FormatContextMenu", NULL, N_("F_ormat by context")},
	{"FormatStrong", BF_STOCK_BOLD, N_("_Strong"), "<control><alt>g", N_("Strong"),
	 G_CALLBACK(htmlbar_insert_strong_tag)},
	{"FormatEmphasis", BF_STOCK_ITALIC, N_("_Emphasis"), "<control><alt>e", N_("Emphasis"),
	 G_CALLBACK(htmlbar_insert_emphasis_tag)},
	{"FormatDefine", BF_STOCK_DFN, N_("_Definition"), NULL, N_("Definition"),
	 G_CALLBACK(htmlbar_insert_define_tag)},
	{"FormatCode", BF_STOCK_CODE, N_("_Code"), NULL, N_("Code"), G_CALLBACK(htmlbar_insert_code_tag)},
	{"FormatSample", BF_STOCK_SAMP, N_("Sa_mple"), NULL, N_("Sample"), G_CALLBACK(htmlbar_insert_sample_tag)},
	{"FormatKeyboard", BF_STOCK_KBD, N_("_Keyboard"), NULL, N_("Keyboard"),
	 G_CALLBACK(htmlbar_insert_keyboard_tag)},
	{"FormatVariable", BF_STOCK_VAR, N_("_Variable"), NULL, N_("Variable"),
	 G_CALLBACK(htmlbar_insert_variable_tag)},
	{"FormatCitation", BF_STOCK_CITE, N_("Ci_tation"), NULL, N_("Citation"),
	 G_CALLBACK(htmlbar_insert_citation_tag)},
	{"FormatAbbr", BF_STOCK_ADDR, N_("_Abbreviation"), NULL, N_("Abbreviation"),
	 G_CALLBACK(htmlbar_insert_abbr_tag)},
	{"FormatAcronym", BF_STOCK_ACRONYM, N_("Ac_ronym"), NULL, N_("Acronym"),
	 G_CALLBACK(htmlbar_insert_acronym_tag)},
	{"FormatInsert", BF_STOCK_INS, N_("_Insert"), NULL, N_("Insert"), G_CALLBACK(htmlbar_insert_insert_tag)},
	{"FormatDelete", BF_STOCK_DEL, N_("De_lete"), NULL, N_("Delete"), G_CALLBACK(htmlbar_insert_delete_tag)},
	{"FormatLayoutMenu", NULL, N_("_Format by layout")},
	{"FormatBold", BF_STOCK_BOLD, N_("_Bold"), "<control><alt>b", N_("Bold"),
	 G_CALLBACK(htmlbar_insert_bold_tag)},
	{"FormatItalic", BF_STOCK_ITALIC, N_("_Italic"), "<control><alt>i", N_("Italic"),
	 G_CALLBACK(htmlbar_insert_italic_tag)},
	{"FormatUnderline", BF_STOCK_UNDERLINE, N_("_Underline"), "<control><alt>u", N_("Underline"),
	 G_CALLBACK(htmlbar_insert_underline_tag)},
	{"FormatStrikeout", BF_STOCK_STRIKEOUT, N_("_Strikeout"), "<control><alt>s", N_("Strikeout"),
	 G_CALLBACK(htmlbar_insert_strikeout_tag)},
	{"FormatSmall", NULL, N_("Sm_all"), NULL, N_("Small"), G_CALLBACK(htmlbar_insert_small_tag)},
	{"FormatBig", NULL, N_("Bi_g"), NULL, N_("Big"), G_CALLBACK(htmlbar_insert_big_tag)},
	{"FormatGeneralMenu", NULL, N_("Format _general")},
	{"FormatParagraph", BF_STOCK_PARAGRAPH, N_("_Paragraph"), "<control><alt>p", N_("Paragraph"),
	 G_CALLBACK(htmlbar_insert_paragraph_tag)},
	{"FormatBreak", BF_STOCK_BRBREAK, N_("_Break"), "<control><alt>k", N_("Break"),
	 G_CALLBACK(htmlbar_insert_break_tag)},
	{"FormatClearAll", BF_STOCK_BREAKALL, N_("Break clear _all"), NULL, N_("Break and clear"),
	 G_CALLBACK(htmlbar_insert_clearall_tag)},
	{"FormatNBSP", BF_STOCK_NBSP, N_("_Non-Breaking Space"), "<control><alt>n", N_("Non-breaking space"),
	 G_CALLBACK(htmlbar_insert_nbsp_tag)},
	{"FormatFontSizePlus", BF_STOCK_FONTP1, N_("Font Si_ze +1"), "<control><alt>equal", N_("Font Size +1"),
	 G_CALLBACK(htmlbar_insert_font_plus_tag)},
	{"FormatFontSizeMinus", BF_STOCK_FONTM1, N_("Font _Size -1"), "<control><alt>minus", N_("Font Size -1"),
	 G_CALLBACK(htmlbar_insert_font_minus_tag)},
	{"FormatPre", BF_STOCK_FONTPRE, N_("Preformatted _Text"), "<Control><Alt>f", N_("Preformatted Text"),
	 G_CALLBACK(htmlbar_insert_pre_tag)},
	{"FormatSubscript", BF_STOCK_FONTSUB, N_("Su_bscript"), NULL, N_("Subscript"),
	 G_CALLBACK(htmlbar_insert_subscript_tag)},
	{"FormatSuperscript", BF_STOCK_FONTSUPER, N_("Su_perscript"), NULL, N_("Superscript"),
	 G_CALLBACK(htmlbar_insert_superscript_tag)},
	{"FormatCenter", BF_STOCK_CENTER, N_("_Center"), NULL, N_("Center"),
	 G_CALLBACK(htmlbar_insert_center_tag)},
	{"FormatAlignRight", BF_STOCK_RIGHT, N_("Align _right"), "<control><Alt>r", N_("Right Justify"),
	 G_CALLBACK(htmlbar_insert_align_right_tag)},
	{"TableMenu", NULL, N_("_Table")},
	{"TableTable", BF_STOCK_TABLE2, N_("_Table"), "<control><alt>t", N_("Table"),
	 G_CALLBACK(htmlbar_insert_table_tag)},
	{"TableRow", BF_STOCK_TABLE_TR2, N_("Table _Row"), NULL, N_("Table Row"),
	 G_CALLBACK(htmlbar_insert_table_row_tag)},
	{"TableHeader", BF_STOCK_TABLE_TH2, N_("Table _Header"), NULL, N_("Table Header"),
	 G_CALLBACK(htmlbar_insert_table_header_tag)},
	{"TableData", BF_STOCK_TABLE_TD2, N_("Table _Data"), NULL, N_("Table Data"),
	 G_CALLBACK(htmlbar_insert_table_data_tag)},
	{"TableCaption", BF_STOCK_TABLE_CAPTION, N_("Table _Caption"), NULL, N_("Table Caption"),
	 G_CALLBACK(htmlbar_insert_table_caption_tag)},
	{"ListMenu", NULL, N_("_List")},
	{"ListUnordered", BF_STOCK_LIST_UL, N_("Unordered _List"), "<control><alt>L", N_("Unordered List"),
	 G_CALLBACK(htmlbar_insert_list_unordered_tag)},
	{"ListOrdered", BF_STOCK_LIST_OL, N_("_Ordered List"), "<control><alt>O", N_("Ordered List"),
	 G_CALLBACK(htmlbar_insert_list_ordered_tag)},
	{"ListItem", BF_STOCK_LIST_LI, N_("List Ite_m"), "<control><alt>M", N_("List Item"),
	 G_CALLBACK(htmlbar_insert_list_item_tag)},
	{"ListDefList", BF_STOCK_LIST_DL, N_("De_finition List"), NULL, N_("Definition List"),
	 G_CALLBACK(htmlbar_insert_definition_list_tag)},
	{"ListDefTerm", BF_STOCK_LIST_DT, N_("Definition _Term"), NULL, N_("Definition Term"),
	 G_CALLBACK(htmlbar_insert_definition_term_tag)},
	{"ListDefinition", BF_STOCK_LIST_DD, N_("_Definition"), NULL, N_("Definition"),
	 G_CALLBACK(htmlbar_insert_definition_tag)},
	{"ListListMenu", NULL, N_("Men_u"), NULL, N_("Menu"), G_CALLBACK(htmlbar_insert_menu_tag)},
	{"FormMenu", NULL, N_("Fo_rm")},
	{"FormForm", BF_STOCK_FORM, N_("_Form"), NULL, N_("Form"), G_CALLBACK(htmlbar_insert_form_tag)},
	{"FormButton", NULL, N_("_Button"), NULL, NULL, G_CALLBACK(htmlbar_insert_form_button_tag)},
	{"FormInput", NULL, N_("_Input"), NULL, NULL, G_CALLBACK(htmlbar_insert_form_input_tag)},
	{"FormTextArea", NULL, N_("_Textarea"), NULL, NULL, G_CALLBACK(htmlbar_insert_form_textarea_tag)},
	{"MiscMenu", NULL, N_("_Misc")},
	{"MiscGenMETATag", NULL, N_("Insert Generator _META-Tag"), NULL, NULL,
	 G_CALLBACK(htmlbar_insert_generator_meta_tag)},
	{"MiscSpan", NULL, N_("_Span"), NULL, NULL, G_CALLBACK(htmlbar_insert_span_tag)},
	{"MiscDiv", NULL, N_("_Div"), NULL, NULL, G_CALLBACK(htmlbar_insert_div_tag)},
	{"MiscLink", NULL, N_("_Link"), NULL, NULL, G_CALLBACK(htmlbar_insert_link_tag)},
	{"MiscScript", NULL, N_("S_cript"), NULL, NULL, G_CALLBACK(htmlbar_insert_script_tag)},
	{"MiscAnchor", BF_STOCK_ANCHOR, N_("_Anchor"), NULL, NULL, G_CALLBACK(htmlbar_insert_anchor_tag)},
	{"MiscImage", BF_STOCK_IMAGE, N_("_Image"), NULL, NULL, G_CALLBACK(htmlbar_insert_image_tag)},
	{"TagsComment", BF_STOCK_COMMENT, N_("_Comment"), "<control><alt>C", N_("HTML Comment"),
	 G_CALLBACK(htmlbar_insert_html_comment)},
	{"DialogsMenu", NULL, N_("Dial_ogs")},
	{"DialogsGeneral", NULL, N_("_General")},
	{"DialogsQuickstart", BF_STOCK_QUICKSTART, N_("_Quickstart..."), "<shift><alt>q", N_("Quickstart..."),
	 G_CALLBACK(htmlbar_dialog_quickstart)},
	{"DialogsLinkHeader", NULL, N_("_Link Header..."), NULL, N_("Link Header..."),
	 G_CALLBACK(htmlbar_dialog_link_header)},
	{"DialogsBody", BF_STOCK_BODY, N_("_Body..."), "<shift><alt>B", N_("Body..."),
	 G_CALLBACK(htmlbar_dialog_body)},
	{"DialogsAnchor", BF_STOCK_ANCHOR, N_("_Anchor..."), "<shift><alt>a", N_("Anchor / Hyperlink..."),
	 G_CALLBACK(htmlbar_dialog_anchor)},
	{"DialogsEmail", BF_STOCK_EMAIL, N_("_E-mail..."), "<shift><alt>e", N_("E-mail..."),
	 G_CALLBACK(htmlbar_dialog_email)},
	{"DialogsRule", BF_STOCK_HRULE, N_("_Rule..."), "<shift><alt>r", N_("Horizontal Rule..."),
	 G_CALLBACK(htmlbar_dialog_rule)},
	{"DialogsFont", BF_STOCK_FONT, N_("_Font..."), "<shift><alt>f", N_("Font..."),
	 G_CALLBACK(htmlbar_dialog_font)},
	{"DialogsFontBase", BF_STOCK_FONT_BASE, N_("Basef_ont..."), NULL, N_("Base font..."),
	 G_CALLBACK(htmlbar_dialog_basefont)},
	{"DialogsQuicklist", BF_STOCK_LIST, N_("Quick_list..."), "<shift><alt>L", N_("Quick List..."),
	 G_CALLBACK(htmlbar_dialog_quicklist)},
	{"DialogsMeta", NULL, N_("_Meta..."), "<shift><alt>m", NULL, G_CALLBACK(htmlbar_dialog_meta)},
	{"DialogsEmbed", NULL, N_("Embe_d..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_embed)},
	{"DialogsColor", NULL, N_("Select _Color..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_color)},
	{"DialogsTime", NULL, N_("Insert _Time..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_time)},
	{"DialogsImage", BF_STOCK_IMAGE, N_("Insert _Image..."), "<shift><alt>I", N_("Insert image..."),
	 G_CALLBACK(htmlbar_dialog_image)},
	{"DialogsThumbnail", BF_STOCK_THUMBNAIL, N_("Insert T_humbnail..."), "<shift><alt>N",
	 N_("Insert thumbnail..."),
	 G_CALLBACK(htmlbar_dialog_thumbnail)},
	{"DialogsTableMenu", NULL, N_("_Table")},
	{"DialogsTableWizard", BF_STOCK_TABLE_WIZARD, N_("Table _Wizard..."), NULL, N_("Table Wizard..."),
	 G_CALLBACK(htmlbar_dialog_table_wizard)},
	{"DialogsTable", BF_STOCK_TABLE, N_("_Table..."), "<shift><alt>T", N_("Table..."),
	 G_CALLBACK(htmlbar_dialog_table)},
	{"DialogsTableRow", BF_STOCK_TABLE_TR, N_("Table _Row..."), NULL, N_("Table Row..."),
	 G_CALLBACK(htmlbar_dialog_table_row)},
	{"DialogsTableHead", BF_STOCK_TABLE_TH, N_("Table _Head..."), NULL, N_("Table Head..."),
	 G_CALLBACK(htmlbar_dialog_table_head)},
	{"DialogsTableData", BF_STOCK_TABLE_TD, N_("Table _Data..."), NULL, N_("Table Data..."),
	 G_CALLBACK(htmlbar_dialog_table_data)},
	{"DialogsCSSMenu", NULL, N_("_CSS")},
	{"DialogsCreateStyle", BF_STOCK_CSSNEWSTYLE, N_("_Create Style..."), "<shift><alt>S",
	 N_("Create stylesheet..."),
	 G_CALLBACK(htmlbar_dialog_style_create)},
	{"DialogsSpan", BF_STOCK_CSSSPAN, N_("S_pan..."), NULL, N_("Span..."), G_CALLBACK(htmlbar_dialog_span)},
	{"DialogsDiv", BF_STOCK_CSSDIV, N_("_Div..."), "<shift><alt>D", N_("Div..."),
	 G_CALLBACK(htmlbar_dialog_div)},
	{"DialogsStyle", BF_STOCK_CSSSTYLE, N_("_Style..."), NULL, N_("Style..."),
	 G_CALLBACK(htmlbar_dialog_style)},
	{"DialogsLinkStylesheet", BF_STOCK_LINK_STYLESHEET, N_("_Link to Stylesheet..."), NULL,
	 N_("Link to Stylesheet..."),
	 G_CALLBACK(htmlbar_dialog_style_link_to)},
	{"DialogsFrameMenu", NULL, N_("_Frame")},
	{"DialogsFrameWizard", BF_STOCK_FRAME_WIZARD, N_("Frame _Wizard..."), NULL, N_("Frame Wizard..."),
	 G_CALLBACK(htmlbar_dialog_frame_wizard)},
	{"DialogsFrameset", BF_STOCK_FRAMESET, N_("Frame_set..."), NULL, N_("Frameset..."),
	 G_CALLBACK(htmlbar_dialog_frameset)},
	{"DialogsFrame", BF_STOCK_FRAME, N_("_Frame..."), NULL, N_("Frame..."), G_CALLBACK(htmlbar_dialog_frame)},
	{"DialogsFormMenu", NULL, N_("F_orm")},
	{"DialogsForm", BF_STOCK_FORM, N_("F_orm..."), NULL, N_("Form..."), G_CALLBACK(htmlbar_dialog_form)},
	{"DialogsFormInput", NULL, N_("_Input..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_form_input)},
	{"DialogsFormButtons", BF_STOCK_FORM_SUBMIT, N_("Input Buttons..."), NULL, N_("Input Button..."),
	 G_CALLBACK(htmlbar_dialog_form_input_buttons)},
	{"DialogsFormText", BF_STOCK_FORM_TEXT, N_("Input Text..."), NULL, N_("Input Text..."),
	 G_CALLBACK(htmlbar_dialog_form_input_text)},
	{"DialogsFormTextarea", BF_STOCK_FORM_TEXTAREA, N_("Text_area..."), NULL, N_("Textarea..."),
	 G_CALLBACK(htmlbar_dialog_form_textarea)},
	{"DialogsFormSelect", BF_STOCK_FORM_SELECT, N_("_Select..."), NULL, N_("Select..."),
	 G_CALLBACK(htmlbar_dialog_form_select)},
	{"DialogsFormOption", BF_STOCK_FORM_OPTION, N_("O_ption..."), NULL, N_("Option..."),
	 G_CALLBACK(htmlbar_dialog_form_option)},
	{"DialogsFormOptionGroup", BF_STOCK_FORM_OPTIONGROUP, N_("Option _Group..."), NULL, N_("Option group..."),
	 G_CALLBACK(htmlbar_dialog_form_option_group)},
	{"DialogsFormButton", BF_STOCK_FORM_SUBMIT, N_("_Button..."), NULL, N_("Button..."),
	 G_CALLBACK(htmlbar_dialog_form_button)},
	{"DialogsEditTag", BF_STOCK_EDIT_TAG, N_("_Edit tag under cursor..."), "F3", NULL,
	 G_CALLBACK(htmlbar_edit_tag_under_cursor)},
	{"DialogsMultiThumbnail", BF_STOCK_MULTITHUMBS, N_("_Multi Thumbnail..."), NULL, N_("Multi thumbnail..."),
	 G_CALLBACK(htmlbar_dialog_multi_thumbnail)},
	{"Frame", BF_STOCK_FRAME2, N_("Frame"), NULL, N_("Frame"), G_CALLBACK(htmlbar_insert_frame_tag)},
	{"Frameset", BF_STOCK_FRAMESET2, N_("Frameset"), NULL, N_("Frameset"),
	 G_CALLBACK(htmlbar_insert_frameset_tag)},
	{"NoFrames", BF_STOCK_FRAME_NO, N_("Noframes"), NULL, N_("Noframes"),
	 G_CALLBACK(htmlbar_insert_noframes_tag)},
	{"FrameTarget", BF_STOCK_FRAME_BASE, N_("Target"), NULL, N_("Target"),
	 G_CALLBACK(htmlbar_insert_target_tag)},
	{"DialogsFormHidden", BF_STOCK_FORM_HIDDEN, N_("Input Hidden..."), NULL, N_("Input Hidden..."), G_CALLBACK(htmlbar_dialog_form_hidden)},
	{"DialogsFormRadioButton", BF_STOCK_FORM_RADIO, N_("Input Radio button..."), NULL,
	 N_("Input Radio Button..."), G_CALLBACK(htmlbar_dialog_form_radio_button)},
	{"DialogsFormCheckBox", BF_STOCK_FORM_CHECK, N_("Input Check box..."), NULL,
	 N_("Input Check Box..."), G_CALLBACK(htmlbar_dialog_form_check_box)}
};

void
htmlbar_menu_create(Thtmlbarwin * hbw)
{
	Tbfwin *bfwin = hbw->bfwin;

	GtkActionGroup *action_group;
	GError *error = NULL;

	static const GtkToggleActionEntry htmlbar_toggle_actions[] = {
		{"ViewHTMLToolbar", NULL, N_("_HTML Toolbar"), NULL, NULL, G_CALLBACK(htmlbar_toolbar_show_toogle),
		 TRUE},
	};

	action_group = gtk_action_group_new("htmlbarActions");
	gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE "_plugin_htmlbar");
	gtk_action_group_add_actions(action_group, htmlbar_actions, G_N_ELEMENTS(htmlbar_actions), bfwin);
	gtk_action_group_add_toggle_actions(action_group, htmlbar_toggle_actions,
										G_N_ELEMENTS(htmlbar_toggle_actions), hbw);
	gtk_ui_manager_insert_action_group(bfwin->uimanager, action_group, 0);
	g_object_unref(action_group);
	g_print("loading htmlbar menu from %s\n",HTMLBAR_MENU_UI);
	gtk_ui_manager_add_ui_from_file(bfwin->uimanager, HTMLBAR_MENU_UI, &error);
	if (error != NULL) {
		g_warning("building htmlbar plugin menu failed: %s", error->message);
		g_error_free(error);
	} else {
		Thtmlbarsession *hbs = g_hash_table_lookup(htmlbar_v.lookup, bfwin->session);

		if (hbs)
			bfwin_set_menu_toggle_item(action_group, "ViewHTMLToolbar", hbs->view_htmlbar);
		else
			DEBUG_MSG("htmlbar_build_menu, ERROR, no htmlbarsession in hasht %p for session %p!!?!?!?!?\n",
					  htmlbar_v.lookup, bfwin->session);
	}
}

void
htmlbar_toolbar_create(Thtmlbarwin * hbw)
{
	Tbfwin *bfwin = hbw->bfwin;

	GtkWidget *html_notebook;
	GtkWidget *toolbar;
	GError *error = NULL;

	html_notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(html_notebook), GTK_POS_TOP);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(html_notebook), TRUE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(html_notebook), TRUE);
	hbw->handlebox = gtk_handle_box_new();
	gtk_container_add(GTK_CONTAINER(hbw->handlebox), html_notebook);
	gtk_box_pack_start(GTK_BOX(bfwin->toolbarbox), hbw->handlebox, FALSE, FALSE, 0);
	g_print("loading htmlbar toolbar from %s\n",HTMLBAR_TOOLBAR_UI);
	gtk_ui_manager_add_ui_from_file(bfwin->uimanager, HTMLBAR_TOOLBAR_UI, &error);
	if (error != NULL) {
		g_warning("building html toolbar failed: %s", error->message);
		g_error_free(error);
	}

	toolbar = gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLStandardToolbar");
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), toolbar, gtk_label_new(_(" Standard ")));

	toolbar = gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLFontsToolbar");
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), toolbar, gtk_label_new(_(" Fonts ")));

	toolbar = gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLFormattingToolbar");
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), toolbar, gtk_label_new(_(" Formatting ")));

	toolbar = gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLTablesToolbar");
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), toolbar, gtk_label_new(_(" Tables ")));

	toolbar = gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLFramesToolbar");
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), toolbar, gtk_label_new(_(" Frames ")));

	toolbar = gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLFormsToolbar");
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), toolbar, gtk_label_new(_(" Forms ")));

	toolbar = gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLListToolbar");
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), toolbar, gtk_label_new(_(" List ")));

	toolbar = gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLCSSToolbar");
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), toolbar, gtk_label_new(_(" CSS ")));

	gtk_widget_show_all(hbw->handlebox);
}

void
htmlbar_register_stock_icons(void)
{
	static struct {
		const guint8 *data;
		gchar *stock_id;
	} htmlbar_stock_icons[] = {
			{pixmap_abbr, BF_STOCK_ADDR},
			{pixmap_acronym, BF_STOCK_ACRONYM},
			{pixmap_anchor, BF_STOCK_ANCHOR},
			{pixmap_body, BF_STOCK_BODY},
			{pixmap_bold, BF_STOCK_BOLD},
			{pixmap_brbreak, BF_STOCK_BRBREAK},
			{pixmap_breakall, BF_STOCK_BREAKALL},
			{pixmap_center, BF_STOCK_CENTER},
			{pixmap_cite, BF_STOCK_CITE},
			{pixmap_code, BF_STOCK_CODE},
			{pixmap_comment, BF_STOCK_COMMENT},
			{pixmap_context, BF_STOCK_CONTEXT},
			{pixmap_css_small, BF_STOCK_CSS_SMALL},
			{pixmap_cssdiv, BF_STOCK_CSSDIV},
			{pixmap_cssnewstyle, BF_STOCK_CSSNEWSTYLE},
			{pixmap_cssspan, BF_STOCK_CSSSPAN},
			{pixmap_cssstyle, BF_STOCK_CSSSTYLE},
			{pixmap_del, BF_STOCK_DEL},
			{pixmap_dfn, BF_STOCK_DFN},
			{pixmap_edit_tag, BF_STOCK_EDIT_TAG},
			{pixmap_email, BF_STOCK_EMAIL},
			{pixmap_font_base, BF_STOCK_FONT_BASE},
			{pixmap_font, BF_STOCK_FONT},
			{pixmap_fonth1, BF_STOCK_FONTH1},
			{pixmap_fonth2, BF_STOCK_FONTH2},
			{pixmap_fonth3, BF_STOCK_FONTH3},
			{pixmap_fonth4, BF_STOCK_FONTH4},
			{pixmap_fonth5, BF_STOCK_FONTH5},
			{pixmap_fonth6, BF_STOCK_FONTH6},
			{pixmap_fontm1, BF_STOCK_FONTM1},
			{pixmap_fontp1, BF_STOCK_FONTP1},
			{pixmap_fontpre, BF_STOCK_FONTPRE},
			{pixmap_fontsub, BF_STOCK_FONTSUB},
			{pixmap_fontsuper, BF_STOCK_FONTSUPER},
			{pixmap_form_check, BF_STOCK_FORM_CHECK},
			{pixmap_form_hidden, BF_STOCK_FORM_HIDDEN},
			{pixmap_form_option, BF_STOCK_FORM_OPTION},
			{pixmap_form_optiongroup, BF_STOCK_FORM_OPTIONGROUP},
			{pixmap_form_radio, BF_STOCK_FORM_RADIO},
			{pixmap_form_select, BF_STOCK_FORM_SELECT},
			{pixmap_form_submit, BF_STOCK_FORM_SUBMIT},
			{pixmap_form_text, BF_STOCK_FORM_TEXT},
			{pixmap_form_textarea, BF_STOCK_FORM_TEXTAREA},
			{pixmap_form, BF_STOCK_FORM},
			{pixmap_frame_base, BF_STOCK_FRAME_BASE},
			{pixmap_frame_no, BF_STOCK_FRAME_NO},
			{pixmap_frame, BF_STOCK_FRAME},
			{pixmap_frame2, BF_STOCK_FRAME2},
			{pixmap_frameset, BF_STOCK_FRAMESET},
			{pixmap_frameset, BF_STOCK_FRAMESET2},
			{pixmap_framewhiz, BF_STOCK_FRAME_WIZARD},
			{pixmap_headings, BF_STOCK_HEADINGS},
			{pixmap_hrule, BF_STOCK_HRULE},
			{pixmap_image, BF_STOCK_IMAGE},
			{pixmap_ins, BF_STOCK_INS},
			{pixmap_italic, BF_STOCK_ITALIC},
			{pixmap_kbd, BF_STOCK_KBD},
			{pixmap_linkstylesheet, BF_STOCK_LINK_STYLESHEET},
			{pixmap_list_dd, BF_STOCK_LIST_DD},
			{pixmap_list_dl, BF_STOCK_LIST_DL},
			{pixmap_list_dt, BF_STOCK_LIST_DT},
			{pixmap_list_li, BF_STOCK_LIST_LI},
			{pixmap_list_ol, BF_STOCK_LIST_OL},
			{pixmap_list_ul, BF_STOCK_LIST_UL},
			{pixmap_list, BF_STOCK_LIST},
			{pixmap_multithumbs, BF_STOCK_MULTITHUMBS},
			{pixmap_nbsp, BF_STOCK_NBSP},
			{pixmap_paragraph, BF_STOCK_PARAGRAPH},
			{pixmap_php3, BF_STOCK_PHP3},
			{pixmap_quick_start, BF_STOCK_QUICKSTART},
			{pixmap_right, BF_STOCK_RIGHT},
			{pixmap_samp, BF_STOCK_SAMP},
			{pixmap_strikeout, BF_STOCK_STRIKEOUT},
			{pixmap_table_capt, BF_STOCK_TABLE_CAPTION},
			{pixmap_table_td, BF_STOCK_TABLE_TD},
			{pixmap_table_td2, BF_STOCK_TABLE_TD2},
			{pixmap_table_th, BF_STOCK_TABLE_TH},
			{pixmap_table_th2, BF_STOCK_TABLE_TH2},
			{pixmap_table_tr, BF_STOCK_TABLE_TR},
			{pixmap_table_tr2, BF_STOCK_TABLE_TR2},
			{pixmap_table, BF_STOCK_TABLE},
			{pixmap_table2, BF_STOCK_TABLE2},
			{pixmap_tablewhiz, BF_STOCK_TABLE_WIZARD},
			{pixmap_thumbnail, BF_STOCK_THUMBNAIL},
			{pixmap_underline, BF_STOCK_UNDERLINE},
			{pixmap_var, BF_STOCK_VAR},
	};

	GtkIconFactory *icon_factory;
	GtkIconSet *icon_set;
	GtkIconSource *icon_source;
	/*gchar *filename;*/
	gint i;

	icon_factory = gtk_icon_factory_new();

	for (i = 0; i < G_N_ELEMENTS(htmlbar_stock_icons); i++) {
		GdkPixbuf *pixbuf;
		icon_set = gtk_icon_set_new();
		icon_source = gtk_icon_source_new();

		/*filename = g_strconcat(HTMLBAR_PIXMAP_DIR, htmlbar_stock_icons[i].filename, NULL);
		gtk_icon_source_set_filename(icon_source, filename);
		g_free(filename);*/
		pixbuf = gdk_pixbuf_new_from_inline(-1,htmlbar_stock_icons[i].data,FALSE,NULL);
		gtk_icon_source_set_pixbuf(icon_source, pixbuf);
		g_object_unref(pixbuf);

		gtk_icon_set_add_source(icon_set, icon_source);
		gtk_icon_source_free(icon_source);
		gtk_icon_factory_add(icon_factory, htmlbar_stock_icons[i].stock_id, icon_set);
		gtk_icon_set_unref(icon_set);
	}

	gtk_icon_factory_add_default(icon_factory);

	g_object_unref(icon_factory);
}
