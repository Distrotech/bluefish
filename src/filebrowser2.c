/* Bluefish HTML Editor
 * filebrowser2.c - the filebrowser v2
 *
 * Copyright (C) 2002-2013 Olivier Sessink
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
 *
 * use G_DEBUG=fatal-warnings
 * gdb src/bluefish
 * to get backtraces what causes a warning
 */

/*#define DEBUG*/

#if defined(__GNUC__) || (defined(__SUNPRO_C) && __SUNPRO_C > 0x580)
#define DBG_NONE(args...)
 /**/
#else							/* notdef __GNUC__ || __SUNPRO_C */
extern void g_none(char *first, ...);
#define DBG_NONE g_none
#endif

#define DEBUG_TREEMODELREFS DBG_NONE
#define DBG_FILTERSORT DBG_NONE
#define DBG_REFRESH DBG_NONE

/* ******* FILEBROWSER DESIGN ********
there is only one treestore left for all bluefish windows. This treestore has all files
and all directories used in all bluefish windows. This treestore has a column for the pixmap
and the name (both shown to the user), but also the GFile for the full path (excluding the
trailing slash for directories, see #1), and the type. This treestore is in main_v->fb2config. Each file
or directory added to the treestore is also added to a hashtable in main_v->fb2config

the GUI uses two filtermodels on top of the treestore, and then two sortmodels, so they can
show a specific part of the treestore (for example with a basedir other then the root, or with
some filter applied).

#1) the hash for file:///home/ is different from file:///home so we should choose which hash is
to be stored in the hashtable. The method gnome_vfs_uri_get_parent() returns directories without
the trailing slash. So it is convenient to use directories without trailing slashes all the way.

alex: g_hash_table_new(gnome_vfs_uri_hash, gnome_vfs_uri_hequal) is what you're supposed to do
*/

/*#define DBG_FBREFCOUNT*/

#include <gtk/gtk.h>
#include <string.h>

#include "bluefish.h"
#include "filebrowser2.h"
#include "file_treemodel.h"
#include "bf_lib.h"
#include "bfwin_uimanager.h"
#include "dialog_utils.h"
#include "document.h"
#include "file.h"
#include "filefilter.h"
#include "file_dialogs.h"
#include "gtk_easy.h"
#include "project.h"
#include "pixmap.h"
#include "stringlist.h"			/* add_to_history_stringlist() */

enum {
	DIR_NAME_COLUMN,
	DIR_URI_COLUMN,
	DIR_ICON_COLUMN
};

enum {
	viewmode_dual,
	viewmode_flat,
	viewmode_tree
};

typedef struct {
	GtkWidget *dirmenu_v;
	gulong dirmenu_changed_signal;
	GtkTreeModel *dirmenu_m;
	GFile *currentdir;

	GtkWidget *vbox;			/* toiplevel vbox for filebrowser widgets */
	GtkWidget *dir_v;			/* treeview widget */
	GtkWidget *vpaned;			/* added to the vbox when two panels are used */
	GtkWidget *dirscrolwin;		/* added to the vbox when one panel is used */
	GtkWidget *file_v;			/* treeview widget */

	gulong expand_signal;
	gulong dirselection_changed_id;

	GFile *basedir;
	Tfilter *curfilter;

	GtkTreeModel *dir_filter;
	GtkTreeModel *file_filter;

	gboolean last_popup_on_dir;
	gboolean filebrowser_show_hidden_files;
	gboolean filebrowser_show_backup_files;
	gint filebrowser_viewmode;
	Tbfwin *bfwin;
} Tfilebrowser2;

#define FILEBROWSER2(var) ((Tfilebrowser2 *)(var))
static void fb2_set_dirmenu(Tfilebrowser2 * fb2, GFile * newcurdir, gboolean force_rebuild);
static void refilter_filelist(Tfilebrowser2 * fb2, GtkTreePath * newroot);
static void fb2_set_viewmode_widgets(Tfilebrowser2 * fb2, gint viewmode);
static void refilter_dirlist(Tfilebrowser2 * fb2, GtkTreePath * newroot);
static void set_basedir_backend(Tfilebrowser2 *fb2, GFile *dir_uri);
static void fb2_set_basedir(Tfilebrowser2 *fb2, GFile *dir_uri);
static void change_focus_to_uri(Tfilebrowser2 *fb2, GFile *uri);
static void set_file_v_root(Tfilebrowser2 *fb2, GFile *dir_uri);


/**************/
#ifdef DEBUG
static void
debug_gfile(GFile * uri, gboolean newline)
{
	if (uri) {
		gchar *name = g_file_get_uri(uri);
		g_print("%s (%p)%s", name, uri, newline ? "\n" : "");
		g_free(name);
	} else {
		g_print("(GFile=NULL)%s", newline ? "\n" : "");
	}
}
#endif

#ifdef DEBUG
#define DEBUG_GFILE debug_gfile
#else							/* not DEBUG */
#ifdef __GNUC__
#define DEBUG_GFILE(args...)
 /**/
#else							/* notdef __GNUC__ */
extern void g_none(gchar * first, ...);
#define DEBUG_GFILE g_none
#endif							/* __GNUC__ */
#endif							/* DEBUG */

#ifdef DBG_FBREFCOUNT
guint fake_finfo_ref = 0;
#endif

/*****************************************************************************************************************************/
/*                 UTILITY FUNCTIONS                             */
/*****************************************************************************************************************************/

#define dir_v_filter_path_from_treestore_path(fb2, treestorepath) gtk_tree_model_filter_convert_child_path_to_path(GTK_TREE_MODEL_FILTER(fb2->dir_filter), treestorepath)
#define file_v_filter_path_from_treestore_path(fb2, treestorepath) gtk_tree_model_filter_convert_child_path_to_path(GTK_TREE_MODEL_FILTER(fb2->file_filter), treestorepath)

/*
treepath_for_uri

returns a GtkTreePath for the uri, and builds it (using fb2_build_dir()) if needed
*/

static GtkTreePath *
treepath_for_uri(Tfilebrowser2 * fb2, GFile * uri)
{
	GtkTreeIter iter;
	if (!uri)
		return NULL;

	if (!filetree_get_iter_for_uri(FB2CONFIG(main_v->fb2config)->ftm, uri, &iter)) {
		if (!filetreemodel_build_dir(FB2CONFIG(main_v->fb2config)->ftm, uri, &iter))
			return NULL;
	}
	return gtk_tree_model_get_path(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->ftm), &iter);
}


/* Gets treepath from uri; treepath is required for scrolling to it or selecting
 * It combines the logic of two functions above ( treepath_for_uri() and and
 dir_filter_path_from_treestore_path() ) and can be used for quick document
 treepath selection after we refresh directory and required iter is already in hash table
 */

/*static GtkTreePath *dir_sort_path_from_uri(Tfilebrowser2 * fb2, GFile * uri)
{
	GtkTreePath *fs_path, *sort_path;

	fs_path = treepath_for_uri(fb2, uri);
	if (!fs_path)
		return NULL;

	sort_path = dir_v_filter_path_from_treestore_path(fb2, fs_path);
	gtk_tree_path_free(fs_path);
	return sort_path;
}*/

/*static gboolean
need_to_scroll_to_dir(Tfilebrowser2 * fb2, GFile *diruri)
{
	GtkTreePath *start_path=NULL, *end_path=NULL;
	GtkTreeIter it1;
	gboolean retval=FALSE;
	if (!gtk_tree_view_get_visible_range(GTK_TREE_VIEW(fb2->dir_v),&start_path,&end_path))
		return FALSE;

	if (!gtk_tree_model_get_iter(fb2->dir_filter,&it1,start_path)) {
		UriRecord *record=NULL;

		gtk_tree_model_get(fb2->dir_filter, &it1, filetreemodel_COL_RECORD, &record, -1);

		if (record) {
			/ * now see if diruri is the parent of uri * /
			retval = !gfile_uri_is_parent(diruri,record->uri,TRUE);
		}
	}
	gtk_tree_path_free(start_path);
	gtk_tree_path_free(end_path);
	return retval;
}*/

static void
expand_without_directory_refresh(Tfilebrowser2 * fb2, GtkTreePath *sort_path)
{
	g_signal_handler_block(fb2->dir_v, fb2->expand_signal);
	DEBUG_MSG("expand_without_directory_refresh, sort+path=%p\n",sort_path);
	gtk_tree_view_expand_to_path(GTK_TREE_VIEW(fb2->dir_v), sort_path);
	g_signal_handler_unblock(fb2->dir_v, fb2->expand_signal);
}

/**
 * fb2_fspath_from_uri:
 *
 * return a newly allocated treepath for the filesystem_tstore based on 'uri'
 */
/*static GtkTreePath *
fb2_fspath_from_uri(Tfilebrowser2 * fb2, GFile * uri)
{
	GtkTreeIter iter;
	if (filetree_get_iter_for_uri(FB2CONFIG(main_v->fb2config)->ftm, uri, &iter)) {
		return gtk_tree_model_get_path(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->ftm), &iter);
	}
	return NULL;
}*/

/**
 * fb2_uri_from_fspath:
 *
 * returns the uri stored in the treestore based on the 'fs_path' from the filesystem treestore
 * /
static GFile *
fb2_uri_from_fspath(Tfilebrowser2 * fb2, GtkTreePath * fs_path)
{
	if (fs_path) {
		GtkTreeIter fsiter;
		if (gtk_tree_model_get_iter
			(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->ftm), &fsiter, fs_path)) {
			return fb2_uri_from_iter(&fsiter);
		} else {
			DEBUG_MSG("fb2_uri_from_fspath, WARNING, no fsiter for fs_path=%p ", fs_path);
			DEBUG_TPATH(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->ftm), fs_path, TRUE);
		}
	}
	return NULL;
}*/

/**
 * fb2_isdir_from_dir_sort_path:
 *
 * returns TRUE if 'sort_path' from the directory sort model points to a directory
 */
static gboolean
fb2_isdir_from_dir_sort_path(Tfilebrowser2 * fb2, GtkTreePath * sort_path)
{
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter(fb2->dir_filter, &iter, sort_path)) {
		UriRecord *record=NULL;
		gtk_tree_model_get(fb2->dir_filter, &iter, filetreemodel_COL_RECORD, &record, -1);
		return record->isdir;
	}
	return FALSE;
}

/**
 * fb2_uri_from_dir_selection:
 *
 * returns a newly allocated treepath for the current selected entry in the dir view
 */
static GFile *
fb2_uri_from_dir_selection(Tfilebrowser2 * fb2)
{
	GFile *uri = NULL;
	GtkTreeModel *sort_model;
	GtkTreeIter sort_iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(fb2->dir_v));
	if (gtk_tree_selection_get_selected(selection, &sort_model, &sort_iter)) {
		UriRecord *record=NULL;
		gtk_tree_model_get(sort_model, &sort_iter, filetreemodel_COL_RECORD, &record, -1);
		if (record) {
			uri = record->uri;
			g_object_ref(uri);
		}
	}
	return uri;
}

/**
 * fb2_uri_from_file_selection:
 *
 * returns the uri stored in the treestore for the current selected entry in the file_v
 */
static GFile *
fb2_uri_from_file_selection(Tfilebrowser2 * fb2, gchar ** mime)
{
	GtkTreeModel *sort_model;
	GtkTreeIter sort_iter;
	UriRecord *record=NULL;
	if (fb2->file_v
		&& gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(fb2->file_v)),
										   &sort_model, &sort_iter)) {
		gtk_tree_model_get(sort_model, &sort_iter, filetreemodel_COL_RECORD, &record, -1);
	} else if (!fb2->file_v && fb2->dir_v
			   &&
			   gtk_tree_selection_get_selected(gtk_tree_view_get_selection
											   (GTK_TREE_VIEW(fb2->dir_v)), &sort_model, &sort_iter)) {
		gtk_tree_model_get(sort_model, &sort_iter, filetreemodel_COL_RECORD, &record, -1);
	}
#ifdef DEBUG
	else
		DEBUG_MSG("fb2_uri_from_file_selection, no selection!\n");
#endif
	if (record) {
		if (mime) {
			*mime = record->fast_content_type;
		}
		return record->uri;
	}
	return NULL;
}

static void
add_uri_to_recent_dirs(Tfilebrowser2 * fb2, GFile * uri)
{
	gchar *tmp;

	tmp = g_file_get_uri(uri);
	DEBUG_MSG("add_uri_to_recent_dirs, add %s to history stringlist\n",tmp);
	fb2->bfwin->session->recent_dirs =
		add_to_history_stringlist(fb2->bfwin->session->recent_dirs, tmp, TRUE);
	g_free(tmp);
}

static void
remove_uri_from_recent_dirs(Tfilebrowser2 * fb2, GFile * uri)
{
	gchar *tmp;

	tmp = g_file_get_uri(uri);

	fb2->bfwin->session->recent_dirs =
		remove_from_stringlist(fb2->bfwin->session->recent_dirs, tmp);
	g_free(tmp);
}


/* Return common uri part of two input uri's. Required for setting basedir when activating or opening file that is outside current basedir */

GFile *find_common_path(GFile *file1, GFile *file2)
{
	if (!file1 || !file2)
		return NULL;
	gchar * filename1 = g_file_get_parse_name(file1);
	gchar * filename2 = g_file_get_parse_name(file2);
	DEBUG_MSG("find_common_path, filename1=%s, filename2=%s \n", filename1, filename2);
	int  pos;
	for (pos = 0; ; pos++) {
		if (filename1[pos] != '\0' && filename1[pos] == filename2[pos])
			continue;
		if (filename1[pos] != G_DIR_SEPARATOR && filename2[pos] != G_DIR_SEPARATOR) {
				/* scroll back */
			while (pos > 0 && filename1[--pos] != G_DIR_SEPARATOR);
		} else {
			if (filename1[pos] == G_DIR_SEPARATOR)
				--pos;
		}
		if (pos==0) {
			DEBUG_MSG("find_common_path, sorry, common path is not found");
			g_free(filename1);
			g_free(filename2);
			return NULL;
		}
		gchar *common_path = g_strndup (filename1, pos);
		DEBUG_MSG("find_common_path, found commom path is %s, pos=%d \n", common_path, pos);
		GFile *common_uri = g_file_parse_name(common_path);
		g_free(filename1);
		g_free(filename2);
		g_free(common_path);
		if (g_file_query_exists(common_uri,NULL)) {
			return common_uri;
		} else {
			g_object_unref(common_uri);
			g_warning("find_common_path, there is some error, common path does not exist");
			return NULL;
		}
	}
	return NULL;
}


/*****************************************************************************************************************************/
/*  END OF UTILITY FUNCTIONS - START OF DIRECTORY RE-READ FUNCTIONS*/
/*****************************************************************************************************************************/

gboolean delayed_refresh_cb(gpointer data)
{
	if (FB2CONFIG(main_v->fb2config)->uri_to_refresh) {
		DEBUG_MSG("delayed_refresh_cb, refresh uri %s\n",g_file_get_path(FB2CONFIG(main_v->fb2config)->uri_to_refresh));
		filetreemodel_refresh_uri_async(FB2CONFIG(main_v->fb2config)->ftm, FB2CONFIG(main_v->fb2config)->uri_to_refresh);
		g_object_unref(FB2CONFIG(main_v->fb2config)->uri_to_refresh);
	}
	FB2CONFIG(main_v->fb2config)->uri_to_refresh = NULL;
	FB2CONFIG(main_v->fb2config)->delayed_refresh_id = 0;
	return FALSE;
}

