/* Bluefish HTML Editor
 *     fref2.c
 *     Library aka Function reference - second implementation
 * 
 *    Copyright (C) 2005 Oskar Świda swida@aragorn.pb.bialystok.pl
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <gtk/gtk.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xmlwriter.h>

#include "bluefish.h"
#include "fref.h"
#include "rcfile.h"				/* array_from_arglist() */
#include "stringlist.h"
#include "document.h"
#include "bf_lib.h"
#include "gtk_easy.h"
#include "char_table.h"
#include "pixmap.h"

enum {
	STR_COLUMN,
	PTR_COLUMN,
	FILE_COLUMN,
	N_COLUMNS
};

enum {
FR_INFO_DESC	,
FR_INFO_ATTRS,
FR_INFO_NOTES,
FR_INFO_EXAMPLES,
FR_INFO_LINKS
};


typedef struct {
	GtkTreeStore *store;
	GHashTable *refcount;		/* Opened reference count */
} Tfref_data;
#define FREFDATA(var) ((Tfref_data *)(var))


typedef struct {
	GtkWidget *tree;
	GtkTooltips *argtips;
	GtkWidget *infocheck;
	GtkWidget *infoview;
	GtkWidget *infoscroll;
	GtkWidget *menu_show,*menu_pref,*menu_edit/*,*menu_insert*/;
	GtkWidget *m_descr,*m_ap,*m_note,*m_example,*m_link;
/*	GtkWidget *m_1,*m_2;*/
	GtkWidget *sentry;
	GtkWidget *btn_show,*btn_pref,*btn_edit;
	GdkColor col_descr,col_ap,col_note,col_example,col_link;
	GtkTextTag *tag_descr,*tag_ap,*tag_note,*tag_example,*tag_link;
	Tbfwin *bfwin;
	guint tree_signal, row_collapse_signal, row_expand_signal;
	gpointer prg; /* progress dialog */
} Tfref_gui;
#define FREFGUI(var) ((Tfref_gui *)(var))

/*
*     NOTE: all params,attrs, examples etc. are inserted into common structure - whether they are defined
*               by <def> or <property> - it simplifies freeing memory.
*/

typedef struct {
	GHashTable *dictionary;
	GHashTable *commons;
	guchar *description;
} Tfref_info;
#define FREFINFO(var) ((Tfref_info *)(var))


enum {
	FREF_EL_FUNCTION,
	FREF_EL_TAG,
	FREF_EL_CSSPROP,
	FREF_EL_CSSSELECT,
	FREF_EL_CLASS,
	FREF_EL_NOTE,
	FREF_EL_ATTR,
	FREF_EL_PARAM,
	FREF_EL_RETURN,
	FREF_EL_DESCR,
	FREF_EL_EXAMPLE,
	FREF_EL_VALUES,
	FREF_EL_VAR,
	FREF_EL_SNIPPET,
	FREF_EL_GROUP,
	FREF_EL_REF,
	FREF_EL_PROP,
	FREF_EL_LINK,
	FREF_EL_DEPEND,
	FREF_EL_PARENTS,
	FREF_EL_NUMCOLS
};

enum {FID_ATTRIBUTE,FID_CLASS,FID_CSSPROP,FID_CHILDREN,FID_DESCRIPTION,
FID_DEPENDENCY,FID_DEFAULT,FID_DEF,FID_DEFTYPE,FID_ELEMENT,FID_EXAMPLE,
FID_FUNCTION,FID_GROUP,FID_INSERT,FID_ID,FID_KIND,FID_LINK,FID_NAME,FID_NOTE,
FID_PROPERTIES,FID_PROPERTY,FID_PARAMETER,FID_PARENTS,FID_REF,FID_RETURN,
FID_REQUIRED,FID_SNIPPET,FID_TITLE,FID_TAG,FID_TYPE,FID_VLISTONLY,FID_VALUES,FID_VERSION,
FID_COMPACT,FID_CSSSELECT};

static char *fref_compact[] = {"a","c","c1","c2","d","d1","d2","d3","d4","e","e1","f","g","i","i1","k","l",
													 "n","n1","p","p1","p2","p3","r","r1","r2",	"s","t","t1","t2","v","v1","v2","x","c3"};
static char *fref_full[] = {"attribute","class","css_property","children","description","dependency", 
"default","def","deftype","element","example","function","group","insert","id","kind","link","name","note","properties",
"property","parameter","parents","ref","return","required","snippet","title","tag","type","vlistonly","values","version",
"compact","css_selector"};

char **fref_names=fref_full;
gint fref_node_count=0,fref_node_current=0;

typedef struct {
	gint etype;
	gpointer data;
} Tfref_record;
#define FREFRECORD(var) ((Tfref_record *)(var))

typedef struct {
	guchar *title;
	guchar *text;
} Tfref_note;
#define FREFNOTE(var) ((Tfref_note *)(var))

/* structure for property */
typedef struct {
	gint ptype;				/* property type whether it is attribute/param/link/example etc... */
	guchar *name;
	guchar *id;		/* needed for references */
	guchar *type;      	/* this is not for markup, but usable for classes/return/params */
	gboolean required;
	gboolean vlistonly;
	gpointer vlist;   /* here we have pointer not values, because it can be common defined list of values */
	gpointer vlist2; /* additional list of values */
	guchar *def;       /* default value */
	gpointer description;
	GtkWidget *dlgitem; /* widget for data retrieve */
} Tfref_property;
#define FREFPROPERTY(var) ((Tfref_property *)(var))


typedef struct {
	gint etype;								/* type of element */
	guchar *name;   						
	gpointer description; 
	GList *properties;				 /* attributes/params/variables/fields */
	GList *notes;
	guchar *insert;
	gpointer parent;		/* parent element when this element is subelement */
	GList *children;					/* list of subelements */
} Tfref_element;
#define FREFELEMENT(var) ((Tfref_element *)(var))
    

guchar *tmps;
static void fill_toplevels(Tfref_data * fdata, gboolean empty_first);
static void frefcb_cursor_changed(GtkTreeView * treeview, Tbfwin * bfwin);

/* *********************************************************************
*****************************     PARSING  *******************************
***********************************************************************/

static GRand *fref_rand; /* for unique names */

static gchar *fref_unique_name(gint pfx) {
	gchar *ret;	
	ret = g_strdup_printf("___%d_%d__",pfx,g_rand_int(fref_rand));
	return ret;
}

static Tfref_record *fref_insert_into_commons(Tfref_info *info,gchar *name,gint tp,gpointer data) {
	Tfref_record *rec = g_new0(Tfref_record,1);
	gpointer ptr;
	rec->etype = tp;
	rec->data = data;
	ptr = g_hash_table_lookup(info->commons,name);
	if (ptr) {
		g_warning("Entity '%s' already exists!",name);
		g_free(rec);
		return NULL;
	}
	g_hash_table_insert(info->commons,name,rec);
	return rec;
}



Tfref_property *fref_parse_property(xmlNodePtr node,xmlDocPtr doc,Tfref_info *info,gboolean common) {
		Tfref_property  *p = g_new0(Tfref_property,1);  
		guchar *auxs2,*auxs3;  
		xmlNodePtr auxn;
		gpointer auxp;

		if ( common) {
			auxs2 = xmlGetProp(node,fref_names[FID_ID]);
			if (auxs2 == NULL) {
					g_warning("Reusable property has to have id attribute!");
					g_free(p);
					return NULL;
			}		
			p->id = auxs2;
			p->name = xmlGetProp(node,fref_names[FID_NAME]);
			if (p->name==NULL)
			    p->name = p->id;
		}
		else {
			p->name = xmlGetProp(node,fref_names[FID_NAME]);
		}

		auxs2 = xmlGetProp(node,fref_names[FID_KIND]);
		if ( auxs2 == NULL ) {
			g_warning("Kind of property not determined ");
			g_free(p);
			return NULL;
		}	
		if (xmlStrcmp(auxs2,fref_names[FID_LINK]) == 0)
			p->ptype = FREF_EL_LINK;
		else if (xmlStrcmp(auxs2,fref_names[FID_EXAMPLE]) == 0)	
			p->ptype = FREF_EL_EXAMPLE;		
		else if (xmlStrcmp(auxs2,fref_names[FID_DEPENDENCY]) == 0)	
			p->ptype = FREF_EL_DEPEND;		
		else if (xmlStrcmp(auxs2,fref_names[FID_ATTRIBUTE]) == 0)	
			p->ptype = FREF_EL_ATTR;		
		else if (xmlStrcmp(auxs2,fref_names[FID_PARAMETER]) == 0)	
			p->ptype = FREF_EL_PARAM;		
		else if (xmlStrcmp(auxs2,fref_names[FID_RETURN]) == 0)	
			p->ptype = FREF_EL_RETURN;		
		else if (xmlStrcmp(auxs2,fref_names[FID_PARENTS]) == 0)	
			p->ptype = FREF_EL_PARENTS;		
		else {
			g_warning("Uknown property kind ");
			xmlFree(auxs2);
			g_free(p);
			return NULL;
		}	
		xmlFree(auxs2);
		
		p->type = xmlGetProp(node,fref_names[FID_TYPE]);
		
		auxs2 = xmlGetProp(node,fref_names[FID_REQUIRED]);		
		if ( auxs2 && (xmlStrcmp(auxs2,"1") == 0)) 
			p->required = TRUE;
		else
			p->required = FALSE;	
			
		if (auxs2) xmlFree(auxs2);	
		
		auxs2 = xmlGetProp(node,fref_names[FID_VLISTONLY]);		
		if (auxs2 && (xmlStrcmp(auxs2,"1") == 0))
			p->vlistonly = TRUE;
		else
			p->vlistonly = FALSE;	
		if (auxs2) 	xmlFree(auxs2);
			
		p->def = xmlGetProp(node,fref_names[FID_DEFAULT]);
		
		auxn = node->xmlChildrenNode;		
		while ( auxn ) {
			if ( xmlStrcmp(auxn->name,fref_names[FID_VALUES]) == 0 ) {
						auxs3 = xmlGetProp(auxn,fref_names[FID_REF]);
						if ( auxs3 != NULL ) {
							auxp = g_hash_table_lookup(info->commons,auxs3);
							if ( auxp == NULL ) {
								g_warning("List of values referenced (%s) but not found",auxs3);
								xmlFree(auxs3);
								g_free(p);
								return NULL;
						   }
						   xmlFree(auxs3);
						   auxs3 = xmlNodeListGetString(doc,auxn->xmlChildrenNode,1);
						   if (auxs3)
						   	p->vlist2 = g_strstrip(auxs3);						   	
						   p->vlist = FREFRECORD(auxp)->data;
						   
			        } else {
			        			auxs2 = fref_unique_name(xmlGetLineNo(auxn));
						      Tfref_record *rec = fref_insert_into_commons(info, auxs2,FREF_EL_VALUES,g_strstrip(xmlNodeListGetString(doc,auxn->xmlChildrenNode,1)));
						      if (rec) {
						      	p->vlist = rec->data;
						      }	
						      /*g_free(auxs2);*/
			        }
			}        
			else if ( xmlStrcmp(auxn->name,fref_names[FID_DESCRIPTION]) == 0 ) {
						auxs3 = xmlGetProp(auxn,fref_names[FID_REF]);
						if ( auxs3 != NULL ) {
							auxp = g_hash_table_lookup(info->commons,auxs3);
							if ( auxp == NULL ) {
								g_warning("Reusable description requested (%s) but not found",auxs3);
								xmlFree(auxs3);
								g_free(p);
								return NULL;
						   }
						   p->description = FREFRECORD(auxp)->data;
						   xmlFree(auxs3);
			        } else {
			        			auxs2 = fref_unique_name(xmlGetLineNo(auxn));
						      Tfref_record *rr = fref_insert_into_commons(info, auxs2,FREF_EL_DESCR,xmlNodeListGetString(doc,auxn->xmlChildrenNode,1));
						      if ( rr ) {
							      p->description = rr->data;
							   }   
						      /* g_free(auxs2); */
			        }
			}        
			auxn = auxn->next;
		}	/* while */			
		return p;		
}  /* parse_property */


