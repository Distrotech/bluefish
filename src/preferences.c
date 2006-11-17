/* Bluefish HTML Editor
 * preferences.c - the preferences code
 *
 * Copyright (C) 2002-2006 Olivier Sessink
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
#include <string.h>        /* strcmp() */

#include "bluefish.h"
#include "autocomp.h"
#include "bf_lib.h"        /* list_switch_order() */
#include "bf-textview.h"
#include "document.h"
#include "filebrowser2.h"
#include "gtk_easy.h"
#include "gui.h"
#include "menu.h"
#include "pixmap.h"
#include "rcfile.h"
#include "textstyle.h"
#include "stringlist.h"    /* duplicate_arraylist*/

enum {
	do_periodic_check,
	view_html_toolbar,
	view_custom_menu,
	view_main_toolbar,
	view_left_panel,
	view_line_numbers,
	filebrowser_two_pane_view,
	filebrowser_unknown_icon,
	editor_show_splash_screen,    /* show splash screen at start-up */
	editor_font_string,           /* editor font */
	editor_tab_width,             /* editor tabwidth */
	editor_indent_wspaces,
	editor_smart_cursor,
	tab_font_string,              /* notebook tabs font */	
	defaulthighlight,             /* highlight documents by default */
	transient_htdialogs,          /* set html dialogs transient ro the main window */
	restore_dimensions,
	left_panel_width,
	left_panel_left,
	main_window_h,                /* main window height */
	main_window_w,                /* main window width */
	max_recent_files,             /* length of Open Recent list */
	max_dir_history,              /* length of directory history */
	backup_file,                  /* wheather to use a backup file */
	backup_suffix,                /* the string to append to the backup file */
	backup_prefix,                /* the string to prepend to the backup file */
	backup_abort_action,          /* if the backup fails, continue 'save', 'abort' save, or 'ask' user */
	backup_cleanuponclose,        /* remove the backupfile after close ? */
	image_thumbnailstring,        /* string to append to thumbnail filenames */
	image_thumbnailtype,          /* fileformat to use for thumbnails, "jpeg" or "png" can be handled by gdkpixbuf*/
	modified_check_type,          /* 0=no check, 1=by mtime and size, 2=by mtime, 3=by size, 4,5,...not implemented (md5sum?) */
	num_undo_levels,              /* number of undo levels per document */
	clear_undo_on_save,           /* clear all undo information on file save */
	newfile_default_encoding,     /* if you open a new file, what encoding will it use */
	auto_set_encoding_meta,       /* auto set metatag for the encoding */
	auto_update_meta_author,      /* auto update author meta tag */
	auto_update_meta_date,        /* auto update date meta tag */
	auto_update_meta_generator,   /* auto update generator meta tag */
	document_tabposition,
	leftpanel_tabposition,
	/* not yet in use */
	image_editor_cline,           /* image editor commandline */
	full_p,                       /* use </p> */
	full_li,                      /* use </li> */
	allow_css,                    /* CSS allowed */
	allow_dep,                    /* allow <FONT>... */
	format_by_context,            /* use <strong> instead of <b>, <emphasis instead of <i> etc. (W3C reccomendation) */
	xhtml,                        /* write <br /> */
	allow_ruby,                   /* allow <ruby> */
	allow_h4,                     /* allow <Q>... */
	allow_frames,                 /* allow <FRAME> */
	force_dtd,                    /* write <!DOCTYPE...> */
	dtd_url,                      /* URL in DTD */
	xml_start,                    /* <?XML...> */
	lowercase_tags,               /* use lowercase tags */
	word_wrap,                    /* use wordwrap */
	autoindent,                   /* autoindent code */
	drop_at_drop_pos,             /* drop at drop position instead of cursor position */
	link_management,              /* perform link management */
	html_ver,
	cust_menu,                    /* entries in the custom menu */
#ifdef WITH_SPC
	/* spell checker options */
	cfg_spc_cline,                /* spell checker command line */
	cfg_spc_lang,                 /* language */
	spc_accept_compound,          /* accept compound words ? */
	spc_use_esc_chars,            /* specify aditional characters that may be part of a word ? */
	spc_esc_chars,                /* which ones ? */
	spc_use_pers_dict,            /* use a personal dictionary */
	spc_pers_dict,                /* which one ? */
   spc_use_input_encoding,        /* use input encoding */
   spc_input_encoding,            /* wich one ? */
   spc_output_html_chars,         /* output html chars ? (like &aacute,)*/
#endif
	/* key conversion */
	conv_ctrl_enter,              /* convert control-enter key press */
	ctrl_enter_text,              /* inserted text */
	conv_shift_enter,             /* convert shift-enter key press */
	shift_enter_text,             /* inserted text */
	conv_special_char,            /* convert ctrl-'<','>','&' */
#ifdef WITH_MSG_QUEUE
	open_in_running_bluefish,     /* open commandline documents in already running session*/
#endif /* WITH_MSG_QUEUE */
	server_zope_compat,
	bflib_info_font,
	bflib_info_bkg,
	bflib_info_fg,
	view_blocks,
	view_symbols,		
	view_mbhl,
	view_cline,
	scan_mode,
	editor_fg,
	editor_bg,
	view_rmargin,
	rmargin_at,
	autocomp_key,	
	property_num_max
};

#define FILETYPES_ARRAY_LEN 8
enum {
	extcommands,
	extfilters,
	extoutputbox,
	pluginconfig,
	textstyles,
	syntax_styles,
	lists_num_max
};

typedef struct {
	GtkListStore *lstore;
	GtkWidget *lview;
	int insertloc;
	GList **thelist;
} Tlistpref;

typedef struct {
	GtkListStore *lstore;
	GtkWidget *lview;
	int insertloc;
	GList **thelist;
	/* the above is identical to Tlistpref, andf makes it possible to typecast this to Tlistpref */
	gchar **curstrarr;
	GtkWidget *bg_color,*fg_color;
	GtkWidget *bold_radio[3];
	GtkWidget *italic_radio[3];
} Ttextstylepref;

typedef struct {
	GtkTreeStore *tstore;
	GtkWidget *tview;
	GtkWidget *textstyle;
	GtkListStore *cstore;
	gchar **curstrarr;
} Thldialog;

enum {
	NAMECOL,
	WIDGETCOL
};

typedef struct {
	GtkListStore *lstore;
	GtkWidget *lview;
} Tplugindialog;

typedef struct {
	GtkWidget *prefs[property_num_max];
	GList *lists[lists_num_max];
	GtkWidget *win;
	GtkWidget *noteb;
	GtkTreeStore *nstore;
	GtkWidget *fixed;
	Tlistpref ftd; /* FileTypeDialog */
	Tlistpref ffd; /* FileFilterDialog */
	Ttextstylepref tsd; /* TextStyleDialog */
	Thldialog hld;
	GtkListStore *lang_files;
	Tlistpref bd;
	Tlistpref ed;
	Tlistpref od;
	Tplugindialog pd;
} Tprefdialog;

typedef enum {
	string_none,
	string_file,
	string_font,
	string_color,
	string_accel
} Tprefstringtype;

void pref_click_column  (GtkTreeViewColumn *treeviewcolumn, gpointer user_data) {
	GtkToggleButton *but = GTK_TOGGLE_BUTTON(user_data);
	GList *lst = gtk_tree_view_column_get_cell_renderers(treeviewcolumn);
	if ( gtk_toggle_button_get_active(but))
	{
		gtk_toggle_button_set_active(but,FALSE);
		gtk_button_set_label(GTK_BUTTON(but),"");
		gtk_tree_view_column_set_sizing(treeviewcolumn,GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_fixed_width(treeviewcolumn,30);
		if (lst) {
			g_object_set(G_OBJECT(lst->data),"sensitive",FALSE,"mode",GTK_CELL_RENDERER_MODE_INERT,NULL);
			g_list_free(lst);
		}	
	}	
	else
	{
		gtk_toggle_button_set_active(but,TRUE);
		gtk_button_set_label(GTK_BUTTON(but),(gchar*)g_object_get_data(G_OBJECT(but),"_title_"));
		gtk_tree_view_column_set_sizing(treeviewcolumn,GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		if (lst) {
			g_object_set(G_OBJECT(lst->data),"sensitive",TRUE,"mode",GTK_CELL_RENDERER_MODE_EDITABLE,NULL);
			g_list_free(lst);
		}			
	}
}
/*
#ifdef USE_SCANNER
static void pref_create_combo_column(GtkTreeView *treeview,GCallback func, gpointer data, const gchar *title, gint num,GtkListStore *store) {
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkWidget *but;
		renderer = gtk_cell_renderer_combo_new();
		if (func) {
			g_object_set(G_OBJECT(renderer), "editable", TRUE, 
	             "model", store,
                "text-column", 0,
                "has-entry", FALSE,	NULL);
			g_signal_connect(G_OBJECT(renderer), "edited", func, data);
		}
	column = gtk_tree_view_column_new_with_attributes(title, renderer,"text",num,NULL);

	gtk_tree_view_column_set_clickable(GTK_TREE_VIEW_COLUMN(column),TRUE);
	but = gtk_check_button_new_with_label(title);
	g_object_set_data(G_OBJECT(but),"_title_",g_strdup(title));
	gtk_widget_show(but);
	gtk_tree_view_column_set_widget(GTK_TREE_VIEW_COLUMN(column),but);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(but),TRUE);
	g_signal_connect(G_OBJECT(column), "clicked", G_CALLBACK(pref_click_column), but);

	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
}
#endif
*/
/* type 0/1=text, 2=toggle,3=radio, 4=non-editable combo */
static GtkCellRenderer *pref_create_column(GtkTreeView *treeview, gint type, GCallback func, gpointer data, const gchar *title, gint num) {
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkWidget *but;
	if (type == 1 || type == 0) {
		renderer = gtk_cell_renderer_text_new();
		if (func) {
			g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
			g_signal_connect(G_OBJECT(renderer), "edited", func, data);
		}
	} else if (type == 2 || type == 3) {
		renderer = gtk_cell_renderer_toggle_new();
		if (type == 3) {
			gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(renderer),TRUE);
		}
		if (func) {
			g_object_set(G_OBJECT(renderer), "activatable", TRUE, NULL);
			g_signal_connect(G_OBJECT(renderer), "toggled", func, data);
		}
	} else /* if (type ==4) */ {
		renderer = gtk_cell_renderer_combo_new();
		g_object_set(G_OBJECT(renderer), "has-entry", FALSE, NULL);
	/* should be done by the calling function: g_object_set(G_OBJECT(renderer), "model", model, NULL);*/
		g_object_set(G_OBJECT(renderer), "text-column", 0, NULL);
		if (func) {
			g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
			g_signal_connect(G_OBJECT(renderer), "edited", func, data);
		}
	}
	column = gtk_tree_view_column_new_with_attributes(title, renderer,(type ==1) ? "text" : "active" ,num,NULL);

	gtk_tree_view_column_set_clickable(GTK_TREE_VIEW_COLUMN(column),TRUE);
	but = gtk_check_button_new_with_label(title);
	g_object_set_data(G_OBJECT(but),"_title_",g_strdup(title));
	gtk_widget_show(but);
	gtk_tree_view_column_set_widget(GTK_TREE_VIEW_COLUMN(column),but);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(but),TRUE);
	g_signal_connect(G_OBJECT(column), "clicked", G_CALLBACK(pref_click_column), but);

	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	return renderer;
}
/* 3 entries must have len 3, but in reality it will be 4, because there is a NULL pointer appended */
static gchar **pref_create_empty_strarr(gint len) {
	gchar **strarr = g_malloc0((len+1)*sizeof(gchar *));
	gint i;
	strarr[0] = g_strdup(_("Untitled"));
	for (i=1;i<len;i++) {
		strarr[i] = g_strdup("");
	}
	strarr[len] = NULL;
	return strarr;
}
/* type 0=escapedtext, 1=text, 2=toggle */
static void pref_apply_change(GtkListStore *lstore, gint pointerindex, gint type, gchar *path, gchar *newval, gint index) {
	gchar **strarr;
	GtkTreeIter iter;
	GtkTreePath* tpath = gtk_tree_path_new_from_string(path);
	if (tpath && gtk_tree_model_get_iter(GTK_TREE_MODEL(lstore),&iter,tpath)) {
		gtk_tree_model_get(GTK_TREE_MODEL(lstore), &iter, pointerindex, &strarr, -1);
		DEBUG_MSG("pref_apply_change, lstore=%p, index=%d, type=%d, got strarr=%p\n",lstore,index,type,strarr);
		if (type ==1) {
			gtk_list_store_set(GTK_LIST_STORE(lstore),&iter,index,newval,-1);
		} else {
			gtk_list_store_set(GTK_LIST_STORE(lstore),&iter,index,(newval[0] == '1'),-1);
		}
		if (strarr[index]) {
			DEBUG_MSG("pref_apply_change, old value for strarr[%d] was %s\n",index,strarr[index]);
			g_free(strarr[index]);
		}
		if (type == 0) {
			strarr[index] = unescape_string(newval, FALSE);
		} else {
			strarr[index] = g_strdup(newval);
		}
		DEBUG_MSG("pref_apply_change, strarr[%d] now is %s\n",index,strarr[index]);
	} else {
		DEBUG_MSG("ERROR: path %s was not converted to tpath(%p) or iter (lstore=%p)\n",path,tpath,lstore);
	}
	gtk_tree_path_free(tpath);
}
static void pref_delete_strarr(Tprefdialog *pd, Tlistpref *lp, gint pointercolumn) {
	GtkTreeIter iter;
	GtkTreeSelection *select;
	lp->insertloc = -1;
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(lp->lview));
	if (gtk_tree_selection_get_selected (select,NULL,&iter)) {
		gchar **strarr;
		gtk_tree_model_get(GTK_TREE_MODEL(lp->lstore), &iter, pointercolumn, &strarr, -1);
		gtk_list_store_remove(GTK_LIST_STORE(lp->lstore),&iter);
		*lp->thelist = g_list_remove(*lp->thelist, strarr);
		g_strfreev(strarr);
	}
}

static void listpref_row_inserted(GtkTreeModel *treemodel,GtkTreePath *arg1,GtkTreeIter *arg2,Tlistpref *lp) {
	gint *indices = gtk_tree_path_get_indices(arg1);
	if (indices) {
		lp->insertloc = indices[0];
		DEBUG_MSG("reorderable_row_inserted, insertloc=%d\n",lp->insertloc);
	}
}
static void listpref_row_deleted(GtkTreeModel *treemodel,GtkTreePath *arg1,Tlistpref *lp) {
	if (lp->insertloc > -1) {
		gint *indices = gtk_tree_path_get_indices(arg1);
		if (indices) {
			GList *lprepend, *ldelete;
			gint deleteloc = indices[0];
			if (deleteloc > lp->insertloc) deleteloc--;
			DEBUG_MSG("reorderable_row_deleted, deleteloc=%d, insertloc=%d, listlen=%d\n",deleteloc,lp->insertloc,g_list_length(*lp->thelist));
			*lp->thelist = g_list_first(*lp->thelist);
			lprepend = g_list_nth(*lp->thelist,lp->insertloc);
			ldelete = g_list_nth(*lp->thelist,deleteloc);
			if (ldelete && (ldelete != lprepend)) {
				gpointer data = ldelete->data;
				*lp->thelist = g_list_remove(*lp->thelist, data);
				if (lprepend == NULL) {
					DEBUG_MSG("lprepend=NULL, appending %s to the list\n",((gchar **)data)[0]);
					*lp->thelist = g_list_append(g_list_last(*lp->thelist), data);
				} else {
					DEBUG_MSG("lprepend %s, ldelete %s\n",((gchar **)lprepend->data)[0], ((gchar **)data)[0]);
					*lp->thelist = g_list_prepend(lprepend, data);
				}
				*lp->thelist = g_list_first(*lp->thelist);
			} else {
				DEBUG_MSG("listpref_row_deleted, ERROR: ldelete %p, lprepend %p\n", ldelete, lprepend);
			}
		}
		lp->insertloc = -1;
	}
}

