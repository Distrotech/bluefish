/* Bluefish HTML Editor
 * rcfile.c - loading and parsing of the configfiles
 *
 * Copyright (C) 2000-2009 Olivier Sessink
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
#include "document.h"    /* DOCUMENT_BACKUP_ABORT_ASK */
#include "plugins.h"
#include "stringlist.h"


typedef struct {
	void *pointer; /* where should the value be stored ?*/
	unsigned char type; /* a=arraylist, l=stringlist, s=string, e=string with escape, i=integer, m=limiTed stringlist */
	gchar *identifier; /* the string that should be in the config file for this entry */
	gint len; /* used for arrays and limitedstringlists, the length the list should have (only during save)
						, or the number of items the array should have (during load) */
} Tconfig_list_item;

static GHashTable *main_configlist=NULL;
#ifdef USE_CUSTOM_MENU
static GHashTable *custom_menu_configlist=NULL;
#endif
static void free_configlist(GHashTable *configlist) {
	g_hash_table_destroy(configlist);
/*	GList *tmplist = g_list_first(configlist);
	while(tmplist) {
		Tconfig_list_item *cli = tmplist->data;
		g_free(cli);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(configlist);*/
}

/*this should add 1 empty entry to the configuration list */
GHashTable *make_config_list_item(GHashTable * config_list, void *pointer_to_var, unsigned char type_of_var, gchar * name_of_var, gint len)
{
	Tconfig_list_item *config_list_item;
	if (!pointer_to_var) {
		DEBUG_MSG("make_config_list_item, pointer to var = NULL !\n");
		return config_list;
	}
	config_list_item = g_malloc(sizeof(Tconfig_list_item));
	config_list_item->pointer = pointer_to_var;
	config_list_item->type = type_of_var;
	config_list_item->identifier = name_of_var;
	config_list_item->len = len;
	g_hash_table_replace(config_list, name_of_var, config_list_item);
	return config_list; 
}

static void init_prop_integer(GHashTable ** config_list, void *pointer_to_var, gchar * name_of_var, gint default_value, gboolean set_default)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 'i', name_of_var, 0);
	if (set_default) *(gint *)pointer_to_var = default_value;
}

static void init_prop_string(GHashTable ** config_list, void *pointer_to_var, gchar * name_of_var, const gchar * default_value)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 's', name_of_var, 0);
	if (*(gchar **) pointer_to_var == NULL) {
		*(gchar **) pointer_to_var = g_strdup(default_value);
	}
	DEBUG_MSG("init_prop_string, name_of_var=%s, default_value=%s, current value=%s\n", name_of_var, default_value, *(gchar **) pointer_to_var);
}

static void init_prop_string_with_escape(GHashTable ** config_list, void *pointer_to_var, gchar * name_of_var, gchar * default_value)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 'e', name_of_var, 0);
	if (*(gchar **) pointer_to_var == NULL && default_value) {
		*(gchar **) pointer_to_var = unescape_string(default_value, FALSE);
	}
	DEBUG_MSG("init_prop_string, name_of_var=%s, default_value=%s\n", name_of_var, default_value);
}

static void init_prop_stringlist(GHashTable ** config_list, void *pointer_to_var, gchar * name_of_var, gboolean setNULL)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 'l', name_of_var, 0);
	if (setNULL) {
	 	pointer_to_var = NULL;
	}
}

static void init_prop_arraylist(GHashTable ** config_list, void *pointer_to_var, gchar * name_of_var, gint len, gboolean setNULL)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 'a', name_of_var, len);
	if (setNULL) {
	 	pointer_to_var = NULL;
	}
}

/* limited lists should have the most recent on top, the last entries will be cut if there are too many entries */
static void init_prop_limitedstringlist(GHashTable ** config_list, void *pointer_to_var, gchar * name_of_var, gint len, gboolean setNULL)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 'm', name_of_var, len);
	if (setNULL) {
	 	pointer_to_var = NULL;
	}
}

