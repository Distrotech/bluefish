/*
 * Bluefish HTML Editor
 * htmlbar_uimanager.c
 *
 * Copyright (C) 2005-2012 Olivier Sessink
 * Copyright (C) 2011-2012 James Hayward
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
/*#define DEBUG*/

#include "htmlbar_uimanager.h"
#include "../bluefish.h"
#include "../bfwin_uimanager.h"
#include "../document.h"
#include "../stringlist.h"
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


#define HTMLBAR_PIXMAP_DIR PKGDATADIR"/plugins/htmlbar/pixmaps/"
#define HTMLBAR_MENU_UI	   PKGDATADIR"/plugins/htmlbar/ui/htmlbar_menu_ui.xml"
/*#define HTMLBAR_TOOLBAR_UI PKGDATADIR"/plugins/htmlbar/ui/htmlbar_toolbar_ui.xml"*/ /* menu and toolbar are merged into one file now */

#include "pixmaps/htmlbar_icons.c"

/* For testing purposes */
/*#define HTMLBAR_MENU_UI	"src/plugin_htmlbar/ui/htmlbar_menu_ui.xml"
*/


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
htmlbar_dialog_script(GtkAction * action, gpointer user_data)
{
	script_dialog(BFWIN(user_data), NULL);
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
htmlbar_dialog_columns(GtkAction * action, gpointer user_data)
{
	columns_dialog(BFWIN(user_data), NULL);
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
htmlbar_misc_style(GtkAction * action, gpointer user_data)
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
						   get_curlang_option_value(BFWIN(user_data), lang_is_XHTML) ? cap("<DIV STYLE=\"text-align: right\">") : cap("<DIV ALIGN=\"RIGHT\">"),
						   cap("</DIV>"));
}

static void
htmlbar_insert_anchor_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<A HREF=\""), cap("\"></A>"));
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
						   get_curlang_option_value(BFWIN(user_data), self_close_singleton_tags) ? cap("<br />") : cap("<br>"), NULL);
}

static void
htmlbar_insert_center_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document,
						   get_curlang_option_value(BFWIN(user_data), lang_is_XHTML) ? cap("<DIV STYLE=\"text-align: center\">") : cap("<DIV ALIGN=\"CENTER\">"),
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
						   get_curlang_option_value(BFWIN(user_data), self_close_singleton_tags) ? cap("<BR CLEAR=\"ALL\" />") : cap("<BR CLEAR=\"ALL\">"),
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
						   get_curlang_option_value(BFWIN(user_data), lang_is_XHTML) ? cap("<SPAN STYLE=\"font-size: smaller;\">") : cap("<FONT SIZE=\"-1\">")
						   , get_curlang_option_value(BFWIN(user_data), lang_is_XHTML) ? cap("</SPAN>") : cap("</FONT>"));
}

static void
htmlbar_insert_font_plus_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document,
						   get_curlang_option_value(BFWIN(user_data), lang_is_XHTML) ? cap("<SPAN STYLE=\"font-size: larger;\">") : cap("<FONT SIZE=\"+1\">")
						   , get_curlang_option_value(BFWIN(user_data), lang_is_XHTML) ? cap("</SPAN>") : cap("</FONT>"));
}

static void
htmlbar_insert_form_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<FORM>"), cap("</FORM>"));
}

static void
htmlbar_insert_form_fieldset_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<FIELDSET>"), cap("</FIELDSET>"));
}

static void
htmlbar_insert_form_legend_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<LEGEND>"), cap("<LEGEND/>"));
}

static void
htmlbar_insert_form_label_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<LABEL>"), cap("</LABEL>"));
}

static void
htmlbar_insert_form_input_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document,
						   get_curlang_option_value(BFWIN(user_data), self_close_singleton_tags) ? cap("<INPUT />") : cap("<INPUT>"), NULL);
}

static void
htmlbar_insert_form_button_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<BUTTON>"), cap("</BUTTON>"));
}

static void
htmlbar_insert_form_select_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<SELECT>"), cap("</SELECT>"));
}

static void
htmlbar_insert_form_datalist_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<DATALIST>"), cap("</DATALIST>"));
}

static void
htmlbar_insert_form_optgroup_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<OPTGROUP>"), cap("</OPTGROUP>"));
}

static void
htmlbar_insert_form_option_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<OPTION>"), cap("</OPTION>"));
}

static void
htmlbar_insert_form_textarea_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<TEXTAREA>"), cap("</TEXTAREA>"));
}

static void
htmlbar_insert_form_keygen_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document,
						   get_curlang_option_value(BFWIN(user_data), self_close_singleton_tags) ? cap("<KEYGEN />") : cap("<KEYGEN>"), NULL);
}

static void
htmlbar_insert_form_output_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<OUTPUT>"), cap("</OUTPUT>"));
}

