/* Bluefish HTML Editor
 * bluefish.c - the main function
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * Copyright (C) 1999-2002 Olivier Sessink
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <gtk/gtk.h>
#include <unistd.h> /* getopt() */
#include <stdlib.h> /* getopt() exit() and abort() on Solaris */

#include "bluefish.h"
#include "document.h" /*  */
#include "gui.h" /* gui_create_main() */
#include "rcfile.h" /* rcfile_parse_main() */
#include "bf_lib.h" /* create_full_path() */
#include "highlight.h" /* hl_init() */
#include "msg_queue.h" /* msg_queue_start()*/
#include "stringlist.h" /* put_stringlist(), get_stringlist() */
#include "gtk_easy.h" /* flush_queue() */

/*********************************************/
/* this var is global for all bluefish files */
/*********************************************/
Tmain *main_v;

/********************************/
/* functions used in bluefish.c */
/********************************/

static gint parse_commandline(int argc, char **argv
		, gboolean *root_override
		, GList **load_filenames) {
	int c;
	gchar *tmpname;

	opterr = 0;
	DEBUG_MSG("parse_commandline, started\n");
	while ((c = getopt(argc, argv, "hsv?")) != -1) {
		switch (c) {
		case 's':
			*root_override = 1;
			break;
		case 'v':
			g_print(CURRENT_VERSION_NAME);
			g_print("\n");
			exit(1);
			break;
		case 'h':
		case '?':
			g_print(CURRENT_VERSION_NAME);
			g_print("\nUsage: %s [options] [filename]\n", argv[0]);
			g_print(_("\nCurrently accepted options are:\n"));
			g_print(_("-s           skip root check\n"));
			g_print(_("-v           current version\n"));
			g_print(_("-h           this help screen\n"));
			exit(1);
			break;
		default:
			DEBUG_MSG("parse_commandline, abort ?!?\n");
			abort();
		}
	}
	DEBUG_MSG("parse_commandline, optind=%d, argc=%d\n", optind, argc);
	while (optind < argc) {
		tmpname = create_full_path(argv[optind], NULL);
		DEBUG_MSG("parse_commandline, argv[%d]=%s, tmpname=%s\n", optind, argv[optind], tmpname);
		*load_filenames = g_list_append(*load_filenames, tmpname);
		optind++;
	}
	DEBUG_MSG("parse_commandline, finished\n");
	return 0;
}


/*********************/
/* the main function */
/*********************/

int main(int argc, char *argv[])
{
	gboolean root_override;
	GList *filenames = NULL;
#ifndef NOSPLASH
	GtkWidget *splash_window;
#endif /* #ifndef NOSPLASH */
	gtk_init(&argc, &argv);
	
	main_v = g_new0(Tmain, 1);
	DEBUG_MSG("main, main_v is at %p\n", main_v);
	
	rcfile_check_directory();
	rcfile_parse_main();
	
	parse_commandline(argc, argv, &root_override, &filenames);
	
	if (filenames && main_v->props.open_in_running_bluefish) {
		msg_queue_start(filenames);
	}
	
#ifndef NOSPLASH
	/* start splash screen somewhere here */
	splash_window = start_splash_screen();
	splash_screen_set_label(_("parsing highlighting file..."));
#endif /* #ifndef NOSPLASH */

	{
		gchar *filename = g_strconcat(g_get_home_dir(), "/.bluefish/dir_history", NULL);
		main_v->recent_directories = get_stringlist(filename, NULL);
		g_free(filename);
	}

	rcfile_parse_highlighting();
#ifndef NOSPLASH
	splash_screen_set_label(_("compiling highlighting patterns..."));
#endif /* #ifndef NOSPLASH */
	hl_init();
#ifndef NOSPLASH
	splash_screen_set_label(_("parsing custom menu file..."));
#endif /* #ifndef NOSPLASH */
	rcfile_parse_custom_menu();

	if (!filenames && main_v->props.open_in_running_bluefish) {
		msg_queue_start(NULL);
	}

#ifndef NOSPLASH
	splash_screen_set_label(_("creating main gui..."));
#endif /* #ifndef NOSPLASH */
	gui_create_main(filenames);
#ifndef NOSPLASH
	splash_screen_set_label(_("showing main gui..."));
#endif /* #ifndef NOSPLASH */
	/* set GTK settings, must be AFTER the menu is created */
	{
		GtkSettings* gtksettings = gtk_settings_get_default();
		g_object_set(G_OBJECT(gtksettings), "gtk-can-change-accels", TRUE, NULL); 
	}

	gui_show_main();
#ifndef NOSPLASH
	flush_queue();
	gtk_widget_destroy(splash_window);
#endif /* #ifndef NOSPLASH */

	gtk_main();
	
	/* do the cleanup */
	msg_queue_cleanup();
	return 0;
}

void bluefish_exit_request() {
	/* if we have modified documents we have to do something, file_close_all_cb()
	does exactly want we want to do */
	if (test_docs_modified(NULL)) {
		file_close_all_cb(NULL, NULL);
		/* if we still have modified documents we don't do a thing,
		 if we don't have them we can quit */
		if (test_docs_modified(NULL)) {
			return;
		}
	}

	gtk_widget_hide(main_v->main_window);
	rcfile_save_all();
	{
		gchar *filename = g_strconcat(g_get_home_dir(), "/.bluefish/dir_history", NULL);
		put_stringlist_limited(filename, main_v->recent_directories, main_v->props.max_dir_history);
		g_free(filename);
	}
	gtk_main_quit();
}
