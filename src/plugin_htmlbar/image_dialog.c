/* Bluefish HTML Editor
* image_dialog.c
*
* Copyright (C) 2008-2012 James Hayward and Olivier Sessink
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

#include <string.h>
#include "htmlbar.h"
#include "image_dialog.h"
#include "html2.h"
#include "cap.h"

#include "../bf_lib.h"
#include "../dialog_utils.h"
#include "../document.h"
#include "../file.h"
#include "../gtk_easy.h"

#define BLUEFISH_IMAGE_DIALOG_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), BLUEFISH_TYPE_IMAGE_DIALOG, BluefishImageDialogPrivate))


struct _BluefishImageDialogPrivate {
	GFile *fileuri;
	gchar *filename;

	gint origHeight;
	gint origWidth;

	GdkPixbufLoader *pbloader;

	gint tagStart;
	gint tagEnd;

	GtkWidget *align;
	GtkWidget *alt;
	GtkWidget *border;
	GtkWidget *class;
	GtkWidget *custom;
	GtkWidget *dlg;
	GtkWidget *fileButton;
	GtkWidget *height;
	GtkWidget *heightPercent;
	GtkWidget *hspace;
	GtkWidget *id;
	GtkWidget *keepAspect;
	GtkWidget *longDesc;
	GtkWidget *preview;
	GtkWidget *previewInfo;
	GtkWidget *previewPane;
	GtkWidget *resetSizeButton;
	GtkWidget *source;
	GtkWidget *style;
	GtkWidget *transitionalVbox;
	GtkWidget *usemap;
	GtkWidget *useTransitional;
	GtkWidget *vspace;
	GtkWidget *width;
	GtkWidget *widthPercent;

	Tbfwin *bfwin;
	Tdocument *doc;
	Topenfile *openfile;
};

enum {
	PROP_0,
	PROP_BFWIN,
	PROP_SRC,
	PROP_WIDTH,
	PROP_WIDTH_IS_PERCENT,
	PROP_HEIGHT,
	PROP_HEIGHT_IS_PERCENT,
	PROP_ALT,
	PROP_KEEP_ASPECT,
	PROP_LONGDESC,
	PROP_CLASS,
	PROP_ID,
	PROP_USEMAP,
	PROP_CUSTOM,
	PROP_ALIGN,
	PROP_BORDER,
	PROP_HSPACE,
	PROP_VSPACE,
	PROP_USE_TRANSITIONAL,
	PROP_TAG_START,
	PROP_TAG_END,
	PROP_STYLE
};

G_DEFINE_TYPE(BluefishImageDialog, bluefish_image_dialog, GTK_TYPE_DIALOG)

/* static function prototypes */
static void image_dialog_reset_dimensions(GtkButton * button, BluefishImageDialog * dialog);

static void image_dialog_source_activate(GtkWidget * widget, BluefishImageDialog * dialog);

static void image_dialog_source_changed(GtkWidget * widget, BluefishImageDialog * dialog);

/*static void filebutton_clicked(GtkButton * button, BluefishImageDialog * imageDialog);*/

static void image_dialog_height_percent_toggled(GtkToggleButton * togglebutton, BluefishImageDialog * dialog);

static void image_dialog_width_percent_toggled(GtkToggleButton * togglebutton, BluefishImageDialog * dialog);

static void
image_dialog_use_transitional_toggled(GtkToggleButton * togglebutton, BluefishImageDialog * dialog);

static void image_dialog_response_lcb(GtkDialog * dialog, gint response, BluefishImageDialog * imageDialog);
/* end static function prototypes */


static void
bluefish_image_dialog_finalize(GObject * object)
{
	BluefishImageDialog *dialog = BLUEFISH_IMAGE_DIALOG(object);

	if (dialog->priv->fileuri)
		g_object_unref(dialog->priv->fileuri);

	if (dialog->priv->filename)
		g_free(dialog->priv->filename);

	G_OBJECT_CLASS(bluefish_image_dialog_parent_class)->finalize(object);
}

