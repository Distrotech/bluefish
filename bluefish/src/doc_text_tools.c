/* Bluefish HTML Editor
 * doc_text_tools.c - text tools
 *
 * Copyright (C) 2008-2012 Olivier Sessink
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
/*#define DEBUG*/

#include "bluefish.h"
#include "bf_lib.h"
#include "gtk_easy.h"
#include "dialog_utils.h"
#include "document.h"
#include "undo_redo.h"
#include "stringlist.h"

/* special replace: strip trailing spaces */
/* use a very simple loop, one that knows whitespace, non-whitespace and a newline */
void
strip_trailing_spaces(Tdocument * doc)
{
	gint i = 0, wstart = 0, coffset = 0;
	gint start, end;
	gchar *buf;

	if (!doc_get_selection(doc, &start, &end)) {
		start = 0;
		end = -1;
	}
	buf = doc_get_chars(doc, start, end);
	coffset = start;

	doc_unre_new_group(doc);
	while (buf[i] != '\0') {
		switch (buf[i]) {
		case ' ':
		case '\t':
			/* do nothing */
			break;
		case '\n':
			if (wstart + 1 < i) {
				gint cstart, cend;
				cstart = utf8_byteoffset_to_charsoffset_cached(buf, wstart + 1);
				cend = utf8_byteoffset_to_charsoffset_cached(buf, i);
				doc_replace_text_backend(doc, "", cstart + coffset, cend + coffset);
				coffset -= (cend - cstart);
			}
			/* no break, fall trough */
		default:
			wstart = i;
			break;
		}
		i++;
	}
	g_free(buf);
	doc_unre_new_group(doc);
}

static gint
join_lines_backend(Tdocument * doc, gint start, gint end)
{
	gint i = 0, cstart, cend, coffset;
	gchar *buf;
	gboolean in_split = FALSE;
	gint so_line_split = 0, eo_line_split = 0;

	coffset = start; /* the offset in 'buf' compared to the gtktextbuffer */
	buf = doc_get_chars(doc, start, end);
	utf8_offset_cache_reset();
	DEBUG_MSG("join_lines_backend, from %d:%d\n",start,end);
	while (buf[i] != '\0') {
		if (in_split) {
			if (buf[i] == '\n' || buf[i] == '\r') {
				in_split = FALSE;
				DEBUG_MSG("join_lines, don't join empty line at byte %d\n", i);
			} else if (buf[i] != '\t' && buf[i] != ' ') {
				eo_line_split = i;
				in_split = FALSE;
				cstart = utf8_byteoffset_to_charsoffset_cached(buf, so_line_split);
				cend = utf8_byteoffset_to_charsoffset_cached(buf, eo_line_split);
				DEBUG_MSG("join_lines, replace from %d to %d, coffset=%d\n", cstart + coffset, cend + coffset,
						  coffset);
				doc_replace_text_backend(doc, " ", cstart + coffset, cend + coffset);
				coffset += (1 - (cend - cstart));
			}
		} else {
			if (buf[i] == '\n') {
				so_line_split = i;
				in_split = TRUE;
			} else if (buf[i] == '\r') {
				so_line_split = i;
				in_split = TRUE;
				if (buf[i + 1] == '\n')
					i++;
			}
		}
		i++;
	}
	g_free(buf);
	DEBUG_MSG("join_lines_backend, return offset %d\n",coffset-start);
	return coffset - start;
}

void
join_lines(Tdocument * doc)
{
	gint start, end;
	if (!doc_get_selection(doc, &start, &end)) {
		start = 0;
		end = -1;
	}
	doc_unre_new_group(doc);
	join_lines_backend(doc, start, end);
	doc_unre_new_group(doc);
}

