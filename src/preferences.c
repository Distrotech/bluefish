/* Bluefish HTML Editor
 * preferences.c - the preferences code
 *
 * Copyright (C) 2002-2009 Olivier Sessink
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
#include "bf_lib.h"        /* list_switch_order() */
#include "bftextview2_langmgr.h"
#include "document.h"
#include "filebrowser2.h"
#include "gtk_easy.h"
#include "gui.h"
#include "menu.h"
#include "pixmap.h"
#include "rcfile.h"
#include "stringlist.h"    /* duplicate_arraylist*/

enum {
	do_periodic_check,
	view_line_numbers,
	editor_show_splash_screen,    /* show splash screen at start-up */
	editor_font_string,           /* editor font */
	editor_tab_width,             /* editor tabwidth */
	editor_smart_cursor,
	editor_indent_wspaces,
	tab_font_string,              /* notebook tabs font */
	defaulthighlight,             /* highlight documents by default */
	transient_htdialogs,          /* set html dialogs transient ro the main window */
	leave_to_window_manager,
	restore_dimensions,
	left_panel_left,
	max_recent_files,             /* length of Open Recent list */
	max_dir_history,              /* length of directory history */
	backup_file,                  /* wheather to use a backup file */
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
	switch_tabs_by_altx,          /* switch tabs using Alt+X (#385860) */
	/* not yet in use */
	image_editor_cline,           /* image editor commandline */
	allow_dep,                    /* allow <FONT>... */
	format_by_context,            /* use <strong> instead of <b>, <emphasis instead of <i> etc. (W3C reccomendation) */
	xhtml,                        /* write <br /> */
	insert_close_tag, /* write a closingtag after a start tag */
	close_tag_newline, /* insert the closing tag after a newline */
	allow_ruby,                   /* allow <ruby> */
	force_dtd,                    /* write <!DOCTYPE...> */
	dtd_url,                      /* URL in DTD */
	xml_start,                    /* <?XML...> */
	lowercase_tags,               /* use lowercase tags */
	word_wrap,                    /* use wordwrap */
	autoindent,                   /* autoindent code */
	drop_at_drop_pos,             /* drop at drop position instead of cursor position */
	link_management,              /* perform link management */
#ifdef WITH_MSG_QUEUE
	open_in_running_bluefish,     /* open commandline documents in already running session*/
#endif /* WITH_MSG_QUEUE */
	load_reference,
	show_autocomp_reference,
	show_tooltip_reference,
	delay_full_scan,
	autocomplete,
	autocomp_popup_mode,
	reduced_scan_triggers,
	view_blocks,
	view_mbhl,
	view_cline,
	editor_fg,
	editor_bg,
	/* now the entries in globses */
	left_panel_width,
	main_window_h,
	main_window_w,
	property_num_max
};

#define FILETYPES_ARRAY_LEN 8
enum {
	extcommands,
	extfilters,
	extoutputbox,
	pluginconfig,
	textstyles,
	highlight_styles,
	bflang_options,
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
	GtkWidget *bold_radio[2];
	GtkWidget *italic_radio[2];
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
	WIDGETCOL,
	FUNCCOL,
	DATACOL
};

typedef struct {
	GtkListStore *lstore;
	GtkWidget *lview;
} Tplugindialog;

typedef struct {
	GtkTreeStore *lstore;
	GtkTreeModelFilter *lfilter;
	GtkWidget *lview;
	GtkTreeStore *lstore2;
	GtkTreeModelFilter *lfilter2;
	GtkWidget *lview2;
	Tbflang *curbflang;
} Tbflangpref;

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
	Tbflangpref bld;
} Tprefdialog;

