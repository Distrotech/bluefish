/* Bluefish HTML Editor
 * filebrowser.h the filebrowser
 *
 * Copyright (C) 2002 Olivier Sessink
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __FILEBROWSER_H_
#define __FILEBROWSER_H_

void filebrowser_refresh_dir(gchar *dir);
void filebrowser_open_dir(const gchar *dirarg);
void filebrowser_filters_rebuild();
GtkWidget *filebrowser_init();
void filebrowser_cleanup();
void filebrowser_scroll_initial();
#endif /* __FILEBROWSER_H_ */
