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

#include <gmodule.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>      /* getopt() */

#include "../config.h"
#include "../plugins.h"
#include "../bluefish.h" /* BLUEFISH_SPLASH_FILENAME, gnome_vfs_url_show() */


static void bluefish_url_show(const gchar *url) {
	GnomeVFSResult error = gnome_vfs_url_show(url);
	if (error != GNOME_VFS_OK) {
		g_print("GnomeVFSResult while trying to launch URL in about dialog: error %u\n", error);
	}
}

static void bluefish_url_show_lcb(Tbfwin *bfwin, guint callback_action, GtkWidget *widget) {
	switch (callback_action) {
		case 1:
			bluefish_url_show("http://bluefish.openoffice.nl");
			break;
		case 2:
			bluefish_url_show("http://bugzilla.gnome.org/enter_bug.cgi?product=bluefish");
			break;
		default:
			g_print("bluefish_url_show_cb, unknown action, abort!\n");
			exit(123);
	}
}

static void about_activate_url(GtkAboutDialog *about, const gchar *url, gpointer data) {
	bluefish_url_show(url);
}

static void about_dialog_create(gpointer * data, guint * callback_action, GtkWidget * widget) {
	static GtkWidget *about = NULL;
	GdkPixbuf *logo;
	
	const gchar *artists[] = {
		"Dave Lyon",
		NULL
	};
	
	const gchar *authors[] = {
		"Olivier Sessink <olivier@bluefish.openoffice.nl> (Project leader)",
		"Jim Hayward <jimhayward@linuxexperience.net>",
		"Daniel Leidert <daniel.leidert@wgdd.de>",
		"Oskar Świda <swida@aragorn.pb.bialystok.pl>",
		"Eugene Morenko <more@irpin.com>",
		"Alastair Porter <alastair@linuxexperience.com>",
		_("\nDevelopers of previous releases:"),
		"Antti-Juhani Kaijanaho",
		"Bo Forslund",
		"Chris Mazuc",
		"Christian Tellefsen <chris@tellefsen.net>",
		"David Arno",
		"Gero Takke",
		"Neil Millar",
		"Pablo De Napoli",
		"Rasmus Toftdahl Olesen <rto@pohldata.dk>",
		"Roland Steinbach <roland@netzblick.de>",
		"Santiago Capel Torres",
		_("\nPackage maintainers:"),
		"Debian: Daniel Leidert <daniel.leidert@wgdd.de>",
		"Fink: Michèle Garoche <michele.garoche@easyconnect.fr>",
		"Mandrake: Todd Lyons <todd@mrball.net>",
		"Redhat: Matthias Haase <matthias_haase@bennewitz.com>",
		_("\nIf you know of anyone missing from this list,\nplease let us know via <bluefish@bluefish.openoffice.nl>."),
		_("\nThanks to all who helped making this software available."),
		NULL
	};
	
	const gchar *documenters[] = {
		"Scott White <wwsw3@earthlink.net>",
		"Michèle Garoche <michele.garoche@easyconnect.fr>",
		"Anita Lewis <ajreiki@highstream.net>",
		"Alastair Porter <alastair@porter.net.nz>",
		"Daniel Blair <joecamel@realcoders.org>",
		"Olivier Sessink <olivier@bluefish.openoffice.nl>",
		"Denny Reeh",
		NULL
	};

	const gchar *copyright =
		"Copyright \xc2\xa9 1998-2000 Chris Mazuc.\n"
		"Copyright \xc2\xa9 1998-2006 Olivier Sessink and others.\n";

	/* wrap the license here,
	 * the "wrap-license" property is only available with GTK >= 2.8
	 */
	const gchar *license =
		"This program is free software; you can redistribute it and/or modify it\n"
		"under the terms of the GNU General Public License as published by\n"
		"the Free Software Foundation; either version 2 of the License, or\n"
		"(at your option) any later version.\n"
		"\n"
		"This program is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
		"GNU General Public License for more details.\n"
		"\n"
		"You should have received a copy of the GNU General Public License\n"
		"along with this program; if not, write to the Free Software\n"
		"Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,\n"
		"MA 02110-1301, USA.";
	
	gchar *comments = g_strconcat(
			_("An open-source editor for experienced web designers and programmers, supporting many programming and markup languages, but focusing on creating dynamic and interactive websites."),
			_("\n\nThis version of Bluefish was built with: "),
#ifdef HAVE_LIBASPELL
			_("libaspell support, "),
#endif /* HAVE_LIBASPELL */
#ifdef DEVELOPMENT
			_("development checks support, "),
#endif /* DEVELOPMENT */
#ifdef ENABLE_NLS
			_("i18n support, "),
#endif /* ENABLE_NLS */
			"...",
			NULL);

	/* Translators: This is a special message that shouldn't be translated
	 * literally. It is used in the about box to give credits to
	 * the translators.
	 * Thus, you should translate it to your name and email address and
	 * the name and email address of all translator who have contributed
	 * to this translation (in general: this special locale). Please
	 * use a separate line for every translator ending with a newlines (\n).
	 */
	const gchar *translator_credits = _("translator-credits");

	{
		GError *error=NULL;
		logo = gdk_pixbuf_new_from_file (BLUEFISH_SPLASH_FILENAME, &error);
		if (error) {
			g_print("ERROR while loading splash screen: %s\n", error->message);
			g_error_free(error);
		}
	}
	
	/* use gnome_vfs_url_show() in activate_url() */
	gtk_about_dialog_set_url_hook(about_activate_url, NULL, NULL);
	/* gtk_about_dialog_set_email_hook(); */

	about = g_object_new (GTK_TYPE_ABOUT_DIALOG,
			"logo", logo,
			"name", PACKAGE,
			"version", VERSION,
			"comments", comments,
			"copyright", copyright,
			"license", license,
			"website", "http://bluefish.openoffice.nl",
			"authors", authors,
			"artists", artists,
			"documenters", documenters,
			"translator_credits", translator_credits,
			NULL);

	gtk_window_set_destroy_with_parent(GTK_WINDOW(about), TRUE);

	gtk_window_present(GTK_WINDOW(about));

	if (logo) g_object_unref(logo);
	g_free (comments);
}


