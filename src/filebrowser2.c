/* Bluefish HTML Editor
 * filebrowser2.c - the filebrowser v2
 *
 * Copyright (C) 2002-2011 Olivier Sessink
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
 */

/*#define DEBUG*/

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
#include "bf_lib.h"
#include "bfwin_uimanager.h"
#include "dialog_utils.h"
#include "document.h"
#include "file.h"
#include "filefilter.h"
#include "file_dialogs.h"
#include "gtk_easy.h"
#include "project.h"
#include "stringlist.h"			/* add_to_history_stringlist() */

typedef struct {
	GCancellable *cancel;
	GFile *uri;
	/*GFile *p_uri; */
	GFileEnumerator *gfe;
	GtkTreeIter *parent;
} Turi_in_refresh;

enum {
	PIXMAP_COLUMN,
	ICON_NAME_COLUMN,
	FILENAME_COLUMN,
	URI_COLUMN,
	REFRESH_COLUMN,
	TYPE_COLUMN,
	FILEINFO_COLUMN,
	N_COLUMNS
};

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

	GFile *basedir;
	Tfilter *curfilter;

	GtkTreeModel *dir_tfilter;
	GtkTreeModel *dir_tsort;

	GtkTreeModel *file_lfilter;
	GtkTreeModel *file_lsort;

	gboolean last_popup_on_dir;
	gboolean filebrowser_show_hidden_files;
	gboolean filebrowser_show_backup_files;
	gint filebrowser_viewmode;
	Tbfwin *bfwin;
} Tfilebrowser2;

#define FILEBROWSER2(var) ((Tfilebrowser2 *)(var))
static void dirmenu_set_curdir(Tfilebrowser2 * fb2, GFile * newcurdir);
static void refilter_filelist(Tfilebrowser2 * fb2, GtkTreePath * newroot);
static GFile *fb2_uri_from_fspath(Tfilebrowser2 * fb2, GtkTreePath * fs_path);
static void fb2_set_basedir_backend(Tfilebrowser2 * fb2, GFile * uri);
static void fb2_set_viewmode_widgets(Tfilebrowser2 * fb2, gint viewmode);
/**************/
#ifdef DEBUG
static void
debug_gfile(GFile * uri, gboolean newline)
{
	if (uri) {
		gchar *name = g_file_get_uri(uri);
		g_print("%s%s", name, newline ? "\n" : "");
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

static void
DEBUG_DIRITER(GtkTreeIter * diriter)
{
	gchar *name;
	GFile *uri;
	gtk_tree_model_get(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->filesystem_tstore), diriter,
					   FILENAME_COLUMN, &name, URI_COLUMN, &uri, -1);
	DEBUG_MSG("DEBUG_DIRITER, iter(%p) has filename %s, uri ", diriter, name);
	DEBUG_GFILE(uri, TRUE);
	g_free(name);
}

static void
DEBUG_TPATH(GtkTreeModel * model, GtkTreePath * path, gboolean newline)
{
	gchar *tname, *filename;
	GtkTreeIter iter;
	tname = gtk_tree_path_to_string(path);
	DEBUG_MSG("%s", tname);
	if (gtk_tree_model_get_iter(model, &iter, path)) {
		gtk_tree_model_get(model, &iter, FILENAME_COLUMN, &filename, -1);
	} else {
		filename = "NO VALID ITER";
	}
	DEBUG_MSG(" -> %s", filename);
	if (newline) {
		DEBUG_MSG("\n");
	}
	g_free(tname);
	g_free(filename);
}

#ifdef DBG_FBREFCOUNT
guint fake_finfo_ref = 0;
#endif

static void
uri_hash_destroy(gpointer data)
{
	g_object_unref((GObject *) data);
}

static void
iter_value_destroy(gpointer data)
{
	g_slice_free(GtkTreeIter, data);
}

/**************/
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

/**
 * fb2_get_uri_in_refresh:
 *
 */
static Turi_in_refresh *
fb2_get_uri_in_refresh(GFile * uri)
{
	GList *tmplist = g_list_first(FB2CONFIG(main_v->fb2config)->uri_in_refresh);
	while (tmplist) {
		if (((Turi_in_refresh *) tmplist->data)->uri == uri
			|| g_file_equal(((Turi_in_refresh *) tmplist->data)->uri, uri))
			return tmplist->data;
		tmplist = g_list_next(tmplist);
	}
	return NULL;
}

static void
fb2_uri_in_refresh_cleanup(Turi_in_refresh * uir)
{
	DEBUG_MSG("fb2_uri_in_refresh_cleanup, called for %p\n", uir);
	FB2CONFIG(main_v->fb2config)->uri_in_refresh =
		g_list_remove(FB2CONFIG(main_v->fb2config)->uri_in_refresh, uir);
	g_object_unref(uir->uri);
	/*g_object_unref(uir->p_uri); */
	g_object_unref(uir->cancel);
	g_slice_free(Turi_in_refresh, uir);
}

static GFileInfo *
fake_directory_fileinfo(const gchar * name)
{
	GFileInfo *finfo;
	finfo = g_file_info_new();
#ifdef DBG_FBREFCOUNT
	fake_finfo_ref++;
#endif
	g_file_info_set_display_name(finfo, name);
	g_file_info_set_name(finfo, name);
	g_file_info_set_edit_name(finfo, name);
	g_file_info_set_file_type(finfo, G_FILE_TYPE_DIRECTORY);
	g_file_info_set_attribute_string(finfo, G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE, DIR_MIME_TYPE);
	return finfo;
}

static GFileInfo *
fake_directory_fileinfo_for_uri(GFile * uri)
{
	GFileInfo *finfo;
	gchar *name = g_file_get_basename(uri);
	finfo = fake_directory_fileinfo(name);
	g_free(name);
	return finfo;
}

static gchar *
icon_name_from_icon(GIcon * icon)
{
	gchar *icon_name = NULL;
	if (icon && G_IS_THEMED_ICON(icon)) {
		GStrv names;

		g_object_get(icon, "names", &names, NULL);
		if (names && names[0]) {
			GtkIconTheme *icon_theme;
			int i;
			icon_theme = gtk_icon_theme_get_default();
			for (i = 0; i < g_strv_length(names); i++) {
				if (gtk_icon_theme_has_icon(icon_theme, names[i])) {
					icon_name = g_strdup(names[i]);
					break;
				}
			}
			g_strfreev(names);
		}
	} else {
		DEBUG_MSG("icon %p is not themed, use icon name 'folder'\n", icon);
		icon_name = g_strdup("folder");
	}
	return icon_name;
}

/**
 * fb2_add_filesystem_entry:
 *
 * returns the GtkTreeIter* that is stored in the hashtable. This can be a new iter pointer
 * if there was no iter in the hashtable yet, else it is the existing iter
 *
 */
static GtkTreeIter *
fb2_add_filesystem_entry(GtkTreeIter * parent, GFile * child_uri, GFileInfo * finfo, gboolean load_subdirs)
{
	GtkTreeIter *newiter;
	DEBUG_MSG("fb2_add_filesystem_entry(parent=%p, child_uri=%p, finfo=%p, load_subdirs=%d)\n",
			  parent, child_uri, finfo, load_subdirs);
	newiter = g_hash_table_lookup(FB2CONFIG(main_v->fb2config)->filesystem_itable, child_uri);
	if (newiter != NULL) {
		gboolean refresh;
		/* the child exists already, update the REFRESH column */
		DEBUG_MSG("entry exists with iter %p, update the 'refresh' column\n", newiter);
		gtk_tree_model_get(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->filesystem_tstore), newiter,
						   REFRESH_COLUMN, &refresh, -1);
		if (refresh != 0) {
			gtk_tree_store_set(GTK_TREE_STORE(FB2CONFIG(main_v->fb2config)->filesystem_tstore),
							   newiter, REFRESH_COLUMN, 0, -1);
		}
	} else {					/* child does not yet exist */
		gchar *display_name;
		gchar *mime_type;
#ifdef WIN32
		const gchar *cont_type;
#endif
		GIcon *icon;
		gchar *icon_name = NULL;
		newiter = g_slice_new0(GtkTreeIter);
		g_object_ref(child_uri);
		g_object_ref(finfo);
		display_name = gfile_display_name(child_uri, finfo);
#ifdef WIN32
		if (g_file_info_get_file_type(finfo) == G_FILE_TYPE_DIRECTORY) {
			cont_type = NULL;
			mime_type = g_strdup("inode/directory");
		} else {
			cont_type = g_file_info_get_attribute_string(finfo, G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE);
			mime_type = g_content_type_get_mime_type(cont_type);
		}
		/*g_print("display_name=%s, cont_type=%s, mime_type=%s\n",display_name,cont_type,mime_type); */
#else
		mime_type =
			(gchar *) g_file_info_get_attribute_string(finfo, G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE);
		if (mime_type == NULL && g_file_info_get_file_type(finfo) == G_FILE_TYPE_DIRECTORY) {
			/* GVFS SMB module on Ubuntu 8.10 returns NULL as FAST_CONTENT_TYPE, but it does set
			   the type (regular file or directory). In the case of a directory we manually set the 
			   mime type  */
			mime_type = "inode/directory";
		}
#endif
		icon = g_file_info_get_icon(finfo);
		icon_name = icon_name_from_icon(icon);
		/*gtk_tree_store_append(GTK_TREE_STORE(FB2CONFIG(main_v->fb2config)->filesystem_tstore),
		   newiter, parent);
		   gtk_tree_store_set(GTK_TREE_STORE(FB2CONFIG(main_v->fb2config)->filesystem_tstore), newiter,
		   ICON_NAME_COLUMN, icon_name, FILENAME_COLUMN, display_name, URI_COLUMN,
		   child_uri, REFRESH_COLUMN, 0, TYPE_COLUMN, mime_type, FILEINFO_COLUMN,
		   finfo, -1); */
		gtk_tree_store_insert_with_values(GTK_TREE_STORE(FB2CONFIG(main_v->fb2config)->filesystem_tstore),
										  newiter, parent, 0,
										  ICON_NAME_COLUMN, icon_name, FILENAME_COLUMN, display_name,
										  URI_COLUMN, child_uri, REFRESH_COLUMN, 0, TYPE_COLUMN, mime_type,
										  FILEINFO_COLUMN, finfo, -1);
		DEBUG_MSG("store %s in iter %p, parent %p\n", display_name, newiter, parent);
		g_free(icon_name);
#ifdef WIN32
		g_free(mime_type);
#endif
		DEBUG_MSG("insert newiter %p in hashtable for child_uri %p\n", newiter, child_uri);
		/* give it an extra reference for the hashtable */
		g_object_ref(child_uri);
		g_hash_table_insert(FB2CONFIG(main_v->fb2config)->filesystem_itable, child_uri, newiter);
		DEBUG_MSG("load_subdirs=%d, finfo=%p\n", load_subdirs, finfo);
		if (load_subdirs && g_file_info_get_file_type(finfo) == G_FILE_TYPE_DIRECTORY) {
			GFile *dummy_uri;
			GFileInfo *dummy_finfo;
			/* add a dummy item so the expander will be shown */
			dummy_uri = g_file_get_child(child_uri, " ");
			dummy_finfo = fake_directory_fileinfo(" ");
			DEBUG_MSG("add dummy directory\n");
			fb2_add_filesystem_entry(newiter, dummy_uri, dummy_finfo, FALSE);
			g_object_unref(dummy_uri);
#ifdef DBG_FBREFCOUNT
			fake_finfo_ref--;
#endif
			g_object_unref(dummy_finfo);
		}
		g_free(display_name);
	}
	return newiter;
}

