/* Bluefish HTML Editor
 * fref.c
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <gtk/gtk.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

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
	gpointer data;
	Tbfwin *bfwin;
} Tcallbackdata;


typedef struct {
	GtkWidget *tree;
	GtkTooltips *argtips;
	GtkWidget *infocheck;
	GtkWidget *infoview;
	GtkWidget *infoscroll;
	GtkWidget *menu_show,*menu_pref,*menu_edit;
	GtkWidget *m_descr,*m_ap,*m_note,*m_example,*m_link;
	GtkWidget *btn_show,*btn_pref,*btn_edit;
	GdkColor col_descr,col_ap,col_note,col_example,col_link;
	GtkTextTag *tag_descr,*tag_ap,*tag_note,*tag_example,*tag_link;
	Tbfwin *bfwin;
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
	FREF_EL_PARENTS
};

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
	guchar *type;      	/* this is not for markup, but usable for classes/return/params */
	gboolean required;
	gboolean vlistonly;
	gpointer vlist;   /* here we have pointer not values, because it can be common defined list of values */
	guchar *def;       /* default value */
	gpointer description;
} Tfref_property;
#define FREFPROPERTY(var) ((Tfref_property *)(var))


typedef struct {
	gint etype;								/* type of element */
	guchar *name;   						
	gpointer description; 
	GList *properties;				 /* attributes/params/variables/fields */
	GList *notes;
	guchar *insert;
	guchar *dialog_title;
	guchar *dialog_insert;
	gpointer parent;		/* parent element when this element is subelement */
	GList *children;					/* list of subelements */
} Tfref_element;
#define FREFELEMENT(var) ((Tfref_element *)(var))
    


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
	rec->etype = tp;
	rec->data = data;
	g_hash_table_replace(info->commons,name,rec);
	return rec;
}


