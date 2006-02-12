/* Bluefish HTML Editor
 * html.c - menu/toolbar callbacks, inserting functions, and other cool stuff 
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * modifications Copyright (C) 1999 Antti-Juhani Kaijanaho
 * Copyright (C) 1999-2006 Olivier Sessink
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*#define DEBUG*/

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>        /* strtod() */
#include <string.h>
#include <time.h>

#include "html.h"
#include "bluefish.h"      /* main_v */
#include "bf_lib.h"
#include "cap.h"           /* cap() */
#include "document.h"
#include "gtk_easy.h"
#include "html2.h"         /* style_but_new*/
#include "html_diag.h"     /* the new html dialog stuff  */
#include "pixmap.h"        /* new_pixmap() */
#include "stringlist.h"

/*****************************************************************************/

void insert_char_cb(Tbfwin* bfwin,guint callback_action, GtkWidget *widget) {
	gchar *sp_chars[] = {
	NULL, "&Aacute;", "&Acirc;", "&Atilde;", "&AElig;",
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
	"&middot;","&euro;", "&oelig;", "&OElig;", "&Agrave;"
	};
	doc_insert_two_strings(bfwin->current_document, sp_chars[callback_action], NULL);
}

void general_html_menu_cb(Tbfwin* bfwin,guint callback_action, GtkWidget *widget) {
	gchar *tmp;
	switch (callback_action) {
	case 1:
		doc_insert_two_strings(bfwin->current_document, cap("<B>"), cap("</B>"));
		break;
	case 2:
		doc_insert_two_strings(bfwin->current_document, cap("<I>"), cap("</I>"));
		break;
	case 3:
		doc_insert_two_strings(bfwin->current_document, main_v->props.xhtml == 1 ? cap("<SPAN STYLE=\"text-decoration: underline;\">") :cap("<U>")
			, main_v->props.xhtml == 1 ? cap("</SPAN>"):cap("</U>"));
		break;
	case 4:
		doc_insert_two_strings(bfwin->current_document, cap("<STRIKE>"), cap("</STRIKE>"));
		break;
	case 5:
		doc_insert_two_strings(bfwin->current_document, cap("<P>"), cap("</P>"));
		break;
	case 6:
		doc_insert_two_strings(bfwin->current_document, main_v->props.xhtml == 1 ? cap("<br />") : cap("<br>"), NULL);
		break;
	case 7:
		doc_insert_two_strings(bfwin->current_document, "&nbsp;", NULL);
		break;
	case 8:
		doc_insert_two_strings(bfwin->current_document, main_v->props.xhtml == 1 ? cap("<DIV STYLE=\"text-align: center\">"):cap("<DIV ALIGN=\"CENTER\">"),cap("</DIV>"));
		break;
	case 9:
		doc_insert_two_strings(bfwin->current_document, main_v->props.xhtml == 1 ? cap("<DIV STYLE=\"text-align: right\">"):cap("<DIV ALIGN=\"RIGHT\">"), cap("</DIV>"));
		break;
	case 10:
		doc_insert_two_strings(bfwin->current_document, "<!-- ", " -->");
		break;
	case 11:
		doc_insert_two_strings(bfwin->current_document, main_v->props.xhtml == 1 ? cap("<SPAN STYLE=\"font-size: larger;\">"):cap("<FONT SIZE=\"+1\">")
			, main_v->props.xhtml == 1 ? cap("</SPAN>"):cap("</FONT>"));
		break;
	case 12:
		doc_insert_two_strings(bfwin->current_document, main_v->props.xhtml == 1 ? cap("<SPAN STYLE=\"font-size: smaller;\">"):cap("<FONT SIZE=\"-1\">")
			, main_v->props.xhtml == 1 ? cap("</SPAN>"):cap("</FONT>"));
		break;
	case 13:
		doc_insert_two_strings(bfwin->current_document, cap("<PRE>"), cap("</PRE>"));
		break;
	case 14:
		doc_insert_two_strings(bfwin->current_document, cap("<SUB>"), cap("</SUB>"));
		break;
	case 15:
		doc_insert_two_strings(bfwin->current_document, cap("<SUP>"), cap("</SUP>"));
		break;
	case 16:
		doc_insert_two_strings(bfwin->current_document, cap("<STRONG>"), cap("</STRONG>"));
		break;
	case 17:
		doc_insert_two_strings(bfwin->current_document, cap("<EM>"), cap("</EM>"));
		break;
	case 18:
		doc_insert_two_strings(bfwin->current_document, cap("<H1>"), cap("</H1>"));
		break;
	case 19:
		doc_insert_two_strings(bfwin->current_document, cap("<H2>"), cap("</H2>"));
		break;
	case 20:
		doc_insert_two_strings(bfwin->current_document, cap("<H3>"), cap("</H3>"));
		break;
	case 21:
		doc_insert_two_strings(bfwin->current_document, cap("<H4>"), cap("</H4>"));
		break;
	case 22:
		doc_insert_two_strings(bfwin->current_document, cap("<H5>"), cap("</H5>"));
		break;
	case 23:
		doc_insert_two_strings(bfwin->current_document, cap("<H6>"), cap("</H6>"));
		break;
	case 24:
		doc_insert_two_strings(bfwin->current_document, cap("<TABLE>"), cap("</TABLE>"));
		break;
	case 25:
		doc_insert_two_strings(bfwin->current_document, cap("<TR>"), cap("</TR>"));
		break;
	case 26:
		doc_insert_two_strings(bfwin->current_document, cap("<TH>"), cap("</TH>"));
		break;
	case 27:
		doc_insert_two_strings(bfwin->current_document, cap("<TD>"), cap("</TD>"));
		break;
	case 28:
		doc_insert_two_strings(bfwin->current_document, cap("<CAPTION>"), cap("</CAPTION>"));
		break;
	case 29:
		doc_insert_two_strings(bfwin->current_document, cap("<FRAMESET>"), cap("</FRAMESET>"));
		break;
	case 30:
		doc_insert_two_strings(bfwin->current_document, main_v->props.xhtml == 1 ? cap("<FRAME />") : cap("<FRAME>"), NULL);
		break;
	case 31:
		doc_insert_two_strings(bfwin->current_document, cap("<NOFRAMES>"), cap("</NOFRAMES>"));
		break;
	case 32:
		doc_insert_two_strings(bfwin->current_document, main_v->props.xhtml == 1 ? cap("<BASE TARGET=\"\" />") : cap("<BASE TARGET=\"\">"), NULL);
		break;
	case 33:
		doc_insert_two_strings(bfwin->current_document, cap("<UL>"), cap("</UL>"));
		break;
	case 34:
		doc_insert_two_strings(bfwin->current_document, cap("<OL>"), cap("</OL>"));
		break;
	case 35:
		doc_insert_two_strings(bfwin->current_document, cap("<LI>"), cap("</LI>"));
		break;
	case 36:
		doc_insert_two_strings(bfwin->current_document, cap("<DL>\n\t<DT></DT>\n\t<DD></DD>\n</DL>"), "\n");
		break;
	case 37:
		doc_insert_two_strings(bfwin->current_document, cap("<DT>"), cap("</DT>"));
		break;
	case 38:
		doc_insert_two_strings(bfwin->current_document, cap("<DD>"), cap("</DD>"));
		break;
	case 39:
		doc_insert_two_strings(bfwin->current_document, cap("<MENU>"), cap("</MENU>"));
		break;
	case 40:
		doc_insert_two_strings(bfwin->current_document, cap("<MAP NAME=\""), main_v->props.xhtml == 1 ?
			cap("\">\n\t<AREA SHAPE=\"\" COORDS=\"\" HREF=\"\" />\n</MAP>\n") :
			cap("\">\n\t<AREA SHAPE=\"\" COORDS=\"\" HREF=\"\">\n</MAP>\n"));
		break;
	case 41:
		doc_insert_two_strings(bfwin->current_document, main_v->props.xhtml == 1 ? cap("<BR CLEAR=\"ALL\" />") : cap("<BR CLEAR=\"ALL\">"), NULL);
		break;
	case 42:
		/* the text/css should be always lowercase! */
		tmp = g_strdup_printf("<%s=\"text/css\"><!--\n", cap("STYLE TYPE"));
		doc_insert_two_strings(bfwin->current_document, tmp, cap("\n--></STYLE>"));
		g_free(tmp);
		break;
	case 43:
		doc_insert_two_strings(bfwin->current_document, cap("<NOBR>"), cap("</NOBR>"));
		break;
	case 44:
		tmp = g_strdup_printf("<%s=\"javascript\" %s=\"text/javascript\"><!--\n", cap("SCRIPT LANGUAGE"), cap("TYPE"));
		doc_insert_two_strings(bfwin->current_document, tmp, cap("\n//--></SCRIPT>"));
		g_free(tmp);
		break;
/*	case 45:
		tmp = g_strdup_printf("<%s=\"javascript\" %s>", cap("SCRIPT LANGUAGE"), cap("SRC=\"\" TYPE=\"text/javascript\""));
		doc_insert_two_strings(bfwin->current_document, tmp, cap("</SCRIPT>"));
		g_free(tmp);
		break; */
/*	case 46:
		tmp = g_strdup_printf("<%s=\"stylesheet\" %s=\"\" %s=\"text/css\">", cap("LINK REL"), cap("HREF"), cap("TYPE"));
		doc_insert_two_strings(bfwin->current_document, tmp, "");
		g_free(tmp);
		break; */
	case 47:
		tmp = g_strconcat(cap("<META NAME=\"Generator\" CONTENT=\"")
			, "Bluefish ",  VERSION, " http://bluefish.openoffice.nl/"
			, main_v->props.xhtml == 1 ? "\" />\n" : "\">\n" , NULL);
		doc_insert_two_strings(bfwin->current_document, tmp, NULL);
		g_free(tmp);
		break;
	case 48:
		doc_insert_two_strings(bfwin->current_document, cap("<DFN>"), cap("</DFN>"));
		break;
	case 49:
		doc_insert_two_strings(bfwin->current_document, cap("<CODE>"), cap("</CODE>"));
		break;
	case 50:
		doc_insert_two_strings(bfwin->current_document, cap("<SAMP>"), cap("</SAMP>"));
		break;
	case 51:
		doc_insert_two_strings(bfwin->current_document, cap("<KBD>"), cap("</KBD>"));
		break;
	case 52:
		doc_insert_two_strings(bfwin->current_document, cap("<VAR>"), cap("</VAR>"));
		break;
	case 53:
		doc_insert_two_strings(bfwin->current_document, cap("<CITE>"), cap("</CITE>"));
		break;
	case 54:
		doc_insert_two_strings(bfwin->current_document, cap("<ABBR>"), cap("</ABBR>"));
		break;
	case 55:
		doc_insert_two_strings(bfwin->current_document, cap("<ACRONYM>"), cap("</ACRONYM>"));
		break;
	case 56:
		doc_insert_two_strings(bfwin->current_document, cap("<SMALL>"), cap("</SMALL>"));
		break;
	case 57:
		doc_insert_two_strings(bfwin->current_document, cap("<BIG>"), cap("</BIG>"));
		break;
	case 58:
		doc_insert_two_strings(bfwin->current_document, cap("<BUTTON>"), cap("</BUTTON>"));
		break;
	case 59:
		doc_insert_two_strings(bfwin->current_document, cap("<SCRIPT>"), cap("</SCRIPT>"));
		break;
	case 60:
		doc_insert_two_strings(bfwin->current_document, cap("<SPAN>"), cap("</SPAN>"));
		break;
	case 61:
		doc_insert_two_strings(bfwin->current_document, main_v->props.xhtml == 1 ? cap("<LINK />") : cap("<LINK>"), NULL);
		break;
	case 62:
		doc_insert_two_strings(bfwin->current_document, cap("<DIV>"), cap("</DIV>"));
		break;
	case 63:
		doc_insert_two_strings(bfwin->current_document, cap("<A>"), cap("</A>"));
		break;
	case 64:
		doc_insert_two_strings(bfwin->current_document, main_v->props.xhtml == 1 ? cap("<IMG />") : cap("<IMG>"), NULL);
		break;
	case 65:
		doc_insert_two_strings(bfwin->current_document, main_v->props.xhtml == 1 ? cap("<INPUT />") : cap("<INPUT>"), NULL);
		break;
	case 66:
		doc_insert_two_strings(bfwin->current_document, cap("<TEXTAREA>"), cap("</TEXTAREA>"));
		break;
	case 67:
		doc_insert_two_strings(bfwin->current_document, cap("<FORM>"), cap("</FORM>"));
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

/* time insert struct */
typedef struct {
	GtkWidget *check[7];
	GtkWidget *label[7];
	GtkWidget *dialog;
	Tbfwin *bfwin;
} TimeInsert;


static void insert_time_destroy_lcb(GtkWidget * widget, TimeInsert * data) {
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
	for (count = 1; count < 7; count++) {
		if (GTK_TOGGLE_BUTTON(timeinsert->check[count])->active) {
			gtk_label_get(GTK_LABEL(timeinsert->label[count]), &temp_string);
			insert_string = extract_time_string(temp_string);
			strncat(final_string, insert_string, 31);
			strncat(final_string, " ", 31);
		}
		DEBUG_MSG("insert_time_callback, count=%d\n", count);
	}
	
	DEBUG_MSG("insert_time_callback, final_string=%s\n", final_string);
	doc_insert_two_strings(timeinsert->bfwin->current_document, final_string, "");
	DEBUG_MSG("insert_time_callback, text inserted\n");
	g_free(insert_string);
	g_free(final_string);
	insert_time_destroy_lcb(NULL, timeinsert);
	DEBUG_MSG("insert_time_callback, finished\n");
}

/************************************************************************/

static void insert_time_cancel(GtkWidget * widget, TimeInsert * data)
{
	DEBUG_MSG("insert_time_cancel, data=%p\n", data);
	insert_time_destroy_lcb(widget, data);
}

/************************************************************************/
void insert_time_dialog(Tbfwin *bfwin) {

	gint month, year, count;
	gchar isotime[60];
	time_t time_var;
	gchar *temp = NULL;
	struct tm *time_struct;
	TimeInsert *timeinsert;
	GtkWidget *ok_b, *cancel_b, *vbox, *hbox;

	timeinsert = g_malloc(sizeof(TimeInsert));
	timeinsert->bfwin = bfwin;
	time_var = time(NULL);
	time_struct = localtime(&time_var);
	DEBUG_MSG("insert_time_cb, timeinsert=%p\n", timeinsert);
	timeinsert->dialog = window_full(_("Insert Time"), GTK_WIN_POS_MOUSE
			, 12, G_CALLBACK(insert_time_destroy_lcb), timeinsert, TRUE);
	vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(timeinsert->dialog), vbox);

	for (count = 1; count < 7; count++) {
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
		case 6:
			strftime(isotime, 30, "%Y-%m-%dT%H:%M:%S%z", time_struct);
			temp = g_strdup_printf(_("  ISO-8601 Ti_me (%s)"), isotime);
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
	Tbfwin *bfwin = dg->bfwin;

	thestring = g_strdup(cap("<A"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry), cap("HREF"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry), cap("TARGET"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[2]), cap("NAME"), thestring, NULL);
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
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->attrwidget[0])->entry), cap("CLASS"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->attrwidget[1]), cap("ID"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->attrwidget[2]), cap("STYLE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->entry[17]), cap("LANG"), thestring, NULL);
	finalstring = g_strdup_printf("%s>", thestring);
	g_free(thestring);

	bfwin->session->urllist = add_entry_to_stringlist(bfwin->session->urllist, GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry));
	bfwin->session->targetlist = add_entry_to_stringlist(bfwin->session->targetlist, GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry));
	bfwin->session->classlist = add_entry_to_stringlist(bfwin->session->classlist, GTK_WIDGET(GTK_COMBO(dg->attrwidget[0])->entry));

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</A>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void quickanchor_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	static gchar *aitems[] = { "href", "target", "name", "onkeyup", "onclick", "ondblclick", "onmouseover", "onmousedown", "onmousemove", "onmouseout", "onmouseup", "onkeydown", "onkeypress", "class", "id", "style", "lang", NULL };
	gchar *custom = NULL;
	gchar *avalues[18];
	Thtml_diag *dg;

	GtkWidget *noteb, *dgtable, *file_but;
	DEBUG_MSG("quickanchor_dialog, bfwin=%p, data=%p\n",bfwin,data);
	dg = html_diag_new(bfwin,_("Quick Anchor"));
	fill_dialogvalues(aitems, avalues, &custom, (Ttagpopup *) data, dg);

	noteb = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(dg->vbox), noteb, FALSE, FALSE, 0);

	dgtable = generic_table_inside_notebookframe(noteb, _("Attributes"), 8, 3);

	{
		GList *rel_link_list=NULL, *tmplist;
		rel_link_list = list_relative_document_filenames(bfwin->current_document);
		tmplist = duplicate_stringlist(bfwin->session->urllist, 1);
		rel_link_list = g_list_concat(tmplist, rel_link_list);
	
		dg->combo[2] = combo_with_popdown(avalues[0], rel_link_list, 1);
	
		free_stringlist(rel_link_list);
	}
	file_but = file_but_new(GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry), 0, bfwin);
	gtk_table_attach(GTK_TABLE(dgtable), GTK_WIDGET(file_but), 2, 3, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	bf_mnemonic_label_tad_with_alignment(_("_HREF:"), dg->combo[2], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[2])), 1, 2, 0, 1);

	dg->combo[1] = combo_with_popdown(avalues[1], bfwin->session->targetlist, 1);
	bf_mnemonic_label_tad_with_alignment(_("_Target:"), dg->combo[1], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[1])), 1, 3, 1, 2);

	dg->entry[2] = entry_with_text(avalues[2], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Name:"), dg->entry[2], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 3, 2, 3);

	generic_class_id_style_section(dg, 0, dgtable, 3, avalues, 13);

	dg->entry[17] = entry_with_text(avalues[16], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Language:"), dg->entry[17], 0, 0.5, dgtable, 0, 1, 6, 7);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[17], 1, 3, 6, 7);

	dg->entry[4] = entry_with_text(custom, 1024);
	bf_mnemonic_label_tad_with_alignment(_("Custo_m:"), dg->entry[4], 0, 0.5, dgtable, 0, 1, 7, 8);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[4], 1, 3, 7, 8);

	dgtable = generic_table_inside_notebookframe(noteb, _("Events"), 10, 2);

	dg->entry[5] = entry_with_text(avalues[4], 256);
	bf_mnemonic_label_tad_with_alignment(_("OnClic_k:"), dg->entry[5], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[5], 1, 2, 0, 1);

	dg->entry[6] = entry_with_text(avalues[5], 256);
	bf_mnemonic_label_tad_with_alignment(_("OnD_blClick:"), dg->entry[6], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[6], 1, 2, 1, 2);
	
	dg->entry[7] = entry_with_text(avalues[6], 256);
	bf_mnemonic_label_tad_with_alignment(_("OnMouseO_ver:"), dg->entry[7], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[7], 1, 2, 2, 3);
	
	dg->entry[8] = entry_with_text(avalues[7], 256);
	bf_mnemonic_label_tad_with_alignment(_("OnMouse_Down:"), dg->entry[8], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[8], 1, 2, 3, 4);

	dg->entry[9] = entry_with_text(avalues[8], 256);
	bf_mnemonic_label_tad_with_alignment(_("OnMouse_Move:"), dg->entry[9], 0, 0.5, dgtable, 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[9], 1, 2, 4, 5);

	dg->entry[10] = entry_with_text(avalues[9], 256);
	bf_mnemonic_label_tad_with_alignment(_("OnMouseOu_t:"), dg->entry[10], 0, 0.5, dgtable, 0, 1, 5, 6);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[10], 1, 2, 5, 6);
	
	dg->entry[11] = entry_with_text(avalues[10], 256);
	bf_mnemonic_label_tad_with_alignment(_("OnMouse_Up:"), dg->entry[11], 0, 0.5, dgtable, 0, 1, 6, 7);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[11], 1, 2, 6, 7);

	dg->entry[12] = entry_with_text(avalues[11], 256);
	bf_mnemonic_label_tad_with_alignment(_("OnKeyDo_wn:"), dg->entry[12], 0, 0.5, dgtable, 0, 1, 7, 8);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[12], 1, 2, 7, 8);

	dg->entry[13] = entry_with_text(avalues[12], 256);
	bf_mnemonic_label_tad_with_alignment(_("OnKey_Press:"), dg->entry[13], 0, 0.5, dgtable, 0, 1, 8, 9);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[13], 1, 2, 8, 9);

	dg->entry[14] = entry_with_text(avalues[3], 256);
	bf_mnemonic_label_tad_with_alignment(_("OnKe_yUp:"), dg->entry[14], 0, 0.5, dgtable, 0, 1, 9, 10);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[14], 1, 2, 9, 10);

	html_diag_finish(dg, G_CALLBACK(quickanchorok_lcb));
	if (custom)
		g_free(custom);
}

