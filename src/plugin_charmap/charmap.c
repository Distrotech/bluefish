/* Bluefish HTML Editor
 * charmap.c - the charmap panel
 *
 * Copyright (C) 2009 Olivier Sessink
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


#include "charmap.h"
#include "charmap_gui.h"

static void charmap_init(void) {
#ifdef ENABLE_NLS
  DEBUG_MSG("charmap_init, gettext domain-name=%s\n",PACKAGE"_plugin_charmap");
  bindtextdomain(PACKAGE"_plugin_charmap", LOCALEDIR);
  bind_textdomain_codeset(PACKAGE"_plugin_charmap", "UTF-8");
#endif /* ENABLE_NLS */

	main_v->sidepanel_initgui = g_slist_prepend(main_v->sidepanel_initgui,charmap_sidepanel_initgui);
	main_v->sidepanel_destroygui = g_slist_prepend(main_v->sidepanel_destroygui,charmap_sidepanel_destroygui);
}

#ifdef ENABLE_NLS
static gchar *charmap_menu_translate(const gchar * path, gpointer data) {
  return _(path);
}
#endif /* ENABLE_NLS */

static void charmap_initgui(Tbfwin* bfwin) {
	
}

static void charmap_enforce_session(Tbfwin* bfwin) {
}
static void charmap_cleanup(void) {
}

static void charmap_cleanup_gui(Tbfwin *bfwin) {
}

static GHashTable *charmap_register_globses_config(GHashTable *configlist) {
  return configlist;
}
static GHashTable *charmap_register_session_config(GHashTable *configlist, Tsessionvars *session) {
  return configlist;
}

static TBluefishPlugin bfplugin = {
  "charmap dialog",
  BFPLUGIN_VERSION,
  sizeof(Tdocument),
  sizeof(Tsessionvars),
  sizeof(Tglobalsession),
  sizeof(Tbfwin),
  sizeof(Tproject),
  sizeof(Tmain),
  NULL, /* private */
  charmap_init, /* init */
  charmap_initgui, 
  charmap_enforce_session,
  charmap_cleanup,
  charmap_cleanup_gui,
  charmap_register_globses_config,
  charmap_register_session_config,
  NULL, /* binary compatibility */
  NULL,
  NULL,
  NULL
};

G_MODULE_EXPORT TBluefishPlugin *getplugin(void) {
  return &bfplugin;
}
