/* Bluefish HTML Editor
 * file.c - file operations based on GIO
 *
 * Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2009 Olivier Sessink
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

/*#define DEBUG*/

#include <gtk/gtk.h>
#include <string.h> /* memcpy */

#include "bluefish.h"
#include "bf_lib.h"
#include "bookmark.h"
#include "dialog_utils.h"
#include "document.h"
#include "project.h"
#include "file.h"
#include "filebrowser2.h"
#include "file_dialogs.h"
#include "gtk_easy.h"
#include "gui.h"
#include "stringlist.h"

#if !GTK_CHECK_VERSION(2,14,0)
#include "gtkmountoperation.h"
#endif

void DEBUG_URI(GFile * uri, gboolean newline)
{
	gchar *name = g_file_get_uri(uri);
	DEBUG_MSG("%s", name);
	if (newline) {
		DEBUG_MSG("\n");
	}
	g_free(name);
}

/* queue functions */
typedef struct {
	guint worknum; /* number of elements that are being worked on */
	GList *head; /* data structures that are *not* being worked on */
	GList *tail;
	/*guint task_id;*/ /* the event source id of the task running with g_timeout_add(), or 0 if no task is running */
	guint max_worknum;
	/*void (*activate_func) ();*/
} Tqueue;

static void queue_init(Tqueue *queue, guint max_worknum) {
	queue->worknum=0;
	queue->tail=queue->head=NULL;
	queue->max_worknum=max_worknum;
} 

static void queue_run(Tqueue *queue, void (*activate_func) () )  {
	if (queue->worknum < queue->max_worknum) {
		while (queue->tail != NULL && queue->worknum < queue->max_worknum) {
			GList *curlst = queue->tail;
			queue->tail = curlst->prev;
			if (queue->tail == NULL)
				queue->head = NULL;
			else
				queue->tail->next = NULL;
			activate_func(curlst->data);
			queue->worknum++;
			g_list_free_1(curlst);
			/*g_print("queue_run, %d working, %d queued\n",queue->worknum,g_list_length(queue->head));*/
		}
	}
}

static void queue_worker_ready(Tqueue *queue, void (*activate_func) ()) {
	queue->worknum--;
	queue_run(queue,activate_func);
}

static void queue_push(Tqueue *queue, gpointer item, void (*activate_func) ()) {
	queue->head = g_list_prepend(queue->head, item);
	if (queue->tail==NULL)
		queue->tail=queue->head;
	queue_run(queue,activate_func);
}
/*
static gboolean process_queue(gpointer data) {
	Tqueue *queue = (Tqueue *)data;
	GList *lastlst = g_list_last(queue->todo);
	while (lastlst != NULL && queue->worknum < queue->max_worknum) {
		GList *curlst = lastlst;
		lastlst = curlst->prev;
		queue->todo = g_list_remove_link(queue->todo, curlst);
		queue->activate_func(curlst->data);
		g_list_free_1(curlst);
	}
	if (queue->todo == NULL) {
		queue->task_id = 0;
		return FALSE;
	}
	return TRUE;
}

static void push_to_queue(Tqueue *queue, gpointer data) {
	if (queue->worknum >= queue->max_worknum) {
		queue->todo = g_list_append(queue->todo, data);
		if (queue->task_id == 0) {
			queue->task_id = g_timeout_add(250, process_queue, queue);
		}
	} else {
		queue->activate_func(data);
	}
}*/

/*************************** FILE DELETE ASYNC ******************************/
typedef struct {
	DeleteAsyncCallback callback;
	gpointer callback_data;
	GFile *uri;
	gboolean recursive;
} Tdelete;

static gboolean delete_async_finished_lcb(gpointer data) {
	Tdelete *del = data;
	if (del->callback) {
		del->callback(del->callback_data);
	}
	g_object_unref(del->uri);
	g_free(del);
	return FALSE;
}

static void delete_recursive(GFile *dir) {
	GFileEnumerator *enumer;
	GError *error=NULL;
	GFileInfo *finfo;
	
	enumer = g_file_enumerate_children(dir,"standard::type,standard::name",0,NULL,&error);
	if (!enumer|| error) {
		return;
	}
	do {
		finfo = g_file_enumerator_next_file(enumer,NULL,&error);
		GFile *child;
		const gchar *name;
		if (error) {
			g_print("delete_recursive, next file, got error %d %s\n",error->code,error->message);
			g_error_free(error);
			error=NULL;
			break;
		}
		if (finfo) {
			name = g_file_info_get_name(finfo);
			if (strcmp(name,".")==0||strcmp(name,"..")==0) {
				g_object_unref(finfo);
				break;
			} 
			child = g_file_get_child(dir,g_file_info_get_name(finfo));
			if (g_file_info_get_file_type(finfo)==G_FILE_TYPE_DIRECTORY) {
				delete_recursive(child);
			}
			g_file_delete(child,NULL,&error);
			if (error) {
				g_print("delete_recursive, delete, got error %d %s\n",error->code,error->message);
				g_error_free(error);
				error=NULL;
			}
			g_object_unref(child);
			g_object_unref(finfo);
		}
	} while (finfo && !error);
	g_file_enumerator_close(enumer,NULL,&error);
	if (error) {
		g_print("delete_recursive, close enumer, got error %d %s\n",error->code,error->message);
		g_error_free(error);
		error=NULL;
	}
	g_object_unref(enumer);
}

static gboolean delete_async(GIOSchedulerJob *job,GCancellable *cancellable,gpointer user_data) {
	Tdelete *del = user_data;
	GError *error=NULL;	
	g_file_delete(del->uri,NULL,&error);
	if (error) {
		if (error->code == G_IO_ERROR_NOT_EMPTY && del->recursive) {
			GError *err2=NULL;
			delete_recursive(del->uri);
			g_file_delete(del->uri,NULL,&err2);
			if (err2) {
				g_print("delete_async, after recursion failed to delete: %s\n",error->message);
				g_error_free(err2);
			}
		} else {
			g_print("delete_async, failed to delete: %s\n",error->message);
		}
		g_error_free(error);
	}
	g_idle_add_full(G_PRIORITY_LOW,delete_async_finished_lcb, del,NULL);
	
	return FALSE;
}

void file_delete_async(GFile *uri, gboolean recursive, DeleteAsyncCallback callback, gpointer callback_data) {
	Tdelete *del = g_new0(Tdelete,1);
	g_object_ref(uri);
	del->uri = uri;
	del->callback = callback;
	del->callback_data = callback_data;
	del->recursive = recursive;
	g_io_scheduler_push_job(delete_async, del,NULL,G_PRIORITY_LOW,NULL);
}

/*************************** FILE INFO ASYNC ******************************/
static void checkmodified_cleanup(Tcheckmodified *cm) {
	g_object_unref(cm->uri);
	g_object_unref(cm->cancel);
	g_object_unref(cm->orig_finfo);
	g_free(cm);
}
void checkmodified_cancel(Tcheckmodified *cm) {
	g_cancellable_cancel(cm->cancel);
}


static gboolean checkmodified_is_modified(GFileInfo *orig, GFileInfo *new) {
	/* modified_check_type;  0=no check, 1=by mtime and size, 2=by mtime, 3=by size, 4,5,...not implemented (md5sum?) */
	if (main_v->props.modified_check_type == 1 || main_v->props.modified_check_type == 2) {
		if (g_file_info_get_attribute_uint64(orig,G_FILE_ATTRIBUTE_TIME_MODIFIED) != g_file_info_get_attribute_uint64(new,G_FILE_ATTRIBUTE_TIME_MODIFIED)) return TRUE;
	}
	if (main_v->props.modified_check_type == 1 || main_v->props.modified_check_type == 3) {
		if (g_file_info_get_size(orig) != g_file_info_get_size(new)) return TRUE;
	}
	return FALSE;
}

static void checkmodified_asyncfileinfo_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
	GFileInfo *info;
	GError *error=NULL;
	Tcheckmodified *cm = user_data;
	info = g_file_query_info_finish(cm->uri,res,&error);
	if (info) {
		if (checkmodified_is_modified(cm->orig_finfo, info)) {
			cm->callback_func(CHECKMODIFIED_MODIFIED, 0, cm->orig_finfo, info, cm->callback_data);
		} else {
			cm->callback_func(CHECKMODIFIED_OK, 0, cm->orig_finfo, info, cm->callback_data);
		}
	} else {
		/* error condition */
		DEBUG_MSG("************************ checkmodified_asyncfileinfo_lcb, non-handled error condition\n");
		
		cm->callback_func(CHECKMODIFIED_ERROR, error->code, NULL, NULL, cm->callback_data);
		g_error_free(error);
	}
	checkmodified_cleanup(cm);
}


