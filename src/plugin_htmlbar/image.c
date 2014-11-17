/* Bluefish HTML Editor
 * image.c - the thumbnail/multi-thumbnail dialogs
 *
 * Copyright (C) 2003-2012 Olivier Sessink
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
/* indented with indent -psl -ts4 -kr -l110   */

/*#define DEBUG */

#include <string.h>

#include "image.h"
#include "cap.h"
#include "htmlbar.h"
#include "html_diag.h"
#include "../bf_lib.h"
#include "../dialog_utils.h"
#include "../document.h"
#include "../file.h"
#include "../gtk_easy.h"
#include "../stringlist.h"

static GdkPixbufLoader *
pbloader_from_filename(const gchar * filename)
{
	GdkPixbufLoader *pbloader;
	GError *error = NULL;
	gchar *ext = strrchr(filename, '.');
	if (ext) {
		gchar *tmp2 = g_utf8_strdown(ext + 1, -1);
		if (strcmp(tmp2, "jpg") == 0) {
			pbloader = gdk_pixbuf_loader_new_with_type("jpeg", &error);
		} else {
			pbloader = gdk_pixbuf_loader_new_with_type(tmp2, &error);
		}
		if (error) {
			pbloader = gdk_pixbuf_loader_new();	/* try to guess from the data */
			g_error_free(error);
		}
		g_free(tmp2);
	} else {
		pbloader = gdk_pixbuf_loader_new();
	}
	return pbloader;
}

static gchar *
create_thumbnail_filename(gchar * filename)
{
	gchar *retval, *tmp;
	gint len = 0, size;
	tmp = strrchr(filename, '.');
	if (tmp) {
		len = strlen(tmp);
	}
	size =
		strlen(filename) - len + strlen(main_v->props.image_thumbnailstring) +
		strlen(main_v->props.image_thumbnailtype) + 2;
	retval = g_malloc0(size * sizeof(gchar));
	DEBUG_MSG("create_thumbnail_filename, size=%d bytes at %p\n", size, retval);
	retval = strncpy(retval, filename, strlen(filename) - len);
	retval = strcat(retval, main_v->props.image_thumbnailstring);
	retval = strcat(retval, ".");
	retval = strcat(retval, main_v->props.image_thumbnailtype);

	return retval;
}

typedef struct {
	Thtml_diag *dg;
	GtkWidget *message;
	GtkWidget *frame;
	GdkPixbuf *pb;
	GtkWidget *im;
	GFile *full_uri;

	GdkPixbufLoader *pbloader;
	Topenfile *of;

	/* and some options for the thumbnails */
	GtkAdjustment *adjustment;
	guint adj_changed_id;
} Timage_diag;

void
image_diag_destroy_cb(GtkWidget * widget, Timage_diag * imdg)
{
	html_diag_destroy_cb(widget, imdg->dg);
	if (imdg->pb) {
		g_object_unref(imdg->pb);
	}
	if (imdg->full_uri) {
		g_object_unref(imdg->full_uri);
	}
	g_free(imdg);
}

static TcheckNsave_return
async_thumbsave_lcb(TcheckNsave_status status, GError * gerror, gpointer callback_data)
{
	DEBUG_MSG("async_thumbsave_lcb, status=%d\n", status);
	/* TODO: handle error */
	if (gerror) {
		g_print("failed to save thumbnail: %s\n",gerror->message);
	}
	return CHECKNSAVE_CONT;
}

