/* Bluefish HTML Editor
 * file.h - file operations based on GnomeVFS
 *
 * Copyright (C) 2005 Olivier Sessink
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __FILE_H_
#define __FILE_H_

#include "bf_lib.h"

typedef void (* DeleteAsyncCallback) (gpointer callback_data);
#ifdef HAVE_ATLEAST_GIO_2_16
void file_delete_file_async(GFile *uri, DeleteAsyncCallback callback, gpointer callback_data);
#else
void file_delete_file_async(GnomeVFSURI *uri, DeleteAsyncCallback callback, gpointer callback_data);
#endif
typedef enum {
	CHECKMODIFIED_ERROR,
	CHECKMODIFIED_CANCELLED,
	CHECKMODIFIED_MODIFIED,
	CHECKMODIFIED_OK
} Tcheckmodified_status;

#ifdef HAVE_ATLEAST_GIO_2_16
typedef void (* CheckmodifiedAsyncCallback) (Tcheckmodified_status status,gint error_info,GFileInfo *orig, GFileInfo *new, gpointer callback_data);
#else /* GnomeVFS */
typedef void (* CheckmodifiedAsyncCallback) (Tcheckmodified_status status,gint error_info,GnomeVFSFileInfo *orig, GnomeVFSFileInfo *new, gpointer callback_data);
#endif /* #else HAVE_ATLEAST_GIO_2_16 */

typedef struct {
#ifdef HAVE_ATLEAST_GIO_2_16
	GCancellable *handle;
	GFileInfo *orig_finfo;
	GFile *uri;
#else
	GnomeVFSAsyncHandle *handle;
	GnomeVFSFileInfo *orig_finfo;
	GList *uris;
#endif
	CheckmodifiedAsyncCallback callback_func;
	gpointer callback_data;
} Tcheckmodified;

void checkmodified_cancel(Tcheckmodified * cm);

#ifdef HAVE_ATLEAST_GIO_2_16
Tcheckmodified *file_checkmodified_uri_async(GFile *uri, GFileInfo *curinfo, CheckmodifiedAsyncCallback callback_func, gpointer callback_data);
#else /* GnomeVFS */
Tcheckmodified *file_checkmodified_uri_async(GnomeVFSURI *uri, GnomeVFSFileInfo *curinfo, CheckmodifiedAsyncCallback callback_func, gpointer callback_data);
#endif /* #else HAVE_ATLEAST_GIO_2_16 */
typedef enum {
	SAVEFILE_ERROR,
	SAVEFILE_ERROR_NOCHANNEL,
	SAVEFILE_ERROR_NOWRITE,
	SAVEFILE_ERROR_NOCLOSE,
	SAVEFILE_ERROR_CANCELLED,
	SAVEFILE_CHANNEL_OPENED,
	SAVEFILE_WRITTEN,
	SAVEFILE_FINISHED
} Tsavefile_status;

typedef void (* SavefileAsyncCallback) (Tsavefile_status status,gint error_info,gpointer callback_data);

typedef struct {
#ifdef HAVE_ATLEAST_GIO_2_16
	GCancellable *handle;
	goffset buffer_size;
	GFile *uri;
#else /* GnomeVFS */	
	GnomeVFSAsyncHandle *handle;
	GnomeVFSFileSize buffer_size;
	GnomeVFSURI *uri;
#endif /* #else HAVE_ATLEAST_GIO_2_16 */
	Trefcpointer *buffer;
	SavefileAsyncCallback callback_func;
	gpointer callback_data;
} Tsavefile;
#ifdef HAVE_ATLEAST_GIO_2_16
Tsavefile *file_savefile_uri_async(GFile *uri, Trefcpointer *buffer, goffset buffer_size, SavefileAsyncCallback callback_func, gpointer callback_data);
#else /* GnomeVFS */
Tsavefile *file_savefile_uri_async(GnomeVFSURI *uri, Trefcpointer *buffer, GnomeVFSFileSize buffer_size, SavefileAsyncCallback callback_func, gpointer callback_data);
#endif /* #else HAVE_ATLEAST_GIO_2_16 */
typedef enum {
	CHECKANDSAVE_ERROR,
	CHECKANDSAVE_ERROR_NOBACKUP,
	CHECKANDSAVE_ERROR_NOCHANNEL,
	CHECKANDSAVE_ERROR_NOWRITE,
	CHECKANDSAVE_ERROR_MODIFIED,
	CHECKANDSAVE_ERROR_MODIFIED_FAILED,
	CHECKANDSAVE_ERROR_CANCELLED,
	CHECKANDSAVE_CHECKED,
	CHECKANDSAVE_BACKUP,
	CHECKANDSAVE_CHANNEL_OPENED,
	CHECKANDSAVE_FINISHED
} TcheckNsave_status;

typedef enum {
	CHECKNSAVE_STOP,
	CHECKNSAVE_CONT
} TcheckNsave_return;

