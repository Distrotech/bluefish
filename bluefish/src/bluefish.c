/* Bluefish HTML Editor
 * bluefish.c - the main function
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * Copyright (C) 1999-2007 Olivier Sessink
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
#include <stdlib.h> /* exit() on Solaris */
#include <time.h> /* nanosleep */

#include "bluefish.h"
#include <libxml/parser.h>

#ifdef HAVE_LIBGNOMEUI_LIBGNOMEUI_H
#include <libgnomeui/libgnomeui.h>
#ifndef GNOME_PARAM_GOPTION_CONTEXT
#include <popt.h>
#endif /* GNOME_PARAM_GOPTION_CONTEXT */
#endif /* HAVE_LIBGNOMEUI_LIBGNOMEUI_H */

#ifdef ENABLE_NLS
#include <locale.h>
#endif /* ENABLE_NLS */

#include "pixmap.h"        /* set_default_icon() */
#include "bf_lib.h"        /* create_full_path() */
#include "bookmark.h"      /* bmark_init() */
#include "dialog_utils.h"  /* message_dialog_new() */
#include "document.h"
#include "filebrowser2.h"
#include "filetype.h"
#include "file_dialogs.h"
#include "gtk_easy.h"      /* flush_queue() */
#include "gui.h"           /* gui_create_main() */
#include "msg_queue.h"     /* msg_queue_start()*/
#include "plugins.h"
#include "project.h"
#include "rcfile.h"        /* rcfile_parse_main() */
#include "stringlist.h"    /* put_stringlist(), get_stringlist() */
#include "textstyle.h"
#include "filefilter.h"

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
#endif /* __GNUC__ */

void cb_print_version (const gchar *option_name, const gchar *value, gpointer data, GError **error)
{
	char *version;
	
	version = g_strconcat(_("Bluefish Editor"), " ", VERSION, "\n",
	                      _("Visit us at http://bluefish.openoffice.nl"), "\n",
	                      NULL);
	g_print(version);
	g_free(version);
	exit(0);
}

/*********************/
/* the main function */
/*********************/

int main(int argc, char *argv[])
{
	gboolean newwindow = FALSE, skiprootcheck = FALSE, root_override=FALSE, open_in_new_window=FALSE;
	gchar **files = NULL, *project = NULL, *tmpname;
	gint filearray, i;
	GList *filenames = NULL, *projectfiles=NULL;
	Tbfwin *firstbfwin;
#ifdef HAVE_ATLEAST_GIO_2_16
	GFile *tmpfile;
#endif

#ifdef HAVE_LIBGNOMEUI_LIBGNOMEUI_H
	GnomeProgram *bfprogram;
#else /* HAVE_LIBGNOMEUI_LIBGNOMEUI_H */
	GError *error = NULL;
#endif /* HAVE_LIBGNOMEUI_LIBGNOMEUI_H */

#ifndef NOSPLASH
	GtkWidget *splash_window = NULL;
#endif /* NOSPLASH */

#if !defined (HAVE_LIBGNOMEUI_LIBGNOMEUI_H) || defined (GNOME_PARAM_GOPTION_CONTEXT)
	GOptionContext *context;
	const GOptionEntry options[] = {
		{"newwindow", 'n', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, &newwindow, N_("Open in a new window."), NULL},
		{"project", 'p', G_OPTION_FLAG_FILENAME, G_OPTION_ARG_FILENAME, &project, N_("Open a project from the specified projectfile."), N_("PROJECTFILE")},
		{"skiprootcheck", 's', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, &skiprootcheck, N_("Skip root check."), NULL},
		{"Version", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, (void*) cb_print_version, N_("Print version information."), NULL},
		{G_OPTION_REMAINING, 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_FILENAME_ARRAY, &files, "Special option that collects any remaining arguments for us"},
		{NULL}
};
#else /* GNOME_PARAM_GOPTION_CONTEXT */
	poptContext pcontext;	
	const struct poptOption options[] = {
		{"newwindow", 'n', POPT_ARG_NONE, &newwindow, 0, N_("Open in a new window."), NULL},
		{"project", 'p', POPT_ARG_STRING, &project, 0, N_("Open a project from the specified projectfile."), N_("PROJECTFILE")},
		{"skiprootcheck", 's', POPT_ARG_NONE, &skiprootcheck, 0, N_("Skip root check."), NULL},
		{"Version", 'v', POPT_ARG_CALLBACK, (void*) cb_print_version, 0, N_("Print version information."), NULL},
		{NULL, '\0', 0, NULL, 0, NULL, NULL }
	};
#endif /* GNOME_PARAM_GOPTION_CONTEXT */

#ifdef ENABLE_NLS
	setlocale(LC_ALL,"");                                                   
	bindtextdomain(PACKAGE,LOCALEDIR);
	DEBUG_MSG("set bindtextdomain for %s to %s\n",PACKAGE,LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	textdomain(PACKAGE);                                                    
#endif /* ENABLE_NLS */

#if !defined (HAVE_LIBGNOMEUI_LIBGNOMEUI_H) || defined (GNOME_PARAM_GOPTION_CONTEXT)
	context = g_option_context_new (_(" [FILE(S)]"));
#ifdef ENABLE_NLS	
	g_option_context_add_main_entries (context, options, PACKAGE);
#else
	g_option_context_add_main_entries (context, options, NULL);
#endif /* ENABLE_NLS */
#endif /* GNOME_PARAM_GOPTION_CONTEXT || !HAVE_LIBGNOMEUI_LIBGNOMEUI_H */

#ifdef HAVE_LIBGNOMEUI_LIBGNOMEUI_H
	bfprogram = gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE, 
	                                argc, argv,
#ifdef GNOME_PARAM_GOPTION_CONTEXT
	                                GNOME_PARAM_GOPTION_CONTEXT, context,
#else /* GNOME_PARAM_GOPTION_CONTEXT */
	                                GNOME_PARAM_POPT_TABLE, options,
#endif /* GNOME_PARAM_GOPTION_CONTEXT */
	                                NULL);
#else /* HAVE_LIBGNOMEUI_LIBGNOMEUI_H */
	/* this one or bail out with the error of g_option_context_parse () */
	g_option_context_set_ignore_unknown_options (context, TRUE);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_error(N_("Error parsing command line options. %s\nPlease run: %s -?\n"),
		        error->message, argv[0]);
	}

	g_option_context_free (context);
	if (error) g_error_free (error);
