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
	return gtk_text_tag_table_lookup(langmgr.tagtable,style);
}

/* this is called in the mainloop again */
static gboolean build_lang_finished_lcb(gpointer data)
{
	Tbflangparsing *bfparser=data;
	bfparser->bflang->st = bfparser->st;
	/* now walk and rescan all documents that use this bflang */
	/* TODO */
	testapp_rescan_bflang(bfparser->bflang);
	
	/* cleanup the parsing structure */
	g_slice_free(Tbflangparsing,bfparser);
	
	
	return FALSE;
}

static gboolean set_string_if_attribute_name(xmlTextReaderPtr reader, gchar *aname, gchar *searchname, gchar **string) {
	xmlChar *avalue=NULL;
	if (xmlStrEqual(aname,searchname)) {
		avalue = xmlTextReaderValue(reader);
		*string = g_strdup(avalue);
		xmlFree(avalue);
		return TRUE;
	}
	return FALSE;
}

static gboolean set_boolean_if_attribute_name(xmlTextReaderPtr reader, gchar *aname, gchar *searchname, gboolean *bool) {
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
	xmlChar *name=NULL,*value;
	gint ret;
	ret = xmlTextReaderRead(reader);
	if (!ret)
		return;
	
	name = xmlTextReaderName(reader);
	
	while (ret && name && !xmlStrEqual(name,"detection")) {
		/*name = xmlTextReaderName(reader);*/
		if (xmlStrEqual(name,"mime")) {
			while (xmlTextReaderMoveToNextAttribute(reader)) {
				gchar *mimetype=NULL;
				xmlChar *aname = xmlTextReaderName(reader);
				set_string_if_attribute_name(reader,aname,"type", &mimetype);
				if (mimetype)
					bflang->mimetypes = g_list_prepend(bflang->mimetypes, mimetype);
				xmlFree(aname);
			}
		}
		xmlFree(name);
		name=NULL;
		ret = xmlTextReaderRead(reader);
		if (ret)
			name = xmlTextReaderName(reader);
	}
}
/* declaration needed for recursive calling */
static guint16 process_scanning_context(xmlTextReaderPtr reader, Tbflangparsing *bfparser, guint16 prevcontext);

