/* Bluefish HTML Editor
 * doc_text_tools.c - text tools
 *
 * Copyright (C) 2008-2010 Olivier Sessink
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

#include "bluefish.h"
#include "bf_lib.h"
#include "gtk_easy.h"
#include "document.h"
#include "undo_redo.h"
#include "stringlist.h"

/* special replace: strip trailing spaces */
/* use a very simple loop, one that knows whitespace, non-whitespace and a newline */
void strip_trailing_spaces(Tdocument *doc) {
	gint i=0,wstart=0,coffset=0;
	gint start,end;
	gchar *buf;
	
	if (!doc_get_selection(doc, &start, &end)) {
		start=0;
		end=-1;
	}
	buf = doc_get_chars(doc,start,end);
	coffset=start;
	
	doc_unre_new_group(doc);
	while (buf[i] != '\0') {
		switch (buf[i]) {
		case ' ':
		case '\t':
			/* do nothing */
		break;
		case '\n':
			if (wstart+1 < i) {
				gint cstart, cend;
				cstart = utf8_byteoffset_to_charsoffset_cached(buf, wstart+1);
				cend = utf8_byteoffset_to_charsoffset_cached(buf, i);
				doc_replace_text_backend(doc, "", cstart+coffset, cend+coffset);
				coffset -= (cend-cstart);
			}
		/* no break, fall trough */
		default:
			wstart=i;
		break;
		}
		i++;
	}
	g_free(buf);
	doc_unre_new_group(doc);
}

void join_lines(Tdocument *doc) {
	gint i=0,j,cstart,start,end,coffset;
	gchar *buf;
	
	if (!doc_get_selection(doc, &start, &end)) {
		start=0;
		end=-1;
	}
	coffset=start;
	buf = doc_get_chars(doc,start,end);
	doc_unre_new_group(doc);
	while (buf[i] != '\0') {
		if (buf[i] == '\n' || buf[i] == '\r') {
			cstart = utf8_byteoffset_to_charsoffset_cached(buf, i);
			j=i+1;
			/* remove indenting for the next line too */
			while (buf[j]=='\r'||buf[j]=='\n'||buf[j]==' '||buf[j]=='\t')
				j++;
			doc_replace_text_backend(doc, " ", cstart+coffset, cstart+(j-i)+coffset);
			coffset += (1 - (j-i));
		}
		i++;
	}
	g_free(buf);
	doc_unre_new_group(doc);
}

void split_lines(Tdocument *doc) {
	gint coffset=0,count=0,start,end,tabsize;
	gint startws=0, endws=0,starti=0,endi=-1,requested_size; /* ws= whitespace, i=indenting */
	gint charpos;
	gchar *buf,*p;
	gunichar c;
	tabsize = doc_get_tabsize(doc);
	if (!doc_get_selection(doc, &start, &end)) {
		start=0;
		end=-1;
	}
	
	p = buf = doc_get_chars(doc,start,end);
	utf8_offset_cache_reset();
	doc_unre_new_group(doc);
	
	requested_size = 80;
	charpos=start;
	c=g_utf8_get_char(p);
	while (c != '\0') {
		if (count > requested_size) {
			gchar *new_indenting, *tmp1, *tmp2;
			tmp1 = g_utf8_offset_to_pointer(buf, starti);
			tmp2 = g_utf8_offset_to_pointer(buf, endi);
			new_indenting = g_strndup(tmp1, (tmp2-tmp1));
			DEBUG_MSG("startws=%d,endws=%d\n",startws,endws);
			doc_replace_text_backend(doc, new_indenting, startws+coffset, endws+coffset);
			coffset += (g_utf8_strlen(new_indenting,-1) - (endws-startws));
			DEBUG_MSG("new coffset=%d\n",coffset);
			count=0;
			startws=charpos;
			endws=charpos;
			g_free(new_indenting);
		}
		if (c == '\t') {
			count += tabsize;
			if (startws<endws) {
				startws=charpos;
				endws=charpos;
				DEBUG_MSG("set startws to %d\n",startws);
			}
		} else if (c == ' ') {
			count++;
			if (startws<endws) {
				startws=charpos;
				endws=charpos;
				DEBUG_MSG("set startws to %d\n",startws);
			}
		} else if (c=='\n') {
			count=0;
			starti=charpos;
			endws=startws=charpos+1;
			DEBUG_MSG("newline, set starti=%d, endws=startws=%d\n",starti,endws);
		} else {
			count++;
			if (starti>endi) {
				endi=charpos;
				DEBUG_MSG("non-whitespace, set endi to %d, starti=%d\n",endi,starti);
			} else if (startws>=endws) {
				endws=charpos;
				DEBUG_MSG("non-whitespace, set endws to %d\n",endws);
			}
		}
		p = g_utf8_next_char(p);
		charpos++;
		c=g_utf8_get_char(p);
	}
	g_free(buf);
	doc_unre_new_group(doc);

}

