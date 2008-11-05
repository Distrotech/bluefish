#include <libxml/xmlreader.h>

#include "bftextview2.h"
#include "bftextview2_langmgr.h"
#include "bftextview2_patcompile.h"

typedef struct {
	GHashTable *patterns;
	GHashTable *contexts;
	GHashTable *setoptions;
	Tscantable *st; /* while not finished */
	Tbflang *bflang;
} Tbflangparsing;

typedef struct {
	GHashTable *bflang_lookup;
	GList *bflang_list;
	GtkTextTagTable *tagtable;
	gboolean load_reference;
} Tlangmgr;

static Tlangmgr langmgr = {NULL,NULL,NULL};

static void skip_to_end_tag(xmlTextReaderPtr reader, int depth) {
	while (xmlTextReaderRead(reader)==1) {
		if (xmlTextReaderNodeType(reader)==XML_READER_TYPE_END_ELEMENT && depth == xmlTextReaderDepth(reader)) {
			DBG_PARSING("found node %s type %d with depth %d\n", xmlTextReaderName(reader),xmlTextReaderNodeType(reader), xmlTextReaderDepth(reader));
			break;
		}
	}
}

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
		if (tmp) {
			*bool = (tmp[0] == '1');
			g_free(tmp);
		}
		return TRUE;
	}
	return FALSE;
}

static void process_header(xmlTextReaderPtr reader, Tbflang *bflang) {
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
		} else if (xmlStrEqual(name,(xmlChar *)"header")) {
			xmlFree(name);
			break;
		}
		xmlFree(name);
	}
}
/* declaration needed for recursive calling */
static gint16 process_scanning_context(xmlTextReaderPtr reader, Tbflangparsing *bfparser, GQueue *contextstack);

