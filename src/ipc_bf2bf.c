/* Bluefish HTML Editor
 * ipc_bf2bf.c - socket IPC communication, bluefish to bluefish
 *
 * Copyright (C) 2009 Olivier Sessink
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
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "bluefish.h"
#include "gui.h"
#include "file.h"
/*#include "ipc_bf2bf.h"*/

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

typedef struct {
	gint fd;
	GIOChannel *chan;
	guint iochan_id;
} Tconnection;

typedef struct {
	gint fd;
	GIOChannel *chan;
	guint iochan_id;
	gboolean master;
	char *path;
} Tipc_bf2bf;

static Tipc_bf2bf ibf = {0, NULL, -1, 0, NULL};

static void handle_message(const gchar *message, gsize len) {
	Tbfwin *bfwin = BFWIN(g_list_last(main_v->bfwinlist)->data);
	if (strcmp(message, "openwin")==0) {
		/* call open new window */
		g_print("open new window\n");
		bfwin = gui_new_window(NULL);
	} else if (strncmp(message, "openuri", 7)==0 && len > 12) {
		GFile *file;
		g_print("open URI %s\n",&message[8]);
		file = g_file_new_for_uri(&message[8]);
		file_handle(file, bfwin);
		g_object_unref(file);	
	} else {
		g_print("unknown message with len %d on socket...\n", len);
		g_print("message: %s\n",message);	
	}
}

static gboolean socket_data_in_lcb(GIOChannel * source, GIOCondition condition, gpointer data)
{
	Tconnection *client = data;
	gchar *instring=NULL;
	gsize instringlen=0, termpos=0;
	GError *error=NULL;
	GIOStatus status;

	g_print("socket_data_in_lcb, called\n");
	if (data == NULL) {
		socklen_t tmp=0;
	
		client = g_new0(Tconnection, 1);
		g_print("socket_data_in_lcb, accept new connection, new client=%p\n",client);
		client->fd = accept(ibf.fd, NULL, &tmp);
		client->chan = g_io_channel_unix_new(client->fd);
		g_io_channel_set_line_term(client->chan, "\n", 1);
		client->iochan_id = g_io_add_watch(client->chan, G_IO_IN, socket_data_in_lcb, client);
		return TRUE;
	}
	
	status = g_io_channel_read_line(source, &instring, &instringlen, &termpos, &error);
	g_print("socket_data_in_lcb, status=%d\n",status);
	while(status ==G_IO_STATUS_NORMAL && instringlen>0) {
		g_print("received data instring=%p, instringlen=%d, termpos=%d\n",instring, instringlen, termpos);
		instring[termpos]='\0';
		handle_message(instring, instringlen);
		g_free(instring);
		instring=NULL;
		status = g_io_channel_read_line(source, &instring, &instringlen, &termpos, &error);
		g_print("socket_data_in_lcb, status=%d\n",status);
	}
	if (status == G_IO_STATUS_EOF) {
		g_print("socket_data_in_lcb, EOF, shutdown channel\n");
		g_io_channel_shutdown(client->chan, FALSE, NULL);
		g_io_channel_unref(client->chan);		
		close(client->fd);
/*		client->chan_id = 0;
		client->chan = NULL;
*/		
		g_print("free client %p\n",client);
		g_free(client);
		return FALSE;	
	}
	return TRUE;
}

static gboolean socket_is_valid(const char *path)
{
	struct stat sbuf;

	if (stat(path, &sbuf) == -1)
		return FALSE;
	if (sbuf.st_uid != geteuid())
		return FALSE;
	if ((sbuf.st_mode & S_IFSOCK) != S_IFSOCK)
		return FALSE;
	/* check permissions ? */
	g_print("socket %s is valid\n",path);
	return TRUE;
}

static char *socket_filename(void)
{
	gchar *path, *newfile, *display;
	display = gdk_get_display();
	newfile = g_strdup_printf(PACKAGE_NAME"-%s-%s", g_get_user_name(), display);
	path = g_build_filename(g_get_tmp_dir(), newfile, NULL);
	g_free(newfile);
	g_free(display);
	return path;
}

static void send_filename(GFile *file)
{
	gchar *uri, *data;
	
	uri = g_file_get_uri(file);
	data = g_strdup_printf("openuri %s\n", uri);
	g_io_channel_write_chars(ibf.chan, data, strlen(data), NULL, NULL);
	g_io_channel_flush(ibf.chan, NULL);
	g_free(uri);
	g_free(data);
}

static void send_openwin(void) {
	gchar *data = g_strdup("openwin\n");
	g_io_channel_write_chars(ibf.chan, data, strlen(data), NULL, NULL);
	g_io_channel_flush(ibf.chan, NULL);
	g_free(data);
}

static gboolean become_server(void)
{
	struct sockaddr_un uaddr;

	uaddr.sun_family = AF_UNIX;
	strncpy(uaddr.sun_path, ibf.path, MIN(strlen(ibf.path) + 1, UNIX_PATH_MAX));
	ibf.fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (bind(ibf.fd, (struct sockaddr *) &uaddr, sizeof(uaddr)) == -1) {
		ibf.fd = -1;
		return FALSE;
	}
	chmod(ibf.path, 0600);
	listen(ibf.fd, 10);
	ibf.chan = g_io_channel_unix_new(ibf.fd);
	g_io_channel_set_line_term(ibf.chan, "\n", 1);
	ibf.iochan_id = g_io_add_watch(ibf.chan, G_IO_IN, socket_data_in_lcb, NULL);
	ibf.master = TRUE;
	return TRUE;
}

static gboolean become_client(void)
{
	struct sockaddr_un uaddr;

	uaddr.sun_family = AF_UNIX;
	strncpy(uaddr.sun_path, ibf.path, MIN(strlen(ibf.path) + 1, UNIX_PATH_MAX));
	ibf.fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (connect(ibf.fd, (struct sockaddr *) &uaddr, sizeof(uaddr)) == -1) {
		ibf.fd = -1;
		g_print("become_client, could not connect to socket, errno=%d: %s\n",errno, strerror(errno));
		return FALSE;
	}
	ibf.chan = g_io_channel_unix_new(ibf.fd);
	ibf.master = FALSE;
	return TRUE;
}

/* filenames is a list of GFile * */
gboolean ipc_bf2bf_start(GList *filenames, gboolean new_window) {

	ibf.path = socket_filename();

	if (socket_is_valid(ibf.path)) {
		if (become_client()) {
			GList *tmplist = g_list_first(filenames);
			g_print("ipc_bf2bf_start, there is a server, send files\n");
			if (new_window)
				send_openwin();
			/* send all files and exit */
			while (tmplist) {
				send_filename(tmplist->data);
				tmplist = g_list_next(tmplist);
			}
			return FALSE;
		}
	}
	unlink(ibf.path);
	become_server();
	g_print("ipc_bf2bf_start, master=%d\n",ibf.master);
	return TRUE;
}

void ipc_bf2bf_cleanup(void)
{
	if (ibf.iochan_id) {
		g_source_remove(ibf.iochan_id);
	}
	if (ibf.chan) {
		g_io_channel_shutdown(ibf.chan, FALSE, NULL);
		g_io_channel_unref(ibf.chan);
	}
	if (ibf.path) {
		if (ibf.master) {
			unlink(ibf.path);
		}
		close(ibf.fd);
		g_free(ibf.path);
	}
}
