#include <libxml/xmlreader.h>

#include "bftextview2.h"
#include "bftextview2_patcompile.h"

typedef struct {
	gchar *name;
	GList *mimetypes;
	GList *extensions;
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
		g_print("%s ",tmplist->data);
		tmplist=g_list_next(tmplist);
	}
	g_print("\n");
	return FALSE;
}

static void process_detection(xmlTextReaderPtr reader, Tbflang *bflang) {
	xmlChar *name,value;
	gint ret;
	ret = xmlTextReaderRead(reader);
	name = xmlTextReaderName(reader);
	while (ret && name && !xmlStrEqual(name,"detection")) {
		/*name = xmlTextReaderName(reader);*/
		if (xmlStrEqual(name,"mime")) {
			ret = xmlTextReaderRead(reader);
			if (ret) {
				name = xmlTextReaderName(reader);
				g_print("name=%s\n",name);
				value = xmlTextReaderValue(reader);
				g_print("value=%s\n",value);
				if (value) {
					bflang->mimetypes = g_list_prepend(bflang->mimetypes, g_strdup(value));
					xmlFree(value);
				}
			}
		} else if (xmlStrEqual(name,"extension")) {
			ret = xmlTextReaderRead(reader);
/*			if (value)
				bflang->extensions = g_list_prepend(bflang->extensions, g_strdup(value));*/
		}
		xmlFree(name);
		name=NULL;
		ret = xmlTextReaderRead(reader);
		if (ret)
			name = xmlTextReaderName(reader);
	}
	
}

static void processNode(xmlTextReaderPtr reader, Tbflang *bflang)
{
	/* handling of a node in the tree */
	xmlChar *name, *value;
	gint ret;
	name = xmlTextReaderName(reader);
	if (name == NULL)
		name = xmlStrdup(BAD_CAST "--");
	
	if (xmlStrEqual(name,"detection")) {
		ret = xmlTextReaderRead(reader);
		process_detection(reader,bflang);
	} else {
		value = xmlTextReaderValue(reader);
		g_print("%d %d %s %d", xmlTextReaderDepth(reader), xmlTextReaderNodeType(reader), name,
			   xmlTextReaderIsEmptyElement(reader));
	
		if (value == NULL)
			g_print("\n");
		else {
			g_print(" %s\n", value);
			xmlFree(value);
		}
	}
	xmlFree(name);

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
			processNode(reader,bflang);
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
