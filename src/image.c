#include <gtk/gtk.h>

#include "bluefish.h"
#include "html_diag.h"
#include "cap.h"
#include "document.h"
#include "gtk_easy.h"

typedef struct {
	Thtml_diag *dg;
	GtkWidget *frame;
	GdkPixbuf *pb;
	GtkWidget *im;
} Timage_diag;


void image_diag_destroy_cb(GtkWidget * widget, GdkEvent *event,  Timage_diag *imdg) {
	window_destroy(imdg->dg->dialog);
	g_free(imdg->dg);
	g_free(imdg);
}

static void image_insert_dialogok_lcb(GtkWidget * widget, Timage_diag *imdg) {
	gchar *thestring, *finalstring;

	thestring = g_strdup(cap("<IMG"));
	thestring = insert_string_if_entry(imdg->dg->entry[0], cap("SRC"), thestring, NULL);
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
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(imdg->dg->spin[0]), pb_width);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(imdg->dg->spin[1]), pd_height);
	
	tmp_pb = gdk_pixbuf_scale_simple(imdg->pb, (pb_width / toobig), (pd_height / toobig), GDK_INTERP_NEAREST);
	imdg->im = gtk_image_new_from_pixbuf(tmp_pb);
	gdk_pixbuf_unref(tmp_pb);
	gtk_container_add(GTK_CONTAINER(imdg->frame), imdg->im);
	gtk_widget_show(imdg->im);
}

void image_insert_dialog_backend(gchar *filename,GtkWidget *widget, gpointer data) {
	static gchar *tagitems[] = { "width", "height", "alt", "border", "src", "hspace", "vspace", "align", "name", "usemap", NULL };
	gchar *tagvalues[11];
	gchar *custom = NULL;
	Timage_diag *imdg;
	
	GtkWidget *dgtable;

	imdg = g_new(Timage_diag, 1);
	imdg->pb = NULL;
	imdg->im = NULL;
	imdg->dg = html_diag_new(_("Insert image"));
	fill_dialogvalues(tagitems, tagvalues, &custom, (Ttagpopup *) data, widget, imdg->dg);

	imdg->frame = gtk_frame_new(_("Preview"));
	gtk_widget_set_usize(imdg->frame, -1, 50);
	gtk_box_pack_start(GTK_BOX(imdg->dg->vbox), imdg->frame, FALSE, FALSE, 0);

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
	image_insert_dialog_backend(NULL, widget, data);
}
