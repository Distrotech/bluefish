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

#include "bluefish.h"
#include "document.h" /*  */
#include "gui.h" /* gui_create_main() */
#include "rcfile.h" /* rcfile_parse_main() */
#include "bf_lib.h" /* create_full_path() */
#include "highlight.h" /* hl_init() */


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
	
	rcfile_parse_main();
	
	parse_commandline(argc, argv, &root_override, &filenames);
	
#ifndef NOSPLASH
	/* start splash screen somewhere here */
	splash_window = start_splash_screen();
	splash_screen_set_label(_("parsing highlighting file..."));
#endif /* #ifndef NOSPLASH */

	rcfile_parse_highlighting();
#ifndef NOSPLASH
	splash_screen_set_label(_("compiling highlighting patterns..."));
#endif /* #ifndef NOSPLASH */
	hl_init();
#ifndef NOSPLASH
	splash_screen_set_label(_("creating custom menu..."));
#endif /* #ifndef NOSPLASH */
	rcfile_parse_custom_menu();
#ifndef NOSPLASH
	splash_screen_set_label(_("creating main gui..."));
#endif /* #ifndef NOSPLASH */
	gui_create_main(filenames);
#ifndef NOSPLASH
	splash_screen_set_label(_("showing main gui..."));
#endif /* #ifndef NOSPLASH */
	gui_show_main();
#ifndef NOSPLASH
	flush_queue();
	gtk_widget_destroy(splash_window);
#endif /* #ifndef NOSPLASH */
	gtk_main();
	return 0;
}

void bluefish_exit_request() {

	file_close_all_cb(NULL, NULL);

	/* check for changed documents here */
	if (test_only_empty_doc_left()) {
		rcfile_save_main();
		gtk_main_quit();
	}
}
