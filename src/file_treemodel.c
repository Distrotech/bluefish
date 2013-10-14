/* Bluefish HTML Editor
 * file_treemodel.c - non-GUI general functions
 *
 * Copyright (C) 2013 Olivier Sessink
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

#include <string.h>
#include <stdlib.h>
#include "bluefish.h"
#include "file_treemodel.h"
#include "bf_lib.h"

static void filetreemodel_init(FileTreemodel * pkg_tree);
static void filetreemodel_class_init(FileTreemodelClass * klass);
static void filetreemodel_tree_model_init(GtkTreeModelIface * iface);
static void filetreemodel_finalize(GObject * object);
static GtkTreeModelFlags filetreemodel_get_flags(GtkTreeModel * tree_model);
static gint filetreemodel_get_n_columns(GtkTreeModel * tree_model);
static GType filetreemodel_get_column_type(GtkTreeModel * tree_model, gint index);
static gboolean filetreemodel_get_iter(GtkTreeModel * tree_model, GtkTreeIter * iter, GtkTreePath * path);
static GtkTreePath *filetreemodel_get_path(GtkTreeModel * tree_model, GtkTreeIter * iter);
static void filetreemodel_get_value(GtkTreeModel * tree_model,
									GtkTreeIter * iter, gint column, GValue * value);
static gboolean filetreemodel_iter_next(GtkTreeModel * tree_model, GtkTreeIter * iter);
static gboolean filetreemodel_iter_children(GtkTreeModel * tree_model,
									GtkTreeIter * iter, GtkTreeIter * parent);
static gboolean filetreemodel_iter_has_child(GtkTreeModel * tree_model, GtkTreeIter * iter);
static gint filetreemodel_iter_n_children(GtkTreeModel * tree_model, GtkTreeIter * iter);
static gboolean filetreemodel_iter_nth_child(GtkTreeModel * tree_model,
									 GtkTreeIter * iter, GtkTreeIter * parent, gint n);
static gboolean filetreemodel_iter_parent(GtkTreeModel * tree_model, GtkTreeIter * iter, GtkTreeIter * child);

static GObjectClass *parent_class = NULL;	/* GObject stuff  */

/*************************** debug functions ***************************/
#ifdef DUMP_TREE
static void dump_record(UriRecord * record)
{
	gint i;
	g_print("record %s at position %d has %d children\n", record->name, record->pos, record->num_rows);
	for (i = 0; i < record->num_rows; i++) {
		g_print("     %d: %s\n", i, record->rows[i]->name);
		if (record->rows[i]->pos != i) {
			g_print("ABORT: this child at %d has pos %d ?????????????\n", i, record->rows[i]->pos);
		}
	}
}

static void pindent(gint indent)
{
	gint i;
	for (i = 0; i < indent; i++) {
		g_print("  ");
	}
}

static void print_record(UriRecord * record, gint indent)
{
	gint i;
	pindent(indent);
	g_print("'%s' (%p), isdir=%d, mime-type=%s", record->name, record, record->isdir, record->fast_content_type);
	if (record->num_rows) {
		g_print(" with %d children\n", record->num_rows);
		for (i = 0; i < record->num_rows; i++) {
			print_record(record->rows[i], indent + 1);
		}
	} else {
		g_print("\n");
	}
}

static void print_tree(FileTreemodel * ftm)
{
	gint i;
	g_print("Dump filetreemodel %p with %d top-level records\n", ftm, ftm->num_rows);
	for (i = 0; i < ftm->num_rows; i++) {
		print_record(ftm->rows[i], 1);
	}
	g_print("--------------------------\n");
}
#endif /*DUMP_TREE*/
/*************************** end of debug functions ***************************/


static void record_cleanup(UriRecord * record)
{
	if (record->num_rows > 0) {
		g_critical("record_cleanup, record still has children ????\n");
	}
	DEBUG_MSG("record_cleanup %p name='%s', unref uri %p\n", record, record->name, record->uri);
	g_object_unref(record->uri);
	g_free(record->name);
	g_free(record->fast_content_type);
	g_free(record->icon_name);
	/*g_free(record->name_collate_key); */
	g_slice_free(UriRecord, record);
}

static void record_recursive_cleanup(UriRecord * record)
{
	gint j;
	for (j = 0; j < record->num_rows; j++) {
		record_recursive_cleanup(record->rows[j]);
	}
	g_free(record->rows);
	record->num_rows = 0;
	record_cleanup(record);
}


static UriRecord *get_nth_record(FileTreemodel * ftm, UriRecord * precord, gint n)
{
	if (precord) {
		if (n >= precord->num_rows) {
			g_critical("get_nth_record, requested a record (n=%d) beyond the end (precord->num_rows=%d)\n",n,precord->num_rows);
			return NULL;
		}
		return precord->rows[n];
	}
	if (n >= ftm->num_rows) {
		g_critical("requested a record (n=%d) beyond the end (ftm->num_rows=%d)\n",n,ftm->num_rows);
		return NULL;
	}
	return ftm->rows[n];
}

static gchar *get_toplevel_name_for_uri(gchar * curi)
{
	gchar *needle;
	if (!curi)
		return NULL;

	if (curi[0] == '/')
		return g_strdup("/");

	if (strncmp(curi, "file:///", 8) == 0)
		return g_strdup("/");

	needle = strstr(curi, "://");
	DEBUG_MSG("searching for toplevel, got non-local uri %s\n", needle);
	if (!needle) {
		g_warning("cannot handle uri %s\n", curi);
		return NULL;
	}
	needle = strchr(needle + 3, '/');
	return g_strndup(curi, needle - curi + 1);
}

#ifdef NOTUSED
static gchar **split_gfile(GFile * uri)
{
	gchar **arr;
	gint i = 0, n = 8;
	gchar *curi, *needle, *toplevel, *haystack;

	arr = g_malloc0(n * sizeof(gchar *));
	curi = g_file_get_uri(uri);

	/* find toplevel */
	if (curi[0] == '/') {
		haystack = curi + 1;
		toplevel = g_strdup("/");
	} else if (strncmp(curi, "file://", 7) == 0) {
		haystack = curi + 7;
		toplevel = g_strdup("/");
	} else {
		toplevel = get_toplevel_name_for_uri(curi);
		if (!toplevel) {
			g_free(arr);
			g_free(curi);
			return NULL;
		}
		haystack = curi + strlen(toplevel) - 1;
	}

	needle = strrchr(haystack, '/');
	while (needle) {
		gint nlen;
		nlen = strlen(needle);
		DEBUG_MSG("searched for / in '%s', got %s for i=%d\n", curi, needle, i);
		if (nlen < 2) {
			DEBUG_MSG("nlen=%d, needle=%s, break;\n", nlen, needle);
			break;
		}
		if (*(needle + 1) != '\0') {
			arr[i] = g_strdup(needle + 1);
			i++;
			if (i > n + 2) {
				n += 8;
				arr = g_realloc(arr, n * sizeof(gchar *));
			}
			*needle = '\0';
		}
		needle = strrchr(haystack, '/');
	}
	arr[i] = toplevel;
	DEBUG_MSG("set arr[%d]=NULL\n", i + 1);
	arr[i + 1] = NULL;
	g_free(curi);
	return arr;
}


