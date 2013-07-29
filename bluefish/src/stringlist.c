/* Bluefish HTML Editor
 * stringlist.c - functions that deal with stringlists
 *
 * Copyright (C) 1999-2010 Olivier Sessink
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

/* #define DEBUG */

/*
 * This module deals with the GUI dialog used in bluefish to manage
 * GLists in which each element is composed of several strings,
 * like the lists of syntax highlighting, external filters, external commands,
 * and lately doctypes. For example, the list of doctypes contains a string
 * for the DOCTYPE and a string for the FILENAME of that doctype.
 *
 * When the stringlist is edited, the old one is deleted and a new one is created,
 * so the order can be different from the original.
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>  	/* strspn() */
#include <strings.h>  	/* strcasecmp() */
#include "bluefish.h"
#include "bf_lib.h"

/*#include "coloursel.h"  color_but_new() */
#include "stringlist.h"
#include "gtk_easy.h"

#define STRING_MAX_SIZE 4096
#define MAX_ARRAY_LENGTH 20

/************************************************************************/
/************************************************************************/

/**
 * array_to_string:
 * @array: #gchar** with NULL terminated array
 * @delimiter: #gchar with the delimiter character
 *
 * Converts a NULL terminated array to a string with a delimiter
 * between the entries, and some characters backslash escaped
 * like \t, \n, \\ and the delimiter. In Bluefish the : is the most 
 * used delimiter
 *
 * Return value: #gchar* newly allocated string
 */
gchar *array_to_string(gchar **array) {
	gchar **tmp, *escaped1, *finalstring;
	gint newsize=1;
	gsize allocsize=64;
	g_return_val_if_fail(array,g_strdup(""));
	finalstring = g_malloc0(allocsize);
	tmp = array;
	while(*tmp) {
		/*DEBUG_MSG("array_to_string, *tmp = %s\n", *tmp);*/
		escaped1 = escape_string(*tmp, TRUE);
		newsize += strlen(escaped1)+1;
		if (newsize > allocsize) {
			allocsize = MAX(newsize, (allocsize * 2));
			finalstring = g_realloc(finalstring, allocsize);
		}
		strcat(finalstring, escaped1);
		finalstring[newsize-2] = ':';
		finalstring[newsize-1] = '\0';
		g_free(escaped1);
		tmp++;
	}
	DEBUG_MSG("array_to_string, len(finalstring) = %zd\n", strlen(finalstring));
	return finalstring;
}

#define ARRAYBLOCKSIZE 8
#define BUFBLOCKSIZE 64
/**
 * string_to_array:
 * @string: #gchar* with the string to convert
 * @delimiter: #gchar with the delimiter character
 *
 * breaks the string apart into a NULL terminated array
 * using the delimiter character. \t, \\ and \n are also unescaped
 * to their original characters
 *
 * Return value: #gchar** newly allocated NULL terminated array
 */
