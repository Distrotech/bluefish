/* Bluefish HTML Editor
 * bftextview2_spell.h
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

/* for the design docs see bftextview2.h */
#ifndef _BFTEXTVIEW2_SPELL_H_
#define _BFTEXTVIEW2_SPELL_H_

#include "bftextview2.h"
void unload_spell_dictionary(Tbfwin * bfwin);
gboolean bftextview2_run_spellcheck(BluefishTextView * btv);
void bftextview2_spell_init(void);
void bftextview2_spell_cleanup(void);
void bftextview2_populate_suggestions_popup(GtkMenu * menu, Tdocument * doc);
void bftextview2_populate_preferences_popup(GtkMenu * menu, Tdocument * doc);
void bftextview2_gui_toggle_spell_check(GtkWidget * widget, gpointer data);
void reload_spell_dictionary(Tbfwin * bfwin);
#endif
