/* Bluefish HTML Editor
 * filebrowser2.c the filebrowser v2
 *
 * Copyright (C) 2002-2005 Olivier Sessink
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
#define DEBUG

/* ******* FILEBROWSER DESIGN ********
there is only one treestore left for all bluefish windows. This treestore has all files 
and all directories used in all bluefish windows. This treestore has a column for the pixmap
and the name (both shown to the user), but also the GnomeVFSURI for the full path (excluding the 
trailing slash for directories, see #1), and the type. This treestore is in main_v->fb2config. Each file 
or directory added to the treestore is also added to a hashtable in main_v->fb2config

the GUI uses two filtermodels on top of the treestore, and then two sortmodels, so they can 
show a specific part of the treestore (for example with a basedir other then the root, or with 
some filter applied).

#1) the hash for file:///home/ is different from file:///home so we should choose which hash is
to be stored in the hashtable. The method gnome_vfs_uri_get_parent() returns directories without
the trailing slash. So it is convenient to use directories without trailing slashes all the way.
*/

#include <gtk/gtk.h>
#include <string.h>
#include "bluefish.h"
#include "file.h"
#include "bf_lib.h"
#include "stringlist.h" /* count_array() */
#include "project.h"
#include "document.h"
#include "file_dialogs.h"
#include "image.h"
#include "menu.h" /* menu_translate() */
#include "gtk_easy.h" /* destroy_disposable_menu_cb() */

typedef struct {
	gchar *name;
	gboolean mode; /* 0= hide matching files, 1=show matching files */
	GList *filetypes; /* if NULL all files are OK */
} Tfilter;

typedef struct {
	GList *filters; /* the compiled filters */
	GList *filetypes_with_icon; /* name says it all! */
	
	GtkTreeStore *filesystem_tstore; /* the directory tree */	
	GHashTable *filesystem_itable; /* iter to known files and directories */
	GdkPixbuf *unknown_icon;
	GdkPixbuf *dir_icon;
	GList *uri_in_refresh; /* all uris currently in refresh are stored here, because 
									two refreshes on the same uri should not happen */
} Tfilebrowser2config;
#define FILEBROWSER2CONFIG(var) ((Tfilebrowser2config *)(var))

enum {
	PIXMAP_COLUMN,
	FILENAME_COLUMN,
	URI_COLUMN,
	REFRESH_COLUMN,
	TYPE_COLUMN,
	N_COLUMNS
};

enum {
	DIR_NAME_COLUMN,
	DIR_IN_HISTORY_COLUMN,
	DIR_URI_COLUMN
};
#define TYPE_HIDDEN -1
#define TYPE_HIDDEN_DIR -2
#define TYPE_DIR -3
#define TYPE_BACKUP -4
#define TYPE_FILE -5

typedef struct {
	GtkWidget *dirmenu_v;
	gulong dirmenu_changed_signal;
	GtkTreeModel *dirmenu_m;
	GnomeVFSURI *currentdir;
	
	GtkWidget *dir_v; /* treeview widget */
	GtkWidget *file_v; /* listview widget */
	
	gulong expand_signal;
	
	GnomeVFSURI *basedir;
	Tfilter *curfilter;

	GtkTreeModel *dir_tfilter;
	GtkTreeModel *dir_tsort;

	GtkTreeModel *file_lfilter;
	GtkTreeModel *file_lsort;

	gboolean last_popup_on_dir;

	Tbfwin *bfwin;
} Tfilebrowser2;

#define FILEBROWSER2(var) ((Tfilebrowser2 *)(var))
static void refilter_filelist(Tfilebrowser2 *fb2, GtkTreePath *newroot);
static void fb2_fill_dir_async(GtkTreeIter *parent, GnomeVFSURI *uri);
static GnomeVFSURI *fb2_uri_from_fspath(Tfilebrowser2 *fb2, GtkTreePath *fs_path);
/**************/
static void DEBUG_DIRITER(Tfilebrowser2 *fb2, GtkTreeIter *diriter) {
	gchar *name;
	gtk_tree_model_get(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore), diriter, FILENAME_COLUMN, &name, -1);
	DEBUG_MSG("DEBUG_DIRITER, iter(%p) has filename %s\n",diriter,name);
}
static void DEBUG_URI(const GnomeVFSURI *uri, gboolean newline) {
	guint hash;
	gchar *name = gnome_vfs_uri_to_string(uri, GNOME_VFS_URI_HIDE_PASSWORD);
	hash = gnome_vfs_uri_hash(uri);
	DEBUG_MSG("%u->%s", hash, name);
	if (newline) {
		DEBUG_MSG("\n");
	}
	g_free(name);
}
static void DEBUG_TPATH(GtkTreeModel *model, GtkTreePath *path, gboolean newline) {
	gchar *tname, *filename;
	GtkTreeIter iter;
	tname = gtk_tree_path_to_string(path);
	DEBUG_MSG(tname);
	if (gtk_tree_model_get_iter(model,&iter,path)) {
		gtk_tree_model_get(model, &iter, FILENAME_COLUMN, &filename, -1);
	} else {
		filename = "NO VALID ITER";
	}
	DEBUG_MSG(" -> %s", filename);
	if (newline) {
		DEBUG_MSG("\n");
	}
	g_free(tname);
}
static void DEBUG_CHILD_ITERS(Tfilebrowser2* fb2, GtkTreeIter *sorted_iter) {
	GtkTreeIter filter_iter, fs_iter;
	gchar *name;
	
	DEBUG_MSG("DEBUG_CHILD_ITERS, dir_v displays sort_model %p\n", gtk_tree_view_get_model(GTK_TREE_VIEW(fb2->dir_v)));
	
	gtk_tree_model_get(fb2->dir_tsort, sorted_iter, FILENAME_COLUMN, &name, -1);
	DEBUG_MSG("DEBUG_CHILD_ITERS, dir_tsort=%p, sorted name=%s\n",fb2->dir_tsort , name);
	gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(fb2->dir_tsort),&filter_iter,sorted_iter);
	DEBUG_MSG("DEBUG_CHILD_ITERS, dir_tsort %p is sorting dir_tfilter %p\n",fb2->dir_tsort ,gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(fb2->dir_tsort)));

	gtk_tree_model_get(fb2->dir_tfilter, &filter_iter, FILENAME_COLUMN, &name, -1);
	DEBUG_MSG("DEBUG_CHILD_ITERS, dir_tfilter=%p, filter name=%s\n", fb2->dir_tfilter, name);
	gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(fb2->dir_tfilter),&fs_iter,&filter_iter);
	DEBUG_MSG("DEBUG_CHILD_ITERS, dir_tfilter %p is filtering filesystem %p, == %p\n",fb2->dir_tfilter 
				,gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(fb2->dir_tfilter))
				, FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore
				);
	
	gtk_tree_model_get(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore), &fs_iter, FILENAME_COLUMN, &name, -1);
	DEBUG_MSG("DEBUG_CHILD_ITERS, fs name=%s\n", name);
}
/**************/

/**
 * fb2_add_filesystem_entry:
 *
 * returns the GtkTreeIter* that is stored in the hashtable. This can be a new iter pointer
 * if there was no iter in the hashtable yet, else it is the existing iter
 *
 */
static GtkTreeIter *fb2_add_filesystem_entry(GtkTreeIter *parent, GnomeVFSURI *child_uri, gint type, gboolean load_subdirs) {
	GtkTreeIter *newiter;
	guint *hashkey;
	
	hashkey = g_new(guint,1);
	*hashkey = gnome_vfs_uri_hash(child_uri);
	newiter = g_hash_table_lookup(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_itable, hashkey);
	if (newiter != NULL) {
		gint refresh;
		DEBUG_MSG("fb2_add_filesystem_entry, set refresh to 0 for iter %p, uri exists ",newiter);
		DEBUG_URI(child_uri, TRUE);
		/* the child exists already, update the REFRESH column */
		gtk_tree_model_get(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore), newiter, REFRESH_COLUMN, &refresh, -1);
		if (refresh != 0) {
			gtk_tree_store_set(GTK_TREE_STORE(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),newiter,REFRESH_COLUMN, 0,-1);
		}
		g_free(hashkey);
	} else {
		gchar *tmp, *tmp2, *display_name;
		GnomeVFSURI *uri_dup;
		gpointer pixmap;
		newiter = g_new(GtkTreeIter,1);
		uri_dup = gnome_vfs_uri_dup(child_uri);
		tmp = uri_to_document_filename(child_uri);
		tmp2 = strrchr(tmp, '/');
		DEBUG_MSG("fb2_add_filesystem_entry, tmp2=%s for tmp=%s\n",tmp2,tmp);
		display_name = tmp2 && strlen(tmp2)>2 ? g_strdup(tmp2+1) : gnome_vfs_uri_to_string(child_uri,GNOME_VFS_URI_HIDE_PASSWORD);
		pixmap = FILEBROWSER2CONFIG(main_v->fb2config)->dir_icon;
		if (type != TYPE_DIR) {
			Tfiletype *ft = get_filetype_by_filename_and_content(display_name, NULL);
			if (ft && ft->icon) pixmap = ft->icon;
			else pixmap = FILEBROWSER2CONFIG(main_v->fb2config)->unknown_icon;
		}
		g_free(tmp);
		gtk_tree_store_append(GTK_TREE_STORE(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),newiter,parent);
		DEBUG_MSG("fb2_add_filesystem_entry, will add ");
		DEBUG_URI(uri_dup, TRUE);
		gtk_tree_store_set(GTK_TREE_STORE(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),newiter,
				PIXMAP_COLUMN, pixmap,
				FILENAME_COLUMN, display_name,
				URI_COLUMN, uri_dup,
				REFRESH_COLUMN, 0,
				TYPE_COLUMN, type,
				-1);
		DEBUG_MSG("fb2_add_filesystem_entry adding iter %p to hashkey %u\n",newiter,*hashkey);
		g_hash_table_insert(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_itable,hashkey,newiter);
		if (load_subdirs && type == TYPE_DIR) {
			GnomeVFSURI *dummy_uri;
			/* add a dummy item so the expander will be shown */
			dummy_uri = gnome_vfs_uri_append_string(child_uri,"dummy");
			fb2_add_filesystem_entry(newiter, dummy_uri, TYPE_DIR, FALSE);
			gnome_vfs_uri_unref(dummy_uri);
		}
	}
	return newiter;
}

