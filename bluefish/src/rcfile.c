
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "bluefish.h"
#include "rcfile.h"
#include "bf_lib.h"
#include "stringlist.h"
#include "highlight.h" /* hl_reset_to_default()*/

typedef struct {
	void *pointer;
	unsigned char type;
	gchar *identifier;
} Tconfig_list_item;

static GList *main_configlist=NULL;
static GList *highlighting_configlist=NULL;
static GList *custom_menu_configlist=NULL;

gchar **array_from_arglist(const gchar *string1, ...) {
	gint numargs=1;
	va_list args;
	gchar *s;
	gchar **retval, **index;

	va_start (args, string1);
	s = va_arg (args, gchar*);
	while (s) {
		numargs++;
		s = va_arg (args, gchar*);
	}
	va_end (args);
	DEBUG_MSG("array_from_arglist, numargs=%d\n", numargs);

	index = retval = g_new(gchar *, numargs + 1);
	*index = g_strdup(string1);

	va_start (args, string1);
	s = va_arg (args, gchar*);
	while (s) {
		index++;
		*index = g_strdup(s);
		s = va_arg (args, gchar*);
	}
	va_end (args);
	index++;
	*index = NULL;
	return retval;
}

static gchar *array_to_string(gchar **array, gchar delimiter) {
	if (array) {
		gchar **tmp, *escaped1, *finalstring;
		gint newsize=1;
		DEBUG_MSG("array_to_string, started\n");
		finalstring = g_malloc0(newsize);
		tmp = array;
		while(*tmp) {
			DEBUG_MSG("array_to_string, *tmp = %s\n", *tmp);
			escaped1 = escapestring(*tmp, delimiter);
			newsize += strlen(escaped1)+1;
			finalstring = g_realloc(finalstring, newsize);
			strcat(finalstring, escaped1);
			finalstring[newsize-2] = delimiter;
			finalstring[newsize-1] = '\0';
			g_free(escaped1);
			tmp++;
		}	
		DEBUG_MSG("array_to_string, finalstring = %s\n", finalstring);
		return finalstring;
	} else {
#ifdef DEBUG
		g_print("array_to_string, array=NULL !!!\n");
		exit(135);
#else
		return g_strdup("");
#endif
	}
}

#define ARRAYBLOCKSIZE 6
#define BUFBLOCKSIZE 60
gchar **string_to_array(gchar *string, gchar delimiter) {
	gchar **array;
	gchar *tmpchar, *tmpchar2;
	gchar *newstring;
	gint count=0;
	gint newstringsize;
	gint arraycount=0, arraysize;

	newstringsize = BUFBLOCKSIZE;
	newstring = g_malloc(newstringsize * sizeof(char));
	
	arraysize = ARRAYBLOCKSIZE;
	array = g_malloc(arraysize * sizeof(char *));
	DEBUG_MSG("string_to_array, started, array=%p\n", array);	
	
	tmpchar = string;
	while (*tmpchar != '\0') {
		DEBUG_MSG("string_to_array, count=%d, newstring(%p)\n", count, newstring);
		if (*tmpchar == '\\') {
			tmpchar2 = tmpchar;
			tmpchar2++;
			switch (*tmpchar2) {
			case '\0':
				newstring[count] = '\\';
			break;
			case '\\':
				newstring[count] = '\\';
				tmpchar++;
			break;
			case 'n':
				newstring[count] = '\n';
				tmpchar++;
			break;
			default:
				if (*tmpchar2 == delimiter) {
					newstring[count] = delimiter;
					tmpchar++;
				} else {
					DEBUG_MSG("string_to_array, weird, an unescaped backslash ?\n");
					newstring[count] = '\\';
				}
			break;
			}
		} else if (*tmpchar == delimiter) {
			newstring[count] = '\0';  /* end of the current newstring */
			DEBUG_MSG("string_to_array, newstring(%p)=%s\n", newstring, newstring);
			array[arraycount] = g_strdup(newstring);
			DEBUG_MSG("string_to_array, found delimiter, arraycount=%d, result(%p)=%s\n",arraycount, array[arraycount], array[arraycount]);
			arraycount++;
			if (arraycount == arraysize-2) { /* we need 1 spare entry in the array */
				arraysize += ARRAYBLOCKSIZE;  /* and arraysize starts at 1, arraycount at 0 */
				DEBUG_MSG("string_to_array, arraycount=%d, about to increase arraysize to %d, sizeof(array(%p))=%d\n", arraycount, arraysize, array, sizeof(&array));
				array = g_realloc(array, arraysize * sizeof(char *));
				DEBUG_MSG("string_to_array, arraysize=%d, array(%p), sizeof(array)=%d\n", arraysize, array, sizeof(&array));
			}
			count = -1;
		} else {
			newstring[count] = *tmpchar;
		}
		tmpchar++;
		count++;
		if (count == newstringsize-2) {
			newstringsize += BUFBLOCKSIZE;
			DEBUG_MSG("string_to_array, about to increase newstring(%p) to %d bytes\n", newstring, newstringsize);
			newstring = g_realloc(newstring, newstringsize * sizeof(char));
			DEBUG_MSG("string_to_array, newstringsize=%d, sizeof(newstring(%p))=%d\n", newstringsize, newstring, sizeof(newstring));
		}
	}
	
	if (count > 0) {
		newstring[count] = '\0';
		array[arraycount] = g_strdup(newstring);
		DEBUG_MSG("string_to_array, last array entry, arraycount=%d, result(%p)=%s\n",arraycount, array[arraycount],array[arraycount]);
	} else {
		array[arraycount] = NULL;
	}
	array[arraycount+1] = NULL; /* since we have 1 spare entry in the array this is safe to do*/
	DEBUG_MSG("string_to_array, returning %p\n", array);
	g_free(newstring);
	return array;
}