static void font_dialog_response_lcb(GtkDialog *fsd,gint response,GtkWidget *entry) {
	DEBUG_MSG("font_dialog_response_lcb, response=%d\n", response);
	if (response == GTK_RESPONSE_OK) {
		gchar *fontname;
		fontname = gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(fsd));
		gtk_entry_set_text(GTK_ENTRY(entry), fontname);
		g_free(fontname);
	}
	gtk_widget_destroy(GTK_WIDGET(fsd));
}

static void font_button_lcb(GtkWidget *wid, GtkWidget *entry) {
	GtkWidget *fsd;
	const gchar *fontname;
	fsd = gtk_font_selection_dialog_new(_("Select font"));
	fontname = gtk_entry_get_text(GTK_ENTRY(entry)); /* do NOT free, this is an internal pointer */
	if (strlen(fontname)) {
		gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(fsd), fontname);
	}
	g_signal_connect(GTK_OBJECT(fsd),"response",G_CALLBACK(font_dialog_response_lcb),entry);
	gtk_window_set_transient_for(GTK_WINDOW(GTK_DIALOG(fsd)), GTK_WINDOW(gtk_widget_get_toplevel(entry)));
	gtk_window_set_modal(GTK_WINDOW(GTK_DIALOG(fsd)), TRUE);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(GTK_DIALOG(fsd)), TRUE);
	gtk_widget_show(fsd);
}

static void color_dialog_response_lcb(GtkDialog *fsd,gint response,GtkWidget *entry) {
	if (response == GTK_RESPONSE_OK) {
		GdkColor cc;
		gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(fsd)->colorsel),&cc);
		gtk_entry_set_text(GTK_ENTRY(entry), gdk_color_to_hexstring(&cc,TRUE));
	}
	gtk_widget_destroy(GTK_WIDGET(fsd));
}

static void color_button_lcb(GtkWidget *wid, GtkWidget *entry) {
	GtkWidget *fsd;
	const gchar *cname;
	GdkColor cc;
	fsd = gtk_color_selection_dialog_new(_("Select color"));
	cname = gtk_entry_get_text(GTK_ENTRY(entry)); /* do NOT free, this is an internal pointer */
	if (strlen(cname)) {
		gdk_color_parse(cname,&cc);
		gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(fsd)->colorsel), &cc);
	}
	g_signal_connect(GTK_OBJECT(fsd),"response",G_CALLBACK(color_dialog_response_lcb),entry);
	gtk_window_set_transient_for(GTK_WINDOW(GTK_DIALOG(fsd)), GTK_WINDOW(gtk_widget_get_toplevel(entry)));
	gtk_window_set_modal(GTK_WINDOW(GTK_DIALOG(fsd)), TRUE);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(GTK_DIALOG(fsd)), TRUE);
	gtk_widget_show(fsd);
}

static void accel_button_lcb(GtkWidget *wid, gpointer data) 
{
	gchar *name = ac_key_choice();
	if (name)
		gtk_button_set_label(GTK_BUTTON(wid),name);
}

static GtkWidget *prefs_string(const gchar *title, const gchar *curval, GtkWidget *box, Tprefdialog *pd, Tprefstringtype prefstringtype) {
	GtkWidget *hbox, *return_widget;

	hbox = gtk_hbox_new(FALSE,3);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(title), FALSE, FALSE, 3);
	if (prefstringtype == string_color) {
		return_widget = boxed_entry_with_text(curval, 8, hbox);
		gtk_entry_set_width_chars(GTK_ENTRY(return_widget),8);
	}
	else if (prefstringtype == string_accel)
	{
		return_widget = bf_allbuttons_backend(curval, FALSE, -1, G_CALLBACK(accel_button_lcb), NULL);;
		gtk_box_pack_start(GTK_BOX(hbox), return_widget, FALSE, FALSE, 3);	
	}	
	else
		return_widget = boxed_entry_with_text(curval, 1023, hbox);
	if (prefstringtype == file) {
		gtk_box_pack_start(GTK_BOX(hbox), file_but_new(return_widget, 1, NULL), FALSE, FALSE, 3);
	} else if (prefstringtype == font) {
		GtkWidget *but = bf_gtkstock_button(GTK_STOCK_SELECT_FONT, G_CALLBACK(font_button_lcb), return_widget);
		gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 3);
	}	 else if (prefstringtype == string_color) {
		GtkWidget *but = bf_gtkstock_button(GTK_STOCK_SELECT_COLOR, G_CALLBACK(color_button_lcb), return_widget);
		gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 3);
	}
	return return_widget;
}

static GtkWidget *prefs_combo(const gchar *title, const gchar *curval, GtkWidget *box, Tprefdialog *pd, GList *poplist, gboolean editable) {
	GtkWidget *return_widget;
	GtkWidget *hbox;

	hbox = gtk_hbox_new(FALSE,3);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(title), FALSE, FALSE, 3);
	return_widget = boxed_combo_with_popdown(curval, poplist, editable, hbox);
	return return_widget;
}

static GtkWidget *prefs_integer(const gchar *title, const gint curval, GtkWidget *box, Tprefdialog *pd, gfloat lower, gfloat upper) {
	GtkWidget *return_widget;
	GtkWidget *hbox;
	GtkObject *adjust;
	gfloat step_increment, page_increment;

	step_increment = (upper - lower)/1000;
	if (step_increment < 1) {
		step_increment = 1;
	}
	page_increment = (upper - lower)/20;
	if (page_increment < 10) {
		page_increment = 10;
	}
	hbox = gtk_hbox_new(FALSE,3);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 3);
	adjust = gtk_adjustment_new((1.0 * curval), lower, upper, step_increment ,page_increment , 0);
	return_widget = gtk_spin_button_new(GTK_ADJUSTMENT(adjust), 0.1, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(title), FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), return_widget, TRUE, TRUE, 3);
	return return_widget;
}


/*************************/
/* GENERAL FUNCTIONS     */
/*************************/
/* static GList *general_poplist(GList *arraylist, gint required_items, gint poplist_label_index) {
	GList *tmplist, *poplist=NULL;
	
	tmplist = g_list_first(arraylist);
	while(tmplist){
		gchar **strarr =(gchar **)tmplist->data;
		if (count_array(strarr) >= required_items) poplist = g_list_append(poplist, strarr[poplist_label_index]);
		tmplist = g_list_next(tmplist);
	}
	return poplist;
}*/
/*
static void generic_selection_changed_cb(GtkTreeSelection *selection
	, GtkWidget **entry, void (*apply_func)(), Tprefdialog *pd,gint whichlist, gint numentries, gchar ***newcurstrarr){
	GtkTreeIter iter;
	GtkTreeModel *model;
	DEBUG_MSG("generic_selection_changed_cb, started\n");
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gchar *value;
		GList *tmplist = g_list_first(pd->lists[whichlist]);
		gtk_tree_model_get(model, &iter, 0, &value, -1);
		apply_func(pd);
		while (tmplist) {
			gchar **strarr =(gchar **)tmplist->data;
			if (strcmp(strarr[0],value)==0) {
				gint i;
				for (i=0;i<numentries;i++) {
					gtk_entry_set_text(GTK_ENTRY(entry[i]), strarr[i]);
				}
				DEBUG_MSG("generic_selection_changed_cb, newcurstrarr(%p) is now %p\n", newcurstrarr, *newcurstrarr);
				*newcurstrarr = strarr;
				DEBUG_MSG("generic_selection_changed_cb, newcurstrarr(%p) set to %p (check %p)\n", newcurstrarr, strarr, *newcurstrarr);
				return;
			}
			tmplist = g_list_next(tmplist);
		}
	} else {
		DEBUG_MSG("generic_selection_changed_cb, no selection ?!?!\n");
	}
}

static gchar **generic_create_strarr(gint numitems, GtkWidget **entries, gboolean force_firstitem_strlen) {
	gint i;
	gchar **strarr = g_malloc((numitems+1)*sizeof(gchar *));
	for (i=0;i<numitems;i++) {
		strarr[i] = gtk_editable_get_chars(GTK_EDITABLE(entries[i]), 0, -1);
	}
	strarr[numitems] = NULL;
	if (strlen(strarr[0])) {
		return strarr;
	} else {
		g_strfreev(strarr);
		return NULL;
	}
}*/

/************ plugin code ****************/
static void set_plugin_strarr_in_list(GtkTreeIter *iter, gchar **strarr, Tprefdialog *pd) {
	gint arrcount;
	arrcount = count_array(strarr);
	if (arrcount==3) {
		g_print("set_plugin_strarr_in_list, arrcount=%d, file=%s\n",arrcount,strarr[0]);
		gtk_list_store_set(GTK_LIST_STORE(pd->pd.lstore), iter
				,0,strarr[2]
				,1,(strarr[1][0] == '1')
				,2,strarr[0]
				,3,strarr,-1);
	}
}
static void plugin_1_toggled_lcb(GtkCellRendererToggle *cellrenderertoggle,gchar *path,Tprefdialog *pd) {
	gchar *val = g_strdup(cellrenderertoggle->active ? "0" : "1");
	pref_apply_change(pd->pd.lstore,3,2,path,val,1);
	g_free(val);
}
static void create_plugin_gui(Tprefdialog *pd, GtkWidget *vbox1) {
	GtkWidget *hbox, *but, *scrolwin;
	pd->lists[pluginconfig] = duplicate_arraylist(main_v->props.plugin_config);
	pd->pd.lstore = gtk_list_store_new (4,G_TYPE_STRING,G_TYPE_BOOLEAN,G_TYPE_STRING,G_TYPE_POINTER);
	pd->pd.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->pd.lstore));
	pref_create_column(GTK_TREE_VIEW(pd->pd.lview), 1, NULL, pd, _("Message"), 0);
	pref_create_column(GTK_TREE_VIEW(pd->pd.lview), 2, G_CALLBACK(plugin_1_toggled_lcb), pd, _("Enabled"), 1);
	pref_create_column(GTK_TREE_VIEW(pd->pd.lview), 1, NULL, pd, _("File"), 2);
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), pd->pd.lview);
	gtk_widget_set_size_request(scrolwin, 200, 350);
	gtk_box_pack_start(GTK_BOX(vbox1), scrolwin, TRUE, TRUE, 2);
	
	{
		GList *tmplist = g_list_first(pd->lists[pluginconfig]);
		while (tmplist) {
			gint arrcount;
			gchar **strarr = (gchar **)tmplist->data;
			arrcount = count_array(strarr);
			if (arrcount==3) {
				GtkTreeIter iter;
				gtk_list_store_append(GTK_LIST_STORE(pd->pd.lstore), &iter);
				set_plugin_strarr_in_list(&iter, strarr,pd);
			}
			tmplist = g_list_next(tmplist);
		}
	}
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox, TRUE, TRUE, 2);
/*	but = bf_gtkstock_button(GTK_STOCK_ADD, G_CALLBACK(add_new_filetype_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox),but, FALSE, FALSE, 2);
	but = bf_gtkstock_button(GTK_STOCK_DELETE, G_CALLBACK(delete_filetype_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox),but, FALSE, FALSE, 2);*/
}


/****** Text style **************/

static void set_textstyle_strarr_in_list(GtkTreeIter *iter, gchar **strarr, Tprefdialog *pd) {
	gint arrcount;
	arrcount = count_array(strarr);
	if (arrcount==5) {
		gtk_list_store_set(GTK_LIST_STORE(pd->tsd.lstore), iter
				,0,strarr[0],1,strarr,-1);
	} else {
		DEBUG_MSG("ERROR: set_textstyle_strarr_in_list, arraycount != 5 !!!!!!\n");
	}
}

static void textstyle_0_edited_lcb(GtkCellRendererText *cellrenderertext,gchar *path,gchar *newtext,Tprefdialog *pd) {
	pref_apply_change(pd->tsd.lstore, 1, 1, path, newtext, 0);
}

static void add_new_textstyle_lcb(GtkWidget *wid, Tprefdialog *pd) {
	gchar **strarr;
	GtkTreeIter iter;
	strarr = pref_create_empty_strarr(5);
	gtk_list_store_append(GTK_LIST_STORE(pd->tsd.lstore), &iter);
	set_textstyle_strarr_in_list(&iter, strarr,pd);
	pd->lists[textstyles] = g_list_append(pd->lists[textstyles], strarr);
	pd->tsd.insertloc = -1;
}

static void delete_textstyle_lcb(GtkWidget *wid, Tprefdialog *pd) {
	GtkTreeIter iter;
	GtkTreeSelection *select;
	pd->tsd.insertloc = -1;
	pd->tsd.curstrarr = NULL;
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->tsd.lview));
	if (gtk_tree_selection_get_selected (select,NULL,&iter)) {
		gchar **strarr;
		gtk_tree_model_get(GTK_TREE_MODEL(pd->tsd.lstore), &iter, 1, &strarr, -1);
		gtk_list_store_remove(GTK_LIST_STORE(pd->tsd.lstore),&iter);
		*pd->tsd.thelist = g_list_remove(*pd->tsd.thelist, strarr);
		g_strfreev(strarr);
	}
}

static void textstyle_selection_changed_cb(GtkTreeSelection *selection, Tprefdialog *pd) {
	GtkTreeIter iter;
	GtkTreeModel *model;
	DEBUG_MSG("textstyle_selection_changed_cb, started\n");
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gchar **strarr;
		gtk_tree_model_get(model, &iter, 1, &strarr, -1);
		pd->tsd.curstrarr = NULL;
		DEBUG_MSG("textstyle_selection_changed_cb, setting %s, strarr=%p\n",strarr[0], strarr);
		gtk_entry_set_text(GTK_ENTRY(pd->tsd.fg_color), strarr[1]);
		gtk_entry_set_text(GTK_ENTRY(pd->tsd.bg_color), strarr[2]);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->tsd.bold_radio[0]), (strarr[3][0] != '1' && strarr[3][0] != '2'));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->tsd.bold_radio[1]), (strarr[3][0] == '1'));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->tsd.bold_radio[2]), (strarr[3][0] == '2'));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->tsd.italic_radio[0]), (strarr[4][0] != '1' && strarr[4][0] != '2'));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->tsd.italic_radio[1]), (strarr[4][0] == '1'));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->tsd.italic_radio[2]), (strarr[4][0] == '2'));
		pd->tsd.curstrarr = strarr;
	} else {
		DEBUG_MSG("no selection, returning..\n");
	}
}

