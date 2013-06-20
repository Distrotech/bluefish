/* Bluefish HTML Editor
 * file_autosave.c - autosave 
 *
 * Copyright (C) 2009,2010,2011,2012,2013 Olivier Sessink
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
/* indented with indent -ts4 -kr -l110   */

/**********************************************************************************
*** THE AUTOSAVE DESIGN ***

every Tdocument has
GList *needs_autosave
GList *autosave_progress
GList *autosaved
GFile *autosave_uri

Tmain has
GList *autosave_journal
GList *need_autosave
GList *autosave_progress

On a document change we register the Tdocument* in main_v->need_autosave
the GList* element that has this reference is stored in doc->needs_autosave

Every X seconds we loop over main_v->need_autosave
for every Tdocument that needs autosave
 - if it does not exist create autosave_uri (which is placed in a hash table, so 
    if we have to create autosave uri's for 100 documents that are named 
    index.html we just do a lookup in the hash table if it exists already)
 - move the need_autosave pointer to autosave_progress 
 - start an async call to store the contents in a autosave file
 - if there are no documents that need to store their context, we only see if 
   main_v->autosave_need_journal_save is TRUE is which case we store the autosave_journal 
   to disk (for example after a document has saved it's changes or a document has been closed)

In the callback after the save is finished:
 - register the Tdocument* in main_v->autosaved and store the GList* element in doc->autosaved
 - remove the autosave_progress link from main_v->autosave_progress (because we know which GList* element
   is involved this can be done without looping trough the list to find the right entry)
 - if main_v->autosave_progress == NULL we save the main_v->autosave_journal to a file autosave_journal.PID

during document close/save etc.
 - remove the Tdocument* reference from main_v->need_autosave using the GList* element stored in doc->need_autosave
 - delete the doc->autosave_uri file
 - remove the entry in main_v->autosave_journal using the GList* entry in doc->autosaved
 - the journal is only updated on disk on the next autosave_run, so the journal on disk might still mention the autosave_uri !

during quit:
 - remove the autosave_journal file
 
during startup:
 - check for old autosave_journal.PID files 
 - test if this process is still alive using kill()
 - recover unsaved files
 

************************************************************************************/

/*#define DEBUG*/

#include <gtk/gtk.h>
#include <glib/gstdio.h>		/* g_mkdir */

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "bluefish.h"
#include "bf_lib.h"
#include "bfwin.h"
#include "document.h"
#include "file.h"
#include "stringlist.h"
#include "project.h"
#include "rcfile.h"


static GHashTable *
autosave_uri_list(void)
{
	GList *tmplist;
	GHashTable *hasht = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	tmplist = g_list_first(main_v->autosave_progress);
	while (tmplist) {
		Tdocument *doc = tmplist->data;
		if (doc->autosave_uri) {
			g_hash_table_insert(hasht, g_file_get_path(doc->autosave_uri), GINT_TO_POINTER(1));
		}
		tmplist = g_list_next(tmplist);
	}
	return hasht;
}

static GFile *
create_autosave_path(Tdocument * doc, GHashTable * excludehash)
{
	GFile *retval;
	DEBUG_MSG("create_autosave_path\n");
	if (main_v->props.autosave_location_mode != 0
		&& (main_v->props.autosave_file_prefix == NULL || main_v->props.autosave_file_prefix[0] == '\0')
		&& (main_v->props.autosave_file_suffix == NULL || main_v->props.autosave_file_suffix[0] == '\0')) {
		main_v->props.autosave_location_mode = 0;
	}
	if (main_v->props.autosave_location_mode == 0 || doc->uri == NULL) {
		gchar *path, *dir = g_strconcat(g_get_home_dir(), "/." PACKAGE "/autosave/", NULL);
		if (doc->uri) {
			gchar *base = g_file_get_basename(doc->uri);
			path = unique_path(dir, base, excludehash);
			g_free(base);
		} else {
			/* because the actual saving is done async multi-threaded, a file with 
			   this name might be created a moment after we check.  */
			path = unique_path(dir, gtk_label_get_text(GTK_LABEL(doc->tab_label)), excludehash);
		}
		retval = g_file_new_for_path(path);
		g_free(path);
		g_free(dir);
	} else {
		char *base, *newbase;
		GFile *dir = g_file_get_parent(doc->uri);
		base = g_file_get_basename(doc->uri);
		newbase =
			g_strconcat(main_v->props.autosave_file_prefix, base, main_v->props.autosave_file_suffix, NULL);
		retval = g_file_get_child(dir, newbase);
		g_free(base);
		g_free(newbase);
		g_object_unref(dir);
	}
	return retval;
}

