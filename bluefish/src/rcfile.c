/* Bluefish HTML Editor
 * rcfile.c - loading and parsing of the configfiles
 *
 * Copyright (C) 2000-2012 Olivier Sessink
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

#define CURCONFIG "rcfile-2.0"
#define OLDCONFIG "rcfile_v2"
#define CURSESSION "session-2.0"
#define OLDSESSION "session"

/*****
 * THE WORKING OF THE CONFIG FILES AND STRUCTURES IN BLUEFISH
 *
 * there are 3 different places to store options in bluefish
 *
 * 1) main_v->props.item stored in ~/.bluefish/rcfile_v2
 * these items are only stored after preferences OK button
 * is hit, so they should only change if they are changed
 * by the user in the preferences panel
 * They should be added in bluefish.h in struct Tproperties
 * They should be added in rcfile.c in props_init_main
 * They should be added in preferences.c
 *        - in the enum on top
 *        - in preferences_dialog() to create the widget
 *        - in the preferences_apply(Tprefdialog *pd) to do something with the new value
 *
 * 2) main_v->globses.item stored in ~/.bluefish/session
 * these are the *global* session variables. global means
 * that they cannot have a different value in projects. This
 * file is saved on exit. Items in here should be items that
 * are always global for all bluefish windows, but can be
 * changed outside the preference panel (such as main window
 * width, or recent project files)
 * They should be added in bluefish.h in struct Tglobalsession
 * They should be in rcfile.c in return_globalsession_configlist()
 * They can be set in any bluefish file
 *
 * 3) bfwin->session->item, stored in ~/.bluefish/session for non-project
 * windows, or stored as part of the project for project windows
 * these are specific session variables. that means they can be different
 * for project windows. Items in here should be items that are useful to
 * have a different value in a project. For example the encoding, the spell
 * check language, the filebrowser filter, etc.
 * They should be added in bluefish.h in struct Tsessionvars
 * They should be in rcfile.c in return_session_configlist()
 * They *probably* should be added to project.c
 *        - in project_setup_initial_session()
 *        - in setup_bfwin_for_nonproject()
 *        - in project_open_from_file()
 *
 */

/* #define DEBUG */

#include <gtk/gtk.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "bluefish.h"
#include "rcfile.h"
#include "bf_lib.h"
#include "document.h"			/* DOCUMENT_BACKUP_ABORT_ASK */
#include "plugins.h"
#include "stringlist.h"


typedef struct {
	void *pointer;				/* where should the value be stored ? */
	unsigned char type;			/* a=arraylist, l=stringlist, s=string, e=string with escape, i=integer, m=limiTed stringlist */
	gchar *identifier;			/* the string that should be in the config file for this entry */
	gint len;					/* used for arrays and limitedstringlists, the length the list should have (only during save)
								   , or the number of items the array should have (during load) */
} Tconfig_list_item;

static GHashTable *main_configlist = NULL;

static void
free_configlist(GHashTable * configlist)
{
	g_hash_table_destroy(configlist);
}

static void
free_config_list_item(gpointer data)
{
	g_slice_free(Tconfig_list_item, data);
}

/*this should add 1 empty entry to the configuration list */
GHashTable *
make_config_list_item(GHashTable * config_list, void *pointer_to_var, unsigned char type_of_var,
					  gchar * name_of_var, gint len)
{
	Tconfig_list_item *config_list_item;
	if (!pointer_to_var) {
		DEBUG_MSG("make_config_list_item, pointer to var = NULL !\n");
		return config_list;
	}
	config_list_item = g_slice_new(Tconfig_list_item);
	config_list_item->pointer = pointer_to_var;
	config_list_item->type = type_of_var;
	config_list_item->identifier = name_of_var;
	config_list_item->len = len;
	g_hash_table_replace(config_list, name_of_var, config_list_item);
	return config_list;
}

static void
init_prop_integer(GHashTable ** config_list, void *pointer_to_var, gchar * name_of_var,
				  gint default_value, gboolean set_default)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 'i', name_of_var, 0);
	if (set_default)
		*(gint *) pointer_to_var = default_value;
}

static void
init_prop_string(GHashTable ** config_list, void *pointer_to_var, gchar * name_of_var,
				 const gchar * default_value)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 's', name_of_var, 0);
	if (*(gchar **) pointer_to_var == NULL) {
		*(gchar **) pointer_to_var = g_strdup(default_value);
	}
	DEBUG_MSG("init_prop_string, name_of_var=%s, default_value=%s, current value=%s\n", name_of_var,
			  default_value, *(gchar **) pointer_to_var);
}

static void
init_prop_string_with_escape(GHashTable ** config_list, void *pointer_to_var, gchar * name_of_var,
							 gchar * default_value)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 'e', name_of_var, 0);
	if (*(gchar **) pointer_to_var == NULL && default_value) {
		*(gchar **) pointer_to_var = unescape_string(default_value, FALSE);
	}
	DEBUG_MSG("init_prop_string, name_of_var=%s, default_value=%s\n", name_of_var, default_value);
}

static void
init_prop_stringlist(GHashTable ** config_list, void *pointer_to_var, gchar * name_of_var, gboolean setNULL)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 'l', name_of_var, 0);
	if (setNULL) {
		pointer_to_var = NULL;
	}
}

static void
init_prop_arraylist(GHashTable ** config_list, void *pointer_to_var, gchar * name_of_var,
					gint len, gboolean setNULL)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 'a', name_of_var, len);
	if (setNULL) {
		pointer_to_var = NULL;
	}
}

/* limited lists should have the most recent on top, the last entries will be cut if there are too many entries */
static void
init_prop_limitedstringlist(GHashTable ** config_list, void *pointer_to_var, gchar * name_of_var,
							gint len, gboolean setNULL)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 'm', name_of_var, len);
	if (setNULL) {
		pointer_to_var = NULL;
	}
}