static void
image_insert_dialogok_lcb(GtkWidget * widget, Timage_diag * imdg)
{
	gchar *thestring, *finalstring;
	gchar *thumbnailfilename, *filename;
	gchar *tmp1, *tmp2;
	gchar *buffer;
	gsize buflen;
	gint w, h;
	GError *error = NULL;
	GdkPixbuf *tmp_im;
	GFile *fullthumbfilename;

	filename = gtk_editable_get_chars(GTK_EDITABLE(imdg->dg->entry[0]), 0, -1);
	if (strlen(filename)) {
		thumbnailfilename = create_thumbnail_filename(filename);
		/* we should use the full path to create the thumbnail filename */

		tmp1 = g_file_get_uri(imdg->full_uri);
		tmp2 = create_thumbnail_filename(tmp1);
		fullthumbfilename = g_file_new_for_uri(tmp2);

		g_free(tmp1);
		g_free(tmp2);

#ifdef DEBUG
		gchar *path = g_file_get_path(fullthumbfilename);
		DEBUG_MSG("image_insert_dialogok_lcb, thumbnail will be stored at %s\n", path);
		g_free(path);
#endif
		w = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(imdg->dg->spin[0]));
		h = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(imdg->dg->spin[1]));
		tmp_im = gdk_pixbuf_scale_simple(imdg->pb, w, h, GDK_INTERP_BILINEAR);
		
		if (strcmp(main_v->props.image_thumbnailtype, "jpeg") == 0) {
			gdk_pixbuf_save_to_buffer(tmp_im, &buffer, &buflen, main_v->props.image_thumbnailtype, &error,
									  "quality", "50", NULL);
			/* gdk_pixbuf_save(tmp_im,fullthumbfilename,main_v->props.image_thumbnailtype,&error, "quality", "50",NULL); */
		} else {
			gdk_pixbuf_save_to_buffer(tmp_im, &buffer, &buflen, main_v->props.image_thumbnailtype, &error,
									  NULL);
			/*gdk_pixbuf_save(tmp_im,fullthumbfilename,main_v->props.image_thumbnailtype,&error, NULL); */
		}
		g_object_unref(tmp_im);
		if (error) {
			g_print("ERROR while saving thumbnail to buffer: %s\n", error->message);
			g_error_free(error);
		} else {
			GError *error = NULL;
			GFileInfo *finfo;
			Trefcpointer *refbuf = refcpointer_new(buffer);

			finfo = g_file_query_info(fullthumbfilename, BF_FILEINFO, G_FILE_QUERY_INFO_NONE, NULL, &error);
			if (error != NULL) {
				g_print("image_insert_dialogok_lcb: %s\n ", error->message);
				g_error_free(error);
			}
#ifdef DEBUG
			gchar *path = g_file_get_path(fullthumbfilename);
			DEBUG_MSG("image_insert_dialogok_lcb, starting async save to %s\n", path);
			g_free(path);
#endif

			file_checkNsave_uri_async(fullthumbfilename, finfo, refbuf, buflen, FALSE, FALSE,
									  (CheckNsaveAsyncCallback) async_thumbsave_lcb, NULL);
			refcpointer_unref(refbuf);
		}

		g_object_unref(fullthumbfilename);

		thestring =
			g_strconcat(cap("<A HREF=\""), filename, cap("\"><IMG SRC=\""), thumbnailfilename, "\"", NULL);
		g_free(filename);
		g_free(thumbnailfilename);

		thestring =
			insert_integer_if_spin(imdg->dg->spin[0], cap("WIDTH"), thestring,
								   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(imdg->dg->check[0])), 0);
		thestring =
			insert_integer_if_spin(imdg->dg->spin[1], cap("HEIGHT"), thestring,
								   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(imdg->dg->check[1])), 0);
		if (!get_curlang_option_value(imdg->dg->bfwin, lang_is_XHTML)) {
			thestring = insert_if_spin(imdg->dg->spin[2], cap("BORDER"), thestring, FALSE);
		}
		thestring = insert_if_spin(imdg->dg->spin[3], cap("HSPACE"), thestring, FALSE);
		thestring = insert_if_spin(imdg->dg->spin[4], cap("VSPACE"), thestring, FALSE);
		thestring = insert_string_if_entry(GTK_ENTRY(imdg->dg->entry[1]), cap("NAME"), thestring, NULL);
		thestring = insert_string_if_entry(GTK_ENTRY(imdg->dg->entry[2]), cap("ALT"), thestring, "");
		thestring = insert_string_if_entry(GTK_ENTRY(imdg->dg->entry[3]), cap("USEMAP"), thestring, NULL);
		thestring =
			insert_string_if_combobox(GTK_COMBO_BOX(imdg->dg->combo[0]), cap("ALIGN"), thestring, NULL);
		thestring = insert_string_if_entry(GTK_ENTRY(imdg->dg->entry[4]), NULL, thestring, NULL);

		finalstring = g_strconcat(thestring, get_curlang_option_value(imdg->dg->bfwin, self_close_singleton_tags) ? " />" : ">", NULL);
		g_free(thestring);

		if (imdg->dg->range.end == -1) {
			doc_insert_two_strings(imdg->dg->doc, finalstring, cap("</a>"));
		} else {
			doc_replace_text(imdg->dg->doc, finalstring, imdg->dg->range.pos, imdg->dg->range.end);
		}

		g_free(finalstring);
	}
	image_diag_destroy_cb(NULL, imdg);
}

void
image_diag_cancel_clicked_cb(GtkWidget * widget, gpointer data)
{
	image_diag_destroy_cb(NULL, data);
}

static void
image_diag_finish(Timage_diag * imdg, GCallback ok_func)
{
	GtkWidget *align, *hbox;

#if GTK_CHECK_VERSION(3,0,0)
	hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
	hbox = gtk_hbutton_box_new();
#endif
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(hbox), 6);

	imdg->dg->obut = bf_stock_ok_button(ok_func, imdg);
	imdg->dg->cbut = bf_stock_cancel_button(G_CALLBACK(image_diag_cancel_clicked_cb), imdg);

	gtk_box_pack_start(GTK_BOX(hbox), imdg->dg->cbut, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), imdg->dg->obut, FALSE, FALSE, 0);
	gtk_window_set_default(GTK_WINDOW(imdg->dg->dialog), imdg->dg->obut);

	align = gtk_alignment_new(0, 0, 1, 0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align), 12, 0, 0, 0);
	gtk_container_add(GTK_CONTAINER(align), hbox);
	gtk_box_pack_start(GTK_BOX(imdg->dg->vbox), align, FALSE, FALSE, 0);
	gtk_widget_show_all(GTK_WIDGET(imdg->dg->dialog));
}

static void
image_dialog_set_pixbuf(Timage_diag * imdg)
{
	gfloat toobig, pb_width, pd_height;
	GdkPixbuf *tmp_pb;
	if (!imdg->pb) {
		return;
	}
	
	pb_width = (gfloat)gdk_pixbuf_get_width(imdg->pb);
	pd_height = (gfloat)gdk_pixbuf_get_height(imdg->pb);
	if (imdg->dg->bfwin) {
		Thtmlbarsession *hbs;
		hbs = g_hash_table_lookup(htmlbar_v.lookup, imdg->dg->bfwin->session);
		toobig = pb_width / ((gfloat)hbs->thumbnailwidth);
		DEBUG_MSG("initialize toobig as %f, using session thumbnailwidth %d and pixbuf width %f\n",toobig,hbs->thumbnailwidth,pb_width);
	} else {
		toobig = 1.0;
		if ((pb_width / 250.0) > toobig) {
			toobig = pb_width / 250.0;
		}
		
		if ((pd_height / 300.0) > toobig) {
			toobig = pd_height / 300.0;
		}
	}
	/* because the spin buttons in the html dialogs are designed that they can have an empty string value ""
	they will not accept a number as long as there is an empty string in them */
	gtk_entry_set_text(GTK_ENTRY(imdg->dg->spin[0]), "1");
	gtk_entry_set_text(GTK_ENTRY(imdg->dg->spin[1]), "1");

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(imdg->dg->spin[0]), (pb_width / toobig));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(imdg->dg->spin[1]), (pd_height / toobig));
	g_signal_handler_block(G_OBJECT(imdg->adjustment), imdg->adj_changed_id);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(imdg->adjustment), 1.0 / toobig);
	g_signal_handler_unblock(G_OBJECT(imdg->adjustment), imdg->adj_changed_id);

	tmp_pb =
		gdk_pixbuf_scale_simple(imdg->pb, (gint)(pb_width / toobig), (gint)(pd_height / toobig),
								main_v->globses.image_thumbnail_refresh_quality ? GDK_INTERP_BILINEAR :
								GDK_INTERP_NEAREST);

	if (GTK_IS_WIDGET(imdg->im)) {
		DEBUG_MSG("imdg->im == %p\n", imdg->im);
		DEBUG_MSG("gtk_widget_destroy() %p\n", imdg->im);
		gtk_widget_destroy(imdg->im);
	}

	imdg->im = gtk_image_new_from_pixbuf(tmp_pb);

	g_object_unref(tmp_pb);
	/*gtk_container_remove(GTK_CONTAINER(imdg->frame),imdg->message); */
	gtk_widget_destroy(imdg->message);
	imdg->message = NULL;

	gtk_container_add(GTK_CONTAINER(imdg->frame), imdg->im);
	gtk_widget_show(imdg->im);
	DEBUG_MSG("imdg->im == %p\n", imdg->im);
	DEBUG_MSG("image_filename_changed() finished. GTK_IS_WIDGET(imdg->im) == %d\n", GTK_IS_WIDGET(imdg->im));
}

