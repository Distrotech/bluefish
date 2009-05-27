/* Bluefish HTML Editor
 * bluefish.c - the main function
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * Copyright (C) 1999-2009 Olivier Sessink
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

/*#define DEBUG */

#include <gtk/gtk.h>
#include <stdlib.h>				/* exit() on Solaris */
#include <time.h>				/* nanosleep */

#include "bluefish.h"
#include <libxml/parser.h>

#ifdef ENABLE_NLS
#include <locale.h>
#endif							/* ENABLE_NLS */

#include "bftextview2_langmgr.h"
#include "pixmap.h"				/* set_default_icon() */
#include "bf_lib.h"				/* create_full_path() */
#include "bookmark.h"			/* bmark_init() */
#include "dialog_utils.h"		/* message_dialog_new() */
#include "document.h"
#include "filebrowser2.h"
#include "file_dialogs.h"
#include "gtk_easy.h"			/* flush_queue() */
#include "gui.h"				/* gui_create_main() */
#include "msg_queue.h"			/* msg_queue_start() */
#include "plugins.h"
#include "project.h"
#include "rcfile.h"				/* rcfile_parse_main() */
#include "stringlist.h"			/* put_stringlist(), get_stringlist() */
#include "filefilter.h"
#include "file.h" /* file_handle() */
#include "file_autosave.h"

/*********************************************/
/* this var is global for all bluefish files */
/*********************************************/
Tmain *main_v;

/********************************/
/* functions used in bluefish.c */
/********************************/
#ifndef __GNUC__
void g_none(gchar * first, ...)
{
	return;
}
#endif							/* __GNUC__ */

void cb_print_version(const gchar * option_name, const gchar * value, gpointer data,
					  GError ** error)
{
	g_print("%s %s\n%s\n",_("Bluefish Editor"),VERSION,_("Visit us at http://bluefish.openoffice.nl"));
	exit(0);
}

/*********************/
/* the main function */
/*********************/

int main(int argc, char *argv[])
{
	gboolean curwindow = FALSE;
	gchar **files = NULL;
	GList *filenames = NULL;
	Tbfwin *firstbfwin;

	GError *error = NULL;

#ifndef NOSPLASH
	GtkWidget *splash_window = NULL;
#endif							/* NOSPLASH */

	GOptionContext *context;
	const GOptionEntry options[] = {
		{"curwindow", 'c', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, &curwindow,
		 N_("Open in current window."), NULL},
		{"version", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, (void *) cb_print_version,
		 N_("Print version information."), NULL},
		{G_OPTION_REMAINING, 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_FILENAME_ARRAY, &files,
		 "Special option that collects any remaining arguments for us"},
		{NULL}
	};

#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	DEBUG_MSG("set bindtextdomain for %s to %s\n", PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	textdomain(PACKAGE);
#endif							/* ENABLE_NLS */

	if (!g_thread_supported())
		g_thread_init(NULL);
/*  gdk_threads_init ();*/

	gtk_rc_parse_string ("style \"bluefish-small-close-button-style\"\n"
                       "{\n"
                          "GtkWidget::focus-padding = 0\n"
                          "GtkWidget::focus-line-width = 0\n"
                          "xthickness = 0\n"
                          "ythickness = 0\n"
                       "}\n"
                       "widget \"*.bluefish-small-close-button\" style \"bluefish-small-close-button-style\"");

	context = g_option_context_new(_(" [FILE(S)]"));
#ifdef ENABLE_NLS
	g_option_context_add_main_entries(context, options, PACKAGE);
#else
	g_option_context_add_main_entries(context, options, NULL);
#endif							/* ENABLE_NLS */

	/* this one or bail out with the error of g_option_context_parse () */
	g_option_context_set_ignore_unknown_options(context, TRUE);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_error(N_("Error parsing command line options. %s\nPlease run: %s -?\n"), error->message,
				argv[0]);
	}

	g_option_context_free(context);
	if (error)
		g_error_free(error);

	xmlInitParser();

	main_v = g_new0(Tmain, 1);
	DEBUG_MSG("main, main_v is at %p\n", main_v);
	rcfile_check_directory();
	rcfile_parse_main();

	if (files != NULL) {
		gchar **tmp = files;
		while (*tmp) {
			GFile *tmpfile = g_file_new_for_commandline_arg(*tmp);
			filenames = g_list_append(filenames, tmpfile);
			DEBUG_MSG("main, file=%s\n", *tmp);
			tmp++;
		}
		g_strfreev(files);
	}
