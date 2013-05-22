/* Bluefish HTML Editor
 * bookmark.c - bookmarks
 *
 * Copyright (C) 2003 Oskar Swida
 * modifications (C) 2004-2013 Olivier Sessink
 * modifications (C) 2011-2012 James Hayward
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
#define DEBUG
/*#define BMARKREF*/

#include <gtk/gtk.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bluefish.h"
#include "bookmark.h"
#include "bf_lib.h"
#include "bfwin_uimanager.h"
#include "dialog_utils.h"
#include "document.h"
#include "gtk_easy.h"
#include "stringlist.h"

#ifdef BMARKREF
typedef struct {
	gint itercount;
} Tbmarkref;

Tbmarkref bmarkref = { 0 };

#endif

/*
bookmarks will be loaded and saved to an arraylist (see stringlist.c). This
is a double linked list (GList *) with pointers to string arrays (gchar **).

To have the GUI work with them, we convert those arrays (gchar **) into a
Tbmark struct. This struct will have a pointer to the array (gchar **) so
on change it can directly change the array as well, without any need to
look it up in the list.

For the GUI, we store everything in a Gtktreestore. The treestore will have a
pointer to a string with the name, and it will also have a pointer to the
Tbmark. When the user clicks in the Gtktreeview widget, we can get
immediately a pointer to the Tbmark, and that has the Gtktextmark, so that
is very easy, and very fast!

All normal (non-project) windows do share the same bookmarks list.
So they store the same Gtktreestore as well. The project functions 
create a new gtktreestore when they convert a window (Tbfwin)
into a project window.
*/

#define BMARK_SHOW_NUM_TEXT_CHARS 40
#define BMARK_STORE_TEXT_NUM_CHARS 25

enum {
	NAME_COLUMN,				/* bookmark name */
	PTR_COLUMN,					/* bookmark pointer */
	N_COLUMNS
};

enum {
	BMARK_ADD_PERM_DIALOG,
	BMARK_RENAME_TEMP_DIALOG,
	BMARK_RENAME_PERM_DIALOG
};

typedef struct {
	GtkTextMark *mark;
	GFile *uri;
	gint offset;
	Tdocument *doc;
	GtkTreeIter iter;			/* for tree view */
	gchar *description;
	gchar *text;
	gchar *name;
	gint len;					/* file length for integrity check - perhaps some hash code is needed */
	gboolean is_temp;
	gchar **strarr;				/* this is a pointer to the location where this bookmark is stored in the sessionlist,
								   so we can immediately change it _in_ the list */
} Tbmark;
#define BMARK(var) ((Tbmark *)(var))

typedef struct {
	GtkTreeStore *bookmarkstore;	/* the treestore with the name and the pointer to the Tbmark */
	GHashTable *bmarkfiles;		/* a hash table with the GFile as key, and the iter in the treestore as value */
} Tbmarkdata;
#define BMARKDATA(var) ((Tbmarkdata *)(var))

enum {
	BM_FMODE_FILE,
	BM_FMODE_PATH,
	BM_FMODE_URI,
	BM_FMODE_HOME				/* not implemented, defaults to full */
};

enum {
	BM_SMODE_CONTENT,
	BM_SMODE_NAME,
	BM_SMODE_BOTH
};

enum {
	BM_SEARCH_CONTENT,
	BM_SEARCH_NAME,
	BM_SEARCH_BOTH
};

/* Free bookmark structure */
static void
bmark_free(gpointer ptr)
{
	Tbmark *m;
	if (ptr == NULL)
		return;
	m = BMARK(ptr);
	if (m->doc && m->mark) {
		DEBUG_MSG("bmark_free, deleting mark %p\n", m->mark);
		gtk_text_buffer_delete_mark(m->doc->buffer, m->mark);
		m->doc = NULL;
	}
#ifdef DEBUG
	if (m->strarr) {
		DEBUG_MSG("bmark_free, NOT GOOD, strarr should be NULL here...\n");
	}
#endif
	DEBUG_MSG("bmark_free, unref uri %p\n", m->uri);
	if (m->uri)
		g_object_unref(m->uri);
	g_free(m->text);
	g_free(m->name);
	g_free(m->description);
	/*g_print("free bmark %p\n",m); */
	g_slice_free(Tbmark, m);
}

static gchar *
bmark_showname(Tbfwin * bfwin, Tbmark * b)
{
	if (b->name && strlen(b->name) > 0 && bfwin->session->bookmarks_show_mode == BM_SMODE_BOTH) {
		return g_strconcat(b->name, " - ", b->text, NULL);
	} else if ((b->name && bfwin->session->bookmarks_show_mode == BM_SMODE_NAME) || !b->text) {
		return g_strdup(b->name);
	} else {
		return g_strdup(b->text);
	}
}

static gchar *
bmark_filename(Tbfwin * bfwin, GFile * uri)
{
	gchar *title;
	if (!uri) {
		g_warning("Bookmark without uri! Please report this message as a bug!\n");
		return g_strdup("Bug - please report");
	}
	if (g_file_has_uri_scheme(uri, "tmp")) {
		DEBUG_MSG("bmark_filename, have untitled document with uri scheme %s and path %s\n",g_file_get_uri_scheme(uri), g_file_get_basename(uri));
		return g_file_get_basename(uri);
	}
	
	switch (bfwin->session->bookmarks_filename_mode) {
	case BM_FMODE_PATH:
		title = g_file_get_uri(uri);
		break;
	case BM_FMODE_FILE:
		title = g_file_get_basename(uri);
		break;
	case BM_FMODE_URI:
	default:
		title = g_file_get_uri(uri);
		break;
	}
	return title;
}

static void
bmark_update_treestore_name(Tbfwin * bfwin)
{
	GtkTreeIter piter, citer;
	gboolean cont1, cont2;
	cont1 = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &piter);

	while (cont1) {
		Tbmark *b = NULL;
		gchar *name;
		cont2 =
			gtk_tree_model_iter_children(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &citer,
										 &piter);
		/* first handle the filename of the parent */
		if (cont2) {
			gtk_tree_model_get(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &citer, PTR_COLUMN,
							   &b, -1);
			/* TODO: if the bookmark has a document, use the untitled number from that document */
			name = bmark_filename(bfwin, b->uri);
			gtk_tree_store_set(BMARKDATA(bfwin->bmarkdata)->bookmarkstore, &piter, NAME_COLUMN, name, -1);
			g_free(name);
		}
		while (cont2) {
			gtk_tree_model_get(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &citer, PTR_COLUMN,
							   &b, -1);
			name = bmark_showname(bfwin, b);
			gtk_tree_store_set(BMARKDATA(bfwin->bmarkdata)->bookmarkstore, &citer, NAME_COLUMN, name, -1);
			g_free(name);
			cont2 =
				gtk_tree_model_iter_next(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &citer);
		}
		cont1 = gtk_tree_model_iter_next(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &piter);
	}
}

static void
bmark_update_offset_from_textmark(Tbmark * b)
{
	if (b->doc && b->mark) {
		GtkTextIter it, it2;
		int len;
		gtk_text_buffer_get_iter_at_mark(b->doc->buffer, &it, b->mark);
		b->offset = gtk_text_iter_get_offset(&it);
		len = strlen(b->text);
		/* to aid future repositioning (if the file has changed) update the text as well */
		gtk_text_buffer_get_iter_at_offset(b->doc->buffer, &it2, b->offset + len);
		g_free(b->text);
		b->text = gtk_text_buffer_get_text(b->doc->buffer, &it, &it2, FALSE);
		DEBUG_MSG("bmark_update_offset_from_textmark, text=%s\n", b->text);
	}
}

/*
 * this function should use a smart sorting algorithm to find
 * the GtkTreeIter of the bookmark *before* the place where this
 * bookmark should be added, but the same function can be used to
 * find the bookmarks we have to check to detect double bookmarks
 * at the same line.
 *
 * returns the bookmark closest before 'offset', or the bookmark exactly at 'offset'
 *
 * returns NULL if we have to append this as first child to the parent
 *
 */
static Tbmark *
bmark_find_bookmark_before_offset(Tbfwin * bfwin, guint offset, GtkTreeIter * parent)
{
	gboolean cont;
	GtkTreeIter iter;
	Tbmark *b1 = NULL, *b2;
	cont =
		gtk_tree_model_iter_children(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &iter,
									 parent);
	while (cont) {
		gtk_tree_model_get(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &iter, PTR_COLUMN, &b2,
						   -1);
		bmark_update_offset_from_textmark(b2);
		if (b2->offset > offset)
			return b1;
		b1 = b2;
		cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &iter);
	}
	return b1;
}

void
bmark_rename_uri(Tbfwin * bfwin, Tbmark * b, GFile * newuri)
{
	if (b->uri)
		g_object_unref(b->uri);
	b->uri = newuri;
	if (newuri)
		g_object_ref(b->uri);
	if (b->strarr != NULL) {
		g_free(b->strarr[2]);
		if (newuri)
			b->strarr[2] = g_file_get_parse_name(b->uri);
		else
			b->strarr[2] = g_strdup("");
	}
}

void
bmark_doc_renamed(Tbfwin * bfwin, Tdocument * doc)
{
	if (doc->uri && doc->bmark_parent) {
		GtkTreeIter tmpiter;
		gboolean cont;
		gchar *name;
		name = bmark_filename(bfwin, doc->uri);
		gtk_tree_store_set(BMARKDATA(bfwin->bmarkdata)->bookmarkstore, doc->bmark_parent, NAME_COLUMN, name,
						   -1);
		g_free(name);
		cont =
			gtk_tree_model_iter_children(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &tmpiter,
										 doc->bmark_parent);
		while (cont) {
			Tbmark *b;
			gtk_tree_model_get(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &tmpiter,
							   PTR_COLUMN, &b, -1);
			if (b) {
				bmark_rename_uri(bfwin, b, doc->uri);
			}
			cont =
				gtk_tree_model_iter_next(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore),
										 &tmpiter);
		}
	}
}