Tcheckmodified *file_checkmodified_uri_async(GFile *uri, GFileInfo *curinfo, CheckmodifiedAsyncCallback callback_func, gpointer callback_data) {
	Tcheckmodified *cm;
	if (curinfo == NULL) {
		callback_func(CHECKMODIFIED_OK, 0, NULL, NULL, callback_data);
		return NULL;
	}
	cm = g_new(Tcheckmodified,1);
	cm->callback_func = callback_func;
	cm->callback_data = callback_data;
	g_object_ref(uri);
	cm->uri = uri;
	g_object_ref(curinfo);
	cm->orig_finfo = curinfo;
	g_object_ref(curinfo);
	cm->cancel = g_cancellable_new();

	g_file_query_info_async(uri,"standard::size,unix::mode,unix::uid,unix::gid,time::modified"
					,G_FILE_QUERY_INFO_NONE
					,G_PRIORITY_DEFAULT
					,cm->cancel /*cancellable*/
					,checkmodified_asyncfileinfo_lcb,cm);
	return cm;
}

/*************************** CHECK MODIFIED AND SAVE ASYNC ******************************/

typedef struct {
	gsize buffer_size;
	GFile *uri;
	GFileInfo *finfo;
	const gchar *etag;
	Tsavefile *sf; /* to cancel the actual save */
	Tcheckmodified *cm; /* to cancel the checkmodified check */
	Trefcpointer *buffer;
	gboolean check_modified;
	CheckNsaveAsyncCallback callback_func;
	gpointer callback_data;
	gboolean abort; /* the backup callback may set this to true, it means that the user choosed to abort save because the backup failed */
} TcheckNsave;

static void checkNsave_cleanup(TcheckNsave *cns) {
	DEBUG_MSG("checkNsave_cleanup, called for %p\n",cns);
	refcpointer_unref(cns->buffer);
	g_object_unref(cns->uri);
	if (cns->finfo) g_object_unref(cns->finfo);
	g_free(cns);
}

static void checkNsave_replace_async_lcb(GObject *source_object,GAsyncResult *res, gpointer user_data) {
	TcheckNsave *cns = user_data;
	gboolean retval;
	char *etag=NULL;
	GError *error=NULL;
	
	retval = g_file_replace_contents_finish(cns->uri,res,&etag,&error);
	if (error) {
		DEBUG_MSG("checkNsave_replace_async_lcb,error %d: %s\n",error->code,error->message);
		if (error->code == G_IO_ERROR_WRONG_ETAG) {
			if (cns->callback_func(CHECKANDSAVE_ERROR_MODIFIED,error->code, cns->callback_data) == CHECKNSAVE_CONT) {
				g_file_replace_contents_async(cns->uri,cns->buffer->data,cns->buffer_size
						,NULL,TRUE
						,G_FILE_CREATE_NONE,NULL
						,checkNsave_replace_async_lcb,cns);
				return;
			}
		} else if (error->code == G_IO_ERROR_CANT_CREATE_BACKUP) {
			if (cns->callback_func(CHECKANDSAVE_ERROR_NOBACKUP, 0, cns->callback_data) == CHECKNSAVE_CONT) {
				g_file_replace_contents_async(cns->uri,cns->buffer->data,cns->buffer_size
						,cns->etag,FALSE
						,G_FILE_CREATE_NONE,NULL
						,checkNsave_replace_async_lcb,cns);
				return;
			} 
		}
#if !GLIB_CHECK_VERSION(2, 18, 0)	
		else if (error->code == G_IO_ERROR_EXISTS && (g_file_has_uri_scheme(cns->uri, "sftp")||g_file_has_uri_scheme(cns->uri, "smb"))) {
			/* there is  a bug in the GIO sftp and smb module in glib version 2.18 that returns 'file exists error' 
			if you request an async content_replace */
			GError *error2=NULL;
			g_file_replace_contents(cns->uri,cns->buffer->data,cns->buffer_size
					,cns->etag,main_v->props.backup_file
					, G_FILE_CREATE_NONE,NULL
					,NULL, &error2);
			if (error2) {
				g_print("sync version returns error %d: %s\n",error->code,error->message);
				cns->callback_func(CHECKANDSAVE_ERROR, 0, cns->callback_data);
				g_error_free(error2);
			} else {
				cns->callback_func(CHECKANDSAVE_FINISHED, 0, cns->callback_data);
			}
		}
#endif
		 else {
			g_print("****************** checkNsave_replace_async_lcb() unhandled error %d: %s\n",error->code,error->message);
			cns->callback_func(CHECKANDSAVE_ERROR, 0, cns->callback_data);
		}
		g_error_free(error);
	} else {
#if !GLIB_CHECK_VERSION(2, 18, 0)
		/* a bug in the fuse smbnetfs mount code */
		if (g_file_has_uri_scheme(cns->uri, "smb")) {	
			/* check that file exists/got created */
			if (!g_file_query_exists(cns->uri,NULL)) {
				g_file_replace_contents_async(cns->uri,cns->buffer->data,cns->buffer_size
						,cns->etag,FALSE /* we already created a backup */
						,G_FILE_CREATE_NONE,NULL
						,checkNsave_replace_async_lcb,cns);
				return;			
			}
		}
#endif
		cns->callback_func(CHECKANDSAVE_FINISHED, 0, cns->callback_data);
		
	}
	checkNsave_cleanup(cns);
}

gpointer file_checkNsave_uri_async(GFile *uri, GFileInfo *info, Trefcpointer *buffer, gsize buffer_size, gboolean check_modified, CheckNsaveAsyncCallback callback_func, gpointer callback_data) {
	TcheckNsave *cns;
	cns = g_new0(TcheckNsave,1);
	/*cns->etag=NULL;*/
	cns->callback_data = callback_data;
	cns->callback_func = callback_func;
	cns->buffer = buffer;
	refcpointer_ref(buffer);
	cns->buffer_size = buffer_size;
	cns->uri = uri;
	g_object_ref(uri);
	cns->finfo = info;
	cns->check_modified = check_modified;
	cns->abort = FALSE;
	if (info) {
		g_object_ref(info);
		if (check_modified && g_file_info_has_attribute(info,G_FILE_ATTRIBUTE_ETAG_VALUE)) {
			cns->etag = g_file_info_get_etag(info);
			DEBUG_MSG("file_checkNsave_uri_async, using etag=%s\n",cns->etag);
		}
	}
	DEBUG_MSG("file_checkNsave_uri_async, saving %ld bytes to ",(long int)cns->buffer_size);
	DEBUG_URI(cns->uri, TRUE);
	g_file_replace_contents_async(cns->uri,cns->buffer->data,cns->buffer_size
					,cns->etag,main_v->props.backup_file ? TRUE : FALSE
					,G_FILE_CREATE_NONE,NULL
					,checkNsave_replace_async_lcb,cns);
	return cns;
}
/*
GFile *backup_uri_from_orig_uri(GFile * origuri) {
	gchar *tmp, *tmp2;
	GFile *ret;
	
	tmp = g_file_get_parse_name(origuri);
	tmp2 = g_strconcat(tmp,"~",NULL);
	
	ret = g_file_parse_name(tmp2);
	g_free(tmp);
	g_free(tmp2);
	return ret;
}*/

/*************************** OPEN FILE ASYNC ******************************/
typedef struct {
	GList *todo;
	guint worknum;
} Tofqueue;
static Tofqueue ofqueue = {NULL,0};
#define OF_MAX_WORKNUM 32

static void process_ofqueue(gpointer data);

static void openfile_cleanup(Topenfile *of) {
	g_object_unref(of->uri);
	g_object_unref(of->cancel);
	g_free(of);
	ofqueue.worknum--;
	process_ofqueue(NULL);
}
void openfile_cancel(Topenfile *of) {
	g_cancellable_cancel(of->cancel);
}

static void openfile_async_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data);

static void openfile_async_mount_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
	Topenfile *of = user_data;
	GError *error=NULL;
	if (g_file_mount_enclosing_volume_finish(of->uri,res,&error)) {
		/* open again */
		g_file_load_contents_async(of->uri,of->cancel,openfile_async_lcb,of);
	} else {
		g_print("failed to mount with error %d %s!!\n",error->code,error->message);
		of->callback_func(OPENFILE_ERROR,error->code,NULL,0, of->callback_data);		
	}
}

