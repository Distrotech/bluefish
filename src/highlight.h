/* Bluefish HTML Editor
 * highlight.h - highlighting in gtk2 text widget
 *
 * Copyright (C) 2002-2005 Olivier Sessink
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

#ifdef USE_SCANNER

#include "bf-textview.h"
void hl_token_slot(BfTextView *view, BfLangToken *tokenDef, GtkTextIter *startIter,GtkTextIter *endIter);

void hl_block_slot(BfTextView * view, BfLangBlock * blockDef, GtkTextIter * b_startIter,
                   GtkTextIter * b_endIter, GtkTextIter * e_startIter,
                   GtkTextIter * e_endIter, GtkTextMark * startMark, GtkTextMark * endMark);

void hl_tag_begin_slot(BfTextView * view, gchar * tagName, GtkTextIter * startIter, GtkTextIter * endIter); 

void hl_tag_end_slot(BfTextView * view, gchar * tagName, GtkTextIter * b_startIter,
                     GtkTextIter * b_endIter, GtkTextIter * e_startIter, GtkTextIter * e_endIter);
                     
void hl_tag_attr_slot(BfTextView * view, gchar * attrName, gchar * attrValue,
                      GtkTextIter * n_startIter, GtkTextIter * n_endIter,
                      GtkTextIter * v_startIter, GtkTextIter * v_endIter);

#else    /* USE_SCANNER */

void doc_highlight_region(Tdocument * doc, guint startof, guint endof);
void doc_highlight_line(Tdocument *doc);
void doc_remove_highlighting(Tdocument *doc);

#endif    /* USE_SCANNER */

void filetype_highlighting_rebuild(gboolean gui_errors);			   
void hl_init(void);
void doc_highlight_full(Tdocument *doc);
GtkTextTagTable *highlight_return_tagtable();

#endif /* __HIGHLIGHT_H_ */
