/* Bluefish HTML Editor
 * filebrowser.c the filebrowser
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
/* #define DEBUG */

#include <gtk/gtk.h>
#include <sys/types.h> /* stat() getuid */
#include <sys/stat.h> /* stat() */
#include <unistd.h> /* stat() getuid */
#include <string.h> /*strchr() */
#include <stdio.h> /* rename() */	
#include <stdlib.h> /* atoi() */

#include "bluefish.h"
#include "document.h"
#include "gtk_easy.h" /* error_dialog() */
#include "bf_lib.h"
#include "stringlist.h" /* count_array() */
#include "filebrowser.h"

/* #define DEBUG_SORTING */
/* #define DEBUG_FILTER */
/* #define DEBUG_ADDING_TO_TREE */

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
	uid_t uid;
	Tfilter *curfilter;
	GList *filters;
	GList *filetypes_with_icon;
	GdkPixbuf *unknown_icon;
	GdkPixbuf *dir_icon;
	GtkWidget *tree;
	GtkTreeStore *store;
	GtkWidget *dirmenu;
	GList *dirmenu_entries;
} Tfilebrowser;

/*******************************/
/* global vars for this module */
/*******************************/
static Tfilebrowser filebrowser = {0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};


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
			return TRUE;
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
	}
	DEBUG_MSG("get_iter_by_filename_from_parent, NOT found..\n");
	return FALSE;
}

static gchar *return_filename_from_path(GtkTreeStore *store,const GtkTreePath *thispath) {
	gboolean valid = TRUE;
	gchar *retval = NULL, *tmp;
	GtkTreePath *path = gtk_tree_path_copy(thispath);

	while (valid) {
		gchar *name, *tmpstr;
		GtkTreeIter iter;

		tmpstr = gtk_tree_path_to_string(path);
		DEBUG_MSG("adding path='%s' to filename\n", tmpstr);
		g_free(tmpstr);

		gtk_tree_model_get_iter(GTK_TREE_MODEL(store),&iter,path);
		gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, FILENAME_COLUMN, &name, -1);
		if (strcmp(name, "/") !=0) {
			tmp = retval;
			retval = g_strconcat("/", name, retval,NULL);
			g_free(tmp);
			valid = gtk_tree_path_up(path);
		} else {
			/* found the root */
			if (!retval) {
				retval = g_strdup("/");
			}
			DEBUG_MSG("return_filename_from_path, found the root, retval=%s\n", retval);
			valid = FALSE;
		}
	}
	gtk_tree_path_free(path);
	return retval;
}

static gboolean view_filter(Tdir_entry *entry) {
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
	if (!main_v->props.filebrowser_show_others_files) {
		if (!S_ISDIR(entry->stat.st_mode)
		&& (entry->stat.st_uid != filebrowser.uid)) {
#ifdef DEBUG_FILTER
			DEBUG_MSG("view_filter, we';re not the owner of %s\n", entry->name);
#endif
			return FALSE;
		}
	}
	
	{
		gboolean default_retval;
		GList *tmplist;
		if (filebrowser.curfilter->filetypes && filebrowser.curfilter->mode == 1) {
		/* there is some filter active, set the default to FALSE except if we find the extension */
			tmplist = g_list_first(filebrowser.curfilter->filetypes);
			default_retval = FALSE;
		} else {
			if (filebrowser.curfilter->filetypes) { /* mode == 0, hide certain files */
				tmplist = g_list_first(filebrowser.curfilter->filetypes);
				while (tmplist) {
					Tfiletype *filetype = (Tfiletype *)tmplist->data;
					if (filename_test_extensions(filetype->extensions, entry->name)) {
						return FALSE;
					}
					tmplist = g_list_next(tmplist);
				}
			}
			/* everything that comes here only needs the icon */
			tmplist = g_list_first(filebrowser.filetypes_with_icon);
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

static GList *return_dir_entries(gchar *dirname) {
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
		if (!view_filter(entry)) {
			/* free entry */
			g_free(entry);
		} else {
			entry->has_widget = FALSE;
			tmplist = g_list_append(tmplist, entry);
		}
	}
	g_dir_close(dir);
	return tmplist;
}

static void free_dir_entries(GList *direntrylist) {
	GList *tmplist = g_list_first(direntrylist);
	while (tmplist) {
		g_free(((Tdir_entry *)(tmplist->data))->name);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(tmplist);
}

static GtkTreePath *return_path_from_filename(GtkTreeStore *store,gchar *this_filename) {
	gchar *tmpstr, *p, *filepath=g_strdup(this_filename);
	gint totlen, curlen, prevlen=1;
	gboolean found=TRUE;
	GtkTreeIter iter,parent;

	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
	parent = iter;
	
	totlen = strlen(filepath);
	curlen = strlen(strchr(&filepath[prevlen], '/'));
	p = strchr(&filepath[prevlen], '/');
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
				break;
			}
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
		}
		g_free(tmpstr);
		prevlen = (totlen - curlen+1);
		p = strchr(&filepath[prevlen], '/');
	}
	if (found) {
		DEBUG_MSG("return_path_from_filename, we DID found the path, return a path\n");
		return gtk_tree_model_get_path(GTK_TREE_MODEL(store),&iter);
	} else {
		DEBUG_MSG("return_path_from_filename, we did NOT found the path, return NULL\n");
		return NULL;
	}
}