/*
by delaying the refresh, if quickly change multiple tabs, you won't call a directory refresh for every document, only
a dir for a document where you stay more than 200ms will be refreshed
*/
static void
register_delayed_refresh(GFile *uri) {
	if (!uri)
		return;
	DEBUG_MSG("register_delayed_refresh, called for %s\n",g_file_get_path(uri));
	if (uri_in_refresh(FB2CONFIG(main_v->fb2config)->ftm, uri)) {
		if (FB2CONFIG(main_v->fb2config)->uri_to_refresh && g_file_equal(FB2CONFIG(main_v->fb2config)->uri_to_refresh,uri)) {
			g_object_unref(FB2CONFIG(main_v->fb2config)->uri_to_refresh);
			FB2CONFIG(main_v->fb2config)->uri_to_refresh = NULL;
			if (FB2CONFIG(main_v->fb2config)->delayed_refresh_id) {
				g_source_remove(FB2CONFIG(main_v->fb2config)->delayed_refresh_id);
			}
		}
		DEBUG_MSG("register_delayed_refresh, uri=%s is already in refresh, returning\n",g_file_get_path(uri));
		return;
	}

	if (FB2CONFIG(main_v->fb2config)->uri_to_refresh && g_file_equal(FB2CONFIG(main_v->fb2config)->uri_to_refresh,uri)) {
		return;
	}
	if (FB2CONFIG(main_v->fb2config)->uri_to_refresh)
		g_object_unref(FB2CONFIG(main_v->fb2config)->uri_to_refresh);
	FB2CONFIG(main_v->fb2config)->uri_to_refresh = uri;
	g_object_ref(FB2CONFIG(main_v->fb2config)->uri_to_refresh);
	if (FB2CONFIG(main_v->fb2config)->delayed_refresh_id) {
		g_source_remove(FB2CONFIG(main_v->fb2config)->delayed_refresh_id);
	}
	FB2CONFIG(main_v->fb2config)->delayed_refresh_id = g_timeout_add_full( G_PRIORITY_LOW,200,delayed_refresh_cb,NULL,NULL);
}

/**
 * fb2_refresh_parent_of_uri:
 *
 * convenience function, will refresh the parent directory of child_uri
 */
void
fb2_refresh_parent_of_uri(GFile * child_uri)
{
	GFile *parent_uri = g_file_get_parent(child_uri);
	if (parent_uri) {
		filetreemodel_refresh_uri_async(FB2CONFIG(main_v->fb2config)->ftm, parent_uri);
		g_object_unref(parent_uri);
	}
}

/*****************************************************************************************************************************/
/*  END OF DIRECTORY RE-READ FUNCTIONS - START OF FILTERING FUNCTIONS*/
/*****************************************************************************************************************************/


/**
 * tree_model_filter_func
 *
 * will return TRUE if this file should be visible in the dir view
 */
static gboolean
tree_model_filter_func(GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
	Tfilebrowser2 *fb2 = data;
	gint len;
	gboolean retval = TRUE;
	UriRecord *record=NULL;
	DBG_FILTERSORT("tree_model_filter_func, model=%p, fb2=%p, model,fb2, iter %p with iter->user_data=%p\n",model,fb2,iter,iter?iter->user_data:NULL);
	gtk_tree_model_get(GTK_TREE_MODEL(model), iter, filetreemodel_COL_RECORD, &record, -1);
	if (!record) {
		g_warning("tree_model_filter_func, model=%p, fb2=%p, iter=%p, iter->user_data=%p, item without record!!\n",model, fb2, iter, iter?iter->user_data:NULL);
		return FALSE;
	}

	if (!record->isdir) {	/* file */
		if (fb2->filebrowser_viewmode == viewmode_dual) {
			/* in the two paned view we don't show files in the dir view */
			retval = FALSE;
		} else {
			if (fb2->filebrowser_viewmode == viewmode_flat) {
				if (fb2->basedir) {
					if (!gfile_uri_is_parent(fb2->basedir, record->uri, FALSE)) {
						retval = FALSE;
					}
				}
			}
			if (retval && !fb2->filebrowser_show_backup_files) {
				len = strlen(record->name);
				if (len > 1 && (record->name[len - 1] == '~'))
					retval = FALSE;
			}
			if (retval && !fb2->filebrowser_show_hidden_files) {
				if (record->name[0] == '.')
					retval = FALSE;
			}
			if (fb2->basedir && !gfile_uri_is_parent(fb2->basedir, record->uri, TRUE)) {
				retval = FALSE;
			}
			if (retval && fb2->curfilter)
				retval = file_visible_in_filter(fb2->curfilter, record->fast_content_type, record->name);
		}
		DBG_FILTERSORT("file %s with mime_type %s: returning %d\n", record->name, record->fast_content_type, retval);
	} else {					/* directory */
		if (fb2->filebrowser_viewmode == viewmode_flat) {
			if (fb2->basedir) {
				/* show only the level of our basedir no deeper directories */
				if (!gfile_uri_is_parent(fb2->basedir, record->uri, FALSE)) {
					retval = FALSE;
				}
			}
		} else {
			if (fb2->basedir) {
				/* show only our basedir on the root level, no other directories at that level */
				if (!gfile_uri_is_parent(fb2->basedir, record->uri, TRUE)
					&& !g_file_equal(fb2->basedir, record->uri)) {
					retval = FALSE;
				}
			}
		}
		if (retval && !fb2->filebrowser_show_hidden_files) {
			if (record->name[0] == '.')
				retval = FALSE;
		}
		DBG_FILTERSORT("dir %s: returning %d\n", record->name, retval);
	}
	return retval;
}

/**
 * file_list_filter_func:
 *
 * will return TRUE if this file should be visible in the file list
 */
static gboolean
file_list_filter_func(GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
	Tfilebrowser2 *fb2 = data;
	gint len;
	gboolean retval = TRUE;
	UriRecord *record=NULL;
	DBG_FILTERSORT("file_list_filter_func, called for model=%p and fb2=%p, iter %p with iter->user_data=%p\n",model,fb2,iter,iter?iter->user_data:NULL);
	gtk_tree_model_get(GTK_TREE_MODEL(model), iter, filetreemodel_COL_RECORD, &record, -1);

	if (!record)
		return FALSE;

	if (record->isdir)
		return FALSE;

	if (retval && !fb2->filebrowser_show_backup_files) {
		len = strlen(record->name);
		if (len > 1 && (record->name[len - 1] == '~'))
			retval = FALSE;
	}
	if (retval && !fb2->filebrowser_show_hidden_files) {
		if (record->name[0] == '.')
			retval = FALSE;
	}
	if (retval && fb2->curfilter)
		retval = file_visible_in_filter(fb2->curfilter, record->fast_content_type, record->name);
#ifdef DEBUG
	if (retval == FALSE) {
		DBG_FILTERSORT("file_list_filter_func, hiding %s (%s)\n", record->name, record->fast_content_type);
	}
#endif
	return retval;
}

static void
fb2_refilter(Tfilebrowser2 * fb2)
{
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(fb2->dir_filter));
	if (fb2->filebrowser_viewmode == viewmode_dual) {
		gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(fb2->file_filter));
	}
}

/*****************************************************************************************************************************/
/*  END OF FILTERING FUNCTIONS, START OF DATA MODEL FUNCTIONS */
/*****************************************************************************************************************************/

void
fb2_file_is_opened(GFile *uri, const gchar *mimetype)
{
	GtkTreeIter iter;
	if (!uri)
		return;
	DEBUG_MSG("fb2_file_is_opened, called for %s with mimetype %s\n",g_file_get_path(uri),mimetype);
	if (!filetree_get_iter_for_uri(FB2CONFIG(main_v->fb2config)->ftm, uri, &iter)) {
		/* add entry */
		filetreemodel_add_file(FB2CONFIG(main_v->fb2config)->ftm, uri, mimetype, PANGO_WEIGHT_BOLD);
	} else {
		filetreemodel_set_weight(FB2CONFIG(main_v->fb2config)->ftm, uri, PANGO_WEIGHT_BOLD);
	}
}

void fb2_file_is_closed(GFile *uri)
{
	if (!uri)
		return;
	filetreemodel_set_weight(FB2CONFIG(main_v->fb2config)->ftm, uri, PANGO_WEIGHT_NORMAL);
}

static void
fb2config_set_documentroot_icon(GFile *uri)
{
	if (!uri)
		return;
	filetreemodel_set_icon(FB2CONFIG(main_v->fb2config)->ftm, uri,BF_STOCK_BROWSER_PREVIEW);
}

/* if a filter is deleted, this function is called to make sure
the current filebrowser isn't actually using the filter */
void
fb2_unset_filter(Tbfwin * bfwin, Tfilter * filter)
{
	if (bfwin->fb2 && FILEBROWSER2(bfwin->fb2)->curfilter == filter) {
		FILEBROWSER2(bfwin->fb2)->curfilter = NULL;
		fb2_refilter(FILEBROWSER2(bfwin->fb2));
	}
}

void
fb2config_init(void)
{
	Tfilebrowser2config *fb2config;
	DEBUG_MSG("fb2config_init, started\n");
	/* a lot of data can be stored once for all windows;
	   This will be initialized in this function */
	fb2config = g_new0(Tfilebrowser2config, 1);
	main_v->fb2config = fb2config;
	fb2config->ftm = filetreemodel_new();
}

#ifdef MEMORY_LEAK_DEBUG

/* avoid segfaults during bluefish exit */
void
fb2config_cleanup(void)
{
	g_object_unref(FB2CONFIG(main_v->fb2config)->ftm);
	DEBUG_TREEMODELREFS("fb2config_cleanup, unreffed filesystem treestore at %p\n",FB2CONFIG(main_v->fb2config)->ftm);
	g_free(main_v->fb2config);
}
#endif

/*****************************************************************************************************************************/
/*  END OF DATA MODEL FUNCTIONS, START OF DIALOGS / MENU FUNCTIONS */
/*****************************************************************************************************************************/


typedef struct {
	Tbfwin *bfwin;
	GtkWidget *win;
	GtkWidget *doc_entry;
	GtkWidget *web_entry;
	GFile *documentroot;
	GFile *webroot;
} Tdocrootdialog;

static void
drd_response_lcb(GtkDialog * dialog, gint response, Tdocrootdialog * drd)
{
	if (response == GTK_RESPONSE_ACCEPT) {
		GFile *docroot, *webroot;

		docroot = g_file_new_for_uri(gtk_entry_get_text(GTK_ENTRY(drd->doc_entry)));
		webroot = g_file_new_for_uri(gtk_entry_get_text(GTK_ENTRY(drd->web_entry)));

		if (docroot && webroot) {
			if (drd->bfwin->session->documentroot)
				g_free(drd->bfwin->session->documentroot);
			drd->bfwin->session->documentroot = g_file_get_uri(docroot);
			fb2config_set_documentroot_icon(docroot);

			if (drd->bfwin->session->webroot)
				g_free(drd->bfwin->session->webroot);
			drd->bfwin->session->webroot = g_file_get_uri(webroot);

			g_object_unref(docroot);
			g_object_unref(webroot);
		}
	}
	gtk_widget_destroy(drd->win);
	if (drd->webroot)
		g_object_unref(drd->webroot);
	if (drd->documentroot)
		g_object_unref(drd->documentroot);
	g_free(drd);
}

static void
set_documentroot_dialog(Tbfwin * bfwin, GFile * uri)
{
	/* now start the dialog to set the webroot */
	GtkWidget *table, *label;
	gchar *tmp;
	Tdocrootdialog *drd;
	drd = g_new0(Tdocrootdialog, 1);
	drd->bfwin = bfwin;
	drd->documentroot = uri;
	g_object_ref(drd->documentroot);
	drd->win =
		gtk_dialog_new_with_buttons(_("Set documentroot and webroot"),
									GTK_WINDOW(bfwin->main_window), GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_OK,
									GTK_RESPONSE_ACCEPT, NULL);
	g_signal_connect(G_OBJECT(drd->win), "response", G_CALLBACK(drd_response_lcb), drd);
	table = dialog_table_in_vbox_defaults(2, 2, 5, gtk_dialog_get_content_area(GTK_DIALOG(drd->win)));
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),
						 _
						 ("The <i>documentroot</i> is the place where Bluefish finds the file,\nthe <i>webroot</i> is the location where those same files can be viewed on the webserver"));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 2, 0, 1);

	tmp = g_file_get_uri(uri);
	drd->doc_entry = dialog_entry_in_table(tmp, table, 1, 2, 1, 2);
	gtk_editable_set_editable(GTK_EDITABLE(drd->doc_entry), FALSE);
	g_free(tmp);
	dialog_mnemonic_label_in_table(_("Documentroot"), drd->doc_entry, table, 0, 1, 1, 2);

	if (bfwin->session->webroot) {
		tmp = g_strdup(bfwin->session->webroot);
	} else if (g_file_has_uri_scheme(uri, "file")) {
		tmp = g_strdup("http://localhost/");
	} else {
		tmp = g_strdup("http://");
	}
	drd->web_entry = dialog_entry_in_table(tmp, table, 1, 2, 2, 3);
	g_free(tmp);
	dialog_mnemonic_label_in_table(_("Webroot"), drd->web_entry, table, 0, 1, 2, 3);

	gtk_widget_show_all(drd->win);
}



static void
rename_not_open_file(Tbfwin * bfwin, GFile * olduri)
{
	gchar *newfilename = NULL, *oldfilename, *dialogtext;
	/* Promt user, "File/Move To"-style. */
	oldfilename = g_file_get_uri(olduri);
	dialogtext = g_strdup_printf(_("Move/rename %s to"), oldfilename);
	newfilename = ask_new_filename(bfwin, oldfilename, dialogtext);
	g_free(dialogtext);
	if (newfilename) {
		GFile *newuri = NULL;
		gboolean res;
		GError *error = NULL;
		newuri = g_file_new_for_uri(newfilename);
		res = g_file_move(olduri, newuri, G_FILE_COPY_NONE, NULL, NULL, NULL, &error);

		if (!res) {
			gchar *errmessage = g_strconcat(_("Could not rename\n"), oldfilename, NULL);
			message_dialog_new(bfwin->main_window, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, errmessage, NULL);
			g_free(errmessage);
		} else {
			GFile *parent1, *parent2;

/*
			GtkTreeIter *olditer;
			olditer = g_hash_table_lookup(FB2CONFIG(main_v->fb2config)->filesystem_itable, olduri);
			if (olditer)
				fb2_treestore_delete_children(FB2CONFIG(main_v->fb2config)->ftm, olditer,
											  FALSE);
*/
			parent1 = g_file_get_parent(olduri);
			parent2 = g_file_get_parent(newuri);
			DEBUG_MSG("parent1=");
			DEBUG_GFILE(parent1, FALSE);
			DEBUG_MSG(", parent2=");
			DEBUG_GFILE(parent2, TRUE);
			if (parent1 && parent2 && !g_file_equal(parent1, parent2)) {
				fb2_refresh_parent_of_uri(olduri);
				fb2_refresh_parent_of_uri(newuri);
			} else if (parent1) {
				DEBUG_MSG("rename_not_open_file, refresh parent1");
				DEBUG_GFILE(parent1, TRUE);
				filetreemodel_refresh_uri_async(FB2CONFIG(main_v->fb2config)->ftm, parent1);
			} else if (parent2) {
				filetreemodel_refresh_uri_async(FB2CONFIG(main_v->fb2config)->ftm, parent2);
			}
			g_object_unref(parent1);
			g_object_unref(parent2);
		}
		g_free(newfilename);
		g_object_unref(newuri);
	}
	g_free(oldfilename);
}

