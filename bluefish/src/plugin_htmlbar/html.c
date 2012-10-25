/* Bluefish HTML Editor
 * html.c - menu/toolbar callbacks, inserting functions, and other cool stuff
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * modifications Copyright (C) 1999 Antti-Juhani Kaijanaho
 * rewrite November 2000 (C) Olivier Sessink
 * Copyright (C) 1999-2012 Olivier Sessink
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

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>				/* strtod() */
#include <string.h>
#include <time.h>

#include "htmlbar.h"
#include "html.h"
#include "html_diag.h"			/* the new html dialog stuff  */
#include "html2.h"				/* style_but_new */
#include "cap.h"				/* cap() */
#include "../bf_lib.h"			/* main_v */
#include "../dialog_utils.h"
#include "../document.h"
#include "../gtk_easy.h"
#include "../stringlist.h"


/*****************************************************************************/

void
htmlbar_insert_special_char(Tbfwin * bfwin, TBFSpecialChars spchar)
{
	const gchar *sp_chars[] = {
		NULL, "&nbsp;", "&brvbar;", "&shy;"
	};

	doc_insert_two_strings(bfwin->current_document, sp_chars[spchar], NULL);
}

void
general_html_menu_cb(Tbfwin * bfwin, guint callback_action, GtkWidget * widget)
{
	gchar *tmp;
	switch (callback_action) {
	case 1:
		doc_insert_two_strings(bfwin->current_document, cap("<B>"), cap("</B>"));
		break;
	case 2:
		doc_insert_two_strings(bfwin->current_document, cap("<I>"), cap("</I>"));
		break;
	case 3:
		doc_insert_two_strings(bfwin->current_document,
							   main_v->props.xhtml == 					   1 ? cap("<SPAN STYLE=\"text-decoration: underline;\">") : cap("<U>")
							   , main_v->props.xhtml == 1 ? cap("</SPAN>") : cap("</U>"));
		break;
	case 4:
		doc_insert_two_strings(bfwin->current_document, cap("<STRIKE>"), cap("</STRIKE>"));
		break;
	case 5:
		doc_insert_two_strings(bfwin->current_document, cap("<P>"), cap("</P>"));
		break;
	case 6:
		doc_insert_two_strings(bfwin->current_document,
							   main_v->props.xhtml == 1 ? cap("<br />") : cap("<br>"), NULL);
		break;
	case 7:
		doc_insert_two_strings(bfwin->current_document, "&nbsp;", NULL);
		break;
	case 8:
		doc_insert_two_strings(bfwin->current_document,
							   main_v->props.xhtml == 					   1 ? cap("<DIV STYLE=\"text-align: center\">") : cap("<DIV ALIGN=\"CENTER\">"),
							   cap("</DIV>"));
		break;
	case 9:
		doc_insert_two_strings(bfwin->current_document,
							   main_v->props.xhtml == 					   1 ? cap("<DIV STYLE=\"text-align: right\">") : cap("<DIV ALIGN=\"RIGHT\">"),
							   cap("</DIV>"));
		break;
	case 10:
		doc_insert_two_strings(bfwin->current_document, "<!-- ", " -->");
		break;
	case 11:
		doc_insert_two_strings(bfwin->current_document,
							   main_v->props.xhtml == 					   1 ? cap("<SPAN STYLE=\"font-size: larger;\">") : cap("<FONT SIZE=\"+1\">")
							   , main_v->props.xhtml == 1 ? cap("</SPAN>") : cap("</FONT>"));
		break;
	case 12:
		doc_insert_two_strings(bfwin->current_document,
							   main_v->props.xhtml == 					   1 ? cap("<SPAN STYLE=\"font-size: smaller;\">") : cap("<FONT SIZE=\"-1\">")
							   , main_v->props.xhtml == 1 ? cap("</SPAN>") : cap("</FONT>"));
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
		doc_insert_two_strings(bfwin->current_document,
							   main_v->props.xhtml == 1 ? cap("<FRAME />") : cap("<FRAME>"), NULL);
		break;
	case 31:
		doc_insert_two_strings(bfwin->current_document, cap("<NOFRAMES>"), cap("</NOFRAMES>"));
		break;
	case 32:
		doc_insert_two_strings(bfwin->current_document,
							   main_v->props.xhtml == 					   1 ? cap("<BASE TARGET=\"\" />") : cap("<BASE TARGET=\"\">"), NULL);
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
		doc_insert_two_strings(bfwin->current_document,
							   main_v->props.xhtml == 					   1 ? cap("<BR CLEAR=\"ALL\" />") : cap("<BR CLEAR=\"ALL\">"), NULL);
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
		tmp = 	g_strdup_printf("<%s=\"javascript\" %s=\"text/javascript\"><!--\n", cap("SCRIPT LANGUAGE"),
							cap("TYPE"));
		doc_insert_two_strings(bfwin->current_document, tmp, cap("\n//--></SCRIPT>"));
		g_free(tmp);
		break;
/*  case 45:
    tmp = g_strdup_printf("<%s=\"javascript\" %s>", cap("SCRIPT LANGUAGE"), cap("SRC=\"\" TYPE=\"text/javascript\""));
    doc_insert_two_strings(bfwin->current_document, tmp, cap("</SCRIPT>"));
    g_free(tmp);
    break; */
/*  case 46:
    tmp = g_strdup_printf("<%s=\"stylesheet\" %s=\"\" %s=\"text/css\">", cap("LINK REL"), cap("HREF"), cap("TYPE"));
    doc_insert_two_strings(bfwin->current_document, tmp, "");
    g_free(tmp);
    break; */
	case 47:
		tmp = g_strconcat(cap("<META NAME=\"Generator\" CONTENT=\"")
						  , "Bluefish ", VERSION, " http://bluefish.openoffice.nl/",
						  main_v->props.xhtml == 1 ? "\" />\n" : "\">\n", NULL);
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
		doc_insert_two_strings(bfwin->current_document,
							   main_v->props.xhtml == 1 ? cap("<LINK />") : cap("<LINK>"), NULL);
		break;
	case 62:
		doc_insert_two_strings(bfwin->current_document, cap("<DIV>"), cap("</DIV>"));
		break;
	case 63:
		doc_insert_two_strings(bfwin->current_document, cap("<A>"), cap("</A>"));
		break;
	case 64:
		doc_insert_two_strings(bfwin->current_document,
							   main_v->props.xhtml == 1 ? cap("<IMG />") : cap("<IMG>"), NULL);
		break;
	case 65:
		doc_insert_two_strings(bfwin->current_document,
							   main_v->props.xhtml == 1 ? cap("<INPUT />") : cap("<INPUT>"), NULL);
		break;
	case 66:
		doc_insert_two_strings(bfwin->current_document, cap("<TEXTAREA>"), cap("</TEXTAREA>"));
		break;
	case 67:
		doc_insert_two_strings(bfwin->current_document, cap("<FORM>"), cap("</FORM>"));
		break;
	case 68:
		doc_insert_two_strings(bfwin->current_document, cap("<INS>"), cap("</INS>"));
		break;
	case 69:
		doc_insert_two_strings(bfwin->current_document, cap("<DEL>"), cap("</DEL>"));
		break;
	case 70:
		doc_insert_two_strings(bfwin->current_document, cap("<ARTICLE>"), cap("</ARTICLE>"));
		break;
	default:
		break;
	}
}


/************************************************************************/

static gchar *
extract_time_string(const gchar *original_string)
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


static void
insert_time_destroy_lcb(GtkWidget * widget, TimeInsert * data)
{
	DEBUG_MSG("insert_time_destroy_lcb, data=%p\n", data);
	window_destroy(data->dialog);
	g_free(data);
}

static void
insert_time_callback(GtkWidget * widget, TimeInsert * timeinsert)
{
	const gchar *temp_string;
	gchar *insert_string;
	gchar *final_string;
	gint count;

	insert_string = g_malloc0(32);
	final_string = g_malloc0(255);
	for (count = 1; count < 7; count++) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(timeinsert->check[count]))) {
			temp_string = gtk_label_get_text(GTK_LABEL(timeinsert->label[count]));
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

static void
insert_time_cancel(GtkWidget * widget, TimeInsert * data)
{
	DEBUG_MSG("insert_time_cancel, data=%p\n", data);
	insert_time_destroy_lcb(widget, data);
}

/************************************************************************/
void
insert_time_dialog(Tbfwin * bfwin)
{

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
	timeinsert->dialog = window_full(_("Insert Time"), GTK_WIN_POS_MOUSE, 12, G_CALLBACK(insert_time_destroy_lcb), timeinsert,
					TRUE);
	vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(timeinsert->dialog), vbox);

	for (count = 1; count < 7; count++) {
		switch (count) {
		case 1:
			temp = 		g_strdup_printf(_("  _Time (%i:%i:%i)"), time_struct->tm_hour, time_struct->tm_min, time_struct->tm_sec);
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
				break;
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
			{
				gchar *tmpstrtime = bf_portable_time(&time_var);
				temp = g_strdup_printf(_("  Unix Date _String (%s)"), tmpstrtime);
				g_free(tmpstrtime);
			}
			/* Replace \n on ')' */
			temp[strlen(temp) - 1] = ')';
			break;
		case 6:
			{
				gchar *tmpstr = _("  ISO-8601 Ti_me ");
#ifdef WIN32
				glong hours, mins;
				gchar gmtsign;

				strftime(isotime, 30, "%Y-%m-%dT%H:%M:%S", time_struct);
				gmtsign = _timezone > 0 ? '-' : '+';
				hours = abs(_timezone) / 3600;
				mins = (abs(_timezone) % 3600) / 60;
				temp = g_strconcat(tmpstr, g_strdup_printf("(%s%c%02ld%02ld)", 						   isotime, gmtsign, hours, mins), NULL);

#else							/* WIN32 */
				strftime(isotime, 30, "%Y-%m-%dT%H:%M:%S%z", time_struct);
				temp = g_strconcat(tmpstr, g_strdup_printf("(%s)", isotime), NULL);
#endif							/* WIN32 */
			}
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
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 10);
#if GTK_CHECK_VERSION(3,0,0)
	hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
	hbox = gtk_hbutton_box_new();
#endif
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(hbox), 12);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

	ok_b = bf_stock_ok_button(G_CALLBACK(insert_time_callback), (gpointer) timeinsert);
	gtk_window_set_default(GTK_WINDOW(timeinsert->dialog), ok_b);
	cancel_b = bf_stock_cancel_button(G_CALLBACK(insert_time_cancel), (gpointer) timeinsert);
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

static void
quickanchorok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	gchar *thestring, *finalstring;
	Tbfwin *bfwin = dg->bfwin;

	thestring = g_strdup(cap("<A"));
	thestring = insert_string_if_entry(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(dg->combo[2]))), cap("HREF"), thestring,
							   NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[1]), cap("TARGET"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[2]), cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[4]), NULL, thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[5]), cap("ONCLICK"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[6]), cap("ONDBLCLICK"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[7]), cap("ONMOUSEOVER"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[8]), cap("ONMOUSEDOWN"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[9]), cap("ONMOUSEMOVE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[10]), cap("ONMOUSEOUT"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[11]), cap("ONMOUSEUP"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[12]), cap("ONKEYDOWN"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[13]), cap("ONKEYPRESS"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[14]), cap("ONKEYUP"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(dg->attrwidget[0]))), cap("CLASS"),
							   thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->attrwidget[1]), cap("ID"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->attrwidget[2]), cap("STYLE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[17]), cap("LANG"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[18]), cap("TITLE"), thestring, NULL);
	finalstring = g_strdup_printf("%s>", thestring);
	g_free(thestring);

	bfwin->session->urllist = add_entry_to_stringlist(bfwin->session->urllist, GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[2]))));
	bfwin->session->targetlist = add_entry_to_stringlist(bfwin->session->targetlist, GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[1]))));
	bfwin->session->classlist = add_entry_to_stringlist(bfwin->session->classlist, GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->attrwidget[0]))));

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</A>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

