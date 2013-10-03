#include "filetreemodel.h"


/* boring declarations of local functions */

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



static GObjectClass *parent_class = NULL;	/* GObject stuff - nothing to worry about */


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

static void filetreemodel_init(FileTreemodel * filetreemodel)
{
	filetreemodel->n_columns = filetreemodel_N_COLUMNS;

	filetreemodel->column_types[0] = G_TYPE_POINTER;	/* filetreemodel_COL_RECORD    */
	filetreemodel->column_types[1] = G_TYPE_STRING;	/* filetreemodel_COL_NAME      */
	filetreemodel->column_types[2] = G_TYPE_STRING;	/* filetreemodel_COL_ICON_NAME      */
	filetreemodel->column_types[3] = G_TYPE_UINT;	/* filetreemodel_COL_WEIGHT */

	g_assert(filetreemodel_N_COLUMNS == 4);

	filetreemodel->num_rows = 0;
	filetreemodel->rows = NULL;
	
	filetreemodel->alluri = g_hash_table_new_full(g_file_hash, (GEqualFunc) g_file_equal, uri_hash_destroy, iter_value_destroy);

	filetreemodel->stamp = g_random_int();	/* Random int to check whether an iter belongs to our model */

}


/*****************************************************************************
 *
 *  filetreemodel_finalize: this is called just before a filetree model is
 *                        destroyed. Free dynamically allocated memory here.
 *
 *****************************************************************************/

