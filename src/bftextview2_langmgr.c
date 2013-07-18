/* Bluefish HTML Editor
 * bftextview2_langmgr.c
 *
 * Copyright (C) 2008,2009,2010,2011,2012,2013 Olivier Sessink
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

#include <libxml/xmlreader.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <stdlib.h>				/* qsort */

#include "bftextview2.h"
#include "bftextview2_private.h"
#include "bftextview2_langmgr.h"
#include "bftextview2_patcompile.h"

#include "bluefish.h"
#include "bf_lib.h"
#include "bfwin_uimanager.h"
#include "document.h"
#include "stringlist.h"

#define UNDEFINED -1

/*#undef DBG_PARSING
#define DBG_PARSING g_print
#undef DBG_MSG
#define DBG_MSG g_print*/

typedef struct {
	GHashTable *patterns;
	GHashTable *contexts;
	Tscantable *st;				/* while not finished */
	GHashTable *commentid_table;	/* a hash table with a comment ID string as key and an integer position in the GArray as value */
	gchar *smartindentchars;
	gchar *smartoutdentchars;
	Tbflang *bflang;
	gboolean load_completion;
	gboolean load_reference;
	guint reference_size;
	gboolean autoclose_tags;
	gboolean stretch_tag_block;
#ifdef HAVE_LIBENCHANT
	gboolean default_spellcheck;
	gboolean spell_decode_entities;
#endif
} Tbflangparsing;

typedef enum {
	ac_mode_none,
	ac_mode_string,
	ac_mode_append
} Tautocomplete_mode;

typedef struct {
	gchar *string;
	Tautocomplete_mode mode;
	gint backup_cursor;
} Tautocomplete;

typedef struct {
	gboolean done_scanning;
	GHashTable *bflang_lookup;
	GList *bflang_list;
	GtkTextTagTable *tagtable;	/* a GtkTextTagTable uses a hashtable internally, so lookups by name are fast */
	gboolean load_reference;
	GHashTable *configured_styles;	/* key: a NULL terminated char **array with first value the language name,
									   second value the highlight name, third NULL
									   and as value a gchar * with the name of the GtkTextTag to use */
	GHashTable *bflang_options;	/* key: a NULL terminated char **array with first value the language name,
								   second value the option name, third NULL
								   and as value a gchar * with the value of the option */
	GHashTable *option_descriptions; /* key: the option name, value a description */
	GtkTextTag **highlight_tags;	/* text used for highlighting, so not _needscanning_ or _needspellcheck_ or _folded_ and such */
#ifdef HAVE_LIBENCHANT
	GtkTextTag **need_spellcheck_tags;
	GtkTextTag **no_spellcheck_tags;
#endif
} Tlangmgr;
#ifdef HAVE_LIBENCHANT
static Tlangmgr langmgr = { FALSE, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL };
#else
static Tlangmgr langmgr = { FALSE, NULL, NULL, NULL, 0, NULL, NULL };
#endif

/* utils */

/* these hash functions hash the first 2 strings in a gchar ** */
gboolean
arr2_equal(gconstpointer v1, gconstpointer v2)
{
	const gchar **arr1 = (const gchar **) v1;
	const gchar **arr2 = (const gchar **) v2;

	return ((strcmp((char *) arr1[0], (char *) arr2[0]) == 0) &&
			(strcmp((char *) arr1[1], (char *) arr2[1]) == 0));
}

guint
arr2_hash(gconstpointer v)
{
	/* modified after g_str_hash */
	const signed char **tmp = (const signed char **) v;
	const signed char *p;
	guint32 h = *tmp[0];
	if (h) {
		gshort i = 0;
		while (i < 2) {
			p = tmp[i];
			for (p += 1; *p != '\0'; p++)
				h = (h << 5) - h + *p;
			i++;
		}
	}
	return h;
}

/* langmgr code */

static void
skip_to_end_tag(xmlTextReaderPtr reader, int depth)
{
	while (xmlTextReaderRead(reader) == 1) {
		if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT
			&& depth == xmlTextReaderDepth(reader)) {
			DBG_PARSING("skip_to_end_tag, found node %s type %d with depth %d, break!\n", xmlTextReaderName(reader),
						xmlTextReaderNodeType(reader), xmlTextReaderDepth(reader));
			break;
		}
	}
}

static GtkTextTag *
langmrg_create_style(const gchar * name, const gchar * fgcolor, const gchar * bgcolor, const gchar * bold,
					 const gchar * italic)
{
	GtkTextTag *tag;
	gboolean newtag = FALSE;
	if (!name || name[0] == '\0')
		return NULL;

	tag = gtk_text_tag_table_lookup(langmgr.tagtable, name);
	if (!tag) {
		tag = gtk_text_tag_new(name);
		DBG_MSG("creating tag %p for new textstyle %s\n", tag, name);
		newtag = TRUE;
	}
	/*else {
	   g_print("modyfying tag %p for textstyle %s\n",tag,name);
	   } */
	if (fgcolor && fgcolor[0] != '\0') {
		g_object_set(tag, "foreground", fgcolor, "foreground-set", TRUE, NULL);
	} else if (!newtag) {
		g_object_set(tag, "foreground-set", FALSE, NULL);
	}
	if (bgcolor && bgcolor[0] != '\0') {
		g_object_set(tag, "background", bgcolor, "background-set", TRUE, NULL);
	} else if (!newtag) {
		g_object_set(tag, "background-set", FALSE, NULL);
	}
	if (bold && (bold[0] == '1' || bold[0] == '2')) {
		g_object_set(tag, "weight", PANGO_WEIGHT_BOLD, NULL);
	} else if (!newtag) {
		g_object_set(tag, "weight", PANGO_WEIGHT_NORMAL, NULL);
	}
	if (italic && (italic[0] == '1' || italic[0] == '2')) {
		g_object_set(tag, "style", PANGO_STYLE_ITALIC, NULL);
	} else if (!newtag) {
		g_object_set(tag, "style", PANGO_STYLE_NORMAL, NULL);
	}
/*
	if (fgcolor && fgcolor[0]!='\0') g_object_set(tag, "foreground", fgcolor, NULL);
	if (bgcolor && bgcolor[0]!='\0') g_object_set(tag, "background", bgcolor, NULL);
	if (bold) g_object_set(tag, "weight", PANGO_WEIGHT_BOLD, NULL);
	if (italic) g_object_set(tag, "style", PANGO_STYLE_ITALIC, NULL);
	*/
	if (newtag) {
		gtk_text_tag_table_add(langmgr.tagtable, tag);
		g_object_unref(tag);
	}
	return tag;
}

static GtkTextTag **
texttag_array_from_list(GList * thelist)
{
	gint i = 0;
	GList *tmplist;
	GtkTextTag **retval = (GtkTextTag **) g_new0(gpointer, g_list_length(thelist) + 1);
	for (tmplist = g_list_first(thelist); tmplist; tmplist = tmplist->next) {
		retval[i] = tmplist->data;
		i++;
	}
	g_list_free(thelist);
	return retval;
}

void
langmgr_reload_user_styles(void)
{
	GList *tmplist, *highlightlist = NULL;
#ifdef HAVE_LIBENCHANT
	GList *needlist = NULL, *noscanlist = NULL;
#endif
	gint i=0;
	GtkTextTag *tag;
	const gchar *defaultarr[][7] = {
		/* name, foreground-color, background-color, bold, italics, spell-check */
		{"preprocessor", "#aaaa00", "", "0", "0", "0", NULL},
		{"comment", "#555555", "", "0", "1", "1", NULL},
		{"string", "#009900", "", "0", "0", "1", NULL},
		{"string-no-spell-check", "#009900", "", "0", "0", "0", NULL},
		{"special-string", "#005400", "", "0", "0", "1", NULL},
		{"special-string2", "#00D000", "", "0", "0", "1", NULL},
		{"special-string3", "#999900", "", "0", "0", "1", NULL},
		{"type", "", "", "1", "0", "0", NULL},
		{"special-type", "#990000", "", "1", "", "0", NULL},
		{"function", "#000099", "", "0", "0", "0", NULL},
		{"special-function", "#990000", "", "", "", "0", NULL},
		{"keyword", "#000000", "", "1", "0", "0", NULL},
		{"special-keyword", "#005050", "", "1", "", "0", NULL},
		{"value", "#0000FF", "", "0", "0", "1", NULL},
		{"special-value", "#0000FF", "", "1", "", "0", NULL},
		{"variable", "#990000", "", "1", "0", "0", NULL},
		{"operator", "#C86400", "", "0", "0", "0", NULL},
		{"tag", "#990099", "", "1", "", "0", NULL},
		{"special-tag", "#990000", "", "1", "", "0", NULL},
		{"special-tag2", "#005500", "", "1", "", "0", NULL},
		{"special-tag3", "#FF9900", "", "1", "", "0", NULL},
		{"attribute", "#000099", "", "", "", "0", NULL},
		{"special-attribute", "#FF0000", "", "", "", "0", NULL},
		{"brackets", "#000000", "", "1", "0", "0", NULL},
		{"warning", "#FF0000", "", "1", "0", "0", NULL},
		{"special-warning", "", "#FF0000", "", "", "0", NULL},
		{NULL, NULL, NULL, NULL, NULL, NULL, NULL}
	};
	/* because the order of the styles is important (last added GtkTextTag is most important)
	   we begin with the last style in the list */
	for (tmplist = g_list_last(main_v->props.textstyles); tmplist; tmplist = tmplist->prev) {
		gchar **arr = (gchar **) tmplist->data;
		if (g_strv_length(arr) == 6) {
			tag = langmrg_create_style(arr[0], arr[1], arr[2], arr[3], arr[4]);
			highlightlist = g_list_prepend(highlightlist, tag);
#ifdef HAVE_LIBENCHANT
			if (arr[5][0] == '1')
				needlist = g_list_prepend(needlist, tag);
			else
				noscanlist = g_list_prepend(noscanlist, tag);
#endif
		}
	}
	/* now see if all the default styles are defined, if not -> add them */
	while (defaultarr[i][0]) {
		if (gtk_text_tag_table_lookup(langmgr.tagtable, defaultarr[i][0])==NULL) {
			tag = langmrg_create_style(defaultarr[i][0], defaultarr[i][1], defaultarr[i][2], defaultarr[i][3], defaultarr[i][4]);
			highlightlist = g_list_prepend(highlightlist, tag);
#ifdef HAVE_LIBENCHANT
			if (defaultarr[i][5][0] == '1')
				needlist = g_list_prepend(needlist, tag);
			else
				noscanlist = g_list_prepend(noscanlist, tag);
#endif
			main_v->props.textstyles = g_list_append(main_v->props.textstyles, g_strdupv((gchar **) defaultarr[i]));
		}
		i++;
	}

	langmgr.highlight_tags = texttag_array_from_list(highlightlist);
#ifdef HAVE_LIBENCHANT
	langmgr.need_spellcheck_tags = texttag_array_from_list(needlist);
	langmgr.no_spellcheck_tags = texttag_array_from_list(noscanlist);
#endif
}

static void
langmgr_insert_user_option(gchar * lang, gchar * option, gchar * val)
{
	if (lang && option && val) {
		/*g_print("insert %s %s: %s\n",lang,option,val); */
		g_hash_table_insert(langmgr.bflang_options, array_from_arglist(lang, option, NULL), g_strdup(val));
	}
}