static UriRecord *get_record_for_uri(FileTreemodel * ftm, GFile * uri)
{
	gchar **arr;
	UriRecord **rows;
	guint16 num_rows;
	gint i, arrlen;
	UriRecord srecord, *psrecord, **tmp;

	arr = split_gfile(uri);
	if (!arr)
		return NULL;
	arrlen = g_strv_length(arr);
	if (arrlen == 0)
		return NULL;

	psrecord = &srecord;
	rows = ftm->rows;
	num_rows = ftm->num_rows;

	for (i = arrlen - 1; i >= 0; i--) {
		/* search this entry! */
		srecord.name = arr[i];
		srecord.isdir = 1;
		DEBUG_MSG("bsearch for name %s in %d rows\n", srecord.name, num_rows);
		tmp = bsearch(&psrecord, rows, num_rows, sizeof(UriRecord *), compare_records);
		/* if it is not found, and i==0, it might have been a file, try the name as file */
		if (!tmp && i == 0) {
			srecord.isdir = 0;
			tmp = bsearch(&psrecord, rows, num_rows, sizeof(UriRecord *), compare_records);
		}
		if (!tmp) {
			DEBUG_MSG("get_record_for_uri, did not find a record for arr[%d]=%s\n", i, arr[i]);
			break;
		}
		DEBUG_MSG("get_record_for_uri, found record %s for arr[%d]=%s\n", (*tmp)->name, i, arr[i]);
		rows = (*tmp)->rows;
		num_rows = (*tmp)->num_rows;
	}
	g_strfreev(arr);
	return (tmp ? *tmp : NULL);
}



#endif

static GtkTreePath *get_treepath_for_record(UriRecord * record)
{
	GtkTreePath *path = gtk_tree_path_new();
	while (record) {
		gtk_tree_path_prepend_index(path, record->pos);
		record = record->parent;
	}
	return path;
}


/********************** sorting functions **************************************************************************************/

static int compare_records(const void *a, const void *b)
{
	UriRecord *ra = *((UriRecord **) a);
	UriRecord *rb = *((UriRecord **) b);
	if (ra->isdir != rb->isdir) {
		return (rb->isdir - ra->isdir);
	}
	if (ra->name[0] == '.' && rb->name[0] != '.') {
		return -1;
	}
	if (ra->name[0] != '.' && rb->name[0] == '.') {
		return 1;
	}
	/*DEBUG_MSG("a=%p, b=%p, ra=%p, rb=%p\n",a,b,ra,rb);
	   DEBUG_MSG("compare %s and %s\n",ra->name,rb->name); */
	return g_utf8_collate(ra->name, rb->name);
}


static void filetree_re_sort(FileTreemodel * ftm, UriRecord * precord)
{
	guint num_rows;
	UriRecord ***arr;
	gint *neworder;
	gint i;
	GtkTreePath *path;
	gboolean reordered = FALSE;

	/* do the sorting ! */
	if (precord == NULL) {
		/* toplevel */
		arr = &ftm->rows;
		num_rows = ftm->num_rows;
	} else {
		arr = &precord->rows;
		num_rows = precord->num_rows;
#ifdef DUMP_TREE
		dump_record(precord);
#endif
	}
	if (num_rows < 2)
		return;

	qsort(*arr, num_rows, sizeof(UriRecord *), compare_records);

	DEBUG_MSG("precord %p has rows=%p\n", precord, precord ? precord->rows : NULL);

	/* let other objects know about the new order */
	neworder = g_new0(gint, num_rows);
	for (i = 0; i < num_rows; ++i) {
		/* Note that the API reference might be wrong about
		 * this, see bug number 124790 on bugs.gnome.org.
		 * Both will work, but one will give you 'jumpy'
		 * selections after row reordering. */
		/* neworder[(custom_list->rows[i])->pos] = i; */
		if (neworder[i] != ((*arr)[i])->pos)
			reordered = TRUE;
		neworder[i] = ((*arr)[i])->pos;
		DEBUG_MSG("filetree_re_sort, moved '%s' from row %d to row %d \n", (*arr)[i]->name, (*arr)[i]->pos, i);
		((*arr)[i])->pos = i;
	}
	if (reordered) {
		GtkTreeIter iter;
		if (precord) {
			path = get_treepath_for_record(precord);
			iter.user_data = precord;
			iter.stamp = ftm->stamp;
		} else {
			path = gtk_tree_path_new_first();
		}
#ifdef DEBUG
		DEBUG_MSG("call gtk_tree_model_rows_reordered(path=%s)\n",gtk_tree_path_to_string(path));
#endif
		gtk_tree_model_rows_reordered(GTK_TREE_MODEL(ftm), path, precord?&iter:NULL, neworder);
		DEBUG_MSG("done re-ordering\n");
		gtk_tree_path_free(path);
	}
	g_free(neworder);
}


/*
gboolean gfile_uri_is_parent(GFile * parent, GFile * child, gboolean recursive)
{
	gboolean retval = FALSE;
	GFile *tmp, *tmp2;

	if (parent == NULL) {
		return FALSE;
	}
	if (child == NULL) {
		return FALSE;
	}
	g_object_ref(child);
	tmp2 = child;
	do {
		tmp = g_file_get_parent(tmp2);
		g_object_unref(tmp2);
		if (tmp == NULL) {
			break;
		}
		retval = g_file_equal(tmp, parent);
		tmp2 = tmp;
	} while (recursive == TRUE && retval != TRUE);
	if (tmp) {
		g_object_unref(tmp);
	}
	return retval;
}*/

static gchar *icon_name_from_icon(GIcon * icon)
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
		DEBUG_MSG("icon %p is not themed, revert to default icon 'folder'\n",icon);
		icon_name = g_strdup("folder");
	}
	return icon_name;
}

/*
gchar *gfile_display_name(GFile * uri, GFileInfo * finfo)
{
	gchar *retval;
	if (finfo && g_file_info_has_attribute(finfo, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME)) {
		retval = g_strdup(g_file_info_get_display_name(finfo));
	} else {
		GFileInfo *finfo2;
		DEBUG_MSG("gfile_display_name, finfo does not have STANDARD_DISPLAY_NAME, query for it!\n");
		finfo2 =
			g_file_query_info(uri, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME, G_FILE_QUERY_INFO_NONE, NULL,
							  NULL);
		retval = g_strdup(g_file_info_get_display_name(finfo2));
		g_object_unref(finfo2);
	}
	return retval;
}*/

static void fill_uri(UriRecord * newrecord, GFile * uri, GFileInfo * finfo)
{
	GIcon *icon;
	newrecord->name = g_strdup(g_file_info_get_display_name(finfo));
	newrecord->fast_content_type = g_strdup(g_file_info_get_attribute_string(finfo, G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE));
	/*newrecord->name_collate_key = g_utf8_collate_key(newrecord->name, -1); *//* for fast sorting, used later */
	newrecord->uri = uri;
	DEBUG_MSG("fill_uri, newrecord=%p, uri=%p, name='%s'\n", newrecord, newrecord->uri, newrecord->name);
	g_object_ref(uri);
	icon = g_file_info_get_icon(finfo);
	newrecord->icon_name = icon_name_from_icon(icon);
	/*g_print("got icon name %s from finfo %p with icon %p, content_type=%s\n",newrecord->icon_name, finfo, icon, newrecord->fast_content_type);*/
	newrecord->weight = PANGO_WEIGHT_NORMAL;
	newrecord->isdir = (g_file_info_get_file_type(finfo) == G_FILE_TYPE_DIRECTORY);
#ifdef DEVELOPMENT
	if (newrecord->isdir
		&&
		strcmp(g_file_info_get_attribute_string
			   (finfo, G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE), "inode/directory") != 0) {
		g_print("%s: isdir=%d but mime type =%s ???????????\n", newrecord->name, newrecord->isdir,
				g_file_info_get_attribute_string(finfo,
												 G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE));
		g_assert_not_reached();
	}
#endif
	/*DEBUG_MSG("fill_uri, isdir=%d for name='%s'\n",newrecord->isdir,newrecord->name); */
}