static void block_tag_editok_lcb(gint type, Thtml_diag * dg) {
	Tbfwin *bfwin = dg->bfwin;
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

	bfwin->session->classlist = add_entry_to_stringlist(bfwin->session->classlist, GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry));

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

void block_tag_edit_dialog(Tbfwin *bfwin, gint type, Ttagpopup *data) {
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
	dg = html_diag_new(bfwin,labeltext);
	g_free(labeltext);
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 4);
	
	popuplist = g_list_append(NULL, "left");
	popuplist = g_list_append(popuplist, "center");
	popuplist = g_list_append(popuplist, "right");
	dg->combo[1] = combo_with_popdown(tagvalues[0], popuplist, 1);
	g_list_free(popuplist);
	bf_mnemonic_label_tad_with_alignment(_("Ali_gn:"), dg->combo[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[1])), 1, 2, 0, 1);

	dg->combo[2] = combo_with_popdown(tagvalues[1], bfwin->session->classlist, 1);
	bf_mnemonic_label_tad_with_alignment(_("Cl_ass:"), dg->combo[2], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[2])), 1, 2, 1, 2);

	dg->entry[1] = entry_with_text(tagvalues[2], 1024);
	bf_mnemonic_label_tad_with_alignment(_("St_yle:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 3, 2, 3);

	but = style_but_new(dg->entry[1], dg->dialog);
	gtk_table_attach(GTK_TABLE(dgtable), but, 3, 4, 2, 3, GTK_EXPAND, GTK_EXPAND, 0, 0);

	dg->entry[2] = entry_with_text(tagvalues[3], 1024);
	bf_mnemonic_label_tad_with_alignment(_("_Name:"), dg->entry[2], 1, 0.5, dgtable, 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 3, 4, 0, 1);
	
	dg->entry[3] = entry_with_text(tagvalues[4], 1024);
	bf_mnemonic_label_tad_with_alignment(_("_ID:"), dg->entry[3], 1, 0.5, dgtable, 2, 3, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[3], 3, 4, 1, 2);

	dg->entry[4] = entry_with_text(custom, 1024);
	bf_mnemonic_label_tad_with_alignment(_("Custo_m:"), dg->entry[4], 0, 0.5, dgtable, 0, 1, 3, 4);
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
void p_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	block_tag_edit_dialog(bfwin, 1, data);
}
void div_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	block_tag_edit_dialog(bfwin, 2, data);
}
void span_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	block_tag_edit_dialog(bfwin, 3, data);
}
void h1_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	block_tag_edit_dialog(bfwin, 4, data);
}
void h2_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	block_tag_edit_dialog(bfwin, 5, data);
}
void h3_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	block_tag_edit_dialog(bfwin, 6, data);
}
void h4_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	block_tag_edit_dialog(bfwin, 7, data);
}
void h5_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	block_tag_edit_dialog(bfwin, 8, data);
}
void h6_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	block_tag_edit_dialog(bfwin, 9, data);
}



