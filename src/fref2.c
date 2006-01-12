/* Bluefish HTML Editor
 * fref2.c - Library aka Function reference (second implementation)
 * 
 * Copyright (C) 2005 Oskar Świda swida@aragorn.pb.bialystok.pl
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* #define DEBUG */

#include <gtk/gtk.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xmlwriter.h>

#include "bluefish.h"
#include "bf_lib.h"
/* #include "char_table.h" */ /* Compilation problem */
#include "dialog_utils.h"
#include "document.h"
#include "fref2.h"
#include "gtk_easy.h"
#include "pixmap.h"
#include "rcfile.h"				/* array_from_arglist() */
#include "stringlist.h"

enum {
	VISIBLE_COLUMN,
	PIXMAP_COLUMN,
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

enum {
FREF_INP_ENTRY,
FREF_INP_LIST,
FREF_INP_LIST_ENTRY,
FREF_INP_COLOUR,
FREF_INP_COLOUR_LIST,
FREF_INP_FILE
};

typedef struct {
	GdkPixbuf *icon_ro; /* 162 */
	GdkPixbuf *icon_rw; /* 170 */
	GdkPixbuf *icon_hide; /* 160 */
	GdkPixbuf *icon_dialog2; /* 161 */
	GdkPixbuf *icon_group; /* 163 */
	GdkPixbuf *icon_note; /* 164 */
	GdkPixbuf *icon_function; /* 165 */
	GdkPixbuf *icon_tag; /* 166 */
	GdkPixbuf *icon_var; /* 167 */
	GdkPixbuf *icon_css; /* 168 */
	GdkPixbuf *icon_snippet; /* 169 */
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
	GtkWidget *menu_show,/**menu_pref,*/*menu_edit;
	GtkWidget *m_descr,*m_ap,*m_note,*m_example,*m_link;
	GtkWidget *sentry;
	GtkWidget *btn_show,*btn_pref,*btn_edit;
	GdkColor col_descr,col_ap,col_note,col_example,col_link;
	/* GtkTextTag *tag_descr,*tag_ap,*tag_note,*tag_example,*tag_link; */
	Tbfwin *bfwin;
	guint tree_signal, row_collapse_signal, row_expand_signal;
	gpointer prg; /* progress dialog */
	GtkTreeModelFilter *view_filter;
} Tfref_gui;
#define FREFGUI(var) ((Tfref_gui *)(var))

/*
*     NOTE: all params,attrs, examples etc. are inserted into common structure - whether they are defined
*               by <def> or <property> - it simplifies freeing memory.
*/

typedef struct {
	GHashTable *dictionary;
	GHashTable *commons, *elements;
	GHashTable *proplists;
	guchar *description;
	gboolean readonly;
} Tfref_info;
#define FREFINFO(var) ((Tfref_info *)(var))


enum {
	FREF_EL_FUNCTION,
	FREF_EL_TAG,
	FREF_EL_CSSPROP,
	FREF_EL_CSSSELECT,
	FREF_EL_VAR,
	FREF_EL_NOTE,
	FREF_EL_ATTR,
	FREF_EL_PARAM,
	FREF_EL_RETURN,
	FREF_EL_DESCR,
	FREF_EL_EXAMPLE,
	FREF_EL_VALUES,
	FREF_EL_SNIPPET,
	FREF_EL_GROUP,
	FREF_EL_REF,
	FREF_EL_PROP,
	FREF_EL_LINK,
	FREF_EL_DEPEND,
	FREF_EL_PARENTS,
	FREF_EL_PROPLIST,
	FREF_EL_NUMCOLS
};

enum {FID_ATTRIBUTE,FID_VAR,FID_CSSPROP,FID_CHILDREN,FID_DESCRIPTION,
FID_DEPENDENCY,FID_DEFAULT,FID_DEF,FID_DEFTYPE,FID_ELEMENT,FID_EXAMPLE,
FID_FUNCTION,FID_GROUP,FID_INSERT,FID_ID,FID_KIND,FID_LINK,FID_NAME,FID_NOTE,
FID_PROPERTIES,FID_PROPERTY,FID_PARAMETER,FID_PARENTS,FID_REF,FID_RETURN,
FID_REQUIRED,FID_SNIPPET,FID_TITLE,FID_TAG,FID_TYPE,FID_INPUT,FID_VALUES,FID_VERSION,
FID_COMPACT,FID_CSSSELECT,FID_PROPLIST};

static char *fref_compact[] = {"a","v0","c1","c2","d","d1","d2","d3","d4","e","e1","f","g","i","i1","k","l",
													 "n","n1","p","p1","p2","p3","r","r1","r2",	"s","t","t1","t2","i2","v1","v2","x","c3","p4"};
static char *fref_full[] = {"attribute","var","css_property","children","description","dependency", 
"default","def","deftype","element","example","function","group","insert","id","kind","link","name","note","properties",
"property","parameter","parents","ref","return","required","snippet","title","tag","type","input","values","version",
"compact","css_selector","proplist"};

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
	guchar *type;      	/* this is not for markup, but usable for return/params */
	gboolean required;
	gpointer vlist;   /* here we have pointer not values, because it can be common defined list of values */
	gpointer vlist2; /* additional list of values */
	guchar *def;       /* default value */
	gpointer description;
	GtkWidget *dlgitem; /* widget for data retrieve */
	gint input_type;			/* type of control for input */
	GList *input_params;
	gpointer aux;        /* auxiliary pointer */
} Tfref_property;
#define FREFPROPERTY(var) ((Tfref_property *)(var))


typedef struct {
	gint etype;								/* type of element */
	guchar *name;   						
	gpointer description; 
	GList *properties;				 /* attributes/params/variables/fields */
	GList *notes;
	guchar *insert;
} Tfref_element;
#define FREFELEMENT(var) ((Tfref_element *)(var))
    

guchar *tmps;
static void fill_toplevels(Tfref_data * fdata, gboolean empty_first);
static void frefcb_cursor_changed(GtkTreeView * treeview, Tbfwin * bfwin);
static void fref_search(Tbfwin * bfwin,const gchar *phrase);

/* *********************************************************************
*****************************     PARSING  *******************************
***********************************************************************/

static GRand *fref_rand; /* for unique names */

static gchar *fref_unique_name(gint pfx) {
	gchar *ret;	
	ret = g_strdup_printf("___%d_%d__",pfx,g_rand_int(fref_rand));
	return ret;
}

static Tfref_record *fref_insert_into_commons(Tfref_info *info,gchar *name,gint tp,gpointer data, gboolean def) {
	Tfref_record *rec = g_new0(Tfref_record,1);
	gpointer ptr;
	rec->etype = tp;
	rec->data = data;
	if (def)
		ptr = g_hash_table_lookup(info->commons,name);
	else
		ptr = g_hash_table_lookup(info->elements,name);	
	if (ptr) {
		g_warning("Entity '%s' already exists!",name);
		g_free(rec);
		return NULL;
	}
	if (def)
		g_hash_table_insert(info->commons,name,rec);
	else
		g_hash_table_insert(info->elements,name,rec);	
	return rec;
}

static Tfref_info *fref_refinfo_for_current(Tbfwin *bfwin)
{
	GtkTreePath *path;
	GtkTreeViewColumn *col;
	GtkTreeIter auxit;
	GValue *val;
	Tfref_record *rec=NULL;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), &path, &col);
	if (path==NULL) return NULL;
	while (gtk_tree_path_get_depth(path) > 1 && gtk_tree_path_up(path));
	gtk_tree_model_get_iter(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),&auxit,path);
	val = g_new0(GValue, 1);
	gtk_tree_model_get_value(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),&auxit,PTR_COLUMN, val);
	if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) {
				rec =  (Tfref_record*)g_value_peek_pointer(val);
				g_value_unset(val);
				g_free(val);
				if ( rec->etype == FREF_EL_REF)
					return FREFINFO(rec->data);
				else
					return NULL;	
	}
	g_value_unset(val);
	g_free(val);	
	return NULL;
}

static gboolean fref_ro_for_current(Tbfwin *bfwin)
{
	GtkTreePath *path;
	GtkTreeViewColumn *col;
	GtkTreeIter auxit;
	gboolean ro=TRUE;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), &path, &col);
	if (path==NULL) return TRUE;
	while (gtk_tree_path_get_depth(path) > 1 && gtk_tree_path_up(path));
	gtk_tree_model_get_iter(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),&auxit,path);
	gtk_tree_model_get(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),&auxit,VISIBLE_COLUMN, &ro,-1);
	return ro;
}