/*this should add 1 empty entry to the configuration list */
GList *make_config_list_item(GList * config_list, void *pointer_to_var, unsigned char type_of_var, gchar * name_of_var)
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
	return (GList *) g_list_append(config_list, config_list_item);
}

static void init_prop_integer(GList ** config_list, void *pointer_to_var, gchar * name_of_var, gint default_value)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 'i', name_of_var);
	*(gint *) pointer_to_var = default_value;
}

static void init_prop_string(GList ** config_list, void *pointer_to_var, gchar * name_of_var, gchar * default_value)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 's', name_of_var);
	*(gchar **) pointer_to_var = g_strdup(default_value);
	DEBUG_MSG("init_prop_string, name_of_var=%s, default_value=%s\n", name_of_var, default_value);
}

static void init_prop_string_with_escape(GList ** config_list, void *pointer_to_var, gchar * name_of_var, gchar * default_value)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 'e', name_of_var);
	*(gchar **) pointer_to_var = unescapestring(default_value);
	DEBUG_MSG("init_prop_string, name_of_var=%s, default_value=%s\n", name_of_var, default_value);
}

static void init_prop_stringlist(GList ** config_list, void *pointer_to_var, gchar * name_of_var)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 'l', name_of_var);
	pointer_to_var = NULL;
}