static void
autosave_save_journal(void)
{
	GFile *file;
	gchar *path = g_strdup_printf("%s/." PACKAGE "/autosave_journal.%d", g_get_home_dir(), getpid());
	DEBUG_MSG("autosave_save_journal in %s\n", path);
	file = g_file_new_for_path(path);
	g_free(path);
	put_stringlist(file, main_v->autosave_journal, TRUE);
	g_object_unref(file);
	main_v->autosave_need_journal_save = FALSE;
}

GList *
register_autosave_journal(GFile * autosave_file, GFile * document_uri, GFile * project_uri)
{
	gchar **arr, *arr1, *arr2, *arr3;

	arr1 = g_file_get_uri(autosave_file);
	arr2 = document_uri ? g_file_get_uri(document_uri) : g_strdup("");
	arr3 = project_uri ? g_file_get_uri(project_uri) : g_strdup("");
	DEBUG_MSG("register_autosave_journal %s for %s\n", arr1, arr2);
	arr = array_from_arglist(arr1, arr2, arr3, NULL);
	main_v->autosave_journal = g_list_prepend(main_v->autosave_journal, arr);
	main_v->autosave_need_journal_save = TRUE;
	g_free(arr1);
	g_free(arr2);
	g_free(arr3);
	return main_v->autosave_journal;	/* the returned GList has arr as data */
}

void
remove_autosave(Tdocument * doc)
{
	DEBUG_MSG("remove_autosave for doc %p, autosave_uri=%p, autosaved=%p, need_autosave=%p\n", doc,
			  doc->autosave_uri, doc->autosaved, doc->need_autosave);
	/* remove from journal */
	if (doc->autosaved) {
		main_v->autosave_journal = g_list_delete_link(main_v->autosave_journal, doc->autosaved);
		doc->autosaved = NULL;
		main_v->autosave_need_journal_save = TRUE;
	}

	/* remove need to autosave */
	if (doc->need_autosave) {
		main_v->need_autosave = g_list_delete_link(main_v->need_autosave, doc->need_autosave);
		doc->need_autosave = NULL;
	}

	/* cancel running autosave */
	if (doc->autosave_progress) {
		if (doc->autosave_action) {
			file_checkNsave_cancel(doc->autosave_action);
			doc->autosave_action = NULL;
		}
		main_v->autosave_progress = g_list_delete_link(main_v->autosave_progress, doc->autosave_progress);
		doc->autosave_progress = NULL;
	}

	/* delete autosaved file */
	if (doc->autosave_uri) {
		file_delete_async(doc->autosave_uri, FALSE, NULL, NULL);
	}

	if (doc->autosave_uri) {
		g_object_unref(doc->autosave_uri);
		doc->autosave_uri = NULL;
	}
}

static void
autosave_complete_lcb(gint status, gint error_info, gpointer data)
{
	Tdocument *doc = data;
	DEBUG_MSG("autosave_complete_lcb, status=%d for doc %p\n", status, doc);
	switch (status) {
	case CHECKANDSAVE_FINISHED:
		if (!doc->autosaved) {
			doc->autosaved =
				register_autosave_journal(doc->autosave_uri, doc->uri,
										  (doc->bfwin
										   && BFWIN(doc->bfwin)->project) ? BFWIN(doc->bfwin)->
										  project->uri : NULL);
		}
		main_v->autosave_progress = g_list_delete_link(main_v->autosave_progress, doc->autosave_progress);
		doc->autosave_progress = NULL;
		doc->autosave_action = NULL;
		break;
	case CHECKANDSAVE_ERROR_CANCELLED:
		DEBUG_MSG("autosave_complete_lcb, cancelled, don't touch the 'doc' pointer anymore\n");
		break;
	case CHECKANDSAVE_ERROR:
	case CHECKANDSAVE_ERROR_NOBACKUP:
	case CHECKANDSAVE_ERROR_NOWRITE:
	case CHECKANDSAVE_ERROR_MODIFIED:
		main_v->autosave_progress = g_list_delete_link(main_v->autosave_progress, doc->autosave_progress);
		doc->autosave_progress = NULL;
		break;
	default:
		DEBUG_MSG("autosave_complete_lcb, status %d is not handled\n", status);
		break;
	}
	if (main_v->autosave_progress == NULL && main_v->autosave_need_journal_save) {
		autosave_save_journal();
	}
}

