#include <gtk/gtk.h>
#include <string.h>

#define DEBUG

#include "bluefish.h"
#include "html_diag.h"
#include "cap.h"
#include "document.h"
#include "file.h"
#include "gtk_easy.h"
#include "bf_lib.h"
#include "stringlist.h"

static gchar *create_thumbnail_filename(gchar *filename) {
	gchar *retval, *tmp;
	gint len = 0, size;
	tmp = strrchr(filename, '.');
	if (tmp) {
		len = strlen(tmp);
	}
	size = strlen(filename)-len+strlen(main_v->props.image_thumbnailstring)+strlen(main_v->props.image_thumbnailtype)+2;
	retval = g_malloc0(size*sizeof(gchar));
	DEBUG_MSG("create_thumbnail_filename, size=%d bytes at %p\n", size, retval);
	retval = strncpy(retval, filename, strlen(filename)-len);
	retval = strcat(retval, main_v->props.image_thumbnailstring);
	retval = strcat(retval, ".");
	retval = strcat(retval, main_v->props.image_thumbnailtype);

	return retval;
}

typedef struct {
	Thtml_diag *dg;
	GtkWidget *frame;
	GdkPixbuf *pb;
	GnomeVFSURI *full_uri;
	GtkWidget *im;

	GdkPixbufLoader* pbloader;
	Topenfile *of;

	/* and some options for the thumbnails */
	gboolean is_thumbnail;
	GtkAdjustment *adjustment;
	guint adj_changed_id;
} Timage_diag;

void image_diag_destroy_cb(GtkWidget * widget, Timage_diag *imdg) {
	html_diag_destroy_cb(widget,imdg->dg);
	if (imdg->pb) {
		g_object_unref(imdg->pb);
	}
	if (imdg->full_uri) {
		gnome_vfs_uri_unref(imdg->full_uri);
	}
	g_free(imdg);
}

static void async_thumbsave_lcb(Tsavefile_status status,gint error_info,gpointer callback_data) {
	DEBUG_MSG("async_thumbsave_lcb, status=%d\n",status);
}