Tfref_property *fref_parse_property(xmlNodePtr node,xmlDocPtr doc,Tfref_info *info,gboolean common) {
		Tfref_property  *p = g_new0(Tfref_property,1);  
		guchar *auxs2,*auxs3;  
		xmlNodePtr auxn;
		gpointer auxp;

		if ( common) {
			auxs2 = xmlGetProp(node,"id");
			if (auxs2 == NULL) {
					g_warning("Reusable property has to have id attribute!");
					g_free(p);
					return NULL;
			}		
			p->name = auxs2;
		}
		else {
			p->name = xmlGetProp(node,"name");
		}

		auxs2 = xmlGetProp(node,"kind");
		if ( auxs2 == NULL ) {
			g_warning("Kind of property not determined ");
			g_free(p);
			return NULL;
		}	
		if (xmlStrcmp(auxs2,"link") == 0)
			p->ptype = FREF_EL_LINK;
		else if (xmlStrcmp(auxs2,"example") == 0)	
			p->ptype = FREF_EL_EXAMPLE;		
		else if (xmlStrcmp(auxs2,"dependency") == 0)	
			p->ptype = FREF_EL_DEPEND;		
		else if (xmlStrcmp(auxs2,"attribute") == 0)	
			p->ptype = FREF_EL_ATTR;		
		else if (xmlStrcmp(auxs2,"parameter") == 0)	
			p->ptype = FREF_EL_PARAM;		
		else if (xmlStrcmp(auxs2,"return") == 0)	
			p->ptype = FREF_EL_RETURN;		
		else if (xmlStrcmp(auxs2,"parents") == 0)	
			p->ptype = FREF_EL_PARENTS;		
		else {
			g_warning("Uknown property kind ");
			xmlFree(auxs2);
			g_free(p);
			return NULL;
		}	
		xmlFree(auxs2);
		
		p->type = xmlGetProp(node,"type");
		
		auxs2 = xmlGetProp(node,"required");		
		if ( auxs2 && (xmlStrcmp(auxs2,"1") == 0)) 
			p->required = TRUE;
		else
			p->required = FALSE;	
			
		if ( auxs2) xmlFree(auxs2);	
		
		auxs2 = xmlGetProp(node,"vlistonly");		
		if (auxs2 && (xmlStrcmp(auxs2,"1") == 0))
			p->vlistonly = TRUE;
		else
			p->vlistonly = FALSE;	
			
		if ( auxs2) 	xmlFree(auxs2);
			
		p->def = xmlGetProp(node,"default");
		auxn = node->xmlChildrenNode;		
		while ( auxn ) {
			if ( xmlStrcmp(auxn->name,"values") == 0 ) {
						auxs3 = xmlGetProp(auxn,"ref");
						if ( auxs3 != NULL ) {
							auxp = g_hash_table_lookup(info->commons,auxs3);
							if ( auxp == NULL ) {
								g_warning("List of values referenced (%s) but not found",auxs3);
								xmlFree(auxs3);
								g_free(p);
								return NULL;
						   }
						   p->vlist = FREFRECORD(auxp)->data;
						   xmlFree(auxs3);
			        } else {
			        			auxs3 = xmlNodeListGetString(doc,auxn->xmlChildrenNode,0);
			        			auxs2 = fref_unique_name(xmlGetLineNo(auxn));
						      Tfref_record *rec = fref_insert_into_commons(info, auxs2,FREF_EL_VALUES,auxs3);
						      if (rec) {
						      	p->vlist = rec->data;
						      }	
						      /*g_free(auxs2);*/
			        }
			}        
			else if ( xmlStrcmp(auxn->name,"description") == 0 ) {
						auxs3 = xmlGetProp(auxn,"ref");
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
			        			auxs3 = xmlNodeListGetString(doc,auxn->xmlChildrenNode,0);
						      auxs2 = fref_unique_name(xmlGetLineNo(auxn));
						      Tfref_record *rr = fref_insert_into_commons(info, auxs2,FREF_EL_DESCR,auxs3);
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


Tfref_element *fref_parse_element(xmlNodePtr node,xmlDocPtr doc,Tfref_info *info) {
	Tfref_element *p = g_new0(Tfref_element,1);
	xmlNodePtr auxn;
	guchar *auxs;
	gpointer auxp;

	auxs = xmlGetProp(node,"ref");
	if ( auxs ) {
		auxp = g_hash_table_lookup(info->dictionary,auxs);
		if ( auxp == NULL ) {
			g_warning("Previously defined element %s requested but not found",auxs);
			xmlFree(auxs);
			g_free(p);
			return NULL;
		}  else  {
			xmlFree(auxs);
	 	   return FREFELEMENT(auxp);
		}    
	}
	
	p->name = xmlGetProp(node,"name"); 
	
	if ( p->name == NULL) {
		g_warning("Element has to have a name!");
		g_free(p);
		return NULL;
	}

	auxs = xmlGetProp(node,"kind");
	if ( auxs == NULL ) {
			g_warning("Kind of element not determined ");
			g_free(p);
			return NULL;
	}	
	
	if (xmlStrcmp(auxs,"function") == 0)
			p->etype = FREF_EL_FUNCTION;
		else if (xmlStrcmp(auxs,"tag") == 0)	
			p->etype = FREF_EL_TAG;		
		else if (xmlStrcmp(auxs,"class") == 0)	
			p->etype = FREF_EL_CLASS;		
		else if (xmlStrcmp(auxs,"css") == 0)	
			p->etype = FREF_EL_CSSPROP;		
		else if (xmlStrcmp(auxs,"snippet") == 0)	
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
		if ( xmlStrcmp(auxn->name,"description") == 0) { /* description */
			auxs = xmlGetProp(auxn,"ref");
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
				Tfref_record *rec = fref_insert_into_commons(info,auxs,FREF_EL_DESCR,xmlNodeListGetString(doc,auxn->xmlChildrenNode,0));
				if ( rec ) {
					p->description = rec->data;
				}	
				/* g_free(auxs);*/
			}
		} /* description */
		else if ( xmlStrcmp(auxn->name,"properties") == 0) { /* properties */
				xmlNodePtr nptr = auxn->xmlChildrenNode;
				Tfref_property *apv; 				
				while ( nptr  ) {
				 if (xmlStrcmp(nptr->name,"property")==0) 
				 {
							auxs = xmlGetProp(nptr,"ref");
							if ( auxs ) {
								auxp = g_hash_table_lookup(info->commons,auxs);
								if ( auxp == NULL ) {
									g_warning("Reusable property requested (%s) but not found",auxs);
									xmlFree(auxs);
									g_free(p);
									return NULL;
								}
								apv = FREFRECORD(auxp)->data;
								xmlFree(auxs);
							} else {
												apv = fref_parse_property(nptr,doc,info,FALSE);
												if ( apv ) {
													auxs = fref_unique_name(xmlGetLineNo(nptr));
													fref_insert_into_commons(info,auxs,apv->ptype,apv);
													/* g_free(auxs); */
												}	
							}		
							if ( apv )
								p->properties = g_list_append(p->properties,apv);			
				 }		/* is property */
					nptr = nptr->next;
				}
		} /* properties */
		else if ( xmlStrcmp(auxn->name,"children") == 0) { /* children */
				xmlNodePtr nptr = auxn->xmlChildrenNode;
				Tfref_element *ftpc; 				
				while ( nptr  ) {
				 if (xmlStrcmp(nptr->name,"element")==0) 
				 {
							ftpc = fref_parse_element(nptr,doc,info);
							if ( ftpc ) {
														p->children = g_list_append(p->children,ftpc);
												}	
				 }		/* is element */
					nptr = nptr->next;
				}				
		} /* children */		
		else if ( xmlStrcmp(auxn->name,"note") == 0) { /* note */
			Tfref_note *note = g_new0(Tfref_note,1);
			note->title = xmlGetProp(auxn,"title");
			note->text = xmlNodeListGetString(doc,auxn->xmlChildrenNode,0);
			p->notes = g_list_append(p->notes,note);
		} /* note */
		else if ( xmlStrcmp(auxn->name,"insert") == 0) { /* insert */
				p->insert = xmlNodeListGetString(doc,auxn->xmlChildrenNode,0);
		} /* insert */
		else if ( xmlStrcmp(auxn->name,"dialog") == 0) { /* dialog */
				p->dialog_title = xmlGetProp(auxn,"title");
				p->dialog_insert = xmlNodeListGetString(doc,auxn->xmlChildrenNode,0);
		} /* dialog */		
		auxn = auxn->next;
	}
	return p;
}


static void fref_parse_node(xmlNodePtr node,GtkWidget * tree, GtkTreeStore * store,
   	              						  GtkTreeIter * parent, Tfref_info *info, xmlDocPtr doc) {
   	              						  
	GtkTreeIter auxit;
	GtkTreePath *path;
	xmlNodePtr auxn;
	guchar *auxs,*auxs2;
	gpointer auxp;
	
	   	              						  
	if (!node) return;
	if ( xmlStrcmp(node->name,"group") == 0 ) { /* group */
		Tfref_record *el = g_new0(Tfref_record,1);
		Tfref_note *n = g_new0(Tfref_note,1);
		el->etype = FREF_EL_GROUP;
		el->data = n;
		n->title = xmlGetProp(node,"name");
		auxn = node->xmlChildrenNode;
		while ( auxn ) {
			if ( xmlStrcmp(auxn->name,"description") == 0 ) {
				auxs = xmlGetProp(auxn,"ref");
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
					auxs2 = xmlNodeListGetString(doc,auxn->xmlChildrenNode,0);
					auxs = fref_unique_name(xmlGetLineNo(auxn));
					Tfref_record *rec = fref_insert_into_commons(info,auxs,FREF_EL_DESCR,auxs2);
					if ( rec ) {
						n->text = rec->data;
					}
					g_free(auxs);
				}
				break;			
			}
			auxn = auxn->next;
		}
		gtk_tree_store_append(store, &auxit, parent);
		gtk_tree_store_set(store, &auxit, STR_COLUMN,xmlGetProp(node,"name"), FILE_COLUMN, NULL, PTR_COLUMN, el, -1);
		auxn = node->xmlChildrenNode;
		while (auxn!=NULL ) {
			fref_parse_node(auxn,tree,store,&auxit,info,doc);
			auxn = auxn->next;
		}
	} /* group */   	              						  
	else 	if ( xmlStrcmp(node->name,"note") == 0 ) { /* global note */
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(store),parent);
		if (gtk_tree_path_get_depth(path) == 1) { 
			Tfref_note *n = g_new0(Tfref_note,1);
			Tfref_record *el = g_new0(Tfref_record,1);
			el->etype = FREF_EL_NOTE;
			el->data = n;
			n->title = xmlGetProp(node,"title");
			n->text = xmlNodeListGetString(doc,node->xmlChildrenNode,0);
			gtk_tree_store_append(store, &auxit, parent);
			gtk_tree_store_set(store, &auxit, STR_COLUMN,n->title, FILE_COLUMN, NULL, PTR_COLUMN,el, -1);
		}
	} /* global note */
	else 	if ( xmlStrcmp(node->name,"def") == 0 ) { /* definition */
		auxs = xmlGetProp(node,"id");
		if ( auxs == NULL ) {
			g_warning("Definition has to have an id");
			return;
		}
		auxs2 = xmlGetProp(node,"deftype");		
		if ( auxs2 == NULL ) {
			g_warning("Definition type not defined");
			return;
		}
		if ( xmlStrcmp(auxs2,"property") == 0 ) {			
			Tfref_property *pp = fref_parse_property(node,doc,info,TRUE);
			if ( pp ) {
				fref_insert_into_commons(info,auxs,pp->ptype,pp);
			}	
		}
		else if ( xmlStrcmp(auxs2,"description") == 0 ) {		
			fref_insert_into_commons(info,auxs,FREF_EL_DESCR,xmlNodeListGetString(doc,node->xmlChildrenNode,0));	
		} 
		else if ( xmlStrcmp(auxs2,"values") == 0 ) {		
			fref_insert_into_commons(info,auxs,FREF_EL_VALUES,xmlNodeListGetString(doc,node->xmlChildrenNode,0));	
		} 		
		else {
			g_warning("Uknown definition type");
			xmlFree(auxs2);
			return;
		}
		xmlFree(auxs2);
	} /* definition */	
	else 	if ( xmlStrcmp(node->name,"element") == 0 ) { /* element */
		Tfref_element *el = fref_parse_element(node,doc,info);
		Tfref_record *rec; 
		if ( el ) {
			rec = g_new0(Tfref_record,1);
			rec->etype = el->etype;
			rec->data = el;
			gtk_tree_store_append(store, &auxit, parent);
			gtk_tree_store_set(store, &auxit, STR_COLUMN,el->name, FILE_COLUMN, NULL, PTR_COLUMN,rec, -1);			
		}
	} /* element */	
	else {
		/*g_warning("Uknown tag %s",node->name);*/
	}
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
	if ( xmlStrcmp(cur->name, "ref") != 0 ) {
		g_warning(_("Bad root element in reference file %s\nShould be <ref> and found %s"),filename,cur->name);
		xmlFreeDoc(doc);
		return NULL;
	}
	ret = xmlGetProp(cur,"name");
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
							  GtkTreeIter * parent, Tfref_info *info )
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	xmlChar *tmps;

	if (filename == NULL)
		return;
	info->dictionary =g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
								      (GDestroyNotify) gtk_tree_row_reference_free);
	info->commons = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,g_free);
	xmlLineNumbersDefault(1);
	doc = xmlParseFile(filename);
	if (doc==NULL) {
		g_warning(_("Cannot load reference file %s\n"),filename);
		g_hash_table_destroy(info->dictionary);
		g_hash_table_destroy(info->commons);
		return;
	}	
	cur = xmlDocGetRootElement(doc);	
	if ( xmlStrcmp(cur->name, (const xmlChar *) "ref") != 0 ) {
		g_warning(_("Bad root element in reference file %s\nShould be <ref> and found %s"),filename,cur->name);
		g_hash_table_destroy(info->dictionary);
		g_hash_table_destroy(info->commons);		
		xmlFreeDoc(doc);
		return;
	}
	tmps = xmlGetProp(cur,"description");
	if (tmps!=NULL)
		info->description = tmps;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		fref_parse_node(cur,tree,store,parent,info,doc);
		cur = cur->next;
	}
	xmlFreeDoc(doc);
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
		gtk_tree_model_get_iter(gtk_tree_view_get_model(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree)),
								&iter, path);
		gtk_tree_path_free(path);
		val = g_new0(GValue, 1);
		gtk_tree_model_get_value(gtk_tree_view_get_model(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree)),
								 &iter, 1, val);
		if (G_IS_VALUE(val) && g_value_fits_pointer(val)) {
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
	GList *lst;
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
					if (FREFELEMENT(entry->data)->description) {
						ret =	g_strconcat(ret, _("TAG: "),fref_notnull_str(FREFELEMENT(entry->data)->name),"\n ",
						                            fref_notnull_str(FREFELEMENT(entry->data)->description),NULL);
						tofree = fref_get_prop_descr(FREFELEMENT(entry->data),FREF_EL_PARENTS);
						if ( tofree ) {
							ret =	g_strconcat(ret, _("\nPARENT TAGS: "),tofree,NULL);						                            
							g_free(tofree);
						}	
					}				
			break;
			case FREF_EL_FUNCTION:
					if (FREFELEMENT(entry->data)->description) {
						ret =	g_strconcat(ret, _("FUNCTION: "),fref_notnull_str(FREFELEMENT(entry->data)->name),"\n ",
						                            fref_notnull_str(FREFELEMENT(entry->data)->description),NULL);
						tofree = fref_get_prop_descr(FREFELEMENT(entry->data),FREF_EL_RETURN);                            
						if (tofree) {
							ret =	g_strconcat(ret, _("\nRETURNS: "),tofree,NULL);						                            
						   g_free(tofree);                         
						}
						tofree = fref_get_prop_descr(FREFELEMENT(entry->data),FREF_EL_DEPEND);                            
						if (tofree) {
							ret =	g_strconcat(ret, _("\nDEPENDENCIES: "),tofree,NULL);						                            
						   g_free(tofree);                         
						}						
					}				
			break;			
			case FREF_EL_CLASS:
					if (FREFELEMENT(entry->data)->description) {
						ret =	g_strconcat(ret, _("CLASS: "),fref_notnull_str(FREFELEMENT(entry->data)->name),"\n ",
						                            FREFELEMENT(entry->data)->description,NULL);
					}				
					tofree = fref_get_prop_descr(FREFELEMENT(entry->data),FREF_EL_PARENTS);
					if ( tofree ) {
							ret =	g_strconcat(ret, _("\nPARENT CLASSES: "),tofree,NULL);						                            
							g_free(tofree);
					}	
					tofree = fref_get_prop_descr(FREFELEMENT(entry->data),FREF_EL_DEPEND);                            
					if (tofree) {
							ret =	g_strconcat(ret, _("\nDEPENDENCIES: "),tofree,NULL);						                            
						   g_free(tofree);                         
					}											
			break;			
			case FREF_EL_SNIPPET:
					if (FREFELEMENT(entry->data)->description) {
						ret =	g_strconcat(ret, _("SNIPPET: "),fref_notnull_str(FREFELEMENT(entry->data)->name),"\n ",
						                            FREFELEMENT(entry->data)->description,NULL);
					}				
			break;			
			default: ret="";
		} /* switch */
	break;
	case FR_INFO_ATTRS:	 
		switch ( entry->etype ) {
			case FREF_EL_FUNCTION:
				lst = g_list_first(FREFELEMENT(entry->data)->properties);
				ret = g_strdup(_("\nPARAMS:\n"));
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
				ret = g_strdup(_("\nATTRIBUTES:\n"));
				while ( lst != NULL ) {
					if (FREFPROPERTY(lst->data)->ptype == FREF_EL_ATTR) {
							ret = g_strconcat(ret," - ",fref_notnull_str(FREFPROPERTY(lst->data)->name),": ",
							                            fref_notnull_str(FREFPROPERTY(lst->data)->description),"\n",NULL);
					}                            
					lst = g_list_next(lst);
				}
			break;
			case FREF_EL_CLASS:
				lst = g_list_first(FREFELEMENT(entry->data)->properties);
				ret = g_strdup(_("\n----- FIELDS:\n"));
				while ( lst != NULL ) {
					if (FREFPROPERTY(lst->data)->ptype == FREF_EL_ATTR) {
							ret = g_strconcat(ret," - ",fref_notnull_str(FREFPROPERTY(lst->data)->name),": ",
							                            fref_notnull_str(FREFPROPERTY(lst->data)->description),"\n",NULL);
					}                            
					lst = g_list_next(lst);
				}
				ret = g_strconcat(ret,"----- METHODS:\n",NULL);  
				lst = g_list_first(FREFELEMENT(entry->data)->children);
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
				ret = g_strdup(_("\nPARAMS:\n"));
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
			case FREF_EL_SNIPPET:
						lst = g_list_first(FREFELEMENT(entry->data)->notes);
						ret = g_strdup(_("\nNOTES:\n"));
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
			case FREF_EL_SNIPPET:
						tofree = fref_get_prop_descr(FREFELEMENT(entry->data),FREF_EL_EXAMPLE);
						if ( tofree ) {
									ret = g_strconcat(_("\nEXAMPLES:\n "),tofree,NULL);
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
			case FREF_EL_SNIPPET:
						tofree = fref_get_prop_descr(FREFELEMENT(entry->data),FREF_EL_LINK);
						if ( tofree ) {
									ret = g_strconcat(_("\nSEE ALSO:\n "),tofree,NULL);
									g_free(tofree);
						}			
			break;
			default: ret="";
		} /* switch */		
	break;
} /* switch infotype */

