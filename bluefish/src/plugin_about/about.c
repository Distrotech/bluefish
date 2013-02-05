/* Bluefish HTML Editor
 * about.c - the About dialog
 *
 * Copyright (C) 2004 Eugene Morenko(More) more@irpin.com
 * Copyright (C) 2008-2011 Olivier Sessink
 * Copyright (C) 2011 James Hayward
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

#include <gmodule.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>				/* getopt() */

#ifdef WIN32
#include <windows.h>
#include <shellapi.h>			/* ShellExecute */
#endif							/* WIN32 */

#include "about.h"
#include "about_rev.h"

#include "../config.h"
#include "../plugins.h"
#include "../bluefish.h"		/* BLUEFISH_SPLASH_FILENAME */


static void
bluefish_url_show(const gchar * url)
{
#ifdef WIN32
	if ((int) ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL) <= 32) {
		g_warning("Failed trying to launch URL in about dialog");
	}
#else
	GError *error = NULL;

	g_app_info_launch_default_for_uri(url, NULL, &error);
	if (error) {
		g_warning("bluefish_url_show, %s", error->message);
		g_error_free(error);
	}
#endif
}

static void
about_options_dialog_create(GtkAction * action, gpointer user_data)
{
	GtkWidget *dialog;
	gchar *sec_text;

	dialog =
		gtk_message_dialog_new(GTK_WINDOW(BFWIN(user_data)->main_window), GTK_DIALOG_DESTROY_WITH_PARENT,
							   GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
#ifdef SVN_REVISION
							   PACKAGE_STRING " rev" SVN_REVISION);
#else	/* SVN_REVISION */
							   PACKAGE_STRING);
#endif	/* SVN_REVISION */
	sec_text = g_strconcat(_("This version of Bluefish was built with:\n"), CONFIGURE_OPTIONS, NULL);
	
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), 
			"%s\ngtk %d.%d.%d (runtime gtk %d.%d.%d)\nglib %d.%d.%d (runtime %d.%d.%d)\n"
			"with libenchant... %s\nwith libenchant >= 1.4... %s\n"
			"with libgucharmap... %s\nwith libgucharmap_2... %s\n"
			"with python... %s"
			, sec_text
			, GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION
			, gtk_major_version, gtk_minor_version, gtk_micro_version
			, GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION
			, glib_major_version, glib_minor_version, glib_micro_version
#ifdef HAVE_LIBENCHANT
			, "yes"
#else
			, "no"
#endif
#ifdef HAVE_LIBENCHANT_1_4
			, "yes"
#else
			, "no"
#endif
#ifdef HAVE_LIBGUCHARMAP
			, "yes"
#else
			, "no"
#endif
#ifdef HAVE_LIBGUCHARMAP_2
			, "yes"
#else
			, "no"
#endif
#ifdef HAVE_PYTHON
			, "yes"
#else
			, "no"
#endif
			);
	
	g_free(sec_text);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void
about_report_bug(GtkAction * action, gpointer user_data)
{
	GString *string;
	gchar *options;

	string = g_string_new("http://bugzilla.gnome.org/enter_bug.cgi?product=bluefish");
#ifdef WIN32
	string = g_string_append(string, ";op_sys=Windows");
#endif	/* WIN32 */
	string = g_string_append(string, ";version=");
#ifdef SVN_REVISION
	string = g_string_append(string, "development (SVN TRUNK)");
#else	/* SVN_REVISION */
	string = g_string_append(string, PACKAGE_VERSION);
#endif	/* SVN_REVISION */
	string = g_string_append(string, ";comment=");

	options = g_strconcat(
#ifdef SVN_REVISION
						  "SVN revision ", SVN_REVISION, "\n\n"
#endif	/* SVN_REVISION */
						  "Bluefish was configured with: ", CONFIGURE_OPTIONS, "\n", NULL);

	string = g_string_append_uri_escaped(string, options, NULL, FALSE);
	g_free(options);

	bluefish_url_show(string->str);
	g_string_free(string, TRUE);
}