static void fb2_treestore_delete_children(GtkTreeStore * tstore, GtkTreeIter * iter,
										  gboolean only_when_refresh1);

static void
fb2_treestore_delete(GtkTreeStore * tstore, GtkTreeIter * iter)
{
	GFile *d_uri;
	GFileInfo *finfo;
	if (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(tstore), iter)) {
		fb2_treestore_delete_children(tstore, iter, FALSE);
	}
	gtk_tree_model_get(GTK_TREE_MODEL(tstore), iter, URI_COLUMN, &d_uri, FILEINFO_COLUMN, &finfo, -1);
	gtk_tree_store_remove(tstore, iter);
	/* remove from hash table too! */
	g_hash_table_remove(FB2CONFIG(main_v->fb2config)->filesystem_itable, d_uri);

	DEBUG_MSG("fb2_treestore_delete, unref d_uri %p and finfo %p\n", d_uri, finfo);
	g_object_unref(d_uri);
#ifdef DBG_FBREFCOUNT
	fake_finfo_ref--;
#endif
	g_object_unref(finfo);
}

static void
fb2_treestore_delete_children(GtkTreeStore * tstore, GtkTreeIter * iter, gboolean only_when_refresh1)
{
	GtkTreeIter child;
	if (gtk_tree_model_iter_children(GTK_TREE_MODEL(tstore), &child, iter)) {
		gboolean cont = TRUE;
		while (cont) {
			gboolean do_delete = !only_when_refresh1;
			GtkTreeIter this = child;
			if (only_when_refresh1) {
				gboolean refresh;
				gtk_tree_model_get(GTK_TREE_MODEL(tstore), &child, REFRESH_COLUMN, &refresh, -1);
				do_delete = (refresh == 1);
			}
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(tstore), &child);
			if (do_delete) {
				/* delete 'this' ! */
				fb2_treestore_delete(tstore, &this);
			}
		}
	}

}

/**
 * fb2_treestore_delete_children_refresh1
 *
 * deletes all children of 'iter' that have value 1 in the REFRESH_COLUMN
 */
static void
fb2_treestore_delete_children_refresh1(GtkTreeStore * tstore, GtkTreeIter * iter)
{
	fb2_treestore_delete_children(tstore, iter, TRUE);
}

/**
 * fb2_treestore_mark_children_refresh1:
 *
 * sets value 1 in REFRESH_COLUMN for all children of 'iter' 
 */
static void
fb2_treestore_mark_children_refresh1(GtkTreeStore * tstore, GtkTreeIter * iter)
{
	GtkTreeIter child;
	DEBUG_MSG("fb2_treestore_mark_children_refresh1, started for model=%p, mark children of iter %p\n",
			  tstore, iter);
	if (gtk_tree_model_iter_children(GTK_TREE_MODEL(tstore), &child, iter)) {
		gboolean cont = TRUE;
		while (cont) {
			gtk_tree_store_set(GTK_TREE_STORE(tstore), &child, REFRESH_COLUMN, 1, -1);
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(tstore), &child);
		}
	}
	DEBUG_MSG("fb2_treestore_mark_children_refresh1, finished for model=%p\n", tstore);
}

static void
fb2_enumerator_close_lcb(GObject * source_object, GAsyncResult * res, gpointer user_data)
{
	Turi_in_refresh *uir = user_data;
	GError *error = NULL;
	g_file_enumerator_close_finish(uir->gfe, res, &error);
	g_object_unref(uir->gfe);
	fb2_treestore_delete_children_refresh1(FB2CONFIG(main_v->fb2config)->filesystem_tstore, uir->parent);
	fb2_uri_in_refresh_cleanup(uir);
}

static void
fb2_enumerate_next_files_lcb(GObject * source_object, GAsyncResult * res, gpointer user_data)
{
	Turi_in_refresh *uir = user_data;
	GError *error = NULL;
	GList *list, *tmplist;
	DEBUG_MSG("fb2_enumerate_next_files_lcb, started for uir %p\n", uir);
	list = g_file_enumerator_next_files_finish(uir->gfe, res, &error);
	if (error) {
		g_warning("ERROR: unhandled error %d in fb2_enumerate_next_files_lcb(): %s\n", error->code,
				  error->message);
		return;
	}
#ifdef DEBUG
	g_print("fb2_enumerate_next_files_lcb, number of results=%d\n", g_list_length(list));
#endif
	if (list == NULL) {
		/* done */
		g_file_enumerator_close_async(uir->gfe, G_PRIORITY_LOW, uir->cancel, fb2_enumerator_close_lcb, uir);
		return;
	}
	tmplist = g_list_first(list);
	while (tmplist) {
		GFileInfo *finfo = tmplist->data;
#ifdef DBG_FBREFCOUNT
		fake_finfo_ref++;
#endif
		if (g_file_info_has_attribute(finfo, G_FILE_ATTRIBUTE_STANDARD_NAME)) {
			const gchar *name;
			GFile *newchild;
			name = g_file_info_get_name(finfo);
			newchild = g_file_get_child(uir->uri, name);
			DEBUG_MSG("found newchild %p ", newchild);
			DEBUG_GFILE(newchild, TRUE);
			fb2_add_filesystem_entry(uir->parent, newchild, finfo, TRUE);
			g_object_unref(newchild);
		} else {
			DEBUG_MSG
				("fb2_enumerate_next_files_lcb, weird, finfo=%p does not have attribute name ???\n", finfo);
		}
#ifdef DBG_FBREFCOUNT
		fake_finfo_ref--;
#endif
		g_object_unref(finfo);
		tmplist = g_list_next(tmplist);
	}
	/* BUG: do error handling */
	g_list_free(list);

#ifdef DEBUG
	g_print("fb2_enumerate_next_files_lcb, done\n");
#endif
	g_file_enumerator_next_files_async(uir->gfe, 40, G_PRIORITY_LOW, uir->cancel,
									   fb2_enumerate_next_files_lcb, uir);
}

static void
fb2_enumerate_children_lcb(GObject * source_object, GAsyncResult * res, gpointer user_data)
{
	Turi_in_refresh *uir = user_data;
	GError *error = NULL;
	uir->gfe = g_file_enumerate_children_finish(uir->uri, res, &error);
	if (error) {
		/* delete the directory from the treestore */
		GtkTreeIter *iter = g_hash_table_lookup(FB2CONFIG(main_v->fb2config)->filesystem_itable, uir->uri);
		if (iter) {
			fb2_treestore_delete(FB2CONFIG(main_v->fb2config)->filesystem_tstore, iter);
		}
		g_warning("failed to list directory in filebrowser: %s\n", error->message);
		g_error_free(error);
		fb2_uri_in_refresh_cleanup(uir);
		return;
	}
	if (uir->gfe) {
		g_file_enumerator_next_files_async(uir->gfe, 40, G_PRIORITY_LOW, uir->cancel,
										   fb2_enumerate_next_files_lcb, uir);
	}
}

static gboolean
fb2_fill_dir_async_low_priority(gpointer data)
{
	Turi_in_refresh *uir = data;
/*	g_print("start fill dir async low priority\n");*/
	g_file_enumerate_children_async(uir->uri,
									"standard::name,standard::display-name,standard::fast-content-type,standard::icon,standard::edit-name,standard::is-backup,standard::is-hidden,standard::type",
									G_FILE_QUERY_INFO_NONE, G_PRIORITY_LOW, uir->cancel,
									fb2_enumerate_children_lcb, uir);
	return FALSE;
}

static void
fb2_fill_dir_async(GtkTreeIter * parent, GFile * uri)
{
	if (fb2_get_uri_in_refresh(uri) == NULL) {
		Turi_in_refresh *uir;

		fb2_treestore_mark_children_refresh1(FB2CONFIG(main_v->fb2config)->filesystem_tstore, parent);
		uir = g_slice_new0(Turi_in_refresh);
		uir->parent = parent;
		uir->uri = uri;
		g_object_ref(uir->uri);
		DEBUG_MSG("fb2_fill_dir_async, opening ");
		DEBUG_GFILE(uir->uri, TRUE);
		uir->cancel = g_cancellable_new();
		/*g_file_enumerate_children_async(uir->uri,
		   "standard::name,standard::display-name,standard::fast-content-type,standard::icon,standard::edit-name,standard::is-backup,standard::is-hidden,standard::type",
		   G_FILE_QUERY_INFO_NONE, G_PRIORITY_LOW, uir->cancel,
		   fb2_enumerate_children_lcb, uir); */
		FB2CONFIG(main_v->fb2config)->uri_in_refresh =
			g_list_prepend(FB2CONFIG(main_v->fb2config)->uri_in_refresh, uir);
		g_idle_add_full(G_PRIORITY_LOW, fb2_fill_dir_async_low_priority, uir, NULL);
	}
}

/**
 * fb2_uri_from_iter:
 *
 * reads the uri stored in the treestore for iter 'iter'
 */
static GFile *
fb2_uri_from_iter(GtkTreeIter * iter)
{
	GFile *uri;
	gtk_tree_model_get(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->filesystem_tstore), iter,
					   URI_COLUMN, &uri, -1);
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
 * this function does call fb2_fill_dir_async() which changes the treestore and 
 * thus might invalidates iters
 *
 */
static void
fb2_refresh_dir(GFile * uri, GtkTreeIter * dir)
{
	/* first we mark all children as 'refresh=1' in the REFRESH_COLUMN, then
	   we read the directory, and all existing items are set to refresh=0, after we
	   have finished reading the directory, all items that still have refresh=1 can be 
	   deleted */
	if (uri != NULL && dir == NULL) {
		dir = g_hash_table_lookup(FB2CONFIG(main_v->fb2config)->filesystem_itable, uri);
	}
	if (dir != NULL && uri == NULL) {
		uri = fb2_uri_from_iter(dir);
	}
	if (dir && uri) {
		g_object_ref(uri);
		fb2_fill_dir_async(dir, uri);
		g_object_unref(uri);
	}
}

/**
 * fb2_refresh_dir_from_uri:
 * @dir: the uri to refresh
 *
 * can be used to make the filebrowser refresh a certain directory
 * from external (other then filebrowser) code.
 *
 * will not refresh if this directory does not yet exist in the filebrowser !!
 */
void
fb2_refresh_dir_from_uri(GFile * dir)
{
	fb2_refresh_dir(dir, NULL);
}

/**
 * fb2_refresh_parent_of_uri:
 *
 * convenience function, will refresh the parent directory of child_uri
 */
void
fb2_refresh_parent_of_uri(GFile * child_uri)
{
	GFile *parent_uri;
	DEBUG_MSG("fb2_refresh_parent_of_uri, started for %p\n", child_uri);
	parent_uri = g_file_get_parent(child_uri);
	if (parent_uri) {
		fb2_refresh_dir(parent_uri, NULL);
		g_object_unref(parent_uri);
	}
}

static gchar *
get_toplevel_name(GFile * uri)
{
	GError *error = NULL;
	GMount *mount;
	gchar *name;
	mount = g_file_find_enclosing_mount(uri, NULL, &error);
	if (!error && mount) {
		name = g_mount_get_name(mount);
		g_object_unref(mount);
	} else {
		name = g_file_get_basename(uri);
		if (error)
			g_error_free(error);
	}
	return name;
}

