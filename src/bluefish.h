/* Bluefish HTML Editor
 * bluefish.h - global prototypes
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
#ifndef __BLUEFISH_H_
#define __BLUEFISH_H_

/*********************/
/* undo/redo structs */
/*********************/
typedef enum {
	UndoDelete = 1, UndoInsert
} undo_op_t;

typedef struct {
	GList *entries;	/* the list of entries that should be undone in one action */
	gint changed;		/* doc changed status at this undo node */
} unregroup_t;

typedef struct {
	GList *first;
	GList *last;
	unregroup_t *current;
	gint num_groups;
	GList *redofirst;
} unre_t;

/************************/
/* highlighting structs */
/************************/

typedef struct {
	gchar *spat1;
	gchar *spat2;
	gchar *spat3;
	gchar *name;
	gchar *parentname;
	regex_t rpat1;
	regex_t rpat2;
	regex_t rpat3;
	gint mode;
	gint style;
	GList *childs;
	gboolean need_subpatterns;
	GtkTextTag *tag;
} Tpattern;

typedef struct {
	gchar *type;
	gchar **extensions;
	gchar *update_chars;
	GList *highlightlist;
	GtkTextTagTable *tagtable;
} Thighlightset;

/*******************/
/* document struct */
/*******************/

typedef struct {
	gchar *filename;
	gint modified;
	time_t mtime; /* from stat() */
	uid_t owner_uid; /* user ID of owner */
	gid_t owner_gid; /* group ID of owner */
	mode_t mode; /* mode field in stat() format so any stat macro's will work*/
	gint is_symlink; /* file is a symbolic link */
	unsigned del_txt_id;
	unsigned ins_txt_id;
	unre_t unre;
	GtkWidget *view;
	GtkWidget *tab_label;
	GtkTextBuffer *buffer;
	gint last_rbutton_event; /* index of last 3rd button click */
	Thighlightset *hl; /* highlighting set to use for this document */
} Tdocument;

typedef struct {
	Tdocument *current_document;
	GList *documentlist;
	GList *hlsetlist;
	GtkWidget *main_window;
	GtkWidget *notebook;
} Tmain;

extern Tmain *main_v;

#endif /* __BLUEFISH_H_ */