static void
about_show_homepage(GtkAction * action, gpointer user_data)
{
	bluefish_url_show("http://bluefish.openoffice.nl");
}

#if !GTK_CHECK_VERSION(3,0,0)
static void
about_activate_url(GtkAboutDialog * about, const gchar * url, gpointer data)
{
	bluefish_url_show(url);
}
#endif

static void
about_dialog_create(GtkAction * action, gpointer user_data)
{
	Tbfwin *bfwin = BFWIN(user_data);
	GdkPixbuf *logo;

	const gchar *artists[] = {
		"Dave Lyon",
		NULL
	};

	const gchar *authors[] = {
		"Olivier Sessink <olivier@bluefish.openoffice.nl> (Project leader)",
		"Jim Hayward <jimhayward@linuxexperience.net>",
		"Daniel Leidert <daniel.leidert@wgdd.de>",
		"Shawn Novak <kernel86@gmail.com>",
		_("\nDevelopers of previous releases:"),
		"Alastair Porter <alastair@porter.net.nz>",
		"Antti-Juhani Kaijanaho",
		"Bo Forslund",
		"Chris Mazuc",
		"Christian Tellefsen <chris@tellefsen.net>",
		"David Arno",
		"Eugene Morenko <more@irpin.com>",
		"Gero Takke",
		"Neil Millar",
		"Oskar Świda <swida@aragorn.pb.bialystok.pl>",
		"Pablo De Napoli",
		"Rasmus Toftdahl Olesen <rto@pohldata.dk>",
		"Roland Steinbach <roland@netzblick.de>",
		"Santiago Capel Torres",
		"Yanike Mann <yanikem@gmail.com>",
		_("\nPackage maintainers:"),
		"Debian: Daniel Leidert <daniel.leidert@wgdd.de>",
		"Fink: Michèle Garoche <michele.garoche@easyconnect.fr>, Kevin Horton <khorton01@rogers.com>",
		"Gentoo: Hanno Böck <hanno@gentoo.org>",
		"Mandrake: Todd Lyons <todd@mrball.net>",
		"Redhat: Matthias Haase <matthias_haase@bennewitz.com>",
		"Windows: Shawn Novak <kernel86@gmail.com>, Daniel Leidert <daniel.leidert@wgdd.de>",
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

	const gchar *copyright = "Copyright \xc2\xa9 1998-2012 Olivier Sessink and others.\n";

	/* wrap the license here,
	 * the "wrap-license" property is only available with GTK >= 2.8
	 */
	const gchar *license =
		"This program is free software; you can redistribute it and/or modify it "
		"under the terms of the GNU General Public License as published by "
		"the Free Software Foundation; either version 3 of the License, or "
		"(at your option) any later version.\n"
		"\n"
		"This program is distributed in the hope that it will be useful, "
		"but WITHOUT ANY WARRANTY; without even the implied warranty of "
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
		"GNU General Public License for more details.\n"
		"\n"
		"You should have received a copy of the GNU General Public License "
		"along with this program.  If not, see <http://www.gnu.org/licenses/>.";

	const gchar *comments =
		_
		("An open-source editor for experienced web designers and programmers, supporting many programming and markup languages, but focusing on creating dynamic and interactive websites.");

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
		GError *error = NULL;
		logo = gdk_pixbuf_new_from_file(BLUEFISH_SPLASH_FILENAME, &error);
		if (error) {
			g_print("ERROR while loading splash screen: %s\n", error->message);
			g_error_free(error);
		}
	}

#if !GTK_CHECK_VERSION(3, 0, 0)
	gtk_about_dialog_set_url_hook(about_activate_url, NULL, NULL);
#endif /* gtk3 */

	gtk_show_about_dialog(GTK_WINDOW(bfwin->main_window), "logo", logo, "name", PACKAGE,
#ifdef SVN_REVISION
						  "version", VERSION " rev" SVN_REVISION,
#else	/* SVN_REVISION */
						  "version", VERSION,
#endif	/* SVN_REVISION */
						  "comments", comments,
						  "copyright", copyright,
						  "license", license,
						  "website", "http://bluefish.openoffice.nl",
						  "authors", authors,
						  "artists", artists,
						  "documenters", documenters,
						  "translator_credits", translator_credits,
						  "wrap-license", TRUE,
						  NULL);

	if (logo)
		g_object_unref(logo);
}

