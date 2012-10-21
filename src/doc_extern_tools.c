/* Bluefish HTML Editor
 * doc_text_tools.c - text tools
 *
 * Copyright (C) 2012 Olivier Sessink
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

#include "bluefish.h"
#include "gtk_easy.h"
#include "dialog_utils.h"
#include "document.h"
#include "stringlist.h"
#include "external_commands.h"

static void
lorem_ipsum_command_callback(const gchar *output, gpointer bfwin)
{
	if (output)
		doc_insert_two_strings(BFWIN(bfwin)->current_document, output, NULL);
}

void
lorem_ipsum_dialog(Tbfwin *bfwin)
{
	gint result, i;
	GtkWidget *dialog, *spin1, *spin2, *combo, *table;
	GList *poplist;
	gchar *command;
	dialog = gtk_dialog_new_with_buttons(_("Lorem Ipsum generator"),
											  GTK_WINDOW(bfwin->main_window),
											  GTK_DIALOG_DESTROY_WITH_PARENT,
											  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
											  GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	table = dialog_table_in_vbox_defaults(3, 2, 6, gtk_dialog_get_content_area(GTK_DIALOG(dialog)));
	spin1 = dialog_spin_button_in_table(0, 5000, 1, table,1,2, 0,1);
	dialog_mnemonic_label_in_table(_("Number of paragraphs"), spin1, table,0,1, 0,1);
	spin2 = dialog_spin_button_in_table(0, 5000, 0, table,1,2, 1,2);
	dialog_mnemonic_label_in_table(_("Number of sentences"), spin2, table,0,1, 1,2);
	poplist = list_from_arglist(FALSE, _("Insert plain text"), _("Insert in <li>"), _("Insert in <p>"), NULL);
	combo = combobox_with_popdown(NULL, poplist, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), combo, 0,2, 2,3);
	g_list_free(poplist);
	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG (dialog));
	switch (result) {
	case GTK_RESPONSE_ACCEPT:
		i = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
#ifdef WIN32
		command = g_strdup_printf("python "PKGDATADIR"/lorem-ipsum-generator -l -p %d -s %d %s|", 
								(gint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin1)),
								(gint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin2)),
								 i == 2 ? "" : (i==1 ? "-f html-li" : "-f html-p")
								 );
#else
		command = g_strdup_printf(PKGDATADIR"/lorem-ipsum-generator -l -p %d -s %d %s|", 
								(gint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin1)),
								(gint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin2)),
								 i == 2 ? "" : (i==1 ? "-f html-li" : "-f html-p")
								 );
#endif
		custom_command(bfwin, command, (CustomCommandCallback) lorem_ipsum_command_callback, NULL);
		g_free(command);
	break;
	default:
	break;
	}
	gtk_widget_destroy(dialog);
}


void
jsbeautify_dialog(Tbfwin *bfwin)
{
	GtkWidget *dialog, *table, *inselection, *usetabs, *tabsize, *nopreservenewline, *jslinthappy, *unescape_encoded_chars;
	gint result,begin,end;
	gchar *command;
	dialog = gtk_dialog_new_with_buttons(_("Javascript Beautify"),
											  GTK_WINDOW(bfwin->main_window),
											  GTK_DIALOG_DESTROY_WITH_PARENT,
											  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
											  GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	table = dialog_table_in_vbox_defaults(3, 2, 6, gtk_dialog_get_content_area(GTK_DIALOG(dialog)));
	if (doc_get_selection(DOCUMENT(bfwin->current_document), &begin, &end)) {
		inselection = dialog_check_button_in_table(_("Beautify only in selection"), TRUE, table,
										0, 2, 0,1);
	} else {
		begin = 0;
		end = -1;
	}
	usetabs = dialog_check_button_in_table(_("Use tabs to indent, not spaces"), TRUE, table,
										0, 2, 1,2);
	gtk_table_attach_defaults(GTK_TABLE(table), gtk_label_new(_("Indent size")),0,1,2,3);
	tabsize = dialog_spin_button_in_table(1, 99, 3, table,1, 2, 2,3);
	
	nopreservenewline = dialog_check_button_in_table(_("Do not preserve existing line breaks"), FALSE, table,0, 2, 3,4);
	jslinthappy = dialog_check_button_in_table(_("More jslint-compatible output"), FALSE, table,0, 2, 4,5);
	unescape_encoded_chars = dialog_check_button_in_table(_("Decode printable chars encoded in \\\\xNN notation"), FALSE, table,0, 2, 5,6);
	/* in selection or all text
-s, --indent-size=NUMBER indentation size. (default 4).
-c, --indent-char=CHAR character to indent with. (default space).
-t, --indent-with-tabs Indent with tabs, overrides -s and -c
-d, --disable-preserve-newlines do not preserve existing line breaks.
-j, --jslint-happy more jslint-compatible output
-b, --brace-style=collapse brace style (collapse, expand, end-expand)
-k, --keep-array-indentation keep array indentation.
-o, --outfile=FILE specify a file to output to (default stdout)
-f, --keep-function-indentation Do not re-indent function bodies defined in var lines.
-x, --unescape-strings Decode printable chars encoded in \\xNN notation. */
	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG (dialog));
	switch (result) {
	case GTK_RESPONSE_ACCEPT:
#ifdef WIN32
		command = g_strdup_printf("|python "PKGDATADIR"/jsbeautify %s -s %d %s %s %s -|",
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(usetabs)) ? "-t" : "",
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(tabsize)),
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(nopreservenewline)) ? "-d" : "",
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(jslinthappy)) ? "-j" : "",
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(unescape_encoded_chars)) ? "-x" : ""
		);
#else
		command = g_strdup_printf("|"PKGDATADIR"/jsbeautify %s -s %d %s %s %s -|",
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(usetabs)) ? "-t" : "",
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(tabsize)),
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(nopreservenewline)) ? "-d" : "",
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(jslinthappy)) ? "-j" : "",
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(unescape_encoded_chars)) ? "-x" : ""
		);
#endif
		filter_command(bfwin, command, begin, end);
		g_free(command);
	break;
	default:
	break;
	}
	gtk_widget_destroy(dialog);
}

#ifdef WIN32
gboolean
check_python()
{
	HKEY hPython;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Python\\PythonCore\\2.7\\InstallPath", 0, KEY_QUERY_VALUE, &hPython) == ERROR_SUCCESS) {
		RegCloseKey(hPython);
		return TRUE;
	}
	else if (RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Python\\PythonCore\\2.7\\InstallPath", 0, KEY_QUERY_VALUE, &hPython) == ERROR_SUCCESS) {
		RegCloseKey(hPython);
		return TRUE;
	}
	else if (g_file_test("python.exe", G_FILE_TEST_EXISTS)) {
		// For a possible bundled copy of Python for portable installations
		return TRUE;
	}
	else {
		return FALSE;
	}
}
#endif