static void image_insert_dialogok_lcb(GtkWidget * widget, Timage_diag *imdg) {
	gchar *thestring, *finalstring;

	if (imdg->is_thumbnail) {
		gchar *thumbnailfilename, *filename;
		GnomeVFSURI *fullthumbfilename;
		gchar *tmp1, *tmp2;
		gchar *buffer;
		gsize buflen;
		gint w,h;
		GError *error=NULL;
		GdkPixbuf *tmp_im;

		
		filename = gtk_editable_get_chars(GTK_EDITABLE(imdg->dg->entry[0]), 0, -1);
		thumbnailfilename = create_thumbnail_filename(filename);
		/* we should use the full path to create the thumbnail filename */
		tmp1 = gnome_vfs_uri_to_string(imdg->full_uri,GNOME_VFS_URI_HIDE_PASSWORD);
		tmp2 = create_thumbnail_filename(tmp1);
		fullthumbfilename = gnome_vfs_uri_new(tmp2);
		g_free(tmp1);
		g_free(tmp2);
		
		DEBUG_MSG("image_insert_dialogok_lcb, thumbnail will be stored at %s\n", gnome_vfs_uri_get_path(fullthumbfilename));
		w = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(imdg->dg->spin[0]));
		h = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(imdg->dg->spin[1]));
		tmp_im = gdk_pixbuf_scale_simple(imdg->pb, w, h, GDK_INTERP_BILINEAR);
		if (strcmp(main_v->props.image_thumbnailtype, "jpeg")==0) {
			gdk_pixbuf_save_to_buffer(tmp_im, &buffer, &buflen,main_v->props.image_thumbnailtype,&error, "quality", "50",NULL);
 			/* gdk_pixbuf_save(tmp_im,fullthumbfilename,main_v->props.image_thumbnailtype,&error, "quality", "50",NULL);*/
		} else {
			gdk_pixbuf_save_to_buffer(tmp_im, &buffer, &buflen,main_v->props.image_thumbnailtype,&error,NULL);
			/*gdk_pixbuf_save(tmp_im,fullthumbfilename,main_v->props.image_thumbnailtype,&error, NULL);*/
		}
		g_object_unref (tmp_im);
		if (error) {
			g_print("ERROR while saving thumbnail to buffer: %s\n", error->message);
			g_error_free(error);
		} else {
			Tsavefile *sf;
			Trefcpointer *refbuf = refcpointer_new(buffer);
			DEBUG_MSG("image_insert_dialogok_lcb, starting async save to %s\n",gnome_vfs_uri_get_path(fullthumbfilename));
			sf = file_savefile_uri_async(fullthumbfilename, refbuf, buflen, async_thumbsave_lcb, NULL);
			refcpointer_unref(refbuf);
		}
		gnome_vfs_uri_unref(fullthumbfilename);
		thestring = g_strconcat(cap("<A HREF=\""),filename, cap("\"><IMG SRC=\""), thumbnailfilename, "\"", NULL);
		g_free(filename);
		g_free(thumbnailfilename);
	} else {
		thestring = g_strdup(cap("<IMG"));
		thestring = insert_string_if_entry(imdg->dg->entry[0], cap("SRC"), thestring, NULL);
	}

	thestring = insert_integer_if_spin(imdg->dg->spin[0], cap("WIDTH"), thestring, GTK_TOGGLE_BUTTON(imdg->dg->check[0])->active,0);
	thestring = insert_integer_if_spin(imdg->dg->spin[1], cap("HEIGHT"), thestring, GTK_TOGGLE_BUTTON(imdg->dg->check[1])->active,0);
	if (!main_v->props.xhtml) {
		thestring = insert_integer_if_spin(imdg->dg->spin[2], cap("BORDER"), thestring, FALSE, 1);
	}
	thestring = insert_integer_if_spin(imdg->dg->spin[3], cap("HSPACE"), thestring, FALSE,0);
	thestring = insert_integer_if_spin(imdg->dg->spin[4], cap("VSPACE"), thestring, FALSE,0);
	thestring = insert_string_if_entry(imdg->dg->entry[1], cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry(imdg->dg->entry[2], cap("ALT"), thestring, "");
	thestring = insert_string_if_entry(imdg->dg->entry[3], cap("USEMAP"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(imdg->dg->combo[0])->entry), cap("ALIGN"), thestring, NULL);
	thestring = insert_string_if_entry(imdg->dg->entry[4], NULL, thestring, NULL);

	finalstring = g_strconcat(thestring, (main_v->props.xhtml == 1) ? " />" : ">", NULL);
	g_free(thestring);

	if (imdg->dg->range.end == -1) {
		doc_insert_two_strings(imdg->dg->doc, finalstring, imdg->is_thumbnail == 1 ? cap("</a>") : NULL);
	} else {
		doc_replace_text(imdg->dg->doc, finalstring, imdg->dg->range.pos, imdg->dg->range.end);
	}

	g_free(finalstring);
	image_diag_destroy_cb(NULL, imdg);	
}

void image_diag_cancel_clicked_cb(GtkWidget *widget, gpointer data) {
	image_diag_destroy_cb(NULL, data);
}