/* this function re-uses the b->strarr if possible, otherwise it will create a new one and
append it to the list */
static void
bmark_store(Tbfwin * bfwin, Tbmark * b)
{
	gchar **strarr;
	if (b->is_temp) {
		DEBUG_MSG("bmark_store, called for temp bookmark %p ?!?! weird!!!! returning\n", b);
		return;
	}
	if (!b->uri) {
		DEBUG_MSG("bmark_store, cannot store bookmark for file without filename\n");
		return;
	}
	if (g_file_has_uri_scheme(b->uri,"tmp")) {
		DEBUG_MSG("bmark_store, cannot store bookmark for file without filename (tmp:// uri)\n");
		return;
	}
	/* if there is a strarr already, we only update the fields, else we append a new one */
	if (b->strarr == NULL) {
		DEBUG_MSG("bmark_store, creating new strarr for bookmark %p\n", b);
		strarr = g_malloc0(sizeof(gchar *) * 7);
		DEBUG_MSG("name=%s, description=%s, text=%s\n", b->name, b->description, b->text);
		strarr[2] = g_file_get_parse_name(b->uri);
		strarr[4] = g_strdup(b->text);
	} else {
		DEBUG_MSG("bmark_store, bookmark %p has strarr at %p\n", b, b->strarr);
		strarr = b->strarr;
		/* free the ones we are going to update */
		g_free(strarr[0]);
		g_free(strarr[1]);
		g_free(strarr[3]);
		g_free(strarr[5]);
	}
	strarr[0] = g_strdup(b->name);
	strarr[1] = g_strdup(b->description);

	if (b->doc)
		b->len = gtk_text_buffer_get_char_count(b->doc->buffer);

	strarr[3] = g_strdup_printf("%d", b->offset);
	DEBUG_MSG("bmark_store, offset string=%s, offset int=%d\n", strarr[3], b->offset);
	strarr[5] = g_strdup_printf("%d", b->len);
	DEBUG_MSG("bmark_store, stored size=%d\n", b->len);
	if (b->strarr == NULL) {
		bfwin->session->bmarks = g_list_append(bfwin->session->bmarks, strarr);
		DEBUG_MSG("added new (previously unstored) bookmark to session list, list length=%d\n",
				  g_list_length(bfwin->session->bmarks));
		b->strarr = strarr;
	}
}

/* when a users want to save the project, it's good to have updated bookmarks
so this function will update all arrays (strarr**)
 */
void
bmark_store_all(Tbfwin * bfwin)
{
	/* we loop over all filename iters, and only for the ones that are opened
	   we loop over the children (the ones that are not open cannot be changed) */
	GtkTreeIter fileit;
	gboolean cont;

	cont =
		gtk_tree_model_iter_children(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &fileit,
									 NULL);
	while (cont) {
		Tdocument *doc = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &fileit, PTR_COLUMN,
						   &doc, -1);
		if (doc) {
			/* the document is open, so the offsets could be changed, store all permanent */
			GtkTreeIter bmit;
			gboolean cont2 =
				gtk_tree_model_iter_children(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore),
											 &bmit, &fileit);
			DEBUG_MSG("bmark_store_all, storing bookmarks for %s\n",
					  gtk_label_get_text(GTK_LABEL(doc->tab_menu)));
			while (cont2) {
				Tbmark *bmark;
				gtk_tree_model_get(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &bmit,
								   PTR_COLUMN, &bmark, -1);
				if (!bmark->is_temp) {
					bmark_update_offset_from_textmark(bmark);
					bmark_store(bfwin, bmark);
				}
				cont2 =
					gtk_tree_model_iter_next(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore),
											 &bmit);
			}
		} else {
			DEBUG_MSG("doc not set, so not open...\n");
		}
		cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &fileit);
	}							/* cont */
}

/* removes the bookmark from the session, removed the b->strarr pointer and frees it */
static void
bmark_unstore(Tbfwin * bfwin, Tbmark * b)
{
	if (bfwin->session->bmarks == NULL || b->strarr == NULL)
		return;
	DEBUG_MSG("bmark_remove, removing bookmark %p from sessionlist\n", b);
	bfwin->session->bmarks = g_list_remove(bfwin->session->bmarks, b->strarr);
	g_strfreev(b->strarr);
	b->strarr = NULL;
}

static Tbmark *
get_bmark_at_iter(GtkTreeModel * model, GtkTreeIter * iter)
{
	Tbmark *retval = NULL;
	gtk_tree_model_get(model, iter, PTR_COLUMN, &retval, -1);
	return retval;
}

/* get value from pointer column */
static Tbmark *
get_current_bmark(Tbfwin * bfwin)
{
	if (bfwin->bmark) {
		GtkTreePath *path;
		GtkTreeViewColumn *col;
		gtk_tree_view_get_cursor(bfwin->bmark, &path, &col);
		if (path != NULL) {
			Tbmark *retval = NULL;
			if (gtk_tree_path_get_depth(path) == 2) {
				GtkTreeIter iter;
				GtkTreeModel *model = gtk_tree_view_get_model(bfwin->bmark);
				gtk_tree_model_get_iter(model, &iter, path);
				retval = get_bmark_at_iter(model, &iter);
			} else {
				DEBUG_MSG("get_current_bmark, error, depth=%d\n", gtk_tree_path_get_depth(path));
			}
			gtk_tree_path_free(path);
			DEBUG_MSG("get_current_bmark, returning %p\n", retval);
			return retval;
		}
	}
	return NULL;
}

void
bmark_add_rename_dialog(Tbfwin * bfwin, gchar * dialogtitle)
{
#if GTK_CHECK_VERSION(3,0,0)
	GtkWidget *align, *child, *dlg, *name, *desc, *button, *grid, *istemp;
#else
	GtkWidget *dlg, *name, *desc, *button, *table, *istemp;
#endif
	gint result;
	Tbmark *m = get_current_bmark(bfwin);
	if (!m)
		return;

	dlg =
		gtk_dialog_new_with_buttons(dialogtitle, GTK_WINDOW(bfwin->main_window), GTK_DIALOG_MODAL,
									GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	button = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_widget_set_can_default(button, TRUE);
	gtk_dialog_add_action_widget(GTK_DIALOG(dlg), button, GTK_RESPONSE_OK);

#if GTK_CHECK_VERSION(3,0,0)
	align = gtk_alignment_new(0, 0, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align), 12, 12, 6, 6);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dlg))), align, FALSE, FALSE, 0);

	grid = gtk_grid_new();
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_container_add(GTK_CONTAINER(align), grid);

	name = gtk_entry_new();
	if (m->name)
		gtk_entry_set_text(GTK_ENTRY(name), m->name);
	gtk_widget_set_hexpand(name, TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(name), TRUE);
	gtk_container_add(GTK_CONTAINER(grid), name);
	gtk_grid_insert_column(GTK_GRID(grid), 0);
	gtk_grid_attach(GTK_GRID(grid), dialog_mnemonic_label_new(_("_Name:"), name), 0, 0, 1, 1);

	desc = gtk_entry_new();
	if (m->description)
		gtk_entry_set_text(GTK_ENTRY(desc), m->description);
	gtk_entry_set_activates_default(GTK_ENTRY(desc), TRUE);
	gtk_grid_attach_next_to(GTK_GRID(grid), desc, name, GTK_POS_BOTTOM, 1, 1);
	gtk_grid_attach_next_to(GTK_GRID(grid), dialog_mnemonic_label_new(_("_Description:"), desc), desc,
							GTK_POS_LEFT, 1, 1);

	istemp = checkbut_with_value(_("_Temporary"), m->is_temp);
	child = gtk_grid_get_child_at(GTK_GRID(grid), 0, 1);
	gtk_grid_attach_next_to(GTK_GRID(grid), istemp, child, GTK_POS_BOTTOM, 2, 1);
#else
	table = gtk_table_new(2, 3, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dlg))), table, FALSE, FALSE, 12);

	name = dialog_entry_in_table(m->name, table, 1, 2, 0, 1);
	gtk_entry_set_activates_default(GTK_ENTRY(name), TRUE);
	dialog_mnemonic_label_in_table(_("_Name:"), name, table, 0, 1, 0, 1);

	desc = dialog_entry_in_table(m->description, table, 1, 2, 1, 2);
	gtk_entry_set_activates_default(GTK_ENTRY(desc), TRUE);
	dialog_mnemonic_label_in_table(_("_Description:"), desc, table, 0, 1, 1, 2);

	istemp = checkbut_with_value(_("Temporary"), m->is_temp);
	gtk_table_attach_defaults(GTK_TABLE(table), istemp, 0, 2, 2, 3);
#endif

	gtk_window_set_default(GTK_WINDOW(dlg), button);

	gtk_widget_show_all(dlg);
	result = gtk_dialog_run(GTK_DIALOG(dlg));

	if (result == GTK_RESPONSE_OK) {
		gchar *tmpstr;
		g_free(m->name);
		m->name = g_strdup(gtk_entry_get_text(GTK_ENTRY(name)));
		g_free(m->description);
		m->description = g_strdup(gtk_entry_get_text(GTK_ENTRY(desc)));
		m->is_temp = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(istemp));
		tmpstr = bmark_showname(bfwin, m);
		gtk_tree_store_set(BMARKDATA(bfwin->bmarkdata)->bookmarkstore, &m->iter, NAME_COLUMN, tmpstr, -1);
		g_free(tmpstr);
		if (m->is_temp) {
			if (m->strarr) {
				/* hmm previously this was not a temporary bookmark */
				bmark_unstore(bfwin, m);
			}
		} else {
			bmark_store(bfwin, m);
		}
	}
	gtk_widget_destroy(dlg);
}

static void
bmark_activate(Tbfwin * bfwin, Tbmark * b, gboolean select_bmark)
{
	GtkTextIter it;

	if (!b)
		return;

	if (!b->doc && !b->uri)
		return;

	if (b->doc && b->mark) {
		/* recalculate offset */
		gtk_text_buffer_get_iter_at_mark(b->doc->buffer, &it, b->mark);
		b->offset = gtk_text_iter_get_offset(&it);
	}
	DEBUG_MSG("bmark_activate, bmark at %p, uri at %p\n", b, b->uri);
	DEBUG_MSG("bmark_activate, calling doc_new_from_uri with goto_offset %d\n", b->offset);
	if (b->doc) {
		bfwin_switch_to_document_by_pointer(BFWIN(b->doc->bfwin), b->doc);
		doc_select_line_by_offset(b->doc, b->offset, TRUE);
	} else {
		doc_new_from_uri(bfwin, b->uri, NULL, FALSE, FALSE, -1, b->offset);
	}
	/* remove selection */
	if (b->doc) {
		gtk_text_buffer_get_iter_at_mark(b->doc->buffer, &it, gtk_text_buffer_get_insert(b->doc->buffer));
		gtk_text_buffer_move_mark_by_name(b->doc->buffer, "selection_bound", &it);
		gtk_widget_grab_focus(b->doc->view);
	} else if (bfwin->current_document) {
		gtk_widget_grab_focus(bfwin->current_document->view);
	}
	if (select_bmark) {
		GtkTreeIter fiter;
		gtk_tree_model_filter_convert_child_iter_to_iter(bfwin->bmarkfilter, &fiter, &b->iter);
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(bfwin->bmark), &fiter);
	}

}

/*static void bmark_goto_selected(Tbfwin *bfwin) {
	Tbmark *b = get_current_bmark(bfwin);
	if (b) {
		bmark_activate(bfwin, b, FALSE);
	}
}*/
/*
 * removes the bookmark from the treestore, and if it is the last remaining bookmark
 * for the document, it will remove the parent iter (the filename) from the treestore as well
 *
 * if the parent is not removed it will return TRUE
 * if the parent is removed, it will return FALSE
 */
