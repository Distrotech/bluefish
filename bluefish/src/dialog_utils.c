/* Bluefish HTML Editor
 * dialog_utils.c --> dialog utility functions
 *
 * Copyright (C) 2005 James Hayward and Olivier Sessink
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
 
#define DEBUG

#include <gtk/gtk.h>

#include "bluefish.h"
#include "dialog_utils.h"


GtkWidget *
dialog_label_with_alignment(const gchar *labeltext, GtkWidget *box)
{
	GtkWidget *label, *alignment, *vbox;

	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), labeltext);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);	
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	
	alignment = gtk_alignment_new (0, 0, 1, 1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 6);
	gtk_box_pack_start (GTK_BOX (box), alignment, FALSE, FALSE, 0);	
	vbox = gtk_vbox_new (FALSE, 12);
	gtk_container_add (GTK_CONTAINER (alignment), vbox);
	
	return vbox;
}

static GtkWidget *
dialog_table_new(gint rows, gint cols, gint borderWidth)
{
	GtkWidget *table = gtk_table_new (rows, cols, FALSE);
	
	gtk_table_set_row_spacings (GTK_TABLE (table), 12);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	
	if (borderWidth > 0) {
		gtk_container_set_border_width (GTK_CONTAINER (table), borderWidth);
	}

	return table;
}

GtkWidget *
dialog_table_in_vbox(gint rows, gint cols, gint borderWidth, GtkWidget *box, gboolean expand, gboolean fill, guint padding)
{
	GtkWidget *table = dialog_table_new (rows, cols, borderWidth);
		
	gtk_box_pack_start (GTK_BOX (box), table, expand, fill, padding);
	
	return table;
}

GtkWidget *
dialog_table_in_vbox_defaults(gint rows, gint cols, gint borderWidth, GtkWidget *box)
{
	GtkWidget *table = dialog_table_new (rows, cols, borderWidth);
		
	gtk_box_pack_start_defaults (GTK_BOX (box), table);
	
	return table;
}

static gchar *
message_dialog_set_text(const gchar *primaryText, const gchar *secondaryText)
{
	gchar *text;
	
	if (!secondaryText) {
		text = g_strconcat ("<span weight=\"bold\" size=\"larger\">", primaryText, "</span>\n", NULL);
	} else {
		text = g_strconcat ("<span weight=\"bold\" size=\"larger\">", primaryText, "</span>\n\n", secondaryText, "\n", NULL);
	}
	
	return text;
}

void
message_dialog_new(GtkWidget *parent, GtkMessageType type, GtkButtonsType button, const gchar *primaryText, const gchar *secondaryText)
{
	GtkWidget *dialog;
	gchar *text;
	
	text = message_dialog_set_text(primaryText, secondaryText);
	
	dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (parent),
																GTK_DIALOG_DESTROY_WITH_PARENT,
																type,
																button,
																text);	
	g_free (text);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

gint
message_dialog_new_multi(GtkWidget *parent, GtkMessageType type, const gchar *buttons[], const gchar *primaryText, const gchar *secondaryText)
{
	GtkWidget *dialog;
	gchar *text;
	gint response;
	int i = 0;
	
	text = message_dialog_set_text(primaryText, secondaryText);
	
	dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (parent),
																GTK_DIALOG_DESTROY_WITH_PARENT,
																type,
																GTK_BUTTONS_NONE,
																text);
	g_free (text);

	for (i = 0; *buttons; i++) {
		gtk_dialog_add_button (GTK_DIALOG (dialog), *buttons++, i);
	}
	/* the Gnome HIG specficies that the default response should always be the far right button) */
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), i-1);

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	
	return response;
}
