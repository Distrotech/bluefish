/* Bluefish HTML Editor
 * dialog_utils.c - dialog utility functions
 *
 * Copyright (C) 2005-2012 James Hayward and Olivier Sessink
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

/* #define DEBUG */

#include <gtk/gtk.h>

#include "bluefish.h"
#include "gtk_easy.h"
#include "dialog_utils.h"
#include "pixmap.h"


#if !GTK_CHECK_VERSION(3,0,0)
static void
button_set_style(GtkWidget * button, GtkStyle * previous_style, gpointer user_data)
{
	gint h, w;

	gtk_icon_size_lookup_for_settings(gtk_widget_get_settings(button), GTK_ICON_SIZE_MENU, &w, &h);

	gtk_widget_set_size_request(button, w + 2, h + 2);
}
#endif

GtkWidget *
bluefish_small_close_button_new(void)
{
	/* TODO: Add a tooltip to the button */
	GtkWidget *button, *image;
#if GTK_CHECK_VERSION(3,0,0)
	GtkCssProvider *provider;
	GtkStyleContext *context;

	static const gchar button_style[] =
		"GtkButton#bluefish-small-close-button {\n"
		  "-GtkButton-default-border: 0;\n"
		  "-GtkButton-default-outside-border: 0;\n"
		  "-GtkButton-inner-border: 0;\n"
		  "-GtkWidget-focus-line-width: 0;\n"
		  "-GtkWidget-focus-padding: 0;\n"
		  "padding: 0;\n"
		"}";
#endif

	button = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
	gtk_widget_set_name(button, "bluefish-small-close-button");

	image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(button), image);

#if GTK_CHECK_VERSION(3,0,0)
	provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(provider, button_style, -1, NULL);
	context = gtk_widget_get_style_context(GTK_WIDGET(button));
	gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	g_object_unref(provider);
#else
	g_signal_connect(button, "style-set", G_CALLBACK(button_set_style), NULL);
#endif

	return button;
}

/**
 * dialog_stock_button_new_in_table:
 *  @stockID:        #const gchar * GTK stock ID or NULL.
 * 	@table:			 #GtkWidget * GtkTable the button is attached to.
 *  @left_attach:	 #guint The column number to attach the left side of the button to.
 *  @right_attach:	 #guint The column number to attach the right side of the button to.
 *  @top_attach:	 #guint The row number to attach the top of the button to.
 *  @bottom_attach:  #guint The row number to attach the bottom of the button to.
 *  
 * 	Creates a new GTK Stock buttton and places it in a GtkTable.
 *
 * Return value: #GtkWidget * The newly created GTK Stock button
 */
GtkWidget *
dialog_stock_button_new_in_table(const gchar * stockID, GtkWidget * table, guint left_attach,
								 guint right_attach, guint top_attach, guint bottom_attach)
{
	GtkWidget *button;

	button = gtk_button_new_from_stock(stockID);
	gtk_table_attach(GTK_TABLE(table), button, left_attach, right_attach, top_attach, bottom_attach, GTK_FILL,
					 GTK_SHRINK, 0, 0);
	return button;
}

/**
 * dialog_button_new_in_table:
 * 	@labeltext:		 #const gchar * The label text.
 * 	@table:			 #GtkWidget * GtkTable the button is attached to.
 *  @left_attach:	 #guint The column number to attach the left side of the button to.
 *  @right_attach:	 #guint The column number to attach the right side of the button to.
 *  @top_attach:	 #guint The row number to attach the top of the button to.
 *  @bottom_attach:  #guint The row number to attach the bottom of the button to.
 *  
 * 	Creates a new buttton with a label and places it in a GtkTable.
 *
 * Return value: #GtkWidget * The newly created button
 */
GtkWidget *
dialog_button_new_in_table(const gchar * labeltext, GtkWidget * table, guint left_attach, guint right_attach,
						   guint top_attach, guint bottom_attach)
{
	GtkWidget *button;

	button = gtk_button_new_with_mnemonic(labeltext);
	gtk_table_attach(GTK_TABLE(table), button, left_attach, right_attach, top_attach, bottom_attach, GTK_FILL,
					 GTK_SHRINK, 0, 0);
	return button;
}

