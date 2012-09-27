/* Bluefish HTML Editor
 * external_commands.c - backend for external commands, filters and the outputbox
 *
 * Copyright (C) 1999-2005 Olivier Sessink
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

#ifndef __EXTERNAL_COMMANDS_H_
#define __EXTERNAL_COMMANDS_H_

gboolean operatable_on_selection(const gchar *formatstring);
void outputbox_command(Tbfwin *bfwin, const gchar *formatstring);
void filter_command(Tbfwin *bfwin, const gchar *formatstring, gint begin, gint end);
void external_command(Tbfwin *bfwin, const gchar *formatstring);

typedef void (*CustomCommandCallback) (const gchar *output, gpointer bfwin, gpointer data);
void custom_command(Tbfwin *bfwin, const gchar *formatstring, CustomCommandCallback func, gpointer data);

#endif /* __EXTERNAL_COMMANDS_H_ */
