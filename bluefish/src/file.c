/* Bluefish HTML Editor
 * file.c --> file operations based on GnomeVFS
 *
 * Copyright (C) 2002-2005 Olivier Sessink
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
#define DEBUG

#include <gtk/gtk.h>
#include <string.h> /* memcpy */
#include "bluefish.h"
#include "bf_lib.h"
#include "stringlist.h"
#include "document.h"
#include "file.h"
#include "file_dialogs.h"
#include "gui.h"
#include "bookmark.h"

/*************************** FILE INFO ASYNC ******************************/

static void checkmodified_cleanup(Tcheckmodified *cm) {
	gnome_vfs_uri_unref(cm->uris->data);
	g_list_free(cm->uris);
	gnome_vfs_file_info_unref(cm->orig_finfo);
	g_free(cm);
}

void checkmodified_cancel(Tcheckmodified * cm) {
	gnome_vfs_async_cancel(cm->handle);
	cm->callback_func(CHECKMODIFIED_CANCELLED, 0, NULL, NULL, cm->callback_data);
	checkmodified_cleanup(cm);
}

static gboolean checkmodified_is_modified(GnomeVFSFileInfo *orig, GnomeVFSFileInfo *new) {
	/* modified_check_type;  0=no check, 1=by mtime and size, 2=by mtime, 3=by size, 4,5,...not implemented (md5sum?) */
/*	DEBUG_MSG("checkmodified_is_modified, matches=%d\n",gnome_vfs_file_info_matches(orig,new));*/
/*	if (gnome_vfs_file_info_matches(orig,new)) {
		/ * this is a test of all the fields * /
		return FALSE;
	}*/
	if (main_v->props.modified_check_type == 1 || main_v->props.modified_check_type == 2) {
/*		DEBUG_MSG("checkmodified_is_modified, mtime1=%d, mtime2=%d\n",orig->mtime, new->mtime);*/
		if (orig->mtime != new->mtime) return TRUE;
	}
	if (main_v->props.modified_check_type == 1 || main_v->props.modified_check_type == 3) {
		/* DEBUG_MSG("checkmodified_is_modified, 1validfields=%d, 2validfields=%d, size1=%d, size2=%d\n",orig->valid_fields,new->valid_fields,orig->size, new->size);*/
		DEBUG_MSG("checkmodified_is_modified, matches=%d\n",gnome_vfs_file_info_matches(orig,new));
		if (orig->size != new->size) return TRUE;
	}
	return FALSE;
}

static void checkmodified_asyncfileinfo_lcb(GnomeVFSAsyncHandle *handle, GList *results, /* GnomeVFSGetFileInfoResult* items */gpointer data) {
	GnomeVFSGetFileInfoResult* item;
	Tcheckmodified *cm = data;
	DEBUG_MSG("checkmodified_asyncfileinfo_lcb, with %d results\n",g_list_length(results));
	item = results->data;
	if (item->result == GNOME_VFS_OK) {
		if (checkmodified_is_modified(cm->orig_finfo, item->file_info)) {
			DEBUG_MSG("checkmodified_asyncfileinfo_lcb, calling callback with MODIFIED\n");
			cm->callback_func(CHECKMODIFIED_MODIFIED, item->result, cm->orig_finfo, item->file_info, cm->callback_data);
		} else {
			DEBUG_MSG("checkmodified_asyncfileinfo_lcb, calling callback with OK\n");
			cm->callback_func(CHECKMODIFIED_OK, item->result, cm->orig_finfo, item->file_info, cm->callback_data);
		}
	} else {
		cm->callback_func(CHECKMODIFIED_ERROR, item->result, NULL, NULL, cm->callback_data);
	}	
	checkmodified_cleanup(cm);
}

Tcheckmodified * file_checkmodified_uri_async(GnomeVFSURI *uri, GnomeVFSFileInfo *curinfo, CheckmodifiedAsyncCallback callback_func, gpointer callback_data) {
	Tcheckmodified *cm;
	DEBUG_MSG("file_checkmodified_uri_async, STARTED for %s\n", gnome_vfs_uri_get_path(uri));
	if (curinfo == NULL) {
		callback_func(CHECKMODIFIED_OK, 0, NULL, NULL, callback_data);
		return NULL;
	}
	cm = g_new(Tcheckmodified,1);
	cm->callback_func = callback_func;
	cm->callback_data = callback_data;
	gnome_vfs_uri_ref(uri);
	cm->uris = g_list_append(NULL, uri);
	gnome_vfs_file_info_ref(curinfo);
	cm->orig_finfo = curinfo;
	gnome_vfs_async_get_file_info(&cm->handle,cm->uris,GNOME_VFS_FILE_INFO_DEFAULT|GNOME_VFS_FILE_INFO_FOLLOW_LINKS
			,GNOME_VFS_PRIORITY_DEFAULT,checkmodified_asyncfileinfo_lcb,cm);
	return cm;
}
/*************************** SAVE FILE ASYNC ******************************/
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
	SavefileAsyncCallback callback_func;
	gpointer callback_data;
} Tsavefile;

static void savefile_cleanup(Tsavefile *sf) {
	DEBUG_MSG("savefile_cleanup, called for %p\n",sf);
	refcpointer_unref(sf->buffer);
	g_free(sf);
}

static void savefile_cancel(Tsavefile *sf) {
	gnome_vfs_async_cancel(sf->handle);
	sf->callback_func(SAVEFILE_ERROR_CANCELLED, 0, sf->callback_data);
	savefile_cleanup(sf);
}

static void savefile_asyncclose_lcb(GnomeVFSAsyncHandle *handle,GnomeVFSResult result,gpointer data) {
	Tsavefile *sf = data;
	if (result == GNOME_VFS_OK) {
		DEBUG_MSG("savefile_asyncclose_lcb, closed!\n");
		sf->callback_func(SAVEFILE_FINISHED, result, sf->callback_data);
	} else {
		DEBUG_MSG("savefile_asyncclose_lcb, close error!\n");
		sf->callback_func(SAVEFILE_ERROR_NOCLOSE, result, sf->callback_data);
	}
	savefile_cleanup(sf);
}