static gboolean
bmark_check_remove(Tbfwin * bfwin, Tbmark * b)
{
	GtkTreeIter parent;
	/*GtkTextIter it;*/

	if (gtk_tree_model_iter_parent
		(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &parent, &b->iter)) {
		gint numchild =
			gtk_tree_model_iter_n_children(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore),
										   &parent);
		DEBUG_MSG("bmark_check_remove, the parent of this bookmark has %d children\n", numchild);
		gtk_tree_store_remove(BMARKDATA(bfwin->bmarkdata)->bookmarkstore, &(b->iter));

		/* Olivier, 22 may 2013: what does the next line do ?????????? seems it can be removed */
		/*if (b->doc) {
			gtk_text_buffer_get_iter_at_mark(b->doc->buffer, &it, b->mark);
		}*/

		if (numchild == 1) {
			GtkTextIter *tmpiter;
			DEBUG_MSG("bmark_check_remove, we removed the last child, now remove the parent\n");
			gtk_tree_store_remove(BMARKDATA(bfwin->bmarkdata)->bookmarkstore, &parent);
			/* if the document is open, it should be removed from the hastable as well */
			tmpiter = g_hash_table_lookup(BMARKDATA(bfwin->bmarkdata)->bmarkfiles, b->uri);
			if (tmpiter) {
				DEBUG_MSG("bmark_check_remove, removing iter %p from hashtable\n", tmpiter);
				g_hash_table_remove(BMARKDATA(bfwin->bmarkdata)->bmarkfiles, b->uri);
				if (b->doc)
					b->doc->bmark_parent = NULL;
			}

			if (b->doc && b->doc->view)
				bluefish_text_view_set_show_symbols_redraw(BLUEFISH_TEXT_VIEW(b->doc->view), FALSE);

			return FALSE;
		}
	}
	DEBUG_MSG("bmark_check_remove, finished\n");
	return TRUE;
}

/* *parent should be a valid GtkTreeIter pointing to a filename.  */
static void
bmark_del_children_backend(Tbfwin * bfwin, GtkTreeIter * parent)
{
	GtkTreeIter tmpiter;
	gboolean have_parent = TRUE;
	while (have_parent
		   && gtk_tree_model_iter_children(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore),
										   &tmpiter, parent)) {
		Tbmark *b;
		gtk_tree_model_get(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &tmpiter, PTR_COLUMN,
						   &b, -1);
		if (b) {
			DEBUG_MSG("bmark_del_children_backend, found b=%p\n", b);
			have_parent = bmark_check_remove(bfwin, b);
			if (!b->is_temp)
				bmark_unstore(bfwin, b);
			bmark_free(b);
		} else {
			DEBUG_MSG("bmark_del_children_backend, iter without bookmark ??? LOOP WARNING!\n");
		}
	}
}

static void
popup_menu_default_permanent(GtkAction * action, gpointer user_data)
{
	main_v->globses.bookmarks_default_store = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
}

static void
popup_menu_delete(GtkAction * action, gpointer user_data)
{
	Tbfwin *bfwin = BFWIN(user_data);
	Tbmark *bmark;
	gint retval;
	gchar *pstr;
	const gchar *buttons[] = { GTK_STOCK_NO, GTK_STOCK_YES, NULL };
	DEBUG_MSG("popup_menu_delete\n");
	bmark = get_current_bmark(bfwin);
	if (!bmark)
		return;
	/* check if it is temp mark */
	if (bmark->is_temp) {
		bmark_check_remove(bfwin, bmark);	/* check  if we should remove a filename too */
		bmark_free(bmark);
	} else {
		pstr = g_strdup_printf(_("Do you really want to delete %s?"), bmark->name);
		retval = message_dialog_new_multi(bfwin->main_window,
										  GTK_MESSAGE_QUESTION,
										  buttons, _("Delete permanent bookmark."), pstr);
		g_free(pstr);
		if (retval == 0)
			return;
		bmark_check_remove(bfwin, bmark);	/* check  if we should remove a filename too */
		bmark_unstore(bfwin, bmark);
		bmark_free(bmark);
	}
	if (bfwin->current_document)
		gtk_widget_grab_focus(bfwin->current_document->view);
}

static void
popup_menu_delete_all(GtkAction * action, gpointer user_data)
{
	Tbfwin *bfwin = BFWIN(user_data);
	GtkTreeIter tmpiter;
	gint retval;

	const gchar *buttons[] = { GTK_STOCK_NO, GTK_STOCK_YES, NULL };


	if (bfwin == NULL || !bfwin->current_document)
		return;

	retval = message_dialog_new_multi(bfwin->main_window,
									  GTK_MESSAGE_QUESTION, buttons, _("Delete all bookmarks."), NULL);
	if (retval == 0)
		return;

	DEBUG_MSG("bmark_del_all, deleting all bookmarks!\n");
	while (gtk_tree_model_iter_children
		   (GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &tmpiter, NULL))
		bmark_del_children_backend(bfwin, &tmpiter);

	gtk_widget_grab_focus(bfwin->current_document->view);
}

static void
popup_menu_delete_all_doc(GtkAction * action, gpointer user_data)
{
	Tbfwin *bfwin = BFWIN(user_data);

	if (bfwin->bmark) {
		GtkTreePath *path;
		GtkTreeViewColumn *col;
		gtk_tree_view_get_cursor(bfwin->bmark, &path, &col);
		if (path != NULL) {
			gchar *name;
			gchar *pstr;
			const gchar *buttons[] = { GTK_STOCK_NO, GTK_STOCK_YES, NULL };
			GtkTreeIter iter, realiter;
			GtkTreeModel *model = gtk_tree_view_get_model(bfwin->bmark);
			gint depth, retval;
			depth = gtk_tree_path_get_depth(path);
			if (depth == 2) {
				/* go up to parent */
				gtk_tree_path_up(path);
			}
			gtk_tree_model_get_iter(model, &iter, path);
			/* iter is now an iter in the filter model, not in the real backend model !!!! */
			gtk_tree_path_free(path);
			gtk_tree_model_get(model, &iter, NAME_COLUMN, &name, -1);

			pstr = g_strdup_printf(_("Do you really want to delete all bookmarks for %s?"), name);
			retval = message_dialog_new_multi(bfwin->main_window,
											  GTK_MESSAGE_QUESTION, buttons, _("Delete bookmarks?"), pstr);
			g_free(pstr);
			if (retval == 0)
				return;
			gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(model), &realiter, &iter);
			bmark_del_children_backend(bfwin, &realiter);
		}
	}
	if (bfwin->current_document)
		gtk_widget_grab_focus(bfwin->current_document->view);
}

static void
popup_menu_edit(GtkAction * action, gpointer user_data)
{
	Tbfwin *bfwin = BFWIN(user_data);
	Tbmark *bmark = get_current_bmark(bfwin);

	if (!bmark)
		return;

	bmark_add_rename_dialog(bfwin, _("Edit bookmark"));
}

static void
popup_menu_show_bookmark(GtkRadioAction * action, GtkRadioAction * current, gpointer user_data)
{
	BFWIN(user_data)->session->bookmarks_show_mode = gtk_radio_action_get_current_value(action);
	bmark_update_treestore_name(BFWIN(user_data));
}

static void
popup_menu_show_file(GtkRadioAction * action, GtkRadioAction * current, gpointer user_data)
{
	BFWIN(user_data)->session->bookmarks_filename_mode = gtk_radio_action_get_current_value(action);
	bmark_update_treestore_name(BFWIN(user_data));
}

static void
popup_search_mode_changed(GtkRadioAction * action, GtkRadioAction * current, gpointer user_data)
{
	BFWIN(user_data)->session->bmarksearchmode = gtk_radio_action_get_current_value(action);
}

static const gchar *bookmark_menu_ui =
	"<ui>"
	"  <popup action='BookmarkMenu'>"
	"    <menuitem action='EditBookmark'/>"
	"    <menuitem action='DeleteBookmark'/>"
	"    <separator/>"
	"    <menuitem action='DeleteAllBookmarkInDoc'/>"
	"    <menuitem action='DeleteAllBookmark'/>"
	"    <separator/>"
	"    <menuitem action='DefaultPermanent'/>"
	"    <separator/>"
	"    <menu action='ShowFileMenu'>"
	"    	<menuitem action='BookmarkFileByName'/>"
	"    	<menuitem action='BookmarkFileByPath'/>"
	"    	<menuitem action='BookmarkFileByURI'/>"
	"    </menu>"
	"    <menu action='ShowBookmarkMenu'>"
	"    	<menuitem action='BookmarkContent'/>"
	"    	<menuitem action='BookmarkName'/>"
	"    	<menuitem action='BookmarkNameContent'/>"
	"    </menu>"
	"  </popup>"
	"</ui>";

static const gchar *bookmark_search_menu_ui =
	"<ui>"
	"  <popup action='BookmarkSearchMenu'>"
	"    <menuitem action='BookmarkSearchContent'/>"
	"    <menuitem action='BookmarkSearchName'/>"
	"    <menuitem action='BookmarkSearchNameContent'/>"
	"  </popup>"
	"</ui>";

static const GtkActionEntry bookmark_actions[] = {
	{"BookmarkMenu", NULL, N_("Bookmark menu")},
	{"ShowBookmarkMenu", NULL, N_("Show Bookmark")},
	{"ShowFileMenu", NULL, N_("Show File")},
	{"BookmarkSearchMenu", NULL, N_("Bookmark search menu")},
	{"EditBookmark", NULL, N_("_Edit..."), NULL, N_("Edit bookmark"),
	 G_CALLBACK(popup_menu_edit)},
	{"DeleteBookmark", NULL, N_("_Delete"), NULL, N_("Delete bookmark"), G_CALLBACK(popup_menu_delete)},
	{"DeleteAllBookmark", NULL, N_("Delete All"), NULL, N_("Delete all bookmarks"),
	 G_CALLBACK(popup_menu_delete_all)},
	{"DeleteAllBookmarkInDoc", NULL, N_("Delete All in Document"), NULL,
	 N_("Delete all bookmarks in document"),
	 G_CALLBACK(popup_menu_delete_all_doc)}
};

static const GtkToggleActionEntry bookmark_toggle_actions[] = {
	{"DefaultPermanent", NULL, N_("Permanent by default"), NULL, N_("Permanent by default"),
	 G_CALLBACK(popup_menu_default_permanent)}
};

static const GtkRadioActionEntry bookmark_file_radio_actions[] = {
	{"BookmarkFileByName", NULL, N_("By Name"), NULL, NULL, 0},
	{"BookmarkFileByPath", NULL, N_("By Full Path"), NULL, NULL, 1},
	{"BookmarkFileByURI", NULL, N_("By Full URI"), NULL, NULL, 2},
};

