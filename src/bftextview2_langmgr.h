/* Bluefish HTML Editor
 * bftextview2_langmgr.h
 *
 * Copyright (C) 2008 Olivier Sessink
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

#ifndef _BFTEXTVIEW2_LANGMGR_H_
#define _BFTEXTVIEW2_LANGMGR_H_

#include "bftextview2.h"
void langmgr_reload_user_styles(void);
void langmgr_reload_user_highlights(void);
GtkTextTag *langmrg_lookup_tag_highlight(const gchar *lang, const gchar *highlight);
GtkTextTagTable *langmgr_get_tagtable(void);
Tbflang *langmgr_get_bflang_for_mimetype(const gchar *mimetype);
GList *langmgr_get_languages(void);
void langmgr_init(void);
void langmgr_cleanup(void);
#endif /* _BFTEXTVIEW2_LANGMGR_H_ */
