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
#include "html_form.h"
#include "html_table.h"
#include "image.h"
#include "image_dialog.h"
#include "rpopup.h"
#include "quickstart.h"
#include "wizards.h"

/* temporary */
#include "htmlbar_gui.h"

/*#define DEBUG*/

/* For testing purposes */
#define HTMLBAR_MENU_UI "src/plugin_htmlbar/ui/htmlbar_menu_ui.xml"


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
	htmlbar_insert_special_char(BFWIN(user_data), ACUTE);
}

static void
htmlbar_insert_cedilla(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), CEDILLA);
}

static void
htmlbar_insert_umlaut(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), UMLAUT);
}

static void
htmlbar_insert_aacute(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), A_ACUTE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), A_ACUTE_LOWER);
}

static void
htmlbar_insert_acirc(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), A_CIRC_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), A_CIRC_LOWER);
}

static void
htmlbar_insert_aelig(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), A_ELIG_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), A_ELIG_LOWER);
}

static void
htmlbar_insert_agrave(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), A_GRAVE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), A_GRAVE_LOWER);
}

static void
htmlbar_insert_aring(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), A_RING_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), A_RING_LOWER);
}

static void
htmlbar_insert_atilde(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), A_TILDE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), A_TILDE_LOWER);
}

static void
htmlbar_insert_auml(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), A_UML_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), A_UML_LOWER);
}

static void
htmlbar_insert_ccedil(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), C_CEDIL_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), C_CEDIL_LOWER);
}

static void
htmlbar_insert_eacute(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), E_ACUTE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), E_ACUTE_LOWER);
}

static void
htmlbar_insert_ecirc(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), E_CIRC_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), E_CIRC_LOWER);
}

static void
htmlbar_insert_egrave(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), E_GRAVE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), E_GRAVE_LOWER);
}

static void
htmlbar_insert_euml(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), E_UML_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), E_UML_LOWER);
}

static void
htmlbar_insert_iacute(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), I_ACUTE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), I_ACUTE_LOWER);
}

static void
htmlbar_insert_icirc(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), I_CIRC_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), I_CIRC_LOWER);
}

static void
htmlbar_insert_igrave(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), I_GRAVE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), I_GRAVE_LOWER);
}

static void
htmlbar_insert_iuml(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), I_UML_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), I_UML_LOWER);
}

static void
htmlbar_insert_ntilde(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), N_TILDE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), N_TILDE_LOWER);
}

static void
htmlbar_insert_oacute(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), O_ACUTE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), O_ACUTE_LOWER);
}

static void
htmlbar_insert_ocirc(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), O_CIRC_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), O_CIRC_LOWER);
}

static void
htmlbar_insert_oelig(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), O_ELIG_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), O_ELIG_LOWER);
}

static void
htmlbar_insert_ograve(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), O_GRAVE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), O_GRAVE_LOWER);
}

static void
htmlbar_insert_oslash(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), O_SLASH_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), O_SLASH_LOWER);
}

static void
htmlbar_insert_otilde(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), O_TILDE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), O_TILDE_LOWER);
}

static void
htmlbar_insert_ouml(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), O_UML_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), O_UML_LOWER);
}

static void
htmlbar_insert_uacute(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), U_ACUTE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), U_ACUTE_LOWER);
}

static void
htmlbar_insert_ucirc(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), U_CIRC_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), U_CIRC_LOWER);
}

static void
htmlbar_insert_ugrave(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), U_GRAVE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), U_GRAVE_LOWER);
}

static void
htmlbar_insert_uuml(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), U_UML_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), U_UML_LOWER);
}

static void
htmlbar_insert_yacute(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), Y_ACUTE_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), Y_ACUTE_LOWER);
}

static void
htmlbar_insert_yuml(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), Y_UML_LOWER);
}

static void
htmlbar_insert_cent_sign(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), CENT);
}

static void
htmlbar_insert_currency(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), CURRENCY);
}

static void
htmlbar_insert_euro(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), EURO);
}

static void
htmlbar_insert_pound(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), POUND);
}

static void
htmlbar_insert_yen(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), YEN);
}

static void
htmlbar_insert_division(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), DIVISION);
}

static void
htmlbar_insert_logical_not(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), LOGICAL_NOT);
}

static void
htmlbar_insert_times(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), TIMES);
}

static void
htmlbar_insert_plus_minus(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), PLUS_MINUS);
}

static void
htmlbar_insert_less_than(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), LESS_THAN);
}

static void
htmlbar_insert_greater_than(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), GREATER_THAN);
}

