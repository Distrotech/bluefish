/* Bluefish HTML Editor
 * filebrowser->c the filebrowser
 *
 * Copyright (C) 2002 Olivier Sessink
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
/*#define DEBUG
#define DEBUG_ADDING_TO_TREE*/

#include <gtk/gtk.h>
#include <sys/types.h> /* stat() getuid */
#include <sys/stat.h> /* stat() */
#include <unistd.h> /* stat() getuid */
#include <string.h> /*strchr() */
#include <stdio.h> /* rename() */	
#include <stdlib.h> /* atoi() */

#include "bluefish.h"

#ifdef HAVE_GNOME_VFS
#include <libgnomevfs/gnome-vfs.h>
#endif

#include "document.h"
#include "gtk_easy.h" /* *_dialog(), flush_queue() */
#include "bf_lib.h"
#include "stringlist.h" /* count_array() */
#include "filebrowser.h"
#include "gui.h" /* statusbar_message() */
#include "image.h" /* image_insert_from_filename() */

/* #define DEBUG_SORTING */
/* #define DEBUG_FILTER */
/* #define DEBUG_ADDING_TO_TREE */

#define FILEBROWSER(var) ((Tfilebrowser *)(var))
#define FILEBROWSERCONFIG(var) ((Tfilebrowserconfig *)(var))

enum {
   TYPE_DIR,
   TYPE_FILE
};
enum {
   PIXMAP_COLUMN,
   FILENAME_COLUMN,
   N_COLUMNS
};

typedef struct {
	gchar *name;
	gboolean mode; /* 0= hide matching files, 1=show matching files */
	GList *filetypes; /* if NULL all files are OK */
} Tfilter;

typedef struct {
	gchar *name;
	struct stat stat;
	gint type;
	gboolean has_widget;
	GdkPixbuf *icon;
} Tdir_entry;

typedef struct {
	Tfilter *curfilter;
	GtkWidget *tree;
	GtkTreeStore *store;
	GtkWidget *tree2;
	GtkListStore *store2;
	GtkWidget *dirmenu;
	GList *dirmenu_entries;
	gchar *last_opened_dir;
	Tbfwin *bfwin;
	gchar *basedir;
} Tfilebrowser;

typedef struct {
	GdkPixbuf *unknown_icon;
	GdkPixbuf *dir_icon;
	GList *filters;
	GList *filetypes_with_icon;
} Tfilebrowserconfig;

/* some functions need to be referenced before they are declared*/
static void row_expanded_lcb(GtkTreeView *tree,GtkTreeIter *iter,GtkTreePath *path,Tfilebrowser *filebrowser);

#ifdef DEBUG
void DEBUG_DUMP_TREE_PATH(GtkTreePath *path) {
	if (path) {
		gchar *tmpstr = gtk_tree_path_to_string(path);
		DEBUG_MSG("path='%s'\n", tmpstr);
		g_free(tmpstr);
	} else {
		DEBUG_MSG("path=NULL\n");
	}
}
#else
#define DEBUG_DUMP_TREE_PATH(path)
 /**/
#endif /* DEBUG */


/* MODIFIES THE ITER POINTER TO BY PARENT !!!!!!!! */
static gboolean get_iter_by_filename_from_parent(GtkTreeStore *store, GtkTreeIter *parent, gchar *filename) {
	gboolean valid;
	GtkTreeIter iter;
	iter = *parent;
	valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(store),&iter,parent);
	while (valid) {
		gchar *name;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, FILENAME_COLUMN, &name, -1);
		if (strcmp(name, filename)==0) {
			*parent = iter;
			DEBUG_MSG("get_iter_by_filename_from_parent, found!!\n");
			g_free(name);
			return TRUE;
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
		g_free(name);
	}
	DEBUG_MSG("get_iter_by_filename_from_parent, NOT found..\n");
	return FALSE;
}

static gchar *return_filename_from_path(Tfilebrowser *filebrowser, GtkTreeModel *store,const GtkTreePath *thispath) {
	gboolean valid = TRUE;
	gchar *retval = NULL, *tmp;
	GtkTreePath *path = gtk_tree_path_copy(thispath);

	while (valid) {
		gchar *name = NULL, *tmpstr;
		GtkTreeIter iter;

		gtk_tree_model_get_iter(GTK_TREE_MODEL(store),&iter,path);
		gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, FILENAME_COLUMN, &name, -1);
		tmpstr = gtk_tree_path_to_string(path);
		DEBUG_MSG("path='%s' contains '%s'\n", tmpstr, name);
		g_free(tmpstr);
		if (filebrowser->basedir && strcmp(name, filebrowser->basedir) == 0) {
			tmp = retval;
			retval = g_strconcat(name, retval,NULL);
			g_free(tmp);
			DEBUG_MSG("return_filename_from_path, found the root (%s ?), retval=%s\n", name,retval);
			valid = FALSE;
		} else if (filebrowser->basedir == NULL && strcmp(name, "/") ==0) {
			/* found the root */
			tmp = retval;
			retval = g_strconcat(name, retval,NULL);
			g_free(tmp);
			DEBUG_MSG("return_filename_from_path, found the root (%s ?), retval=%s\n", name,retval);
			valid = FALSE;
		} else {
			tmp = retval;
			if (retval) {
				retval = g_strconcat(name, "/", retval,NULL);
			} else {
				retval = g_strdup(name);
			}
			g_free(tmp);
			DEBUG_MSG("added a file or directory component %s, retval=%s\n",name,retval);
			valid = gtk_tree_path_up(path);
		}
		g_free(name);
	}
	gtk_tree_path_free(path);

	return retval;
}

static gboolean view_filter(Tfilebrowser *filebrowser, Tdir_entry *entry) {
	if (!main_v->props.filebrowser_show_hidden_files) {
		if (entry->name[0] == '.') {
#ifdef DEBUG_FILTER
			DEBUG_MSG("view_filter, hidden file %s\n", entry->name);
#endif
			return FALSE;
		}
	}
	
	if (entry->type == TYPE_DIR) {
		/* all other filter types don't apply to directories */
		return TRUE;
	}
	
	if (!main_v->props.filebrowser_show_backup_files) {
		if (entry->name[strlen(entry->name)-1] == '~') {
#ifdef DEBUG_FILTER
			DEBUG_MSG("view_filter, backup file %s\n", entry->name);
#endif
			return FALSE;
		}
	}
#ifdef HAVE_GNOME_VFS
	DEBUG_MSG("view_filter, I don't know my uid() for a possibly remote file..\n");
#else  /* HAVE_GNOME_VFS */
	if (!main_v->props.filebrowser_show_others_files) {
		if (!S_ISDIR(entry->stat.st_mode)
		&& (entry->stat.st_uid != getuid())) {
#ifdef DEBUG_FILTER
			DEBUG_MSG("view_filter, we';re not the owner of %s\n", entry->name);
#endif
			return FALSE;
		}
	}
#endif /* HAVE_GNOME_VFS */
	
	{
		gboolean default_retval;
		GList *tmplist;
		if (filebrowser->curfilter && filebrowser->curfilter->filetypes && filebrowser->curfilter->mode == 1) {
		/* there is some filter active, set the default to FALSE except if we find the extension */
			tmplist = g_list_first(filebrowser->curfilter->filetypes);
			default_retval = FALSE;
		} else {
			if (filebrowser->curfilter && filebrowser->curfilter->filetypes) { /* mode == 0, hide certain files */
				tmplist = g_list_first(filebrowser->curfilter->filetypes);
				while (tmplist) {
					Tfiletype *filetype = (Tfiletype *)tmplist->data;
					if (filename_test_extensions(filetype->extensions, entry->name)) {
						return FALSE;
					}
					tmplist = g_list_next(tmplist);
				}
			}
			/* everything that comes here only needs the icon */
			tmplist = g_list_first(FILEBROWSERCONFIG(main_v->filebrowserconfig)->filetypes_with_icon);
			default_retval = TRUE;
		}
		while (tmplist) {
			Tfiletype *filetype = (Tfiletype *)tmplist->data;
			if (filename_test_extensions(filetype->extensions, entry->name)) {
				entry->icon = filetype->icon;
				return TRUE;
			}
			tmplist = g_list_next(tmplist);
		}
		return default_retval;
	}
}


