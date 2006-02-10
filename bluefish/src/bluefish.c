/* Bluefish HTML Editor
 * bluefish.c - the main function
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * Copyright (C) 1999-2006 Olivier Sessink
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

/* #define DEBUG */

#include <gtk/gtk.h>
#include <stdlib.h>    /* getopt() exit() and abort() on Solaris */
#include <time.h>      /* nanosleep */
#include <unistd.h>    /* getopt() */

#include "bluefish.h"

#ifdef HAVE_GNOME_VFS
#include <libgnomevfs/gnome-vfs.h>
#endif
#ifdef HAVE_ATLEAST_GNOMEUI_2_6
#include <libgnomeui/libgnomeui.h>
#endif

#ifdef ENABLE_NLS
#include <locale.h>
#endif

#include "authen.h"        /* set_authen_callbacks() */
#include "bf_lib.h"        /* create_full_path() */
#include "bookmark.h"      /* bmark_init() */
#include "document.h"
#include "filebrowser.h"   /* filters_rebuild() */
#include "fref.h"          /* fref_init() */
#include "gtk_easy.h"      /* flush_queue() */
#include "gui.h"           /* gui_create_main() */
#include "highlight.h"     /* hl_init() */
#include "msg_queue.h"     /* msg_queue_start()*/
#include "pixmap.h"        /* default_icon_list() */
#include "project.h"
#include "rcfile.h"        /* rcfile_parse_main() */
#include "stringlist.h"    /* put_stringlist(), get_stringlist() */

/*********************************************/
/* this var is global for all bluefish files */
/*********************************************/
Tmain *main_v;

/********************************/
/* functions used in bluefish.c */
/********************************/
#ifndef __GNUC__
void g_none(gchar *first, ...) {
	return;
}
#endif