static inline void
autosave(Tdocument * doc, GHashTable * hasht)
{
	Trefcpointer *buffer;
	gchar *data;
	DEBUG_MSG("autosave doc %p\n", doc);
	if (!doc->autosave_uri) {
		doc->autosave_uri = create_autosave_path(doc, hasht);
		g_hash_table_insert(hasht, g_file_get_path(doc->autosave_uri), GINT_TO_POINTER(1));
	}
	data = doc_get_chars(doc, 0, -1);
	if (!data || data[0] == '\0')
		return;

	buffer = refcpointer_new(data);
	doc->autosave_action =
		file_checkNsave_uri_async(doc->autosave_uri, NULL, buffer, strlen(buffer->data), FALSE, FALSE,
								  (CheckNsaveAsyncCallback) autosave_complete_lcb, doc);
	refcpointer_unref(buffer);
}

static gboolean
run_autosave(gpointer data)
{
	GList *tmplist;
	static gint counter=0;
	gboolean disk_spun_up=FALSE;
	DEBUG_SIG("run_autosave, after timeout priority=%d\n",G_PRIORITY_DEFAULT);
	DEBUG_MSG("run_autosave\n");
	if (main_v->autosave_progress)
		return TRUE;

	/* we try to synchronise auto-project and auto-session save wit the autosave, so only if we 
	have to auto save something we might save the project or session as well */
	counter++;
	if (main_v->need_autosave) {
		GHashTable *hasht;
		bfwin_all_statusbar_message(_("Autosave in progress..."), 1);
		hasht = autosave_uri_list();
		main_v->autosave_progress = main_v->need_autosave;
		main_v->need_autosave = NULL;
		tmplist = g_list_first(main_v->autosave_progress);
		while (tmplist) {
			Tdocument *doc = tmplist->data;
			doc->autosave_progress = doc->need_autosave;
			doc->need_autosave = NULL;
			autosave(doc, hasht);
			tmplist = g_list_next(tmplist);
		}
		g_hash_table_destroy(hasht);
		disk_spun_up = TRUE;
	} else if (main_v->autosave_need_journal_save) {
		autosave_save_journal();
		disk_spun_up = TRUE;
	}
	if (disk_spun_up && counter > 30) {
		/* with default values (every minute autosave) this means once in 30 minutes config file save */
		for (tmplist=g_list_first(main_v->bfwinlist);tmplist;tmplist=g_list_next(tmplist)) {
			if (BFWIN(tmplist->data)->project) {
				project_save(BFWIN(tmplist->data), FALSE);
			}
		}
		rcfile_save_global_session();
		counter=0;
	}
	return TRUE;
}

static inline void
autosave_recover(Tbfwin * bfwin, GFile * file)
{
	GList *list, *tmplist;
	DEBUG_MSG("autosave_recover\n");
	list = get_list(file, NULL, TRUE);
	tmplist = g_list_first(list);
	while (tmplist) {
		gchar **arr = (gchar **) tmplist->data;
		GFile *recover_uri;
		if (g_strv_length(arr) != 3) {
			tmplist = g_list_next(tmplist);
			continue;
		}
		DEBUG_MSG("recover %s for %s\n", arr[0], arr[1]);
		
		/* it might be that the file was mentioned in the journal, but the document 
		was closed already and the corresponding autosave file was deleted before the next
		autosave_run (which would have updated the journal. we should thus check if the 
		autosave file actualy exists before we try to restore it. */
		recover_uri = g_file_new_for_uri(arr[0]);
		if (g_file_query_exists(recover_uri, NULL)) {
			if (arr[1] && arr[1][0] != '\0') {
				GFile *uri = g_file_new_for_uri(arr[1]);
				DEBUG_MSG("recover named document \n");
				file_doc_from_uri(bfwin, uri, recover_uri, NULL, -1, -1, FALSE);
			} else {
				Tdocument *doc = doc_new_loading_in_background(bfwin, NULL, NULL, FALSE);
				DEBUG_MSG("recover unnamed document into %p\n", doc);
				file_into_doc(doc, recover_uri, FALSE, TRUE);
				doc->autosave_uri = recover_uri;
				doc->autosaved = register_autosave_journal(recover_uri, NULL, NULL);
			}
		}
		tmplist = g_list_next(tmplist);
	}
	free_stringlist(list);
	DEBUG_MSG("autosave_recover done\n");
}

void
need_autosave(Tdocument * doc)
{
	if (main_v->props.autosave) {
		if (doc->need_autosave == NULL) {
			DEBUG_MSG("need_autosave, mark doc %p for autosave\n", doc);
			main_v->need_autosave = g_list_prepend(main_v->need_autosave, doc);
			doc->need_autosave = main_v->need_autosave;
		}
	}
}