static gboolean
string_looks_like_url(gchar *string)
{
	gint len;
	if (string == NULL)
		return FALSE;
	if (strchr(string, '\n')!=NULL)
		return FALSE;
	if (strchr(string, '\t')!=NULL)
		return FALSE;
	if (strchr(string, '\r')!=NULL)
		return FALSE;
	if (strncmp(string,"http://", 7)==0 ||  
	 			strncmp(string,"https://", 8)==0 ||
	 			strncmp(string,"ftp://", 6)==0) {
	 	return TRUE;
	}
	/* look for an extension and no spaces */
	if (g_regex_match_simple("^[\\./a-z0-9]+\\.[a-z]{3,5}$",string,G_REGEX_CASELESS|G_REGEX_ANCHORED,0)) {
		/*g_print("%s matches regex\n",string);*/
		return TRUE;
	}
	return FALSE;
}

void
quickanchor_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	static gchar *aitems[] = { "href", "target", "name", "onkeyup", "onclick", "ondblclick", "onmouseover", "onmousedown",
"onmousemove", "onmouseout", "onmouseup", "onkeydown", "onkeypress", "class", "id", "style", "lang", "title", NULL };
	gchar *custom = NULL;
	gchar *avalues[19];
	gchar *url=NULL;
	Thtml_diag *dg;
	gint so, eo;
	GList *rel_link_list = NULL, *tmplist;


	GtkWidget *noteb, *dgtable, *file_but;
	DEBUG_MSG("quickanchor_dialog, bfwin=%p, data=%p\n", bfwin, data);
	dg = html_diag_new(bfwin, _("Quick Anchor"));
	fill_dialogvalues(aitems, avalues, &custom, (Ttagpopup *) data, dg);

	noteb = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(dg->vbox), noteb, FALSE, FALSE, 0);

	dgtable = generic_table_inside_notebookframe(noteb, _("Attributes"), 9,3);
	if (avalues[0] != NULL && avalues[0][0] != '\0') {
		url = g_strdup(avalues[0]);
	} else if (doc_get_selection(bfwin->current_document, &so, &eo)){
		gchar *tmp = doc_get_chars(bfwin->current_document, so, eo);
		/* if the current selected text is an URL, use it */
		if (string_looks_like_url(tmp)) {
			url = tmp;
		} else {
			g_free(tmp);
		}
	}
	rel_link_list = list_relative_document_filenames(bfwin->current_document);
	tmplist = duplicate_stringlist(bfwin->session->urllist, 1);
	rel_link_list = g_list_concat(tmplist, rel_link_list);
	dg->combo[2] = html_diag_combobox_with_popdown(url, rel_link_list, 1);

	free_stringlist(rel_link_list);
	g_free(url);
	file_but = file_but_new(GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[2]))), 0, bfwin);
	gtk_table_attach(GTK_TABLE(dgtable), GTK_WIDGET(file_but), 2,3, 0,1, GTK_SHRINK, GTK_SHRINK, 0,0);
	dialog_mnemonic_label_in_table(_("_HREF:"), dg->combo[2], dgtable, 0,1, 0,1);
	gtk_table_attach(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[2])), 1,2, 0,1, GTK_EXPAND|GTK_FILL, GTK_SHRINK, 0,0);
	if (bfwin->session->targetlist == NULL) {
		bfwin->session->targetlist = list_from_arglist(TRUE, "_top", "_blank", "_parent", "_selfs", NULL);
	}
	dg->combo[1] = html_diag_combobox_with_popdown(avalues[1], bfwin->session->targetlist, 1);
	dialog_mnemonic_label_in_table(_("_Target:"), dg->combo[1], dgtable, 0,1, 1,2);
	gtk_table_attach(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[1])), 1,3, 1,2, GTK_EXPAND|GTK_FILL,GTK_SHRINK, 0,0);

	dg->entry[2] = dialog_entry_in_table(avalues[2], dgtable, 1,3, 2,3);
	dialog_mnemonic_label_in_table(_("_Name:"), dg->entry[2], dgtable, 0,1, 2,3);

	dg->entry[18] = dialog_entry_in_table(avalues[17], dgtable, 1,3, 3,4);
	dialog_mnemonic_label_in_table(_("Titl_e:"), dg->entry[18], dgtable, 0,1, 3,4);

	generic_class_id_style_section(dg, 0, dgtable, 4, avalues, 13);

	dg->entry[17] = dialog_entry_in_table(avalues[16], dgtable, 1,3, 7,8);
	dialog_mnemonic_label_in_table(_("_Language:"), dg->entry[17], dgtable, 0,1, 7,8);

	dg->entry[4] = dialog_entry_in_table(custom, dgtable, 1,3, 8,9);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dg->entry[4], dgtable, 0,1, 8,9);

	dgtable = generic_table_inside_notebookframe(noteb, _("Events"), 10,2);

	dg->entry[5] = dialog_entry_in_table(avalues[4], dgtable, 1,2, 0,1);
	dialog_mnemonic_label_in_table(_("OnClic_k:"), dg->entry[5], dgtable, 0,1, 0,1);

	dg->entry[6] = dialog_entry_in_table(avalues[5], dgtable, 1,2, 1,2);
	dialog_mnemonic_label_in_table(_("OnD_blClick:"), dg->entry[6], dgtable, 0,1, 1,2);

	dg->entry[7] = dialog_entry_in_table(avalues[6], dgtable, 1,2, 2,3);
	dialog_mnemonic_label_in_table(_("OnMouseO_ver:"), dg->entry[7], dgtable, 0,1, 2,3);

	dg->entry[8] = dialog_entry_in_table(avalues[7], dgtable, 1,2, 3,4);
	dialog_mnemonic_label_in_table(_("OnMouse_Down:"), dg->entry[8], dgtable, 0,1, 3,4);

	dg->entry[9] = dialog_entry_in_table(avalues[8], dgtable, 1,2, 4,5);
	dialog_mnemonic_label_in_table(_("OnMouse_Move:"), dg->entry[9], dgtable, 0,1, 4,5);

	dg->entry[10] = dialog_entry_in_table(avalues[9], dgtable, 1,2, 5,6);
	dialog_mnemonic_label_in_table(_("OnMouseOu_t:"), dg->entry[10], dgtable, 0,1, 5,6);

	dg->entry[11] = dialog_entry_in_table(avalues[10], dgtable, 1,2, 6,7);
	dialog_mnemonic_label_in_table(_("OnMouse_Up:"), dg->entry[11], dgtable, 0,1, 6,7);

	dg->entry[12] = dialog_entry_in_table(avalues[11], dgtable, 1,2, 7,8);
	dialog_mnemonic_label_in_table(_("OnKeyDo_wn:"), dg->entry[12], dgtable, 0,1, 7,8);

	dg->entry[13] = dialog_entry_in_table(avalues[12], dgtable, 1,2, 8,9);
	dialog_mnemonic_label_in_table(_("OnKey_Press:"), dg->entry[13], dgtable, 0,1, 8,9);

	dg->entry[14] = dialog_entry_in_table(avalues[3], dgtable, 1,2, 9,10);
	dialog_mnemonic_label_in_table(_("OnKe_yUp:"), dg->entry[14], dgtable, 0,1, 9,10);

	html_diag_finish(dg, G_CALLBACK(quickanchorok_lcb));
	gtk_widget_grab_focus(dg->combo[2]);
	if (custom)
		g_free(custom);
}

static void
block_tag_editok_lcb(gint type, Thtml_diag * dg)
{
	Tbfwin *bfwin = dg->bfwin;
	gchar *thestring = NULL, *endstring = NULL, *finalstring;

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

	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[1]), cap("ALIGN"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[2]), cap("CLASS"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[1]), cap("STYLE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[2]), cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[3]), cap("ID"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[4]), NULL, thestring, NULL);
	finalstring = g_strdup_printf("%s>", thestring);
	g_free(thestring);

	bfwin->session->classlist = add_entry_to_stringlist(bfwin->session->classlist, GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[2]))));

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, endstring);
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(endstring);
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

static void
p_editok_lcb(GtkWidget * widget, gpointer data)
{
	block_tag_editok_lcb(1, data);
}

static void
div_editok_lcb(GtkWidget * widget, gpointer data)
{
	block_tag_editok_lcb(2, data);
}

static void
span_editok_lcb(GtkWidget * widget, gpointer data)
{
	block_tag_editok_lcb(3, data);
}

static void
h1_editok_lcb(GtkWidget * widget, gpointer data)
{
	block_tag_editok_lcb(4, data);
}

static void
h2_editok_lcb(GtkWidget * widget, gpointer data)
{
	block_tag_editok_lcb(5, data);
}

static void
h3_editok_lcb(GtkWidget * widget, gpointer data)
{
	block_tag_editok_lcb(6, data);
}

static void
h4_editok_lcb(GtkWidget * widget, gpointer data)
{
	block_tag_editok_lcb(7, data);
}

static void
h5_editok_lcb(GtkWidget * widget, gpointer data)
{
	block_tag_editok_lcb(8, data);
}

static void
h6_editok_lcb(GtkWidget * widget, gpointer data)
{
	block_tag_editok_lcb(9, data);
}

void
block_tag_edit_dialog(Tbfwin * bfwin, gint type, Ttagpopup * data)
{
	gchar *labeltext;
	GList *popuplist = NULL;
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
	dg = html_diag_new(bfwin, labeltext);
	g_free(labeltext);
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5,4);

	popuplist = g_list_append(NULL, "left");
	popuplist = g_list_append(popuplist, "center");
	popuplist = g_list_append(popuplist, "right");
	dg->combo[1] = html_diag_combobox_with_popdown(tagvalues[0], popuplist, 1);
	g_list_free(popuplist);
	dialog_mnemonic_label_in_table(_("Ali_gn:"), dg->combo[1], dgtable, 0,1, 0,1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[1])), 1,2, 0,1);

	dg->combo[2] = html_diag_combobox_with_popdown(tagvalues[1], bfwin->session->classlist, 1);
	dialog_mnemonic_label_in_table(_("Cl_ass:"), dg->combo[2], dgtable, 0,1, 1,2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[2])), 1,2, 1,2);

	dg->entry[1] = dialog_entry_in_table(tagvalues[2], dgtable, 1,3, 2,3);
	dialog_mnemonic_label_in_table(_("St_yle:"), dg->entry[1], dgtable, 0,1, 2,3);

	but = style_but_new(dg->entry[1], dg->dialog);
	gtk_table_attach(GTK_TABLE(dgtable), but, 3,4, 2,3, GTK_SHRINK, GTK_SHRINK, 0,0);

	dg->entry[2] = dialog_entry_in_table(tagvalues[3], dgtable, 3,4, 0,1);
	dialog_mnemonic_label_in_table(_("_Name:"), dg->entry[2], dgtable, 2,3, 0,1);

	dg->entry[3] = dialog_entry_in_table(tagvalues[4], dgtable, 3,4, 1,2);
	dialog_mnemonic_label_in_table(_("_ID:"), dg->entry[3], dgtable, 2,3, 1,2);

	dg->entry[4] = dialog_entry_in_table(custom, dgtable, 1,4, 3,4);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dg->entry[4], dgtable, 0,1, 3,4);

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

void
p_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	block_tag_edit_dialog(bfwin, 1, data);
}

void
div_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	block_tag_edit_dialog(bfwin, 2, data);
}

void
span_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	block_tag_edit_dialog(bfwin, 3, data);
}

void
h1_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	block_tag_edit_dialog(bfwin, 4, data);
}

void
h2_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	block_tag_edit_dialog(bfwin, 5, data);
}

void
h3_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	block_tag_edit_dialog(bfwin, 6, data);
}

void
h4_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	block_tag_edit_dialog(bfwin, 7, data);
}

void
h5_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	block_tag_edit_dialog(bfwin, 8, data);
}

void
h6_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	block_tag_edit_dialog(bfwin, 9, data);
}



static void
quickruleok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<HR"));
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[1]), cap("ALIGN"), thestring, NULL);
	thestring = insert_integer_if_spin(dg->spin[1], cap("SIZE"), thestring, FALSE, 1);
	thestring = insert_integer_if_spin(dg->spin[2], cap("WIDTH"), thestring,
							   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[1])),
							   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[1])) ? 100 : 0);
	thestring = insert_attr_if_checkbox(dg->check[2], main_v->props.xhtml == 1 ? cap("NOSHADE=\"noshade\"") : cap("NOSHADE"), thestring);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[1]), NULL, thestring, NULL);
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