/**
 * dialog_button_new_with_image:
 * 	@labeltext:		 #const gchar * The label text.
 * 	@pixmap:		 #gint pixmap ID.
 *  @stockID:        #const gchar * GTK stock ID or NULL.
 *  @iconSize:       #GtkIconSize Size of the displayed icon.
 *  
 * 	Creates a new buttton with an image and sets the image size.
 *  Image can be either a GTK Stock ID or a pixmap ID
 *
 * Return value: #GtkWidget * The newly created button
 */
GtkWidget *
dialog_button_new_with_image(const gchar * labeltext,const gchar * stockid,
							GCallback func, gpointer func_data, gboolean force_image, gboolean mnemonic)
{
	GtkWidget *button, *child=NULL, *label=NULL;
	
	if (labeltext)
		label = mnemonic ? gtk_label_new_with_mnemonic(labeltext) : gtk_label_new(labeltext);
		
	if (labeltext && stockid) {
		child = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(child), gtk_image_new_from_stock(stockid, GTK_ICON_SIZE_MENU), FALSE, FALSE, 1);
		gtk_box_pack_start(GTK_BOX(child), label, TRUE, TRUE, 1);
	} else if (force_image) {
		child = gtk_image_new_from_stock(stockid, GTK_ICON_SIZE_MENU);
	} else if (label) {
		child = label;
	}

	if (child) {
		button = gtk_button_new();
		gtk_container_add(GTK_CONTAINER(button), child);
	} else /* if (stockid) */ {
		button = gtk_button_new_from_stock(stockid);
	}
	g_signal_connect(G_OBJECT(button), "clicked", func, func_data);
	gtk_widget_set_can_default(button, TRUE);
	gtk_widget_show_all(button);
	return button;
}

/**
 * dialog_button_new_with_image_in_table:
 * 	@labeltext:		 #const gchar * The label text.
 * 	@pixmap:		 #gint pixmap ID.
 *  @stockID:        #const gchar * GTK stock ID or NULL.
 *  @iconSize:       #GtkIconSize Size of the displayed icon.
 * 	@table:			 #GtkWidget * GtkTable the button is attached to.
 *  @left_attach:	 #guint The column number to attach the left side of the button to.
 *  @right_attach:	 #guint The column number to attach the right side of the button to.
 *  @top_attach:	 #guint The row number to attach the top of the button to.
 *  @bottom_attach:  #guint The row number to attach the bottom of the button to.
 *  
 * 	Creates a new buttton with an image, sets the image size, and places it in a GtkTable.
 *  Image can be either a GTK Stock ID or a pixmap ID
 *
 * Return value: #GtkWidget * The newly created button
 */
GtkWidget *
dialog_button_new_with_image_in_table(const gchar * labeltext, const gchar * stockID,
										GCallback func, gpointer func_data, gboolean force_image, gboolean mnemonic,
										GtkWidget * table, guint left_attach,
										guint right_attach, guint top_attach, guint bottom_attach)
{
	GtkWidget *button;

	button = dialog_button_new_with_image(labeltext, stockID, func, func_data, force_image, mnemonic);
	gtk_table_attach(GTK_TABLE(table), button, left_attach, right_attach, top_attach, bottom_attach, GTK_FILL,
					 GTK_SHRINK, 0, 0);

	return button;
}

GtkWidget *
dialog_check_button_new(const gchar * labeltext, gint active)
{
	GtkWidget *button;

	button = gtk_check_button_new_with_mnemonic(labeltext);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), active);

	return button;
}

GtkWidget *
dialog_check_button_in_table(const gchar * labeltext, gint active, GtkWidget * table, guint left_attach,
							 guint right_attach, guint top_attach, guint bottom_attach)
{
	GtkWidget *button;

	button = dialog_check_button_new(labeltext, active);
	gtk_table_attach(GTK_TABLE(table), button, left_attach, right_attach, top_attach, bottom_attach, GTK_FILL,
					 GTK_SHRINK, 0, 0);

	return button;
}

