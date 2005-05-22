#define DEBUG

#include <gtk/gtk.h>
#include <string.h>

#include "config.h"
#include "bluefish.h"
#include "document.h"
#include "bf_lib.h"

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
	Tbfwin *bfwin;
	const gchar *formatstring;
	gchar *commandstring;

	gchar *changedir;
	gchar *securedir; /* if we need any local temporary files for input or output we'll use fifo's */
	gchar *fifo_in;
	gchar *fifo_out;
	gboolean pipe_in;
	gboolean pipe_out;
	
	pid_t child_process;
	gchar *buffer_in;
	GIOChannel* channel_in;
	gchar *buffer_out;
	gchar *buffer_out_position;
	GIOChannel* channel_out;
} Texternalp;

static void externalp_cleanup(Texternalp *ep) {
	DEBUG_MSG("externalp_cleanup, started for %p\n",ep);
	if (ep->commandstring) g_free(ep->commandstring);
	if (ep->changedir) g_free(ep->changedir);
	if (ep->fifo_in) g_free(ep->fifo_in);
	if (ep->fifo_out) {
		unlink(ep->fifo_out);
		g_free(ep->fifo_out);
	}
	if (ep->buffer_in) g_free(ep->buffer_in);
	if (ep->buffer_out) g_free(ep->buffer_out);
	if (ep->securedir) {
		rmdir(ep->securedir);
		g_free(ep->securedir);
	}
	g_free(ep);
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
		DEBUG_MSG("start_command_write_lcb, finished\n");
		g_io_channel_shutdown(channel,TRUE,&error);
		/* BUG: cleanup here ? */
		return FALSE;
	}
	return TRUE;
}

static void start_command(Texternalp *ep, GIOFunc channel_out_lcb,gpointer out_data) {
	int out_pipe[2],in_pipe[2]; /* in pipes, 0 is for reading, 1 is for writing */
	
	if (ep->pipe_in) {
		if (pipe(in_pipe) != 0) {
			g_print("some error happened creating a pipe??\n");
			return;
		}
	} else if (ep->fifo_in) {
		if (mkfifo(ep->fifo_in, 0600) != 0) {
			g_print("some error happened creating fifo %s??\n",ep->fifo_in);
			return;
		}
		DEBUG_MSG("start_command, created fifo %s\n",ep->fifo_in);
	}
	if (ep->pipe_out) {
		if (pipe(out_pipe) != 0) {
			g_print("some error happened ??\n");
			return;
		}
		ep->channel_out = g_io_channel_unix_new(out_pipe[0]);
	} else if (ep->fifo_out) {
		if (mkfifo(ep->fifo_out, 0600) != 0) {
			g_print("some error happened creating fifo %s??\n",ep->fifo_out);
			return;
		}
	}
	
	DEBUG_MSG("start_command, before fork()\n");
	if ((ep->child_process = fork()) == 0) {
		if (ep->pipe_out) {
			close(out_pipe[0]);
			dup2(out_pipe[1],STDOUT_FILENO);
			close(out_pipe[1]);
		}
		if (ep->pipe_in) {
			close(out_pipe[1]);
			dup2(out_pipe[0],STDIN_FILENO);
			close(out_pipe[0]);
		}
		dup2(STDOUT_FILENO,STDERR_FILENO);
		if (ep->changedir) {
			chdir(ep->changedir);
		}
		execlp("/bin/sh", "sh", "-c", ep->commandstring, NULL);
		exit(127);
	}
	if (ep->pipe_in) {
		close(in_pipe[0]);
		ep->channel_in = g_io_channel_unix_new(in_pipe[1]);
	} else if (ep->fifo_in) {
		ep->channel_in = g_io_channel_new_file(ep->fifo_in,"w",NULL);
	}
	if (ep->pipe_in || ep->fifo_in) {
		ep->buffer_out = ep->buffer_out_position = doc_get_chars(ep->bfwin->current_document,0,-1);
		g_io_channel_set_flags(ep->channel_in,G_IO_FLAG_NONBLOCK,NULL);
		/* now we should start writing, correct ? */
		DEBUG_MSG("start_command, add watch for channel_in\n");
		g_io_add_watch(ep->channel_in,G_IO_OUT,start_command_write_lcb,ep);
	}
	if (ep->pipe_out) {
		close(out_pipe[1]);
		ep->channel_out = g_io_channel_unix_new(out_pipe[0]);
	} else if (ep->fifo_out) {
		ep->channel_out = g_io_channel_new_file(ep->fifo_out,"r",NULL);
	}
	if (ep->pipe_out || ep->fifo_out) {
		g_io_channel_set_flags(ep->channel_out,G_IO_FLAG_NONBLOCK,NULL);
		DEBUG_MSG("start_command, add watch for channel_out\n");
		g_io_add_watch(ep->channel_out, G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP,channel_out_lcb,out_data);
	}
}


