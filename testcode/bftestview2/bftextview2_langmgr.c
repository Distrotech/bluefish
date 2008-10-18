#include <libxml/xmlreader.h>

#include "bftextview2.h"
#include "bftextview2_langmgr.h"
#include "bftextview2_patcompile.h"

typedef struct {
	GHashTable *patterns;
	GHashTable *setoptions;
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
	g_hash_table_destroy(bfparser->patterns);
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

static gboolean set_integer_if_attribute_name(xmlTextReaderPtr reader, xmlChar *aname, xmlChar *searchname, gint *integer) {
	gchar *tmp=NULL;
	if (set_string_if_attribute_name(reader, aname, searchname, &tmp)) {
		if (tmp && tmp[0] != '\0')
			*integer = (gint)g_ascii_strtoll(tmp,NULL,10);
		g_free(tmp);
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
		} else if (xmlStrEqual(name,(xmlChar *)"option")) {
			gchar *optionname=NULL;
			gboolean defaultval=FALSE;
			while (xmlTextReaderMoveToNextAttribute(reader)) {
				xmlChar *aname = xmlTextReaderName(reader);
				set_string_if_attribute_name(reader,aname,(xmlChar *)"name", &optionname);
				set_boolean_if_attribute_name(reader,aname,(xmlChar *)"default", &defaultval);
				xmlFree(aname);
			}
			if (optionname) {
				bflang->langoptions = g_list_prepend(bflang->langoptions, optionname);
				if (defaultval) {
					bflang->setoptions = g_list_prepend(bflang->setoptions, optionname);
				}
			}
		} else if (xmlStrEqual(name,(xmlChar *)"detection")) {
			xmlFree(name);
			break;
		}
		xmlFree(name);
	}
}
/* declaration needed for recursive calling */
static guint16 process_scanning_context(xmlTextReaderPtr reader, Tbflangparsing *bfparser, GQueue *contextstack);