static void
image_loaded_lcb(Topenfile_status status, GError * gerror, Trefcpointer * refp, goffset buflen,
				 gpointer callback_data)
{
	Timage_diag *imdg = callback_data;
	gboolean cleanup = TRUE;
	switch (status) {
	case OPENFILE_ERROR:
	case OPENFILE_ERROR_NOCHANNEL:
	case OPENFILE_ERROR_NOREAD:
		/* TODO: use error info in gerror */
		gtk_label_set_text(GTK_LABEL(imdg->message), _("Loading image failed..."));
		break;
	case OPENFILE_ERROR_CANCELLED:
		/* should we warn the user ?? */
		gdk_pixbuf_loader_close(imdg->pbloader, NULL);
		break;
	case OPENFILE_CHANNEL_OPENED:
		/* do nothing */
		cleanup = FALSE;
		break;
	case OPENFILE_FINISHED:{
			GError *error = NULL;
			if (gdk_pixbuf_loader_write(imdg->pbloader, (const guchar *) refp->data, buflen, &error)
				&& gdk_pixbuf_loader_close(imdg->pbloader, &error)) {
				imdg->pb = gdk_pixbuf_loader_get_pixbuf(imdg->pbloader);
				if (imdg->pb) {
					g_object_ref(imdg->pb);
					image_dialog_set_pixbuf(imdg);
				}
			}
		}
		break;
	}
	if (cleanup) {
		g_object_unref(imdg->pbloader);
		imdg->pbloader = NULL;
		imdg->of = NULL;
	}
}

static void
image_filename_changed(GtkWidget * widget, Timage_diag * imdg)
{
	const gchar *filename;
	gchar *tmp;
	GFile *fullfilename = NULL;

	DEBUG_MSG("image_filename_changed() started. GTK_IS_WIDGET(imdg->im) == %d\n", GTK_IS_WIDGET(imdg->im));
	if (imdg->pb) {
		g_object_unref(imdg->pb);
	}
	if (imdg->of) {
		openfile_cancel(imdg->of);
	}
	DEBUG_MSG("image_filename_changed: filename=%s\n", gtk_entry_get_text(GTK_ENTRY(imdg->dg->entry[0])));

	/* the entry usually has a relative filename, so we should make it absolute 
	   using the basedir of the document */
	filename = gtk_entry_get_text(GTK_ENTRY(imdg->dg->entry[0]));
	/* we should use the full path to create the thumbnail filename */
	tmp = strstr(filename, "://");
	if ((tmp == NULL && filename[0] != '/') && imdg->dg->doc->uri) {
		/* a relative path. create the absolute path. */
		GFile *parent = g_file_get_parent(imdg->dg->doc->uri);
		gchar *tmp;
		/* filename is an URI, not a file path. the function g_file_resolve_relative_path
		   does not handle URI parts like %20 (a space) */
		tmp = g_uri_unescape_string(filename, NULL);
		DEBUG_MSG("unescaped filename=%s\n", tmp);
		fullfilename = g_file_resolve_relative_path(parent, tmp);
		g_free(tmp);
		g_object_unref(parent);
	} else if (tmp != NULL || filename[0] == '/') {
		fullfilename = g_file_new_for_uri(filename);
	} else {
		return;
	}

	if (fullfilename && g_file_query_exists(fullfilename, NULL)) {
		gchar *name, *msg;
		gchar *path = g_file_get_path(fullfilename);
		DEBUG_MSG("path for fullfilename=%s\n", path);
		imdg->pbloader = pbloader_from_filename(path);
		g_free(path);

#ifdef DEBUG
		gchar *path2 = g_file_get_path(fullfilename);
		DEBUG_MSG("image_filename_changed: fullfilename=%s, loading!\n", path2);
		g_free(path2);
#endif
		imdg->of = file_openfile_uri_async(fullfilename, NULL, image_loaded_lcb, imdg);
		imdg->full_uri = fullfilename;
		name = g_file_get_uri(fullfilename);
		msg = g_strdup_printf(_("Loading file %s..."), name);
		if (imdg->message) {
			gtk_widget_destroy(imdg->message);
		}
		imdg->message = gtk_label_new(msg);
		gtk_container_add(GTK_CONTAINER(imdg->frame), imdg->message);
		gtk_widget_show(imdg->message);
		g_free(msg);
		g_free(name);
	}
}