static void
bluefish_image_dialog_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
	BluefishImageDialog *dialog = BLUEFISH_IMAGE_DIALOG(object);

	switch (prop_id) {
	case PROP_BFWIN:
		g_value_set_pointer(value, dialog->priv->bfwin);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void
bluefish_image_dialog_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
	BluefishImageDialog *dialog = BLUEFISH_IMAGE_DIALOG(object);
	DEBUG_MSG("bluefish_image_dialog_set_property, called for prop_id=%d\n",prop_id);
	switch (prop_id) {
	case PROP_BFWIN:
		dialog->priv->bfwin = g_value_get_pointer(value);
		break;
	case PROP_SRC:
/*		g_print("bluefish_image_dialog_set_property, src will be set to:\n");*/
		if (g_value_get_string(value) != NULL) {
/*			g_print("bluefish_image_dialog_set_property, %s\n", g_value_get_string(value));*/
			gtk_entry_set_text(GTK_ENTRY(dialog->priv->source), g_value_get_string(value));
		}
		break;
	case PROP_WIDTH:
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->priv->width), g_value_get_double(value));
		break;
	case PROP_WIDTH_IS_PERCENT:
		g_signal_handlers_block_matched(dialog->priv->widthPercent,
				G_SIGNAL_MATCH_FUNC, 0, 0, NULL, image_dialog_width_percent_toggled, NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->priv->widthPercent),
				g_value_get_boolean(value));
		g_signal_handlers_unblock_matched(dialog->priv->widthPercent,
						G_SIGNAL_MATCH_FUNC, 0, 0, NULL, image_dialog_width_percent_toggled, NULL);
		break;
	case PROP_HEIGHT:
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->priv->height), g_value_get_double(value));
		break;
	case PROP_HEIGHT_IS_PERCENT:
		g_signal_handlers_block_matched(dialog->priv->heightPercent,
				G_SIGNAL_MATCH_FUNC, 0, 0, NULL, image_dialog_height_percent_toggled, NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->priv->heightPercent),
				g_value_get_boolean(value));
		g_signal_handlers_unblock_matched(dialog->priv->heightPercent,
				G_SIGNAL_MATCH_FUNC, 0, 0, NULL, image_dialog_height_percent_toggled, NULL);
		break;
	case PROP_ALT:
		if (g_value_get_string(value) != NULL)
			gtk_entry_set_text(GTK_ENTRY(dialog->priv->alt), g_value_get_string(value));
		break;
	case PROP_KEEP_ASPECT:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->priv->keepAspect),
				g_value_get_boolean(value));
		break;
	case PROP_LONGDESC:
		if (g_value_get_string(value) != NULL)
			gtk_entry_set_text(GTK_ENTRY(dialog->priv->longDesc), g_value_get_string(value));
		break;
	case PROP_CLASS:
		if (g_value_get_string(value) != NULL) {
			gtk_combo_box_text_prepend_text(GTK_COMBO_BOX_TEXT(dialog->priv->class), g_value_get_string(value));
			gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->priv->class), 0);
		}
		break;
	case PROP_ID:
		if (g_value_get_string(value) != NULL)
			gtk_entry_set_text(GTK_ENTRY(dialog->priv->id), g_value_get_string(value));
		break;
	case PROP_USEMAP:
		if (g_value_get_string(value) != NULL)
			gtk_entry_set_text(GTK_ENTRY(dialog->priv->usemap), g_value_get_string(value));
		break;
	case PROP_CUSTOM:
		if (g_value_get_string(value) != NULL) {
			gchar *string = g_strdup(g_value_get_string(value));
			g_strstrip(string);
			gtk_entry_set_text(GTK_ENTRY(dialog->priv->custom), string);
			g_free(string);
		}
		break;
	case PROP_ALIGN:
		gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->priv->align), g_value_get_int(value));
		break;
	case PROP_BORDER:
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->priv->border), g_value_get_double(value));
		break;
	case PROP_HSPACE:
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->priv->hspace), g_value_get_double(value));
		break;
	case PROP_VSPACE:
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->priv->vspace), g_value_get_double(value));
		break;
	case PROP_USE_TRANSITIONAL:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->priv->useTransitional),
									 g_value_get_boolean(value));
		break;
	case PROP_TAG_START:
		dialog->priv->tagStart = g_value_get_int(value);
		DEBUG_MSG("bluefish_image_dialog_set_property, tagStart set to %d\n", dialog->priv->tagStart);
		break;
	case PROP_TAG_END:
		dialog->priv->tagEnd = g_value_get_int(value);
		DEBUG_MSG("bluefish_image_dialog_set_property, tagEnd set to %d\n", dialog->priv->tagEnd);
		break;
	case PROP_STYLE:
		if (g_value_get_string(value) != NULL)
			gtk_entry_set_text(GTK_ENTRY(dialog->priv->style), g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void
spin_buttons_value_changed(GtkSpinButton *spinbutton,BluefishImageDialog * dialog)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->priv->keepAspect)) && dialog->priv->origHeight > 0 && dialog->priv->origWidth > 0) {
		gfloat targetval;
		GtkWidget *target;
		if ((GtkWidget *) spinbutton == dialog->priv->height) {
			target = dialog->priv->width;
			targetval = 1.0 * gtk_spin_button_get_value(GTK_SPIN_BUTTON(spinbutton)) / dialog->priv->origHeight * dialog->priv->origWidth;
		} else {
			target = dialog->priv->height;
			targetval = 1.0 * gtk_spin_button_get_value(GTK_SPIN_BUTTON(spinbutton)) / dialog->priv->origWidth * dialog->priv->origHeight;
		}
		DEBUG_MSG("spin_buttons_value_changed, set %p to value %f\n",target,targetval);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(target), targetval);
	}
}

