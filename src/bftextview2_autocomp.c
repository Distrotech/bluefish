/* Bluefish HTML Editor
 * bftextview2_autocomp.c
 *
 * Copyright (C) 2008,2009,2010,2011,2012,2013 Olivier Sessink
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

#include <string.h>
#include <gdk/gdkkeysyms.h>
#include "bluefish.h"
#include "bftextview2_private.h"
#include "bftextview2_scanner.h"
#include "bftextview2_identifier.h"
#include "bftextview2_autocomp.h"
#include "stringlist.h"

/*#undef DBG_AUTOCOMP
#define DBG_AUTOCOMP g_print*/

typedef struct {
	BluefishTextView *btv;
	gchar *prefix;
	gchar *newprefix;
	GtkWidget *win;
	GtkListStore *store;
	GtkTreeView *tree;
	GtkWidget *scroll;
	GtkWidget *reflabel;
	gint listwidth;
	gint w;
	gint h;
	gboolean in_fill; /* TRUE while filling the liststore */
	guint16 contextnum;
} Tacwin;

#define ACWIN(p) ((Tacwin *)(p))

static void
acwin_cleanup(BluefishTextView * btv)
{
	if (btv->autocomp) {
		g_free(ACWIN(btv->autocomp)->prefix);
		gtk_widget_destroy(ACWIN(btv->autocomp)->win);
		g_slice_free(Tacwin, btv->autocomp);
		btv->autocomp = NULL;
	}
}

static gboolean
acwin_move_selection(BluefishTextView * btv, gint keyval)
{
	GtkTreeSelection *selection;
	GtkTreeIter it;
	GtkTreeModel *model;
	GtkTreePath *path;

	selection = gtk_tree_view_get_selection(ACWIN(btv->autocomp)->tree);
	if (gtk_tree_selection_get_selected(selection, &model, &it)) {
		gboolean retval = TRUE;
		gint i, rows = 12, *indices = NULL;
		path = gtk_tree_model_get_path(model, &it);
		indices = gtk_tree_path_get_indices(path);
		switch (keyval) {
		case GDK_KEY_Up:			/* move the selection one up */
			retval = gtk_tree_path_prev(path);
			break;
		case GDK_KEY_Down:
			gtk_tree_path_next(path);
			break;
		case GDK_KEY_Page_Down:
			i = MIN(gtk_tree_model_iter_n_children(model, NULL) - 1, indices[0] + rows);
			gtk_tree_path_free(path);
			path = gtk_tree_path_new_from_indices(i, -1);
			break;
		case GDK_KEY_Page_Up:
			i = MAX(indices[0] - rows, 0);
			gtk_tree_path_free(path);
			path = gtk_tree_path_new_from_indices(i, -1);
			break;
		case GDK_KEY_Home:
			gtk_tree_path_free(path);
			path = gtk_tree_path_new_first();
			break;
		case GDK_KEY_End:
			gtk_tree_path_free(path);
			i = gtk_tree_model_iter_n_children(model, NULL);
			path = gtk_tree_path_new_from_indices(i - 1, -1);
			break;
		default:
			return FALSE;
			break;
		}
		if (gtk_tree_model_get_iter(model, &it, path)) {
			gtk_tree_selection_select_iter(selection, &it);
			gtk_tree_view_scroll_to_cell(ACWIN(btv->autocomp)->tree, path, NULL, FALSE, 0, 0);
		} else
			retval = FALSE;
		gtk_tree_path_free(path);
		return retval;
	} else {
		/* set selection */

	}
	return FALSE;
}

/* returns the number of bytes
that are already present in the text.

this is used to avoid for example inserting "return;" in a location where ';' is already the
character directly beyond the insert position.
*/
static gint
get_existing_end_len(BluefishTextView * btv, const gchar *string, gint prefix_bytelen)
{
	gchar *tmp;
	GtkTextIter it1, it2;
	gint i;
	gint string_len = g_utf8_strlen(string, -1);

	gtk_text_buffer_get_iter_at_mark(btv->buffer, &it1, gtk_text_buffer_get_insert(btv->buffer));
	it2 = it1;
	DBG_AUTOCOMP("get_existing_end_len, forward %d chars\n",string_len - prefix_bytelen);
	gtk_text_iter_forward_chars(&it2,string_len - prefix_bytelen);
	DBG_AUTOCOMP("get the text %d:%d\n",gtk_text_iter_get_offset(&it1),gtk_text_iter_get_offset(&it2));
	tmp = gtk_text_buffer_get_text(btv->buffer, &it1, &it2, TRUE);
	g_print("got tmp='%s'\n",tmp);
	i = strlen(tmp);
	do {
		if (strncmp(string+prefix_bytelen+i, tmp, i)==0) {
			DBG_AUTOCOMP("get_existing_end_len, found %d existing characters\n",i);
			g_free(tmp);
			return i;
		}
		i--;
	} while(i>0);
	g_free(tmp);
	DBG_AUTOCOMP("get_existing_end_len, found no existing characters\n");
	return 0;
}

