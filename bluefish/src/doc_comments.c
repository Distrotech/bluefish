/* Bluefish HTML Editor
 * doc_comments.c - toggle comments
 *
 * Copyright (C) 2010 Olivier Sessink
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
#include "bftextview2.h"
#include "document.h"
#include "undo_redo.h"
#include "bf_lib.h"

static void add_line_comment(Tdocument *doc, const gchar *commentstring, gint start, gint end) {
	gint i=0,coffset,commentstring_len;
	gchar *buf;

	if (start!=0)	
		start--; /* include a possible newline character if 
			the selection started at te first character of a line */

	commentstring_len = g_utf8_strlen(commentstring,-1);
	
	buf = doc_get_chars(doc,start,end);
	utf8_offset_cache_reset();
	
	coffset=start;
	doc_unre_new_group(doc);
	while (buf[i] != '\0') {
		if (i==0 || (buf[i]=='\n' && buf[i+1]!='\0')) {
			gint cstart;
			cstart = utf8_byteoffset_to_charsoffset_cached(buf, i+1);
			doc_replace_text_backend(doc, commentstring, coffset+cstart, coffset+cstart);
			coffset += commentstring_len;
		}
		i++;
	}
	g_free(buf);
	doc_unre_new_group(doc);
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
	gint so=0,eo;
	
	doc_unre_new_group(doc);
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
				eo = i;
				break;
			}
		} else {
			break;
		}
		i++;
	}
	if (n == strlen(so_commentstring)) {
		gint cstart,cend;
		cstart = utf8_byteoffset_to_charsoffset_cached(buf, so);
		cend = so+strlen(so_commentstring);
		doc_replace_text_backend(doc, NULL, coffset+cstart, coffset+cend);
		coffset -= (cend-cstart); 
	}
	
	/* now find the end-of-block */
	i=strlen(buf)-1;
	eo_commentstring_len = strlen(eo_commentstring)-1; 
	n=eo_commentstring_len;
	while (i>=0) {
		if (n==eo_commentstring_len && (buf[i]==' '||buf[i]=='\n'||buf[i]=='\t')) {
			/* do nothing */
		} else if (buf[i]==eo_commentstring[n]) {
			if (n==eo_commentstring_len) {
				eo=n+1;
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
		gint cstart,cend;
		cstart = utf8_byteoffset_to_charsoffset_cached(buf, so);
		cend = so+strlen(eo_commentstring);
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
		gboolean sameline;
		Tcomment *comment;
		DEBUG_MSG("toggle_comment, add comment\n");
		if (gtk_text_buffer_get_has_selection(doc->buffer)) {
			sameline = gtk_text_iter_get_line(&its)==gtk_text_iter_get_line(&ite);
			comment = bluefish_text_view_get_comment(BLUEFISH_TEXT_VIEW(doc->view), &its, sameline?comment_type_line:comment_type_block);
		 } else {
		 	gtk_text_iter_set_line_offset(&its,0);
		 	gtk_text_iter_forward_to_line_end(&ite);
		 	sameline = gtk_text_iter_get_line(&its)==gtk_text_iter_get_line(&ite);
		 	comment = bluefish_text_view_get_comment(BLUEFISH_TEXT_VIEW(doc->view), &its, sameline?comment_type_line:comment_type_block);
		}
		DEBUG_MSG("toggle_comment, comment=%p\n",comment);
	 	if (!comment)
	 		return;
	 	
	 	if (comment->type == comment_type_line) {
	 		add_line_comment(doc, comment->so, gtk_text_iter_get_offset(&its),gtk_text_iter_get_offset(&ite));
	 	} else {
			add_block_comment(doc, comment->so, comment->eo, gtk_text_iter_get_offset(&its), gtk_text_iter_get_offset(&ite));
		}
	}
}
