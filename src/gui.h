/* Bluefish HTML Editor
 * gui.h - global function for the GUI
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
#ifndef __GUI_H_
#define __GUI_H_

void notebook_changed(void);
void gui_create_main();
void statusbar_message(gchar *message, gint time);
gboolean switch_to_document_by_index(gint index);
gboolean switch_to_document_by_pointer(Tdocument *document);
gboolean switch_to_document_by_filename(gchar *filename);

#endif /* __GUI_H_ */
