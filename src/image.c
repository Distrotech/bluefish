#include <gtk/gtk.h>
#include <string.h>

#define DEBUG

#include "bluefish.h"
#include "html_diag.h"
#include "cap.h"
#include "document.h"
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
	GtkWidget *im;

	/* and some options for the thumbnails */
	gboolean is_thumbnail;
	GtkAdjustment *adjustment;
	guint adj_changed_id;
} Timage_diag;

void image_diag_destroy_cb(GtkWidget * widget, GdkEvent *event,  Timage_diag *imdg) {
	window_destroy(imdg->dg->dialog);
	g_free(imdg->dg);
	g_free(imdg);
}

static void image_insert_dialogok_lcb(GtkWidget * widget, Timage_diag *imdg) {
	gchar *thestring, *finalstring;

	if (imdg->is_thumbnail) {
		gchar *thumbnailfilename, *filename;

		filename = gtk_editable_get_chars(GTK_EDITABLE(imdg->dg->entry[0]), 0, -1);
		thumbnailfilename = create_thumbnail_filename(filename);
		{
			gint w,h;
			GError *error=NULL;
			GdkPixbuf *tmp_im;
			w = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(imdg->dg->spin[0]));
			h = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(imdg->dg->spin[1]));
			tmp_im = gdk_pixbuf_scale_simple(imdg->pb, w, h, GDK_INTERP_BILINEAR);
 			gdk_pixbuf_save(tmp_im,thumbnailfilename,main_v->props.image_thumbnailtype,&error, NULL);
			gdk_pixbuf_unref (tmp_im);
			if (error) {
				g_print("ERROR while saving thumbnail: %s\n", error->message);
				g_error_free(error);
			}
		}
		thestring = g_strconcat(cap("<A HREF=\""),filename, cap("\"><IMG SRC=\""), thumbnailfilename, "\"", NULL);
		g_free(filename);
		g_free(thumbnailfilename);
	} else {
		thestring = g_strdup(cap("<IMG"));
		thestring = insert_string_if_entry(imdg->dg->entry[0], cap("SRC"), thestring, NULL);
	}

	thestring = insert_integer_if_spin(imdg->dg->spin[0], cap("WIDTH"), thestring, imdg->dg->check[0]);
	thestring = insert_integer_if_spin(imdg->dg->spin[1], cap("HEIGHT"), thestring, imdg->dg->check[1]);
	thestring = insert_integer_if_spin(imdg->dg->spin[2], cap("BORDER"), thestring, NULL);
	thestring = insert_integer_if_spin(imdg->dg->spin[3], cap("HSPACE"), thestring, NULL);
	thestring = insert_integer_if_spin(imdg->dg->spin[4], cap("VSPACE"), thestring, NULL);
	thestring = insert_string_if_entry(imdg->dg->entry[1], cap("NAME"), thestring, NULL);
	thestring = insert_string_if_entry(imdg->dg->entry[2], cap("ALT"), thestring, NULL);
	thestring = insert_string_if_entry(imdg->dg->entry[3], cap("USEMAP"), thestring, NULL);
	thestring = insert_string_if_entry(GTK_WIDGET(GTK_COMBO(imdg->dg->combo[0])->entry), cap("ALIGN"), thestring, NULL);
	thestring = insert_string_if_entry(imdg->dg->entry[4], NULL, thestring, NULL);

	finalstring = g_strdup_printf(main_v->props.xhtml == 1 ? "%s />" : "%s>", thestring);
	g_free(thestring);

	if (imdg->dg->range.end == -1) {
		doc_insert_two_strings(imdg->dg->doc, finalstring, NULL);
	} else {
		doc_replace_text(imdg->dg->doc, finalstring, imdg->dg->range.pos, imdg->dg->range.end);
	}

	g_free(finalstring);
	image_diag_destroy_cb(NULL, NULL, imdg);	
}

void image_diag_cancel_clicked_cb(GtkWidget *widget, gpointer data) {
	image_diag_destroy_cb(NULL, NULL, data);
}

