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
#include "file.h"

/*************************** OPEN FILE ASYNC ******************************/

enum {
	OPENFILE_ERROR,
	OPENFILE_ERROR_NOCHANNEL,
	OPENFILE_ERROR_NOREAD,
	OPENFILE_CHANNEL_OPENED,
	OPENFILE_FINISHED
};

typedef void (* OpenfileAsyncCallback) (gint status,gint error_info, gchar *buffer,gpointer callback_data);

typedef struct {
	gchar *buffer;
	GIOChannel *channel;
	GnomeVFSAsyncHandle *handle;
	unsigned long long buffer_size;
	unsigned long long used_size;
	OpenfileAsyncCallback callback_func;
	gpointer callback_data;
} Topenfile;
#define CHUNK_SIZE 4096
#define BUFFER_INCR_SIZE 40960

static void openfile_cleanup(Topenfile *of) {
	DEBUG_MSG("openfile_cleanup %p\n",of);
	/*g_io_channel_unref(of->channel);*/
	g_free(of->buffer);
	g_free(of);
}

/* static void open_file__close_lcb(GnomeVFSAsyncHandle *handle,GnomeVFSResult result,gpointer data) {
	openfile_cleanup(data);
} */

static gboolean open_file_channel_lcb(GIOChannel *source,GIOCondition condition,gpointer data) {
	Topenfile *of = data;
	DEBUG_MSG("open_file_channel_lcb, of=%p\n",of);
	if (condition & G_IO_IN) { /* Data is available. */
		gchar chunk[CHUNK_SIZE];
		gsize bytes_read;
		GIOStatus gstatus;
		DEBUG_MSG("open_file_channel_lcb, try to read %d bytes from channel",CHUNK_SIZE);
		gstatus = g_io_channel_read_chars(source, chunk,CHUNK_SIZE,&bytes_read,NULL);
		DEBUG_MSG(" status: %d\n",gstatus);
		if ((bytes_read + of->used_size) > of->buffer_size) {
			of->buffer_size += BUFFER_INCR_SIZE;
			of->buffer = g_realloc(of->buffer, of->buffer_size+1);/* the +1 is so we can add a \0 to the buffer */
		}
		memcpy(of->buffer + of->used_size, chunk, bytes_read);
		of->used_size += bytes_read;
		DEBUG_MSG("open_file_channel_lcb, chunk read, used_size=%lld\n",of->used_size);
	}
	if (condition & G_IO_NVAL) { /* an error happened */
		/* cleanup ! */
		DEBUG_MSG("open_file_channel_lcb, an error happened! cleanup!\n");
		of->callback_func(OPENFILE_ERROR_NOREAD,condition,of->buffer,of->callback_data);
		openfile_cleanup(of);
		return FALSE;
	}
	if (condition & G_IO_HUP) { /* end-of-file */
		GError *error = NULL;
		DEBUG_MSG("open_file_channel_lcb, end-of-file!, used_size=%lld\n",of->used_size);
		g_io_channel_shutdown(source, FALSE, &error);
		of->buffer[of->used_size] = '\0'; /* because we allocated 1 more byte then requested we can do this */
		/* now do something with the data */
		of->callback_func(OPENFILE_FINISHED,condition,of->buffer, of->callback_data);
		openfile_cleanup(of);
		/*gnome_vfs_async_close(of->handle, open_file__close_lcb, of);*/
		return FALSE;
	}
	DEBUG_MSG("open_file_channel_lcb, return TRUE\n");
	return TRUE; /* keep calling me for this channel */
}

static void open_file_as_channel_lcb(GnomeVFSAsyncHandle *handle,GIOChannel *channel, GnomeVFSResult result, gpointer data) {
	Topenfile *of = data;
	DEBUG_MSG("open_file_as_channel_lcb, of=%p\n",of);
	if (result == GNOME_VFS_OK) {
		of->callback_func(OPENFILE_CHANNEL_OPENED,GNOME_VFS_OK,NULL,of->callback_data);
		of->channel = channel;
		g_io_channel_set_encoding(channel, NULL, NULL);
		g_io_add_watch_full(channel, G_PRIORITY_HIGH,G_IO_IN|G_IO_NVAL|G_IO_HUP,open_file_channel_lcb,of,NULL);
		g_io_channel_unref(channel);
	} else {
		DEBUG_MSG("open_file_as_channel_lcb, error %d, cleanup %p\n",result,of);
		of->callback_func(OPENFILE_ERROR_NOCHANNEL, result, NULL, of->callback_data);
		openfile_cleanup(of);
	}
}

static void file_open_uri_async(GnomeVFSURI *uri, OpenfileAsyncCallback callback_func, gpointer callback_data) {
	Topenfile *of;
	of = g_new(Topenfile,1);
	DEBUG_MSG("file_open_uri_async, %s, of=%p\n",gnome_vfs_uri_get_path(uri), of);
	of->callback_data = callback_data;
	of->callback_func = callback_func;
	of->buffer_size = BUFFER_INCR_SIZE;
	of->used_size = 0;
	of->buffer = g_malloc(of->buffer_size+1); /* the +1 is so we can add a \0 to the buffer */
	gnome_vfs_async_open_uri_as_channel(&of->handle, uri,GNOME_VFS_OPEN_READ, CHUNK_SIZE
							,GNOME_VFS_PRIORITY_DEFAULT,open_file_as_channel_lcb,of);

}

/************ MAKE DOCUMENT FROM ASYNC OPENED FILE ************************/
typedef struct {
	Tbfwin *bfwin;
	GnomeVFSURI *uri;
} Tfile2doc;

