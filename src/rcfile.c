/* Bluefish HTML Editor
 * rcfile.c - loading and parsing of the configfiles
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
/*#define DEBUG*/

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
#include "fref.h"
#include "stringlist.h"
#include "highlight.h" /* hl_reset_to_default()*/
#include "document.h" /* DOCUMENT_BACKUP_ABORT_ASK */

typedef struct {
	void *pointer; /* where should the value be stored ?*/
	unsigned char type; /* a=arraylist, l=stringlist, s=string, e=string with escape, i=integer */
	gchar *identifier; /* the string that should be in the config file for this entry */
	gint len; /* only used for arrays, the length the array should have */
} Tconfig_list_item;

static GList *main_configlist=NULL;
static GList *highlighting_configlist=NULL;
static GList *custom_menu_configlist=NULL;

static void free_configlist(GList *configlist) {
	GList *tmplist = g_list_first(configlist);
	while(tmplist) {
		Tconfig_list_item *cli = tmplist->data;
		g_free(cli);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(configlist);
}

/*this should add 1 empty entry to the configuration list */
GList *make_config_list_item(GList * config_list, void *pointer_to_var, unsigned char type_of_var, gchar * name_of_var, gint len)
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
	return (GList *) g_list_append(config_list, config_list_item);
}

static void init_prop_integer(GList ** config_list, void *pointer_to_var, gchar * name_of_var, gint default_value, gboolean set_default)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 'i', name_of_var, 0);
	if (set_default) *(gint *)pointer_to_var = default_value;
}

static void init_prop_string(GList ** config_list, void *pointer_to_var, gchar * name_of_var, const gchar * default_value)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 's', name_of_var, 0);
	if (*(gchar **) pointer_to_var == NULL) {
		*(gchar **) pointer_to_var = g_strdup(default_value);
	}
	DEBUG_MSG("init_prop_string, name_of_var=%s, default_value=%s, current value=%s\n", name_of_var, default_value, *(gchar **) pointer_to_var);
}

static void init_prop_string_with_escape(GList ** config_list, void *pointer_to_var, gchar * name_of_var, gchar * default_value)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 'e', name_of_var, 0);
	*(gchar **) pointer_to_var = unescape_string(default_value, FALSE);
	DEBUG_MSG("init_prop_string, name_of_var=%s, default_value=%s\n", name_of_var, default_value);
}

static void init_prop_stringlist(GList ** config_list, void *pointer_to_var, gchar * name_of_var, gboolean setNULL)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 'l', name_of_var, 0);
	if (setNULL) {
	 	pointer_to_var = NULL;
	}
}


static void init_prop_arraylist(GList ** config_list, void *pointer_to_var, gchar * name_of_var, gint len, gboolean setNULL)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 'a', name_of_var, len);
	if (setNULL) {
	 	pointer_to_var = NULL;
	}
}

static gint save_config_file(GList * config_list, gchar * filename)
{
	gchar *tmpstring = NULL, *tmpstring2;
	GList *rclist, *tmplist, *tmplist2;
	Tconfig_list_item *tmpitem;

	DEBUG_MSG("save_config_file, started\n");

	rclist = NULL;

/* We must first make a list with 1 string per item. */
	tmplist = g_list_first(config_list);
	while (tmplist != NULL) {
		DEBUG_MSG("save_config_file, tmpitem at %p\n", tmplist->data);
		tmpitem = tmplist->data;
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
			DEBUG_MSG("save_config_file, tmpitem(%p), &tmpitem=%p\n", tmpitem, &tmpitem);
			tmplist2 = g_list_last((GList *) * (void **) tmpitem->pointer);
			DEBUG_MSG("save_config_file, the tmplist2(%p)\n", tmplist2);
			while (tmplist2 != NULL) {
				tmpstring2 = (char *) tmplist2->data;
				DEBUG_MSG("save_config_file, tmpstring2(%p)=%s\n", tmpstring2, tmpstring2);
				tmpstring = g_strdup_printf("%s %s", tmpitem->identifier, tmpstring2);
				DEBUG_MSG("save_config_file, tmpstring(%p)=%s\n", tmpstring, tmpstring);

				rclist = g_list_append(rclist, tmpstring);
				tmplist2 = g_list_previous(tmplist2);
			}
			break;
		case 'a':
			DEBUG_MSG("save_config_file, tmpitem(%p), &tmpitem=%p\n", tmpitem, &tmpitem);
			tmplist2 = g_list_last((GList *) * (void **) tmpitem->pointer);
			DEBUG_MSG("save_config_file, the tmplist2(%p)\n", tmplist2);
			while (tmplist2 != NULL) {
				tmpstring2 = array_to_string((char **) tmplist2->data);
				tmpstring = g_strdup_printf("%s %s", tmpitem->identifier, tmpstring2);
				DEBUG_MSG("save_config_file, tmpstring(%p)=%s\n", tmpstring, tmpstring);
				rclist = g_list_append(rclist, tmpstring);
				tmplist2 = g_list_previous(tmplist2);
				g_free(tmpstring2);
			}
			break;
		default:
			break;
		}
		tmplist = g_list_next(tmplist);

	}

	put_stringlist(filename, rclist);
	free_stringlist(rclist);
	return 1;
}

