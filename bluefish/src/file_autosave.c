/* Bluefish HTML Editor
 * file_autosave.c - autosave 
 *
 * Copyright (C) 2009 Olivier Sessink
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
/* indented with indent -ts4 -kr -l110   */

/**********************************************************************************
*** THE AUTOSAVE DESIGN ***

every Tdocument has
GList * needs_autosave
GList *autosaved
GFile *autosave_uri

Tmain has
GList *autosave_journal
GList *need_autosave

On a document change we register the Tdocument* in main_v->need_autosave
the GList* element that has this reference is stored in doc->needs_autosave

Every X seconds we loop over main_v->need_autosave
for every Tdocument that needs autosave
 - if it does not exist create autosave_uri 
 - start an async call to store the contents in a autosave file

In the callback after the save is finished:
 - register the Tdocument* in main_v->autosaved and store the GList* element in doc->autosaved
 - remove the needs_autosave link from main_v->need_autosave (because we know which GList* element
   is involved this can be done without looping trough the list to find the right entry)
  - if main_v->need_autosave == NULL we save the main_v->autosave_journal to a file autosave_journal.PID

during document close/save etc.
 - remove the Tdocument* reference from main_v->need_autosave using the GList* element stored in doc->need_autosave
 - delete the doc->autosave_uri file
 - remove the entry in main_v->autosave_journal using the GList* entry in doc->autosaved

during quit:
 - remove the autosave_journal file
 
during startup:
 - check for old autosave_journal.PID files 
 - test if this process is still alive using kill()
 - recover unsaved files
 

************************************************************************************/

/*#define DEBUG*/

#include <gtk/gtk.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "bluefish.h"
#include "stringlist.h"
#include "bf_lib.h"
#include "file.h"
#include "document.h"

static GFile * create_autosave_path(Tdocument *doc) {
	GFile *retval;
	g_print("create_autosave_path\n");
	if (main_v->props.autosave_location_mode!=0 &&  (main_v->props.autosave_file_prefix==NULL||main_v->props.autosave_file_prefix[0]=='\0')
				&& (main_v->props.autosave_file_suffix==NULL||main_v->props.autosave_file_suffix[0]=='\0')) {
		main_v->props.autosave_location_mode = 0;
	}
	if (main_v->props.autosave_location_mode == 0 || doc->uri == NULL) {
		gchar *path, *dir = g_strconcat(g_get_home_dir(), "/."PACKAGE"/autosave/",NULL);
		if (doc->uri) {
			gchar *base = g_file_get_basename(doc->uri);
			path = unique_path(dir, base);
			g_free(base);
		} else {
			path = unique_path(dir, "no-name");
		}
		retval = g_file_new_for_path(path);
		g_free(path); 
		g_free(dir);
	} else {
		char *base, *newbase;
		GFile *dir = g_file_get_parent(doc->uri);
		base = g_file_get_basename(doc->uri);
		newbase = g_strconcat(main_v->props.autosave_file_prefix, base, main_v->props.autosave_file_suffix, NULL);
		retval = g_file_get_child(dir, newbase);
		g_free(base);
		g_free(newbase);
		g_object_unref(dir);
	}
	return retval;
}

static void autosave_save_journal(void) {
	if (main_v->autosave_need_journal_save) {
		GFile *file;
		gchar *path = g_strdup_printf("%s/."PACKAGE"/autosave_journal.%d",g_get_home_dir(),getpid());
		g_print("autosave_save_journal in %s\n",path);
		file = g_file_new_for_path(path);
		g_free(path);
		put_stringlist(file, main_v->autosave_journal, TRUE);
		g_object_unref(file);
		main_v->autosave_need_journal_save=FALSE;
	}
}

GList *register_autosave_journal(GFile *autosave_file, GFile *document_uri, GFile *project_uri) {
	gchar **arr, *arr1, *arr2, *arr3;
	
	arr1 = g_file_get_uri(autosave_file);
	arr2 = document_uri ? g_file_get_uri(document_uri) : g_strdup("");
	arr3 = project_uri ? g_file_get_uri(project_uri) : g_strdup("");
	g_print("register_autosave_journal %s for %s\n",arr1,arr2);
	arr = array_from_arglist(arr1,arr2,arr3,NULL);
	main_v->autosave_journal = g_list_prepend(main_v->autosave_journal, arr);
	main_v->autosave_need_journal_save=TRUE;
	g_free(arr1);
	g_free(arr2);
	g_free(arr3);
	return main_v->autosave_journal; /* the returned GList has arr as data */
}

void remove_autosave(Tdocument *doc) {
	g_print("remove_autosave for doc %p\n",doc);
	/* delete autosaved file */
	if (doc->autosave_uri) {
		file_delete_async(doc->autosave_uri, FALSE, NULL, NULL);
	}

	/* remove from journal */
	if (doc->autosaved) {
		main_v->autosave_journal = g_list_delete_link(main_v->autosave_journal, doc->autosaved);
		doc->autosaved = NULL;
		main_v->autosave_need_journal_save=TRUE;
	}
	
	/* remove need to autosave */
	if (doc->need_autosave) {
		main_v->need_autosave = g_list_delete_link(main_v->need_autosave, doc->need_autosave);
		doc->need_autosave = NULL;
	}
	
	g_object_unref(doc->autosave_uri);
	doc->autosave_uri = NULL;
}