Tfref_element *fref_parse_element(xmlNodePtr node,xmlDocPtr doc,Tfref_info *info,Tbfwin *bfwin) {
	Tfref_element *p = g_new0(Tfref_element,1);
	xmlNodePtr auxn;
	guchar *auxs;
	gpointer auxp;

	auxs = xmlGetProp(node,fref_names[FID_REF]);
	if ( auxs ) {
		auxp = g_hash_table_lookup(info->commons,auxs);
		if ( auxp == NULL ) {
			g_warning("Previously defined element %s requested but not found",auxs);
			xmlFree(auxs);
			g_free(p);
			return NULL;
		}  else  {
			xmlFree(auxs);
	 	   return FREFELEMENT(FREFRECORD(auxp)->data);
		}    
	}
	
	p->name = xmlGetProp(node,fref_names[FID_NAME]);
	
	if ( p->name == NULL) {
		g_warning("Element has to have a name!");
		g_free(p);
		return NULL;
	}

	auxs = xmlGetProp(node,fref_names[FID_KIND]);
	if ( auxs == NULL ) {
			g_warning("Kind of element not determined ");
			g_free(p);
			return NULL;
	}	
	
	if (xmlStrcmp(auxs,fref_names[FID_FUNCTION]) == 0)
			p->etype = FREF_EL_FUNCTION;
		else if (xmlStrcmp(auxs,fref_names[FID_TAG]) == 0)	
			p->etype = FREF_EL_TAG;		
		else if (xmlStrcmp(auxs,fref_names[FID_CLASS]) == 0)	
			p->etype = FREF_EL_CLASS;		
		else if (xmlStrcmp(auxs,fref_names[FID_CSSPROP]) == 0)	
			p->etype = FREF_EL_CSSPROP;		
		else if (xmlStrcmp(auxs,fref_names[FID_CSSSELECT]) == 0)	
			p->etype = FREF_EL_CSSSELECT;					
		else if (xmlStrcmp(auxs,fref_names[FID_SNIPPET]) == 0)	
			p->etype = FREF_EL_SNIPPET;		
		else {
			g_warning("Uknown element kind ");
			xmlFree(auxs);
			g_free(p);
			return NULL;
		}	
	xmlFree(auxs);
	
	auxn = node->xmlChildrenNode;
	while ( auxn ) {
		if ( xmlStrcmp(auxn->name,fref_names[FID_DESCRIPTION]) == 0) { /* description */
			auxs = xmlGetProp(auxn,fref_names[FID_REF]);
			if ( auxs ) {
				auxp = g_hash_table_lookup(info->commons,auxs);
				if ( auxp == NULL ) {
					g_warning("Reusable description requested (%s) but not found",auxs);
					xmlFree(auxs);
					g_free(p);
					return NULL;
				}
				p->description = FREFRECORD(auxp)->data;
				xmlFree(auxs);
			} else {
				auxs = fref_unique_name(xmlGetLineNo(auxn));
				Tfref_record *rec = fref_insert_into_commons(info,auxs,FREF_EL_DESCR,xmlNodeListGetString(doc,auxn->xmlChildrenNode,1));
				if ( rec ) {
					p->description = rec->data;
				}	
				/* g_free(auxs);*/
			}
		} /* description */
		else if ( xmlStrcmp(auxn->name,fref_names[FID_PROPERTIES]) == 0) { /* properties */
				xmlNodePtr nptr = auxn->xmlChildrenNode;
				Tfref_property *apv; 				
				while ( nptr  ) {
				 if (xmlStrcmp(nptr->name,fref_names[FID_PROPERTY])==0) 
				 {
							Tfref_record *rec=NULL;
							auxs = xmlGetProp(nptr,fref_names[FID_REF]);
							if ( auxs ) {
								auxp = g_hash_table_lookup(info->commons,auxs);
								if ( auxp == NULL ) {
									g_warning("Reusable property requested (%s) but not found",auxs);
									xmlFree(auxs);
									g_free(p);
									return NULL;
								}
								rec = auxp;
								apv = FREFRECORD(auxp)->data;
								xmlFree(auxs);
							} else {
												apv = fref_parse_property(nptr,doc,info,FALSE);
												if ( apv ) {
													auxs = fref_unique_name(xmlGetLineNo(nptr));
													rec = fref_insert_into_commons(info,auxs,apv->ptype,apv);
												}	
							}		
							if ( apv && rec )
								p->properties = g_list_append(p->properties,apv);			
				 }		/* is property */
					nptr = nptr->next;
				}
		} /* properties */
		else if ( xmlStrcmp(auxn->name,fref_names[FID_CHILDREN]) == 0) { /* children */
				xmlNodePtr nptr = auxn->xmlChildrenNode;
				Tfref_element *ftpc; 				
				while ( nptr  ) {
				 if (xmlStrcmp(nptr->name,fref_names[FID_ELEMENT])==0) 
				 {
							ftpc = fref_parse_element(nptr,doc,info,bfwin);
							if ( ftpc ) {
														p->children = g_list_append(p->children,ftpc);
														auxs = fref_unique_name(xmlGetLineNo(nptr));
														if ( fref_insert_into_commons(info,auxs ,ftpc->etype,ftpc) == NULL)
														{
															g_free(ftpc); /* not proper, should free whole element - improve! */
														}	
														fref_node_current++;
														progress_set(FREFGUI(bfwin->fref)->prg,fref_node_current);
														flush_queue();
												}	
				 }		/* is element */
					nptr = nptr->next;
				}				
		} /* children */		
		else if ( xmlStrcmp(auxn->name,fref_names[FID_NOTE]) == 0) { /* note */
			Tfref_note *note = g_new0(Tfref_note,1);
			note->title = xmlGetProp(auxn,fref_names[FID_TITLE]);
			note->text = xmlNodeListGetString(doc,auxn->xmlChildrenNode,1);
			p->notes = g_list_append(p->notes,note);
			auxs = fref_unique_name(xmlGetLineNo(auxn));
			fref_insert_into_commons(info,auxs,FREF_EL_NOTE,note);			
		} /* note */
		else if ( xmlStrcmp(auxn->name,fref_names[FID_INSERT]) == 0) { /* insert */
				p->insert = xmlNodeListGetString(doc,auxn->xmlChildrenNode,1);
		} /* insert */
		auxn = auxn->next;
	}
	return p;
}


static void fref_parse_node(xmlNodePtr node,GtkWidget * tree, GtkTreeStore * store,
   	              						  GtkTreeIter * parent, Tfref_info *info, xmlDocPtr doc,Tbfwin *bfwin) {
   	              						  
	GtkTreeIter auxit;
	GtkTreePath *path;
	xmlNodePtr auxn;
	guchar *auxs,*auxs2;
	gpointer auxp;
	
	   	              						  
	if (!node) return;
	if ( xmlStrcmp(node->name,fref_names[FID_GROUP]) == 0 ) { /* group */
		Tfref_record *el;
		Tfref_note *n = g_new0(Tfref_note,1);
		
		auxs = fref_unique_name(xmlGetLineNo(node));
		el = fref_insert_into_commons(info,auxs,FREF_EL_GROUP,n);	
		if ( el == NULL ) {
			g_free(auxs);
			g_free(n);
			return;
		}
		n->title = xmlGetProp(node,fref_names[FID_NAME]);
		
		auxn = node->xmlChildrenNode;
		while ( auxn ) {
			if ( xmlStrcmp(auxn->name,fref_names[FID_DESCRIPTION]) == 0 ) {
				auxs = xmlGetProp(auxn,fref_names[FID_REF]);
				if ( auxs ) {
					auxp = g_hash_table_lookup(info->commons,auxs);
					if ( auxp == NULL ) {
						g_warning("Reusable description requested (%s) but not found",auxs);
						xmlFree(auxs);
						g_free(el);
						g_free(n);
						return;
					}
					xmlFree(auxs);
					n->text = FREFRECORD(auxp)->data;
				} else {
					auxs = fref_unique_name(xmlGetLineNo(auxn));
					Tfref_record *rec = fref_insert_into_commons(info,auxs,FREF_EL_DESCR,xmlNodeListGetString(doc,auxn->xmlChildrenNode,1));
					if ( rec ) {
						n->text = rec->data;
					}
					/*g_free(auxs);*/
				}
				break;			
			}
			auxn = auxn->next;
		}
		gtk_tree_store_append(store, &auxit, parent);
		gtk_tree_store_set(store, &auxit, STR_COLUMN,n->title, FILE_COLUMN, NULL, PTR_COLUMN, el, -1);
		
		auxn = node->xmlChildrenNode;
		
		while (auxn!=NULL ) {
			fref_parse_node(auxn,tree,store,&auxit,info,doc,bfwin);
			auxn = auxn->next;
		}
	} /* group */   	              						  
	else 	if ( xmlStrcmp(node->name,fref_names[FID_NOTE]) == 0 ) { /* global note */
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(store),parent);
		if (gtk_tree_path_get_depth(path) == 1) { 
			Tfref_note *n = g_new0(Tfref_note,1);
			Tfref_record *el;
			n->title = xmlGetProp(node,fref_names[FID_TITLE]);
			n->text = xmlNodeListGetString(doc,node->xmlChildrenNode,1);

			auxs = fref_unique_name(xmlGetLineNo(node));
			el = fref_insert_into_commons(info,auxs,FREF_EL_NOTE,n);
			if ( el ) {
				gtk_tree_store_append(store, &auxit, parent);
				gtk_tree_store_set(store, &auxit, STR_COLUMN,n->title, FILE_COLUMN, NULL, PTR_COLUMN,el, -1);
			} else {
				xmlFree(n->title);
				xmlFree(n->text);
				g_free(n);
			}	
		}
	} /* global note */
	else 	if ( xmlStrcmp(node->name,fref_names[FID_DEF]) == 0 ) { /* definition */
		auxs = xmlGetProp(node,fref_names[FID_ID]);
		if ( auxs == NULL ) {
			g_warning("Definition has to have an id");
			return;
		}
		auxs2 = xmlGetProp(node,fref_names[FID_DEFTYPE]);		
		if ( auxs2 == NULL ) {
			g_warning("Definition type not defined");
			return;
		}
		if ( xmlStrcmp(auxs2,fref_names[FID_PROPERTY]) == 0 ) {			
			Tfref_property *pp = fref_parse_property(node,doc,info,TRUE);
			if ( pp ) {
				if ( fref_insert_into_commons(info,g_strdup(auxs),pp->ptype,pp) == NULL) {
					g_free(pp); /* Improve! */
				}
				xmlFree(auxs);
			}	
		}
		else if ( xmlStrcmp(auxs2,fref_names[FID_DESCRIPTION]) == 0 ) {		
			fref_insert_into_commons(info,g_strdup(auxs),FREF_EL_DESCR,xmlNodeListGetString(doc,node->xmlChildrenNode,1));	
			xmlFree(auxs);
		} 
		else if ( xmlStrcmp(auxs2,fref_names[FID_VALUES]) == 0 ) {		
			fref_insert_into_commons(info,g_strdup(auxs),FREF_EL_VALUES,g_strstrip(xmlNodeListGetString(doc,node->xmlChildrenNode,1)));	
			xmlFree(auxs);
		} 		
		else {
			g_warning("Uknown definition type");
			xmlFree(auxs2);
			return;
		}
		xmlFree(auxs2);
	} /* definition */	
	else 	if ( xmlStrcmp(node->name,fref_names[FID_ELEMENT]) == 0 ) { /* element */
		Tfref_element *el = fref_parse_element(node,doc,info,bfwin);
		Tfref_record *rec; 
		if ( el ) {
							rec = fref_insert_into_commons(info,g_strdup(el->name),el->etype,el);	
							if ( rec ) { 
								gtk_tree_store_append(store, &auxit, parent);
								gtk_tree_store_set(store, &auxit, STR_COLUMN,el->name, FILE_COLUMN, NULL, PTR_COLUMN,rec, -1);			
								path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &auxit);
								GtkTreeRowReference *rref = gtk_tree_row_reference_new(GTK_TREE_MODEL(store), path);
								g_hash_table_replace(info->dictionary, el->name, rref);
								fref_node_current++;
								progress_set(FREFGUI(bfwin->fref)->prg,fref_node_current);
								flush_queue();
							}	else {
								g_free(el); /*Improve!*/
							}
		}
	} /* element */	
	/*else {
		 g_warning("Uknown tag %s at line %d",node->name,xmlGetLineNo(node));
	}*/
}   	              						  

gint fref_count_nodes(xmlDocPtr doc, xmlChar *xpath){
	
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;

	context = xmlXPathNewContext(doc);
	if (context == NULL) {
		g_warning("Error in xmlXPathNewContext\n");
		return 0;
	}
	result = xmlXPathEvalExpression(xpath, context);
	xmlXPathFreeContext(context);
	if (result == NULL) {
		g_warning("Error in xmlXPathEvalExpression\n");
		return 0;
	}
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
		xmlXPathFreeObject(result);
		return 0;
	}
	return result->nodesetval->nodeNr;
}



gchar *fref_load_refname(gchar *filename) {
	xmlDocPtr doc;
	xmlNodePtr cur;
	gchar *ret;
	
	if (filename == NULL)	return NULL;
	doc = xmlParseFile(filename);
	if (doc==NULL) {
		g_warning(_("Cannot load reference file %s\n"),filename);
		return NULL;
	}	
	cur = xmlDocGetRootElement(doc);	
	if ( xmlStrcmp(cur->name, "ref") != 0  && xmlStrcmp(cur->name, "r") != 0) {
		g_warning(_("Bad root element in reference file %s\nShould be <ref>or <r> and found %s"),filename,cur->name);
		xmlFreeDoc(doc);
		return NULL;
	}
	if (xmlStrcmp(cur->name, "ref")==0)
		ret = xmlGetProp(cur,"name");
	else
		ret = xmlGetProp(cur,"n");	
   xmlFreeDoc(doc);
   return ret;
}

static gboolean reference_file_known(gchar * path)
{
	GList *tmplist = g_list_first(main_v->props.reference_files);
	while (tmplist) {
		gchar **arr = tmplist->data;
		if (count_array(arr) == 2 && strcmp(arr[1], path) == 0) {
			return TRUE;
		}
		tmplist = g_list_next(tmplist);
	}
	return FALSE;
}

void fref_rescan_dir(const gchar * dir)
{
	const gchar *filename;
	GError *error = NULL;
	gchar *tofree;
	GPatternSpec *ps = g_pattern_spec_new("funcref_*.xml");
	GDir *gd = g_dir_open(dir, 0, &error);
	filename = g_dir_read_name(gd);
	while (filename) {
		if (g_pattern_match(ps, strlen(filename), filename, NULL)) {
			gchar *path = g_strconcat(dir, filename, NULL);
			DEBUG_MSG("filename %s has a match!\n", filename);
			if (!reference_file_known(path)) {
				tofree = fref_load_refname(path);
				main_v->props.reference_files =
					g_list_append(main_v->props.reference_files,
								  array_from_arglist(g_strdup(tofree), path, NULL));
				g_free(tofree);
			}
			g_free(path);
		}
		filename = g_dir_read_name(gd);
	}
	g_dir_close(gd);
	g_pattern_spec_free(ps);
}