static const GtkRadioActionEntry bookmark_radio_actions[] = {
	{"BookmarkContent", NULL, N_("Content"), NULL, NULL, 0},
	{"BookmarkName", NULL, N_("Name"), NULL, NULL, 1},
	{"BookmarkNameContent", NULL, N_("Name & Content"), NULL, NULL, 2}
};

static const GtkRadioActionEntry bookmark_search_radio_actions[] = {
	{"BookmarkSearchContent", NULL, N_("Content"), NULL, NULL, 0},
	{"BookmarkSearchName", NULL, N_("Name"), NULL, NULL, 1},
	{"BookmarkSearchNameContent", NULL, N_("Name & Content"), NULL, NULL, 2}
};

static void
popup_menu_action_group_init(Tbfwin * bfwin)
{
	GError *error = NULL;

	bfwin->bookmarkGroup = gtk_action_group_new("BookmarkActions");
	gtk_action_group_set_translation_domain(bfwin->bookmarkGroup, GETTEXT_PACKAGE);
	gtk_action_group_add_actions(bfwin->bookmarkGroup, bookmark_actions, G_N_ELEMENTS(bookmark_actions),
								 bfwin);
	gtk_action_group_add_toggle_actions(bfwin->bookmarkGroup, bookmark_toggle_actions,
										G_N_ELEMENTS(bookmark_toggle_actions), bfwin);
	gtk_action_group_add_radio_actions(bfwin->bookmarkGroup, bookmark_file_radio_actions,
									   G_N_ELEMENTS(bookmark_file_radio_actions),
									   bfwin->session->bookmarks_filename_mode,
									   G_CALLBACK(popup_menu_show_file), bfwin);
	gtk_action_group_add_radio_actions(bfwin->bookmarkGroup, bookmark_radio_actions,
									   G_N_ELEMENTS(bookmark_radio_actions),
									   bfwin->session->bookmarks_show_mode,
									   G_CALLBACK(popup_menu_show_bookmark), bfwin);
	gtk_action_group_add_radio_actions(bfwin->bookmarkGroup, bookmark_search_radio_actions,
									   G_N_ELEMENTS(bookmark_search_radio_actions),
									   bfwin->session->bmarksearchmode,
									   G_CALLBACK(popup_search_mode_changed), bfwin);
	gtk_ui_manager_insert_action_group(bfwin->uimanager, bfwin->bookmarkGroup, 1);
	g_object_unref(bfwin->bookmarkGroup);

	gtk_ui_manager_add_ui_from_string(bfwin->uimanager, bookmark_menu_ui, -1, &error);
	if (error != NULL) {
		g_warning("building bookmark menu failed: %s", error->message);
		g_error_free(error);
	}

	gtk_ui_manager_add_ui_from_string(bfwin->uimanager, bookmark_search_menu_ui, -1, &error);
	if (error != NULL) {
		g_warning("building bookmark search menu failed: %s", error->message);
		g_error_free(error);
	}
}

static void
popup_menu(Tbfwin * bfwin, GdkEventButton * event, gboolean show_bmark_specific, gboolean show_file_specific)
{
	GtkWidget *menu = gtk_ui_manager_get_widget(bfwin->uimanager, "/BookmarkMenu");
	if (!menu)
		return;

	bfwin_action_set_sensitive(bfwin->uimanager, "/BookmarkMenu/EditBookmark", show_bmark_specific);
	bfwin_action_set_sensitive(bfwin->uimanager, "/BookmarkMenu/DeleteBookmark", show_bmark_specific);
	bfwin_action_set_sensitive(bfwin->uimanager, "/BookmarkMenu/DeleteAllBookmarkInDoc", show_file_specific);
	bfwin_set_menu_toggle_item_from_path(bfwin->uimanager, "/BookmarkMenu/DefaultPermanent",
										 main_v->globses.bookmarks_default_store);
	gtk_radio_action_set_current_value((GtkRadioAction *)
									   gtk_ui_manager_get_action(bfwin->uimanager,
																 "/BookmarkMenu/ShowFileMenu/BookmarkFileByName"),
									   bfwin->session->bookmarks_filename_mode);
	gtk_radio_action_set_current_value((GtkRadioAction *)
									   gtk_ui_manager_get_action(bfwin->uimanager,
																 "/BookmarkMenu/ShowBookmarkMenu/BookmarkName"),
									   bfwin->session->bookmarks_show_mode);
	gtk_widget_show(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
}

static void
popup_search_menu(Tbfwin * bfwin, GdkEventButton * bevent)
{
	GtkWidget *menu = gtk_ui_manager_get_widget(bfwin->uimanager, "/BookmarkSearchMenu");

	if (menu) {
		gtk_widget_show(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, bevent->button, bevent->time);
	} else
		g_warning("showing bookmark search popup menu failed");
}

static void
bmark_search_icon_press(GtkEntry * entry, GtkEntryIconPosition icon_pos, GdkEvent * event, gpointer user_data)
{
	popup_search_menu(user_data, (GdkEventButton *) event);
}

static void
bmark_row_activated(GtkTreeView * tree, GtkTreePath * path, GtkTreeViewColumn * column, Tbfwin * bfwin)
{
	GtkTreeIter iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(bfwin->bmarkfilter), &iter, path);
	if (gtk_tree_path_get_depth(path) == 2) {
		bmark_activate(bfwin, get_bmark_at_iter(GTK_TREE_MODEL(bfwin->bmarkfilter), &iter), FALSE);
	}
}

/* mouse click */
static gboolean
bmark_event_mouseclick(GtkWidget * widget, GdkEventButton * event, Tbfwin * bfwin)
{
	GtkTreePath *path;
	gboolean show_bmark_specific = FALSE, show_file_specific = FALSE;
	if (gtk_tree_view_get_path_at_pos
		(GTK_TREE_VIEW(bfwin->bmark), event->x, event->y, &path, NULL, NULL, NULL)) {
		if (path) {
			gint depth = gtk_tree_path_get_depth(path);

			if (depth == 2) {
				show_bmark_specific = TRUE;
				show_file_specific = TRUE;
				if (event->button == 1) {
					GtkTreeIter iter;
					gtk_tree_model_get_iter(GTK_TREE_MODEL(bfwin->bmarkfilter), &iter, path);
					bmark_activate(bfwin, get_bmark_at_iter(GTK_TREE_MODEL(bfwin->bmarkfilter), &iter),
								   FALSE);
				}
			} else if (depth == 1) {
				show_file_specific = TRUE;
			}
			gtk_tree_path_free(path);
		}
	}
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {	/* right mouse click */
		popup_menu(bfwin, event, show_bmark_specific, show_file_specific);
	}

	return FALSE;
}

/*static void bmark_selection_changed_lcb(GtkTreeSelection *treeselection,Tbfwin * bfwin) {
	/ * this is not the best way to activate bookmarks. according to the gtk documentation:
	Emitted whenever the selection has (possibly) changed. Please note that this signal is 
	mostly a hint. It may only be emitted once when a range of rows are selected, and it 
	may occasionally be emitted when nothing has happened.
	
	THUS: we should better use the mouse click event to find the correct bookmark to 
	activate.
	* /
	DEBUG_MSG("bmark_selection_changed_lcb, started\n");
	bmark_goto_selected(bfwin);
}*/

static void
bmark_first_lcb(GtkWidget * widget, Tbfwin * bfwin)
{
	GtkTreeModel *model = GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore);
	GtkTreeIter iter;

	if (!CURDOC(bfwin) || !CURDOC(bfwin)->bmark_parent)
		return;
	DEBUG_MSG("bmark_first_lcb, started\n");
	if (gtk_tree_model_iter_children(model, &iter, CURDOC(bfwin)->bmark_parent))
		bmark_activate(bfwin, get_bmark_at_iter(model, &iter), TRUE);
}

static void
bmark_last_lcb(GtkWidget * widget, Tbfwin * bfwin)
{
	GtkTreeModel *model = GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore);
	GtkTreeIter iter;
	gint num;

	if (!CURDOC(bfwin) || !CURDOC(bfwin)->bmark_parent)
		return;
	DEBUG_MSG("bmark_last_lcb, started\n");
	num = gtk_tree_model_iter_n_children(model, CURDOC(bfwin)->bmark_parent);
	if (gtk_tree_model_iter_nth_child(model, &iter, CURDOC(bfwin)->bmark_parent, num - 1))
		bmark_activate(bfwin, get_bmark_at_iter(model, &iter), TRUE);
}

static void
bmark_previous_lcb(GtkWidget * widget, Tbfwin * bfwin)
{
	GtkTextMark *insert;
	GtkTextIter titer;
	Tbmark *bmark = NULL;
	gint line;

	if (!CURDOC(bfwin) || !CURDOC(bfwin)->bmark_parent)
		return;
	DEBUG_MSG("bmark_previous_lcb, started\n");
	insert = gtk_text_buffer_get_insert(CURDOC(bfwin)->buffer);
	gtk_text_buffer_get_iter_at_mark(CURDOC(bfwin)->buffer, &titer, insert);
	/*gtk_text_iter_set_line_offset(&titer, 0); */
	gtk_text_iter_backward_line(&titer);
	line = bmark_margin_get_initial_bookmark(CURDOC(bfwin), &titer, (gpointer) & bmark);
	DEBUG_MSG("bmark_previous_lcb, got initial bookmark at line %d, cursor is at line %d\n", line,
			  gtk_text_iter_get_line(&titer));
	if (-1 == line)
		return;
	if (line > gtk_text_iter_get_line(&titer)) {
		bmark_last_lcb(widget, bfwin);
		return;
	}
	bmark_activate(bfwin, bmark, TRUE);
}

static void
bmark_next_lcb(GtkWidget * widget, Tbfwin * bfwin)
{
	GtkTextMark *insert;
	GtkTextIter titer;
	Tbmark *bmark = NULL;
	gint line;

	if (!CURDOC(bfwin) || !CURDOC(bfwin)->bmark_parent)
		return;
	DEBUG_MSG("bmark_next_lcb, started\n");
	insert = gtk_text_buffer_get_insert(CURDOC(bfwin)->buffer);
	gtk_text_buffer_get_iter_at_mark(CURDOC(bfwin)->buffer, &titer, insert);
	gtk_text_iter_forward_to_line_end(&titer);
	line = bmark_margin_get_initial_bookmark(CURDOC(bfwin), &titer, (gpointer) & bmark);
	if (-1 == line)
		return;
	if (line <= gtk_text_iter_get_line(&titer)) {
		/* get the 'next' bookmark */
		if (-1 == bmark_margin_get_next_bookmark(CURDOC(bfwin), (gpointer) & bmark)) {
			bmark_first_lcb(widget, bfwin);
			return;
		}
	}
	bmark_activate(bfwin, bmark, TRUE);
}

