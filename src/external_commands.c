/* Bluefish HTML Editor
 * external_commands.c - backend for external commands, filters and the outputbox
 *
 * Copyright (C) 2005-2008 Olivier Sessink
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

/* #define DEBUG */

#include <gtk/gtk.h>
#include <string.h>

#include "config.h"
#include "bluefish.h"
#include "bf_lib.h"
#include "document.h"
#include "gtk_easy.h"
#include "outputbox.h"

/*
 * for the external commands, the external filters, and the outputbox, we have some general code
 * to create the command, and to start the command
 *
 * the external command is simple: it only needs formatting of the commandstring, and creation of any output files
 * for the outputbox, it needs the above, but also the output needs to go back to the outputbox widget
 * for filters it needs formatting of the commandstring, and the output should go to the text widget
 *
 */
typedef struct {
	gint refcount;
	Tbfwin *bfwin;
	Tdocument *doc;
	gint begin; /* 0 for begin of the file */
	gint end; /* -1 for end of the file */
	const gchar *formatstring;
	gchar *commandstring;

	gchar *securedir; /* if we need any local temporary files for input or output we'll use fifo's */
	gchar *fifo_in;
	gchar *fifo_out;
	gchar *tmp_in;
	gchar *tmp_out;
	gchar *inplace;
	gboolean pipe_in;
	gboolean pipe_out;
	
	gboolean include_stderr;
	
	GIOChannel* channel_in;
	gchar *buffer_out;
	gchar *buffer_out_position;
	GIOChannel* channel_out;
	
	GIOFunc channel_out_lcb;
	gpointer channel_out_data;
	
	GPid child_pid;
	
	gpointer data;
} Texternalp;

static void start_command_backend(Texternalp *ep);

static void externalp_unref(Texternalp *ep) {
	ep->refcount--;
	DEBUG_MSG("externalp_unref, started for %p, refount=%d\n",ep,ep->refcount);
	if (ep->refcount <= 0) {
		DEBUG_MSG("externalp_unref, cleanup!\n");
		if (ep->commandstring) g_free(ep->commandstring);
		if (ep->fifo_in) {
			unlink(ep->fifo_in);
			g_free(ep->fifo_in);
		}
		if (ep->fifo_out) {
			unlink(ep->fifo_out);
			g_free(ep->fifo_out);
		}
		if (ep->tmp_in) {
			unlink(ep->tmp_in);
			g_free(ep->tmp_in);
		}
		if (ep->tmp_out) {
			unlink(ep->tmp_out);
			g_free(ep->tmp_out);
		}
		if (ep->inplace) {
			unlink(ep->inplace);
			g_free(ep->inplace);
		}
		
		if (ep->buffer_out) g_free(ep->buffer_out);
		if (ep->securedir) {
			rmdir(ep->securedir);
			g_free(ep->securedir);
		}
		g_free(ep);
	}
}

static gboolean start_command_write_lcb(GIOChannel *channel,GIOCondition condition,gpointer data) {
	Texternalp *ep = data;
	GIOStatus status;
	GError *error=NULL;
	gsize bytes_written=0;
	DEBUG_MSG("start_command_write_lcb, started, still %d bytes to go\n",strlen(ep->buffer_out_position));

	status = g_io_channel_write_chars(channel,ep->buffer_out_position,-1,&bytes_written,&error);
	DEBUG_MSG("start_command_write_lcb, %d bytes written\n",bytes_written);
	ep->buffer_out_position += bytes_written;
	if (strlen(ep->buffer_out) <= (ep->buffer_out_position - ep->buffer_out)) {
		DEBUG_MSG("start_command_write_lcb, finished, shutting down channel\n");
		g_io_channel_shutdown(channel,TRUE,&error);
		if (ep->tmp_in) {
			start_command_backend(ep);
		} else {
			externalp_unref(ep);
		}
		return FALSE;
	}
	return TRUE;
}

