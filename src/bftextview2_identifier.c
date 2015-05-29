/* Bluefish HTML Editor
 * bftextview2_identifier.c
 *
 * Copyright (C) 2010-2011 Olivier Sessink
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

#include "bluefish.h"
#include "bftextview2.h"
#include "bftextview2_private.h"
#include "bftextview2_identifier.h"

/*#undef DBG_IDENTIFIER
#define DBG_IDENTIFIER g_print*/

static gboolean
identifier_jump_equal(gconstpointer k1, gconstpointer k2)
{
	if (JUMPKEY(k1)->bflang != JUMPKEY(k2)->bflang)
		return FALSE;
	if (JUMPKEY(k1)->context != JUMPKEY(k2)->context)
		return FALSE;
	if (g_strcmp0(JUMPKEY(k1)->name, JUMPKEY(k2)->name) != 0)
		return FALSE;
	return TRUE;
}

static guint
identifier_jump_hash(gconstpointer v)
{
	gint tmp = g_str_hash(JUMPKEY(v)->name) * g_direct_hash(JUMPKEY(v)->bflang) * JUMPKEY(v)->context;
	return g_int_hash(&tmp);
}

static void
identifier_jump_key_free(gpointer p)
{
	DBG_IDENTIFIER("identifier_jump_key_free %p\n", p);
	g_free(JUMPKEY(p)->name);
	g_slice_free(Tjumpkey, p);
}

static void
identifier_jump_data_free(gpointer p)
{
	DBG_IDENTIFIER("identifier_jump_data_free\n");
	g_slice_free(Tjumpdata, p);
}

static gboolean
identifier_ac_equal(gconstpointer k1, gconstpointer k2)
{
	if (ACKEY(k1)->bflang != ACKEY(k2)->bflang)
		return FALSE;
	if (ACKEY(k1)->context != ACKEY(k2)->context)
		return FALSE;
	return TRUE;
}

static guint
identifier_ac_hash(gconstpointer v)
{
	gint bflanghash = g_direct_hash(ACKEY(v)->bflang) * ACKEY(v)->context;
	return g_int_hash(&bflanghash);
}

static void
identifier_ac_key_free(gpointer p)
{
	DBG_IDENTIFIER("identifier_ac_key_free\n");
	g_slice_free(Tackey, p);
}

static void
identifier_ac_data_free(gpointer p)
{
	DBG_IDENTIFIER("identifier_ac_data_free\n");
	g_completion_free(p);
}

static gboolean
identifier_remove_by_doc(gpointer key, gpointer value, gpointer user_data)
{
	if (JUMPDATA(value)->doc == user_data)
		return TRUE;
	return FALSE;
}

void
bftextview2_identifier_hash_remove_doc(gpointer bfwin, gpointer doc)
{
	GHashTableIter iter;
	gpointer key, value;
	DBG_IDENTIFIER("bftextview2_identifier_hash_remove_doc, start for bfwin=%p, doc=%p\n", bfwin, doc);
	/* iterate of the jump table to find the strings that have to be removed
	   from the GCompletion structures in the autocompletion table */
	g_hash_table_iter_init(&iter, BFWIN(bfwin)->identifier_jump);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		if (JUMPDATA(value)->doc == doc) {
			Tackey iak;
			GCompletion *compl;

			iak.bflang = JUMPKEY(key)->bflang;
			iak.context = JUMPKEY(key)->context;
			compl = g_hash_table_lookup(BFWIN(bfwin)->identifier_ac, &iak);
			if (compl) {
				GList *items = g_list_append(NULL, JUMPKEY(key)->name);
				DBG_IDENTIFIER("remove item %p(%s)\n", JUMPKEY(key)->name, JUMPKEY(key)->name);
				g_completion_remove_items(compl, items);
				g_list_free(items);
			}
		}
	}

	g_hash_table_foreach_remove(BFWIN(bfwin)->identifier_jump, identifier_remove_by_doc, doc);
	DBG_IDENTIFIER("bftextview2_identifier_hash_remove_doc, done for bfwin=%p, doc=%p\n", bfwin, doc);
}

void
bftextview2_identifier_hash_destroy(gpointer bfwin)
{
	g_hash_table_destroy(BFWIN(bfwin)->identifier_jump);
	BFWIN(bfwin)->identifier_jump = NULL;
	g_hash_table_destroy(BFWIN(bfwin)->identifier_ac);
	BFWIN(bfwin)->identifier_ac = NULL;
}

void
bftextview2_identifier_hash_init(gpointer bfwin)
{
	BFWIN(bfwin)->identifier_jump =
		g_hash_table_new_full(identifier_jump_hash, identifier_jump_equal, identifier_jump_key_free,
							  identifier_jump_data_free);
	BFWIN(bfwin)->identifier_ac =
		g_hash_table_new_full(identifier_ac_hash, identifier_ac_equal, identifier_ac_key_free,
							  identifier_ac_data_free);
}

