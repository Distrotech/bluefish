/* Bluefish HTML Editor
 *
 * undo_redo.c - imrpoved undo/redo functionality
 * Copyright (C) 2001-2002 Olivier Sessink
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <gtk/gtk.h>
#include <string.h>

#include "bluefish.h" 
#include "undo_redo.h"
#include "document.h" /* doc_bind_signals() */

typedef struct {
	char *text;			/* text to be inserted or deleted */
	int start;			/* starts at this position */
	int end;				/* ends at this position */
	undo_op_t op;		/* action to execute */	
} unreentry_t;

static unregroup_t *unregroup_new(Tdocument *doc) {
	unregroup_t *newgroup;
	
	newgroup = g_malloc(sizeof(unregroup_t));
	newgroup->changed = doc->modified;
	newgroup->entries = NULL;
	DEBUG_MSG("unregroup_new, at %p with modified=%d\n", newgroup, newgroup->changed);
	return newgroup;
}

static void unreentry_destroy(unreentry_t *remove_entry) {
	if (remove_entry->text) {
		g_free(remove_entry->text);
	}
	g_free(remove_entry);
}

static void unregroup_destroy(unregroup_t *to_remove) {
	GList *tmplist;
	
	tmplist = g_list_first(to_remove->entries);
	while (tmplist) {
		unreentry_destroy((unreentry_t *)tmplist->data);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(tmplist);
	g_free(to_remove);
}

static void doc_unre_destroy_last_group(Tdocument *doc) {
	DEBUG_MSG("doc_unre_destroy_last_group, called, last=%p\n", doc->unre.last);
	if (doc->unre.last) {
		unregroup_t *to_remove = doc->unre.last->data;
		doc->unre.last = g_list_previous(doc->unre.last);
		g_list_remove (doc->unre.last, to_remove);
		doc->unre.num_groups--;
		unregroup_destroy(to_remove);
	}
}

static void unregroup_activate(unregroup_t *curgroup, Tdocument *doc, gint is_redo) {
	GList *tmplist;

	if (is_redo) {
		tmplist = g_list_last(curgroup->entries);
	} else {
		tmplist = g_list_first(curgroup->entries);
	}
	while (tmplist) {
		GtkTextIter itstart;
		unreentry_t *entry = tmplist->data;
		if ((entry->op == UndoInsert && !is_redo) || (entry->op == UndoDelete && is_redo)) {
			GtkTextIter itend;
			gtk_text_buffer_get_iter_at_offset(doc->buffer,&itstart,entry->start);
			gtk_text_buffer_get_iter_at_offset(doc->buffer,&itend,entry->end);
			gtk_text_buffer_delete(doc->buffer,&itstart,&itend);
		} else {
			gtk_text_buffer_get_iter_at_offset(doc->buffer,&itstart,entry->start);
			gtk_text_buffer_insert(doc->buffer,&itstart,entry->text,-1);
		}
		if (is_redo) {
			tmplist = g_list_previous(tmplist);
		} else {
			tmplist = g_list_next(tmplist);
		}
	}
	if (is_redo) {
		doc_set_modified(doc, 1);
	} else {
		DEBUG_MSG("unregroup_activate, calling set modified with %d\n", curgroup->changed);
		doc_set_modified(doc, curgroup->changed);
	}
}


static unreentry_t *unreentry_new(const char *text, int start, int end, undo_op_t op) {
	unreentry_t *new_entry;
	new_entry = g_malloc(sizeof(unreentry_t));
	DEBUG_MSG("unreentry_new, for text='%s'\n", text);
	new_entry->text = g_strdup(text);
	new_entry->start = start;
	new_entry->end = end;
	new_entry->op = op;
	return new_entry;
}

static void unre_list_cleanup(GList **list) {
	
	if (list && *list) {
		GList *tmplist;
		tmplist = g_list_first(*list);
		while (tmplist) {
			unregroup_destroy(tmplist->data);
			tmplist = g_list_next(tmplist);
		}
		g_list_free(*list);
		*list = NULL;
	}
}

static void doc_undo(Tdocument *doc) {
	unregroup_t *curgroup = NULL;
	if (g_list_length(doc->unre.current->entries) > 0) {
		/* if the current group has entries we have to undo that one */
		curgroup = doc->unre.current;
		doc->unre.current = unregroup_new(doc);
	} else if (doc->unre.first){
		/* we have to undo the first one in the list */
		curgroup = doc->unre.first->data;
		doc->unre.first = g_list_remove(doc->unre.first, curgroup);
		doc->unre.num_groups--;
		/* what happens when this was the last entry in the list?*/
		DEBUG_MSG("doc_undo, removed a group, num_groups =%d\n", doc->unre.num_groups);
		if (doc->unre.num_groups == 0) {
			doc->unre.first = NULL;
			doc->unre.last = NULL;
		} else {
			doc->unre.first = g_list_first(doc->unre.first);
		}
	}
	if (curgroup) {
		doc->unre.redofirst = g_list_prepend(doc->unre.redofirst,curgroup);
		/* since activate calls doc_set_modified, and doc_set_modified sets 
		the undo/redo widgets, the lenght of the redolist should be > 0 _before_
		activate is called */
		unregroup_activate(curgroup, doc, 0);
	}
}

static void doc_redo(Tdocument *doc) {
	if (doc->unre.redofirst) {
		unregroup_t *curgroup = NULL;
	
		curgroup = doc->unre.redofirst->data;
		doc->unre.redofirst = g_list_remove(doc->unre.redofirst, curgroup);
		/* what happens when this was the last one of the list? does it return NULL ? */
		DEBUG_MSG("doc_redo, doc->unre.redofirst=%p\n", doc->unre.redofirst);

		doc_unre_new_group(doc);
		doc->unre.first = g_list_prepend(doc->unre.first, curgroup);
		if (!doc->unre.last) {
			doc->unre.last = doc->unre.first;
		}
		doc->unre.num_groups++;
		DEBUG_MSG("doc_redo, added a group, num_groups =%d\n", doc->unre.num_groups);
		unregroup_activate(curgroup, doc, 1);
	}
}

void doc_unre_add(Tdocument *doc, const char *text, int start, int end, undo_op_t op) {
	unreentry_t *entry=NULL;
	gboolean handled = FALSE;
	
	DEBUG_MSG("doc_unre_add, start=%d, end=%d\n", start, end);
	if (doc->unre.current->entries) {
		entry = (unreentry_t *)(doc->unre.current->entries->data);
		DEBUG_MSG("doc_unre_add, currentgroup=%p, entry=%p\n", doc->unre.current,entry );
		DEBUG_MSG("doc_unre_add, entry=%p\n", entry);
		/* do efficient, add the current one to the previous one */
		DEBUG_MSG("doc_unre_add, entry->end=%d, start=%d\n", entry->end, start);
		DEBUG_MSG("doc_unre_add, entry->op=%d, op=%d, UndoInsert=%d\n", entry->op, op, UndoInsert);
		if ((entry->end == start && entry->op == UndoInsert && op == UndoInsert) 
			|| (entry->start == end && entry->op == UndoDelete && op == UndoDelete)) {
			gchar *newstr;
			if (op == UndoInsert) {
				newstr = g_strconcat(entry->text, text, NULL);
				entry->end = end;
			} else {
				newstr = g_strconcat(text, entry->text, NULL);
				entry->start = start;
			}
			g_free(entry->text);
			entry->text = newstr;
			DEBUG_MSG("doc_unre_add, added to previous entry, text=%s\n", entry->text);
			handled = TRUE;
		}
	}
	if (!handled) {
		unreentry_t *new_entry;
		new_entry = unreentry_new(text, start, end, op);
		DEBUG_MSG("doc_unre_add, not handled yet, new entry with text=%s\n", text);
		doc->unre.current->entries = g_list_prepend(doc->unre.current->entries, new_entry);
		if (doc->unre.redofirst) {
			/* destroy the redo list, groups and entries */
			unre_list_cleanup(&doc->unre.redofirst);
			DEBUG_MSG("doc_unre_add, redolist=%p\n", doc->unre.redofirst);
		}
	}
}


static void doc_unre_start(Tdocument *doc) {
	DEBUG_MSG("doc_unre_start, started\n");
	doc_unbind_signals(doc);
}

static void doc_unre_finish(Tdocument *doc) {
	DEBUG_MSG("doc_unre_finish, started\n");
	/* now re-establish the signals */
	doc_bind_signals(doc);
	if (doc->highlightstate) {
		/* doc_need_highlighting(doc);*/
	}
}

void doc_unre_new_group(Tdocument *doc) {
	DEBUG_MSG("doc_unre_new_group, started, num entries=%d\n", g_list_length(doc->unre.current->entries));
	if (g_list_length(doc->unre.current->entries) > 0) {
		doc->unre.first = g_list_prepend(doc->unre.first, doc->unre.current);
		if (!doc->unre.last) {
			doc->unre.last = doc->unre.first;
		}
		doc->unre.num_groups++;
		DEBUG_MSG("doc_unre_new_group, added a group, num_groups =%d\n", doc->unre.num_groups);
		doc->unre.current = unregroup_new(doc);
		if (doc->unre.num_groups > main_v->props.num_undo_levels) {
			doc_unre_destroy_last_group(doc);
		}
	}
}


void doc_unre_init(Tdocument *doc) {
	DEBUG_MSG("doc_unre_init, started\n");
	doc->unre.first = NULL;
	doc->unre.last = NULL;
	doc->unre.current = unregroup_new(doc);
	doc->unre.num_groups = 0;
	doc->unre.redofirst = NULL;
}

void doc_unre_destroy(Tdocument *doc) {
	/* TODO */
	DEBUG_MSG("doc_unre_destroy, about to destroy undolist %p\n",doc->unre.first );
	unre_list_cleanup(&doc->unre.first);
	DEBUG_MSG("doc_unre_destroy, about to destroy redofirst %p\n", doc->unre.redofirst);
	unre_list_cleanup(&doc->unre.redofirst);
	DEBUG_MSG("doc_unre_destroy, about to destroy current %p\n", doc->unre.current);
	unregroup_destroy(doc->unre.current);
}

void doc_unre_clear_all(Tdocument *doc) {
	doc_unre_destroy(doc);
	doc_unre_init(doc);
}

gint doc_undo_op_compare(Tdocument *doc, undo_op_t testfor) {
	if (doc->unre.current->entries && doc->unre.current->entries->data) {
		unreentry_t *entry = doc->unre.current->entries->data;
		if (entry->op == testfor) {
			return 1;
		}
		return 0;
	}
	return 1;
}

void undo_cb(GtkWidget * widget, gpointer data) {
	DEBUG_MSG("undo_cb, started\n");
	if (main_v->current_document) {
		doc_unre_start(main_v->current_document);
		doc_undo(main_v->current_document);
		doc_unre_finish(main_v->current_document);
	}
}

void redo_cb(GtkWidget * widget, gpointer data) {
	if (main_v->current_document) {
		doc_unre_start(main_v->current_document);
		doc_redo(main_v->current_document);
		doc_unre_finish(main_v->current_document);
	}
}

void undo_all_cb(GtkWidget * widget, gpointer data) {
	/* TODO */
	if (main_v->current_document) {
		doc_unre_start(main_v->current_document);
		while (main_v->current_document->unre.first) {
			doc_undo(main_v->current_document);
		}
		doc_unre_finish(main_v->current_document);
	}
}

void redo_all_cb(GtkWidget * widget, gpointer data) {
	/* TODO */
	if (main_v->current_document) {
		doc_unre_start(main_v->current_document);
		while (main_v->current_document->unre.redofirst) {
			doc_redo(main_v->current_document);
		}
		doc_unre_finish(main_v->current_document);
	}
}

#ifdef __GNUC__
__inline__ 
#endif
gboolean doc_has_undo_list(Tdocument *doc) {
	return (doc->unre.first || doc->unre.current->entries) ? TRUE : FALSE;
}

#ifdef __GNUC__
__inline__
#endif
gboolean doc_has_redo_list(Tdocument *doc) {
	return (doc->unre.redofirst) ? TRUE : FALSE;
}