static const gchar *
lookup_user_option(const gchar * lang, const gchar * option)
{
	if (lang && option) {
		const gchar *arr[] = { lang, option, NULL };
		return g_hash_table_lookup(langmgr.bflang_options, arr);
	}
	return NULL;
}

static gboolean
lookup_block_foldable(const gchar *lang_name, const gchar *block_name)
{
	gchar *name;
	const gchar *tmp;
	gboolean foldable;
	name = g_strconcat(block_name, "_foldable", NULL);
	tmp = lookup_user_option(lang_name, name);
	foldable = !(tmp && tmp[0] == '0');
	g_free(name);
	return foldable;
}

void
langmgr_reload_user_options(void)
{
	GList *tmplist;
	const gchar *tmp;
	gint i;
	g_hash_table_remove_all(langmgr.bflang_options);
	for (tmplist = g_list_first(main_v->props.bflang_options); tmplist; tmplist = tmplist->next) {
		gchar **arr = (gchar **) tmplist->data;
		langmgr_insert_user_option(arr[0], arr[1], arr[2]);
	}

	/* loop over the languages and set some of the options */

	tmplist = g_list_first(langmgr.bflang_list);
	while (tmplist) {
		tmp = lookup_user_option(BFLANG(tmplist->data)->name, "show_in_menu");
		BFLANG(tmplist->data)->in_menu = !(tmp && tmp[0] == '0');
		if (BFLANG(tmplist->data)->st) {
			for (i = 1; i < BFLANG(tmplist->data)->st->blocks->len; i++) {
				if (g_array_index(BFLANG(tmplist->data)->st->blocks, Tpattern_block, i).name) {
					g_array_index(BFLANG(tmplist->data)->st->blocks, Tpattern_block, i).foldable
							= lookup_block_foldable(
									BFLANG(tmplist->data)->name,
									g_array_index(BFLANG(tmplist->data)->st->blocks, Tpattern_block, i).name
									);
				}
			}
		}
		tmplist = tmplist->next;
	}
}

void
langmgr_reload_user_highlights(void)
{
	GList *tmplist;
	g_hash_table_remove_all(langmgr.configured_styles);
	for (tmplist = g_list_first(main_v->props.highlight_styles); tmplist; tmplist = tmplist->next) {
		gchar **arr = (gchar **) tmplist->data;
		if (arr[0] && arr[1] && arr[2]) {
			/*g_print("set style %s for highlight %s:%s\n",arr[2],arr2[0],arr2[1]); */
			g_hash_table_insert(langmgr.configured_styles, array_from_arglist(arr[0], arr[1], NULL),
								g_strdup(arr[2]));
		}
	}
	/* now apply the changes */
	tmplist = g_list_first(langmgr.bflang_list);
	while (tmplist) {
		Tbflang *bflang = tmplist->data;
		if (bflang->st) {
			/*g_print("langmgr_reload_user_highlights, bflang name %s\n",bflang->name); */
			g_list_free(bflang->tags);
			bflang->tags = bftextview2_scantable_rematch_highlights(bflang->st, bflang->name);
		}
		tmplist = g_list_next(tmplist);
	}
}

gchar *
langmgr_get_option_description(const gchar *optionname)
{
	return g_hash_table_lookup(langmgr.option_descriptions, optionname);
}

static gchar *
langmgr_lookup_style_for_highlight(const gchar * lang, const gchar * highlight)
{
	const gchar *arr[] = { lang, highlight, NULL };
	/*g_print("langmgr_lookup_style_for_highlight ,lookup %s:%s\n",lang,highlight); */
	return g_hash_table_lookup(langmgr.configured_styles, arr);
}

GtkTextTag *
langmrg_lookup_tag_highlight(const gchar * lang, const gchar * highlight)
{
	GtkTextTag *tag = NULL;
	if (lang && highlight) {
		gchar *style = langmgr_lookup_style_for_highlight(lang, highlight);
		if (style)
			tag = gtk_text_tag_table_lookup(langmgr.tagtable, style);
		else
			tag = gtk_text_tag_table_lookup(langmgr.tagtable, highlight);
	}
	/*g_print("found tag %p for lang %s highlight %s\n",tag,lang,highlight); */
	return tag;
}

static void
foreachdoc_lcb(Tdocument * doc, gpointer data)
{
	if (BLUEFISH_TEXT_VIEW(doc->view)->bflang == data) {
/*		g_print("calling bluefish_text_view_rescan for %p\n",doc);*/
		bluefish_text_view_rescan(BLUEFISH_TEXT_VIEW(doc->view));
	}
}

/* this is called in the mainloop again */
static gboolean
build_lang_finished_lcb(gpointer data)
{
	Tbflangparsing *bfparser = data;
	DEBUG_SIG("build_lang_finished_lcb, priority=%d\n",BUILD_LANG_FINISHED_PRIORITY);
	if (bfparser->st) {
		bfparser->bflang->st = bfparser->st;
		/*bfparser->bflang->line = bfparser->line;
		   bfparser->bflang->block = bfparser->block; */
/*		g_print("build_lang_finished_lcb, bflang %p, line=%p, block=%p\n",bfparser->bflang, bfparser->bflang->line, bfparser->bflang->block);*/
	} else {
		bfparser->bflang->no_st = TRUE;
	}
	bfparser->bflang->smartindentchars = bfparser->smartindentchars;
	bfparser->bflang->smartoutdentchars = bfparser->smartoutdentchars;
#ifdef HAVE_LIBENCHANT
	bfparser->bflang->default_spellcheck = bfparser->default_spellcheck;
	bfparser->bflang->spell_decode_entities = bfparser->spell_decode_entities;
#endif
	bfparser->bflang->parsing = FALSE;
	DBG_PARSING("build_lang_finished_lcb..\n");
	/* now walk and rescan all documents that use this bflang */
	if (bfparser->bflang->st) {
		alldocs_foreach(foreachdoc_lcb, bfparser->bflang);
	}
	/* cleanup the parsing structure */
	g_hash_table_destroy(bfparser->patterns);
	g_hash_table_destroy(bfparser->contexts);
	g_hash_table_destroy(bfparser->commentid_table);
	/*g_hash_table_destroy(bfparser->setoptions); */
	g_slice_free(Tbflangparsing, bfparser);
	return FALSE;
}

typedef enum {
	attribtype_int,
	attribtype_string,
	attribtype_boolean
} Tattribtype;

typedef struct {
	gchar *name;
	gpointer var;
	Tattribtype type;
} Tattrib;

static gchar *
process_condition(Tbflang *bflang, gchar *value)
{
	gchar *tok1, *tok2, *lookupval;
	tok1 = strchr(value, '?');
	if (!tok1)
		return value;
	tok2 = strchr(tok1, ':');
	if (!tok2)
		return value;
	*tok1 = '\0';
	*tok2 = '\0';
	lookupval = (gchar *) lookup_user_option(bflang->name, value);
	if (lookupval && lookupval[0]=='1') {
		return tok1+1;
	}
	return tok2+1;
}

static void
parse_attributes(Tbflang *bflang,xmlTextReaderPtr reader, Tattrib *attribs, gint num_attribs)
{
	while (xmlTextReaderMoveToNextAttribute(reader)) {
		gint i;
		xmlChar *aname = xmlTextReaderName(reader);
		for (i=0;i<num_attribs;i++) {
			if (xmlStrEqual(aname, (xmlChar *)attribs[i].name)) {
				gchar *value = (gchar *)xmlTextReaderValue(reader);
				gchar *useval = value;
				if (strncmp(value, "option:", 7)==0) {
					/*g_print("lookup option %s\n",value+7);*/
					useval = (gchar *) lookup_user_option(bflang->name, value+7);
					/*g_print("useval=%s\n",useval);*/
				} else if (strncmp(value, "condition:",10)==0) {
					useval = process_condition(bflang,value+10);
				}
				switch (attribs[i].type) {
					case attribtype_int:
						*((gint *)attribs[i].var) = GPOINTER_TO_INT(useval)?g_ascii_strtoll(useval, NULL, 10):0;
					break;
					case attribtype_string:
						*((gchar **)attribs[i].var) = g_strdup(useval?useval:"");
					break;
					case attribtype_boolean:
						*((gboolean *)attribs[i].var) = useval?(useval[0]=='1'):FALSE;
					break;
				}
				xmlFree(value);
				break;
			}
		}
		xmlFree(aname);
	}
}

static void
process_header(xmlTextReaderPtr reader, Tbflang * bflang)
{
	const gchar *tmp;
	while (xmlTextReaderRead(reader) == 1) {
		int nodetype = xmlTextReaderNodeType(reader);
		xmlChar *name;

		if (nodetype == 14 /* #text */) {
			continue;
		}

		name = xmlTextReaderName(reader);
		if (xmlStrEqual(name, (xmlChar *) "mime")) {
			gchar *mimetype=NULL;
			Tattrib attribs[] = {{"type", &mimetype, attribtype_string}};
			parse_attributes(bflang,reader, attribs, 1);
			if (mimetype) {
				bflang->mimetypes = g_list_prepend(bflang->mimetypes, mimetype);
			}
		} else if (xmlStrEqual(name, (xmlChar *) "option")) {
			gchar *optionname = NULL, *description = NULL;
			gboolean defaultval = FALSE;
			gboolean free_description=TRUE;
			Tattrib attribs[] = {{"name", &optionname, attribtype_string},
					{"description", &description, attribtype_string},
					{"default", &defaultval, attribtype_boolean}};
			parse_attributes(bflang,reader, attribs, 3);
			if (optionname) {
				const gchar *val = lookup_user_option(bflang->name, optionname);
				if (!val) {
					/* not set by the user */
					main_v->props.bflang_options =
						g_list_prepend(main_v->props.bflang_options,
									   array_from_arglist(bflang->name, optionname, defaultval ? "1" : "0",
														  NULL));
					langmgr_insert_user_option(bflang->name, optionname, defaultval ? "1" : "0");
				}
				if (description) {
					gpointer val = g_hash_table_lookup(langmgr.option_descriptions, optionname);
/*					g_print("optionname=%s, description=%s, val=%p\n",optionname,description, val);*/
					if (!val) {
						g_hash_table_insert(langmgr.option_descriptions, optionname, description);
						free_description = FALSE;
					}

				}
				if (free_description) {
					g_free(optionname);
					g_free(description);
				}
			}
		} else if (xmlStrEqual(name, (xmlChar *) "highlight")) {
			gchar *name = NULL, *style = NULL;
			Tattrib attribs[] = {{"name", &name, attribtype_string},
					{"style", &style, attribtype_string}};
			parse_attributes(bflang,reader, attribs, 2);
			if (name) {
				gchar *use_textstyle = langmgr_lookup_style_for_highlight(bflang->name, name);
				if (use_textstyle) {	/* we have a user-configured textstyle for this highlight */
					if (!gtk_text_tag_table_lookup(langmgr.tagtable, use_textstyle)) {
						/* the user-set style does not exist, create an empty user-set style */
						gchar *arr[] = { use_textstyle, "", "", "", "", NULL };
						g_print("Warning, textstyle %s is set by the user but does not exist\n",
								use_textstyle);
						main_v->props.textstyles = g_list_prepend(main_v->props.textstyles, g_strdupv(arr));
					}
				} else if (style) {	/* no textstyle was configured, set the provided style */
					if (!gtk_text_tag_table_lookup(langmgr.tagtable, style)) {
						gchar *arr[] = { style, "", "", "", "", NULL };
						g_print("Possible error in language file, textstyle %s does not exist\n", style);
						main_v->props.textstyles = g_list_prepend(main_v->props.textstyles, g_strdupv(arr));
					}
					g_hash_table_insert(langmgr.configured_styles,
										array_from_arglist(bflang->name, name, NULL), g_strdup(style));
					main_v->props.highlight_styles =
						g_list_prepend(main_v->props.highlight_styles,
									   array_from_arglist(bflang->name, name, style, NULL));
				}
			}
			g_free(name);
			g_free(style);
		} else if (xmlStrEqual(name, (xmlChar *) "header")) {
			xmlFree(name);
			break;
		}
		xmlFree(name);
	}

	tmp = lookup_user_option(bflang->name, "show_in_menu");
	bflang->in_menu = !(tmp && tmp[0] == '0');
}

