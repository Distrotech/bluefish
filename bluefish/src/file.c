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
#include "gui.h"

/*************************** OPEN FILE ASYNC ******************************/

enum {
	OPENFILE_ERROR,
	OPENFILE_ERROR_NOCHANNEL,
	OPENFILE_ERROR_NOREAD,
	OPENFILE_CHANNEL_OPENED,
	OPENFILE_FINISHED
};

typedef void (* OpenfileAsyncCallback) (gint status,gint error_info, gchar *buffer,GnomeVFSFileSize buflen,gpointer callback_data);

typedef struct {
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

static void file_openfile_uri_async(GnomeVFSURI *uri, OpenfileAsyncCallback callback_func, gpointer callback_data) {
	Topenfile *of;
	GnomeVFSAsyncHandle *handle;
	of = g_new(Topenfile,1);
	DEBUG_MSG("file_open_uri_async, %s, of=%p\n",gnome_vfs_uri_get_path(uri), of);
	of->callback_data = callback_data;
	of->callback_func = callback_func;
	of->buffer_size = BUFFER_INCR_SIZE;
	of->used_size = 0;
	of->buffer = g_malloc(of->buffer_size+1); /* the +1 is so we can add a \0 to the buffer */
	gnome_vfs_async_open_uri(&handle,uri,GNOME_VFS_OPEN_READ,GNOME_VFS_PRIORITY_DEFAULT
				,openfile_asyncopenuri_lcb,of);
}

/************ MAKE DOCUMENT FROM ASYNC OPENED FILE ************************/
typedef struct {
	Tbfwin *bfwin;
	Tdocument *doc;
	GnomeVFSURI *uri;
} Tfile2doc;

static void file2doc_cleanup(Tfile2doc *f2d) {
	gnome_vfs_uri_unref(f2d->uri);
	g_free(f2d);
}

static void file2doc_lcb(gint status,gint error_info,gchar *buffer,GnomeVFSFileSize buflen ,gpointer data) {
	Tfile2doc *f2d = data;
	DEBUG_MSG("file2doc_lcb, status=%d, f2d=%p\n",status,f2d);
	switch (status) {
		case OPENFILE_FINISHED:
			DEBUG_MSG("file2doc_lcb, status=%d, now we should convert %s data into a GtkTextBuffer and such\n",status, gnome_vfs_uri_get_path(f2d->uri));
			doc_buffer_to_textbox(f2d->doc, buffer, buflen, FALSE, TRUE);
			if (f2d->bfwin->focus_next_new_doc) {
				switch_to_document_by_pointer(f2d->bfwin,f2d->doc);
				f2d->bfwin->focus_next_new_doc = FALSE;
			}
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

typedef struct {
	Tdocument *doc;
	GList *uris;
} Tfileinfo;

static void file_asyncfileinfo_lcb(GnomeVFSAsyncHandle *handle, GList *results, /* GnomeVFSGetFileInfoResult* items */gpointer data) {
	GnomeVFSGetFileInfoResult* item;
	Tfileinfo *fi;
	DEBUG_MSG("file_asyncfileinfo_lcb, started, results=%p\n",results);
	fi = data;
	item = results->data;
	if (item->result == GNOME_VFS_OK) {
		DEBUG_MSG("file_asyncfileinfo_lcb, file_info=%p\n",item->file_info);
		doc_set_fileinfo(fi->doc, item->file_info);
	}
	gnome_vfs_uri_unref(fi->uris->data);
	g_list_free(fi->uris);
	g_free(fi);
}

void file_doc_fill_fileinfo(Tdocument *doc, GnomeVFSURI *uri) {
	GnomeVFSAsyncHandle *handle;
	Tfileinfo *fi;
	DEBUG_MSG("file_doc_fill_fileinfo, started\n");
	fi = g_new(Tfileinfo,1);
	fi->doc = doc;
	gnome_vfs_uri_ref(uri);
	fi->uris = g_list_append(NULL, uri);
	gnome_vfs_async_get_file_info(&handle,fi->uris,GNOME_VFS_FILE_INFO_DEFAULT|GNOME_VFS_FILE_INFO_FOLLOW_LINKS
			,GNOME_VFS_PRIORITY_DEFAULT,file_asyncfileinfo_lcb,fi);
}
 

void file_doc_from_uri(Tbfwin *bfwin, GnomeVFSURI *uri, GnomeVFSFileInfo *finfo) {
	Tfile2doc *f2d;
	gchar *curi;
	f2d = g_new(Tfile2doc,1);
	DEBUG_MSG("file_doc_from_uri, open %s, f2d=%p\n", gnome_vfs_uri_get_path(uri), f2d);
	f2d->bfwin = bfwin;
	f2d->uri = gnome_vfs_uri_ref(uri);
	curi = gnome_vfs_uri_to_string(uri,0);
	f2d->doc = doc_new_loading_in_background(bfwin, curi, finfo);
	if (finfo == NULL) {
		/* get the fileinfo also async */
		file_doc_fill_fileinfo(f2d->doc, uri);
	}
	g_free(curi);
	file_openfile_uri_async(f2d->uri,file2doc_lcb,f2d);
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
	GnomeVFSFileInfo *finfo;
	gchar *content_filter;
	gboolean use_regex;
} Topenadv_uri;

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

static void open_adv_content_filter_lcb(gint status,gint error_info, gchar *buffer,GnomeVFSFileSize buflen,gpointer data) {
	Topenadv_uri *oau = data;
	switch (status) {
		case OPENFILE_FINISHED:
			DEBUG_MSG("open_adv_content_filter_lcb, status=%d, now we should do the content filtering\n",status);
			/* we have all content, do the filtering, and if correct, open the file as document */
			if (open_adv_content_matches_filter(buffer,buflen,oau)) {
				gchar *curi;
				Tfile2doc *f2d = g_new(Tfile2doc,1);
				f2d->uri = gnome_vfs_uri_dup(oau->uri);
				f2d->bfwin = oau->bfwin;
				curi = gnome_vfs_uri_to_string(oau->uri,0);
				f2d->doc = doc_new_loading_in_background(oau->bfwin, curi, oau->finfo);
				g_free(curi);
				file2doc_lcb(status,error_info,buffer,buflen,f2d);
			}
			open_adv_open_uri_cleanup(data);
		break;
		case OPENFILE_ERROR:
		case OPENFILE_ERROR_NOCHANNEL:
		case OPENFILE_ERROR_NOREAD:
			open_adv_open_uri_cleanup(data);
		break;
	}
}

static void openadv_content_filter_file(Tbfwin *bfwin, GnomeVFSURI *uri, GnomeVFSFileInfo* finfo, gchar *content_filter, gboolean use_regex) {
	Topenadv_uri *oau;
	
	oau = g_new0(Topenadv_uri,1);
	oau->bfwin = bfwin;
	oau->uri = gnome_vfs_uri_dup(uri);
	oau->finfo = gnome_vfs_file_info_dup(finfo);
	oau->content_filter = g_strdup(content_filter);
	oau->use_regex = use_regex;
	file_openfile_uri_async(uri, open_adv_content_filter_lcb, oau);
}

static void open_adv_load_directory_cleanup(Topenadv_dir *oa) {
	DEBUG_MSG("open_adv_load_directory_cleanup %p for %s\n", oa, gnome_vfs_uri_get_path(oa->basedir));
	gnome_vfs_uri_unref(oa->basedir);
	if (oa->extension_filter) g_free(oa->extension_filter);
	if (oa->content_filter) g_free(oa->content_filter);
	g_free(oa);
}

static void open_adv_load_directory_lcb(GnomeVFSAsyncHandle *handle,GnomeVFSResult result,GList *list,guint entries_read,gpointer data) {
	Topenadv_dir *oa = data;
	GList *tmplist;
	gchar **ext;
	DEBUG_MSG("open_adv_load_directory_lcb, called for %p %s with %d items, result=%d\n",oa, gnome_vfs_uri_get_path(oa->basedir), entries_read, result);
	ext = array_from_arglist(oa->extension_filter, NULL);
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
				if (oa->extension_filter) {
					if (filename_test_extensions(ext, finfo->name)) { /* test extension */
						if (oa->content_filter) { /* do we need content filtering */
							DEBUG_MSG("open_adv_load_directory_lcb, content filter %s\n", gnome_vfs_uri_get_path(child_uri));
							openadv_content_filter_file(oa->bfwin, child_uri, finfo, oa->content_filter, oa->use_regex);
						} else { /* open this file as document */
							DEBUG_MSG("open_adv_load_directory_lcb, open %s\n", gnome_vfs_uri_get_path(child_uri));
							file_doc_from_uri(oa->bfwin, child_uri, finfo);
						}
					}
				} else if (oa->content_filter) {
					openadv_content_filter_file(oa->bfwin, child_uri, finfo, oa->content_filter, oa->use_regex);
				} else {
					file_doc_from_uri(oa->bfwin, child_uri, finfo);
				}
			}
			gnome_vfs_uri_unref(child_uri);
		}
		tmplist = g_list_next(tmplist);
	}	
	if (result == GNOME_VFS_ERROR_EOF) {
		open_adv_load_directory_cleanup(oa);
	}
	g_strfreev(ext);
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
		if (extension_filter) oa->extension_filter = g_strdup(extension_filter);
		if (content_filter) oa->content_filter = g_strdup(content_filter);
		oa->use_regex = use_regex;
	
		gnome_vfs_async_load_directory_uri(&handle,basedir,GNOME_VFS_FILE_INFO_DEFAULT|GNOME_VFS_FILE_INFO_FOLLOW_LINKS,
										10,GNOME_VFS_PRIORITY_DEFAULT,open_adv_load_directory_lcb,oa);
	}
}





