/* Bluefish HTML Editor
 * bftextview2_langmgr.h
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

#ifndef _BFTEXTVIEW2_LANGMGR_H_
#define _BFTEXTVIEW2_LANGMGR_H_

#include "bftextview2.h"
const gchar *lookup_user_option(const gchar * lang, const gchar * option);
void langmgr_add_custom_mime(Tbflang *bflang, const gchar *mime);
void langmgr_reload_user_options(void);
void langmgr_reload_user_styles(void);
void langmgr_reload_user_highlights(void);
gchar *langmgr_get_option_description(const gchar *optionname);
GtkTextTag *langmrg_lookup_tag_highlight(const gchar * lang, const gchar * highlight);
GtkTextTagTable *langmgr_get_tagtable(void);
Tbflang *langmgr_get_bflang(const gchar * mimetype, const gchar * filename);
GList *langmgr_get_languages_mimetypes(void);
gchar *langmgr_get_language_name_by_mimetype(gchar * mime);
gboolean langmgr_in_highlight_tags(GtkTextTag * tag);
#ifdef HAVE_LIBENCHANT
GtkTextTag **langmgr_no_spellcheck_tags(void);
GtkTextTag **langmgr_need_spellcheck_tags(void);
#endif
GList *langmgr_get_languages(void);
void langmgr_init(void);
void langmgr_cleanup(void);
#endif							/* _BFTEXTVIEW2_LANGMGR_H_ */
