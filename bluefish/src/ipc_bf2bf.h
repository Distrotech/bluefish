/* Bluefish HTML Editor
 * ipc_bf2bf.h - IPC socket bluefish to bluefish
 *
 * Copyright (C) 2009 Olivier Sessink
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __IPC_BF2BF_H_
#define __IPC_BF2BF_H_

gboolean ipc_bf2bf_start(GList * filenames, gboolean in_new_window);
void ipc_bf2bf_cleanup(void);

#endif /* __IPC_BF2BF_H_ */