static void
split_lines_backend(Tdocument * doc, gint start, gint end)
{
	gint coffset = 0; /* the offset that we have introduced since the start of this function call */
	gint count = 0, tabsize;
	gint startws = 0, endws = 0, starti = start, endi = -1, requested_size;	
	/* ws= whitespace, i=indenting, these are character positions in the GtkTextBufferr !!!!!!!! */
	gint charpos;
	gchar *buf, *p;
	gunichar c;

	tabsize = doc_get_tabsize(doc);
	p = buf = doc_get_chars(doc, start, end);
	utf8_offset_cache_reset();
	requested_size = main_v->props.right_margin_pos;
	coffset = 0;
	charpos = start;
	DEBUG_MSG("split_lines_backend, from %d:%d on right margin %d\n",start,end, requested_size);
	c = g_utf8_get_char(p);
	while (c != '\0') {
		if (count > requested_size) {
			gchar *new_indenting, *tmp1, *tmp2;
			if (startws >= endws)
				endws = charpos;
			DEBUG_MSG("split_lines, count=%d, startws=%d, endws=%d, coffset=%d c='%c'\n", count, startws,
					  endws, coffset, c);
			if (starti == endi || endi==-1) {
				new_indenting = g_strdup("\n");
			} else {
				tmp1 = buf+utf8_byteoffset_to_charsoffset_cached(buf, starti-start);
				tmp2 = buf+utf8_byteoffset_to_charsoffset_cached(buf, endi-start);
				/*tmp1 = g_utf8_offset_to_pointer(buf, starti-start);
				tmp2 = g_utf8_offset_to_pointer(buf, endi-start);*/
				new_indenting = g_strndup(tmp1, (tmp2 - tmp1));
				DEBUG_MSG("split_lines_backend, starti=%d,endi=%d, len=%d, bytes=%d, new_indenting='%s'\n", starti, endi, endi-starti, (gint) (tmp2 - tmp1),
						  new_indenting);
			}
			DEBUG_MSG("split_lines_backend, replace from startws=%d to endws=%d with offset %d with new indenting\n", startws, endws, coffset);
			count = charpos - endws;
			doc_replace_text_backend(doc, new_indenting, startws + coffset, endws + coffset);
			coffset += (g_utf8_strlen(new_indenting, -1) - (endws - startws));
			DEBUG_MSG("split_lines_backend, new coffset=%d, new count=%d\n", coffset, count);
			startws = charpos;
			endws = charpos;
			g_free(new_indenting);
		}
		if (c == '\t') {
			count += tabsize;
			if (startws < endws) {
				startws = charpos;
				endws = charpos;
				DEBUG_MSG("split_lines_backend, tab, set startws to %d\n", startws);
			}
		} else if (c == ' ') {
			count++;
			if (startws < endws) {
				startws = charpos;
				endws = charpos;
				DEBUG_MSG("split_lines_backend, space, set startws to %d\n", startws);
			}
		} else if (c == '\n') {
			count = 0;
			starti = charpos;
			endws = startws = charpos + 1;
			DEBUG_MSG("split_lines_backend, newline, set starti=%d, endws=startws=%d\n", starti, endws);
		} else {
			count++;
			if (starti > endi) {
				endi = charpos;
				DEBUG_MSG("split_lines_backend, non-whitespace, set endi to %d, starti=%d\n", endi, starti);
			} else if (startws >= endws) {
				endws = charpos;
				DEBUG_MSG("split_lines_backend, non-whitespace, set endws to %d\n", endws);
			}
		}
		p = g_utf8_next_char(p);
		charpos++;
		c = g_utf8_get_char(p);
	}
	g_free(buf);
}

void
split_lines(Tdocument * doc)
{
	gint start, end;
	if (!doc_get_selection(doc, &start, &end)) {
		start = 0;
		end = -1;
	}
	doc_unre_new_group(doc);
	split_lines_backend(doc, start, end);
	doc_unre_new_group(doc);
}


void
rewrap_lines(Tdocument * doc)
{
	gint start, end, offset;
	if (!doc_get_selection(doc, &start, &end)) {
		start = 0;
		end = -1;
	}
	doc_unre_new_group(doc);
	offset = join_lines_backend(doc, start, end);
	DEBUG_MSG("rewrap_lines, offset=%d\n",offset);
	split_lines_backend(doc, start, end == -1 ? end : end+offset);
	doc_unre_new_group(doc);
}