static gint parse_commandline(int argc, char **argv
		, gboolean *root_override
		, GList **load_filenames
		, GList **load_projects
		, gboolean *open_in_new_win) {
	int c;
	gchar *tmpname;

	opterr = 0;
	DEBUG_MSG("parse_commandline, started\n");
	while ((c = getopt(argc, argv, "hsvnp:?")) != -1) {
		switch (c) {
		case 's':
			*root_override = 1;
			break;
		case 'v':
			g_print(CURRENT_VERSION_NAME);
			g_print("\n");
			exit(1);
			break;
		case 'p':
			tmpname = create_full_path(optarg, NULL);
			*load_projects = g_list_append(*load_projects, tmpname);
			break;
		case 'h':
		case '?':
			g_print(CURRENT_VERSION_NAME);
			g_print(_("\nUsage: %s [options] [filenames ...]\n"), argv[0]);
			g_print(_("\nCurrently accepted options are:\n"));
			g_print(_("-s           skip root check\n"));
			g_print(_("-v           current version\n"));
			g_print(_("-n           open new window\n"));
			g_print(_("-p filename  open project\n"));
			g_print(_("-h           this help screen\n"));
			exit(1);
			break;
		case 'n':
			*open_in_new_win = 1;
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
	DEBUG_MSG("parse_commandline, finished, num files=%d, num projects=%d\n"
		, g_list_length(*load_filenames), g_list_length(*load_projects));
	return 0;
}


/*********************/
/* the main function */
/*********************/

int main(int argc, char *argv[])
{
	gboolean root_override=FALSE, open_in_new_window=FALSE;
	GList *filenames = NULL, *projectfiles=NULL;
	Tbfwin *firstbfwin;
#ifndef NOSPLASH
	GtkWidget *splash_window = NULL;
#endif /* #ifndef NOSPLASH */

#ifdef ENABLE_NLS                                                               
	setlocale(LC_ALL,"");                                                   
	bindtextdomain(PACKAGE,LOCALEDIR);
	DEBUG_MSG("set bindtextdomain for %s to %s\n",PACKAGE,LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	textdomain(PACKAGE);                                                    
#endif
#ifdef HAVE_ATLEAST_GNOMEUI_2_6
	gnome_init(PACKAGE, VERSION, argc, argv);
#else
	gtk_init(&argc, &argv);
#endif /* HAVE_ATLEAST_GNOMEUI_2_6
 */
#ifdef HAVE_GNOME_VFS
	DEBUG_MSG("main, we have gnome_vfs, so we init it\n");
	gnome_vfs_init();
#ifdef HAVE_ATLEAST_GNOMEUI_2_6
	gnome_authentication_manager_init();
#else
#ifdef HAVE_ATLEAST_GNOMEVFS_2_6
	set_authen_callbacks();
#endif /* HAVE_ATLEAST_GNOMEVFS_2_6 */
#endif /* HAVE_ATLEAST_GNOME_2_6 */
#endif /* HAVE_GNOME_VFS */
    gtk_window_set_default_icon_list (default_icon_list());
	main_v = g_new0(Tmain, 1);
	main_v->session = g_new0(Tsessionvars,1);
	main_v->session->view_html_toolbar = main_v->session->view_main_toolbar = main_v->session->view_custom_menu = main_v->session->view_left_panel = 1;
	DEBUG_MSG("main, main_v is at %p\n", main_v);

	rcfile_check_directory();
	rcfile_parse_main();
	
	parse_commandline(argc, argv, &root_override, &filenames, &projectfiles, &open_in_new_window);
#ifdef WITH_MSG_QUEUE	
	if (((filenames || projectfiles) && main_v->props.open_in_running_bluefish) ||  open_in_new_window) {
		msg_queue_start(filenames, projectfiles, open_in_new_window);
	}
#endif /* WITH_MSG_QUEUE */
#ifndef NOSPLASH
	if (main_v->props.show_splash_screen) {
		/* start splash screen somewhere here */
		splash_window = start_splash_screen();
		splash_screen_set_label(_("parsing highlighting file..."));
	}
#endif /* #ifndef NOSPLASH */

	{
		gchar *filename = g_strconcat(g_get_home_dir(), "/.bluefish/dir_history", NULL);
		main_v->recent_directories = get_stringlist(filename, NULL);
		g_free(filename);
	}
	rcfile_parse_global_session();
	rcfile_parse_highlighting();
#ifndef NOSPLASH
	if (main_v->props.show_splash_screen) splash_screen_set_label(_("compiling highlighting patterns..."));
#endif /* #ifndef NOSPLASH */
	hl_init();
	filebrowserconfig_init();
	filebrowser_filters_rebuild();
	autoclosing_init();
#ifndef NOSPLASH
	if (main_v->props.show_splash_screen) splash_screen_set_label(_("parsing custom menu file..."));
#endif /* #ifndef NOSPLASH */
	rcfile_parse_custom_menu(FALSE,FALSE);
	main_v->tooltips = gtk_tooltips_new();
	fref_init();
	bmark_init();
#ifdef WITH_MSG_QUEUE
	if (!filenames && !projectfiles && main_v->props.open_in_running_bluefish) {
		msg_queue_start(NULL, NULL, open_in_new_window);
	}
#endif /* WITH_MSG_QUEUE */
#ifndef NOSPLASH
	if (main_v->props.show_splash_screen) splash_screen_set_label(_("creating main gui..."));
#endif /* #ifndef NOSPLASH */

	/* create the first window */
	firstbfwin = g_new0(Tbfwin,1);
	firstbfwin->session = main_v->session;
	firstbfwin->bookmarkstore = main_v->bookmarkstore;
	main_v->bfwinlist = g_list_append(NULL, firstbfwin);
	gui_create_main(firstbfwin,filenames);
	bmark_reload(firstbfwin);
#ifndef NOSPLASH
	if (main_v->props.show_splash_screen) splash_screen_set_label(_("showing main gui..."));
#endif /* #ifndef NOSPLASH */
	/* set GTK settings, must be AFTER the menu is created */
	{
		gchar *shortcutfilename;
		GtkSettings* gtksettings = gtk_settings_get_default();
		g_object_set(G_OBJECT(gtksettings), "gtk-can-change-accels", TRUE, NULL); 
		shortcutfilename = g_strconcat(g_get_home_dir(), "/.bluefish/menudump_2", NULL);
		gtk_accel_map_load(shortcutfilename);
		g_free(shortcutfilename);
	}

	gui_show_main(firstbfwin);
	if (firstbfwin->session->view_html_toolbar && 
         main_v->globses.quickbar_items == NULL && 
         main_v->props.show_quickbar_tip == TRUE) 
   {	
	   GtkWidget *dialog, *hbox, *image, *vbox, *label, *button, *show_again;
	   
	   dialog = gtk_dialog_new_with_buttons ("", 
	                                         GTK_WINDOW (firstbfwin->main_window),
	                                         GTK_DIALOG_DESTROY_WITH_PARENT,
	                                         NULL);
		button = gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
		gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
		
		hbox = gtk_hbox_new (FALSE, 12);
		gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
		gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox);
		image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
		gtk_misc_set_alignment (GTK_MISC (image), 0 ,0);
		gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
		vbox = gtk_vbox_new (FALSE, 0);
		gtk_box_pack_start_defaults (GTK_BOX (hbox), vbox);
		label = gtk_label_new (NULL);
		gtk_label_set_markup (GTK_LABEL (label), 
									 _("<span weight=\"bold\" size=\"larger\">Bluefish Tip:</span>\n\nThis message is shown if you do not have any items in the Quickbar.\n\nTo add buttons to the Quickbar, right click on a button in the HTML toolbars.\n"));
		gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
		gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

	   show_again = gtk_check_button_new_with_mnemonic (_("_Don't show this dialog again."));
	   gtk_box_pack_start (GTK_BOX (vbox), show_again, FALSE, FALSE, 12);
	   
	   gtk_widget_show_all (GTK_DIALOG (dialog)->vbox);
		gtk_widget_grab_focus (button);
	   gtk_dialog_run (GTK_DIALOG (dialog));
	   if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (show_again))) {
	      main_v->props.show_quickbar_tip = FALSE;
	   }
	   gtk_widget_destroy (dialog);
	}
	
	if (projectfiles) {
		GList *tmplist = g_list_first(projectfiles);
		while (tmplist) {
			project_open_from_file(firstbfwin, tmplist->data);
			tmplist = g_list_next(tmplist);
		}
	}
	
#ifndef NOSPLASH
	if (main_v->props.show_splash_screen) {
		static struct timespec const req = { 0, 10000000};
		flush_queue();
		nanosleep(&req, NULL);
		gtk_widget_destroy(splash_window);
	}
#endif /* #ifndef NOSPLASH */
	DEBUG_MSG("main, before gtk_main()\n");
	gtk_main();
	DEBUG_MSG("main, after gtk_main()\n");
#ifdef WITH_MSG_QUEUE	
	/* do the cleanup */
	msg_queue_cleanup();
#endif /* WITH_MSG_QUEUE */
	DEBUG_MSG("Bluefish: exiting cleanly\n");
	return 0;
}