static gboolean
do_parse(Tbflangparsing * bfparser, const gchar *class, const gchar *notclass)
{
	const gchar *haveclass=NULL, *havenotclass=NULL;
/*
	if class is not set and notclass is not set we return TRUE

	if only class is set, and haveclass is not set, we return TRUE
	if only class is set, and haveclass is also set, we return haveclass[0] == '1'

	if only notclass is set, and havenotclass is not set, we return FALSE
	if only notclass is set, and havenotclass is also set, we return haveclass[0] != '1'

	if both haveclass and havenotclass are both set, the havenotclass overrules
	if both class and notclass are both set, the notclass overrules (return FALSE)


*/
	if (!class && !notclass)
		return TRUE;

	if (class)
		haveclass = lookup_user_option(bfparser->bflang->name, class);
	if (notclass)
		havenotclass = lookup_user_option(bfparser->bflang->name, notclass);
	DBG_PARSING("do_parse, class=%s, haveclass=%s, notclass=%s, havenotclass=%s\n",class, haveclass, notclass, havenotclass);
	if (havenotclass)
		return havenotclass[0] != '1';

	if (haveclass)
		return haveclass[0] == '1';

	if (notclass)
		return FALSE;

	return TRUE;
}

static void
free_autocomplete(GSList *list)
{
	GSList *tmpslist;
	for (tmpslist = list;tmpslist;tmpslist = g_slist_next(tmpslist)) {
		Tautocomplete *ac = tmpslist->data;
		if (ac->mode == ac_mode_append)
			g_free(ac->string);
		g_slice_free(Tautocomplete, ac);
	}
	g_slist_free(list);
}

static void
add_autocomplete(Tbflangparsing * bfparser, guint16 matchnum, GSList *list, const gchar *tag)
{
	GSList *tmpslist;
	for (tmpslist = list;tmpslist;tmpslist = g_slist_next(tmpslist)) {
		Tautocomplete *ac = tmpslist->data;
		if (ac->mode != ac_mode_none) {
			gchar *string = ac->string;
			gboolean freestring=FALSE;
			gint backup_cursor = ac->backup_cursor;
			if (ac->string == NULL && tag && ac->mode == ac_mode_string) {
				string = g_strconcat("></", tag, ">", NULL);
				backup_cursor = strlen(string)-1;
				freestring=TRUE;
			}
			match_add_autocomp_item(bfparser->st, matchnum,
										(ac->mode == ac_mode_string) ? string : NULL,
										(ac->mode == ac_mode_append) ? string : NULL,
										backup_cursor);
			if (freestring)
				g_free(string);
		}
	}
}

static void
process_autocomplete(xmlTextReaderPtr reader, Tbflangparsing * bfparser, GSList **list)
{
	gboolean enable=TRUE;
	gchar *string=NULL, *append=NULL, *class=NULL, *notclass=NULL;
	gint backup_cursor=0;
	gboolean enabled;
	Tautocomplete *ac;
	Tattrib attribs[] = {
			{"enable", &enable, attribtype_boolean},
			{"string", &string, attribtype_string},
			{"append", &append, attribtype_string},
			{"class", &class, attribtype_string},
			{"notclass", &notclass, attribtype_string},
			{"backup_cursor", &backup_cursor, attribtype_int}
	};
	if (!bfparser->load_completion)
		return;

	parse_attributes(bfparser->bflang,reader, attribs, 6);
	enabled = do_parse(bfparser, class, notclass);
	g_free(class);
	g_free(notclass);
	if (!enabled || (string && string[0]=='\0') || (append && append[0] == '\0')) {
		g_free(string);
		g_free(append);
		return;
	}
	ac = g_slice_new0(Tautocomplete);
	if (!enable) {
		ac->mode = ac_mode_none;
		g_free(append);
		g_free(string);
	} else if (string) {
		ac->mode = ac_mode_string;
		ac->string = string;
		g_free(append);
	} else if (append) {
		ac->mode = ac_mode_append;
		ac->string = append;
		g_free(string);
	} else {
		ac->mode = ac_mode_string;
		ac->string = NULL; /* use the pattern */
		g_free(string);
		g_free(append);
	}
	ac->backup_cursor = backup_cursor;
	/*g_print("prepend item %s to autocomplete\n",ac->string);*/
	*list = g_slist_prepend(*list, ac);
}

/* declaration needed for recursive calling */
static gint16 process_scanning_context(xmlTextReaderPtr reader, Tbflangparsing * bfparser,
									   GQueue * contextstack);

static guint16
process_scanning_element(xmlTextReaderPtr reader, Tbflangparsing * bfparser, gint16 context,
						 GQueue * contextstack, const gchar * ih_highlight,
						 gint ih_case_insens, gint ih_is_regex, GSList *ih_autocomplete)
{
	guint16 matchnum = 0;
	gboolean add_element;
	gchar *pattern = NULL, *idref = NULL, *highlight = NULL, *blockstartelement = NULL, *blockhighlight =
		NULL, *block_name = NULL, *class = NULL, *notclass = NULL, *id =	NULL;
	gboolean starts_block = FALSE, ends_block = FALSE, is_empty, tagclose_from_blockstack = FALSE, stretch_blockstart=FALSE;
	gint case_insens = UNDEFINED, is_regex = UNDEFINED;
	gint ends_context = 0;
	gint identifier_mode=0, identifier_jump=0, identifier_autocomp=0;
	GSList *autocomplete=NULL;
	is_empty = xmlTextReaderIsEmptyElement(reader);

	Tattrib attribs[] = {{"pattern", &pattern, attribtype_string},
					{"id", &id, attribtype_string},
					{"idref", &idref, attribtype_string},
					{"highlight", &highlight, attribtype_string},
					{"blockhighlight", &blockhighlight, attribtype_string},
					{"blockstartelement", &blockstartelement, attribtype_string},
					{"class", &class, attribtype_string},
					{"notclass", &notclass, attribtype_string},
					{"is_regex", &is_regex, attribtype_boolean},
					{"starts_block", &starts_block, attribtype_boolean},
					{"block_name", &block_name, attribtype_string},
					{"ends_block", &ends_block, attribtype_boolean},
					{"case_insens", &case_insens, attribtype_boolean},
					{"ends_context", &ends_context, attribtype_int},
					{"tagclose_from_blockstack", &tagclose_from_blockstack, attribtype_boolean},
					{"stretch_blockstart", &stretch_blockstart, attribtype_int},
					{"identifier_mode", &identifier_mode, attribtype_int},
					{"identifier_jump", &identifier_jump, attribtype_int},
					{"identifier_autocomp", &identifier_autocomp, attribtype_int}
					};
	parse_attributes(bfparser->bflang,reader, attribs, 19);
	if (stretch_blockstart && ends_block) {
		g_print("Error in language file, id %s / pattern %s has mutually exclusive options stretch_blockstart and ends_block both enabled\n", id?id:"-", pattern?pattern:"null");
		stretch_blockstart = FALSE;
	}
	if ((stretch_blockstart || ends_block) && !blockstartelement) {
		g_print("Error in language file, id %s / pattern %s has stretch_blockstart or ends_block enabled without blockstartelement\n", id?id:"-", pattern?pattern:"null");
	}
	add_element = do_parse(bfparser, class, notclass);
/*	g_print("add_element=%d for class=%s and notclass=%s\n",add_element, class, notclass);*/
	if (add_element) {
		if (idref && idref[0] && !id && !pattern) {
			guint16 matchnum;
			matchnum = GPOINTER_TO_INT(g_hash_table_lookup(bfparser->patterns, idref));
			if (!matchnum) {
				g_print("Error in language file, element with id %s does not exist\n", idref);
			} else {
				compile_existing_match(bfparser->st, matchnum, context);
				if (g_array_index(bfparser->st->matches, Tpattern, matchnum).nextcontext < 0
					&& (-1 * g_array_index(bfparser->st->matches, Tpattern, matchnum).nextcontext) >=
					g_queue_get_length(contextstack)) {
					g_print
						("Possible error in language file, idref %s ends_context=%d, but has only %d parent contexts\n",
						 idref, ends_context, g_queue_get_length(contextstack));
				}

			}
		} else if (pattern && pattern[0]) {
			gchar *reference = NULL;
			gboolean foldable=TRUE;
			guint16 blockstartelementum = 0, nextcontext = 0;
			DBG_PARSING("pattern %s\n", pattern);
			if (ends_context) {
				/* the nth number in the stack */
				nextcontext = -1 * ends_context;	/*GPOINTER_TO_INT(g_queue_peek_nth(contextstack,ends_context)); */
				if (ends_context >= g_queue_get_length(contextstack)) {
					g_print
						("Possible error in language file, id %s / pattern %s has ends_context=%d, but has only %d parent contexts\n",
						 id?id:"-", pattern?pattern:"null", ends_context, g_queue_get_length(contextstack));
				}
			}
			if (blockstartelement) {
				blockstartelementum =
					GPOINTER_TO_INT(g_hash_table_lookup(bfparser->patterns, blockstartelement));
				if (!blockstartelementum) {
					g_print("Error in language file, blockstartelement %s does not exist\n",
							blockstartelement);
				}
				DBG_PARSING("got blockstartelementum %d for blockstartelement %s, ends_block=%d\n",
							blockstartelementum, blockstartelement, ends_block);
			}
			matchnum = add_pattern_to_scanning_table(bfparser->st,
								pattern,
								is_regex != UNDEFINED ? is_regex : (ih_is_regex !=UNDEFINED ? ih_is_regex : FALSE),
								case_insens != UNDEFINED ? case_insens : (ih_case_insens != UNDEFINED ? ih_case_insens :FALSE),
								context);
			if (block_name) {
				foldable = lookup_block_foldable(bfparser->bflang->name, block_name);
			}
			pattern_set_blockmatch(bfparser->st, matchnum,
								starts_block,
								ends_block,
								blockstartelementum,
								blockhighlight,
								block_name,
								foldable);
			pattern_set_runtime_properties(bfparser->st, matchnum,
								 highlight ? highlight : ih_highlight,
								 nextcontext,
								 tagclose_from_blockstack, /* the very special case for the generix xml tag close pattern */
								 stretch_blockstart,
								 identifier_mode,
								 identifier_jump,
								 identifier_autocomp);
			DBG_PARSING("add matchnum %d to hash table for key %s, starts_block=%d\n", matchnum, pattern,
						starts_block);
			g_hash_table_insert(bfparser->patterns, g_strdup(id ? id : pattern),
								GINT_TO_POINTER((gint) matchnum));
			/* now check if there is a deeper context */
			if (!is_empty) {
				while (xmlTextReaderRead(reader) == 1) {
					xmlChar *name;
					int nodetype = xmlTextReaderNodeType(reader);
					if (nodetype == 14 /* #text */) {
						continue;
					}
					name = xmlTextReaderName(reader);
					if (xmlStrEqual(name, (xmlChar *) "context")) {
						DBG_PARSING("in pattern, found countext\n");
						nextcontext = process_scanning_context(reader, bfparser, contextstack);
						match_set_nextcontext(bfparser->st, matchnum, nextcontext);
					} else if (xmlStrEqual(name, (xmlChar *) "reference")) {
						DBG_PARSING("in pattern, found reference\n");
						if (!xmlTextReaderIsEmptyElement(reader)) {
							if (langmgr.load_reference && bfparser->load_reference)
								reference = (gchar *) xmlTextReaderReadInnerXml(reader);
							DBG_PARSING("reference=%s\n", reference);
							skip_to_end_tag(reader, xmlTextReaderDepth(reader));
						}
					} else if (xmlStrEqual(name, (xmlChar *) "element")) {
						DBG_PARSING("in element, found 'element' --> must be end-of-element\n");
						xmlFree(name);
						break;
					} else if (nodetype == 1 && xmlStrEqual(name, (xmlChar *) "autocomplete")) {
						process_autocomplete(reader, bfparser, &autocomplete);
					} else {
						DBG_PARSING("process_scanning_element, parsing element with name %s\n", name);
					}
					xmlFree(name);
				}
			}
			add_autocomplete(bfparser, matchnum, autocomplete?autocomplete:ih_autocomplete, NULL);
			if (reference != NULL) {
				bfparser->reference_size += strlen(reference);
				match_set_reference(bfparser->st, matchnum, reference);
				xmlFree(reference);
			}
			match_autocomplete_reference(bfparser->st, matchnum, context);
		}
	}
	/* TODO cleanup! */
	free_autocomplete(autocomplete);

	if (pattern)
		xmlFree(pattern);
	if (id)
		xmlFree(id);
	if (idref)
		xmlFree(idref);
	/*if (highlight) xmlFree(highlight); */
	if (blockstartelement)
		xmlFree(blockstartelement);
	/*if (blockhighlight) xmlFree(blockhighlight); */
	if (class)
		xmlFree(class);
	return matchnum;
}

