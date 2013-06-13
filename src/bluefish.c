/* Bluefish HTML Editor
 * bluefish.c - the main function
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * Copyright (C) 1999-2011 Olivier Sessink
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

/*#define DEBUG */

#include <gtk/gtk.h>
#include <stdlib.h>				/* exit() on Solaris */
#include <time.h>				/* nanosleep */

#ifndef WIN32
#include <signal.h>             /* sigaction */
#endif

#ifdef MAC_INTEGRATION
#include <gtkosxapplication.h>
#endif


#include "bluefish.h"
#include <libxml/parser.h>

#ifdef ENABLE_NLS
#include <locale.h>
#endif							/* ENABLE_NLS */

#include "bftextview2_langmgr.h"
#ifdef HAVE_LIBENCHANT
#include "bftextview2_spell.h"
#endif
#include "pixmap.h"				/* set_default_icon() */
#include "bf_lib.h"				/* create_full_path() */
#include "bfwin.h"
#include "bookmark.h"			/* bmark_init() */
#include "dialog_utils.h"		/* message_dialog_new() */
#include "document.h"
#include "filebrowser2.h"
#include "file_dialogs.h"
#include "gtk_easy.h"			/* flush_queue() */
#ifdef HAVE_MESSAGE_QUEUE
#include "msg_queue.h"			/* msg_queue_start() */
#else
#include "ipc_bf2bf.h"
#endif
#include "plugins.h"
#include "project.h"
#include "rcfile.h"				/* rcfile_parse_main() */
#include "stringlist.h"			/* put_stringlist(), get_stringlist() */
#include "filefilter.h"
#include "file.h" /* file_handle() */
#include "file_autosave.h"
#include "languages.h"

/*#define STARTUP_PROFILING*/

/*********************************************/
/* this var is global for all bluefish files */
/*********************************************/
Tmain *main_v;

#ifdef STARTUP_PROFILING
static GTimer *startuptimer;
#endif

#ifndef __GNUC__
void g_none(char * first, ...)
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

static void init_default_session(Tsessionvars *session) {
	session->enable_syntax_scan = session->view_main_toolbar
			= session->view_left_panel
			= session->filebrowser_focus_follow
			= session->view_statusbar
			= session->autoindent
			= session->view_line_numbers
			= session->view_cline
			= session->view_blocks
			= session->view_blockstack
			= session->autocomplete
			= session->show_mbhl
			= 1;
#ifdef HAVE_LIBENCHANT
	session->spell_check_default = 1;
#endif
	session->snr_position_x = session->snr_position_y = -1;
	session->editor_tab_width = 3;
	session->snr3_recursion_level = 5;
}

#ifndef WIN32
static void sigterm_handler(int signalnum, siginfo_t *si, void *data) {
	g_print("caught SIGTERM, exiting...\n");
	bluefish_exit_request();
}

static void handle_signals(void) {
	struct sigaction sa;
	sa.sa_sigaction = sigterm_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGTERM, &sa, NULL);
}
#endif

/* Tstartup is used during the
initialization inside gtk_main() */
typedef struct {
	Tbfwin *firstbfwin;
	GList *filenames;
	guint state;
#ifdef MAC_INTEGRATION
	GtkosxApplication *OsxApp;
#endif
} Tstartup;


#ifdef MAC_INTEGRATION
static gboolean
osx_open_file_cb(GtkosxApplication *app, gchar *path, gpointer user_data)
{
	GFile *uri = g_file_new_for_path(path);
	if (main_v->bfwinlist) {
		Tbfwin *bfwin = BFWIN(g_list_last(main_v->bfwinlist)->data);
		g_print("osx_open_file_cb, open %s, bfwin=%p\n",path, bfwin);
		file_handle(uri, bfwin , NULL, TRUE);
		g_object_unref(uri);
	} else {
		Tstartup *startup = user_data;
		g_print("osx_open_file_cb, open %s, add uri to startup->filenames\n",path);
		startup->filenames = g_list_prepend(startup->filenames, uri);
	}
	return TRUE;
}
#endif