GtkWidget *
dialog_color_button_in_table(const gchar * color, const gchar * title, GtkWidget * table, guint left_attach,
							 guint right_attach, guint top_attach, guint bottom_attach)
{
	GdkColor gdkcolor;
	GtkWidget *button;

	if (color && gdk_color_parse(color, &gdkcolor)) {
		button = gtk_color_button_new_with_color(&gdkcolor);
	} else {
		button = gtk_color_button_new();
		gtk_color_button_set_alpha(GTK_COLOR_BUTTON(button), 0);
	}

	gtk_color_button_set_title(GTK_COLOR_BUTTON(button), title);
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(button), TRUE);
	gtk_table_attach(GTK_TABLE(table), button, left_attach, right_attach, top_attach, bottom_attach, GTK_FILL,
					 GTK_SHRINK, 0, 0);

	return button;
}

GtkWidget *
dialog_combo_box_text_labeled_from_list(const GList * options, const gchar * value, const gchar * labeltext,
										GtkWidget * box, guint padding)
{
	GtkWidget *combobox;
	GtkWidget *label;

	combobox = combobox_with_popdown(value, options, FALSE);

	label = dialog_label_new(labeltext, 0, 0.5, box, padding);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), combobox);

	gtk_box_pack_start(GTK_BOX(box), combobox, FALSE, FALSE, 0);

	return combobox;
}

GtkWidget *
dialog_combo_box_text_from_list_in_table(const GList * options, const gchar * value, GtkWidget * table,
										 guint left_attach, guint right_attach, guint top_attach,
										 guint bottom_attach)
{
	GtkWidget *combobox;

	combobox = combobox_with_popdown(value, options, FALSE);
	gtk_table_attach(GTK_TABLE(table), combobox, left_attach, right_attach, top_attach, bottom_attach,
					 GTK_FILL, GTK_SHRINK, 0, 0);

	return combobox;
}

GtkWidget *
dialog_combo_box_text_new(const gchar ** options, gint index)
{
	GtkWidget *combobox;
	const gchar **tmp;

	combobox = gtk_combo_box_text_new();

	tmp = options;
	while (*tmp)
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), gettext(*tmp++));

	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), index);

	return combobox;
}

GtkWidget *
dialog_combo_box_text_in_table(const gchar ** options, gint index, GtkWidget * table, guint left_attach,
							   guint right_attach, guint top_attach, guint bottom_attach)
{
	GtkWidget *combobox;

	combobox = dialog_combo_box_text_new(options, index);
	gtk_table_attach(GTK_TABLE(table), combobox, left_attach, right_attach, top_attach, bottom_attach,
					 GTK_FILL, GTK_SHRINK, 0, 0);

	return combobox;
}

GtkWidget *
dialog_combo_box_text_labeled(const gchar * labeltext, const gchar ** options, gint index, GtkWidget * box,
							  guint padding)
{
	GtkWidget *combobox;
	GtkWidget *label;

	combobox = dialog_combo_box_text_new(options, index);

	label = dialog_label_new(labeltext, 0, 0.5, box, padding);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), combobox);

	gtk_box_pack_start(GTK_BOX(box), combobox, FALSE, FALSE, 0);

	return combobox;
}

GtkWidget *
dialog_entry_labeled(const gchar * text, const gchar * labeltext, GtkWidget * box, guint padding)
{
	GtkWidget *entry;
	GtkWidget *label;

	entry = gtk_entry_new();

	if (text)
		gtk_entry_set_text(GTK_ENTRY(entry), text);

	label = dialog_label_new(labeltext, 0, 0.5, box, padding);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), entry);

	gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);

	return entry;
}

GtkWidget *
dialog_entry_in_table(const gchar * text, GtkWidget * table, guint left_attach, guint right_attach,
					  guint top_attach, guint bottom_attach)
{
	GtkWidget *entry;

	entry = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), entry, left_attach, right_attach, top_attach, bottom_attach
					, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 0, 0);

	if (text)
		gtk_entry_set_text(GTK_ENTRY(entry), text);

	return entry;
}

GtkWidget *
dialog_mnemonic_label_new(const gchar * labeltext, GtkWidget * m_widget)
{
	GtkWidget *label;

	label = gtk_label_new(NULL);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), labeltext);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	if (m_widget)
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), m_widget);
	return label;
}