static void savefile_asyncwrite_lcb(GnomeVFSAsyncHandle *handle,GnomeVFSResult result,gconstpointer buffer,GnomeVFSFileSize bytes_requested,GnomeVFSFileSize bytes_written,gpointer data) {
	Tsavefile *sf = data;
	DEBUG_MSG("savefile_asyncwrite_lcb, called with result=%d, %"GNOME_VFS_SIZE_FORMAT_STR" bytes written\n",result,bytes_written);
	if (result == GNOME_VFS_OK) {
#ifdef DEBUG
		if (sf->buffer_size != bytes_written || bytes_written != bytes_requested) {
			DEBUG_MSG("savefile_asyncwrite_lcb, WARNING, LESS BYTES WRITTEN THEN THE BUFFER HAS\n");
		}
#endif
		gnome_vfs_async_close(handle, savefile_asyncclose_lcb, sf);
	} else {
		DEBUG_MSG("savefile_asyncwrite_lcb, error while writing (%d), calling SAVEFILE_ERROR_NOWRITE and cleaning up\n", result);
		sf->callback_func(SAVEFILE_ERROR_NOWRITE, result, sf->callback_data);
		savefile_cleanup(sf);
	}
}

static void savefile_asyncopenuri_lcb(GnomeVFSAsyncHandle *handle,GnomeVFSResult result,gpointer data) {
	Tsavefile *sf = data;
	DEBUG_MSG("savefile_asyncopenuri_lcb, called with result=%d\n",result);
	if (result == GNOME_VFS_OK) {
		sf->callback_func(SAVEFILE_CHANNEL_OPENED, result, sf->callback_data);
		gnome_vfs_async_write(handle,sf->buffer->data,sf->buffer_size,savefile_asyncwrite_lcb, sf);
	} else {
		/* error! */
		sf->callback_func(SAVEFILE_ERROR_NOCHANNEL, result, sf->callback_data);
		savefile_cleanup(sf);
	}
}

static Tsavefile *file_savefile_uri_async(GnomeVFSURI *uri, Trefcpointer *buffer, GnomeVFSFileSize buffer_size, SavefileAsyncCallback callback_func, gpointer callback_data) {
	Tsavefile *sf;
	sf = g_new(Tsavefile,1);
	sf->callback_data = callback_data;
	sf->callback_func = callback_func;
	sf->buffer = buffer;
	refcpointer_ref(buffer);
	sf->buffer_size = buffer_size;
	gnome_vfs_async_create_uri(&sf->handle,uri,GNOME_VFS_OPEN_WRITE, FALSE,0644,GNOME_VFS_PRIORITY_DEFAULT
				,savefile_asyncopenuri_lcb,sf);
	return sf;
}

/*************************** CHECK MODIFIED AND SAVE ASYNC ******************************/

typedef struct {
	GnomeVFSAsyncHandle *handle; /* to cancel backups */
	Tsavefile *sf; /* to cancel the actual save */
	Tcheckmodified *cm; /* to cancel the checkmodified check */
	GnomeVFSFileSize buffer_size;
	Trefcpointer *buffer;
	GnomeVFSURI *uri;
	GnomeVFSFileInfo *finfo;
	gboolean check_modified;
	CheckNsaveAsyncCallback callback_func;
	gpointer callback_data;
	gboolean abort; /* the backup callback may set this to true, it means that the user choosed to abort save because the backup failed */
} TcheckNsave;

static void checkNsave_cleanup(TcheckNsave *cns) {
	DEBUG_MSG("checkNsave_cleanup, called for %p\n",cns);
	refcpointer_unref(cns->buffer);
	gnome_vfs_uri_unref(cns->uri);
	gnome_vfs_file_info_unref(cns->finfo);
	g_free(cns);
}

static void checkNsave_cancel(TcheckNsave *cns) {
	if (cns->sf) {
		savefile_cancel(cns->sf);
		/* do not cleanup, we will get another callback */
	} else if (cns->handle) {
		gnome_vfs_async_cancel(cns->handle);
		cns->callback_func(CHECKANDSAVE_ERROR_CANCELLED, 0, cns->callback_data);
		checkNsave_cleanup(cns);
	} else if (cns->cm) {
		checkmodified_cancel(cns->cm);
		/* do not cleanup, we will get another callback */
	}
}

static void checkNsave_savefile_lcb(Tsavefile_status status,gint error_info,gpointer data) {
	TcheckNsave *cns = data;
	DEBUG_MSG("checkNsave_savefile_lcb, started with status=%d\n",status);
	switch (status) {
	case SAVEFILE_FINISHED:
		DEBUG_MSG("checkNsave_savefile_lcb, calling CHECKANDSAVE_FINISHED\n");
		cns->callback_func(CHECKANDSAVE_FINISHED, error_info, cns->callback_data);
		checkNsave_cleanup(cns);
	break;
	case SAVEFILE_WRITTEN:
		/* do nothing */
	break;
	case SAVEFILE_CHANNEL_OPENED:
		cns->callback_func(CHECKANDSAVE_CHANNEL_OPENED, error_info, cns->callback_data);
	break;
	case SAVEFILE_ERROR_NOWRITE:
	case SAVEFILE_ERROR_NOCLOSE:
		cns->callback_func(CHECKANDSAVE_ERROR_NOWRITE, error_info, cns->callback_data);
		checkNsave_cleanup(cns);
	break;
	case SAVEFILE_ERROR_NOCHANNEL:
		cns->callback_func(CHECKANDSAVE_ERROR_NOCHANNEL, error_info, cns->callback_data);
		checkNsave_cleanup(cns);
	break;
	case SAVEFILE_ERROR_CANCELLED:
		cns->callback_func(CHECKANDSAVE_ERROR_CANCELLED, error_info, cns->callback_data);
		checkNsave_cleanup(cns);
	break;
	case SAVEFILE_ERROR:
		cns->callback_func(CHECKANDSAVE_ERROR, error_info, cns->callback_data);
		checkNsave_cleanup(cns);
	break;
	}
}