/**
 * fb2_build_dir:
 *
 * builds directories up to uri (uri should point to a directory, not to a file)
 *
 * returns a pointer to the iter stored in the hashtable, so this is a persistent iter
 */
static GtkTreeIter *
fb2_build_dir(GFile * uri)
{
	GFile *tmp, *parent_uri = NULL;
	GtkTreeIter *parent = NULL;
	tmp = uri;

	if (!uri)
		return NULL;

	DEBUG_MSG("fb2_build_dir(uri=%p)\n", uri);
	/* first find if any directory part of this uri exists already in the treestore */
	g_object_ref(tmp);

	while (parent == NULL) {
		GFile *tmp2 = g_file_get_parent(tmp);
		if (tmp2 == NULL) {
			GFileInfo *finfo;
			gchar *name;
			/*name = g_file_get_basename(tmp); */
			name = get_toplevel_name(tmp);
			/* there was no parent for this filesystem yet */
			finfo = fake_directory_fileinfo(name);
			parent = fb2_add_filesystem_entry(NULL, tmp, finfo, FALSE);
			g_free(name);
			break;
		} else {
			g_object_unref(tmp);
		}
		tmp = tmp2;
		parent = g_hash_table_lookup(FB2CONFIG(main_v->fb2config)->filesystem_itable, tmp);
	}							/* after this loop 'tmp' is newly allocated */
	parent_uri = tmp;

#ifdef DEBUG
	if (parent_uri == NULL) {
		g_critical("parent_uri should not be NULL\n");
		g_return_val_if_reached(NULL);
	}
#endif
	if (parent_uri) {			/* now loop to create all directories in the treestore that were not in the treestore yet */
		gboolean done = g_file_equal(parent_uri, uri);
		while (!done) {
			GFile *tmp2 = uri;
			GFileInfo *finfo;

			g_object_ref(tmp2);	/* both 'parent_uri'='tmp' and 'tmp2' are newly allocated */
			while (!gfile_uri_is_parent(parent_uri, tmp2, FALSE)) {
				GFile *tmp3 = g_file_get_parent(tmp2);
				if (!tmp3) {
					g_critical("uh-oh: fb2_build_dir, tried to get parent for %s, parent_uri=%s, uri=%s\n",
							   g_file_get_uri(tmp2), g_file_get_uri(parent_uri), g_file_get_uri(uri));
					g_return_val_if_reached(0);	/* TODO: must be g_return_val_if_reached() or handle the situation */
				}
				g_object_unref(tmp2);
				tmp2 = tmp3;
			}					/* after this loop both 'parent_uri'='tmp' and 'tmp2' are newly allocated */

			finfo = fake_directory_fileinfo_for_uri(tmp2);
			parent = fb2_add_filesystem_entry(parent, tmp2, finfo, g_file_equal(tmp2, uri));
			g_object_unref(finfo);
			g_object_unref(parent_uri);
			parent_uri = tmp2;	/* here 'parent_uri'='tmp2' is newly allocated */
			DEBUG_MSG("new parent_uri=");
			DEBUG_GFILE(parent_uri, FALSE);
			DEBUG_MSG(", requested uri=");
			DEBUG_GFILE(uri, TRUE);
			if (g_file_equal(parent_uri, uri)) {
				DEBUG_MSG("exit loop\n");
				done = TRUE;
			}
		}
		g_object_unref(parent_uri);	/* no memory leaks in the uri's... (I hope) */
	}
	return parent;
}

static GtkTreePath *
treepath_for_uri(Tfilebrowser2 * fb2, GFile * uri)
{
	if (uri) {
		GtkTreeIter *iter;
		iter = g_hash_table_lookup(FB2CONFIG(main_v->fb2config)->filesystem_itable, uri);
		if (!iter) {
			fb2_build_dir(uri);
			iter = g_hash_table_lookup(FB2CONFIG(main_v->fb2config)->filesystem_itable, uri);
		}
		return gtk_tree_model_get_path(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->filesystem_tstore), iter);
	}
	return NULL;
}


/**
 * fb2_focus_dir:
 *
 * builds the directory tree, refreshed the directory, and (if not noselect) expands and selects the 
 * result. during the expand it will block the expand signal handler, so the expand callback
 * will not be called
 *
 */
static void
fb2_focus_dir(Tfilebrowser2 * fb2, GFile * uri, gboolean noselect)
{
	GtkTreeIter *dir;
	DEBUG_MSG("fb2_focus_dir(fb2=%p, uri=%p, noselect=%d)", fb2, uri, noselect);
	if (!uri) {
		DEBUG_MSG("fb2_focus_dir, WARNING, CANNOT FOCUS WITHOUT URI\n");
		return;
	}
	g_object_ref(uri);
	if (fb2->filebrowser_viewmode == viewmode_flat) {
		fb2_set_basedir_backend(fb2, uri);
	} else {
		if (fb2->basedir) {
			if (!gfile_uri_is_parent(fb2->basedir, uri, TRUE)
				&& !g_file_equal(fb2->basedir, uri)) {
				fb2_set_basedir_backend(fb2, NULL);
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
		/* set this directory as the top tree for the file widget */
		fs_path =
			gtk_tree_model_get_path(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->filesystem_tstore), dir);
		if (fs_path) {
			refilter_filelist(fb2, fs_path);
			DEBUG_MSG("fb2_focus_dir, fb2=%p, expand dir tree to this dir..\n", fb2);
			if (!noselect && fb2->filebrowser_viewmode != viewmode_flat) {
				GtkTreePath *filter_path =
					gtk_tree_model_filter_convert_child_path_to_path(GTK_TREE_MODEL_FILTER
																	 (fb2->dir_tfilter), fs_path);
				if (filter_path) {
					GtkTreePath *sort_path =
						gtk_tree_model_sort_convert_child_path_to_path(GTK_TREE_MODEL_SORT(fb2->dir_tsort),
																	   filter_path);
					if (sort_path) {
						g_signal_handler_block(fb2->dir_v, fb2->expand_signal);
						gtk_tree_view_expand_to_path(GTK_TREE_VIEW(fb2->dir_v), sort_path);
						g_signal_handler_unblock(fb2->dir_v, fb2->expand_signal);
						DEBUG_MSG("fb2_focus_dir, selecting path\n");
						gtk_tree_selection_select_path(gtk_tree_view_get_selection
													   (GTK_TREE_VIEW(fb2->dir_v)), sort_path);
						gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(fb2->dir_v), sort_path, 0, TRUE, 0.5, 0.5);
						gtk_tree_path_free(sort_path);
					} else {
						DEBUG_MSG("fb2_focus_dir, no sort_path\n");
					}
					gtk_tree_path_free(filter_path);
				} else {
					DEBUG_MSG("fb2_focus_dir, no filter_path\n");
				}
			} else {
				/* 'uri' is not persistent, 'dir' is peristent, so only pass 'dir' 
				   the "select" or "expand" signal (dual or tree view) will also refresh this directory, so 
				   we call this only on 'noselect' or in the flat view
				 */
				DEBUG_MSG("fb2_focus_dir, noselect, so directly call refresh\n");
				fb2_refresh_dir(NULL, dir);
				if (fb2->filebrowser_viewmode == viewmode_flat) {
					dirmenu_set_curdir(fb2, uri);
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
	if (bfwin->fb2 && doc->uri) {
		GFile *dir_uri;
		/* first we make sure we have the correct directory open, then
		   we could select the document, but if the directory *was* open already, this
		   could disturb the user... hmmm... */
		dir_uri = g_file_get_parent(doc->uri);
#ifdef DEVELOPMENT
		if (!dir_uri) {
			g_critical("dir_uri should not be NULL\n");
			g_return_if_reached();
		}
#endif
		fb2_focus_dir(FILEBROWSER2(bfwin->fb2), dir_uri, FALSE);
		g_object_unref(dir_uri);
	}
}

/**
 * tree_model_filter_func
 *
 * will return TRUE if this file should be visible in the dir view
 */
static gboolean
tree_model_filter_func(GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
	Tfilebrowser2 *fb2 = data;
	gchar *name, *mime_type;
	gint len;
	GFile *uri;
	gboolean retval = TRUE;

	gtk_tree_model_get(GTK_TREE_MODEL(model), iter, FILENAME_COLUMN, &name, URI_COLUMN, &uri,
					   TYPE_COLUMN, &mime_type, -1);
	if (!name || !uri) {
		DEBUG_MSG
			("tree_model_filter_func, model=%p, fb2=%p, item without name!!, uri=%p, mime_type=%s\n",
			 model, fb2, uri, mime_type);
		return TRUE;
	}
	DEBUG_MSG("tree_model_filter_func, model=%p and fb2=%p, name=%s, mime=%s, and uri=", model, fb2,
			  name, mime_type);
	DEBUG_GFILE(uri, TRUE);
	if (!mime_type || !MIME_ISDIR(mime_type)) {	/* file */
		if (fb2->filebrowser_viewmode == viewmode_dual) {
			/* in the two paned view we don't show files in the dir view */
			retval = FALSE;
		} else {
			if (fb2->filebrowser_viewmode == viewmode_flat) {
				if (fb2->basedir) {
					if (!gfile_uri_is_parent(fb2->basedir, uri, FALSE)) {
						retval = FALSE;
					}
				}
			}
			if (retval && !fb2->filebrowser_show_backup_files) {
				len = strlen(name);
				if (len > 1 && (name[len - 1] == '~'))
					retval = FALSE;
			}
			if (retval && !fb2->filebrowser_show_hidden_files) {
				if (name[0] == '.')
					retval = FALSE;
			}
			if (fb2->basedir && !gfile_uri_is_parent(fb2->basedir, uri, TRUE)) {
				retval = FALSE;
			}
			if (retval && fb2->curfilter)
				retval = file_visible_in_filter(fb2->curfilter, mime_type, name);
		}
		DEBUG_MSG("file %s with mime_type %s: returning %d\n", name, mime_type, retval);
		g_free(name);
		g_free(mime_type);
		return retval;
	} else {					/* directory */
		if (fb2->filebrowser_viewmode == viewmode_flat) {
			if (fb2->basedir) {
				/* show only the level of our basedir no deeper directories */
				if (!gfile_uri_is_parent(fb2->basedir, uri, FALSE)) {
					retval = FALSE;
				}
			}
		} else {
			if (fb2->basedir) {
				/* show only our basedir on the root level, no other directories at that level */
				if (!gfile_uri_is_parent(fb2->basedir, uri, TRUE)
					&& !g_file_equal(fb2->basedir, uri)) {
					retval = FALSE;
				}
			}
		}
		if (retval && !fb2->filebrowser_show_hidden_files) {
			if (name[0] == '.')
				retval = FALSE;
		}
	}
	g_free(name);
	g_free(mime_type);
	DEBUG_MSG("returning %d\n", retval);
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
	gchar *name, *mime_type;
	gint len;
	gboolean retval = TRUE;
/*  DEBUG_MSG("file_list_filter_func, called for model=%p and fb2=%p\n",model,fb2);*/
	gtk_tree_model_get((GtkTreeModel *) model, iter, FILENAME_COLUMN, &name, TYPE_COLUMN, &mime_type, -1);
	if (!name)
		return FALSE;

	if (mime_type && MIME_ISDIR(mime_type))
		retval = FALSE;

	if (retval && !fb2->filebrowser_show_backup_files) {
		len = strlen(name);
		if (len > 1 && (name[len - 1] == '~'))
			retval = FALSE;
	}
	if (retval && !fb2->filebrowser_show_hidden_files) {
		if (name[0] == '.')
			retval = FALSE;
	}
	if (retval && fb2->curfilter)
		retval = file_visible_in_filter(fb2->curfilter, mime_type, name);
#ifdef DEBUG
	if (retval == FALSE) {
		g_print("file_list_filter_func, hiding %s (%s)\n", name, mime_type);
	}
#endif
	g_free(name);
	g_free(mime_type);

	return retval;
}

/*
 * filebrowser_sort_func
 *
 * this function is the sort function, and has the following  features:
 * - directories go before files
 * - files are first sorted without extension, only equal names are sorted by extension
 *
 */
gint
filebrowser_sort_func(GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b, gpointer user_data)
{
	gchar *namea, *nameb, *mimea, *mimeb;
	gboolean isdira, isdirb;
	gint retval = 0;
	gtk_tree_model_get((GtkTreeModel *) model, a, FILENAME_COLUMN, &namea, TYPE_COLUMN, &mimea, -1);
	gtk_tree_model_get((GtkTreeModel *) model, b, FILENAME_COLUMN, &nameb, TYPE_COLUMN, &mimeb, -1);
	isdira = (mimea && MIME_ISDIR(mimea));
	isdirb = (mimeb && MIME_ISDIR(mimeb));
	/*g_print("isdira=%d, mimea=%s, isdirb=%d, mimeb=%s\n",isdira,mimea,isdirb,mimeb); */
	if (isdira == isdirb) {		/* both files, or both directories */
		if (namea == nameb) {
			retval = 0;			/* both NULL */
		} else if (namea == NULL || nameb == NULL) {
			retval = (namea - nameb);
		} else {				/* sort by name, first without extension */
			gchar *dota, *dotb;
			dota = strrchr(namea, '.');
			dotb = strrchr(nameb, '.');
			if (dota)
				*dota = '\0';
			if (dotb)
				*dotb = '\0';
			retval = strcmp(namea, nameb);
			if (retval == 0) {
				if (dota)
					*dota = '.';
				if (dotb)
					*dotb = '.';
				retval = strcmp(namea, nameb);
			}
		}
	} else {					/* a directory and a file */
		retval = (isdirb - isdira);
	}
	g_free(namea);
	g_free(nameb);
	g_free(mimea);
	g_free(mimeb);
	return retval;
}

static void
add_uri_to_recent_dirs(Tfilebrowser2 * fb2, GFile * uri)
{
	gchar *tmp;

	tmp = g_file_get_uri(uri);

	fb2->bfwin->session->recent_dirs =
		add_to_history_stringlist(fb2->bfwin->session->recent_dirs, tmp, FALSE, TRUE);
	g_free(tmp);
}

/**
 * refilter_dirlist:
 *
 * will set the root of the directory view to 'newroot'
 *
 */
static void
refilter_dirlist(Tfilebrowser2 * fb2, GtkTreePath * newroot)
{
	GtkTreePath *useroot = NULL;
	if (fb2->basedir)
		g_object_unref(fb2->basedir);
	fb2->basedir = NULL;
	if (newroot) {
		/* to make it possible to select the root, we move the filter-root one up */
		useroot = gtk_tree_path_copy(newroot);
		if (fb2->filebrowser_viewmode == viewmode_flat) {
			fb2->basedir = fb2_uri_from_fspath(fb2, newroot);
			add_uri_to_recent_dirs(fb2, fb2->basedir);
			g_object_ref(fb2->basedir);
			DEBUG_MSG("refilter_dirlist, we use useroot as new root\n");
		} else {
			if (gtk_tree_path_get_depth(newroot) > 1 && gtk_tree_path_up(useroot)) {	/* do not set the root as basedir, it is useless  */
				GFile *uri;
				/* store this basedir in fb2 */
				uri = fb2_uri_from_fspath(fb2, newroot);
				g_object_ref(uri);
				fb2->basedir = uri;
				add_uri_to_recent_dirs(fb2, fb2->basedir);
			} else {
				DEBUG_MSG("there is no parent for this path, so we will set the basedir to NULL\n");
				gtk_tree_path_free(useroot);
				useroot = NULL;
			}
		}
	}
	fb2->dir_tfilter =
		gtk_tree_model_filter_new(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->filesystem_tstore), useroot);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(fb2->dir_tfilter),
										   tree_model_filter_func, fb2, NULL);

	fb2->dir_tsort = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(fb2->dir_tfilter));
	if (fb2->filebrowser_viewmode != viewmode_dual) {
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(fb2->dir_tsort), FILENAME_COLUMN,
										filebrowser_sort_func, NULL, NULL);
	}
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fb2->dir_tsort), FILENAME_COLUMN,
										 GTK_SORT_ASCENDING);

