/* Bluefish HTML Editor
 * image_dialog.c
 *
 * Copyright (C) 2008 James Hayward and Olivier Sessink
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


#include <string.h>

#include "image_dialog.h"
#include "cap.h"

#include "../dialog_utils.h"
#include "../document.h"
#include "../file.h"


#define BLUEFISH_IMAGE_DIALOG_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), BLUEFISH_TYPE_IMAGE_DIALOG, BluefishImageDialogPrivate))


struct _BluefishImageDialogPrivate
{
  GFile *fileuri;
	gchar *filename;
	
	gint origHeight;
	gint origWidth;
	
	GdkPixbufLoader *pbloader;
	
	GtkWidget *align;
	GtkWidget *alt;
	GtkWidget *border;
	GtkWidget *class;
	GtkWidget *dlg;
	GtkWidget *fileButton;
	GtkWidget *height;
	GtkWidget *heightPercent;
	GtkWidget *hspace;
	GtkWidget *id;
	GtkWidget *longDesc;
	GtkWidget *preview;
	GtkWidget *previewInfo;
	GtkWidget *previewPane;
	GtkWidget *resetSizeButton;
	GtkWidget *source;
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

enum
{
  PROP_0,
  PROP_BFWIN
};

G_DEFINE_TYPE(BluefishImageDialog, bluefish_image_dialog, GTK_TYPE_DIALOG)

/* static function prototypes */
static void
image_dialog_reset_dimensions (GtkButton *button,
															 BluefishImageDialog *dialog);

static void
image_dialog_source_changed (GtkWidget *widget,
														 BluefishImageDialog *dialog);

static void
filebutton_clicked (GtkButton *button,
										BluefishImageDialog *imageDialog);

static void
image_dialog_height_percent_toggled (GtkToggleButton *togglebutton,
																		 BluefishImageDialog *dialog);

static void
image_dialog_width_percent_toggled (GtkToggleButton *togglebutton,
																		BluefishImageDialog *dialog);

static void
image_dialog_use_transitional_toggled (GtkToggleButton *togglebutton,
																			 BluefishImageDialog *dialog);

static void 
image_dialog_response_lcb (GtkDialog *dialog,
													 gint response,
													 BluefishImageDialog *imageDialog);
/* end static function prototypes */


static void 
bluefish_image_dialog_finalize (GObject *object)
{
	BluefishImageDialog *dialog = BLUEFISH_IMAGE_DIALOG (object);
	
	if (dialog->priv->fileuri)
		g_object_unref (dialog->priv->fileuri);
		
	if (dialog->priv->filename)
		g_free (dialog->priv->filename);
	
  G_OBJECT_CLASS (bluefish_image_dialog_parent_class)->finalize (object);
}

