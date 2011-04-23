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

#define _GNU_SOURCE
#include <string.h>
#include "config.h"
#include "bluefish.h"
#include "document.h"
#include "file.h"
#include "outputbox.h"
#include "snr3.h"
#include "snr3_files.h"
#include "async_queue.h"

/*#ifndef HAVE_STRCASESTR
char *strcasestr(char *a, char *b)
{
	size_t l;
	char f[3];

	snprintf(f, sizeof(f), "%c%c", tolower(*b), toupper(*b));
	for (l = strcspn(a, f); l != strlen(a); l += strcspn(a + l + 1, f) + 1)
		if (strncasecmp(a + l, b, strlen(b)) == 0)
			return(a + l);
	return(NULL);
}
#endif*/


typedef struct {
	Tasyncqueue queue;
	guint refcount;
	Tsnr3run *s3run;
	gpointer querydata;
} Tfilesworker;

typedef struct {
	gsize pos;
	guint line;
} Tlineinbuffer;

static guint calculate_line_in_buffer(Tlineinbuffer *lib, gchar *buffer, gsize pos) {
	char *tmp = buffer + lib->pos;
	char *newpos = buffer + pos;
	guint line = lib->line;
	gboolean prev_was_cr=FALSE;
	
	while (*tmp != '\0' && tmp != newpos) {
		if (*tmp == '\n') {
			if (!prev_was_cr)
				line++;
		} else if (*tmp == '\r') {
			line++;
			prev_was_cr=TRUE;
		} else {
			prev_was_cr=FALSE;
		}
		tmp++;
	}
	lib->pos = pos;
	lib->line = line;
	return line;
}

static GList *snr3_replace_pcre(Tfilesworker *fw, gchar *buffer, gchar **replacedbuffer) {
	gchar *newbuf;
	gchar *bufferpos, *newbufpos;
	gsize buflen;
	gsize alloced;
	Tlineinbuffer lib = {0,1};
	GList *results=NULL;
	GMatchInfo *match_info;
	gsize prevpos=0;
	
	DEBUG_MSG("snr3_replace_pcre, search for %s in %s, replace with %s\n",s3run->query, buffer, s3run->replace);
	buflen = strlen(buffer);
	
	alloced = MAX(buflen*2,4096);
	newbuf = g_malloc0(alloced);

	bufferpos = buffer;
	newbufpos = newbuf;
	g_regex_match(fw->querydata, buffer, 0, &match_info);
	while(g_match_info_matches(match_info)) {
		gint so, eo;
		guint line, replacelen;
		gchar *replacestring;
		GError *gerror=NULL;
		g_match_info_fetch_pos(match_info,0,&so,&eo);

		memcpy(newbufpos, bufferpos, so-prevpos);
		newbufpos += (so-prevpos);
		bufferpos = buffer+eo;
		prevpos = eo;
		
		line = calculate_line_in_buffer(&lib, newbuf, (newbufpos-newbuf));
		results = g_list_prepend(results, GINT_TO_POINTER(line));
		
		replacestring = g_match_info_expand_references(match_info, fw->s3run->replace, &gerror);
		if (gerror) {
			g_print("replace error %s\n",gerror->message);
			g_error_free(gerror);
		}
		replacelen = strlen(replacestring);	
		
		/* now check if we have enough memory */
		if (alloced < (1+ newbufpos-newbuf + replacelen + buflen-prevpos)) {
			gchar *tmp;
			alloced += MAX(buflen, 4096);
			tmp = g_realloc(newbuf, alloced);
			newbufpos = tmp + (newbufpos-newbuf);
			newbuf=tmp;
		}
			
		memcpy(newbufpos, replacestring, replacelen);
		newbufpos += replacelen;
		g_free(replacestring);
		
		g_match_info_next(match_info, NULL);
	}
	g_match_info_free(match_info);
	
	memcpy(newbufpos, buffer+prevpos, buflen-prevpos+1);
	*replacedbuffer = newbuf;
	return results; 
}

