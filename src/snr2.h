/* Bluefish HTML Editor
 * snr2.h - the search 'n replace rewrite prototypes
 *
 * Copyright (C) 2000-2002 Olivier Sessink
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

#ifndef __SNR2_H_
#define __SNR2_H_

void copy_cb(GtkWidget * w, gpointer data);
void paste_cb(GtkWidget * w, gpointer data);
void cut_cb(GtkWidget * w, gpointer data);
void sel_all_cb(GtkWidget * w, gpointer data);

void snr2_run_extern_replace(gchar *pattern, gint region,
							gint is_regex, gint is_case_sens, gchar *replace_string);
void search_cb(GtkWidget *widget, gpointer data);
void new_search_cb(GtkWidget *widget, gpointer data);
void replace_again_cb(GtkWidget *widget, gpointer data);
void search_again_cb(GtkWidget *widget, gpointer data);
void replace_cb(GtkWidget *widget, gpointer data);
void new_replace_cb(GtkWidget *widget, gpointer data);
void update_filenames_in_file(Tdocument *doc, gchar *oldfilename, gchar *newfilename, gint doc_has_newfilename);
void update_encoding_meta_in_file(Tdocument *doc, gchar *encoding);

#endif							/* __SNR_H_ */