static void
bluefish_image_dialog_get_property (GObject *object,
																		guint prop_id,
																		GValue *value,
																		GParamSpec *pspec)
{
  BluefishImageDialog *dialog = BLUEFISH_IMAGE_DIALOG (object);
  
  switch (prop_id)
  {
    case PROP_BFWIN:
      g_value_set_pointer (value, dialog->priv->bfwin);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
bluefish_image_dialog_set_property (GObject *object,
																		guint prop_id,
																		const GValue *value,
																		GParamSpec *pspec)
{
  BluefishImageDialog *dialog = BLUEFISH_IMAGE_DIALOG (object);

  switch (prop_id)
  {
    case PROP_BFWIN:
      dialog->priv->bfwin = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GObject *
bluefish_image_dialog_create (GType type,
															guint n_construct_properties,
															GObjectConstructParam *construct_properties)
{
  BluefishImageDialogClass *klass = BLUEFISH_IMAGE_DIALOG_CLASS (g_type_class_peek (BLUEFISH_TYPE_IMAGE_DIALOG));
  GObjectClass *parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
  GObject *obj = parent_class->constructor (type,
                                            n_construct_properties,
                                            construct_properties);
                                            
  BluefishImageDialog *dialog = BLUEFISH_IMAGE_DIALOG (obj);  

	GtkWidget *frame, *hbox, *label, *notebook, *table, *vbox;
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
	
	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_OK, GTK_RESPONSE_OK);
	
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
																		 GTK_RESPONSE_OK, FALSE);
	
	g_signal_connect (dialog, "response", G_CALLBACK (image_dialog_response_lcb), dialog);
	
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE, FALSE, 6);
	
	label = gtk_label_new_with_mnemonic (_("_Source:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	dialog->priv->source = gtk_entry_new ();
	gtk_box_pack_start_defaults (GTK_BOX (hbox), dialog->priv->source);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), dialog->priv->source);
	g_signal_connect (dialog->priv->source, "changed", G_CALLBACK (image_dialog_source_changed), dialog);
	
	dialog->priv->fileButton = dialog_button_new_with_image (NULL,
																													 -1, GTK_STOCK_OPEN,
																													 GTK_ICON_SIZE_MENU);
	gtk_button_set_focus_on_click (GTK_BUTTON (dialog->priv->fileButton), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), dialog->priv->fileButton, FALSE, FALSE, 0);
	g_signal_connect (dialog->priv->fileButton, "clicked", G_CALLBACK (filebutton_clicked), dialog);
	
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 6);
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	
	frame = gtk_frame_new (_("Preview"));
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
	dialog->priv->previewPane = gtk_vbox_new (FALSE, 6);
	gtk_container_add (GTK_CONTAINER (frame), dialog->priv->previewPane);
	
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
	
	notebook = gtk_notebook_new ();
	gtk_notebook_popup_disable (GTK_NOTEBOOK (notebook));
	gtk_box_pack_start (GTK_BOX (vbox), notebook, FALSE, FALSE, 6);
	
	vbox = gtk_vbox_new (FALSE, 6);	
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	
	table = dialog_table_in_vbox (3, 3, 6, vbox, FALSE, FALSE, 0);
	
	dialog->priv->width = gtk_spin_button_new_with_range (0, 3000, 1);
	gtk_table_attach (GTK_TABLE (table), dialog->priv->width, 1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	dialog_mnemonic_label_in_table(_("_Width:"), dialog->priv->width, table, 0, 1, 0, 1);
	
	dialog->priv->widthPercent = gtk_check_button_new_with_label ("%");
	gtk_table_attach (GTK_TABLE (table), dialog->priv->widthPercent, 2, 3, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	g_signal_connect (dialog->priv->widthPercent, "toggled",
										G_CALLBACK (image_dialog_width_percent_toggled), dialog);
	
	dialog->priv->height = gtk_spin_button_new_with_range (0, 3000, 1);
	gtk_table_attach (GTK_TABLE (table), dialog->priv->height, 1, 2, 1, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);
	dialog_mnemonic_label_in_table(_("_Height:"), dialog->priv->height, table, 0, 1, 1, 2);
	
	dialog->priv->heightPercent = gtk_check_button_new_with_label ("%");
	gtk_table_attach (GTK_TABLE (table), dialog->priv->heightPercent, 2, 3, 1, 2, GTK_SHRINK, GTK_SHRINK, 0, 0);
	g_signal_connect (dialog->priv->heightPercent, "toggled",
										G_CALLBACK (image_dialog_height_percent_toggled), dialog);
	
	dialog->priv->resetSizeButton = dialog_button_new_with_image_in_table(_("_Reset Dimensions"),
																		 -1, GTK_STOCK_REFRESH,
																		 GTK_ICON_SIZE_MENU,
																		 table, 0, 3, 2, 3);
	gtk_widget_set_sensitive (dialog->priv->resetSizeButton, FALSE);																	 
	g_signal_connect (dialog->priv->resetSizeButton, "clicked", G_CALLBACK (image_dialog_reset_dimensions), dialog);																		 
	
	table = dialog_table_in_vbox(5, 3, 6, vbox, TRUE, TRUE, 0);
	/*
	dialog->priv->source = gtk_entry_new ();
	gtk_table_attach (GTK_TABLE (table), dialog->priv->source, 1, 2, 0, 1, GTK_EXPAND|GTK_FILL, GTK_SHRINK, 0, 0);
	dialog_mnemonic_label_in_table(_("_Source:"), dialog->priv->source, table, 0, 1, 0, 1);

	gtk_table_set_col_spacing (GTK_TABLE (table), 1, 6);
	dialog->priv->fileButton = dialog_button_new_with_image_in_table(NULL,
																	-1, GTK_STOCK_OPEN,
																	GTK_ICON_SIZE_MENU,
																	table, 2, 3, 0, 1);
	gtk_button_set_focus_on_click(GTK_BUTTON (dialog->priv->fileButton), FALSE);
	g_signal_connect (dialog->priv->fileButton, "clicked", G_CALLBACK (filebutton_clicked), dialog);
	*/
	dialog->priv->alt = gtk_entry_new ();
	gtk_table_attach (GTK_TABLE (table), dialog->priv->alt, 1, 3, 0, 1, GTK_EXPAND|GTK_FILL, GTK_SHRINK, 0, 0);
	dialog_mnemonic_label_in_table(_("Alternate te_xt:"), dialog->priv->alt, table, 0, 1, 0, 1);
	
	dialog->priv->longDesc = gtk_entry_new ();
	gtk_table_attach (GTK_TABLE (table), dialog->priv->longDesc, 1, 3, 1, 2, GTK_EXPAND|GTK_FILL, GTK_SHRINK, 0, 0);
	dialog_mnemonic_label_in_table(_("_Long description:"), dialog->priv->longDesc, table, 0, 1, 1, 2);
	
	dialog->priv->class = gtk_combo_box_entry_new_text ();
	gtk_table_attach (GTK_TABLE (table), dialog->priv->class, 1, 3, 2, 3, GTK_EXPAND|GTK_FILL, GTK_SHRINK, 0, 0);
	dialog_mnemonic_label_in_table(_("Cl_ass:"), dialog->priv->class, table, 0, 1, 2, 3);
	
	dialog->priv->id = gtk_entry_new ();
	gtk_table_attach (GTK_TABLE (table), dialog->priv->id, 1, 3, 3, 4, GTK_EXPAND|GTK_FILL, GTK_SHRINK, 0, 0);
	dialog_mnemonic_label_in_table(_("I_D:"), dialog->priv->id, table, 0, 1, 3, 4);
	
	dialog->priv->usemap = gtk_entry_new ();
	gtk_table_attach (GTK_TABLE (table), dialog->priv->usemap, 1, 3, 4, 5, GTK_EXPAND|GTK_FILL, GTK_SHRINK, 0, 0);
	dialog_mnemonic_label_in_table(_("_Usemap:"), dialog->priv->usemap, table, 0, 1, 4, 5);
	
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, gtk_label_new (_("Strict")));
	
	vbox = gtk_vbox_new (FALSE, 6);	
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	
	dialog->priv->useTransitional = gtk_check_button_new ();
	dialog->priv->transitionalVbox = dialog_vbox_labeled_checkbutton(_("_Use Transitional options"), dialog->priv->useTransitional, vbox);
	g_signal_connect (GTK_TOGGLE_BUTTON (dialog->priv->useTransitional), "toggled", G_CALLBACK (image_dialog_use_transitional_toggled), dialog);
	
	table = dialog_table_in_vbox(4, 2, 6, dialog->priv->transitionalVbox, FALSE, FALSE, 0);
	
	dialog->priv->align = gtk_combo_box_new_text ();
	for (i = 0; i < G_N_ELEMENTS (alignments); i++) {
		gtk_combo_box_append_text (GTK_COMBO_BOX (dialog->priv->align), alignments[i]);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->priv->align), 0);
	dialog_mnemonic_label_in_table(_("_Align:"), dialog->priv->align, table, 0, 1, 0, 1);
	gtk_table_attach (GTK_TABLE (table), dialog->priv->align, 1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
	
	dialog->priv->border = gtk_spin_button_new_with_range (-1, 500, 1);
	gtk_table_attach (GTK_TABLE (table), dialog->priv->border, 1, 2, 1, 2, GTK_FILL, GTK_SHRINK, 0, 0);
	dialog_mnemonic_label_in_table(_("_Border:"), dialog->priv->border, table, 0, 1, 1, 2);
	
	dialog->priv->hspace = gtk_spin_button_new_with_range (-1, 500, 1);
	gtk_table_attach (GTK_TABLE (table), dialog->priv->hspace, 1, 2, 2, 3, GTK_FILL, GTK_SHRINK, 0, 0);
	dialog_mnemonic_label_in_table(_("_Hspace:"), dialog->priv->hspace, table, 0, 1, 2, 3);
	
	dialog->priv->vspace = gtk_spin_button_new_with_range (-1, 500, 1);
	gtk_table_attach (GTK_TABLE (table), dialog->priv->vspace, 1, 2, 3, 4, GTK_FILL, GTK_SHRINK, 0, 0);
	dialog_mnemonic_label_in_table(_("_Vspace:"), dialog->priv->vspace, table, 0, 1, 3, 4);
	
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, gtk_label_new (_("Transitional")));
	
	image_dialog_use_transitional_toggled (GTK_TOGGLE_BUTTON (dialog->priv->useTransitional), dialog);
	
  return (obj);
}