static gint checkNsave_progress_lcb(GnomeVFSAsyncHandle *handle,GnomeVFSXferProgressInfo *info,gpointer data) {
	TcheckNsave *cns = data;
	DEBUG_MSG("checkNsave_progress_lcb, started with status %d and phase %d for source %s and target %s, index=%ld, total=%ld, thread=%p\n"
			,info->status,info->phase,info->source_name,info->target_name,info->file_index,info->files_total, g_thread_self());
	if (info->status == GNOME_VFS_XFER_PROGRESS_STATUS_OVERWRITE) {
		DEBUG_MSG("checkNsave_progress_lcb, status=OVERWRITE, return REPLACE\n");
		return GNOME_VFS_XFER_OVERWRITE_ACTION_REPLACE;
	} else if (info->status == GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR) {
		DEBUG_MSG("checkNsave_progress_lcb, status=VFSERROR, abort?\n");
		/* when this code was in the 'sync' callback, this results in "Xlib: unexpected async reply" which seems to be a gnome_vfs bug */
		if (cns->callback_func(CHECKANDSAVE_ERROR_NOBACKUP, 0, cns->callback_data) == CHECKNSAVE_CONT) {
			return GNOME_VFS_XFER_ERROR_ACTION_SKIP;
		} else {
			cns->abort = TRUE;
			return GNOME_VFS_XFER_ERROR_ACTION_ABORT;
		}
	} else if (info->status == GNOME_VFS_XFER_PROGRESS_STATUS_OK) {
		DEBUG_MSG("checkNsave_progress_lcb, status=OK, phase=%d\n",info->phase);
		if (info->phase == GNOME_VFS_XFER_PHASE_COMPLETED) {
			/* backup == ok, we start the actual file save */
			if (cns->abort) {
				DEBUG_MSG("checkNsave_progress_lcb, phase=COMPLETED, but abort=TRUE, so we stop now\n");
				checkNsave_cleanup(cns);
			} else {
				DEBUG_MSG("checkNsave_progress_lcb, phase=COMPLETED, starting the actual save\n");
				cns->sf = file_savefile_uri_async(cns->uri, cns->buffer, cns->buffer_size, checkNsave_savefile_lcb, cns);
			}
		}
	}
	return 1; 	/* Nautilus returns 1 by default for this callback */
}

/*
 this function is called in the wrong thread by gnome_vfs, so we have to avoid threading issues:
 <gicmo> Oli4, the first thing is to use gdk_thread_enter () gdk_thread_leave ()
 <Oli4> I suppose in the start and end of the callback?
 <gicmo> Oli4, the second thing is you could shedule a message with g_idle_add in the callback and wait for the result through g_thread_cond_wait
 <gicmo> Oli4, yeah .. look at the gtk doc for some examples how to use them ..
 <Oli4> ok
 <gicmo> Oli4, nautilus/libnautilus-private/nautilus-file-operations.c could be a place you should look at
*/
static gint checkNsave_sync_lcb(GnomeVFSXferProgressInfo *info,gpointer data) {
   DEBUG_MSG("checkNsave_sync_lcb, started with status %d and phase %d for source %s and target %s, index=%ld, total=%ld, thread=%p\n"
			,info->status,info->phase,info->source_name,info->target_name,info->file_index,info->files_total, g_thread_self());
	/* Christian Kellner (gicmo on #nautilus on irc.gimp.ca) found 
		we should NEVER return 0 for default calls, it aborts!! 
		
		nautilus returns *always* 1 in this callback, no matter what happens */
	return 1;
}
static void checkNsave_checkmodified_lcb(Tcheckmodified_status status,gint error_info, GnomeVFSFileInfo *orig, GnomeVFSFileInfo *new, gpointer data) {
	TcheckNsave *cns = data;
	gboolean startbackup = FALSE;
	DEBUG_MSG("checkNsave_checkmodified_lcb, status=%d\n",status);
	switch (status) {
	case CHECKMODIFIED_OK:
		startbackup = TRUE;
	break;
	case CHECKMODIFIED_MODIFIED:
		startbackup = (cns->callback_func(CHECKANDSAVE_ERROR_MODIFIED, error_info, cns->callback_data) == CHECKNSAVE_CONT);
	break;
	case CHECKMODIFIED_ERROR:
		cns->callback_func(CHECKANDSAVE_ERROR, error_info, cns->callback_data);
	break;
	case CHECKMODIFIED_CANCELLED:
		cns->callback_func(CHECKANDSAVE_ERROR_CANCELLED, error_info, cns->callback_data);
	break;
	}
	if (startbackup) {
		GList *sourcelist;
		GList *destlist;
		gchar *tmp, *tmp2;
		GnomeVFSURI *dest;
		GnomeVFSResult ret;
		/* now first create the backup, then start save */
		tmp = gnome_vfs_uri_to_string(cns->uri,0);
		tmp2 = g_strconcat(tmp, main_v->props.backup_filestring, NULL);
		dest = gnome_vfs_uri_new(tmp2);
		g_free(tmp);
		g_free(tmp2);
		sourcelist = g_list_append(NULL, cns->uri);
		gnome_vfs_uri_ref(cns->uri);
		destlist = g_list_append(NULL, dest);
		DEBUG_MSG("checkNsave_checkmodified_lcb, start backup, source=%s, dest=%s (len=%d,%d) in thread %p\n",gnome_vfs_uri_get_path(cns->uri),gnome_vfs_uri_get_path(dest)
				,g_list_length(sourcelist),g_list_length(destlist),g_thread_self());
		ret = gnome_vfs_async_xfer(&cns->handle,sourcelist,destlist
					,GNOME_VFS_XFER_FOLLOW_LINKS,GNOME_VFS_XFER_ERROR_MODE_QUERY
					,GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,GNOME_VFS_PRIORITY_DEFAULT
					,checkNsave_progress_lcb, cns
					,checkNsave_sync_lcb, cns);
		DEBUG_MSG("checkNsave_checkmodified_lcb, ret ok=%d\n",(ret == GNOME_VFS_OK));
		gnome_vfs_uri_list_free(sourcelist);
		gnome_vfs_uri_list_free(destlist);
/*		gnome_vfs_uri_unref(dest);
		g_list_free(sourcelist);
		g_list_free(destlist);*/
	} else {
		checkNsave_cleanup(cns);
	}
}

