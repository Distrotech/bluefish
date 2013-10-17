/* Bluefish HTML Editor
 * preferences.h - the preferences code
 *
 * Copyright (C) 2002-2011 Olivier Sessink
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

#ifndef __PREFERENCES_H_
#define __PREFERENCES_H_

enum {
	enable_syntax_scan,
	session_wrap_text,
	view_line_numbers,
	editor_tab_width,			/* editor tabwidth */
	editor_indent_wspaces,
	autoindent,					/* autoindent code */
	autocomplete,
	view_blocks,
	view_blockstack,
	show_mbhl,
	view_cline,
	default_mime_type,
	template,
	display_right_margin,
	show_visible_spacing,
#ifdef HAVE_LIBENCHANT
	session_spell_check,
#endif
	session_num_max
};

typedef struct {
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *prefs[session_num_max];
} Tsessionprefs;

void sessionprefs_apply(Tsessionprefs * sprefs, Tsessionvars * sessionvars);
Tsessionprefs *sessionprefs(const gchar * label, Tsessionprefs * sprefs, Tsessionvars * sessionvars);

enum {
	NAMECOL,
	WIDGETCOL,
	FUNCCOL,
	DATACOL
};

typedef void (*PrefInitguiCallback) (GtkTreeStore *nstore, GtkTreeIter *pit, GSList **widgetfreelist);
typedef void (*PrefApplyCallback) (void);

void preferences_dialog_new(Tbfwin *bfwin);

#endif							/* __PREFERENCES_H_ */
