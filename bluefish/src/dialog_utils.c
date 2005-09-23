/* Bluefish HTML Editor
 * dialog_utils.c - dialog utility functions
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* #define DEBUG */

#include "config.h"

#ifdef HAVE_ATLEAST_GTK_2_4
#include <gtk/gtk.h>

#include "dialog_utils.h"

/**
 * dialog_mnemonic_label_in_table:
 * 	@labeltext:		 #const gchar * The label text.
 * 	@m_widget:		 #GtkWidget * GtkWidget that is the mnemonic target.
 * 	@table:			 #GtkWidget * GtkTable the label is attached to.
 *    @left_attach:   #guint The column number to attach the left side of the label to.
 *    @right_attach:	 #guint The column number to attach the right side of the label to.
 *    @top_attach:	 #guint The row number to attach the top of the label to.
 *    @bottom_attach: #guint The row number to attach the bottom of the label to.
 *  
 * 	Creates a GtkLabel, places it in a GtkTable, and sets it mnemonic widget.
 *
 * Return value: void
 */
void
dialog_mnemonic_label_in_table(const gchar *labeltext,
										 GtkWidget *m_widget,
										 GtkWidget *table,
										 guint left_attach, guint right_attach, 
										 guint top_attach, guint bottom_attach)
{
	GtkWidget *label;
	
	label = gtk_label_new_with_mnemonic (labeltext);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, left_attach, right_attach, top_attach, bottom_attach, GTK_FILL, GTK_FILL, 0, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL(label), m_widget);
}

/**
 * dialog_vbox_label_new:
 * 	@labeltext:	#const gchar * The label text.
 * 	@xalign:		#gfloat The horizontal alignment, from 0 (left) to 1 (right).
 *    @yalign:		#gfloat the vertical alignment, from 0 (top) to 1 (bottom).
 * 	@box:			#GtkWidget * GtkBox the label is packed into.
 *  
 * 	Creates a GtkLabel, aligns it, and packs it in a GtkBox.
 *
 * Return value: #GtkWidget * The new GtkLabel widget.
 */
static GtkWidget *
dialog_vbox_label_new(const gchar *labeltext, gfloat xalign, gfloat yalign, GtkWidget *box)
{
	GtkWidget *label;
	
	label = gtk_label_new (NULL);
	gtk_label_set_markup_with_mnemonic (GTK_LABEL (label), labeltext);
	gtk_misc_set_alignment (GTK_MISC (label), xalign, yalign);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	
	return label;
}

/**
 * dialog_vbox_new:
 * 	@box:	#GtkWidget * GtkBox that a new GtkAlignment is packed into.
 *  
 * 	Creates a GtkVBox packed into a GtkAlignment.
 *
 * Return value: #GtkWidget * The new GtkVBox widget.
 */
static GtkWidget *
dialog_vbox_new(GtkWidget *box)
{
	GtkWidget *alignment, *vbox;
	
	alignment = gtk_alignment_new (0, 0, 1, 1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 6);
	gtk_box_pack_start (GTK_BOX (box), alignment, FALSE, FALSE, 0);	
	vbox = gtk_vbox_new (FALSE, 12);
	gtk_container_add (GTK_CONTAINER (alignment), vbox);
	
	return vbox;
}

/**
 * dialog_vbox_labeled:
 * 	@labeltext:	#const gchar * The label text.
 * 	@box:			#GtkWidget * GtkBox the label is packed into.
 *  
 * 	Creates a GtkVBox and packs a new GtkLabel into it.
 *
 * Return value: #GtkWidget * The new GtkVBox widget.
 */
GtkWidget *
dialog_vbox_labeled(const gchar *labeltext, GtkWidget *box)
{
	GtkWidget *label;

	label = dialog_vbox_label_new(labeltext, 0, 0, box);
		
	return dialog_vbox_new(box);
}

/**
 * dialog_vbox_labeled_checkbutton:
 * 	@labeltext:	  #const gchar * The label text.
 *    @checkbutton: #GtkWidget * GtkWidget that is the mnemonic target.
 * 	@box:			  #GtkWidget * GtkBox the new GtkVBox is packed into.
 *  
 * 	Creates a GtkVBox with a GtkCheckButton as it's label.
 *
 * Return value: #GtkWidget * The new GtkVBox widget.
 */
GtkWidget *
dialog_vbox_labeled_checkbutton(const gchar *labeltext, GtkWidget *checkbutton, GtkWidget *box)
{
	GtkWidget *label, *hbox;

	hbox = gtk_hbox_new (FALSE, 2);
	gtk_box_pack_start (GTK_BOX (hbox), checkbutton, FALSE, FALSE, 0);
	
	label = dialog_vbox_label_new(labeltext, 0, 0.5, hbox);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), checkbutton);
	gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
			
	return dialog_vbox_new(box);
}

/**
 * dialog_table_new:
 *    @rows:		  #gint The number of rows in the new table.
 *    @cols:		  #gint The number of columns in the new table.
 *    @borderWidth: #gint Amount of blank space to leave outside the table.
 *  
 * 	Creates a GtkTable and optionally sets a border width.
 *
 * Return value: #GtkWidget * The new GtkTable widget
 */
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