static int comparefunc(GtkTreeStore *store, gchar *name, GtkTreeIter *iter, const gchar *text, gint type) {
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

static gboolean get_iter_at_correct_position(GtkTreeStore *store, GtkTreeIter *parent, GtkTreeIter *iter, const gchar *text, gint type) {
	gint num_children, jumpsize, child, possible_positions;
	gchar *name;

	DEBUG_MSG("get_iter_at_correct_position, started\n");
	if (!parent || !gtk_tree_model_iter_has_child(GTK_TREE_MODEL(store), parent)) {
		DEBUG_MSG("get_iter_at_correct_position, this parent does NOT have children, returning FALSE\n");
		return FALSE;
	}
	DEBUG_MSG("get_iter_at_correct_position, this parent DOES have children\n");
	num_children = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), parent);
	possible_positions = num_children+1;
	jumpsize = (possible_positions+1)/2;
	child = possible_positions - jumpsize;

#ifdef DEBUG_SORTING
	DEBUG_MSG("get_iter_at_correct_position, num_children=%d, jumpsize=%d\n", num_children, jumpsize);
#endif
	if (num_children == 1) {
		gint compare;
		gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), iter, parent, 0);
		gtk_tree_model_get(GTK_TREE_MODEL(store),iter,FILENAME_COLUMN,&name, -1);
		compare = comparefunc(store, name, iter, text, type);
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



static GtkTreeIter add_tree_item(GtkTreeIter *parent, GtkTreeStore *store, const gchar *text, gint type, GdkPixbuf *pixbuf) {
	GtkTreeIter iter1, iter2;

	if (!pixbuf) {
		if (type == TYPE_DIR) {	
			pixbuf = filebrowser.dir_icon;
		} else {
			pixbuf = filebrowser.unknown_icon;
		}
	}
	g_assert(pixbuf);

	if (get_iter_at_correct_position(store,parent,&iter2,text,type)) {
#ifdef DEBUG_SORTING
		DEBUG_MSG("add_tree_item, inserting %s!\n", text);
#endif
		gtk_tree_store_insert_before(store,&iter1,parent,&iter2);
	} else {
#ifdef DEBUG_SORTING
		DEBUG_MSG("add_tree_item, appending %s!\n", text);
#endif
		gtk_tree_store_append(store, &iter1, parent);
	}
	gtk_tree_store_set (store, &iter1,
				PIXMAP_COLUMN, pixbuf,
				FILENAME_COLUMN, text,
				-1);
	return iter1;
}

