/* Bluefish HTML Editor
 * html.c - menu/toolbar callbacks, inserting functions, and other cool stuff 
 *
 * Copyright (C) 
 * 1998 Olivier Sessink and Chris Mazuc
 * 1999-2002 Olivier Sessink
 * rewrite November 2000 (C) Olivier Sessink
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
/* 
 * Changes by Antti-Juhani Kaijanaho <gaia@iki.fi> on 1999-10-20
 * $Id: html.c,v 1.18 2003-03-04 18:55:53 oli4 Exp $
 */

#include <gtk/gtk.h>
#include <stdio.h>
/* #include <gdk/gdk.h>*/
#include <string.h>
#include <stdlib.h> /* strtod() */
#include <time.h>

#include "html.h" 	/* myself */
#include "html_diag.h" 	/* the new html dialog stuff  */
#include "html2.h" /* style_but_new*/
#include "cap.h"	/* cap() */
#include "bluefish.h"	/* main_v */
#include "bf_lib.h"	/* main_v */
#include "pixmap.h"  /* new_pixmap() */
#include "gtk_easy.h"
/*#include "coloursel.h"*/ /* color_but_new () */
#include "document.h"
/*#include "images.h"
#include "callbacks.h"
#include "wizards.h"*/
#include "stringlist.h"

/* time insert struct */
typedef struct {
	GtkWidget *check[6];
	GtkWidget *label[6];
	GtkWidget *dialog;
} TimeInsert;

/*****************************************************************************/

void insert_char_cb(GtkWidget * widget, gint data) {
	gchar *sp_chars[] = {
	"&Agrave;", "&Aacute;", "&Acirc;", "&Atilde;", "&AElig;",
	"&Auml;", "&Aring;", "&Ccedil;", "&Egrave;", "&Eacute;",
	"&Ecirc;", "&Euml;", "&Igrave;", "&Iacute;", "&Icirc;",
	"&Iuml;", "&Ntilde;", "&Ograve;", "&Oacute;", "&Ocirc;",
	"&Otilde;", "&Ouml;", "&Oslash;", "&Ugrave;", "&Uacute;",
	"&Ucirc;", "&Uuml;", "&Yacute;", "&agrave;", "&aacute;",
	"&acirc;", "&atilde;", "&aring;", "&aelig;", "&auml;",
	"&ccedil;", "&egrave;", "&eacute;", "&ecirc;", "&euml;",
	"&igrave;", "&iacute;", "&icirc;", "&iuml;", "&ntilde;",
	"&ograve;", "&oacute;", "&ocirc;", "&otilde;", "&ouml;",
	"&oslash;", "&ugrave;", "&uacute;", "&ucirc;", "&uuml;",
	"&yacute;", "&yuml;", "&uml;", "&acute;", "&cedil;", "&cent;",
	"&pound;", "&curren;", "&yen;", "&not;", "&times;", "&divide;",
	"&plusmn;", "&lt;", "&gt;", "&sup1;", "&sup2;", "&sup3;",
	"&frac14;", "&frac12;", "&frac34;", "&deg;", "&ETH;", "&THORN;",
	"&eth;", "&thorn;", "&szlig;", "&micro;", "&nbsp;", "&sect;",
	"&copy;", "&laquo;", "&raquo;", "&reg;", "&iexcl;", "&iquest;",
	"&ordf;", "&ordm;", "&para;", "&brvbar;", "&shy;", "&macr;",
	"&middot;","&euro;", "&oelig;", "&OElig;"
	};
	doc_insert_two_strings(main_v->current_document, sp_chars[data], NULL);
}

void general_html_cb(GtkWidget * widget, gpointer data)
{
	gchar *tmp;
	switch (GPOINTER_TO_INT(data)) {
	case 1:
		doc_insert_two_strings(main_v->current_document, cap("<B>"), cap("</B>"));
		break;
	case 2:
		doc_insert_two_strings(main_v->current_document, cap("<I>"), cap("</I>"));
		break;
	case 3:
		doc_insert_two_strings(main_v->current_document, main_v->props.xhtml == 1 ? cap("<SPAN STYLE=\"text-decoration: underline;\">") :cap("<U>")
			, main_v->props.xhtml == 1 ? cap("</SPAN>"):cap("</U>"));
		break;
	case 4:
		doc_insert_two_strings(main_v->current_document, cap("<STRIKE>"), cap("</STRIKE>"));
		break;
	case 5:
		doc_insert_two_strings(main_v->current_document, cap("<P>"), main_v->props.full_p == 1 ? cap("</P>") : NULL);
		break;
	case 6:
		doc_insert_two_strings(main_v->current_document, main_v->props.xhtml == 1 ? cap("<br />") : cap("<br>"), NULL);
		break;
	case 7:
		doc_insert_two_strings(main_v->current_document, "&nbsp;", NULL);
		break;
	case 8:
		doc_insert_two_strings(main_v->current_document, main_v->props.xhtml == 1 ? cap("<DIV STYLE=\"text-align: center\">"):cap("<DIV ALIGN=\"CENTER\">"),cap("</DIV>"));
		break;
	case 9:
		doc_insert_two_strings(main_v->current_document, main_v->props.xhtml == 1 ? cap("<DIV STYLE=\"text-align: right\">"):cap("<DIV ALIGN=\"RIGHT\">"), cap("</DIV>"));
		break;
	case 10:
		doc_insert_two_strings(main_v->current_document, "<!-- ", " -->");
		break;
	case 11:
		doc_insert_two_strings(main_v->current_document, main_v->props.xhtml == 1 ? cap("<SPAN STYLE=\"font-size: larger;\">"):cap("<FONT SIZE=\"+1\">")
			, main_v->props.xhtml == 1 ? cap("</SPAN>"):cap("</FONT>"));
		break;
	case 12:
		doc_insert_two_strings(main_v->current_document, main_v->props.xhtml == 1 ? cap("<SPAN STYLE=\"font-size: smaller;\">"):cap("<FONT SIZE=\"-1\">")
			, main_v->props.xhtml == 1 ? cap("</SPAN>"):cap("</FONT>"));
		break;
	case 13:
		doc_insert_two_strings(main_v->current_document, cap("<PRE>"), cap("</PRE>"));
		break;
	case 14:
		doc_insert_two_strings(main_v->current_document, cap("<SUB>"), cap("</SUB>"));
		break;
	case 15:
		doc_insert_two_strings(main_v->current_document, cap("<SUP>"), cap("</SUP>"));
		break;
	case 16:
		doc_insert_two_strings(main_v->current_document, cap("<STRONG>"), cap("</STRONG>"));
		break;
	case 17:
		doc_insert_two_strings(main_v->current_document, cap("<EM>"), cap("</EM>"));
		break;
	case 18:
		doc_insert_two_strings(main_v->current_document, cap("<H1>"), cap("</H1>"));
		break;
	case 19:
		doc_insert_two_strings(main_v->current_document, cap("<H2>"), cap("</H2>"));
		break;
	case 20:
		doc_insert_two_strings(main_v->current_document, cap("<H3>"), cap("</H3>"));
		break;
	case 21:
		doc_insert_two_strings(main_v->current_document, cap("<H4>"), cap("</H4>"));
		break;
	case 22:
		doc_insert_two_strings(main_v->current_document, cap("<H5>"), cap("</H5>"));
		break;
	case 23:
		doc_insert_two_strings(main_v->current_document, cap("<H6>"), cap("</H6>"));
		break;
	case 24:
		doc_insert_two_strings(main_v->current_document, cap("<TABLE>"), cap("</TABLE>"));
		break;
	case 25:
		doc_insert_two_strings(main_v->current_document, cap("<TR>"), cap("</TR>"));
		break;
	case 26:
		doc_insert_two_strings(main_v->current_document, cap("<TH>"), cap("</TH>"));
		break;
	case 27:
		doc_insert_two_strings(main_v->current_document, cap("<TD>"), cap("</TD>"));
		break;
	case 28:
		doc_insert_two_strings(main_v->current_document, cap("<CAPTION>"), cap("</CAPTION>"));
		break;
	case 29:
		doc_insert_two_strings(main_v->current_document, cap("<FRAMESET>"), cap("</FRAMESET>"));
		break;
	case 30:
		doc_insert_two_strings(main_v->current_document, main_v->props.xhtml == 1 ? cap("<FRAME />") : cap("<FRAME>"), NULL);
		break;
	case 31:
		doc_insert_two_strings(main_v->current_document, cap("<NOFRAMES>"), cap("</NOFRAMES>"));
		break;
	case 32:
		doc_insert_two_strings(main_v->current_document, main_v->props.xhtml == 1 ? cap("<BASE TARGET=\"\" />") : cap("<BASE TARGET=\"\">"), NULL);
		break;
	case 33:
		doc_insert_two_strings(main_v->current_document, cap("<UL>"), cap("</UL>"));
		break;
	case 34:
		doc_insert_two_strings(main_v->current_document, cap("<OL>"), cap("</OL>"));
		break;
	case 35:
		doc_insert_two_strings(main_v->current_document, cap("<LI>"), main_v->props.full_li == 1 ? cap("</LI>") : NULL);
		break;
	case 36:
		doc_insert_two_strings(main_v->current_document, main_v->props.full_li == 1 ? cap("<DL>\n<DT></DT>\n\t<DD></DD>\n</DL>") : cap("<DL>\n<DT>\n\t<DD>\n</DL>"), "\n");
		break;
	case 37:
		doc_insert_two_strings(main_v->current_document, cap("<DT>"), main_v->props.full_li == 1 ? cap("</DT>") : NULL);
		break;
	case 38:
		doc_insert_two_strings(main_v->current_document, cap("<DD>"), main_v->props.full_li == 1 ? cap("</DD>") : NULL);
		break;
	case 39:
		doc_insert_two_strings(main_v->current_document, cap("<MENU>"), cap("</MENU>"));
		break;
	case 40:
		doc_insert_two_strings(main_v->current_document, cap("<MAP NAME=\""), main_v->props.xhtml == 1 ?
			cap("\">\n\t<AREA SHAPE=\"\" COORDS=\"\" HREF=\"\" />\n</MAP>\n") :
			cap("\">\n\t<AREA SHAPE=\"\" COORDS=\"\" HREF=\"\">\n</MAP>\n"));
		break;
	case 41:
		doc_insert_two_strings(main_v->current_document, main_v->props.xhtml == 1 ? cap("<BR CLEAR=\"ALL\" />") : cap("<BR CLEAR=\"ALL\">"), NULL);
		break;
	case 42:
		/* the text/css should be always lowercase! */
		tmp = g_strdup_printf("<%s=\"text/css\"><!--\n", cap("STYLE TYPE"));
		doc_insert_two_strings(main_v->current_document, tmp, cap("\n--></STYLE>"));
		g_free(tmp);
		break;
	case 43:
		doc_insert_two_strings(main_v->current_document, cap("<NOBR>"), cap("</NOBR>"));
		break;
	case 44:
		tmp = g_strdup_printf("<%s=\"javascript\" %s=\"text/javascript\"><!--\n", cap("SCRIPT LANGUAGE"), cap("TYPE"));
		doc_insert_two_strings(main_v->current_document, tmp, cap("\n//--></SCRIPT>"));
		g_free(tmp);
		break;
/*	case 45:
		tmp = g_strdup_printf("<%s=\"javascript\" %s>", cap("SCRIPT LANGUAGE"), cap("SRC=\"\" TYPE=\"text/javascript\""));
		doc_insert_two_strings(main_v->current_document, tmp, cap("</SCRIPT>"));
		g_free(tmp);
		break; */
/*	case 46:
		tmp = g_strdup_printf("<%s=\"stylesheet\" %s=\"\" %s=\"text/css\">", cap("LINK REL"), cap("HREF"), cap("TYPE"));
		doc_insert_two_strings(main_v->current_document, tmp, "");
		g_free(tmp);
		break; */
	case 47:
		tmp = g_strconcat(cap("<META NAME=\"Generator\" CONTENT=\"")
			, "Bluefish ",  VERSION, " http://bluefish.openoffice.nl/"
			, main_v->props.xhtml == 1 ? "\" />\n" : "\">\n" , NULL);
		doc_insert_two_strings(main_v->current_document, tmp, NULL);
		g_free(tmp);
		break;
	case 48:
		doc_insert_two_strings(main_v->current_document, cap("<DFN>"), cap("</DFN>"));
		break;
	case 49:
		doc_insert_two_strings(main_v->current_document, cap("<CODE>"), cap("</CODE>"));
		break;
	case 50:
		doc_insert_two_strings(main_v->current_document, cap("<SAMP>"), cap("</SAMP>"));
		break;
	case 51:
		doc_insert_two_strings(main_v->current_document, cap("<KBD>"), cap("</KBD>"));
		break;
	case 52:
		doc_insert_two_strings(main_v->current_document, cap("<VAR>"), cap("</VAR>"));
		break;
	case 53:
		doc_insert_two_strings(main_v->current_document, cap("<CITE>"), cap("</CITE>"));
		break;
	case 54:
		doc_insert_two_strings(main_v->current_document, cap("<ABBR>"), cap("</ABBR>"));
		break;
	case 55:
		doc_insert_two_strings(main_v->current_document, cap("<ACRONYM>"), cap("</ACRONYM>"));
		break;
	default:
		break;
	}
}


