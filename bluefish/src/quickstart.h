/* Bluefish HTML Editor
 * quickstart.h - quickstart dialog
 *
 * Copyright (C) 2005 James Hayward and Olivier Sessink
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

#ifndef __QUICKSTART_H_
#define __QUICKSTART_H_

#include "config.h"

#ifdef HAVE_ATLEAST_GTK_2_4
void quickstart_dialog_new(Tbfwin *bfwin);
#endif /* HAVE_ATLEAST_GTK_2_4 */

#endif /* __QUICKSTART_H_ */