static void
bluefish_image_dialog_class_init (BluefishImageDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor = bluefish_image_dialog_create;
  object_class->finalize = bluefish_image_dialog_finalize;
	
  object_class->get_property = bluefish_image_dialog_get_property;
  object_class->set_property = bluefish_image_dialog_set_property;

  g_type_class_add_private (object_class, sizeof(BluefishImageDialogPrivate));
  
  g_object_class_install_property (object_class,
																	 PROP_BFWIN,
																	 g_param_spec_pointer ("bfwin",
																												 "BFwin",
																												 "The image dialogs bfwin",
																												 G_PARAM_READWRITE |
																												 G_PARAM_CONSTRUCT_ONLY |
																												 G_PARAM_STATIC_STRINGS));
}

static void 
bluefish_image_dialog_init (BluefishImageDialog *dialog)
{
  dialog->priv = BLUEFISH_IMAGE_DIALOG_GET_PRIVATE (dialog);
  g_return_if_fail (dialog->priv != NULL);
}

static void
pbloader_size_prepared (GdkPixbufLoader *pbloader,
												gint width,
												gint height,
												BluefishImageDialog *dialog)
{
	dialog->priv->origWidth = width;
	dialog->priv->origHeight = height;
	
	if (width > 256) {
		gfloat scale = width/256.0;
		gdk_pixbuf_loader_set_size (pbloader, width/scale, height/scale);	
	}
}