static void image_diag_finish(Timage_diag *imdg, GtkSignalFunc ok_func) {
	GtkWidget *hbox;

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_hseparator_new(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(imdg->dg->vbox), hbox, FALSE, FALSE, 12);
	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 12);
	
	imdg->dg->obut = bf_stock_ok_button(ok_func, imdg);
	imdg->dg->cbut = bf_stock_cancel_button(G_CALLBACK(image_diag_cancel_clicked_cb), imdg);

	gtk_box_pack_start(GTK_BOX(hbox),imdg->dg->cbut , FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox),imdg->dg->obut , FALSE, FALSE, 0);
	gtk_window_set_default(GTK_WINDOW(imdg->dg->dialog), imdg->dg->obut);

	gtk_box_pack_start(GTK_BOX(imdg->dg->vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show_all(GTK_WIDGET(imdg->dg->dialog));
}

static void image_dialog_set_pixbuf(Timage_diag *imdg) {
	gint pb_width, pd_height, toobig;
	GdkPixbuf *tmp_pb;
	if (!imdg->pb) {
		return;
	}
	toobig = 1;
	pb_width = gdk_pixbuf_get_width(imdg->pb);
	if ((pb_width / 250) > toobig) {
		toobig = pb_width / 250;
	}
	pd_height = gdk_pixbuf_get_height(imdg->pb);
	if ((pd_height / 300) > toobig) {
		toobig = pd_height / 300;
	}
	if (imdg->is_thumbnail) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(imdg->dg->spin[0]), (pb_width/toobig));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(imdg->dg->spin[1]), (pd_height/toobig));
		g_signal_handler_block(G_OBJECT(imdg->adjustment), imdg->adj_changed_id);
		gtk_adjustment_set_value(GTK_ADJUSTMENT(imdg->adjustment), 1.0 / toobig);
		g_signal_handler_unblock(G_OBJECT(imdg->adjustment), imdg->adj_changed_id);
	} else {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(imdg->dg->spin[0]), pb_width);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(imdg->dg->spin[1]), pd_height);
	}
	
	tmp_pb = gdk_pixbuf_scale_simple(imdg->pb, (pb_width / toobig), (pd_height / toobig), main_v->props.image_thumbnail_refresh_quality ? GDK_INTERP_BILINEAR : GDK_INTERP_NEAREST);
	
	if (GTK_IS_WIDGET(imdg->im)) {
		DEBUG_MSG("imdg->im == %p\n", imdg->im);
		DEBUG_MSG("gtk_widget_destroy() %p\n", imdg->im);
		gtk_widget_destroy(imdg->im);
	}
	
	imdg->im = gtk_image_new_from_pixbuf(tmp_pb);

	g_object_unref(tmp_pb);
	gtk_container_add(GTK_CONTAINER(imdg->frame), imdg->im);
	gtk_widget_show(imdg->im);
	DEBUG_MSG("imdg->im == %p\n", imdg->im);
	DEBUG_MSG("image_filename_changed() finished. GTK_IS_WIDGET(imdg->im) == %d\n", GTK_IS_WIDGET(imdg->im));
}

static void image_loaded_lcb(Topenfile_status status,gint error_info, gchar *buffer,GnomeVFSFileSize buflen,gpointer callback_data) {
	Timage_diag *imdg = callback_data;
	gboolean cleanup = TRUE;
	switch (status) {
		case OPENFILE_ERROR:
		case OPENFILE_ERROR_NOCHANNEL:
		case OPENFILE_ERROR_NOREAD:
		case OPENFILE_ERROR_CANCELLED:
			/* should we warn the user ?? */
			gdk_pixbuf_loader_close(imdg->pbloader,NULL);
		break;
		case OPENFILE_CHANNEL_OPENED:
			/* do nothing */
			cleanup = FALSE;
		break;
		case OPENFILE_FINISHED: {
			GError *error=NULL;
			if (gdk_pixbuf_loader_write(imdg->pbloader,buffer,buflen,&error) 
						&& gdk_pixbuf_loader_close(imdg->pbloader,&error)) {
				imdg->pb = gdk_pixbuf_loader_get_pixbuf(imdg->pbloader);
				if (imdg->pb) {
					g_object_ref(imdg->pb);
					image_dialog_set_pixbuf(imdg);
				}
			}
		} break;
	}
	if (cleanup) {
		g_object_unref(imdg->pbloader);
		imdg->pbloader = NULL;
		imdg->of = NULL;
	}
}