void
quickrule_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
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
	dg->combo[1] = html_diag_combobox_with_popdown(hrvalues[0], popdownlist, 1);
	g_list_free(popdownlist);
	dialog_mnemonic_label_in_table(_("_Align:"), dg->combo[1], dgtable, 0,1, 0,1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(dg->combo[1]), 1,4, 0,1);

	dgadj = (GtkAdjustment *) gtk_adjustment_new((gfloat) 1, 0.0, 200.0, 1.0, 5.0, 0.0);
	dg->spin[1] = gtk_spin_button_new(dgadj, 1,0);
	/* gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin1), 1); */
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1,2, 1,2);
	dialog_mnemonic_label_in_table(_("_Height:"), dg->spin[1], dgtable, 0,1, 1,2);
	parse_integer_for_dialog(hrvalues[1], dg->spin[1], NULL, NULL);

	dgadj = (GtkAdjustment *) gtk_adjustment_new((gfloat) 50, 0.0, 600.0, 1.0, 5.0, 0.0);
	dg->spin[2] = gtk_spin_button_new(dgadj, 1,0);
	/*   gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin2), 1); */
	dialog_mnemonic_label_in_table(_("_Width:"), dg->spin[2], dgtable, 0,1, 2,3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[2], 1,2, 2,3);

	dg->check[1] = gtk_check_button_new_with_mnemonic(_("Is _percent"));
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 3,4, 2,3);

	parse_integer_for_dialog(hrvalues[2], dg->spin[2], NULL, dg->check[1]);

	dg->check[2] = gtk_check_button_new_with_mnemonic(_("No _shading"));
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[2], 1,2, 3,4);
	parse_existence_for_dialog(hrvalues[3], dg->check[2]);

	dg->entry[1] = dialog_entry_in_table(custom, dgtable, 1,10, 4,5);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dg->entry[1], dgtable, 0,1, 4,5);

	html_diag_finish(dg, G_CALLBACK(quickruleok_lcb));

	if (custom)
		g_free(custom);
}

static void
bodyok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	Tbfwin *bfwin = dg->bfwin;
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<BODY"));
	if (dg->entry[1]) {
		thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[1]), cap("BACKGROUND"), thestring, NULL);
		thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[1]), cap("BGCOLOR"), thestring, NULL);
		thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[2]), cap("TEXT"), thestring, NULL);
		thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[3]), cap("LINK"), thestring, NULL);
		thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[4]), cap("VLINK"), thestring, NULL);
		thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[5]), cap("ALINK"), thestring, NULL);
	}
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[3]), cap("STYLE"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[6]), cap("CLASS"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[4]), cap("ID"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[5]), cap("LANG"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[6]), cap("ONLOAD"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[7]), cap("ONUNLOAD"), thestring, NULL);

	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[2]), NULL, thestring, NULL);
	finalstring = g_strconcat(thestring, ">\n", NULL);
	g_free(thestring);

	if (dg->entry[1]) {
		bfwin->session->colorlist = 	add_entry_to_stringlist(bfwin->session->colorlist, 	GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[1]))));
		bfwin->session->colorlist = 	add_entry_to_stringlist(bfwin->session->colorlist, 	GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[2]))));
		bfwin->session->colorlist = 	add_entry_to_stringlist(bfwin->session->colorlist, 	GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[3]))));
		bfwin->session->colorlist = 	add_entry_to_stringlist(bfwin->session->colorlist, 	GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[4]))));
		bfwin->session->colorlist = 	add_entry_to_stringlist(bfwin->session->colorlist, 	GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[5]))));
	}
	bfwin->session->classlist = add_entry_to_stringlist(bfwin->session->classlist, GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[6]))));

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</BODY>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}

	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void
body_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	static gchar *bodyitems[] = { "background", "bgcolor", "text", "link", "vlink", "alink", "style", "class", "id", "language",
"onload", "onunload", NULL
	};

	GtkWidget *color_but, *file_but, *dgtable[3], *stylebut, *noteb, *hbox, *vbox2, *frame;
	gchar *bodyvalues[13];
	gchar *custom = NULL;
	Thtml_diag *dg;

	dg = html_diag_new(bfwin, _("Body"));
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

	dg->entry[3] = dialog_entry_in_table(bodyvalues[6], dgtable[1], 1,2, 0,1);
	dialog_mnemonic_label_in_table(_("St_yle:"), dg->entry[3], dgtable[1], 0,1, 0,1);

	stylebut = style_but_new(dg->entry[3], dg->dialog);
	gtk_table_attach(GTK_TABLE(dgtable[1]), stylebut, 2, 3, 0, 1, GTK_SHRINK, GTK_SHRINK, 0,0);

	dg->combo[6] = html_diag_combobox_with_popdown(bodyvalues[7], bfwin->session->classlist, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable[1]), dg->combo[6], 1,2, 1,2);
	dialog_mnemonic_label_in_table(_("Cl_ass:"), dg->combo[6], dgtable[1], 0,1, 1,2);

	dg->entry[4] = dialog_entry_in_table(bodyvalues[8], dgtable[1], 1,2, 2,3);
	dialog_mnemonic_label_in_table(_("_ID:"), dg->entry[4], dgtable[1], 0,1, 2,3);

	dg->entry[5] = dialog_entry_in_table(bodyvalues[9], dgtable[1], 1,2, 3,4);
	dialog_mnemonic_label_in_table(_("_Language:"), dg->entry[5], dgtable[1], 0,1, 3,4);

	dg->entry[2] = dialog_entry_in_table(custom, dgtable[1], 1,3, 4,5);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dg->entry[2], dgtable[1], 0,1, 4,5);

	/* Events Tab */
	frame = bf_generic_frame_new(NULL, GTK_SHADOW_NONE, 12);
	gtk_notebook_append_page(GTK_NOTEBOOK(noteb), frame, gtk_label_new(_("Events")));

	vbox2 = gtk_vbox_new(FALSE, 12);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	hbox = gtk_hbox_new(FALSE, 12);
	dg->entry[6] = dialog_entry_labeled(bodyvalues[10], _("On _Load:"), hbox, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 12);
	dg->entry[7] = dialog_entry_labeled(bodyvalues[11], _("On _Unload:"), hbox, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

	/* Depreciated options tab */
	if (!(!main_v->props.allow_dep && !data)) {
		frame = bf_generic_frame_new(NULL, GTK_SHADOW_NONE, 12);
		gtk_notebook_append_page(GTK_NOTEBOOK(noteb), frame, gtk_label_new(_("Depreciated options")));
		dgtable[0] = gtk_table_new(6, 3, FALSE);
		gtk_table_set_row_spacings(GTK_TABLE(dgtable[0]), 6);
		gtk_table_set_col_spacings(GTK_TABLE(dgtable[0]), 12);
		gtk_container_add(GTK_CONTAINER(frame), dgtable[0]);

		dg->entry[1] = dialog_entry_in_table(bodyvalues[0], dgtable[0], 1,2, 0,1);
		file_but = file_but_new(dg->entry[1], 0, bfwin);
		gtk_table_attach(GTK_TABLE(dgtable[0]), GTK_WIDGET(file_but), 2,3, 0,1, GTK_FILL, GTK_FILL, 0,0);
		dialog_mnemonic_label_in_table(_("Background _Image:"), dg->entry[1], dgtable[0], 0, 1, 			 0,1);

		dg->combo[1] = html_diag_combobox_with_popdown(bodyvalues[1], bfwin->session->colorlist, 1);
		color_but = color_but_new(gtk_bin_get_child(GTK_BIN(dg->combo[1])), dg->dialog);
		gtk_table_attach(GTK_TABLE(dgtable[0]), color_but, 2,3, 1,2, GTK_FILL, GTK_FILL, 0,0);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[0]), dg->combo[1], 1,2, 1,2);
		dialog_mnemonic_label_in_table(_("Back_ground Color:"), dg->combo[1], dgtable[0], 0, 1, 			 1,2);

		dg->combo[2] = html_diag_combobox_with_popdown(bodyvalues[2], bfwin->session->colorlist, 1);
		color_but = color_but_new(gtk_bin_get_child(GTK_BIN(dg->combo[2])), dg->dialog);
		gtk_table_attach(GTK_TABLE(dgtable[0]), color_but, 2,3, 2,3, GTK_FILL, GTK_FILL, 0,0);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[0]), dg->combo[2], 1,2, 2,3);
		dialog_mnemonic_label_in_table(_("_Text Color:"), dg->combo[2], dgtable[0], 0,1, 2,3);

		dg->combo[3] = html_diag_combobox_with_popdown(bodyvalues[3], bfwin->session->colorlist, 1);
		color_but = color_but_new(gtk_bin_get_child(GTK_BIN(dg->combo[3])), dg->dialog);
		gtk_table_attach(GTK_TABLE(dgtable[0]), color_but, 2,3, 3,4, GTK_FILL, GTK_FILL, 0,0);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[0]), dg->combo[3], 1,2, 3,4);
		dialog_mnemonic_label_in_table(_("_Link Color:"), dg->combo[3], dgtable[0], 0,1, 3,4);

		dg->combo[4] = html_diag_combobox_with_popdown(bodyvalues[4], bfwin->session->colorlist, 1);
		color_but = color_but_new(gtk_bin_get_child(GTK_BIN(dg->combo[4])), dg->dialog);
		gtk_table_attach(GTK_TABLE(dgtable[0]), color_but, 2,3, 4,5, GTK_FILL, GTK_FILL, 0,0);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[0]), dg->combo[4], 1,2, 4,5);
		dialog_mnemonic_label_in_table(_("_Visited Link Color:"), dg->combo[4], dgtable[0], 0, 			 1, 4,5);

		dg->combo[5] = html_diag_combobox_with_popdown(bodyvalues[5], bfwin->session->colorlist, 1);
		color_but = color_but_new(gtk_bin_get_child(GTK_BIN(dg->combo[5])), dg->dialog);
		gtk_table_attach(GTK_TABLE(dgtable[0]), color_but, 2,3, 5,6, GTK_FILL, GTK_FILL, 0,0);
		gtk_table_attach_defaults(GTK_TABLE(dgtable[0]), dg->combo[5], 1,2, 5,6);
		dialog_mnemonic_label_in_table(_("_Active Link Color:"), dg->combo[5], dgtable[0], 0, 1, 			 5,6);

	} else {
		dg->combo[1] = NULL;
		dg->combo[2] = NULL;
		dg->combo[3] = NULL;
		dg->combo[4] = NULL;
		dg->combo[5] = NULL;
		dg->entry[1] = NULL;
	}

	html_diag_finish(dg, G_CALLBACK(bodyok_lcb));

	if (custom)
		g_free(custom);
}

static void
metaok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<META"));
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[1]), cap("HTTP-EQUIV"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[2]), cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[1]), cap("CONTENT"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[2]), cap("LANG"), thestring, NULL);
	if (main_v->props.xhtml == 1) {
		thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[2]), cap("XML:LANG"), thestring, NULL);
	}
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[3]), cap("SCHEME"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[4]), NULL, thestring, NULL);
	finalstring = g_strconcat(thestring, main_v->props.xhtml == 1 ? " />" : ">", NULL);
	g_free(thestring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, NULL);
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}


void
meta_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	GList *popuplist = NULL;

	static gchar *tagitems[] = { "http-equiv", "name", "content", "lang", "scheme", NULL };
	gchar *tagvalues[6];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable;

	dg = html_diag_new(bfwin, _("Meta"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 10);
	popuplist = list_from_arglist(FALSE, "abstract", "audience", "author", "copyright", "date", "description",
						  "generator", "keywords", "page-topic", "page-type", "publisher", "revisit-after",
						  "robots", NULL);
	dg->combo[2] = html_diag_combobox_with_popdown(tagvalues[1], popuplist, 1);
	dialog_mnemonic_label_in_table(_("_Name:"), dg->combo[2], dgtable, 0,1, 0,1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[2])), 1,10, 0,1);
	g_list_free(popuplist);

	/* extra field for Dublin Core meta tags: DC.foo ? */
	popuplist = list_from_arglist(FALSE, "expires", "refresh", "content-encoding", "content-location",
						  "content-language", "content-style-type", "content-script-type", "content-type",
						  "ext-cache", "cache-control", "pragma", "set-cookie", "PICS-Label", NULL);
	dg->combo[1] = html_diag_combobox_with_popdown(tagvalues[0], popuplist, 1);
	dialog_mnemonic_label_in_table(_("_HTTP-EQUIV:"), dg->combo[1], dgtable, 0,1, 1,2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[1])), 1,10, 1,2);
	g_list_free(popuplist);

	dg->entry[1] = dialog_entry_in_table(tagvalues[2], dgtable, 1,10, 2,3);
	dialog_mnemonic_label_in_table(_("Con_tent:"), dg->entry[1], dgtable, 0,1, 2,3);

	dg->entry[2] = dialog_entry_in_table(tagvalues[3], dgtable, 1,10, 3,4);
	dialog_mnemonic_label_in_table(_("_Language:"), dg->entry[2], dgtable, 0,1, 3,4);

	dg->entry[3] = dialog_entry_in_table(tagvalues[4], dgtable, 1,10, 4,5);
	dialog_mnemonic_label_in_table(_("_Scheme:"), dg->entry[3], dgtable, 0,1, 4,5);

	dg->entry[4] = dialog_entry_in_table(custom, dgtable, 1,10, 5,6);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dg->entry[4], dgtable, 0,1, 5,6);

	html_diag_finish(dg, G_CALLBACK(metaok_lcb));
	if (custom)
		g_free(custom);
}