return ret;
}


static void frefcb_row_collapsed(GtkTreeView * treeview, GtkTreeIter * arg1, GtkTreePath * arg2,
								 Tbfwin * bfwin)
{
/*	gint *cnt = NULL;
	gpointer *aux;
	gboolean do_unload = FALSE;
	GtkTreeModel *treemodel = GTK_TREE_MODEL(FREFDATA(main_v->frefdata)->store);
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
				gtk_timeout_add(250, (GtkFunction) fref_idle_cleanup, data);
			}
		} else
			do_unload = FALSE;
	} else
		do_unload = FALSE;*/
}

static gboolean frefcb_event_mouseclick(GtkWidget * widget, GdkEventButton * event, Tbfwin * bfwin)
{
/*	FRInfo *entry;

	if (widget != FREFGUI(bfwin->fref)->tree)
		return FALSE;

	entry = get_current_entry(bfwin);
	if (entry == NULL) {
		if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
			gtk_menu_popup(GTK_MENU(fref_popup_menu(bfwin, NULL)), NULL, NULL, NULL, NULL,
						   event->button, event->time);
			return TRUE;
		} else
			return FALSE;
	}

	if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {	 right mouse click 
		gtk_menu_popup(GTK_MENU(fref_popup_menu(bfwin, entry)), NULL, NULL, NULL, NULL,
					   event->button, event->time);
	} else if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {	 double click  
		Tcallbackdata *cd = g_new(Tcallbackdata, 1);
		cd->data = entry;
		cd->bfwin = bfwin;
		switch (main_v->globses.fref_ldoubleclick_action) {
		case FREF_ACTION_INSERT:
			fref_popup_menu_insert(NULL, cd);
			break;
		case FREF_ACTION_DIALOG:
			fref_popup_menu_dialog(NULL, cd);
			break;
		case FREF_ACTION_INFO:
			fref_popup_menu_info(NULL, cd);
			break;
		default:
			g_print("weird, fref_doubleclick_action=%d\n",
					main_v->globses.fref_ldoubleclick_action);
			main_v->globses.fref_ldoubleclick_action = FREF_ACTION_DIALOG;
			break;
		}
	}*/
	return FALSE;				
	/* we have handled the event, but the treeview freezes if you return TRUE,    so we return FALSE */
}

