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

/* ******* NEW FILEBROWSER DESIGN ********
I'm thinking about a new design for the filebrowser, the 
code is too complicated right now. For now I'll just write some thoughts, so 
after we have the next stable releae we can implement it.

we'll drop the one-pane-view, we'll only have the files and directories separate

the public API:
--------------
GtkWidget *fb2_init(Tbfwin *bfwin);
void fb2_cleanup(Tbfwin *bfwin);
void fb2_set_basedir(Tbfwin *bfwin, gchar *basedir);
void fb2_focus_document(Tbfwin *bfwin, Tdocument *doc);
--------------

in the treemodel for the directories, we'll have two columns. A visible column (with the name), 
and a column with the full path,  so if a click on an item is done, it is very easy to see which 
full path corresponds to that item.

to make 'focus document' very easy, each document can have a GtkTreeIter 
pointing to the directory where the file located. If it does not have the treeiter, the 
document has not been focused before (so it is not required to have a treeiter for 
every document).

the most difficult thing to code now is when we for example open a new file, we'll have to 
find which directory item corresponds to that filename, and if it is not yet there, we'll 
have to find where we should add it. Same for a document that has not been focused before.
A possibility to do this is to have a hashtable with TreeIters to each directory that is already 
in the tree. If you then want to open some directory, you check the full path in the hashtable, 
if not exists you remove the last directory component, check again etc. until you have found a
position in the tree where you can add things.

USER INTERFACE NOTES:

each directory should be expandable by default (so have a dummy item) unless we know there are 
no subdirectories (and not the other way around like it is right now)

*/

#include <gtk/gtk.h>
#include <string.h>
#include "bluefish.h"
#include "bf_lib.h"
typedef struct {
	GtkTreeStore *filesystem_tstore; /* the directory tree */	
	GHashTable *filesystem_itable; /* iter to known files and directories */
	GdkPixbuf *unknown_icon;
	GdkPixbuf *dir_icon;
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
#define TYPE_HIDDEN -1
#define TYPE_HIDDEN_DIR -2
#define TYPE_DIR -3
#define TYPE_BACKUP -4
#define TYPE_FILE -5

typedef struct {
	GtkWidget *dirmenu;
	GtkWidget *dir_v; /* treeview widget */
	GtkWidget *file_v; /* listview widget */
	
	GnomeVFSURI *basedir;

	GtkTreeModel *dir_tfilter;
	GtkTreeModel *dir_tsort;

	GtkTreeModel *file_lfilter;
	GtkTreeModel *file_lsort;

	Tbfwin *bfwin;
} Tfilebrowser2;

#define FILEBROWSER2(var) ((Tfilebrowser2 *)(var))
static void refilter_filelist(Tfilebrowser2 *fb2, GtkTreePath *newroot);
static void fb2_fill_dir_async(Tfilebrowser2 *fb2, GtkTreeIter *parent, GnomeVFSURI *uri, gboolean load_subdirs);
/**************/
static void DEBUG_DIRITER(Tfilebrowser2 *fb2, GtkTreeIter *diriter) {
	gchar *name;
	gtk_tree_model_get(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore), diriter, FILENAME_COLUMN, &name, -1);
	g_print("DEBUG_DIRITER, iter(%p) has filename %s\n",diriter,name);
}


/**************/

static GtkTreeIter *fb2_add_filesystem_entry(Tfilebrowser2 *fb2, GtkTreeIter *parent, GnomeVFSURI *child_uri, gint type, gboolean load_subdirs) {
	GtkTreeIter *newiter;
	guint *hashkey;
	
	hashkey = g_new(guint,1);
	*hashkey = gnome_vfs_uri_hash(child_uri);
	newiter = g_hash_table_lookup(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_itable, hashkey);
	if (newiter != NULL) {
		/* the child exists already, update the REFRESH column */
		gtk_tree_store_set(GTK_TREE_STORE(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),newiter,REFRESH_COLUMN, 0,-1);
		g_free(hashkey);
	} else {
		gpointer pixmap = FILEBROWSER2CONFIG(main_v->fb2config)->dir_icon;
		if (type != TYPE_DIR) {
			pixmap = FILEBROWSER2CONFIG(main_v->fb2config)->unknown_icon;
		}
		newiter = g_new(GtkTreeIter,1);
		gtk_tree_store_append(GTK_TREE_STORE(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),newiter,parent);
		DEBUG_MSG("fb2_add_filesystem_entry, will add %s\n",gnome_vfs_uri_extract_short_name(child_uri));
		gtk_tree_store_set(GTK_TREE_STORE(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),newiter,
				PIXMAP_COLUMN, pixmap,
				FILENAME_COLUMN, gnome_vfs_uri_extract_short_name(child_uri),
				URI_COLUMN, child_uri,
				REFRESH_COLUMN, 0,
				TYPE_COLUMN, type,
				-1);
		DEBUG_MSG("fb2_add_filesystem_entry adding iter %p to hashkey %u\n",newiter,*hashkey);
		g_hash_table_insert(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_itable,hashkey,newiter);
		if (load_subdirs && type == TYPE_DIR) {
			/* load this directory, if it has subdirectories the expander will be shown after they are added */
			fb2_fill_dir_async(fb2, newiter, child_uri, FALSE);
		}
	}
	return newiter;
}