/* qsort string comparison function */
int
stringcmp(const void *sp1, const void *sp2)
{
	return (strcmp(*(char **) sp1, *(char **) sp2));
}

static guint16
add_string(Tbflangparsing * bfparser, guint16 contexttag, const gchar *stringname, const gchar *singlematch, const gchar *fullmatch)
{
	static const gchar *stringhighlight = "attribute-string";
	guint16 matchstring;
	const gchar *val = lookup_user_option(bfparser->bflang->name, "attribute_string_is_block");
	if (val && val[0]=='1') {
		guint16 strcontext, endmatch;
		strcontext = new_context(bfparser->st, 4, bfparser->bflang->name, (gchar *)singlematch, stringhighlight, FALSE, FALSE);
		matchstring = add_pattern_to_scanning_table(bfparser->st, singlematch, FALSE, FALSE, contexttag);
		pattern_set_runtime_properties(bfparser->st, matchstring, stringhighlight,strcontext,FALSE,FALSE,0,FALSE,FALSE);
		pattern_set_blockmatch(bfparser->st, matchstring,TRUE,FALSE,0,NULL,NULL,FALSE);
		endmatch = add_pattern_to_scanning_table(bfparser->st, singlematch, FALSE, FALSE, strcontext);
		pattern_set_runtime_properties(bfparser->st, endmatch, stringhighlight,-1,FALSE,FALSE,0,FALSE,FALSE);
		pattern_set_blockmatch(bfparser->st, endmatch,FALSE,TRUE,matchstring,NULL,NULL,FALSE);
	} else {
		matchstring = add_pattern_to_scanning_table(bfparser->st, fullmatch, TRUE, FALSE, contexttag);
		pattern_set_runtime_properties(bfparser->st, matchstring, stringhighlight,
					0, FALSE, FALSE,0, FALSE, FALSE);
	}
	g_hash_table_insert(bfparser->patterns, g_strdup(stringname),
						GINT_TO_POINTER((gint) matchstring));
	return matchstring;
}