static void refresh_dir_by_path_and_filename(GtkTreeStore *store, GtkTreePath *thispath, gchar *filename) {
	GList *direntrylist;
	gboolean valid, remove_dot = FALSE;
	GtkTreeIter myiter, remove_dot_iter;
#ifdef DEBUG_ADDING_TO_TREE
	DEBUG_MSG("refresh_dir_by_path_and_filename, filename='%s'\n", filename);
#endif
	direntrylist = return_dir_entries(filename);
	{
		GtkTreePath *path = gtk_tree_path_copy(thispath);		
		DEBUG_DUMP_TREE_PATH(path);
		gtk_tree_path_down(path);
		valid = gtk_tree_model_get_iter(GTK_TREE_MODEL(store),&myiter,path);
		gtk_tree_path_free(path);
	}
	while (valid) {
		gchar *name;
		gtk_tree_model_get (GTK_TREE_MODEL(store), &myiter, 1, &name, -1);

		if (!find_name_in_dir_entries(direntrylist, name)) {
			if (direntrylist == NULL && strcmp(name, ".")==0) {
#ifdef DEBUG_ADDING_TO_TREE
				DEBUG_MSG("refresh_dir_by_path_and_filename, not removing fake entry for empty \n");
#endif
				valid = gtk_tree_model_iter_next (GTK_TREE_MODEL(store), &myiter);
			} else {
				if (strcmp(name, ".")==0) {
					/* the dot entry is removed as last one, because the rows fail expanding if they are emptied on the way*/
					remove_dot_iter = myiter;
					valid = gtk_tree_model_iter_next (GTK_TREE_MODEL(store), &myiter);
					remove_dot = TRUE;
				} else {
					GtkTreeIter myiter2 = myiter;
					valid = gtk_tree_model_iter_next (GTK_TREE_MODEL(store), &myiter);
#ifdef DEBUG_ADDING_TO_TREE
					DEBUG_MSG("refresh_dir_by_path_and_filename, removing row for name %s\n", name);
#endif
					gtk_tree_store_remove(store,&myiter2);
				}
			}
		} else {
			valid = gtk_tree_model_iter_next (GTK_TREE_MODEL(store), &myiter);
		}
	}
	/* now add the new entries */
	{
		GList *tmplist;
		gtk_tree_model_get_iter(GTK_TREE_MODEL(store),&myiter,thispath);
		
		tmplist = g_list_first(direntrylist);
		while (tmplist) {
			Tdir_entry *entry = (Tdir_entry *)tmplist->data;
			if (!entry->has_widget) {
				GtkTreeIter myiter2;
#ifdef DEBUG_ADDING_TO_TREE
				DEBUG_MSG("refresh_dir_by_path_and_filename, adding row for name=%s\n", entry->name);
#endif
				myiter2 = add_tree_item(&myiter, store, entry->name, entry->type, entry->icon);
				if (entry->type == TYPE_DIR) {
#ifdef DEBUG_ADDING_TO_TREE
					DEBUG_MSG("refresh_dir_by_path_and_filename, %s is TYPE_DIR so we setup the fake item\n", entry->name);
#endif
					add_tree_item(&myiter2, store, ".", TYPE_FILE, entry->icon);
				}
			}
			tmplist = g_list_next(tmplist);
		}
	}
	if (remove_dot) {
		gtk_tree_store_remove(store,&remove_dot_iter);
	}
	free_dir_entries(direntrylist);
	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(filebrowser.tree));
}

static GtkTreePath *build_tree_from_path(GtkTreeStore *store, gchar *filepath) {
	GtkTreeIter iter;
	/* first build path from root to here */

	{
	gchar *tmpstr, *p;
	gint totlen, curlen, prevlen=1;
	
	if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter)) {
		iter = add_tree_item(NULL, store, "/", TYPE_DIR, NULL);
	}
	
	totlen = strlen(filepath);
	DEBUG_MSG("build_tree_from_path, totlen=%d\n", totlen);
	curlen = strlen(strchr(&filepath[prevlen], '/'));
	DEBUG_MSG("build_tree_from_path, curlen=%d\n", curlen);
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
		if (!get_iter_by_filename_from_parent(store, &iter, tmpstr)) {
			iter = add_tree_item(&iter, store, tmpstr, TYPE_DIR, NULL);
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
		direntrylist = return_dir_entries(dirname);
		tmplist = g_list_first(direntrylist);
		while (tmplist) {
			Tdir_entry *entry = (Tdir_entry *)tmplist->data;
			if (entry->type == TYPE_DIR) {
				GtkTreeIter tmp;
				tmp = add_tree_item(&iter, store, entry->name, TYPE_DIR, NULL);
				add_tree_item(&tmp, store, ".", TYPE_FILE, NULL);
			} else {
				add_tree_item(&iter, store, entry->name, TYPE_FILE, entry->icon);
			}
			tmplist = g_list_next(tmplist);
		}
		g_free(dirname);
		free_dir_entries(direntrylist);
	}
	return gtk_tree_model_get_path(GTK_TREE_MODEL(store),&iter);
}

/* I guess the dir must have a trailing slash in this function .. not sure */
void filebrowser_refresh_dir(gchar *dir) {
	/* get the path for this dir */
	GtkTreePath *path = return_path_from_filename(filebrowser.store, dir);
	DEBUG_DUMP_TREE_PATH(path);
	if (!path) return;
	/* check if the dir is expanded, return if not */	
	if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(filebrowser.tree), path)) {
		DEBUG_MSG("refresh_dir, it IS expanded\n");
		/* refresh it */
		refresh_dir_by_path_and_filename(GTK_TREE_STORE(filebrowser.store), path, dir);
	} else {
		DEBUG_MSG("refresh_dir, it is NOT expanded\n");
	}
	gtk_tree_path_free(path);
}