static void
fb2rpopup_new(Tfilebrowser2 * fb2, gboolean newisdir, GFile * noselectionbaseuri)
{
	GFile *baseuri = NULL;
	if (fb2->last_popup_on_dir) {
		baseuri = fb2_uri_from_dir_selection(fb2);
		/* fb2_uri_from_dir_selection returns a pointer without an extra reference, so we ref it */
		if (baseuri) {
			g_object_ref(baseuri);
			DEBUG_MSG("fb2rpopup_new, baseuri from dir selection=");
			DEBUG_GFILE(baseuri, TRUE);
		}
	} else {
		GFile *childuri = fb2_uri_from_file_selection(fb2, NULL);
		/* fb2_uri_from_file_selection returns a pointer without an extra reference, so we ref it */
		if (childuri) {
			baseuri = g_file_get_parent(childuri);
			DEBUG_MSG("fb2rpopup_new, baseuri from file selection=");
			DEBUG_GFILE(baseuri, TRUE);
		}
	}
	if (!baseuri) {
		/* no selection, try the noselectionbaseuri and give it an extra reference */
		baseuri = noselectionbaseuri;
		g_object_ref(baseuri);
		DEBUG_MSG("fb2rpopup_new, baseuri from noselectionbaseuri=");
		DEBUG_GFILE(baseuri, TRUE);
	}

	if (baseuri) {
		GFile *newuri;
		gboolean done = FALSE;
		DEBUG_MSG("fb2rpopup_new, baseuri=");
		DEBUG_GFILE(baseuri, TRUE);
		if (newisdir) {
			GError *error = NULL;
			newuri = g_file_get_child(baseuri, _("New directory"));
			if (!newuri) {
				DEBUG_MSG("failed to create GFile for baseuri ???????????????????????????\n");
				g_object_unref(baseuri);
				return;
			}
			DEBUG_MSG("fb2rpopup_new, newuri=");
			DEBUG_GFILE(newuri, TRUE);
			done = g_file_make_directory(newuri, NULL, &error);
			if (error) {
				gchar *tmp = g_file_get_uri(newuri);
				DEBUG_MSG("fb2rpopup_new, failed to create directory %s: %s, done=%d\n", tmp, error->message,
						done);
				g_error_free(error);
				g_free(tmp);
			} else {
				rename_not_open_file(fb2->bfwin, newuri);
			}
		} else {
			gint counter = 0;
			gchar *filename;
			do {
				GFileOutputStream *gfos;
				GError *error = NULL;
				if (counter == 0)
					filename = g_strdup(_("New file"));
				else
					filename = g_strdup_printf("%s%d", _("New file"), counter);
				newuri = g_file_get_child(baseuri, filename);
				g_free(filename);
				DEBUG_MSG("fb2rpopup_new, newuri=");
				DEBUG_GFILE(newuri, TRUE);
				gfos = g_file_create(newuri, G_FILE_CREATE_NONE, NULL, &error);
				if (gfos) {
					g_output_stream_close((GOutputStream *) gfos, NULL, &error);
					done = TRUE;
					counter = 100;
				} else if (error && error->code == G_IO_ERROR_EXISTS) {
					counter++;
					g_object_unref(newuri);
					g_error_free(error);
				} else {
					counter = 100;
					if (error) {
						DEBUG_MSG("fb2rpopup_new, failed to create file: %s\n", error->message);
						g_error_free(error);
					}
				}
			} while (counter < 100);
			if (done) {
				rename_not_open_file(fb2->bfwin, newuri);
			}
		}
		if (done) {
			DEBUG_MSG("calling for newuri %p %s\n", newuri, g_file_get_uri(newuri));
			fb2_refresh_parent_of_uri(newuri);
		}
		g_object_unref(newuri);
		g_object_unref(baseuri);
	} else {
		DEBUG_MSG("not creating new file => no baseuri, perhaps we should use the basedir ??\n");
	}
}

static void
rcpopup_async_delete_lcb(gpointer data)
{
	GFile *uri = data;
	if (uri) {
		GList *alldocs;
		Tdocument *exdoc;
		alldocs = return_allwindows_documentlist();
		exdoc = documentlist_return_document_from_uri(alldocs, uri);
		if (exdoc) {
			document_unset_filename(exdoc);
		}
		g_list_free(alldocs);

		fb2_refresh_parent_of_uri(uri);

		g_object_unref(uri);
	}
}

static void
popup_menu_delete(GtkAction * action, gpointer user_data)
{
	Tfilebrowser2 *fb2 = FILEBROWSER2(user_data);
	GFile *uri;

	if (fb2->last_popup_on_dir) {
		uri = fb2_uri_from_dir_selection(fb2);
	} else {
		uri = fb2_uri_from_file_selection(fb2, NULL);
	}
	if (uri) {
		const gchar *buttons[] = { GTK_STOCK_CANCEL, GTK_STOCK_DELETE, NULL };
		gchar *text, *text2;
		gint retval;
		gchar *fullpath, *filename;
		fullpath = g_file_get_uri(uri);
		filename = gfile_display_name(uri, NULL);
		/* Do we really need to display the full path here?
		 *
		 * Having the filename in the both the primary and secondary text seems to be redundant.
		 * Set back to just the primary text for now.
		 *
		 * --> Sometimes you can have a file in many directories (e.g. Makefile.in, index.html)
		 * and then this might give you more indication if this is indeed the file you wanted to delete
		 */
		text = g_strdup_printf(_("Are you sure you want to delete\n\"%s\"?"), filename);
		text2 = g_strdup_printf(_("If you delete %s, it will be permanently lost."), fullpath);
		retval =
			message_dialog_new_multi(fb2->bfwin->main_window, GTK_MESSAGE_QUESTION, buttons, text, text2);
		g_free(text);
		g_free(text2);
		if (retval == 1) {
			/* ref the uri, it is unreffed by the callback */
			g_object_ref(uri);
			DEBUG_MSG("fb2rpopup_delete, calling file_delete_file_async\n");
			file_delete_async(uri, FALSE, rcpopup_async_delete_lcb, uri);
		}
		g_free(filename);
		g_free(fullpath);
	}
}

static void
popup_menu_filter_activate(GtkAction * action, gpointer user_data)
{

	Tbfwin *bfwin=user_data;
	Tfilebrowser2 *fb2;

	if (!bfwin || !bfwin->fb2)
		return;

	fb2 = FILEBROWSER2(bfwin->fb2);
	if (gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action))) {
		/* loop trough the filters for a filter with this name */
		const gchar *name = gtk_action_get_name(action);
		Tfilter *filter = find_filter_by_name(name);

		DEBUG_MSG("popup_menu_filter_activate, setting curfilter to %p from name %s\n", filter, name);
		fb2->curfilter = filter;
		if (filter) {
			if (fb2->bfwin->session->last_filefilter)
				g_free(fb2->bfwin->session->last_filefilter);
			fb2->bfwin->session->last_filefilter = g_strdup(filter->name);
		}
		fb2_refilter(fb2);
	}
}

static void
popup_menu_follow_active_doc(GtkAction * action, gpointer user_data)
{
	Tfilebrowser2 *fb2 = FILEBROWSER2(user_data);

	fb2->bfwin->session->filebrowser_focus_follow = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));

	if (fb2->bfwin->session->filebrowser_focus_follow && fb2->bfwin->current_document)
		fb2_focus_document(fb2->bfwin, fb2->bfwin->current_document);
}

static void
popup_menu_new_directory(GtkAction * action, gpointer user_data)
{
	Tfilebrowser2 *fb2 = FILEBROWSER2(user_data);

	fb2rpopup_new(fb2, TRUE, fb2->basedir);
}

static void
popup_menu_new_file(GtkAction * action, gpointer user_data)
{
	Tfilebrowser2 *fb2 = FILEBROWSER2(user_data);

	if (fb2->file_v == NULL) {
		fb2rpopup_new(fb2, FALSE, fb2->basedir);
	} else {
		fb2rpopup_new(fb2, FALSE, fb2->currentdir);
	}
}

static void
popup_menu_open(GtkAction * action, gpointer user_data)
{
	Tfilebrowser2 *fb2 = FILEBROWSER2(user_data);
	GFile *uri;
	gchar *mime = NULL;

	uri = fb2_uri_from_file_selection(fb2, &mime);
	if (uri) {
		DEBUG_MSG("calling file_handle for popup menu\n");
		file_handle(uri, fb2->bfwin, mime, TRUE);
	}
}

static void
popup_menu_open_advanced(GtkAction * action, gpointer user_data)
{
	Tfilebrowser2 *fb2 = FILEBROWSER2(user_data);
	GFile *uri = fb2_uri_from_dir_selection(fb2);
	gchar *curi;

	curi = g_file_get_uri(uri);
	files_advanced_win(fb2->bfwin, curi);
	g_free(curi);
}

static void
popup_menu_refresh(GtkAction * action, gpointer user_data)
{
	Tfilebrowser2 *fb2 = FILEBROWSER2(user_data);
	GFile *baseuri = NULL;
	gboolean unref_baseuri = FALSE;

	if (fb2->last_popup_on_dir) {
		baseuri = fb2_uri_from_dir_selection(fb2);	/* returns the uri in the treestore */
	} else {
		GFile *childuri = fb2_uri_from_file_selection(fb2, NULL);	/* returns the uri in the treestore */
		if (childuri) {
			baseuri = g_file_get_parent(childuri);
			unref_baseuri = TRUE;
		}
	}
	if (!baseuri
		&& (fb2->filebrowser_viewmode == viewmode_flat || fb2->filebrowser_viewmode == viewmode_dual)) {
		/* in flat view or dual view we can refresh the basedir now */
		baseuri = fb2->basedir;
	}
	if (baseuri) {
		filetreemodel_refresh_uri_async(FB2CONFIG(main_v->fb2config)->ftm, baseuri);
		if (unref_baseuri)
			g_object_unref(baseuri);
	}
}

static void
popup_menu_rename(GtkAction * action, gpointer user_data)
{
	Tfilebrowser2 *fb2 = FILEBROWSER2(user_data);
	GFile *olduri;

	if (fb2->last_popup_on_dir) {
		olduri = fb2_uri_from_dir_selection(fb2);
	} else {
		olduri = fb2_uri_from_file_selection(fb2, NULL);
	}
	if (olduri) {
		Tdocument *tmpdoc;
		GList *alldocs;

		g_object_ref(olduri);

		/* Use doc_save(doc, 1, 1) if the file is open. */

		alldocs = return_allwindows_documentlist();
		tmpdoc = documentlist_return_document_from_uri(alldocs, olduri);
		g_list_free(alldocs);
		if (tmpdoc != NULL) {
			DEBUG_MSG("fb2rpopup_rename, file is open. Calling doc_save() with 'do_move'.\n");
			/* If an error occurs, doc_save takes care of notifying the user.
			 * Currently, nothing is done here. */
			doc_save_backend(tmpdoc, docsave_move, FALSE, FALSE);
		} else {				/* olduri is not open */
			rename_not_open_file(fb2->bfwin, olduri);
		}
		g_object_unref(olduri);
	}
}


static void
popup_menu_set_basedir(GtkAction * action, gpointer user_data)
{
	Tfilebrowser2 *fb2 = FILEBROWSER2(user_data);
	GtkTreePath *fs_path;
	GtkTreeIter iter;

	GFile *new_basedir = fb2_uri_from_dir_selection(fb2);
	fb2_set_basedir(fb2, new_basedir);
	g_object_unref(new_basedir);
	if (fb2->bfwin->session->filebrowser_focus_follow && fb2->bfwin->current_document) {
		fb2_focus_document(fb2->bfwin, fb2->bfwin->current_document);
	} else {
		gtk_tree_model_get_iter_first(fb2->dir_filter, &iter);
		fs_path = gtk_tree_model_get_path(fb2->dir_filter, &iter);
		if (!gtk_tree_view_row_expanded(GTK_TREE_VIEW(fb2->dir_v), fs_path)) {
			gtk_tree_view_expand_row(GTK_TREE_VIEW(fb2->dir_v), fs_path, FALSE);
		}
		gtk_tree_path_free(fs_path);
	}
}

static void
popup_menu_set_document_root(GtkAction * action, gpointer user_data)
{
	Tfilebrowser2 *fb2 = FILEBROWSER2(user_data);
	GFile *uri = fb2_uri_from_dir_selection(fb2);

	set_documentroot_dialog(fb2->bfwin, uri);
}

static void
popup_menu_show_backup_files(GtkAction * action, gpointer user_data)
{
	Tfilebrowser2 *fb2 = FILEBROWSER2(user_data);

	fb2->filebrowser_show_backup_files = fb2->bfwin->session->filebrowser_show_backup_files =
		gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	fb2_refilter(fb2);
}

static void
popup_menu_show_hidden_files(GtkAction * action, gpointer user_data)
{
	Tfilebrowser2 *fb2 = FILEBROWSER2(user_data);

	fb2->filebrowser_show_hidden_files = fb2->bfwin->session->filebrowser_show_hidden_files =
		gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	fb2_refilter(fb2);
}

static void
popup_menu_show_full_tree(GtkAction * action, gpointer user_data)
{
	Tfilebrowser2 *fb2 = FILEBROWSER2(user_data);
	set_basedir_backend(fb2, NULL);
	if (fb2->bfwin->current_document && fb2->bfwin->session->filebrowser_focus_follow)
		fb2_focus_document(fb2->bfwin, fb2->bfwin->current_document);
}

static void
popup_menu_view_mode_changed(GtkRadioAction * action, GtkRadioAction * current, gpointer user_data)
{
	Tbfwin *bfwin = user_data;
	Tfilebrowser2 *fb2;
	gint view_mode;

	if (!bfwin || !bfwin->fb2)
		return;

	fb2 = FILEBROWSER2(bfwin->fb2);
	view_mode = gtk_radio_action_get_current_value(action);

	fb2->bfwin->session->filebrowser_viewmode = view_mode;
	fb2_set_viewmode_widgets(fb2, view_mode);

	/* Focus to current document in order to update fb view  when viewmode changes*/
	if (fb2->bfwin->session->filebrowser_focus_follow && fb2->bfwin->current_document)
		fb2_focus_document(fb2->bfwin, fb2->bfwin->current_document);
}

static void
popup_menu_delete_filter(GtkAction * action, gpointer user_data)
{
	filter_delete(FILEBROWSER2(user_data)->curfilter);
}

