#include <libxml/xmlreader.h>

#include "bftextview2.h"
#include "bftextview2_langmgr.h"
#include "bftextview2_patcompile.h"

typedef struct {
	GHashTable *patterns;
	Tscantable *st; /* while not finished */
	Tbflang *bflang;
} Tbflangparsing;

typedef struct {
	GHashTable *bflang_lookup;
	GList *bflang_list;
	GtkTextTagTable *tagtable;
} Tlangmgr;

static Tlangmgr langmgr = {NULL,NULL,NULL};

GtkTextTag *langmrg_lookup_style(const gchar *style) {
	GtkTextTag *tag=NULL;
	if (style)
		tag = gtk_text_tag_table_lookup(langmgr.tagtable,style);
	DBG_PARSING("found tag %p for style %s\n",tag,style);
	return tag;
}

/* this is called in the mainloop again */
static gboolean build_lang_finished_lcb(gpointer data)
{
	Tbflangparsing *bfparser=data;
	bfparser->bflang->st = bfparser->st;
	bfparser->bflang->parsing=FALSE;
	/* now walk and rescan all documents that use this bflang */
	/* TODO */
	testapp_rescan_bflang(bfparser->bflang);
	
	/* cleanup the parsing structure */
	g_slice_free(Tbflangparsing,bfparser);
	
	
	return FALSE;
}

static gboolean set_string_if_attribute_name(xmlTextReaderPtr reader, xmlChar *aname, xmlChar *searchname, gchar **string) {
	xmlChar *avalue=NULL;
	if (xmlStrEqual(aname,searchname)) {
		avalue = xmlTextReaderValue(reader);
		*string = g_strdup((gchar *)avalue);
		xmlFree(avalue);
		return TRUE;
	}
	return FALSE;
}

static gboolean set_boolean_if_attribute_name(xmlTextReaderPtr reader, xmlChar *aname, xmlChar *searchname, gboolean *bool) {
	gchar *tmp=NULL;
	if (set_string_if_attribute_name(reader, aname, searchname, &tmp)) {
		if (tmp && tmp[0] == '1')
			*bool = TRUE;
		g_free(tmp);
		return TRUE;
	}
	return FALSE;
}

static void process_detection(xmlTextReaderPtr reader, Tbflang *bflang) {
	while (xmlTextReaderRead(reader)==1) {
		xmlChar *name=xmlTextReaderName(reader);
		if (xmlStrEqual(name,(xmlChar *)"mime")) {
			while (xmlTextReaderMoveToNextAttribute(reader)) {
				gchar *mimetype=NULL;
				xmlChar *aname = xmlTextReaderName(reader);
				set_string_if_attribute_name(reader,aname,(xmlChar *)"type", &mimetype);
				if (mimetype)
					bflang->mimetypes = g_list_prepend(bflang->mimetypes, mimetype);
				xmlFree(aname);
			}
		} else if (xmlStrEqual(name,(xmlChar *)"detection")) {
			xmlFree(name);
			break;
		}
	}
}
/* declaration needed for recursive calling */
static guint16 process_scanning_context(xmlTextReaderPtr reader, Tbflangparsing *bfparser, guint16 prevcontext);