static void
image_adjust_changed(GtkAdjustment * adj, Timage_diag * imdg)
{
	GdkPixbuf *tmp_pb;
	gint tn_width, tn_height;
	DEBUG_MSG("image_adjust_changed started. GTK_IS_WIDGET(imdg->im) == %d\n", GTK_IS_WIDGET(imdg->im));
	if (!imdg->pb) {
		image_filename_changed(NULL, imdg);
		return;
	}

	tn_width = gtk_adjustment_get_value(imdg->adjustment) * gdk_pixbuf_get_width(imdg->pb);
	tn_height = gtk_adjustment_get_value(imdg->adjustment) * gdk_pixbuf_get_height(imdg->pb);
	DEBUG_MSG("image_adjust_changed, width=%d, height=%d\n",tn_width,tn_height);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(imdg->dg->spin[0]), tn_width);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(imdg->dg->spin[1]), tn_height);

	/* TODO: move this to an idle callback so we will not block the UI */ 
	tmp_pb =
		gdk_pixbuf_scale_simple(imdg->pb, tn_width, tn_height,
								main_v->globses.image_thumbnail_refresh_quality ? GDK_INTERP_BILINEAR :
								GDK_INTERP_NEAREST);

	if (GTK_IS_WIDGET(imdg->im)) {
		DEBUG_MSG("imdg->im == %p\n", imdg->im);
		DEBUG_MSG("gtk_widget_destroy() %p\n", imdg->im);
		gtk_widget_destroy(imdg->im);
	}

	imdg->im = gtk_image_new_from_pixbuf(tmp_pb);
	g_object_unref(tmp_pb);
	gtk_container_add(GTK_CONTAINER(imdg->frame), imdg->im);
	gtk_widget_show(imdg->im);
	DEBUG_MSG("image_adjust_changed finished. GTK_IS_WIDGET(imdg->im) == %d\n", GTK_IS_WIDGET(imdg->im));
}

void
image_insert_dialog_backend(gchar * filename, Tbfwin * bfwin, Ttagpopup * data)
{
	static gchar *tagitems[] =
		{ "width", "height", "alt", "border", "src", "hspace", "vspace", "align", "name", "usemap", NULL };
	gchar *tagvalues[11];
	gchar *custom = NULL, *tmp;
	Timage_diag *imdg;
	GList *popuplist = NULL;
	GtkWidget *dgtable, *scale;

	imdg = g_new0(Timage_diag, 1);

	tmp = main_v->props.image_thumbnailtype; /* it is often uppercase, we need it lowercase */
	main_v->props.image_thumbnailtype = g_ascii_strdown(tmp, -1);
	g_free(tmp);

	imdg->dg = html_diag_new(bfwin, _("Insert thumbnail"));

	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, imdg->dg);

	imdg->frame = gtk_frame_new(_("Preview"));
	imdg->message = NULL;

	gtk_box_pack_start(GTK_BOX(imdg->dg->vbox), imdg->frame, TRUE, TRUE, 0);

	imdg->adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.5, 0.0001, 1.1, 0.001, 0.1, 0.1));
#if GTK_CHECK_VERSION(3,0,0)
	scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, imdg->adjustment);
#else
	scale = gtk_hscale_new(imdg->adjustment);
#endif
	imdg->adj_changed_id =
		g_signal_connect(G_OBJECT(imdg->adjustment), "value_changed", G_CALLBACK(image_adjust_changed), imdg);
	gtk_scale_set_digits(GTK_SCALE(scale), 3);
	gtk_box_pack_start(GTK_BOX(imdg->dg->vbox), scale, FALSE, FALSE, 0);

	dgtable = html_diag_table_in_vbox(imdg->dg, 5, 9);

	if (filename) {
		imdg->dg->entry[0] = dialog_entry_in_table(filename, dgtable, 1, 7, 0, 1);
	} else {
		imdg->dg->entry[0] = dialog_entry_in_table(tagvalues[4], dgtable, 1, 7, 0, 1);
	}
	dialog_mnemonic_label_in_table(_("_Image location:"), imdg->dg->entry[0], dgtable, 0, 1, 0, 1);

	gtk_table_attach_defaults(GTK_TABLE(dgtable), file_but_new(imdg->dg->entry[0], 0, bfwin), 7, 9, 0, 1);
	g_signal_connect(G_OBJECT(imdg->dg->entry[0]), "changed", G_CALLBACK(image_filename_changed), imdg);

	imdg->dg->spin[0] = spinbut_with_value(NULL, 0, 5000, 1.0, 10.0);
	imdg->dg->check[0] = gtk_check_button_new_with_label("%");
	parse_integer_for_dialog(tagvalues[0], imdg->dg->spin[0], NULL, imdg->dg->check[0]);
	dialog_mnemonic_label_in_table(_("_Width:"), imdg->dg->spin[0], dgtable, 6, 7, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->spin[0], 7, 8, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->check[0], 8, 9, 1, 2);

	imdg->dg->spin[1] = spinbut_with_value(NULL, 0, 5000, 1.0, 10.0);
	imdg->dg->check[1] = gtk_check_button_new_with_label("%");
	parse_integer_for_dialog(tagvalues[1], imdg->dg->spin[1], NULL, imdg->dg->check[1]);
	dialog_mnemonic_label_in_table(_("Hei_ght:"), imdg->dg->spin[1], dgtable, 6, 7, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->spin[1], 7, 8, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->check[1], 8, 9, 2, 3);

	imdg->dg->entry[3] = dialog_entry_in_table(tagvalues[9], dgtable, 1, 3, 1, 2);
	dialog_mnemonic_label_in_table(_("_Usemap:"), imdg->dg->entry[3], dgtable, 0, 1, 1, 2);

	imdg->dg->entry[1] = dialog_entry_in_table(tagvalues[8], dgtable, 1, 3, 2, 3);
	dialog_mnemonic_label_in_table(_("_Name:"), imdg->dg->entry[1], dgtable, 0, 1, 2, 3);

	imdg->dg->entry[2] = dialog_entry_in_table(tagvalues[2], dgtable, 1, 6, 3, 4);
	dialog_mnemonic_label_in_table(_("Alternate _text:"), imdg->dg->entry[2], dgtable, 0, 1, 3, 4);

	imdg->dg->entry[4] = dialog_entry_in_table(custom, dgtable, 1, 6, 4, 5);
	dialog_mnemonic_label_in_table(_("Custo_m:"), imdg->dg->entry[4], dgtable, 0, 1, 4, 5);

	imdg->dg->spin[3] = spinbut_with_value(tagvalues[5], 0, 500, 1.0, 5.0);
	dialog_mnemonic_label_in_table(_("_Hspace:"), imdg->dg->spin[3], dgtable, 6, 7, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->spin[3], 7, 9, 3, 4);

	imdg->dg->spin[4] = spinbut_with_value(tagvalues[6], 0, 500, 1.0, 5.0);
	dialog_mnemonic_label_in_table(_("_Vspace:"), imdg->dg->spin[4], dgtable, 6, 7, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->spin[4], 7, 9, 4, 5);

	popuplist = g_list_append(NULL, "bottom");
	popuplist = g_list_append(popuplist, "middle");
	popuplist = g_list_append(popuplist, "top");
	popuplist = g_list_append(popuplist, "left");
	popuplist = g_list_append(popuplist, "right");
	imdg->dg->combo[0] = html_diag_combobox_with_popdown_sized(tagvalues[7], popuplist, 1, 90);
	g_list_free(popuplist);
	dialog_mnemonic_label_in_table(_("_Align:"), imdg->dg->combo[0], dgtable, 3, 4, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->combo[0], 4, 6, 1, 2);
	if (!get_curlang_option_value(imdg->dg->bfwin, lang_is_XHTML)) {
		imdg->dg->spin[2] = spinbut_with_value(tagvalues[3], 0, 500, 1.0, 5.0);
		dialog_mnemonic_label_in_table(_("Borde_r:"), imdg->dg->spin[2], dgtable, 3, 4, 2, 3);
		gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->spin[2], 4, 6, 2, 3);
	}
	if (filename || tagvalues[4]) {
		g_signal_emit_by_name(G_OBJECT(imdg->dg->entry[0]), "changed");
	}

	image_diag_finish(imdg, G_CALLBACK(image_insert_dialogok_lcb));

	if (custom)
		g_free(custom);
}

