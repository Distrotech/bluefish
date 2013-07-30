/* Bluefish HTML Editor
 * preferences.c - the preferences code
 *
 * Copyright (C) 2002-2012 Olivier Sessink
 * Copyright (C) 2010-2011 James Hayward
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

/*#define DEBUG*/

#include <gtk/gtk.h>
#include <string.h>				/* strcmp() */
#include <stdlib.h>	/* atoi () */
#include "bluefish.h"

#ifdef MAC_INTEGRATION
/*#include <ige-mac-integration.h>*/
#include <gtkosxapplication.h>
#endif

#include "preferences.h"
#include "bf_lib.h"				/* list_switch_order() */
#include "bftextview2_langmgr.h"
#include "bfwin.h"
#include "bfwin_uimanager.h"
#include "dialog_utils.h"
#include "document.h"
#include "filebrowser2.h"
#include "file_dialogs.h"
#include "gtk_easy.h"
#include "pixmap.h"
#include "rcfile.h"
#include "file_autosave.h"
#include "stringlist.h"			/* duplicate_arraylist */
#include "languages.h"

enum {
	do_periodic_check,
	editor_font_string,			/* editor font */
	editor_smart_cursor,
	editor_tab_indent_sel,
	editor_auto_close_brackets,
	use_system_tab_font,
	max_shown_filename_len,
	tab_font_string,			/* notebook tabs font */
	/*tab_color_normal, *//* notebook tabs text color normal.  This is just NULL! */
	tab_color_modified,			/* tab text color when doc is modified and unsaved */
	tab_color_loading,			/* tab text color when doc is loading */
	tab_color_error,			/* tab text color when doc has errors */
	visible_ws_mode,
	/*defaulthighlight, *//* highlight documents by default */
	leave_to_window_manager,
	restore_dimensions,
	left_panel_left,
	hide_bars_on_fullscreen,
	cursor_size,
	highlight_cursor,
	save_accelmap,
	max_recent_files,			/* length of Open Recent list */
	max_dir_history,			/* length of directory history */
	backup_file,				/* wheather to use a backup file */
	show_long_line_warning,
	backup_abort_action,		/* if the backup fails, continue 'save', 'abort' save, or 'ask' user */
	backup_cleanuponclose,		/* remove the backupfile after close ? */
	image_thumbnailstring,		/* string to append to thumbnail filenames */
	image_thumbnailtype,		/* fileformat to use for thumbnails, "jpeg" or "png" can be handled by gdkpixbuf */
	modified_check_type,		/* 0=no check, 1=by mtime and size, 2=by mtime, 3=by size, 4,5,...not implemented (md5sum?) */
	num_undo_levels,			/* number of undo levels per document */
	clear_undo_on_save,		/* clear all undo information on file save */
	newfile_default_encoding,	/* if you open a new file, what encoding will it use */
	auto_set_encoding_meta,		/* auto set metatag for the encoding */
	strip_trailing_spaces_on_save,
	max_window_title,
	document_tabposition,
	leftpanel_tabposition,
	switch_tabs_by_altx,		/* switch tabs using Alt+X (#385860) */
	/* not yet in use */
	smartindent,
#ifndef WIN32
	open_in_running_bluefish,	/* open commandline documents in already running session */
	open_in_new_window,
#endif							/* ifndef WIN32 */
	register_recent_mode,
	recent_means_recently_closed,
	autocomp_accel_string,
	load_reference,
	show_autocomp_reference,
	show_tooltip_reference,
	delay_full_scan,
	autocomp_popup_mode,
	autocomp_min_prefix_len,
	reduced_scan_triggers,
	use_system_colors,
	editor_fg,
	editor_bg,
	cline_bg,
	visible_ws,
	cursor_color,
	selected_color,
	cursor_highlight_color,
	/* now the entries in globses */
	left_panel_width,
	main_window_h,
	main_window_w,
	autosave,
	autosave_time,
	language,
	right_margin_pos,
	wrap_on_right_margin,
	property_num_max
};

#define FILETYPES_ARRAY_LEN 8
enum {
	extcommands,
	extfilters,
	extoutputbox,
#ifdef OLDTEMPLATES
	templates,
#endif
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
	GtkWidget *bg_color, *fg_color;
	GtkWidget *bold_radio[2];
	GtkWidget *italic_radio[2];
#ifdef HAVE_LIBENCHANT
	GtkWidget *need_spellcheck;
#endif
} Ttextstylepref;

typedef struct {
	GtkTreeStore *tstore;
	GtkWidget *tview;
	GtkWidget *textstyle;
	GtkListStore *cstore;
	gchar **curstrarr;
} Thldialog;

typedef struct {
	GtkListStore *lstore;
	GtkWidget *lview;
} Tplugindialog;

typedef struct {
	GtkWidget *title;
	GtkListStore *lstore;
	GtkTreeModelFilter *lfilter;
	GtkWidget *lview;
	GtkListStore *lstore2;
	GtkTreeModelFilter *lfilter2;
	GtkWidget *lview2;
	Tbflang *curbflang;
} Tbflangpref;

typedef struct {
	GtkWidget *prefs[property_num_max];
	GList *lists[lists_num_max];
	Tsessionprefs sprefs;
	GtkWidget *win;
	GtkWidget *curchild;
	GSList *widgetfreelist;
	GtkWidget *noteb;
	GtkTreeStore *nstore;
	GtkWidget *fixed;
	Tlistpref ftd;				/* FileTypeDialog */
	Tlistpref ffd;				/* FileFilterDialog */
	Tlistpref tg;				/* template gui */
	Ttextstylepref tsd;			/* TextStyleDialog */
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

typedef enum {
	prefapply_escapedtext,
	prefapply_text,
	prefapply_toggle,
	prefapply_bit1toggle,
	prefapply_escapedtext_w_userdefined,
	prefapply_text_w_userdefined,
	prefapply_toggle_w_userdefined,
} Tprefapplytype;

void
pref_click_column(GtkTreeViewColumn * treeviewcolumn, gpointer user_data)
{
	GtkToggleButton *but = GTK_TOGGLE_BUTTON(user_data);
	GList *lst = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(treeviewcolumn));
	if (gtk_toggle_button_get_active(but)) {
		gtk_toggle_button_set_active(but, FALSE);
		gtk_button_set_label(GTK_BUTTON(but), "");
		gtk_tree_view_column_set_sizing(treeviewcolumn, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_fixed_width(treeviewcolumn, 30);
		if (lst) {
			g_object_set(G_OBJECT(lst->data), "sensitive", FALSE, "mode", GTK_CELL_RENDERER_MODE_INERT, NULL);
			g_list_free(lst);
		}
	} else {
		gtk_toggle_button_set_active(but, TRUE);
		gtk_button_set_label(GTK_BUTTON(but), (gchar *) g_object_get_data(G_OBJECT(but), "_title_"));
		gtk_tree_view_column_set_sizing(treeviewcolumn, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		if (lst) {
			g_object_set(G_OBJECT(lst->data), "sensitive", TRUE, "mode", GTK_CELL_RENDERER_MODE_EDITABLE,
						 NULL);
			g_list_free(lst);
		}
	}
}

/* type 0/1=text, 2=toggle,3=radio, 4=non-editable combo */
static GtkCellRenderer *
pref_create_column(GtkTreeView * treeview, gint type, GCallback func, gpointer data,
				   const gchar * title, gint num, gboolean collumn_hide_but)
{
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
			gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(renderer), TRUE);
		}
		if (func) {
			g_object_set(G_OBJECT(renderer), "activatable", TRUE, NULL);
			g_signal_connect(G_OBJECT(renderer), "toggled", func, data);
		}
	} else {					/* if (type ==4) */

		renderer = gtk_cell_renderer_combo_new();
		g_object_set(G_OBJECT(renderer), "has-entry", FALSE, NULL);
		/* should be done by the calling function: g_object_set(G_OBJECT(renderer), "model", model, NULL); */
		g_object_set(G_OBJECT(renderer), "text-column", 0, NULL);
		if (func) {
			g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
			g_signal_connect(G_OBJECT(renderer), "edited", func, data);
		}
	}
	column =
		gtk_tree_view_column_new_with_attributes(title, renderer, (type == 1) ? "text" : "active", num, NULL);
	gtk_tree_view_column_set_clickable(GTK_TREE_VIEW_COLUMN(column), TRUE);
	if (collumn_hide_but) {
		but = gtk_check_button_new_with_label(title);
		g_object_set_data(G_OBJECT(but), "_title_", g_strdup(title));
		gtk_widget_show(but);
		gtk_tree_view_column_set_widget(GTK_TREE_VIEW_COLUMN(column), but);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(but), TRUE);
		g_signal_connect(G_OBJECT(column), "clicked", G_CALLBACK(pref_click_column), but);
	}
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	return renderer;
}

/* 3 entries must have len 3, but in reality it will be 4, because there is a NULL pointer appended */
static gchar **
pref_create_empty_strarr(gint len)
{
	gchar **strarr = g_malloc0((len + 1) * sizeof(gchar *));
	gint i;
	strarr[0] = g_strdup(_("Untitled"));
	for (i = 1; i < len; i++) {
		strarr[i] = g_strdup("");
	}
	strarr[len] = NULL;
	return strarr;
}

static gchar *
bittoggle(gchar *orig, gint bit, gboolean value, gint defaultvalue)
{
	int i;
	i = (orig && orig[0]!='\0') ? atoi(orig) : defaultvalue;
	/*g_print("bittoggle, orig=%s, so i=%d\n",orig,i);*/
	if (value) {
		i |= (1 << bit);
	} else {
		i &= ~ (1 << bit);
	}
	DEBUG_MSG("bittoggle, toggled bit %d, orig=%s, result=%d\n",bit,orig,i);
	return g_strdup_printf("%d", i);
}

static void
set_arr_user_defined(gchar **arr)
{
	gchar *tmp = arr[0];
	arr[0] = bittoggle(tmp, 1, TRUE, 3/* user defined enabled */);
	DEBUG_MSG("set_arr_user_defined, original was %s, new is %s\n",tmp,arr[0]);
	g_free(tmp);
}

/* type 0=escapedtext, 1=text, 2=toggle */
static void
pref_apply_change(GtkListStore * lstore, gint pointerindex, Tprefapplytype type, gchar * path, gchar * newval,
				  gint index)
{
	gchar **strarr;
	GtkTreeIter iter;
	GtkTreePath *tpath = gtk_tree_path_new_from_string(path);
	DEBUG_MSG("pref_apply_change, tpath %p\n", tpath);
	if (tpath && gtk_tree_model_get_iter(GTK_TREE_MODEL(lstore), &iter, tpath)) {
		gchar *oldval;
		gtk_tree_model_get(GTK_TREE_MODEL(lstore), &iter, pointerindex, &strarr, -1);
		DEBUG_MSG("pref_apply_change, lstore=%p, index=%d, type=%d, got strarr=%p\n", lstore, index, type,
				  strarr);
		if (type == prefapply_text || type == prefapply_escapedtext || type == prefapply_escapedtext_w_userdefined || type == prefapply_text_w_userdefined) {
			gtk_list_store_set(GTK_LIST_STORE(lstore), &iter, index, newval, -1);
		} else if (type == prefapply_toggle || type == prefapply_bit1toggle || type == prefapply_toggle_w_userdefined){
			gtk_list_store_set(GTK_LIST_STORE(lstore), &iter, index, (newval[0] == '1'), -1);
		}
		DEBUG_MSG("pref_apply_change, old value for strarr[%d] was %s\n", index, strarr[index]);
		oldval = strarr[index];
		if (type == prefapply_escapedtext) {
			strarr[index] = unescape_string(newval, FALSE);
		} else if (type == prefapply_bit1toggle) {
			/*int i;
			i = (oldval && oldval[0]!='\0') ? atoi(oldval) : 2;
			g_print("bit1toggle, oldval=%s, so i=%d\n",oldval,i);
			if (newval[0] == '1') {
				i |= 0x02;
			} else {
				i &= 0x3FE;
			}
			strarr[index] = g_strdup_printf("%d", i);*/
			strarr[index] = bittoggle(oldval, 0, (newval[0] == '1'), 2);
			DEBUG_MSG("new value for index %d is %s\n",index,strarr[index]);
		} else {
			strarr[index] = g_strdup(newval);
		}
		g_free(oldval);
		DEBUG_MSG("pref_apply_change, strarr[%d] now is %s\n", index, strarr[index]);
		if (type == prefapply_escapedtext_w_userdefined || type == prefapply_text_w_userdefined || type == prefapply_toggle_w_userdefined) {
			/*g_print("pref_apply_change, type=%d, call set_arr_user_defined()\n", type);*/
			set_arr_user_defined(strarr);
		}

	} else {
		DEBUG_MSG("ERROR: path %s was not converted to tpath(%p) or iter (lstore=%p)\n", path, tpath, lstore);
	}
	gtk_tree_path_free(tpath);
}

static void
pref_delete_strarr(Tprefdialog * pd, Tlistpref * lp, gint pointercolumn)
{
	GtkTreeIter iter;
	GtkTreeSelection *select;
	lp->insertloc = -1;
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(lp->lview));
	if (gtk_tree_selection_get_selected(select, NULL, &iter)) {
		gchar **strarr;
		gtk_tree_model_get(GTK_TREE_MODEL(lp->lstore), &iter, pointercolumn, &strarr, -1);
		gtk_list_store_remove(GTK_LIST_STORE(lp->lstore), &iter);
		*lp->thelist = g_list_remove(*lp->thelist, strarr);
		g_strfreev(strarr);
	}
}

static void
listpref_row_inserted(GtkTreeModel * treemodel, GtkTreePath * arg1, GtkTreeIter * arg2, Tlistpref * lp)
{
	gint *indices = gtk_tree_path_get_indices(arg1);
	if (indices) {
		lp->insertloc = indices[0];
		DEBUG_MSG("reorderable_row_inserted, insertloc=%d\n", lp->insertloc);
	}
}