static void frefcb_row_expanded(GtkTreeView * treeview, GtkTreeIter * arg1, GtkTreePath * arg2,
								gpointer user_data)
{
	GValue *val;
	GtkTreeIter iter;
	gchar *cat;
	gint *cnt = NULL;
	gpointer *aux;
	gboolean do_load = FALSE;
	Tfref_info *info;
	Tfref_record *el;

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
		gtk_tree_model_get_value(GTK_TREE_MODEL(user_data), arg1, 2, val);
		info = g_new0(Tfref_info,1);
		DEBUG_MSG("frefcb_row_expanded, search hash_table at %p\n", dict);
		
		if (G_IS_VALUE(val) && g_value_peek_pointer(val) != NULL) {
			fref_load_from_file((gchar *) g_value_peek_pointer(val), GTK_WIDGET(treeview),
									 GTK_TREE_STORE(user_data), arg1, info);
			el = g_new0(Tfref_record,1);						 
			el->etype = FREF_EL_REF;
			el->data = info;
			gtk_tree_store_set(GTK_TREE_STORE(user_data), arg1, PTR_COLUMN, el, -1);
		}
		if (g_value_peek_pointer(val) != NULL
			&& gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(user_data), &iter, arg1, 0)) {
			gtk_tree_store_remove(GTK_TREE_STORE(user_data), &iter);
		}
		g_value_unset (val);
		g_free (val);
	} 
}