/*	fb2->dir_tsort = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(fb2->dir_tfilter));
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fb2->dir_tsort),FILENAME_COLUMN,GTK_SORT_ASCENDING);*/
	DEBUG_MSG("refilter_dirlist, connect dir_v to new sort(%p)&filter(%p) model\n", fb2->dir_tsort,
			  fb2->dir_tfilter);
	gtk_tree_view_set_model(GTK_TREE_VIEW(fb2->dir_v), GTK_TREE_MODEL(fb2->dir_tsort));
	/* we remove our reference, so the only reference is kept by the treeview, if the treeview is destroyed, the models will be destroyed */
	g_object_unref(fb2->dir_tfilter);
	g_object_unref(fb2->dir_tsort);
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
	DEBUG_MSG("refilter_filelist, started for fb2=%p, file_lfilter=%p, viewmode=%d\n", fb2,
			  fb2->file_lfilter, fb2->filebrowser_viewmode);
	if (fb2->filebrowser_viewmode == viewmode_dual) {
		if (fb2->file_lfilter) {
			GtkTreePath *curpath;
			gboolean equal;
			g_object_get(fb2->file_lfilter, "virtual-root", &curpath, NULL);
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
		fb2->file_lfilter =
			gtk_tree_model_filter_new(GTK_TREE_MODEL
									  (FB2CONFIG(main_v->fb2config)->filesystem_tstore), newroot);
		DEBUG_MSG("refilter_filelist, set file list filter func, fb2=%p\n", fb2);
		gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(fb2->file_lfilter),
											   file_list_filter_func, fb2, NULL);
		fb2->file_lsort = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(fb2->file_lfilter));
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(fb2->file_lsort), FILENAME_COLUMN,
										filebrowser_sort_func, NULL, NULL);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fb2->file_lsort), FILENAME_COLUMN,
											 GTK_SORT_ASCENDING);
		DEBUG_MSG("refilter_filelist, connect file_v to new sort(%p)&filter(%p) model\n",
				  fb2->file_lsort, fb2->file_lfilter);
		gtk_tree_view_set_model(GTK_TREE_VIEW(fb2->file_v), GTK_TREE_MODEL(fb2->file_lsort));
		/* we remove our reference, so the only reference is kept by the treeview, if the treeview is destroyed, the models will be destroyed */
		g_object_unref(fb2->file_lfilter);
		g_object_unref(fb2->file_lsort);
	}
}

/**
 * fb2_fspath_from_uri:
 *
 * return a newly allocated treepath for the filesystem_tstore based on 'uri'
 */
static GtkTreePath *
fb2_fspath_from_uri(Tfilebrowser2 * fb2, GFile * uri)
{
	GtkTreeIter *iter;

	iter = g_hash_table_lookup(FB2CONFIG(main_v->fb2config)->filesystem_itable, uri);
	if (iter) {
		return gtk_tree_model_get_path(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->filesystem_tstore), iter);
	}
	return NULL;
}

/**
 * fb2_uri_from_fspath:
 *
 * returns the uri stored in the treestore based on the 'fs_path' from the filesystem treestore
 */
static GFile *
fb2_uri_from_fspath(Tfilebrowser2 * fb2, GtkTreePath * fs_path)
{
	if (fs_path) {
		GtkTreeIter fsiter;
		if (gtk_tree_model_get_iter
			(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->filesystem_tstore), &fsiter, fs_path)) {
			return fb2_uri_from_iter(&fsiter);
		} else {
			DEBUG_MSG("fb2_uri_from_fspath, WARNING, no fsiter for fs_path=%p ", fs_path);
			DEBUG_TPATH(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->filesystem_tstore), fs_path, TRUE);
		}
	}
	return NULL;
}

/**
 * fb2_uri_from_file_sort_path:
 *
 * returns the uri stored in the treestore based on the 'sort_path' from the file sort
 */
static GFile *
fb2_uri_from_file_sort_path(Tfilebrowser2 * fb2, GtkTreePath * sort_path, gchar ** mime)
{
	GFile *uri = NULL;
	GtkTreeIter iter;
	/* because of the buggy behaviour of the GtkTreeModelFilter we don't use this code
	   GtkTreePath *fs_path = fb2_fspath_from_file_sortpath(fb2, sort_path);
	   uri = fb2_uri_from_fspath(fb2, fs_path);
	   gtk_tree_path_free(fs_path); */
	if (gtk_tree_model_get_iter(fb2->file_lsort, &iter, sort_path)) {
		gtk_tree_model_get(fb2->file_lsort, &iter, URI_COLUMN, &uri, -1);
		if (mime) {
			gtk_tree_model_get(fb2->file_lsort, &iter, TYPE_COLUMN, &mime, -1);
		}
	}
	return uri;
}

/**
 * fb2_uri_from_dir_sort_path:
 *
 * returns the uri stored in the treestore based on the 'sort_path' from the dir sort
 */
static GFile *
fb2_uri_from_dir_sort_path(Tfilebrowser2 * fb2, GtkTreePath * sort_path, gchar ** mime)
{
	GFile *uri = NULL;
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter(fb2->dir_tsort, &iter, sort_path)) {
		gtk_tree_model_get(fb2->dir_tsort, &iter, URI_COLUMN, &uri, -1);
		if (mime) {
			gtk_tree_model_get(fb2->dir_tsort, &iter, TYPE_COLUMN, mime, -1);
		}
	}
	return uri;
}

/**
 * fb2_isdir_from_dir_sort_path:
 *
 * returns TRUE if 'sort_path' from the directory sort model points to a directory
 */
static gboolean
fb2_isdir_from_dir_sort_path(Tfilebrowser2 * fb2, GtkTreePath * sort_path)
{
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter(fb2->dir_tsort, &iter, sort_path)) {
		gchar *mime_type = NULL;
		gboolean is_dir = FALSE;
		gtk_tree_model_get(fb2->dir_tsort, &iter, TYPE_COLUMN, &mime_type, -1);
		DEBUG_MSG("fb2_isdir_from_file_sort_path, mime_type=%s\n", mime_type);
		if (mime_type && MIME_ISDIR(mime_type)) {
			is_dir = TRUE;
		}
		g_free(mime_type);
		return (is_dir);
	} else {
		DEBUG_MSG
			("fb2_isdir_from_dir_sort_path, WARNING, sort_path CANNOT be converted into a valid iter!\n");
	}
	return FALSE;
}

/**
 * fb2_fspath_from_dir_selection:
 *
 * returns a newly allocated treepath for the current selected entry in the dir view
 */