static guint16
process_scanning_tag(xmlTextReaderPtr reader, Tbflangparsing * bfparser, guint16 context,
					 GQueue * contextstack, gchar * ih_highlight,
					 gchar * ih_attrib_autocomplete_append, gchar * ih_attribhighlight,
					 gint ih_attrib_autocomplete_backup_cursor, GSList *ih_autocomplete)
{
	gchar *tag = NULL, *idref = NULL, *highlight = NULL, *attributes = NULL, *attribhighlight = NULL, *class =
		NULL, *notclass = NULL, *attrib_autocomplete_append = NULL, *id = NULL, *block_name = NULL;
	gint attrib_autocomplete_backup_cursor = 0;
	guint16 matchnum = 0, innercontext = context;
	gboolean is_empty, case_insens = FALSE, sgml_shorttag = FALSE, no_close = FALSE;
	gboolean add_tag;
	GSList *autocomplete=NULL;
	DBG_PARSING("processing tag...\n");
	is_empty = xmlTextReaderIsEmptyElement(reader);

	Tattrib attribs[] = {{"name", &tag, attribtype_string},
					{"block_name", &block_name, attribtype_string},
					{"id", &id, attribtype_string},
					{"idref", &idref, attribtype_string},
					{"highlight", &highlight, attribtype_string},
					{"class", &class, attribtype_string},
					{"notclass", &notclass, attribtype_string},
					{"case_insens", &case_insens, attribtype_boolean},
					{"attributes", &attributes, attribtype_string},
					{"attribhighlight", &attribhighlight, attribtype_string},
					{"sgml_shorttag", &sgml_shorttag, attribtype_boolean},
					{"no_close", &no_close, attribtype_boolean},
					{"attrib_autocomplete_append", &attrib_autocomplete_append, attribtype_string},
					{"attrib_autocomplete_backup_cursor", &attrib_autocomplete_backup_cursor, attribtype_int}};
	parse_attributes(bfparser->bflang,reader, attribs, bfparser->load_completion ? 14 : 12);
	add_tag = do_parse(bfparser, class, notclass);
	if (add_tag) {
		if (idref && idref[0] && !tag) {
			guint16 matchnum = GPOINTER_TO_INT(g_hash_table_lookup(bfparser->patterns, idref));
			if (!matchnum) {
				g_print("Error in language file, tag with id %s does not exist\n", idref);
			}
			DBG_PARSING("lookup tag with id %s returned matchnum %d\n", id, matchnum);
			if (matchnum)
				compile_existing_match(bfparser->st, matchnum, context);
		} else if (tag && tag[0]) {
			gchar *attrib_context_id = NULL;
			gchar **attrib_arr = NULL;
			guint16 starttagmatch = 0, endtagmatch, matchstring;
			gint contexttag = 0 /*, contextstring */ ;
			gchar *tmp, *reference = NULL;
			gboolean foldable=TRUE;

			/* try to re-use the context of other tags. this is only possible if the other tags have exactly the same attribute set
			   we check this by sorting the attributes, concatenating them comma separated together, and using this as a hash key
			   if there are no attributes we use context __internal_no_attribs__ */
			if (attributes) {
				attrib_arr = g_strsplit(attributes, ",", -1);
				qsort(attrib_arr, g_strv_length(attrib_arr), sizeof(gchar *), stringcmp);
				attrib_context_id = g_strjoinv(",", attrib_arr);
				if (sgml_shorttag) {
					tmp = attrib_context_id;
					attrib_context_id = g_strconcat("1,", tmp, NULL);
					g_free(tmp);
				}
			} else {
				attrib_context_id = g_strdup("__internal_no_attribs__");
			}
			if (!bfparser->autoclose_tags && is_empty) {
				/* we can only re-use an existing context for attributes
				   if the attributes are the same, both tags have the same
				   nextcontext (or -1) and the same sgml_shorttag setting.

				   'is_empty' is not a good measure for the next context,
				   because a tag can also have a reference, but it is a start  */
				contexttag = GPOINTER_TO_INT(g_hash_table_lookup(bfparser->contexts, attrib_context_id));
				if (contexttag) {
				   g_print("HIT for %s, saves %ld bytes\n",attrib_context_id, (long int)((GArray *)g_array_index(bfparser->st->contexts, Tcontext, contexttag).table)->len*sizeof(Ttablerow));
				}
			}
			tmp = g_strconcat("<", tag, NULL);
			matchnum = add_pattern_to_scanning_table(bfparser->st, tmp, FALSE, case_insens, context);
			if (block_name) {
				foldable = lookup_block_foldable(bfparser->bflang->name, block_name);
			}
			pattern_set_blockmatch(bfparser->st, matchnum,TRUE,FALSE,0,NULL,block_name,foldable);
			pattern_set_runtime_properties(bfparser->st, matchnum,
								highlight ? highlight : ih_highlight,
								contexttag, TRUE, FALSE,0, FALSE, FALSE);
			DBG_PARSING("insert tag %s into hash table with matchnum %d\n", id ? id : tmp, matchnum);
			if (g_hash_table_lookup(bfparser->patterns, id ? id : tmp) != NULL) {
				g_print("Possible error in language file, id %s already exists\n", id ? id : tmp);
			} else {
				g_hash_table_insert(bfparser->patterns, g_strdup(id ? id : tmp),
									GINT_TO_POINTER((gint) matchnum));
			}
			g_free(tmp);

			if (!contexttag) {
				static const gchar *internal_tag_string_d = "__internal_tag_string_d__";
				static const gchar *internal_tag_string_s = "__internal_tag_string_s__";
				contexttag = new_context(bfparser->st, 8, bfparser->bflang->name, ">\"=' \t\n\r<", NULL, FALSE, FALSE);
				match_set_nextcontext(bfparser->st, matchnum, contexttag);
				if (attrib_arr) {
					gchar **tmp2;

					tmp2 = attrib_arr;
					while (*tmp2) {
						guint16 attrmatch = add_pattern_to_scanning_table(bfparser->st, *tmp2, FALSE, TRUE, contexttag);
						pattern_set_runtime_properties(bfparser->st, attrmatch,
								attribhighlight ? attribhighlight : ih_attribhighlight,
								0, FALSE, FALSE,0, FALSE, FALSE);
						match_add_autocomp_item(bfparser->st, attrmatch, NULL,
												attrib_autocomplete_append ? attrib_autocomplete_append :
												ih_attrib_autocomplete_append,
												attrib_autocomplete_backup_cursor ?
												attrib_autocomplete_backup_cursor :
												ih_attrib_autocomplete_backup_cursor);
						match_autocomplete_reference(bfparser->st, attrmatch, contexttag);
						tmp2++;
					}
				}
				matchstring = GPOINTER_TO_INT(g_hash_table_lookup(bfparser->patterns, internal_tag_string_d));
				if (matchstring) {
					/*g_print("adding existing tag string %s to context %d\n",internal_tag_string_d,contexttag); */
					compile_existing_match(bfparser->st, matchstring, contexttag);
				} else {
					add_string(bfparser, contexttag, internal_tag_string_d, "\"", "\"[^\"]*\"");
/*					gchar *val = lookup_user_option(bfparser->bflang->name, "attribute_string_is_block");
					if (val && val[0]=='1') {
						guint16 strcontext, endmatch;
						strcontext = new_context(bfparser->st, 4, bfparser->bflang->name, "\"", stringhighlight, FALSE);
						matchstring = add_pattern_to_scanning_table(bfparser->st, "\"", FALSE, FALSE, contexttag);
						pattern_set_runtime_properties(bfparser->st, matchstring, stringhighlight,strcontext,FALSE,FALSE,0,FALSE,FALSE);
						pattern_set_blockmatch(bfparser->st, matchstring,TRUE,FALSE,0,NULL,NULL,FALSE);
						endmatch = add_pattern_to_scanning_table(bfparser->st, "\"", FALSE, FALSE, strcontext);
						pattern_set_runtime_properties(bfparser->st, endmatch, stringhighlight,-1,FALSE,FALSE,0,FALSE,FALSE);
						pattern_set_blockmatch(bfparser->st, endmatch,FALSE,TRUE,matchstring,NULL,NULL,FALSE);
					} else {
						matchstring = add_pattern_to_scanning_table(bfparser->st, "\"[^\"]*\"", TRUE, FALSE, contexttag);
						pattern_set_runtime_properties(bfparser->st, matchstring, stringhighlight,
									0, FALSE, FALSE,0, FALSE, FALSE);
					}
					g_hash_table_insert(bfparser->patterns, g_strdup(internal_tag_string_d),
										GINT_TO_POINTER((gint) matchstring));*/
				}
				matchstring = GPOINTER_TO_INT(g_hash_table_lookup(bfparser->patterns, internal_tag_string_s));
				if (matchstring) {
					/*g_print("adding existing tag string %s to context %d\n",internal_tag_string_s,contexttag); */
					compile_existing_match(bfparser->st, matchstring, contexttag);
				} else {
					add_string(bfparser, contexttag, internal_tag_string_s, "'", "'[^']*'");
/*					static const gchar *stringhighlight = "attribute-string";
					matchstring = add_pattern_to_scanning_table(bfparser->st, "'[^']*'", TRUE, FALSE, contexttag);
					pattern_set_runtime_properties(bfparser->st, matchstring, stringhighlight,
								0, FALSE, FALSE,0, FALSE, FALSE);
					g_hash_table_insert(bfparser->patterns, g_strdup(internal_tag_string_s),
										GINT_TO_POINTER((gint) matchstring));*/
				}

				if (!sgml_shorttag) {
					guint16 tmpnum = add_pattern_to_scanning_table(bfparser->st, "/>", FALSE, FALSE, contexttag);
					pattern_set_runtime_properties(bfparser->st, tmpnum,
								highlight ? highlight : ih_highlight,
								-1, FALSE, FALSE,0, FALSE, FALSE);
					pattern_set_blockmatch(bfparser->st, tmpnum,
								FALSE,TRUE, -1,NULL,NULL,TRUE);
					if (bfparser->autoclose_tags)
						match_add_autocomp_item(bfparser->st, tmpnum, NULL, NULL, 0);
					match_autocomplete_reference(bfparser->st, tmpnum, contexttag);
				}
				tmp = no_close ? NULL : g_strconcat("</", tag, ">", NULL);
				starttagmatch = add_pattern_to_scanning_table(bfparser->st, ">", FALSE, FALSE, contexttag);
				pattern_set_runtime_properties(bfparser->st, starttagmatch,
								highlight ? highlight : ih_highlight,
								-1, FALSE, bfparser->stretch_tag_block,0, FALSE, FALSE);
				pattern_set_blockmatch(bfparser->st, starttagmatch, FALSE, FALSE, matchnum /* blockstartpattern for stretch_block */, NULL, NULL, TRUE);
				if (bfparser->autoclose_tags && !no_close) {
					gchar *tmp2;
					tmp2 = g_strconcat("></", tag, ">", NULL);
					match_add_autocomp_item(bfparser->st, starttagmatch, NULL, tmp, tmp ? strlen(tmp) : 0);
					match_add_autocomp_item(bfparser->st, matchnum, NULL, tmp2, strlen(tag)+3);
				}
				if (tmp)
					g_free(tmp);
				match_autocomplete_reference(bfparser->st, starttagmatch, contexttag);
				g_hash_table_insert(bfparser->contexts, g_strdup(attrib_context_id),
									GINT_TO_POINTER(contexttag));
			}

			if (attrib_arr)
				g_strfreev(attrib_arr);
			if (attrib_context_id)
				g_free(attrib_context_id);

			if (!is_empty) {
				while (xmlTextReaderRead(reader) == 1) {
					int nodetype = xmlTextReaderNodeType(reader);
					xmlChar *name;

					if (nodetype == 14) {
						continue;
					}

					name = xmlTextReaderName(reader);
					if (xmlStrEqual(name, (xmlChar *) "reference")) {
						if (!xmlTextReaderIsEmptyElement(reader)) {
							if (langmgr.load_reference && bfparser->load_reference)
								reference = (gchar *) xmlTextReaderReadInnerXml(reader);
							DBG_PARSING("reference=%s\n", reference);
							skip_to_end_tag(reader, xmlTextReaderDepth(reader));
						}
					} else if (xmlStrEqual(name, (xmlChar *) "context")) {
						if (no_close) {
							g_print
								("Error in language file, tag %s has no_close=%d but specifies an inner context\n",
								 tag, no_close);
							no_close = FALSE;
						}
						innercontext = process_scanning_context(reader, bfparser, contextstack);
						match_set_nextcontext(bfparser->st, starttagmatch, innercontext);
					} else if (xmlStrEqual(name, (xmlChar *) "tag")) {
						xmlFree(name);
						break;
					} else if (nodetype == 1 && xmlStrEqual(name, (xmlChar *) "autocomplete")) {
						process_autocomplete(reader, bfparser, &autocomplete);
					} else {
						DBG_PARSING("process_scanning_tag, parsing element with name %s\n", name);
					}
					xmlFree(name);
				}
			}
			if (!no_close && autocomplete == NULL && ih_autocomplete == NULL) {
				gchar *tmp2 = g_strconcat("></", tag, ">", NULL);
				match_add_autocomp_item(bfparser->st, matchnum, NULL, tmp2, strlen(tag) + 3);
				g_free(tmp2);
			} else {
				add_autocomplete(bfparser, matchnum, autocomplete?autocomplete:ih_autocomplete, tag);
				/*match_add_autocomp_item(bfparser->st, matchnum, NULL,
										autocomplete_append ? autocomplete_append : ih_autocomplete_append,
										autocomplete_backup_cursor ? autocomplete_backup_cursor :
										ih_autocomplete_backup_cursor);*/
			}

			if (reference != NULL) {
				bfparser->reference_size += strlen(reference);
				match_set_reference(bfparser->st, matchnum, reference);
				xmlFree(reference);
			}
			match_autocomplete_reference(bfparser->st, matchnum, context);
			if (!no_close) {
				tmp = g_strconcat("</", tag, ">", NULL);
				endtagmatch = add_pattern_to_scanning_table(bfparser->st, tmp, FALSE, case_insens , innercontext);
				pattern_set_runtime_properties(bfparser->st, endtagmatch,
									highlight ? highlight : ih_highlight,
									(innercontext == context) ? 0 : -2,
									FALSE, FALSE,0, FALSE, FALSE);
				pattern_set_blockmatch(bfparser->st, endtagmatch,
									FALSE,TRUE,matchnum,NULL,NULL, TRUE);
/*				g_print("context %d: matchnum %d is ended by endtagmatch %d while working on id %s\n",innercontext,matchnum, endtagmatch, id);*/
				match_add_autocomp_item(bfparser->st, endtagmatch, NULL, NULL, 0);
				match_autocomplete_reference(bfparser->st, endtagmatch, innercontext);
				if (g_hash_table_lookup(bfparser->patterns, tmp)) {
					g_print("Possible error in language file: id %s already exists\n", tmp);
				} else {
					g_hash_table_insert(bfparser->patterns, g_strdup(tmp),
										GINT_TO_POINTER((gint) endtagmatch));
				}
				g_free(tmp);
			}
		}
	}
	free_autocomplete(autocomplete);
	g_free(id);
	g_free(idref);
	g_free(tag);
	/*g_free(highlight); */
	g_free(class);
	g_free(notclass);
	g_free(attrib_autocomplete_append);
	/*g_free(attribhighlight); */
	g_free(attributes);

	return matchnum;
}

/* ih stands for InHerit */
static void
process_scanning_group(xmlTextReaderPtr reader, Tbflangparsing * bfparser, gint context,
					   GQueue * contextstack, gchar * ih_highlight,
					   gchar * ih_class, gchar * ih_attrib_autocomplete_append, gchar * ih_attribhighlight,
					   gint ih_case_insens, gint ih_is_regex, gint ih_attrib_autocomplete_backup_cursor,
					   GSList *ih_autocomplete)
{
	gchar *highlight = NULL, *class = NULL, *notclass =
		NULL, *attrib_autocomplete_append = NULL, *attribhighlight = NULL;
	gint case_insens = UNDEFINED, is_regex = UNDEFINED;
	gint attrib_autocomplete_backup_cursor = 0;
	GSList *autocomplete=NULL;
	gint depth;
	gboolean add_group;
	if (xmlTextReaderIsEmptyElement(reader)) {
		return;
	}
	depth = xmlTextReaderDepth(reader);
	Tattrib attribs[] = {{"highlight", &highlight, attribtype_string},
					{"class", &class, attribtype_string},
					{"notclass", &notclass, attribtype_string},
					{"is_regex", &is_regex, attribtype_boolean},
					{"case_insens", &case_insens, attribtype_boolean},
					{"attribhighlight", &attribhighlight, attribtype_string},
					{"attrib_autocomplete_append", &attrib_autocomplete_append, attribtype_string},
					{"attrib_autocomplete_backup_cursor", &attrib_autocomplete_backup_cursor, attribtype_int}};
	parse_attributes(bfparser->bflang,reader, attribs, bfparser->load_completion ? 8 : 6);
	add_group = do_parse(bfparser, class, notclass);
	DBG_PARSING("add_group=%d for class=%s and notclass=%s\n",add_group, class, notclass);
	if (!add_group) {
		DBG_PARSING("group disabled, class=%s, notclass=%s, skip to end of group, my depth=%d\n", class,
					notclass, depth);
		skip_to_end_tag(reader, depth);
	} else {
		while (xmlTextReaderRead(reader) == 1) {
			int nodetype = xmlTextReaderNodeType(reader);
			xmlChar *name;

			if (nodetype == 14 /* #text */) {
				continue;
			}

			name = xmlTextReaderName(reader);
			if (xmlStrEqual(name, (xmlChar *) "element")) {
				process_scanning_element(reader, bfparser, context, contextstack,
									highlight ? highlight : ih_highlight,
									case_insens != UNDEFINED ? case_insens : ih_case_insens,
									is_regex != UNDEFINED ? is_regex : ih_is_regex,
									autocomplete ? autocomplete : ih_autocomplete);
			} else if (xmlStrEqual(name, (xmlChar *) "tag")) {
				process_scanning_tag(reader, bfparser, context, contextstack,
									highlight ? highlight : ih_highlight,
									attrib_autocomplete_append ? attrib_autocomplete_append :
									ih_attrib_autocomplete_append,
									attribhighlight ? attribhighlight : ih_attribhighlight,
									attrib_autocomplete_backup_cursor ? attrib_autocomplete_backup_cursor :
									ih_attrib_autocomplete_backup_cursor,
									autocomplete ? autocomplete : ih_autocomplete);
			} else if (xmlStrEqual(name, (xmlChar *) "group")) {
				if (xmlTextReaderDepth(reader) == depth) {
					DBG_PARSING("end-of-group\n");
					xmlFree(name);
					break;
				} else {
					process_scanning_group(reader, bfparser, context, contextstack,
									highlight ? highlight : ih_highlight, class ? class : ih_class,
									attrib_autocomplete_append ? attrib_autocomplete_append :
									ih_attrib_autocomplete_append,
									attribhighlight ? attribhighlight : ih_attribhighlight,
									case_insens != UNDEFINED ? case_insens : ih_case_insens,
									is_regex != UNDEFINED ? is_regex : ih_is_regex,
									attrib_autocomplete_backup_cursor ?
									attrib_autocomplete_backup_cursor :
									ih_attrib_autocomplete_backup_cursor,
									autocomplete ? autocomplete : ih_autocomplete);
				}
			} else if (nodetype == 1 && xmlStrEqual(name, (xmlChar *) "autocomplete")) {
				process_autocomplete(reader, bfparser, &autocomplete);
			} else {
				DBG_PARSING("found %s\n", name);
			}
			xmlFree(name);
		}
	}
	free_autocomplete(autocomplete);
	/*g_free(highlight); stored in the structure */
	g_free(class);
	g_free(notclass);
	g_free(attrib_autocomplete_append);
	/*g_free(attribhighlight); */
}