/************************************************************************/

static gchar *extract_time_string(char *original_string)
{
	static gchar *return_string;
	gchar *start_ptr;
	gchar *end_ptr;
	gint string_size;

	return_string = g_malloc0(32);
	start_ptr = strchr(original_string, '(');
	start_ptr++;
	end_ptr = strchr(original_string, ')');
	string_size = end_ptr - start_ptr;
	strncat(return_string, start_ptr, string_size);
	return return_string;
}

/************************************************************************/
static void insert_time_destroy_lcb(GtkWidget * widget, GdkEvent *event, TimeInsert * data) {
	DEBUG_MSG("insert_time_destroy_lcb, data=%p\n", data);
	window_destroy(data->dialog);
	g_free(data);
}

static void insert_time_callback(GtkWidget * widget, TimeInsert * timeinsert)
{
	gchar *temp_string;
	gchar *insert_string;
	gchar *final_string;
	gint count;

	insert_string = g_malloc0(32);
	final_string = g_malloc0(255);
	for (count = 1; count < 6; count++) {
		if (GTK_TOGGLE_BUTTON(timeinsert->check[count])->active) {
			gtk_label_get(GTK_LABEL(timeinsert->label[count]), &temp_string);
			insert_string = extract_time_string(temp_string);
			strncat(final_string, insert_string, 31);
			strncat(final_string, " ", 31);
		}
		DEBUG_MSG("insert_time_callback, count=%d\n", count);
	}
	
	DEBUG_MSG("insert_time_callback, final_string=%s\n", final_string);
	doc_insert_two_strings(main_v->current_document, final_string, "");
	DEBUG_MSG("insert_time_callback, text inserted\n");
	g_free(insert_string);
	g_free(final_string);
	insert_time_destroy_lcb(NULL, NULL, timeinsert);
	DEBUG_MSG("insert_time_callback, finished\n");
}

/************************************************************************/

static void insert_time_cancel(GtkWidget * widget, TimeInsert * data)
{
	DEBUG_MSG("insert_time_cancel, data=%p\n", data);
	insert_time_destroy_lcb(widget, NULL, data);
}

/************************************************************************/
void insert_time_cb(GtkWidget * widget, gpointer data)
{

	gint month, year, count;
	time_t time_var;
	gchar *temp = NULL;
	struct tm *time_struct;
	TimeInsert *timeinsert;
	GtkWidget *ok_b, *cancel_b, *vbox, *hbox;

	timeinsert = g_malloc(sizeof(TimeInsert));
	time_var = time(NULL);
	time_struct = localtime(&time_var);
	DEBUG_MSG("insert_time_cb, timeinsert=%p\n", timeinsert);
	timeinsert->dialog = window_full(_("Insert Time"), GTK_WIN_POS_MOUSE
			, 12, G_CALLBACK(insert_time_destroy_lcb), timeinsert, TRUE);
	vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(timeinsert->dialog), vbox);

	for (count = 1; count < 6; count++) {
		switch (count) {
		case 1:
			temp = g_strdup_printf(_("  _Time (%i:%i:%i)"), time_struct->tm_hour, time_struct->tm_min, time_struct->tm_sec);
			break;
		case 2:
			switch (time_struct->tm_wday) {
			case 0:
				temp = g_strdup(_("  Day of the _week (Sunday)"));
				break;
			case 1:
				temp = g_strdup(_("  Day of the _week (Monday)"));
				break;
			case 2:
				temp = g_strdup(_("  Day of the _week (Tuesday)"));
				break;
			case 3:
				temp = g_strdup(_("  Day of the _week (Wednesday)"));
				break;
			case 4:
				temp = g_strdup(_("  Day of the _week (Thursday)"));
				break;
			case 5:
				temp = g_strdup(_("  Day of the _week (Friday)"));
				break;
			case 6:
				temp = g_strdup(_("  Day of the _week (Saturday)"));
				break;
			default:
				g_message(_("You appear to have a non existant day!\n"));
				temp = g_strdup(" ** Error ** see stdout");
			}					/* end of switch day of week */
			break;
		case 3:
			month = time_struct->tm_mon + 1;
			year = time_struct->tm_year;
			year = 1900 + year;
			temp = g_strdup_printf(_("  _Date (%i/%i/%i)"), time_struct->tm_mday, month, year);
			break;
		case 4:
			temp = g_strdup_printf(_("  _Unix Time (%i)"), (int) time_var);
			break;
		case 5:
			temp = g_strdup_printf(_("  Unix Date _String (%s"), ctime(&time_var));
			/* Replace \n on ')' */
			temp[strlen(temp) - 1] = ')';
			break;
		default:
			break;
		}						/* end of switch count */
		timeinsert->check[count] = gtk_check_button_new();
		timeinsert->label[count] = gtk_label_new_with_mnemonic(temp);
		gtk_label_set_mnemonic_widget(GTK_LABEL(timeinsert->label[count]), timeinsert->check[count]);
		g_free(temp);
		gtk_container_add(GTK_CONTAINER(timeinsert->check[count]), GTK_WIDGET(timeinsert->label[count]));
		gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(timeinsert->check[count]), TRUE, TRUE, 0);
	}							/* end of for loop */

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_hseparator_new(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 10);
	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 12);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

	ok_b = bf_stock_ok_button(GTK_SIGNAL_FUNC(insert_time_callback), (gpointer) timeinsert);
	gtk_window_set_default(GTK_WINDOW(timeinsert->dialog), ok_b);
	cancel_b = bf_stock_cancel_button(GTK_SIGNAL_FUNC(insert_time_cancel), (gpointer) timeinsert);
	gtk_box_pack_start(GTK_BOX(hbox), cancel_b, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), ok_b, TRUE, TRUE, 0);

	gtk_widget_show_all(timeinsert->dialog);
}

/************************************************************************/
/*
gchar *converttohex(gchar * color)
{
	if (!strncasecmp(color, "black", 32)) {
		return "#000000";
	}
	if (!strncasecmp(color, "maroon", 32)) {
		return "#800000";
	}
	if (!strncasecmp(color, "green", 32)) {
		return "#008000";
	}
	if (!strncasecmp(color, "olive", 32)) {
		return "#808000";
	}
	if (!strncasecmp(color, "navy", 32)) {
		return "#000080";
	}
	if (!strncasecmp(color, "purple", 32)) {
		return "#800080";
	}
	if (!strncasecmp(color, "fuchsia", 32)) {
		return "#FF00FF";
	}
	if (!strncasecmp(color, "teal", 32)) {
		return "#008080";
	}
	if (!strncasecmp(color, "gray", 32)) {
		return "#808080";
	}
	if (!strncasecmp(color, "silver", 32)) {
		return "#C0C0C0";
	}
	if (!strncasecmp(color, "red", 32)) {
		return "#FF0000";
	}
	if (!strncasecmp(color, "lime", 32)) {
		return "#00FF00";
	}
	if (!strncasecmp(color, "yellow", 32)) {
		return "#FFFF00";
	}
	if (!strncasecmp(color, "blue", 32)) {
		return "#0000FF";
	}
	if (!strncasecmp(color, "aqua", 32)) {
		return "#00FFFF";
	}
	if (!strncasecmp(color, "white", 32)) {
		return "#FFFFFF";
	}
	return color;
} */

/************************************************************************/
/************************************************************************/
/****************                                  **********************/
/****************  BELOW ARE ALL DIALOGS MIGRATED  **********************/
/****************      TO HTML_DIAG FUNCTIONS      **********************/
/****************                                  **********************/
/************************************************************************/
/************************************************************************/

