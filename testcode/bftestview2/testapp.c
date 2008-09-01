#include <gtk/gtk.h>
#include <gio/gio.h>
#include "bftextview2.h"

void on_window_destroy(GtkWidget * widget, gpointer data)
{
	gtk_main_quit();
}

/* Callback for close button */
void on_button_clicked(GtkWidget * button, GtkTextBuffer * buffer)
{
	g_print("not used");
}

int main(int argc, char *argv[])
{
	GtkWidget *window, *scroll;
	GtkWidget *vbox;
	Tbftextview2 *text_view;
	GtkWidget *button;
	GtkTextBuffer *buffer;
	gchar *data = NULL;
	gsize datalen;
	GFile *uri;


	gtk_init(&argc, &argv);

	/* Create a Window. */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Highlight widget test");

	/* Set a decent default size for the window. */
	gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_window_destroy), NULL);

	vbox = gtk_vbox_new(FALSE, 2);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	/* Create a multiline text widget. */
	buffer = gtk_text_buffer_new(NULL);
	text_view = bftextview2_new_with_buffer(buffer);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), text_view);
	gtk_box_pack_start(GTK_BOX(vbox), scroll, 1, 1, 0);

	uri = g_file_new_for_commandline_arg(argv[1]);
	g_file_load_contents(uri, NULL, &data, &datalen, NULL, NULL);
	gtk_text_buffer_set_text(buffer, data, datalen);

	/* Create a close button. */
	button = gtk_button_new_with_label("test");
	gtk_box_pack_start(GTK_BOX(vbox), button, 0, 0, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_button_clicked), buffer);

	gtk_widget_show_all(window);

	gtk_main();
	return 0;
}