static void quickruleok_lcb(GtkWidget * widget, Thtml_diag * dg) {
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<HR"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry), cap("ALIGN"), thestring, NULL);
	thestring = insert_integer_if_spin(dg->spin[1], cap("SIZE"), thestring, FALSE, 1);
	thestring = insert_integer_if_spin(dg->spin[2], cap("WIDTH"), thestring, GTK_TOGGLE_BUTTON(dg->check[1])->active,GTK_TOGGLE_BUTTON(dg->check[1])->active ? 100 : 0);
	thestring = insert_attr_if_checkbox(dg->check[2], main_v->props.xhtml == 1 ? cap("NOSHADE=\"noshade\"") : cap("NOSHADE"), thestring);
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

void quickrule_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	GList *popdownlist = NULL;

	static gchar *hritems[] = { "align", "size", "width", "noshade", NULL };
	gchar *hrvalues[5];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable;
	GtkAdjustment *dgadj;
	
	dg = html_diag_new(bfwin, _("Horizontal Rule"));
	fill_dialogvalues(hritems, hrvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 10);
	gtk_table_set_row_spacings(GTK_TABLE(dgtable), 12);

	popdownlist = g_list_insert(popdownlist, "center", 0);
	popdownlist = g_list_insert(popdownlist, "left", 1);
	popdownlist = g_list_insert(popdownlist, "right", 2);
	dg->combo[1] = combo_with_popdown(hrvalues[0], popdownlist, 1);
	g_list_free(popdownlist);
	bf_mnemonic_label_tad_with_alignment(_("_Align:"), dg->combo[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(dg->combo[1]), 1, 4, 0, 1);

	dgadj = (GtkAdjustment *) gtk_adjustment_new((gfloat) 1, 0.0, 200.0, 1.0, 5.0, 0.0);
	dg->spin[1] = gtk_spin_button_new(dgadj, 1, 0);
	/* gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin1), 1); */
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1, 2, 1, 2);
	bf_mnemonic_label_tad_with_alignment(_("_Height:"), dg->spin[1], 0, 0.5, dgtable, 0, 1, 1, 2);
	parse_integer_for_dialog(hrvalues[1], dg->spin[1], NULL, NULL);

	dgadj = (GtkAdjustment *) gtk_adjustment_new((gfloat) 50, 0.0, 600.0, 1.0, 5.0, 0.0);
	dg->spin[2] = gtk_spin_button_new(dgadj, 1, 0);
	/*   gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin2), 1); */
	bf_mnemonic_label_tad_with_alignment(_("_Width:"), dg->spin[2], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[2], 1, 2, 2, 3);

	dg->check[1] = gtk_check_button_new_with_mnemonic(_("Is _percent"));
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 3, 4, 2, 3);

	parse_integer_for_dialog(hrvalues[2], dg->spin[2] , NULL, dg->check[1]);

	dg->check[2] = gtk_check_button_new_with_mnemonic(_("No _shading"));
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[2], 1, 2, 3, 4);
	parse_existence_for_dialog(hrvalues[3], dg->check[2]);

	dg->entry[1] = entry_with_text(custom, 1024);
	bf_mnemonic_label_tad_with_alignment(_("Custo_m:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 10, 4, 5);

	html_diag_finish(dg, G_CALLBACK(quickruleok_lcb));

	if (custom)	g_free(custom);
}

#ifndef HAVE_ATLEAST_GTK_2_4
static void quickstart_ok_lcb(GtkWidget * widget, Thtml_diag * dg) {
	gchar *tmpchar, *tmpchar1, *tmpchar2, *tmpchar3, *finalstring;
	GList *tmplist;
	gchar *text;

	recent_attribs.dtd_cblist =
		add_to_stringlist(recent_attribs.dtd_cblist, gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(dg->combo[1])->entry)));

	if(strstr(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(dg->combo[1])->entry)), "XHTML")) {
		if (dg->check[0] && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[0]))) {
			tmpchar = g_strconcat("<?xml version=\"1.0\" encoding=\"", main_v->props.newfile_default_encoding, "\"?>\n", NULL);
		} else {
			tmpchar = g_strdup("");
		}
		if (strcmp(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(dg->combo[1])->entry)), "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">") == 0 ) {
			tmpchar1 = g_strdup_printf("%s%s\n%shttp://www.w3.org/1999/xhtml%sen\">\n%s\n", tmpchar, gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(dg->combo[1])->entry)), cap("<HTML XMLNS=\""), cap("\" XML:LANG=\""),  cap("<HEAD>"));
		} else {
			tmpchar1 = g_strdup_printf("%s%s\n%shttp://www.w3.org/1999/xhtml%sen%sen\">\n%s\n", tmpchar, gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(dg->combo[1])->entry)), cap("<HTML XMLNS=\""), cap("\" XML:LANG=\""), cap("\" LANG=\""), cap("<HEAD>"));
		}
		g_free(tmpchar);	
	} else {
		tmpchar1 = g_strdup_printf("%s\n%s\n", gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(dg->combo[1])->entry)), cap("<HTML>\n<HEAD>"));
	}

	tmplist = g_list_first(GTK_CLIST(dg->clist[1])->selection);
	while (tmplist) {
		DEBUG_MSG("tmplist->data=%d\n", GPOINTER_TO_INT(tmplist->data));
		gtk_clist_get_text(GTK_CLIST(dg->clist[1]), GPOINTER_TO_INT(tmplist->data), 0, &text);
		tmpchar2 = g_strconcat(tmpchar1, text,"\n",NULL);
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

void quickstart_doctype_changed_cb(GtkWidget* widget, Thtml_diag *dg) {
	if (dg->check[0] != NULL) {
		if(strstr(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(dg->combo[1])->entry)), "XHTML")) {
			gtk_widget_set_sensitive(GTK_WIDGET(dg->check[0]), TRUE);
		} else {
			gtk_widget_set_sensitive(GTK_WIDGET(dg->check[0]), FALSE);
		}
	}				
}