static void quickanchorok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<A"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry), cap("HREF"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry), cap("TARGET"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[2]), cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[3]), cap("ID"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[4]), NULL, thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[5]), cap("ONCLICK"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[6]), cap("ONDBLCLICK"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[7]), cap("ONMOUSEOVER"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[8]), cap("ONMOUSEDOWN"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[9]), cap("ONMOUSEMOVE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[10]), cap("ONMOUSEOUT"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[11]), cap("ONMOUSEUP"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[12]), cap("ONKEYDOWN"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[13]), cap("ONKEYPRESS"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[14]), cap("ONKEYUP"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[3])->entry), cap("CLASS"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[16]), cap("STYLE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[17]), cap("LANG"), thestring, NULL);
	finalstring = g_strdup_printf("%s>", thestring);
	g_free(thestring);

	recent_attribs.urllist = add_entry_to_stringlist(recent_attribs.urllist, GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry));
	recent_attribs.targetlist = add_entry_to_stringlist(recent_attribs.targetlist, GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry));
	recent_attribs.classlist = add_entry_to_stringlist(recent_attribs.classlist, GTK_WIDGET(GTK_COMBO(dg->combo[3])->entry));

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</A>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void quickanchor_cb(GtkWidget * widget, gpointer data)
{
	static gchar *aitems[] = { "href", "target", "name", "id", "onclick", "ondblclick", "onmouseover", "onmousedown", "onmousemove", "onmouseout", "onmouseup", "onkeydown", "onkeypress", "onkeyup", "class", "style", "lang", NULL };
	gchar *custom = NULL;
	gchar *avalues[18];
	Thtml_diag *dg;

	GtkWidget *noteb, *but, *dgtable, *file_but, *frame;

	dg = html_diag_new(_("Quick Anchor"));
	fill_dialogvalues(aitems, avalues, &custom, (Ttagpopup *) data, widget, dg);

	noteb = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(dg->vbox), noteb, FALSE, FALSE, 0);

	frame = bf_generic_frame_new(NULL, GTK_SHADOW_NONE, 12);
	gtk_notebook_append_page(GTK_NOTEBOOK(noteb), frame, gtk_label_new(_("Options")));
	dgtable = gtk_table_new(8, 3, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(dgtable), 12);
	gtk_container_add(GTK_CONTAINER(frame), dgtable);

	{
	GList *rel_link_list=NULL, *tmplist;
/*	rel_link_list = generate_relative_doc_list();*/
	tmplist = duplicate_stringlist(recent_attribs.urllist, 1);
	rel_link_list = g_list_concat(tmplist, rel_link_list);

	dg->combo[2] = combo_with_popdown(avalues[0], rel_link_list, 1);

	free_stringlist(rel_link_list);
	}
	file_but = file_but_new(GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry), dg->dialog, 0);
	gtk_table_attach(GTK_TABLE(dgtable), GTK_WIDGET(file_but), 2, 3, 0, 1, GTK_EXPAND, GTK_EXPAND, 0, 0);
	bf_mnemonic_label_tad_with_alignment(N_("_HREF:"), dg->combo[2], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[2])), 1, 2, 0, 1);

	dg->combo[1] = combo_with_popdown(avalues[1], recent_attribs.targetlist, 1);
	bf_mnemonic_label_tad_with_alignment(N_("_Target:"), dg->combo[1], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[1])), 1, 3, 1, 2);

	dg->entry[2] = entry_with_text(avalues[2], 256);
	bf_mnemonic_label_tad_with_alignment(N_("_Name:"), dg->entry[2], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 3, 2, 3);

	dg->entry[3] = entry_with_text(avalues[3], 256);
	bf_mnemonic_label_tad_with_alignment(N_("_ID:"), dg->entry[3], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[3], 1, 3, 3, 4);

	dg->combo[3] = combo_with_popdown(avalues[14], recent_attribs.classlist, 1);
	bf_mnemonic_label_tad_with_alignment(N_("Cl_ass:"), dg->combo[3], 0, 0.5, dgtable, 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[3], 1, 3, 4, 5);

	dg->entry[16] = entry_with_text(avalues[15], 256);
	bf_mnemonic_label_tad_with_alignment(N_("St_yle:"), dg->entry[16], 0, 0.5, dgtable, 0, 1, 5, 6);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[16], 1, 2, 5, 6);

	but = style_but_new(dg->entry[16], dg->dialog);
	gtk_table_attach(GTK_TABLE(dgtable), but, 2, 3, 5, 6, GTK_EXPAND, GTK_EXPAND, 0, 0);

	dg->entry[17] = entry_with_text(avalues[16], 256);
	bf_mnemonic_label_tad_with_alignment(N_("_Language:"), dg->entry[17], 0, 0.5, dgtable, 0, 1, 6, 7);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[17], 1, 3, 6, 7);

	dg->entry[4] = entry_with_text(custom, 1024);
	bf_mnemonic_label_tad_with_alignment(N_("Custo_m:"), dg->entry[4], 0, 0.5, dgtable, 0, 1, 7, 8);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[4], 1, 3, 7, 8);

	frame = bf_generic_frame_new(NULL, GTK_SHADOW_NONE, 12);
	gtk_notebook_append_page(GTK_NOTEBOOK(noteb), frame, gtk_label_new(_("Events")));
	dgtable = gtk_table_new(10, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(dgtable), 6);
	gtk_table_set_col_spacings(GTK_TABLE(dgtable), 12);
	gtk_container_add(GTK_CONTAINER(frame), dgtable);

	dg->entry[5] = entry_with_text(avalues[4], 256);
	bf_mnemonic_label_tad_with_alignment(N_("OnClic_k:"), dg->entry[5], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[5], 1, 2, 0, 1);

	dg->entry[6] = entry_with_text(avalues[5], 256);
	bf_mnemonic_label_tad_with_alignment(N_("OnD_blClick:"), dg->entry[6], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[6], 1, 2, 1, 2);
	
	dg->entry[7] = entry_with_text(avalues[6], 256);
	bf_mnemonic_label_tad_with_alignment(N_("OnMouseO_ver:"), dg->entry[7], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[7], 1, 2, 2, 3);
	
	dg->entry[8] = entry_with_text(avalues[7], 256);
	bf_mnemonic_label_tad_with_alignment(N_("OnMouse_Down:"), dg->entry[8], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[8], 1, 2, 3, 4);

	dg->entry[9] = entry_with_text(avalues[8], 256);
	bf_mnemonic_label_tad_with_alignment(N_("OnMouse_Move:"), dg->entry[9], 0, 0.5, dgtable, 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[9], 1, 2, 4, 5);

	dg->entry[10] = entry_with_text(avalues[9], 256);
	bf_mnemonic_label_tad_with_alignment(N_("OnMouseOu_t:"), dg->entry[10], 0, 0.5, dgtable, 0, 1, 5, 6);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[10], 1, 2, 5, 6);
	
	dg->entry[11] = entry_with_text(avalues[10], 256);
	bf_mnemonic_label_tad_with_alignment(N_("OnMouse_Up:"), dg->entry[11], 0, 0.5, dgtable, 0, 1, 6, 7);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[11], 1, 2, 6, 7);

	dg->entry[12] = entry_with_text(avalues[11], 256);
	bf_mnemonic_label_tad_with_alignment(N_("OnKeyDo_wn:"), dg->entry[12], 0, 0.5, dgtable, 0, 1, 7, 8);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[12], 1, 2, 7, 8);

	dg->entry[13] = entry_with_text(avalues[12], 256);
	bf_mnemonic_label_tad_with_alignment(N_("OnKey_Press:"), dg->entry[13], 0, 0.5, dgtable, 0, 1, 8, 9);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[13], 1, 2, 8, 9);

	dg->entry[14] = entry_with_text(avalues[13], 256);
	bf_mnemonic_label_tad_with_alignment(N_("OnKe_yUp:"), dg->entry[14], 0, 0.5, dgtable, 0, 1, 9, 10);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[14], 1, 2, 9, 10);

	html_diag_finish(dg, G_CALLBACK(quickanchorok_lcb));
	if (custom)
		g_free(custom);
}

static void block_tag_editok_lcb(gint type, Thtml_diag * dg) {

	gchar *thestring=NULL, *endstring=NULL, *finalstring;

	switch (type) {
	case 1:
		thestring = g_strdup(cap("<P"));
		endstring = g_strdup(cap("</P>"));
	break;
	case 2:
		thestring = g_strdup(cap("<DIV"));
		endstring = g_strdup(cap("</DIV>"));
	break;
	case 3:
		thestring = g_strdup(cap("<SPAN"));
		endstring = g_strdup(cap("</SPAN>"));
	break;
	case 4:
		thestring = g_strdup(cap("<H1"));
		endstring = g_strdup(cap("</H1>"));
	break;
	case 5:
		thestring = g_strdup(cap("<H2"));
		endstring = g_strdup(cap("</H2>"));
	break;
	case 6:
		thestring = g_strdup(cap("<H3"));
		endstring = g_strdup(cap("</H3>"));
	break;
	case 7:
		thestring = g_strdup(cap("<H4"));
		endstring = g_strdup(cap("</H4>"));
	break;
	case 8:
		thestring = g_strdup(cap("<H5"));
		endstring = g_strdup(cap("</H5>"));
	break;
	case 9:
		thestring = g_strdup(cap("<H6"));
		endstring = g_strdup(cap("</H6>"));
	break;
	}

	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry), cap("ALIGN"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry), cap("CLASS"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[1]), cap("STYLE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[2]), cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[3]), cap("ID"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[4]), NULL, thestring, NULL);
	finalstring = g_strdup_printf("%s>", thestring);
	g_free(thestring);

	recent_attribs.classlist = add_entry_to_stringlist(recent_attribs.classlist, GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry));

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, endstring);
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(endstring);
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