typedef struct {
	GtkTreeIter *parent;
	GnomeVFSURI *p_uri;
} Tdirectoryloaddata;
/**
 * fb2_treestore_delete_children_refresh1
 *
 * deletes all children of 'iter' that have value 1 in the REFRESH_COLUMN
 */
static void fb2_treestore_delete_children_refresh1(GtkTreeStore *tstore, GtkTreeIter *iter) {
	GtkTreeIter child;
	if (gtk_tree_model_iter_children(GTK_TREE_MODEL(tstore),&child,iter)) {
		gboolean cont=TRUE;
		while (cont) {
			gboolean refresh;
			GtkTreeIter this = child;
			gtk_tree_model_get(GTK_TREE_MODEL(tstore), &child, REFRESH_COLUMN, &refresh, -1);
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(tstore), &child);
			if (refresh == 1) {
				GnomeVFSURI *d_uri;
				gchar *d_name;
				guint hashkey;
				/* delete 'this' ! */
				gtk_tree_model_get(GTK_TREE_MODEL(tstore), &this, FILENAME_COLUMN, &d_name, URI_COLUMN, &d_uri, -1);
				DEBUG_MSG("fb2_treestore_delete_children_refresh1, delete %s ",d_name);
				DEBUG_URI(d_uri, TRUE);
				gtk_tree_store_remove(tstore,&this);
				
				hashkey = gnome_vfs_uri_hash(d_uri);
				g_hash_table_remove(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_itable, &hashkey);
				
				gnome_vfs_uri_unref(d_uri);
				g_free(d_name);
			}
		}
	}
}
/**
 * fb2_treestore_mark_children_refresh1:
 *
 * sets value 1 in REFRESH_COLUMN for all children of 'iter' 
 */
static void fb2_treestore_mark_children_refresh1(GtkTreeStore *tstore, GtkTreeIter *iter) {
	GtkTreeIter child;
	DEBUG_MSG("fb2_treestore_mark_children_refresh1, started\n");
	if (gtk_tree_model_iter_children(GTK_TREE_MODEL(tstore),&child,iter)) {
		gboolean cont=TRUE;
		while (cont) {
			gtk_tree_store_set(GTK_TREE_STORE(tstore),&child,REFRESH_COLUMN, 1,-1);
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(tstore), &child);
		}
	}
	DEBUG_MSG("fb2_treestore_mark_children_refresh1, finished\n");
}


/**
 * fb2_load_directory_lcb
 *
 * the callback from the asynchronous directory load
 *
 */
static void fb2_load_directory_lcb(GnomeVFSAsyncHandle *handle,GnomeVFSResult result,GList *list,guint entries_read,gpointer data) {
	GList *tmplist;
	Tdirectoryloaddata *cdata = data;
	DEBUG_MSG("fb2_load_directory_lcb, appending %d childs to ",entries_read);
	DEBUG_URI(cdata->p_uri, TRUE);
	tmplist = g_list_first(list);
	while (tmplist) {
		GnomeVFSFileInfo *finfo = tmplist->data;
		
		if (strcmp(finfo->name,".")!=0 && strcmp(finfo->name,"..")!=0) {
			GnomeVFSURI *child_uri;
			gint type;
			/*DEBUG_MSG("found %s with type %d\n", finfo->name, finfo->type);*/
			if (finfo->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
				type = TYPE_DIR;
			} else {
				type = TYPE_FILE;
			}
			child_uri = gnome_vfs_uri_append_file_name(cdata->p_uri,finfo->name);
			fb2_add_filesystem_entry(cdata->parent, child_uri, type, TRUE);
			gnome_vfs_uri_unref(child_uri);
		}
		tmplist = g_list_next(tmplist);
	}
	if (result == GNOME_VFS_ERROR_EOF) {
		DEBUG_MSG("fb2_load_directory_lcb, EOF, list=%p\n",list);
		fb2_treestore_delete_children_refresh1(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore, cdata->parent);
		FILEBROWSER2CONFIG(main_v->fb2config)->uri_in_refresh = g_list_remove(FILEBROWSER2CONFIG(main_v->fb2config)->uri_in_refresh, cdata->p_uri);
		g_free(cdata);
	}
}

/**
 * fb2_fill_dir_async:
 *
 * starts the asynchronous diretory fill
 *
 * the GtkTreeIter* 'parent' should exist after this function returns, use the 
 * GtkTreeIter that is stored in the hashtable to make sure it is persistent
 *
 * the uri passed to this function should also exist after this function returns
 * use an uri stored in the treestore to make sure it is persistent
 *
 */
static void fb2_fill_dir_async(GtkTreeIter *parent, GnomeVFSURI *uri) {
	if (g_list_find(FILEBROWSER2CONFIG(main_v->fb2config)->uri_in_refresh, uri) == NULL) {
		GnomeVFSAsyncHandle *handle;
		Tdirectoryloaddata *cdata;

		fb2_treestore_mark_children_refresh1(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore, parent);
		
		cdata = g_new(Tdirectoryloaddata,1);
		cdata->parent = parent;
		cdata->p_uri = uri;
		FILEBROWSER2CONFIG(main_v->fb2config)->uri_in_refresh = g_list_prepend(FILEBROWSER2CONFIG(main_v->fb2config)->uri_in_refresh, uri);
		DEBUG_MSG("fb2_fill_dir_async, opening ");
		DEBUG_URI(cdata->p_uri, TRUE);
		gnome_vfs_async_load_directory_uri(&handle,uri,GNOME_VFS_FILE_INFO_DEFAULT|GNOME_VFS_FILE_INFO_FOLLOW_LINKS,
									10,GNOME_VFS_PRIORITY_DEFAULT,fb2_load_directory_lcb,cdata);
	}
#ifdef DEBUG	
	 else {
		DEBUG_MSG("fb2_fill_dir_async, uri is already refreshing!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	}
#endif
}

/**
 * fb2_uri_from_iter:
 *
 * reads the uri stored in the treestore for iter 'iter'
 */
static GnomeVFSURI *fb2_uri_from_iter(GtkTreeIter *iter) {
	GnomeVFSURI *uri;
	gtk_tree_model_get(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore), iter, URI_COLUMN, &uri, -1);
	DEBUG_MSG("fb2_uri_from_iter, iter %p points to uri ",iter);
	DEBUG_URI(uri, TRUE);
	return uri;
}

/**
 * fb2_refresh_dir:
 *
 * the 'dir' iter that is passed to this function will be used in the asynchronous
 * update, so it needs to be a persistent iter. The iters that are in the hashtable 
 * are persistent. Most others are not. So better pass NULL then pass a temporary 
 * iter
 *
 * the uri should also be peristent, the uri's in the treestore are persistent
 */
static void fb2_refresh_dir(GnomeVFSURI *uri, GtkTreeIter *dir) {
	/* first we mark all children as 'refresh=1' in the REFRESH_COLUMN, then
	we read the directory, and all existing items are set to refresh=0, after we
	have finished reading the directory, all items that still have refresh=1 can be 
	deleted */
	if (uri != NULL && dir == NULL) {
		guint hashkey;
		DEBUG_MSG("fb2_refresh_dir, request iter from uri ");
		DEBUG_URI(uri, TRUE);
		hashkey = gnome_vfs_uri_hash(uri);
		dir = g_hash_table_lookup(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_itable, &hashkey);
	}
	if (dir != NULL && uri == NULL) {
		DEBUG_MSG("fb2_refresh_dir, request the uri from iter: ");
		uri = fb2_uri_from_iter(dir);
		DEBUG_URI(uri, TRUE);
	}
	if (dir && uri) {
		fb2_fill_dir_async(dir, uri);
	}
}
/**
 * fb2_refresh_parent_of_uri:
 *
 * convenience function, will refresh the parent directory of child_uri
 */
static void fb2_refresh_parent_of_uri(GnomeVFSURI *child_uri) {
	guint hashkey;
	GtkTreeIter *iter;
	GnomeVFSURI *parent_uri = gnome_vfs_uri_get_parent(child_uri);
	hashkey = gnome_vfs_uri_hash(parent_uri);
	iter = g_hash_table_lookup(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_itable, &hashkey);
	fb2_refresh_dir(NULL, iter);
	gnome_vfs_uri_unref(parent_uri);
}
/**
 * fb2_build_dir:
 *
 * builds directories up to uri (uri should point to a directory, not to a file)
 *
 * returns a pointer to the iter stored in the hashtable, so this is a persistent iter
 */
static GtkTreeIter *fb2_build_dir(GnomeVFSURI *uri) {
	GnomeVFSURI* tmp, *parent_uri;
	GtkTreeIter *parent = NULL;

	tmp = gnome_vfs_uri_dup(uri);
	while (!parent && gnome_vfs_uri_has_parent(tmp)) {
		guint hashkey;
		GnomeVFSURI* tmp2 = gnome_vfs_uri_get_parent(tmp);
		gnome_vfs_uri_unref(tmp);
		tmp = tmp2;
		hashkey = gnome_vfs_uri_hash(tmp);
		parent = g_hash_table_lookup(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_itable, &hashkey);
		DEBUG_MSG("fb2_build_dir, found parent=%p for hashkey=%u for uri ",parent,hashkey);
		DEBUG_URI(tmp, TRUE);
	}/* after this loop 'tmp' is newly allocated */
	if (!parent) {
		DEBUG_MSG("adding toplevel ");
		DEBUG_URI(tmp, TRUE);
		parent = fb2_add_filesystem_entry(NULL, tmp, TYPE_DIR, FALSE);
		parent_uri = tmp;
	} else {
		parent_uri = tmp;
	}/* after this loop 'parent_uri'='tmp' is newly allocated */
	DEBUG_MSG("we should have a parent here!! parent(%p), and the uri for that parent is ",parent);
	DEBUG_URI(parent_uri, TRUE);
	{
		gboolean done=FALSE;
		
		while (!done) {
			GnomeVFSURI* tmp2 = gnome_vfs_uri_dup(uri); /* both 'parent_uri'='tmp' and 'tmp2' are newly allocated */
			while (!gnome_vfs_uri_is_parent(parent_uri,tmp2,FALSE)) {
				GnomeVFSURI* tmp3 = gnome_vfs_uri_get_parent(tmp2);
				gnome_vfs_uri_unref(tmp2);
				tmp2 = tmp3;
			} /* after this loop both 'parent_uri'='tmp' and 'tmp2' are newly allocated */
			parent = fb2_add_filesystem_entry(parent, tmp2, TYPE_DIR, gnome_vfs_uri_equal(tmp2,uri));
			gnome_vfs_uri_unref(parent_uri);
			parent_uri = tmp2; /* here 'parent_uri'='tmp2' is newly allocated */
			DEBUG_MSG("new parent_uri=");
			DEBUG_URI(parent_uri, FALSE);
			DEBUG_MSG(", requested uri=");
			DEBUG_URI(uri, TRUE);
			if (gnome_vfs_uri_equal(parent_uri,uri)) {
				DEBUG_MSG("exit loop\n");
				done = TRUE;
			}
		}
	}
	gnome_vfs_uri_unref(parent_uri); /* no memory leaks in the uri's... (I hope) */
	return parent;
}
/**
 * fb2_focus_dir:
 *
 * builds the directory tree, refreshed the directory, and (if not noselect) expands and selects the 
 * result. during the expand it will block the expand signal handler, so the expand callback
 * will not be called
 *
 */
static void fb2_focus_dir(Tfilebrowser2 *fb2, GnomeVFSURI *uri, gboolean noselect) {
	guint hashkey;
	GtkTreeIter *dir;
	if (!uri) {
		DEBUG_MSG("fb2_focus_dir, WARNING, CANNOT FOCUS WITHOUT URI\n");
		return;
	}
	hashkey = gnome_vfs_uri_hash(uri);
	dir = g_hash_table_lookup(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_itable, &hashkey);
	DEBUG_MSG("fb2_focus_dir, found %p for hashkey %u for uri ",dir,hashkey);
	DEBUG_URI(uri, TRUE);
	if (!dir) {
		dir = fb2_build_dir(uri);
	}
	if (dir) {
		GtkTreePath *fs_path;
		DEBUG_DIRITER(fb2, dir);
		/* set this directory as the top tree for the file widget */
		fs_path = gtk_tree_model_get_path(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),dir);
		if (fs_path) {
			DEBUG_MSG("fb2_focus_dir, set new root %s for file list..\n",gnome_vfs_uri_extract_short_name(uri));
			refilter_filelist(fb2, fs_path);
			DEBUG_MSG("fb2_focus_dir, expand dir tree to this dir..\n");
			if (!noselect) {
				GtkTreePath *filter_path = gtk_tree_model_filter_convert_child_path_to_path(GTK_TREE_MODEL_FILTER(fb2->dir_tfilter),fs_path);
				if (filter_path) {
					GtkTreePath *sort_path = gtk_tree_model_sort_convert_child_path_to_path(GTK_TREE_MODEL_SORT(fb2->dir_tsort),filter_path);
					if (sort_path) {
						g_signal_handler_block(fb2->dir_v, fb2->expand_signal);
						gtk_tree_view_expand_to_path(GTK_TREE_VIEW(fb2->dir_v), sort_path);
						g_signal_handler_unblock(fb2->dir_v, fb2->expand_signal);
						gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(fb2->dir_v)),sort_path);
						gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(fb2->dir_v),sort_path,0,TRUE,0.5,0.5);
						gtk_tree_path_free(sort_path);
					}
					gtk_tree_path_free(filter_path);
				}
			} else {
				/* 'uri' is not persistent, 'dir' is peristent, so only pass 'dir' 
				the "select" or "expand" signal will also refresh this directory, so 
				we call this only on 'noselect'
				*/
				fb2_refresh_dir(NULL, dir);
			}
			gtk_tree_path_free(fs_path);
		} else {
			DEBUG_MSG("NO TREEPATH FOR THE DIR ITER WE TRY TO FOCUS ?!?!?!\n");
		}
	}
}
/**
 * fb2_focus_document:
 *
 * external function, will make sure the filebrowser shows the current dcoument
 * directory
 */