static void
htmlbar_insert_form_progress_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<PROGRESS>"), cap("</PROGRESS>"));
}

static void
htmlbar_insert_form_meter_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<METER>"), cap("</METER>"));
}

static void
htmlbar_insert_frame_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document,
						   get_curlang_option_value(BFWIN(user_data), self_close_singleton_tags) ? cap("<FRAME />") : cap("<FRAME>"), NULL);
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
							 get_curlang_option_value(BFWIN(user_data), self_close_singleton_tags) ? "\" />\n" : "\">\n", NULL);
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
						   get_curlang_option_value(BFWIN(user_data), self_close_singleton_tags) ? cap("<IMG />") : cap("<IMG>"), NULL);
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
						   get_curlang_option_value(BFWIN(user_data), self_close_singleton_tags) ? cap("<LINK />") : cap("<LINK>"), NULL);
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
						   get_curlang_option_value(BFWIN(user_data), self_close_singleton_tags) ? cap("<BASE TARGET=\"\" />") : cap("<BASE TARGET=\"\">"), NULL);
}

static void
htmlbar_insert_underline_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document,
						   get_curlang_option_value(BFWIN(user_data), lang_is_XHTML) ? cap("<SPAN STYLE=\"text-decoration: underline;\">") : cap("<U>")
						   , get_curlang_option_value(BFWIN(user_data), lang_is_XHTML) ? cap("</SPAN>") : cap("</U>"));
}

static void
htmlbar_insert_variable_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<VAR>"), cap("</VAR>"));
}

static void
htmlbar_insert_nbsp(GtkAction * action, gpointer user_data)
{
	htmlbar_insert_special_char(BFWIN(user_data), BF_NBSP);
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

void
htmlbar_toolbar_show(Thtmlbarwin * hbw, Thtmlbarsession *hbs, gboolean show)
{
	if (htmlbar_v.in_sidepanel) {
		DEBUG_MSG("htmlbar_toolbar_show, in_sidepanel=%d, nothing to do (handlebox=%p), return\n",htmlbar_v.in_sidepanel,hbw->handlebox);
		return;
	}
	DEBUG_MSG("htmlbar_toolbar_show, show=%d, handlebox=%p\n",show,hbw->handlebox);
	if (show) {
		if (hbw->handlebox) {
			gtk_widget_show(hbw->handlebox);
		} else {
			GtkWidget *html_notebook;
			html_notebook = htmlbar_toolbar_create(hbw, hbs);
#if GTK_CHECK_VERSION(3,2,0)
			/*gtk_notebook_set_tab_pos(GTK_NOTEBOOK(html_notebook), GTK_POS_TOP); this is the default, so why set it?*/
 			gtk_widget_set_hexpand(GTK_WIDGET(html_notebook),TRUE);
 			hbw->handlebox = gtk_event_box_new();
 			gtk_widget_set_name(GTK_WIDGET(hbw->handlebox), "html_notebook_event_box");
			gtk_container_add(GTK_CONTAINER(hbw->handlebox),html_notebook);
			gtk_container_add(GTK_CONTAINER(hbw->bfwin->toolbarbox),hbw->handlebox);
			gtk_widget_show_all(hbw->handlebox);
#else
			hbw->handlebox = gtk_handle_box_new();
			gtk_container_add(GTK_CONTAINER(hbw->handlebox), html_notebook);
			gtk_notebook_set_tab_pos(GTK_NOTEBOOK(html_notebook), GTK_POS_TOP);
			gtk_box_pack_start(GTK_BOX(hbw->bfwin->toolbarbox), hbw->handlebox, FALSE, FALSE, 0);
			gtk_widget_show_all(hbw->handlebox);
#endif
		DEBUG_MSG("htmlbar_toolbar_show, created handlebox=%p\n",hbw->handlebox);
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
	if (!hbs)
		return;
	hbs->view_htmlbar = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	htmlbar_toolbar_show(hbw, hbs, gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));
}

static void
htmlbar_insert_article_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<ARTICLE>"), cap("</ARTICLE>"));
}

static void
htmlbar_insert_aside_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<ASIDE>"), cap("</ASIDE>"));
}

static void
htmlbar_insert_figcaption_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<FIGCAPTION>"), cap("</FIGCAPTION>"));
}

static void
htmlbar_insert_figure_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<FIGURE>"), cap("</FIGURE>"));
}

static void
htmlbar_insert_footer_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<FOOTER>"), cap("</FOOTER>"));
}

static void
htmlbar_insert_header_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<HEADER>"), cap("</HEADER>"));
}

static void
htmlbar_insert_hgroup_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<HGROUP>"), cap("</HGROUP>"));
}

static void
htmlbar_insert_mark_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<MARK>"), cap("</MARK>"));
}

static void
htmlbar_insert_nav_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<NAV>"), cap("</NAV>"));
}