static GList *snr3_replace_string(Tfilesworker *fw, gchar *buffer, gchar **replacedbuffer) {
	gchar *result, *newbuf;
	gchar *bufferpos, *newbufpos;
	gsize querylen, replacelen, buflen;
	gsize alloced;
	Tlineinbuffer lib = {0,1};
	GList *results=NULL;
	char *(*f) ();
	
	DEBUG_MSG("snr3_replace_string, search for %s in %s, replace with %s\n",fw->s3run->query, buffer, fw->s3run->replace);
	querylen = strlen(fw->s3run->query);
	replacelen = strlen(fw->s3run->replace);
	buflen = strlen(buffer);
	
	
	alloced = (replacelen > querylen)? MAX(1+replacelen+buflen*2,4096):MAX(buflen+querylen+1, 4096);
	newbuf = g_malloc0(alloced);

	bufferpos = buffer;
	newbufpos = newbuf;
	
	if (fw->s3run->is_case_sens) {
		f = strstr;
	} else {
		f = strcasestr;
	}
	
	result = buffer;
	
	do {
		result = f(result, fw->s3run->query);
		if (result) {
			guint line;
			
			memcpy(newbufpos, bufferpos, result-bufferpos);
			newbufpos += (result-bufferpos);
			
			line = calculate_line_in_buffer(&lib, newbuf, (newbufpos-newbuf));

			memcpy(newbufpos, fw->s3run->replace, replacelen);
			newbufpos += replacelen;
			result += querylen;
			bufferpos = result;
			
			results = g_list_prepend(results, GINT_TO_POINTER(line));
			
			if (alloced <= (1+replacelen+(newbufpos-newbuf)+replacelen+(buflen-(bufferpos-buffer)))) {
				gchar *tmp;
				alloced += MAX(buflen, 4096);
				tmp = g_realloc(newbuf, alloced);
				newbufpos = tmp + (newbufpos-newbuf);
				newbuf=tmp;
			}
		}		
	} while (result);
	
	memcpy(newbufpos, bufferpos, strlen(bufferpos)+1);
	*replacedbuffer = newbuf;
	return results; 
}

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
		if (s3run->type == snr3type_pcre) {
			g_regex_unref((GRegex *)fw->querydata);
		}
		
		g_print("filesworker_unref, free %p\n",fw);
		g_slice_free(Tfilesworker, fw);
		
	}
}

static gboolean replace_files_in_thread_finished(gpointer data) {
	Treplaceinthread *rit = data;
	gchar *curi;
	GList *tmplist;
	
	g_print("add %d results to outputbox\n",g_list_length(rit->results));
	
	curi = g_file_get_uri(rit->uri);
	for (tmplist=g_list_first(rit->results);tmplist;tmplist=g_list_next(tmplist)) {
		outputbox_add_line(rit->fw->s3run->bfwin, curi, GPOINTER_TO_INT(tmplist->data), rit->fw->s3run->query);
	}
	g_free(curi);
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
	} else {
		g_print("thread %p: calling buffer_find_encoding for %ld bytes\n", g_thread_self(),(glong)strlen(inbuf));
		/* is the following function thread safe ?? */
		utf8buf = buffer_find_encoding(inbuf, inbuflen, &encoding, rit->fw->s3run->bfwin->session->encoding);
		g_free(inbuf);
		
		if (utf8buf) {
			gchar *replacedbuf=NULL;
			
			switch (rit->fw->s3run->type) {
				case snr3type_string:
					rit->results = snr3_replace_string(rit->fw, utf8buf, &replacedbuf);
				break;
				case snr3type_pcre:
					rit->results = snr3_replace_pcre(rit->fw, utf8buf, &replacedbuf);
				break;
			}
			g_free(utf8buf);
			if (rit->results && replacedbuf) {
				g_print("replaced %d entries\n",g_list_length(rit->results));
				outbuf = g_convert(replacedbuf, -1, encoding, "UTF-8", NULL, &outbuflen, NULL);
				
			
				ret = g_file_replace_contents(rit->uri,outbuf,outbuflen,NULL,TRUE,G_FILE_CREATE_NONE,NULL,NULL,&gerror);
				if (gerror) {
					g_print("failed to save file: %s\n",gerror->message);
					g_error_free(gerror);
				}
				g_free(outbuf);
			}
			g_free(replacedbuf);
			
		}
	}
	rit->results = g_list_reverse(rit->results);
	tmpqueue = &rit->fw->queue;
	g_idle_add(replace_files_in_thread_finished, rit);
	
	g_print("thread %p: calling queue_worker_ready_inthread\n",g_thread_self());
	queue_worker_ready_inthread(tmpqueue);
	/* we don't need to start a new thread by calling _inthread() inside the thread */
	return NULL;
}

static void finished_finding_files_cb(Tfilesworker *fw) {
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
	queue_init_full(&fw->queue, 4, TRUE, TRUE, (QueueFunc)files_replace_run);
	fw->s3run=s3run;
	fw->refcount=1;
	if (s3run->type == snr3type_pcre) {
		GError *gerror=NULL;
		fw->querydata = g_regex_new(s3run->query,G_REGEX_MULTILINE, 0, &gerror);
		if (gerror) {
			g_print("regex compile error %s\n",gerror->message);
			g_error_free(gerror);
			/* TODO: bail out? */
		}
		
	}
	
	basedir = g_file_new_for_path("/tmp/");
	findfiles(basedir, TRUE, 1, TRUE,"*.txt", G_CALLBACK(filematch_cb), G_CALLBACK(finished_finding_files_cb), fw);
	g_object_unref(basedir);
	outputbox(s3run->bfwin,NULL, 0, 0, 0, NULL);
}