static GdkPixbufLoader *
pbloader_get_for_mime_type (const gchar *mimetype)
{
	GdkPixbufLoader *pbloader = NULL;
	
	if (mimetype) {
		GError *error = NULL;
		
		pbloader = gdk_pixbuf_loader_new_with_mime_type (mimetype, &error);
		
		if (error) {
			g_message ("%s", error->message);
			g_error_free (error);	
		}
	}
	
	if (!pbloader) {
		pbloader = gdk_pixbuf_loader_new ();	
	}
	
	return pbloader;
}

static void
image_dialog_reset_dimensions (GtkButton *button,
															 BluefishImageDialog *dialog)
{
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->priv->width), dialog->priv->origWidth);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->priv->height), dialog->priv->origHeight);
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->widthPercent)))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->widthPercent), FALSE);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->heightPercent)))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->heightPercent), FALSE);
}

static void
image_dialog_set_source (BluefishImageDialog *dialog)
{
	gchar *relpath = NULL;
	
	if (dialog->priv->doc->uri != NULL) {
		GFile *parent;
		
		parent = g_file_get_parent (dialog->priv->doc->uri);				
		relpath = g_file_get_relative_path (parent, dialog->priv->fileuri);
		g_object_unref (parent);		
	}
		
	if (relpath)
		dialog->priv->filename = relpath;
	else
		dialog->priv->filename = g_file_get_parse_name (dialog->priv->fileuri);
	
	g_signal_handlers_block_matched (dialog->priv->source,
																	 G_SIGNAL_MATCH_FUNC,
																	 0, 0, NULL,
																	 image_dialog_source_changed,
																	 NULL);
	
	gtk_entry_set_text (GTK_ENTRY (dialog->priv->source), dialog->priv->filename);
	
	g_signal_handlers_unblock_matched (dialog->priv->source,
																		 G_SIGNAL_MATCH_FUNC,
																		 0, 0, NULL,
																		 image_dialog_source_changed,
																		 NULL);
}