void quickstart_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	GList *tmplist;
	gchar *text[1];
	GtkWidget *scrolwin, *dgtable, *label;
	Thtml_diag *dg;
	
	dg = html_diag_new(bfwin,_("Quick Start"));

	dgtable = html_diag_table_in_vbox(dg, 10, 4);

	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, "");
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, 
		"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">");
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, 
		"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">");
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, 
		"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\"  \"http://www.w3.org/TR/REC-html40/loose.dtd\">");
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, 
		"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Frameset//EN\" \"http://www.w3.org/TR/REC-html40/frameset.dtd\">");	
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, 
		"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">");
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, 
		"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">");
	recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, 
		"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN\" \"http://www.w3.org/TR/html4/frameset.dtd\">");
	
	if (main_v->props.xhtml == 1)
	{
		recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, 
			"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">");
		recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, 
			"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">");
		recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, 
			"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Frameset//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd\">");
		recent_attribs.dtd_cblist = add_to_stringlist(recent_attribs.dtd_cblist, 
			"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">" );
	}
	else
	{
		recent_attribs.dtd_cblist = remove_from_stringlist(recent_attribs.dtd_cblist, 
			"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">");
		recent_attribs.dtd_cblist = remove_from_stringlist(recent_attribs.dtd_cblist, 
			"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">");
		recent_attribs.dtd_cblist = remove_from_stringlist(recent_attribs.dtd_cblist, 
			"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Frameset//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd\">");
		recent_attribs.dtd_cblist = remove_from_stringlist(recent_attribs.dtd_cblist, 
			"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">" );
	}
	
	if (main_v->props.xhtml == 1) {
		dg->combo[1] = combo_with_popdown("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">", recent_attribs.dtd_cblist, 1);
	} else {
		dg->combo[1] = combo_with_popdown("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">", recent_attribs.dtd_cblist, 1);
	}
	gtk_widget_set_size_request(dg->combo[1], 425, -1);
	bf_mnemonic_label_tad_with_alignment(_("_Doctype:"), dg->combo[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[1], 1, 4, 0, 1);
	g_signal_connect(G_OBJECT(GTK_ENTRY(GTK_COMBO(dg->combo[1])->entry)), "changed", G_CALLBACK(quickstart_doctype_changed_cb), dg);
	
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	label = gtk_label_new(_("Header tags:"));
	gtk_table_attach(GTK_TABLE(dgtable), label , 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_set_size_request(scrolwin, 300, 100);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), scrolwin, 1, 4, 1, 4);

	dg->clist[1] = gtk_clist_new(1);
	gtk_clist_set_selection_mode(GTK_CLIST(dg->clist[1]), GTK_SELECTION_MULTIPLE);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolwin), dg->clist[1]);
	gtk_clist_freeze(GTK_CLIST(dg->clist[1]));
	{		
		gchar *tmpstr;
		const gchar *endstr = main_v->props.xhtml == 1 ? " />" : ">";
		tmpstr = g_strconcat("<meta name=\"generator\" content=\"Bluefish\"",endstr,NULL);
		recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, tmpstr);
		g_free(tmpstr);
		tmpstr = g_strconcat("<meta name=\"author\" content=\"\"",endstr,NULL);
		recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, tmpstr);
		g_free(tmpstr);
		tmpstr = g_strconcat("<meta name=\"date\" content=\"\"",endstr,NULL);
		recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, tmpstr);
		g_free(tmpstr);
		tmpstr = g_strconcat("<meta name=\"copyright\" content=\"\"",endstr,NULL);
		recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, tmpstr);
		g_free(tmpstr);
		tmpstr = g_strconcat("<meta name=\"keywords\" content=\"\"",endstr,NULL);
		recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, tmpstr);
		g_free(tmpstr);
		tmpstr = g_strconcat("<meta name=\"description\" content=\"\"",endstr,NULL);
		recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, tmpstr);
		g_free(tmpstr);
		tmpstr = g_strconcat("<meta name=\"ROBOTS\" content=\"NOINDEX, NOFOLLOW\"", endstr, NULL);
		recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, tmpstr);
		g_free(tmpstr);		
		tmpstr = g_strconcat("<meta http-equiv=\"content-type\" content=\"text/html; charset=",main_v->props.newfile_default_encoding,"\"",endstr,NULL);
		recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, tmpstr);
		g_free(tmpstr);
		if (main_v->props.xhtml == 1) {
			tmpstr = g_strconcat("<meta http-equiv=\"content-type\" content=\"application/xhtml+xml; charset=",main_v->props.newfile_default_encoding,"\"",endstr,NULL);			
			recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, tmpstr);
			g_free(tmpstr);
		}
		tmpstr = g_strconcat("<meta http-equiv=\"expires\" content=\"0\"", endstr, NULL);
		recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, tmpstr);
		g_free(tmpstr);
		tmpstr = g_strconcat("<meta http-equiv=\"refresh\" content=\"5; URL=http://\"", endstr, NULL);
		recent_attribs.headerlist = add_to_stringlist(recent_attribs.headerlist, tmpstr);
		g_free(tmpstr);	
	}
	tmplist = g_list_first(recent_attribs.headerlist);
	while (tmplist) {
		text[0] = tmplist->data;
		gtk_clist_append(GTK_CLIST(dg->clist[1]), text);
		tmplist = g_list_next(tmplist);
	}
	gtk_clist_thaw(GTK_CLIST(dg->clist[1]));

	dg->entry[1] = entry_with_text(NULL, 0);
	bf_mnemonic_label_tad_with_alignment(_("_Title:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 4, 5);
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
		label = gtk_label_new_with_mnemonic(_("St_yle:"));
		gtk_table_attach(GTK_TABLE(dgtable), label, 0, 1, 5, 6, GTK_FILL, GTK_FILL, 0, 0);
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), dg->text[0]);
		gtk_widget_set_size_request(scroll, 300,100);
		gtk_table_attach_defaults(GTK_TABLE(dgtable), scroll, 1, 3, 5, 9);
		stylebut = style_but_new_for_wizard(dg->text[0]);
		gtk_table_attach(GTK_TABLE(dgtable), stylebut, 3, 4, 5, 6, GTK_EXPAND, GTK_EXPAND, 0, 0);
	}

	if (main_v->props.xhtml == 1) {
		gchar *tmpstr;
		tmpstr = g_strconcat(" _Include: ", "<?xml version=\"1.0\" encoding=\"", main_v->props.newfile_default_encoding, "\"?>", NULL);
		dg->check[0] = checkbut_with_value(tmpstr, 0);
		gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[0], 1, 2, 9, 10);
		g_free(tmpstr);
	} else {
		dg->check[0] = NULL;
	}
	
	html_diag_finish(dg, G_CALLBACK(quickstart_ok_lcb));
}
#endif /* HAVE_ATLEAST_GTK_2_4 */

