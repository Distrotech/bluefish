/* Bluefish HTML Editor
 * infb_wizard.c - plugin for information browser
 *
 * Copyright (C) 2007 Oskar Świda
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

#include <string.h>
#include <stdio.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlwriter.h>
#include <glib/gstdio.h>
#include "infb_wizard.h"
#include "infbrowser.h"
#include "infb_load.h"
#include "infb_text.h"

#define ENTRY_TYPE_NONE			0
#define ENTRY_TYPE_DTD_LOCAL	1
#define ENTRY_TYPE_DTD_REMOTE	2

#define LAST_PAGE 3;

static GtkWidget *assistant=NULL;


typedef struct {
	gint type;
	gchar *name;
	gchar *desc;
	gchar *uri;
	Tbfwin *win;
} TentryData;


static void infbw_save_entry(TentryData *data) {
	gchar *fname,*pstr; 
	xmlDocPtr doc;
	xmlNodePtr node;
	FILE *ff;
	
	if (!data) return;
	pstr = g_strdup(data->name);
	fname = g_strconcat(g_get_home_dir(), "/."PACKAGE"/bflib_",
						g_strcanon(pstr,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",'_'), 
						".xml",NULL);
	
	doc = xmlNewDoc(BAD_CAST "1.0");
	node = xmlNewNode(NULL, BAD_CAST "ref");
	xmlDocSetRootElement(doc, node);
	xmlNewProp(node,BAD_CAST "name",BAD_CAST data->name);
	if (data->desc)
		xmlNewProp(node,BAD_CAST "description",BAD_CAST data->desc);
	xmlNewProp(node,BAD_CAST "type",BAD_CAST "dtd");
	xmlNewProp(node,BAD_CAST "uri",BAD_CAST data->uri);
	ff = fopen(fname,"w");
	if (ff) {
		xmlDocDump(ff,doc);
		fclose(ff);
		infb_reload_home(data->win);
	}	
	g_free(fname);
	g_free(pstr);	
	
}


static void infbw_type_set(GtkToggleButton *btn, gpointer data) {
	TentryData *d = (TentryData*)data;
	gpointer ptr = g_object_get_data(G_OBJECT(btn),"type");
	if (ptr) {
		if (strcmp((gchar*)ptr,"dtd_local")==0) 
			d->type = ENTRY_TYPE_DTD_LOCAL;
		else if (strcmp((gchar*)ptr,"dtd_remote")==0)
			d->type = ENTRY_TYPE_DTD_REMOTE;
		else 
			d->type = ENTRY_TYPE_NONE;		
	}
}


static void create_page_type(GtkWidget *dlg, TentryData *edata) {
	GtkWidget *box, *btn;
  	GSList *btn_group = NULL;

  	box = gtk_vbox_new (FALSE,0 );
  	btn = gtk_radio_button_new_with_mnemonic (NULL, _("Local DTD file"));
  	g_object_set_data(G_OBJECT(btn),"type","dtd_local");  	
  	gtk_box_pack_start (GTK_BOX (box), btn, FALSE, FALSE, 2);
   gtk_radio_button_set_group (GTK_RADIO_BUTTON (btn), btn_group);
  	btn_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (btn));
  	g_signal_connect(G_OBJECT(btn),"toggled",G_CALLBACK(infbw_type_set),edata);  
  	edata->type = ENTRY_TYPE_DTD_LOCAL;

	btn = gtk_radio_button_new_with_mnemonic (NULL, _("Remote DTD file"));	
	g_object_set_data(G_OBJECT(btn),"type","dtd_remote");
	gtk_box_pack_start (GTK_BOX (box), btn, FALSE, FALSE, 2);
  	gtk_radio_button_set_group (GTK_RADIO_BUTTON (btn), btn_group);
  	btn_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (btn));
  	g_signal_connect(G_OBJECT(btn),"toggled",G_CALLBACK(infbw_type_set),edata);

	g_object_set_data(G_OBJECT(box),"edata",edata);
  	gtk_widget_show_all (box);
  	gtk_assistant_append_page (GTK_ASSISTANT (dlg), box);
  	gtk_assistant_set_page_title (GTK_ASSISTANT (dlg), box, _("Entry type"));
  	gtk_assistant_set_page_type (GTK_ASSISTANT (dlg), box, GTK_ASSISTANT_PAGE_INTRO);
  	gtk_assistant_set_page_complete (GTK_ASSISTANT(dlg), box, TRUE);

}

void  infbw_file_chosen (GtkFileChooser *chooser,gpointer user_data) {
  GtkAssistant *assistant = GTK_ASSISTANT (user_data);
  GtkWidget *current_page;
  gint page_number;
  gchar *fname;
  gpointer ptr;
  
  page_number = gtk_assistant_get_current_page (assistant);
  current_page = gtk_assistant_get_nth_page (assistant, page_number);
  fname = gtk_file_chooser_get_filename(chooser);
  if ( fname && *fname ) {
  		gtk_assistant_set_page_complete (assistant, current_page, TRUE);
 	   ptr = g_object_get_data(G_OBJECT(current_page),"edata");
   	if (ptr) {
    		((TentryData*)ptr)->uri = g_strdup(fname);
    	}
    	g_free(fname);
  } 
  else {
  		gtk_assistant_set_page_complete (assistant, current_page, FALSE);
 	   ptr = g_object_get_data(G_OBJECT(current_page),"edata");
   	if (ptr) {
    		if ( ((TentryData*)ptr)->uri  ) {
    			g_free(((TentryData*)ptr)->uri);
    			((TentryData*)ptr)->uri = NULL;
    		}	
    	}  		
  } 
}

static void create_page_dtd_file(GtkWidget *dlg, TentryData *edata) {
	GtkWidget *box, *label, *btn,*box2;

  	box = gtk_hbox_new (FALSE,0 );
	box2 = gtk_vbox_new (FALSE, 15);
  	gtk_box_pack_start (GTK_BOX (box2), box, FALSE, FALSE, 5);
  	
  	label = gtk_label_new (_("Choose file"));
  	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 2);

	btn = gtk_file_chooser_button_new(_("Local DTD file"),GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_box_pack_start (GTK_BOX (box), btn,TRUE, TRUE, 5);
	g_signal_connect (G_OBJECT (btn), "update-preview",G_CALLBACK (infbw_file_chosen), assistant);

  	gtk_widget_show_all (box2);
  	g_object_set_data(G_OBJECT(box2),"edata",edata);
  	
  	gtk_assistant_append_page (GTK_ASSISTANT (dlg), box2);
  	gtk_assistant_set_page_title (GTK_ASSISTANT (dlg), box2, _("Local DTD file"));
  	gtk_assistant_set_page_type (GTK_ASSISTANT (dlg), box2, GTK_ASSISTANT_PAGE_CONFIRM);

}

static void infbw_name_changed (GtkWidget *widget, gpointer data) {
  GtkAssistant *assistant = GTK_ASSISTANT (data);
  GtkWidget *current_page;
  gint page_number;
  const gchar *text;
  gpointer ptr;

  page_number = gtk_assistant_get_current_page (assistant);
  current_page = gtk_assistant_get_nth_page (assistant, page_number);
  text = gtk_entry_get_text (GTK_ENTRY (widget));

  if (text && *text) {
    gtk_assistant_set_page_complete (assistant, current_page, TRUE);
    ptr = g_object_get_data(G_OBJECT(current_page),"edata");
    if (ptr) {
    	((TentryData*)ptr)->name = g_strdup(text);
    }
  }  
  else {
    gtk_assistant_set_page_complete (assistant, current_page, FALSE);
    ptr = g_object_get_data(G_OBJECT(current_page),"edata");
    if (ptr) {
    	if ( ((TentryData*)ptr)->name ) {
    		g_free(((TentryData*)ptr)->name);
    		((TentryData*)ptr)->name = NULL;
    	}	
    }    
  }  
}

static void infbw_desc_changed (GtkWidget *widget, gpointer data) {
  GtkAssistant *assistant = GTK_ASSISTANT (data);
  GtkWidget *current_page;
  gint page_number;
  const gchar *text;
  gpointer ptr;

  page_number = gtk_assistant_get_current_page (assistant);
  current_page = gtk_assistant_get_nth_page (assistant, page_number);
  text = gtk_entry_get_text (GTK_ENTRY (widget));

  if (text && *text) {
    ptr = g_object_get_data(G_OBJECT(current_page),"edata");
    if (ptr) {
    	((TentryData*)ptr)->desc = g_strdup(text);
    }
  }  
  else {
    ptr = g_object_get_data(G_OBJECT(current_page),"edata");
    if (ptr) {
    	if ( ((TentryData*)ptr)->desc ) {
    		g_free(((TentryData*)ptr)->desc);
    		((TentryData*)ptr)->name = NULL;
    	}	
    }    
  }  
}



static void create_page_info(GtkWidget *dlg,TentryData *edata) {
  GtkWidget *box, *label, *entry,*box2;

  box = gtk_vbox_new (FALSE, 0);
  box2 = gtk_hbox_new (TRUE, 5);
  gtk_box_pack_start (GTK_BOX (box2), box, TRUE, TRUE, 5);
  
  label = gtk_label_new (_("Entry name"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label), 2, 2);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (box), entry, TRUE, TRUE, 5);
  g_signal_connect (G_OBJECT (entry), "changed", G_CALLBACK (infbw_name_changed), dlg);

  label = gtk_label_new (_("Description"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);  
  gtk_misc_set_padding (GTK_MISC (label), 2, 2);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);  


  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (box), entry, TRUE, TRUE, 5);
  g_signal_connect (G_OBJECT (entry), "changed", G_CALLBACK (infbw_desc_changed), dlg);
  
  gtk_widget_show_all (box2);
  g_object_set_data(G_OBJECT(box2),"edata",edata);
  
  gtk_assistant_append_page (GTK_ASSISTANT (dlg), box2);
  gtk_assistant_set_page_title (GTK_ASSISTANT (dlg), box2, _("Description"));
  gtk_assistant_set_page_type (GTK_ASSISTANT (dlg), box2, GTK_ASSISTANT_PAGE_CONTENT);

}


static void infbw_uri_changed (GtkWidget *widget, gpointer data) {
  GtkAssistant *assistant = GTK_ASSISTANT (data);
  GtkWidget *current_page;
  gint page_number;
  const gchar *text;
  gpointer ptr;

  page_number = gtk_assistant_get_current_page (assistant);
  current_page = gtk_assistant_get_nth_page (assistant, page_number);
  text = gtk_entry_get_text (GTK_ENTRY (widget));

  if (text && *text) {
    gtk_assistant_set_page_complete (assistant, current_page, TRUE);
    ptr = g_object_get_data(G_OBJECT(current_page),"edata");
    if (ptr) {
    	((TentryData*)ptr)->uri = g_strdup(text);
    }
  }  
  else {
    gtk_assistant_set_page_complete (assistant, current_page, FALSE);
    ptr = g_object_get_data(G_OBJECT(current_page),"edata");
    if (ptr) {
    	if ( ((TentryData*)ptr)->uri ) {
    		g_free(((TentryData*)ptr)->uri);
    		((TentryData*)ptr)->uri = NULL;
    	}	
    }    
  }  
}


static void create_page_dtd_remote(GtkWidget *dlg,TentryData *edata) {
  GtkWidget *box, *label, *entry,*box2;

  box = gtk_vbox_new (FALSE, 2);
  box2 = gtk_hbox_new (TRUE, 5);
  gtk_box_pack_start (GTK_BOX (box2), box, TRUE, TRUE, 5);
  
  label = gtk_label_new (_("DTD file URI"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label), 2, 2);  
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 2);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 5);
  g_signal_connect (G_OBJECT (entry), "changed", G_CALLBACK (infbw_uri_changed), dlg);
 
  gtk_widget_show_all (box2);
  g_object_set_data(G_OBJECT(box2),"edata",edata);
  
  gtk_assistant_append_page (GTK_ASSISTANT (dlg), box2);
  gtk_assistant_set_page_title (GTK_ASSISTANT (dlg), box2, _("Remote DTD file"));
  gtk_assistant_set_page_type (GTK_ASSISTANT (dlg), box2, GTK_ASSISTANT_PAGE_CONFIRM);

}



static void infbw_close_cancel (GtkWidget *widget, gpointer data) {
  GtkWidget **dlg = (GtkWidget **) data;
  GtkAssistant *assistant = GTK_ASSISTANT (*dlg);
  GtkWidget *page = gtk_assistant_get_nth_page (assistant, 0);
  gpointer ptr = g_object_get_data(G_OBJECT(page),"edata");
  if ( ptr ) {  	
  		TentryData *dat = (TentryData*)ptr;
  		if (dat->name) g_free(dat->name);
  		if (dat->desc) g_free(dat->desc);
  		if (dat->uri) g_free(dat->uri);
  	  	g_free(dat);
  }  
  gtk_widget_destroy (*dlg);
  *dlg = NULL;
}


gint  infbw_next_page  (gint current_page,gpointer data) {
  	GtkAssistant *assistant = GTK_ASSISTANT (data);
  	GtkWidget *page = gtk_assistant_get_nth_page (assistant, current_page);
  	gpointer ptr = g_object_get_data(G_OBJECT(page),"edata");
  	 
	
  	if (ptr==NULL) return -1;
  		 
	switch ( current_page ) 
	{
		case 1:
			switch(((TentryData*)ptr)->type) {
				case ENTRY_TYPE_DTD_LOCAL: return 2;
				case ENTRY_TYPE_DTD_REMOTE: return 3;
				default: return -1;
			}
		break;
	  	case 2:
	  	case 3:
  			ptr = g_object_get_data(G_OBJECT(page),"edata");
  			if (ptr) {
  				infbw_save_entry((TentryData*)ptr);
  			}
  			return -1;
  		break;		
		default:
			if ( current_page + 1 < gtk_assistant_get_n_pages(assistant) )   
				return current_page+1;
			else 
				return -1;		
	}  
}


void infb_add_item(Tbfwin *bfwin) {
	TentryData *edata = g_new0(TentryData,1);
	edata->win = bfwin;	
	if (!assistant)
    {	
		assistant = gtk_assistant_new ();
		/*gtk_window_set_default_size (GTK_WINDOW (dlg), -1, 300);*/
		create_page_type(assistant,edata);
		create_page_info(assistant,edata);
		create_page_dtd_file(assistant,edata); /* page number 2 */
		create_page_dtd_remote(assistant,edata); /* page number 3 */		
		g_signal_connect (G_OBJECT (assistant), "cancel",G_CALLBACK (infbw_close_cancel), &assistant);
   	g_signal_connect (G_OBJECT (assistant), "close",G_CALLBACK (infbw_close_cancel), &assistant);
   	gtk_assistant_set_forward_page_func (GTK_ASSISTANT(assistant),infbw_next_page,assistant,NULL);
	 }
	  
	gtk_window_set_modal(GTK_WINDOW(assistant),TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(assistant),GTK_WINDOW(bfwin->main_window));
	gtk_window_set_position(GTK_WINDOW(assistant),GTK_WIN_POS_CENTER_ON_PARENT);
  	gtk_widget_show_all(assistant);

   /*gtk_widget_destroy(assistant);*/ 
}