static void
generalfontdialog_lcb(gint type, GtkWidget * widget, Thtml_diag * dg)
{
	gchar *thestring, *finalstring;

	switch (type) {
	case 1:
		thestring = g_strdup(cap("<FONT"));
		break;
	case 2:
		thestring = g_strdup(cap("<BASEFONT"));
		break;
	default:
		DEBUG_MSG_C("uh-oh: generalfontdialog_lcb, unknown action %d\n", type);
		g_return_if_reached();
		break;
	}
	if (strlen(gtk_entry_get_text(GTK_ENTRY(dg->spin[1])))) {
		gchar *sizecombo, *tmpstr;
		sizecombo = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dg->combo[1]));
		if (strlen(sizecombo)) {
			tmpstr = g_strconcat(thestring, cap(" size=\"")
								 , sizecombo, gtk_entry_get_text(GTK_ENTRY(GTK_SPIN_BUTTON(dg->spin[1])))
								 , "\"", NULL);
		} else {
			tmpstr = g_strconcat(thestring, cap(" size=\"")
								 , gtk_entry_get_text(GTK_ENTRY(GTK_SPIN_BUTTON(dg->spin[1])))
								 , "\"", NULL);
		}
		g_free(thestring);
		thestring = tmpstr;
		g_free(sizecombo);
	}
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[2]), cap("COLOR"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[3]), cap("FACE"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[1]), NULL, thestring, NULL);
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

static void
fontok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	generalfontdialog_lcb(1, widget, dg);
}

static void
basefontok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	generalfontdialog_lcb(2, widget, dg);
}

static void
generalfontdialog_cb(gint type, Tbfwin * bfwin, Ttagpopup * data)
{
	GList *popdownlist = NULL;
	GtkWidget *color_but;

	static gchar *tagitems[] = { "size", "face", "color", NULL };
	gchar *tagvalues[4];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable;

	switch (type) {
	case 1:
		dg = html_diag_new(bfwin, _("Font"));
		break;
	case 2:
		dg = html_diag_new(bfwin, _("Base Font"));
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
	dg->combo[1] = html_diag_combobox_with_popdown("", popdownlist, 0);
	g_list_free(popdownlist);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Size")), 0,1, 0,1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[1])), 1,2, 0,1);
	dg->spin[1] = spinbut_with_value(NULL, 0, 100, 1.0, 5.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 2,3, 0,1);
	parse_integer_for_dialog(tagvalues[0], dg->spin[1], GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[1]))),
							 NULL);

	dg->combo[2] = html_diag_combobox_with_popdown(tagvalues[2], bfwin->session->colorlist, 1);
	color_but = color_but_new(gtk_bin_get_child(GTK_BIN(dg->combo[2])), dg->dialog);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Color")), 6,7, 0,1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[2])), 7,9, 0,1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(color_but), 9,10, 0,1);

	dg->combo[3] = html_diag_combobox_with_popdown(tagvalues[1], bfwin->session->fontlist, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Face")), 0,1, 1,2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[3])), 1,10, 1,2);

	dg->entry[1] = dialog_entry_in_table(custom, dgtable, 1,10, 2,3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Custom")), 0,1, 2,3);
	switch (type) {
	case 1:
		html_diag_finish(dg, G_CALLBACK(fontok_lcb));
		break;
	case 2:
		html_diag_finish(dg, G_CALLBACK(basefontok_lcb));
		break;
	}

	if (custom)
		g_free(custom);
}

void
font_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	generalfontdialog_cb(1, bfwin, data);
}

void
basefont_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	generalfontdialog_cb(2, bfwin, data);
}

static void
emailok_lcb(GtkWidget * widget, Thtml_diag * dg)
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
		urlencoded = g_uri_escape_string(gtk_entry_get_text(GTK_ENTRY(dg->entry[4])), NULL, FALSE);
		if (have_questionmark) {
			subj = g_strconcat("&amp;subject=", urlencoded, NULL);
		} else {
			subj = g_strconcat("?subject=", urlencoded, NULL);
			have_questionmark = TRUE;
		}
	} else {
		subj = g_strdup("");
	}
	if (strlen(gtk_entry_get_text(GTK_ENTRY(dg->entry[5])))) {
		urlencoded = g_uri_escape_string(gtk_entry_get_text(GTK_ENTRY(dg->entry[5])), NULL, FALSE);
		if (have_questionmark) {
			body = g_strconcat("&amp;body=", urlencoded, NULL);
		} else {
			body = g_strconcat("?body=", urlencoded, NULL);
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

void
email_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	GtkWidget *dgtable;
	Thtml_diag *dg;

	dg = html_diag_new(bfwin, _("E-mail"));

	dgtable = gtk_table_new(5, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(dgtable), 12);
	gtk_table_set_row_spacings(GTK_TABLE(dgtable), 12);
	gtk_box_pack_start(GTK_BOX(dg->vbox), dgtable, FALSE, FALSE, 0);

	dg->entry[1] = dialog_entry_in_table(NULL, dgtable, 1,2, 0,1);
	dialog_mnemonic_label_in_table(_("_Mail to:"), dg->entry[1], dgtable, 0,1, 0,1);

	dg->entry[2] = dialog_entry_in_table(NULL, dgtable, 1,2, 1,2);
	dialog_mnemonic_label_in_table(_("_Copy to:"), dg->entry[2], dgtable, 0,1, 1,2);

	dg->entry[3] = dialog_entry_in_table(NULL, dgtable, 1,2, 2,3);
	dialog_mnemonic_label_in_table(_("Blin_d copy to:"), dg->entry[3], dgtable, 0,1, 2,3);

	dg->entry[4] = dialog_entry_in_table(NULL, dgtable, 1,2, 3,4);
	dialog_mnemonic_label_in_table(_("with _Subject:"), dg->entry[4], dgtable, 0,1, 3,4);

	dg->entry[5] = dialog_entry_in_table(NULL, dgtable, 1,2, 4,5);
	dialog_mnemonic_label_in_table(_("with _Body:"), dg->entry[5], dgtable, 0,1, 4,5);

	html_diag_finish(dg, G_CALLBACK(emailok_lcb));
}

static void
quicklistok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	gint active, temp, i;
	gchar *finalstring;

	if (strlen(gtk_entry_get_text(GTK_ENTRY(dg->spin[1])))) {
		temp = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dg->spin[1]));
		finalstring = g_malloc((8 + temp * 12) * sizeof(gchar));

		active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->radio[1]));

		if (active) {
			strcpy(finalstring, cap("<OL>"));
		} else {
			strcpy(finalstring, cap("<UL>"));
		}
	/**********************************************/
		/* TODO: use bf_str_repeat() function !!!!!!! */
	/**********************************************/
		for (i = 0; i < temp; i++) {
			strcat(finalstring, cap("\n\t<LI></LI>"));
		}
		strcat(finalstring, "\n");

		if (active) {
			doc_insert_two_strings(dg->doc, finalstring, cap("</OL>"));
		} else {
			doc_insert_two_strings(dg->doc, finalstring, cap("</UL>"));
		}
		g_free(finalstring);
	}
	html_diag_destroy_cb(NULL, dg);
}

void
quicklist_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	Thtml_diag *dg;
	GtkWidget *dgtable;

	dg = html_diag_new(bfwin, _("Quick List"));

	dgtable = html_diag_table_in_vbox(dg, 2, 10);
	dg->spin[1] = spinbut_with_value("3", 0, 500, 1.0, 5.0);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dg->spin[1]), 1);
	dialog_mnemonic_label_in_table(_("Number of _rows:"), dg->spin[1], dgtable, 0,1, 0,1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1,2, 0,1);

	dg->radio[1] = gtk_radio_button_new_with_mnemonic(NULL, _("Or_dered"));
	dg->radio[2] = gtk_radio_button_new_with_mnemonic(gtk_radio_button_get_group(GTK_RADIO_BUTTON(dg->radio[1])), 		   _("_Unordered"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dg->radio[1]), TRUE);
	dialog_mnemonic_label_in_table(_("Style:"), NULL, dgtable, 0,1, 1,2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->radio[1], 1,5, 1,2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->radio[2], 5,10, 1,2);

	html_diag_finish(dg, G_CALLBACK(quicklistok_lcb));

}

static void
framesetdialogok_lcb(GtkWidget * widget, Thtml_diag * dg)
{

	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<FRAMESET"));
	thestring = insert_string_if_entry((GTK_ENTRY(dg->entry[1])), cap("COLS"), thestring, NULL);
	thestring = insert_string_if_entry((GTK_ENTRY(dg->entry[2])), cap("ROWS"), thestring, NULL);
	finalstring = g_strconcat(thestring, ">", NULL);
	g_free(thestring);

	if ((dg->check[1]) && (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[1])))) {
		gint rowcount = 1, colcount = 1, framecount = 0;
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


void
frameset_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	static gchar *tagitems[] = { "cols", "rows", "border", "spacing", NULL };
	gchar *tagvalues[5];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable;

	dg = html_diag_new(bfwin, _("Frameset"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 10);

	dg->entry[1] = dialog_entry_in_table(tagvalues[0], dgtable, 2,10, 0,1);
	dialog_mnemonic_label_in_table(_("Co_ls:"), dg->entry[1], dgtable, 0,2, 0,1);

	dg->entry[2] = dialog_entry_in_table(tagvalues[1], dgtable, 2,10, 1,2);
	dialog_mnemonic_label_in_table(_("_Rows:"), dg->entry[2], dgtable, 0,2, 1,2);

	if (dg->range.end == -1) {
		dg->check[1] = gtk_check_button_new();
		dialog_mnemonic_label_in_table(_("_Add &lt;frame&gt; elements"), dg->check[1], dgtable, 3,4, 2,3);
		gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 2,3, 2,3);
	} else {
		dg->check[1] = NULL;
	}

/* THIS IS NO VALID HTML !!!!
  dg->check[1] = gtk_check_button_new();
  parse_existence_for_dialog(tagvalues[2], dg->check[1]);
  gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Border")), 0,2, 2,3);
  gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 2,4, 2,3);

  dg->spin[1] = spinbut_with_value(tagvalues[3], 0, 500, 1.0, 5.0);
  gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Spacing")), 0,2, 3,4);
  gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 2,4, 3,4);
*/
	dg->entry[3] = dialog_entry_in_table(custom, dgtable, 2,10, 3,4);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dg->entry[3], dgtable, 0,2, 3,4);

	html_diag_finish(dg, G_CALLBACK(framesetdialogok_lcb));

	if (custom)
		g_free(custom);
}