static void bodyok_lcb(GtkWidget * widget, Thtml_diag *dg) {
	Tbfwin *bfwin = dg->bfwin;
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
	thestring = insert_string_if_entry(dg->entry[6], cap("ONLOAD"), thestring, NULL);
	thestring = insert_string_if_entry(dg->entry[7], cap("ONUNLOAD"), thestring, NULL);

	thestring = insert_string_if_entry(dg->entry[2], NULL, thestring, NULL);
	finalstring = g_strconcat(thestring, ">\n", NULL);
	g_free(thestring);

	if (dg->entry[1]) {
		bfwin->session->colorlist = add_entry_to_stringlist(bfwin->session->colorlist, GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry));
		bfwin->session->colorlist = add_entry_to_stringlist(bfwin->session->colorlist, GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry));
		bfwin->session->colorlist = add_entry_to_stringlist(bfwin->session->colorlist, GTK_WIDGET(GTK_COMBO(dg->combo[3])->entry));
		bfwin->session->colorlist = add_entry_to_stringlist(bfwin->session->colorlist, GTK_WIDGET(GTK_COMBO(dg->combo[4])->entry));
		bfwin->session->colorlist = add_entry_to_stringlist(bfwin->session->colorlist, GTK_WIDGET(GTK_COMBO(dg->combo[5])->entry));
	}
	bfwin->session->classlist = add_entry_to_stringlist(bfwin->session->classlist, GTK_WIDGET(GTK_COMBO(dg->combo[6])->entry));

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</BODY>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}

	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void body_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	static gchar *bodyitems[] = { "background", "bgcolor", "text", "link", "vlink", "alink"
									, "style", "class", "id", "language", "onload", "onunload", NULL };

	GtkWidget *color_but, *file_but, *dgtable[3], *stylebut, *noteb, *hbox, *vbox2, *label, *frame;
	gchar *bodyvalues[13];
	gchar *custom = NULL;
	Thtml_diag *dg;

	dg = html_diag_new(bfwin,_("Body"));
	fill_dialogvalues(bodyitems, bodyvalues, &custom, (Ttagpopup *) data, dg);

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
		bf_mnemonic_label_tad_with_alignment(_("St_yle:"), dg->entry[3], 0, 0.5, dgtable[1], 0, 1, 0, 1);

		stylebut = style_but_new(dg->entry[3], dg->dialog);
		gtk_table_attach(GTK_TABLE(dgtable[1]), stylebut, 2, 3, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);

		dg->combo[6] = combo_with_popdown(bodyvalues[7], bfwin->session->classlist, 1);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[1]), dg->combo[6], 1, 2, 1, 2);
		bf_mnemonic_label_tad_with_alignment(_("Cl_ass:"), dg->combo[6], 0, 0.5, dgtable[1], 0, 1, 1, 2);

		dg->entry[4] = entry_with_text(bodyvalues[8], 256);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[1]), dg->entry[4], 1, 2, 2, 3);
		bf_mnemonic_label_tad_with_alignment(_("_ID:"), dg->entry[4], 0, 0.5, dgtable[1], 0, 1, 2, 3);

		dg->entry[5] = entry_with_text(bodyvalues[9], 256);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[1]), dg->entry[5], 1, 2, 3, 4);
		bf_mnemonic_label_tad_with_alignment(_("_Language:"), dg->entry[5], 0, 0.5, dgtable[1], 0, 1, 3, 4);

		dg->entry[2] = entry_with_text(custom, 1024);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[1]), dg->entry[2], 1, 3, 4, 5);
		bf_mnemonic_label_tad_with_alignment(_("Custo_m:"), dg->entry[2], 0, 0.5, dgtable[1], 0, 1, 4, 5);

		/* Events Tab */
		frame = bf_generic_frame_new(NULL, GTK_SHADOW_NONE, 12);
		gtk_notebook_append_page(GTK_NOTEBOOK(noteb), frame, gtk_label_new(_("Events")));

		vbox2 = gtk_vbox_new(FALSE, 12);
		gtk_container_add(GTK_CONTAINER(frame), vbox2);
		
		hbox = gtk_hbox_new(FALSE, 12);
		dg->entry[6] = entry_with_text(bodyvalues[10], 256);
		label = gtk_label_new_with_mnemonic(_("On_Load:"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), dg->entry[6]);
		gtk_box_pack_start(GTK_BOX(hbox), dg->entry[6], TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

		hbox = gtk_hbox_new(FALSE, 12);
		dg->entry[7] = entry_with_text(bodyvalues[11], 256);
		label = gtk_label_new_with_mnemonic(_("On_UnLoad:"));
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), dg->entry[7]);
		gtk_box_pack_start(GTK_BOX(hbox), dg->entry[7], TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
		
		/* Depreciated options tab */
	if (!(!main_v->props.allow_dep && !data)){
	        frame = bf_generic_frame_new(NULL, GTK_SHADOW_NONE, 12);
		gtk_notebook_append_page(GTK_NOTEBOOK(noteb), frame, gtk_label_new(_("Depreciated options")));
		dgtable[0] = gtk_table_new(6, 3, FALSE);
		gtk_table_set_row_spacings(GTK_TABLE(dgtable[0]), 6);
		gtk_table_set_col_spacings(GTK_TABLE(dgtable[0]), 12);
		gtk_container_add(GTK_CONTAINER(frame), dgtable[0]);

		dg->entry[1] = entry_with_text(bodyvalues[0], 256);
		file_but = file_but_new(dg->entry[1], 0, bfwin);

		gtk_table_attach(GTK_TABLE(dgtable[0]), GTK_WIDGET(file_but), 2, 3, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
		bf_mnemonic_label_tad_with_alignment(_("Background _Image:"), dg->entry[1], 0, 0.5, dgtable[0], 0, 1, 0, 1);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[0]), dg->entry[1], 1, 2, 0, 1);

		dg->combo[1] = combo_with_popdown(bodyvalues[1], bfwin->session->colorlist, 1);
		color_but = color_but_new(GTK_COMBO(dg->combo[1])->entry, dg->dialog);
		gtk_table_attach(GTK_TABLE(dgtable[0]), color_but, 2, 3, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[0]), dg->combo[1], 1, 2, 1, 2);
		bf_mnemonic_label_tad_with_alignment(_("Back_ground Color:"), dg->combo[1], 0, 0.5, dgtable[0], 0, 1, 1, 2);

		dg->combo[2] = combo_with_popdown(bodyvalues[2], bfwin->session->colorlist, 1);
		color_but = color_but_new(GTK_COMBO(dg->combo[2])->entry, dg->dialog);
		gtk_table_attach(GTK_TABLE(dgtable[0]), color_but, 2, 3, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[0]), dg->combo[2], 1, 2, 2, 3);
		bf_mnemonic_label_tad_with_alignment(_("_Text Color:"), dg->combo[2], 0, 0.5, dgtable[0], 0, 1, 2, 3);

		dg->combo[3] = combo_with_popdown(bodyvalues[3], bfwin->session->colorlist, 1);
		color_but = color_but_new(GTK_COMBO(dg->combo[3])->entry, dg->dialog);
		gtk_table_attach(GTK_TABLE(dgtable[0]), color_but, 2, 3, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[0]), dg->combo[3], 1, 2, 3, 4);
		bf_mnemonic_label_tad_with_alignment(_("_Link Color:"), dg->combo[3], 0, 0.5, dgtable[0], 0, 1, 3, 4);

		dg->combo[4] = combo_with_popdown(bodyvalues[4], bfwin->session->colorlist, 1);
		color_but = color_but_new(GTK_COMBO(dg->combo[4])->entry, dg->dialog);
		gtk_table_attach(GTK_TABLE(dgtable[0]), color_but, 2, 3, 4, 5, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[0]), dg->combo[4], 1, 2, 4, 5);
		bf_mnemonic_label_tad_with_alignment(_("_Visited Link Color:"), dg->combo[4], 0, 0.5, dgtable[0], 0, 1, 4, 5);

		dg->combo[5] = combo_with_popdown(bodyvalues[5], bfwin->session->colorlist, 1);
		color_but = color_but_new(GTK_COMBO(dg->combo[5])->entry, dg->dialog);
		gtk_table_attach(GTK_TABLE(dgtable[0]), color_but, 2, 3, 5, 6, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[0]), dg->combo[5], 1, 2, 5, 6);
		bf_mnemonic_label_tad_with_alignment(_("_Active Link Color:"), dg->combo[5], 0, 0.5, dgtable[0], 0, 1, 5, 6);

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
	thestring = insert_string_if_entry(dg->entry[2], cap("LANG"), thestring, NULL);
	if (main_v->props.xhtml == 1) {
		thestring = insert_string_if_entry(dg->entry[2], cap("XML:LANG"), thestring, NULL);
	}
	thestring = insert_string_if_entry(dg->entry[3], cap("SCHEME"), thestring, NULL);
	thestring = insert_string_if_entry(dg->entry[4], NULL, thestring, NULL);
	finalstring = g_strconcat(thestring,main_v->props.xhtml == 1 ? " />" : ">", NULL);
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, NULL);
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}