static gint
save_config_file(GHashTable * config_list, GFile * file)
{
	gchar *tmpstring = NULL, *tmpstring2;
	GList *rclist, *tmplist2;
	GHashTableIter iter;
	gpointer key, value;
	gboolean retval;

	DEBUG_MSG("save_config_file, started\n");

	rclist = NULL;

/* We must first make a list with 1 string per item. */
	g_hash_table_iter_init(&iter, config_list);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		Tconfig_list_item *tmpitem = value;
		DEBUG_MSG("save_config_file, identifier=%s datatype %c\n", tmpitem->identifier, tmpitem->type);
		switch (tmpitem->type) {
		case 'i':
			DEBUG_MSG("save_config_file, converting \"%p\" to integer\n", tmpitem);
			DEBUG_MSG("save_config_file, converting \"%s %i\"\n", tmpitem->identifier,
					  *(int *) (void *) tmpitem->pointer);

			tmpstring = g_strdup_printf("%s %i", tmpitem->identifier, *(int *) (void *) tmpitem->pointer);

			DEBUG_MSG("save_config_file, adding %s\n", tmpstring);

			rclist = g_list_append(rclist, tmpstring);
			break;
		case 's':
			DEBUG_MSG("save_config_file, converting \"%p\" to string\n", tmpitem);
			DEBUG_MSG("save_config_file, converting \"%s %s\"\n", tmpitem->identifier,
					  (gchar *) * (void **) tmpitem->pointer);
			if (*(void **) tmpitem->pointer) {
				tmpstring =
					g_strdup_printf("%s %s", tmpitem->identifier, (gchar *) * (void **) tmpitem->pointer);

				DEBUG_MSG("save_config_file, adding %s\n", tmpstring);

				rclist = g_list_append(rclist, tmpstring);
			}
			break;
		case 'e':
			DEBUG_MSG("save_config_file, converting \"%p\"\n", tmpitem);
			DEBUG_MSG("save_config_file, converting \"%s %s\"\n", tmpitem->identifier,
					  (gchar *) * (void **) tmpitem->pointer);
			if (*(void **) tmpitem->pointer) {
				tmpstring2 = escape_string((gchar *) * (void **) tmpitem->pointer, FALSE);
				tmpstring = g_strdup_printf("%s %s", tmpitem->identifier, tmpstring2);

				DEBUG_MSG("save_config_file, adding %s\n", tmpstring);

				rclist = g_list_append(rclist, tmpstring);
				g_free(tmpstring2);
			}
			break;
		case 'l':
		case 'm':{
				gint max = -1;	/* by setting it to -1, it will never become zero if we substract 1 every round */
				DEBUG_MSG("save_config_file, type %c, tmpitem(%p), &tmpitem=%p\n", tmpitem->type, tmpitem,
						  &tmpitem);
				if (tmpitem->type == 'm')
					max = tmpitem->len;
				tmplist2 = g_list_last((GList *) * (void **) tmpitem->pointer);
				while (tmplist2 != NULL && max != 0) {
					tmpstring2 = escape_string((char *) tmplist2->data, FALSE);
					tmpstring = g_strconcat(tmpitem->identifier, " ", tmpstring2, NULL);
					g_free(tmpstring2);
					DEBUG_MSG("save_config_file, tmpstring(%p)=%s, orig was %s\n", tmpstring, tmpstring,
							  (gchar *) tmplist2->data);
					rclist = g_list_append(rclist, tmpstring);
					tmplist2 = g_list_previous(tmplist2);
					max--;
#ifdef DEBUG
					if (max == 0 && tmplist2 != NULL)
						g_print("save_config_file, limit reached!, next item would have been %s\n",
								(gchar *) tmplist2->data);
#endif
				}
			}
			break;
		case 'a':
			DEBUG_MSG("save_config_file, tmpitem(%p), &tmpitem=%p\n", tmpitem, &tmpitem);
			tmplist2 = g_list_last((GList *) * (void **) tmpitem->pointer);
			DEBUG_MSG("save_config_file, the tmplist2(%p)\n", tmplist2);
			while (tmplist2 != NULL) {
				tmpstring2 = array_to_string((char **) tmplist2->data);
				if (tmpitem->identifier)
					tmpstring = g_strdup_printf("%s %s", tmpitem->identifier, tmpstring2);
				else
					tmpstring = g_strdup_printf("%s", tmpstring2);
				DEBUG_MSG("save_config_file, tmpstring(%p)=%s\n", tmpstring, tmpstring);
				rclist = g_list_append(rclist, tmpstring);
				tmplist2 = g_list_previous(tmplist2);
				g_free(tmpstring2);
			}
			break;
		default:
			break;
		}
	}
	DEBUG_MSG("save_config_file, will save list with len %d to file\n", g_list_length(rclist));
	retval = put_stringlist(file, rclist, FALSE);
	free_stringlist(rclist);
	return retval;
}

static gboolean
parse_config_file(GHashTable * config_list, GFile * file)
{
	gboolean retval = FALSE;
	gchar *tmpstring = NULL, *tmpstring2;
	gchar **tmparray;
	GList *rclist, *tmplist;
	Tconfig_list_item *tmpitem;
	GHashTableIter iter;
	gpointer key, value;

	DEBUG_MSG("parse_config_file, started\n");

	rclist = NULL;
	rclist = get_list(file, rclist, FALSE);

	if (rclist == NULL) {
		DEBUG_MSG("no rclist, returning!\n");
		return retval;
	}

	/* empty all variables that have type GList ('l') */
	g_hash_table_iter_init(&iter, config_list);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		tmpitem = (Tconfig_list_item *) value;
		DEBUG_MSG("parse_config_file, type=%c, identifier=%s\n", tmpitem->type, tmpitem->identifier);
		if (tmpitem->type == 'l' || tmpitem->type == 'a') {
			DEBUG_MSG("parse_config_file, freeing list before filling it\n");
			free_stringlist((GList *) * (void **) tmpitem->pointer);
			*(void **) tmpitem->pointer = (GList *) NULL;
		}
		DEBUG_MSG("parse_config_file, type=%c, identifier=%s\n", tmpitem->type, tmpitem->identifier);
	}
	DEBUG_MSG("parse_config_file, all the type 'l' and 'a' have been emptied\n");
	DEBUG_MSG("parse_config_file, length rclist=%d\n", g_list_length(rclist));
/* And now for parsing every line in the config file, first check if there is a valid identifier at the start. */
	for (tmplist = g_list_first(rclist); tmplist; tmplist = g_list_next(tmplist)) {
		tmpstring = (gchar *) tmplist->data;
		if (tmpstring != NULL && tmpstring[0] != '\0') {
			gchar *key, *tmp;
			g_strchug(tmpstring);
			DEBUG_MSG("parse_config_file, tmpstring=%s\n", tmpstring);
			tmp = strchr(tmpstring, ':');
			if (!tmp)
				break;
			key = g_strndup(tmpstring, tmp - tmpstring + 1);
			tmpitem = g_hash_table_lookup(config_list, key);
			g_free(key);
			if (tmpitem) {
				/* we have found the correct identifier */
				retval = TRUE;
				DEBUG_MSG("parse_config_file, identifier=%s, string=%s\n", tmpitem->identifier, tmpstring);
				/* move pointer past the identifier */
				tmpstring += strlen(tmpitem->identifier);
				trunc_on_char(tmpstring, '\n');
				g_strstrip(tmpstring);

				switch (tmpitem->type) {
				case 'i':
					*(int *) (void *) tmpitem->pointer = atoi(tmpstring);
					break;
				case 's':
					*(void **) tmpitem->pointer =
						(char *) realloc((char *) *(void **) tmpitem->pointer, strlen(tmpstring) + 1);
					strcpy((char *) *(void **) tmpitem->pointer, tmpstring);
					break;
				case 'e':
					tmpstring2 = unescape_string(tmpstring, FALSE);
					*(void **) tmpitem->pointer =
						(char *) realloc((char *) *(void **) tmpitem->pointer, strlen(tmpstring2) + 1);
					strcpy((char *) *(void **) tmpitem->pointer, tmpstring2);
					g_free(tmpstring2);
					break;
				case 'l':
				case 'm':
					/*tmpstring2 = g_strdup(tmpstring); */
					tmpstring2 = unescape_string(tmpstring, FALSE);
					*(void **) tmpitem->pointer =
						g_list_prepend((GList *) * (void **) tmpitem->pointer, tmpstring2);
					DEBUG_MSG("parse_config_file, *(void **)tmpitem->pointer=%p\n",
							  *(void **) tmpitem->pointer);
					break;
				case 'a':
					tmparray = string_to_array(tmpstring);
					if (tmpitem->len <= 0 || tmpitem->len == g_strv_length(tmparray)) {
						*(void **) tmpitem->pointer =
							g_list_prepend((GList *) * (void **) tmpitem->pointer, tmparray);
					} else {
						DEBUG_MSG("parse_config_file, not storing array, g_strv_length() != tmpitem->len\n");
						g_strfreev(tmparray);
					}
					DEBUG_MSG("parse_config_file, *(void **)tmpitem->pointer=%p\n",
							  *(void **) tmpitem->pointer);
					break;
				default:
					break;
				}
			}
		}
	}
	DEBUG_MSG("parse_config_file, parsed all entries, freeing list read from file\n");
	free_stringlist(rclist);
	return retval;
}