static void autosave_complete_lcb(gint status,gint error_info,gpointer data) {
	Tdocument *doc=data;
	g_print("autosave_complete_lcb, status=%d for doc %p\n",status,doc);
	if (status == CHECKANDSAVE_FINISHED) {
		if (!doc->autosaved) {
			doc->autosaved = register_autosave_journal(doc->autosave_uri, doc->uri, (doc->bfwin && BFWIN(doc->bfwin)->project)?BFWIN(doc->bfwin)->project->uri:NULL);
		} 
		main_v->need_autosave = g_list_delete_link(main_v->need_autosave, doc->need_autosave);
		doc->need_autosave = NULL;
		
		if (main_v->need_autosave==NULL) {
			autosave_save_journal();
		}
	}	
}

static inline void autosave(Tdocument *doc) {
	Trefcpointer *buffer;
	g_print("autosave doc %p\n",doc);
	if (!doc->autosave_uri) {
		doc->autosave_uri = create_autosave_path(doc);
	} 
	buffer = refcpointer_new(doc_get_chars(doc, 0, -1));
	file_checkNsave_uri_async(doc->autosave_uri, NULL, buffer, strlen(buffer->data), FALSE, FALSE, autosave_complete_lcb, doc);
	refcpointer_unref(buffer);
}

static gboolean run_autosave(gpointer data) {
	GList *tmplist = g_list_first(main_v->need_autosave);
	g_print("run_autosave\n");
	if (!tmplist) {
		autosave_save_journal();
		return TRUE;
	}
	while (tmplist) {
		autosave(tmplist->data);
		tmplist = g_list_next(tmplist);
	}
	return TRUE;
}

static inline void autosave_recover(Tbfwin *bfwin, GFile *file) {
	GList *list,*tmplist;
	/* TODO: recovery */	
	g_print("autosave_recover\n");
	list = get_list(file, NULL, TRUE);
	tmplist = g_list_first(list);
	while (tmplist) {
		gchar **arr = (gchar **)tmplist->data;
		GFile *uri=NULL, *recover_uri;
		if (count_array(arr)!=3) {
			tmplist = g_list_next(tmplist);
			continue;
		}
		/* TODO */
		g_print("recover %s for %s\n",arr[0],arr[1]);
		recover_uri = g_file_new_for_uri(arr[0]);
		if (arr[1] && arr[1][0] != '\0') {
			uri = g_file_new_for_uri(arr[1]);
			file_doc_from_uri(bfwin, uri, recover_uri, NULL, -1, -1, FALSE);
		} else {
			Tdocument *doc = doc_new_loading_in_background(bfwin, NULL, NULL, FALSE);
			file_into_doc(doc, recover_uri, FALSE);
			doc_set_modified(doc, TRUE);
			doc->autosave_uri = recover_uri;
			doc->autosaved = register_autosave_journal(recover_uri, NULL, NULL);
		}
		tmplist = g_list_next(tmplist);
	}
	free_stringlist(list);
	g_print("autosave_recover done\n");
}

void need_autosave(Tdocument *doc) {
	if (main_v->props.autosave) {
		if (doc->need_autosave == NULL) {
			g_print("need_autosave, mark doc %p for autosave\n",doc);
			main_v->need_autosave = g_list_prepend(main_v->need_autosave, doc);
			doc->need_autosave = main_v->need_autosave;
		}
	}
}

void autosave_init(gboolean recover, Tbfwin *bfwin) {
	g_print("autosave_init, recover=%d\n",recover);
	if (main_v->autosave_id) {
		g_source_remove(main_v->autosave_id);
	}
	main_v->autosave_need_journal_save = FALSE;
	
	if (recover) {
		GError *error=NULL;
		const gchar *tmp;
		GPatternSpec* patspec;
		GDir* gdir;
		gchar*dir;
		
		/* test if autosave directory exists */
		dir = g_strconcat(g_get_home_dir(), "/."PACKAGE"/autosave/",NULL);
		if (!g_file_test(dir, G_FILE_TEST_EXISTS)) {
			g_mkdir(dir, 0700);
		}
		g_free(dir);
		
		dir = g_strconcat(g_get_home_dir(), "/."PACKAGE"/",NULL);
		gdir = g_dir_open(dir ,0,&error);
 
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
			if (tmp && tmp[0]!='\0' && tmp[tmplen-1]!='~' && g_pattern_match(patspec, tmplen, tmp, NULL)) {
				gint pid;
				pid = get_int_from_string(tmp);
				if (kill(pid, 0)!=0) { /* process pid is not alive, or not our process which also means we should recover */
					gchar *path;
					GError *error=NULL;
					GFile *file;
					path = g_strconcat(dir, tmp, NULL);
					file = g_file_new_for_path(path);
					autosave_recover(bfwin, file);
					g_file_delete(file,NULL,&error);
					if (error) {
						g_warning("failed to delete recovered autosave_journal %s: %s\n",path,error->message);
					}
					g_free(path);
					g_object_unref(file);
				}
			}
			tmp = g_dir_read_name(gdir);
		}
		g_free(dir);
	}
	if (main_v->props.autosave) { 
		main_v->autosave_id = g_timeout_add_seconds_full(G_PRIORITY_DEFAULT, main_v->props.autosave_time, run_autosave, NULL, NULL);
	}
}

void autosave_cleanup(void) {
	GFile *file;
	GError *error=NULL;
	gchar *path = g_strdup_printf("%s/."PACKAGE"/autosave_journal.%d",g_get_home_dir(),getpid());
	g_print("autosave_cleanup\n");
	file = g_file_new_for_path(path);
	
	/* is async a good option during exit ??????? */
	g_file_delete(file,NULL,&error);
	if (error) {
		g_warning("failed to delete %s: %s\n",path,error->message);
		g_error_free(error);
	}
	g_free(path);
	g_object_unref(file);
}