static void
listpref_row_deleted(GtkTreeModel * treemodel, GtkTreePath * arg1, Tlistpref * lp)
{
	if (lp->insertloc > -1) {
		gint *indices = gtk_tree_path_get_indices(arg1);
		if (indices) {
			GList *lprepend, *ldelete;
			gint deleteloc = indices[0];
			if (deleteloc > lp->insertloc)
				deleteloc--;
			DEBUG_MSG("reorderable_row_deleted, deleteloc=%d, insertloc=%d, listlen=%d\n", deleteloc,
					  lp->insertloc, g_list_length(*lp->thelist));
			*lp->thelist = g_list_first(*lp->thelist);
			lprepend = g_list_nth(*lp->thelist, lp->insertloc);
			ldelete = g_list_nth(*lp->thelist, deleteloc);
			if (ldelete && (ldelete != lprepend)) {
				gpointer data = ldelete->data;
				*lp->thelist = g_list_remove(*lp->thelist, data);
				if (lprepend == NULL) {
					DEBUG_MSG("lprepend=NULL, appending %s to the list\n", ((gchar **) data)[0]);
					*lp->thelist = g_list_append(g_list_last(*lp->thelist), data);
				} else {
					DEBUG_MSG("lprepend %s, ldelete %s\n", ((gchar **) lprepend->data)[0],
							  ((gchar **) data)[0]);
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

/* session preferences */

void
sessionprefs_apply(Tsessionprefs * sprefs, Tsessionvars * sessionvars)
{
	gchar *template_name = NULL;
	GList *tmplist;
	integer_apply(&sessionvars->enable_syntax_scan, sprefs->prefs[enable_syntax_scan], TRUE);
	integer_apply(&sessionvars->autoindent, sprefs->prefs[autoindent], TRUE);
	integer_apply(&sessionvars->editor_tab_width, sprefs->prefs[editor_tab_width], FALSE);
	integer_apply(&sessionvars->editor_indent_wspaces, sprefs->prefs[editor_indent_wspaces], TRUE);
	integer_apply(&sessionvars->view_line_numbers, sprefs->prefs[view_line_numbers], TRUE);
	integer_apply(&sessionvars->view_blocks, sprefs->prefs[view_blocks], TRUE);
	integer_apply(&sessionvars->view_blockstack, sprefs->prefs[view_blockstack], TRUE);
	integer_apply(&sessionvars->autocomplete, sprefs->prefs[autocomplete], TRUE);
	integer_apply(&sessionvars->show_mbhl, sprefs->prefs[show_mbhl], TRUE);
	integer_apply(&sessionvars->view_cline, sprefs->prefs[view_cline], TRUE);
	string_apply(&sessionvars->default_mime_type, sprefs->prefs[default_mime_type]);
	integer_apply(&sessionvars->wrap_text_default, sprefs->prefs[session_wrap_text], TRUE);
	integer_apply(&sessionvars->display_right_margin, sprefs->prefs[display_right_margin], TRUE);
	integer_apply(&sessionvars->show_visible_spacing, sprefs->prefs[show_visible_spacing], TRUE);

	string_apply(&template_name, sprefs->prefs[template]);
	g_free(sessionvars->template);
	sessionvars->template = NULL;
	for (tmplist = g_list_first(main_v->templates); tmplist; tmplist = g_list_next(tmplist)) {
		gchar **arr = tmplist->data;
		if (g_strcmp0(arr[0], template_name) == 0) {
			sessionvars->template = g_strdup(arr[1]);
		}
	}
	g_free(template_name);

#ifdef HAVE_LIBENCHANT
	integer_apply(&sessionvars->spell_check_default, sprefs->prefs[session_spell_check], TRUE);
#endif
}

Tsessionprefs *
sessionprefs(const gchar * label, Tsessionprefs * sprefs, Tsessionvars * sessionvars)
{
	gchar *curtemplate = NULL;
	GList *poplist, *tmplist;
	GtkWidget *hbox, *table, *vbox2;

	sprefs->frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(sprefs->frame), GTK_SHADOW_IN);
	sprefs->vbox = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(sprefs->vbox), 6);
	gtk_container_add(GTK_CONTAINER(sprefs->frame), sprefs->vbox);

	vbox2 = dialog_vbox_labeled(label, sprefs->vbox);
	table = dialog_table_in_vbox_defaults(2, 2, 0, vbox2);

	poplist = g_list_sort(langmgr_get_languages_mimetypes(), (GCompareFunc) g_strcmp0);
	sprefs->prefs[default_mime_type] = dialog_combo_box_text_from_list_in_table(poplist,
																				sessionvars->default_mime_type,
																				table, 1, 2, 0, 1);
	dialog_mnemonic_label_in_table(_("_Mime type:"), sprefs->prefs[default_mime_type], table, 0, 1, 0, 1);
	g_list_free(poplist);

	poplist = NULL;
	for (tmplist = g_list_first(main_v->templates); tmplist; tmplist = g_list_next(tmplist)) {
		gchar **arr = tmplist->data;
		poplist = g_list_prepend(poplist, arr[0]);
		if (g_strcmp0(arr[1], sessionvars->template) == 0) {
			curtemplate = arr[0];
		}
	}
	poplist = g_list_sort(poplist, (GCompareFunc) g_strcmp0);
	poplist = g_list_prepend(poplist, _("None"));
	sprefs->prefs[template] =
		dialog_combo_box_text_from_list_in_table(poplist, curtemplate ? curtemplate : _("None"), table, 1, 2,
												 1, 2);
	dialog_mnemonic_label_in_table(_("_Template:"), sprefs->prefs[template], table, 0, 1, 1, 2);
	g_list_free(poplist);

	vbox2 = dialog_vbox_labeled(_("<b>Options</b>"), sprefs->vbox);
	table = dialog_table_in_vbox_defaults(6, 4, 0, vbox2);

	sprefs->prefs[autocomplete] =
		dialog_check_button_in_table(_("Enable a_uto-completion"), sessionvars->autocomplete, table, 0, 1, 0,1);
	sprefs->prefs[view_blocks] =
		dialog_check_button_in_table(_("Enable _block folding"), sessionvars->view_blocks, table, 0, 1, 1, 2);
	sprefs->prefs[view_blockstack] =
		dialog_check_button_in_table(_("Display block stack in statusbar"), sessionvars->view_blockstack, table, 0, 1, 2, 3);
	sprefs->prefs[show_mbhl] =
		dialog_check_button_in_table(_("Highlight block _delimiters"), sessionvars->show_mbhl, table, 0,1,3,4);
	sprefs->prefs[view_cline] =
		dialog_check_button_in_table(_("_Highlight current line"), sessionvars->view_cline, table, 0,1,4,5);
	sprefs->prefs[view_line_numbers] =
		dialog_check_button_in_table(_("Show line _numbers"), sessionvars->view_line_numbers, table, 0, 1, 5,6);

	/* right column */
	sprefs->prefs[display_right_margin] =
		dialog_check_button_in_table(_("Show _right margin indicator"), sessionvars->display_right_margin,table, 1,2,0,1);
	sprefs->prefs[autoindent] =
		dialog_check_button_in_table(_("Smart auto indentin_g"), sessionvars->autoindent, table, 1,2,1,2);
	sprefs->prefs[session_wrap_text] =
		dialog_check_button_in_table(_("Wra_p lines"), sessionvars->wrap_text_default, table, 1,2,2,3);
	sprefs->prefs[enable_syntax_scan] =
		dialog_check_button_in_table(_("Enable syntax scanning"), sessionvars->enable_syntax_scan, table, 1,2,3,4);
	sprefs->prefs[show_visible_spacing] =
		dialog_check_button_in_table(_("Show _visible spacing"), sessionvars->show_visible_spacing, table, 1,2,4,5);


#ifdef HAVE_LIBENCHANT
	sprefs->prefs[session_spell_check] = dialog_check_button_in_table(_("_Enable spell check"),
																	  sessionvars->spell_check_default, table,
																	  1,2,5,6);
#endif

	vbox2 = dialog_vbox_labeled(_("<b>Tab Stops</b>"), sprefs->vbox);

	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	sprefs->prefs[editor_tab_width] =
		dialog_spin_button_labeled(1, 50, sessionvars->editor_tab_width, _("Tab _width:"), hbox, 0);

	sprefs->prefs[editor_indent_wspaces] =
		dialog_check_button_new(_("Insert _spaces instead of tabs"), sessionvars->editor_indent_wspaces);
	gtk_box_pack_start(GTK_BOX(vbox2), sprefs->prefs[editor_indent_wspaces], FALSE, FALSE, 0);

	gtk_widget_show_all(sprefs->frame);
	return sprefs;
}

/************ plugin code ****************/
static void
set_plugin_strarr_in_list(GtkTreeIter * iter, gchar ** strarr, Tprefdialog * pd)
{
	gint arrcount;
	arrcount = g_strv_length(strarr);
	if (arrcount == 3) {
		DEBUG_MSG("set_plugin_strarr_in_list, arrcount=%d, file=%s\n", arrcount, strarr[0]);
		gtk_list_store_set(GTK_LIST_STORE(pd->pd.lstore), iter, 0, strarr[2], 1, (strarr[1][0] == '1'), 2,
						   strarr[0], 3, strarr, -1);
	}
}

static void
plugin_1_toggled_lcb(GtkCellRendererToggle * cellrenderertoggle, gchar * path, Tprefdialog * pd)
{
	gchar *val = g_strdup(gtk_cell_renderer_toggle_get_active(cellrenderertoggle) ? "0" : "1");
	pref_apply_change(pd->pd.lstore, 3, 2, path, val, 1);
	g_free(val);
}

static void
create_plugin_gui(Tprefdialog * pd, GtkWidget * vbox1)
{
	GtkWidget *hbox, *scrolwin;
	pd->lists[pluginconfig] = duplicate_arraylist(main_v->props.plugin_config);
	pd->pd.lstore = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_POINTER);
	pd->pd.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->pd.lstore));
	g_object_unref(G_OBJECT(pd->pd.lstore));
	pref_create_column(GTK_TREE_VIEW(pd->pd.lview), 1, NULL, pd, _("Name"), 0, FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->pd.lview), 2, G_CALLBACK(plugin_1_toggled_lcb), pd, _("Enabled"), 1,
					   FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->pd.lview), 1, NULL, pd, _("Path to plugin"), 2, FALSE);
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolwin), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scrolwin), pd->pd.lview);
	gtk_box_pack_start(GTK_BOX(vbox1), scrolwin, TRUE, TRUE, 2);

	{
		GList *tmplist = g_list_first(pd->lists[pluginconfig]);
		while (tmplist) {
			gint arrcount;
			gchar **strarr = (gchar **) tmplist->data;
			arrcount = g_strv_length(strarr);
			if (arrcount == 3) {
				GtkTreeIter iter;
				gtk_list_store_append(GTK_LIST_STORE(pd->pd.lstore), &iter);
				set_plugin_strarr_in_list(&iter, strarr, pd);
			}
			tmplist = g_list_next(tmplist);
		}
	}
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 2);
}

/****** Text style **************/

static void
set_textstyle_strarr_in_list(GtkTreeIter * iter, gchar ** strarr, Tprefdialog * pd)
{
	gint arrcount;
	arrcount = g_strv_length(strarr);
	if (arrcount == 6) {
		gtk_list_store_set(GTK_LIST_STORE(pd->tsd.lstore), iter, 0, strarr[0], 1, strarr, -1);
	} else {
		DEBUG_MSG("ERROR: set_textstyle_strarr_in_list, arraycount != 5 !!!!!!\n");
	}
}

static void
textstyle_0_edited_lcb(GtkCellRendererText * cellrenderertext, gchar * path, gchar * newtext,
					   Tprefdialog * pd)
{
	pref_apply_change(pd->tsd.lstore, 1, 1, path, newtext, 0);
}

static void
add_new_textstyle_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	gchar **strarr;
	GtkTreeIter iter;
	strarr = pref_create_empty_strarr(6);
	gtk_list_store_append(GTK_LIST_STORE(pd->tsd.lstore), &iter);
	set_textstyle_strarr_in_list(&iter, strarr, pd);
	pd->lists[textstyles] = g_list_append(pd->lists[textstyles], strarr);
	pd->tsd.insertloc = -1;
}

static void
delete_textstyle_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	GtkTreeIter iter;
	GtkTreeSelection *select;
	pd->tsd.insertloc = -1;
	pd->tsd.curstrarr = NULL;
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->tsd.lview));
	if (gtk_tree_selection_get_selected(select, NULL, &iter)) {
		gchar **strarr;
		gtk_tree_model_get(GTK_TREE_MODEL(pd->tsd.lstore), &iter, 1, &strarr, -1);
		gtk_list_store_remove(GTK_LIST_STORE(pd->tsd.lstore), &iter);
		*pd->tsd.thelist = g_list_remove(*pd->tsd.thelist, strarr);
		g_strfreev(strarr);
	}
}

static void
textstyle_selection_changed_cb(GtkTreeSelection * selection, Tprefdialog * pd)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	DEBUG_MSG("textstyle_selection_changed_cb, started\n");
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gchar **strarr;
		GdkColor color;

		gtk_tree_model_get(model, &iter, 1, &strarr, -1);
		pd->tsd.curstrarr = NULL;
		DEBUG_MSG("textstyle_selection_changed_cb, setting %s, strarr=%p\n", strarr[0], strarr);

		if (gdk_color_parse(strarr[1], &color)) {
			DEBUG_MSG("textstyle_selection_changed_cb, strarr[1]=%s\n", strarr[1]);
			gtk_color_button_set_color(GTK_COLOR_BUTTON(pd->tsd.fg_color), &color);
			gtk_color_button_set_alpha(GTK_COLOR_BUTTON(pd->tsd.fg_color), 65535);
		} else {
			color.blue = 0;
			color.green = 0;
			color.red = 0;
			gtk_color_button_set_color(GTK_COLOR_BUTTON(pd->tsd.fg_color), &color);
			gtk_color_button_set_alpha(GTK_COLOR_BUTTON(pd->tsd.fg_color), 0);
		}
		if (gdk_color_parse(strarr[2], &color)) {
			DEBUG_MSG("textstyle_selection_changed_cb, strarr[2]=%s\n", strarr[2]);
			gtk_color_button_set_color(GTK_COLOR_BUTTON(pd->tsd.bg_color), &color);
			gtk_color_button_set_alpha(GTK_COLOR_BUTTON(pd->tsd.bg_color), 65535);
		} else {
			color.blue = 0;
			color.green = 0;
			color.red = 0;
			gtk_color_button_set_color(GTK_COLOR_BUTTON(pd->tsd.bg_color), &color);
			gtk_color_button_set_alpha(GTK_COLOR_BUTTON(pd->tsd.bg_color), 0);
		}

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->tsd.bold_radio[0]), (strarr[3][0] != '1'));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->tsd.bold_radio[1]), (strarr[3][0] == '1'));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->tsd.italic_radio[0]), (strarr[4][0] != '1'));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->tsd.italic_radio[1]), (strarr[4][0] == '1'));
#ifdef HAVE_LIBENCHANT
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->tsd.need_spellcheck), (strarr[5][0] == '1'));
#endif
		pd->tsd.curstrarr = strarr;
	} else {
		DEBUG_MSG("no selection, returning..\n");
	}
}

static void
textstyle_color_button_color_set(GtkColorButton * button, gpointer user_data)
{
	Tprefdialog *pd = (Tprefdialog *) user_data;
	DEBUG_MSG("textstyle_color_button_color_set\n");
	if (pd->tsd.curstrarr) {
		int index = 2;
		GdkColor color;

		if (((GtkWidget *) button) == pd->tsd.fg_color) {
			index = 1;
		}

		if (pd->tsd.curstrarr[index])
			g_free(pd->tsd.curstrarr[index]);

		gtk_color_button_get_color(GTK_COLOR_BUTTON(button), &color);
		pd->tsd.curstrarr[index] = gdk_color_to_string(&color);
		DEBUG_MSG("textstyle_radio_changed, changing arr[%d] to %s\n", index, pd->tsd.curstrarr[index]);
	}
}

static void
textstyle_radio_changed(GtkToggleButton * togglebutton, gpointer user_data)
{
	Tprefdialog *pd = (Tprefdialog *) user_data;
	DEBUG_MSG("textstyle_radio_changed\n");
	if (pd->tsd.curstrarr && gtk_toggle_button_get_active(togglebutton)) {
		int index = 3;
		gchar *val = "0";
		if ((GtkWidget *) togglebutton == pd->tsd.bold_radio[0]) {
			index = 3;
			val = "0";
		} else if ((GtkWidget *) togglebutton == pd->tsd.bold_radio[1]) {
			index = 3;
			val = "1";
		} else if ((GtkWidget *) togglebutton == pd->tsd.italic_radio[0]) {
			index = 4;
			val = "0";
		} else if ((GtkWidget *) togglebutton == pd->tsd.italic_radio[1]) {
			index = 4;
			val = "1";
		}
		if (pd->tsd.curstrarr[index])
			g_free(pd->tsd.curstrarr[index]);
		pd->tsd.curstrarr[index] = g_strdup(val);
		DEBUG_MSG("textstyle_radio_changed, changing arr[%d] to %s\n", index, val);
	}
}

