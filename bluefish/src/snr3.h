/* Bluefish HTML Editor
 * snr3.h - search and replace
 *
 * Copyright (C) 2011 Olivier Sessink
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
#ifndef __SNR3_H_
#define __SNR3_H_

typedef enum {
	snr3type_string,
	snr3type_pcre
} Tsnr3type;

typedef enum {
	snr3scope_doc,
	snr3scope_cursor,
	snr3scope_selection,
	snr3scope_alldocs,
	snr3scope_files
} Tsnr3scope;

typedef enum {
	snr3replace_string,
	snr3replace_upper,
	snr3replace_lower
} Tsnr3replace;

typedef struct {
	gint so;
	gint eo;
} Tsnr3submatches;

typedef struct {
	gint so;
	gint eo;
	gpointer doc;
} Tsnr3result;

typedef enum {
	alldocs,
	singledocupdate
} Tsnr3workmode;

typedef struct {
	gpointer querydata; /* for example the compiled pcre pattern */
	Tdocument *curdoc;
	gchar *curbuf;
	guint so;
	guint eo;
	guint curoffset;	
	Tsnr3workmode workmode;
	void (*callback) (void *);
} Tsnr3working;

typedef struct {
	Tbfwin *bfwin;
	gpointer dialog;
	gchar *query;
	GRegex *regex;
	gchar *replace; /* enabled if not NULL */
	gboolean replaceall; /* set to TRUE bluefish will immediately (while searching) do the replace */
	Tsnr3type type;
	Tsnr3replace replacetype;
	Tsnr3scope scope;
	GList *doclist;
	GFile *basedir; /* when replace in files */
	gchar *filepattern;
	gboolean recursive;
	gboolean is_case_sens;
	gboolean overlapping;
	gboolean escape_chars;
	gboolean select_matches;
	gboolean bookmark_matches;
	guint idle_id;
	/* following entries are used during the search run */
	Tdocument *curdoc;
	gchar *curbuf;
	gint curoffset; /* when running replace all, the difference between the offset in curbuf and the offset in the text widget */
	guint curposition; /* the position in curbuf to continue the next search run */
	guint so;
	guint eo;
	Tsnr3workmode workmode; /* not yet used */
	void (*callback) (void *);	/* to be called when the search has finished */
	GQueue results;
	GList *current;
} Tsnr3run;

enum {
	SNR_RESPONSE_FIND = 0,
	SNR_RESPONSE_BACK,
	SNR_RESPONSE_REPLACE,
	SNR_RESPONSE_REPLACE_ALL,
	SNR_RESPONSE_BOOKMARK_ALL
};

typedef struct {
	GtkWidget *dialog;
	GtkWidget *expander;
	GtkWidget *search;
	GtkWidget *searchfeedback;
	GtkWidget *replace;
	GtkWidget *scope;
	GtkWidget *basedir;
	GtkWidget *fileshbox;
	GtkWidget *filepattern;
	GtkWidget *countlabel;
	GtkWidget *warninglabel;
	GtkWidget *searchType;
	GtkWidget *replaceType;
	GtkWidget *overlappingMatches;
	GtkWidget *matchCase;
	GtkWidget *escapeChars;
	GtkWidget *select_match;
	GtkWidget *bookmarks;
	GtkWidget *findButton;
	GtkWidget *backButton;	
	GtkWidget *findAllButton;
	GtkWidget *replaceButton;
	GtkWidget *replaceAllButton;
	GtkWidget *bookmarkButton;
	Tbfwin *bfwin;
	Tsnr3run *s3run;
} TSNRWin;


typedef struct {
	gpointer nothing;
} Tsnr3config;

#define MAX_CONTINUOUS_SEARCH_INTERVAL 0.1

void
snr3_run(Tsnr3run *s3run, Tdocument *doc, void (*callback)(void *)); /* called from snr3_files.c */

void snr3_run_go(Tsnr3run *s3run, gboolean forward);
void snr3run_free(Tsnr3run *s3run);
gpointer simple_search_run(Tbfwin *bfwin, const gchar *string);
void snr3_advanced_dialog(Tbfwin * bfwin);
#endif /* #define __SNR3_H_ */