/**
 * dialog_mnemonic_label_in_table:
 * 	@labeltext:		 #const gchar * The label text.
 * 	@m_widget:		 #GtkWidget * GtkWidget that is the mnemonic target or NULL.
 * 	@table:			 #GtkWidget * GtkTable the label is attached to.
 *  @left_attach:	 #guint The column number to attach the left side of the label to.
 *  @right_attach:	 #guint The column number to attach the right side of the label to.
 *  @top_attach:	 #guint The row number to attach the top of the label to.
 *  @bottom_attach:  #guint The row number to attach the bottom of the label to.
 *  
 * 	Creates a GtkLabel, places it in a GtkTable, and sets it mnemonic widget.
 *
 * Return value: void
 */
GtkWidget *
dialog_mnemonic_label_in_table(const gchar * labeltext, GtkWidget * m_widget, GtkWidget * table,
							   guint left_attach, guint right_attach, guint top_attach, guint bottom_attach)
{
	GtkWidget *label = dialog_mnemonic_label_new(labeltext, m_widget);
	gtk_table_attach(GTK_TABLE(table), label, left_attach, right_attach, top_attach, bottom_attach, GTK_FILL,
					 GTK_SHRINK, 0, 0);
	return label;
}

GtkWidget *
dialog_radio_button_in_table(GSList * group, const gchar * label, GtkWidget * table, guint left_attach,
							 guint right_attach, guint top_attach, guint bottom_attach)
{
	GtkWidget *button;

	button = gtk_radio_button_new_with_mnemonic(group, label);
	gtk_table_attach(GTK_TABLE(table), button, left_attach, right_attach, top_attach, bottom_attach, GTK_FILL,
					 GTK_SHRINK, 0, 0);

	return button;
}

GtkWidget *
dialog_radio_button_from_widget_in_table(GtkRadioButton * radio_group_member, const gchar * label,
										 GtkWidget * table, guint left_attach, guint right_attach,
										 guint top_attach, guint bottom_attach)
{
	GtkWidget *button;

	button = gtk_radio_button_new_with_mnemonic_from_widget(radio_group_member, label);
	gtk_table_attach(GTK_TABLE(table), button, left_attach, right_attach, top_attach, bottom_attach, GTK_FILL,
					 GTK_SHRINK, 0, 0);

	return button;
}

GtkWidget *
dialog_spin_button_new(gfloat lower, gfloat upper, const gint value)
{
	GtkAdjustment *adjustment;
	GtkWidget *button;
	gfloat step_increment, page_increment;

	step_increment = (upper - lower) / 100;
	if (step_increment < 1)
		step_increment = 1;

	page_increment = (upper - lower) / 20;
	if (page_increment < 10)
		page_increment = 10;

	adjustment = (GtkAdjustment *)gtk_adjustment_new((1.0 * value), lower, upper, step_increment, page_increment, 0);
	button = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 0.1, 0);

	return button;
}

GtkWidget *
dialog_spin_button_in_table(gfloat lower, gfloat upper, const gint value, GtkWidget * table,
							guint left_attach, guint right_attach, guint top_attach, guint bottom_attach)
{
	GtkWidget *button;

	button = dialog_spin_button_new(lower, upper, value);
	gtk_table_attach(GTK_TABLE(table), button, left_attach, right_attach, top_attach, bottom_attach, GTK_FILL,
					 GTK_SHRINK, 0, 0);

	return button;
}

GtkWidget *
dialog_spin_button_labeled(gfloat lower, gfloat upper, const gint value, const gchar * labeltext,
						   GtkWidget * box, guint padding)
{
	GtkWidget *button;
	GtkWidget *label;

	button = dialog_spin_button_new(lower, upper, value);

	label = dialog_label_new(labeltext, 0, 0.5, box, padding);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), button);

	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);

	return button;
}

/**
 * dialog_box_label_new:
 * 	@labeltext:	#const gchar * The label text.
 * 	@xalign:	#gfloat The horizontal alignment, from 0 (left) to 1 (right).
 *  @yalign:	#gfloat the vertical alignment, from 0 (top) to 1 (bottom).
 * 	@box:		#GtkWidget * GtkBox the label is packed into.
 *  @padding:	#guint Extra space in pixels to put between the label and its neighbors.
 *
 * 	Creates a GtkLabel, aligns it, and packs it in a GtkBox.
 *
 * Return value: #GtkWidget * The new GtkLabel widget.
 */