void fref_load_from_file(gchar * filename, GtkWidget * tree, GtkTreeStore * store,
							  GtkTreeIter * parent, Tfref_info *info, Tbfwin *bfwin )
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	xmlChar *tmps;

	if (filename == NULL)
		return;
	info->dictionary =g_hash_table_new_full(g_str_hash, g_str_equal, NULL,
								      (GDestroyNotify) gtk_tree_row_reference_free);
	info->commons = g_hash_table_new(g_str_hash, g_str_equal);
	xmlLineNumbersDefault(1);
	doc = xmlParseFile(filename);
	if (doc==NULL) {
		g_warning(_("Cannot load reference file %s\n"),filename);
		g_hash_table_destroy(info->dictionary);
		g_hash_table_destroy(info->commons);
		return;
	}	
	cur = xmlDocGetRootElement(doc);	
	if ( xmlStrcmp(cur->name, "ref") != 0 &&  xmlStrcmp(cur->name,"r") != 0 ) {
		g_warning(_("Bad root element in reference file %s\nShould be <ref> or <r> and found %s"),filename,cur->name);
		g_hash_table_destroy(info->dictionary);
		g_hash_table_destroy(info->commons);		
		xmlFreeDoc(doc);
		return;
	}
	if (xmlStrcmp(cur->name, "ref") == 0) 
	{
		fref_names = fref_full;
		fref_node_count = fref_count_nodes(doc,"//element");
		fref_node_current = 0;
	}	
	else
	{
		fref_names = fref_compact;
		fref_node_count = fref_count_nodes(doc,"//e");
		fref_node_current = 0;
	}	

	tmps = xmlGetProp(cur,fref_names[FID_VERSION]);
	if (xmlStrcmp(tmps, (const xmlChar *) "2") != 0 ) {
		warning_dialog(bfwin->main_window,_("Bad reference"),
									 _("This is probably old reference file. \nPlease update it to the new format."));
		g_hash_table_destroy(info->dictionary);
		g_hash_table_destroy(info->commons);		
		xmlFree(tmps);
		xmlFreeDoc(doc);
		return;		
	}
	tmps = xmlGetProp(cur,fref_names[FID_DESCRIPTION]);
	if (tmps!=NULL)
		info->description = tmps;
	cur = cur->xmlChildrenNode;
	FREFGUI(bfwin->fref)->prg = progress_popup(bfwin->main_window,_("Loading reference"),fref_node_count);
	flush_queue();
	while (cur != NULL) {
		fref_parse_node(cur,tree,store,parent,info,doc,bfwin);
		cur = cur->next;
	}
	progress_destroy(FREFGUI(bfwin->fref)->prg);
	xmlFreeDoc(doc);

}

void fref_free_record(gpointer key, gpointer value,gpointer udata) {
	Tfref_record *rec;
		
	if ( value == NULL ) return;
	rec = FREFRECORD(value);

	switch ( rec->etype ) {
		case FREF_EL_FUNCTION:
		case FREF_EL_TAG:
		case FREF_EL_CSSPROP:
		case FREF_EL_CSSSELECT:		
		case FREF_EL_CLASS:
		case FREF_EL_SNIPPET:if (rec->data) {
					Tfref_element *el = FREFELEMENT(rec->data);					
					if (el->name)  xmlFree(el->name); 
					g_list_free(el->properties);
					g_list_free(el->notes);
					if (el->insert) xmlFree(el->insert);
					g_list_free(el->children);
					g_free(el);		
		}
		break;
		case FREF_EL_GROUP: if (rec->data) {
				Tfref_note *note = FREFNOTE(rec->data);
				if (note->title) xmlFree(note->title);
				/* text is a description !*/
				g_free(note);
		}	
		break;				
		case FREF_EL_NOTE: if (rec->data) {
				Tfref_note *note = FREFNOTE(rec->data);
				if (note->title) xmlFree(note->title);
				if (note->text) xmlFree(note->text);
				g_free(note);
		}	
		break;
		case FREF_EL_ATTR:
		case FREF_EL_PARAM:
		case FREF_EL_RETURN:
		case FREF_EL_EXAMPLE:
		case FREF_EL_LINK:
		case FREF_EL_DEPEND:
		case FREF_EL_PARENTS:	if (rec->data)	{
			Tfref_property *prop = FREFPROPERTY(rec->data);
			if (prop->name) xmlFree(prop->name);
			if (prop->type) xmlFree(prop->type);
			if (prop->def) xmlFree(prop->def);
			g_free(prop);
		}
		break;
		case FREF_EL_VALUES:
			if (rec->data) g_free(rec->data);	/* because I do g_strstrip */
		break;	
		case FREF_EL_DESCR:
			if (rec->data) xmlFree(rec->data);
		break;
	} /* switch */
	g_free(rec);
}

void fref_free_storeelem(GtkTreeStore *store, GtkTreeIter *position) {
	GtkTreeIter iter;
	
	while (gtk_tree_model_iter_children(GTK_TREE_MODEL(store), &iter, position)) {	
			fref_free_storeelem(store, &iter);		
	}							/* while */
	gtk_tree_store_remove(store, position);
}

void fref_unload_ref(GtkTreeStore *store, GtkTreeIter *iter,Tbfwin *bfwin) {
	Tfref_record *rec=NULL;
	Tfref_info *info=NULL;
	GValue *val;
	GtkTreeIter iter2;
	GtkWidget *winfo,*wlabel;
	
	
		val = g_new0(GValue, 1);
		gtk_tree_model_get_value(GTK_TREE_MODEL(store),iter, PTR_COLUMN, val);
		if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) {
				rec = (Tfref_record *) g_value_peek_pointer(val);
		}
		if ( rec == NULL  || rec->etype != FREF_EL_REF)  {
			return;
		}	
		info = FREFINFO(rec->data);
		/* free commons - there are elements of reference */
		
		fref_node_count = g_hash_table_size(info->commons);
		winfo = window_with_title(_("Unloading reference"), GTK_WIN_POS_CENTER,1);
		wlabel = gtk_label_new(_(" Freeing reference memory "));
		gtk_container_add(GTK_CONTAINER(winfo),wlabel);
		gtk_widget_show_all(winfo);
		flush_queue();
		g_hash_table_foreach(info->commons,fref_free_record,bfwin);
		g_hash_table_destroy(info->commons);		
		while (gtk_tree_model_iter_children(GTK_TREE_MODEL(store), &iter2, iter)) {
				fref_free_storeelem(store, &iter2);
		}	
		
		 if (info->description) xmlFree(info->description);

		g_hash_table_destroy(info->dictionary);
		g_free(info);
		gtk_tree_store_set(store,iter, PTR_COLUMN,NULL,-1);
		g_free(rec);
		g_value_unset(val);
		g_free(val);		
		window_destroy(winfo);
}

void fref_unload_all(GtkWidget * tree, GtkTreeStore * store,Tbfwin *bfwin)
{

	GtkTreeIter iter;
	gint *cnt = NULL;
	gpointer *aux;
	gboolean do_unload = FALSE;

	DEBUG_MSG("fref_loader_unload_all, started for tree=%p, store=%p\n", tree, store);
	while (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter, NULL, 0)) {
		GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
		if (gtk_tree_path_get_depth(path) == 1) {
			gchar *cat;
			gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, STR_COLUMN, &cat, -1);
			DEBUG_MSG("fref_loader_unload_all, cat=%s\n", cat);
			aux = g_hash_table_lookup(FREFDATA(main_v->frefdata)->refcount, cat);
			if (cat) 
				g_free (cat);
				
			if (aux != NULL) {
				cnt = (gint *) aux;
				*cnt = (*cnt) - 1;
				if (*cnt <= 0)
					do_unload = TRUE;
				else
					do_unload = FALSE;
				DEBUG_MSG("fref_loader_unload_all, cnt=%d, do_unload=%d\n", *cnt, do_unload);
			} else
				do_unload = FALSE;				
		} else
			do_unload = FALSE;
		if (do_unload) {
			gchar *file, *name;
			DEBUG_MSG("fref_loader_unload_all, calling fref_loader_unload_ref\n");
			fref_unload_ref(store, &iter,bfwin);
			gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, STR_COLUMN, &name, FILE_COLUMN, &file,
							   -1);
			if (file) {
				DEBUG_MSG("fref_loader_unload_all, freeing %s\n", file);
				g_free(file);
			}
			if (name) {
				DEBUG_MSG("fref_loader_unload_all, freeing %s\n", name);
				g_free(name);
			}
			gtk_tree_store_remove(store, &iter);
		}						/* do_unload */
		if (path)
			gtk_tree_path_free (path);
	}							/* while */
	/* gtk_tree_store_clear(store); */
}

static void fref_save_element(GtkTreeStore *store,GtkTreeIter *iter,xmlDocPtr doc,xmlNodePtr parent)
{
	GValue *val=NULL;
	xmlNodePtr node,txt;
	
	val = g_new0(GValue, 1);	
	gtk_tree_model_get_value(GTK_TREE_MODEL(store),iter,PTR_COLUMN, val);
	if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) 
	{
		Tfref_record *rec = FREFRECORD(g_value_peek_pointer(val));
		switch ( rec->etype )
		{
			case FREF_EL_NOTE: {
				Tfref_note *n = FREFNOTE(rec->data);
				node = xmlNewNode(NULL, "note");
				xmlNewProp(node, "title", n->title);
				txt = xmlNewText(n->text);
				xmlAddChild(node,txt);
				xmlAddChild(parent,node);
			}	
			break;
		} /* switch */
	}
}

static gboolean fref_save_ref(GtkTreeIter *iter,Tbfwin *bfwin)
{
		gchar *fname = NULL;
		GValue *val,*val2;
/*		GtkTreePath *path;*/
		GtkTreeIter it;
		xmlDocPtr doc;
		xmlNodePtr rnode;


		val = g_new0(GValue, 1);
		gtk_tree_model_get_value(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),iter,FILE_COLUMN, val);
		if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) {
			fname =  (gchar*)g_value_peek_pointer(val);
		}
		if ( !fname ) return FALSE;		
		
		doc = xmlNewDoc("1.0");
		rnode = xmlNewNode(NULL, "ref");
		xmlDocSetRootElement(doc, rnode);
		
		xmlNewProp(rnode, "version", "2");
		val2 = g_new0(GValue, 1);
		gtk_tree_model_get_value(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),iter,STR_COLUMN, val2);
		if (G_IS_VALUE(val2) && g_value_fits_pointer(val2) && g_value_peek_pointer(val2)) {
			xmlNewProp(rnode,"name",g_value_peek_pointer(val2));
		}
		g_value_unset(val2);
		g_free(val2);    	
		val2 = g_new0(GValue, 1);
		gtk_tree_model_get_value(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),iter,PTR_COLUMN, val2);
		if (G_IS_VALUE(val2) && g_value_fits_pointer(val2) && g_value_peek_pointer(val2)) {
			Tfref_record *rr = FREFRECORD(g_value_peek_pointer(val2));
			if (FREFINFO(rr->data)->description)
			{
				xmlNewProp(rnode,"description",FREFINFO(rr->data)->description);
			}	
			gtk_tree_model_iter_children(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store), &it, iter);
			while (1) 
			{	
				fref_save_element(FREFDATA(main_v->frefdata)->store, &it,doc,rnode);		
				if ( !gtk_tree_model_iter_next(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store), &it)) break;
			}							/* while */			
		}
		g_value_unset(val2);
		g_free(val2);    	
		    	
		xmlSaveFormatFileEnc(fname, doc, "UTF-8", 1);
    	xmlFreeDoc(doc);    	
    	xmlCleanupParser();
		g_value_unset(val);
		g_free(val);    	
    	return TRUE;
}

/* ---------------------            CALLBACKS  ---------------------------------------*/

static GtkWidget *fref_dialog2(gchar *title, gchar *label1,gchar *label2,Tbfwin *bfwin, 
																	GtkWidget **entry1,GtkWidget **entry2) 
{
	GtkWidget *dialog,*vbox,*label,*cancelbutton,*okbutton,*table;
		
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), title);
	vbox = GTK_DIALOG(dialog)->vbox;
	table = gtk_table_new(2, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);	
	label = gtk_label_new(label1);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 5, 6);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);			
	gtk_widget_show(label);				
	*entry1 = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), *entry1, 1, 2, 0, 1,
									 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
									 (GtkAttachOptions) (0), 5, 5);	
	label = gtk_label_new(label2);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 5, 6);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);			
	gtk_widget_show(label);				
	*entry2 = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), *entry2, 1, 2, 1,2,
									 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
									 (GtkAttachOptions) (0), 5, 5);	
									 
	cancelbutton = gtk_button_new_from_stock("gtk-cancel");
	gtk_widget_show(cancelbutton);
	gtk_dialog_add_action_widget(GTK_DIALOG(dialog), cancelbutton, GTK_RESPONSE_CANCEL);
	okbutton = gtk_button_new_from_stock("gtk-ok");
	gtk_widget_show(okbutton);
	gtk_dialog_add_action_widget(GTK_DIALOG(dialog), okbutton, GTK_RESPONSE_OK);
	gtk_widget_show_all(dialog);
	return dialog;	
}


static Tfref_record *get_current_entry(Tbfwin * bfwin)
{
	GtkTreePath *path;
	GtkTreeViewColumn *col;
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), &path, &col);
	if (path != NULL) {
		Tfref_record *retval = NULL;
		GValue *val;
		GtkTreeIter iter;
		gtk_tree_model_get_iter(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),/*gtk_tree_view_get_model(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree))*/
								&iter, path);
		gtk_tree_path_free(path);
		val = g_new0(GValue, 1);
		gtk_tree_model_get_value(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),/*gtk_tree_view_get_model(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree)),*/
								 &iter,PTR_COLUMN, val);
		if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) {
			retval = (Tfref_record *) g_value_peek_pointer(val);
		}
		g_value_unset(val);
		g_free(val);
		return retval;
	}
	return NULL;
}

static void fref_popup_menu_rescan_lcb(GtkWidget * widget, gpointer user_data)
{
	gchar *userdir = g_strconcat(g_get_home_dir(), "/.bluefish/", NULL);
	DEBUG_MSG("fref_popup_menu_rescan_lcb, started\n");
	fref_rescan_dir(PKGDATADIR);
	fref_rescan_dir(userdir);
	g_free(userdir);
	DEBUG_MSG("about to refill toplevels\n");
	fill_toplevels(FREFDATA(main_v->frefdata), TRUE);
}

static void fref_setfont_lcb(GtkWidget * widget, Tbfwin *bfwin) {
	GtkWidget *dlg = gtk_font_selection_dialog_new(_("Select font for reference view"));
	gchar *f;
	
	gtk_dialog_run(GTK_DIALOG(dlg));
	f = gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(dlg));
	gtk_widget_destroy(dlg);
	gtk_widget_modify_font(FREFGUI(bfwin->fref)->infoview,pango_font_description_from_string(f));
	g_free(f);
}