void meta_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	GList *popuplist = NULL;

	static gchar *tagitems[] = { "http-equiv", "name", "content", "lang", "scheme", NULL };
	gchar *tagvalues[6];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable;
	
	dg = html_diag_new(bfwin,_("Meta"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 10);

	popuplist = g_list_append(NULL, "abstract");
	popuplist = g_list_append(popuplist, "audience");
	popuplist = g_list_append(popuplist, "author");
	popuplist = g_list_append(popuplist, "copyright");
	popuplist = g_list_append(popuplist, "date");
	popuplist = g_list_append(popuplist, "description");
	popuplist = g_list_append(popuplist, "keywords");
	popuplist = g_list_append(popuplist, "page-topic");
	popuplist = g_list_append(popuplist, "page-type");
	popuplist = g_list_append(popuplist, "publisher");
	popuplist = g_list_append(popuplist, "revisit-after");
	popuplist = g_list_append(popuplist, "robots");
	dg->combo[2] = combo_with_popdown(tagvalues[1], popuplist, 1);
	bf_mnemonic_label_tad_with_alignment(_("_Name:"), dg->combo[2], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[2])), 1, 10, 0, 1);
	g_list_free(popuplist);

	/* extra field for Dublin Core meta tags: DC.foo ?*/
	
	popuplist = g_list_append(NULL, "expires");
	popuplist = g_list_append(popuplist, "refresh");
	popuplist = g_list_append(popuplist, "content-encoding");
	popuplist = g_list_append(popuplist, "content-location");
	popuplist = g_list_append(popuplist, "content-language");
	popuplist = g_list_append(popuplist, "content-style-type");
	popuplist = g_list_append(popuplist, "content-script-type");
	popuplist = g_list_append(popuplist, "content-type");
	popuplist = g_list_append(popuplist, "ext-cache");
	popuplist = g_list_append(popuplist, "cache-control");
	popuplist = g_list_append(popuplist, "pragma");
	popuplist = g_list_append(popuplist, "set-cookie");
	popuplist = g_list_append(popuplist, "PICS-Label");
	dg->combo[1] = combo_with_popdown(tagvalues[0], popuplist, 1);
	bf_mnemonic_label_tad_with_alignment(_("_HTTP-EQUIV:"), dg->combo[1], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[1])), 1, 10, 1, 2);
	g_list_free(popuplist);

	dg->entry[1] = entry_with_text(tagvalues[2], 512);
	bf_mnemonic_label_tad_with_alignment(_("Con_tent:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 10, 2, 3);

	dg->entry[2] = entry_with_text(tagvalues[3], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Language:"), dg->entry[2], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 10, 3, 4);
	
	dg->entry[3] = entry_with_text(tagvalues[4], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Scheme:"), dg->entry[3], 0, 0.5, dgtable, 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[3], 1, 10, 4, 5);

	dg->entry[4] = entry_with_text(custom, 1024);
	bf_mnemonic_label_tad_with_alignment(_("Custo_m:"), dg->entry[4], 0, 0.5, dgtable, 0, 1, 5, 6);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[4], 1, 10, 5, 6);

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

	dg->bfwin->session->fontlist = add_entry_to_stringlist(dg->bfwin->session->fontlist, GTK_WIDGET(GTK_COMBO(dg->combo[3])->entry));

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

static void generalfontdialog_cb(gint type, Tbfwin *bfwin, Ttagpopup *data) {
	GList *popdownlist = NULL;
	GtkWidget *color_but;

	static gchar *tagitems[] = { "size", "face", "color", NULL };
	gchar *tagvalues[4];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable;

	switch(type) {
		case 1:
			dg = html_diag_new(bfwin,_("Font"));
		break;
		case 2:
			dg = html_diag_new(bfwin,_("Base Font"));
		break;
		default:
			return;
		break;
	}
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

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

	dg->combo[2] = combo_with_popdown(tagvalues[2], bfwin->session->colorlist, 1);
	color_but = color_but_new(GTK_COMBO(dg->combo[2])->entry, dg->dialog);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Color")), 6, 7, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[2])), 7, 9, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(color_but), 9, 10, 0, 1);

	dg->combo[3] = combo_with_popdown(tagvalues[1], bfwin->session->fontlist, 1);
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

void font_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	generalfontdialog_cb(1, bfwin,data);
}

void basefont_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	generalfontdialog_cb(2, bfwin,data);
}

static void emailok_lcb(GtkWidget * widget, Thtml_diag *dg)
{
	gchar *finalstring, *tmpstr, *cc, *bcc, *subj, *body;
	gchar *urlencoded = NULL;
	gboolean have_questionmark = FALSE;
	tmpstr = g_strconcat(cap("<A HREF=\"mailto:")
			, gtk_entry_get_text(GTK_ENTRY(dg->entry[1]))
			, NULL);
	if (strlen(gtk_entry_get_text(GTK_ENTRY(dg->entry[2])))) {
		cc = g_strconcat("?cc=", gtk_entry_get_text(GTK_ENTRY(dg->entry[2])), NULL);
		have_questionmark = TRUE;
	} else {
		cc = g_strdup("");
	}
	if (strlen(gtk_entry_get_text(GTK_ENTRY(dg->entry[3])))) {
		if (have_questionmark) {
			bcc = g_strconcat("&amp;bcc=", gtk_entry_get_text(GTK_ENTRY(dg->entry[3])), NULL);
		} else {
			bcc = g_strconcat("?bcc=", gtk_entry_get_text(GTK_ENTRY(dg->entry[3])), NULL);
			have_questionmark = TRUE;
		}		
	} else {
		bcc = g_strdup("");
	}
	if (strlen(gtk_entry_get_text(GTK_ENTRY(dg->entry[4])))) {
#ifdef HAVE_GNOME_VFS
		urlencoded = gnome_vfs_escape_string(gtk_entry_get_text(GTK_ENTRY(dg->entry[4])));
#endif /* HAVE_GNOME_VFS */
		if (have_questionmark) {
#ifdef HAVE_GNOME_VFS
			subj = g_strconcat("&amp;subject=", urlencoded, NULL);
#else
			subj = g_strconcat("&amp;subject=", gtk_entry_get_text(GTK_ENTRY(dg->entry[4])), NULL);
#endif /* HAVE_GNOME_VFS */
		} else {
#ifdef HAVE_GNOME_VFS
			subj = g_strconcat("?subject=", urlencoded, NULL);
#else
			subj = g_strconcat("?subject=", gtk_entry_get_text(GTK_ENTRY(dg->entry[4])), NULL);
#endif /* HAVE_GNOME_VFS */
			have_questionmark = TRUE;			
		}		
	} else {
		subj = g_strdup("");
	}
	if (strlen(gtk_entry_get_text(GTK_ENTRY(dg->entry[5])))) {
#ifdef HAVE_GNOME_VFS
		urlencoded = gnome_vfs_escape_string(gtk_entry_get_text(GTK_ENTRY(dg->entry[5])));
#endif /* HAVE_GNOME_VFS */
		if (have_questionmark) {
#ifdef HAVE_GNOME_VFS
			body = g_strconcat("&amp;body=", urlencoded, NULL);
#else
			body = g_strconcat("&amp;body=", gtk_entry_get_text(GTK_ENTRY(dg->entry[5])), NULL);
#endif /* HAVE_GNOME_VFS */
		} else {
#ifdef HAVE_GNOME_VFS
			body = g_strconcat("?body=", urlencoded, NULL);
#else
			body = g_strconcat("?body=", gtk_entry_get_text(GTK_ENTRY(dg->entry[5])), NULL);
#endif /* HAVE_GNOME_VFS */
			have_questionmark = TRUE;			
		}		
	} else {
		body = g_strdup("");
	}
	finalstring = g_strconcat(tmpstr, cc, bcc, subj, body, "\">", NULL);
	g_free(tmpstr);
	g_free(cc);
	g_free(bcc);
	g_free(subj);
	g_free(body);
	g_free(urlencoded);
	doc_insert_two_strings(dg->doc, finalstring, cap("</A>"));
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void email_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	GtkWidget *dgtable;
	Thtml_diag *dg;
	
	dg = html_diag_new(bfwin,_("E-mail"));

	dgtable = gtk_table_new(5, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(dgtable), 12);
	gtk_table_set_row_spacings(GTK_TABLE(dgtable), 12);
	gtk_box_pack_start(GTK_BOX(dg->vbox), dgtable, FALSE, FALSE, 0);

	dg->entry[1] = gtk_entry_new_with_max_length(256);
	bf_mnemonic_label_tad_with_alignment(_("_Mail to:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 2, 0, 1);

	dg->entry[2] = gtk_entry_new_with_max_length(256);
	bf_mnemonic_label_tad_with_alignment(_("_Copy to:"), dg->entry[2], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 1, 2, 1, 2);

	dg->entry[3] = gtk_entry_new_with_max_length(256);
	bf_mnemonic_label_tad_with_alignment(_("Blin_d copy to:"), dg->entry[3], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[3], 1, 2, 2, 3);
	
	dg->entry[4] = gtk_entry_new_with_max_length(256);
#ifdef HAVE_GNOME_VFS
	bf_mnemonic_label_tad_with_alignment(_("with _Subject:"), dg->entry[4], 0, 0.5, dgtable, 0, 1, 3, 4);
#else
	bf_mnemonic_label_tad_with_alignment(_("with _Subject (URL-encoded):"), dg->entry[4], 0, 0.5, dgtable, 0, 1, 3, 4);
#endif /* HAVE_GNOME_VFS */
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[4], 1, 2, 3, 4);

	dg->entry[5] = gtk_entry_new_with_max_length(256);
#ifdef HAVE_GNOME_VFS
	bf_mnemonic_label_tad_with_alignment(_("with _Body:"), dg->entry[5], 0, 0.5, dgtable, 0, 1, 4, 5);
#else
	bf_mnemonic_label_tad_with_alignment(_("with _Body (URL-encoded):"), dg->entry[5], 0, 0.5, dgtable, 0, 1, 4, 5);
#endif /* HAVE_GNOME_VFS */
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[5], 1, 2, 4, 5);

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
			strcat(finalstring,  cap("\n\t<LI></LI>"));
		}
		strcat(finalstring, "\n");

		if (active) {
			doc_insert_two_strings(dg->doc, finalstring, cap("</OL>"));
		} else {
			doc_insert_two_strings(dg->doc, finalstring, cap("</UL>"));
		}
		g_free (finalstring);
	}
	html_diag_destroy_cb(NULL, dg);
}