static void
image_dialog_preview_loaded (BluefishImageDialog *dialog)
{
	GFileInfo *fileinfo;
	GError *error = NULL;
		
	gtk_box_pack_start (GTK_BOX (dialog->priv->previewPane), dialog->priv->preview, FALSE, FALSE, 6);
	gtk_widget_show (dialog->priv->preview);
	
	fileinfo = g_file_query_info (dialog->priv->fileuri,
																"standard::display-name,standard::size",
																G_FILE_QUERY_INFO_NONE,
																NULL,
																&error);
	
	if (error == NULL) {
		GString *previewInfo;
		
		previewInfo = g_string_new (g_file_info_get_display_name (fileinfo));
		
		if (g_file_info_has_attribute (fileinfo, G_FILE_ATTRIBUTE_STANDARD_SIZE)) {
			gchar *filesize;
			goffset size;
			
			size = g_file_info_get_size (fileinfo);
			filesize = g_format_size_for_display (size);
																						
			g_string_append_printf (previewInfo, _("\n%s"), filesize);
			
			g_free (filesize);
		}
		
		g_string_append_printf (previewInfo, _("\n%d x %d pixels"),
														dialog->priv->origWidth,
														dialog->priv->origHeight);
		
		dialog->priv->previewInfo = gtk_label_new (previewInfo->str);
		gtk_label_set_use_markup (GTK_LABEL (dialog->priv->previewInfo), TRUE);
		gtk_label_set_justify (GTK_LABEL (dialog->priv->previewInfo), GTK_JUSTIFY_CENTER);
		gtk_box_pack_start (GTK_BOX (dialog->priv->previewPane), dialog->priv->previewInfo, FALSE, FALSE, 0);
		gtk_widget_show (dialog->priv->previewInfo);
		
		g_string_free (previewInfo, TRUE);
	}
	
	g_object_unref (fileinfo);
	
	image_dialog_reset_dimensions (NULL, dialog);
	gtk_widget_set_sensitive (dialog->priv->resetSizeButton, TRUE);
	image_dialog_set_source (dialog);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_OK, TRUE);
}

static void
image_dialog_load_preview (Topenfile_status status, 
													 gint error_info,
													 gchar *buffer,
													 goffset buflen,
													 gpointer callback_data)
{
	BluefishImageDialog *imageDialog = callback_data;
	gboolean cleanup = TRUE;
	
	switch (status) {
		case OPENFILE_ERROR:
		case OPENFILE_ERROR_NOCHANNEL:
		case OPENFILE_ERROR_NOREAD:
/*			gtk_label_set_text(GTK_LABEL(imdg->message), _("Loading image failed..."));*/
		break;
		case OPENFILE_ERROR_CANCELLED:
			/* should we warn the user ?? */
			gdk_pixbuf_loader_close (imageDialog->priv->pbloader, NULL);
		break;
		case OPENFILE_CHANNEL_OPENED:
			cleanup = FALSE;
		break;
		case OPENFILE_FINISHED: {
			GdkPixbuf *pixbuf;
			GError *error= NULL;
			
			if (gdk_pixbuf_loader_write (imageDialog->priv->pbloader,(guchar *) buffer, buflen, &error) 
					&& gdk_pixbuf_loader_close (imageDialog->priv->pbloader, &error)) {
						
				pixbuf = gdk_pixbuf_loader_get_pixbuf (imageDialog->priv->pbloader);
				if (pixbuf) {
					imageDialog->priv->preview = gtk_image_new_from_pixbuf (pixbuf);
					image_dialog_preview_loaded (imageDialog);
				}
			}
		}
		break;
	}
	
	if (cleanup) {
		g_object_unref (imageDialog->priv->pbloader);
		imageDialog->priv->pbloader = NULL;
		imageDialog->priv->openfile = NULL;
	}
}

