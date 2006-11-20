/* Bluefish HTML Editor
 * snippets_gui.c - plugin for snippets sidebar
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
#define DEBUG

#include <string.h>

#include "snippets.h"

#include "snippets_gui.h"


void snippets_sidepanel_initgui(Tbfwin *bfwin) {
	Tsnippetswin *snw;
	DEBUG_MSG("snippets_sidepanel_initgui, bfwin=%p\n",bfwin);
	
	snw = g_new0(Tsnippetswin,1);
	snw->bfwin = bfwin;
	g_hash_table_insert(snippets_v.lookup,bfwin,snw);	
	DEBUG_MSG("snippets_sidepanel_initgui, snw=%p, store=%p\n",snw,snippets_v.store);
	snw->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(snippets_v.store));
	gtk_notebook_append_page(bfwin->leftpanel_notebook,snw->view,gtk_label_new(_("snippets")));
}

void snippets_sidepanel_destroygui(Tbfwin *bfwin) {
	/* the widget is auto destroyed, and there is nothing more to destroy */
}