typedef enum {
	string_none,
	string_file,
	string_font,
	string_color
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
/* type 0/1=text, 2=toggle,3=radio, 4=non-editable combo */
static GtkCellRenderer *pref_create_column(GtkTreeView *treeview, gint type, GCallback func, gpointer data, const gchar *title, gint num, gboolean collumn_hide_but) {
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
	if (collumn_hide_but) {
		but = gtk_check_button_new_with_label(title);
		g_object_set_data(G_OBJECT(but),"_title_",g_strdup(title));
		gtk_widget_show(but);
		gtk_tree_view_column_set_widget(GTK_TREE_VIEW_COLUMN(column),but);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(but),TRUE);
		g_signal_connect(G_OBJECT(column), "clicked", G_CALLBACK(pref_click_column), but);
	}
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
	g_print("pref_apply_change, tpath %p\n",tpath);
	if (tpath && gtk_tree_model_get_iter(GTK_TREE_MODEL(lstore),&iter,tpath)) {
		gtk_tree_model_get(GTK_TREE_MODEL(lstore), &iter, pointerindex, &strarr, -1);
		g_print("pref_apply_change, lstore=%p, index=%d, type=%d, got strarr=%p\n",lstore,index,type,strarr);
		if (type ==1) {
			gtk_list_store_set(GTK_LIST_STORE(lstore),&iter,index,newval,-1);
		} else {
			gtk_list_store_set(GTK_LIST_STORE(lstore),&iter,index,(newval[0] == '1'),-1);
		}
		if (strarr[index]) {
			g_print("pref_apply_change, old value for strarr[%d] was %s\n",index,strarr[index]);
			g_free(strarr[index]);
		}
		if (type == 0) {
			strarr[index] = unescape_string(newval, FALSE);
		} else {
			strarr[index] = g_strdup(newval);
		}
		g_print("pref_apply_change, strarr[%d] now is %s\n",index,strarr[index]);
	} else {
		g_print("ERROR: path %s was not converted to tpath(%p) or iter (lstore=%p)\n",path,tpath,lstore);
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

static GtkWidget *prefs_string(const gchar *title, const gchar *curval, GtkWidget *box, Tprefdialog *pd, Tprefstringtype prefstringtype) {
	GtkWidget *hbox, *return_widget;

	hbox = gtk_hbox_new(FALSE,3);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(title), FALSE, FALSE, 3);
	if (prefstringtype == string_color) {
		return_widget = boxed_entry_with_text(curval, 8, hbox);
		gtk_entry_set_width_chars(GTK_ENTRY(return_widget),8);
	} else
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


/************ plugin code ****************/
static void set_plugin_strarr_in_list(GtkTreeIter *iter, gchar **strarr, Tprefdialog *pd) {
	gint arrcount;
	arrcount = count_array(strarr);
	if (arrcount==3) {
		DEBUG_MSG("set_plugin_strarr_in_list, arrcount=%d, file=%s\n",arrcount,strarr[0]);
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
	GtkWidget *hbox, *scrolwin;
	pd->lists[pluginconfig] = duplicate_arraylist(main_v->props.plugin_config);
	pd->pd.lstore = gtk_list_store_new (4,G_TYPE_STRING,G_TYPE_BOOLEAN,G_TYPE_STRING,G_TYPE_POINTER);
	pd->pd.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->pd.lstore));
	pref_create_column(GTK_TREE_VIEW(pd->pd.lview), 1, NULL, pd, _("Message"), 0,FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->pd.lview), 2, G_CALLBACK(plugin_1_toggled_lcb), pd, _("Enabled"), 1,FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->pd.lview), 1, NULL, pd, _("File"), 2,FALSE);
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
	gtk_box_pack_start(GTK_BOX(vbox1),hbox, FALSE, FALSE, 2);
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
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->tsd.bold_radio[0]), (strarr[3][0] != '1'));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->tsd.bold_radio[1]), (strarr[3][0] == '1'));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->tsd.italic_radio[0]), (strarr[4][0] != '1'));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->tsd.italic_radio[1]), (strarr[4][0] == '1'));
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
		} else if ((GtkWidget *)togglebutton == pd->tsd.italic_radio[0]) {
			index = 4;
			val = "0";
		} else if ((GtkWidget *)togglebutton == pd->tsd.italic_radio[1]) {
			index = 4;
			val = "1";
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
	pref_create_column(GTK_TREE_VIEW(pd->tsd.lview), 1, G_CALLBACK(textstyle_0_edited_lcb), pd, _("Label"), 0,FALSE);
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
	pd->tsd.bold_radio[0] = gtk_radio_button_new_with_label(NULL, _("normal weight"));
	gtk_box_pack_start(GTK_BOX(vbox),pd->tsd.bold_radio[0], FALSE, FALSE, 0);
	pd->tsd.bold_radio[1] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pd->tsd.bold_radio[0]), _("bold weight"));
	gtk_box_pack_start(GTK_BOX(vbox),pd->tsd.bold_radio[1], FALSE, FALSE, 0);
	pd->tsd.italic_radio[0] = gtk_radio_button_new_with_label(NULL, _("normal style"));
	gtk_box_pack_start(GTK_BOX(vbox),pd->tsd.italic_radio[0], FALSE, FALSE, 0);
	pd->tsd.italic_radio[1] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pd->tsd.italic_radio[0]), _("italic style"));
	gtk_box_pack_start(GTK_BOX(vbox),pd->tsd.italic_radio[1], FALSE, FALSE, 0);
	{
		GList *tmplist = g_list_first(pd->lists[textstyles]); /* the order of this list has a meaning !!! */
		while (tmplist) {
			gchar **strarr = (gchar **)tmplist->data;
			if (count_array(strarr)==5) {
				GtkTreeIter iter;
				gtk_list_store_append(GTK_LIST_STORE(pd->tsd.lstore), &iter);
				set_textstyle_strarr_in_list(&iter, strarr,pd);
			} else {
				g_print("invalid textstyle with length %d\n",count_array(strarr));
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
	g_signal_connect(G_OBJECT(pd->tsd.italic_radio[0]),"toggled",G_CALLBACK(textstyle_radio_changed),pd);
	g_signal_connect(G_OBJECT(pd->tsd.italic_radio[1]),"toggled",G_CALLBACK(textstyle_radio_changed),pd);

	hbox2 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox2, FALSE, FALSE, 2);
	but = bf_gtkstock_button(GTK_STOCK_ADD, G_CALLBACK(add_new_textstyle_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox2),but, FALSE, FALSE, 2);
	but = bf_gtkstock_button(GTK_STOCK_DELETE, G_CALLBACK(delete_textstyle_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox2),but, FALSE, FALSE, 2);
}

/*
the highlighting GUI will look like this (only 1st column is visible, 2, 3 and 4 are not rendered):

-|- PHP                    "PHP"          NULL
 |  |- php-comment         "PHP",           NULL, or some gchar **
 |  |- keywords             ... ....
 |  |- tag                 "PHP",       "begin_tag"    NULL, or some gchar **
 |
 |-Python
 |  |

in the liststore, we will have three columns:
	0: the visible name
	1: the language name (bflang->name)
	2: the highlight type as stored in the bflang file
	3: the gchar ** that should be updated if the option is changed
*/
#ifdef OLD_HL_GUI 
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

static gint fill_hl_tree_highlight_list_sort_lcb(gconstpointer a,gconstpointer b) {
	gchar **arra=(gchar **)a, **arrb=(gchar **)b;
	return g_strcmp0(arra[1],arrb[1]);
}

static gint fill_hl_tree_bflang_list_sort_lcb(gconstpointer a,gconstpointer b) {
	return g_strcmp0(((Tbflang *)a)->name, ((Tbflang *)b)->name);
}

static void fill_hl_tree(Tprefdialog *pd) {
	/* To fill the tree quickly, we create the parents (language names), and add the names with the corresponding
	GtkTreeIter to a hashtable. Then we simply walk the GList and lookup the correct GtkTreeIter in the hashtable */
	GHashTable *langiters;
	GList *tmplist = g_list_first(g_list_sort(langmgr_get_languages(), (GCompareFunc)fill_hl_tree_bflang_list_sort_lcb));
	langiters = g_hash_table_new_full(g_str_hash,g_str_equal,NULL,g_free);
	while (tmplist) {
		GtkTreeIter *toplevel = g_new0(GtkTreeIter,1);
		Tbflang* bflang = tmplist->data;
		gtk_tree_store_append(pd->hld.tstore, toplevel, NULL);
		gtk_tree_store_set(pd->hld.tstore, toplevel
					,0,bflang->name,-1);
		g_hash_table_insert(langiters,bflang->name,toplevel);
		tmplist = g_list_next(tmplist);
	}
	tmplist = g_list_first(g_list_sort(pd->lists[highlight_styles], (GCompareFunc)fill_hl_tree_highlight_list_sort_lcb));
	while (tmplist) {
		GtkTreeIter *parent;
		gchar **arr = tmplist->data;
		if (count_array(arr)==3) {
			parent = g_hash_table_lookup(langiters,arr[0]);
			if (parent) {
				GtkTreeIter iter;
				gtk_tree_store_append(pd->hld.tstore, &iter, parent);
				gtk_tree_store_set(pd->hld.tstore, &iter,0,arr[1],1, arr[0],2,arr, -1);
			}
			tmplist = g_list_next(tmplist);
		}
	}
	g_hash_table_unref(langiters);
}

static void hl_set_textstylecombo_by_text(Tprefdialog *pd, const gchar *text) {
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
		if (pd->hld.curstrarr[2]) {
			DEBUG_MSG("free old value %s\n",pd->hld.curstrarr[3]);
			g_free(pd->hld.curstrarr[2]);
		}
		pd->hld.curstrarr[2] = name;
	}
}

