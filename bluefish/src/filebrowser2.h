/* Bluefish HTML Editor
 * filebrowser2.h
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __FILEBROWSER2_H_
#define __FILEBROWSER2_H_

void fb2_focus_document(Tbfwin *bfwin, Tdocument *doc);
void fb2_set_basedir(Tbfwin *bfwin, gchar *curi);
GtkWidget *fb2_init(Tbfwin *bfwin);
void fb2_cleanup(Tbfwin *bfwin);
void fb2_filters_rebuild(void);
void fb2config_init(void);

#endif /* __FILEBROWSER2_H_ */