static UriRecord *add_single_uri(FileTreemodel * ftm, UriRecord * record, GFile * child_uri, const gchar *name, const gchar *fast_content_type, const gchar *icon_name, gboolean isdir)
{
	guint newsize, pos;
	UriRecord *newrecord;
	GtkTreeIter iter;
	GtkTreePath *path;

	newrecord = g_slice_new0(UriRecord);
	newrecord->name = g_strdup(name);
	newrecord->fast_content_type = g_strdup(fast_content_type);
	newrecord->uri = child_uri;
	g_object_ref(child_uri);
	newrecord->icon_name = g_strdup(icon_name);
	newrecord->weight = PANGO_WEIGHT_NORMAL;
	newrecord->isdir = isdir;

	/* actually add it to the array now */
	if (record) {
		pos = record->num_rows;
		record->num_rows++;
		newsize = record->num_rows * sizeof(UriRecord *);
		record->rows = g_realloc(record->rows, newsize);
		record->rows[pos] = newrecord;
		DEBUG_MSG("adding newrecord %p (%s) to pos %d of array %p of parent %s\n", newrecord, newrecord->name, pos, record->rows,record->name);
		newrecord->pos = pos;
		newrecord->parent = record;
	} else {
		/* add to toplevel */
		pos = ftm->num_rows;
		ftm->num_rows++;
		newsize = ftm->num_rows * sizeof(UriRecord *);
		ftm->rows = g_realloc(ftm->rows, newsize);
		ftm->rows[pos] = newrecord;
		newrecord->pos = pos;
		newrecord->parent = NULL;
	}
	DEBUG_MSG("added record %p for uri %p to hash table\n", newrecord, child_uri);
	g_hash_table_insert(ftm->alluri, child_uri, newrecord);

	/* inform the tree view and other interested objects
	 *  (e.g. tree row references) that we have inserted
	 *  a new row, and where it was inserted */

	path = get_treepath_for_record(newrecord);
	filetreemodel_get_iter(GTK_TREE_MODEL(ftm), &iter, path);
	gtk_tree_model_row_inserted(GTK_TREE_MODEL(ftm), path, &iter);
	gtk_tree_path_free(path);

	/* qsort it !?!? */
	filetree_re_sort(ftm, record);

	return newrecord;
}

static gchar *get_toplevel_name(GFile * uri)
{
	GError *gerror = NULL;
	GMount *gmnt;
	gchar *name;
	gmnt = g_file_find_enclosing_mount(uri, NULL, &gerror);
	if (!gerror && gmnt) {
		name = g_mount_get_name(gmnt);
		g_object_unref(gmnt);
	} else {
		gchar *curi = g_file_get_uri(uri);
		DEBUG_MSG("get_toplevel_name, got error %s (%d), using uri string\n", gerror->message, gerror->code);
		name = get_toplevel_name_for_uri(curi);
		g_free(curi);
		if (gerror)
			g_error_free(gerror);
	}
	return name;
}

static void add_dummy_subdir(FileTreemodel * ftm, UriRecord * precord)
{
	GFile *dummy_uri;
	const static gchar *space = " ";
	/* add a dummy item so the expander will be shown */
	dummy_uri = g_file_get_child(precord->uri, space);
	DEBUG_MSG("add_dummy_subdir, precord %p '%s', dummy_uri=%p\n", precord, precord->name,
			dummy_uri);
	add_single_uri(ftm, precord, dummy_uri, space, DIR_MIME_TYPE, "folder", TRUE);
	g_object_unref(dummy_uri);
}


typedef struct {
	GCancellable *cancel;
	GFile *uri;
	GFileEnumerator *gfe;
	UriRecord *precord;
	FileTreemodel *ftm;
} Turi_in_refresh;

static Turi_in_refresh *get_uri_in_refresh(FileTreemodel * ftm, GFile * uri)
{
	GList *tmplist = g_list_first(ftm->uri_in_refresh);
	while (tmplist) {
		if (((Turi_in_refresh *) tmplist->data)->uri == uri
			|| g_file_equal(((Turi_in_refresh *) tmplist->data)->uri, uri))
			return tmplist->data;
		tmplist = g_list_next(tmplist);
	}
	return NULL;
}

gboolean uri_in_refresh(FileTreemodel * ftm, GFile * uri) {
	return (get_uri_in_refresh(ftm, uri)!=NULL);

}

static void uri_in_refresh_cleanup(FileTreemodel * ftm, Turi_in_refresh * uir)
{
	DEBUG_MSG("uri_in_refresh_cleanup, called for %p with uri %p\n", uir, uir->uri);
	ftm->uri_in_refresh = g_list_remove(ftm->uri_in_refresh, uir);
	g_object_unref(uir->uri);
	/*g_object_unref(uir->p_uri); */
	g_object_unref(uir->cancel);
	g_slice_free(Turi_in_refresh, uir);
}


static void ftm_remove(FileTreemodel * ftm, UriRecord * record, gboolean dont_remove_from_parent)
{
	gint i;
	GtkTreePath *path;
	DEBUG_MSG("ftm_remove, remove record %p with name %s, pos=%d, dontremovefromparent=%d\n", record,
			record->name, record->pos, dont_remove_from_parent);
	/* if it has any children, remove them first */
	if (record->num_rows) {
		for (i = 0; i < record->num_rows; i++) {
			ftm_remove(ftm, record->rows[i], TRUE);
		}
		/* remove them all from their parent in one go */
		g_free(record->rows);
		record->num_rows = 0;
		record->rows = NULL;
	}

	/* let the treeview know that this one is gone */
	path = get_treepath_for_record(record);
	gtk_tree_model_row_deleted(GTK_TREE_MODEL(ftm), path);
	gtk_tree_path_free(path);

	if (!dont_remove_from_parent) {
		UriRecord ***arr;
		guint16 *num_rows;
		/*now remove it really from it's parent */
		if (record->parent) {
			DEBUG_MSG("ftm_remove, remove record %p from parent %p which has rows=%p\n", record, record->parent,
					record->parent->rows);
			arr = &(record->parent->rows);
			num_rows = &(record->parent->num_rows);
		} else {
			arr = &(ftm->rows);
			num_rows = &(ftm->num_rows);
		}
		DEBUG_MSG("*num_rows=%d\n", *num_rows);
		if (record->pos + 1 < *num_rows) {
			DEBUG_MSG("ftm_remove, move %d rows from pos=%d (%p) to pos=%d (%p), total rows=%d\n",
					(*num_rows - record->pos - 1), record->pos + 1, (*arr)[record->pos], record->pos,
					(*arr)[record->pos + 1], *num_rows);
			DEBUG_MSG("current item at pos %d is '%s' (%p), record=%p\n", record->pos,
					(*arr)[record->pos]->name, (*arr)[record->pos], record);
			memmove(&(*arr)[record->pos], &(*arr)[record->pos + 1],
					(*num_rows - record->pos - 1) * sizeof(UriRecord *));
			/*memmove((*arr)+record->pos*sizeof(UriRecord *), (*arr)+(record->pos+1)*sizeof(UriRecord *), (*num_rows - record->pos - 1)*sizeof(UriRecord *)); */
			DEBUG_MSG("after memmove, parent %p has rows %p, *arr=%p\n", record->parent, record->parent->rows,
					*arr);
			DEBUG_MSG("new item at pos %d is '%s' (%p) with pos=%d, record=%p\n", record->pos,
					(*arr)[record->pos]->name, (*arr)[record->pos], (*arr)[record->pos]->pos, record);
		}
		(*num_rows)--;
		if (*num_rows == 0) {
			g_free(*arr);
			*arr = NULL;
		} else {
			*arr = g_realloc(*arr, *num_rows * sizeof(UriRecord *));
			/* now adjust all positions */
			for (i = record->pos; i < *num_rows; i++) {
				DEBUG_MSG("changing from position %d to %d\n", (*arr)[i]->pos, i);
				(*arr)[i]->pos = i;
			}
		}


	}
	record_cleanup(record);
}

