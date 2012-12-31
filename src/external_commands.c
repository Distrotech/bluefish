/* Bluefish HTML Editor
 * external_commands.c - backend for external commands, filters and the outputbox
 *
 * Copyright (C) 2005-2012 Olivier Sessink
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

/*#define DEBUG*/

#include <gtk/gtk.h>
#include <string.h>

#include "config.h"
#include "bluefish.h"
#include "bf_lib.h"
#include "document.h"
#include "gtk_easy.h"
#include "outputbox.h"
#include "dialog_utils.h"
#include "external_commands.h"

#ifndef WIN32
#define USEBINSH
#endif

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
	gint refcount; /* the reference count for this structure, on 0 we can free this structure */
	Tbfwin *bfwin;
	Tdocument *doc;
	gint begin; /* 0 for begin of the file */
	gint end; /* -1 for end of the file */
	const gchar *formatstring; /* the command from the configuration, so including placeholders such as %i, %f, |,  etc. */
	gchar *commandstring; /* the command that will be started by g_spawn, the placeholders should have been replaced/removed */
	CustomCommandCallback customcommand_cb;
	gpointer data;

	gchar *securedir; /* if we need any local temporary files for input or output */
	gchar *tmp_in; /* the path for the input/output temporary filename */
	gchar *tmp_out;
	gchar *inplace; /* if we use in-place editing, so the input file will become the output file */
	gboolean pipe_in; /* if we use pipes for input and output */
	gboolean pipe_out;

	gboolean include_stderr; /* if stderr should be included in the output */

	gint standard_input; /* the pipes that will be filled by g_spawn_async_with_pipes */
	gint standard_output;

	GIOChannel* channel_in; /* the GIO channels for input into the filter */
	gchar *buffer_out; /* the buffer that is the input for the filter */
	gchar *buffer_out_position; /* a pointer inside the above buffer that points to the data that has not yet been written to the filter */
	gboolean havedata;

	GIOChannel* channel_out; /* the GIO channel for output from the filter */
	GIOFunc channel_out_lcb; /* the callback that will be registered for output data  */
	gpointer channel_out_data; /* the corresponding callback data */

	GPid child_pid; /* PID of the filter */
	guint start_command_idle_id;
} Texternalp;

static void start_command_backend(Texternalp *ep);

static void externalp_unref(Texternalp *ep) {
	ep->refcount--;
	DEBUG_MSG("externalp_unref, started for %p, refount=%d\n",ep,ep->refcount);
	if (ep->refcount <= 0) {
		DEBUG_MSG("externalp_unref, cleanup!\n");
		if (ep->commandstring) g_free(ep->commandstring);
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
		if (ep->channel_in)
			g_io_channel_unref(ep->channel_in);
		if (ep->channel_out)
			g_io_channel_unref(ep->channel_out);
		g_free(ep);
	}
}