/*
static gboolean frefcb_event_keypress(GtkWidget * widget, GdkEventKey * event, Tbfwin * bfwin)
{
	FRInfo *entry;
	entry = get_current_entry(bfwin);
	if (entry != NULL) {
		if (g_strcasecmp(gdk_keyval_name(event->keyval), "F1") == 0) {
			fref_show_info(bfwin, entry, FALSE, NULL);
			return TRUE;
		}
	}
	return FALSE;
}*/

static void frefcb_cursor_changed(GtkTreeView * treeview, Tbfwin * bfwin)
{
 	Tfref_record *entry;
	gchar *tmpinfo = NULL; 
	GtkTextIter its,ite;
	GtkTextBuffer *buff = gtk_text_view_get_buffer(GTK_TEXT_VIEW(FREFGUI(bfwin->fref)->infoview));
	
	entry = get_current_entry(bfwin);
	if (entry == NULL)
		return;
		
	gtk_text_buffer_get_start_iter(buff,&its);
	gtk_text_buffer_get_end_iter(buff,&ite);
   gtk_text_buffer_delete(buff,&its,&ite);
		
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(FREFGUI(bfwin->fref)->infocheck))) {						
			if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_descr))) {
				tmpinfo = g_strconcat(fref_prepare_info(entry, FR_INFO_DESC),"\n",NULL);
				if ( tmpinfo ) {
					gtk_text_buffer_get_iter_at_mark(buff,&its,gtk_text_buffer_get_insert(buff));
					gtk_text_buffer_insert_with_tags_by_name(buff,&its,tmpinfo,strlen(tmpinfo),"color_descr",NULL);
					g_free(tmpinfo);
				}
			}
			if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_ap))) {
				tmpinfo = g_strconcat(fref_prepare_info(entry, FR_INFO_ATTRS),"\n",NULL);
				if ( tmpinfo ) {
					gtk_text_buffer_get_iter_at_mark(buff,&its,gtk_text_buffer_get_insert(buff));
					gtk_text_buffer_insert_with_tags_by_name(buff,&its,tmpinfo,strlen(tmpinfo),"color_ap",NULL);
					g_free(tmpinfo);				
				}	
			}
			if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_note))) {
				tmpinfo = g_strconcat(fref_prepare_info(entry, FR_INFO_NOTES),"\n",NULL);
				if ( tmpinfo ) {
					gtk_text_buffer_get_iter_at_mark(buff,&its,gtk_text_buffer_get_insert(buff));
					gtk_text_buffer_insert_with_tags_by_name(buff,&its,tmpinfo,strlen(tmpinfo),"color_note",NULL);
					g_free(tmpinfo);
				}	
			}						
			if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_example))) {
				tmpinfo = g_strconcat(fref_prepare_info(entry, FR_INFO_EXAMPLES),"\n",NULL);
				if ( tmpinfo ) {
					gtk_text_buffer_get_iter_at_mark(buff,&its,gtk_text_buffer_get_insert(buff));
					gtk_text_buffer_insert_with_tags_by_name(buff,&its,tmpinfo,strlen(tmpinfo),"color_example",NULL);
					g_free(tmpinfo);
				}	
			}						
			if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(FREFGUI(bfwin->fref)->m_link))) {
				tmpinfo = g_strconcat(fref_prepare_info(entry, FR_INFO_LINKS),"\n",NULL);
				if ( tmpinfo ) {
					gtk_text_buffer_get_iter_at_mark(buff,&its,gtk_text_buffer_get_insert(buff));
					gtk_text_buffer_insert_with_tags_by_name(buff,&its,tmpinfo,strlen(tmpinfo),"color_link",NULL);
					g_free(tmpinfo);
				}	
			}									
	} 
}