static void ftm_delete_children(FileTreemodel * ftm, UriRecord * record, gboolean only_possibly_deleted)
{
	gint i;
	DEBUG_MSG("ftm_delete_children from %s, num_rows=%d, rows=%p\n", record->name, record->num_rows,
			record->rows);
	if (only_possibly_deleted) {
		for (i = record->num_rows - 1; i >= 0; i--) {
			if (record->rows[i]->possibly_deleted) {
				ftm_remove(ftm, record->rows[i], FALSE);
			}
		}
	} else {
		for (i = record->num_rows - 1; i >= 0; i--) {
			ftm_remove(ftm, record->rows[i], FALSE);
		}
		g_free(record->rows);
		record->rows = NULL;
		record->num_rows = 0;
	}
	if (only_possibly_deleted && record->num_rows > 0) {
		filetree_re_sort(ftm, record);
	}
}

static void enumerator_close_lcb(GObject * source_object, GAsyncResult * res, gpointer user_data)
{
	Turi_in_refresh *uir = user_data;
	GError *gerror = NULL;
	DEBUG_MSG("enumerator_close_lcb, close uir %p\n", uir);
	g_file_enumerator_close_finish(uir->gfe, res, &gerror);
	g_object_unref(uir->gfe);
	ftm_delete_children(uir->ftm, uir->precord, TRUE);
#ifdef DUMP_TREE
	print_tree(uir->ftm);
#endif
	uri_in_refresh_cleanup(uir->ftm, uir);
}

static void add_multiple_uris(FileTreemodel * ftm, UriRecord * precord, GList * finfolist)
{
	guint pos, alloced_num, listlen, old_num_rows;
	UriRecord *newrecord;
	GtkTreeIter iter;
	GtkTreePath *path;
	guint16 *num_rows;
	UriRecord ***rows;

	GList *tmplist = g_list_first(finfolist);
	listlen = g_list_length(tmplist);
	DEBUG_MSG("add_multiple_uris, adding %d entries to %s\n", listlen, precord ? precord->name : "root");
	newrecord = g_slice_new0(UriRecord);

	if (precord) {
		num_rows = &precord->num_rows;
		rows = &precord->rows;
#ifdef DUMP_TREE
		dump_record(precord);
#endif
	} else {
		num_rows = &ftm->num_rows;
		rows = &ftm->rows;
	}
	alloced_num = *num_rows;
	old_num_rows = *num_rows;
	if (alloced_num < listlen) {
		alloced_num = listlen + 1;	/* often there is a single entry - a space - that will be removed after adding all items */
		*rows = g_realloc(*rows, alloced_num * sizeof(UriRecord *));
		DEBUG_MSG
			("add_multiple_uris, increase allocation to %d rows, num_rows is at %d, *rows=%p, precord->rows=%p\n",
			 alloced_num, *num_rows, precord ? precord->rows : NULL, *rows);
	}


	while (tmplist) {
		UriRecord **tmp;
		GFileInfo *finfo = tmplist->data;
		/* don't allocate new memory for 'name', if it already exists we don't need it */
		newrecord->name = (gchar *) g_file_info_get_name(finfo);
		newrecord->isdir = (g_file_info_get_file_type(finfo) == G_FILE_TYPE_DIRECTORY);
		DEBUG_MSG("bsearch for name %s\n", newrecord->name);
		tmp = bsearch(&newrecord, *rows, old_num_rows, sizeof(UriRecord *), compare_records);
		if (tmp) {
			/* this file exists */
			DEBUG_MSG("mark %p '%s' as existing\n", *tmp, (*tmp)->name);
			(*tmp)->possibly_deleted = FALSE;

			/* the memory was not allocated for ->name */
			newrecord->name = NULL;
			newrecord->isdir = 0;
		} else {
			GFile *child;
			DEBUG_MSG("%s does not yet exist\n", newrecord->name);
			/* this file does not exist */
			child = g_file_get_child(precord->uri, newrecord->name);
			fill_uri(newrecord, child, finfo);
			g_object_unref(child);
#ifdef DEVELOPMENT
			if (g_hash_table_lookup(ftm->alluri, newrecord->uri)) {
				g_assert_not_reached();
			}
#endif
			pos = *num_rows;
			DEBUG_MSG("add_multiple_uris, add %s at pos %d\n", newrecord->name, pos);
			/* see if we have to alloc more space in the array */
			if (pos >= alloced_num) {
				alloced_num += 32;
				DEBUG_MSG("alloc 32 more positions in array to %d\n", alloced_num);
				*rows = g_realloc(*rows, alloced_num * sizeof(UriRecord *));
				DEBUG_MSG("after realloc, precord->rows=%p, *rows=%p\n", precord ? precord->rows : NULL, *rows);
			}
			(*rows)[pos] = newrecord;
			DEBUG_MSG("adding newrecord %p to pos %d of array %p\n", newrecord, pos, (*rows));
			newrecord->pos = pos;
			(*num_rows)++;
			newrecord->parent = precord;
			g_hash_table_insert(ftm->alluri, newrecord->uri, newrecord);

			path = get_treepath_for_record(newrecord);
			filetreemodel_get_iter(GTK_TREE_MODEL(ftm), &iter, path);
			gtk_tree_model_row_inserted(GTK_TREE_MODEL(ftm), path, &iter);
			gtk_tree_path_free(path);

			if (newrecord->isdir) {
				add_dummy_subdir(ftm, newrecord);
			}

			/* make a 'newrecord' ready for the next loop */
			newrecord = g_slice_new0(UriRecord);
		}
		g_object_unref(finfo);
		tmplist = tmplist->next;
	}
	g_slice_free(UriRecord, newrecord);
/*	DEBUG_MSG("add_multiple_uris, done adding\n");
	dump_record(precord);*/
	/* now allocate the size that is actually used */
	DEBUG_MSG("precord=%p, finalize allocation to %d rows, precord->rows=%p, *rows=%p\n", precord, *num_rows,
			precord ? precord->rows : NULL, *rows);
	*rows = g_realloc(*rows, *num_rows * sizeof(UriRecord *));
	DEBUG_MSG("add_multiple_uris, calling re_sort\n");
	filetree_re_sort(ftm, precord);
	DEBUG_MSG("add_multiple_uris, done\n");
}