static GtkTreePath *filebrowser_path_up_multi(GtkTreePath *path, gint num) {
	while (num > 0) {
		gtk_tree_path_up(path);
		num--;
	}
	return path;
}

static void filebrowser_expand_to_root(const GtkTreePath *this_path) {
	gint num = gtk_tree_path_get_depth((GtkTreePath *)this_path);
	while (num >= 0) {
		GtkTreePath *path = gtk_tree_path_copy(this_path);
		path = filebrowser_path_up_multi(path, num);
		DEBUG_MSG("filebrowser_expand_to_root, expanding ");
		DEBUG_DUMP_TREE_PATH(path);
		gtk_tree_view_expand_row(GTK_TREE_VIEW(filebrowser.tree), path, FALSE);
		gtk_tree_path_free(path);
		num--;
	}
}

void filebrowser_open_dir(gchar *dir) {
	/* first check if the dir already exists */
	GtkTreePath *path = return_path_from_filename(filebrowser.store, dir);
	DEBUG_DUMP_TREE_PATH(path);
	if (path) {
		DEBUG_MSG("jump_to_dir, it exists in tree, refreshing\n");
		refresh_dir_by_path_and_filename(GTK_TREE_STORE(filebrowser.store), path, dir);
		DEBUG_MSG("jump_to_dir, now scroll to the path\n");
		filebrowser_expand_to_root(path);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(filebrowser.tree),path,0,TRUE,0.5,0.5);
	} else {
		DEBUG_MSG("jump_to_dir, it does NOT exist in the tree, building..\n");
		path = build_tree_from_path(GTK_TREE_STORE(filebrowser.store), dir);
/*		path = return_path_from_filename(GTK_TREE_STORE(filebrowser.store), dir);*/
/*		gtk_tree_view_expand_row(GTK_TREE_VIEW(filebrowser.tree),path,FALSE);*/
		filebrowser_expand_to_root(path);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(filebrowser.tree),path,0,TRUE,0.5,1.0);
	}
	gtk_tree_path_free(path);
}

static void dirmenu_activate_lcb(GtkWidget *widget, gchar *dir) {
	if (dir) {
		DEBUG_MSG("dirmenu_activate_lcb, dir=%s\n", dir);
		filebrowser_open_dir(dir);
	}
}

static void populate_dir_history(gboolean firsttime) {
	GtkWidget *menu;
	gchar *tmpchar, *tmpdir;
	GList *tmplist;
	GList *new_history_list=NULL;
	DEBUG_MSG("populate_dir_history, empty\n");

	if (!firsttime) {
		gtk_option_menu_remove_menu(GTK_OPTION_MENU(filebrowser.dirmenu));
		free_stringlist(filebrowser.dirmenu_entries);
		filebrowser.dirmenu_entries = NULL;
	
		menu = gtk_menu_new();
		gtk_option_menu_set_menu(GTK_OPTION_MENU(filebrowser.dirmenu), menu);
		gtk_widget_show(menu);
	} else {
		filebrowser.dirmenu_entries = NULL;
		menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(filebrowser.dirmenu));
		gtk_widget_show(menu);
	}

	tmpdir = g_strconcat(g_get_current_dir(), DIRSTR, NULL);
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
		/* dirname is not free-ed here, but it is added to the dirmenu_entries list
		 so it can be freed when there is a re-populate for the dirmenu */
		gchar *dirname = ending_slash((gchar *)tmplist->data);
		DEBUG_MSG("populate_dir_history, adding %s to menu\n", dirname);
		menuitem = gtk_menu_item_new_with_label(dirname);
		g_signal_connect(G_OBJECT (menuitem), "activate",G_CALLBACK(dirmenu_activate_lcb),dirname);
		gtk_menu_append(GTK_MENU(menu), menuitem);
		filebrowser.dirmenu_entries = g_list_append(filebrowser.dirmenu_entries, dirname);
		gtk_widget_show(menuitem);
		g_free(tmplist->data);
		tmplist = g_list_next(tmplist);
	} 
	g_list_free(new_history_list);

	gtk_option_menu_set_history(GTK_OPTION_MENU(filebrowser.dirmenu), 0);
}

typedef struct {
	GtkWidget *win;
	GtkWidget *entry;
	gchar *basedir; /* must have a trailing slash */
	gint is_file;
} Tcfod;

static void create_file_or_dir_destroy_lcb(GtkWidget *widget, GdkEvent *event,Tcfod *ws) {
	window_destroy(ws->win);
	g_free(ws->basedir);
	g_free(ws);
}

