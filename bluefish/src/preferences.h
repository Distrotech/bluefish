/* Bluefish HTML Editor
 * preferences.h - the preferences code
 *
 * Copyright (C) 2002-2010 Olivier Sessink
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

#ifndef __PREFERENCES_H_
#define __PREFERENCES_H_

enum {
	session_wrap_text,
	view_line_numbers,
	editor_tab_width,             /* editor tabwidth */
	autoindent,                   /* autoindent code */
	autocomplete,
	view_blocks,
	show_mbhl,
	view_cline,	
	default_mime_type,
	template,
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
void sessionprefs_apply(Tsessionprefs *sprefs, Tsessionvars *sessionvars);
Tsessionprefs *sessionprefs(const gchar *frame_title, Tsessionprefs *sprefs, Tsessionvars *sessionvars);
void open_preferences_cb(GtkWidget *wid, gpointer data);
void open_preferences_menu_cb(gpointer callback_data,guint action,GtkWidget *widget);

#endif /* __PREFERENCES_H_ */
