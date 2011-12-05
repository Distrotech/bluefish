/* Bluefish HTML Editor
 * infb_gui.c - plugin for information browser
 *
 * Copyright (C) 2007 Oskar Świda
 * Copyright (C) 2010-2011 Olivier Sessink
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
 
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h> 
#include <string.h>
#include <time.h>

#include "../dialog_utils.h"
#include "infbrowser.h"
#include "infb_gui.h"
#include "infb_text.h"
#include "infb_load.h"
#include "infb_dtd.h"
#include "infb_html.h"
#include "infb_wizard.h"
#include "infb_docbook.h"

#include <libxml/nanohttp.h>
#include <libxml/HTMLtree.h>



/*
*		Mouse motion and link discovery
*/
static gboolean infb_motion_notify_event (GtkWidget  *text_view,  GdkEventMotion *event,gpointer user_data) {
  gint x, y;
  GSList *tags = NULL, *tagp = NULL;
  GtkTextIter iter;
  gboolean hovering = FALSE;
  Tbfwin *bfwin = BFWIN(user_data);
  Tinfbwin *win;
  gpointer auxp; 

	auxp = g_hash_table_lookup(infb_v.windows,bfwin);
	if (!auxp) return FALSE;
	win = (Tinfbwin*)auxp;
	

	
 	gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view),GTK_TEXT_WINDOW_WIDGET,
                                         event->x, event->y, &x, &y);
 	gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW(text_view), &iter, x, y);
 	tags = gtk_text_iter_get_tags (&iter);
   if (gtk_widget_get_visible(win->tip_window))
   	gtk_widget_hide(win->tip_window);
	 for (tagp = tags;  tagp != NULL;  tagp = tagp->next)
    {    	
      GtkTextTag *tag = tagp->data;
		gpointer type,tipv;
		
		tipv = g_object_get_data (G_OBJECT (tag), "tip");
		if (tipv && !gtk_widget_get_visible(win->tip_window)) {
			gtk_label_set_markup(GTK_LABEL(win->tip_label),(gchar*)tipv);
	  		gdk_window_get_pointer (NULL, &x, &y, NULL);		
  			gtk_window_move (GTK_WINDOW(win->tip_window), x + 8, y + 16); 			
			gtk_widget_show_all(win->tip_window);
		} 
		    
      type = g_object_get_data (G_OBJECT (tag), "type");
      if (type == &infb_v.nt_fileref || type == &infb_v.nt_node 
      	|| type == &infb_v.nt_group || type == &infb_v.nt_localref) 
        {
          hovering = TRUE;
          
          break;
        }
    }    
	 if (hovering != win->hovering_over_link)
   	 {
      	win->hovering_over_link = hovering;
     		if (win->hovering_over_link)
        			set_link_cursor(GTK_TEXT_VIEW(text_view));
     		else
       		 	set_normal_cursor(GTK_TEXT_VIEW(text_view));
    }
   
  	if (tags) g_slist_free (tags);
  	gdk_window_get_pointer (gtk_text_view_get_window(GTK_TEXT_VIEW(text_view), GTK_TEXT_WINDOW_WIDGET), NULL, NULL, NULL);
  	return FALSE;
}


