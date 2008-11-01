/* Bluefish HTML Editor
 * rpopup.h
 *
 * Copyright (C) 2003-2004 Olivier Sessink
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

#ifndef __RPOPUP_H_
#define __RPOPUP_H_

gboolean rpopup_bevent_in_html_code(Tdocument *doc, GtkTextIter *iter);
gboolean rpopup_doc_located_tag(Tdocument *doc);
gboolean rpopup_doc_located_color(Tdocument *doc);
void rpopup_edit_tag_cb(GtkMenuItem *menuitem,Tdocument *doc);
void rpopup_edit_color_cb(GtkMenuItem *menuitem,Tdocument *doc);
void edit_tag_under_cursor_cb(Tbfwin *bfwin);

#endif /* __RPOPUP_H_ */