#endif /* HAVE_LIBGNOMEUI_LIBGNOMEUI_H */

	xmlInitParser();
#ifndef HAVE_ATLEAST_GIO_2_16
	gnome_vfs_init();
#endif
#ifdef HAVE_LIBGNOMEUI_LIBGNOMEUI_H
	/* will be crippled without libgnomeui*/
	gnome_authentication_manager_init();
#endif /* HAVE_LIBGNOMEUI_LIBGNOMEUI_H */

	set_default_icon();
	main_v = g_new0(Tmain, 1);
	DEBUG_MSG("main, main_v is at %p\n", main_v);
	rcfile_check_directory();
	rcfile_parse_main();
	
	if (newwindow) {
		open_in_new_window = 1;
	}

	if (skiprootcheck) {
		root_override = 1;
	}

	if (project) {
#ifdef HAVE_ATLEAST_GIO_2_16
		tmpfile = g_file_new_for_commandline_arg(project);
		projectfiles = g_list_append(projectfiles, g_file_get_parse_name(tmpfile));
		g_object_unref(tmpfile);
#else
		tmpname = create_full_path(project, NULL);
		projectfiles = g_list_append(projectfiles, tmpname);
#endif
		/*
			TODO 1: Check if given file is a project-file.
			Maybe using gnome-vfs API and checking for application/bluefish-project.
			If not at least one project file was given, fail or print a warning?
			
			ATM: Nothing happens.
			

			TODO 2: suggestion, on what should happen using the following command:
			
			bluefish(-unstable) -p foo.bfproject bar.html foo.bar
			bluefish(-unstable) --project=foo.bfproject bar.html foo.bar
			
			or in general:
			
			bluefish(-unstable) [-p |--project=]projectfile [file(s)]
			
			1) first open project
			2) open the other files in the project
			
			this could be a good starting poitn, when you want to open a file inside the project,
			without seeing the whole tree (just see the project)
		*/
		
		DEBUG_MSG("main, project=%s, tmpname=%s\n", project, tmpname);
	}
#if defined (HAVE_LIBGNOMEUI_LIBGNOMEUI_H) && !defined (GNOME_PARAM_GOPTION_CONTEXT)
	g_object_get(G_OBJECT(bfprogram), GNOME_PARAM_POPT_CONTEXT, &pcontext, NULL);
	files = (char**) poptGetArgs(pcontext);
#endif /* HAVE_LIBGNOMEUI_LIBGNOMEUI_H && !GNOME_PARAM_GOPTION_CONTEXT */
	if (files != NULL) {
		filearray = g_strv_length(files);
		for (i = 0; i < filearray; ++i) {
#ifdef HAVE_ATLEAST_GIO_2_16
			tmpfile = g_file_new_for_commandline_arg(files[i]);
			filenames = g_list_append(filenames, g_file_get_parse_name(tmpfile));
			g_object_unref(tmpfile);
#else
			tmpname = create_full_path(files[i], NULL);
			filenames = g_list_append(filenames, tmpname);
#endif
			
			DEBUG_MSG("main, files[%d]=%s, tmpname=%s\n", i, files[i], tmpname);
		}
		g_strfreev(files);
	}
		
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
#endif /* NOSPLASH */