static gboolean start_command_write_lcb(GIOChannel *channel,GIOCondition condition,gpointer data) {
	Texternalp *ep = data;
	GError *gerror=NULL;
	gsize bytes_written=0;
	DEBUG_MSG("start_command_write_lcb, started, still %zd bytes to go\n",strlen(ep->buffer_out_position));

	g_io_channel_write_chars(channel,ep->buffer_out_position,-1,&bytes_written,&gerror);
	if (gerror) {
		g_warning("failed to write data to external program, %d: %s\n",gerror->code,gerror->message);
		g_error_free(gerror);
		gerror=NULL;
		g_io_channel_shutdown(channel,TRUE,&gerror);
		return FALSE;
	}
	DEBUG_MSG("start_command_write_lcb, %zd bytes written\n",bytes_written);
	ep->buffer_out_position += bytes_written;
	if (strlen(ep->buffer_out) <= (ep->buffer_out_position - ep->buffer_out)) {
		DEBUG_MSG("start_command_write_lcb, finished, shutting down channel\n");
		g_io_channel_shutdown(channel,TRUE,&gerror);
		if (gerror) {
			g_warning("failed to close down data stream to external program, %d: %s\n",gerror->code,gerror->message);
			g_error_free(gerror);
			return FALSE;
		}
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
	GError *gerror=NULL;
	gchar *tmp;
	DEBUG_MSG("child_watch_lcb, child exited with status=%d\n",status);
#ifndef WIN32
	/* on windows this even fires dialogs when opening something in firefox, disabling that for the moment */
	if (status != 0) {
		tmp = g_markup_printf_escaped(_("The command %s exited with error code %d. %s"), ep->commandstring, status, (status==32512)?_("Probably this application is not installed on your system."):"");
		message_dialog_new(BFWIN(ep->bfwin)->main_window, GTK_MESSAGE_ERROR
					, GTK_BUTTONS_CLOSE, _("Command returned error code"), tmp);
		g_free(tmp);
	}
#endif
	if (ep->pipe_in && !ep->buffer_out) {
		DEBUG_MSG("child_watch_lcb, the child has exited before we actually started\n");
		/* the child has exited before we actually started to write data to the child, just abort now */
		g_source_remove(ep->start_command_idle_id);
		externalp_unref(ep); /* unref twice, once for the start_command_idle, once for child_watch_lcb */
		externalp_unref(ep);
		return;
	}

	/* if there was a temporary output file, we should now open it and start to read it */
	if (ep->tmp_out) {

		ep->channel_out = g_io_channel_new_file(ep->tmp_out,"r",&gerror);
		DEBUG_MSG("child_watch_lcb, created channel_out from file %s\n",ep->tmp_out);
		if (gerror) {
			g_warning("failed to read data from external program, %d: %s\n",gerror->code,gerror->message);
			g_error_free(gerror);
		}
	} else if (ep->inplace) {
		ep->channel_out = g_io_channel_new_file(ep->inplace,"r",NULL);
		DEBUG_MSG("child_watch_lcb, created channel_out from file %s\n",ep->inplace);
		if (gerror) {
			g_warning("failed to read data from external program, %d: %s\n",gerror->code,gerror->message);
			g_error_free(gerror);
		}
	}
	if (ep->tmp_out||ep->inplace){
		ep->refcount++;
		g_io_channel_set_flags(ep->channel_out,G_IO_FLAG_NONBLOCK,NULL);
		DEBUG_MSG("child_watch_lcb, add watch for channel_out\n");
		g_io_add_watch(ep->channel_out, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP,ep->channel_out_lcb,ep->channel_out_data);
	}
	g_spawn_close_pid(pid);
	DEBUG_MSG("child_watch_lcb, almost finished, call externalp_unref\n");
	externalp_unref(ep);
}

static gboolean
start_command_idle(gpointer data)
{
	Texternalp *ep = data;
	if (ep->pipe_in) {
		DEBUG_MSG("start_command_idle, creating channel_in from pipe\n");
#ifdef WIN32
		ep->channel_in = g_io_channel_win32_new_fd(ep->standard_input);
#else
		ep->channel_in = g_io_channel_unix_new(ep->standard_input);
#endif
	}
	if (ep->pipe_in) {
		ep->refcount++;
		ep->buffer_out = ep->buffer_out_position = doc_get_chars(ep->bfwin->current_document,ep->begin,ep->end);
		g_io_channel_set_flags(ep->channel_in,G_IO_FLAG_NONBLOCK,NULL);
		/* now we should start writing, correct ? */
		DEBUG_MSG("start_command_idle, begin=%d, end=%d, add watch for channel_in\n",ep->begin, ep->end);
		g_io_add_watch(ep->channel_in,G_IO_OUT,start_command_write_lcb,ep);
	}
	if (ep->pipe_out) {
#ifdef WIN32
		ep->channel_out = g_io_channel_win32_new_fd(ep->standard_output);
#else
		ep->channel_out = g_io_channel_unix_new(ep->standard_output);
#endif
		DEBUG_MSG("start_command_idle, created channel_out from pipe\n");
	}
	if (ep->channel_out_lcb && ep->pipe_out) {
		ep->refcount++;
		g_io_channel_set_flags(ep->channel_out,G_IO_FLAG_NONBLOCK,NULL);
		DEBUG_MSG("start_command_idle, add watch for channel_out\n");
		g_io_add_watch(ep->channel_out, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP,ep->channel_out_lcb,ep->channel_out_data);
	}
	externalp_unref(ep);

	return FALSE; /* don't call me again */
}

static void start_command_backend(Texternalp *ep) {
#ifdef USEBINSH
	gchar *argv[4];
#else
	gchar **argv;
#endif
	GError *error=NULL;

	if (!ep->bfwin->current_document)
		return;

#ifdef USEBINSH
	argv[0] = "/bin/sh";
	argv[1] = "-c";
	argv[2] = ep->commandstring;
	argv[3] = NULL;
#else
    g_shell_parse_argv(ep->commandstring, NULL, &argv, NULL);
#endif
	DEBUG_MSG("start_command_backend,commandstring=%s\n",ep->commandstring);
	DEBUG_MSG("start_command_backend, pipe_in=%d, pipe_out=%d, include_stderr=%d\n",ep->pipe_in,ep->pipe_out,ep->include_stderr);
#ifdef USEBINSH
	DEBUG_MSG("start_command_backend, about to spawn process /bin/sh -c %s\n",argv[2]);
#else
	DEBUG_MSG("start_command_backend, about to spawn process %s\n",ep->commandstring);
#endif
	g_spawn_async_with_pipes(NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH, (ep->include_stderr)?spawn_setup_lcb:NULL, ep, &ep->child_pid,
				(ep->pipe_in) ? &ep->standard_input : NULL,
				(ep->pipe_out) ? &ep->standard_output : NULL,
				NULL, &error);
	ep->refcount+=2; /* one reference for this function (cleared in the end), one reference for the child_watch function */
	g_child_watch_add(ep->child_pid,child_watch_lcb,ep);
	if (error) {
		g_warning("error while trying to create a new process %d: %s\n",error->code,error->message);
		DEBUG_MSG("start_command_backend, there is an error: %s!!\n",error->message);
		g_error_free(error);
		externalp_unref(ep);
		return;
	}
	ep->start_command_idle_id = g_idle_add(start_command_idle, ep);
}

static void start_command(Texternalp *ep) {
	if (!ep->bfwin->current_document)
		return;

	if (ep->tmp_in) {
		GError *gerror=NULL;
		/* first create tmp_in, then start the real command in the callback */
		ep->channel_in = g_io_channel_new_file(ep->tmp_in,"w",&gerror);
		if (gerror) {
			g_warning("failed to create input file for external program, %d: %s\n",gerror->code,gerror->message);
			g_error_free(gerror);
			return;
		}
		ep->buffer_out = ep->buffer_out_position = doc_get_chars(ep->bfwin->current_document,ep->begin,ep->end);
		g_io_channel_set_flags(ep->channel_in,G_IO_FLAG_NONBLOCK,NULL);
		DEBUG_MSG("start_command, add watch for channel_in\n");
		g_io_add_watch(ep->channel_in,G_IO_OUT,start_command_write_lcb,ep);
	} else {
		start_command_backend(ep);
	}
}

gboolean operatable_on_selection(const gchar *formatstring) {

	return (strstr(formatstring, "%f") == NULL && strstr(formatstring, "%c") == NULL
		&& strstr(formatstring, "%n") == NULL && strstr(formatstring, "%u") == NULL
		&& strstr(formatstring, "%p") == NULL);

}

/* The format string should have new options:
    * %f local full path (function should abort for remote files)
    * %c local directory of file (function should abort for remote files)
    * %n filename without path
    * %u URL
    * %p preview URL if basedir and preview dir are set in project settings, else identical to %u
    * %i temporary filename for input (fifo is faster), if the document is not modified and local equal to %f
    * %o temporary filename for output of filters or outputbox (fifo is faster) (previously %f)
    * %t temporary filename for both input and output (for in-place-editing filters)
    * %a user supplied arguments
*/
static gchar *create_commandstring(Texternalp *ep, const gchar *formatstr, gboolean discard_output) {
	Tconvert_table *table;
	gchar *localname=NULL, *localfilename=NULL, *retstring, *curi=NULL;
	gchar *formatstring, *user_argument=NULL;
	gint formatstringlen;
	gboolean is_local_non_modified;
	gboolean need_local = FALSE,
		need_pipein = FALSE,
		need_pipeout = FALSE,
		need_tmpin = FALSE,
		need_tmpout = FALSE,
		need_inplace = FALSE,
		need_preview_uri = FALSE,
		need_filename=FALSE,
		need_arguments=FALSE;
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
	need_arguments = (strstr(formatstring, "%a") != NULL);
	need_filename = need_local = (strstr(formatstring, "%c") != NULL || strstr(formatstring, "%f") != NULL);
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
		return NULL;
	}

	need_tmpin = (strstr(formatstring, "%i") != NULL);
	need_tmpout = (strstr(formatstring, "%o") != NULL);
	need_inplace = (strstr(formatstring, "%t") != NULL);

	if ((need_tmpout || need_inplace || need_pipeout) && discard_output) {
		DEBUG_MSG("create_commandstring, external_commands should not have %%o\n");
		g_free(formatstring);
		g_free(localname);
		return NULL;
	}
	if ((need_tmpin && (need_inplace || need_pipein))
			|| (need_inplace && need_pipein)) {
		DEBUG_MSG("create_commandstring, cannot have multiple inputs\n");
		/*  BUG: give a warning that you cannot have multiple inputs */
		g_free(formatstring);
		g_free(localname);
		return NULL;
	}
	if ((need_tmpout && (need_inplace || need_pipeout))
			|| (need_inplace && need_pipeout)) {
		DEBUG_MSG("create_commandstring, cannot have multiple outputs, tmpout=%d, pipeout=%d, inplace=%d\n",need_tmpout,need_pipeout,need_inplace);
		/*  BUG: give a warning that you cannot have multiple outputs */
		g_free(formatstring);
		g_free(localname);

		return NULL;
	}

	if (need_arguments) {
		GtkWidget *dialog, *entry;
		gchar *tmp;
		gint result;
		dialog = gtk_dialog_new_with_buttons(_("Supply extra arguments"),
											  GTK_WINDOW(ep->bfwin->main_window),
											  GTK_DIALOG_DESTROY_WITH_PARENT,
											  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
											  GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
		tmp = g_strdup_printf(_("Supply arguments to define %%a in '%s'"), formatstring);
		entry = dialog_entry_labeled(NULL, tmp, gtk_dialog_get_content_area(GTK_DIALOG(dialog)), 6);
		g_free(tmp);
		gtk_widget_show_all(dialog);
		result = gtk_dialog_run(GTK_DIALOG (dialog));
		switch (result) {
		case GTK_RESPONSE_ACCEPT:
			user_argument = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
			if (!user_argument || user_argument[0]=='\0') {
				need_arguments = FALSE;
			}
		break;
		default:
			gtk_widget_destroy(dialog);
			return NULL;
		break;
		}
		gtk_widget_destroy(dialog);
	}


	if (need_pipein)  ep->pipe_in = TRUE;
	if (need_pipeout) ep->pipe_out = TRUE;
	DEBUG_MSG("create_commandstring, formatstring '%s' seems OK\n",formatstring);

	is_local_non_modified = (ep->bfwin->current_document->uri
		&& !ep->bfwin->current_document->modified
		&& localname != NULL);

	DEBUG_MSG("create_commandstring,is_local_non_modified=%d, uri=%p, modified=%d\n",is_local_non_modified,ep->bfwin->current_document->uri, ep->bfwin->current_document->modified);

	if (need_filename || is_local_non_modified) {
		curi = g_file_get_uri(ep->bfwin->current_document->uri);

		if (need_local || (is_local_non_modified && need_tmpin)) {
			#ifdef WIN32
				localfilename = strrchr(localname, '\\')+1;
			#else
				localfilename = strrchr(localname, '/')+1;
			#endif
			items += 2;
		}
		if (need_preview_uri) {
			items++;
		}
	}
	if (need_tmpin) items++;
	if (need_tmpout) items++;
	if (need_inplace) items++;
	if (need_arguments) items++;

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
		table[cur].my_int = 'i';
		if (is_local_non_modified) {
			table[cur].my_char = g_strdup(localname);
		} else {
			ep->tmp_in = create_secure_dir_return_filename();
			table[cur].my_char = g_strdup(ep->tmp_in);
		}
		cur++;
	} else if (need_inplace) {
		table[cur].my_int = 't';
		ep->inplace = create_secure_dir_return_filename();
		table[cur].my_char = g_strdup(ep->inplace);
		DEBUG_MSG("create_commandstring, %%t will be at %s\n",ep->inplace);
		cur++;
	}
	if (need_tmpout) {
		table[cur].my_int = 'o';
		ep->tmp_out = create_secure_dir_return_filename();
		table[cur].my_char = g_strdup(ep->tmp_out);
		cur++;
	}
	if (need_local) {
		table[cur].my_int = 'f';
		table[cur].my_char = localname;
		cur++;
		table[cur].my_int = 'c';
		table[cur].my_char = g_strndup(localname, strlen(localname) - strlen(localfilename));
		cur++;
	}
	if (need_arguments) {
		table[cur].my_int = 'a';
		table[cur].my_char = g_strdup(user_argument);
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
	g_free(user_argument);
	DEBUG_MSG("create_commandstring, returning %s\n",retstring);
	return retstring;
}

static gboolean outputbox_io_watch_lcb(GIOChannel *channel,GIOCondition condition,gpointer data) {
	Texternalp *ep = data;
	DEBUG_MSG("outputbox_io_watch_lcb, called with condition %d\n",condition);
	if (!ep->bfwin->outputbox) {
		DEBUG_MSG("outputbox_io_watch_lcb, outputbox == NULL, close the channel\n");
		g_io_channel_shutdown(channel,TRUE,NULL);
		externalp_unref(ep);
		return FALSE;
	}

	if (condition & G_IO_IN) {
		gchar *buf=NULL;
		gsize buflen=0,termpos=0;
		GError *gerror=NULL;
		GIOStatus status = g_io_channel_read_line(channel,&buf,&buflen,&termpos,&gerror);
		while (status == G_IO_STATUS_NORMAL) {
			if (buflen > 0) {
				if (termpos < buflen) buf[termpos] = '\0';
				fill_output_box(ep->bfwin->outputbox, buf);
				ep->havedata=TRUE;
			}
			g_free(buf);
			status = g_io_channel_read_line(channel,&buf,&buflen,&termpos,&gerror);
		}
		if (gerror) {
			g_warning("error while trying to read data for outputbox, %d: %s\n",gerror->code,gerror->message);
			g_error_free(gerror);
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

static gboolean full_data_io_watch_lcb(GIOChannel *channel,GIOCondition condition,gpointer data) {
	Texternalp *ep = data;
	DEBUG_MSG("full_data_io_watch_lcb, started with condition=%d\n",condition);
	if (condition & G_IO_IN) {
		gchar *str_return;
		gsize length;
		GIOStatus status;
		GError *gerror=NULL;

		status = g_io_channel_read_to_end(channel,&str_return,&length,&gerror);
		if (gerror) {
			g_warning("error while trying to read data for filter or custom command, %d: %s\n",gerror->code,gerror->message);
			g_error_free(gerror);
		}

		if (status == G_IO_STATUS_NORMAL && str_return) {
			GError *error=NULL;
			DEBUG_MSG("ep->customcommand_cb=%p\n",ep->customcommand_cb);
			if (ep->customcommand_cb)
				ep->customcommand_cb(str_return, ep->bfwin, ep->data);
			g_io_channel_shutdown(channel,TRUE,&error);
			externalp_unref(ep);
			g_free(str_return);
			return FALSE;
		} else {
			DEBUG_MSG("full_data_io_watch_lcb, status=%d\n",status);
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

static void
command_backend(Tbfwin *bfwin, gint begin, gint end,
								const gchar *formatstring,
								GIOFunc channel_out_cb,
								gboolean incl_stderr,
								CustomCommandCallback customcommand_cb, gpointer data) {
	Texternalp *ep;

	DEBUG_MSG("command_backend, started\n");
	ep = g_new0(Texternalp,1);
	ep->bfwin = bfwin;
	ep->customcommand_cb = customcommand_cb;
	ep->data = data ? data : ep;
	ep->doc = bfwin->current_document;
	ep->begin = begin;
	ep->end = end;
	ep->formatstring = formatstring;
	ep->commandstring = create_commandstring(ep, formatstring, (channel_out_cb == NULL));
	if (!ep->commandstring) {
		gchar *tmp = g_markup_printf_escaped(_("Failed to create a command for %s."), formatstring);
		message_dialog_new(BFWIN(ep->bfwin)->main_window, GTK_MESSAGE_ERROR
					, GTK_BUTTONS_CLOSE, _("Failed to create a command"), tmp);
		g_free(tmp);
		DEBUG_MSG("command_backend, failed to create commandstring\n");
		g_free(ep);
		return;
	}
	ep->channel_out_data = ep;
	ep->include_stderr = incl_stderr;
	ep->channel_out_lcb = channel_out_cb;
	start_command(ep);
}

void outputbox_command(Tbfwin *bfwin, const gchar *formatstring) {
	command_backend(bfwin, 0, -1,formatstring, outputbox_io_watch_lcb, TRUE, NULL, NULL);
}

static void
filter_custom_lcb(const gchar *str_return, gpointer bfwin, gpointer data)
{
	Texternalp *ep = data;
	GtkTextIter iter;
	GtkTextBuffer *buffer = BFWIN(bfwin)->current_document->buffer;
	DEBUG_MSG("filter_custom_lcb, received '%s'\n",str_return);
	gint line=-1,offset=-1, end=ep->end;
	if (ep->end == -1) {
		end = gtk_text_buffer_get_char_count(buffer);
	}
	if (!gtk_text_buffer_get_has_selection(buffer)) {
		gtk_text_buffer_get_iter_at_mark(buffer, &iter,gtk_text_buffer_get_insert(buffer));
		line = gtk_text_iter_get_line(&iter);
		offset = gtk_text_iter_get_line_offset(&iter);
	}
	doc_replace_text(BFWIN(bfwin)->current_document, str_return, ep->begin, end);
	if (line != -1) {
		gtk_text_buffer_get_iter_at_line_offset(buffer, &iter, line, offset);
		gtk_text_buffer_place_cursor(buffer, &iter);
	}
}

void filter_command(Tbfwin *bfwin, const gchar *formatstring, gint begin, gint end) {
	command_backend(bfwin, begin, end,formatstring, full_data_io_watch_lcb, FALSE, filter_custom_lcb, NULL);
}

void external_command(Tbfwin *bfwin, const gchar *formatstring) {
	command_backend(bfwin, 0, -1,formatstring, NULL, FALSE, NULL, NULL);
}

void
custom_command(Tbfwin *bfwin, const gchar *formatstring, CustomCommandCallback func, gpointer data)
{
	command_backend(bfwin, 0, -1,formatstring, full_data_io_watch_lcb, TRUE, func, data);
}