static void
popup_menu_edit_filter(GtkAction * action, gpointer user_data)
{
	filefilter_gui(FILEBROWSER2(user_data)->curfilter);
}

static void
popup_menu_new_filter(GtkAction * action, gpointer user_data)
{
	filefilter_gui(NULL);
}

static const gchar *filebrowser_menu_ui =
	"<ui>"
	"  <popup action='FileBrowserMenu'>"
	"    <menuitem action='FB2Open'/>"
	"    <menuitem action='FB2OpenAdvanced'/>"
	"    <menuitem action='FB2Rename'/>"
	"    <menuitem action='FB2Delete'/>"
	"    <separator/>"
	"    <menuitem action='FB2NewFile'/>"
	"    <menuitem action='FB2NewDirectory'/>"
	"    <separator/>"
	"    <menuitem action='FB2Refresh'/>"
	"    <menuitem action='FB2FollowActiveDoc'/>"
	"    <menuitem action='FB2SetBaseDir'/>"
	"    <menuitem action='FB2SetDocumentRoot'/>"
	"    <separator/>"
	"    <menuitem action='FB2ShowFullTree'/>"
	"    <menuitem action='FB2ShowBackupFiles'/>"
	"    <menuitem action='FB2ShowHiddenFiles'/>"
	"    <separator/>"
	"    <menu action='FB2ViewModeMenu'>"
	"      <menuitem action='FB2ViewModeDual'/>"
	"      <menuitem action='FB2ViewModeFlat'/>"
	"      <menuitem action='FB2ViewModeTree'/>"
	"    </menu>"
	"    <menu action='FB2FilterMenu'>"
	"      <placeholder name='FB2FilterPlaceholder'/>"
	"      <separator/>"
	"      <menuitem action='FB2NewFilter'/>"
	"      <menuitem action='FB2EditFilter'/>"
	"      <menuitem action='FB2DeleteFilter'/>"
	"    </menu>"
	"  </popup>"
	"</ui>";

static const GtkActionEntry filebrowser_actions[] = {
	{"FB2FileBrowserMenu", NULL, N_("File Browser menu")},
	{"FB2Open", NULL, N_("_Open"), NULL, N_("Open"), G_CALLBACK(popup_menu_open)},
	{"FB2OpenAdvanced", NULL, N_("Open _Advanced..."), NULL, N_("Open advanced"),
	 G_CALLBACK(popup_menu_open_advanced)},
	{"FB2Rename", NULL, N_("Rena_me"), NULL, N_("Rename"), G_CALLBACK(popup_menu_rename)},
	{"FB2Delete", NULL, N_("_Delete"), NULL, N_("Delete"), G_CALLBACK(popup_menu_delete)},
	{"FB2NewFile", NULL, N_("New _File"), NULL, N_("New file"), G_CALLBACK(popup_menu_new_file)},
	{"FB2NewDirectory", NULL, N_("_New Directory"), NULL, N_("New directory"),
	 G_CALLBACK(popup_menu_new_directory)},
	{"FB2Refresh", NULL, N_("_Refresh"), NULL, N_("Refresh"), G_CALLBACK(popup_menu_refresh)},
	{"FB2SetBaseDir", NULL, N_("_Set as base dir"), NULL, N_("Set as base directory"),
	 G_CALLBACK(popup_menu_set_basedir)},
	{"FB2SetDocumentRoot", NULL, N_("Set as document root"), NULL, N_("Set as document root"),
	 G_CALLBACK(popup_menu_set_document_root)},
	{"FB2ShowFullTree", NULL, N_("Show Full _Tree"), NULL, N_("Show full tree"),
	 G_CALLBACK(popup_menu_show_full_tree)},
	{"FB2ViewModeMenu", NULL, N_("View Mode")},
	{"FB2FilterMenu", NULL, N_("Filter")},
	{"FB2NewFilter", NULL, N_("New Filter"), NULL, N_("New filter"), G_CALLBACK(popup_menu_new_filter)},
	{"FB2EditFilter", NULL, N_("Edit Filter"), NULL, N_("Edit filter"), G_CALLBACK(popup_menu_edit_filter)},
	{"FB2DeleteFilter", NULL, N_("Delete Filter"), NULL, N_("Delete filter"), G_CALLBACK(popup_menu_delete_filter)}
};

static const GtkToggleActionEntry filebrowser_toggle_actions[] = {
	{"FB2FollowActiveDoc", NULL, N_("Follow Active Document"), NULL, N_("Follow active document"),
	 G_CALLBACK(popup_menu_follow_active_doc), FALSE},
	{"FB2ShowBackupFiles", NULL, N_("Show Backup Files"), NULL, N_("Show backup files"),
	 G_CALLBACK(popup_menu_show_backup_files), FALSE},
	{"FB2ShowHiddenFiles", NULL, N_("Show Hidden Files"), NULL, N_("Show hidden files"),
	 G_CALLBACK(popup_menu_show_hidden_files), FALSE}
};

static const GtkRadioActionEntry filebrowser_radio_actions[] = {
	{"FB2ViewModeDual", NULL, N_("Dual"), NULL, NULL, viewmode_dual},
	{"FB2ViewModeFlat", NULL, N_("Flat"), NULL, NULL, viewmode_flat},
	{"FB2ViewModeTree", NULL, N_("Tree"), NULL, NULL, viewmode_tree}
};

static void
popup_menu_action_group_init(Tbfwin * bfwin)
{
	Tfilebrowser2 *fb2 = FILEBROWSER2(bfwin->fb2);
	GError *error = NULL;

	bfwin->filebrowserGroup = gtk_action_group_new("FileBrowserActions");
	gtk_action_group_set_translation_domain(bfwin->filebrowserGroup, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(bfwin->filebrowserGroup, filebrowser_actions, G_N_ELEMENTS(filebrowser_actions), fb2);
	gtk_action_group_add_toggle_actions(bfwin->filebrowserGroup, filebrowser_toggle_actions,
										G_N_ELEMENTS(filebrowser_toggle_actions), fb2);
	DEBUG_MSG("popup_menu_action_group_init, set default viewmode to %d\n",fb2->filebrowser_viewmode);
	gtk_action_group_add_radio_actions(bfwin->filebrowserGroup, filebrowser_radio_actions,
									   G_N_ELEMENTS(filebrowser_radio_actions),
									   fb2->filebrowser_viewmode, G_CALLBACK(popup_menu_view_mode_changed),
									   bfwin);
	gtk_ui_manager_insert_action_group(bfwin->uimanager, bfwin->filebrowserGroup, 1);
	g_object_unref(bfwin->filebrowserGroup);

	bfwin->filebrowser_merge_id = gtk_ui_manager_add_ui_from_string(bfwin->uimanager, filebrowser_menu_ui, -1, &error);
	if (error != NULL) {
		g_warning("building file browser popup menu failed: %s", error->message);
		g_error_free(error);
	}
}

static void
popup_menu_create(Tfilebrowser2 * fb2, gboolean is_directory, gboolean is_file, GdkEventButton * event)
{
	Tbfwin *bfwin = fb2->bfwin;
	GtkWidget *menu;
	GSList *group = NULL;
	GList *list;
	gint value = 0;

	menu = gtk_ui_manager_get_widget(bfwin->uimanager, "/FileBrowserMenu");
	if (!menu) {
		g_warning("showing file browser popup menu failed");
		return;
	}
	fb2->last_popup_on_dir = is_directory;
	/* We need to disconnect signals during toggling the action since this emits the signal and filebrowser follows to active documents on first activation of popup */
	GtkToggleAction *follow_active_doc_action = (GtkToggleAction *)gtk_ui_manager_get_action(bfwin->uimanager, "/FileBrowserMenu/FB2FollowActiveDoc");
	if (gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(follow_active_doc_action)) != fb2->bfwin->session->filebrowser_focus_follow) {
		g_signal_handlers_disconnect_by_func(GTK_TOGGLE_ACTION(follow_active_doc_action), G_CALLBACK(popup_menu_follow_active_doc), fb2);
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(follow_active_doc_action), fb2->bfwin->session->filebrowser_focus_follow);
		g_signal_connect(G_OBJECT(follow_active_doc_action), "toggled", G_CALLBACK(popup_menu_follow_active_doc), fb2);
	}
	bfwin_set_menu_toggle_item_from_path(bfwin->uimanager, "/FileBrowserMenu/FB2ShowBackupFiles",
										 fb2->filebrowser_show_backup_files);
	bfwin_set_menu_toggle_item_from_path(bfwin->uimanager, "/FileBrowserMenu/FB2ShowHiddenFiles",
										 fb2->filebrowser_show_hidden_files);

	bfwin_action_set_sensitive(bfwin->uimanager, "/FileBrowserMenu/FB2Rename", (is_directory || is_file));
	bfwin_action_set_sensitive(bfwin->uimanager, "/FileBrowserMenu/FB2Delete", (is_directory || is_file));
	bfwin_action_set_sensitive(bfwin->uimanager, "/FileBrowserMenu/FB2OpenAdvanced", is_directory);
	bfwin_action_set_sensitive(bfwin->uimanager, "/FileBrowserMenu/FB2SetDocumentRoot", is_directory);
	bfwin_action_set_sensitive(bfwin->uimanager, "/FileBrowserMenu/FB2SetBaseDir", is_directory);
	bfwin_action_set_sensitive(bfwin->uimanager, "/FileBrowserMenu/FB2Open", is_file);
	bfwin_action_set_sensitive(bfwin->uimanager, "/FileBrowserMenu/FB2ShowFullTree", (fb2->basedir != NULL && fb2->filebrowser_viewmode != viewmode_flat));

	gtk_radio_action_set_current_value((GtkRadioAction *)gtk_ui_manager_get_action(bfwin->uimanager, "/FileBrowserMenu/FB2ViewModeMenu/FB2ViewModeDual"),fb2->filebrowser_viewmode);

	if (!bfwin->fb2_filters_group) {
		bfwin->fb2_filters_group = gtk_action_group_new("FB2FileBrowserFilterActions");
		gtk_ui_manager_insert_action_group(bfwin->uimanager, bfwin->fb2_filters_group, 1);
	} else {
		GList *actions, *list;

		gtk_ui_manager_remove_ui(bfwin->uimanager, bfwin->fb2_filters_merge_id);

		actions = gtk_action_group_list_actions(bfwin->fb2_filters_group);
		for (list = actions; list; list = list->next) {
			g_signal_handlers_disconnect_by_func(GTK_ACTION(list->data),
												 G_CALLBACK(popup_menu_filter_activate), bfwin);
			gtk_action_group_remove_action(bfwin->fb2_filters_group, GTK_ACTION(list->data));
		}
		g_list_free(actions);
	}

	bfwin->fb2_filters_merge_id = gtk_ui_manager_new_merge_id(bfwin->uimanager);

	for (list = g_list_last(main_v->filefilters); list; list = list->prev) {
		Tfilter *filter = (Tfilter *) list->data;
		GtkRadioAction *action;

		action = gtk_radio_action_new(filter->name, filter->name, NULL, NULL, value);
		gtk_action_group_add_action(bfwin->fb2_filters_group, GTK_ACTION(action));
		gtk_radio_action_set_group(action, group);
		group = gtk_radio_action_get_group(action);

		g_signal_connect(G_OBJECT(action), "activate", G_CALLBACK(popup_menu_filter_activate), bfwin);

		gtk_ui_manager_add_ui(bfwin->uimanager, bfwin->fb2_filters_merge_id,
							  "/FileBrowserMenu/FB2FilterMenu/FB2FilterPlaceholder", filter->name,
							  filter->name, GTK_UI_MANAGER_MENUITEM, FALSE);

		if (fb2->curfilter == filter)
			gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), TRUE);
	}

	gtk_widget_show(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
}



/*****************************************************************************************************************************/
/*  END OF DIALOGS / MENU FUNCTIONS FUNCTIONS, START OF MAIN GUI FUNCTIONS */
/*****************************************************************************************************************************/


/**
 * refilter_dirlist:
 *
 * will set the root of the directory view to 'newroot'
 *
 * returns TRUE if a sub-root is set, returns FALSE if the filter shows everything from the real-root
 *
 */
static void
refilter_dirlist(Tfilebrowser2 * fb2, GtkTreePath * newroot)
{
	GtkTreePath *useroot = NULL;

	if (newroot) {
		/* to make it possible to select the root in treeview or dual-view, we move the filter-root one up */
		useroot = gtk_tree_path_copy(newroot);
		if (fb2->filebrowser_viewmode != viewmode_flat) {
			if (!(gtk_tree_path_get_depth(newroot) > 1) || !gtk_tree_path_up(useroot)) {
				/* do not set the root as basedir, it is useless  */
				DEBUG_MSG("refilter_dirlist, there is no parent for this path, so we will set the filter root to NULL\n");
				gtk_tree_path_free(useroot);
				useroot = NULL;
			}
		}
	}
	fb2->dir_filter =
		gtk_tree_model_filter_new(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->ftm), useroot);
	DEBUG_TREEMODELREFS("refilter_dirlist, created new tree model filter at %p for fb2 %p\n",fb2->dir_filter, fb2);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(fb2->dir_filter),
										   tree_model_filter_func, fb2, NULL);

	gtk_tree_view_set_model(GTK_TREE_VIEW(fb2->dir_v), GTK_TREE_MODEL(fb2->dir_filter));
	/* we remove our reference, so the only reference is kept by the treeview, if the treeview is destroyed, the models will be destroyed */
	g_object_unref(fb2->dir_filter);
	if (useroot)
		gtk_tree_path_free(useroot);
}

/**
 * refilter_filelist:
 *
 * will set the root of the file view to 'newroot'
 *
 */
static void
refilter_filelist(Tfilebrowser2 * fb2, GtkTreePath * newroot)
{
	DEBUG_MSG("refilter_filelist, started for fb2=%p, file_filter=%p, viewmode=%d, newroot=%p\n", fb2,
			  fb2->file_filter, fb2->filebrowser_viewmode, newroot);
	if (fb2->filebrowser_viewmode == viewmode_dual) {
		if (fb2->file_filter) {
			GtkTreePath *curpath;
			gboolean equal;
			g_object_get(fb2->file_filter, "virtual-root", &curpath, NULL);
			equal = (curpath == NULL && newroot == NULL) || (curpath != NULL && newroot != NULL
															 && gtk_tree_path_compare(curpath, newroot) == 0);
			gtk_tree_path_free(curpath);
			if (equal) {
#ifdef DEBUG
				DEBUG_MSG("refilter_filelist, root did not change!\n");
#endif
				return;
			}
		}
		fb2->file_filter =
			gtk_tree_model_filter_new(GTK_TREE_MODEL
									  (FB2CONFIG(main_v->fb2config)->ftm), newroot);
		DEBUG_TREEMODELREFS("refilter_filelist, created new tree model filter at %p for fb2 %p\n",fb2->file_filter, fb2);
		DEBUG_MSG("refilter_filelist, set file list filter func, fb2=%p\n", fb2);
		gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(fb2->file_filter),
											   file_list_filter_func, fb2, NULL);
		gtk_tree_view_set_model(GTK_TREE_VIEW(fb2->file_v), GTK_TREE_MODEL(fb2->file_filter));
		/* we remove our reference, so the only reference is kept by the treeview, if the treeview is destroyed, the models will be destroyed */
		g_object_unref(fb2->file_filter);
	}
}