/* from spaces to tabs or from tabs to spaces */
void
convert_identing(Tdocument * doc, gboolean to_tabs)
{
	gint i = 0, wstart = 0, coffset = 0, indenting = 0, tabsize;
	gchar *buf = doc_get_chars(doc, 0, -1);

	utf8_offset_cache_reset();
	tabsize = doc_get_tabsize(doc);
	/*g_print("got tabsize %d\n",tabsize); */
	doc_unre_new_group(doc);
	while (buf[i] != '\0') {
		switch (buf[i]) {
		case '\n':
			wstart = i;
			indenting = 0;
			break;
		case '\t':
			/* a tab increases to the next tab stop */
			indenting = ((indenting / tabsize) + 1) * tabsize;
			break;
		case ' ':
			indenting += 1;
			break;
		default:
			if (wstart != -1 && indenting > 0 && (wstart + 1 != i)) {
				gchar *newindent;
				gint cstart, cend;
				if (to_tabs) {
					newindent = bf_str_repeat("\t", (indenting / tabsize));
				} else {
					newindent = bf_str_repeat(" ", indenting);
				}
				cstart = utf8_byteoffset_to_charsoffset_cached(buf, wstart + 1);
				cend = utf8_byteoffset_to_charsoffset_cached(buf, i);
				doc_replace_text_backend(doc, newindent, cstart + coffset, cend + coffset);
				coffset += strlen(newindent) - (cend - cstart);
				g_free(newindent);
			}
			wstart = -1;
			break;
		}
		i++;
	}
	g_free(buf);
	doc_unre_new_group(doc);
}

static void
convert_to_columns_backend(Tdocument * doc, gint so, gint eo, gint numcolumns, gboolean spread_horiz,
						   const gchar * separator, const gchar * fillempty)
{
	gint numlines, numnewlines, i = 0, j = 0;
	gchar *buf;
	GList *buflist;
	guint offset = 0, separatorlen;
	/* get buffer */
	buf = doc_get_chars(doc, so, eo);
	utf8_offset_cache_reset();
	/* buffer to list */
	buflist = get_list_from_buffer(buf, NULL, FALSE);
	g_free(buf);
	/* get the number of lines */
	numlines = g_list_length(buflist);
	numnewlines = (0.99999999 + 1.0 * numlines / numcolumns);
/*	g_print("float=%f, int=%d\n",0.9999+1.0*numlines/numcolumns, (int)(0.9999+1.0*numlines/numcolumns));*/
	/*g_print("numlines=%d, numcolumns=%d, numnewlines=%d\n",numlines,numcolumns,numnewlines); */
	separatorlen = g_utf8_strlen(separator, -1);
	doc_unre_new_group(doc);
	doc_replace_text_backend(doc, NULL, so, eo);
	for (i = 0; i < numnewlines; i++) {
		for (j = 0; j < numcolumns; j++) {
			gchar *tmp;
			if (spread_horiz) {
				/*g_print("i=%d,j=%d,numcolumns=%d, insert string i*numcolumns+j %d\n",i,j,numcolumns,i*numcolumns+j); */
				tmp = g_list_nth_data(buflist, i * numcolumns + j);
			} else {
				/*g_print("i=%d,j=%d,numnewlines=%d, insert string i+j*numnewlines %d\n",i,j,numnewlines,i+j*numnewlines); */
				tmp = g_list_nth_data(buflist, i + j * numnewlines);
			}
			if (tmp) {
				doc_replace_text_backend(doc, tmp, so + offset, so + offset);
				offset += g_utf8_strlen(tmp, -1);
			} else {
				doc_replace_text_backend(doc, fillempty, so + offset, so + offset);
				offset += g_utf8_strlen(fillempty, -1);
			}
			if (j + 1 == numcolumns) {
				/*g_print("j=%d, numcolumns=%d, j+1==numcolumns, newline!\n",j,numcolumns); */
				doc_replace_text_backend(doc, "\n", so + offset, so + offset);
				offset += 1;
			} else {
				doc_replace_text_backend(doc, separator, so + offset, so + offset);
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

static void
convert_to_columns_lcb(GtkDialog * dialog, gint response, Tconvertcolumn * cc)
{
	if (response == GTK_RESPONSE_ACCEPT) {
		gint start, end;
		const gchar *separator, *fillempty;

		if (!doc_get_selection(cc->doc, &start, &end)) {
			start = 0;
			end = -1;
		}

		separator = gtk_entry_get_text(GTK_ENTRY(cc->separator));
		fillempty = gtk_entry_get_text(GTK_ENTRY(cc->fillempty));
		if (BFWIN(cc->doc->bfwin)->session->convertcolumn_separator)
			g_free(BFWIN(cc->doc->bfwin)->session->convertcolumn_separator);
		if (BFWIN(cc->doc->bfwin)->session->convertcolumn_fillempty)
			g_free(BFWIN(cc->doc->bfwin)->session->convertcolumn_fillempty);
		BFWIN(cc->doc->bfwin)->session->convertcolumn_separator = g_strdup(separator);
		BFWIN(cc->doc->bfwin)->session->convertcolumn_fillempty = g_strdup(fillempty);
		BFWIN(cc->doc->bfwin)->session->convertcolumn_horizontally =
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cc->horizontally));

		convert_to_columns_backend(cc->doc, start, end,
								   gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(cc->numcolumns)),
								   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cc->horizontally)),
								   separator, fillempty);
	}
	gtk_widget_destroy(cc->dialog);
	g_free(cc);
}