static gboolean startup_in_idle(gpointer data) {
	Tstartup *startup=data;
	DEBUG_MSG("startup_in_idle, started state=%d, elapsed=\n",startup->state);
#ifdef STARTUP_PROFILING
	g_print("startup_in_idle, state=%d, elapsed=%d\n",startup->state,(gint)(g_timer_elapsed(startuptimer,NULL)*1000.0));
#endif
	switch (startup->state) {
		case 0:
			bluefish_load_plugins();
			main_v->session = g_new0(Tsessionvars, 1);
			init_default_session(main_v->session);
			rcfile_parse_global_session();
			if (main_v->session->recent_dirs == NULL) {
				GFile *uri = g_file_new_for_path(g_get_home_dir());
				main_v->session->recent_dirs =
					g_list_append(main_v->session->recent_dirs,
								  g_file_get_uri(uri));
				g_object_unref(uri);
			}
			bftextview2_init_globals();
			langmgr_init();
#ifdef HAVE_LIBENCHANT
			bftextview2_spell_init();
#endif /*HAVE_LIBENCHANT*/
			set_default_icon();
			fb2config_init();			/* filebrowser2config */
			filters_rebuild();
			main_v->bmarkdata = bookmark_data_new();
			register_bf_stock_icons();
			load_templates();
			/* create the first window */
			startup->firstbfwin = g_new0(Tbfwin, 1);
			startup->firstbfwin->session = main_v->session;
			startup->firstbfwin->bmarkdata = main_v->bmarkdata;
			main_v->bfwinlist = g_list_append(NULL, startup->firstbfwin);
#ifdef WITH_MSG_QUEUE
			if (main_v->props.open_in_running_bluefish) {
				msg_queue_check_server(FALSE);
			}
#endif							/* WITH_MSG_QUEUE */
		break;
		case 1:
			bfwin_create_main(startup->firstbfwin);
#ifdef WITH_MSG_QUEUE
			if (main_v->props.open_in_running_bluefish) {
				msg_queue_check_server(FALSE);
			}
#endif
		break;
		case 2:
			file_static_queues_init();
			if (startup->filenames) {
				GList *tmplist = g_list_first(startup->filenames);
				DEBUG_MSG("startup_in_idle, we have filenames, load them\n");
				startup->firstbfwin->focus_next_new_doc = TRUE;
				while (tmplist) {
					file_handle((GFile *)tmplist->data, startup->firstbfwin, NULL, TRUE);
					g_object_unref((GFile *)tmplist->data);
					tmplist = g_list_next(tmplist);
				}
				g_list_free(startup->filenames);
			}
			if (startup->firstbfwin->session == main_v->session)
				bmark_reload(startup->firstbfwin); /* do not reload bookmarks for a project */
			/* set GTK settings, must be AFTER the menu is created */
			{
				GtkSettings *gtksettings = gtk_settings_get_default();
				g_object_set(G_OBJECT(gtksettings), "gtk-can-change-accels", TRUE, NULL);
				rcfile_load_accelerators(FALSE);
			}
			autosave_init(TRUE, startup->firstbfwin);
		#ifdef WITH_MSG_QUEUE
			if (main_v->props.open_in_running_bluefish) {
				msg_queue_check_server(TRUE);
			}
		#endif
		break;
		case 3:
			bfwin_show_main(startup->firstbfwin);
		break;
		case 4:
			main_v->recentm = gtk_recent_manager_get_default();
			doc_scroll_to_cursor(BFWIN(startup->firstbfwin)->current_document);
			modified_on_disk_check_init();
#ifndef WIN32
			handle_signals();
#endif
			g_free(startup);
			return FALSE;
		break;
	}
#ifdef STARTUP_PROFILING
	g_print("startup_in_idle, end of state=%d, elapsed=%d\n",startup->state,(gint)(g_timer_elapsed(startuptimer,NULL)*1000.0));
#endif
	startup->state++;
	return TRUE;
}