static guint16 process_scanning_pattern(xmlTextReaderPtr reader, Tbflangparsing *bfparser, guint16 context, guint16 prevcontext) {
	gchar *pattern, *style, *blockstartpattern, *blockstyle;
	gboolean case_insens=FALSE, is_regex=FALSE, starts_block=FALSE, ends_block=FALSE,ends_context=FALSE, is_empty;
	is_empty = xmlTextReaderIsEmptyElement(reader);
	while (xmlTextReaderMoveToNextAttribute(reader)) {
		xmlChar *aname = xmlTextReaderName(reader);
		set_string_if_attribute_name(reader,aname,"name",&pattern);
		set_string_if_attribute_name(reader,aname,"style",&style);
		set_string_if_attribute_name(reader,aname,"blockstyle",&blockstyle);
		set_string_if_attribute_name(reader,aname,"blockstartpattern",&blockstartpattern);
		set_boolean_if_attribute_name(reader, aname, "is_regex", &is_regex);
		set_boolean_if_attribute_name(reader, aname, "starts_block", &starts_block);
		set_boolean_if_attribute_name(reader, aname, "ends_block", &ends_block);
		set_boolean_if_attribute_name(reader, aname, "case_insens", &case_insens);
		xmlFree(aname);
	}
	
	if (pattern) {
		GtkTextTag *stylet=NULL,*blockstylet=NULL;
		guint16 blockstartpatternum=0, nextcontext=context;
		guint16 matchnum;
		if (ends_context) {
			nextcontext=prevcontext;
		}
		stylet = langmrg_lookup_style(style);
		blockstylet = langmrg_lookup_style(blockstyle);
		if (blockstartpattern)
			blockstartpatternum = GPOINTER_TO_INT(g_hash_table_lookup(bfparser->patterns, blockstartpattern));
		matchnum = add_keyword_to_scanning_table(bfparser->st, pattern, is_regex,case_insens, stylet, context, nextcontext
				, starts_block, ends_block, blockstartpattern,blockstylet,FALSE, NULL);
		g_hash_table_insert(bfparser->patterns, pattern, GPOINTER_TO_INT(matchnum));
		/* now check if there is a deeper context */
		if (!is_empty) {
			gint ret;
			xmlChar *name=NULL;
			ret = xmlTextReaderRead(reader);
			if (ret) name = xmlTextReaderName(reader);
			while (ret && name) {
				if (xmlStrEqual(name,"context")) {
					nextcontext = process_scanning_context(reader,bfparser,context);
					match_set_nextcontext(bfparser->st, matchnum, nextcontext);
				} if (xmlStrEqual(name,"pattern")) {
					break;
				}
				xmlFree(name);
				name=NULL;
				ret = xmlTextReaderRead(reader);
				if (ret)	name = xmlTextReaderName(reader);
			}
			if (name)
				xmlFree(name);
		}
	}

}
static guint16 process_scanning_keyword(xmlTextReaderPtr reader, Tbflangparsing *bfparser, guint16 context) {
	gchar *name=NULL, *style=NULL, *reference=NULL;
	gboolean autocomplete=FALSE, is_empty;
	is_empty = xmlTextReaderIsEmptyElement(reader);
	g_print("processing keyword...\n");
	while (xmlTextReaderMoveToNextAttribute(reader)) {
		xmlChar *aname = xmlTextReaderName(reader);
		set_string_if_attribute_name(reader,aname,"name",&name);
		set_string_if_attribute_name(reader,aname,"style",&style);
		set_boolean_if_attribute_name(reader,aname, "autocomplete", &autocomplete);
		xmlFree(aname);
	}
	if (!is_empty) {
		gint ret;
		/* get reference data */
		reference = xmlTextReaderReadInnerXml(reader);
		ret = xmlTextReaderRead(reader);
		while (ret) {
			xmlChar *name = xmlTextReaderName(reader);
			if (xmlStrEqual(name,"keyword")) { /* end of keyword */
				break;
			}
			ret = xmlTextReaderRead(reader);
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
	gint ret;
	while (xmlTextReaderMoveToNextAttribute(reader)) {
		xmlChar *aname = xmlTextReaderName(reader);
		set_string_if_attribute_name(reader,aname,"symbols",&symbols);
		set_string_if_attribute_name(reader,aname,"style",&style);
		xmlFree(aname);
	}
	/* create context */
	g_print("create context symbols %s and style %s\n",symbols,style);
	context = new_context(bfparser->st,symbols,style);
	/* now get the children */
	ret = xmlTextReaderRead(reader);
	while (ret == 1) {
		xmlChar *name;
		name = xmlTextReaderName(reader);
		g_print("parsing context, found node name %s\n",name);
		if (xmlStrEqual(name,"pattern")) {
			process_scanning_pattern(reader,bfparser,context,prevcontext);
		} else if (xmlStrEqual(name,"keyword")) {
			process_scanning_keyword(reader,bfparser,context);
		} else if (xmlStrEqual(name,"context")) {
			xmlFree(name);
			return context;
		}
		xmlFree(name);
		ret = xmlTextReaderRead(reader);
	}
	return context;
}

static gpointer build_lang_thread(gpointer data)
{
	xmlTextReaderPtr reader;
	gint ret;
	Tbflang *bflang = data;
	Tbflangparsing *bfparser;
	
	bfparser = g_slice_new0(Tbflangparsing);
	bfparser->patterns =  g_hash_table_new(g_str_hash,g_str_equal);
	bfparser->bflang = bflang;
	bfparser->st = scantable_new();
	
	g_print("build_lang_thread %p, started for %s\n",g_thread_self(),bfparser->bflang->filename);
	reader = xmlNewTextReaderFilename(bfparser->bflang->filename);
	xmlTextReaderSetParserProp(reader,XML_PARSER_SUBST_ENTITIES,TRUE);
	if (reader != NULL) {
		ret = xmlTextReaderRead(reader);
		while (ret == 1) {
			xmlChar *name;
			name = xmlTextReaderName(reader);
			g_print("found %s\n",name);
			if (xmlStrEqual(name,"detection")) {
				g_print("processing <detection>\n");
				process_detection(reader,bflang);
			} else if (xmlStrEqual(name,"scanning")) {
				g_print("processing <scanning>\n");
				/* next node should be main context */
				ret = xmlTextReaderRead(reader);
				if (ret) {
					xmlChar *name2;
					name2 = xmlTextReaderName(reader);
					g_print("found %s\n",name2);
					if (xmlStrEqual(name2,"context")) {
						process_scanning_context(reader,bfparser,0);
					}
					xmlFree(name2);
				}
			}
			xmlFree(name);
			ret = xmlTextReaderRead(reader);
		}
		xmlFreeTextReader(reader);
		if (ret != 0) {
			g_print("%s : failed to parse\n", bflang->filename);
		}
	} else {
		g_print("Unable to open %s\n", bflang->filename);
	}


	g_print("build_lang_thread finished bflang=%p\n",bflang);
	print_scantable_stats(bfparser->st);
	
	/* when done call mainloop */
	g_idle_add(build_lang_finished_lcb, bfparser);
	return bflang;
}

Tbflang *langmgr_get_bflang_for_mimetype(const gchar *mimetype) {
	Tbflang *bflang;
	
	bflang = g_hash_table_lookup(langmgr.bflang_lookup,mimetype);
	if (bflang && bflang->filename && !bflang->st) {
		GError *error=NULL;
		GThread* thread;
		DBG_MSG("no scantable, start thread\n");
		thread = g_thread_create(build_lang_thread,bflang,FALSE,&error);
		if (error) {
			g_print("start thread, error\n");
		}
	}
	return bflang;
}

void langmgr_init(void) {
	Tbflang *bflang;
	langmgr.tagtable = gtk_text_tag_table_new();
	langmgr.bflang_lookup = g_hash_table_new(g_str_hash,g_str_equal);
	
 	bflang = g_slice_new0(Tbflang);
 	bflang->filename = "c.bflang2";
 	bflang->mimetypes = g_list_prepend(bflang->mimetypes, "text/x-csrc");
 	bflang->mimetypes = g_list_prepend(bflang->mimetypes, "text/x-chdr");
	g_hash_table_insert(langmgr.bflang_lookup, "text/x-csrc", bflang);
	g_hash_table_insert(langmgr.bflang_lookup, "text/x-chdr", bflang);
	langmgr.bflang_list = g_list_prepend(langmgr.bflang_list, bflang);
	g_print("langmgr_init, returning \n");
}