void fb2_focus_document(Tbfwin *bfwin, Tdocument *doc) {
	DEBUG_MSG("fb2_focus_document,doc->uri=%s\n",doc->uri);
	if (bfwin->fb2 && doc->uri) {
		GnomeVFSURI *file_uri, *dir_uri;
		/* first we make sure we have the correct directory open, then
		we could select the document, but if the directory *was* open already, this
		could disturb the user... hmmm... */
		file_uri = gnome_vfs_uri_new(doc->uri);
		dir_uri = gnome_vfs_uri_get_parent(file_uri);
		fb2_focus_dir(FILEBROWSER2(bfwin->fb2), dir_uri, FALSE);
		gnome_vfs_uri_unref(file_uri);
		gnome_vfs_uri_unref(dir_uri);
	}
}

static gboolean name_visible_in_filter(Tfilebrowser2 *fb2, gchar *name) {
	GList *tmplist;
	
	if (!fb2->curfilter) {
		return TRUE;
	}
	if (!fb2->curfilter->filetypes) {
		return !fb2->curfilter->mode;
	}
	tmplist = g_list_first(fb2->curfilter->filetypes);
	while (tmplist) {
		if (filename_test_extensions(((Tfiletype *)tmplist->data)->extensions, name)) {
			return fb2->curfilter->mode;
		}
		tmplist = g_list_next(tmplist);
	}
	return !fb2->curfilter->mode;
}

/**
 * tree_model_filter_func
 *
 * will return TRUE if this file should be visible in the dir view
 */
static gboolean tree_model_filter_func(GtkTreeModel *model,GtkTreeIter *iter,gpointer data) {
	Tfilebrowser2 *fb2 = data;
	gchar *name;
	gint len, type;
	GnomeVFSURI *uri;
#ifdef DEBUG
	if (model != GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore)) {
		DEBUG_MSG("file_list_filter_func, WRONG MODEL\n");
	}
#endif
	gtk_tree_model_get(GTK_TREE_MODEL(model), iter, FILENAME_COLUMN, &name, URI_COLUMN, &uri, TYPE_COLUMN, &type, -1);
	if (type != TYPE_DIR) return FALSE;
	if (!name) return FALSE;
	if (fb2->basedir) {
		/* show only our basedir on the root level, no other directories at that level */
		if (!gnome_vfs_uri_is_parent(fb2->basedir, uri, TRUE) && !gnome_vfs_uri_equal(fb2->basedir, uri)) {
			DEBUG_MSG("tree_model_filter_func, not showing because is_parent=%d, equal=%d, for ",gnome_vfs_uri_is_parent(fb2->basedir,uri,TRUE),gnome_vfs_uri_equal(fb2->basedir,uri));
			DEBUG_URI(uri,FALSE);
			DEBUG_MSG(" compared to basedir ");
			DEBUG_URI(fb2->basedir,TRUE);
			return FALSE;
		}
	}
	
	if (!fb2->bfwin->session->filebrowser_show_backup_files) {
		len = strlen(name);
		if (len > 1 && (name[len-1] == '~')) return FALSE;
	}
	if (!fb2->bfwin->session->filebrowser_show_hidden_files) {
		if (name[0] == '.') return FALSE;
#ifdef DEBUG
		{
			GnomeVFSURI *tmp1, *tmp2;
			tmp1 = gnome_vfs_uri_dup(uri);
			while (gnome_vfs_uri_has_parent(tmp1)) {
				gchar *tmpname;
				tmp2 = tmp1;
				tmp1 = gnome_vfs_uri_get_parent(tmp2);
				gnome_vfs_uri_unref(tmp2);
				tmpname = gnome_vfs_uri_extract_short_path_name(tmp1);
				if (tmpname[0] == '.') {
					DEBUG_MSG("tree_model_filter_func, one of the parents %s is not visible, make invisible too ", tmpname);
					DEBUG_URI(tmp1, TRUE);
					gnome_vfs_uri_unref(tmp1);
					g_free(tmpname);
					return FALSE;
				}
				g_free(tmpname);
			}
		}
#endif
	}
	return TRUE;
}
/**
 * file_list_filter_func:
 *
 * will return TRUE if this file should be visible in the file list
 */
static gboolean file_list_filter_func(GtkTreeModel *model,GtkTreeIter *iter,gpointer data) {
	Tfilebrowser2 *fb2 = data;
	gchar *name;
	gint len, type;
#ifdef DEBUG
	if (model != GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore)) {
		DEBUG_MSG("file_list_filter_func, WRONG MODEL\n");
	}
#endif
	gtk_tree_model_get(GTK_TREE_MODEL(model), iter, FILENAME_COLUMN, &name, TYPE_COLUMN, &type, -1);
	if (type != TYPE_FILE) return FALSE;
	if (!name) return FALSE;
	if (!fb2->bfwin->session->filebrowser_show_backup_files) {
		len = strlen(name);
		if (len > 1 && (name[len-1] == '~')) return FALSE;
	}
	if (!fb2->bfwin->session->filebrowser_show_hidden_files) {
		if (name[0] == '.') return FALSE;
	}
	return name_visible_in_filter(fb2, name);
}
/**
 * refilter_dirlist:
 *
 * will set the root of the directory view to 'newroot'
 *
 */
