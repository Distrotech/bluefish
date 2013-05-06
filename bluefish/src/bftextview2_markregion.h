/* Bluefish HTML Editor
 * bftextview2_markregion.h
 *
 * Copyright (C) 2013 Olivier Sessink
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

#ifndef _BFTEXTVIEW2_MARKREGION_H_
#define _BFTEXTVIEW2_MARKREGION_H_

#ifdef MARKREGION
void markregion_region_done(Tregions *rg, guint end);
void markregion_insert(Tregions *rg, guint markstart, guint markend);
void markregion_delete(Tregions *rg, guint markstart, guint markend, gint offset);
void markregion_nochange(Tregions *rg, guint markstart, guint markend);
gpointer markregion_get_region(Tregions *rg, gpointer cur, guint *start, guint *end);
#endif

#endif /* _BFTEXTVIEW2_MARKREGION_H_ */