static gboolean find_name_in_dir_entries(GList *list, gchar *name) {
	GList *tmplist;
	
	tmplist = g_list_first(list);
	while (tmplist) {
		Tdir_entry *entry = (Tdir_entry *)tmplist->data;
		if (strcmp(entry->name, name)==0) {
			entry->has_widget = TRUE;
			return TRUE;
		}
		tmplist = g_list_next(tmplist);
	}
	return FALSE;
}
#ifdef HAVE_GNOME_VFS
static GList *return_dir_entries(Tfilebrowser *filebrowser,gchar *dirname) {
	GList *tmplist=NULL;
	Tdir_entry *entry;
	GnomeVFSResult result;
	GnomeVFSDirectoryHandle *handle;
	GnomeVFSFileInfo *gvfi;
	result = gnome_vfs_directory_open(&handle,dirname,GNOME_VFS_FILE_INFO_DEFAULT|GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	if (result != GNOME_VFS_OK) {
		return NULL;
	}
	gvfi = gnome_vfs_file_info_new();
	while (GNOME_VFS_OK == gnome_vfs_directory_read_next(handle, gvfi)) {
		entry = g_new(Tdir_entry,1);
		entry->icon = NULL;
		entry->name = g_strdup(gvfi->name);
		if (gvfi->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
			entry->type = TYPE_DIR;
		} else {
			entry->type = TYPE_FILE;
		}
		if (!view_filter(filebrowser,entry)) {
			/* free entry */
			g_free(entry->name);
			g_free(entry);
		} else {
			entry->has_widget = FALSE;
			tmplist = g_list_append(tmplist, entry);
		}
	}
	gnome_vfs_file_info_unref(gvfi);
	gnome_vfs_directory_close(handle);
	return tmplist;
}
#else /* HAVE_GNOME_VFS */
static GList *return_dir_entries(Tfilebrowser *filebrowser,gchar *dirname) {
	GDir *dir;
	GList *tmplist=NULL;
	Tdir_entry *entry;
	const gchar *name;
	
	dir = g_dir_open(dirname, 0, NULL);
	while ((name = g_dir_read_name(dir))) {
		gchar *fullpath;

		entry = g_new(Tdir_entry,1);
		entry->icon = NULL;
		entry->name = g_strdup(name);
		
		fullpath = g_strconcat(dirname, "/", name, NULL);
		stat(fullpath, &entry->stat);
		g_free(fullpath);
		if (S_ISDIR(entry->stat.st_mode)) {
			entry->type = TYPE_DIR;
		} else {
			entry->type = TYPE_FILE;
			
		}
		if (!view_filter(filebrowser,entry)) {
			/* free entry */
			g_free(entry->name);
			g_free(entry);
		} else {
			entry->has_widget = FALSE;
			tmplist = g_list_append(tmplist, entry);
		}
	}
	g_dir_close(dir);
	return tmplist;
}
#endif /* HAVE_GNOME_VFS */

static void free_dir_entries(GList *direntrylist) {
	GList *tmplist = g_list_first(direntrylist);
	while (tmplist) {
		g_free(((Tdir_entry *)(tmplist->data))->name);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(tmplist);
}

static GtkTreePath *return_path_from_filename(Tfilebrowser *filebrowser,gchar *this_filename) {
	gchar *tmpstr, *p, *filepath;
	gint totlen, curlen, prevlen=1;
	gboolean found=TRUE;
	GtkTreeStore *store = filebrowser->store;
	GtkTreeIter iter,parent;

	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
	parent = iter;

	filepath = g_strdup(this_filename);
	
	if (filebrowser->basedir) {
		prevlen = strlen(filebrowser->basedir);
	}
	
	totlen = strlen(filepath);
	curlen = strlen(strchr(&filepath[prevlen], '/'));
	p = strchr(&filepath[prevlen], '/');
	DEBUG_MSG("return_path_from_filename, filepath=%s\n",filepath);
	while (p && found) {
		gboolean valid;
		
		found = FALSE;
		curlen = strlen(p);
		tmpstr = g_strndup(&filepath[prevlen], (totlen - curlen - prevlen));
		/* now search for this dirname */
		valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(store),&iter,&parent);
		while (valid) {
			gchar *found_name;
			gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, FILENAME_COLUMN, &found_name, -1);
			DEBUG_MSG("return_path_from_filename, comparing %s and %s\n", found_name, tmpstr);
			if (strcmp(found_name, tmpstr)==0) {
				/* we found the node!!, make this node the next parent */
				found = TRUE;
				parent = iter;
				g_free(found_name);
				break;
			}
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
			g_free(found_name);
		}
		g_free(tmpstr);
		prevlen = (totlen - curlen+1);
		p = strchr(&filepath[prevlen], '/');
	}
	g_free(filepath);
	if (found) {
		DEBUG_MSG("return_path_from_filename, we DID found the path, return a path\n");
		return gtk_tree_model_get_path(GTK_TREE_MODEL(store),&iter);
	} else {
		DEBUG_MSG("return_path_from_filename, we did NOT found the path, return NULL\n");
		return NULL;
	}
}

static int comparefunc(GtkTreeModel *store, gchar *name, GtkTreeIter *iter, const gchar *text, gint type) {
	gboolean has_child = gtk_tree_model_iter_has_child(GTK_TREE_MODEL(store), iter);
#ifdef DEBUG_SORTING
	DEBUG_MSG("comparefunc, comparing %s and %s\n", name, text);
#endif
	if (type == TYPE_DIR) {
		if (has_child) {
			return strcmp(name, text);
		} else {
			return 1;
		}
	} else {
		if (has_child) {		
			return -1;
		} else {
			return strcmp(name, text);
		}
	}
}

static gboolean get_iter_at_correct_position(GtkTreeModel *store, GtkTreeIter *parent, GtkTreeIter *iter, const gchar *text, gint type) {
	gint num_children, jumpsize, child, possible_positions;
	gchar *name;

	DEBUG_MSG("get_iter_at_correct_position, started, parent=%p\n",parent);
	/* if parent=NULL it will return the top level, which is correct for the liststore */
	num_children = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), parent);
	if (num_children == 0) {
		DEBUG_MSG("get_iter_at_correct_position, 0 children so we return FALSE\n");
		return FALSE;
	}
	possible_positions = num_children+1;
	jumpsize = (possible_positions+1)/2;
	child = possible_positions - jumpsize;

#ifdef DEBUG_SORTING
	DEBUG_MSG("get_iter_at_correct_position, num_children=%d, jumpsize=%d\n", num_children, jumpsize);
#endif
	if (num_children == 1) {
		gint compare;
		/* if parent=NULL it will return the child from the toplevel which is correct for the liststore */
		gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), iter, parent, 0);
		gtk_tree_model_get(GTK_TREE_MODEL(store),iter,FILENAME_COLUMN,&name, -1);
		compare = comparefunc(store, name, iter, text, type);
		g_free(name);
		if (compare == 0) {
			return FALSE;
		} else if (compare < 0) {
			return FALSE;
		} else {
			return TRUE;
		}
	}
	while (jumpsize > 0) {
		gint compare;
		if (child > num_children) child = num_children;
		if (child < 1) child = 1;
#ifdef DEBUG_SORTING
		DEBUG_MSG("get_iter_at_correct_position, compare %d\n", child);
#endif
		gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), iter, parent, child-1);
		gtk_tree_model_get(GTK_TREE_MODEL(store),iter,FILENAME_COLUMN,&name, -1);
		compare = comparefunc(store, name, iter, text, type);
		g_free(name);
		if (compare == 0) {
			return TRUE;
		} else if (compare > 0) {
			jumpsize = (jumpsize > 3) ? (jumpsize+1)/2 : jumpsize-1;
			child = child - jumpsize;
		} else {
			jumpsize = (jumpsize > 3) ? (jumpsize+1)/2 : jumpsize-1;
			child = child + jumpsize;
			if (jumpsize<=0) {
#ifdef DEBUG_SORTING
				DEBUG_MSG("get_iter_at_correct_position, jumpsize too small and compare negative --> next iter\n");
#endif
				return gtk_tree_model_iter_next(GTK_TREE_MODEL(store), iter);
			}
		}
#ifdef DEBUG_SORTING
		DEBUG_MSG("get_iter_at_correct_position, compare=%d, jumpsize=%d, child=%d\n", compare, jumpsize, child);
#endif
	}
	return TRUE;
}

