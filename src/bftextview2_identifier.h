/* Bluefish HTML Editor
 * bftextview2_identifier.h
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

/* for the design docs see bftextview2.h */
#ifndef _BFTEXTVIEW2_IDENTIFIER_H_
#define _BFTEXTVIEW2_IDENTIFIER_H_

#include "bftextview2.h"

typedef struct {
	gpointer doc;
	guint line;
} Tjumpdata;
#define JUMPDATA(var) ((Tjumpdata *)var)

typedef struct {
	gpointer bflang;
	gint16 context;
	gchar *name;
} Tjumpkey;
#define JUMPKEY(var) ((Tjumpkey *)var)

typedef struct {
	gpointer bflang;
	gint16 context;
} Tackey;
#define ACKEY(var) ((Tackey *)var)

void bftextview2_identifier_hash_remove_doc(gpointer bfwin, gpointer doc);
void bftextview2_identifier_hash_destroy(gpointer bfwin);
void bftextview2_identifier_hash_init(gpointer bfwin);
gchar *bf_get_identifier_at_iter(BluefishTextView * btv, GtkTextIter * iter, gint * context);
Tjumpdata *bftextview2_lookup_identifier(gpointer bfwin, BluefishTextView * btv, gint context,
										 const gchar * text);



/* only called internally within bftextview2 */
GCompletion *identifier_ac_get_completion(BluefishTextView * btv, gint16 context, gboolean create);
void found_identifier(BluefishTextView * btv, GtkTextIter * start, GtkTextIter * end, gint16 context, guint8 identaction);

#endif							/* _BFTEXTVIEW2_IDENTIFIER_H_ */