static GHashTable *
props_init_main(GHashTable * config_rc)
{
	init_prop_string(&config_rc, &main_v->props.config_version, "config_version:", "2.2.5");
	init_prop_integer(&config_rc, &main_v->props.do_periodic_check, "do_periodic_check:", 1, TRUE);
	init_prop_string(&config_rc, &main_v->props.editor_font_string, "editor_font_string:", "monospace 10");
	init_prop_integer(&config_rc, &main_v->props.editor_smart_cursor, "editor_smart_cursor:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.editor_tab_indent_sel, "editor_tab_indent_sel:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.editor_auto_close_brackets, "editor_auto_close_brackets:", 2, TRUE);
	init_prop_integer(&config_rc, &main_v->props.use_system_tab_font, "use_system_tab_font:", 1, TRUE);
	init_prop_string(&config_rc, &main_v->props.tab_font_string, "tab_font_string:", "");
	init_prop_string(&config_rc, &main_v->props.tab_color_modified, "tab_color_modified:", "#0000FF");
	init_prop_string(&config_rc, &main_v->props.tab_color_loading, "tab_color_loading:", "#C7C7C7");
	init_prop_string(&config_rc, &main_v->props.tab_color_error, "tab_color_error:", "#FF0000");
	init_prop_integer(&config_rc, &main_v->props.visible_ws_mode, "visible_ws_mode:", 3, TRUE);
	init_prop_integer(&config_rc, &main_v->props.right_margin_pos, "right_margin_pos:", 80, TRUE);
	init_prop_arraylist(&config_rc, &main_v->props.external_command, "external_command:", 0, TRUE);
	init_prop_arraylist(&config_rc, &main_v->props.external_filter, "external_filter:", 0, TRUE);
	init_prop_arraylist(&config_rc, &main_v->props.external_outputbox, "external_outputbox:", 0, TRUE);

	/*init_prop_integer   (&config_rc, &main_v->props.defaulthighlight, "defaulthighlight:", 1, TRUE); */
	/* old type filetypes have a different count, they are converted below */

	init_prop_integer(&config_rc, &main_v->props.leave_to_window_manager, "leave_to_window_manager:", 0,
					  TRUE);
	init_prop_integer(&config_rc, &main_v->props.restore_dimensions, "restore_dimensions:", 1, TRUE);

	init_prop_integer(&config_rc, &main_v->props.left_panel_left, "left_panel_left:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.hide_bars_on_fullscreen, "hide_bars_on_fullscreen:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.cursor_size, "cursor_size:", 4, TRUE);
	init_prop_integer(&config_rc, &main_v->props.highlight_cursor, "highlight_cursor:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.save_accelmap, "save_accelmap:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.max_recent_files, "max_recent_files:", 15, TRUE);
	init_prop_integer(&config_rc, &main_v->props.max_dir_history, "max_dir_history:", 10, TRUE);
	init_prop_integer(&config_rc, &main_v->props.backup_file, "backup_file:", 1, TRUE);
/*	init_prop_string    (&config_rc, &main_v->props.backup_suffix,"backup_suffix:","~");
	init_prop_string    (&config_rc, &main_v->props.backup_prefix,"backup_prefix:","");*/
	init_prop_integer(&config_rc, &main_v->props.backup_abort_action, "backup_abort_action:",
					  DOCUMENT_BACKUP_ABORT_ASK, TRUE);
	init_prop_integer(&config_rc, &main_v->props.backup_cleanuponclose, "backup_cleanuponclose:", 0, TRUE);
	init_prop_string(&config_rc, &main_v->props.image_thumbnailstring, "image_thumbnailstring:",
					 "_thumbnail");
	init_prop_string(&config_rc, &main_v->props.image_thumbnailtype, "image_thumbnailtype:", "jpeg");
	init_prop_integer(&config_rc, &main_v->props.modified_check_type, "modified_check_type:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.num_undo_levels, "num_undo_levels:", 100, TRUE);
	init_prop_integer(&config_rc, &main_v->props.clear_undo_on_save, "clear_undo_on_save:", 0, TRUE);
	init_prop_string(&config_rc, &main_v->props.newfile_default_encoding, "newfile_default_encoding:",
					 "UTF-8");
	init_prop_integer(&config_rc, &main_v->props.auto_set_encoding_meta, "auto_set_encoding_meta:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.auto_update_meta_author, "auto_update_meta_author:", 1,
					  TRUE);
	init_prop_integer(&config_rc, &main_v->props.auto_update_meta_date, "auto_update_meta_date:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.auto_update_meta_generator, "auto_update_meta_generator:", 1,
					  TRUE);
	init_prop_integer(&config_rc, &main_v->props.strip_trailing_spaces_on_save, "strip_trailing_spaces_on_save:", 0,
					  TRUE);
	init_prop_integer(&config_rc, &main_v->props.encoding_search_Nbytes, "encoding_search_Nbytes:", 500,
					  TRUE);
	init_prop_integer(&config_rc, &main_v->props.max_window_title, "max_window_title:", 0,TRUE);
	init_prop_integer(&config_rc, &main_v->props.document_tabposition, "document_tabposition:",
					  (gint) GTK_POS_BOTTOM, TRUE);
	init_prop_integer(&config_rc, &main_v->props.leftpanel_tabposition, "leftpanel_tabposition:",
					  (gint) GTK_POS_BOTTOM, TRUE);
	init_prop_integer(&config_rc, &main_v->props.switch_tabs_by_altx, "switch_tabs_by_altx:", 1, TRUE);
/*	init_prop_string    (&config_rc, &main_v->props.default_basedir,"default_basedir:",g_get_home_dir());*/
	init_prop_string(&config_rc, &main_v->props.project_suffix, "project_suffix:", ".bfproject");
	/* not yet in use */
	init_prop_integer(&config_rc, &main_v->props.allow_dep, "allow_the_use_of_font:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.format_by_context, "format_by_context:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.xhtml, "use_xhtml:", 1, TRUE);
/*	init_prop_integer(&config_rc, &main_v->props.insert_close_tag, "insert_close_tag:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.close_tag_newline, "close_tag_newline:", 1, TRUE);*/
	init_prop_integer(&config_rc, &main_v->props.smartindent, "smartindent:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.open_in_running_bluefish, "open_in_running_bluefish:", 1,
					  TRUE);
	init_prop_integer(&config_rc, &main_v->props.open_in_new_window, "open_in_new_window:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.register_recent_mode, "register_recent_mode:", 2, TRUE);
	init_prop_arraylist(&config_rc, &main_v->props.plugin_config, "plugin_config:", 3, TRUE);
	init_prop_integer(&config_rc, &main_v->props.use_system_colors, "use_system_colors:", 1, TRUE);
	init_prop_string(&config_rc, &main_v->props.btv_color_str[BTV_COLOR_ED_FG], "editor_fg:", "");
	init_prop_string(&config_rc, &main_v->props.btv_color_str[BTV_COLOR_ED_BG], "editor_bg:", "");
	init_prop_string(&config_rc, &main_v->props.btv_color_str[BTV_COLOR_CURRENT_LINE], "cline_bg:",
					 "#E0E0E0");
	init_prop_string(&config_rc, &main_v->props.btv_color_str[BTV_COLOR_WHITESPACE], "visible_ws:",
					 "#FF0000");
	init_prop_string(&config_rc, &main_v->props.btv_color_str[BTV_COLOR_RIGHT_MARGIN], "right_margin:",
					 "#990000");
	init_prop_string(&config_rc, &main_v->props.btv_color_str[BTV_COLOR_CURSOR], "cursor_color:", "");
	init_prop_string(&config_rc, &main_v->props.btv_color_str[BTV_COLOR_SELECTION], "selection_color:", "");
	init_prop_string(&config_rc, &main_v->props.btv_color_str[BTV_COLOR_CURSOR_HIGHLIGHT], "cursor_highlight_color:", "#ffff33");
	init_prop_arraylist(&config_rc, &main_v->props.textstyles, "textstyles3:", 6, TRUE);
	init_prop_integer(&config_rc, &main_v->props.block_folding_mode, "block_folding_mode:", 1, TRUE);
	init_prop_arraylist(&config_rc, &main_v->props.highlight_styles, "highlight_styles:", 3, TRUE);
	init_prop_arraylist(&config_rc, &main_v->props.bflang_options, "bflang_options:", 3, TRUE);
	init_prop_string(&config_rc, &main_v->props.autocomp_accel_string, "autocomp_accel_string:",
					 "<Control>space");
	init_prop_integer(&config_rc, &main_v->props.load_reference, "load_reference:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.show_autocomp_reference, "show_autocomp_reference:", 1,
					  TRUE);
	init_prop_integer(&config_rc, &main_v->props.show_tooltip_reference, "show_tooltip_reference:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.delay_full_scan, "delay_full_scan:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.delay_scan_time, "delay_scan_time:", 900, TRUE);
	init_prop_integer(&config_rc, &main_v->props.autocomp_popup_mode, "autocomp_popup_mode:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.reduced_scan_triggers, "reduce_scan_triggers:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.autosave, "autosave:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.autosave_time, "autosave_time:", 60, TRUE);
	init_prop_integer(&config_rc, &main_v->props.autosave_location_mode, "autosave_location_mode:", 0, TRUE);
	init_prop_string(&config_rc, &main_v->props.autosave_file_prefix, "autosave_file_prefix:", "");
	init_prop_string(&config_rc, &main_v->props.autosave_file_suffix, "autosave_file_suffix:", "#");
	init_prop_string(&config_rc, &main_v->props.language, "language:", "");

	init_prop_integer(&config_rc, &main_v->props.adv_textview_right_margin, "adv_textview_right_margin:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.adv_textview_left_margin, "adv_textview_left_margin:", 1, TRUE);
	/*init_prop_arraylist(&config_rc, &main_v->props.templates, "templates:", 2, TRUE);*/
	return config_rc;
}

static gboolean
merge_config_files(GFile * oldrc, GFile * oldsession, GFile * newrc, GFile * newsession)
{
	GError *gerror = NULL;
	GInputStream *istream;
	GOutputStream *ostream;

	istream = (GInputStream *) g_file_read(oldrc, NULL, &gerror);
	if (gerror) {
		g_print("config file migration error %d:%s", gerror->code, gerror->message);
		g_error_free(gerror);
		return FALSE;
	}
	ostream = (GOutputStream *) g_file_append_to(newrc, G_FILE_CREATE_PRIVATE, NULL, &gerror);
	if (gerror) {
		g_print("config file migration error %d:%s", gerror->code, gerror->message);
		g_error_free(gerror);
		g_input_stream_close(istream, NULL, &gerror);
		return FALSE;
	}
	g_output_stream_splice(ostream, istream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE, NULL, &gerror);
	if (gerror) {
		g_print("config file migration error %d:%s", gerror->code, gerror->message);
		g_error_free(gerror);
		g_output_stream_close(ostream, NULL, &gerror);
		return FALSE;
	}
	istream = (GInputStream *) g_file_read(oldsession, NULL, &gerror);
	if (gerror) {
		g_print("config file migration error %d:%s", gerror->code, gerror->message);
		g_error_free(gerror);
		g_output_stream_close(ostream, NULL, &gerror);
		return FALSE;
	}
	g_output_stream_splice(ostream, istream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE, NULL, &gerror);
	if (gerror) {
		g_print("config file migration error %d:%s", gerror->code, gerror->message);
		g_error_free(gerror);
		g_output_stream_close(ostream, NULL, &gerror);
		return FALSE;
	}
	g_output_stream_close(ostream, NULL, &gerror);
	g_file_copy(newrc, newsession, G_FILE_COPY_BACKUP, NULL, NULL, NULL, &gerror);
	if (gerror) {
		g_print("config file migration error %d:%s", gerror->code, gerror->message);
		g_error_free(gerror);
		return FALSE;
	}
	return TRUE;
}

static void
migrate_config_files(GHashTable * main_configlist, GFile * newrc)
{
	GFile *oldrc, *oldsession, *newsession;
	oldrc = user_bfdir(OLDCONFIG);
	oldsession = user_bfdir(OLDSESSION);
	newsession = user_bfdir(CURSESSION);
	merge_config_files(oldrc, oldsession, newrc, newsession);
	/* add some entries to main_configlist. because main_configlist is used for
	   saving as well this means we'll save these entries too */
	if (parse_config_file(main_configlist, newrc)) {
		/* are there any entries that we want to convert ?? */
		save_config_file(main_configlist, newrc);
	}
}


static gchar *
convert_old_placeholders(gchar *orig, gchar old1, gchar new1, gchar old2, gchar new2)
{
	gint i;
	gboolean prev_was_percent=FALSE;
	if (!orig)
		return NULL;

	for (i=0;orig[i]!='\0';i++) {
		if (prev_was_percent && orig[i] == old1) {
			orig[i] = new1;
		} else if (prev_was_percent && orig[i] == old2) {
			orig[i] = new2;
		}
		prev_was_percent=FALSE;
		if (orig[i]=='%') {
			prev_was_percent=TRUE;
		}
	}
	return orig;
}

/* on array length = newlen-1 we update to the new version

if (overwrite) we free the list and replace it with the default values

if (!overwrite) we overwrite all bluefish_defined options, and we append any new options
*/
static GList *update_externals(GList *current, GList *defaults, gboolean overwrite, gint newlen, gint commandindex)
{
	GList *tmplist;
	GHashTable *ht=NULL;

	if (current == NULL)
		overwrite = TRUE;

	if (overwrite) {
		free_arraylist(current);
		current = NULL;
	} else {
		ht = g_hash_table_new(g_str_hash, g_str_equal);
		tmplist = g_list_first(current);
		while (tmplist) {
			gchar **arr = tmplist->data;
			gint len;
			GList *cur;
			len = g_strv_length(arr);

			cur = tmplist;
			tmplist = g_list_next(tmplist);

			if (len == (newlen-1)) { /* convert the old (2.2.2 or older) format into the new format */
				gchar **oldarr = arr;
				/*g_print("prepend %s in front of %s\n",USER_DEFINED_ENABLED,arr[0]);*/
				cur->data = arr = prepend_array(USER_DEFINED_ENABLED,arr);
				g_strfreev(oldarr);
				/* arr[2] contains the commandstring. now replace %I and %O (previously fifo in and
				fifo out) to temporary file in %i and out %o */
				arr[commandindex] = convert_old_placeholders(arr[commandindex], 'O', 'o', 'I', 'i');
				len = newlen;
			}
			if (len == newlen && arr[0][0]!='0' && arr[0][0]!='1') {
				/* keep all the user defined options */
				/*g_print("update_externals, insert into hash: %s\n",arr[1]);*/
				g_hash_table_insert(ht, arr[1], arr);
			} else {
				/* remove all others */
				current = g_list_delete_link(current, cur);
				DEBUG_MSG("removed list item %p from list, current =%p\n",cur, current);
				g_strfreev(arr);
			}
		}
	}
	for (tmplist=g_list_first(defaults);tmplist;tmplist = g_list_next(tmplist)) {
		gchar **arr=tmplist->data;
		if (overwrite || g_hash_table_lookup(ht, arr[0])==NULL) {
			/*g_print("update_externals, prepend a missing or removed-default value %s\n",arr[0]);*/
			current = g_list_prepend(current, prepend_array(BLUEFISH_DEFINED_ENABLED, arr));
		}
	}
	if (ht)
		g_hash_table_destroy(ht);
	return current;
}

GList * update_outputbox(GList *current, gboolean overwrite) {
	GList *defaults = NULL, *retlist;
	DEBUG_MSG("update_outputbox, started\n");
#ifdef WIN32
	defaults =
			g_list_append(defaults,
						  array_from_arglist(_("php"),
											 "(.*) in (.*:\\\\[a-zA-Z0-9\\\\_.-]+) on line ([0-9]+)", "2",
											 "3", "1", "php '%f'|", NULL));
#else
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("make"), "([a-zA-Z0-9/_.-]+):([0-9]+):(.*)", "1", "2", "3",
										 "cd %c && make|", NULL));
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("weblint HTML checker"),
										 "([a-zA-Z0-9/_.-]+) \\(([0-9:]+)\\) (.*)", "1", "2", "3",
										 "weblint '%f'|", NULL));
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("tidy HTML validator"), "line ([0-9]+) column [0-9]+ - (.*)",
										 "-1", "1", "2", "tidy -qe '%i'|", NULL));
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("javac"), "([a-zA-Z0-9/_.-]+):([0-9]+):(.*)", "1", "2", "3",
										 "javac '%f'|", NULL));
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("xmllint XML checker"),
										 "([a-zA-Z0-9/_.-]+)\\:([0-9]+)\\: (.*)", "1", "2", "3",
										 "xmllint --noout --valid '%f'|", NULL));
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("php"), "(.*) in (/[a-zA-Z0-9/_.-]+) on line ([0-9]+)", "2",
										 "3", "1", "php '%f'|", NULL));
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("php syntax check"), "(.*) in (/[a-zA-Z0-9/_.-]+) on line ([0-9]+)", "2",
										 "3", "1", "php -l -q -f '%f'|", NULL));
	defaults =
		g_list_append(defaults,
						array_from_arglist(_("perl syntax check"), "(.*) at (/[a-zA-Z0-9/_.-]+) line ([0-9]+)", "2",
										 "3", "1", "perl -c '%f'|", NULL));
	defaults =
		g_list_append(defaults,
						array_from_arglist(_("php codesniffer"), ":([0-9:]+):(.*)", "-1",
										 "1", "2", "phpcs --report=emacs '%f'|", NULL));
	defaults =
		g_list_append(defaults,
						array_from_arglist(_("pylint"), "(C|R|E|F|W):[ ]*([0-9]+),[0-9]+:(.*)", "-1",
										 "2", "0", "pylint '%f'|", NULL));
