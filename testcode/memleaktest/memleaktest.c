#include <gtk/gtk.h>
#include <gio/gio.h>

GtkWidget *text_view, *window;


void on_window_destroy(GtkWidget * widget, gpointer data)
{
	gtk_widget_destroy(window);
	gtk_main_quit();
}

gint
widget_get_string_size(GtkWidget * widget, gchar * string)
{
	PangoLayout *layout;
	gint retval = -1;
	layout = gtk_widget_create_pango_layout(widget, string);
	if (layout != NULL) {
		pango_layout_get_pixel_size(layout, &retval, NULL);
		g_object_unref(G_OBJECT(layout));
	}
	return retval;
}

gboolean idle_callback(gpointer data) {
	
	gint size = widget_get_string_size(GTK_WIDGET(data), "x");
	g_print("got size %d\n", size);
	return TRUE;
}

void on_button_clicked(GtkWidget * button, GtkTextBuffer * buffer)
{
	g_print("clicked\n");
	g_idle_add(idle_callback, text_view);
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

	gtk_init(&argc, &argv);

	/* Create a Window. */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Highlight widget test");

	/* Set a decent default size for the window. */
	gtk_window_set_default_size(GTK_WINDOW(window), 500, 300);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_window_destroy), NULL);

	vbox = gtk_vbox_new(FALSE, 2);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	buffer = gtk_text_buffer_new(NULL);
	text_view = gtk_text_view_new_with_buffer(buffer);
	g_object_unref(buffer);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), text_view);
	gtk_box_pack_start(GTK_BOX(vbox), scroll, 1, 1, 0);

	if (argc>=1) {
		uri = g_file_new_for_commandline_arg(argv[1]);
		g_file_load_contents(uri, NULL, &data, &datalen, NULL, NULL);
		gtk_text_buffer_set_text(buffer, data, datalen);
		g_free(data);
	}
	
	/* Create a close button. */
	button = gtk_button_new_with_label("test");
	gtk_box_pack_start(GTK_BOX(vbox), button, 0, 0, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_button_clicked), buffer);

	gtk_widget_show_all(window);
	
	gtk_main();
	return 0;
}