static void hl_selection_changed_cb(GtkTreeSelection *selection, Tprefdialog *pd) {
	GtkTreeIter iter;
	GtkTreeModel *model;
	DEBUG_MSG("hl_selection_changed_cb, started\n");
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreeIter parent;
		if (gtk_tree_model_iter_parent (model, &parent, &iter)) {
			gchar *name, *filetype, **strarr;
			gtk_tree_model_get(model, &iter, 0, &name, 1, &filetype, 2, &strarr, -1);
			pd->hld.curstrarr = NULL;
			if (strarr == NULL) {
				/* create the strarr if there is none yet */
				strarr = g_malloc(4*sizeof(gchar *));
				strarr[0] = filetype;
				strarr[1] = name;
				strarr[2] = g_strdup("");
				strarr[3] = NULL;
				gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 2, strarr, -1);
				hl_set_textstylecombo_by_text(pd, NULL);
				pd->lists[highlight_styles] = g_list_prepend(pd->lists[highlight_styles], strarr);
			} else {
				hl_set_textstylecombo_by_text(pd, strarr[2]);
			}
			pd->hld.curstrarr = strarr;
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
	pd->lists[highlight_styles] = duplicate_arraylist(main_v->props.highlight_styles);

	/* new structure: one treestore for all, column 1:visible label, 2:label for config file 3:pointer to strarr */
	pd->hld.tstore = gtk_tree_store_new(3,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER);
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
	gtk_widget_set_size_request(pd->hld.tview, 150, 300);
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

}
#endif /* OLD_HL_GUI */
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
	pref_create_column(GTK_TREE_VIEW(pd->bd.lview), 1, G_CALLBACK(extcommands_0_edited_lcb), pd, _("Label"), 0,FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->bd.lview), 1, G_CALLBACK(extcommands_1_edited_lcb), pd, _("Command"), 1,FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->bd.lview), 2, G_CALLBACK(extcommands_2_edited_lcb), pd, _("Default browser"), 2,FALSE);
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<small><b>Input options</b>\nstart with a | to send the input to the standard input\n%s local filename (available for local files)\n%i temporary fifo for input, equals %s if the document is not modified and local\n%I temporary filename for input, equals %s if the document is not modified and local\n<b>Other options</b>\n%c local directory of file (available for local files)\n%n filename without path (available for all titled files)\n%u URL (available for all titled files)\n%p preview URL if basedir and preview dir are set in project settings, else identical to %u</small>"));
	gtk_box_pack_start(GTK_BOX(vbox1),label, FALSE, FALSE, 2);
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
	gtk_box_pack_start(GTK_BOX(vbox1),hbox, FALSE, FALSE, 2);
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
	pref_create_column(GTK_TREE_VIEW(pd->ed.lview), 1, G_CALLBACK(external_filter_0_edited_lcb), pd, _("Label"), 0,FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->ed.lview), 1, G_CALLBACK(external_filter_1_edited_lcb), pd, _("Command"), 1,FALSE);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<small><b>Input options</b>\nstart with a | to send the input to the standard input\n%s local filename (requires local file, cannot operate on selection)\n%i temporary fifo for input\n%I temporary filename for input\n<b>Output options</b>\nend with a | to read the output from the standard output\n%o temporary fifo\n%O temporary filename\n%t temporary filename for both input and output (for in-place-editing filters, cannot operate on selection)\n<b>Other options</b>\n%c local directory of file (requires local file)\n%n filename without path (available for all titled files)\n%u URL (available for all titled files)\n%p preview URL if basedir and preview dir are set in project settings, else identical to %u</small>"));
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
	gtk_box_pack_start(GTK_BOX(vbox1),hbox, FALSE, FALSE, 2);
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
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 1, G_CALLBACK(outputbox_0_edited_lcb), pd, _("Name"), 0,TRUE);
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 1, G_CALLBACK(outputbox_1_edited_lcb), pd, _("Pattern"), 1,TRUE);
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 1, G_CALLBACK(outputbox_2_edited_lcb), pd, _("File #"), 2,TRUE);
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 1, G_CALLBACK(outputbox_3_edited_lcb), pd, _("Line #"), 3,TRUE);
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 1, G_CALLBACK(outputbox_4_edited_lcb), pd, _("Output #"), 4,TRUE);
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 1, G_CALLBACK(outputbox_5_edited_lcb), pd, _("Command"), 5,TRUE);
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 2, G_CALLBACK(outputbox_6_toggled_lcb), pd, _("Show all output"), 6,TRUE);
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<small><b>Input options</b>\nstart with a | to send the input to the standard input\n%s local filename (available for local files)\n%i temporary fifo for input, equals %s if the document is not modified and local\n%I temporary filename for input, equals %s if the document is not modified and local\n<b>Output options</b>\nend with a | to read the output from the standard output\n%o temporary fifo\n%O temporary filename\n%t temporary filename for both input and output (for in-place-editing filters)\n<b>Other options</b>\n%c local directory of file (available for local files)\n%n filename without path (available for all titled files)\n%u URL (available for all titled files)\n%p preview URL if basedir and preview dir are set in project settings, else identical to %u</small>"));
	gtk_box_pack_start(GTK_BOX(vbox1),label, FALSE, FALSE, 2);
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
	gtk_box_pack_start(GTK_BOX(vbox1),hbox, FALSE, FALSE, 2);
	but = bf_gtkstock_button(GTK_STOCK_ADD, G_CALLBACK(add_new_outputbox_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox),but, FALSE, FALSE, 2);
	but = bf_gtkstock_button(GTK_STOCK_DELETE, G_CALLBACK(delete_outputbox_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox),but, FALSE, FALSE, 2);
}