typedef struct {
	GtkTreeIter *parent;
	GnomeVFSURI *p_uri;
	Tfilebrowser2 *fb2;
	gboolean load_subdirs;
} Tdirectoryloaddata;

static void fb2_treestore_delete_children_refresh1(Tfilebrowser2 *fb2, GtkTreeStore *tstore, GtkTreeIter *iter) {
	GtkTreeIter child;
	if (gtk_tree_model_iter_children(GTK_TREE_MODEL(tstore),&child,iter)) {
		gboolean cont=TRUE;
		while (cont) {
			gboolean refresh;
			GtkTreeIter this = child;
			gtk_tree_model_get(GTK_TREE_MODEL(tstore), &child, REFRESH_COLUMN, &refresh, -1);
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(tstore), &child);
			if (refresh) {
				GnomeVFSURI *d_uri;
				gchar *d_name;
				guint *hashkey;
				hashkey = g_new(guint,1);
				/* delete 'this' ! */
				gtk_tree_model_get(GTK_TREE_MODEL(tstore), &this, FILENAME_COLUMN, &d_name, URI_COLUMN, &d_uri -1);
				gtk_tree_store_remove(tstore,&this);
				*hashkey = gnome_vfs_uri_hash(d_uri);
				g_hash_table_remove(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_itable, hashkey);
				g_free(hashkey);
				gnome_vfs_uri_unref(d_uri);
				g_free(d_name);
			}
		}
	}
}

static void fb2_load_directory_lcb(GnomeVFSAsyncHandle *handle,GnomeVFSResult result,GList *list,guint entries_read,Tdirectoryloaddata *cdata) {
	GList *tmplist;
	DEBUG_MSG("cdata=%p\n",cdata);
	DEBUG_MSG("fb2_load_directory_lcb, appending %d childs to %s\n",entries_read,gnome_vfs_uri_get_path(cdata->p_uri));
	tmplist = g_list_first(list);
	while (tmplist) {
		GnomeVFSFileInfo *finfo = tmplist->data;
		GnomeVFSURI *child_uri = gnome_vfs_uri_append_string(cdata->p_uri,finfo->name);
		if (strcmp(finfo->name,".")!=0 && strcmp(finfo->name,"..")!=0) {
			gint type;
			DEBUG_MSG("found %s with type %d\n", finfo->name, finfo->type);
			if (finfo->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
				type = TYPE_DIR;
			} else {
				type = TYPE_FILE;
			}
			fb2_add_filesystem_entry(cdata->fb2, cdata->parent, child_uri, type, cdata->load_subdirs);
		}
		tmplist = g_list_next(tmplist);
	}

	if (result == GNOME_VFS_ERROR_EOF) {
		DEBUG_MSG("fb2_load_directory_lcb, EOF, list=%p\n",list);
		fb2_treestore_delete_children_refresh1(cdata->fb2, FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore, cdata->parent);
		g_free(cdata);
	}
}