#ifdef HAVE_LIBENCHANT
static void
textstyle_spellcheck_changed(GtkToggleButton * togglebutton, gpointer user_data)
{
	Tprefdialog *pd = (Tprefdialog *) user_data;
	if (pd->tsd.curstrarr) {
		if (pd->tsd.curstrarr[5])
			g_free(pd->tsd.curstrarr[5]);
		pd->tsd.curstrarr[5] = g_strdup(gtk_toggle_button_get_active(togglebutton) ? "1" : "0");
	}
}
#endif
/*
 the textstyle gui is based on a liststore with 2 columns, first the name, second the gchar **
 where this should be updated. We keep track of the current selected name, and set that gchar ** in
 pd->tsd.curstrarr. Whenever any of the radiobuttons or colors is changed, we can immediatly change
 it in pd->tsd.curstrarr
 if pd->tsd.curstrarr == NULL then there is no name selected, for example we are in the middle of
 a switch, delete, add etc.
 */
static void
create_textstyle_gui(Tprefdialog * pd, GtkWidget * vbox1)
{
	GtkWidget *but, *hbox, *hbox2, *label, *scrolwin, *table, *vbox;
	GtkTreeSelection *select;
	DEBUG_MSG("create_textstyle_gui\n");
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),
						 _
						 ("<small>Text styles are applied on top of each other. If multiple styles are applied to the same text, the top-most style in this list has the highest priority. Use drag and drop to re-order the text styles.</small>"));
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), label, FALSE, TRUE, 2);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, TRUE, TRUE, 2);
	pd->lists[textstyles] = duplicate_arraylist(main_v->props.textstyles);
	pd->tsd.lstore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
	pd->tsd.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->tsd.lstore));
	g_object_unref(G_OBJECT(pd->tsd.lstore));
	pref_create_column(GTK_TREE_VIEW(pd->tsd.lview), 1, G_CALLBACK(textstyle_0_edited_lcb), pd, _("Label"), 0,
					   FALSE);
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->tsd.lview));
	g_signal_connect(G_OBJECT(select), "changed", G_CALLBACK(textstyle_selection_changed_cb), pd);

	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolwin), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scrolwin), pd->tsd.lview);
	gtk_box_pack_start(GTK_BOX(hbox), scrolwin, TRUE, TRUE, 2);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);
	table = dialog_table_in_vbox(2, 2, 0, vbox, FALSE, FALSE, 0);

	pd->tsd.fg_color = dialog_color_button_in_table(NULL, _("Foreground color"), table, 1, 2, 0, 1);
	dialog_mnemonic_label_in_table(_("_Foreground color:"), pd->tsd.fg_color, table, 0, 1, 0, 1);
	pd->tsd.bg_color = dialog_color_button_in_table(NULL, _("Background color"), table, 1, 2, 1, 2);
	dialog_mnemonic_label_in_table(_("_Background color:"), pd->tsd.bg_color, table, 0, 1, 1, 2);

	table = dialog_table_in_vbox(5, 1, 0, vbox, FALSE, FALSE, 12);

	pd->tsd.bold_radio[0] = dialog_radio_button_in_table(NULL, _("_Normal weight"), table, 0, 1, 0, 1);
	pd->tsd.bold_radio[1] =
		dialog_radio_button_from_widget_in_table(GTK_RADIO_BUTTON(pd->tsd.bold_radio[0]), _("Bold _weight"),
												 table, 0, 1, 1, 2);
	pd->tsd.italic_radio[0] = dialog_radio_button_in_table(NULL, _("Normal st_yle"), table, 0, 1, 2, 3);
	pd->tsd.italic_radio[1] =
		dialog_radio_button_from_widget_in_table(GTK_RADIO_BUTTON(pd->tsd.italic_radio[0]),
												 _("_Italic style"), table, 0, 1, 3, 4);
#ifdef HAVE_LIBENCHANT
	pd->tsd.need_spellcheck = dialog_check_button_in_table(_("_Spell check"), FALSE, table, 0, 1, 4, 5);
#endif

	{
		GList *tmplist = g_list_first(pd->lists[textstyles]);	/* the order of this list has a meaning !!! */
		while (tmplist) {
			gchar **strarr = (gchar **) tmplist->data;
			gint count = g_strv_length(strarr);
			if (count == 6) {
				GtkTreeIter iter;
				gtk_list_store_append(GTK_LIST_STORE(pd->tsd.lstore), &iter);
				set_textstyle_strarr_in_list(&iter, strarr, pd);
			} else {
				g_print("invalid/outdated textstyle with length %d\n", g_strv_length(strarr));
			}
			tmplist = g_list_next(tmplist);
		}
	}
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(pd->tsd.lview), TRUE);
	pd->tsd.thelist = &pd->lists[textstyles];
	pd->tsd.insertloc = -1;
	g_signal_connect(G_OBJECT(pd->tsd.lstore), "row-inserted", G_CALLBACK(listpref_row_inserted), &pd->tsd);
	g_signal_connect(G_OBJECT(pd->tsd.lstore), "row-deleted", G_CALLBACK(listpref_row_deleted), &pd->tsd);

	g_signal_connect(G_OBJECT(pd->tsd.fg_color), "color-set", G_CALLBACK(textstyle_color_button_color_set),
					 pd);
	g_signal_connect(G_OBJECT(pd->tsd.bg_color), "color-set", G_CALLBACK(textstyle_color_button_color_set),
					 pd);

	g_signal_connect(G_OBJECT(pd->tsd.bold_radio[0]), "toggled", G_CALLBACK(textstyle_radio_changed), pd);
	g_signal_connect(G_OBJECT(pd->tsd.bold_radio[1]), "toggled", G_CALLBACK(textstyle_radio_changed), pd);

	g_signal_connect(G_OBJECT(pd->tsd.italic_radio[0]), "toggled", G_CALLBACK(textstyle_radio_changed), pd);
	g_signal_connect(G_OBJECT(pd->tsd.italic_radio[1]), "toggled", G_CALLBACK(textstyle_radio_changed), pd);

#ifdef HAVE_LIBENCHANT
	g_signal_connect(G_OBJECT(pd->tsd.need_spellcheck), "toggled", G_CALLBACK(textstyle_spellcheck_changed),
					 pd);
#endif

	hbox2 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox2, FALSE, FALSE, 2);
	but = dialog_button_new_with_image(_("Add entry"), GTK_STOCK_ADD, G_CALLBACK(add_new_textstyle_lcb), pd, TRUE, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox2), but, FALSE, FALSE, 2);
	but = dialog_button_new_with_image(_("Delete entry"), GTK_STOCK_DELETE, G_CALLBACK(delete_textstyle_lcb), pd, TRUE, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox2), but, FALSE, FALSE, 2);
}

/************ external commands code ****************/

static void
set_extcommands_strarr_in_list(GtkTreeIter * iter, gchar ** strarr, Tprefdialog * pd)
{
	gint arrcount = g_strv_length(strarr);
	if (arrcount == 4) {
		gtk_list_store_set(GTK_LIST_STORE(pd->bd.lstore), iter,
				0, (strarr[0][0]=='1' || strarr[0][0]=='3'),
				1, strarr[1],
				2, strarr[2],
				3, strarr[3][0] == '1',
				4, strarr, -1);
	} else {
		DEBUG_MSG("ERROR: set_extcommands_strarr_in_list, arraycount %d != 4 !!!!!!\n", arrcount);
	}
}

static void
extcommands_0_edited_lcb(GtkCellRendererToggle * cellrenderertoggle, gchar * path, Tprefdialog * pd)
{
	pref_apply_change(pd->bd.lstore, 4, prefapply_bit1toggle, path, gtk_cell_renderer_toggle_get_active(cellrenderertoggle) ? "0" : "1", 0);
}

static void
extcommands_1_edited_lcb(GtkCellRendererText * cellrenderertext, gchar * path, gchar * newtext,
						 Tprefdialog * pd)
{
	pref_apply_change(pd->bd.lstore, 4, prefapply_text_w_userdefined, path, newtext, 1);
}

static void
extcommands_2_edited_lcb(GtkCellRendererText * cellrenderertext, gchar * path, gchar * newtext,
						 Tprefdialog * pd)
{
	pref_apply_change(pd->bd.lstore, 4, prefapply_text_w_userdefined, path, newtext, 2);
}

static void
extcommands_3_edited_lcb(GtkCellRendererToggle * cellrenderertoggle, gchar * path, Tprefdialog * pd)
{
	pref_apply_change(pd->bd.lstore, 4, prefapply_toggle, path,
					  gtk_cell_renderer_toggle_get_active(cellrenderertoggle) ? "0" : "1", 3);
}

static void reload_extcommands(Tprefdialog * pd, GList *list)
{
	GList *tmplist = g_list_first(list);
	/* now make sure that the list clear will not actuallt modify the list itself */
	pd->bd.insertloc = -1;
	gtk_list_store_clear(GTK_LIST_STORE(pd->bd.lstore));
	while (tmplist) {
		GtkTreeIter iter;
		gtk_list_store_append(GTK_LIST_STORE(pd->bd.lstore), &iter);
		set_extcommands_strarr_in_list(&iter, (gchar **) tmplist->data, pd);
		tmplist = g_list_next(tmplist);
	}
}

static void
add_new_extcommands_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	gchar **strarr;
	GtkTreeIter iter;

	strarr = pref_create_empty_strarr(4);
	gtk_list_store_append(GTK_LIST_STORE(pd->bd.lstore), &iter);
	set_extcommands_strarr_in_list(&iter, strarr, pd);
	pd->lists[extcommands] = g_list_append(pd->lists[extcommands], strarr);
	pd->bd.insertloc = -1;
}

static void
delete_extcommands_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	pref_delete_strarr(pd, &pd->bd, 4);
}

static void
updatedefaults_extcommands_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	pd->lists[extcommands] = update_commands(pd->lists[extcommands], FALSE);
	reload_extcommands(pd, pd->lists[extcommands]);
}

static void
reset_extcommands_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	pd->lists[extcommands] = update_commands(pd->lists[extcommands], TRUE);
	reload_extcommands(pd, pd->lists[extcommands]);
}

static gboolean
extcommands_query_tooltip_lcb(GtkWidget  *widget, gint        x,  gint        y,  gboolean    keyboard_mode,  GtkTooltip *tooltip,  gpointer    user_data)
{
	gtk_tooltip_set_markup(tooltip, _
						 (
						 "<small><b>Input options</b>\n"
						 "start with a | to send the input to the standard input\n"
						 "%f local filename (available for local files)\n"
						 "%i temporary filename for input, equals %f if the document is not modified and local\n"
						 "<b>Other options</b>\n"
						 "%a additional arguments that will be asked when this filter is activated\n"
						 "%c local directory of file (available for local files)\n"
						 "%n filename without path (available for all titled files)\n"
						 "%u URL (available for all titled files)\n"
						 "%p preview URL if basedir and preview dir are set in project settings, else identical to %u</small>"
						 ));
	return TRUE;
}

static void
create_extcommands_gui(Tprefdialog * pd, GtkWidget * vbox1)
{
	GtkWidget *hbox, *but, *scrolwin;
	pd->lists[extcommands] = duplicate_arraylist(main_v->props.external_command);
	pd->bd.lstore = gtk_list_store_new(5, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_POINTER);
	pd->bd.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->bd.lstore));
	g_object_unref(G_OBJECT(pd->bd.lstore));
	pref_create_column(GTK_TREE_VIEW(pd->bd.lview), 2, G_CALLBACK(extcommands_0_edited_lcb), pd,
					   _("Enabled"), 0, FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->bd.lview), 1, G_CALLBACK(extcommands_1_edited_lcb), pd, _("Label"),
					   1, FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->bd.lview), 1, G_CALLBACK(extcommands_2_edited_lcb), pd, _("Command"),
					   2, FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->bd.lview), 2, G_CALLBACK(extcommands_3_edited_lcb), pd,
					   _("Default browser"), 3, FALSE);
	g_signal_connect(G_OBJECT(pd->bd.lview), "query-tooltip", G_CALLBACK(extcommands_query_tooltip_lcb), pd);
	g_object_set(G_OBJECT(pd->bd.lview), "has-tooltip", TRUE, NULL);
	/*label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),
						 _
						 ("<small><b>Input options</b>\nstart with a | to send the input to the standard input\n"
						 "%f local filename (available for local files)\n"
						 "%i temporary filename for input, equals %f if the document is not modified and local\n"
						 "<b>Other options</b>\n"
						 "%c local directory of file (available for local files)\n"
						 "%n filename without path (available for all titled files)\n"
						 "%u URL (available for all titled files)\n"
						 "%p preview URL if basedir and preview dir are set in project settings, else identical to %u</small>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), label, TRUE, TRUE, 2);*/
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolwin), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scrolwin), pd->bd.lview);
	gtk_box_pack_start(GTK_BOX(vbox1), scrolwin, TRUE, TRUE, 2);

	reload_extcommands(pd, pd->lists[extcommands]);

	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(pd->bd.lview), TRUE);
	pd->bd.thelist = &pd->lists[extcommands];
	pd->bd.insertloc = -1;
	g_signal_connect(G_OBJECT(pd->bd.lstore), "row-inserted", G_CALLBACK(listpref_row_inserted), &pd->bd);
	g_signal_connect(G_OBJECT(pd->bd.lstore), "row-deleted", G_CALLBACK(listpref_row_deleted), &pd->bd);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 2);
	but = dialog_button_new_with_image(_("Add entry"), GTK_STOCK_ADD, G_CALLBACK(add_new_extcommands_lcb), pd, TRUE, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 2);
	but = dialog_button_new_with_image(_("Delete entry"), GTK_STOCK_DELETE, G_CALLBACK(delete_extcommands_lcb), pd, TRUE, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 2);
	but = dialog_button_new_with_image(_("Update defaults"), GTK_STOCK_REFRESH, G_CALLBACK(updatedefaults_extcommands_lcb), pd, TRUE, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 2);
	but = dialog_button_new_with_image(_("Reset all"), GTK_STOCK_CONVERT, G_CALLBACK(reset_extcommands_lcb), pd, TRUE, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 2);
}

/************ external filters?? code ****************/

static void
set_external_filters_strarr_in_list(GtkTreeIter * iter, gchar ** strarr, Tprefdialog * pd)
{
	gint arrcount = g_strv_length(strarr);
	if (arrcount == 3) {
		gtk_list_store_set(GTK_LIST_STORE(pd->ed.lstore), iter,
				0, (strarr[0][0]=='1' || strarr[0][0]=='3'),
				1, strarr[1], 2, strarr[2], 3, strarr, -1);
	} else {
		DEBUG_MSG("ERROR: set_external_command_strarr_in_list, arraycount != 2 !!!!!!\n");
	}
}

static void
external_filter_0_edited_lcb(GtkCellRendererToggle * cellrenderertoggle, gchar * path, Tprefdialog * pd)
{
	pref_apply_change(pd->ed.lstore, 3, prefapply_bit1toggle, path, gtk_cell_renderer_toggle_get_active(cellrenderertoggle) ? "0" : "1", 0);
}

static void
external_filter_1_edited_lcb(GtkCellRendererText * cellrenderertext, gchar * path, gchar * newtext,
							 Tprefdialog * pd)
{
	pref_apply_change(pd->ed.lstore, 3, prefapply_text_w_userdefined, path, newtext, 1);
}

static void
external_filter_2_edited_lcb(GtkCellRendererText * cellrenderertext, gchar * path, gchar * newtext,
							 Tprefdialog * pd)
{
	pref_apply_change(pd->ed.lstore, 3, prefapply_text_w_userdefined, path, newtext, 2);
}

static void
add_new_external_filter_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	gchar **strarr;
	GtkTreeIter iter;
	strarr = pref_create_empty_strarr(3);
	gtk_list_store_append(GTK_LIST_STORE(pd->ed.lstore), &iter);
	set_external_filters_strarr_in_list(&iter, strarr, pd);
	pd->lists[extfilters] = g_list_append(pd->lists[extfilters], strarr);
	pd->ed.insertloc = -1;
}

static void
delete_external_filter_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	pref_delete_strarr(pd, &pd->ed, 3);
}