static void
framedialogok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	Tbfwin *bfwin = dg->bfwin;
	gchar *thestring, *finalstring, *tmp;

	thestring = g_strdup(cap("<FRAME"));
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[1]), cap("SRC"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[2]), cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry((GTK_ENTRY(dg->spin[0])), cap("FRAMEBORDER"), thestring, NULL);
	tmp = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dg->combo[3]));	
	if(strlen(tmp)) {
		thestring = g_strconcat(thestring, cap(" SCROLLING=\""), tmp, "\"",NULL);
	} 
	g_free(tmp);
	thestring = insert_string_if_entry((GTK_ENTRY(dg->spin[1])), cap("MARGINWIDTH"), thestring, NULL);
	thestring = insert_string_if_entry((GTK_ENTRY(dg->spin[2])), cap("MARGINHEIGHT"), thestring, NULL);
	thestring = insert_attr_if_checkbox(dg->check[1], main_v->props.xhtml == 1 ? cap("NORESIZE=\"noresize\"") : cap("NORESIZE"), thestring);
	thestring = insert_string_if_entry((GTK_ENTRY(dg->entry[1])), NULL, thestring, NULL);

	bfwin->session->targetlist = add_entry_to_stringlist(bfwin->session->targetlist, GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[2]))));
	bfwin->session->urllist = add_entry_to_stringlist(bfwin->session->urllist, GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[1]))));

	if (main_v->props.xhtml == 1) {
		finalstring = g_strconcat(thestring, " />", NULL);
	} else {
		finalstring = g_strconcat(thestring, ">", NULL);
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

void
frame_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	GList *popuplist = NULL;
	static gchar *tagitems[] = { "src", "name", "frameborder", "scrolling", "marginwidth", "marginheight", "noresize", NULL };
	gchar *tagvalues[9];
	gchar *custom = NULL;

	GtkWidget *dgtable, *file_but;
	Thtml_diag *dg;

	dg = html_diag_new(bfwin, _("Frame"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 5, 10);

	dg->combo[1] = html_diag_combobox_with_popdown(tagvalues[0], bfwin->session->urllist, 1);
	file_but = file_but_new(GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[1]))), 0, bfwin);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(file_but), 9,10, 0,1);
	dialog_mnemonic_label_in_table(_("_Source:"), dg->combo[1], dgtable, 0,1, 0,1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[1])), 1,9, 0,1);

	dg->combo[2] = html_diag_combobox_with_popdown_sized(tagvalues[1], bfwin->session->targetlist, 1, 90);
	dialog_mnemonic_label_in_table(_("_Name:"), dg->combo[2], dgtable, 0,1, 1,2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[2], 1,5, 1,2);

	dg->spin[1] = spinbut_with_value(tagvalues[4], 0, 500, 1.0, 5.0);
	dialog_mnemonic_label_in_table(_("Margin _Width:"), dg->spin[1], dgtable, 0,1, 2,3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1,5, 2,3);

	dg->spin[2] = spinbut_with_value(tagvalues[5], 0, 500, 1.0, 5.0);
	dialog_mnemonic_label_in_table(_("Margin _Height:"), dg->spin[2], dgtable, 0,1, 3,4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[2], 1,5, 3,4);

	popuplist = g_list_insert(popuplist, "auto",0);
	popuplist = g_list_insert(popuplist, "yes",1);
	popuplist = g_list_insert(popuplist, "no",2);
	popuplist = g_list_insert(popuplist, "",3);
	dg->combo[3] = html_diag_combobox_with_popdown("", popuplist, 0);
	g_list_free(popuplist);
	dialog_mnemonic_label_in_table(_("Scrollin_g:"), dg->combo[3], dgtable, 5,6, 1,2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[3], 6,10, 1,2);

	dg->spin[0] = spinbut_with_value(tagvalues[2], 0, 1, 1.0, 1.0);
	dialog_mnemonic_label_in_table(_("_Frameborder:"), dg->spin[0], dgtable, 5,6, 2,3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[0], 6,10, 2,3);

	dg->check[1] = gtk_check_button_new();
	parse_existence_for_dialog(tagvalues[6], dg->check[1]);
	dialog_mnemonic_label_in_table(_("No _Resize:"), dg->check[1], dgtable, 5,6, 3,4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 6,10, 3,4);

/* THIS IS NO HTML
  adj = (GtkAdjustment *) gtk_adjustment_new((gfloat) 1, 0.0, 500.0, 1.0, 5.0, 0.0);
  spin3 = gtk_spin_button_new(adj, 1,0);
  parse_integer_for_dialog(tagvalues[5], spin3, NULL, NULL);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin3), 1);
  gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Spacing")), 4,5, 3,4);
  gtk_table_attach_defaults(GTK_TABLE(dgtable), spin3, 5,8, 3,4); */

	dg->entry[1] = dialog_entry_in_table(custom, dgtable, 1, 10, 4,5);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dg->entry[1], dgtable, 0,1, 4,5);

	html_diag_finish(dg, G_CALLBACK(framedialogok_lcb));

	if (custom)
		g_free(custom);
}

static void
audiodialogok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	gchar *finalstring, *thestring;

	/* Tag construction */
	finalstring = g_strdup(cap("<AUDIO"));
	finalstring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[1]), cap("SRC"), finalstring, NULL);
	finalstring = insert_attr_if_checkbox(dg->check[1], main_v->props.xhtml == 1 ? cap("AUTOPLAY=\"autoplay\"") : cap("AUTOPLAY"), finalstring);
	finalstring = insert_attr_if_checkbox(dg->check[2], main_v->props.xhtml == 1 ? cap("CONTROLS=\"controls\"") : cap("CONTROLS"), finalstring);
	finalstring = insert_attr_if_checkbox(dg->check[3], main_v->props.xhtml == 1 ? cap("LOOP=\"loop\"") : cap("LOOP"), finalstring);
	finalstring = insert_attr_if_checkbox(dg->check[4], main_v->props.xhtml == 1 ? cap("MUTE=\"mute\"") : cap("MUTE"), finalstring);
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->entry[0])), cap("ID"), finalstring, NULL);
	finalstring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[3]), cap("CLASS"), finalstring, NULL);	
	finalstring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[2]), cap("PRELOAD"), finalstring, NULL);
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->entry[2])), cap("STYLE"), finalstring, NULL);
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->entry[3])), NULL, finalstring, NULL);
	thestring = finalstring;
	finalstring = g_strconcat(thestring, ">", NULL);
	g_free(thestring);
	/* Insert tag in document */
	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring,  cap("</AUDIO>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}

	/* Free */
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void
audio_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	/* Controls */
	GtkWidget *dgtable, *but;
	Thtml_diag *dg;
	static gchar *tagitems[] = { "src", "preload", "id", "class", "style", NULL };
	gchar *tagvalues[6];
	gchar *custom=NULL;		

	/* Values attributes lists */
	GList *listPreload;

	/* Dialog construction and settings */
	dg = html_diag_new(bfwin, _("Audio"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);
	dgtable = html_diag_table_in_vbox(dg, 6, 11);

	dg->combo[1] = html_diag_combobox_with_popdown(tagvalues[0], bfwin->session->urllist, 1);
	but = file_but_new(GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[1]))), 0, bfwin);
	gtk_table_attach(GTK_TABLE(dgtable), GTK_WIDGET(but), 9,10, 0,1, GTK_SHRINK,GTK_SHRINK,0,0);
	dialog_mnemonic_label_in_table(_("_Source:"), dg->combo[1], dgtable, 0,1, 0,1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[1])), 1,9, 0,1);

	dg->check[1] = gtk_check_button_new();
	dialog_mnemonic_label_in_table(_("_Autoplay:"), dg->check[1], dgtable, 0,1, 1,2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 1,2, 1,2);	

	dg->check[2] = gtk_check_button_new();
	dialog_mnemonic_label_in_table(_("C_ontrols:"), dg->check[2], dgtable, 2,3, 1,2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[2], 3,4, 1,2);	

	dg->check[3] = gtk_check_button_new();
	dialog_mnemonic_label_in_table(_("_Loop:"), dg->check[3], dgtable, 4,5, 1,2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[3], 5,6, 1,2);

	dg->check[4] = gtk_check_button_new();
	dialog_mnemonic_label_in_table(_("M_ute:"), dg->check[4], dgtable, 7,8, 1,2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[4], 8,9, 1,2);	

	listPreload = list_from_arglist(FALSE, "", "auto", "metadata", "none", NULL);
	dg->combo[2] = html_diag_combobox_with_popdown(tagvalues[1], listPreload, 1);
	dialog_mnemonic_label_in_table(_("_Preload:"), dg->combo[2], dgtable, 0,1, 2,3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(dg->combo[2]), 1,4, 2,3);
	g_list_free(listPreload);

	dg->entry[0] = dialog_entry_in_table(tagvalues[2], dgtable, 5,6, 2,3);
	dialog_mnemonic_label_in_table(_("_Id:"), dg->entry[0], dgtable, 4,5, 2,3);

	dg->combo[3] = html_diag_combobox_with_popdown(tagvalues[3], bfwin->session->classlist, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[3], 8,9, 2,3);
	dialog_mnemonic_label_in_table(_("Cl_ass:"), dg->combo[3], dgtable, 7,8, 2,3);
	
	dg->entry[2] = dialog_entry_in_table(tagvalues[4], dgtable, 1,8 , 3,4);
	dialog_mnemonic_label_in_table(_("_Style:"), dg->entry[2], dgtable, 0,1, 3,4);
	but = style_but_new(dg->entry[2], dg->dialog);
	gtk_table_attach(GTK_TABLE(dgtable), but, 8,10, 3,4, GTK_SHRINK,GTK_SHRINK,0,0);

	dg->entry[3] = dialog_entry_in_table(custom, dgtable, 1,10, 4,5);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dg->entry[3], dgtable, 0,1, 4,5);

	html_diag_finish(dg, G_CALLBACK(audiodialogok_lcb));
}

static void
canvasdialogok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	gchar *finalstring, *thestring;

	/* Tag construction */
	finalstring = g_strdup(cap("<CANVAS"));
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->spin[1])), cap("WIDTH"), finalstring, NULL);
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->spin[0])), cap("HEIGHT"), finalstring, NULL);
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->entry[0])), cap("ID"), finalstring, NULL);
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->entry[1])), cap("STYLE"), finalstring, NULL);
	finalstring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[0]), cap("CLASS"), finalstring, NULL);	
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->entry[2])), NULL, finalstring, NULL);
	thestring = finalstring;
	finalstring = g_strconcat(thestring, NULL);
	g_free(thestring);
	/* Insert tag in document */
	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</CANVAS>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void
canvas_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	/* Controls */
	GtkWidget *dgtable, *but;
	Thtml_diag *dg;
	static gchar *tagitems[] = { "height", "width", "id", "style", "class", NULL };
	gchar *tagvalues[6];
	gchar *custom=NULL;
		
	/* Dialog construction */
	dg = html_diag_new(bfwin, _("Canvas"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);
	dgtable = html_diag_table_in_vbox(dg, 4,3);

	dg->entry[0] = dialog_entry_in_table(tagvalues[2], dgtable, 1,2, 0,1);
	dialog_mnemonic_label_in_table(_("_Id:"), dg->entry[0], dgtable, 0,1, 0,1);

	dg->combo[0] = html_diag_combobox_with_popdown(tagvalues[4], bfwin->session->classlist, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[0], 1,2, 1,2);
	dialog_mnemonic_label_in_table(_("Cl_ass:"), dg->combo[0], dgtable, 0,1, 1,2);

	dg->entry[1] = dialog_entry_in_table(tagvalues[3], dgtable, 1,3 , 2,3);
	dialog_mnemonic_label_in_table(_("_Style:"), dg->entry[1], dgtable, 0,1, 2,3);
	but = style_but_new(dg->entry[1], dg->dialog);
	gtk_table_attach(GTK_TABLE(dgtable), but, 3,4, 2,3, GTK_SHRINK,GTK_SHRINK,0,0);

	/* Width */
	dg->spin[1] = spinbut_with_value(tagvalues[1]?tagvalues[1]:"200", 0, 10000, 1.0, 5.0);
	dialog_mnemonic_label_in_table(_("_Width:"), dg->spin[1], dgtable, 2,3, 0,1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 3,4, 0,1);

	/* Height */
	dg->spin[0] = spinbut_with_value(tagvalues[0]?tagvalues[0]:"200", 0, 10000, 1.0, 5.0);
	dialog_mnemonic_label_in_table(_("_Height:"), dg->spin[0], dgtable, 2,3, 1,2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[0], 3,4, 1,2);

	dg->entry[2] = dialog_entry_in_table(custom, dgtable, 1,4, 3,4);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dg->entry[2], dgtable, 0,1, 3,4);

	html_diag_finish(dg, G_CALLBACK(canvasdialogok_lcb));
}

