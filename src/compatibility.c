/* Bluefish HTML Editor
 * compatibility.c
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
#include "bluefish.h"

#if !GLIB_CHECK_VERSION(2,28,0)
 void g_list_free_full(GList *list,GDestroyNotify free_func)
 {
	GList *tmplist;
 	for (tmplist = g_list_first(list);tmplist;tmplist=tmplist->next) {
		free_func(tmplist->data);
	}
	g_list_free(list);
 }
 #endif

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