static void
image_dialog_remove_preview (BluefishImageDialog *dialog)
{
	if (dialog->priv->preview) {
		gtk_widget_destroy (dialog->priv->preview);
		dialog->priv->preview = NULL;
	}

	if (dialog->priv->previewInfo) {
		gtk_widget_destroy (dialog->priv->previewInfo);
		dialog->priv->previewInfo = NULL;	
	}

	if (dialog->priv->openfile) {
		openfile_cancel(dialog->priv->openfile);
	}
	
	if (dialog->priv->filename) {
		g_free (dialog->priv->filename);
		dialog->priv->filename = NULL;
	}	
 
	dialog->priv->origHeight = 0;
	dialog->priv->origWidth = 0;	
	image_dialog_reset_dimensions (NULL, dialog);
	gtk_widget_set_sensitive (dialog->priv->resetSizeButton, FALSE);
}

static void
image_dialog_set_preview (BluefishImageDialog *dialog)
{
	GFileInfo *fileinfo;
	GError *error = NULL;
	gchar *mimetype = NULL;

	if (dialog->priv->preview)
		image_dialog_remove_preview (dialog);
	
	fileinfo = g_file_query_info (dialog->priv->fileuri,
																G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
																G_FILE_QUERY_INFO_NONE,
																NULL,
																&error);
		
	if (error == NULL) {
		if (g_file_info_has_attribute (fileinfo, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE)) {
			const gchar *contenttype = g_file_info_get_content_type (fileinfo);
			mimetype = g_content_type_get_mime_type (contenttype);
		}
	}

	g_object_unref (fileinfo);
		
	dialog->priv->pbloader = pbloader_get_for_mime_type (mimetype);
		
	g_free (mimetype);
		
	g_signal_connect (dialog->priv->pbloader, "size-prepared",
										G_CALLBACK (pbloader_size_prepared), dialog);
		
	dialog->priv->openfile = file_openfile_uri_async (dialog->priv->fileuri, image_dialog_load_preview, dialog);
}

static void
image_dialog_source_changed (GtkWidget *widget,
														 BluefishImageDialog *dialog)
{
	const gchar *filename = NULL;
	gchar *tmp;
	
	if (dialog->priv->preview)
		image_dialog_remove_preview (dialog);
	
	if (dialog->priv->fileuri) {
			g_object_unref (dialog->priv->fileuri);
			dialog->priv->fileuri = NULL;
	}
	
  filename = gtk_entry_get_text (GTK_ENTRY (dialog->priv->source));
  
	if (filename && strlen (filename)) {
		tmp = strstr (filename, "://");
		if ((tmp == NULL && filename[0] != '/') && dialog->priv->doc->uri) {
			GFile *parent = g_file_get_parent (dialog->priv->doc->uri);
			dialog->priv->fileuri = g_file_resolve_relative_path (parent, filename);
			g_object_unref (parent);
		} else if (tmp != NULL)
				dialog->priv->fileuri = g_file_new_for_uri (filename);
			else if	(filename[0] == '/')
				dialog->priv->fileuri = g_file_new_for_path (filename);
			else
				return;

		if (dialog->priv->fileuri && g_file_query_exists (dialog->priv->fileuri, NULL)) {
			GFileInfo *fileinfo;
			GError *error = NULL;

			fileinfo = g_file_query_info (dialog->priv->fileuri,
																		G_FILE_ATTRIBUTE_STANDARD_TYPE,
																		G_FILE_QUERY_INFO_NONE,
																		NULL,
																		&error);

			if (error == NULL) {
				if (g_file_info_has_attribute (fileinfo, G_FILE_ATTRIBUTE_STANDARD_TYPE)) {
					if (g_file_info_get_file_type (fileinfo) != G_FILE_TYPE_DIRECTORY)
						image_dialog_set_preview (dialog);
				}
			}

			g_object_unref (fileinfo);
		}
	}
}