void quicklist_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	Thtml_diag *dg;
	GtkWidget *dgtable;

	dg = html_diag_new(bfwin,_("Quick List"));

	dgtable = html_diag_table_in_vbox(dg, 2, 10);
	dg->spin[1] = spinbut_with_value("3", 0, 500, 1.0, 5.0);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dg->spin[1]), 1);
	bf_mnemonic_label_tad_with_alignment(_("_Rows:"), dg->spin[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1, 2, 0, 1);

	dg->radio[1] = gtk_radio_button_new_with_mnemonic(NULL, _("Or_dered"));
	dg->radio[2] = gtk_radio_button_new_with_mnemonic(gtk_radio_button_get_group(GTK_RADIO_BUTTON(dg->radio[1])), _("_Unordered"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dg->radio[1]), TRUE);
	bf_mnemonic_label_tad_with_alignment(_("Style:"), NULL, 0, 0.5, dgtable, 0, 1, 1, 2);
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


void frameset_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	static gchar *tagitems[] = { "cols", "rows", "border", "spacing", NULL };
	gchar *tagvalues[5];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable;

	dg = html_diag_new(bfwin,_("Frameset"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 10);

	dg->entry[1] = entry_with_text(tagvalues[0], 256);
	bf_mnemonic_label_tad_with_alignment(_("Co_ls:"), dg->entry[1], 0, 0.5, dgtable, 0, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 2, 10, 0, 1);

	dg->entry[2] = entry_with_text(tagvalues[1], 256);
	bf_mnemonic_label_tad_with_alignment(_("_Rows:"), dg->entry[2], 0, 0.5, dgtable, 0, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[2], 2, 10, 1, 2);

	if (dg->range.end == -1) {
		dg->check[1] = gtk_check_button_new();
		bf_mnemonic_label_tad_with_alignment(_("_Add <frame> elements"), dg->check[1], 0, 0.5, dgtable, 3, 4, 2, 3);
		gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 2, 3, 2, 3);
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
	bf_mnemonic_label_tad_with_alignment(_("Custo_m:"), dg->entry[3], 0, 0.5, dgtable, 0, 2, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[3], 2, 10, 3, 4);
	html_diag_finish(dg, G_CALLBACK(framesetdialogok_lcb));

	if (custom)	g_free(custom);
}


static void framedialogok_lcb(GtkWidget * widget, Thtml_diag *dg) {
	Tbfwin *bfwin = dg->bfwin;
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

	bfwin->session->targetlist = add_entry_to_stringlist(bfwin->session->targetlist, GTK_WIDGET(GTK_COMBO(dg->combo[2])->entry));
	bfwin->session->urllist = add_entry_to_stringlist(bfwin->session->urllist, GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry));

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


void frame_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	GList *popuplist = NULL;
	static gchar *tagitems[] =
		{ "src", "name", "frameborder", "scrolling", "marginwidth", "marginheight", "noresize", NULL };
	gchar *tagvalues[9];
	gchar *custom = NULL;
	
	GtkWidget *dgtable, *file_but;
	Thtml_diag *dg;

	dg = html_diag_new(bfwin,_("Frame"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 10);

	dg->combo[1] = combo_with_popdown(tagvalues[0], bfwin->session->urllist, 1);
	file_but = file_but_new(GTK_WIDGET(GTK_COMBO(dg->combo[1])->entry), 0, bfwin);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(file_but), 9, 10, 0, 1);
	bf_mnemonic_label_tad_with_alignment(_("_Source:"), dg->combo[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_COMBO(dg->combo[1])), 1, 9, 0, 1);

	dg->combo[2] = combo_with_popdown_sized(tagvalues[1], bfwin->session->targetlist, 1, 90);
	bf_mnemonic_label_tad_with_alignment(_("_Name:"), dg->combo[2], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[2], 1, 5, 1, 2);

	dg->spin[1] = spinbut_with_value(tagvalues[4], 0, 500, 1.0, 5.0);
	bf_mnemonic_label_tad_with_alignment(_("Margin _Width:"), dg->spin[1], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1, 5, 2, 3);

	dg->spin[2] = spinbut_with_value(tagvalues[5], 0, 500, 1.0, 5.0);
	bf_mnemonic_label_tad_with_alignment(_("Margin _Height:"), dg->spin[2], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[2], 1, 5, 3, 4);

	popuplist = g_list_append(popuplist, "auto");
	popuplist = g_list_append(popuplist, "yes");
	popuplist = g_list_append(popuplist, "no");
	dg->combo[3] = combo_with_popdown_sized(tagvalues[3], popuplist, 0, 90);
	g_list_free(popuplist);
	bf_mnemonic_label_tad_with_alignment(_("Scrollin_g:"), dg->combo[3], 0, 0.5, dgtable, 5, 6, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[3], 6, 10, 1, 2);

	dg->spin[0] = spinbut_with_value(tagvalues[2], 0, 1, 1.0, 1.0);
	bf_mnemonic_label_tad_with_alignment(_("_Frameborder:"), dg->spin[0], 0, 0.5, dgtable, 5, 6, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[0], 6, 10, 2, 3);

	dg->check[1] = gtk_check_button_new();
	parse_existence_for_dialog(tagvalues[6], dg->check[1]);
	bf_mnemonic_label_tad_with_alignment(_("No _Resize:"), dg->check[1], 0, 0.5, dgtable, 5, 6, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 6, 10, 3, 4);

/* THIS IS NO HTML
	adj = (GtkAdjustment *) gtk_adjustment_new((gfloat) 1, 0.0, 500.0, 1.0, 5.0, 0.0);
	spin3 = gtk_spin_button_new(adj, 1, 0);
	parse_integer_for_dialog(tagvalues[5], spin3, NULL, NULL);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin3), 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Spacing")), 4, 5, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), spin3, 5, 8, 3, 4); */

	dg->entry[1] = entry_with_text(custom, 1024);
	bf_mnemonic_label_tad_with_alignment(_("Custo_m:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 4, 5);
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
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_ENTRY(dg->spin[2])), cap("HEIGHT"), thestring, NULL);
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

void embed_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	GtkWidget *file_but;
	GtkWidget *dgtable;
	Thtml_diag *dg;

	dg = html_diag_new(bfwin,_("Embed"));

	dgtable = html_diag_table_in_vbox(dg, 5, 4);
	dg->entry[1] = gtk_entry_new_with_max_length(256);
	file_but = file_but_new(dg->entry[1], 0, bfwin);
	gtk_table_attach(GTK_TABLE(dgtable), file_but, 3, 4, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	bf_mnemonic_label_tad_with_alignment(_("_Source:"), dg->entry[1], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 1, 3, 0, 1);

	dg->combo[1] = combo_with_popdown("", recent_attribs.positionlist, 1);
	bf_mnemonic_label_tad_with_alignment(_("_Align:"), dg->combo[1], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(dg->combo[1]), 1, 2, 1, 2);

	dg->spin[1] = spinbut_with_value("", 0, 10000, 1.0, 5.0);
	bf_mnemonic_label_tad_with_alignment(_("_Width:"), dg->spin[1], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1, 2, 2, 3);

	dg->spin[2] = spinbut_with_value("", 0, 10000, 1.0, 5.0);
	bf_mnemonic_label_tad_with_alignment(_("_Height:"), dg->spin[2], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[2], 1, 2, 3, 4);

	dg->spin[3] = spinbut_with_value("", 0, 500, 1.0, 5.0);
	bf_mnemonic_label_tad_with_alignment(_("Bo_rder:"), dg->spin[3], 0, 0.5, dgtable, 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[3], 1, 2, 4, 5);

	html_diag_finish(dg, G_CALLBACK(embedok_lcb));	
}

static void scriptok_lcb(GtkWidget * widget,Thtml_diag *dg ) {
	gchar *thestring, *finalstring, *endstring;

	thestring = g_strdup(cap("<SCRIPT"));
	thestring = insert_string_if_entry(dg->entry[0], cap("SRC"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_COMBO(dg->combo[0])->entry, cap("LANGUAGE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_COMBO(dg->combo[1])->entry, cap("TYPE"), thestring, NULL);
	thestring = insert_string_if_entry(dg->entry[1], NULL, thestring, NULL);
	finalstring = g_strconcat(thestring, ">", NULL);
	endstring = cap("</SCRIPT>");
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, endstring);
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void script_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	GtkWidget *file_but;
	GList *tmplist, *tmplist2;
	GtkWidget *dgtable, *label;
	Thtml_diag *dg;
	static gchar *scriptitems[] =	{ "src", "language", "type", NULL };
	gchar *tagvalues[4];
	gchar *custom = NULL;

	dg = html_diag_new(bfwin,_("Script"));
	fill_dialogvalues(scriptitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 4, 12);
	dg->entry[0] = entry_with_text(tagvalues[0], 1024);
	file_but = file_but_new(dg->entry[0], 0, bfwin);
	gtk_table_attach(GTK_TABLE(dgtable), file_but, 10, 12, 0, 1, GTK_EXPAND, GTK_EXPAND, 0, 0);
	bf_mnemonic_label_tad_with_alignment(_("_Source:"), dg->entry[0], 0, 0.5, dgtable, 0, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[0], 2, 10, 0, 1);


		tmplist = g_list_append(NULL, "javascript");
		tmplist = g_list_append(tmplist, "javascript1.2");
		tmplist = g_list_append(tmplist, "javascript1.1");
		label = gtk_label_new_with_mnemonic(_("_Language:"));
		tmplist2 = g_list_append(NULL, "application/javascript");
		tmplist2 = g_list_append(NULL, "text/javascript");
		tmplist2 = g_list_append(tmplist2, "application/x-javascript");
	tmplist2 = g_list_append(tmplist2, "text/plain");
	tmplist2 = g_list_append(tmplist2, "text/html");

	dg->combo[0] = combo_with_popdown(tagvalues[1], tmplist, 1);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), (GTK_COMBO(dg->combo[0])->entry));
	gtk_entry_set_activates_default(GTK_ENTRY(GTK_COMBO(dg->combo[0])->entry), TRUE);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), label, 0, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[0], 2, 6, 1, 2);

	dg->combo[1] = combo_with_popdown(tagvalues[2], tmplist2, 1);
	bf_mnemonic_label_tad_with_alignment(_("_Type:"), dg->combo[1], 0, 0.5, dgtable, 0, 2, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[1], 2, 6, 2, 3);

	dg->entry[1] = entry_with_text(custom, 1024);
	bf_mnemonic_label_tad_with_alignment(_("Custo_m:"), dg->entry[1], 0, 0.5, dgtable, 0, 2, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->entry[1], 2, 10, 3, 4);

	html_diag_finish(dg, G_CALLBACK(scriptok_lcb));
	g_list_free(tmplist);
	g_list_free(tmplist2);
}