static void p_editok_lcb(GtkWidget *widget, gpointer data) {
	block_tag_editok_lcb(1, data);
}
static void div_editok_lcb(GtkWidget *widget, gpointer data) {
	block_tag_editok_lcb(2, data);
}
static void span_editok_lcb(GtkWidget *widget, gpointer data) {
	block_tag_editok_lcb(3, data);
}
static void h1_editok_lcb(GtkWidget *widget, gpointer data) {
	block_tag_editok_lcb(4, data);
}
static void h2_editok_lcb(GtkWidget *widget, gpointer data) {
	block_tag_editok_lcb(5, data);
}
static void h3_editok_lcb(GtkWidget *widget, gpointer data) {
	block_tag_editok_lcb(6, data);
}
static void h4_editok_lcb(GtkWidget *widget, gpointer data) {
	block_tag_editok_lcb(7, data);
}
static void h5_editok_lcb(GtkWidget *widget, gpointer data) {
	block_tag_editok_lcb(8, data);
}
static void h6_editok_lcb(GtkWidget *widget, gpointer data) {
	block_tag_editok_lcb(9, data);
}


void block_tag_edit_cb(gint type, GtkWidget *widget, gpointer data)
{
	gchar *labeltext;
	GList *popuplist=NULL;
	GtkWidget *dgtable, *but;

	static gchar *tagitems[] = { "align", "class", "style", "name", "id", NULL };
	gchar *tagvalues[6];
	gchar *custom = NULL;
	Thtml_diag *dg;

	switch (type) {
	case 1:
		labeltext = g_strdup(_("Paragraph"));
	break;
	case 2:
		labeltext = g_strdup(_("Div"));
	break;
	case 3:
		labeltext = g_strdup(_("Span"));
	break;
	case 4:
		labeltext = g_strdup(_("Heading 1"));
	break;
	case 5:
		labeltext = g_strdup(_("Heading 2"));
	break;
	case 6:
		labeltext = g_strdup(_("Heading 3"));
	break;
	case 7:
		labeltext = g_strdup(_("Heading 4"));
	break;
	case 8:
		labeltext = g_strdup(_("Heading 5"));
	break;
	case 9:
		labeltext = g_strdup(_("Heading 6"));
	break;
	default:
		labeltext = g_strdup("");
	break;
	}
	dg = html_diag_new(labeltext);
	g_free(labeltext);
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, widget, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 4);
	
	popuplist = g_list_append(NULL, "left");
	popuplist = g_list_append(popuplist, "center");
	popuplist = g_list_append(popuplist, "right");
	dg->combo[1] = combo_with_popdown(tagvalues[0], popuplist, 1);
	g_list_free(popuplist);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Align")), 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[1])), 1, 2, 0, 1);

	dg->combo[2] = combo_with_popdown(tagvalues[1], recent_attribs.classlist, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Class")), 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[2])), 1, 2, 1, 2);

	dg->entry[1] = entry_with_text(tagvalues[2], 1024);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Style")), 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 2, 2, 3);

	but = style_but_new(dg->entry[1], dg->dialog);
	gtk_table_attach(GTK_TABLE(dgtable), but, 2, 3, 2, 3, GTK_FILL, GTK_FILL, 0, 0);

	dg->entry[2] = entry_with_text(tagvalues[3], 1024);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Name")), 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 3, 4, 0, 1);
	
	dg->entry[3] = entry_with_text(tagvalues[4], 1024);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Id")), 2, 3, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[3], 3, 4, 1, 2);

	dg->entry[4] = entry_with_text(custom, 1024);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Custom")), 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[4], 1, 4, 3, 4);
	
	switch (type) {
	case 1:
		html_diag_finish(dg, G_CALLBACK(p_editok_lcb));
	break;
	case 2:
		html_diag_finish(dg, G_CALLBACK(div_editok_lcb));
	break;
	case 3:
		html_diag_finish(dg, G_CALLBACK(span_editok_lcb));
	break;
	case 4:
		html_diag_finish(dg, G_CALLBACK(h1_editok_lcb));
	break;
	case 5:
		html_diag_finish(dg, G_CALLBACK(h2_editok_lcb));
	break;
	case 6:
		html_diag_finish(dg, G_CALLBACK(h3_editok_lcb));
	break;
	case 7:
		html_diag_finish(dg, G_CALLBACK(h4_editok_lcb));
	break;
	case 8:
		html_diag_finish(dg, G_CALLBACK(h5_editok_lcb));
	break;
	case 9:
		html_diag_finish(dg, G_CALLBACK(h6_editok_lcb));
	break;
	}
	if (custom)
		g_free(custom);
}

void p_dialog(GtkWidget * widget, gpointer data) {
	block_tag_edit_cb(1, widget,data);
}
void div_dialog(GtkWidget * widget, gpointer data) {
	block_tag_edit_cb(2, widget,data);
}
void span_dialog(GtkWidget * widget, gpointer data) {
	block_tag_edit_cb(3, widget,data);
}
void h1_dialog(GtkWidget * widget, gpointer data) {
	block_tag_edit_cb(4, widget,data);
}
void h2_dialog(GtkWidget * widget, gpointer data) {
	block_tag_edit_cb(5, widget,data);
}
void h3_dialog(GtkWidget * widget, gpointer data) {
	block_tag_edit_cb(6, widget,data);
}
void h4_dialog(GtkWidget * widget, gpointer data) {
	block_tag_edit_cb(7, widget,data);
}
void h5_dialog(GtkWidget * widget, gpointer data) {
	block_tag_edit_cb(8, widget,data);
}
void h6_dialog(GtkWidget * widget, gpointer data) {
	block_tag_edit_cb(9, widget,data);
}

static void quickruleok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<HR"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry), cap("ALIGN"), thestring, NULL);
	thestring = insert_integer_if_spin(dg->spin[1], cap("SIZE"), thestring, NULL);
	thestring = insert_integer_if_spin(dg->spin[2], cap("WIDTH"), thestring, dg->check[1]);
	thestring = insert_attr_if_checkbox(dg->check[2], cap("NOSHADE"), thestring);
	thestring = insert_string_if_entry(dg->entry[1], NULL, thestring, NULL);
	finalstring = g_strdup_printf(main_v->props.xhtml == 1 ? "%s />" : "%s>", thestring);
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, NULL);
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}

	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void quickrule_cb(GtkWidget * widget, gpointer data)
{
	GList *popdownlist = NULL;

	static gchar *hritems[] = { "align", "size", "width", "noshade", NULL };
	gchar *hrvalues[5];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable;
	GtkAdjustment *dgadj;
	
	dg = html_diag_new(_("Horizontal Rule"));
	fill_dialogvalues(hritems, hrvalues, &custom, (Ttagpopup *) data, widget, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 10);
	gtk_table_set_row_spacings(GTK_TABLE(dgtable), 12);

	popdownlist = g_list_insert(popdownlist, "center", 0);
	popdownlist = g_list_insert(popdownlist, "left", 1);
	popdownlist = g_list_insert(popdownlist, "right", 2);
	dg->combo[1] = combo_with_popdown(hrvalues[0], popdownlist, 1);
	g_list_free(popdownlist);
	bf_mnemonic_label_tad_with_alignment(N_("_Align:"), dg->combo[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(dg->combo[1]), 1, 4, 0, 1);

	dgadj = (GtkAdjustment *) gtk_adjustment_new((gfloat) 1, 0.0, 200.0, 1.0, 5.0, 0.0);
	dg->spin[1] = gtk_spin_button_new(dgadj, 1, 0);
	/* gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin1), 1); */
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1, 2, 1, 2);
	bf_mnemonic_label_tad_with_alignment(N_("_Height:"), dg->spin[1], 0, 0.5, dgtable, 0, 1, 1, 2);
	parse_integer_for_dialog(hrvalues[1], dg->spin[1], NULL, NULL);

	dgadj = (GtkAdjustment *) gtk_adjustment_new((gfloat) 50, 0.0, 600.0, 1.0, 5.0, 0.0);
	dg->spin[2] = gtk_spin_button_new(dgadj, 1, 0);
	/*   gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin2), 1); */
	bf_mnemonic_label_tad_with_alignment(N_("_Width:"), dg->spin[2], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[2], 1, 2, 2, 3);

	dg->check[1] = gtk_check_button_new_with_mnemonic(N_("Is _percent"));
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 3, 4, 2, 3);

	parse_integer_for_dialog(hrvalues[2], dg->spin[2] , NULL, dg->check[1]);

	dg->check[2] = gtk_check_button_new_with_mnemonic(N_("No _shading"));
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[2], 1, 2, 3, 4);
	parse_existence_for_dialog(hrvalues[3], dg->check[2]);

	dg->entry[1] = entry_with_text(custom, 1024);
	bf_mnemonic_label_tad_with_alignment(N_("Custo_m:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 10, 4, 5);

	html_diag_finish(dg, G_CALLBACK(quickruleok_lcb));

	if (custom)	g_free(custom);
}

static void quickstart_ok_lcb(GtkWidget * widget, Thtml_diag * dg) {

	gchar *tmpchar1, *tmpchar2, *tmpchar3, *finalstring;
	GList *tmplist;
	gchar *text;

	recent_attribs.dtd_cblist =
		add_to_stringlist(recent_attribs.dtd_cblist, gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(dg->combo[1])->entry)));

	if(main_v->props.xhtml){
		tmpchar1 = g_strdup_printf("%s\n%shttp://www.w3.org/1999/xhtml%sen%sen\">\n%s\n", gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(dg->combo[1])->entry)), cap("<HTML XMLNS=\""), cap("\" XML:LANG=\""), cap("\" LANG=\""), cap("<HEAD>"));
	} else{
		tmpchar1 = g_strdup_printf("%s\n%s\n", gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(dg->combo[1])->entry)), cap("<HTML>\n<HEAD>"));
	}
	tmplist = g_list_first(GTK_CLIST(dg->clist[1])->selection);
	while (tmplist) {
		DEBUG_MSG("tmplist->data=%d\n", GPOINTER_TO_INT(tmplist->data));
		gtk_clist_get_text(GTK_CLIST(dg->clist[1]), GPOINTER_TO_INT(tmplist->data), 0, &text);
		tmpchar2 = g_strconcat(tmpchar1, text, "\n", NULL);
		g_free(tmpchar1);
		tmpchar1 = tmpchar2;
		tmplist = g_list_next(tmplist);
	}
	tmpchar3 = gtk_editable_get_chars(GTK_EDITABLE(dg->entry[1]), 0, -1);
	tmpchar2 = g_strconcat(tmpchar1, cap("<TITLE>"), tmpchar3, cap("</TITLE>\n"), NULL);
	g_free(tmpchar1);
	g_free(tmpchar3);
	{
		GtkTextBuffer *buf;
		GtkTextIter itstart, itend;
		buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(dg->text[0]));
		gtk_text_buffer_get_bounds(buf,&itstart,&itend);
		tmpchar3 = gtk_text_buffer_get_text(buf, &itstart, &itend, FALSE);
		if (strlen(tmpchar3)) {
			tmpchar1 = tmpchar2;
			tmpchar2 = g_strconcat(tmpchar1, cap("<style type=\"text/css\"><!--\n"), tmpchar3, cap("\n--></style>"), NULL);
			g_free(tmpchar1);
		}
		g_free(tmpchar3);
	}
	
	tmpchar1 = tmpchar2;
	finalstring = g_strconcat(tmpchar1, cap("</HEAD>\n<BODY>\n"), NULL);
	g_free(tmpchar1);

	doc_insert_two_strings(dg->doc, finalstring, cap("\n</BODY>\n</HTML>"));

	g_free(finalstring);
	g_object_unref(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(dg->text[0]))));
	html_diag_destroy_cb(NULL, dg);
}