static void spawn_setup_lcb(gpointer data) {
	Texternalp *ep = data;
#ifndef WIN32
	/* because win32 does not have both fork() and excec(), this function is
	not called in the child but in the parent on win32 */
	if (ep->include_stderr) {
		dup2(STDOUT_FILENO,STDERR_FILENO);
	}
#endif
}
static void child_watch_lcb(GPid pid,gint status,gpointer data) {
	Texternalp *ep = data;
	DEBUG_MSG("child_watch_lcb, child exited with status=%d\n",status);
	/* if there was a temporary output file, we should now open it and start to read it */
	if (ep->tmp_out) {
		ep->channel_out = g_io_channel_new_file(ep->tmp_out,"r",NULL);
		DEBUG_MSG("child_watch_lcb, created channel_out from file %s\n",ep->tmp_out);
	} else if (ep->inplace) {
		ep->channel_out = g_io_channel_new_file(ep->inplace,"r",NULL);
		DEBUG_MSG("child_watch_lcb, created channel_out from file %s\n",ep->inplace);
	}
	if (ep->tmp_out||ep->inplace){
		ep->refcount++;
		g_io_channel_set_flags(ep->channel_out,G_IO_FLAG_NONBLOCK,NULL);
		DEBUG_MSG("child_watch_lcb, add watch for channel_out\n");
		g_io_add_watch(ep->channel_out, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP,ep->channel_out_lcb,ep->channel_out_data);
	}
	g_spawn_close_pid(pid);
	externalp_unref(ep);
}

#define USEBINSH

/*static gint count_char(char *string, char mychar) {
	gint retval = 0;
	gchar *tmp = string;
	while (*tmp) {
		if (*tmp == mychar)
			retval++;
	}
	return retval;
}*/

static void start_command_backend(Texternalp *ep) {
#ifdef USEBINSH	
	gchar *argv[4];
#else
	gchar **argv;
#endif
	gint standard_input=0,standard_output=0;
	GError *error=NULL;

#ifdef USEBINSH
	argv[0] = "/bin/sh";
	argv[1] = "-c";
	argv[2] = ep->commandstring;
	argv[3] = NULL;
#else
	argv = g_malloc(sizeof(char *)*(count_char(ep->commandstring, ' ')+1));
#endif
	DEBUG_MSG("start_command_backend,commandstring=%s\n",ep->commandstring);
	if (ep->fifo_in) {
		if (mkfifo(ep->fifo_in, 0600) != 0) {
			g_print("some error happened creating fifo %s??\n",ep->fifo_in);
			return;
		}
		DEBUG_MSG("start_command_backend, created fifo %s\n",ep->fifo_in);
	}
	if (ep->fifo_out) {
		if (mkfifo(ep->fifo_out, 0600) != 0) {
			g_print("some error happened creating fifo %s??\n",ep->fifo_out);
			return;
		}
	}
#ifdef WIN32
	ep->include_stderr = FALSE;
#endif
	DEBUG_MSG("start_command_backend, pipe_in=%d, pipe_out=%d, fifo_in=%s, fifo_out=%s,include_stderr=%d\n",ep->pipe_in,ep->pipe_out,ep->fifo_in,ep->fifo_out,ep->include_stderr);
	DEBUG_MSG("start_command_backend, about to spawn process /bin/sh -c %s\n",argv[2]);
	g_spawn_async_with_pipes(NULL,argv,NULL,G_SPAWN_DO_NOT_REAP_CHILD,(ep->include_stderr)?spawn_setup_lcb:NULL,ep,&ep->child_pid,
				(ep->pipe_in) ? &standard_input : NULL,
				(ep->pipe_out) ? &standard_output : NULL,
				NULL,&error);
	ep->refcount+=2; /* one reference for this function (cleared in the end), one reference for the child_watch function */
	g_child_watch_add(ep->child_pid,child_watch_lcb,ep);
	if (error) {
		DEBUG_MSG("start_command_backend, there is an error: %s!!\n",error->message);
		externalp_unref(ep);
		return;
	}
	flush_queue();
	if (ep->pipe_in) {
		DEBUG_MSG("start_command_backend, creating channel_in from pipe\n");
		ep->channel_in = g_io_channel_unix_new(standard_input);
	} else if (ep->fifo_in) {
		DEBUG_MSG("start_command_backend, connecting channel_in to fifo %s\n",ep->fifo_in);
		/* problem: this can hang if there is no process actually reading from this fifo... so if the 
		command died in some way (a nonexisting command), this call will hang bluefish */
		ep->channel_in = g_io_channel_new_file(ep->fifo_in,"w",&error);
		if (error) {
			DEBUG_MSG("start_command_backend, error connecting to fifo %s: %s\n",ep->fifo_in,error->message);
			
			return;
		}
	}
	if (ep->pipe_in || ep->fifo_in) {
		ep->refcount++;
		ep->buffer_out = ep->buffer_out_position = doc_get_chars(ep->bfwin->current_document,ep->begin,ep->end);
		g_io_channel_set_flags(ep->channel_in,G_IO_FLAG_NONBLOCK,NULL);
		/* now we should start writing, correct ? */
		DEBUG_MSG("start_command_backend, begin=%d, end=%d, add watch for channel_in\n",ep->begin, ep->end);
		g_io_add_watch(ep->channel_in,G_IO_OUT,start_command_write_lcb,ep);
	}
	if (ep->pipe_out) {
		ep->channel_out = g_io_channel_unix_new(standard_output);
		DEBUG_MSG("start_command_backend, created channel_out from pipe\n");
	} else if (ep->fifo_out) {
		ep->channel_out = g_io_channel_new_file(ep->fifo_out,"r",NULL);
		DEBUG_MSG("start_command_backend, created channel_out from fifo %s\n",ep->fifo_out);
	}
	if (ep->channel_out_lcb && (ep->pipe_out || ep->fifo_out)) {
		ep->refcount++;
		g_io_channel_set_flags(ep->channel_out,G_IO_FLAG_NONBLOCK,NULL);
		DEBUG_MSG("start_command_backend, add watch for channel_out\n");
		g_io_add_watch(ep->channel_out, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP,ep->channel_out_lcb,ep->channel_out_data);
	}
	externalp_unref(ep);
}