static void textstyle_entry_changed(GtkEditable *editable, gpointer user_data) {
	Tprefdialog *pd = (Tprefdialog *)user_data;
	GtkEntry *entry = GTK_ENTRY(editable);
	DEBUG_MSG("textstyle_entry_changed\n");
	if (pd->tsd.curstrarr) {
		int index = 2;
		if (((GtkWidget *)entry) == pd->tsd.fg_color) {
			index = 1;
		}
		if (pd->tsd.curstrarr[index]) g_free(pd->tsd.curstrarr[index]);
		pd->tsd.curstrarr[index] = gtk_editable_get_chars(GTK_EDITABLE(entry),0,-1);
		DEBUG_MSG("textstyle_radio_changed, changing arr[%d] to %s\n",index,pd->tsd.curstrarr[index]);
	}
}

static void textstyle_radio_changed(GtkToggleButton *togglebutton, gpointer user_data) {
	Tprefdialog *pd = (Tprefdialog *)user_data;
	DEBUG_MSG("textstyle_radio_changed\n");
	if (pd->tsd.curstrarr && togglebutton->active) {
		int index = 3;
		gchar *val = "0";
		if ((GtkWidget *)togglebutton == pd->tsd.bold_radio[0]) {
			index = 3;
			val = "0";
		} else if ((GtkWidget *)togglebutton == pd->tsd.bold_radio[1]) {
			index = 3;
			val = "1";
		} else if ((GtkWidget *)togglebutton == pd->tsd.bold_radio[2]) {
			index = 3;
			val = "2";
		} else if ((GtkWidget *)togglebutton == pd->tsd.italic_radio[0]) {
			index = 4;
			val = "0";
		} else if ((GtkWidget *)togglebutton == pd->tsd.italic_radio[1]) {
			index = 4;
			val = "1";
		} else if ((GtkWidget *)togglebutton == pd->tsd.italic_radio[2]) {
			index = 4;
			val = "2";
		}
		if (pd->tsd.curstrarr[index]) g_free(pd->tsd.curstrarr[index]);
		pd->tsd.curstrarr[index] = g_strdup(val);
		DEBUG_MSG("textstyle_radio_changed, changing arr[%d] to %s\n",index,val);
	}
}

/*
the textstyle gui is based on a liststore with 2 columns, first the name, second the gchar **
where this should be updated. We keep track of the current selected name, and set that gchar ** in 
pd->tsd.curstrarr. Whenever any of the radiobuttons or colors is changed, we can immediatly change
it in pd->tsd.curstrarr
if pd->tsd.curstrarr == NULL then there is no name selected, for example we are in the middle of
a switch, delete, add etc.
*/
static void create_textstyle_gui(Tprefdialog *pd, GtkWidget *vbox1) {
	GtkWidget *hbox, *vbox, *hbox2, *but, *scrolwin, *label;
	GtkTreeSelection *select;
	DEBUG_MSG("create_textstyle_gui\n");
	label = gtk_label_new(_("Text styles are applied on top of each other. If multiple styles are applied to the same text, the top-most style in this list has the highest priority. Use drag and drop to re-order the text styles."));
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox1),label, FALSE, TRUE, 2);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, TRUE, TRUE, 2);
	pd->lists[textstyles] = duplicate_arraylist(main_v->props.textstyles);
	pd->tsd.lstore = gtk_list_store_new (2,G_TYPE_STRING,G_TYPE_POINTER);
	pd->tsd.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->tsd.lstore));
	pref_create_column(GTK_TREE_VIEW(pd->tsd.lview), 1, G_CALLBACK(textstyle_0_edited_lcb), pd, _("Label"), 0);
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->tsd.lview));
	g_signal_connect(G_OBJECT(select), "changed",G_CALLBACK(textstyle_selection_changed_cb),pd);
	
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), pd->tsd.lview);
	gtk_widget_set_size_request(scrolwin, 200, 350);
	gtk_box_pack_start(GTK_BOX(hbox), scrolwin, TRUE, TRUE, 2);
	
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);
	pd->tsd.fg_color = prefs_string(_("Foreground color"), "", vbox, pd, string_color);
	pd->tsd.bg_color = prefs_string(_("Background color"), "", vbox, pd, string_color);
	pd->tsd.bold_radio[0] = gtk_radio_button_new_with_label(NULL, _("don't change weight"));
	gtk_box_pack_start(GTK_BOX(vbox),pd->tsd.bold_radio[0], TRUE, TRUE, 0);
	pd->tsd.bold_radio[1] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pd->tsd.bold_radio[0]), _("force non-bold weight"));
	gtk_box_pack_start(GTK_BOX(vbox),pd->tsd.bold_radio[1], TRUE, TRUE, 0);
	pd->tsd.bold_radio[2] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pd->tsd.bold_radio[0]), _("force bold weight"));
	gtk_box_pack_start(GTK_BOX(vbox),pd->tsd.bold_radio[2], TRUE, TRUE, 0);
	pd->tsd.italic_radio[0] = gtk_radio_button_new_with_label(NULL, _("don't change style"));
	gtk_box_pack_start(GTK_BOX(vbox),pd->tsd.italic_radio[0], TRUE, TRUE, 0);
	pd->tsd.italic_radio[1] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pd->tsd.italic_radio[0]), _("force non-italic style"));
	gtk_box_pack_start(GTK_BOX(vbox),pd->tsd.italic_radio[1], TRUE, TRUE, 0);
	pd->tsd.italic_radio[2] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pd->tsd.italic_radio[0]), _("force italic style"));
	gtk_box_pack_start(GTK_BOX(vbox),pd->tsd.italic_radio[2], TRUE, TRUE, 0);
	{
		GList *tmplist = g_list_first(pd->lists[textstyles]);
		while (tmplist) {
			gchar **strarr = (gchar **)tmplist->data;
			if (count_array(strarr)==5) {
				GtkTreeIter iter;
				gtk_list_store_append(GTK_LIST_STORE(pd->tsd.lstore), &iter);
				set_textstyle_strarr_in_list(&iter, strarr,pd);
			}
			tmplist = g_list_next(tmplist);
		}
	}
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(pd->tsd.lview), TRUE);
	pd->tsd.thelist = &pd->lists[textstyles];
	pd->tsd.insertloc = -1;
	g_signal_connect(G_OBJECT(pd->tsd.lstore), "row-inserted", G_CALLBACK(listpref_row_inserted), &pd->tsd);
	g_signal_connect(G_OBJECT(pd->tsd.lstore), "row-deleted", G_CALLBACK(listpref_row_deleted), &pd->tsd);

	g_signal_connect(G_OBJECT(pd->tsd.fg_color),"changed",G_CALLBACK(textstyle_entry_changed),pd);
	g_signal_connect(G_OBJECT(pd->tsd.bg_color),"changed",G_CALLBACK(textstyle_entry_changed),pd);
	g_signal_connect(G_OBJECT(pd->tsd.bold_radio[0]),"toggled",G_CALLBACK(textstyle_radio_changed),pd);
	g_signal_connect(G_OBJECT(pd->tsd.bold_radio[1]),"toggled",G_CALLBACK(textstyle_radio_changed),pd);
	g_signal_connect(G_OBJECT(pd->tsd.bold_radio[2]),"toggled",G_CALLBACK(textstyle_radio_changed),pd);
	g_signal_connect(G_OBJECT(pd->tsd.italic_radio[0]),"toggled",G_CALLBACK(textstyle_radio_changed),pd);
	g_signal_connect(G_OBJECT(pd->tsd.italic_radio[1]),"toggled",G_CALLBACK(textstyle_radio_changed),pd);
	g_signal_connect(G_OBJECT(pd->tsd.italic_radio[2]),"toggled",G_CALLBACK(textstyle_radio_changed),pd);

	hbox2 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox2, TRUE, TRUE, 2);
	but = bf_gtkstock_button(GTK_STOCK_ADD, G_CALLBACK(add_new_textstyle_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox2),but, FALSE, FALSE, 2);
	but = bf_gtkstock_button(GTK_STOCK_DELETE, G_CALLBACK(delete_textstyle_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox2),but, FALSE, FALSE, 2);
}

/************ highlighting, scanner engine ****************/
/*
static void bf_ins_key(gpointer key,gpointer value,gpointer udata) {
	GList **lst = (GList **)udata;
	*lst = g_list_append(*lst,key);
}

static GList *bf_hash_keys_to_list(GHashTable *table) {
	GList *lst=NULL;
	g_hash_table_foreach(table,bf_ins_key,&lst);
	return lst;
}

typedef struct {
	GList **list;
	gchar *grpcrit;
} Thf;

static void bf_ins_token(gpointer key,gpointer value,gpointer udata) {
	Thf *d = (Thf*)udata;
	BfLangToken *t = (BfLangToken*)value;
	if ( d->grpcrit == NULL && t->group==NULL)
	{
		if ( strcmp(t->name,"_tag_end_")!=0 && strcmp(t->name,"_attr_")!=0 && 
				strcmp(t->name,"_attr2_")!=0 && strcmp(t->name,"_attr_tag_begin_end_")!=0 &&
				strcmp(t->name,"_fake_ident_")!=0)
		*(d->list) = g_list_append(*(d->list),t->name);
	}	
	else
		if ( d->grpcrit!=NULL && t->group!=NULL && strcmp(t->group,d->grpcrit) == 0 )	
		{
		if ( strcmp(t->name,"_tag_end_")!=0 && strcmp(t->name,"_attr_")!=0 && 
				strcmp(t->name,"_attr2_")!=0 && strcmp(t->name,"_attr_tag_begin_end_")!=0 &&
				strcmp(t->name,"_fake_ident_")!=0)
	  			  *(d->list) = g_list_append(*(d->list),t->name);
		}	
}

static void bf_ins_block(gpointer key,gpointer value,gpointer udata) {
	Thf *d = (Thf*)udata;
	BfLangBlock *t = (BfLangBlock*)value;
	if ( d->grpcrit == NULL  && t->group==NULL) {
		if ( strcmp(t->id,"_tag_begin_")!=0 )
			*(d->list) = g_list_append(*(d->list),t->id);
	}	
	else
		if ( d->grpcrit!=NULL && t->group!=NULL && strcmp(t->group,d->grpcrit) == 0 )	 {
			if ( strcmp(t->id,"_tag_begin_")!=0 )		
				*(d->list) = g_list_append(*(d->list),t->id);
		}	
}

static void  hlg_toggled  (GtkToggleButton *togglebutton, gpointer user_data) {
	Tprefdialog *pd = (Tprefdialog *)user_data;
	BfLangConfig *cfg = bf_lang_mgr_get_config(main_v->lang_mgr,gtk_combo_box_get_active_text(GTK_COMBO_BOX(pd->hld.ftype_combo)));

	if ( !cfg ) return;
	if ( !gtk_toggle_button_get_active(togglebutton)) return;
	pd->hld.filetype_change = TRUE;		
	if ( GTK_WIDGET(togglebutton) == pd->hld.blk_radio ) {
		gtk_tree_view_set_model(GTK_TREE_VIEW(pd->hld.tree),GTK_TREE_MODEL(pd->hld.blk_store));
	}
	else if ( GTK_WIDGET(togglebutton) == pd->hld.tk_radio ) {
		gtk_tree_view_set_model(GTK_TREE_VIEW(pd->hld.tree),GTK_TREE_MODEL(pd->hld.tk_store));
	}
	else if ( GTK_WIDGET(togglebutton) == pd->hld.tag_radio ) {
		gtk_tree_view_set_model(GTK_TREE_VIEW(pd->hld.tree),GTK_TREE_MODEL(pd->hld.tag_store));
	}
	pd->hld.filetype_change = FALSE;		
}

static gchar **hlg_find_pattern(Tprefdialog *pd, gchar *filetype, gchar *type, gchar *name) {
	GList *lst = g_list_first(pd->lists[highlight_patterns]);
	while ( lst ) {
		gchar **arr = lst->data;
		if ( strcmp(arr[0],filetype)==0  && strcmp(arr[1],type)==0 && strcmp(arr[2],name)==0 ) {
			return arr;
		}	
		lst = g_list_next(lst);
	}
	return NULL;
}*/

