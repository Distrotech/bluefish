/* Bluefish HTML Editor
 * async_queue.c - asynchronous function execution queue 
 *
 * Copyright (C) 2009-2013 Olivier Sessink
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

#if defined(__GNUC__) || (defined(__SUNPRO_C) && __SUNPRO_C > 0x580)
#define DBG_NONE(args...)
 /**/
#else							/* notdef __GNUC__ || __SUNPRO_C */
extern void g_none(char *first, ...);
#define DBG_NONE g_none
#endif
 /**/

#define DBG_THREAD DBG_NONE

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
	queue->threads=NULL;
	queue->lockmutex = lockmutex;
	if (lockmutex)
#if GLIB_CHECK_VERSION(2, 32, 0)
		g_mutex_init(&queue->mutex);
#else
		g_static_mutex_init(&queue->mutex);
#endif
}

void
queue_cleanup(Tasyncqueue * queue)
{
	if (queue->lockmutex)
#if GLIB_CHECK_VERSION(2, 32, 0)
		g_mutex_clear(&queue->mutex);
#else
		g_static_mutex_free(&queue->mutex);
#endif
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
			DEBUG_MSG("queue_run %p, create new thread, worknum now is %d\n",queue,queue->worknum);
			DBG_THREAD("about to start thread, have %d threads in queue->threads\n",g_slist_length(queue->threads));
			/* the old and the new function:
			GThread *g_thread_new (const gchar *name,             GThreadFunc func,              gpointer data);
			GThread *g_thread_create (GThreadFunc func,                 gpointer data,                 gboolean joinable,                 GError **error);
			*/
#if GLIB_CHECK_VERSION(2, 32, 0)
			thread = g_thread_new(NULL, (GThreadFunc)queue->queuefunc, item);
#else
			thread = g_thread_create((GThreadFunc)queue->queuefunc, item, TRUE, NULL);
#endif
			queue->threads = g_slist_append(queue->threads, thread);
			DBG_THREAD("started thread %p, have %d threads in queue->threads\n",thread, g_slist_length(queue->threads));
		} else {
			if (queue->lockmutex) {
#if GLIB_CHECK_VERSION(2, 32, 0)
				DBG_THREAD("unlock queue %p\n",queue);
				g_mutex_unlock(&queue->mutex);
#else
				g_static_mutex_unlock(&queue->mutex);
#endif
			}
			DEBUG_MSG("queue_run %p, calling queuefunc(), worknum now is %d\n",queue,queue->worknum);
			queue->queuefunc(item);
			if (queue->lockmutex) {
#if GLIB_CHECK_VERSION(2, 32, 0)
				g_mutex_lock(&queue->mutex);
				DBG_THREAD("lock queue %p\n",queue);
#else
				g_static_mutex_lock(&queue->mutex);
#endif
			}
		}
		startednew=TRUE;
	}
	return startednew;
}

gboolean
queue_worker_ready(Tasyncqueue * queue)
{
	gboolean startednew;
	if (queue->lockmutex) {
#if GLIB_CHECK_VERSION(2, 32, 0)
		g_mutex_lock(&queue->mutex);
		DBG_THREAD("lock queue %p\n",queue);
#else
		g_static_mutex_lock(&queue->mutex);
#endif
	}
	queue->worknum--;
	DEBUG_MSG("queue_worker_ready %p, len=%d, worknum=%d\n",queue,queue->q.length, queue->worknum);
	startednew = queue_run(queue);
	if (queue->lockmutex) {
#if GLIB_CHECK_VERSION(2, 32, 0)
		DBG_THREAD("unlock queue %p\n",queue);
		g_mutex_unlock(&queue->mutex);
#else
		g_static_mutex_unlock(&queue->mutex);
#endif
	}
	return startednew;
}