static void
htmlbar_insert_superscript1(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), SUPERSCRIPT_1);
}

static void
htmlbar_insert_superscript2(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), SUPERSCRIPT_2);
}

static void
htmlbar_insert_superscript3(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), SUPERSCRIPT_3);
}

static void
htmlbar_insert_one_quarter(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), FRAC_QUARTER);
}

static void
htmlbar_insert_one_half(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), FRAC_HALF);
}

static void
htmlbar_insert_three_quarter(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), FRAC_THREE_QUARTER);
}

static void
htmlbar_insert_degree(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), DEGREE);
}

static void
htmlbar_insert_eth(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), ETH_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), ETH_LOWER);
}

static void
htmlbar_insert_thorn(GtkAction * action, gpointer user_data)
{
	if (g_str_has_prefix(gtk_action_get_name(action), "Upper"))
		htmlbar_insert_special_char(BFWIN(user_data), THORN_UPPER);
	else
		htmlbar_insert_special_char(BFWIN(user_data), THORN_LOWER);
}

static void
htmlbar_insert_sharp(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), SZLIG);
}

static void
htmlbar_insert_micro_sign(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), MIRCO);
}

static void
htmlbar_insert_nbsp(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), NBSP);
}

static void
htmlbar_insert_section_sign(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), SECTION);
}

static void
htmlbar_insert_copyright(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), COPYRIGHT);
}

static void
htmlbar_insert_left_angle(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), LEFT_ANGLE);
}

static void
htmlbar_insert_right_angle(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), RIGHT_ANGLE);
}

static void
htmlbar_insert_trademark(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), TRADEMARK);
}

static void
htmlbar_insert_inverted_exclamation(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), INVERTED_EXCLAMATION);
}

static void
htmlbar_insert_inverted_question_mark(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), INVERTED_QUESTION_MARK);
}

static void
htmlbar_insert_ordinal(GtkAction * action, gpointer user_data)
{
	if (g_str_has_suffix(gtk_action_get_name(action), "F"))
		htmlbar_insert_special_char(BFWIN(user_data), ORDINAL_F);
	else
		htmlbar_insert_special_char(BFWIN(user_data), ORDINAL_M);
}

static void
htmlbar_insert_paragraph(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), PARAGRAPH);
}

static void
htmlbar_insert_broken_bar(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BROKEN_BAR);
}

static void
htmlbar_insert_soft_hyphen(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), SOFT_HYPHEN);
}

static void
htmlbar_insert_spacing_macron(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), SPACING_MACRON);
}

static void
htmlbar_insert_middot(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), MIDDOT);
}

