/* Bluefish HTML Editor
 * snippets_gui.h - plugin for snippets sidebar
 *
 * Copyright (C) 2006-2011 Olivier Sessink
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
#ifndef SNIPPET_GUI_H
#define SNIPPET_GUI_H

void snippets_rebuild_accelerators(void);
void snippets_show_as_menu(Tsnippetswin *snw, gboolean enable);
void snippets_sidepanel_initgui(Tbfwin *bfwin);
void snippets_create_gui(Tbfwin *bfwin);

#endif /* SNIPPET_GUI_H */