static void create_file_or_dir_cancel_clicked_lcb(GtkWidget *widget, Tcfod *ws) {
	create_file_or_dir_destroy_lcb(NULL, NULL, ws);
}

static void create_file_or_dir_ok_clicked_lcb(GtkWidget *widget, Tcfod *ws) {
	gchar *newname, *name;

	name = gtk_editable_get_chars(GTK_EDITABLE(ws->entry), 0, -1);	
	if (name) {
		if (strlen(name)) {
			newname = g_strconcat(ws->basedir,name, NULL);
			DEBUG_MSG("create_file_or_dir_ok_clicked_lcb, newname=%s\n", newname);
			if (file_exists_and_readable(newname)) {
				error_dialog(_("Error creating path"),_("Path exists already"));
			} else {
				if (ws->is_file) {
					doc_new_with_new_file(newname);
				} else {
					if(mkdir(newname, 0755)== -1) {
/*						error_dialog(_("Error creating directory"),strerror(errno));*/
					}
				}
			}
			g_free(newname);
			DEBUG_MSG("create_file_or_dir_ok_clicked_lcb, refreshing basedir %s\n", ws->basedir);
			filebrowser_refresh_dir(ws->basedir);
		}
		g_free(name);
	}
	create_file_or_dir_destroy_lcb(NULL, NULL, ws);
}

static GtkTreePath * filebrowser_get_path_from_selection(GtkTreeIter *iter) {
	GtkTreeModel *model = GTK_TREE_MODEL(filebrowser.store);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(filebrowser.tree));
	GtkTreeIter localiter;
	GtkTreeIter *usethisiter = (iter != NULL) ? iter : &localiter;
	if (gtk_tree_selection_get_selected(selection,&model,usethisiter)) {
		return gtk_tree_model_get_path(GTK_TREE_MODEL(filebrowser.store),usethisiter);
	}
	return NULL;
}


static void create_file_or_dir_win(gint is_file) {
	GtkTreePath *path;
	GtkTreeIter iter;
	path = filebrowser_get_path_from_selection(&iter);
	if (path) {
		Tcfod *ws;
		gchar *title, *tmp;
		GtkWidget *vbox, *but, *hbox;
		GtkTreePath *path;

		ws = g_malloc(sizeof(Tcfod));
		if (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(filebrowser.store), &iter)) {
			DEBUG_MSG("create_file_or_dir_win, a dir is selected\n");
		} else {
			GtkTreeIter parent;
			DEBUG_MSG("create_file_or_dir_win, a file is selected\n");
			gtk_tree_model_iter_parent(GTK_TREE_MODEL(filebrowser.store), &parent, &iter);
			iter = parent;
		}
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(filebrowser.store),&iter);
		tmp = return_filename_from_path(filebrowser.store,path);
		ws->basedir = ending_slash(tmp);
		g_free(tmp);
		gtk_tree_path_free(path);
		
		if (is_file) {
			title = _("File name");
		} else {
			title = _("Directory name");
		}
		ws->is_file = is_file;

		ws->win = window_full(title, GTK_WIN_POS_MOUSE, 5,G_CALLBACK(create_file_or_dir_destroy_lcb), ws, TRUE);
		vbox = gtk_vbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(ws->win), vbox);
		ws->entry = boxed_entry_with_text(NULL, 250, vbox);
	
		hbox = gtk_hbutton_box_new();
		gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
		gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 1);
	
		but = bf_stock_ok_button(G_CALLBACK(create_file_or_dir_ok_clicked_lcb), ws);
		gtk_box_pack_start(GTK_BOX(hbox),but , FALSE, FALSE, 0);
	
		but = bf_stock_cancel_button(G_CALLBACK(create_file_or_dir_cancel_clicked_lcb), ws);
		gtk_box_pack_start(GTK_BOX(hbox),but , FALSE, FALSE, 0);
	
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
		gtk_widget_grab_focus(ws->entry);
		gtk_widget_show_all(GTK_WIDGET(ws->win));
	}
}

static void filebrowser_rpopup_new_file_lcb(GtkWidget *widget, gpointer data) {
	create_file_or_dir_win(TRUE);
}

static void filebrowser_rpopup_new_dir_lcb(GtkWidget *widget, gpointer data) {
	create_file_or_dir_win(FALSE);
}