Tfref_property *fref_parse_property(xmlNodePtr node,xmlDocPtr doc,Tfref_info *info,gboolean common) {
		Tfref_property  *p = g_new0(Tfref_property,1);  
		guchar *auxs2,*auxs3;  
		xmlNodePtr auxn;
		gpointer auxp;


		if ( common) {
			auxs2 = xmlGetProp(node, (const xmlChar *) fref_names[FID_ID]);
			if (auxs2 == NULL) {
					g_warning("Definition has to have id attribute!");
					g_free(p);
					return NULL;
			}		
			p->id = auxs2;
			p->name = xmlGetProp(node, (const xmlChar *) fref_names[FID_NAME]);
			if (p->name==NULL)
			    p->name = p->id;
		}
		else {
			p->name = xmlGetProp(node, (const xmlChar *) fref_names[FID_NAME]);
			p->id = "";
		}

		auxs2 = xmlGetProp(node, (const xmlChar *) fref_names[FID_KIND]);
		if ( auxs2 == NULL ) {
			g_warning("Kind of property not determined ");
			g_free(p);
			return NULL;
		}	
		if (xmlStrcmp(auxs2, (const xmlChar *) fref_names[FID_LINK]) == 0)
			p->ptype = FREF_EL_LINK;
		else if (xmlStrcmp(auxs2,(const xmlChar *) fref_names[FID_EXAMPLE]) == 0)	
			p->ptype = FREF_EL_EXAMPLE;		
		else if (xmlStrcmp(auxs2, (const xmlChar *) fref_names[FID_DEPENDENCY]) == 0)	
			p->ptype = FREF_EL_DEPEND;		
		else if (xmlStrcmp(auxs2, (const xmlChar *) fref_names[FID_ATTRIBUTE]) == 0)	
			p->ptype = FREF_EL_ATTR;		
		else if (xmlStrcmp(auxs2, (const xmlChar *) fref_names[FID_PARAMETER]) == 0)	
			p->ptype = FREF_EL_PARAM;		
		else if (xmlStrcmp(auxs2, (const xmlChar *) fref_names[FID_RETURN]) == 0)	
			p->ptype = FREF_EL_RETURN;		
		else if (xmlStrcmp(auxs2, (const xmlChar *) fref_names[FID_PARENTS]) == 0)	
			p->ptype = FREF_EL_PARENTS;		
		else {
			g_warning("Uknown property kind ");
			xmlFree(auxs2);
			g_free(p);
			return NULL;
		}	
		xmlFree(auxs2);
		
		p->type = xmlGetProp(node, (const xmlChar *) fref_names[FID_TYPE]);
		
		auxs2 = xmlGetProp(node, (const xmlChar *) fref_names[FID_REQUIRED]);		
		if ( auxs2 && (xmlStrcmp(auxs2, (const xmlChar *)  "1") == 0)) 
			p->required = TRUE;
		else
			p->required = FALSE;	
			
		if (auxs2) xmlFree(auxs2);	
		
		auxs2 = xmlGetProp(node, (const xmlChar *) fref_names[FID_INPUT]);		
		p->input_type = FREF_INP_ENTRY;
		if (auxs2)
		{
			if ( xmlStrcmp(auxs2, (const xmlChar *) "list") == 0 )
				p->input_type = FREF_INP_LIST;
			else if 	( xmlStrcmp(auxs2, (const xmlChar *) "list+entry") == 0 )
				p->input_type = FREF_INP_LIST_ENTRY;
			else if 	( xmlStrcmp(auxs2, (const xmlChar *) "color") == 0 || xmlStrcmp(auxs2, (const xmlChar *) "colour") == 0)	
				p->input_type = FREF_INP_COLOUR;
			else if 	( xmlStrcmp(auxs2, (const xmlChar *) "color+list") == 0 || xmlStrcmp(auxs2, (const xmlChar *) "colour+list") == 0)		
				p->input_type = FREF_INP_COLOUR_LIST;
			else if 	( xmlStrcmp(auxs2, (const xmlChar *) "file") == 0 )
				p->input_type = FREF_INP_FILE;				
			xmlFree(auxs2);
		}
			
		p->def = xmlGetProp(node, (const xmlChar *) fref_names[FID_DEFAULT]);
		
		auxn = node->xmlChildrenNode;		
		while ( auxn ) {
			if ( xmlStrcmp(auxn->name, (const xmlChar *) fref_names[FID_VALUES]) == 0 ) {
						auxs3 = xmlGetProp(auxn, (const xmlChar *) fref_names[FID_REF]);
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
			        			auxs3 = xmlNodeListGetString(doc,auxn->xmlChildrenNode,1);
			        			if (auxs3 && strcmp(g_strstrip(auxs3),"")!=0)
			        			{
						      	Tfref_record *rec = fref_insert_into_commons(info, auxs2,FREF_EL_VALUES,g_strstrip(auxs3),FALSE);
						      	if (rec) {
						      		p->vlist = rec->data;
						      	}	
						      }	
						      else
						       g_free(auxs2);
						      /*g_free(auxs2);*/
			        }
			}        
			else if ( xmlStrcmp(auxn->name, (const xmlChar *) fref_names[FID_DESCRIPTION]) == 0 ) {
						auxs3 = xmlGetProp(auxn, (const xmlChar *) fref_names[FID_REF]);
						if ( auxs3 != NULL ) {
							auxp = g_hash_table_lookup(info->commons,auxs3);
							if ( auxp == NULL ) {
								g_warning("Description definition requested (%s) but not found",auxs3);
								xmlFree(auxs3);
								g_free(p);
								return NULL;
						   }
						   p->description = FREFRECORD(auxp)->data;
						   xmlFree(auxs3);
			        } else {
			        			auxs2 = fref_unique_name(xmlGetLineNo(auxn));
						      Tfref_record *rr = fref_insert_into_commons(info, auxs2,FREF_EL_DESCR,xmlNodeListGetString(doc,auxn->xmlChildrenNode,1),FALSE);
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

	auxs = xmlGetProp(node, (const xmlChar *) fref_names[FID_REF]);
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
	
	p->name = xmlGetProp(node, (const xmlChar *) fref_names[FID_NAME]);
	
	if ( p->name == NULL) {
		g_warning("Element has to have a name!");
		g_free(p);
		return NULL;
	}

	auxs = xmlGetProp(node, (const xmlChar *) fref_names[FID_KIND]);
	if ( auxs == NULL ) {
			g_warning("Kind of element not determined ");
			g_free(p);
			return NULL;
	}	
	
	if (xmlStrcmp(auxs, (const xmlChar *) fref_names[FID_FUNCTION]) == 0)
			p->etype = FREF_EL_FUNCTION;
		else if (xmlStrcmp(auxs, (const xmlChar *) fref_names[FID_TAG]) == 0)	
			p->etype = FREF_EL_TAG;		
		else if (xmlStrcmp(auxs, (const xmlChar *) fref_names[FID_VAR]) == 0)	
			p->etype = FREF_EL_VAR;		
		else if (xmlStrcmp(auxs, (const xmlChar *) fref_names[FID_CSSPROP]) == 0)	
			p->etype = FREF_EL_CSSPROP;		
		else if (xmlStrcmp(auxs, (const xmlChar *) fref_names[FID_CSSSELECT]) == 0)	
			p->etype = FREF_EL_CSSSELECT;					
		else if (xmlStrcmp(auxs, (const xmlChar *) fref_names[FID_SNIPPET]) == 0)	
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
		if ( xmlStrcmp(auxn->name, (const xmlChar *) fref_names[FID_DESCRIPTION]) == 0) { /* description */
			auxs = xmlGetProp(auxn, (const xmlChar *) fref_names[FID_REF]);
			if ( auxs ) {
				auxp = g_hash_table_lookup(info->commons,auxs);
				if ( auxp == NULL ) {
					g_warning("Description definition requested (%s) but not found",auxs);
					xmlFree(auxs);
					g_free(p);
					return NULL;
				}
				p->description = FREFRECORD(auxp)->data;
				xmlFree(auxs);
			} else {
				auxs = fref_unique_name(xmlGetLineNo(auxn));
				Tfref_record *rec = fref_insert_into_commons(info,auxs,FREF_EL_DESCR,xmlNodeListGetString(doc,auxn->xmlChildrenNode,1),FALSE);
				if ( rec ) {
					p->description = rec->data;
				}	
				/* g_free(auxs);*/
			}
		} /* description */
		else if ( xmlStrcmp(auxn->name, (const xmlChar *) fref_names[FID_PROPERTIES]) == 0) { /* properties */
				xmlNodePtr nptr = auxn->xmlChildrenNode;
				Tfref_property *apv; 				
				while ( nptr  ) {
				 if (xmlStrcmp(nptr->name, (const xmlChar *) fref_names[FID_PROPERTY])==0) 
				 {
							Tfref_record *rec=NULL;
							auxs = xmlGetProp(nptr, (const xmlChar *) fref_names[FID_REF]);
							if ( auxs ) {
								auxp = g_hash_table_lookup(info->commons,auxs);
								if ( auxp == NULL ) {
									g_warning("Property definition requested (%s) but not found",auxs);
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
													rec = fref_insert_into_commons(info,auxs,apv->ptype,apv,FALSE);
												}	
							}		
							if ( apv && rec )
								p->properties = g_list_append(p->properties,apv);			
				 }		/* is property */
				 else
 				 if (xmlStrcmp(nptr->name, (const xmlChar *) fref_names[FID_PROPLIST])==0)  /* property list */
				 {
				 	auxs = xmlGetProp(nptr, (const xmlChar *) fref_names[FID_REF]);
				 	if ( auxs != NULL )
				 	{
				 		gpointer props = g_hash_table_lookup(info->proplists,auxs);
				 		if ( props )
				 		{
				 			GList *ll = g_list_first(props);
				 			while (ll)
				 			{
				 				p->properties = g_list_append(p->properties,ll->data);	
				 				ll = g_list_next(ll);
				 			}
				 		} else g_warning("Property list %s required but not found. ",auxs);	
				 	} else g_warning("Property list should have 'ref' attribute. ");	
				 	xmlFree(auxs);
				 } /* is property list */
				 nptr = nptr->next;
				}
		} /* properties */
		else if ( xmlStrcmp(auxn->name, (const xmlChar *) fref_names[FID_NOTE]) == 0) { /* note */
			Tfref_note *note = g_new0(Tfref_note,1);
			note->title = xmlGetProp(auxn, (const xmlChar *) fref_names[FID_TITLE]);
			note->text = xmlNodeListGetString(doc,auxn->xmlChildrenNode,1);
			p->notes = g_list_append(p->notes,note);
			auxs = fref_unique_name(xmlGetLineNo(auxn));
			fref_insert_into_commons(info,auxs,FREF_EL_NOTE,note,FALSE);			
		} /* note */
		else if ( xmlStrcmp(auxn->name, (const xmlChar *) fref_names[FID_INSERT]) == 0) { /* insert */
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
	if ( xmlStrcmp(node->name, (const xmlChar *) fref_names[FID_GROUP]) == 0 ) { /* group */
		Tfref_record *el;
		Tfref_note *n = g_new0(Tfref_note,1);
		
		auxs = fref_unique_name(xmlGetLineNo(node));
		el = fref_insert_into_commons(info,auxs,FREF_EL_GROUP,n,FALSE);	
		if ( el == NULL ) {
			g_free(auxs);
			g_free(n);
			return;
		}
		n->title = xmlGetProp(node, (const xmlChar *) fref_names[FID_NAME]);
		
		auxn = node->xmlChildrenNode;
		while ( auxn ) {
			if ( xmlStrcmp(auxn->name, (const xmlChar *) fref_names[FID_DESCRIPTION]) == 0 ) {
				auxs = xmlGetProp(auxn, (const xmlChar *) fref_names[FID_REF]);
				if ( auxs ) {
					auxp = g_hash_table_lookup(info->commons,auxs);
					if ( auxp == NULL ) {
						g_warning("Description definition requested (%s) but not found",auxs);
						xmlFree(auxs);
						g_free(el);
						g_free(n);
						return;
					}
					xmlFree(auxs);
					n->text = FREFRECORD(auxp)->data;
				} else {
					auxs = fref_unique_name(xmlGetLineNo(auxn));
					Tfref_record *rec = fref_insert_into_commons(info,auxs,FREF_EL_DESCR,xmlNodeListGetString(doc,auxn->xmlChildrenNode,1),FALSE);
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
		gtk_tree_store_set(store, &auxit, PIXMAP_COLUMN,FREFDATA(main_v->frefdata)->icon_group,
		STR_COLUMN,n->title, FILE_COLUMN, NULL, PTR_COLUMN, el, -1);
		
		auxn = node->xmlChildrenNode;
		
		while (auxn!=NULL ) {
			fref_parse_node(auxn,tree,store,&auxit,info,doc,bfwin);
			auxn = auxn->next;
		}
	} /* group */   	              						  
	else 	if ( xmlStrcmp(node->name, (const xmlChar *) fref_names[FID_NOTE]) == 0 ) { /* global note */
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(store),parent);
		/*if (gtk_tree_path_get_depth(path) == 1) { */
			Tfref_note *n = g_new0(Tfref_note,1);
			Tfref_record *el;
			n->title = xmlGetProp(node, (const xmlChar *) fref_names[FID_TITLE]);
			n->text = xmlNodeListGetString(doc,node->xmlChildrenNode,1);

			auxs = fref_unique_name(xmlGetLineNo(node));
			el = fref_insert_into_commons(info,auxs,FREF_EL_NOTE,n,FALSE);
			if ( el ) {
				gtk_tree_store_append(store, &auxit, parent);
				gtk_tree_store_set(store, &auxit, PIXMAP_COLUMN,FREFDATA(main_v->frefdata)->icon_note,
				STR_COLUMN,n->title, FILE_COLUMN, NULL, PTR_COLUMN,el, -1);
			} else {
				xmlFree(n->title);
				xmlFree(n->text);
				g_free(n);
			}	
		/*}*/
	} /* global note */
	else 	if ( xmlStrcmp(node->name, (const xmlChar *) fref_names[FID_DEF]) == 0 ) { /* definition */
		auxs = xmlGetProp(node, (const xmlChar *) fref_names[FID_ID]);
		if ( auxs == NULL ) {
			g_warning("Definition has to have an id");
			return;
		}
		auxs2 = xmlGetProp(node, (const xmlChar *) fref_names[FID_DEFTYPE]);		
		if ( auxs2 == NULL ) {
			g_warning("Definition type not defined");
			return;
		}
		if ( xmlStrcmp(auxs2, (const xmlChar *) fref_names[FID_PROPERTY]) == 0 ) {			/* property definition */
			Tfref_property *pp = fref_parse_property(node,doc,info,TRUE);
			if ( pp ) {
				if ( fref_insert_into_commons(info,g_strdup(auxs),pp->ptype,pp,TRUE) == NULL) {				
					g_free(pp); /* Improve! */
					xmlFree(auxs);
				}
			}	
		}
		else if ( xmlStrcmp(auxs2, (const xmlChar *) fref_names[FID_DESCRIPTION]) == 0 ) {		/* descr definition */
			fref_insert_into_commons(info,g_strdup(auxs),FREF_EL_DESCR,xmlNodeListGetString(doc,node->xmlChildrenNode,1),TRUE);	
			xmlFree(auxs);
		} 
		else if ( xmlStrcmp(auxs2, (const xmlChar *) fref_names[FID_VALUES]) == 0 ) {		/* values definition */
			fref_insert_into_commons(info,g_strdup(auxs),FREF_EL_VALUES,g_strstrip(xmlNodeListGetString(doc,node->xmlChildrenNode,1)),TRUE);	
			xmlFree(auxs);
		} 		
		else if ( xmlStrcmp(auxs2, (const xmlChar *) fref_names[FID_PROPLIST]) == 0 ) {		/* property list definition */
			GList *plst=NULL;
			Tfref_record *rec;
			auxn = node->xmlChildrenNode;
			while ( auxn )
			{
				if ( xmlStrcmp(auxn->name, (const xmlChar *) fref_names[FID_PROPERTY]) == 0 )
				{
					Tfref_property *pp = fref_parse_property(auxn,doc,info,FALSE);
					if ( pp )
					{
						rec = fref_insert_into_commons(info,fref_unique_name(xmlGetLineNo(auxn)),pp->ptype,pp,FALSE);
						if ( rec )
						{
							plst = g_list_append(plst,pp);
						}
					}
				}
				auxn = auxn->next;
			}/* while */
			g_hash_table_replace(info->proplists,auxs,plst);
		} 				
		else {
			g_warning("Uknown definition type");
			xmlFree(auxs2);
			return;
		}
		xmlFree(auxs2);
	} /* definition */	
	else 	if ( xmlStrcmp(node->name, (const xmlChar *) fref_names[FID_ELEMENT]) == 0 ) { /* element */
		Tfref_element *el = fref_parse_element(node,doc,info,bfwin);
		Tfref_record *rec; 
		if ( el ) {
							rec = fref_insert_into_commons(info,g_strdup(el->name),el->etype,el,FALSE);	
							if ( rec ) { 
								gtk_tree_store_append(store, &auxit, parent);
								gtk_tree_store_set(store, &auxit, STR_COLUMN,el->name, FILE_COLUMN, NULL, 
								                               PTR_COLUMN,rec, -1);			
								switch (el->etype)
								{
									case FREF_EL_FUNCTION:gtk_tree_store_set(store, &auxit, PIXMAP_COLUMN,
									FREFDATA(main_v->frefdata)->icon_function, -1);break;
									case FREF_EL_TAG:gtk_tree_store_set(store, &auxit, PIXMAP_COLUMN,
									FREFDATA(main_v->frefdata)->icon_tag, -1);break;
									case FREF_EL_VAR:gtk_tree_store_set(store, &auxit, PIXMAP_COLUMN,
									FREFDATA(main_v->frefdata)->icon_var, -1);break;
									case FREF_EL_CSSPROP:
									case FREF_EL_CSSSELECT:
									gtk_tree_store_set(store, &auxit, PIXMAP_COLUMN,
									FREFDATA(main_v->frefdata)->icon_css, -1);break;
									case FREF_EL_SNIPPET:gtk_tree_store_set(store, &auxit, PIXMAP_COLUMN,
									FREFDATA(main_v->frefdata)->icon_snippet, -1);break;
								}                               
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
	gint ret;

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
	ret = result->nodesetval->nodeNr;
	xmlXPathFreeObject(result);
	return ret;
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
	if ( xmlStrcmp(cur->name, (const xmlChar *) "ref") != 0  && xmlStrcmp(cur->name, (const xmlChar *) "r") != 0) {
		g_warning(_("Bad root element in reference file %s\nShould be <ref> or <r> and found %s"),filename,cur->name);
		xmlFreeDoc(doc);
		return NULL;
	}
	if (xmlStrcmp(cur->name, (const xmlChar *) "ref")==0)
		ret = xmlGetProp(cur, (const xmlChar *) "name");
	else
		ret = xmlGetProp(cur, (const xmlChar *) "n");	
   xmlFreeDoc(doc);
   return ret;
}

static gboolean reference_file_known(gchar * path)
{
	GList *tmplist = g_list_first(main_v->globses.reference_files);
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
	/* BUG: deprecated reference files are never removed from the list !! */
	const gchar *filename;
	GError *error = NULL;
	gchar *tofree;
	GPatternSpec *ps = g_pattern_spec_new("bflib_*.xml"); 
	GPatternSpec *ps2 = g_pattern_spec_new("bflib_*.xml.gz"); /* also xml.gz files */
	GDir *gd = g_dir_open(dir, 0, &error);
	filename = g_dir_read_name(gd);
	while (filename) {
		if (g_pattern_match(ps, strlen(filename), filename, NULL) ||
		    g_pattern_match(ps2, strlen(filename), filename, NULL)) {
			gchar *path = g_strconcat(dir, filename, NULL);
			DEBUG_MSG("fref_rescan_dir, filename %s has a match!\n", filename);
			if (!reference_file_known(path)) {
				tofree = fref_load_refname(path);
				DEBUG_MSG("fref_rescan_dir, adding %s:%s to list\n",tofree,path);
				main_v->globses.reference_files =
					g_list_append(main_v->globses.reference_files,
								  array_from_arglist(tofree, path, NULL));
				g_free(tofree);
			}
			g_free(path);
		}
		filename = g_dir_read_name(gd);
	}
	g_dir_close(gd);
	g_pattern_spec_free(ps);
	g_pattern_spec_free(ps2);
}


gboolean fref_load_from_file(gchar * filename, GtkWidget * tree, GtkTreeStore * store,
							  GtkTreeIter * parent, Tfref_info *info, Tbfwin *bfwin )
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	xmlChar *tmps;

	if (filename == NULL)
		return FALSE;
	info->dictionary =g_hash_table_new(g_str_hash, g_str_equal);
	info->commons = g_hash_table_new(g_str_hash, g_str_equal);
	info->elements = g_hash_table_new(g_str_hash, g_str_equal);
	info->proplists = g_hash_table_new(g_str_hash, g_str_equal);
	xmlLineNumbersDefault(1);
	doc = xmlParseFile(filename);
	if (doc==NULL) {
		message_dialog_new(bfwin->main_window, 
						 		 GTK_MESSAGE_ERROR, 
						 		 GTK_BUTTONS_CLOSE, 
						 		 _("Load error"), 
						 		 _("Cannot load reference file, perhaps it is not well-formed."));		
		g_hash_table_destroy(info->dictionary);
		g_hash_table_destroy(info->commons);
		g_hash_table_destroy(info->elements);
		g_hash_table_destroy(info->proplists);
		return FALSE;
	}	
	cur = xmlDocGetRootElement(doc);	
	if ( xmlStrcmp(cur->name, (const xmlChar *) "ref") != 0 &&  xmlStrcmp(cur->name, (const xmlChar *) "r") != 0 ) {
		
		message_dialog_new(bfwin->main_window, 
						 		 GTK_MESSAGE_ERROR, 
						 		 GTK_BUTTONS_CLOSE, 
						 		 _("Bad root element in reference file"), 
						 		 _(" \nShould be <ref> or <r> "));
		g_hash_table_destroy(info->dictionary);
		g_hash_table_destroy(info->commons);		
		g_hash_table_destroy(info->elements);
		xmlFreeDoc(doc);
		return FALSE;
	}
	if (xmlStrcmp(cur->name, (const xmlChar *) "ref") == 0) 
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

	tmps = xmlGetProp(cur, (const xmlChar *) fref_names[FID_VERSION]);
	if (xmlStrcmp(tmps, (const xmlChar *) "2") != 0 ) {
		message_dialog_new(bfwin->main_window, 
						 		 GTK_MESSAGE_WARNING, 
						 		 GTK_BUTTONS_CLOSE, 
						 		 _("Bad reference"), 
						 		 _("This is probably an old reference file. \nPlease update it to the new format."));
		g_hash_table_destroy(info->dictionary);
		g_hash_table_destroy(info->commons);		
		g_hash_table_destroy(info->elements);
		xmlFree(tmps);
		xmlFreeDoc(doc);
		return FALSE;		
	}
	if (tmps) xmlFree(tmps);
	tmps = xmlGetProp(cur, (const xmlChar *) fref_names[FID_DESCRIPTION]);
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
	return TRUE;
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
		case FREF_EL_VAR:
		case FREF_EL_SNIPPET:if (rec->data) {
					Tfref_element *el = FREFELEMENT(rec->data);					
					if (el->name)  xmlFree(el->name); 
					g_list_free(el->properties);
					g_list_free(el->notes);
					if (el->insert) xmlFree(el->insert);
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
		g_hash_table_foreach(info->elements,fref_free_record,bfwin);
		g_hash_table_destroy(info->elements);		
		
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


static void fref_save_common(gpointer key,gpointer value,gpointer data)
{
	xmlNodePtr ptr = (xmlNodePtr)data;
	Tfref_record *rec = FREFRECORD(value);
	gchar *name = (gchar*)key;
	xmlNodePtr anode,txt,n;
	gchar *tp=NULL;
		
	if ( name[0]=='_' || name[1]=='_') return;
	
	anode = xmlNewNode(NULL, (const xmlChar *) "def");
	switch (rec->etype)
	{
		case FREF_EL_DESCR:
			xmlNewProp(anode, (const xmlChar *) "deftype", (const xmlChar *) "description");
			xmlNewProp(anode, (const xmlChar *) "id", key);
			txt = xmlNewText(rec->data);
			xmlAddChild(anode,txt);
			xmlAddChild(ptr,anode);
		break;
		case FREF_EL_VALUES:
			xmlNewProp(anode, (const xmlChar *) "deftype", (const xmlChar *) "values");
			xmlNewProp(anode, (const xmlChar *) "id", key);			
			txt = xmlNewText(rec->data);
			xmlAddChild(anode,txt);
			xmlAddChild(ptr,anode);
		break;		
		case FREF_EL_LINK:			
			if (!tp) tp = "link";				
		case FREF_EL_EXAMPLE:
			if (!tp) tp = "example";							
		case FREF_EL_DEPEND:
			if (!tp) tp = "dependency";							
		case FREF_EL_ATTR:
			if (!tp) tp = "attribute";							
		case FREF_EL_PARAM:
			if (!tp) tp = "parameter";							
		case FREF_EL_RETURN:
			if (!tp) tp = "return";							
		case FREF_EL_PARENTS:
			if (!tp) tp = "parents";								
			xmlNewProp(anode, (const xmlChar *) "deftype", (const xmlChar *) "property");
			xmlNewProp(anode, (const xmlChar *) "id", key);			
			xmlNewProp(anode, (const xmlChar *) "kind", (const xmlChar *) tp);			
			xmlNewProp(anode, (const xmlChar *) "name", FREFPROPERTY(rec->data)->name);			
			if (FREFPROPERTY(rec->data)->required)
				xmlNewProp(anode, (const xmlChar *) "required", (const xmlChar *) "1");			
			if (FREFPROPERTY(rec->data)->def)
				xmlNewProp(anode, (const xmlChar *) "default",FREFPROPERTY(rec->data)->def);
			if (FREFPROPERTY(rec->data)->type)
				xmlNewProp(anode, (const xmlChar *) "type",FREFPROPERTY(rec->data)->type);
			switch (FREFPROPERTY(rec->data)->input_type)
			{
				case FREF_INP_LIST:xmlNewProp(anode, (const xmlChar *) "input", (const xmlChar *) "list");break;			
				case FREF_INP_LIST_ENTRY:xmlNewProp(anode, (const xmlChar *) "input", (const xmlChar *) "list+entry");break;			
				case FREF_INP_COLOUR:xmlNewProp(anode, (const xmlChar *) "input", (const xmlChar *) "color");break;							
				case FREF_INP_COLOUR_LIST:xmlNewProp(anode, (const xmlChar *) "input", (const xmlChar *) "color+list");break;							
				case FREF_INP_FILE:xmlNewProp(anode, (const xmlChar *) "input", (const xmlChar *) "file");break;							
			}
			if (FREFPROPERTY(rec->data)->vlist)
			{
				n = xmlNewNode(NULL, (const xmlChar *) "values");
				txt = xmlNewText(FREFPROPERTY(rec->data)->vlist);
				xmlAddChild(n,txt);
				xmlAddChild(anode,n);
			}	
			if (FREFPROPERTY(rec->data)->description)
			{
				n = xmlNewNode(NULL, (const xmlChar *) "description");
				txt = xmlNewText(FREFPROPERTY(rec->data)->description);
				xmlAddChild(n,txt);
				xmlAddChild(anode,n);
			}					
			xmlAddChild(ptr,anode);
		break;
	}
	
}

static void fref_save_property(Tfref_property *prop,xmlNodePtr parent)
{
	xmlNodePtr node,txt,node2;
	gchar *tp=NULL;
	
	node = xmlNewNode(NULL, (const xmlChar *) "property");
	if (prop->id && strcmp(prop->id,"")!=0) /* common */
	{
		xmlNewProp(node, (const xmlChar *) "ref", prop->id);
	}
	else
	{
		switch (prop->ptype)
		{
			case FREF_EL_LINK:
				if (!tp) tp = "link";
			case FREF_EL_EXAMPLE:
				if (!tp) tp = "example";
			case FREF_EL_DEPEND:
				if (!tp) tp = "dependency";
			case FREF_EL_ATTR:
				if (!tp) tp = "attribute";
			case FREF_EL_PARAM:
				if (!tp) tp = "parameter";
			case FREF_EL_RETURN:
				if (!tp) tp = "return";
				xmlNewProp(node, (const xmlChar *) "kind", (const xmlChar *) tp);
				xmlNewProp(node, (const xmlChar *) "name", prop->name);
				if ( prop->description )
				{
					node2 = xmlNewNode(NULL, (const xmlChar *) "description");
					txt = xmlNewText(prop->description);
					xmlAddChild(node2,txt);
					xmlAddChild(node,node2);
				}	
				if ( prop->vlist )
				{
					node2 = xmlNewNode(NULL, (const xmlChar *) "values");
					txt = xmlNewText(prop->vlist);
					xmlAddChild(node2,txt);
					xmlAddChild(node,node2);
				}	
				if ( prop->type )
					xmlNewProp(node, (const xmlChar *) "type", prop->type);
				if (prop->def)
					xmlNewProp(node, (const xmlChar *) "default", prop->def);
				if (prop->required)
					xmlNewProp(node, (const xmlChar *) "required", (const xmlChar *) "1");	
				tp=NULL;	
				switch (prop->input_type)
				{
					case FREF_INP_LIST:
						if (!tp) tp="list";
					case FREF_INP_LIST_ENTRY:
						if (!tp) tp="list+entry";
					case FREF_INP_COLOUR:
						if (!tp) tp="color";
					case FREF_INP_COLOUR_LIST:
						if (!tp) tp="color+list";
					case FREF_INP_FILE:
						if (!tp) tp="file";
						xmlNewProp(node, (const xmlChar *) "input", (const xmlChar *) tp);	
				}/* sw */	
			break;
		} /* switch */
	}	
	xmlAddChild(parent,node);
}

static void fref_save_element(GtkTreeStore *store,GtkTreeIter *iter,xmlDocPtr doc,xmlNodePtr parent)
{
	GValue *val=NULL;
	xmlNodePtr node,txt,node2;
	GtkTreeIter it;
	GList *lst=NULL;

	val = g_new0(GValue, 1);	
	gtk_tree_model_get_value(GTK_TREE_MODEL(store),iter,PTR_COLUMN, val);
	if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) 
	{
		Tfref_record *rec = FREFRECORD(g_value_peek_pointer(val));
		switch ( rec->etype )
		{
			case FREF_EL_NOTE: {
				Tfref_note *n = FREFNOTE(rec->data);
				node = xmlNewNode(NULL, (const xmlChar *) "note");
				xmlNewProp(node, (const xmlChar *) "title", n->title);
				txt = xmlNewText(n->text);
				xmlAddChild(node,txt);
				xmlAddChild(parent,node);
			}	
			break;
			case FREF_EL_GROUP: {
				Tfref_note *n = FREFNOTE(rec->data);
				node = xmlNewNode(NULL, (const xmlChar *) "group");
				xmlNewProp(node, (const xmlChar *) "name", n->title);
				node2 = xmlNewNode(NULL, (const xmlChar *) "description");
				txt = xmlNewText(n->text);
				xmlAddChild(node2,txt);
				xmlAddChild(node,node2);
				xmlAddChild(parent,node);
				if ( gtk_tree_model_iter_children(GTK_TREE_MODEL(store), &it, iter) )
				{
					while (1) 
					{	
						fref_save_element(FREFDATA(main_v->frefdata)->store, &it,doc,node);		
						if ( !gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &it)) break;
					}							/* while */			
				 }		
				}	
			break;			
			default:{
				Tfref_element *el = FREFELEMENT(rec->data);
				node = xmlNewNode(NULL, (const xmlChar *) "element");
				switch (el->etype)
				{
					case FREF_EL_FUNCTION:xmlNewProp(node, (const xmlChar *) "kind", (const xmlChar *) "function");break;
					case FREF_EL_TAG:xmlNewProp(node, (const xmlChar *) "kind", (const xmlChar *) "tag");break;
					case FREF_EL_VAR:xmlNewProp(node, (const xmlChar *) "kind", (const xmlChar *) "var");break;
					case FREF_EL_CSSPROP:xmlNewProp(node, (const xmlChar *) "kind", (const xmlChar *) "css_property");break;
					case FREF_EL_CSSSELECT:xmlNewProp(node, (const xmlChar *) "kind", (const xmlChar *) "css_selector");break;
					case FREF_EL_SNIPPET:xmlNewProp(node, (const xmlChar *) "kind", (const xmlChar *) "snippet");break;
				} /* switch */
				xmlNewProp(node, (const xmlChar *) "name", el->name);
				if ( el->description )
				{
					node2 = xmlNewNode(NULL, (const xmlChar *) "description");
					txt = xmlNewText(el->description);
					xmlAddChild(node2,txt);
					xmlAddChild(node,node2);				
				}
				if ( el->insert )
				{
					node2 = xmlNewNode(NULL, (const xmlChar *) "insert");
					txt = xmlNewText(el->insert);
					xmlAddChild(node2,txt);
					xmlAddChild(node,node2);				
				}				
				/* properties ! */
				lst = g_list_first(el->properties);
				node2 = xmlNewNode(NULL, (const xmlChar *) "properties");
				xmlAddChild(node,node2);
				while ( lst )
				{
					fref_save_property(FREFPROPERTY(lst->data),node2);
					lst = g_list_next(lst);
				} /* while */
				lst = g_list_first(el->notes);
				while (lst)
				{
					Tfref_note *n = FREFNOTE(lst->data);
					node2 = xmlNewNode(NULL, (const xmlChar *) "note");
					xmlNewProp(node2, (const xmlChar *) "title", n->title);
					txt = xmlNewText(n->text);
					xmlAddChild(node2,txt);
					xmlAddChild(node,node2);									
					lst = g_list_next(lst);
				}				
				xmlAddChild(parent,node);
			}	
		} /* switch */
	}
}

static gboolean fref_save_ref(Tbfwin *bfwin)
{
		gchar *fname = NULL;
		GValue *val,*val2;
		GtkTreePath *path;
		GtkTreeViewColumn *col;
		GtkTreeIter it,iter;
		xmlDocPtr doc;
		xmlNodePtr rnode;
		Tfref_info *info = fref_refinfo_for_current(bfwin);


	gtk_tree_view_get_cursor(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), &path, &col);
	if (path==NULL) return FALSE;
	while (gtk_tree_path_get_depth(path) > 1 && gtk_tree_path_up(path));
	gtk_tree_model_get_iter(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),&iter,path);

		val = g_new0(GValue, 1);
		gtk_tree_model_get_value(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),&iter,FILE_COLUMN, val);
		if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) {
			fname =  (gchar*)g_value_peek_pointer(val);
		}
		if ( !fname ) return FALSE;		
		
		doc = xmlNewDoc((const xmlChar *) "1.0");
		rnode = xmlNewNode(NULL, (const xmlChar *) "ref");
		xmlDocSetRootElement(doc, rnode);
		
		xmlNewProp(rnode, (const xmlChar *) "version", (const xmlChar *) "2");
		val2 = g_new0(GValue, 1);
		gtk_tree_model_get_value(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),&iter,STR_COLUMN, val2);
		if (G_IS_VALUE(val2) && g_value_fits_pointer(val2) && g_value_peek_pointer(val2)) {
			xmlNewProp(rnode, (const xmlChar *) "name", g_value_peek_pointer(val2));
		}
		g_value_unset(val2);
		g_free(val2);    	
		
		g_hash_table_foreach(info->commons,fref_save_common,rnode);
		
		val2 = g_new0(GValue, 1);
		gtk_tree_model_get_value(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),&iter,PTR_COLUMN, val2);
		if (G_IS_VALUE(val2) && g_value_fits_pointer(val2) && g_value_peek_pointer(val2)) {
			Tfref_record *rr = FREFRECORD(g_value_peek_pointer(val2));
			if (FREFINFO(rr->data)->description)
			{
				xmlNewProp(rnode, (const xmlChar *) "description", FREFINFO(rr->data)->description);
			}	
			gtk_tree_model_iter_children(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store), &it, &iter);
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

/* ------------------------- REFERENCE EDITOR ----------------------------------- */




/* dialog box for title and description */
static GtkWidget *fref_editor_td(gchar *title, gchar *label1,gchar *label2,Tbfwin *bfwin, 
																	GtkWidget **entry1,GtkWidget **entry2) 
{
	GtkWidget *dialog,*vbox,*label,*cancelbutton,*okbutton,*table,*scroll;
		
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), title);
	vbox = GTK_DIALOG(dialog)->vbox;
	table = gtk_table_new(2, 3, FALSE);
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
	gtk_table_attach(GTK_TABLE(table), label, 0, 2, 1, 2,(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 5, 6);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);			
	gtk_widget_show(label);				
	*entry2 = gtk_text_view_new();
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll),*entry2);
	gtk_table_attach(GTK_TABLE(table), scroll, 0, 2, 2,3,
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

/* dialog box with list and ok/cancel button */
static GtkWidget *fref_editor_list_dialog(Tbfwin *bfwin,gchar *title,
																						gboolean showcancel,gboolean showinfo,
																						GtkWidget **view, GtkListStore **store, GtkWidget **info)
{
	GtkWidget *dialog,*vbox,*scroll,*cancelbutton,*okbutton;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), title);
	vbox = GTK_DIALOG(dialog)->vbox;
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);
	*store = gtk_list_store_new(2,G_TYPE_POINTER,G_TYPE_STRING);								   
	*view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(*store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(*view), FALSE);

   cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", cell, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(*view), column);
	
	gtk_container_add(GTK_CONTAINER(scroll), *view);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(*view), FALSE);
	
	gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
	if ( showinfo )
	{
		*info = gtk_label_new("");
		gtk_box_pack_start(GTK_BOX(vbox), *info, FALSE, FALSE, 0);
	}
	
	if (showcancel)
	{
		cancelbutton = gtk_button_new_from_stock("gtk-cancel");
		gtk_widget_show(cancelbutton);
		gtk_dialog_add_action_widget(GTK_DIALOG(dialog), cancelbutton, GTK_RESPONSE_CANCEL);
	}	
	okbutton = gtk_button_new_from_stock("gtk-ok");
	gtk_widget_show(okbutton);
	gtk_dialog_add_action_widget(GTK_DIALOG(dialog), okbutton, GTK_RESPONSE_OK);
	
	gtk_widget_show_all(dialog);
	
	return dialog;
}