static GObject *
bluefish_image_dialog_create(GType type, guint n_construct_properties,
							 GObjectConstructParam * construct_properties)
{
	BluefishImageDialogClass *klass =
		BLUEFISH_IMAGE_DIALOG_CLASS(g_type_class_peek(BLUEFISH_TYPE_IMAGE_DIALOG));
	GObjectClass *parent_class = G_OBJECT_CLASS(g_type_class_peek_parent(klass));
	GObject *obj = parent_class->constructor(type,
											 n_construct_properties,
											 construct_properties);
	BluefishImageDialog *dialog = BLUEFISH_IMAGE_DIALOG(obj);

	GtkWidget *alignment, *frame, *hbox, *vbox, *but;
	GtkWidget *label, *notebook, *table;
	unsigned int i = 0;

	const gchar *alignments[] = {
		"",
		"bottom",
		"left",
		"middle",
		"right",
		"top",
	};

	dialog->priv->doc = dialog->priv->bfwin->current_document;
	dialog->priv->openfile = NULL;
	dialog->priv->preview = NULL;
	dialog->priv->previewInfo = NULL;
	dialog->priv->fileuri = NULL;

	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK, GTK_RESPONSE_OK);

	g_signal_connect(dialog, "response", G_CALLBACK(image_dialog_response_lcb), dialog);

	alignment = gtk_alignment_new(0, 0, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 6, 12, 6, 6);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), alignment, TRUE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(alignment), vbox);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic(_("_Source:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	dialog->priv->source = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), dialog->priv->source, TRUE, TRUE, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), dialog->priv->source);
	g_signal_connect(dialog->priv->source, "activate", G_CALLBACK(image_dialog_source_activate), dialog);
	g_signal_connect(dialog->priv->source, "changed", G_CALLBACK(image_dialog_source_changed), dialog);

	dialog->priv->fileButton = file_but_new2(dialog->priv->source, 0, dialog->priv->bfwin, GTK_FILE_CHOOSER_ACTION_OPEN);
	/*dialog_button_new_with_image(NULL, GTK_STOCK_OPEN, G_CALLBACK(filebutton_clicked), dialog, TRUE, FALSE);*/
	gtk_button_set_focus_on_click(GTK_BUTTON(dialog->priv->fileButton), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), dialog->priv->fileButton, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	frame = gtk_frame_new(_("Preview"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	dialog->priv->previewPane = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(frame), dialog->priv->previewPane);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

	notebook = gtk_notebook_new();
	gtk_notebook_popup_disable(GTK_NOTEBOOK(notebook));
	gtk_box_pack_start(GTK_BOX(vbox), notebook, FALSE, FALSE, 0);

	vbox = gtk_vbox_new(FALSE, 6);
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	table = dialog_table_in_vbox(2, 4, 6, vbox, FALSE, FALSE, 0);

	dialog->priv->width = gtk_spin_button_new_with_range(0, 3000, 1);
	gtk_table_attach(GTK_TABLE(table), dialog->priv->width, 1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	dialog_mnemonic_label_in_table(_("_Width:"), dialog->priv->width, table, 0, 1, 0, 1);
	g_signal_connect(dialog->priv->width, "value-changed",
					 G_CALLBACK(spin_buttons_value_changed), dialog);

	dialog->priv->widthPercent = gtk_check_button_new_with_label("%");
	gtk_table_attach(GTK_TABLE(table), dialog->priv->widthPercent, 2, 3, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	g_signal_connect(dialog->priv->widthPercent, "toggled",
					 G_CALLBACK(image_dialog_width_percent_toggled), dialog);

	dialog->priv->height = gtk_spin_button_new_with_range(0, 3000, 1);
	gtk_table_attach(GTK_TABLE(table), dialog->priv->height, 1, 2, 1, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);
	dialog_mnemonic_label_in_table(_("_Height:"), dialog->priv->height, table, 0, 1, 1, 2);
	g_signal_connect(dialog->priv->height, "value-changed",
					 G_CALLBACK(spin_buttons_value_changed), dialog);

	dialog->priv->heightPercent = gtk_check_button_new_with_label("%");
	gtk_table_attach(GTK_TABLE(table), dialog->priv->heightPercent, 2, 3, 1, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);
	g_signal_connect(dialog->priv->heightPercent, "toggled",
					 G_CALLBACK(image_dialog_height_percent_toggled), dialog);

	dialog->priv->keepAspect = gtk_check_button_new_with_label(_("Keep aspect ratio"));
	gtk_table_attach(GTK_TABLE(table), dialog->priv->keepAspect, 3, 4, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);

	dialog->priv->resetSizeButton = dialog_button_new_with_image_in_table(_("_Reset Dimensions"),
									  GTK_STOCK_REFRESH,G_CALLBACK(image_dialog_reset_dimensions),dialog,FALSE, TRUE,
									  table, 3, 4, 1, 2);
	gtk_widget_set_sensitive(dialog->priv->resetSizeButton, FALSE);

	table = dialog_table_in_vbox(7, 3, 6, vbox, TRUE, TRUE, 0);

	dialog->priv->alt = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), dialog->priv->alt, 1, 3, 0, 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0,
					 0);
	dialog_mnemonic_label_in_table(_("Alternate te_xt:"), dialog->priv->alt, table, 0, 1, 0, 1);

	dialog->priv->longDesc = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), dialog->priv->longDesc, 1, 3, 1, 2, GTK_EXPAND | GTK_FILL, GTK_SHRINK,
					 0, 0);
	dialog_mnemonic_label_in_table(_("_Long description:"), dialog->priv->longDesc, table, 0, 1, 1, 2);

	dialog->priv->class = gtk_combo_box_text_new_with_entry();
	gtk_table_attach(GTK_TABLE(table), dialog->priv->class, 1, 3, 2, 3, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0,
					 0);
	dialog_mnemonic_label_in_table(_("Cl_ass:"), dialog->priv->class, table, 0, 1, 2, 3);

	dialog->priv->id = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), dialog->priv->id, 1, 3, 3, 4, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
	dialog_mnemonic_label_in_table(_("I_D:"), dialog->priv->id, table, 0, 1, 3, 4);

	dialog->priv->usemap = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), dialog->priv->usemap, 1, 3, 4, 5, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0,
					 0);
	dialog_mnemonic_label_in_table(_("_Usemap:"), dialog->priv->usemap, table, 0, 1, 4, 5);

	dialog->priv->style = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), dialog->priv->style, 1, 2, 5, 6, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0,
					 0);
	dialog_mnemonic_label_in_table(_("Style:"), dialog->priv->style, table, 0, 1, 5, 6);
	but = style_but_new(GTK_WIDGET(dialog->priv->style), GTK_WIDGET(dialog));
	gtk_table_attach(GTK_TABLE(table), but, 2, 3, 5, 6, GTK_FILL, GTK_SHRINK, 0, 0);
	dialog->priv->custom = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), dialog->priv->custom, 1, 3, 6, 7, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0,
					 0);
	dialog_mnemonic_label_in_table(_("Custo_m:"), dialog->priv->custom, table, 0, 1, 6, 7);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, gtk_label_new("Strict"));

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	dialog->priv->useTransitional = gtk_check_button_new();
	dialog->priv->transitionalVbox =
		dialog_vbox_labeled_checkbutton(_("_Use Transitional options"), dialog->priv->useTransitional, vbox);
	g_signal_connect(GTK_TOGGLE_BUTTON(dialog->priv->useTransitional), "toggled",
					 G_CALLBACK(image_dialog_use_transitional_toggled), dialog);

	table = dialog_table_in_vbox(4, 2, 6, dialog->priv->transitionalVbox, FALSE, FALSE, 0);

	dialog->priv->align = gtk_combo_box_text_new();
	for (i = 0; i < G_N_ELEMENTS(alignments); i++) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(dialog->priv->align), alignments[i]);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->priv->align), 0);
	dialog_mnemonic_label_in_table(_("_Align:"), dialog->priv->align, table, 0, 1, 0, 1);
	gtk_table_attach(GTK_TABLE(table), dialog->priv->align, 1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);

	dialog->priv->border = gtk_spin_button_new_with_range(-1, 500, 1);
	gtk_table_attach(GTK_TABLE(table), dialog->priv->border, 1, 2, 1, 2, GTK_FILL, GTK_SHRINK, 0, 0);
	dialog_mnemonic_label_in_table(_("_Border:"), dialog->priv->border, table, 0, 1, 1, 2);

	dialog->priv->hspace = gtk_spin_button_new_with_range(-1, 500, 1);
	gtk_table_attach(GTK_TABLE(table), dialog->priv->hspace, 1, 2, 2, 3, GTK_FILL, GTK_SHRINK, 0, 0);
	dialog_mnemonic_label_in_table(_("_Hspace:"), dialog->priv->hspace, table, 0, 1, 2, 3);

	dialog->priv->vspace = gtk_spin_button_new_with_range(-1, 500, 1);
	gtk_table_attach(GTK_TABLE(table), dialog->priv->vspace, 1, 2, 3, 4, GTK_FILL, GTK_SHRINK, 0, 0);
	dialog_mnemonic_label_in_table(_("_Vspace:"), dialog->priv->vspace, table, 0, 1, 3, 4);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, gtk_label_new("Transitional"));

	image_dialog_use_transitional_toggled(GTK_TOGGLE_BUTTON(dialog->priv->useTransitional), dialog);

	return (obj);
}

