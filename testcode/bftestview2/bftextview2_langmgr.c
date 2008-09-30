#include <libxml/xmlreader.h>

#include "bftextview2.h"
#include "bftextview2_patcompile.h"

typedef struct {
	gchar *name;
	GList *mimetypes;
	gchar *filename;
	Tscantable *scantable;
} Tbflang;

/* this is called in the mainloop again */
static gboolean build_lang_finished_lcb(gpointer data)
{
	Tbflang *bflang=data;
	GList *tmplist;
	g_print("build_lang_finished_lcb, mimetypes= ");
	tmplist=g_list_first(bflang->mimetypes);
	while (tmplist) {
		g_print("%s ",(gchar *)tmplist->data);
		tmplist=g_list_next(tmplist);
	}
	g_print("\n");
	return FALSE;
}

static gchar *get_atrribute_value_if_name(xmlTextReaderPtr reader, gchar *aname, gchar *searchname) {
	xmlChar *avalue=NULL;
	if (xmlStrEqual(aname,searchname)) {
		avalue = xmlTextReaderValue(reader);
	}
	return avalue;
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
				gchar mimetype;
				xmlChar *aname = xmlTextReaderName(reader);
				mimetype = get_atrribute_value_if_name(reader,aname,"type");
				if (mimetype)
					bflang->mimetypes = g_list_prepend(bflang->mimetypes, mimetype);
				xmlFree(aname);
			}
			ret = xmlTextReaderRead(reader);
		}
		xmlFree(name);
		name=NULL;
		ret = xmlTextReaderRead(reader);
		if (ret)
			name = xmlTextReaderName(reader);
	}
}
static void process_scanning_regex(xmlTextReaderPtr reader, Tbflang *bflang, guint context) {

}
static void process_scanning_pattern(xmlTextReaderPtr reader, Tbflang *bflang, guint context) {

}
static void process_scanning_keyword(xmlTextReaderPtr reader, Tbflang *bflang, guint context) {

}
static void process_scanning_context(xmlTextReaderPtr reader, Tbflang *bflang, guint prevcontext) {
	gchar *symbols=NULL, *style=NULL;
	guint context;
	gint ret;
	while (xmlTextReaderMoveToNextAttribute(reader)) {
		xmlChar *aname = xmlTextReaderName(reader);
		symbols = get_atrribute_value_if_name(reader,aname,"symbols");
		style = get_atrribute_value_if_name(reader,aname,"style");
		xmlFree(aname);
	}
	/* create context */
	/*context = new_context(symbols,style);*/
	/* now get the children */
	ret = xmlTextReaderRead(reader);
	while (ret == 1) {
		xmlChar *name;
		name = xmlTextReaderName(reader);
		if (xmlStrEqual(name,"regex")) {
			process_scanning_regex(reader,bflang,context);
		} else if (xmlStrEqual(name,"pattern")) {
			process_scanning_pattern(reader,bflang,context);
		} else if (xmlStrEqual(name,"keyword")) {
			process_scanning_keyword(reader,bflang,context);
		}
		xmlFree(name);
		ret = xmlTextReaderRead(reader);
	}
}

static gpointer build_lang_thread(gpointer data)
{
	xmlTextReaderPtr reader;
	gint ret;
	gchar *filename = data;
	Tbflang *bflang;
	
	bflang = g_slice_new0(Tbflang);
	g_print("build_lang_thread %p, started for %s\n",g_thread_self(),filename);
	bflang->filename = g_strdup(filename);
	reader = xmlNewTextReaderFilename(filename);
	xmlTextReaderSetParserProp(reader,XML_PARSER_SUBST_ENTITIES,TRUE);
	if (reader != NULL) {
		ret = xmlTextReaderRead(reader);
		while (ret == 1) {
			xmlChar *name;
			name = xmlTextReaderName(reader);
			if (xmlStrEqual(name,"detection")) {
				process_detection(reader,bflang);
			} else if (xmlStrEqual(name,"scanning")) {
				/* next node should be main context */
				ret = xmlTextReaderRead(reader);
				if (ret) {
					xmlChar *name2;
					name2 = xmlTextReaderName(reader);
					if (xmlStrEqual(name2,"context")) {
						process_scanning_context(reader,bflang,0);
					}
					xmlFree(name2);
				}
			}
			xmlFree(name);
			ret = xmlTextReaderRead(reader);
		}
		xmlFreeTextReader(reader);
		if (ret != 0) {
			g_print("%s : failed to parse\n", filename);
		}
	} else {
		g_print("Unable to open %s\n", filename);
	}


	g_print("build_lang_thread %p, finished\n",g_thread_self());
	/* when done call mainloop */
	g_idle_add(build_lang_finished_lcb, bflang);
	return bflang;
}

static void start_langbuilder_thread(gchar *filename)
{
	GError *error=NULL;
	GThread* thread;

	g_print("new thread from main thread %p\n",g_thread_self());	
	thread = g_thread_create(build_lang_thread,filename,FALSE,&error);
	if (error) {
		g_print("start_langbuilder_thread, error for filename %s\n",filename);
	}
}


void langmgr_init()
{
	
	start_langbuilder_thread("c.bflang2");
	g_print("langmgr_init, returning \n");
}