/********* bflangdialog */
static gchar *string_path_to_child_string_path(GtkTreeModelFilter *lfilter, const gchar *path) {
	GtkTreePath* tpath, *ctpath;
	gchar *ret;
	tpath = gtk_tree_path_new_from_string(path);
	if (!tpath) {
		g_print("no tree path for string path %s\n",path);
		return NULL;
	}
	ctpath = gtk_tree_model_filter_convert_path_to_child_path(lfilter,tpath);
	gtk_tree_path_free(tpath);
	if (!ctpath) {
		g_print("no child path for string path %s\n",path);
		return NULL;
	}
	ret = gtk_tree_path_to_string(ctpath);
	gtk_tree_path_free(ctpath);
	g_print("return child path %s for path %s\n",ret,path);
	return ret;
}

static void bflang_1_edited_lcb(GtkCellRendererToggle *cellrenderertoggle,gchar *path,Tprefdialog *pd) {
	gchar *cpath, *val = g_strdup(cellrenderertoggle->active ? "0" : "1");
	cpath = string_path_to_child_string_path(pd->bld.lfilter, path);
	/*	BUG FIX THIS, a treemodelfilter is inbetween so path is not the path that can be used on the model!!!!!!!! */
	pref_apply_change(pd->bld.lstore,3,2,cpath,val,2);
	g_free(val);
	g_free(cpath);
}
/*static void bflang_highlight_changed_lcb(GtkCellRendererCombo *combo,gchar *path,GtkTreeIter *new_iter,Tprefdialog *pd) {
	gchar *val;
	gtk_tree_model_get(pd->tsd.lstore, new_iter, 0, &val, -1);
	g_print("bflang_highlight_changed_lcb, val=%s\n",val);
	pref_apply_change(pd->bld.lstore2, 3, 4, path, val, 2);
	g_free(val);
}*/

static void bflang_highlight_edited_lcb(GtkCellRendererCombo *combo,gchar *path,gchar *val,Tprefdialog *pd) {
	gchar *cpath = string_path_to_child_string_path(pd->bld.lfilter2, path);
	g_print("bflang_highlight_edited_lcb, val=%s\n",val);
	pref_apply_change(pd->bld.lstore2, 3, 1/* 1 is text, not a combo */, cpath, val, 2);
	g_free(cpath);
}

static void bflanggui_set_bflang(Tprefdialog *pd, gpointer data) {
	Tbflang *bflang=data;
	pd->bld.curbflang = bflang;
	gtk_tree_model_filter_refilter(pd->bld.lfilter);
	gtk_tree_model_filter_refilter(pd->bld.lfilter2);
}
static gboolean bflang_gui_filter_func_lcb(GtkTreeModel *model,GtkTreeIter *iter,gpointer data) {
	gchar *name;
	Tprefdialog *pd =data;
	gboolean visible=FALSE;
	gtk_tree_model_get(model, iter, 0, &name, -1);
	if (name && pd->bld.curbflang && strcmp(name, pd->bld.curbflang->name) == 0)
		visible = TRUE;
	g_free (name);
	return visible;
}
static void fill_bflang_gui(Tprefdialog *pd) {
	GList *tmplist = g_list_first(pd->lists[bflang_options]);
	while (tmplist) {
		gchar **strarr = (gchar **)tmplist->data;
		if (count_array(strarr)==3) {
			GtkTreeIter iter;
			gtk_list_store_append(GTK_LIST_STORE(pd->bld.lstore), &iter);
			gtk_list_store_set(GTK_LIST_STORE(pd->bld.lstore), &iter
				,0,strarr[0],1,strarr[1],2,strarr[2][0]=='1',3,strarr,-1);
		}
		tmplist = g_list_next(tmplist);
	}
	
	tmplist = g_list_first(pd->lists[highlight_styles]);
	while (tmplist) {
		gchar **strarr = (gchar **)tmplist->data;
		/*g_print("found %s:%s:%s\n",strarr[0],strarr[1],strarr[2]);*/
		if (count_array(strarr)==3) {
			GtkTreeIter iter;
			gtk_list_store_append(GTK_LIST_STORE(pd->bld.lstore2), &iter);
			gtk_list_store_set(GTK_LIST_STORE(pd->bld.lstore2), &iter
				,0,strarr[0],1,strarr[1],2,strarr[2],3,strarr,-1);
		}
		tmplist = g_list_next(tmplist);
	}
}