static void fb2_fill_dir_async(Tfilebrowser2 *fb2, GtkTreeIter *parent, GnomeVFSURI *uri, gboolean load_subdirs) {
	GnomeVFSAsyncHandle *handle;
	Tdirectoryloaddata *cdata;
	gint priority = GNOME_VFS_PRIORITY_DEFAULT;
	
	cdata = g_new(Tdirectoryloaddata,1);
	cdata->fb2 = fb2;
	cdata->parent = parent;
	cdata->p_uri = uri;
	cdata->load_subdirs = load_subdirs;
	DEBUG_MSG("fb2_fill_dir_async, opening %s\n",gnome_vfs_uri_get_path(uri));	
	if (!load_subdirs) {
		priority = GNOME_VFS_PRIORITY_MIN;
	}
	gnome_vfs_async_load_directory_uri(&handle,uri,GNOME_VFS_FILE_INFO_DEFAULT,
								10,priority,fb2_load_directory_lcb,cdata);
}

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

static GnomeVFSURI *fb2_uri_from_iter(GtkTreeIter *iter) {
	GnomeVFSURI *uri;
	gtk_tree_model_get(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore), iter, URI_COLUMN, &uri, -1);
	return uri;
}

/*
the 'dir' iter that is passed to this function will be used in the asynchronous
update, so it needs to be a persistent iter. The iters that are in the hashtable 
are persistent. Most others are not. So better pass NULL then pass a temporary 
iter
*/
static void fb2_refresh_dir(Tfilebrowser2 *fb2, GnomeVFSURI *uri, GtkTreeIter *dir) {
	/* first we mark all children as 'refresh=1' in the REFRESH_COLUMN, then
	we read the directory, and all existing items are set to refresh=0, after we
	have finished reading the directory, all items that still have refresh=1 can be 
	deleted */
	if (uri != NULL && dir == NULL) {
		guint hashkey;
		hashkey = gnome_vfs_uri_hash(uri);
		dir = g_hash_table_lookup(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_itable, &hashkey);
	}
	if (dir != NULL && uri == NULL) {
		uri = fb2_uri_from_iter(dir);
	}
	DEBUG_MSG("fb2_refresh_dir, uri is %s\n", gnome_vfs_uri_get_path(uri));
	if (dir && uri) {
		fb2_treestore_mark_children_refresh1(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore, dir);
		fb2_fill_dir_async(fb2, dir, uri, TRUE);
	}
}

static GtkTreeIter *fb2_build_dir(Tfilebrowser2 *fb2, GnomeVFSURI *uri) {
	GnomeVFSURI* tmp, *parent_uri;
	GtkTreeIter *parent = NULL;

	tmp = gnome_vfs_uri_dup(uri);
	while (!parent && gnome_vfs_uri_has_parent(tmp)) {
		guint hashkey;
		GnomeVFSURI* tmp2 = gnome_vfs_uri_get_parent(tmp);
		gnome_vfs_uri_unref(tmp);
		tmp = tmp2;
		hashkey = gnome_vfs_uri_hash(tmp);
		DEBUG_MSG("fb2_build_dir, hashkey for %s is %u\n",gnome_vfs_uri_get_path(tmp), hashkey);
		parent = g_hash_table_lookup(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_itable, &hashkey);
		DEBUG_MSG("fb2_build_dir, found parent=%p for hashkey=%u\n",parent,hashkey);
	}
	if (!parent) {
		DEBUG_MSG("adding toplevel %s\n",gnome_vfs_uri_get_path(tmp));
		parent = fb2_add_filesystem_entry(fb2, NULL, tmp, TYPE_DIR, FALSE);
		parent_uri = tmp;
	} else {
		parent_uri = tmp;
	}
	DEBUG_MSG("we should have a parent here!! parent(%p), and the uri for that parent is %s\n",parent,gnome_vfs_uri_get_path(parent_uri));
	{
		gboolean done=FALSE;
		
		while (!done) {
			GnomeVFSURI* tmp2 = gnome_vfs_uri_dup(uri);
			while (!gnome_vfs_uri_is_parent(parent_uri,tmp2,FALSE)) {
				GnomeVFSURI* tmp3 = gnome_vfs_uri_get_parent(tmp2);
				gnome_vfs_uri_unref(tmp2);
				tmp2 = tmp3;
			}
			DEBUG_MSG("%s is the parent of %s\n",gnome_vfs_uri_get_path(parent_uri),gnome_vfs_uri_get_path(tmp2));
			parent = fb2_add_filesystem_entry(fb2, parent, tmp2, TYPE_DIR, FALSE);
			/* probably needed: gnome_vfs_uri_unref(parent_uri); */
			parent_uri = tmp2;
			DEBUG_MSG("new parent_uri=%s, requested uri=%s\n",gnome_vfs_uri_get_path(parent_uri),gnome_vfs_uri_get_path(uri));
			if (gnome_vfs_uri_equal(parent_uri,uri)) {
				DEBUG_MSG("exit loop, created requested dir %s\n",gnome_vfs_uri_get_path(uri));
				done = TRUE;
			}
		}
	}
	return parent;
}