/* called when a filetype is set in the syntax highlighting dialog */
/*static void hlg_ftype_changed  (GtkComboBox *widget, gpointer user_data) {
	BfLangConfig *cfg = bf_lang_mgr_get_config(main_v->lang_mgr,gtk_combo_box_get_active_text(widget));
	Tprefdialog *pd = (Tprefdialog *)user_data;
	GList *lst=NULL,*lst2,*lst3;
	GtkTreeIter iter,iter2;
	Thf *hf = g_new0(Thf,1);
	
	if (!cfg) {
		gtk_widget_hide(pd->hld.main_view);
		return;
	}	 
	pd->hld.filetype_change = TRUE;
	gtk_widget_show(pd->hld.main_view);
	/ * fill all stores * /
	lst = bf_hash_keys_to_list(cfg->groups);
	gtk_tree_store_clear(pd->hld.blk_store);
	gtk_tree_store_clear(pd->hld.tk_store);
	while ( lst ) {
		lst3 = NULL;
		hf->list = &lst3;
		hf->grpcrit = lst->data;
		g_hash_table_foreach(cfg->blocks,bf_ins_block,hf);
		lst2 = g_list_first(lst3);
		if ( lst2 ) {
			gtk_tree_store_append(pd->hld.blk_store, &iter, NULL);
			gtk_tree_store_set(pd->hld.blk_store, &iter, 0,0,1,lst->data, 
			      2, hlg_find_pattern(pd,gtk_combo_box_get_active_text(widget),"g",lst->data), -1);		
			while (lst2) {
				gtk_tree_store_append(pd->hld.blk_store, &iter2, &iter);
				gtk_tree_store_set(pd->hld.blk_store, &iter2, 0,1,1,lst2->data,
					2, hlg_find_pattern(pd,gtk_combo_box_get_active_text(widget),"b",lst2->data), -1);			
				lst2 = g_list_next(lst2);
			}		
			g_list_free(lst3);		
		}
		lst3 = NULL;
		hf->list = &lst3;
		hf->grpcrit = lst->data;
		g_hash_table_foreach(cfg->tokens,bf_ins_token,hf);
		lst2 = g_list_first(lst3);
		if ( lst2 ) {
			gtk_tree_store_append(pd->hld.tk_store, &iter, NULL);
			gtk_tree_store_set(pd->hld.tk_store, &iter, 0,0,1,lst->data,
					2, hlg_find_pattern(pd,gtk_combo_box_get_active_text(widget),"g",lst->data),-1);		
			while (lst2) {
				gtk_tree_store_append(pd->hld.tk_store, &iter2, &iter);
				gtk_tree_store_set(pd->hld.tk_store, &iter2, 0,2,1,lst2->data,
					2, hlg_find_pattern(pd,gtk_combo_box_get_active_text(widget),"t",lst2->data), -1);			
				lst2 = g_list_next(lst2);
			}
			g_list_free(lst3);
		}
		lst = g_list_next(lst);
	}
	lst3 = NULL;
	hf->list = &lst3;
	hf->grpcrit = NULL;
	g_hash_table_foreach(cfg->blocks,bf_ins_block,hf);
	lst2 = g_list_first(lst3);
	while (lst2) {
		gtk_tree_store_append(pd->hld.blk_store, &iter2, NULL);
		gtk_tree_store_set(pd->hld.blk_store, &iter2, 0,1,1,lst2->data,
					2, hlg_find_pattern(pd,gtk_combo_box_get_active_text(widget),"b",lst2->data), -1);			
		lst2 = g_list_next(lst2);
	}
	g_list_free(lst3);		
	lst3 = NULL;
	hf->list = &lst3;
	hf->grpcrit = NULL;
	g_hash_table_foreach(cfg->tokens,bf_ins_token,hf);
	lst2 = g_list_first(lst3);
	while (lst2) {
		gtk_tree_store_append(pd->hld.tk_store, &iter2, NULL);
		gtk_tree_store_set(pd->hld.tk_store, &iter2, 0,2,1,lst2->data,
					2, hlg_find_pattern(pd,gtk_combo_box_get_active_text(widget),"t",lst2->data), -1);			
		lst2 = g_list_next(lst2);
	}		
	g_list_free(lst3);		

	
	g_free(hf);
	if ( !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pd->hld.blk_radio)) ) 
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hld.blk_radio),TRUE);	
	else
		hlg_toggled(GTK_TOGGLE_BUTTON(pd->hld.blk_radio),pd);
	if ( cfg->scan_tags ) {
		gtk_widget_set_sensitive(pd->hld.tag_radio,TRUE);	
		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pd->hld.tag_store),&iter);
		gtk_tree_store_set(pd->hld.tag_store,&iter,2, hlg_find_pattern(pd,gtk_combo_box_get_active_text(widget),"m","tag_begin"), -1);
		gtk_tree_model_iter_next(GTK_TREE_MODEL(pd->hld.tag_store),&iter);
		gtk_tree_store_set(pd->hld.tag_store,&iter,2, hlg_find_pattern(pd,gtk_combo_box_get_active_text(widget),"m","tag_end"), -1);
		gtk_tree_model_iter_next(GTK_TREE_MODEL(pd->hld.tag_store),&iter);
		gtk_tree_store_set(pd->hld.tag_store,&iter,2, hlg_find_pattern(pd,gtk_combo_box_get_active_text(widget),"m","attr_name"), -1);
		gtk_tree_model_iter_next(GTK_TREE_MODEL(pd->hld.tag_store),&iter);
		gtk_tree_store_set(pd->hld.tag_store,&iter,2, hlg_find_pattern(pd,gtk_combo_box_get_active_text(widget),"m","attr_val"), -1);		
	}	
	else
		gtk_widget_set_sensitive(pd->hld.tag_radio,FALSE);		
	pd->hld.filetype_change = FALSE;		
}*/
/*
static void hlg_apply_style(Tprefdialog *pd) {
	GtkTreeIter iter;
	GtkTreeSelection *select;
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->hld.tree));
	if ( !select ) return;
	if (gtk_tree_selection_get_selected (select,NULL,&iter)) {
		gchar **strarr=NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(pd->hld.tree))), &iter, 2, &strarr, -1);
		if ( strarr ) {
			gtk_entry_set_text(GTK_ENTRY(pd->hld.fg_color),strarr[3]);
			gtk_entry_set_text(GTK_ENTRY(pd->hld.bg_color),strarr[4]);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hld.bold_radio[atoi(strarr[5])]),TRUE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hld.italic_radio[atoi(strarr[6])]),TRUE);
		} else {
			gtk_entry_set_text(GTK_ENTRY(pd->hld.fg_color),"");
			gtk_entry_set_text(GTK_ENTRY(pd->hld.bg_color),"");		
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hld.bold_radio[0]),TRUE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hld.italic_radio[0]),TRUE);			
		}
	}
}*/
/*
static void hlg_cursor_changed  (GtkTreeView *treeview, gpointer user_data) {
	Tprefdialog *pd = (Tprefdialog *)user_data;
	pd->hld.filetype_change = TRUE;		
	hlg_apply_style(pd);
	pd->hld.filetype_change = FALSE;		
}*/

/* static void hlg_apply_changes_to_item(Tprefdialog *pd) {
	gchar **strarr=NULL;
	GtkTreeIter iter;
	GtkTreeSelection *select;
	gint tp;
	gchar *pomstr;

	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->hld.tree));
	if ( !select ) return;
	if (gtk_tree_selection_get_selected (select,NULL,&iter)) {
		gtk_tree_model_get(GTK_TREE_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(pd->hld.tree))), &iter, 2, &strarr, -1);	
		if ( strarr ) { / * already have array * /
			strarr[3] = gtk_editable_get_chars(GTK_EDITABLE(pd->hld.fg_color),0,-1);
			strarr[4] = gtk_editable_get_chars(GTK_EDITABLE(pd->hld.bg_color),0,-1);
			g_free(strarr[5]);
			if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pd->hld.bold_radio[1])) )
				strarr[5] = g_strdup("1");
			else if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pd->hld.bold_radio[2])) )	
				strarr[5] = g_strdup("2");
			else
				strarr[5] = g_strdup("0");	
			g_free(strarr[6]);	
			if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pd->hld.italic_radio[1])) )
				strarr[6] = g_strdup("1");
			else if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pd->hld.italic_radio[2])) )	
				strarr[6] = g_strdup("2");				
			else
				strarr[6] = g_strdup("0");	
		} else { / * create new array * /
			strarr = g_malloc(8*sizeof(gchar *));
			strarr[7] = NULL;
			strarr[0] = g_strdup(gtk_combo_box_get_active_text(GTK_COMBO_BOX(pd->hld.ftype_combo)));				
			gtk_tree_model_get(GTK_TREE_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(pd->hld.tree))), &iter, 0, &tp, -1);	
			switch ( tp ) {
				case 0: / * group * /
					strarr[1] = g_strdup("g");break;
				case 1: / * block  * /
					strarr[1] = g_strdup("b");break;
				case 2: / * token * /
					strarr[1] = g_strdup("t");break;					
				case 3: / * markup * /
				case 4: 
				case 5:
				case 6:
					strarr[1] = g_strdup("m");break;										
			}
			if ( tp < 3 ) {
				gtk_tree_model_get(GTK_TREE_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(pd->hld.tree))), &iter, 1, &pomstr, -1);
				strarr[2] = g_strdup(pomstr);
			} else {
				switch ( tp ) {
					case 3: strarr[2] = g_strdup("tag_begin");break;
					case 4: strarr[2] = g_strdup("tag_end");break;
					case 5: strarr[2] = g_strdup("attr_name");break;
					case 6: strarr[2] = g_strdup("attr_val");break;
				}
			}	
			strarr[3] = gtk_editable_get_chars(GTK_EDITABLE(pd->hld.fg_color),0,-1);
			strarr[4] = gtk_editable_get_chars(GTK_EDITABLE(pd->hld.bg_color),0,-1);
			if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pd->hld.bold_radio[1])) )
				strarr[5] = g_strdup("1");
			else if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pd->hld.bold_radio[2])) )	
				strarr[5] = g_strdup("2");
			else
				strarr[5] = g_strdup("0");	
			if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pd->hld.italic_radio[1])) )
				strarr[6] = g_strdup("1");
			else if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pd->hld.italic_radio[2])) )	
				strarr[6] = g_strdup("2");				
			else
				strarr[6] = g_strdup("0");				
			gtk_tree_store_set(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(pd->hld.tree))), &iter,
			   2, strarr,-1);	
			pd->lists[highlight_patterns] = g_list_append(pd->lists[highlight_patterns],strarr);   
		} / * create new array * /
	}	/ * get_selected * /
}
*/

/* 
the new GUI will look like this (only 1st column is visible, 2, 3 and 4 are not rendered):

-|- PHP                    "PHP"        NULL,   NULL
 |  |- php-group           "PHP",       "g"     NULL, or some gchar **
 |  |  |- comment          "PHP",       "b"     NULL, or some gchar **
 |  |  |- braces             ... .... 
 |  |- markup group             ... .... 
 |  |- general             ... .... 
 |  |- keywords             ... .... 
 |  |- tag begin            "PHP",       "begin_tag"    NULL, or some gchar **
 |  |- tag end             ... .... 
 |
 |-Python
 |  |

in the liststore, we will have three columns: 
	0: the visible name
	1: the filetype
	2: the type as stored in the config file
	3: the gchar ** that should be updated if the option is changed

*/
static void fill_hl_combo(Tprefdialog *pd) {
	GList *tmplist;
	GtkTreeIter iter;
	gtk_list_store_append(GTK_LIST_STORE(pd->hld.cstore), &iter);
	gtk_list_store_set(GTK_LIST_STORE(pd->hld.cstore), &iter,0,"",-1);
	for (tmplist = g_list_first(pd->lists[textstyles]);tmplist;tmplist = g_list_next(tmplist)) {
		gchar **arr = tmplist->data;
		DEBUG_MSG("fill_hl_combo, adding %s\n",arr[0]);
		gtk_list_store_append(GTK_LIST_STORE(pd->hld.cstore), &iter);
		gtk_list_store_set(GTK_LIST_STORE(pd->hld.cstore), &iter,0,arr[0],-1);
	}
}

static void retrieve_arr_add_to_model(Tprefdialog *pd, GtkTreeIter *parent, GtkTreeIter *iiter, const gchar *filetype, const gchar *type, const gchar *name) {
	const gchar *arr2[] = {filetype, type, name, NULL};
	gchar **iarr =  arraylist_value_exists(pd->lists[syntax_styles], arr2, 3, TRUE);
	DEBUG_MSG("retrieve_arr_add_to_model, adding %s to model\n",name);
	gtk_tree_store_append(GTK_TREE_STORE(pd->hld.tstore), iiter, parent);
	gtk_tree_store_set(GTK_TREE_STORE(pd->hld.tstore), iiter,0,name,1,filetype,2,type,3,iarr,-1);
}

static void fill_hl_tree(Tprefdialog *pd) {
	GList *tmplist;
	tmplist = g_list_first(main_v->lang_mgr->languages);
	while (tmplist) {
		BfLangConfig *cfg = (BfLangConfig *)tmplist->data;
		GtkTreeIter ftiter;
		if (cfg) {
			GtkTreeIter giter, iiter;
			GList *grouplist, *tmplist, *ilist, *tmplist2;
			/* add the parent entry */
			gtk_tree_store_append(pd->hld.tstore, &ftiter, NULL);
			gtk_tree_store_set(pd->hld.tstore, &ftiter
					,0,cfg->name,1,cfg->name,2, NULL,3,NULL, -1);

			/* add blocks/tokens/tags to the tree, the user doesn't need to know if something is a block, a token or 
			a tag, so we insert their groups in the same level for the user */
			grouplist = bf_lang_get_groups(cfg);
			for (tmplist = g_list_first(grouplist);tmplist;tmplist = g_list_next(tmplist)) {
				retrieve_arr_add_to_model(pd, &ftiter, &giter, cfg->name, "g", tmplist->data);
				/* get blocks for this group and add them */
				ilist = bf_lang_get_blocks_for_group(cfg, tmplist->data);
				for (tmplist2 = g_list_first(ilist);tmplist2;tmplist2 = g_list_next(tmplist2)) {
					retrieve_arr_add_to_model(pd, &giter, &iiter, cfg->name, "b", tmplist2->data);
				}
				g_list_free(ilist);
				/* get tokens for this group and add them */
				ilist = bf_lang_get_tokens_for_group(cfg, tmplist->data);
				for (tmplist2 = g_list_first(ilist);tmplist2;tmplist2 = g_list_next(tmplist2)) {
					retrieve_arr_add_to_model(pd, &giter, &iiter, cfg->name, "t", tmplist2->data);
				}
				g_list_free(ilist);
			}
			g_list_free(grouplist);
			/* now add blocks/tokens that do not have a group */
			ilist = bf_lang_get_blocks_for_group(cfg, NULL);
			for (tmplist2 = g_list_first(ilist);tmplist2;tmplist2 = g_list_next(tmplist2)) {
				retrieve_arr_add_to_model(pd, &ftiter, &iiter, cfg->name, "b", tmplist2->data);
			}
			g_list_free(ilist);
			ilist = bf_lang_get_tokens_for_group(cfg, NULL);
			for (tmplist2 = g_list_first(ilist);tmplist2;tmplist2 = g_list_next(tmplist2)) {
				retrieve_arr_add_to_model(pd, &ftiter, &iiter, cfg->name, "t", tmplist2->data);
			}
			g_list_free(ilist);
			/* add tags if required */
			if (bf_lang_needs_tags(cfg)) {
				retrieve_arr_add_to_model(pd, &ftiter, &giter, cfg->name, "m", "tag_begin");
				retrieve_arr_add_to_model(pd, &ftiter, &giter, cfg->name, "m", "tag_end");
				retrieve_arr_add_to_model(pd, &ftiter, &giter, cfg->name, "m", "attr_name");
				retrieve_arr_add_to_model(pd, &ftiter, &giter, cfg->name, "m", "attr_val");
			}
		}
		tmplist = g_list_next(tmplist);
	}  
}

static void hl_set_textstylecombo_by_text(Tprefdialog *pd, const gchar *text) {
	DEBUG_MSG("hl_set_textstylecombo_by_text, text=%s\n",text);
	if (text == NULL) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(pd->hld.textstyle),-1);
	} else {
		GtkTreeIter iter;
		gboolean cont = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pd->hld.cstore),&iter);
		while (cont) {
			gchar *name;
			gtk_tree_model_get(GTK_TREE_MODEL(pd->hld.cstore),&iter,0,&name,-1);
			DEBUG_MSG("hl_set_textstylecombo_by_text, compare %s and %s\n",text,name);
			if (strcmp(name,text)==0) {
				gtk_combo_box_set_active_iter(GTK_COMBO_BOX(pd->hld.textstyle),&iter);
				g_free(name);
				return;
			}
			g_free(name);
			cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(pd->hld.cstore),&iter);
		}
		gtk_combo_box_set_active(GTK_COMBO_BOX(pd->hld.textstyle),-1);
	}
}

static void hl_textstylecombo_changed(GtkComboBox *widget,Tprefdialog *pd) {
	GtkTreeIter iter;
	if (!pd->hld.curstrarr) return;
	DEBUG_MSG("hl_textstylecombo_changed, curstrarr=%p\n",pd->hld.curstrarr);
	if (gtk_combo_box_get_active_iter(widget,&iter)) {
		gchar *name;
		gtk_tree_model_get(GTK_TREE_MODEL(pd->hld.cstore),&iter,0,&name,-1);
		DEBUG_MSG("hl_textstylecombo_changed, found name %s\n",name);
		if (pd->hld.curstrarr[3]) {
			DEBUG_MSG("free old value %s\n",pd->hld.curstrarr[3]);
			g_free(pd->hld.curstrarr[3]);
		}
		pd->hld.curstrarr[3] = name;
	}
}

