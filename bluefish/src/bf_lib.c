/* Bluefish HTML Editor
 * bf_lib.c - non-GUI general functions
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

#include <gtk/gtk.h>
#include <unistd.h> /* chdir() */
#include <stdio.h> /* fopen() */
#include <ctype.h> /* toupper */
#include <string.h> /* strrchr strncmp memmove strncat*/
#include <sys/stat.h> /* S_IFDIR */
#include <errno.h> 	/* errno */

#include "bluefish.h"  /* for DEBUG_MSG and stuff like that */
#include "bf_lib.h"  /* myself */

#ifdef WIN32
#define DIRSTR "\\"
#define DIRCHR 92
#else
#define DIRSTR "/"
#define DIRCHR '/'
#endif

gboolean file_copy(gchar *source, gchar *dest) {
#ifdef DEVELOPMENT
	g_assert(source);
	g_assert(dest);
#endif
	int c;
	FILE *in, *out;
	
	in = fopen(source, "r");
	if (!in) {
		return FALSE;
	}
	out = fopen(dest, "w");
	if (!out) {
		fclose(in);
		return FALSE;
	}
	while((c=fgetc(in)) != EOF) {
		fputc(c,out);
	}
	fclose(in);
	fclose(out);
	return TRUE;
}

gboolean append_string_to_file(gchar *filename, gchar *string) {
	FILE *out;

	out = fopen(filename, "a");
	if (!out) {
		DEBUG_MSG("append_to_file, could not open file %s for append\n", filename);
		return FALSE;
	}
	fputs(string, out);
	fclose(out);
	return TRUE;
}

gint table_convert_char2int(Tconvert_table *table, gchar *my_char) {
	Tconvert_table *entry;
	entry = table;
	while (entry->my_char) {
		if (strcmp(entry->my_char, my_char) == 0) {
			return entry->my_int;
		}
		entry++;
	}
	return -1;
}
/* 
 * WARNING: does NOT malloc any memory, do not free!!
 */
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

gchar *replace_string_printflike(const gchar *string, Tconvert_table *table) {
	gchar *p, *prev, *stringdup;
	gchar *tmp, *dest = g_strdup("");

	stringdup = g_strdup(string); /* we make a copy so we can set some \0 chars in the string */
	prev = stringdup;
	DEBUG_MSG("replace_string_printflike, string='%s'\n", string);
	p = strchr(prev, '%');
	while (p) {
		gchar *converted;
		tmp = dest;
		*p = '\0'; /* set a \0 at this point, the pointer prev now contains everything up to the current % */
		DEBUG_MSG("replace_string_printflike, prev='%s'\n", prev);
		p++;
		if (*p == '%') {
			converted = "%";
		} else {
			converted = table_convert_int2char(table, *p);
		}
		DEBUG_MSG("replace_string_printflike, converted='%s'\n", converted);
		dest = g_strconcat(dest, prev, converted, NULL);
		g_free(tmp);
		prev = ++p;
		p = strchr(p, '%');
	}
	tmp = dest;
	dest = g_strconcat(dest, prev, NULL); /* append the end to the current string */
	g_free(tmp);
	
	g_free(stringdup);
	DEBUG_MSG("replace_string_printflike, dest='%s'\n", dest);
	return dest;
}

#ifdef __GNUC__
__inline__ 
#endif
glong utf8_byteoffset_to_charsoffset(gchar *string, glong byteoffset) {
	return g_utf8_pointer_to_offset(string, &string[byteoffset]);
}

/* static guint countchars(gchar *string, gchar *chars)
 * counts the amount of times the chars in chars are present in string */
static guint countchars(gchar *string, gchar *chars) {
	guint count=0;
	gchar *newstr = strpbrk(string, chars);
	while(newstr) {
		count++;
		newstr = strpbrk(++newstr, chars);
	}
	return count;
}

/* gchar *escapestring(gchar *original, gchar delimiter)
 * returns a newly allocated string which is backslash escaped */
gchar *escapestring(gchar *original, gchar delimiter)
{
	gchar *tmp, *newstring, *escapedchars;
	guint newsize, pos=0;

	/* count the size of the new string */
	escapedchars = g_strdup_printf("\n\\%c", delimiter);
	DEBUG_MSG("escapestring, escapedchars=%s, extra length=%d\n", escapedchars, countchars(original, escapedchars));
	newsize = countchars(original, escapedchars) + strlen(original) + 1;
	newstring = g_malloc0(newsize * sizeof(gchar));
	g_free(escapedchars);
	DEBUG_MSG("escapestring, original=%s, newsize = %d\n", original, newsize);

	tmp = original;
	while (*tmp != '\0') {
		switch (*tmp) {
		case '\\':
			strcat(newstring, "\\\\");
			pos +=2;
		break;
		case '\n':
			strcat(newstring, "\\n");
			pos +=2;
		break;
		default:
			if (*tmp == delimiter) {
				newstring[pos] = '\\';
				newstring[pos+1] = delimiter;
				pos +=2;			
			} else {
				newstring[pos] = *tmp;
				pos++;
			}
		break;
		}
		newstring[pos] = '\0';
		tmp++;
	}
	DEBUG_MSG("escapestring, newstring = %s\n", newstring);
	return newstring;
}