static void init_prop_arraylist(GList ** config_list, void *pointer_to_var, gchar * name_of_var)
{
	*config_list = make_config_list_item(*config_list, pointer_to_var, 'a', name_of_var);
	pointer_to_var = NULL;
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
				tmpstring2 = escapestring((gchar*)*(void**)tmpitem->pointer, '\0');
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
				tmpstring2 = array_to_string((char **) tmplist2->data, ':');
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
	gchar *tmpstring = NULL, *tmpstring2;
	gchar **tmparray;
	GList *rclist, *tmplist, *tmplist2;
	Tconfig_list_item *tmpitem;

	DEBUG_MSG("parse_config_file, started\n");

	rclist = NULL;
	rclist = get_stringlist(filename, rclist);
	
	if (rclist == NULL) {
		return FALSE;
	}

	/* empty all variables that have type GList ('l') */
	tmplist = g_list_first(config_list);
	while (tmplist != NULL) {
		tmpitem = (Tconfig_list_item *) tmplist->data;
		DEBUG_MSG("parse_config_file, type=%c, identifier=%s\n", tmpitem->type, tmpitem->identifier);
		if (tmpitem->type == 'l' || tmpitem->type == 'a') {
			DEBUG_MSG("parse_config_file, freeing list before filling it\n");
			free_stringlist((GList *) * (void **) tmpitem->pointer);
			(GList *) * (void **) tmpitem->pointer = NULL;
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
						(char *) *(void **) tmpitem->pointer = realloc((char *) *(void **) tmpitem->pointer, strlen(tmpstring) + 1);
						strcpy((char *) *(void **) tmpitem->pointer, tmpstring);
						break;
					case 'e':
						tmpstring2 = unescapestring(tmpstring);
						(char *) *(void **) tmpitem->pointer = realloc((char *) *(void **) tmpitem->pointer, strlen(tmpstring2) + 1);
						strcpy((char *) *(void **) tmpitem->pointer, tmpstring2);
						g_free(tmpstring2);
						break;
					case 'l':
						tmpstring2 = g_strdup(tmpstring);
						(GList *) * (void **) tmpitem->pointer = g_list_prepend((GList *) * (void **) tmpitem->pointer, tmpstring2);
						DEBUG_MSG("parse_config_file, *(void **)tmpitem->pointer=%p\n", *(void **) tmpitem->pointer);
						break;
					case 'a':
						tmparray = string_to_array(tmpstring, ':');
						(GList *) * (void **) tmpitem->pointer = g_list_prepend((GList *) * (void **) tmpitem->pointer, tmparray);
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
	return TRUE;
}

static GList *props_init_main(GList * config_rc)
{
/* these are used in the gtk-2 port already */
	init_prop_integer   (&config_rc, &main_v->props.view_html_toolbar, "view_html_toolbar:", 1);
	init_prop_integer   (&config_rc, &main_v->props.view_custom_menu, "view_custom_menu:", 1);
	init_prop_integer   (&config_rc, &main_v->props.view_main_toolbar, "view_main_toolbar:", 1);
	init_prop_integer   (&config_rc, &main_v->props.view_left_panel, "view_left_panel:", 1);
	init_prop_integer   (&config_rc, &main_v->props.filebrowser_show_hidden_files, "fb_show_hidden_f:", 0);
	init_prop_integer   (&config_rc, &main_v->props.filebrowser_show_others_files, "fb_show_others_f:", 0);
	init_prop_integer   (&config_rc, &main_v->props.filebrowser_show_backup_files, "fb_show_backup_f:", 0);
	init_prop_string    (&config_rc, &main_v->props.filebrowser_unknown_icon, "fb_unknown_icon:", PKGDATADIR"icon_unknown.png");
	init_prop_string    (&config_rc, &main_v->props.filebrowser_dir_icon, "fb_dir_icon:", PKGDATADIR"icon_dir.png");
	init_prop_string    (&config_rc, &main_v->props.editor_font_string, "editor_font_string:", "courier 11");
	init_prop_integer   (&config_rc, &main_v->props.editor_tab_width, "editor_tab_width:", 3);
	init_prop_string    (&config_rc, &main_v->props.tab_font_string, "tab_font_string:", "helvetica 8");
	init_prop_arraylist (&config_rc, &main_v->props.browsers, "browsers:");
	init_prop_arraylist (&config_rc, &main_v->props.external_commands, "external_commands:");
	init_prop_stringlist(&config_rc, &main_v->props.quickbar_items, "quickbar_items:");
	init_prop_integer   (&config_rc, &main_v->props.highlight_num_lines_count, "highlight_num_lines_count:", 1);
	init_prop_arraylist (&config_rc, &main_v->props.filetypes, "filetypes:");
	init_prop_arraylist (&config_rc, &main_v->props.filefilters, "filefilters:");
	init_prop_string    (&config_rc, &main_v->props.last_filefilter, "last_filefilter:", "");
	init_prop_integer   (&config_rc, &main_v->props.transient_htdialogs, "transient_htdialogs:", 1);
	init_prop_integer   (&config_rc, &main_v->props.restore_dimensions, "restore_dimensions:", 1);	
	init_prop_integer   (&config_rc, &main_v->props.left_panel_width, "left_panel_width:", 150);	
	init_prop_integer   (&config_rc, &main_v->props.main_window_h, "main_window_height:", 400);
	init_prop_integer   (&config_rc, &main_v->props.main_window_w, "main_window_width:", 600);
	init_prop_integer   (&config_rc, &main_v->props.max_recent_files, "max_recent_files:", 15);
	init_prop_integer   (&config_rc, &main_v->props.max_dir_history, "max_dir_history:", 10);
	init_prop_integer   (&config_rc, &main_v->props.backup_file,"backup_file:",1);
	init_prop_string    (&config_rc, &main_v->props.backup_filestring,"backup_filestring:","~");
	init_prop_string    (&config_rc, &main_v->props.backup_abort_style,"backup_abort_style:","ask");
	init_prop_string    (&config_rc, &main_v->props.image_thumbnailstring, "image_thumbnailstring:", "_thumbnail");
	init_prop_string    (&config_rc, &main_v->props.image_thumbnailtype, "image_thumbnailtype:", "png");
	init_prop_integer   (&config_rc, &main_v->props.image_thumbnailsizing_type,"image_thumbnailsizing_type:",0);
	init_prop_integer   (&config_rc, &main_v->props.image_thumbnailsizing_val1,"image_thumbnailsizing_val1:",100);
	init_prop_integer   (&config_rc, &main_v->props.image_thumbnailsizing_val2,"image_thumbnailsizing_val2:",100);
	init_prop_string    (&config_rc, &main_v->props.image_thumnailformatstring,"image_thumnailformatstring:","<a href=\"%r\"><img src=\"%t\" width=\"%x\" height=\"%y\" border=\"0\"></a>");
	init_prop_integer   (&config_rc, &main_v->props.allow_multi_instances,"allow_multi_instances:",0);
	init_prop_integer   (&config_rc, &main_v->props.num_undo_levels,"num_undo_levels:",100);
	init_prop_integer   (&config_rc, &main_v->props.clear_undo_on_save,"clear_undo_on_save:",0);

	/* not yet in use */

	init_prop_string(&config_rc, &main_v->props.image_editor_cline, "image_editor_command:", "gimp-remote -n \"%s\"&");
	init_prop_string(&config_rc, &main_v->props.cfg_tab_pos, "notebook_tab_position:", "bottom");
/*	init_prop_string(&config_rc, &main_v->props.cfg_weblint_cline, "weblint_command:", WEBLINT_COMMAND);*/
	init_prop_integer(&config_rc, &main_v->props.full_p, "closing_paragraph_tag:", 1);
	init_prop_integer(&config_rc, &main_v->props.full_li, "closing_list_item_tag:", 1);
	init_prop_integer(&config_rc, &main_v->props.allow_css, "allow_the_use_of_css:", 1);
	init_prop_integer(&config_rc, &main_v->props.allow_dep, "allow_the_use_of_font:", 1);
	init_prop_integer(&config_rc, &main_v->props.format_by_context, "format_by_context:", 1);
	init_prop_integer(&config_rc, &main_v->props.xhtml, "use_xhtml:", 0);
	init_prop_integer(&config_rc, &main_v->props.allow_ruby, "allow_the_use_of_ruby:", 0);
	init_prop_integer(&config_rc, &main_v->props.allow_h4, "allow_the_use_of_html4:", 1);
	init_prop_integer(&config_rc, &main_v->props.allow_frames, "allow_the_use_of_frames:", 1);
	init_prop_integer(&config_rc, &main_v->props.force_dtd, "force_an_dtd:", 0);
	init_prop_integer(&config_rc, &main_v->props.dtd_url, "url_in_dtd:", 0);
	init_prop_integer(&config_rc, &main_v->props.xml_start, "xml_starting_line:", 0);
	init_prop_integer(&config_rc, &main_v->props.lowercase_tags, "lowercase_tags:", 1);
	init_prop_integer(&config_rc, &main_v->props.word_wrap, "word_wrap:", 0);
	init_prop_integer(&config_rc, &main_v->props.autoindent, "autoindent:", 1);
	init_prop_integer(&config_rc, &main_v->props.fontset, "fontset:", 0);
	init_prop_integer(&config_rc, &main_v->props.force_def_style, "force_white_background:", 1);
	init_prop_integer(&config_rc, &main_v->props.drop_at_drop_pos, "drop_at_drop_position:", 0);
	init_prop_integer(&config_rc, &main_v->props.link_management, "link_management:", 1);
	init_prop_integer(&config_rc, &main_v->props.defaulthighlight, "defaulthighlight:", 1);
	init_prop_integer(&config_rc, &main_v->props.cont_highlight_full, "cont_highlight_full:", 0);
	init_prop_integer(&config_rc, &main_v->props.cont_highlight_update, "continuous_highlight_update:", 1);
	init_prop_string(&config_rc, &main_v->props.html_ver, "used_html_version:", "HTML 4.0 Transitional");
	/* key conversion */
	init_prop_integer (&config_rc, &main_v->props.conv_ctrl_enter,"conv_ctrl_enter:",0);
	init_prop_integer (&config_rc, &main_v->props.conv_shift_enter,"conv_shift_enter:",0);
	init_prop_string_with_escape (&config_rc, &main_v->props.shift_enter_text,"shift_enter_text:","<br>\\n");
	init_prop_string_with_escape (&config_rc, &main_v->props.ctrl_enter_text,"ctrl_enter_text:","\\n</p>\\n<p>");
	init_prop_integer (&config_rc, &main_v->props.conv_special_char,"conv_special_char:",0);
#ifdef WITH_MSG_QUEUE
	init_prop_integer (&config_rc, &main_v->props.open_in_running_bluefish,"open_in_running_bluefish:",1);
#endif
	return config_rc;
}

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
		gchar **arr;
		arr = array_from_arglist("Galeon", "galeon %s&",NULL);
		main_v->props.browsers = g_list_append(main_v->props.browsers,arr);
		arr = array_from_arglist("Mozilla", "mozilla -remote 'openURL(%s, new-window)' || mozilla %s&",NULL);
		main_v->props.browsers = g_list_append(main_v->props.browsers,arr);
		arr = array_from_arglist("Opera", "opera -remote 'openURL(%s,new-window)' || opera %s&",NULL);
		main_v->props.browsers = g_list_append(main_v->props.browsers,arr);
		arr = array_from_arglist("Netscape", "/usr/lib/netscape/477/communicator/communicator-smotif %s&",NULL);
		main_v->props.browsers = g_list_append(main_v->props.browsers,arr);
		arr = array_from_arglist("Gnome default", "gnome-moz-remote --newwin %s&",NULL);
		main_v->props.browsers = g_list_append(main_v->props.browsers,arr);
	}
	if (main_v->props.external_commands == NULL) {
		gchar **arr;
		arr = array_from_arglist("Java Compiler", "xterm -e sh -c \"javac %s;read\"&",NULL);
		main_v->props.external_commands = g_list_append(main_v->props.external_commands,arr);
		arr = array_from_arglist("Make", "xterm -e sh -c \"make;read\"&",NULL);
		main_v->props.external_commands = g_list_append(main_v->props.external_commands,arr);
		arr = array_from_arglist("Dos2Unix", "cat %s | dos2unix > %f",NULL);
		main_v->props.external_commands = g_list_append(main_v->props.external_commands,arr);
	}
	if (main_v->props.filetypes == NULL) {
		gchar **arr;
		/* the default file types */
		arr = array_from_arglist("php", ".php:.php4:.inc:.php3", " <>'\"/?$\t-{}[]{}\n;", PKGDATADIR"icon_php.png", NULL);
		main_v->props.filetypes = g_list_append(main_v->props.filetypes, arr);
		arr = array_from_arglist("html", ".html:.htm:.shtml:.shtm", "<> \n\"", PKGDATADIR"icon_html.png", NULL);
		main_v->props.filetypes = g_list_append(main_v->props.filetypes, arr);
		arr = array_from_arglist("javascript", ".js", "\n'\" ", "", NULL);
		main_v->props.filetypes = g_list_append(main_v->props.filetypes, arr);
		arr = array_from_arglist("xml", ".xml", "<> \n\"", PKGDATADIR"icon_xml.png", NULL);
		main_v->props.filetypes = g_list_append(main_v->props.filetypes, arr);
		arr = array_from_arglist("java", ".java:.jar:.class", "(){}'[]\n\" ", PKGDATADIR"icon_java.png", NULL);
		main_v->props.filetypes = g_list_append(main_v->props.filetypes, arr);
		arr = array_from_arglist("sql", ".sql", "(){}'[]\n\" ", "", NULL);
		main_v->props.filetypes = g_list_append(main_v->props.filetypes, arr);
		arr = array_from_arglist("c", ".c:.h", "(){}'[]\n\" ", PKGDATADIR"icon_c.png", NULL);
		main_v->props.filetypes = g_list_append(main_v->props.filetypes, arr);
		arr = array_from_arglist("image", ".jpg:.png:.gif:.jpeg:.tif:.tiff:.xpm:.xcf", "", PKGDATADIR"icon_image.png", NULL);
		main_v->props.filetypes = g_list_append(main_v->props.filetypes, arr);
		arr = array_from_arglist("webimage", ".jpg:.png:.gif:.jpeg", "", PKGDATADIR"icon_image.png", NULL);
		main_v->props.filetypes = g_list_append(main_v->props.filetypes, arr);
		arr = array_from_arglist("stylesheet", ".css", "", "", NULL);
		main_v->props.filetypes = g_list_append(main_v->props.filetypes, arr);
		arr = array_from_arglist("objectfile", ".o:.class", "", "", NULL);
		main_v->props.filetypes = g_list_append(main_v->props.filetypes, arr);
		arr = array_from_arglist("python", ".py", "", "", NULL);
		main_v->props.filetypes = g_list_append(main_v->props.filetypes, arr);
		arr = array_from_arglist("perl", ".pl", "", "", NULL);
		main_v->props.filetypes = g_list_append(main_v->props.filetypes, arr);
	}
	if (main_v->props.filefilters == NULL) {
		gchar **arr;
		arr = array_from_arglist("C programming","1","c:image", NULL);
		main_v->props.filefilters = g_list_append(main_v->props.filefilters, arr);
		arr = array_from_arglist("All web files","1", "html:php:webimage:xml:javascript:stylesheet", NULL);
		main_v->props.filefilters = g_list_append(main_v->props.filefilters, arr);
		arr = array_from_arglist("Java programming","1", "java:image", NULL);
		main_v->props.filefilters = g_list_append(main_v->props.filefilters, arr);
		arr = array_from_arglist("Images","1", "image", NULL);
		main_v->props.filefilters = g_list_append(main_v->props.filefilters, arr);
		arr = array_from_arglist("Hide objectfiles","0", "objectfile", NULL);
		main_v->props.filefilters = g_list_append(main_v->props.filefilters, arr);
	}
}