gchar **string_to_array(gchar *string) {
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
			tmpchar2 = tmpchar+1;
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
			case 't':
				newstring[count] = '\t';
				tmpchar++;
			break;
			case ':':
				newstring[count] = ':';
				tmpchar++;
			break;
			default:
				DEBUG_MSG("string_to_array, weird, an unescaped backslash ?\n");
				newstring[count] = '\\';
			break;
			}
		} else if (*tmpchar == ':') {
			newstring[count] = '\0';  /* end of the current newstring */
			DEBUG_MSG("string_to_array, newstring(%p)=%s\n", newstring, newstring);
			array[arraycount] = g_strdup(newstring);
			DEBUG_MSG("string_to_array, found delimiter, arraycount=%d, result(%p)=%s\n",arraycount, array[arraycount], array[arraycount]);
			arraycount++;
			if (arraycount == arraysize-2) { /* we need 1 spare entry in the array */
				arraysize += ARRAYBLOCKSIZE;  /* and arraysize starts at 1, arraycount at 0 */
				DEBUG_MSG("string_to_array, arraycount=%d, about to increase arraysize to %d, sizeof(array(%p))=%ld\n", arraycount, arraysize, array, sizeof(&array));
				array = g_realloc(array, arraysize * sizeof(char *));
				DEBUG_MSG("string_to_array, arraysize=%d, array(%p), sizeof(array)=%ld\n", arraysize, array, sizeof(&array));
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
			DEBUG_MSG("string_to_array, newstringsize=%d, sizeof(newstring(%p))=%ld\n", newstringsize, newstring, sizeof(newstring));
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

/**
 * array_from_arglist:
 * @...: #gchar* with the first string, NULL terminated
 *
 * builds a NULL terminated array from the argumentlist to this function
 *
 * Return value: #gchar** with the array
 */
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

gchar **prepend_array(gchar *prepend, gchar **arr)
{
	gint len, i;
	gchar **new;
	
	len = g_strv_length(arr);
	new = g_new(gchar *, len + 2);
	new[0] = g_strdup(prepend);
	for (i=0;i<=len;i++) {
		new[i+1] = g_strdup(arr[i]);
	}
	new[i]=NULL;
	return new;
}

/**
 * array_from_arglist:
 * @allocate_strings: #gboolean if the strings should be newly allocated
 * @...: #gchar* with the first string, NULL terminated
 *
 * builds a GList with strings (a stringlist), copied by reference or by content
 *
 * Return value: GList *
 */
GList *list_from_arglist(gboolean allocate_strings, ...) {
	GList *retval=NULL;
	va_list args;
	gchar *s;
	va_start(args, allocate_strings);
	s = va_arg(args, gchar*);
	while (s) {
		retval = g_list_append(retval, allocate_strings ? g_strdup(s) : s);
		s = va_arg (args, gchar*);
	}
	va_end (args);
	return retval;
}

GList *duplicate_stringlist(GList *list, gint dup_data) {
	GList *retlist=NULL;
	if (list) {
		GList *tmplist;
		tmplist = g_list_last(list);
		while (tmplist) {
			if (tmplist->data) {
				gchar *data;
				if (dup_data) {
					data = g_strdup((gchar *)tmplist->data);
				} else {
					data = (gchar *)tmplist->data;
				}
				retlist = g_list_prepend(retlist, data);
			}
			tmplist = g_list_previous(tmplist);
		}
	}
	return retlist;
}

gint free_stringlist(GList * which_list)
{
	GList *tmplist;

	DEBUG_MSG("free_stringlist, started\n");

	tmplist = g_list_first(which_list);
	while (tmplist != NULL) {
		/*DEBUG_MSG("free_stringlist, tmplist->data(%p)\n", tmplist->data);*/
		g_free(tmplist->data);
		tmplist = g_list_next(tmplist);
	}
	DEBUG_MSG("free_stringlist, strings free-ed, about to free list itself\n");
	g_list_free(which_list);
	which_list = NULL;
	return 1;
}

gint free_arraylist(GList * which_list)
{
	GList *tmplist;
#ifdef DEBUGGING
	gchar **tmpchar;
#endif
	DEBUG_MSG("free_arraylist, started\n");

	tmplist = g_list_first(which_list);
	while (tmplist != NULL) {

#ifdef DEBUGGING
		DEBUG_MSG("free_arraylist, starting list %p with data %p\n", tmplist, tmplist->data);
		tmpchar = (gchar **)tmplist->data;
		while (*tmpchar) {
			DEBUG_MSG("free_arraylist, freeing %p\n", *tmpchar);
			DEBUG_MSG("free_arraylist, containing %s\n", *tmpchar);
			g_free(*tmpchar);
			tmpchar++;
		}
		DEBUG_MSG("free_arraylist, content freed\n");
		g_free((gchar **)tmplist->data);
		DEBUG_MSG("free_arraylist, array freed\n");
#else
		g_strfreev((gchar **)tmplist->data);
#endif
		tmplist = g_list_next(tmplist);
	}
	g_list_free(which_list);
	return 1;
}

GList *duplicate_arraylist(GList *arraylist) {
	GList *tmplist;
	GList *newlist=NULL;

	tmplist = g_list_last(arraylist);
	while (tmplist != NULL) {
		newlist = g_list_prepend(newlist, g_strdupv((gchar **)tmplist->data));
		tmplist = g_list_previous(tmplist);
	}
	return newlist;
}

GList *get_list_from_buffer(gchar *buffer, GList *which_list, gboolean is_arraylist) {
	gchar *pos,*nextpos;
	pos = buffer;
	nextpos = strchr(pos, '\n');
	do {
		if (nextpos)
			*nextpos='\0';
		/*g_print("next line: %s\n",pos);*/
		if (is_arraylist) {
			gchar **temparr = string_to_array(pos);
			which_list = g_list_prepend(which_list, temparr);
		} else {
			DEBUG_MSG("get_list, adding string \"%s\" to the stringlist=%p\n", pos, which_list);
			which_list = g_list_prepend(which_list, g_strdup(pos));
		}
		if (nextpos) {
			nextpos++;
			if (*nextpos == '\0')
				pos = NULL;
			else
				pos = nextpos;
		} else 
			pos = NULL;
		if (pos)
			nextpos = strchr(pos, '\n');
	} while (pos);
	return g_list_reverse(which_list);
}

/*****************************************************************************
 * gets a stringlist from a file
 */
GList *get_list(GFile * file, GList * which_list, gboolean is_arraylist) {
	gchar *buffer=NULL;
	gsize length;
	GError *error=NULL;
	if (file == NULL) {
		return NULL;
	}
	g_file_load_contents(file,NULL,&buffer,&length,NULL,&error);
	if (error) {
		g_print("error reading list %d %s\n",error->code, error->message);
		return NULL;
	}
	which_list = get_list_from_buffer(buffer, which_list, is_arraylist);
	g_free(buffer);
	
/*	FILE *fd;
	gchar *tmpbuf, *tmpfilename;

	DEBUG_MSG("get_stringlist, started with filename=%s\n", filename);
	if (filename == NULL) {
		return NULL;
	}
	tmpfilename = g_strdup(filename+((strncmp(filename, "file://",7)==0)?7:0));
	DEBUG_MSG("get_stringlist, opening %s\n", tmpfilename);
	fd = fopen(tmpfilename, "r");
	g_free(tmpfilename);
	if (fd == NULL) {
		return NULL;
	}
	tmpbuf = g_malloc(STRING_MAX_SIZE);
	while (fgets(tmpbuf, STRING_MAX_SIZE, fd) != NULL) {
		gchar *tempstr;
		tmpbuf = trunc_on_char(tmpbuf, '\n');
		tempstr = g_strdup(tmpbuf);
		if (is_arraylist) {
			gchar **temparr = string_to_array(tempstr);
			which_list = g_list_append(which_list, temparr);
			g_free(tempstr);
		} else {
			DEBUG_MSG("get_list, adding string \"%s\" to the stringlist=%p\n", tempstr, which_list);
			which_list = g_list_append(which_list, tempstr);
		}
	}
	fclose(fd);
	g_free(tmpbuf);*/
	return which_list;
}
#ifdef __GNUC__
__inline__ 
#endif
GList *get_stringlist(GFile * filename, GList * which_list) {
	return get_list(filename,which_list,FALSE);
}

/**
 * put_stringlist_limited:
 * @filename: #gchar* with the filename to store the list
 * @which_list: #GList* with the list to store
 * @maxentries: #gint only the LAST maxentries of the list will be stored
 *
 * stores the LAST maxentries entries of list which_list in file filename
 * if maxentries is 0 all entries will be stored
 *
 * Return value: #gboolean TRUE on success, FALSE on failure
 * /
gboolean put_stringlist_limited(GFile * file, GList * which_list, gint maxentries, gboolean is_arraylist) {
	GString *strbuffer;
	GError *error=NULL;
	GList *tmplist;
	DEBUG_MSG("put_stringlist_limited, limit file %s to %d entries\n", g_file_get_uri(file), maxentries);
	if (maxentries > 0) {
		gint count;
		count = g_list_length(which_list) - maxentries;
		tmplist = g_list_nth(which_list, (count<0) ? 0 : count);
	} else {
		tmplist = g_list_first(which_list);
	}
	strbuffer = g_string_sized_new(1024);
	if (is_arraylist) {
		while (tmplist) {
			gchar *tmp = array_to_string(tmplist->data);
			strbuffer = g_string_append(strbuffer,tmp);
			g_free(tmp);
			strbuffer = g_string_append_c(strbuffer,'\n');
			tmplist = g_list_next(tmplist);
		}
	} else {
		while (tmplist) {
			strbuffer = g_string_append(strbuffer,(char *) tmplist->data);
			strbuffer = g_string_append_c(strbuffer,'\n');
			tmplist = g_list_next(tmplist);
		}
	}
	g_file_replace_contents(file,strbuffer->str,strbuffer->len
				,NULL,FALSE,G_FILE_CREATE_PRIVATE,NULL,NULL,&error);
	if (error) {
		g_warning("save stringlist error %d %s\n",error->code,error->message);
		g_error_free(error);
		return FALSE;
	}
	g_string_free(strbuffer,TRUE);
	DEBUG_MSG("put_stringlist_limited, finished, filedescriptor closed\n");
	return TRUE;
}*/

/**
 * stringlist_to_string:
 * @stringlist: a #GList * to convert
 * @delimiter: a #const gchar * item with the delimiter
 * 
 * this function will convert a stringlist (GList that contains 
 * only \0 terminated gchar* elements) to a string, putting the 
 * delimiter inbetween all elements;
 * 
 * Return value: the gchar *
 **/
gchar *stringlist_to_string(GList *stringlist, gchar *delimiter) {
	GString *strbuffer;
	GList *tmplist;
	strbuffer = g_string_sized_new(4096);
	for (tmplist=g_list_first(stringlist);tmplist;tmplist=g_list_next(tmplist)) {
		strbuffer = g_string_append(strbuffer,(char *) tmplist->data);
		strbuffer = g_string_append(strbuffer,(char *) delimiter);
	}
	return g_string_free(strbuffer, FALSE);
}
gchar *arraylist_to_string(GList *arraylist, gchar *delimiter) {
	GString *strbuffer;
	GList *tmplist;
	strbuffer = g_string_sized_new(4096);
	for (tmplist=g_list_first(arraylist);tmplist;tmplist=g_list_next(tmplist)) {
		gchar *tmp = array_to_string(tmplist->data);
		strbuffer = g_string_append(strbuffer,tmp);
		g_free(tmp);
		strbuffer = g_string_append(strbuffer,(char *) delimiter);
	}
	return g_string_free(strbuffer, FALSE);	
}

gboolean put_stringlist(GFile * file, GList * which_list, gboolean is_arraylist) {
	/*return put_stringlist_limited(file,which_list, -1, is_arraylist);*/
	gchar *buf;
	GError *gerror=NULL;
	if (is_arraylist) {
		buf = arraylist_to_string(which_list, "\n");
	} else {
		buf = stringlist_to_string(which_list, "\n");
	}
	g_file_replace_contents(file,buf,strlen(buf)
				,NULL,FALSE,G_FILE_CREATE_PRIVATE,NULL,NULL,&gerror);
	if (gerror) {
		g_warning("save stringlist error %d %s\n",gerror->code,gerror->message);
		g_error_free(gerror);
		return FALSE;
	}
	g_free(buf);
	return TRUE;
}

GList *remove_from_stringlist(GList *which_list, const gchar * string) {
	if (string && strlen(string) ) {
		GList *tmplist = g_list_first(which_list);
		while (tmplist) {
			if (strcmp((gchar *) tmplist->data, string) == 0) {
				DEBUG_MSG("remove_from_stringlist, removing '%s' (%p)\n", (gchar *)tmplist->data, tmplist->data);
				g_free(tmplist->data);
				return g_list_delete_link(which_list, tmplist);
			}
			tmplist = g_list_next(tmplist);
		}
	}
	return which_list;
}

static void unlink_before(GList *tmplist) {
	GList *prev = tmplist->prev;
	DEBUG_MSG("unlink_before, freeing starting at %s\n",(gchar *)tmplist->data);
	if (prev) {
		prev->next = NULL;
	}
	tmplist->prev = NULL;
}

GList *limit_stringlist(GList *which_list, gint num_entries, gboolean keep_end) {
	GList *retlist, *freelist;
	if (keep_end) {
		gint len;
		freelist = g_list_first(which_list);
		len = g_list_length(freelist);
		if (len <= num_entries) return which_list;
		retlist = g_list_nth(freelist, len - num_entries);
		unlink_before(retlist);
	} else {
		retlist = g_list_first(which_list);
		freelist = g_list_nth(retlist, num_entries);
		if (freelist) unlink_before(freelist);
	}
	if (freelist) free_stringlist(freelist);
	return retlist;
}

GList *find_in_stringlist(GList *thelist, const gchar *string) {
	GList *list;
	for (list=g_list_first(thelist);list;list=list->next) {
		if (g_strcmp0(string, list->data)==0)
			return list;
	}
	return NULL;
} 

/**
 * add_to_history_stringlist:
 * @which_list: #GList* the list to add to
 * @string: #const gchar* with the string to add
 * @recent_on_top: #gboolean, TRUE if the most recent entry is the one on top
 * @move_if_exists: #gboolean, if TRUE do move existing entries to the most recent part
 *
 * adds string to the stringlist which_list
 *
 * if string exists in this list already and move_if_exists is TRUE, 
 * it will be moved to the most recent part of the list (which is the 
 * end or the beginning based on the value of recent_on_top
 *
 * Return value: GList* with the modified list
 */
GList *add_to_history_stringlist(GList *which_list, const gchar *string, gboolean move_if_exists) {
	GList *tmplist;
	if (!string || string[0]=='\0' )
		return which_list;
	
	tmplist = find_in_stringlist(which_list, string);
	if (tmplist) {
		/* move this entry to the end */
		if (move_if_exists) {
			DEBUG_MSG("add_to_history_stringlist, entry %s exists, moving!\n", string);
			which_list = g_list_remove_link(which_list, tmplist);
			return g_list_concat(tmplist, which_list);
		} else {
			return which_list;
		}
	} else {
		DEBUG_MSG("add_to_history_stringlist, adding new entry %s\n",string);
		which_list = g_list_prepend(which_list, g_strdup(string));
	}
	return which_list;
}

/**
 * add_to_stringlist:
 * @which_list: a #GList * to add to
 * @string: a #const gchar * item you want to add to the list
 * 
 * this function will check if a string with same content exists already
 * and if not it will add it to the list, it returns the new list pointer
 * 
 * Return value: the new GList *
 **/
GList *add_to_stringlist(GList * which_list, const gchar * string) {
	if (string && strlen(string) ) {
		GList *tmplist = g_list_first(which_list);
		while (tmplist) {
			if (strcmp((gchar *) tmplist->data, string) == 0) {
				DEBUG_MSG("add_to_stringlist, strings are the same, don't add!!\n");
				return which_list;
			}
			tmplist = g_list_next(tmplist);
		}
		/* if we arrive here the string was not yet in the list */
		which_list = g_list_prepend(which_list, g_strdup(string));
	}
	return which_list;
}

/**
 * array_n_strings_identical:
 * @array1: #gchar**
 * @array2: #gchar**
 * @case_sensitive: #gboolean 
 * @testlevel: #gint
 *
 * tests the first testlevel strings in the arrays if they are identical
 * returns the first strcmp() value that is not 0, or 0 if all
 * strings up to testlevel are identical.
 *
 * if BOTH array end before testlevel is reached, 0 is returned
 * if ONE array ends before the other, -1 or 1 is returned
 *
 * Return value: #gint
 */
gint array_n_strings_identical(const gchar **array1, const gchar **array2, gboolean case_sensitive, gint testlevel) {
	gint i=0, res=0;
	while (i<testlevel && res==0) {
		/*  array1[i]==array2[i] will only happen when they are both NULL	*/
		if (array1[i] == NULL || array2[i] == NULL) {
			/* NULL - NULL = 0
			 * NULL - ptr = negative
			 * ptr - NULL = positive */
			return (gint)(array1[i] - array2[i]);
		} else if (case_sensitive) {
			res = strcmp(array1[i],array2[i]);
		} else /*if (!case_sensitive)*/ {
			res = strcasecmp(array1[i],array2[i]);
		}
		if (res !=0) return res;
		i++;
	}
	return res;
}
#ifdef OBSOLETE
/**
 * arraylist_delete_identical:
 * @thelist: #GList*
 * @compare: #gchar**
 * @testlevel: #gint
 * @case_sensitive: #gboolean
 *
 * Deletes all arrays from 'arraylist' that are, up to testlevel, identical to array 'compare'
 *
 * Return value: the new list
 */
GList *arraylist_delete_identical(GList *thelist, const gchar **compare, gint testlevel, gboolean case_sensitive) {
	GList *tmplist = g_list_first(thelist);
	while (tmplist) {
		GList *nextlist = g_list_next(tmplist);
		gchar **tmparr = tmplist->data;
		if (array_n_strings_identical(compare, (const gchar **)tmparr, case_sensitive, testlevel)==0) {
			DEBUG_MSG("arraylist_delete_identical, %s and %s are identical, will delete %p from list\n",tmparr[0],compare[0], tmplist);
			thelist = g_list_delete_link(thelist, tmplist);
			DEBUG_MSG("arraylist_delete_identical, free array %p (%s)\n",tmparr,tmparr[0]);
			g_strfreev(tmparr);
		}
		tmplist = nextlist;
	}
	return thelist;
}

/**
 * arraylist_append_identical_from_list:
 * @thelist: #GList*
 * @source: #GList*
 * @compare: #gchar**
 * @testlevel: #gint
 * @case_sensitive: #gboolean
 *
 * compares every array in 'source' with 'compare', and if it is identical up to 'testlevel', it will
 * add the array to 'thelist'
 *
 * Return value: the new arraylist
 */
GList *arraylist_append_identical_from_list(GList *thelist, GList *source, const gchar **compare, gint testlevel, gboolean case_sensitive) {
	GList *tmplist = g_list_first(source);
	while (tmplist) {
		gchar **tmparr = tmplist->data;
		if (array_n_strings_identical(compare, (const gchar **)tmparr, case_sensitive, testlevel)==0) {
			thelist = g_list_append(thelist, g_strdupv(tmparr));
		}
		tmplist = g_list_next(tmplist);
	}
	return thelist;
}

/**
 * arraylist_append_identical_from_file:
 * @thelist: #GList*
 * @sourcefilename: #const gchar*
 * @compare: #gchar**
 * @testlevel: #gint
 * @case_sensitive: #gboolean
 *
 * compares every array read from 'sourcefilename' with 'compare', and if it is identical up to 'testlevel', it will
 * add the array to 'thelist'
 *
 * Return value: the new arraylist
 * /
GList *arraylist_append_identical_from_file(GList *thelist, const gchar *sourcefilename, const gchar **compare, gint testlevel, gboolean case_sensitive) {
	GList *sourcelist = get_list(sourcefilename,NULL,TRUE);
	thelist = arraylist_append_identical_from_list(thelist, sourcelist, compare, testlevel, case_sensitive);
	free_arraylist(sourcelist);
	return thelist;
}*/
#endif
/**
 * arraylist_value_exists:
 * @arraylist: #GList*
 * @value: #gchar**
 * @testlevel: #gint
 * @case_sensitive: #gboolean whether the test should be case sensitive
 *
 * tests for the occurence of an array with identical content as value
 * in arraylist. It will only test the first testlevel strings in the array
 * so if you want the test to check for a 100% identical array that number 
 * should be high (9999 or so)
 *
 * Return value: #gchar ** 
 */
gchar **arraylist_value_exists(GList *arraylist, const gchar **value, gint testlevel, gboolean case_sensitive) {
	GList *tmplist = g_list_first(arraylist);
	while (tmplist) {
		gchar **tmparr = tmplist->data;
		if (array_n_strings_identical(value, (const gchar **)tmparr, case_sensitive, testlevel)==0) {
/*			g_print("arraylist_value_exists,found it!\n");*/
			return tmparr;
		}
		tmplist = g_list_next(tmplist);
	}
/*	g_print("arraylist_value_exists,found nothing, listlen=%d\n",g_list_length(arraylist));*/
	return NULL;
}
#ifdef OBSOLETE
/**
 * arraylist_load_new_identifiers_from_list:
 * @mylist: #GList*
 * @fromlist: #GList*
 * @uniquelevel: #gint
 *
 * compares every entry in fromlist 
 * with all entries in list mylist. Comparision is done uniquelevel deep
 * by function arraylist_value_exists()
 * those arrays that do _not_ match are appended to mylist which is returned
 * at the end of the function
 *
 * Return value: #GList*
 */
GList *arraylist_load_new_identifiers_from_list(GList *mylist, GList *deflist, gint uniquelevel) {
	GList *tmplist = g_list_first(deflist);
	while (tmplist) {
		gchar **tmparr = tmplist->data;
		if (g_strv_length(tmparr) >= uniquelevel) {
			if (!arraylist_value_exists(mylist, (const gchar **)tmparr, uniquelevel, TRUE)) {
				DEBUG_MSG("arraylist_load_new_identifiers, adding %s to thelist\n",tmparr[0]);
				mylist = g_list_append(mylist, g_strdupv(tmparr));
			}
		}
		tmplist = g_list_next(tmplist);
	}
	return mylist;
}
/**
 * arraylist_load_new_identifiers_from_file:
 * @mylist: #GList*
 * @fromfilename: #gchar*
 * @uniquelevel: #gint
 *
 * loads an arraylist from fromfilename and compares every entry 
 * with all entries in list mylist. Comparision is done uniquelevel deep
 * by function arraylist_value_exists()
 * those arrays that do _not_ match are appended to mylist which is returned
 * at the end of the function
 *
 * Return value: #GList*
 * / 
GList *arraylist_load_new_identifiers_from_file(GList *mylist, const gchar *fromfilename, gint uniquelevel) {
	GList *deflist = get_list(fromfilename,NULL,TRUE);
	mylist = arraylist_load_new_identifiers_from_list(mylist, deflist, uniquelevel);
	free_arraylist(deflist);
	return mylist;	
}*/
#endif

/* pure for debugging purposes */
#ifdef DEVELOPMENT
void dump_arraylist(GList *list) {
	GList *tmplist;
	for (tmplist=g_list_first(list);tmplist;tmplist=tmplist->next) {
		gchar *tmp = array_to_string((gchar **)tmplist->data);
		g_print("%s\n",tmp);
		g_free(tmp);
	}
}
#endif