static guint16 process_scanning_pattern(xmlTextReaderPtr reader, Tbflangparsing *bfparser, guint16 context, guint16 prevcontext) {
	guint16 matchnum;
	gchar *pattern=NULL, *style=NULL, *blockstartpattern=NULL, *blockstyle=NULL;
	gboolean case_insens=FALSE, is_regex=FALSE, starts_block=FALSE, ends_block=FALSE,ends_context=FALSE, is_empty;
	is_empty = xmlTextReaderIsEmptyElement(reader);
	while (xmlTextReaderMoveToNextAttribute(reader)) {
		xmlChar *aname = xmlTextReaderName(reader);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"name",&pattern);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"style",&style);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"blockstyle",&blockstyle);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"blockstartpattern",&blockstartpattern);
		set_boolean_if_attribute_name(reader, aname, (xmlChar *)"is_regex", &is_regex);
		set_boolean_if_attribute_name(reader, aname, (xmlChar *)"starts_block", &starts_block);
		set_boolean_if_attribute_name(reader, aname, (xmlChar *)"ends_block", &ends_block);
		set_boolean_if_attribute_name(reader, aname, (xmlChar *)"case_insens", &case_insens);
		set_boolean_if_attribute_name(reader, aname, (xmlChar *)"ends_context", &ends_context);
		xmlFree(aname);
	}
	
	if (pattern) {
		GtkTextTag *stylet=NULL,*blockstylet=NULL;
		guint16 blockstartpatternum=0, nextcontext=context;
		if (ends_context) {
			nextcontext=prevcontext;
		}
		stylet = langmrg_lookup_style(style);
		blockstylet = langmrg_lookup_style(blockstyle);
		if (blockstartpattern)
			blockstartpatternum = GPOINTER_TO_INT(g_hash_table_lookup(bfparser->patterns, blockstartpattern));
		matchnum = add_keyword_to_scanning_table(bfparser->st, pattern, is_regex,case_insens, stylet, context, nextcontext
				, starts_block, ends_block, blockstartpatternum,blockstylet,FALSE, NULL);
		DBG_PARSING("add matchnum %d to hash table for key %s\n",matchnum,pattern);
		g_hash_table_insert(bfparser->patterns, pattern, GINT_TO_POINTER((gint)matchnum));
		/* now check if there is a deeper context */
		if (!is_empty) {
			while (xmlTextReaderRead(reader)==1) {
				xmlChar *name=xmlTextReaderName(reader);
				if (xmlStrEqual(name,(xmlChar *)"context")) {
					nextcontext = process_scanning_context(reader,bfparser,context);
					match_set_nextcontext(bfparser->st, matchnum, nextcontext);
				} else if (xmlStrEqual(name,(xmlChar *)"pattern")) {
					xmlFree(name);
					break;
				}
				xmlFree(name);
			}
		}
	}
	return matchnum;
}
static guint16 process_scanning_keyword(xmlTextReaderPtr reader, Tbflangparsing *bfparser, guint16 context) {
	gchar *name=NULL, *style=NULL, *reference=NULL;
	gboolean autocomplete=FALSE, is_empty;
	is_empty = xmlTextReaderIsEmptyElement(reader);
	DBG_PARSING("processing keyword...\n");
	while (xmlTextReaderMoveToNextAttribute(reader)) {
		xmlChar *aname = xmlTextReaderName(reader);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"name",&name);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"style",&style);
		set_boolean_if_attribute_name(reader,aname, (xmlChar *)"autocomplete", &autocomplete);
		xmlFree(aname);
	}
	if (!is_empty) {
		gint ret;
		/* get reference data */
		reference = (gchar *)xmlTextReaderReadInnerXml(reader);
		ret = xmlTextReaderRead(reader);
		while (xmlTextReaderRead(reader)==1) {
			xmlChar *name = xmlTextReaderName(reader);
			if (xmlStrEqual(name,(xmlChar *)"keyword")) { /* end of keyword */
				break;
			}
		} 
	} 
	
	if (name) {
		guint16 matchnum;
		GtkTextTag *stylet;
		stylet = langmrg_lookup_style(style);
		matchnum = add_keyword_to_scanning_table(bfparser->st, name, FALSE,FALSE, stylet, context, context
				, FALSE, FALSE, 0,NULL,autocomplete, reference);
		return matchnum;
	}
	return 0;
}
static guint16 process_scanning_context(xmlTextReaderPtr reader, Tbflangparsing *bfparser, guint16 prevcontext) {
	gchar *symbols=NULL, *style=NULL;
	guint16 context;
	while (xmlTextReaderMoveToNextAttribute(reader)) {
		xmlChar *aname = xmlTextReaderName(reader);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"symbols",&symbols);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"style",&style);
		xmlFree(aname);
	}
	/* create context */
	DBG_PARSING("create context symbols %s and style %s\n",symbols,style);
	context = new_context(bfparser->st,symbols,langmrg_lookup_style(style));
	/* now get the children */
	while (xmlTextReaderRead(reader)==1) {
		xmlChar *name = xmlTextReaderName(reader);
		DBG_PARSING("parsing context, found node name %s\n",name);
		if (xmlStrEqual(name,(xmlChar *)"pattern")) {
			process_scanning_pattern(reader,bfparser,context,prevcontext);
		} else if (xmlStrEqual(name,(xmlChar *)"keyword")) {
			process_scanning_keyword(reader,bfparser,context);
		} else if (xmlStrEqual(name,(xmlChar *)"context")) {
			xmlFree(name);
			DBG_PARSING("end-of-context, return context %d\n",context);
			return context;
		}
		xmlFree(name);
	}
	return context;
}