void
bookmark_navigate(Tbfwin * bfwin, guint action)
{
	switch (action) {
	case 1:
		bmark_first_lcb(NULL, bfwin);
		break;
	case 2:
		bmark_previous_lcb(NULL, bfwin);
		break;
	case 3:
		bmark_next_lcb(NULL, bfwin);
		break;
	case 4:
		bmark_last_lcb(NULL, bfwin);
		break;
	default:
		g_warning("invalid menu action for bookmark menu, please report as bluefish bug\n ");
		break;
	}
}

static gboolean
all_children_hidden(GtkTreeModel * model, GtkTreeIter * iter, gpointer data,
					GtkTreeModelFilterVisibleFunc func)
{
	GtkTreeIter citer;
	gboolean cont = TRUE;
	if (!gtk_tree_model_iter_children(model, &citer, iter)) {
		return TRUE;			/* there are no children */
	}
	while (cont) {
		if (func(model, &citer, data) == TRUE)
			return FALSE;
		cont = gtk_tree_model_iter_next(model, &citer);
	}
	return TRUE;
}

static gboolean
bmark_search_filter_func(GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
	Tbfwin *bfwin = data;
	Tbmark *bmark;

	if (bfwin->bmark_search_prefix == NULL || bfwin->bmark_search_prefix[0] == '\0')
		return TRUE;

	if (gtk_tree_model_iter_has_child(model, iter)) {
		if (all_children_hidden(model, iter, data, bmark_search_filter_func)) {
			return FALSE;
		}
		return TRUE;
	}
	gtk_tree_model_get(model, iter, PTR_COLUMN, &bmark, -1);
	if (bmark) {
		switch ( /*bfwin->session->bmark_search_mode */ BM_SEARCH_BOTH) {
		case BM_SEARCH_NAME:
			return (bmark->name && strstr(bmark->name, bfwin->bmark_search_prefix));
			break;
		case BM_SEARCH_CONTENT:
			return (bmark->text && strstr(bmark->text, bfwin->bmark_search_prefix));
			break;
		case BM_SEARCH_BOTH:
			return ((bmark->text && strstr(bmark->text, bfwin->bmark_search_prefix))
					|| (bmark->name && g_str_has_prefix(bmark->name, bfwin->bmark_search_prefix)));
			break;
		}
	}
	return FALSE;
}

static void
bmark_search_changed(GtkEditable * editable, gpointer user_data)
{
	Tbfwin *bfwin = user_data;
	/* call refilter on the bmarkfilter */
	g_free(bfwin->bmark_search_prefix);
	bfwin->bmark_search_prefix = gtk_editable_get_chars(editable, 0, -1);
	gtk_tree_model_filter_refilter(bfwin->bmarkfilter);
}

/* Initialize bookmarks gui for window */
GtkWidget *
bmark_gui(Tbfwin * bfwin)
{
	GtkWidget *vbox, *hbox, *scroll, *entry;
	GtkToolItem *but;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	DEBUG_MSG("bmark_gui, building gui for bfwin=%p\n", bfwin);
	/* Tree Store is in BMARKDATA(bfwin->bmarkdata)->bookmarkstore
	   Tree View is in bfwin->bmark
	 */
	vbox = gtk_vbox_new(FALSE, 1);
	entry = gtk_entry_new();
	gtk_entry_set_icon_from_stock(GTK_ENTRY(entry), GTK_ENTRY_ICON_PRIMARY, GTK_STOCK_FIND);
	gtk_entry_set_icon_activatable(GTK_ENTRY(entry), GTK_ENTRY_ICON_PRIMARY, TRUE);
	g_signal_connect(G_OBJECT(entry), "icon-press", G_CALLBACK(bmark_search_icon_press), bfwin);
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(bmark_search_changed), bfwin);
#if GTK_CHECK_VERSION(3,2,0)
	gtk_entry_set_width_chars(GTK_ENTRY(entry), 1);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, TRUE, 0);
	hbox = gtk_toolbar_new();
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(hbox), GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(hbox), GTK_TOOLBAR_ICONS);

	but = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_GOTO_TOP, GTK_ICON_SIZE_MENU), "");
	g_signal_connect(G_OBJECT(but), "clicked", G_CALLBACK(bmark_first_lcb), bfwin);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(but), _("First bookmark"));
	gtk_toolbar_insert(GTK_TOOLBAR(hbox), but, -1);
	but = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_MENU), "");
	g_signal_connect(G_OBJECT(but), "clicked", G_CALLBACK(bmark_previous_lcb), bfwin);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(but), _("Previous bookmark"));
	gtk_toolbar_insert(GTK_TOOLBAR(hbox), but, -1);
	but = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_MENU), "");
	g_signal_connect(G_OBJECT(but), "clicked", G_CALLBACK(bmark_next_lcb), bfwin);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(but), _("Next bookmark"));
	gtk_toolbar_insert(GTK_TOOLBAR(hbox), but, -1);
	but = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_GOTO_BOTTOM, GTK_ICON_SIZE_MENU), "");
	g_signal_connect(G_OBJECT(but), "clicked", G_CALLBACK(bmark_last_lcb), bfwin);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(but), _("Last bookmark"));
	gtk_toolbar_insert(GTK_TOOLBAR(hbox), but, -1);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	bfwin->bmarkfilter = (GtkTreeModelFilter *)
		gtk_tree_model_filter_new(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), NULL);
	gtk_tree_model_filter_set_visible_func(bfwin->bmarkfilter, bmark_search_filter_func, bfwin, NULL);
	bfwin->bmark = (GtkTreeView *) gtk_tree_view_new_with_model(GTK_TREE_MODEL(bfwin->bmarkfilter));
	g_object_unref(bfwin->bmarkfilter);
	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", cell, "text", NAME_COLUMN, NULL);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(bfwin->bmark), column);
	gtk_widget_show_all(GTK_WIDGET(bfwin->bmark));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(bfwin->bmark), FALSE);
	/*gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(bfwin->bmark), TRUE); */
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), GTK_WIDGET(bfwin->bmark));
	gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(bfwin->bmark), "button-press-event", G_CALLBACK(bmark_event_mouseclick), bfwin);
	g_signal_connect(G_OBJECT(bfwin->bmark), "row-activated", G_CALLBACK(bmark_row_activated), bfwin);
	gtk_tree_view_expand_all(bfwin->bmark);
	/*{
	   GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(bfwin->bmark));
	   gtk_tree_selection_set_mode(selection,GTK_SELECTION_BROWSE);
	   g_signal_connect(G_OBJECT(selection), "changed",G_CALLBACK(bmark_selection_changed_lcb), bfwin);
	   } */

	if (!bfwin->bookmarkGroup)
		popup_menu_action_group_init(bfwin);

	return vbox;
}

/**
 * bmark_get_iter_at_tree_position:
 *
 * determine bookmark's location in the tree and  insert - result GtkTreeIter is stored in m->iter
 */
static void
bmark_get_iter_at_tree_position(Tbfwin * bfwin, Tbmark * m)
{
	GtkTreeIter *parent;
	gpointer ptr;
	DEBUG_MSG("bmark_get_iter_at_tree_position, started\n");
	ptr = g_hash_table_lookup(BMARKDATA(bfwin->bmarkdata)->bmarkfiles, m->uri);
	DEBUG_MSG("bmark_get_iter_at_tree_position, found %p in hashtable %p\n", ptr,
			  BMARKDATA(bfwin->bmarkdata)->bmarkfiles);
	if (ptr == NULL) {			/* closed document or bookmarks never set */
		gchar *title;
		parent = g_slice_new0(GtkTreeIter);
#ifdef BMARKREF
		bmarkref.itercount++;
		g_print("bmark_get_iter_at_tree_position, itercount=%d\n", bmarkref.itercount);
#endif
		/* we should sort the document names in the treestore */
		title = bmark_filename(bfwin, m->uri);
		DEBUG_MSG("insert parent with name %s and doc=%p in treestore %p\n", title, m->doc,
				  BMARKDATA(bfwin->bmarkdata)->bookmarkstore);
		gtk_tree_store_insert_with_values(BMARKDATA(bfwin->bmarkdata)->bookmarkstore, parent, NULL, 0,
										  NAME_COLUMN, title, PTR_COLUMN, m->doc, -1);
/*		gtk_tree_store_append(BMARKDATA(bfwin->bmarkdata)->bookmarkstore, parent, NULL);
		gtk_tree_store_set(BMARKDATA(bfwin->bmarkdata)->bookmarkstore, parent, NAME_COLUMN, title, PTR_COLUMN, m->doc, -1);*/
		g_free(title);
		if (m->doc != NULL) {
			m->doc->bmark_parent = parent;
		}
		DEBUG_MSG("bmark_get_iter_at_tree_position, appending parent %p in hashtable %p\n", parent,
				  BMARKDATA(bfwin->bmarkdata)->bmarkfiles);
		/* the hash table frees the key, but not the value, on destroy */
		g_object_ref(m->uri);
		g_hash_table_insert(BMARKDATA(bfwin->bmarkdata)->bmarkfiles, m->uri, parent);
	} else {
		parent = (GtkTreeIter *) ptr;
	}
	gtk_tree_store_prepend(BMARKDATA(bfwin->bmarkdata)->bookmarkstore, &m->iter, parent);
}

static gint
bmark_sort_func(GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b, gpointer user_data)
{
	GtkTreeIter tmp;
	if (gtk_tree_model_iter_parent(model, &tmp, a) == FALSE) {
		gint retval;
		gchar *name_a, *name_b;
		gtk_tree_model_get(model, a, NAME_COLUMN, &name_a, -1);
		gtk_tree_model_get(model, b, NAME_COLUMN, &name_b, -1);
		retval = g_strcmp0(name_a, name_b);
		g_free(name_a);
		g_free(name_b);
		return retval;
	} else {
		Tbmark *bmark_a, *bmark_b;
		gtk_tree_model_get(model, a, PTR_COLUMN, &bmark_a, -1);
		gtk_tree_model_get(model, b, PTR_COLUMN, &bmark_b, -1);
		if (bmark_a && bmark_b) {
			return bmark_a->offset - bmark_b->offset;
		} else {
			return (gint) (bmark_a - bmark_b);
		}
	}
}

static void
bmark_hash_value_free(gpointer data)
{
	DEBUG_MSG("bmark_hash_value_free, free iter %p\n", data);
#ifdef BMARKREF
	bmarkref.itercount--;
	g_print("bmark_hash_value_free, itercount=%d\n", bmarkref.itercount);
#endif
	g_slice_free(GtkTreeIter, data);
}

static void
bmark_hash_key_free(gpointer data)
{
	if (!data)
		return;
	DEBUG_MSG("bmark_hash_key_free, unref %p\n", data);
	g_object_unref(data);
}