/*
*			Mouse release: link activation
*/
gboolean  infb_button_release_event(GtkWidget  *widget,GdkEventButton *event, gpointer  user_data) {
   gint x,y;
	GtkTextBuffer *buffer;
	GtkTextIter start,end,iter;
	GSList *tags = NULL, *tagp = NULL;
	GtkTextTag *tag;
	gpointer aux;
	xmlDocPtr doc;
	xmlNodePtr auxnode;	   
	xmlChar *text;
	Tbfwin *bfwin = BFWIN(user_data);
	Tinfbwin *win = (Tinfbwin*)g_hash_table_lookup(infb_v.windows,bfwin);
   
   if (event->button == 1) {
   	if (win && gtk_widget_get_visible(win->tip_window))
   		gtk_widget_hide(win->tip_window);
   	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
   	gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
  		if (gtk_text_iter_get_offset (&start) != gtk_text_iter_get_offset (&end))  return FALSE;
	   gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (widget),GTK_TEXT_WINDOW_WIDGET,event->x, event->y, &x, &y);
	   gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (widget), &iter, x, y);
	   tags = gtk_text_iter_get_tags (&iter);
  		for (tagp = tags;  tagp != NULL;  tagp = tagp->next)
    	{
      	tag = tagp->data;
      	aux = g_object_get_data(G_OBJECT(tag),"type");
      	if ( aux && aux == &infb_v.nt_fileref) { /* fileref */
      		aux = g_object_get_data(G_OBJECT(tag),"loaded");
      		if ( aux ) {
      			infb_v.currentDoc = aux;
					infb_fill_doc(bfwin,NULL);      				
      		}
      		else {      			
      			aux = g_object_get_data(G_OBJECT(tag),"file");
      			if ( aux ) {
      				infb_insert_message(GTK_TEXT_VIEW(widget),BAD_CAST _("Loading..."));
	      			if (g_str_has_prefix ((gchar*)aux,"http://")) {
							gchar *pstr = g_strdup_printf("%s/bfish_%ld",g_get_tmp_dir(),(long int)time(NULL));
							gchar *info;
							if (xmlNanoHTTPFetch((const char *)aux,pstr,&info)==-1) {							
								g_free(pstr);
								infb_insert_error(GTK_TEXT_VIEW(widget),BAD_CAST _("Cannot load file from network"));
	      					/*message_dialog_new(bfwin->main_window,
      								GTK_MESSAGE_INFO,GTK_BUTTONS_CLOSE,_("Cannot load file"),(gchar*)aux);*/
								break;
							}
							if (info) g_free(info);
							doc = xmlReadFile(pstr,NULL,
							       XML_PARSE_RECOVER | XML_PARSE_NOENT | XML_PARSE_NOBLANKS | 
							       XML_PARSE_XINCLUDE | XML_PARSE_NOWARNING | XML_PARSE_NOERROR);
							if (doc) doc->URL = xmlStrdup(BAD_CAST aux);
	      				g_free(pstr);
						}      				
      				else {
	      				if (!g_file_test((gchar*)aux,G_FILE_TEST_EXISTS) || !g_file_test((gchar*)aux,G_FILE_TEST_IS_REGULAR)) {
	      					infb_insert_error(GTK_TEXT_VIEW(widget),BAD_CAST _("Cannot find file"));	
   	   					/*message_dialog_new(bfwin->main_window,
      									GTK_MESSAGE_INFO,GTK_BUTTONS_CLOSE,_("Cannot find file"),(gchar*)aux);*/
      						break;			
      					}     				
      					doc = xmlReadFile((gchar*) aux,NULL,
      											XML_PARSE_RECOVER | XML_PARSE_NOENT | XML_PARSE_NOBLANKS | 
      											XML_PARSE_XINCLUDE | XML_PARSE_NOWARNING | XML_PARSE_NOERROR);
      				}	
      				if ( doc ) {
      					g_object_set_data (G_OBJECT (tag), "loaded", doc);
      					/* check for conversion */
      					auxnode = xmlDocGetRootElement(doc);
      					if (xmlStrcmp(auxnode->name,BAD_CAST "ref")==0) {
      						text = xmlGetProp(auxnode,BAD_CAST "type");
      						if (text) {
      							if (xmlStrcmp(text,BAD_CAST "dtd")==0) {
      								xmlFree(text);
      								text = xmlGetProp(auxnode,BAD_CAST "uri");
      								if (text) {
      									/*const gchar *buttons[] = { GTK_STOCK_NO, GTK_STOCK_YES, NULL };*/
      									/*gint res;      									 
      									f (g_str_has_prefix ((gchar*)text,"http://")) {*/ 
      										/*res = message_dialog_new_multi(bfwin->main_window,GTK_MESSAGE_QUESTION,
      												buttons,_("Remote DTD file"),
      												_("It seems this is remote DTD.\nIt can take some time to load.\nContinue?"));
      										if ( res == 0 ) {
      											g_object_set_data (G_OBJECT (tag), "loaded", NULL);
      											if (tags) g_slist_free (tags);      												
      										 	return FALSE;
      										} 	*/		
      									/*	infb_convert_dtd(doc);
      									} 
											else*/				
      									infb_convert_dtd(doc);
      									xmlFree(text);
      								}	
      							}
      							else if (xmlStrcmp(text,BAD_CAST "http")==0) {
      								xmlFree(text);
      								text = xmlGetProp(auxnode,BAD_CAST "uri");
      								if (text) {
												gchar *pstr = g_strdup_printf("%s/bfish_%ld",g_get_tmp_dir(),(long int)time(NULL));
												gchar *info;
												if (xmlNanoHTTPFetch((const char *)text,pstr,&info)==-1) {							
													g_free(pstr);
													infb_insert_error(GTK_TEXT_VIEW(widget),BAD_CAST _("Cannot load file from network"));
	      										/*message_dialog_new(bfwin->main_window,
      																	GTK_MESSAGE_INFO,GTK_BUTTONS_CLOSE,_("Cannot load file from network"),(gchar*)text);*/
													break;
												}
												if (info) g_free(info);												
												doc = xmlReadFile(pstr,NULL,
														XML_PARSE_RECOVER | XML_PARSE_NOENT | XML_PARSE_NOBLANKS
														| XML_PARSE_XINCLUDE | XML_PARSE_NOWARNING | XML_PARSE_NOERROR);	
												if (doc) doc->URL = xmlStrdup(BAD_CAST text);
	      									g_free(pstr);
      								}
      							} 
      							else xmlFree(text);
      						}
      					}
      					if (xmlStrcmp(auxnode->name,BAD_CAST "html")==0) {
      						/* bad trick - but HTML files can be non well-formed */
      						xmlErrorPtr err = xmlGetLastError();
      						if (err) {
      								/* try to reload */
      								xmlFreeDoc(doc);
      								doc = htmlParseFile((gchar*)aux,NULL);
      								if (!doc) break;
      						}
      					}
			      		if (infb_v.currentDoc!=NULL && infb_v.currentDoc!=infb_v.homeDoc)
		      				xmlFreeDoc(infb_v.currentDoc);
		      			infb_v.currentDoc = doc;
		      			infb_fill_doc(bfwin,NULL);      				
      				} /* if doc */	 
      								
      			}
      		}
      		break;	
      	} /* fileref */
      	else if ( aux == &infb_v.nt_group ) { /* group */
				aux = g_object_get_data(G_OBJECT(tag),"node");
				if ( aux ) {
					auxnode = (xmlNodePtr)aux;
					text = xmlGetProp(auxnode,BAD_CAST "expanded");
					if ( !text ) {
						xmlSetProp(auxnode,BAD_CAST "expanded",BAD_CAST "0");
						text = xmlGetProp(auxnode,BAD_CAST "expanded");
					}
					if ( xmlStrcmp(text,BAD_CAST "1") == 0 )
						xmlSetProp(auxnode,BAD_CAST "expanded",BAD_CAST "0");
					else
						xmlSetProp(auxnode,BAD_CAST "expanded",BAD_CAST "1");
					infb_fill_doc( bfwin,NULL );
					xmlFree(text);
				}
      	} /* group */
      	else if ( aux == &infb_v.nt_node ) { /* node */
				aux = g_object_get_data(G_OBJECT(tag),"node");
				if ( aux ) {
					infb_fill_doc( bfwin, (xmlNodePtr)aux );
				}
			} /* node */
      	else if ( aux == &infb_v.nt_localref ) { /* localref */
				aux = g_object_get_data(G_OBJECT(tag),"node");
				if ( aux ) {
					GtkTextMark *mark = gtk_text_buffer_get_mark(buffer,(gchar*)aux);
					if (mark) { 
						gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(widget),mark,0.0,TRUE,0.0,0.0);
					}
				}
			} /* localref */
			
	   }
		if (tags) g_slist_free (tags);		
   }
	return FALSE;
}  