static GtkTreeIter add_tree_item(GtkTreeIter *parent, Tfilebrowser *filebrowser, const gchar *text, gint type, GdkPixbuf *pixbuf) {
	GtkTreeIter iter1, iter2;
	if (!pixbuf) {
		if (type == TYPE_DIR) {	
			pixbuf = FILEBROWSERCONFIG(main_v->filebrowserconfig)->dir_icon;
		} else {
			pixbuf = FILEBROWSERCONFIG(main_v->filebrowserconfig)->unknown_icon;
		}
	}

	if (type == TYPE_FILE && main_v->props.filebrowser_two_pane_view && parent==NULL) {
		if (get_iter_at_correct_position(GTK_TREE_MODEL(filebrowser->store2),parent,&iter2,text,type)) {
			DEBUG_MSG("add_tree_item, inserting\n");
			gtk_list_store_insert_before(GTK_LIST_STORE(filebrowser->store2),&iter1,&iter2);
		} else {
			DEBUG_MSG("add_tree_item, appending\n");
			gtk_list_store_append(GTK_LIST_STORE(filebrowser->store2),&iter1);
		}
		gtk_list_store_set(GTK_LIST_STORE(filebrowser->store2),&iter1,
					PIXMAP_COLUMN, pixbuf,
					FILENAME_COLUMN, text,
					-1);
	} else {
		if (get_iter_at_correct_position(GTK_TREE_MODEL(filebrowser->store),parent,&iter2,text,type)) {
	#ifdef DEBUG
			DEBUG_MSG("add_tree_item, inserting %s!\n", text);
	#endif
			gtk_tree_store_insert_before(GTK_TREE_STORE(filebrowser->store),&iter1,parent,&iter2);
		} else {
	#ifdef DEBUG
			DEBUG_MSG("add_tree_item, appending %s!\n", text);
	#endif
			gtk_tree_store_append(GTK_TREE_STORE(filebrowser->store), &iter1, parent);
		}
		gtk_tree_store_set(GTK_TREE_STORE(filebrowser->store), &iter1,
					PIXMAP_COLUMN, pixbuf,
					FILENAME_COLUMN, text,
					-1);
	}
	return iter1;
}

static void refresh_dir_by_path_and_filename(Tfilebrowser *filebrowser, GtkTreePath *thispath, gchar *filename) {
	GList *direntrylist;
	gboolean valid, remove_dot = FALSE;
	GtkTreeIter myiter, remove_dot_iter;
#ifdef DEBUG_ADDING_TO_TREE
	DEBUG_MSG("refresh_dir_by_path_and_filename, filename='%s'\n", filename);
#endif
	if (filebrowser->last_opened_dir) g_free(filebrowser->last_opened_dir);
	filebrowser->last_opened_dir = g_strdup(filename);
	direntrylist = return_dir_entries(filebrowser,filename);
	{
		GtkTreePath *path = gtk_tree_path_copy(thispath);		
		DEBUG_DUMP_TREE_PATH(path);
		gtk_tree_path_down(path);
		valid = gtk_tree_model_get_iter(GTK_TREE_MODEL(filebrowser->store),&myiter,path);
		gtk_tree_path_free(path);
	}
	while (valid) {
		gchar *name;
		gtk_tree_model_get(GTK_TREE_MODEL(filebrowser->store), &myiter, 1, &name, -1);
		if (!find_name_in_dir_entries(direntrylist, name)) {
			if (direntrylist == NULL && strcmp(name, ".")==0) {
#ifdef DEBUG_ADDING_TO_TREE
				DEBUG_MSG("refresh_dir_by_path_and_filename, not removing fake entry for empty \n");
#endif
				valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(filebrowser->store), &myiter);
			} else {
				if (strcmp(name, ".")==0) {
					/* the dot entry is removed as last one, because the rows fail expanding if they are emptied on the way*/
					remove_dot_iter = myiter;
					valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(filebrowser->store), &myiter);
					remove_dot = TRUE;
				} else {
					GtkTreeIter myiter2 = myiter;
					valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(filebrowser->store), &myiter);
#ifdef DEBUG_ADDING_TO_TREE
					DEBUG_MSG("refresh_dir_by_path_and_filename, removing row for name %s\n", name);
#endif
					gtk_tree_store_remove(filebrowser->store,&myiter2);
				}
			}
		} else {
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(filebrowser->store), &myiter);
		}
		g_free(name);
	}
	/* we should also check the entries in the listtore, if they have to be refreshed as well */
	if (main_v->props.filebrowser_two_pane_view) {
		valid = TRUE;
		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(filebrowser->store2),&myiter);
		while (valid) {
			gchar *name;
			gtk_tree_model_get(GTK_TREE_MODEL(filebrowser->store2), &myiter, 1, &name, -1);
			if (!find_name_in_dir_entries(direntrylist, name)) {
				GtkTreeIter myiter2 = myiter;
				valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(filebrowser->store2), &myiter);
				DEBUG_MSG("refresh_dir_by_path_and_filename, removing %s from liststore\n",name);
				gtk_list_store_remove(filebrowser->store2,&myiter2);
			} else {
				DEBUG_MSG("refresh_dir_by_path_and_filename, %s is valid\n",name);
				valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(filebrowser->store2), &myiter);
			}
			g_free(name);
		}
	}
	
	/* now add the new entries */
	{
		GList *tmplist;
		gtk_tree_model_get_iter(GTK_TREE_MODEL(filebrowser->store),&myiter,thispath);
		
		tmplist = g_list_first(direntrylist);
		while (tmplist) {
			Tdir_entry *entry = (Tdir_entry *)tmplist->data;
			if (!entry->has_widget) {
				GtkTreeIter myiter2;
#ifdef DEBUG_ADDING_TO_TREE
				DEBUG_MSG("refresh_dir_by_path_and_filename, adding row for name=%s\n", entry->name);
#endif
				if (entry->type == TYPE_FILE && main_v->props.filebrowser_two_pane_view) {
					myiter2 = add_tree_item(NULL, filebrowser, entry->name, entry->type, entry->icon);
				} else {
					myiter2 = add_tree_item(&myiter, filebrowser, entry->name, entry->type, entry->icon);
				}
				if (entry->type == TYPE_DIR) {
#ifdef DEBUG_ADDING_TO_TREE
					DEBUG_MSG("refresh_dir_by_path_and_filename, %s is TYPE_DIR so we setup the fake item\n", entry->name);
#endif
					add_tree_item(&myiter2, filebrowser, ".", TYPE_FILE, entry->icon);
				}
			}
			tmplist = g_list_next(tmplist);
		}
	}
	if (remove_dot) {
		gtk_tree_store_remove(filebrowser->store,&remove_dot_iter);
	}
	free_dir_entries(direntrylist);
	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(filebrowser->tree));
}

/* can return NULL if the filepath is outside the basedir !! */
static GtkTreePath *build_tree_from_path(Tfilebrowser *filebrowser, const gchar *filepath) {
	GtkTreeIter iter;
	
	if (filebrowser->basedir && strncmp(filepath, filebrowser->basedir, strlen(filebrowser->basedir))!=0) {
		DEBUG_MSG("build_tree_from_path, filepath %s outside basedir %s, returning NULL\n",filepath, filebrowser->basedir);
		return NULL;
	}
	
	/* first build path from root to here */
	{
		gchar *tmpstr, *p;
		gint totlen, curlen, prevlen=1;
		
		if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(filebrowser->store), &iter)) {
			if (filebrowser->basedir) {
				DEBUG_MSG("build_tree_from_path, creating root %s\n",filebrowser->basedir);
				iter = add_tree_item(NULL, filebrowser, filebrowser->basedir, TYPE_DIR, NULL);
			} else {
				DEBUG_MSG("build_tree_from_path, creating root /\n");
				iter = add_tree_item(NULL, filebrowser, "/", TYPE_DIR, NULL);
			}
		}
		
		totlen = strlen(filepath);
		DEBUG_MSG("build_tree_from_path, totlen=%d\n", totlen);
		curlen = strlen(strchr(&filepath[prevlen], '/'));
		DEBUG_MSG("build_tree_from_path, curlen=%d\n", curlen);
		if (filebrowser->basedir) {
			prevlen = strlen(filebrowser->basedir);
		}
		DEBUG_MSG("build_tree_from_path, initial prevlen=%d, searching for / in '%s'\n", prevlen, &filepath[prevlen]);
		p = strchr(&filepath[prevlen], '/');
		while (p) {
			curlen = strlen(p);
	#ifdef DEBUG_ADDING_TO_TREE
			DEBUG_MSG("build_tree_from_path, curlen=%d\n", curlen);
	#endif
			tmpstr = g_strndup(&filepath[prevlen], (totlen - curlen - prevlen));
	#ifdef DEBUG_ADDING_TO_TREE
			DEBUG_MSG("build_tree_from_path, tmpstr='%s'\n", tmpstr);
	#endif
			if (!get_iter_by_filename_from_parent(filebrowser->store, &iter, tmpstr)) {
				iter = add_tree_item(&iter, filebrowser, tmpstr, TYPE_DIR, NULL);
			}
			g_free(tmpstr);
			prevlen = (totlen - curlen+1);
			p = strchr(&filepath[prevlen], '/');
		}
	}
	{
		gchar *dirname;
		GList *direntrylist, *tmplist;
		
		dirname = g_path_get_dirname(filepath);
		DEBUG_MSG("build_tree_from_path, dirname='%s'\n",dirname);
		
		if (filebrowser->last_opened_dir) g_free(filebrowser->last_opened_dir);
		filebrowser->last_opened_dir = g_strdup(dirname);
		
		direntrylist = return_dir_entries(filebrowser,dirname);
		tmplist = g_list_first(direntrylist);
		while (tmplist) {
			Tdir_entry *entry = (Tdir_entry *)tmplist->data;
			if (entry->type == TYPE_DIR) {
				GtkTreeIter tmp;
				tmp = add_tree_item(&iter, filebrowser, entry->name, TYPE_DIR, NULL);
				add_tree_item(&tmp, filebrowser, ".", TYPE_FILE, NULL);
			} else {
				if (main_v->props.filebrowser_two_pane_view) {
					add_tree_item(NULL, filebrowser, entry->name, TYPE_FILE, entry->icon);
				} else {
					add_tree_item(&iter, filebrowser, entry->name, TYPE_FILE, entry->icon);
				}
			}
			tmplist = g_list_next(tmplist);
		}
		g_free(dirname);
		free_dir_entries(direntrylist);
	}
	return gtk_tree_model_get_path(GTK_TREE_MODEL(filebrowser->store),&iter);
}