/*
 * this function is used to create the global main_v->bookmarkstore
 * as well as the project bookmarkstores
 */
gpointer
bookmark_data_new(void)
{
	Tbmarkdata *bmd;
	bmd = g_new0(Tbmarkdata, 1);
	bmd->bookmarkstore = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
	gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(bmd->bookmarkstore), bmark_sort_func, bmd,
											NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(bmd->bookmarkstore),
										 GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);
	bmd->bmarkfiles =
		g_hash_table_new_full(g_file_hash, (GEqualFunc) g_file_equal, bmark_hash_key_free,
							  bmark_hash_value_free);
	DEBUG_MSG("bookmark_data_new, created bookmarkstore at %p\n", bmd->bookmarkstore);
	return bmd;
}

/* used to clean up the project bookmarkdata */
gpointer
bookmark_data_cleanup(gpointer data)
{
	Tbmarkdata *bmd = BMARKDATA(data);
	GtkTreeIter fileit;
	gboolean cont;
	/*walk the treestore and free all Tbmark's in the pointer columns */
	DEBUG_MSG("bookmark_data_cleanup bmarkdata %p\n", bmd);
	cont = gtk_tree_model_iter_children(GTK_TREE_MODEL(bmd->bookmarkstore), &fileit, NULL);
	while (cont) {				/* walk the toplevel */
		GtkTreeIter bmit;
		gboolean cont2 = gtk_tree_model_iter_children(GTK_TREE_MODEL(bmd->bookmarkstore), &bmit, &fileit);
		while (cont2) {
			Tbmark *bmark;
			gtk_tree_model_get(GTK_TREE_MODEL(bmd->bookmarkstore), &bmit, PTR_COLUMN, &bmark, -1);
			bmark->strarr = NULL;
			if (bmark->doc)
				bmark->doc->bmark_parent = NULL;
			bmark_free(bmark);
			cont2 = gtk_tree_model_iter_next(GTK_TREE_MODEL(bmd->bookmarkstore), &bmit);
		}
		cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(bmd->bookmarkstore), &fileit);
	}
	g_object_unref(bmd->bookmarkstore);
	g_hash_table_destroy(bmd->bmarkfiles);
	g_free(bmd);
	return NULL;
}

/* this function will load the bookmarks
 * from bfwin->session->bmarks and parse
 * them into treestore BMARKDATA(bfwin->bmarkdata)->bookmarkstore
 *
 * it is called from bluefish.c for the first window (global bookmarks)
 * and from project.c during opening a project
 *
 * this function should ALSO check all douments that are
 * opened (bfwin->documentlist) if they have bookmarks !!
 */
void
bmark_reload(Tbfwin * bfwin)
{
	GFile *cacheduri = NULL;
	GList *tmplist;

	DEBUG_MSG("bmark_reload for bfwin %p\n", bfwin);
	bmark_store_all(bfwin);

	tmplist = g_list_first(bfwin->session->bmarks);
	while (tmplist) {
		gchar **items = (gchar **) tmplist->data;
		if (items && g_strv_length(items) == 6) {
			gchar *ptr;
			Tbmark *b;
			b = g_slice_new0(Tbmark);
			/*g_print("bmark_reload, alloc bmark %p\n",b); */
			b->name = g_strdup(items[0]);
			b->description = g_strdup(items[1]);
			/* convert old (Bf 1.0) bookmarks to new bookmarks with uri's */
			if (strchr(items[2], ':') == NULL) {
				gchar *tmp;
				tmp = g_strconcat("file://", items[2], NULL);
				b->uri = g_file_parse_name(tmp);
				g_free(tmp);
			} else {
				b->uri = g_file_parse_name(items[2]);
			}
			/* because the bookmark list is usually sorted, we try to cache the uri's and consume less memory */
			if (cacheduri && (cacheduri == b->uri || g_file_equal(cacheduri, b->uri))) {
				DEBUG_MSG("bmark_reload, uri %p and %p are identical, unref %p and use %p\n", cacheduri,
						  b->uri, b->uri, cacheduri);
				g_object_unref(b->uri);
				b->uri = g_object_ref(cacheduri);;
			} else {
				DEBUG_MSG("bmark_reload, new uri %p\n", b->uri);
				cacheduri = b->uri;
			}

			b->offset = atoi(items[3]);
			b->text = g_strdup(items[4]);
			b->len = atoi(items[5]);
			b->strarr = items;
			DEBUG_MSG("bmark_reload, loaded bookmark %p for uri=%pat offset %d with text %s\n", b,
					  b->uri, b->offset, b->text);
			bmark_get_iter_at_tree_position(bfwin, b);
			ptr = bmark_showname(bfwin, b);
			gtk_tree_store_set(BMARKDATA(bfwin->bmarkdata)->bookmarkstore, &(b->iter), NAME_COLUMN, ptr,
							   PTR_COLUMN, b, -1);
			g_free(ptr);
		}
		tmplist = g_list_next(tmplist);
	}
#ifdef WALKTREE
	/* walk over all bookmarks and print them to stdout */
	{
		GtkTreeIter iter;
		gboolean cont;
		cont =
			gtk_tree_model_get_iter_first(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &iter);
		while (cont) {
			gchar *name;
			gtk_tree_model_get(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &iter, NAME_COLUMN,
							   &name, -1);
			g_print("walk bookmarks, got name %s\n", name);
			g_free(name);
			cont =
				gtk_tree_model_iter_next(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &iter);
		}
	}
#endif
}

/*
 * this function will simply call
 * gtk_tree_view_set_model() to connect the treeview
 * to the new treestore, used in unloading and
 * loading of projects
 */
void
bmark_set_store(Tbfwin * bfwin)
{
	DEBUG_MSG("bmark_set_store set store %p for bfwin %p\n", BMARKDATA(bfwin->bmarkdata)->bookmarkstore,
			  bfwin);
	if (BMARKDATA(bfwin->bmarkdata)->bookmarkstore && bfwin->bmark) {
		bfwin->bmarkfilter = (GtkTreeModelFilter *)
			gtk_tree_model_filter_new(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), NULL);
		gtk_tree_model_filter_set_visible_func(bfwin->bmarkfilter, bmark_search_filter_func, bfwin, NULL);
		gtk_tree_view_set_model(bfwin->bmark, GTK_TREE_MODEL(bfwin->bmarkfilter));
		g_object_unref(bfwin->bmarkfilter);
	}
}

/* the Tdocument will be closed, but the bookmark should stay in the treestore */
void
bmark_clean_for_doc(Tdocument * doc)
{
	GtkTreeIter tmpiter;
	GtkTreePath *path;
	gboolean cont;

	if (doc->bmark_parent == NULL)
		return;

	if (!doc->uri)
		return;

	if (BFWIN(doc->bfwin)->bmarkdata == NULL)
		return;

	DEBUG_MSG("bmark_clean_for_doc, doc=%p, bfwin=%p, bmarkdata=%p, getting children for parent_iter=%p\n",
			  doc, doc->bfwin, BFWIN(doc->bfwin)->bmarkdata, doc->bmark_parent);
	/* a segfault is reported here, coming from a document save and close */
	cont =
		gtk_tree_model_iter_children(GTK_TREE_MODEL(BMARKDATA(BFWIN(doc->bfwin)->bmarkdata)->bookmarkstore),
									 &tmpiter, doc->bmark_parent);
	while (cont) {
		Tbmark *b = NULL;
		DEBUG_MSG("bmark_clean_for_doc, getting bookmark for first child\n");
		gtk_tree_model_get(GTK_TREE_MODEL(BMARKDATA(BFWIN(doc->bfwin)->bmarkdata)->bookmarkstore), &tmpiter,
						   PTR_COLUMN, &b, -1);
		if (b) {
			bmark_update_offset_from_textmark(b);
			DEBUG_MSG
				("bmark_clean_for_doc, bookmark=%p, new offset=%d, now deleting GtkTextMark from TextBuffer\n",
				 b, b->offset);
			gtk_text_buffer_delete_mark(doc->buffer, b->mark);
			if (doc->fileinfo)
				b->len = gtk_text_buffer_get_char_count(doc->buffer);
			b->mark = NULL;
			b->doc = NULL;
			if (!b->is_temp) {
				bmark_store(doc->bfwin, b);
			}
		}
		cont =
			gtk_tree_model_iter_next(GTK_TREE_MODEL(BMARKDATA(BFWIN(doc->bfwin)->bmarkdata)->bookmarkstore),
									 &tmpiter);
	}							/* cont */
	/* now unset the Tdocument* in the second column */
	DEBUG_MSG("bmark_clean_for_doc, unsetting and freeing parent_iter %p for doc %p\n", doc->bmark_parent,
			  doc);
	gtk_tree_store_set(GTK_TREE_STORE(BMARKDATA(BFWIN(doc->bfwin)->bmarkdata)->bookmarkstore),
					   doc->bmark_parent, PTR_COLUMN, NULL, -1);
	gtk_tree_model_filter_convert_child_iter_to_iter(BFWIN(doc->bfwin)->bmarkfilter, &tmpiter,
													 doc->bmark_parent);
	doc->bmark_parent = NULL;
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bmarkfilter), &tmpiter);
	gtk_tree_view_collapse_row(BFWIN(doc->bfwin)->bmark, path);
	gtk_tree_path_free(path);
}

static gboolean
bookmark_reposition(Tbmark * mark, gint offset)
{
	gint doclen = gtk_text_buffer_get_char_count(mark->doc->buffer);
	gint bandwidth = offset > 0 ? 2 * offset : -2 * offset;
	if (bandwidth < (5 * strlen(mark->text)))
		bandwidth = 5 * strlen(mark->text);
	/* search for the bookmark near the old positions */

	while (TRUE) {
		GtkTextIter its, ite, /*starr,end */ itrs, itre;	/* resultstart, resultend */
		gint startpos;
		startpos = mark->offset + offset - bandwidth / 2;
		if (startpos < 0)
			startpos = 0;
		DEBUG_MSG("bookmark_reposition, searching from %d to %d for %s\n", startpos, startpos + bandwidth,
				  mark->text);
		gtk_text_buffer_get_iter_at_offset(mark->doc->buffer, &its, startpos);
		gtk_text_buffer_get_iter_at_offset(mark->doc->buffer, &ite, startpos + bandwidth);
		if (gtk_text_iter_forward_search(&its, mark->text, GTK_TEXT_SEARCH_TEXT_ONLY, &itrs, &itre, &ite)) {
			/* found !!!!!!! */
			DEBUG_MSG("bookmark_reposition, found result! resposition from %d to %d\n", mark->offset,
					  gtk_text_iter_get_offset(&itrs));
			mark->offset = gtk_text_iter_get_offset(&itrs);
			return TRUE;
		}
		if (bandwidth > doclen) {
			DEBUG_MSG("bookmark_reposition, no result for %s, original offset %d\n", mark->text,
					  mark->offset);
			return FALSE;
		}
		bandwidth *= 2;
	}
}