void quickstart_cb(GtkWidget * widget, gpointer data)
{
	GList *tmplist;
	gchar *text[1];
	GtkWidget *scrolwin, *dgtable, *label;
	Thtml_diag *dg;
	
	dg = html_diag_new(_("Quick Start"));

	dgtable = html_diag_table_in_vbox(dg, 9, 4);

	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, "");
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 3.2//EN\">");
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Strict//EN\">");
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Frameset//EN\">");
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">");
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0//EN\">");
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"DTD/xhtml1-strict.dtd\">");
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\">");
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Frameset//EN\" \"DTD/xhtml1-frameset.dtd\">");
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Frameset//EN\">");
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"DTD/xhtml1-transitional.dtd\">");
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\">" );
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist,"<!DOCTYPE html PUBLIC \"-//IETF//DTD XHTML 1.1//EN\">" );

	dg->combo[1] = combo_with_popdown("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">", recent_attribs.dtd_cblist, 1);
	gtk_widget_set_size_request(dg->combo[1], 425, -1);
	label = gtk_label_new_with_mnemonic(N_("_Doctype:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), label, 0, 1, 0, 1);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), (GTK_COMBO(dg->combo[1])->entry));
	gtk_entry_set_activates_default(GTK_ENTRY(GTK_COMBO(dg->combo[1])->entry), TRUE);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[1], 1, 4, 0, 1);
	
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	label = gtk_label_new(_("Header tags:"));
	gtk_table_attach_defaults(GTK_TABLE(dgtable), label , 0, 1, 2, 3);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_set_size_request(scrolwin, 300, 100);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), scrolwin, 1, 4, 1, 4);

	dg->clist[1] = gtk_clist_new(1);
	gtk_clist_set_selection_mode(GTK_CLIST(dg->clist[1]), GTK_SELECTION_MULTIPLE);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolwin), dg->clist[1]);
	gtk_clist_freeze(GTK_CLIST(dg->clist[1]));

	recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, "<meta name=\"generator\" content=\"Bluefish\">");
	recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, "<meta name=\"author\" content=\"\">");
	recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, "<meta name=\"copyright\" content=\"\">");
	recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, "<meta name=\"keywords\" content=\"\">");
	recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, "<meta name=\"description\" content=\"\">");
	{
		gchar *tmpstr = g_strconcat("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=",main_v->props.newfile_default_encoding,"\">",NULL);
		recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, tmpstr);
	}
	recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, "<meta http-equiv=\"Expires\" content=\"Tue, 20 Aug 1996 14:25:27 GMT\">");
	recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, "<meta http-equiv=\"refresh\" content=\"5; URL=http://\">");
	recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, "<meta name=\"ROBOTS\" content=\"NOINDEX, NOFOLLOW\">");

	tmplist = g_list_first(recent_attribs.headerlist);
	while (tmplist) {
		text[0] = tmplist->data;
		gtk_clist_append(GTK_CLIST(dg->clist[1]), text);
		tmplist = g_list_next(tmplist);
	}
	gtk_clist_thaw(GTK_CLIST(dg->clist[1]));

	dg->entry[1] = entry_with_text(NULL, 0);
	label = gtk_label_new_with_mnemonic(N_("_Title:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), label, 0, 1, 4, 5);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), dg->entry[1]);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 4, 4, 5);
	{
		GtkWidget *scroll, *stylebut;
		GtkTextBuffer *buf;
		buf = gtk_text_buffer_new(NULL);
		dg->text[0] = gtk_text_view_new_with_buffer(buf);
		scroll = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
									   GTK_POLICY_AUTOMATIC,
									   GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
											(scroll), GTK_SHADOW_IN);
		gtk_container_add(GTK_CONTAINER(scroll), dg->text[0]);
		label = gtk_label_new_with_mnemonic(N_("St_yle:"));
		gtk_table_attach_defaults(GTK_TABLE(dgtable), label, 0, 1, 5, 6);
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), dg->text[0]);
		gtk_widget_set_size_request(scroll, 300,100);
		gtk_table_attach_defaults(GTK_TABLE(dgtable), scroll, 1, 3, 5, 9);
		stylebut = style_but_new_for_wizard(dg->text[0]);
		gtk_table_attach(GTK_TABLE(dgtable), stylebut, 3, 4, 5, 6, GTK_EXPAND, GTK_EXPAND, 0, 0);
	}

	html_diag_finish(dg, G_CALLBACK(quickstart_ok_lcb));
}

