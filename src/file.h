/* Bluefish HTML Editor
 * file.h
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __FILE_H_
#define __FILE_H_

#include "bf_lib.h"

typedef enum {
	CHECKANDSAVE_ERROR,
	CHECKANDSAVE_ERROR_NOBACKUP,
	CHECKANDSAVE_ERROR_NOCHANNEL,
	CHECKANDSAVE_ERROR_NOWRITE,
	CHECKANDSAVE_ERROR_MODIFIED,
	CHECKANDSAVE_CHECKED,
	CHECKANDSAVE_BACKUP,
	CHECKANDSAVE_CHANNEL_OPENED,
	CHECKANDSAVE_FINISHED
} TcheckNsave_status;

typedef enum {
	CHECKNSAVE_STOP,
	CHECKNSAVE_CONT
} TcheckNsave_return;

typedef enum {
	CHECKMODIFIED_ERROR,
	CHECKMODIFIED_MODIFIED,
	CHECKMODIFIED_OK
} Tcheckmodified_status;

typedef struct {
	GnomeVFSAsyncHandle *handle;
} Tfileaction; /* we can typecast all different actions that start with a GnomeVFSAsyncHandle * as Tfileaction */

typedef void (* CheckmodifiedAsyncCallback) (Tcheckmodified_status status,gint error_info,GnomeVFSFileInfo *orig, GnomeVFSFileInfo *new, gpointer callback_data);
typedef TcheckNsave_return (* CheckNsaveAsyncCallback) (TcheckNsave_status status,gint error_info,gpointer callback_data);

void file_checkmodified_uri_async(GnomeVFSURI *uri, GnomeVFSFileInfo *curinfo, CheckmodifiedAsyncCallback callback_func, gpointer callback_data);
void file_checkNsave_uri_async(GnomeVFSURI *uri, GnomeVFSFileInfo *info, Trefcpointer *buffer, GnomeVFSFileSize buffer_size, gboolean check_modified, CheckNsaveAsyncCallback callback_func, gpointer callback_data);
void file_doc_fill_fileinfo(Tdocument *doc, GnomeVFSURI *uri);
void file_doc_retry_uri(Tdocument *doc);
void file_doc_fill_from_uri(Tdocument *doc, GnomeVFSURI *uri, GnomeVFSFileInfo *finfo, gint goto_line);
void file_doc_from_uri(Tbfwin *bfwin, GnomeVFSURI *uri, GnomeVFSFileInfo *finfo, gint goto_line, gint goto_offset);
void file_docs_from_uris(Tbfwin *bfwin, GSList *urislist);
void file_into_doc(Tdocument *doc, GnomeVFSURI *uri);
void open_advanced(Tbfwin *bfwin, GnomeVFSURI *basedir, gboolean recursive, gchar *extension_filter, gchar *content_filter, gboolean use_regex);

#endif /* __FILE_H_ */
