/* Bluefish HTML Editor
 * entities.h - Entity/ASCII replace dialog
 *
 * Copyright (C) 2006 Daniel Leidert <daniel.leidert@wgdd.de>.
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

#ifndef __ENTITIES_H_
#define __ENTITIES_H_

#define ENABLEPLUGINS

#include "../bluefish.h"

#ifdef ENABLE_NLS
#undef _
#define _(String) dgettext(PACKAGE"_plugin_entities", String)
#endif

#endif /* __ENTITIES_H_  */