static void image_filename_changed(GtkWidget * widget, Timage_diag *imdg) {
	const gchar *filename;
	gchar *tmp;
	GnomeVFSURI *fullfilename=NULL;

	DEBUG_MSG("image_filename_changed() started. GTK_IS_WIDGET(imdg->im) == %d\n", GTK_IS_WIDGET(imdg->im));
	if (imdg->pb) {
		g_object_unref(imdg->pb);
	}
	if (imdg->of) {
		openfile_cancel(imdg->of);
	}
	DEBUG_MSG("image_filename_changed: filename=%s\n",gtk_entry_get_text(GTK_ENTRY(imdg->dg->entry[0])));
	
	/* the entry usually has a relative filename, so we should make it absolute 
	using the basedir of the document */
	filename = gtk_entry_get_text(GTK_ENTRY(imdg->dg->entry[0]));
	/* we should use the full path to create the thumbnail filename */
	tmp = strstr(filename, "://");
	if ((tmp == NULL && filename[0] != '/') && imdg->dg->doc->uri && strlen(imdg->dg->doc->uri)) {
		GnomeVFSURI *parent;
		gchar *basedir = path_get_dirname_with_ending_slash(imdg->dg->doc->uri);
		parent = gnome_vfs_uri_new(basedir);
		fullfilename = gnome_vfs_uri_resolve_relative (parent,filename);
		g_free(basedir);
		gnome_vfs_uri_unref(parent);
	} else if (tmp != NULL || filename[0]=='/') {
		fullfilename = gnome_vfs_uri_new(filename);
	} else {
		return;
	}
	DEBUG_MSG("image_filename_changed: fullfilename=%s, loading!\n",gnome_vfs_uri_get_path(fullfilename));
	if (fullfilename) {
		GError *error=NULL;
		gchar *ext = strrchr(filename, '.');
		if (ext) {
			gchar *tmp2 = g_utf8_strdown(ext+1,-1);
			if (strcmp(tmp2, "jpg")==0) {
				imdg->pbloader = gdk_pixbuf_loader_new_with_type("jpeg", &error);
			} else {
				imdg->pbloader = gdk_pixbuf_loader_new_with_type(tmp2, &error);
			}
			if (error) {
				imdg->pbloader = gdk_pixbuf_loader_new(); /* try to guess from the data */
			}
			g_free(tmp2);
		} else {
			imdg->pbloader = gdk_pixbuf_loader_new();
		}
		imdg->of = file_openfile_uri_async(fullfilename, image_loaded_lcb, imdg);
		imdg->full_uri = fullfilename;
	}
}

static void image_adjust_changed(GtkAdjustment * adj, Timage_diag *imdg) {
	GdkPixbuf *tmp_pb;
	gint tn_width, tn_height;
	DEBUG_MSG("image_adjust_changed started. GTK_IS_WIDGET(imdg->im) == %d\n", GTK_IS_WIDGET(imdg->im));
	if (!imdg->pb) {
		image_filename_changed(NULL, imdg);
		return;
	}

	tn_width = imdg->adjustment->value * gdk_pixbuf_get_width(imdg->pb);
	tn_height = imdg->adjustment->value * gdk_pixbuf_get_height(imdg->pb);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(imdg->dg->spin[0]), tn_width);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(imdg->dg->spin[1]),tn_height);

	tmp_pb = gdk_pixbuf_scale_simple(imdg->pb, tn_width, tn_height, main_v->props.image_thumbnail_refresh_quality ? GDK_INTERP_BILINEAR : GDK_INTERP_NEAREST);
	
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

void image_insert_dialog_backend(gchar *filename,Tbfwin *bfwin, Ttagpopup *data, gboolean is_thumbnail) {
	gchar *tagvalues[11];
	gchar *custom = NULL;
	Timage_diag *imdg;
	GList *popuplist = NULL;
	GtkWidget *dgtable;

	imdg = g_new0(Timage_diag, 1);
	imdg->is_thumbnail = is_thumbnail;
	if (is_thumbnail) {
		imdg->dg = html_diag_new(bfwin,_("Insert thumbnail"));
	} else {
		imdg->dg = html_diag_new(bfwin,_("Insert image"));
	}
	{
		static gchar *tagitems[] = { "width", "height", "alt", "border", "src", "hspace", "vspace", "align", "name", "usemap", NULL };
		fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, imdg->dg);
	}
	imdg->frame = gtk_frame_new(_("Preview"));
