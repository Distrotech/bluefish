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
void file_delete_file_async(GnomeVFSURI *uri, DeleteAsyncCallback callback, gpointer callback_data);

typedef enum {
	CHECKMODIFIED_ERROR,
	CHECKMODIFIED_CANCELLED,
	CHECKMODIFIED_MODIFIED,
	CHECKMODIFIED_OK
} Tcheckmodified_status;

typedef void (* CheckmodifiedAsyncCallback) (Tcheckmodified_status status,gint error_info,GnomeVFSFileInfo *orig, GnomeVFSFileInfo *new, gpointer callback_data);

typedef struct {
	GnomeVFSAsyncHandle *handle;
	GList *uris;
	GnomeVFSFileInfo *orig_finfo;
	CheckmodifiedAsyncCallback callback_func;
	gpointer callback_data;
} Tcheckmodified;

void checkmodified_cancel(Tcheckmodified * cm);
Tcheckmodified *file_checkmodified_uri_async(GnomeVFSURI *uri, GnomeVFSFileInfo *curinfo, CheckmodifiedAsyncCallback callback_func, gpointer callback_data);

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
	GnomeVFSAsyncHandle *handle;
	GnomeVFSFileSize buffer_size;
	Trefcpointer *buffer;
	GnomeVFSURI *uri;
	SavefileAsyncCallback callback_func;
	gpointer callback_data;
} Tsavefile;

Tsavefile *file_savefile_uri_async(GnomeVFSURI *uri, Trefcpointer *buffer, GnomeVFSFileSize buffer_size, SavefileAsyncCallback callback_func, gpointer callback_data);

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
gpointer file_checkNsave_uri_async(GnomeVFSURI *uri, GnomeVFSFileInfo *info, Trefcpointer *buffer, GnomeVFSFileSize buffer_size, gboolean check_modified, CheckNsaveAsyncCallback callback_func, gpointer callback_data);

typedef enum {
	OPENFILE_ERROR,
	OPENFILE_ERROR_NOCHANNEL,
	OPENFILE_ERROR_NOREAD,
	OPENFILE_ERROR_CANCELLED,
	OPENFILE_CHANNEL_OPENED,
	OPENFILE_FINISHED
} Topenfile_status;

typedef void (* OpenfileAsyncCallback) (Topenfile_status status,gint error_info, gchar *buffer,GnomeVFSFileSize buflen,gpointer callback_data);

typedef struct {
	GnomeVFSURI *uri;
	GnomeVFSAsyncHandle *handle;
	gchar *buffer;
	GnomeVFSFileSize buffer_size;
	GnomeVFSFileSize used_size;
	OpenfileAsyncCallback callback_func;
	gpointer callback_data;
} Topenfile;
void openfile_cancel(Topenfile *of);
Topenfile *file_openfile_uri_async(GnomeVFSURI *uri, OpenfileAsyncCallback callback_func, gpointer callback_data);

void file2doc_cancel(gpointer f2d);
void file_asyncfileinfo_cancel(gpointer fi);
void file_doc_fill_fileinfo(Tdocument *doc, GnomeVFSURI *uri);
void file_doc_retry_uri(Tdocument *doc);
void file_doc_fill_from_uri(Tdocument *doc, GnomeVFSURI *uri, GnomeVFSFileInfo *finfo, gint goto_line);
void file_doc_from_uri(Tbfwin *bfwin, GnomeVFSURI *uri, GnomeVFSFileInfo *finfo, gint goto_line, gint goto_offset);
void file_docs_from_uris(Tbfwin *bfwin, GSList *urislist);
void file_into_doc(Tdocument *doc, GnomeVFSURI *uri, gboolean isTemplate);
void open_advanced(Tbfwin *bfwin, GnomeVFSURI *basedir, gboolean recursive, gchar *extension_filter, gchar *content_filter, gboolean use_regex);
void copy_files_async(Tbfwin *bfwin, GnomeVFSURI *destdir, gchar *sources);
#endif /* __FILE_H_ */
