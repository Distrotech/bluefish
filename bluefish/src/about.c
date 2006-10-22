/* Bluefish HTML Editor
 * about.c - the About dialog
 *
 * Copyright (C) 2004 Eugene Morenko(More) more@irpin.com
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

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   /* getopt() */

#include "config.h"

#include "bluefish.h"
#include "about.h"
#include "gtk_easy.h"

static GtkWidget *info;

static void add_page(GtkNotebook * notebook, const gchar * name, const gchar * buf,
					 gboolean hscrolling) {
	GtkWidget *textview, *label, *sw;
	label = gtk_label_new(name);
	sw = textview_buffer_in_scrolwin(&textview, -1, 200, buf, GTK_WRAP_WORD);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (textview), FALSE);
	gtk_notebook_append_page(notebook, sw, label);
}

static void about_dialog_close_lcb(GtkObject *object,GtkWidget *win) {
	window_destroy(win);
}

void about_dialog_create(gpointer * data, guint * callback_action, GtkWidget * widget) {
	GtkWidget *vbox, *vbox2, *hbox;
	GtkWidget *notebook;
	GtkWidget *info_ok_button;
	GdkColor color;
	gchar *I1, *config_options;
	const gchar *INFO = _("Bluefish is an editor for experienced web designers and programmers. It supports many programming and markup languages, but focuses on editing dynamic and interactive websites. Bluefish is an open source development project, released under the GPL license.\n\nFor more information, visit the Bluefish Website at http://bluefish.openoffice.nl/\n\nThis version of bluefish was compiled with the following options:\n");
	const gchar *AUTHORS = _("Project leader:\n\
  Olivier Sessink <olivier@bluefish.openoffice.nl>\n\
\n\
Developers for this release:\n\
  Olivier Sessink\n\
   <olivier@bluefish.openoffice.nl>\n\
  Jim Hayward\n\
   <jim@bluefish.openoffice.nl>\n\
  Daniel Leidert\n\
   <daniel.leidert@wgdd.de>\n\
  Oskar Świda\n\
   <swida@aragorn.pb.bialystok.pl>\n\
  David Marrs\n\
   <David.Marrs@myrealbox.com>\n\
  Michèle Garoche\n\
   <michele.garoche@easyconnect.fr>\n\
\n\
Developers for previous releases:\n\
  Oskar Świda\n\
   <swida@aragorn.pb.bialystok.pl>\n\
  Eugene Morenko(More)\n\
   <more@irpin.com>\n\
  Alastair Porter\n\
   <alastair@porter.net.nz>\n\
  Chris Mazuc\n\
  Neil Millar\n\
  Gero Takke\n\
  Bo Forslund\n\
  David Arno\n\
  Pablo De Napoli\n\
  Santiago Capel Torres\n\
  Rasmus Toftdahl Olesen\n\
   <rto@pohldata.dk>\n\
  Roland Steinbach\n\
   <roland@netzblick.de>\n\
  Christian Tellefsen\n\
   <chris@tellefsen.net>\n\
  Antti-Juhani Kaijanaho\n\
\n\
Documentation:\n\
  Michèle Garoche\n\
   <michele.garoche@easyconnect.fr>\n\
  Scott White\n\
   <wwsw3@earthlink.net>\n\
  Anita Lewis\n\
   <ajreiki@highstream.net>\n\
  Alastair Porter\n\
   <alastair@porter.net.nz>\n\
  Daniel Blair\n\
   <joecamel@realcoders.org>\n\
  Olivier Sessink\n\
   <olivier@bluefish.openoffice.nl>\n\
  Denny Reeh\n\
\n\
Package Maintainers:\n\
  Debian:   Daniel Leidert\n\
   <daniel.leidert@wgdd.de>\n\
  Redhat:   Matthias Haase\n\
   <matthias_haase@bennewitz.com>\n\
  Mandrake: Todd Lyons\n\
   <todd@mrball.net>\n\
  Fink: Michèle Garoche\n\
   <michele.garoche@easyconnect.fr>\n\
\n\
If you know of anyone missing from this list, please let us know\n\
<bluefish@bluefish.openoffice.nl>\n\
\n\
Thanks to all who helped making this software available.\n\
");
	const gchar *TRANSLATORS = _("Current translators:\n\n\
  Basque: Hizkuntza Politikarako Sailburuordetza\n\
   <hizkpol@ej-gv.es>\n\n\
  Brazilian Portuguese: Anderson Rocha\n\
   <anderson@maxlinux.com.br>\n\n\
  Bulgarian: Peio Popov\n\
   <peio@peio.org>\n\n\
  Chinese: Ting Yang (Dormouse)\n\
   <mouselinux@163.com>\n\n\
  Chinese, Traditional: OLS3\n\
   <ols3@lxer.idv.tw>\n\n\
  Danish: Rasmus Toftdahl Olesen\n\
   <rto@pohldata.dk>\n\n\
  Finnish: Juho Roukala\n\
   <j.mr@luukku.com>\n\n\
  French: Michèle Garoche\n\
   <michele.garoche@easyconnect.fr>\n\n\
  German: Roland Steinbach\n\
   <roland@netzblick.de>\n\n\
  Hungarian: Péter Sáska\n\
   <sasek@ccsystem.hu>\n\n\
  Italian: Stefano Canepa\n\
   <sc@linux.it>\n\n\
  Japanese: Takeshi Hamasaki\n\
   <hma@syd.odn.ne.jp>\n\n\
  Norwegian: Christian Tellefsen\n\
   <chris@tellefsen.net>\n\n\
  Polish: Oskar Swida\n\
   <swida@aragorn.pb.bialystok.pl>\n\n\
  Portuguese: Lopo Pizarro\n\
   <lopopizarro@netcabo.pt>\n\n\
  Russian: Eugene Rupakov\n\
   <rupakov@jet.msk.su>\n\n\
  Serbian: Marko Milenović\n\
   <raven@sbb.co.yu>\n\n\
  Slovak: Vladimir VASIL\n\
   <vvasil@post.sk>\n\n\
  Spanish: Walter Oscar Echarri\n\
   <wecharri@gmail.com>\n\n\
  Swedish: David Smeringe\n\
   <david.smeringe@telia.com>\n\n\
  Tamil: Murugapandian Barathee\n\
   <barathee@yahoo.com>\n\n\
  Turkish: Oğuz Eren\n\
   <ogzeren@gmail.com>\n\n\
");

	info = window_full2(_("About Bluefish"), GTK_WIN_POS_CENTER, 6
			,G_CALLBACK(about_dialog_close_lcb),NULL, TRUE, NULL);
	gtk_window_set_resizable(GTK_WINDOW(info), FALSE);
	color.red = 65535;
	color.blue = 65535;
	color.green = 65535;
	gtk_widget_modify_bg(info, GTK_STATE_NORMAL,&color);

	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(info), vbox2);
	{
		GError *error=NULL;
		GtkWidget *image;
		GdkPixbuf* pixbuf= gdk_pixbuf_new_from_file(BLUEFISH_SPLASH_FILENAME,&error);
		if (error) {
			g_print("ERROR while loading splash screen: %s\n", error->message);
			g_error_free(error);
		} else if (pixbuf) {
			image = gtk_image_new_from_pixbuf(pixbuf);
			gtk_box_pack_start(GTK_BOX(vbox2), image, FALSE, FALSE, 0);
			g_object_unref(pixbuf);
		}
	}

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), vbox, TRUE, TRUE, 0);

	/* the notebook */
	notebook = gtk_notebook_new();

	config_options = g_strconcat (CONFIGURE_OPTIONS, _("\n\nResulting in the detection of:\n"),
#ifdef HAVE_LIBASPELL
			"libaspell\n",
#endif
#ifdef DEVELOPMENT
			"Development request\n",
#endif
#ifdef ENABLE_NLS
			"i18n request\n",
#endif
#ifdef HAVE_ATLEAST_GTK_2_4
			"GTK+ 2.4 or newer\n",
#endif
#ifdef HAVE_ATLEAST_GNOMEVFS_2_6
			"GnomeVFS 2.6 or newer\n",
#endif
#ifdef HAVE_ATLEAST_GNOMEUI_2_6
			"libgnomeui 2.6 or newer\n",
#endif
#ifdef PLATFORM_DARWIN
			"Platform Darwin",
#endif
			NULL);

	/* add pages */
	I1 = g_strconcat(INFO, config_options, NULL);
	g_free (config_options);
	add_page(GTK_NOTEBOOK(notebook), _("Info"), I1, TRUE);
	g_free(I1);
	add_page(GTK_NOTEBOOK(notebook), _("Authors"), AUTHORS, TRUE);
	add_page(GTK_NOTEBOOK(notebook), _("Translators"), TRANSLATORS, TRUE);

	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	hbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start( GTK_BOX (vbox), hbox, FALSE, FALSE, 4);
	info_ok_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	GTK_WIDGET_SET_FLAGS(info_ok_button, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(hbox), info_ok_button, FALSE, FALSE, 0);
	gtk_widget_grab_default(info_ok_button);

	g_signal_connect(info_ok_button, "clicked", G_CALLBACK(about_dialog_close_lcb), info);

	gtk_widget_show_all(info);
}