void
thumbnail_insert_dialog(Tbfwin * bfwin)
{
	image_insert_dialog_backend(NULL, bfwin, NULL);
}

/* 
to get the multi thumbnail dialog to work with asynchronous backend is a little complex:

1) start loading images asynchronous, to avoid heavy memory usage, we'll limit the number 
of simultaneous image loads to 3 or so
2) each image that is loaded, we check is there are images not loading yet, and we'll fire 
the next load, we also immediately create the thumbnail and dispose the original pixbuf to 
free that memory. we will write the HTML string only if all previous files have written 
their html string in the document, if previous files have not finished we do nothing. If
we can write the html string, we also check if the next image is already finished, and 
should write its html string. If there is no next image, we can call for cleanup
*/

typedef struct {
	GtkWidget *win;
	GtkWidget *radio[4];
	GtkWidget *spinlabels[2];
	GtkWidget *spins[2];
	GtkTextBuffer *tbuf;
	gint mode;
	GList *images;
	Tbfwin *bfwin;
	Tdocument *document;
} Tmuthudia;

typedef struct {
	GFile *imagename;
	GFile *thumbname;
	Topenfile *of;				/* if != NULL, the image is loading */
	gpointer sf;				/* if != NULL, the thumbnail is saving */
	gboolean created;			/* both loading and saving is finished */
	gchar *string;				/* the string to insert, if NULL && ->create = TRUE means 
								   that the string is written to the document */
	Tmuthudia *mtd;
} Timage2thumb;

static void
mt_dialog_destroy(GtkWidget * wid, Tmuthudia * mtd)
{
	/* check if we have some images still loading, all images that have 'created == TRUE'
	   are ready */
	GList *tmplist;
	for (tmplist = g_list_first(mtd->images); tmplist; tmplist = g_list_next(tmplist)) {
		Timage2thumb *tmp = tmplist->data;
		if (tmp->created == FALSE) {
			return;
		}
	}
	for (tmplist = g_list_first(mtd->images); tmplist; tmplist = g_list_next(tmplist)) {
		Timage2thumb *tmp = tmplist->data;
		g_object_unref(tmp->imagename);
		g_object_unref(tmp->thumbname);
		g_free(tmp);
	}
	DEBUG_MSG("multi_thumbnail_dialog_destroy, called for mtd=%p\n", mtd);
	window_destroy(mtd->win);
	g_free(mtd);
}

