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

#include "async_queue.h"

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
	gpointer doc;
	gint32 so;
	gint32 eo;
} Tsnr3result;

#define S3RESULT(var)  ((Tsnr3result *)var)

typedef struct {
	Tbfwin *bfwin;
	gpointer dialog; /* NULL for simple search or external search */
	gchar *query;
	gchar *queryreal; /* with characters escaped and such */
	GRegex *regex;
	gchar *replace; /* enabled if not NULL */
	gchar *replacereal; /* with characters escaped and such */
	gboolean replaceall; /* set to TRUE bluefish will immediately (while searching) do the replace */
	gboolean findall; /* only used when find all in files is used */
	Tsnr3type type;
	Tsnr3replace replacetype;
	Tsnr3scope scope;
	GFile *basedir; /* when replace in files */
	gchar *filepattern;
	gint recursion_level;
	gboolean is_case_sens;
/*	gboolean overlapping;*/
	gboolean escape_chars;
	gboolean dotmatchall;
	gboolean select_matches;
	gboolean bookmark_matches;
	gboolean showinoutputbox;

	/* the resultss of a search run */
	gboolean in_replace; /* TRUE if the code is in a replace, so the doc_insert and doc_delete signals do not need to do anything */
	GQueue results; /* all results */
	GList *current; /* current result, used in replace, or when pressing next or previous */
	guint resultnumdoc; /* the number of unique documents in the resultset */
	guint searchednumdoc; /* the number of documents searched */

	guint unre_action_id;
	
	/* following entries are used during the search run */
	Tdocument *curdoc; /* the current document */
	gchar *curbuf; /* the current buffer */
	gint curoffset; /* when running replace all, the difference between the offset in curbuf and the offset in the text widget */
	guint curposition; /* the position in curbuf to continue the next search run, used if the first 
							search run took longer than our maximum-allowed-gui-block-time */
	guint so; /* area to search in */
	guint eo; /* see so */
	void (*callback) (gpointer data);	/* to be called when the search has finished */
	guint idle_id;
	guint changed_idle_id;	
	Tasyncqueue idlequeue;
	Tasyncqueue threadqueue;
	volatile gint runcount;
	volatile gint cancelled;
	gpointer findfiles; /* a pointer for the return value of findfiles() so we can cancel it */
} Tsnr3run;

#define S3RUN(var)  ((Tsnr3run *)var)

typedef struct {
	Tasyncqueue queue;
	guint refcount;
	volatile gint cancelled;
	Tsnr3run *s3run;
	void (*callback) (void *);
	gpointer findfiles;
} Tfilesworker;

typedef struct {
	gsize pos;
	guint line;
} Tlineinbuffer;



enum {
	SNR_RESPONSE_FIND = 0,
	SNR_RESPONSE_BACK,
	SNR_RESPONSE_REPLACE,
	SNR_RESPONSE_REPLACE_ALL,
	SNR_RESPONSE_BOOKMARK_ALL,
	SNR_RESPONSE_FIND_ALL,
};

typedef struct {
	GtkWidget *dialog;
	GtkWidget *expander;
	GtkWidget *search;
	GtkWidget *searchfeedback;
	GtkWidget *replace;
	GtkWidget *scope;
	GtkWidget *basedir;
	GtkWidget *basedirL;
	GtkWidget *basedirB;
	GtkWidget *filepattern;
	GtkWidget *filepatternL;
	GtkWidget *recursion_level;
	GtkWidget *recursion_levelL;
	GtkWidget *countlabel;
	GtkWidget *warninglabel;
	GtkWidget *searchType;
	GtkWidget *replaceType;
	GtkWidget *replaceTypeL;
	GtkWidget *overlappingMatches;
	GtkWidget *matchCase;
	GtkWidget *escapeChars;
	GtkWidget *dotmatchall;
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

void snr3_run(Tsnr3run *s3run, TSNRWin *snrwin, Tdocument *doc, void (*callback)(void *)); /* called from snr3_files.c */
void snr3_run_in_doc(Tsnr3run *s3run, Tdocument *doc, gint so, gint eo, gboolean update);
void snr3_run_go(Tsnr3run *s3run, gboolean forward);
void snr3run_free(Tsnr3run *s3run);
void snr3run_unrun(Tsnr3run *s3run);
gpointer simple_search_run(Tbfwin *bfwin, const gchar *string, Tsnr3type type
		, gboolean casesens, gboolean dotmatchall, gboolean unescape);
void simple_search_next(Tbfwin *bfwin);
void snr3_advanced_dialog(Tbfwin * bfwin, const gchar *searchstring);

void snr3_run_extern_replace(Tdocument * doc, const gchar * search_pattern, Tsnr3scope scope,
							 Tsnr3type type, gboolean is_case_sens, const gchar * replace_pattern,
							 gboolean unescape, gboolean dotmatchall);

#endif /* #define __SNR3_H_ */