/* I guess the dir must have a trailing slash in this function .. not sure */
void filebrowser_refresh_dir(Tfilebrowser *filebrowser, gchar *dir) {
	if (filebrowser->tree) {
		/* get the path for this dir */
		GtkTreePath *path = return_path_from_filename(filebrowser, dir);
		DEBUG_DUMP_TREE_PATH(path);
		if (!path) return;
		/* check if the dir is expanded, return if not */	
		if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(filebrowser->tree), path)) {
			DEBUG_MSG("refresh_dir, it IS expanded\n");
			/* refresh it */
			refresh_dir_by_path_and_filename(filebrowser, path, dir);
		} else {
			DEBUG_MSG("refresh_dir, it is NOT expanded\n");
		}
		gtk_tree_path_free(path);
	}
}

void bfwin_filebrowser_refresh_dir(Tbfwin *bfwin, gchar *dir) {
	filebrowser_refresh_dir(FILEBROWSER(bfwin->filebrowser), dir);
}

static GtkTreePath *filebrowser_path_up_multi(GtkTreePath *path, gint num) {
	while (num > 0) {
		gtk_tree_path_up(path);
		num--;
	}
	return path;
}

static void filebrowser_expand_to_root(Tfilebrowser *filebrowser, const GtkTreePath *this_path) {
	gint num = gtk_tree_path_get_depth((GtkTreePath *)this_path);
	DEBUG_MSG("filebrowser_expand_to_root, num=%d, ",num);
	DEBUG_DUMP_TREE_PATH((GtkTreePath *) this_path);
	while (num >= 0) {
		GtkTreePath *path = gtk_tree_path_copy(this_path);
		path = filebrowser_path_up_multi(path, num);
		DEBUG_MSG("filebrowser_expand_to_root, expanding ");
		DEBUG_DUMP_TREE_PATH(path);
		g_signal_handlers_block_matched(G_OBJECT(filebrowser->tree), G_SIGNAL_MATCH_FUNC,
					0, 0, NULL, row_expanded_lcb, NULL);
		gtk_tree_view_expand_row(GTK_TREE_VIEW(filebrowser->tree), path, FALSE);
		g_signal_handlers_unblock_matched(G_OBJECT(filebrowser->tree), G_SIGNAL_MATCH_FUNC,
					0, 0, NULL, row_expanded_lcb, NULL);
		gtk_tree_path_free(path);
		num--;
	}
}


/**
 * filebrowser_get_path_from_selection:
 * @model: #GtkTreeModel*
 * @treeview: #GtkTreeView*
 * @iter: #GtkTreeIter
 *
 * Retrieves a path to the currently selected item in the filebrowser->
 * iter may be omitted, set to NULL (and usually is).
 *
 * Returns: #GtkTreePath identifying currently selected item. NULL if no item is selected.
 **/
static GtkTreePath *filebrowser_get_path_from_selection(GtkTreeModel *model, GtkTreeView *treeview, GtkTreeIter *iter) {
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	GtkTreeIter localiter;
	GtkTreeIter *usethisiter = (iter != NULL) ? iter : &localiter;
	DEBUG_MSG("filebrowser_get_path_from_selection, selection=%p for treeview %p and model %p\n", selection, treeview, model);
	if (gtk_tree_selection_get_selected(selection,&model,usethisiter)) {
		return gtk_tree_model_get_path(GTK_TREE_MODEL(model),usethisiter);
	}
	return NULL;
}


/**
 * filebrowser_open_dir:
 * bfwin: #Tbfwin* with filebrowser window
 * @dirarg const #char * dirname or filename to focus on.
 *
 * This function makes the filebrowser zoom to a designated directory,
 * unless the selected item is this directory (or contained in it).
 **/
void filebrowser_open_dir(Tbfwin *bfwin,const gchar *dirarg) {
	Tfilebrowser *filebrowser = FILEBROWSER(bfwin->filebrowser);
	if (filebrowser->tree) {
		gchar *dir;
		gchar *seldir = NULL, *selfile = NULL; /* Filebrowser-selected file/dir. Temp storage. */
		GtkTreePath *path;
		GtkTreePath *selpath; /* Path to currently selected item, if it exists. */
		
		/* first check if the dir already exists */
		if (!g_file_test(dirarg,G_FILE_TEST_IS_DIR)) {
			dir = path_get_dirname_with_ending_slash(dirarg);
		} else {
			dir = g_strdup(dirarg);
		}
		path = return_path_from_filename(filebrowser, dir);
		DEBUG_MSG("filebrowser_open_dir, called for %s, dir=%s\n", dirarg, dir);
		DEBUG_DUMP_TREE_PATH(path);

		/* Get currently selected item. */
		selpath = filebrowser_get_path_from_selection(GTK_TREE_MODEL(filebrowser->store), GTK_TREE_VIEW(filebrowser->tree),NULL);
		if(selpath) {
			selfile = return_filename_from_path(filebrowser,GTK_TREE_MODEL(filebrowser->store), selpath);
			seldir = path_get_dirname_with_ending_slash(selfile);
			gtk_tree_path_free(selpath);
		}
		/* Continue if a) no selection or b) Zoom neccessary */
		if(!seldir || strcmp(seldir, dir) != 0) {
			if (path) {
				DEBUG_MSG("jump_to_dir, it exists in tree, refreshing\n");
				refresh_dir_by_path_and_filename(filebrowser, path, dir);
				DEBUG_MSG("jump_to_dir, now scroll to the path\n");
				filebrowser_expand_to_root(filebrowser,path);
				gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(filebrowser->tree),path,0,TRUE,0.5,0.5);
			} else {
				DEBUG_MSG("jump_to_dir, it does NOT exist in the tree, building..\n");
				path = build_tree_from_path(filebrowser, dir);
/*				path = return_path_from_filename(GTK_TREE_STORE(filebrowser->store), dir);*/
/*				gtk_tree_view_expand_row(GTK_TREE_VIEW(filebrowser->tree),path,FALSE);*/
				filebrowser_expand_to_root(filebrowser,path);
				gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(filebrowser->tree),path,0,TRUE,0.5,1.0);
			}
		}
		gtk_tree_path_free(path);
		g_free(dir);
		g_free(seldir);
		g_free(selfile);
	}
}

static void dirmenu_activate_lcb(GtkMenuItem *widget, Tfilebrowser *filebrowser) {
	const gchar *dir = gtk_label_get_text(GTK_LABEL(GTK_BIN(widget)->child));
	if (dir) {
		DEBUG_MSG("dirmenu_activate_lcb, dir=%s\n", dir);
		filebrowser_open_dir(filebrowser->bfwin,dir);
	}
}

static void populate_dir_history(Tfilebrowser *filebrowser,gboolean firsttime) {
	GtkWidget *menu;
	gchar *tmpchar, *tmpdir, *tmp;
	GList *tmplist;
	GList *new_history_list=NULL;
	DEBUG_MSG("populate_dir_history, empty\n");

	if (!firsttime) {
		gtk_option_menu_remove_menu(GTK_OPTION_MENU(filebrowser->dirmenu));
		free_stringlist(filebrowser->dirmenu_entries);
		filebrowser->dirmenu_entries = NULL;
	
		menu = gtk_menu_new();
		gtk_option_menu_set_menu(GTK_OPTION_MENU(filebrowser->dirmenu), menu);
		gtk_widget_show(menu);
	} else {
		filebrowser->dirmenu_entries = NULL;
		menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(filebrowser->dirmenu));
		gtk_widget_show(menu);
	}

	tmp = g_get_current_dir();
	tmpdir = g_strconcat(tmp, DIRSTR, NULL);
	g_free(tmp);
	
	new_history_list = add_to_stringlist(new_history_list, tmpdir);
	while ((tmpchar = strrchr(tmpdir, DIRCHR))) {
		tmpchar++;
		*tmpchar = '\0';
		DEBUG_MSG("populate_dir_history, adding %s\n", tmpdir);
		new_history_list = add_to_stringlist(new_history_list, tmpdir);
		tmpchar--;
		*tmpchar = '\0';
	}
	g_free(tmpdir);
	
	tmpdir = g_strconcat(g_get_home_dir(), DIRSTR, NULL);
	new_history_list = add_to_stringlist(new_history_list, tmpdir);
	g_free(tmpdir);

	tmplist = g_list_last(main_v->recent_directories);
	while (tmplist) {
		new_history_list = add_to_stringlist(new_history_list, (gchar *)tmplist->data);
		tmplist = g_list_previous(tmplist);
	}

	/* make the actual option menu */
	tmplist = g_list_first(new_history_list);
	while (tmplist) {
		GtkWidget *menuitem;
		gchar *dirname = ending_slash((gchar *)tmplist->data);
		DEBUG_MSG("populate_dir_history, adding %s to menu\n", dirname);
		menuitem = gtk_menu_item_new_with_label(dirname);
		g_signal_connect(G_OBJECT(menuitem),"activate",G_CALLBACK(dirmenu_activate_lcb),filebrowser);
		g_free(dirname);
		gtk_menu_append(GTK_MENU(menu), menuitem);
		gtk_widget_show(menuitem);
		g_free(tmplist->data);
		tmplist = g_list_next(tmplist);
	} 
	g_list_free(new_history_list);

	gtk_option_menu_set_history(GTK_OPTION_MENU(filebrowser->dirmenu), 0);
}