static void
videodialogok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	gchar *finalstring, *thestring;

	/* Tag construction */
	finalstring = g_strdup(cap("<VIDEO"));
	finalstring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[1]), cap("SRC"), finalstring, NULL);
	finalstring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[2]), cap("POSTER"), finalstring, NULL);
	finalstring = insert_attr_if_checkbox(dg->check[1], main_v->props.xhtml == 1 ? cap("AUTOPLAY=\"autoplay\"") : cap("AUTOPLAY"), finalstring);
	finalstring = insert_attr_if_checkbox(dg->check[2], main_v->props.xhtml == 1 ? cap("CONTROLS=\"controls\"") : cap("CONTROLS"), finalstring);
	finalstring = insert_attr_if_checkbox(dg->check[3], main_v->props.xhtml == 1 ? cap("LOOP=\"loop\"") : cap("LOOP"), finalstring);
	finalstring = insert_attr_if_checkbox(dg->check[4], main_v->props.xhtml == 1 ? cap("MUTE=\"mute\"") : cap("MUTE"), finalstring);	
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->spin[1])), cap("WIDTH"), finalstring, NULL);
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->spin[2])), cap("HEIGHT"), finalstring, NULL);
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->entry[0])), cap("ID"), finalstring, NULL);
	finalstring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[3]), cap("CLASS"), finalstring, NULL);
	finalstring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[4]), cap("PRELOAD"), finalstring, NULL);
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->entry[1])), cap("STYLE"), finalstring, NULL);
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->entry[2])), NULL, finalstring, NULL);
	thestring = finalstring;
	finalstring = g_strconcat(thestring, ">", NULL);
	g_free(thestring);
	/* Insert tag in document */
	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</VIDEO>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}

	/* Free */
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void
video_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	/* Controls */
	GtkWidget *dgtable, *but;
	Thtml_diag *dg;

	/* Attributes name list */
	static gchar *tagitems[] = { "src", "poster", "width", "height", "id", "class", "preload", "style", NULL };
	gchar *tagvalues[9];
	gchar *custom=NULL;
		
	/* Values attributes lists */
	GList *tmplist;

	dg = html_diag_new(bfwin, _("Video"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);
	dgtable = html_diag_table_in_vbox(dg, 7,9);

	/* Source */
	dg->combo[1] = html_diag_combobox_with_popdown(tagvalues[0], bfwin->session->urllist, 1);
	but = file_but_new(GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[1]))), 0, bfwin);
	gtk_table_attach(GTK_TABLE(dgtable), GTK_WIDGET(but), 8,9, 0,1, GTK_SHRINK,GTK_SHRINK,0,0);
	dialog_mnemonic_label_in_table(_("_Source:"), dg->combo[1], dgtable, 0,1, 0,1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[1])), 1,8, 0,1);

	/* Poster */
	dg->combo[2] = html_diag_combobox_with_popdown(tagvalues[1], bfwin->session->urllist, 1);
	but = file_but_new(GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[2]))), 0, bfwin);
	gtk_table_attach(GTK_TABLE(dgtable), GTK_WIDGET(but), 8,9, 1,2, GTK_SHRINK,GTK_SHRINK,0,0);
	dialog_mnemonic_label_in_table(_("_Poster:"), dg->combo[2], dgtable, 0,1, 1,2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[2])), 1,8, 1,2);

	/* autoplay */
	dg->check[1] = gtk_check_button_new();
	dialog_mnemonic_label_in_table(_("_Autoplay:"), dg->check[1], dgtable, 0,1, 2,3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 1,2, 2,3);	

	/* controls */
	dg->check[2] = gtk_check_button_new();
	dialog_mnemonic_label_in_table(_("C_ontrols:"), dg->check[2], dgtable, 2,3, 2,3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[2], 3,4, 2,3);	

	/* loop */
	dg->check[3] = gtk_check_button_new();
	dialog_mnemonic_label_in_table(_("_Loop:"), dg->check[3], dgtable, 4,5, 2,3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[3], 5,6, 2,3);

	/* mute */
	dg->check[4] = gtk_check_button_new();
	dialog_mnemonic_label_in_table(_("M_ute:"), dg->check[4], dgtable, 7,8, 2,3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[4], 8,9, 2,3);	

	/* Width */
	dg->spin[1] = spinbut_with_value(tagvalues[2]?tagvalues[2]:"200", 0, 10000, 1.0, 5.0);
	dialog_mnemonic_label_in_table(_("_Width:"), dg->spin[1], dgtable, 4,5, 3,4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 5,6, 3,4);

	/* Height */
	dg->spin[2] = spinbut_with_value(tagvalues[3]?tagvalues[3]:"200", 0, 10000, 1.0, 5.0);
	dialog_mnemonic_label_in_table(_("_Height:"), dg->spin[2], dgtable, 7,8, 3,4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[2], 8,9, 3,4);


	dg->entry[0] = dialog_entry_in_table(tagvalues[4], dgtable, 1,4, 4,5);
	dialog_mnemonic_label_in_table(_("_Id:"), dg->entry[0], dgtable, 0,1, 4,5);

	dg->combo[3] = html_diag_combobox_with_popdown(tagvalues[5], bfwin->session->classlist, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[3], 5,9, 4,5);
	dialog_mnemonic_label_in_table(_("Cl_ass:"), dg->combo[3], dgtable, 4,5, 4,5);

	/* Preload */
	tmplist = list_from_arglist(FALSE, "", "auto", "metadata", "none", NULL);
	dg->combo[4] = html_diag_combobox_with_popdown(tagvalues[6], tmplist, 1);
	dialog_mnemonic_label_in_table(_("_Preload:"), dg->combo[4], dgtable, 0,1, 3,4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(dg->combo[4]), 1,4, 3,4);
	g_list_free(tmplist);

	dg->entry[1] = dialog_entry_in_table(tagvalues[7], dgtable, 1,7, 5,6);
	dialog_mnemonic_label_in_table(_("_Style:"), dg->entry[1], dgtable, 0,1, 5,6);
	but = style_but_new(dg->entry[1], dg->dialog);
	gtk_table_attach(GTK_TABLE(dgtable), but, 7,9, 5,6, GTK_SHRINK,GTK_SHRINK,0,0);

	dg->entry[2] = dialog_entry_in_table(custom, dgtable, 1,9, 6,7);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dg->entry[2], dgtable, 0,1, 6,7);

	html_diag_finish(dg, G_CALLBACK(videodialogok_lcb));
}

static void
html5timedialogok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	gchar *finalstring, *thestring;

	/* Tag construction */
	finalstring = g_strdup(cap("<TIME"));
/*	finalstring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[0]), cap("PUBDATE"), finalstring, NULL);*/
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->entry[0])), cap("DATETIME"), finalstring, NULL);
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->entry[1])), NULL, finalstring, NULL);

	thestring = finalstring;
	finalstring = g_strconcat(thestring, ">", NULL);
	g_free(thestring);
	/* Insert tag in document */
	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</TIME>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}

	/* Free */
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);
}

void
html5time_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	/* Controls */
	GtkWidget *dgtable;
	Thtml_diag *dg;
	GtkWidget* l;

	/* there is no more pubdate attribute */
	static gchar *tagitems[] = { "datetime", "pubdate", NULL };
	gchar *tagvalues[3];
	gchar *custom=NULL;

	/* Dialog construction */
	dg = html_diag_new(bfwin, _("Time"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);
	dgtable = html_diag_table_in_vbox(dg, 4,3);

	/* Datetime help */
	l = gtk_label_new(_("Datetime format (ex. YYYY-MM-DDThh:mm:ssTZD) :\n\n"
		"YYYY - year (e.g. 2009)\n"
		"MM - month (e.g. 01 for January)\nDD - day of the month (e.g. 08)\n"
		"T - a required separator\nhh - hour (e.g. 22 for 10.00pm)\n"
		"mm - minutes (e.g. 55)\nss - seconds (e.g. 03)\n"
		"TZD - Time Zone Designator (Z denotes Zulu, also known as Greenwich Mean Time)\n"));
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(l), 0,3, 0,1);

	/* Datetime */
	dg->entry[0] = dialog_entry_in_table(tagvalues[0], dgtable, 1,3, 1,2);
	dialog_mnemonic_label_in_table(_("_Datetime:"), dg->entry[0], dgtable, 0,1, 1,2);

	/* Pubdate is removed of html5*/

	dg->entry[1] = dialog_entry_in_table(custom, dgtable, 1,3, 3,4);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dg->entry[1], dgtable, 0,1, 3,4);

	html_diag_finish(dg, G_CALLBACK(html5timedialogok_lcb));
}

/* this is no HTML, but often requested, so we leave it for now */
/* this is HTML5, modified for HTML5, src,type(Valid MIME type),width,height,&globals; */

static void
embedok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	gchar *thestring, *finalstring;

	finalstring = g_strdup(cap("<EMBED"));
	finalstring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[0]), cap("SRC"), finalstring, NULL);
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->entry[0])), cap("TYPE"), finalstring, NULL);
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->spin[1])), cap("WIDTH"), finalstring, NULL);
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->spin[2])), cap("HEIGHT"), finalstring, NULL);
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->entry[1])), cap("ID"), finalstring, NULL);
	finalstring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[1]), cap("CLASS"), finalstring, NULL);	
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->entry[2])), cap("STYLE"), finalstring, NULL);
	finalstring = insert_string_if_entry((GTK_ENTRY(dg->entry[3])), NULL, finalstring, NULL);
	thestring = finalstring;
	finalstring = g_strconcat(thestring, ">", NULL);
	g_free(thestring);
	/* Insert tag in document */
	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, finalstring, cap("</EMBED>"));
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}

	/* Free */
	g_free(finalstring);
	html_diag_destroy_cb(NULL, dg);

}

void
embed_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
		/* Controls */
	GtkWidget *dgtable, *but;
	Thtml_diag *dg;

	/* Attributes name list */
	static gchar *tagitems[] = { "src", "type", "width", "height", "id", "class", "style", NULL };
	gchar *tagvalues[8];
	gchar *custom=NULL;

	dg = html_diag_new(bfwin, _("Embed"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);
	dgtable = html_diag_table_in_vbox(dg, 5,5);

	dg->combo[0] = html_diag_combobox_with_popdown(tagvalues[0], bfwin->session->urllist, 1);
	but = file_but_new(GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[0]))), 0, bfwin);
	gtk_table_attach(GTK_TABLE(dgtable), GTK_WIDGET(but), 5,6, 0,1, GTK_SHRINK,GTK_SHRINK,0,0);
	dialog_mnemonic_label_in_table(_("_Source:"), dg->combo[0], dgtable, 0,1, 0,1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[0])), 1,5, 0,1);

	dg->entry[0] = dialog_entry_in_table(tagvalues[1], dgtable, 1,5, 1,2);
	dialog_mnemonic_label_in_table(_("_MIME Type:"), dg->entry[0], dgtable, 0,1, 1,2);

	dg->spin[1] = spinbut_with_value(tagvalues[2]?tagvalues[2]:"200", 0, 10000, 1.0, 5.0);
	dialog_mnemonic_label_in_table(_("_Width:"), dg->spin[1], dgtable, 0,1, 2,3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1,2, 2,3);

	dg->spin[2] = spinbut_with_value(tagvalues[3]?tagvalues[3]:"200", 0, 10000, 1.0, 5.0);
	dialog_mnemonic_label_in_table(_("_Height:"), dg->spin[2], dgtable, 3,4, 2,3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[2], 4,6, 2,3);

	dg->entry[1] = dialog_entry_in_table(tagvalues[4], dgtable, 1,2, 3,4);
	dialog_mnemonic_label_in_table(_("_Id:"), dg->entry[1], dgtable, 0,1, 3,4);

	dg->combo[1] = html_diag_combobox_with_popdown(tagvalues[5], bfwin->session->classlist, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[1], 4,6, 3,4);
	dialog_mnemonic_label_in_table(_("Cl_ass:"), dg->combo[1], dgtable, 3,4, 3,4);

	dg->entry[2] = dialog_entry_in_table(tagvalues[6], dgtable, 1,4, 4,5);
	dialog_mnemonic_label_in_table(_("_Style:"), dg->entry[2], dgtable, 0,1, 4,5);
	but = style_but_new(dg->entry[2], dg->dialog);
	gtk_table_attach(GTK_TABLE(dgtable), but, 4,6, 4,5, GTK_SHRINK,GTK_SHRINK,0,0);

	dg->entry[3] = dialog_entry_in_table(custom, dgtable, 1,6, 5,6);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dg->entry[3], dgtable, 0,1, 5,6);

	html_diag_finish(dg, G_CALLBACK(embedok_lcb));
}