/*	gtk_widget_set_usize(imdg->frame, -1, 50);*/
	gtk_box_pack_start(GTK_BOX(imdg->dg->vbox), imdg->frame, TRUE, TRUE, 0);
	if (is_thumbnail) {
		GtkWidget *scale;

		imdg->adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.5, 0.0001, 1.1, 0.001, 0.1, 0.1));
		scale = gtk_hscale_new(imdg->adjustment);
		imdg->adj_changed_id = g_signal_connect(G_OBJECT(imdg->adjustment), "value_changed", G_CALLBACK(image_adjust_changed), imdg);
		gtk_scale_set_digits(GTK_SCALE(scale), 3);
		gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DISCONTINUOUS);
		gtk_box_pack_start(GTK_BOX(imdg->dg->vbox), scale, FALSE, FALSE, 0);
	}

	dgtable = html_diag_table_in_vbox(imdg->dg, 5, 9);
	if (filename){
		imdg->dg->entry[0] = entry_with_text(filename, 1024);
	} else {
		imdg->dg->entry[0] = entry_with_text(tagvalues[4], 1024);
	}
	bf_mnemonic_label_tad_with_alignment(_("_Image location:"), imdg->dg->entry[0], 0, 0.5, dgtable, 0, 1, 0, 1); 
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->entry[0], 1, 7, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), file_but_new(imdg->dg->entry[0], 0, bfwin), 7, 9, 0, 1);
	g_signal_connect(G_OBJECT(imdg->dg->entry[0]), "changed", G_CALLBACK(image_filename_changed), imdg);

	imdg->dg->spin[0] = spinbut_with_value(NULL, 0, 5000, 1.0, 10.0);
	imdg->dg->check[0] = gtk_check_button_new_with_label("%");
	parse_integer_for_dialog(tagvalues[0], imdg->dg->spin[0] , NULL, imdg->dg->check[0]);
	bf_mnemonic_label_tad_with_alignment(_("_Width:"), imdg->dg->spin[0], 0, 0.5, dgtable, 6, 7, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->spin[0], 7, 8, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->check[0], 8, 9, 1, 2);

	imdg->dg->spin[1] = spinbut_with_value(NULL, 0, 5000, 1.0, 10.0);
	imdg->dg->check[1] = gtk_check_button_new_with_label("%");
	parse_integer_for_dialog(tagvalues[1], imdg->dg->spin[1] , NULL, imdg->dg->check[1]);
	bf_mnemonic_label_tad_with_alignment(_("Hei_ght:"), imdg->dg->spin[1], 0, 0.5, dgtable, 6, 7, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->spin[1], 7, 8, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->check[1], 8, 9, 2, 3);

	imdg->dg->entry[3] = entry_with_text(tagvalues[9], 1024);
	bf_mnemonic_label_tad_with_alignment(_("_Usemap:"), imdg->dg->entry[3], 0, 0.5, dgtable, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->entry[3], 1, 3, 1, 2);

	imdg->dg->entry[1] = entry_with_text(tagvalues[8], 1024);
	bf_mnemonic_label_tad_with_alignment(_("_Name:"), imdg->dg->entry[1], 0, 0.5, dgtable, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->entry[1], 1, 3, 2, 3);

	imdg->dg->entry[2] = entry_with_text(tagvalues[2], 1024);
	bf_mnemonic_label_tad_with_alignment(_("Alternate _text:"), imdg->dg->entry[2], 0, 0.5, dgtable, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->entry[2], 1, 6, 3, 4);

	imdg->dg->entry[4] = entry_with_text(custom, 1024);
	bf_mnemonic_label_tad_with_alignment(_("Custo_m:"), imdg->dg->entry[4], 0, 0.5, dgtable, 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->entry[4], 1, 6, 4, 5);

	imdg->dg->spin[3] = spinbut_with_value(tagvalues[5], 0, 500, 1.0, 5.0);
	bf_mnemonic_label_tad_with_alignment(_("_Hspace:"), imdg->dg->spin[3], 0, 0.5, dgtable, 6, 7, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->spin[3], 7, 9, 3, 4);

	imdg->dg->spin[4] = spinbut_with_value(tagvalues[6], 0, 500, 1.0, 5.0);
	bf_mnemonic_label_tad_with_alignment(_("_Vspace:"), imdg->dg->spin[4], 0, 0.5, dgtable, 6, 7, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->spin[4], 7, 9, 4, 5);

	popuplist = g_list_append(NULL, "bottom");
	popuplist = g_list_append(popuplist, "middle");
	popuplist = g_list_append(popuplist, "top");
	popuplist = g_list_append(popuplist, "left");
	popuplist = g_list_append(popuplist, "right");
	imdg->dg->combo[0] = combo_with_popdown_sized(tagvalues[7], popuplist, 1, 90);
	g_list_free(popuplist);
	bf_mnemonic_label_tad_with_alignment(_("_Align:"), imdg->dg->combo[0], 0, 0.5, dgtable, 3, 4, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->combo[0], 4, 6, 1, 2);
	if (!main_v->props.xhtml) {
		imdg->dg->spin[2] = spinbut_with_value(tagvalues[3], 0, 500, 1.0, 5.0);
		bf_mnemonic_label_tad_with_alignment(_("Borde_r:"), imdg->dg->spin[2], 0, 0.5, dgtable, 3, 4, 2, 3);
		gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->spin[2], 4, 6, 2, 3);
	}
	if (filename || tagvalues[4]) {
		g_signal_emit_by_name(G_OBJECT(imdg->dg->entry[0]), "changed");
	}

	image_diag_finish(imdg, G_CALLBACK(image_insert_dialogok_lcb));
	
	if (custom)	g_free(custom);
}