static gint save_config_file(GHashTable * config_list, GFile * file)
{
	gchar *tmpstring = NULL, *tmpstring2;
	GList *rclist, *tmplist2;
	GHashTableIter iter;
	gpointer key,value;
	gboolean retval;

	DEBUG_MSG("save_config_file, started\n");

	rclist = NULL;

/* We must first make a list with 1 string per item. */
	g_hash_table_iter_init(&iter,config_list);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		Tconfig_list_item *tmpitem = value;
		DEBUG_MSG("save_config_file, identifier=%s datatype %c\n", tmpitem->identifier,tmpitem->type);
		switch (tmpitem->type) {
		case 'i':
			DEBUG_MSG("save_config_file, converting \"%p\" to integer\n", tmpitem);
			DEBUG_MSG("save_config_file, converting \"%s %i\"\n", tmpitem->identifier, *(int *) (void *) tmpitem->pointer);

			tmpstring = g_strdup_printf("%s %i", tmpitem->identifier, *(int *) (void *) tmpitem->pointer);

			DEBUG_MSG("save_config_file, adding %s\n", tmpstring);

			rclist = g_list_append(rclist, tmpstring);
			break;
		case 's':
			DEBUG_MSG("save_config_file, converting \"%p\" to string\n", tmpitem);
			DEBUG_MSG("save_config_file, converting \"%s %s\"\n", tmpitem->identifier, (gchar *) * (void **) tmpitem->pointer);
			if (*(void **) tmpitem->pointer) {
				tmpstring = g_strdup_printf("%s %s", tmpitem->identifier, (gchar *) * (void **) tmpitem->pointer);

				DEBUG_MSG("save_config_file, adding %s\n", tmpstring);

				rclist = g_list_append(rclist, tmpstring);
			}
			break;
		case 'e':
			DEBUG_MSG("save_config_file, converting \"%p\"\n", tmpitem);
			DEBUG_MSG("save_config_file, converting \"%s %s\"\n", tmpitem->identifier, (gchar *) * (void **) tmpitem->pointer);
			if (*(void **) tmpitem->pointer) {
				tmpstring2 = escape_string((gchar*)*(void**)tmpitem->pointer, FALSE);
				tmpstring = g_strdup_printf("%s %s", tmpitem->identifier, tmpstring2);

				DEBUG_MSG("save_config_file, adding %s\n", tmpstring);

				rclist = g_list_append(rclist, tmpstring);
				g_free(tmpstring2);
			}
			break;
		case 'l':
		case 'm': {
			gint max = -1; /* by setting it to -1, it will never become zero if we substract 1 every round */
			DEBUG_MSG("save_config_file, type %c, tmpitem(%p), &tmpitem=%p\n", tmpitem->type, tmpitem, &tmpitem);
			if (tmpitem->type == 'm') max = tmpitem->len;
			tmplist2 = g_list_last((GList *) * (void **) tmpitem->pointer);
			while (tmplist2 != NULL && max != 0) {
				tmpstring2 = escape_string((char *) tmplist2->data, FALSE);
				tmpstring = g_strconcat(tmpitem->identifier, " ", tmpstring2, NULL);
				g_free(tmpstring2);
				DEBUG_MSG("save_config_file, tmpstring(%p)=%s, orig was %s\n", tmpstring, tmpstring,(gchar *)tmplist2->data);
				rclist = g_list_append(rclist, tmpstring);
				tmplist2 = g_list_previous(tmplist2);
				max--;
#ifdef DEBUG
				if (max ==0 && tmplist2 != NULL) g_print("save_config_file, limit reached!, next item would have been %s\n",(gchar *)tmplist2->data);
#endif
			}
			} break;
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
	DEBUG_MSG("save_config_file, will save list with len %d to file\n",g_list_length(rclist));
	retval = put_stringlist(file, rclist);
	free_stringlist(rclist);
	return retval;
}

static gboolean parse_config_file(GHashTable * config_list, GFile * file)
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
	rclist = get_list(file, rclist,FALSE);

	if (rclist == NULL) {
		DEBUG_MSG("no rclist, returning!\n");
		return retval;
	}

	/* empty all variables that have type GList ('l') */
	g_hash_table_iter_init(&iter, config_list);
	while (g_hash_table_iter_next (&iter, &key, &value)) { 
		tmpitem = (Tconfig_list_item *) value;
		DEBUG_MSG("parse_config_file, type=%c, identifier=%s\n", tmpitem->type, tmpitem->identifier);
		if (tmpitem->type == 'l' || tmpitem->type == 'a') {
			DEBUG_MSG("parse_config_file, freeing list before filling it\n");
			free_stringlist((GList *) * (void **) tmpitem->pointer);
			*(void **) tmpitem->pointer = (GList *)NULL;
		}
		DEBUG_MSG("parse_config_file, type=%c, identifier=%s\n", tmpitem->type, tmpitem->identifier);
	}
	DEBUG_MSG("parse_config_file, all the type 'l' and 'a' have been emptied\n");
	DEBUG_MSG("parse_config_file, length rclist=%d\n", g_list_length(rclist));
/* And now for parsing every line in the config file, first check if there is a valid identifier at the start. */
	for (tmplist= g_list_first(rclist);tmplist;tmplist=g_list_next(tmplist)) {
		tmpstring = (gchar *) tmplist->data;
		if (tmpstring != NULL && tmpstring[0] != '\0') {
			gchar *key, *tmp;
			g_strchug(tmpstring);
			DEBUG_MSG("parse_config_file, tmpstring=%s\n", tmpstring);
			tmp = strchr(tmpstring,':');
			if (!tmp)
				break;
			key = g_strndup(tmpstring, tmp-tmpstring+1);
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
					*(void **) tmpitem->pointer = (char *) realloc((char *) *(void **) tmpitem->pointer, strlen(tmpstring) + 1);
					strcpy((char *) *(void **) tmpitem->pointer, tmpstring);
					break;
				case 'e':
					tmpstring2 = unescape_string(tmpstring, FALSE);
					*(void **) tmpitem->pointer = (char *) realloc((char *) *(void **) tmpitem->pointer, strlen(tmpstring2) + 1);
					strcpy((char *) *(void **) tmpitem->pointer, tmpstring2);
					g_free(tmpstring2);
					break;
				case 'l':
				case 'm':
					/*tmpstring2 = g_strdup(tmpstring);*/
					tmpstring2 = unescape_string(tmpstring,FALSE);
					* (void **) tmpitem->pointer = g_list_prepend((GList *) * (void **) tmpitem->pointer, tmpstring2);
					DEBUG_MSG("parse_config_file, *(void **)tmpitem->pointer=%p\n", *(void **) tmpitem->pointer);
					break;
				case 'a':
					tmparray = string_to_array(tmpstring);
					if (tmpitem->len <= 0 || tmpitem->len == count_array(tmparray)) {
						* (void **) tmpitem->pointer = g_list_prepend((GList *) * (void **) tmpitem->pointer, tmparray);
					} else {
						DEBUG_MSG("parse_config_file, not storing array, count_array() != tmpitem->len\n");
						g_strfreev(tmparray);
					}
					DEBUG_MSG("parse_config_file, *(void **)tmpitem->pointer=%p\n", *(void **) tmpitem->pointer);
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

static GHashTable *props_init_main(GHashTable * config_rc)
{
	init_prop_integer   (&config_rc, &main_v->props.do_periodic_check, "do_periodic_check:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.view_line_numbers, "view_line_numbers:", 1, TRUE);
	init_prop_string    (&config_rc, &main_v->props.editor_font_string, "editor_font_string:", "courier 11");
	init_prop_integer   (&config_rc, &main_v->props.editor_tab_width, "editor_tab_width:", 3, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.editor_smart_cursor, "editor_smart_cursor:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.editor_indent_wspaces, "editor_indent_wspaces:", 0, TRUE);
	init_prop_string    (&config_rc, &main_v->props.tab_font_string, "tab_font_string:", "");
	init_prop_arraylist (&config_rc, &main_v->props.external_command, "external_command:", 3, TRUE);
	init_prop_arraylist (&config_rc, &main_v->props.external_filter, "external_filter:", 2, TRUE);
	init_prop_arraylist (&config_rc, &main_v->props.external_outputbox, "external_outputbox:", 7, TRUE);

	init_prop_integer   (&config_rc, &main_v->props.defaulthighlight, "defaulthighlight:", 1, TRUE);
	/* old type filetypes have a different count, they are converted below */
	init_prop_integer   (&config_rc, &main_v->props.transient_htdialogs, "transient_htdialogs:", 1, TRUE);

	init_prop_integer   (&config_rc, &main_v->props.leave_to_window_manager, "leave_to_window_manager:", 0, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.restore_dimensions, "restore_dimensions:", 1, TRUE);

	init_prop_integer   (&config_rc, &main_v->props.left_panel_left, "left_panel_left:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.max_recent_files, "max_recent_files:", 15, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.max_dir_history, "max_dir_history:", 10, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.backup_file,"backup_file:",1, TRUE);
/*	init_prop_string    (&config_rc, &main_v->props.backup_suffix,"backup_suffix:","~");
	init_prop_string    (&config_rc, &main_v->props.backup_prefix,"backup_prefix:","");*/
	init_prop_integer   (&config_rc, &main_v->props.backup_abort_action,"backup_abort_action:",DOCUMENT_BACKUP_ABORT_ASK, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.backup_cleanuponclose,"backup_cleanuponclose:",0, TRUE);
	init_prop_string    (&config_rc, &main_v->props.image_thumbnailstring, "image_thumbnailstring:", "_thumbnail");
	init_prop_string    (&config_rc, &main_v->props.image_thumbnailtype, "image_thumbnailtype:", "png");
	init_prop_integer   (&config_rc, &main_v->props.modified_check_type,"modified_check_type:",1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.num_undo_levels,"num_undo_levels:",100, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.clear_undo_on_save,"clear_undo_on_save:",0, TRUE);
	init_prop_string    (&config_rc, &main_v->props.newfile_default_encoding,"newfile_default_encoding:","UTF-8");
	init_prop_integer   (&config_rc, &main_v->props.auto_set_encoding_meta,"auto_set_encoding_meta:",1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.auto_update_meta_author,"auto_update_meta_author:",1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.auto_update_meta_date,"auto_update_meta_date:",1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.auto_update_meta_generator,"auto_update_meta_generator:",1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.encoding_search_Nbytes, "encoding_search_Nbytes:", 500, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.document_tabposition,"document_tabposition:",(gint)GTK_POS_BOTTOM, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.leftpanel_tabposition,"leftpanel_tabposition:",(gint)GTK_POS_BOTTOM, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.switch_tabs_by_altx,"switch_tabs_by_altx:",1, TRUE);
/*	init_prop_string    (&config_rc, &main_v->props.default_basedir,"default_basedir:",g_get_home_dir());*/
	init_prop_string    (&config_rc, &main_v->props.project_suffix,"project_suffix:",".bfproject");
	/* not yet in use */
	init_prop_string(&config_rc, &main_v->props.image_editor_cline, "image_editor_command:", "gimp-remote -n \"%s\"&");
	init_prop_integer(&config_rc, &main_v->props.allow_dep, "allow_the_use_of_font:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.format_by_context, "format_by_context:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.xhtml, "use_xhtml:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.insert_close_tag, "insert_close_tag:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.close_tag_newline, "close_tag_newline:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.allow_ruby, "allow_the_use_of_ruby:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.force_dtd, "force_an_dtd:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.dtd_url, "url_in_dtd:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.xml_start, "xml_starting_line:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.lowercase_tags, "lowercase_tags:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.word_wrap, "word_wrap:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.autoindent, "autoindent:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.drop_at_drop_pos, "drop_at_drop_position:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.link_management, "link_management:", 1, TRUE);
#ifdef WITH_MSG_QUEUE
	init_prop_integer (&config_rc, &main_v->props.open_in_running_bluefish,"open_in_running_bluefish:",1, TRUE);
#endif
#ifndef NOSPLASH
	init_prop_integer(&config_rc, &main_v->props.show_splash_screen, "show_splash_screen:", 1, TRUE);
#endif
	init_prop_arraylist (&config_rc, &main_v->props.plugin_config, "plugin_config:", 3, TRUE);
	init_prop_string    (&config_rc, &main_v->props.editor_fg,"editor_fg:","#000000");
	init_prop_string    (&config_rc, &main_v->props.editor_bg,"editor_bg:","#FFFFFF");
	init_prop_arraylist (&config_rc, &main_v->props.textstyles, "textstyles2:", 5, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.view_mbhl, "view_mbhl:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.view_cline, "view_cline:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.view_blocks, "view_blocks:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.block_folding_mode, "block_folding_mode:", 1, TRUE);
	init_prop_arraylist (&config_rc, &main_v->props.highlight_styles, "highlight_styles:", 3, TRUE);
	init_prop_arraylist (&config_rc, &main_v->props.bflang_options, "bflang_options:", 3, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.load_reference, "load_reference:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.show_autocomp_reference, "show_autocomp_reference:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.show_tooltip_reference, "show_tooltip_reference:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.delay_full_scan, "delay_full_scan:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.delay_scan_time, "delay_scan_time:", 900, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.autocomplete, "autocomplete:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.autocomp_popup_mode, "autocomp_popup_mode:", 2, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.reduced_scan_triggers, "reduced_scan_triggers:", 1, TRUE);
	return config_rc;
}

void rcfile_parse_main(void)  {
	GFile *file;

	DEBUG_MSG("rcfile_parse_main, started\n");

	/* set the props struct completely empty */
	memset(&main_v->props, 0, sizeof(Tproperties));

	/*Make the config_rc list ready for filling with data and set default values */
	main_configlist = props_init_main(g_hash_table_new_full(g_str_hash,g_str_equal,NULL, g_free));

	file = user_bfdir("rcfile_v2");
	if (!parse_config_file(main_configlist, file)) {
		/* should we initialize some things ?? */
	}
	g_object_unref(file);
	if (main_v->props.encoding_search_Nbytes< 1000) main_v->props.encoding_search_Nbytes = 2048;
	/* do some default configuration for the lists */

	if (main_v->props.external_outputbox==NULL) {
		/* if the user does not have outputbox settings --> set them to defaults values */
		main_v->props.external_outputbox = g_list_append(main_v->props.external_outputbox,array_from_arglist(_("make"),"([a-zA-Z0-9/_.-]+):([0-9]+):(.*)","1","2","3","cd %c && make|","1",NULL));
		main_v->props.external_outputbox = g_list_append(main_v->props.external_outputbox,array_from_arglist(_("weblint HTML checker"),"([a-zA-Z0-9/_.-]+) \\(([0-9:]+)\\) (.*)","1","2","3","weblint '%s'|","0",NULL));
		main_v->props.external_outputbox = g_list_append(main_v->props.external_outputbox,array_from_arglist(_("tidy HTML validator"),"line ([0-9]+) column [0-9]+ - (.*)","-1","1","2","|tidy -qe '%s'|","0",NULL));
		main_v->props.external_outputbox = g_list_append(main_v->props.external_outputbox,array_from_arglist(_("javac"),"([a-zA-Z0-9/_.-]+):([0-9]+):(.*)","1","2","3","javac '%s'|","0",NULL));
		main_v->props.external_outputbox = g_list_append(main_v->props.external_outputbox,array_from_arglist(_("xmllint XML checker"),"([a-zA-Z0-9/_.-]+)\\:([0-9]+)\\: (.*)","1","2","3","xmllint --noout --valid '%s'|","0",NULL));
		main_v->props.external_outputbox = g_list_append(main_v->props.external_outputbox,array_from_arglist(_("php"),"(.*) in (/[a-zA-Z0-9/_.-]+) on line ([0-9]+)","1","2","3","php '%s'|","0",NULL));
	}
	if (main_v->props.external_filter==NULL) {
		main_v->props.external_filter = g_list_append(main_v->props.external_filter,array_from_arglist(_("Sort"),"|sort|",NULL));
		main_v->props.external_filter = g_list_append(main_v->props.external_filter,array_from_arglist(_("Reverse lines"),"|tac|",NULL));
		main_v->props.external_filter = g_list_append(main_v->props.external_filter,array_from_arglist(_("Dos2unix"),"|dos2unix|",NULL));
		main_v->props.external_filter = g_list_append(main_v->props.external_filter,array_from_arglist(_("Tidy HTML"),"|tidy|",NULL));
		main_v->props.external_filter = g_list_append(main_v->props.external_filter,array_from_arglist(_("Strip empty lines"),"|egrep -v '^[ \t]*$'|",NULL));
		main_v->props.external_filter = g_list_append(main_v->props.external_filter,array_from_arglist(_("Render HTML to text"),"lynx -force_html -dump %i |",NULL));
	}
	if (main_v->props.external_command==NULL) {
		main_v->props.external_command = g_list_append(main_v->props.external_command, array_from_arglist(_("Mozilla"), "mozilla -remote 'openURL(%i)' || mozilla '%i'&","0",NULL));
		main_v->props.external_command = g_list_append(main_v->props.external_command, array_from_arglist(_("Firefox"), "firefox -remote 'openURL(%i)' || firefox '%i'&","1",NULL));
		main_v->props.external_command = g_list_append(main_v->props.external_command, array_from_arglist(_("Opera"), "opera -remote 'openURL(%I)' || opera '%I'&","0",NULL));
		main_v->props.external_command = g_list_append(main_v->props.external_command, array_from_arglist(_("Konqueror"), "konqueror '%I'&","0",NULL));
		main_v->props.external_command = g_list_append(main_v->props.external_command, array_from_arglist(_("Galeon"), "galeon -n  '%I'&","0",NULL));
		main_v->props.external_command = g_list_append(main_v->props.external_command, array_from_arglist(_("Epiphany"), "epiphany-browser -n '%I'&","0",NULL));
		main_v->props.external_command = g_list_append(main_v->props.external_command, array_from_arglist(_("Kazehakase"), "kazehakase '%I'&","0",NULL));
		main_v->props.external_command = g_list_append(main_v->props.external_command, array_from_arglist(_("Links2 (graphics)"), "links2 -g '%I'&","0",NULL));
	}
}

gint rcfile_save_main(void) {
	gint ret;
	GFile *filename = user_bfdir("rcfile_v2");
	ret = save_config_file(main_configlist, filename);
	g_object_unref(filename);
	return ret;
}

#define DIR_MODE (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)	/* same as 0755 */
void rcfile_check_directory(void) {
	gchar *rcdir = g_strconcat(g_get_home_dir(), "/."PACKAGE, NULL);
	if (!g_file_test(rcdir, G_FILE_TEST_IS_DIR)) {
		mkdir(rcdir, DIR_MODE);
	}
	g_free(rcdir);
}

void rcfile_save_configfile_menu_cb(gpointer callback_data,guint action,GtkWidget *widget) {
	switch (action) {
	case 0:
		rcfile_save_main();
	break;
#ifdef USE_CUSTOM_MENU
	case 2:
		rcfile_save_custom_menu();
	break;
#endif
	case 3:
		{
			gchar *shortcutfilename = g_strconcat(g_get_home_dir(), "/."PACKAGE"/menudump_2", NULL);
			gtk_accel_map_save(shortcutfilename);
			g_free(shortcutfilename);
		}
	break;
	default:
		g_print("rcfile_save_configfile_menu_cb, unknown action %d\n", action);
#ifdef DEBUG
		exit(10);
#endif
	break;
	}
}

static GHashTable *return_globalsession_configlist(gboolean init_values) {
	GHashTable *config_rc = g_hash_table_new_full(g_str_hash,g_str_equal,NULL, g_free);
	init_prop_integer   (&config_rc, &main_v->globses.main_window_h, "main_window_height:", 400, init_values);
	init_prop_integer   (&config_rc, &main_v->globses.main_window_w, "main_window_width:", 600, init_values); /* negative width means maximized */
	init_prop_integer   (&config_rc, &main_v->globses.two_pane_filebrowser_height, "two_pane_filebrowser_height:", 250, init_values);
	init_prop_integer   (&config_rc, &main_v->globses.left_panel_width, "left_panel_width:", 150, init_values);
	/*init_prop_integer   (&config_rc, &main_v->globses.lasttime_filetypes, "lasttime_filetypes:", 0, init_values);
	init_prop_integer   (&config_rc, &main_v->globses.lasttime_encodings, "lasttime_encodings:", 0, init_values);*/
	init_prop_integer   (&config_rc, &main_v->globses.snr_select_match, "snr_select_match:", 1, init_values);
	init_prop_integer   (&config_rc, &main_v->globses.bookmarks_default_store,"bookmarks_default_store:",1, init_values);
	init_prop_integer   (&config_rc, &main_v->globses.image_thumbnail_refresh_quality,"image_thumbnail_refresh_quality:",1, init_values);
	init_prop_integer   (&config_rc, &main_v->globses.image_thumbnailsizing_type,"image_thumbnailsizing_type:",0, init_values);
	init_prop_integer   (&config_rc, &main_v->globses.image_thumbnailsizing_val1,"image_thumbnailsizing_val1:",100, init_values);
	init_prop_integer   (&config_rc, &main_v->globses.image_thumbnailsizing_val2,"image_thumbnailsizing_val2:",100, init_values);
	init_prop_string    (&config_rc, &main_v->globses.image_thumnailformatstring,"image_thumnailformatstring:",(init_values ? "<a href=\"%r\"><img src=\"%t\" width=\"%x\" height=\"%y\" border=\"0\"></a>" : NULL));
	init_prop_arraylist (&config_rc, &main_v->globses.filefilters, "filefilters:", 4, init_values);
	init_prop_arraylist (&config_rc, &main_v->globses.reference_files, "reference_files:", 2, init_values);
	init_prop_limitedstringlist(&config_rc, &main_v->globses.recent_projects, "recent_projects:", main_v->props.max_recent_files, init_values);
	init_prop_arraylist (&config_rc, &main_v->globses.encodings, "encodings:", 3, FALSE);
	init_prop_integer   (&config_rc, &main_v->globses.msg_queue_poll_time,"msg_queue_poll_time:",500, init_values);
	config_rc = bfplugins_register_globses_config(config_rc);
	return config_rc;
}

static GHashTable *return_session_configlist(GHashTable *configlist, Tsessionvars *session) {
	/* this function should *NOT* initialize any values to default values
	because it is also used on existing sessions that already have a value, and
	that would wipe out the value of the existing session */
	init_prop_string_with_escape(&configlist, &session->webroot, "webroot:", NULL);
	init_prop_string_with_escape(&configlist, &session->documentroot, "documentroot:", NULL);
	init_prop_limitedstringlist(&configlist, &session->searchlist, "searchlist:", 10, FALSE);
	init_prop_limitedstringlist(&configlist, &session->replacelist, "replacelist:", 10, FALSE);
	init_prop_stringlist(&configlist, &session->classlist, "classlist:", FALSE);
	init_prop_stringlist(&configlist, &session->colorlist, "colorlist:", FALSE);
	init_prop_stringlist(&configlist, &session->targetlist, "targetlist:", FALSE);
	init_prop_stringlist(&configlist, &session->urllist, "urllist:", FALSE);
	init_prop_stringlist(&configlist, &session->fontlist, "fontlist:", FALSE);
	init_prop_arraylist (&configlist, &session->bmarks, "bmarks:", 6, FALSE); /* what is the lenght for a bookmark array? */
	init_prop_limitedstringlist(&configlist, &session->recent_files, "recent_files:", main_v->props.max_recent_files, FALSE);
	init_prop_limitedstringlist(&configlist, &session->recent_dirs, "recent_dirs:", main_v->props.max_dir_history, FALSE);
	init_prop_string_with_escape(&configlist, &session->opendir, "opendir:", NULL);
	init_prop_string_with_escape(&configlist, &session->savedir, "savedir:", NULL);
	init_prop_string_with_escape(&configlist, &session->encoding, "encoding:", NULL);
	init_prop_integer   (&configlist, &session->adv_open_matchname, "adv_open_matchname:", 1, FALSE);
	init_prop_integer   (&configlist, &session->adv_open_recursive, "adv_open_recursive:", 0, FALSE);
	init_prop_string    (&configlist, &session->last_filefilter, "last_filefilter:", "");
	init_prop_integer   (&configlist, &session->view_main_toolbar, "view_main_toolbar:", 1, FALSE);
	init_prop_integer   (&configlist, &session->view_left_panel, "view_left_panel:", 1, FALSE);
	init_prop_integer   (&configlist, &session->view_statusbar, "view_statusbar:", 1, FALSE);
	init_prop_integer   (&configlist, &session->filebrowser_show_hidden_files, "fb_show_hidden_f:", 0, FALSE);
	init_prop_integer   (&configlist, &session->filebrowser_show_backup_files, "fb_show_backup_f:", 0, FALSE);
	init_prop_integer   (&configlist, &session->filebrowser_focus_follow, "fb_focus_follow:", 1, FALSE);
	init_prop_integer   (&configlist, &session->filebrowser_viewmode, "fb_viewmode:", 2, FALSE);
	init_prop_integer   (&configlist, &session->bookmarks_filename_mode,"bookmarks_filename_mode:",1, FALSE);
	init_prop_integer   (&configlist, &session->outputb_scroll_mode,"outputb_scroll_mode:",0, FALSE);
	init_prop_integer   (&configlist, &session->outputb_show_all_output,"outputb_show_all_output:",0, FALSE);
#ifdef HAVE_LIBASPELL
	init_prop_string(&configlist, &session->spell_default_lang, "spell_default_lang:", "en");
#endif /* HAVE_LIBASPELL */
	configlist = bfplugins_register_session_config(configlist,session);
	return configlist;
}

static GHashTable *return_project_configlist(Tproject *project) {
	GHashTable *configlist = g_hash_table_new_full(g_str_hash,g_str_equal,NULL, g_free);
	init_prop_string(&configlist, &project->name,"name:",_("Untitled Project"));
	init_prop_stringlist(&configlist, &project->files, "files:", FALSE);
	init_prop_string(&configlist, &project->template,"template:","");
/*	init_prop_stringlist(&configlist, &project->recentfiles, "recentfiles:", FALSE); / * should be changed to use the session->recent_files */
	init_prop_integer (&configlist, &project->word_wrap,"word_wrap:",1,FALSE);
	configlist = return_session_configlist(configlist, project->session);
	return configlist;
}

gboolean rcfile_parse_project(Tproject *project, GFile *file) {
	gboolean retval;
	GHashTable *configlist = return_project_configlist(project);
	retval = parse_config_file(configlist, file);
	free_configlist(configlist);
	return retval;
}

gboolean rcfile_save_project(Tproject *project, GFile *file) {
	gboolean retval;
	GHashTable *configlist = return_project_configlist(project);
	DEBUG_MSG("rcfile_save_project, project %p, name='%s'\n",project, project->name);
	DEBUG_MSG("rcfile_save_project, bmarks=%p, list length=%d\n",project->session->bmarks, g_list_length(project->session->bmarks));
	DEBUG_MSG("rcfile_save_project, length session recent_files=%d\n",g_list_length(project->session->recent_files));
	retval = save_config_file(configlist, file);
	free_configlist(configlist);
	return retval;
}

gboolean rcfile_save_global_session(void) {
	gboolean retval;
	GFile *filename;
	GHashTable *configlist;
	filename = user_bfdir("session");
	configlist = return_globalsession_configlist(FALSE);
	configlist = return_session_configlist(configlist, main_v->session);
	DEBUG_MSG("rcfile_save_global_session, saving global session to %s\n",filename);
	DEBUG_MSG("rcfile_save_global_session, length session recent_files=%d\n",g_list_length(main_v->session->recent_files));
	DEBUG_MSG("rcfile_save_global_session, length session recent_projects=%d\n",g_list_length(main_v->globses.recent_projects));
	retval = save_config_file(configlist, filename);
	free_configlist(configlist);
	g_object_unref(filename);
	return TRUE;
}
/* should be called AFTER the normal properties are loaded, becauses return_session_configlist() uses
 settings from main_v->props */
gboolean rcfile_parse_global_session(void) {
	gboolean retval;
	GFile *file;
	GHashTable *configlist = return_globalsession_configlist(TRUE);
	configlist = return_session_configlist(configlist, main_v->session);
	file = user_bfdir("session");
	retval = parse_config_file(configlist, file);
	free_configlist(configlist);
	g_object_unref(file);

	if (main_v->globses.filefilters == NULL) {
		/* if the user does not have file filters --> set them to defaults values */
		gchar **arr;
		arr = array_from_arglist(_("C programming"),"1","image/jpeg:text/x-install:text/x-readme:text/x-chdr:text/x-csrc:image/png:text/x-authors:text/x-copying:text/x-makefile", "", NULL);
		main_v->globses.filefilters = g_list_prepend(main_v->globses.filefilters, arr);
		arr = array_from_arglist(_("Web files"),"1", "text/html:image/png:image/jpeg", "", NULL);
		main_v->globses.filefilters = g_list_prepend(main_v->globses.filefilters, arr);
		/*arr = array_from_arglist(_("Java programming"),"1", "java:image:jsp", NULL);
		main_v->globses.filefilters = g_list_append(main_v->globses.filefilters, arr);*/
		arr = array_from_arglist(_("Images"),"1", "image/png:image/jpeg:image/gif:image/tiff", "", NULL);
		main_v->globses.filefilters = g_list_append(main_v->globses.filefilters, arr);
		arr = array_from_arglist(_("Hide objectfiles"),"0", "application/octet-stream:application/x-object", "", NULL);
		main_v->globses.filefilters = g_list_append(main_v->globses.filefilters, arr);
	}
	if (main_v->globses.encodings == NULL) {
		GFile *defaultfile = return_first_existing_filename(PKGDATADIR"/encodings","data/encodings","../data/encodings",NULL);
		if (defaultfile) { 
			main_v->globses.encodings = get_list(defaultfile,NULL,TRUE);
			g_object_unref(defaultfile);
		}
	}

	return retval;
}