static gboolean parse_config_file(GList * config_list, gchar * filename)
{
	gboolean retval = FALSE;
	gchar *tmpstring = NULL, *tmpstring2;
	gchar **tmparray;
	GList *rclist, *tmplist, *tmplist2;
	Tconfig_list_item *tmpitem;

	DEBUG_MSG("parse_config_file, started\n");

	rclist = NULL;
	rclist = get_list(filename, rclist,FALSE);
	
	if (rclist == NULL) {
		DEBUG_MSG("no rclist, returning!\n");
		return retval;
	}

	/* empty all variables that have type GList ('l') */
	tmplist = g_list_first(config_list);
	while (tmplist != NULL) {
		tmpitem = (Tconfig_list_item *) tmplist->data;
		DEBUG_MSG("parse_config_file, type=%c, identifier=%s\n", tmpitem->type, tmpitem->identifier);
		if (tmpitem->type == 'l' || tmpitem->type == 'a') {
			DEBUG_MSG("parse_config_file, freeing list before filling it\n");
			free_stringlist((GList *) * (void **) tmpitem->pointer);
			*(void **) tmpitem->pointer = (GList *)NULL;
		}
		DEBUG_MSG("parse_config_file, type=%c, identifier=%s\n", tmpitem->type, tmpitem->identifier);
		tmplist = g_list_next(tmplist);
	}
	DEBUG_MSG("parse_config_file, all the type 'l' and 'a' have been emptied\n");
	DEBUG_MSG("parse_config_file, length rclist=%d\n", g_list_length(rclist));
/* And now for parsing every line in the config file, first check if there is a valid identifier at the start. */
	tmplist = g_list_first(rclist);
	while (tmplist) {
		tmpstring = (gchar *) tmplist->data;

		if (tmpstring != NULL) {
			DEBUG_MSG("parse_config_file, tmpstring=%s\n", tmpstring);
			g_strchug(tmpstring);

			tmplist2 = g_list_first(config_list);
			while (tmplist2) {
				tmpitem = (Tconfig_list_item *) tmplist2->data;
#ifdef DEVELOPMENT
				if (!tmpitem || !tmpitem->identifier || !tmpstring) {
					g_print("WARNING: almost a problem!\n");
				}
#endif
				if (g_strncasecmp(tmpitem->identifier, tmpstring, strlen(tmpitem->identifier)) == 0) {
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
						tmpstring2 = unescape_string(tmpstring, FALSE); /* I wonder if that should be TRUE */
						*(void **) tmpitem->pointer = (char *) realloc((char *) *(void **) tmpitem->pointer, strlen(tmpstring2) + 1);
						strcpy((char *) *(void **) tmpitem->pointer, tmpstring2);
						g_free(tmpstring2);
						break;
					case 'l':
						tmpstring2 = g_strdup(tmpstring);
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
					tmplist2 = g_list_last(tmplist2);
				}
				tmplist2 = g_list_next(tmplist2);
			}
		}
		tmplist = g_list_next(tmplist);
	}
	DEBUG_MSG("parse_config_file, parsed all entries, freeing list read from file\n");	
	free_stringlist(rclist);
	return retval;
}