static guint16 process_scanning_pattern(xmlTextReaderPtr reader, Tbflangparsing *bfparser, guint16 context, GQueue *contextstack) {
	guint16 matchnum;
	gchar *pattern=NULL, *style=NULL, *blockstartpattern=NULL, *blockstyle=NULL, *class=NULL;
	gboolean case_insens=FALSE, is_regex=FALSE, starts_block=FALSE, ends_block=FALSE, is_empty;
	gint ends_context=0;
	is_empty = xmlTextReaderIsEmptyElement(reader);
	while (xmlTextReaderMoveToNextAttribute(reader)) {
		xmlChar *aname = xmlTextReaderName(reader);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"name",&pattern);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"style",&style);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"blockstyle",&blockstyle);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"blockstartpattern",&blockstartpattern);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"class",&class);
		set_boolean_if_attribute_name(reader, aname, (xmlChar *)"is_regex", &is_regex);
		set_boolean_if_attribute_name(reader, aname, (xmlChar *)"starts_block", &starts_block);
		set_boolean_if_attribute_name(reader, aname, (xmlChar *)"ends_block", &ends_block);
		set_boolean_if_attribute_name(reader, aname, (xmlChar *)"case_insens", &case_insens);
		set_integer_if_attribute_name(reader, aname, (xmlChar *)"ends_context", &ends_context);
		xmlFree(aname);
	}
	if (!class || g_hash_table_lookup(bfparser->setoptions,class)) {
		if (pattern && pattern[0]) {
			GtkTextTag *stylet=NULL,*blockstylet=NULL;
			guint16 blockstartpatternum=0, nextcontext=context;
			DBG_PARSING("pattern %s\n",pattern);
			if (ends_context) {
				/* the nth number in the stack */
				nextcontext=GPOINTER_TO_INT(g_queue_peek_nth(contextstack,ends_context));
				if (!is_empty)
					g_print("LANGUAGE FILE COMPILE ERROR: pattern %s ends one or more contexts, but it is not an empty pattern\n",pattern);
				
			}
			stylet = langmrg_lookup_style(style);
			blockstylet = langmrg_lookup_style(blockstyle);
			if (blockstartpattern) {
				blockstartpatternum = GPOINTER_TO_INT(g_hash_table_lookup(bfparser->patterns, blockstartpattern));
				DBG_PARSING("got blockstartpatternum %d for blockstartpattern %s, ends_block=%d\n",blockstartpatternum,blockstartpattern,ends_block);
			}
			matchnum = add_keyword_to_scanning_table(bfparser->st, pattern, is_regex,case_insens, stylet, context, nextcontext
					, starts_block, ends_block, blockstartpatternum,blockstylet,FALSE,NULL, NULL);
			DBG_PARSING("add matchnum %d to hash table for key %s, starts_block=%d\n",matchnum,pattern,starts_block);
			g_hash_table_insert(bfparser->patterns, g_strdup(pattern), GINT_TO_POINTER((gint)matchnum));
			/* now check if there is a deeper context */
			if (!is_empty) {
				while (xmlTextReaderRead(reader)==1) {
					xmlChar *name=xmlTextReaderName(reader);
					if (xmlStrEqual(name,(xmlChar *)"context")) {
						nextcontext = process_scanning_context(reader,bfparser,contextstack);
						match_set_nextcontext(bfparser->st, matchnum, nextcontext);
					} else if (xmlStrEqual(name,(xmlChar *)"pattern")) {
						xmlFree(name);
						break;
					}
					xmlFree(name);
				}
			}
		}
	}
	/* TODO cleanup! */
	if (pattern) xmlFree(pattern);
	if (style) xmlFree(style);
	if (blockstartpattern) xmlFree(blockstartpattern);
	if (blockstyle) xmlFree(blockstyle);
	if (class) xmlFree(class);
	return matchnum;
}
static guint16 process_scanning_keyword(xmlTextReaderPtr reader, Tbflangparsing *bfparser, guint16 context) {
	gchar *name=NULL, *style=NULL, *reference=NULL, *class=NULL, *autocomplete_append=NULL;
	gboolean autocomplete=FALSE,case_insens=FALSE, is_empty;
	guint16 matchnum=0;
	is_empty = xmlTextReaderIsEmptyElement(reader);
	DBG_PARSING("processing keyword...\n");
	while (xmlTextReaderMoveToNextAttribute(reader)) {
		xmlChar *aname = xmlTextReaderName(reader);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"name",&name);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"style",&style);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"class",&class);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"autocomplete_append",&autocomplete_append);
		set_boolean_if_attribute_name(reader,aname, (xmlChar *)"autocomplete", &autocomplete);
		set_boolean_if_attribute_name(reader,aname, (xmlChar *)"case_insens", &case_insens);
		xmlFree(aname);
	}
	if (!is_empty) {
		gint ret;
		/* get reference data */
		reference = (gchar *)xmlTextReaderReadInnerXml(reader);
		ret = xmlTextReaderRead(reader);
		while (xmlTextReaderRead(reader)==1) {
			xmlChar *name2 = xmlTextReaderName(reader);
			if (xmlStrEqual(name2,(xmlChar *)"keyword")) { /* end of keyword */
				xmlFree(name2);
				break;
			}
			xmlFree(name2);
		} 
	}
	if (!class || g_hash_table_lookup(bfparser->setoptions,class)) {
		if (name && name[0]) {
			GtkTextTag *stylet;
			stylet = langmrg_lookup_style(style);
			matchnum = add_keyword_to_scanning_table(bfparser->st, name, FALSE,case_insens, stylet, context, context
					, FALSE, FALSE, 0,NULL,autocomplete,autocomplete_append, reference);
		}
	}
	if (name) g_free(name);
	if (style) g_free(style);
	if (class) g_free(class);
	if (reference) xmlFree(reference);
	if (autocomplete_append) g_free(autocomplete_append);
	return matchnum;
}

