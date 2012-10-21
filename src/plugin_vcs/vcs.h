/* Bluefish HTML Editor
 * vcs.h - version control
 *
 * Copyright (C) 2012 Olivier Sessink
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

#ifndef __VCS_H_
#define __VCS_H_

#define ENABLEPLUGINS

#include "../bluefish.h"

#ifdef ENABLE_NLS
#undef _
#define _(String) dgettext(PACKAGE"_plugin_vcs", String)
#endif

typedef struct {
	GHashTable *lookup;
} Tvcs;

typedef struct {
	gchar *type;
	gchar *basedir;
} Tvcssession;

typedef enum {
	vcsstatus_normal,
	vcsstatus_modified,
	vcsstatus_update,
	vcsstatus_conflict
} Tvcsstatus;

typedef struct {
	gchar *filename;
	Tvcsstatus status;
} Tvcsfilestatus;

typedef void (*VcsStatusCallback) (gpointer vcsdialog, GList *statuslist);
void vcs_status(Tbfwin *bfwin, Tvcssession *vs, gpointer data);
void vcs_commit_real(Tbfwin *bfwin, Tvcssession *vs, GList *files, const gchar *message);

#endif /* __VCS_H_  */