typedef struct {
	GtkWidget *win;
	GtkWidget *entry;
	gchar *basedir; /* must have a trailing slash */
	gint is_file;
	Tfilebrowser *filebrowser;
} Tcfod;

static void create_file_or_dir_destroy_lcb(GtkWidget *widget, Tcfod *ws) {
	window_destroy(ws->win);
	g_free(ws->basedir);
	g_free(ws);
}

static void create_file_or_dir_cancel_clicked_lcb(GtkWidget *widget, Tcfod *ws) {
	create_file_or_dir_destroy_lcb(NULL, ws);
}

static void create_file_or_dir_ok_clicked_lcb(GtkWidget *widget, Tcfod *ws) {
	gchar *newname, *name;

	name = gtk_editable_get_chars(GTK_EDITABLE(ws->entry), 0, -1);	
	if (name) {
		if (strlen(name)) {
			newname = g_strconcat(ws->basedir,name, NULL);
			DEBUG_MSG("create_file_or_dir_ok_clicked_lcb, newname=%s\n", newname);
			if (file_exists_and_readable(newname)) {
				error_dialog(ws->filebrowser->bfwin->main_window,_("Error creating path"),_("The specified pathname already exists."));
			} else {
				if (ws->is_file) {
					doc_new_with_new_file(ws->filebrowser->bfwin,newname);
				} else {
#ifdef HAVE_GNOME_VFS
					gnome_vfs_make_directory(newname,0755);
#else
					if(mkdir(newname, 0755)== -1) {
/*						error_dialog(_("Error creating directory"),strerror(errno));*/
					}
#endif
				}
			}
			g_free(newname);
			DEBUG_MSG("create_file_or_dir_ok_clicked_lcb, refreshing basedir %s\n", ws->basedir);
			filebrowser_refresh_dir(ws->filebrowser,ws->basedir);
		}
		g_free(name);
	}
	create_file_or_dir_destroy_lcb(NULL, ws);
}

static void create_file_or_dir_win(Tfilebrowser *filebrowser, gint is_file) {
	GtkTreePath *path;
	GtkTreeIter iter;
	path = filebrowser_get_path_from_selection(GTK_TREE_MODEL(filebrowser->store),GTK_TREE_VIEW(filebrowser->tree),&iter);
	if (path) {
		Tcfod *ws;
		gchar *title, *tmp;
		GtkWidget *vbox, *but, *hbox;
		GtkTreePath *path;

		ws = g_malloc(sizeof(Tcfod));
		ws->filebrowser = filebrowser;
		if (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(filebrowser->store), &iter)) {
			DEBUG_MSG("create_file_or_dir_win, a dir is selected\n");
		} else {
			GtkTreeIter parent;
			DEBUG_MSG("create_file_or_dir_win, a file is selected\n");
			gtk_tree_model_iter_parent(GTK_TREE_MODEL(filebrowser->store), &parent, &iter);
			iter = parent;
		}
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(filebrowser->store),&iter);
		tmp = return_filename_from_path(filebrowser,GTK_TREE_MODEL(filebrowser->store),path);
		ws->basedir = ending_slash(tmp);
		g_free(tmp);
		gtk_tree_path_free(path);
		
		if (is_file) {
			title = _("File name");
		} else {
			title = _("Directory name");
		}
		ws->is_file = is_file;

		ws->win = window_full2(title, GTK_WIN_POS_MOUSE, 5,G_CALLBACK(create_file_or_dir_destroy_lcb), ws, TRUE, NULL);
		vbox = gtk_vbox_new(FALSE, 12);
		gtk_container_add(GTK_CONTAINER(ws->win), vbox);
		ws->entry = boxed_entry_with_text(NULL, 250, vbox);
	
		hbox = gtk_hbutton_box_new();
		gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
		gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 6);
	
		but = bf_stock_cancel_button(G_CALLBACK(create_file_or_dir_cancel_clicked_lcb), ws);
		gtk_box_pack_start(GTK_BOX(hbox),but , FALSE, FALSE, 0);	
	
		but = bf_stock_ok_button(G_CALLBACK(create_file_or_dir_ok_clicked_lcb), ws);
		gtk_box_pack_start(GTK_BOX(hbox),but , FALSE, FALSE, 0);	
		
		gtk_entry_set_activates_default(GTK_ENTRY(ws->entry), TRUE);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
		gtk_widget_grab_focus(ws->entry);
		gtk_widget_grab_default(but);
		gtk_widget_show_all(GTK_WIDGET(ws->win));
	}
	gtk_tree_path_free(path);
}


static void row_expanded_lcb(GtkTreeView *tree,GtkTreeIter *iter,GtkTreePath *path,Tfilebrowser *filebrowser) {
	gchar *filename = return_filename_from_path(filebrowser,GTK_TREE_MODEL(filebrowser->store),path);
	DEBUG_MSG("row_expanded_lcb, started on filename=%s\n", filename);
	statusbar_message(filebrowser->bfwin,_("Opening directory..."), 500);
	flush_queue();
	refresh_dir_by_path_and_filename(filebrowser, path, filename);
	g_free(filename);
	DEBUG_MSG("row_expanded_lcb, finished\n");
}

static void handle_activate_on_file(Tfilebrowser *filebrowser, gchar *filename) {
	Tfiletype *ft = get_filetype_by_filename_and_content(filename, NULL);
	DEBUG_MSG("handle_activate_on_file, file %s has type %p\n",filename, ft);
	if (ft == NULL || ft->editable) {
		doc_new_with_file(filebrowser->bfwin,filename, FALSE);
	} else if (strcmp(ft->type, "webimage")==0 || strcmp(ft->type, "image")==0) {
		gchar *relfilename = create_relative_link_to(filebrowser->bfwin->current_document->filename, filename);
		image_insert_from_filename(filebrowser->bfwin,relfilename);
		g_free(relfilename);
	} else {
		DEBUG_MSG("handle_activate_on_file, file %s is not-editable, do something special now?\n",filename);
	}
}

/* Called both by the filebrowser "activate"-signal and filebrowser_rpopup_open_lcb */
static void row_activated_lcb(GtkTreeView *tree, GtkTreePath *path,GtkTreeViewColumn *column, Tfilebrowser *filebrowser) {
	GtkTreeIter iter;
	gchar *filename = return_filename_from_path(filebrowser,GTK_TREE_MODEL(filebrowser->store),path);
	DEBUG_MSG("row_activated_lcb, filename=%s\n", filename);
	/* test if this is a dir */
	gtk_tree_model_get_iter(GTK_TREE_MODEL(filebrowser->store),&iter,path);
	if (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(filebrowser->store),&iter)) {
		if (gtk_tree_view_row_expanded(tree, path)) {
			gtk_tree_view_collapse_row(tree,path);
		} else {
			gtk_tree_view_expand_row(tree,path,FALSE);
		}
	} else {
		handle_activate_on_file(filebrowser,filename);
	}
	g_free(filename);
	DEBUG_MSG("row_activated_lcb, finished\n");
}

/*
 * filebrowser_rpopup_rename
 *
 * Called by filebrowser-> Renames the currently selected item.
 * If the selected item is an opened document, doc_save() is used.
 * Else, it uses the same logic as the former callback to filebrowser->
 *
 * widget and data are not used.
 */