void infbw_selected(GtkTreeView *view,gpointer user_data) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkAssistant *assistant = GTK_ASSISTANT (user_data);
  	GtkWidget *current_page;
  	gint page_number;
  	gchar *val,*val2;
  	GtkTreeSelection *sel = gtk_tree_view_get_selection(view);
  	gpointer ptr;

	page_number = gtk_assistant_get_current_page (assistant);
	current_page = gtk_assistant_get_nth_page (assistant, page_number);

	if ( sel && gtk_tree_selection_get_selected(sel,&model,&iter) )
	{
		gtk_tree_model_get(model,&iter,1,&val,0,&val2,-1);
		ptr = g_object_get_data(G_OBJECT(current_page),"edata");
		if (ptr) {	
			((TentryData*)ptr)->uri = g_strdup(val);
			((TentryData*)ptr)->name = g_strdup(val2);
		} 
		gtk_assistant_set_page_complete (assistant, current_page, TRUE);
	}
	else {
		ptr = g_object_get_data(G_OBJECT(current_page),"edata");
		if (ptr) {
			if ( ((TentryData*)ptr)->uri ) {
				g_free(((TentryData*)ptr)->uri);
				((TentryData*)ptr)->uri = NULL;
			}
			if ( ((TentryData*)ptr)->name ) {
				g_free(((TentryData*)ptr)->name);
				((TentryData*)ptr)->name = NULL;
			}
		}
		gtk_assistant_set_page_complete (assistant, current_page, FALSE);
	}	 
}