static void
set_file_v_root(Tfilebrowser2 *fb2, GFile *dir_uri)
{
	GtkTreeIter dir_iter;
	GtkTreePath *dir_path;

	DEBUG_MSG("set_file_v_root, dir_uri=%p\n",dir_uri);
	if (fb2->filebrowser_viewmode != viewmode_dual) {
		g_warning("set_file_v_root called but viewmode is not dual ????\n");
		return;
	}
	if (!filetree_get_iter_for_uri(FB2CONFIG(main_v->fb2config)->ftm, dir_uri, &dir_iter)) {
		return;
	}
	dir_path = gtk_tree_model_get_path(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->ftm), &dir_iter);
	refilter_filelist(fb2, dir_path);
	gtk_tree_path_free(dir_path);
}

/**
 * scroll_to_uri
 *
 * scrolls to this uri.
 *
 *
*/
static void
scroll_to_iter(Tfilebrowser2 *fb2, GtkTreeIter *file_iter, GtkTreeIter *dir_iter)
{
	GtkTreePath *fs_path, *filter_path;
	GtkTreeSelection *dirselection;
	if (fb2->filebrowser_viewmode == viewmode_dual && file_iter) {
		fs_path = gtk_tree_model_get_path(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->ftm), file_iter);
		if (!fs_path) {
			DEBUG_MSG("scroll_to_iter, for file_v fs_path==NULL !?!?!?!\n");
			return;
		}
		filter_path = file_v_filter_path_from_treestore_path(fb2, fs_path);
		if (filter_path) { /* possible the file was not yet created in the treestore, or outside the filter */
			gtk_tree_selection_select_path(gtk_tree_view_get_selection
													   (GTK_TREE_VIEW(fb2->file_v)), filter_path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(fb2->file_v), filter_path, 0, TRUE, 0.5, 0.5);
			DEBUG_MSG("scroll_to_iter, selected/scrolled on file_v\n");
		}
		fs_path = gtk_tree_model_get_path(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->ftm), dir_iter);
	} else {
		fs_path = gtk_tree_model_get_path(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->ftm), file_iter ? file_iter : dir_iter);
	}
	if (!fs_path) {
		DEBUG_MSG("scroll_to_iter, for dir_v fs_path==NULL !?!?!?!\n");
		return;
	}

	filter_path = dir_v_filter_path_from_treestore_path(fb2, fs_path);
	if (!filter_path) {
		gtk_tree_path_free(fs_path);
		DEBUG_MSG("scroll_to_iter, no filter_path, return\n");
		return;
	}
	DEBUG_MSG("scroll_to_iter, got filter_path=%p\n",filter_path);
	expand_without_directory_refresh(fb2, filter_path);
	dirselection = gtk_tree_view_get_selection(GTK_TREE_VIEW(fb2->dir_v));
	if (fb2->dirselection_changed_id)
		g_signal_handler_block(dirselection, fb2->dirselection_changed_id);
	gtk_tree_selection_select_path(gtk_tree_view_get_selection
													   (GTK_TREE_VIEW(fb2->dir_v)), filter_path);
	if (fb2->dirselection_changed_id)
		g_signal_handler_unblock(dirselection, fb2->dirselection_changed_id);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(fb2->dir_v), filter_path, 0, TRUE, 0.5, 0.5);
	gtk_tree_path_free(fs_path);
	gtk_tree_path_free(filter_path);
}

static void
switch_to_directory(Tfilebrowser2 *fb2, GFile *dir_uri)
{
	GtkTreeIter dir_iter;
	DEBUG_MSG("switch_to_directory for dir_uri %p\n",dir_uri);
	/* check if the requested dir is within the basedir, or if we have viewmode flat */
	if ((fb2->filebrowser_viewmode == viewmode_flat)
				||  (fb2->basedir && (!gfile_uri_is_parent(fb2->basedir, dir_uri, TRUE) && !g_file_equal(fb2->basedir, dir_uri)))) {
		set_basedir_backend(fb2, dir_uri);
	}

	if (!filetree_get_iter_for_uri(FB2CONFIG(main_v->fb2config)->ftm, dir_uri, &dir_iter)) {
		filetreemodel_build_dir(FB2CONFIG(main_v->fb2config)->ftm, dir_uri, &dir_iter);
	}
	if (fb2->filebrowser_viewmode == viewmode_dual) {
		set_file_v_root(fb2, dir_uri);
	}
	scroll_to_iter(fb2, NULL, &dir_iter);
	filetreemodel_refresh_iter_async(FB2CONFIG(main_v->fb2config)->ftm, &dir_iter);
	/* last entry in recent_dirs is used as basedir of the project, so we can add entries this way only in flat mode, which does not have basedir
	*  For other modes the right thing to do would be to add uri not to last postion, but one before last, so basedir will not change.
	*  Saved basedir should be changed only from popup menu
	*/
	if ((fb2->filebrowser_viewmode == viewmode_flat) || !fb2->bfwin->project)
		add_uri_to_recent_dirs(fb2, dir_uri);
}

/**
 * change_focus_to_uri
 *
 * builds the directory tree (if the directory does not yet exist)
 * in dual view changes the displayed directory for the file_v
 * and scrolls to this file (and in dual view also to the dir).
 *
 * no callbacks will be called for expand signals,
 * the directory will be re-read ONLY IF THE FILE DOES NOT YET EXIST IN THE TREESTORE
 * the dirmenu will not reflect this change.
*/
static void
change_focus_to_uri(Tfilebrowser2 *fb2, GFile *uri)
{
	GtkTreeIter dir_iter, iter;
	GFile *dir_uri;
	if (!uri) {
		return;
	}
	DEBUG_MSG("change_focus_to_uri, called for %s\n",g_file_get_path(uri));
	dir_uri = g_file_get_parent(uri);
	if (!filetree_get_iter_for_uri(FB2CONFIG(main_v->fb2config)->ftm, uri, &iter)) {
		if (!filetree_get_iter_for_uri(FB2CONFIG(main_v->fb2config)->ftm, dir_uri, &dir_iter)) {
			DEBUG_MSG("change_focus_to_uri, need to refresh directory for doc_uri=%p and/or dir_uri=%p\n", uri, dir_uri);
			filetreemodel_build_dir(FB2CONFIG(main_v->fb2config)->ftm, dir_uri, &dir_iter);
			filetreemodel_refresh_iter_async(FB2CONFIG(main_v->fb2config)->ftm, &dir_iter);
		}
		g_object_unref(dir_uri);
		return;
	}
	if (!filetree_get_iter_for_uri(FB2CONFIG(main_v->fb2config)->ftm, dir_uri, &dir_iter)) {
		/* weird ?? file exists, but parent directory not ? */
		g_assert_not_reached();
	}
	if (fb2->filebrowser_viewmode == viewmode_dual) {
		set_file_v_root(fb2, dir_uri);
	}
	scroll_to_iter(fb2, &iter, &dir_iter);
	fb2_set_dirmenu(fb2, dir_uri, FALSE);
	g_object_unref(dir_uri);
}

/*

 * for viewmode dual and tree: if the dir is outside the basedir, this will
 * change the basedir

*/

void
fb2_follow_uri(Tbfwin *bfwin, GFile *uri)
{
	if (bfwin && uri) {
		Tfilebrowser2 *fb2 = bfwin->fb2;
		GFile *dir_uri = g_file_get_parent(uri);
		DEBUG_MSG("fb2_follow_uri, started for uri %p\n",uri);
		/* In flat mode we always have to change basedir */
		if (fb2->filebrowser_viewmode == viewmode_flat) {
			if (!gfile_uri_is_parent(fb2->basedir, uri, FALSE)) {
				set_basedir_backend(bfwin->fb2, dir_uri);
			}
		} else {
			if (!gfile_uri_is_parent(fb2->basedir, uri, TRUE)) {
				GFile *new_basedir = find_common_path(dir_uri, fb2->basedir);
				set_basedir_backend(bfwin->fb2, new_basedir);
				if (new_basedir)
					g_object_unref(new_basedir);
			}
		}
		/* now build the directory and focus */
		change_focus_to_uri(bfwin->fb2, uri);
		register_delayed_refresh(dir_uri);
		g_object_unref(dir_uri);
	}
}


static gboolean
dirmenu_idle_cleanup_lcb(gpointer callback_data)
{
	GtkTreeModel *oldmodel = callback_data;
	gboolean cont;
	GtkTreeIter iter;
	DEBUG_SIG("dirmenu_idle_cleanup_lcb, priority=%d\n",G_PRIORITY_LOW+10);
	DEBUG_MSG("dirmenu_idle_cleanup_lcb, cleanup the old model %p\n", oldmodel);
	/* now we cleanup the old model and it's contents */

	cont = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(oldmodel), &iter);
	while (cont) {
		GFile *uri = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(oldmodel), &iter, DIR_URI_COLUMN, &uri, -1);
		if (uri) {
			DEBUG_MSG("dirmenu_idle_cleanup_lcb, unref uri %p\n",uri);
			g_object_unref(uri);
		}
		/* hmm if this last remove results in an empty liststore there is a crash?? */
		cont = gtk_list_store_remove(GTK_LIST_STORE(oldmodel), &iter);
	}
	DEBUG_MSG("dirmenu_idle_cleanup_lcb, unref the old model\n");
	g_object_unref(oldmodel);
	return FALSE;
}

/**
 * fb2_set_dirmenu
 *
 * sets the dirmenu to another directory
 * no callbacks will be called
*/
static void
fb2_set_dirmenu(Tfilebrowser2 *fb2, GFile *newcurdir, gboolean force_rebuild)
{
	GtkTreeIter iter, setiter;
	GHashTable *hasht;
	GList *tmplist;
	GtkTreeModel *oldmodel = fb2->dirmenu_m;
	GFile *tmp;
	GVolumeMonitor *gvolmon;
	gboolean cont, havesetiter = FALSE;
	DEBUG_MSG("fb2_set_dirmenu(fb2=%p, newcurdir uri=%p)\n", fb2, newcurdir);
	if (fb2->currentdir) {
		if (newcurdir && !force_rebuild && (fb2->currentdir == newcurdir || g_file_equal(fb2->currentdir, newcurdir)))
			return;
		g_object_unref(fb2->currentdir);
	}
	fb2->currentdir = g_object_ref(newcurdir);
#ifdef DEBUG
	DEBUG_MSG("fb2_set_dirmenu, newcurdir=");
	DEBUG_GFILE(newcurdir, TRUE);
#endif

	fb2->dirmenu_m = GTK_TREE_MODEL(gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING));

	hasht = g_hash_table_new_full(g_file_hash, (GEqualFunc) g_file_equal, NULL, NULL);

	/* rebuild the current uri */
	tmp = g_object_ref(newcurdir);
	do {
		gchar *name;
		GFile *tmp2;
		tmp2 = g_file_get_parent(tmp);
		DEBUG_MSG("parent for uri %p is at %p\n", tmp, tmp2);

		gtk_list_store_append(GTK_LIST_STORE(fb2->dirmenu_m), &iter);
		if (!havesetiter) {
			setiter = iter;
			havesetiter = TRUE;
		}
		name = g_file_is_native(tmp) ? g_file_get_path(tmp) : g_file_get_uri(tmp);
		DEBUG_MSG("fb2_set_dirmenu, appended %s (uri=%p) to the new model %p\n", name, tmp, fb2->dirmenu_m);
		if (tmp2 == NULL) {
			gtk_list_store_set(GTK_LIST_STORE(fb2->dirmenu_m), &iter, DIR_NAME_COLUMN, name, DIR_URI_COLUMN, tmp	/* don't unref tmp at this point, because there is a reference in the model */
							   , DIR_ICON_COLUMN, "drive-harddisk", -1);
		} else {
			gtk_list_store_set(GTK_LIST_STORE(fb2->dirmenu_m), &iter, DIR_NAME_COLUMN, name, DIR_URI_COLUMN, tmp	/* don't unref tmp at this point, because there is a reference in the model */
							   , DIR_ICON_COLUMN, "folder", -1);
		}
		g_hash_table_insert(hasht, tmp, GINT_TO_POINTER(1));
		g_free(name);
		tmp = tmp2;
		cont = (tmp != NULL);
	} while (cont);

	tmplist = g_list_first(fb2->bfwin->session->recent_dirs);
	while (tmplist) {
		GFile *uri;
		gchar *name;

		uri = g_file_new_for_uri(tmplist->data);
		DEBUG_MSG("new uri at %p for session recent directory %s\n", uri, (gchar *)tmplist->data);
		if (uri && g_hash_table_lookup(hasht, uri) == NULL) {
			name = g_file_is_native(uri) ? g_file_get_path(uri) : g_file_get_uri(uri);
			DEBUG_MSG("fb2_set_dirmenu, appending %s (uri=%p) to model %p\n", name, uri, fb2->dirmenu_m);
			gtk_list_store_append(GTK_LIST_STORE(fb2->dirmenu_m), &iter);
			gtk_list_store_set(GTK_LIST_STORE(fb2->dirmenu_m), &iter, DIR_NAME_COLUMN, name,
							   DIR_URI_COLUMN, uri, DIR_ICON_COLUMN, "folder", -1);
			g_hash_table_insert(hasht, uri, GINT_TO_POINTER(1));
			g_free(name);
		} else if (uri) {
			g_object_unref(uri);
		}
		tmplist = g_list_next(tmplist);
	}

	gvolmon = g_volume_monitor_get();
	{
		GList *mountlist = tmplist = g_list_first(g_volume_monitor_get_mounts(gvolmon));
		while (tmplist) {
			GMount *gmount = tmplist->data;
			gchar *name;
			GFile *root;
			name = g_mount_get_name(gmount);
			root = g_mount_get_root(gmount);
			if (g_hash_table_lookup(hasht, root) == NULL) {
				gtk_list_store_append(GTK_LIST_STORE(fb2->dirmenu_m), &iter);
				gtk_list_store_set(GTK_LIST_STORE(fb2->dirmenu_m), &iter, DIR_NAME_COLUMN, name, DIR_URI_COLUMN, root	/* don't unref root at this point, because there is a reference in the model */
								   , DIR_ICON_COLUMN, "drive-harddisk", -1);
				g_hash_table_insert(hasht, root, GINT_TO_POINTER(1));

			}
			DEBUG_MSG("set_dirmenu, found mount %s\n", name);
			g_free(name);
			g_object_unref(gmount);
			tmplist = tmplist->next;
		}
		g_list_free(mountlist);
	}
	g_object_unref(gvolmon);
	g_hash_table_destroy(hasht);

	DEBUG_MSG("fb2_set_dirmenu, activate the new model\n");
	g_signal_handler_block(fb2->dirmenu_v, fb2->dirmenu_changed_signal);
	gtk_combo_box_set_model(GTK_COMBO_BOX(fb2->dirmenu_v), GTK_TREE_MODEL(fb2->dirmenu_m));
	if (havesetiter)
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(fb2->dirmenu_v), &setiter);
	g_signal_handler_unblock(fb2->dirmenu_v, fb2->dirmenu_changed_signal);
	DEBUG_MSG("fb2_set_dirmenu, activated the new model!\n");
	g_idle_add_full(G_PRIORITY_LOW+10, dirmenu_idle_cleanup_lcb, oldmodel, NULL);

}

