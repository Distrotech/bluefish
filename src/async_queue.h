/* Bluefish HTML Editor
 * async_queue.c - asynchronous function execution queue 
 *
 * Copyright (C) 2009-2011 Olivier Sessink
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

#ifndef __ASYNC_QUEUE_H_
#define __ASYNC_QUEUE_H_

#include "glib.h"

typedef void (*QueueFunc) (gpointer data); 

typedef struct {
	QueueFunc queuefunc;
	GList *head;				/* data structures that are *not* being worked on */
	GList *tail;
	GStaticMutex mutex;
	gboolean lockmutex; /* whether or not to lock the mutex (if used from threads) */
	guint queuelen;
	guint worknum;				/* number of elements that are being worked on */
	guint max_worknum;
} Tqueue;

void queue_init(Tqueue * queue, guint max_worknum, gboolean lockmutex, QueueFunc queuefunc);
void queue_cleanup(Tqueue * queue);
void queue_worker_ready(Tqueue * queue);
void queue_push(Tqueue * queue, gpointer item);
gboolean queue_remove(Tqueue * queue, gpointer item);

#endif /* ASYNC_QUEUE */
