/* Bluefish HTML Editor
 * languages.c - available languages
 *
 * Copyright (C) 2010 Daniel Leidert
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

#include "bluefish.h"
#include "languages.h"

GHashTable *lingua_table = NULL;

typedef struct {
	gchar* translated;
	gchar* native;
	gchar* iso639;
	gchar* locale;
} linguas_t;

static const linguas_t linguas[] = {
	{ N_( "English" ), "English", "en", "C" }, /* or better en_GB? */
#ifdef HAVE_LINGUA_BG
	{ N_( "Bulgarian"), "Български", "bg", "bg_BG" },
#endif
#ifdef HAVE_LINGUA_CS
	{ N_( "Czech"), "Český", "cs", "cs_CS" },
#endif
#ifdef HAVE_LINGUA_DA
	{ N_( "Danish"), "Dansk", "da", "da_DA" },
#endif
#ifdef HAVE_LINGUA_DE
	{ N_( "German" ), "Deutsch", "de", "de_DE" },
#endif
#ifdef HAVE_LINGUA_EL
	{ N_( "Greek"), "Ελληνικά", "el", "es_EL" },
#endif
#ifdef HAVE_LINGUA_ES
	{ N_( "Spanish"), "Español", "es", "es_ES" },
#endif
#ifdef HAVE_LINGUA_EU
	{ N_( "Basque"), "Euskara", "eu", "eu_EU" },
#endif
#ifdef HAVE_LINGUA_FI
	{ N_( "Finnish"), "Suomi", "fi", "fi_FI" },
#endif
#ifdef HAVE_LINGUA_FR
	{ N_( "French"), "Français", "fr", "fr_FR" },
#endif
#ifdef HAVE_LINGUA_GL
	{ N_( "Gallician" ), "Galego", "gl", "gl_GL" },
#endif
#ifdef HAVE_LINGUA_HU
	{ N_( "Hungarian"), "Magyar", "hu", "hu_HU" },
#endif
#ifdef HAVE_LINGUA_IT
	{ N_( "Italian"), "Italiano", "it", "it_IT" },
#endif
#ifdef HAVE_LINGUA_JA
	{ N_( "Japanese"), "日本語", "ja", "ja_JA" },
#endif
#ifdef HAVE_LINGUA_KO_KR
	{ N_( "Korean"), "Korean", "ko", "ko_KR" },
#endif
#ifdef HAVE_LINGUA_NB
	{ N_( "Norwegian"), "Norsk (bokmål)", "nb", "nb_NB" },
#endif
#ifdef HAVE_LINGUA_NL
	{ N_( "Dutch"), "Nederlands", "nl", "nl_NL" },
#endif
#ifdef HAVE_LINGUA_PL
	{ N_( "Polish"), "Polski", "pl", "pl_PL" },
#endif
#ifdef HAVE_LINGUA_PT
	{ N_( "Portuguese"), "Português", "pt", "pt_PT" },
#endif
#ifdef HAVE_LINGUA_PT_BR
	{ N_( "Brazilian Portuguese"), "Português do Brasil", "pt_br", "pt_BR" },
#endif
#ifdef HAVE_LINGUA_RO
	{ N_( "Romanian"), "Română", "ro", "ro_RO" },
#endif
#ifdef HAVE_LINGUA_RU
	{ N_( "Russian"), "Русский", "ru", "ru_RU" },
#endif
#ifdef HAVE_LINGUA_SK
	{ N_( "Slovak"), "Slovenčina", "sk", "sk_SK" },
#endif
#ifdef HAVE_LINGUA_SR
	{ N_( "Serbian"), "српски", "sr", "sr_SR" },
#endif
#ifdef HAVE_LINGUA_SV
	{ N_( "Swedish"), "Svenska", "sv", "sv_SV" },
#endif
#ifdef HAVE_LINGUA_TA
	{ N_( "Tamil"), "தமிழ்", "ta", "ta_TA" },
#endif
#ifdef HAVE_LINGUA_TR
	{ N_( "Turkish"), "Türkçe", "tr", "tr_TR" },
#endif
#ifdef HAVE_LINGUA_UK
	{ N_( "Ukrainian"), "Українська", "uk", "uk_UK" },
#endif
#ifdef HAVE_LINGUA_ZH_CN
	{ N_( "Chinese Simplified"), "Chinese Simplified", "zh", "zh_CN" },
#endif
#ifdef HAVE_LINGUA_ZH_TW
	{ N_( "Chinese Traditional"), "Chinese Traditional", "zh", "zh_TW" },
#endif
	{ NULL, NULL, NULL, NULL }
};

static void
lingua_build_hasht (void) {
	gint i;

	if (lingua_table) return;

	lingua_table = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	for (i=0; i<G_N_ELEMENTS(linguas); i++)
	{
		if (linguas[i].native && linguas[i].locale) {
			g_hash_table_insert(lingua_table, (gpointer) linguas[i].native, GINT_TO_POINTER(i));
			g_hash_table_insert(lingua_table, (gpointer) linguas[i].locale, GINT_TO_POINTER(i));
		}
	}
}

GList *
lingua_list_sorted (void) {
	GList *list = NULL;
	gint i;

	for (i=0; i<G_N_ELEMENTS(linguas); i++)
	{
		if (linguas[i].native) {
			list = g_list_append(list, (gpointer) g_strdup(linguas[i].native));
		}
	}
	
	list = g_list_sort(list, (GCompareFunc) g_strcmp0);
	list = g_list_prepend(list, (gpointer) g_strdup(_("Auto")));
	return list;
}

gchar *
lingua_lang_to_locale (const gchar *lang) {
	gint i;
	if (!lingua_table)
		lingua_build_hasht();

	i = GPOINTER_TO_INT(g_hash_table_lookup(lingua_table, lang));
	return g_strdup(linguas[i].locale);
}

const gchar *
lingua_locale_to_lang (const gchar *locale) {
	gint i;
	if (!lingua_table)
		lingua_build_hasht();

	i = GPOINTER_TO_INT(g_hash_table_lookup(lingua_table, locale));
	return linguas[i].native;
}

void lingua_cleanup(void) {
	if (lingua_table) {
		g_hash_table_destroy(lingua_table);
		lingua_table = NULL;
	}
}