void image_insert_dialog(Tbfwin *bfwin, Ttagpopup *data) {
	image_insert_dialog_backend(NULL, bfwin, data, FALSE);
}
void thumbnail_insert_dialog(Tbfwin *bfwin) {
	image_insert_dialog_backend(NULL, bfwin, NULL, TRUE);
}
void image_insert_from_filename(Tbfwin *bfwin, gchar *filename) {
	image_insert_dialog_backend(filename,bfwin, NULL, FALSE);
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
	GnomeVFSURI *image;
	GnomeVFSURI *thumb;
	Topenfile *of; /* if != NULL, the image is loading */
	Tsavefile *sf; /* if != NULL, the thumbnail is saving */
	gboolean created; /* both loading and saving is finished */
	gchar *string; /* the string to insert, if NULL && ->create = TRUE means 
						that the string is written to the document */
} Timage2thumb;

typedef struct {
	GtkWidget *win;
	GtkWidget *radio[4];
	GtkWidget *spinlabels[2];
	GtkWidget *spins[2];
	GtkTextBuffer *tbuf;
	gint mode;
	GList *images;
	Tbfwin *bfwin;
} Tmuthudia;

static void multi_thumbnail_dialog_destroy(GtkWidget *wid, Tmuthudia *mtd) {
	DEBUG_MSG("multi_thumbnail_dialog_destroy, called for mtd=%p\n",mtd);
	window_destroy(mtd->win);
	g_free(mtd);
}