void rewrap_lines(Tdocument *doc) {
	join_lines(doc);
	split_lines(doc);
}

/* from spaces to tabs or from tabs to spaces */
void convert_identing(Tdocument *doc, gboolean to_tabs) {
	gint i=0,wstart=0,coffset=0,indenting=0,tabsize;
	gchar *buf = doc_get_chars(doc,0,-1);

	utf8_offset_cache_reset();
	tabsize = doc_get_tabsize(doc);
	/*g_print("got tabsize %d\n",tabsize);*/
	doc_unre_new_group(doc);
	while (buf[i] != '\0') {
		switch (buf[i]) {
		case '\n':
			wstart=i;
			indenting=0;
		break;
		case '\t':
			/* a tab increases to the next tab stop */
			indenting = ((indenting/tabsize)+1)*tabsize;
		break;
		case ' ':
			indenting += 1;
		break;
		default:
			if (wstart != -1 && indenting>0 && (wstart+1 != i)) {
				gchar *newindent;
				gint cstart, cend;
				if (to_tabs) {
					newindent = bf_str_repeat("\t", (indenting/tabsize));
				} else {
					newindent = bf_str_repeat(" ", indenting);
				}
				cstart = utf8_byteoffset_to_charsoffset_cached(buf, wstart+1);
				cend = utf8_byteoffset_to_charsoffset_cached(buf, i);
				doc_replace_text_backend(doc, newindent, cstart+coffset, cend+coffset);
				coffset += strlen(newindent)-(cend-cstart);
				g_free(newindent);
			}
			wstart=-1;
		break;
		}
		i++;
	}
	g_free(buf);
	doc_unre_new_group(doc);
}

static void convert_to_columns_backend(Tdocument *doc, gint so, gint eo, gint numcolumns, gboolean spread_horiz, const gchar *separator, const gchar *fillempty) {
	gint numlines,numnewlines,i=0,j=0;
	gchar *buf;
	GList *buflist;
	guint offset=0,separatorlen;
	/* get buffer */
	buf = doc_get_chars(doc,so,eo);
	utf8_offset_cache_reset();
	/* buffer to list */
	buflist = get_list_from_buffer(buf, NULL, FALSE);
	g_free(buf);
	/* get the number of lines */
	numlines = g_list_length(buflist);
	numnewlines = (0.99999999+1.0*numlines/numcolumns);
/*	g_print("float=%f, int=%d\n",0.9999+1.0*numlines/numcolumns, (int)(0.9999+1.0*numlines/numcolumns));*/
	/*g_print("numlines=%d, numcolumns=%d, numnewlines=%d\n",numlines,numcolumns,numnewlines);*/
	separatorlen=g_utf8_strlen(separator,-1);
	doc_unre_new_group(doc);
	doc_replace_text_backend(doc, NULL, so, eo);
	for (i=0;i<numnewlines;i++) {
		for(j=0;j<numcolumns;j++) {
			gchar *tmp;
			if (spread_horiz) {
				/*g_print("i=%d,j=%d,numcolumns=%d, insert string i*numcolumns+j %d\n",i,j,numcolumns,i*numcolumns+j);*/
				tmp = g_list_nth_data(buflist, i*numcolumns+j);
			} else {
				/*g_print("i=%d,j=%d,numnewlines=%d, insert string i+j*numnewlines %d\n",i,j,numnewlines,i+j*numnewlines);*/
				tmp = g_list_nth_data(buflist, i+j*numnewlines);
			}
			if (tmp) {
				doc_replace_text_backend(doc, tmp, so+offset, so+offset);
				offset += g_utf8_strlen(tmp,-1);
			} else {
				doc_replace_text_backend(doc, fillempty, so+offset, so+offset);
				offset += g_utf8_strlen(fillempty,-1);
			}
			if(j+1==numcolumns) {
				/*g_print("j=%d, numcolumns=%d, j+1==numcolumns, newline!\n",j,numcolumns);*/
				doc_replace_text_backend(doc, "\n", so+offset, so+offset);
				offset += 1;
			} else {
				doc_replace_text_backend(doc, separator, so+offset, so+offset);
				offset += separatorlen;
			}
		}
		
	}
	doc_unre_new_group(doc);
	free_stringlist(buflist);
}

