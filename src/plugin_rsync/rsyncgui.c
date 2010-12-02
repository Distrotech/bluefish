/* Bluefish HTML Editor
 * rsyncgui.c - plugin for rsync gui
 *
 * Copyright (C) 2005 Olivier Sessink
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

#include "rsync.h"
#include "../gtk_easy.h"

static void rsync_dialog_close_lcb(GtkObject *object,gpointer data) {
	Trsyncsession *rss = data;
	gtk_widget_destroy(rss->win);
}

static GtkWidget *create_entry(GtkBox *box, const gchar *curval, const gchar *label) {
	GtkWidget *hbox, *retwid;
	hbox = gtk_hbox_new(FALSE,3);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(label), FALSE, FALSE, 3);
	retwid = boxed_entry_with_text(curval, 8, hbox);
	gtk_box_pack_start(GTK_BOX(hbox), file_but_new2(retwid, TRUE, NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER), FALSE, FALSE, 3);
	return retwid;
}

void rsync_dialog_create(gpointer * data, guint * callback_action, GtkWidget * widget) {
	Trsyncsession *rss = data;
	GtkWidget *vbox, *hbox, *bbox, *but, *source_dir_e, *target_dir_e, *hostname_e;
	rss->win = window_full2(_("Rsync options"), GTK_WIN_POS_MOUSE, 10, rsync_dialog_close_lcb,
							rss, TRUE, FALSE);
	vbox = gtk_vbox_new(TRUE,5);
	gtk_container_add(rss->win,vbox);

	source_dir_e = create_entry(vbox,rss->source_dir,_("Source directory"));
	hostname_e = create_entry(vbox,rss->hostname,_("Target Host"));
	target_dir_e = create_entry(vbox,rss->target_dir,_("Target directory"));
	
	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout (GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start( GTK_BOX (vbox), bbox, FALSE, FALSE, 4);
	but = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	GTK_WIDGET_SET_FLAGS(but, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), but, FALSE, FALSE, 0);
	gtk_widget_grab_default(but);
	g_signal_connect(but, "clicked", G_CALLBACK(rsync_dialog_close_lcb), rss);

	
	gtk_widget_show_all(rss->win);
}

