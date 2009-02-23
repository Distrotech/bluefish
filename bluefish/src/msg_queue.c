/* Bluefish HTML Editor
 * msg_queue.c - message queue handling
 *
 * Copyright (C) 2003-2009 Olivier Sessink
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

#include <gtk/gtk.h>

/*#define DEBUG*/

#include "bluefish.h"

#ifdef WITH_MSG_QUEUE

#include <sys/types.h>
#include <sys/ipc.h>			/* msgsnd() */
#include <sys/msg.h>			/* msgsnd() */
#include <time.h>
#include <errno.h>				/* errno */
#include <unistd.h> /* getpid, getuid */
#include <string.h> /* strncpy */
#include <stdlib.h> /* exit() */

#include "stringlist.h"
#include "gtk_easy.h" /* *_dialog */
#include "gui.h" /* notebook_changed() */
#include "document.h"
#include "project.h"
#include "file.h"

#define BLUEFISH_MSG_QUEUE 9723476 /* randomly chosen number, I hope it is not used by other apps */
#define MSQ_QUEUE_SIZE 1024
#define MSQ_QUEUE_SMALL_SIZE 7
#define MSQ_QUEUE_CHECK_TIME 500	/* miliseconds for g_timeout_add()
												this is also the time that another process needs to wait
												before it knows for sure that the master is dead. so this should
												not be too slow, otherwise the other process will be slowed down. */

#define MSG_QUEUE_SEND_ALIVE 46070

#define MSG_QUEUE_OPENFILE_LAST 46050
#define MSG_QUEUE_OPENFILE 46040
/*#define MSG_QUEUE_OPENPROJECT 46030*/
#define MSG_QUEUE_OPENNEWWIN 46020

/* from man msgrcv: 'the first message on the queue with the lowest type less 
 * than or equal to the absolute value of msgtyp will be read'
 * that means the requestalive should have the lowest number, because
 * it has the highest priority
 */
#define MSG_QUEUE_ASK_ALIVE 46010



/* 
the message queue system is quite easy:

if there is no queue --> open it and start listening
on a MSG_QUEUE_OPENFILE, open the file, on a MSG_QUEUE_ASK_ALIVE return
the same data with type MSG_QUEUE_SEND_ALIVE (which we are not
listening for)

if there is a queue and there are files loaded on the commandline --> 
check the server (send a keepalive containing the pid) and send the 
files to the queue, after sending the files check if a 
MSG_QUEUE_SEND_ALIVE is received contaning the pid. If so, there is
a server and we can quit. If not, we continue starting and load them

if there is a queue and we do not have files loaded on the commandline
we just start, but we don't listen to the queue!
*/

typedef struct {
	gboolean functional;
	gboolean server;
	gboolean received_keepalive;
	int msgid;
	GList *file_error_list;
	GTimer *timer;

} Tmsg_queue;

/******************************/
/* global var for this module */
/******************************/
Tmsg_queue msg_queue = { TRUE, FALSE, FALSE, -1, NULL};

/**
 * msg_queue_check_alive:
 *
 * checks the message queue for messages of type MSG_QUEUE_SEND_ALIVE
 * if we receive such a message, there must be another process 
 * active on this message queue
 */
static gboolean msg_queue_check_alive(void)
{
	struct small_msgbuf {
		long mtype;
		char mtext[MSQ_QUEUE_SMALL_SIZE];
	} small_msgp;
	gchar *pid_string = g_strdup_printf("%d", (int) getpid());

	/*if (wait_first) {
		static struct timespec const req = { 0, MSQ_QUEUE_CHECK_TIME * 1000000};
		static struct timespec rem;
		nanosleep(&req, &rem);
	}*/

	while (msgrcv
		   (msg_queue.msgid, &small_msgp, MSQ_QUEUE_SMALL_SIZE * sizeof(char), MSG_QUEUE_SEND_ALIVE,
			IPC_NOWAIT) != -1) {
		DEBUG_MSG("msg_queue_check_alive, received a keepalive, mtext=%s!\n", small_msgp.mtext);
		if (strncmp(pid_string, small_msgp.mtext, MSQ_QUEUE_SMALL_SIZE - 1) == 0) {
			DEBUG_MSG("msg_queue_check_alive, keepalive matches request!\n");
			/* we did receive a keepalive on our request !! */
			g_free(pid_string);
			return TRUE;
		} else {
			DEBUG_MSG("msg_queue_check_alive, keepalive does NOT match request %s\n", pid_string);
		}
	}
	DEBUG_MSG("msg_queue_check_alive, errno=%d, error=%s\n", errno, g_strerror(errno));
	g_free(pid_string);
	return FALSE;
}

