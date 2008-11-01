/* Bluefish HTML Editor
 * cap.c - handle tag capitalisation
 *
 * Copyright (C) 1999 Antti-Juhani Kaijanaho
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

/* added dirty hack to prevent printf %x-stuff to be altered */
#include <gtk/gtk.h>
#include <ctype.h>
#include <string.h>

#include "cap.h"
#include "bluefish.h"

#define MAX_BUCKETS 9

/* In theory you could implement this as a caching system: when you
   capitalise a new string, cache the result; when you are called
   again, see if the result is cached and return that.  This would
   ensure that there are no memory leaks and that every unique string
   has an unlimited lifetime.  Of course, garbage collection would
   make all this playing around unnecessary... */
char *cap(const char *s)
{
	int (*needschange) (int);
	int (*dochange) (int);
	static int bn = 0;			/* which bucket is next? */
	static char *bucket[MAX_BUCKETS] = { 0 };	/* Initialize all to 0. */
	char *rv;
	char lastchar = '.';
	size_t i;
	size_t slen;

	DEBUG_MSG("cap, lowercase_tags=%d\n", main_v->props.lowercase_tags);
	if (!main_v->props.lowercase_tags) {
		DEBUG_MSG("cap, we DO NOT want lowercase\n");
		needschange = islower;
		dochange = toupper;
	} else {
		DEBUG_MSG("cap, we want lowercase\n");
		needschange = isupper;
		dochange = tolower;
	}

	slen = strlen(s);

	if (bucket[bn] != 0) {
		/* A non-null bucket is in use. */
		g_free(bucket[bn]);
	}

	bucket[bn] = g_malloc(slen + 1);

	for (i = 0; i < slen; i++) {
		if ((needschange(s[i])) && (lastchar != '%')) {
			bucket[bn][i] = dochange(s[i]);
			DEBUG_MSG("cap, lastchar is -->%c\n", lastchar);
		} else {
			DEBUG_MSG("cap, lastchar NO -->%c\n", lastchar);
			bucket[bn][i] = s[i];
		}
		lastchar = s[i];
	}

	bucket[bn][slen] = 0;

	rv = bucket[bn];

	bn = (bn + 1 == MAX_BUCKETS) ? 0 : bn + 1;

	return rv;
}