gboolean
acwin_check_keypress(BluefishTextView * btv, GdkEventKey * event)
{
	DBG_AUTOCOMP("got keyval %c\n", event->keyval);
	if ((event->state & GDK_CONTROL_MASK) || (event->state & GDK_MOD1_MASK)) {
		DBG_AUTOCOMP("a modifier is active (state=%d), don't handle the keypress\n", event->state);
		return FALSE;
	}
	switch (event->keyval) {
	case GDK_KEY_Return:{
			GtkTreeSelection *selection;
			GtkTreeIter it;
			GtkTreeModel *model;
			BluefishTextView *master = BLUEFISH_TEXT_VIEW(btv->master);
			selection = gtk_tree_view_get_selection(ACWIN(btv->autocomp)->tree);
			if (selection && gtk_tree_selection_get_selected(selection, &model, &it)) {
				gchar *string;
				gint stringlen;
				gint existing_len, prefix_len;
				guint pattern_id;
				gint backup_chars = 0;
				gtk_tree_model_get(model, &it, 1, &string, -1);
				/*g_print("context %d has patternhash %p, string=%s\n",ACWIN(btv->autocomp)->contextnum, g_array_index(btv->bflang->st->contexts, Tcontext, ACWIN(btv->autocomp)->contextnum).patternhash, string); */
				if (g_array_index
					(master->bflang->st->contexts, Tcontext, ACWIN(btv->autocomp)->contextnum).patternhash) {
					/*g_print("looking in context %d patternhash for '%s'\n",ACWIN(btv->autocomp)->contextnum, string); */
					pattern_id =
						GPOINTER_TO_INT(g_hash_table_lookup
										(g_array_index
										 (master->bflang->st->contexts, Tcontext,
										  ACWIN(btv->autocomp)->contextnum).patternhash, string));
					DBG_AUTOCOMP("got pattern_id=%d\n", pattern_id);
					if (pattern_id) {
						GSList *tmpslist =
							g_array_index(master->bflang->st->matches, Tpattern, pattern_id).autocomp_items;
						/* a pattern MAY have multiple autocomplete items. This code is not efficient iof in the future some
						   patterns would have many autocomplete items. I don't expect this, so I leave this as it is right now  */
						while (tmpslist) {
							Tpattern_autocomplete *pac = tmpslist->data;
							if (g_strcmp0(string, pac->autocomplete_string) == 0) {
								backup_chars = pac->autocomplete_backup_cursor;
							}
							tmpslist = g_slist_next(tmpslist);
						}
					}
				}
				stringlen = strlen(string);
				prefix_len = strlen(ACWIN(btv->autocomp)->prefix);
				existing_len = get_existing_end_len(btv, string, prefix_len);

				DBG_AUTOCOMP("acwin_check_keypress: ENTER: insert %s\n",
							 string + prefix_len);
				string[stringlen - existing_len] = '\0';
				gtk_text_buffer_insert_at_cursor(btv->buffer, string + prefix_len, -1);
				if (backup_chars != 0) {
					GtkTextIter iter;
					gtk_text_buffer_get_iter_at_mark(btv->buffer, &iter, gtk_text_buffer_get_insert(btv->buffer));
					if (backup_chars-existing_len > 0 && gtk_text_iter_backward_chars(&iter, backup_chars-existing_len)) {
						DBG_AUTOCOMP("move cursor %d chars back!\n", backup_chars);
						gtk_text_buffer_place_cursor(btv->buffer, &iter);
					}
				}

				g_free(string);
			}
			acwin_cleanup(btv);
			return TRUE;
		}
		break;
	case GDK_KEY_Tab:
		if (ACWIN(btv->autocomp)->newprefix
			&& strlen(ACWIN(btv->autocomp)->newprefix) > strlen(ACWIN(btv->autocomp)->prefix)) {
			gtk_text_buffer_insert_at_cursor(gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv)),
											 ACWIN(btv->autocomp)->newprefix +
											 strlen(ACWIN(btv->autocomp)->prefix), -1);
		}
		/* now we have to re-position the autocomplete window */
		autocomp_run(btv, FALSE);
		return TRUE;
		break;
	case GDK_KEY_Right:
	case GDK_KEY_KP_Right:
	case GDK_KEY_Left:
	case GDK_KEY_KP_Left:
		acwin_cleanup(btv);
		return FALSE;
		break;
	case GDK_KEY_Escape:
		acwin_cleanup(btv);
		return TRUE;
		break;
	case GDK_KEY_Up:
	case GDK_KEY_Down:
	case GDK_KEY_Page_Down:
	case GDK_KEY_Page_Up:
	case GDK_KEY_Home:
	case GDK_KEY_End:
		if (acwin_move_selection(btv, event->keyval))
			return TRUE;
		break;
	}

	return FALSE;
}