static void fref_setcolors_lcb(GtkWidget * widget, Tbfwin *bfwin) {
	GtkWidget *dlg = gtk_dialog_new_with_buttons (_("Set reference colors"),
                                         NULL,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_OK,
                                         GTK_RESPONSE_OK,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_CANCEL,                                         
                                         NULL);
 GtkWidget *btn_descr,*btn_ap,*btn_note,*btn_example,*btn_link;
 GtkWidget *label;
 GtkTextBuffer *buff = NULL;
 
 label = gtk_label_new(_("Description"));
 gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dlg)->vbox),label);
 btn_descr = gtk_color_button_new_with_color(&FREFGUI(bfwin->fref)->col_descr);
 gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dlg)->vbox),btn_descr);
 label = gtk_label_new(_("Attributes/Parameters"));
 gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dlg)->vbox),label);
 btn_ap = gtk_color_button_new_with_color(&FREFGUI(bfwin->fref)->col_ap);
 gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dlg)->vbox),btn_ap);                                        
 label = gtk_label_new(_("Notes"));
 gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dlg)->vbox),label); 
 btn_note = gtk_color_button_new_with_color(&FREFGUI(bfwin->fref)->col_note);
 gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dlg)->vbox),btn_note);          
 label = gtk_label_new(_("Examples"));
 gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dlg)->vbox),label);                               
 btn_example = gtk_color_button_new_with_color(&FREFGUI(bfwin->fref)->col_example);
 gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dlg)->vbox),btn_example);                                        
 label = gtk_label_new(_("Links"));
 gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dlg)->vbox),label); 
 btn_link = gtk_color_button_new_with_color(&FREFGUI(bfwin->fref)->col_link);
 gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dlg)->vbox),btn_link);                                        
 gtk_widget_show_all(GTK_DIALOG(dlg)->vbox);
 if ( gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK ) {
 	gtk_color_button_get_color(GTK_COLOR_BUTTON(btn_descr),&FREFGUI(bfwin->fref)->col_descr);
 	gtk_color_button_get_color(GTK_COLOR_BUTTON(btn_ap),&FREFGUI(bfwin->fref)->col_ap);
 	gtk_color_button_get_color(GTK_COLOR_BUTTON(btn_note),&FREFGUI(bfwin->fref)->col_note);
 	gtk_color_button_get_color(GTK_COLOR_BUTTON(btn_example),&FREFGUI(bfwin->fref)->col_example);
 	gtk_color_button_get_color(GTK_COLOR_BUTTON(btn_link),&FREFGUI(bfwin->fref)->col_link);
 	buff = gtk_text_view_get_buffer(GTK_TEXT_VIEW(FREFGUI(bfwin->fref)->infoview));
 	g_object_set(G_OBJECT(FREFGUI(bfwin->fref)->tag_descr),"foreground-gdk",&FREFGUI(bfwin->fref)->col_descr,NULL);
 	g_object_set(G_OBJECT(FREFGUI(bfwin->fref)->tag_ap),"foreground-gdk",&FREFGUI(bfwin->fref)->col_ap,NULL);
 	g_object_set(G_OBJECT(FREFGUI(bfwin->fref)->tag_note),"foreground-gdk",&FREFGUI(bfwin->fref)->col_note,NULL);
	g_object_set(G_OBJECT(FREFGUI(bfwin->fref)->tag_example),"foreground-gdk",&FREFGUI(bfwin->fref)->col_example,NULL); 	
	g_object_set(G_OBJECT(FREFGUI(bfwin->fref)->tag_link),"foreground-gdk",&FREFGUI(bfwin->fref)->col_link,NULL);
 	frefcb_cursor_changed(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), bfwin);
 }
 gtk_widget_destroy(dlg);                                        
}

static gchar *fref_notnull_str(gchar *str) {
	if ( str == NULL ) return "";
	return str;
}

static gchar *fref_get_prop_descr(Tfref_element *el,gint proptype) {
	gchar *ret = g_strdup("");
	GList *lst = g_list_first(el->properties);
	while ( lst ) {
		if ( proptype == FREFPROPERTY(lst->data)->ptype ) {
		  switch (proptype) {
		  		case FREF_EL_RETURN: if ( FREFPROPERTY(lst->data)->type )
															  			ret = g_strconcat(ret,FREFPROPERTY(lst->data)->type,"\n",NULL);
				break;
		  } /* switch */	
		  ret = g_strconcat(ret,fref_notnull_str(FREFPROPERTY(lst->data)->description),"\n",NULL);
		}  
		lst = g_list_next(lst);
	}
	return ret;
}

gchar *fref_prepare_info(Tfref_record * entry, gint infotype)
{
	gchar *ret,*tofree;
	GList *lst=NULL;
	Tfref_record *rec=NULL;

ret = g_strdup("");

switch (infotype) {
	case FR_INFO_DESC	:
		switch (entry->etype) {
			case FREF_EL_REF: ret = g_strconcat(fref_notnull_str(FREFINFO(entry->data)->description),"\n ",NULL);
			break;		
			case FREF_EL_GROUP:
			case FREF_EL_NOTE: ret = g_strconcat(fref_notnull_str(FREFNOTE(entry->data)->title),":\n ",
														fref_notnull_str(FREFNOTE(entry->data)->text),NULL);
			break;
			case FREF_EL_TAG:
					ret =	g_strconcat(ret, _("TAG: "),fref_notnull_str(FREFELEMENT(entry->data)->name),"\n ",NULL);
					if (FREFELEMENT(entry->data)->description) {
							ret = g_strconcat(ret,fref_notnull_str(FREFELEMENT(entry->data)->description),"\n",NULL);
					}		
					tofree = fref_get_prop_descr(FREFELEMENT(entry->data),FREF_EL_PARENTS);
					if ( tofree && strcmp(tofree,"")!=0 ) {
							ret =	g_strconcat(ret, _("PARENT TAGS: "),tofree,NULL);						                            
							g_free(tofree);
					}						
			break;
			case FREF_EL_FUNCTION:
					ret =	g_strconcat(ret, _("FUNCTION: "),fref_notnull_str(FREFELEMENT(entry->data)->name),"\n",NULL);
					if (FREFELEMENT(entry->data)->description) {
						ret = g_strconcat(ret,fref_notnull_str(FREFELEMENT(entry->data)->description),"\n",NULL);
					}	
					tofree = fref_get_prop_descr(FREFELEMENT(entry->data),FREF_EL_RETURN);                            
					if (tofree && strcmp(tofree,"")!=0) {
							ret =	g_strconcat(ret, _("RETURNS: "),tofree,NULL);						                            
						   g_free(tofree);                         
					}
					tofree = fref_get_prop_descr(FREFELEMENT(entry->data),FREF_EL_DEPEND);                            
					if (tofree && strcmp(tofree,"")!=0) {
							ret =	g_strconcat(ret, _("DEPENDENCIES: "),tofree,NULL);						                            
						   g_free(tofree);                         
					}										
			break;			
			case FREF_EL_CSSPROP:
					ret =	g_strconcat(ret, _("PROPERTY: "),fref_notnull_str(FREFELEMENT(entry->data)->name),"\n",NULL);
					if (FREFELEMENT(entry->data)->description) {
						ret = g_strconcat(ret,fref_notnull_str(FREFELEMENT(entry->data)->description),"\n",NULL);
					}	
					tofree = fref_get_prop_descr(FREFELEMENT(entry->data),FREF_EL_DEPEND);                            
					if (tofree && strcmp(tofree,"")!=0) {
							ret =	g_strconcat(ret, _("DEPENDENCIES: "),tofree,NULL);						                            
						   g_free(tofree);                         
					}										
			break;						
			case FREF_EL_CSSSELECT:
					ret =	g_strconcat(ret, _("SELECTOR: "),fref_notnull_str(FREFELEMENT(entry->data)->name),"\n",NULL);
					if (FREFELEMENT(entry->data)->description) {
						ret = g_strconcat(ret,fref_notnull_str(FREFELEMENT(entry->data)->description),"\n",NULL);
					}	
					tofree = fref_get_prop_descr(FREFELEMENT(entry->data),FREF_EL_DEPEND);                            
					if (tofree && strcmp(tofree,"")!=0) {
							ret =	g_strconcat(ret, _("DEPENDENCIES: "),tofree,NULL);						                            
						   g_free(tofree);                         
					}										
			break;									
			case FREF_EL_CLASS:
					ret =	g_strconcat(ret, _("CLASS: "),fref_notnull_str(FREFELEMENT(entry->data)->name),"\n ",NULL);
					if (FREFELEMENT(entry->data)->description) {
						ret = g_strconcat(ret,FREFELEMENT(entry->data)->description,NULL);
					}				
					tofree = fref_get_prop_descr(FREFELEMENT(entry->data),FREF_EL_PARENTS);
					if ( tofree  && strcmp(tofree,"")!=0) {
							ret =	g_strconcat(ret, _("PARENT CLASSES: "),tofree,NULL);						                            
							g_free(tofree);
					}	
					tofree = fref_get_prop_descr(FREFELEMENT(entry->data),FREF_EL_DEPEND);                            
					if (tofree && strcmp(tofree,"")!=0) {
							ret =	g_strconcat(ret, _("DEPENDENCIES: "),tofree,NULL);						                            
						   g_free(tofree);                         
					}											
			break;			
			case FREF_EL_SNIPPET:
					ret =	g_strconcat(ret, _("SNIPPET: "),fref_notnull_str(FREFELEMENT(entry->data)->name),"\n ",NULL);
					if (FREFELEMENT(entry->data)->description) {
						ret = g_strconcat(ret, FREFELEMENT(entry->data)->description,"\n",NULL);
					}				
			break;			
			default: ret="";
		} /* switch */
	break;
	case FR_INFO_ATTRS:	 
		switch ( entry->etype ) {
			case FREF_EL_FUNCTION:
				lst = g_list_first(FREFELEMENT(entry->data)->properties);
				if (g_list_length(lst)>0) 	ret = g_strdup(_("PARAMS:\n"));
				while ( lst != NULL ) {
					if (FREFPROPERTY(lst->data)->ptype == FREF_EL_PARAM) {
							ret = g_strconcat(ret," - ",fref_notnull_str(FREFPROPERTY(lst->data)->name),"(",
							                            fref_notnull_str(FREFPROPERTY(lst->data)->type),"): ",
							                            fref_notnull_str(FREFPROPERTY(lst->data)->description),"\n",NULL);
					}                            
					lst = g_list_next(lst);
				}
			break;	
			case FREF_EL_TAG:
				lst = g_list_first(FREFELEMENT(entry->data)->properties);
				if (g_list_length(lst)>0) ret = g_strdup(_("ATTRIBUTES:\n"));
				while ( lst != NULL ) {
					if (FREFPROPERTY(lst->data)->ptype == FREF_EL_ATTR) {
							ret = g_strconcat(ret," - ",fref_notnull_str(FREFPROPERTY(lst->data)->name),"(",
							                            fref_notnull_str(FREFPROPERTY(lst->data)->type),"): ",
							                            fref_notnull_str(FREFPROPERTY(lst->data)->description),"\n",NULL);
					}                            
					lst = g_list_next(lst);
				}
			break;
			case FREF_EL_CSSPROP:
				lst = g_list_first(FREFELEMENT(entry->data)->properties);
				ret = g_strdup("");
				while ( lst != NULL ) {
					if (FREFPROPERTY(lst->data)->ptype == FREF_EL_PARAM) {
							tofree = ret;
							ret = g_strconcat(ret,fref_notnull_str(FREFPROPERTY(lst->data)->name),": ",
							                            fref_notnull_str(FREFPROPERTY(lst->data)->vlist),NULL);
							g_free(tofree);
							tofree = ret;							
							if (FREFPROPERTY(lst->data)->vlist2)
								ret = g_strconcat(tofree,",",FREFPROPERTY(lst->data)->vlist2,"\n",NULL);			
							else
								ret = g_strconcat(tofree,"\n",NULL);			
							g_free(tofree);	
					}                            
					lst = g_list_next(lst);
				}			
			break;				
			case FREF_EL_CSSSELECT:
			break;							
			case FREF_EL_CLASS:
				lst = g_list_first(FREFELEMENT(entry->data)->properties);
				if (g_list_length(lst)>0) ret = g_strdup(_("----- FIELDS:\n"));
				while ( lst != NULL ) {
					if (FREFPROPERTY(lst->data)->ptype == FREF_EL_ATTR) {
							ret = g_strconcat(ret," - ",fref_notnull_str(FREFPROPERTY(lst->data)->name),": ",
							                            fref_notnull_str(FREFPROPERTY(lst->data)->description),"\n",NULL);
					}                            
					lst = g_list_next(lst);
				}				
				lst = g_list_first(FREFELEMENT(entry->data)->children);
				if (g_list_length(lst)>0) ret = g_strconcat(ret,_("----- METHODS:\n"),NULL);  
				while ( lst ) {
					rec = g_new0(Tfref_record,1);
					rec->etype = FREFELEMENT(lst->data)->etype;
					rec->data = FREFELEMENT(lst->data);
					ret = g_strconcat(ret,fref_prepare_info(rec,FR_INFO_DESC),"\n",NULL);
					lst = g_list_next(lst);
					g_free(rec);
				}
			break;
			case FREF_EL_SNIPPET:
				lst = g_list_first(FREFELEMENT(entry->data)->properties);
				if (g_list_length(lst)>0) ret = g_strdup(_("PARAMS:\n"));
				while ( lst != NULL ) {
					if (FREFPROPERTY(lst->data)->ptype == FREF_EL_PARAM) {
							ret = g_strconcat(ret," - ",fref_notnull_str(FREFPROPERTY(lst->data)->name),": ",
							                            fref_notnull_str(FREFPROPERTY(lst->data)->description),"\n",NULL);
					}                            
					lst = g_list_next(lst);
				}
			break;			
			default: ret="";
		} 
	break;
	case FR_INFO_NOTES:
		switch ( entry->etype ) {
			case FREF_EL_TAG:
			case FREF_EL_FUNCTION:
			case FREF_EL_CLASS:
			case FREF_EL_CSSPROP:
			case FREF_EL_CSSSELECT:			
			case FREF_EL_SNIPPET:
						lst = g_list_first(FREFELEMENT(entry->data)->notes);
						if (g_list_length(lst)>0) ret = g_strdup(_("\nNOTES:\n"));
						while (lst != NULL) {
							Tfref_note *note = FREFNOTE(lst->data);
							if ( note->title && note->text ) {
								ret = g_strconcat(ret,fref_notnull_str(note->title),"\n", 
								                           fref_notnull_str(note->text),"\n", NULL);
							}
							lst = g_list_next(lst);
						}			
			break;
			default: ret="";
		} /* switch */
	break;
	case FR_INFO_EXAMPLES:
		switch ( entry->etype ) {
			case FREF_EL_TAG:
			case FREF_EL_FUNCTION:
			case FREF_EL_CLASS:
			case FREF_EL_CSSPROP:
			case FREF_EL_CSSSELECT:			
			case FREF_EL_SNIPPET:
						tofree = fref_get_prop_descr(FREFELEMENT(entry->data),FREF_EL_EXAMPLE);
						if ( tofree && strcmp("",tofree)!=0) {
									ret = g_strconcat(_("\nEXAMPLES:\n"),tofree,"\n",NULL);
									g_free(tofree);
						}			
			break;
			default: ret="";
		} /* switch */	
	break;
	case FR_INFO_LINKS:
		switch ( entry->etype ) {
			case FREF_EL_TAG:
			case FREF_EL_FUNCTION:
			case FREF_EL_CLASS:
			case FREF_EL_CSSPROP:
			case FREF_EL_CSSSELECT:			
			case FREF_EL_SNIPPET:
						tofree = fref_get_prop_descr(FREFELEMENT(entry->data),FREF_EL_LINK);
						if ( tofree && strcmp(tofree,"")!=0) {
									ret = g_strconcat(_("SEE ALSO:\n "),tofree,NULL);
									g_free(tofree);
						}			
			break;
			default: ret="";
		} /* switch */		
	break;
} /* switch infotype */

return ret;
}