#endif
	retlist = update_externals(current, defaults, overwrite, 7, 6);
	free_arraylist(defaults);
	return retlist;
}

GList *update_filters(GList *current, gboolean overwrite)
{
	GList *defaults=NULL, *retlist;

	defaults =
		g_list_append(defaults, array_from_arglist(_("Sort"), "|sort|", NULL));
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("Sort / Uniq"), "|sort|uniq|", NULL));
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("Reverse lines"), "|tac|", NULL));
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("Dos2unix"), "|dos2unix|", NULL));
	defaults =
		g_list_append(defaults, array_from_arglist(_("Tidy HTML"), "|tidy -utf8|", NULL));
	defaults =
		g_list_append(defaults, array_from_arglist(_("Tidy convert to XHTML"), "|tidy -utf8 -i -w 80 -c -q -asxhtml|", NULL));
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("Tidy HTML (perltidy)"), "|perltidy -b|", NULL));
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("Strip empty lines"), "|egrep -v '^[ \t]*$'|", NULL));
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("Render HTML to text"), "lynx -force_html -dump %i |", NULL));
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("PHP Beautifier"), "|php_beautifier -t|", NULL));
#ifdef WIN32
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("Javascript Minify with jsmin"), "|python "PKGDATADIR"/jsmin.py|", NULL));
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("CSS Minify with cssmin"), "|python "PKGDATADIR"/cssmin.py|", NULL));
#else
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("Javascript Minify with jsmin"), "|"PKGDATADIR"/jsmin.py|", NULL));
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("CSS Minify with cssmin"), "|"PKGDATADIR"/cssmin.py|", NULL));
#endif
	defaults =
		g_list_append(defaults,
					  array_from_arglist(_("CSS tidy"), "|csstidy --preserve_css=true -|", NULL));
	retlist = update_externals(current, defaults, overwrite, 3, 2);
	free_arraylist(defaults);
	return retlist;
}