static void
acw_selection_changed_lcb(GtkTreeSelection * selection, Tacwin * acw)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	BluefishTextView *master = BLUEFISH_TEXT_VIEW(acw->btv->master);
	DBG_AUTOCOMP("acw_selection_changed_lcb, in_fill=%d\n",acw->in_fill);
	if (acw->in_fill)
		return;

	if (!g_array_index(master->bflang->st->contexts, Tcontext, acw->contextnum).patternhash
		|| !main_v->props.show_autocomp_reference)
		return;

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gchar *key;
		gtk_tree_model_get(model, &iter, 1, &key, -1);
		if (key) {
			gint pattern_id =
				GPOINTER_TO_INT(g_hash_table_lookup
								(g_array_index
								 (master->bflang->st->contexts, Tcontext, acw->contextnum).patternhash, key));
			g_free(key);
			if (pattern_id && g_array_index(master->bflang->st->matches, Tpattern, pattern_id).reference) {
				GtkRequisition requisition;
				DBG_AUTOCOMP("acw_selection_changed_lcb, show %s\n",
							 g_array_index(master->bflang->st->matches, Tpattern, pattern_id).reference);
				gtk_label_set_markup(GTK_LABEL(acw->reflabel),
									 g_array_index(master->bflang->st->matches, Tpattern,
												   pattern_id).reference);
				gtk_widget_show(acw->reflabel);
#if GTK_CHECK_VERSION(3,0,0)
				gtk_widget_get_preferred_size(acw->reflabel, &requisition, NULL);
#else
				gtk_widget_size_request(acw->reflabel, &requisition);
#endif
				/*gtk_window_get_size(GTK_WINDOW(acw->win),&width,&height); */
				acw->w = acw->listwidth + requisition.width + 2;
				gtk_widget_set_size_request(acw->win, acw->w, -1);
				return;
			}
		}
	}
	gtk_widget_hide(acw->reflabel);
	acw->w = acw->listwidth;
	gtk_widget_set_size_request(acw->win, acw->listwidth, -1);
}

static Tacwin *
acwin_create(BluefishTextView * btv)
{
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	GtkWidget *hbox;
	/* GtkWidget *vbar; */
	Tacwin *acw;
	GtkTreeSelection *selection;

	acw = g_slice_new0(Tacwin);
	acw->btv = btv;
	acw->in_fill = TRUE;
	acw->win = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_widget_set_app_paintable(acw->win, TRUE);
	gtk_window_set_resizable(GTK_WINDOW(acw->win), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(acw->win), 1);
	gtk_window_set_decorated(GTK_WINDOW(acw->win), FALSE);
	gtk_window_set_type_hint(GTK_WINDOW(acw->win), GDK_WINDOW_TYPE_HINT_POPUP_MENU);

	acw->store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	acw->tree = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(acw->store)));
	g_object_unref(acw->store);

	gtk_tree_view_set_headers_visible(acw->tree, FALSE);
	acw->scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(acw->scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", cell, "markup", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(acw->tree), column);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(acw->tree), FALSE);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(acw->tree));
	g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(acw_selection_changed_lcb), acw);