Tjumpdata *
bftextview2_lookup_identifier(gpointer bfwin, BluefishTextView * btv, gint context, const gchar * text)
{
	Tjumpkey ijk;
	Tjumpdata *ijd;

	ijk.bflang = btv->bflang;
	ijk.context = context;
	ijk.name = (gchar *) text;
	ijd = g_hash_table_lookup(BFWIN(bfwin)->identifier_jump, &ijk);
	return ijd;
}

static Tjumpkey *
identifier_jumpkey_new(gpointer bflang, gint16 context, gchar * name)
{
	Tjumpkey *ijk = g_slice_new0(Tjumpkey);
	ijk->bflang = bflang;
	ijk->context = context;
	ijk->name = name;			/* don't dup the string, we allocate that memory only once and re-use the same block in the completion and in this key  */
	return ijk;
}

static Tjumpdata *
identifier_jumpdata_new(Tdocument * doc, guint line)
{
	Tjumpdata *ijd = g_slice_new0(Tjumpdata);
	ijd->doc = doc;
	ijd->line = line;
	return ijd;
}

GCompletion *
identifier_ac_get_completion(BluefishTextView * btv, gint16 context, gboolean create)
{
	Tackey iak;
	GCompletion *compl;
	iak.bflang = btv->bflang;
	iak.context = context;
	compl = g_hash_table_lookup(BFWIN(DOCUMENT(btv->doc)->bfwin)->identifier_ac, &iak);
	if (!compl && create) {
		Tackey *iakp = g_slice_new0(Tackey);
		*iakp = iak;
		compl = g_completion_new(NULL);
		g_hash_table_insert(BFWIN(DOCUMENT(btv->doc)->bfwin)->identifier_ac, iakp, compl);
	}
	return compl;
}

void
found_identifier(BluefishTextView * btv, GtkTextIter * start, GtkTextIter * end, gint16 context, guint8 identaction)
{
	Tjumpkey *ijk;
	Tjumpdata *ijd, *oldijd;
	GCompletion *compl;
	gchar *tmp;
	GList *items;
	gboolean freetmp=TRUE;

	tmp = gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv)), start, end, TRUE);
	DBG_IDENTIFIER("found identifier %s at %p, identaction=%d\n", tmp, tmp, identaction);
	if (identaction & 1) {
		ijk = identifier_jumpkey_new(btv->bflang, context, tmp);
		oldijd = g_hash_table_lookup(BFWIN(DOCUMENT(btv->doc)->bfwin)->identifier_jump, ijk);
		if (oldijd) {
			DBG_IDENTIFIER("found identifier, %s already exists\n", tmp);
			/* it exists, now only update the line number, don't add to the completion */
			if (oldijd->doc == btv->doc)
				oldijd->line = gtk_text_iter_get_line(end) + 1;
			identifier_jump_key_free(ijk);	/* that will free tmp as well */
			return; /* if it has identaction 2 it should exist in the completion already */
		} else {
			DBG_IDENTIFIER("found identifier, %s is new\n", tmp);
			ijd = identifier_jumpdata_new(DOCUMENT(btv->doc), gtk_text_iter_get_line(end) + 1);
			g_hash_table_insert(BFWIN(DOCUMENT(btv->doc)->bfwin)->identifier_jump, ijk, ijd);
			freetmp=FALSE;
		}
	}
	if (identaction & 2) {
		gboolean havecompl=FALSE;
		DBG_IDENTIFIER("freetmp=%d for identifier %s\n", freetmp, tmp);
		compl = identifier_ac_get_completion(btv, context, TRUE);
		if (freetmp!=FALSE) { /* if tmp was new as jumpkey, it cannot exist in the autocompletion, so we don't have to check it */
			/* see if we have this item already */
			GList *tmplist;
			DBG_IDENTIFIER("look if %s is in the autocompletion list\n", tmp);
			for (tmplist=g_list_first(compl->items);tmplist;tmplist=g_list_next(tmplist)) {
				if (g_strcmp0(tmp, tmplist->data)==0) {
					havecompl=TRUE;
					DBG_IDENTIFIER("identifier %s exists already\n", tmp);
					break;
				}
			}
		}
		if (!havecompl) {
			items = g_list_prepend(NULL, tmp);
			DBG_IDENTIFIER("add identifier %s to completion %p for context %d\n",tmp,compl,context);
			g_completion_add_items(compl, items);
			g_list_free(items);
			freetmp=FALSE;
		}
	}
	if (freetmp) {
		DBG_IDENTIFIER("tmp is not stored somewhere, free tmp\n");
		g_free(tmp);
	}
}