static void hl_selection_changed_cb(GtkTreeSelection *selection, Tprefdialog *pd) {
	GtkTreeIter iter;
	GtkTreeModel *model;
	DEBUG_MSG("hl_selection_changed_cb, started\n");
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gchar *name, *filetype, *type, **strarr;
		gtk_tree_model_get(model, &iter, 0, &name, 1, &filetype, 2, &type, 3, &strarr, -1);
		pd->hld.curstrarr = NULL;
		if (type) {
			if (strarr == NULL) {
				/* create the strarr if there is none yet */
				strarr = g_malloc((5)*sizeof(gchar *));
				strarr[0] = filetype;
				strarr[1] = type;
				strarr[2] = name;
				strarr[3] = g_strdup("");
				strarr[4] = NULL;
				DEBUG_MSG("hl_selection_changed_cb, created %p %s:%s:%s:%s\n",strarr, strarr[0],strarr[1],strarr[2],strarr[3]);
				gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 3, strarr, -1);
				hl_set_textstylecombo_by_text(pd, NULL);
				pd->lists[syntax_styles] = g_list_prepend(pd->lists[syntax_styles], strarr);
				pd->hld.curstrarr = strarr;
			} else {
				DEBUG_MSG("textstyle_selection_changed_cb, existing strarr=%p has strarr[3]=%s\n",strarr,strarr[3]);
				hl_set_textstylecombo_by_text(pd, strarr[3]);
				pd->hld.curstrarr = strarr;
			}
		}
	} else {
		DEBUG_MSG("no selection, returning..\n");
	}
}

static void create_hl_gui(Tprefdialog *pd, GtkWidget *mainbox) {
	GtkWidget *hbox, *vbox, *scrolledwindow1;

	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *select;

	DEBUG_MSG("create_hl_gui, duplicate arraylist \n");
	pd->lists[syntax_styles] = duplicate_arraylist(main_v->props.syntax_styles);
	/* new structure: one treestore for all, column 1:visible label, 2:label for config file 3:pointer to strarr */
	pd->hld.tstore = gtk_tree_store_new(4,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER);
	pd->hld.cstore = gtk_list_store_new(1,G_TYPE_STRING);

	hbox = gtk_hbox_new(TRUE,0);
	gtk_box_pack_start(GTK_BOX(mainbox), hbox, TRUE, TRUE, 2);
	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start(GTK_BOX(hbox), scrolledwindow1, TRUE, TRUE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow1), 2);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	/* fill the tree */	
	fill_hl_tree(pd);
	/* fill the combo box model */
	fill_hl_combo(pd);
	
	/* create the view component */
	pd->hld.tview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->hld.tstore));
	gtk_container_add (GTK_CONTAINER(scrolledwindow1), pd->hld.tview);
	gtk_container_set_border_width(GTK_CONTAINER (pd->hld.tview), 2);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW (pd->hld.tview), FALSE);
	gtk_widget_set_size_request(pd->hld.tview, 150, 500);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (_("Name"), renderer,"text", 0,NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(pd->hld.tview), column);
	
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->hld.tview));
	g_signal_connect(G_OBJECT(select), "changed",G_CALLBACK(hl_selection_changed_cb),pd);
	vbox = gtk_vbox_new(TRUE,0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 2);

	pd->hld.textstyle = gtk_combo_box_new_with_model(GTK_TREE_MODEL(pd->hld.cstore));
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(pd->hld.textstyle),renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(pd->hld.textstyle), renderer, "text", 0, NULL);
	g_signal_connect(G_OBJECT(pd->hld.textstyle), "changed",G_CALLBACK(hl_textstylecombo_changed),pd);
	
	
	gtk_box_pack_start(GTK_BOX(vbox), pd->hld.textstyle, TRUE, FALSE, 2);
	
/*	reset_btn = gtk_button_new_with_mnemonic (_("Reset"));
	gtk_box_pack_start(GTK_BOX (hbox1), reset_btn, FALSE, FALSE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (reset_btn), 2);
	
	vbox2 = gtk_vbox_new (FALSE, 3);
	gtk_box_pack_start (GTK_BOX (vbox1), vbox2, TRUE, TRUE, 0);
	pd->hld.main_view = vbox2;
	
	hbox2 = gtk_hbox_new (FALSE, 3);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 3);
	
	pd->hld.blk_radio = gtk_radio_button_new_with_mnemonic (NULL, _("Blocks"));
	gtk_box_pack_start (GTK_BOX (hbox2), pd->hld.blk_radio, FALSE, FALSE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (pd->hld.blk_radio), 2);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (pd->hld.blk_radio), blk_radio_group);
	blk_radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (pd->hld.blk_radio));
	
	pd->hld.tk_radio = gtk_radio_button_new_with_mnemonic (NULL, _("Tokens"));
	gtk_box_pack_start (GTK_BOX (hbox2), pd->hld.tk_radio, FALSE, FALSE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (pd->hld.tk_radio), 2);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (pd->hld.tk_radio), blk_radio_group);
	blk_radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (pd->hld.tk_radio));
	
	pd->hld.tag_radio = gtk_radio_button_new_with_mnemonic (NULL, _("Tags"));
	gtk_box_pack_start (GTK_BOX (hbox2), pd->hld.tag_radio, FALSE, FALSE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (pd->hld.tag_radio), 2);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (pd->hld.tag_radio), blk_radio_group);
	blk_radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (pd->hld.tag_radio));
	
	hbox3 = gtk_hbox_new (FALSE, 3);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox3, TRUE, TRUE, 0);*/
	
/*	t_pbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox3), t_pbox, TRUE, TRUE, 5);
	pd->hld.fg_color = prefs_string(_("Foreground color"), "", t_pbox, pd, string_color);
	pd->hld.bg_color = prefs_string(_("Background color"), "", t_pbox, pd, string_color);
	pd->hld.bold_radio[0] = gtk_radio_button_new_with_label(NULL, _("don't change weight"));
	gtk_box_pack_start(GTK_BOX(t_pbox),pd->hld.bold_radio[0], TRUE, TRUE, 0);
	pd->hld.bold_radio[1] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pd->hld.bold_radio[0]), _("force non-bold weight"));
	gtk_box_pack_start(GTK_BOX(t_pbox),pd->hld.bold_radio[1], TRUE, TRUE, 0);
	pd->hld.bold_radio[2] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pd->hld.bold_radio[0]), _("force bold weight"));
	gtk_box_pack_start(GTK_BOX(t_pbox),pd->hld.bold_radio[2], TRUE, TRUE, 0);
	pd->hld.italic_radio[0] = gtk_radio_button_new_with_label(NULL, _("don't change style"));
	gtk_box_pack_start(GTK_BOX(t_pbox),pd->hld.italic_radio[0], TRUE, TRUE, 0);
	pd->hld.italic_radio[1] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pd->hld.italic_radio[0]), _("force non-italic style"));
	gtk_box_pack_start(GTK_BOX(t_pbox),pd->hld.italic_radio[1], TRUE, TRUE, 0);
	pd->hld.italic_radio[2] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pd->hld.italic_radio[0]), _("force italic style"));
	gtk_box_pack_start(GTK_BOX(t_pbox),pd->hld.italic_radio[2], TRUE, TRUE, 0);*/
	
/*	g_signal_connect(G_OBJECT(pd->hld.ftype_combo),"changed",G_CALLBACK(hlg_ftype_changed),pd);
	g_signal_connect(G_OBJECT(pd->hld.blk_radio),"toggled",G_CALLBACK(hlg_toggled),pd);
	g_signal_connect(G_OBJECT(pd->hld.tk_radio),"toggled",G_CALLBACK(hlg_toggled),pd);
	g_signal_connect(G_OBJECT(pd->hld.tag_radio),"toggled",G_CALLBACK(hlg_toggled),pd);
	g_signal_connect(G_OBJECT(pd->hld.tree),"cursor-changed",G_CALLBACK(hlg_cursor_changed),pd);*/
/*	pd->hld.filetype_change = FALSE;*/;
}

/************ external commands code ****************/

static void set_extcommands_strarr_in_list(GtkTreeIter *iter, gchar **strarr, Tprefdialog *pd) {
	gint arrcount = count_array(strarr);
	if (arrcount==3) {
		gtk_list_store_set(GTK_LIST_STORE(pd->bd.lstore), iter
				,0,strarr[0],1,strarr[1],2,(strarr[2][0]=='1'),3,strarr,-1);
	} else {
		DEBUG_MSG("ERROR: set_extcommands_strarr_in_list, arraycount %d != 3 !!!!!!\n",arrcount);
	}
}
static void extcommands_0_edited_lcb(GtkCellRendererText *cellrenderertext,gchar *path,gchar *newtext,Tprefdialog *pd) {
	pref_apply_change(pd->bd.lstore,3,1,path,newtext,0);
}
static void extcommands_1_edited_lcb(GtkCellRendererText *cellrenderertext,gchar *path,gchar *newtext,Tprefdialog *pd) {
	pref_apply_change(pd->bd.lstore,3,1,path,newtext,1);
}
static void extcommands_2_edited_lcb(GtkCellRendererToggle *cellrenderertoggle,gchar *path,Tprefdialog *pd) {
	pref_apply_change(pd->bd.lstore,3,2,path,cellrenderertoggle->active ? "0" : "1",2);
}
static void add_new_extcommands_lcb(GtkWidget *wid, Tprefdialog *pd) {
	gchar **strarr;
	GtkTreeIter iter;
	strarr = pref_create_empty_strarr(3);
	gtk_list_store_append(GTK_LIST_STORE(pd->bd.lstore), &iter);
	set_extcommands_strarr_in_list(&iter, strarr,pd);
	pd->lists[extcommands] = g_list_append(pd->lists[extcommands], strarr);
	pd->bd.insertloc = -1;
}
static void delete_extcommands_lcb(GtkWidget *wid, Tprefdialog *pd) {
	pref_delete_strarr(pd, &pd->bd, 3);
}
static void create_extcommands_gui(Tprefdialog *pd, GtkWidget *vbox1) {
	GtkWidget *hbox, *but, *scrolwin, *label;
	pd->lists[extcommands] = duplicate_arraylist(main_v->props.external_command);
	pd->bd.lstore = gtk_list_store_new (4,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_BOOLEAN,G_TYPE_POINTER);
	pd->bd.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->bd.lstore));
	pref_create_column(GTK_TREE_VIEW(pd->bd.lview), 1, G_CALLBACK(extcommands_0_edited_lcb), pd, _("Label"), 0);
	pref_create_column(GTK_TREE_VIEW(pd->bd.lview), 1, G_CALLBACK(extcommands_1_edited_lcb), pd, _("Command"), 1);
	pref_create_column(GTK_TREE_VIEW(pd->bd.lview), 2, G_CALLBACK(extcommands_2_edited_lcb), pd, _("Default browser"), 2);
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<small><b>Input options</b>\nspecify no options to read the input from the standard output\n%s local filename (available for local files)\n%i temporary fifo for input, equals %s if the document is not modified and local\n%I temporary filename for input, equals %s if the document is not modified and local\n<b>Other options</b>\n%c local directory of file (available for local files)\n%n filename without path (available for all titled files)\n%u URL (available for all titled files)\n%p preview URL if basedir and preview dir are set in project settings, else identical to %u</small>"));
	gtk_box_pack_start(GTK_BOX(vbox1),label, TRUE, TRUE, 2);
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), pd->bd.lview);
	gtk_widget_set_size_request(scrolwin, 200, 210);
	gtk_box_pack_start(GTK_BOX(vbox1), scrolwin, TRUE, TRUE, 2);
	{
		GList *tmplist = g_list_first(pd->lists[extcommands]);
		while (tmplist) {
			gchar **strarr = (gchar **)tmplist->data;
			GtkTreeIter iter;
			DEBUG_MSG("create_extcommands_gui");
			gtk_list_store_append(GTK_LIST_STORE(pd->bd.lstore), &iter);
			set_extcommands_strarr_in_list(&iter, strarr,pd);
			tmplist = g_list_next(tmplist);
		}
	}
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(pd->bd.lview), TRUE);
	pd->bd.thelist = &pd->lists[extcommands];
	pd->bd.insertloc = -1;
	g_signal_connect(G_OBJECT(pd->bd.lstore), "row-inserted", G_CALLBACK(listpref_row_inserted), &pd->bd);
	g_signal_connect(G_OBJECT(pd->bd.lstore), "row-deleted", G_CALLBACK(listpref_row_deleted), &pd->bd);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox, TRUE, TRUE, 2);
	but = bf_gtkstock_button(GTK_STOCK_ADD, G_CALLBACK(add_new_extcommands_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox),but, FALSE, FALSE, 2);
	but = bf_gtkstock_button(GTK_STOCK_DELETE, G_CALLBACK(delete_extcommands_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox),but, FALSE, FALSE, 2);	
}

/************ external filters?? code ****************/