void
convert_to_columns(Tdocument * doc)
{
	Tconvertcolumn *cc;
	GtkWidget *content_area, *hbox, *table;

	cc = g_new0(Tconvertcolumn, 1);
	cc->doc = doc;
	cc->dialog = gtk_dialog_new_with_buttons(_("Lines into columns"),
											 GTK_WINDOW(BFWIN(doc->bfwin)->main_window),
											 GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL,
											 GTK_RESPONSE_REJECT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	g_signal_connect(G_OBJECT(cc->dialog), "response", G_CALLBACK(convert_to_columns_lcb), cc);
	window_delete_on_escape(GTK_WINDOW(cc->dialog));
	gtk_container_set_border_width(GTK_CONTAINER(cc->dialog), 10);
	content_area = gtk_dialog_get_content_area(GTK_DIALOG(cc->dialog));
	gtk_box_set_spacing(GTK_BOX(content_area), 10);

	cc->numcolumns = spinbut_with_value("2", 2, 99, 1, 5);
	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Number of columns:")), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), cc->numcolumns, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content_area), hbox, FALSE, FALSE, 0);
	cc->horizontally =
		boxed_checkbut_with_value(_("Spread lines horizontally"),
								  BFWIN(cc->doc->bfwin)->session->convertcolumn_horizontally, content_area);

	table = dialog_table_in_vbox_defaults(2, 2, 0, content_area);

	cc->separator =
		dialog_entry_in_table(BFWIN(cc->doc->bfwin)->session->convertcolumn_separator, table, 1, 2, 0, 1);
	dialog_mnemonic_label_in_table(_("Column separator:"), cc->separator, table, 0, 1, 0, 1);

	cc->fillempty =
		dialog_entry_in_table(BFWIN(cc->doc->bfwin)->session->convertcolumn_fillempty, table, 1, 2, 1, 2);
	dialog_mnemonic_label_in_table(_("Fill empty entries:"), cc->separator, table, 0, 1, 1, 2);

	gtk_widget_show_all(cc->dialog);
}

void
select_between_matching_block_boundaries(Tdocument *doc)
{
	GtkTextIter so,eo;
	static gboolean innerblock=FALSE;
	GtkTextIter cursor;
	guint offset;
	/* if we do not have a selection we start with innerblock is true, in all other
	cases we just revert innerblock */
	if (!gtk_text_buffer_get_has_selection(doc->buffer)) {
		innerblock = TRUE;
	} else {
		innerblock = !innerblock;
	}
	
	gtk_text_buffer_get_iter_at_mark(doc->buffer, &cursor, gtk_text_buffer_get_insert(doc->buffer));
	DEBUG_MSG("select_between_matching_block_boundaries, innerblock=%d, location=%d\n", innerblock, gtk_text_iter_get_offset(&cursor));
	offset = gtk_text_iter_get_offset(&cursor);
	if (!bluefish_text_view_get_active_block_boundaries(BLUEFISH_TEXT_VIEW(doc->view), 
					offset, innerblock, &so, &eo)) {
		DEBUG_MSG("select_between_matching_block_boundaries, no block, return\n");
		return;
	}
	if (innerblock && gtk_text_iter_equal(&so, &eo)) {
		DEBUG_MSG("select_between_matching_block_boundaries, iters are equal, request innerblock=FALSE\n");
		innerblock = FALSE;
		if (!bluefish_text_view_get_active_block_boundaries(BLUEFISH_TEXT_VIEW(doc->view), 
						offset, innerblock, &so, &eo)) {
			DEBUG_MSG("select_between_matching_block_boundaries, innerblock=FALSE, no block, return\n");
			return;
		}	
	}
	gtk_text_buffer_select_range(doc->buffer, &so, &eo);
}

void
duplicate_line(Tdocument *doc)
{
	GtkTextIter it1, it2;
	gchar *text;
	gtk_text_buffer_get_iter_at_mark(doc->buffer, &it1, gtk_text_buffer_get_insert(doc->buffer));
	gtk_text_iter_set_line_offset(&it1,0);
	it2 = it1;
	gtk_text_iter_forward_line(&it2);
	text = gtk_text_buffer_get_text(doc->buffer,&it1,&it2,TRUE);
	doc_unre_new_group(doc);
	gtk_text_buffer_insert(doc->buffer,&it2,text,-1);
	g_free(text);
	doc_unre_new_group(doc);
}

void
delete_line(Tdocument *doc)
{
	GtkTextIter it1, it2;
	gtk_text_buffer_get_iter_at_mark(doc->buffer, &it1, gtk_text_buffer_get_insert(doc->buffer));
	gtk_text_iter_set_line_offset(&it1,0);
	it2 = it1;
	gtk_text_iter_forward_line(&it2);
	doc_unre_new_group(doc);
	gtk_text_buffer_delete(doc->buffer, &it1,&it2);
	doc_unre_new_group(doc);
}

void
doc_move_selection(Tdocument *doc, gboolean up)
{
	GtkTextIter so, eo;
	gchar *text;
	gint offset, size;
	if (!gtk_text_buffer_get_selection_bounds(doc->buffer, &so, &eo)) {
		return;
	}
	/* so and eo are guaranteed to be in ascending order */
	doc_unre_new_group(doc);
	doc_block_undo_reg(doc);
	
	offset = gtk_text_iter_get_offset(&so);
	size = gtk_text_iter_get_offset(&eo)-offset;
	text = gtk_text_buffer_get_text(doc->buffer,&so,&eo,TRUE);
	/*g_print("doc_move_selection, got selection %d:%d\n",offset,offset+size);*/
	gtk_text_buffer_delete(doc->buffer, &so, &eo);
	doc_unre_add(doc, text, offset, offset+size, UndoDelete);

	/* now we have to move the cursor up */
	gtk_text_buffer_get_iter_at_offset(doc->buffer, &so, offset);
	if (up) {
		gtk_text_iter_backward_line(&so);
	} else {
		gtk_text_iter_forward_line(&so);
	}
	offset = gtk_text_iter_get_offset(&so);
	gtk_text_buffer_insert(doc->buffer,&so,text,-1);
	doc_unre_add(doc, text, offset, offset+size, UndoInsert);
	g_free(text);

	doc_unblock_undo_reg(doc);
	doc_set_modified(doc, 1);
	doc_unre_new_group(doc);
	
	/* and select the text again */
	/*g_print("doc_move_selection, select %d:%d\n",offset,offset+size);*/
	gtk_text_buffer_get_iter_at_offset(doc->buffer, &so, offset);
	gtk_text_buffer_get_iter_at_offset(doc->buffer, &eo, offset+size);
	gtk_text_buffer_select_range(doc->buffer,&so,&eo);
}

void
doc_insert_filename(Tdocument *doc, gboolean relative)
{
	gchar *tmp, *relativeto=NULL;
	if (relative && doc->uri) {
		relativeto = g_file_get_uri(doc->uri);
	}
	tmp = run_file_select_dialog(GTK_WINDOW(BFWIN(doc->bfwin)->main_window)
							, NULL, relativeto,GTK_FILE_CHOOSER_ACTION_OPEN);
	if (tmp)
		doc_insert_two_strings(doc, tmp, NULL);	
	g_free(relativeto);
	g_free(tmp);
}