static gboolean
autosave_cleanup_old_autosaves(gpointer data)
{
	GError *error = NULL;
	GDir *gdir;
	gchar *dir;
	const gchar *tmp;
	DEBUG_MSG("autosave_cleanup_old_autosaves\n");
	dir = g_strconcat(g_get_home_dir(), "/." PACKAGE "/autosave/", NULL);
	gdir = g_dir_open(dir, 0, &error);
	if (error) {
		g_warning("failed to open directory %s for cleanup\n", dir);
		g_error_free(error);
	} else {
		tmp = g_dir_read_name(gdir);
		while (tmp) {
			gchar *path = g_strconcat(dir, tmp, NULL);
			DEBUG_MSG("autosave_cleanup_old_autosaves, unlink %s\n", path);
			g_unlink(path);
			g_free(path);
			tmp = g_dir_read_name(gdir);
		}
		g_dir_close(gdir);
	}
	g_free(dir);
	return FALSE;
}

void
autosave_init(gboolean recover, Tbfwin * bfwin)
{
	DEBUG_MSG("autosave_init, recover=%d\n", recover);
	if (main_v->autosave_id) {
		g_source_remove(main_v->autosave_id);
	}
	main_v->autosave_need_journal_save = FALSE;

	if (recover && bfwin) {
		GError *error = NULL;
		const gchar *tmp;
		GPatternSpec *patspec;
		GDir *gdir;
		gboolean found_journal = FALSE;
		gchar *dir;

		/* test if autosave directory exists */
		dir = g_strconcat(g_get_home_dir(), "/." PACKAGE "/autosave/", NULL);
		if (!g_file_test(dir, G_FILE_TEST_EXISTS)) {
			g_mkdir(dir, 0700);
		}
		g_free(dir);

		dir = g_strconcat(g_get_home_dir(), "/." PACKAGE "/", NULL);
		gdir = g_dir_open(dir, 0, &error);

		if (error) {
			g_warning("failed to open %s to recover autosaved files\n", dir);
			g_free(dir);
			g_error_free(error);
			return;
		}

		patspec = g_pattern_spec_new("autosave_journal.*");
		tmp = g_dir_read_name(gdir);
		while (tmp) {
			int tmplen = strlen(tmp);
			if (tmp && tmp[0] != '\0' && tmp[tmplen - 1] != '~'
				&& g_pattern_match(patspec, tmplen, tmp, NULL)) {
				gint pid;
				found_journal = TRUE;
				pid = get_int_from_string(tmp);
#ifdef WIN32
				HANDLE hProc;
				if (pid > 0 && !(hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid))) {
#else
				if (pid > 0 && kill(pid, 0) != 0) {	/* process pid is not alive, or not our process which also means we should recover */
#endif
					gchar *path;
					GError *error = NULL;
					GFile *file;
					path = g_strconcat(dir, tmp, NULL);
					file = g_file_new_for_path(path);
					autosave_recover(bfwin, file);
					g_file_delete(file, NULL, &error);
					if (error) {
						g_warning("failed to delete recovered autosave_journal %s: %s\n", path,
								  error->message);
					}
					g_free(path);
					g_object_unref(file);
				}
#ifdef WIN32
				else
					CloseHandle(hProc);
#endif
			}
			tmp = g_dir_read_name(gdir);
		}
		g_free(dir);
		g_dir_close(gdir);
		g_pattern_spec_free(patspec);
		if (!found_journal) {
			/* if there was no journal we can remove *ALL* autosave files */
			g_idle_add_full(G_PRIORITY_LOW, autosave_cleanup_old_autosaves, NULL, NULL);
		}
	}
	if (main_v->props.autosave) {
		main_v->autosave_id =
			g_timeout_add_seconds_full(G_PRIORITY_DEFAULT, main_v->props.autosave_time, run_autosave, NULL,
									   NULL);
	}
}

void
autosave_cleanup(void)
{
	GFile *file;
	GError *error = NULL;
	gchar *path = g_strdup_printf("%s/." PACKAGE "/autosave_journal.%d", g_get_home_dir(), getpid());
	DEBUG_MSG("autosave_cleanup\n");
	file = g_file_new_for_path(path);

	/* is async a good option during exit ??????? */
	g_file_delete(file, NULL, &error);
	if (error) {
		/* not really a problem if it does not exist */
		/*g_warning("failed to delete %s: %s\n",path,error->message); */
		g_error_free(error);
	}
	g_free(path);
	g_object_unref(file);
}