static void
bluefish_image_dialog_class_init(BluefishImageDialogClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->constructor = bluefish_image_dialog_create;
	object_class->finalize = bluefish_image_dialog_finalize;

	object_class->get_property = bluefish_image_dialog_get_property;
	object_class->set_property = bluefish_image_dialog_set_property;

	g_type_class_add_private(object_class, sizeof(BluefishImageDialogPrivate));

	g_object_class_install_property(object_class,
									PROP_BFWIN,
									g_param_spec_pointer("bfwin",
														 "bfwin",
														 "The image dialogs bfwin",
														 G_PARAM_READWRITE |
														 G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class,
									PROP_SRC,
									g_param_spec_string("src",
														"src",
														"The image source",
														NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class,
									PROP_WIDTH,
									g_param_spec_double("width",
														"width",
														"The image width",
														0, 3000, 0,
														G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class,
									PROP_WIDTH_IS_PERCENT,
									g_param_spec_boolean("width-is-percent",
														 "width is percent",
														 "If image width is a percent",
														 FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class,
									PROP_HEIGHT,
									g_param_spec_double("height",
														"height",
														"The image height",
														0, 3000, 0,
														G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class,
									PROP_HEIGHT_IS_PERCENT,
									g_param_spec_boolean("height-is-percent",
														 "height is percent",
														 "If image height is a percent",
														 FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class,
									PROP_ALT,
									g_param_spec_string("alt",
														"alt",
														"Alternate text",
														NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property(object_class,
									PROP_KEEP_ASPECT,
									g_param_spec_boolean("keep-aspect",
														 "keep aspect ratio",
														 "Keepaspect ratio",
														 TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class,
									PROP_LONGDESC,
									g_param_spec_string("longdesc",
														"longdesc",
														"Long description",
														NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class,
									PROP_CLASS,
									g_param_spec_string("class",
														"class",
														"Class",
														NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class,
									PROP_ID,
									g_param_spec_string("id",
														"id",
														"ID",
														NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class,
									PROP_USEMAP,
									g_param_spec_string("usemap",
														"usemap",
														"Usemap",
														NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class,
									PROP_CUSTOM,
									g_param_spec_string("custom",
														"custom",
														"Custom",
														NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));


	g_object_class_install_property(object_class,
									PROP_ALIGN,
									g_param_spec_int("align",
													 "align",
													 "The image alignment",
													 0, 5, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class,
									PROP_BORDER,
									g_param_spec_double("border",
														"border",
														"The image border width",
														-1, 500, -1,
														G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class,
									PROP_HSPACE,
									g_param_spec_double("hspace",
														"hspace",
														"The image hspace",
														-1, 500, -1,
														G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class,
									PROP_VSPACE,
									g_param_spec_double("vspace",
														"vspace",
														"The image vspace",
														-1, 500, -1,
														G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class,
									PROP_USE_TRANSITIONAL,
									g_param_spec_boolean("use-transitional",
														 "use transitional",
														 "Use transitional options",
														 FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class,
									PROP_TAG_START,
									g_param_spec_int("tag-start",
													 "tag start",
													 "Start position to replace existing tag",
													 -1, G_MAXINT, -1,
													 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class,
									PROP_TAG_END,
									g_param_spec_int("tag-end",
													 "tag end",
													 "End position to replace existing tag",
													 -1, G_MAXINT, -1,
													 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property(object_class,
									PROP_STYLE,
									g_param_spec_string("style",
														"style",
														"Style",
														NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}

static void
bluefish_image_dialog_init(BluefishImageDialog * dialog)
{
	dialog->priv = BLUEFISH_IMAGE_DIALOG_GET_PRIVATE(dialog);
	g_return_if_fail(dialog->priv != NULL);
}

static void
pbloader_size_prepared(GdkPixbufLoader * pbloader, gint width, gint height, BluefishImageDialog * dialog)
{
	DEBUG_MSG("pbloader_size_prepared, width=%d, height=%d\n", width, height);
	dialog->priv->origWidth = width;
	dialog->priv->origHeight = height;

	if (width > 256) {
		gfloat scale = width / 256.0;
		gdk_pixbuf_loader_set_size(pbloader, width / scale, height / scale);
	}
	DEBUG_MSG("pbloader_size_prepared, done\n");
}

static GdkPixbufLoader *
pbloader_get_for_mime_type(const gchar * mimetype)
{
	GdkPixbufLoader *pbloader = NULL;

	if (mimetype) {
		GError *error = NULL;

		pbloader = gdk_pixbuf_loader_new_with_mime_type(mimetype, &error);

		if (error) {
			g_message("%s", error->message);
			g_error_free(error);
		}
	}

	if (!pbloader) {
		pbloader = gdk_pixbuf_loader_new();
	}

	return pbloader;
}

static void
image_dialog_reset_dimensions(GtkButton * button, BluefishImageDialog * dialog)
{
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->priv->width), dialog->priv->origWidth);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->priv->height), dialog->priv->origHeight);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->priv->widthPercent)))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->priv->widthPercent), FALSE);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->priv->heightPercent)))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->priv->heightPercent), FALSE);
}

static void
image_dialog_set_source(BluefishImageDialog * dialog)
{
	gchar *relpath = NULL;

	DEBUG_MSG("image_dialog_set_source, started\n");
	if (dialog->priv->doc->uri != NULL) {
		gchar *curi1, *curi2;
		curi1 = g_file_get_uri(dialog->priv->doc->uri);
		curi2 = g_file_get_uri(dialog->priv->fileuri);
		DEBUG_MSG("curi1=%s, curi2=%s\n", curi1, curi2);
		relpath = create_relative_link_to(curi1, curi2);
		DEBUG_MSG("image_dialog_set_source, relpath=%s\n", relpath);
		g_free(curi1);
		g_free(curi2);
	}

	if (relpath) {
		dialog->priv->filename = g_uri_unescape_string(relpath, NULL);;
		g_free(relpath);
	}
	else
		dialog->priv->filename = g_file_get_parse_name(dialog->priv->fileuri);

	g_signal_handlers_block_matched(dialog->priv->source,
									G_SIGNAL_MATCH_FUNC, 0, 0, NULL, image_dialog_source_changed, NULL);
	DEBUG_MSG("image_dialog_set_source, filename=%s\n", dialog->priv->filename);
	gtk_entry_set_text(GTK_ENTRY(dialog->priv->source), dialog->priv->filename);
	gtk_editable_set_position(GTK_EDITABLE(dialog->priv->source), -1);

	g_signal_handlers_unblock_matched(dialog->priv->source,
									  G_SIGNAL_MATCH_FUNC, 0, 0, NULL, image_dialog_source_changed, NULL);
}

static void
image_dialog_set_preview_info(BluefishImageDialog * dialog, const gchar * string)
{
	dialog->priv->previewInfo = gtk_label_new(string);
	gtk_label_set_use_markup(GTK_LABEL(dialog->priv->previewInfo), TRUE);
	gtk_label_set_justify(GTK_LABEL(dialog->priv->previewInfo), GTK_JUSTIFY_CENTER);
	gtk_box_pack_start(GTK_BOX(dialog->priv->previewPane), dialog->priv->previewInfo, FALSE, FALSE, 0);
	gtk_widget_show(dialog->priv->previewInfo);
}

static void
image_dialog_preview_loaded(BluefishImageDialog * dialog)
{
	GFileInfo *fileinfo;
	GError *error = NULL;
	DEBUG_MSG("image_dialog_preview_loaded, started\n");
	if (dialog->priv->previewInfo) {
		gtk_widget_destroy(dialog->priv->previewInfo);
		dialog->priv->previewInfo = NULL;
	}

	gtk_box_pack_start(GTK_BOX(dialog->priv->previewPane), dialog->priv->preview, FALSE, FALSE, 6);
	gtk_widget_show(dialog->priv->preview);

	fileinfo = g_file_query_info(dialog->priv->fileuri,
								 "standard::display-name,standard::size",
								 G_FILE_QUERY_INFO_NONE, NULL, &error);

	if (error == NULL) {
		GString *previewInfo;

		previewInfo = g_string_new(g_file_info_get_display_name(fileinfo));

		if (g_file_info_has_attribute(fileinfo, G_FILE_ATTRIBUTE_STANDARD_SIZE)) {
			gchar *filesize;

#if (GLIB_CHECK_VERSION(2,30,0))
			guint64 size;

			size = g_file_info_get_attribute_uint64(fileinfo, G_FILE_ATTRIBUTE_STANDARD_SIZE);
			filesize = g_format_size(size);
#else
			goffset size;

			size = g_file_info_get_size(fileinfo);
			filesize = g_format_size_for_display(size);
#endif
			g_string_append_printf(previewInfo, _("\n%s"), filesize);

			g_free(filesize);
		}

		g_string_append_printf(previewInfo, _("\n%d x %d pixels"),
							   dialog->priv->origWidth, dialog->priv->origHeight);

		image_dialog_set_preview_info(dialog, previewInfo->str);

		g_string_free(previewInfo, TRUE);
	}

	g_object_unref(fileinfo);

	if (gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dialog->priv->height)) == 0)
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->priv->height), dialog->priv->origHeight);
	if (gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dialog->priv->width)) == 0)
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->priv->width), dialog->priv->origWidth);

	gtk_widget_set_sensitive(dialog->priv->resetSizeButton, TRUE);
	image_dialog_set_source(dialog);
}

static void
image_dialog_load_preview(Topenfile_status status,
						  GError * gerror, Trefcpointer *refcp, goffset buflen, gpointer callback_data)
{
	BluefishImageDialog *imageDialog = callback_data;
	gboolean cleanup = TRUE;
	DEBUG_MSG("image_dialog_load_preview, started with status %d\n", status);
	switch (status) {
	case OPENFILE_ERROR:
	case OPENFILE_ERROR_NOCHANNEL:
	case OPENFILE_ERROR_NOREAD:
		/* TODO: warn the user */
	case OPENFILE_ERROR_CANCELLED:
		gdk_pixbuf_loader_close(imageDialog->priv->pbloader, NULL);
		break;
	case OPENFILE_CHANNEL_OPENED:
		cleanup = FALSE;
		break;
	case OPENFILE_FINISHED:
		{
			GdkPixbuf *pixbuf;
			GError *error = NULL;
			DEBUG_MSG("limage_dialog_load_preview, oading data into pixbuf\n");
			if (gdk_pixbuf_loader_write(imageDialog->priv->pbloader, (guchar *) refcp->data, buflen, &error)
				&& gdk_pixbuf_loader_close(imageDialog->priv->pbloader, &error)) {

				pixbuf = gdk_pixbuf_loader_get_pixbuf(imageDialog->priv->pbloader);
				if (pixbuf) {
					imageDialog->priv->preview = gtk_image_new_from_pixbuf(pixbuf);
					image_dialog_preview_loaded(imageDialog);
				}
			}
		}
		break;
	}

	if (cleanup) {
		g_object_unref(imageDialog->priv->pbloader);
		imageDialog->priv->pbloader = NULL;
		imageDialog->priv->openfile = NULL;
	}
}

static void
image_dialog_remove_preview(BluefishImageDialog * dialog)
{
	if (dialog->priv->preview) {
		gtk_widget_destroy(dialog->priv->preview);
		dialog->priv->preview = NULL;
	}

	if (dialog->priv->previewInfo) {
		gtk_widget_destroy(dialog->priv->previewInfo);
		dialog->priv->previewInfo = NULL;
	}

	if (dialog->priv->openfile) {
		openfile_cancel(dialog->priv->openfile);
	}

	if (dialog->priv->filename) {
		g_free(dialog->priv->filename);
		dialog->priv->filename = NULL;
	}

	dialog->priv->origHeight = 0;
	dialog->priv->origWidth = 0;
	image_dialog_reset_dimensions(NULL, dialog);
	gtk_widget_set_sensitive(dialog->priv->resetSizeButton, FALSE);
}

static void
image_dialog_set_preview(BluefishImageDialog * dialog, const gchar * mimetype)
{
	DEBUG_MSG("image_dialog_set_preview, started\n");
	if (dialog->priv->preview)
		image_dialog_remove_preview(dialog);
	DEBUG_MSG("image_dialog_set_preview, removed previous preview, loading new preview\n");
	if (!g_file_has_uri_scheme(dialog->priv->fileuri, "file"))
		image_dialog_set_preview_info(dialog, _("\n\n\t<b>Loading preview...</b>\t\n\n"));

	dialog->priv->pbloader = pbloader_get_for_mime_type(mimetype);

	g_signal_connect(dialog->priv->pbloader, "size-prepared", G_CALLBACK(pbloader_size_prepared), dialog);

	dialog->priv->openfile =
		file_openfile_uri_async(dialog->priv->fileuri, NULL, image_dialog_load_preview, dialog);
}

static void
image_dialog_check_is_image_file(BluefishImageDialog * dialog)
{
	GFileInfo *fileinfo;
	GError *error = NULL;
	fileinfo = g_file_query_info(dialog->priv->fileuri,
								 G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
								 G_FILE_QUERY_INFO_NONE, NULL, &error);
	if (error == NULL) {
		if (g_file_info_has_attribute(fileinfo, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE)) {
#ifdef WIN32
			const gchar *contenttype;
			gchar *mimetype = NULL;
			contenttype = g_file_info_get_content_type(fileinfo);
			if (contenttype)
				mimetype = g_content_type_get_mime_type(contenttype);
#else
			const gchar *mimetype = g_file_info_get_content_type(fileinfo);
#endif
			DEBUG_MSG("found mimetype %s\n", mimetype);
			if (mimetype && strncmp(mimetype, "image/", 6) == 0) {
				image_dialog_set_preview(dialog, mimetype);
#ifdef WIN32

				g_free(mimetype);
#endif
			}
		}
	} else {
		g_warning("failed to find if image mimetype %s\n", error->message);
		g_error_free(error);
	}

	if (fileinfo)
		g_object_unref(fileinfo);
}

static void
source_changed_or_activate(BluefishImageDialog * dialog, gboolean is_activate)
{
	const gchar *filename = NULL;
	gchar *tmp;
	DEBUG_MSG("source_changed_or_activate, started\n");
	if (dialog->priv->preview)
		image_dialog_remove_preview(dialog);

	if (dialog->priv->fileuri) {
		g_object_unref(dialog->priv->fileuri);
		dialog->priv->fileuri = NULL;
	}

	filename = gtk_entry_get_text(GTK_ENTRY(dialog->priv->source));
	DEBUG_MSG("source_changed_or_activate, filename=%s\n", filename);
	if (filename && strlen(filename)) {
		tmp = strstr(filename, "://");
		if ((tmp == NULL && filename[0] != '/') && dialog->priv->doc->uri) {
			GFile *parent = g_file_get_parent(dialog->priv->doc->uri);
			dialog->priv->fileuri = g_file_resolve_relative_path(parent, filename);
			g_object_unref(parent);
		} else if (tmp != NULL) {
			dialog->priv->fileuri = g_file_new_for_uri(filename);
			if (is_activate == FALSE && !g_file_has_uri_scheme(dialog->priv->fileuri, "file"))
				return;
		} else if (filename[0] == '/')
			dialog->priv->fileuri = g_file_new_for_path(filename);
		else
			return;

		if (dialog->priv->fileuri && g_file_query_exists(dialog->priv->fileuri, NULL))
			image_dialog_check_is_image_file(dialog);
	}
}

static void
image_dialog_source_activate(GtkWidget * widget, BluefishImageDialog * dialog)
{
	source_changed_or_activate(dialog, TRUE);
}

static void
image_dialog_source_changed(GtkWidget * widget, BluefishImageDialog * dialog)
{
	source_changed_or_activate(dialog, FALSE);
}
#ifdef NOT_GENERIC_FILEBUT
static void
filebutton_clicked(GtkButton * button, BluefishImageDialog * imageDialog)
{
	GtkWidget *dialog;
	GtkFileFilter *filefilter;
	gchar *seturi = NULL;

	dialog = gtk_file_chooser_dialog_new(_("Select Image"),
										 GTK_WINDOW(imageDialog),
										 GTK_FILE_CHOOSER_ACTION_OPEN,
										 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
										 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("All images"));
	gtk_file_filter_add_pixbuf_formats(filefilter);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filefilter);

	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), FALSE);

	if (imageDialog->priv->fileuri && imageDialog->priv->preview) {
		seturi = g_file_get_uri(imageDialog->priv->fileuri);

		if (!gtk_file_chooser_set_uri(GTK_FILE_CHOOSER(dialog), seturi)) {
			g_free(seturi);
			seturi = NULL;
		}
	} else if (imageDialog->priv->doc->uri) {
		GFile *parent = g_file_get_parent(imageDialog->priv->doc->uri);
		seturi = g_file_get_uri(parent);
		g_object_unref(parent);

		if (seturi)
			gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), seturi);
	}

	if (seturi == NULL && imageDialog->priv->bfwin->session->opendir)
		gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog),
												imageDialog->priv->bfwin->session->opendir);
	else if (seturi == NULL)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), g_get_home_dir());

	if (seturi)
		g_free(seturi);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *stringuri = NULL;

		gtk_widget_hide(GTK_WIDGET(dialog));

		stringuri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
		DEBUG_MSG("filebutton_clicked, stringuri=%s\n", stringuri);
		if (imageDialog->priv->fileuri) {
			g_object_unref(imageDialog->priv->fileuri);
			imageDialog->priv->fileuri = NULL;
		}

		imageDialog->priv->fileuri = g_file_new_for_uri(stringuri);
		image_dialog_check_is_image_file(imageDialog);

		g_free(stringuri);
	}

	gtk_widget_destroy(dialog);
}
#endif
static void
image_dialog_percent_toggled(GtkToggleButton * togglebutton, GtkSpinButton * spinbutton, gdouble origsize)
{
	if (gtk_toggle_button_get_active(togglebutton))
		gtk_spin_button_set_value(spinbutton, 100);
	else
		gtk_spin_button_set_value(spinbutton, origsize);
}