static guint16 process_scanning_element(xmlTextReaderPtr reader, Tbflangparsing *bfparser, gint16 context, GQueue *contextstack
		,gchar *ih_autocomplete_append ,gchar *ih_style
		,gboolean ih_case_insens ,gboolean ih_is_regex ,gboolean ih_autocomplete) {
	guint16 matchnum=0;
	gchar *pattern=NULL, *style=NULL, *blockstartelement=NULL, *blockstyle=NULL, *class=NULL, *autocomplete_append=NULL, *autocomplete_string=NULL, *id=NULL;
	gboolean case_insens=FALSE, is_regex=FALSE, starts_block=FALSE, ends_block=FALSE, is_empty, autocomplete=FALSE;
	gint ends_context=0;
	is_empty = xmlTextReaderIsEmptyElement(reader);
	while (xmlTextReaderMoveToNextAttribute(reader)) {
		xmlChar *aname = xmlTextReaderName(reader);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"pattern",&pattern);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"id",&id);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"style",&style);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"blockstyle",&blockstyle);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"blockstartelement",&blockstartelement);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"class",&class);
		set_boolean_if_attribute_name(reader, aname, (xmlChar *)"is_regex", &is_regex);
		set_boolean_if_attribute_name(reader, aname, (xmlChar *)"starts_block", &starts_block);
		set_boolean_if_attribute_name(reader, aname, (xmlChar *)"ends_block", &ends_block);
		set_boolean_if_attribute_name(reader, aname, (xmlChar *)"case_insens", &case_insens);
		set_integer_if_attribute_name(reader, aname, (xmlChar *)"ends_context", &ends_context);
		set_boolean_if_attribute_name(reader, aname, (xmlChar *)"autocomplete", &autocomplete);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"autocomplete_string",&autocomplete_string);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"autocomplete_append",&autocomplete_append);
		xmlFree(aname);
	}
	if (!class || g_hash_table_lookup(bfparser->setoptions,class)) {
		if (pattern && pattern[0]) {
			GtkTextTag *stylet=NULL,*blockstylet=NULL;
			gchar *reference=NULL;
			guint16 blockstartelementum=0, nextcontext=context;
			DBG_PARSING("pattern %s\n",pattern);
			if (ends_context) {
				/* the nth number in the stack */
				nextcontext=-1*ends_context;/*GPOINTER_TO_INT(g_queue_peek_nth(contextstack,ends_context));*/
			
			}
			stylet = langmrg_lookup_style(style?style:ih_style);
			blockstylet = langmrg_lookup_style(blockstyle);
			if (blockstartelement) {
				blockstartelementum = GPOINTER_TO_INT(g_hash_table_lookup(bfparser->patterns, blockstartelement));
				DBG_PARSING("got blockstartelementum %d for blockstartelement %s, ends_block=%d\n",blockstartelementum,blockstartelement,ends_block);
			}
			matchnum = add_keyword_to_scanning_table(bfparser->st, pattern, is_regex?is_regex:ih_is_regex,case_insens?case_insens:ih_case_insens, stylet, context, nextcontext
					, starts_block, ends_block, blockstartelementum,blockstylet,FALSE,NULL, NULL);
			DBG_PARSING("add matchnum %d to hash table for key %s, starts_block=%d\n",matchnum,pattern,starts_block);
			g_hash_table_insert(bfparser->patterns, g_strdup(id?id:pattern), GINT_TO_POINTER((gint)matchnum));
			/* now check if there is a deeper context */
			if (!is_empty) {
				while (xmlTextReaderRead(reader)==1) {
					xmlChar *name=xmlTextReaderName(reader);
					if (xmlStrEqual(name,(xmlChar *)"context")) {
						DBG_PARSING("in pattern, found countext\n");
						nextcontext = process_scanning_context(reader,bfparser,contextstack);
						match_set_nextcontext(bfparser->st, matchnum, nextcontext);
					} else if (xmlStrEqual(name,(xmlChar *)"reference")) {
						DBG_PARSING("in pattern, found reference\n");
						if (!xmlTextReaderIsEmptyElement(reader)) {
							if (langmgr.load_reference)
								reference = (gchar *)xmlTextReaderReadInnerXml(reader);
							DBG_PARSING("reference=%s\n",reference);
							while (xmlTextReaderRead(reader)==1) {
								xmlFree(name);
								name=xmlTextReaderName(reader);
								if (xmlStrEqual(name,(xmlChar *)"reference")) {
									break;
								}
							}
						}
					} else if (xmlStrEqual(name,(xmlChar *)"element")) {
						DBG_PARSING("in pattern, found pattern --> end -of-pattern\n");
						xmlFree(name);
						break;
					} else {
						DBG_PARSING("parsing element with name %s\n",name);
					}
					xmlFree(name);
				}
			}
			match_autocomplete_reference(bfparser->st,autocomplete?autocomplete:ih_autocomplete,autocomplete_string?autocomplete_string:pattern,context,autocomplete_append?autocomplete_append:ih_autocomplete_append,reference);
		}
	}
	/* TODO cleanup! */
	if (pattern) xmlFree(pattern);
	if (id) xmlFree(id);
	if (autocomplete_string) xmlFree(autocomplete_string);
	if (autocomplete_append) xmlFree(autocomplete_append);
	if (style) xmlFree(style);
	if (blockstartelement) xmlFree(blockstartelement);
	if (blockstyle) xmlFree(blockstyle);
	if (class) xmlFree(class);
	return matchnum;
}