/* gchar *unescapestring(gchar *original)
 * returns a newly allocated string which is backslash un-escaped */
gchar *unescapestring(gchar *original)
{
	gchar *tmp1, *tmp2, *newstring;
	guint newsize;
	gint escaped;

	/* note: to split string into an array,
	 * use string_to_array().
	 */

	newsize = strlen(original) + 1;
	newstring = g_malloc0(newsize * sizeof(gchar));
	DEBUG_MSG("unescapestring, original=%s, newsize = %d\n", original, newsize);

	tmp1 = original;
	tmp2 = newstring;
	escaped = 0;
	while (*tmp1 != '\0') {
		if (escaped) {
			switch (*tmp1) {
			case '\\':
				*tmp2++ = '\\';
			break;
			case 'n':
				*tmp2++ = '\n';
			break;
			default:
				*tmp2++ = *tmp1;
			break;
			}

			escaped = 0;
		} else {
			if (*tmp1 == '\\')
				escaped = 1;
			else
				*tmp2++ = *tmp1;
		}
		tmp1++;
	}
	DEBUG_MSG("unescapestring, newstring = %s\n", newstring);
	return newstring;
}


/* gboolean change_dir(gchar * filename)
 * changes the current working directory to the directory of filename */
gboolean change_dir(gchar * filename)
{
	gchar *tmpstring;
	gboolean returncode;

	if (!filename)
		return FALSE;
	tmpstring = g_dirname(filename);
	DEBUG_MSG("change_dir, filename = %s, tmpstring = %s in %p\n", filename, tmpstring, tmpstring);
	returncode = chdir(tmpstring) == 0 ? TRUE : FALSE;
	g_free(tmpstring);
	DEBUG_MSG("change_dir, after gfree(tmpstring) \n");
	return returncode;
};

/* gchar *strip_any_whitespace(gchar *string)
 * strips any double chars defined by isspace() from the string, 
 * only single spaces are returned */
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


/* gchar *trunc_on_char(gchar * string, gchar which_char)
 *      string - text string
 *      which_char      - termination character
 * Returns a pointer to the same string which is truncated at the first
 * occurence of which_char */
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
};

/* gchar *strip_common_path(char *image_fn, char *html_fn)
 * returns a newly allocated string containing the the to_filename
 * but all the common path with from_filename is removed 
 *
 * IS THIS IN USE ?? */ 
gchar *strip_common_path(char *to_filename, char *from_filename)
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

	/* contributed by Tony Kemp <tony.kemp@studentmail.newcastle.edu.au> */
	while (to_filename[count - 1] != DIRCHR)
		count--;


	DEBUG_MSG("strip_common_path, equal count = %d\n", count);
	count2 = strlen(to_filename);
	tempstr = g_malloc(count2 - count + 2);
	memcpy(tempstr, &to_filename[count], count2 - count + 2);
	DEBUG_MSG("strip_common_path, tempstr= %s, should be %d long\n", tempstr, count2 - count);
	return tempstr;
	/* the function that calls this one must use gfree() somewhere!! */
}


/* gchar *most_efficient_filename(gchar *filename)
 *      filename: the string which will be modified to remove dir/../
 * Returns the same string, but modified , 
 * not wanted dir/../ combinations are removed */
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


/* gchar *create_relative_link_to(gchar * current_filepath, gchar * link_to_filepath)
 * 	current_filepath: the file to link FROM, a full path
 * 	link_to_filepath: the file to link TO, a full path
 * Returns a newly allocated string containing a relative path */
