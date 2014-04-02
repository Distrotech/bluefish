/* Bluefish HTML Editor
 * ipc_bf2bf.c - socket IPC communication, bluefish to bluefish
 *
 * Copyright (C) 2009-2012 Olivier Sessink
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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else							/* WIN32 */
#include <sys/socket.h>
#include <sys/un.h>
#endif		/* WIN32 */
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bluefish.h"
#include "ipc_bf2bf.h"
#include "bfwin.h"
#include "file.h"


#ifdef WIN32
#ifndef HAVE_STDINT_H
typedef unsigned char uint8_t;
#endif							/* HAVE_STDINT_H */
typedef uint8_t sa_family_t;	/* socket address family */
struct sockaddr_un {
	uint8_t sun_len;			/* total sockaddr length */
	sa_family_t sun_family;		/* AF_LOCAL */
	char sun_path[104];			/* path name (gag) */
};
#endif							/* WIN32 */

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 104 /* 108 on Linux, 104 on BSD */
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

static Tipc_bf2bf ibf = { 0, NULL, -1, 0, NULL };

static void
handle_message(const gchar * message, gsize len)
{
	Tbfwin *bfwin;
	if (!main_v->bfwinlist)
		return;

	bfwin = BFWIN(g_list_last(main_v->bfwinlist)->data);
	if (strcmp(message, "openwin") == 0) {
		/* call open new window */
		DEBUG_MSG("open new window\n");
		bfwin = bfwin_window_new();
	} else if (strncmp(message, "openuri", 7) == 0 && len > 12) {
		GFile *file;
		DEBUG_MSG("open URI %s\n", &message[8]);
		file = g_file_new_for_uri(&message[8]);
		file_handle(file, bfwin, NULL, TRUE);
		g_object_unref(file);
		gtk_window_present(GTK_WINDOW(bfwin->main_window));
	} else {
		g_print("unknown message with len %" G_GSIZE_FORMAT " on socket...\n", len);
		/*DEBUG_MSG("message: %s\n",message); */
	}
}

static gboolean
socket_data_in_lcb(GIOChannel * source, GIOCondition condition, gpointer data)
{
	Tconnection *client = data;
	gchar *instring = NULL;
	gsize instringlen = 0, termpos = 0;
	GError *error = NULL;
	GIOStatus status;

	DEBUG_MSG("socket_data_in_lcb, called\n");
	if (data == NULL) {
		client = g_slice_new(Tconnection);
		DEBUG_MSG("socket_data_in_lcb, accept new connection, new client=%p\n", client);
#ifdef WIN32
		client->fd = SOCKET_ERROR;
		while (client->fd == SOCKET_ERROR)
			client->fd = accept(ibf.fd, NULL, NULL);
		client->chan = g_io_channel_win32_new_socket(client->fd);
#else
		socklen_t tmp = 0;

		client->fd = accept(ibf.fd, NULL, &tmp);
		client->chan = g_io_channel_unix_new(client->fd);
#endif
		g_io_channel_set_line_term(client->chan, "\n", 1);
		client->iochan_id = g_io_add_watch(client->chan, G_IO_IN, socket_data_in_lcb, client);
		return TRUE;
	}

	status = g_io_channel_read_line(source, &instring, &instringlen, &termpos, &error);
	DEBUG_MSG("socket_data_in_lcb, status=%d\n", status);
	while (status == G_IO_STATUS_NORMAL && instringlen > 0) {
		DEBUG_MSG("received data instring=%p, instringlen=%zd, termpos=%zd\n", instring, instringlen,
				  termpos);
		instring[termpos] = '\0';
		handle_message(instring, instringlen);
		g_free(instring);
		instring = NULL;
		status = g_io_channel_read_line(source, &instring, &instringlen, &termpos, &error);
		DEBUG_MSG("socket_data_in_lcb, status=%d\n", status);
	}
	if (status == G_IO_STATUS_EOF) {
		DEBUG_MSG("socket_data_in_lcb, EOF, shutdown channel\n");
		g_io_channel_shutdown(client->chan, FALSE, NULL);
		g_io_channel_unref(client->chan);
#ifdef WIN32
		closesocket(client->fd);
#else
		close(client->fd);
#endif
/*		client->chan_id = 0;
		client->chan = NULL;
*/
		DEBUG_MSG("free client %p\n", client);
		g_slice_free(Tconnection, client);
		return FALSE;
	}
	return TRUE;
}

static gboolean
socket_is_valid(const char *path)
{
#ifndef WIN32 /* Just return true for windows */
	struct stat sbuf;

	if (stat(path, &sbuf) == -1)
		return FALSE;
	if (sbuf.st_uid != geteuid())
		return FALSE;
#ifdef S_IFSOCK
	if ((sbuf.st_mode & S_IFSOCK) != S_IFSOCK)
		return FALSE;
#endif
	/* check permissions ? */
	DEBUG_MSG("socket %s is valid\n", path);
#endif
	return TRUE;
}

