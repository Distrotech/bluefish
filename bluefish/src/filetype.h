/* Bluefish HTML Editor
 * filetype.h
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

#ifndef __FILETYPE_H_
#define __FILETYPE_H_

#include "bluefish.h"
#if !GTK_CHECK_VERSION(2,14,0)
GdkPixbuf *get_pixbuf_for_gicon(GIcon *icon);
#endif
GdkPixbuf *get_icon_for_mime_type (const char *mime_type);
Tfiletype *get_filetype_for_mime_type(const gchar *mime_type);
#ifdef HAVE_ATLEAST_GIO_2_16
gchar *get_mimetype_for_uri(GFile *uri, GFileInfo *finfo, gboolean fast);
Tfiletype *get_filetype_for_uri(GFile *uri, GFileInfo *finfo, gboolean fast);
#else /* GnomeVFS */
const gchar *get_mimetype_for_uri(GnomeVFSURI *uri, gboolean fast);
Tfiletype *get_filetype_for_uri(GnomeVFSURI *uri, gboolean fast);
#endif /* #else HAVE_ATLEAST_GIO_2_16 */

void filetype_highlighting_rebuild(gboolean gui_errors);

#endif /* __FILETYPE_H_ */