static gchar *fref_editor_getviewtext(GtkTextView *view)
{
	GtkTextBuffer *buff = gtk_text_view_get_buffer(view);
	GtkTextIter it1,it2;
	
	gtk_text_buffer_get_start_iter(buff,&it1);
	gtk_text_buffer_get_end_iter(buff,&it2);
	return gtk_text_buffer_get_text(buff,&it1,&it2,FALSE);
}

/* dialog for property input */

typedef struct {
	GtkWidget *e_kind,*e_name,*e_type,*e_default,*e_input,*e_required,*e_description,*e_values,*e_id;
} Tfref_propdialog_data;

GtkWidget*  fref_editor_propdialog(gchar *title,Tfref_propdialog_data *data,gboolean showid)
{
  GtkWidget *dialog1,*dialog_vbox1,*table1;
  GtkWidget *label1,*label2,*label3,*label4,*label5,*label7,*label6,*label0;
  GtkWidget *scrolledwindow2,*scrolledwindow1;
  GtkWidget *dialog_action_area1;
  GtkWidget *cancelbutton1;
  GtkWidget *okbutton1;

  dialog1 = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog1), _(title));
  dialog_vbox1 = GTK_DIALOG (dialog1)->vbox;
  table1 = gtk_table_new (11, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), table1, TRUE, TRUE, 0);

	if (showid)
	{ 
	  data->e_id = gtk_entry_new ();
	  gtk_table_attach (GTK_TABLE (table1), data->e_id, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
		label0 = gtk_label_new (_("Identifier"));
  		gtk_table_attach (GTK_TABLE (table1), label0, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  		gtk_misc_set_alignment (GTK_MISC (label0), 0, 0.5);
  }

  data->e_kind = gtk_combo_box_new_text ();
  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_kind),"link");
  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_kind),"example");
  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_kind),"dependency");
  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_kind),"attribute");
  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_kind),"parameter");
  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_kind),"return");
/*  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_kind),"parents");  */
  gtk_combo_box_set_active(GTK_COMBO_BOX(data->e_kind),0);
  gtk_table_attach (GTK_TABLE (table1), data->e_kind, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);

  label1 = gtk_label_new (_("Kind"));
  gtk_table_attach (GTK_TABLE (table1), label1, 0, 1, 1,2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);

  label2 = gtk_label_new (_("Name"));
  gtk_table_attach (GTK_TABLE (table1), label2, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

  data->e_name = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table1), data->e_name, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);

  label3 = gtk_label_new (_("Type"));
  gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);

  data->e_type = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table1), data->e_type, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);

  label4 = gtk_label_new (_("Default value"));
  gtk_table_attach (GTK_TABLE (table1), label4, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);

  data->e_default = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table1), data->e_default, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);

  label5 = gtk_label_new (_("Input type"));
  gtk_table_attach (GTK_TABLE (table1), label5, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_misc_set_alignment (GTK_MISC (label5), 0, 0.5);

  data->e_input = gtk_combo_box_new_text ();
  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_input),"entry");  
  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_input),"list");  
  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_input),"list+entry");  
  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_input),"color");  
  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_input),"color+list");  
  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_input),"file");  
  gtk_combo_box_set_active(GTK_COMBO_BOX(data->e_input),0);
  gtk_table_attach (GTK_TABLE (table1), data->e_input, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
                    
  data->e_required = gtk_check_button_new_with_mnemonic (_("Required"));
  gtk_table_attach (GTK_TABLE (table1), data->e_required, 0, 2, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  label7 = gtk_label_new (_("Description"));
  gtk_table_attach (GTK_TABLE (table1), label7, 0, 2, 9, 10,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_misc_set_alignment (GTK_MISC (label7), 0, 0.5);
  scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
  gtk_table_attach (GTK_TABLE (table1), scrolledwindow2, 0, 2, 10, 11,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  data->e_description = gtk_text_view_new ();
  gtk_container_add (GTK_CONTAINER (scrolledwindow2), data->e_description);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_table_attach (GTK_TABLE (table1), scrolledwindow1, 0, 2, 8, 9,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  data->e_values = gtk_text_view_new ();
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), data->e_values);

  label6 = gtk_label_new (_("Values"));
  gtk_table_attach (GTK_TABLE (table1), label6, 0, 2, 7, 8,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);

  dialog_action_area1 = GTK_DIALOG (dialog1)->action_area;
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);

  cancelbutton1 = gtk_button_new_from_stock ("gtk-cancel");
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog1), cancelbutton1, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (cancelbutton1, GTK_CAN_DEFAULT);

  okbutton1 = gtk_button_new_from_stock ("gtk-ok");
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog1), okbutton1, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (okbutton1, GTK_CAN_DEFAULT);

	gtk_widget_show_all(dialog1);
  return dialog1;
}

typedef struct {
	GtkWidget *e_name,*e_kind,*e_insert,*e_description,*e_proplist,*e_propinfo,*btn_add,*btn_del,*btn_addref;
} Tfref_elemdialog_data;

static void frefcb_inserthelp_clicked(GtkButton *button, GtkWidget *w) 
{
	message_dialog_new(w,
							 GTK_MESSAGE_INFO,
							 GTK_BUTTONS_OK,
							 _("Insert syntax"), 
							 _("All elements:\n%0,%1,... - value of parameter number 0,1,...%* - values of all parameters\n\
Tag element:\n%_ - only non-empty attributes\n%! - only required attributes\n%~ - only non-empty OR required attributes."));
}