static gboolean
bookmark_needs_repositioning(Tbmark * mark, GtkTextIter * it)
{
	GtkTextIter it2;
	gboolean retval;
	gchar *tmpstr;
	/* check the content at the bookmark */
	gtk_text_buffer_get_iter_at_offset(mark->doc->buffer, &it2, mark->offset + strlen(mark->text));
	tmpstr = gtk_text_buffer_get_text(mark->doc->buffer, it, &it2, FALSE);
	DEBUG_MSG("original offset %d, compare %s and %s\n", mark->offset, tmpstr, mark->text);
	retval = (strcmp(tmpstr, mark->text) != 0);
	DEBUG_MSG("bookmark_needs_repositioning, reposition=%d,text='%s', tmpstr='%s'\n", retval, mark->text,
			  tmpstr);
	g_free(tmpstr);
	return retval;
}

/*
 * this function will check is this document needs any bookmarks, and set the
 * doc->bmark_parent if needed
 *
 * if there are bookmarks, the bookmark GtkTextMark's will be inserted
 *
 * if check_position is TRUE, the content of the bookmark will be checked, and if
 * changed, the offset will be re-positioned
 *
 */
void
bmark_set_for_doc(Tdocument * doc, gboolean check_positions)
{
	gboolean cont2;
	GtkTreeIter child;
	GtkTreePath *path;

	if (!doc->uri) {
		DEBUG_MSG("bmark_set_for_doc, document %p does not have a filename, returning\n", doc);
		return;
	}

	DEBUG_MSG("bmark_set_for_doc, doc=%p, filename=%s\n", doc, gtk_label_get_text(GTK_LABEL(doc->tab_menu)));
/*	if (!BFWIN(doc->bfwin)->bmark) {
		DEBUG_MSG("bmark_set_for_doc, no leftpanel, not implemented yet!!\n");
		return;
	}*/
	if (doc->bmark_parent) {
		DEBUG_MSG("this document (%p) already has a bmark_parent (%p) why is this function called?\n", doc,
				  doc->bmark_parent);
		return;
	}

	doc->bmark_parent = g_hash_table_lookup(BMARKDATA(BFWIN(doc->bfwin)->bmarkdata)->bmarkfiles, doc->uri);
	if (!doc->bmark_parent)
		return;

	/*g_print("bmark_set_for_doc, we found a bookmark for document %s at offset=%d!\n",gtk_label_get_text(GTK_LABEL(doc->tab_menu)),mark->offset); */
	gtk_tree_store_set(GTK_TREE_STORE(BMARKDATA(BFWIN(doc->bfwin)->bmarkdata)->bookmarkstore),
					   doc->bmark_parent, PTR_COLUMN, doc, -1);

	cont2 =
		gtk_tree_model_iter_children(GTK_TREE_MODEL
									 (BMARKDATA(BFWIN(doc->bfwin)->bmarkdata)->bookmarkstore), &child,
									 doc->bmark_parent);
	while (cont2) {				/* loop the bookmarks for this document  */
		Tbmark *mark = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(BMARKDATA(BFWIN(doc->bfwin)->bmarkdata)->bookmarkstore), &child,
						   PTR_COLUMN, &mark, -1);
		if (mark) {
			GtkTextIter it;
			mark->doc = doc;
			DEBUG_MSG("bmark_set_for_doc, next bookmark at offset=%d!\n", mark->offset);
			gtk_text_buffer_get_iter_at_offset(doc->buffer, &it, mark->offset);
			if (check_positions && bookmark_needs_repositioning(mark, &it)) {	/* repositioning required ! */
				if (bookmark_reposition(mark, gtk_text_buffer_get_char_count(doc->buffer) - mark->len)) {
					gtk_text_buffer_get_iter_at_offset(doc->buffer, &it, mark->offset);
				} else {
					/* BUG: bookmark not restored, what to do now ???? - just put it where it was ??  */
				}
			}
			mark->mark = gtk_text_buffer_create_mark(doc->buffer, NULL, &it, TRUE);
		}
		cont2 =
			gtk_tree_model_iter_next(GTK_TREE_MODEL
									 (BMARKDATA(BFWIN(doc->bfwin)->bmarkdata)->bookmarkstore), &child);
	}

	/* expand it */
	gtk_tree_model_filter_convert_child_iter_to_iter(BFWIN(doc->bfwin)->bmarkfilter, &child,
													 doc->bmark_parent);
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(BFWIN(doc->bfwin)->bmarkfilter), &child);
	gtk_tree_view_expand_row(BFWIN(doc->bfwin)->bmark, path, TRUE);
	gtk_tree_path_free(path);
	bluefish_text_view_set_show_symbols_redraw(BLUEFISH_TEXT_VIEW(doc->view), TRUE);
}

/* this is called by the editor widget to show bookmarks in the left margin.
returns a line number for the Tbmark that bmark points to, or -1 if there is no bmark  */
gint
bmark_margin_get_next_bookmark(Tdocument * doc, gpointer * bmark)
{
	gboolean cont;
	GtkTextIter textit;
	GtkTreeIter treeit = ((Tbmark *) * bmark)->iter;
	cont =
		gtk_tree_model_iter_next(GTK_TREE_MODEL(BMARKDATA(BFWIN(doc->bfwin)->bmarkdata)->bookmarkstore),
								 &treeit);
	if (!cont) {
		return -1;
	}
	gtk_tree_model_get(GTK_TREE_MODEL(BMARKDATA(BFWIN(doc->bfwin)->bmarkdata)->bookmarkstore), &treeit,
					   PTR_COLUMN, bmark, -1);
	gtk_text_buffer_get_iter_at_mark(doc->buffer, &textit, ((Tbmark *) * bmark)->mark);
	return gtk_text_iter_get_line(&textit);
}

/* this is called by the editor widget to show bookmarks in the left margin.
 returns a line number for the Tbmark that bmark points to, or -1 if there is no bmark */
gint
bmark_margin_get_initial_bookmark(Tdocument * doc, GtkTextIter * fromit, gpointer * bmark)
{
	guint offset;
	GtkTextIter textit;
	if (!doc->bmark_parent) {
		return -1;
	}
	offset = gtk_text_iter_get_offset(fromit);
	*bmark = bmark_find_bookmark_before_offset(BFWIN(doc->bfwin), offset, doc->bmark_parent);	/* returns NULL if there is no existing bookmark *before* offset */
	if (!*bmark) {
		GtkTreeIter treeit;
		gboolean retval =
			gtk_tree_model_iter_children(GTK_TREE_MODEL
										 (BMARKDATA(BFWIN(doc->bfwin)->bmarkdata)->bookmarkstore), &treeit,
										 doc->bmark_parent);
		if (!retval) {
			return -1;
		}
		gtk_tree_model_get(GTK_TREE_MODEL(BMARKDATA(BFWIN(doc->bfwin)->bmarkdata)->bookmarkstore), &treeit,
						   PTR_COLUMN, bmark, -1);
	}
	gtk_text_buffer_get_iter_at_mark(doc->buffer, &textit, ((Tbmark *) * bmark)->mark);
	return gtk_text_iter_get_line(&textit);
}

/* this function will simply add the bookmark as defined in the arguments
 *
 * will use offset if itoffset is NULL
 * will use itoffset if not NULL
 */
static Tbmark *
bmark_add_backend(Tdocument * doc, GtkTextIter * itoffset, gint offset, const gchar * name,
				  const gchar * text, gboolean is_temp)
{
	Tbmark *m;
	gchar *displaytext = NULL;
	GtkTextIter it;
	m = g_slice_new0(Tbmark);
	/*g_print("bmark_add_backend, alloc bmark %p\n",m); */
	m->doc = doc;

	if (itoffset) {
		it = *itoffset;
		m->offset = gtk_text_iter_get_offset(&it);
	} else {
		gtk_text_buffer_get_iter_at_offset(doc->buffer, &it, offset);
		m->offset = offset;
	}

	m->mark = gtk_text_buffer_create_mark(doc->buffer, NULL, &it, TRUE);
	m->uri = doc->uri;
	if (m->uri) {
		g_object_ref(m->uri);
	} else {
		gchar *tmp = g_strdup_printf("tmp:///%s", gtk_label_get_text(GTK_LABEL(doc->tab_label)));
		DEBUG_MSG("use uri '%s' for untitled document\n", tmp);
		m->uri = g_file_new_for_uri(tmp);
		g_free(tmp);
	}
		
	m->is_temp = is_temp;
	m->text = g_strdup(text);
	m->name = (name) ? g_strdup(name) : g_strdup("");
	m->description = g_strdup("");

	/* insert into tree */
	bmark_get_iter_at_tree_position(doc->bfwin, m);
	displaytext = bmark_showname(doc->bfwin, m);
	gtk_tree_store_set(BMARKDATA(BFWIN(doc->bfwin)->bmarkdata)->bookmarkstore, &m->iter, NAME_COLUMN,
					   displaytext, PTR_COLUMN, m, -1);
	g_free(displaytext);

	/* and store */
	if (!m->is_temp) {
		bmark_store(BFWIN(doc->bfwin), m);
	}
	DEBUG_MSG("bmark_add_backend, set show symbols to true and call for redraw\n");
	bluefish_text_view_set_show_symbols_redraw(BLUEFISH_TEXT_VIEW(doc->view), TRUE);
	return m;
}

/**
 * bmark_text_for_offset:
 *
 * will use offset if itoffset is NULL
 * will use itoffset if not NULL
 */
static gchar *
bmark_text_for_offset(Tdocument * doc, GtkTextIter * itoffset, gint offset)
{
	GtkTextIter it, eit, sit;
	if (itoffset) {
		it = *itoffset;
	} else {
		gtk_text_buffer_get_iter_at_offset(doc->buffer, &it, offset);
	}
	sit = eit = it;
	gtk_text_iter_forward_to_line_end(&eit);
	gtk_text_iter_forward_chars(&sit, BMARK_SHOW_NUM_TEXT_CHARS);
	if (!gtk_text_iter_in_range(&sit, &it, &eit))
		sit = eit;
#ifdef DEBUG
	{
		gchar *tmp = gtk_text_iter_get_text(&it, &sit);
		DEBUG_MSG("bmark_text_for_offset, text=%s\n", tmp);
		g_free(tmp);
	}
#endif
	return gtk_text_iter_get_text(&it, &sit);
}