static void set_external_filters_strarr_in_list(GtkTreeIter *iter, gchar **strarr, Tprefdialog *pd) {
	gint arrcount = count_array(strarr);
	if (arrcount==2) {
		gtk_list_store_set(GTK_LIST_STORE(pd->ed.lstore), iter
				,0,strarr[0],1,strarr[1],2,strarr,-1);
	} else {
		DEBUG_MSG("ERROR: set_external_command_strarr_in_list, arraycount != 2 !!!!!!\n");
	}
}
static void external_filter_0_edited_lcb(GtkCellRendererText *cellrenderertext,gchar *path,gchar *newtext,Tprefdialog *pd) {
	pref_apply_change(pd->ed.lstore,2,1,path,newtext,0);
}
static void external_filter_1_edited_lcb(GtkCellRendererText *cellrenderertext,gchar *path,gchar *newtext,Tprefdialog *pd) {
	pref_apply_change(pd->ed.lstore,2,1,path,newtext,1);
}
static void add_new_external_filter_lcb(GtkWidget *wid, Tprefdialog *pd) {
	gchar **strarr;
	GtkTreeIter iter;
	strarr = pref_create_empty_strarr(2);
	gtk_list_store_append(GTK_LIST_STORE(pd->ed.lstore), &iter);
	set_external_filters_strarr_in_list(&iter, strarr,pd);
	pd->lists[extfilters] = g_list_append(pd->lists[extfilters], strarr);
	pd->ed.insertloc = -1;
}
static void delete_external_filter_lcb(GtkWidget *wid, Tprefdialog *pd) {
	pref_delete_strarr(pd, &pd->ed, 2);
}
static void create_filters_gui(Tprefdialog *pd, GtkWidget *vbox1) {
	GtkWidget *hbox, *but, *scrolwin, *label;
	pd->lists[extfilters] = duplicate_arraylist(main_v->props.external_filter);
	pd->ed.lstore = gtk_list_store_new (3,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER);
	pd->ed.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->ed.lstore));
	pref_create_column(GTK_TREE_VIEW(pd->ed.lview), 1, G_CALLBACK(external_filter_0_edited_lcb), pd, _("Label"), 0);
	pref_create_column(GTK_TREE_VIEW(pd->ed.lview), 1, G_CALLBACK(external_filter_1_edited_lcb), pd, _("Command"), 1);
	
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<small><b>Input options</b>\nspecify no options to read the input from the standard output\n%s local filename (requires local file, cannot operate on selection)\n%i temporary fifo for input\n%I temporary filename for input\n<b>Output options</b>\nspecify no options to read the output from the standard output\n%o temporary fifo\n%O temporary filename\n%t temporary filename for both input and output (for in-place-editing filters, cannot operate on selection)\n<b>Other options</b>\n%c local directory of file (requires local file)\n%n filename without path (available for all titled files)\n%u URL (available for all titled files)\n%p preview URL if basedir and preview dir are set in project settings, else identical to %u</small>"));
	gtk_box_pack_start(GTK_BOX(vbox1),label, TRUE, TRUE, 2);
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), pd->ed.lview);
	gtk_widget_set_size_request(scrolwin, 200, 190);
	gtk_box_pack_start(GTK_BOX(vbox1), scrolwin, TRUE, TRUE, 2);
	{
		GList *tmplist = g_list_first(pd->lists[extfilters]);
		while (tmplist) {
			gchar **strarr = (gchar **)tmplist->data;
			GtkTreeIter iter;
			gtk_list_store_append(GTK_LIST_STORE(pd->ed.lstore), &iter);
			set_external_filters_strarr_in_list(&iter, strarr,pd);
			tmplist = g_list_next(tmplist);
		}
	}
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(pd->ed.lview), TRUE);
	pd->ed.thelist = &pd->lists[extfilters];
	pd->ed.insertloc = -1;
	g_signal_connect(G_OBJECT(pd->ed.lstore), "row-inserted", G_CALLBACK(listpref_row_inserted), &pd->ed);
	g_signal_connect(G_OBJECT(pd->ed.lstore), "row-deleted", G_CALLBACK(listpref_row_deleted), &pd->ed);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox, TRUE, TRUE, 2);
	but = bf_gtkstock_button(GTK_STOCK_ADD, G_CALLBACK(add_new_external_filter_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox),but, FALSE, FALSE, 2);
	but = bf_gtkstock_button(GTK_STOCK_DELETE, G_CALLBACK(delete_external_filter_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox),but, FALSE, FALSE, 2);	
}

/************ outputbox code ****************/

static void set_outputbox_strarr_in_list(GtkTreeIter *iter, gchar **strarr, Tprefdialog *pd) {
	gint arrcount;
	arrcount = count_array(strarr);
	if (arrcount==7) {
		gtk_list_store_set(GTK_LIST_STORE(pd->od.lstore), iter
				,0,strarr[0],1,strarr[1],2,strarr[2],3,strarr[3]
				,4,strarr[4],5,strarr[5],6,(strarr[6][0] != '0')
				,7,strarr,-1);
	}
}
static void outputbox_apply_change(Tprefdialog *pd, gint type, gchar *path, gchar *newval, gint index) {
	pref_apply_change(pd->od.lstore,7,type,path,newval,index);
}
static void outputbox_0_edited_lcb(GtkCellRendererText *cellrenderertext,gchar *path,gchar *newtext,Tprefdialog *pd) {
	outputbox_apply_change(pd, 1, path, newtext, 0);
}
static void outputbox_1_edited_lcb(GtkCellRendererText *cellrenderertext,gchar *path,gchar *newtext,Tprefdialog *pd) {
	outputbox_apply_change(pd, 1, path, newtext, 1);
}
static void outputbox_2_edited_lcb(GtkCellRendererText *cellrenderertext,gchar *path,gchar *newtext,Tprefdialog *pd) {
	outputbox_apply_change(pd, 1, path, newtext, 2);
}
static void outputbox_3_edited_lcb(GtkCellRendererText *cellrenderertext,gchar *path,gchar *newtext,Tprefdialog *pd) {
	outputbox_apply_change(pd, 1, path, newtext, 3);
}
static void outputbox_4_edited_lcb(GtkCellRendererText *cellrenderertext,gchar *path,gchar *newtext,Tprefdialog *pd) {
	outputbox_apply_change(pd, 1, path, newtext, 4);
}
static void outputbox_5_edited_lcb(GtkCellRendererText *cellrenderertext,gchar *path,gchar *newtext,Tprefdialog *pd) {
	outputbox_apply_change(pd, 1, path, newtext, 5);
}
static void outputbox_6_toggled_lcb(GtkCellRendererToggle *cellrenderertoggle,gchar *path,Tprefdialog *pd) {
	gchar *val = g_strdup(cellrenderertoggle->active ? "0" : "1");
	outputbox_apply_change(pd, 2, path, val, 6);
	g_free(val);
}
static void add_new_outputbox_lcb(GtkWidget *wid, Tprefdialog *pd) {
	gchar **strarr;
	GtkTreeIter iter;
	strarr = pref_create_empty_strarr(7);
	gtk_list_store_append(GTK_LIST_STORE(pd->od.lstore), &iter);
	set_outputbox_strarr_in_list(&iter, strarr,pd);
	pd->lists[extoutputbox] = g_list_append(pd->lists[extoutputbox], strarr);
	pd->od.insertloc = -1;
}
static void delete_outputbox_lcb(GtkWidget *wid, Tprefdialog *pd) {
	pref_delete_strarr(pd, &pd->od, 7);
}

static void create_outputbox_gui(Tprefdialog *pd, GtkWidget *vbox1) {
	GtkWidget *hbox, *but, *scrolwin, *label;
	pd->lists[extoutputbox] = duplicate_arraylist(main_v->props.external_outputbox);
	pd->od.lstore = gtk_list_store_new (8,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_BOOLEAN,G_TYPE_POINTER);
	pd->od.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->od.lstore));
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 1, G_CALLBACK(outputbox_0_edited_lcb), pd, _("Name"), 0);
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 1, G_CALLBACK(outputbox_1_edited_lcb), pd, _("Pattern"), 1);
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 1, G_CALLBACK(outputbox_2_edited_lcb), pd, _("File #"), 2);
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 1, G_CALLBACK(outputbox_3_edited_lcb), pd, _("Line #"), 3);
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 1, G_CALLBACK(outputbox_4_edited_lcb), pd, _("Output #"), 4);
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 1, G_CALLBACK(outputbox_5_edited_lcb), pd, _("Command"), 5);
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 2, G_CALLBACK(outputbox_6_toggled_lcb), pd, _("Show all output"), 6);
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<small><b>Input options</b>\nspecify no options to read the input from the standard output\n%s local filename (available for local files)\n%i temporary fifo for input, equals %s if the document is not modified and local\n%I temporary filename for input, equals %s if the document is not modified and local\n<b>Output options</b>\nspecify no options to read the output from the standard output\n%o temporary fifo\n%O temporary filename\n%t temporary filename for both input and output (for in-place-editing filters)\n<b>Other options</b>\n%c local directory of file (available for local files)\n%n filename without path (available for all titled files)\n%u URL (available for all titled files)\n%p preview URL if basedir and preview dir are set in project settings, else identical to %u</small>"));
	gtk_box_pack_start(GTK_BOX(vbox1),label, TRUE, TRUE, 2);
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), pd->od.lview);
	gtk_widget_set_size_request(scrolwin, 200, 190);
	gtk_box_pack_start(GTK_BOX(vbox1), scrolwin, TRUE, TRUE, 2);
	{
		GList *tmplist = g_list_first(pd->lists[extoutputbox]);
		while (tmplist) {
			gint arrcount;
			gchar **strarr = (gchar **)tmplist->data;
			arrcount = count_array(strarr);
			if (arrcount==7) {
				GtkTreeIter iter;
				gtk_list_store_append(GTK_LIST_STORE(pd->od.lstore), &iter);
				set_outputbox_strarr_in_list(&iter, strarr,pd);
			}
			tmplist = g_list_next(tmplist);
		}
	}
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(pd->od.lview), TRUE);
	pd->od.thelist = &pd->lists[extoutputbox];
	pd->od.insertloc = -1;
	g_signal_connect(G_OBJECT(pd->od.lstore), "row-inserted", G_CALLBACK(listpref_row_inserted), &pd->od);
	g_signal_connect(G_OBJECT(pd->od.lstore), "row-deleted", G_CALLBACK(listpref_row_deleted), &pd->od);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox, TRUE, TRUE, 2);
	but = bf_gtkstock_button(GTK_STOCK_ADD, G_CALLBACK(add_new_outputbox_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox),but, FALSE, FALSE, 2);
	but = bf_gtkstock_button(GTK_STOCK_DELETE, G_CALLBACK(delete_outputbox_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox),but, FALSE, FALSE, 2);
}

/**************************************/
/* MAIN DIALOG FUNCTIONS              */
/**************************************/

static void preferences_destroy_lcb(GtkWidget * widget, Tprefdialog *pd) {
	GtkTreeSelection *select;
	DEBUG_MSG("preferences_destroy_lcb, started\n");

	free_arraylist(pd->lists[textstyles]);
	free_arraylist(pd->lists[syntax_styles]);
	pd->lists[syntax_styles] = NULL;
	free_arraylist(pd->lists[extcommands]);
	free_arraylist(pd->lists[extfilters]);
	free_arraylist(pd->lists[extoutputbox]);
	pd->lists[extcommands] = NULL;
	pd->lists[extfilters] = NULL;
	pd->lists[extoutputbox] = NULL;
	
/*	g_signal_handlers_destroy(G_OBJECT(GTK_COMBO(pd->bd.combo)->list));*/
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->bd.lview));
	g_signal_handlers_destroy(G_OBJECT(select));
/*	g_signal_handlers_destroy(G_OBJECT(GTK_COMBO(pd->ed.combo)->list));*/
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->ed.lview));
	g_signal_handlers_destroy(G_OBJECT(select));
	DEBUG_MSG("preferences_destroy_lcb, about to destroy the window\n");
	window_destroy(pd->win);
	g_free(pd);
}
static void preferences_apply(Tprefdialog *pd) {
	DEBUG_MSG("preferences_apply, started\n");
#ifndef NOSPLASH
	integer_apply(&main_v->props.show_splash_screen, pd->prefs[editor_show_splash_screen], TRUE);
#endif /* #ifndef NOSPLASH */
	string_apply(&main_v->props.editor_font_string, pd->prefs[editor_font_string]);
	integer_apply(&main_v->props.editor_tab_width, pd->prefs[editor_tab_width], FALSE);
	integer_apply(&main_v->props.editor_smart_cursor, pd->prefs[editor_smart_cursor], TRUE);
	integer_apply(&main_v->props.editor_indent_wspaces, pd->prefs[editor_indent_wspaces], TRUE);
	integer_apply(&main_v->props.word_wrap, pd->prefs[word_wrap], TRUE);
	integer_apply(&main_v->props.view_line_numbers, pd->prefs[view_line_numbers], TRUE);	
	integer_apply(&main_v->props.view_blocks, pd->prefs[view_blocks], TRUE);
	integer_apply(&main_v->props.view_symbols, pd->prefs[view_symbols], TRUE);
	integer_apply(&main_v->props.view_mbhl, pd->prefs[view_mbhl], TRUE);
	integer_apply(&main_v->props.view_cline, pd->prefs[view_cline], TRUE);
	main_v->props.scan_mode = gtk_option_menu_get_history(GTK_OPTION_MENU(pd->prefs[scan_mode]));
	string_apply(&main_v->props.editor_fg, pd->prefs[editor_fg]);
	string_apply(&main_v->props.editor_bg, pd->prefs[editor_bg]);	
	integer_apply(&main_v->props.view_rmargin, pd->prefs[view_rmargin], TRUE);
	integer_apply(&main_v->props.rmargin_at, pd->prefs[rmargin_at], FALSE);
	{
	  	guint key;
		GdkModifierType mod;
		g_closure_ref(main_v->autocompletion->closure);		
		gtk_accel_group_disconnect(main_v->autocompletion->group,main_v->autocompletion->closure);
		button_apply(&main_v->props.autocomp_key, pd->prefs[autocomp_key]);		
		gtk_accelerator_parse(main_v->props.autocomp_key,&key,&mod);
		gtk_accel_group_connect(main_v->autocompletion->group,key,mod,GTK_ACCEL_VISIBLE, main_v->autocompletion->closure);
		g_closure_unref(main_v->autocompletion->closure);					
	}

	integer_apply(&main_v->props.defaulthighlight, pd->prefs[defaulthighlight], TRUE);

/*	integer_apply(&main_v->props.bookmarks_default_store, pd->prefs[bookmarks_default_store], TRUE);
	main_v->props.bookmarks_filename_mode = gtk_option_menu_get_history(GTK_OPTION_MENU(pd->prefs[bookmarks_filename_mode]));*/

	integer_apply(&main_v->props.xhtml, pd->prefs[xhtml], TRUE);
	if (main_v->props.xhtml) {
		main_v->props.lowercase_tags = 1;
		main_v->props.allow_dep = 0;
	} else {
		integer_apply(&main_v->props.lowercase_tags, pd->prefs[lowercase_tags], TRUE);
		integer_apply(&main_v->props.allow_dep, pd->prefs[allow_dep], TRUE);
	}
	integer_apply(&main_v->props.auto_update_meta_author, pd->prefs[auto_update_meta_author], TRUE);
	integer_apply(&main_v->props.auto_update_meta_date, pd->prefs[auto_update_meta_date], TRUE);
	integer_apply(&main_v->props.auto_update_meta_generator, pd->prefs[auto_update_meta_generator], TRUE);
	
	string_apply(&main_v->props.newfile_default_encoding, GTK_COMBO(pd->prefs[newfile_default_encoding])->entry);
	integer_apply(&main_v->props.auto_set_encoding_meta, pd->prefs[auto_set_encoding_meta], TRUE);
	integer_apply(&main_v->props.backup_file, pd->prefs[backup_file], TRUE);
	string_apply(&main_v->props.backup_suffix, pd->prefs[backup_suffix]);
	string_apply(&main_v->props.backup_prefix, pd->prefs[backup_prefix]);
	main_v->props.backup_abort_action = gtk_option_menu_get_history(GTK_OPTION_MENU(pd->prefs[backup_abort_action]));
	integer_apply(&main_v->props.backup_cleanuponclose, pd->prefs[backup_cleanuponclose], TRUE);
	integer_apply(&main_v->props.num_undo_levels, pd->prefs[num_undo_levels], FALSE);
	integer_apply(&main_v->props.clear_undo_on_save, pd->prefs[clear_undo_on_save], TRUE);
#ifdef WITH_MSG_QUEUE
	integer_apply(&main_v->props.open_in_running_bluefish, pd->prefs[open_in_running_bluefish], TRUE);
#endif
	main_v->props.modified_check_type = gtk_option_menu_get_history(GTK_OPTION_MENU(pd->prefs[modified_check_type]));
	integer_apply(&main_v->props.do_periodic_check, pd->prefs[do_periodic_check], TRUE);
	integer_apply(&main_v->props.max_recent_files, pd->prefs[max_recent_files], FALSE);
	integer_apply(&main_v->props.restore_dimensions, pd->prefs[restore_dimensions], TRUE);
	if (!main_v->props.restore_dimensions) {
		integer_apply(&main_v->globses.left_panel_width, pd->prefs[left_panel_width], FALSE);
		integer_apply(&main_v->globses.main_window_h, pd->prefs[main_window_h], FALSE);
		integer_apply(&main_v->globses.main_window_w, pd->prefs[main_window_w], FALSE);
	}
	string_apply(&main_v->props.tab_font_string, pd->prefs[tab_font_string]);
	main_v->props.document_tabposition = gtk_option_menu_get_history(GTK_OPTION_MENU(pd->prefs[document_tabposition]));
	main_v->props.leftpanel_tabposition = gtk_option_menu_get_history(GTK_OPTION_MENU(pd->prefs[leftpanel_tabposition]));
	main_v->props.left_panel_left = gtk_option_menu_get_history(GTK_OPTION_MENU(pd->prefs[left_panel_left]));

	string_apply(&main_v->props.bflib_info_font, pd->prefs[bflib_info_font]);
	string_apply(&main_v->props.bflib_info_bkg, pd->prefs[bflib_info_bkg]);
	string_apply(&main_v->props.bflib_info_fg, pd->prefs[bflib_info_fg]);
	
/*	integer_apply(&main_v->props.view_main_toolbar, pd->prefs[view_main_toolbar], TRUE);
	integer_apply(&main_v->props.view_left_panel, pd->prefs[view_left_panel], TRUE);
	integer_apply(&main_v->props.view_custom_menu, pd->prefs[view_custom_menu], TRUE);
	integer_apply(&main_v->props.view_html_toolbar, pd->prefs[view_html_toolbar], TRUE);*/
	integer_apply(&main_v->props.transient_htdialogs, pd->prefs[transient_htdialogs], TRUE);
	
/*	string_apply(&main_v->props.default_basedir, pd->prefs[default_basedir]);*/
	integer_apply(&main_v->props.filebrowser_two_pane_view, pd->prefs[filebrowser_two_pane_view], TRUE);
	string_apply(&main_v->props.filebrowser_unknown_icon, pd->prefs[filebrowser_unknown_icon]);
	
	string_apply(&main_v->props.image_thumbnailstring, pd->prefs[image_thumbnailstring]);
	string_apply(&main_v->props.image_thumbnailtype, GTK_COMBO(pd->prefs[image_thumbnailtype])->entry);

	integer_apply(&main_v->props.server_zope_compat, pd->prefs[server_zope_compat], TRUE);

	/*filetype_apply_changes(pd);*/
	/*filefilter_apply_changes(pd);*/
	/*browsers_apply_changes(pd);*/
	/*externals_apply_changes(pd);*/
	/*outputbox_apply_changes(pd);*/

	free_arraylist(main_v->props.plugin_config);
	main_v->props.plugin_config = duplicate_arraylist(pd->lists[pluginconfig]);
/*
	free_arraylist(main_v->props.filetypes);
	main_v->props.filetypes = duplicate_arraylist(pd->lists[filetypes]);
*/
	DEBUG_MSG("preferences_apply: free old syntax styles, and building new list\n");
	free_arraylist(main_v->props.syntax_styles);
	main_v->props.syntax_styles = duplicate_arraylist(pd->lists[syntax_styles]);
	
	free_arraylist(main_v->props.external_command);
	main_v->props.external_command = duplicate_arraylist(pd->lists[extcommands]);
	
	free_arraylist(main_v->props.external_filter);
	main_v->props.external_filter = duplicate_arraylist(pd->lists[extfilters]);
	
	free_arraylist(main_v->props.external_outputbox);
	main_v->props.external_outputbox = duplicate_arraylist(pd->lists[extoutputbox]);

	DEBUG_MSG("preferences_apply: free old textstyles, and building new list\n");
	free_arraylist(main_v->props.textstyles);
	main_v->props.textstyles = duplicate_arraylist(pd->lists[textstyles]);

	/* apply the changes to highlighting patterns and filetypes to the running program */

	textstyle_rebuild();

	bf_lang_mgr_retag();
	
	all_documents_apply_settings();
	{
		GList *tmplist = g_list_first(main_v->bfwinlist);
		while (tmplist) {
			Tbfwin *bfwin = BFWIN(tmplist->data);
			DEBUG_MSG("preferences_ok_clicked_lcb, calling encoding_menu_rebuild\n");
			encoding_menu_rebuild(bfwin);
			external_menu_rebuild(bfwin); /* browsers is also rebuild here! */
			DEBUG_MSG("preferences_ok_clicked_lcb, calling gui_apply_settings\n");
			gui_apply_settings(bfwin);
			left_panel_rebuild(bfwin);
			DEBUG_MSG("preferences_ok_clicked_lcb, calling doc_force_activate\n");
			doc_force_activate(bfwin->current_document);
			tmplist = g_list_next(tmplist);
		}
	}
	rcfile_save_main();
}

