#ifndef __IMAGE_H_
#define __IMAGE_H_

void image_insert_dialog_cb(GtkWidget * widget, gpointer data);
void thumbnail_insert_dialog_cb(GtkWidget * widget, gpointer data);
void image_insert_from_filename(gchar *filename);
void multi_thumbnail_dialog_cb(GtkWidget * widget, gpointer data);
#endif /* __IMAGE_H_ */
