/* Bluefish HTML Editor
 * bf_lib.h - non-GUI general functions
 *
 * Copyright (C) 2000 Olivier Sessink
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

#ifndef __BF_LIB_H_
#define __BF_LIB_H_

typedef struct {
	gint my_int;
	gchar *my_char;
} Tconvert_table;

void list_switch_order(GList *first, GList *second);
gboolean file_copy(gchar *source, gchar *dest);
gboolean append_string_to_file(gchar *filename, gchar *string);
gint table_convert_char2int(Tconvert_table *table, gchar *my_char);
gchar *table_convert_int2char(Tconvert_table *table, gint my_int);
gchar *replace_string_printflike(const gchar *string, Tconvert_table *table);

#define utf8_byteoffset_to_charsoffset(string,byteoffset) g_utf8_pointer_to_offset(string, string+byteoffset)
/*glong utf8_byteoffset_to_charsoffset(gchar *string, glong byteoffset);*/
void utf8_offset_cache_reset();
guint utf8_byteoffset_to_charsoffset_cached(gchar *string, glong byteoffset);
gchar *escapestring(gchar *original, gchar delimiter);
gchar *unescapestring(gchar *original);
gboolean change_dir(gchar * filename);
gchar *strip_any_whitespace(gchar *string);
gchar *trunc_on_char(gchar * string, gchar which_char);
gchar *strip_common_path(char *to_filename, char *from_filename);
gchar *most_efficient_filename(gchar *filename);
gchar *create_relative_link_to(gchar * current_filepath, gchar * link_to_filepath);
gchar *create_full_path(gchar * filename, gchar *basedir);
gchar *ending_slash(const gchar *dirname);
gchar *path_get_dirname_with_ending_slash(const gchar *filename);
gboolean file_exists_and_readable(const gchar * filename);
gchar *return_first_existing_filename(const gchar * filename, ...);
gboolean filename_test_extensions(gchar **extensions, gchar *filename);
gchar *bf_str_repeat(const gchar * str, gint number_of);
gint get_int_from_string(gchar *string);
gchar *create_secure_dir_return_filename();
void remove_secure_dir_and_filename(gchar *filename);
gchar *buf_replace_char(gchar *buf, gint len, gchar srcchar, gchar destchar);
#endif /* __BF_LIB_H_ */
