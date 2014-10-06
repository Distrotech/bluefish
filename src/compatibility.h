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
#define GDK_KEY_Enter GDK_Enter
#define GDK_KEY_Return GDK_Return
#define GDK_KEY_KP_Enter GDK_KP_Enter
#define GDK_KEY_Home GDK_Home
#define GDK_KEY_KP_Home GDK_KP_Home
#define GDK_KEY_End GDK_End
#define GDK_KEY_KP_End GDK_KP_End
#define GDK_KEY_Tab GDK_Tab
#define GDK_KEY_KP_Tab GDK_KP_Tab
#define GDK_KEY_ISO_Left_Tab GDK_ISO_Left_Tab
#define GDK_KEY_Up GDK_Up
#define GDK_KEY_Down GDK_Down
#define GDK_KEY_Page_Down GDK_Page_Down
#define GDK_KEY_Page_Up GDK_Page_Up
#define GDK_KEY_Right GDK_Right
#define GDK_KEY_KP_Right GDK_KP_Right
#define GDK_KEY_Left GDK_Left
#define GDK_KEY_KP_Left GDK_KP_Left
#define GDK_KEY_Escape GDK_Escape
#define GDK_KEY_0 GDK_0
#define GDK_KEY_1 GDK_1
#define GDK_KEY_2 GDK_2
#define GDK_KEY_3 GDK_3
#define GDK_KEY_4 GDK_4
#define GDK_KEY_5 GDK_5
#define GDK_KEY_6 GDK_6
#define GDK_KEY_7 GDK_7
#define GDK_KEY_8 GDK_8
#define GDK_KEY_9 GDK_9
#define GDK_KEY_F1 GDK_F1
#define GDK_KEY_F12 GDK_F12
#define GDK_KEY_Delete GDK_Delete
#define GDK_KEY_BackSpace GDK_BackSpace
#define GDK_KEY_KP_Delete GDK_KP_Delete
#define GDK_KEY_Alt_L GDK_Alt_L
#define GDK_KEY_Alt_R GDK_Alt_R
#define GDK_KEY_Control_L GDK_Control_L
#define GDK_KEY_Control_R GDK_Control_R

/*#define GDK_KEY_ GDK_*/
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