GtkWidget *
dialog_label_new(const gchar * labeltext, gfloat xalign, gfloat yalign, GtkWidget * box, guint padding)
{
	GtkWidget *label;

	label = gtk_label_new(NULL);
	if (labeltext)
		gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), labeltext);
	gtk_misc_set_alignment(GTK_MISC(label), xalign, yalign);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, padding);

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
GtkWidget *
dialog_vbox_new(GtkWidget * box)
{
	GtkWidget *alignment, *vbox;

	alignment = gtk_alignment_new(0, 0, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 8, 4);
	gtk_box_pack_start(GTK_BOX(box), alignment, FALSE, FALSE, 0);
	vbox = gtk_vbox_new(FALSE, 8);
	gtk_container_add(GTK_CONTAINER(alignment), vbox);

	return vbox;
}

/**
 * dialog_vbox_labeled:
 * 	@labeltext:	#const gchar * The label text.
 * 	@box:		#GtkWidget * GtkBox the label is packed into.
 *  
 * 	Creates a GtkVBox and packs a new GtkLabel into it.
 *
 * Return value: #GtkWidget * The new GtkVBox widget.
 */
GtkWidget *
dialog_vbox_labeled(const gchar * labeltext, GtkWidget * box)
{
	dialog_label_new(labeltext, 0, 0, box, 0);
	return dialog_vbox_new(box);
}

/**
 * dialog_vbox_labeled_checkbutton:
 * 	@labeltext:	  #const gchar * The label text.
 *  @checkbutton: #GtkWidget * GtkWidget that is the mnemonic target.
 * 	@box:		  #GtkWidget * GtkBox the new GtkVBox is packed into.
 *  
 * 	Creates a GtkVBox with a GtkCheckButton as it's label.
 *
 * Return value: #GtkWidget * The new GtkVBox widget.
 */
GtkWidget *
dialog_vbox_labeled_checkbutton(const gchar * labeltext, GtkWidget * checkbutton, GtkWidget * box)
{
	GtkWidget *label, *hbox;

	hbox = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(hbox), checkbutton, FALSE, FALSE, 0);

	label = dialog_label_new(labeltext, 0, 0.5, hbox, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), checkbutton);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);

	return dialog_vbox_new(box);
}

/**
 * dialog_table_new:
 *  @rows:		   #gint The number of rows in the new table.
 *  @cols:		   #gint The number of columns in the new table.
 *  @borderWidth:  #gint Amount of blank space to leave outside the table.
 *  
 * 	Creates a GtkTable and optionally sets a border width.
 *
 * Return value: #GtkWidget * The new GtkTable widget
 */
static GtkWidget *
dialog_table_new(gint rows, gint cols, gint borderWidth)
{
	GtkWidget *table = gtk_table_new(rows, cols, FALSE);

	gtk_table_set_row_spacings(GTK_TABLE(table), 10);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);

	if (borderWidth > 0) {
		gtk_container_set_border_width(GTK_CONTAINER(table), borderWidth);
	}

	return table;
}

/**
 * dialog_table_in_vbox:
 *  @rows:		  #gint The number of rows in the new table.
 *  @cols:		  #gint The number of columns in the new table.
 *  @borderWidth: #gint Amount of blank space to leave outside the table.
 *  @box:		  #GtkWidget * The GtkVBox to pack the table into.
 *  @expand:	  #gboolean TRUE if the new table is to be given extra space allocated to box.
 *  @fill:		  #gboolean TRUE if space given to table by the expand option is actually allocated to table.
 *  @padding:	  #guint Extra space in pixels to put between the table and its neighbors.
 *  
 * 	Creates a GtkTable and packs it into a GtkVBox using the supplied values.
 *
 * Return value: #GtkWidget * The new GtkTable widget
 */
GtkWidget *
dialog_table_in_vbox(gint rows, gint cols, gint borderWidth, GtkWidget * box, gboolean expand, gboolean fill,
					 guint padding)
{
	GtkWidget *table = dialog_table_new(rows, cols, borderWidth);

	gtk_box_pack_start(GTK_BOX(box), table, expand, fill, padding);

	return table;
}

