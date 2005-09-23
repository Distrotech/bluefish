/* Bluefish HTML Editor
 * filebrowser.h - the filebrowser
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

#ifndef __FILEBROWSER_H_
#define __FILEBROWSER_H_

void bfwin_filebrowser_refresh_dir(Tbfwin *bfwin, gchar *dir);
void filebrowser_open_dir(Tbfwin *bfwin, const gchar *dirarg);
void filebrowser_filters_rebuild(void);
GtkWidget *filebrowser_init(Tbfwin *bfwin);
void filebrowser_cleanup(Tbfwin *bfwin);
void filebrowser_scroll_initial(Tbfwin *bfwin);
void filebrowserconfig_init(void);
void filebrowser_set_basedir(Tbfwin *bfwin, const gchar *basedir);
#endif /* __FILEBROWSER_H_ */