static void bodyok_lcb(GtkWidget * widget, Thtml_diag *dg)
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<BODY"));
	if (dg->entry[1]) {
		thestring = insert_string_if_entry(dg->entry[1], cap("BACKGROUND"), thestring, NULL);
		thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry), cap("BGCOLOR"), thestring, NULL);
		thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry), cap("TEXT"), thestring, NULL);
		thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[3])->entry), cap("LINK"), thestring, NULL);
		thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[4])->entry), cap("VLINK"), thestring, NULL);
		thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[5])->entry), cap("ALINK"), thestring, NULL);
	}
	thestring = insert_string_if_entry(dg->entry[3], cap("STYLE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[6])->entry), cap("CLASS"), thestring, NULL);
	thestring = insert_string_if_entry(dg->entry[4], cap("ID"), thestring, NULL);
	thestring = insert_string_if_entry(dg->entry[5], cap("LANG"), thestring, NULL);
	thestring = insert_string_if_entry(dg->entry[6], cap("OnLoad"), thestring, NULL);
	thestring = insert_string_if_entry(dg->entry[7], cap("OnUnLoad"), thestring, NULL);

	thestring = insert_string_if_entry(dg->entry[2], NULL, thestring, NULL);
	finalstring = g_strconcat(thestring, ">\n", NULL);
	g_free(thestring);

	if (dg->entry[1]) {
		recent_attribs.colorlist = add_entry_to_stringlist(recent_attribs.colorlist, GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry));
		recent_attribs.colorlist = add_entry_to_stringlist(recent_attribs.colorlist, GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry));
		recent_attribs.colorlist = add_entry_to_stringlist(recent_attribs.colorlist, GTK_WIDGET(GTK_COMBO(dg->combo[3])->entry));
		recent_attribs.colorlist = add_entry_to_stringlist(recent_attribs.colorlist, GTK_WIDGET(GTK_COMBO(dg->combo[4])->entry));
		recent_attribs.colorlist = add_entry_to_stringlist(recent_attribs.colorlist, GTK_WIDGET(GTK_COMBO(dg->combo[5])->entry));
	}
	recent_attribs.classlist = add_entry_to_stringlist(recent_attribs.classlist, GTK_WIDGET(GTK_COMBO(dg->combo[6])->entry));

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</BODY>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}

	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void body_cb(GtkWidget * widget, gpointer data)
{
	static gchar *bodyitems[] = { "background", "bgcolor", "text", "link", "vlink", "alink"
									, "style", "class", "id", "language", "onload", "onunload", NULL };

	GtkWidget *color_but, *file_but, *dgtable[3], *stylebut, *noteb, *hbox, *vbox2, *label, *frame;
	gchar *bodyvalues[11];
	gchar *custom = NULL;
	Thtml_diag *dg;

	dg = html_diag_new(_("Body"));
	fill_dialogvalues(bodyitems, bodyvalues, &custom, (Ttagpopup *) data, widget, dg);

	noteb = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(dg->vbox), noteb, FALSE, FALSE, 0);

	/* Options Tab */
	        frame = bf_generic_frame_new(NULL, GTK_SHADOW_NONE, 12);
		gtk_notebook_append_page(GTK_NOTEBOOK(noteb), frame, gtk_label_new(_("Options")));
		dgtable[1] = gtk_table_new(8, 3, FALSE);
		gtk_table_set_row_spacings(GTK_TABLE(dgtable[1]), 6);
		gtk_table_set_col_spacings(GTK_TABLE(dgtable[1]), 12);
		gtk_container_add(GTK_CONTAINER(frame), dgtable[1]);
		
		dg->entry[3] = entry_with_text(bodyvalues[6], 256);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[1]), dg->entry[3], 1, 2, 0, 1);
		bf_mnemonic_label_tad_with_alignment(N_("St_yle:"), dg->entry[3], 1, 0.5, dgtable[1], 0, 1, 0, 1);

		stylebut = style_but_new(dg->entry[3], dg->dialog);
		gtk_table_attach(GTK_TABLE(dgtable[1]), stylebut, 2, 3, 0, 1, GTK_EXPAND, GTK_EXPAND, 0, 0);

		dg->combo[6] = combo_with_popdown(bodyvalues[7], recent_attribs.classlist, 1);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[1]), dg->combo[6], 1, 2, 1, 2);
		bf_mnemonic_label_tad_with_alignment(N_("Cl_ass:"), dg->combo[6], 1, 0.5, dgtable[1], 0, 1, 1, 2);

		dg->entry[4] = entry_with_text(bodyvalues[8], 256);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[1]), dg->entry[4], 1, 2, 2, 3);
		bf_mnemonic_label_tad_with_alignment(N_("_ID:"), dg->entry[4], 1, 0.5, dgtable[1], 0, 1, 2, 3);

		dg->entry[5] = entry_with_text(bodyvalues[9], 256);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[1]), dg->entry[5], 1, 2, 3, 4);
		bf_mnemonic_label_tad_with_alignment(N_("_Language:"), dg->entry[5], 1, 0.5, dgtable[1], 0, 1, 3, 4);

		dg->entry[2] = entry_with_text(custom, 1024);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[1]), dg->entry[2], 1, 3, 4, 5);
		bf_mnemonic_label_tad_with_alignment(N_("Custom _values:"), dg->entry[2], 1, 0.5, dgtable[1], 0, 1, 4, 5);

		/* Events Tab */
		frame = bf_generic_frame_new(NULL, GTK_SHADOW_NONE, 12);
		gtk_notebook_append_page(GTK_NOTEBOOK(noteb), frame, gtk_label_new(_("Events")));

		vbox2 = gtk_vbox_new(FALSE, 12);
		gtk_container_add(GTK_CONTAINER(frame), vbox2);
		
		hbox = gtk_hbox_new(FALSE, 12);
		dg->entry[6] = entry_with_text(bodyvalues[10], 256);
		label = gtk_label_new_with_mnemonic(N_("On_Load:"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), dg->entry[6]);
		gtk_box_pack_start(GTK_BOX(hbox), dg->entry[6], TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

		hbox = gtk_hbox_new(FALSE, 12);
		dg->entry[7] = entry_with_text(bodyvalues[11], 256);
		label = gtk_label_new_with_mnemonic(N_("On_UnLoad:"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), dg->entry[7]);
		gtk_box_pack_start(GTK_BOX(hbox), dg->entry[7], TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
		
		/* Depreciated options tab */
	if (!(!main_v->props.allow_dep && widget && !data)){
	        frame = bf_generic_frame_new(NULL, GTK_SHADOW_NONE, 12);
		gtk_notebook_append_page(GTK_NOTEBOOK(noteb), frame, gtk_label_new(_("Depreciated options")));
		dgtable[0] = gtk_table_new(6, 3, FALSE);
		gtk_table_set_row_spacings(GTK_TABLE(dgtable[0]), 6);
		gtk_table_set_col_spacings(GTK_TABLE(dgtable[0]), 12);
		gtk_container_add(GTK_CONTAINER(frame), dgtable[0]);

		dg->entry[1] = entry_with_text(bodyvalues[0], 256);
		file_but = file_but_new(dg->entry[1], dg->dialog, 0);

		gtk_table_attach(GTK_TABLE(dgtable[0]), GTK_WIDGET(file_but), 2, 3, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
		bf_mnemonic_label_tad_with_alignment(N_("Background _Image:"), dg->entry[1], 0, 0.5, dgtable[0], 0, 1, 0, 1);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[0]), dg->entry[1], 1, 2, 0, 1);

		dg->combo[1] = combo_with_popdown(bodyvalues[1], recent_attribs.colorlist, 1);
		color_but = color_but_new(GTK_COMBO(dg->combo[1])->entry, dg->dialog);
		gtk_table_attach(GTK_TABLE(dgtable[0]), color_but, 2, 3, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[0]), dg->combo[1], 1, 2, 1, 2);
		bf_mnemonic_label_tad_with_alignment(N_("Back_ground Color:"), dg->combo[1], 0, 0.5, dgtable[0], 0, 1, 1, 2);

		dg->combo[2] = combo_with_popdown(bodyvalues[2], recent_attribs.colorlist, 1);
		color_but = color_but_new(GTK_COMBO(dg->combo[2])->entry, dg->dialog);
		gtk_table_attach(GTK_TABLE(dgtable[0]), color_but, 2, 3, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[0]), dg->combo[2], 1, 2, 2, 3);
		bf_mnemonic_label_tad_with_alignment(N_("_Text Color:"), dg->combo[2], 0, 0.5, dgtable[0], 0, 1, 2, 3);

		dg->combo[3] = combo_with_popdown(bodyvalues[3], recent_attribs.colorlist, 1);
		color_but = color_but_new(GTK_COMBO(dg->combo[3])->entry, dg->dialog);
		gtk_table_attach(GTK_TABLE(dgtable[0]), color_but, 2, 3, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[0]), dg->combo[3], 1, 2, 3, 4);
		bf_mnemonic_label_tad_with_alignment(N_("_Link Color:"), dg->combo[3], 0, 0.5, dgtable[0], 0, 1, 3, 4);

		dg->combo[4] = combo_with_popdown(bodyvalues[4], recent_attribs.colorlist, 1);
		color_but = color_but_new(GTK_COMBO(dg->combo[4])->entry, dg->dialog);
		gtk_table_attach(GTK_TABLE(dgtable[0]), color_but, 2, 3, 4, 5, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[0]), dg->combo[4], 1, 2, 4, 5);
		bf_mnemonic_label_tad_with_alignment(N_("_Visited Link Color:"), dg->combo[4], 0, 0.5, dgtable[0], 0, 1, 4, 5);

		dg->combo[5] = combo_with_popdown(bodyvalues[5], recent_attribs.colorlist, 1);
		color_but = color_but_new(GTK_COMBO(dg->combo[5])->entry, dg->dialog);
		gtk_table_attach(GTK_TABLE(dgtable[0]), color_but, 2, 3, 5, 6, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[0]), dg->combo[5], 1, 2, 5, 6);
		bf_mnemonic_label_tad_with_alignment(N_("_Active Link Color:"), dg->combo[5], 0, 0.5, dgtable[0], 0, 1, 5, 6);

	} else {
		dg->combo[1] = NULL;
		dg->combo[2] = NULL;
		dg->combo[3] = NULL;
		dg->combo[4] = NULL;
		dg->combo[5] = NULL;
		dg->entry[1] = NULL;
	}

	html_diag_finish(dg, G_CALLBACK(bodyok_lcb));

	if (custom)	g_free(custom);
}

static void metaok_lcb(GtkWidget * widget, Thtml_diag *dg)
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<META"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry), cap("HTTP-EQUIV"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry), cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry(dg->entry[1], cap("CONTENT"), thestring, NULL);
	thestring = insert_string_if_entry(dg->entry[2], cap("SCHEME"), thestring, NULL);
	thestring = insert_string_if_entry(dg->entry[3], NULL, thestring, NULL);

	if (main_v->props.xhtml == 1) {
		finalstring = g_strconcat(thestring," />", NULL);
	} else {
		finalstring = g_strconcat(thestring,">", NULL);
	}
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, NULL);
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}


void meta_cb(GtkWidget * widget, gpointer data)
{
	GList *popuplist = NULL;

	static gchar *tagitems[] = { "http-equiv", "name", "content", "scheme", NULL };
	gchar *tagvalues[5];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable;
	
	dg = html_diag_new(_("Meta"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, widget, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 10);

	popuplist = g_list_append(NULL, "Description");
	popuplist = g_list_append(popuplist, "Date");
	popuplist = g_list_append(popuplist, "Copyright");
	popuplist = g_list_append(popuplist, "Author");
	popuplist = g_list_append(popuplist, "Reply-to"); /* name or equiv? */
	popuplist = g_list_append(popuplist, "Keywords");
	popuplist = g_list_append(popuplist, "PICS-label"); /* name or equiv? */
	popuplist = g_list_append(popuplist, "Refresh");
	dg->combo[2] = combo_with_popdown(tagvalues[1], popuplist, 1);
	bf_mnemonic_label_tad_with_alignment(N_("_Name:"), dg->combo[2], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[2])), 1, 10, 0, 1);
	g_list_free(popuplist);

	popuplist = g_list_append(NULL, "Expires");
	popuplist = g_list_append(popuplist, "Content-Type");
	popuplist = g_list_append(popuplist, "Content-Location");
	popuplist = g_list_append(popuplist, "Content-Language");
	popuplist = g_list_append(popuplist, "Content-Encoding");
	popuplist = g_list_append(popuplist, "Pragma");
	dg->combo[1] = combo_with_popdown(tagvalues[0], popuplist, 1);
	bf_mnemonic_label_tad_with_alignment(N_("_HTTP-EQUIV:"), dg->combo[1], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[1])), 1, 10, 1, 2);
	g_list_free(popuplist);

	dg->entry[1] = entry_with_text(tagvalues[2], 512);
	bf_mnemonic_label_tad_with_alignment(N_("Con_tent:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 10, 2, 3);

	dg->entry[2] = entry_with_text(tagvalues[3], 256);
	bf_mnemonic_label_tad_with_alignment(N_("_Scheme:"), dg->entry[2], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 10, 3, 4);

	dg->entry[3] = entry_with_text(custom, 1024);
	bf_mnemonic_label_tad_with_alignment(N_("Custo_m:"), dg->entry[3], 0, 0.5, dgtable, 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[3], 1, 10, 4, 5);

	html_diag_finish(dg, G_CALLBACK(metaok_lcb));
	if (custom)	g_free(custom);
}

static void generalfontdialog_lcb(gint type, GtkWidget * widget, Thtml_diag *dg)
{
	gchar *thestring, *finalstring;

	switch(type) {
	case 1:
		thestring = g_strdup(cap("<FONT"));
	break;
	case 2:
		thestring = g_strdup(cap("<BASEFONT"));
	break;
	default:
#ifdef DEVELOPMENT
		DEBUG_MSG("generalfontdialog_cb is called with an unknown type\n");
		exit(1);
#endif 
		return;
	break;
	}
	if (strlen(gtk_entry_get_text(GTK_ENTRY(dg->spin[1])))) {
		gchar *sizecombo, *tmpstr;
		sizecombo = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(dg->combo[1])->entry), 0, -1);
		if (strlen(sizecombo)) {
			tmpstr = g_strconcat(thestring, cap(" size=\"")
					,sizecombo
					,gtk_entry_get_text(GTK_ENTRY(GTK_SPIN_BUTTON(dg->spin[1])))
					, "\"", NULL);
		} else {
			tmpstr = g_strconcat(thestring, cap(" size=\"")
					,gtk_entry_get_text(GTK_ENTRY(GTK_SPIN_BUTTON(dg->spin[1])))
					, "\"", NULL);
		}
		g_free(thestring);
		thestring = tmpstr;
		g_free(sizecombo);
	}
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry), cap("COLOR"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[3])->entry), cap("FACE"), thestring, NULL);	
	thestring = insert_string_if_entry(dg->entry[1], NULL, thestring, NULL);
	finalstring = g_strconcat(thestring, ">", NULL);
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</FONT>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

static void fontok_lcb(GtkWidget * widget, Thtml_diag *dg) {
	generalfontdialog_lcb(1, widget, dg);
}
static void basefontok_lcb(GtkWidget * widget, Thtml_diag *dg) {
	generalfontdialog_lcb(2, widget, dg);
}

