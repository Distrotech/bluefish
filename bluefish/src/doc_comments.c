/* Bluefish HTML Editor
 * doc_comments.c - toggle comments
 *
 * Copyright (C) 2010-2012 Olivier Sessink
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
#include "bftextview2.h"
#include "doc_comments.h"
#include "undo_redo.h"


static gint
add_line_comment(Tdocument *doc, const gchar *commentstring, gint start, gint end) {
	gint i=0,coffset=0,commentstring_len;
	gchar *buf;

	if (start!=0)	
		start--; /* include a possible newline character if 
			the selection started at te first character of a line */

	commentstring_len = g_utf8_strlen(commentstring,-1);
	
	buf = doc_get_chars(doc,start,end);
	utf8_offset_cache_reset();
	
	doc_unre_new_group(doc);
	while (buf[i] != '\0') {
		if (i==0 || (buf[i]=='\n' && buf[i+1]!='\0')) {
			gint cstart;
			cstart = utf8_byteoffset_to_charsoffset_cached(buf, i+1);
			doc_replace_text_backend(doc, commentstring, coffset+start+cstart, coffset+start+cstart);
			coffset += commentstring_len;
		}
		i++;
	}
	g_free(buf);
	doc_unre_new_group(doc);
	return coffset;
}

static void remove_line_comment(Tdocument *doc, const gchar *buf, const gchar *commentstring, gint start, gint end) {
	gint commentstring_len,i=0,coffset;
	gboolean newline;

	commentstring_len=strlen(commentstring);
	doc_unre_new_group(doc);
	coffset=start;
	newline=TRUE;
	while (buf[i] != '\0') {
		if (buf[i]=='\n') {
			newline=TRUE;
		} else if (newline) {
			if (strncmp(&buf[i], commentstring, commentstring_len)==0) {
				gint cstart = utf8_byteoffset_to_charsoffset_cached(buf, i);
				doc_replace_text_backend(doc, NULL, coffset+cstart, coffset+cstart+commentstring_len);
				coffset -= commentstring_len;
			}
			newline=FALSE;
		}
		i++;
	}
	doc_unre_new_group(doc);
}

static void add_block_comment(Tdocument *doc, const gchar *so_commentstring, const gchar *eo_commentstring, gint start, gint end) {
	gint offset;
	
	doc_unre_new_group(doc);
	
	doc_replace_text_backend(doc, so_commentstring, start, start);
	offset = g_utf8_strlen(so_commentstring,-1);
	doc_replace_text_backend(doc, eo_commentstring, end+offset, end+offset);

	doc_unre_new_group(doc);
}

static void remove_block_comment(Tdocument *doc, const gchar *buf, const gchar *so_commentstring, const gchar *eo_commentstring, gint start, gint end) {
	gint i=0,n=0,coffset,eo_commentstring_len;
	gint so=0;
	gint cstart,cend;
	
	/* first see if there is an start-of-block */
	coffset=start;
	
	while (buf[i] != '\0') {
		if (n==0 && (buf[i]==' '||buf[i]=='\n'||buf[i]=='\t')) {
			/* do nothing */
		} else if (buf[i]==so_commentstring[n]) {
			if (n==0)
				so=i;
			n++;
			if (so_commentstring[n]=='\0') {
				/* we found the end of the start-of-comment string */
				break;
			}
		} else {
			n=0;
			break;
		}
		i++;
	}
	if (n != strlen(so_commentstring)) {
		return;
	}
	
	doc_unre_new_group(doc);
	cstart = utf8_byteoffset_to_charsoffset_cached(buf, so);
	cend = cstart+strlen(so_commentstring);
	DEBUG_MSG("remove_block_comment, remove start-of-comment %d:%d\n",coffset+cstart, coffset+cend);
	doc_replace_text_backend(doc, NULL, coffset+cstart, coffset+cend);
	coffset -= (cend-cstart); 
	
	/* now find the end-of-block */
	i=strlen(buf)-1;
	eo_commentstring_len = strlen(eo_commentstring)-1; 
	n=eo_commentstring_len;
	while (i>=0) {
		if (n==eo_commentstring_len && (buf[i]==' '||buf[i]=='\n'||buf[i]=='\t')) {
			/* do nothing */
		} else if (buf[i]==eo_commentstring[n]) {
			if (n==eo_commentstring_len) {
				/*eo=n+1;*/
				/* we found the end of the end-of-comment string */
			}
			if (n==0) {
				so = i;
				break;
			}
			n--;	
		} else {
			break;
		}
		i--;
	}
	if (n==0) {
		cstart = utf8_byteoffset_to_charsoffset_cached(buf, so);
		cend = cstart+strlen(eo_commentstring);
		DEBUG_MSG("remove_block_comment, remove end-of-comment %d:%d\n",coffset+cstart, coffset+cend);
		doc_replace_text_backend(doc, NULL, coffset+cstart, coffset+cend);
	}
	
	doc_unre_new_group(doc);
}