static char *
socket_filename(void)
{
	gchar *path, *newfile;
#if defined MAC_INTEGRATION || defined WIN32
	newfile = g_strdup_printf(PACKAGE_NAME "-%s", g_get_user_name());
	path = g_build_filename(g_get_tmp_dir(), newfile, NULL);
#else
	gchar *display;
	/* the documentation is unclear if we can free the returned string from gdk_get_display()
		it seems we can, but OSX users report crashes in ipc_bf2bf_start() */
	display = gdk_get_display();
	newfile = g_strdup_printf(PACKAGE_NAME "-%s-%s", g_get_user_name(), display);
	path = g_build_filename(g_get_tmp_dir(), newfile, NULL);
	g_free(display);
#endif
	g_free(newfile);
	return path;
}

static void
send_filename(GFile * file)
{
	gchar *uri, *data;

	uri = g_file_get_uri(file);
	data = g_strdup_printf("openuri %s\n", uri);
	g_io_channel_write_chars(ibf.chan, data, strlen(data), NULL, NULL);
	g_io_channel_flush(ibf.chan, NULL);
	g_free(uri);
	g_free(data);
}

static void
send_openwin(void)
{
	gchar *data = g_strdup("openwin\n");
	g_io_channel_write_chars(ibf.chan, data, strlen(data), NULL, NULL);
	g_io_channel_flush(ibf.chan, NULL);
	g_free(data);
}

static gboolean
become_server(void)
{
#ifdef WIN32
	struct sockaddr_in iaddr;

	iaddr.sin_family = AF_INET;
	iaddr.sin_port = htons(5150);
	iaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	ibf.fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (bind(ibf.fd, (struct sockaddr *) &iaddr, sizeof(iaddr)) == SOCKET_ERROR) {
		ibf.fd = -1;
		return FALSE;
	}

	listen(ibf.fd, 10);
	ibf.chan = g_io_channel_win32_new_socket(ibf.fd);
#else
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
	/*g_io_channel_set_line_term(ibf.chan, "\n", 1); */
#endif

	ibf.iochan_id = g_io_add_watch(ibf.chan, G_IO_IN, socket_data_in_lcb, NULL);
	ibf.master = TRUE;
	return TRUE;
}

static gboolean
become_client(void)
{
#ifdef WIN32
	struct sockaddr_in iaddr;

	iaddr.sin_family = AF_INET;
	iaddr.sin_port = htons(5150);
	iaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	ibf.fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connect(ibf.fd, (struct sockaddr *) &iaddr, sizeof(iaddr)) == SOCKET_ERROR) {
		ibf.fd = -1;
		DEBUG_MSG("become_client, could not connect to socket, errno=%d: %s\n", errno, strerror(errno));
		return FALSE;
	}

	ibf.chan = g_io_channel_win32_new_socket(ibf.fd);
#else
	struct sockaddr_un uaddr;

	uaddr.sun_family = AF_UNIX;
	strncpy(uaddr.sun_path, ibf.path, MIN(strlen(ibf.path) + 1, UNIX_PATH_MAX));
	ibf.fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (connect(ibf.fd, (struct sockaddr *) &uaddr, sizeof(uaddr)) == -1) {
		ibf.fd = -1;
		DEBUG_MSG("become_client, could not connect to socket, errno=%d: %s\n", errno, strerror(errno));
		return FALSE;
	}
	ibf.chan = g_io_channel_unix_new(ibf.fd);
#endif

	ibf.master = FALSE;
	return TRUE;
}

/* filenames is a list of GFile * */
gboolean
ipc_bf2bf_start(GList * filenames, gboolean new_window)
{
	ibf.path = socket_filename();
	DEBUG_PATH("create socket %s\n", ibf.path);
	if (socket_is_valid(ibf.path)) {
		if (become_client()) {
			GList *tmplist = g_list_first(filenames);
			DEBUG_MSG("sending files to existing bluefish process... ");
			if (new_window)
				send_openwin();

			/* send all files and exit */
			while (tmplist) {
				send_filename(tmplist->data);
				tmplist = g_list_next(tmplist);
			}
			DEBUG_MSG("done\n");
			return FALSE;
		}
	}
#ifndef WIN32
	unlink(ibf.path);
#endif
	become_server();
	DEBUG_MSG("ipc_bf2bf_start, master=%d\n", ibf.master);
	return TRUE;
}

void
ipc_bf2bf_cleanup(void)
{
	if (ibf.iochan_id) {
		g_source_remove(ibf.iochan_id);
	}
	if (ibf.chan) {
		g_io_channel_shutdown(ibf.chan, FALSE, NULL);
		g_io_channel_unref(ibf.chan);
	}
	if (ibf.path) {
#ifdef WIN32
		closesocket(ibf.fd);
#else
		if (ibf.master) {
			unlink(ibf.path);
		}
		close(ibf.fd);
#endif
		g_free(ibf.path);
	}
}