static void start_command(Texternalp *ep) {
	if (ep->tmp_in) {
		/* first create tmp_in, then start the real command in the callback */
		ep->channel_in = g_io_channel_new_file(ep->tmp_in,"w",NULL);
		ep->buffer_out = ep->buffer_out_position = doc_get_chars(ep->bfwin->current_document,ep->begin,ep->end);
		g_io_channel_set_flags(ep->channel_in,G_IO_FLAG_NONBLOCK,NULL);
		DEBUG_MSG("start_command, add watch for channel_in\n");
		g_io_add_watch(ep->channel_in,G_IO_OUT,start_command_write_lcb,ep);
	} else {
		start_command_backend(ep);
	}
}

gboolean operatable_on_selection(const gchar *formatstring) {

	return (strstr(formatstring, "%s") == NULL && strstr(formatstring, "%c") == NULL 
		&& strstr(formatstring, "%n") == NULL && strstr(formatstring, "%u") == NULL
		&& strstr(formatstring, "%p") == NULL);

} 

/* The format string should have new options:
    * %s local full path (function should abort for remote files)
    * %c local directory of file (function should abort for remote files)
    * %n filename without path
    * %u URL
    * %p preview URL if basedir and preview dir are set in project settings, else identical to %u
    * %i temporary fifo for input, if the document is not modified and local equal to %s
    * %o temporary fifo for output of filters or outputbox
    * %I temporary filename for input (fifo is faster), if the document is not modified and local equal to %s
    * %O temporary filename for output of filters or outputbox (fifo is faster) (previously %f)
    * %t temporary filename for both input and output (for in-place-editing filters)
*/
static gchar *create_commandstring(Texternalp *ep, const gchar *formatstr, gboolean discard_output) {
	Tconvert_table *table;
	gchar *localname=NULL, *localfilename=NULL, *retstring, *curi=NULL;
	gchar *formatstring;
	gint formatstringlen;
	gboolean is_local_non_modified;
	gboolean need_local = FALSE,
		need_pipein = FALSE,
		need_pipeout = FALSE,
		need_tmpin = FALSE,
		need_tmpout = FALSE,
		need_fifoin = FALSE,
		need_fifoout = FALSE,
		need_inplace = FALSE,
		need_preview_uri = FALSE,
		need_filename=FALSE;
	gint items = 2, cur=0;
	
	if (!ep->bfwin->current_document) {
		return NULL;
	}
	
	formatstring = g_strstrip(g_strdup(formatstr));
	formatstringlen = strlen(formatstring);

	if (formatstring[0]=='|') {
		need_pipein = TRUE;
		formatstring[0] = ' ';
	}
	
	if (!discard_output && (formatstring[formatstringlen-1]=='|')) {
		need_pipeout = TRUE;
		formatstring[formatstringlen-1]=' ';
	}
	
	need_filename = need_local = (strstr(formatstring, "%c") != NULL || strstr(formatstring, "%s") != NULL);
	need_preview_uri = (strstr(formatstring, "%p") != NULL);
	if (!need_filename) { /* local already implies we need a filename */
		need_filename = (need_preview_uri || strstr(formatstring, "%n") != NULL || strstr(formatstring, "%u") != NULL);
	}
	if (need_filename && !ep->bfwin->current_document->uri) {
		/* BUG: give a warning that the current command only works for files with a name */
		DEBUG_MSG("create_commandstring, this command needs a filename, but there is no\n");
		g_free(formatstring);
		return NULL;
	}
	if (ep->bfwin->current_document->uri)
		localname = g_file_get_path(ep->bfwin->current_document->uri);
	
	if (need_local && !localname) {
		g_free(formatstring);
		g_free(localname);
	}

	need_tmpin = (strstr(formatstring, "%I") != NULL);
	/* %f is for backwards compatibility with bluefish 1.0 */
	need_tmpout = (strstr(formatstring, "%O") != NULL);
	need_fifoin = (strstr(formatstring, "%i") != NULL);
	need_fifoout = (strstr(formatstring, "%o") != NULL);
	need_inplace = (strstr(formatstring, "%t") != NULL);
	
	if ((need_tmpout || need_fifoout || need_inplace || need_pipeout) && discard_output) {
		DEBUG_MSG("create_commandstring, external_commands should not have %%o\n");
		/*  BUG: give a warning that external commands should not use %o */
		g_free(formatstring);
		g_free(localname);

		return NULL;
	}
	if ((need_tmpin && (need_fifoin || need_inplace || need_pipein)) 
			|| (need_fifoin && (need_inplace || need_pipein))
			|| (need_inplace && need_pipein)) {
		DEBUG_MSG("create_commandstring, cannot have multiple inputs\n");
		/*  BUG: give a warning that you cannot have multiple inputs */
		g_free(formatstring);
		g_free(localname);

		return NULL;
	}
	if ((need_tmpout && (need_fifoout || need_inplace || need_pipeout)) 
			|| (need_fifoout && (need_inplace || need_pipeout))
			|| (need_inplace && need_pipeout)) {
		DEBUG_MSG("create_commandstring, cannot have multiple outputs\n");
		/*  BUG: give a warning that you cannot have multiple outputs */
		g_free(formatstring);
		g_free(localname);

		return NULL;
	}
	if (need_pipein)  ep->pipe_in = TRUE;
	if (need_pipeout) ep->pipe_out = TRUE;
/*	if (!need_tmpin && !need_fifoin && !need_inplace && !need_filename && !need_preview_uri) {
		ep->pipe_in = TRUE;
	}
	if (!discard_output && !need_tmpout && !need_fifoout && !need_inplace) {
		ep->pipe_out = TRUE;
	}*/
	DEBUG_MSG("create_commandstring, formatstring '%s' seems OK\n",formatstring);

	is_local_non_modified = (ep->bfwin->current_document->uri 
		&& !ep->bfwin->current_document->modified
		&& localname != NULL);

	DEBUG_MSG("create_commandstring,is_local_non_modified=%d, uri=%p, modified=%d\n",is_local_non_modified,ep->bfwin->current_document->uri, ep->bfwin->current_document->modified);

	if (need_filename || is_local_non_modified) {
		curi = g_file_get_uri(ep->bfwin->current_document->uri);

		if (need_local || (is_local_non_modified && (need_tmpin||need_fifoin))) {
			localfilename = strrchr(localname, '/')+1;
			items += 2;
		}
		if (need_preview_uri) {
			items++;
		}
	}
	if (need_tmpin) items++;
	if (need_tmpout) items++;
	if (need_fifoin) items++;
	if (need_fifoout) items++;
	if (need_inplace) items++;
	
	table = g_new(Tconvert_table, items+1);
	if (need_filename) {
		table[cur].my_int = 'n';
		table[cur].my_char = need_local ? g_strdup(localfilename) : g_file_get_basename(ep->bfwin->current_document->uri);
		cur++;
		table[cur].my_int = 'u';
		table[cur].my_char = curi;
		cur++;
		if (need_preview_uri) {
			table[cur].my_int = 'p';
			if (ep->bfwin->session->documentroot && ep->bfwin->session->webroot 
					&& strlen(ep->bfwin->session->documentroot)>5
					&& strlen(ep->bfwin->session->webroot)>5 
					&& strncmp(curi, ep->bfwin->session->documentroot, strlen(ep->bfwin->session->documentroot))==0) {
						table[cur].my_char = g_strconcat(ep->bfwin->session->webroot, &curi[strlen(ep->bfwin->session->documentroot)], NULL);
			} else {
				table[cur].my_char = g_strdup(curi);
			}
			cur++;
		}
	}
	if (need_tmpin) {
		table[cur].my_int = 'I';
		if (is_local_non_modified) {
			table[cur].my_char = g_strdup(localname);
		} else {
			ep->tmp_in = create_secure_dir_return_filename();
			table[cur].my_char = g_strdup(ep->tmp_in);
		}
		cur++;
	} else if (need_fifoin) {
		table[cur].my_int = 'i';
		if (is_local_non_modified) {
			table[cur].my_char = g_strdup(localname);;
		} else {
			ep->fifo_in = create_secure_dir_return_filename();
			table[cur].my_char = g_strdup(ep->fifo_in);
		}
		
		DEBUG_MSG("create_commandstring, %%i will be at %s\n",table[cur].my_char);
		cur++;
	} else if (need_inplace) {
		table[cur].my_int = 't';
		ep->inplace = create_secure_dir_return_filename();
		table[cur].my_char = g_strdup(ep->inplace);
		DEBUG_MSG("create_commandstring, %%t will be at %s\n",ep->inplace);
		cur++;
	}
	if (need_tmpout) {
		table[cur].my_int = 'O';
		ep->tmp_out = create_secure_dir_return_filename();
		table[cur].my_char = g_strdup(ep->tmp_out);
		cur++;
	} else if (need_fifoout) {
		table[cur].my_int = 'o';
		ep->fifo_out = create_secure_dir_return_filename();
		table[cur].my_char = g_strdup(ep->fifo_out);
		DEBUG_MSG("create_commandstring, %%o will be at %s\n",ep->fifo_out);
		cur++;
	}
	if (need_local) {
		table[cur].my_int = 's';
		table[cur].my_char = localname;
		cur++;
		table[cur].my_int = 'c';
		table[cur].my_char = g_strndup(localname, strlen(localname) - strlen(localfilename));
		cur++;
	}
	table[cur].my_int = '\0';
	table[cur].my_char = NULL;
	if (cur == 0) {
		retstring = g_strdup(formatstring);
	} else {
		retstring = replace_string_printflike(formatstring, table);
	}
	free_convert_table(table);
	g_free(formatstring);
	DEBUG_MSG("create_commandstring, returning %s\n",retstring);
	return retstring;
}