static void filebrowser_rpopup_delete_lcb(GtkWidget *widget, gpointer data) {
	GtkTreePath *path;
	path = filebrowser_get_path_from_selection(NULL);
	if (path) {
		gchar *filename;
		filename = return_filename_from_path(filebrowser.store,path);
		gtk_tree_path_free(path);
		if (filename) {
			gchar *buttons[] = {GTK_STOCK_DELETE, GTK_STOCK_CANCEL, NULL};
			gchar *label;
			gint retval;
			label = g_strdup_printf(_("Are you sure you want to\ndelete \"%s\" ?"), filename);
			retval = multi_stockbutton_dialog(_("delete file?"), 1, label, buttons);
			g_free(label);
			if (retval == 0) {
				gchar *tmp, *dir;
				DEBUG_MSG("file_list_rpopup_file_delete %s\n", filename);
				unlink(filename);
				tmp = g_path_get_dirname(filename);
				dir = ending_slash(tmp);
				g_free(tmp);
				filebrowser_refresh_dir(dir);
				g_free(dir);
				
			}
			g_free(filename);
		}
		
	}
}

static void filebrowser_rpopup_refresh_lcb(GtkWidget *widget, gpointer data) {
	GtkTreePath *path;
	GtkTreeIter iter;
	path = filebrowser_get_path_from_selection(&iter);
	if (path) {
		gchar *tmp, *dir;
		if (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(filebrowser.store), &iter)) {
			DEBUG_MSG("create_file_or_dir_win, a dir is selected\n");
		} else {
			GtkTreeIter parent;
			DEBUG_MSG("create_file_or_dir_win, a file is selected\n");
			gtk_tree_model_iter_parent(GTK_TREE_MODEL(filebrowser.store), &parent, &iter);
			iter = parent;
		}
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(filebrowser.store),&iter);
		tmp = return_filename_from_path(filebrowser.store,path);
		DEBUG_MSG("filebrowser_rpopup_refresh_lcb, return_filename_from_path returned %s\n", tmp);
		dir = ending_slash(tmp);
		DEBUG_MSG("filebrowser_rpopup_refresh_lcb, ending_slash returned %s\n", dir);
		filebrowser_refresh_dir(dir);
		g_free(tmp);
		g_free(dir);
		gtk_tree_path_free(path);
	}
}

static void filebrowser_rpopup_filter_toggled_lcb(GtkWidget *widget, Tfilter *filter) {
	if (GTK_CHECK_MENU_ITEM(widget)->active) {
		DEBUG_MSG("filebrowser_rpopup_filter_toggled_lcb, setting curfilter to (%p) %s\n", filter, filter->name);
		filebrowser.curfilter = filter;
		if (main_v->props.last_filefilter) g_free(main_v->props.last_filefilter);
		main_v->props.last_filefilter = g_strdup(filter->name);
		filebrowser_rpopup_refresh_lcb(widget, NULL);
	}
}


static GtkWidget *filebrowser_rpopup_create_menu() {
	GtkWidget *menu, *menu_item;

	menu = gtk_menu_new();
	menu_item = gtk_menu_item_new_with_label(_("New file"));
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(filebrowser_rpopup_new_file_lcb), NULL);
	gtk_menu_append(GTK_MENU(menu), menu_item);
	menu_item = gtk_menu_item_new_with_label(_("New directory"));
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(filebrowser_rpopup_new_dir_lcb), NULL);
	gtk_menu_append(GTK_MENU(menu), menu_item);
	menu_item = gtk_menu_item_new_with_label(_("Delete file"));
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(filebrowser_rpopup_delete_lcb), NULL);
	gtk_menu_append(GTK_MENU(menu), menu_item);
	menu_item = gtk_menu_item_new_with_label(_("Refresh"));
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(filebrowser_rpopup_refresh_lcb), NULL);
	gtk_menu_append(GTK_MENU(menu), menu_item);
	menu_item = gtk_menu_item_new_with_label(_("Filter"));
	gtk_menu_append(GTK_MENU(menu), menu_item);
	{
		GtkWidget *fmenu;
		GSList *group=NULL;
		GList *tmplist;
		fmenu = gtk_menu_new();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), fmenu);
		tmplist = g_list_first(filebrowser.filters);
		while (tmplist) {
			Tfilter *filter = (Tfilter *)tmplist->data;
			menu_item = gtk_radio_menu_item_new_with_label(group, filter->name);
			if (filebrowser.curfilter == filter) {
				gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(menu_item), TRUE);
			}
			g_signal_connect(GTK_OBJECT(menu_item), "toggled", G_CALLBACK(filebrowser_rpopup_filter_toggled_lcb), filter);
			if (!group) {
				group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menu_item));
			}
			gtk_menu_append(GTK_MENU(fmenu), menu_item);
			tmplist = g_list_next(tmplist);
		}
	}

	gtk_widget_show_all(menu);
	return menu;
}

