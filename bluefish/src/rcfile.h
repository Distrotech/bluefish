/* Bluefish HTML Editor
 * rcfile.h - functions to load the config
 *
 * Copyright (C) 2001-2003 Olivier Sessink
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __RCFILE_H_
#define __RCFILE_H_
void rcfile_parse_main(void);
void rcfile_parse_highlighting(void);
void rcfile_parse_custom_menu(void);
void rcfile_check_directory(void);
void rcfile_save_configfile_menu_cb(gpointer callback_data,guint action,GtkWidget *widget);
void rcfile_save_all(void);
#endif /* __RCFILE_H_ */