static gboolean outputbox_io_watch_lcb(GIOChannel *channel,GIOCondition condition,gpointer data) {
	Texternalp *ep = data;
	DEBUG_MSG("outputbox_io_watch_lcb, called with condition %d\n",condition);
	if (condition & G_IO_IN) {
		gchar *buf=NULL;
		gsize buflen=0,termpos=0;
		GError *error=NULL;
		GIOStatus status = g_io_channel_read_line(channel,&buf,&buflen,&termpos,&error);
		while (status == G_IO_STATUS_NORMAL) {
			if (buflen > 0) {
				if (termpos < buflen) buf[termpos] = '\0';
				fill_output_box(ep->bfwin->outputbox, buf);
				g_free(buf);
			}
			status = g_io_channel_read_line(channel,&buf,&buflen,&termpos,&error);
		}
		if (status == G_IO_STATUS_EOF) {
			GError *error=NULL;
			/* cleanup !!!!!!!!! */
			g_io_channel_shutdown(channel,TRUE,&error);
			externalp_unref(ep);
			return FALSE;
		}
	}
	if (condition & G_IO_OUT) {
		DEBUG_MSG("outputbox_io_watch_lcb, condition %d G_IO_OUT not handled\n",condition);
	}
	if (condition & G_IO_PRI) {
		DEBUG_MSG("outputbox_io_watch_lcb, condition %d G_IO_PRI not handled\n",condition);
	}
	if (condition & G_IO_ERR) {
		DEBUG_MSG("outputbox_io_watch_lcb, condition %d G_IO_ERR not handled\n",condition);
	}
	if (condition & G_IO_HUP) {
		GError *error=NULL;
		DEBUG_MSG("outputbox_io_watch_lcb, condition %d G_IO_HUP\n",condition);
		/* cleanup !!!!!!!!! */
		g_io_channel_shutdown(channel,TRUE,&error);
		externalp_unref(ep);
		return FALSE;
	}
	if (condition & G_IO_NVAL) {
		DEBUG_MSG("outputbox_io_watch_lcb, condition %d G_IO_NVAL not handled\n",condition);
	}
	return TRUE;
}

