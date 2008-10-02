#include <gtk/gtk.h>
#include <gio/gio.h>
#include "bftextview2.h"
#include "bftextview2_patcompile.h"
#include "bftextview2_langmgr.h"

GtkWidget *text_view;

void on_window_destroy(GtkWidget * widget, gpointer data)
{
	gtk_main_quit();
}

/* Callback for close button */
void on_button_clicked(GtkWidget * button, GtkTextBuffer * buffer)
{
	g_print("not used");
}

void testapp_rescan_bflang(Tbflang *bflang) {
	g_print("rescan if bflang %p\n",bflang);
	if (BLUEFISH_TEXT_VIEW(text_view)->bflang == bflang) {
		bluefish_text_view_rescan(text_view);

	}
}

int main(int argc, char *argv[])
{
	GtkWidget *window, *scroll;
	GtkWidget *vbox;

	GtkWidget *button;
	GtkTextBuffer *buffer;
	gchar *data = NULL;
	gsize datalen;
	GFile *uri;
	GFileInfo* finfo;
	gchar *mime;

	g_thread_init(NULL); /* we don't do gtk from other threads, only glib, so no gdk_thread_init() required */
	gtk_init(&argc, &argv);

	langmgr_init();
	/* Create a Window. */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Highlight widget test");

	/* Set a decent default size for the window. */
	gtk_window_set_default_size(GTK_WINDOW(window), 500, 300);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_window_destroy), NULL);

	vbox = gtk_vbox_new(FALSE, 2);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	/* Create a multiline text widget. */
	buffer = gtk_text_buffer_new(NULL);
	
	text_view = bftextview2_new_with_buffer(buffer);
	/*BLUEFISH_TEXT_VIEW (text_view)->scantable = bftextview2_scantable_new(buffer);*/
	gtk_text_buffer_create_tag(buffer,"needscanning",NULL);
	gtk_text_buffer_create_tag(buffer,"blockmatch","background","red","foreground","white",NULL);
	gtk_text_buffer_create_tag(buffer,"_folded_","editable",FALSE, "invisible", TRUE, NULL);
	gtk_text_buffer_create_tag(buffer,"_foldheader_","editable",FALSE, "background", "#99FF99", NULL);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), text_view);
	gtk_box_pack_start(GTK_BOX(vbox), scroll, 1, 1, 0);

	uri = g_file_new_for_commandline_arg(argv[1]);
	
	g_file_load_contents(uri, NULL, &data, &datalen, NULL, NULL);
	gtk_text_buffer_set_text(buffer, data, datalen);
	g_free(data);
	
	finfo = g_file_query_info(uri,G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,G_FILE_QUERY_INFO_NONE,NULL,NULL);
	mime = g_file_info_get_content_type(finfo);
	g_print("setting mime %s\n",mime);
	bluefish_text_view_set_mimetype(text_view, mime);
	g_print("text_view->bflang = %p\n",BLUEFISH_TEXT_VIEW(text_view)->bflang);
	/* Create a close button. */
	button = gtk_button_new_with_label("test");
	gtk_box_pack_start(GTK_BOX(vbox), button, 0, 0, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_button_clicked), buffer);

	gtk_widget_show_all(window);

	gtk_main();
	return 0;
}