/**
 * msg_queue_open:
 *
 *  returns 1 if another process has the queue open already
 *  returns 0 if we opened the queue
 */
static gboolean msg_queue_open(void)
{
	DEBUG_MSG("msg_queue_open, started\n");
	msg_queue.msgid = msgget((key_t) BLUEFISH_MSG_QUEUE + getuid(), 0666 | IPC_CREAT | IPC_EXCL);
#ifdef DEBUG
	if (msg_queue.msgid == -1) {
		DEBUG_MSG("msg_queue_open, errno=%d, error=%s\n", errno, g_strerror(errno));
	}
#endif
	/* if msg_queue.msgid == -1 the message queue was already opened by another process */
	DEBUG_MSG("msg_queue_open, msg_queue.msgid=%d\n", msg_queue.msgid);
	if (msg_queue.msgid == -1) {
		msg_queue.msgid = msgget((key_t) BLUEFISH_MSG_QUEUE + getuid(), 0666);
		DEBUG_MSG("msg_queue_open, connected to existing message queue, id=%d\n", msg_queue.msgid);
		
		/* now we want to avoid the situation where the message queue is full (because the server died)
		so we cannot send a keepalive, so we check if the queue is filled (assume when there are >5 messages)
		and the last completed msgrcv() call was > 5 seconds ago */
		{
			struct msqid_ds msg_stat;
			gint timediff;
			/* check if there are messages on the queue, if so, check when the last msgrcv() call was on this queue */
			msgctl(msg_queue.msgid, IPC_STAT, &msg_stat);
			if (msg_stat.msg_qnum > 5) {
				timediff = time(NULL) - msg_stat.msg_ctime;
				if (timediff > 2) {
					DEBUG_MSG("msg_queue_request_alive, more then 2 seconds no reads on message_queue, timediff=%d, deleting queue\n", timediff);
					msgctl(msg_queue.msgid, IPC_RMID, NULL);
					msg_queue.msgid = msgget((key_t) BLUEFISH_MSG_QUEUE + getuid(), 0666 | IPC_CREAT | IPC_EXCL);
					return FALSE;
				}
			}
		}
		if (msg_queue.msgid == -1) {
			DEBUG_MSG("msg_queue_open, errno=%d, error=%s\n", errno, g_strerror(errno));
			msg_queue.functional = FALSE;
		}
		return TRUE;
	}
	return FALSE;
}
/**
 * msg_queue_check:
 *
 * checks the queue for any messages
 * this is called by the master program, usually by gtk_timeout_add()
 * the messages it will listen to are the types:
 * - MSG_QUEUE_ASK_ALIVE - we should respond with a type MSG_QUEUE_SEND_ALIVE
 * - MSG_QUEUE_OPENFILE - open a filename
 * - MSG_QUEUE_OPENPROJECT - open a filename as project
 * - MSG_QUEUE_OPENNEWWIN - open a new window
 * - MSG_QUEUE_FINISHED_SENDING - the calling process will stop sending now
 */