static void linkdialogok_lcb(GtkWidget * widget, Thtml_diag *dg) {
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<LINK"));
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->attrwidget[0])->entry), cap("HREF"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->attrwidget[1]), cap("HREFLANG"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->attrwidget[2]), cap("TITLE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->attrwidget[3])->entry), cap("TYPE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->attrwidget[4])->entry), cap("REL"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->attrwidget[5])->entry), cap("REV"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(dg->attrwidget[6])->entry), cap("MEDIA"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->attrwidget[7]), cap("LANG"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(dg->attrwidget[8]), NULL, thestring, NULL);
	finalstring = g_strconcat(thestring, (main_v->props.xhtml == 1) ? " />" : ">", NULL);
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, NULL);
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void linkdialog_dialog(Tbfwin *bfwin, Ttagpopup *data, Tlinkdialog_mode mode) {
	static gchar *tagitems[] = {"href", "hreflang", "title", "type", "rel", "rev", "media", "lang", NULL };
	gchar *tagvalues[9];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable, *but;
	
	dg = html_diag_new(bfwin,_("Link"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);
	dgtable = html_diag_table_in_vbox(dg, 3, 10);
	{
		GList *rel_link_list=NULL, *tmplist;
		rel_link_list = list_relative_document_filenames(bfwin->current_document);
		tmplist = duplicate_stringlist(bfwin->session->urllist, 1);
		rel_link_list = g_list_concat(tmplist, rel_link_list);
		dg->attrwidget[0] = combo_with_popdown(tagvalues[0], rel_link_list, 1);
		free_stringlist(rel_link_list);
	}
	bf_mnemonic_label_tad_with_alignment(_("_HREF:"), dg->attrwidget[0], 0, 0.5, dgtable, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->attrwidget[0], 1, 2, 0, 1);
	but = file_but_new(GTK_COMBO(dg->attrwidget[0])->entry, 0, bfwin);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), but, 2, 3, 0, 1);

	dg->attrwidget[1] = entry_with_text(tagvalues[1], 1024);
	bf_mnemonic_label_tad_with_alignment(_("HREF_LANG:"), dg->attrwidget[1], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->attrwidget[1], 1, 3, 1, 2);

	dg->attrwidget[2] = entry_with_text(tagvalues[2], 1024);
	bf_mnemonic_label_tad_with_alignment(_("T_itle:"), dg->attrwidget[2], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->attrwidget[2], 1, 3, 2, 3);
	
	{
		gchar *tmp;
		GList *tmplist = list_from_arglist(FALSE, "text/html", "text/css", "text/plain", "text/javascript", "application/postscript", NULL);
		
		switch (mode) {
			case linkdialog_mode_default:
				tmp = tagvalues[3];
			break;
			case linkdialog_mode_css:
				tmp = "text/css";
			break;
			default:
				tmp = tagvalues[3];
			break;				
		}
		dg->attrwidget[3] = combo_with_popdown(tmp, tmplist, 1);

		g_list_free(tmplist);
	}
	bf_mnemonic_label_tad_with_alignment(_("_Type:"), dg->attrwidget[3], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->attrwidget[3], 1, 3, 3, 4);
	
	{
		gchar *tmp;
		GList *tmplist = list_from_arglist(FALSE, "stylesheet", "alternate", "alternate stylesheet", "shortcut icon", "contents", "chapter", "section", "subsection", "index", "glossary", "appendix", "search", "author", "copyright", "next", "prev", "first", "last", "up", "top", "help", "bookmark", NULL);

		switch (mode) {
			case linkdialog_mode_default:
				tmp = tagvalues[4];
			break;
			case linkdialog_mode_css:
				tmp = "stylesheet";
			break;
			default:
				tmp = tagvalues[4];
			break;				
		}
		dg->attrwidget[4] = combo_with_popdown(tmp, tmplist, 1);
		bf_mnemonic_label_tad_with_alignment(_("_Forward Relation:"), dg->attrwidget[4], 0, 0.5, dgtable, 0, 1, 4, 5);
		gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->attrwidget[4], 1, 3, 4, 5);
		
		dg->attrwidget[5] = combo_with_popdown(tagvalues[5], tmplist, 1);
		bf_mnemonic_label_tad_with_alignment(_("_Reverse Relation:"), dg->attrwidget[5], 0, 0.5, dgtable, 0, 1, 5, 6);
		gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->attrwidget[5], 1, 3, 5, 6);
		
		g_list_free(tmplist);
	}
	
	{
		GList *tmplist = list_from_arglist(FALSE, "screen", "tty", "tv", "projection", "handheld", "print", "braille", "aural", "all", NULL);
		dg->attrwidget[6] = combo_with_popdown(tagvalues[6], tmplist, 1);
		g_list_free(tmplist);
	}
	bf_mnemonic_label_tad_with_alignment(_("Media:"), dg->attrwidget[6], 0, 0.5, dgtable, 0, 1, 6, 7);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->attrwidget[6], 1, 3, 6, 7);

	dg->attrwidget[7] = entry_with_text(tagvalues[7], 1024);
	bf_mnemonic_label_tad_with_alignment(_("L_ang:"), dg->attrwidget[7], 0, 0.5, dgtable, 0, 1, 7, 8);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->attrwidget[7], 1, 3, 7, 8);
	
	dg->attrwidget[8] = entry_with_text(custom, 1024);
	bf_mnemonic_label_tad_with_alignment(_("_Custom:"), dg->attrwidget[8], 0, 0.5, dgtable, 0, 1, 8, 9);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->attrwidget[8], 1, 3, 8, 9);
	
	html_diag_finish(dg, G_CALLBACK(linkdialogok_lcb));
	if (custom)	g_free(custom);
}	
