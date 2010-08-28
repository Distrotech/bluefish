/* Bluefish HTML Editor
 * cap.h - handle tag capitalisation
 *
 * Copyright (C) 1999 Antti-Juhani Kaijanaho
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

#ifndef __CAP_H_
#define __CAP_H_

/* cap assumes that the argument is a HTML tag and capitalises it
   accordingly, based on user preferences.  The return value is a
   pointer to a string whose uniqueness is quaranteed for the next
   five calls to this function.  (That is, the return strings are
   local static buffers (five of them) used in a round-robin style.  */
char *cap(const char *);

#endif							/* __CAP_H_ */