static void
reload_extfilters(Tprefdialog * pd, GList *list)
{
	GList *tmplist = g_list_first(list);
	pd->ed.insertloc = -1;
	gtk_list_store_clear(GTK_LIST_STORE(pd->ed.lstore));
	while (tmplist) {
		gchar **strarr = (gchar **) tmplist->data;
		GtkTreeIter iter;
		gtk_list_store_append(GTK_LIST_STORE(pd->ed.lstore), &iter);
		set_external_filters_strarr_in_list(&iter, strarr, pd);
		tmplist = g_list_next(tmplist);
	}
}

static void
updatedefaults_extfilters_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	pd->lists[extfilters] = update_filters(pd->lists[extfilters], FALSE);
	reload_extfilters(pd, pd->lists[extfilters]);
}

static void
reset_extfilters_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	pd->lists[extfilters] = update_filters(pd->lists[extfilters], TRUE);
	reload_extfilters(pd, pd->lists[extfilters]);
}

static gboolean
filters_query_tooltip_lcb(GtkWidget  *widget, gint        x,  gint        y,  gboolean    keyboard_mode,  GtkTooltip *tooltip,  gpointer    user_data)
{
	gtk_tooltip_set_markup(tooltip, _
						 ("<small><b>Input options</b>\n"
						 "start with a | to send the input to the standard input\n"
						 "%f local filename (requires local file, cannot operate on selection)\n"
						 "%i temporary filename for input\n"
						 "<b>Output options</b>\nend with a | to read the output from the standard output\n"
						 "%o temporary filename\n"
						 "%t temporary filename for both input and output (for in-place-editing filters, cannot operate on selection)\n"
						 "<b>Other options</b>\n"
						 "%a additional arguments that will be asked when this filter is activated\n"
						 "%c local directory of file (requires local file)\n"
						 "%n filename without path (available for all titled files)\n"
						 "%u URL (available for all titled files)\n"
						 "%p preview URL if basedir and preview dir are set in project settings, else identical to %u</small>"));
	return TRUE;
}

static void
create_filters_gui(Tprefdialog * pd, GtkWidget * vbox1)
{
	GtkWidget *hbox, *but, *scrolwin;
	pd->lists[extfilters] = duplicate_arraylist(main_v->props.external_filter);
	pd->ed.lstore = gtk_list_store_new(4, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
	pd->ed.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->ed.lstore));
	g_object_unref(G_OBJECT(pd->ed.lstore));
	pref_create_column(GTK_TREE_VIEW(pd->ed.lview), 2, G_CALLBACK(external_filter_0_edited_lcb), pd,
					   _("Enabled"), 0, FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->ed.lview), 1, G_CALLBACK(external_filter_1_edited_lcb), pd,
					   _("Label"), 1, FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->ed.lview), 1, G_CALLBACK(external_filter_2_edited_lcb), pd,
					   _("Command"), 2, FALSE);
	g_signal_connect(G_OBJECT(pd->ed.lview), "query-tooltip", G_CALLBACK(filters_query_tooltip_lcb), pd);
	g_object_set(G_OBJECT(pd->ed.lview), "has-tooltip", TRUE, NULL);
/*	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),
						 _
						 ("<small><b>Input options</b>\n"
						 "start with a | to send the input to the standard input\n"
						 "%f local filename (requires local file, cannot operate on selection)\n"
						 "%i temporary filename for input\n"
						 "<b>Output options</b>\nend with a | to read the output from the standard output\n"
						 "%o temporary filename\n%t temporary filename for both input and output (for in-place-editing filters, cannot operate on selection)\n<b>Other options</b>\n%c local directory of file (requires local file)\n%n filename without path (available for all titled files)\n%u URL (available for all titled files)\n%p preview URL if basedir and preview dir are set in project settings, else identical to %u</small>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), label, TRUE, TRUE, 2);*/
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolwin), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scrolwin), pd->ed.lview);
	gtk_box_pack_start(GTK_BOX(vbox1), scrolwin, TRUE, TRUE, 2);
	reload_extfilters(pd, pd->lists[extfilters]);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(pd->ed.lview), TRUE);
	pd->ed.thelist = &pd->lists[extfilters];
	pd->ed.insertloc = -1;
	g_signal_connect(G_OBJECT(pd->ed.lstore), "row-inserted", G_CALLBACK(listpref_row_inserted), &pd->ed);
	g_signal_connect(G_OBJECT(pd->ed.lstore), "row-deleted", G_CALLBACK(listpref_row_deleted), &pd->ed);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 2);
	but = dialog_button_new_with_image(_("Add entry"), GTK_STOCK_ADD, G_CALLBACK(add_new_external_filter_lcb), pd, TRUE, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 2);
	but = dialog_button_new_with_image(_("Delete entry"), GTK_STOCK_DELETE, G_CALLBACK(delete_external_filter_lcb), pd, TRUE, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 2);
	but = dialog_button_new_with_image(_("Update defaults"), GTK_STOCK_REFRESH, G_CALLBACK(updatedefaults_extfilters_lcb), pd, TRUE, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 2);
	but = dialog_button_new_with_image(_("Reset all"), GTK_STOCK_CONVERT, G_CALLBACK(reset_extfilters_lcb), pd, TRUE, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 2);
}

/************ outputbox code ****************/

static void
set_outputbox_strarr_in_list(GtkTreeIter * iter, gchar ** strarr, Tprefdialog * pd)
{
	gint arrcount;
	arrcount = g_strv_length(strarr);
	if (arrcount == 7) {
		gtk_list_store_set(GTK_LIST_STORE(pd->od.lstore), iter,
				0, (strarr[0][0]=='1' || strarr[0][0]=='3'),
				1, strarr[1], 2, strarr[2], 3, strarr[3], 4, strarr[4],
				5, strarr[5], 6, strarr[6], 7, strarr, -1);
	}
}

static void
outputbox_0_edited_lcb(GtkCellRendererToggle * cellrenderertoggle, gchar * path, Tprefdialog * pd)
{
	pref_apply_change(pd->od.lstore, 7, prefapply_bit1toggle, path, gtk_cell_renderer_toggle_get_active(cellrenderertoggle) ? "0" : "1", 0);
}

static void
outputbox_1_edited_lcb(GtkCellRendererText * cellrenderertext, gchar * path, gchar * newtext,
					   Tprefdialog * pd)
{
	pref_apply_change(pd->od.lstore, 7, prefapply_text_w_userdefined, path, newtext, 1);
}

static void
outputbox_2_edited_lcb(GtkCellRendererText * cellrenderertext, gchar * path, gchar * newtext,
					   Tprefdialog * pd)
{
	pref_apply_change(pd->od.lstore, 7, prefapply_text_w_userdefined, path, newtext, 2);
}

static void
outputbox_3_edited_lcb(GtkCellRendererText * cellrenderertext, gchar * path, gchar * newtext,
					   Tprefdialog * pd)
{
	pref_apply_change(pd->od.lstore, 7, prefapply_text_w_userdefined, path, newtext, 3);
}

static void
outputbox_4_edited_lcb(GtkCellRendererText * cellrenderertext, gchar * path, gchar * newtext,
					   Tprefdialog * pd)
{
	pref_apply_change(pd->od.lstore, 7, prefapply_text_w_userdefined, path, newtext, 4);
}

static void
outputbox_5_edited_lcb(GtkCellRendererText * cellrenderertext, gchar * path, gchar * newtext,
					   Tprefdialog * pd)
{
	pref_apply_change(pd->od.lstore, 7, prefapply_text_w_userdefined, path, newtext, 5);
}

static void
outputbox_6_edited_lcb(GtkCellRendererText * cellrenderertext, gchar * path, gchar * newtext,
					   Tprefdialog * pd)
{
	pref_apply_change(pd->od.lstore, 7, prefapply_text_w_userdefined, path, newtext, 6);
}

static void
add_new_outputbox_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	gchar **strarr;
	GtkTreeIter iter;
	strarr = pref_create_empty_strarr(7);
	gtk_list_store_append(GTK_LIST_STORE(pd->od.lstore), &iter);
	set_outputbox_strarr_in_list(&iter, strarr, pd);
	pd->lists[extoutputbox] = g_list_append(pd->lists[extoutputbox], strarr);
	pd->od.insertloc = -1;
}

static void
delete_outputbox_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	pref_delete_strarr(pd, &pd->od, 7);
}

static void
reload_extoutputbox(Tprefdialog * pd, GList *list)
{
	GList *tmplist = g_list_first(list);
	pd->od.insertloc = -1;
	gtk_list_store_clear(GTK_LIST_STORE(pd->od.lstore));
	while (tmplist) {
		gint arrcount;
		gchar **strarr = (gchar **) tmplist->data;
		arrcount = g_strv_length(strarr);
		if (arrcount == 7) {
			GtkTreeIter iter;
			gtk_list_store_append(GTK_LIST_STORE(pd->od.lstore), &iter);
			set_outputbox_strarr_in_list(&iter, strarr, pd);
		}
		tmplist = g_list_next(tmplist);
	}
}

static void
updatedefaults_extoutputbox_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	pd->lists[extoutputbox] = update_outputbox(pd->lists[extoutputbox], FALSE);
	reload_extoutputbox(pd, pd->lists[extoutputbox]);
}

static void
reset_extoutputbox_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	pd->lists[extoutputbox] = update_outputbox(pd->lists[extoutputbox], TRUE);
	reload_extoutputbox(pd, pd->lists[extoutputbox]);
}

static gboolean
outputbox_query_tooltip_lcb(GtkWidget  *widget, gint        x,  gint        y,  gboolean    keyboard_mode,  GtkTooltip *tooltip,  gpointer    user_data)
{
	gtk_tooltip_set_markup(tooltip, _
						 ("<small><b>Input options</b>\n"
							"start with a | to send the input to the standard input\n"
							"%f local filename (available for local files)\n"
							"%i temporary filename for input, equals %f if the document is not modified and local\n"
							"<b>Output options</b>\n"
							"end with a | to read the output from the standard output\n"
							"%o temporary filename\n"
							"<b>Other options</b>\n"
							"%a additional arguments that will be asked when this filter is activated\n"
							"%c local directory of file (available for local files)\n"
							"%n filename without path (available for all titled files)\n"
							"%u URL (available for all titled files)\n"
							"%p preview URL if basedir and preview dir are set in project settings, else identical to %u</small>"));
	return TRUE;
}

static void
create_outputbox_gui(Tprefdialog * pd, GtkWidget * vbox1)
{
	GtkWidget *hbox, *but, *scrolwin;
	pd->lists[extoutputbox] = duplicate_arraylist(main_v->props.external_outputbox);
	pd->od.lstore =
		gtk_list_store_new(8, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
						   G_TYPE_STRING, G_TYPE_POINTER);
	pd->od.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->od.lstore));
	g_object_unref(G_OBJECT(pd->od.lstore));
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 2, G_CALLBACK(outputbox_0_edited_lcb), pd, _("Name"), 0,
					   FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 1, G_CALLBACK(outputbox_1_edited_lcb), pd, _("Name"), 1,
					   FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 1, G_CALLBACK(outputbox_2_edited_lcb), pd, _("Pattern"),
					   2, FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 1, G_CALLBACK(outputbox_3_edited_lcb), pd, _("File #"), 3,
					   FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 1, G_CALLBACK(outputbox_4_edited_lcb), pd, _("Line #"), 4,
					   FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 1, G_CALLBACK(outputbox_5_edited_lcb), pd, _("Output #"),
					   5, FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->od.lview), 1, G_CALLBACK(outputbox_6_edited_lcb), pd, _("Command"),
					   6, FALSE);
	g_signal_connect(G_OBJECT(pd->od.lview), "query-tooltip", G_CALLBACK(outputbox_query_tooltip_lcb), pd);
	g_object_set(G_OBJECT(pd->od.lview), "has-tooltip", TRUE, NULL);
/*	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),
						 _
						 ("<small><b>Input options</b>\nstart with a | to send the input to the standard input\n%f local filename (available for local files)\n"
						 "%i temporary filename for input, equals %f if the document is not modified and local\n<b>Output options</b>\n"
						 "end with a | to read the output from the standard output\n%o temporary filename\n%t temporary filename for both input and output (for in-place-editing filters)\n<b>Other options</b>\n%c local directory of file (available for local files)\n%n filename without path (available for all titled files)\n%u URL (available for all titled files)\n%p preview URL if basedir and preview dir are set in project settings, else identical to %u</small>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), label, FALSE, FALSE, 2);*/
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolwin), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scrolwin), pd->od.lview);
	gtk_box_pack_start(GTK_BOX(vbox1), scrolwin, TRUE, TRUE, 2);
	reload_extoutputbox(pd, pd->lists[extoutputbox]);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(pd->od.lview), TRUE);
	pd->od.thelist = &pd->lists[extoutputbox];
	pd->od.insertloc = -1;
	g_signal_connect(G_OBJECT(pd->od.lstore), "row-inserted", G_CALLBACK(listpref_row_inserted), &pd->od);
	g_signal_connect(G_OBJECT(pd->od.lstore), "row-deleted", G_CALLBACK(listpref_row_deleted), &pd->od);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 2);
	but = dialog_button_new_with_image(_("Add entry"), GTK_STOCK_ADD, G_CALLBACK(add_new_outputbox_lcb), pd, TRUE, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 2);
	but = dialog_button_new_with_image(_("Delete entry"), GTK_STOCK_DELETE, G_CALLBACK(delete_outputbox_lcb), pd, TRUE, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 2);
	but = dialog_button_new_with_image(_("Update defaults"), GTK_STOCK_REFRESH, G_CALLBACK(updatedefaults_extoutputbox_lcb), pd, TRUE, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 2);
	but = dialog_button_new_with_image(_("Reset all"), GTK_STOCK_CONVERT, G_CALLBACK(reset_extoutputbox_lcb), pd, TRUE, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 2);
}

#ifdef OLDTEMPLATES
/********* template manager GUI */
static void
set_template_strarr_in_list(GtkTreeIter * iter, gchar ** strarr, Tprefdialog * pd)
{
	if (g_strv_length(strarr) == 2) {
		gtk_list_store_set(GTK_LIST_STORE(pd->tg.lstore), iter, 0, strarr[0], 1, strarr[1], 2, strarr, -1);
	}
}

static void
template_apply_change(Tprefdialog * pd, gint type, gchar * path, gchar * newval, gint index)
{
	pref_apply_change(pd->tg.lstore, 2, type, path, newval, index);
}

static void
template_0_edited_lcb(GtkCellRendererText * cellrenderertext, gchar * path, gchar * newtext, Tprefdialog * pd)
{
	template_apply_change(pd, 1, path, newtext, 0);
}

static void
template_1_edited_lcb(GtkCellRendererText * cellrenderertext, gchar * path, gchar * newtext, Tprefdialog * pd)
{
	template_apply_change(pd, 1, path, newtext, 1);
}

static void
add_new_template_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	gchar **strarr;
	GtkTreeIter iter;
	strarr = pref_create_empty_strarr(2);
	gtk_list_store_append(GTK_LIST_STORE(pd->tg.lstore), &iter);
	set_template_strarr_in_list(&iter, strarr, pd);
	pd->lists[templates] = g_list_append(pd->lists[templates], strarr);
	pd->tg.insertloc = -1;
}

static void
delete_template_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	pref_delete_strarr(pd, &pd->tg, 2);
}

