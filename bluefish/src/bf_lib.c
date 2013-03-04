/* Bluefish HTML Editor
 * bf_lib.c - non-GUI general functions
 *
 * Copyright (C) 2000-20010 Olivier Sessink
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

/*#define DEBUG */

#include "config.h"

/* this is needed for Solaris to comply with the latest POSIX standard 
 * regarding the ctime_r() function
 */
#ifdef PLATFORM_SOLARIS
#define __EXTENSIONS__
#define _POSIX_C_SOURCE 200312L
#endif

#include <gtk/gtk.h>
#include <glib/gstdio.h>	/* g_mkdir */
#include <ctype.h>     /* toupper */
#include <errno.h>     /* errno */
#include <stdio.h>     /* fopen(), tempnam() */
#include <string.h>    /* strrchr strncmp memmove strncat*/
#include <sys/stat.h>  /* S_IFDIR */
#include <time.h>      /* ctime_r() */
#include <unistd.h>    /* chdir() */

#include "bluefish.h"  /* for DEBUG_MSG and stuff like that */
#include "bf_lib.h"    /* myself */

#ifdef DEVELOPMENT
#include <stdlib.h>
#endif

/*#define REFP_REFS*/
#ifdef REFP_REFS
	gint num_refp_refs=0;
#endif

#ifdef REFP_DEBUG
void refcpointer_ref(Trefcpointer *rp) {
	rp->count++;
	g_print("refcpointer_ref, %p refcount=%d\n",rp, rp->count);
}
#endif
Trefcpointer *refcpointer_new(gpointer data) {
	Trefcpointer *rp = g_slice_new(Trefcpointer);
#ifdef REFP_REFS
	num_refp_refs++;
	g_print("num_refp_refs=%d\n",num_refp_refs);
#endif
	rp->data = data;
	rp->count = 1;
#ifdef REFP_DEBUG
	g_print("refcpointer_new, created %p with refcount 1\n",rp);
#endif
	return rp;
}

void refcpointer_unref(Trefcpointer *rp) {
	rp->count--;
#ifdef REFP_DEBUG
	g_print("refcpointer_unref, %p refcount=%d %s\n",rp, rp->count, (rp->count ==0 ? "freeing data" : ""));
#endif
	if (G_UNLIKELY(rp->count <= 0)) {
#ifdef REFP_REFS
		num_refp_refs--;
		g_print("num_refp_refs=%d\n",num_refp_refs);
#endif
		g_free(rp->data);
		g_slice_free(Trefcpointer,rp);
	}
}
/*GFile *add_suffix_to_uri(GFile *file, const char *suffix) {
	if (!suffix) {
		g_object_ref(file);
		return file;
	} else {
		gchar *tmp, *tmp2;
		GFile *retval;
		tmp = g_file_get_parse_name(file);
		tmp2 =  g_strconcat(tmp,suffix,NULL);
		retval = g_file_parse_name(tmp2);
		g_free(tmp);
		g_free(tmp2);
		return retval;
	}
}*/
GList *urilist_to_stringlist(GList *urilist) {
	GList *retlist=NULL, *tmplist = g_list_last(urilist);
	while (tmplist) {
		retlist = g_list_prepend(retlist, g_file_get_parse_name((GFile *)tmplist->data));
		tmplist = g_list_previous(tmplist);
	}
	return retlist;
}