static guint8
get_first_comment_of_type(Tbflangparsing * bfparser, Tcomment_type type)
{
	gint i = 0;
	while (g_array_index(bfparser->st->comments, Tcomment, i).so) {
		if (g_array_index(bfparser->st->comments, Tcomment, i).type == type)
			return i;
		i++;
	}
	return COMMENT_INDEX_NONE;
}

static void
set_commentid(Tbflangparsing * bfparser, gboolean topevel_context, guint8 * toset, const gchar * string,
			  Tcomment_type type)
{
	*toset = COMMENT_INDEX_INHERIT;
	if (string) {
		DEBUG_MSG("set_commentid, request for commentid %s\n",string);
		if (g_strcmp0(string, "none") == 0) {
			*toset = COMMENT_INDEX_NONE;
		} else {
			gint tmp = GPOINTER_TO_INT(g_hash_table_lookup(bfparser->commentid_table, string));
			/* decrement 1 because to store index 0 in the hashtable, all numbers in the hastable are incremented with 1 */
			DEBUG_MSG("set_commentid, id %s has index %d\n",string,(gint)(tmp-1));
			if (tmp != 0)
				*toset = (guint8) (tmp-1);
		}
	}
	if (topevel_context && *toset == COMMENT_INDEX_INHERIT) {
		/* set the first block comment as default for the toplevel context */
		*toset = get_first_comment_of_type(bfparser, type);
	}
}


static gint16
process_scanning_context(xmlTextReaderPtr reader, Tbflangparsing * bfparser, GQueue * contextstack)
{
	gchar *symbols = NULL, *highlight = NULL, *id = NULL, *idref = NULL, *commentid_block =
		NULL, *commentid_line = NULL;
	gboolean autocomplete_case_insens = FALSE;
	gint default_spellcheck = SPELLCHECK_INHERIT;
	gint context, depth;

	Tattrib attribs[] = {{"id", &id, attribtype_string},
					{"idref", &idref, attribtype_string},
					{"symbols", &symbols, attribtype_string},
					{"highlight", &highlight, attribtype_string},
					{"commentid_block", &commentid_block, attribtype_string},
					{"commentid_line", &commentid_line, attribtype_string},
					{"default_spellcheck", &default_spellcheck, attribtype_boolean},
					{"autocomplete_case_insens", &autocomplete_case_insens, attribtype_boolean}};
	if (g_queue_get_length(contextstack)==0) {
		DBG_PARSING("top level context, set default_spellcheck to %d\n", bfparser->default_spellcheck);
		default_spellcheck = bfparser->default_spellcheck;
	}
	depth = xmlTextReaderDepth(reader);

	parse_attributes(bfparser->bflang,reader, attribs, bfparser->load_completion ? 7 : 6);
	DBG_PARSING("found <context> with id=%s, idref=%s\n", id, idref);
	if (idref && idref[0] && !id && !symbols && !highlight && !autocomplete_case_insens) {
		DBG_PARSING("lookup context %s in hash table..\n", idref);
		context = GPOINTER_TO_INT(g_hash_table_lookup(bfparser->contexts, idref));
		g_free(idref);
		return context;
	}
	if (!symbols) {
		g_warning("language file has context without symbols, abort.");
		return 0;
	}
	/* create context */
	DBG_PARSING("create context %s, symbols %s and highlight %s\n", id, symbols, highlight);
	context = new_context(bfparser->st, 32, bfparser->bflang->name, symbols, highlight, autocomplete_case_insens, default_spellcheck);
	g_queue_push_head(contextstack, GINT_TO_POINTER(context));
	if (id) {
		DBG_PARSING("insert context %s into hash table as %d\n", id, context);
		if (g_hash_table_lookup(bfparser->contexts, id) != NULL) {
			g_print("Error in language file, context id %s already exists\n", id);
		} else {
			g_hash_table_insert(bfparser->contexts, g_strdup(id), GINT_TO_POINTER(context));
		}
	}
	set_commentid(bfparser, (g_queue_get_length(contextstack) == 1)
				  , &g_array_index(bfparser->st->contexts, Tcontext, context).comment_block, commentid_block,
				  comment_type_block);
	set_commentid(bfparser, (g_queue_get_length(contextstack) == 1)
				  , &g_array_index(bfparser->st->contexts, Tcontext, context).comment_line, commentid_line,
				  comment_type_line);

	g_free(id);
	g_free(symbols);
	/*g_free(highlight); stored in the structure */
	/* now get the children */
	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);
		DBG_PARSING("parsing context, found node id %s\n", name);
		if (xmlStrEqual(name, (xmlChar *) "element")) {
			process_scanning_element(reader, bfparser, context, contextstack, NULL, UNDEFINED,
									 UNDEFINED, NULL);
		} else if (xmlStrEqual(name, (xmlChar *) "tag")) {
			process_scanning_tag(reader, bfparser, context, contextstack, NULL, NULL, NULL, 0, NULL);
		} else if (xmlStrEqual(name, (xmlChar *) "group")) {
			process_scanning_group(reader, bfparser, context, contextstack, NULL, NULL, NULL, NULL,
								   UNDEFINED, UNDEFINED, 0, NULL);
		} else if (xmlStrEqual(name, (xmlChar *) "context")) {
			if (depth == xmlTextReaderDepth(reader)) {
				xmlFree(name);
				DBG_PARSING("parsing context, end-of-context, return context %d\n", context);
				g_queue_pop_head(contextstack);
				return context;
			} else {
				/* a context inside a context is usually a context that is referred to by various other elements or tags */
				process_scanning_context(reader, bfparser, contextstack);
			}
		} else {
			DBG_PARSING("parsing context, found %s\n", name);
		}
		xmlFree(name);
	}
	/* can we ever get here ?? */
	DBG_PARSING("pop context %d\n", GPOINTER_TO_INT(g_queue_peek_head(contextstack)));
	g_queue_pop_head(contextstack);
	return context;
}

