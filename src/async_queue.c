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

/*#define DEBUG*/

#include "bluefish.h"
#include "async_queue.h"


/******************************** queue functions ********************************/


void
queue_init_full(Tasyncqueue * queue, guint max_worknum, gboolean lockmutex, gboolean startinthread, QueueFunc queuefunc)
{
	queue->q.head=queue->q.tail=NULL;
	queue->q.length=0;
	queue->worknum = 0;
	queue->max_worknum = max_worknum;
	queue->queuefunc = queuefunc;
	queue->startinthread=startinthread;
	if (lockmutex)
		g_static_mutex_init(&queue->mutex);
}

void
queue_cleanup(Tasyncqueue * queue)
{
	if (queue->lockmutex)
		g_static_mutex_free(&queue->mutex);
}

static gboolean
queue_run(Tasyncqueue * queue)
{
	gboolean startednew=FALSE;
	/* THE QUEUE MUTEX SHOULD BE LOCKED IF NEEDED WHEN CALLING THIS FUNCTION !!!!!!!!!!!!!!!!!!!!! */
	DEBUG_MSG("queue_run %p, length=%d, worknum=%d, max_worknum=%d\n",queue, queue->q.length, queue->worknum, queue->max_worknum);
	while (queue->q.length > 0 && queue->worknum < queue->max_worknum) {
		gpointer item;
		
		item = g_queue_pop_tail(&queue->q);
		queue->worknum++;
		if (queue->startinthread) {
			GThread *thread;
			GError *gerror=NULL;
			DEBUG_MSG("queue_run %p, create new thread, worknum now is %d\n",queue,queue->worknum);
			thread = g_thread_create((GThreadFunc)queue->queuefunc, item, FALSE, &gerror);
			queue->threads = g_slist_append(queue->threads, thread);
		} else {
			if (queue->lockmutex)
				g_static_mutex_unlock(&queue->mutex);
			DEBUG_MSG("queue_run %p, calling queuefunc(), worknum now is %d\n",queue,queue->worknum);
			queue->queuefunc(item);
			if (queue->lockmutex)
				g_static_mutex_lock(&queue->mutex);
		}
		startednew=TRUE;
	}
	return startednew;
}

gboolean
queue_worker_ready(Tasyncqueue * queue)
{
	gboolean startednew;
	if (queue->lockmutex)
		g_static_mutex_lock(&queue->mutex);
	queue->worknum--;
	DEBUG_MSG("queue_worker_ready %p, len=%d, worknum=%d\n",queue,queue->q.length, queue->worknum);
	startednew = queue_run(queue);
	if (queue->lockmutex)
		g_static_mutex_unlock(&queue->mutex);
	return startednew;
}

void
queue_worker_ready_inthread(Tasyncqueue *queue)
{
	gpointer item;
	g_static_mutex_lock(&queue->mutex);
	
	if (!queue->q.tail) {
		queue->worknum--;
		DEBUG_MSG("queue_worker_ready_inthread %p, queue length %d, just return (end thread, worknum=%d)\n",queue,g_queue_get_length(&queue->q),queue->worknum);
		queue->threads = g_slist_remove(queue->threads, g_thread_self());
		g_static_mutex_unlock(&queue->mutex);
		return;
	}
	item = g_queue_pop_tail(&queue->q);
	DEBUG_MSG("queue_worker_ready_inthread %p, queue length=%d, worknum=%d\n",queue,g_queue_get_length(&queue->q), queue->worknum);
	g_static_mutex_unlock(&queue->mutex);
	queue->queuefunc(item);
}

void
queue_push(Tasyncqueue * queue, gpointer item)
{
	if (queue->lockmutex)
		g_static_mutex_lock(&queue->mutex);
	g_queue_push_head(&queue->q, item);
	DEBUG_MSG("queue_push %p, new queue length=%d, worknum=%d\n",queue,queue->q.length, queue->worknum);
	queue_run(queue);
	if (queue->lockmutex)
		g_static_mutex_unlock(&queue->mutex);
}

/* return TRUE if we found the item on the queue, FALSE if we did not find the item */
gboolean
queue_remove(Tasyncqueue * queue, gpointer item)
{
	gboolean retval;
	gint len;
	if (queue->lockmutex)
		g_static_mutex_lock(&queue->mutex);
	len = queue->q.length;
	g_queue_remove(&queue->q, item);
	retval = (len<queue->q.length);
	if (queue->lockmutex)
		g_static_mutex_unlock(&queue->mutex);
	return retval;
}

void
queue_cancel(Tasyncqueue *queue, GFunc freefunc, gpointer user_data)
{
	if (queue->lockmutex)
		g_static_mutex_lock(&queue->mutex);	
	g_queue_foreach(&queue->q, freefunc, user_data);
	g_queue_clear(&queue->q);
	if (queue->lockmutex)
		g_static_mutex_unlock(&queue->mutex);
	if (queue->startinthread) {
		GSList *tmpslist;
		for (tmpslist = queue->threads;tmpslist;tmpslist=g_slist_next(tmpslist)) {
			g_thread_join(tmpslist->data);
		}
	}
}
