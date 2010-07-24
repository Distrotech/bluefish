/* Bluefish HTML Editor
 * snr2.h - the search 'n replace rewrite prototypes
 *
 * Copyright (C) 2000-2009 Olivier Sessink
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

#ifndef __SNR2_H_
#define __SNR2_H_

typedef struct {
	/* span of characters */
	gint start;
	gint end;

	/* span of byte-data in buffer */
	gint bstart;
	gint bend;
	
	/* these data are only used (and alloc'ed) if want_submatches is set in the search backend,
	they should be freed by the calling function! */
	regmatch_t *pmatch;
	gint nmatch;
	Tdocument *doc; /* used when searching all documents */
	gint errorcode; /* 0 means no error */
} Tsearch_result;

/*typedef struct {
	gint start;
	gint end;
	Tdocument *doc;
} Tsearch_all_result;*/

typedef enum { match_normal, match_word, match_posix, match_perl } Tmatch_types;

void snr2_init(Tbfwin *bfwin);
void snr2_cleanup(Tbfwin *bfwin);

gint snr2_run_extern_replace(Tdocument *doc, const gchar *search_pattern, gint region,
							Tmatch_types matchtype, gint is_case_sens, const gchar *replace_pattern,
							gboolean unescape);
Tsearch_result doc_search_run_extern(Tdocument *doc, gchar *search_pattern, gint matchtype, gint is_case_sens, gint offset);
void doc_show_result(Tdocument *document, GtkWindow *window, gint start, gint end, gboolean select_match);
void search_from_selection(Tbfwin *bfwin);
void search_cb(GtkWidget *widget, Tbfwin *bfwin);
void replace_again_cb(GtkWidget *widget, Tbfwin *bfwin);
void search_again_cb(GtkWidget *widget, Tbfwin *bfwin);
void replace_cb(GtkWidget *widget, Tbfwin *bfwin);
void update_filenames_in_file(Tdocument *doc, gchar *oldfilename, gchar *newfilename, gint doc_has_newfilename);
void update_encoding_meta_in_file(Tdocument *doc, gchar *encoding);

enum {
	BF_FIND_DIALOG = 0,
	BF_REPLACE_DIALOG
};
void snr_dialog_new(Tbfwin * bfwin, gint dialogType);

void strip_trailing_spaces(Tdocument *doc);
void join_lines(Tdocument *doc);
void split_lines(Tdocument *doc);
void convert_identing(Tdocument *doc, gboolean to_tabs);
void toggle_comment(Tdocument *doc);
void convert_to_columns(Tdocument *doc);
#endif							/* __SNR_H_ */