static void filebrowser_rpopup_rename(Tfilebrowser *filebrowser) {
	Tdocument *tmpdoc;
	GList *alldocs;
	GtkTreePath *path;
	gchar *newfilename=NULL, *oldfilename, *errmessage = NULL, *dirname;

	/* this function should, together with doc_save() use a common backend.. */
	
	path = filebrowser_get_path_from_selection(GTK_TREE_MODEL(filebrowser->store),GTK_TREE_VIEW(filebrowser->tree),NULL);
	oldfilename = return_filename_from_path(filebrowser,GTK_TREE_MODEL(filebrowser->store), path);	

	/* Use doc_save(doc, 1, 1) if the file is open. */
	alldocs = return_allwindows_documentlist();
	tmpdoc = documentlist_return_document_from_filename(alldocs,oldfilename);
	g_list_free(alldocs);
	if (tmpdoc != NULL) {
		DEBUG_MSG("File is open. Calling doc_save().\n");

		/* If an error occurs, doc_save takes care of notifying the user.
		 * Currently, nothing is done here.
		 */	
		doc_save(tmpdoc, 1, 1);
	} else {
		/* Promt user, "File/Move To"-style. */
		newfilename = ask_new_filename(filebrowser->bfwin,oldfilename, 1);
		if (newfilename) {
#ifdef HAVE_GNOME_VFS
			if (gnome_vfs_move(oldfilename,newfilename,TRUE) != GNOME_VFS_OK) {
				errmessage = g_strconcat(_("Could not rename\n"), oldfilename, NULL);
			}
#else
			if(rename(oldfilename, newfilename) != 0) {
				errmessage = g_strconcat(_("Could not rename\n"), oldfilename, NULL);
			}
#endif
		}
	} /* if(oldfilename is open) */

	if (errmessage) {
		error_dialog(filebrowser->bfwin->main_window,errmessage, NULL);
		g_free(errmessage);
	} else {
		/* Refresh the appropriate parts of the filebrowser-> */
		char *tmp;

		tmp = g_path_get_dirname(oldfilename);
		DEBUG_MSG("Got olddirname %s\n", tmp);
		dirname = ending_slash(tmp);
		filebrowser_refresh_dir(filebrowser,dirname);
		g_free(dirname);
		g_free(tmp);
		
		if (newfilename && (tmp = path_get_dirname_with_ending_slash(newfilename))) { /* Don't refresh the NULL-directory.. */
			DEBUG_MSG("Got newdirname %s\n", tmp);
			dirname = ending_slash(tmp);
			filebrowser_refresh_dir(filebrowser,dirname);
			g_free(dirname);
			g_free(tmp);
		}
	} /* if(error) */
	if (newfilename) {
		g_free(newfilename);
	}
	gtk_tree_path_free(path);
	g_free(oldfilename);

}

static void filebrowser_rpopup_delete(Tfilebrowser *filebrowser) {
	GtkTreePath *path;
	path = filebrowser_get_path_from_selection(GTK_TREE_MODEL(filebrowser->store),GTK_TREE_VIEW(filebrowser->tree),NULL);
	if (path) {
		gchar *filename;
		filename = return_filename_from_path(filebrowser,GTK_TREE_MODEL(filebrowser->store),path);
		gtk_tree_path_free(path);
		if (filename) {
			gchar *buttons[] = {GTK_STOCK_CANCEL, GTK_STOCK_DELETE, NULL};
			gchar *label;
			gint retval;
			label = g_strdup_printf(_("Are you sure you want to\ndelete \"%s\" ?"), filename);
			retval = multi_query_dialog(filebrowser->bfwin->main_window,label, _("The file will be permanently deleted."), 0, 0, buttons);
			g_free(label);
			if (retval == 1) {
				gchar *tmp, *dir;
				DEBUG_MSG("file_list_rpopup_file_delete %s\n", filename);
#ifdef HAVE_GNOME_VFS
				gnome_vfs_unlink(filename);
#else
				unlink(filename);
#endif
				tmp = g_path_get_dirname(filename);
				dir = ending_slash(tmp);
				g_free(tmp);
				filebrowser_refresh_dir(filebrowser,dir);
				g_free(dir);
				
			}
			g_free(filename);
		}
		
	}
}

static void filebrowser_rpopup_refresh(Tfilebrowser *filebrowser) {
	GtkTreePath *path;
	GtkTreeIter iter;
	path = filebrowser_get_path_from_selection(GTK_TREE_MODEL(filebrowser->store),GTK_TREE_VIEW(filebrowser->tree),&iter);
	if (path) {
		gchar *tmp, *dir;
		GtkTreePath *path;
		if (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(filebrowser->store), &iter)) {
			DEBUG_MSG("create_file_or_dir_win, a dir is selected\n");
		} else {
			GtkTreeIter parent;
			DEBUG_MSG("create_file_or_dir_win, a file is selected\n");
			gtk_tree_model_iter_parent(GTK_TREE_MODEL(filebrowser->store), &parent, &iter);
			iter = parent;
		}
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(filebrowser->store),&iter);
		tmp = return_filename_from_path(filebrowser,GTK_TREE_MODEL(filebrowser->store),path);
		DEBUG_MSG("filebrowser_rpopup_refresh_lcb, return_filename_from_path returned %s\n", tmp);
		dir = ending_slash(tmp);
		DEBUG_MSG("filebrowser_rpopup_refresh_lcb, ending_slash returned %s\n", dir);
		filebrowser_refresh_dir(filebrowser,dir);
		g_free(tmp);
		g_free(dir);
		gtk_tree_path_free(path);
	}
	gtk_tree_path_free(path);
}

static void filebrowser_rpopup_action_lcb(Tfilebrowser *filebrowser,guint callback_action, GtkWidget *widget) {
	switch (callback_action) {
	case 1: {
		GtkTreePath *path;
		path = filebrowser_get_path_from_selection(GTK_TREE_MODEL(filebrowser->store),GTK_TREE_VIEW(filebrowser->tree),NULL);
		if(path) {
			row_activated_lcb(GTK_TREE_VIEW(filebrowser->tree), path, NULL, filebrowser);
			gtk_tree_path_free(path);
		}
	} break;
	case 2:
		filebrowser_rpopup_rename(filebrowser);
	break;
	case 3:
		filebrowser_rpopup_delete(filebrowser);
	break;
	case 4:
		create_file_or_dir_win(filebrowser, TRUE);
	break;
	case 5:
		create_file_or_dir_win(filebrowser,FALSE);
	break;
	case 6:
		filebrowser_rpopup_refresh(filebrowser);
	break;
#ifdef EXTERNAL_GREP
#ifdef EXTERNAL_FIND
	case 7: {
		GtkTreePath *tmppath;
		gchar *path;
		tmppath = filebrowser_get_path_from_selection(GTK_TREE_MODEL(filebrowser->store),GTK_TREE_VIEW(filebrowser->tree),NULL);
		path = return_filename_from_path(filebrowser,GTK_TREE_MODEL(filebrowser->store), tmppath);
		open_advanced_from_filebrowser(filebrowser->bfwin, path);
		gtk_tree_path_free(tmppath);
		g_free(path);
	} break;	
#endif
#endif	
	}
}

static Tfilter *find_filter_by_name(const gchar *name) {
	GList *tmplist = g_list_first(FILEBROWSERCONFIG(main_v->filebrowserconfig)->filters);
	while(tmplist) {
		Tfilter *filter = (Tfilter *)tmplist->data;
		if (strcmp(filter->name, name)==0) {
			return filter;
		}
		tmplist = g_list_next(tmplist);
	}
	return NULL;
}

static void filebrowser_rpopup_filter_toggled_lcb(GtkWidget *widget, Tfilebrowser *filebrowser) {
	if (GTK_CHECK_MENU_ITEM(widget)->active) {
		/* loop trough the filters for a filter with this name */
		const gchar *name = gtk_label_get_text(GTK_LABEL(GTK_BIN(widget)->child));
		Tfilter *filter = find_filter_by_name(name);
		DEBUG_MSG("filebrowser_rpopup_filter_toggled_lcb, setting curfilter to %p from name %s\n", filter, name);
		filebrowser->curfilter = filter;
		if (main_v->props.last_filefilter) g_free(main_v->props.last_filefilter);
		main_v->props.last_filefilter = g_strdup(filter->name);
		filebrowser_rpopup_refresh(filebrowser);
	}
}

static GtkItemFactoryEntry filebrowser_menu_entries[] = {
	{ N_("/_Open"),			NULL,	filebrowser_rpopup_action_lcb,		1,	"<Item>" },
#ifdef EXTERNAL_GREP
#ifdef EXTERNAL_FIND	
	{ N_("/Open _Advanced"),			NULL,	filebrowser_rpopup_action_lcb,		7,	"<Item>" },
#endif
#endif
	{ N_("/sep1"),				NULL,	NULL,								0, "<Separator>" },
	{ N_("/Rena_me"),		NULL,	filebrowser_rpopup_action_lcb,			2,	"<Item>" },
	{ N_("/_Delete"),		NULL,	filebrowser_rpopup_action_lcb,			3,	"<Item>" },
	{ N_("/sep2"),				NULL,	NULL,								0, "<Separator>" },
	{ N_("/New _File"),			NULL,	filebrowser_rpopup_action_lcb,	4,	"<Item>" },
	{ N_("/_New Directory"),	NULL,	filebrowser_rpopup_action_lcb,		5,	"<Item>" },
	{ N_("/sep3"),				NULL,	NULL,								0, "<Separator>" },
	{ N_("/_Refresh"),			NULL,	filebrowser_rpopup_action_lcb,		6,	"<Item>" }};