static void enumerate_next_files_lcb(GObject * source_object, GAsyncResult * res, gpointer user_data)
{
	Turi_in_refresh *uir = user_data;
	GError *gerror = NULL;
	GList *list;
	DEBUG_MSG("enumerate_next_files_lcb, started for uir %p which has uri %s\n", uir,
			g_file_get_path(uir->uri));
	list = g_file_enumerator_next_files_finish(uir->gfe, res, &gerror);
	if (gerror) {
		g_warning("ERROR: unhandled error %d in enumerate_next_files_lcb(): %s\n", gerror->code,
				  gerror->message);
		return;
	}
	DEBUG_MSG("enumerate_next_files_lcb, number of results=%d\n", g_list_length(list));
	if (list == NULL) {
		/* done */
		g_file_enumerator_close_async(uir->gfe, G_PRIORITY_LOW, uir->cancel, enumerator_close_lcb, uir);
		return;
	}

	add_multiple_uris(uir->ftm, uir->precord, list);
	g_list_free(list);

	DEBUG_MSG("enumerate_next_files_lcb, done\n");
	g_file_enumerator_next_files_async(uir->gfe, 256, G_PRIORITY_LOW, uir->cancel,
									   enumerate_next_files_lcb, uir);
}

static void enumerate_children_lcb(GObject * source_object, GAsyncResult * res, gpointer user_data)
{
	Turi_in_refresh *uir = user_data;
	GError *gerror = NULL;
	uir->gfe = g_file_enumerate_children_finish(uir->uri, res, &gerror);
	if (gerror) {
		if (gerror->code == 14 /* 14 = permission denied, delete any children */ ) {
			ftm_delete_children(uir->ftm, uir->precord, FALSE);
		} else if (gerror->code == 4 /* 4 = not a directory */ ) {
			if (uir->precord->isdir) {
				ftm_remove(uir->ftm, uir->precord, FALSE);
			}
		} else {
			/* delete the directory itself from the treestore */
			ftm_remove(uir->ftm, uir->precord, FALSE);
		}
		g_warning("failed to list directory in filebrowser: %s (%d)\n", gerror->message, gerror->code);
		g_error_free(gerror);
		uri_in_refresh_cleanup(uir->ftm, uir);
		return;
	}
	if (uir->gfe) {
		/*DEBUG_MSG("opened the directory\n"); */
		g_file_enumerator_next_files_async(uir->gfe, 256, G_PRIORITY_LOW, uir->cancel,
										   enumerate_next_files_lcb, uir);
	}
}

static gboolean fill_dir_async_low_priority(gpointer data)
{
	Turi_in_refresh *uir = data;
	DEBUG_MSG("fill_dir_async_low_priority, start fill dir %s async low priority\n",
			g_file_get_path(uir->uri));
	g_file_enumerate_children_async(uir->uri,
									"standard::name,standard::display-name,standard::fast-content-type,standard::icon,standard::edit-name,standard::is-backup,standard::is-hidden,standard::type",
									G_FILE_QUERY_INFO_NONE, G_PRIORITY_LOW, uir->cancel,
									enumerate_children_lcb, uir);
	return FALSE;
}

static void mark_children_refresh(FileTreemodel * ftm, UriRecord * parent)
{
	gint i;
	UriRecord **arr;
	guint num_rows;
	if (parent) {
		arr = parent->rows;
		num_rows = parent->num_rows;
	} else {
		arr = ftm->rows;
		num_rows = ftm->num_rows;
	}

	for (i = 0; i < num_rows; i++) {
		arr[i]->possibly_deleted = 1;
	}

}


/*static GtkTreePath *
path_for_uri(FileTreemodel * ftm, GFile * uri)
{
	UriRecord *record;

	record = g_hash_table_lookup(ftm->alluri, uri);
	if (record) {
		return get_treepath_for_record(record);
	}
	return NULL;
}*/

static void refresh_dir_async(FileTreemodel * ftm, UriRecord * precord, GFile * uri)
{
	Turi_in_refresh *uir;

	if (!uri || !precord) {
		DEBUG_MSG("refresh_dir_async, abort, no uri (%p) or no record (%p)\n", uri, precord);
		return;
	}

	if (get_uri_in_refresh(ftm, uri)) {
		DEBUG_MSG("refresh_dir_async, uri is already in refresh, return\n");
		return;
	}

	DEBUG_MSG("about to register a low priority callback for fill_dir_async_low_priority, precord=%p\n",precord);
	if (!precord) {
		DEBUG_MSG("no precord, build dir\n");
		precord = filetreemodel_build_dir(ftm, uri, NULL);
	} else {
		DEBUG_MSG("existing precord, mark children possible_deleted\n");
		mark_children_refresh(ftm, precord);
	}
	uir = g_slice_new0(Turi_in_refresh);
	uir->precord = precord;
	uir->ftm = ftm;
	uir->uri = g_object_ref(uri);
	uir->cancel = g_cancellable_new();
	ftm->uri_in_refresh = g_list_prepend(ftm->uri_in_refresh, uir);
	g_idle_add_full(G_PRIORITY_LOW, fill_dir_async_low_priority, uir, NULL);
	DEBUG_MSG("refresh_dir_async, low priority callback is registered\n");
}


/*****************************************************************************
 *
 *  filetreemodel_refresh_iter_async: starts a refresh of the directory pointed to by iter.
 *
 *****************************************************************************/
void filetreemodel_refresh_iter_async(FileTreemodel * ftm, GtkTreeIter * iter)
{
	UriRecord *record;
	if (!iter) {
		g_warning("filetreemodel_refresh_iter_async, iter == NULL?\n");
		return;
	}
	record = iter->user_data;

	refresh_dir_async(ftm, record, record->uri);
}

/*****************************************************************************
 *
 *  filetreemodel_refresh_uri_async: starts a refresh of the directory pointed to by uri.
 *
 *****************************************************************************/

void filetreemodel_refresh_uri_async(FileTreemodel * ftm, GFile * uri)
{
	UriRecord *record;
	if (!uri) {
		g_warning("filetreemodel_refresh_uri_async, uri == NULL?\n");
		return;
	}

	record = g_hash_table_lookup(ftm->alluri, uri);
#ifdef DEBUG
	DEBUG_MSG("filetreemodel_refresh_uri_async(uri=%p,%s), record=%p\n",uri,g_file_get_uri(uri),record);
#endif
	refresh_dir_async(ftm, record, uri);
}
/*****************************************************************************
 *
 *  filetree_get_iter_for_uri: sets iter for uri, and returns FALSE if no iter could be set.
 *
 *****************************************************************************/

gboolean filetree_get_iter_for_uri(FileTreemodel * ftm, GFile * uri, GtkTreeIter *iter)
{
	UriRecord *record;
	if (!uri) {
		g_warning("filetree_get_iter_for_uri, uri == NULL?\n");
		return FALSE;
	}

	record = g_hash_table_lookup(ftm->alluri, uri);
	if (!record)
		return FALSE;

	iter->stamp = ftm->stamp;
	iter->user_data = record;
	return TRUE;
}


/*****************************************************************************
 *
 *  filetreemodel_build_dir: builds a directory with all it's parents in the filetreemodel .
 *
 *****************************************************************************/