static gchar *fref_prepare_text(Tfref_element *elem) {
	gchar *p, *prev, *stringdup/*, *tofree*/;
	gchar *tmp, *dest, *tmp3 = NULL;
	GList *lst;
	gint d1, d2,cnt;
	gchar *converted = NULL/*, *tmp2 = NULL*/;

	dest = g_strdup("");
	stringdup = g_strdup(elem->insert);
	prev = stringdup;
	p = strchr(prev, '%');
	
while (p) {
	tmp = dest;
	*p = '\0';
	p++;
	if (*p == '%') 
	{
			converted = "%";
	} else 
	{
		switch (elem->etype) 
		{
			case FREF_EL_TAG:
				converted = "";
				d1 = g_ascii_digit_value(*p);
				if (d1 != -1) 
				{
					d2 = g_ascii_digit_value(*(p + 1));
					if (d2 != -1)
					{
						d1 = d1 * 10 + d2;
						p++;
					}	
					lst = g_list_first(elem->properties);
					cnt=0;
					while ( lst ) 
					{
						if ( FREFPROPERTY(lst->data)->ptype == FREF_EL_ATTR) 
						{ 
							if (cnt>=d1) break;												
							cnt++;
					   }
						lst = g_list_next(lst);
					} /* while */
					if (lst != NULL && FREFPROPERTY(lst->data)->ptype == FREF_EL_ATTR) 
					{
							if ( FREFPROPERTY(lst->data)->dlgitem != NULL )
							{
								if ( GTK_IS_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem) ) { /* combo */
									GtkTreeIter it;
									if ( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem),&it) ) {
										GValue *val = g_new0(GValue, 1);
										gtk_tree_model_get_value(GTK_TREE_MODEL(gtk_combo_box_get_model(
																								 GTK_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem)	)),
																								 &it, 0, val);
										if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) {
												converted = g_strconcat(" ",FREFPROPERTY(lst->data)->name,"=\"",
																		 g_value_peek_pointer(val),
																		 "\"",NULL);
										}
										g_value_unset(val);
										g_free(val);												
									}									
								} else { /* entry */
									converted = g_strconcat(" ",FREFPROPERTY(lst->data)->name,"=\"",
															 gtk_entry_get_text(GTK_ENTRY(FREFPROPERTY(lst->data)->dlgitem)),
															 "\"",NULL);
								}
							} /* null item */													
					} 
				} /* d1 == -1 */
				else if (*p == '_' ) /* only non-empty attributes from dialog */
				{
					lst = g_list_first(elem->properties);
					converted = "";
					while ( lst ) 
					{
							if ( FREFPROPERTY(lst->data)->ptype == FREF_EL_ATTR ) 
							{ 
									if ( FREFPROPERTY(lst->data)->dlgitem != NULL )
									{
										if ( GTK_IS_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem) ) { /* combo */
											GtkTreeIter it;
											if ( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem),&it) ) {
												GValue *val = g_new0(GValue, 1);
												gtk_tree_model_get_value(GTK_TREE_MODEL(gtk_combo_box_get_model(
																										 GTK_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem)	)),
																										 &it, 0, val);
												if (G_IS_VALUE(val) && g_value_fits_pointer(val) && 
														g_value_peek_pointer(val) && strcmp("",(gchar*)g_value_peek_pointer(val))!=0) {
														converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																				 g_value_peek_pointer(val),
																				 "\"",NULL);
												}
												g_value_unset(val);
												g_free(val);												
											}									
										} else { /* entry */
											if ( strcmp("",gtk_entry_get_text(GTK_ENTRY(FREFPROPERTY(lst->data)->dlgitem))) != 0 )
												converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																	 gtk_entry_get_text(GTK_ENTRY(FREFPROPERTY(lst->data)->dlgitem)),
																	 "\"",NULL);
										}
									} /* null item */							
					   	}
							lst = g_list_next(lst);
					} /* while */							
				}
				else if (*p == '*') /* ALL attributes */
				{
						lst = g_list_first(elem->properties);
						converted = "";
						while ( lst ) 
						{
							if ( FREFPROPERTY(lst->data)->ptype == FREF_EL_ATTR ) 
							{ 
									if ( FREFPROPERTY(lst->data)->dlgitem != NULL )
									{
										if ( GTK_IS_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem) ) { /* combo */
											GtkTreeIter it;
											if ( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem),&it) ) {
												GValue *val = g_new0(GValue, 1);
												gtk_tree_model_get_value(GTK_TREE_MODEL(gtk_combo_box_get_model(
																										 GTK_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem)	)),
																										 &it, 0, val);
												if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) {
														converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																				 g_value_peek_pointer(val),
																				 "\"",NULL);
												}
												g_value_unset(val);
												g_free(val);												
											}									
										} else { /* entry */
											converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																	 gtk_entry_get_text(GTK_ENTRY(FREFPROPERTY(lst->data)->dlgitem)),
																	 "\"",NULL);
										}
									} /* null item */							
					   	}
							lst = g_list_next(lst);
						} /* while */							
				}
				else if (*p == '!') /* only required attributes */
				{
						lst = g_list_first(elem->properties);
						converted = "";
						while ( lst ) 
						{
							if ( FREFPROPERTY(lst->data)->ptype == FREF_EL_ATTR && FREFPROPERTY(lst->data)->required) 
							{ 
									if ( FREFPROPERTY(lst->data)->dlgitem != NULL )
									{
										if ( GTK_IS_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem) ) { /* combo */
											GtkTreeIter it;
											if ( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem),&it) ) {
												GValue *val = g_new0(GValue, 1);
												gtk_tree_model_get_value(GTK_TREE_MODEL(gtk_combo_box_get_model(
																										 GTK_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem)	)),
																										 &it, 0, val);
												if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) {
														converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																				 g_value_peek_pointer(val),
																				 "\"",NULL);
												}
												g_value_unset(val);
												g_free(val);												
											}									
										} else { /* entry */
											converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																	 gtk_entry_get_text(GTK_ENTRY(FREFPROPERTY(lst->data)->dlgitem)),
																	 "\"",NULL);
										}
									} /* null item */							
					   	}
							lst = g_list_next(lst);
						} /* while */			
				}
				else if (*p == '~' ) /* only non-empty and required attributes from dialog*/
				{
					lst = g_list_first(elem->properties);
					converted = "";
					while ( lst ) 
					{
							if ( FREFPROPERTY(lst->data)->ptype == FREF_EL_ATTR && FREFPROPERTY(lst->data)->required) 
							{ 
									if ( FREFPROPERTY(lst->data)->dlgitem != NULL )
									{
										if ( GTK_IS_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem) ) { /* combo */
											GtkTreeIter it;
											if ( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem),&it) ) {
												GValue *val = g_new0(GValue, 1);
												gtk_tree_model_get_value(GTK_TREE_MODEL(gtk_combo_box_get_model(
																										 GTK_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem)	)),
																										 &it, 0, val);
												if (G_IS_VALUE(val) && g_value_fits_pointer(val) && 
												      g_value_peek_pointer(val) && strcmp("",(gchar*)g_value_peek_pointer(val))!=0) {
														converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																				 g_value_peek_pointer(val),
																				 "\"",NULL);
												}
												g_value_unset(val);
												g_free(val);												
											}									
										} else { /* entry */
											if ( strcmp("",gtk_entry_get_text(GTK_ENTRY(FREFPROPERTY(lst->data)->dlgitem))) != 0 )
												converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																	 gtk_entry_get_text(GTK_ENTRY(FREFPROPERTY(lst->data)->dlgitem)),
																	 "\"",NULL);
										}
									} /* null item */							
					   	}
							lst = g_list_next(lst);
					} /* while */											
				}				
			break	; /* TAG DATA */
			case FREF_EL_FUNCTION:
			case FREF_EL_SNIPPET:			
			case FREF_EL_CSSPROP:
			case FREF_EL_CSSSELECT:						
				converted="";
				d1 = g_ascii_digit_value(*p);
				if (d1 != -1) 
				{
					d2 = g_ascii_digit_value(*(p + 1));
					if (d2 != -1)
					{
						d1 = d1 * 10 + d2;
						p++;
					}	
					lst = g_list_first(elem->properties);
					cnt=0;
					while ( lst ) 
					{
						if ( FREFPROPERTY(lst->data)->ptype == FREF_EL_PARAM) 
						{ 
							if (cnt>=d1) break;						
							cnt++;
						}
						lst = g_list_next(lst);
					}
					if (lst != NULL && FREFPROPERTY(lst->data)->ptype == FREF_EL_PARAM ) {
							if ( FREFPROPERTY(lst->data)->dlgitem != NULL )
							{
								if ( GTK_IS_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem) ) { /* combo */
									GtkTreeIter it;
									if ( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem),&it) ) {
										GValue *val = g_new0(GValue, 1);
										gtk_tree_model_get_value(GTK_TREE_MODEL(gtk_combo_box_get_model(
																								 GTK_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem)	)),
																								 &it, 0, val);
										if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) {
												converted = g_strdup(g_value_peek_pointer(val));
										}
										g_value_unset(val);
										g_free(val);												
									}									
								} else { /* entry */
									converted = g_strdup(gtk_entry_get_text(GTK_ENTRY(FREFPROPERTY(lst->data)->dlgitem)));
								}
							} /* null item */							
					} 
				} /* d1 != -1 */
				else if (*p == '*') /* ALL parameters */
				{
						lst = g_list_first(elem->properties);
						converted = "";
						gboolean first=TRUE;
						while ( lst ) 
						{
							if ( FREFPROPERTY(lst->data)->ptype == FREF_EL_PARAM) 
							{ 
									if ( FREFPROPERTY(lst->data)->dlgitem != NULL )
									{
										if ( GTK_IS_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem) ) { /* combo */
											GtkTreeIter it;
											if ( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem),&it) ) {
												GValue *val = g_new0(GValue, 1);
												gtk_tree_model_get_value(GTK_TREE_MODEL(gtk_combo_box_get_model(
																										 GTK_COMBO_BOX(FREFPROPERTY(lst->data)->dlgitem)	)),
																										 &it, 0, val);
												if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) {
														if ( first )
														{
															converted = g_strconcat(converted,g_value_peek_pointer(val),NULL);
															first = FALSE;
														} else	
															converted = g_strconcat(converted,",",g_value_peek_pointer(val),NULL);
												}
												g_value_unset(val);
												g_free(val);												
											}									
										} else { /* entry */
											if (first)
											{
												converted = g_strconcat(converted,
																		 gtk_entry_get_text(GTK_ENTRY(FREFPROPERTY(lst->data)->dlgitem)),NULL);
												first = FALSE;						 
											} else
												converted = g_strconcat(converted,",",
																		 gtk_entry_get_text(GTK_ENTRY(FREFPROPERTY(lst->data)->dlgitem)),NULL);
										}
									} /* null item */							
					   	}
							lst = g_list_next(lst);
						} /* while */							
				}
			break; /* FUNCTION DATA */
		}					/* switch */
	} /* else */
		dest = g_strconcat(dest, prev, converted, NULL);
		g_free(tmp);			/* here I free the dest (tmp=dest) */
		g_free(tmp3);
		if (strcmp(converted,"")!=0)
			g_free(converted);
		prev = ++p;
		p = strchr(p, '%');
}
	tmp = dest;
	dest = g_strconcat(dest, prev, NULL);
	g_free(tmp);
	g_free(stringdup);
	return dest;
}


static GList *fref_string_to_list(gchar * string, gchar * delimiter)
{
	GList *lst;
	gchar **arr;
	gint i;

	lst = NULL;
	arr = g_strsplit(string, delimiter, 0);
	i = 0;
	while (arr[i] != NULL && strcmp(arr[i],"\n")!=0) {
		lst = g_list_append(lst, g_strstrip(arr[i]));
		i++;
	}
	return lst;
}