static GtkTreePath *
fb2_fspath_from_dir_selection(Tfilebrowser2 * fb2)
{
	GtkTreeModel *sort_model;
	GtkTreeIter sort_iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(fb2->dir_v));
	if (gtk_tree_selection_get_selected(selection, &sort_model, &sort_iter)) {
		GFile *uri;
		gtk_tree_model_get(sort_model, &sort_iter, URI_COLUMN, &uri, -1);
		if (uri) {
			return fb2_fspath_from_uri(fb2, uri);
		}
	}
	DEBUG_MSG("fb2_fspath_from_dir_selection, returning NULL\n");
	return NULL;
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
	if (fb2->file_v
		&& gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(fb2->file_v)),
										   &sort_model, &sort_iter)) {
		GFile *uri = NULL;
		gtk_tree_model_get(sort_model, &sort_iter, URI_COLUMN, &uri, -1);
		if (mime) {
			gtk_tree_model_get(sort_model, &sort_iter, TYPE_COLUMN, mime, -1);
		}
		return uri;
	} else if (!fb2->file_v && fb2->dir_v
			   &&
			   gtk_tree_selection_get_selected(gtk_tree_view_get_selection
											   (GTK_TREE_VIEW(fb2->dir_v)), &sort_model, &sort_iter)) {
		GFile *uri = NULL;
		gtk_tree_model_get(sort_model, &sort_iter, URI_COLUMN, &uri, -1);
		if (mime) {
			gtk_tree_model_get(sort_model, &sort_iter, TYPE_COLUMN, mime, -1);
		}
		return uri;
	}
#ifdef DEBUG
	else
		DEBUG_MSG("fb2_uri_from_file_selection, no selection!\n");
#endif
	return NULL;
}

/**
 * fb2_uri_from_dir_selection:
 *
 * returns the uri stored in the treestore for the current selected entry in the dir_v
 */
static GFile *
fb2_uri_from_dir_selection(Tfilebrowser2 * fb2)
{
	GtkTreeModel *sort_model;
	GtkTreeIter sort_iter;
	if (gtk_tree_selection_get_selected
		(gtk_tree_view_get_selection(GTK_TREE_VIEW(fb2->dir_v)), &sort_model, &sort_iter)) {
		GFile *uri = NULL;
		gtk_tree_model_get(sort_model, &sort_iter, URI_COLUMN, &uri, -1);
		return uri;
	}
	return NULL;
}

/**
 * handle_activate_on_file:
 *
 * opens the file, project or inserts the image pointer to by 'uri'
 */
/*
static void handle_activate_on_file(Tfilebrowser2 * fb2, GFile * uri, gchar *mimetype) {
	if (mimetype) {
		if (strncmp(mimetype, "image", 5) == 0) {
			/ * image! * /
			g_print("handle_activate_on_file, TODO, handle image activate!\n");
		} else if (strcmp(mimetype, "application/x-bluefish-project") == 0) {
			project_open_from_file(fb2->bfwin, uri);
			return;
		}
	}
	doc_new_from_uri(fb2->bfwin, uri, NULL, FALSE, FALSE, -1, -1);
	DEBUG_MSG("handle_activate_on_file, finished\n");
}
*/

static void
rename_not_open_file(Tbfwin * bfwin, GFile * olduri)
{
	gchar *newfilename = NULL, *oldfilename;

	/* Promt user, "File/Move To"-style. */
	oldfilename = g_file_get_uri(olduri);
	newfilename = ask_new_filename(bfwin, oldfilename, oldfilename, TRUE);
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
			GtkTreeIter *olditer;

			olditer = g_hash_table_lookup(FB2CONFIG(main_v->fb2config)->filesystem_itable, olduri);
			if (olditer)
				fb2_treestore_delete_children(FB2CONFIG(main_v->fb2config)->filesystem_tstore, olditer,
											  FALSE);

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
				fb2_refresh_dir_from_uri(parent1);
			} else if (parent2) {
				fb2_refresh_dir_from_uri(parent2);
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
				g_print("fb2rpopup_new, failed to create directory %s: %s, done=%d\n", tmp, error->message,
						done);
				g_error_free(error);
				g_free(tmp);
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
						g_print("fb2rpopup_new, failed to create file: %s\n", error->message);
						g_error_free(error);
					}
				}
			} while (counter < 100);
			if (done) {
				rename_not_open_file(fb2->bfwin, newuri);
			}
		}
		if (done) {
			g_print("calling for newuri %p %s\n", newuri, g_file_get_uri(newuri));
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
fb2_refilter(Tfilebrowser2 * fb2)
{
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(fb2->dir_tfilter));
	if (fb2->filebrowser_viewmode == viewmode_dual) {
		gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(fb2->file_lfilter));
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
			g_print("fb2rpopup_delete, calling file_delete_file_async\n");
			file_delete_async(uri, FALSE, rcpopup_async_delete_lcb, uri);
		}
		g_free(filename);
		g_free(fullpath);
	}
}

static void
popup_menu_filter_activate(GtkAction * action, gpointer user_data)
{
	Tfilebrowser2 *fb2 = FILEBROWSER2(user_data);

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

	/* BUG: in the case of a click in a dual-view fileview, we should not use the basedir
	   as third argument */
	fb2rpopup_new(fb2, TRUE, fb2->basedir);
}

static void
popup_menu_new_file(GtkAction * action, gpointer user_data)
{
	Tfilebrowser2 *fb2 = FILEBROWSER2(user_data);

	/* BUG: in the case of a click in a dual-view fileview, we should not use the basedir
	   as third argument */
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
	if (uri)
		file_handle(uri, fb2->bfwin, mime, FALSE);
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
		GtkTreeIter *iter;
		iter = g_hash_table_lookup(FB2CONFIG(main_v->fb2config)->filesystem_itable, baseuri);
		fb2_refresh_dir(NULL, iter);
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
			doc_save_backend(tmpdoc, TRUE, TRUE, FALSE, FALSE);
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

	fs_path = fb2_fspath_from_dir_selection(fb2);
	DEBUG_MSG("fb2rpopup_rpopup_action_lcb, fs_path=%p\n", fs_path);
	refilter_dirlist(fb2, fs_path);
	gtk_tree_path_free(fs_path);
	if (fb2->bfwin->session->filebrowser_focus_follow && fb2->bfwin->current_document) {
		fb2_focus_document(fb2->bfwin, fb2->bfwin->current_document);
	}
	gtk_tree_model_get_iter_first(fb2->dir_tsort, &iter);
	fs_path = gtk_tree_model_get_path(fb2->dir_tsort, &iter);
	if (!gtk_tree_view_row_expanded(GTK_TREE_VIEW(fb2->dir_v), fs_path)) {
		gtk_tree_view_expand_row(GTK_TREE_VIEW(fb2->dir_v), fs_path, FALSE);
	}
	gtk_tree_path_free(fs_path);
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

	refilter_dirlist(fb2, NULL);
	if (fb2->bfwin->current_document)
		fb2_focus_document(fb2->bfwin, fb2->bfwin->current_document);
}