static guint16 process_scanning_tag(xmlTextReaderPtr reader, Tbflangparsing *bfparser, guint16 context, GQueue *contextstack) {
	gchar *tag=NULL, *style=NULL, *attributes=NULL, *attribstyle=NULL,*class=NULL, *autocomplete_append=NULL,*attrib_autocomplete_append=NULL;
	guint16 matchnum=0,innercontext=context;
	gboolean is_empty;
	DBG_PARSING("processing tag...\n");
	is_empty = xmlTextReaderIsEmptyElement(reader);
	while (xmlTextReaderMoveToNextAttribute(reader)) {
		xmlChar *aname = xmlTextReaderName(reader);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"name",&tag);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"style",&style);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"class",&class);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"autocomplete_append",&autocomplete_append);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"attribstyle",&attribstyle);
		set_string_if_attribute_name(reader,aname, (xmlChar *)"attributes", &attributes);
		set_string_if_attribute_name(reader,aname, (xmlChar *)"attrib_autocomplete_append", &attrib_autocomplete_append);
		xmlFree(aname);
	}
	if (!class || g_hash_table_lookup(bfparser->setoptions,class)) {
		if (tag && tag[0]) {
			GtkTextTag *stylet;
			stylet = langmrg_lookup_style(style);
			guint16 contexttag, contextstring, starttagmatch, endtagmatch;
			gchar *tmp;
		
			contexttag = new_context(bfparser->st, ">\"=' \t\n\r", NULL, FALSE);
			tmp = g_strconcat("<",tag,NULL);
			matchnum = add_keyword_to_scanning_table(bfparser->st, tmp, FALSE, FALSE, stylet, context, contexttag, TRUE, FALSE, 0, NULL,TRUE,autocomplete_append,NULL);
			g_free(tmp);
			starttagmatch = add_keyword_to_scanning_table(bfparser->st, "/?>", TRUE, FALSE, stylet, contexttag, context, FALSE, FALSE, 0, NULL,FALSE,NULL,NULL);
			if (attributes) {
				gchar **arr, **tmp2;
				GtkTextTag *string, *attrib = langmrg_lookup_style(attribstyle);
				string = langmrg_lookup_style("string");
				arr = g_strsplit(attributes,",",-1);
				tmp2 = arr;
				while (*tmp2) {
					add_keyword_to_scanning_table(bfparser->st, *tmp2, FALSE, FALSE, attrib, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,attrib_autocomplete_append,NULL);
					tmp2++;
				}
				g_strfreev(arr);
				contextstring = new_context(bfparser->st, "\"=' \t\n\r", string, FALSE);
				add_keyword_to_scanning_table(bfparser->st, "\"", FALSE, FALSE, string, contexttag, contextstring, FALSE, FALSE, 0, NULL,FALSE,NULL,NULL);
				add_keyword_to_scanning_table(bfparser->st, "\"", FALSE, FALSE, string, contextstring, contexttag, FALSE, FALSE, 0, string,FALSE,NULL,NULL);
			}
			
			if (!is_empty) {
				while (xmlTextReaderRead(reader)==1) {
					xmlChar *name=xmlTextReaderName(reader);
					if (xmlStrEqual(name,(xmlChar *)"context")) {
						innercontext = process_scanning_context(reader,bfparser,contextstack);
						match_set_nextcontext(bfparser->st, starttagmatch, innercontext);
					} else if (xmlStrEqual(name,(xmlChar *)"tag")) {
						xmlFree(name);
						break;
					}
					xmlFree(name);
				}
			}
			tmp = g_strconcat("</",tag,">",NULL);
			endtagmatch = add_keyword_to_scanning_table(bfparser->st, tmp, FALSE, FALSE, stylet, innercontext, context, FALSE, TRUE, matchnum, NULL,TRUE,NULL,NULL);
			g_free(tmp);
		}
	}
	
	g_free(tag);
	g_free(style);
	g_free(class);
	g_free(autocomplete_append);
	g_free(attrib_autocomplete_append);
	g_free(attribstyle);
	g_free(attributes);

	return matchnum;
}