static GtkWidget *fref_prepare_dialog(Tbfwin * bfwin, Tfref_element *entry)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *input;
	GtkWidget *combo;
	GtkWidget *dialog_action_area;
	GtkWidget *cancelbutton;
	GtkWidget *okbutton;
	GtkWidget *scroll;
	GtkRequisition req, req2;
	GList *list = NULL;
	gint itnum, w, h;
	gchar *tofree;

	dialog = gtk_dialog_new();
	if (entry->name != NULL)
		gtk_window_set_title(GTK_WINDOW(dialog), entry->name);
	vbox = GTK_DIALOG(dialog)->vbox;
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);

	list = g_list_first(entry->properties);

	if (list == NULL) {
		gtk_widget_destroy(dialog);
		DEBUG_MSG("fref_prepare_dialog, list==NULL, aborting..\n");
		return NULL;
	}

	itnum = 0;
	while ( list )
	{
		if ( FREFPROPERTY(list->data)->ptype == FREF_EL_ATTR || 
			  FREFPROPERTY(list->data)->ptype == FREF_EL_PARAM ) itnum++;
		list = g_list_next(list);
	}
	
	table = gtk_table_new(itnum, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), table);

	list = g_list_first(entry->properties);	
	itnum=0;
	while ( list )
	{
		if ( FREFPROPERTY(list->data)->ptype == FREF_EL_ATTR || 
			   FREFPROPERTY(list->data)->ptype == FREF_EL_PARAM )
		{
			label = gtk_label_new("");
			if ( FREFPROPERTY(list->data)->required )
			{
				tofree =g_strconcat("<span color='#FF0000'>", FREFPROPERTY(list->data)->name, "</span>", NULL);
				gtk_label_set_markup(GTK_LABEL(label), tofree);
				g_free(tofree);			
			}
			else
			{
						gtk_label_set_text(GTK_LABEL(label), FREFPROPERTY(list->data)->name);			
			}
			gtk_table_attach(GTK_TABLE(table), label, 0, 1, itnum, itnum + 1,
								 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 5, 6);
			gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
			gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);			
			gtk_widget_show(label);			
			if ( FREFPROPERTY(list->data)->vlist )
			{
					GList *gl = fref_string_to_list(FREFPROPERTY(list->data)->vlist, ","),*pomgl,*l;
					gint pos,pos2=0;
					if ( FREFPROPERTY(list->data)->vlist2 )
					{
						pomgl = fref_string_to_list(FREFPROPERTY(list->data)->vlist2, ",");
						l = g_list_first(pomgl);
						while (l)
						{
							g_list_append(gl,l->data);
							l = g_list_next(l);
						}
						g_list_free(pomgl);
					}
					
					if ( FREFPROPERTY(list->data)->vlistonly )
						combo = gtk_combo_box_new_text();
					else
						combo = gtk_combo_box_entry_new_text();	
					if ( gl )
					{
						pomgl = g_list_first(gl);
						pos=0;
						while (pomgl)
						{
								gtk_combo_box_insert_text(GTK_COMBO_BOX(combo),pos,pomgl->data);
								if ( FREFPROPERTY(list->data)->def && ( strcmp(pomgl->data,FREFPROPERTY(list->data)->def) == 0) )
								{
									pos2 = pos;
								}
								pomgl = g_list_next(pomgl);
								pos++;
						}
						g_list_free(gl);
					}						
					gtk_widget_show(combo);
					gtk_table_attach(GTK_TABLE(table), combo, 1, 2, itnum, itnum + 1,
									 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
									 (GtkAttachOptions) (0), 5, 5);
					if (FREFPROPERTY(list->data)->def != NULL)
					{
						gtk_combo_box_set_active(GTK_COMBO_BOX(combo),pos2);
					}	
					FREFPROPERTY(list->data)->dlgitem = combo;
					if (FREFPROPERTY(list->data)->description)
						gtk_tooltips_set_tip(FREFGUI(bfwin->fref)->argtips, combo,FREFPROPERTY(list->data)->description, "");
			}
			else /* not vlist */
			{
					input = gtk_entry_new();
					if (FREFPROPERTY(list->data)->def != NULL)
						gtk_entry_set_text(GTK_ENTRY(input), FREFPROPERTY(list->data)->def);
					gtk_widget_show(input);
					gtk_table_attach(GTK_TABLE(table), input, 1, 2, itnum, itnum + 1,
									 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
									 (GtkAttachOptions) (0), 5, 5);
					FREFPROPERTY(list->data)->dlgitem = input;									 
					gtk_entry_set_activates_default(GTK_ENTRY(input), TRUE);
					if (FREFPROPERTY(list->data)->description)
						gtk_tooltips_set_tip(FREFGUI(bfwin->fref)->argtips, input,FREFPROPERTY(list->data)->description, "");
			}
		}	  		
		itnum++;
		list = g_list_next(list);
	} /* while */

	dialog_action_area = GTK_DIALOG(dialog)->action_area;
	gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area), GTK_BUTTONBOX_END);
	cancelbutton = gtk_button_new_from_stock("gtk-cancel");
	gtk_widget_show(cancelbutton);
	gtk_dialog_add_action_widget(GTK_DIALOG(dialog), cancelbutton, GTK_RESPONSE_CANCEL);
	okbutton = gtk_button_new_from_stock("gtk-ok");
	gtk_widget_show(okbutton);
	gtk_dialog_add_action_widget(GTK_DIALOG(dialog), okbutton, GTK_RESPONSE_OK);
	gtk_widget_show(dialog_action_area);
	gtk_widget_show(table);
	gtk_window_get_size(GTK_WINDOW(dialog), &w, &h);
	gtk_widget_size_request(table, &req);
	gtk_widget_size_request(dialog_action_area, &req2);
	gtk_window_resize(GTK_WINDOW(dialog), req.width + 25, MIN(400, req.height + req2.height + 20));
	gtk_widget_show_all(dialog);
	gtk_tooltips_enable(FREFGUI(bfwin->fref)->argtips);
	return dialog;
}


static void fref_dialog_lcb(GtkButton * button, Tbfwin * bfwin) {
	Tfref_record *el = get_current_entry(bfwin);
	GtkWidget *dialog;
	gint resp;
	gchar *pomstr;
	if ( el == NULL || el->etype == FREF_EL_GROUP || el->etype == FREF_EL_REF ) return;
	switch ( el->etype ) 
	{
		case FREF_EL_FUNCTION:
		case FREF_EL_TAG:
		case FREF_EL_CSSPROP:
		case FREF_EL_CSSSELECT:				
		case FREF_EL_CLASS:
		case FREF_EL_SNIPPET:{
					dialog = fref_prepare_dialog(bfwin, el->data);
					if (dialog) 
					{
							resp = gtk_dialog_run(GTK_DIALOG(dialog));
							if (resp == GTK_RESPONSE_OK) 
							{
									pomstr = fref_prepare_text(el->data);
									gtk_widget_destroy(dialog);
									doc_insert_two_strings(bfwin->current_document, pomstr, NULL);
									gtk_widget_grab_focus(bfwin->current_document->view);									
									g_free(pomstr);
							} else
								gtk_widget_destroy(dialog);
					}			
		}	
		break;		
	}		
}

static void fref_insert_lcb(GtkButton * button, Tbfwin * bfwin) {
	Tfref_record *el = get_current_entry(bfwin);
	if ( el == NULL || el->etype == FREF_EL_GROUP || el->etype == FREF_EL_REF ) return;
	switch ( el->etype ) {
		case FREF_EL_FUNCTION:
		case FREF_EL_TAG:
		case FREF_EL_CSSPROP:
		case FREF_EL_CSSSELECT:		
		case FREF_EL_CLASS:
		case FREF_EL_SNIPPET:{
			Tfref_element *elem = FREFELEMENT(el->data);
			if (elem->insert != NULL) {
				gchar *p = strchr(elem->insert, '%');
				if ( p ) 
				{
					fref_dialog_lcb(button,bfwin);
				}
				else
				{
					doc_insert_two_strings(bfwin->current_document, elem->insert, NULL);
					gtk_widget_grab_focus(bfwin->current_document->view);
				}	
			}	
		}	
		break;		
	}	
}



static gboolean frefcb_event_mouseclick(GtkWidget * widget, GdkEventButton * event, Tbfwin * bfwin);

typedef struct {
	Tbfwin *bfwin;
	gchar *cat;
} Tfref_cleanup;

static guint fref_idle_cleanup(Tfref_cleanup * data)
{
	GtkTreeIter iter;
	gboolean cont = TRUE;
	
	
	DEBUG_MSG("fref_idle_cleanup, started for data=%s\n", data->cat);
	cont = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store), &iter);
	while (cont) {
		gchar *str = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store), &iter, STR_COLUMN,
						   &str, -1);
		DEBUG_MSG("fref_idle_cleanup, comparing %s,%s\n", str, data->cat);
		if (strcmp(str, data->cat) == 0) {
			DEBUG_MSG("fref_idle_cleanup, found!\n");
			fref_unload_ref(GTK_TREE_STORE(FREFDATA(main_v->frefdata)->store), &iter,data->bfwin);
			break;
		}
		g_free (str);
		cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store), &iter);
	}
	{
		GtkTreeIter newiter;
		gtk_tree_store_append(GTK_TREE_STORE(FREFDATA(main_v->frefdata)->store), &newiter, &iter);
	}
	g_free (data->cat);
	g_free (data);
	return FALSE;				/* do not call again */
}


static void frefcb_row_collapsed(GtkTreeView * treeview, GtkTreeIter * arg1, GtkTreePath * arg2,
								 Tbfwin * bfwin)
{
	gint *cnt = NULL;
	gpointer *aux;
	gboolean do_unload = FALSE;
	GtkTreeModel *treemodel = GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store);
	GtkTextIter its,ite;
	GtkTextBuffer *buff = gtk_text_view_get_buffer(GTK_TEXT_VIEW(FREFGUI(bfwin->fref)->infoview));
	
	
	
	if (gtk_tree_path_get_depth(arg2) == 1) {
		gchar *cat;
		gtk_tree_model_get(treemodel, arg1, STR_COLUMN, &cat, -1);
		DEBUG_MSG("frefcb_row_collapsed, lookup for the refcount of %s\n", cat);
		aux = g_hash_table_lookup(FREFDATA(main_v->frefdata)->refcount, cat);
		if (aux != NULL) {
			cnt = (gint *) aux;
			DEBUG_MSG("frefcb_row_collapsed, refcount=%d, about to decrease\n", *cnt);
			*cnt = (*cnt) - 1;
			if (*cnt == 0) {		
				Tfref_cleanup *data = g_new(Tfref_cleanup, 1);
				do_unload = TRUE;
				data->cat = cat;
				data->bfwin = bfwin;
				g_signal_handler_block(FREFGUI(bfwin->fref)->tree,FREFGUI(bfwin->fref)->row_expand_signal);	
				g_signal_handler_block(FREFGUI(bfwin->fref)->tree,FREFGUI(bfwin->fref)->tree_signal);				
				/*gtk_timeout_add(250, (GtkFunction) fref_idle_cleanup, data); */
				fref_idle_cleanup(data);
				gtk_text_buffer_get_start_iter(buff,&its);
				gtk_text_buffer_get_end_iter(buff,&ite);
   			gtk_text_buffer_delete(buff,&its,&ite);

				g_signal_handler_unblock(FREFGUI(bfwin->fref)->tree,FREFGUI(bfwin->fref)->tree_signal);	
				g_signal_handler_unblock(FREFGUI(bfwin->fref)->tree,FREFGUI(bfwin->fref)->row_expand_signal);
			}
		} else
			do_unload = FALSE;
	} else
		do_unload = FALSE;	
}

static gboolean frefcb_event_mouseclick(GtkWidget * widget, GdkEventButton * event, Tbfwin * bfwin)
{
	Tfref_record *entry;

	if (widget != FREFGUI(bfwin->fref)->tree)
		return FALSE;
		
	entry = get_current_entry(bfwin);
	if (entry == NULL) {
			return FALSE;
	}
  if (event->type == GDK_2BUTTON_PRESS && event->button == 1) {	 /* double click  */
		 fref_insert_lcb(NULL,bfwin);
	}
	return FALSE;				
	/*  we have handled the event, but the treeview freezes if you return TRUE,    so we return FALSE */
}


static void frefcb_row_expanded(GtkTreeView * treeview, GtkTreeIter * arg1, GtkTreePath * arg2,
								Tbfwin *bfwin)
{
	GValue *val;
	GtkTreeIter iter;
	gchar *cat;
	gint *cnt = NULL;
	gpointer *aux;
	gboolean do_load = FALSE;
	Tfref_info *info;
	Tfref_record *el;
	gpointer user_data = FREFDATA(main_v->frefdata)->store;
	
	g_signal_handler_block(FREFGUI(bfwin->fref)->tree,FREFGUI(bfwin->fref)->tree_signal);
	g_signal_handler_block(FREFGUI(bfwin->fref)->tree,FREFGUI(bfwin->fref)->row_collapse_signal);
	
	if (gtk_tree_path_get_depth(arg2) == 1) {
		val = g_new0(GValue, 1);
		gtk_tree_model_get_value(GTK_TREE_MODEL(user_data), arg1, 0, val);
		cat = (gchar *) (g_value_peek_pointer(val));
		aux = g_hash_table_lookup(FREFDATA(main_v->frefdata)->refcount, cat);
		if (aux != NULL) {
			cnt = (gint *) aux;
			*cnt = (*cnt) + 1;
			if (*cnt == 1)
				do_load = TRUE;
			else
				do_load = FALSE;
		} else
			do_load = FALSE;
		g_value_unset (val);
		g_free (val);
	} else
		do_load = FALSE;
	if (do_load) {
		val = g_new0(GValue, 1);
		gtk_tree_model_get_value(GTK_TREE_MODEL(user_data), arg1, FILE_COLUMN, val);
		info = g_new0(Tfref_info,1);
		DEBUG_MSG("frefcb_row_expanded, search hash_table at %p\n", info->dictionary);
		
		if (G_IS_VALUE(val) && g_value_peek_pointer(val) != NULL) {			
			fref_load_from_file((gchar *) g_value_peek_pointer(val), GTK_WIDGET(treeview),
									 GTK_TREE_STORE(user_data), arg1, info,bfwin);			
			el = g_new0(Tfref_record,1);						 
			el->etype = FREF_EL_REF;
			el->data = info;
			gtk_tree_store_set(GTK_TREE_STORE(user_data), arg1, PTR_COLUMN, el, -1);
		}
		if (g_value_peek_pointer(val) != NULL
			&& gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(user_data), &iter, arg1, 0)) {
			gtk_tree_store_remove(GTK_TREE_STORE(user_data), &iter);
		}
		g_value_unset(val);
		g_free(val);
	}
	g_signal_handler_unblock(FREFGUI(bfwin->fref)->tree,FREFGUI(bfwin->fref)->row_collapse_signal);
	g_signal_handler_unblock(FREFGUI(bfwin->fref)->tree,FREFGUI(bfwin->fref)->tree_signal);
}

