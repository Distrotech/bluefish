/* Bluefish HTML Editor
 * treetips.h
 *
 * Copyright (C) 2003 Oskar Swida
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

#ifndef TREETIPS_H
#define TREETIPS_H

#include <gtk/gtk.h>
#include "bluefish.h"

/* function to create treetip content 
   Note: there is no 'static' content in treetips, you should provide a function
*/
typedef gchar* (*TreeTipsCFunc) (gconstpointer bfwin, gconstpointer tree, gint x, gint y);

typedef struct {
	GtkTreeView *tree;
	Tbfwin *bfwin;
	GtkWidget *window;
	gint posx,posy;
	gint show_tout,hide_tout;
	gboolean tip_on,tip_outside,enabled;	
	TreeTipsCFunc func;
	gint show_interval,hide_interval;
} TreeTips;

#define TREE_TIPS(var) ((TreeTips *)(var))


TreeTips *tree_tips_new(Tbfwin *win);
TreeTips *tree_tips_new_full(Tbfwin *win,GtkTreeView *tree,TreeTipsCFunc function);
void tree_tips_set_show_interval(TreeTips *tips,gint interv);
void tree_tips_set_hide_interval(TreeTips *tips,gint interv);
void tree_tips_set_content_function(TreeTips *tips,TreeTipsCFunc func);
void tree_tips_set_tree(TreeTips *tips,GtkTreeView *tree);
void tree_tips_destroy(TreeTips *tips);
void tree_tips_set_enabled(TreeTips *tips,gboolean en);


#endif /* TREETIPS_H */