static gpointer
build_lang_thread(gpointer data)
{
	xmlTextReaderPtr reader;
	Tbflang *bflang = data;
	Tbflangparsing *bfparser;
	const gchar *tmp;
	GList *tmplist;

	bfparser = g_slice_new0(Tbflangparsing);
	bfparser->patterns = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	bfparser->contexts = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	/*bfparser->setoptions =  g_hash_table_new_full(g_str_hash,g_str_equal,g_free,NULL); */
	bfparser->bflang = bflang;
	/*for(tmplist = g_list_first(bfparser->bflang->setoptions);tmplist;tmplist=g_list_next(tmplist)) {
	   g_hash_table_insert(bfparser->setoptions,g_strdup(tmplist->data),GINT_TO_POINTER(1));
	   } */
	bfparser->st = scantable_new(bflang->size_table, bflang->size_matches, bflang->size_contexts);
	bfparser->commentid_table = g_hash_table_new_full(g_str_hash, g_str_equal, xmlFree, NULL);

	tmp = lookup_user_option(bflang->name, "load_reference");
	bfparser->load_reference = !(tmp && tmp[0] == '0');
	tmp = lookup_user_option(bflang->name, "load_completion");
	bfparser->load_completion = !(tmp && tmp[0] == '0');
	tmp = lookup_user_option(bflang->name, "autoclose_tags");
	bfparser->autoclose_tags = !(tmp && tmp[0] == '0');
	tmp = lookup_user_option(bflang->name, "stretch_tag_block");
	bfparser->stretch_tag_block = !(tmp && tmp[0] == '0');

	/* insert the special option is_LANGNAME to the hashtable, so you can check for the language file being parsed using class or notclass */
	for (tmplist=g_list_first(langmgr.bflang_list);tmplist;tmplist=g_list_next(tmplist)) {
		Tbflang* tmpbflang = tmplist->data;
		gchar *tmp2 = g_strconcat("is_", tmpbflang->name, NULL);
		langmgr_insert_user_option(bflang->name, tmp2, (tmpbflang == bflang) ? "1" : "0");
		g_free(tmp2);
	}

	DBG_PARSING("build_lang_thread %p, started for %s\n", g_thread_self(), bfparser->bflang->filename);
	reader = xmlNewTextReaderFilename(bfparser->bflang->filename);
	if (!reader) {
		g_print("failed to open %s\n", bfparser->bflang->filename);
		/* TODO CLEANUP */
		return NULL;
	}
	xmlTextReaderSetParserProp(reader, XML_PARSER_SUBST_ENTITIES, TRUE);

	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);
		DBG_PARSING("build_lang_thread, found %s\n", name);
		if (xmlStrEqual(name, (xmlChar *) "header")) {
			/* actually we can skip detection */
			DBG_PARSING("skip <header>\n");
			skip_to_end_tag(reader, xmlTextReaderDepth(reader));
			/*process_header(reader,bflang); */
		} else if (xmlStrEqual(name, (xmlChar *) "definition")) {
			if (xmlTextReaderIsEmptyElement(reader)) {
				DBG_PARSING("empty <definition />\n");
				/* empty <definition />, probably text/plain */
				g_array_free(bfparser->st->matches, TRUE);
				g_array_free(bfparser->st->contexts, TRUE);
				g_array_free(bfparser->st->comments, TRUE);
				g_array_free(bfparser->st->blocks, TRUE);
				g_slice_free(Tscantable, bfparser->st);
				bfparser->st = NULL;
				xmlFree(name);
				break;
			}
			DBG_PARSING("processing <definition>\n");
			while (xmlTextReaderRead(reader) == 1) {
				xmlChar *name2 = xmlTextReaderName(reader);
				if (xmlStrEqual(name2, (xmlChar *) "context")) {
					GQueue *contextstack = g_queue_new();
					process_scanning_context(reader, bfparser, contextstack);
					g_queue_free(contextstack);
				} else if (xmlStrEqual(name2, (xmlChar *) "definition")) {
					xmlFree(name2);
					break;
				} else {
					DBG_PARSING("build_lang_thread, within <definition>, found %s\n", name2);
				}
				xmlFree(name2);
			}
		} else if (xmlStrEqual(name, (xmlChar *) "properties")) {
			DBG_PARSING("processing <properties>\n");
			while (xmlTextReaderRead(reader) == 1) {
				xmlChar *name2 = xmlTextReaderName(reader);
				if (xmlStrEqual(name2, (xmlChar *) "comment")) {
					gchar *typestr = NULL, *id = NULL;
					Tcomment com = { NULL, NULL, 0 };
					Tattrib attribs[] = {{"start", &com.so, attribtype_string},
							{"end", &com.eo, attribtype_string},
							{"type", &typestr, attribtype_string},
							{"id", &id, attribtype_string}};
					parse_attributes(bflang,reader, attribs, 4);
					com.type = g_strcmp0(typestr, "block") == 0 ? comment_type_block : comment_type_line;
					if (com.so && (com.type == comment_type_line || com.eo)) {
						g_array_append_val(bfparser->st->comments, com);
						if (id)
							/* we insert one position higher than the actual index in the hashtable, because index 0 would
							otherwise produce pointer NULL in the hashtable. set_comment() should thus decrement by 1) */
							g_hash_table_insert(bfparser->commentid_table, id,
												GINT_TO_POINTER(bfparser->st->comments->len));
					} else {
						xmlFree(com.so);
						xmlFree(com.eo);
						xmlFree(id);
					}
					xmlFree(typestr);
				} else if (xmlStrEqual(name2, (xmlChar *) "smartindent")) {
					Tattrib attribs[] = {{"characters", &bfparser->smartindentchars, attribtype_string}};
					parse_attributes(bflang,reader, attribs, 1);
				} else if (xmlStrEqual(name2, (xmlChar *) "smartoutdent")) {
					Tattrib attribs[] = {{"characters", &bfparser->smartoutdentchars, attribtype_string}};
					parse_attributes(bflang,reader, attribs, 1);
#ifdef HAVE_LIBENCHANT
				} else if (xmlStrEqual(name2, (xmlChar *) "default_spellcheck")) {
					Tattrib attribs[] = {{"enabled", &bfparser->default_spellcheck, attribtype_boolean},
							{"spell_decode_entities", &bfparser->spell_decode_entities, attribtype_boolean}};
					parse_attributes(bflang,reader, attribs, 2);
#endif
				} else if (xmlStrEqual(name2, (xmlChar *) "properties")) {
					xmlFree(name2);
					break;
				}
				xmlFree(name2);
			}
		}
		xmlFree(name);
	}
	xmlFreeTextReader(reader);
	/* do some final memory management */
	if (bfparser->st) {
		gint i, tablenum=0, largest_table=0;
		bfparser->st->contexts->data =
			g_realloc(bfparser->st->contexts->data, (bfparser->st->contexts->len + 1) * sizeof(Tcontext));
		bfparser->st->matches->data =
			g_realloc(bfparser->st->matches->data, (bfparser->st->matches->len + 1) * sizeof(Tpattern));
		bfparser->st->comments->data =
			g_realloc(bfparser->st->comments->data, (bfparser->st->comments->len + 1) * sizeof(Tcomment));
		bfparser->st->blocks->data =
			g_realloc(bfparser->st->blocks->data, (bfparser->st->blocks->len + 1) * sizeof(Tpattern_block));
		/* now optimise the DFA tables for each context */
		for (i = 1; i < bfparser->st->contexts->len; i++) {
			g_array_index(bfparser->st->contexts, Tcontext, i).table->data = g_realloc(g_array_index(bfparser->st->contexts, Tcontext, i).table->data, (g_array_index(bfparser->st->contexts, Tcontext, i).table->len + 1) * sizeof(Ttablerow));
			tablenum += get_table(bfparser->st, i)->len + 1;
			if (get_table(bfparser->st, i)->len > largest_table)
				largest_table = get_table(bfparser->st, i)->len;
		}
		g_print("Language statistics for %s from %s\n", bfparser->bflang->name, bfparser->bflang->filename);
		g_print("reference size       %9.2f Kbytes\n", bfparser->reference_size/1024.0);
		g_print("largest table %5d (%9.2f Kbytes)\n", largest_table,
				1.0 * largest_table * sizeof(Ttablerow) / 1024.0);
		g_print("total tables  %5d (%9.2f Kbytes)\n", tablenum,
				1.0 * tablenum * sizeof(Ttablerow) / 1024.0);
		g_print("contexts      %5d (%9.2f Kbytes)\n", bfparser->st->contexts->len,
				1.0 * bfparser->st->contexts->len * sizeof(Tcontext) / 1024.0);
		g_print("matches       %5d (%9.2f Kbytes)\n", bfparser->st->matches->len,
				1.0 * bfparser->st->matches->len * sizeof(Tpattern) / 1024.0);
		g_print("blocks        %5d (%9.2f Kbytes)\n", bfparser->st->blocks->len,
				1.0 * bfparser->st->blocks->len * sizeof(Tpattern_block) / 1024.0);
		/*print_DFA(bfparser->st, '&','Z'); */
		/*print_DFA_subset(bfparser->st, "<PpIi>"); */

		bflang->tags = bftextview2_scantable_rematch_highlights(bfparser->st, bflang->name);
	}
	DBG_PARSING("build_lang_thread finished bflang=%p\n", bflang);
	/* when done call mainloop, see the top of bftextview2.c why we use priority 122 here */
	g_idle_add_full(BUILD_LANG_FINISHED_PRIORITY, build_lang_finished_lcb, bfparser, NULL);
	return bflang;
}

Tbflang *
langmgr_get_bflang(const gchar * mimetype, const gchar * filename)
{
	Tbflang *bflang = NULL;
	if (!mimetype)
		return NULL;
	DBG_MSG("langmgr_get_bflang, request for mimetype %s filename %s\n",mimetype, filename);
	if (filename && strchr(mimetype, '?') == NULL) {
		gchar *key = mime_with_extension(mimetype, filename);
/*		g_print("search for key=%s\n",key);*/
		bflang = g_hash_table_lookup(langmgr.bflang_lookup, key);
		g_free(key);
	}
	if (!bflang) {
		bflang = g_hash_table_lookup(langmgr.bflang_lookup, mimetype);
	}

	if (bflang && bflang->filename && !bflang->st && !bflang->no_st && !bflang->parsing) {
		GError *error = NULL;
		bflang->parsing = TRUE;
		DBG_MSG("langmgr_get_bflang, no scantable in %p, start thread\n", bflang);
		/*thread = g_thread_create(build_lang_thread,bflang,FALSE,&error); */
		g_thread_create_full(build_lang_thread, bflang, 0, FALSE, FALSE, G_THREAD_PRIORITY_LOW, &error);

		if (error) {
			DBG_PARSING("start thread, error\n");
		}
	} else {
		DBG_MSG("langmgr_get_bflang, have scantable or no scantable available for bflang %p\n", bflang);
	}
	return bflang;
}

GtkTextTagTable *
langmgr_get_tagtable(void)
{
	return langmgr.tagtable;
}

static Tbflang *
parse_bflang2_header(const gchar * filename)
{
	xmlTextReaderPtr reader;
	Tbflang *bflang;
	gboolean failed=FALSE;
	gchar *bflangversion = NULL;

	reader = xmlNewTextReaderFilename(filename);
	xmlTextReaderSetParserProp(reader, XML_PARSER_SUBST_ENTITIES, TRUE);
	if (!reader) {
		return NULL;
	}
	bflang = g_slice_new0(Tbflang);
	bflang->size_table = 2;	/* bare minimum sizes */
	bflang->size_matches = 2;
	bflang->size_contexts = 2;
	bflang->filename = g_strdup(filename);
	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);
		if (xmlStrEqual(name, (xmlChar *) "bflang")) {
			Tattrib attribs[] = {{"name", &bflang->name, attribtype_string},
					{"version", &bflangversion, attribtype_string},
					{"table", &bflang->size_table, attribtype_int},
					{"matches", &bflang->size_matches, attribtype_int},
					{"contexts", &bflang->size_contexts, attribtype_int}};
			parse_attributes(bflang,reader, attribs, 5);
		} else if (xmlStrEqual(name, (xmlChar *) "header")) {
			process_header(reader, bflang);
			xmlFree(name);
			break;
		}
		xmlFree(name);
	}
	xmlFreeTextReader(reader);

	if (bflang->name == NULL) {
		g_print("Language file %s has no name.. abort..\n", filename);
		failed=TRUE;
	}
	if (g_strcmp0(bflangversion, CURRENT_BFLANG2_VERSION)!=0) {
		g_print("Language file %s has incompatible version %s (need version "CURRENT_BFLANG2_VERSION"), abort..\n", filename, bflangversion);
		failed=TRUE;
	}
	g_free(bflangversion);
	if (failed) {
		g_free(bflang->name);
		g_slice_free(Tbflang, bflang);
		return NULL;
	}
	return bflang;
}

GList *
langmgr_get_languages_mimetypes(void)
{
	GList *retlist = NULL, *tmplist = g_list_first(langmgr.bflang_list);
	while (tmplist) {
		if (((Tbflang *) (tmplist->data))->mimetypes)
			retlist = g_list_prepend(retlist, ((Tbflang *) (tmplist->data))->mimetypes->data);
		tmplist = tmplist->next;
	}
	return retlist;
}

gboolean
langmgr_in_highlight_tags(GtkTextTag * tag)
{
	gint i = 0;
	while (langmgr.highlight_tags[i]) {
		if (langmgr.highlight_tags[i] == tag)
			return TRUE;
		i++;
	}
	return FALSE;
}

#ifdef HAVE_LIBENCHANT
GtkTextTag **
langmgr_no_spellcheck_tags(void)
{
	return langmgr.no_spellcheck_tags;
}

GtkTextTag **
langmgr_need_spellcheck_tags(void)
{
	return langmgr.need_spellcheck_tags;
}
#endif

GList *
langmgr_get_languages(void)
{
	if (langmgr.done_scanning)
		return g_list_copy(langmgr.bflang_list);
	return NULL;
}

static gint
sort_bflang_list(gconstpointer a, gconstpointer b)
{
	return g_strcmp0(((Tbflang *) a)->name, ((Tbflang *) b)->name);
}