/**
 * dialog_table_in_vbox_defaults:
 *  @rows:		  #gint The number of rows in the new table.
 *  @cols:		  #gint The number of columns in the new table.
 *  @borderWidth: #gint Amount of blank space to leave outside the table.
 *  @box:		  #GtkWidget * The GtkVBox to pack the table into.
 *  
 * 	Creates a GtkTable and packs it into a GtkVBox using the default packing values for a GtkBox.
 *
 * Return value: #GtkWidget * The new GtkTable widget
 */
GtkWidget *
dialog_table_in_vbox_defaults(gint rows, gint cols, gint borderWidth, GtkWidget * box)
{
	GtkWidget *table = dialog_table_new(rows, cols, borderWidth);

	gtk_box_pack_start(GTK_BOX(box), table, TRUE, TRUE, 0);

	return table;
}

/*****************************************************************************/
/* Message Dialog Functions																  */
/*****************************************************************************/

/**
 * message_dialog_set_text:
 * 	@dialog:		#GtkWidget * The message dialog.
 * 	@primaryText:	#const gchar * The primary text displayed in the message dialog.
 * 	@secondaryText: #const gchar * The secondary text displayed in the message dialog.
 *  
 * 	Sets the primary and optional secondary text in a message dialog.
 *
 * Return value: void
 */
static void
message_dialog_set_text(GtkWidget * dialog, const gchar * primaryText, const gchar * secondaryText)
{
	gchar *text;

	if (!secondaryText) {
		text = g_strconcat("<span weight=\"bold\" size=\"larger\">", primaryText, "</span>\n", NULL);
	} else {
		text =
			g_strconcat("<span weight=\"bold\" size=\"larger\">", primaryText, "</span>\n\n", secondaryText,
						"\n", NULL);
	}

	gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), text);
	g_free(text);

	gtk_window_set_title(GTK_WINDOW(dialog), "");
}

/**
 * message_dialog_new:
 * 	@parent:		#GtkWidget * Transient parent, or NULL for none
 *  @type:			#GtkMessageType The type of message being displayed in the dialog.
 * 	@button:		#GtkButtonsType The type of button displayed in the dialog.
 * 	@primaryText:	#const gchar * The primary text displayed in the message dialog.
 * 	@secondaryText: #const gchar * The secondary text displayed in the message dialog.
 *  
 * 	Creates a modal GtkMessageDialog with a single button.
 *
 * Return value: void
 */
void
message_dialog_new(GtkWidget * parent, GtkMessageType type, GtkButtonsType button, const gchar * primaryText,
				   const gchar * secondaryText)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(GTK_WINDOW(parent), GTK_DIALOG_DESTROY_WITH_PARENT, type, button, NULL);

	message_dialog_set_text(dialog, primaryText, secondaryText);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

/**
 * message_dialog_new_multi:
 * 	@parent:		#GtkWidget * Transient parent, or NULL for none
 *  @type:			#GtkMessageType The type of message being displayed in the dialog.
 * 	@buttons:		#const gchar * An array of strings to be displayed on the dialog buttons.
 * 	@primaryText:	#const gchar * The primary text displayed in the message dialog.
 * 	@secondaryText: #const gchar * The secondary text displayed in the message dialog.
 *  
 * 	Creates a non-modal GtkMessageDialog with multiple buttons.
 *
 * Return value: #gint The response ID
 */
gint
message_dialog_new_multi(GtkWidget * parent, GtkMessageType type, const gchar * buttons[],
						 const gchar * primaryText, const gchar * secondaryText)
{
	GtkWidget *dialog;
	gint response;
	int i = 0;

	dialog =
		gtk_message_dialog_new((GtkWindow *) parent, GTK_DIALOG_DESTROY_WITH_PARENT, type, GTK_BUTTONS_NONE,
							   NULL);

	message_dialog_set_text(dialog, primaryText, secondaryText);

	for (i = 0; *buttons; i++) {
		gtk_dialog_add_button(GTK_DIALOG(dialog), *buttons++, i);
	}
	/* the Gnome HIG specficies that the default response should always be the far right button) */
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), i - 1);

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	return response;
}