/**
 * file_checkNsave_uri_async:
 * @uri: #GnomeVFSURI*
 * @info: #GnomeVFSFileInfo* or NULL if the info on-disk is unknown (new file??)
 * @buffer: #Trefcpointer* with the contents of to be saved to uri
 * @buffer_size: #GnomeVFSFileSize 
 * @check_modified: #gboolean (ignored if 'info' is NULL)
 * @callback_func: #CheckNsaveAsyncCallback
 * @callback_data: #gpointer
 *
 * checks if the target uri is modified, if not creates a backup, then writes 
 * the buffer; all done async
 */
void file_checkNsave_uri_async(GnomeVFSURI *uri, GnomeVFSFileInfo *info, Trefcpointer *buffer, GnomeVFSFileSize buffer_size, gboolean check_modified, CheckNsaveAsyncCallback callback_func, gpointer callback_data) {
	TcheckNsave *cns;
	DEBUG_MSG("file_checkNsave_uri_async, started for (%p) %s with callback_data %p\n", uri, gnome_vfs_uri_get_path(uri),callback_data);
	cns = g_new0(TcheckNsave,1);
	cns->callback_data = callback_data;
	cns->callback_func = callback_func;
	cns->buffer = buffer;
	refcpointer_ref(buffer);
	cns->buffer_size = buffer_size;
	cns->uri = uri;
	gnome_vfs_uri_ref(uri);
	cns->finfo = info;
	cns->check_modified = check_modified;
	cns->abort = FALSE;
	if (info) gnome_vfs_file_info_ref(info);
	if (!info || check_modified) {
		/* first check if the file is modified on disk */
		cns->cm = file_checkmodified_uri_async(uri, info, checkNsave_checkmodified_lcb, cns);
	} else {
		checkNsave_checkmodified_lcb(CHECKMODIFIED_OK,0,NULL, NULL, cns);
	}
}

/*************************** OPEN FILE ASYNC ******************************/

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
	GnomeVFSAsyncHandle *handle;
	gchar *buffer;
	GnomeVFSFileSize buffer_size;
	GnomeVFSFileSize used_size;
	OpenfileAsyncCallback callback_func;
	gpointer callback_data;
} Topenfile;
#define CHUNK_SIZE 4096
#define BUFFER_INCR_SIZE 40960

static void openfile_cleanup(Topenfile *of) {
	DEBUG_MSG("openfile_cleanup %p\n",of);
	g_free(of->buffer);
	g_free(of);
}
static void openfile_cancel(Topenfile *of) {
	DEBUG_MSG("openfile_cancel, of=%p\n",of);
	gnome_vfs_async_cancel(of->handle);
	of->callback_func(OPENFILE_ERROR_CANCELLED,0,of->buffer,of->used_size,of->callback_data);
	openfile_cleanup(of);
}
static void openfile_asyncclose_lcb(GnomeVFSAsyncHandle *handle,GnomeVFSResult result,gpointer data) {
	Topenfile *of = data;
	DEBUG_MSG("openfile_asyncclose_lcb, cleaning %p\n",of);
	openfile_cleanup(of);
}

static void openfile_asyncread_lcb(GnomeVFSAsyncHandle *handle,GnomeVFSResult result,gpointer buffer
							,GnomeVFSFileSize bytes_requested,GnomeVFSFileSize bytes_read ,gpointer data) {
	Topenfile *of = data;
	if (result == GNOME_VFS_OK) {
		of->used_size += bytes_read;
		DEBUG_MSG("openfile_asyncread_lcb, read %"GNOME_VFS_SIZE_FORMAT_STR" bytes, in total %"GNOME_VFS_SIZE_FORMAT_STR"\n",bytes_read,of->used_size);
		if ((of->used_size + CHUNK_SIZE) > of->buffer_size) {
			of->buffer_size += BUFFER_INCR_SIZE;
			of->buffer = g_realloc(of->buffer, of->buffer_size+1);/* the +1 is so we can add a \0 to the buffer */
		}
		gnome_vfs_async_read(handle,of->buffer+of->used_size,CHUNK_SIZE,openfile_asyncread_lcb,of);
	} else if (result == GNOME_VFS_ERROR_EOF) {
		DEBUG_MSG("openfile_asyncread_lcb, EOF after %lld bytes\n",of->used_size);
		of->buffer[of->used_size] = '\0';
		of->callback_func(OPENFILE_FINISHED,result,of->buffer,of->used_size, of->callback_data);
		gnome_vfs_async_close(handle,openfile_asyncclose_lcb,of);
	} else {
		DEBUG_MSG("openfile_asyncread_lcb, error?? result=%d\n",result);
		/* should we call close, or the cleanup function now? */
	}
}

static void openfile_asyncopenuri_lcb(GnomeVFSAsyncHandle *handle,GnomeVFSResult result,gpointer data) {
	Topenfile *of = data;
	if (result == GNOME_VFS_OK) {
		of->callback_func(OPENFILE_CHANNEL_OPENED,result,NULL,0,of->callback_data);
		gnome_vfs_async_read(handle,of->buffer,CHUNK_SIZE,openfile_asyncread_lcb,of);
	} else {
		of->callback_func(OPENFILE_ERROR_NOCHANNEL, result, NULL,0, of->callback_data);
		openfile_cleanup(of);
	}
}

static Topenfile *file_openfile_uri_async(GnomeVFSURI *uri, OpenfileAsyncCallback callback_func, gpointer callback_data) {
	Topenfile *of;
	of = g_new(Topenfile,1);
	DEBUG_MSG("file_open_uri_async, %s, of=%p\n",gnome_vfs_uri_get_path(uri), of);
	of->callback_data = callback_data;
	of->callback_func = callback_func;
	of->buffer_size = BUFFER_INCR_SIZE;
	of->used_size = 0;
	of->buffer = g_malloc(of->buffer_size+1); /* the +1 is so we can add a \0 to the buffer */
	gnome_vfs_async_open_uri(&of->handle,uri,GNOME_VFS_OPEN_READ,GNOME_VFS_PRIORITY_DEFAULT
				,openfile_asyncopenuri_lcb,of);
	return of;
}
/************ LOAD A FILE ASYNC INTO A DOCUMENT ************************/
typedef struct {
	Tbfwin *bfwin;
	Tdocument *doc;
	GnomeVFSURI *uri;
} Tfileintodoc;