GList *update_commands(GList *current, gboolean overwrite)
{
	GList *defaults=NULL, *retlist;
#ifdef WIN32
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("Windows XP Firefox"),
										 "\"C:\\Program Files\\Mozilla Firefox\\firefox.exe\" '%p'",
										 "1", NULL));
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("Windows XP Internet Explorer"),
										 "\"C:\\Program Files\\Internet Explorer\\iexplore.exe\" '%p'",
										 "0", NULL));
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("Windows XP Opera"),
										 "\"C:\\Program Files\\Opera\\opera.exe\" '%p'", "0", NULL));
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("Windows XP Safari"),
										 "\"C:\\Program Files\\Safari\\safari.exe\" '%p'", "0", NULL));
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("Windows 7 Firefox 32-bit"),
										 "\"C:\\Program Files (x86)\\Mozilla Firefox\\firefox.exe\" '%p'",
										 "1", NULL));
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("Windows 7 Internet Explorer 32-bit"),
										 "\"C:\\Program Files (x86)\\Internet Explorer\\iexplore.exe\" '%p'",
										 "0", NULL));
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("Windows 7 Internet Explorer 64-bit"),
										 "\"C:\\Program Files\\Internet Explorer\\iexplore.exe\" '%p'",
										 "0", NULL));
#elif PLATFORM_DARWIN
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("Safari"), "open -a Safari '%p'", "0", NULL));
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("Firefox"), "open -a Firefox '%p'", "1", NULL));
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("Opera"), "open -a Opera '%p'", "0", NULL));
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("chmod a+x"), "chmod a+x %f", "0", NULL));
#else
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("Firefox"), "firefox -remote 'openURL(%p)' || firefox '%p'&",
										 "1", NULL));
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("Opera"), "opera -remote 'openURL(%p)' || opera '%p'&", "0",
										 NULL));
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("Chromium"), "chromium-browser '%p'&", "0", NULL));
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("Konqueror"), "konqueror '%p'&", "0", NULL));
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("Epiphany"), "epiphany-browser -n '%p'&", "0", NULL));
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("Kazehakase"), "kazehakase '%p'&", "0", NULL));
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("Links2 (graphics)"), "links2 -g '%p'&", "0", NULL));
	defaults =
		g_list_prepend(defaults,
					  array_from_arglist(_("chmod a+x"), "chmod a+x %f", "0", NULL));
#endif
	retlist = update_externals(current, defaults, overwrite, 4, 2);
	free_arraylist(defaults);
	return retlist;
}

