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
#include "bf_lib.h"

typedef struct {
	gint line;
	gchar *text;
} Tfileresult;

static guint calculate_line_in_buffer(Tlineinbuffer *lib, gchar *buffer, gsize pos) {
	char *tmp = buffer + lib->pos;
	char *newpos = buffer + pos;
	guint line = lib->line;
	gboolean prev_was_cr=FALSE;
	DEBUG_MSG("calculate_line_in_buffer, called for position %d ",pos);
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
	DEBUG_MSG("returned line %d ",line);
	return line;
}

static gchar *line_from_buffer(gchar *buffer, guint offset) {
	gint i,j;
	i=j=offset;
	while (i >=0 && i > (offset-40)) {
		if (buffer[i] == '\n' || buffer[i] == '\r') {
			break;
		}
		i--;
	}
	if (i <= offset-40) {
		gchar *tmp = g_utf8_find_next_char(buffer+i, NULL);
		if (tmp)
			i = tmp-buffer;
	} else {
		i++;
	}
	while (buffer[j] !='\0' && j < (offset+40)) {
		if (buffer[j] == '\n' || buffer[j] == '\r') {
			break;
		}
		j++;
	}
	if (j >= offset+40) {
		gchar *tmp = g_utf8_find_prev_char(buffer, buffer+j);
		if (tmp)
			j = tmp-buffer;
	}
	return g_strndup(buffer+i, j-i);
}

static Tfileresult *new_result(guint line, gchar *buffer, guint offset) {
	Tfileresult *fr;

	fr = g_slice_new(Tfileresult);
	fr->line = line;
	fr->text = line_from_buffer(buffer, offset);
	return fr;
}

static GList *snr3_find_pcre(Tsnr3run *s3run, gchar *buffer) {
	Tlineinbuffer lib = {0,1};
	GList *results=NULL;
	GMatchInfo *match_info;

	g_regex_match(s3run->regex, buffer, 0, &match_info);
	while(g_match_info_matches(match_info)) {
		gint so, eo;
		guint line;
		g_match_info_fetch_pos(match_info,0,&so,&eo);
		line = calculate_line_in_buffer(&lib, buffer, so);
		results = g_list_prepend(results, new_result(line, buffer, so));
		g_match_info_next(match_info, NULL);
	}
	g_match_info_free(match_info);

	return results;
}


