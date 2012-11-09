/* Bluefish HTML Editor
 * file_dialogs.h - file dialogs
 *
 * Copyright (C) 2005-2009 Olivier Sessink
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

#ifndef __FILEDIALOGS_H_
#define __FILEDIALOGS_H_

typedef enum {
	close_mode_per_file,
	close_mode_close_all,
	close_mode_cancel,
	close_mode_save_all
} Tclose_mode;

void file_close_all(Tbfwin * bfwin);
void file_new_doc(Tbfwin * bfwin);
void file_open_doc(Tbfwin * bfwin);
void file_save_all(Tbfwin * bfwin);


void files_advanced_win(Tbfwin * bfwin, gchar * basedir);
void file_open_advanced_cb(GtkWidget * widget, Tbfwin * bfwin);
void file_open_cb(GtkWidget * widget, Tbfwin * bfwin);
void file_open_url_cb(GtkAction * action, Tbfwin * bfwin);
gchar *ask_new_filename(Tbfwin * bfwin, const gchar * old_curi, const gchar * dialogtext);

typedef enum {
	docsave_normal,
	docsave_saveas,
	docsave_move,
	docsave_copy
} Tdocsave_mode;

void doc_save_backend(Tdocument * doc, Tdocsave_mode savemode, gboolean close_doc,
					  gboolean close_window);
void file_save_cb(GtkWidget * widget, Tbfwin * bfwin);
void file_save_as_cb(GtkWidget * widget, Tbfwin * bfwin);
void file_move_to_cb(GtkWidget * widget, Tbfwin * bfwin);
void file_save_all_cb(GtkWidget * widget, Tbfwin * bfwin);

void doc_save_all_close(Tbfwin * bfwin);
gint doc_modified_dialog(Tdocument * doc);
Tclose_mode multiple_files_modified_dialog(Tbfwin * bfwin);
gboolean choose_per_file(Tbfwin * bfwin, gboolean close_window);

gboolean doc_close_single_backend(Tdocument * doc, gboolean delay, gboolean close_window);
void file_close_cb(GtkWidget * widget, Tbfwin * bfwin);
void doc_close_multiple_backend(Tbfwin * bfwin, gboolean close_window, Tclose_mode close_mode);
void file_close_all_cb(GtkWidget * widget, Tbfwin * bfwin);

void file_new_cb(GtkWidget * widget, Tbfwin * bfwin);
void file_reload_all_modified(Tbfwin * bfwin);

void sync_dialog(Tbfwin * bfwin);

void modified_on_disk_check_init(void);

#endif							/* __FILEDIALOGS_H_ */