static void infb_up_clicked(GtkButton *button, gpointer data) {
	if ( !infb_v.currentDoc || !data) return;
	if (infb_v.currentNode && infb_v.currentNode->parent && 
		 (void*)infb_v.currentNode->parent!=(void*)infb_v.currentNode->doc )
	{
		infb_fill_doc(BFWIN(data),infb_v.currentNode->parent);
	}	
	else {	
		infb_fill_doc(BFWIN(data),NULL);
	}	
}

static void infb_idx_clicked(GtkButton *button, gpointer data) {
	if ( !data ) return;
	if (infb_v.currentDoc!=NULL) {
		infb_fill_doc(BFWIN(data),NULL);
	}	
}


static void infb_midx_clicked(GtkButton *button, gpointer data) {
	if ( !data ) return;
	if (infb_v.currentDoc!=NULL && infb_v.currentDoc!=infb_v.homeDoc)
		xmlFreeDoc(infb_v.currentDoc);
	infb_v.currentDoc = infb_v.homeDoc;
	infb_fill_doc(BFWIN(data),NULL);
}

static void infb_save_clicked(GtkButton *button, gpointer data) {
	gchar *pstr,*tofree;
	gchar *userdir = g_strconcat(g_get_home_dir(), "/."PACKAGE"/", NULL);
	FILE *f;
	xmlBufferPtr buff;
	xmlChar *text=NULL;
	xmlNodePtr fnode=NULL; 
	
	if ( !data ) return;
	if (infb_v.currentNode!=NULL) {
		if (infb_v.currentType == INFB_DOCTYPE_DOCBOOK) {
			pstr = (gchar*)infb_db_get_title(infb_v.currentDoc,FALSE,NULL);
			tofree = pstr;
			pstr = g_strconcat(pstr,":",infb_db_get_title(infb_v.currentDoc,FALSE,infb_v.currentNode),NULL);
			g_free(tofree);
			text = BAD_CAST pstr;
		
 		   if (xmlStrcmp(infb_v.currentNode->name,BAD_CAST "book")==0) {
		  		fnode = xmlCopyNode(infb_v.currentNode,1);
		   }
		   else 
		   { 
				fnode = xmlNewDocNode(infb_v.currentDoc,NULL,BAD_CAST "book",NULL);
				xmlAddChild(fnode,xmlCopyNode(infb_v.currentNode,1));
		   } 	
		}
		else if (infb_v.currentType == INFB_DOCTYPE_HTML) {
			pstr = (gchar*)infb_html_get_title(infb_v.currentDoc);
			if (!pstr) 
				pstr = g_path_get_basename((gchar*)infb_v.currentDoc->URL);
			text = BAD_CAST pstr;
			fnode = infb_v.currentNode;
		}
		else {
			if ( xmlStrcmp(infb_v.currentNode->name,BAD_CAST "element")==0 ||
				  xmlStrcmp(infb_v.currentNode->name,BAD_CAST "ref")==0 )
				text = xmlGetProp(infb_v.currentNode,BAD_CAST "name");
			else 	if ( xmlStrcmp(infb_v.currentNode->name,BAD_CAST "note")==0 ||
						  xmlStrcmp(infb_v.currentNode->name,BAD_CAST "search_result")==0	  )
				text = xmlGetProp(infb_v.currentNode,BAD_CAST "title");
			else 
				text = xmlStrdup(BAD_CAST "unknown");
			fnode = infb_v.currentNode;	
		}		
		pstr = g_strdup_printf("%s/bfrag_%s_%ld",userdir,
										(gchar*)text,(long int)time(NULL));								
		f = fopen(pstr,"w");
		if (f) {
			buff = xmlBufferCreate();
			if (infb_v.currentType == INFB_DOCTYPE_HTML) {
				htmlNodeDump(buff,infb_v.currentDoc,fnode);
				htmlNodeDumpFile		(stdout,infb_v.currentDoc,fnode);
			}	
			else
				xmlNodeDump(buff,infb_v.currentDoc,fnode,1,1);
			xmlBufferDump(f,buff);
			xmlBufferFree(buff);
			fclose(f);
			infb_load_fragments((Tinfbwin*)data);
			if (infb_v.currentNode != fnode)
				xmlFreeNode(fnode);
			message_dialog_new(((Tinfbwin*)data)->bfwin->main_window,GTK_MESSAGE_INFO,GTK_BUTTONS_CLOSE,_("Fragment saved"),(gchar*)text);
			g_free(pstr);
			xmlFree(text);
		}	else {
			message_dialog_new(((Tinfbwin*)data)->bfwin->main_window,GTK_MESSAGE_INFO,GTK_BUTTONS_CLOSE,_("Cannot open file"),(gchar*)pstr);
			g_free(pstr);	
			xmlFree(text);				
		}
	}
	g_free(userdir);
}