static void
image_dialog_height_percent_toggled(GtkToggleButton * togglebutton, BluefishImageDialog * dialog)
{
	image_dialog_percent_toggled(togglebutton,
			GTK_SPIN_BUTTON(dialog->priv->height), dialog->priv->origHeight);
}

static void
image_dialog_width_percent_toggled(GtkToggleButton * togglebutton, BluefishImageDialog * dialog)
{
	image_dialog_percent_toggled(togglebutton,
			GTK_SPIN_BUTTON(dialog->priv->width), dialog->priv->origWidth);
}

static void
image_dialog_use_transitional_toggled(GtkToggleButton * togglebutton, BluefishImageDialog * dialog)
{
	if (gtk_toggle_button_get_active(togglebutton)) {
		gtk_widget_set_sensitive(dialog->priv->transitionalVbox, TRUE);
	} else {
		gtk_widget_set_sensitive(dialog->priv->transitionalVbox, FALSE);
	}
}

static void
image_dialog_ok_clicked(BluefishImageDialog * dialog)
{
	gchar *strvalue = NULL;
	gint intvalue;
	GString *tag;
	GtkTextIter start, end;

	gtk_widget_hide(GTK_WIDGET(dialog));

	tag = g_string_new(cap("<IMG"));

	strvalue = gtk_editable_get_chars(GTK_EDITABLE(dialog->priv->source), 0, -1);
	if (strlen(strvalue)) {
		if (dialog->priv->filename)
			g_string_append_printf(tag, " %s=\"%s\"", cap("SRC"), dialog->priv->filename);
		else
			g_string_append_printf(tag, " %s=\"%s\"", cap("SRC"), strvalue);
	} else
		g_string_append_printf(tag, " %s=\"\"", cap("SRC"));
	g_free(strvalue);

	intvalue = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dialog->priv->width));
	if (intvalue != 0) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->priv->widthPercent)))
			g_string_append_printf(tag, " %s=\"%d%%\"", cap("WIDTH"), intvalue);
		else
			g_string_append_printf(tag, " %s=\"%d\"", cap("WIDTH"), intvalue);
	}

	intvalue = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dialog->priv->height));
	if (intvalue != 0) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->priv->heightPercent)))
			g_string_append_printf(tag, " %s=\"%d%%\"", cap("HEIGHT"), intvalue);
		else
			g_string_append_printf(tag, " %s=\"%d\"", cap("HEIGHT"), intvalue);
	}

	strvalue = gtk_editable_get_chars(GTK_EDITABLE(dialog->priv->alt), 0, -1);
	if (strlen(strvalue))
		g_string_append_printf(tag, " %s=\"%s\"", cap("ALT"), strvalue);
	else
		g_string_append_printf(tag, " %s=\"\"", cap("ALT"));
	g_free(strvalue);

	strvalue = gtk_editable_get_chars(GTK_EDITABLE(dialog->priv->longDesc), 0, -1);
	if (strlen(strvalue))
		g_string_append_printf(tag, " %s=\"%s\"", cap("LONGDESC"), strvalue);
	g_free(strvalue);

	strvalue = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dialog->priv->class));
	if (strlen(strvalue))
		g_string_append_printf(tag, " %s=\"%s\"", cap("CLASS"), strvalue);
	g_free(strvalue);

	strvalue = gtk_editable_get_chars(GTK_EDITABLE(dialog->priv->id), 0, -1);
	if (strlen(strvalue))
		g_string_append_printf(tag, " %s=\"%s\"", cap("ID"), strvalue);
	g_free(strvalue);

	strvalue = gtk_editable_get_chars(GTK_EDITABLE(dialog->priv->usemap), 0, -1);
	if (strlen(strvalue))
		g_string_append_printf(tag, " %s=\"%s\"", cap("USEMAP"), strvalue);
	g_free(strvalue);

	strvalue = gtk_editable_get_chars(GTK_EDITABLE(dialog->priv->style), 0, -1);
	if (strlen(strvalue))
		g_string_append_printf(tag, " %s=\"%s\"", cap("STYLE"), strvalue);
	g_free(strvalue);

	strvalue = gtk_editable_get_chars(GTK_EDITABLE(dialog->priv->custom), 0, -1);
	if (strlen(strvalue))
		g_string_append_printf(tag, " %s", strvalue);
	g_free(strvalue);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->priv->useTransitional))) {
		if (gtk_combo_box_get_active(GTK_COMBO_BOX(dialog->priv->align))) {
			strvalue = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dialog->priv->align));
			g_string_append_printf(tag, " %s=\"%s\"", cap("ALIGN"), strvalue);
			g_free(strvalue);
		}

		intvalue = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dialog->priv->border));
		if (intvalue >= 0)
			g_string_append_printf(tag, " %s=\"%d\"", cap("BORDER"), intvalue);

		intvalue = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dialog->priv->hspace));
		if (intvalue >= 0)
			g_string_append_printf(tag, " %s=\"%d\"", cap("HSPACE"), intvalue);

		intvalue = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dialog->priv->vspace));
		if (intvalue >= 0)
			g_string_append_printf(tag, " %s=\"%d\"", cap("VSPACE"), intvalue);
	}

	g_string_append_printf(tag, (main_v->props.xhtml == 1) ? " />" : ">");
	DEBUG_MSG("image_dialog_ok_clicked, tagStart=%d\n", dialog->priv->tagStart);
	if (dialog->priv->tagStart >= 0) {
		doc_replace_text(dialog->priv->doc, tag->str, dialog->priv->tagStart, dialog->priv->tagEnd);
	} else if (gtk_text_buffer_get_selection_bounds(dialog->priv->doc->buffer, &start, &end)) {
		gint soffset, eoffset;

		soffset = gtk_text_iter_get_offset(&start);
		eoffset = gtk_text_iter_get_offset(&end);

		doc_replace_text(dialog->priv->doc, tag->str, soffset, eoffset);
	} else
		doc_insert_two_strings(dialog->priv->doc, tag->str, NULL);

	g_string_free(tag, TRUE);
}