/* dialog for element */
GtkWidget* fref_editor_elemdialog (gchar *title, Tfref_elemdialog_data *data,Tbfwin *bfwin)
{
  GtkWidget *dialog1,*dialog_vbox1,*hbox1,*table1;
  GtkWidget *label1,*label2,*label3;
  GtkWidget *scrolledwindow1;
  GtkWidget *label4;
  GtkWidget *scrolledwindow2;
  GtkWidget *table2;
  GtkWidget *label5;
  GtkWidget *scrolledwindow3;
  GtkWidget *label6;
  GtkWidget *vbuttonbox1;
  GtkWidget *dialog_action_area1;
  GtkWidget *cancelbutton1;
  GtkWidget *okbutton1;
  GtkCellRenderer *cell;
  GtkTreeViewColumn *column;

  dialog1 = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog1), title);
  dialog_vbox1 = GTK_DIALOG (dialog1)->vbox;
  hbox1 = gtk_hbox_new (FALSE, 0);
  
  table1 = gtk_table_new (6, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox1), table1, TRUE, TRUE, 0);
  label1 = gtk_label_new (_("Name"));
  gtk_table_attach (GTK_TABLE (table1), label1, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);
  data->e_name = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table1), data->e_name, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  label2 = gtk_label_new (_("Kind"));
  gtk_table_attach (GTK_TABLE (table1), label2, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

  data->e_kind = gtk_combo_box_new_text ();
  gtk_table_attach (GTK_TABLE (table1), data->e_kind, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_kind),"function");  
  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_kind),"tag");  
  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_kind),"var");  
  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_kind),"css_property");  
  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_kind),"css_selector");  
  gtk_combo_box_append_text(GTK_COMBO_BOX(data->e_kind),"snippet");  
  gtk_combo_box_set_active(GTK_COMBO_BOX(data->e_kind),0);
  
  label3 = gtk_label_new (_("Insert text"));
  gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);
  label6 = gtk_button_new_with_label(_(" ? "));
  g_signal_connect(G_OBJECT(label6), "clicked",G_CALLBACK(frefcb_inserthelp_clicked), dialog1);	
  gtk_tooltips_set_tip(FREFGUI(bfwin->fref)->argtips,label6,_("Help"),"");
  gtk_table_attach (GTK_TABLE (table1), label6, 1, 2, 2, 3,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 2, 2);
  
  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_table_attach (GTK_TABLE (table1), scrolledwindow1, 0, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 2, 2);

  data->e_insert = gtk_text_view_new ();
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), data->e_insert);
  
  label4 = gtk_label_new (_("Description"));
  gtk_table_attach (GTK_TABLE (table1), label4, 0, 2, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);

  scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
  gtk_table_attach (GTK_TABLE (table1), scrolledwindow2, 0, 2, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 2, 2);

  data->e_description = gtk_text_view_new ();
  gtk_container_add (GTK_CONTAINER (scrolledwindow2), data->e_description);

  table2 = gtk_table_new (4, 1, FALSE);
  gtk_box_pack_end(GTK_BOX (hbox1), table2, TRUE, TRUE, 0);

  label5 = gtk_label_new (_("Properties"));
  gtk_table_attach (GTK_TABLE (table2), label5, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_misc_set_alignment (GTK_MISC (label5), 0, 0.5);

  scrolledwindow3 = gtk_scrolled_window_new (NULL, NULL);
  gtk_table_attach (GTK_TABLE (table2), scrolledwindow3, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 2, 2);

  data->e_proplist = gtk_tree_view_new ();
  gtk_container_add (GTK_CONTAINER (scrolledwindow3), data->e_proplist);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(data->e_proplist), FALSE);
  cell = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("", cell, "text", 1, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(data->e_proplist), column);



  data->e_propinfo = gtk_label_new ("");
  gtk_table_attach (GTK_TABLE (table2), data->e_propinfo, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_misc_set_alignment (GTK_MISC (data->e_propinfo), 0.5, 0.5);

  vbuttonbox1 = gtk_vbutton_box_new ();
  gtk_table_attach (GTK_TABLE (table2), vbuttonbox1, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 2, 2);

  data->btn_add = gtk_button_new_with_mnemonic (_("Add property"));
  gtk_container_add (GTK_CONTAINER (vbuttonbox1), data->btn_add);
  GTK_WIDGET_SET_FLAGS (data->btn_add, GTK_CAN_DEFAULT);

  data->btn_addref = gtk_button_new_with_mnemonic (_("Add ref definition"));
  gtk_container_add (GTK_CONTAINER (vbuttonbox1), data->btn_addref);
  GTK_WIDGET_SET_FLAGS (data->btn_addref, GTK_CAN_DEFAULT);

  data->btn_del = gtk_button_new_with_mnemonic (_("Delete"));
  gtk_container_add (GTK_CONTAINER (vbuttonbox1), data->btn_del);
  GTK_WIDGET_SET_FLAGS (data->btn_del, GTK_CAN_DEFAULT);

  dialog_action_area1 = GTK_DIALOG (dialog1)->action_area;
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);

  cancelbutton1 = gtk_button_new_from_stock ("gtk-cancel");
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog1), cancelbutton1, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (cancelbutton1, GTK_CAN_DEFAULT);

  okbutton1 = gtk_button_new_from_stock ("gtk-ok");
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog1), okbutton1, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (okbutton1, GTK_CAN_DEFAULT);
	
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), hbox1, TRUE, TRUE, 0);
	gtk_widget_show_all(dialog1);
	
  return dialog1;
}



/* ---------------------            CALLBACKS  ---------------------------------------*/



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
	gchar *userdir = g_strconcat(g_get_home_dir(), "/."PACKAGE"/", NULL);
	DEBUG_MSG("fref_popup_menu_rescan_lcb, started\n");
	fref_rescan_dir(PKGDATADIR);
	fref_rescan_dir(userdir);
	g_free(userdir);
	DEBUG_MSG("about to refill toplevels\n");
	fill_toplevels(FREFDATA(main_v->frefdata), TRUE);
}

static gchar *fref_notnull_str(gchar *str) {
	if ( str == NULL ) return "";
	return str;
}

static gchar *fref_bool_str(gboolean val) {
	if ( val ) return "true";
	return "false";
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

static void info_common(gpointer key,gpointer value,gpointer data)
{
   gchar *name = (gchar*)key;
   gchar **ret = (gchar**)data;
   gchar *tofree,*k=NULL;
   
   
   if ( name[0]!='_' && name[1]!='_')
   {
		Tfref_record *rec = FREFRECORD(value);
		switch (rec->etype)
		{
			case FREF_EL_DESCR:
				tofree = *ret;
				*ret = g_strconcat(*ret,"\n - ","description (",key,")",NULL);
				g_free(tofree);
			break;
			case FREF_EL_VALUES:
				tofree = *ret;
				*ret = g_strconcat(*ret,"\n - ","values (",key,")",NULL);
				g_free(tofree);
			break;
			case FREF_EL_PARAM:
				k="parameter";
			case FREF_EL_ATTR:
			   if (!k) k="attribute";
			case FREF_EL_RETURN:
				if (!k) k="return";
			case FREF_EL_EXAMPLE:
				if (!k) k="example";
			case FREF_EL_DEPEND:
				if (!k) k="dependency";
			case FREF_EL_LINK:		
				if (!k) k="link";
  				tofree = *ret;
				*ret = g_strconcat(*ret,"\n - ",FREFPROPERTY(rec->data)->name," (",
				fref_notnull_str(FREFPROPERTY(rec->data)->id),")\n   kind=",k,NULL);
				g_free(tofree);
				if (FREFPROPERTY(rec->data)->required)
				{
					tofree = *ret;
					*ret = g_strconcat(*ret," | required ",NULL);
					g_free(tofree);
				}
				if (FREFPROPERTY(rec->data)->type)
				{
					tofree = *ret;
					*ret = g_strconcat(*ret," | type=",FREFPROPERTY(rec->data)->type,NULL);
					g_free(tofree);
				}
				if (FREFPROPERTY(rec->data)->def)
				{
					tofree = *ret;
					*ret = g_strconcat(*ret," | default=",FREFPROPERTY(rec->data)->def,NULL);
					g_free(tofree);
				}
				if (FREFPROPERTY(rec->data)->description)
				{
					tofree = *ret;
					*ret = g_strconcat(*ret,"\n ",FREFPROPERTY(rec->data)->description,NULL);
					g_free(tofree);				
				}
			break;	
		}	   
	}	
}


gchar *fref_prepare_info(Tfref_record * entry, gint infotype)
{
	gchar *ret,*tofree,*tofree2;
	GList *lst=NULL;
	/* Tfref_record *rec=NULL;*/

ret = g_strdup("");

switch (infotype) {
	case FR_INFO_DESC	:
		switch (entry->etype) {
			case FREF_EL_REF: 
			{
				ret = g_strconcat(fref_notnull_str(FREFINFO(entry->data)->description),"\n",NULL);
				tofree2  = g_strdup("");
				g_hash_table_foreach(FREFINFO(entry->data)->commons,info_common,&tofree2);
				if ( strcmp(tofree2,"")!=0 )
				{
					tofree = ret;
					ret = g_strconcat(ret,"DEFINITIONS:",tofree2,NULL);
					g_free(tofree);
					g_free(tofree2);
				}
			}
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
			case FREF_EL_VAR:
					ret =	g_strconcat(ret, _("VAR: "),fref_notnull_str(FREFELEMENT(entry->data)->name),"\n ",NULL);
					if (FREFELEMENT(entry->data)->description) {
						ret = g_strconcat(ret,FREFELEMENT(entry->data)->description,NULL);
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
				ret = g_strdup("VALUES:\n");
				while ( lst != NULL ) {
					if (FREFPROPERTY(lst->data)->ptype == FREF_EL_PARAM) {
							tofree = ret;
							ret = g_strconcat(tofree," - ",fref_notnull_str(FREFPROPERTY(lst->data)->name),": ",
							                            fref_notnull_str(FREFPROPERTY(lst->data)->description),"\n",NULL);
							g_free(tofree);                            					
					}                            
					lst = g_list_next(lst);
				}			
			break;				
			case FREF_EL_CSSSELECT:
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
			case FREF_EL_VAR:
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
			case FREF_EL_VAR:
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
			case FREF_EL_VAR:
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
	GList *lst=NULL;
	gint d1, d2,cnt;
	gchar *converted = NULL/*, *tmp2 = NULL*/;
	gint proptype,input_type;
	gboolean first;
	GtkWidget *dlgitem;
	gchar *sep="";

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
/*		switch (elem->etype) 
		{
			case FREF_EL_TAG:*/
			
			switch (elem->etype)			
			{
				case FREF_EL_FUNCTION:sep=",";break;
				case FREF_EL_CSSPROP:
				case FREF_EL_CSSSELECT:
							sep=" ";break;
			} /* switch */
				
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
						proptype = FREFPROPERTY(lst->data)->ptype;
						if ( proptype == FREF_EL_ATTR || proptype == FREF_EL_PARAM) 
						{ 
							if (cnt>=d1) break;												
							cnt++;
					   }
						lst = g_list_next(lst);
					} /* while */
					if (lst != NULL && ( proptype == FREF_EL_ATTR || proptype == FREF_EL_PARAM)) 
					{
							proptype = FREFPROPERTY(lst->data)->ptype;
							input_type = FREFPROPERTY(lst->data)->input_type;
							dlgitem = FREFPROPERTY(lst->data)->dlgitem;					
							if ( dlgitem != NULL )
							{
									   switch ( input_type )
									   {
									   	case FREF_INP_ENTRY:
									   	case FREF_INP_COLOUR:
									   	case FREF_INP_FILE:
									   			if (proptype==FREF_EL_ATTR)
														converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																	 gtk_entry_get_text(GTK_ENTRY(dlgitem)),
																	 "\"",NULL);					
													else
														converted = g_strdup(gtk_entry_get_text(GTK_ENTRY(dlgitem)));									 				   	
									   	break;
									   	case FREF_INP_LIST:
									   	{
														GtkTreeIter it;
														if ( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(dlgitem),&it) ) 
														{
																GValue *val = g_new0(GValue, 1);
																gtk_tree_model_get_value(GTK_TREE_MODEL(gtk_combo_box_get_model(
																										 GTK_COMBO_BOX(dlgitem)	)),
																										 &it, 0, val);
																if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) 
																{
																	if (proptype==FREF_EL_ATTR)
																		converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																				 g_value_peek_pointer(val),
																				 "\"",NULL);
																	else
																		converted = g_strdup(g_value_peek_pointer(val));			 
																}
																g_value_unset(val);
																g_free(val);
														}		
											}			
									   	break;
									   	case FREF_INP_LIST_ENTRY:
									   	case FREF_INP_COLOUR_LIST:
									   		if (proptype == FREF_EL_ATTR)
													converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																				 gtk_entry_get_text(GTK_ENTRY(GTK_BIN(dlgitem)->child)),
																				 "\"",NULL);									   	
												else
													converted = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(dlgitem)->child)));								 
									   	break;
									   } /* switch */
							} /* null item */													
					} 
				} /* d1 == -1 */
				else if (*p == '_' ) /* only non-empty attributes from dialog */
				{
					lst = g_list_first(elem->properties);
					converted = "";
					first=TRUE;
					while ( lst ) 
					{
							proptype = FREFPROPERTY(lst->data)->ptype;
							input_type = FREFPROPERTY(lst->data)->input_type;
							dlgitem = FREFPROPERTY(lst->data)->dlgitem;
					
							if ( proptype == FREF_EL_ATTR  || proptype == FREF_EL_PARAM) 
							{ 
									if ( dlgitem != NULL )
									{
									   switch ( input_type )
									   {
									   	case FREF_INP_ENTRY:
									   	case FREF_INP_COLOUR:
									   	case FREF_INP_FILE:
									   		if (strcmp(gtk_entry_get_text(GTK_ENTRY(dlgitem)),"")!=0)
									   		{
									   			if (proptype==FREF_EL_ATTR)
														converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																	 gtk_entry_get_text(GTK_ENTRY(dlgitem)),
																	 "\"",NULL);
														/* BUG: valgrind reports that this 'converted' is never freed: memory leak! */
														/* O.S. It seems to me it is freed at the end  near line 2654 */
													else
													{
														if (first)
														{
															converted = g_strconcat(converted,gtk_entry_get_text(GTK_ENTRY(dlgitem)),NULL);				 
															first=FALSE;
														}
														else
															converted = g_strconcat(converted,sep,gtk_entry_get_text(GTK_ENTRY(dlgitem)),NULL);
													}
												}
									   	break;
									   	case FREF_INP_LIST:
									   	{
														GtkTreeIter it;
														if ( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(dlgitem),&it) ) 
														{
																GValue *val = g_new0(GValue, 1);
																gtk_tree_model_get_value(GTK_TREE_MODEL(gtk_combo_box_get_model(
																										 GTK_COMBO_BOX(dlgitem)	)),
																										 &it, 0, val);
																if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) 
																{
																	if (strcmp(g_value_peek_pointer(val),"")!=0)
																	{
																		if (proptype==FREF_EL_ATTR)
																			converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																				 g_value_peek_pointer(val),
																				 "\"",NULL);
																		else
																		{
																			if (first)
																			{
																				converted = g_strconcat(converted,g_value_peek_pointer(val),NULL);		 
																				first=FALSE;
																			}	else converted = g_strconcat(converted,sep,g_value_peek_pointer(val),NULL);
																		}	
																	}			 
																}
																g_value_unset(val);
																g_free(val);
														}		
											}			
									   	break;
									   	case FREF_INP_LIST_ENTRY:
									   	case FREF_INP_COLOUR_LIST:
									   	   if (strcmp("",gtk_entry_get_text(GTK_ENTRY(GTK_BIN(dlgitem)->child)))!=0)
									   	   {
									   	   	if (proptype == FREF_EL_ATTR)
														converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																				 gtk_entry_get_text(GTK_ENTRY(GTK_BIN(dlgitem)->child)),
																				 "\"",NULL);									   	
													else
													{
														if (first)
														{
															converted = g_strconcat(converted,gtk_entry_get_text(GTK_ENTRY(GTK_BIN(dlgitem)->child)),NULL);
															first=FALSE;
														} else converted = g_strconcat(converted,sep,gtk_entry_get_text(GTK_ENTRY(GTK_BIN(dlgitem)->child)),NULL);	
													}	
												}								 
									   	break;
									   } /* switch */
									} /* null item */							
					   	}
							lst = g_list_next(lst);
					} /* while */							
				}
				else if (*p == '*') /* ALL attributes */
				{
						lst = g_list_first(elem->properties);
						converted = "";
						first=TRUE;
						while ( lst ) 
						{
							proptype = FREFPROPERTY(lst->data)->ptype;
							input_type = FREFPROPERTY(lst->data)->input_type;
							dlgitem = FREFPROPERTY(lst->data)->dlgitem;
						
							if ( proptype == FREF_EL_ATTR || proptype == FREF_EL_PARAM) 
							{ 
									if ( dlgitem != NULL )
									{
									   switch ( input_type )
									   {
									   	case FREF_INP_ENTRY:
									   	case FREF_INP_COLOUR:
									   	case FREF_INP_FILE:
									   			if (proptype==FREF_EL_ATTR)
														converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																	 gtk_entry_get_text(GTK_ENTRY(dlgitem)),
																	 "\"",NULL);					
													else
													{
														if (first)
														{
															converted = g_strconcat(converted,gtk_entry_get_text(GTK_ENTRY(dlgitem)),NULL);
															first=FALSE;
														} else converted = g_strconcat(converted,sep,gtk_entry_get_text(GTK_ENTRY(dlgitem)),NULL);
													}		
									   	break;
									   	case FREF_INP_LIST:
									   	{
														GtkTreeIter it;
														if ( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(dlgitem),&it) ) 
														{
																GValue *val = g_new0(GValue, 1);
																gtk_tree_model_get_value(GTK_TREE_MODEL(gtk_combo_box_get_model(
																										 GTK_COMBO_BOX(dlgitem)	)),
																										 &it, 0, val);
																if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) 
																{
																	if (proptype==FREF_EL_ATTR)
																	converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																				 g_value_peek_pointer(val),
																				 "\"",NULL);
																	else
																	{
																		if (first)
																		{
																			converted = g_strconcat(converted,g_value_peek_pointer(val),NULL);
																			first=FALSE;
																		} else	g_strconcat(converted,sep,g_value_peek_pointer(val),NULL);
																	}	
																}
																g_value_unset(val);
																g_free(val);
														}		
											}			
									   	break;
									   	case FREF_INP_LIST_ENTRY:
									   	case FREF_INP_COLOUR_LIST:
									   		if (proptype==FREF_EL_ATTR)
												converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																				 gtk_entry_get_text(GTK_ENTRY(GTK_BIN(dlgitem)->child)),
																				 "\"",NULL);									   	
												else
												{
													if (first)
													{
														converted = g_strconcat(converted,gtk_entry_get_text(GTK_ENTRY(GTK_BIN(dlgitem)->child)),NULL);
														first=FALSE;
													} else converted = g_strconcat(converted,sep,gtk_entry_get_text(GTK_ENTRY(GTK_BIN(dlgitem)->child)),NULL);
												} 
									   	break;
									   } /* switch */
									} /* null item */							
					   	}
							lst = g_list_next(lst);
						} /* while */							
				}
				else if (*p == '!') /* only required attributes */
				{
						lst = g_list_first(elem->properties);
						converted = "";
						first=TRUE;
						while ( lst ) 
						{
							proptype = FREFPROPERTY(lst->data)->ptype;
							input_type = FREFPROPERTY(lst->data)->input_type;
							dlgitem = FREFPROPERTY(lst->data)->dlgitem;
						
							if ( ( proptype == FREF_EL_ATTR  || proptype == FREF_EL_PARAM )
							      && FREFPROPERTY(lst->data)->required) 
							{ 
									if ( dlgitem != NULL  && FREFPROPERTY(lst->data)->required)
									{
									   switch ( input_type )
									   {
									   	case FREF_INP_ENTRY:
									   	case FREF_INP_COLOUR:
									   	case FREF_INP_FILE:
									   		if (proptype==FREF_EL_ATTR)
														converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																	 gtk_entry_get_text(GTK_ENTRY(dlgitem)),
																	 "\"",NULL);									   	
												else
												{
													if (first)
													{
														converted = g_strconcat(converted,gtk_entry_get_text(GTK_ENTRY(dlgitem)),NULL);
														first=FALSE;
													} else 	converted = g_strconcat(converted,sep,gtk_entry_get_text(GTK_ENTRY(dlgitem)),NULL);
												}	
									   	break;
									   	case FREF_INP_LIST:
									   	{
														GtkTreeIter it;
														if ( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(dlgitem),&it) ) 
														{
																GValue *val = g_new0(GValue, 1);
																gtk_tree_model_get_value(GTK_TREE_MODEL(gtk_combo_box_get_model(
																										 GTK_COMBO_BOX(dlgitem)	)),
																										 &it, 0, val);
																if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) 
																{
																	if (proptype==FREF_EL_ATTR)
																	converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																				 g_value_peek_pointer(val),
																				 "\"",NULL);
																	else
																	{
																		if (first)
																		{
																			converted = g_strconcat(converted,g_value_peek_pointer(val),NULL);
																			first=FALSE;
																		} else	converted = g_strconcat(converted,sep,g_value_peek_pointer(val),NULL);
																	}
																}
																g_value_unset(val);
																g_free(val);
														}		
											}			
									   	break;
									   	case FREF_INP_LIST_ENTRY:
									   	case FREF_INP_COLOUR_LIST:
									   		if (proptype==FREF_EL_ATTR)
												converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																				 gtk_entry_get_text(GTK_ENTRY(GTK_BIN(dlgitem)->child)),
																				 "\"",NULL);									   	
												else
												{	
													if (first)
													{
														converted = g_strconcat(converted,gtk_entry_get_text(GTK_ENTRY(GTK_BIN(dlgitem)->child)),NULL);
														first = FALSE;
													} else 	converted = g_strconcat(converted,sep,gtk_entry_get_text(GTK_ENTRY(GTK_BIN(dlgitem)->child)),NULL);
												}	
									   	break;
									   } /* switch */
									} /* null item */							
					   	}
							lst = g_list_next(lst);
						} /* while */			
				}
				else if (*p == '~' ) /* only non-empty or required attributes from dialog*/
				{
					lst = g_list_first(elem->properties);
					converted = "";
					first = TRUE;
					while ( lst ) 
					{
							proptype = FREFPROPERTY(lst->data)->ptype;
							input_type = FREFPROPERTY(lst->data)->input_type;
							dlgitem = FREFPROPERTY(lst->data)->dlgitem;
					
							if (  proptype == FREF_EL_ATTR || proptype == FREF_EL_PARAM ) 
							{ 
									if ( dlgitem != NULL)
									{
									   switch ( input_type )
									   {
									   	case FREF_INP_ENTRY:
									   	case FREF_INP_COLOUR:
									   	case FREF_INP_FILE:
									   		if (FREFPROPERTY(lst->data)->required || strcmp("",gtk_entry_get_text(GTK_ENTRY(dlgitem)))!=0)
									   		{
									   			if (proptype==FREF_EL_ATTR)
														converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																	 gtk_entry_get_text(GTK_ENTRY(dlgitem)),
																	 "\"",NULL);									   	
													else
													{
														if (first)
														{
															converted = g_strconcat(converted,gtk_entry_get_text(GTK_ENTRY(dlgitem)),NULL);
															first=FALSE;
														} else converted = g_strconcat(converted,sep,gtk_entry_get_text(GTK_ENTRY(dlgitem)),NULL);
													}				 
												}					 
									   	break;
									   	case FREF_INP_LIST:
									   	{
														GtkTreeIter it;
														if ( gtk_combo_box_get_active_iter(GTK_COMBO_BOX(dlgitem),&it) ) 
														{
																GValue *val = g_new0(GValue, 1);
																gtk_tree_model_get_value(GTK_TREE_MODEL(gtk_combo_box_get_model(
																										 GTK_COMBO_BOX(dlgitem)	)),
																										 &it, 0, val);
																if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) 
																{
																	if (FREFPROPERTY(lst->data)->required || strcmp("",g_value_peek_pointer(val))!=0)
																	{
																		if (proptype==FREF_EL_ATTR)
																			converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																				 g_value_peek_pointer(val),
																				 "\"",NULL);
																		else
																		{
																			if (first)
																			{
																				converted = g_strconcat(converted,g_value_peek_pointer(val),NULL);
																				first=FALSE;
																			} else converted = g_strconcat(converted,sep,g_value_peek_pointer(val),NULL);
																		}		 
																	}			 
																}
																g_value_unset(val);
																g_free(val);
														}		
											}			
									   	break;
									   	case FREF_INP_LIST_ENTRY:
									   	case FREF_INP_COLOUR_LIST:
									   		if (FREFPROPERTY(lst->data)->required ||
									   		     strcmp(gtk_entry_get_text(GTK_ENTRY(GTK_BIN(dlgitem)->child)),"")!=0)
									   		{
									   			if (proptype==FREF_EL_ATTR)
													converted = g_strconcat(converted," ",FREFPROPERTY(lst->data)->name,"=\"",
																				 gtk_entry_get_text(GTK_ENTRY(GTK_BIN(dlgitem)->child)),
																				 "\"",NULL);									   	
													else
													{
														if (first)
														{
															converted = g_strconcat(converted,gtk_entry_get_text(GTK_ENTRY(GTK_BIN(dlgitem)->child)),NULL);
															first=FALSE;
														} else converted = g_strconcat(converted,sep,gtk_entry_get_text(GTK_ENTRY(GTK_BIN(dlgitem)->child)),NULL);
													}							 
												}								 
									   	break;
									   } /* switch */
									} /* null item */							
					   	}
							lst = g_list_next(lst);
					} /* while */											
				}				
		/*	break	;  TAG DATA - now all types */