static void
popup_menu_view_mode_changed(GtkRadioAction * action, GtkRadioAction * current, gpointer user_data)
{
	Tfilebrowser2 *fb2 = FILEBROWSER2(user_data);
	gint view_mode = gtk_radio_action_get_current_value(action);

	BFWIN(fb2->bfwin)->session->filebrowser_viewmode = view_mode;
	fb2_set_viewmode_widgets(fb2, view_mode);
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
	"    <menuitem action='Open'/>"
	"    <menuitem action='OpenAdvanced'/>"
	"    <menuitem action='Rename'/>"
	"    <menuitem action='Delete'/>"
	"    <separator/>"
	"    <menuitem action='NewFile'/>"
	"    <menuitem action='NewDirectory'/>"
	"    <separator/>"
	"    <menuitem action='Refresh'/>"
	"    <menuitem action='FollowActiveDoc'/>"
	"    <menuitem action='SetBaseDir'/>"
	"    <menuitem action='SetDocumentRoot'/>"
	"    <separator/>"
	"    <menuitem action='ShowFullTree'/>"
	"    <menuitem action='ShowBackupFiles'/>"
	"    <menuitem action='ShowHiddenFiles'/>"
	"    <separator/>"
	"    <menu action='ViewModeMenu'>"
	"      <menuitem action='ViewModeDual'/>"
	"      <menuitem action='ViewModeFlat'/>"
	"      <menuitem action='ViewModeTree'/>"
	"    </menu>"
	"    <menu action='FilterMenu'>"
	"      <placeholder name='FilterPlaceholder'/>"
	"      <separator/>"
	"      <menuitem action='NewFilter'/>"
	"      <menuitem action='EditFilter'/>"
	"      <menuitem action='DeleteFilter'/>"
	"    </menu>"
	"  </popup>"
	"</ui>";

static const GtkActionEntry filebrowser_actions[] = {
	{"FileBrowserMenu", NULL, N_("File Browser menu")},
	{"Open", NULL, N_("_Open"), NULL, N_("Open"), G_CALLBACK(popup_menu_open)},
	{"OpenAdvanced", NULL, N_("Open _Advanced..."), NULL, N_("Open advanced"),
	 G_CALLBACK(popup_menu_open_advanced)},
	{"Rename", NULL, N_("Rena_me"), NULL, N_("Rename"), G_CALLBACK(popup_menu_rename)},
	{"Delete", NULL, N_("_Delete"), NULL, N_("Delete"), G_CALLBACK(popup_menu_delete)},
	{"NewFile", NULL, N_("New _File"), NULL, N_("New file"), G_CALLBACK(popup_menu_new_file)},
	{"NewDirectory", NULL, N_("_New Directory"), NULL, N_("New directory"),
	 G_CALLBACK(popup_menu_new_directory)},
	{"Refresh", NULL, N_("_Refresh"), NULL, N_("Refresh"), G_CALLBACK(popup_menu_refresh)},
	{"SetBaseDir", NULL, N_("_Set as base dir"), NULL, N_("Set as base directory"),
	 G_CALLBACK(popup_menu_set_basedir)},
	{"SetDocumentRoot", NULL, N_("Set as document root"), NULL, N_("Set as document root"),
	 G_CALLBACK(popup_menu_set_document_root)},
	{"ShowFullTree", NULL, N_("Show Full _Tree"), NULL, N_("Show full tree"),
	 G_CALLBACK(popup_menu_show_full_tree)},
	{"ViewModeMenu", NULL, N_("View Mode")},
	{"FilterMenu", NULL, N_("Filter")},
	{"NewFilter", NULL, N_("New Filter"), NULL, N_("New filter"), G_CALLBACK(popup_menu_new_filter)},
	{"EditFilter", NULL, N_("Edit Filter"), NULL, N_("Edit filter"), G_CALLBACK(popup_menu_edit_filter)},
	{"DeleteFilter", NULL, N_("Delete Filter"), NULL, N_("Delete filter"), G_CALLBACK(popup_menu_delete_filter)}
};

static const GtkToggleActionEntry filebrowser_toggle_actions[] = {
	{"FollowActiveDoc", NULL, N_("Follow Active Document"), NULL, N_("Follow active document"),
	 G_CALLBACK(popup_menu_follow_active_doc), FALSE},
	{"ShowBackupFiles", NULL, N_("Show Backup Files"), NULL, N_("Show backup files"),
	 G_CALLBACK(popup_menu_show_backup_files), FALSE},
	{"ShowHiddenFiles", NULL, N_("Show Hidden Files"), NULL, N_("Show hidden files"),
	 G_CALLBACK(popup_menu_show_hidden_files), FALSE}
};

static const GtkRadioActionEntry filebrowser_radio_actions[] = {
	{"ViewModeDual", NULL, N_("Dual"), NULL, NULL, viewmode_dual},
	{"ViewModeFlat", NULL, N_("Flat"), NULL, NULL, viewmode_flat},
	{"ViewModeTree", NULL, N_("Tree"), NULL, NULL, viewmode_tree}
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
	gtk_action_group_add_radio_actions(bfwin->filebrowserGroup, filebrowser_radio_actions,
									   G_N_ELEMENTS(filebrowser_radio_actions),
									   fb2->filebrowser_viewmode, G_CALLBACK(popup_menu_view_mode_changed),
									   fb2);
	gtk_ui_manager_insert_action_group(bfwin->uimanager, bfwin->filebrowserGroup, 1);
	g_object_unref(bfwin->filebrowserGroup);

	gtk_ui_manager_add_ui_from_string(bfwin->uimanager, filebrowser_menu_ui, -1, &error);
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
	if (menu) {
		bfwin_set_menu_toggle_item_from_path(bfwin->uimanager, "/FileBrowserMenu/FollowActiveDoc",
											 fb2->bfwin->session->filebrowser_focus_follow);
		bfwin_set_menu_toggle_item_from_path(bfwin->uimanager, "/FileBrowserMenu/ShowBackupFiles",
											 fb2->filebrowser_show_backup_files);
		bfwin_set_menu_toggle_item_from_path(bfwin->uimanager, "/FileBrowserMenu/ShowHiddenFiles",
											 fb2->filebrowser_show_hidden_files);

		if (!is_directory && !is_file) {
			bfwin_action_set_sensitive(bfwin->uimanager, "/FileBrowserMenu/Rename", FALSE);
			bfwin_action_set_sensitive(bfwin->uimanager, "/FileBrowserMenu/Delete", FALSE);
		}
		bfwin_action_set_sensitive(bfwin->uimanager, "/FileBrowserMenu/OpenAdvanced", is_directory);
		bfwin_action_set_sensitive(bfwin->uimanager, "/FileBrowserMenu/SetDocumentRoot", is_directory);
		bfwin_action_set_sensitive(bfwin->uimanager, "/FileBrowserMenu/SetBaseDir", is_directory);

		if (!is_file) {
			bfwin_action_set_sensitive(bfwin->uimanager, "/FileBrowserMenu/Open", FALSE);
		}
		if (fb2->basedir == NULL || fb2->filebrowser_viewmode == viewmode_flat) {
			bfwin_action_set_sensitive(bfwin->uimanager, "/FileBrowserMenu/ShowFullTree", FALSE);
		}

		if (!bfwin->fb2_filters_group) {
			bfwin->fb2_filters_group = gtk_action_group_new("FileBrowserFilterActions");
			gtk_ui_manager_insert_action_group(bfwin->uimanager, bfwin->fb2_filters_group, 1);
		} else {
			GList *actions, *list;

			gtk_ui_manager_remove_ui(bfwin->uimanager, bfwin->fb2_filters_merge_id);

			actions = gtk_action_group_list_actions(bfwin->fb2_filters_group);
			for (list = actions; list; list = list->next) {
				g_signal_handlers_disconnect_by_func(GTK_ACTION(list->data),
													 G_CALLBACK(popup_menu_filter_activate), fb2);
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

			g_signal_connect(G_OBJECT(action), "activate", G_CALLBACK(popup_menu_filter_activate), fb2);

			gtk_ui_manager_add_ui(bfwin->uimanager, bfwin->fb2_filters_merge_id,
								  "/FileBrowserMenu/FilterMenu/FilterPlaceholder", filter->name,
								  filter->name, GTK_UI_MANAGER_MENUITEM, FALSE);

			if (fb2->curfilter == filter)
				gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), TRUE);

			value++;
		}

		gtk_widget_show(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
	} else
		g_warning("showing file browser popup menu failed");
}

static void
dir_v_row_expanded_lcb(GtkTreeView * tree, GtkTreeIter * sort_iter,
					   GtkTreePath * sort_path, Tfilebrowser2 * fb2)
{
	GFile *uri;
	DEBUG_MSG("dir_v_row_expanded_lcb, called for fb2=%p with sort_path=\n", fb2);
	/* refresh the directory that is being expanded */
	gtk_tree_model_get(fb2->dir_tsort, sort_iter, URI_COLUMN, &uri, -1);
	if (uri) {
		fb2_refresh_dir(uri, NULL);
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
			g_print("context menu: %s selected\n",is_dir?"dir":"file");
			popup_menu_create(fb2, is_dir, !is_dir, event);
			gtk_tree_path_free(path);
		} else {
			DEBUG_MSG("dir_v_button_press_lcb, no path for position\n");
			g_print("context menu: nothing selected\n");
			popup_menu_create(fb2, FALSE, FALSE, event);
		}
	} else if (!(fb2->filebrowser_viewmode == viewmode_dual) && event->button == 1
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
				file_handle(uri, fb2->bfwin, mime, FALSE);
				/*handle_activate_on_file(fb2, uri,mime); */
			}
			/* BUG??: do we need to free mime here ?? */
		}
	}
	return FALSE;				/* pass the event on */
}

static gboolean
file_v_button_press_lcb(GtkWidget * widget, GdkEventButton * event, Tfilebrowser2 * fb2)
{
	DEBUG_MSG("file_v_button_press_lcb, called for fb2=%p and event->button=%d\n", fb2, event->button);
	if (event->button == 3) {
		GtkTreePath *path;
		gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(fb2->file_v), event->x, event->y, &path, NULL,
									  NULL, NULL);
		if (path) {
			g_print("context menu: file selected\n");
			popup_menu_create(fb2, FALSE, TRUE, event);
			gtk_tree_path_free(path);
		} else {
			g_print("context menu: nothing selected\n");
			DEBUG_MSG("no path for position\n");
			popup_menu_create(fb2, FALSE, FALSE, event);
		}
	} else if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
		GtkTreePath *sort_path;
		gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(fb2->file_v), event->x, event->y, &sort_path,
									  NULL, NULL, NULL);
		if (sort_path) {
			GFile *uri;
			gchar *mime = NULL;
			uri = fb2_uri_from_file_sort_path(fb2, sort_path, &mime);
			if (uri) {
#ifdef DEBUG
				gchar *basename = g_file_get_basename(uri);
				DEBUG_MSG("file_v_button_press_lcb, doucleclick on %s\n", basename);
				g_free(basename);
#endif
				file_handle(uri, fb2->bfwin, mime, FALSE);
				/*handle_activate_on_file(fb2, uri,mime); */
			}
#ifdef DEBUG
			else {
				DEBUG_MSG("file_v_button_press_lcb, WARNING, NO URI FOUND, cannot activate NULL\n");
			}
#endif
			gtk_tree_path_free(sort_path);
		}
	}
	return FALSE;				/* pass the event on */
}

static gboolean
dirmenu_idle_cleanup_lcb(gpointer callback_data)
{
	GtkTreeModel *oldmodel = callback_data;
	gboolean cont;
	GtkTreeIter iter;
	DEBUG_MSG("dirmenu_idle_cleanup_lcb, cleanup the old model\n");
	/* now we cleanup the old model and it's contents */

	cont = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(oldmodel), &iter);
	while (cont) {
		GFile *uri = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(oldmodel), &iter, DIR_URI_COLUMN, &uri, -1);
		if (uri)
			g_object_unref(uri);
		/* hmm if this last remove results in an empty listtore there is a crash?? */
		cont = gtk_list_store_remove(GTK_LIST_STORE(oldmodel), &iter);
	}
	DEBUG_MSG("dirmenu_idle_cleanup_lcb, unref the old model\n");
	g_object_unref(oldmodel);
	return FALSE;
}

static void
dirmenu_set_curdir(Tfilebrowser2 * fb2, GFile * newcurdir)
{
	GtkTreeIter iter, setiter;
	GHashTable *hasht;
	GList *tmplist;
	GtkTreeModel *oldmodel = fb2->dirmenu_m;
	GFile *tmp;
	GVolumeMonitor *gvolmon;
	gboolean cont, havesetiter = FALSE;
	DEBUG_MSG("dirmenu_set_curdir(fb2=%p, newcurdir=%p)\n", fb2, newcurdir);
	if (fb2->currentdir) {
		if (newcurdir && (fb2->currentdir == newcurdir || g_file_equal(fb2->currentdir, newcurdir)))
			return;
		if (fb2->currentdir)
			g_object_unref(fb2->currentdir);
	}
	fb2->currentdir = newcurdir;
	g_object_ref(fb2->currentdir);
#ifdef DEBUG
	DEBUG_MSG("dirmenu_set_curdir, newcurdir=");
	DEBUG_GFILE(newcurdir, TRUE);
#endif

	fb2->dirmenu_m = GTK_TREE_MODEL(gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING));

	hasht = g_hash_table_new_full(g_file_hash, (GEqualFunc) g_file_equal, NULL, NULL);

	/* rebuild the current uri */
	tmp = g_file_dup(newcurdir);
	do {
		gchar *name = g_file_get_uri(tmp);
		GFile *tmp2 = g_file_get_parent(tmp);
		DEBUG_MSG("dirmenu_set_curdir, appending %s to the new model\n", name);
		gtk_list_store_append(GTK_LIST_STORE(fb2->dirmenu_m), &iter);
		if (!havesetiter) {
			setiter = iter;
			havesetiter = TRUE;
		}
		if (tmp2 == NULL) {
			gchar *icon_name;
			GError *error = NULL;
			GMount *gmount = g_file_find_enclosing_mount(tmp, NULL, &error);
			if (!error) {
				GIcon *icon;
				icon = g_mount_get_icon(gmount);
				icon_name = icon_name_from_icon(icon);
				DEBUG_MSG("detected icon name %s for %s\n", icon_name, name);
				g_object_unref(gmount);
			} else {
				icon_name = g_strdup("drive-harddisk");
				DEBUG_MSG("fallback icon name %s for %s\n", icon_name, name);
			}
			gtk_list_store_set(GTK_LIST_STORE(fb2->dirmenu_m), &iter, DIR_NAME_COLUMN, name, DIR_URI_COLUMN, tmp	/* don't unref tmp at this point, because there is a reference in the model */
							   , DIR_ICON_COLUMN, icon_name, -1);
			g_free(icon_name);
		} else {
			gtk_list_store_set(GTK_LIST_STORE(fb2->dirmenu_m), &iter, DIR_NAME_COLUMN, name, DIR_URI_COLUMN, tmp	/* don't unref tmp at this point, because there is a reference in the model */
							   , DIR_ICON_COLUMN, "folder", -1);
		}
		g_hash_table_insert(hasht, tmp, GINT_TO_POINTER(1));
		g_free(name);
		tmp = tmp2;
		cont = (tmp != NULL);
	} while (cont);

	tmplist = g_list_last(fb2->bfwin->session->recent_dirs);
	while (tmplist) {
		GFile *uri;
		gchar *name;

		uri = g_file_new_for_uri(tmplist->data);
		if (uri && g_hash_table_lookup(hasht, uri) == NULL) {
			name = g_file_get_uri(uri);
			DEBUG_MSG("dirmenu_set_curdir, appending %s\n", name);
			gtk_list_store_append(GTK_LIST_STORE(fb2->dirmenu_m), &iter);
			gtk_list_store_set(GTK_LIST_STORE(fb2->dirmenu_m), &iter, DIR_NAME_COLUMN, name,
							   DIR_URI_COLUMN, uri, DIR_ICON_COLUMN, "folder", -1);
			g_hash_table_insert(hasht, uri, GINT_TO_POINTER(1));
			g_free(name);
		}
		tmplist = g_list_previous(tmplist);
	}

	gvolmon = g_volume_monitor_get();
	{
		GList *mountlist = tmplist = g_list_first(g_volume_monitor_get_mounts(gvolmon));
		while (tmplist) {
			GMount *gmount = tmplist->data;
			gchar *name, *icon_name;
			GFile *root;
			GIcon *icon;
			name = g_mount_get_name(gmount);
			root = g_mount_get_root(gmount);
			icon = g_mount_get_icon(gmount);
			icon_name = icon_name_from_icon(icon);
			if (g_hash_table_lookup(hasht, root) == NULL) {
				gtk_list_store_append(GTK_LIST_STORE(fb2->dirmenu_m), &iter);
				gtk_list_store_set(GTK_LIST_STORE(fb2->dirmenu_m), &iter, DIR_NAME_COLUMN, name, DIR_URI_COLUMN, root	/* don't unref root at this point, because there is a reference in the model */
								   , DIR_ICON_COLUMN, icon_name, -1);
				g_hash_table_insert(hasht, root, GINT_TO_POINTER(1));

			}
			DEBUG_MSG("found mount %s and icon_name %s\n", name, icon_name);
			g_free(name);
			g_free(icon_name);
			if (icon) {
				g_object_unref(icon);
			}
			g_object_unref(gmount);
			tmplist = tmplist->next;
		}
		g_list_free(mountlist);
	}
	g_object_unref(gvolmon);
	g_hash_table_destroy(hasht);

	DEBUG_MSG("dirmenu_set_curdir, activate the new model\n");
	g_signal_handler_block(fb2->dirmenu_v, fb2->dirmenu_changed_signal);
	gtk_combo_box_set_model(GTK_COMBO_BOX(fb2->dirmenu_v), GTK_TREE_MODEL(fb2->dirmenu_m));
	if (havesetiter)
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(fb2->dirmenu_v), &setiter);
	g_signal_handler_unblock(fb2->dirmenu_v, fb2->dirmenu_changed_signal);
	DEBUG_MSG("dirmenu_set_curdir, activated!\n");
	g_idle_add_full(G_PRIORITY_LOW, dirmenu_idle_cleanup_lcb, oldmodel, NULL);
}