static void preferences_cancel_clicked_lcb(GtkWidget *wid, Tprefdialog *pd) {
	preferences_destroy_lcb(NULL, pd);
}
static void preferences_apply_clicked_lcb(GtkWidget *wid, Tprefdialog *pd) {
	preferences_apply(pd);
}
static void preferences_ok_clicked_lcb(GtkWidget *wid, Tprefdialog *pd) {
	preferences_apply(pd);
	preferences_destroy_lcb(NULL, pd);
}

static void restore_dimensions_toggled_lcb(GtkToggleButton *togglebutton,Tprefdialog *pd) {
	gtk_widget_set_sensitive(pd->prefs[left_panel_width], !togglebutton->active);
	gtk_widget_set_sensitive(pd->prefs[main_window_h], !togglebutton->active);
	gtk_widget_set_sensitive(pd->prefs[main_window_w], !togglebutton->active);
}
static void create_backup_toggled_lcb(GtkToggleButton *togglebutton,Tprefdialog *pd) {
    gtk_widget_set_sensitive(pd->prefs[backup_prefix], togglebutton->active);
    gtk_widget_set_sensitive(pd->prefs[backup_suffix], togglebutton->active);
    gtk_widget_set_sensitive(pd->prefs[backup_abort_action], togglebutton->active);
    gtk_widget_set_sensitive(pd->prefs[backup_cleanuponclose], togglebutton->active);
}

void preftree_cursor_changed_cb (GtkTreeView *treeview, gpointer user_data) {
	Tprefdialog *pd = (Tprefdialog*)user_data;
	GtkTreePath *path;
	GtkTreeIter iter;
	gpointer child;
	GList *lst;
	
	lst = gtk_container_get_children(GTK_CONTAINER(pd->fixed));
	while (lst) {
		if (GTK_IS_WIDGET(lst->data)) {
			g_object_ref(G_OBJECT(lst->data));
			gtk_container_remove(GTK_CONTAINER(pd->fixed),lst->data);
		}	
		lst = g_list_next(lst);
	}
	
	gtk_tree_view_get_cursor(treeview,&path,NULL);
	if ( path ) {
		gtk_tree_model_get_iter(GTK_TREE_MODEL(pd->nstore),&iter,path);
		gtk_tree_model_get(GTK_TREE_MODEL(pd->nstore),&iter,WIDGETCOL,&child,-1);
		if ( child ) {
			gtk_box_pack_start(GTK_BOX(pd->fixed),child,TRUE,TRUE,1);
			gtk_widget_show_all(pd->fixed);
		}	
		gtk_tree_path_free(path);
	}
}

/* List of language files */
	
static void rescan_lang_files(Tprefdialog *pd)
{
	const gchar *filename;
	GError *error = NULL;
	GtkTreeIter iter;
	GPatternSpec *ps = g_pattern_spec_new("*.bflang"); 
	GDir *gd;
	gchar *userdir = g_strconcat(g_get_home_dir(), "/."PACKAGE"/", NULL);

	if ( pd->lang_files != NULL ) {
		gtk_list_store_clear(pd->lang_files);
	} else {
			pd->lang_files = gtk_list_store_new (1, G_TYPE_STRING);
	}
   gtk_list_store_append (pd->lang_files, &iter);
 	gtk_list_store_set (pd->lang_files, &iter,0, "",-1);	
	gd = g_dir_open(PKGDATADIR, 0, &error);
	filename = g_dir_read_name(gd);
	while (filename) {
		if (g_pattern_match(ps, strlen(filename), filename, NULL) ) {
 	      gtk_list_store_append (pd->lang_files, &iter);
      	gtk_list_store_set (pd->lang_files, &iter,0, filename,-1);
		}
		filename = g_dir_read_name(gd);
	}
	g_dir_close(gd);
	
	gd = g_dir_open(userdir, 0, &error);
	filename = g_dir_read_name(gd);
	while (filename) {
		if (g_pattern_match(ps, strlen(filename), filename, NULL) ) {
 	      gtk_list_store_append (pd->lang_files, &iter);
      	gtk_list_store_set (pd->lang_files, &iter,0, filename,-1);
		}
		filename = g_dir_read_name(gd);
	}
	g_dir_close(gd);
	g_free(userdir);
	g_pattern_spec_free(ps);

}

static void preferences_dialog() {
	Tprefdialog *pd;
	GtkWidget *dvbox, *frame, *vbox1, *vbox2;
	gchar *notebooktabpositions[] = {N_("left"), N_("right"), N_("top"), N_("bottom"), NULL};
	gchar *panellocations[] = {N_("right"), N_("left"), NULL};
	gchar *modified_check_types[] = {N_("Nothing"), N_("Modified time and file size"), N_("Modified time"), N_("File size"), NULL};
	GtkWidget *dhbox, *label;	
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;	
	GtkTreeIter auxit,iter;
	GtkTreePath *path;

	pd = g_new0(Tprefdialog,1);
	pd->win = window_full(_("Edit preferences"), GTK_WIN_POS_CENTER, 0, G_CALLBACK(preferences_destroy_lcb), pd, TRUE);
	rescan_lang_files(pd);
	
	dvbox = gtk_vbox_new(FALSE, 5);
	dhbox = gtk_hbox_new(FALSE, 5);
	pd->fixed = gtk_hbox_new(FALSE,5);
	pd->nstore = gtk_tree_store_new(2,G_TYPE_STRING,G_TYPE_POINTER);
	pd->noteb = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->nstore));
	gtk_box_pack_start(GTK_BOX(dhbox), pd->noteb, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(dhbox), pd->fixed, TRUE, TRUE, 5);
	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", cell, "text", NAMECOL, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(pd->noteb), column);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(pd->noteb), FALSE);	
	gtk_box_pack_start(GTK_BOX(dvbox), dhbox, TRUE, TRUE, 5);
	gtk_container_add(GTK_CONTAINER(pd->win), dvbox);
/*
	gtk_container_add(GTK_CONTAINER(pd->win), dvbox);
	pd->noteb = gtk_notebook_new();
	gtk_notebook_set_homogeneous_tabs(GTK_NOTEBOOK(pd->noteb), TRUE);
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(pd->noteb), GTK_POS_LEFT);	
	gtk_box_pack_start(GTK_BOX(dvbox), pd->noteb, FALSE, FALSE, 0);
*/

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Editor"), WIDGETCOL,vbox1,-1);
/*
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("Editor"),150,TRUE));
*/