/*		}					TYPE  switch */
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



static void fref_input_button_cb(GtkButton *button,Tfref_property *prop)
{
	Tbfwin *bfwin = NULL;
	if ( !prop->aux ) return;
	bfwin = BFWIN(prop->aux);
	switch (prop->input_type)
	{
		case FREF_INP_COLOUR:
		case FREF_INP_COLOUR_LIST:
		{
			GtkWidget *sdlg = gtk_color_selection_dialog_new(_("Choose colour"));
			if (bfwin && bfwin->session->colorlist) {
							GtkSettings* gtksettings;
							gchar *strings = stringlist_to_string(bfwin->session->colorlist, ":");
							strings[strlen(strings)-1] = '\0';
							gtksettings = gtk_widget_get_settings(GTK_WIDGET(
							GTK_COLOR_SELECTION_DIALOG(sdlg)->colorsel));
							g_object_set(G_OBJECT(gtksettings), "gtk-color-palette", strings, NULL); 
							g_free(strings);
			}
			gtk_color_selection_set_has_palette(GTK_COLOR_SELECTION(
			GTK_COLOR_SELECTION_DIALOG(sdlg)->colorsel), TRUE);			
			if ( gtk_dialog_run(GTK_DIALOG(sdlg)) == GTK_RESPONSE_OK )
			{
				GdkColor cc;
				gchar *tmp=NULL;
				
				gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(
				      GTK_COLOR_SELECTION_DIALOG(sdlg)->colorsel),&cc);
				tmp = gdk_color_to_hexstring(&cc,TRUE);
				if ( tmp )
				{
					if ( GTK_IS_ENTRY(prop->dlgitem) )
						gtk_entry_set_text(GTK_ENTRY(prop->dlgitem),tmp);
					else
						gtk_entry_set_text(GTK_ENTRY(GTK_BIN(prop->dlgitem)->child),tmp);
					g_free(tmp);	
				}		
			}
			gtk_widget_destroy(sdlg);
		}
		break;
	}/*switch*/
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
	GList *list = NULL,*gl=NULL;
	gint itnum, w, h, pos=0,pos2=0;
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
	
	table = gtk_table_new(itnum, 3, FALSE);
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
			
			gl = NULL;
			if ( FREFPROPERTY(list->data)->vlist )
			{
					GList *pomgl,*l;
					gl = fref_string_to_list(FREFPROPERTY(list->data)->vlist, ",");					
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
			}			
			
			switch ( 	FREFPROPERTY(list->data)->input_type )
			{
				case FREF_INP_ENTRY:
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
				break;
				case FREF_INP_LIST:
				case FREF_INP_LIST_ENTRY:
				{
					GList *pomgl=NULL;
					if (FREFPROPERTY(list->data)->input_type == FREF_INP_LIST)
						combo = gtk_combo_box_new_text();
					else
						combo = gtk_combo_box_entry_new_text();	
					if ( gl )
					{
						pomgl = g_list_first(gl);
						pos = pos2=0;
						while (pomgl)
						{
								gtk_combo_box_insert_text(GTK_COMBO_BOX(combo),pos,pomgl->data);
								if (FREFPROPERTY(list->data)->def != NULL && strcmp(FREFPROPERTY(list->data)->def,pomgl->data)==0)
									pos2=pos;
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
						 
						if ( GTK_IS_COMBO_BOX_ENTRY(combo) )
							gtk_entry_set_text(GTK_ENTRY(GTK_BIN(combo)->child),FREFPROPERTY(list->data)->def);
						else
							gtk_combo_box_set_active(GTK_COMBO_BOX(combo),pos2);	
					}	
					FREFPROPERTY(list->data)->dlgitem = combo;
					if (FREFPROPERTY(list->data)->description)
						gtk_tooltips_set_tip(FREFGUI(bfwin->fref)->argtips, combo,FREFPROPERTY(list->data)->description, "");					
				}
				break;
				case FREF_INP_COLOUR:
				case FREF_INP_FILE:				
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
				   combo = gtk_button_new_with_label("...");
					gtk_table_attach(GTK_TABLE(table), combo, 2, 3, itnum, itnum + 1,
									 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
									 (GtkAttachOptions) (0), 5, 5);									 
					FREFPROPERTY(list->data)->aux = bfwin;				 
					g_signal_connect(G_OBJECT(combo), "clicked", G_CALLBACK(fref_input_button_cb), FREFPROPERTY(list->data));	
					gtk_widget_show(combo);				 
				}					 
				break;
				case FREF_INP_COLOUR_LIST:				
				{
					GList *pomgl=NULL;
					combo = gtk_combo_box_entry_new_text();	
					if ( gl )
					{
						pomgl = g_list_first(gl);
						while (pomgl)
						{
								gtk_combo_box_insert_text(GTK_COMBO_BOX(combo),pos,pomgl->data);
								pomgl = g_list_next(pomgl);
						}
						g_list_free(gl);
					}						
					gtk_widget_show(combo);
					gtk_table_attach(GTK_TABLE(table), combo, 1, 2, itnum, itnum + 1,
									 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
									 (GtkAttachOptions) (0), 5, 5);
					if (FREFPROPERTY(list->data)->def != NULL)
					{						 
						gtk_entry_set_text(GTK_ENTRY(GTK_BIN(combo)->child),FREFPROPERTY(list->data)->def);
					}	
					FREFPROPERTY(list->data)->dlgitem = combo;
					if (FREFPROPERTY(list->data)->description)
						gtk_tooltips_set_tip(FREFGUI(bfwin->fref)->argtips, combo,FREFPROPERTY(list->data)->description, "");					
				   input = gtk_button_new_with_label("...");
					gtk_table_attach(GTK_TABLE(table), input, 2, 3, itnum, itnum + 1,
									 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
									 (GtkAttachOptions) (0), 5, 5);
					FREFPROPERTY(list->data)->aux = bfwin;				 									 
					g_signal_connect(G_OBJECT(input), "clicked", G_CALLBACK(fref_input_button_cb), FREFPROPERTY(list->data));					 
					gtk_widget_show(input);				 
				}					 
				break;

			} /* switch */		
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
		case FREF_EL_VAR:
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
		case FREF_EL_VAR:
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


gboolean  fref_keypressed(GtkWidget *widget,GdkEventKey *event,gpointer user_data)
{
	GtkTextBuffer *buffer;
	GtkTextIter it1,it2,it3;
	
	if (event->keyval == GDK_F1 && user_data!=NULL)
	{
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(BFWIN(user_data)->current_document->view));
		gtk_text_buffer_get_iter_at_mark(buffer,&it1,gtk_text_buffer_get_insert(buffer));
		it2=it1; it3=it1;
		if (gtk_text_iter_inside_word(&it1) )
		{
			gtk_text_iter_backward_word_start(&it2);
			gtk_text_iter_forward_word_end(&it3);
			fref_search(BFWIN(user_data),gtk_text_buffer_get_text(buffer,&it2,&it3,FALSE));
			gtk_widget_grab_focus(BFWIN(user_data)->current_document->view);									
		}		
		
	}
	return FALSE;
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
	gtk_tree_view_columns_autosize  (treeview)	;
}

static gboolean frefcb_event_mouseclick(GtkWidget * widget, GdkEventButton * event, Tbfwin * bfwin)
{
	Tfref_record *entry;

	if (widget != FREFGUI(bfwin->fref)->tree)
		return FALSE;
		
  if (event->type == GDK_2BUTTON_PRESS && event->button == 1) {	 /* double click  */
		entry = get_current_entry(bfwin);
		if (entry == NULL) return FALSE;
		 fref_insert_lcb(NULL,bfwin);
  } else if (event->type == GDK_BUTTON_PRESS && event->button == 3 ) { /* right click */
	   gtk_menu_popup(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),NULL,NULL,NULL,NULL,1,
                             gtk_get_current_event_time());
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
		gtk_tree_model_get_value(GTK_TREE_MODEL(user_data), arg1, STR_COLUMN, val);
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
			if ( fref_load_from_file((gchar *) g_value_peek_pointer(val), GTK_WIDGET(treeview),
						 GTK_TREE_STORE(user_data), arg1, info,bfwin) )
			{			 			
			 el = g_new0(Tfref_record,1);						 
			 el->etype = FREF_EL_REF;
			 el->data = info;
			 gtk_tree_store_set(GTK_TREE_STORE(user_data), arg1, PTR_COLUMN, el, -1);

			} 
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
	gtk_tree_view_columns_autosize  (treeview)	;
	gtk_tree_view_set_cursor(treeview,arg2,NULL,FALSE);
}

static GList *fref_get_entry_links(Tfref_record *entry)
{
	GList *lst=NULL,*lst2=NULL;
	
	if (!entry || !entry->data) return NULL;
	if (entry->etype!=FREF_EL_TAG &&
			entry->etype!= FREF_EL_FUNCTION &&
			entry->etype!=FREF_EL_VAR &&
			entry->etype!=FREF_EL_CSSPROP &&
			entry->etype!=FREF_EL_CSSSELECT &&			
			entry->etype!=FREF_EL_SNIPPET) return NULL;

	lst = g_list_first(FREFELEMENT(entry->data)->properties);
	while (lst)
	{
		if (FREFPROPERTY(lst->data)->ptype == FREF_EL_LINK)
		 lst2 = g_list_append(lst2,g_strstrip(FREFPROPERTY(lst->data)->description));
		lst = g_list_next(lst);
	}
	return lst2;
}

/* ----------------------------  INTERNAL LINKS STUFF (from gtk-demo) ----------------------------------------------------*/

static void fref_insert_link (GtkTextBuffer *buffer,GtkTextIter   *iter, gchar  *text, gchar  *page)
{
  GtkTextTag *tag;
  
  tag = gtk_text_buffer_create_tag (buffer, NULL, 
				    "foreground", "blue", 
				    "underline", PANGO_UNDERLINE_SINGLE, 
				    NULL);
  g_object_set_data (G_OBJECT (tag), "page", page);
  gtk_text_buffer_insert_with_tags (buffer, iter, text, -1, tag, NULL);
}

static void fref_follow_if_link (GtkWidget   *text_view,GtkTextIter *iter,Tbfwin *bfwin)
{
  GSList *tags = NULL, *tagp = NULL;
  gchar *command;

  tags = gtk_text_iter_get_tags (iter);
  for (tagp = tags;  tagp != NULL;  tagp = tagp->next)
    {
      GtkTextTag *tag = tagp->data;
      gchar *page = g_object_get_data (G_OBJECT (tag), "page");

      if (page != 0)
        {
           if (g_str_has_prefix(page,"http://"))
           {
					GList *tmplist = g_list_first(main_v->props.browsers);
					if (tmplist && tmplist->data) {
						gchar **arr = tmplist->data;
						command = g_strdup_printf(arr[1],page);
						system(command);
						g_free(command);
					}           
           } else
           {
        	  		fref_search(bfwin,page);
        	  }	
			  break;
        }
    }

  if (tags) 
    g_slist_free (tags);
}