static void create_bflang_gui(Tprefdialog *pd, GtkWidget *vbox1) {
	GtkWidget *label,*scrolwin;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	
	pd->lists[bflang_options] = duplicate_arraylist(main_v->props.bflang_options);
	pd->lists[highlight_styles] = duplicate_arraylist(main_v->props.highlight_styles);
	
	pd->bld.lstore = gtk_list_store_new (4,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_BOOLEAN,G_TYPE_POINTER);
	pd->bld.lfilter = gtk_tree_model_filter_new(pd->bld.lstore,NULL);
	gtk_tree_model_filter_set_visible_func(pd->bld.lfilter,bflang_gui_filter_func_lcb,pd,NULL);
	pd->bld.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->bld.lfilter));
	
	/*label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<small>language dialog not yet ready..</small>"));
	gtk_box_pack_start(GTK_BOX(vbox1),label, TRUE, TRUE, 2);*/
	/* skip column 0, the language name */
	pref_create_column(GTK_TREE_VIEW(pd->bld.lview), 1/* 1=text */, NULL, NULL, _("Option name"), 1,FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->bld.lview), 2 /* 2=boolean */, G_CALLBACK(bflang_1_edited_lcb), pd, _("value"), 2,FALSE);
	
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), pd->bld.lview);
	
	gtk_box_pack_start(GTK_BOX(vbox1),scrolwin, TRUE, TRUE, 2);
	
	pd->bld.lstore2 = gtk_list_store_new (4,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER);
	pd->bld.lfilter2 = gtk_tree_model_filter_new(pd->bld.lstore2,NULL);
	gtk_tree_model_filter_set_visible_func(pd->bld.lfilter2,bflang_gui_filter_func_lcb,pd,NULL);
	pd->bld.lview2 = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->bld.lfilter2));
	
	pref_create_column(GTK_TREE_VIEW(pd->bld.lview2), 1/* 1=text */, NULL, NULL, _("Highlight name"), 1,FALSE);
	renderer = gtk_cell_renderer_combo_new();
	g_object_set(G_OBJECT(renderer), "model", pd->tsd.lstore/* the textstyle model */, "text-column", 0, "has-entry", FALSE, "editable", TRUE, NULL);
	column = gtk_tree_view_column_new_with_attributes(_("Textstyle"), renderer, "text",2, NULL);
	gtk_tree_view_column_set_clickable(GTK_TREE_VIEW_COLUMN(column),TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(pd->bld.lview2), column);
	g_signal_connect(G_OBJECT(renderer), "edited", bflang_highlight_edited_lcb, pd);
	/*g_signal_connect(G_OBJECT(renderer), "changed", bflang_highlight_changed_lcb, pd);*/

	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), pd->bld.lview2);
	
	gtk_box_pack_start(GTK_BOX(vbox1),scrolwin, TRUE, TRUE, 2);
	
	fill_bflang_gui(pd);
}

/**************************************/
/* MAIN DIALOG FUNCTIONS              */
/**************************************/