static void frefcb_cursor_changed(GtkTreeView * treeview, Tbfwin * bfwin)
{
 	Tfref_record *entry;
	gchar *tmpinfo = NULL; 
	GtkTextIter its,ite;
	GtkTextBuffer *buff = gtk_text_view_get_buffer(GTK_TEXT_VIEW(FREFGUI(bfwin->fref)->infoview));

	gtk_text_buffer_get_start_iter(buff,&its);
	gtk_text_buffer_get_end_iter(buff,&ite);
   gtk_text_buffer_delete(buff,&its,&ite);
	
	entry = get_current_entry(bfwin);
	if (entry == NULL)
		return;
		
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(FREFGUI(bfwin->fref)->infocheck))) {						
			if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_descr))) {
				tmpinfo = g_strconcat(fref_prepare_info(entry, FR_INFO_DESC),NULL);
				if ( tmpinfo ) {
					gtk_text_buffer_get_end_iter(buff,&its);
					gtk_text_buffer_insert_with_tags_by_name(buff,&its,tmpinfo,strlen(tmpinfo),"color_descr",NULL);
					g_free(tmpinfo);
				}
			}
			if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_ap))) {
				tmpinfo = g_strconcat(fref_prepare_info(entry, FR_INFO_ATTRS),NULL);
				if ( tmpinfo ) {
					gtk_text_buffer_get_end_iter(buff,&its);
					gtk_text_buffer_insert_with_tags_by_name(buff,&its,tmpinfo,strlen(tmpinfo),"color_ap",NULL);
					g_free(tmpinfo);				
				}	
			}
			if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_note))) {
				tmpinfo = g_strconcat(fref_prepare_info(entry, FR_INFO_NOTES),NULL);
				if ( tmpinfo ) {
					gtk_text_buffer_get_end_iter(buff,&its);
					gtk_text_buffer_insert_with_tags_by_name(buff,&its,tmpinfo,strlen(tmpinfo),"color_note",NULL);
					g_free(tmpinfo);
				}	
			}						
			if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_example))) {
				tmpinfo = g_strconcat(fref_prepare_info(entry, FR_INFO_EXAMPLES),NULL);
				if ( tmpinfo ) {
					gtk_text_buffer_get_end_iter(buff,&its);
					gtk_text_buffer_insert_with_tags_by_name(buff,&its,tmpinfo,strlen(tmpinfo),"color_example",NULL);
					g_free(tmpinfo);
				}	
			}						
			if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_link))) {
				tmpinfo = g_strconcat(fref_prepare_info(entry, FR_INFO_LINKS),NULL);
				if ( tmpinfo ) {
					gtk_text_buffer_get_end_iter(buff,&its);
					gtk_text_buffer_insert_with_tags_by_name(buff,&its,tmpinfo,strlen(tmpinfo),"color_link",NULL);
					g_free(tmpinfo);
				}	
			}									
	} 
}


static void frefcb_infocheck_toggled(GtkToggleButton * togglebutton, Tbfwin * bfwin)
{
	if (gtk_toggle_button_get_active(togglebutton))
		gtk_widget_show(FREFGUI(bfwin->fref)->infoscroll);
	else
		gtk_widget_hide(FREFGUI(bfwin->fref)->infoscroll);
}

static void frefcb_showbtn_clicked(GtkToggleButton * togglebutton, Tbfwin * bfwin) {
 gtk_menu_popup(GTK_MENU(FREFGUI(bfwin->fref)->menu_show),NULL,NULL,NULL,NULL,1,
                             gtk_get_current_event_time()); 
}

static void frefcb_prefbtn_clicked(GtkToggleButton * togglebutton, Tbfwin * bfwin) {
 gtk_menu_popup(GTK_MENU(FREFGUI(bfwin->fref)->menu_pref),NULL,NULL,NULL,NULL,1,
                             gtk_get_current_event_time());
}

static void frefcb_editbtn_clicked(GtkToggleButton * togglebutton, Tbfwin * bfwin) {
 gtk_menu_popup(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),NULL,NULL,NULL,NULL,1,
                             gtk_get_current_event_time());
}

static void frefcb_showitem_activated(GtkCheckMenuItem *menuitem,Tbfwin * bfwin) {
frefcb_cursor_changed(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), bfwin);                                                          
}


static void fref_search(Tbfwin * bfwin,const gchar *phrase)
{
	gchar *stf = NULL;
	Tfref_record *el=NULL;
	gpointer ret=NULL;
	GtkTreePath *path;
	GtkTreeViewColumn *col;
	GValue *val;
	GtkTreeIter iter;
	
	
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), &path, &col);
	if (path != NULL) {
		while (gtk_tree_path_get_depth(path) > 1 && gtk_tree_path_up(path));
		gtk_tree_model_get_iter(gtk_tree_view_get_model(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree)),
								&iter, path);
		gtk_tree_path_free(path);
		val = g_new0(GValue, 1);
		gtk_tree_model_get_value(gtk_tree_view_get_model(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree)),
								 &iter, PTR_COLUMN, val);
		if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) 
			el = FREFRECORD(g_value_peek_pointer(val));		
	}	else {
			error_dialog(bfwin->main_window, _("Error"),
			                 _("Perhaps you didn't load a library, or you did not select a library to search in."));
			return;			                 		
	}	
	
	if ( el == NULL)
		return;
	ret = g_hash_table_lookup(FREFINFO(el->data)->dictionary, phrase);
	if (!ret) {
						stf = g_strdup_printf(_("Element %s not found"),phrase);
						error_dialog(bfwin->main_window, _("Element search"),stf	 );
						g_free(stf);
					}	
	if (ret != NULL) {
					GtkTreePath *path2 = gtk_tree_row_reference_get_path(ret);
#ifndef HAVE_ATLEAST_GTK_2_2
					gtktreepath_expand_to_root(FREFGUI(bfwin->fref)->tree, path2);
#else
					gtk_tree_view_expand_to_path(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), path2);
#endif
					gtk_tree_view_set_cursor(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), path2,
											 gtk_tree_view_get_column(GTK_TREE_VIEW
																	  (FREFGUI(bfwin->fref)->tree),
																	  0), FALSE);
					gtk_widget_grab_focus(FREFGUI(bfwin->fref)->tree);												  
				}
}

gboolean fref_search_keypress (GtkWidget *widget,GdkEventKey *event,Tbfwin *bfwin) {
	if ( event->keyval != GDK_Return ) 
		return FALSE;
   fref_search(bfwin,gtk_entry_get_text(GTK_ENTRY(FREFGUI(bfwin->fref)->sentry)));
   gtk_widget_grab_focus(FREFGUI(bfwin->fref)->sentry);
   gtk_editable_select_region(GTK_EDITABLE(FREFGUI(bfwin->fref)->sentry),0,-1);
	return TRUE;
}


static void frefcb_new_library(GtkWidget * widget, Tbfwin *bfwin) 
{
	GtkWidget *dialog;
	GtkWidget *e1,*e2;
	gchar *fname;
		
	dialog = fref_dialog2(_("New library"),_("Library name"),_("Description"),bfwin,&e1,&e2);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) 
							{
									if ( strcmp("",gtk_entry_get_text(GTK_ENTRY(e1))) != 0 )
									{
										gchar *userdir = g_strconcat(g_get_home_dir(), "/.bluefish/", NULL);
										FILE *ff;
										gchar *pomstr;
										
										fname = g_strconcat(userdir,"funcref_",gtk_entry_get_text(GTK_ENTRY(e1)),".xml",NULL);
										/* check if file exists */
										ff = fopen(fname,"w");
										pomstr = g_strdup_printf("<ref name=\"%s\" description=\"%s\" version=\"2\">\n</ref>",
																							gtk_entry_get_text(GTK_ENTRY(e1)),
																							gtk_entry_get_text(GTK_ENTRY(e2)));
										fwrite(pomstr,strlen(pomstr),1,ff);
										fclose(ff);
										fref_rescan_dir(userdir);
										g_free(pomstr);
										g_free(userdir);
										fill_toplevels(FREFDATA(main_v->frefdata), TRUE);										
									}	
									else error_dialog(bfwin->main_window,_("Error"),_("Library has to have a name."));
									gtk_widget_destroy(dialog);
							} else
								gtk_widget_destroy(dialog);
}

static void frefcb_delete_library(GtkWidget * widget, Tbfwin *bfwin) 
{
	gchar *btns[] = { GTK_STOCK_NO, GTK_STOCK_YES, NULL };
	gint ret;	
	GtkTreePath *path;
	GtkTreeViewColumn *col;

	
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), &path, &col);
	if (path != NULL) {
		GValue *val;
		GtkTreeIter iter;
		gtk_tree_model_get_iter(gtk_tree_view_get_model(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree)),
								&iter, path);
		gtk_tree_path_free(path);
		val = g_new0(GValue, 1);
		gtk_tree_model_get_value(gtk_tree_view_get_model(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree)),
								 &iter, FILE_COLUMN, val);
		if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) {
					gchar *userdir = g_strconcat(g_get_home_dir(), "/.bluefish/", NULL);		
					ret =	multi_query_dialog(bfwin->main_window, _("Delete library?"), 
											g_value_peek_pointer(val), 0, 0, btns);
					if (ret == 0)	return;
					unlink(g_value_peek_pointer(val));
					fref_rescan_dir(userdir);	
					g_free(userdir);
					fill_toplevels(FREFDATA(main_v->frefdata), TRUE);																		
		}
		g_value_unset(val);
		g_free(val);
	} else
	{
		error_dialog(bfwin->main_window,_("Error"),_("You have to select a reference."));
		return;
	}	
}


static void frefcb_new_note(GtkWidget * widget, Tbfwin *bfwin) {
	GtkTreePath *path,*oldpath;
	GtkTreeViewColumn *col;
	Tfref_record *entry,*rec=NULL;
	Tfref_note *note;
	GtkWidget *dialog;
	GtkWidget *e1,*e2;
	GtkTreeIter iter,auxit;
	GValue *val;
		
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), &path, &col);
	if (path == NULL) return;
	entry = get_current_entry(bfwin);
	if ( !entry )
		{
			error_dialog(bfwin->main_window,_("Error"),_("Reference not loaded."));
			return;
		}
	if  ( entry->etype!=FREF_EL_REF && entry->etype!=FREF_EL_GROUP &&
			entry->etype!=FREF_EL_FUNCTION && entry->etype!=FREF_EL_TAG &&
			entry->etype!=FREF_EL_CLASS && entry->etype!=FREF_EL_CSSPROP && 
			entry->etype!=FREF_EL_CSSSELECT )
		{
			error_dialog(bfwin->main_window,_("Error"),_("Bad element."));
			return;
		}
			
	dialog = fref_dialog2(_("New note"),_("Title"),_("Text"),bfwin,&e1,&e2);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) 
	{
		if ( strcmp(gtk_entry_get_text(GTK_ENTRY(e1)),"") != 0 )
		{
			note = g_new0(Tfref_note,1);
			note->title = xmlStrdup(gtk_entry_get_text(GTK_ENTRY(e1)));
			note->text = xmlStrdup(gtk_entry_get_text(GTK_ENTRY(e2)));
			/* find parent */
			gtk_tree_view_get_cursor(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), &oldpath, &col);
			while (gtk_tree_path_get_depth(path) > 1 && gtk_tree_path_up(path));
			gtk_tree_model_get_iter(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),&auxit,path);
			val = g_new0(GValue, 1);
			gtk_tree_model_get_value(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),&auxit,PTR_COLUMN, val);
			if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) {
				rec =  (Tfref_record*)g_value_peek_pointer(val);
			}
			if (rec->etype != FREF_EL_REF)			
			{
				xmlFree(note->title);
				xmlFree(note->text);
				g_free(note);
				return;
			}											
			rec = fref_insert_into_commons(rec->data,fref_unique_name(-1),FREF_EL_NOTE,note);
			if ( !rec )
			{
				xmlFree(note->title);
				xmlFree(note->text);
				g_free(note);
				return;
			}								
			gtk_tree_model_get_iter(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),&iter, oldpath);			
			gtk_tree_store_append(GTK_TREE_STORE(FREFDATA(main_v->frefdata)->store), &auxit, &iter);
			gtk_tree_store_set(GTK_TREE_STORE(FREFDATA(main_v->frefdata)->store), &auxit, STR_COLUMN,note->title, FILE_COLUMN, NULL, PTR_COLUMN,rec, -1);	
			gtk_tree_model_get_iter(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),&iter, path);
			fref_save_ref(&iter,bfwin);
			gtk_tree_view_expand_row(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree),oldpath,FALSE);
			g_value_unset(val);
			g_free(val);										
		}		
	}		
	gtk_widget_destroy(dialog);
}

/* --------------------------      END CALLBACKS   ---------------------------------------------------------- */