static void refilter_dirlist(Tfilebrowser2 *fb2, GtkTreePath *newroot) {
	GtkTreeModel *oldmodel1, *oldmodel2;
	GtkTreePath *useroot;
	oldmodel1 = fb2->dir_tfilter;
	oldmodel2 = fb2->dir_tsort;
	if (newroot) {
		GnomeVFSURI *uri;
		/* store this basedir in fb2 */
		if (fb2->basedir) gnome_vfs_uri_unref(fb2->basedir);
		uri = fb2_uri_from_fspath(fb2, newroot);
		fb2->basedir = gnome_vfs_uri_dup(uri);
		DEBUG_MSG("refilter_dirlist, basedir is set to ");
		DEBUG_URI(uri, FALSE);
		DEBUG_MSG(" at path(%p) ",newroot);
		DEBUG_TPATH(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore), newroot, TRUE);
		/* to make it possible to select the root, we move the filter-root one up*/
		useroot = gtk_tree_path_copy(newroot);
		gtk_tree_path_up(useroot);
	} else {
		DEBUG_MSG("refilter_dirlist, show full tree, and unset basedir\n");
		useroot = NULL;
		if (fb2->basedir) gnome_vfs_uri_unref(fb2->basedir);
		fb2->basedir = NULL;
	}
	fb2->dir_tfilter = gtk_tree_model_filter_new(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),useroot);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(fb2->dir_tfilter),tree_model_filter_func,fb2,NULL);
	fb2->dir_tsort = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(fb2->dir_tfilter));
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fb2->dir_tsort),FILENAME_COLUMN,GTK_SORT_ASCENDING);
	DEBUG_MSG("refilter_dirlist, connect dir_v to new sort(%p)&filter(%p) model\n", fb2->dir_tsort, fb2->dir_tfilter);
	gtk_tree_view_set_model(GTK_TREE_VIEW(fb2->dir_v),GTK_TREE_MODEL(fb2->dir_tsort));
	g_object_unref(oldmodel1);
	g_object_unref(oldmodel2);
}
/**
 * refilter_filelist:
 *
 * will set the root of the file view to 'newroot'
 *
 */
static void refilter_filelist(Tfilebrowser2 *fb2, GtkTreePath *newroot) {
	GtkTreePath *curpath;
	g_object_get(fb2->file_lfilter, "virtual-root", &curpath, NULL);
	if ((curpath == NULL && newroot != NULL) || (curpath != NULL && newroot == NULL) || (curpath != NULL && gtk_tree_path_compare(curpath, newroot) != 0)) {
		GtkTreeModel *oldmodel1, *oldmodel2;
		oldmodel1 = fb2->file_lfilter;
		oldmodel2 = fb2->file_lsort;
		
		fb2->file_lfilter = gtk_tree_model_filter_new(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),newroot);
		gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(fb2->file_lfilter),file_list_filter_func,fb2,NULL);
	
		fb2->file_lsort = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(fb2->file_lfilter));
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fb2->file_lsort),FILENAME_COLUMN,GTK_SORT_ASCENDING);
		DEBUG_MSG("refilter_filelist, connect file_v to new sort(%p)&filter(%p) model\n", fb2->file_lsort, fb2->file_lfilter);
		gtk_tree_view_set_model(GTK_TREE_VIEW(fb2->file_v),GTK_TREE_MODEL(fb2->file_lsort));
		g_object_unref(oldmodel1);
		g_object_unref(oldmodel2);
	}
#ifdef DEBUG
	else DEBUG_MSG("refilter_filelist, root did not change!\n");
#endif
}

/**
 * fb2_fspath_from_dir_sortpath:
 *
 * return a newly allocated treepath for the filesystem_tstore based on a 'sort_path' from the 
 * directory sort model
 */
static GtkTreePath *fb2_fspath_from_dir_sortpath(Tfilebrowser2 *fb2, GtkTreePath *sort_path) {
	if (sort_path) {
		GtkTreePath *filter_path;
		DEBUG_MSG("fb2_fspath_from_dir_sortpath, sort_path=");
		DEBUG_TPATH(fb2->dir_tsort, sort_path,TRUE);
		filter_path = gtk_tree_model_sort_convert_path_to_child_path(GTK_TREE_MODEL_SORT(fb2->dir_tsort),sort_path);
		if (filter_path) {
			GtkTreePath *fs_path;
			DEBUG_MSG("fb2_fspath_from_dir_sortpath, filter_path=");
			DEBUG_TPATH(fb2->dir_tfilter, filter_path,TRUE);
			fs_path = gtk_tree_model_filter_convert_path_to_child_path(GTK_TREE_MODEL_FILTER(fb2->dir_tfilter), filter_path);
			gtk_tree_path_free(filter_path);
#ifdef DEBUG
			if (fs_path) {
				DEBUG_MSG("fb2_fspath_from_dir_sortpath, fs_path=");
				DEBUG_TPATH(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore), fs_path,TRUE);
			}
#endif
			return fs_path;
		}
	}
	return NULL;
}
/**
 * fb2_fspath_from_file_sortpath:
 *
 * return a newly allocated treepath for the filesystem_tstore based on a 'sort_path' from the 
 * file sort model
 */
static GtkTreePath *fb2_fspath_from_file_sortpath(Tfilebrowser2 *fb2, GtkTreePath *sort_path) {
	if (sort_path) {
		GtkTreePath *filter_path;
		filter_path = gtk_tree_model_sort_convert_path_to_child_path(GTK_TREE_MODEL_SORT(fb2->file_lsort),sort_path);
		if (filter_path) {
			GtkTreePath *fs_path;
			fs_path = gtk_tree_model_filter_convert_path_to_child_path(GTK_TREE_MODEL_FILTER(fb2->file_lfilter), filter_path);
			gtk_tree_path_free(filter_path);
			return fs_path;
		}
	}
	return NULL;
}
/**
 * fb2_fspath_from_uri:
 *
 * return a newly allocated treepath for the filesystem_tstore based on 'uri'
 */
static GtkTreePath *fb2_fspath_from_uri(Tfilebrowser2 *fb2, GnomeVFSURI *uri) {
	GtkTreeIter *iter;
	guint hashkey;

	hashkey = gnome_vfs_uri_hash(uri);
	iter = g_hash_table_lookup(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_itable, &hashkey);
	if (iter) {
		return gtk_tree_model_get_path(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore), iter);
	}
	return NULL;
}
/**
 * fb2_uri_from_fspath:
 *
 * returns the uri stored in the treestore based on the 'fs_path' from the filesystem treestore
 */
static GnomeVFSURI *fb2_uri_from_fspath(Tfilebrowser2 *fb2, GtkTreePath *fs_path) {
	if (fs_path) {
		GtkTreeIter fsiter;
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),&fsiter,fs_path)) {
			return fb2_uri_from_iter(&fsiter);
		} else {
			DEBUG_MSG("fb2_uri_from_fspath, WARNING, no fsiter for fs_path=%p ",fs_path);
			DEBUG_TPATH(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore), fs_path, TRUE);
		}
	}
	return NULL;
}
/**
 * fb2_uri_from_file_sort_path:
 *
 * returns the uri stored in the treestore based on the 'sort_path' from the file sort
 */
static GnomeVFSURI *fb2_uri_from_file_sort_path(Tfilebrowser2 *fb2, GtkTreePath *sort_path) {
	GnomeVFSURI *uri;
	GtkTreePath *fs_path = fb2_fspath_from_file_sortpath(fb2, sort_path);
	uri = fb2_uri_from_fspath(fb2, fs_path);
	gtk_tree_path_free(fs_path);
	return uri;
}
/**
 * fb2_uri_from_dir_sort_path:
 *
 * returns the uri stored in the treestore based on the 'sort_path' from the dir sort
 */
static GnomeVFSURI *fb2_uri_from_dir_sort_path(Tfilebrowser2 *fb2, GtkTreePath *sort_path) {
	GnomeVFSURI *uri;
	GtkTreePath *fs_path = fb2_fspath_from_dir_sortpath(fb2, sort_path);
	DEBUG_MSG("fb2_uri_from_dir_sort_path, retrieved fs_path=%p from sort_path=%p\n",fs_path, sort_path);
	uri = fb2_uri_from_fspath(fb2, fs_path);
	gtk_tree_path_free(fs_path);
	return uri;
}
/**
 * fb2_isdir_from_dir_sort_path:
 *
 * returns TRUE if 'sort_path' from the directory sort model points to a directory
 */
static gboolean fb2_isdir_from_dir_sort_path(Tfilebrowser2 *fb2, GtkTreePath *sort_path) {
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter(fb2->dir_tsort,&iter,sort_path)) {
		gint type;
		gtk_tree_model_get(fb2->dir_tsort, &iter, TYPE_COLUMN, &type, -1);
		DEBUG_MSG("fb2_isdir_from_file_sort_path, type=%d\n",type);
		return (type == TYPE_DIR || type == TYPE_HIDDEN_DIR);
	} else {
		DEBUG_MSG("fb2_isdir_from_dir_sort_path, WARNING, sort_path CANNOT be converted into a valid iter!\n");
	}
/*	GtkTreePath *fs_path = fb2_fspath_from_dir_sortpath(fb2, sort_path);
	if (fs_path) {
		GtkTreeIter fsiter;
		gint type;
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),&fsiter,fs_path)) {
			gtk_tree_model_get(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore), &fsiter, TYPE_COLUMN, &type, -1);
			DEBUG_MSG("fb2_isdir_from_file_sort_path, type=%d\n",type);
			gtk_tree_path_free(fs_path);
			return (type == TYPE_DIR || type == TYPE_HIDDEN_DIR);
		}
		gtk_tree_path_free(fs_path);
	}*/
	return FALSE;
}
/**
 * fb2_fspath_from_dir_selection:
 *
 * returns a newly allocated treepath for the current selected entry in the dir view
 */
static GtkTreePath *fb2_fspath_from_dir_selection(Tfilebrowser2 *fb2) {
	GtkTreeModel *sort_model;
	GtkTreeIter sort_iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(fb2->dir_v));
	if (gtk_tree_selection_get_selected(selection,&sort_model,&sort_iter)) {
		GtkTreePath *sort_path;
		sort_path = gtk_tree_model_get_path(sort_model,&sort_iter);
		if (sort_path) {
			GtkTreePath *fs_path;
			fs_path = fb2_fspath_from_dir_sortpath(fb2, sort_path);
			gtk_tree_path_free(sort_path);
			return fs_path;
		}
		DEBUG_MSG("fb2_fspath_from_dir_selection, a selection, but no sort_path?\n");
	}
	DEBUG_MSG("fb2_fspath_from_dir_selection, returning NULL\n");
	return NULL;
}
/**
 * fb2_uri_from_file_selection:
 *
 * returns the uri stored in the treestore for the current selected entry in the file_v
 */