void
htmlbar_toolbar_show(Thtmlbarwin * hbw, gboolean show)
{
	if (show) {
		if (hbw->handlebox) {
			gtk_widget_show(hbw->handlebox);
		} else {
			htmlbar_toolbar(hbw);
		}
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

	static const GtkActionEntry htmlbar_actions[] = {
		{"TagsMenu", NULL, N_("T_ags")},
		{"HeadingsMenu", NULL, N_("_Headings")},
		{"HeadingsH1", NULL, N_("H_1"), "<control><alt>1", N_("Insert H1 tag"),
		 G_CALLBACK(htmlbar_insert_h1_tag)},
		{"HeadingsH2", NULL, N_("H_2"), "<control><alt>2", N_("Insert H2 tag"),
		 G_CALLBACK(htmlbar_insert_h2_tag)},
		{"HeadingsH3", NULL, N_("H_3"), "<control><alt>3", N_("Insert H3 tag"),
		 G_CALLBACK(htmlbar_insert_h3_tag)},
		{"HeadingsH4", NULL, N_("H_4"), "<control><alt>4", N_("Insert H4 tag"),
		 G_CALLBACK(htmlbar_insert_h4_tag)},
		{"HeadingsH5", NULL, N_("H_5"), "<control><alt>5", N_("Insert H5 tag"),
		 G_CALLBACK(htmlbar_insert_h5_tag)},
		{"HeadingsH6", NULL, N_("H_6"), "<control><alt>6", N_("Insert H6 tag"),
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
		{"FormatStrong", NULL, N_("_Strong"), "<control><alt>g", NULL, G_CALLBACK(htmlbar_insert_strong_tag)},
		{"FormatEmphasis", NULL, N_("_Emphasis"), "<control><alt>e", NULL,
		 G_CALLBACK(htmlbar_insert_emphasis_tag)},
		{"FormatDefine", NULL, N_("_Define"), NULL, NULL, G_CALLBACK(htmlbar_insert_define_tag)},
		{"FormatCode", NULL, N_("_Code"), NULL, NULL, G_CALLBACK(htmlbar_insert_code_tag)},
		{"FormatSample", NULL, N_("Sa_mple"), NULL, NULL, G_CALLBACK(htmlbar_insert_sample_tag)},
		{"FormatKeyboard", NULL, N_("_Keyboard"), NULL, NULL, G_CALLBACK(htmlbar_insert_keyboard_tag)},
		{"FormatVariable", NULL, N_("_Variable"), NULL, NULL, G_CALLBACK(htmlbar_insert_variable_tag)},
		{"FormatCitation", NULL, N_("Ci_tation"), NULL, NULL, G_CALLBACK(htmlbar_insert_citation_tag)},
		{"FormatAbbr", NULL, N_("_Abbreviation"), NULL, NULL, G_CALLBACK(htmlbar_insert_abbr_tag)},
		{"FormatAcronym", NULL, N_("Ac_ronym"), NULL, NULL, G_CALLBACK(htmlbar_insert_acronym_tag)},
		{"FormatInsert", NULL, N_("_Insert"), NULL, NULL, G_CALLBACK(htmlbar_insert_insert_tag)},
		{"FormatDelete", NULL, N_("De_lete"), NULL, NULL, G_CALLBACK(htmlbar_insert_delete_tag)},
		{"FormatLayoutMenu", NULL, N_("_Format by layout")},
		{"FormatBold", NULL, N_("_Bold"), "<control><alt>b", NULL, G_CALLBACK(htmlbar_insert_bold_tag)},
		{"FormatItalic", NULL, N_("_Italic"), "<control><alt>i", NULL, G_CALLBACK(htmlbar_insert_italic_tag)},
		{"FormatUnderline", NULL, N_("_Underline"), "<control><alt>u", NULL,
		 G_CALLBACK(htmlbar_insert_underline_tag)},
		{"FormatStrikeout", NULL, N_("_Strikeout"), "<control><alt>s", NULL,
		 G_CALLBACK(htmlbar_insert_strikeout_tag)},
		{"FormatSmall", NULL, N_("Sm_all"), NULL, NULL, G_CALLBACK(htmlbar_insert_small_tag)},
		{"FormatBig", NULL, N_("Bi_g"), NULL, NULL, G_CALLBACK(htmlbar_insert_big_tag)},
		{"FormatGeneralMenu", NULL, N_("Format _general")},
		{"FormatParagraph", NULL, N_("_Paragraph"), "<control><alt>p", NULL,
		 G_CALLBACK(htmlbar_insert_paragraph_tag)},
		{"FormatBreak", NULL, N_("_Break"), "<control><alt>k", NULL, G_CALLBACK(htmlbar_insert_break_tag)},
		{"FormatClearAll", NULL, N_("Break clear _all"), NULL, NULL, G_CALLBACK(htmlbar_insert_clearall_tag)},
		{"FormatNBSP", NULL, N_("_Non-Breaking Space"), "<control><alt>n", NULL,
		 G_CALLBACK(htmlbar_insert_nbsp_tag)},
		{"FormatFontSizePlus", NULL, N_("Font Si_ze +1"), "<control><alt>equal", NULL,
		 G_CALLBACK(htmlbar_insert_font_plus_tag)},
		{"FormatFontSizeMinus", NULL, N_("Font _Size -1"), "<control><alt>minus", NULL,
		 G_CALLBACK(htmlbar_insert_font_minus_tag)},
		{"FormatPre", NULL, N_("Preformatted _Text"), "<Control><Alt>f", NULL,
		 G_CALLBACK(htmlbar_insert_pre_tag)},
		{"FormatSubscript", NULL, N_("Su_bscript"), NULL, NULL, G_CALLBACK(htmlbar_insert_subscript_tag)},
		{"FormatSuperscript", NULL, N_("Su_perscript"), NULL, NULL,
		 G_CALLBACK(htmlbar_insert_superscript_tag)},
		{"FormatCenter", NULL, N_("_Center"), NULL, NULL, G_CALLBACK(htmlbar_insert_center_tag)},
		{"FormatAlignRight", NULL, N_("Align _right"), "<control><Alt>r", NULL,
		 G_CALLBACK(htmlbar_insert_align_right_tag)},
		{"TableMenu", NULL, N_("_Table")},
		{"TableTable", NULL, N_("_Table"), "<control><alt>t", NULL, G_CALLBACK(htmlbar_insert_table_tag)},
		{"TableRow", NULL, N_("Table _Row"), NULL, NULL, G_CALLBACK(htmlbar_insert_table_row_tag)},
		{"TableHeader", NULL, N_("Table _Header"), NULL, NULL, G_CALLBACK(htmlbar_insert_table_header_tag)},
		{"TableData", NULL, N_("Table _Data"), NULL, NULL, G_CALLBACK(htmlbar_insert_table_data_tag)},
		{"TableCaption", NULL, N_("Table _Caption"), NULL, NULL,
		 G_CALLBACK(htmlbar_insert_table_caption_tag)},
		{"ListMenu", NULL, N_("_List")},
		{"ListUnordered", NULL, N_("Unordered _List"), "<control><alt>L", NULL,
		 G_CALLBACK(htmlbar_insert_list_unordered_tag)},
		{"ListOrdered", NULL, N_("_Ordered List"), "<control><alt>O", NULL,
		 G_CALLBACK(htmlbar_insert_list_ordered_tag)},
		{"ListItem", NULL, N_("List Ite_m"), "<control><alt>M", NULL,
		 G_CALLBACK(htmlbar_insert_list_item_tag)},
		{"ListDefList", NULL, N_("De_finition List"), NULL, NULL,
		 G_CALLBACK(htmlbar_insert_definition_list_tag)},
		{"ListDefTerm", NULL, N_("Definition _Term"), NULL, NULL,
		 G_CALLBACK(htmlbar_insert_definition_term_tag)},
		{"ListDefinition", NULL, N_("_Definition"), NULL, NULL, G_CALLBACK(htmlbar_insert_definition_tag)},
		{"ListListMenu", NULL, N_("Men_u"), NULL, NULL, G_CALLBACK(htmlbar_insert_menu_tag)},
		{"FormMenu", NULL, N_("Fo_rm")},
		{"FormForm", NULL, N_("_Form"), NULL, NULL, G_CALLBACK(htmlbar_insert_form_tag)},
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
		{"MiscAnchor", NULL, N_("_Anchor"), NULL, NULL, G_CALLBACK(htmlbar_insert_anchor_tag)},
		{"MiscImage", NULL, N_("_Image"), NULL, NULL, G_CALLBACK(htmlbar_insert_image_tag)},
		{"TagsComment", NULL, N_("_Comment"), "<control><alt>C", NULL,
		 G_CALLBACK(htmlbar_insert_html_comment)},
		{"DialogsMenu", NULL, N_("Dial_ogs")},
		{"DialogsGeneral", NULL, N_("_General")},
		{"DialogQuickstart", NULL, N_("_Quickstart..."), "<shift><alt>q", NULL,
		 G_CALLBACK(htmlbar_dialog_quickstart)},
		{"DialogLinkHeader", NULL, N_("_Link Header..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_link_header)},
		{"DialogBody", NULL, N_("_Body..."), "<shift><alt>B", NULL, G_CALLBACK(htmlbar_dialog_body)},
		{"DialogAnchor", NULL, N_("_Anchor..."), "<shift><alt>a", NULL, G_CALLBACK(htmlbar_dialog_anchor)},
		{"DialogEmail", NULL, N_("_Email..."), "<shift><alt>e", NULL, G_CALLBACK(htmlbar_dialog_email)},
		{"DialogRule", NULL, N_("_Rule..."), "<shift><alt>r", NULL, G_CALLBACK(htmlbar_dialog_rule)},
		{"DialogFont", NULL, N_("_Font..."), "<shift><alt>f", NULL, G_CALLBACK(htmlbar_dialog_font)},
		{"DialogBasefont", NULL, N_("Basef_ont..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_basefont)},
		{"DialogQuicklist", NULL, N_("Quick_list..."), "<shift><alt>L", NULL,
		 G_CALLBACK(htmlbar_dialog_quicklist)},
		{"DialogMeta", NULL, N_("_Meta..."), "<shift><alt>m", NULL, G_CALLBACK(htmlbar_dialog_meta)},
		{"DialogEmbed", NULL, N_("Embe_d..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_embed)},
		{"DialogColor", NULL, N_("Select _Color..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_color)},
		{"DialogTime", NULL, N_("Insert _Time..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_time)},
		{"DialogImage", NULL, N_("Insert _Image..."), "<shift><alt>I", NULL,
		 G_CALLBACK(htmlbar_dialog_image)},
		{"DialogThumbnail", NULL, N_("Insert T_humbnail..."), "<shift><alt>N", NULL,
		 G_CALLBACK(htmlbar_dialog_thumbnail)},
		{"DialogsTableMenu", NULL, N_("_Table")},
		{"DialogsTableWizard", NULL, N_("Table _Wizard..."), NULL, NULL,
		 G_CALLBACK(htmlbar_dialog_table_wizard)},
		{"DialogsTable", NULL, N_("_Table..."), "<shift><alt>T", NULL, G_CALLBACK(htmlbar_dialog_table)},
		{"DialogsTableRow", NULL, N_("Table _Row..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_table_row)},
		{"DialogsTableHead", NULL, N_("Table _Head..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_table_head)},
		{"DialogsTableData", NULL, N_("Table _Data..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_table_data)},
		{"DialogsCSSMenu", NULL, N_("_CSS")},
		{"DialogsCreateStyle", NULL, N_("_Create Style..."), "<shift><alt>S", NULL,
		 G_CALLBACK(htmlbar_dialog_style_create)},
		{"DialogsSpan", NULL, N_("S_pan..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_span)},
		{"DialogsDiv", NULL, N_("_Div..."), "<shift><alt>D", NULL, G_CALLBACK(htmlbar_dialog_div)},
		{"DialogsStyle", NULL, N_("_Style..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_style)},
		{"DialogsLinkStylesheet", NULL, N_("_Link to Stylesheet..."), NULL, NULL,
		 G_CALLBACK(htmlbar_dialog_style_link_to)},
		{"DialogsFrameMenu", NULL, N_("_Frame")},
		{"DialogsFrameWizard", NULL, N_("Frame _Wizard..."), NULL, NULL,
		 G_CALLBACK(htmlbar_dialog_frame_wizard)},
		{"DialogsFrameset", NULL, N_("Frame_set..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_frameset)},
		{"DialogsFrame", NULL, N_("_Frame..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_frame)},
		{"DialogsFormMenu", NULL, N_("F_orm")},
		{"DialogsForm", NULL, N_("F_orm..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_form)},
		{"DialogsFormInput", NULL, N_("_Input..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_form_input)},
		{"DialogsFormButtons", NULL, N_("Input Buttons..."), NULL, NULL,
		 G_CALLBACK(htmlbar_dialog_form_input_buttons)},
		{"DialogsFormText", NULL, N_("Input Text..."), NULL, NULL,
		 G_CALLBACK(htmlbar_dialog_form_input_text)},
		{"DialogsFormTextarea", NULL, N_("Text_area..."), NULL, NULL,
		 G_CALLBACK(htmlbar_dialog_form_textarea)},
		{"DialogsFormSelect", NULL, N_("_Select..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_form_select)},
		{"DialogsFormOption", NULL, N_("O_ption..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_form_option)},
		{"DialogsFormOptionGroup", NULL, N_("Option _Group..."), NULL, NULL,
		 G_CALLBACK(htmlbar_dialog_form_option_group)},
		{"DialogsFormButton", NULL, N_("_Button..."), NULL, NULL, G_CALLBACK(htmlbar_dialog_form_button)},
		{"DialogsEditTag", NULL, N_("_Edit tag under cursor..."), "F3", NULL,
		 G_CALLBACK(htmlbar_edit_tag_under_cursor)}
	};

	action_group = gtk_action_group_new("htmlbarActions");
	gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE "_plugin_htmlbar");
	gtk_action_group_add_actions(action_group, htmlbar_actions, G_N_ELEMENTS(htmlbar_actions), bfwin);
	gtk_action_group_add_toggle_actions(action_group, htmlbar_toggle_actions,
										G_N_ELEMENTS(htmlbar_toggle_actions), hbw);
	gtk_ui_manager_insert_action_group(bfwin->uimanager, action_group, 0);
	g_object_unref(action_group);

	gtk_ui_manager_add_ui_from_file(bfwin->uimanager, HTMLBAR_MENU_UI, &error);
	if (error != NULL) {
		g_warning("building htmlbar plugin menu failed: %s", error->message);
		g_error_free(error);
	} else {
		Thtmlbarsession *hbs = g_hash_table_lookup(htmlbar_v.lookup, bfwin->session);

		if (hbs)
			setup_menu_toggle_item(action_group, "ViewHTMLToolbar", hbs->view_htmlbar);
		else
			DEBUG_MSG("htmlbar_build_menu, ERROR, no htmlbarsession in hasht %p for session %p!!?!?!?!?\n",
					  htmlbar_v.lookup, bfwin->session);
	}
}
