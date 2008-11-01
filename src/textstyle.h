/* Bluefish HTML Editor
 * textstyle.h - color styles used for syntax highlighting
 * 
 * Copyright (C) 2005 Olivier Sessink
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

#include <gtk/gtk.h>
#ifndef __TEXTSTYLE_H__
#define __TEXTSTYLE_H__

#ifdef DEBUG_TEXTTAG
#define DEBUG_TEXTTAG_MSG g_print
#else /* not DEBUG_TEXTTAG */
#ifdef __GNUC__
#define DEBUG_TEXTTAG_MSG(args...)
 /**/
#else/* notdef __GNUC__ */
extern void g_none(gchar *first, ...);
#define DEBUG_TEXTTAG_MSG g_none
#endif /* __GNUC__ */
#endif /* DEBUG_TEXTTAG */
void textstyle_build_lookup_table(void);
void textstyle_cleanup_lookup_table(void);

void textstyle_rebuild(void);
GtkTextTagTable *textstyle_return_tagtable(void);

/* this function is *not* designed to run very often. It is designed 
to run *once* for each style, on start, or after 'OK' or 'apply' in the
preferences panel.
*/
GtkTextTag *textstyle_get(const gchar *name);

/* returns a widget that can be used in the preferences panel */
GtkWidget* textstyle_combo_box(GList *configstyles);
GtkTextTag *get_tag_for_scanner_style(const gchar *filetype,const gchar *type,const gchar *name, const gchar *defaultstyle);

#endif /* __TEXTSTYLE_H__ */
