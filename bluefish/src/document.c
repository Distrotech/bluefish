/* Bluefish HTML Editor
 * document.c - the document
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * Copyright (C) 1999-2002 Olivier Sessink
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
#include <gtk/gtk.h>


#include "bluefish.h"

Tdocument *doc_new() {
	Tdocument *newdoc = g_new0(Tdocument, 1);

	newdoc->hl = hl_get_highlightset_by_filename(NULL);
	newdoc->buffer = gtk_text_buffer_new(newdoc->hl->tagtable);
	newdoc->view = gtk_text_view_new_with_buffer(newdoc->buffer);

	main_v->documentlist = g_list_append(main_v->documentlist, newdoc);

	gtk_notebook_append_page(GTK_NOTEBOOK(main_v->notebook), newdoc->view,newdoc->tab_label);
	gtk_widget_show(tmptable);
	
	return newdoc;
}