static GList *props_init_main(GList * config_rc)
{
/* these are used in the gtk-2 port already */
	init_prop_integer   (&config_rc, &main_v->props.view_html_toolbar, "view_html_toolbar:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.view_custom_menu, "view_custom_menu:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.view_main_toolbar, "view_main_toolbar:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.view_left_panel, "view_left_panel:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.view_line_numbers, "view_line_numbers:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.filebrowser_show_hidden_files, "fb_show_hidden_f:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.filebrowser_show_backup_files, "fb_show_backup_f:", 0, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.filebrowser_two_pane_view, "fb_two_pane_view:", 0, TRUE);
	init_prop_string    (&config_rc, &main_v->props.filebrowser_unknown_icon, "fb_unknown_icon:", PKGDATADIR"icon_unknown.png");
	init_prop_string    (&config_rc, &main_v->props.filebrowser_dir_icon, "fb_dir_icon:", PKGDATADIR"icon_dir.png");
	init_prop_string    (&config_rc, &main_v->props.editor_font_string, "editor_font_string:", "courier 11");
	init_prop_integer   (&config_rc, &main_v->props.editor_tab_width, "editor_tab_width:", 3, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.editor_indent_wspaces, "editor_indent_wspaces:", 0, TRUE);
	init_prop_string    (&config_rc, &main_v->props.tab_font_string, "tab_font_string:", "");
	init_prop_arraylist (&config_rc, &main_v->props.browsers, "browsers:", 2, TRUE);
	init_prop_arraylist (&config_rc, &main_v->props.external_commands, "external_commands:", 2, TRUE);
	init_prop_stringlist(&config_rc, &main_v->props.quickbar_items, "quickbar_items:", TRUE);
	init_prop_integer   (&config_rc, &main_v->props.highlight_num_lines_count, "highlight_num_lines_count:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.defaulthighlight, "defaulthighlight:", 1, TRUE);
	/* old type filetypes have a different count, they are converted below */
	init_prop_arraylist (&config_rc, &main_v->props.filetypes, "filetypes:", 0, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.numcharsforfiletype, "numcharsforfiletype:", 200, TRUE);
	init_prop_arraylist (&config_rc, &main_v->props.filefilters, "filefilters:", 3, TRUE);
	init_prop_string    (&config_rc, &main_v->props.last_filefilter, "last_filefilter:", "");
	init_prop_integer   (&config_rc, &main_v->props.transient_htdialogs, "transient_htdialogs:", 1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.restore_dimensions, "restore_dimensions:", 1, TRUE);	
	init_prop_integer   (&config_rc, &main_v->props.left_panel_width, "left_panel_width:", 150, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.two_pane_filebrowser_height, "two_pane_filebrowser_height:", 250, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.main_window_h, "main_window_height:", 400, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.main_window_w, "main_window_width:", 600, TRUE); /* negative width means maximized */
	init_prop_integer   (&config_rc, &main_v->props.max_recent_files, "max_recent_files:", 15, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.max_dir_history, "max_dir_history:", 10, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.backup_file,"backup_file:",1, TRUE);
	init_prop_string    (&config_rc, &main_v->props.backup_filestring,"backup_filestring:","~");
	init_prop_integer    (&config_rc, &main_v->props.backup_abort_action,"backup_abort_action:",DOCUMENT_BACKUP_ABORT_ASK, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.backup_cleanuponclose,"backup_cleanuponclose:",0, TRUE);
	init_prop_string    (&config_rc, &main_v->props.image_thumbnailstring, "image_thumbnailstring:", "_thumbnail");
	init_prop_string    (&config_rc, &main_v->props.image_thumbnailtype, "image_thumbnailtype:", "png");
	init_prop_integer   (&config_rc, &main_v->props.image_thumbnail_refresh_quality,"image_thumbnail_refresh_quality:",1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.image_thumbnailsizing_type,"image_thumbnailsizing_type:",0, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.image_thumbnailsizing_val1,"image_thumbnailsizing_val1:",100, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.image_thumbnailsizing_val2,"image_thumbnailsizing_val2:",100, TRUE);
	init_prop_string    (&config_rc, &main_v->props.image_thumnailformatstring,"image_thumnailformatstring:","<a href=\"%r\"><img src=\"%t\" width=\"%x\" height=\"%y\" border=\"0\"></a>");
	init_prop_integer   (&config_rc, &main_v->props.allow_multi_instances,"allow_multi_instances:",0, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.modified_check_type,"modified_check_type:",1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.num_undo_levels,"num_undo_levels:",100, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.clear_undo_on_save,"clear_undo_on_save:",0, TRUE);
	init_prop_string    (&config_rc, &main_v->props.newfile_default_encoding,"newfile_default_encoding:","UTF-8");
	init_prop_arraylist (&config_rc, &main_v->props.encodings, "encodings:", 2, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.auto_set_encoding_meta,"auto_set_encoding_meta:",1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.auto_update_meta,"auto_update_meta:",1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.encoding_search_Nbytes, "encoding_search_Nbytes:", 500, TRUE);
	init_prop_arraylist (&config_rc, &main_v->props.outputbox, "outputbox:", 7, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.ext_browsers_in_submenu,"ext_browsers_in_submenu:",0, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.ext_commands_in_submenu,"ext_commands_in_submenu:",1, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.ext_outputbox_in_submenu,"ext_outputbox_in_submenu:",1, TRUE);
	init_prop_arraylist (&config_rc, &main_v->props.reference_files, "reference_files:", 2, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.fref_ldoubleclick_action,"fref_ldoubleclick_action:",0, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.fref_info_type,"fref_info_type:",0, TRUE);	
	init_prop_integer   (&config_rc, &main_v->props.document_tabposition,"document_tabposition:",(gint)GTK_POS_BOTTOM, TRUE);
	init_prop_integer   (&config_rc, &main_v->props.leftpanel_tabposition,"leftpanel_tabposition:",(gint)GTK_POS_BOTTOM, TRUE);
	init_prop_string    (&config_rc, &main_v->props.default_basedir,"default_basedir:",g_get_home_dir());
	init_prop_string    (&config_rc, &main_v->props.project_suffix,"project_suffix:",".bfproject");
#ifdef HAVE_LIBASPELL
	init_prop_string(&config_rc, &main_v->props.spell_default_lang, "spell_default_lang:", "en");
#endif /* HAVE_LIBASPELL */
	init_prop_integer(&config_rc, &main_v->props.cont_highlight_full, "cont_highlight_full:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.lasttime_cust_menu, "lasttime_cust_menu:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.lasttime_highlighting, "lasttime_highlighting:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.lasttime_filetypes, "lasttime_filetypes:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.lasttime_encodings, "lasttime_encodings:", 0, TRUE);
	/* not yet in use */
	init_prop_string(&config_rc, &main_v->props.image_editor_cline, "image_editor_command:", "gimp-remote -n \"%s\"&");
	init_prop_integer(&config_rc, &main_v->props.full_p, "closing_paragraph_tag:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.full_li, "closing_list_item_tag:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.allow_css, "allow_the_use_of_css:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.allow_dep, "allow_the_use_of_font:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.format_by_context, "format_by_context:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.xhtml, "use_xhtml:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.allow_ruby, "allow_the_use_of_ruby:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.allow_h4, "allow_the_use_of_html4:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.allow_frames, "allow_the_use_of_frames:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.force_dtd, "force_an_dtd:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.dtd_url, "url_in_dtd:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.xml_start, "xml_starting_line:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.lowercase_tags, "lowercase_tags:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.word_wrap, "word_wrap:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.autoindent, "autoindent:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.fontset, "fontset:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.force_def_style, "force_white_background:", 1, TRUE);
	init_prop_integer(&config_rc, &main_v->props.drop_at_drop_pos, "drop_at_drop_position:", 0, TRUE);
	init_prop_integer(&config_rc, &main_v->props.link_management, "link_management:", 1, TRUE);
	
	init_prop_integer(&config_rc, &main_v->props.cont_highlight_update, "continuous_highlight_update:", 1, TRUE);
	init_prop_string(&config_rc, &main_v->props.html_ver, "used_html_version:", "HTML 4.0 Transitional");
	/* key conversion */
	init_prop_integer (&config_rc, &main_v->props.conv_ctrl_enter,"conv_ctrl_enter:",0, TRUE);
	init_prop_integer (&config_rc, &main_v->props.conv_shift_enter,"conv_shift_enter:",0, TRUE);
	init_prop_string_with_escape (&config_rc, &main_v->props.shift_enter_text,"shift_enter_text:","<br>\\n");
	init_prop_string_with_escape (&config_rc, &main_v->props.ctrl_enter_text,"ctrl_enter_text:","\\n</p>\\n<p>");
	init_prop_integer (&config_rc, &main_v->props.conv_special_char,"conv_special_char:",0, TRUE);
#ifdef WITH_MSG_QUEUE
	init_prop_integer (&config_rc, &main_v->props.open_in_running_bluefish,"open_in_running_bluefish:",1, TRUE);
#endif

	return config_rc;
}

/* we save the value in 'days' precision, so we can divide seconds by 24*60*60
, this way we can store it in a gint (which is the config file precision) */
#define TIME_T_TO_GINT(time) ((gint)(time / (24*60*60)))

static gboolean config_file_is_newer(gint lasttime, const gchar *configfile) {
	struct stat statbuf;
	if(stat(configfile, &statbuf)==0) {
		if (TIME_T_TO_GINT(statbuf.st_mtime) >= lasttime) return TRUE;
	}
	return FALSE;
}

/*
static GList *arraylist_load_defaults(GList *thelist, const gchar *filename, const gchar *name) {
	GList *deflist,*tmplist = g_list_first(thelist);
	if (name) {
		while (tmplist) {
			gchar **tmparr = tmplist->data;
			if (strcmp(tmparr[0],name)==0) {
				GList *todelete = tmplist;
				tmplist = g_list_next(tmplist);
				if (tmplist) {
					g_list_delete_link(tmplist, todelete);
					g_strfreev(tmparr);
					g_list_free_1(todelete);
				} else {
					thelist = NULL;
					g_strfreev(tmparr);
					g_list_free(todelete);
				}
			} else {
				tmplist = g_list_next(tmplist);
			}
		}
	} else {
		while (tmplist) {
			g_strfreev((gchar **)tmplist->data);
			tmplist = g_list_next(tmplist);
		}
		g_list_free(thelist);
		thelist = NULL;
	}
	if (name) {
		deflist = get_list(filename,NULL,TRUE);
		tmplist = g_list_first(deflist);
		while (tmplist) {
			gchar **tmparr = tmplist->data;
			DEBUG_MSG("arraylist_load_defaults, testing if %s should be added (requested=%s)\n",tmparr[0],name);
			if (strcmp(tmparr[0],name)==0) {
				DEBUG_MSG("adding %s to thelist\n",tmparr[0]);
				thelist = g_list_append(thelist, duplicate_stringarray(tmparr));
			}
			tmplist = g_list_next(tmplist);
		}
		free_arraylist(deflist);
	} else {
		thelist = get_list(filename,NULL,TRUE);
	}
	return thelist;
}
*/
void rcfile_parse_main(void)
{
	gchar *filename;

	DEBUG_MSG("rcfile_parse_main, started\n");

	/* set the props struct completely empty */
	memset(&main_v->props, 0, sizeof(Tproperties));

	/*Make the config_rc list ready for filling with data and set default values */
	main_configlist = props_init_main(NULL);

	filename = g_strconcat(g_get_home_dir(), "/.bluefish/rcfile_v2", NULL);
	if (!parse_config_file(main_configlist, filename)) {
		/* should we initialize some things ?? */
	}
	g_free(filename);
	/* do some default configuration for the lists */
	if (main_v->props.browsers == NULL) {
		/* if the user does not have browsers --> set them to defaults values */
		gchar **arr;
		arr = array_from_arglist(_("Galeon"), "galeon -x %s&",NULL);
		main_v->props.browsers = g_list_append(main_v->props.browsers,arr);
		arr = array_from_arglist(_("Mozilla"), "mozilla -remote 'openURL(%s, new-window)' || mozilla %s&",NULL);
		main_v->props.browsers = g_list_append(main_v->props.browsers,arr);
		arr = array_from_arglist(_("Opera"), "opera -remote 'openURL(%s,new-window)' || opera %s&",NULL);
		main_v->props.browsers = g_list_append(main_v->props.browsers,arr);
		arr = array_from_arglist(_("Netscape"), "/usr/lib/netscape/477/communicator/communicator-smotif %s&",NULL);
		main_v->props.browsers = g_list_append(main_v->props.browsers,arr);
		arr = array_from_arglist(_("Gnome default"), "gnome-moz-remote --newwin %s&",NULL);
		main_v->props.browsers = g_list_append(main_v->props.browsers,arr);
	}
	{
		gchar *defaultfile = return_first_existing_filename(PKGDATADIR"encodings.default",
											"data/encodings.default",
										"../data/encodings.default",NULL);
		if (main_v->props.encodings == NULL) {
			/* if the user does not have encodings --> set them to defaults values */
			if (defaultfile) {
				main_v->props.encodings = get_list(defaultfile,NULL,TRUE);
			} else {
				g_print("Unable to find '"PKGDATADIR"encodings.default'\n");
			}
		} else {
			if (config_file_is_newer(main_v->props.lasttime_encodings,defaultfile)) {
				main_v->props.encodings = arraylist_load_new_identifiers_from_file(main_v->props.encodings,defaultfile,1);
				main_v->props.lasttime_encodings = TIME_T_TO_GINT(time(NULL));
			}
		}
		g_free(defaultfile);
	}
	if (main_v->props.outputbox==NULL) {
		/* if the user does not have outputbox settings --> set them to defaults values */
		main_v->props.outputbox = g_list_append(main_v->props.outputbox,array_from_arglist(_("make"),"([a-zA-Z0-9/_.-]+):([0-9]+):(.*)","1","2","3","make","1",NULL));
		main_v->props.outputbox = g_list_append(main_v->props.outputbox,array_from_arglist(_("weblint HTML checker"),"([a-zA-Z0-9/_.-]+)\\(([0-9]+)\\): (.*)","1","2","3","weblint '%s'","1",NULL));
		main_v->props.outputbox = g_list_append(main_v->props.outputbox,array_from_arglist(_("tidy HTML validator"),"line ([0-9]+) column [0-9]+ - (.*)","-1","1","2","tidy -qe '%s'","0",NULL));
		main_v->props.outputbox = g_list_append(main_v->props.outputbox,array_from_arglist(_("javac"),"([a-zA-Z0-9/_.-]+):([0-9]+):(.*)","1","2","3","javac '%s'","0",NULL));
/*		main_v->props.outputbox = g_list_append(main_v->props.outputbox,array_from_arglist(,NULL)); */
	}
	if (main_v->props.external_commands == NULL) {
		/* if the user does not have external commands --> set them to defaults values */
		gchar **arr;
		arr = array_from_arglist(_("Dos2Unix filter"), "cat '%s' | dos2unix > '%f'",NULL);
		main_v->props.external_commands = g_list_append(main_v->props.external_commands,arr);
		arr = array_from_arglist(_("Tidy cleanup filter"), "cat '%s' | tidy -utf8 -q >'%f' 2>/dev/null",NULL);
		main_v->props.external_commands = g_list_append(main_v->props.external_commands,arr);
	}
	{
	gchar *defaultfile = return_first_existing_filename(PKGDATADIR"filetypes.default",
										"data/filetypes.default",
									"../data/filetypes.default",NULL);
		if (main_v->props.filetypes == NULL) {
			/* if the user does not have file-types --> set them to defaults values */
			if (defaultfile) {
				main_v->props.filetypes = get_list(defaultfile,NULL,TRUE);
			} else {
				g_print("Unable to find '"PKGDATADIR"filetypes.default'\n");
			}
		} else {
			if (config_file_is_newer(main_v->props.lasttime_filetypes,defaultfile)) {
				main_v->props.filetypes = arraylist_load_new_identifiers_from_file(main_v->props.filetypes,defaultfile,1);
				main_v->props.lasttime_filetypes = TIME_T_TO_GINT(time(NULL));
			}
		}
		g_free(defaultfile);
	}
	if (main_v->props.filefilters == NULL) {
		/* if the user does not have file filters --> set them to defaults values */
		gchar **arr;
		arr = array_from_arglist(_("C programming"),"1","c:image", NULL);
		main_v->props.filefilters = g_list_append(main_v->props.filefilters, arr);
		arr = array_from_arglist(_("All web files"),"1", "html:php:webimage:xml:javascript:stylesheet:jsp", NULL);
		main_v->props.filefilters = g_list_append(main_v->props.filefilters, arr);
		arr = array_from_arglist(_("Java programming"),"1", "java:image:jsp", NULL);
		main_v->props.filefilters = g_list_append(main_v->props.filefilters, arr);
		arr = array_from_arglist(_("Images"),"1", "image", NULL);
		main_v->props.filefilters = g_list_append(main_v->props.filefilters, arr);
		arr = array_from_arglist(_("Hide objectfiles"),"0", "objectfile", NULL);
		main_v->props.filefilters = g_list_append(main_v->props.filefilters, arr);
	}
	if (main_v->props.reference_files == NULL) {
		gchar *userdir = g_strconcat(g_get_home_dir(), "/.bluefish/", NULL);
		/* if the user does not yet have any function reference files, set them to default values */
		fref_rescan_dir(PKGDATADIR);
		fref_rescan_dir(userdir);
		g_free(userdir);
	}
	/* for backwards compatibility with old filetypes */
	{
		GList *tmplist = g_list_first(main_v->props.filetypes);
		while (tmplist) {
			gchar **orig = (gchar **)tmplist->data;
			if (count_array(orig)==4) {
				gchar **new = array_from_arglist(orig[0], orig[1], orig[2], orig[3], "1", "", NULL);
				tmplist->data = new;
				g_strfreev(orig);
			}
			tmplist = g_list_next(tmplist);
		}
	}
}

static gint rcfile_save_main(void) {
	gchar *filename = g_strconcat(g_get_home_dir(), "/.bluefish/rcfile_v2", NULL);
	return save_config_file(main_configlist, filename);
}
/*
static gboolean arraylist_test_identifier_exists(GList *arrlist, const gchar *name) {
	GList *tmplist = g_list_first(arrlist);
	while(tmplist) {
		if (strcmp(name, ((gchar **)(tmplist->data))[0])==0) {
			return TRUE;
		}
		tmplist = g_list_next(tmplist);
	}
	return FALSE;
}
*/
void rcfile_parse_highlighting(void) {
	gchar *filename;
	gchar *defaultfile;

	DEBUG_MSG("rcfile_parse_highlighting, started\n");

	highlighting_configlist = NULL;
	init_prop_arraylist(&highlighting_configlist, &main_v->props.highlight_patterns, "patterns:", 0, TRUE);

	filename = g_strconcat(g_get_home_dir(), "/.bluefish/highlighting", NULL);
	defaultfile = return_first_existing_filename(PKGDATADIR"highlighting.default",
									"data/highlighting.default",
									"../data/highlighting.default",NULL);
	if (!parse_config_file(highlighting_configlist, filename)) {
		/* init the highlighting in some way? */
		if (defaultfile) {
			main_v->props.highlight_patterns = get_list(defaultfile,NULL,TRUE);
		} else {
			g_print("Unable to find '"PKGDATADIR"highlighting.default'\n");
		}
		save_config_file(highlighting_configlist, filename);
		DEBUG_MSG("rcfile_parse_highlighting, done saving\n");
	} else {
		if (config_file_is_newer(main_v->props.lasttime_highlighting,defaultfile)) {
			main_v->props.highlight_patterns = arraylist_load_new_identifiers_from_file(main_v->props.highlight_patterns,defaultfile,2);
			main_v->props.lasttime_highlighting = TIME_T_TO_GINT(time(NULL));
		}
	}
	g_free(filename);
	g_free(defaultfile);
}

static gint rcfile_save_highlighting(void) {
	gint retval;
	gchar *filename = g_strconcat(g_get_home_dir(), "/.bluefish/highlighting", NULL);
	retval = save_config_file(highlighting_configlist, filename);
	g_free(filename);
	return retval;
}

void rcfile_parse_custom_menu(void) {
	gchar *filename;
	gchar *defaultfile;
	DEBUG_MSG("rcfile_parse_custom_menu, started\n");

	custom_menu_configlist = NULL;
	init_prop_arraylist(&custom_menu_configlist, &main_v->props.cust_menu, "custom_menu:", 0, TRUE);
	init_prop_arraylist(&custom_menu_configlist, &main_v->props.cmenu_insert, "cmenu_insert:", 0, TRUE);
	init_prop_arraylist(&custom_menu_configlist, &main_v->props.cmenu_replace, "cmenu_replace:", 0, TRUE);

	filename = g_strconcat(g_get_home_dir(), "/.bluefish/custom_menu", NULL);
	defaultfile = return_first_existing_filename(PKGDATADIR"custom_menu.default",
									"data/custom_menu.default",
									"../data/custom_menu.default",NULL);
	if (!parse_config_file(custom_menu_configlist, filename) || (main_v->props.cust_menu==NULL && main_v->props.cmenu_insert==NULL && main_v->props.cmenu_replace==NULL )) {
		DEBUG_MSG("error parsing the custom menu file\n");
		/* init the custom_menu in some way? */
		if (defaultfile) {
			parse_config_file(custom_menu_configlist, defaultfile);
		} else {
			g_print("Unable to find '"PKGDATADIR"custom_menu.default'\n");
		}
	} else {
		if (config_file_is_newer(main_v->props.lasttime_cust_menu,defaultfile)) {
			GList *default_insert=NULL, *default_replace=NULL, *tmp_configlist=NULL;
			DEBUG_MSG("config_file_is_newer!\n");
			init_prop_arraylist(&tmp_configlist, &default_insert, "cmenu_insert:", 0, TRUE);
			init_prop_arraylist(&tmp_configlist, &default_replace, "cmenu_replace:", 0, TRUE);
			parse_config_file(tmp_configlist, defaultfile);
			main_v->props.cmenu_insert = arraylist_load_new_identifiers_from_list(main_v->props.cmenu_insert, default_insert, 1);
			main_v->props.cmenu_replace = arraylist_load_new_identifiers_from_list(main_v->props.cmenu_replace, default_replace, 1);
			main_v->props.lasttime_cust_menu = TIME_T_TO_GINT(time(NULL));
			free_arraylist(default_replace);
			free_arraylist(default_insert);
			free_configlist(tmp_configlist);
		}
	}
	/* for backwards compatibility with older (before Bluefish 0.10) custom menu files we can convert those.. 
	we will not need the 'type' anymore, since we will put them in separate lists, hence the memmove() call
	*/
	DEBUG_MSG("main_v->props.cust_menu=%p\n",main_v->props.cust_menu);
	if (main_v->props.cust_menu) {
		GList *tmplist= g_list_first(main_v->props.cust_menu);
		while (tmplist) {
			gchar **strarr = (gchar **)tmplist->data;
			gint count = count_array(strarr);
			DEBUG_MSG("converting cust_menu, found count=%d\n",count);
			if (count >= 5 && strarr[1][0] == '0') {
				DEBUG_MSG("rcfile_parse_custom_menu, converting insert, 0=%s, 1=%s\n", strarr[0], strarr[1]);
				g_free(strarr[1]);
				memmove(&strarr[1], &strarr[2], (count-1) * sizeof(gchar *));
				main_v->props.cmenu_insert = g_list_append(main_v->props.cmenu_insert, strarr);
			} else if (count >= 8 && strarr[1][0] == '1') {
				DEBUG_MSG("rcfile_parse_custom_menu, converting replace, 0=%s, 1=%s\n", strarr[0], strarr[1]);
				g_free(strarr[1]);
				memmove(&strarr[1], &strarr[2], (count-1) * sizeof(gchar *));
				main_v->props.cmenu_replace = g_list_append(main_v->props.cmenu_replace, strarr);
			} else if (count >= 4 && count == (4+atoi(strarr[1]))) { /*  the first check avoids a segfault if count == 1 */
				/* a very old insert type, 0=menupath, 1=numvariables, 2=string1, 3=string2, 4... are variables 
				   we can re-arrange it for the new insert type */
				gchar *numvars = strarr[1];
				strarr[1] = strarr[2];
				strarr[2] = strarr[3];
				strarr[3] = numvars; /* the variables; beyond [3], are still the same */
				DEBUG_MSG("rcfile_parse_custom_menu, converting very old insert, 0=%s\n", strarr[0]);
				main_v->props.cmenu_insert = g_list_append(main_v->props.cmenu_insert, strarr);
			} else {
#ifdef DEBUG
				if (count > 2) {
					g_print("rcfile_parse_custom_menu, ignoring %s with type %s (count=%d)\n",strarr[0], strarr[1], count);
				} else {
					g_print("rcfile_parse_custom_menu, ignoring invalid cust_menu entry with count=%d..\n", count);
				}
#endif
			}
			tmplist = g_list_next(tmplist);
		}
		g_list_free(main_v->props.cust_menu);
		main_v->props.cust_menu=NULL;
	}
	
/*		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/ibase/ibase fetch row"), "0", "while ($%1 = ibase_fetch_row($%0)) {\n	echo $%1[0];\n", "\n}\nibase_free_result($%0);\n", "2", _("ibase result variable name"), _("row variable name"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/mysql/mysql fetch row"), "0", "while ($%1 = mysql_fetch_row($%0)) {\n	echo $%1[0];\n", "\n}\nmysql_free_result($%0);\n", "2", _("mysql result variable name"), _("row variable name"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/mysql/mysql connect"), "0", "$mysql_id = mysql_connect('%0', '%1', '%2');\nmysql_select_db('%3', $mysql_id);\n", "", "4", _("mysql host"), _("mysql username"), _("mysql password"), _("database name"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/ibase/ibase connect"), "0", "$ibase_id = ibase_connect('%0:%1', '%2', '%3');\n", "", "4", _("ibase host"), _("ibase database path"), _("ibase username"), _("ibase password"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/general/while each in array list"), "0", "while (list($key, $val) = each($%0)) {\n	", "\n}\n", "1", _("array name"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/sql/select all from table"), "0", "SELECT * FROM %0", "", "1", _("table name"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/general/mail"), "0", "mail(\"%0\", \"%1\", $%2);\n", "", "3", _("email adress"), _("subject"), _("mail body variable name"),  NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/general/for loop"), "0", "for ($%0 = %1; $%0 <= %2; $%0++) {\n ", "\n}\n", "3", _("loop variable name"), _("from value"), _("to value"),  NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/ibase/query"), "0", "$%0 = ibase_query(%1, %2);\nif (ibase_errmsg() != 0){\n	echo 'Error: '.%2.' returned '.ibase_errmsg().\"<br>\\n\";\n}\n", "", "3", _("result variable name"), _("ibase connection id name"), _("query string (quoted) or variable (including $)"),  NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/general/include file"), "0", "include('%0');\n", "", "1", _("filename to include"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/general/require file"), "0", "require('%0');\n", "", "1", _("filename to require"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/general/if isset"), "0", "if (isset($%0)) {\n	", "\n}", "1", _("variable name to test for"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/general/if not isset"), "0", "if (!isset($%0)) {\n	", "\n}", "1", _("variable name to test for"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/mysql/query"), "0", "$%0 = mysql_query(%2, %1);\n", "", "3", _("result variable name"), _("mysql connection id name"), _("query string (quoted) or variable (including $)"),  NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/html/font symbol"), "0", "<font face=\"Symbol\">", "</font>", NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/html/div with class"), "0", "<div class=\"%0\">", "</div>", "1", _("CSS class name"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/html/span with class"), "0", "<span class=\"%0\">", "</span>", "1", _("CSS class name"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/php block"), "0", "\n<?php\n", "\n?>\n", NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/html/made with bluefish"), "0", "<a href=\"http://bluefish.openoffice.nl/\">Made with the Bluefish HTML editor</a>", "", NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/html/meta keywords"), "0", "<meta name=\"keywords\" value=\"%0\">", "", "1", _("keywords, space separated"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/html/meta refresh (client pull)"), "0", "<meta http-equiv=\"refresh\" content=\"%0; URL=%1\">", "", "2", _("refresh time, in seconds"), _("URL to redirect to"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/active html/form"), "0", "<form method=\"POST\" action=\"<?php echo $SCRIPT_NAME ?>\">\n\n", "</form>", NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/active html/form and action"), "0", "<?php if (isset($submitted)) {\n//form submit code here\n} else { ?>\n<form method=\"POST\" action=\"<?php echo $SCRIPT_NAME ?>\">\n\n<input type=\"submit\" name=\"submitted\" value=\"%0\"></form>\n<?php } // end of form ?>", "", "1", _("Submit button text"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/active html/input text"), "0", "<input type=\"text\" name=\"%0\" value=\"<?php if (isset($%0)) { echo $%0; } ?>\">", "", "1", _("variable name"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/active html/input checkbox"), "0", "<input type=\"checkbox\" name=\"%0\" value=\"%1\" <?php if (isset($%0)) { echo 'checked'; } ?>>", "", "2", _("variable name"), _("value when checked"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/active html/textarea"), "0", "<textarea name=\"%0\" rows=\"20\" cols=\"40\"><?php if (isset($%0)) { echo $%0; } ?></textarea>", "", "1", _("variable name"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/active html/select options/from array"), "0", "<select name=\"%0\">\n<?php\nwhile(list($key, $val)=each($%1)) {\n\tif (isset($%0) && ($%0 == $val)) {\n\t\techo '<option value=\"'.$key.'\" selected>'.$val.'</option>';\n\t} else {\n\t\techo '<option value=\"'.$key.'\">'.$val.'</option>';\n\t}\n}\n?></select>", "", "2", _("variable name"), _("array name containing options"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/active html/select options/from mysql result"), "0", "<select name=\"%0\">\n<?php\nwhile($row = mysql_fetch_row($%1)) {\n\tif (isset($%0) && ($%0 == $row[%2])) {\n\t\techo '<option value=\"'.$row[%2].'\" selected>'.$row[%3].'</option>';\n\t} else {\n\t\techo '<option value=\"'.$row[%2].'\">'.$row[%3].'</option>';\n\t}\n}\n?></select>", "", "4", _("variable name"), _("mysql query result"), _("index of the value"), _("index of the label"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/active html/select options/from ibase result"), "0", "<select name=\"%0\">\n<?php\nwhile($row = ibase_fetch_row($%1)) {\n\tif (isset($%0) && ($%0 == $row[%2])) {\n\t\techo '<option value=\"'.$row[%2].'\" selected>'.$row[%3].'</option>';\n\t} else {\n\t\techo '<option value=\"'.$row[%2].'\">'.$row[%3].'</option>';\n\t}\n}\n?></select>", "", "4", _("variable name"), _("ibase query result"), _("index of the value"), _("index of the label"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/DHTML/mouseover image"), "0", "<a href=\"\" onMouseOver=\"bfmovepic('%0', '%2')\" onMouseOut=\"bfmovepic('%0', '%1')\"><img src=\"%1\" name=\"%0\"></a>", "", "3", _("unique name"), _("onMouseOut src"), _("onMouseOver src"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/odbc/odbc connect"), "0", "$odbc_id = odbc_connect('%0', '%1', '%2');\n", "", "3", _("odbc data source name"), _("username"), _("password"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/odbc/odbc exec query"), "0", "$%0 = odbc_exec(%1, %2);\n", "", "3", _("result variable name"), _("odbc connection id name"), _("query string (quoted) or variable (including $)"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/odbc/odbc fetch row"), "0", "while($success = odbc_fetch_row($%0)) {\n\t\n}", "", "1", _("result variable name"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/general/echo date"), "0", "echo date(\"l dS of F Y h:i:s A\");", "", NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/filesystem/open and echo file"), "0", "$fd = fopen(%0, 'r');\nwhile (!feof($fd)) {\n\t$buffer = fgets($fd, 4096);\n\techo $buffer;\n}\nfclose($fd);", "", "1", _("file (quoted) or variable (including $)"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/php/general/location header"), "0", "header('Location: '.%0);", "", "1", _("location (URL) to redirect to (quoted) or variable (including $)"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/apache/basic password required"), "0", "AuthName \"%1\"\nAuthType Basic\nrequire valid-user\nAuthUserFile %0\n", "", "2", _("full path to password file"), _("security description"), NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/apache/deny access to .inc"), "0", "<FilesMatch \"\\.inc$\">\n	Order allow,deny\n	Deny from all\n</FilesMatch>", "", "0", NULL));	
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/replace/strip <font>"), "1", "(<font[^>]*>|</font>)", "", "0", "1", "0", "0", NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/replace/strip <any tag> in selection"), "1", "(<%0[^>]*>|</%0>)", "", "2", "1", "0", "1", "tag name, like td, font or a", NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/replace/convert <td> in <th> in selection"), "1", "(<|</)(td)([^>]*)(>)", "\\0th\\2>", "2", "1", "0", "0", NULL));
		main_v->props.cust_menu = g_list_append(main_v->props.cust_menu, array_from_arglist(N_("/replace/convert <any tag> in <any other tag> in selection"), "1", "(<|</)(%0)([^>]*)(>)", "\\0%1\\2>", "2", "1", "0", "2", "any tag name", "any other tag name", NULL));
	}
	*/

	g_free(filename);
	g_free(defaultfile);
}
static gint rcfile_save_custom_menu(void) {
	gint retval;
	gchar *filename = g_strconcat(g_get_home_dir(), "/.bluefish/custom_menu", NULL);
	retval = save_config_file(custom_menu_configlist, filename);
	g_free(filename);
	return retval;
}

#define DIR_MODE (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)	/* same as 0755 */
void rcfile_check_directory(void) {
	gchar *rcdir = g_strconcat(g_get_home_dir(), "/.bluefish", NULL);
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
	case 1:
		rcfile_save_highlighting();
	break;
	case 2:
		rcfile_save_custom_menu();
	break;
	case 3:
		{
			gchar *shortcutfilename = g_strconcat(g_get_home_dir(), "/.bluefish/menudump_2", NULL);
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

void rcfile_save_all(void) {
	rcfile_save_main();
	rcfile_save_highlighting();
	rcfile_save_custom_menu();
}

static GList *return_project_configlist(Tproject *project) {
	GList *configlist = NULL;
	init_prop_string(&configlist, &project->name,"name:",_("Untitled Project"));
	init_prop_stringlist(&configlist, &project->files, "files:", FALSE);
	init_prop_string(&configlist, &project->basedir,"basedir:","");
	init_prop_string(&configlist, &project->webdir,"webdir:","");
	init_prop_string(&configlist, &project->template,"template:","");
	init_prop_stringlist(&configlist, &project->recentfiles, "recentfiles:", FALSE);
	init_prop_integer (&configlist, &project->view_main_toolbar,"view_main_toolbar:",1,FALSE);
	init_prop_integer (&configlist, &project->view_left_panel,"view_left_panel:",1,FALSE);
	init_prop_integer (&configlist, &project->view_custom_menu,"view_custom_menu:",1,FALSE);
	init_prop_integer (&configlist, &project->view_html_toolbar,"view_html_toolbar:",1,FALSE);
	init_prop_integer (&configlist, &project->word_wrap,"word_wrap:",1,FALSE);

	init_prop_stringlist(&configlist, &project->session->searchlist, "searchlist:", FALSE);
	init_prop_stringlist(&configlist, &project->session->replacelist, "replacelist:", FALSE);
	init_prop_stringlist(&configlist, &project->session->classlist, "classlist:", FALSE);
	init_prop_stringlist(&configlist, &project->session->colorlist, "colorlist:", FALSE);
	init_prop_stringlist(&configlist, &project->session->targetlist, "targetlist:", FALSE);
	init_prop_stringlist(&configlist, &project->session->urllist, "urllist:", FALSE);
	init_prop_stringlist(&configlist, &project->session->fontlist, "fontlist:", FALSE);
	init_prop_stringlist(&configlist, &project->session->dtd_cblist, "dtd_cblist:", FALSE);

	return configlist;
}

gboolean rcfile_parse_project(Tproject *project, gchar *filename) {
	gboolean retval;
	GList *configlist = return_project_configlist(project);
	retval = parse_config_file(configlist, filename);
	free_configlist(configlist);
	return retval;
}

gboolean rcfile_save_project(Tproject *project, gchar *filename) {
	gboolean retval;
	GList *configlist = return_project_configlist(project);
	DEBUG_MSG("rcfile_save_project, project %p, name='%s', basedir='%s', webdir='%s'\n",project, project->name, project->basedir, project->webdir);
	retval = save_config_file(configlist, filename);
	free_configlist(configlist);
	return retval;
}
