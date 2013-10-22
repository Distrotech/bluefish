#ifndef _filetreemodel_h_included_
#define _filetreemodel_h_included_

#include <gtk/gtk.h>

/* Some boilerplate GObject defines. 'klass' is used
 *   instead of 'class', because 'class' is a C++ keyword */

#define FILETREE_TYPE_LIST            (filetreemodel_get_type ())
#define filetreemodel(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FILETREE_TYPE_LIST, FileTreemodel))
#define filetreemodel_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  FILETREE_TYPE_LIST, FileTreemodelClass))
#define IS_FILETREE_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FILETREE_TYPE_LIST))
#define IS_FILETREE_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  FILETREE_TYPE_LIST))
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
typedef struct _FileTreemodel FileTreemodel;
typedef struct _FileTreemodelClass FileTreemodelClass;

/* UriRecord: this structure represents a file */

struct _UriRecord {
	gchar *name;
	gchar *icon_name;
	gchar *fast_content_type; /* copied from GFileInfo */
	/* internal data */
	GFile *uri;
	UriRecord *parent;
	UriRecord **rows;
	guint16 num_rows;
	guint16 pos;					/* pos within the array */
	guint16 weight; /* An enumeration specifying the weight (boldness) of a font. This is a numerical value ranging from 100 to 900,*/

	guint8 isdir;
	guint8 possibly_deleted;
};
/*
on 64 bit systems: 6*8bytes + 3*2bytes + 2*1byte = 56 bytes
on 32 bit systems: 6*4bytes + 3*2bytes + 2*1byte = 32 bytes
*/

typedef void (*DirChangedCallback) (FileTreemodel * ftm, GFile *dir_uri, gpointer data);

typedef struct {
	DirChangedCallback func;
	gpointer user_data;
} Tdirchangedlistener; 


/* FileTreemodel: this structure contains everything we need for our
 *             model implementation. You can add extra fields to
 *             this structure, e.g. hashtables to quickly lookup
 *             rows or whatever else you might need, but it is
 *             crucial that 'parent' is the first member of the
 *             structure.                                          */

struct _FileTreemodel {
	GObject parent;				/* this MUST be the first member */

	guint16 num_rows;/* the toplevel */
	UriRecord **rows;

	/* These two fields are not absolutely necessary, but they    */
	/*   speed things up a bit in our get_value implementation    */
	gint n_columns;
	GType column_types[filetreemodel_N_COLUMNS];
	GHashTable *alluri;
	GList *uri_in_refresh;
	GList *dirchangedlisteners;
	gint stamp;					/* Random integer to check whether an iter belongs to our model */
};



/* FileTreemodelClass: more boilerplate GObject stuff */

struct _FileTreemodelClass {
	GObjectClass parent_class;
};


GType filetreemodel_get_type(void);

FileTreemodel *filetreemodel_new(void);
gboolean uri_in_refresh(FileTreemodel * ftm, GFile * uri);
UriRecord *filetreemodel_build_dir(FileTreemodel * ftm, GFile * uri, GtkTreeIter *iter);
void filetreemodel_refresh_iter_async(FileTreemodel * ftm, GtkTreeIter * iter);
void filetreemodel_refresh_uri_async(FileTreemodel * ftm, GFile * uri);
gboolean filetree_get_iter_for_uri(FileTreemodel * ftm, GFile * uri, GtkTreeIter *iter);
void filetreemodel_add_file(FileTreemodel * ftm, GFile * uri, const gchar *content_type, guint16 weight);
void filetreemodel_set_weight(FileTreemodel * ftm, GFile * uri, guint16 weight);
void filetreemodel_set_icon(FileTreemodel * ftm, GFile * uri, const gchar *icon_name);
void filetreemodel_dirchange_register(FileTreemodel * ftm, DirChangedCallback func, gpointer user_data);
void filetreemodel_dirchange_unregister_by_data(FileTreemodel * ftm, gpointer user_data);
#define DIR_MIME_TYPE "inode/directory"


#endif							/* _filetreemodel_h_included_ */