/* The format string should have new options:
    * %s local full path (function should abort for remote files)
    * %c local directory of file (function should abort for remote files)
    * %f filename without path
    * %u URL
    * %i temporary filename, before execute bluefish should create this locally
    * %o temporary local filename for output of filters
    * if the first character is a pipe | we'll use a pipe to feed the documentdata
    * if the last character is a pipe | we'll use a pipe to read the filter/outputbox data
*/
static gchar *create_commandstring(Texternalp *ep, const gchar *formatstring, gboolean discard_output) {
	Tconvert_table *table;
	gchar *localname=NULL, *localfilename=NULL, *retstring, *curi;
	gboolean need_local = FALSE, need_tmpin = FALSE, need_tmpout = FALSE;
	gint items = 2, cur;
	
	if (!ep->bfwin->current_document || !ep->bfwin->current_document->uri) {
		return NULL;
	}
	need_local = (strstr(formatstring, "%c") != NULL || strstr(formatstring, "%s") != NULL);
	if (!gnome_vfs_uri_is_local(ep->bfwin->current_document->uri) && need_local) {
		/* BUG: give a warning that the current command only works for local files */
		return NULL;
	}
	need_tmpin = (strstr(formatstring, "%i") != NULL);
	need_tmpout = (strstr(formatstring, "%o") != NULL);
	if (need_tmpout && discard_output) {
		/*  BUG: give a warning that external commands should not use %o */
		return NULL;
	}
	DEBUG_MSG("create_commandstring, formatstring '%s' seems OK\n",formatstring);
	curi = gnome_vfs_uri_to_string(ep->bfwin->current_document->uri,GNOME_VFS_URI_HIDE_PASSWORD);
	if (need_local) {
		localname = gnome_vfs_get_local_path_from_uri(curi);
		localfilename = strrchr(localname, '/');
		items += 2;
	}
	if (need_tmpin) items++;
	if (need_tmpout) items++;
	
	
	table = g_new(Tconvert_table, items+1);
	table[0].my_int = 'f';
	table[0].my_char = need_local ? g_strdup(localfilename) : gnome_vfs_uri_extract_short_path_name(ep->bfwin->current_document->uri);
	table[1].my_int = 'u';
	table[1].my_char = curi;
	cur = 2;
	if (need_tmpin) {
		table[cur].my_int = 'i';
		ep->fifo_in = create_secure_dir_return_filename();
		table[cur].my_char = g_strdup(ep->fifo_in);
		DEBUG_MSG("create_commandstring, %%i will be at %s\n",ep->fifo_in);
		cur++;
	}
	if (need_tmpout) {
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
		table[5].my_int = 'c';
		table[5].my_char = g_strndup(localname, strlen(localname) - strlen(localfilename));
		cur++;
	}
	table[cur].my_int = '\0';
	table[cur].my_char = NULL;
	
	retstring = replace_string_printflike(formatstring, table);
	free_convert_table(table);

	return retstring;
}

void outputbox_command(Tbfwin *bfwin, const gchar *formatstring, GIOFunc callback, gpointer callback_data) {
	Texternalp *ep;
	
	ep = g_new0(Texternalp,1);
	ep->bfwin = bfwin;
	ep->formatstring = formatstring;
	ep->commandstring = create_commandstring(ep, formatstring, FALSE);
	if (!ep->commandstring) {
		g_free(ep);
		/* BUG: is the user notified of the error ?*/
		return;
	}
	if (!ep->fifo_out) {
		ep->pipe_out = TRUE;
	}
	if (!ep->fifo_in) {
		ep->pipe_in = TRUE;
	}
	start_command(ep, callback, callback_data);
}

static gboolean filter_io_watch_lcb(GIOChannel *channel,GIOCondition condition,gpointer data) {
/*	if (condition & G_IO_IN) {
		gchar *buf=NULL;
		gsize buflen=0,termpos=0;
		GError *error=NULL;
		GIOStatus status = g_io_channel_read_chars(channel,,,,&error);
	}
	if (condition & G_IO_OUT) {
		DEBUG_MSG("io_watch_lcb, condition %d G_IO_OUT not handled\n",condition);
	}
	if (condition & G_IO_PRI) {
		DEBUG_MSG("io_watch_lcb, condition %d G_IO_PRI not handled\n",condition);
	}
	if (condition & G_IO_ERR) {
		DEBUG_MSG("io_watch_lcb, condition %d G_IO_ERR not handled\n",condition);
	}
	if (condition & G_IO_HUP) {
		DEBUG_MSG("io_watch_lcb, condition %d G_IO_HUP\n",condition);
		/ *BUG: need cleanup !!!!!!!!! * /
		return FALSE;
	}
	if (condition & G_IO_NVAL) {
		DEBUG_MSG("io_watch_lcb, condition %d G_IO_NVAL not handled\n",condition);
	}*/
	return TRUE;
}

void filter_command(Tbfwin *bfwin, const gchar *formatstring, GIOFunc callback, gpointer callback_data) {
	Texternalp *ep;
	
	ep = g_new0(Texternalp,1);
	ep->bfwin = bfwin;
	ep->formatstring = formatstring;
	ep->commandstring = create_commandstring(ep, formatstring, FALSE);
	if (!ep->commandstring) {
		g_free(ep);
		/* BUG: is the user notified of the error ?*/
		return;
	}
	if (!ep->fifo_out) {
		ep->pipe_out = TRUE;
	}
	if (!ep->fifo_in) {
		ep->pipe_in = TRUE;
	}
	start_command(ep, callback, callback_data);
}

void external_command(Tbfwin *bfwin, const gchar *formatstring) {
	Texternalp *ep;
	
	ep = g_new0(Texternalp,1);
	ep->bfwin = bfwin;
	ep->formatstring = formatstring;
	ep->commandstring = create_commandstring(ep, formatstring, TRUE);
	if (!ep->commandstring) {
		g_free(ep);
		/* BUG: is the user notified of the error ?*/
		return;
	}
	
	start_command(ep, NULL, NULL);
}

