/* Bluefish HTML Editor
 * snr3_files.c - multi-threaded search and replace in files
 *
 * Copyright (C) 2011 Olivier Sessink
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
 *
 */

/*#define DEBUG*/

#include "config.h"
#include "bluefish.h"
#include "document.h"

#include "snr3.h"
#include "snr3_files.h"
#include "async_queue.h"


typedef struct {
	Tasyncqueue queue;
	guint refcount;
	Tsnr3run *s3run;
} Tfilesworker;

typedef struct {
	GFile *uri;
	GList *results;
	Tfilesworker *fw;
} Treplaceinthread;

static void filesworker_unref(Tfilesworker *fw) {
	fw->refcount--;
	g_print("filesworker_unref, fw=%p, refcount=%d\n",fw,fw->refcount);
	if (fw->refcount == 0) {
		Tsnr3run *s3run = fw->s3run;
		
		queue_cleanup(&fw->queue);
		
		g_print("filesworker_unref, free %p\n",fw);
		g_slice_free(Tfilesworker, fw);
		
	}
	
}

static gboolean replace_files_in_thread_finished(gpointer data) {
	Treplaceinthread *rit = data;
	/* TODO: add the results to the outputbox */
	
	g_print("add %d results to outputbox\n",g_list_length(rit->results));
	
	g_print("replace_files_in_thread_finished, finished rit %p\n", rit);
	filesworker_unref(rit->fw);
	/* cleanup */
	g_object_unref(rit->uri);
	g_list_free(rit->results);
	g_slice_free(Treplaceinthread, rit);
	return FALSE;
}

static gpointer files_replace_run(gpointer data) {
	Treplaceinthread *rit = data;
	GError *gerror=NULL;
	gchar *inbuf=NULL, *encoding=NULL, *outbuf, *utf8buf;
	gsize inbuflen=0, outbuflen=0;
	gboolean ret;
	Tasyncqueue *tmpqueue;
	
	g_print("thread %p: files_replace_run, started rit %p\n", g_thread_self(), rit);
	
	ret = g_file_load_contents(rit->uri,NULL,&inbuf,&inbuflen,NULL,&gerror);
	if (gerror) {
		g_print("failed to load file: %s\n",gerror->message);
		g_error_free(gerror);
		tmpqueue = &rit->fw->queue;
		g_idle_add(replace_files_in_thread_finished, rit);
		queue_worker_ready_inthread(tmpqueue);
		return NULL;
	}
	
	g_print("thread %p: calling buffer_find_encoding for %ld bytes\n", g_thread_self(),(glong)strlen(inbuf));
	/* is the following function thread safe ?? */
	utf8buf = buffer_find_encoding(inbuf, inbuflen, &encoding, rit->fw->s3run->bfwin->session->encoding);
	
	g_free(inbuf);
	
	
	/* TODO: do the replace */
	g_print("thread %p: TODO: do the actual replace on buf %p on file %s\n", g_thread_self(),utf8buf, g_file_get_uri(rit->uri)); /* memleak in this debug statement */

	outbuf = g_convert(utf8buf, -1, encoding, "UTF-8", NULL, &outbuflen, NULL);
	g_free(utf8buf);
	
	ret = g_file_replace_contents(rit->uri,outbuf,outbuflen,NULL,TRUE,G_FILE_CREATE_NONE,NULL,NULL,&gerror);
	if (gerror) {
		g_print("failed to save file: %s\n",gerror->message);
		g_error_free(gerror);
		tmpqueue = &rit->fw->queue;
		g_idle_add(replace_files_in_thread_finished, rit);
		queue_worker_ready_inthread(tmpqueue);
		return NULL;
	}
	
	g_free(outbuf);
	
	tmpqueue = &rit->fw->queue;
	g_idle_add(replace_files_in_thread_finished, rit);
	
	g_print("thread %p: calling queue_worker_ready_inthread\n",g_thread_self());
	queue_worker_ready_inthread(tmpqueue);
	/* we don't need to start a new thread by calling _inthread() inside the thread */
	return NULL;
}

static void finished_cb(Tfilesworker *fw) {
	g_print("finished_cb, fw refcount (before unref) = %d\n",fw->refcount);
	
	filesworker_unref(fw);
}

static void filematch_cb(Tfilesworker *fw, GFile *uri, GFileInfo *finfo) {
	Treplaceinthread *rit;
	
	rit = g_slice_new0(Treplaceinthread);
	rit->uri = uri;
	g_object_ref(rit->uri);
	rit->fw = fw;
	rit->fw->refcount++;
	g_print("filematch_cb, push rit %p to queue, fw refcount is %d\n", rit, rit->fw->refcount);
	/* first check if we have this file open, in that case we have to run the 
	function that replaces in the document */
	/*file_openfile_uri_async(uri, s3run->bfwin, pcre_filematch_openfile_cb, s3run);*/
	queue_push(&fw->queue, rit);
}

void snr3_run_in_files(Tsnr3run *s3run, GCallback callback) {
	GFile *basedir;
	Tfilesworker *fw;
	
	fw = g_slice_new0(Tfilesworker);
	queue_init_full(&fw->queue, 4, TRUE, TRUE, files_replace_run);
	fw->s3run=s3run;
	fw->refcount=1;
	
	basedir = g_file_new_for_path("/tmp/");
	findfiles(basedir, TRUE, 1, TRUE,"*.txt", filematch_cb, finished_cb, fw);
	g_object_unref(basedir);
}