static void fileintodoc_cleanup(Tfileintodoc *fid) {
	gnome_vfs_uri_unref(fid->uri);
	g_free(fid);
}

static void fileintodoc_lcb(Topenfile_status status,gint error_info,gchar *buffer,GnomeVFSFileSize buflen ,gpointer data) {
	Tfileintodoc *fid = data;
	switch (status) {
		case OPENFILE_FINISHED:
			doc_buffer_to_textbox(fid->doc, buffer, buflen, FALSE, TRUE);
			doc_set_status(fid->doc, DOC_STATUS_COMPLETE);
			fid->doc->action.load = NULL;
			fileintodoc_cleanup(data);
		break;
		case OPENFILE_ERROR_CANCELLED: /* hmm what to do here ? */
			doc_buffer_to_textbox(fid->doc, buffer, buflen, FALSE, TRUE);
			fid->doc->action.load = NULL;
			fileintodoc_cleanup(data);
		break;
		case OPENFILE_CHANNEL_OPENED:
			/* do nothing */
		break;
		case OPENFILE_ERROR:
		case OPENFILE_ERROR_NOCHANNEL:
		case OPENFILE_ERROR_NOREAD:
			DEBUG_MSG("fileitodoc_lcb, ERROR status=%d, cleanup!!!!!\n",status);
			fid->doc->action.load = NULL;
			fileintodoc_cleanup(data);
		break;
	}
}

void file_into_doc(Tdocument *doc, GnomeVFSURI *uri) {
	Tfileintodoc *fid;
	fid = g_new(Tfileintodoc,1);
	fid->bfwin = doc->bfwin;
	fid->doc = doc;
	doc_set_status(doc, DOC_STATUS_LOADING);
	fid->uri = uri;
	gnome_vfs_uri_ref(uri);
	file_openfile_uri_async(fid->uri,fileintodoc_lcb,fid);
}

/************ MAKE DOCUMENT FROM ASYNC OPENED FILE ************************/
typedef struct {
	Topenfile *of;
	Tbfwin *bfwin;
	Tdocument *doc;
	GnomeVFSURI *uri;
} Tfile2doc;

static void file2doc_cleanup(Tfile2doc *f2d) {
	DEBUG_MSG("file2doc_cleanup, %p\n",f2d);
	gnome_vfs_uri_unref(f2d->uri);
	g_free(f2d);
}

void file2doc_cancel(gpointer f2d) {
	DEBUG_MSG("file2doc_cancel, called for %p\n",f2d);
	openfile_cancel(((Tfile2doc *)f2d)->of);
	/* no cleanup, there is a CANCELLED callback coming */
}

static void file2doc_lcb(Topenfile_status status,gint error_info,gchar *buffer,GnomeVFSFileSize buflen ,gpointer data) {
	Tfile2doc *f2d = data;
	DEBUG_MSG("file2doc_lcb, status=%d, f2d=%p\n",status,f2d);
	switch (status) {
		case OPENFILE_FINISHED:
			DEBUG_MSG("file2doc_lcb, status=%d, now we should convert %s data into a GtkTextBuffer and such\n",status, gnome_vfs_uri_get_path(f2d->uri));
			doc_buffer_to_textbox(f2d->doc, buffer, buflen, FALSE, TRUE);
			doc_reset_filetype(f2d->doc, f2d->doc->uri, buffer);
			doc_set_status(f2d->doc, DOC_STATUS_COMPLETE);
			bmark_set_for_doc(f2d->doc);
			bmark_check_length(f2d->bfwin,f2d->doc);
			DEBUG_MSG("file2doc_lcb, focus_next_new_doc=%d\n",f2d->bfwin->focus_next_new_doc);
			if (f2d->bfwin->focus_next_new_doc) {
				f2d->bfwin->focus_next_new_doc = FALSE;
				if (f2d->bfwin->current_document == f2d->doc) {
					doc_force_activate(f2d->doc);
				} else {
					switch_to_document_by_pointer(f2d->bfwin,f2d->doc);
				}
			}
			if (f2d->doc->action.goto_line >= 0) {
			   DEBUG_MSG("file2doc_lcb, goto_line=%d\n",f2d->doc->action.goto_line);
				doc_select_line(f2d->doc, f2d->doc->action.goto_line, TRUE);
			} else if (f2d->doc->action.goto_offset >= 0) {
				DEBUG_MSG("file2doc_lcb, goto_offset=%d\n",f2d->doc->action.goto_offset);
				doc_select_line_by_offset(f2d->doc, f2d->doc->action.goto_offset, TRUE);
			}
			f2d->doc->action.goto_line = -1;
			f2d->doc->action.goto_offset = -1;
			f2d->doc->action.load = NULL;
			file2doc_cleanup(data);
		break;
		case OPENFILE_CHANNEL_OPENED:
			/* do nothing */
		break;
   	case OPENFILE_ERROR_CANCELLED:
		/* lets close the document */
		f2d->doc->action.load = NULL;
		DEBUG_MSG("file2doc_lcb, calling doc_close_single_backend\n");
		doc_close_single_backend(f2d->doc, f2d->doc->action.close_window);
		file2doc_cleanup(f2d);
   	break;
		case OPENFILE_ERROR:
		case OPENFILE_ERROR_NOCHANNEL:
		case OPENFILE_ERROR_NOREAD:
			DEBUG_MSG("file2doc_lcb, ERROR status=%d, cleanup!!!!!\n",status);
			if (f2d->doc->action.close_doc) {
				f2d->doc->action.load = NULL;
				doc_close_single_backend(f2d->doc, f2d->doc->action.close_window);
			} else {
				doc_set_status(f2d->doc, DOC_STATUS_ERROR);
			}
			f2d->doc->action.load = NULL;
			file2doc_cleanup(f2d);
		break;
	}
}