void
rcfile_parse_main(void)
{
	GFile *file;

	DEBUG_MSG("rcfile_parse_main, started\n");

	/* set the props struct completely empty */
	memset(&main_v->props, 0, sizeof(Tproperties));

	/*Make the config_rc list ready for filling with data and set default values */
	main_configlist =
		props_init_main(g_hash_table_new_full(g_str_hash, g_str_equal, NULL, free_config_list_item));

	file = user_bfdir(CURCONFIG);
	if (!parse_config_file(main_configlist, file)) {
		g_warning("no configfile %s, try to convert config files from older versions\n", CURCONFIG);
		/* probably there is no configfile. try to migrate the configfile from a previous
		   version */
		migrate_config_files(main_configlist, file);
	}
	g_object_unref(file);
	if (main_v->props.encoding_search_Nbytes < 1000)
		main_v->props.encoding_search_Nbytes = 2048;

	/* do some default configuration for the lists */
	if (!main_v->props.config_version || strlen(main_v->props.config_version)<5 ||main_v->props.config_version[0] < '2' || main_v->props.config_version[2] < '2' || main_v->props.config_version[4] < '4') {
		main_v->props.rcfile_from_old_version = 1;
		main_v->props.external_command = update_commands(main_v->props.external_command, FALSE);
		main_v->props.external_filter = update_filters(main_v->props.external_filter, FALSE);
		main_v->props.external_outputbox = update_outputbox(main_v->props.external_outputbox, FALSE);
	}
#ifdef OLDTEMPLATES
	if (main_v->props.templates == NULL) {
		main_v->props.templates =
			g_list_append(main_v->props.templates,
						  array_from_arglist(_("C Header"), PKGDATADIR "/templates/C_header", NULL));
		main_v->props.templates =
			g_list_append(main_v->props.templates,
						  array_from_arglist(_("C Header GPL"), PKGDATADIR "/templates/C_header_GPL", NULL));
		main_v->props.templates =
			g_list_append(main_v->props.templates,
						  array_from_arglist(_("XHTML 1.0"), PKGDATADIR "/templates/XHTML_1.0", NULL));
		main_v->props.templates =
			g_list_append(main_v->props.templates,
						  array_from_arglist(_("HTML 5"), PKGDATADIR "/templates/HTML_5", NULL));
		main_v->props.templates =
			g_list_append(main_v->props.templates,
						  array_from_arglist(_("PHP"), PKGDATADIR "/templates/PHP", NULL));
		main_v->props.templates =
			g_list_append(main_v->props.templates,
						  array_from_arglist(_("LaTex Book"), PKGDATADIR "/templates/Latex_Book", NULL));
		main_v->props.templates =
			g_list_append(main_v->props.templates,
						  array_from_arglist(_("LaTex Presentation"), PKGDATADIR "/templates/Latex_Presentation", NULL));
		/* TODO: list the templates in the directory */
	}
#endif
}

gint
rcfile_save_main(void)
{
	gint ret;
	GFile *filename = user_bfdir(CURCONFIG);

	g_free(main_v->props.config_version);
	main_v->props.config_version = g_strdup(VERSION);

	ret = save_config_file(main_configlist, filename);
	g_object_unref(filename);
	return ret;
}

void
rcfile_check_directory(void)
{
	GFile *file, *rcpath;
	GFileInfo *finfo;
	GError *gerror = NULL;

	file = g_file_new_for_path(g_get_home_dir());
	rcpath = g_file_get_child(file, ".bluefish");

	DEBUG_PATH("test if rcfile directory %s exists\n", rcpath);
	finfo = g_file_query_info(rcpath, G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_QUERY_INFO_NONE, NULL, &gerror);
	if (gerror && gerror->code == G_IO_ERROR_NOT_FOUND) {
		g_clear_error(&gerror);

		g_file_make_directory(rcpath, NULL, &gerror);
		if (gerror) {
			g_warning("Error creating directory: .bluefish\n");
			g_warning("%s", gerror->message);
			g_error_free(gerror);
		}
	}

	g_object_unref(rcpath);
	g_object_unref(file);
	if (finfo)
		g_object_unref(finfo);
}

static gchar *
get_user_accelmap(void)
{
	return g_strconcat(g_get_home_dir(), "/." PACKAGE "/menudump_2", NULL);
}

void
rcfile_save_accelerators(void)
{
#ifndef MAC_INTEGRATION
	gchar *shortcutfilename = get_user_accelmap();
	gtk_accel_map_save(shortcutfilename);
	g_free(shortcutfilename);
#endif
}

void
rcfile_load_accelerators(gboolean defaultmap)
{
#ifndef MAC_INTEGRATION
	gchar *shortcutfilename;
	if (defaultmap) {
		shortcutfilename = g_strdup(PKGDATADIR"/default_accelmap");
	} else {
		shortcutfilename = get_user_accelmap();
	}
	gtk_accel_map_load(shortcutfilename);
	g_free(shortcutfilename);
#endif
}

static GHashTable *
return_globalsession_configlist(gboolean init_values)
{
	GHashTable *config_rc = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, free_config_list_item);
	init_prop_integer(&config_rc, &main_v->globses.main_window_h, "main_window_height:", 400, init_values);
	init_prop_integer(&config_rc, &main_v->globses.main_window_w, "main_window_width:", 600, init_values);	/* negative width means maximized */
	init_prop_integer(&config_rc, &main_v->globses.two_pane_filebrowser_height,
					  "two_pane_filebrowser_height:", 250, init_values);
	init_prop_integer(&config_rc, &main_v->globses.left_panel_width, "left_panel_width:", 150, init_values);
	/*init_prop_integer   (&config_rc, &main_v->globses.lasttime_filetypes, "lasttime_filetypes:", 0, init_values);
	   init_prop_integer   (&config_rc, &main_v->globses.lasttime_encodings, "lasttime_encodings:", 0, init_values); */
	init_prop_integer(&config_rc, &main_v->globses.bookmarks_default_store, "bookmarks_default_store:", 1,
					  init_values);
	init_prop_integer(&config_rc, &main_v->globses.image_thumbnail_refresh_quality,
					  "image_thumbnail_refresh_quality:", 1, init_values);
	init_prop_integer(&config_rc, &main_v->globses.image_thumbnailsizing_type, "image_thumbnailsizing_type:",
					  0, init_values);
	init_prop_integer(&config_rc, &main_v->globses.image_thumbnailsizing_val1, "image_thumbnailsizing_val1:",
					  100, init_values);
	init_prop_integer(&config_rc, &main_v->globses.image_thumbnailsizing_val2, "image_thumbnailsizing_val2:",
					  100, init_values);
	init_prop_string(&config_rc, &main_v->globses.image_thumnailformatstring, "image_thumnailformatstring:",
					 (init_values ?
					  "<a href=\"%r\"><img src=\"%t\" width=\"%x\" height=\"%y\" border=\"0\"></a>" : NULL));
	init_prop_integer(&config_rc, &main_v->globses.filter_on_selection_mode,
					  "filter_on_selection_mode:", 0, init_values);
	init_prop_arraylist(&config_rc, &main_v->globses.filefilters, "filefilters:", 4, init_values);
	init_prop_arraylist(&config_rc, &main_v->globses.reference_files, "reference_files:", 2, init_values);
	init_prop_limitedstringlist(&config_rc, &main_v->globses.recent_projects, "recent_projects:",
								main_v->props.max_recent_files, init_values);
	init_prop_arraylist(&config_rc, &main_v->globses.encodings, "encodings:", 3, init_values);
	init_prop_arraylist(&config_rc, &main_v->globses.custombflangmime, "custom_bflang_mime:", 2, init_values);
#ifdef WITH_MSG_QUEUE
	init_prop_integer(&config_rc, &main_v->globses.msg_queue_poll_time, "msg_queue_poll_time:", 500,
					  init_values);
#endif
	config_rc = bfplugins_register_globses_config(config_rc);
	return config_rc;
}

