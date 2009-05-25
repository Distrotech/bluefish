
#include "bluefish.h"

static GFile * create_autosave_path(Tdocument *doc) {
	GFile *retval;
	
	/* TODO use the preferences to create a GFile */
	
	
	return retval;
}

static GList *register_autosave_journal(GFile *autosave_file, GFile *document_uri, GFile *project_uri) {
	gchar **arr, *arr1, *arr2, *arr3;
	
	arr1 = g_file_get_uri(autosave_file);
	arr2 = document_uri : g_file_get_uri(document_uri) ? g_strdup("");
	arr2 = project_uri : g_file_get_uri(project_uri) ? g_strdup("");
	
	arr = array_from_arglist(arr1,arr2,arr3,NULL);
	main_v->autosave_journal = g_list_prepend(main_v->autosave_journal, arr);
	g_free(arr1);
	g_free(arr2);
	g_free(arr3);
	return main_v->autosave_journal; /* the returned GList has arr as data */
}

void remove_autosave(Tdocument *doc) {
	
	/* TODO delete autosaved file */


	/* remove from journal */
	main_v->autosave_journal = g_list_delete_link(main_v->autosave_journal, doc->autosaved);
	doc->autosaved = NULL;
	
	/* remove need to autosave */
	if (doc->need_autosave) {
		main_v->need_autosave = g_list_delete_link(main_v->need_autosave, doc->need_autosave);
		doc->need_autosave = NULL;
	}
	
	g_object_unref(doc->autosave_uri);
	doc->autosave_uri = NULL;
}

static inline void autosave(Tdocument *doc) {
	GFile *autosave_file;
	
	if (!doc->autosave_uri) {
		doc->autosave_uri = create_autosave_path(doc);
	} 
	/*  TODO store the contents into the file */
	
	doc->autosaved = register_autosave_journal(doc->autosave_uri, doc->uri, (doc->bfwin && doc->bfwin->project):doc->bfwin->project->uri?NULL); 
	main_v->need_autosave = g_list_delete_link(main_v->need_autosave, doc->need_autosave);
	doc->need_autosave = NULL;
}

static gboolean run_autosave(gpointer data) {
	GList *tmplist = g_list_first(main_v->need_autosave);
	while (tmplist) {
		autosave(tmplist->data);
		tmplist = g_list_next(tmplist);
	}
}

static inline void autosave_recover(const gchar *filename) {
	/* TODO: recovery */	
}

void autosave_init(gboolean recover) {
	if (main_v->autosave_id) {
		g_source_remove(main_v->autosave_id);
	}
	
	if (recover) {
		GError *error=NULL;
		const gchar *tmp;
		GPatternSpec* patspec;
		GDir* gdir;
		gchar*dir = g_strconcat(g_get_home_dir(), "/."PACKAGE"/",NULL);
		gdir = g_dir_open(dir ,0,&error);
 
		if (error) {
			g_warning("failed to open %s to recover autosaved files\n", dir);
			g_free(dir);
			return;
		}
		g_free(dir);
		patspec = g_pattern_spec_new("autosave_journal.*");
		tmp = g_dir_read_name(gdir);
		while (tmp) {
			if (g_pattern_match(patspec, strlen(tmp), tmp, NULL)) {
				gint pid, ret;
				
				/* TODO extract pid from filename */
				
				if (kill(pid, 0)!=0) {
					/* process pid is not alive, or not our process which also means we should recover */
					
					autosave_recover(const gchar *filename);
					
				}
			}
			tmp = g_dir_read_name(gdir);
		}
	}
	
	main_v->autosave_id = g_timeout_add_seconds_full(G_PRIORITY_DEFAULT, main_v->props.autosave_time, run_autosave, NULL, NULL);
}

void autosave_cleanup(void) {
	
	/* TODO: remove autosave_journal file */
}