static void renderer_edited_lcb(GtkCellRendererText *cell,const gchar *path_string,const gchar *new_text,GtkTreeStore *store) {
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	gchar *newfilename, *oldfilename, *basedir, *tmp;

	oldfilename = return_filename_from_path(store, path);
	tmp = g_path_get_dirname(oldfilename);
	basedir = ending_slash(tmp);
	g_free(tmp);
	newfilename = g_strconcat(basedir, new_text, NULL);

	if (strcmp(oldfilename, newfilename)!=0) {
		DEBUG_MSG("text changed, oldfilename=%s, newfilename=%s\n", oldfilename, newfilename);
		if (rename(oldfilename, newfilename) == 0) {
			gint index;
			index = documentlist_return_index_from_filename(oldfilename);
			if (index >= 0) {
				Tdocument *doc = (Tdocument *)g_list_nth_data(main_v->documentlist, index);
				g_free(doc->filename);
				doc->filename = g_strdup(newfilename);
				doc->modified = 1 - doc->modified; /* force the update of the labels */
				doc_set_modified(doc, 1 - doc->modified);
			}
			filebrowser_refresh_dir(basedir);
		}
	}
	g_free(basedir);
	g_free(oldfilename);
	g_free(newfilename);
	gtk_tree_path_free(path);
}

static void row_expanded_lcb(GtkTreeView *tree,GtkTreeIter *iter,GtkTreePath *path,GtkTreeStore *store) {
	gchar *filename = return_filename_from_path(store,path);
	DEBUG_MSG("row_expanded_lcb, started on filename=%s\n", filename);
	refresh_dir_by_path_and_filename(store, path, filename);
	g_free(filename);
	DEBUG_MSG("row_expanded_lcb, finished\n");
}

static void row_activated_lcb(GtkTreeView *tree, GtkTreePath *path,GtkTreeViewColumn *column, GtkTreeStore *store) {
	GtkTreeIter iter;
	gchar *filename = return_filename_from_path(store,path);
	DEBUG_MSG("row_activated_lcb, filename=%s\n", filename);
	/* test if this is a dir */
	gtk_tree_model_get_iter(GTK_TREE_MODEL(store),&iter,path);
	if (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(store),&iter)) {
		if (gtk_tree_view_row_expanded(tree, path)) {
			gtk_tree_view_collapse_row(tree,path);
		} else {
			gtk_tree_view_expand_row(tree,path,FALSE);
		}
	} else {
		doc_new_with_file(filename, FALSE);
	}
	g_free(filename);
	DEBUG_MSG("row_activated_lcb, finished\n");
}