static void
htmlbar_insert_rp_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<RP>"), cap("</RP>"));
}

static void
htmlbar_insert_rt_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<RT>"), cap("</RT>"));
}

static void
htmlbar_insert_ruby_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<RUBY>"), cap("</RUBY>"));
}

static void
htmlbar_insert_section_tag(GtkAction * action, gpointer user_data)
{
	doc_insert_two_strings(BFWIN(user_data)->current_document, cap("<SECTION>"), cap("</SECTION>"));
}

static void
htmlbar_dialog_audio(GtkAction * action, gpointer user_data)
{
	audio_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_canvas(GtkAction * action, gpointer user_data)
{
	canvas_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_video(GtkAction * action, gpointer user_data)
{
	video_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_dialog_html5time(GtkAction * action, gpointer user_data)
{
	html5time_dialog(BFWIN(user_data), NULL);
}

static void
htmlbar_insert_color(GtkAction * action, gpointer user_data)
{
	gchar *color = return_color(NULL);
	doc_insert_two_strings(BFWIN(user_data)->current_document,color, NULL);
	g_free(color);
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
	{"SpecialMenu", NULL, N_("_Entities")},
	{"NBSP", NULL, N_("_Non-breaking space"), NULL, NULL, G_CALLBACK(htmlbar_insert_nbsp)},
	{"BrokenBar", NULL, N_("_Broken bar ¦"), NULL, NULL, G_CALLBACK(htmlbar_insert_broken_bar)},
	{"SoftHyphen", NULL, N_("Soft _hyphen -"), NULL, NULL, G_CALLBACK(htmlbar_insert_soft_hyphen)},
	{"FormatContextMenu", NULL, N_("_Format by Context")},
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
	{"FormatLayoutMenu", NULL, N_("Format by L_ayout")},
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
	{"FormatGeneralMenu", NULL, N_("Format _General")},
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
	{"FormMenu", NULL, N_("F_orm")},
	{"FormForm", BF_STOCK_FORM, N_("F_orm"), NULL, N_("Form"), G_CALLBACK(htmlbar_insert_form_tag)},
	{"FormFieldset", NULL, N_("_Fieldset"), NULL, NULL, G_CALLBACK(htmlbar_insert_form_fieldset_tag)},
	{"FormLegend", NULL, N_("_Legend"), NULL, NULL, G_CALLBACK(htmlbar_insert_form_legend_tag)},
	{"FormLabel", NULL, N_("L_abel"), NULL, NULL, G_CALLBACK(htmlbar_insert_form_label_tag)},
	{"FormInput", NULL, N_("_Input"), NULL, NULL, G_CALLBACK(htmlbar_insert_form_input_tag)},
	{"FormButton", NULL, N_("_Button"), NULL, NULL, G_CALLBACK(htmlbar_insert_form_button_tag)},
	{"FormSelect", NULL, N_("_Select"), NULL, NULL, G_CALLBACK(htmlbar_insert_form_select_tag)},
	{"FormDatalist", NULL, N_("_Datalist"), NULL, NULL, G_CALLBACK(htmlbar_insert_form_datalist_tag)},
	{"FormOptgroup", NULL, N_("Opt_group"), NULL, NULL, G_CALLBACK(htmlbar_insert_form_optgroup_tag)},
	{"FormOption", NULL, N_("Optio_n"), NULL, NULL, G_CALLBACK(htmlbar_insert_form_option_tag)},
	{"FormTextArea", NULL, N_("_Textarea"), NULL, NULL, G_CALLBACK(htmlbar_insert_form_textarea_tag)},
	{"FormKeygen", NULL, N_("_Keygen"), NULL, NULL, G_CALLBACK(htmlbar_insert_form_keygen_tag)},
	{"FormOutput", NULL, N_("O_utput"), NULL, NULL, G_CALLBACK(htmlbar_insert_form_output_tag)},
	{"FormProgress", NULL, N_("_Progress"), NULL, NULL, G_CALLBACK(htmlbar_insert_form_progress_tag)},
	{"FormMeter", NULL, N_("_Meter"), NULL, NULL, G_CALLBACK(htmlbar_insert_form_meter_tag)},
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
	{"DialogsScript", BF_STOCK_SCRIPT, N_("_Script..."), "<shift><alt>P", N_("Script..."),
	 G_CALLBACK(htmlbar_dialog_script)},
	{"DialogsAnchor", BF_STOCK_ANCHOR, N_("_Anchor..."), "<shift><alt>A", N_("Anchor / Hyperlink..."),
	 G_CALLBACK(htmlbar_dialog_anchor)},
	{"DialogsEmail", BF_STOCK_EMAIL, N_("_E-mail..."), "<shift><alt>E", N_("E-mail..."),
	 G_CALLBACK(htmlbar_dialog_email)},
	{"DialogsRule", BF_STOCK_HRULE, N_("_Rule..."), "<shift><alt>R", N_("Horizontal Rule..."),
	 G_CALLBACK(htmlbar_dialog_rule)},
	{"DialogsFont", BF_STOCK_FONT, N_("_Font..."), "<shift><alt>F", N_("Font..."),
	 G_CALLBACK(htmlbar_dialog_font)},
	{"DialogsFontBase", BF_STOCK_FONT_BASE, N_("Basef_ont..."), NULL, N_("Base font..."),
	 G_CALLBACK(htmlbar_dialog_basefont)},
	{"DialogsQuicklist", BF_STOCK_LIST, N_("Quick_list..."), "<shift><alt>L", N_("Quick List..."),
	 G_CALLBACK(htmlbar_dialog_quicklist)},
	{"DialogsMeta", NULL, N_("_Meta..."), "<shift><alt>M", NULL, G_CALLBACK(htmlbar_dialog_meta)},
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
	{"DialogsCreateStyle", BF_STOCK_CSSNEWSTYLE, N_("_Create Style..."), "<control><shift><alt>S",
	 N_("Create stylesheet..."),
	 G_CALLBACK(htmlbar_dialog_style_create)},
	{"DialogsSpan", BF_STOCK_CSSSPAN, N_("S_pan..."), NULL, N_("Span..."), G_CALLBACK(htmlbar_dialog_span)},
	{"DialogsDiv", BF_STOCK_CSSDIV, N_("_Div..."), "<shift><alt>D", N_("Div..."),
	 G_CALLBACK(htmlbar_dialog_div)},
	{"MiscStyle", BF_STOCK_CSSSTYLE, N_("_Style"), NULL, N_("Style"),
	 G_CALLBACK(htmlbar_misc_style)},
	{"Dialogscolumns", BF_STOCK_CSS_COLUMNS, N_("C_olumns..."), NULL, N_("Columns..."),
	 G_CALLBACK(htmlbar_dialog_columns)},
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
	{"DialogsFormButtons", BF_STOCK_FORM_SUBMIT, N_("Input B_uttons..."), NULL, N_("Input Button..."),
	 G_CALLBACK(htmlbar_dialog_form_input_buttons)},
	{"DialogsFormText", BF_STOCK_FORM_TEXT, N_("Input T_ext..."), NULL, N_("Input Text..."),
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
	 N_("Input Check Box..."), G_CALLBACK(htmlbar_dialog_form_check_box)},
	{"HTML5Menu", NULL, N_("HTML_5")},
	{"FormatArticle", BF_STOCK_ARTICLE, N_("_Article"), NULL, N_("Article"), G_CALLBACK(htmlbar_insert_article_tag)},
	{"FormatAside", BF_STOCK_ASIDE, N_("As_ide"), NULL, N_("Aside"), G_CALLBACK(htmlbar_insert_aside_tag)},
	{"FormatFigcaption", BF_STOCK_FIGCAPTION, N_("Fig_caption"), NULL, N_("Figcaption"), G_CALLBACK(htmlbar_insert_figcaption_tag)},
	{"FormatFigure", BF_STOCK_FIGURE, N_("Fig_ure"), NULL, N_("Figure"), G_CALLBACK(htmlbar_insert_figure_tag)},
	{"FormatFooter", BF_STOCK_FOOTER, N_("_Footer"), NULL, N_("Footer"), G_CALLBACK(htmlbar_insert_footer_tag)},
	{"FormatHeader", BF_STOCK_HEADER, N_("_Header"), NULL, N_("Header"), G_CALLBACK(htmlbar_insert_header_tag)},
	{"FormatHgroup", BF_STOCK_HGROUP, N_("H_group"), NULL, N_("Hgroup"), G_CALLBACK(htmlbar_insert_hgroup_tag)},
	{"FormatMark", BF_STOCK_MARK, N_("_Mark"), NULL, N_("Mark"), G_CALLBACK(htmlbar_insert_mark_tag)},
	{"FormatNav", BF_STOCK_NAV, N_("_Nav"), NULL, N_("Nav"), G_CALLBACK(htmlbar_insert_nav_tag)},
	{"FormatRp", BF_STOCK_RP, N_("R_p"), NULL, N_("Ruby parenthesis"), G_CALLBACK(htmlbar_insert_rp_tag)},
	{"FormatRt", BF_STOCK_RT, N_("R_t"), NULL, N_("Ruby text"), G_CALLBACK(htmlbar_insert_rt_tag)},
	{"FormatRuby", BF_STOCK_RUBY, N_("_Ruby"), NULL, N_("Ruby"), G_CALLBACK(htmlbar_insert_ruby_tag)},
	{"FormatSection", BF_STOCK_SECTION, N_("_Section"), NULL, N_("Section"), G_CALLBACK(htmlbar_insert_section_tag)},
	{"DialogsHTML5Menu", NULL, N_("HTML_5")},
	{"DialogsAudio", BF_STOCK_AUDIO, N_("Au_dio..."), NULL, N_("Audio..."), G_CALLBACK(htmlbar_dialog_audio)},
	{"DialogsVideo", BF_STOCK_VIDEO, N_("_Video..."), NULL, N_("Video..."), G_CALLBACK(htmlbar_dialog_video)},
	{"DialogsCanvas", BF_STOCK_CANVAS, N_("_Canvas..."), NULL, N_("Canvas..."), G_CALLBACK(htmlbar_dialog_canvas)},
	{"DialogsHtml5Time", BF_STOCK_HTML5TIME, N_("_Time..."), NULL, N_("Time..."), G_CALLBACK(htmlbar_dialog_html5time)},
	{"InsertColor", GTK_STOCK_SELECT_COLOR, N_("Insert _Color"), NULL, N_("Insert Color"), G_CALLBACK(htmlbar_insert_color)},
};

void
htmlbar_menu_create(Thtmlbarwin * hbw)
{
	Tbfwin *bfwin = hbw->bfwin;
	Thtmlbarsession *hbs;

	static const GtkToggleActionEntry htmlbar_toggle_actions[] = {
		{"ViewHTMLToolbar", NULL, N_("_HTML Toolbar"), NULL, NULL, G_CALLBACK(htmlbar_toolbar_show_toogle),
		 TRUE},
	};

	hbw->actiongroup = gtk_action_group_new("htmlbarActions");
	gtk_action_group_set_translation_domain(hbw->actiongroup, GETTEXT_PACKAGE "_plugin_htmlbar");
	gtk_action_group_add_actions(hbw->actiongroup, htmlbar_actions, G_N_ELEMENTS(htmlbar_actions), bfwin);
	gtk_action_group_add_toggle_actions(hbw->actiongroup, htmlbar_toggle_actions,
										G_N_ELEMENTS(htmlbar_toggle_actions), hbw);
	gtk_ui_manager_insert_action_group(bfwin->uimanager, hbw->actiongroup, 0);
	g_object_unref(hbw->actiongroup);
	hbs = g_hash_table_lookup(htmlbar_v.lookup, bfwin->session);
	if (hbs)
		bfwin_set_menu_toggle_item(hbw->actiongroup, "ViewHTMLToolbar", hbs->view_htmlbar);

}


static GtkWidget *quickbar_create_popup_menu(gboolean add, const gchar *actionname);

static gboolean
quickbar_button_press_event_lcb(GtkWidget *widget,GdkEvent  *event,gpointer   user_data)
{
	if (event->button.button == 3) {
		GtkWidget *p, *menu;
		GtkAction *action;
		p = gtk_widget_get_parent(widget);
		if (!p)
			return FALSE;
		action = gtk_activatable_get_related_action(GTK_ACTIVATABLE(p));
		if (!action)
			return FALSE;
		menu = quickbar_create_popup_menu(FALSE, gtk_action_get_name(action));
		gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,3,event->button.time);
		return TRUE;
	}
	return FALSE;
}


static void
htmlbar_quickbar_add_item(Thtmlbarwin * hbw, const gchar *actionname)
{
	GtkWidget *toolitem;
	GtkAction *action;
	GList *children2, *tmplist2;
	action = gtk_action_group_get_action(hbw->actiongroup, actionname);
	if (!action)
		return;
	toolitem = gtk_action_create_tool_item(action);
	if (!toolitem)
		return;
	children2 = gtk_container_get_children(GTK_CONTAINER(toolitem));
	for (tmplist2=g_list_first(children2);tmplist2;tmplist2=g_list_next(tmplist2)) {
		/*g_print("have child-child %p of type %s\n", tmplist2->data, G_OBJECT_TYPE_NAME(tmplist2->data));*/
		g_signal_connect(tmplist2->data, "button-press-event", G_CALLBACK(quickbar_button_press_event_lcb), hbw);
	}
	g_list_free(children2);

	gtk_toolbar_insert(GTK_TOOLBAR(hbw->quickbar_toolbar), GTK_TOOL_ITEM(toolitem), -1);
}

static void
add_to_quickbar_activate_lcb(GtkMenuItem *m, gpointer data)
{
	GList *tmplist;
	gboolean firstitem;
	/* first check if it is not a duplicate */
	if (find_in_stringlist(htmlbar_v.quickbar_items, (gchar *)data)) {
		DEBUG_MSG("add_to_quickbar_activate_lcb, duplicate entry for %s\n",(gchar *)data);
		return;
	}
	firstitem = (htmlbar_v.quickbar_items==NULL);

	DEBUG_MSG("add_to_quickbar_activate_lcb, adding %s to quickbar_items\n", (gchar *)data);
	htmlbar_v.quickbar_items = g_list_append(htmlbar_v.quickbar_items, g_strdup(data));
	/* now loop over all the windows that have a htmlbar, and add the item */
	for (tmplist=g_list_first(main_v->bfwinlist);tmplist;tmplist=g_list_next(tmplist)) {
		Thtmlbarwin * hbw;
		hbw = g_hash_table_lookup(htmlbar_v.lookup, tmplist->data);
		if (hbw) {
			if (firstitem) {
				GList *children = gtk_container_get_children(GTK_CONTAINER(hbw->quickbar_toolbar));
				if (children)
					gtk_container_remove(GTK_CONTAINER(hbw->quickbar_toolbar), children->data);
			}

			htmlbar_quickbar_add_item(hbw, (gchar *)data);
		}
	}
}

static void
htmlbar_quickbar_remove_item(Thtmlbarwin * hbw, const gchar *actionname)
{
	GList *children, *tmplist;
	children = gtk_container_get_children(GTK_CONTAINER(hbw->quickbar_toolbar));
	for (tmplist=g_list_first(children);tmplist;tmplist=g_list_next(tmplist)) {
/*		g_print("have child %p of type %s\n", tmplist->data, G_OBJECT_TYPE_NAME(tmplist->data));*/
		GtkAction *action = gtk_activatable_get_related_action(tmplist->data);
		if (action && strcmp(gtk_action_get_name(action), actionname)==0) {
			gtk_container_remove(GTK_CONTAINER(hbw->quickbar_toolbar), tmplist->data);
			return;
		}
		/*g_print("action=%p %s\n", action, action? gtk_action_get_name(action): "NULL");*/
	}
	g_list_free(children);
}

static void
remove_from_quickbar_activate_lcb(GtkMenuItem *m, gpointer data)
{
	GList *tmplist;

	/* now loop over all the windows that have a htmlbar, and remove the item */
	for (tmplist=g_list_first(main_v->bfwinlist);tmplist;tmplist=g_list_next(tmplist)) {
		Thtmlbarwin * hbw;
		hbw = g_hash_table_lookup(htmlbar_v.lookup, tmplist->data);
		if (hbw) {
			htmlbar_quickbar_remove_item(hbw, (const gchar *)data);
		}
	}
	DEBUG_MSG("remove_from_quickbar_activate_lcb, removing %s from quickbar_items\n", (gchar *)data);
	htmlbar_v.quickbar_items = remove_from_stringlist(htmlbar_v.quickbar_items, data);
}

static GtkWidget *
quickbar_create_popup_menu(gboolean add, const gchar *actionname)
{
	GtkWidget *menu, *menuitem;
	menu = gtk_menu_new();
	menuitem = gtk_menu_item_new_with_label(add ? _("Add to Quickbar") : _("Remove from Quickbar"));
	g_signal_connect(G_OBJECT(menuitem), "activate", add ? G_CALLBACK(add_to_quickbar_activate_lcb) : G_CALLBACK(remove_from_quickbar_activate_lcb), (gpointer)actionname);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_show_all(menu);
	return menu;
}

static gboolean
toolbar_button_press_event_lcb(GtkWidget *widget,GdkEvent  *event,gpointer   user_data)
{
	/*Thtmlbarwin * hbw=user_data;*/
	DEBUG_MSG("toolbar_button_press_event_lcb, called for widget %p\n",widget);
	if (event->button.button == 3) {
		GtkWidget *p, *menu;
		GtkAction *action;
		p = gtk_widget_get_parent(widget);
		if (!p)
			return FALSE;
		action = gtk_activatable_get_related_action(GTK_ACTIVATABLE(p));
		if (!action)
			return FALSE;
		DEBUG_MSG("toolbar_button_press_event_lcb, add action %s \n", gtk_action_get_name(action));

		menu = quickbar_create_popup_menu(TRUE, gtk_action_get_name(action));
		gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,3,event->button.time);
/*		htmlbar_v.quickbar_items = g_list_prepend(htmlbar_v.quickbar_items, g_strdup(gtk_action_get_name(action)));*/
		return TRUE;
	}
	return FALSE;
}

static void
htmlbar_load_quickbar(Thtmlbarwin * hbw, GtkWidget *toolbar)
{
	GList *tmplist;
	if (htmlbar_v.quickbar_items == NULL) {
		GtkWidget *label;
		GtkToolItem *ti = gtk_tool_item_new();
		label = gtk_label_new(_("Right click any html toolbar button to add it to the Quickbar."));
		if (htmlbar_v.in_sidepanel) {
			gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
		}
		gtk_container_add(GTK_CONTAINER(ti), label);
		gtk_toolbar_insert(GTK_TOOLBAR(toolbar),ti,0);
	}
	for (tmplist=g_list_first(htmlbar_v.quickbar_items);tmplist;tmplist=g_list_next(tmplist)) {
		htmlbar_quickbar_add_item(hbw, tmplist->data);
	}
}

static void
setup_items_for_quickbar(Thtmlbarwin * hbw, GtkWidget *toolbar)
{
	GList *children, *tmplist, *children2, *tmplist2;
	DEBUG_MSG("setup_items_for_quickbar, about to connect signals to toolbar buttons\n");
	children = gtk_container_get_children(GTK_CONTAINER(toolbar));
	for (tmplist=g_list_first(children);tmplist;tmplist=g_list_next(tmplist)) {
		/*g_print("have child %p of type %s\n", tmplist->data, G_OBJECT_TYPE_NAME(tmplist->data));*/
		GtkAction *action = gtk_activatable_get_related_action(tmplist->data);
		if (action) {
			children2 = gtk_container_get_children(GTK_CONTAINER(tmplist->data));
			for (tmplist2=g_list_first(children2);tmplist2;tmplist2=g_list_next(tmplist2)) {
				/*g_print("have child-child %p of type %s\n", tmplist2->data, G_OBJECT_TYPE_NAME(tmplist2->data));*/
				g_signal_connect(tmplist2->data, "button-press-event", G_CALLBACK(toolbar_button_press_event_lcb), hbw);
			}
			g_list_free(children2);
		}
		/*g_print("action=%p %s\n", action, action? gtk_action_get_name(action): "NULL");*/
	}
	g_list_free(children);
}

static void
notebook_switch_page_lcb(GtkNotebook *notebook,GtkWidget   *page,guint        page_num,gpointer     user_data)
{
	Thtmlbarwin * hbw = user_data;
	Thtmlbarsession *hbs;
	hbs = g_hash_table_lookup(htmlbar_v.lookup, hbw->bfwin->session);
	if (hbs) {
		DEBUG_MSG("htmlbar notebook_switch_page_lcb, set page %d\n", page_num);
		hbs->notebooktab = page_num;
	}
}

static GtkWidget *new_html_subtoolbar(Thtmlbarwin * hbw, GtkWidget *html_notebook, GtkWidget *toolbar, const gchar *labeltext)
{
	GtkWidget *label;
	GtkWidget *parent;
	parent = gtk_widget_get_parent(toolbar);
	DEBUG_MSG("new_html_subtoolbar, setup toolbar(parent=%p) %p for %s\n",parent, toolbar,labeltext);
	if (parent) {
		g_object_ref(toolbar);
		gtk_container_remove(GTK_CONTAINER(parent), toolbar);
	}


	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
#ifdef MAC_INTEGRATION
#if GTK_CHECK_VERSION(3,4,0)
	gtk_toolbar_set_icon_size (GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);
#endif
#endif
	DEBUG_MSG("new_html_subtoolbar, setup orientation need_vertical=%d,need_horizontal=%d\n",htmlbar_v.in_sidepanel,!htmlbar_v.in_sidepanel);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(toolbar), htmlbar_v.in_sidepanel?GTK_ORIENTATION_VERTICAL:GTK_ORIENTATION_HORIZONTAL);
	label = gtk_label_new(labeltext);
	DEBUG_MSG("new_html_subtoolbar, append toolbar to html_notebook\n");
	gtk_notebook_append_page(GTK_NOTEBOOK(html_notebook), toolbar, label);
	gtk_container_child_set(GTK_CONTAINER(html_notebook), label, "tab-fill", TRUE, "tab-expand", TRUE, NULL);
	DEBUG_MSG("new_html_subtoolbar, return toolbar=%p for %s\n", toolbar, labeltext);
	if (parent) {
		g_object_unref(toolbar);
	}
	return toolbar;
}