/* needs both pixbufs to get the width !! */
static void
mt_fill_string(Timage2thumb * i2t, GdkPixbuf * image, GdkPixbuf * thumb)
{
	gint tw, th, ow, oh;
	gchar *relthumb, *tmp, *relimage;
	gchar *doc_curi = NULL;

	relimage = tmp = g_file_get_uri(i2t->imagename);

	if (i2t->mtd->document->uri) {
		doc_curi = g_file_get_uri(i2t->mtd->document->uri);
	}

	if (i2t->mtd->document->uri) {
		relimage = create_relative_link_to(doc_curi, tmp);
		g_free(tmp);
	}

	relthumb = tmp = g_file_get_uri(i2t->thumbname);

	if (i2t->mtd->bfwin->current_document->uri) {
		relthumb = create_relative_link_to(doc_curi, tmp);
		g_free(tmp);
	}
	if (doc_curi)
		g_free(doc_curi);

	ow = gdk_pixbuf_get_width(image);
	oh = gdk_pixbuf_get_height(image);
	tw = gdk_pixbuf_get_width(thumb);
	th = gdk_pixbuf_get_height(thumb);
	{
		Tconvert_table *table, *tmpt;

		table = tmpt = g_new(Tconvert_table, 8);
		tmpt->my_int = 'r';
		tmpt->my_char = g_strdup(relimage);
		tmpt++;
		tmpt->my_int = 't';
		tmpt->my_char = g_strdup(relthumb);
		tmpt++;
		tmpt->my_int = 'w';
		tmpt->my_char = g_strdup_printf("%d", ow);
		tmpt++;
		tmpt->my_int = 'h';
		tmpt->my_char = g_strdup_printf("%d", oh);
		tmpt++;
		tmpt->my_int = 'x';
		tmpt->my_char = g_strdup_printf("%d", tw);
		tmpt++;
		tmpt->my_int = 'y';
		tmpt->my_char = g_strdup_printf("%d", th);
		tmpt++;
		tmpt->my_int = 'b';
		tmpt->my_char = g_strdup("xxx");
		tmpt++;
		tmpt->my_char = NULL;
		i2t->string = replace_string_printflike(main_v->globses.image_thumnailformatstring, table);
		DEBUG_MSG("string to insert: %s\n", i2t->string);
		tmpt = table;
		while (tmpt->my_char) {
			g_free(tmpt->my_char);
			tmpt++;
		}
		g_free(table);
	}
	g_free(relimage);
	g_free(relthumb);
}

static Timage2thumb *
mt_next(Timage2thumb * i2t)
{
	GList *tmplist;
	tmplist = g_list_find(i2t->mtd->images, i2t);
	tmplist = g_list_next(tmplist);
	return (tmplist) ? tmplist->data : NULL;
}

static Timage2thumb *
mt_prev(Timage2thumb * i2t)
{
	GList *tmplist;
	tmplist = g_list_find(i2t->mtd->images, i2t);
	tmplist = g_list_previous(tmplist);
	return (tmplist) ? tmplist->data : NULL;
}

/* TRUE if already inserted or successfully inserted, FALSE if not yet ready */
static gboolean
mt_print_string(Timage2thumb * i2t)
{
	if (i2t->string == NULL && i2t->created == TRUE) {
		/* already added the HTML string */
		return TRUE;
	} else if (i2t->string) {
		Timage2thumb *tmp;
		/* check the previous entry */
		tmp = mt_prev(i2t);
		if (tmp && !mt_print_string(tmp)) {
			return FALSE;
		}
		/* we have a string, but it is not yet added, insert the string */
		doc_insert_two_strings(i2t->mtd->document, i2t->string, NULL);
		g_free(i2t->string);
		i2t->string = NULL;
		i2t->created = TRUE;
		/* now check if the next string is also already ready for printing */
		tmp = mt_next(i2t);
		if (tmp)
			mt_print_string(tmp);
		return TRUE;
	} else {
		return FALSE;
	}
}

static void mt_start_load(Timage2thumb * i2t);

static gboolean
mt_start_next_load(Timage2thumb * i2t)
{
	GList *tmplist;
	for (tmplist = g_list_first(i2t->mtd->images); tmplist; tmplist = g_list_next(tmplist)) {
		Timage2thumb *tmp = tmplist->data;
		if (tmp->of == NULL && tmp->string == NULL && tmp->created == FALSE) {
			mt_start_load(tmp);
			return TRUE;
		}
	}
	return FALSE;
}