static GnomeVFSURI *fb2_uri_from_file_selection(Tfilebrowser2 *fb2) {
	GtkTreeModel *sort_model;
	GtkTreeIter sort_iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(fb2->file_v));
	if (gtk_tree_selection_get_selected(selection,&sort_model,&sort_iter)) {
		GtkTreePath *sort_path;
		sort_path = gtk_tree_model_get_path(sort_model,&sort_iter);
		if (sort_path) {
			GnomeVFSURI *uri;
			uri = fb2_uri_from_file_sort_path(fb2,sort_path);
			gtk_tree_path_free(sort_path);
			return uri;
		}
	}
	return NULL;
}
/**
 * fb2_uri_from_dir_selection:
 *
 * returns the uri stored in the treestore for the current selected entry in the dir_v
 */
static GnomeVFSURI *fb2_uri_from_dir_selection(Tfilebrowser2 *fb2) {
	GtkTreeModel *sort_model;
	GtkTreeIter sort_iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(fb2->dir_v));
	if (gtk_tree_selection_get_selected(selection,&sort_model,&sort_iter)) {
		GtkTreePath *sort_path;
		sort_path = gtk_tree_model_get_path(sort_model,&sort_iter);
		if (sort_path) {
			GnomeVFSURI *uri;
			uri = fb2_uri_from_dir_sort_path(fb2,sort_path);
			gtk_tree_path_free(sort_path);
			return uri;
		}
	}
	return NULL;
}
/**
 * handle_activate_on_file:
 *
 * opens the file, project or inserts the image pointer to by 'uri'
 */
static void handle_activate_on_file(Tfilebrowser2 *fb2, GnomeVFSURI *uri) {
	Tfiletype *ft;
	gchar *filename;
	filename = gnome_vfs_uri_to_string(uri, GNOME_VFS_URI_HIDE_PASSWORD);
	ft = get_filetype_by_filename_and_content(filename, NULL);
	DEBUG_MSG("handle_activate_on_file, file %s has type %p\n",filename, ft);
	if (ft == NULL || ft->editable) {
		/* doc_new_with_file(fb2->bfwin,filename, FALSE, FALSE); */
		doc_new_from_uri(fb2->bfwin, NULL, uri, NULL, FALSE, FALSE, -1);
	} else if (strcmp(ft->type, "webimage")==0 || strcmp(ft->type, "image")==0) {
		gchar *relfilename = create_relative_link_to(fb2->bfwin->current_document->uri, filename);
		image_insert_from_filename(fb2->bfwin,relfilename);
		g_free(relfilename);
	} else if (strcmp(ft->type, "bfproject") == 0) {
		project_open_from_file(fb2->bfwin, filename);
	} else {
		DEBUG_MSG("handle_activate_on_file, file %s is not-editable, do something special now?\n",filename);
	}
	g_free(filename);
	DEBUG_MSG("handle_activate_on_file, finished\n");
}

static void fb2rpopup_refresh(Tfilebrowser2 *fb2) {
	GnomeVFSURI *baseuri;
	if (fb2->last_popup_on_dir) {
		baseuri = gnome_vfs_uri_dup(fb2_uri_from_dir_selection(fb2));
	} else {
		GnomeVFSURI *childuri = fb2_uri_from_file_selection(fb2);
		baseuri = gnome_vfs_uri_get_parent(childuri);
	}
	if (baseuri) {
		guint hashkey;
		GtkTreeIter *iter;
		hashkey = gnome_vfs_uri_hash(baseuri);
		iter = g_hash_table_lookup(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_itable, &hashkey);
		fb2_refresh_dir(NULL, iter);
		gnome_vfs_uri_unref(baseuri);
	}
}

static void fb2rpopup_new(Tfilebrowser2 *fb2, gboolean newisdir) {
	GnomeVFSURI *baseuri;
	if (fb2->last_popup_on_dir) {
		baseuri = gnome_vfs_uri_dup(fb2_uri_from_dir_selection(fb2));
	} else {
		GnomeVFSURI *childuri = fb2_uri_from_file_selection(fb2);
		baseuri = gnome_vfs_uri_get_parent(childuri);
	}
	if (baseuri) {
		GnomeVFSURI *newuri;
		GnomeVFSResult res;
		if (newisdir) {
			newuri = gnome_vfs_uri_append_file_name(baseuri, _("New directory"));
			res = gnome_vfs_make_directory_for_uri(newuri, 0755);
		} else {
			GnomeVFSHandle *handle;
			newuri = gnome_vfs_uri_append_file_name(baseuri, _("New file"));
			res = gnome_vfs_create_uri(&handle,newuri,GNOME_VFS_OPEN_WRITE,FALSE,0644);
			if (res) {
				res = gnome_vfs_close(handle);
			}
		}
		if (res == GNOME_VFS_OK) {
			fb2_refresh_parent_of_uri(newuri);
		} else {
			DEBUG_MSG("fb2rpopup_new, failed creation..\n");
		}
		gnome_vfs_uri_unref(newuri);
		gnome_vfs_uri_unref(baseuri);
	}
}

static void fb2rpopup_rename(Tfilebrowser2 *fb2) {
	GnomeVFSURI *olduri;
	if (fb2->last_popup_on_dir) {
		olduri = fb2_uri_from_dir_selection(fb2);
	} else {
		olduri = fb2_uri_from_file_selection(fb2);
	}
	if (olduri) {
		Tdocument *tmpdoc;
		GList *alldocs;
		gchar *oldfilename;
		GnomeVFSURI *newuri=NULL;
		/* Use doc_save(doc, 1, 1) if the file is open. */
		oldfilename = gnome_vfs_uri_to_string(olduri,0);
		alldocs = return_allwindows_documentlist();
		tmpdoc = documentlist_return_document_from_filename(alldocs,oldfilename);
		g_list_free(alldocs);
		if (tmpdoc != NULL) {
			DEBUG_MSG("File is open. Calling doc_save().\n");
			/* If an error occurs, doc_save takes care of notifying the user.
			 * Currently, nothing is done here. */	
			doc_save_backend(tmpdoc, TRUE, TRUE, FALSE, FALSE);
		} else { /* olduri is not open */
			gchar *newfilename=NULL;
			/* Promt user, "File/Move To"-style. */
			newfilename = ask_new_filename(fb2->bfwin, oldfilename, oldfilename, TRUE);
			if (newfilename) {
				GnomeVFSResult res;
				newuri = gnome_vfs_uri_new(newfilename);
				res = gnome_vfs_move_uri(olduri,newuri,TRUE);
				if (res != GNOME_VFS_OK) {
					gchar *errmessage = g_strconcat(_("Could not rename\n"), oldfilename, NULL);
					error_dialog(fb2->bfwin->main_window,errmessage, NULL);
					g_free(errmessage);
				}
				g_free(newfilename);
			}
		}
		/* Refresh the appropriate parts of the filebrowser */
		fb2_refresh_parent_of_uri(olduri);
		if (newuri) {
			fb2_refresh_parent_of_uri(newuri);
		}
		g_free(oldfilename);
		gnome_vfs_uri_unref(newuri);
	}
}

static void fb2rpopup_delete(Tfilebrowser2 *fb2) {
	GnomeVFSURI *uri;
	
	if (fb2->last_popup_on_dir) {
		uri = fb2_uri_from_dir_selection(fb2);
	} else {
		uri = fb2_uri_from_file_selection(fb2);
	}
	if (uri) {
		gchar *buttons[] = {GTK_STOCK_CANCEL, GTK_STOCK_DELETE, NULL};
		gchar *label;
		gint retval;
		gchar *filename;
		filename = gnome_vfs_uri_to_string(uri,0);
		label = g_strdup_printf(_("Are you sure you want to delete \"%s\" ?"), filename);
		retval = multi_query_dialog(fb2->bfwin->main_window,label, _("If you delete this file, it will be permanently lost."), 0, 0, buttons);
		g_free(label);
		if (retval == 1) {
			GnomeVFSResult res;
			gchar *errmessage = NULL;
			if (fb2->last_popup_on_dir) {
				res = gnome_vfs_remove_directory_from_uri(uri);
			} else {
				res = gnome_vfs_unlink_from_uri(uri);
			}
			if (res != GNOME_VFS_OK) {
				errmessage = g_strconcat(_("Could not delete \n"), filename, NULL);
				error_dialog(fb2->bfwin->main_window,errmessage, NULL);
				g_free(errmessage);
			} else {
				GList *alldocs = return_allwindows_documentlist();
				Tdocument *exdoc = documentlist_return_document_from_filename(alldocs, filename);
				if (exdoc) document_unset_filename(exdoc);
				g_list_free(alldocs);
			}
			fb2_refresh_parent_of_uri(uri);
		}
		g_free(filename);
	}
}

static void fb2rpopup_rpopup_action_lcb(Tfilebrowser2 *fb2,guint callback_action, GtkWidget *widget) {
	DEBUG_MSG("fb2rpopup_rpopup_action_lcb, called with action %d and widget %p\n",callback_action,widget);
	switch (callback_action) {
		case 1:
			{
				GnomeVFSURI *uri = fb2_uri_from_file_selection(fb2);
				handle_activate_on_file(fb2, uri);
			}
		break;
		case 2:
			fb2rpopup_rename(fb2);
		break;
		case 3:
			fb2rpopup_delete(fb2);
		break;
		case 4:
			fb2rpopup_new(fb2, FALSE);
		break;
		case 5:
			fb2rpopup_new(fb2, TRUE);
		break;
		case 6:
			fb2rpopup_refresh(fb2);
		break;
		case 7:
			{
				gchar *curi;
				GnomeVFSURI *uri = fb2_uri_from_dir_selection(fb2);
				curi = gnome_vfs_uri_to_string(uri,0);
				files_advanced_win(fb2->bfwin, curi);
				g_free(curi);
			}
		break;
		case 8:
			{
				GtkTreePath *fs_path;
				fs_path = fb2_fspath_from_dir_selection(fb2);
				DEBUG_MSG("fb2rpopup_rpopup_action_lcb, fs_path=%p\n",fs_path);
				refilter_dirlist(fb2, fs_path);
				gtk_tree_path_free(fs_path);
				fb2_focus_document(fb2->bfwin, fb2->bfwin->current_document);
			}
		break;
		case 9:
			refilter_dirlist(fb2, NULL);
			fb2_focus_document(fb2->bfwin, fb2->bfwin->current_document);
		break;
		case 15:
			main_v->props.filebrowser_focus_follow = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
			if (main_v->props.filebrowser_focus_follow) {
				fb2_focus_document(fb2->bfwin, fb2->bfwin->current_document);
			}
		break;
		case 16:
			fb2->bfwin->session->filebrowser_show_hidden_files = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
			gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(fb2->dir_tfilter));
			gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(fb2->file_lfilter));
		break;
		case 17:
			fb2->bfwin->session->filebrowser_show_backup_files = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
			gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(fb2->dir_tfilter));
			gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(fb2->file_lfilter));
		break;
	}
}