static void
create_template_gui(Tprefdialog * pd, GtkWidget * vbox1)
{
	GList *tmplist;
	GtkWidget *hbox, *but, *scrolwin;
	pd->lists[templates] = duplicate_arraylist(main_v->props.templates);
	pd->tg.lstore = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
	pd->tg.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->tg.lstore));
	g_object_unref(G_OBJECT(pd->tg.lstore));
	pref_create_column(GTK_TREE_VIEW(pd->tg.lview), 1, G_CALLBACK(template_0_edited_lcb), pd, _("Name"), 0,
					   FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->tg.lview), 1, G_CALLBACK(template_1_edited_lcb), pd, _("File"), 1,
					   FALSE);

	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolwin), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scrolwin), pd->tg.lview);
	gtk_widget_set_size_request(scrolwin, 200, 200);
	gtk_box_pack_start(GTK_BOX(vbox1), scrolwin, TRUE, TRUE, 2);
	tmplist = g_list_first(pd->lists[templates]);
	while (tmplist) {
		gint arrcount;
		gchar **strarr = (gchar **) tmplist->data;
		arrcount = g_strv_length(strarr);
		if (arrcount == 2) {
			GtkTreeIter iter;
			gtk_list_store_append(GTK_LIST_STORE(pd->tg.lstore), &iter);
			set_template_strarr_in_list(&iter, strarr, pd);
		}
		tmplist = g_list_next(tmplist);
	}
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(pd->tg.lview), TRUE);
	pd->tg.thelist = &pd->lists[templates];
	pd->tg.insertloc = -1;
	g_signal_connect(G_OBJECT(pd->tg.lstore), "row-inserted", G_CALLBACK(listpref_row_inserted), &pd->tg);
	g_signal_connect(G_OBJECT(pd->tg.lstore), "row-deleted", G_CALLBACK(listpref_row_deleted), &pd->tg);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 2);
	but = dialog_button_new_with_image(_("Add entry"), GTK_STOCK_ADD, G_CALLBACK(add_new_template_lcb), pd, TRUE, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 2);
	but = dialog_button_new_with_image(_("Delete entry"), GTK_STOCK_DELETE, G_CALLBACK(delete_template_lcb), pd, TRUE, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 2);
}
#endif /*OLDTEMPLATES*/

/********* bflangdialog */
static gchar *
string_path_to_child_string_path(GtkTreeModelFilter * lfilter, const gchar * path)
{
	GtkTreePath *tpath, *ctpath;
	gchar *ret;
	tpath = gtk_tree_path_new_from_string(path);
	if (!tpath) {
		DEBUG_MSG("no tree path for string path %s\n", path);
		return NULL;
	}
	ctpath = gtk_tree_model_filter_convert_path_to_child_path(lfilter, tpath);
	gtk_tree_path_free(tpath);
	if (!ctpath) {
		DEBUG_MSG("no child path for string path %s\n", path);
		return NULL;
	}
	ret = gtk_tree_path_to_string(ctpath);
	gtk_tree_path_free(ctpath);
	DEBUG_MSG("return child path %s for path %s\n", ret, path);
	return ret;
}

static void
bflang_1_edited_lcb(GtkCellRendererToggle * cellrenderertoggle, gchar * path, Tprefdialog * pd)
{
	gchar *cpath, *val = g_strdup(gtk_cell_renderer_toggle_get_active(cellrenderertoggle) ? "0" : "1");
	cpath = string_path_to_child_string_path(pd->bld.lfilter, path);
	pref_apply_change(GTK_LIST_STORE(pd->bld.lstore), 3, 2, cpath, val, 2);
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

static void
bflang_highlight_edited_lcb(GtkCellRendererCombo * combo, gchar * path, gchar * val, Tprefdialog * pd)
{
	gchar *cpath = string_path_to_child_string_path(pd->bld.lfilter2, path);
	DEBUG_MSG("bflang_highlight_edited_lcb, val=%s\n", val);
	pref_apply_change(GTK_LIST_STORE(pd->bld.lstore2), 3, 1 /* 1 is text, not a combo */ , cpath, val, 2);
	g_free(cpath);
}

static void
bflanggui_set_bflang(Tprefdialog * pd, gpointer data)
{
	gchar *name;
	Tbflang *bflang = data;
	pd->bld.curbflang = bflang;
	if (bflang) {
		name = g_strdup_printf("<b>%s</b>",bflang->name);
		gtk_label_set_markup(GTK_LABEL(pd->bld.title), name);
		g_free(name);
	}
	gtk_tree_model_filter_refilter(pd->bld.lfilter);
	gtk_tree_model_filter_refilter(pd->bld.lfilter2);
}

static gboolean
bflang_gui_filter_func_lcb(GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
	gchar *name;
	Tprefdialog *pd = data;
	gboolean visible = FALSE;
	gtk_tree_model_get(model, iter, 0, &name, -1);
	if (name && pd->bld.curbflang && strcmp(name, pd->bld.curbflang->name) == 0)
		visible = TRUE;
	g_free(name);
	return visible;
}

static gint
sort_array2_lcb(gconstpointer a, gconstpointer b)
{
	const gchar **arra = (const gchar **) a, **arrb = (const gchar **) b;
	gint ret;
	if (a == NULL || b == NULL)
		return a - b;
	ret = g_strcmp0(arra[0], arrb[0]);
	if (ret != 0)
		return ret;
	return g_strcmp0(arra[1], arrb[1]);
}

static void
fill_bflang_gui(Tprefdialog * pd)
{
	GList *tmplist = g_list_first(g_list_sort(pd->lists[bflang_options], sort_array2_lcb));
	while (tmplist) {
		gchar **strarr = (gchar **) tmplist->data;
		if (g_strv_length(strarr) == 3) {
			GtkTreeIter iter;
			gchar *tmp;
			tmp = langmgr_get_option_description(strarr[1]);
			gtk_list_store_append(GTK_LIST_STORE(pd->bld.lstore), &iter);
			gtk_list_store_set(GTK_LIST_STORE(pd->bld.lstore), &iter, 0, strarr[0], 1,
								tmp ? tmp : strarr[1], 2,
							   strarr[2][0] == '1', 3, strarr, -1);
		}
		tmplist = g_list_next(tmplist);
	}

	tmplist = g_list_first(g_list_sort(pd->lists[highlight_styles], sort_array2_lcb));
	while (tmplist) {
		gchar **strarr = (gchar **) tmplist->data;
		/*g_print("found %s:%s:%s\n",strarr[0],strarr[1],strarr[2]); */
		if (g_strv_length(strarr) == 3) {
			GtkTreeIter iter;
			gtk_list_store_append(GTK_LIST_STORE(pd->bld.lstore2), &iter);
			gtk_list_store_set(GTK_LIST_STORE(pd->bld.lstore2), &iter, 0, strarr[0], 1, strarr[1], 2,
							   strarr[2], 3, strarr, -1);
		}
		tmplist = g_list_next(tmplist);
	}
}

static void
create_bflang_gui(Tprefdialog * pd, GtkWidget * vbox1)
{
	GtkWidget *scrolwin, *label;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	pd->lists[bflang_options] = duplicate_arraylist(main_v->props.bflang_options);
	pd->lists[highlight_styles] = duplicate_arraylist(main_v->props.highlight_styles);

	pd->bld.title = dialog_label_new(NULL, 0, 0, vbox1, 0);

	label = gtk_label_new(_("A restart is needed to see the effect of these options"));
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox1), label, FALSE, FALSE, 5);

	pd->bld.lstore = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_POINTER);
	pd->bld.lfilter = (GtkTreeModelFilter *) gtk_tree_model_filter_new(GTK_TREE_MODEL(pd->bld.lstore), NULL);
	DEBUG_MSG("create_bflang_gui, create lfilter at %p\n",pd->bld.lfilter );
	g_object_unref(G_OBJECT(pd->bld.lstore));
	gtk_tree_model_filter_set_visible_func(pd->bld.lfilter, bflang_gui_filter_func_lcb, pd, NULL);
	pd->bld.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->bld.lfilter));
	g_object_unref(G_OBJECT(pd->bld.lfilter));

	pref_create_column(GTK_TREE_VIEW(pd->bld.lview), 1 /* 1=text */ , NULL, NULL, _("Option name"), 1, FALSE);
	pref_create_column(GTK_TREE_VIEW(pd->bld.lview), 2 /* 2=boolean */ , G_CALLBACK(bflang_1_edited_lcb), pd,
					   _("value"), 2, FALSE);

	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolwin), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scrolwin), pd->bld.lview);
	gtk_box_pack_start(GTK_BOX(vbox1), scrolwin, TRUE, TRUE, 2);

	pd->bld.lstore2 = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
	pd->bld.lfilter2 =
		(GtkTreeModelFilter *) gtk_tree_model_filter_new(GTK_TREE_MODEL(pd->bld.lstore2), NULL);
	DEBUG_MSG("create_bflang_gui, create lfilter2 at %p\n",pd->bld.lfilter2);
	g_object_unref(G_OBJECT(pd->bld.lstore2));
	gtk_tree_model_filter_set_visible_func(pd->bld.lfilter2, bflang_gui_filter_func_lcb, pd, NULL);
	pd->bld.lview2 = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->bld.lfilter2));
	g_object_unref(G_OBJECT(pd->bld.lfilter2));

	pref_create_column(GTK_TREE_VIEW(pd->bld.lview2), 1 /* 1=text */ , NULL, NULL, _("Highlight name"), 1,
					   FALSE);
	renderer = gtk_cell_renderer_combo_new();
	g_object_set(G_OBJECT(renderer), "model", pd->tsd.lstore /* the textstyle model */ , "text-column", 0,
				 "has-entry", FALSE, "editable", TRUE, NULL);
	column = gtk_tree_view_column_new_with_attributes(_("Textstyle"), renderer, "text", 2, NULL);
	gtk_tree_view_column_set_clickable(GTK_TREE_VIEW_COLUMN(column), TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(pd->bld.lview2), column);
	g_signal_connect(G_OBJECT(renderer), "edited", G_CALLBACK(bflang_highlight_edited_lcb), pd);
	/*g_signal_connect(G_OBJECT(renderer), "changed", bflang_highlight_changed_lcb, pd); */

	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolwin), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scrolwin), pd->bld.lview2);
	gtk_box_pack_start(GTK_BOX(vbox1), scrolwin, TRUE, TRUE, 2);

	fill_bflang_gui(pd);
}

/**************************************/
/* MAIN DIALOG FUNCTIONS              */
/**************************************/

static void
destroy_widgets_in_freelist_lcb(gpointer child,gpointer user_data)
{
	Tprefdialog * pd = user_data;
	if (child && child != pd->curchild) {
		DEBUG_MSG("destroy_widgets_in_freelist_lcb, got child %p\n",child);
		if (g_object_is_floating(child)) {
			DEBUG_MSG("%p was floating, sink first\n",child);
			g_object_ref_sink(child);
		}
		g_object_unref(child);
	}
}

static void
preferences_destroy_lcb(GtkWidget * widget, Tprefdialog * pd)
{
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
#ifdef OLDTEMPLATES
	free_arraylist(pd->lists[templates]);
	pd->lists[templates] = NULL;
#endif
	free_arraylist(pd->lists[pluginconfig]);
	pd->lists[pluginconfig] = NULL;
	/*  g_signal_handlers_destroy(G_OBJECT(GTK_COMBO(pd->bd.combo)->list)); */
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->bd.lview));
	g_signal_handlers_destroy(G_OBJECT(select));
	/*  g_signal_handlers_destroy(G_OBJECT(GTK_COMBO(pd->ed.combo)->list)); */
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->ed.lview));
	g_signal_handlers_destroy(G_OBJECT(select));
	lingua_cleanup();

	DEBUG_MSG("preferences_destroy_lcb, about to destroy all widgets in the model\n");
	g_slist_foreach(pd->widgetfreelist,destroy_widgets_in_freelist_lcb,pd);

	DEBUG_MSG("preferences_destroy_lcb, about to destroy the window\n");
	window_destroy(pd->win);
	main_v->prefdialog = NULL;
	g_free(pd);
}

static void
preferences_apply(Tprefdialog * pd)
{
	GSList *tmpslist;
	DEBUG_MSG("preferences_apply, started\n");
	string_apply(&main_v->props.editor_font_string, pd->prefs[editor_font_string]);
	integer_apply(&main_v->props.cursor_size, pd->prefs[cursor_size], FALSE);
	integer_apply(&main_v->props.highlight_cursor, pd->prefs[highlight_cursor], TRUE);
	integer_apply(&main_v->props.editor_smart_cursor, pd->prefs[editor_smart_cursor], TRUE);
	main_v->props.editor_auto_close_brackets = gtk_combo_box_get_active(GTK_COMBO_BOX(pd->prefs[editor_auto_close_brackets]));
	integer_apply(&main_v->props.editor_tab_indent_sel, pd->prefs[editor_tab_indent_sel], TRUE);
	integer_apply(&main_v->props.smartindent, pd->prefs[smartindent], TRUE);
	integer_apply(&main_v->props.use_system_colors, pd->prefs[use_system_colors], TRUE);
	string_apply(&main_v->props.btv_color_str[BTV_COLOR_ED_FG], pd->prefs[editor_fg]);
	string_apply(&main_v->props.btv_color_str[BTV_COLOR_ED_BG], pd->prefs[editor_bg]);
	string_apply(&main_v->props.btv_color_str[BTV_COLOR_CURRENT_LINE], pd->prefs[cline_bg]);
	string_apply(&main_v->props.btv_color_str[BTV_COLOR_WHITESPACE], pd->prefs[visible_ws]);
	string_apply(&main_v->props.btv_color_str[BTV_COLOR_CURSOR], pd->prefs[cursor_color]);
	string_apply(&main_v->props.btv_color_str[BTV_COLOR_SELECTION], pd->prefs[selected_color]);
	string_apply(&main_v->props.btv_color_str[BTV_COLOR_CURSOR_HIGHLIGHT], pd->prefs[cursor_highlight_color]);
	integer_apply(&main_v->props.right_margin_pos, pd->prefs[right_margin_pos], FALSE);
	integer_apply(&main_v->props.wrap_on_right_margin, pd->prefs[wrap_on_right_margin], TRUE);

	main_v->props.visible_ws_mode = gtk_combo_box_get_active(GTK_COMBO_BOX(pd->prefs[visible_ws_mode]));
	/*integer_apply(&main_v->props.defaulthighlight, pd->prefs[defaulthighlight], TRUE); */

	sessionprefs_apply(&pd->sprefs, main_v->session);

	string_apply(&main_v->props.newfile_default_encoding, pd->prefs[newfile_default_encoding]);
	integer_apply(&main_v->props.strip_trailing_spaces_on_save, pd->prefs[strip_trailing_spaces_on_save], TRUE);
	integer_apply(&main_v->props.auto_set_encoding_meta, pd->prefs[auto_set_encoding_meta], TRUE);
	integer_apply(&main_v->props.backup_file, pd->prefs[backup_file], TRUE);
	/*  string_apply(&main_v->props.backup_suffix, pd->prefs[backup_suffix]);
	   string_apply(&main_v->props.backup_prefix, pd->prefs[backup_prefix]); */
	main_v->props.backup_abort_action =
		gtk_combo_box_get_active(GTK_COMBO_BOX(pd->prefs[backup_abort_action]));
	integer_apply(&main_v->props.backup_cleanuponclose, pd->prefs[backup_cleanuponclose], TRUE);

	integer_apply(&main_v->props.autosave, pd->prefs[autosave], TRUE);
	integer_apply(&main_v->props.autosave_time, pd->prefs[autosave_time], FALSE);

	integer_apply(&main_v->props.num_undo_levels, pd->prefs[num_undo_levels], FALSE);
	integer_apply(&main_v->props.clear_undo_on_save, pd->prefs[clear_undo_on_save], TRUE);
#ifndef WIN32
	integer_apply(&main_v->props.open_in_running_bluefish, pd->prefs[open_in_running_bluefish], TRUE);
	integer_apply(&main_v->props.open_in_new_window, pd->prefs[open_in_new_window], TRUE);
#endif							/* ifndef WIN32 */
	integer_apply(&main_v->props.show_long_line_warning, pd->prefs[show_long_line_warning], TRUE);
	main_v->props.recent_means_recently_closed =
		gtk_combo_box_get_active(GTK_COMBO_BOX(pd->prefs[recent_means_recently_closed]));
	main_v->props.register_recent_mode =
		gtk_combo_box_get_active(GTK_COMBO_BOX(pd->prefs[register_recent_mode]));
	main_v->props.modified_check_type =
		gtk_combo_box_get_active(GTK_COMBO_BOX(pd->prefs[modified_check_type]));
	integer_apply(&main_v->props.do_periodic_check, pd->prefs[do_periodic_check], TRUE);
	integer_apply(&main_v->props.max_recent_files, pd->prefs[max_recent_files], FALSE);
	integer_apply(&main_v->props.leave_to_window_manager, pd->prefs[leave_to_window_manager], TRUE);
	integer_apply(&main_v->props.restore_dimensions, pd->prefs[restore_dimensions], TRUE);
	if (!main_v->props.restore_dimensions) {
		integer_apply(&main_v->globses.left_panel_width, pd->prefs[left_panel_width], FALSE);
		integer_apply(&main_v->globses.main_window_h, pd->prefs[main_window_h], FALSE);
		integer_apply(&main_v->globses.main_window_w, pd->prefs[main_window_w], FALSE);
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pd->prefs[max_window_title])) && main_v->props.max_window_title==0) {
		main_v->props.max_window_title=120;
	} else if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pd->prefs[max_window_title]))) {
		main_v->props.max_window_title=0;
	}

	integer_apply(&main_v->props.use_system_tab_font, pd->prefs[use_system_tab_font], TRUE);
	integer_apply(&main_v->props.max_shown_filename_len, pd->prefs[max_shown_filename_len], FALSE);
	string_apply(&main_v->props.tab_font_string, pd->prefs[tab_font_string]);
	string_apply(&main_v->props.tab_color_modified, pd->prefs[tab_color_modified]);
	string_apply(&main_v->props.tab_color_loading, pd->prefs[tab_color_loading]);
	string_apply(&main_v->props.tab_color_error, pd->prefs[tab_color_error]);
	main_v->props.document_tabposition =
		gtk_combo_box_get_active(GTK_COMBO_BOX(pd->prefs[document_tabposition]));
	integer_apply(&main_v->props.switch_tabs_by_altx, pd->prefs[switch_tabs_by_altx], TRUE);
	integer_apply(&main_v->props.hide_bars_on_fullscreen, pd->prefs[hide_bars_on_fullscreen], TRUE);
	main_v->props.leftpanel_tabposition =
		gtk_combo_box_get_active(GTK_COMBO_BOX(pd->prefs[leftpanel_tabposition]));
	main_v->props.left_panel_left = gtk_combo_box_get_active(GTK_COMBO_BOX(pd->prefs[left_panel_left]));