static void
scriptok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	gchar *thestring, *finalstring, *endstring;

	thestring = g_strdup(cap("<SCRIPT"));
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[0]), cap("SRC"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[1]), cap("LANGUAGE"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[2]), cap("TYPE"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->combo[3]), cap("CHARSET"), thestring, NULL);
	thestring = insert_attr_if_checkbox(dg->check[0], main_v->props.xhtml == 1 ? cap("ASYNC=\"async\"") : cap("ASYNC"), thestring);
	thestring = insert_attr_if_checkbox(dg->check[1], main_v->props.xhtml == 1 ? cap("DEFER=\"defer\"") : cap("DEFER"), thestring);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->entry[1]), NULL, thestring, NULL);
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

void
script_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	GtkWidget *but;
	GList *tmplist, *tmplist2, *tmplist3;
	GtkWidget *dgtable;
	Thtml_diag *dg;
	static gchar *scriptitems[] = { "src", "language", "type", "charset", NULL };
	gchar *tagvalues[5];
	gchar *custom = NULL;

	dg = html_diag_new(bfwin, _("Script"));
	fill_dialogvalues(scriptitems, tagvalues, &custom, (Ttagpopup *) data, dg);

	dgtable = html_diag_table_in_vbox(dg, 6, 12);

	tmplist = g_list_append(NULL, "javascript");
	tmplist = g_list_append(tmplist, "javascript1.2");
	tmplist = g_list_append(tmplist, "javascript1.1");
	tmplist2 = g_list_append(NULL, "application/javascript");
	tmplist2 = g_list_append(tmplist2, "text/javascript");
	tmplist2 = g_list_append(tmplist2, "application/x-javascript");
	tmplist2 = g_list_append(tmplist2, "text/plain");
	tmplist2 = g_list_append(tmplist2, "text/html");
	tmplist3 = list_from_arglist(FALSE, "UTF-8", "ISO-8859-1", "ISO-8859-15", "ARMSCII-8", "BIG5", "BIG5-HKSCS", "CP866", "EUC-JP", "EUC-KR", "EUC-TW", "GB18030", "GB2312", "GBK", "GEORGIAN-ACADEMY", "HZ", "IBM850", "IBM852", "IBM855", "IBM857", "IBM862", "IBM864", "ISO-2022-JP", "ISO-2022-KR", "ISO-8859-10", "ISO-8859-13", "ISO-8859-14", "ISO-8859-16", "ISO-8859-2", "ISO-8859-3", "ISO-8859-4", "ISO-8859-5", "ISO-8859-6", "ISO-8859-7", "ISO-8859-8", "ISO-8859-8-I", "ISO-8859-9", "ISO-IR-111", "JOHAB", "KOI8R", "KOI8U", "SHIFT_JIS", "TCVN", "TIS-620", "UHC", "VISCII", "WINDOWS-1250", "WINDOWS-1251", "WINDOWS-1252", "WINDOWS-1253", "WINDOWS-1254", "WINDOWS-1255", "WINDOWS-1256", "WINDOWS-1257", "WINDOWS-1258",NULL);
	
	dg->combo[0] = html_diag_combobox_with_popdown(tagvalues[0], bfwin->session->urllist, 1);
	but = file_but_new(GTK_WIDGET(gtk_bin_get_child(GTK_BIN(dg->combo[0]))), 0, bfwin);
	gtk_table_attach(GTK_TABLE(dgtable), GTK_WIDGET(but), 10,12, 0,1, GTK_SHRINK,GTK_SHRINK,0,0);
	dialog_mnemonic_label_in_table(_("_Source:"), dg->combo[0], dgtable, 0,2, 0,1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[0])), 2,10, 0,1);

	dg->combo[1] = html_diag_combobox_with_popdown(tagvalues[1], tmplist, 1);
	dialog_mnemonic_label_in_table(_("_Language:"), dg->combo[1], dgtable, 0,2, 1,2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[1], 2,6, 1,2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(gtk_label_new(_("(HTML4 only)"))), 7,8, 1,2);

	dg->combo[2] = html_diag_combobox_with_popdown(tagvalues[2], tmplist2,1);
	dialog_mnemonic_label_in_table(_("MIME _Type:"), dg->combo[2], dgtable, 0,2, 2,3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[2], 2,6, 2,3);
	
	dg->combo[3] = html_diag_combobox_with_popdown(tagvalues[3], tmplist3,1);
	dialog_mnemonic_label_in_table(_("_Charset:"), dg->combo[3], dgtable, 0,2, 3,4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[3], 2,6, 3,4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(gtk_label_new(_("(HTML5)"))), 7,8, 3,4);	

	dg->check[0] = gtk_check_button_new();
	dialog_mnemonic_label_in_table(_("_Async:"), dg->check[0], dgtable, 0,1, 4,5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[0], 1,2, 4,5);

	dg->check[1] = gtk_check_button_new();
	dialog_mnemonic_label_in_table(_("_Defer:"), dg->check[1], dgtable, 4,5, 4,5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[1], 5,6, 4,5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(gtk_label_new(_("(HTML5)"))), 7,8, 4,5);

	dg->entry[1] = dialog_entry_in_table(custom, dgtable, 2,12, 5,6);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dg->entry[1], dgtable, 0,2, 5,6);

	html_diag_finish(dg, G_CALLBACK(scriptok_lcb));
	g_list_free(tmplist);
	g_list_free(tmplist2);
	g_list_free(tmplist3);
}

static void
columncount_auto_toggled_lcb(GtkWidget * checkbutton, Thtml_diag * dg)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton))){
		gtk_widget_set_sensitive(dg->spin[0], FALSE);
	} else {
		gtk_widget_set_sensitive(dg->spin[0], TRUE);
	}
}

static void
columnwidth_auto_toggled_lcb(GtkWidget * checkbutton, Thtml_diag * dg)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton))){
		gtk_widget_set_sensitive(dg->spin[1], FALSE);
		gtk_widget_set_sensitive(dg->combo[0], FALSE);
	} else {
		gtk_widget_set_sensitive(dg->spin[1], TRUE);
		gtk_widget_set_sensitive(dg->combo[0], TRUE);
	}
}

static void
columngap_normal_clicked_lcb(GtkWidget * checkbutton, Thtml_diag * dg)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton))){
		gtk_widget_set_sensitive(dg->spin[2], FALSE);
		gtk_widget_set_sensitive(dg->combo[1], FALSE);
	} else {
		gtk_widget_set_sensitive(dg->spin[2], TRUE);
		gtk_widget_set_sensitive(dg->combo[1], TRUE);
	}
}

static void
columnrulewidth_changed_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	if (strlen(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dg->combo[5])))) {
		gtk_widget_set_sensitive(dg->spin[3], FALSE);
		gtk_widget_set_sensitive(dg->combo[4], FALSE);	
	} else {
		gtk_widget_set_sensitive(dg->spin[3], TRUE);
		gtk_widget_set_sensitive(dg->combo[4], TRUE);
	}	
}

static void
columnselector_toggled_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->radio[0]))) {
		gtk_widget_set_sensitive(dg->entry[0], TRUE);
	} else {
		gtk_widget_set_sensitive(dg->entry[0], FALSE);
	}	
}

static void
columnsok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	gchar *thestring, *finalstring, *endstring, *tmpstring;
	gboolean moz, webkit;
	moz = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[3]));
	webkit = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[4]));
	
	thestring = g_strdup("");
	endstring = g_strdup("; ");
	finalstring = g_strdup("");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->radio[0]))) {
		if(gtk_entry_get_text_length(GTK_ENTRY(dg->entry[0]))==0) {
			finalstring = g_strdup("\n");
			endstring = g_strdup(";\n");	
		} else {
			thestring = g_strconcat(thestring, gtk_entry_get_text(GTK_ENTRY(dg->entry[0])), " {\n",  NULL);
			finalstring = g_strdup("\n}\n");
			endstring = g_strdup(";\n");
		}
	} 
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->radio[1]))) {
		thestring = g_strdup(" style=\"");
		finalstring = g_strdup("\"");
	}
	
	tmpstring = g_strdup("columns: ");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[2]))) {
	   tmpstring =  g_strconcat(tmpstring, "auto", NULL); 
	} else {
		tmpstring = g_strconcat(tmpstring, gtk_entry_get_text(GTK_ENTRY(GTK_SPIN_BUTTON(dg->spin[0]))), NULL);
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[3]))) {
	   tmpstring =  g_strconcat(tmpstring, " auto", endstring, NULL); 
	} else {
		tmpstring = g_strconcat(tmpstring, " ", gtk_entry_get_text(GTK_ENTRY(GTK_SPIN_BUTTON(dg->spin[1]))), 
		gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dg->combo[0])), endstring, NULL);
	}
	thestring = g_strconcat(thestring, tmpstring, NULL);
	if (moz) {thestring = g_strconcat(thestring, "-moz-", tmpstring, NULL);}
	if (webkit) {thestring = g_strconcat(thestring, "-webkit-", tmpstring, NULL);}
	g_free(tmpstring);

	tmpstring =  g_strdup("column-gap: ");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dg->check[4]))) {
	   tmpstring =  g_strconcat (tmpstring, "normal", endstring, NULL); 
	} else {
		tmpstring = g_strconcat(tmpstring, gtk_entry_get_text(GTK_ENTRY(GTK_SPIN_BUTTON(dg->spin[2]))), 
		gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dg->combo[1])), endstring, NULL);
	}
	thestring = g_strconcat(thestring, tmpstring, NULL);
	if (moz) {thestring = g_strconcat(thestring, "-moz-", tmpstring, NULL);}
	if (webkit) {thestring = g_strconcat(thestring, "-webkit-", tmpstring, NULL);}
	g_free(tmpstring);

	tmpstring =  g_strdup("column-rule: ");
	tmpstring = g_strconcat(tmpstring, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dg->combo[2])), " ", NULL);
	tmpstring = g_strconcat(tmpstring, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dg->combo[3])), " ", NULL);
	if (strlen(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dg->combo[5])))) {
		tmpstring = g_strconcat(tmpstring, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dg->combo[5])), endstring, NULL); 
	} else {
		tmpstring = g_strconcat(tmpstring, gtk_entry_get_text(GTK_ENTRY(GTK_SPIN_BUTTON(dg->spin[3]))), 
		gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dg->combo[4])), endstring, NULL);
	}
	thestring = g_strconcat(thestring, tmpstring, NULL);
	if (moz) {thestring = g_strconcat(thestring, "-moz-", tmpstring, NULL);}
	if (webkit) {thestring = g_strconcat(thestring, "-webkit-", tmpstring, NULL);}
	g_free(tmpstring);

	if (dg->range.end == -1) {
		doc_insert_two_strings(dg->doc, thestring, finalstring );
	} else {
		doc_replace_text(dg->doc, finalstring, dg->range.pos, dg->range.end);
	}
	g_free(thestring);
	g_free(finalstring);
	g_free(endstring);
	html_diag_destroy_cb(NULL, dg);
}