static void
image_dialog_response_lcb(GtkDialog * dialog, gint response, BluefishImageDialog * imageDialog)
{
	if (response == GTK_RESPONSE_OK) {
		image_dialog_ok_clicked(imageDialog);
	}

	gtk_widget_destroy(GTK_WIDGET(dialog));
}

void
bluefish_image_dialog_new(Tbfwin * bfwin)
{
	BluefishImageDialog *dialog;

	dialog = g_object_new(BLUEFISH_TYPE_IMAGE_DIALOG,
						  "bfwin", bfwin,
						  "destroy-with-parent", TRUE,
#if !GTK_CHECK_VERSION(3, 0, 0)
						  "has-separator", FALSE,
#endif
						  "title", _("Insert Image"),
						  "keep-aspect", TRUE,
						  "transient-for", bfwin->main_window,
						  "tag-start", -1, "tag-end", -1, NULL);
	DEBUG_MSG("bluefish_image_dialog_new, tag-start set to -1 in g_object_new\n");
	g_return_if_fail(dialog != NULL);
/*#ifdef DEBUG
	g_object_set(G_OBJECT(dialog), "tag-start", -1, NULL);
	gint tagstart;
	g_object_get(G_OBJECT(dialog), "tag-start", &tagstart, NULL);
	g_print("tag start actually is at %d\n",tagstart);
#endif
*/	gtk_widget_show_all(GTK_WIDGET(dialog));
}