/*	{ N_("/Filter"),				NULL,	NULL,										0,	"<Branch>" }}*/

static gint filebrowser_menu_entries_count = sizeof (filebrowser_menu_entries) / sizeof (filebrowser_menu_entries[0]);

static GtkWidget *filebrowser_rpopup_create_menu(Tfilebrowser *filebrowser) {
	GtkWidget *menu, *menu_item;
	GtkItemFactory *menumaker;

	/* Create menu as defined in filebrowser_menu_entries[] */
	menumaker = gtk_item_factory_new(GTK_TYPE_MENU, "<Filebrowser>", NULL);
	gtk_item_factory_create_items(menumaker, filebrowser_menu_entries_count, filebrowser_menu_entries, filebrowser);
	menu = gtk_item_factory_get_widget(menumaker, "<Filebrowser>");
		
	/* Add filter submenu */
	menu_item = gtk_menu_item_new_with_label(_("Filter"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	{
		GtkWidget *fmenu;
		GSList *group=NULL;
		GList *tmplist;
		fmenu = gtk_menu_new();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), fmenu);
		tmplist = g_list_first(FILEBROWSERCONFIG(main_v->filebrowserconfig)->filters);
		while (tmplist) {
			Tfilter *filter = (Tfilter *)tmplist->data;
			menu_item = gtk_radio_menu_item_new_with_label(group, filter->name);
			if (filebrowser->curfilter == filter) {
				gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(menu_item), TRUE);
			}
			g_signal_connect(GTK_OBJECT(menu_item), "toggled", G_CALLBACK(filebrowser_rpopup_filter_toggled_lcb), filebrowser);
			if (!group) {
				group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menu_item));
			}
			gtk_menu_shell_append(GTK_MENU_SHELL(fmenu), menu_item);
			tmplist = g_list_next(tmplist);
		}
	}
	gtk_widget_show_all(menu);
	g_signal_connect_after(G_OBJECT(menu), "destroy", G_CALLBACK(destroy_disposable_menu_cb), menu);
	return menu;
}


static gboolean filebrowser_button_press_lcb(GtkWidget *widget, GdkEventButton *event, Tfilebrowser *filebrowser) {
	DEBUG_MSG("filebrowser_button_press_lcb, button=%d\n",event->button);
	if (event->button == 3) {
		GtkWidget *menu = filebrowser_rpopup_create_menu(filebrowser);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
	}
	if (event->button==1 && event->type == GDK_2BUTTON_PRESS) {
		GtkTreePath *path = filebrowser_get_path_from_selection(GTK_TREE_MODEL(filebrowser->store), GTK_TREE_VIEW(filebrowser->tree),NULL);
		DEBUG_MSG("doubleclick!! open the file!!\n");
		if (path) {
			row_activated_lcb(GTK_TREE_VIEW(filebrowser->tree), path,NULL,filebrowser);
		}
		gtk_tree_path_free(path);
		return TRUE;
	}
	return FALSE; /* pass the event on */
}