static void
mt_openfile_lcb(Topenfile_status status, GError * gerror, Trefcpointer *refp, goffset buflen,
				gpointer callback_data)
{
	Timage2thumb *i2t = callback_data;
	switch (status) {
	case OPENFILE_ERROR:
	case OPENFILE_ERROR_NOCHANNEL:
	case OPENFILE_ERROR_NOREAD:
	case OPENFILE_ERROR_CANCELLED:{
			/* TODO: should we warn the user ?? */
#ifdef DEBUG
			{
				gchar *path = g_file_get_path(i2t->imagename);
				DEBUG_MSG("mt_openfile_lcb, some error! status=%d for image %s\n", status, path);
				g_free(path);
			}
#endif
		}
		break;
	case OPENFILE_CHANNEL_OPENED:
		/* do nothing */
		break;
	case OPENFILE_FINISHED:{
			GError *error = NULL;
			gboolean nextload;
			GdkPixbufLoader *pbloader;
#ifdef DEBUG
			{
				gchar *path = g_file_get_path(i2t->imagename);
				DEBUG_MSG("mt_openfile_lcb, finished loading image %s\n", path);
				g_free(path);
			}
#endif
			nextload = mt_start_next_load(i2t);	/* fire up the next image load */

			gchar *path = g_file_get_path(i2t->imagename);
			pbloader = pbloader_from_filename(path);
			g_free(path);

			if (gdk_pixbuf_loader_write(pbloader, (const guchar *) refp->data, buflen, &error)
				&& gdk_pixbuf_loader_close(pbloader, &error)) {
				gint tw, th, ow, oh;
				GdkPixbuf *image;
				GdkPixbuf *thumb;
				gsize buflen;
				image = gdk_pixbuf_loader_get_pixbuf(pbloader);
				if (image) {
					ow = gdk_pixbuf_get_width(image);
					oh = gdk_pixbuf_get_height(image);
					switch (main_v->globses.image_thumbnailsizing_type) {
					case 0:
						tw = (1.0 * ow / 100 * main_v->globses.image_thumbnailsizing_val1);
						th = (1.0 * oh / 100 * main_v->globses.image_thumbnailsizing_val1);
						break;
					case 1:
						tw = main_v->globses.image_thumbnailsizing_val1;
						th = (1.0 * tw / ow * oh);
						break;
					case 2:
						th = main_v->globses.image_thumbnailsizing_val1;
						tw = (1.0 * th / oh * ow);
						break;
					default:	/* all fall back to type 3 */
						tw = main_v->globses.image_thumbnailsizing_val1;
						th = main_v->globses.image_thumbnailsizing_val2;
						break;
					}
#ifdef DEBUG
					{
						gchar *path = g_file_get_path(i2t->imagename);
						DEBUG_MSG("mt_openfile_lcb, start scaling %s to %dx%d\n", path, tw, th);
						g_free(path);
					}
#endif
					thumb = gdk_pixbuf_scale_simple(image, tw, th, GDK_INTERP_BILINEAR);
#ifdef DEBUG
					gchar *path = g_file_get_path(i2t->imagename);
					DEBUG_MSG("mt_openfile_lcb, done scaling %s\n", path);
					g_free(path);
#endif
					mt_fill_string(i2t, image, thumb);	/* create the string */
					mt_print_string(i2t);	/* print the string and all previous string (if possible) */
					g_object_unref(pbloader);
					/*gdk_pixbuf_unref(image); will be unreffed with the loader! */
					/* save the thumbnail */
					if (strcmp(main_v->props.image_thumbnailtype, "jpeg") == 0) {
						gdk_pixbuf_save_to_buffer(thumb, (gchar **)&refp->data, &buflen, main_v->props.image_thumbnailtype,
												  &error, "quality", "50", NULL);
					} else {
						gdk_pixbuf_save_to_buffer(thumb, (gchar **)&refp->data, &buflen, main_v->props.image_thumbnailtype,
												  &error, NULL);
					}
					g_object_unref(thumb);
					if (error) {
						g_print("ERROR while saving thumbnail to buffer: %s\n", error->message);
						g_error_free(error);
					} else {
						GError *error = NULL;
						GFileInfo *finfo;
						refcpointer_ref(refp);
						finfo = g_file_query_info(i2t->thumbname,
												  BF_FILEINFO, G_FILE_QUERY_INFO_NONE, NULL, &error);
						if (error != NULL) {
							g_print("mt_openfile_lcb %s\n ", error->message);
							g_error_free(error);
						}
#ifdef DEBUG
						gchar *path = g_file_get_path(i2t->thumbname);
						DEBUG_MSG("mt_openfile_lcb, starting thumbnail save to %s\n", path);
						g_free(path);
#endif
						i2t->sf =
							file_checkNsave_uri_async(i2t->thumbname, finfo, refp, buflen, FALSE, FALSE,
													  async_thumbsave_lcb, NULL);
						refcpointer_unref(refp);
					}
				} else {
					/* ok, this image is not valid, how do we continue ?? */
#ifdef DEBUG
					gchar *path = g_file_get_path(i2t->imagename);
					DEBUG_MSG("mt_openfile_lcb, failed to convert %s to image\n", path);
					g_free(path);
#endif
					i2t->string = g_strdup("");
					mt_print_string(i2t);
				}
				if (!nextload) {
					/* there were no more images to load, perhaps we could already call cleanup */
					mt_dialog_destroy(NULL, i2t->mtd);
				}
			}
		}
		break;
	}
	/* BUG: the last image that reaches this function should free 'mtd' after it is finished */
}

static void
mt_start_load(Timage2thumb * i2t)
{
#ifdef DEBUG
	gchar *path = g_file_get_path(i2t->imagename);
	DEBUG_MSG("mt_start_load, starting load for %s\n", path);
	g_free(path);
#endif
	i2t->of = file_openfile_uri_async(i2t->imagename, NULL, mt_openfile_lcb, i2t);
}

static Timage2thumb *
mt_image2thumbnail(Tmuthudia * mtd, gchar * curi)
{
	Timage2thumb *i2t;
	gchar *tmp;
	GFile *uri;

	DEBUG_MSG("mt_image2thumbnail, called for %s\n", curi);
	if (!curi)
		return NULL;
	uri = g_file_new_for_uri(curi);

	if (!uri)
		return NULL;
	i2t = g_new0(Timage2thumb, 1);
	i2t->mtd = mtd;
	i2t->imagename = uri;
	tmp = create_thumbnail_filename(curi);
	i2t->thumbname = g_file_new_for_uri(tmp);
	g_free(tmp);
	return i2t;
}

static void
multi_thumbnail_ok_clicked(GtkWidget * widget, Tmuthudia * mtd)
{
	GSList *files = NULL, *tmplist;
	GtkWidget *dialog;
	gint i;

	gtk_widget_hide(mtd->win);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mtd->radio[0]))) {
		main_v->globses.image_thumbnailsizing_type = 0;
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mtd->radio[1]))) {
		main_v->globses.image_thumbnailsizing_type = 1;
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mtd->radio[2]))) {
		main_v->globses.image_thumbnailsizing_type = 2;
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mtd->radio[3]))) {
		main_v->globses.image_thumbnailsizing_type = 3;
	}
	{
		gchar *tmp;
		GtkTextIter start, end;
		gtk_text_buffer_get_bounds(mtd->tbuf, &start, &end);
		tmp = gtk_text_buffer_get_text(mtd->tbuf, &start, &end, FALSE);
		if (tmp) {
			if (main_v->globses.image_thumnailformatstring)
				g_free(main_v->globses.image_thumnailformatstring);
			main_v->globses.image_thumnailformatstring = tmp;
		}
	}

	dialog =
		file_chooser_dialog(mtd->bfwin, _("Select files for thumbnail creation"),
							GTK_FILE_CHOOSER_ACTION_OPEN, NULL, FALSE, TRUE, "webimage", FALSE);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		files = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));
	}
	gtk_widget_destroy(dialog);

	main_v->globses.image_thumbnailsizing_val1 =
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mtd->spins[0]));
	main_v->globses.image_thumbnailsizing_val2 =
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mtd->spins[1]));

	i = 3;
	tmplist = files;
	while (tmplist) {
		Timage2thumb *i2t;
		i2t = mt_image2thumbnail(mtd, (gchar *) tmplist->data);
		mtd->images = g_list_prepend(mtd->images, i2t);
		if (i > 0) {
			mt_start_load(i2t);
			i--;
		}
		tmplist = g_slist_next(tmplist);
	}
	mtd->images = g_list_reverse(mtd->images);
	/* BUG: should we free the list of files now ?? */
}

