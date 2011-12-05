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
	GSList *threads;	/* currently active threads */
	QueueFunc queuefunc;	
	GQueue q;				/* data structures that are *not* being worked on */
	GStaticMutex mutex;
	gboolean lockmutex; /* whether or not to lock the mutex (if used from threads) */
	gboolean startinthread;
	guint worknum;				/* number of elements that are being worked on */
	guint max_worknum;
} Tasyncqueue;

void queue_init_full(Tasyncqueue *queue, guint max_worknum, gboolean lockmutex, gboolean startinthread, QueueFunc queuefunc);

#define queue_init(queue, max_worknum, queuefunc) queue_init_full(queue, max_worknum, FALSE, FALSE, queuefunc)

void queue_cleanup(Tasyncqueue * queue);
gboolean queue_worker_ready(Tasyncqueue * queue);
void queue_worker_ready_inthread(Tasyncqueue *queue);
void queue_push(Tasyncqueue * queue, gpointer item);
gboolean queue_remove(Tasyncqueue * queue, gpointer item);
void queue_cancel(Tasyncqueue *queue, GFunc freefunc, gpointer user_data);
#endif /* ASYNC_QUEUE */