/* this function will add a bookmark to the current document at current cursor / selection */
static void
bmark_add_current_doc_backend(Tbfwin * bfwin, const gchar * name, gint offset, gboolean is_temp)
{
	GtkTextIter it, eit, sit;
	Tbmark *m;
	if (!bfwin->current_document)
		return;
	DEBUG_MSG("bmark_add_backend, adding bookmark at offset=%d for bfwin=%p\n", offset, bfwin);
	/* create bookmark */
	gtk_text_buffer_get_iter_at_offset(DOCUMENT(bfwin->current_document)->buffer, &it, offset);
	/* if there is a selection, and the offset is within the selection, we'll use it as text content */
	if (gtk_text_buffer_get_selection_bounds(DOCUMENT(bfwin->current_document)->buffer, &sit, &eit)
		&& gtk_text_iter_in_range(&it, &sit, &eit)) {
		gchar *text = gtk_text_iter_get_text(&sit, &eit);
		m = bmark_add_backend(DOCUMENT(bfwin->current_document), &sit, offset, name, text, is_temp);
		g_free(text);

	} else {
		gchar *text;
		text = bmark_text_for_offset(DOCUMENT(bfwin->current_document), &it, offset);
		m = bmark_add_backend(DOCUMENT(bfwin->current_document), &it, offset, name, text, is_temp);
		g_free(text);
	}
	if (bfwin->bmark) {			/* only if there is a left panel we should do this */
		GtkTreePath *path;
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(BMARKDATA(bfwin->bmarkdata)->bookmarkstore), &m->iter);
		gtk_tree_view_expand_to_path(bfwin->bmark, path);
		gtk_tree_path_free(path);
		gtk_widget_grab_focus(bfwin->current_document->view);
	}
}

/*
can we make this function faster? when adding bookmarks from a search this function uses
a lot of time, perhaps that can be improved
*/
static Tbmark *
bmark_get_bmark_at_iter(Tdocument * doc, GtkTextIter * iter, gint offset)
{
	GtkTextIter sit, eit;
	GtkTreeIter tmpiter;
	gint linenum;
	sit = *iter;
	linenum = gtk_text_iter_get_line(&sit);
	eit = sit;
	gtk_text_iter_set_line_offset(&sit, 0);
	gtk_text_iter_forward_to_line_end(&eit);
	/* check for existing bookmark in this place */
	if (DOCUMENT(doc)->bmark_parent) {
		GtkTextIter testit;
		Tbmark *m, *m2;
		/* the next function is probably the slowest since it jumps through the listmodel 
		   to find the right bookmark */
		m = bmark_find_bookmark_before_offset(BFWIN(doc->bfwin), offset, doc->bmark_parent);
		if (m == NULL) {
			DEBUG_MSG("bmark_get_bmark_at_iter, m=NULL, get first child\n");
			if (gtk_tree_model_iter_children
				(GTK_TREE_MODEL(BMARKDATA(BFWIN(doc->bfwin)->bmarkdata)->bookmarkstore), &tmpiter,
				 doc->bmark_parent)) {
				gtk_tree_model_get(GTK_TREE_MODEL(BMARKDATA(BFWIN(doc->bfwin)->bmarkdata)->bookmarkstore),
								   &tmpiter, PTR_COLUMN, &m2, -1);
				gtk_text_buffer_get_iter_at_mark(doc->buffer, &testit, m2->mark);
				if (gtk_text_iter_get_line(&testit) == linenum) {
					return m2;
				}
			}
		} else {
			gtk_text_buffer_get_iter_at_mark(doc->buffer, &testit, m->mark);
			DEBUG_MSG("bmark_get_bmark_at_iter, m=%p, has linenum=%d\n", m, gtk_text_iter_get_line(&testit));
			if (gtk_text_iter_get_line(&testit) == linenum) {
				return m;
			}
			tmpiter = m->iter;
			if (gtk_tree_model_iter_next
				(GTK_TREE_MODEL(BMARKDATA(BFWIN(doc->bfwin)->bmarkdata)->bookmarkstore), &tmpiter)) {
				gtk_tree_model_get(GTK_TREE_MODEL(BMARKDATA(BFWIN(doc->bfwin)->bmarkdata)->bookmarkstore),
								   &tmpiter, PTR_COLUMN, &m2, -1);
				gtk_text_buffer_get_iter_at_mark(doc->buffer, &testit, m2->mark);
				if (gtk_text_iter_get_line(&testit) == linenum) {
					return m2;
				}
			}
		}
		DEBUG_MSG("bmark_get_bmark_at_iter, nothing found at this line, return NULL\n");
		return NULL;

	}
	DEBUG_MSG("bmark_get_bmark_at_iter, no existing bookmark found, return NULL\n");
	return NULL;
}

static Tbmark *
bmark_get_bmark_at_line(Tdocument * doc, gint line)
{
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_line(doc->buffer, &iter, line);
	return bmark_get_bmark_at_iter(doc, &iter, gtk_text_iter_get_offset(&iter));
}

static Tbmark *
bmark_get_bmark_at_offset(Tdocument * doc, gint offset)
{
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_offset(doc->buffer, &iter, offset);
	return bmark_get_bmark_at_iter(doc, &iter, offset);
}

/**
 * bmark_add_extern
 * @doc: a #Tdocument* with the document
 * @offset: the character position where to set the bookmark
 * @name: a name to set for the bookmark, or NULL for no name
 * @text: the text for the bookmark, or NULL to have it set automatically
 *
 * Code in bluefish that want to set a bookmark, not related to
 * the cursor location or a mouse position should use
 * this function.
 */
void
bmark_add_extern(Tdocument * doc, gint offset, const gchar * name, const gchar * text, gboolean is_temp)
{
	if (!doc)
		return;
	DEBUG_MSG("adding bookmark at offset %d with name %s\n", offset, name);	/* dummy */
	if (!bmark_get_bmark_at_offset(doc, offset)) {
		if (text) {
			bmark_add_backend(doc, NULL, offset, (name) ? name : "", text, is_temp);
		} else {
			gchar *tmp = bmark_text_for_offset(doc, NULL, offset);
			bmark_add_backend(doc, NULL, offset, (name) ? name : "", tmp, is_temp);
			g_free(tmp);
		}
	}
}

void
bmark_toggle(Tdocument * doc, gint offset, const gchar * name, const gchar * text)
{
	Tbmark *bmark;
	if (!doc)
		return;
	bmark = bmark_get_bmark_at_offset(doc, offset);
	if (bmark) {
		bmark_check_remove(BFWIN(doc->bfwin), bmark);	/* check  if we should remove a filename too */
		bmark_unstore(BFWIN(doc->bfwin), bmark);
		bmark_free(bmark);
	} else {
		bmark_add_extern(doc, offset, name, text, !main_v->globses.bookmarks_default_store);
	}
}

void
bmark_add(Tbfwin * bfwin)
{
	GtkTextIter it, it2;
	gint offset;
	gboolean has_mark;
	/* if the left panel is disabled, we simply should add the bookmark to the list, and do nothing else */
	gtk_text_buffer_get_iter_at_mark(DOCUMENT(bfwin->current_document)->buffer, &it,
									 gtk_text_buffer_get_insert(DOCUMENT(bfwin->current_document)->buffer));
	gtk_text_buffer_get_iter_at_mark(DOCUMENT(bfwin->current_document)->buffer, &it2,
									 gtk_text_buffer_get_selection_bound(DOCUMENT
																		 (bfwin->current_document)->buffer));
	gtk_text_iter_order(&it, &it2);
	offset = gtk_text_iter_get_offset(&it);
	/* check for existing bookmark in this place */
	has_mark = (bmark_get_bmark_at_offset(DOCUMENT(bfwin->current_document), offset) != NULL);
	if (has_mark) {
		message_dialog_new(bfwin->main_window,
						   GTK_MESSAGE_ERROR,
						   GTK_BUTTONS_CLOSE,
						   _("Can't add bookmark"), _("You already have a bookmark here!"));
		return;
	}
	bmark_add_current_doc_backend(bfwin, "", offset, !main_v->globses.bookmarks_default_store);
}

gboolean
bmark_have_bookmark_at_stored_bevent(Tdocument * doc)
{
	if (main_v->bevent_doc == doc) {
		return (bmark_get_bmark_at_offset(doc, main_v->bevent_charoffset) != NULL);
	}
	return FALSE;
}

gchar *
bmark_get_tooltip_for_line(Tdocument * doc, gint line)
{
	Tbmark *bmark;
	bmark = bmark_get_bmark_at_line(doc, line);
	if (!bmark || !bmark->text)
		return NULL;
	if (bmark->text && bmark->name)
		return g_markup_printf_escaped("%s %s", bmark->name, bmark->text);
	if (bmark->name)
		return g_markup_escape_text(bmark->name, -1);
	return g_markup_escape_text(bmark->text, -1);
}


/*gchar *
bmark_get_tooltip_for_line(Tdocument *doc, gint line)
{
	Tbmark *bmark;
	bmark = bmark_get_bmark_at_line(doc, line);
	if (!bmark || !bmark->text)
		return NULL;
	if (bmark->text && bmark->name) 
		return g_strconcat(bmark->name," ", bmark->text, NULL);
	if (bmark->name)
		return g_strdup(bmark->name);
	return g_strdup(bmark->text);
}
*/
void
bmark_del_at_bevent(Tdocument * doc)
{
	if (main_v->bevent_doc == doc) {
		Tbmark *b = bmark_get_bmark_at_offset(doc, main_v->bevent_charoffset);
		if (b) {
			DEBUG_MSG("bmark_del_at_bevent, deleting bookmark %p\n", b);
			bmark_check_remove(BFWIN(doc->bfwin), b);	/* check  if we should remove a filename too */
			bmark_unstore(BFWIN(doc->bfwin), b);
			bmark_free(b);
		}
	}
}

void
bmark_add_at_bevent(Tdocument * doc)
{
	/* check for unnamed document */
	if (main_v->bevent_doc == doc) {
		gint offset = main_v->bevent_charoffset;
		/* we have the location */
		bmark_add_current_doc_backend(doc->bfwin, "", offset, !main_v->globses.bookmarks_default_store);
	}
}

/* not used yet
void bmark_del_for_filename(Tbfwin *bfwin, gchar *filename) {
	GtkTreeIter *parent = (GtkTreeIter *)g_hash_table_lookup(BMARKDATA(bfwin->bmarkdata)->bmarkfiles,filename);
	if (parent) {
		bmark_del_children_backend(bfwin, parent);
	}
}
*/

void
bmark_cleanup(Tbfwin * bfwin)
{
	DEBUG_MSG("bmark_cleanup, cleanup for bfwin=%p\n", bfwin);
	bfwin->bmark = NULL;
	bfwin->bmarkfilter = NULL;
	g_free(bfwin->bmark_search_prefix);
	bfwin->bmark_search_prefix = NULL;
#ifdef BMARKREF
	g_print("bmark_cleanup, itercount=%d\n", bmarkref.itercount);
#endif
}