static void
multi_thumbnail_cancel_clicked(GtkWidget * widget, Tmuthudia * mtd)
{
	mt_dialog_destroy(NULL, mtd);
}

static void
multi_thumbnail_radio_toggled_lcb(GtkToggleButton * togglebutton, Tmuthudia * mtd)
{
	/* only call this for activate, not for de-activate */
	if (gtk_toggle_button_get_active(togglebutton)) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mtd->radio[0]))) {
			gtk_widget_hide(mtd->spins[1]);
			gtk_widget_hide(mtd->spinlabels[1]);
			gtk_label_set_text(GTK_LABEL(mtd->spinlabels[0]), _("Scaling (%)"));
		} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mtd->radio[1]))) {
			gtk_widget_hide(mtd->spins[1]);
			gtk_widget_hide(mtd->spinlabels[1]);
			gtk_label_set_text(GTK_LABEL(mtd->spinlabels[0]), _("Width"));
		} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mtd->radio[2]))) {
			gtk_widget_hide(mtd->spins[1]);
			gtk_widget_hide(mtd->spinlabels[1]);
			gtk_label_set_text(GTK_LABEL(mtd->spinlabels[0]), _("Height"));
		} else {
			gtk_widget_show(mtd->spins[1]);
			gtk_widget_show(mtd->spinlabels[1]);
			gtk_label_set_text(GTK_LABEL(mtd->spinlabels[0]), _("Width"));
		}
	}
}

void
multi_thumbnail_dialog(Tbfwin * bfwin)
{
	Tmuthudia *mtd;
	GtkWidget *vbox, *hbox, *but, *table, *label, *scrolwin, *textview;
	gint tb;
	gchar *tmp;

	if (!bfwin->current_document) {
		return;
	}

	tmp = main_v->props.image_thumbnailtype; /* it is often uppercase, we need it lowercase */
	main_v->props.image_thumbnailtype = g_ascii_strdown(tmp, -1);
	g_free(tmp);

	mtd = g_new0(Tmuthudia, 1);
	mtd->bfwin = bfwin;
	mtd->document = bfwin->current_document;
	mtd->win =
		window_full2(_("Multi thumbnail"), GTK_WIN_POS_CENTER, 5, G_CALLBACK(mt_dialog_destroy), mtd, TRUE,
					 bfwin->main_window);
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(mtd->win), vbox);

	table = gtk_table_new(4, 3, FALSE);
	mtd->radio[0] = gtk_radio_button_new_with_label(NULL, _("By scaling"));
	mtd->radio[1] =
		gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(mtd->radio[0]),
													_("By width, keep aspect ratio"));
	mtd->radio[2] =
		gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(mtd->radio[0]),
													_("By height, keep aspect ratio"));
	mtd->radio[3] =
		gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(mtd->radio[0]),
													_("By width and height, ignore aspect ratio"));
	mtd->spinlabels[0] = gtk_label_new("");
	mtd->spinlabels[1] = gtk_label_new(_("Height"));
	mtd->spins[0] = gtk_spin_button_new_with_range(0, 1000, 1);
	mtd->spins[1] = gtk_spin_button_new_with_range(0, 1000, 1);

	g_signal_connect(G_OBJECT(mtd->radio[0]), "toggled", G_CALLBACK(multi_thumbnail_radio_toggled_lcb), mtd);
	g_signal_connect(G_OBJECT(mtd->radio[1]), "toggled", G_CALLBACK(multi_thumbnail_radio_toggled_lcb), mtd);
	g_signal_connect(G_OBJECT(mtd->radio[2]), "toggled", G_CALLBACK(multi_thumbnail_radio_toggled_lcb), mtd);
	g_signal_connect(G_OBJECT(mtd->radio[3]), "toggled", G_CALLBACK(multi_thumbnail_radio_toggled_lcb), mtd);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(mtd->spins[0]), main_v->globses.image_thumbnailsizing_val1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(mtd->spins[1]), main_v->globses.image_thumbnailsizing_val2);

	gtk_table_attach_defaults(GTK_TABLE(table), mtd->radio[0], 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->radio[1], 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->radio[2], 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->radio[3], 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->spinlabels[0], 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->spinlabels[1], 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->spins[0], 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->spins[1], 2, 3, 1, 2);

	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	label =
		gtk_label_new(_
					  ("%r: original filename  %t: thumbnail filename\n%w: original width  %h: original height\n%x: thumbnail width  %y: thumbnail height\n%b: original size (bytes)"));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	scrolwin =
		textview_buffer_in_scrolwin(&textview, -1, -1, main_v->globses.image_thumnailformatstring,
									GTK_WRAP_CHAR);
	mtd->tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	gtk_box_pack_start(GTK_BOX(vbox), scrolwin, TRUE, TRUE, 0);
#if GTK_CHECK_VERSION(3,0,0)
	hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
	hbox = gtk_hbutton_box_new();
#endif
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(hbox), 1);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
	but = bf_stock_cancel_button(G_CALLBACK(multi_thumbnail_cancel_clicked), mtd);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 5);
	but = bf_stock_ok_button(G_CALLBACK(multi_thumbnail_ok_clicked), mtd);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 5);
	gtk_window_set_default(GTK_WINDOW(mtd->win), but);
	gtk_widget_show_all(mtd->win);

	tb = main_v->globses.image_thumbnailsizing_type < 4 ? main_v->globses.image_thumbnailsizing_type : 0;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mtd->radio[tb]), TRUE);
	multi_thumbnail_radio_toggled_lcb(GTK_TOGGLE_BUTTON(mtd->radio[tb]), mtd);
}
