/* Bluefish HTML Editor
 * bftextview2_autocomp.h
 *
 * Copyright (C) 2008-2011 Olivier Sessink
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

#ifndef _BFTEXTVIEW2_AUTOCOMP_H_
#define _BFTEXTVIEW2_AUTOCOMP_H_

#include "bftextview2.h"
gboolean acwin_check_keypress(BluefishTextView * btv, GdkEventKey * event);
void autocomp_stop(BluefishTextView * btv);
void autocomp_run(BluefishTextView * btv, gboolean user_requested);

#endif							/* _BFTEXTVIEW2_AUTOCOMP_H_ */