static void openfile_async_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
	Topenfile *of = user_data;
	gboolean retval;
	GError *error=NULL;
	gchar *buffer=NULL;
	gchar *etag=NULL;
	gsize size=0;
	
	retval = g_file_load_contents_finish(of->uri,res,&buffer,&size,&etag,&error);
	if (error) {
		DEBUG_MSG("openfile_async_lcb, finished, received error code %d: %s\n",error->code,error->message);
		if (error->code == G_IO_ERROR_NOT_MOUNTED) {
#if 1
			GMountOperation * gmo;
			g_print("not mounted, try to mount!!\n");
			gmo = gtk_mount_operation_new(of->bfwin?(GtkWindow *)of->bfwin->main_window:NULL); /* TODO, add bfwin to the Topenfile */
			g_file_mount_enclosing_volume(of->uri
					,G_MOUNT_MOUNT_NONE
					,gmo
					,of->cancel
					,openfile_async_mount_lcb,of);
#else
			g_print("TODO, CREATE GMOUNTOPERATION FOR BACKWARDS COMPATIBILITY WITH OLDER GTK?!?!\n");
			/* for anonymous ftp */
			if (g_file_has_uri_scheme(of->uri,"ftp")) {
				gchar *curi, *starthost;
				/* check if there is a user name in the URI, if not, prepend anonymous@ in front of the URI */
				curi = g_file_get_uri(of->uri);
				if (strlen(curi)>8) {
					gchar *endhost, *at;
					starthost = curi+6; /* ftp:// is 6 bytes */
					endhost = strchr(starthost,'/');
					at = strchr(starthost,'@');
					if ((!at) || (endhost && at > endhost)) {
						gchar *newcuri;
						GFile *nfile;
						/* insert anonymous */
						newcuri = g_strconcat("ftp://anonymous@",starthost,NULL);
						g_print("newcuri=%s\n",newcuri);
						nfile = g_file_new_for_uri(newcuri);
						g_object_unref(of->uri);
						of->uri = nfile;
						g_free(newcuri);
					}
				}
				g_free(curi);
			} 
			g_file_mount_enclosing_volume(of->uri
					,G_MOUNT_MOUNT_NONE
					,NULL
					,of->cancel
					,openfile_async_mount_lcb,of);
#endif
		} else {
			of->callback_func(OPENFILE_ERROR,error->code,buffer,size, of->callback_data);
			g_free(buffer);
		}
		g_error_free(error);
	} else {
		DEBUG_MSG("openfile_async_lcb, finished, received %d bytes\n",size);
		of->callback_func(OPENFILE_FINISHED,0,buffer,size, of->callback_data);
		openfile_cleanup(of);
		g_free(buffer);
	}	
}

static void process_ofqueue(gpointer data) {
	Topenfile *of;
	if (ofqueue.todo == NULL) {
		return;
	}
	if (ofqueue.worknum > OF_MAX_WORKNUM) { /* load max OAD_MAX_WORKNUM directories simultaneously */
		return;
	}
	while (ofqueue.todo!=NULL && ofqueue.worknum <= OF_MAX_WORKNUM) {
		of = ofqueue.todo->data;
		ofqueue.todo = g_list_delete_link(ofqueue.todo, ofqueue.todo);
		ofqueue.worknum++;
		DEBUG_MSG("process_ofqueue, processing next uri %p ",of->uri);
		DEBUG_URI(of->uri, TRUE);
		g_file_load_contents_async(of->uri,of->cancel,openfile_async_lcb,of);
	}
}

Topenfile *file_openfile_uri_async(GFile *uri, Tbfwin *bfwin, OpenfileAsyncCallback callback_func, gpointer callback_data) {
	Topenfile *of;
	of = g_new(Topenfile,1);
	of->callback_data = callback_data;
	of->callback_func = callback_func;
	of->uri = uri;
	of->bfwin = bfwin;
	of->cancel = g_cancellable_new();
	g_object_ref(of->uri);
	ofqueue.todo = g_list_prepend(ofqueue.todo, of);
	process_ofqueue(NULL);
	return of;
}

/************ LOAD A FILE ASYNC INTO A DOCUMENT ************************/
typedef struct {
	Tbfwin *bfwin;
	Tdocument *doc;
	GFile *uri;
	gboolean isTemplate;
} Tfileintodoc;

static void fileintodoc_cleanup(Tfileintodoc *fid) {
	g_object_unref(fid->uri);
	g_free(fid);
}