static gpointer build_lang_thread(gpointer data)
{
	xmlTextReaderPtr reader;
	Tbflang *bflang = data;
	Tbflangparsing *bfparser;
	
	bfparser = g_slice_new0(Tbflangparsing);
	bfparser->patterns =  g_hash_table_new(g_str_hash,g_str_equal);
	bfparser->bflang = bflang;
	bfparser->st = scantable_new();
	
	DBG_PARSING("build_lang_thread %p, started for %s\n",g_thread_self(),bfparser->bflang->filename);
	reader = xmlNewTextReaderFilename(bfparser->bflang->filename);
	if (!reader) {
		g_print("failed to open %s\n",bfparser->bflang->filename);
		/* TODO CLEANUP*/
		return NULL;
	}
	xmlTextReaderSetParserProp(reader,XML_PARSER_SUBST_ENTITIES,TRUE);

	while (xmlTextReaderRead(reader) == 1) {
		xmlChar *name = xmlTextReaderName(reader);
		DBG_PARSING("found %s\n",name);
		if (xmlStrEqual(name,(xmlChar *)"detection")) {
			/* actually we can skip detection */
			DBG_PARSING("processing <detection>\n");
			process_detection(reader,bflang);
		} else if (xmlStrEqual(name,(xmlChar *)"scanning")) {
			DBG_PARSING("processing <scanning>\n");
			while (xmlTextReaderRead(reader)==1) {
				xmlChar *name2 = xmlTextReaderName(reader);
				if (xmlStrEqual(name2,(xmlChar *)"context")) {
					process_scanning_context(reader,bfparser,0);
				} else if (xmlStrEqual(name2,(xmlChar *)"scanning")) {
					xmlFree(name2);
					break;
				}
				xmlFree(name2);
			}
		}
		xmlFree(name);
	}
	xmlFreeTextReader(reader);

	DBG_PARSING("build_lang_thread finished bflang=%p\n",bflang);
	print_scantable_stats(bfparser->st);
	/*print_DFA(bfparser->st, '0','9');*/
	
	/* when done call mainloop */
	g_idle_add(build_lang_finished_lcb, bfparser);
	return bflang;
}

Tbflang *langmgr_get_bflang_for_mimetype(const gchar *mimetype) {
	Tbflang *bflang;
	
	bflang = g_hash_table_lookup(langmgr.bflang_lookup,mimetype);
	if (bflang && bflang->filename && !bflang->st && !bflang->parsing) {
		GError *error=NULL;
		GThread* thread;
		bflang->parsing=TRUE;
		DBG_MSG("no scantable, start thread\n");
		thread = g_thread_create(build_lang_thread,bflang,FALSE,&error);
		if (error) {
			DBG_PARSING("start thread, error\n");
		}
	}
	return bflang;
}

GtkTextTagTable *langmgr_get_tagtable(void) {
	return langmgr.tagtable;
}

static Tbflang *parse_bflang2_header(const gchar *filename) {
	xmlTextReaderPtr reader;
	Tbflang *bflang=NULL; 
	reader = xmlNewTextReaderFilename(filename);
	xmlTextReaderSetParserProp(reader,XML_PARSER_SUBST_ENTITIES,TRUE);
	if (reader != NULL) {
		while (xmlTextReaderRead(reader) == 1 && bflang==NULL) {
			xmlChar *name = xmlTextReaderName(reader);
			if (xmlStrEqual(name,(xmlChar *)"detection")) {
				bflang = g_slice_new0(Tbflang);
				bflang->filename = g_strdup(filename);
				process_detection(reader,bflang);
			}
			xmlFree(name);
		}
		xmlFreeTextReader(reader);
	}
	return bflang;
}

static void register_bflanguage(Tbflang *bflang) {
	if (bflang) {
		GList *tmplist;
		
		tmplist = g_list_first(bflang->mimetypes);
		while (tmplist) {
			g_hash_table_insert(langmgr.bflang_lookup, (gchar *)tmplist->data, bflang);
			tmplist = g_list_next(tmplist);
		}
		langmgr.bflang_list = g_list_prepend(langmgr.bflang_list, bflang);
	}
}

void langmgr_init(void) {
	Tbflang *bflang;
	langmgr.tagtable = gtk_text_tag_table_new();
	langmgr.bflang_lookup = g_hash_table_new(g_str_hash,g_str_equal);
	
	bflang = parse_bflang2_header("c.bflang2");
	register_bflanguage(bflang);
	bflang = parse_bflang2_header("php.bflang2");
	register_bflanguage(bflang);
	DBG_PARSING("langmgr_init, returning \n");
}