static void preferences_destroy_lcb(GtkWidget * widget, Tprefdialog *pd) {
	GtkTreeSelection *select;
	DEBUG_MSG("preferences_destroy_lcb, started\n");

	free_arraylist(pd->lists[textstyles]);
	free_arraylist(pd->lists[highlight_styles]);
	free_arraylist(pd->lists[bflang_options]);
	pd->lists[highlight_styles] = NULL;
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
	main_v->prefdialog = NULL;
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
	integer_apply(&main_v->props.view_mbhl, pd->prefs[view_mbhl], TRUE);
	integer_apply(&main_v->props.view_cline, pd->prefs[view_cline], TRUE);
	string_apply(&main_v->props.editor_fg, pd->prefs[editor_fg]);
	string_apply(&main_v->props.editor_bg, pd->prefs[editor_bg]);
	integer_apply(&main_v->props.defaulthighlight, pd->prefs[defaulthighlight], TRUE);

	integer_apply(&main_v->props.xhtml, pd->prefs[xhtml], TRUE);
	if (main_v->props.xhtml) {
		main_v->props.lowercase_tags = 1;
		main_v->props.allow_dep = 0;
	} else {
		integer_apply(&main_v->props.lowercase_tags, pd->prefs[lowercase_tags], TRUE);
		integer_apply(&main_v->props.allow_dep, pd->prefs[allow_dep], TRUE);
	}
	integer_apply(&main_v->props.insert_close_tag, pd->prefs[insert_close_tag], TRUE);
	integer_apply(&main_v->props.close_tag_newline, pd->prefs[close_tag_newline], TRUE);
	integer_apply(&main_v->props.auto_update_meta_author, pd->prefs[auto_update_meta_author], TRUE);
	integer_apply(&main_v->props.auto_update_meta_date, pd->prefs[auto_update_meta_date], TRUE);
	integer_apply(&main_v->props.auto_update_meta_generator, pd->prefs[auto_update_meta_generator], TRUE);

	string_apply(&main_v->props.newfile_default_encoding, GTK_COMBO(pd->prefs[newfile_default_encoding])->entry);
	integer_apply(&main_v->props.auto_set_encoding_meta, pd->prefs[auto_set_encoding_meta], TRUE);
	integer_apply(&main_v->props.backup_file, pd->prefs[backup_file], TRUE);
/*	string_apply(&main_v->props.backup_suffix, pd->prefs[backup_suffix]);
	string_apply(&main_v->props.backup_prefix, pd->prefs[backup_prefix]);*/
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
	integer_apply(&main_v->props.leave_to_window_manager, pd->prefs[leave_to_window_manager], TRUE);
	integer_apply(&main_v->props.restore_dimensions, pd->prefs[restore_dimensions], TRUE);
	if (!main_v->props.restore_dimensions) {
		integer_apply(&main_v->globses.left_panel_width, pd->prefs[left_panel_width], FALSE);
		integer_apply(&main_v->globses.main_window_h, pd->prefs[main_window_h], FALSE);
		integer_apply(&main_v->globses.main_window_w, pd->prefs[main_window_w], FALSE);
	}
	string_apply(&main_v->props.tab_font_string, pd->prefs[tab_font_string]);
	main_v->props.document_tabposition = gtk_option_menu_get_history(GTK_OPTION_MENU(pd->prefs[document_tabposition]));
	integer_apply(&main_v->props.switch_tabs_by_altx, pd->prefs[switch_tabs_by_altx], TRUE);
	main_v->props.leftpanel_tabposition = gtk_option_menu_get_history(GTK_OPTION_MENU(pd->prefs[leftpanel_tabposition]));

	integer_apply(&main_v->props.transient_htdialogs, pd->prefs[transient_htdialogs], TRUE);

	string_apply(&main_v->props.image_thumbnailstring, pd->prefs[image_thumbnailstring]);
	string_apply(&main_v->props.image_thumbnailtype, GTK_COMBO(pd->prefs[image_thumbnailtype])->entry);
	
	main_v->props.autocomp_popup_mode = gtk_option_menu_get_history(GTK_OPTION_MENU(pd->prefs[autocomp_popup_mode]));
	integer_apply(&main_v->props.load_reference, pd->prefs[load_reference], TRUE);
	integer_apply(&main_v->props.show_autocomp_reference, pd->prefs[show_autocomp_reference], TRUE);
	integer_apply(&main_v->props.show_tooltip_reference, pd->prefs[show_tooltip_reference], TRUE);

	free_arraylist(main_v->props.plugin_config);
	main_v->props.plugin_config = duplicate_arraylist(pd->lists[pluginconfig]);

	free_arraylist(main_v->props.external_command);
	main_v->props.external_command = duplicate_arraylist(pd->lists[extcommands]);

	free_arraylist(main_v->props.external_filter);
	main_v->props.external_filter = duplicate_arraylist(pd->lists[extfilters]);

	free_arraylist(main_v->props.external_outputbox);
	main_v->props.external_outputbox = duplicate_arraylist(pd->lists[extoutputbox]);

	DEBUG_MSG("preferences_apply: free old textstyles, and building new list\n");
	free_arraylist(main_v->props.textstyles);
	main_v->props.textstyles = duplicate_arraylist(pd->lists[textstyles]);
	free_arraylist(main_v->props.highlight_styles);
	main_v->props.highlight_styles = duplicate_arraylist(pd->lists[highlight_styles]);

	free_arraylist(main_v->props.bflang_options);
	main_v->props.bflang_options = duplicate_arraylist(pd->lists[bflang_options]);

	/* apply the changes to highlighting patterns and filetypes to the running program */
	langmgr_reload_user_options();
	langmgr_reload_user_styles();
	langmgr_reload_user_highlights();

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
	if (togglebutton == GTK_TOGGLE_BUTTON(pd->prefs[restore_dimensions])) {
		gtk_widget_set_sensitive(pd->prefs[left_panel_width], !togglebutton->active);
		gtk_widget_set_sensitive(pd->prefs[main_window_h], !togglebutton->active);
		gtk_widget_set_sensitive(pd->prefs[main_window_w], !togglebutton->active);
	} else if (togglebutton == GTK_TOGGLE_BUTTON(pd->prefs[leave_to_window_manager])) {
		gtk_widget_set_sensitive(pd->prefs[restore_dimensions], !togglebutton->active);

		gtk_widget_set_sensitive(pd->prefs[left_panel_width], !togglebutton->active && !GTK_TOGGLE_BUTTON(pd->prefs[restore_dimensions])->active);
		gtk_widget_set_sensitive(pd->prefs[main_window_h], !togglebutton->active && !GTK_TOGGLE_BUTTON(pd->prefs[restore_dimensions])->active);
		gtk_widget_set_sensitive(pd->prefs[main_window_w], !togglebutton->active && !GTK_TOGGLE_BUTTON(pd->prefs[restore_dimensions])->active);
	} else {
		g_print("restore_dimensions_toggled_lcb, unknown togglebutton\n");
	}
}
static void create_backup_toggled_lcb(GtkToggleButton *togglebutton,Tprefdialog *pd) {
/*    gtk_widget_set_sensitive(pd->prefs[backup_prefix], togglebutton->active);
    gtk_widget_set_sensitive(pd->prefs[backup_suffix], togglebutton->active);*/
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
		gpointer data=NULL;
		void (*func) ();
		gtk_tree_model_get_iter(GTK_TREE_MODEL(pd->nstore),&iter,path);
		gtk_tree_model_get(GTK_TREE_MODEL(pd->nstore),&iter,WIDGETCOL,&child,FUNCCOL,&func,DATACOL,&data,-1);
		if ( child ) {
			gtk_box_pack_start(GTK_BOX(pd->fixed),child,TRUE,TRUE,1);
			gtk_widget_show_all(pd->fixed);
			if (func && data) {
				func(pd,data);
			}
		}
		gtk_tree_path_free(path);
	}
}