static void generalfontdialog_cb(gint type, GtkWidget * widget, gpointer data)
{
	GList *popdownlist = NULL;
	GtkWidget *color_but;

	static gchar *tagitems[] = { "size", "face", "color", NULL };
	gchar *tagvalues[4];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable;

	switch(type) {
		case 1:
			dg = html_diag_new(_("Font"));
		break;
		case 2:
			dg = html_diag_new(_("Base Font"));
		break;
		default:
			return;
		break;
	}
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, widget, dg);		

	dgtable = html_diag_table_in_vbox(dg, 3, 10);

	popdownlist = g_list_insert(popdownlist, "+", 0);
	popdownlist = g_list_insert(popdownlist, "-", 1);
	popdownlist = g_list_insert(popdownlist, "", 2);
	dg->combo[1] = combo_with_popdown("", popdownlist, 0);
	g_list_free(popdownlist);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Size")), 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[1])), 1, 2, 0, 1);
	dg->spin[1] = spinbut_with_value(NULL, 0, 100, 1.0, 5.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 2, 3, 0, 1);
	parse_integer_for_dialog(tagvalues[0], dg->spin[1], GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry), NULL);

	dg->combo[2] = combo_with_popdown(tagvalues[2], recent_attribs.colorlist, 1);
	color_but = color_but_new(GTK_COMBO(dg->combo[2])->entry, dg->dialog);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Color")), 6, 7, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[2])), 7, 9, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(color_but), 9, 10, 0, 1);

	dg->combo[3] = combo_with_popdown(tagvalues[1], recent_attribs.fontlist, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Face")), 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[3])), 1, 10, 1, 2);

	dg->entry[1] = entry_with_text(custom, 1024);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Custom")), 1, 2, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 2, 10, 2, 3);
	switch(type) {
		case 1:
			html_diag_finish(dg, G_CALLBACK(fontok_lcb));
		break;
		case 2:
			html_diag_finish(dg, G_CALLBACK(basefontok_lcb));
		break;
	}

	if (custom)	g_free(custom);
}

void fontdialog_cb(GtkWidget *widget, gpointer data) {
	generalfontdialog_cb(1, widget,data);
}

void basefont_cb(GtkWidget *widget, gpointer data) {
	generalfontdialog_cb(2, widget,data);
}

static void emailok_lcb(GtkWidget * widget, Thtml_diag *dg)
{
	gchar *finalstring, *tmpstr, *subj, *body;
	tmpstr = g_strconcat(cap("<A HREF=\"mailto:")
			, gtk_entry_get_text(GTK_ENTRY(dg->entry[1]))
			, NULL);
	if (strlen(gtk_entry_get_text(GTK_ENTRY(dg->entry[2])))) {
		subj = g_strconcat("?Subject=", gtk_entry_get_text(GTK_ENTRY(dg->entry[2])), NULL);
	} else {
		subj = g_strdup("");
	}
	if (strlen(gtk_entry_get_text(GTK_ENTRY(dg->entry[3])))) {
		body = g_strconcat("?Body=", gtk_entry_get_text(GTK_ENTRY(dg->entry[3])), NULL);
	} else {
		body = g_strdup("");
	}
	finalstring = g_strconcat(tmpstr, subj, body, "\">", NULL);
	g_free(tmpstr);
	g_free(subj);
	g_free(body);
	doc_insert_two_strings(dg->doc, finalstring, cap("</A>"));
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void email_cb(GtkWidget * widget, gpointer data)
{
        GtkWidget *dgtable;
	Thtml_diag *dg;
	
	dg = html_diag_new(_("Email"));

	dgtable = gtk_table_new(3, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(dgtable), 12);
	gtk_table_set_row_spacings(GTK_TABLE(dgtable), 12);
	gtk_box_pack_start(GTK_BOX(dg->vbox), dgtable, FALSE, FALSE, 0);

	dg->entry[1] = gtk_entry_new_with_max_length(256);
	bf_mnemonic_label_tad_with_alignment(N_("_Email address:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 2, 0, 1);

	
	dg->entry[2] = gtk_entry_new_with_max_length(256);
	bf_mnemonic_label_tad_with_alignment(N_("UrlEncoded _subject:"), dg->entry[2], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 2, 1, 2);

	dg->entry[3] = gtk_entry_new_with_max_length(256);
	bf_mnemonic_label_tad_with_alignment(N_("UrlEncoded _body:"), dg->entry[3], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[3], 1, 2, 2, 3);

	html_diag_finish(dg, G_CALLBACK(emailok_lcb));
}

static void quicklistok_lcb(GtkWidget * widget, Thtml_diag *dg)
{
	gint active, temp, i;
	gchar *finalstring;

	if (strlen(gtk_entry_get_text(GTK_ENTRY(dg->spin[1])))) {
		temp = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dg->spin[1]));
		finalstring = g_malloc((8+temp*12)*sizeof(gchar));

		active = GTK_TOGGLE_BUTTON(dg->radio[1])->active;

		if (active) {
			strcpy(finalstring, cap("<OL>"));
		} else {
			strcpy(finalstring, cap("<UL>"));
		}
		/**********************************************/
		/* TODO: use bf_str_repeat() function !!!!!!! */
		/**********************************************/
		for (i = 0; i < temp; i++) {
			strcat(finalstring,  main_v->props.full_li == 1 ? cap("\n\t<LI></LI>") : cap("\n\t<LI>"));
		}
		strcat(finalstring, "\n");

		if (active) {
			doc_insert_two_strings(dg->doc, finalstring, cap("</OL>"));
		} else {
			doc_insert_two_strings(dg->doc, finalstring, cap("</UL>"));
		}
	}
	html_diag_destroy_cb(NULL, dg);
}

void quicklist_cb(GtkWidget * widget, gpointer data)
{
	Thtml_diag *dg;
	GtkWidget *dgtable;

	dg = html_diag_new(_("Quick List"));

	dgtable = html_diag_table_in_vbox(dg, 2, 10);
	dg->spin[1] = spinbut_with_value("3", 0, 500, 1.0, 5.0);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dg->spin[1]), 1);
	bf_mnemonic_label_tad_with_alignment(N_("_Rows:"), dg->spin[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1, 2, 0, 1);

	dg->radio[1] = gtk_radio_button_new_with_mnemonic(NULL, N_("Or_dered"));
	dg->radio[2] = gtk_radio_button_new_with_mnemonic(gtk_radio_button_get_group(GTK_RADIO_BUTTON(dg->radio[1])), N_("_Unordered"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dg->radio[1]), TRUE);
	bf_mnemonic_label_tad_with_alignment(N_("Style:"), NULL, 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->radio[1], 1, 5, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->radio[2], 5, 10, 1, 2);

	html_diag_finish(dg, G_CALLBACK(quicklistok_lcb));
	
}

static void framesetdialogok_lcb(GtkWidget * widget, Thtml_diag *dg) {

	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<FRAMESET"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[1])), cap("COLS"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[2])), cap("ROWS"), thestring, NULL);
	finalstring = g_strconcat(thestring, ">", NULL);
	g_free(thestring);

	if ((dg->check[1]) && (GTK_TOGGLE_BUTTON(dg->check[1])->active)) {
		gint rowcount=1, colcount=1, framecount=0;
		gchar *rowstr, *colstr, *tmpstr, *framestring, *tmp;
		rowstr = gtk_editable_get_chars(GTK_EDITABLE(dg->entry[1]), 0, -1);
		tmp = rowstr;
		while ((tmp = strchr(tmp, ','))) {
			rowcount++;
			tmp++;
		}
		g_free(rowstr);
		colstr = gtk_editable_get_chars(GTK_EDITABLE(dg->entry[2]), 0, -1);
		tmp = colstr;
		while ((tmp = strchr(tmp, ','))) {
			colcount++;
			tmp++;
		}
		g_free(colstr);

		framecount = colcount * rowcount;

		if (main_v->props.xhtml == 1) {
			framestring = bf_str_repeat(cap("\n<FRAME />"), framecount);
		} else {
			framestring = bf_str_repeat(cap("\n<FRAME>"), framecount);
		}
		tmpstr = g_strconcat(finalstring, framestring, NULL);
		g_free(framestring);
		g_free(finalstring);
		finalstring = tmpstr;
	}

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("\n</FRAMESET>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}


void framesetdialog_cb(GtkWidget * widget, gpointer data)
{

	static gchar *tagitems[] = { "cols", "rows", "border", "spacing", NULL };
	gchar *tagvalues[5];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable;

	dg = html_diag_new(_("Frameset"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, widget, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 10);

	dg->entry[1] = entry_with_text(tagvalues[0], 256);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Cols")), 0, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 2, 10, 0, 1);

	dg->entry[2] = entry_with_text(tagvalues[1], 256);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Rows")), 0, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 2, 10, 1, 2);

	if (dg->range.end == -1) {
		dg->check[1] = gtk_check_button_new();
		gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Add <frame> elements")), 0, 6, 2, 3);
		gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 6, 10, 2, 3);
	} else {
		dg->check[1] = NULL;
	}

/* THIS IS NO VALID HTML !!!! 
	dg->check[1] = gtk_check_button_new();
	parse_existence_for_dialog(tagvalues[2], dg->check[1]);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Border")), 0, 2, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 2, 4, 2, 3);

	dg->spin[1] = spinbut_with_value(tagvalues[3], 0, 500, 1.0, 5.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Spacing")), 0, 2, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 2, 4, 3, 4);
*/
	dg->entry[3] = entry_with_text(custom, 1024);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Custom")), 0, 2, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[3], 2, 10, 3, 4);
	html_diag_finish(dg, G_CALLBACK(framesetdialogok_lcb));

	if (custom)	g_free(custom);
}


static void framedialogok_lcb(GtkWidget * widget, Thtml_diag *dg) {
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<FRAME"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry), cap("SRC"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry), cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->spin[0])), cap("FRAMEBORDER"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[3])->entry), cap("SCROLLING"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->spin[1])), cap("MARGINWIDTH"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->spin[2])), cap("MARGINHEIGHT"), thestring, NULL);
	thestring = insert_attr_if_checkbox(dg->check[1], cap("NORESIZE"), thestring);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[1])), NULL, thestring, NULL);

	recent_attribs.targetlist = add_entry_to_stringlist(recent_attribs.targetlist, GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry));
	recent_attribs.urllist = add_entry_to_stringlist(recent_attribs.urllist, GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry));

	if (main_v->props.xhtml == 1) {
		finalstring = g_strconcat(thestring," />", NULL);
	} else {
		finalstring = g_strconcat(thestring,">", NULL);
	}
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, NULL);
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}