static gboolean msg_queue_check(gint started_by_gtk_timeout) {
	struct msgbuf {
		long mtype;
		char mtext[MSQ_QUEUE_SIZE];
	} msgp;
	gint retval;
	gboolean run_again = TRUE;
	Tbfwin *bfwin;	
	
	if (main_v->bfwinlist == NULL || BFWIN(main_v->bfwinlist->data)->documentlist == NULL) {
		DEBUG_MSG("msg_queue_check, no documentlist yet, so we do not continue\n");
		return TRUE;
	}

	if (msg_queue.msgid == -1) {
		return FALSE;
	}
	bfwin = BFWIN(g_list_last(main_v->bfwinlist)->data);
	while (run_again) {
		gboolean is_last = FALSE;
		run_again = FALSE;
		
		/* read all except MSG_QUEUE_SEND_ALIVE */
		retval =	msgrcv(msg_queue.msgid, &msgp, MSQ_QUEUE_SIZE, -MSG_QUEUE_OPENFILE_LAST, IPC_NOWAIT);
		if (retval != -1) {
			DEBUG_MSG("msg_queue_check, found type %ld\n", msgp.mtype);
			switch (msgp.mtype) {
			case MSG_QUEUE_ASK_ALIVE:
				{
				struct small_msgbuf {
					long mtype;
					char mtext[MSQ_QUEUE_SMALL_SIZE];
				} small_msgp;
				DEBUG_MSG("msg_queue_check, a keepalive is asked from %s, sending!\n", msgp.mtext);
				small_msgp.mtype = MSG_QUEUE_SEND_ALIVE;
				strncpy(small_msgp.mtext, msgp.mtext, MSQ_QUEUE_SMALL_SIZE - 1);
				msgsnd(msg_queue.msgid, (void *) &small_msgp, MSQ_QUEUE_SMALL_SIZE * sizeof(char),
					   IPC_NOWAIT);
				}
			break;
			case MSG_QUEUE_OPENFILE_LAST:
				is_last = TRUE;
			/* don't break, we have to open file file */
			case MSG_QUEUE_OPENFILE: {
				GFile *file;
				DEBUG_MSG("msg_queue_check, a filename %s is received, is_last=%d\n", msgp.mtext,is_last);
				file = g_file_new_for_commandline_arg(msgp.mtext);
				/*doc_new_from_input(bfwin, msgp.mtext, !is_last, FALSE, -1);*/
				file_handle(file, bfwin);
				g_object_unref(file);
				run_again = TRUE;	/* call myself again, there may have been multiple files */
			} break;
/*			case MSG_QUEUE_OPENPROJECT:
				{
					GFile *uri;
					DEBUG_MSG("msg_queue_check, a project %s is received\n", msgp.mtext);
					uri = g_file_new_for_commandline_arg(msgp.mtext);
					project_open_from_file(bfwin, uri);
					g_object_unref(uri);
					run_again = TRUE;	/ * call myself again, there may have been multiple projects * /
				}
			break;*/
			case MSG_QUEUE_OPENNEWWIN:
				{
					/* now check if this is indeed send by another process
					if the message queue was dead during the startup of this process,
					it might be started by this very process */
					int otherpid = atoi(msgp.mtext);
					DEBUG_MSG("msg_queue_check, a new window is requested by PID=%d\n",otherpid);
					if (otherpid != (int) getpid()) {
						DEBUG_MSG("msg_queue_check, the PID is not ours, opening new window\n");
						bfwin = gui_new_window(NULL);
					}
				}
			break;
	#ifdef DEBUG
			default:
				DEBUG_MSG("msg_queue_check, unknown message queue type %ld\n", msgp.mtype);
			break;
	#endif
			}
		} else {
			if (errno == 22 || errno == 43) { /* 43 = Identifier removed */
				DEBUG_MSG("msg_queue_check, re-opening message queue ?!?!?\n");
				/* the msg_queue was removed !?!?! */
				if (msg_queue_open()) {
					DEBUG_MSG("msg_queue_check, another process has opened the message_queue, stopping server\n");
					msg_queue.server = FALSE;
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

/**
 * msg_queue_send_names:
 *
 *
 * returns FALSE if we never received a keepalive, so the server process seems to be non-responsive
 */
static gboolean msg_queue_send_names(gint send_with_id, GList * names, gboolean received_keepalive)
{
	struct msgbuf {
		long mtype;
		char mtext[MSQ_QUEUE_SIZE];
	} msgp;
	
	gint success = 1, check_keepalive_cnt = 0, send_failure_cnt = 0;
	GList *tmplist;
	gint num_files_sent = 0;

	/* we have a message queue now, opened by another bluefish process */
	msgp.mtype = send_with_id;
	 
	
	tmplist = g_list_first(names);
	while (tmplist && success) {
		gchar *curi;
		gint retval;
		gint len;
		
		curi = g_file_get_uri((GFile *)tmplist->data);
		len = strlen(curi);
		/* we start with checking for keepalives */
		if (!received_keepalive) {
			if (msg_queue_check_alive()) {
				received_keepalive = TRUE;
				DEBUG_MSG("msg_queue_send_files, received keepalive\n");
			} else {
				check_keepalive_cnt++;
				DEBUG_MSG("msg_queue_send_files, no keepalive (try %d)\n", check_keepalive_cnt);
			}
		}
		
		if (len < MSQ_QUEUE_SIZE - 1) {
			strncpy(msgp.mtext, curi, MSQ_QUEUE_SIZE - 1);
			
			/* this is a bit of specific code for sending files, to notify the server process that the last file is coming */
			if (tmplist->next == NULL && msgp.mtype == MSG_QUEUE_OPENFILE) {
				DEBUG_MSG("msg_queue_send_files, %s is the last file!\n",msgp.mtext);
				msgp.mtype = MSG_QUEUE_OPENFILE_LAST;
			}
			retval =	msgsnd(msg_queue.msgid, (void *) &msgp, MSQ_QUEUE_SIZE * sizeof(char), IPC_NOWAIT);
			if (retval == -1) {
				if (errno == EAGAIN) { /* EAGAIN = 11 */
					struct timespec req = { 0, 40000000	/* nanoseconds */};
					static struct timespec rem;
					if (received_keepalive && num_files_sent > 10) 
						req.tv_nsec = (5*req.tv_nsec);
					else if (num_files_sent > 10 ) req.tv_nsec = (2*req.tv_nsec);
					DEBUG_MSG("msg_queue_send_files, failed sending with EAGAIN, sleep %f\n", ((gdouble)req.tv_nsec)/1000000000.0);
					nanosleep(&req, &rem);
					send_failure_cnt++;
				} else {
					DEBUG_MSG("msg_queue_send_files, failing to send, errno=%d, aborting\n", errno);
					success = 0;
				}
			} else {
				if (!received_keepalive) {
					/* if we fill the message queue with loads of data, the server 
					   process doesn't even get a chance of reply-ing. So as long as we 
					   don't know a thing about it, we give it some time and check for
					   a reply often */
					if (msg_queue_check_alive()) {
						received_keepalive = TRUE;
						DEBUG_MSG("msg_queue_send_files, received keepalive\n");
					} else {
						check_keepalive_cnt++;
						DEBUG_MSG("msg_queue_send_files, no keepalive (try %d)\n", check_keepalive_cnt);
					}
				}
				DEBUG_MSG("msg_queue_send_files, sending %s succeeded\n", curi);
				send_failure_cnt = 0;
				tmplist = g_list_next(tmplist);
			}
		} else {
			DEBUG_MSG("msg_queue_send_files, failed sending, length increased message size\n");
			success = 0;
		}
		if ((check_keepalive_cnt > 500) || (send_failure_cnt > 60)) {
			DEBUG_MSG
				("msg_queue_send_files, too many tries, check_keepalive_cnt=%d, send_failure_cnt=%d\n",
				 check_keepalive_cnt, send_failure_cnt);
			success = 0;
		}
		g_free(curi);
		num_files_sent++;
	}
	if (success) {
		/* send a signal that we are finished sending */
		DEBUG_MSG
			("msg_queue_send_files, sending filenames complete and successfull, received_keepalive=%d\n",
			 received_keepalive);
		return received_keepalive;
	}
	return FALSE;
}

/**
 * msg_queue_request_alive:
 *
 * sends a message of type MSG_QUEUE_ASK_ALIVE to the already existing queue
 * to check if the queue is alive
 */
static void msg_queue_request_alive(void)
{
	gboolean ask_alive;
	struct small_msgbuf {
		long mtype;
		char mtext[MSQ_QUEUE_SMALL_SIZE];
	} small_msgp;
	gchar *pid_string = g_strdup_printf("%d", (int) getpid());

	DEBUG_MSG("msg_queue_request_alive, asking for keepalive, string %s\n", pid_string);
	small_msgp.mtype = MSG_QUEUE_ASK_ALIVE;
	strncpy(small_msgp.mtext, pid_string, MSQ_QUEUE_SMALL_SIZE - 1);
	ask_alive =
		msgsnd(msg_queue.msgid, (void *) &small_msgp, MSQ_QUEUE_SMALL_SIZE * sizeof(char),
			   IPC_NOWAIT);
	g_free(pid_string);
	if (ask_alive == -1) {
		if (errno == 11) {
			/* the resource is temporary unavailable - perhaps the queue is full, this could mean a very busy
			message queue or a dead server */
			struct msqid_ds msg_stat;
			gint timediff;

			/* check the last time a process listened to the queue */
			msgctl(msg_queue.msgid, IPC_STAT, &msg_stat);
			timediff = time(NULL) - msg_stat.msg_rtime;
			if (timediff > 2) {
				DEBUG_MSG("msg_queue_request_alive, more then 2 seconds no reads on message_queue, timediff=%d, deleting queue\n", timediff);
				
			}
		}
		DEBUG_MSG("msg_queue_request_alive, errno=%d, error=%s\n", errno, g_strerror(errno));
		msg_queue.functional = FALSE;
	}
}

static void msg_queue_send_new_window(void) {
	int retval;
	struct msgbuf {
		long mtype;
		char mtext[MSQ_QUEUE_SMALL_SIZE];
	} small_msgp;
	/* perhaps we should first check if the queue is alive */
	gchar *pid_string = g_strdup_printf("%d", (int) getpid());
	DEBUG_MSG("msg_queue_send_new_window, requesting new window using our PID %s!\n",pid_string);
	small_msgp.mtype = MSG_QUEUE_OPENNEWWIN;
	strncpy(small_msgp.mtext, pid_string, MSQ_QUEUE_SMALL_SIZE - 1);
	retval = msgsnd(msg_queue.msgid,(void *) &small_msgp, MSQ_QUEUE_SMALL_SIZE * sizeof(char),IPC_NOWAIT);
	if (retval == -1) {
		/* hmm an error, we have to do some error handling here */
	}
}

static void msg_queue_become_server(void) {
	msg_queue.server = TRUE;
	DEBUG_MSG("msg_queue_become_server, we will be server!\n");
	g_timeout_add(MSQ_QUEUE_CHECK_TIME, (GSourceFunc)msg_queue_check, GINT_TO_POINTER(1));
}

/*
	static struct timespec const req = { 0, 200000000};
	static struct timespec rem;
	nanosleep(&req, &rem);
*/
void msg_queue_start(GList * filenames, gboolean open_new_window) {
	gboolean queue_already_open;
	msg_queue.timer = g_timer_new();
	DEBUG_MSG("msg_queue_start, open message queue\n");
	queue_already_open = msg_queue_open();
	if (queue_already_open && msg_queue.functional) {
		msg_queue_request_alive();
		if (open_new_window) {
			msg_queue_send_new_window();
		}
		/* if we have filenames to open, we start sending them now, else we just check if we have to be master or not */
		if (filenames) {
			msg_queue.received_keepalive = msg_queue_send_names(MSG_QUEUE_OPENFILE, filenames, msg_queue.received_keepalive);
			DEBUG_MSG("msg_queue_start, after sending files and projects, keepalive=%d\n",msg_queue.received_keepalive);
			if (msg_queue.received_keepalive) {
				DEBUG_MSG("msg_queue_start, we did send all our messages to an active queue, exiting!\n");
				exit(0);
			}
		}
		
/*		if (!received_keepalive) {
			gint check_keepalive_cnt = 0;
			/ * if the message queue is still open and the process listening is killed
			   we should be the server process --> we have to check if the process is still running * /
			while (!msg_queue.received_keepalive && check_keepalive_cnt < 10) {
				DEBUG_MSG("msg_queue_start, no keepalive yet, check_keepalive_cnt=%d\n", check_keepalive_cnt);
				if (msg_queue_check_alive(TRUE)) {
					msg_queue.received_keepalive = TRUE;
				}
				check_keepalive_cnt++;
			}
			if ((filenames || 
			open_new_window) && msg_queue.received_keepalive) {
				DEBUG_MSG("msg_queue_start, we did send all our messages to an active queue, exiting!\n");
				exit(0);
			}
		} else {
			DEBUG_MSG("msg_queue_start, we did send all our messages to an active queue, exiting!\n");
			exit(0);
		}*/
	}
	
	/* if we opened the queue */
	if (msg_queue.functional && !queue_already_open) {
		msg_queue_become_server();
	}
}

void msg_queue_check_server(gboolean last_check) {
	if (msg_queue.functional && !msg_queue.server && !msg_queue.received_keepalive) {
		if (last_check) {
			gdouble remainder = (((gdouble)MSQ_QUEUE_CHECK_TIME)/1000.0) - g_timer_elapsed(msg_queue.timer,NULL);
			DEBUG_MSG("start to here took %f seconds\n",g_timer_elapsed(msg_queue.timer,NULL));  
			if (remainder > 0) {
				/* wait for the remaining time */
				struct timespec rem;
				struct timespec req = { 0, (int)(1000000000.0 * remainder)};			
				g_print("msg_queue_check_server, wait for %f seconds, %ld nanoseconds\n",remainder,req.tv_nsec);
				nanosleep(&req, &rem);
			}
		}
		msg_queue.received_keepalive = msg_queue_check_alive();
		if (msg_queue.received_keepalive) {
			DEBUG_MSG("msg_queue_check_server, we did send all our messages to an active queue, exiting!\n");
			exit(0);
		} else if (last_check) { /* still no keepalive on last_check */
			msg_queue_become_server();
		}
	}
	if (last_check) {
		g_timer_destroy(msg_queue.timer);
	}
}

void msg_queue_cleanup(void)
{
	if (msg_queue.functional && msg_queue.server) {
		DEBUG_MSG("msg_queue_cleanup, removing msg_queue()\n");
		msgctl(msg_queue.msgid, IPC_RMID, NULL);
	}
}

#endif							/* WITH_MSG_QUEUE */