static guint16 process_scanning_tag(xmlTextReaderPtr reader, Tbflangparsing *bfparser, guint16 context, GQueue *contextstack
		,gchar *ih_autocomplete_append ,gchar *ih_style,gchar *ih_attrib_autocomplete_append ,gchar *ih_attribstyle) {
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
			stylet = langmrg_lookup_style(style?style:ih_style);
			guint16 contexttag, starttagmatch, endtagmatch;
			gint contextstring;
			gchar *tmp, *reference=NULL;
		
			contexttag = new_context(bfparser->st, ">\"=' \t\n\r", NULL, FALSE);
			tmp = g_strconcat("<",tag,NULL);
			matchnum = add_keyword_to_scanning_table(bfparser->st, tmp, FALSE, FALSE, stylet, context, contexttag, TRUE, FALSE, 0, NULL,FALSE,NULL,NULL);
			g_free(tmp);
			add_keyword_to_scanning_table(bfparser->st, "/>", FALSE, FALSE, stylet, contexttag, -1, FALSE, FALSE, 0, NULL,FALSE,NULL,NULL);
			starttagmatch = add_keyword_to_scanning_table(bfparser->st, ">", FALSE, FALSE, stylet, contexttag, -1, FALSE, FALSE, 0, NULL,FALSE,NULL,NULL);
			if (attributes) {
				gchar **arr, **tmp2;
				GtkTextTag *string, *attrib = langmrg_lookup_style(attribstyle?attribstyle:ih_attribstyle);
				string = langmrg_lookup_style("string");
				arr = g_strsplit(attributes,",",-1);
				tmp2 = arr;
				while (*tmp2) {
					add_keyword_to_scanning_table(bfparser->st, *tmp2, FALSE, FALSE, attrib, contexttag, contexttag, FALSE, FALSE, 0, NULL,TRUE,attrib_autocomplete_append?attrib_autocomplete_append:ih_attrib_autocomplete_append,NULL);
					tmp2++;
				}
				g_strfreev(arr);
				contextstring = GPOINTER_TO_INT(g_hash_table_lookup(bfparser->contexts, "__internal_tag__"));
				if (!contextstring) {
					contextstring = new_context(bfparser->st, "\"=' \t\n\r", string, FALSE);
					add_keyword_to_scanning_table(bfparser->st, "\"", FALSE, FALSE, string, contextstring, -1, FALSE, FALSE, 0, string,FALSE,NULL,NULL);
					g_hash_table_insert(bfparser->contexts, "__internal_tag__", GINT_TO_POINTER(contextstring));
				}
				add_keyword_to_scanning_table(bfparser->st, "\"", FALSE, FALSE, string, contexttag, contextstring, FALSE, FALSE, 0, NULL,FALSE,NULL,NULL);
				
			}
			
			if (!is_empty) {
				while (xmlTextReaderRead(reader)==1) {
					xmlChar *name=xmlTextReaderName(reader);
					if (xmlStrEqual(name,(xmlChar *)"reference")) {
						if (!xmlTextReaderIsEmptyElement(reader)) {
							if (langmgr.load_reference)
								reference = (gchar *)xmlTextReaderReadInnerXml(reader);
							DBG_PARSING("reference=%s\n",reference);
							while (xmlTextReaderRead(reader)==1) {
								xmlFree(name);
								name=xmlTextReaderName(reader);
								if (xmlStrEqual(name,(xmlChar *)"reference")) {
									break;
								}
							}
						}
					} else if (xmlStrEqual(name,(xmlChar *)"context")) {
						innercontext = process_scanning_context(reader,bfparser,contextstack);
						match_set_nextcontext(bfparser->st, starttagmatch, innercontext);
					} else if (xmlStrEqual(name,(xmlChar *)"tag")) {
						xmlFree(name);
						break;
					}
					xmlFree(name);
				}
			}
			tmp = g_strconcat("<",tag,NULL);
			match_autocomplete_reference(bfparser->st,TRUE,tmp,context,autocomplete_append?autocomplete_append:ih_autocomplete_append,reference);
			g_free(tmp);
			tmp = g_strconcat("</",tag,">",NULL);
			endtagmatch = add_keyword_to_scanning_table(bfparser->st, tmp, FALSE, FALSE, stylet, innercontext, (innercontext==context)?context:-2, FALSE, TRUE, matchnum, NULL,TRUE,NULL,NULL);
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

/* ih stands for InHerit */
static void process_scanning_group(xmlTextReaderPtr reader, Tbflangparsing *bfparser, gint context, GQueue *contextstack
		,gchar *ih_autocomplete_append ,gchar *ih_style ,gchar *ih_class ,gchar *ih_attrib_autocomplete_append ,gchar *ih_attribstyle
		,gboolean ih_case_insens ,gboolean ih_is_regex ,gboolean ih_autocomplete) {
	gchar *autocomplete_append=NULL, *style=NULL, *class=NULL, *attrib_autocomplete_append=NULL, *attribstyle=NULL; 
	gboolean case_insens=FALSE, is_regex=FALSE, autocomplete=FALSE;
	gint depth;
	if (xmlTextReaderIsEmptyElement(reader)) {
		return;
	}
	depth = xmlTextReaderDepth(reader);
	while (xmlTextReaderMoveToNextAttribute(reader)) {
		xmlChar *aname = xmlTextReaderName(reader);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"autocomplete_append",&autocomplete_append);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"style",&style);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"class",&class);
		set_boolean_if_attribute_name(reader,aname,(xmlChar *)"case_insens",&case_insens);
		set_boolean_if_attribute_name(reader,aname,(xmlChar *)"is_regex",&is_regex);
		set_boolean_if_attribute_name(reader,aname,(xmlChar *)"autocomplete",&autocomplete);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"attrib_autocomplete_append",&attrib_autocomplete_append);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"attribstyle",&attribstyle);
		xmlFree(aname);
	}
	
	if (class && GPOINTER_TO_INT(g_hash_table_lookup(bfparser->setoptions,class))!=1){
		DBG_PARSING("group disabled, class=%s, skip to end of group, my depth=%d\n",class,depth);
		skip_to_end_tag(reader, depth);
	} else {
		while (xmlTextReaderRead(reader)==1) {
			xmlChar *name = xmlTextReaderName(reader);
			if (xmlStrEqual(name,(xmlChar *)"element")) {
				process_scanning_element(reader,bfparser,context,contextstack
						,autocomplete_append?autocomplete_append:ih_autocomplete_append
						,style?style:ih_style
						,case_insens?case_insens:ih_case_insens
						,is_regex?is_regex:ih_is_regex
						,autocomplete?autocomplete:ih_autocomplete
						);
			} else if (xmlStrEqual(name,(xmlChar *)"tag")) {
				process_scanning_tag(reader,bfparser,context,contextstack
						,autocomplete_append?autocomplete_append:ih_autocomplete_append
						,style?style:ih_style 
						,attrib_autocomplete_append?attrib_autocomplete_append:ih_attrib_autocomplete_append 
						,attribstyle?attribstyle:ih_attribstyle
						);
			} else if (xmlStrEqual(name,(xmlChar *)"group")) {
				if (xmlTextReaderDepth(reader)==depth) {
					DBG_PARSING("end-of-group\n");
					xmlFree(name);
					break;
				} else {
					process_scanning_group(reader, bfparser, context, contextstack
							,autocomplete_append?autocomplete_append:ih_autocomplete_append
							,style?style:ih_style
							,class?class:ih_class
							,attrib_autocomplete_append?attrib_autocomplete_append:ih_attrib_autocomplete_append
							,attribstyle?attribstyle:ih_attribstyle
							,case_insens?case_insens:ih_case_insens
							,is_regex?is_regex:ih_is_regex
							,autocomplete?autocomplete:ih_autocomplete
							 );
				}
			} else
				DBG_PARSING("found %s\n",name);
			xmlFree(name);
		}
	}
	g_free(autocomplete_append);
	g_free(style);
	g_free(class);
	g_free(attrib_autocomplete_append);
	g_free(attribstyle);
}