static void
dir_v_row_activated_lcb(GtkTreeView * tree, GtkTreePath * path,
						GtkTreeViewColumn * column, Tfilebrowser2 * fb2)
{
	if (fb2->filebrowser_viewmode == viewmode_flat) {
		gchar *mime = NULL;
		GFile *uri = fb2_uri_from_dir_sort_path(fb2, path, &mime);	/* this is a pointer to the uri stored in the treemodel */
		if (mime && MIME_ISDIR(mime)) {
			fb2_set_basedir_backend(fb2, uri);
			dirmenu_set_curdir(fb2, uri);
		}
		/* BUG?? do we need to free mime here ? */
	}
}

static void
dir_v_selection_changed_lcb(GtkTreeSelection * treeselection, Tfilebrowser2 * fb2)
{
	GtkTreeModel *sort_model = NULL;
	GtkTreeIter sort_iter;
	/* Get the current selected row and the model. */
	DEBUG_MSG("dir_v_selection_changed_lcb, treeselection=%p, fb2=%p\n", treeselection, fb2);
	if (treeselection && gtk_tree_selection_get_selected(treeselection, &sort_model, &sort_iter)) {
		GFile *uri;
		gchar *mime_type;
		gtk_tree_model_get(sort_model, &sort_iter, URI_COLUMN, &uri, TYPE_COLUMN, &mime_type, -1);
		DEBUG_MSG("dir_v_selection_changed_lcb, mime_type=%s and uri=", mime_type);
		DEBUG_GFILE(uri, TRUE);
		if (uri && (mime_type && MIME_ISDIR(mime_type))) {
			DEBUG_MSG("uri %p is directory, calling dirmenu_set_curdir\n", uri);
			dirmenu_set_curdir(fb2, uri);
			fb2_focus_dir(fb2, uri, TRUE);
		}
		g_free(mime_type);
	} else {
		DEBUG_MSG("dir_v_selection_changed_lcb, could not get any selection, returning..\n");
	}
}

/* use NULL to unset the basedir */
static void
fb2_set_basedir_backend(Tfilebrowser2 * fb2, GFile * uri)
{
	GtkTreePath *basepath = NULL;
	if (uri && fb2->basedir && (fb2->basedir == uri || g_file_equal(fb2->basedir, uri))) {
		DEBUG_MSG("fb2_set_basedir_backend, basedir did not change, do nothing\n");
		return;
	}
	/* disconnect the dir_v and file_v for higher performance */
	gtk_tree_view_set_model(GTK_TREE_VIEW(fb2->dir_v), NULL);
	if (fb2->filebrowser_viewmode == viewmode_dual)
		gtk_tree_view_set_model(GTK_TREE_VIEW(fb2->file_v), NULL);
	fb2->file_lfilter = NULL;	/* this is required, because the refilter_filelist function tries to check if the virtual root did change for the file filter */
	DEBUG_MSG("fb2_set_basedir_backend, disconnected current filter/sort models, lfilter=%p\n",
			  fb2->file_lfilter);

	basepath = treepath_for_uri(fb2, uri);

	DEBUG_MSG("fb2_set_basedir_backend, refilter, basepath=%p\n", basepath);
	refilter_dirlist(fb2, basepath);
	if (basepath) {
		fb2_focus_dir(fb2, fb2->basedir, (fb2->filebrowser_viewmode == viewmode_flat));
		gtk_tree_path_free(basepath);
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
void
fb2_set_basedir(Tbfwin * bfwin, const gchar * curi)
{
	if (bfwin->fb2) {
		Tfilebrowser2 *fb2 = bfwin->fb2;
		if (curi) {
			GFile *uri;

			uri = g_file_new_for_uri(strip_trailing_slash((gchar *) curi));

			if (uri) {
				fb2_set_basedir_backend(fb2, uri);
				g_object_unref(uri);
			} else {
				DEBUG_MSG("fb2_set_basedir, failed to convert to GFile!!!!!!!\n");
			}
		} else {
			DEBUG_MSG("fb2_set_basedir, curi=NULL\n");
			refilter_dirlist(fb2, NULL);
		}
	}
}

static void
dirmenu_changed_lcb(GtkComboBox * widget, gpointer data)
{
	Tfilebrowser2 *fb2 = data;
	GtkTreeIter iter;
	DEBUG_MSG("dirmenu_changed_lcb, started\n");
	if (gtk_combo_box_get_active_iter(widget, &iter)) {
		GFile *uri;
		DEBUG_MSG("dirmenu_changed_lcb. we have an active iter\n");
		gtk_tree_model_get(GTK_TREE_MODEL(fb2->dirmenu_m), &iter, DIR_URI_COLUMN, &uri, -1);
		if (!uri)
			return;
		g_object_ref(uri);
		g_signal_handler_block(fb2->dirmenu_v, fb2->dirmenu_changed_signal);
		if (fb2->basedir || fb2->filebrowser_viewmode == viewmode_flat) {
			if (fb2->filebrowser_viewmode != viewmode_flat && (gfile_uri_is_parent(fb2->basedir, uri, TRUE)
															   || g_file_equal(fb2->basedir, uri))) {
				fb2_focus_dir(FILEBROWSER2(fb2), uri, FALSE);
			} else {
				/* drop basedir or set as new basedir, for now we set it as new basedir */
				fb2_set_basedir_backend(fb2, uri);
			}
		} else {				/* no basedir, we can focus it */
			DEBUG_MSG("dirmenu_changed_lcb, no basedir, call fb2_focus_dir\n");
			fb2_focus_dir(FILEBROWSER2(fb2), uri, FALSE);
		}
		g_object_unref(uri);
		g_signal_handler_unblock(fb2->dirmenu_v, fb2->dirmenu_changed_signal);
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
		if (gtk_tree_model_get_iter(fb2->dir_tsort, &iter, path)) {
			GFile *uri;
			gchar *mime_type;
			gtk_tree_model_get(fb2->dir_tsort, &iter, TYPE_COLUMN, &mime_type, URI_COLUMN, &uri, -1);
			if (mime_type && strncmp(mime_type, "x-directory", 11) == 0) {
				destdir = uri;
				g_object_ref(destdir);
			} else {
				destdir = g_file_get_parent(uri);
			}
			g_free(mime_type);
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

static void
fb2_two_pane_notify_position_lcb(GObject * object, GParamSpec * pspec, gpointer data)
{
	gint position;
	g_object_get(object, pspec->name, &position, NULL);
	if (main_v->props.restore_dimensions) {
		main_v->globses.two_pane_filebrowser_height = position;
	}
}

static gboolean
fb2_tooltip_lcb(GtkWidget * widget, gint x, gint y, gboolean keyboard_tip, GtkTooltip * tooltipwidget,
				gpointer user_data)
{
	GtkTreeView *tview = user_data;
	GtkTreePath *path;
	gboolean retval = FALSE;
	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tview), x, y, &path, NULL, NULL, NULL)) {
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter(gtk_tree_view_get_model(tview), &iter, path)) {
			GFile *uri = NULL;
			gtk_tree_model_get(gtk_tree_view_get_model(tview), &iter, URI_COLUMN, &uri, -1);
			if (uri) {
				char *text;
				text = g_file_get_uri(uri);
				gtk_tooltip_set_text(tooltipwidget, text);
				g_free(text);
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

	DEBUG_MSG("fb2_set_viewmode_widgets, destroying old widgets (if any)\n");
	if (fb2->filebrowser_viewmode == viewmode) {
		/* the current viewmode is the same as the requested viewmode
		   we can return if all the widgets exist */
		if (fb2->dir_tfilter != NULL)
			return;
	} else {
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
		fb2->dir_tfilter = fb2->file_lfilter = NULL;
		DEBUG_MSG("\n");
	}
	fb2->filebrowser_viewmode = viewmode;

	DEBUG_MSG("fb2_set_viewmode_widgets, building new GUI\n");
	if (fb2->basedir) {
		basepath = treepath_for_uri(fb2, fb2->basedir);
		if (fb2->filebrowser_viewmode != viewmode_flat) {
			gtk_tree_path_up(basepath);
		}
	}

	fb2->dir_tfilter =
		gtk_tree_model_filter_new(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->filesystem_tstore), basepath);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(fb2->dir_tfilter),
										   tree_model_filter_func, fb2, NULL);

	fb2->dir_tsort = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(fb2->dir_tfilter));
	if (fb2->filebrowser_viewmode != viewmode_dual) {
		DEBUG_MSG("setting sort function\n");
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(fb2->dir_tsort), FILENAME_COLUMN,
										filebrowser_sort_func, NULL, NULL);
	} else {
		DEBUG_MSG("sort alphabetical\n");
	}
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fb2->dir_tsort), FILENAME_COLUMN,
										 GTK_SORT_ASCENDING);

	fb2->dir_v = gtk_tree_view_new_with_model(fb2->dir_tsort);
	/* we remove our reference, so the only reference is kept by the treeview, if the treeview is destroyed, the models will be destroyed */
	g_object_unref(G_OBJECT(fb2->dir_tfilter));
	g_object_unref(G_OBJECT(fb2->dir_tsort));

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(fb2->dir_v), FALSE);
	if (fb2->filebrowser_viewmode != viewmode_flat) {
		dirselection = gtk_tree_view_get_selection(GTK_TREE_VIEW(fb2->dir_v));
		DEBUG_MSG("fb2_init, NEW FILEBROWSER2, treeselection=%p, fb2=%p, dir_tfilter=%p\n",
				  dirselection, fb2, fb2->dir_tfilter);
		g_signal_connect(G_OBJECT(dirselection), "changed", G_CALLBACK(dir_v_selection_changed_lcb), fb2);
	}

	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer,
										"icon-name", ICON_NAME_COLUMN,
										"pixbuf_expander_closed", PIXMAP_COLUMN,
										"pixbuf_expander_open", PIXMAP_COLUMN, NULL);
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "editable", FALSE, NULL);	/* Not editable. */
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", FILENAME_COLUMN, NULL);
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
		fb2->vpaned = gtk_vpaned_new();
		gtk_widget_set_size_request(fb2->vpaned, main_v->globses.left_panel_width, -1);
		gtk_paned_set_position(GTK_PANED(fb2->vpaned), main_v->globses.two_pane_filebrowser_height);
		g_signal_connect(G_OBJECT(fb2->vpaned), "notify::position",
						 G_CALLBACK(fb2_two_pane_notify_position_lcb), NULL);

		gtk_paned_add1(GTK_PANED(fb2->vpaned), fb2->dirscrolwin);

		fb2->file_lfilter =
			gtk_tree_model_filter_new(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->filesystem_tstore), NULL);
		gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(fb2->file_lfilter),
											   file_list_filter_func, fb2, NULL);

		fb2->file_lsort = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(fb2->file_lfilter));
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fb2->file_lsort), FILENAME_COLUMN,
											 GTK_SORT_ASCENDING);
		DEBUG_MSG("fb2_init, file_lfilter=%p\n", fb2->file_lfilter);
		fb2->file_v = gtk_tree_view_new_with_model(fb2->file_lsort);
		/* we remove our reference, so the only reference is kept by the treeview, if the treeview is destroyed, the models will be destroyed */
		g_object_unref(G_OBJECT(fb2->file_lfilter));
		g_object_unref(G_OBJECT(fb2->file_lsort));

		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(fb2->file_v), FALSE);
		renderer = gtk_cell_renderer_pixbuf_new();
		column = gtk_tree_view_column_new();
		gtk_tree_view_column_pack_start(column, renderer, FALSE);
		gtk_tree_view_column_set_attributes(column, renderer,
											"icon-name", ICON_NAME_COLUMN,
											"pixbuf_expander_closed", PIXMAP_COLUMN,
											"pixbuf_expander_open", PIXMAP_COLUMN, NULL);
		renderer = gtk_cell_renderer_text_new();
		g_object_set(G_OBJECT(renderer), "editable", FALSE, NULL);	/* Not editable. */
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_set_attributes(column, renderer, "text", FILENAME_COLUMN, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(fb2->file_v), column);
		scrolwin = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC,
									   GTK_POLICY_AUTOMATIC);
		gtk_container_add(GTK_CONTAINER(scrolwin), fb2->file_v);
		gtk_paned_add2(GTK_PANED(fb2->vpaned), scrolwin);

		gtk_box_pack_start(GTK_BOX(fb2->vbox), fb2->vpaned, TRUE, TRUE, 0);
		g_signal_connect(G_OBJECT(fb2->file_v), "button_press_event",
						 G_CALLBACK(file_v_button_press_lcb), fb2);
		gtk_drag_dest_set(fb2->file_v, (GTK_DEST_DEFAULT_ALL), drag_dest_types, 2,
						  (GDK_ACTION_DEFAULT | GDK_ACTION_COPY));
		g_signal_connect(G_OBJECT(fb2->file_v), "drag_data_received",
						 G_CALLBACK(fb2_file_v_drag_data_received), fb2);
		g_object_set(fb2->file_v, "has-tooltip", TRUE, NULL);
		g_signal_connect(fb2->file_v, "query-tooltip", G_CALLBACK(fb2_tooltip_lcb), fb2->file_v);
	}

	g_signal_connect(G_OBJECT(fb2->dir_v), "row-activated", G_CALLBACK(dir_v_row_activated_lcb), fb2);
	g_signal_connect(G_OBJECT(fb2->dir_v), "button_press_event", G_CALLBACK(dir_v_button_press_lcb), fb2);
	fb2->expand_signal =
		g_signal_connect(G_OBJECT(fb2->dir_v), "row-expanded", G_CALLBACK(dir_v_row_expanded_lcb), fb2);
	g_signal_connect(fb2->dir_v, "query-tooltip", G_CALLBACK(fb2_tooltip_lcb), fb2->dir_v);
	g_object_set(fb2->dir_v, "has-tooltip", TRUE, NULL);
	/*gtk_container_resize_children(GTK_CONTAINER(fb2->vbox)); */
	gtk_widget_show_all(fb2->vbox);
	DEBUG_MSG("fb2_set_viewmode_widgets, new GUI finished\n");
}