static void file2doc_cleanup(Tfile2doc *f2d) {
	gnome_vfs_uri_unref(f2d->uri);
	g_free(f2d);
}

static void file2doc_lcb(gint status,gint error_info,gchar *buffer,gpointer data) {
	Tfile2doc *f2d = data;
	DEBUG_MSG("file2doc_lcb, status=%d, f2d=%p\n",status,f2d);
	switch (status) {
		case OPENFILE_FINISHED:
			DEBUG_MSG("file2doc_lcb, status=%d, now we should convert %s data into a GtkTextBuffer and such\n",status, gnome_vfs_uri_get_path(f2d->uri));
			puts(buffer);
			file2doc_cleanup(data);
		break;
		case OPENFILE_ERROR:
		case OPENFILE_ERROR_NOCHANNEL:
		case OPENFILE_ERROR_NOREAD:
			DEBUG_MSG("file2doc_lcb, status=%d, cleanup!!!!!\n",status);
			file2doc_cleanup(data);
		break;
	}
}

void file_doc_from_uri(Tbfwin *bfwin, GnomeVFSURI *uri) {
	Tfile2doc *f2d;
	f2d = g_new(Tfile2doc,1);
	DEBUG_MSG("file_doc_from_uri, open %s, f2d=%p\n", gnome_vfs_uri_get_path(uri), f2d);
	f2d->bfwin = bfwin;
	f2d->uri = gnome_vfs_uri_dup(uri);
	file_open_uri_async(f2d->uri,file2doc_lcb,f2d);
}

/*************************** OPEN ADVANCED ******************************/

typedef struct {
	Tbfwin *bfwin;
	GnomeVFSURI *basedir;
	gboolean recursive;
	gchar *extension_filter;
	gchar *content_filter;
	gboolean use_regex;
} Topenadv_dir;

typedef struct {
	Tbfwin *bfwin;
	GnomeVFSURI *uri;
	gchar *content_filter;
	gboolean use_regex;
} Topenadv_uri;

static void open_adv_open_uri_cleanup(Topenadv_uri *oau) {
	gnome_vfs_uri_unref(oau->uri);
	g_free(oau->content_filter);
	g_free(oau);
}

static void open_adv_content_filter_lcb(gint status,gint error_info, gchar *buffer,gpointer data) {
	switch (status) {
		case OPENFILE_FINISHED:
			DEBUG_MSG("open_adv_content_filter_lcb, status=%d, now we should do the content filtering\n",status);
			open_adv_open_uri_cleanup(data);
		break;
		case OPENFILE_ERROR:
		case OPENFILE_ERROR_NOCHANNEL:
		case OPENFILE_ERROR_NOREAD:
			open_adv_open_uri_cleanup(data);
		break;
	}
}

static void openadv_content_filter_file(Tbfwin *bfwin, GnomeVFSURI *uri, gchar *content_filter, gboolean use_regex) {
	Topenadv_uri *oau;
	
	oau = g_new0(Topenadv_uri,1);
	oau->bfwin = bfwin;
	oau->uri = gnome_vfs_uri_dup(uri);
	oau->content_filter = g_strdup(content_filter);
	oau->use_regex = use_regex;
	file_open_uri_async(uri, open_adv_content_filter_lcb, oau);
}



static void open_adv_load_directory_lcb(GnomeVFSAsyncHandle *handle,GnomeVFSResult result,GList *list,guint entries_read,gpointer data) {
	Topenadv_dir *oa = data;
	GList *tmplist;
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
				gchar **ext;
				ext = array_from_arglist(oa->extension_filter, NULL);
				if (filename_test_extensions(ext, gnome_vfs_uri_get_path(child_uri))) { /* test extension */
					if (oa->content_filter) { /* do we need content filtering */
/*						DEBUG_MSG("open_adv_load_directory_lcb, content filter %s\n", gnome_vfs_uri_get_path(child_uri));*/
						openadv_content_filter_file(oa->bfwin, child_uri, oa->content_filter, oa->use_regex);
					} else { /* open this file as document */
						DEBUG_MSG("open_adv_load_directory_lcb, open %s\n", gnome_vfs_uri_get_path(child_uri));
						file_doc_from_uri(oa->bfwin, child_uri);
					}
				}
				g_strfreev(ext);
			}
			gnome_vfs_uri_unref(child_uri);
		}
		tmplist = g_list_next(tmplist);
	}	
	
	if (result == GNOME_VFS_ERROR_EOF) {
		gnome_vfs_uri_unref(oa->basedir);
		g_free(oa->extension_filter);
		g_free(oa->content_filter);
		g_free(oa);
	}
}

void open_advanced(Tbfwin *bfwin, GnomeVFSURI *basedir, gboolean recursive, gchar *extension_filter, gchar *content_filter, gboolean use_regex) {
	if (basedir) {
		GnomeVFSAsyncHandle *handle;
		Topenadv_dir *oa;
		
		oa = g_new0(Topenadv_dir, 1);
		oa->bfwin = bfwin;
		oa->basedir = gnome_vfs_uri_dup(basedir);
		oa->recursive = recursive;
		if (extension_filter) oa->extension_filter = g_strdup(extension_filter);
		if (content_filter) oa->content_filter = g_strdup(content_filter);
		oa->use_regex = use_regex;
	
		gnome_vfs_async_load_directory_uri(&handle,basedir,GNOME_VFS_FILE_INFO_DEFAULT,
										10,GNOME_VFS_PRIORITY_DEFAULT,open_adv_load_directory_lcb,oa);
	}
}