void framedialog_cb(GtkWidget * widget, gpointer data)
{
	GList *popuplist = NULL;

	static gchar *tagitems[] =
		{ "src", "name", "frameborder", "scrolling", "marginwidth", "marginheight", "noresize", NULL };
	gchar *tagvalues[9];
	gchar *custom = NULL;
	
	GtkWidget *dgtable, *file_but;
	Thtml_diag *dg;

	dg = html_diag_new(_("Frame"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, widget, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 10);

	dg->combo[1] = combo_with_popdown(tagvalues[0], recent_attribs.urllist, 1);
	file_but = file_but_new(GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry), dg->dialog, 0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(file_but), 9, 10, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Source")), 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[1])), 1, 9, 0, 1);

	dg->combo[2] = combo_with_popdown_sized(tagvalues[1], recent_attribs.targetlist, 1, 90);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Name")), 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[2], 1, 5, 1, 2);

	dg->spin[1] = spinbut_with_value(tagvalues[4], 0, 500, 1.0, 5.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_(" Margin Width ")), 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1, 5, 2, 3);

	dg->spin[2] = spinbut_with_value(tagvalues[5], 0, 500, 1.0, 5.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_(" Margin Height ")), 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[2], 1, 5, 3, 4);

	popuplist = g_list_append(popuplist, "auto");
	popuplist = g_list_append(popuplist, "yes");
	popuplist = g_list_append(popuplist, "no");
	dg->combo[3] = combo_with_popdown_sized(tagvalues[3], popuplist, 0, 90);
	g_list_free(popuplist);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Scrolling ")), 5, 6, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[3], 6, 10, 1, 2);

	dg->spin[0] = spinbut_with_value(tagvalues[2], 0, 1, 1.0, 1.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Frameborder")), 5, 6, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[0], 6, 10, 2, 3);

	dg->check[1] = gtk_check_button_new();
	parse_existence_for_dialog(tagvalues[6], dg->check[1]);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("No Resize")), 5, 6, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 6, 10, 3, 4);

/* THIS IS NO HTML
	adj = (GtkAdjustment *) gtk_adjustment_new((gfloat) 1, 0.0, 500.0, 1.0, 5.0, 0.0);
	spin3 = gtk_spin_button_new(adj, 1, 0);
	parse_integer_for_dialog(tagvalues[5], spin3, NULL, NULL);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin3), 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Spacing")), 4, 5, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), spin3, 5, 8, 3, 4); */

	dg->entry[1] = entry_with_text(custom, 1024);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Custom")), 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 10, 4, 5);

	html_diag_finish(dg, G_CALLBACK(framedialogok_lcb));
	
	if (custom)	g_free(custom);
}

/* this is no HTML, but often requested, so we leave it for now */

static void embedok_lcb(GtkWidget * widget,Thtml_diag *dg )
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<EMBED"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->entry[1])), cap("SRC"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->spin[1])), cap("WIDTH"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->spin[2])), cap("HEGHT"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->spin[3])), cap("BORDER"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(GTK_COMBO(dg->combo[1])->entry)), cap("ALIGN"), thestring, NULL);

	if (main_v->props.xhtml == 1) {
		finalstring = g_strconcat(thestring, " />", NULL);
	} else {
		finalstring = g_strconcat(thestring, ">", NULL);
	}
	g_free(thestring);

	doc_insert_two_strings(dg->doc, finalstring, NULL);
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void embed_cb(GtkWidget * widget, gpointer data)
{
	GtkWidget *file_but;
	GtkWidget *dgtable;
	Thtml_diag *dg;

	dg = html_diag_new(_("Embed"));

	dgtable = html_diag_table_in_vbox(dg, 5, 4);
	dg->entry[1] = gtk_entry_new_with_max_length(256);
	file_but = file_but_new(dg->entry[1], dg->dialog, 0);
	gtk_table_attach(GTK_TABLE(dgtable), file_but, 3, 4, 0, 1, GTK_EXPAND, GTK_EXPAND, 0, 0);
	bf_mnemonic_label_tad_with_alignment(N_("_Source:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 3, 0, 1);

	dg->combo[1] = combo_with_popdown("", recent_attribs.positionlist, 1);
	bf_mnemonic_label_tad_with_alignment(N_("_Align:"), dg->combo[1], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(dg->combo[1]), 1, 2, 1, 2);

	dg->spin[1] = spinbut_with_value("", 0, 500, 1.0, 5.0);
	bf_mnemonic_label_tad_with_alignment(N_("_Width:"), dg->spin[1], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1, 2, 2, 3);

	dg->spin[2] = spinbut_with_value("", 0, 500, 1.0, 5.0);
	bf_mnemonic_label_tad_with_alignment(N_("_Height:"), dg->spin[2], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[2], 1, 2, 3, 4);

	dg->spin[3] = spinbut_with_value("", 0, 500, 1.0, 5.0);
	bf_mnemonic_label_tad_with_alignment(N_("Bo_rder:"), dg->spin[3], 0, 0.5, dgtable, 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[3], 1, 2, 4, 5);

	html_diag_finish(dg, G_CALLBACK(embedok_lcb));	
}

static void script_linkok_lcb(gint type, GtkWidget * widget,Thtml_diag *dg ) {
	gchar *thestring, *finalstring, *endstring;

	if (type == 0) {
		thestring = g_strdup(cap("<SCRIPT"));
		thestring = insert_string_if_entry(dg->entry[0], cap("SRC"), thestring, NULL);
		thestring = insert_string_if_entry(GTK_COMBO(dg->combo[0])->entry, cap("LANGUAGE"), thestring, NULL);
	} else {
		thestring = g_strdup(cap("<LINK"));
		thestring = insert_string_if_entry(dg->entry[0], cap("HREF"), thestring, NULL);
		thestring = insert_string_if_entry(GTK_COMBO(dg->combo[0])->entry, cap("REL"), thestring, NULL);
	}
	thestring = insert_string_if_entry(GTK_COMBO(dg->combo[1])->entry, cap("TYPE"), thestring, NULL);
	thestring = insert_string_if_entry(dg->entry[1], NULL, thestring, NULL);
	if (main_v->props.xhtml == 1 && type == 1) {
		finalstring = g_strconcat(thestring, " />", NULL);
		endstring = NULL;
	} else {
		finalstring = g_strconcat(thestring, ">", NULL);
		if (type == 0) {
			endstring = cap("</SCRIPT>");
		} else {
			endstring = NULL;
		}
	}
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, endstring);
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

static void linkok_lcb(GtkWidget * widget,Thtml_diag *dg ) {
	script_linkok_lcb(1, widget, dg);
}

static void scriptok_lcb(GtkWidget * widget,Thtml_diag *dg ) {
	script_linkok_lcb(0, widget, dg);
}

static void script_link_cb(gint type, GtkWidget * widget, gpointer data) {
	GtkWidget *file_but;
	GList *tmplist, *tmplist2;
	GtkWidget *dgtable, *label;
	Thtml_diag *dg;
	static gchar *scriptitems[] =	{ "src", "language", "type", NULL };
	static gchar *linkitems[] =	{ "href", "rel", "type", NULL };
	gchar *tagvalues[4];
	gchar *custom = NULL;

	if (type == 0) {
		dg = html_diag_new(_("Script"));
		fill_dialogvalues(scriptitems, tagvalues, &custom, (Ttagpopup *) data, widget, dg);
	} else {
		dg = html_diag_new(_("Link"));
		fill_dialogvalues(linkitems, tagvalues, &custom, (Ttagpopup *) data, widget, dg);
	}

	dgtable = html_diag_table_in_vbox(dg, 3, 12);
	dg->entry[0] = entry_with_text(tagvalues[0], 1024);
	file_but = file_but_new(dg->entry[0], dg->dialog, 0);
	gtk_table_attach(GTK_TABLE(dgtable), file_but, 10, 12, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Source")), 0, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[0], 2, 10, 0, 1);

	if (type == 0) {
		tmplist = g_list_append(NULL, "javascript");
		tmplist = g_list_append(tmplist, "javascript1.2");
		tmplist = g_list_append(tmplist, "javascript1.1");
		label = gtk_label_new(_("Language"));
		tmplist2 = g_list_append(NULL, "application/javascript");
		tmplist2 = g_list_append(NULL, "text/javascript");
		tmplist2 = g_list_append(tmplist2, "application/x-javascript");
	} else {
		tmplist = g_list_append(NULL, "stylesheet");
		tmplist = g_list_append(tmplist, "alternate");
		tmplist = g_list_append(tmplist, "start");
		tmplist = g_list_append(tmplist, "next");
		tmplist = g_list_append(tmplist, "prev");
		tmplist = g_list_append(tmplist, "contents");
		tmplist = g_list_append(tmplist, "index");
		tmplist = g_list_append(tmplist, "glossary");
		tmplist = g_list_append(tmplist, "copyright");
		tmplist = g_list_append(tmplist, "chapter");
		tmplist = g_list_append(tmplist, "section");
		tmplist = g_list_append(tmplist, "subsection");
		tmplist = g_list_append(tmplist, "appendix");
		tmplist = g_list_append(tmplist, "help");
		tmplist = g_list_append(tmplist, "bookmark");
		label = gtk_label_new(_("Rel"));
		tmplist2 = g_list_append(NULL, "text/css");
	}
	tmplist2 = g_list_append(tmplist2, "text/plain");
	tmplist2 = g_list_append(tmplist2, "text/html");

	dg->combo[0] = combo_with_popdown(tagvalues[1], tmplist, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), label, 0, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[0], 2, 6, 1, 2);

	dg->combo[1] = combo_with_popdown(tagvalues[2], tmplist2, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Type")), 6, 8, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[1], 8, 12, 1, 2);

	dg->entry[1] = entry_with_text(custom, 1024);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Custom")), 1, 2, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 2, 10, 2, 3);

	if (type == 0) {
		html_diag_finish(dg, G_CALLBACK(scriptok_lcb));
	} else {
		html_diag_finish(dg, G_CALLBACK(linkok_lcb));
	}
	g_list_free(tmplist);
	g_list_free(tmplist2);
}

void script_cb(GtkWidget * widget, gpointer data) {
	script_link_cb(0, widget, data);
}

void link_cb(GtkWidget * widget, gpointer data) {
	script_link_cb(1, widget, data);
}