#ifdef WITH_MSG_QUEUE
	/* start message queue as early as possible so a running bluefish process has a lot of 
	time to respond to our request-alive request */
	if (main_v->props.open_in_running_bluefish) {
		msg_queue_start(filenames, !curwindow);
	}
#endif							/* WITH_MSG_QUEUE */
#ifndef NOSPLASH
	if (main_v->props.show_splash_screen) {
		/* start splash screen somewhere here */
		splash_window = start_splash_screen();
		splash_screen_set_label(_("loading plugins..."));
	}
#endif							/* NOSPLASH */

/*  {
    gchar *filename = g_strconcat(g_get_home_dir(), "/."PACKAGE"/dir_history", NULL);
    main_v->recent_directories = get_stringlist(filename, NULL);
    g_free(filename);
  }*/
	bluefish_load_plugins();
	main_v->session = g_new0(Tsessionvars, 1);
	main_v->session->view_main_toolbar = main_v->session->view_left_panel =
		main_v->session->filebrowser_focus_follow = main_v->session->view_statusbar = 1;
	main_v->session->snr_position_x = main_v->session->snr_position_y = -1;
	rcfile_parse_global_session();
	if (main_v->session->recent_dirs == NULL) {
		main_v->session->recent_dirs =
			g_list_append(main_v->session->recent_dirs,
						  g_strconcat("file://", g_get_home_dir(), NULL));
	}
#ifndef NOSPLASH
	if (main_v->props.show_splash_screen)
		splash_screen_set_label(_("reading language files..."));
#endif							/* NOSPLASH */
	langmgr_init();
#ifndef NOSPLASH
	if (main_v->props.show_splash_screen)
		splash_screen_set_label(_("building file browser ..."));
#endif							/* NOSPLASH */
	set_default_icon();
	fb2config_init();			/* filebrowser2config */
	filters_rebuild();
	main_v->tooltips = gtk_tooltips_new();
	regcomp(&main_v->find_encoding,
			"<meta[ \t\n\r\f]http-equiv[ \t\n\r\f]*=[ \t\n\r\f]*\"content-type\"[ \t\n\r\f]+content[ \t\n\r\f]*=[ \t\n\r\f]*\"text/x?html;[ \t\n\r\f]*charset=([a-z0-9_-]+)\"[ \t\n\r\f]*/?>",
			REG_EXTENDED | REG_ICASE);
	main_v->bmarkdata = bookmark_data_new();
#ifdef WITH_MSG_QUEUE
	if (main_v->props.open_in_running_bluefish) {
#ifndef NOSPLASH
		if (main_v->props.show_splash_screen)
			splash_screen_set_label(_("checking for running bluefish process ..."));
#endif							/* NOSPLASH */
		msg_queue_check_server(FALSE);
	}
#endif							/* WITH_MSG_QUEUE */
#ifndef NOSPLASH
	if (main_v->props.show_splash_screen)
		splash_screen_set_label(_("creating main gui..."));
#endif							/* NOSPLASH */
	/* create the first window */
	firstbfwin = g_new0(Tbfwin, 1);
	firstbfwin->session = main_v->session;
	firstbfwin->bmarkdata = main_v->bmarkdata;
	main_v->bfwinlist = g_list_append(NULL, firstbfwin);
	gui_create_main(firstbfwin);
