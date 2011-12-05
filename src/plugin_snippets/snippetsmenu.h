/* Bluefish HTML Editor
 * snippetsmenu.h
 *
 * Copyright (C) 2009 Olivier Sessink
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
#ifndef __SNIPPETSMENU_H_
#define __SNIPPETSMENU_H_


/*****************************************************************/
/* stuff for the widget */
/*****************************************************************/

#define SNIPPETS_TYPE_MENU            (snippets_menu_get_type())
#define SNIPPETS_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), SNIPPETS_TYPE_MENU, SnippetsMenu))
#define SNIPPETS_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), SNIPPETS_TYPE_MENU, SnippetsMenuClass))
#define SNIPPETS_IS_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), SNIPPETS_TYPE_MENU))
#define SNIPPETS_IS_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), SNIPPETS_TYPE_MENU))
#define SNIPPETS_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SNIPPETS_TYPE_MENU, SnippetsMenuClass))

typedef struct _SnippetsMenu SnippetsMenu;
typedef struct _SnippetsMenuClass SnippetsMenuClass;

typedef void (* SnippetMenuCallback) (gpointer user_data, gpointer data);

struct _SnippetsMenu {
	GtkMenuBar parent;
	gint maxwidth;
	gint data_column;
	gint name_column;
	gint accel_column;
	SnippetMenuCallback callback;
	gpointer user_data;
};

struct _SnippetsMenuClass {
	GtkMenuBarClass parent_class;
};

GType snippets_menu_get_type(void);

GtkWidget *snippets_menu_new(gint maxwidth);
void snippets_menu_set_model(SnippetsMenu * sm, GtkTreeModel * model, SnippetMenuCallback callback, gpointer user_data, gint name_column, gint data_column);

#endif /*__SNIPPETSMENU_H_*/