void toggle_comment(Tdocument *doc) {
	GtkTextIter its,ite;
	gboolean ret;
	
	ret = bluefish_text_view_in_comment(BLUEFISH_TEXT_VIEW(doc->view), &its, &ite);
	if (ret) { /* remove comment */
		gchar *buf;
		gint i=0;
		buf = gtk_text_buffer_get_text(doc->buffer, &its, &ite, FALSE);
		DEBUG_MSG("toggle_comment, remove comment\n");
		while (g_array_index(BLUEFISH_TEXT_VIEW(doc->view)->bflang->st->comments, Tcomment, i).so) {
			if (strncmp(g_array_index(BLUEFISH_TEXT_VIEW(doc->view)->bflang->st->comments, Tcomment, i).so
							, buf
							, strlen(g_array_index(BLUEFISH_TEXT_VIEW(doc->view)->bflang->st->comments, Tcomment, i).so)
						)==0) {
				if (g_array_index(BLUEFISH_TEXT_VIEW(doc->view)->bflang->st->comments, Tcomment, i).type == comment_type_block)
					remove_block_comment(doc, buf, g_array_index(BLUEFISH_TEXT_VIEW(doc->view)->bflang->st->comments, Tcomment, i).so
							, g_array_index(BLUEFISH_TEXT_VIEW(doc->view)->bflang->st->comments, Tcomment, i).eo
							, gtk_text_iter_get_offset(&its),gtk_text_iter_get_offset(&ite));
				else
					remove_line_comment(doc, buf, g_array_index(BLUEFISH_TEXT_VIEW(doc->view)->bflang->st->comments, Tcomment, i).so
							, gtk_text_iter_get_offset(&its),gtk_text_iter_get_offset(&ite));
				break;
			}
			i++;
		}
		g_free(buf);
	} else { /* add comment */
		gboolean sameline, selection=FALSE;
		gint offsets,offsete,extraoffset=0;
		Tcomment *comment;
		DEBUG_MSG("toggle_comment, add comment\n");
		if (gtk_text_buffer_get_has_selection(doc->buffer)) {
			sameline = gtk_text_iter_get_line(&its)==gtk_text_iter_get_line(&ite);
			selection=TRUE;
		 } else {
		 	gtk_text_iter_set_line_offset(&its,0);
		 	if (!gtk_text_iter_ends_line(&ite)) {
				gtk_text_iter_forward_to_line_end(&ite);
			}
		 	sameline = TRUE;
		}
		comment = bluefish_text_view_get_comment(BLUEFISH_TEXT_VIEW(doc->view), &its, sameline?comment_type_line:comment_type_block);
		DEBUG_MSG("toggle_comment, comment=%p\n",comment);
	 	if (!comment)
	 		return;
		offsets = gtk_text_iter_get_offset(&its);
		offsete = gtk_text_iter_get_offset(&ite);
		DEBUG_MSG("got comment with type %d, so=%s\n",comment->type, comment->so);
	 	if (comment->type == comment_type_line) {
	 		extraoffset = add_line_comment(doc, comment->so, offsets,offsete);
	 		/*extraoffset = strlen(comment->so);*/
	 	} else {
			add_block_comment(doc, comment->so, comment->eo, gtk_text_iter_get_offset(&its), gtk_text_iter_get_offset(&ite));
			extraoffset = strlen(comment->so) + strlen(comment->eo);
		}
		if (selection) {
			/* the buffer has changes, so we have to re-set the iters from the offsets */
			gtk_text_buffer_get_iter_at_offset(doc->buffer, &its, offsets);
			gtk_text_buffer_get_iter_at_offset(doc->buffer, &ite, offsete+extraoffset);
			DEBUG_MSG("toggle_comment, reset selection to %d:%d\n", gtk_text_iter_get_offset(&its), gtk_text_iter_get_offset(&ite));
			gtk_text_buffer_select_range(doc->buffer,&its,&ite);
		}
	}
}