/**
 * fb2_set_dir_v_root
 *
 * refilters the directory view to have this uri as visual root
 *
 *
*/
static void
set_dir_v_root(Tfilebrowser2 *fb2, GFile *dir_uri)
{
	GtkTreePath *basepath = NULL;
	GtkTreeSelection *dirselection;
	DEBUG_MSG("set_dir_v_root for uri %p\n",dir_uri);
	/* disconnect the dir_v and file_v for higher performance */

	dirselection = gtk_tree_view_get_selection(GTK_TREE_VIEW(fb2->dir_v));
	if (fb2->dirselection_changed_id)
		g_signal_handler_block(dirselection, fb2->dirselection_changed_id); /* block signal while we are reconfiguting tree model */

	gtk_tree_view_set_model(GTK_TREE_VIEW(fb2->dir_v), NULL);
	if (fb2->filebrowser_viewmode == viewmode_dual)
		gtk_tree_view_set_model(GTK_TREE_VIEW(fb2->file_v), NULL);
	fb2->file_filter = NULL;	/* this is required, because the refilter_filelist function tries to check if the virtual root did change for the file filter */
	DEBUG_MSG("fb2_set_dir_v_root, disconnected current filter/sort models, lfilter=%p\n",
			  fb2->file_filter);

	if (dir_uri) {
		/* treepath_for_uri returns a GtkTreePath for the uri, and builds it (using fb2_build_dir()) if needed */
		if (fb2->filebrowser_viewmode != viewmode_flat)
			filetreemodel_refresh_uri_async(FB2CONFIG(main_v->fb2config)->ftm, dir_uri);
		basepath = treepath_for_uri(fb2, dir_uri);
		DEBUG_MSG("fb2_set_dir_v_root, refilter, basepath=%p\n", basepath);
		if (basepath) {
			refilter_dirlist(fb2, basepath);
			gtk_tree_path_free(basepath);
		}
	} else {
		refilter_dirlist(fb2, NULL);
	}
	if (fb2->dirselection_changed_id)
		g_signal_handler_unblock(dirselection, fb2->dirselection_changed_id);
}


/**
 * set_basedir_backend
 *
 * use NULL to unset the basedir
 *
 * sets fb2->basedir
 * makes it the visual root
 *
 * does NOT scroll to the active document or register this basedir in the history !!!!!!
 */
static void
set_basedir_backend(Tfilebrowser2 *fb2, GFile *dir_uri)
{

	if (dir_uri && fb2->basedir && (fb2->basedir == dir_uri || g_file_equal(fb2->basedir, dir_uri))) {
		DEBUG_MSG("set_basedir_backend, basedir did not change, do nothing\n");
		return;
	}
	DEBUG_MSG("set_basedir_backend, new basedir %p\n", dir_uri);
	if (fb2->basedir)
		g_object_unref(fb2->basedir);
	fb2->basedir = dir_uri;
	if (dir_uri) {
		g_object_ref(fb2->basedir);
	}
	set_dir_v_root(fb2, dir_uri);
}

/**
 * fb2_set_basedir
 *
 * to be called from the popup menu
 *
 * use NULL to unset the basedir
 *
 * sets fb2->basedir
 * makes it the visual root
 * DOES add it to the history - TODO
 * DOES expand and scroll to the current document - TODO
 */
static void
fb2_set_basedir(Tfilebrowser2 *fb2, GFile *dir_uri)
{
	DEBUG_MSG("fb2_set_basedir, dir_uri=%p\n",dir_uri);
	set_basedir_backend(fb2, dir_uri);
	if (dir_uri)
		add_uri_to_recent_dirs(fb2, dir_uri);
}

/* reset basedir to HOME */
gboolean
fb2_check_reset_basedir(GFile *uri)
{
	GList *tmplist;
	GFile *fallback_basedir;
	gboolean retval;
	Tfilebrowser2 * fb2;
	retval=FALSE;
	tmplist = g_list_first(main_v->bfwinlist);
	while (tmplist) {
		fb2 = BFWIN(tmplist->data)->fb2;
		if (fb2->basedir && (fb2->basedir == uri || g_file_equal(fb2->basedir, uri))) {
			/* Reset basedir to some location that is known to exist */
			g_warning("fb2_check_reset_basedir, basedir %s does not exists, setting basedir to $HOME", g_file_get_path(uri));
			retval=TRUE;
			fallback_basedir = g_file_new_for_path(g_get_home_dir()); /*If  basedir is not valid default to home directory */
			fb2_set_basedir(fb2, fallback_basedir);
			fb2_set_dirmenu(fb2, fallback_basedir, FALSE);
			if (fb2->filebrowser_viewmode == viewmode_dual) {
				set_file_v_root(fb2, fallback_basedir);
			}
			filetreemodel_refresh_uri_async(FB2CONFIG(main_v->fb2config)->ftm, fallback_basedir);
			/* now expand it */
			GtkTreePath *fs_path, *filter_path;
			fs_path = treepath_for_uri(fb2, fallback_basedir);
			if (fs_path) {
				filter_path = dir_v_filter_path_from_treestore_path(fb2, fs_path);
				if (filter_path) {
					expand_without_directory_refresh(fb2, filter_path);
					gtk_tree_path_free(filter_path);
				}
				gtk_tree_path_free(fs_path);
			}
			g_object_unref(fallback_basedir);
		}
		tmplist = g_list_next(tmplist);
	}
	g_list_free(tmplist);
	return retval;
}

static void
dir_v_row_expanded_lcb(GtkTreeView * tree, GtkTreeIter * sort_iter,
					   GtkTreePath * sort_path, Tbfwin *bfwin)
{
	UriRecord *record=NULL;
	Tfilebrowser2 * fb2;
	if (!bfwin || !bfwin->fb2)
		return;
	fb2 = bfwin->fb2;
	DEBUG_MSG("dir_v_row_expanded_lcb, called for fb2=%p with sort_path=\n", fb2);
	/* refresh the directory that is being expanded */
	gtk_tree_model_get(fb2->dir_filter, sort_iter, filetreemodel_COL_RECORD, &record, -1);
	if (record) {
		DEBUG_MSG("dir_v_row_expanded_lcb, calling fb2_refresh_dir()\n");
		filetreemodel_refresh_uri_async(FB2CONFIG(main_v->fb2config)->ftm, record->uri);
	}
}

static gboolean
dir_v_button_press_lcb(GtkWidget * widget, GdkEventButton * event, Tfilebrowser2 * fb2)
{
	DEBUG_MSG("dir_v_button_press_lcb, called for fb2=%p and event->button=%d\n", fb2, event->button);
	if (event->button == 3) {
		GtkTreePath *path;

		gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(fb2->dir_v), event->x, event->y, &path, NULL, NULL, NULL);
		if (path) {
			gboolean is_dir = fb2_isdir_from_dir_sort_path(fb2, path);
			DEBUG_MSG("context menu: %s selected\n",is_dir?"dir":"file");
			popup_menu_create(fb2, is_dir, !is_dir, event);
			gtk_tree_path_free(path);
		} else {
			DEBUG_MSG("dir_v_button_press_lcb, no path for position\n");
			DEBUG_MSG("context menu: nothing selected\n");
			popup_menu_create(fb2, FALSE, FALSE, event);
		}
	}/* else if (!(fb2->filebrowser_viewmode == viewmode_dual) && event->button == 1
			   && event->type == GDK_2BUTTON_PRESS) {
		GtkTreePath *path;
		gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(fb2->dir_v), event->x, event->y, &path, NULL, NULL, NULL);
		if (path && !fb2_isdir_from_dir_sort_path(fb2, path)) {
			GFile *uri;
			gchar *mime = NULL;
			uri = fb2_uri_from_dir_sort_path(fb2, path, &mime);
			if (uri) {
#ifdef DEBUG
				gchar *basename = g_file_get_basename(uri);
				DEBUG_MSG("file_v_button_press_lcb, doucleclick on %s\n", basename);
				g_free(basename);
#endif
				g_print("calling file_handle for doubleclick on dir_v\n");
				file_handle(uri, fb2->bfwin, mime, FALSE);
			}
			g_free(mime);
		}
	}*/
	return FALSE;				/* pass the event on */
}

static gboolean
file_v_button_press_lcb(GtkWidget * widget, GdkEventButton * event, Tfilebrowser2 * fb2)
{
	DEBUG_MSG("file_v_button_press_lcb, called for fb2=%p and event->button=%d\n", fb2, event->button);
	if (event->button == 3) {
		GtkTreePath *path=NULL;
		if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(fb2->file_v), event->x, event->y, &path, NULL,
									  NULL, NULL) && path) {
			DEBUG_MSG("context menu: file selected\n");
			popup_menu_create(fb2, FALSE, TRUE, event);
			gtk_tree_path_free(path);
		} else {
			DEBUG_MSG("context menu: nothing selected\n");
			DEBUG_MSG("no path for position\n");
			popup_menu_create(fb2, FALSE, FALSE, event);
		}
	} /*else if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
		GtkTreePath *sort_path=NULL;
		if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(fb2->file_v), event->x, event->y, &sort_path,
									  NULL, NULL, NULL) && sort_path) {
			GFile *uri;
			gchar *mime = NULL;
			uri = fb2_uri_from_file_sort_path(fb2, sort_path, &mime);
			if (uri) {
				g_print("call file_handle for doubleclick on file_v\n");
				file_handle(uri, fb2->bfwin, mime, FALSE);
			}
#ifdef DEBUG
			else {
				DEBUG_MSG("file_v_button_press_lcb, WARNING, NO URI FOUND, cannot activate NULL\n");
			}
#endif
			gtk_tree_path_free(sort_path);
		}
	}*/
	return FALSE;				/* pass the event on */
}

static void
add_parent_uri_to_recent_dirs(Tfilebrowser2 * fb2, GFile *uri)
{
	GFile *dir_uri = g_file_get_parent(uri);
	add_uri_to_recent_dirs(fb2, dir_uri);
	g_object_unref(dir_uri);
}

static void
dir_v_row_activated_lcb(GtkTreeView * tree, GtkTreePath * path,
						GtkTreeViewColumn * column, Tfilebrowser2 * fb2)
{
	GtkTreeIter iter;
	UriRecord *record=NULL;

	if (gtk_tree_model_get_iter(fb2->dir_filter, &iter, path)) {
		gtk_tree_model_get(fb2->dir_filter, &iter, filetreemodel_COL_RECORD, &record, -1);
	}

	if (!record)
		return;

	if (!record->isdir) {
		DEBUG_MSG("calling file_handle for row_activated\n");
		file_handle(record->uri, fb2->bfwin, record->fast_content_type, FALSE);
		add_parent_uri_to_recent_dirs(fb2, record->uri);
	} else { /* a directory */
		add_uri_to_recent_dirs(fb2, record->uri);
		if (fb2->filebrowser_viewmode == viewmode_flat) {
			set_basedir_backend(fb2, record->uri);
			filetreemodel_refresh_uri_async(FB2CONFIG(main_v->fb2config)->ftm, record->uri);
			fb2_set_dirmenu(fb2, record->uri, FALSE);
		} else {
			if (gtk_tree_view_row_expanded(tree, path)) {
				gtk_tree_view_collapse_row(tree, path);
			} else {
				expand_without_directory_refresh(fb2, path);
				filetreemodel_refresh_uri_async(FB2CONFIG(main_v->fb2config)->ftm, record->uri);
			}
		}
	}
}

static void
file_v_row_activated_lcb(GtkTreeView * tree, GtkTreePath * path,
						GtkTreeViewColumn * column, Tfilebrowser2 * fb2)
{
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter(fb2->file_filter, &iter, path)) {
		UriRecord *record;
		gtk_tree_model_get(fb2->file_filter, &iter, filetreemodel_COL_RECORD, &record, -1);
		DEBUG_MSG("calling file_handle for file_v row activat\n");
		file_handle(record->uri, fb2->bfwin, record->fast_content_type, FALSE);
		add_parent_uri_to_recent_dirs(fb2, record->uri);
	}
}

static void
dir_v_selection_changed_lcb(GtkTreeSelection * treeselection, Tbfwin *bfwin)
{
	GtkTreeModel *sort_model = NULL;
	GtkTreeIter sort_iter;
	Tfilebrowser2 *fb2;
	if (!bfwin || !bfwin->fb2)
		return;
	fb2 = bfwin->fb2;
	/* Get the current selected row and the model. */
	DEBUG_MSG("dir_v_selection_changed_lcb, treeselection=%p, fb2=%p\n", treeselection, fb2);
	if (treeselection && gtk_tree_selection_get_selected(treeselection, &sort_model, &sort_iter)) {
		UriRecord *record=NULL;
		gtk_tree_model_get(sort_model, &sort_iter, filetreemodel_COL_RECORD, &record, -1);
		if (record && record->isdir) {
			fb2_set_dirmenu(fb2, record->uri, FALSE);
			if (fb2->filebrowser_viewmode == viewmode_dual)
				set_file_v_root(fb2, record->uri);
			register_delayed_refresh(record->uri);
		}
	} else {
		DEBUG_MSG("dir_v_selection_changed_lcb, could not get any selection, returning..\n");
	}
}

static void
dirmenu_changed_lcb(GtkComboBox * widget, gpointer data)
{
	Tbfwin *bfwin = data;
	Tfilebrowser2 *fb2;
	GtkTreeIter iter;

	if (!bfwin || !bfwin->fb2)
		return;
	fb2 = bfwin->fb2;
	DEBUG_MSG("dirmenu_changed_lcb, started for fb2 %p\n", fb2);
	if (gtk_combo_box_get_active_iter(widget, &iter)) {
		GFile *uri;
		DEBUG_MSG("dirmenu_changed_lcb. we have an active iter\n");
		gtk_tree_model_get(GTK_TREE_MODEL(fb2->dirmenu_m), &iter, DIR_URI_COLUMN, &uri, -1);
		if (!uri) {
			DEBUG_MSG("dirmenu_changed_lcb. WEIRD, we do not have uri for this dirmenu entry\n");
			return;
		}
	/* TODO check the uri validity here before trying to display it. If  uri is not valid, display error message and remove it from recent_dirs
	*  Invalid uri will break many things downstream
	*/
		if (g_file_query_exists(uri,NULL)) {
			g_object_ref(uri);
			switch_to_directory(fb2, uri);
			g_object_unref(uri);
		} else {
		remove_uri_from_recent_dirs(fb2, uri);
		fb2_set_dirmenu(fb2, fb2->currentdir, TRUE);
		g_object_unref(uri);
		}
	}
}

enum {
	TARGET_URI_LIST,
	TARGET_STRING
};