static Tfilter *find_filter_by_name(const gchar *name) {
	GList *tmplist = g_list_first(FILEBROWSER2CONFIG(main_v->fb2config)->filters);
	while(tmplist) {
		Tfilter *filter = (Tfilter *)tmplist->data;
		if (strcmp(filter->name, name)==0) {
			return filter;
		}
		tmplist = g_list_next(tmplist);
	}
	return NULL;
}

static void fb2rpopup_filter_toggled_lcb(GtkWidget *widget, Tfilebrowser2 *fb2) {
	if (GTK_CHECK_MENU_ITEM(widget)->active) {
		/* loop trough the filters for a filter with this name */
		const gchar *name = gtk_label_get_text(GTK_LABEL(GTK_BIN(widget)->child));
		Tfilter *filter = find_filter_by_name(name);
		DEBUG_MSG("fb2rpopup_filter_toggled_lcb, setting curfilter to %p from name %s\n", filter, name);
		fb2->curfilter = filter;
		if (fb2->bfwin->session->last_filefilter) g_free(fb2->bfwin->session->last_filefilter);
		fb2->bfwin->session->last_filefilter = g_strdup(filter->name);
		gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(fb2->dir_tfilter));
		gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(fb2->file_lfilter));
	}
}


static GtkItemFactoryEntry fb2rpopup_menu_entries[] = {
	{ N_("/_Open"),		NULL,	fb2rpopup_rpopup_action_lcb,		1,	"<Item>" },
	{ N_("/Open _Advanced..."),NULL,	fb2rpopup_rpopup_action_lcb,	7,	"<Item>" },
	{ N_("/Rena_me"),		NULL,	fb2rpopup_rpopup_action_lcb,		2,	"<Item>" },
	{ N_("/_Delete"),		NULL,	fb2rpopup_rpopup_action_lcb,		3,	"<Item>" },
	{ "/sep1",						NULL,	NULL,									0,	"<Separator>" },
	{ N_("/New _File"),			NULL,	fb2rpopup_rpopup_action_lcb,	4,	"<Item>" },
	{ N_("/_New Directory"),	NULL,	fb2rpopup_rpopup_action_lcb,	5,	"<Item>" },
	{ "/sep2",						NULL,	NULL,									0,	"<Separator>" },
	{ N_("/_Refresh"),			NULL,	fb2rpopup_rpopup_action_lcb,	6,	"<Item>" },
	{ N_("/Show Full _Tree"),	NULL,	fb2rpopup_rpopup_action_lcb,	9,	"<Item>" },
	{ N_("/_Set as basedir"),	NULL,	fb2rpopup_rpopup_action_lcb,	8,	"<Item>" },
	{ "/sep3",						NULL,	NULL,									0,	"<Separator>" },
	{ N_("/Follow active document"),	NULL,	fb2rpopup_rpopup_action_lcb,	15,	"<ToggleItem>" },
	{ N_("/Show hidden files"),	NULL,	fb2rpopup_rpopup_action_lcb,	16,	"<ToggleItem>" },
	{ N_("/Show backup files"),	NULL,	fb2rpopup_rpopup_action_lcb,	17,	"<ToggleItem>" },
};
/* 
 * what should the menu look like?
 *
 * /Open              (if a file is selected)
 * /Open advanced     (if a dir is selected)
 * /Rename            (if file or dir is selected)
 * /Delete            (if file or dir is selected)
 * /-----
 * /New file
 * /New directory
 * /-----
 * /Refresh
 * /Show full tree
 * /Set as basedir    (if dir is selected)
 *
 * 
 */
static GtkWidget *fb2_rpopup_create_menu(Tfilebrowser2 *fb2, gboolean is_directory, gboolean is_file) {
	GtkWidget *menu, *menu_item;
	GtkItemFactory *menumaker;

	fb2->last_popup_on_dir = is_directory;
	menumaker = gtk_item_factory_new(GTK_TYPE_MENU, "<Filebrowser>", NULL);
#ifdef ENABLE_NLS
	gtk_item_factory_set_translate_func(menumaker,menu_translate,"<Filebrowser>",NULL);
#endif
	gtk_item_factory_create_items(menumaker, sizeof(fb2rpopup_menu_entries)/sizeof(GtkItemFactoryEntry), fb2rpopup_menu_entries, fb2);
	menu = gtk_item_factory_get_widget(menumaker, "<Filebrowser>");

	/* set toggle options */
	setup_toggle_item(menumaker, "/Follow active document", main_v->props.filebrowser_focus_follow);
	setup_toggle_item(menumaker, "/Show hidden files", fb2->bfwin->session->filebrowser_show_hidden_files);
	setup_toggle_item(menumaker, "/Show backup files", fb2->bfwin->session->filebrowser_show_backup_files);
	if (!is_directory && !is_file) {
		gtk_widget_set_sensitive(gtk_item_factory_get_widget(menumaker, "/Rename"), FALSE);
		gtk_widget_set_sensitive(gtk_item_factory_get_widget(menumaker, "/Delete"), FALSE);
	}
	if (!is_directory) {
		gtk_widget_set_sensitive(gtk_item_factory_get_widget(menumaker, "/Open Advanced..."), FALSE);
		gtk_widget_set_sensitive(gtk_item_factory_get_widget(menumaker, "/Set as basedir"), FALSE);
	} 
	if (!is_file) {
		gtk_widget_set_sensitive(gtk_item_factory_get_widget(menumaker, "/Open"), FALSE);
	}
	if (fb2->basedir == NULL) {
		gtk_widget_set_sensitive(gtk_item_factory_get_widget(menumaker, "/Show Full Tree"), FALSE);
	}
	
	/* Add filter submenu */
	menu_item = gtk_menu_item_new_with_label(_("Filter"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	{
		GtkWidget *fmenu;
		GSList *group=NULL;
		GList *tmplist;
		fmenu = gtk_menu_new();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), fmenu);
		tmplist = g_list_last(FILEBROWSER2CONFIG(main_v->fb2config)->filters);
		while (tmplist) {
			Tfilter *filter = (Tfilter *)tmplist->data;
			menu_item = gtk_radio_menu_item_new_with_label(group, filter->name);
			if (fb2->curfilter == filter) {
				gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(menu_item), TRUE);
			}
			g_signal_connect(GTK_OBJECT(menu_item), "toggled", G_CALLBACK(fb2rpopup_filter_toggled_lcb), fb2);
			if (!group) {
				group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menu_item));
			}
			gtk_menu_shell_append(GTK_MENU_SHELL(fmenu), menu_item);
			tmplist = g_list_previous(tmplist);
		}
	}

	gtk_widget_show_all(menu);
	g_signal_connect_after(G_OBJECT(menu), "destroy", G_CALLBACK(destroy_disposable_menu_cb), menu);
	return menu;
}

static void dir_v_row_expanded_lcb(GtkTreeView *tree,GtkTreeIter *sort_iter,GtkTreePath *sort_path,Tfilebrowser2 *fb2) {
	GnomeVFSURI *uri;
	DEBUG_MSG("dir_v_row_expanded_lcb, called for fb2=%p with sort_path=\n",fb2);
	/* refresh the directory that is being expanded */
	gtk_tree_model_get(fb2->dir_tsort, sort_iter, URI_COLUMN, &uri, -1);
	if (uri) {
		fb2_refresh_dir(uri, NULL);
	}
}

static void dir_v_row_activated_lcb(GtkTreeView *tree, GtkTreePath *path,GtkTreeViewColumn *column, Tfilebrowser2 *fb2) {
	DEBUG_MSG("dir_v_row_activated_lcb, called\n");
}

static gboolean dir_v_button_press_lcb(GtkWidget *widget, GdkEventButton *event, Tfilebrowser2 *fb2) {
	DEBUG_MSG("dir_v_button_press_lcb, called for fb2=%p\n",fb2);
	if (event->button == 3) {
		GtkWidget *menu = NULL;
		GtkTreePath *path;
		gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(fb2->dir_v), event->x, event->y, &path, NULL, NULL, NULL);
		if (path) {
			gboolean is_dir = fb2_isdir_from_dir_sort_path(fb2, path);
			menu = fb2_rpopup_create_menu(fb2, is_dir, !is_dir);
			gtk_tree_path_free(path);
		} else {
			menu = fb2_rpopup_create_menu(fb2, FALSE, FALSE);
		}
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);		
	}
	return FALSE; /* pass the event on */
}