#ifndef MAC_INTEGRATION
	integer_apply(&main_v->props.save_accelmap, pd->prefs[save_accelmap], TRUE);
#endif
	string_apply(&main_v->props.language, pd->prefs[language]);
	if (g_strcmp0(main_v->props.language, _("Auto")) == 0) {
		g_free(main_v->props.language);
		main_v->props.language = NULL;
	} else {
		main_v->props.language = lingua_lang_to_locale(main_v->props.language);
	}

	string_apply(&main_v->props.image_thumbnailstring, pd->prefs[image_thumbnailstring]);
	string_apply(&main_v->props.image_thumbnailtype, pd->prefs[image_thumbnailtype]);

	main_v->props.autocomp_popup_mode =
		gtk_combo_box_get_active(GTK_COMBO_BOX(pd->prefs[autocomp_popup_mode]));
	integer_apply(&main_v->props.autocomp_min_prefix_len, pd->prefs[autocomp_min_prefix_len], FALSE);

	if (main_v->props.autocomp_accel_string)
		g_free(main_v->props.autocomp_accel_string);
	main_v->props.autocomp_accel_string =
		g_strdup(gtk_button_get_label(GTK_BUTTON(pd->prefs[autocomp_accel_string])));

	integer_apply(&main_v->props.load_reference, pd->prefs[load_reference], TRUE);
	integer_apply(&main_v->props.show_autocomp_reference, pd->prefs[show_autocomp_reference], TRUE);
	integer_apply(&main_v->props.show_tooltip_reference, pd->prefs[show_tooltip_reference], TRUE);

	for (tmpslist=main_v->pref_apply;tmpslist;tmpslist=g_slist_next(tmpslist)) {
		PrefApplyCallback func = tmpslist->data;
		func();
	}


	free_arraylist(main_v->props.plugin_config);
	main_v->props.plugin_config = duplicate_arraylist(pd->lists[pluginconfig]);

	free_arraylist(main_v->props.external_command);
	main_v->props.external_command = duplicate_arraylist(pd->lists[extcommands]);
	free_arraylist(main_v->props.external_filter);
	main_v->props.external_filter = duplicate_arraylist(pd->lists[extfilters]);
	free_arraylist(main_v->props.external_outputbox);
	main_v->props.external_outputbox = duplicate_arraylist(pd->lists[extoutputbox]);
#ifdef OLDTEMPLATES
	free_arraylist(main_v->props.templates);
	main_v->props.templates = duplicate_arraylist(pd->lists[templates]);
#endif
	DEBUG_MSG("preferences_apply: free old textstyles, and building new list\n");
	free_arraylist(main_v->props.textstyles);
	main_v->props.textstyles = duplicate_arraylist(pd->lists[textstyles]);
	free_arraylist(main_v->props.highlight_styles);
	main_v->props.highlight_styles = duplicate_arraylist(pd->lists[highlight_styles]);

	free_arraylist(main_v->props.bflang_options);
	main_v->props.bflang_options = duplicate_arraylist(pd->lists[bflang_options]);

	/* apply the changes to highlighting patterns and filetypes to the running program */
	bftextview2_init_globals();
	langmgr_reload_user_options();
	langmgr_reload_user_styles();
	langmgr_reload_user_highlights();
	modified_on_disk_check_init();
	autosave_init(FALSE, NULL);
	all_documents_apply_settings();
	{
		GList *tmplist = g_list_first(main_v->bfwinlist);
		while (tmplist) {
			Tbfwin *bfwin = BFWIN(tmplist->data);
			DEBUG_MSG("preferences_ok_clicked_lcb, calling encoding_menu_rebuild\n");
			lang_mode_menu_create(bfwin);
			bfwin_commands_menu_create(bfwin);
			bfwin_filters_menu_create(bfwin);
			bfwin_outputbox_menu_create(bfwin);
#ifdef OLDTEMPLATES
			bfwin_templates_menu_create(bfwin);
#endif
			DEBUG_MSG("preferences_ok_clicked_lcb, calling gui_apply_settings\n");
			bfwin_apply_settings(bfwin);
			bfwin_side_panel_rebuild(bfwin);
			DEBUG_MSG("preferences_ok_clicked_lcb, calling doc_force_activate\n");
			if (bfwin->current_document)
				doc_force_activate(bfwin->current_document);
#ifdef MAC_INTEGRATION
/*			ige_mac_menu_sync(GTK_MENU_SHELL(BFWIN(bfwin)->menubar));*/
			gtkosx_application_sync_menubar(g_object_new(GTKOSX_TYPE_APPLICATION, NULL));
#endif
			tmplist = g_list_next(tmplist);
		}
	}
	rcfile_save_main();
}

static void
preferences_cancel_clicked_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	preferences_destroy_lcb(NULL, pd);
}

static void
preferences_apply_clicked_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	preferences_apply(pd);
}

static void
preferences_ok_clicked_lcb(GtkWidget * wid, Tprefdialog * pd)
{
	preferences_apply(pd);
	preferences_destroy_lcb(NULL, pd);
}

static void
leave_to_window_manager_toggled_lcb(GtkToggleButton * togglebutton, gpointer user_data)
{
	gtk_widget_set_sensitive(GTK_WIDGET(user_data), !gtk_toggle_button_get_active(togglebutton));
}

static void
restore_dimensions_toggled_lcb(GtkToggleButton * togglebutton, Tprefdialog * pd)
{
	gtk_widget_set_sensitive(pd->prefs[left_panel_width], !gtk_toggle_button_get_active(togglebutton));
	gtk_widget_set_sensitive(pd->prefs[main_window_h], !gtk_toggle_button_get_active(togglebutton));
	gtk_widget_set_sensitive(pd->prefs[main_window_w], !gtk_toggle_button_get_active(togglebutton));
}

static void
load_reference_toggled_lcb(GtkToggleButton * togglebutton, Tprefdialog * pd)
{
	if (!gtk_toggle_button_get_active(togglebutton)) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->prefs[show_autocomp_reference]), FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->prefs[show_tooltip_reference]), FALSE);
	}

	gtk_widget_set_sensitive(pd->prefs[show_autocomp_reference], gtk_toggle_button_get_active(togglebutton));
	gtk_widget_set_sensitive(pd->prefs[show_tooltip_reference], gtk_toggle_button_get_active(togglebutton));
}

static void
prefs_togglebutton_toggled_lcb(GtkToggleButton * togglebutton, gpointer user_data)
{
	gtk_widget_set_sensitive(GTK_WIDGET(user_data), gtk_toggle_button_get_active(togglebutton));
}

static void
prefs_togglebutton_toggled_not_lcb(GtkToggleButton * togglebutton, gpointer user_data)
{
	gtk_widget_set_sensitive(GTK_WIDGET(user_data), !gtk_toggle_button_get_active(togglebutton));
}

void
preftree_cursor_changed_cb(GtkTreeView * treeview, gpointer user_data)
{
	Tprefdialog *pd = (Tprefdialog *) user_data;
	GtkTreePath *path;
	GtkTreeIter iter;
	gpointer child;
	GList *lst;

	lst = gtk_container_get_children(GTK_CONTAINER(pd->fixed));
	while (lst) {
		if (GTK_IS_WIDGET(lst->data)) {
			DEBUG_MSG("preftree_cursor_changed_cb, ref %p\n",lst->data);
			g_object_ref(G_OBJECT(lst->data));
			gtk_container_remove(GTK_CONTAINER(pd->fixed), lst->data);
		}
		lst = g_list_next(lst);
	}
	pd->curchild = NULL;

	gtk_tree_view_get_cursor(treeview, &path, NULL);
	if (path) {
		gpointer data = NULL;
		void
		 (*func) ();
		gtk_tree_model_get_iter(GTK_TREE_MODEL(pd->nstore), &iter, path);
		gtk_tree_model_get(GTK_TREE_MODEL(pd->nstore), &iter, WIDGETCOL, &child, FUNCCOL, &func, DATACOL,
						   &data, -1);
		DEBUG_MSG("preftree_cursor_changed_cb, got child %p\n",child);
		if (child) {
			pd->curchild = child;
			gtk_box_pack_start(GTK_BOX(pd->fixed), child, TRUE, TRUE, 1);
			gtk_widget_show_all(pd->fixed);
			if (func && data) {
				func(pd, data);
			}
		}
		gtk_tree_path_free(path);
	}
}