static void fileintodoc_lcb(Topenfile_status status,gint error_info,gchar *buffer,goffset buflen ,gpointer data) {
	Tfileintodoc *fid = data;
	switch (status) {
		case OPENFILE_FINISHED:
			if (fid->isTemplate) {
				doc_buffer_to_textbox(fid->doc, buffer, buflen, FALSE, TRUE);
	/*			DEBUG_MSG("fileintodoc_lcb, fid->doc->hl=%p, %s, first=%p\n",fid->doc->hl,fid->doc->hl->type,((GList *)g_list_first(main_v->filetypelist))->data);*/
				doc_reset_filetype(fid->doc, fid->doc->uri, buffer, buflen);
				doc_set_tooltip(fid->doc);
				doc_set_status(fid->doc, DOC_STATUS_COMPLETE);
				bfwin_docs_not_complete(fid->doc->bfwin, FALSE);
				fid->doc->action.load = NULL;
			} else { /* file_insert, convert to UTF-8 and insert it! */
				gchar *encoding, *newbuf;
				newbuf = buffer_find_encoding(buffer, buflen, &encoding, BFWIN(fid->doc->bfwin)->session->encoding);
				if (newbuf) {
					GtkTextIter iter;
					gtk_text_buffer_get_iter_at_mark(fid->doc->buffer,&iter,gtk_text_buffer_get_insert(fid->doc->buffer));
					gtk_text_buffer_insert(fid->doc->buffer,&iter,newbuf,-1);
					g_free(newbuf);
					g_free(encoding);
				}
			}
			if (fid->bfwin->current_document == fid->doc) {
				doc_force_activate(fid->doc);
			}
			fileintodoc_cleanup(data);
		break;
		case OPENFILE_ERROR_CANCELLED: /* hmm what to do here ? */
			if (fid->isTemplate) {
				doc_buffer_to_textbox(fid->doc, buffer, buflen, FALSE, TRUE);
			} else {
				/* do nothing */
			}
			fid->doc->action.load = NULL;
			fileintodoc_cleanup(data);
		break;
		case OPENFILE_CHANNEL_OPENED:
			/* do nothing */
		{
			gchar *utf8uri, *tmp;
			utf8uri = gfile_display_name(fid->uri, NULL);
			tmp = g_strdup_printf("Loading %s", utf8uri);
			statusbar_message(fid->doc->bfwin,tmp, 1000);
			g_free(tmp);
			g_free(utf8uri);
		}
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

/* used for template loading, and for file_insert */
void file_into_doc(Tdocument *doc, GFile *uri, gboolean isTemplate) {
	Tfileintodoc *fid;
	fid = g_new(Tfileintodoc,1);
	fid->bfwin = doc->bfwin;
	fid->doc = doc;
	fid->isTemplate = isTemplate;
	if (isTemplate) {
		doc_set_status(doc, DOC_STATUS_LOADING);
		bfwin_docs_not_complete(doc->bfwin, TRUE);
	}
	fid->uri = uri;
	g_object_ref(uri);
	file_openfile_uri_async(fid->uri, doc->bfwin,fileintodoc_lcb,fid);
}

/************ MAKE DOCUMENT FROM ASYNC OPENED FILE ************************/
typedef struct {
	Topenfile *of;
	Tbfwin *bfwin;
	Tdocument *doc;
	GFile *uri;
	gboolean readonly;
} Tfile2doc;

static void file2doc_cleanup(Tfile2doc *f2d) {
	DEBUG_MSG("file2doc_cleanup, %p, num open documents=%d\n",f2d,g_list_length(f2d->bfwin->documentlist));
	g_object_unref(f2d->uri);
	g_slice_free(Tfile2doc, f2d);
}

void file2doc_cancel(gpointer f2d) {
	DEBUG_MSG("file2doc_cancel, called for %p\n",f2d);
	openfile_cancel(((Tfile2doc *)f2d)->of);
	/* no cleanup, there is a CANCELLED callback coming */
}

static void file2doc_lcb(Topenfile_status status,gint error_info,gchar *buffer,goffset buflen ,gpointer data) {
	Tfile2doc *f2d = data;
	DEBUG_MSG("file2doc_lcb, status=%d, f2d=%p\n",status,f2d);
	switch (status) {
		case OPENFILE_FINISHED:
			DEBUG_MSG("finished loading data in memory for view %p\n",f2d->doc->view);
			doc_buffer_to_textbox(f2d->doc, buffer, buflen, FALSE, TRUE);
			doc_reset_filetype(f2d->doc, f2d->doc->uri, buffer,buflen);
			doc_set_tooltip(f2d->doc);
			doc_set_status(f2d->doc, DOC_STATUS_COMPLETE);
			bfwin_docs_not_complete(f2d->doc->bfwin, FALSE);
			bmark_set_for_doc(f2d->doc,TRUE);
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
			{
				gchar *utf8uri, *tmp;
				utf8uri = gfile_display_name(f2d->uri,NULL);
				if (BFWIN(f2d->bfwin)->num_docs_not_completed > 0) {
					tmp = g_strdup_printf(_("Still loading %d files, finished %s"), BFWIN(f2d->bfwin)->num_docs_not_completed, utf8uri);
				} else {
					tmp = g_strdup_printf(_("All files loaded, finished %s"), utf8uri);
				}
				statusbar_message(f2d->doc->bfwin,tmp, 3000);
				g_free(tmp);
				g_free(utf8uri);
			}
			f2d->doc->action.goto_line = -1;
			f2d->doc->action.goto_offset = -1;
			f2d->doc->action.load = NULL;
			file2doc_cleanup(data);
			DEBUG_MSG("finished data in document view %p\n",f2d->doc->view);
		break;
		case OPENFILE_CHANNEL_OPENED:
			/* do nothing */
		break;
   	case OPENFILE_ERROR_CANCELLED:
		/* lets close the document */
		f2d->doc->action.load = NULL;
		DEBUG_MSG("file2doc_lcb, calling doc_close_single_backend\n");
		doc_close_single_backend(f2d->doc, FALSE, f2d->doc->action.close_window);
		file2doc_cleanup(f2d);
   	break;
		case OPENFILE_ERROR:
		case OPENFILE_ERROR_NOCHANNEL:
		case OPENFILE_ERROR_NOREAD:
			DEBUG_MSG("file2doc_lcb, ERROR status=%d, cleanup!!!!!\n",status);
			if (f2d->doc->action.close_doc) {
				f2d->doc->action.load = NULL;
				doc_close_single_backend(f2d->doc, FALSE, f2d->doc->action.close_window);
			} else {
				doc_set_status(f2d->doc, DOC_STATUS_ERROR);
			}
			f2d->doc->action.load = NULL;
			file2doc_cleanup(f2d);
		break;
	}
}

typedef struct {
	Tdocument *doc;
	GFile *uri;
	GCancellable *cancel;
} Tfileinfo;

void file_asyncfileinfo_cancel(gpointer fi) {
	g_cancellable_cancel(((Tfileinfo *)fi)->cancel);
}
static void fill_fileinfo_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
	GFileInfo *info = NULL;
	GError *error=NULL;
	Tfileinfo *fi=user_data;
	
	info = g_file_query_info_finish(fi->uri,res,&error);
	if (info) {
		if (fi->doc->fileinfo) {
			g_object_unref(fi->doc->fileinfo);
		}
		fi->doc->fileinfo = info;
		g_object_ref(info);
		doc_set_tooltip(fi->doc);
	}
	fi->doc->action.info = NULL;
	if (fi->doc->action.close_doc) {
		doc_close_single_backend(fi->doc, FALSE, fi->doc->action.close_window);
	}
	g_object_unref(fi->uri);
	g_object_unref(fi->cancel);
	g_free(fi);
}

void file_doc_fill_fileinfo(Tdocument *doc, GFile *uri) {
	Tfileinfo *fi;
	fi = g_new(Tfileinfo,1);
	DEBUG_MSG("file_doc_fill_fileinfo, started for doc %p and uri %p at fi=%p\n",doc,uri,fi);
	fi->doc = doc;
	fi->doc->action.info = fi;
	g_object_ref(uri);
	fi->uri = uri;
	fi->cancel = g_cancellable_new();
	
	g_file_query_info_async(fi->uri,BF_FILEINFO
					,G_FILE_QUERY_INFO_NONE
					,G_PRIORITY_LOW
					,fi->cancel
					,fill_fileinfo_lcb,fi);
}

void file_doc_retry_uri(Tdocument *doc) {
	Tfile2doc *f2d;
	f2d = g_slice_new(Tfile2doc);
	f2d->bfwin = doc->bfwin;
	f2d->doc = doc;
	
	if (doc->status != DOC_STATUS_ERROR) {
		bfwin_docs_not_complete(doc->bfwin, TRUE);
	}
	doc_set_status(doc, DOC_STATUS_LOADING);
	/* this forces an activate on the document, which will call widget_show() on the textview */
	BFWIN(doc->bfwin)->focus_next_new_doc = TRUE;
	f2d->uri = doc->uri;
	g_object_ref(doc->uri);
	if (doc->fileinfo == NULL) {
		file_doc_fill_fileinfo(f2d->doc, f2d->uri);
	}
	f2d->of = file_openfile_uri_async(f2d->uri,doc->bfwin,file2doc_lcb,f2d);
}

void file_doc_fill_from_uri(Tdocument *doc, GFile *uri, GFileInfo *finfo, gint goto_line) {
	Tfile2doc *f2d;
	f2d = g_slice_new(Tfile2doc);
	f2d->bfwin = doc->bfwin;
	f2d->uri = g_object_ref(uri);
	f2d->doc = doc;
	f2d->doc->action.load = f2d;
	f2d->doc->action.goto_line = goto_line;
	/* this forces an activate on the document, which will call widget_show() on the textview */
	BFWIN(doc->bfwin)->focus_next_new_doc = TRUE;
	if (finfo == NULL) {
		file_doc_fill_fileinfo(f2d->doc, uri);
	}
	f2d->of = file_openfile_uri_async(f2d->uri,doc->bfwin,file2doc_lcb,f2d);
}

/* this funcion is usually used to load documents */
void file_doc_from_uri(Tbfwin *bfwin, GFile *uri, GFileInfo *finfo, gint goto_line, gint goto_offset, gboolean readonly) {
	Tfile2doc *f2d;
	f2d = g_slice_new(Tfile2doc);
	DEBUG_MSG("file_doc_from_uri, open uri %p, f2d=%p\n", uri, f2d);
	f2d->bfwin = bfwin;
	f2d->uri = uri;
	f2d->readonly = readonly;
	g_object_ref(uri);
	f2d->doc = doc_new_loading_in_background(bfwin, uri, finfo, readonly);
	f2d->doc->action.load = f2d;
	f2d->doc->action.goto_line = goto_line;
	f2d->doc->action.goto_offset = goto_offset;
	DEBUG_MSG("file_doc_from_uri, got doc %p\n",f2d->doc);
	if (finfo == NULL) {
		/* get the fileinfo also async */
		file_doc_fill_fileinfo(f2d->doc, uri);
	}
	f2d->of = file_openfile_uri_async(f2d->uri,bfwin,file2doc_lcb,f2d);
}

/*************************** OPEN ADVANCED ******************************/

typedef struct {
	GList *todo;
	guint worknum;
} Toadqueue; /* a queue of Topenadv_dir to avoid 'Too many open files' errors */

static Toadqueue oadqueue = {NULL, 0};
#define OAD_MAX_WORKNUM 16
#define OAD_NUM_FILES_PER_CB 40
static void process_oadqueue(gpointer data);

#define LOAD_TIMER
typedef struct {
	guint refcount;
	Tbfwin *bfwin;
	gboolean recursive;
	gboolean matchname;
	gchar *extension_filter;
	GPatternSpec* patspec;
	gchar *content_filter;
	gboolean use_regex;
	pcre *contentf_pcre; /* a compiled content_filter */
	GFile *topbasedir; /* the top directory where advanced open started */
#ifdef LOAD_TIMER
	GTimer *timer;
#endif
} Topenadv;

typedef struct {
	Topenadv *oa;
	GFile *basedir;
	GFileEnumerator *gfe;
} Topenadv_dir;

typedef struct {
	Topenadv *oa;
	GFile *uri;
	GFileInfo *finfo;
} Topenadv_uri;

static void openadv_unref(Topenadv *oa) {
	oa->refcount--;
	DEBUG_MSG("openadv_unref, count=%d\n",oa->refcount);
	if (oa->refcount <= 0 ) {
		gchar *tmp, *tmp2;
		tmp = g_strdup_printf(ngettext("%d document open.","%d documents open.",g_list_length(oa->bfwin->documentlist)),
				g_list_length(oa->bfwin->documentlist));
		tmp2 = g_strconcat("Advanced open: Finished searching files. ",tmp,NULL);
		statusbar_message(oa->bfwin, tmp2, 4000);
#ifdef LOAD_TIMER
		g_print("%f ms, %s\n",g_timer_elapsed(oa->timer,NULL), tmp2);
		g_timer_destroy(oa->timer);
#endif
		g_free(tmp);
		g_free(tmp2);
		if (oa->extension_filter) g_free(oa->extension_filter);
		if (oa->patspec) g_pattern_spec_free(oa->patspec);
		if (oa->content_filter) g_free(oa->content_filter);
		if (oa->contentf_pcre) pcre_free(oa->contentf_pcre);
		if (oa->topbasedir) g_object_unref(oa->topbasedir);
		g_free(oa);
	}
}

static void open_adv_open_uri_cleanup(Topenadv_uri *oau) {
	g_object_unref(oau->uri);
	g_object_unref(oau->finfo);
	openadv_unref(oau->oa);
	g_free(oau);
}

static gboolean open_adv_content_matches_filter(gchar *buffer,goffset buflen, Topenadv_uri *oau) {
	if (oau->oa->use_regex) {
		int rc;
		int ovector[30];
		rc = pcre_exec(oau->oa->contentf_pcre, NULL, buffer, buflen, 0, 0, ovector, 30);
		return rc;
	} else {
		return (strstr(buffer, oau->oa->content_filter) != NULL);
	}
	return FALSE;
}

static void open_adv_content_filter_lcb(Topenfile_status status,gint error_info, gchar *buffer,goffset buflen,gpointer data) {
	Topenadv_uri *oau = data;
	switch (status) {
		case OPENFILE_FINISHED:
			DEBUG_MSG("open_adv_content_filter_lcb, status=%d, now we should do the content filtering\n",status);
			/* we have all content, do the filtering, and if correct, open the file as document */
			if (open_adv_content_matches_filter(buffer,buflen,oau)) {
				Tfile2doc *f2d = g_slice_new(Tfile2doc);
				f2d->uri = oau->uri;
				g_object_ref(oau->uri);
				f2d->bfwin = oau->oa->bfwin;
				f2d->doc = doc_new_loading_in_background(oau->oa->bfwin, oau->uri, oau->finfo, FALSE);
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

static void openadv_content_filter_file(Topenadv *oa, GFile *uri, GFileInfo* finfo) {
	Topenadv_uri *oau;
	Tdocument *tmpdoc;
	GList *alldocs;
	
	/* don't content filter if the file is an already opened document */
	alldocs = return_allwindows_documentlist();
	tmpdoc = documentlist_return_document_from_uri(alldocs, uri);
	g_list_free(alldocs);
	if (tmpdoc)
		return;
	
	oau = g_new0(Topenadv_uri,1);
	oau->oa = oa;
	oa->refcount++;
	oau->uri = uri;
	g_object_ref(uri);

	oau->finfo = g_file_info_dup(finfo);

	file_openfile_uri_async(uri, oa->bfwin, open_adv_content_filter_lcb, oau);
}

static void open_advanced_backend(Topenadv *oa, GFile *basedir);

static void open_adv_load_directory_cleanup(Topenadv_dir *oad) {
	DEBUG_MSG("open_adv_load_directory_cleanup %p\n", oad);
	g_object_unref(oad->basedir);
	if (oad->gfe)
		g_object_unref(oad->gfe);
	openadv_unref(oad->oa);
	g_free(oad);
	oadqueue.worknum--;
	process_oadqueue(NULL);
}

static void enumerator_next_files_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
	GList *list, *tmplist;
	GError *error=NULL;
	Topenadv_dir *oad = user_data;
	GList *alldoclist;
	
	list = tmplist = g_file_enumerator_next_files_finish (oad->gfe,res,&error);
	DEBUG_MSG("enumerator_next_files_lcb for oad=%p has %d results\n",oad,g_list_length(list));
	if (!list ) {
		/* cleanup */
		open_adv_load_directory_cleanup(oad);
		return;
	}
	alldoclist = return_allwindows_documentlist();
	while (tmplist) {
		GFileInfo *finfo=tmplist->data;
		if (g_file_info_get_file_type(finfo)==G_FILE_TYPE_DIRECTORY) {
			GFile *dir;
			const gchar *name = g_file_info_get_name(finfo);
			DEBUG_MSG("enumerator_next_files_lcb, %s is a dir\n",name);
			dir = g_file_get_child(oad->basedir,name);
			open_advanced_backend(oad->oa, dir);
			g_object_unref(dir);
		} else if (g_file_info_get_file_type(finfo)==G_FILE_TYPE_REGULAR) {
			GFile *child_uri;
			const gchar *name = g_file_info_get_name(finfo);
			DEBUG_MSG("enumerator_next_files_lcb, %s is a regular file\n",name);
			child_uri = g_file_get_child(oad->basedir,name);

			if (oad->oa->patspec) {
				gchar *nametomatch;
				/* check if we have to match the name only or path+name */
				if (oad->oa->matchname) {
					nametomatch = g_strdup(name);
				} else {
					nametomatch = g_file_get_uri(child_uri);
				}
				DEBUG_MSG("open_adv_load_directory_lcb, matching on %s\n",nametomatch);
				if (g_pattern_match_string(oad->oa->patspec, nametomatch)) { /* test extension */
					if (oad->oa->content_filter) { /* do we need content filtering */
						openadv_content_filter_file(oad->oa, child_uri, finfo);
					} else { /* open this file as document */
						doc_new_from_uri(oad->oa->bfwin, child_uri, finfo, TRUE, FALSE, -1, -1);
					}
				}
				g_free(nametomatch);
			} else if (oad->oa->content_filter) {
			/* content filters are expensive, first see if this file is already open */
				if (documentlist_return_document_from_uri(alldoclist, child_uri)==NULL) { /* if this file is already open, there is no need to do any of these checks */
					openadv_content_filter_file(oad->oa, child_uri, finfo);
				}
			} else {
				doc_new_from_uri(oad->oa->bfwin, child_uri, finfo, TRUE, FALSE, -1, -1);
			}
		}
		g_object_unref(finfo);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(list);
	g_list_free(alldoclist);
	g_file_enumerator_next_files_async(oad->gfe,OAD_NUM_FILES_PER_CB,G_PRIORITY_DEFAULT+2
			,NULL
			,enumerator_next_files_lcb,oad);
}

static void enumerate_children_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
	Topenadv_dir *oad = user_data;
	GError *error=NULL;
	DEBUG_MSG("enumerate_children_lcb, started for oad %p\n",oad);
	oad->gfe = g_file_enumerate_children_finish(oad->basedir,res,&error);
	if (error) {
		/*if (error->code == G_IO_ERROR_) {
		
		}*/
		g_print("BUG: enumerate_children_lcb, unhandled error: %d %s\n", error->code, error->message);
		g_error_free(error);
		open_adv_load_directory_cleanup(oad);
	} else {
		g_file_enumerator_next_files_async(oad->gfe,OAD_NUM_FILES_PER_CB,G_PRIORITY_DEFAULT+2
				,NULL,enumerator_next_files_lcb,oad);
	}
}

static void process_oadqueue(gpointer data) {
	Topenadv_dir *oad;
	if (oadqueue.todo == NULL) {
		return;
	}
	if (oadqueue.worknum > OAD_MAX_WORKNUM) { /* load max OAD_MAX_WORKNUM directories simultaneously */
		return;
	}
	while (oadqueue.todo!=NULL && oadqueue.worknum <= OAD_MAX_WORKNUM) {
		oad=oadqueue.todo->data;
		oadqueue.todo = g_list_delete_link(oadqueue.todo, oadqueue.todo);
		oadqueue.worknum++;
		g_file_enumerate_children_async(oad->basedir,BF_FILEINFO,0
					,G_PRIORITY_DEFAULT+3 
					,NULL
					,enumerate_children_lcb,oad);
	}
}

static void open_advanced_backend(Topenadv *oa, GFile *basedir) {
	Topenadv_dir *oad;
	DEBUG_MSG("open_advanced_backend on basedir %p ",basedir);
	DEBUG_URI(basedir,TRUE);
	oad = g_new0(Topenadv_dir, 1);
	oad->oa = oa;
	oa->refcount++;

	oad->basedir = basedir;
	g_object_ref(oad->basedir);
	oadqueue.todo = g_list_prepend(oadqueue.todo,oad);
	process_oadqueue(NULL);
}

void open_advanced(Tbfwin *bfwin, GFile *basedir, gboolean recursive, gboolean matchname, gchar *name_filter, gchar *content_filter, gboolean use_regex) {
	if (basedir) {
		Topenadv *oa;
		oa = g_new0(Topenadv, 1);
#ifdef LOAD_TIMER
		oa->timer = g_timer_new();
#endif
		oa->bfwin = bfwin;
		oa->recursive = recursive;
		oa->matchname = matchname;
		oa->topbasedir = basedir;
		g_object_ref(oa->topbasedir);
		if (name_filter) {
			oa->extension_filter = g_strdup(name_filter);
			oa->patspec = g_pattern_spec_new(name_filter);
		}
		if (content_filter) oa->content_filter = g_strdup(content_filter);
		oa->use_regex = use_regex;
		if (oa->use_regex) {
			const char *error;
			int erroffset;
			oa->contentf_pcre = pcre_compile(oa->content_filter, 0, &error, &erroffset, NULL);
			/* BUG: need error handling here */
			openadv_unref(oa);
		}
		open_advanced_backend(oa, basedir);
	}
}

/************************/
typedef struct {
	Tbfwin *bfwin;
	GSList *sourcelist;
	GFile *destdir;
	GFile *curfile, *curdest;
} Tcopyfile;

static gboolean copy_uris_process_queue(Tcopyfile *cf);
static void copy_async_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
	Tcopyfile *cf = user_data;
	gboolean done;
	GError *error=NULL;
	/* fill in the blanks */
	done = g_file_copy_finish(cf->curfile,res,&error);

	if (!done) {
		if (error->code == G_IO_ERROR_EXISTS) {
			const gchar *buttons[] = {_("_Skip"), _("_Overwrite"), NULL};
			gint retval;
			gchar *tmpstr, *dispname;
			dispname = gfile_display_name(cf->curfile,NULL);
			tmpstr = g_strdup_printf(_("%s cannot be copied, it already exists, overwrite?"),dispname);
			retval = message_dialog_new_multi(BFWIN(cf->bfwin)->main_window,
														 GTK_MESSAGE_WARNING,
														 buttons,
														 _("Overwrite file?"),
														 tmpstr);
			g_free(tmpstr);
			g_free(dispname);
			if (retval == 1) {
				g_file_copy_async(cf->curfile,cf->curdest,G_FILE_COPY_OVERWRITE,
					G_PRIORITY_LOW,NULL,
					NULL,NULL,
					copy_async_lcb,cf);
				return;
			}
		}
	}
	g_object_unref(cf->curfile);
	g_object_unref(cf->curdest);

	if (!copy_uris_process_queue(cf)) {
		fb2_refresh_dir_from_uri(cf->destdir);
		g_object_unref(cf->destdir);
		g_free(cf);
	}
}

static gboolean copy_uris_process_queue(Tcopyfile *cf) {
	if (cf->sourcelist) {
		GFile *uri, *dest;
		char *tmp;
		
		uri = cf->sourcelist->data;
		cf->sourcelist = g_slist_remove(cf->sourcelist, uri);
		
		tmp = g_file_get_basename(uri);
		dest = g_file_get_child(cf->destdir,tmp);
		g_free(tmp);
		
		cf->curfile = uri;
		cf->curdest = dest;
		
		g_file_copy_async(uri,dest,G_FILE_COPY_NONE,
				G_PRIORITY_LOW,NULL,
				NULL,NULL,
				copy_async_lcb,cf);
		
		return TRUE;
	}
	return FALSE;
}

void copy_uris_async(Tbfwin *bfwin, GFile *destdir, GSList *sources) {
	Tcopyfile *cf;
	GSList *tmplist;
	cf = g_new0(Tcopyfile,1);
	cf->bfwin = bfwin;
	cf->destdir = destdir;
	g_object_ref(cf->destdir);
	cf->sourcelist = g_slist_copy(sources);
	tmplist = cf->sourcelist;
	while (tmplist) {
		g_object_ref(tmplist->data);
		tmplist = tmplist->next;
	}
	copy_uris_process_queue(cf);
}


void copy_files_async(Tbfwin *bfwin, GFile *destdir, gchar *sources) {
	Tcopyfile *cf;
	gchar **splitted, **tmp;
	cf = g_new0(Tcopyfile,1);
	cf->bfwin = bfwin;
	cf->destdir = destdir;
	g_object_ref(cf->destdir);
	/* create the source and destlist ! */
	tmp = splitted = g_strsplit(sources, "\n",0);
	while (*tmp) {
		trunc_on_char(trunc_on_char(*tmp, '\r'), '\n');
		if (strlen(*tmp) > 1) {
			GFile *src;
			src = g_file_new_for_commandline_arg(*tmp);
			cf->sourcelist = g_slist_append(cf->sourcelist, src);
		}
		tmp++;
	}
	g_strfreev(splitted);
	copy_uris_process_queue(cf);
}
/* code for local to remote synchronisation */
typedef struct {
	guint refcount;
	GFile *basedir;
	GFile *targetdir;
	gboolean delete_deprecated;
	gboolean include_hidden;
	gint numworking;
	Tqueue queue_walkdir_local;
	Tqueue queue_walkdir_remote;
/*	Tqueue queue_delete;
	Tqueue queue_need_update;*/
	Tqueue queue_update;
	guint num_found;
	guint num_finished;
	/*GTimer *timer;*/
	SyncProgressCallback progress_callback;
	gpointer callback_data;
} Tsync;

static void sync_unref(Tsync *sync) {
	sync->refcount--;
	if (sync->refcount == 0) {
		g_object_unref(sync->basedir);
		g_object_unref(sync->targetdir);
		sync->progress_callback(-1,-1,sync->callback_data);
		/*g_timer_destroy(sync->timer);*/
		g_print("sync_unref, unreffing!\n");
		g_slice_free(Tsync,sync);
	}
}

static void walk_local_directory_run(gpointer data);
static void walk_directory_remote_run(gpointer data);
/*static void need_update_run(gpointer data);*/
static void do_update_run(gpointer data);
typedef struct {
	Tsync *sync;
	GFile *local_dir;
	/*GFileEnumerator *enmrt;*/
	GFile *remote_dir;
	GHashTable *localnames;
} Tsync_walkdir;

static void walk_directory_cleanup(Tsync_walkdir *swd) {
	sync_unref(swd->sync);
	g_object_unref(swd->remote_dir);
	g_object_unref(swd->local_dir);
	/*g_object_unref(swd->enmrt);*/
	g_hash_table_destroy(swd->localnames);
	g_slice_free(Tsync_walkdir,swd);
}
/*
typedef struct {
	Tsync *sync;
	GFile *local_uri;
	GFileInfo *local_finfo;
	GFile *remote_uri;
	gboolean is_dir; 
} Tsync_needupdate;

static void need_update_cleanup(Tsync_needupdate *snu) {
	sync_unref(snu->sync);
	g_object_unref(snu->local_uri);
	g_object_unref(snu->local_finfo);
	g_object_unref(snu->remote_uri);
	g_slice_free(Tsync_needupdate,snu);
}*/

typedef struct {
	Tsync *sync;
	GFile *local_uri;
	GFile *remote_uri;
} Tsync_update;

static void update_cleanup(Tsync_update *su) {
	sync_unref(su->sync);
	g_object_unref(su->local_uri);
	g_object_unref(su->remote_uri);
	g_slice_free(Tsync_update,su);	
}

static void walk_local_directory(Tsync *sync, GFile *local_dir, GFile *remote_dir);

GFile *remote_for_local(Tsync *sync,GFile *local) {
	gchar *lcuri,*base,*target,*remote;
	GFile *uri;
	lcuri = g_file_get_uri(local);
	base = g_file_get_uri(sync->basedir);
	target = g_file_get_uri(sync->targetdir);
	remote = g_strconcat(target,lcuri+strlen(base),NULL); 
	/*g_print("remote_for_local,base=%s,target=%s,lcuri=%s,remote=%s\n",base,target,lcuri,remote);*/
	uri = g_file_new_for_uri(remote);
	g_free(lcuri);
	g_free(base);
	g_free(target);
	g_free(remote);
	return uri;
}

static void progress_update(Tsync *sync) {
/*	if (g_timer_elapsed(sync->timer,NULL) > 0.05) */
	if (sync->num_found % 10==0 || (sync->num_found - sync->num_finished) % 10 == 0) 
	{
		sync->progress_callback(sync->num_found,sync->num_finished,sync->callback_data);
/*		g_print("timer elapsed %f\n",g_timer_elapsed(sync->timer,NULL));
		g_timer_start(sync->timer);*/
	}
}

/*
TODO
static void remote_cleanup(Tsync *sync) {
	g_file_enumerate_children_async(dir,
                                                         const char *attributes,
                                                         GFileQueryInfoFlags flags,
                                                         int io_priority,
                                                         GCancellable *cancellable,
                                                         GAsyncReadyCallback callback,
                                                         gpointer user_data);

}*/

static void do_update_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
	Tsync_update *su = user_data;
	gboolean ret;
	GError *error=NULL;
	ret = g_file_copy_finish(su->local_uri,res,&error);
	if (error) {
		g_print("do_update_lcb got error %d %s\n",error->code,error->message);
		g_error_free(error);
	}
	/*
	TODO: remove all files that are locally not present ???
	if () {
		remote_cleanup();
	}*/
	su->sync->num_finished++;
	progress_update(su->sync);
	/*g_print("%d%%: %d found, %d finished\n",(gint)(100.0*su->sync->num_finished/su->sync->num_found), su->sync->num_found,su->sync->num_finished);*/
	queue_worker_ready(&su->sync->queue_update,do_update_run);
	update_cleanup(su);
}

static void do_update_run(gpointer data) {
	Tsync_update *su = data;
	g_file_copy_async(su->local_uri,su->remote_uri,G_FILE_COPY_OVERWRITE,G_PRIORITY_LOW,NULL
					,NULL,NULL,do_update_lcb,su);	 
}

static void do_update(Tsync *sync, GFile *local_uri, GFile *remote_uri) {
	Tsync_update *su = g_slice_new0(Tsync_update);
	su->sync = sync;
	su->sync->refcount++;
	su->local_uri = local_uri;
	g_object_ref(su->local_uri);
	su->remote_uri = remote_uri;
	g_object_ref(su->remote_uri);
	queue_push(&sync->queue_update,su,do_update_run);
}
/*
static void check_update_need_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
	Tsync_needupdate *snu= user_data;
	GFileInfo *remote_finfo;
	GError *error=NULL;
	remote_finfo = g_file_query_info_finish(snu->remote_uri,res,&error);
	if (error) {
		if (error->code == G_IO_ERROR_NOT_FOUND) {
			g_error_free(error);
			if (snu->is_dir) {
				error=NULL;
				if (g_file_make_directory(snu->remote_uri,NULL,&error)) {
					walk_local_directory(snu->sync, snu->local_uri, snu->remote_uri);
				} else if (error) {
					g_print("check_update_need_lcb, while creating remote dir got error %d %s\n",error->code,error->message);
					g_error_free(error);
				}
			} else {
				do_update(snu->sync,snu->local_uri,snu->remote_uri);
			}
		} else {
			g_print("check_update_need_lcb got error %d %s\n",error->code,error->message);
			g_error_free(error);
		}
	} else if(remote_finfo) {
		if (snu->is_dir) {
			walk_local_directory(snu->sync, snu->local_uri, snu->remote_uri);
		} else {
			GTimeVal remote_mtime,local_mtime;
			g_file_info_get_modification_time(remote_finfo,&remote_mtime);
			g_file_info_get_modification_time(snu->local_finfo,&local_mtime);
			if (g_file_info_get_size(remote_finfo)!=g_file_info_get_size(snu->local_finfo)
						|| remote_mtime.tv_sec+remote_mtime.tv_usec <  local_mtime.tv_sec+local_mtime.tv_usec) {
				do_update(snu->sync,snu->local_uri,snu->remote_uri);
			} else {
				snu->sync->num_finished++;
				progress_update(snu->sync);
			}
		}
		g_object_unref(remote_finfo);
	} else {
		g_print("weird, no error and no remote finfo??? snu=%p\n",snu);
	}
	queue_worker_ready(&snu->sync->queue_need_update,need_update_run);
	need_update_cleanup(snu);
}

static void need_update_run(gpointer data) {
	Tsync_needupdate *snu=data;
	g_file_query_info_async(snu->remote_uri,"standard::name,standard::type,standard::display-name,standard::size,time::modified",
					G_FILE_QUERY_INFO_NONE,G_PRIORITY_LOW,NULL,check_update_need_lcb,snu);
}

static void check_update_need(Tsync *sync, GFile *uri,GFileInfo *finfo, gboolean is_dir) {
	Tsync_needupdate *snu = g_slice_new0(Tsync_needupdate);
	snu->sync = sync;
	snu->is_dir = is_dir;
	snu->sync->refcount++;
	snu->local_uri = uri;
	g_object_ref(snu->local_uri);
	snu->local_finfo = finfo;
	g_object_ref(snu->local_finfo);
	snu->remote_uri = remote_for_local(snu->sync,snu->local_uri);
	queue_push(&sync->queue_need_update,snu,need_update_run);
}*/

/*static void walk_directory_remote_close_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
	GError *error=NULL;
	Tsync_walkdir *swd = user_data;
	
	g_file_enumerator_close_finish(swd->renmrt,res,&error);
	if (error) {
		g_print("walk_local_directory_close_lcb, error %d=%s\n",error->code,error->message);
		g_error_free(error);
	}
	queue_worker_ready(&swd->sync->queue_walkdir_remote, walk_directory_remote_run);
	walk_directory_cleanup(swd);
}

static void walk_directory_remote_enumerate_next_files_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
	Tsync_walkdir *swd = user_data;
	GList *list;
	GError *error=NULL;
	list = g_file_enumerator_next_files_finish(swd->renmrt,res,&error);
	if (error) {
		g_print("walk_directory_remote_enumerate_next_files_lcb, got error %d: %s\n", error->code,error->message);
		g_error_free(error);
		g_file_enumerator_close_async(swd->renmrt, G_PRIORITY_LOW, NULL,walk_directory_remote_close_lcb, swd);
	} else {
		if (list) {
			GList *tmplist = g_list_first(list);
			while (tmplist) {
				GFileInfo *finfo = tmplist->data;
				const gchar *name;
				name = g_file_info_get_name(finfo);
				if (g_hash_table_lookup(swd->localnames, name)==NULL) { / * . and .. are in the hash * /
					GFile *uri;
					uri = g_file_get_child(swd->remote_dir, name);
					file_delete_async(uri, TRUE, NULL, NULL);
					g_object_unref(uri);
				}
				g_object_unref(finfo);
				tmplist = g_list_next(tmplist);
			}
			g_list_free(list);
			g_file_enumerator_next_files_async(swd->renmrt,64,G_PRIORITY_LOW,NULL,walk_directory_remote_enumerate_next_files_lcb,swd);
		} else {
			g_file_enumerator_close_async(swd->renmrt, G_PRIORITY_LOW, NULL,walk_directory_remote_close_lcb, swd);
		}
	}
}

static void walk_directory_remote_enumerate_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
	GError *error=NULL;
	Tsync_walkdir *swd = user_data;
	swd->renmrt = g_file_enumerate_children_finish(swd->remote_dir,res,&error);
	if (error) {
		g_print("walk_local_directory_remote_enumerate_lcb, error %d=%s\n",error->code,error->message);
		g_error_free(error);
	} else {
		g_file_enumerator_next_files_async(swd->renmrt,64,G_PRIORITY_LOW,NULL,walk_directory_remote_enumerate_next_files_lcb,swd);
	}	
}*/

static gboolean walk_directory_remote_job(GIOSchedulerJob *job,GCancellable *cancellable,gpointer user_data) {
	Tsync_walkdir *swd = user_data;
	GFileEnumerator *enumer;
	GError *error=NULL;
	enumer = g_file_enumerate_children(swd->remote_dir,"standard::name,standard::type,standard::display-name,standard::size,time::modified"
							,G_FILE_QUERY_INFO_NONE,NULL,&error);
	if (error) {
		g_print("walk_directory_remote_job, enumerate_children, error %d=%s\n",error->code,error->message);
		g_error_free(error);
	} else if (enumer) {
		gboolean cont=TRUE;
		while (cont) {
			GFileInfo *finfo;
			finfo = g_file_enumerator_next_file(enumer,NULL,&error);
			if (error) {
				g_print("walk_directory_remote_job, enumerate_next_file, error %d=%s\n",error->code,error->message);
				g_error_free(error);
				error=NULL;
			} else if (finfo) {
				const gchar *name = g_file_info_get_name(finfo);
				if (g_hash_table_lookup(swd->localnames, name)==NULL) { /* . and .. are in the hash */
					GFile *uri;
					uri = g_file_get_child(swd->remote_dir, name);
					if (g_file_info_get_file_type(finfo) == G_FILE_TYPE_DIRECTORY) {
						delete_recursive(uri);
					}
					g_file_delete(uri,NULL,&error);
					if (error) {
						g_print("walk_directory_remote_job, delete, error %d=%s\n",error->code,error->message);
						g_error_free(error);
						error=NULL;
					}
					g_object_unref(uri);
				}
				g_object_unref(finfo);
			} else {
				cont = FALSE;
			}
		}
		g_file_enumerator_close(enumer,NULL,&error);
		if (error) {
			g_print("walk_directory_remote_job, close_enumerator, error %d=%s\n",error->code,error->message);
			g_error_free(error);
		}
		g_object_unref(enumer);
	}
	queue_worker_ready(&swd->sync->queue_walkdir_remote, walk_directory_remote_run);
	walk_directory_cleanup(swd);
	return FALSE;
}


static void walk_directory_remote_run(gpointer data) {
	Tsync_walkdir *swd = data;
	g_io_scheduler_push_job(walk_directory_remote_job,swd,NULL,G_PRIORITY_LOW,NULL);
/*	g_file_enumerate_children_async(swd->remote_dir,"standard::name,standard::type,standard::display-name,standard::size,time::modified",
					G_FILE_QUERY_INFO_NONE,G_PRIORITY_LOW,NULL,walk_directory_remote_enumerate_lcb,swd);*/
}

/*static void walk_local_directory_close_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
	GError *error=NULL;
	Tsync_walkdir *swd = user_data;
	
	g_file_enumerator_close_finish(swd->enmrt,res,&error);
	if (error) {
		g_print("walk_local_directory_close_lcb, error %d=%s\n",error->code,error->message);
		g_error_free(error);
	}
	queue_worker_ready(&swd->sync->queue_walkdir_local,walk_local_directory_run);
	if (swd->sync->delete_deprecated) {
		queue_push(&swd->sync->queue_walkdir_remote,swd,walk_directory_remote_run);
	} else {
		walk_directory_cleanup(swd);
	}
}

static void walk_local_directory_enumerate_next_files_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
	Tsync_walkdir *swd = user_data;
	GList *list;
	GError *error=NULL;
	list = g_file_enumerator_next_files_finish(swd->enmrt,res,&error);
	if (error) {
		g_print("walk_local_directory_enumerate_next_files_lcb, got error %d: %s\n", error->code,error->message);
		g_error_free(error);
		g_file_enumerator_close_async(swd->enmrt, G_PRIORITY_LOW, NULL,walk_local_directory_close_lcb, swd);
	} else {
		if (list) {
			GList *tmplist = g_list_first(list);
			while (tmplist) {
				GFileInfo *finfo = tmplist->data;
				const gchar *name;
				GFileType ft = g_file_info_get_file_type(finfo);
				name = g_file_info_get_name(finfo);
				if (strcmp(name,"..")!=0 && strcmp(name,".")!=0) {
					GFile *uri;
					uri = g_file_get_child(swd->local_dir, name);
					if (ft == G_FILE_TYPE_DIRECTORY) {
						check_update_need(swd->sync,uri,finfo, TRUE);
					} else if (ft == G_FILE_TYPE_REGULAR) {
						check_update_need(swd->sync,uri,finfo, FALSE);
						swd->sync->num_found++;
						progress_update(swd->sync);
					}
					g_object_unref(uri);
				}
				g_hash_table_insert(swd->localnames, g_strdup(name),GINT_TO_POINTER(1));
				g_object_unref(finfo);
				tmplist = g_list_next(tmplist);
			}
			g_list_free(list);
			g_file_enumerator_next_files_async(swd->enmrt,64,G_PRIORITY_LOW,NULL,walk_local_directory_enumerate_next_files_lcb,swd);
		} else {
			/ * finished * /
			g_file_enumerator_close_async(swd->enmrt, G_PRIORITY_LOW, NULL,walk_local_directory_close_lcb, swd);
		}
	}
}

static void walk_local_directory_enumerate_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
	GError *error=NULL;
	Tsync_walkdir *swd = user_data;
	swd->enmrt = g_file_enumerate_children_finish(swd->local_dir,res,&error);
	if (error) {
		g_print("walk_local_directory_enumerate_lcb, error %d=%s\n",error->code,error->message);
		g_error_free(error);
	} else {
		g_file_enumerator_next_files_async(swd->enmrt,64,G_PRIORITY_LOW,NULL,walk_local_directory_enumerate_next_files_lcb,swd);
	}
}*/

static gboolean walk_local_directory_job(GIOSchedulerJob *job,GCancellable *cancellable,gpointer user_data) {
	Tsync_walkdir *swd = user_data;
	GFileEnumerator *enumer;
	GError *error=NULL;
	enumer = g_file_enumerate_children(swd->local_dir,"standard::name,standard::type,standard::display-name,standard::size,time::modified"
							,G_FILE_QUERY_INFO_NONE,NULL,&error);
	if (error) {
		g_print("walk_local_directory_job, enumerate_children, error %d=%s\n",error->code,error->message);
		g_error_free(error);
	} else if (enumer) {
		gboolean cont=TRUE;
		while (cont) {
			GFileInfo *finfo;
			finfo = g_file_enumerator_next_file(enumer,NULL,&error);
			if (error) {
				g_print("walk_local_directory_job, enumerate_next_file, error %d=%s\n",error->code,error->message);
				g_error_free(error);
				error=NULL;
			} else if (finfo) {
				const gchar *name = g_file_info_get_name(finfo);
				if (strcmp(name,"..")!=0 && strcmp(name,".")!=0) {
					GFile *local, *remote;
					GFileInfo *rfinfo;
					local = g_file_get_child(swd->local_dir, name);
					remote = g_file_get_child(swd->remote_dir, name);
					rfinfo = g_file_query_info(remote,"standard::name,standard::type,standard::display-name,standard::size,time::modified"
								,G_FILE_QUERY_INFO_NONE,NULL,&error);
					if (error) {
						if (error->code == G_IO_ERROR_NOT_FOUND) { /* file/dir does not exist */
							g_error_free(error);
							error=NULL;
							if (g_file_info_get_file_type(finfo) == G_FILE_TYPE_DIRECTORY) {
								if (g_file_make_directory(remote,NULL,&error)) {
									walk_local_directory(swd->sync, local, remote);
								} else if (error) {
									g_print("walk_local_directory_job, while creating remote dir got error %d %s\n",error->code,error->message);
									g_error_free(error);
									error = NULL;
								}
							} else {
								do_update(swd->sync,local,remote);
							}
						} else {
							g_print("walk_local_directory_run_job, query remote, got error %d: %s\n", error->code,error->message);
							g_error_free(error);
							error=NULL;
						}
					} else if (rfinfo) {
						if (g_file_info_get_file_type(finfo) ==  G_FILE_TYPE_DIRECTORY) {
							if (g_file_info_get_file_type(rfinfo) != G_FILE_TYPE_DIRECTORY) {
								g_file_delete(remote,NULL,&error);
								if (error) {
									g_print("walk_local_directory_run_job, delete, got error %d: %s\n", error->code,error->message);
									g_error_free(error);
									error=NULL;
								}
							}
							walk_local_directory(swd->sync, local, remote);
						} else if (g_file_info_get_file_type(finfo) ==  G_FILE_TYPE_REGULAR) {
							if (g_file_info_get_file_type(rfinfo) == G_FILE_TYPE_DIRECTORY) {
								delete_recursive(remote);
							}
							GTimeVal remote_mtime,local_mtime;
							g_file_info_get_modification_time(rfinfo,&remote_mtime);
							g_file_info_get_modification_time(finfo,&local_mtime);
							if (g_file_info_get_size(rfinfo)!=g_file_info_get_size(finfo)
										|| remote_mtime.tv_sec+remote_mtime.tv_usec <  local_mtime.tv_sec+local_mtime.tv_usec) {
								do_update(swd->sync,local,remote);
							} else {
								swd->sync->num_finished++;
								/*progress_update(snu->sync);*/
							}
						}
						g_object_unref(rfinfo);
					}
					swd->sync->num_found++;
					/*progress_update(swd->sync);*/
					g_object_unref(local);
					g_object_unref(remote);
				}
				if (swd->sync->delete_deprecated) {
					g_hash_table_insert(swd->localnames, g_strdup(name),GINT_TO_POINTER(1));
				}
				g_object_unref(finfo);
			} else {
				cont = FALSE;
			}
		}
		g_file_enumerator_close(enumer,NULL,&error);
		if (error) {
			g_print("walk_local_directory_job, close_enumerator, error %d=%s\n",error->code,error->message);
			g_error_free(error);
		}
		g_object_unref(enumer);
	}
	queue_worker_ready(&swd->sync->queue_walkdir_local, walk_local_directory_run);
	if (swd->sync->delete_deprecated) {
		queue_push(&swd->sync->queue_walkdir_remote,swd,walk_directory_remote_run);
	} else {
		walk_directory_cleanup(swd);
	}
	return FALSE;
}

static void walk_local_directory_run(gpointer data) {
	Tsync_walkdir *swd = data;
	g_io_scheduler_push_job(walk_local_directory_job,swd,NULL,G_PRIORITY_LOW,NULL);
	/*g_file_enumerate_children_async(swd->local_dir,"standard::name,standard::type,standard::display-name,standard::size,time::modified",
					G_FILE_QUERY_INFO_NONE,G_PRIORITY_LOW,NULL,walk_local_directory_enumerate_lcb,swd);*/
}

static void walk_local_directory(Tsync *sync, GFile *local_dir, GFile *remote_dir) {
	Tsync_walkdir *swd = g_slice_new0(Tsync_walkdir);
	swd->sync = sync;
	swd->sync->refcount++;
	swd->local_dir = local_dir;
	g_object_ref(swd->local_dir);
	swd->remote_dir = remote_dir;
	g_object_ref(swd->remote_dir);
	swd->localnames = g_hash_table_new_full(g_str_hash, g_str_equal,g_free,NULL);
	queue_push(&sync->queue_walkdir_local,swd,walk_local_directory_run);
}

static void sync_directory_mount_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
	Tsync *sync = user_data;
	GError *error=NULL;
	g_file_mount_enclosing_volume_finish(sync->targetdir,res,&error);
	if (error) {
		if (error->code == G_IO_ERROR_ALREADY_MOUNTED || error->code == G_IO_ERROR_NOT_SUPPORTED) {
			walk_local_directory(sync, sync->basedir, sync->targetdir);
		} else {
			g_print("sync_directory_mount_lcb, error %d=%s\n",error->code,error->message);
		}
		g_error_free(error);
	} else {
		walk_local_directory(sync, sync->basedir,sync->targetdir);
	}
	sync_unref(sync);
}

