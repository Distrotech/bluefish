/* Bluefish HTML Editor
 * about.h - the About dialog
 *
 * Copyright (C) 2004 Eugene Morenko(More) more@irpin.com
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

#ifndef __ABOUT_H_
#define __ABOUT_H_

#define ENABLEPLUGINS

#include "../bluefish.h"

#ifdef ENABLE_NLS
#undef _
#define _(String) dgettext(PACKAGE"_plugin_about", String)
#endif

#endif /* __ABOUT_H_  */