static void multi_thumbnail_ok_clicked(GtkWidget *widget, Tmuthudia *mtd) {
	GList *files=NULL, *tmplist;
	gchar *string2insert=g_strdup("");

	gtk_widget_hide(mtd->win);

	if (GTK_TOGGLE_BUTTON(mtd->radio[0])->active) {
		main_v->props.image_thumbnailsizing_type = 0;
	} else if (GTK_TOGGLE_BUTTON(mtd->radio[1])->active) {
		main_v->props.image_thumbnailsizing_type = 1;
	} else if (GTK_TOGGLE_BUTTON(mtd->radio[2])->active) {
		main_v->props.image_thumbnailsizing_type = 2;
	} else if (GTK_TOGGLE_BUTTON(mtd->radio[3])->active) {
		main_v->props.image_thumbnailsizing_type = 3;
	}
	{
		gchar *tmp;
		GtkTextIter start, end;
		gtk_text_buffer_get_bounds(mtd->tbuf,&start,&end);
		tmp = gtk_text_buffer_get_text(mtd->tbuf, &start, &end, FALSE);
		if (tmp) {
			if (main_v->props.image_thumnailformatstring) g_free(main_v->props.image_thumnailformatstring);
			main_v->props.image_thumnailformatstring = tmp;
		}
	}
	{
		GtkWidget *dialog;
		/*dialog = gtk_file_chooser_dialog_new (_("Select files for thumbnail creation"),NULL,
				GTK_FILE_CHOOSER_ACTION_OPEN,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);
		gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);*/
		dialog = file_chooser_dialog(mtd->bfwin, _("Select files for thumbnail creation"), GTK_FILE_CHOOSER_ACTION_OPEN, NULL, TRUE, TRUE, "webimage");
		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
			GSList *slist = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
			files = glist_from_gslist(slist);
			g_slist_free(slist);
		}
		gtk_widget_destroy (dialog);
	}
	tmplist = g_list_first(files);
	while (tmplist) {
		GdkPixbuf *tmp_im1, *tmp_im2;
		gint tw,th,ow,oh;
		GError *error=NULL;
		gchar *thumbfilename, *filename=(gchar *)tmplist->data, *relfilename;
	
		if (mtd->bfwin->current_document->uri) {
			relfilename = create_relative_link_to(mtd->bfwin->current_document->uri, filename);
			DEBUG_MSG("create_relative_link_to, filename=%s relfilename=%s \n", filename, relfilename);
		} else {
			relfilename = g_strdup (filename);
		}
		thumbfilename = create_thumbnail_filename(relfilename);
		g_free(relfilename);

		DEBUG_MSG("filename=%s, thumbfilename=%s\n", filename, thumbfilename);
		tmp_im1 = gdk_pixbuf_new_from_file(filename, NULL);
		ow = gdk_pixbuf_get_width(tmp_im1);
		oh = gdk_pixbuf_get_height(tmp_im1);
		main_v->props.image_thumbnailsizing_val1 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mtd->spins[0]));
		main_v->props.image_thumbnailsizing_val2 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mtd->spins[1]));

		switch (main_v->props.image_thumbnailsizing_type) {
		case 0:
			tw = (1.0*ow / 100 * main_v->props.image_thumbnailsizing_val1);
			th = (1.0*oh / 100 * main_v->props.image_thumbnailsizing_val1);
		break;
		case 1:
			tw = main_v->props.image_thumbnailsizing_val1;
			th = (1.0*tw/ow*oh);
		break;
		case 2:
			th = main_v->props.image_thumbnailsizing_val1;
			tw = (1.0*th/oh*ow);
		break;
		default: /* all fall back to type 3 */
			tw = main_v->props.image_thumbnailsizing_val1;
			th = main_v->props.image_thumbnailsizing_val2;
		break;
		}
		DEBUG_MSG("scaling %s to %dx%d\n", filename, tw,th);
		tmp_im2 = gdk_pixbuf_scale_simple(tmp_im1, tw, th, GDK_INTERP_BILINEAR);
		gdk_pixbuf_unref(tmp_im1);
		if (strcmp(main_v->props.image_thumbnailtype, "jpeg")==0) {
	 		gdk_pixbuf_save(tmp_im2,thumbfilename,main_v->props.image_thumbnailtype,&error, "quality", "50", NULL);
		} else {
	 		gdk_pixbuf_save(tmp_im2,thumbfilename,main_v->props.image_thumbnailtype,&error, NULL);
		}
 		if (error) {
 			/* do something */
 			g_error_free(error);
 		}
		gdk_pixbuf_unref (tmp_im2);
		{
			Tconvert_table *table, *tmpt;
			gchar *str, *tmp;
			table = tmpt = g_new(Tconvert_table, 8);
			tmpt->my_int = 'r';
			tmpt->my_char = g_strdup(filename);
			tmpt++;
			tmpt->my_int = 't';
			tmpt->my_char = g_strdup(thumbfilename);
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
			str = replace_string_printflike(main_v->props.image_thumnailformatstring, table);
			DEBUG_MSG("string to insert: %s\n", str);
			tmpt = table;
			while (tmpt->my_char) {
				g_free(tmpt->my_char);
				tmpt++;
			}
			g_free(table);
			tmp = string2insert;
			string2insert = g_strconcat(string2insert, str, "\n", NULL);
			g_free(tmp);
			g_free(str);
		}
		DEBUG_MSG("about to free %s (%p)\n", thumbfilename, thumbfilename);
		g_free(thumbfilename);
		DEBUG_MSG("next thumbnail!!\n");
		tmplist = g_list_next(tmplist);
	}
	DEBUG_MSG("done with all the files, inserting totalstring %s\n", string2insert);
	doc_insert_two_strings(mtd->bfwin->current_document, string2insert, NULL);
	g_free(string2insert);
	DEBUG_MSG("ready, cleanup time!\n");
	free_stringlist(files);
	multi_thumbnail_dialog_destroy(NULL, mtd);
}

static void multi_thumbnail_cancel_clicked(GtkWidget *widget, Tmuthudia *mtd) {
	multi_thumbnail_dialog_destroy(NULL, mtd);
}