static gint16 process_scanning_context(xmlTextReaderPtr reader, Tbflangparsing *bfparser, GQueue *contextstack) {
	gchar *symbols=NULL, *style=NULL, *name=NULL;
	gboolean autocomplete_case_insens=FALSE,is_empty;
	gint context;
	is_empty = xmlTextReaderIsEmptyElement(reader);
	while (xmlTextReaderMoveToNextAttribute(reader)) {
		xmlChar *aname = xmlTextReaderName(reader);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"id",&name);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"symbols",&symbols);
		set_string_if_attribute_name(reader,aname,(xmlChar *)"style",&style);
		set_boolean_if_attribute_name(reader,aname,(xmlChar *)"autocomplete_case_insens",&autocomplete_case_insens);
		xmlFree(aname);
	}
	if (is_empty && name && !symbols && !style && !autocomplete_case_insens) {
		g_print("lookup context %s in hash table..\n",name);
		return GPOINTER_TO_INT(g_hash_table_lookup(bfparser->contexts, name));
	}	
	/* create context */
	DBG_PARSING("create context symbols %s and style %s\n",symbols,style);
	context = new_context(bfparser->st,symbols,langmrg_lookup_style(style),autocomplete_case_insens);
	g_queue_push_head(contextstack,GINT_TO_POINTER(context)); 
	if (name) {
		g_print("insert context %s into hash table as %d\n",name,context);
		g_hash_table_insert(bfparser->contexts, g_strdup(name), GINT_TO_POINTER(context));
	}
	g_free(name);
	g_free(symbols);
	g_free(style);
	/* now get the children */
	while (xmlTextReaderRead(reader)==1) {
		xmlChar *name = xmlTextReaderName(reader);
		DBG_PARSING("parsing context, found node name %s\n",name);
		if (xmlStrEqual(name,(xmlChar *)"element")) {
			process_scanning_element(reader,bfparser,context,contextstack,NULL,NULL,FALSE,FALSE,FALSE);
		} else if (xmlStrEqual(name,(xmlChar *)"tag")) {
			process_scanning_tag(reader,bfparser,context,contextstack,NULL,NULL,NULL,NULL);
		} else if (xmlStrEqual(name,(xmlChar *)"group")) {
			g_print("parsing context, group found\n");
			process_scanning_group(reader,bfparser,context,contextstack,NULL,NULL,NULL,NULL,NULL,FALSE,FALSE,FALSE);
		} else if (xmlStrEqual(name,(xmlChar *)"context")) {
			xmlFree(name);
			DBG_PARSING("parsing context, end-of-context, return context %d\n",context);
			g_queue_pop_head(contextstack); 
			return context;
		} else
			DBG_PARSING("parsing context, found %s\n",name);
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
	bfparser->contexts =  g_hash_table_new_full(g_str_hash,g_str_equal,g_free,NULL);
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
		if (xmlStrEqual(name,(xmlChar *)"header")) {
			/* actually we can skip detection */
			DBG_PARSING("processing <detection>\n");
			process_header(reader,bflang);
		} else if (xmlStrEqual(name,(xmlChar *)"definition")) {
			DBG_PARSING("processing <scanning>\n");
			while (xmlTextReaderRead(reader)==1) {
				xmlChar *name2 = xmlTextReaderName(reader);
				if (xmlStrEqual(name2,(xmlChar *)"context")) {
					GQueue *contextstack = g_queue_new();
					process_scanning_context(reader,bfparser,contextstack);
				} else if (xmlStrEqual(name2,(xmlChar *)"definition")) {
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
		g_print("no scantable in %p, start thread\n",bflang);
		thread = g_thread_create(build_lang_thread,bflang,FALSE,&error);
		if (error) {
			DBG_PARSING("start thread, error\n");
		}
	} else {
		g_print("have scantable, return %p\n",bflang);
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
			if (xmlStrEqual(name,(xmlChar *)"header")) {
				bflang = g_slice_new0(Tbflang);
				bflang->filename = g_strdup(filename);
				process_header(reader,bflang);
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

void langmgr_init(gboolean load_reference) {
	Tbflang *bflang;
	langmgr.tagtable = gtk_text_tag_table_new();
	langmgr.bflang_lookup = g_hash_table_new(g_str_hash,g_str_equal);
	langmgr.load_reference = load_reference;
	
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
	bflang = parse_bflang2_header("css.bflang2");
	register_bflanguage(bflang);
	bflang = parse_bflang2_header("javascript.bflang2");
	register_bflanguage(bflang);
	bflang = parse_bflang2_header("java.bflang2");
	register_bflanguage(bflang);
	bflang = parse_bflang2_header("jsp.bflang2");
	register_bflanguage(bflang);
	bflang = parse_bflang2_header("po.bflang2");
	register_bflanguage(bflang);
	bflang = parse_bflang2_header("shell.bflang2");
	register_bflanguage(bflang);
	bflang = parse_bflang2_header("ruby.bflang2");
	register_bflanguage(bflang);
	DBG_PARSING("langmgr_init, returning \n");
}