static gboolean file_v_button_press_lcb(GtkWidget *widget, GdkEventButton *event, Tfilebrowser2 *fb2) {
	DEBUG_MSG("file_v_button_press_lcb, called\n");
	if (event->button == 3) {
		GtkWidget *menu = NULL;
		GtkTreePath *path;
		gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(fb2->file_v), event->x, event->y, &path, NULL, NULL, NULL);
		if (path) {
			menu = fb2_rpopup_create_menu(fb2, FALSE, TRUE);
			gtk_tree_path_free(path);
		} else {
			menu = fb2_rpopup_create_menu(fb2, FALSE, FALSE);
		}
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
	}
	
	if (event->button==1 && event->type == GDK_2BUTTON_PRESS) {
		GtkTreePath *sort_path;
		gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(fb2->file_v), event->x, event->y, &sort_path, NULL, NULL, NULL);
		if (sort_path) {
			GnomeVFSURI *uri;
			uri = fb2_uri_from_file_sort_path(fb2,sort_path);
			if (uri) {
				DEBUG_MSG("file_v_button_press_lcb, doucleclick on %s\n", gnome_vfs_uri_extract_short_name(uri));
				handle_activate_on_file(fb2, uri);
			}
#ifdef DEBUG
			else {
				DEBUG_MSG("file_v_button_press_lcb, WARNING, NO URI FOUND, cannot activate NULL\n");
			}
#endif			
			gtk_tree_path_free(sort_path);
		}
	}
	return FALSE; /* pass the event on */
}

static void dirmenu_set_curdir(Tfilebrowser2 *fb2, GnomeVFSURI *newcurdir) {
	/* first we remove all non-histroy items */
	GtkTreeIter iter, setiter;
	GnomeVFSURI *tmp;
	gboolean cont, havesetiter=FALSE;
	if (fb2->currentdir) {
		if (gnome_vfs_uri_equal(fb2->currentdir, newcurdir)) return;
		DEBUG_MSG("dirmenu_set_curdir, old_curdir=%s, new_curdir=%s\n",gnome_vfs_uri_get_path(fb2->currentdir),gnome_vfs_uri_get_path(newcurdir));
		gnome_vfs_uri_unref(fb2->currentdir);
	}
	fb2->currentdir = newcurdir;
	gnome_vfs_uri_ref(fb2->currentdir);
	/* block the signal handler */
	g_signal_handler_block(fb2->dirmenu_v, fb2->dirmenu_changed_signal);
	cont = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(fb2->dirmenu_m), &iter);
	while (cont) {
		gboolean inhis;
		gtk_tree_model_get(GTK_TREE_MODEL(fb2->dirmenu_m), &iter, DIR_IN_HISTORY_COLUMN, &inhis, -1);
		if (!inhis) {
			/*GtkTreeIter iter2;*/
			GnomeVFSURI *uri;
			gchar *name;
			/*iter2 = iter;*/
			/*cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(fb2->dirmenu_m), &iter);*/
			/* now remove iter2 */
			gtk_tree_model_get(GTK_TREE_MODEL(fb2->dirmenu_m), &iter, DIR_NAME_COLUMN, &name, DIR_URI_COLUMN, &uri, -1);
			DEBUG_MSG("dirmenu_set_curdir, will remove inhis %d for name %s and uri %s, cont=%d\n",inhis,name,gnome_vfs_uri_get_path(uri),cont);
			g_free(name);
			gnome_vfs_uri_unref(uri);
			/* hmm if this last remove results in an empty listtore there is a crash?? */
			cont = gtk_list_store_remove(GTK_LIST_STORE(fb2->dirmenu_m),&iter);
		} else {
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(fb2->dirmenu_m), &iter);
			DEBUG_MSG("dirmenu_set_curdir, inhis=%d, cont=%d\n",inhis,cont);
		}
	}
	/* then we rebuild the current uri */
	tmp = gnome_vfs_uri_dup(newcurdir);
	cont = gnome_vfs_uri_has_parent(tmp);
	while (cont) {
		gchar *name = uri_to_document_filename(tmp);
		DEBUG_MSG("dirmenu_set_curdir, appending %s\n",name);
		gtk_list_store_append(GTK_LIST_STORE(fb2->dirmenu_m),&iter);
		if (!havesetiter) {
			setiter = iter;
			havesetiter = TRUE;
		}
		gtk_list_store_set(GTK_LIST_STORE(fb2->dirmenu_m),&iter
					,DIR_NAME_COLUMN,name
					,DIR_IN_HISTORY_COLUMN,FALSE
					,DIR_URI_COLUMN,tmp /* don't unref tmp at this point, because there is a reference from the model */
					,-1);
		cont = gnome_vfs_uri_has_parent(tmp);
		if (cont) {
			tmp = gnome_vfs_uri_get_parent(tmp);
		}
	}
	gtk_combo_box_set_active_iter(GTK_COMBO_BOX(fb2->dirmenu_v),&setiter);
	g_signal_handler_unblock(fb2->dirmenu_v, fb2->dirmenu_changed_signal);
}

static void dir_v_selection_changed_lcb(GtkTreeSelection *treeselection,Tfilebrowser2 *fb2) {
	GtkTreeModel *sort_model = NULL;
	GtkTreeIter sort_iter;
	/* Get the current selected row and the model. */
	DEBUG_MSG("dir_v_selection_changed_lcb, treeselection=%p, fb2=%p\n",treeselection,fb2);
	if (treeselection && gtk_tree_selection_get_selected(treeselection,&sort_model,&sort_iter)) {
		GnomeVFSURI *uri;
		gtk_tree_model_get(sort_model, &sort_iter, URI_COLUMN, &uri, -1);
		if (uri) {
			dirmenu_set_curdir(fb2, uri);
			fb2_focus_dir(fb2, uri, TRUE);
		}
/*		GtkTreePath *sort_path;
#ifdef DEBUG11
		DEBUG_CHILD_ITERS(fb2, &sort_iter);
#endif
		sort_path = gtk_tree_model_get_path(sort_model,&sort_iter);
		if (sort_path) {
			GnomeVFSURI *uri;
			uri = fb2_uri_from_dir_sort_path(fb2,sort_path);
			/ *fb2_refresh_dir(fb2, uri, NULL);* /
			fb2_focus_dir(fb2, uri, TRUE);
			gtk_tree_path_free(sort_path);
		}*/
	}
}
/**
 * fb2_set_basedir:
 * bfwin: Tbfwin *
 * curi: gchar * or NULL
 *
 *
 *
 */
void fb2_set_basedir(Tbfwin *bfwin, gchar *curi) {
	if (bfwin->fb2) {
		Tfilebrowser2 *fb2 = bfwin->fb2;
		if (curi) {
			GnomeVFSURI *uri;
			DEBUG_MSG("fb2_set_basedir, set uri=%s\n",curi);
			uri = gnome_vfs_uri_new(strip_trailing_slash(curi));
			fb2_build_dir(uri);
			{
				GtkTreePath *basedir = fb2_fspath_from_uri(fb2, uri);
				refilter_dirlist(fb2, basedir);
				gtk_tree_path_free(basedir);
			}
			fb2_focus_dir(fb2, fb2->basedir, FALSE);
		} else {
			refilter_dirlist(fb2, NULL);
		}
	}
}

static void dirmenu_changed_lcb(GtkComboBox *widget,gpointer data) {
	Tfilebrowser2 *fb2 = data;
	GtkTreeIter iter;
	DEBUG_MSG("dirmenu_changed_lcb, started\n");
	if (gtk_combo_box_get_active_iter(widget,&iter)) {
		GnomeVFSURI *uri;
		DEBUG_MSG("dirmenu_changed_lcb. we have an active iter\n");
		gtk_tree_model_get(GTK_TREE_MODEL(fb2->dirmenu_m), &iter, DIR_URI_COLUMN, &uri, -1);
		DEBUG_MSG("dirmenu_changed_lcb. active iter has url %s\n",gnome_vfs_uri_get_path(uri));
		g_signal_handler_block(fb2->dirmenu_v, fb2->dirmenu_changed_signal);
		fb2_focus_dir(FILEBROWSER2(fb2), uri, FALSE);
		g_signal_handler_unblock(fb2->dirmenu_v, fb2->dirmenu_changed_signal);
	}
}