static GHashTable *
return_session_configlist(GHashTable * configlist, Tsessionvars * session)
{
	/* this function should *NOT* initialize any values to default values
	   because it is also used on existing sessions that already have a value, and
	   that would wipe out the value of the existing session */
	init_prop_integer(&configlist, &session->enable_syntax_scan, "enable_syntax_scan:", 1, FALSE);
	init_prop_integer(&configlist, &session->wrap_text_default, "wrap_text_default:", 1, FALSE);
	init_prop_integer(&configlist, &session->autoindent, "autoindent:", 1, FALSE);
	init_prop_integer(&configlist, &session->editor_tab_width, "editor_tab_width:", 3, FALSE);
	init_prop_integer(&configlist, &session->editor_indent_wspaces, "editor_indent_wspaces:", 0, FALSE);
	init_prop_integer(&configlist, &session->view_line_numbers, "view_line_numbers:", 1, FALSE);
	init_prop_integer(&configlist, &session->view_cline, "view_cline:", 1, FALSE);
	init_prop_integer(&configlist, &session->view_blocks, "view_blocks:", 1, FALSE);
	init_prop_integer(&configlist, &session->view_blockstack, "view_blockstack:", 1, FALSE);
	init_prop_integer(&configlist, &session->autocomplete, "autocomplete:", 1, FALSE);
	init_prop_integer(&configlist, &session->show_mbhl, "show_mbhl:", 1, FALSE);

	init_prop_integer(&configlist, &session->snr3_type, "snr_type:", 1, FALSE);
	init_prop_integer(&configlist, &session->snr3_replacetype, "snr_replacetype:", 1, FALSE);
	init_prop_integer(&configlist, &session->snr3_scope, "snr_scope:", 1, FALSE);
	init_prop_integer(&configlist, &session->snr3_casesens, "snr_casesens:", 1, FALSE);
	init_prop_integer(&configlist, &session->snr3_escape_chars, "snr_escape_chars:", 1, FALSE);
	init_prop_integer(&configlist, &session->snr3_dotmatchall, "snr_dotmatchall:", 1, FALSE);
	init_prop_integer(&configlist, &session->snr3_recursion_level, "snr_recursion_level:", 5, FALSE);

	init_prop_integer(&configlist, &session->ssearch_regex, "ssearch_regex:", 0, FALSE);
	init_prop_integer(&configlist, &session->ssearch_dotmatchall, "ssearch_dotmatchall:", 0, FALSE);
	init_prop_integer(&configlist, &session->ssearch_unescape, "ssearch_unescape:", 0, FALSE);
	init_prop_integer(&configlist, &session->ssearch_casesens, "ssearch_casesens:", 0, FALSE);
	init_prop_string_with_escape(&configlist, &session->ssearch_text, "ssearch_text:", NULL);

	init_prop_integer(&configlist, &session->display_right_margin, "display_right_margin:", 0, FALSE);
	init_prop_integer(&configlist, &session->show_visible_spacing, "show_visible_spacing:", 0, FALSE);
#ifdef HAVE_LIBENCHANT
	init_prop_integer(&configlist, &session->spell_check_default, "spell_check_default:", 1, FALSE);
	init_prop_integer(&configlist, &session->spell_insert_entities, "spell_insert_entities:", 0, FALSE);
#endif
	init_prop_string(&configlist, &session->default_mime_type, "default_mime_type:", NULL);
	init_prop_string(&configlist, &session->template, "template:", NULL);
	init_prop_string_with_escape(&configlist, &session->webroot, "webroot:", NULL);
	init_prop_string_with_escape(&configlist, &session->documentroot, "documentroot:", NULL);
	init_prop_limitedstringlist(&configlist, &session->searchlist, "searchlist:", 15, FALSE);
	init_prop_limitedstringlist(&configlist, &session->replacelist, "replacelist:", 15, FALSE);
	init_prop_limitedstringlist(&configlist, &session->filegloblist, "filegloblist:", 15, FALSE);
	init_prop_stringlist(&configlist, &session->classlist, "classlist:", FALSE);
	init_prop_stringlist(&configlist, &session->colorlist, "colorlist:", FALSE);
	init_prop_stringlist(&configlist, &session->targetlist, "targetlist:", FALSE);
	init_prop_stringlist(&configlist, &session->urllist, "urllist:", FALSE);
	init_prop_stringlist(&configlist, &session->fontlist, "fontlist:", FALSE);
	init_prop_arraylist(&configlist, &session->bmarks, "bmarks:", 6, FALSE);	/* what is the lenght for a bookmark array? */
	init_prop_limitedstringlist(&configlist, &session->recent_files, "recent_files:",
								main_v->props.max_recent_files, FALSE);
	init_prop_limitedstringlist(&configlist, &session->recent_dirs, "recent_dirs:",
								main_v->props.max_dir_history, FALSE);
	init_prop_string_with_escape(&configlist, &session->opendir, "opendir:", NULL);
	init_prop_string_with_escape(&configlist, &session->savedir, "savedir:", NULL);
	init_prop_string_with_escape(&configlist, &session->sync_local_uri, "sync_local_uri:", NULL);
	init_prop_string_with_escape(&configlist, &session->sync_remote_uri, "sync_remote_uri:", NULL);
	init_prop_string_with_escape(&configlist, &session->encoding, "encoding:", NULL);
	init_prop_integer(&configlist, &session->adv_open_matchname, "adv_open_matchname:", 1, FALSE);
	init_prop_integer(&configlist, &session->adv_open_recursive, "adv_open_recursive:", 0, FALSE);
	init_prop_string(&configlist, &session->last_filefilter, "last_filefilter:", "");
	init_prop_integer(&configlist, &session->leftpanel_active_tab, "leftpanel_active_tab:", 1, FALSE);
	init_prop_integer(&configlist, &session->view_main_toolbar, "view_main_toolbar:", 1, FALSE);
	init_prop_integer(&configlist, &session->view_left_panel, "view_left_panel:", 1, FALSE);
	init_prop_integer(&configlist, &session->view_statusbar, "view_statusbar:", 1, FALSE);
	init_prop_integer(&configlist, &session->filebrowser_show_hidden_files, "fb_show_hidden_f:", 0, FALSE);
	init_prop_integer(&configlist, &session->filebrowser_show_backup_files, "fb_show_backup_f:", 0, FALSE);
	init_prop_integer(&configlist, &session->filebrowser_focus_follow, "fb_focus_follow:", 1, FALSE);
	init_prop_integer(&configlist, &session->filebrowser_viewmode, "fb_viewmode:", 1, FALSE);
	init_prop_integer(&configlist, &session->bookmarks_filename_mode, "bookmarks_filename_mode:", 1, FALSE);
	init_prop_integer(&configlist, &session->bookmarks_show_mode, "bookmarks_show_mode:", 0, FALSE);
	init_prop_integer(&configlist, &session->bmarksearchmode, "bmarksearchmode:", 0, FALSE);
	init_prop_integer(&configlist, &session->outputb_scroll_mode, "outputb_scroll_mode:", 0, FALSE);
	init_prop_integer(&configlist, &session->outputb_show_all_output, "outputb_show_all_output:", 0, FALSE);
	init_prop_string_with_escape(&configlist, &session->convertcolumn_separator, "convertcolumn_separator:",
								 NULL);
	init_prop_integer(&configlist, &session->convertcolumn_horizontally, "convertcolumn_horizontally:", 0,
					  FALSE);
	init_prop_string_with_escape(&configlist, &session->convertcolumn_fillempty, "convertcolumn_fillempty:",
								 NULL);
#ifdef HAVE_LIBENCHANT
	init_prop_string_with_escape(&configlist, &session->spell_lang, "spell_lang:", NULL);
#endif
	configlist = bfplugins_register_session_config(configlist, session);
	return configlist;
}

static void
setup_session_after_parse(Tsessionvars * session)
{
	if (session->editor_tab_width < 1)
		session->editor_tab_width = 3;

	if (session->default_mime_type == NULL)
		session->default_mime_type = g_strdup("text/plain");

	if (session->filegloblist == NULL) {
		session->filegloblist = list_from_arglist(TRUE, "*",
				"*.c",
				"*.cgi",
				"*.cpp",
				"*.css",
				"*.h",
				"*.html",
				"*.htm",
				"*.java",
				"*.js",
				"*.php",
				"*.pl",
				"*.py",
				"*.shtml",
				"*.txt",
				"*.xml" , NULL);
	}
/* TODO: set spell check language to a sensible default

#ifdef HAVE_LIBENCHANT
	if (session->spell_lang==NULL)
		session->spell_lang=
#endif
*/
}