#ifdef WITH_MSG_QUEUE
	if (main_v->props.open_in_running_bluefish) {
		msg_queue_check_server(FALSE);
	}
#endif
	file_static_queues_init();
	if (filenames) {
		GList *tmplist = g_list_first(filenames);
		DEBUG_MSG("main, we have filenames, load them\n");
		firstbfwin->focus_next_new_doc = TRUE;
		while (tmplist) {
			file_handle((GFile *)tmplist->data, firstbfwin);
			tmplist = g_list_next(tmplist);
		}
	}
	bmark_reload(firstbfwin);

#ifndef NOSPLASH
	if (main_v->props.show_splash_screen)
		splash_screen_set_label(_("showing main gui..."));
#endif							/* NOSPLASH */
	/* set GTK settings, must be AFTER the menu is created */
	{
		gchar *shortcutfilename;
		GtkSettings *gtksettings = gtk_settings_get_default();
		g_object_set(G_OBJECT(gtksettings), "gtk-can-change-accels", TRUE, NULL);
		shortcutfilename = g_strconcat(g_get_home_dir(), "/." PACKAGE "/menudump_2", NULL);
		gtk_accel_map_load(shortcutfilename);
		g_free(shortcutfilename);
	}
#ifdef WITH_MSG_QUEUE
	if (main_v->props.open_in_running_bluefish) {
		msg_queue_check_server(TRUE);
	}
#endif
	autosave_init(TRUE, firstbfwin);
	gui_show_main(firstbfwin);
#ifndef NOSPLASH
	if (main_v->props.show_splash_screen) {
		/*static struct timespec const req = { 0, 10000000}; */
		flush_queue();
		/*nanosleep(&req, NULL); */
		gtk_widget_destroy(splash_window);
	}
#endif							/* NOSPLASH */
	DEBUG_MSG("main, before gtk_main()\n");
/*  gdk_threads_enter ();*/
	gtk_main();
/*  gdk_threads_leave ();*/
	DEBUG_MSG("main, after gtk_main()\n");
	autosave_cleanup();
#ifdef WITH_MSG_QUEUE
	/* do the cleanup */
	msg_queue_cleanup();
#endif							/* WITH_MSG_QUEUE */
	DEBUG_MSG("calling fb2config_cleanup()\n");
	fb2config_cleanup();
	langmgr_cleanup();
	regfree(&main_v->find_encoding);
	DEBUG_MSG("Bluefish: exiting cleanly\n");

	return 0;
}

void bluefish_exit_request()
{
	GList *tmplist;
	gboolean tmpb;
	DEBUG_MSG("bluefish_exit_request, started\n");
	/* if we have modified documents we have to do something, file_close_all_cb()
	   does exactly want we want to do */
	tmplist = return_allwindows_documentlist();
	tmpb = (tmplist && have_modified_documents(tmplist));
	g_list_free(tmplist);
	tmplist = g_list_first(main_v->bfwinlist);
	while (tmplist) {
		main_window_delete_event_lcb(NULL,NULL,BFWIN(tmplist->data));
/*		if (tmpb) {
			file_close_all_cb(NULL, BFWIN(tmplist->data));
		}*/
		tmplist = g_list_next(tmplist);
	}
	/* if we still have modified documents we don't do a thing,
	   if we don't have them we can quit */
	if (tmpb) {
		tmplist = return_allwindows_documentlist();
		tmpb = (tmplist && have_modified_documents(tmplist));
		g_list_free(tmplist);
		if (tmpb) {
			return;
		}
	}

	tmplist = g_list_first(gtk_window_list_toplevels());
	while (tmplist) {
		gtk_widget_hide(GTK_WIDGET(tmplist->data));
		tmplist = g_list_next(tmplist);
	}
	flush_queue();

	rcfile_save_global_session();

	gtk_main_quit();
	DEBUG_MSG("bluefish_exit_request, after gtk_main_quit()\n");
}
