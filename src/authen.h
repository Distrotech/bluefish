/* Bluefish HTML Editor
 * authen.h - handle authentication requests for gnome-vfs
 *
 * Copyright (C) 2004 Salvador Fandino
 * additions (C) 2004 Olivier Sessink
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

#ifndef __AUTHEN_H_
#define __AUTHEN_H_

#ifdef HAVE_ATLEAST_GNOMEVFS_2_6
#ifndef HAVE_ATLEAST_GNOMEUI_2_6
void set_authen_callbacks(void);
#endif
#endif
#endif							/* __AUTHEN_H_ */