void
columns_dialog(Tbfwin * bfwin, Ttagpopup * data)
{
	GtkWidget *color_but;
	GList *tmplist = 	list_from_arglist(FALSE,"em", "px", "ex", "ch", "rem", "px", "pc", "pt", 
				"mm", "cm", "in", "%", NULL),
		*tmplist2 = list_from_arglist(FALSE, "", "none", "hidden", "dotted", "dashed", "solid", 
				"double", "groove", "ridge", "inset", "outset", NULL), 
		*tmplist3 = list_from_arglist(FALSE, "", "thin", "medium", "thick", NULL) ;
	GtkWidget *dgtable;
	Thtml_diag *dg;
	static gchar *tagitems[] = { "column-count", "column-width", "column-width-unit", "column-gap", 
				"column-gap-unit", "column-rule-color", "column-rule-style", "column-rule-width", 
				"column-rule-width-unit", "column-rule-width-var", "selectors", NULL };
	gchar *tagvalues[12];
	gchar *custom = NULL;

/*
todo check buttons: 
column-span: all (default none) 
column-fill: auto (default balance)	 
*/	

	dg = html_diag_new(bfwin, _("CSS3 multi-column layout"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);
	dgtable = html_diag_table_in_vbox(dg, 9, 4);

	dg->spin[0] = spinbut_with_value(tagvalues[0] ? tagvalues[0]: "2", 1, 100, 1, 5);
	gtk_widget_set_tooltip_text(
			dialog_mnemonic_label_in_table("column-c_ount:", dg->spin[0], dgtable, 0,1, 0,1),
			_("Auto (default) or number of columns"));
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[0], 1,2, 0,1);
	dg->check[2] = gtk_check_button_new_with_mnemonic("_auto");
	g_signal_connect(dg->check[2], "toggled", G_CALLBACK(columncount_auto_toggled_lcb), dg);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[2], 3,4, 0,1);

	dg->spin[1] = spinbut_with_value(tagvalues[1] ? tagvalues[1]: "50", 0, 10000, 0.1, 1.0);
	gtk_widget_set_tooltip_text(
			dialog_mnemonic_label_in_table("column-_width:", dg->spin[1], dgtable, 0,1, 1,2),
			_("Auto (default) or width"));
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[1], 1,2, 1,2);
	dg->combo[0] = html_diag_combobox_with_popdown_sized(tagvalues[2] ? tagvalues[2]: "%", tmplist, 0, 70);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[0], 2,3, 1,2);
	dg->check[3] = gtk_check_button_new_with_mnemonic("a_uto");
	g_signal_connect(dg->check[3], "toggled", G_CALLBACK(columnwidth_auto_toggled_lcb), dg);	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dg->check[3]), TRUE);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[3], 3,4, 1,2);

	dg->spin[2] = spinbut_with_value(tagvalues[3] ? tagvalues[3]: "1", 0, 10000, 0.1, 1.0);
	gtk_widget_set_tooltip_text(
			dialog_mnemonic_label_in_table("column-_gap:", dg->spin[2], dgtable, 0,1, 2,3),
			_("Normal (default) or width of the gap between columns"));
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[2], 1,2, 2,3);
	dg->combo[1] = html_diag_combobox_with_popdown_sized(tagvalues[4] ? tagvalues[4]: "em", tmplist, 0, 70);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[1], 2,3, 2,3);
	dg->check[4] = gtk_check_button_new_with_mnemonic("_normal");
	g_signal_connect(dg->check[4], "clicked", G_CALLBACK(columngap_normal_clicked_lcb), dg);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dg->check[4]), TRUE);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[4], 3,4, 2,3);

	dg->combo[2] = html_diag_combobox_with_popdown_sized(tagvalues[5], bfwin->session->colorlist, 1, 110);
	color_but = color_but_new(gtk_bin_get_child(GTK_BIN(dg->combo[2])), dg->dialog);
	dialog_mnemonic_label_in_table("column-rule-co_lor:", dg->combo[2], dgtable, 0,1, 3,4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[2])), 1,2, 3,4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(color_but), 2,3, 3,4);

	dg->combo[3] = html_diag_combobox_with_popdown_sized(tagvalues[6] ? tagvalues[6]: "solid", tmplist2, 0, 110);
	gtk_widget_set_tooltip_text(
			dialog_mnemonic_label_in_table("column-rule-_style:", dg->combo[3], dgtable, 0,1, 4,5),
			_("None (default) or style"));	
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[3])), 1,2, 4,5);

	dg->spin[3] = spinbut_with_value(tagvalues[7] ? tagvalues[7]: "0.5", 0, 10000, 0.1, 1.0);
	gtk_widget_set_tooltip_text(
			dialog_mnemonic_label_in_table("column-rule-wi_dth:", dg->spin[3], dgtable, 0,1, 5,6),
			_("Medium (default) or width"));
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->spin[3], 1,2, 5,6);

	dg->combo[4] = html_diag_combobox_with_popdown_sized(tagvalues[8] ? tagvalues[8]: "em", tmplist, 0, 70);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->combo[4], 2,3, 5,6);

	dg->combo[5] = html_diag_combobox_with_popdown_sized(tagvalues[9] ? tagvalues[9]: "medium", tmplist3, 0, 90);
	g_signal_connect(dg->combo[5], "changed", G_CALLBACK(columnrulewidth_changed_lcb), dg);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(GTK_BIN(dg->combo[5])), 3,4, 5,6);
	gtk_widget_set_sensitive(dg->spin[3], FALSE);
	gtk_widget_set_sensitive(dg->combo[4], FALSE);	

	gtk_table_attach_defaults(GTK_TABLE(dgtable), GTK_WIDGET(gtk_label_new(
			_("Use vendor-prefixed CSS property:"))), 0,2, 6,7);
	dg->check[3] = gtk_check_button_new_with_mnemonic("G_ecko");	
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[3], 2,3, 6,7);	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dg->check[3]), TRUE);
	dg->check[4] = gtk_check_button_new_with_mnemonic("Web_kit");
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->check[4], 3,4, 6,7);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dg->check[4]), TRUE);

	dg->radio[0] = gtk_radio_button_new_with_mnemonic(NULL, _("styles_heet"));
	gtk_widget_set_tooltip_text(dg->radio[0],_("Add selector(s) to create a new rule"));
	dg->radio[1] = gtk_radio_button_new_with_mnemonic(gtk_radio_button_get_group
			(GTK_RADIO_BUTTON(dg->radio[0])), _("style a_ttribute"));
	gtk_widget_set_tooltip_text(dg->radio[1],_("Add a style attribute in tag"));		
	dg->radio[2] = gtk_radio_button_new_with_mnemonic(gtk_radio_button_get_group
			(GTK_RADIO_BUTTON(dg->radio[0])), _("style _values"));
	gtk_widget_set_tooltip_text(dg->radio[2],_("Add values in a style attribute"));		
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dg->radio[0]), TRUE);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->radio[0], 0,1, 7,8);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->radio[1], 1,2, 7,8);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->radio[2], 2,4, 7,8);
	g_signal_connect(GTK_TOGGLE_BUTTON(dg->radio[0]), "toggled", 
				G_CALLBACK(columnselector_toggled_lcb), dg);

	dg->entry[0] = dialog_entry_in_table(tagvalues[10], dgtable, 1,4, 8,9);
	gtk_widget_set_tooltip_text(
			dialog_mnemonic_label_in_table(_("Selecto_rs:"), dg->entry[0], dgtable, 0,1, 8,9),
			_("Leave empty to insert declarations into an existing rule."));
		
	g_list_free(tmplist);
	g_list_free(tmplist2);
	g_list_free(tmplist3);

	html_diag_finish(dg, G_CALLBACK(columnsok_lcb));
}

static void
linkdialogok_lcb(GtkWidget * widget, Thtml_diag * dg)
{
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<LINK"));
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->attrwidget[0]), cap("HREF"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->attrwidget[1]), cap("HREFLANG"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->attrwidget[2]), cap("TITLE"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->attrwidget[3]), cap("TYPE"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->attrwidget[4]), cap("REL"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->attrwidget[5]), cap("REV"), thestring, NULL);
	thestring = insert_string_if_combobox(GTK_COMBO_BOX(dg->attrwidget[6]), cap("MEDIA"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->attrwidget[7]), cap("LANG"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_ENTRY(dg->attrwidget[8]), NULL, thestring, NULL);
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

/*
 * This dialog can be used for almost every <link .../> tag usage. A few parts
 * are missing:
 *
 *   - rel="fontdef" src="..." which is for downloadable font types
 *     should grey out everything else or separate dialog?
 *   - missing: target (bad-style?), and charset attributes are not part of
 *     the dialog
 *
 * The mode-switch can be used to add more default values for links (see
 * html.h -> Tlinkdialog_mode. Maybe we can use this to handle the link-dialog
 * for downloadable font types. Oliver suggested to add some buttons as a way
 * for a fast and direct way to access several standard values (e.g. CSS) from
 * inside the dialog.
 */
void
linkdialog_dialog(Tbfwin * bfwin, Ttagpopup * data, Tlinkdialog_mode mode)
{
	static gchar *tagitems[] = { "href", "hreflang", "title", "type", "rel", "rev", "media", "lang", NULL };
	gchar *tagvalues[9];
	gchar *custom = NULL;
	Thtml_diag *dg;
	GtkWidget *dgtable, *but;

	dg = html_diag_new(bfwin, _("Link"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, dg);
	dgtable = html_diag_table_in_vbox(dg, 3, 10);
	{
		GList *rel_link_list = NULL, *tmplist;
		rel_link_list = list_relative_document_filenames(bfwin->current_document);
		tmplist = duplicate_stringlist(bfwin->session->urllist, 1);
		rel_link_list = g_list_concat(tmplist, rel_link_list);
		dg->attrwidget[0] = html_diag_combobox_with_popdown(tagvalues[0], rel_link_list, 1);
		free_stringlist(rel_link_list);
	}
	dialog_mnemonic_label_in_table(_("_HREF:"), dg->attrwidget[0], dgtable, 0,1, 0,1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->attrwidget[0], 1,2, 0,1);
	but = file_but_new(gtk_bin_get_child(GTK_BIN(dg->attrwidget[0])), 0, bfwin);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), but, 2,3, 0,1);

	dg->attrwidget[1] = dialog_entry_in_table(tagvalues[1], dgtable, 1,3, 1,2);
	dialog_mnemonic_label_in_table(_("HREF_LANG:"), dg->attrwidget[1], dgtable, 0,1, 1,2);

	dg->attrwidget[2] = dialog_entry_in_table(tagvalues[2], dgtable, 1,3, 2,3);
	dialog_mnemonic_label_in_table(_("T_itle:"), dg->attrwidget[2], dgtable, 0,1, 2,3);

	{
		gchar *tmp;
		GList *tmplist = 	list_from_arglist(FALSE, "text/html", "text/css", "text/plain", "text/javascript",
							  "application/postscript", NULL);
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
		dg->attrwidget[3] = html_diag_combobox_with_popdown(tmp, tmplist, 1);

		g_list_free(tmplist);
	}
	dialog_mnemonic_label_in_table(_("_Type:"), dg->attrwidget[3], dgtable, 0,1, 3,4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->attrwidget[3], 1,3, 3,4);

	{
		gchar *tmp;
		GList *tmplist = 	list_from_arglist(FALSE, "stylesheet", "alternate", "alternate stylesheet", "shortcut icon",
							  "contents", "chapter", "section", "subsection", "index", "glossary", "appendix",
							  "search", "author", "copyright", "next", "prev", "first", "last", "up", "top",
							  "help", "bookmark", NULL);

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
		dg->attrwidget[4] = html_diag_combobox_with_popdown(tmp, tmplist, 1);
		dialog_mnemonic_label_in_table(_("_Forward Relation:"), dg->attrwidget[4], dgtable, 0,1, 4,5);
		gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->attrwidget[4], 1,3, 4,5);

		dg->attrwidget[5] = html_diag_combobox_with_popdown(tagvalues[5], tmplist, 1);
		dialog_mnemonic_label_in_table(_("_Reverse Relation:"), dg->attrwidget[5], dgtable, 0,1, 5,6);
		gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->attrwidget[5], 1,3, 5,6);

		g_list_free(tmplist);
	}

	{
		GList *tmplist = 	list_from_arglist(FALSE, "screen", "tty", "tv", "projection", "handheld", "print", "braille",
							  "aural", "all", NULL);
		dg->attrwidget[6] = html_diag_combobox_with_popdown(tagvalues[6], tmplist, 1);
		g_list_free(tmplist);
	}
	dialog_mnemonic_label_in_table(_("Media:"), dg->attrwidget[6], dgtable, 0,1, 6,7);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), dg->attrwidget[6], 1,3, 6,7);

	dg->attrwidget[7] = dialog_entry_in_table(tagvalues[7], dgtable, 1,3, 7,8);
	dialog_mnemonic_label_in_table(_("L_ang:"), dg->attrwidget[7], dgtable, 0,1, 7,8);

	dg->attrwidget[8] = dialog_entry_in_table(custom, dgtable, 1,3, 8,9);
	dialog_mnemonic_label_in_table(_("_Custom:"), dg->attrwidget[8], dgtable, 0,1, 8,9);

	html_diag_finish(dg, G_CALLBACK(linkdialogok_lcb));
	if (custom)
		g_free(custom);
}