/**
 * dialog_table_in_vbox:
 *    @rows:		  #gint The number of rows in the new table.
 *    @cols:		  #gint The number of columns in the new table.
 *    @borderWidth: #gint Amount of blank space to leave outside the table.
 *    @box:			  #GtkWidget * The GtkVBox to pack the table into.
 *    @expand:		  #gboolean TRUE if the new table is to be given extra space allocated to box.
 *    @fill:		  #gboolean TRUE if space given to table by the expand option is actually allocated to table.
 *    @padding:	  #guint Extra space in pixels to put between the table and its neighbors.
 *  
 * 	Creates a GtkTable and packs it into a GtkVBox using the supplied values.
 *
 * Return value: #GtkWidget * The new GtkTable widget
 */
GtkWidget *
dialog_table_in_vbox(gint rows, gint cols, 
							gint borderWidth, 
							GtkWidget *box, 
							gboolean expand, gboolean fill, guint padding)
{
	GtkWidget *table = dialog_table_new (rows, cols, borderWidth);
		
	gtk_box_pack_start (GTK_BOX (box), table, expand, fill, padding);
	
	return table;
}

/**
 * dialog_table_in_vbox_defaults:
 *    @rows:		  #gint The number of rows in the new table.
 *    @cols:		  #gint The number of columns in the new table.
 *    @borderWidth: #gint Amount of blank space to leave outside the table.
 *    @box:			  #GtkWidget * The GtkVBox to pack the table into.
 *  
 * 	Creates a GtkTable and packs it into a GtkVBox using the default packing values for a GtkBox.
 *
 * Return value: #GtkWidget * The new GtkTable widget
 */
GtkWidget *
dialog_table_in_vbox_defaults(gint rows, gint cols, gint borderWidth, GtkWidget *box)
{
	GtkWidget *table = dialog_table_new (rows, cols, borderWidth);
		
	gtk_box_pack_start_defaults (GTK_BOX (box), table);
	
	return table;
}

/*****************************************************************************/
/* Message Dialog Functions																  */
/*****************************************************************************/

/**
 * message_dialog_set_text:
 * 	@dialog:			 #GtkWidget * The message dialog.
 * 	@primaryText:	 #const gchar * The primary text displayed in the message dialog.
 * 	@secondaryText: #const gchar * The secondary text displayed in the message dialog.
 *  
 * 	Sets the primary and optional secondary text in a message dialog.
 *
 * Return value: void
 */
static void
message_dialog_set_text(GtkWidget *dialog, const gchar *primaryText, const gchar *secondaryText)
{
	gchar *text;
	
	if (!secondaryText) {
		text = g_strconcat ("<span weight=\"bold\" size=\"larger\">", primaryText, "</span>\n", NULL);
	} else {
		text = g_strconcat ("<span weight=\"bold\" size=\"larger\">", primaryText, "</span>\n\n", secondaryText, "\n", NULL);
	}
	
	gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dialog), text);
	g_free (text);
	
	gtk_window_set_title (GTK_WINDOW (dialog), "");
}

/**
 * message_dialog_new:
 * 	@parent:			 #GtkWidget * Transient parent, or NULL for none
 *    @type:			 #GtkMessageType The type of message being displayed in the dialog.
 * 	@button:			 #GtkButtonsType The type of button displayed in the dialog.
 * 	@primaryText:	 #const gchar * The primary text displayed in the message dialog.
 * 	@secondaryText: #const gchar * The secondary text displayed in the message dialog.
 *  
 * 	Creates a modal GtkMessageDialog with a single button.
 *
 * Return value: void
 */
void
message_dialog_new(GtkWidget *parent, 
						 GtkMessageType type, 
						 GtkButtonsType button, 
						 const gchar *primaryText, const gchar *secondaryText)
{
	GtkWidget *dialog;
		
	dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
												GTK_DIALOG_DESTROY_WITH_PARENT,
												type,
												button,
												NULL);	

	message_dialog_set_text(dialog, primaryText, secondaryText);
	
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

/**
 * message_dialog_new_multi:
 * 	@parent:			 #GtkWidget * Transient parent, or NULL for none
 *    @type:			 #GtkMessageType The type of message being displayed in the dialog.
 * 	@buttons:		 #const gchar * An array of strings to be displayed on the dialog buttons.
 * 	@primaryText:	 #const gchar * The primary text displayed in the message dialog.
 * 	@secondaryText: #const gchar * The secondary text displayed in the message dialog.
 *  
 * 	Creates a non-modal GtkMessageDialog with multiple buttons.
 *
 * Return value: #gint The response ID
 */
gint
message_dialog_new_multi(GtkWidget *parent, 
								 GtkMessageType type, 
								 const gchar *buttons[], 
								 const gchar *primaryText, const gchar *secondaryText)
{
	GtkWidget *dialog;
	gint response;
	int i = 0;
		
	dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
												GTK_DIALOG_DESTROY_WITH_PARENT,
												type,
												GTK_BUTTONS_NONE,
												NULL);

	message_dialog_set_text(dialog, primaryText, secondaryText);																

	for (i = 0; *buttons; i++) {
		gtk_dialog_add_button (GTK_DIALOG (dialog), *buttons++, i);
	}
	/* the Gnome HIG specficies that the default response should always be the far right button) */
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), i-1);

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	
	return response;
}

#endif /* HAVE_ATLEAST_GTK_2_4 */