/*	gtk_tree_view_set_search_column(GTK_TREE_VIEW(acw->tree),1);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(acw->tree),acwin_tree_search_lcb,prefix,NULL);*/

	/*g_signal_connect_swapped(GTK_WINDOW(acw->win),"expose-event",G_CALLBACK(ac_paint),acw->win); */
	/*gtk_window_set_position (GTK_WINDOW(acw->win), GTK_WIN_POS_MOUSE); */
	gtk_container_add(GTK_CONTAINER(acw->scroll), GTK_WIDGET(acw->tree));

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), acw->scroll, FALSE, TRUE, 0);
	acw->reflabel = gtk_label_new(NULL);
	gtk_label_set_line_wrap(GTK_LABEL(acw->reflabel), TRUE);
	gtk_misc_set_alignment(GTK_MISC(acw->reflabel), 0.1, 0.1);
	gtk_box_pack_start(GTK_BOX(hbox), acw->reflabel, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(acw->win), hbox);
	/*gtk_widget_set_size_request(acw->reflabel,150,-1); */
	gtk_widget_show_all(acw->scroll);
	gtk_widget_show(hbox);
	/*gtk_widget_set_size_request(GTK_WIDGET(acw->tree),100,200); */
	/*gtk_widget_set_size_request(acw->win, 150, 200); */
	/*g_signal_connect(G_OBJECT(acw->win),"key-release-event",G_CALLBACK(acwin_key_release_lcb),acw); */

	return acw;
}

/* returns TRUE if window is popped-up lower than the cursor,
returns FALSE if window is popped-up higher than the cursor (because cursor is low in the screen) */
static gboolean
acwin_position_at_cursor(BluefishTextView * btv)
{
	GtkTextIter it;
	GdkRectangle rect;
	GdkScreen *screen;
	gint x, y, sh;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
	screen = gtk_widget_get_screen(GTK_WIDGET(btv));

	gtk_text_buffer_get_iter_at_mark(buffer, &it, gtk_text_buffer_get_insert(buffer));
	gtk_text_view_get_iter_location(GTK_TEXT_VIEW(btv), &it, &rect);
	gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(btv), GTK_TEXT_WINDOW_TEXT, rect.x, rect.y, &rect.x,
										  &rect.y);
	gdk_window_get_origin(gtk_text_view_get_window(GTK_TEXT_VIEW(btv), GTK_TEXT_WINDOW_TEXT), &x, &y);

	sh = gdk_screen_get_height(screen);
	DBG_AUTOCOMP("rect.y+y=%d, acw->h=%d, sh=%d\n", rect.y + y, ACWIN(btv->autocomp)->h, sh);
	if (rect.y + y + ACWIN(btv->autocomp)->h > sh) {
		gtk_window_move(GTK_WINDOW(ACWIN(btv->autocomp)->win), rect.x + x,
						rect.y + y + rect.height - ACWIN(btv->autocomp)->h);
		return FALSE;
	} else {
		gtk_window_move(GTK_WINDOW(ACWIN(btv->autocomp)->win), rect.x + x, rect.y + y);
		return TRUE;
	}
}

static void
acwin_calculate_window_size(Tacwin * acw, GList * items, GList * items2, const gchar *closetag, gint *numitems)
{
	GList *tmplist;
	gboolean runtwo = FALSE;
	gchar *longest = NULL, *tmp;
	guint longestlen = 1;
	*numitems = 0;
	DBG_AUTOCOMP("acwin_calculate_window_size, items=%p, items2=%p, closetag=%s\n", items, items2, closetag);
	if (closetag) {
		longest = g_markup_escape_text(closetag, -1);
		longestlen = strlen(longest);
	}
	tmplist = g_list_first(items);
	while (!runtwo || tmplist) {
		guint len;
		if (!tmplist) {
			tmplist = g_list_first(items2);
			runtwo = TRUE;
			if (!tmplist)
				break;
		}
		g_assert(tmplist != NULL);
		g_assert(tmplist->data != NULL);
		DBG_AUTOCOMP("acwin_calculate_window_size, tmplist=%p", tmplist);
		DBG_AUTOCOMP(", tmplist->data=%p",tmplist->data);
		DBG_AUTOCOMP("=%s\n", (gchar *)tmplist->data);
		tmp = g_markup_escape_text(tmplist->data, -1);
		len = strlen(tmp);
		if (len > longestlen) {
			longest = tmp;
			longestlen = len;
		} else {
			g_free(tmp);
		}
		(*numitems)++;
		tmplist = g_list_next(tmplist);
	}
	if (longest) {
		gint len, rowh;
		PangoLayout *panlay = gtk_widget_create_pango_layout(GTK_WIDGET(acw->tree), NULL);
		DBG_AUTOCOMP("longest=%s\n", longest);
		pango_layout_set_markup(panlay, longest, -1);
		pango_layout_get_pixel_size(panlay, &len, &rowh);
		acw->h = MIN(MAX((*numitems + 1) * rowh + 8, 150), 350);
		acw->w = acw->listwidth = MIN(len + 20, 350);
		DBG_AUTOCOMP("numitems=%d, rowh=%d, new height=%d, listwidth=%d\n", *numitems, rowh, acw->h,
				acw->listwidth);
		gtk_widget_set_size_request(GTK_WIDGET(acw->scroll), acw->listwidth, acw->h);	/* ac_window */
		g_free(longest);
		g_object_unref(G_OBJECT(panlay));
	}
}