static GList *snr3_replace_pcre(Tsnr3run *s3run, gchar *buffer, gchar **replacedbuffer) {
	gchar *newbuf;
	gchar *bufferpos, *newbufpos;
	gsize buflen;
	gsize alloced;
	Tlineinbuffer lib = {0,1};
	GList *results=NULL;
	GMatchInfo *match_info;
	gsize prevpos=0;

	DEBUG_MSG("snr3_replace_pcre, search for %s, replace with %s\n",s3run->query, s3run->replace);
	buflen = strlen(buffer);

	alloced = MAX(buflen*2,4096);
	newbuf = g_malloc0(alloced);

	bufferpos = buffer;
	newbufpos = newbuf;
	g_regex_match(s3run->regex, buffer, 0, &match_info);
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
		results = g_list_prepend(results, new_result(line, newbuf, (newbufpos-newbuf)));

		replacestring = g_match_info_expand_references(match_info, s3run->replace, &gerror);
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

static GList *snr3_find_string(Tsnr3run *s3run, gchar *buffer) {
	gchar *result;
	guint querylen;
	Tlineinbuffer lib = {0,1};
	GList *results=NULL;
	char *(*f) ();

	if (s3run->is_case_sens) {
		f = strstr;
	} else {
		f = strcasestr;
	}
	DEBUG_MSG("snr3_find_string, search for %s\n",s3run->query);
	querylen = strlen(s3run->query);
	result = buffer;
	do {
		result = f(result, s3run->query);
		DEBUG_MSG("snr3_find_string, result=%p\n",result);
		if (result) {
			guint line = calculate_line_in_buffer(&lib, buffer, (result-buffer));
			results = g_list_prepend(results, new_result(line, buffer, result-buffer));
			result += querylen;
		}
	} while (result);
	DEBUG_MSG("snr3_find_string, finished\n");
	return results;
}

static GList *snr3_replace_string(Tsnr3run *s3run, gchar *buffer, gchar **replacedbuffer) {
	gchar *result, *newbuf;
	gchar *bufferpos, *newbufpos;
	gsize querylen, replacelen, buflen;
	gsize alloced;
	Tlineinbuffer lib = {0,1};
	GList *results=NULL;
	char *(*f) ();

	DEBUG_MSG("snr3_replace_string, search for %s, replace with %s\n",s3run->query, s3run->replace);
	querylen = strlen(s3run->query);
	replacelen = strlen(s3run->replace);
	buflen = strlen(buffer);


	alloced = (replacelen > querylen)? MAX(1+replacelen+buflen*2,4096):MAX(buflen+querylen+1, 4096);
	newbuf = g_malloc0(alloced);

	bufferpos = buffer;
	newbufpos = newbuf;

	if (s3run->is_case_sens) {
		f = strstr;
	} else {
		f = strcasestr;
	}

	result = buffer;

	do {
		result = f(result, s3run->query);
		if (result) {
			guint line;

			memcpy(newbufpos, bufferpos, result-bufferpos);
			newbufpos += (result-bufferpos);

			line = calculate_line_in_buffer(&lib, newbuf, (newbufpos-newbuf));

			memcpy(newbufpos, s3run->replace, replacelen);
			newbufpos += replacelen;
			result += querylen;
			bufferpos = result;

			results = g_list_prepend(results, new_result(line, newbuf, newbufpos-newbuf));

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
	Tsnr3run *s3run;
} Treplaceinthread;

static gboolean replace_files_in_thread_finished(gpointer data) {
	Treplaceinthread *rit = data;
	gchar *curi;
	GList *tmplist;

	DEBUG_MSG("replace_files_in_thread_finished, add %d results to outputbox\n",g_list_length(rit->results));
	if (g_atomic_int_get(&rit->s3run->cancelled)==0) {
		curi = g_file_get_uri(rit->uri);
		for (tmplist=g_list_first(rit->results);tmplist;tmplist=g_list_next(tmplist)) {
			Tfileresult *fr = tmplist->data;
			outputbox_add_line(rit->s3run->bfwin, curi, fr->line, fr->text);
			g_free(fr->text);
			g_slice_free(Tfileresult, fr);
			rit->s3run->files_resultcount++;
		}
		g_free(curi);
	}
	DEBUG_MSG("replace_files_in_thread_finished, finished rit %p\n", rit);
	snr3run_unrun(rit->s3run);
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
	Tasyncqueue *tmpqueue;

	if (!data) {
		g_warning("problem detected in files_replace_run, data is NULL, please report a bug\n");
		return NULL;
	}

	DEBUG_MSG("thread %p: files_replace_run, started rit %p\n", g_thread_self(), rit);

	g_file_load_contents(rit->uri,NULL,&inbuf,&inbuflen,NULL,&gerror);
	if (g_atomic_int_get(&rit->s3run->cancelled)!=0) {
		g_free(inbuf);
		return NULL;
	}
	if (gerror) {
		g_print("failed to load file: %s\n",gerror->message);
		g_error_free(gerror);
		tmpqueue = &rit->s3run->threadqueue;
		g_idle_add(replace_files_in_thread_finished, rit);
		queue_worker_ready_inthread(tmpqueue);
		return NULL;
	} else {
		DEBUG_MSG("thread %p: calling buffer_find_encoding for %ld bytes\n", g_thread_self(),(glong)strlen(inbuf));
		/* is the following function thread safe ?? */
		utf8buf = buffer_find_encoding(inbuf, inbuflen, &encoding, rit->s3run->bfwin->session->encoding);
		g_free(inbuf);

		if (utf8buf) {
			gchar *replacedbuf=NULL;
			DEBUG_MSG("starting threaded search/replace\n");
			switch (rit->s3run->type) {
				case snr3type_string:
					if (rit->s3run->replaceall) {
						rit->results = snr3_replace_string(rit->s3run, utf8buf, &replacedbuf);
					} else {
						rit->results = snr3_find_string(rit->s3run, utf8buf);
					}
				break;
				case snr3type_pcre:
				if (rit->s3run->replaceall) {
						rit->results = snr3_replace_pcre(rit->s3run, utf8buf, &replacedbuf);
					} else {
						rit->results = snr3_find_pcre(rit->s3run, utf8buf);
					}
				break;
			}
			DEBUG_MSG("finished threaded search/replace\n");
			g_free(utf8buf);
			if ((g_atomic_int_get(&rit->s3run->cancelled)==0) && rit->results && replacedbuf) {
				DEBUG_MSG("replaced %d entries\n",g_list_length(rit->results));
				outbuf = g_convert(replacedbuf, -1, encoding, "UTF-8", NULL, &outbuflen, NULL);

				g_file_replace_contents(rit->uri,outbuf,outbuflen,NULL,TRUE,G_FILE_CREATE_NONE,NULL,NULL,&gerror);
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
	tmpqueue = &rit->s3run->threadqueue;
	if (g_atomic_int_get(&rit->s3run->cancelled)==0) {
		g_idle_add(replace_files_in_thread_finished, rit);
	}

	DEBUG_MSG("thread %p: calling queue_worker_ready_inthread\n",g_thread_self());
	queue_worker_ready_inthread(tmpqueue);
	/* we don't need to start a new thread by calling _inthread() inside the thread */
	return NULL;
}

static void finished_finding_files_cb(Tsnr3run *s3run) {
	DEBUG_MSG("finished_finding_files_cb\n");
	s3run->findfiles=NULL;
	snr3run_unrun(s3run);
}

static void filematch_cb(Tsnr3run *s3run, GFile *uri, GFileInfo *finfo) {
	Treplaceinthread *rit;
	Tdocument *doc;
	DEBUG_MSG("filematch_cb\n");
	if (g_atomic_int_get(&s3run->cancelled)!=0) {
		/* do nothing */
		DEBUG_MSG("filematch_cb, cancelled, do nothing\n");
		return;
	}
	/* TODO: first check if we have this file open, in that case we have to run the
	function that replaces in the document */
	doc = documentlist_return_document_from_uri(s3run->bfwin->documentlist, uri);
	if (doc) {
		DEBUG_MSG("filematch_cb, this file is already open, use snr3_run_in_doc()\n");
		snr3_run_in_doc(s3run, doc, 0, -1, FALSE);
		return;
	}
	rit = g_slice_new0(Treplaceinthread);
	rit->uri = uri;
	g_object_ref(rit->uri);
	rit->s3run = s3run;
	g_atomic_int_inc(&rit->s3run->runcount);
	DEBUG_MSG("filematch_cb, push rit %p to queue, s3run runcount is %d\n", rit, rit->s3run->runcount);
	queue_push(&s3run->threadqueue, rit);
}

static void
queue_cancel_freefunc(gpointer data, gpointer user_data)
{
	Treplaceinthread *rit=data;
	g_object_unref(rit->uri);
	snr3run_unrun(rit->s3run);
	g_slice_free(Treplaceinthread, rit);
}

void snr3_run_in_files_cancel(Tsnr3run *s3run) {
	DEBUG_MSG("snr3_run_in_files_cancel s3run %p\n",s3run);
	g_atomic_int_set(&s3run->cancelled, 1);
	if (s3run->findfiles) {
		findfiles_cancel(s3run->findfiles);
	}
	queue_cancel(&s3run->threadqueue, queue_cancel_freefunc, NULL);
	/* hmm but what if there are still idle callbacks registered ???
	the s3run structure can only be free'd after the fw refcount
	is 0 */
}

void snr3_run_in_files(Tsnr3run *s3run) {
	DEBUG_MSG("snr3_run_in_files, started for s3run=%p\n",s3run);
	g_atomic_int_set(&s3run->cancelled, 0);
	queue_init_full(&s3run->threadqueue, 4, TRUE, TRUE, (QueueFunc)files_replace_run);
	g_print("filepattern=%s\n",s3run->filepattern);
	g_atomic_int_set(&s3run->runcount, 1); /* start with one reference for the findfiles() call */
	s3run->files_resultcount = 0;
	s3run->findfiles = findfiles(s3run->basedir, (s3run->recursion_level > 0), s3run->recursion_level, TRUE,s3run->filepattern, G_CALLBACK(filematch_cb), G_CALLBACK(finished_finding_files_cb), s3run);
}