static void filetreemodel_finalize(GObject * object)
{
/*  FileTreemodel *filetreemodel = filetreemodel(object); */

	/* free all records and free all memory used by the list */
#warning IMPLEMENT

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


static UriRecord *
get_nth_record(FileTreemodel * ftm, UriRecord *precord, gint n)
{
	if (precord) {
		if (n > precord->num_rows) {
			g_critical("requested a record beyond the end\n");
			return NULL;
		}
		return precord->rows[n];
	}
	return ftm->rows[n];
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
	FileTreemodel *filetreemodel;
	UriRecord *record;
	gint *indices, i, depth;

	g_assert(IS_FILETREE_MODEL(tree_model));
	g_assert(path != NULL);
	g_print("filetreemodel_get_iter\n");
	filetreemodel = filetreemodel(tree_model);

	indices = gtk_tree_path_get_indices(path);
	depth = gtk_tree_path_get_depth(path);
	
	record=NULL;
	for (i=0;i<depth;i++) {
		record = get_nth_record(filetreemodel, record, indices[i]);
		if (!record)
			return FALSE;
	}

	/* We simply store a pointer to our custom record in the iter */
	iter->stamp = filetreemodel->stamp;
	iter->user_data = record;
	iter->user_data2 = NULL;	/* unused */
	iter->user_data3 = NULL;	/* unused */

	return TRUE;
}


/*****************************************************************************
 *
 *  filetreemodel_get_path: converts a tree iter into a tree path (ie. the
 *                        physical position of that row in the list).
 *
 *****************************************************************************/

static GtkTreePath *filetreemodel_get_path(GtkTreeModel * tree_model, GtkTreeIter * iter)
{
	GtkTreePath *path;
	UriRecord *record;
	FileTreemodel *filetreemodel;

	g_return_val_if_fail(IS_FILETREE_MODEL(tree_model), NULL);
	g_return_val_if_fail(iter != NULL, NULL);
	g_return_val_if_fail(iter->user_data != NULL, NULL);

	filetreemodel = filetreemodel(tree_model);

	
	record = (UriRecord *) iter->user_data;

	path = gtk_tree_path_new();
	gtk_tree_path_append_index(path, record->pos);

	return path;
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
	FileTreemodel *filetreemodel;

	g_return_if_fail(IS_FILETREE_MODEL(tree_model));
	g_return_if_fail(iter != NULL);
	g_return_if_fail(column < filetreemodel(tree_model)->n_columns);

	g_value_init(value, filetreemodel(tree_model)->column_types[column]);

	filetreemodel = filetreemodel(tree_model);

	record = (UriRecord *) iter->user_data;

	g_return_if_fail(record != NULL);

	if (record->pos >= filetreemodel->num_rows)
		g_return_if_reached();

	switch (column) {
	case filetreemodel_COL_RECORD:
		g_value_set_pointer(value, record);
		break;

	case filetreemodel_COL_NAME:
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
	FileTreemodel *filetreemodel;

	g_return_val_if_fail(IS_FILETREE_MODEL(tree_model), FALSE);
	g_print("filetreemodel_iter_next\n");
	if (iter == NULL || iter->user_data == NULL)
		return FALSE;

	filetreemodel = filetreemodel(tree_model);

	record = (UriRecord *) iter->user_data;

	/* Is this the last record in the list? */
	if ((record->pos + 1) >= filetreemodel->num_rows)
		return FALSE;

	nextrecord = filetreemodel->rows[(record->pos + 1)];

	g_assert(nextrecord != NULL);
	g_assert(nextrecord->pos == (record->pos + 1));

	iter->stamp = filetreemodel->stamp;
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
	FileTreemodel *filetreemodel;

	g_return_val_if_fail(parent == NULL || parent->user_data != NULL, FALSE);

	/* this is a list, nodes have no children */
	if (parent)
		return FALSE;

	/* parent == NULL is a special case; we need to return the first top-level row */

	g_return_val_if_fail(IS_FILETREE_MODEL(tree_model), FALSE);

	filetreemodel = filetreemodel(tree_model);

	/* No rows => no first row */
	if (filetreemodel->num_rows == 0)
		return FALSE;

	/* Set iter to first item in list */
	iter->stamp = filetreemodel->stamp;
	iter->user_data = filetreemodel->rows[0];

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
	return FALSE;
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
	FileTreemodel *filetreemodel;

	g_return_val_if_fail(IS_FILETREE_MODEL(tree_model), -1);
	g_return_val_if_fail(iter == NULL || iter->user_data != NULL, FALSE);

	filetreemodel = filetreemodel(tree_model);

	/* special case: if iter == NULL, return number of top-level rows */
	if (!iter)
		return filetreemodel->num_rows;

	return 0;					/* otherwise, this is easy again for a list */
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
	FileTreemodel *filetreemodel;

	g_return_val_if_fail(IS_FILETREE_MODEL(tree_model), FALSE);
	g_print("filetreemodel_iter_nth_child(%d)\n",n);
	filetreemodel = filetreemodel(tree_model);

	/* a list has only top-level rows */
	if (parent)
		return FALSE;

	/* special case: if parent == NULL, set iter to n-th top-level row */

	if (n >= filetreemodel->num_rows)
		return FALSE;

	record = filetreemodel->rows[n];

	g_assert(record != NULL);
	g_assert(record->pos == n);

	iter->stamp = filetreemodel->stamp;
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
	return FALSE;
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
 *  filetreemodel_append_record:  Empty lists are boring. This function can
 *                              be used in your own code to add rows to the
 *                              list. Note how we emit the "row-inserted"
 *                              signal after we have appended the row
 *                              internally, so the tree view and other
 *                              interested objects know about the new row.
 *
 *****************************************************************************/

void filetreemodel_append_record(FileTreemodel * filetreemodel, const gchar * name, const gchar *icon_name)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	UriRecord *newrecord;
	gulong newsize;
	guint pos;

	g_return_if_fail(IS_FILETREE_MODEL(filetreemodel));
	g_return_if_fail(name != NULL);

	pos = filetreemodel->num_rows;

	filetreemodel->num_rows++;

	newsize = filetreemodel->num_rows * sizeof(UriRecord *);

	filetreemodel->rows = g_realloc(filetreemodel->rows, newsize);

	newrecord = g_slice_new0(UriRecord);

	newrecord->name = g_strdup(name);
	newrecord->name_collate_key = g_utf8_collate_key(name, -1);	/* for fast sorting, used later */
	newrecord->icon_name = g_strdup(icon_name);
	newrecord->weight = 0;

	filetreemodel->rows[pos] = newrecord;
	newrecord->pos = pos;

	/* inform the tree view and other interested objects
	 *  (e.g. tree row references) that we have inserted
	 *  a new row, and where it was inserted */

	path = gtk_tree_path_new();
	gtk_tree_path_append_index(path, newrecord->pos);

	filetreemodel_get_iter(GTK_TREE_MODEL(filetreemodel), &iter, path);

	gtk_tree_model_row_inserted(GTK_TREE_MODEL(filetreemodel), path, &iter);

	gtk_tree_path_free(path);
}

gboolean gfile_uri_is_parent(GFile *parent, GFile *child, gboolean recursive) {
	gboolean retval = FALSE;
	GFile *tmp, *tmp2;

	if (parent == NULL) {
		return FALSE;
	}
	if (child == NULL) {
		return FALSE;
	}
	g_object_ref(child);
	tmp2=child;
	do {
		tmp = g_file_get_parent(tmp2);
		g_object_unref(tmp2);
		if (tmp == NULL) {
			break;
		}
		retval = g_file_equal(tmp,parent);
		tmp2 = tmp;
	} while (recursive == TRUE && retval != TRUE);
	if (tmp) {
		g_object_unref(tmp);
	}
	return retval;
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
		icon_name = g_strdup("folder");
	}
	return icon_name;
}


gchar *gfile_display_name(GFile *uri, GFileInfo *finfo) {
	gchar *retval;
	if (finfo && g_file_info_has_attribute(finfo,G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME)) {
		retval = g_strdup(g_file_info_get_display_name(finfo));
	} else {
		GFileInfo *finfo2;

		finfo2 = g_file_query_info(uri,G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,G_FILE_QUERY_INFO_NONE,NULL,NULL);
		retval = g_strdup(g_file_info_get_display_name(finfo2));
		g_object_unref(finfo2);
	}
	return retval;
}



static UriRecord * 
add_single_uri(FileTreemodel * filetreemodel, UriRecord *record, GFile * child_uri, GFileInfo * finfo)
{
	guint newsize, pos;
	UriRecord *newrecord;
	GIcon *icon;
	GtkTreeIter iter;
	GtkTreePath *path;

	newrecord = g_slice_new0(UriRecord);
	newrecord->name = gfile_display_name(child_uri, finfo);
	newrecord->name_collate_key = g_utf8_collate_key(newrecord->name, -1);	/* for fast sorting, used later */
	
	icon = g_file_info_get_icon(finfo);
	newrecord->icon_name = icon_name_from_icon(icon);;
	newrecord->weight = 0;
	
	/* actually add it to the array now */
	if (record) {
		pos = record->num_rows;
		record->num_rows++;
		newsize = record->num_rows * sizeof(UriRecord *);
		record->rows = g_realloc(record->rows, newsize);
		record->rows[pos] = newrecord;
		newrecord->pos = pos;
	} else {
		/* add to toplevel */
		pos = filetreemodel->num_rows;
		filetreemodel->num_rows++;
		newsize = filetreemodel->num_rows * sizeof(UriRecord *);
		filetreemodel->rows = g_realloc(filetreemodel->rows, newsize);
		filetreemodel->rows[pos] = newrecord;
		newrecord->pos = pos;
	}
	
	/* inform the tree view and other interested objects
	 *  (e.g. tree row references) that we have inserted
	 *  a new row, and where it was inserted */

	path = gtk_tree_path_new();
	gtk_tree_path_append_index(path, newrecord->pos);
	filetreemodel_get_iter(GTK_TREE_MODEL(filetreemodel), &iter, path);
	gtk_tree_model_row_inserted(GTK_TREE_MODEL(filetreemodel), path, &iter);
	gtk_tree_path_free(path);
	return newrecord;
}

static gchar *
get_toplevel_name(GFile * uri)
{
	GError *gerror = NULL;
	GMount *gmnt;
	gchar *name;
	gmnt = g_file_find_enclosing_mount(uri, NULL, &gerror);
	if (!gerror && gmnt) {
		name = g_mount_get_name(gmnt);
		g_object_unref(gmnt);
	} else {
		name = g_file_get_basename(uri);
		if (gerror)
			g_error_free(gerror);
	}
	return name;
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

UriRecord *filetreemodel_build_dir(FileTreemodel * filetreemodel, GFile *uri)
{
	UriRecord *record=NULL;
	
	GFile *tmp, *parent_uri = NULL;
	tmp = uri;

	if (!uri)
		return;

	/* first find if any directory part of this uri exists already in the treestore */
	g_object_ref(tmp);

	while (record == NULL) {
		GFile *tmp2 = g_file_get_parent(tmp);
		if (tmp2 == NULL) {
			GFileInfo *finfo;
			gchar *name;
			name = get_toplevel_name(tmp);
			/* there was no parent for this filesystem yet */
			finfo = fake_directory_fileinfo(name);
			record = add_single_uri(filetreemodel, NULL, tmp, finfo);
			g_free(name);
			break;
		} else {
			g_object_unref(tmp);
		}
		tmp = tmp2;
		record = g_hash_table_lookup(filetreemodel->alluri, tmp);
	}							/* after this loop 'tmp' is newly allocated */
	parent_uri = tmp;

#ifdef DEBUG
	if (parent_uri == NULL) {
		g_critical("parent_uri should not be NULL\n");
		g_return_val_if_reached(NULL);
	}
#endif
	/* now loop to create all directories in the treestore that were not in the treestore yet */
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
				g_return_val_if_reached(NULL);	/* TODO: must be g_return_val_if_reached() or handle the situation */
			}
			g_object_unref(tmp2);
			tmp2 = tmp3;
		}					/* after this loop both 'parent_uri'='tmp' and 'tmp2' are newly allocated */

		finfo = fake_directory_fileinfo_for_uri(tmp2);
		record = add_single_uri(filetreemodel, record, tmp2, finfo);
		if (g_file_equal(tmp2, uri)) {
			GFile *dummy_uri;
			GFileInfo *dummy_finfo;
			/* add a dummy item so the expander will be shown */
			dummy_uri = g_file_get_child(tmp2, " ");
			dummy_finfo = fake_directory_fileinfo(" ");

			add_single_uri(filetreemodel, record, dummy_uri, dummy_finfo);
			g_object_unref(dummy_uri);
			g_object_unref(dummy_finfo);
		}
		g_object_unref(finfo);
		g_object_unref(parent_uri);
		parent_uri = tmp2;	/* here 'parent_uri'='tmp2' is newly allocated */
	
		if (g_file_equal(parent_uri, uri)) {
			done = TRUE;
		}
	}
	g_object_unref(parent_uri);	/* no memory leaks in the uri's... (I hope) */
	return record;
}

