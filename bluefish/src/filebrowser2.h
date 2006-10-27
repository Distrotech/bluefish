/* Bluefish HTML Editor
 * filebrowser2.h - the filebrowser v2
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

#ifndef __FILEBROWSER2_H_
#define __FILEBROWSER2_H_

void fb2_refresh_dir_from_uri(GnomeVFSURI *dir);
void fb2_refresh_parent_of_uri(GnomeVFSURI *child_uri);
void fb2_focus_document(Tbfwin *bfwin, Tdocument *doc);
void fb2_update_settings_from_session(Tbfwin *bfwin);
GtkWidget *fb2_init(Tbfwin *bfwin);
void fb2_cleanup(Tbfwin *bfwin);
void fb2_unset_filter(Tbfwin *bfwin, Tfilter *filter);
void fb2config_init(void);
void fb2config_cleanup(void);
#endif /* __FILEBROWSER2_H_ */
