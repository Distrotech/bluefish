
typedef struct {
	BF_ELIST_HEAD;
	guint32 pos;
	guint8 is_start;
} Tchange;
#define CHANGE(var) ((Tchange *)var)


typedef struct {
	Tchange *head;
	Tchange *tail;
} Tregions;

static Tchange *
new_change(guint pos, gboolean is_start)
{
	Tchange *change = g_slice_new(Tchange);
	change->next = change->prev = NULL;
	change->pos = pos;
	change->is_start = is_start;
	return change;
}

static Tchange *
insert_start_and_end(Tregions *rg, guint start, guint end)
{
	Tchange *tmp = rg->head;
	/* insert start */
	if (tmp->pos > start) {
		rg->head = bf_elist_prepend(rg->head, new_change(start, TRUE));
		tmp = rg->head;
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
		rg->tail = bf_elist_append(rg->tail, new_change(start, FALSE));
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
mark_region_changed(Tregions *rg, guint start, guint end)
{
	if (!rg->head) {
		rg->head = new_change(start, TRUE);
		rg->tail = new_change(end, FALSE);
		return;
	}

	if (rg->tail->position < start) {
		rg->tail = bf_elist_append(rg->tail, new_change(start, TRUE));
		rg->tail = bf_elist_append(rg->tail, new_change(start, FALSE));
		return;
	} else if (rg->head->position > end){
		rg->head = bf_elist_prepend(rg->head, new_change(start, FALSE));
		rg->head = bf_elist_prepend(rg->head, new_change(start, TRUE));
		return;
	}

	insert_start_and_end(rg, start, end);
}

void mark_region_done(Tregions *rg, guint end)
{
	if (!rg->head) {
		return;
	}
	tmp = rg->head;
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

gpointer
update_offset(Tregions *rg, gpointer cur, guint start , gint offset, guint nextpos)
{
	if (cur == NULL) {
		cur = rg->head;
		while (cur && CHANGE(cur)->pos < start) {
			cur = CHANGE(cur)->next;
		}
	}
	while (cur && CHANGE(cur)->pos+offset < nextpos) {
		CHANGE(cur)->pos += offset;
		cur = CHANGE(cur)->next;
	}
	return cur;
}