void
bluefish_image_dialog_new_with_data(Tbfwin * bfwin, Ttagpopup * data)
{
	BluefishImageDialog *dialog;
	gboolean usetransitional = FALSE;
	gboolean widthispercent = FALSE;
	gboolean heightispercent = FALSE;
	gchar *custom = NULL;
	gchar *temp;
	gint align = 0;
	gdouble width = 0;
	gdouble height = 0;
	gdouble border = -1;
	gdouble hspace = -1;
	gdouble vspace = -1;

	gchar *tagitems[] = {
		"src",
		"width",
		"height",
		"alt",
		"longdesc",
		"class",
		"id",
		"usemap",
		"custom",
		"align",
		"border",
		"hspace",
		"vspace",
		"style",
		NULL
	};

	gchar *tagvalues[15];

	parse_html_for_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data);

	if (tagvalues[1]) {
		temp = strchr(tagvalues[1], '%');
		if (temp) {
			tagvalues[1] = trunc_on_char(tagvalues[1], '%');
			width = g_strtod(tagvalues[1], NULL);
			widthispercent = TRUE;
		} else
			width = g_strtod(tagvalues[1], NULL);
	}

	if (tagvalues[2]) {
		temp = strchr(tagvalues[2], '%');
		if (temp) {
			tagvalues[2] = trunc_on_char(tagvalues[2], '%');
			height = g_strtod(tagvalues[2], NULL);
			heightispercent = TRUE;
		} else
			height = g_strtod(tagvalues[2], NULL);
	}

	if (tagvalues[9]) {
		int i;

		const gchar *alignments[] = {
			"",
			"bottom",
			"left",
			"middle",
			"right",
			"top",
		};

		for (i = 0; i < G_N_ELEMENTS(alignments); i++) {
			if (strcmp(alignments[i], tagvalues[9]) == 0) {
				align = i;
				break;
			}
		}
	}

	if (tagvalues[10])
		border = g_strtod(tagvalues[10], NULL);

	if (tagvalues[11])
		hspace = g_strtod(tagvalues[11], NULL);

	if (tagvalues[12])
		vspace = g_strtod(tagvalues[12], NULL);

	if (tagvalues[9] || tagvalues[10] || tagvalues[11] || tagvalues[12])
		usetransitional = TRUE;
	DEBUG_MSG("bluefish_image_dialog_new_with_data, src=%s, tag-start=%d, tag-end=%d\n", tagvalues[0], data->pos, data->end);
	dialog = g_object_new(BLUEFISH_TYPE_IMAGE_DIALOG,
						  "bfwin", bfwin,
						  "destroy-with-parent", TRUE,
#if !GTK_CHECK_VERSION(3, 0, 0)
						  "has-separator", FALSE,
#endif
						  "title", _("Insert Image"),
						  "transient-for", bfwin->main_window,
						  "src", tagvalues[0],
						  "width", width,
						  "width-is-percent", widthispercent,
						  "height", height,
						  "height-is-percent", heightispercent,
						  "alt", tagvalues[3],
						  "longdesc", tagvalues[4],
						  "class", tagvalues[5],
						  "id", tagvalues[6],
						  "usemap", tagvalues[7],
						  "style", tagvalues[13],
						  "custom", custom,
						  "align", align,
						  "border", border,
						  "hspace", hspace,
						  "vspace", vspace,
						  "use-transitional", usetransitional,
						  "tag-start", data->pos, "tag-end", data->end, NULL);

	g_return_if_fail(dialog != NULL);

	gtk_widget_show_all(GTK_WIDGET(dialog));

	g_free(custom);
}