typedef struct {
	GnomeVFSAsyncHandle *handle;
	Tdocument *doc;
	GList *uris;
} Tfileinfo;

static void file_asyncfileinfo_cleanup(Tfileinfo *fi) {
	DEBUG_MSG("file_asyncfileinfo_cleanup, fi=%p\n",fi);
	gnome_vfs_uri_unref(fi->uris->data);
	g_list_free(fi->uris);
	g_free(fi);
}

void file_asyncfileinfo_cancel(gpointer fi) {
	DEBUG_MSG("file_asyncfileinfo_cancel, fi=%p\n",fi);
	((Tfileinfo *)fi)->doc->action.info = NULL;
	gnome_vfs_async_cancel(((Tfileinfo *)fi)->handle);
	file_asyncfileinfo_cleanup((Tfileinfo *)fi);
}

static void file_asyncfileinfo_lcb(GnomeVFSAsyncHandle *handle, GList *results, /* GnomeVFSGetFileInfoResult* items */gpointer data) {
	GnomeVFSGetFileInfoResult* item;
	Tfileinfo *fi;
	DEBUG_MSG("file_asyncfileinfo_lcb, started, results=%p\n",results);
	fi = data;
	item = results->data;
	if (item->result == GNOME_VFS_OK) {
		DEBUG_MSG("file_asyncfileinfo_lcb, new file_info=%p for doc %p\n",item->file_info, fi->doc);
		doc_set_fileinfo(fi->doc, item->file_info);
	} else {
		DEBUG_MSG("no fileinfo found.., not doing anything..\n");
	}
	fi->doc->action.info = NULL;
	if (fi->doc->action.close_doc) {
		doc_close_single_backend(fi->doc, fi->doc->action.close_window);
	}
	file_asyncfileinfo_cleanup(fi);
}

void file_doc_fill_fileinfo(Tdocument *doc, GnomeVFSURI *uri) {
	Tfileinfo *fi;
	fi = g_new(Tfileinfo,1);
	DEBUG_MSG("file_doc_fill_fileinfo, started for doc %p and uri %s at fi=%p\n",doc,gnome_vfs_uri_get_path(uri),fi);
	fi->doc = doc;
	fi->doc->action.info = fi;
	gnome_vfs_uri_ref(uri);
	fi->uris = g_list_append(NULL, uri);
	gnome_vfs_async_get_file_info(&fi->handle,fi->uris,GNOME_VFS_FILE_INFO_DEFAULT|GNOME_VFS_FILE_INFO_FOLLOW_LINKS
			,GNOME_VFS_PRIORITY_DEFAULT,file_asyncfileinfo_lcb,fi);
}

void file_doc_retry_uri(Tdocument *doc) {
	Tfile2doc *f2d;
	f2d = g_new(Tfile2doc,1);
	f2d->bfwin = doc->bfwin;
	f2d->doc = doc;
	doc_set_status(doc, DOC_STATUS_LOADING);
	f2d->uri = gnome_vfs_uri_new(doc->uri);
	if (doc->fileinfo == NULL) {
		file_doc_fill_fileinfo(f2d->doc, f2d->uri);
	}
	f2d->of = file_openfile_uri_async(f2d->uri,file2doc_lcb,f2d);
}

void file_doc_fill_from_uri(Tdocument *doc, GnomeVFSURI *uri, GnomeVFSFileInfo *finfo, gint goto_line) {
	Tfile2doc *f2d;
	f2d = g_new(Tfile2doc,1);
	f2d->bfwin = doc->bfwin;
	f2d->uri = gnome_vfs_uri_ref(uri);
	f2d->doc = doc;
	f2d->doc->action.load = f2d;
	f2d->doc->action.goto_line = goto_line;
	if (finfo == NULL) {
		file_doc_fill_fileinfo(f2d->doc, uri);
	}
	f2d->of = file_openfile_uri_async(f2d->uri,file2doc_lcb,f2d);
}

void file_doc_from_uri(Tbfwin *bfwin, GnomeVFSURI *uri, GnomeVFSFileInfo *finfo, gint goto_line, gint goto_offset) {
	Tfile2doc *f2d;
	gchar *curi;
	f2d = g_new(Tfile2doc,1);
	DEBUG_MSG("file_doc_from_uri, open %s, f2d=%p\n", gnome_vfs_uri_get_path(uri), f2d);
	f2d->bfwin = bfwin;
	f2d->uri = gnome_vfs_uri_ref(uri);
	curi = gnome_vfs_uri_to_string(uri,0);
	f2d->doc = doc_new_loading_in_background(bfwin, curi, finfo);
	f2d->doc->action.load = f2d;
	f2d->doc->action.goto_line = goto_line;
	f2d->doc->action.goto_offset = goto_offset;
	DEBUG_MSG("file_doc_from_uri, got doc %p\n",f2d->doc);
	if (finfo == NULL) {
		/* get the fileinfo also async */
		file_doc_fill_fileinfo(f2d->doc, uri);
	}
	g_free(curi);
	f2d->of = file_openfile_uri_async(f2d->uri,file2doc_lcb,f2d);
}

/*************************** OPEN ADVANCED ******************************/

typedef struct {
	GnomeVFSAsyncHandle *handle;
	Tbfwin *bfwin;
	GnomeVFSURI *basedir;
	gboolean recursive;
	gchar *extension_filter;
	GPatternSpec* patspec;
	gchar *content_filter;
	gboolean use_regex;
} Topenadv_dir; /* can be typecasted to Tfileaction !!*/

typedef struct {
	Tbfwin *bfwin;
	GnomeVFSURI *uri;
	GnomeVFSFileInfo *finfo;
	gchar *content_filter;
	gboolean use_regex;
} Topenadv_uri; /* can be typecasted to Tfileaction !!*/

static void open_adv_open_uri_cleanup(Topenadv_uri *oau) {
	gnome_vfs_uri_unref(oau->uri);
	gnome_vfs_file_info_unref(oau->finfo);
	g_free(oau->content_filter);
	g_free(oau);
}