static int
ac_sort_func(const gchar *a, const gchar *b)
{
	if (a && b && a[0] == '<' && b[0] == '<' && a[1] != b[1]) {
		/* make sure that </ is sorted below <[a-z] */
		if (a[1] == '/') return 1;
		if (b[1] == '/') return -1;
		return a[1]-b[1];
	}
	return g_strcmp0(a,b);
}

/* fills the tree */
static void
acwin_fill_tree(Tacwin * acw, GList * items, GList * items2, gchar * closetag, gboolean reverse, gint numitems)
{
	GList *tmplist, *list = NULL;
	gint everynth=1, i=0;

	/* to avoid that we show 10000 items in the autocomplete list, we insert every N items where N % everynth == 0 */
	if (numitems > 512) {
		everynth = 1.0 * numitems / 512.0;
		DBG_AUTOCOMP("numitems=%d, everynth=%d\n", numitems, everynth);
	}
	if (items)
		list = g_list_copy(items);
	if (items2)
		list = g_list_concat(g_list_copy(items2), list);

	list = g_list_sort(list, (GCompareFunc) ac_sort_func);
	if (closetag) {
		GList *tlist2;
		tlist2 = find_in_stringlist(list, closetag);
		if (tlist2) {
			list = g_list_remove_link(list, tlist2);
			list = g_list_concat(tlist2, list);
		} else {
			list = g_list_prepend(list, closetag);
		}
	}
	if (reverse) {
		list = g_list_reverse(list);
		DBG_AUTOCOMP("reverse list!\n");
	}
	tmplist = g_list_first(list);
	while (tmplist) {
		if (i % everynth == 0) {
			GtkTreeIter it;
			gchar *tmp;
			gtk_list_store_append(acw->store, &it);
			tmp = g_markup_escape_text(tmplist->data, -1);
			gtk_list_store_set(acw->store, &it, 0, tmp, 1, tmplist->data, -1);
			DBG_AUTOCOMP("acwin_fill_tree, add item %s\n",tmp);
			g_free(tmp);
		}
		i++;
		tmplist = g_list_next(tmplist);
	}
	g_list_free(list);
}

/* static void print_ac_items(GCompletion *gc) {
	GList *tmplist = g_list_first(gc->items);
	DBG_AUTOCOMP("autocompletion has %d items:",g_list_length(g_list_first(gc->items)));
	while (tmplist) {
		DBG_AUTOCOMP(" %s",(char *)tmplist->data);
		tmplist = g_list_next(tmplist);
	}
	DBG_AUTOCOMP("\n");
} */

void
autocomp_stop(BluefishTextView * btv)
{
	acwin_cleanup(btv);
}