static guint16 process_scanning_context(xmlTextReaderPtr reader, Tbflangparsing *bfparser, GQueue *contextstack) {
	gchar *symbols=NULL, *style=NULL;
	gboolean autocomplete_case_insens=FALSE;
	guint16 context;
	while (xmlTextReaderMoveToNextAttribute(reader)) {
		xmlChar *aname = xmlTextReaderName(reader);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"symbols",&symbols);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"style",&style);
		set_boolean_if_attribute_name(reader,aname,(xmlChar *)"autocomplete_case_insens",&autocomplete_case_insens);
		xmlFree(aname);
	}
	/* create context */
	DBG_PARSING("create context symbols %s and style %s\n",symbols,style);
	context = new_context(bfparser->st,symbols,langmrg_lookup_style(style),autocomplete_case_insens);
	g_queue_push_head(contextstack,GINT_TO_POINTER((gint)context)); 
	
	g_free(symbols);
	g_free(style);
	/* now get the children */
	while (xmlTextReaderRead(reader)==1) {
		xmlChar *name = xmlTextReaderName(reader);
		DBG_PARSING("parsing context, found node name %s\n",name);
		if (xmlStrEqual(name,(xmlChar *)"pattern")) {
			process_scanning_pattern(reader,bfparser,context,contextstack);
		} else if (xmlStrEqual(name,(xmlChar *)"keyword")) {
			process_scanning_keyword(reader,bfparser,context);
		} else if (xmlStrEqual(name,(xmlChar *)"tag")) {
			process_scanning_tag(reader,bfparser,context,contextstack);
		} else if (xmlStrEqual(name,(xmlChar *)"context")) {
			xmlFree(name);
			DBG_PARSING("end-of-context, return context %d\n",context);
			g_queue_pop_head(contextstack); 
			return context;
		} else
			DBG_PARSING("found %s\n",name);
		xmlFree(name);
	}
	/* can we ever get here ?? */
	g_print("pop context %d\n",GPOINTER_TO_INT(g_queue_peek_head(contextstack)));
	g_queue_pop_head(contextstack); 
	return context;
}

static gpointer build_lang_thread(gpointer data)
{
	xmlTextReaderPtr reader;
	Tbflang *bflang = data;
	Tbflangparsing *bfparser;
	GList *tmplist;
	
	bfparser = g_slice_new0(Tbflangparsing);
	bfparser->patterns =  g_hash_table_new_full(g_str_hash,g_str_equal,g_free,NULL);
	bfparser->setoptions =  g_hash_table_new(g_str_hash,g_str_equal);
	bfparser->bflang = bflang;
	for(tmplist = g_list_first(bfparser->bflang->setoptions);tmplist;tmplist=g_list_next(tmplist)) {
		g_hash_table_insert(bfparser->setoptions,tmplist->data,GINT_TO_POINTER(1));
	}
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
					GQueue *contextstack = g_queue_new();
					process_scanning_context(reader,bfparser,contextstack);
				} else if (xmlStrEqual(name2,(xmlChar *)"scanning")) {
					xmlFree(name2);
					break;
				} else
					DBG_PARSING("found %s\n",name2);
				xmlFree(name2);
			}
		}
		xmlFree(name);
	}
	xmlFreeTextReader(reader);

	DBG_PARSING("build_lang_thread finished bflang=%p\n",bflang);
	print_scantable_stats(bfparser->st);
	/*print_DFA(bfparser->st, '&','Z');*/
	
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
	bflang = parse_bflang2_header("html.bflang2");
	register_bflanguage(bflang);
	bflang = parse_bflang2_header("xml.bflang2");
	register_bflanguage(bflang);
	bflang = parse_bflang2_header("python.bflang2");
	register_bflanguage(bflang);
	bflang = parse_bflang2_header("asp.bflang2");
	register_bflanguage(bflang);
	bflang = parse_bflang2_header("perl.bflang2");
	register_bflanguage(bflang);

	DBG_PARSING("langmgr_init, returning \n");
}