static void preferences_dialog() {
	Tprefdialog *pd;
	GtkWidget *dvbox, *frame, *vbox1, *vbox2;
	gchar *notebooktabpositions[] = {N_("left"), N_("right"), N_("top"), N_("bottom"), NULL};
	gchar *panellocations[] = {N_("right"), N_("left"), NULL};
	gchar *modified_check_types[] = {N_("Nothing"), N_("Modified time and file size"), N_("Modified time"), N_("File size"), NULL};
	GtkWidget *dhbox, *scrolwin;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	GtkTreeIter auxit,iter;
	GtkTreePath *path;
	GList *tmplist,*poplist;
	
	if (main_v->prefdialog) {
		pd = (Tprefdialog *)main_v->prefdialog;
		/* bring window to focus ?? */
		gtk_window_present(GTK_WINDOW(pd->win));
		return;
	}

	main_v->prefdialog = pd = g_new0(Tprefdialog,1);
	pd->win = window_full(_("Edit preferences"), GTK_WIN_POS_CENTER, 0, G_CALLBACK(preferences_destroy_lcb), pd, TRUE);

	dvbox = gtk_vbox_new(FALSE, 5);
	dhbox = gtk_hbox_new(FALSE, 5);
	pd->fixed = gtk_hbox_new(FALSE,5);
	pd->nstore = gtk_tree_store_new(4,G_TYPE_STRING,G_TYPE_POINTER,G_TYPE_POINTER,G_TYPE_POINTER);
	pd->noteb = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->nstore));
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), pd->noteb);
	gtk_box_pack_start(GTK_BOX(dhbox), scrolwin, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(dhbox), pd->fixed, TRUE, TRUE, 5);
	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", cell, "text", NAMECOL, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(pd->noteb), column);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(pd->noteb), FALSE);
	gtk_box_pack_start(GTK_BOX(dvbox), dhbox, TRUE, TRUE, 5);
	gtk_container_add(GTK_CONTAINER(pd->win), dvbox);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Editor options"), WIDGETCOL,vbox1,-1);

	frame = gtk_frame_new(_("Generic editor options"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	pd->prefs[editor_font_string] = prefs_string(_("Font"), main_v->props.editor_font_string, vbox2, pd, string_font);
	pd->prefs[editor_fg] = prefs_string(_("Foreground color"), main_v->props.editor_fg, vbox2, pd, string_color);
	pd->prefs[editor_bg] = prefs_string(_("Background color"), main_v->props.editor_bg, vbox2, pd, string_color);
	pd->prefs[editor_smart_cursor] = boxed_checkbut_with_value(_("Smart Home/End cursor positioning"), main_v->props.editor_smart_cursor, vbox2);
	pd->prefs[editor_indent_wspaces] = boxed_checkbut_with_value(_("Use spaces to indent, not tabs"), main_v->props.editor_indent_wspaces, vbox2);

	pd->prefs[num_undo_levels] = prefs_integer(_("Number of actions in undo history"), main_v->props.num_undo_levels, vbox2, pd, 50, 10000);
	pd->prefs[clear_undo_on_save] = boxed_checkbut_with_value(_("Clear undo history on save"), main_v->props.clear_undo_on_save, vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Editor defaults"), WIDGETCOL,vbox1,-1);


	frame = gtk_frame_new(_("Default values"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	pd->prefs[editor_tab_width] = prefs_integer(_("Tab width"), main_v->props.editor_tab_width, vbox2, pd, 1, 50);
	pd->prefs[word_wrap] = boxed_checkbut_with_value(_("Word wrap"), main_v->props.word_wrap, vbox2);
	pd->prefs[view_line_numbers] = boxed_checkbut_with_value(_("Show line numbers"), main_v->props.view_line_numbers, vbox2);
	
	pd->prefs[defaulthighlight] = boxed_checkbut_with_value(_("Highlight syntax"), main_v->props.defaulthighlight, vbox2);
	pd->prefs[view_cline] = boxed_checkbut_with_value(_("Highlight current line"), main_v->props.view_cline, vbox2);
	pd->prefs[view_blocks] = boxed_checkbut_with_value(_("Enable block folding"), main_v->props.view_blocks, vbox2);
	pd->prefs[autocomplete] = boxed_checkbut_with_value(_("Enable automatic completion pop-up"), main_v->props.autocomplete, vbox2);
	pd->prefs[view_mbhl] = boxed_checkbut_with_value(_("Highlight matching block begin-end"), main_v->props.view_mbhl, vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);

	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("HTML"), WIDGETCOL,vbox1,-1);

	frame = gtk_frame_new(_("HTML options"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	pd->prefs[lowercase_tags] = boxed_checkbut_with_value(_("Lowercase HTML tags"), main_v->props.lowercase_tags, vbox2);
	pd->prefs[allow_dep] = boxed_checkbut_with_value(_("Use deprecated tags (e.g. <font> and <nobr>)"), main_v->props.allow_dep, vbox2);
	pd->prefs[xhtml] = boxed_checkbut_with_value(_("Use XHTML style tags (<br />)"), main_v->props.xhtml, vbox2);
	pd->prefs[insert_close_tag] = boxed_checkbut_with_value(_("Automatically close tags"), main_v->props.insert_close_tag, vbox2);
	pd->prefs[close_tag_newline] = boxed_checkbut_with_value(_("Prepend close tags with a newline"), main_v->props.close_tag_newline, vbox2);

	pd->prefs[auto_update_meta_author] = boxed_checkbut_with_value(_("Automatically update author meta tag"), main_v->props.auto_update_meta_author, vbox2);
	pd->prefs[auto_update_meta_date] = boxed_checkbut_with_value(_("Automatically update date meta tag"), main_v->props.auto_update_meta_date, vbox2);
	pd->prefs[auto_update_meta_generator] = boxed_checkbut_with_value(_("Automatically update generator meta tag"), main_v->props.auto_update_meta_generator, vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Files"), WIDGETCOL,vbox1,-1);

	frame = gtk_frame_new(_("Encoding"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	poplist = NULL;
	tmplist = g_list_first(main_v->globses.encodings);
	while (tmplist) {
		gchar **strarr = (gchar **)tmplist->data;
		poplist = g_list_append(poplist, strarr[1]);
		tmplist = g_list_next(tmplist);
	}
	pd->prefs[newfile_default_encoding] = prefs_combo(_("Default character set for new files"),main_v->props.newfile_default_encoding, vbox2, pd, poplist, TRUE);
	g_list_free(poplist);

	pd->prefs[auto_set_encoding_meta] = boxed_checkbut_with_value(_("Auto set <meta> HTML tag on encoding change"), main_v->props.auto_set_encoding_meta, vbox2);

	frame = gtk_frame_new(_("Backup"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	pd->prefs[backup_file] = boxed_checkbut_with_value(_("Create backup file during file save"), main_v->props.backup_file, vbox2);
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

#ifndef NOSPLASH
	frame = gtk_frame_new(_("Startup options"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	pd->prefs[editor_show_splash_screen] = boxed_checkbut_with_value(_("Show splash-screen"), main_v->props.show_splash_screen, vbox2);
#endif /* #ifndef NOSPLASH */

   frame = gtk_frame_new(_("General"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	pd->prefs[transient_htdialogs] = boxed_checkbut_with_value(_("Make HTML dialogs transient"), main_v->props.transient_htdialogs, vbox2);
	pd->prefs[tab_font_string] = prefs_string(_("Notebook tab font \n(leave empty for gtk default)"), main_v->props.tab_font_string, vbox2, pd, string_font);
	pd->prefs[document_tabposition] = boxed_optionmenu_with_value(_("Document notebook tab position"), main_v->props.document_tabposition, vbox2, notebooktabpositions);
	pd->prefs[switch_tabs_by_altx] = boxed_checkbut_with_value(_("Switch between tabs with <Alt>+0..9"), main_v->props.switch_tabs_by_altx, vbox2);
	pd->prefs[leftpanel_tabposition] = boxed_optionmenu_with_value(_("Sidebar notebook tab position"), main_v->props.leftpanel_tabposition, vbox2, notebooktabpositions);
	pd->prefs[left_panel_left] = boxed_optionmenu_with_value(_("Sidebar location"), main_v->props.left_panel_left, vbox2, panellocations);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, &iter);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Dimensions"), WIDGETCOL,vbox1,-1);

	frame = gtk_frame_new(_("Dimensions"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	pd->prefs[leave_to_window_manager] = boxed_checkbut_with_value(_("Leave dimensions to window manager"), main_v->props.leave_to_window_manager, vbox2);
	pd->prefs[restore_dimensions] = boxed_checkbut_with_value(_("Restore last used dimensions"), main_v->props.restore_dimensions, vbox2);
	pd->prefs[left_panel_width] = prefs_integer(_("Initial sidebar width"), main_v->globses.left_panel_width, vbox2, pd, 1, 4000);
	pd->prefs[main_window_h] = prefs_integer(_("Initial window height"), main_v->globses.main_window_h, vbox2, pd, 1, 4000);
	pd->prefs[main_window_w] = prefs_integer(_("Initial window width"), main_v->globses.main_window_w, vbox2, pd, 1, 4000);
	restore_dimensions_toggled_lcb(GTK_TOGGLE_BUTTON(pd->prefs[restore_dimensions]), pd);
	g_signal_connect(G_OBJECT(pd->prefs[restore_dimensions]), "toggled", G_CALLBACK(restore_dimensions_toggled_lcb), pd);
	g_signal_connect(G_OBJECT(pd->prefs[leave_to_window_manager]), "toggled", G_CALLBACK(restore_dimensions_toggled_lcb), pd);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, &iter);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("File and directory view"), WIDGETCOL,vbox1,-1);

	frame = gtk_frame_new(_("File and directory view"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Images"), WIDGETCOL,vbox1,-1);

	frame = gtk_frame_new(_("Thumbnails"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	pd->prefs[image_thumbnailstring] = prefs_string(_("Thumbnail suffix"), main_v->props.image_thumbnailstring, vbox2, pd, string_none);
	poplist = g_list_append(NULL, "png");
	poplist = g_list_append(poplist, "jpeg");
	pd->prefs[image_thumbnailtype] = prefs_combo(_("Thumbnail filetype"),main_v->props.image_thumbnailtype, vbox2, pd, poplist, FALSE);
	g_list_free(poplist);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("External commands"), WIDGETCOL,vbox1,-1);

	frame = gtk_frame_new(_("Commands"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, TRUE, TRUE, 5);
	vbox2 = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	create_extcommands_gui(pd, vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("External filters"), WIDGETCOL,vbox1,-1);

	frame = gtk_frame_new(_("Filters"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, TRUE, TRUE, 5);
	vbox2 = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	create_filters_gui(pd, vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Output parsers"), WIDGETCOL,vbox1,-1);

	frame = gtk_frame_new(_("Output parsers"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, TRUE, TRUE, 5);
	vbox2 = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	create_outputbox_gui(pd, vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Plugins"), WIDGETCOL,vbox1,-1);
	frame = gtk_frame_new(_("Plugins"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, TRUE, TRUE, 5);
	vbox2 = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	create_plugin_gui(pd, vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Text styles"), WIDGETCOL,vbox1,-1);
	frame = gtk_frame_new(_("Text styles"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, TRUE, TRUE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	create_textstyle_gui(pd,vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);

	gtk_tree_store_append(pd->nstore, &iter, NULL);
	gtk_tree_store_set(pd->nstore, &iter, NAMECOL,_("Language support"), WIDGETCOL,vbox1,-1);

	frame = gtk_frame_new(_("Advanced syntax scanning options"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	{
		gchar *autocompmodes[] = {N_("Delayed"),N_("Immediately"), NULL};
		pd->prefs[autocomp_popup_mode] = boxed_optionmenu_with_value(_("Show the automatic completion pop-up window"), main_v->props.autocomp_popup_mode, vbox2, autocompmodes);
	}
	pd->prefs[load_reference] = boxed_checkbut_with_value(_("Load reference information from language file"), main_v->props.load_reference, vbox2);
	pd->prefs[show_autocomp_reference] = boxed_checkbut_with_value(_("Show reference information in autocompletion pop-up"), main_v->props.show_autocomp_reference, vbox2);
	pd->prefs[show_tooltip_reference] = boxed_checkbut_with_value(_("Show reference information in tooltip window"), main_v->props.show_tooltip_reference, vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);
	create_bflang_gui(pd, vbox1);

	tmplist = g_list_first(langmgr_get_languages());
	while (tmplist) {
		Tbflang *bflang = tmplist->data;
		gtk_tree_store_append(pd->nstore, &auxit, &iter);
		gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,bflang->name, WIDGETCOL,vbox1, FUNCCOL, bflanggui_set_bflang, DATACOL, bflang,-1);
		tmplist = g_list_next(tmplist);
	}
	/*
	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_tree_store_append(pd->nstore, &auxit, &iter);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL,_("Syntax highlighting"), WIDGETCOL,vbox1,-1);

	frame = gtk_frame_new(_("Syntax highlighting"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	create_hl_gui(pd,vbox2);*/

	/* end, create buttons for dialog now */
	{
		GtkWidget *ahbox, *but;
		ahbox = gtk_hbutton_box_new();
		gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
		gtk_button_box_set_spacing(GTK_BUTTON_BOX(ahbox), 6);

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