UriRecord *filetreemodel_build_dir(FileTreemodel * ftm, GFile * uri, GtkTreeIter *iter)
{
	UriRecord *record;
	GFile *tmp, *parent_uri = NULL;
	tmp = uri;

	if (!uri)
		return NULL;
	
	/* first find if any directory part of this uri exists already in the treestore */
	g_object_ref(tmp);

	record = g_hash_table_lookup(ftm->alluri, tmp);
	while (record == NULL) {
		GFile *tmp2 = g_file_get_parent(tmp);
		if (tmp2 == NULL) {
			gchar *name;
			name = get_toplevel_name(tmp);
			/* there was no parent for this filesystem yet */
			DEBUG_MSG("filetreemodel_build_dir, adding parent %s as fake folder\n", name);
			record = add_single_uri(ftm, NULL, tmp, name, DIR_MIME_TYPE, "folder", TRUE);
			g_free(name);
			break;
		} else {
			g_object_unref(tmp);
		}
		tmp = tmp2;
		record = g_hash_table_lookup(ftm->alluri, tmp);
	}							/* after this loop 'tmp' is newly allocated */
	parent_uri = tmp;

#ifdef DEBUG
	if (parent_uri == NULL) {
		g_critical("filetreemodel_build_dir, parent_uri should not be NULL\n");
		g_return_val_if_reached(NULL);
	}
#endif
	/* now loop to create all directories in the treestore that were not in the treestore yet */
	gboolean done = g_file_equal(parent_uri, uri);
	while (!done) {
		GFile *tmp2 = uri;
		gchar *name;

		g_object_ref(tmp2);		/* both 'parent_uri'='tmp' and 'tmp2' are newly allocated */
		while (!gfile_uri_is_parent(parent_uri, tmp2, FALSE)) {
			GFile *tmp3 = g_file_get_parent(tmp2);
			if (!tmp3) {
				g_critical("uh-oh: filetreemodel_build_dir, tried to get parent for %s, parent_uri=%s, uri=%s\n",
						   g_file_get_uri(tmp2), g_file_get_uri(parent_uri), g_file_get_uri(uri));
				g_return_val_if_reached(NULL);	/* TODO: must be g_return_val_if_reached() or handle the situation */
			}
			g_object_unref(tmp2);
			tmp2 = tmp3;
		}						/* after this loop both 'parent_uri'='tmp' and 'tmp2' are newly allocated */

		name = g_file_get_basename(tmp2);
		DEBUG_MSG("filetreemodel_build_dir, add folder %s, uri=%p\n", name, tmp2);
		record = add_single_uri(ftm, record, tmp2, name, DIR_MIME_TYPE, "folder", TRUE);
		g_free(name);
		if (g_file_equal(tmp2, uri)) {
			add_dummy_subdir(ftm, record);
		}
		g_object_unref(parent_uri);
		parent_uri = tmp2;		/* here 'parent_uri'='tmp2' is newly allocated */
		DEBUG_MSG("filetreemodel_build_dir, parent uri=%p, uri=%p\n", parent_uri, uri);
		if (g_file_equal(parent_uri, uri)) {
			done = TRUE;
		}
	}
	g_object_unref(parent_uri);	/* no memory leaks in the uri's... (I hope) */
	if (iter) {
		iter->stamp = ftm->stamp;
		iter->user_data = record;
	}
	return record;
}

static void gtk_tree_model_record_changed(FileTreemodel * ftm, UriRecord *record) {
	GtkTreePath *path;
	GtkTreeIter iter;
	iter.stamp = ftm->stamp;
	iter.user_data = record;
	path = get_treepath_for_record(record);
	gtk_tree_model_row_changed(GTK_TREE_MODEL(ftm),path,&iter);
	gtk_tree_path_free(path);
}

void filetreemodel_set_weight(FileTreemodel * ftm, GFile * uri, guint16 weight) {
	UriRecord *record;
	if (!uri) {
		g_warning("filetreemodel_set_weight, uri == NULL?\n");
	}

	record = g_hash_table_lookup(ftm->alluri, uri);
	if (!record)
		return;

	if (record->weight != weight) {
		record->weight = weight;
		gtk_tree_model_record_changed(ftm, record);
	}
}

void filetreemodel_set_icon(FileTreemodel * ftm, GFile * uri, const gchar *icon_name) {
	UriRecord *record;
	if (!uri) {
		g_warning("filetreemodel_set_icon_name, uri == NULL?\n");
	}

	record = g_hash_table_lookup(ftm->alluri, uri);
	if (!record)
		return;

	if (strcmp(record->icon_name,icon_name)!=0) {
		g_free(record->icon_name);
		record->icon_name = g_strdup(icon_name);
		gtk_tree_model_record_changed(ftm, record);
	}
}

/*****************************************************************************
 *
 *  filetreemodel_get_path: converts a tree iter into a tree path (ie. the
 *                        physical position of that row in the list).
 *
 *****************************************************************************/

static GtkTreePath *filetreemodel_get_path(GtkTreeModel * tree_model, GtkTreeIter * iter)
{
	g_return_val_if_fail(IS_FILETREE_MODEL(tree_model), NULL);
	if (iter == NULL || iter->user_data == NULL || iter->stamp != filetreemodel(tree_model)->stamp) {
		g_warning("filetreemodel_get_path, called with invalid iter %p\n",iter);
		return NULL;
	}
	return get_treepath_for_record((UriRecord *) iter->user_data);
}


/*****************************************************************************
 *
 *  filetreemodel_get_value: Returns a row's exported data columns
 *                         (_get_value is what gtk_tree_model_get uses)
 *
 *****************************************************************************/

static void
filetreemodel_get_value(GtkTreeModel * tree_model, GtkTreeIter * iter, gint column, GValue * value)
{
	UriRecord *record;

	g_return_if_fail(IS_FILETREE_MODEL(tree_model));
	g_return_if_fail(column < filetreemodel(tree_model)->n_columns);

	if (iter == NULL || iter->user_data == NULL || iter->stamp != filetreemodel(tree_model)->stamp) {
		g_warning("filetreemodel_get_value, called with invalid iter %p\n",iter);
		return;
	}

	g_value_init(value, filetreemodel(tree_model)->column_types[column]);
	record = (UriRecord *) iter->user_data;

	switch (column) {
	case filetreemodel_COL_RECORD:
		g_value_set_pointer(value, record);
		break;

	case filetreemodel_COL_NAME:
		/*DEBUG_MSG("filetreemodel_get_value, return '%s'\n", record->name);*/
		g_value_set_string(value, record->name);
		break;

	case filetreemodel_COL_ICON_NAME:
		g_value_set_string(value, record->icon_name);
		break;
	case filetreemodel_COL_WEIGHT:
		g_value_set_uint(value, record->weight);
		break;
	}
}


/*****************************************************************************
 *
 *  filetreemodel_iter_next: Takes an iter structure and sets it to point
 *                         to the next row.
 *
 *****************************************************************************/

static gboolean filetreemodel_iter_next(GtkTreeModel * tree_model, GtkTreeIter * iter)
{
	UriRecord *record, *nextrecord;
	FileTreemodel *ftm;

	g_return_val_if_fail(IS_FILETREE_MODEL(tree_model), FALSE);
	/*DEBUG_MSG("filetreemodel_iter_next\n"); */
	ftm = filetreemodel(tree_model);
	if (iter == NULL || iter->user_data == NULL || iter->stamp != ftm->stamp) {
		g_warning("filetreemodel_iter_next, called with invalid iter %p\n",iter);
		return FALSE;
	}

	record = (UriRecord *) iter->user_data;

	/* Is this the last record in the list? */
	if (record->parent == NULL) {
		if ((record->pos + 1) >= ftm->num_rows)
			return FALSE;
		nextrecord = ftm->rows[(record->pos + 1)];
	} else {
		if ((record->pos + 1) >= record->parent->num_rows)
			return FALSE;
		nextrecord = record->parent->rows[(record->pos + 1)];
	}

	g_assert(nextrecord != NULL);
	g_assert(nextrecord->pos == (record->pos + 1));

	iter->stamp = ftm->stamp;
	iter->user_data = nextrecord;

	return TRUE;
}