static void create_page_select(GtkWidget *dlg,TentryData *edata) {
  GtkWidget *box, *label, *entry,*box2;
  GtkWidget *scrolledwindow1;
  GtkListStore *store;
  GtkCellRenderer* rend = gtk_cell_renderer_text_new(); 
  GtkTreeIter iter;
  GList *list = NULL,*lpt;

  box = gtk_vbox_new (FALSE, 2);
  box2 = gtk_hbox_new (TRUE, 5);
  gtk_box_pack_start (GTK_BOX (box2), box, TRUE, TRUE, 5);
  
  label = gtk_label_new (_("Entries you can delete"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label), 2, 2);  
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 2);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow1), 
  											GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (box), scrolledwindow1, FALSE, FALSE, 5);
  
  store = gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_STRING);
  list = infb_user_files();
  lpt = list;
  while (lpt) {
  	gchar **arr = g_strsplit(lpt->data,",",-1);
  	if (arr[0] && arr[1] ) {
	  gtk_list_store_append(store,&iter);
	  gtk_list_store_set(store,&iter,0,arr[0],1,arr[1],-1); 
  	}
  	lpt = g_list_next(lpt);
  }
  entry = gtk_tree_view_new ();
  gtk_tree_view_set_model(GTK_TREE_VIEW(entry),GTK_TREE_MODEL(store)); 
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), entry);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (entry), FALSE);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(entry),0,"",rend,"text",0,NULL);
  g_signal_connect(G_OBJECT(entry),"cursor-changed",G_CALLBACK(infbw_selected),dlg);
 
  gtk_widget_show_all (box2);
  g_object_set_data(G_OBJECT(box2),"edata",edata);
  
  gtk_assistant_append_page (GTK_ASSISTANT (dlg), box2);
  gtk_assistant_set_page_title (GTK_ASSISTANT (dlg), box2, _("Entry selection"));
  gtk_assistant_set_page_type (GTK_ASSISTANT (dlg), box2, GTK_ASSISTANT_PAGE_CONTENT);

}

