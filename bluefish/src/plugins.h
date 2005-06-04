/* Bluefish HTML Editor
 * plugins.h - plugin structure
 *
 * Copyright (C) 2005 Olivier Sessink
 * heavily inspired by code from Eugene Morenko(More)
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
#ifndef __PLUGINS_H__
#define __PLUGINS_H__

#include "bluefish.h"

typedef struct {
	const gchar *name; /* plugin name */
	gpointer private; /* to be filled by Bluefish after loading */
	void (*init) (void); /* called once after plugin is loaded */
	void (*init_gui) (Tbfwin *bfwin); /* builds the gui */
	void (*cleanup) (void);
	void (*cleanup_gui) (Tbfwin *bfwin);
	
	void (*doc_view_populate_popup)(GtkTextView *textview,GtkMenu *menu,Tdocument *doc);
	void (*doc_view_button_press)(GtkWidget *widget,GdkEventButton *bevent, Tdocument *doc);

	GList *(*register_config)(GList *configlist);

	gpointer extra1; /* for binary compatibility */
	gpointer extra2;
	gpointer extra3;
	gpointer extra4;
} TBluefishPlugin;

#ifdef __cplusplus
extern "C" {
#endif

void bluefish_load_plugins(void);
void bluefish_cleanup_plugins(void);

#ifdef __cplusplus
};
#endif

#endif /* __PLUGINS_H__ */
