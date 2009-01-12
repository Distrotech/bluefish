/* Bluefish HTML Editor
 * bftextview2_scanner.h
 *
 * Copyright (C) 2008-2009 Olivier Sessink
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* for the design docs see bftextview2.h */
#ifndef _BFTEXTVIEW2_SCANNER_H_
#define _BFTEXTVIEW2_SCANNER_H_

#include "bftextview2.h"

Tfoundstack *get_stackcache_next(BluefishTextView * bt2, GSequenceIter ** siter);
Tfoundstack *get_stackcache_first(BluefishTextView * bt2, GSequenceIter ** retsiter);
Tfoundstack *get_stackcache_at_position(BluefishTextView * bt2, GtkTextIter *position, GSequenceIter ** retsiter);
guint16 get_context_and_startposition(BluefishTextView * bt2, GtkTextIter *position);
gboolean bftextview2_run_scanner(BluefishTextView * btv, GtkTextIter *visible_end);
void scan_for_prefix_start(BluefishTextView *btv, guint16 contextnum, GtkTextIter *start, GtkTextIter *cursor);
void scan_for_autocomp_prefix(BluefishTextView *btv,GtkTextIter *mstart,GtkTextIter *cursorpos,gint *contextnum);
gboolean scan_for_tooltip(BluefishTextView *btv,GtkTextIter *mstart,GtkTextIter *position,gint *contextnum);
void cleanup_scanner(BluefishTextView *btv);
void scancache_destroy(BluefishTextView *btv);

#endif