static gboolean filebrowser_button_press_lcb(GtkWidget *widget, GdkEventButton *event, gpointer data) {
	DEBUG_MSG("filebrowser_button_press_lcb, button=%d\n",event->button);
	if (event->button == 3) {
		GtkWidget *menu = filebrowser_rpopup_create_menu();
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
	}
	if (event->button==1 && event->type == GDK_2BUTTON_PRESS) {
		GtkTreePath *path = filebrowser_get_path_from_selection(NULL);
		DEBUG_MSG("doubleclick!! open the file!!\n");
		if (path) {
			row_activated_lcb(GTK_TREE_VIEW(filebrowser.tree), path,NULL,GTK_TREE_STORE(filebrowser.store));
		}
		gtk_tree_path_free(path);
		return TRUE;
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

void filebrowser_filters_rebuild() {
	GList *tmplist;

	tmplist = g_list_first(filebrowser.filters);
	while (tmplist) {
		filter_destroy(tmplist->data);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(filebrowser.filters);
	filebrowser.filters = NULL;

	filebrowser.filetypes_with_icon = NULL;
	tmplist = g_list_first(main_v->filetypelist);
	while (tmplist) {
		if (((Tfiletype *)tmplist->data)->icon) {
			filebrowser.filetypes_with_icon = g_list_append(filebrowser.filetypes_with_icon, tmplist->data);
		}
		tmplist = g_list_next(tmplist);
	}
	
	filebrowser.curfilter = new_filter(_("All files"), "0", NULL);
	filebrowser.filters = g_list_append(NULL, filebrowser.curfilter);
	
	tmplist = g_list_first(main_v->props.filefilters);
	while (tmplist) {
		gchar **strarr = (gchar **) tmplist->data;
		if (count_array(strarr) == 3) {
			Tfilter *filter = new_filter(strarr[0], strarr[1], strarr[2]);
			filebrowser.filters = g_list_append(filebrowser.filters, filter);
			if (strcmp(filter->name, main_v->props.last_filefilter)==0) {
				filebrowser.curfilter = filter;
			}
		}
		tmplist = g_list_next(tmplist);
	}
}

GtkWidget *filebrowser_init() {
	if (!filebrowser.curfilter) {
		gchar *filename;
		filebrowser.uid = getuid();

		filename = return_first_existing_filename(main_v->props.filebrowser_unknown_icon,
						"icon_unknown.png","../icons/icon_unknown.png",
						"icons/icon_unknown.png",NULL);
	
		filebrowser.unknown_icon = gdk_pixbuf_new_from_file(filename, NULL);
		
		filename = return_first_existing_filename(main_v->props.filebrowser_dir_icon,
						"icon_dir.png","../icons/icon_dir.png",
						"icons/icon_dir.png",NULL);

		filebrowser.dir_icon = gdk_pixbuf_new_from_file(filename, NULL);

		if (!filebrowser.dir_icon || !filebrowser.unknown_icon) {
			g_print("the dir_icon and unknown_icon items in the config file are invalid\n");
			return gtk_label_new("cannot load icons");
		}

		filebrowser_filters_rebuild();

	}
	
	filebrowser.store = gtk_tree_store_new (N_COLUMNS,GDK_TYPE_PIXBUF,G_TYPE_STRING);
	filebrowser.tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL(filebrowser.store));
	/* The view now holds a reference.  We can get rid of our own reference */
	g_object_unref(G_OBJECT(filebrowser.store));

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(filebrowser.tree), FALSE);

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
		gtk_tree_view_append_column (GTK_TREE_VIEW(filebrowser.tree), column);
		
		renderer = gtk_cell_renderer_text_new();
/*		column = gtk_tree_view_column_new ();*/
		g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
/*		g_object_set(G_OBJECT(renderer), "xpad", 1, NULL);
		g_object_set(G_OBJECT(renderer), "ypad", 1, NULL);*/
		g_signal_connect(G_OBJECT(renderer), "edited", G_CALLBACK(renderer_edited_lcb), filebrowser.store);	
		gtk_tree_view_column_pack_start (column, renderer, TRUE);
		gtk_tree_view_column_set_attributes(column,renderer
			,"text", FILENAME_COLUMN,NULL);
/*		gtk_tree_view_column_set_spacing(column, 1);
		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);*/
/*		gtk_tree_view_append_column(GTK_TREE_VIEW(filebrowser.tree), column);*/
	}
	{
		gchar curdir[1024];
		GtkTreePath *path;
		
		getcwd(curdir, 1023);
		strncat(curdir, "/", 1023);
		DEBUG_MSG("curdir=%s\n",curdir);
		path = build_tree_from_path(GTK_TREE_STORE(filebrowser.store), curdir);
		filebrowser_expand_to_root(path);
	}
	
	g_signal_connect(G_OBJECT(filebrowser.tree), "row-expanded", G_CALLBACK(row_expanded_lcb), filebrowser.store);
	g_signal_connect(G_OBJECT(filebrowser.tree), "row-activated",G_CALLBACK(row_activated_lcb),filebrowser.store);
	g_signal_connect(G_OBJECT(filebrowser.tree), "button_press_event",G_CALLBACK(filebrowser_button_press_lcb),filebrowser.store);

	{
		GtkWidget *vbox, *scrolwin;
	/*	GtkAdjustment* adj;*/
		vbox = gtk_vbox_new(FALSE, 0);
		
		filebrowser.dirmenu = gtk_option_menu_new();
		gtk_option_menu_set_menu(GTK_OPTION_MENU(filebrowser.dirmenu), gtk_menu_new());
		populate_dir_history(TRUE);
		gtk_box_pack_start(GTK_BOX(vbox),filebrowser.dirmenu, FALSE, FALSE, 0);
		
		scrolwin = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_widget_set_size_request(scrolwin, main_v->props.left_panel_width, -1);
		gtk_container_add(GTK_CONTAINER(scrolwin), filebrowser.tree);
		gtk_box_pack_start(GTK_BOX(vbox), scrolwin, TRUE, TRUE, 0);
/*		adj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolwin));
		gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), GTK_ADJUSTMENT(adj)->lower);
		adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolwin));
		gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), GTK_ADJUSTMENT(adj)->lower);*/
		return vbox;
	}
}

void filebrowser_scroll_initial() {
	if (filebrowser.tree) {
		gtk_tree_view_scroll_to_point(GTK_TREE_VIEW(filebrowser.tree), 2000, 2000);
	}
}