static void fb2_focus_dir(Tfilebrowser2 *fb2, GnomeVFSURI *path) {
	guint hashkey;
	GtkTreeIter *dir;
	hashkey = gnome_vfs_uri_hash(path);
	dir = g_hash_table_lookup(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_itable, &hashkey);
	DEBUG_MSG("fb2_focus_dir, found %p for hashkey %u\n",dir,hashkey);
	if (!dir) {
		dir = fb2_build_dir(fb2, path);
	}
	if (dir) {
		GtkTreeIter sort_iter, filter_iter;
		GtkTreePath *tpath;
		DEBUG_DIRITER(fb2, dir);
		fb2_refresh_dir(fb2, path, dir);

		/* set this directory as the top tree for the file widget */
		tpath = gtk_tree_model_get_path(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),dir);
		if (tpath) {
			DEBUG_MSG("fb2_focus_dir, set new root for file list..\n");
			refilter_filelist(fb2, tpath);
			DEBUG_MSG("fb2_focus_dir, expand dir tree to this dir..\n");
			gtk_tree_view_expand_to_path(GTK_TREE_VIEW(fb2->dir_v), tpath);
			gtk_tree_path_free(tpath);
		} else {
			DEBUG_MSG("NO TREEPATH FOR THE DIR ITER WE TRY TO FOCUS ?!?!?!\n");
		}
		gtk_tree_model_filter_convert_child_iter_to_iter(GTK_TREE_MODEL_FILTER(fb2->dir_tfilter), &filter_iter,dir);
		gtk_tree_model_sort_convert_child_iter_to_iter(GTK_TREE_MODEL_SORT(fb2->dir_tsort),	&sort_iter,&filter_iter);
		DEBUG_MSG("fb2_focus_dir, try to select iter in dir tree..\n");
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(fb2->dir_v)),&sort_iter);
	}
}

void fb2_focus_document(Tbfwin *bfwin, Tdocument *doc) {
	DEBUG_MSG("fb2_focus_document,doc->filename=%s\n",doc->filename);
	if (bfwin->fb2 && doc->filename) {
		GnomeVFSURI *path, *dir;
		/* first we make sure we have the correct directory open, then
		we could select the document, but if the directory *was* open already, this
		could disturb the user... hmmm... */
		path = gnome_vfs_uri_new(doc->filename);
		dir = gnome_vfs_uri_get_parent(path);
		fb2_focus_dir(FILEBROWSER2(bfwin->fb2), dir);
		gnome_vfs_uri_unref(path);
	}
}

static gboolean tree_model_filter_func(GtkTreeModel *model,GtkTreeIter *iter,gpointer data) {
	gchar *name;
	gint len, type;
	gtk_tree_model_get(GTK_TREE_MODEL(model), iter, FILENAME_COLUMN, &name, TYPE_COLUMN, &type, -1);
	if (type != TYPE_DIR) return FALSE;
	if (name) {
		len = strlen(name);
		return (len < 2 || (name[strlen(name)-1] != '~'));
	}
	return TRUE;
}

static gboolean file_list_filter_func(GtkTreeModel *model,GtkTreeIter *iter,gpointer data) {
	gchar *name;
	gint len, type;
	gtk_tree_model_get(GTK_TREE_MODEL(model), iter, FILENAME_COLUMN, &name, TYPE_COLUMN, &type, -1);
	if (type != TYPE_FILE) return FALSE;
	if (name) {
		len = strlen(name);
		return (len < 2 || (name[strlen(name)-1] != '~'));
	}
	return TRUE;
}

static void refilter_dirlist(Tfilebrowser2 *fb2, GtkTreePath *newroot) {
	GtkTreeModel *oldmodel1, *oldmodel2;
	oldmodel1 = fb2->dir_tfilter;
	oldmodel2 = fb2->dir_tsort;
	fb2->dir_tfilter = gtk_tree_model_filter_new(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),newroot);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(fb2->dir_tfilter),tree_model_filter_func,fb2,NULL);
	fb2->dir_tsort = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(fb2->dir_tfilter));
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fb2->dir_tsort),FILENAME_COLUMN,GTK_SORT_ASCENDING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(fb2->dir_v),GTK_TREE_MODEL(fb2->dir_tsort));
	g_object_unref(oldmodel1);
	g_object_unref(oldmodel2);
}

