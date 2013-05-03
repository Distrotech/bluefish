
typedef struct {
	BF_ELIST_HEAD;
	guint32 pos;
	guint8 is_start;
} Tchange;

typedef struct {

	Tchange *head;
	Tchange *tail;
}

Tchange *new_change(guint pos, gboolean is_start) {


}

Tchange *
insert_start_and_end(guint start, guint end)
{
	Tchange *tmp = btv->head;
	/* insert start */
	if (tmp->pos > start) {
		btv->head = bf_elist_prepend(btv->head, new_change(start, TRUE));
		tmp = btv->head;
	} else {
		while (tmp && tmp->pos < start) {
			tmp = tmp->next;
		}
		/* if tmp->isstart == FALSE we don't need to insert a new start, because 
		the previous start is valid */
		if (tmp->isstart == TRUE) {
			tmp = bf_elist_prepend(tmp, new_change(start, TRUE));
		} else {
			tmp = tmp->prev;
		}
	}
	/* tmp now points to the start position, continue to the end position */
	while (tmp && tmp->pos < end) {
		Tchange *toremove = tmp;
		tmp = bf_elist_remove(toremove); /* returns the previous entry */
		g_slice_free(Tchange, toremove);
		tmp = tmp->next;
	}
	if (!tmp) {
		btv->tail = bf_elist_append(btv->tail, new_change(start, FALSE));
		return;
	}

	/* if tmp->isstart == FALSE we do not have to do anything:
		the already existing region ends on or beyond the region we are ending now, so use 
		the existing end and merge them together */
	if (tmp->issstart == TRUE) {
		if (tmp->pos == end) {
			/* the end of the current region starts the next region, simply remove 
			the start of the next region so they form a continuous region */
			bf_elist_remove(tmp);
		} else {
			bf_elist_prepend(tmp, new_change(start, FALSE));
		}
	} 
}

void
mark_region_changed(guint start, guint end)
{
	if (!btv->head) {
		btv->head = new_change(start, TRUE);
		btv->tail = new_change(end, FALSE);
		return;
	}

	if (btv->tail->position < start) {
		btv->tail = bf_elist_append(btv->tail, new_change(start, TRUE));
		btv->tail = bf_elist_append(btv->tail, new_change(start, FALSE));
		return;
	} else if (btv->head->position > end){
		btv->head = bf_elist_prepend(btv->head, new_change(start, FALSE));
		btv->head = bf_elist_prepend(btv->head, new_change(start, TRUE));
		return;
	}
	
	insert_start_and_end(start, end);
}

void mark_region_done(guint end)
{
	if (!btv->head) {
		return;
	}
	
	while (tmp && tmp->pos < end) {
		tmp = bf_elist_remove(tmp); /* returns the previous entry if that exists, but 
										it does not exist in this case because we remove all entries */
	}
	if (tmp->isstart == FALSE) {
		if (tmp->pos == end) {
			bf_elist_remove(tmp);
		} else {
			bf_elist_prepend(tmp, new_change(end, TRUE));
		}
	}
}

void update_offset(guint pos , gint offset) {
	
	
}

