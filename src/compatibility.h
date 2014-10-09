/* Bluefish HTML Editor
 * compatibility.h
 *
 * Copyright (C) 2014 Olivier Sessink
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
#include "config.h"

#if GTK_CHECK_VERSION(3,0,0)
#include <gdk/gdkkeysyms-compat.h>
#else
#include <gdk/gdkkeysyms.h>
#endif

#if !GLIB_CHECK_VERSION(2,28,0)
void g_list_free_full(GList *list,GDestroyNotify free_func);
#endif

#ifndef HAVE_STRCASESTR
char *strcasestr(char *a, char *b);
#endif

/* Avoid lots of warnings from API depreciated in GTK 3.0. -Wdeprecated-declarations */
#if GTK_CHECK_VERSION(3,0,0)
#define gtk_hbox_new(arg, arg2) gtk_box_new(GTK_ORIENTATION_HORIZONTAL, arg2)
#define gtk_vbox_new(arg, arg2) gtk_box_new(GTK_ORIENTATION_VERTICAL, arg2)
#endif

/* backwards compatibility */
#if !GTK_CHECK_VERSION(2,24,0)
#define GTK_COMBO_BOX_TEXT(arg) GTK_COMBO_BOX(arg)
#define gtk_combo_box_text_get_active_text gtk_combo_box_get_active_text
#define gtk_combo_box_text_new_with_entry gtk_combo_box_entry_new_text
#define gtk_combo_box_text_new gtk_combo_box_new_text
#define gtk_combo_box_text_append_text gtk_combo_box_append_text
#define gtk_combo_box_text_prepend_text gtk_combo_box_prepend_text
#define gtk_combo_box_text_remove gtk_combo_box_remove_text
#endif /* GTK_CHECK_VERSION(2,24,0) */

#ifdef OLD_MAC_INTEGRATION
#define GtkosxApplication GtkOSXApplication
#define gtkosx_application_ready gtk_osxapplication_ready
#define GTKOSX_TYPE_APPLICATION GTK_TYPE_OSX_APPLICATION
#define gtkosx_application_sync_menubar gtk_osxapplication_sync_menubar
#define gtkosx_application_get_resource_path quartz_application_get_resource_path
#define gtkosx_application_set_window_menu gtk_osxapplication_set_window_menu
#define gtkosx_application_insert_app_menu_item gtk_osxapplication_insert_app_menu_item
#define gtkosx_application_set_menu_bar gtk_osxapplication_set_menu_bar
#define gtkosx_application_get_bundle_id quartz_application_get_bundle_id
#endif