void
autocomp_run(BluefishTextView * btv, gboolean user_requested)
{
	GtkTextIter cursorpos, iter;
	BluefishTextView *master = BLUEFISH_TEXT_VIEW(btv->master);
	gint contextnum;
	gunichar uc;
	Tfoundblock *fblock = NULL;	/* needed for the special case to close generix xml tags based on the top of the blockstack */

	if (G_UNLIKELY(!master->bflang || !master->bflang->st))
		return;

	gtk_text_buffer_get_iter_at_mark(btv->buffer, &cursorpos, gtk_text_buffer_get_insert(btv->buffer));

	iter = cursorpos;
	gtk_text_iter_set_line_offset(&iter, 0);


	scan_for_autocomp_prefix(master, &iter, &cursorpos, &contextnum);
	DBG_AUTOCOMP("autocomp_run, got possible match start at %d in context %d, cursor is at %d\n",
				 gtk_text_iter_get_offset(&iter), contextnum, gtk_text_iter_get_offset(&cursorpos));
	/* see if character at cursor is end or symbol */
	uc = gtk_text_iter_get_char(&cursorpos);
	if (G_UNLIKELY(uc > NUMSCANCHARS))
		return;

	/*identstate = g_array_index(master->bflang->st->contexts, Tcontext, contextnum).identstate;*/
	if (!character_is_symbol(master->bflang->st,contextnum,uc)) {
		/* current character is not a symbol! */
		DBG_AUTOCOMP("autocomp_run, character at cursor %d '%c' is not a symbol, return\n", uc, (char) uc);
		acwin_cleanup(btv);
		return;
	}

	/* see if we have enough characters */
	if (!user_requested && gtk_text_iter_get_offset(&cursorpos) - gtk_text_iter_get_offset(&iter) < main_v->props.autocomp_min_prefix_len) {
		DBG_AUTOCOMP("autocomp_run, prefix len %d < autocomp_min_prefix_len (%d), abort!\n"
					, gtk_text_iter_get_offset(&cursorpos) - gtk_text_iter_get_offset(&iter)
					, main_v->props.autocomp_min_prefix_len);
		acwin_cleanup(btv);
		return;
	}

	if (g_array_index(master->bflang->st->contexts, Tcontext, contextnum).has_tagclose_from_blockstack) {
		Tfound *found;
		GSequenceIter *siter = NULL;
		found = get_foundcache_at_offset(master, gtk_text_iter_get_offset(&cursorpos), &siter);
		if (found) {
			fblock =
				found->numblockchange < 0 ? pop_blocks(found->numblockchange, found->fblock) : found->fblock;
			if (fblock && fblock->start2_o != BF_OFFSET_UNDEFINED) {
				g_print("abort offering closing tag: block has an end already\n");
				fblock = NULL;
				DBG_AUTOCOMP("blockstack has pattern %d on top, with tagclose_from_blockstack=%d\n",
						 fblock->patternum, g_array_index(master->bflang->st->matches, Tpattern,
														  fblock->patternum).tagclose_from_blockstack);
			}
/*		if (g_array_index(btv->bflang->st->matches, Tpattern, fblock->patternum).tagclose_from_blockstack) {
				gchar *start;
				gtk_text_buffer_get_iter_at_mark(buffer, &it1, fblock->start1);
				gtk_text_buffer_get_iter_at_mark(buffer, &it2, fblock->end1);
				gtk_text_iter_forward_char(&it1);
				start = gtk_text_buffer_get_text(buffer,&it1,&it2,TRUE);
				g_print("close tag %s\n",start);
				g_free(start);
			}*/
		}
	}
	if ((user_requested || !gtk_text_iter_equal(&iter, &cursorpos))
		&& (g_array_index(master->bflang->st->contexts, Tcontext, contextnum).ac != NULL
			|| (fblock
				&& g_array_index(master->bflang->st->matches, Tpattern,
								 fblock->patternum).tagclose_from_blockstack)
		)
		) {
		/* we have a prefix or it is user requested, and we have a context with autocompletion or we have blockstack-tag-auto-closing */
		gchar *newprefix = NULL, *prefix, *closetag = NULL;
		GList *items = NULL, *items2 = NULL;
		/*print_ac_items(g_array_index(btv->bflang->st->contexts,Tcontext, contextnum).ac); */

		prefix = gtk_text_buffer_get_text(btv->buffer, &iter, &cursorpos, TRUE);

		if (fblock) {
			GString *tmpstr;
			gint plen;
			GtkTextIter it1;
			gtk_text_buffer_get_iter_at_offset(btv->buffer, &it1, fblock->start1_o);
			tmpstr = g_string_new("</");
			while(gtk_text_iter_forward_char(&it1)) {
				gunichar uc = gtk_text_iter_get_char(&it1);
				if (!g_unichar_isalnum(uc) && uc != '_') {
					break;
				}
				g_string_append_c(tmpstr, uc);
			}
			g_string_append_c(tmpstr, '>');
			closetag = g_string_free(tmpstr, FALSE);
			DBG_AUTOCOMP("closetag=%s, prefix=%s\n", closetag, prefix);
			plen = strlen(prefix);
			if (plen == strlen(closetag) || strncmp(closetag, prefix, plen) != 0) {
				g_free(closetag);
				closetag = NULL;
			}
		}
		if (g_array_index(master->bflang->st->contexts, Tcontext, contextnum).ac) {
			items =
				g_completion_complete(g_array_index(master->bflang->st->contexts, Tcontext, contextnum).ac,
									  prefix, &newprefix);
			DBG_AUTOCOMP("got %d autocompletion items for prefix %s in context %d, newprefix=%s\n",
						 g_list_length(items), prefix, contextnum, newprefix);
#ifdef IDENTSTORING
			{
				GCompletion *compl = identifier_ac_get_completion(master, contextnum, FALSE);
				DBG_IDENTIFIER("got completion %p for context %d\n", compl, contextnum);
				if (compl) {
					gchar *newprefix2 = NULL;
					items2 = g_completion_complete(compl, prefix, &newprefix2);
					DBG_IDENTIFIER("got %d identifier_items for prefix %s, newprefix=%s\n", g_list_length(items2), prefix, newprefix2);
					if (!newprefix)
						newprefix = newprefix2;
					else
						g_free(newprefix2);
				}
			}
#endif
		}
		if (closetag || items2
			|| (items != NULL && (items->next != NULL || strcmp(items->data, prefix) != 0))) {
			/* do not popup if there are 0 items, and also not if there is 1 item which equals the prefix */
			GtkTreeSelection *selection;
			GtkTreeIter it;
			gboolean below;
			gint numitems=0;
			/* create the GUI */
			if (!btv->autocomp) {
				btv->autocomp = acwin_create(btv);
			} else {
				ACWIN(btv->autocomp)->in_fill=TRUE;
				g_free(ACWIN(btv->autocomp)->prefix);
				g_free(ACWIN(btv->autocomp)->newprefix);
				ACWIN(btv->autocomp)->prefix = NULL;
				ACWIN(btv->autocomp)->newprefix = NULL;
				gtk_list_store_clear(ACWIN(btv->autocomp)->store);
			}
			ACWIN(btv->autocomp)->contextnum = contextnum;
			ACWIN(btv->autocomp)->prefix = g_strdup(prefix);
			if (newprefix) {
				ACWIN(btv->autocomp)->newprefix = g_strdup(newprefix);
			}
			acwin_calculate_window_size(ACWIN(btv->autocomp), items, items2, closetag, &numitems);
			below = acwin_position_at_cursor(btv);
			acwin_fill_tree(ACWIN(btv->autocomp), items, items2, closetag, !below, numitems);
			gtk_widget_show(ACWIN(btv->autocomp)->win);
			selection = gtk_tree_view_get_selection(ACWIN(btv->autocomp)->tree);
			if (below)
				gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ACWIN(btv->autocomp)->store), &it);
			else {
				GtkTreePath *path;
				gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(ACWIN(btv->autocomp)->store), &it, NULL,
											  gtk_tree_model_iter_n_children(GTK_TREE_MODEL
																			 (ACWIN(btv->autocomp)->store),
																			 NULL) - 1);
				path = gtk_tree_model_get_path(GTK_TREE_MODEL(ACWIN(btv->autocomp)->store), &it);
				gtk_tree_view_scroll_to_cell(ACWIN(btv->autocomp)->tree, path, NULL, FALSE, 0.0, 0.0);
				gtk_tree_path_free(path);
			}
			ACWIN(btv->autocomp)->in_fill=FALSE;
			gtk_tree_selection_select_iter(selection, &it);
			g_free(closetag);
		} else {
			acwin_cleanup(btv);
		}
		g_free(newprefix);
		g_free(prefix);
	} else {
		DBG_AUTOCOMP("no autocompletion data for context %d (ac=%p), or no prefix\n", contextnum,
					 g_array_index(master->bflang->st->contexts, Tcontext, contextnum).ac);
		acwin_cleanup(btv);
	}
}