#ifndef WIN32
static void
open_in_running_bluefish_toggled_lcb(GtkWidget * widget, Tprefdialog * pd)
{
	gtk_widget_set_sensitive(pd->prefs[open_in_new_window],
							 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}
#endif							/* ifndef WIN32 */

static void
use_system_colors_toggled_lcb(GtkWidget * widget, Tprefdialog * pd)
{
	gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	gtk_widget_set_sensitive(pd->prefs[editor_fg], !active);
	gtk_widget_set_sensitive(pd->prefs[editor_bg], !active);
	gtk_widget_set_sensitive(pd->prefs[cursor_color], !active);
	gtk_widget_set_sensitive(pd->prefs[selected_color], !active);
}

static void
highlight_cursor_toggled_lcb(GtkWidget * widget, Tprefdialog * pd)
{
	gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	gtk_widget_set_sensitive(pd->prefs[cursor_highlight_color], active);
}

static gint
prefs_combo_box_get_index_from_text(const gchar ** options, const gchar * string)
{
	gboolean found = FALSE;
	gint index = 0;
	const gchar **tmp = options;

	while (*tmp) {
		if (g_strcmp0(*tmp++, string) == 0) {
			found = TRUE;
			break;
		} else
			index++;
	}

	return (found ? index : 0);
}

static void save_user_menu_accelerators(GObject *object, gpointer data)
{
	rcfile_save_accelerators();
}

static void reset_menu_accelerators(GObject *object, gpointer data)
{
	rcfile_load_accelerators(TRUE);
}

void
preferences_dialog_new(void)
{
	Tprefdialog *pd;
	gint index;
	GList *tmplist, *poplist, *freelist;
	GSList *tmpslist;
	GtkWidget *dvbox, *frame, *hbox, *label, *table, *vbox1, *vbox2, *vbox3;
	GtkWidget *dhbox, *scrolwin, *but;
	GtkCellRenderer *cell;
	GtkTreeIter auxit, iter;
	GtkTreePath *path;
	GtkTreeViewColumn *column;

	const gchar *autocompmodes[] = { N_("Delayed"), N_("Immediately"), NULL };
	const gchar *failureactions[] = { N_("Continue save"), N_("Abort save"), N_("Ask what to do"), NULL };
	const gchar *modified_check_types[] =
		{ N_("Nothing"), N_("Modified time and file size"), N_("Modified time"), N_("File size"), NULL };
	const gchar *notebooktabpositions[] = { N_("left"), N_("right"), N_("top"), N_("bottom"), NULL };
	const gchar *panellocations[] = { N_("right"), N_("left"), NULL };
	const gchar *recentmeaningmode[] = {N_("Recently opened"), N_("Recently closed"), NULL};
	const gchar *registerrecentmodes[] = { N_("Never"), N_("All files"), N_("Only project files"), NULL };
	const gchar *thumbnail_filetype[] = { "jpeg", "png", NULL }; /* values should be lowercase, othewise image.c will not get them */
	const gchar *visible_ws_modes[] =
		{ N_("All"), N_("All except spaces"), N_("All trailing"), N_("All except non-trailing spaces"),
		NULL
	};
	const gchar *autobracketclosemodes[] = {N_("Never"), N_("Always"), N_("Smart"), NULL};

	if (main_v->prefdialog) {
		pd = (Tprefdialog *) main_v->prefdialog;
		/* bring window to focus ?? */
		gtk_window_present(GTK_WINDOW(pd->win));
		return;
	}

	main_v->prefdialog = pd = g_new0(Tprefdialog, 1);
	pd->win =
		window_full(_("Edit preferences"), GTK_WIN_POS_CENTER, 6, G_CALLBACK(preferences_destroy_lcb), pd,
					TRUE);

	dvbox = gtk_vbox_new(FALSE, 5);
	dhbox = gtk_hbox_new(FALSE, 5);
	pd->fixed = gtk_hbox_new(FALSE, 5);
	pd->nstore = gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER);
	pd->noteb = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->nstore));
	g_object_unref(pd->nstore);
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolwin), pd->noteb);
	gtk_box_pack_start(GTK_BOX(dhbox), scrolwin, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(dhbox), pd->fixed, TRUE, TRUE, 5);
	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", cell, "text", NAMECOL, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(pd->noteb), column);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(pd->noteb), FALSE);
	gtk_box_pack_start(GTK_BOX(dvbox), dhbox, TRUE, TRUE, 5);
	gtk_container_add(GTK_CONTAINER(pd->win), dvbox);

	/*
	 *  Initial document settings
	 */
	sessionprefs(_("<b>Non Project Defaults</b>"), &pd->sprefs, main_v->session);
	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL, _("Initial document settings"), WIDGETCOL, /*vbox1*/pd->sprefs.frame, -1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, /*vbox1*/pd->sprefs.frame);

	/*
	 *  Editor settings
	 */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	vbox1 = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 6);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);
	gtk_tree_store_append(pd->nstore, &iter, NULL);
	gtk_tree_store_set(pd->nstore, &iter, NAMECOL, _("Editor settings"), WIDGETCOL, frame, -1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, frame);

	vbox2 = dialog_vbox_labeled(_("<b>Editor settings</b>"), vbox1);

	table = dialog_table_in_vbox_defaults(5, 2, 0, vbox2);

	pd->prefs[smartindent] =
		dialog_check_button_in_table(_("Smart auto indentin_g"), main_v->props.smartindent, table, 0, 1, 0,
									 1);
	pd->prefs[editor_smart_cursor] =
		dialog_check_button_in_table(_("Smart Home/_End cursor positioning"),
									 main_v->props.editor_smart_cursor, table, 0, 1, 1, 2);

	pd->prefs[editor_tab_indent_sel] =
		dialog_check_button_in_table(_("_Tab key indents selection"), main_v->props.editor_tab_indent_sel,
									 table, 0, 1, 2, 3);

	pd->prefs[editor_auto_close_brackets] = dialog_combo_box_text_in_table(autobracketclosemodes,
																	main_v->props.editor_auto_close_brackets, table,
																	1, 2, 3, 4);
	dialog_mnemonic_label_in_table(_("Automatically insert closing brackets"), pd->prefs[editor_auto_close_brackets], table, 0, 1, 3,
								   4);

	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	pd->prefs[right_margin_pos] =
		dialog_spin_button_labeled(1, 500, main_v->props.right_margin_pos,
								   _("Right _margin/split line end position:"), hbox, 0);

	pd->prefs[wrap_on_right_margin] =
		boxed_checkbut_with_value(_("Wrap lines on right margin indicator"),
								  main_v->props.wrap_on_right_margin, vbox2);

	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	pd->prefs[visible_ws_mode] =
		dialog_combo_box_text_labeled(_("Visible _whitespace mode:"), visible_ws_modes,
									  main_v->props.visible_ws_mode, hbox, 0);

	vbox2 = dialog_vbox_labeled(_("<b>Undo</b>"), vbox1);
	table = dialog_table_in_vbox_defaults(2, 2, 0, vbox2);

	pd->prefs[num_undo_levels]
		= dialog_spin_button_in_table(50, 1000, main_v->props.num_undo_levels, table, 1, 2, 0, 1);
	dialog_mnemonic_label_in_table(_("_Number of actions in history:"), pd->prefs[num_undo_levels], table, 0,
								   1, 0, 1);
	pd->prefs[clear_undo_on_save] =
				dialog_check_button_in_table(_("Clear _history on save"), main_v->props.clear_undo_on_save, table, 0,1, 1, 2);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_tree_store_append(pd->nstore, &auxit, &iter);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL, _("Auto-completion & Reference"), WIDGETCOL, frame, -1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, frame);
	vbox1 = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 6);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);

	vbox2 = dialog_vbox_labeled(_("<b>Auto-completion</b>"), vbox1);
	table = dialog_table_in_vbox_defaults(3, 2, 0, vbox2);

	pd->prefs[autocomp_popup_mode] = dialog_combo_box_text_in_table(autocompmodes,
																	main_v->props.autocomp_popup_mode, table,
																	1, 2, 0, 1);
	dialog_mnemonic_label_in_table(_("Show _pop-up window:"), pd->prefs[autocomp_popup_mode], table, 0, 1, 0,
								   1);
	pd->prefs[autocomp_min_prefix_len]
		= dialog_spin_button_in_table(1, 10, main_v->props.autocomp_min_prefix_len, table, 1, 2, 1, 2);
	dialog_mnemonic_label_in_table(_("Minimum matching length for auto-completion:"), pd->prefs[autocomp_min_prefix_len], table, 0,1,1,2);

	pd->prefs[autocomp_accel_string] = accelerator_button(main_v->props.autocomp_accel_string);
	gtk_table_attach(GTK_TABLE(table), pd->prefs[autocomp_accel_string], 1, 2, 2, 3, GTK_FILL, GTK_SHRINK, 0,
					 0);
	dialog_mnemonic_label_in_table(_("Shortcut _key combination:"), pd->prefs[autocomp_accel_string], table,
								   0, 1, 2, 3);

	vbox2 = dialog_vbox_labeled(_("<b>Reference Information</b>"), vbox1);
	table = dialog_table_in_vbox_defaults(3, 1, 0, vbox2);

	pd->prefs[load_reference] = dialog_check_button_in_table(_("_Load reference data from language files"),
															 main_v->props.load_reference, table, 0, 1, 0, 1);
	pd->prefs[show_autocomp_reference] =
		dialog_check_button_in_table(_("Show references in auto-completion _pop-up window"),
									 main_v->props.show_autocomp_reference, table, 0, 1, 1, 2);
	pd->prefs[show_tooltip_reference] =
		dialog_check_button_in_table(_("Show references if mouse is over text editor"), main_v->props.show_tooltip_reference,
									 table, 0, 1, 2, 3);
	load_reference_toggled_lcb(GTK_TOGGLE_BUTTON(pd->prefs[load_reference]), pd);
	g_signal_connect(G_OBJECT(pd->prefs[load_reference]), "toggled", G_CALLBACK(load_reference_toggled_lcb),
					 pd);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_tree_store_append(pd->nstore, &auxit, &iter);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL, _("Font & Colors"), WIDGETCOL, frame, -1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, frame);

	vbox1 = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 6);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);
	vbox2 = dialog_vbox_labeled(_("<b>Font &amp; Cursor</b>"), vbox1);

	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	label = dialog_label_new(_("_Editor font:"), 0, 0.5, hbox, 0);
	pd->prefs[editor_font_string] = gtk_font_button_new_with_font(main_v->props.editor_font_string);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), pd->prefs[editor_font_string]);
	gtk_box_pack_start(GTK_BOX(hbox), pd->prefs[editor_font_string], FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	pd->prefs[cursor_size] = dialog_spin_button_labeled(1, 999, main_v->props.cursor_size,
								   _("Cursor / font size ratio (%):"), hbox, 0);

	vbox2 = dialog_vbox_labeled(_("<b>Colors</b>"), vbox1);
	table = dialog_table_in_vbox_defaults(5, 4, 0, vbox2);

	pd->prefs[use_system_colors] =
		dialog_check_button_in_table(_("Use system wide color settings"), main_v->props.use_system_colors, table, 0,2, 0,1);

	pd->prefs[editor_fg] = dialog_color_button_in_table(main_v->props.btv_color_str[BTV_COLOR_ED_FG],
														_("Foreground color"), table, 1, 2, 1,2);
	dialog_mnemonic_label_in_table(_("_Foreground color:"), pd->prefs[editor_fg], table, 0, 1, 1,2);

	pd->prefs[editor_bg] = dialog_color_button_in_table(main_v->props.btv_color_str[BTV_COLOR_ED_BG],
														_("Background color"), table, 1, 2, 2,3);
	dialog_mnemonic_label_in_table(_("_Background color:"), pd->prefs[editor_bg], table, 0, 1, 2,3);

	pd->prefs[cursor_color] = dialog_color_button_in_table(main_v->props.btv_color_str[BTV_COLOR_CURSOR],
														   _("Cursor color"), table, 1, 2, 3,4);
	dialog_mnemonic_label_in_table(_("C_ursor color:"), pd->prefs[cursor_color], table, 0, 1, 3,4);

	pd->prefs[selected_color] = dialog_color_button_in_table(main_v->props.btv_color_str[BTV_COLOR_SELECTION],
														_("Selection background color"), table, 1, 2, 4,5);
	dialog_mnemonic_label_in_table(_("_Selection background color:"), pd->prefs[selected_color], table, 0, 1, 4,5);

	g_signal_connect(G_OBJECT(pd->prefs[use_system_colors]), "toggled", G_CALLBACK(use_system_colors_toggled_lcb), pd);
	use_system_colors_toggled_lcb(pd->prefs[use_system_colors], pd);

	pd->prefs[highlight_cursor] = dialog_check_button_in_table(_("Highlight cursor position"), main_v->props.highlight_cursor, table, 2,4, 0,1);
	pd->prefs[cursor_highlight_color] = dialog_color_button_in_table(main_v->props.btv_color_str[BTV_COLOR_CURSOR_HIGHLIGHT],
														 _("Cursor highlight color"), table, 3,4, 1,2);
	dialog_mnemonic_label_in_table(_("_Cursor highlight color:"), pd->prefs[cursor_highlight_color], table, 2,3, 1,2);
	g_signal_connect(G_OBJECT(pd->prefs[highlight_cursor]), "toggled", G_CALLBACK(highlight_cursor_toggled_lcb), pd);
	highlight_cursor_toggled_lcb(pd->prefs[highlight_cursor], pd);

	pd->prefs[cline_bg] = dialog_color_button_in_table(main_v->props.btv_color_str[BTV_COLOR_CURRENT_LINE],
													   _("Current line color"), table, 3,4, 2,3);
	dialog_mnemonic_label_in_table(_("Cu_rrent line color:"), pd->prefs[cline_bg], table, 2,3, 2,3);

	pd->prefs[visible_ws] = dialog_color_button_in_table(main_v->props.btv_color_str[BTV_COLOR_WHITESPACE],
														 _("Visible whitespace color"), table, 3,4, 3,4);
	dialog_mnemonic_label_in_table(_("_Visible whitespace color:"), pd->prefs[visible_ws], table, 2,3, 3,4);

	/*
	 *  Text Styles
	 */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	vbox1 = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 6);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);

	gtk_tree_store_append(pd->nstore, &auxit, &iter);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL, _("Text styles"), WIDGETCOL, frame, -1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, frame);
	dialog_label_new(_("<b>Text Styles</b>"), 0, 0, vbox1, 4);
	create_textstyle_gui(pd, vbox1);

	/*
	 *  Files
	 */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	vbox1 = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 6);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);

	gtk_tree_store_append(pd->nstore, &iter, NULL);
	gtk_tree_store_set(pd->nstore, &iter, NAMECOL, _("Files"), WIDGETCOL, frame, -1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, frame);

	vbox2 = dialog_vbox_labeled(_("<b>Encoding</b>"), vbox1);

	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	poplist = NULL;
	tmplist = g_list_first(main_v->globses.encodings);
	while (tmplist) {
		gchar **strarr = (gchar **) tmplist->data;
		poplist = g_list_append(poplist, strarr[1]);
		tmplist = g_list_next(tmplist);
	}
	poplist = g_list_sort(poplist, (GCompareFunc) g_strcmp0);
	pd->prefs[newfile_default_encoding] = dialog_combo_box_text_labeled_from_list(poplist,
																				  main_v->
																				  props.newfile_default_encoding,
																				  _
																				  ("_Default character set for new files:"),
																				  hbox, 0);
	g_list_free(poplist);
	poplist = NULL;

	pd->prefs[auto_set_encoding_meta] =
		dialog_check_button_new(_("Auto set <meta> _HTML tag on encoding change"),
								main_v->props.auto_set_encoding_meta);
	gtk_box_pack_start(GTK_BOX(vbox2), pd->prefs[auto_set_encoding_meta], FALSE, FALSE, 0);

	vbox2 = dialog_vbox_labeled(_("<b>Misc</b>"), vbox1);

	pd->prefs[strip_trailing_spaces_on_save] =
		dialog_check_button_new(_("Strip trailing spaces on save"),
								main_v->props.strip_trailing_spaces_on_save);
	gtk_box_pack_start(GTK_BOX(vbox2), pd->prefs[strip_trailing_spaces_on_save], FALSE, FALSE, 0);

#ifndef WIN32
	pd->prefs[open_in_running_bluefish] =
		boxed_checkbut_with_value(_("Open co_mmandline files in running bluefish process"),
								  main_v->props.open_in_running_bluefish, vbox2);
	g_signal_connect(pd->prefs[open_in_running_bluefish], "toggled",
					 G_CALLBACK(open_in_running_bluefish_toggled_lcb), pd);
	pd->prefs[open_in_new_window] =
		boxed_checkbut_with_value(_("Open commandline files in new _window"),
								  main_v->props.open_in_new_window, vbox2);
	gtk_widget_set_sensitive(pd->prefs[open_in_new_window], main_v->props.open_in_running_bluefish);
#endif							/* ifndef WIN32 */
	pd->prefs[do_periodic_check] =
		boxed_checkbut_with_value(_("_Periodically check if file is modified on disk"),
								  main_v->props.do_periodic_check, vbox2);

	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	pd->prefs[modified_check_type] =
		dialog_combo_box_text_labeled(_("File properties to check on dis_k for modifications:"),
									  modified_check_types, main_v->props.modified_check_type, hbox, 0);

	pd->prefs[show_long_line_warning] =
		boxed_checkbut_with_value(_("Show warning for files with very long lines"),
								  main_v->props.show_long_line_warning, vbox2);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	vbox1 = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 6);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);

	gtk_tree_store_append(pd->nstore, &auxit, &iter);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL, _("Backup & Recovery"), WIDGETCOL, frame, -1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, frame);

	vbox2 = dialog_vbox_labeled(_("<b>Backup</b>"), vbox1);

	pd->prefs[backup_file] = dialog_check_button_new(_("Create _backup file during file save"),
													 main_v->props.backup_file);
	gtk_box_pack_start(GTK_BOX(vbox2), pd->prefs[backup_file], FALSE, FALSE, 0);
	vbox3 = gtk_vbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox2), vbox3, FALSE, FALSE, 0);
	pd->prefs[backup_cleanuponclose] = dialog_check_button_new(_("_Remove backup file on close"),
															   main_v->props.backup_cleanuponclose);
	gtk_box_pack_start(GTK_BOX(vbox3), pd->prefs[backup_cleanuponclose], FALSE, FALSE, 0);
	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox3), hbox, FALSE, FALSE, 0);
	pd->prefs[backup_abort_action] = dialog_combo_box_text_labeled(_("If back_up fails:"), failureactions,
																   main_v->props.backup_abort_action, hbox,
																   0);
	prefs_togglebutton_toggled_lcb(GTK_TOGGLE_BUTTON(pd->prefs[backup_file]), vbox3);
	g_signal_connect(G_OBJECT(pd->prefs[backup_file]), "toggled", G_CALLBACK(prefs_togglebutton_toggled_lcb),
					 vbox3);

	vbox2 = dialog_vbox_labeled(_("<b>Document Recovery</b>"), vbox1);
	pd->prefs[autosave] =
		dialog_check_button_new(_("_Enable recovery of modified documents"), main_v->props.autosave);
	gtk_box_pack_start(GTK_BOX(vbox2), pd->prefs[autosave], FALSE, FALSE, 0);
	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	label = dialog_label_new(_("_Frequency to store changes (seconds):"), 0, 0.5, hbox, 0);
	pd->prefs[autosave_time] = dialog_spin_button_new(1, 600, main_v->props.autosave_time);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), pd->prefs[autosave_time]);
	gtk_box_pack_start(GTK_BOX(hbox), pd->prefs[autosave_time], FALSE, FALSE, 0);
	prefs_togglebutton_toggled_lcb(GTK_TOGGLE_BUTTON(pd->prefs[autosave]), hbox);
	g_signal_connect(G_OBJECT(pd->prefs[autosave]), "toggled", G_CALLBACK(prefs_togglebutton_toggled_lcb),
					 hbox);
#ifdef OLDTEMPLATES
	/*
	 *  Templates
	 */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	vbox1 = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 6);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);

	gtk_tree_store_append(pd->nstore, &auxit, &iter);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL, _("Templates"), WIDGETCOL, frame, -1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, frame);

	vbox2 = dialog_vbox_labeled(_("<b>Templates</b>"), vbox1);
	create_template_gui(pd, vbox2);