typedef struct {
	GtkWidget *label1,*label2;
} TselLabels;

void infbw_select_prepare (GtkAssistant *assistant,GtkWidget *page,gpointer user_data) {
	TselLabels *labs = (TselLabels*)user_data;
	gpointer ptr = g_object_get_data(G_OBJECT(page),"edata");
	gchar *pstr;
	
	if (ptr) {
		pstr = g_strconcat("<b>Entry name:</b>",((TentryData*)ptr)->name,NULL);
		gtk_label_set_markup(GTK_LABEL(labs->label1),pstr);
		g_free(pstr);
		pstr = g_strconcat("<b>Entry file:</b>",((TentryData*)ptr)->uri,NULL);
		gtk_label_set_markup(GTK_LABEL(labs->label2),pstr);
		g_free(pstr);		
	}
} 

static void create_page_select_confirm(GtkWidget *dlg,TentryData *edata) {
  GtkWidget *box, *label, *box2;
  gchar *pstr;
  TselLabels *labList = g_new0(TselLabels,1);

  box = gtk_vbox_new (FALSE, 2);
  box2 = gtk_hbox_new (TRUE, 5);
  gtk_box_pack_start (GTK_BOX (box2), box, TRUE, TRUE, 5);
  
  label = gtk_label_new (_("Confirm entry removal"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label), 2, 2);  
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 2);
  labList->label1 = label;

  pstr = g_strconcat("Entry name: ",edata->name,"\nEntry file: ",edata->uri,NULL);	
  label = gtk_label_new (pstr);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label), 2, 2);  
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 2);
  g_free(pstr);	
  labList->label2 = label;

  gtk_widget_show_all (box2);
  g_object_set_data(G_OBJECT(box2),"edata",edata);
  
  gtk_assistant_append_page (GTK_ASSISTANT (dlg), box2);
  gtk_assistant_set_page_title (GTK_ASSISTANT (dlg), box2, _("Confirmation"));
  gtk_assistant_set_page_type (GTK_ASSISTANT (dlg), box2, GTK_ASSISTANT_PAGE_CONFIRM);
  g_signal_connect(G_OBJECT(assistant),"prepare",G_CALLBACK(infbw_select_prepare),labList);
  gtk_assistant_set_page_complete (assistant, box2, TRUE);	
}