static void refilter_filelist(Tfilebrowser2 *fb2, GtkTreePath *newroot) {
	GtkTreeModel *oldmodel1, *oldmodel2;
	oldmodel1 = fb2->file_lfilter;
	oldmodel2 = fb2->file_lsort;
	fb2->file_lfilter = gtk_tree_model_filter_new(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),newroot);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(fb2->file_lfilter),file_list_filter_func,fb2,NULL);
	fb2->file_lsort = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(fb2->file_lfilter));
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fb2->file_lsort),FILENAME_COLUMN,GTK_SORT_ASCENDING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(fb2->file_v),GTK_TREE_MODEL(fb2->file_lsort));
	g_object_unref(oldmodel1);
	g_object_unref(oldmodel2);
}

static GnomeVFSURI *fb2_uri_from_dir_sort_path(Tfilebrowser2 *fb2, GtkTreePath *sort_path) {
	if (sort_path) {
		GtkTreePath *filter_path;
		filter_path = gtk_tree_model_sort_convert_path_to_child_path(GTK_TREE_MODEL_SORT(fb2->dir_tsort),sort_path);
		if (filter_path) {
			GtkTreePath *fs_path;
			fs_path = gtk_tree_model_filter_convert_path_to_child_path(GTK_TREE_MODEL_FILTER(fb2->dir_tfilter), filter_path);
			if (fs_path) {
				GtkTreeIter fsiter;
				gtk_tree_model_get_iter(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),&fsiter,fs_path);
				gtk_tree_path_free(fs_path);
				return fb2_uri_from_iter(&fsiter);
			}
			gtk_tree_path_free(filter_path);
		}
	}
	return NULL;
}
static GnomeVFSURI *fb2_uri_from_file_sort_path(Tfilebrowser2 *fb2, GtkTreePath *sort_path) {
	if (sort_path) {
		GtkTreePath *filter_path;
		filter_path = gtk_tree_model_sort_convert_path_to_child_path(GTK_TREE_MODEL_SORT(fb2->file_lsort),sort_path);
		if (filter_path) {
			GtkTreePath *fs_path;
			fs_path = gtk_tree_model_filter_convert_path_to_child_path(GTK_TREE_MODEL_FILTER(fb2->file_lfilter), filter_path);
			if (fs_path) {
				GtkTreeIter fsiter;
				gtk_tree_model_get_iter(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),&fsiter,fs_path);
				gtk_tree_path_free(fs_path);
				return fb2_uri_from_iter(&fsiter);
			}
			gtk_tree_path_free(filter_path);
		}
	}
	return NULL;
}


static void dir_v_row_expanded_lcb(GtkTreeView *tree,GtkTreeIter *iter,GtkTreePath *path,Tfilebrowser2 *fb2) {
	DEBUG_MSG("dir_v_row_expanded_lcb, called\n");
}

static void dir_v_row_activated_lcb(GtkTreeView *tree, GtkTreePath *path,GtkTreeViewColumn *column, Tfilebrowser2 *fb2) {
	DEBUG_MSG("dir_v_row_activated_lcb, called\n");
}

static gboolean dir_v_button_press_lcb(GtkWidget *widget, GdkEventButton *event, Tfilebrowser2 *fb2) {
	DEBUG_MSG("dir_v_button_press_lcb, called\n");
	return FALSE; /* pass the event on */
}

static gboolean file_v_button_press_lcb(GtkWidget *widget, GdkEventButton *event, Tfilebrowser2 *fb2) {
	DEBUG_MSG("file_v_button_press_lcb, called\n");
	if (event->button==1 && event->type == GDK_2BUTTON_PRESS) {
		GtkTreePath *sort_path;
		gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(fb2->file_v), event->x, event->y, &sort_path, NULL, NULL, NULL);
		if (sort_path) {
			GnomeVFSURI *uri;
			uri = fb2_uri_from_file_sort_path(fb2,sort_path);
			DEBUG_MSG("file_v_button_press_lcb, doucleclick on %s\n", gnome_vfs_uri_extract_short_name(uri));
			gtk_tree_path_free(sort_path);
		}
	}

	return FALSE; /* pass the event on */
}