static void
fb2_file_v_drag_data_received(GtkWidget * widget, GdkDragContext * context, gint x,
							  gint y, GtkSelectionData * data, guint info, guint time, Tfilebrowser2 * fb2)
{

	gchar *stringdata;
	const guchar *seldata;
	gint format;
	gint length;
	GFile *destdir = fb2->currentdir;

	g_object_ref(destdir);

	g_signal_stop_emission_by_name(widget, "drag_data_received");

	format = gtk_selection_data_get_format(data);
	length = gtk_selection_data_get_length(data);
	seldata = gtk_selection_data_get_data(data);

	if ((length == 0) || (format != 8)
		|| ((info != TARGET_STRING) && (info != TARGET_URI_LIST))) {
		gtk_drag_finish(context, FALSE, TRUE, time);
		return;
	}
	stringdata = g_strndup((gchar *) seldata, length);
	DEBUG_MSG("fb2_file_v_drag_data_received, stringdata='%s', len=%d\n", stringdata, length);
	if (destdir) {
		if (strchr(stringdata, '\n') == NULL) {	/* no newlines, probably a single file */
			GSList *list = NULL;
			GFile *uri;
			uri = g_file_new_for_commandline_arg(stringdata);
			list = g_slist_append(list, uri);
			copy_uris_async(fb2->bfwin, destdir, list);
			g_slist_free(list);
			g_object_unref(uri);
		} else {				/* there are newlines, probably this is a list of uri's */
			copy_files_async(fb2->bfwin, destdir, stringdata);
		}
		g_object_unref(destdir);
		gtk_drag_finish(context, TRUE, TRUE, time);
	} else {
		gtk_drag_finish(context, FALSE, TRUE, time);
	}
	g_free(stringdata);
}

static void
fb2_dir_v_drag_data_received(GtkWidget * widget, GdkDragContext * context, gint x,
							 gint y, GtkSelectionData * data, guint info, guint time, Tfilebrowser2 * fb2)
{
	gchar *stringdata;
	const guchar *seldata;
	gint format;
	gint length;
	GFile *destdir = NULL;
	GtkTreePath *path;
	/* if we don't do this, we get this text: Gtk-WARNING **: You must override the default
	   'drag_data_received' handler on GtkTreeView when using models that don't support the
	   GtkTreeDragDest interface and enabling drag-and-drop. The simplest way to do this is to
	   connect to 'drag_data_received' and call g_signal_stop_emission_by_name() in your signal
	   handler to prevent the default handler from running. Look at the source code for the
	   default handler in gtktreeview.c to get an idea what your handler should do.  */
	g_signal_stop_emission_by_name(widget, "drag_data_received");

	format = gtk_selection_data_get_format(data);
	length = gtk_selection_data_get_length(data);
	seldata = gtk_selection_data_get_data(data);

	if ((length == 0) || (format != 8)
		|| ((info != TARGET_STRING) && (info != TARGET_URI_LIST))) {
		gtk_drag_finish(context, FALSE, TRUE, time);
		return;
	}
	stringdata = g_strndup((gchar *) seldata, length);
	DEBUG_MSG("fb2_dir_v_drag_data_received, stringdata='%s', len=%d\n", stringdata, length);
	/* first find the destination directory */
	gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(fb2->dir_v), x, y, &path, NULL, NULL, NULL);
	if (path) {
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter(fb2->dir_filter, &iter, path)) {
			UriRecord *record;
			gtk_tree_model_get(fb2->dir_filter, &iter, filetreemodel_COL_RECORD, &record, -1);
			if (record && strncmp(record->fast_content_type, "x-directory", 11) == 0) {
				destdir = record->uri;
				g_object_ref(destdir);
			} else {
				destdir = g_file_get_parent(record->uri);
			}
		}
	}
	if (destdir) {
		if (strchr(stringdata, '\n') == NULL) {	/* no newlines, probably a single file */
			GSList *list = NULL;
			GFile *uri;
			uri = g_file_new_for_commandline_arg(stringdata);
			list = g_slist_append(list, uri);
			copy_uris_async(fb2->bfwin, destdir, list);
			g_slist_free(list);
			g_object_unref(uri);
		} else {				/* there are newlines, probably this is a list of uri's */
			copy_files_async(fb2->bfwin, destdir, stringdata);
		}
		g_object_unref(destdir);
		gtk_drag_finish(context, TRUE, TRUE, time);
	} else {
		gtk_drag_finish(context, FALSE, TRUE, time);
	}
	g_free(stringdata);
}

static gboolean
file_search_func(GtkTreeModel *model, gint column,const gchar *key, GtkTreeIter *iter,gpointer search_data)
{
	gint len;
	gchar *tmp=NULL;
	gboolean retval = TRUE;
	if (!key || key[0]=='\0')
		return TRUE;
	gtk_tree_model_get(model, iter, filetreemodel_COL_NAME, &tmp, -1);
	len = strlen(key);
	if (strncmp(key, tmp, len)==0){
		retval = FALSE;
	}
	g_free(tmp);
	return retval;
}

static void
fb2_two_pane_notify_position_lcb(GObject * object, GParamSpec * pspec, gpointer data)
{
	gint position;
	g_object_get(object, pspec->name, &position, NULL);
	if (main_v->props.restore_dimensions) {
		main_v->globses.two_pane_filebrowser_height = position;
	}
}

static void
dir_changed_lcb(FileTreemodel * ftm, GFile *dir_uri, gpointer user_data)
{
	Tfilebrowser2 *fb2 = user_data;
	if (fb2->bfwin->session->filebrowser_focus_follow && fb2->bfwin->current_document && fb2->bfwin->current_document->uri) {
		if (gfile_uri_is_parent(dir_uri, fb2->bfwin->current_document->uri, FALSE)) {
			change_focus_to_uri(fb2, fb2->bfwin->current_document->uri);
		}
	}
}

static gboolean
fb2_tooltip_lcb(GtkWidget * widget, gint x, gint y, gboolean keyboard_tip, GtkTooltip * tooltipwidget,
				gpointer user_data)
{
	GtkTreeView *tview = (GtkTreeView *)widget;
	Tfilebrowser2 *fb2 = user_data;
	GtkTreePath *path;
	gboolean retval = FALSE;
	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tview), x, y, &path, NULL, NULL, NULL)) {
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter(gtk_tree_view_get_model(tview), &iter, path)) {
			UriRecord *record = NULL;
			gtk_tree_model_get(gtk_tree_view_get_model(tview), &iter, filetreemodel_COL_RECORD, &record, -1);
			if (record) {
				gchar *curi = g_file_get_uri(record->uri);
				gchar *localname=NULL;
				gchar *text;
				if (g_file_is_native(record->uri)) {
					localname = g_file_get_path(record->uri);
				}
				if (fb2->bfwin->session->webroot && fb2->bfwin->session->documentroot && g_str_has_prefix(curi, fb2->bfwin->session->documentroot)) {
					text = g_strdup_printf(_("<b>%s</b>\nwhich is the equivalent of\n<b>%s%s</b>"), localname?localname:curi, fb2->bfwin->session->webroot, curi+strlen(fb2->bfwin->session->documentroot));
				} else {
					text = g_strdup_printf("<b>%s</b>",localname?localname:curi);
				}
				gtk_tooltip_set_markup(tooltipwidget, text);
				g_free(curi);
				g_free(text);
				g_free(localname);
				retval = TRUE;
			}
		}
		gtk_tree_path_free(path);
	}
	return retval;
}

static void
fb2_set_viewmode_widgets(Tfilebrowser2 * fb2, gint viewmode)
{
	GtkTreeViewColumn *column;
	GtkTreeSelection *dirselection;
	GtkWidget *scrolwin;
	GtkCellRenderer *renderer;
	GtkTreePath *basepath = NULL;

	const GtkTargetEntry drag_dest_types[] = {
		{"text/uri-list", 0, TARGET_URI_LIST},
		{"STRING", 0, TARGET_STRING},
	};


	if (fb2->filebrowser_viewmode == viewmode) {
		/* the current viewmode is the same as the requested viewmode
		   we can return if all the widgets exist */
		if (fb2->dir_filter != NULL) {
			DEBUG_MSG("fb2_set_viewmode_widgets, keep existing widgets and return\n");
			return;
		}
	} else {
		DEBUG_MSG("fb2_set_viewmode_widgets, destroying old widgets (if any)\n");
		if (fb2->vpaned) {
			DEBUG_MSG("remove fb2->vpaned, ");
			gtk_container_remove(GTK_CONTAINER(fb2->vbox), fb2->vpaned);
			DEBUG_MSG("fb2->vpaned, ");
			fb2->vpaned = NULL;
		} else if (fb2->dirscrolwin) {
			DEBUG_MSG("remove fb2->dirscrolwin, ");
			gtk_container_remove(GTK_CONTAINER(fb2->vbox), fb2->dirscrolwin);
			fb2->dirscrolwin = NULL;
		}
		fb2->dir_v = fb2->file_v = NULL;
		fb2->dir_filter = fb2->file_filter = NULL;
		DEBUG_MSG("fb2_set_viewmode_widgets, reset dir_filter and file_filter\n");
	}
	fb2->filebrowser_viewmode = viewmode;

	DEBUG_MSG("fb2_set_viewmode_widgets, building new GUI\n");
	if (fb2->basedir) {
		basepath = treepath_for_uri(fb2, fb2->basedir);
		if (basepath && fb2->filebrowser_viewmode != viewmode_flat) {
			gtk_tree_path_up(basepath);
		}
	}

	fb2->dir_filter =
		gtk_tree_model_filter_new(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->ftm), basepath);
	DEBUG_TREEMODELREFS("fb2_set_viewmode_widgets, created new tree model filter at %p for fb2 %p\n",fb2->dir_filter, fb2);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(fb2->dir_filter),
										   tree_model_filter_func, fb2, NULL);

	fb2->dir_v = gtk_tree_view_new_with_model(fb2->dir_filter);
	/* we remove our reference, so the only reference is kept by the treeview, if the treeview is destroyed, the models will be destroyed */
	g_object_unref(G_OBJECT(fb2->dir_filter));
	DEBUG_TREEMODELREFS("fb2_set_viewmode_widgets, unreffed tree model filter at %p for fb2 %p, ref is added to view %p\n",fb2->dir_filter, fb2, fb2->dir_v);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(fb2->dir_v), FALSE);
	gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW(fb2->dir_v),file_search_func,fb2,NULL);
	if (fb2->filebrowser_viewmode != viewmode_flat) {
		dirselection = gtk_tree_view_get_selection(GTK_TREE_VIEW(fb2->dir_v));
		DEBUG_MSG("fb2_set_viewmode_widgets, treeselection=%p, fb2=%p, dir_filter=%p\n",
				  dirselection, fb2, fb2->dir_filter);
		fb2->dirselection_changed_id = g_signal_connect(G_OBJECT(dirselection), "changed", G_CALLBACK(dir_v_selection_changed_lcb), fb2->bfwin);
	} else {
		fb2->dirselection_changed_id = 0;
	}

	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer,
										"icon-name", filetreemodel_COL_ICON_NAME,NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", filetreemodel_COL_NAME, "weight", filetreemodel_COL_WEIGHT, NULL);
	g_object_set(G_OBJECT(renderer), "editable", FALSE, "weight-set", TRUE, NULL);	/* Not editable. */
	gtk_tree_view_append_column(GTK_TREE_VIEW(fb2->dir_v), column);
	fb2->dirscrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(fb2->dirscrolwin), GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(fb2->dirscrolwin), fb2->dir_v);

	gtk_drag_dest_set(fb2->dir_v, (GTK_DEST_DEFAULT_ALL), drag_dest_types, 2,
					  (GDK_ACTION_DEFAULT | GDK_ACTION_COPY));
	g_signal_connect(G_OBJECT(fb2->dir_v), "drag_data_received",
					 G_CALLBACK(fb2_dir_v_drag_data_received), fb2);

	if (fb2->filebrowser_viewmode != viewmode_dual) {
		gtk_box_pack_start(GTK_BOX(fb2->vbox), fb2->dirscrolwin, TRUE, TRUE, 0);
	} else {
#if GTK_CHECK_VERSION(3,0,0)
		fb2->vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
#else
		fb2->vpaned = gtk_vpaned_new();
#endif
		gtk_paned_set_position(GTK_PANED(fb2->vpaned), main_v->globses.two_pane_filebrowser_height);
		g_signal_connect(G_OBJECT(fb2->vpaned), "notify::position",
						 G_CALLBACK(fb2_two_pane_notify_position_lcb), NULL);

		gtk_paned_add1(GTK_PANED(fb2->vpaned), fb2->dirscrolwin);

		fb2->file_filter =
			gtk_tree_model_filter_new(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->ftm), NULL);
		DEBUG_TREEMODELREFS("fb2_set_viewmode_widgets, created new tree model filter at %p for fb2 %p\n",fb2->file_filter, fb2);
		gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(fb2->file_filter),
											   file_list_filter_func, fb2, NULL);

		DEBUG_MSG("fb2_init, file_filter=%p\n", fb2->file_filter);
		fb2->file_v = gtk_tree_view_new_with_model(fb2->file_filter);
		/* we remove our reference, so the only reference is kept by the treeview, if the treeview is destroyed, the models will be destroyed */
		g_object_unref(G_OBJECT(fb2->file_filter));
		DEBUG_TREEMODELREFS("fb2_set_viewmode_widgets, unreffed tree model filter at %p for fb2 %p, ref should have been added by view %p\n",fb2->file_filter, fb2, fb2->file_v);

		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(fb2->file_v), FALSE);
		gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW(fb2->file_v),file_search_func,fb2,NULL);
		renderer = gtk_cell_renderer_pixbuf_new();
		column = gtk_tree_view_column_new();
		gtk_tree_view_column_pack_start(column, renderer, FALSE);
		gtk_tree_view_column_set_attributes(column, renderer,
											"icon-name", filetreemodel_COL_ICON_NAME,NULL);
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_set_attributes(column, renderer, "text", filetreemodel_COL_NAME, "weight", filetreemodel_COL_WEIGHT, NULL);
		g_object_set(G_OBJECT(renderer), "editable", FALSE, "weight-set", TRUE, NULL);	/* Not editable. */
		gtk_tree_view_append_column(GTK_TREE_VIEW(fb2->file_v), column);
		scrolwin = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC,
									   GTK_POLICY_AUTOMATIC);
		gtk_container_add(GTK_CONTAINER(scrolwin), fb2->file_v);
		gtk_paned_add2(GTK_PANED(fb2->vpaned), scrolwin);

		gtk_box_pack_start(GTK_BOX(fb2->vbox), fb2->vpaned, TRUE, TRUE, 0);
		g_signal_connect(G_OBJECT(fb2->file_v), "button_press_event",
						 G_CALLBACK(file_v_button_press_lcb), fb2);
		g_signal_connect(G_OBJECT(fb2->file_v), "row-activated", G_CALLBACK(file_v_row_activated_lcb), fb2);
		gtk_drag_dest_set(fb2->file_v, (GTK_DEST_DEFAULT_ALL), drag_dest_types, 2,
						  (GDK_ACTION_DEFAULT | GDK_ACTION_COPY));
		g_signal_connect(G_OBJECT(fb2->file_v), "drag_data_received",
						 G_CALLBACK(fb2_file_v_drag_data_received), fb2);
		g_object_set(fb2->file_v, "has-tooltip", TRUE, NULL);
		g_signal_connect(fb2->file_v, "query-tooltip", G_CALLBACK(fb2_tooltip_lcb), fb2);
	}

	g_signal_connect(G_OBJECT(fb2->dir_v), "row-activated", G_CALLBACK(dir_v_row_activated_lcb), fb2);
	g_signal_connect(G_OBJECT(fb2->dir_v), "button_press_event", G_CALLBACK(dir_v_button_press_lcb), fb2);
	fb2->expand_signal =
		g_signal_connect(G_OBJECT(fb2->dir_v), "row-expanded", G_CALLBACK(dir_v_row_expanded_lcb), fb2->bfwin);
	g_signal_connect(fb2->dir_v, "query-tooltip", G_CALLBACK(fb2_tooltip_lcb), fb2);
	g_object_set(fb2->dir_v, "has-tooltip", TRUE, NULL);
	/*gtk_container_resize_children(GTK_CONTAINER(fb2->vbox)); */
	gtk_widget_show_all(fb2->vbox);
	DEBUG_MSG("fb2_set_viewmode_widgets, new GUI finished\n");
}

