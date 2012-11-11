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
#ifdef WIN32 /* winnt.h */
	USHORT lang;
	USHORT sublang;
#endif /* or #else ? */
	gchar* iso639;
	gchar* locale;
} linguas_t;

/* note: System locale: LANG_NEUTRAL, SUBLANG_DEFAULT */
static const linguas_t linguas[] = {
	{ N_("English" ), "English",
#ifdef WIN32
	  LANG_ENGLISH, SUBLANG_DEFAULT,
#endif /* WIN32 */
	  "en", "C" }, /* or better en_GB? */

#ifdef HAVE_LINGUA_BG
	{
	  N_("Bulgarian"), "Български",
#ifdef WIN32
	  LANG_BULGARIAN, SUBLANG_DEFAULT,
#endif /* WIN32 */
	  "bg", "bg_BG"
	},
#endif /* HAVE_LINGUA_BG */

#ifdef HAVE_LINGUA_CS
	{
	  N_("Czech"), "Český",
#ifdef WIN32
	  LANG_CZECH, SUBLANG_DEFAULT,
#endif /* WIN32 */
	  "cs", "cs_CZ"
	},
#endif /* HAVE_LINGUA_CS */

#ifdef HAVE_LINGUA_DA
	{
	  N_("Danish"), "Dansk",
#ifdef WIN32
	  LANG_DANISH, SUBLANG_DEFAULT,
#endif /* WIN32 */
	  "da", "da_DK"
	},
#endif /* HAVE_LINGUA_DA */

#ifdef HAVE_LINGUA_DE
	{
	  N_("German" ), "Deutsch",
#ifdef WIN32
	  LANG_GERMAN, SUBLANG_GERMAN,
#endif /* WIN32 */
	  "de", "de_DE"
	},
#endif /* HAVE_LINGUA_DE */

#ifdef HAVE_LINGUA_EL
	{
	  N_("Greek"), "Ελληνικά",
#ifdef WIN32
	  LANG_GREEK, SUBLANG_DEFAULT,
#endif /* WIN32 */
	  "el", "es_EL"
	},
#endif /* HAVE_LINGUA_EL */

#ifdef HAVE_LINGUA_ES
	{
	  N_("Spanish"), "Español",
#ifdef WIN32
	  LANG_SPANISH, SUBLANG_SPANISH,
#endif /* WIN32 */
	  "es", "es_ES"
	},
#endif /* HAVE_LINGUA_ES */

#ifdef HAVE_LINGUA_EU
	{
	  N_("Basque"), "Euskara",
#ifdef WIN32
	  LANG_BASQUE, SUBLANG_DEFAULT,
#endif /* WIN32 */
	  "eu", "eu_ES"
	},
#endif /* HAVE_LINGUA_EU */

#ifdef HAVE_LINGUA_FI
	{
	  N_("Finnish"), "Suomi",
#ifdef WIN32
	  LANG_FINNISH, SUBLANG_DEFAULT,
#endif /* WIN32 */
	  "fi", "fi_FI"
	},
#endif /* HAVE_LINGUA_FI */

#ifdef HAVE_LINGUA_FR
	{
	  N_("French"), "Français",
#ifdef WIN32
	  LANG_FRENCH, SUBLANG_FRENCH,
#endif /* WIN32 */
	  "fr", "fr_FR"
	},
#endif /* HAVE_LINGUA_FR */

#ifdef HAVE_LINGUA_GL
	{
	  N_("Gallician" ), "Galego",
#ifdef WIN32 /* might not exist on Windows! Check before next release. */
	  LANG_GALICIAN, SUBLANG_GALICIAN_GALICIAN,
#endif /* WIN32 */
	  "gl", "gl_ES"
	},
#endif /* HAVE_LINGUA_GL */

#ifdef HAVE_LINGUA_HU
	{
	  N_("Hungarian"), "Magyar",
#ifdef WIN32
	  LANG_HUNGARIAN, SUBLANG_DEFAULT,
#endif /* WIN32 */
	  "hu", "hu_HU"
	},
#endif /* HAVE_LINGUA_HU */

#ifdef HAVE_LINGUA_IT
	{
	  N_("Italian"), "Italiano",
#ifdef WIN32
	  LANG_ITALIAN, SUBLANG_ITALIAN,
#endif /* WIN32 */
	  "it", "it_IT"
	},
#endif /* HAVE_LINGUA_IT */

#ifdef HAVE_LINGUA_JA
	{
	  N_("Japanese"), "日本語",
#ifdef WIN32
	  LANG_JAPANESE, SUBLANG_DEFAULT,
#endif /* WIN32 */
	  "ja", "ja_JP"
	},
#endif /* HAVE_LINGUA_JA */

#ifdef HAVE_LINGUA_KO_KR
	{
	  N_("Korean"), "Korean",
#ifdef WIN32
	  LANG_KOREAN, SUBLANG_KOREAN,
#endif /* WIN32 */
	  "ko", "ko_KR"
	},
#endif /* HAVE_LINGUA_KO_KR */

#ifdef HAVE_LINGUA_NB
	{
	  N_("Norwegian"), "Norsk (bokmål)",
#ifdef WIN32
	  LANG_NORWEGIAN, SUBLANG_NORWEGIAN_BOKMAL,
#endif /* WIN32 */
	  "nb", "nb_NO"
	},
#endif /* HAVE_LINGUA_NB */

#ifdef HAVE_LINGUA_NL
	{
	  N_("Dutch"), "Nederlands",
#ifdef WIN32
	  LANG_DUTCH, SUBLANG_DUTCH,
#endif /* WIN32 */
	  "nl", "nl_NL"
	},
#endif /* HAVE_LINGUA_NL */

#ifdef HAVE_LINGUA_NN
	{
	  N_("Norwegian Nynorsk"), "Norsk (nynorsk)",
#ifdef WIN32
	  LANG_NORWEGIAN, SUBLANG_NORWEGIAN_NYNORSK,
#endif /* WIN32 */
	  "nn", "nn_NO"
	},
#endif /* HAVE_LINGUA_NN */

#ifdef HAVE_LINGUA_PL
	{
	  N_("Polish"), "Polski",
#ifdef WIN32
	  LANG_POLISH, SUBLANG_DEFAULT,
#endif /* WIN32 */
	  "pl", "pl_PL"
	},
#endif /* HAVE_LINGUA_PL */

#ifdef HAVE_LINGUA_PT
	{
	  N_("Portuguese"), "Português",
#ifdef WIN32
	  LANG_PORTUGUESE, SUBLANG_PORTUGUESE,
#endif /* WIN32 */
	  "pt", "pt_PT"
	},
#endif /* HAVE_LINGUA_PT */

#ifdef HAVE_LINGUA_PT_BR
	{
	  N_("Brazilian Portuguese"), "Português do Brasil",
#ifdef WIN32
	  LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN,
#endif /* WIN32 */
	  "pt_br", "pt_BR"
	},
#endif /* HAVE_LINGUA_PT_BR */

#ifdef HAVE_LINGUA_RO
	{
	  N_("Romanian"), "Română",
#ifdef WIN32
	  LANG_ROMANIAN, SUBLANG_DEFAULT,
#endif /* WIN32 */
	  "ro", "ro_RO"
	},
#endif /* HAVE_LINGUA_RO */

#ifdef HAVE_LINGUA_RU
	{
	  N_("Russian"), "Русский",
#ifdef WIN32
	  LANG_RUSSIAN, SUBLANG_DEFAULT,
#endif /* WIN32 */
	  "ru", "ru_RU"
	},
#endif /* HAVE_LINGUA_RU */

#ifdef HAVE_LINGUA_SK
	{
	  N_("Slovak"), "Slovenčina",
#ifdef WIN32
	  LANG_SLOVAK, SUBLANG_DEFAULT,
#endif /* WIN32 */
	  "sk", "sk_SK"
	},
#endif /* HAVE_LINGUA_SK */

#ifdef HAVE_LINGUA_SR
	{
	  N_("Serbian"), "српски",
#ifdef WIN32
	  LANG_SERBIAN, SUBLANG_SERBIAN_CYRILLIC,
#endif /* WIN32 */
	  "sr", "sr_SR"
	},
#endif /* HAVE_LINGUA_SR */

#ifdef HAVE_LINGUA_SV
	{
	  N_("Swedish"), "Svenska",
#ifdef WIN32
	  LANG_SWEDISH, SUBLANG_SWEDISH,
#endif /* WIN32 */
	  "sv", "sv_SV"
	},
#endif /* HAVE_LINGUA_SV */

#ifdef HAVE_LINGUA_TA
	{
	  N_("Tamil"), "தமிழ்",
#ifdef WIN32
	  LANG_TAMIL, SUBLANG_DEFAULT,
#endif /* WIN32 */
	  "ta", "ta_TA"
	},
#endif /* HAVE_LINGUA_TA */

#ifdef HAVE_LINGUA_TR
	{
	  N_("Turkish"), "Türkçe",
#ifdef WIN32
	  LANG_TURKISH, SUBLANG_DEFAULT,
#endif /* WIN32 */
	  "tr", "tr_TR"
	},
#endif /* HAVE_LINGUA_TR */

#ifdef HAVE_LINGUA_UK
	{
	  N_("Ukrainian"), "Українська",
#ifdef WIN32
	  LANG_UKRAINIAN, SUBLANG_DEFAULT,
#endif /* WIN32 */
	  "uk", "uk_UA"
	},
#endif /* HAVE_LINGUA_UK */

#ifdef HAVE_LINGUA_ZH_CN
	{ N_("Chinese Simplified"), "Chinese Simplified",
#ifdef WIN32
	LANG_CHINESE_SIMPLIFIED, SUBLANG_CHINESE_SIMPLIFIED,
#endif /* WIN32 */
	  "zh", "zh_CN"
	},
#endif /* HAVE_LINGUA_ZH_CN */

#ifdef HAVE_LINGUA_ZH_TW
	{
	  N_("Chinese Traditional"), "Chinese Traditional",
#ifdef WIN32
	LANG_CHINESE_TRADITIONAL, SUBLANG_CHINESE_TRADITIONAL,
#endif /* WIN32 */
	  "zh", "zh_TW"
	},
#endif /* HAVE_LINGUA_ZH_TW */

	{
	  NULL, NULL,
#ifdef WIN32
	  LANG_NEUTRAL, SUBLANG_DEFAULT,
#endif /* WIN32 */
	  NULL, NULL
	}
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

#ifdef WIN32
gboolean
lingua_set_thread_locale_on_windows (const gchar *locale) {
	gint i;
	gboolean retsuccess;

	if (!lingua_table)
		lingua_build_hasht();
	
	i = GPOINTER_TO_INT(g_hash_table_lookup(lingua_table, locale));
	retsuccess = SUCCEEDED(SetThreadLocale(MAKELCID(MAKELANGID(linguas[i].lang, linguas[i].sublang), SORT_DEFAULT)));
	if (!retsuccess) {
		g_warning("setThreadLocaleOnWindows Could not set thread locale for %s.", locale);
		g_return_val_if_reached(FALSE);
	} else {
		/* set LC_ALL too?, run gtk_set_locale()? */
		return TRUE;
	}
}
#endif

void lingua_cleanup(void) {
	if (lingua_table) {
		g_hash_table_destroy(lingua_table);
		lingua_table = NULL;
	}
}
