#ifndef _filetreemodel_h_included_
#define _filetreemodel_h_included_

#include <gtk/gtk.h>

/* Some boilerplate GObject defines. 'klass' is used
 *   instead of 'class', because 'class' is a C++ keyword */

#define FILETREE_TYPE_LIST            (filetreemodel_get_type ())
#define filetreemodel(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FILETREE_TYPE_LIST, FileTreemodel))
#define filetreemodel_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  FILETREE_TYPE_LIST, FileTreemodelClass))
#define CUSTOM_IS_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FILETREE_TYPE_LIST))
#define CUSTOM_IS_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  FILETREE_TYPE_LIST))
#define filetreemodel_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  FILETREE_TYPE_LIST, FileTreemodelClass))

/* The data columns that we export via the tree model interface */

enum {
	filetreemodel_COL_RECORD = 0,
	filetreemodel_COL_NAME,
	filetreemodel_COL_ICON_NAME,
	filetreemodel_COL_WEIGHT,
	filetreemodel_N_COLUMNS,
};


typedef struct _UriRecord UriRecord;
typedef struct _DirRecord DirRecord;
typedef struct _FileTreemodel FileTreemodel;
typedef struct _FileTreemodelClass FileTreemodelClass;

/* UriRecord: this structure represents a file */

struct _UriRecord {
	/* visible columns */
	gchar *name;
	gchar *icon_name;
	gboolean weight;

	/* internal data */
	GFile *uri;
	GFileInfo *finfo;
	gchar *name_collate_key;
	guint num_rows;
	UriRecord **rows;

	guint pos;					/* pos within the array */
};

struct _DirRecord {

};



/* FileTreemodel: this structure contains everything we need for our
 *             model implementation. You can add extra fields to
 *             this structure, e.g. hashtables to quickly lookup
 *             rows or whatever else you might need, but it is
 *             crucial that 'parent' is the first member of the
 *             structure.                                          */

struct _FileTreemodel {
	GObject parent;				/* this MUST be the first member */

	guint num_rows;/* the toplevel */
	UriRecord **rows;

	/* These two fields are not absolutely necessary, but they    */
	/*   speed things up a bit in our get_value implementation    */
	gint n_columns;
	GType column_types[filetreemodel_N_COLUMNS];

	gint stamp;					/* Random integer to check whether an iter belongs to our model */
};



/* FileTreemodelClass: more boilerplate GObject stuff */

struct _FileTreemodelClass {
	GObjectClass parent_class;
};


GType filetreemodel_get_type(void);

FileTreemodel *filetreemodel_new(void);

void filetreemodel_append_record(FileTreemodel * filetreemodel, const gchar * name, guint year_born);

#endif							/* _filetreemodel_h_included_ */
