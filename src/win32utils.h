/* Bluefish HTML Editor
 * win32utils.c - non-portable win32 support header
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

/* User auth levels */
#define AUTH_ADMIN 1
#define AUTH_USER 2
#define AUTH_CLASSIC 3

/* Registry read access */
#ifndef ACCESS_READ
	#define ACCESS_READ 1
#endif
#ifndef ACCESS_WRITE
	#define ACCESS_WRITE 2
#endif

gboolean pythoncheck_win32(void);
gint getauthlevel_win32(void);
gboolean isadmin_win32(void);