void
fb2_update_settings_from_session(Tbfwin * bfwin)
{
	if (bfwin->fb2) {
		gboolean need_refilter = FALSE;
		Tfilebrowser2 *fb2 = bfwin->fb2;

		DEBUG_MSG("fb2_update_settings_from_session, started, bfwin=%p, fb2=%p, viewmode=%d\n",
				  bfwin, fb2, fb2->filebrowser_viewmode);

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
			const gchar *tmp = (gchar *) ((GList *) g_list_last(bfwin->session->recent_dirs))->data;
			/* the fb2_set_basedir function tests itself if  the basedir if changed, if not it does not refresh */
			DEBUG_MSG("fb2_update_settings_from_session, set basedir %s\n", tmp);
			if (tmp && tmp[0]) {
				fb2_set_basedir(bfwin, tmp);
			}
		} else {
			fb2_set_basedir(bfwin, NULL);
		}
		/* TODO: fb2_set_basedir already calls refilter in most cases (not if the 
		   requested basedir was already the active basedir), so
		   we can optimise this and call refilter only when really needed. */
		if (need_refilter) {
			if (fb2->dir_tfilter)
				gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(fb2->dir_tfilter));
			if (fb2->file_lfilter && fb2->filebrowser_viewmode == viewmode_dual)
				gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(fb2->file_lfilter));
		}
	}
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

	fb2->dirmenu_m = GTK_TREE_MODEL(gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING));
	fb2->dirmenu_v = gtk_combo_box_new_with_model(fb2->dirmenu_m);
	/*gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(fb2->dirmenu_v),3); */
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(fb2->dirmenu_v), renderer, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(fb2->dirmenu_v), renderer, "icon-name", DIR_ICON_COLUMN,
								   /*"pixbuf_expander_closed", DIR_ICON_COLUMN,
								      "pixbuf_expander_open", DIR_ICON_COLUMN, */ NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(fb2->dirmenu_v), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(fb2->dirmenu_v), renderer, "text", DIR_NAME_COLUMN, NULL);
	gtk_box_pack_start(GTK_BOX(fb2->vbox), fb2->dirmenu_v, FALSE, FALSE, 0);
	fb2->dirmenu_changed_signal =
		g_signal_connect(fb2->dirmenu_v, "changed", G_CALLBACK(dirmenu_changed_lcb), fb2);

	fb2_update_settings_from_session(bfwin);

	if (!bfwin->filebrowserGroup)
		popup_menu_action_group_init(bfwin);

	gtk_widget_show_all(fb2->vbox);
/*  {
    GFile *uri = NULL;
    if (bfwin->session->recent_dirs) {
      uri = gnome_vfs_uri_new(strip_trailing_slash(((GList *)g_list_first(bfwin->session->recent_dirs))->data));
    } else if (bfwin->project) {
      uri = gnome_vfs_uri_new(strip_trailing_slash(bfwin->project->basedir));
    }
    if (uri) {
      DEBUG_MSG("fb2_init, first build, and then set the basedir to ");
      DEBUG_GFILE(uri, TRUE);
      fb2_build_dir(uri);
      {
        GtkTreePath *basedir = fb2_fspath_from_uri(fb2, uri);
        DEBUG_MSG("fb2_init, will set the basedir to path(%p) ",basedir);
        DEBUG_TPATH(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->filesystem_tstore), basedir, TRUE);
        refilter_dirlist(fb2, basedir);
        gtk_tree_path_free(basedir);
      }
      DEBUG_MSG("fb2_init, focus fb2->basedir ");
      DEBUG_GFILE(fb2->basedir, TRUE);
      fb2_focus_dir(fb2, fb2->basedir, FALSE);
      g_object_unref(uri);
    }
  }*/
	return fb2->vbox;
}

void
fb2_cleanup(Tbfwin * bfwin)
{
	if (bfwin->fb2) {
		Tfilebrowser2 *fb2 = FILEBROWSER2(bfwin->fb2);
		if (fb2->basedir)
			g_object_unref(fb2->basedir);
		g_free(fb2);
		bfwin->fb2 = NULL;
	}
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
	/* a lot of things can be the same for all windows, such as the hashtable and the treestore with
	   all the files, and the pixmaps to use. All stored in Tfilebrowser2config;
	   This will be initialized in this function */
	fb2config = g_new0(Tfilebrowser2config, 1);
	main_v->fb2config = fb2config;

	fb2config->filesystem_itable =
		g_hash_table_new_full(g_file_hash, (GEqualFunc) g_file_equal, uri_hash_destroy, iter_value_destroy);
	fb2config->filesystem_tstore =
		gtk_tree_store_new(N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER,
						   G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_POINTER);
	DEBUG_MSG("fb2config_init, finished\n");
}

#ifdef MEMORY_LEAK_DEBUG

static gboolean
treestore_foreach_cleanup(GtkTreeModel * model, GtkTreePath * path, GtkTreeIter * iter, gpointer data)
{
	GFile *uri;
	GFileInfo *finfo;
	gtk_tree_model_get(model, iter, FILEINFO_COLUMN, &finfo, URI_COLUMN, &uri, -1);
#ifdef DBG_FBREFCOUNT
	fake_finfo_ref--;
#endif
	g_object_unref(finfo);
	g_object_unref(uri);
	return FALSE;
}

/* avoid segfaults during bluefish exit */
void
fb2config_cleanup(void)
{
	GList *tmplist = g_list_first(FB2CONFIG(main_v->fb2config)->uri_in_refresh);
	DEBUG_MSG("fb2config_cleanup, stopping all async directory reads\n");
	while (tmplist) {
		Turi_in_refresh *uir = tmplist->data;
		g_cancellable_cancel(uir->cancel);
		g_object_unref(uir->uri);
		g_slice_free(Turi_in_refresh, uir);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(FB2CONFIG(main_v->fb2config)->uri_in_refresh);
	FB2CONFIG(main_v->fb2config)->uri_in_refresh = NULL;
	g_hash_table_unref(FB2CONFIG(main_v->fb2config)->filesystem_itable);


	gtk_tree_model_foreach(GTK_TREE_MODEL(FB2CONFIG(main_v->fb2config)->filesystem_tstore),
						   treestore_foreach_cleanup, NULL);
#ifdef DBG_FBREFCOUNT
	g_print("fake_finfo_ref=%d\n", fake_finfo_ref);
#endif
	g_object_unref(FB2CONFIG(main_v->fb2config)->filesystem_tstore);
	g_free(main_v->fb2config);
}
#endif