#ifndef NOSPLASH
	frame = gtk_frame_new(_("Editor startup options"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	pd->prefs[editor_show_splash_screen] = boxed_checkbut_with_value(_("Show splash-screen"), main_v->props.show_splash_screen, vbox2);
#endif /* #ifndef NOSPLASH */
	
	frame = gtk_frame_new(_("Editor options"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	
	pd->prefs[editor_font_string] = prefs_string(_("Font"), main_v->props.editor_font_string, vbox2, pd, string_font);	
	pd->prefs[editor_fg] = prefs_string(_("Foreground color"), main_v->props.editor_fg, vbox2, pd, string_color);
	pd->prefs[editor_bg] = prefs_string(_("Background color"), main_v->props.editor_bg, vbox2, pd, string_color);	
	pd->prefs[editor_tab_width] = prefs_integer(_("Tab width"), main_v->props.editor_tab_width, vbox2, pd, 1, 50);
	pd->prefs[editor_smart_cursor] = boxed_checkbut_with_value(_("Smart cursor positioning"), main_v->props.editor_smart_cursor, vbox2);
	pd->prefs[editor_indent_wspaces] = boxed_checkbut_with_value(_("Use spaces to indent, not tabs"), main_v->props.editor_indent_wspaces, vbox2);
	pd->prefs[word_wrap] = boxed_checkbut_with_value(_("Word wrap default"), main_v->props.word_wrap, vbox2);
	pd->prefs[view_line_numbers] = boxed_checkbut_with_value(_("Line numbers by default"), main_v->props.view_line_numbers, vbox2);	
	pd->prefs[view_blocks] = boxed_checkbut_with_value(_("Block folding view by default"), main_v->props.view_blocks, vbox2);
	pd->prefs[view_symbols] = boxed_checkbut_with_value(_("Symbols view by default"), main_v->props.view_symbols, vbox2);
	pd->prefs[defaulthighlight] = boxed_checkbut_with_value(_("Highlight syntax by default"), main_v->props.defaulthighlight, vbox2);

	frame = gtk_frame_new(_("Undo options"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	pd->prefs[num_undo_levels] = prefs_integer(_("Undo history size"), main_v->props.num_undo_levels, vbox2, pd, 50, 10000);
	pd->prefs[clear_undo_on_save] = boxed_checkbut_with_value(_("Clear undo history on save"), main_v->props.clear_undo_on_save, vbox2);
	
/*	frame = gtk_frame_new(_("Bookmark options"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	
	pd->prefs[bookmarks_default_store] = boxed_checkbut_with_value(_("Make permanent by default"), main_v->props.bookmarks_default_store, vbox2);
	{
		gchar *actions[] = {N_("full path"), N_("path from basedir"), N_("filename"), NULL};
		pd->prefs[bookmarks_filename_mode] = boxed_optionmenu_with_value(_("Bookmarks filename display"), main_v->props.bookmarks_filename_mode, vbox2, actions);
	}*/

	vbox1 = gtk_vbox_new(FALSE, 5);

	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("HTML"), WIDGETCOL,vbox1,-1);	
/*
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("HTML"),154,TRUE));
*/

	frame = gtk_frame_new(_("HTML options"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	
	pd->prefs[lowercase_tags] = boxed_checkbut_with_value(_("Lowercase HTML tags"), main_v->props.lowercase_tags, vbox2);
	pd->prefs[allow_dep] = boxed_checkbut_with_value(_("Use deprecated tags (e.g. <font> and <nobr>)"), main_v->props.allow_dep, vbox2);
	pd->prefs[xhtml] = boxed_checkbut_with_value(_("Use XHTML style tags (<br />)"), main_v->props.xhtml, vbox2);
	
	pd->prefs[auto_update_meta_author] = boxed_checkbut_with_value(_("Automatically update author meta tag"), main_v->props.auto_update_meta_author, vbox2);
	pd->prefs[auto_update_meta_date] = boxed_checkbut_with_value(_("Automatically update date meta tag"), main_v->props.auto_update_meta_date, vbox2);
	pd->prefs[auto_update_meta_generator] = boxed_checkbut_with_value(_("Automatically update generator meta tag"), main_v->props.auto_update_meta_generator, vbox2);
	
	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Files"), WIDGETCOL,vbox1,-1);	
/*
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("Files"),152,TRUE));
*/

	frame = gtk_frame_new(_("Encoding"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	{
		GList *tmplist, *poplist = NULL;
		tmplist = g_list_first(main_v->props.encodings);
		while (tmplist) {
			gchar **strarr = (gchar **)tmplist->data;
			poplist = g_list_append(poplist, strarr[1]);
			tmplist = g_list_next(tmplist);
		}
		pd->prefs[newfile_default_encoding] = prefs_combo(_("Default character set for new files"),main_v->props.newfile_default_encoding, vbox2, pd, poplist, TRUE);
		g_list_free(poplist);
	}	
	pd->prefs[auto_set_encoding_meta] = boxed_checkbut_with_value(_("Auto set <meta> HTML tag on encoding change"), main_v->props.auto_set_encoding_meta, vbox2);

	frame = gtk_frame_new(_("Backup"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	pd->prefs[backup_file] = boxed_checkbut_with_value(_("Create backup file during file save"), main_v->props.backup_file, vbox2);
	label = gtk_label_new(_("The backup prefix may contain a slash to indicate a directory component, for example \".backup/\""));
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox2),label, TRUE, TRUE, 2);
	pd->prefs[backup_prefix] = prefs_string(_("Backup file prefix"), main_v->props.backup_prefix, vbox2, pd, string_none);
	pd->prefs[backup_suffix] = prefs_string(_("Backup file suffix"), main_v->props.backup_suffix, vbox2, pd, string_none);
	{
		gchar *failureactions[] = {N_("Continue save"), N_("Abort save"), N_("Ask what to do"), NULL};
		pd->prefs[backup_abort_action] = boxed_optionmenu_with_value(_("If the backup fails"), main_v->props.backup_abort_action, vbox2, failureactions);
	}
	pd->prefs[backup_cleanuponclose] = boxed_checkbut_with_value(_("Remove backup file on close"), main_v->props.backup_cleanuponclose, vbox2);
	create_backup_toggled_lcb(GTK_TOGGLE_BUTTON(pd->prefs[backup_file]), pd);
	g_signal_connect(G_OBJECT(pd->prefs[backup_file]), "toggled", G_CALLBACK(create_backup_toggled_lcb), pd);

	frame = gtk_frame_new(_("Misc"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

#ifdef WITH_MSG_QUEUE
	pd->prefs[open_in_running_bluefish] = boxed_checkbut_with_value(_("Open files in already running bluefish window"),main_v->props.open_in_running_bluefish, vbox2);
#endif /* WITH_MSG_QUEUE */		
	pd->prefs[modified_check_type] = boxed_optionmenu_with_value(_("File properties to check on disk for modifications"), main_v->props.modified_check_type, vbox2, modified_check_types);
	pd->prefs[do_periodic_check] = boxed_checkbut_with_value(_("Periodically check if file is modified on disk"), main_v->props.do_periodic_check, vbox2);
	pd->prefs[max_recent_files] = prefs_integer(_("Number of files in 'Open recent' menu"), main_v->props.max_recent_files, vbox2, pd, 3, 100);

	vbox1 = gtk_vbox_new(FALSE, 5);

	gtk_tree_store_append(pd->nstore, &iter, NULL);
	gtk_tree_store_set(pd->nstore, &iter, NAMECOL,_("User interface"), WIDGETCOL,vbox1,-1);	

   frame = gtk_frame_new(_("General"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	
	pd->prefs[transient_htdialogs] = boxed_checkbut_with_value(_("Make HTML dialogs transient"), main_v->props.transient_htdialogs, vbox2);
	pd->prefs[tab_font_string] = prefs_string(_("Notebook tab font \n(leave empty for gtk default)"), main_v->props.tab_font_string, vbox2, pd, string_font);
	pd->prefs[document_tabposition] = boxed_optionmenu_with_value(_("Document notebook tab position"), main_v->props.document_tabposition, vbox2, notebooktabpositions);
	pd->prefs[leftpanel_tabposition] = boxed_optionmenu_with_value(_("Sidebar notebook tab position"), main_v->props.leftpanel_tabposition, vbox2, notebooktabpositions);
	pd->prefs[left_panel_left] = boxed_optionmenu_with_value(_("Sidebar location"), main_v->props.left_panel_left, vbox2, panellocations);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, &iter);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Dimensions"), WIDGETCOL,vbox1,-1);	
	
	frame = gtk_frame_new(_("Dimensions"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	pd->prefs[restore_dimensions] = boxed_checkbut_with_value(_("Restore last used dimensions"), main_v->props.restore_dimensions, vbox2);
	pd->prefs[left_panel_width] = prefs_integer(_("Initial sidebar width"), main_v->globses.left_panel_width, vbox2, pd, 1, 4000);
	pd->prefs[main_window_h] = prefs_integer(_("Initial window height"), main_v->globses.main_window_h, vbox2, pd, 1, 4000);
	pd->prefs[main_window_w] = prefs_integer(_("Initial window width"), main_v->globses.main_window_w, vbox2, pd, 1, 4000);
	restore_dimensions_toggled_lcb(GTK_TOGGLE_BUTTON(pd->prefs[restore_dimensions]), pd);
	g_signal_connect(G_OBJECT(pd->prefs[restore_dimensions]), "toggled", G_CALLBACK(restore_dimensions_toggled_lcb), pd);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, &iter);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Reference library"), WIDGETCOL,vbox1,-1);	

	frame = gtk_frame_new(_("Reference library"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	pd->prefs[bflib_info_font] = prefs_string(_("Font"), main_v->props.bflib_info_font, vbox2, pd, string_font);
	pd->prefs[bflib_info_bkg] = prefs_string(_("Info background color"), main_v->props.bflib_info_bkg, vbox2, pd, string_color);
	pd->prefs[bflib_info_fg] = prefs_string(_("Info foreground color"), main_v->props.bflib_info_fg, vbox2, pd, string_color);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, &iter);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("File and directory view"), WIDGETCOL,vbox1,-1);	

	frame = gtk_frame_new(_("File and directory view"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
/*	pd->prefs[default_basedir] = prefs_string(_("Default basedir"), main_v->props.default_basedir, vbox2, pd, string_none);*/
	pd->prefs[filebrowser_two_pane_view] = boxed_checkbut_with_value(_("Use separate file and directory view"), main_v->props.filebrowser_two_pane_view, vbox2);
	pd->prefs[filebrowser_unknown_icon] = prefs_string(_("Unknown icon"), main_v->props.filebrowser_unknown_icon, vbox2, pd, string_file);
	
	
/*
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("User interface"), 156,TRUE));


	frame = gtk_frame_new(_("Dimensions"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	pd->prefs[restore_dimensions] = boxed_checkbut_with_value(_("Restore last used dimensions"), main_v->props.restore_dimensions, vbox2);
	pd->prefs[left_panel_width] = prefs_integer(_("Initial sidebar width"), main_v->globses.left_panel_width, vbox2, pd, 1, 4000);
	pd->prefs[main_window_h] = prefs_integer(_("Initial window height"), main_v->globses.main_window_h, vbox2, pd, 1, 4000);
	pd->prefs[main_window_w] = prefs_integer(_("Initial window width"), main_v->globses.main_window_w, vbox2, pd, 1, 4000);
	restore_dimensions_toggled_lcb(GTK_TOGGLE_BUTTON(pd->prefs[restore_dimensions]), pd);
	g_signal_connect(G_OBJECT(pd->prefs[restore_dimensions]), "toggled", G_CALLBACK(restore_dimensions_toggled_lcb), pd);

	frame = gtk_frame_new(_("General"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	

/ *	pd->prefs[view_main_toolbar] = boxed_checkbut_with_value(_("Show main toolbar by default"), main_v->props.view_main_toolbar, vbox2);
	pd->prefs[view_left_panel] = boxed_checkbut_with_value(_("Show sidebar by default"), main_v->props.view_left_panel, vbox2);
	pd->prefs[view_custom_menu] = boxed_checkbut_with_value(_("Show custom menu by default"), main_v->props.view_custom_menu, vbox2);
	pd->prefs[view_html_toolbar] = boxed_checkbut_with_value(_("Show HTML toolbar by default"), main_v->props.view_html_toolbar, vbox2);* /
	pd->prefs[transient_htdialogs] = boxed_checkbut_with_value(_("Make HTML dialogs transient"), main_v->props.transient_htdialogs, vbox2);

	pd->prefs[tab_font_string] = prefs_string(_("Notebook tab font (leave empty for gtk default)"), main_v->props.tab_font_string, vbox2, pd, string_font);
	
	pd->prefs[document_tabposition] = boxed_optionmenu_with_value(_("Document notebook tab position"), main_v->props.document_tabposition, vbox2, notebooktabpositions);
	pd->prefs[leftpanel_tabposition] = boxed_optionmenu_with_value(_("Sidebar notebook tab position"), main_v->props.leftpanel_tabposition, vbox2, notebooktabpositions);
	pd->prefs[left_panel_left] = boxed_optionmenu_with_value(_("Sidebar location"), main_v->props.left_panel_left, vbox2, panellocations);

	frame = gtk_frame_new(_("Reference library"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	pd->prefs[bflib_info_font] = prefs_string(_("Font"), main_v->props.bflib_info_font, vbox2, pd, string_font);
	pd->prefs[bflib_info_bkg] = prefs_string(_("Info background color"), main_v->props.bflib_info_bkg, vbox2, pd, string_color);
	pd->prefs[bflib_info_fg] = prefs_string(_("Info foreground color"), main_v->props.bflib_info_fg, vbox2, pd, string_color);
	
*/

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Images"), WIDGETCOL,vbox1,-1);	
/*
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("Images"), 155,TRUE));
*/

	frame = gtk_frame_new(_("Thumbnails"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	pd->prefs[image_thumbnailstring] = prefs_string(_("Thumbnail suffix"), main_v->props.image_thumbnailstring, vbox2, pd, string_none);
	{
		GList *poplist = g_list_append(NULL, "png");
		poplist = g_list_append(poplist, "jpeg");
		pd->prefs[image_thumbnailtype] = prefs_combo(_("Thumbnail filetype"),main_v->props.image_thumbnailtype, vbox2, pd, poplist, FALSE);
		g_list_free(poplist);
	}
/*
	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("File types"), WIDGETCOL,vbox1,-1);*/	
/*
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("File types"), 153,TRUE));
*/
/*	frame = gtk_frame_new(_("File types"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	create_filetype_gui(pd, vbox2);
*/

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("External commands"), WIDGETCOL,vbox1,-1);	

	
/*
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("External commands"), 151,TRUE));
*/
	
	frame = gtk_frame_new(_("Commands"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	create_extcommands_gui(pd, vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("External filters"), WIDGETCOL,vbox1,-1);	
/*
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("External filters"), 151,TRUE));
*/
	
	frame = gtk_frame_new(_("Filters"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	create_filters_gui(pd, vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Output parsers"), WIDGETCOL,vbox1,-1);	
/*
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("Output parsers"), 157,TRUE));
*/
	
	frame = gtk_frame_new(_("Output parsers"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	create_outputbox_gui(pd, vbox2);

/*
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("Servers"), 0,TRUE));
*/
	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Plugins"), WIDGETCOL,vbox1,-1);
	frame = gtk_frame_new(_("Plugins"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	create_plugin_gui(pd, vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Servers"), WIDGETCOL,vbox1,-1);	
	
	frame = gtk_frame_new(_("Remote servers"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	
	pd->prefs[server_zope_compat] = boxed_checkbut_with_value(_("Zope compatibility mode"), main_v->props.server_zope_compat, vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Text styles"), WIDGETCOL,vbox1,-1);
	frame = gtk_frame_new(_("Text styles"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);	
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	create_textstyle_gui(pd,vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);
	
	gtk_tree_store_append(pd->nstore, &iter, NULL);
	gtk_tree_store_set(pd->nstore, &iter, NAMECOL,_("Syntax scanning"), WIDGETCOL,vbox1,-1);	


	frame = gtk_frame_new(_("Advanced syntax scanning options"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);	
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	pd->prefs[view_mbhl] = boxed_checkbut_with_value(_("Match block begin-end by default"), main_v->props.view_mbhl, vbox2);
	pd->prefs[view_cline] = boxed_checkbut_with_value(_("Highlight current line by default"), main_v->props.view_cline, vbox2);
	pd->prefs[view_rmargin] = boxed_checkbut_with_value(_("Show right margin by default"), main_v->props.view_rmargin, vbox2);
	pd->prefs[rmargin_at] = prefs_integer(_("Right margin at"), main_v->props.rmargin_at, vbox2, pd,0, 500);
	{
		gchar *modes[] = {N_("whole document"), N_("visible area"),  NULL};
		pd->prefs[scan_mode] = boxed_optionmenu_with_value(_("Scan mode"), main_v->props.scan_mode, vbox2, modes);
	}
	pd->prefs[autocomp_key] = prefs_string(_("Autocompletion key"), main_v->props.autocomp_key, vbox2, pd, string_accel);
	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, &iter);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Syntax highlighting"), WIDGETCOL,vbox1,-1);	


	frame = gtk_frame_new(_("Syntax highlighting"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);	
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	create_hl_gui(pd,vbox2);

	/* end, create buttons for dialog now */
	{
		GtkWidget *ahbox, *but;
		ahbox = gtk_hbutton_box_new();
		gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
		gtk_button_box_set_spacing(GTK_BUTTON_BOX(ahbox), 12);

		gtk_box_pack_start(GTK_BOX(dvbox), ahbox, FALSE, FALSE, 0);
		but = bf_gtkstock_button(GTK_STOCK_APPLY, G_CALLBACK(preferences_apply_clicked_lcb), pd);
		gtk_box_pack_start(GTK_BOX(ahbox), but, TRUE, TRUE, 0);

		but = bf_stock_cancel_button(G_CALLBACK(preferences_cancel_clicked_lcb), pd);
		gtk_box_pack_start(GTK_BOX(ahbox), but, TRUE, TRUE, 0);

		but = bf_stock_ok_button(G_CALLBACK(preferences_ok_clicked_lcb), pd);
		gtk_box_pack_start(GTK_BOX(ahbox), but, TRUE, TRUE, 0);
		gtk_window_set_default(GTK_WINDOW(pd->win), but);
	}

	
	gtk_widget_show_all(pd->win);
	
	g_signal_connect(G_OBJECT(pd->noteb),"cursor-changed",G_CALLBACK(preftree_cursor_changed_cb),pd);
	path = gtk_tree_path_new_first();
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(pd->noteb),path,NULL,FALSE);
	gtk_tree_path_free(path);

}

void open_preferences_cb(GtkWidget *wid, gpointer data) {
	preferences_dialog();
}

void open_preferences_menu_cb(gpointer callback_data,guint action,GtkWidget *widget) {
	preferences_dialog();
}