/*****************************************************************************
 *
 *  filetreemodel_iter_children: Returns TRUE or FALSE depending on whether
 *                             the row specified by 'parent' has any children.
 *                             If it has children, then 'iter' is set to
 *                             point to the first child. Special case: if
 *                             'parent' is NULL, then the first top-level
 *                             row should be returned if it exists.
 *
 *****************************************************************************/

static gboolean
filetreemodel_iter_children(GtkTreeModel * tree_model, GtkTreeIter * iter, GtkTreeIter * parent)
{
	FileTreemodel *ftm;
	UriRecord *record;

	g_return_val_if_fail(parent == NULL || parent->user_data != NULL, FALSE);
	g_return_val_if_fail(IS_FILETREE_MODEL(tree_model), FALSE);
	ftm = filetreemodel(tree_model);

	if (parent && parent->stamp != ftm->stamp) {
		g_warning("filetreemodel_iter_children, called with invalid parent iter\n");
		return FALSE;
	}

	/* parent == NULL is a special case; we need to return the first top-level row */
	if (parent == NULL) {
		if (ftm->num_rows == 0)
			return FALSE;

		/* Set iter to first item in list */
		iter->stamp = ftm->stamp;
		iter->user_data = ftm->rows[0];
	} else {
		record = parent->user_data;
		if (record->num_rows == 0)
			return FALSE;
		if (record->num_rows > 0 && record->isdir != 1) {
			g_assert_not_reached();
		}
		DEBUG_MSG("filetreemodel_iter_children, return first child for '%s'\n", record->name);
		iter->stamp = ftm->stamp;
		iter->user_data = record->rows[0];
	}

	return TRUE;
}


/*****************************************************************************
 *
 *  filetreemodel_iter_has_child: Returns TRUE or FALSE depending on whether
 *                              the row specified by 'iter' has any children.
 *                              We only have a list and thus no children.
 *
 *****************************************************************************/

static gboolean filetreemodel_iter_has_child(GtkTreeModel * tree_model, GtkTreeIter * iter)
{
	UriRecord *record;

	if (!iter || iter->stamp != filetreemodel(tree_model)->stamp) {
		g_warning("filetreemodel_iter_has_child, called with invalid iter %p\n", iter);
		return FALSE;
	}

	record = iter->user_data;

	if (record->num_rows > 0 && record->isdir != 1) {
		g_assert_not_reached();
	}

	return (record->num_rows > 0);
}


/*****************************************************************************
 *
 *  filetreemodel_iter_n_children: Returns the number of children the row
 *                               specified by 'iter' has. This is usually 0,
 *                               as we only have a list and thus do not have
 *                               any children to any rows. A special case is
 *                               when 'iter' is NULL, in which case we need
 *                               to return the number of top-level nodes,
 *                               ie. the number of rows in our list.
 *
 *****************************************************************************/

static gint filetreemodel_iter_n_children(GtkTreeModel * tree_model, GtkTreeIter * iter)
{
	FileTreemodel *ftm;
	UriRecord *record;

	g_return_val_if_fail(IS_FILETREE_MODEL(tree_model), -1);
	g_return_val_if_fail(iter == NULL || iter->user_data != NULL, FALSE);
	ftm = filetreemodel(tree_model);

	if (iter && iter->stamp != ftm->stamp) {
		g_warning("filetreemodel_iter_n_children, called with invalid iter\n");
		return 0;
	}

	/* special case: if iter == NULL, return number of top-level rows */
	if (!iter)
		return ftm->num_rows;
	record = iter->user_data;

	if (record->num_rows > 0 && record->isdir != 1) {
		g_assert_not_reached();
	}
	DEBUG_MSG("filetreemodel_iter_n_children return %d for '%s'\n", record->num_rows, record->name);
	return record->num_rows;
}


/*****************************************************************************
 *
 *  filetreemodel_iter_nth_child: If the row specified by 'parent' has any
 *                              children, set 'iter' to the n-th child and
 *                              return TRUE if it exists, otherwise FALSE.
 *                              A special case is when 'parent' is NULL, in
 *                              which case we need to set 'iter' to the n-th
 *                              row if it exists.
 *
 *****************************************************************************/

static gboolean
filetreemodel_iter_nth_child(GtkTreeModel * tree_model, GtkTreeIter * iter, GtkTreeIter * parent, gint n)
{
	UriRecord *record;
	FileTreemodel *ftm;

	g_return_val_if_fail(IS_FILETREE_MODEL(tree_model), FALSE);
	ftm = filetreemodel(tree_model);

	if (parent && parent->stamp != ftm->stamp) {
		g_warning("filetreemodel_iter_nth_child called with invalid parent iter\n");
		return FALSE;
	}

	/* a list has only top-level rows */
	if (parent) {
		UriRecord *precord;
		precord = parent->user_data;
		DEBUG_MSG("filetreemodel_iter_nth_child(%d) for '%s'\n", n, precord->name);
		if (n >= precord->num_rows)
			return FALSE;
		record = precord->rows[n];
	} else {
		/* special case: if parent == NULL, set iter to n-th top-level row */
		DEBUG_MSG("filetreemodel_iter_nth_child(%d) for root\n", n);
		if (n >= ftm->num_rows)
			return FALSE;

		record = ftm->rows[n];
	}

	g_assert(record != NULL);
	g_assert(record->pos == n);

	iter->stamp = ftm->stamp;
	iter->user_data = record;

	return TRUE;
}


/*****************************************************************************
 *
 *  filetreemodel_iter_parent: Point 'iter' to the parent node of 'child'. As
 *                           we have a list and thus no children and no
 *                           parents of children, we can just return FALSE.
 *
 *****************************************************************************/

static gboolean filetreemodel_iter_parent(GtkTreeModel * tree_model, GtkTreeIter * iter, GtkTreeIter * child)
{
	UriRecord *record;
	FileTreemodel *ftm = filetreemodel(tree_model);

	if (!child || child->stamp != ftm->stamp) {
		g_warning("filetreemodel_iter_parent, called with invalid child iter %p\n",child);
		return FALSE;
	}

	record = child->user_data;
	if (record->parent == NULL) {
		return FALSE;
	}

	iter->user_data = record->parent;
	iter->stamp = ftm->stamp;
	return TRUE;
}


/*****************************************************************************
 *
 *  filetreemodel_new:  This is what you use in your own code to create a
 *                    new filetree model tree model for you to use.
 *
 *****************************************************************************/

FileTreemodel *filetreemodel_new(void)
{
	FileTreemodel *newFileTreemodel;

	newFileTreemodel = (FileTreemodel *) g_object_new(FILETREE_TYPE_LIST, NULL);

	g_assert(newFileTreemodel != NULL);

	return newFileTreemodel;
}


/*****************************************************************************
 *
 *  filetreemodel_get_iter: converts a tree path (physical position) into a
 *                        tree iter structure (the content of the iter
 *                        fields will only be used internally by our model).
 *                        We simply store a pointer to our UriRecord
 *                        structure that represents that row in the tree iter.
 *
 *****************************************************************************/