static gboolean filter_io_watch_lcb(GIOChannel *channel,GIOCondition condition,gpointer data) {
	Texternalp *ep = data;
	DEBUG_MSG("filter_io_watch_lcb, started with condition=%d\n",condition);
	if (condition & G_IO_IN) {
		gchar *str_return;
		gsize length;
		GIOStatus status;
		GError *error=NULL;
		
		status = g_io_channel_read_to_end(channel,&str_return,&length,&error);
		if (status == G_IO_STATUS_NORMAL && str_return) {
			gint end=ep->end;
			GError *error=NULL;
			DEBUG_MSG("filter_io_watch_lcb, received '%s'\n",str_return);
			if (ep->end == -1) {
				end = gtk_text_buffer_get_char_count(ep->bfwin->current_document->buffer);
			}
			doc_replace_text(ep->bfwin->current_document, str_return, ep->begin, end);
			g_io_channel_shutdown(channel,TRUE,&error);
			externalp_unref(ep);
			g_free(str_return);
			return FALSE;
		} else {
			DEBUG_MSG("filter_io_watch_lcb, status=%d\n",status);
		}
		if (str_return) g_free(str_return);
	}
	if (condition & G_IO_HUP) {
		GError *error=NULL;
		DEBUG_MSG("filter_io_watch_lcb, condition %d G_IO_HUP\n",condition);
		g_io_channel_shutdown(channel,TRUE,&error);
		externalp_unref(ep);
		return FALSE;
	}
	if (condition & G_IO_OUT) {
		DEBUG_MSG("filter_io_watch_lcb, condition %d G_IO_OUT not handled\n",condition);
	}
	if (condition & G_IO_PRI) {
		DEBUG_MSG("filter_io_watch_lcb, condition %d G_IO_PRI not handled\n",condition);
	}
	if (condition & G_IO_ERR) {
		DEBUG_MSG("filter_io_watch_lcb, condition %d G_IO_ERR not handled\n",condition);
	}
	if (condition & G_IO_NVAL) {
		DEBUG_MSG("filter_io_watch_lcb, condition %d G_IO_NVAL not handled\n",condition);
	}
	return TRUE;
}