static void frefcb_info_dialog(GtkButton * button, Tcallbackdata * cd)
{
/*	FRInfo *entry;
	GtkWidget *dialog;
	gchar *pomstr;
	gint resp;
	Tbfwin *bfwin = cd->bfwin;

	if (cd->data != NULL) {
		gtk_widget_destroy(GTK_WIDGET(cd->data));
		g_free(cd);
	}
	entry = get_current_entry(bfwin);
	if (entry == NULL)
		return;

	dialog = fref_prepare_dialog(bfwin, entry);
	if (dialog) {
		resp = gtk_dialog_run(GTK_DIALOG(dialog));
		if (resp == GTK_RESPONSE_OK) {
			pomstr = fref_prepare_text(entry, dialog);
			gtk_widget_destroy(dialog);
			doc_insert_two_strings(bfwin->current_document, pomstr, NULL);
			g_free(pomstr);
		} else
			gtk_widget_destroy(dialog);
	}*/
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

static void frefcb_search(GtkButton * button, Tbfwin * bfwin)
{
	GtkWidget *dlg, *entry;
	gint result;
	gchar *stf = NULL;
	Tfref_record *el;
	gpointer ret=NULL;
	
	
	el = get_current_entry(bfwin);
	if ( el==NULL || el->etype != FREF_EL_REF ) {
			error_dialog(bfwin->main_window, _("Error"),
			                 _("Perhaps you didn't load a reference, or you did not select a reference to search in."));
			return;			                 
	}		                 
	

/*	gtk_tree_view_get_cursor(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree), &path, &col);
	if (path != NULL) {
		while (gtk_tree_path_get_depth(path) > 1 && gtk_tree_path_up(path));

		gtk_tree_model_get_iter(gtk_tree_view_get_model(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree)),
								&iter, path);
		gtk_tree_path_free(path);
		val = g_new0(GValue, 1);

		gtk_tree_model_get_value(gtk_tree_view_get_model(GTK_TREE_VIEW(FREFGUI(bfwin->fref)->tree)),
								 &iter, 1, val);
		if (G_IS_VALUE(val) && g_value_fits_pointer(val)) {
			dict = (GHashTable *) g_value_peek_pointer(val);
			if (dict != NULL) {
				gpointer ret = NULL;
*/				
				dlg =
					gtk_dialog_new_with_buttons(_("Reference search"), NULL, GTK_DIALOG_MODAL,
												GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_OK,
												GTK_RESPONSE_ACCEPT, NULL);
				gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_ACCEPT);
				entry = gtk_entry_new();
				gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
				gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), entry, TRUE, TRUE, 0);
				gtk_widget_show(entry);
				result = gtk_dialog_run(GTK_DIALOG(dlg));
				if (result == GTK_RESPONSE_ACCEPT) {
					stf = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
					ret = g_hash_table_lookup(FREFINFO(el->data)->dictionary, stf);
					g_free(stf);
					if (!ret)
						error_dialog(bfwin->main_window, _("Reference search"),	 _("Reference not found"));
				}
				gtk_widget_destroy(dlg);

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
				}
}