static void dir_v_selection_changed_lcb(GtkTreeSelection *treeselection,Tfilebrowser2 *fb2) {
	GtkTreeModel *sort_model = NULL;
	GtkTreeIter sort_iter;
	/* Get the current selected row and the model. */
	if (gtk_tree_selection_get_selected(treeselection,&sort_model,&sort_iter)) {
		GtkTreePath *sort_path;
		sort_path = gtk_tree_model_get_path(sort_model,&sort_iter);
		if (sort_path) {
			GnomeVFSURI *uri;
			uri = fb2_uri_from_dir_sort_path(fb2,sort_path);
			/*fb2_refresh_dir(fb2, uri, NULL);*/
			fb2_focus_dir(fb2, uri);
			gtk_tree_path_free(sort_path);
		}
	}
}

GtkWidget *fb2_init(Tbfwin *bfwin) {
	Tfilebrowser2 *fb2;
	GtkWidget *vbox;
	
	fb2 = g_new0(Tfilebrowser2,1);
	bfwin->fb2 = fb2;
	fb2->bfwin = bfwin;

	vbox = gtk_vbox_new(FALSE, 0);
	fb2->dirmenu = gtk_option_menu_new();
	gtk_option_menu_set_menu(GTK_OPTION_MENU(fb2->dirmenu), gtk_menu_new());
	gtk_box_pack_start(GTK_BOX(vbox),fb2->dirmenu, FALSE, FALSE, 0);
	{
		GtkWidget *scrolwin;
		GtkCellRenderer *renderer;
		GtkTreeViewColumn *column;
		GtkTreeSelection* dirselection;
		GtkWidget *vpaned;
		vpaned = gtk_vpaned_new();
		
		fb2->dir_tfilter = gtk_tree_model_filter_new(GTK_TREE_MODEL(FILEBROWSER2CONFIG(main_v->fb2config)->filesystem_tstore),NULL);
		gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(fb2->dir_tfilter),tree_model_filter_func,fb2,NULL);
		fb2->dir_tsort = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(fb2->dir_tfilter));
		fb2->dir_v = gtk_tree_view_new_with_model(fb2->dir_tsort);
		dirselection = gtk_tree_view_get_selection(GTK_TREE_VIEW(fb2->dir_v));
		g_signal_connect(G_OBJECT(dirselection), "changed", G_CALLBACK(dir_v_selection_changed_lcb), fb2);
		
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fb2->dir_tsort),FILENAME_COLUMN,GTK_SORT_ASCENDING);
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

	g_signal_connect(G_OBJECT(fb2->dir_v), "row-expanded", G_CALLBACK(dir_v_row_expanded_lcb), fb2);
	g_signal_connect(G_OBJECT(fb2->dir_v), "row-activated",G_CALLBACK(dir_v_row_activated_lcb),fb2);
	g_signal_connect(G_OBJECT(fb2->dir_v), "button_press_event",G_CALLBACK(dir_v_button_press_lcb),fb2);
	g_signal_connect(G_OBJECT(fb2->file_v), "button_press_event",G_CALLBACK(file_v_button_press_lcb),fb2);
	
	if (bfwin->project && bfwin->project->basedir && strlen(bfwin->project->basedir)>2) {
		fb2->basedir = gnome_vfs_uri_new(bfwin->project->basedir);
	} else if (main_v->props.default_basedir && strlen(main_v->props.default_basedir)>2) {
		fb2->basedir = gnome_vfs_uri_new(main_v->props.default_basedir);
	}
	DEBUG_MSG("fb2_init, the basedir is set to %s\n",gnome_vfs_uri_get_path(fb2->basedir));
	gtk_widget_show_all(vbox);
	
	return vbox;
}

void fb2config_init() {
	Tfilebrowser2config *fb2config;
	gchar *filename;
	/* a lot of things can be the same for all windows, such as the hashtable and the treestore with
	all the files, and the pixmaps to use. All stored in Tfilebrowser2config;
	This will be initialized in this function */
	fb2config = g_new(Tfilebrowser2config,1);
	main_v->fb2config = fb2config;

	fb2config->filesystem_itable = g_hash_table_new(g_int_hash,g_int_equal);
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
}