static void fill_toplevels(Tfref_data * fdata, gboolean empty_first)
{
	GList *reflist;
	gint *cnt;

	if (empty_first) {
		gtk_tree_store_clear(fdata->store);
	}
	reflist = g_list_first(main_v->props.reference_files);
	while (reflist) {
		gchar **tmparray = reflist->data;
		if (count_array(tmparray) == 2) {
			if (file_exists_and_readable(tmparray[1])) {
				GtkTreeIter iter;
				GtkTreeIter iter2;
				gtk_tree_store_append(fdata->store, &iter, NULL);
				gtk_tree_store_set(fdata->store, &iter, STR_COLUMN, tmparray[0], PTR_COLUMN, 0,
								   FILE_COLUMN, tmparray[1], -1);
				cnt = g_new0(gint, 1);
				*cnt = 0;
				g_hash_table_replace(fdata->refcount, g_strdup(tmparray[0]), cnt);
				gtk_tree_store_append(fdata->store, &iter2, &iter);
			}
		}
		reflist = g_list_next(reflist);
	}
}


void fref_init() {
	Tfref_data *fdata = g_new(Tfref_data, 1);
	fref_rand = g_rand_new();
	fdata->store = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING);
	fdata->refcount = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	fill_toplevels(fdata, FALSE);
	DEBUG_MSG("fref_init, refcount at %p\n", fdata->refcount);
	main_v->frefdata = fdata;
}

void fref_cleanup(Tbfwin *bfwin) {
	DEBUG_MSG("fref_cleanup, started for bfwin=%p, refcount at %p\n", bfwin,
			  FREFDATA(main_v->frefdata)->refcount);
	fref_unload_all(FREFGUI(bfwin->fref)->tree, FREFDATA(main_v->frefdata)->store,bfwin);
/*	g_hash_table_destroy(FREFDATA(main_v->frefdata)->refcount); */
/* Ok I'll not free search dictionary too ... O.S. */
	FREFGUI(bfwin->fref)->tree = NULL;
	FREFGUI(bfwin->fref)->argtips = NULL;
	g_free (bfwin->fref);
	bfwin->fref = NULL;
}



GtkWidget *fref_gui(Tbfwin * bfwin)
{
	GtkWidget *scroll, *box, *pane, *box2, *btn1,*box3;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	Tfref_data *fdata = FREFDATA(main_v->frefdata);
	GtkTreeIter it;
	GtkWidget *item,*menu,*wdg;
	GtkTextBuffer *buff;

	bfwin->fref = g_new0(Tfref_gui, 1);

	pane = gtk_vpaned_new();
	box = gtk_vbox_new(FALSE, 1);
	box2 = gtk_hbox_new(FALSE, 1);
	box3 = gtk_hbox_new(FALSE, 1);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);
	FREFGUI(bfwin->fref)->infoscroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(FREFGUI(bfwin->fref)->infoscroll),
								   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	FREFGUI(bfwin->fref)->tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(fdata->store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), FALSE);
	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", cell, "text", STR_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), column);
	gtk_container_add(GTK_CONTAINER(scroll), FREFGUI(bfwin->fref)->tree);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), FALSE);
	
	FREFGUI(bfwin->fref)->row_collapse_signal = g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->tree), "row-collapsed",
					 G_CALLBACK(frefcb_row_collapsed), bfwin);
	FREFGUI(bfwin->fref)->row_expand_signal = g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->tree), "row-expanded",
					 G_CALLBACK(frefcb_row_expanded), bfwin);
	FREFGUI(bfwin->fref)->tree_signal = g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->tree), "button-press-event",
					 G_CALLBACK(frefcb_event_mouseclick), bfwin);

	gtk_widget_show(FREFGUI(bfwin->fref)->tree);
	gtk_widget_show(scroll);

	FREFGUI(bfwin->fref)->argtips = main_v->tooltips;
	
	FREFGUI(bfwin->fref)->infoview = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(FREFGUI(bfwin->fref)->infoview),GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(FREFGUI(bfwin->fref)->infoview),FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(FREFGUI(bfwin->fref)->infoview),FALSE);

	g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->tree), "cursor-changed",
					 G_CALLBACK(frefcb_cursor_changed), bfwin);

	buff = gtk_text_view_get_buffer(GTK_TEXT_VIEW(FREFGUI(bfwin->fref)->infoview));
	gdk_color_parse("#B71212",&(FREFGUI(bfwin->fref)->col_descr));
	gdk_color_parse("#4D4D4D",&(FREFGUI(bfwin->fref)->col_ap));
	gdk_color_parse("#4D4D4D",&(FREFGUI(bfwin->fref)->col_note));
	gdk_color_parse("#FFA500",&(FREFGUI(bfwin->fref)->col_example));
	gdk_color_parse("#84559D",&(FREFGUI(bfwin->fref)->col_link));
 	FREFGUI(bfwin->fref)->tag_descr = gtk_text_buffer_create_tag(buff,"color_descr","foreground-gdk",&FREFGUI(bfwin->fref)->col_descr,NULL);
 	FREFGUI(bfwin->fref)->tag_ap = gtk_text_buffer_create_tag(buff,"color_ap","foreground-gdk",&FREFGUI(bfwin->fref)->col_ap,NULL);
 	FREFGUI(bfwin->fref)->tag_note = gtk_text_buffer_create_tag(buff,"color_note","foreground-gdk",&FREFGUI(bfwin->fref)->col_note,NULL);
 	FREFGUI(bfwin->fref)->tag_example = gtk_text_buffer_create_tag(buff,"color_example","foreground-gdk",&FREFGUI(bfwin->fref)->col_example,NULL);
 	FREFGUI(bfwin->fref)->tag_link = gtk_text_buffer_create_tag(buff,"color_link","foreground-gdk",&FREFGUI(bfwin->fref)->col_link,NULL);
   gtk_widget_modify_font(FREFGUI(bfwin->fref)->infoview,pango_font_description_from_string("Sans 10"));


	FREFGUI(bfwin->fref)->infocheck = gtk_toggle_button_new();
	gtk_tooltips_set_tip(FREFGUI(bfwin->fref)->argtips,FREFGUI(bfwin->fref)->infocheck , _("Show info window"), "");
	gtk_container_add(GTK_CONTAINER(FREFGUI(bfwin->fref)->infocheck), new_pixmap(160));
	GTK_WIDGET_SET_FLAGS(FREFGUI(bfwin->fref)->infocheck, GTK_CAN_DEFAULT);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FREFGUI(bfwin->fref)->infocheck), TRUE);
   g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->infocheck), "toggled",
					 G_CALLBACK(frefcb_infocheck_toggled), bfwin);
					 
	btn1 = bf_allbuttons_backend(NULL, FALSE, 161, G_CALLBACK(fref_insert_lcb), bfwin);
	gtk_tooltips_set_tip(FREFGUI(bfwin->fref)->argtips, btn1, _("Insert"), "");

	FREFGUI(bfwin->fref)->btn_show = gtk_button_new();
	gtk_tooltips_set_tip(FREFGUI(bfwin->fref)->argtips,FREFGUI(bfwin->fref)->btn_show , _("Show"), "");
	gtk_container_add(GTK_CONTAINER(FREFGUI(bfwin->fref)->btn_show), new_pixmap(107));	
	g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->btn_show), "clicked",
					 G_CALLBACK(frefcb_showbtn_clicked), bfwin);	
	GTK_WIDGET_SET_FLAGS(FREFGUI(bfwin->fref)->btn_show, GTK_CAN_DEFAULT);				 

	FREFGUI(bfwin->fref)->menu_show = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(FREFGUI(bfwin->fref)->menu_show),_("Show"));
	FREFGUI(bfwin->fref)->m_descr = gtk_check_menu_item_new_with_label(_("Description"));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_descr),TRUE);
	g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->m_descr), "toggled",
					 G_CALLBACK(frefcb_showitem_activated), bfwin);	
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_show),FREFGUI(bfwin->fref)->m_descr);
	FREFGUI(bfwin->fref)->m_ap = gtk_check_menu_item_new_with_label(_("Attributes/Parameters"));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_ap),TRUE);	
	g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->m_ap), "toggled",
					 G_CALLBACK(frefcb_showitem_activated), bfwin);
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_show),FREFGUI(bfwin->fref)->m_ap);
	FREFGUI(bfwin->fref)->m_note = gtk_check_menu_item_new_with_label(_("Notes"));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_note),TRUE);	
	g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->m_note), "toggled",
					 G_CALLBACK(frefcb_showitem_activated), bfwin);	
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_show),FREFGUI(bfwin->fref)->m_note);
	FREFGUI(bfwin->fref)->m_example = gtk_check_menu_item_new_with_label(_("Examples"));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_example),TRUE);	
	g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->m_example), "toggled",
					 G_CALLBACK(frefcb_showitem_activated), bfwin);	
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_show),FREFGUI(bfwin->fref)->m_example);
	FREFGUI(bfwin->fref)->m_link = gtk_check_menu_item_new_with_label(_("Links"));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_link),TRUE);	
	g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->m_link), "toggled",
					 G_CALLBACK(frefcb_showitem_activated), bfwin);	
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_show),FREFGUI(bfwin->fref)->m_link);	
	gtk_widget_show_all(FREFGUI(bfwin->fref)->menu_show);


	FREFGUI(bfwin->fref)->btn_pref = gtk_button_new();
	gtk_tooltips_set_tip(FREFGUI(bfwin->fref)->argtips,FREFGUI(bfwin->fref)->btn_pref , _("Settings"), "");
	gtk_container_add(GTK_CONTAINER(FREFGUI(bfwin->fref)->btn_pref), new_pixmap(1002));	
	g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->btn_pref), "clicked",
					 G_CALLBACK(frefcb_prefbtn_clicked), bfwin);	
	GTK_WIDGET_SET_FLAGS(FREFGUI(bfwin->fref)->btn_pref, GTK_CAN_DEFAULT);				 
	FREFGUI(bfwin->fref)->menu_pref = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(FREFGUI(bfwin->fref)->menu_pref),_("Settings"));
	item = gtk_menu_item_new_with_label(_("Rescan reference files"));
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_pref),item);
	g_signal_connect(GTK_OBJECT(item), "activate", G_CALLBACK(fref_popup_menu_rescan_lcb),NULL);
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_pref),gtk_separator_menu_item_new());
	item = gtk_menu_item_new_with_label(_("Set view font"));
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_pref),item);
	g_signal_connect(GTK_OBJECT(item), "activate", G_CALLBACK(fref_setfont_lcb),bfwin);
	item = gtk_menu_item_new_with_label(_("Set view colors"));
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_pref),item);
	g_signal_connect(GTK_OBJECT(item), "activate", G_CALLBACK(fref_setcolors_lcb),bfwin);
	
	gtk_widget_show_all(FREFGUI(bfwin->fref)->menu_pref);

	FREFGUI(bfwin->fref)->btn_edit = gtk_button_new();
	gtk_tooltips_set_tip(FREFGUI(bfwin->fref)->argtips,FREFGUI(bfwin->fref)->btn_edit , _("Edit"), "");
	gtk_container_add(GTK_CONTAINER(FREFGUI(bfwin->fref)->btn_edit), new_pixmap(113));	
	g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->btn_edit), "clicked",
					 G_CALLBACK(frefcb_editbtn_clicked), bfwin);	
	GTK_WIDGET_SET_FLAGS(FREFGUI(bfwin->fref)->btn_edit, GTK_CAN_DEFAULT);				 
	FREFGUI(bfwin->fref)->menu_edit = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),_("Edit"));
	item = gtk_menu_item_new_with_label(_("New library"));
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),item);
	g_signal_connect(GTK_OBJECT(item), "activate", G_CALLBACK(frefcb_new_library),bfwin);
	item = gtk_menu_item_new_with_label(_("Delete library"));
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),item);
	g_signal_connect(GTK_OBJECT(item), "activate", G_CALLBACK(frefcb_delete_library),bfwin);
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),gtk_separator_menu_item_new());
	item = gtk_menu_item_new_with_label(_("New entity"));
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),item);
	menu = gtk_menu_new();
	wdg = gtk_menu_item_new_with_label(_("Note"));
	g_signal_connect(GTK_OBJECT(wdg), "activate", G_CALLBACK(frefcb_new_note),bfwin);
	gtk_menu_append(GTK_MENU(menu),wdg);	
	wdg = gtk_menu_item_new_with_label(_("Group"));
	gtk_menu_append(GTK_MENU(menu),wdg);	
	wdg = gtk_menu_item_new_with_label(_("Reusable definition"));
	gtk_menu_append(GTK_MENU(menu),wdg);	
	wdg = gtk_menu_item_new_with_label(_("Element"));
	gtk_menu_append(GTK_MENU(menu),wdg);		
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),menu);


	
	item = gtk_menu_item_new_with_label(_("Delete entity"));
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),item);
	item = gtk_menu_item_new_with_label(_("Modify entity"));
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),item);
	gtk_widget_show_all(FREFGUI(bfwin->fref)->menu_edit);

	FREFGUI(bfwin->fref)->sentry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(box3), gtk_label_new(_("Find")), FALSE, TRUE, 3);
	gtk_box_pack_start(GTK_BOX(box3), FREFGUI(bfwin->fref)->sentry, TRUE, TRUE, 3);
	g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->sentry), "key-press-event",
					 G_CALLBACK(fref_search_keypress), bfwin);	

	gtk_box_pack_start(GTK_BOX(box2), FREFGUI(bfwin->fref)->infocheck, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(box2), FREFGUI(bfwin->fref)->btn_show, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), FREFGUI(bfwin->fref)->btn_pref, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), FREFGUI(bfwin->fref)->btn_edit, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), btn1, FALSE, TRUE, 0);

	
	gtk_container_add(GTK_CONTAINER(FREFGUI(bfwin->fref)->infoscroll),
										  FREFGUI(bfwin->fref)->infoview);
	gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), box2, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), box3, FALSE, TRUE, 0);
	gtk_paned_pack1(GTK_PANED(pane), box, TRUE, FALSE);
	gtk_paned_pack2(GTK_PANED(pane), FREFGUI(bfwin->fref)->infoscroll, TRUE, TRUE);
	gtk_widget_show_all(pane);

	if (!gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(fdata->store), &it, NULL, 0))
		fill_toplevels(fdata, FALSE);
	return pane;
}

