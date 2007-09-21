/* Bluefish HTML Editor
 * filefilter.h
 *
 * Copyright (C) 2006 Olivier Sessink
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

#ifndef __FILEFILTER_H_
#define __FILEFILTER_H_
gboolean mime_visible_in_filter(Tfilter *filter, const gchar *mime_type);
gboolean file_visible_in_filter(Tfilter *filter, const gchar *mime_type, const gchar *filename);
Tfilter *find_filter_by_name(const gchar *name);
void filter_delete(Tfilter *filter);
void filters_rebuild(void);
void filefilter_gui(Tfilter *filter);

#endif