gchar *create_relative_link_to(gchar * current_filepath, gchar * link_to_filepath)
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

	/* get the size of the directory    of the current_filename */

	
	current_filename_length = strlen(strrchr(eff_current_filepath, DIRCHR))-1;
	link_to_filename_length = strlen(strrchr(eff_link_to_filepath, DIRCHR))-1;
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
		while ((*ltmp != DIRCHR) && (common_lenght > 0)) {
			common_lenght--;
			ltmp--;
		}
	}
	DEBUG_MSG("create_relative_link_to, common_lenght=%d (checked for directory)\n", common_lenght);

	/* now we need to count how much deeper (in directories) the current_filename
	   is compared to the link_to_file, that is the amount of ../ we need to add */
	deeper_dirs = 0;
	for (count = common_lenght+1; count <= current_dirname_length; count++) {
		if (eff_current_filepath[count] == DIRCHR) {
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


/* gchar *create_full_path(gchar * filename, gchar *basedir)
 * 	filename: the relative filename
 * 	basedir: a dir or file in the dir used as basedir for the filename
 * 		if this is NULL the current working directory is used
 * Returns a newly allocated string containing a absolute path */
gchar *create_full_path(gchar * filename, gchar *basedir)
{
	gchar *absolute_filename;
	gchar *tmpcdir;

	DEBUG_MSG("create_full_path, filename=%s, basedir=%p\n", filename, basedir);
	if (g_path_is_absolute(filename)) {
		absolute_filename = g_strdup(filename);
	} else {
		if (basedir) {
			tmpcdir = g_dirname(basedir);
		} else {
			tmpcdir = g_get_current_dir();
		}
		absolute_filename = g_strconcat(tmpcdir, DIRSTR, filename, NULL);
		g_free(tmpcdir);
	}
	absolute_filename = most_efficient_filename(absolute_filename);
	return absolute_filename;
}



/* gchar *ending_slash(const gchar *dirname)
 * returns a newly allocated string which is 
 * guaranteed to end on a slash (/) */
gchar *ending_slash(const gchar *dirname) {
	/* result should be free-ed !!! */
	if (!dirname) {
		return g_strdup("");
	}

	if (dirname[strlen(dirname)-1] == DIRCHR) {
		return g_strdup(dirname);
	} else {
		return g_strconcat(dirname, DIRSTR, NULL);
	}
}

/* gint file_is_dir(gchar * filename)
 * returns 1 if the file pointed to by filename is a dir
 * else returns 0 */
gint file_is_dir(gchar * filename) {
	return g_file_test(filename, G_FILE_TEST_IS_DIR);
}

/* gint file_exists_and_readable(gchar * filename)
 * returns 1 if the file pointed to by filename does exist
 * and is readable for the current process */
gint file_exists_and_readable(gchar * filename)
{
	struct stat naamstat;

#ifdef DEVELOPMENT
	g_assert(filename);
#endif

	if (strlen(filename) < 2) {
		DEBUG_MSG("file_exists_and_readable, strlen(filename) < 2!!!!\n");
		return 0;
	}
	DEBUG_MSG("file_exists_and_readable, filename(%p)=\"%s\", strlen(filename)=%d\n", filename, filename, strlen(filename));
#ifndef WIN32
	if ((stat(filename, &naamstat) == -1) && (errno == ENOENT)) {
/*              if(naamstat.st_mode & S_IREAD) { */
		return (0);
/*              }
   return(0); */
	} else {
		return (1);
	}
#else
	return (1);
#endif
}

/*  gboolean filename_test_extensions(gchar **extensions, gchar *filename)
 *  give a NULL terminated array of strings with all extensions
 *  returns TRUE if the filename has one of those extensions
 *  returns FALSE if not
 */
gboolean filename_test_extensions(gchar **extensions, gchar *filename) {
	if (!extensions) {
		return FALSE;
	}
	while (*extensions) {
		if (strncmp(&filename[strlen(filename)-strlen(*extensions)], *extensions, strlen(*extensions)) == 0 ) {
			return TRUE;
		}
		extensions++;
	}
	return FALSE;
}

/*
 * gchar *bf_str_repeat(const gchar * str, gint number_of)
 *      str - pointer to string
 *      number_of - number copies of string
 * returns:     pointer to new allocated string
 *      calculate, allocate buffer and copies number_of times str to new buffer
 */
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

/* gint get_int_from_string(gchar *string)
 * tries to find a positive integer value in the string and returns it
 * returns -1 on failure
 */
gint get_int_from_string(gchar *string) {
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
/* gchar *create_secure_dir_return_filename()
 * this function uses mkdir(name) to create a dir with permissions rwx------
 * mkdir will fail if name already exists or is a symlink or something
 * the name is chosen by tempnam() so the chance that mkdir() fails in
 * a normal situation is minimal, it almost must be a hacking attempt
 *
 * the filename generated can safely be used for output of an external 
 * script
 */
gchar *create_secure_dir_return_filename() {
	gchar *name, *name2;
	DEBUG_MSG("create_secure_dir_return_filename,g_get_tmp_dir()=%s\n", g_get_tmp_dir());
	name = tempnam(g_get_tmp_dir(), NULL);
	DEBUG_MSG("create_secure_dir_return_filename, name=%s\n", name);
	if (!name) {
		return NULL;
	}

	if (mkdir(name, 0700) != 0) {
		g_free(name);
		return NULL;
	}
	name2 = tempnam(name, NULL);
	DEBUG_MSG("create_secure_dir_return_filename, name2=%s\n", name2);
	g_free(name);
	return name2;
}

void remove_secure_dir_and_filename(gchar *filename) {
	gchar *dirname = g_dirname(filename);
	unlink(filename);
	rmdir(dirname);
	g_free(dirname);
}

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