void sync_directory(GFile *basedir, GFile *targetdir, gboolean delete_deprecated, gboolean include_hidden, SyncProgressCallback progress_callback, gpointer callback_data) {
	GMountOperation * gmo;
	Tsync *sync = g_slice_new0(Tsync);
	sync->refcount++;
	sync->delete_deprecated = delete_deprecated;
	sync->include_hidden = include_hidden;
	sync->progress_callback = progress_callback;
	sync->callback_data = callback_data;
	queue_init(&sync->queue_walkdir_local,3);
	queue_init(&sync->queue_walkdir_remote,2);
/*	queue_init(&sync->queue_delete,7);
	queue_init(&sync->queue_need_update,8);*/
	queue_init(&sync->queue_update,4);
	/*sync->timer = g_timer_new();*/
	sync->basedir = basedir;
	g_object_ref(sync->basedir);
	sync->targetdir = targetdir;
	g_object_ref(sync->targetdir);
	
	gmo = gtk_mount_operation_new(NULL);
	g_file_mount_enclosing_volume(sync->targetdir,G_MOUNT_MOUNT_NONE,gmo,NULL,sync_directory_mount_lcb,sync);
}

/* code to handle a file from the commandline, the filebrowser or from the message queue */

void file_handle(GFile *uri, Tbfwin *bfwin) {
	GFileInfo *finfo;
	GError *error=NULL;
	const gchar *mime;
	finfo = g_file_query_info(uri,"standard::fast-content-type",G_FILE_QUERY_INFO_NONE,NULL,&error);
	if (error) {
		g_print("file_handle got error %d: %s\n",error->code,error->message);
		g_error_free(error);
		return;
	}
	mime = g_file_info_get_attribute_string(finfo, G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE);
	if (strcmp(mime, "application/x-bluefish-project")==0) {
		project_open_from_file(bfwin, uri);
	} else if (strncmp(mime, "image",5)==0) {
		/* do something with the image */
		g_print("file-handle, handling images is not yet implemented\n");
	} else {
		doc_new_from_uri(bfwin, uri, NULL, FALSE, FALSE, -1, -1);
	}
}