static void
filebutton_clicked (GtkButton *button,
										BluefishImageDialog *imageDialog)
{
	GtkWidget *dialog;
	GtkFileFilter *filefilter;
	gchar *seturi = NULL;
	
	dialog = gtk_file_chooser_dialog_new (_("Select Image"),
										  GTK_WINDOW (imageDialog),
										  GTK_FILE_CHOOSER_ACTION_OPEN,
										  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
										  GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
										  NULL);
	
	filefilter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filefilter, _("images"));
	gtk_file_filter_add_pixbuf_formats (filefilter);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filefilter);
	
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), FALSE);

	if (imageDialog->priv->fileuri) {
		seturi = g_file_get_uri (imageDialog->priv->fileuri);
		gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (dialog), seturi);
	}
	else if (imageDialog->priv->doc->uri) {
		GFile *parent = g_file_get_parent (imageDialog->priv->doc->uri);
		seturi = g_file_get_uri (parent);
		g_object_unref (parent);
		
		if (seturi)
			gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dialog), seturi);
	}
	
	if (seturi)
		g_free (seturi);
	
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{	
		gchar *stringuri = NULL;
			
		gtk_widget_hide (GTK_WIDGET (dialog));
	
		stringuri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
		
		if (imageDialog->priv->fileuri) {
			g_object_unref (imageDialog->priv->fileuri);
			imageDialog->priv->fileuri = NULL;
		}

		imageDialog->priv->fileuri = g_file_new_for_uri (stringuri);
		image_dialog_set_preview (imageDialog);
		
		g_free (stringuri);
	}
	
	gtk_widget_destroy (dialog);
}

static void
image_dialog_percent_toggled (GtkToggleButton *togglebutton,
															GtkSpinButton *spinbutton,
															gdouble origsize)
{
	if (gtk_toggle_button_get_active (togglebutton))
		gtk_spin_button_set_value (spinbutton, 100);
	else
		gtk_spin_button_set_value (spinbutton, origsize);
}

static void
image_dialog_height_percent_toggled (GtkToggleButton *togglebutton,
																		 BluefishImageDialog *dialog)
{
	image_dialog_percent_toggled (togglebutton,
																GTK_SPIN_BUTTON (dialog->priv->height),
																dialog->priv->origHeight);
}

static void
image_dialog_width_percent_toggled (GtkToggleButton *togglebutton,
																		BluefishImageDialog *dialog)
{
	image_dialog_percent_toggled (togglebutton,
																GTK_SPIN_BUTTON (dialog->priv->width),
																dialog->priv->origWidth);
}

static void
image_dialog_use_transitional_toggled (GtkToggleButton *togglebutton,
																			 BluefishImageDialog *dialog)
{
	if (gtk_toggle_button_get_active (togglebutton)) {
		gtk_widget_set_sensitive (dialog->priv->transitionalVbox, TRUE);
	} else {
		gtk_widget_set_sensitive (dialog->priv->transitionalVbox, FALSE);	
	}
}