static void multi_thumbnail_radio_toggled_lcb(GtkToggleButton *togglebutton,Tmuthudia *mtd) {
	/* only call this for activate, not for de-activate*/
	if (togglebutton->active) {
		if (GTK_TOGGLE_BUTTON(mtd->radio[0])->active) {
			gtk_widget_hide(mtd->spins[1]);
			gtk_widget_hide(mtd->spinlabels[1]);
			gtk_label_set_text(GTK_LABEL(mtd->spinlabels[0]), _("Scaling (%)"));
		} else if (GTK_TOGGLE_BUTTON(mtd->radio[1])->active) {
			gtk_widget_hide(mtd->spins[1]);
			gtk_widget_hide(mtd->spinlabels[1]);
			gtk_label_set_text(GTK_LABEL(mtd->spinlabels[0]), _("Width"));
		} else if (GTK_TOGGLE_BUTTON(mtd->radio[2])->active) {
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

void multi_thumbnail_dialog(Tbfwin *bfwin) {
	Tmuthudia *mtd;
	GtkWidget *vbox, *hbox, *but, *table, *label, *scrolwin, *textview;
	gint tb;
	
	mtd = g_new(Tmuthudia, 1);
	mtd->bfwin = bfwin;
	mtd->win = window_full2(_("Multi thumbnail"), GTK_WIN_POS_MOUSE, 5
		, G_CALLBACK(multi_thumbnail_dialog_destroy), mtd, TRUE, NULL);
	vbox = gtk_vbox_new(FALSE,5);
	gtk_container_add(GTK_CONTAINER(mtd->win), vbox);
	
	table = gtk_table_new(4,3,FALSE);
	mtd->radio[0] = gtk_radio_button_new_with_label(NULL, _("By scaling"));
	mtd->radio[1] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(mtd->radio[0]),_("By width, keep aspect ratio"));
	mtd->radio[2] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(mtd->radio[0]),_("By height, keep aspect ratio"));
	mtd->radio[3] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(mtd->radio[0]),_("By width and height, ignore aspect ratio"));
	mtd->spinlabels[0] = gtk_label_new("");
	mtd->spinlabels[1] = gtk_label_new(_("Height"));
	mtd->spins[0] = gtk_spin_button_new_with_range(0,1000,1);
	mtd->spins[1] = gtk_spin_button_new_with_range(0,1000,1);
	
	g_signal_connect(G_OBJECT(mtd->radio[0]), "toggled", G_CALLBACK(multi_thumbnail_radio_toggled_lcb), mtd);
	g_signal_connect(G_OBJECT(mtd->radio[1]), "toggled", G_CALLBACK(multi_thumbnail_radio_toggled_lcb), mtd);
	g_signal_connect(G_OBJECT(mtd->radio[2]), "toggled", G_CALLBACK(multi_thumbnail_radio_toggled_lcb), mtd);
	g_signal_connect(G_OBJECT(mtd->radio[3]), "toggled", G_CALLBACK(multi_thumbnail_radio_toggled_lcb), mtd);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(mtd->spins[0]), main_v->props.image_thumbnailsizing_val1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(mtd->spins[1]), main_v->props.image_thumbnailsizing_val2);

	gtk_table_attach_defaults(GTK_TABLE(table), mtd->radio[0], 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->radio[1], 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->radio[2], 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->radio[3], 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->spinlabels[0], 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->spinlabels[1], 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->spins[0], 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->spins[1], 2, 3, 1, 2);
	
	gtk_box_pack_start(GTK_BOX(vbox),table,FALSE,FALSE,0);
	
	label = gtk_label_new(_("%r: original filename  %t: thumbnail filename\n%w: original width  %h: original height\n%x: thumbnail width  %y: thumbnail height\n%b: original size (bytes)"));
	gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);

	scrolwin = textview_buffer_in_scrolwin(&textview, -1, -1, main_v->props.image_thumnailformatstring, GTK_WRAP_CHAR);
	mtd->tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	gtk_box_pack_start(GTK_BOX(vbox),scrolwin,TRUE,TRUE,0);
	
	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 1);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE, FALSE, 5);
	but = bf_stock_cancel_button(G_CALLBACK(multi_thumbnail_cancel_clicked), mtd);
	gtk_box_pack_start(GTK_BOX(hbox), but,FALSE, FALSE, 5);
	but = bf_stock_ok_button(G_CALLBACK(multi_thumbnail_ok_clicked), mtd);
	gtk_box_pack_start(GTK_BOX(hbox), but,FALSE, FALSE, 5);
	gtk_window_set_default(GTK_WINDOW(mtd->win), but);
	gtk_widget_show_all(mtd->win);

	tb = main_v->props.image_thumbnailsizing_type < 4 ? main_v->props.image_thumbnailsizing_type : 0;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mtd->radio[tb]), TRUE);
	multi_thumbnail_radio_toggled_lcb(GTK_TOGGLE_BUTTON(mtd->radio[tb]), mtd);
}
