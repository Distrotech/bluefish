/* Bluefish HTML Editor
 * charmap.c - the charmap panel
 *
 * Copyright (C) 2009-2010 Olivier Sessink
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

#include "charmap.h"
#include "charmap_gui.h"
#include "../plugins.h"
#include "../rcfile.h"
Tcharmap charmap_v;

static void charmap_init(void) {
#ifdef ENABLE_NLS
  DEBUG_MSG("charmap_init, gettext domain-name=%s\n",PACKAGE"_plugin_charmap");
  bindtextdomain(PACKAGE"_plugin_charmap", LOCALEDIR);
  bind_textdomain_codeset(PACKAGE"_plugin_charmap", "UTF-8");
#endif /* ENABLE_NLS */
	charmap_v.lookup = g_hash_table_new_full(NULL /* == g_direct_hash() */,
					NULL /* == g_direct_equal() */,
					NULL,g_free);
	main_v->sidepanel_initgui = g_slist_prepend(main_v->sidepanel_initgui,charmap_sidepanel_initgui);
	main_v->sidepanel_destroygui = g_slist_prepend(main_v->sidepanel_destroygui,charmap_sidepanel_destroygui);
}

/*static void charmap_initgui(Tbfwin* bfwin) {
}
*/
static void charmap_enforce_session(Tbfwin* bfwin) {
	Tcharmapsession *cms;
	Tcharmapwin *cmw;
	cms = g_hash_table_lookup(charmap_v.lookup,bfwin->session);
	cmw = g_hash_table_lookup(charmap_v.lookup,bfwin);
	if (cms && cmw) {
		/*g_print("enforce session, set block %d\n",cms->charmap_block);*/
		gtk_combo_box_set_active(GTK_COMBO_BOX(cmw->chaptersv),cms->charmap_block);
	}
}
static void charmap_cleanup(void) {
	g_hash_table_unref(charmap_v.lookup);
	main_v->sidepanel_initgui=g_slist_remove(main_v->sidepanel_initgui,charmap_sidepanel_initgui);
	main_v->sidepanel_destroygui=g_slist_remove(main_v->sidepanel_destroygui,charmap_sidepanel_destroygui);
}

/*static void charmap_cleanup_gui(Tbfwin *bfwin) {
}
*/
/*static GHashTable *charmap_register_globses_config(GHashTable *configlist) {
  return configlist;
}
*/
Tcharmapsession *get_charmap_session(gpointer session) {
	Tcharmapsession *cms = g_hash_table_lookup(charmap_v.lookup,session);
	if (!cms) {
		cms = g_new0(Tcharmapsession,1);
		cms->charmap_block = TRUE;
		g_hash_table_insert(charmap_v.lookup,session,cms);
	}
	return cms;
}
static void charmap_session_cleanup(Tsessionvars *session) {
	g_hash_table_remove(charmap_v.lookup,session);
}

static GHashTable *charmap_register_session_config(GHashTable *configlist, Tsessionvars *session) {
	Tcharmapsession *cms = get_charmap_session(session);
	configlist = make_config_list_item(configlist, &cms->charmap_block, 'i', "charmap_block:", 0);
	return configlist;
}

static TBluefishPlugin bfplugin = {
  "Character Map",
  BFPLUGIN_VERSION,
  GTK_MAJOR_VERSION,
  sizeof(Tdocument),
  sizeof(Tsessionvars),
  sizeof(Tglobalsession),
  sizeof(Tbfwin),
  sizeof(Tproject),
  sizeof(Tmain),
  sizeof(Tproperties),
  BFPLUGIN_PRIORITY_DEFAULT,
  1,
  NULL, /* private */
  charmap_init, /* init */
  NULL /*charmap_initgui*/, 
  charmap_enforce_session,
  charmap_cleanup,
  NULL /*charmap_cleanup_gui*/,
  NULL /*charmap_register_globses_config*/,
  charmap_register_session_config,
  charmap_session_cleanup,
  NULL,
  NULL,
  NULL
};

G_MODULE_EXPORT TBluefishPlugin *getplugin(void) {
  return &bfplugin;
}