typedef struct {
	Tdocument *doc;
	GtkWidget *dialog;
	GtkWidget *horizontally;
	GtkWidget *separator;
	GtkWidget *numcolumns;
	GtkWidget *fillempty;
} Tconvertcolumn;

static void convert_to_columns_lcb(GtkDialog * dialog, gint response, Tconvertcolumn *cc) {
	if (response == GTK_RESPONSE_ACCEPT) {
		gint start, end;
		const gchar *separator, *fillempty;
		
		if (!doc_get_selection(cc->doc, &start, &end)) {
			start=0;
			end=-1;
		}
		
		separator = gtk_entry_get_text(GTK_ENTRY(cc->separator));
		fillempty = gtk_entry_get_text(GTK_ENTRY(cc->fillempty));
		if (BFWIN(cc->doc->bfwin)->session->convertcolumn_separator)
			g_free(BFWIN(cc->doc->bfwin)->session->convertcolumn_separator);
		if (BFWIN(cc->doc->bfwin)->session->convertcolumn_fillempty)
			g_free(BFWIN(cc->doc->bfwin)->session->convertcolumn_fillempty);
		BFWIN(cc->doc->bfwin)->session->convertcolumn_separator = g_strdup(separator);
		BFWIN(cc->doc->bfwin)->session->convertcolumn_fillempty = g_strdup(fillempty);
		BFWIN(cc->doc->bfwin)->session->convertcolumn_horizontally = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cc->horizontally));
		
		convert_to_columns_backend(cc->doc, start, end, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(cc->numcolumns)), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cc->horizontally)), separator, fillempty);
	}
	gtk_widget_destroy(cc->dialog);
	g_free(cc);	
}

void convert_to_columns(Tdocument *doc) {
	Tconvertcolumn *cc;
	GtkWidget *hbox;
	
	cc = g_new0(Tconvertcolumn,1);
	cc->doc=doc;
	cc->dialog = gtk_dialog_new_with_buttons(_("Lines into columns"),
      GTK_WINDOW(BFWIN(doc->bfwin)->main_window),GTK_DIALOG_DESTROY_WITH_PARENT, 
      GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
      GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, 
      NULL);
	g_signal_connect(G_OBJECT(cc->dialog), "response", G_CALLBACK(convert_to_columns_lcb), cc);
	window_delete_on_escape(GTK_WINDOW(cc->dialog));
	gtk_container_set_border_width(GTK_CONTAINER(cc->dialog),10);
	gtk_box_set_spacing(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(cc->dialog))),10);

	cc->numcolumns = spinbut_with_value("2", 2, 99, 1, 5);
	hbox = gtk_hbox_new(FALSE,5);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Number of columns")), FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(hbox), cc->numcolumns, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(cc->dialog))), hbox, FALSE, FALSE, 0);
	cc->horizontally = boxed_checkbut_with_value(_("Spread lines horizontally"), BFWIN(cc->doc->bfwin)->session->convertcolumn_horizontally, gtk_dialog_get_content_area(GTK_DIALOG(cc->dialog)) );
	cc->separator = boxed_full_entry(_("Column separator"), BFWIN(cc->doc->bfwin)->session->convertcolumn_separator,99, gtk_dialog_get_content_area(GTK_DIALOG(cc->dialog)));
	
	cc->fillempty = boxed_full_entry(_("Fill empty entries"), BFWIN(cc->doc->bfwin)->session->convertcolumn_fillempty,99, gtk_dialog_get_content_area(GTK_DIALOG(cc->dialog)));
	gtk_widget_show_all(cc->dialog);
}