static gboolean filetreemodel_get_iter(GtkTreeModel * tree_model, GtkTreeIter * iter, GtkTreePath * path)
{
	FileTreemodel *ftm;
	UriRecord *record;
	gint *indices, i, depth;

	g_assert(IS_FILETREE_MODEL(tree_model));
	g_assert(path != NULL);
	DEBUG_MSG("filetreemodel_get_iter\n");
	ftm = filetreemodel(tree_model);

	indices = gtk_tree_path_get_indices(path);
	depth = gtk_tree_path_get_depth(path);

	record = NULL;
	for (i = 0; i < depth; i++) {
		record = get_nth_record(ftm, record, indices[i]);
		if (!record)
			return FALSE;
	}

	/* We simply store a pointer to our custom record in the iter */
	iter->stamp = ftm->stamp;
	iter->user_data = record;
	iter->user_data2 = NULL;	/* unused */
	iter->user_data3 = NULL;	/* unused */

	return TRUE;
}


/*****************************************************************************
 *
 *  filetreemodel_get_type: here we register our new type and its interfaces
 *                        with the type system. If you want to implement
 *                        additional interfaces like GtkTreeSortable, you
 *                        will need to do it here.
 *
 *****************************************************************************/

GType filetreemodel_get_type(void)
{
	static GType filetreemodel_type = 0;

	/* Some boilerplate type registration stuff */
	if (filetreemodel_type == 0) {
		static const GTypeInfo filetreemodel_info = {
			sizeof(FileTreemodelClass),
			NULL,				/* base_init */
			NULL,				/* base_finalize */
			(GClassInitFunc) filetreemodel_class_init,
			NULL,				/* class finalize */
			NULL,				/* class_data */
			sizeof(FileTreemodel),
			0,					/* n_preallocs */
			(GInstanceInitFunc) filetreemodel_init
		};
		static const GInterfaceInfo tree_model_info = {
			(GInterfaceInitFunc) filetreemodel_tree_model_init,
			NULL,
			NULL
		};

		/* First register the new derived type with the GObject type system */
		filetreemodel_type = g_type_register_static(G_TYPE_OBJECT, "FileTreemodel",
													&filetreemodel_info, (GTypeFlags) 0);

		/* Now register our GtkTreeModel interface with the type system */
		g_type_add_interface_static(filetreemodel_type, GTK_TYPE_TREE_MODEL, &tree_model_info);
	}

	return filetreemodel_type;
}


/*****************************************************************************
 *
 *  filetreemodel_class_init: more boilerplate GObject/GType stuff.
 *                          Init callback for the type system,
 *                          called once when our new class is created.
 *
 *****************************************************************************/

static void filetreemodel_class_init(FileTreemodelClass * klass)
{
	GObjectClass *object_class;

	parent_class = (GObjectClass *) g_type_class_peek_parent(klass);
	object_class = (GObjectClass *) klass;

	object_class->finalize = filetreemodel_finalize;
}

/*****************************************************************************
 *
 *  filetreemodel_tree_model_init: init callback for the interface registration
 *                               in filetreemodel_get_type. Here we override
 *                               the GtkTreeModel interface functions that
 *                               we implement.
 *
 *****************************************************************************/

static void filetreemodel_tree_model_init(GtkTreeModelIface * iface)
{
	iface->get_flags = filetreemodel_get_flags;
	iface->get_n_columns = filetreemodel_get_n_columns;
	iface->get_column_type = filetreemodel_get_column_type;
	iface->get_iter = filetreemodel_get_iter;
	iface->get_path = filetreemodel_get_path;
	iface->get_value = filetreemodel_get_value;
	iface->iter_next = filetreemodel_iter_next;
	iface->iter_children = filetreemodel_iter_children;
	iface->iter_has_child = filetreemodel_iter_has_child;
	iface->iter_n_children = filetreemodel_iter_n_children;
	iface->iter_nth_child = filetreemodel_iter_nth_child;
	iface->iter_parent = filetreemodel_iter_parent;
}


/*****************************************************************************
 *
 *  filetreemodel_init: this is called everytime a new filetree model object
 *                    instance is created (we do that in filetreemodel_new).
 *                    Initialise the list structure's fields here.
 *
 *****************************************************************************/

static void filetreemodel_init(FileTreemodel * ftm)
{
	ftm->n_columns = filetreemodel_N_COLUMNS;

	ftm->column_types[0] = G_TYPE_POINTER;	/* filetreemodel_COL_RECORD    */
	ftm->column_types[1] = G_TYPE_STRING;	/* filetreemodel_COL_NAME      */
	ftm->column_types[2] = G_TYPE_STRING;	/* filetreemodel_COL_ICON_NAME      */
	ftm->column_types[3] = G_TYPE_UINT;	/* filetreemodel_COL_WEIGHT */

	g_assert(filetreemodel_N_COLUMNS == 4);

	ftm->num_rows = 0;
	ftm->rows = NULL;

	ftm->alluri = g_hash_table_new(g_file_hash, (GEqualFunc) g_file_equal);

	ftm->stamp = g_random_int();	/* Random int to check whether an iter belongs to our model */

}

/*****************************************************************************
 *
 *  filetreemodel_finalize: this is called just before a filetree model is
 *                        destroyed. Free dynamically allocated memory here.
 *
 *****************************************************************************/

static void filetreemodel_finalize(GObject * object)
{
	GList *tmplist;
	FileTreemodel *ftm = filetreemodel(object);
	gint i;
	/* free all records and free all memory used by the list */
	for (i = 0; i < ftm->num_rows; i++) {
		record_recursive_cleanup(ftm->rows[i]);
	}

	g_hash_table_destroy(ftm->alluri);

	tmplist = g_list_first(ftm->uri_in_refresh);
	DEBUG_MSG("fb2config_cleanup, stopping all async directory reads\n");
	while (tmplist) {
		Turi_in_refresh *uir = tmplist->data;
		g_cancellable_cancel(uir->cancel);
		g_object_unref(uir->uri);
		g_slice_free(Turi_in_refresh, uir);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(ftm->uri_in_refresh);

	/* must chain up - finalize parent */
	(*parent_class->finalize) (object);
}


/*****************************************************************************
 *
 *  filetreemodel_get_flags: tells the rest of the world whether our tree model
 *                         has any special characteristics. In our case,
 *                         we have a list model (instead of a tree), and each
 *                         tree iter is valid as long as the row in question
 *                         exists, as it only contains a pointer to our struct.
 *
 *****************************************************************************/

static GtkTreeModelFlags filetreemodel_get_flags(GtkTreeModel * tree_model)
{
	g_return_val_if_fail(IS_FILETREE_MODEL(tree_model), (GtkTreeModelFlags) 0);

	return (GTK_TREE_MODEL_ITERS_PERSIST);
}


/*****************************************************************************
 *
 *  filetreemodel_get_n_columns: tells the rest of the world how many data
 *                             columns we export via the tree model interface
 *
 *****************************************************************************/

static gint filetreemodel_get_n_columns(GtkTreeModel * tree_model)
{
	g_return_val_if_fail(IS_FILETREE_MODEL(tree_model), 0);

	return filetreemodel(tree_model)->n_columns;
}


/*****************************************************************************
 *
 *  filetreemodel_get_column_type: tells the rest of the world which type of
 *                               data an exported model column contains
 *
 *****************************************************************************/

static GType filetreemodel_get_column_type(GtkTreeModel * tree_model, gint index)
{
	g_return_val_if_fail(IS_FILETREE_MODEL(tree_model), G_TYPE_INVALID);
	g_return_val_if_fail(index < filetreemodel(tree_model)->n_columns && index >= 0, G_TYPE_INVALID);

	return filetreemodel(tree_model)->column_types[index];
}