void infbw_del_apply(GtkAssistant *assistant,gpointer user_data) {
	gint page_number = gtk_assistant_get_current_page (assistant);
	GtkWidget *current_page = gtk_assistant_get_nth_page (assistant, page_number);
	Tbfwin *win = (Tbfwin*)user_data;
	gpointer ptr;
	
	if (page_number==1) {
		ptr = g_object_get_data(G_OBJECT(current_page),"edata");
		if (ptr) {
			g_remove(((TentryData*)ptr)->uri);
			infb_reload_home(win);
		}	
	}
}  

void infb_del_item(Tbfwin *bfwin) {
	TentryData *edata = g_new0(TentryData,1);
	edata->win = bfwin;	
	if (!assistant)
    {	
		assistant = gtk_assistant_new ();

		create_page_select(assistant,edata);
		create_page_select_confirm(assistant,edata);
		g_signal_connect (G_OBJECT (assistant), "cancel",G_CALLBACK (infbw_close_cancel), &assistant);
   	g_signal_connect (G_OBJECT (assistant), "close",G_CALLBACK (infbw_close_cancel), &assistant);
   	g_signal_connect (G_OBJECT (assistant), "apply",G_CALLBACK (infbw_del_apply), bfwin);
	 }	  
	gtk_window_set_modal(GTK_WINDOW(assistant),TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(assistant),GTK_WINDOW(bfwin->main_window));
	gtk_window_set_position(GTK_WINDOW(assistant),GTK_WIN_POS_CENTER_ON_PARENT);
  	gtk_widget_show_all(assistant);
}