static void about_init(void) {
#ifdef ENABLE_NLS
	bindtextdomain(PACKAGE"_plugin_about", LOCALEDIR);
	bind_textdomain_codeset(PACKAGE"_plugin_about", "UTF-8");
#endif /* ENABLE_NLS */
}

#ifdef ENABLE_NLS
static gchar *about_menu_translate(const gchar * path, gpointer data) {
	return _(path);
}
#endif /* ENABLE_NLS */

static void about_initgui(Tbfwin* bfwin) {
	GtkItemFactory *ifactory;
	static GtkItemFactoryEntry menu_items[] = {
		{N_("/_Help"), NULL, NULL, 0, "<LastBranch>"},
		{N_("/Help/Bluefish _Homepage"), NULL, bluefish_url_show_lcb, 1, "<Item>"},
		{N_("/Help/Report a _Bug"), NULL, bluefish_url_show_lcb, 2, "<Item>"},
		{"/Help/sep1", NULL, NULL, 0, "<Separator>"},
		{N_("/Help/_About..."), NULL, about_dialog_create, 0, "<StockItem>", GTK_STOCK_ABOUT}
	};
	ifactory = gtk_item_factory_from_widget(bfwin->menubar);
#ifdef ENABLE_NLS
	gtk_item_factory_set_translate_func(ifactory, about_menu_translate, "<bluefishmain>", NULL);
#endif
	gtk_item_factory_create_items(ifactory, sizeof(menu_items) / sizeof(menu_items[0]), menu_items, bfwin);
	gtk_widget_show_all(bfwin->menubar);
}

static void about_enforce_session(Tbfwin* bfwin) {
}
static void about_cleanup(void) {
}

static void about_cleanup_gui(Tbfwin *bfwin) {
}

static GList *about_register_globses_config(GList *configlist) {
	return configlist;
}
static GList *about_register_session_config(GList *configlist, Tsessionvars *session) {
	return configlist;
}

static TBluefishPlugin bfplugin = {
	"about dialog",
	BFPLUGIN_VERSION,
	sizeof(Tdocument),
	sizeof(Tsessionvars),
	sizeof(Tglobalsession),
	sizeof(Tbfwin),
	sizeof(Tproject),
	sizeof(Tmain),
	NULL, /* private */
	about_init, /* init */
	about_initgui, 
	about_enforce_session,
	about_cleanup,
	about_cleanup_gui,
	about_register_globses_config,
	about_register_session_config,
	NULL, /* binary compatibility */
	NULL,
	NULL,
	NULL
};

G_MODULE_EXPORT TBluefishPlugin *getplugin(void) {
	return &bfplugin;
}