static void image_diag_finish(Timage_diag *imdg, GtkSignalFunc ok_func) {
	GtkWidget *hbox;

	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 1);
	
	imdg->dg->obut = bf_stock_ok_button(ok_func, imdg);
	imdg->dg->cbut = bf_stock_cancel_button(G_CALLBACK(image_diag_cancel_clicked_cb), imdg);

	gtk_box_pack_start(GTK_BOX(hbox),imdg->dg->cbut , FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox),imdg->dg->obut , FALSE, FALSE, 0);
	gtk_window_set_default(GTK_WINDOW(imdg->dg->dialog), imdg->dg->obut);

	gtk_box_pack_start(GTK_BOX(imdg->dg->vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show_all(GTK_WIDGET(imdg->dg->dialog));
}

static void image_filename_changed(GtkWidget * widget, Timage_diag *imdg) {
	gint pb_width, pd_height, toobig;
	GdkPixbuf *tmp_pb;
	if (imdg->pb) {
		gdk_pixbuf_unref(imdg->pb);
	}
	if (imdg->im) {
		gtk_widget_destroy(imdg->im);
	}
	imdg->pb = gdk_pixbuf_new_from_file(gtk_entry_get_text(GTK_ENTRY(imdg->dg->entry[0])), NULL);
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
	
	tmp_pb = gdk_pixbuf_scale_simple(imdg->pb, (pb_width / toobig), (pd_height / toobig), GDK_INTERP_NEAREST);
	imdg->im = gtk_image_new_from_pixbuf(tmp_pb);
	gdk_pixbuf_unref(tmp_pb);
	gtk_container_add(GTK_CONTAINER(imdg->frame), imdg->im);
	gtk_widget_show(imdg->im);
}


static void image_adjust_changed(GtkAdjustment * adj, Timage_diag *imdg) {
	GdkPixbuf *tmp_pb;
	gint tn_width, tn_height;
	if (!imdg->pb) {
		image_filename_changed(NULL, imdg);
		return;
	}
	if (imdg->im) {
		gtk_widget_destroy(imdg->im);
	}
	tn_width = imdg->adjustment->value * gdk_pixbuf_get_width(imdg->pb);
	tn_height = imdg->adjustment->value * gdk_pixbuf_get_height(imdg->pb);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(imdg->dg->spin[0]), tn_width);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(imdg->dg->spin[1]),tn_height);

	tmp_pb = gdk_pixbuf_scale_simple(imdg->pb, tn_width, tn_height, GDK_INTERP_NEAREST);
	imdg->im = gtk_image_new_from_pixbuf(tmp_pb);
	gdk_pixbuf_unref(tmp_pb);
	gtk_container_add(GTK_CONTAINER(imdg->frame), imdg->im);
	gtk_widget_show(imdg->im);
}

void image_insert_dialog_backend(gchar *filename,GtkWidget *widget, gpointer data, gboolean is_thumbnail) {
	gchar *tagvalues[11];
	gchar *custom = NULL;
	Timage_diag *imdg;
	
	GtkWidget *dgtable;

	imdg = g_new(Timage_diag, 1);
	imdg->pb = NULL;
	imdg->im = NULL;
	imdg->is_thumbnail = is_thumbnail;
	if (is_thumbnail) {
		imdg->dg = html_diag_new(_("Insert image"));
	} else {
		imdg->dg = html_diag_new(_("Insert thumbnail"));
	}
	{
		static gchar *tagitems[] = { "width", "height", "alt", "border", "src", "hspace", "vspace", "align", "name", "usemap", NULL };
		fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, widget, imdg->dg);
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
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Image location")), 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->entry[0], 1, 7, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), file_but_new(imdg->dg->entry[0], imdg->dg->dialog, 0), 7, 9, 0, 1);
	g_signal_connect(G_OBJECT(imdg->dg->entry[0]), "activate", G_CALLBACK(image_filename_changed), imdg);

	imdg->dg->spin[0] = spinbut_with_value(NULL, 0, 5000, 1.0, 10.0);
	imdg->dg->check[0] = gtk_check_button_new_with_label("%");
	parse_integer_for_dialog(tagvalues[0], imdg->dg->spin[0] , NULL, imdg->dg->check[0]);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Width")), 6, 7, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->spin[0], 7, 8, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->check[0], 8, 9, 1, 2);

	imdg->dg->spin[1] = spinbut_with_value(NULL, 0, 5000, 1.0, 10.0);
	imdg->dg->check[1] = gtk_check_button_new_with_label("%");
	parse_integer_for_dialog(tagvalues[1], imdg->dg->spin[1] , NULL, imdg->dg->check[1]);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Height")), 6, 7, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->spin[1], 7, 8, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->check[1], 8, 9, 2, 3);

	imdg->dg->entry[3] = entry_with_text(tagvalues[9], 1024);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Usemap")), 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->entry[3], 1, 3, 1, 2);

	imdg->dg->entry[1] = entry_with_text(tagvalues[8], 1024);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Name")), 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->entry[1], 1, 3, 2, 3);

	imdg->dg->entry[2] = entry_with_text(tagvalues[2], 1024);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Alternate text")), 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->entry[2], 1, 6, 3, 4);

	imdg->dg->entry[4] = entry_with_text(custom, 1024);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Custom")), 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->entry[4], 1, 6, 4, 5);

	imdg->dg->spin[3] = spinbut_with_value(tagvalues[5], 0, 500, 1.0, 5.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Hspace")), 6, 7, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->spin[3], 7, 9, 3, 4);

	imdg->dg->spin[4] = spinbut_with_value(tagvalues[6], 0, 500, 1.0, 5.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Vspace")), 6, 7, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->spin[4], 7, 9, 4, 5);

	imdg->dg->combo[0] = combo_with_popdown_sized(tagvalues[7], recent_attribs.positionlist, 1, 90);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), gtk_label_new(_("Align")), 3, 4, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->combo[0], 4, 6, 1, 2);

	imdg->dg->spin[2] = spinbut_with_value(tagvalues[3], 0, 500, 1.0, 5.0);
	gtk_table_attach_defaults(GTK_TABLE(dgtable),gtk_label_new(_("Border")) , 3, 4, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(dgtable), imdg->dg->spin[2], 4, 6, 2, 3);

	if (filename || tagvalues[4]) {
		g_signal_emit_by_name(G_OBJECT(imdg->dg->entry[0]), "activate");
	}

	image_diag_finish(imdg, G_CALLBACK(image_insert_dialogok_lcb));
	
	if (custom)	g_free(custom);
}