static void frefcb_insert(GtkButton * button, Tbfwin * bfwin) {
	Tfref_record *el = get_current_entry(bfwin);
	if ( el == NULL || el->etype == FREF_EL_GROUP || el->etype == FREF_EL_REF ) return;
	switch ( el->etype ) {
		case FREF_EL_FUNCTION:
		case FREF_EL_TAG:
		case FREF_EL_CSSPROP:
		case FREF_EL_CLASS:
		case FREF_EL_SNIPPET:
			if (FREFELEMENT(el->data)->insert != NULL)
				doc_insert_two_strings(bfwin->current_document, FREFELEMENT(el->data)->insert, NULL);
		break;		
	}	
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
				gtk_tree_store_set(fdata->store, &iter, STR_COLUMN, tmparray[0], PTR_COLUMN, NULL,
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
}



GtkWidget *fref_gui(Tbfwin * bfwin)
{
	GtkWidget *scroll, *box, *pane, *box2, *btn1, *btn3,*btn2;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	Tfref_data *fdata = FREFDATA(main_v->frefdata);
	GtkTreeIter it;
	GtkWidget *item,*menu;
	GtkTextBuffer *buff;

	bfwin->fref = g_new0(Tfref_gui, 1);

	pane = gtk_vpaned_new();
	box = gtk_vbox_new(FALSE, 1);
	box2 = gtk_hbox_new(FALSE, 1);


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

	g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->tree), "row-collapsed",
					 G_CALLBACK(frefcb_row_collapsed), bfwin);
	g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->tree), "row-expanded",
					 G_CALLBACK(frefcb_row_expanded), fdata->store);
	g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->tree), "button-press-event",
					 G_CALLBACK(frefcb_event_mouseclick), bfwin);