void
fb2_update_settings_from_session(Tbfwin * bfwin, Tdocument *active_doc)
{
	if (!bfwin->fb2) {
		DEBUG_MSG("fb2_update_settings_from_session, no fb2, nothing to update... returning\n");
		return;
	}
	gboolean need_refilter = FALSE;
	Tfilebrowser2 *fb2 = bfwin->fb2;

	DEBUG_MSG("fb2_update_settings_from_session, started, bfwin=%p, fb2=%p, viewmode=%d, active_doc=%p\n",
			  bfwin, fb2, fb2->filebrowser_viewmode, active_doc);

	if (!bfwin->filebrowserGroup)
		popup_menu_action_group_init(bfwin);

	fb2_set_viewmode_widgets(fb2, bfwin->session->filebrowser_viewmode);

	if (bfwin->session->last_filefilter) {
		Tfilter *newfilter = find_filter_by_name(bfwin->session->last_filefilter);
		if (fb2->curfilter == NULL || newfilter == NULL
			|| !(newfilter == fb2->curfilter || strcmp(newfilter->name, fb2->curfilter->name) == 0)) {
			fb2->curfilter = newfilter;
			need_refilter = TRUE;
		}
	}
	if (fb2->filebrowser_show_hidden_files != bfwin->session->filebrowser_show_hidden_files) {
		fb2->filebrowser_show_hidden_files = bfwin->session->filebrowser_show_hidden_files;
		need_refilter = TRUE;
	}
	if (fb2->filebrowser_show_backup_files != bfwin->session->filebrowser_show_backup_files) {
		fb2->filebrowser_show_backup_files = bfwin->session->filebrowser_show_backup_files;
		need_refilter = TRUE;
	}
	if (bfwin->session->recent_dirs) {
		const gchar *tmp = (gchar *) ((GList *) g_list_first(bfwin->session->recent_dirs))->data;
		/* the set_basedir_backend function tests itself if  the basedir if changed, if not it does not refresh */
		DEBUG_MSG("fb2_update_settings_from_session, set basedir %s\n", tmp);
		if (tmp && tmp[0]) {
			GtkTreePath *fs_path, *filter_path;
			GFile *uri = g_file_new_for_uri(strcmp(tmp,"file:///")==0?tmp:strip_trailing_slash((gchar *) tmp));
			DEBUG_MSG("fb2_update_settings_from_session, set basedir %p\n",uri);
			fb2_set_basedir(fb2, uri);
			fb2_set_dirmenu(fb2, uri, FALSE);
			if (fb2->filebrowser_viewmode == viewmode_dual) {
				set_file_v_root(fb2, uri);
			}
			filetreemodel_refresh_uri_async(FB2CONFIG(main_v->fb2config)->ftm, uri);
			/* on an empty window / empty project, expand the basedir, because there will be no call for follow document */
			if (!active_doc || !bfwin->session->filebrowser_focus_follow) {
				fs_path = treepath_for_uri(fb2, uri);
				if (fs_path) {
					filter_path = dir_v_filter_path_from_treestore_path(fb2, fs_path);
					if (filter_path) {
						expand_without_directory_refresh(fb2, filter_path);
						gtk_tree_path_free(filter_path);
					}
					gtk_tree_path_free(fs_path);
				}

			}
			g_object_unref(uri);
		}
	} else {
		DEBUG_MSG("fb2_update_settings_from_session, set basedir NULL\n");
		set_basedir_backend(fb2, NULL);
	}
	if (bfwin->session->webroot && bfwin->session->documentroot) {
		GtkTreeIter iter;
		GFile *uri = g_file_new_for_uri(bfwin->session->documentroot);
		if (!filetree_get_iter_for_uri(FB2CONFIG(main_v->fb2config)->ftm, uri, &iter)) {
			filetreemodel_build_dir(FB2CONFIG(main_v->fb2config)->ftm, uri, NULL);
		}
		fb2config_set_documentroot_icon(uri);
		g_object_unref(uri);
	}
	/* TODO: set_basedir_backend already calls refilter in most cases (not if the
	   requested basedir was already the active basedir), so
	   we can optimise this and call refilter only when really needed. */
	if (need_refilter) {
		DEBUG_MSG("fb2_update_settings_from_session, need_refilter=%d, dir_filter=%p, file_filter=%p\n",need_refilter,fb2->file_filter,fb2->dir_filter);
		if (fb2->dir_filter)
			gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(fb2->dir_filter));
		if (fb2->file_filter && fb2->filebrowser_viewmode == viewmode_dual)
			gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(fb2->file_filter));
	}
	DEBUG_MSG("fb2_update_settings_from_session, done.\n");
}

GtkWidget *
fb2_init(Tbfwin * bfwin)
{
	Tfilebrowser2 *fb2;
	GtkCellRenderer *renderer;
#ifdef DEVELOPMENT
	if (!bfwin->session) {
		g_critical("bfwin->session should not be NULL\n");
		g_return_val_if_reached(NULL);
	}
#endif
	fb2 = g_new0(Tfilebrowser2, 1);
	fb2->filebrowser_viewmode = bfwin->session->filebrowser_viewmode;

	bfwin->fb2 = fb2;
	fb2->bfwin = bfwin;
	DEBUG_MSG("fb2_init, started for bfwin=%p, fb2=%p, fb2->filebrowser_viewmode=%d\n", bfwin, fb2,
			  fb2->filebrowser_viewmode);

	fb2->vbox = gtk_vbox_new(FALSE, 0);
	g_signal_connect(G_OBJECT(fb2->vbox), "destroy", G_CALLBACK(gtk_widget_destroyed), &fb2->vbox);
	fb2->dirmenu_m = GTK_TREE_MODEL(gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING));
	fb2->dirmenu_v = gtk_combo_box_new_with_model(fb2->dirmenu_m);
	/*gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(fb2->dirmenu_v),3); */
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(fb2->dirmenu_v), renderer, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(fb2->dirmenu_v), renderer, "icon-name", DIR_ICON_COLUMN,
								   /*"pixbuf_expander_closed", DIR_ICON_COLUMN,
								      "pixbuf_expander_open", DIR_ICON_COLUMN, */ NULL);

	renderer = gtk_cell_renderer_text_new();
#if GTK_CHECK_VERSION(3,0,0)
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_START, NULL);
#endif
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(fb2->dirmenu_v), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(fb2->dirmenu_v), renderer, "text", DIR_NAME_COLUMN, NULL);
	gtk_box_pack_start(GTK_BOX(fb2->vbox), fb2->dirmenu_v, FALSE, FALSE, 0);
	fb2->dirmenu_changed_signal =
		g_signal_connect(fb2->dirmenu_v, "changed", G_CALLBACK(dirmenu_changed_lcb), bfwin);

	/* already called from bfwin.c fb2_update_settings_from_session(bfwin, NULL);*/

	gtk_widget_show_all(fb2->vbox);
	filetreemodel_dirchange_register(FB2CONFIG(main_v->fb2config)->ftm, dir_changed_lcb, fb2);
	return fb2->vbox;
}

void
fb2_cleanup(Tbfwin * bfwin)
{
	if (bfwin->fb2) {
		Tfilebrowser2 *fb2 = FILEBROWSER2(bfwin->fb2);
		GList *actions, *list;
		DEBUG_MSG("fb2_cleanup, fb2=%p, fb2->bfwin=%p, fb2->vbox=%p\n",fb2,fb2->bfwin,fb2->vbox);

		filetreemodel_dirchange_unregister_by_data(FB2CONFIG(main_v->fb2config)->ftm, fb2);
		if (fb2->vbox) {
			DEBUG_MSG("fb2_cleanup, we still have a vbox, destroy vbox\n");
			gtk_widget_destroy(fb2->vbox);
		}

		DEBUG_MSG("fb2_cleanup, remove ui_manager actions\n");
		if (bfwin->fb2_filters_group) {
			gtk_ui_manager_remove_ui(bfwin->uimanager, bfwin->fb2_filters_merge_id);
			actions = gtk_action_group_list_actions(bfwin->fb2_filters_group);
			for (list = actions; list; list = list->next) {
				g_signal_handlers_disconnect_by_func(GTK_ACTION(list->data),
						G_CALLBACK(popup_menu_filter_activate), bfwin);
				gtk_action_group_remove_action(bfwin->fb2_filters_group, GTK_ACTION(list->data));
			}
			g_list_free(actions);
			bfwin->fb2_filters_group = NULL;
		}
		gtk_ui_manager_remove_ui(bfwin->uimanager, bfwin->filebrowser_merge_id);
		actions = gtk_action_group_list_actions(bfwin->filebrowserGroup);
		for (list = actions; list; list = list->next) {
			gtk_action_group_remove_action(bfwin->filebrowserGroup, GTK_ACTION(list->data));
		}
		g_list_free(actions);
		gtk_ui_manager_remove_action_group(bfwin->uimanager,bfwin->filebrowserGroup);
		bfwin->filebrowserGroup = NULL;
/*		g_print("fb2_cleanup, disconnect dirmenu_changed_signal\n");
		g_signal_handler_disconnect(fb2->dirmenu_v, fb2->dirmenu_changed_signal);
*/		dirmenu_idle_cleanup_lcb(fb2->dirmenu_m);
		if (fb2->basedir)
			g_object_unref(fb2->basedir);
		if (fb2->currentdir)
			g_object_unref(fb2->currentdir);
		DEBUG_MSG("fb2_cleanup, free %p\n",fb2);
		g_free(fb2);
		bfwin->fb2 = NULL;
	}
}

/*****************************************************************************************************************************/
/*  END OF MAIN GUI FUNCTIONS  */
/*****************************************************************************************************************************/




/**************/





/**
 * fb2_focus_dir:
 *
 * builds the directory tree, refreshed the directory, and (if not noselect) expands and selects the
 * result. during the expand it will block the expand signal handler, so the expand callback
 * will not be called
 *
 */
/*
static void
fb2_focus_dir(Tfilebrowser2 * fb2, GFile * uri, gboolean noselect)
{
	GtkTreeIter *dir;
	DEBUG_MSG("fb2_focus_dir(fb2=%p, uri=%p, noselect=%d)\n", fb2, uri, noselect);
	if (!uri) {
		DEBUG_MSG("fb2_focus_dir, WARNING, CANNOT FOCUS WITHOUT URI\n");
		return;
	}
	g_object_ref(uri);
	if (fb2->filebrowser_viewmode == viewmode_flat) {
		set_basedir_backend(fb2, uri);
	} else {
		if (fb2->basedir) {
			if (!gfile_uri_is_parent(fb2->basedir, uri, TRUE)
				&& !g_file_equal(fb2->basedir, uri)) {
				set_basedir_backend(fb2, NULL);
			}
		}
	}
	dir = g_hash_table_lookup(FB2CONFIG(main_v->fb2config)->filesystem_itable, uri);
	DEBUG_MSG("fb2_focus_dir, fb2=%p, dir=%p\n", fb2, dir);
	if (!dir) {
		dir = fb2_build_dir(uri);
		DEBUG_MSG("fb2_focus_dir, after building, fb2=%p, dir=%p\n", fb2, dir);
	}
	if (dir) {
		GtkTreePath *fs_path;
		DEBUG_DIRITER(dir);

		/ * set this directory as the top tree for the file widget * /
		fs_path =
			gtk_tree_model_get_path(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->ftm), dir);
		if (fs_path) {
			refilter_filelist(fb2, fs_path);
			if (!noselect && fb2->filebrowser_viewmode != viewmode_flat) {
				GtkTreePath *sort_path = dir_filter_path_from_treestore_path(fb2, fs_path);
				if (sort_path) {
					if (fb2->filebrowser_viewmode == viewmode_tree && gtk_tree_view_row_expanded(GTK_TREE_VIEW(fb2->dir_v), sort_path) && !need_to_scroll_to_dir(fb2, uri)) {
						/ * do nothing * /
					} else {
						g_signal_handler_block(fb2->dir_v, fb2->expand_signal);
						gtk_tree_view_expand_to_path(GTK_TREE_VIEW(fb2->dir_v), sort_path);
						g_signal_handler_unblock(fb2->dir_v, fb2->expand_signal);
						DEBUG_MSG("fb2_focus_dir, selecting path\n");
						gtk_tree_selection_select_path(gtk_tree_view_get_selection
													   (GTK_TREE_VIEW(fb2->dir_v)), sort_path);
						gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(fb2->dir_v), sort_path, 0, TRUE, 0.5, 0.5);
					}
					gtk_tree_path_free(sort_path);
				} else {
					DEBUG_MSG("fb2_focus_dir, no sort_path\n");
				}
			} else {
				/ * 'uri' is not persistent, 'dir' is peristent, so only pass 'dir'
				   the "expand" signal (dual or tree view) will also refresh this directory, so
				   we call this only on 'noselect' or in the flat view
				 * /
				DEBUG_MSG("fb2_focus_dir, noselect, so directly call refresh\n");
				fb2_refresh_dir(NULL, dir);
				if (fb2->filebrowser_viewmode == viewmode_flat) {
					fb2_set_dirmenu(fb2, uri);
				}
			}
			gtk_tree_path_free(fs_path);
		} else {
			DEBUG_MSG("NO TREEPATH FOR THE DIR ITER WE TRY TO FOCUS ?!?!?!\n");
		}
	} else {
		DEBUG_MSG("NO dir RETURNED by fb2_build_dir or the hash table\n");
	}
	g_object_unref(uri);
}
*/

/**
 * fb2_focus_document:
 *
 * external function, will make sure the filebrowser shows the current dcoument
 * directory
 */
void
fb2_focus_document(Tbfwin * bfwin, Tdocument * doc)
{
	DEBUG_MSG("fb2_focus_document,doc %s\n", gtk_label_get_text(GTK_LABEL(doc->tab_menu)));
	if (bfwin->fb2 && doc && doc->uri) {
		fb2_follow_uri(bfwin, doc->uri);

		/*GFile *dir_uri;*/
		/* first we make sure we have the correct directory open, then
		   we could select the document, but if the directory *was* open already, this
		   could disturb the user... hmmm... */
		/*dir_uri = g_file_get_parent(doc->uri);
		fb2_focus_dir(FILEBROWSER2(bfwin->fb2), dir_uri, FALSE);
		g_object_unref(dir_uri);*/
	}
}