static void
image_dialog_ok_clicked (BluefishImageDialog *dialog)
{
	gchar *strvalue = NULL;
	gint intvalue;
	GString *tag;
	GtkTextIter start, end;
	
	gtk_widget_hide (GTK_WIDGET (dialog));
	
	tag = g_string_new (cap ("<IMG"));
  g_string_append_printf (tag, " %s=\"%s\"", cap ("SRC"),
													dialog->priv->filename);

	intvalue = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->width));
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->widthPercent)))
		g_string_append_printf (tag, " %s=\"%d%%\"", cap ("WIDTH"), intvalue);
	else
		g_string_append_printf (tag, " %s=\"%d\"", cap ("WIDTH"), intvalue);
	
	intvalue = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->height));
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->heightPercent)))
		g_string_append_printf (tag, " %s=\"%d%%\"", cap ("HEIGHT"), intvalue);
	else
		g_string_append_printf (tag, " %s=\"%d\"", cap ("HEIGHT"), intvalue);
	
	strvalue = gtk_editable_get_chars (GTK_EDITABLE (dialog->priv->alt), 0, -1);
	if (strlen (strvalue))
		g_string_append_printf (tag, " %s=\"%s\"", cap ("ALT"), strvalue);
	g_free (strvalue);
	
	strvalue = gtk_editable_get_chars (GTK_EDITABLE (dialog->priv->longDesc), 0, -1);
	if (strlen (strvalue))
		g_string_append_printf (tag, " %s=\"%s\"", cap ("LONGDESC"), strvalue);
	g_free (strvalue);
	
	strvalue = gtk_combo_box_get_active_text (GTK_COMBO_BOX (dialog->priv->class));
	if (strlen (strvalue))
		g_string_append_printf (tag, " %s=\"%s\"", cap ("CLASS"), strvalue);
	g_free (strvalue);
	
	strvalue = gtk_editable_get_chars (GTK_EDITABLE (dialog->priv->id), 0, -1);
	if (strlen (strvalue))
		g_string_append_printf (tag, " %s=\"%s\"", cap ("ID"), strvalue);
	g_free (strvalue);
	
	strvalue = gtk_editable_get_chars (GTK_EDITABLE (dialog->priv->usemap), 0, -1);
	if (strlen (strvalue))
		g_string_append_printf (tag, " %s=\"%s\"", cap ("USEMAP"), strvalue);
	g_free (strvalue);
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->useTransitional)))
	{
		if (gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->priv->align)))
		{
			strvalue = gtk_combo_box_get_active_text (GTK_COMBO_BOX (dialog->priv->align));
			g_string_append_printf (tag, " %s=\"%s\"", cap ("ALIGN"), strvalue);
			g_free (strvalue);
		}

		intvalue = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->border));
		if (intvalue >= 0)
			g_string_append_printf (tag, " %s=\"%d\"", cap ("BORDER"), intvalue);
		
		intvalue = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->hspace));
		if (intvalue >= 0)
			g_string_append_printf (tag, " %s=\"%d\"", cap ("HSPACE"), intvalue);
		
		intvalue = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->priv->vspace));
		if (intvalue >= 0)
			g_string_append_printf (tag, " %s=\"%d\"", cap ("VSPACE"), intvalue);
	}
	
  g_string_append_printf (tag, (main_v->props.xhtml == 1) ? " />" : ">");
  
	if (gtk_text_buffer_get_selection_bounds (dialog->priv->doc->buffer, &start, &end))
	{
		gint soffset, eoffset;
		
		soffset = gtk_text_iter_get_offset (&start);
		eoffset = gtk_text_iter_get_offset (&end);
		
		doc_replace_text (dialog->priv->doc, tag->str, soffset, eoffset);
	} 
	else
		doc_insert_two_strings (dialog->priv->doc, tag->str, NULL);
	
	g_string_free (tag, TRUE);
}

static void 
image_dialog_response_lcb (GtkDialog *dialog,
													 gint response,
													 BluefishImageDialog *imageDialog)
{
	if (response == GTK_RESPONSE_OK) {
		image_dialog_ok_clicked (imageDialog);
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

void
bluefish_image_dialog_new (Tbfwin *bfwin)
{
	BluefishImageDialog *dialog;
  
	dialog = g_object_new (BLUEFISH_TYPE_IMAGE_DIALOG,
												 "bfwin", bfwin,
												 "destroy-with-parent", TRUE,
												 "has-separator", FALSE,
												 "title", _("Insert Image"),
												 "transient-for", bfwin->main_window,
												 NULL);
	
	g_return_if_fail (dialog != NULL);
	
	gtk_widget_show_all (GTK_WIDGET (dialog));
}