void
queue_worker_ready_inthread(Tasyncqueue *queue)
{
	gpointer item;

#if GLIB_CHECK_VERSION(2, 32, 0)
	g_mutex_lock(&queue->mutex);
	DBG_THREAD("lock queue %p\n",queue);
#else
	g_static_mutex_lock(&queue->mutex);
#endif
	
	if (!queue->q.tail) {
		gpointer thisthread = g_thread_self();
		queue->worknum--;
		DEBUG_MSG("queue_worker_ready_inthread queue=%p, queue length %d, just return (end thread, worknum=%d)\n",queue,g_queue_get_length(&queue->q),queue->worknum);
		queue->threads = g_slist_remove(queue->threads, thisthread);
		DBG_THREAD("removed thread self %p, have %d threads on queue->threads, will unref it\n",thisthread,g_slist_length(queue->threads));
#if GLIB_CHECK_VERSION(2, 32, 0)
		g_thread_unref(thisthread);
		DBG_THREAD("unlock queue %p\n",queue);
		g_mutex_unlock(&queue->mutex);
#else
		g_static_mutex_unlock(&queue->mutex);
#endif
		return;
	}
	item = g_queue_pop_tail(&queue->q);
	DEBUG_MSG("queue_worker_ready_inthread queue=%p, queue length=%d, worknum=%d\n",queue,g_queue_get_length(&queue->q), queue->worknum);
#if GLIB_CHECK_VERSION(2, 32, 0)
	DBG_THREAD("unlock queue %p\n",queue);
	g_mutex_unlock(&queue->mutex);
#else
	g_static_mutex_unlock(&queue->mutex);
#endif
	queue->queuefunc(item);
}

void
queue_push(Tasyncqueue * queue, gpointer item)
{
	if (queue->lockmutex) {
#if GLIB_CHECK_VERSION(2, 32, 0)
		g_mutex_lock(&queue->mutex);
		DBG_THREAD("lock queue %p\n",queue);
#else
		g_static_mutex_lock(&queue->mutex);
#endif
	}
	g_queue_push_head(&queue->q, item);
	DEBUG_MSG("queue_push %p, new queue length=%d, worknum=%d\n",queue,queue->q.length, queue->worknum);
	queue_run(queue);
	if (queue->lockmutex) {
#if GLIB_CHECK_VERSION(2, 32, 0)
		DBG_THREAD("unlock queue %p\n",queue);
		g_mutex_unlock(&queue->mutex);
#else
		g_static_mutex_unlock(&queue->mutex);
#endif
	}
}

/* return TRUE if we found the item on the queue, FALSE if we did not find the item */
gboolean
queue_remove(Tasyncqueue * queue, gpointer item)
{
	gboolean retval;
	gint len;
	if (queue->lockmutex) {
#if GLIB_CHECK_VERSION(2, 32, 0)
		g_mutex_lock(&queue->mutex);
		DBG_THREAD("lock queue %p\n",queue);
#else
		g_static_mutex_lock(&queue->mutex);
#endif
	}
	len = queue->q.length;
	g_queue_remove(&queue->q, item);
	retval = (len<queue->q.length);
	if (queue->lockmutex) {
#if GLIB_CHECK_VERSION(2, 32, 0)
		DBG_THREAD("unlock queue %p\n",queue);
		g_mutex_unlock(&queue->mutex);
#else
		g_static_mutex_unlock(&queue->mutex);
#endif
	}
	return retval;
}

void
queue_cancel(Tasyncqueue *queue, GFunc freefunc, gpointer user_data)
{
	if (queue->lockmutex) {
#if GLIB_CHECK_VERSION(2, 32, 0)
		g_mutex_lock(&queue->mutex);
		DBG_THREAD("lock queue %p\n",queue);
#else
		g_static_mutex_lock(&queue->mutex);
#endif
	}
	g_queue_foreach(&queue->q, freefunc, user_data);
	g_queue_clear(&queue->q);
	if (queue->lockmutex) {
#if GLIB_CHECK_VERSION(2, 32, 0)
		DBG_THREAD("unlock queue %p\n",queue);
		g_mutex_unlock(&queue->mutex);
#else
		g_static_mutex_unlock(&queue->mutex);
#endif
	}
	if (queue->startinthread) {
		GSList *tmpslist;
		DBG_THREAD("queue_cancel, have %d threads in queue->threads\n",g_slist_length(queue->threads));
		for (tmpslist = queue->threads;tmpslist;tmpslist=g_slist_next(tmpslist)) {
			DBG_THREAD("joining thread %p\n",tmpslist->data);
			g_thread_join(tmpslist->data); /* will unref the thread autyomatically */
		}
		g_slist_free(queue->threads);
		queue->threads = NULL;
	}
}