/*	g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->tree), "key-press-event",
					 G_CALLBACK(frefcb_event_keypress), bfwin);*/

	gtk_widget_show(FREFGUI(bfwin->fref)->tree);
	gtk_widget_show(scroll);

	FREFGUI(bfwin->fref)->argtips = main_v->tooltips;
	
	FREFGUI(bfwin->fref)->infoview = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(FREFGUI(bfwin->fref)->infoview),GTK_WRAP_WORD);
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
					 
	{
		/* BUG??: is this ever freed ??? */
		Tcallbackdata *cd = g_new(Tcallbackdata, 1);
		cd->data = NULL;
		cd->bfwin = bfwin;
		btn1 = bf_allbuttons_backend(NULL, FALSE, 161, G_CALLBACK(frefcb_info_dialog), cd);
	}
	btn2 = bf_allbuttons_backend(NULL, FALSE, 108, G_CALLBACK(frefcb_insert), bfwin);	
	btn3 = bf_allbuttons_backend(NULL, FALSE, 109, G_CALLBACK(frefcb_search), bfwin);	
	gtk_tooltips_set_tip(FREFGUI(bfwin->fref)->argtips, btn1, _("Dialog"), "");
	gtk_tooltips_set_tip(FREFGUI(bfwin->fref)->argtips, btn2, _("Insert"), "");	
	gtk_tooltips_set_tip(FREFGUI(bfwin->fref)->argtips, btn3, _("Search"), "");
	

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
	item = gtk_menu_item_new_with_label(_("Set colors"));
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_pref),item);
	g_signal_connect(GTK_OBJECT(item), "activate", G_CALLBACK(fref_setcolors_lcb),bfwin);
	menu = gtk_menu_new();
	item = gtk_radio_menu_item_new_with_label(NULL,_("Left - Insert, Right - Dialog"));
	gtk_menu_append(GTK_MENU(menu),item);	
	item =  gtk_radio_menu_item_new_with_label_from_widget(GTK_RADIO_MENU_ITEM(item),_("Left - Dialog, Right - Insert"));
	gtk_menu_append(GTK_MENU(menu),item);		
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_pref),gtk_separator_menu_item_new());
	item = gtk_menu_item_new_with_label(_("Left doubleclick action"));
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_pref),item);	
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),menu);
	gtk_widget_show_all(FREFGUI(bfwin->fref)->menu_pref);

	FREFGUI(bfwin->fref)->btn_edit = gtk_button_new();
	gtk_tooltips_set_tip(FREFGUI(bfwin->fref)->argtips,FREFGUI(bfwin->fref)->btn_edit , _("Edit"), "");
	gtk_container_add(GTK_CONTAINER(FREFGUI(bfwin->fref)->btn_edit), new_pixmap(113));	
	g_signal_connect(G_OBJECT(FREFGUI(bfwin->fref)->btn_edit), "clicked",
					 G_CALLBACK(frefcb_editbtn_clicked), bfwin);	
	GTK_WIDGET_SET_FLAGS(FREFGUI(bfwin->fref)->btn_edit, GTK_CAN_DEFAULT);				 
	FREFGUI(bfwin->fref)->menu_edit = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),_("Edit"));
	item = gtk_menu_item_new_with_label(_("Add new reference"));
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),item);
	item = gtk_menu_item_new_with_label(_("Delete reference"));
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),item);
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),gtk_separator_menu_item_new());
	item = gtk_menu_item_new_with_label(_("Add element"));
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),item);
	item = gtk_menu_item_new_with_label(_("Delete element"));
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),item);
	item = gtk_menu_item_new_with_label(_("Modify element"));
	gtk_menu_append(GTK_MENU(FREFGUI(bfwin->fref)->menu_edit),item);
	gtk_widget_show_all(FREFGUI(bfwin->fref)->menu_edit);


	gtk_box_pack_start(GTK_BOX(box2), FREFGUI(bfwin->fref)->infocheck, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(box2), FREFGUI(bfwin->fref)->btn_show, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), FREFGUI(bfwin->fref)->btn_pref, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), FREFGUI(bfwin->fref)->btn_edit, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), btn3, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), btn1, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box2), btn2, FALSE, TRUE, 0);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(FREFGUI(bfwin->fref)->infoscroll),
										  FREFGUI(bfwin->fref)->infoview);
	gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), box2, FALSE, TRUE, 0);
	gtk_paned_pack1(GTK_PANED(pane), box, TRUE, FALSE);
	gtk_paned_pack2(GTK_PANED(pane), FREFGUI(bfwin->fref)->infoscroll, TRUE, TRUE);
	gtk_widget_show_all(pane);

	if (!gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(fdata->store), &it, NULL, 0))
		fill_toplevels(fdata, FALSE);
	return pane;
}