typedef TcheckNsave_return (* CheckNsaveAsyncCallback) (TcheckNsave_status status,gint error_info,gpointer callback_data);

#ifdef HAVE_ATLEAST_GIO_2_16
GFile *backup_uri_from_orig_uri(GFile * origuri);
gpointer file_checkNsave_uri_async(GFile *uri, GFileInfo *info, Trefcpointer *buffer, goffset buffer_size, gboolean check_modified, CheckNsaveAsyncCallback callback_func, gpointer callback_data);
#else /* GnomeVFS */
GnomeVFSURI *backup_uri_from_orig_uri(GnomeVFSURI * origuri);
gpointer file_checkNsave_uri_async(GnomeVFSURI *uri, GnomeVFSFileInfo *info, Trefcpointer *buffer, GnomeVFSFileSize buffer_size, gboolean check_modified, CheckNsaveAsyncCallback callback_func, gpointer callback_data);
#endif /* #else HAVE_ATLEAST_GIO_2_16 */


typedef enum {
	OPENFILE_ERROR,
	OPENFILE_ERROR_NOCHANNEL,
	OPENFILE_ERROR_NOREAD,
	OPENFILE_ERROR_CANCELLED,
	OPENFILE_CHANNEL_OPENED,
	OPENFILE_FINISHED
} Topenfile_status;

#ifdef HAVE_ATLEAST_GIO_2_16
typedef void (* OpenfileAsyncCallback) (Topenfile_status status,gint error_info, gchar *buffer,goffset buflen,gpointer callback_data);
#else /* GnomeVFS */
typedef void (* OpenfileAsyncCallback) (Topenfile_status status,gint error_info, gchar *buffer,GnomeVFSFileSize buflen,gpointer callback_data);
#endif /* #else HAVE_ATLEAST_GIO_2_16 */
typedef struct {
#ifdef HAVE_ATLEAST_GIO_2_16
	GFile *uri;
	GCancellable *handle;
#else /* GnomeVFS */
	GnomeVFSURI *uri;
	GnomeVFSAsyncHandle *handle;
	GnomeVFSFileSize buffer_size;
	GnomeVFSFileSize used_size;
	gchar *buffer;
#endif /* #else HAVE_ATLEAST_GIO_2_16 */
	OpenfileAsyncCallback callback_func;
	gpointer callback_data;
} Topenfile;
void openfile_cancel(Topenfile *of);
#ifdef HAVE_ATLEAST_GIO_2_16
Topenfile *file_openfile_uri_async(GFile *uri, OpenfileAsyncCallback callback_func, gpointer callback_data);
#else /* GnomeVFS */
Topenfile *file_openfile_uri_async(GnomeVFSURI *uri, OpenfileAsyncCallback callback_func, gpointer callback_data);
#endif /* #else HAVE_ATLEAST_GIO_2_16 */

void file2doc_cancel(gpointer f2d);
void file_asyncfileinfo_cancel(gpointer fi);
#ifdef HAVE_ATLEAST_GIO_2_16
void file_doc_fill_fileinfo(Tdocument *doc, GFile *uri);
void file_doc_fill_from_uri(Tdocument *doc, GFile *uri, GFileInfo *finfo, gint goto_line);
void file_doc_from_uri(Tbfwin *bfwin, GFile *uri, GFileInfo *finfo, gint goto_line, gint goto_offset, gboolean readonly);
void file_into_doc(Tdocument *doc, GFile *uri, gboolean isTemplate);
void open_advanced(Tbfwin *bfwin, GFile *basedir, gboolean recursive, gboolean matchname, gchar *name_filter, gchar *content_filter, gboolean use_regex);
void copy_files_async(Tbfwin *bfwin, GFile *destdir, gchar *sources);
#else /* GnomeVFS */
void file_doc_fill_fileinfo(Tdocument *doc, GnomeVFSURI *uri);
void file_doc_fill_from_uri(Tdocument *doc, GnomeVFSURI *uri, GnomeVFSFileInfo *finfo, gint goto_line);
void file_doc_from_uri(Tbfwin *bfwin, GnomeVFSURI *uri, GnomeVFSFileInfo *finfo, gint goto_line, gint goto_offset, gboolean readonly);
void file_into_doc(Tdocument *doc, GnomeVFSURI *uri, gboolean isTemplate);
void open_advanced(Tbfwin *bfwin, GnomeVFSURI *basedir, gboolean recursive, gboolean matchname, gchar *name_filter, gchar *content_filter, gboolean use_regex);
void copy_files_async(Tbfwin *bfwin, GnomeVFSURI *destdir, gchar *sources);
#endif /* #else HAVE_ATLEAST_GIO_2_16 */
void file_doc_retry_uri(Tdocument *doc);
void file_docs_from_uris(Tbfwin *bfwin, GSList *urislist);

#endif /* __FILE_H_ */