static void
about_init(void)
{
#ifdef ENABLE_NLS
	DEBUG_MSG("about_init, gettext domain-name=%s\n", PACKAGE "_plugin_about");
	bindtextdomain(PACKAGE "_plugin_about", LOCALEDIR);
	bind_textdomain_codeset(PACKAGE "_plugin_about", "UTF-8");
#endif							/* ENABLE_NLS */
}

static const gchar *about_plugin_ui =
	"<ui>"
	"  <menubar name='MainMenu'>"
	"    <menu action='HelpMenu'>"
	"      <menuitem action='HelpHomepage'/>"
	"      <menuitem action='HelpReportBug'/>"
	"      <separator/>"
	"      <menuitem action='HelpAbout'/>"
	"      <menuitem action='HelpBuildInfo'/>"
	"    </menu>"
	"  </menubar>"
	"</ui>";

static void
about_initgui(Tbfwin * bfwin)
{
	GtkActionGroup *action_group;
	GError *error = NULL;

	static const GtkActionEntry about_actions[] = {
		{"HelpMenu", NULL, N_("_Help")},
		{"HelpHomepage", GTK_STOCK_HOME, N_("Bluefish _Homepage"), NULL, N_("Bluefish homepage"),
		 G_CALLBACK(about_show_homepage)},
		{"HelpReportBug", NULL, N_("Report a _Bug"), NULL, N_("Report a bug"), G_CALLBACK(about_report_bug)},
		{"HelpAbout", GTK_STOCK_ABOUT, N_("_About"), NULL, N_("About Bluefish"),
		 G_CALLBACK(about_dialog_create)},
		{"HelpBuildInfo", GTK_STOCK_INFO, N_("Build _Info"), NULL, N_("Build info"),
		 G_CALLBACK(about_options_dialog_create)}
	};

	action_group = gtk_action_group_new("AboutActions");
	gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE "_plugin_about");
	gtk_action_group_add_actions(action_group, about_actions, G_N_ELEMENTS(about_actions), bfwin);
	gtk_ui_manager_insert_action_group(bfwin->uimanager, action_group, 0);
	g_object_unref(action_group);

	gtk_ui_manager_add_ui_from_string(bfwin->uimanager, about_plugin_ui, -1, &error);
	if (error != NULL) {
		g_warning("building about plugin menu failed: %s", error->message);
		g_error_free(error);
	}
}

/*static void
about_enforce_session(Tbfwin * bfwin)
{
}
*/
/*static void
about_cleanup(void)
{
}
*/
/*static void
about_cleanup_gui(Tbfwin * bfwin)
{
}
*/
/*static GHashTable *
about_register_globses_config(GHashTable * configlist)
{
	return configlist;
}*/

/*static GHashTable *
about_register_session_config(GHashTable * configlist, Tsessionvars * session)
{
	return configlist;
}
*/
G_MODULE_EXPORT TBluefishPlugin *
getplugin(void)
{
	static TBluefishPlugin bfplugin = {
		"About Dialog",
		BFPLUGIN_VERSION,
		GTK_MAJOR_VERSION,
		sizeof(Tdocument),
		sizeof(Tsessionvars),
		sizeof(Tglobalsession),
		sizeof(Tbfwin),
		sizeof(Tproject),
		sizeof(Tmain),
		sizeof(Tproperties),
		BFPLUGIN_PRIORITY_LAST,
		1,
		NULL,						/* private */
		about_init,					/* init */
		about_initgui,
		NULL /*about_enforce_session*/,
		NULL /*about_cleanup*/,
		NULL /*about_cleanup_gui*/,
		NULL /*about_register_globses_config*/,
		NULL /*about_register_session_config*/,
		NULL,						/* binary compatibility */
		NULL,
		NULL,
		NULL
	};
	return &bfplugin;
}
