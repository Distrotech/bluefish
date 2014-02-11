/* Bluefish HTML Editor
 * file.h - file operations based on GnomeVFS
 *
 * Copyright (C) 2005,2006,2007,2008 Olivier Sessink
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

#ifndef __FILE_H_
#define __FILE_H_

#include "bf_lib.h"

void DEBUG_URI(GFile * uri, gboolean newline);

void file_static_queues_init(void);

typedef void (*DeleteAsyncCallback) (gpointer callback_data);
void file_delete_async(GFile * uri, gboolean recursive, DeleteAsyncCallback callback, gpointer callback_data);
typedef enum {
	CHECKMODIFIED_ERROR,
	CHECKMODIFIED_CANCELLED,
	CHECKMODIFIED_MODIFIED,
	CHECKMODIFIED_OK
} Tcheckmodified_status;

typedef void (*CheckmodifiedAsyncCallback) (Tcheckmodified_status status, GError * gerror, GFileInfo * orig,
											GFileInfo * new, gpointer callback_data);

typedef struct {
	GCancellable *cancel;
	GFileInfo *orig_finfo;
	GFile *uri;
	CheckmodifiedAsyncCallback callback_func;
	gpointer callback_data;
} Tcheckmodified;

void checkmodified_cancel(Tcheckmodified * cm);

Tcheckmodified *file_checkmodified_uri_async(GFile * uri, GFileInfo * curinfo,
											 CheckmodifiedAsyncCallback callback_func,
											 gpointer callback_data);
/*
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
	GCancellable *handle;
	goffset buffer_size;
	GFile *uri;
	Trefcpointer *buffer;
	SavefileAsyncCallback callback_func;
	gpointer callback_data;
} Tsavefile;
Tsavefile *file_savefile_uri_async(GFile *uri, Trefcpointer *buffer, gsize buffer_size, SavefileAsyncCallback callback_func, gpointer callback_data);
*/
typedef enum {
	CHECKANDSAVE_ERROR,
	CHECKANDSAVE_ERROR_NOBACKUP,
	CHECKANDSAVE_ERROR_NOWRITE,
	CHECKANDSAVE_ERROR_MODIFIED,
	CHECKANDSAVE_ERROR_CANCELLED,
	CHECKANDSAVE_FINISHED
} TcheckNsave_status;

typedef enum {
	CHECKNSAVE_STOP,
	CHECKNSAVE_CONT
} TcheckNsave_return;

typedef TcheckNsave_return(*CheckNsaveAsyncCallback) (TcheckNsave_status status, GError * gerror,
													  gpointer callback_data);

GFile *backup_uri_from_orig_uri(GFile * origuri);
void file_checkNsave_cancel(gpointer cns);
gpointer file_checkNsave_uri_async(GFile * uri, GFileInfo * info, Trefcpointer * buffer, gsize buffer_size,
								   gboolean check_modified, gboolean backup,
								   CheckNsaveAsyncCallback callback_func, gpointer callback_data);

typedef enum {
	OPENFILE_ERROR,
	OPENFILE_ERROR_NOCHANNEL,
	OPENFILE_ERROR_NOREAD,
	OPENFILE_ERROR_CANCELLED,
	OPENFILE_CHANNEL_OPENED,
	OPENFILE_FINISHED
} Topenfile_status;

typedef void (*OpenfileAsyncCallback) (Topenfile_status status, GError * gerror, Trefcpointer * buffer,
									   goffset buflen, gpointer callback_data);
typedef struct {
	GFile *uri;
	Tbfwin *bfwin;				/* MAY BE NULL !!!!!!!!!!!!!!!!!!!!!!!!!!! */
	GCancellable *cancel;
	OpenfileAsyncCallback callback_func;
	gpointer callback_data;
} Topenfile;
void openfile_cancel(Topenfile * of);
Topenfile *file_openfile_uri_async(GFile * uri, Tbfwin * bfwin, OpenfileAsyncCallback callback_func,
								   gpointer callback_data);

void file2doc_cancel(gpointer f2d);
void file_asyncfileinfo_cancel(gpointer fi);
void file_doc_fill_fileinfo(Tdocument * doc, GFile * uri);
void file_doc_fill_from_uri(Tdocument * doc, GFile * uri, GFileInfo * finfo, gint goto_line);
void file_doc_from_uri(Tbfwin * bfwin, GFile * uri, GFile * recover_uri, GFileInfo * finfo, gint goto_line,
					   gint goto_offset, gboolean readonly, gint cursor_offset, gboolean align_center, gboolean load_first);
void file_into_doc(Tdocument * doc, GFile * uri, gboolean isTemplate, gboolean untiledRecovery);
gpointer findfiles(GFile *basedir, gboolean recursive, guint max_recursion, gboolean matchname,
			  gchar * name_filter, GCallback filematch_cb, GCallback finished_cb, gpointer data);
void findfiles_cancel(gpointer data);

gboolean open_advanced(Tbfwin * bfwin, GFile * basedir, gboolean recursive, guint max_recursion,
					   gboolean matchname, gchar * name_filter, gchar * content_filter, gboolean use_regex,
					   GError ** reterror);
void copy_uris_async(Tbfwin * bfwin, GFile * destdir, GSList * sources);
void copy_files_async(Tbfwin * bfwin, GFile * destdir, gchar * sources);
void file_doc_retry_uri(Tdocument * doc);
void file_docs_from_uris(Tbfwin * bfwin, GSList * urislist);

typedef void (*SyncProgressCallback) (GFile *uri, gint total, gint done, gint failed, gpointer user_data);
void sync_directory(GFile * basedir, GFile * targetdir, gboolean delete_deprecated, gboolean include_hidden,
					SyncProgressCallback progress_callback, gpointer callback_data);
							/* __FILE_H_ */
void file_handle(GFile * uri, Tbfwin * bfwin, gchar * mimetype, gboolean explicit_open);
#endif							/* __FILE_H_ */