#endif
	/*
	 *  User Interface
	 */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	vbox1 = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 6);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);

	gtk_tree_store_append(pd->nstore, &iter, NULL);
	gtk_tree_store_set(pd->nstore, &iter, NAMECOL, _("User interface"), WIDGETCOL, frame, -1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, frame);

	vbox2 = dialog_vbox_labeled(_("<b>Layout</b>"), vbox1);
	table = dialog_table_in_vbox_defaults(4, 2, 0, vbox2);

	pd->prefs[document_tabposition] = dialog_combo_box_text_in_table(notebooktabpositions,
								main_v->props.document_tabposition,
								table,1,2,0,1);
	dialog_mnemonic_label_in_table(_("Document _tab position:"), pd->prefs[left_panel_left],
								table, 0, 1, 0, 1);

	pd->prefs[max_shown_filename_len] = dialog_spin_button_in_table(0, 1000, main_v->props.max_shown_filename_len,
								table,1, 2, 1,2);
	dialog_mnemonic_label_in_table(_("Maximum filename length shown in tab (0 is no limit)"), pd->prefs[max_shown_filename_len],
								table, 0, 1, 1, 2);

	pd->prefs[left_panel_left] =
		dialog_combo_box_text_in_table(panellocations, main_v->props.left_panel_left,
								table, 1, 2, 2,3);
	dialog_mnemonic_label_in_table(_("_Sidebar position:"), pd->prefs[left_panel_left],
								table, 0, 1, 2,3);

	pd->prefs[leftpanel_tabposition] = dialog_combo_box_text_in_table(notebooktabpositions,
																	  main_v->props.leftpanel_tabposition,
																	  table, 1, 2, 3,4);
	dialog_mnemonic_label_in_table(_("Sidebar tab _position:"), pd->prefs[leftpanel_tabposition], table, 0, 1, 3,4);

	vbox2 = dialog_vbox_labeled(_("<b>Misc</b>"), vbox1);
	pd->prefs[hide_bars_on_fullscreen] =
		dialog_check_button_new(_("Hide toolbars on full-screen"), main_v->props.hide_bars_on_fullscreen);
	gtk_box_pack_start(GTK_BOX(vbox2), pd->prefs[hide_bars_on_fullscreen], FALSE, FALSE, 0);
	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	poplist = lingua_list_sorted();
	pd->prefs[language] = dialog_combo_box_text_labeled_from_list(poplist, (main_v->props.language
																			&& main_v->props.language[0]) ?
																  lingua_locale_to_lang(main_v->
																						props.language) :
																  _("Auto"), _("_Language:"), hbox, 0);
	free_stringlist(poplist);

	vbox2 = dialog_vbox_labeled(_("<b>Recent Files</b>"), vbox1);
	table = dialog_table_in_vbox_defaults(3, 4, 0, vbox2);

	pd->prefs[recent_means_recently_closed] =
		dialog_combo_box_text_in_table(recentmeaningmode, main_v->props.recent_means_recently_closed, table, 1, 4,0, 1);
	dialog_mnemonic_label_in_table(_("Recent files _shows:"),
								   pd->prefs[recent_means_recently_closed], table, 0, 1, 0, 1);

	pd->prefs[max_recent_files] =
		dialog_spin_button_in_table(3, 25, main_v->props.max_recent_files, table, 1, 2, 1, 2);
	dialog_mnemonic_label_in_table(_("_Number of files in 'Open recent' menu:"), pd->prefs[max_recent_files],
								   table, 0, 1, 1, 2);
#ifndef WIN32
	pd->prefs[register_recent_mode] =
		dialog_combo_box_text_in_table(registerrecentmodes, main_v->props.register_recent_mode, table, 1, 4,2,3);
	dialog_mnemonic_label_in_table(_("_Register recent files with your desktop:"),
								   pd->prefs[register_recent_mode], table, 0, 1, 2,3);
#endif


	/*
	 *  accelerators
	 */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_tree_store_append(pd->nstore, &auxit, &iter);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL, _("Shortcut keys"), WIDGETCOL, frame, -1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, frame);
	vbox1 = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 6);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);
	vbox2 = dialog_vbox_labeled(_("<b>Shortcut keys</b>"), vbox1);

	pd->prefs[switch_tabs_by_altx] =
		dialog_check_button_new(_("_Switch between tabs with <Alt>+0..9"), main_v->props.switch_tabs_by_altx);
	gtk_box_pack_start(GTK_BOX(vbox2), pd->prefs[switch_tabs_by_altx], FALSE, FALSE, 0);

#ifndef MAC_INTEGRATION
	pd->prefs[save_accelmap] = dialog_check_button_new(_("Save menu accelerators on exit"),
															 main_v->props.save_accelmap);
	gtk_box_pack_start(GTK_BOX(vbox2), pd->prefs[save_accelmap], FALSE, FALSE, 0);
	but = gtk_button_new_with_label(_("Save user customised menu accelerators now"));
	g_signal_connect(G_OBJECT(but), "clicked", G_CALLBACK(save_user_menu_accelerators), NULL);
	gtk_box_pack_start(GTK_BOX(vbox2), but, FALSE, FALSE, 0);

	but = gtk_button_new_with_label(_("Reset menu accelerators to application defaults"));
	g_signal_connect(G_OBJECT(but), "clicked", G_CALLBACK(reset_menu_accelerators), NULL);
	gtk_box_pack_start(GTK_BOX(vbox2), but, FALSE, FALSE, 0);
#endif

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	vbox1 = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 6);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);

	gtk_tree_store_append(pd->nstore, &auxit, &iter);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL, _("Dimensions"), WIDGETCOL, frame, -1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, frame);

	vbox2 = dialog_vbox_labeled(_("<b>Dimensions</b>"), vbox1);

	pd->prefs[leave_to_window_manager] = dialog_check_button_new(_("_Leave dimensions to window manager"),
																 main_v->props.leave_to_window_manager);
	gtk_box_pack_start(GTK_BOX(vbox2), pd->prefs[leave_to_window_manager], FALSE, FALSE, 0);

	vbox3 = gtk_vbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox2), vbox3, FALSE, FALSE, 0);

	pd->prefs[restore_dimensions] = dialog_check_button_new(_("_Restore last used dimensions"),
															main_v->props.restore_dimensions);
	gtk_box_pack_start(GTK_BOX(vbox3), pd->prefs[restore_dimensions], FALSE, FALSE, 0);

	table = dialog_table_in_vbox_defaults(3, 2, 0, vbox3);

	pd->prefs[left_panel_width] =
		dialog_spin_button_in_table(1, 4000, main_v->globses.left_panel_width, table, 1, 2, 0, 1);
	dialog_mnemonic_label_in_table(_("Initial _sidebar width:"), pd->prefs[left_panel_width], table, 0, 1, 0,
								   1);
	pd->prefs[main_window_h] =
		dialog_spin_button_in_table(1, 4000, main_v->globses.main_window_h, table, 1, 2, 1, 2);
	dialog_mnemonic_label_in_table(_("Initial window _height:"), pd->prefs[main_window_h], table, 0, 1, 1, 2);
	pd->prefs[main_window_w] =
		dialog_spin_button_in_table(1, 4000, main_v->globses.main_window_w, table, 1, 2, 2, 3);
	dialog_mnemonic_label_in_table(_("Initial window _width:"), pd->prefs[main_window_w], table, 0, 1, 2, 3);
	restore_dimensions_toggled_lcb(GTK_TOGGLE_BUTTON(pd->prefs[restore_dimensions]), pd);
	g_signal_connect(G_OBJECT(pd->prefs[restore_dimensions]), "toggled",
					 G_CALLBACK(restore_dimensions_toggled_lcb), pd);
	g_signal_connect(G_OBJECT(pd->prefs[leave_to_window_manager]), "toggled",
					 G_CALLBACK(leave_to_window_manager_toggled_lcb), vbox3);

	pd->prefs[max_window_title] = dialog_check_button_new(_("_Limit window title length"),
															main_v->props.max_window_title);
	gtk_box_pack_start(GTK_BOX(vbox3), pd->prefs[max_window_title], FALSE, FALSE, 0);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_tree_store_append(pd->nstore, &auxit, &iter);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL, _("Font & Colors"), WIDGETCOL, frame, -1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, frame);

	vbox1 = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 6);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);
	vbox2 = dialog_vbox_labeled(_("<b>Document tab</b>"), vbox1);

	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	pd->prefs[use_system_tab_font] =
		dialog_check_button_new(_("_Use system document tab font"), main_v->props.use_system_tab_font);
	gtk_box_pack_start(GTK_BOX(hbox), pd->prefs[use_system_tab_font], FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	label = dialog_label_new(_("_Document tab font:"), 0, 0.5, hbox, 0);
	pd->prefs[tab_font_string] = gtk_font_button_new_with_font(main_v->props.tab_font_string);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), pd->prefs[tab_font_string]);
	gtk_box_pack_start(GTK_BOX(hbox), pd->prefs[tab_font_string], FALSE, FALSE, 0);
	gtk_widget_set_sensitive(hbox,
							 !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
														   (pd->prefs[use_system_tab_font])));
	g_signal_connect(G_OBJECT(pd->prefs[use_system_tab_font]), "toggled",
					 G_CALLBACK(prefs_togglebutton_toggled_not_lcb), hbox);

	vbox2 = dialog_vbox_labeled(_("<b>Colors</b>"), vbox1);
	table = dialog_table_in_vbox_defaults(3, 2, 0, vbox2);

	pd->prefs[tab_color_error] = dialog_color_button_in_table(main_v->props.tab_color_error,
															  _("Document error tab color"), table, 1, 2, 0,
															  1);
	dialog_mnemonic_label_in_table(_("Document tab _error color:"), pd->prefs[tab_color_error], table, 0, 1,
								   0, 1);

	pd->prefs[tab_color_loading] = dialog_color_button_in_table(main_v->props.tab_color_loading,
																_("Document loading tab color"), table, 1, 2,
																1, 2);
	dialog_mnemonic_label_in_table(_("Document tab loadin_g color:"), pd->prefs[tab_color_loading], table, 0,
								   1, 1, 2);

	pd->prefs[tab_color_modified] = dialog_color_button_in_table(main_v->props.tab_color_modified,
																 _("Document modified tab color"), table, 1,
																 2, 2, 3);
	dialog_mnemonic_label_in_table(_("Document tab _modified color:"), pd->prefs[tab_color_modified], table,
								   0, 1, 2, 3);

	/*
	 *  Images
	 */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	vbox1 = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 6);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);

	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL, _("Images"), WIDGETCOL, frame, -1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, frame);

	vbox2 = dialog_vbox_labeled(_("<b>Thumbnails</b>"), vbox1);
	table = dialog_table_in_vbox_defaults(2, 2, 0, vbox2);

	index = prefs_combo_box_get_index_from_text(thumbnail_filetype, main_v->props.image_thumbnailtype);
	pd->prefs[image_thumbnailtype] =
		dialog_combo_box_text_in_table(thumbnail_filetype, index, table, 1, 2, 0, 1);
	dialog_mnemonic_label_in_table(_("Thumbnail _filetype:"), pd->prefs[image_thumbnailtype], table, 0, 1, 0,
								   1);
	pd->prefs[image_thumbnailstring] =
		dialog_entry_in_table(main_v->props.image_thumbnailstring, table, 1, 2, 1, 2);
	dialog_mnemonic_label_in_table(_("Thumbnail _suffix:"), pd->prefs[image_thumbnailstring], table, 0, 1, 1,
								   2);

	/*
	 *  External Commands
	 */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 2);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);

	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL, _("External commands"), WIDGETCOL, frame, -1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, frame);
	dialog_label_new(_("<b>External Commands</b>"), 0, 0, vbox1, 4);
	create_extcommands_gui(pd, vbox1);

	/*
	 * External Filters
	 */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 2);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);

	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL, _("External filters"), WIDGETCOL, frame, -1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, frame);

	dialog_label_new(_("<b>External Filters</b>"), 0, 0, vbox1, 4);
	create_filters_gui(pd, vbox1);

	/*
	 *  Output Parsers
	 */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 2);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);

	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL, _("Output parsers"), WIDGETCOL, frame, -1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, frame);
	dialog_label_new(_("<b>Output Parsers</b>"), 0, 0, vbox1, 4);
	create_outputbox_gui(pd, vbox1);

	/*
	 *  Plugins
	 */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	vbox1 = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 6);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);

	gtk_tree_store_append(pd->nstore, &auxit, NULL);
	gtk_tree_store_set(pd->nstore, &auxit, NAMECOL, _("Plugins"), WIDGETCOL, frame, -1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, frame);
	dialog_label_new(_("<b>Plugins</b>"), 0, 0, vbox1, 4);
	create_plugin_gui(pd, vbox1);

	/* plugin children */

	for (tmpslist=main_v->pref_initgui;tmpslist;tmpslist=g_slist_next(tmpslist))
	{
		PrefInitguiCallback func = tmpslist->data;
		func(pd->nstore, &auxit, &pd->widgetfreelist);
	}


	/*
	 * Language Support
	 */
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	vbox1 = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 6);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);

	gtk_tree_store_append(pd->nstore, &iter, NULL);
	gtk_tree_store_set(pd->nstore, &iter, NAMECOL, _("Language support"), WIDGETCOL, frame, -1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, frame);

	/* empty  ?? */

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	vbox1 = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 6);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);

	create_bflang_gui(pd, vbox1);
	pd->widgetfreelist = g_slist_prepend(pd->widgetfreelist, frame);
	freelist = tmplist = g_list_first(langmgr_get_languages());
	while (tmplist) {
		Tbflang *bflang = tmplist->data;
		gtk_tree_store_append(pd->nstore, &auxit, &iter);
		gtk_tree_store_set(pd->nstore, &auxit, NAMECOL, bflang->name, WIDGETCOL, frame, FUNCCOL,
						   bflanggui_set_bflang, DATACOL, bflang, -1);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(freelist);

	{
		GtkWidget *ahbox, *but;

#if GTK_CHECK_VERSION(3,0,0)
		ahbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
		ahbox = gtk_hbutton_box_new();
#endif
		gtk_button_box_set_layout(GTK_BUTTON_BOX(ahbox), GTK_BUTTONBOX_END);
		gtk_box_set_spacing(GTK_BOX(ahbox), 6);

		gtk_box_pack_start(GTK_BOX(dvbox), ahbox, FALSE, FALSE, 0);
		but = dialog_button_new_with_image(NULL, GTK_STOCK_APPLY, G_CALLBACK(preferences_apply_clicked_lcb), pd, FALSE, FALSE);
		gtk_box_pack_start(GTK_BOX(ahbox), but, FALSE, FALSE, 0);

		but = dialog_button_new_with_image(NULL, GTK_STOCK_CANCEL, G_CALLBACK(preferences_cancel_clicked_lcb), pd, FALSE, FALSE);
		gtk_box_pack_start(GTK_BOX(ahbox), but, FALSE, FALSE, 0);

		but = dialog_button_new_with_image(NULL, GTK_STOCK_OK, G_CALLBACK(preferences_ok_clicked_lcb), pd, FALSE, FALSE);
		gtk_box_pack_start(GTK_BOX(ahbox), but, FALSE, FALSE, 6);
		gtk_window_set_default(GTK_WINDOW(pd->win), but);
	}

	gtk_widget_show_all(pd->win);
	gtk_tree_view_expand_all(GTK_TREE_VIEW(pd->noteb));
	g_signal_connect(G_OBJECT(pd->noteb), "cursor-changed", G_CALLBACK(preftree_cursor_changed_cb), pd);
	path = gtk_tree_path_new_first();
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(pd->noteb), path, NULL, FALSE);
	gtk_tree_path_free(path);
}
