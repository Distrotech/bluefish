/* Bluefish HTML Editor
 * filebrowser2.h - the filebrowser v2
 *
 * Copyright (C) 2005,2006,2007,2008 Olivier Sessink
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

#ifndef __FILEBROWSER2_H_
#define __FILEBROWSER2_H_

#include "bluefish.h"
#include "config.h"
#include "file_treemodel.h"
#define DIR_MIME_TYPE "inode/directory"
#define MIME_ISDIR(string) (strcmp(string, "inode/directory")==0)

typedef struct {
	FileTreemodel *ftm;
	GFile *uri_to_refresh;
	guint delayed_refresh_id;
} Tfilebrowser2config;
#define FB2CONFIG(var) ((Tfilebrowser2config *)(var))

void fb2_refresh_dir_from_uri(GFile *dir);
void fb2_refresh_parent_of_uri(GFile *child_uri);
void fb2_focus_document(Tbfwin *bfwin, Tdocument *doc);
void fb2_file_is_opened(GFile *uri, const gchar *mimetype);
void fb2_file_is_closed(GFile *uri);
void fb2_update_settings_from_session(Tbfwin *bfwin, Tdocument *active_doc);
GtkWidget *fb2_init(Tbfwin *bfwin);
void fb2_cleanup(Tbfwin *bfwin);
void fb2_unset_filter(Tbfwin *bfwin, Tfilter *filter);
void fb2config_init(void);
void fb2config_cleanup(void);
GFile *find_common_path(GFile *file1, GFile *file2);
gboolean fb2_check_reset_basedir(GFile *uri);
#endif /* __FILEBROWSER2_H_ */