/*
static void infb_add_clicked(GtkButton *button, gpointer data) {
	Tbfwin *bfwin;
	if ( !data ) return;
	bfwin = BFWIN(data);
	infb_add_item(bfwin);
}

static void infb_del_clicked(GtkButton *button, gpointer data) {
	Tbfwin *bfwin;
	if ( !data ) return;
	bfwin = BFWIN(data);
	infb_del_item(bfwin);
}
*/

gboolean infb_search_keypress (GtkWidget *widget,GdkEventKey *event,Tbfwin *bfwin) {
	gchar *txt,*str;
	xmlXPathObjectPtr result;	
	gint i;
	xmlNodePtr node=NULL,node2=NULL,node3,node4;
	gboolean found = FALSE;
	Tinfbwin *win = (Tinfbwin*)g_hash_table_lookup(infb_v.windows,bfwin);
	

	if ( event->keyval != GDK_KEY_Return )	return FALSE;

	if (infb_v.currentDoc == NULL ) return FALSE; 
	txt = (gchar*)gtk_entry_get_text(GTK_ENTRY(widget));
	if ( txt && strcmp(txt,"")!=0) {
	
	

		switch (infb_v.currentType) {
			case INFB_DOCTYPE_DOCBOOK:
				node = xmlNewDocNode(infb_v.currentDoc,NULL,BAD_CAST "appendix",NULL);
				node3 = xmlNewDocNode(infb_v.currentDoc,NULL,BAD_CAST "title",NULL);
				str = g_strconcat("Search: ",txt,NULL);
				node4 = xmlNewText(BAD_CAST str);
				xmlAddChild(node3,node4);
				xmlAddChild(node,node3);			
				str = g_strconcat("/descendant::title[contains(child::text(),\"",txt,"\")]",NULL);
				result = getnodeset(infb_v.currentDoc,BAD_CAST str,NULL);
				g_free(str);
				if (result) {
					found = TRUE;
					for(i=0;i<result->nodesetval->nodeNr;i++) {						
						node2 = xmlDocCopyNode(result->nodesetval->nodeTab[i]->parent,infb_v.currentDoc,1);
						xmlAddChild(node,node2);
					}
				}			
			break;
			case INFB_DOCTYPE_HTML:
			if (win) {
				GdkRectangle rect;
				gint line;
				GtkTextIter iter,mstart,mend;
				if (win->search_tag) {
					gtk_text_buffer_get_bounds(gtk_text_view_get_buffer(GTK_TEXT_VIEW(win->view)),&mstart,&mend);
					gtk_text_buffer_remove_tag_by_name(gtk_text_view_get_buffer(GTK_TEXT_VIEW(win->view)),"search_tag",&mstart,&mend);
				}							
				gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(win->view), &rect);
				gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(win->view), &iter, rect.y, &line);
				gtk_text_iter_forward_line(&iter);
				if ( gtk_text_iter_forward_search(&iter,txt,GTK_TEXT_SEARCH_TEXT_ONLY,&mstart,&mend,NULL))
				{
					gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(win->view),&mstart,0.0,TRUE,0.0,0.0);
					if (!win->search_tag)
 						win->search_tag = gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(win->view)),
										"search_tag",INFB_STYLE_SEARCH,NULL);						
					gtk_text_buffer_apply_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(win->view)),win->search_tag,&mstart,&mend);
					return FALSE;
				}
				else {
					message_dialog_new(bfwin->main_window,GTK_MESSAGE_INFO,GTK_BUTTONS_CLOSE,_("Nothing found"),txt);
					return FALSE;
				}	
			}
			break;
			default: /* fref2 */
				node = xmlNewDocNode(infb_v.currentDoc,NULL,BAD_CAST "search_result",NULL);
				str = g_strconcat("Search: ",txt,NULL);
				xmlNewProp(node,BAD_CAST "title",BAD_CAST str);
				g_free(str);			
				str = g_strconcat("/descendant::element[contains(@name,\"",txt,"\")]",NULL);
				result = getnodeset(infb_v.currentDoc,BAD_CAST str,NULL);
				g_free(str);
				if (result) {
					found = TRUE;
					for(i=0;i<result->nodesetval->nodeNr;i++) {						
						node2 = xmlDocCopyNode(result->nodesetval->nodeTab[i],infb_v.currentDoc,1);
						xmlAddChild(node,node2);
					}
				}
				str = g_strconcat("/descendant::note[contains(@title,\"",txt,"\") and local-name(..)!=\"element\"]",NULL);
				result = getnodeset(infb_v.currentDoc,BAD_CAST str,NULL);
				g_free(str);
				if (result) {
					found = TRUE;
					for(i=0;i<result->nodesetval->nodeNr;i++) {
						node2 = xmlDocCopyNode(result->nodesetval->nodeTab[i],infb_v.currentDoc,1);
						xmlAddChild(node,node2);
					}					
				}	
			break;
		} /* switch */
		if ( found && node && node2) {		
			node2 = xmlDocGetRootElement(infb_v.currentDoc);
			xmlAddChild(node2,node);
			infb_fill_doc(bfwin,node);
		}	
		else {
			message_dialog_new(bfwin->main_window,GTK_MESSAGE_INFO,GTK_BUTTONS_CLOSE,_("Nothing found"),txt);
			if (node) xmlFreeNode(node);
		}	 															
	}
	return FALSE;
} 