void bluefish_exit_request() {
	GList *tmplist;
	gboolean tmpb;
	DEBUG_MSG("bluefish_exit_request, started\n");
	/* if we have modified documents we have to do something, file_close_all_cb()
	does exactly want we want to do */
	tmplist = return_allwindows_documentlist();
	tmpb = (tmplist && test_docs_modified(tmplist));
	g_list_free(tmplist);
	tmplist = g_list_first(main_v->bfwinlist);
	while (tmplist) {
		/* if there is a project, we anyway want to save & close the project */
		if (BFWIN(tmplist->data)->project) {
			if (!project_save_and_close(BFWIN(tmplist->data))) {
				/* cancelled or error! */
				DEBUG_MSG("bluefish_exit_request, project_save_and_close returned FALSE\n");
				return;
			}
		}
		if (tmpb) {
			file_close_all_cb(NULL, BFWIN(tmplist->data));
		}
		tmplist = g_list_next(tmplist);
	}
	/* if we still have modified documents we don't do a thing,
	 if we don't have them we can quit */
	if (tmpb) {
		tmplist = return_allwindows_documentlist();
		tmpb = (tmplist && test_docs_modified(tmplist));
		g_list_free(tmplist);
		if (tmpb) {
			return;
		}
	}
/*	gtk_widget_hide(main_v->main_window);*/
	tmplist = g_list_first(gtk_window_list_toplevels());
	while (tmplist) {
		gtk_widget_hide(GTK_WIDGET(tmplist->data));
		tmplist = g_list_next(tmplist);
	}
	flush_queue();
	
	rcfile_save_all();
	{
		gchar *filename = g_strconcat(g_get_home_dir(), "/.bluefish/dir_history", NULL);
		put_stringlist_limited(filename, main_v->recent_directories, main_v->props.max_dir_history);
		g_free(filename);
	}
	
/*	NEEDS TO BE PORTED FOR MULTIPLE-WINDOW SUPPORT
	tmplist = gtk_window_list_toplevels();
	g_list_foreach(tmplist, (GFunc)g_object_ref, NULL);
	tmplist = g_list_first(tmplist);
	while (tmplist) {
		if (tmplist->data != main_v->main_window) {
			gtk_widget_destroy(GTK_WIDGET(tmplist->data));
		}
		tmplist = g_list_next(tmplist);
	}
	g_list_foreach (tmplist, (GFunc)g_object_unref, NULL); */
	
	/* I don't understand why, but if I call gtk_main_quit here, the main() function does not continue after gtk_main(), very weird, so I'll call exit() here */
	gtk_main_quit();
	DEBUG_MSG("bluefish_exit_request, after gtk_main_quit()\n");
#ifdef WITH_MSG_QUEUE	
	/* do the cleanup */
	msg_queue_cleanup();
#endif /* WITH_MSG_QUEUE */
	DEBUG_MSG("Bluefish: exiting cleanly\n");
	exit(0);
}
