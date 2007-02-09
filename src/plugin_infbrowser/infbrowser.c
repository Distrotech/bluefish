/* Bluefish HTML Editor
 * infbrowser.c - plugin for information browser
 *
 * Copyright (C) 2007 Oskar Świda
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
#define DEBUG

#include <string.h>
#include "infbrowser.h"
#include <gmodule.h>
#include "../plugins.h"
#include "../rcfile.h"
#include "../document.h"
#include "../bf_lib.h"
#include "infb_gui.h"
#include "infb_load.h"

static const char *frnote_xpm[] = {
"11 12 5 1",
" 	c None",
".	c #000000",
"+	c #FFFFC0",
"@	c #FFFFFF",
"#	c #303030",
"........   ",
".++++++..  ",
".++++++.@. ",
".++++++....",
".+++++++++.",
".++...#.++.",
".+++++++++.",
".+++....++.",
".+++++++++.",
".++....+++.",
".+++++++++.",
"..........."};

static const char *frlibrary_xpm[] = {
"12 12 6 1",
" 	c None",
".	c #000000",
"+	c #0000FF",
"@	c #C0C0C0",
"#	c #808080",
"$	c #FFFFFF",
"     ..     ",
"   ..++.    ",
" ..+++++.   ",
"..+++++++.  ",
".@.+++++++. ",
".+@.++++++..",
" .+@.+++..#$",
"  .+@...##@$",
"   .+@.#@@..",
"    .+.#..  ",
"     ...    ",
"            "};

static const char *cgroup_xpm[] = {
"15 12 12 1",
" 	c None",
".	c #000000",
"+	c #FFFFFF",
"@	c #CDD2AC",
"#	c #CACFAA",
"$	c #CCD1AB",
"%	c #CED3AE",
"&	c #CED3AF",
"*	c #C9CEA9",
"=	c #C8CDA8",
"-	c #CDD2AD",
";	c #C8CDA9",
"  ....         ",
" .++++.        ",
"............   ",
".@@@@@@@@@@@.  ",
".@@@@@@@@@@@.  ",
".@#$%&%@@@@@.  ",
".@*$&@@@@@@@.  ",
".@=@-@@@@@@@.  ",
".@;-@@@@@@@@.  ",
".@@@@@@@@@@@.  ",
".@@@@@@@@@@@.  ",
" ...........   "};

static const char *ogroup_xpm[] = {
"15 12 5 1",
" 	c None",
".	c #000000",
"+	c #FFFFFF",
"@	c #CDD2AC",
"#	c #979B84",
"  ....         ",
" .++++.        ",
".@#####.....   ",
".@##########.  ",
".@##.......... ",
".##.@@@@@@@@@@.",
".##.@@@@@@@@@@.",
".#.@@@@@@@@@@. ",
".#.@@@@@@@@@@. ",
"..@@@@@@@@@@.  ",
"..@@@@@@@@@@.  ",
" ...........   "};


static const char * frnode_xpm[] = {
"12 12 3 1",
" 	c None",
".	c #000000",
"+	c #4B6983",
"            ",
"     ..     ",
"    .++.    ",
"    .++.    ",
"   .++++.   ",
"  .++++++.  ",
"  .++++++.  ",
" .++++++++. ",
"..++++++++..",
".++++++++++.",
"............",
"            "};

static const char * frpoint_xpm[] = {
"8 8 3 1",
" 	c None",
".	c #000000",
"+	c #4B6983",
"  ....  ",
" .++++. ",
".++++++.",
".++++++.",
".++++++.",
".++++++.",
" .++++. ",
"  ....  "};



Tinfb infb_v;


static void infb_init(void) {

#ifdef ENABLE_NLS
	DEBUG_MSG("infb_init, gettext domain-name=%s\n",PACKAGE"_plugin_infbrowser");
	bindtextdomain(PACKAGE"_plugin_infbrowser", LOCALEDIR);
	bind_textdomain_codeset(PACKAGE"_plugin_infbrowser", "UTF-8");
#endif
	infb_v.currentDoc = NULL;
	infb_v.currentNode = NULL;
	infb_v.currentType = 0;
	infb_v.homeDoc = NULL;
	infb_v.lastSearch = NULL;
	infb_v.windows = g_hash_table_new_full(NULL,NULL,NULL,NULL);
	main_v->sidepanel_initgui = g_slist_prepend(main_v->sidepanel_initgui,infb_sidepanel_initgui);
	main_v->sidepanel_destroygui = g_slist_prepend(main_v->sidepanel_destroygui,infb_sidepanel_destroygui);
	
	infb_v.icon_note = gdk_pixbuf_new_from_xpm_data(frnote_xpm);
	infb_v.icon_doc = gdk_pixbuf_new_from_xpm_data(frlibrary_xpm);
	infb_v.icon_ogroup = gdk_pixbuf_new_from_xpm_data(ogroup_xpm);
	infb_v.icon_cgroup = gdk_pixbuf_new_from_xpm_data(cgroup_xpm);
	infb_v.icon_node = gdk_pixbuf_new_from_xpm_data(frnode_xpm);
	infb_v.icon_point = gdk_pixbuf_new_from_xpm_data(frpoint_xpm);
		
	DEBUG_MSG("infb_init finished \n");
}

static void infb_initgui(Tbfwin* bfwin) {

}

static void infb_enforce_session(Tbfwin* bfwin) {
}

static void infb_cleanup(void) {
	/*g_object_unref(infb_v.icon_note);*/
}

static void infb_cleanup_gui(Tbfwin *bfwin) {
}

static GList *infb_register_globses_config(GList *configlist) {
	return configlist;
}

static GList *infb_register_session_config(GList *configlist, Tsessionvars *session) {
	return configlist;
}

static TBluefishPlugin bfplugin = {
	"infbrowser",
	BFPLUGIN_VERSION,
	sizeof(Tdocument),
	sizeof(Tsessionvars),
	sizeof(Tglobalsession),
	sizeof(Tbfwin),
	sizeof(Tproject),
	sizeof(Tmain),
	NULL, /* private */
	infb_init, /* init */
	infb_initgui, 
	infb_enforce_session,
	infb_cleanup,
	infb_cleanup_gui,
	infb_register_globses_config,
	infb_register_session_config,
	NULL, /* binary compatibility */
	NULL,
	NULL,
	NULL
};

G_MODULE_EXPORT TBluefishPlugin *getplugin(void) {
	return &bfplugin;
}