static void
bflang_cleanup_scantable(Tbflang * bflang)
{
	gint i;
	for (i = 1; i < bflang->st->matches->len; i++) {
		GSList *slist;
		g_free(g_array_index(bflang->st->matches, Tpattern, i).reference);
		g_free(g_array_index(bflang->st->matches, Tpattern, i).pattern);
		/* TODO: cleanup autocomplete list */
/*		g_free(g_array_index(bflang->st->matches, Tpattern, i).autocomplete_string);*/
		/* we cannot cleanup selfhighlight because there are several tags/elements that
		   use the same string in memory for this value... for example if they are part of
		   the same <group>
		   g_free(g_array_index(bflang->st->matches, Tpattern, i).selfhighlight); */
		/*g_free(g_array_index(bflang->st->matches, Tpattern, i).blockhighlight);*/
		for (slist = g_array_index(bflang->st->matches, Tpattern, i).autocomp_items; slist;
			 slist = g_slist_next(slist)) {
			g_slice_free(Tpattern_autocomplete, slist->data);
		}
		g_slist_free(g_array_index(bflang->st->matches, Tpattern, i).autocomp_items);
	}
	for (i = 1; i < bflang->st->contexts->len; i++) {
		if (g_array_index(bflang->st->contexts, Tcontext, i).ac)
			g_completion_free(g_array_index(bflang->st->contexts, Tcontext, i).ac);
		if (g_array_index(bflang->st->contexts, Tcontext, i).patternhash)
			g_hash_table_destroy(g_array_index(bflang->st->contexts, Tcontext, i).patternhash);
		g_free(g_array_index(bflang->st->contexts, Tcontext, i).contexthighlight);
		g_array_free(g_array_index(bflang->st->contexts, Tcontext, i).table, TRUE);
	}
	for (i = 1; i < bflang->st->comments->len; i++) {
		g_free(g_array_index(bflang->st->comments, Tcomment, i).so);
		g_free(g_array_index(bflang->st->comments, Tcomment, i).eo);
	}
	for (i = 1; i < bflang->st->blocks->len; i++) {
		g_free(g_array_index(bflang->st->blocks, Tpattern_block, i).name);
		g_free(g_array_index(bflang->st->blocks, Tpattern_block, i).highlight);
	}
	g_array_free(bflang->st->matches, TRUE);
	g_array_free(bflang->st->contexts, TRUE);
	g_array_free(bflang->st->comments, TRUE);
	g_array_free(bflang->st->blocks, TRUE);
	g_slice_free(Tscantable, bflang->st);
	bflang->st = NULL;
}


static void
bflang_cleanup(Tbflang * bflang)
{
	if (bflang->parsing) {
		DBG_PARSING("cannot cleanup a language that is still parsing\n");
		return;
	}
	if (bflang->st) {
		bflang_cleanup_scantable(bflang);
	}
	g_free(bflang->filename);
	g_free(bflang->name);
	free_stringlist(bflang->mimetypes);
	g_slice_free(Tbflang, bflang);
}
#ifdef MEMORY_LEAK_DEBUG
void
langmgr_cleanup(void)
{
	GList *tmplist = g_list_first(langmgr.bflang_list);
	while (tmplist) {
		bflang_cleanup(tmplist->data);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(langmgr.bflang_list);

	g_object_unref(G_OBJECT(langmgr.tagtable));
	g_hash_table_destroy(langmgr.configured_styles);
	g_hash_table_destroy(langmgr.bflang_options);
	g_hash_table_destroy(langmgr.option_descriptions);
	g_hash_table_destroy(langmgr.bflang_lookup);
	g_free(langmgr.highlight_tags);
#ifdef HAVE_LIBENCHANT
	g_free(langmgr.need_spellcheck_tags);
	g_free(langmgr.no_spellcheck_tags);
#endif
}
#endif

void
langmgr_add_custom_mime(Tbflang *bflang, const gchar *mime)
{
	main_v->globses.custombflangmime = g_list_prepend(main_v->globses.custombflangmime, array_from_arglist(bflang->name,mime,NULL));
	g_hash_table_insert(langmgr.bflang_lookup, g_strdup(mime), bflang);
}

static void
register_bflanguage(Tbflang * bflang)
{
	if (bflang) {
		GList *tmplist;
		gboolean registered = FALSE;
		DBG_PARSING("register_bflanguage, register bflang %p %s\n",bflang,bflang->name);

		/* see if the user has additional mime types */
		for (tmplist = g_list_last(main_v->globses.custombflangmime); tmplist; tmplist = tmplist->prev) {
			gchar **arr = (gchar **) tmplist->data;
			if (g_strv_length(arr) == 2 && strcmp(arr[0],bflang->name)==0) {
				DEBUG_MSG("adding additional user mime %s to %s\n",arr[1], arr[0]);
				bflang->mimetypes = g_list_prepend(bflang->mimetypes, g_strdup(arr[1]));
			}
		}

		tmplist = g_list_first(bflang->mimetypes);
		while (tmplist) {
			if (!g_hash_table_lookup(langmgr.bflang_lookup, (gchar *) tmplist->data)) {
				g_hash_table_insert(langmgr.bflang_lookup, (gchar *) tmplist->data, bflang);
				registered = TRUE;
			}
			tmplist = g_list_next(tmplist);
		}
		if (registered) {
			langmgr.bflang_list = g_list_prepend(langmgr.bflang_list, bflang);
		} else {
			bflang_cleanup(bflang);
		}
	}
}

static void
scan_dir_bflang2files(const gchar * dir)
{
	const gchar *filename;
	GError *error = NULL;
	GPatternSpec *ps = g_pattern_spec_new("*.bflang2");
	GDir *gd = g_dir_open(dir, 0, &error);
	DEBUG_PATH("load language files from %s\n", dir);
	if (!error) {
		filename = g_dir_read_name(gd);
		while (filename) {
			if (g_pattern_match(ps, strlen(filename), filename, NULL)) {
				Tbflang *bflang;
				gchar *path = g_strconcat(dir, "/", filename, NULL);
				bflang = parse_bflang2_header(path);
				register_bflanguage(bflang);
				g_free(path);
			}
			filename = g_dir_read_name(gd);
		}
		g_dir_close(gd);
	}
	g_pattern_spec_free(ps);
}
static gboolean
bflang2scan_finished_lcb(gpointer data)
{
	GList *tmplist;
	DEBUG_SIG("bflang2scan_finished_lcb, priority=%d\n",BFLANGSCAN_FINISHED_PRIORITY);
	langmgr.done_scanning = TRUE;
	/* now add the languages once the GUI if the GUI has been loaded */
	DBG_MSG("bflang2scan_finished_lcb\n");
	for (tmplist = g_list_first(main_v->bfwinlist); tmplist; tmplist = g_list_next(tmplist)) {
		GList *tmplist2;
		g_print("bflang2scan_finished, call lang_mode_menu_create() for bfwin %p\n", tmplist->data);
		lang_mode_menu_create(BFWIN(tmplist->data));
		for (tmplist2 = g_list_first(BFWIN(tmplist->data)->documentlist); tmplist2; tmplist2 = g_list_next(tmplist2)) {
			DBG_MSG("bflang2scan_finished, request bflang for document %p\n", tmplist2->data);
			if (DOCUMENT(tmplist2->data)->info || DOCUMENT(tmplist2->data)->uri) {
				bluefish_text_view_select_language(BLUEFISH_TEXT_VIEW(DOCUMENT(tmplist2->data)->view),
						DOCUMENT(tmplist2->data)->info ? g_file_info_get_content_type(DOCUMENT(tmplist2->data)->info) : NULL,
						DOCUMENT(tmplist2->data)->uri ? g_file_get_path(DOCUMENT(tmplist2->data)->uri): NULL);
			}
		}
	}
	return FALSE;
}

static gpointer
build_bflang2scan_thread(gpointer data)
{
	gchar *homebfdir;
	homebfdir = g_strconcat(g_get_home_dir(), "/." PACKAGE "/", NULL);
	scan_dir_bflang2files(homebfdir);
	g_free(homebfdir);
	scan_dir_bflang2files(PKGDATADIR "/bflang/");
	langmgr.bflang_list = g_list_sort(langmgr.bflang_list, (GCompareFunc) sort_bflang_list);
	g_idle_add_full(BFLANGSCAN_FINISHED_PRIORITY, bflang2scan_finished_lcb, NULL, NULL);
	return NULL;
}

void
langmgr_init(void)
{
	GList *tmplist;
	GtkTextTag *tag;
	GError *gerror=NULL;

	langmgr.tagtable = gtk_text_tag_table_new();
	langmgr.bflang_lookup = g_hash_table_new(g_str_hash, g_str_equal);
	langmgr.load_reference = main_v->props.load_reference;
	langmgr.configured_styles =
		g_hash_table_new_full(arr2_hash, arr2_equal, (GDestroyNotify) g_strfreev, g_free);
	langmgr.bflang_options =
		g_hash_table_new_full(arr2_hash, arr2_equal, (GDestroyNotify) g_strfreev, g_free);
	langmgr.option_descriptions =
		g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	tag = gtk_text_tag_new("_needscanning_");
	gtk_text_tag_table_add(langmgr.tagtable, tag);
	g_object_unref(tag);
#ifdef HAVE_LIBENCHANT
	tag = gtk_text_tag_new("_needspellcheck_");
	gtk_text_tag_table_add(langmgr.tagtable, tag);
	g_object_unref(tag);
	tag = gtk_text_tag_new("_spellerror_");
	g_object_set(tag, "underline", PANGO_UNDERLINE_ERROR, NULL);
	gtk_text_tag_table_add(langmgr.tagtable, tag);
	g_object_unref(tag);
#endif							/*HAVE_LIBENCHANT */
	tag = gtk_text_tag_new("_folded_");
	g_object_set(tag, "editable", FALSE, "invisible", TRUE, NULL);
	gtk_text_tag_table_add(langmgr.tagtable, tag);
	g_object_unref(tag);
	langmgr_reload_user_options();

	langmgr_reload_user_styles();
	if (!gtk_text_tag_table_lookup(langmgr.tagtable, "blockmatch")) {
		const gchar *arr[] = { "blockmatch", "#FFFFFF", "#FF0000", "0", "0", "0", NULL };
		tag = gtk_text_tag_new(arr[0]);
		g_object_set(tag, "foreground", arr[1], "background", arr[2], NULL);
		gtk_text_tag_table_add(langmgr.tagtable, tag);
		g_object_unref(tag);
		main_v->props.textstyles = g_list_prepend(main_v->props.textstyles, g_strdupv((gchar **) arr));
	}
	if (!gtk_text_tag_table_lookup(langmgr.tagtable, "searchresult")) {
		const gchar *arr[] = { "searchresult", "#000000", "#FFFF57", "0", "0", "0", NULL };
		tag = gtk_text_tag_new(arr[0]);
		g_object_set(tag, "foreground", arr[1], "background", arr[2], NULL);
		gtk_text_tag_table_add(langmgr.tagtable, tag);
		g_object_unref(tag);
		main_v->props.textstyles = g_list_prepend(main_v->props.textstyles, g_strdupv((gchar **) arr));
	}
	tag = gtk_text_tag_table_lookup(langmgr.tagtable, "foldheader");
	if (tag) {
		g_object_set(tag, "editable", FALSE, NULL);
	} else {
		const gchar *arr[] = { "foldheader", "", "#99FF99", "0", "0", "0", NULL };
		tag = gtk_text_tag_new("foldheader");
		g_object_set(tag, "editable", FALSE, "background", "#99FF99", NULL);
		gtk_text_tag_table_add(langmgr.tagtable, tag);
		g_object_unref(tag);
		main_v->props.textstyles = g_list_prepend(main_v->props.textstyles, g_strdupv((gchar **) arr));
	}
	for (tmplist = g_list_first(main_v->props.highlight_styles); tmplist; tmplist = tmplist->next) {
		gchar **arr = (gchar **) tmplist->data;
		if (arr[0] && arr[1] && arr[2]) {
			/*g_print("set style %s for highlight %s:%s\n",arr[2],arr2[0],arr2[1]); */
			g_hash_table_insert(langmgr.configured_styles, array_from_arglist(arr[0], arr[1], NULL),
								g_strdup(arr[2]));
		}
	}

	g_thread_create_full(build_bflang2scan_thread, NULL, 0, FALSE, FALSE, G_THREAD_PRIORITY_LOW, &gerror);
	if (gerror)
		g_print("failed to start build_bflang2scan_thread thread\n");

	DBG_PARSING("langmgr_init, returning \n");
}