static gboolean fref_link_event (GtkWidget *text_view, GdkEvent  *ev,Tbfwin *bfwin)
{
  GtkTextIter start, end, iter;
  GtkTextBuffer *buffer;
  GdkEventButton *event;
  gint x, y;

  if (ev->type != GDK_BUTTON_RELEASE)
    return FALSE;
  event = (GdkEventButton *)ev;
  if (event->button != 1)
    return FALSE;
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
  if (gtk_text_iter_get_offset (&start) != gtk_text_iter_get_offset (&end))
    return FALSE;
  gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view), 
                                         GTK_TEXT_WINDOW_WIDGET,
                                         event->x, event->y, &x, &y);
  gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (text_view), &iter, x, y);
  fref_follow_if_link (text_view, &iter,bfwin);
  return FALSE;
}

gboolean fref_hovering_over_link = FALSE;
GdkCursor *fref_hand_cursor = NULL;
GdkCursor *fref_regular_cursor = NULL;

static void fref_set_link_cursor(GtkTextView    *text_view,gint x,gint  y)
{
  GSList *tags = NULL, *tagp = NULL;
  GtkTextBuffer *buffer;
  GtkTextIter iter;
  gboolean hovering = FALSE;

  buffer = gtk_text_view_get_buffer (text_view);
  gtk_text_view_get_iter_at_location (text_view, &iter, x, y);
    tags = gtk_text_iter_get_tags (&iter);
  for (tagp = tags;  tagp != NULL;  tagp = tagp->next)
    {
      GtkTextTag *tag = tagp->data;
      gchar  *page = g_object_get_data (G_OBJECT (tag), "page");

      if (page != NULL) 
        {
          hovering = TRUE;
          break;
        }
    }

  if (hovering != fref_hovering_over_link)
    {
      fref_hovering_over_link = hovering;

      if (fref_hovering_over_link)
        gdk_window_set_cursor (gtk_text_view_get_window (text_view, GTK_TEXT_WINDOW_TEXT), fref_hand_cursor);
      else
        gdk_window_set_cursor (gtk_text_view_get_window (text_view, GTK_TEXT_WINDOW_TEXT), fref_regular_cursor);
    }

  if (tags) 
    g_slist_free (tags);
}

static gboolean fref_link_motion_event (GtkWidget      *text_view,  GdkEventMotion *event)
{
  gint x, y;
  gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view), 
                                         GTK_TEXT_WINDOW_WIDGET,
                                         event->x, event->y, &x, &y);
  fref_set_link_cursor (GTK_TEXT_VIEW (text_view), x, y);
  gdk_window_get_pointer (text_view->window, NULL, NULL, NULL);
  return FALSE;
}

static gboolean fref_link_visibility_event (GtkWidget          *text_view, GdkEventVisibility *event)
{
  gint wx, wy, bx, by;
  gdk_window_get_pointer (text_view->window, &wx, &wy, NULL);
  gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view), 
                                         GTK_TEXT_WINDOW_WIDGET,
                                         wx, wy, &bx, &by);
  fref_set_link_cursor(GTK_TEXT_VIEW (text_view), bx, by);
  return FALSE;
}

/* ----------------------------  END OF INTERNAL LINKS STUFF (from gtk-demo) ----------------------------------------------------*/

static void frefcb_cursor_changed(GtkTreeView * treeview, Tbfwin * bfwin)
{
 	Tfref_record *entry;
	gchar *tmpinfo = NULL; 
	GtkTextIter its,ite;
	GtkTextBuffer *buff = gtk_text_view_get_buffer(GTK_TEXT_VIEW(FREFGUI(bfwin->fref)->infoview));
	GList *lst=NULL;

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
					gtk_text_buffer_insert(buff,&its,tmpinfo,strlen(tmpinfo));
					g_free(tmpinfo);
				}
			}
			if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_ap))) {
				tmpinfo = g_strconcat(fref_prepare_info(entry, FR_INFO_ATTRS),NULL);
				if ( tmpinfo ) {
					gtk_text_buffer_get_end_iter(buff,&its);
					gtk_text_buffer_insert(buff,&its,tmpinfo,strlen(tmpinfo));
					g_free(tmpinfo);				
				}	
			}
			if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_note))) {
				tmpinfo = g_strconcat(fref_prepare_info(entry, FR_INFO_NOTES),NULL);
				if ( tmpinfo ) {
					gtk_text_buffer_get_end_iter(buff,&its);
					gtk_text_buffer_insert(buff,&its,tmpinfo,strlen(tmpinfo));
					g_free(tmpinfo);
				}	
			}						
			if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_example))) {
				tmpinfo = g_strconcat(fref_prepare_info(entry, FR_INFO_EXAMPLES),NULL);
				if ( tmpinfo ) {
					gtk_text_buffer_get_end_iter(buff,&its);
					gtk_text_buffer_insert(buff,&its,tmpinfo,strlen(tmpinfo));
					g_free(tmpinfo);
				}	
			}						
			if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_link))) {
				lst = fref_get_entry_links(entry);
				if (lst)
				{
					gtk_text_buffer_get_end_iter(buff,&its);
					tmpinfo=g_strdup("SEE ALSO:\n");
					gtk_text_buffer_insert(buff,&its,tmpinfo,strlen(tmpinfo));
					g_free(tmpinfo);
				}
				while (lst)
				{
					gtk_text_buffer_get_end_iter(buff,&its);
					tmpinfo=g_strconcat(lst->data,"\n",NULL);
					fref_insert_link(buff,&its,tmpinfo,lst->data);
					g_free(tmpinfo);
					lst=g_list_next(lst);					
				}
				g_list_free(lst);
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

static void frefcb_prefbtn_clicked(GtkWidget * button, Tbfwin * bfwin) {
	fref_popup_menu_rescan_lcb(button,bfwin);
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
			message_dialog_new(bfwin->main_window, 
							 		 GTK_MESSAGE_ERROR, 
							 		 GTK_BUTTONS_CLOSE, 
							 		 _("Error"), 
							 		 _("Perhaps you didn't load a library, or you did not select a library to search in."));
			return;			                 		
	}	
	
	if ( el == NULL)
		return;
	ret = g_hash_table_lookup(FREFINFO(el->data)->dictionary, phrase);
	if (!ret) {
						stf = g_strdup_printf(_("Element %s not found"),phrase);
						message_dialog_new(bfwin->main_window, 
										 		 GTK_MESSAGE_ERROR, 
										 		 GTK_BUTTONS_CLOSE, 
										 		 _("Element search"), 
										 		 stf);						
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


/* ------------------- EDITOR CALLBACKS ----------------------------------*/

static void frefcb_new_library(GtkWidget * widget, Tbfwin *bfwin) 
{
	GtkWidget *dialog;
	GtkWidget *e1,*e2;
	gchar *fname;
		
	dialog = fref_editor_td(_("New library"),_("Library name"),_("Description"),bfwin,&e1,&e2);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) 
							{
									if ( strcmp("",gtk_entry_get_text(GTK_ENTRY(e1))) != 0 )
									{
										gchar *userdir = g_strconcat(g_get_home_dir(), "/."PACKAGE"/", NULL);
										FILE *ff;
										gchar *pomstr;
										
										fname = g_strconcat(userdir,"bflib_",gtk_entry_get_text(GTK_ENTRY(e1)),".xml",NULL);
										/* check if file exists */
										ff = fopen(fname,"w");
										pomstr = g_strdup_printf("<ref name=\"%s\" description=\"%s\" version=\"2\">\n</ref>",
																							gtk_entry_get_text(GTK_ENTRY(e1)),
																							fref_editor_getviewtext(GTK_TEXT_VIEW(e2))
																							);
										fwrite(pomstr,strlen(pomstr),1,ff);
										fclose(ff);
										fref_rescan_dir(userdir);
										g_free(pomstr);
										g_free(userdir);
										fill_toplevels(FREFDATA(main_v->frefdata), TRUE);										
									} else {
										message_dialog_new(bfwin->main_window, 
								 								 GTK_MESSAGE_ERROR, 
								 								 GTK_BUTTONS_CLOSE, 
																 _("Error"), 
																 _("Library has to have a name."));
									}
									gtk_widget_destroy(dialog);
							} else
								gtk_widget_destroy(dialog);
}

static void frefcb_delete_library(GtkWidget * widget, Tbfwin *bfwin) 
{
	const gchar *buttons[] = { GTK_STOCK_NO, GTK_STOCK_YES, NULL };
	gint retval;	
	GtkTreePath *path;
	GtkTreeViewColumn *col;
	
	
	if (fref_ro_for_current(bfwin))
	{
		message_dialog_new(bfwin->main_window, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, 
																 _("Error"), 
																 _("This library is readonly."));
		return;														 
	}	
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
					gchar *userdir = g_strconcat(g_get_home_dir(), "/."PACKAGE"/", NULL);		
					retval = message_dialog_new_multi(bfwin->main_window,
																 GTK_MESSAGE_QUESTION,
																 buttons,
																 _("Delete library?"),
																 g_value_peek_pointer(val));
					if (retval == 0)	return;
					unlink(g_value_peek_pointer(val));
					fref_rescan_dir(userdir);	
					g_free(userdir);
					fill_toplevels(FREFDATA(main_v->frefdata), TRUE);																		
		}
		g_value_unset(val);
		g_free(val);
	} else {
		message_dialog_new(bfwin->main_window, 
 								 GTK_MESSAGE_ERROR, 
 								 GTK_BUTTONS_CLOSE, 
								 _("Error"), 
								 _("You have to select a reference."));
		return;
	}	
}


/* new entity with title and description */

static void frefcb_new_td(GtkWidget * widget, Tbfwin *bfwin, gint type, gboolean def) {
	GtkTreePath *path;
	GtkTreeViewColumn *col;
	Tfref_record *entry,*rec=NULL;
	Tfref_note *note;
	GtkWidget *dialog=NULL;
	GtkWidget *e1,*e2;
	GtkTreeIter iter,auxit;

	if (fref_ro_for_current(bfwin))
	{
		message_dialog_new(bfwin->main_window, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, 
																 _("Error"), 
																 _("This library is readonly."));
		return;														 
	}	

		
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), &path, &col);
	if (path == NULL) return;
	entry = get_current_entry(bfwin);
	if (!entry) {
		message_dialog_new(bfwin->main_window, 
 								 GTK_MESSAGE_ERROR, 
 								 GTK_BUTTONS_CLOSE, 
								 _("Error"), 
								_("Reference not loaded."));
		return;
	}
	switch (type)
	{
	case FREF_EL_NOTE:	
	{
 	   if  ( entry->etype!=FREF_EL_REF && entry->etype!=FREF_EL_GROUP &&
			entry->etype!=FREF_EL_FUNCTION && entry->etype!=FREF_EL_TAG &&
			entry->etype!=FREF_EL_VAR && entry->etype!=FREF_EL_CSSPROP && 
			entry->etype!=FREF_EL_CSSSELECT )
		{
			message_dialog_new(bfwin->main_window, 
 									 GTK_MESSAGE_ERROR, 
 									 GTK_BUTTONS_CLOSE, 
									 _("Bad element"), 
									 _("You can define notes for: library, group or element."));
			return;
		}
		dialog = fref_editor_td(_("New note"),_("Title"),_("Text"),bfwin,&e1,&e2);
	}	
	break;
	case FREF_EL_GROUP:	
	{
 	   if  ( entry->etype!=FREF_EL_REF && entry->etype!=FREF_EL_GROUP )
		{
			message_dialog_new(bfwin->main_window, 
 									 GTK_MESSAGE_ERROR, 
 									 GTK_BUTTONS_CLOSE, 
									 _("Bad element"), 
									 _("You can define group for: library or group."));
			return;
		}
		dialog = fref_editor_td(_("New group"),_("Name"),_("Description"),bfwin,&e1,&e2);
	}		
	break;	
	case FREF_EL_DESCR:	
	{
		if (def)
		{
			dialog = fref_editor_td(_("New description def"),_("Identifier"),_("Description text"),bfwin,&e1,&e2);
		}	
	}		
	break;	
	case FREF_EL_VALUES:	
	{
		if (def)
		{
			dialog = fref_editor_td(_("New values def"),_("Identifier"),_("Values separated by comma"),bfwin,&e1,&e2);
		}	
	}		
	break;	

 } /* switch */	
	
 
	
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) 
	{
		if ( strcmp(gtk_entry_get_text(GTK_ENTRY(e1)),"") != 0 )
		{
			Tfref_info* inf=NULL;
			inf = fref_refinfo_for_current(bfwin);
			switch (type)
			{
				case FREF_EL_NOTE:
					note = g_new0(Tfref_note,1);
					note->title = xmlStrdup(gtk_entry_get_text(GTK_ENTRY(e1)));
					note->text = xmlStrdup(fref_editor_getviewtext(GTK_TEXT_VIEW(e2)));
					if (inf == NULL)			
					{
						xmlFree(note->title);
						xmlFree(note->text);
						g_free(note);
						return;
					}											
			
					rec = fref_insert_into_commons(inf,fref_unique_name(-1),type,note,FALSE);
					if ( !rec )
					{
						xmlFree(note->title);
						xmlFree(note->text);
						g_free(note);
						gtk_widget_destroy(dialog);
						return;
					}								
					if ( entry->etype==FREF_EL_REF || entry->etype==FREF_EL_GROUP )
					{
						gtk_tree_model_get_iter(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),&iter, path);			
						gtk_tree_store_append(GTK_TREE_STORE(FREFDATA(main_v->frefdata)->store), &auxit, &iter);				
						gtk_tree_store_set(GTK_TREE_STORE(FREFDATA(main_v->frefdata)->store), &auxit, 
						PIXMAP_COLUMN,FREFDATA(main_v->frefdata)->icon_note,
						STR_COLUMN,note->title, FILE_COLUMN, NULL, PTR_COLUMN,rec, -1);	
					}
					else /* note for element */
					{
						FREFELEMENT(entry->data)->notes = g_list_append(FREFELEMENT(entry->data)->notes,note);
					}	
				break;
				case FREF_EL_GROUP:
					note = g_new0(Tfref_note,1);
					note->title = xmlStrdup(gtk_entry_get_text(GTK_ENTRY(e1)));
					note->text = xmlStrdup(fref_editor_getviewtext(GTK_TEXT_VIEW(e2)));
					if (inf == NULL)			
					{
						xmlFree(note->title);
						xmlFree(note->text);
						g_free(note);
						return;
					}											
					
					rec = fref_insert_into_commons(inf,fref_unique_name(-1),type,note,FALSE);
					if ( !rec )
					{
						xmlFree(note->title);
						xmlFree(note->text);
						g_free(note);
						gtk_widget_destroy(dialog);
						return;
					}								
					gtk_tree_model_get_iter(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),&iter, path);			
					gtk_tree_store_append(GTK_TREE_STORE(FREFDATA(main_v->frefdata)->store), &auxit, &iter);
				
					gtk_tree_store_set(GTK_TREE_STORE(FREFDATA(main_v->frefdata)->store), &auxit, 
					PIXMAP_COLUMN,FREFDATA(main_v->frefdata)->icon_group,
					STR_COLUMN,note->title, FILE_COLUMN, NULL, PTR_COLUMN,rec, -1);	
				break;	
				case FREF_EL_DESCR:
					if (inf)
						fref_insert_into_commons(inf,g_strdup(gtk_entry_get_text(GTK_ENTRY(e1))),FREF_EL_DESCR,
						         fref_editor_getviewtext(GTK_TEXT_VIEW(e2)),TRUE);
				break;
				case FREF_EL_VALUES:
					if (inf)
						fref_insert_into_commons(inf,g_strdup(gtk_entry_get_text(GTK_ENTRY(e1))),FREF_EL_VALUES,
						         fref_editor_getviewtext(GTK_TEXT_VIEW(e2)),TRUE);				
				break;								
			}		
			fref_save_ref(bfwin);
			frefcb_cursor_changed(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), bfwin);
			/*gtk_tree_view_expand_row(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree),path,FALSE);*/
		}		
		else {
					message_dialog_new(bfwin->main_window, 
 								 GTK_MESSAGE_ERROR, 
 								 GTK_BUTTONS_CLOSE, 
								 _("Error"), 
								_("Element has to have a name."));
		}
	}		
	gtk_widget_destroy(dialog);
}

static void frefcb_new_note(GtkWidget * widget, Tbfwin *bfwin) {
	frefcb_new_td(widget, bfwin, FREF_EL_NOTE,FALSE);
}

static void frefcb_new_group(GtkWidget * widget, Tbfwin *bfwin) {
	frefcb_new_td(widget, bfwin, FREF_EL_GROUP,FALSE);
}

static void frefcb_new_re_descr(GtkWidget * widget, Tbfwin *bfwin) {
	frefcb_new_td(widget, bfwin, FREF_EL_DESCR,TRUE);
}

static void frefcb_new_re_values(GtkWidget * widget, Tbfwin *bfwin) {
	frefcb_new_td(widget, bfwin, FREF_EL_VALUES,TRUE);
}