static gboolean filebrowser_tree2_button_press_lcb(GtkWidget *widget, GdkEventButton *event, Tfilebrowser *filebrowser) {
	if (event->button==1 && event->type == GDK_2BUTTON_PRESS) {
		GtkTreePath *path2;
		DEBUG_MSG("filebrowser_tree2_button_press_lcb, button=%d\n",event->button);
		path2 = filebrowser_get_path_from_selection(GTK_TREE_MODEL(filebrowser->store2), GTK_TREE_VIEW(filebrowser->tree2),NULL);
		if (path2) {
			gchar *tmp1, *tmp2, *tmp3;
			GtkTreeIter iter;
			gtk_tree_model_get_iter(GTK_TREE_MODEL(filebrowser->store2),&iter,path2);
			gtk_tree_model_get(GTK_TREE_MODEL(filebrowser->store2), &iter, FILENAME_COLUMN, &tmp1, -1);
			DEBUG_MSG("filebrowser_tree2_button_press_lcb, last_opened=%s,tmp1=%s\n",filebrowser->last_opened_dir,tmp1);
			tmp2 = ending_slash(filebrowser->last_opened_dir);
			tmp3 = g_strconcat(tmp2,tmp1,NULL);
			handle_activate_on_file(filebrowser,tmp3);
			g_free(tmp1);
			g_free(tmp2);
			g_free(tmp3);
			return TRUE;
		}
	}
	return FALSE; /* pass the event on */
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
					filter->filetypes = g_list_append(filter->filetypes, filetype);
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

/**
 * filebrowser_filters_rebuild:
 *	
 * Reinitializes the filebrowser filters.
 **/
void filebrowser_filters_rebuild() {
	GList *tmplist;

	if (!FILEBROWSERCONFIG(main_v->filebrowserconfig)->dir_icon || !FILEBROWSERCONFIG(main_v->filebrowserconfig)->unknown_icon) {
		g_print("the dir_icon and unknown_icon items in the config file are invalid\n");
/*		return gtk_label_new("cannot load icons");*/
	}

	tmplist = g_list_first(FILEBROWSERCONFIG(main_v->filebrowserconfig)->filters);
	while (tmplist) {
		filter_destroy(tmplist->data);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(FILEBROWSERCONFIG(main_v->filebrowserconfig)->filters);
	FILEBROWSERCONFIG(main_v->filebrowserconfig)->filters = NULL;

	FILEBROWSERCONFIG(main_v->filebrowserconfig)->filetypes_with_icon = NULL;
	tmplist = g_list_first(main_v->filetypelist);
	while (tmplist) {
		if (((Tfiletype *)tmplist->data)->icon) {
			FILEBROWSERCONFIG(main_v->filebrowserconfig)->filetypes_with_icon = g_list_append(FILEBROWSERCONFIG(main_v->filebrowserconfig)->filetypes_with_icon, tmplist->data);
		}
		tmplist = g_list_next(tmplist);
	}
	
	FILEBROWSERCONFIG(main_v->filebrowserconfig)->filters = g_list_append(NULL, new_filter(_("All files"), "0", NULL));
	
	tmplist = g_list_first(main_v->props.filefilters);
	while (tmplist) {
		gchar **strarr = (gchar **) tmplist->data;
		if (count_array(strarr) == 3) {
			Tfilter *filter = new_filter(strarr[0], strarr[1], strarr[2]);
			FILEBROWSERCONFIG(main_v->filebrowserconfig)->filters = g_list_append(FILEBROWSERCONFIG(main_v->filebrowserconfig)->filters, filter);
		}
		tmplist = g_list_next(tmplist);
	}
}

/**
 * filebrowser_set_basedir:
 * @bfwin: #Tbfwin*
 * @basedir: #const gchar* 
 *
 * will set this dir as the basedir for the filebrowser
 * a call with NULL is basically the same as setting it to "/"
 *
 * it there is no slash / appended, this function will add a slash to the end dir
 *
 * Return value: #void
 **/
void filebrowser_set_basedir(Tbfwin *bfwin, const gchar *basedir) {
	if (bfwin->filebrowser) {
		GtkTreePath *path;
		DEBUG_MSG("filebrowser_set_basedir, basedir=%s\n",basedir);
		if (FILEBROWSER(bfwin->filebrowser)->basedir) {
			g_free(FILEBROWSER(bfwin->filebrowser)->basedir);
		}
		if (basedir) {
			FILEBROWSER(bfwin->filebrowser)->basedir = ending_slash(basedir);
		} else {
			FILEBROWSER(bfwin->filebrowser)->basedir = NULL;
		}
		DEBUG_MSG("filebrowser_set_basedir, set basedir to %s\n",FILEBROWSER(bfwin->filebrowser)->basedir);
		/* now rebuild the tree */
		gtk_tree_store_clear(FILEBROWSER(bfwin->filebrowser)->store);
		if (FILEBROWSER(bfwin->filebrowser)->store2) {
			gtk_list_store_clear(FILEBROWSER(bfwin->filebrowser)->store2);
		}
		path = build_tree_from_path(FILEBROWSER(bfwin->filebrowser), basedir);
		if (path) {
			filebrowser_expand_to_root(FILEBROWSER(bfwin->filebrowser),path);
			gtk_tree_path_free(path);
		}
	}
}

/**
 * filebrowsel_init:
 * @bfwin: #Tbfwin*
 *
 * Initializer. Currently called from left_panel_build().
 *
 * Return value: #void
 **/
GtkWidget *filebrowser_init(Tbfwin *bfwin) {
	Tfilebrowser *filebrowser;
	if (bfwin->filebrowser) {
		filebrowser = (Tfilebrowser *)bfwin->filebrowser;
	} else {
		filebrowser =  g_new0(Tfilebrowser,1);
		bfwin->filebrowser = filebrowser;
		filebrowser->bfwin = bfwin;
		if (bfwin->project && bfwin->project->basedir && strlen(bfwin->project->basedir)>2) {
			filebrowser->basedir = ending_slash(bfwin->project->basedir);
		}
	}
	if (!filebrowser->curfilter) {
		/* get the default filter */
		filebrowser->curfilter = find_filter_by_name(main_v->props.last_filefilter);
	}
	
	filebrowser->store = gtk_tree_store_new (N_COLUMNS,GDK_TYPE_PIXBUF,G_TYPE_STRING);
	filebrowser->tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL(filebrowser->store));
	/* The view now holds a reference.  We can get rid of our own reference */
	g_object_unref(G_OBJECT(filebrowser->store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(filebrowser->tree), FALSE);	

	{
		GtkCellRenderer *renderer;
		GtkTreeViewColumn *column;
		renderer = gtk_cell_renderer_pixbuf_new();
		column = gtk_tree_view_column_new ();
		gtk_tree_view_column_pack_start (column, renderer, FALSE);
		
		gtk_tree_view_column_set_attributes(column,renderer
			,"pixbuf",PIXMAP_COLUMN
			,"pixbuf_expander_closed",PIXMAP_COLUMN
			,"pixbuf_expander_open",PIXMAP_COLUMN
			,NULL);
/*		g_object_set(G_OBJECT(renderer), "xpad", 1, NULL);
		g_object_set(G_OBJECT(renderer), "ypad", 1, NULL);
		gtk_tree_view_column_set_spacing(column, 1);
		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);*/
		gtk_tree_view_append_column (GTK_TREE_VIEW(filebrowser->tree), column);
		
		renderer = gtk_cell_renderer_text_new();
/*		column = gtk_tree_view_column_new ();*/
		g_object_set(G_OBJECT(renderer), "editable", FALSE, NULL); /* Not editable. */
/*		g_object_set(G_OBJECT(renderer), "xpad", 1, NULL);
		g_object_set(G_OBJECT(renderer), "ypad", 1, NULL);*/
/*		g_signal_connect(G_OBJECT(renderer), "edited", G_CALLBACK(renderer_edited_lcb), filebrowser->store);	 */
		gtk_tree_view_column_pack_start (column, renderer, TRUE);
		gtk_tree_view_column_set_attributes(column,renderer
			,"text", FILENAME_COLUMN,NULL);
/*		gtk_tree_view_column_set_spacing(column, 1);
		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);*/
/*		gtk_tree_view_append_column(GTK_TREE_VIEW(filebrowser->tree), column);*/
	}
	
	
	if (main_v->props.filebrowser_two_pane_view) {
		GtkCellRenderer *renderer;
		GtkTreeViewColumn *column;

		filebrowser->store2 = gtk_list_store_new (N_COLUMNS,GDK_TYPE_PIXBUF,G_TYPE_STRING);
		filebrowser->tree2 = gtk_tree_view_new_with_model(GTK_TREE_MODEL(filebrowser->store2));
		/* The view now holds a reference.  We can get rid of our own reference */
		g_object_unref(G_OBJECT(filebrowser->store2));
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(filebrowser->tree2), FALSE);

		renderer = gtk_cell_renderer_pixbuf_new();
		column = gtk_tree_view_column_new ();
		gtk_tree_view_column_pack_start (column, renderer, FALSE);
		gtk_tree_view_column_set_attributes(column,renderer
			,"pixbuf",PIXMAP_COLUMN
			,"pixbuf_expander_closed",PIXMAP_COLUMN
			,"pixbuf_expander_open",PIXMAP_COLUMN
			,NULL);
		renderer = gtk_cell_renderer_text_new();
		g_object_set(G_OBJECT(renderer), "editable", FALSE, NULL); /* Not editable. */
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_set_attributes(column,renderer,"text", FILENAME_COLUMN,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(filebrowser->tree2), column);
	} else {
		filebrowser->store2 = NULL;
		filebrowser->tree2 = NULL;
	}
	
	{
		gchar curdir[1024];
		GtkTreePath *path;
		
		getcwd(curdir, 1023);
		strncat(curdir, "/", 1023);
		DEBUG_MSG("curdir=%s\n",curdir);
		path = build_tree_from_path(filebrowser, curdir);
		if (path) {
			filebrowser_expand_to_root(filebrowser,path);
			gtk_tree_path_free(path);
		}
	}
	
	g_signal_connect(G_OBJECT(filebrowser->tree), "row-expanded", G_CALLBACK(row_expanded_lcb), filebrowser);
	g_signal_connect(G_OBJECT(filebrowser->tree), "row-activated",G_CALLBACK(row_activated_lcb),filebrowser);
	g_signal_connect(G_OBJECT(filebrowser->tree), "button_press_event",G_CALLBACK(filebrowser_button_press_lcb),filebrowser);
	if (main_v->props.filebrowser_two_pane_view) {
		g_signal_connect(G_OBJECT(filebrowser->tree2), "button_press_event",G_CALLBACK(filebrowser_tree2_button_press_lcb),filebrowser);
	}

	{
		GtkWidget *vbox, *scrolwin;
	/*	GtkAdjustment* adj;*/
		vbox = gtk_vbox_new(FALSE, 0);
		
		filebrowser->dirmenu = gtk_option_menu_new();
		gtk_option_menu_set_menu(GTK_OPTION_MENU(filebrowser->dirmenu), gtk_menu_new());
		populate_dir_history(filebrowser,TRUE);
		gtk_box_pack_start(GTK_BOX(vbox),filebrowser->dirmenu, FALSE, FALSE, 0);
		
		scrolwin = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_container_add(GTK_CONTAINER(scrolwin), filebrowser->tree);
		if (main_v->props.filebrowser_two_pane_view) {
			GtkWidget *vpaned, *scrolwin2;
			vpaned = gtk_vpaned_new();
			gtk_paned_add1(GTK_PANED(vpaned), scrolwin);
			scrolwin2 = gtk_scrolled_window_new(NULL, NULL);
			gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin2), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
			gtk_container_add(GTK_CONTAINER(scrolwin2), filebrowser->tree2);
			gtk_paned_add2(GTK_PANED(vpaned), scrolwin2);
			gtk_widget_set_size_request(vpaned, main_v->props.left_panel_width, -1);
			gtk_box_pack_start(GTK_BOX(vbox), vpaned, TRUE, TRUE, 0);
		} else {
			gtk_widget_set_size_request(scrolwin, main_v->props.left_panel_width, -1);
			gtk_box_pack_start(GTK_BOX(vbox), scrolwin, TRUE, TRUE, 0);
		}
/*		adj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolwin));
		gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), GTK_ADJUSTMENT(adj)->lower);
		adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolwin));
		gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), GTK_ADJUSTMENT(adj)->lower);*/
		return vbox;
	}
}

void filebrowser_scroll_initial(Tbfwin *bfwin) {
	DEBUG_MSG("filebrowser_scroll_initial, bfwin=%p, filebrowser=%p\n",bfwin,bfwin->filebrowser);
	if (FILEBROWSER(bfwin->filebrowser)->tree) {
		gtk_tree_view_scroll_to_point(GTK_TREE_VIEW(FILEBROWSER(bfwin->filebrowser)->tree), 2000, 2000);
	}
}

void filebrowser_cleanup(Tbfwin *bfwin) {
	/* is this cleanup complete ? I wonder... we need some memleak detection here.. */
	FILEBROWSER(bfwin->filebrowser)->store = NULL;
	FILEBROWSER(bfwin->filebrowser)->tree = NULL;
}

void filebrowserconfig_init() {
	gchar *filename;
	Tfilebrowserconfig *fc = g_new0(Tfilebrowserconfig,1);
	main_v->filebrowserconfig = fc;

	filename = return_first_existing_filename(main_v->props.filebrowser_unknown_icon,
					"icon_unknown.png","../icons/icon_unknown.png",
					"icons/icon_unknown.png",NULL);
		
	fc->unknown_icon = gdk_pixbuf_new_from_file(filename, NULL);
	g_free(filename);
		
	filename = return_first_existing_filename(main_v->props.filebrowser_dir_icon,
					"icon_dir.png","../icons/icon_dir.png",
					"icons/icon_dir.png",NULL);

	fc->dir_icon = gdk_pixbuf_new_from_file(filename, NULL);
	g_free(filename);
}