typedef enum {
	mode_filter,
	mode_outputbox,
	mode_command
} Tcommandmode;

static void command_backend(Tbfwin *bfwin, gint begin, gint end, 
								const gchar *formatstring, Tcommandmode commandmode) {
	Texternalp *ep;
	
	DEBUG_MSG("command_backend, started\n");
	ep = g_new0(Texternalp,1);
	ep->bfwin = bfwin;
	ep->doc = bfwin->current_document;
	ep->begin = begin;
	ep->end = end;
	ep->formatstring = formatstring;
	ep->commandstring = create_commandstring(ep, formatstring, (commandmode==mode_command));
	if (!ep->commandstring) {
		DEBUG_MSG("command_backend, failed to create commandstring\n");
		g_free(ep);
		/* BUG: is the user notified of the error ?*/
		return;
	}
	if (commandmode == mode_filter)	{
		ep->include_stderr = FALSE;
		ep->channel_out_lcb = filter_io_watch_lcb;
		ep->channel_out_data = ep;
	} else if (commandmode == mode_outputbox)	{
		ep->include_stderr = TRUE;
		ep->channel_out_lcb = outputbox_io_watch_lcb;
		ep->channel_out_data = ep;
	} else if (commandmode == mode_command) {
		ep->include_stderr = FALSE;
		ep->channel_out_lcb = NULL;
		ep->channel_out_data = NULL;
	}

	start_command(ep);
}

void outputbox_command(Tbfwin *bfwin, const gchar *formatstring) {
	command_backend(bfwin, 0, -1,formatstring, mode_outputbox);
}


void filter_command(Tbfwin *bfwin, const gchar *formatstring, gint begin, gint end) {
	command_backend(bfwin, begin, end,formatstring, mode_filter);
}

void external_command(Tbfwin *bfwin, const gchar *formatstring) {
	command_backend(bfwin, 0, -1,formatstring, mode_command);
}