static void frefcb_new_re_prop(GtkWidget * widget, Tbfwin *bfwin) {
	GtkWidget *dlg=NULL;
	Tfref_property *prop;
	xmlChar *vals;
	Tfref_record *rec;
	Tfref_info *inf;
	Tfref_propdialog_data *data;
	
	if (fref_ro_for_current(bfwin))
	{
		message_dialog_new(bfwin->main_window, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, 
																 _("Error"), 
																 _("This library is readonly."));
		return;														 
	}	

	data = g_new0(Tfref_propdialog_data,1);
	dlg = fref_editor_propdialog("New property def",data,TRUE);
	if (!dlg ) return;
	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK )
	{
		
		xmlChar *name = xmlStrdup(gtk_entry_get_text(GTK_ENTRY(data->e_name)));
		xmlChar *id = xmlStrdup(gtk_entry_get_text(GTK_ENTRY(data->e_id)));
		if ( xmlStrcmp(name, (const xmlChar *) "")!=0 || xmlStrcmp(id, (const xmlChar *) "")!=0)
		{
			prop = g_new0(Tfref_property,1);
			switch (gtk_combo_box_get_active(GTK_COMBO_BOX(data->e_kind)) )
			{
				case 0: prop->ptype = FREF_EL_LINK;break;
				case 1: prop->ptype = FREF_EL_EXAMPLE;break;
				case 2: prop->ptype = FREF_EL_DEPEND;break;
				case 3: prop->ptype = FREF_EL_ATTR;break;
				case 4: prop->ptype = FREF_EL_PARAM;break;
				case 5: prop->ptype = FREF_EL_RETURN;break;
				case 6: prop->ptype = FREF_EL_PARENTS;break;
			}
			prop->name = name;
			prop->id = id;
			if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->e_required)) )
				prop->required = TRUE;
			else
				prop->required = FALSE;
			if (strcmp("",gtk_entry_get_text(GTK_ENTRY(data->e_type)))!=0)
				prop->type = xmlStrdup(gtk_entry_get_text(GTK_ENTRY(data->e_type)));
			if (strcmp("",gtk_entry_get_text(GTK_ENTRY(data->e_default)))!=0)
				prop->def = xmlStrdup(gtk_entry_get_text(GTK_ENTRY(data->e_default)));
			switch (gtk_combo_box_get_active(GTK_COMBO_BOX(data->e_input)) )
			{
				case 0: prop->input_type = FREF_INP_ENTRY;break;
				case 1: prop->input_type = FREF_INP_LIST;break;
				case 2: prop->input_type = FREF_INP_LIST_ENTRY;break;
				case 3: prop->input_type = FREF_INP_COLOUR;break;
				case 4: prop->input_type = FREF_INP_COLOUR_LIST;break;
				case 5: prop->input_type = FREF_INP_FILE;break;
			}
				
			inf = fref_refinfo_for_current(bfwin);
			vals = xmlStrdup(fref_editor_getviewtext(GTK_TEXT_VIEW(data->e_values)));
			if ( xmlStrcmp(vals, (const xmlChar *) "")!=0 )
				{
					rec = fref_insert_into_commons(inf,fref_unique_name(-1),FREF_EL_VALUES,vals,FALSE);
					prop->vlist = vals;
				} else xmlFree(vals);	
			vals = xmlStrdup(fref_editor_getviewtext(GTK_TEXT_VIEW(data->e_description)));
			if ( xmlStrcmp(vals, (const xmlChar *) "")!=0 )
				{
					rec = fref_insert_into_commons(inf,fref_unique_name(-1),FREF_EL_DESCR,vals,FALSE);
					prop->description = vals;
				} else xmlFree(vals);	
			rec = fref_insert_into_commons(inf,prop->id,prop->ptype,prop,TRUE);		
			fref_save_ref(bfwin);		
		 	frefcb_cursor_changed(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), bfwin);
		} else {
			message_dialog_new(bfwin->main_window, 
 									 GTK_MESSAGE_ERROR, 
 									 GTK_BUTTONS_CLOSE, 
									 _("Missing value"), 
									 _("Property has to have a name."));
		}
	}
	gtk_widget_destroy(dlg);	
}


static void add_common(gpointer key,gpointer value,gpointer data)
{
   gchar *name = (gchar*)key;
   GtkTreeIter it;
   GtkListStore *store = (GtkListStore*)data;
   
   if ( name[0]!='_' && name[1]!='_')
   {
		gtk_list_store_append(store, &it);
		Tfref_record *rec = FREFRECORD(value);
		switch (rec->etype)
		{
			case FREF_EL_DESCR:
				gtk_list_store_set(store, &it, 0,rec,1,g_strconcat("description (",
				key,")",NULL), -1);
			break;
			case FREF_EL_VALUES:
				gtk_list_store_set(store, &it, 0,rec,1,g_strconcat("values (",
				key,")",NULL), -1);
			break;
			case FREF_EL_PARAM:
			case FREF_EL_ATTR:
			case FREF_EL_RETURN:
			case FREF_EL_EXAMPLE:
			case FREF_EL_DEPEND:
			case FREF_EL_LINK:		
				gtk_list_store_set(store, &it, 0,rec,1,g_strconcat(FREFPROPERTY(rec->data)->name," (",
				FREFPROPERTY(rec->data)->id,")",NULL), -1);	
			break;	
		}	   
	}	
}

typedef struct {
	GtkLabel *label;
	GtkListStore *store;
} Tfref_reudata;

static void frefcb_reusable_changed(GtkTreeView *treeview,gpointer user_data)
{
	GtkTreePath *path;
	GtkTreeViewColumn *col;
	GtkTreeIter auxit;
	GValue *val;
	Tfref_reudata *data = (Tfref_reudata*)user_data;
	Tfref_record *rec;
	gchar *ret,*tofree="";
	
	gtk_tree_view_get_cursor(treeview, &path, &col);
	if (path==NULL) return;

	gtk_tree_model_get_iter(GTK_TREE_MODEL(data->store),&auxit,path);
	val = g_new0(GValue, 1);
	gtk_tree_model_get_value(GTK_TREE_MODEL(data->store),&auxit,0, val);
	if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val)) {
				rec =  (Tfref_record*)g_value_peek_pointer(val);
				switch (rec->etype)
				{
					case FREF_EL_DESCR:
						gtk_label_set_text(data->label,rec->data);
					break;
					case FREF_EL_VALUES:
						gtk_label_set_text(data->label,rec->data);
					break;
					case FREF_EL_PARAM:
						if (strcmp(tofree,"")==0)
							tofree = "parameter";
					case FREF_EL_ATTR:
						if (strcmp(tofree,"")==0)					
							tofree = "attribute";
					case FREF_EL_RETURN:
						if (strcmp(tofree,"")==0)					
							tofree = "return type";
					case FREF_EL_EXAMPLE:
						if (strcmp(tofree,"")==0)					
							tofree = "example";
					case FREF_EL_DEPEND:
						if (strcmp(tofree,"")==0)					
							tofree = "dependency";
					case FREF_EL_LINK:		
						if (strcmp(tofree,"")==0)					
							tofree = "link";
						ret = g_strdup_printf("\nKind:%s\nType:%s\nDefault:%s\nRequired:%s\n",
									tofree,
									fref_notnull_str(FREFPROPERTY(rec->data)->type),
									fref_notnull_str(FREFPROPERTY(rec->data)->def),
									fref_bool_str(FREFPROPERTY(rec->data)->required));
						gtk_label_set_text(data->label,ret);
					break;	
				} /* switch */
	}
	g_value_unset(val);
	g_free(val);	
	
}

typedef struct {
	Tbfwin *bfwin;
	GtkListStore *store;
	GtkTreeView *tree;
} Tfref_adddef_data;

static void frefcb_show_reusable(GtkWidget * widget, Tfref_adddef_data *data) {
	Tfref_info *inf = fref_refinfo_for_current(data->bfwin);
	GtkWidget *winfo,*listview,*infolab;
	GtkListStore *store;
	Tfref_reudata *rd;
	GtkTreeIter it,it2;
	GValue *val=NULL,*val2;
	GtkTreePath *path=NULL;
	
	
	if ( !inf ) return;
	
	winfo = fref_editor_list_dialog(data->bfwin,_("Definitions"),TRUE,TRUE,&listview,&store,&infolab);
	g_hash_table_foreach(inf->commons,add_common,store);
	gtk_window_resize(GTK_WINDOW(winfo), 250,300);
	rd = g_new0(Tfref_reudata,1);
	rd->label = GTK_LABEL(infolab);
	rd->store = store;
	g_signal_connect(G_OBJECT(listview),"cursor-changed",G_CALLBACK(frefcb_reusable_changed),rd);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store),1,GTK_SORT_ASCENDING);

	if ( gtk_dialog_run(GTK_DIALOG(winfo)) == GTK_RESPONSE_OK)
	{
		gtk_tree_view_get_cursor(GTK_TREE_VIEW(listview), &path, NULL);
		if (path!=NULL)
		{
			gtk_tree_model_get_iter(GTK_TREE_MODEL(store),&it2,path);	
			val = g_new0(GValue, 1);
			gtk_tree_model_get_value(GTK_TREE_MODEL(store),&it2,0, val);
			if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val))
			{
					Tfref_record *rec =  (Tfref_record*)g_value_peek_pointer(val);
					val2 = g_new0(GValue, 1);
					gtk_tree_model_get_value(GTK_TREE_MODEL(store),&it2,1, val2);
					
					gtk_list_store_append(data->store,&it);
					gtk_list_store_set(data->store,&it,0,rec,1,g_value_peek_pointer(val2),2,1,-1);
					g_value_unset(val2);						
					g_free(val2);
			}
			g_value_unset(val);	
			g_free(val);			
		}	
	}
	gtk_widget_destroy(winfo);	
}


static void frefcb_new_prop(GtkWidget * widget, Tfref_adddef_data *dt) {
	GtkWidget *dlg=NULL;
	Tfref_property *prop;
	xmlChar *vals;
	Tfref_record *rec;
	Tfref_info *inf;
	Tfref_propdialog_data *data;
	GtkTreeIter it;
	

	data = g_new0(Tfref_propdialog_data,1);
	dlg = fref_editor_propdialog(_("New property"),data,FALSE);
	if (!dlg ) return;
	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK )
	{
		
		xmlChar *name = xmlStrdup(gtk_entry_get_text(GTK_ENTRY(data->e_name)));
		if ( xmlStrcmp(name, (const xmlChar *) "")!=0 )
		{
			prop = g_new0(Tfref_property,1);
			switch (gtk_combo_box_get_active(GTK_COMBO_BOX(data->e_kind)) )
			{
				case 0: prop->ptype = FREF_EL_LINK;break;
				case 1: prop->ptype = FREF_EL_EXAMPLE;break;
				case 2: prop->ptype = FREF_EL_DEPEND;break;
				case 3: prop->ptype = FREF_EL_ATTR;break;
				case 4: prop->ptype = FREF_EL_PARAM;break;
				case 5: prop->ptype = FREF_EL_RETURN;break;
				case 6: prop->ptype = FREF_EL_PARENTS;break;
			}
			prop->name = name;
			prop->id = NULL;
			if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->e_required)) )
				prop->required = TRUE;
			else
				prop->required = FALSE;
			if (strcmp("",gtk_entry_get_text(GTK_ENTRY(data->e_type)))!=0)
				prop->type = xmlStrdup(gtk_entry_get_text(GTK_ENTRY(data->e_type)));
			if (strcmp("",gtk_entry_get_text(GTK_ENTRY(data->e_default)))!=0)
				prop->def = xmlStrdup(gtk_entry_get_text(GTK_ENTRY(data->e_default)));
			switch (gtk_combo_box_get_active(GTK_COMBO_BOX(data->e_input)) )
			{
				case 0: prop->input_type = FREF_INP_ENTRY;break;
				case 1: prop->input_type = FREF_INP_LIST;break;
				case 2: prop->input_type = FREF_INP_LIST_ENTRY;break;
				case 3: prop->input_type = FREF_INP_COLOUR;break;
				case 4: prop->input_type = FREF_INP_COLOUR_LIST;break;
				case 5: prop->input_type = FREF_INP_FILE;break;
			}
				
			inf = fref_refinfo_for_current(dt->bfwin);
			vals = xmlStrdup(fref_editor_getviewtext(GTK_TEXT_VIEW(data->e_values)));
			if ( xmlStrcmp(vals, (const xmlChar *) "")!=0 )
				{
					prop->vlist = vals;
				} else xmlFree(vals);	
			vals = xmlStrdup(fref_editor_getviewtext(GTK_TEXT_VIEW(data->e_description)));
			if ( xmlStrcmp(vals, (const xmlChar *) "")!=0 )
				{
					prop->description = vals;
				} else xmlFree(vals);	
			
				rec =  g_new0(Tfref_record,1);
				rec->etype = prop->ptype;
				rec->data = prop;
				gtk_list_store_append(dt->store,&it);
				gtk_list_store_set(dt->store,&it,0,rec,1,prop->name,2,0,-1);	
				
		} else {
			message_dialog_new(dt->bfwin->main_window, 
 									 GTK_MESSAGE_ERROR, 
 									 GTK_BUTTONS_CLOSE, 
									 _("Missing value"), 
									 _("Property has to have a name."));
		}
	}
	gtk_widget_destroy(dlg);	
}

static void frefcb_del_prop(GtkWidget * widget, Tfref_adddef_data *dt)
{
	GtkTreeIter it;
	GtkTreePath *path=NULL;
	GValue *val=NULL;
	gint k;
	
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(dt->tree), &path, NULL);
	if (!path) return;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(dt->store),&it,path);		
	val = g_new0(GValue, 1);
	gtk_tree_model_get_value(GTK_TREE_MODEL(dt->store),&it,0, val);
	if (G_IS_VALUE(val) && g_value_fits_pointer(val) && g_value_peek_pointer(val))
	{
					Tfref_record *rec = (Tfref_record*)g_value_peek_pointer(val);
					gtk_tree_model_get(GTK_TREE_MODEL(dt->store),&it,2, &k,-1);
					if (!k)
					{
						switch (rec->etype)
						{
							case FREF_EL_DESCR:
								xmlFree(rec->data);								
								g_free(rec);
							case FREF_EL_VALUES:
								g_free(rec->data);
								g_free(rec);
							break;
							default:
								if (FREFPROPERTY(rec->data)->vlist) g_free(FREFPROPERTY(rec->data)->vlist);
								if (FREFPROPERTY(rec->data)->description) xmlFree(FREFPROPERTY(rec->data)->description);
								fref_free_record(NULL,rec,dt->bfwin);
						}						
					}
					gtk_list_store_remove(dt->store,&it);
	}
	g_value_unset(val);	
	g_free(val);						
}


static void frefcb_new_element(GtkWidget * widget, Tbfwin *bfwin) 
{
	GtkWidget *dlg=NULL;
	Tfref_elemdialog_data *data = g_new0(Tfref_elemdialog_data,1);
	GtkListStore *store;
	Tfref_adddef_data *defdata;
	Tfref_reudata *rdata;
	xmlChar *vals;
	Tfref_info *inf=NULL;
	GtkTreeIter iter,auxit;
	GtkTreePath *path;
	Tfref_record *rec=NULL;

	if (fref_ro_for_current(bfwin))
	{
		message_dialog_new(bfwin->main_window, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, 
																 _("Error"), 
																 _("This library is readonly."));
		return;														 
	}	

	
	 rec = get_current_entry(bfwin);		
	 if ( !rec ) return;
	 if (rec->etype != FREF_EL_REF && rec->etype != FREF_EL_GROUP )
	 {
		message_dialog_new(bfwin->main_window, 
 								 GTK_MESSAGE_ERROR, 
 								 GTK_BUTTONS_CLOSE, 
								 _("Bad element"), 
								 _("Please select library or group."));
	 	return;
	 }
	 dlg = fref_editor_elemdialog(_("New element"),data,bfwin);
	 gtk_window_resize(GTK_WINDOW(dlg), 500,400);
	 if ( dlg )
	 {
	 	store = gtk_list_store_new(3,G_TYPE_POINTER,G_TYPE_STRING,G_TYPE_INT);
	 	gtk_tree_view_set_model(GTK_TREE_VIEW(data->e_proplist),GTK_TREE_MODEL(store));
	 	defdata = g_new0(Tfref_adddef_data,1);
	 	defdata->bfwin = bfwin;
	 	defdata->store = store;
	 	defdata->tree = GTK_TREE_VIEW(data->e_proplist);
	 	g_signal_connect(G_OBJECT(data->btn_addref),"clicked",G_CALLBACK(frefcb_show_reusable),defdata);
	 	rdata = g_new0(Tfref_reudata,1);
	 	rdata->store = store;
	 	rdata->label = GTK_LABEL(data->e_propinfo);	 	
	 	g_signal_connect(G_OBJECT(data->e_proplist),"cursor-changed",G_CALLBACK(frefcb_reusable_changed),rdata);
	 	g_signal_connect(G_OBJECT(data->btn_add),"clicked",G_CALLBACK(frefcb_new_prop),defdata);
	 	g_signal_connect(G_OBJECT(data->btn_del),"clicked",G_CALLBACK(frefcb_del_prop),defdata);
	 	if (gtk_dialog_run(GTK_DIALOG(dlg))==GTK_RESPONSE_OK)
	 	{
			xmlChar *name = xmlStrdup(gtk_entry_get_text(GTK_ENTRY(data->e_name)));
			if ( xmlStrcmp(name, (const xmlChar *) "")!=0 )	 	
			{
				Tfref_element *el = g_new0(Tfref_element,1);
				el->name = name;
				switch (gtk_combo_box_get_active(GTK_COMBO_BOX(data->e_kind)) )
				{
					case 0: el->etype = FREF_EL_FUNCTION;break;
					case 1: el->etype = FREF_EL_TAG;break;
					case 2: el->etype = FREF_EL_VAR;break;
					case 3: el->etype = FREF_EL_CSSPROP;break;
					case 4: el->etype = FREF_EL_CSSSELECT;break;
					case 5: el->etype = FREF_EL_SNIPPET;break;
				}
				inf = fref_refinfo_for_current(bfwin);	
				vals = xmlStrdup(fref_editor_getviewtext(GTK_TEXT_VIEW(data->e_description)));
				if ( xmlStrcmp(vals, (const xmlChar *) "")!=0 )
				{
					fref_insert_into_commons(inf,fref_unique_name(-1),FREF_EL_DESCR,vals,FALSE);
					el->description = vals;
				} else xmlFree(vals);	
				vals = xmlStrdup(fref_editor_getviewtext(GTK_TEXT_VIEW(data->e_insert)));
				if ( xmlStrcmp(vals, (const xmlChar *) "")!=0 )
				{
					el->insert = vals;
				} else xmlFree(vals);	
				/* properties ! */
				if ( gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store),&iter) )
				{
					while ( 1 )
					{
						gint k;
						gtk_tree_model_get(GTK_TREE_MODEL(store),&iter,2, &k,-1);
						gtk_tree_model_get(GTK_TREE_MODEL(store),&iter,0, &rec,-1);
						if ( k )
						{
								el->properties = g_list_append(el->properties,rec->data);
						}	
						else
						{						
							fref_insert_into_commons(inf,fref_unique_name(-1),rec->etype,rec->data,FALSE);
							el->properties = g_list_append(el->properties,rec->data);
						}
						if  ( !gtk_tree_model_iter_next(GTK_TREE_MODEL(store),&iter) ) break;
					}
				}
				rec = fref_insert_into_commons(inf,el->name,el->etype,el,FALSE);
				gtk_tree_view_get_cursor(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), &path, NULL);
				gtk_tree_model_get_iter(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),&iter, path);			
				gtk_tree_store_append(GTK_TREE_STORE(FREFDATA(main_v->frefdata)->store), &auxit, &iter);				
				gtk_tree_store_set(GTK_TREE_STORE(FREFDATA(main_v->frefdata)->store), &auxit, 
					STR_COLUMN,el->name, FILE_COLUMN, NULL, PTR_COLUMN,rec, -1);	
				switch (el->etype)
				{
					case FREF_EL_FUNCTION:gtk_tree_store_set(GTK_TREE_STORE(FREFDATA(main_v->frefdata)->store), &auxit, PIXMAP_COLUMN,
								FREFDATA(main_v->frefdata)->icon_function, -1);break;
					case FREF_EL_TAG:gtk_tree_store_set(GTK_TREE_STORE(FREFDATA(main_v->frefdata)->store), &auxit, PIXMAP_COLUMN,
								FREFDATA(main_v->frefdata)->icon_tag, -1);break;
					case FREF_EL_VAR:gtk_tree_store_set(GTK_TREE_STORE(FREFDATA(main_v->frefdata)->store), &auxit, PIXMAP_COLUMN,
								FREFDATA(main_v->frefdata)->icon_var, -1);break;
					case FREF_EL_CSSPROP:
					case FREF_EL_CSSSELECT:
					gtk_tree_store_set(GTK_TREE_STORE(FREFDATA(main_v->frefdata)->store), &auxit, PIXMAP_COLUMN,
								FREFDATA(main_v->frefdata)->icon_css, -1);break;
					case FREF_EL_SNIPPET:gtk_tree_store_set(GTK_TREE_STORE(FREFDATA(main_v->frefdata)->store), &auxit, PIXMAP_COLUMN,
								FREFDATA(main_v->frefdata)->icon_snippet, -1);break;				
				}	
				fref_save_ref(bfwin);
				frefcb_cursor_changed(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), bfwin);
			} else 
			{
				message_dialog_new(bfwin->main_window, 
 										 GTK_MESSAGE_ERROR, 
 										 GTK_BUTTONS_CLOSE, 
										 _("No name"), 
										 _("Element has to have a name."));
				xmlFree(name);
			}	
	 	}
	 	g_free(defdata);
	 	g_free(rdata);
	 	gtk_widget_destroy(dlg);
	 }	 
	 g_free(data);
}