static gint infb_tip_paint(GtkWidget *tip)
{
#if GTK_CHECK_VERSION(3, 0, 0)
	cairo_t *arg2;
#else
	GdkWindow *arg2;
#endif /* gtk3 */
	GtkStyle *style;

	if (!tip) return FALSE;
	if (!gtk_widget_get_visible(tip)) return FALSE;

	style = gtk_widget_get_style(tip);
#if GTK_CHECK_VERSION(3, 0, 0)
	arg2 = gdk_cairo_create(gtk_widget_get_window(tip));
	gtk_paint_flat_box (style, arg2,
                      GTK_STATE_NORMAL, GTK_SHADOW_ETCHED_IN,
                      tip, "",
                      0, 0, -1, -1);  
	gtk_paint_shadow (style, arg2,
                      GTK_STATE_NORMAL, GTK_SHADOW_ETCHED_IN,
                      tip, "",
                      0, 0, -1, -1);                        

#else
	arg2= gtk_widget_get_window(tip);
	gtk_paint_flat_box (style, arg2,
                      GTK_STATE_NORMAL, GTK_SHADOW_ETCHED_IN,
                      NULL, tip, "",
                      0, 0, -1, -1);  
	gtk_paint_shadow (style, arg2,
                      GTK_STATE_NORMAL, GTK_SHADOW_ETCHED_IN,
                      NULL, tip, "",
                      0, 0, -1, -1);                        

#endif /* gtk3 */

	return FALSE;                      
}