/*	{
		gchar *filename = g_strconcat(g_get_home_dir(), "/."PACKAGE"/dir_history", NULL);
		main_v->recent_directories = get_stringlist(filename, NULL);
		g_free(filename);
	}*/
	bluefish_load_plugins();
	main_v->session = g_new0(Tsessionvars,1);
	main_v->session->view_main_toolbar = main_v->session->view_left_panel = main_v->session->filebrowser_focus_follow =1;
	main_v->session->snr_position_x = main_v->session->snr_position_y = -1;
	rcfile_parse_global_session();
	if (main_v->session->recent_dirs == NULL) {
		main_v->session->recent_dirs = g_list_append(main_v->session->recent_dirs, g_strconcat("file://", g_get_home_dir(), NULL));
	}
	main_v->autocompletion = ac_init();	
	textstyle_rebuild();

#ifndef NOSPLASH
	if (main_v->props.show_splash_screen) splash_screen_set_label(_("compiling highlighting patterns..."));
#endif /* NOSPLASH */

	main_v->lang_mgr	= bf_lang_mgr_new();
	filetype_highlighting_rebuild(FALSE);
#ifndef NOSPLASH
	if (main_v->props.show_splash_screen) splash_screen_set_label(_("building file filters ..."));
#endif /* NOSPLASH */
	
	fb2config_init(); /* filebrowser2config */
	filters_rebuild();
	autoclosing_init();
	main_v->tooltips = gtk_tooltips_new();
#ifndef NOSPLASH
	if (main_v->props.show_splash_screen) splash_screen_set_label(_("setting up bookmarks ..."));
#endif /* NOSPLASH */

	main_v->bmarkdata = bookmark_data_new();
#ifdef WITH_MSG_QUEUE
	if (!filenames && !projectfiles && main_v->props.open_in_running_bluefish) {
		msg_queue_start(NULL, NULL, open_in_new_window);
	}
#endif /* WITH_MSG_QUEUE */
#ifndef NOSPLASH
	if (main_v->props.show_splash_screen) splash_screen_set_label(_("creating main gui..."));
#endif /* NOSPLASH */
	/* create the first window */
	firstbfwin = g_new0(Tbfwin,1);
	firstbfwin->session = main_v->session;
	firstbfwin->bmarkdata = main_v->bmarkdata;
	main_v->bfwinlist = g_list_append(NULL, firstbfwin);
	gui_create_main(firstbfwin,filenames);
	bmark_reload(firstbfwin);

#ifndef NOSPLASH
	if (main_v->props.show_splash_screen) splash_screen_set_label(_("showing main gui..."));
#endif /* NOSPLASH */
	/* set GTK settings, must be AFTER the menu is created */
	{
		gchar *shortcutfilename;
		GtkSettings* gtksettings = gtk_settings_get_default();
		g_object_set(G_OBJECT(gtksettings), "gtk-can-change-accels", TRUE, NULL); 
		shortcutfilename = g_strconcat(g_get_home_dir(), "/."PACKAGE"/menudump_2", NULL);
		gtk_accel_map_load(shortcutfilename);
		g_free(shortcutfilename);
	}

	gui_show_main(firstbfwin);
	if (projectfiles) {
		GList *tmplist = g_list_first(projectfiles);
		while (tmplist) {
			project_open_from_file(firstbfwin, tmplist->data);
			tmplist = g_list_next(tmplist);
		}
	}
	
#ifndef NOSPLASH
	if (main_v->props.show_splash_screen) {
		/*static struct timespec const req = { 0, 10000000};*/
		flush_queue();
		/*nanosleep(&req, NULL);*/
		gtk_widget_destroy(splash_window);
	}
#endif /* NOSPLASH */
	DEBUG_MSG("main, before gtk_main()\n");
	gtk_main();
	DEBUG_MSG("main, after gtk_main()\n");
#ifdef WITH_MSG_QUEUE	
	/* do the cleanup */
	msg_queue_cleanup();
#endif /* WITH_MSG_QUEUE */
	DEBUG_MSG("calling fb2config_cleanup()\n");
	fb2config_cleanup();
	DEBUG_MSG("Bluefish: exiting cleanly\n");
#ifdef HAVE_LIBGNOMEUI_LIBGNOMEUI_H
	g_object_unref (bfprogram);
#endif /* HAVE_LIBGNOMEUI_LIBGNOMEUI_H */
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
			if (!project_save_and_close(BFWIN(tmplist->data),FALSE)) {
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

	tmplist = g_list_first(gtk_window_list_toplevels());
	while (tmplist) {
		gtk_widget_hide(GTK_WIDGET(tmplist->data));
		tmplist = g_list_next(tmplist);
	}
	flush_queue();
	
	rcfile_save_global_session();
	
/*	{
		gchar *filename = g_strconcat(g_get_home_dir(), "/."PACKAGE"/dir_history", NULL);
		put_stringlist_limited(filename, main_v->recent_directories, main_v->props.max_dir_history);
		g_free(filename);
	}*/
	
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
}