GtkWidget *fb2_init(Tbfwin *bfwin) {
	Tfilebrowser2 *fb2;
	GtkWidget *vbox;
	GtkCellRenderer *renderer;	

	fb2 = g_new0(Tfilebrowser2,1);
	bfwin->fb2 = fb2;
	fb2->bfwin = bfwin;

	vbox = gtk_vbox_new(FALSE, 0);

	fb2->dirmenu_m = GTK_TREE_MODEL(gtk_list_store_new(3,G_TYPE_STRING,G_TYPE_BOOLEAN,G_TYPE_POINTER));
	fb2->dirmenu_v = gtk_combo_box_new_with_model(fb2->dirmenu_m);
	/*gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(fb2->dirmenu_v),3);*/
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(fb2->dirmenu_v),renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(fb2->dirmenu_v), renderer
			, "text", DIR_NAME_COLUMN
			, NULL);
	gtk_box_pack_start(GTK_BOX(vbox),fb2->dirmenu_v, FALSE, FALSE, 0);
	{
		GList *tmplist;
		tmplist = g_list_first(bfwin->session->recent_dirs);
		while (tmplist) {
			GtkTreeIter iter;
			GnomeVFSURI *uri;
			gchar *name;
			uri = gnome_vfs_uri_new(tmplist->data);
			name = uri_to_document_filename(uri);
			gtk_list_store_append(GTK_LIST_STORE(fb2->dirmenu_m),&iter);
			DEBUG_MSG("fb2_init, adding %s to dir menu\n",(gchar *)tmplist->data);
			gtk_list_store_set(GTK_LIST_STORE(fb2->dirmenu_m),&iter
					,DIR_NAME_COLUMN,name
					,DIR_IN_HISTORY_COLUMN,TRUE
					,DIR_URI_COLUMN	,uri
					,-1);
			tmplist = g_list_next(tmplist);
		}
	}
	fb2->dirmenu_changed_signal = g_signal_connect(fb2->dirmenu_v, "changed", G_CALLBACK(dirmenu_changed_lcb), fb2);

	{
		GtkWidget *scrolwin;
		GtkTreeViewColumn *column;
		GtkTreeSelection* dirselection;
		GtkWidget *vpaned;
		vpaned = gtk_vpaned_new();
		gtk_widget_set_size_request(vpaned, main_v->props.left_panel_width, -1);
		gtk_paned_set_position(GTK_PANED(vpaned), main_v->globses.two_pane_filebrowser_height);
		
		fb2->dir_tfilter = gtk_tree_model_filter_new(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),NULL);
		gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(fb2->dir_tfilter),tree_model_filter_func,fb2,NULL);

		fb2->dir_tsort = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(fb2->dir_tfilter));
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fb2->dir_tsort),FILENAME_COLUMN,GTK_SORT_ASCENDING);
		
		fb2->dir_v = gtk_tree_view_new_with_model(fb2->dir_tsort);
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(fb2->dir_v), FALSE);
		dirselection = gtk_tree_view_get_selection(GTK_TREE_VIEW(fb2->dir_v));
		DEBUG_MSG("fb2_init, NEW FILEBROWSER2, bfwin=%p, treeselection=%p, fb2=%p\n",bfwin,dirselection,fb2);
		g_signal_connect(G_OBJECT(dirselection), "changed", G_CALLBACK(dir_v_selection_changed_lcb), fb2);
		
		renderer = gtk_cell_renderer_pixbuf_new();
		column = gtk_tree_view_column_new();
		gtk_tree_view_column_pack_start(column, renderer, FALSE);
		gtk_tree_view_column_set_attributes(column,renderer
			,"pixbuf",PIXMAP_COLUMN
			,"pixbuf_expander_closed",PIXMAP_COLUMN
			,"pixbuf_expander_open",PIXMAP_COLUMN
			,NULL);
		renderer = gtk_cell_renderer_text_new();
		g_object_set(G_OBJECT(renderer), "editable", FALSE, NULL); /* Not editable. */
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_set_attributes(column,renderer,"text", FILENAME_COLUMN,NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(fb2->dir_v),column);
		scrolwin = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_container_add(GTK_CONTAINER(scrolwin), fb2->dir_v);
		gtk_paned_add1(GTK_PANED(vpaned), scrolwin);

		fb2->file_lfilter = gtk_tree_model_filter_new(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),NULL);
		gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(fb2->file_lfilter),file_list_filter_func,fb2,NULL);

		fb2->file_lsort = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(fb2->file_lfilter));
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fb2->file_lsort),FILENAME_COLUMN,GTK_SORT_ASCENDING);

		fb2->file_v = gtk_tree_view_new_with_model(fb2->file_lsort);
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(fb2->file_v), FALSE);
		renderer = gtk_cell_renderer_pixbuf_new();
		column = gtk_tree_view_column_new();
		gtk_tree_view_column_pack_start(column, renderer, FALSE);
		gtk_tree_view_column_set_attributes(column,renderer
			,"pixbuf",PIXMAP_COLUMN
			,"pixbuf_expander_closed",PIXMAP_COLUMN
			,"pixbuf_expander_open",PIXMAP_COLUMN
			,NULL);
		renderer = gtk_cell_renderer_text_new();
		g_object_set(G_OBJECT(renderer), "editable", FALSE, NULL); /* Not editable. */
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_set_attributes(column,renderer,"text", FILENAME_COLUMN,NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(fb2->file_v), column);
		scrolwin = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_container_add(GTK_CONTAINER(scrolwin), fb2->file_v);
		gtk_paned_add2(GTK_PANED(vpaned), scrolwin);

		gtk_box_pack_start(GTK_BOX(vbox), vpaned, TRUE, TRUE, 0);
	}

	fb2->expand_signal = g_signal_connect(G_OBJECT(fb2->dir_v), "row-expanded", G_CALLBACK(dir_v_row_expanded_lcb), fb2);
	g_signal_connect(G_OBJECT(fb2->dir_v), "row-activated",G_CALLBACK(dir_v_row_activated_lcb),fb2);
	g_signal_connect(G_OBJECT(fb2->dir_v), "button_press_event",G_CALLBACK(dir_v_button_press_lcb),fb2);
	g_signal_connect(G_OBJECT(fb2->file_v), "button_press_event",G_CALLBACK(file_v_button_press_lcb),fb2);

	gtk_widget_show_all(vbox);	
	{
		GnomeVFSURI *uri = NULL;
		if (bfwin->project && bfwin->project->basedir && strlen(bfwin->project->basedir)>2) {
			uri = gnome_vfs_uri_new(strip_trailing_slash(bfwin->project->basedir));
			fb2_build_dir(uri);
		} else if (main_v->props.default_basedir && strlen(main_v->props.default_basedir)>2) {
			uri = gnome_vfs_uri_new(strip_trailing_slash(main_v->props.default_basedir));
		}
		if (uri) {
			DEBUG_MSG("fb2_init, first build, and then set the basedir to ");
			DEBUG_URI(uri, TRUE);
			fb2_build_dir(uri);
			{
				GtkTreePath *basedir = fb2_fspath_from_uri(fb2, uri);
				DEBUG_MSG("fb2_init, will set the basedir to path(%p) ",basedir);
				DEBUG_TPATH(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore), basedir, TRUE);
				refilter_dirlist(fb2, basedir);
				gtk_tree_path_free(basedir);
			}
			DEBUG_MSG("fb2_init, focus fb2->basedir ");
			DEBUG_URI(fb2->basedir, TRUE);
			fb2_focus_dir(fb2, fb2->basedir, FALSE);
		}
	}
	return vbox;
}

void fb2_cleanup(Tbfwin *bfwin) {
	Tfilebrowser2 *fb2 = FILEBROWSER2(bfwin->fb2);
	if (fb2->basedir) gnome_vfs_uri_unref(fb2->basedir);
	g_object_unref(G_OBJECT(fb2->dir_tfilter));
	g_object_unref(G_OBJECT(fb2->dir_tsort));
	g_object_unref(G_OBJECT(fb2->file_lfilter));
	g_object_unref(G_OBJECT(fb2->file_lsort));
	g_free(fb2);
	bfwin->fb2 = NULL;
}

static Tfilter *new_filter(gchar *name, gchar *mode, gchar *filetypes) {
	Tfilter *filter = g_new(Tfilter,1);
	filter->name = g_strdup(name);
	filter->mode = atoi(mode);
	filter->filetypes = NULL;
	if (filetypes){
		GList *tmplist;
		gchar **types = g_strsplit(filetypes, ":", 127);
		
		tmplist = g_list_first(main_v->filetypelist);
		while (tmplist) {
			Tfiletype *filetype = (Tfiletype *)tmplist->data;
			gchar **type = types;
			while (*type) {
				if (strcmp(*type, filetype->type)==0) {
					filter->filetypes = g_list_prepend(filter->filetypes, filetype);
					break;
				}
				type++;
			}
			tmplist = g_list_next(tmplist);
		}
		g_strfreev(types);
	}
	return filter;
}

static void filter_destroy(Tfilter *filter) {
	g_free(filter->name);
	g_list_free(filter->filetypes);
	g_free(filter);
}

void fb2_filters_rebuild(void) {
	GList *tmplist;
	/* free any existing filters */
	tmplist = g_list_first(FILEBROWSER2CONFIG(main_v->fb2config)->filters);
	while (tmplist) {
		filter_destroy(tmplist->data);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(FILEBROWSER2CONFIG(main_v->fb2config)->filters);
	FILEBROWSER2CONFIG(main_v->fb2config)->filters = NULL;
	g_list_free(FILEBROWSER2CONFIG(main_v->fb2config)->filetypes_with_icon);
	FILEBROWSER2CONFIG(main_v->fb2config)->filetypes_with_icon = NULL;
	
	/* build a list of filetypes with icon */
	tmplist = g_list_first(main_v->filetypelist);
	while (tmplist) {
		if (((Tfiletype *)tmplist->data)->icon) {
			FILEBROWSER2CONFIG(main_v->fb2config)->filetypes_with_icon = g_list_prepend(FILEBROWSER2CONFIG(main_v->fb2config)->filetypes_with_icon, tmplist->data);
		}
		tmplist = g_list_next(tmplist);
	}
	/* build a list of filters */
	FILEBROWSER2CONFIG(main_v->fb2config)->filters = g_list_prepend(NULL, new_filter(_("All files"), "0", NULL));
	tmplist = g_list_first(main_v->props.filefilters);
	while (tmplist) {
		gchar **strarr = (gchar **) tmplist->data;
		if (count_array(strarr) == 3) {
			Tfilter *filter = new_filter(strarr[0], strarr[1], strarr[2]);
			FILEBROWSER2CONFIG(main_v->fb2config)->filters = g_list_prepend(FILEBROWSER2CONFIG(main_v->fb2config)->filters, filter);
		}
		tmplist = g_list_next(tmplist);
	}
}

void fb2config_init(void) {
	Tfilebrowser2config *fb2config;
	gchar *filename;
	DEBUG_MSG("fb2config_init, started\n");
	/* a lot of things can be the same for all windows, such as the hashtable and the treestore with
	all the files, and the pixmaps to use. All stored in Tfilebrowser2config;
	This will be initialized in this function */
	fb2config = g_new0(Tfilebrowser2config,1);
	main_v->fb2config = fb2config;

	fb2config->filesystem_itable = g_hash_table_new_full(g_int_hash,g_int_equal,g_free,g_free);
	fb2config->filesystem_tstore = gtk_tree_store_new(N_COLUMNS,GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_POINTER,G_TYPE_BOOLEAN,G_TYPE_INT);
	filename = return_first_existing_filename(main_v->props.filebrowser_unknown_icon,
					"icon_unknown.png","../icons/icon_unknown.png",
					"icons/icon_unknown.png",NULL);
	fb2config->unknown_icon = gdk_pixbuf_new_from_file(filename, NULL);
	g_free(filename);
	filename = return_first_existing_filename(main_v->props.filebrowser_dir_icon,
					"icon_dir.png","../icons/icon_dir.png",
					"icons/icon_dir.png",NULL);
	fb2config->dir_icon = gdk_pixbuf_new_from_file(filename, NULL);
	g_free(filename);

	if (main_v->props.default_basedir && strlen(main_v->props.default_basedir) > 2) {
		GtkTreeIter *iter;
		GnomeVFSURI *uri;
		uri = gnome_vfs_uri_new(strip_trailing_slash(main_v->props.default_basedir));
		iter = fb2_build_dir(uri);
		gnome_vfs_uri_unref(uri);
	}
	fb2_filters_rebuild();	
	DEBUG_MSG("fb2config_init, finished\n");
}
