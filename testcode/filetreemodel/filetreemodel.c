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

static void filetreemodel_init(FileTreemodel * filetreemodel)
{
	filetreemodel->n_columns = filetreemodel_N_COLUMNS;

	filetreemodel->column_types[0] = G_TYPE_POINTER;	/* filetreemodel_COL_RECORD    */
	filetreemodel->column_types[1] = G_TYPE_STRING;	/* filetreemodel_COL_NAME      */
	filetreemodel->column_types[2] = G_TYPE_UINT;	/* filetreemodel_COL_YEAR_BORN */

	g_assert(filetreemodel_N_COLUMNS == 3);

	filetreemodel->num_rows = 0;
	filetreemodel->rows = NULL;

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
	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), (GtkTreeModelFlags) 0);

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
	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), 0);

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
	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), G_TYPE_INVALID);
	g_return_val_if_fail(index < filetreemodel(tree_model)->n_columns && index >= 0, G_TYPE_INVALID);

	return filetreemodel(tree_model)->column_types[index];
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
	gint *indices, n, depth;

	g_assert(CUSTOM_IS_LIST(tree_model));
	g_assert(path != NULL);
	g_print("filetreemodel_get_iter\n");
	filetreemodel = filetreemodel(tree_model);

	indices = gtk_tree_path_get_indices(path);
	depth = gtk_tree_path_get_depth(path);

	/* we do not allow children */
	g_assert(depth == 1);		/* depth 1 = top level; a list only has top level nodes and no children */

	n = indices[0];				/* the n-th top level row */

	if (n >= filetreemodel->num_rows || n < 0)
		return FALSE;

	record = filetreemodel->rows[n];

	g_assert(record != NULL);
	g_assert(record->pos == n);

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

	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), NULL);
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

	g_return_if_fail(CUSTOM_IS_LIST(tree_model));
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
		g_value_set_uint(value, record->icon_name);
		break;
	}
	case filetreemodel_COL_ICON_WEIGHT:
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

	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), FALSE);
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

	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), FALSE);

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

	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), -1);
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

	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), FALSE);
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

void filetreemodel_append_record(FileTreemodel * filetreemodel, const gchar * name, guint year_born)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	UriRecord *newrecord;
	gulong newsize;
	guint pos;

	g_return_if_fail(CUSTOM_IS_LIST(filetreemodel));
	g_return_if_fail(name != NULL);

	pos = filetreemodel->num_rows;

	filetreemodel->num_rows++;

	newsize = filetreemodel->num_rows * sizeof(UriRecord *);

	filetreemodel->rows = g_realloc(filetreemodel->rows, newsize);

	newrecord = g_new0(UriRecord, 1);

	newrecord->name = g_strdup(name);
	newrecord->name_collate_key = g_utf8_collate_key(name, -1);	/* for fast sorting, used later */
	newrecord->year_born = year_born;

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
