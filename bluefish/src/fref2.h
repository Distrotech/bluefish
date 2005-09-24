/* Bluefish HTML Editor
 * fref2.h - Library aka Function reference (second implementation)
 *
 * Copyright (C) 2005 Oskar Świda swida@aragorn.pb.bialystok.pl
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

#ifndef __FREF_H__
#define __FREF_H__

GtkWidget *fref_gui(Tbfwin *bfwin); /* used in gui.c */

void fref_init(void); /* only used once */
void fref_cleanup(Tbfwin *bfwin);
void fref_rescan_dir(const gchar *dir); /* used by rcfile.c */

#endif /* __FREF_H__ */
