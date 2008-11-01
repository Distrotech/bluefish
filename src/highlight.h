/* Bluefish HTML Editor
 * highlight.h - highlighting in gtk2 text widget
 *
 * Copyright (C) 2002-2003 Olivier Sessink
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

#ifndef __HIGHLIGHT_H_
#define __HIGHLIGHT_H_

void filetype_highlighting_rebuild(gboolean gui_errors);
void hl_init(void);
void doc_highlight_full(Tdocument *doc);
void doc_highlight_region(Tdocument * doc, guint startof, guint endof);
void doc_highlight_line(Tdocument *doc);
void doc_remove_highlighting(Tdocument *doc);

GtkTextTagTable *highlight_return_tagtable();

#endif /* __HIGHLIGHT_H_ */