void infb_sidepanel_initgui(Tbfwin *bfwin) {
	Tinfbwin *win;
	GtkWidget *scrolwin, *hbox, *vbox ;

	win = g_new0(Tinfbwin,1);
	win->bfwin = bfwin;
	g_hash_table_insert(infb_v.windows,bfwin,win);
	win->hovering_over_link = FALSE;		

    vbox = gtk_vbox_new(FALSE, 1);		

	hbox = gtk_toolbar_new();
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(hbox),GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(hbox),GTK_TOOLBAR_ICONS);
	
	win->btn_home = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_HOME,GTK_ICON_SIZE_MENU),"");
	g_signal_connect(G_OBJECT(win->btn_home),"clicked",G_CALLBACK(infb_midx_clicked),bfwin);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(win->btn_home),_("Documentation index"));
	gtk_toolbar_insert(GTK_TOOLBAR(hbox),win->btn_home,0);

	win->btn_idx = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_INDEX,GTK_ICON_SIZE_MENU),"");
	g_signal_connect(G_OBJECT(win->btn_idx),"clicked",G_CALLBACK(infb_idx_clicked),bfwin);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(win->btn_idx),_("Document index"));
	gtk_toolbar_insert(GTK_TOOLBAR(hbox),win->btn_idx,1);

	win->btn_up = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_GO_UP,GTK_ICON_SIZE_MENU),"");
	g_signal_connect(G_OBJECT(win->btn_up),"clicked",G_CALLBACK(infb_up_clicked),bfwin);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(win->btn_up),_("Upper level"));
	gtk_toolbar_insert(GTK_TOOLBAR(hbox),win->btn_up,2);

	
	win->saved = gtk_menu_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_FLOPPY,GTK_ICON_SIZE_MENU),"");
	g_signal_connect(G_OBJECT(win->saved),"clicked",G_CALLBACK(infb_save_clicked),win);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(win->saved),_("Save current view"));
	gtk_menu_tool_button_set_arrow_tooltip_text(GTK_MENU_TOOL_BUTTON(win->saved),_("Go to selected fragment"));
	gtk_toolbar_insert(GTK_TOOLBAR(hbox),win->saved,3);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

	win->view = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(win->view),GTK_WRAP_WORD);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(win->view),FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(win->view),FALSE);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(win->view),8);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(win->view),8);
	g_signal_connect (win->view, "motion-notify-event",G_CALLBACK (infb_motion_notify_event), bfwin);
	g_signal_connect (win->view, "button-release-event",G_CALLBACK (infb_button_release_event), bfwin);
	
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolwin), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scrolwin), win->view);	
	gtk_box_pack_start(GTK_BOX(vbox), scrolwin, TRUE, TRUE, 4);

    hbox = gtk_hbox_new(FALSE, 1);
	win->sentry = gtk_entry_new();	
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Find")), FALSE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(hbox), win->sentry, TRUE, TRUE, 2);
	g_signal_connect(win->sentry, "key-press-event",G_CALLBACK(infb_search_keypress), bfwin);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
	gtk_notebook_insert_page_menu(GTK_NOTEBOOK(bfwin->leftpanel_notebook),vbox,
									gtk_image_new_from_stock(GTK_STOCK_INFO,GTK_ICON_SIZE_LARGE_TOOLBAR),
									gtk_label_new(_("Info Browser")),-1);
/* ---- tip window ----*/
	win->tip_window = gtk_window_new (GTK_WINDOW_POPUP);
   gtk_widget_set_app_paintable (win->tip_window, TRUE);
   gtk_window_set_resizable (GTK_WINDOW(win->tip_window), FALSE);
   gtk_container_set_border_width (GTK_CONTAINER (win->tip_window), 4); 
   g_signal_connect_swapped(GTK_WINDOW(win->tip_window),"expose-event",G_CALLBACK(infb_tip_paint),win->tip_window);
   win->tip_label = gtk_label_new (NULL);  
   gtk_label_set_markup(GTK_LABEL(win->tip_label),"xx");
   gtk_misc_set_alignment (GTK_MISC (win->tip_label), 0.5, 0.5);
   gtk_container_add (GTK_CONTAINER (win->tip_window), GTK_WIDGET (win->tip_label));  
   gtk_widget_hide(win->tip_window);   
   g_object_set_data(G_OBJECT(win->view),"tip",win->tip_window);
       

	infb_load();
	infb_load_fragments(win);
	infb_v.currentDoc = infb_v.homeDoc;
	infb_fill_doc(bfwin,NULL);
	
			
}

void infb_sidepanel_destroygui(Tbfwin *bfwin) {

}