static gboolean open_adv_content_matches_filter(gchar *buffer,GnomeVFSFileSize buflen, Topenadv_uri *oau) {
	if (oau->use_regex) {
		pcre *re;
		const char *error;
		int erroffset;
		re = pcre_compile(oau->content_filter, 0, &error, &erroffset, NULL);
		if (re) {
			int rc;
			int ovector[30];
			rc = pcre_exec(re, NULL, buffer, buflen, 0, 0, ovector, 30);
			pcre_free(re);
			return rc;
		}
	} else {
		return (strstr(buffer, oau->content_filter) != NULL);
	}
	return FALSE;
}

static void open_adv_content_filter_lcb(Topenfile_status status,gint error_info, gchar *buffer,GnomeVFSFileSize buflen,gpointer data) {
	Topenadv_uri *oau = data;
	switch (status) {
		case OPENFILE_FINISHED:
			DEBUG_MSG("open_adv_content_filter_lcb, status=%d, now we should do the content filtering\n",status);
			/* we have all content, do the filtering, and if correct, open the file as document */
			if (open_adv_content_matches_filter(buffer,buflen,oau)) {
				gchar *curi;
				Tfile2doc *f2d = g_new(Tfile2doc,1);
				f2d->uri = oau->uri;
				gnome_vfs_uri_ref(oau->uri);
				f2d->bfwin = oau->bfwin;
				curi = gnome_vfs_uri_to_string(oau->uri,0);
				f2d->doc = doc_new_loading_in_background(oau->bfwin, curi, oau->finfo);
				g_free(curi);
				file2doc_lcb(status,error_info,buffer,buflen,f2d);
			}
			open_adv_open_uri_cleanup(data);
		break;
		case OPENFILE_CHANNEL_OPENED:
			/* do nothing */
		break;
		case OPENFILE_ERROR:
		case OPENFILE_ERROR_NOCHANNEL:
		case OPENFILE_ERROR_NOREAD:
		case OPENFILE_ERROR_CANCELLED:
			open_adv_open_uri_cleanup(data);
		break;
	}
}

static void openadv_content_filter_file(Tbfwin *bfwin, GnomeVFSURI *uri, GnomeVFSFileInfo* finfo, gchar *content_filter, gboolean use_regex) {
	Topenadv_uri *oau;
	
	oau = g_new0(Topenadv_uri,1);
	oau->bfwin = bfwin;
	oau->uri = uri;
	gnome_vfs_uri_ref(uri);
	oau->finfo = gnome_vfs_file_info_dup(finfo);
	oau->content_filter = g_strdup(content_filter);
	oau->use_regex = use_regex;
	file_openfile_uri_async(uri, open_adv_content_filter_lcb, oau);
}

static void open_adv_load_directory_cleanup(Topenadv_dir *oa) {
	DEBUG_MSG("open_adv_load_directory_cleanup %p for %s\n", oa, gnome_vfs_uri_get_path(oa->basedir));
	gnome_vfs_uri_unref(oa->basedir);
	if (oa->extension_filter) g_free(oa->extension_filter);
	if (oa->patspec) g_pattern_spec_free(oa->patspec);
	if (oa->content_filter) g_free(oa->content_filter);
	g_free(oa);
}

static void open_adv_load_directory_lcb(GnomeVFSAsyncHandle *handle,GnomeVFSResult result,GList *list,guint entries_read,gpointer data) {
	Topenadv_dir *oa = data;
	GList *tmplist;
	DEBUG_MSG("open_adv_load_directory_lcb, called for %p %s with %d items, result=%d\n",oa, gnome_vfs_uri_get_path(oa->basedir), entries_read, result);
	tmplist = g_list_first(list);
	while (tmplist) {
		GnomeVFSFileInfo *finfo = tmplist->data;
		GnomeVFSURI *child_uri;
		if (strcmp(finfo->name,".")!=0 && strcmp(finfo->name,"..")!=0) {
			child_uri = gnome_vfs_uri_append_file_name(oa->basedir,finfo->name);
			if (finfo->type == GNOME_VFS_FILE_TYPE_DIRECTORY && oa->recursive) {
/*				DEBUG_MSG("open_adv_load_directory_lcb, open dir %s\n", gnome_vfs_uri_get_path(child_uri));*/
				open_advanced(oa->bfwin, child_uri, oa->recursive, oa->extension_filter, oa->content_filter, oa->use_regex);
			} else if (finfo->type == GNOME_VFS_FILE_TYPE_REGULAR){
				gchar *curi;
				curi = gnome_vfs_uri_to_string(child_uri,0);
				list = return_allwindows_documentlist();
				if (documentlist_return_document_from_filename(list, curi)==NULL) { /* if this file is already open, there is no need to do any of these checks */
					if (oa->patspec) {
						if (g_pattern_match_string(oa->patspec, finfo->name)) { /* test extension */
							if (oa->content_filter) { /* do we need content filtering */
								DEBUG_MSG("open_adv_load_directory_lcb, content filter %s\n", gnome_vfs_uri_get_path(child_uri));
								openadv_content_filter_file(oa->bfwin, child_uri, finfo, oa->content_filter, oa->use_regex);
							} else { /* open this file as document */
								DEBUG_MSG("open_adv_load_directory_lcb, open %s\n", gnome_vfs_uri_get_path(child_uri));
								doc_new_from_uri(oa->bfwin, NULL, child_uri, finfo, TRUE, FALSE, -1, -1);
							}
						}
					} else if (oa->content_filter) {
						openadv_content_filter_file(oa->bfwin, child_uri, finfo, oa->content_filter, oa->use_regex);
					} else {
						doc_new_from_uri(oa->bfwin, NULL, child_uri, finfo, TRUE, FALSE, -1, -1);
					}
				}
				g_free(curi);
				g_list_free(list);
			}
			gnome_vfs_uri_unref(child_uri);
		}
		tmplist = g_list_next(tmplist);
	}	
	if (result == GNOME_VFS_ERROR_EOF) {
		open_adv_load_directory_cleanup(oa);
	}
}

