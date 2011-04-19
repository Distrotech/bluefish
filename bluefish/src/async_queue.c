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
 
#include "async_queue.h"


/******************************** queue functions ********************************/


void
queue_init(Tqueue * queue, guint max_worknum, gboolean lockmutex, QueueFunc queuefunc)
{
	queue->worknum = 0;
	queue->tail = queue->head = NULL;
	queue->max_worknum = max_worknum;
	queue->queuefunc = queuefunc;
	if (lockmutex)
		g_static_mutex_init(&queue->mutex);
}

void
queue_cleanup(Tqueue * queue)
{
	if (queue->lockmutex)
		g_static_mutex_free(&queue->mutex);
}

static void
queue_run(Tqueue * queue)
{
	/* THE QUEUE MUTEX SHOULD BE LOCKED IF NEEDED WHEN CALLING THIS FUNCTION !!!!!!!!!!!!!!!!!!!!! */
	while (queue->tail != NULL && queue->worknum < queue->max_worknum) {
		GList *curlst = queue->tail;
		queue->tail = curlst->prev;
		if (queue->tail == NULL)
			queue->head = NULL;
		else
			queue->tail->next = NULL;
		queue->queuelen--;
		queue->worknum++;
		queue->queuefunc(curlst->data);
		g_list_free_1(curlst);
		/*g_print("queue_run, %d working, %d queued\n",queue->worknum,g_list_length(queue->head)); */
	}
}

void
queue_worker_ready(Tqueue * queue)
{
	if (queue->lockmutex)
		g_static_mutex_lock(&queue->mutex);
	queue->worknum--;
	queue_run(queue);
	if (queue->lockmutex)
		g_static_mutex_unlock(&queue->mutex);
}

void
queue_push(Tqueue * queue, gpointer item)
{
	if (queue->lockmutex)
		g_static_mutex_lock(&queue->mutex);
	queue->head = g_list_prepend(queue->head, item);
	queue->queuelen++;
	if (queue->tail == NULL)
		queue->tail = queue->head;
	queue_run(queue);
	if (queue->lockmutex)
		g_static_mutex_unlock(&queue->mutex);
}

/* return TRUE if we found the item on the queue, FALSE if we did not find the item */
gboolean
queue_remove(Tqueue * queue, gpointer item)
{
	GList *curlst = g_list_find(queue->head, item);
	if (curlst) {
		if (queue->lockmutex)
			g_static_mutex_lock(&queue->mutex);
		if (curlst == queue->tail)
			queue->tail = curlst->prev;
		queue->head = g_list_remove_link(queue->head, curlst);
		queue->queuelen--;
		g_list_free_1(curlst);
		if (queue->lockmutex)
			g_static_mutex_unlock(&queue->mutex);
		return TRUE;
	}
	return FALSE;
}