int main(int argc, char *argv[])
{
	static gboolean arg_curwindow = FALSE, arg_newwindow=FALSE;
	static gchar **files = NULL;
	Tstartup *startup;

#ifdef WIN32
	gchar *path;

	const char *szStartPath = getenv("PATH");
	char *szPythonPath, *szNewPath;

    HKEY hPython;
    DWORD dwSize;
    DWORD dwError;

    DEBUG_MSG("main, about to add Python to PATH");
    // Open HKLM registry key, if that fails try to open HKCU registry key
    if((dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Python\\PythonCore\\2.7\\InstallPath", 0, KEY_QUERY_VALUE, &hPython)) != ERROR_SUCCESS)
		dwError = RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Python\\PythonCore\\2.7\\InstallPath", 0, KEY_QUERY_VALUE, &hPython);

	// If either key opened a standard Python install is available, either globally or for the current user
    if (dwError == ERROR_SUCCESS) {
    	// Get size of value
		dwError = RegQueryValueEx(hPython, NULL, NULL, NULL, NULL, &dwSize);
		if (dwError == ERROR_SUCCESS) {
			// Allocate memory to store value
			szPythonPath = (char *)malloc(dwSize * sizeof(char));
			// Read value from registry
			dwError = RegQueryValueEx(hPython, NULL, NULL, NULL, (LPBYTE)szPythonPath, &dwSize);
			if (dwError == ERROR_SUCCESS) {
				DEBUG_MSG("main, Python path read: %s", szPythonPath);
				// Allocate memory to hold new PATH (old + new + (PATH= + ; + NULL))
				szNewPath = (char *)malloc((strlen(szStartPath) + strlen(szPythonPath) + 7) * sizeof(char));
				// Compose and store the new PATH
				wsprintf(szNewPath, "PATH=%s;%s", szStartPath, szPythonPath);
				// Set the new PATH
				if (!putenv(szNewPath))
					DEBUG_MSG("main, successfully set new PATH: %s", szNewPath);
				else
					DEBUG_MSG("main, error setting new PATH");
				// Free memory used to hold the new path
				free(szNewPath);
			} // Read
			else DEBUG_MSG("main, error reading path (%lu)", dwError);
			// Free memory used to hold the path
			free(szPythonPath);
			// Close the registry key handle
			RegCloseKey(hPython);
		} // Size
		else DEBUG_MSG("main, error getting size of path (%lu)", dwError);
	} // Open
	else DEBUG_MSG("main, error opening registry key (%lu)", dwError);
#endif

	GError *error = NULL;
	GOptionContext *context;
	const GOptionEntry options[] = {
		{"curwindow", 'c', 0, G_OPTION_ARG_NONE, &arg_curwindow,
		 N_("Open in existing window."), NULL},
		{"newwindow", 'n', 0, G_OPTION_ARG_NONE, &arg_newwindow,
		 N_("Open in new window."), NULL},
		{"version", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, (void *) cb_print_version,
		 N_("Print version information."), NULL},
		{G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &files,
		 "Special option that collects any remaining arguments for us", NULL},
		{NULL}
	};
#if !GLIB_CHECK_VERSION(2, 32, 0)
	/* g_thread_init() is called by default starting at glib 2.32 */
	if (!g_thread_supported())
		g_thread_init(NULL);
#endif

#ifdef STARTUP_PROFILING
	startuptimer = g_timer_new();
#endif /* STARTUP_PROFILING */

#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	DEBUG_MSG("set bindtextdomain for %s to %s\n", PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	textdomain(PACKAGE);
#endif							/* ENABLE_NLS */

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

#if !GTK_CHECK_VERSION(3,0,0)
	gtk_rc_parse_string ("style \"bluefish-small-close-button-style\"\n"
                       "{\n"
                          "GtkWidget::focus-padding = 0\n"
                          "GtkWidget::focus-line-width = 0\n"
                          "xthickness = 0\n"
                          "ythickness = 0\n"
                       "}\n"
                       "widget \"*.bluefish-small-close-button\" style \"bluefish-small-close-button-style\""
                       );
#endif
	xmlMemSetup((xmlFreeFunc) g_free, (xmlMallocFunc) g_malloc, (xmlReallocFunc) g_realloc, (xmlStrdupFunc) g_strdup);
	xmlInitParser();
	startup = g_new0(Tstartup,1);
	main_v = g_new0(Tmain, 1);
	main_v->alldochash = g_hash_table_new(g_file_hash, (GEqualFunc) g_file_equal);
	DEBUG_MSG("main, main_v is at %p\n", main_v);

#ifdef MAC_INTEGRATION
	startup->OsxApp = g_object_new(GTKOSX_TYPE_APPLICATION, NULL);
	g_signal_connect(startup->OsxApp, "NSApplicationOpenFile", G_CALLBACK(osx_open_file_cb), startup);
#endif

	if (files != NULL) {
		gchar **tmp = files;
		while (*tmp) {
			GFile *tmpfile = g_file_new_for_commandline_arg(*tmp);
			startup->filenames = g_list_append(startup->filenames, tmpfile);
			DEBUG_MSG("main, file=%s\n", *tmp);
			tmp++;
		}
		g_strfreev(files);
	}

	/* Dynamically create paths for Win32 *after* we have converted the relative
	filenames from the commandline to GFile objects */	
#ifdef WIN32
 	path = g_malloc0(MAX_PATH+1);
	if (GetModuleFileName(NULL, path, MAX_PATH)) {
		/* set current working directory */
		gchar *cwd = g_malloc0(strlen(path+1));
		strncpy(cwd, path, (strrchr(path,'\\')-path));
		if (cwd) {
			SetCurrentDirectory(cwd);
			DEBUG_MSG("Current directory set to: %s\n", cwd);
		}
		g_free(cwd);
	}
	else {
		g_print("Configuration file(s) could not be found.\nExiting now.\n");
		g_free(path);
		bluefish_exit_request();	
	}
	g_free(path);
#endif

#ifdef DEBUG_PATHS
	{
		gchar *cwd = g_get_current_dir();
		g_print("current directory=%s\n",cwd);
		g_free(cwd);
	}
#endif
	rcfile_check_directory();
	rcfile_parse_main();
#ifdef ENABLE_NLS
	if (main_v->props.language!=NULL && main_v->props.language[0]!='\0') {
#ifndef WIN32
		setlocale(LC_ALL, main_v->props.language);
#else /* WIN32 */
		if (!lingua_set_thread_locale_on_windows(main_v->props.language)) {
			g_free(main_v->props.language);
			main_v->props.language = NULL;
		}
#endif /* WIN32 */
	}
#endif /* ENABLE_NLS */
	if (main_v->props.open_in_running_bluefish) {
#ifdef WITH_MSG_QUEUE
		/* start message queue as early as possible so a running bluefish process has a lot of
		time to respond to our request-alive request */
		msg_queue_start(startup->filenames, (arg_newwindow || (main_v->props.open_in_new_window && !arg_curwindow) ) );
#else
		if (!ipc_bf2bf_start(startup->filenames, (arg_newwindow || (main_v->props.open_in_new_window && !arg_curwindow) ) )) {
			gdk_notify_startup_complete();
			exit(0);
		}
#endif /* WITH_MSG_QUEUE */
	}

	g_idle_add_full(G_PRIORITY_DEFAULT_IDLE-50, startup_in_idle, startup, NULL);
#ifdef MAC_INTEGRATION
			gtkosx_application_ready(startup->OsxApp);
#endif
	DEBUG_MSG("main, before gtk_main()\n");
	gtk_main();
	DEBUG_MSG("main, after gtk_main()\n");

	autosave_cleanup();
	if (main_v->props.save_accelmap)
		rcfile_save_accelerators();
#ifdef WITH_MSG_QUEUE
	/* do the cleanup */
	msg_queue_cleanup();
#else
	ipc_bf2bf_cleanup();
#endif /* WITH_MSG_QUEUE */

#ifdef MEMORY_LEAK_DEBUG
	DEBUG_MSG("calling fb2config_cleanup()\n");
	fb2config_cleanup();
	main_v->bmarkdata = bookmark_data_cleanup(main_v->bmarkdata);
	bluefish_cleanup_plugins();
	langmgr_cleanup();
	xmlCleanupParser();

	/*cairo_debug_reset_static_data();
	FcFini();*/
	DEBUG_MSG("Bluefish: exiting cleanly\n");
#else
	exit(0);
#endif
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
		Tbfwin *bfwin = BFWIN(tmplist->data);
		tmplist = g_list_next(tmplist);
		bfwin_delete_event(NULL,NULL,bfwin);
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
	g_list_free(tmplist);
	flush_queue();

	rcfile_save_global_session();

	gtk_main_quit();
	DEBUG_MSG("bluefish_exit_request, after gtk_main_quit()\n");
}