void open_advanced(Tbfwin *bfwin, GnomeVFSURI *basedir, gboolean recursive, gchar *extension_filter, gchar *content_filter, gboolean use_regex) {
	if (basedir) {
		GnomeVFSAsyncHandle *handle;
		Topenadv_dir *oa;
		
		oa = g_new0(Topenadv_dir, 1);
		DEBUG_MSG("open_advanced, open dir %s, oa=%p, extension_filter=%s\n", gnome_vfs_uri_get_path(basedir), oa, extension_filter);
		oa->bfwin = bfwin;
		oa->basedir = gnome_vfs_uri_dup(basedir);
		oa->recursive = recursive;
		if (extension_filter) {
			oa->extension_filter = g_strdup(extension_filter);
			oa->patspec = g_pattern_spec_new(extension_filter);
		}
		if (content_filter) oa->content_filter = g_strdup(content_filter);
		oa->use_regex = use_regex;
	
		gnome_vfs_async_load_directory_uri(&handle,basedir,GNOME_VFS_FILE_INFO_DEFAULT|GNOME_VFS_FILE_INFO_FOLLOW_LINKS,
										10,GNOME_VFS_PRIORITY_DEFAULT,open_adv_load_directory_lcb,oa);
	}
}

/************************/
typedef struct {
	Tbfwin *bfwin;
	GnomeVFSAsyncHandle *handle;
	GnomeVFSURI *destdir;
	GList *sourcelist;
	GList *destlist;
} Tcopyfile;

static void copyfile_cleanup(Tcopyfile *cf) {
	DEBUG_MSG("copyfile_cleanup %p\n",cf);
	gnome_vfs_uri_list_free(cf->sourcelist);
	gnome_vfs_uri_list_free(cf->destlist);
	gnome_vfs_uri_unref(cf->destdir);
	g_free(cf);
}

static gint copyfile_progress_lcb(GnomeVFSAsyncHandle *handle,GnomeVFSXferProgressInfo *info,gpointer data) {
	Tcopyfile *cf = data;
	DEBUG_MSG("copyfile_progress_lcb, started with status %d and phase %d for source %s and target %s, index=%ld, total=%ld, thread=%p\n"
		,info->status,info->phase,info->source_name,info->target_name,info->file_index,info->files_total, g_thread_self());

	if (info->status == GNOME_VFS_XFER_PROGRESS_STATUS_OVERWRITE) {
		/* BUG: we should ask the user what to do ? */
		gchar *options[] = {_("_Skip"), _("_Overwrite"), NULL};
		gint retval;
		gchar *tmpstr, *dispname;
		DEBUG_MSG("copyfile_progress_lcb, overwrite? skip for the moment\n");
		dispname = gnome_vfs_format_uri_for_display(info->target_name);
		tmpstr = g_strdup_printf(_("%s already exists, overwrite?"),dispname);
		retval = multi_warning_dialog(BFWIN(cf->bfwin)->main_window,_("Overwrite file?"), tmpstr, 1, 0, options);
		g_free(tmpstr);
		return (retval == 0) ? GNOME_VFS_XFER_OVERWRITE_ACTION_SKIP : GNOME_VFS_XFER_OVERWRITE_ACTION_REPLACE;
	} else if (info->status == GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR) {
		DEBUG_MSG("copyfile_progress_lcb, vfs error! skip for the moment\n");
		return GNOME_VFS_XFER_ERROR_ACTION_SKIP;
	} else if (info->status == GNOME_VFS_XFER_PROGRESS_STATUS_OK) {
		if (info->phase == GNOME_VFS_XFER_PHASE_COMPLETED) {
			/* lets tell the filebrowser to refresh this dir */
			fb2_refresh_dir_from_uri(cf->destdir);
			/* hey! we're finished!! */
			copyfile_cleanup(cf);
		}
	}
	return 1; 	/* Nautilus returns 1 by default for this callback, and 0 for unknown status */
}

static gint copyfile_sync_lcb(GnomeVFSXferProgressInfo *info,gpointer data) {
	DEBUG_MSG("copyfile_sync_lcb, started with status %d and phase %d for source %s and target %s, index=%ld, total=%ld, thread=%p\n"
		,info->status,info->phase,info->source_name,info->target_name,info->file_index,info->files_total, g_thread_self());
	return 1;
}

void copy_files_async(Tbfwin *bfwin, GnomeVFSURI *destdir, gchar *sources) {
	Tcopyfile *cf;
	GnomeVFSResult ret;
	gchar **splitted, **tmp;
	cf = g_new0(Tcopyfile,1);
	cf->bfwin = bfwin;
	cf->destdir = destdir;
	gnome_vfs_uri_ref(cf->destdir);
	/* create the source and destlist ! */
	tmp = splitted = g_strsplit(sources, "\n",0);
	while (*tmp) {
		trunc_on_char(trunc_on_char(*tmp, '\r'), '\n');
		DEBUG_MSG("copy_files_async, *tmp='%s', len=%d\n",*tmp,strlen(*tmp));
		if (strlen(*tmp) > 1) {
			GnomeVFSURI *src, *dest;
			gchar *tmpstring;
			src = gnome_vfs_uri_new(*tmp);
			tmpstring = gnome_vfs_uri_extract_short_path_name(src);
			dest = gnome_vfs_uri_append_string(destdir, tmpstring);
			g_free(tmpstring);
			cf->sourcelist = g_list_append(cf->sourcelist, src);
			cf->destlist = g_list_append(cf->destlist, dest);
		}
		tmp++;
	}
	ret = gnome_vfs_async_xfer(&cf->handle,cf->sourcelist,cf->destlist
			,GNOME_VFS_XFER_FOLLOW_LINKS,GNOME_VFS_XFER_ERROR_MODE_QUERY
			,GNOME_VFS_XFER_OVERWRITE_MODE_QUERY,GNOME_VFS_PRIORITY_DEFAULT
			,copyfile_progress_lcb, cf ,copyfile_sync_lcb, cf);
	DEBUG_MSG("copy_file_async, start backup, srclen=%d,destlen=%d, ret=%d\n"
			,g_list_length(cf->sourcelist),g_list_length(cf->destlist),ret);
	g_strfreev(splitted);
}