static gint rcfile_save_main(void) {
	gchar *filename = g_strconcat(g_get_home_dir(), "/.bluefish/rcfile_v2", NULL);
	return save_config_file(main_configlist, filename);
}

void rcfile_parse_highlighting(void) {
	gchar *filename;

	DEBUG_MSG("rcfile_parse_highlighting, started\n");

	highlighting_configlist = NULL;
	init_prop_arraylist(&highlighting_configlist, &main_v->props.highlight_patterns, "patterns:");
	init_prop_arraylist(&highlighting_configlist, &main_v->props.highlight_styles, "styles:");
	
	filename = g_strconcat(g_get_home_dir(), "/.bluefish/highlighting", NULL);
	if (!parse_config_file(highlighting_configlist, filename)) {
		/* init the highlighting in some way? */
		hl_reset_to_default();
		DEBUG_MSG("rcfile_parse_highlighting, about to save highlighting configlist\n");
		save_config_file(highlighting_configlist, filename);
		DEBUG_MSG("rcfile_parse_highlighting, done saving\n");
	}
	g_free(filename);
}
static gint rcfile_save_highlighting(void) {
	gchar *filename = g_strconcat(g_get_home_dir(), "/.bluefish/highlighting", NULL);
	return save_config_file(highlighting_configlist, filename);
}



void rcfile_parse_custom_menu(void) {
	gchar *filename;

	DEBUG_MSG("rcfile_parse_custom_menu, started\n");

	custom_menu_configlist = NULL;
	init_prop_arraylist(&custom_menu_configlist, &main_v->props.cust_menu, "custom_menu:");

	filename = g_strconcat(g_get_home_dir(), "/.bluefish/custom_menu", NULL);
	if (!parse_config_file(custom_menu_configlist, filename)) {
		/* init the custom_menu in some way? */
/*		custom_menu_reset_to_default();
		DEBUG_MSG("rcfile_parse_custom_menu, about to save highlighting configlist\n");
		save_config_file(custom_menu_configlist, filename);
		DEBUG_MSG("rcfile_parse_custom_menu, done saving\n");*/
	}
	g_free(filename);
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
	case 3:
		g_print("saving of  shortcut keys not yet implemented\n");
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
}