GtkWidget *
htmlbar_toolbar_create(Thtmlbarwin * hbw, Thtmlbarsession *hbs)
{
	Tbfwin *bfwin = hbw->bfwin;

	GtkWidget *html_notebook;
	GtkWidget *toolbar;
	GtkWidget *menuitem;

	html_notebook = gtk_notebook_new();
	DEBUG_MSG("htmlbar_toolbar_create, created notebook %p\n",html_notebook);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(html_notebook), TRUE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(html_notebook), TRUE);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(html_notebook), TRUE);

	hbw->quickbar_toolbar = new_html_subtoolbar(hbw, html_notebook, gtk_toolbar_new(), _("Quickbar"));
	htmlbar_load_quickbar(hbw, hbw->quickbar_toolbar);
	DEBUG_MSG("quickbar done\n");
	toolbar = new_html_subtoolbar(hbw, html_notebook, gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLStandardToolbar"), _("Standard"));

	if (main_v->props.format_by_context) {
		gtk_widget_destroy(gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLStandardToolbar/FormatStrong"));
		gtk_widget_destroy(gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLStandardToolbar/FormatEmphasis"));
	} else {
		gtk_widget_destroy(gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLStandardToolbar/FormatBold"));
		gtk_widget_destroy(gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLStandardToolbar/FormatItalic"));
	}

	DEBUG_MSG("standard created\n");
	setup_items_for_quickbar(hbw, toolbar);
	DEBUG_MSG("quickbar setup done\n");

	toolbar = new_html_subtoolbar(hbw, html_notebook, gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLHTML5Toolbar"), _("HTML 5"));
	setup_items_for_quickbar(hbw, toolbar);

	toolbar = new_html_subtoolbar(hbw, html_notebook, gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLFormattingToolbar"), _("Formatting"));
	setup_items_for_quickbar(hbw, toolbar);

	toolbar = new_html_subtoolbar(hbw, html_notebook, gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLTablesToolbar"), _("Tables"));
	setup_items_for_quickbar(hbw, toolbar);

	toolbar = new_html_subtoolbar(hbw, html_notebook, gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLListToolbar"), _("List"));
	setup_items_for_quickbar(hbw, toolbar);

	toolbar = new_html_subtoolbar(hbw, html_notebook, gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLCSSToolbar"), _("CSS"));
	setup_items_for_quickbar(hbw, toolbar);

	toolbar = new_html_subtoolbar(hbw, html_notebook, gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLFormsToolbar"), _("Forms"));
	setup_items_for_quickbar(hbw, toolbar);

	toolbar = new_html_subtoolbar(hbw, html_notebook, gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLFontsToolbar"), _("Fonts"));
	setup_items_for_quickbar(hbw, toolbar);

	toolbar = new_html_subtoolbar(hbw, html_notebook, gtk_ui_manager_get_widget(bfwin->uimanager, "/HTMLFramesToolbar"), _("Frames"));
	setup_items_for_quickbar(hbw, toolbar);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(html_notebook), hbs->notebooktab);
	DEBUG_MSG("htmlbar htmlbar_toolbar_create, make page %d active\n", hbs->notebooktab);
	g_signal_connect(G_OBJECT(html_notebook), "switch-page", G_CALLBACK(notebook_switch_page_lcb), hbw);

	return html_notebook;
}

void htmlbar_load_ui(Thtmlbarwin * hbw)
{
	GError *gerror=NULL;
	const gchar *htmlbar_menu_ui = HTMLBAR_MENU_UI;
	gtk_ui_manager_add_ui_from_file(hbw->bfwin->uimanager, htmlbar_menu_ui, &gerror);
	if (gerror != NULL) {
		g_warning("loading htmlbar UI from file %s failed: %s", htmlbar_menu_ui, gerror->message);
		g_error_free(gerror);
	}
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
			{pixmap_css_columns, BF_STOCK_CSS_COLUMNS},
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
			{pixmap_script, BF_STOCK_SCRIPT},
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
			{pixmap_article, BF_STOCK_ARTICLE},
			{pixmap_aside, BF_STOCK_ASIDE},
			{pixmap_figcaption, BF_STOCK_FIGCAPTION},
			{pixmap_figure, BF_STOCK_FIGURE},
			{pixmap_footer, BF_STOCK_FOOTER},
			{pixmap_header, BF_STOCK_HEADER},
			{pixmap_hgroup, BF_STOCK_HGROUP},
			{pixmap_mark, BF_STOCK_MARK},
			{pixmap_nav, BF_STOCK_NAV},
			{pixmap_rp, BF_STOCK_RP},
			{pixmap_rt, BF_STOCK_RT},
			{pixmap_ruby, BF_STOCK_RUBY},
			{pixmap_section, BF_STOCK_SECTION},
			{pixmap_audio, BF_STOCK_AUDIO},
			{pixmap_video, BF_STOCK_VIDEO},
			{pixmap_canvas, BF_STOCK_CANVAS},
			{pixmap_html5time, BF_STOCK_HTML5TIME},
	};

	GtkIconFactory *icon_factory;
	GtkIconSet *icon_set;
	gint i;

	icon_factory = gtk_icon_factory_new();

	for (i = 0; i < G_N_ELEMENTS(htmlbar_stock_icons); i++) {
		GdkPixbuf *pixbuf;

		pixbuf = gdk_pixbuf_new_from_inline(-1,htmlbar_stock_icons[i].data,FALSE,NULL);
		icon_set = gtk_icon_set_new_from_pixbuf(pixbuf);
		g_object_unref(pixbuf);

		gtk_icon_factory_add(icon_factory, htmlbar_stock_icons[i].stock_id, icon_set);
		gtk_icon_set_unref(icon_set);
	}

	gtk_icon_factory_add_default(icon_factory);

	g_object_unref(icon_factory);
}
