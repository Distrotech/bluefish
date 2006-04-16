/* Bluefish HTML Editor
 * autocomp.h - autocompletion system
 *
 * Copyright (C) 2006  Oskar Świda oskar.swida@gmail.com
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


#include <glib.h>
#include <gtk/gtk.h>
#ifndef __AUTOCOMP_H__
#define __AUTOCOMP_H__


/* Autocompletion structure which can be used in external modules
	I need only one of this for whole Bluefish process */
	
typedef struct {
	GtkTreeView *tree;    				 /* this is tree widget keeping ac strings */
	GtkListStore *store;  				 /* list of strings */
	GtkWidget *window; 				 /* small window shown when ac is activated */
	GCompletion *gc;        				/* autocompletion object used for run */
	GHashTable *lang_lists;  		/* lists of strings which are assigned by file type (.bflang file) */
	GHashTable *dtd_lists;			/* lists for external DTD */
} Tautocomp;


Tautocomp *ac_init();

gchar *ac_run(Tautocomp *ac, GList *strings, gchar *prefix, GtkTextView *view);
gchar *ac_run_lang(Tautocomp *ac, gchar *prefix, gchar *name, GtkTextView *view);
gchar *ac_run_dtd(Tautocomp *ac, gchar *prefix, gchar *name, GtkTextView *view);

void ac_add_lang_list(Tautocomp *ac, gchar *name, GList *strings);
void ac_add_dtd_list(Tautocomp *ac, gchar *name, GList *strings);


#endif /* __AUTOCOMP_H__ */
