/* Bluefish HTML Editor
 * doc_text_tools.h - text editing tools header
 *
 * Copyright (C) 2008-2012 Olivier Sessink
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

#ifndef __DOC_TEXT_TOOLS_H_
#define __DOC_TEXT_TOOLS_H_

void strip_trailing_spaces(Tdocument *doc);
void join_lines(Tdocument *doc);
void split_lines(Tdocument *doc);
void rewrap_lines(Tdocument *doc);
void convert_identing(Tdocument *doc, gboolean to_tabs);
void toggle_comment(Tdocument *doc);
void convert_to_columns(Tdocument *doc);
void select_between_matching_block_boundaries(Tdocument *doc);
void duplicate_line(Tdocument *doc);
void delete_line(Tdocument *doc);
void doc_move_selection(Tdocument *doc, gboolean up, gboolean curline_if_no_selection);
void doc_insert_filename(Tdocument *doc, gboolean relative);
#endif							/* __DOC_TEXT_TOOLS_H_ */