void image_insert_dialog_cb(GtkWidget * widget, gpointer data) {
	image_insert_dialog_backend(NULL, widget, data, FALSE);
}
void thumbnail_insert_dialog_cb(GtkWidget * widget, gpointer data) {
	image_insert_dialog_backend(NULL, NULL, NULL, TRUE);
}

typedef struct {
	GtkWidget *win;
	GtkWidget *radio[4];
	GtkWidget *spinlabels[2];
	GtkWidget *spins[2];
	GtkTextBuffer *tbuf;
	gint mode;
} Tmuthudia;

static void multi_thumbnail_dialog_destroy(GtkWidget *wid, GdkEvent *event, Tmuthudia *mtd) {
	window_destroy(mtd->win);
	g_free(mtd);
}

static void multi_thumbnail_ok_clicked(GtkWidget *widget, Tmuthudia *mtd) {
	GList *files, *tmplist;
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
	files = return_files_w_title(NULL, _("Select files for thumbnail creation"));
	tmplist = g_list_first(files);
	while (tmplist) {
		GdkPixbuf *tmp_im1, *tmp_im2;
		gint tw,th,ow,oh;
		GError *error=NULL;
		gchar *thumbfilename, *filename=(gchar *)tmplist->data;
	
		thumbfilename = create_thumbnail_filename(filename);

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
 		gdk_pixbuf_save(tmp_im2,thumbfilename,main_v->props.image_thumbnailtype,&error, "quality", "50", NULL);
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
			string2insert = g_strconcat(string2insert, str, NULL);
			g_free(tmp);
			g_free(str);
		}
		DEBUG_MSG("about to free %s (%p)\n", thumbfilename, thumbfilename);
		g_free(thumbfilename);
		DEBUG_MSG("next thumbnail!!\n");
		tmplist = g_list_next(tmplist);
	}
	DEBUG_MSG("done with all the files, inserting totalstring %s\n", string2insert);
	doc_insert_two_strings(main_v->current_document, string2insert, NULL);
	g_free(string2insert);
	DEBUG_MSG("ready, cleanup time!\n");
	free_stringlist(files);
	multi_thumbnail_dialog_destroy(NULL, NULL, mtd);
}

static void multi_thumbnail_cancel_clicked(GtkWidget *widget, Tmuthudia *mtd) {
	multi_thumbnail_dialog_destroy(NULL, NULL, mtd);
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

void multi_thumbnail_dialog_cb(GtkWidget * widget, gpointer data) {
	Tmuthudia *mtd;
	GtkWidget *vbox, *hbox, *but, *table, *view, *label;
	
	mtd = g_new(Tmuthudia, 1);
	mtd->win = window_full(_("Multi thumbnail"), GTK_WIN_POS_MOUSE, 5, G_CALLBACK(multi_thumbnail_dialog_destroy), mtd);
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
	if (main_v->props.image_thumbnailsizing_type > 4) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mtd->radio[main_v->props.image_thumbnailsizing_type]), TRUE);
	}
	
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->radio[0], 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->radio[1], 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->radio[2], 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->radio[3], 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->spinlabels[0], 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->spinlabels[1], 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->spins[0], 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), mtd->spins[1], 2, 3, 1, 2);
	
	gtk_box_pack_start(GTK_BOX(vbox),table,FALSE,FALSE,0);
	
	label = gtk_label_new("%r: original filename  %t: thumbnail filename\n%w: original width  %h: original height\n%x: thumbnail width  %y: thumbnail height\n%b: original size (bytes)");
	gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);
	
	mtd->tbuf = gtk_text_buffer_new(NULL);
	gtk_text_buffer_set_text(mtd->tbuf, main_v->props.image_thumnailformatstring, -1);
	view = gtk_text_view_new_with_buffer(mtd->tbuf);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view),GTK_WRAP_CHAR);
	gtk_box_pack_start(GTK_BOX(vbox),view,TRUE,TRUE,0);
	
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
}