static gboolean remove_me(gpointer key,gpointer value,gpointer user_data)
{
	if ( value == user_data ) return TRUE;
	return FALSE;
}

static gboolean check_me(gpointer key,gpointer element,gpointer rec)
{
	Tfref_record *rr = FREFRECORD(element);
	if (	rr->etype != FREF_EL_FUNCTION &&
			rr->etype != FREF_EL_TAG &&
			rr->etype != FREF_EL_VAR &&
			rr->etype != FREF_EL_CSSPROP &&
			rr->etype != FREF_EL_CSSSELECT &&
			rr->etype != FREF_EL_SNIPPET
	   ) return FALSE;
	GList *lst = g_list_first(FREFELEMENT(rr->data)->properties);
	if (lst==NULL) return FALSE;
	while (lst)
	{
		if (lst->data == FREFRECORD(rec)->data) return TRUE;
		lst = g_list_next(lst);
	}
	return FALSE;
}

static void frefcb_del_reusable(GtkWidget * widget, Tbfwin *bfwin) 
{
	Tfref_info *inf = fref_refinfo_for_current(bfwin);
	GtkWidget *winfo,*listview,*infolab;
	GtkListStore *store;
	Tfref_reudata *rd;
	GtkTreeIter it2;
	GtkTreePath *path=NULL;
	gboolean used;
	

	if (fref_ro_for_current(bfwin))
	{
		message_dialog_new(bfwin->main_window, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, 
																 _("Error"), 
																 _("This library is readonly."));
		return;														 
	}	

	
	if ( !inf ) return;
	
	winfo = fref_editor_list_dialog(bfwin,_("Definitions"),TRUE,TRUE,&listview,&store,&infolab);
	g_hash_table_foreach(inf->commons,add_common,store);
	gtk_window_resize(GTK_WINDOW(winfo), 250,300);
	rd = g_new0(Tfref_reudata,1);
	rd->label = GTK_LABEL(infolab);
	rd->store = store;
	g_signal_connect(G_OBJECT(listview),"cursor-changed",G_CALLBACK(frefcb_reusable_changed),rd);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store),1,GTK_SORT_ASCENDING);

	if ( gtk_dialog_run(GTK_DIALOG(winfo)) == GTK_RESPONSE_OK)
	{
		gtk_tree_view_get_cursor(GTK_TREE_VIEW(listview), &path, NULL);
		if (path!=NULL)
		{
			Tfref_record *rec=NULL;
			gchar *nm;
			gtk_tree_model_get_iter(GTK_TREE_MODEL(store),&it2,path);	
			gtk_tree_model_get(GTK_TREE_MODEL(store),&it2,0, &rec,-1);
			gtk_tree_model_get(GTK_TREE_MODEL(store),&it2,1, &nm,-1);
			/* check if property is used */
			used = FALSE;
			if ( g_hash_table_find(inf->elements,check_me,rec) != NULL ) used = TRUE;
			   
			if (rec && !used)
			{
				const gchar *buttons[] = { GTK_STOCK_NO, GTK_STOCK_YES, NULL };					
				gchar *pstr = g_strdup_printf(_("Do you really want to delete %s?"), nm);
				gint retval = message_dialog_new_multi(bfwin->main_window,
																	GTK_MESSAGE_QUESTION,
																 	buttons,
																 	_("Delete definition?"),
																 	pstr);
				g_free(pstr);
				if (retval != 0)
				{
					g_hash_table_foreach_remove(inf->commons,remove_me,rec);
					fref_free_record(NULL,rec,bfwin);
					fref_save_ref(bfwin);
					frefcb_cursor_changed(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), bfwin);
				}
			}
			else
			{
					message_dialog_new(bfwin->main_window, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, 
																 _("Error"), 
																 _("This property is used by some elements. Cannot delete."));
			
			}
		}	
	}
	gtk_widget_destroy(winfo);		
}

static void frefcb_del_entity(GtkWidget * widget, Tbfwin *bfwin) 
{
	Tfref_record *rec = get_current_entry(bfwin);
	const gchar *buttons[] = { GTK_STOCK_NO, GTK_STOCK_YES, NULL };	
	gint retval;
	GtkTreePath *path;
	GtkTreeIter it;
	Tfref_info *inf = fref_refinfo_for_current(bfwin);

	if (fref_ro_for_current(bfwin))
	{
		message_dialog_new(bfwin->main_window, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, 
																 _("Error"), 
																 _("This library is readonly."));
		return;														 
	}	

	
	if (!rec) return;
	switch (rec->etype)
	{
		case FREF_EL_REF:
			message_dialog_new(bfwin->main_window, 
 									 GTK_MESSAGE_ERROR, 
 									 GTK_BUTTONS_CLOSE, 
									 _("Bad element"), 
									 _("Please use delete library instead."));
		break;
		case FREF_EL_GROUP:
			message_dialog_new(bfwin->main_window, 
 									 GTK_MESSAGE_INFO, 
 									 GTK_BUTTONS_OK, 
									 _("Delete group"), 
									 _("Not implemented yet"));
		/*	pstr = g_strdup_printf(_("Do you really want to delete group %s and all its elements?"), FREFNOTE(rec->data)->title);
			ret =	multi_query_dialog(bfwin->main_window, _("Delete group"), pstr,0, 0, btns);
			g_free(pstr);
			if (ret!=0)
			{
			   
			}*/
		break;		
		default:
			gtk_tree_view_get_cursor(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), &path, NULL);
			retval = message_dialog_new_multi(bfwin->main_window,
														 GTK_MESSAGE_QUESTION,
														 buttons,
														 _("Delete group?"),
														 _("Do you really want to delete current entity?"));
			if (path!=NULL && retval !=0)
			{
				gtk_tree_model_get_iter(GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store),&it,path);	
				while (gtk_tree_path_get_depth(path) > 1 && gtk_tree_path_up(path));
				gtk_tree_store_remove(GTK_TREE_STORE(FREFDATA(main_v->frefdata)->store),&it); 
				gtk_tree_view_set_cursor(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), path, NULL,FALSE);
				g_hash_table_remove(inf->elements,FREFELEMENT(rec->data)->name);
				fref_free_record(NULL,rec,bfwin);
				fref_save_ref(bfwin);					
				frefcb_cursor_changed(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), bfwin);	
			}	
		
	}
}


/* --------------------------      END CALLBACKS   ---------------------------------------------------------- */

static void fill_toplevels(Tfref_data * fdata, gboolean empty_first) {
	GList *reflist;
	gint *cnt;

	if (empty_first) {
		gtk_tree_store_clear(fdata->store);
	}
	reflist = g_list_first(main_v->globses.reference_files);
	while (reflist) {
		gchar **tmparray = reflist->data;
		if (count_array(tmparray) == 2) {
			if (access(tmparray[1], R_OK) == 0) {
				GtkTreeIter iter;
				GtkTreeIter iter2;
				DEBUG_MSG("fill_toplevels, adding %s as %s\n",tmparray[1],tmparray[0]);
				gtk_tree_store_append(fdata->store, &iter, NULL);
				if (access(tmparray[1], W_OK) == 0) {
					gtk_tree_store_set(fdata->store, &iter,PIXMAP_COLUMN,fdata->icon_rw,
				               STR_COLUMN, tmparray[0], PTR_COLUMN, 0,
								   FILE_COLUMN, tmparray[1], VISIBLE_COLUMN,FALSE,-1);
				} else {
					gtk_tree_store_set(fdata->store, &iter,PIXMAP_COLUMN,fdata->icon_ro,
				               STR_COLUMN, tmparray[0], PTR_COLUMN, 0,
								   FILE_COLUMN, tmparray[1], VISIBLE_COLUMN,TRUE,-1);
				}				   
				cnt = g_new0(gint, 1);
				*cnt = 0;
				g_hash_table_replace(fdata->refcount, g_strdup(tmparray[0]), cnt);
				gtk_tree_store_append(fdata->store, &iter2, &iter);
			}
		}
		reflist = g_list_next(reflist);
	}
}


void fref_init(void) {
	Tfref_data *fdata;
	
	fref_rand = g_rand_new();
	
	fdata = g_new(Tfref_data, 1);
	main_v->frefdata = fdata;
	fdata->store = gtk_tree_store_new(N_COLUMNS, G_TYPE_BOOLEAN,GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING);
	fdata->refcount = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	fdata->icon_rw = gdk_pixbuf_new_from_inline(-1,pixmap_frlibrary,FALSE,NULL);
	fdata->icon_ro = gdk_pixbuf_new_from_inline(-1,pixmap_frlibrary_ro,FALSE,NULL);
	fdata->icon_hide = gdk_pixbuf_new_from_inline(-1,pixmap_frhide ,FALSE,NULL);
	fdata->icon_dialog2 = gdk_pixbuf_new_from_inline(-1,pixmap_frdialog2 ,FALSE,NULL);
	fdata->icon_group = gdk_pixbuf_new_from_inline(-1,pixmap_frgroup ,FALSE,NULL);
	fdata->icon_note = gdk_pixbuf_new_from_inline(-1,pixmap_frnote ,FALSE,NULL);
	fdata->icon_function = gdk_pixbuf_new_from_inline(-1,pixmap_frfunction ,FALSE,NULL);
	fdata->icon_tag = gdk_pixbuf_new_from_inline(-1,pixmap_frtag ,FALSE,NULL); 
	fdata->icon_var = gdk_pixbuf_new_from_inline(-1,pixmap_frvar ,FALSE,NULL);
	fdata->icon_css = gdk_pixbuf_new_from_inline(-1,pixmap_frcss ,FALSE,NULL);
	fdata->icon_snippet = gdk_pixbuf_new_from_inline(-1,pixmap_frsnippet ,FALSE,NULL);
	fill_toplevels(fdata, FALSE);
	DEBUG_MSG("fref_init, refcount at %p\n", fdata->refcount);
	
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
	GdkColor cc;

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

	cell = gtk_cell_renderer_pixbuf_new();
	gtk_cell_renderer_set_fixed_size(cell,12,12);
	column = gtk_tree_view_column_new_with_attributes("", cell, "pixbuf", PIXMAP_COLUMN, NULL);
	gtk_tree_view_column_set_fixed_width(column,12);
	gtk_tree_view_append_column(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), column);
	
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
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(FREFGUI(bfwin->fref)->infoview),8);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(FREFGUI(bfwin->fref)->infoview),8);
	fref_hand_cursor = gdk_cursor_new (GDK_HAND2);
	fref_regular_cursor = gdk_cursor_new (GDK_XTERM);
 	g_signal_connect (FREFGUI(bfwin->fref)->infoview, "event-after",G_CALLBACK (fref_link_event), bfwin);
	g_signal_connect (FREFGUI(bfwin->fref)->infoview, "motion-notify-event",G_CALLBACK (fref_link_motion_event), NULL);
	g_signal_connect (FREFGUI(bfwin->fref)->infoview, "visibility-notify-event", G_CALLBACK (fref_link_visibility_event), NULL);

	g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->tree), "cursor-changed",
					 G_CALLBACK(frefcb_cursor_changed), bfwin);

	buff = gtk_text_view_get_buffer(GTK_TEXT_VIEW(FREFGUI(bfwin->fref)->infoview));
	
	gtk_widget_modify_font(FREFGUI(bfwin->fref)->infoview,pango_font_description_from_string(main_v->props.bflib_info_font));


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
	gtk_tooltips_set_tip(FREFGUI(bfwin->fref)->argtips,FREFGUI(bfwin->fref)->btn_pref , _("Rescan directories"), "");
	gtk_container_add(GTK_CONTAINER(FREFGUI(bfwin->fref)->btn_pref), new_pixmap(1002));	
	g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->btn_pref), "clicked",
					 G_CALLBACK(frefcb_prefbtn_clicked), bfwin);	
	GTK_WIDGET_SET_FLAGS(FREFGUI(bfwin->fref)->btn_pref, GTK_CAN_DEFAULT);				 

	FREFGUI(bfwin->fref)->btn_edit = gtk_button_new();
	gtk_tooltips_set_tip(FREFGUI(bfwin->fref)->argtips,FREFGUI(bfwin->fref)->btn_edit , _("Edit"), "");
	gtk_container_add(GTK_CONTAINER(FREFGUI(bfwin->fref)->btn_edit), new_pixmap(162));	
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
	g_signal_connect(GTK_OBJECT(wdg), "activate", G_CALLBACK(frefcb_new_group),bfwin);	
	gtk_menu_append(GTK_MENU(menu),wdg);	
	wdg = gtk_menu_item_new_with_label(_("Property definition"));
	g_signal_connect(GTK_OBJECT(wdg), "activate", G_CALLBACK(frefcb_new_re_prop),bfwin);		
	gtk_menu_append(GTK_MENU(menu),wdg);	
	wdg = gtk_menu_item_new_with_label(_("Description definition"));
	g_signal_connect(GTK_OBJECT(wdg), "activate", G_CALLBACK(frefcb_new_re_descr),bfwin);		
	gtk_menu_append(GTK_MENU(menu),wdg);	
	wdg = gtk_menu_item_new_with_label(_("Values definition"));
	g_signal_connect(GTK_OBJECT(wdg), "activate", G_CALLBACK(frefcb_new_re_values),bfwin);			
	gtk_menu_append(GTK_MENU(menu),wdg);	
	
	wdg = gtk_menu_item_new_with_label(_("Element"));
	g_signal_connect(GTK_OBJECT(wdg), "activate", G_CALLBACK(frefcb_new_element),bfwin);
	gtk_menu_append(GTK_MENU(menu),wdg);		
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),menu);	
	item = gtk_menu_item_new_with_label(_("Delete definition"));
	g_signal_connect(GTK_OBJECT(item), "activate", G_CALLBACK(frefcb_del_reusable),bfwin);
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),item);	
	item = gtk_menu_item_new_with_label(_("Delete entity"));
	g_signal_connect(GTK_OBJECT(item), "activate", G_CALLBACK(frefcb_del_entity),bfwin);
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),item);
/*	item = gtk_menu_item_new_with_label(_("Modify entity"));
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),item);	*/
	
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

	gdk_color_parse(main_v->props.bflib_info_bkg,&cc);
	gtk_widget_modify_base(FREFGUI(bfwin->fref)->infoview,FREFGUI(bfwin->fref)->infoview->state,&cc);
	gdk_color_parse(main_v->props.bflib_info_fg,&cc);
	gtk_widget_modify_text(FREFGUI(bfwin->fref)->infoview,FREFGUI(bfwin->fref)->infoview->state,&cc);


	if (!gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(fdata->store), &it, NULL, 0))
		fill_toplevels(fdata, FALSE);
		
	g_signal_connect(G_OBJECT(bfwin->main_window),"key-press-event",G_CALLBACK(fref_keypressed),bfwin);	
		
	return pane;
}

