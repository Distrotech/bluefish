/* Bluefish HTML Editor
 * bftextview2_spell.h
 *
 * Copyright (C) 2009 Olivier Sessink
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
#ifndef _BFTEXTVIEW2_SPELL_H_
#define _BFTEXTVIEW2_SPELL_H_

#include "bftextview2.h"

gboolean bftextview2_run_spellcheck(BluefishTextView * btv);
void bftextview2_spell_init(void);
void bftextview2_spell_cleanup(void);
void bftextview2_populate_suggestions_popup(GtkMenu *menu, Tdocument *doc);
void bftextview2_populate_preferences_popup(GtkMenu *menu, Tdocument *doc);
void bfspell_gui_toggle_spell_check(GtkWidget *widget, gpointer data);
#endif