static GHashTable *
return_project_configlist(Tproject * project)
{
	GHashTable *configlist = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, free_config_list_item);
	init_prop_string(&configlist, &project->name, "name:", _("Untitled Project"));
	init_prop_stringlist(&configlist, &project->files, "files:", FALSE);
/*	init_prop_stringlist(&configlist, &project->recentfiles, "recentfiles:", FALSE); / * should be changed to use the session->recent_files */
	configlist = return_session_configlist(configlist, project->session);
	return configlist;
}

gboolean
rcfile_parse_project(Tproject * project, GFile * file)
{
	gboolean retval;
	GHashTable *configlist = return_project_configlist(project);
	retval = parse_config_file(configlist, file);
	free_configlist(configlist);
	setup_session_after_parse(project->session);
	return retval;
}

gboolean
rcfile_save_project(Tproject * project, GFile * file)
{
	gboolean retval;
	GHashTable *configlist = return_project_configlist(project);
	DEBUG_MSG("rcfile_save_project, project %p, name='%s'\n", project, project->name);
	DEBUG_MSG("rcfile_save_project, bmarks=%p, list length=%d\n", project->session->bmarks,
			  g_list_length(project->session->bmarks));
	DEBUG_MSG("rcfile_save_project, length session recent_files=%d\n",
			  g_list_length(project->session->recent_files));
	retval = save_config_file(configlist, file);
	free_configlist(configlist);
	return retval;
}

gboolean
rcfile_save_global_session(void)
{
	gboolean retval;
	GFile *filename;
	GHashTable *configlist;
	filename = user_bfdir(CURSESSION);
	configlist = return_globalsession_configlist(FALSE);
	configlist = return_session_configlist(configlist, main_v->session);
	DEBUG_MSG("rcfile_save_global_session, saving global session to %s\n", g_file_get_parse_name(filename));
	retval = save_config_file(configlist, filename);
	free_configlist(configlist);
	g_object_unref(filename);
	return retval;
}

static void
add_new_encodings(GList *defaults, GList *current)
{
	GHashTable *ht;
	GList *tmplist, *last=NULL;
	ht = g_hash_table_new(g_str_hash, g_str_equal);

	for (tmplist=g_list_first(current);tmplist;tmplist=tmplist->next) {
		gchar **arr = tmplist->data;
		g_hash_table_insert(ht, arr[1], GINT_TO_POINTER(1));
		last = tmplist;
	}
	for (tmplist=g_list_first(defaults);tmplist;tmplist=tmplist->next) {
		gchar **arr = tmplist->data;
		if (g_hash_table_lookup(ht, arr[1])) {
			g_strfreev(arr);
		} else {
			DEBUG_MSG("append encoding %s\n",arr[1]);
			last = g_list_append(last, arr);
			last = g_list_last(last);
		}
	}
	g_hash_table_destroy(ht);
}

/* should be called AFTER the normal properties are loaded, becauses return_session_configlist() uses
 settings from main_v->props */
gboolean
rcfile_parse_global_session(void)
{
	gboolean retval;
	GFile *file;
	GHashTable *configlist = return_globalsession_configlist(TRUE);
	configlist = return_session_configlist(configlist, main_v->session);
	file = user_bfdir(CURSESSION);
	retval = parse_config_file(configlist, file);
	free_configlist(configlist);
	g_object_unref(file);
	setup_session_after_parse(main_v->session);
	if (main_v->globses.filefilters == NULL) {
		/* if the user does not have file filters --> set them to defaults values */
		gchar **arr;
		arr =
			array_from_arglist(_("C programming"), "1",
							   "image/jpeg:text/x-install:text/x-readme:text/x-chdr:text/x-csrc:image/png:text/x-authors:text/x-copying:text/x-makefile",
							   "", NULL);
		main_v->globses.filefilters = g_list_prepend(main_v->globses.filefilters, arr);
		arr = array_from_arglist(_("Web files"), "1", "text/html:text/css:image/png:image/jpeg", "", NULL);
		main_v->globses.filefilters = g_list_prepend(main_v->globses.filefilters, arr);
		/*arr = array_from_arglist(_("Java programming"),"1", "java:image:jsp", NULL);
		   main_v->globses.filefilters = g_list_append(main_v->globses.filefilters, arr); */
		arr = array_from_arglist(_("Images"), "1", "image/png:image/jpeg:image/gif:image/tiff", "", NULL);
		main_v->globses.filefilters = g_list_prepend(main_v->globses.filefilters, arr);
		arr =
			array_from_arglist(_("Hide objectfiles"), "0", "",
							   "*.o", NULL);
		main_v->globses.filefilters = g_list_prepend(main_v->globses.filefilters, arr);
	}
	if (main_v->globses.encodings == NULL || main_v->props.rcfile_from_old_version == 1) {
		GFile *defaultfile =
			return_first_existing_filename(PKGDATADIR "/encodings", "data/encodings", "../data/encodings",
										   NULL);
		if (defaultfile) {
			GList *default_encodings = get_list(defaultfile, NULL, TRUE);
			if (main_v->globses.encodings == NULL) {
				main_v->globses.encodings = default_encodings;
			} else {
				add_new_encodings(default_encodings, main_v->globses.encodings);
				g_list_free(default_encodings);
			}
			g_object_unref(defaultfile);
		}
	}

	return retval;
}
static gint
sort_templates(gconstpointer a,gconstpointer b)
{
	return g_strcmp0(((gchar **)a)[0], ((gchar **)b)[0]);
}


static GList *
load_templates_from_dir(GFile *uri, gboolean create_ifnexist)
{
	GFileEnumerator *en;
	GFile *child;
	GError *gerror=NULL;
	GFileInfo *finfo;
	GList *retlist=NULL;


	en = g_file_enumerate_children(uri,G_FILE_ATTRIBUTE_STANDARD_NAME,G_FILE_QUERY_INFO_NONE, NULL, &gerror);
	if (gerror) {
		if (gerror->code == G_IO_ERROR_NOT_FOUND && create_ifnexist) {
			g_file_make_directory(uri,NULL,NULL);
		} else {
			g_print("failed to load templates: %s\n",gerror->message);
		}
		g_error_free(gerror);
		return NULL;
	}

	finfo = g_file_enumerator_next_file(en,NULL,&gerror);
	while(finfo) {
		gchar *tmp, **arr;
		child = g_file_get_child(uri, g_file_info_get_name(finfo));
    	arr = g_malloc(sizeof(char *)* 3);
    	arr[0] = g_strdup(g_file_info_get_name(finfo));
		arr[1] = g_file_get_path(child);
		arr[2] = NULL;
		while ((tmp = strchr(arr[0], '_')) != NULL) {
    		*tmp = ' ';
    	}
		retlist = g_list_prepend(retlist, arr);
		g_object_unref(child);
		g_object_unref(finfo);
		finfo = g_file_enumerator_next_file(en,NULL,&gerror);
	}
	if (gerror) {
		g_print("failed to list templates: %s\n",gerror->message);
		g_error_free(gerror);
	}
	g_object_unref(en);
	return g_list_sort(retlist, sort_templates);
}

void
load_templates(void)
{
	GList *t1, *t2;
	GFile *uri = user_bfdir("templates/");
	t1 = load_templates_from_dir(uri, TRUE);
	g_object_unref(uri);
	uri = g_file_new_for_path(PKGDATADIR"/templates/");
	t2 = load_templates_from_dir(uri, FALSE);
	g_object_unref(uri);
	main_v->templates = g_list_concat(t1, t2);
}
