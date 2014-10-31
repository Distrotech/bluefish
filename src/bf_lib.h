/* Bluefish HTML Editor
 * bf_lib.h - non-GUI general functions
 *
 * Copyright (C) 2000-2004 Olivier Sessink
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

#ifndef __BF_LIB_H_
#define __BF_LIB_H_

#include <time.h> /* for time_t */

typedef struct {
	gint my_int;
	gchar *my_char;
} Tconvert_table;

typedef enum {
	tcc2i_full_match = 0,
	tcc2i_firstchar,
	tcc2i_mycharlen,
	tcc2i_full_match_gettext
} Ttcc2i_mode;

typedef struct {
	gpointer data;
	gint count;
} Trefcpointer;

/* #define REFP_DEBUG */
Trefcpointer *refcpointer_new(gpointer data);
#ifdef REFP_DEBUG
void refcpointer_ref(Trefcpointer *rp);
#else
#define refcpointer_ref(rp) rp->count++
#endif
void refcpointer_unref(Trefcpointer *rp);

/*GFile *add_suffix_to_uri(GFile *file, const char *suffix);*/
/*const gchar *full_path_utf8_from_uri(GFile *uri);*/
/*gchar *filename_utf8_from_uri(GFile *uri);*/
/*gchar *get_utf8filename_from_on_disk_encoding(const gchar *encodedname);*/

gchar *mime_with_extension(const gchar *mimetype, const gchar *filename);

GList *urilist_to_stringlist(GList *urilist);
void free_urilist(GList *urilist);
#if GTK_CHECK_VERSION(2,14,0)
#else
GFile *gtk_file_chooser_get_file(GtkFileChooser *chooser);
#endif
/*#define user_bfdir(file) g_strconcat(g_get_home_dir(), "/."PACKAGE"/", file, NULL)*/
GFile *user_bfdir(const gchar *filename);

gchar *filename_utf8_from_full_path_utf8(const gchar *full_path_utf8);

gchar *get_filename_on_disk_encoding(const gchar *utf8filename);

gboolean string_is_color(const gchar *color);

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
gchar *filemode_to_string(mode_t statmode);
/*gchar *return_root_with_protocol(const gchar *url);*/
void pointer_switch_addresses(gpointer *a, gpointer *b);
void list_switch_order(GList *first, GList *second);
gboolean file_copy(gchar *source, gchar *dest);
/*gint find_common_prefixlen_in_stringlist(GList *stringlist);*/
/*guint countchars(const gchar *string, const gchar *chars);*/
gint table_convert_char2int(Tconvert_table *table, const gchar *my_char, Ttcc2i_mode mode);
gchar *table_convert_int2char(Tconvert_table *table, gint my_int);
gchar *expand_string(const gchar *string, const char specialchar, Tconvert_table *table);
gchar *unexpand_string(const gchar *original, const char specialchar, Tconvert_table *table);
gchar *replace_string_printflike(const gchar *string, Tconvert_table *table);
gchar *unescape_string(const gchar *original, gboolean escape_colon);
gchar *escape_string(const gchar *original, gboolean escape_colon);
Tconvert_table *new_convert_table(gint size, gboolean fill_standardescape);
void free_convert_table(Tconvert_table *tct);

#define utf8_byteoffset_to_charsoffset(string,byteoffset) g_utf8_pointer_to_offset(string, string+byteoffset)
/*glong utf8_byteoffset_to_charsoffset(gchar *string, glong byteoffset);*/
void utf8_offset_cache_reset();
guint utf8_charoffset_to_byteoffset_cached(const gchar *string, guint charoffset);
guint utf8_byteoffset_to_charsoffset_cached(const gchar *string, glong byteoffset);

gchar *strip_any_whitespace(gchar *string);
gchar *trunc_on_char(gchar * string, gchar which_char);
/*gchar *strip_common_path(char *to_filename, char *from_filename);*/
gchar *most_efficient_filename(gchar *filename);
gchar *create_relative_link_to(const gchar * current_filepath, const gchar * link_to_filepath);

gchar *strip_trailing_slash(gchar *input);
gchar *ending_slash(const gchar *dirname);
/*gchar *path_get_dirname_with_ending_slash(const gchar *filename);*/
/*gboolean full_path_exists(const gchar *full_path);*/
/*gboolean file_exists_and_readable(const gchar * filename);*/
GFile *return_first_existing_filename(const gchar * filename, ...);
/*gboolean filename_test_extensions(gchar **extensions, const gchar *filename);*/
gchar *bf_str_repeat(const gchar * str, gint number_of);
gint get_int_from_string(const gchar *string);
gchar *unique_path(const gchar *basedir, const gchar *prefix, GHashTable *excludehash);
gchar *create_secure_dir_return_filename(void);
/*void remove_secure_dir_and_filename(gchar *filename);*/
/*gchar *buf_replace_char(gchar *buf, gint len, gchar srcchar, gchar destchar);*/
void wordcount(gchar *text, guint *chars, guint *lines, guint *words);
/*GSList *gslist_from_glist_reversed(GList *src);*/
GList *glist_from_gslist(GSList *src);
gchar *bf_portable_time(const time_t *timep);

gboolean arr3_equal (gconstpointer v1,gconstpointer v2);
guint arr3_hash(gconstpointer v);
gchar *gfile_display_name(GFile *uri,GFileInfo *finfo);
gboolean gfile_uri_is_parent(GFile *parent, GFile *child, gboolean recursive);
gchar *get_hostname_from_uri(GFile *uri);

typedef struct {
	void (*func)();
	gpointer data;
} Tcallback;
void callback_register(GSList **slist, void (*func)(), gpointer data);
void callback_remove_by_data(GSList **slist, gpointer data);
void callback_remove_all(GSList **slist);

#define BF_ELIST_HEAD gpointer next, prev
typedef struct {
	BF_ELIST_HEAD;
} Telist;
#define BF_ELIST(var) ((Telist *)var)
#define bf_elist_next(var) (var)->next
#define bf_elist_prev(var) (var)->prev

Telist * bf_elist_prepend(gpointer cur, gpointer new);
Telist * bf_elist_append(gpointer cur, gpointer new);
Telist * bf_elist_first(gpointer cur);
Telist * bf_elist_last(gpointer cur);
Telist * bf_elist_remove(gpointer toremove);

void print_character_escaped(gunichar uc);

#endif /* __BF_LIB_H_ */