void free_urilist(GList *urilist) {
	GList *tmplist = g_list_first(urilist);
	while (tmplist) {
		g_object_unref((GObject *)tmplist->data);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(urilist);
}

#if GTK_CHECK_VERSION(2,14,0)

#else
GFile *gtk_file_chooser_get_file(GtkFileChooser *chooser) {
	GFile *file=NULL;
	gchar *uri = gtk_file_chooser_get_uri(chooser);
	if (uri) {
		file = g_file_new_for_uri(uri);
		g_free(uri);
	}
	return file;
}
#endif

GFile *user_bfdir(const gchar *filename) {
	GFile *file;
	gchar *path = g_strconcat(g_get_home_dir(), "/."PACKAGE"/", filename, NULL);
	file = g_file_new_for_path(path);
	g_free(path);
	return file;
}
/*const gchar *full_path_utf8_from_uri(GFile *uri) {

	GFileInfo* ginfo;
	GError *error=NULL;
	const gchar *utf8uri; 
	ginfo = g_file_query_info(uri, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME, G_FILE_QUERY_INFO_NONE, NULL, &error);
	utf8uri = g_file_info_get_display_name(ginfo);
	g_object_unref(ginfo);
	return utf8uri;
}

gchar *filename_utf8_from_uri(GFile *uri) {
	gchar *utf8name, *slashpos;
	
	utf8name = full_path_utf8_from_uri(uri);
	slashpos = strrchr(utf8name,'/');
	if (slashpos) {
		gchar *tmp;
		slashpos++;
		tmp = g_strdup(slashpos);
		g_free(utf8name);
		return tmp;
	}
	return utf8name;
}*/

/*gchar *get_utf8filename_from_on_disk_encoding(const gchar *encodedname) {
	if (encodedname) {
		GError *gerror=NULL;
		gsize b_written;
		gchar *ondiskencoding = g_filename_to_utf8(encodedname,-1, NULL,&b_written,&gerror);
		if (gerror) {
			g_print(_("Bluefish has trouble reading the filenames. Try to set the environment variable G_BROKEN_FILENAMES=1\n"));
			ondiskencoding = g_strdup(encodedname);
		}
		DEBUG_MSG("get_utf8filename_from_on_disk_encoding, utf8filename=%s\n",ondiskencoding);
		return ondiskencoding;
	}
	return NULL;
}*/

gboolean string_is_color(const gchar *color) {
	GdkColor gcolor;
	return gdk_color_parse(color, &gcolor);
}

static void fill_rwx(short unsigned int bits, char *chars) {
	chars[0] = (bits & S_IRUSR) ? 'r' : '-';
	chars[1] = (bits & S_IWUSR) ? 'w' : '-';
	chars[2] = (bits & S_IXUSR) ? 'x' : '-';
}
static void fill_setid(short unsigned int bits, char *chars) {
#ifdef S_ISUID
	if (bits & S_ISUID) {
		/* Set-uid, but not executable by owner.  */
		if (chars[3] != 'x') chars[3] = 'S';
		else chars[3] = 's';
	}
#endif
#ifdef S_ISGID
	if (bits & S_ISGID) {
		/* Set-gid, but not executable by group.  */
		if (chars[6] != 'x') chars[6] = 'S';
		else chars[6] = 's';
	}
#endif
#ifdef S_ISVTX
	if (bits & S_ISVTX) {
		/* Sticky, but not executable by others.  */
		if (chars[9] != 'x') chars[9] = 'T';
		else chars[9] = 't';
	}
#endif
}
gchar *filemode_to_string(mode_t statmode) {
	gchar *str = g_malloc0(10);
 	/* following code "adapted" from GNU filemode.c program */
	fill_rwx((statmode & 0700) << 0, &str[0]);
	fill_rwx((statmode & 0070) << 3, &str[3]);
	fill_rwx((statmode & 0007) << 6, &str[6]);
	fill_setid(statmode, str);
	return str;
}


/**
 * return_root_with_protocol:
 * @url: #const gchar* with the url 
 *
 * returns the root of the url, including its trailing slash
 * this might be in the form
 * - "protocol://server:port/"
 * - "/"
 * - NULL, if the url contains no url, nor does it start with a / character
 *
 * if there is no trailing slash, this function will return the root WITH a
 * trailing slash appended!!
 *
 * Return value: #gchar* newly allocated, or NULL
 * /
gchar *return_root_with_protocol(const gchar *url) {
	gchar *q;
	if (!url) return NULL;
	q = strchr(url,':');
	if (q && *(q+1)=='/' && *(q+2)=='/' && *(q+3)!='\0') {
		/ * we have a protocol * /
		gchar *root = strchr(q+3, '/');
		if (root) return g_strndup(url, root - url + 1);
		/ * if there is no third slash character, we probably
		have an url like http://someserver so we will append 
		the slash ourselves * /
		return g_strconcat(url, "/",NULL);
	} else if (url[0] == '/') {
		/ * no protocol, return / * /
		return g_strdup("/");
	}
	/ * no root known * /
	return NULL;
}
*/

gchar *mime_with_extension(const gchar *mimetype, const gchar *filename) {
	gchar *tmp;
	tmp = strrchr(filename, '.');
	if (tmp) {
		return g_strconcat(mimetype, "?", tmp + 1, NULL);
	} else {
		return g_strdup(mimetype);
	}
}

/**
 * pointer_switch_addresses:
 * a: #gpointer;
 * b: #gpointer
 *
 * after this call, a will contain the address previously in a
 * and b will contain the address previously in b
 *
 * Return value: void
 */
#ifdef __GNUC__
__inline__ 
#endif
void pointer_switch_addresses(gpointer *a, gpointer *b) {
	gpointer c;
	DEBUG_MSG("pointer_switch_addresses, before, a=%p, b=%p\n",a,b);
	c = *a;
	*a = *b;
	*b = c;
	DEBUG_MSG("pointer_switch_addresses, after, a=%p, b=%p\n",a,b);
}

/**
 * list_switch_order:
 * @first: a #GList * item
 * @second: a #GList * item
 * 
 * this function will switch place of these two list items
 * actually not the items themselves, but the data they are 
 * pointer to is switched
 * 
 * Return value: void
 **/
void list_switch_order(GList *first, GList *second) {
	gpointer tmp;
	tmp = first->data;
	first->data = second->data;
	second->data = tmp;
}

/*static gint length_common_prefix(gchar *first, gchar *second) {
	gint i=0;
	while (first[i] == second[i] && first[i] != '\0') {
		i++;
	}
	return i;
}*/
/**
 * find_common_prefix_in_stringlist:
 * @stringlist: a #GList* with strings
 * 
 * tests every string in stringlist, and returns the length of the 
 * common prefix all these strings have
 *
 * This is for example useful to find out if a list of filenames
 * share the same base directory
 * 
 * Return value: #gint with number of common characters
 **/
/*gint find_common_prefixlen_in_stringlist(GList *stringlist) {
	gchar *firststring;
	gint commonlen;
	GList *tmplist;
	tmplist = g_list_first(stringlist);
	firststring = (gchar *)tmplist->data;
	commonlen = strlen(firststring);
	tmplist = g_list_next(tmplist);
	while(tmplist){
		gint testlen;
		gchar *secondstring = (gchar *)tmplist->data;
		testlen = length_common_prefix(firststring, secondstring);
		if (testlen < commonlen) {
			commonlen = testlen;
		}
		tmplist = g_list_next(tmplist);
	}
	return commonlen;
}*/
/**
 * countchars:
 * @string: a gchar * to count the chars in
 * @chars: a gchar * with the characters you are interested in
 *
 * this function will count every character in string that is also in chars
 * 
 * Return value: guint with the number of characters found
 **/
static guint countchars(const gchar *string, const gchar *chars) {
	guint count=0;
	gchar *newstr = strpbrk(string, chars);
	while(newstr) {
		count++;
		newstr = strpbrk(++newstr, chars);
	}
	DEBUG_MSG("countchars, returning %d\n",count);
	return count;
}
static gint table_convert_char2int_backend(Tconvert_table *table, const gchar *my_char
		, Ttcc2i_mode mode, int (*func)(const gchar *arg1, const gchar *arg2) ) {
	Tconvert_table *entry;
	entry = table;
	while (entry->my_char) {
		if (func(my_char,entry->my_char)==0) {
			return entry->my_int;
		}
		entry++;
	}
	return -1;
}
static int strfirstchar(const gchar *mychar, const gchar *tablechar) {
	return mychar[0] - tablechar[0];
}
static int strmycharlen(const gchar *mychar, const gchar *tablechar) {
	return strncmp(mychar,tablechar,strlen(mychar));
}
static int strfull_match_gettext(const gchar *mychar, const gchar *tablechar) {
	return strcmp(mychar,_(tablechar));
}
/**
 * table_convert_char2int:
 * @table: a #tconvert_table * with strings and integers
 * @my_char: a #gchar * containing the string to convert
 * @mode: #Ttcc2i_mode
 * 
 * this function can be used to translate a string from some set (in table)
 * to an integer
 * 
 * Return value: gint, found in table, or -1 if not found
 **/
gint table_convert_char2int(Tconvert_table *table, const gchar *my_char, Ttcc2i_mode mode) {
	switch (mode) {
	case tcc2i_firstchar:
		return table_convert_char2int_backend(table,my_char,mode,strfirstchar);
	case tcc2i_mycharlen:
		return table_convert_char2int_backend(table,my_char,mode,strmycharlen);
	case tcc2i_full_match:
		return table_convert_char2int_backend(table,my_char,mode,strcmp);
	case tcc2i_full_match_gettext:
		return table_convert_char2int_backend(table,my_char,mode,strfull_match_gettext);
	default:
		DEBUG_MSG("bug in call to table_convert_char2int()\n");
		return -1;
	}
}
/**
 * table_convert_int2char:
 * @table: a #tconvert_table * with strings and integers
 * @my_int: a #gint containing the integer to convert
 * 
 * this function can be used to translate an integer from some set (in table)
 * to a string
 * WARNING: This function will return a pointer into table, it will 
 * NOT allocate new memory
 * 
 * Return value: gchar * found in table, else NULL
 **/
gchar *table_convert_int2char(Tconvert_table *table, gint my_int) {
	Tconvert_table *entry;
	entry = table;
	while (entry->my_char) {
		if (my_int == entry->my_int) {
			return entry->my_char;
		}
		entry++;
	}
	return NULL;
}
/**
 * expand_string:
 * @string: a formatstring #gchar * to convert
 * @specialchar: a const char to use as 'delimited' or 'special character'
 * @table: a #Tconvert_table * array to use for conversion
 * 
 * this function can convert a format string with %0, %1, or \n, \t 
 * into the final string, where each %number or \char entry is replaced 
 * with the string found in table
 *
 * so this function is the backend for unescape_string() and 
 * for replace_string_printflike()
 *
 * table is an array with last entry {0, NULL}
 * 
 * Return value: a newly allocated gchar * with the resulting string
 **/
gchar *expand_string(const gchar *string, const char specialchar, Tconvert_table *table) {
	gchar *p, *prev, *stringdup;
	gchar *tmp, *dest = g_strdup("");

	stringdup = g_strdup(string); /* we make a copy so we can set some \0 chars in the string */
	prev = stringdup;
	DEBUG_MSG("expand_string, string='%s'\n", string);
	p = strchr(prev, specialchar);
	while (p) {
		gchar *converted;
		tmp = dest;
		*p = '\0'; /* set a \0 at this point, the pointer prev now contains everything up to the current % */
		DEBUG_MSG("expand_string, prev='%s'\n", prev);
		p++;
		converted = table_convert_int2char(table, *p);
		DEBUG_MSG("expand_string, converted='%s'\n", converted);
		dest = g_strconcat(dest, prev, converted, NULL);
		g_free(tmp);
		prev = ++p;
		p = strchr(p, specialchar);
	}
	tmp = dest;
	dest = g_strconcat(dest, prev, NULL); /* append the end to the current string */
	g_free(tmp);
	
	g_free(stringdup);
	DEBUG_MSG("expand_string, dest='%s'\n", dest);
	return dest;
}
gchar *replace_string_printflike(const gchar *string, Tconvert_table *table) {
	return expand_string(string,'%',table);
}

static gint tablesize(Tconvert_table *table) {
	Tconvert_table *tmpentry = table;
	while (tmpentry->my_char) tmpentry++;
	return (tmpentry - table);
}
/* for now this function can only unexpand strings with tables that contain only
single character strings like "\n", "\t" etc. */
gchar *unexpand_string(const gchar *original, const char specialchar, Tconvert_table *table) {
	gchar *tmp, *tosearchfor, *retval, *prev, *dest, *orig;
	Tconvert_table *tmpentry;
	
	orig = g_strdup(original);
	DEBUG_MSG("original='%s', strlen()=%zd\n",original,strlen(original));
	tosearchfor = g_malloc(tablesize(table)+1);
	DEBUG_MSG("tablesize(table)=%d, alloc'ed %d bytes for tosearchfor\n",tablesize(table), tablesize(table)+1);
	tmp = tosearchfor;
	tmpentry = table;
	while(tmpentry->my_char != NULL) {
		*tmp = tmpentry->my_char[0]; /* we fill the search string with the first character */
		tmpentry++;
		tmp++;
	}
	*tmp = '\0';
	DEBUG_MSG("unexpand_string, tosearchfor='%s'\n",tosearchfor);
	DEBUG_MSG("alloc'ing %zd bytes\n", (countchars(original, tosearchfor) + strlen(original) + 1));
	retval = g_malloc((countchars(original, tosearchfor) + strlen(original) + 1) * sizeof(gchar));
	dest = retval;
	prev = orig;
	/* now we go trough the original till we hit specialchar */
	tmp = strpbrk(prev, tosearchfor);
	while (tmp) {
		gint len = tmp - prev;
		gint mychar = table_convert_char2int(table, tmp, tcc2i_firstchar);
		DEBUG_MSG("unexpand_string, tmp='%s', prev='%s'\n",tmp, prev);
		if (mychar == -1) mychar = *tmp;
		DEBUG_MSG("unexpand_string, copy %d bytes and advancing dest\n",len);
		memcpy(dest, prev, len);
		dest += len;
		*dest = specialchar;
		dest++;
		*dest = mychar;
		dest++;
		prev=tmp+1;
		DEBUG_MSG("prev now is '%s'\n",prev);
		tmp = strpbrk(prev, tosearchfor);
	}
	DEBUG_MSG("unexpand_string, copy the rest (%s) to dest\n",prev);
	memcpy(dest,prev,strlen(prev)+1); /* this will also make sure there is a \0 at the end */
	DEBUG_MSG("unexpand_string, retval='%s'\n",retval);
	g_free(orig);
	g_free(tosearchfor);
	return retval;
}
/* if you change this table, please change escape_string() and unescape_string() and new_convert_table() in the same way */
static Tconvert_table standardescapetable [] = {
	{'n', "\n"},
	{'t', "\t"},
	{'\\', "\\"},
	{'f', "\f"},
	{'r', "\r"},
	{'a', "\a"},
	{'b', "\b"},
	{'v', "\v"},
	{'n', "\n"},
	{':', ":"}, /* this double entry is there to make unescape_string and escape_string work efficient */
	{0, NULL}
};
gchar *unescape_string(const gchar *original, gboolean escape_colon) {
	gchar *string, *tmp=NULL;
	DEBUG_MSG("unescape_string, started\n");
	if (!escape_colon) {
		tmp = standardescapetable[9].my_char;
		standardescapetable[9].my_char = NULL;
	}
	string = expand_string(original,'\\',standardescapetable);
	if (!escape_colon) {
		standardescapetable[9].my_char = tmp;
	}
	return string;
}
gchar *escape_string(const gchar *original, gboolean escape_colon) {
	gchar *string, *tmp=NULL;
	DEBUG_MSG("escape_string, started\n");
	if (!escape_colon) {
		tmp = standardescapetable[9].my_char;
		standardescapetable[9].my_char = NULL;
	}
	string = unexpand_string(original,'\\',standardescapetable);
	if (!escape_colon) {
		standardescapetable[9].my_char = tmp;
	}
	return string;
}
Tconvert_table *new_convert_table(gint size, gboolean fill_standardescape) {
	gint realsize = (fill_standardescape) ? size + 10 : size;
	Tconvert_table * tct = g_new(Tconvert_table, realsize+1);
	DEBUG_MSG("new_convert_table, size=%d, realsize=%d,alloced=%d\n",size,realsize,realsize+1);
	if (fill_standardescape) {
		gint i;
		for (i=size;i<realsize;i++) {
			tct[i].my_int = standardescapetable[i-size].my_int;
			tct[i].my_char = g_strdup(standardescapetable[i-size].my_char);
		}
		DEBUG_MSG("new_convert_table, setting tct[%d] (i) to NULL\n",i);
		tct[i].my_char = NULL;
	} else {
		DEBUG_MSG("new_convert_table, setting tct[%d] (size) to NULL\n",size);
		tct[size].my_char = NULL;
	}
	return tct;
}
void free_convert_table(Tconvert_table *tct) {
	Tconvert_table *tmp = tct;
	while (tmp->my_char) {
		DEBUG_MSG("free_convert_table, my_char=%s\n",tmp->my_char);
		g_free(tmp->my_char);
		tmp++;
	}
	DEBUG_MSG("free_convert_table, free table %p\n",tct);
	g_free(tct);
}
/**************************************************/
/* byte offset to UTF8 character offset functions */
/**************************************************/

/*
html files usually have enough cache at size 4
large php files, a cache of 
	10 resulted in 160% of the buffer to be parsed
	12 resulted in 152% of the buffer to be parsed
	14 resulted in 152% of the buffer to be parsed
	16 resulted in 152% of the buffer to be parsed
so we keep it at 12 for the moment
*/
#define UTF8_OFFSET_CACHE_SIZE 12
/*#define UTF8_BYTECHARDEBUG */

typedef struct {
	/* the two arrays must be grouped and in this order, because they are moved back 
	one position in ONE memmove() call */
	guint last_byteoffset[UTF8_OFFSET_CACHE_SIZE];
	guint last_charoffset[UTF8_OFFSET_CACHE_SIZE];
#ifdef UTF8_BYTECHARDEBUG
	guint numcalls_since_reset;
	unsigned long long int numbytes_parsed;
	guint numcalls_cached_since_reset;
	unsigned long long int numbytes_cached_parsed;
	guint num_memmoves;
#endif
#ifdef DEVELOPMENT
	gchar *last_buf;
#endif
} Tutf8_offset_cache;

static Tutf8_offset_cache utf8_offset_cache = {{0,0,0,0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0,0,0,0}};

/**
 * utf8_offset_cache_reset:
 * 
 * this function will reset the utf8 offset cache used by 
 * utf8_byteoffset_to_charsoffset_cached() to use a new buffer
 *
 * Return value: void
 **/
#ifdef __GNUC__
__inline__ 
#endif
void utf8_offset_cache_reset() {
#ifdef UTF8_BYTECHARDEBUG
	g_print("UTF8_BYTECHARDEBUG: called %d times for total %llu bytes\n"
				,utf8_offset_cache.numcalls_since_reset
				,utf8_offset_cache.numbytes_parsed);
	g_print("UTF8_BYTECHARDEBUG: cache HIT %d times, reduced from %llu to %llu bytes (%4.2f%%), cache size %d, num memmoves=%d\n"
				,utf8_offset_cache.numcalls_cached_since_reset
				,utf8_offset_cache.numbytes_parsed
				,utf8_offset_cache.numbytes_cached_parsed
				,((gfloat)utf8_offset_cache.numbytes_cached_parsed/(gfloat)utf8_offset_cache.numbytes_parsed*100.0)
				,UTF8_OFFSET_CACHE_SIZE
				,utf8_offset_cache.num_memmoves);
#endif
	memset(&utf8_offset_cache, 0, sizeof(Tutf8_offset_cache));
}

guint utf8_charoffset_to_byteoffset_cached(const gchar *string, guint charoffset) {
	guint i=UTF8_OFFSET_CACHE_SIZE-1, retval;
	
	if (charoffset ==0) return 0;
	/*g_print("requested charoffset %d\n",charoffset);*/
	while (i > 0 && utf8_offset_cache.last_charoffset[i] > charoffset) {
		i--;
	}
	/*g_print("cache pos %d has charoffset %d and byteoffset %d\n",i,utf8_offset_cache.last_charoffset[i],utf8_offset_cache.last_byteoffset[i]);*/
	if (i > 0) {
		const gchar *tmp;
		if (utf8_offset_cache.last_charoffset[i] == charoffset) {
			return utf8_offset_cache.last_byteoffset[i];
		}
		tmp = string+utf8_offset_cache.last_byteoffset[i];
		retval = (g_utf8_offset_to_pointer(tmp, charoffset-utf8_offset_cache.last_charoffset[i])-tmp)+utf8_offset_cache.last_byteoffset[i];
		/*g_print("non-cached byteoffset %d, cached byteoffset %d for charoffset %d\n",(g_utf8_offset_to_pointer(string, charoffset)-string),retval,charoffset);*/
	} else {
		retval = (g_utf8_offset_to_pointer(string, charoffset)-string);
		/*g_print("non-cached byteoffset %d for charoffset %d\n",retval,charoffset);*/
	}
	if (i == (UTF8_OFFSET_CACHE_SIZE-1)) {
		/* add this new calculation to the cache */
		/* this is a nasty trick to move all guint entries one back in the array, so we can add the new one */
		memmove(&utf8_offset_cache.last_byteoffset[0], &utf8_offset_cache.last_byteoffset[1], (UTF8_OFFSET_CACHE_SIZE+UTF8_OFFSET_CACHE_SIZE-1)*sizeof(guint));
		/*g_print("add byteoffset %d charoffset %d to position %d in the cache\n",retval,charoffset,UTF8_OFFSET_CACHE_SIZE-1);*/
		utf8_offset_cache.last_byteoffset[UTF8_OFFSET_CACHE_SIZE-1] = retval;
		utf8_offset_cache.last_charoffset[UTF8_OFFSET_CACHE_SIZE-1] = charoffset;
	}
	return retval;
}

/**
 * utf8_byteoffset_to_charsoffset_cached:
 * @string: the gchar * you want to count
 * @byteoffset: glong with the byteoffset you want the charoffset for
 * 
 * this function calculates the UTF-8 character offset in a string for
 * a given byte offset
 * It uses caching to speedup multiple calls for the same buffer, the cache
 * is emptied if you change to another buffer. If you use the same buffer but 
 * change it inbetween calls, you have to reset it yourself using
 * the utf8_offset_cache_reset() function
 *
 **** the result is undefined if the provided byteoffset is in the middle of a UTF8 character ***
 * 
 * Return value: guint with character offset
 **/
guint utf8_byteoffset_to_charsoffset_cached(const gchar *string, glong byteoffset) {
	guint retval;
	gint i = UTF8_OFFSET_CACHE_SIZE-1;
	if (byteoffset ==0) return 0;
#ifdef DEVELOPMENT
	if (utf8_offset_cache.last_buf != NULL && string != utf8_offset_cache.last_buf) {
		/*utf8_offset_cache_reset();
		utf8_offset_cache.last_buf = string;*/
		g_print("bug found in a call to utf8_byteoffset_to_charsoffset_cached, the cache was not reset\n");
		exit(156);
	}
#endif
	DEBUG_MSG("utf8_byteoffset_to_charsoffset_cached, string %p has strlen %zd, looking for byteoffset %ld, starting in cache at i=%d\n", string, strlen(string),byteoffset,i);

	while (i > 0 && utf8_offset_cache.last_byteoffset[i] > byteoffset) {
		i--;
	}

	if (i > 0) {
		if (utf8_offset_cache.last_byteoffset[i] == byteoffset) {
			DEBUG_MSG("byteoffset %ld is in the cache at i=%d, returning %d\n",byteoffset,i,utf8_offset_cache.last_charoffset[i]);
			return utf8_offset_cache.last_charoffset[i];
		}
		/* if the byteoffset is in the middle of a multibyte character, this line will fail (but
		we are not supposed to get called in the middle of a character)*/
		retval = g_utf8_pointer_to_offset(string+utf8_offset_cache.last_byteoffset[i], string+byteoffset)+utf8_offset_cache.last_charoffset[i];
#ifdef UTF8_BYTECHARDEBUG
		utf8_offset_cache.numbytes_parsed += byteoffset;
		utf8_offset_cache.numbytes_cached_parsed += (byteoffset - utf8_offset_cache.last_byteoffset[i]);
		utf8_offset_cache.numcalls_cached_since_reset++;
#endif
	} else {
		retval = g_utf8_pointer_to_offset(string, string+byteoffset);
#ifdef UTF8_BYTECHARDEBUG
		utf8_offset_cache.numbytes_parsed += byteoffset;
#endif
	}
	DEBUG_MSG(" and byteoffset %ld has charoffset %d\n",byteoffset,retval);
	if (i == (UTF8_OFFSET_CACHE_SIZE-1)) {
		/* add this new calculation to the cache */
#ifdef UTF8_BYTECHARDEBUG
		utf8_offset_cache.num_memmoves++;
#endif
		/* this is a nasty trick to move all guint entries one back in the array, so we can add the new one */
		memmove(&utf8_offset_cache.last_byteoffset[0], &utf8_offset_cache.last_byteoffset[1], (UTF8_OFFSET_CACHE_SIZE+UTF8_OFFSET_CACHE_SIZE-1)*sizeof(guint));

		utf8_offset_cache.last_byteoffset[UTF8_OFFSET_CACHE_SIZE-1] = byteoffset;
		utf8_offset_cache.last_charoffset[UTF8_OFFSET_CACHE_SIZE-1] = retval;
	}
#ifdef UTF8_BYTECHARDEBUG
	utf8_offset_cache.numcalls_since_reset++;
#endif
	return retval;
}

/**
 * strip_any_whitespace:
 * @string: a gchar * to strip
 *
 * strips any double chars defined by isspace() from the string, 
 * only single spaces are returned
 * the same string is returned, no memory is allocated in this function
 * 
 * Return value: the same gchar * as passed to the function
 **/
/* used only in the htmlbar plugin */
gchar *strip_any_whitespace(gchar *string) {
	gint count=0, len;

#ifdef DEVELOPMENT
	g_assert(string);
#endif

	DEBUG_MSG("strip_any_whitespace, starting string='%s'\n", string);
	len = strlen(string);
	while(string[count]) {
		if (isspace((char)string[count])) {
			string[count] = ' ';
			if (string[count+1] && isspace((char)string[count+1])) {
				gint fcount = count;
				while (string[fcount] && isspace((char)string[fcount])) {
					fcount++;
				}
				DEBUG_MSG("strip_any_whitespace, found %d spaces\n", fcount - count);
				memmove(&string[count+1], &string[fcount], len - (count+1));
				string[len- (fcount-count)+1] = '\0';
				DEBUG_MSG("strip_any_whitespace, after memmove, string='%s'\n", string);
			}
		}
		count++;
	}
	g_strstrip(string);
	DEBUG_MSG("strip_any_whitespace, returning string='%s'\n", string);
	return string;
}
/**
 * trunc_on_char:
 * @string: a #gchar * to truncate
 * @which_char: a #gchar with the char to truncate on
 *
 * Returns a pointer to the same string which is truncated at the first
 * occurence of which_char
 * 
 * Return value: the same gchar * as passed to the function
 **/
gchar *trunc_on_char(gchar * string, gchar which_char)
{
	gchar *tmpchar = string;
	while(*tmpchar) {
		if (*tmpchar == which_char) {
			*tmpchar = '\0';
			return string;
		}
		tmpchar++;
	}
	return string;
}
/* gchar *strip_common_path(char *image_fn, char *html_fn)
 * returns a newly allocated string containing the the to_filename
 * but all the common path with from_filename is removed 
 *
 * IS THIS IN USE ?? OBVIOUSLY NOT BECAUSE I CAN REMOVE IT */ 
/*gchar *strip_common_path(char *to_filename, char *from_filename)
{
	gchar *tempstr;
	int count, count2, dir_length;

	count = 0;
	tempstr = strrchr(to_filename, DIRCHR);
	dir_length = strlen(to_filename) - strlen(tempstr);
	dir_length += 1;
	DEBUG_MSG("strip_common_path, dir_lenght=%d\n", dir_length);
	while ((strncmp(to_filename, from_filename, count + 1)) == 0) {
		count++;
		if (count > dir_length) {
			count = dir_length;
			break;
		}
	}
	while (to_filename[count - 1] != DIRCHR)
		count--;
	DEBUG_MSG("strip_common_path, equal count = %d\n", count);
	count2 = strlen(to_filename);
	tempstr = g_malloc(count2 - count + 2);
	memcpy(tempstr, &to_filename[count], count2 - count + 2);
	DEBUG_MSG("strip_common_path, tempstr= %s, should be %d long\n", tempstr, count2 - count);
	return tempstr;
} */

/**
 * most_efficient_filename:
 * @filename: a gchar * with a possibly inefficient filename like /hello/../tmp/../myfile
 *
 * tries to eliminate any dir/../ combinations in filename
 * this function could do evern better, it should also remove /./ entries
 * 
 * Return value: the same gchar * as passed to the function
 **/
gchar *most_efficient_filename(gchar *filename) {
	gint i,j, len;

	DEBUG_MSG("most_efficient_filename, 1 filename=%s\n", filename);
	len = strlen(filename);
	for (i=0; i < len-4; i++) {
/*		DEBUG_MSG("most_efficient_filename, i=%d\n", i); */
		if (strncmp(&filename[i], "/../", 4) == 0) {
			for (j=1; j < i; j++) {
				if ((filename[i - j] == DIRCHR) || (i==j)) {
					DEBUG_MSG("most_efficient_filename, &filename[%d-%d]=%s, &filename[%d+3]=%s, %d-%d-4=%d\n", i,j,&filename[i-j],i, &filename[i+3], len, j, len-j-4);
					memmove(&filename[i-j], &filename[i+3], len-i);
					j=i;
					i--;
				}
			}
		}
	}
	DEBUG_MSG("most_efficient_filename, 3 filename=%s\n", filename);
	return filename;
}

/**
 * create_relative_link_to:
 * @current_filepath: a #gchar * with the current filename
 * @link_to_filepath: a #gchar * with a file to link to
 *
 * creates a newly allocated relative link from current_filepath 
 * to link_to_filepath
 *
 * if current_filepath == NULL it returns the most efficient filepath 
 * for link_to_filepath
 *
 * if link_to_filepath == NULL it will return NULL
 *
 * Return value: a newly allocated gchar * with the relative link
 **/
gchar *create_relative_link_to(const gchar * current_filepath, const gchar * link_to_filepath)
{
	gchar *returnstring = NULL;
	gchar *eff_current_filepath, *eff_link_to_filepath;
	gint common_lenght, maxcommonlen;
	gint current_filename_length, link_to_filename_length, current_dirname_length, link_to_dirname_length;
	gint count, deeper_dirs;

	if (!current_filepath){
		returnstring = most_efficient_filename(g_strdup(link_to_filepath));
		return returnstring;
	}
	if (!link_to_filepath) {
		return NULL;
	}
	eff_current_filepath = most_efficient_filename(g_strdup(current_filepath));
	eff_link_to_filepath = most_efficient_filename(g_strdup(link_to_filepath));
	DEBUG_MSG("eff_current: '%s'\n",eff_current_filepath);
	DEBUG_MSG("eff_link_to: '%s'\n",eff_link_to_filepath);
	/* get the size of the directory of the current_filename. we work on URI's
	so we always use the / as character */
	current_filename_length = strlen(strrchr(eff_current_filepath, '/'))-1;
	link_to_filename_length = strlen(strrchr(eff_link_to_filepath, '/'))-1;
	DEBUG_MSG("create_relative_link_to, filenames: current: %d, link_to:%d\n", current_filename_length,link_to_filename_length); 
	current_dirname_length = strlen(eff_current_filepath) - current_filename_length;
	link_to_dirname_length = strlen(eff_link_to_filepath) - link_to_filename_length;
	DEBUG_MSG("create_relative_link_to, dir's: current: %d, link_to:%d\n", current_dirname_length, link_to_dirname_length); 

	if (current_dirname_length < link_to_dirname_length) {
		maxcommonlen = current_dirname_length;
	} else {
		maxcommonlen = link_to_dirname_length;
	}
	
	/* first lets get the common basedir for both dir+file  by comparing the
	   common path in the directories */
	common_lenght = 0;
	while ((strncmp(eff_current_filepath, eff_link_to_filepath, common_lenght + 1)) == 0) {
		common_lenght++;
		if (common_lenght >= maxcommonlen) {
			common_lenght = maxcommonlen;
			break;
		}
	}
	DEBUG_MSG("create_relative_link_to, common_lenght=%d (not checked for directory)\n", common_lenght);
	/* this is the common length, but we need the common directories */
	if (eff_current_filepath[common_lenght] != DIRCHR) {
		gchar *ltmp = &eff_current_filepath[common_lenght];
		while ((*ltmp != '/') && (common_lenght > 0)) {
			common_lenght--;
			ltmp--;
		}
	}
	DEBUG_MSG("create_relative_link_to, common_lenght=%d (checked for directory)\n", common_lenght);

	/* now we need to count how much deeper (in directories) the current_filename
	   is compared to the link_to_file, that is the amount of ../ we need to add */
	deeper_dirs = 0;
	for (count = common_lenght+1; count <= current_dirname_length; count++) {
		if (eff_current_filepath[count] == '/') {
			deeper_dirs++;
			DEBUG_MSG("create_relative_link_to, on count=%d, deeper_dirs=%d\n", count, deeper_dirs);
		}
	}
	DEBUG_MSG("create_relative_link_to, deeper_dirs=%d\n", deeper_dirs);

	/* now we know everything we need to know we can create the relative link */
	returnstring = g_malloc0((strlen(link_to_filepath) - common_lenght + 3 * deeper_dirs + 1) * sizeof(gchar *));
	count = deeper_dirs;
	while (count) {
		strcat(returnstring, "../");
		count--;
	}
	strcat(returnstring, &eff_link_to_filepath[common_lenght+1]);
	DEBUG_MSG("create_relative_link_to, returnstring=%s\n", returnstring);
	g_free(eff_current_filepath);
	g_free(eff_link_to_filepath);
	return returnstring;
}

/**
 * strip_trailing_slash:
 * @dirname: a gchar *pointing to a directory
 *
 * Return value: the same string, but 1 char shorter if there was a trailing slash
 */
gchar *strip_trailing_slash(gchar *input) {
	if (input) {
		gint len = strlen(input);
		if (input[len-1] == '/') input[len-1] = '\0';
	}
	return input;
}

/**
 * ending_slash:
 * @dirname: a #const gchar * with a diretory name
 *
 * makes sure the last character of the newly allocated 
 * string it returns is a '/'
 *
 * Return value: a newly allocated gchar * dirname that does end on a '/'
 **/
gchar *ending_slash(const gchar *dirname) {
	if (!dirname) {
		return g_strdup("");
	}

	if (dirname[strlen(dirname)-1] == DIRCHR) {
		return g_strdup(dirname);
	} else {
		return g_strconcat(dirname, DIRSTR, NULL);
	}
}
/**
 * path_get_dirname_with_ending_slash:
 * @filename: a #const gchar * with a file path
 *
 * returns a newly allocated string, containing everything up to 
 * the last '/' character, including that character itself.
 *
 * if no '/' character is found it returns NULL
 *
 * Return value: a newly allocated gchar * dirname that does end on a '/', or NULL on failure
 **/
/*gchar *path_get_dirname_with_ending_slash(const gchar *filename) {
	gchar *tmp = strrchr(filename, DIRCHR);
	if (tmp) {
		return g_strndup(filename, (tmp - filename + 1));
	} else {
		return NULL;
	}
}*/

/**
 * full_path_exists:
 * @full_path: gchar*
 *
 * convenience function that will create a GFile, check if it exists, and unref it again.
 *
 * /
gboolean full_path_exists(const gchar *full_path) {
	gboolean retval;
	GFile *uri;
	uri = g_file_new_for_path(full_path);
	retval = g_file_query_exists(uri,NULL);
	g_object_unref(uri);
	return retval;
}*/

/**
 * return_first_existing_filename:
 * @filename: a #const gchar * with a filename
 * @...: more filenames
 *
 * you can pass multiple filenames to this function, and it will return
 * the first filename that really exists according to file_exists_and_readable()
 *
 * Return value: GFile * with the first filename found
 **/
GFile *return_first_existing_filename(const gchar *filename, ...) {
	va_list args;
	GFile *retval=NULL;

	va_start(args, filename);
	while (filename) {
		DEBUG_MSG("return_first_existing_filename, testing %s\n",filename);
		if (access(filename, R_OK) == 0) {
			retval = g_file_new_for_path(filename);
			break;
		}
		filename = va_arg(args, gchar*);
	}
	va_end(args);
	return retval;
}

/**
 * filename_test_extensions:
 * @extensions: a #gchar ** NULL terminated arrau of strings
 * @filename: a #const gchar * with a filename
 *
 * tests if the filename matches one of the extensions passed in the NULL terminated array
 * of strings
 *
 * Return value: gboolean, TRUE if the file has one of the extensions in the array
 **/
/*gboolean filename_test_extensions(gchar **extensions, const gchar *filename) {
	gint fnlen;
	if (!extensions) {
		return FALSE;
	}
	fnlen = strlen(filename);
	while (*extensions) {
		gint extlen;
		extlen = strlen(*extensions);
		if (fnlen > extlen && strncmp(filename + fnlen - extlen, *extensions, extlen) == 0 ) {
			return TRUE;
		}
		extensions++;
	}
	return FALSE;
}*/

/**
 * bf_str_repeat:
 * @str: a #const gchar * 
 * @number_of: a #gint
 *
 * returns a newly allocated string, 
 * containing str repeated number_of times
 *
 * Return value: the newly allocated #gchar *
 **/
gchar *bf_str_repeat(const gchar * str, gint number_of) {
	gchar *retstr;
	gint len = strlen(str) * number_of;
	retstr = g_malloc(len + 1);
	retstr[0] = '\0';
	while (number_of) {
		strncat(retstr, str, len);
		number_of--;
	};
	return retstr;
}

/**
 * get_int_from_string:
 * @string: a #const gchar * 
 *
 * tries to find a positive integer value in the string and returns it
 * the string does not have to start or end with the integer
 * it returns -1 if no integer was found somewhere in the string
 *
 * Return value: the found #gint, -1 on failure
 **/
gint get_int_from_string(const gchar *string) {
	if (string) {
		gint faktor = 1, result=-1;
		gint i,len = strlen(string);
		for (i=len-1;i>=0;i--) {
			if ((string[i] < 58) && (string[i] > 47)) {
				if (result == -1) {
					result = 0;
				} else {
					faktor *= 10;
				}
				result += (((gint)string[i])-48)*faktor;
				DEBUG_MSG("get_int_from_string, set result to %d\n", result);
			} else {
				if (result !=-1) {
					return result;
				}
			}
		}
		return result;
	}
	return -1;
}

gchar *unique_path(const gchar *basedir, const gchar *prefix, GHashTable *excludehash) {
	gchar *path, *dir;
	gint count=0;
	if (!g_file_test(basedir, G_FILE_TEST_IS_DIR)) {
		g_warning("dir %s does not exist\n",basedir);
		return NULL;
	}
	dir = ending_slash(basedir);
	if (prefix) {
		path = g_strconcat(dir, prefix, NULL);
	} else {
		path = g_strdup_printf("%sbf-%d",dir,count);
		count++;
	}
	while(g_file_test(path,G_FILE_TEST_EXISTS) || (excludehash && g_hash_table_lookup(excludehash, path)!=NULL )) {
		g_free(path);
		if (prefix)
			path = g_strdup_printf("%s%s%d",dir,prefix,count);
		else
			path = g_strdup_printf("%sbf-%d",dir,count);
		count++;
	}
	g_free(dir);
	/*g_print("return %s\n",path);*/
	return path;
}

static gchar *return_securedir(void) {
	if (main_v->securedir) {
		if (access(main_v->securedir, W_OK|X_OK)==0) {
			return main_v->securedir;
		} else {
			g_free(main_v->securedir);
		}
	}
	DEBUG_MSG("return_securedir,g_get_tmp_dir()=%s\n", g_get_tmp_dir());
	/* it is SAFE to use tempnam() here, because we don't open a file with that name,
	 * we create a directory with that name. mkdir() will FAIL if this name is a hardlink
	 * or a symlink, so we DO NOT overwrite any file the link is pointing to
	 */
	main_v->securedir = unique_path(g_get_tmp_dir(), "bluefish", NULL);
	while (g_mkdir(main_v->securedir, 0700) != 0) {
		g_free(main_v->securedir);
		main_v->securedir = unique_path(g_get_tmp_dir(), "bluefish", NULL);
	}
	return main_v->securedir;
}

/**
 * create_secure_dir_return_filename:
 *
 * this function uses mkdir(name) to create a dir with permissions rwx------
 * mkdir will fail if name already exists or is a symlink or something
 * the name is chosen by tempnam() so the chance that mkdir() fails in
 * a normal situation is minimal, it almost must be a hacking attempt
 *
 * the filename generated can safely be used for output of an external 
 * script because the dir has rwx------ permissions
 *
 * Return value: a newly allocated #gchar * containing a temporary filename in a secure dir
 **/
gchar *create_secure_dir_return_filename(void) {
	gchar *name, *name2;
	name = return_securedir();
	DEBUG_MSG("create_secure_dir_return_filename, dir is at %s\n",name);
	name2 = unique_path(name, NULL, NULL);
	DEBUG_MSG("create_secure_dir_return_filename, name2=%s\n", name2);
	return name2;
}
/**
 * remove_secure_dir_and_filename:
 * @filename: the #gchar * filename to remove
 *
 * this function will remove a the filename created 
 * by create_secure_dir_return_filename(), and the safe
 * directory the file was created in
 *
 * Return value: void
 **/
/*void remove_secure_dir_and_filename(gchar *filename) {
	gchar *dirname = g_path_get_dirname(filename);
	unlink(filename);
	rmdir(dirname);
	g_free(dirname);
}*/

/* gchar *buf_replace_char(gchar *buf, gint len, gchar srcchar, gchar destchar) {
	gint curlen=0;
	gchar *tmpbuf=buf;
	while(tmpbuf[curlen] != '\0' && curlen < len) {
		if (tmpbuf[curlen] == srcchar) {
			tmpbuf[curlen] = destchar;
		}
		curlen++;
	}
	return buf;
}*/

/**
 * wordcount:
 * @text: A #gchar* to examine.
 * @chars: #guint*, will contain no. chars in text.
 * @lines: #guint*, will contain no. lines in text.
 * @words: #guint*, will contain no. words in text.
 *
 * Returns number of characters, lines and words in the supplied #gchar*.
 * Handles UTF-8 correctly. Input must be properly encoded UTF-8.
 * Words are defined as any characters grouped, separated with spaces.
 * I.e., this is a word: "12a42a,.". This is two words: ". ."
 *
 * This function contains a switch() with inspiration from the GNU wc utility.
 * Rewritten for glib UTF-8 handling by Christian Tellefsen, chris@tellefsen.net.
 *
 * Note that gchar == char, so that gives us no problems here.
 *
 * Return value: void
 **/
void wordcount(gchar *text, guint *chars, guint *lines, guint *words)
{
	guint in_word = 0;
	gunichar utext;
	
	if(!text) return; /* politely refuse to operate on NULL .. */
		
	*chars = *words = *lines = 0;
	while (*text != '\0') {
		(*chars)++;
		
		switch (*text) {
			case '\n':
				(*lines)++;
				/* Fall through. */
			case '\r':
			case '\f':
			case '\t':
			case ' ':
			case '\v':
				mb_word_separator:
				if (in_word) {
					in_word = 0;
					(*words)++;
				}
				break;
			default:
				utext = g_utf8_get_char_validated(text, 2); /* This might be an utf-8 char..*/
				if (g_unichar_isspace (utext)) /* Unicode encoded space? */
					goto mb_word_separator;
				if (g_unichar_isgraph (utext)) /* Is this something printable? */
					in_word = 1;
				break;
		} /* switch */
		
		text = g_utf8_next_char(text); /* Even if the current char is 2 bytes, this will iterate correctly. */
	}
	
	/* Capture last word, if there's no whitespace at the end of the file. */
	if(in_word) (*words)++;
	/* We start counting line numbers from 1 */
	if(*chars > 0) (*lines)++;
}

/*GSList *gslist_from_glist_reversed(GList *src) {
	GSList *target=NULL;
	GList *tmplist = g_list_first(src);
	while (tmplist) {
		target = g_slist_prepend(target, tmplist->data);
		tmplist = g_list_next(tmplist);
	}
	return target;
}*/

GList *glist_from_gslist(GSList *src) {
	GList *target=NULL;
	GSList *tmplist = src;
	while (tmplist) {
		target = g_list_append(target, tmplist->data);
		tmplist = g_slist_next(tmplist);
	}
	return target;
}
/* returns a newly allocated string! */
gchar *bf_portable_time(const time_t *timep) {
	gchar *retstr=NULL;
#ifdef HAVE_CTIME_R
	retstr = g_new0(gchar, 128);
	ctime_r(timep,retstr);
#else
#ifdef HAVE_ASCTIME_R
	retstr = g_new0(gchar, 128);
	asctime_r(localtime(timep),retstr);
#else
#ifdef HAVE_CTIME
	retstr = g_strdup(ctime(timep));
#else
#ifdef HAVE_ASCTIME
	retstr = g_strdup(asctime(localtime(timep)));
#endif /* HAVE_ASCTIME */
#endif /* HAVE_CTIME */
#endif /* HAVE_ASCTIME_R */
#endif /* HAVE_CTIME_R */
	if (retstr) { /* one or more of the above functions end with a newline */
		int len = strlen(retstr);
		if (retstr[len-1]=='\n')
			retstr[len-1]='\0';
	}
	return retstr;
}

/* these hash functions hash the first 3 strings in a gchar ** */
gboolean arr3_equal (gconstpointer v1,gconstpointer v2)
{
	const gchar **arr1 = (const gchar **)v1;
	const gchar **arr2 = (const gchar **)v2;
  
	return ((strcmp ((char *)arr1[0], (char *)arr2[0]) == 0 )&&
			(strcmp ((char *)arr1[1], (char *)arr2[1]) == 0) &&
			(strcmp ((char *)arr1[2], (char *)arr2[2]) == 0));
}

guint arr3_hash(gconstpointer v)
{
  /* modified after g_str_hash */
  	const signed char **tmp = (const signed char **)v;
	const signed char *p;
	guint32 h = *tmp[0];
	if (h) {
		gshort i=0;
		while (i<3) {
			p = tmp[i];
			for (p += 1; *p != '\0'; p++)
				h = (h << 5) - h + *p;
			i++;
		}
	}
	return h;
}

gchar *gfile_display_name(GFile *uri, GFileInfo *finfo) {
	gchar *retval;
	if (finfo && g_file_info_has_attribute(finfo,G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME)) {
		retval = g_strdup(g_file_info_get_display_name(finfo));
		DEBUG_MSG("gfile_display_name, got %s from finfo\n",retval);
	} else {
		GFileInfo *finfo2;
		
		finfo2 = g_file_query_info(uri,G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,G_FILE_QUERY_INFO_NONE,NULL,NULL);
		retval = g_strdup(g_file_info_get_display_name(finfo2));
		DEBUG_MSG("gfile_display_name, queried uri %p, and got %s from finfo %p\n",uri,retval,finfo2);
		g_object_unref(finfo2);
	}
	return retval;
}

gboolean gfile_uri_is_parent(GFile *parent, GFile *child, gboolean recursive) {
	gboolean retval = FALSE;
	GFile *tmp, *tmp2;

	if (parent == NULL) {
		DEBUG_MSG("gfile_uri_is_parent, parent=NULL ??\n");
		return FALSE;
	} 
	if (child == NULL) {
		DEBUG_MSG("gfile_uri_is_parent, child=NULL ??\n");
		return FALSE;
	}
	g_object_ref(child);
	tmp2=child;
	do {
		tmp = g_file_get_parent(tmp2);
		g_object_unref(tmp2);
		if (tmp == NULL) {
			break;
		}
		retval = g_file_equal(tmp,parent);
		tmp2 = tmp;
	} while (recursive == TRUE && retval != TRUE);
	if (tmp) {
		g_object_unref(tmp);
	}
	return retval;
}

gchar *get_hostname_from_uri(GFile *uri) {
	gchar *tmp, *begin, *end1, *end2, *end, *retval;
	
	tmp = g_file_get_uri(uri);
	begin = strstr(tmp, "://");
	begin += 3;
	end1 = strchr(begin, ':');
	end2 = strchr(begin, '/');
	end = (end1 < end2) ? end1 : end2;
	retval = g_strndup(begin, (end-begin));
	g_free(tmp); 
	return retval;
}

void
callback_register(GSList **slist, void (*func)(), gpointer data) {
	Tcallback *cb;
	cb = g_slice_new0(Tcallback);
	cb->func = func;
	cb->data = data;
	*slist = g_slist_append(*slist, cb);	
}

void
callback_remove_by_data(GSList **slist, gpointer data) {
	Tcallback *cb=NULL;
	GSList *tmpslist=*slist;
	while(tmpslist) {
		cb=tmpslist->data;
		if (cb->data == data) {
			*slist = g_slist_delete_link(*slist, tmpslist);
			g_slice_free(Tcallback, cb);
			return;
		}
		tmpslist = g_slist_next(tmpslist);
	}
}

void
callback_remove_all(GSList **slist) {
	GSList *tmpslist=*slist;
	while(tmpslist) {
		g_slice_free(Tcallback, tmpslist->data);
		tmpslist = g_slist_next(tmpslist);
	}
	g_slist_free(*slist);
	*slist=NULL;
}

#ifndef HAVE_STRCASESTR
#include <ctype.h>
char *strcasestr(char *a, char *b)
{
	size_t l;
	char f[3];

	snprintf(f, sizeof(f), "%c%c", tolower(*b), toupper(*b));
	for (l = strcspn(a, f); l != strlen(a); l += strcspn(a + l + 1, f) + 1)
		if (strncasecmp(a + l, b, strlen(b)) == 0)
			return(a + l);
	return(NULL);
}
#endif

