/* #define DEBUG */

#include <gtk/gtk.h>
#include <string.h> /* strcmp() */

#include "bluefish.h"
#include "stringlist.h" /* duplicate_arraylist*/
#include "bf_lib.h" /* list_switch_order() */
#include "gtk_easy.h"
#include "pixmap.h"

enum {
	view_line_numbers,
	filebrowser_show_hidden_files,
	filebrowser_show_others_files,
	filebrowser_show_backup_files,
	filebrowser_unknown_icon,
	filebrowser_dir_icon,
	editor_font_string,		/* editor font */
	editor_tab_width,	/* editor tabwidth */
	tab_font_string,		/* notebook tabs font */
	highlight_num_lines_count, /* number of lines to highlight in continous highlighting */	
	defaulthighlight,		/* highlight documents by default */
	transient_htdialogs,  /* set html dialogs transient ro the main window */
	restore_dimensions,
	left_panel_width,
	main_window_h,			/* main window height */
	main_window_w,			/* main window width */
	max_recent_files,	/* length of Open Recent list */
	max_dir_history,	/* length of directory history */
	backup_file, 			/* wheather to use a backup file */
	backup_filestring,  /* the string to append to the backup file */
	backup_abort_style, /* if the backup fails, continue 'save', 'abort' save, or 'ask' user */
	image_thumbnailstring,	/* string to append to thumbnail filenames */
	image_thumbnailtype,	/* fileformat to use for thumbnails, "jpeg" or "png" can be handled by gdkpixbuf*/
	image_thumbnailsizing_type,	/* scaling ratio=0, fixed width=1, height=2, width+height (discard aspect ratio)=3 */
	image_thumbnailsizing_val1,	/* the width, height or ratio, depending on the value above */
	image_thumbnailsizing_val2, /* height if the type=3 */
	image_thumnailformatstring, /* like <a href="%r"><img src="%t"></a> or more advanced */
	allow_multi_instances, /* allow multiple instances of the same file */
	num_undo_levels, 	/* number of undo levels per document */
	clear_undo_on_save, 	/* clear all undo information on file save */
	newfile_default_encoding,/* if you open a new file, what encoding will it use */
	auto_set_encoding_meta,/* auto set metatag for the encoding */
	ext_browsers_in_submenu,
	ext_commands_in_submenu,
	ext_outputbox_in_submenu,

	/* not yet in use */
	image_editor_cline, 	/* image editor commandline */
	cfg_weblint_cline,	/* weblint command line */
	cfg_tab_pos,			/* notebook tabs positioning */
	full_p,				/* use </p> */
	full_li,				/* use </li> */
	allow_css,				/* CSS allowed */
	allow_dep,				/* allow <FONT>... */
	format_by_context, 	/* use <strong> instead of <b>, <emphasis instead of <i> etc. (W3C reccomendation) */
	xhtml,					/* write <br /> */
	allow_ruby,			/* allow <ruby> */
	allow_h4,				/* allow <Q>... */
	allow_frames,			/* allow <FRAME> */
	force_dtd,				/* write <!DOCTYPE...> */
	dtd_url,				/* URL in DTD */
	xml_start,				/* <?XML...> */
	lowercase_tags,		/* use lowercase tags */
	word_wrap,				/* use wordwrap */
	autoindent,			/* autoindent code */
	fontset,				/* load fontset (0 = load font) */
	force_def_style,  /* force white editor background */
	drop_at_drop_pos, 	/* drop at drop position instead of cursor position */
	link_management, 	/* perform link management */
	cont_highlight_full, 	/* if you want to highlight the full text or just the line */
	cont_highlight_update,	/* update the syntax highlighting continuous */
	html_ver,
	cust_menu, 		/* entries in the custom menu */
#ifdef WITH_SPC
	/* spell checker options */
	cfg_spc_cline      ,  /* spell checker command line */
	cfg_spc_lang       ,  /* language */
	spc_accept_compound ,  /* accept compound words ? */
	spc_use_esc_chars   ,  /* specify aditional characters that
                                     may be part of a word ? */
	spc_esc_chars      ,  /* which ones ? */
	spc_use_pers_dict  ,  /* use a personal dictionary */
	spc_pers_dict      ,  /* which one ? */
   spc_use_input_encoding ,  /* use input encoding */
   spc_input_encoding     ,  /* wich one ? */
   spc_output_html_chars  , /* output html chars ? (like &aacute,)*/
#endif
	/* key conversion */
	conv_ctrl_enter,		/* convert control-enter key press */
	ctrl_enter_text,		/* inserted text */
	conv_shift_enter,		/* convert shift-enter key press */
	shift_enter_text,	/* inserted text */
	conv_special_char,		/* convert ctrl-'<','>','&' */
#ifdef WITH_MSG_QUEUE
	open_in_running_bluefish, /* open commandline documents in already running session*/
#endif /* WITH_MSG_QUEUE */
	property_num_max
};

enum {
	browsers,
	external_commands,
	filetypes,
	filefilters,
	highlight_patterns,
	outputbox,
	lists_num_max
};

typedef struct {
	GtkListStore *lstore;
	GtkWidget *lview;
	GtkWidget *entry[5];
	gchar **curstrarr;
} Tfiletypedialog;

typedef struct {
	GtkListStore *lstore;
	GtkWidget *lview;
	GtkWidget *entry[2];
	GtkWidget *check;
	gchar **curstrarr;
} Tfilefilterdialog;


typedef struct {
	GtkListStore *lstore;
	GtkWidget *lview;
	GtkWidget *entry[6];
	GtkWidget *popmenu;
	GtkWidget *check;
	GtkWidget *radio[9];
	gchar **curstrarr;
	const gchar *selected_filetype;
} Thighlightpatterndialog;

typedef struct {
	GtkListStore *lstore;
	GtkWidget *lview;
	GtkWidget *entry[2];
	gchar **curstrarr;
} Tbrowsersdialog;

typedef struct {
	GtkListStore *lstore;
	GtkWidget *lview;
	GtkWidget *entry[2];
	gchar **curstrarr;
} Texternaldialog;

typedef struct {
	GtkListStore *lstore;
	GtkWidget *lview;
	GtkWidget *entry[6];
	GtkWidget *check;
	gchar **curstrarr;
} Toutputboxdialog;

typedef struct {
	GtkWidget *prefs[property_num_max];
	GList *lists[lists_num_max];
	GtkWidget *win;
	GtkWidget *noteb;
	Tfiletypedialog ftd;
	Tfilefilterdialog ffd;
	Thighlightpatterndialog hpd;
	Tbrowsersdialog bd;
	Texternaldialog ed;
	Toutputboxdialog od;
} Tprefdialog;

typedef enum {
	string_none,
	string_file,
	string_font,
	string_color
} Tprefstringtype;

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

static GtkWidget *prefs_string(const gchar *title, const gchar *curval, GtkWidget *box, Tprefdialog *pd, Tprefstringtype prefstringtype) {
	GtkWidget *hbox, *return_widget;

	hbox = gtk_hbox_new(FALSE,3);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(title), FALSE, FALSE, 3);
	return_widget = boxed_entry_with_text(curval, 1023, hbox);
	if (prefstringtype == file) {
		gtk_box_pack_start(GTK_BOX(hbox), file_but_new(return_widget, pd->win, 1), FALSE, FALSE, 3);
	} else if (prefstringtype == font) {
		GtkWidget *but = bf_gtkstock_button(GTK_STOCK_SELECT_FONT, G_CALLBACK(font_button_lcb), return_widget);
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
}

/**********************************************************/
/* FILETYPE, FILTERS AND HIGHLIGHT PATTERNS FUNCTIONS     */
/**********************************************************/

/* static void add_new_general_lcb(Tprefdialog *pd, GtkWidget *entry, gint numentries, gint whichlist, GtkListStore *lstore) {
	gchar *newtype = gtk_editable_get_chars(GTK_EDITABLE(entry),0,-1);
	DEBUG_MSG("add_new_general_lcb, newtype=%s\n", newtype);
	if (strlen(newtype)) {
		gint i;
		gchar **strarr = g_malloc((numentries+1)*sizeof(gchar *));
		strarr[0] = newtype;
		for (i=1;i<numentries;i++) {
			strarr[i] = g_strdup("");
		}
		strarr[i] = NULL;
		pd->lists[whichlist] = g_list_append(pd->lists[whichlist], strarr);
		{
			GtkTreeIter iter;
			gtk_list_store_append(GTK_LIST_STORE(lstore), &iter);
			for (i=0;i<numentries;i++) {
				gtk_list_store_set(GTK_LIST_STORE(lstore),&iter,i,strarr[i],-1);
			}
		}
	} else {
		g_free(newtype);
	}
} */

static gchar **filetype_create_strarr(Tprefdialog *pd) {
	gchar **strarr;
	gchar *escaped, *tmp;
	tmp = gtk_editable_get_chars(GTK_EDITABLE(pd->ftd.entry[0]),0,-1);
	if (strlen(tmp)) {
		strarr = g_malloc(5*sizeof(gchar *));
		strarr[0] = tmp;
		strarr[1] = gtk_editable_get_chars(GTK_EDITABLE(pd->ftd.entry[1]),0,-1);
		escaped = gtk_editable_get_chars(GTK_EDITABLE(pd->ftd.entry[2]),0,-1);
		strarr[2] = unescapestring(escaped);
		g_free(escaped);
		strarr[3] = gtk_editable_get_chars(GTK_EDITABLE(pd->ftd.entry[3]),0,-1);
		strarr[4] = NULL;
		DEBUG_MSG("filetype_create_strarr, created at %p\n", strarr);
		return strarr;
	} else {
		g_free(tmp);
		return NULL;
	}
}

static void add_new_filetype_lcb(GtkWidget *wid, Tprefdialog *pd) {
/*	add_new_general_lcb(pd, pd->ftd.entry[0],4,filetypes,GTK_LIST_STORE(pd->ftd.lstore));*/
	gchar **strarr;
	strarr = filetype_create_strarr(pd);
	if (strarr) {
		pd->lists[filetypes] = g_list_append(pd->lists[filetypes], strarr);
		{
			GtkTreeIter iter;
			gchar *escaped = escapestring(strarr[2],'\0');
			gtk_list_store_append(GTK_LIST_STORE(pd->ftd.lstore), &iter);
			gtk_list_store_set(GTK_LIST_STORE(pd->ftd.lstore), &iter
				,0,strarr[0]
				,1,strarr[1]
				,2,escaped
				,3,strarr[3]
				,-1);
			g_free(escaped);
		}
	}
}

static void filetype_apply_changes(Tprefdialog *pd) {
	DEBUG_MSG("filetype_apply_changes, started\n");
	if (pd->ftd.curstrarr) {
		GtkTreeIter iter;
		gboolean retval = TRUE;
		GList *tmplist;
		tmplist = g_list_first(pd->lists[filetypes]);
		while (tmplist) {
			if (tmplist->data == pd->ftd.curstrarr) {
				g_strfreev(pd->ftd.curstrarr);
				tmplist->data = filetype_create_strarr(pd);
				pd->ftd.curstrarr = tmplist->data;
				DEBUG_MSG("filetype_apply_changes, new curstrarr=%p\n", pd->ftd.curstrarr);
				break;
			}
			tmplist = g_list_next(tmplist);
		}
		
		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pd->ftd.lstore),&iter);
		while (retval) {
			gchar *curval;
			gtk_tree_model_get(GTK_TREE_MODEL(pd->ftd.lstore),&iter,0,&curval,-1);
			if (strcmp(curval,pd->ftd.curstrarr[0])==0) {
				gchar *escaped = escapestring(pd->ftd.curstrarr[2],'\0');
				DEBUG_MSG("filetype_apply_changes, set listore 0=%s,1=%s\n", pd->ftd.curstrarr[0], pd->ftd.curstrarr[1]);
				gtk_list_store_set(GTK_LIST_STORE(pd->ftd.lstore), &iter
					,0,pd->ftd.curstrarr[0]
					,1,pd->ftd.curstrarr[1]
					,2,escaped
					,3,pd->ftd.curstrarr[3]
					,-1);
				g_free(escaped);				
			}
			retval = gtk_tree_model_iter_next(GTK_TREE_MODEL(pd->ftd.lstore),&iter);
		}
	} else {
		DEBUG_MSG("filetype_apply_changes, no curstrarr, nothing to apply\n");
	}
}

static void filetype_selection_changed_cb(GtkTreeSelection *selection, Tprefdialog *pd) {
	generic_selection_changed_cb(selection,pd->ftd.entry,filetype_apply_changes,pd,filetypes,4,&pd->ftd.curstrarr);
/*	GtkTreeIter iter;
	GtkTreeModel *model;
	DEBUG_MSG("filetype_selection_changed_cb, started\n");
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gchar *filetype;
		GList *tmplist = g_list_first(pd->lists[filetypes]);
		gtk_tree_model_get(model, &iter, 0, &filetype, -1);
		filetype_apply_changes(pd);
		while (tmplist) {
			gchar **strarr =(gchar **)tmplist->data;
			if (strcmp(strarr[0],filetype)==0) {
				gchar *escaped = escapestring(strarr[2],'\0');
				gtk_entry_set_text(GTK_ENTRY(pd->ftd.entry[0]), strarr[0]);
				gtk_entry_set_text(GTK_ENTRY(pd->ftd.entry[1]), strarr[1]);
				gtk_entry_set_text(GTK_ENTRY(pd->ftd.entry[2]), escaped);
				gtk_entry_set_text(GTK_ENTRY(pd->ftd.entry[3]), strarr[3]);
				pd->ftd.curstrarr = strarr;
				g_free(escaped);
				return;
			}
			tmplist = g_list_next(tmplist);
		}
	} else {
		DEBUG_MSG("filetype_selection_changed_cb, no selection ?!?!\n");
	}*/
}

static void create_filetype_gui(Tprefdialog *pd, GtkWidget *vbox1) {
	GtkWidget *hbox, *but;
	pd->lists[filetypes] = duplicate_arraylist(main_v->props.filetypes);

	pd->ftd.lstore = gtk_list_store_new (4,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
	pd->ftd.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->ftd.lstore));
	{
		GtkTreeViewColumn *column;
		GtkWidget *scrolwin;
		GtkTreeSelection *select;
	   GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();

		column = gtk_tree_view_column_new_with_attributes ("Filetype", renderer,"text",0,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->ftd.lview), column);
		column = gtk_tree_view_column_new_with_attributes ("Extensions", renderer,"text",1,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->ftd.lview), column);
		column = gtk_tree_view_column_new_with_attributes ("Update chars", renderer,"text",2,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->ftd.lview), column);
		column = gtk_tree_view_column_new_with_attributes ("Icon", renderer,"text",3,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->ftd.lview), column);
		scrolwin = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
		gtk_container_add(GTK_CONTAINER(scrolwin), pd->ftd.lview);
		gtk_widget_set_usize(scrolwin, 150, 150);
		gtk_box_pack_start(GTK_BOX(vbox1), scrolwin, TRUE, TRUE, 2);
		
		select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->ftd.lview));
		g_signal_connect(G_OBJECT(select), "changed",G_CALLBACK(filetype_selection_changed_cb),pd);
	}
	{
		GList *tmplist = g_list_first(pd->lists[filetypes]);
		while (tmplist) {
			gchar **strarr = (gchar **)tmplist->data;
			if (count_array(strarr)==4) {
				GtkTreeIter iter;
				gchar *escaped = escapestring(strarr[2],'\0');
				gtk_list_store_append(GTK_LIST_STORE(pd->ftd.lstore), &iter);
				gtk_list_store_set(GTK_LIST_STORE(pd->ftd.lstore), &iter
					,0,strarr[0]
					,1,strarr[1]
					,2,escaped
					,3,strarr[3]
					,-1);
				g_free(escaped);
			}
			tmplist = g_list_next(tmplist);
		}
	}
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1),hbox, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new(_("Filetype")), FALSE, FALSE, 2);
	pd->ftd.entry[0] = entry_with_text(NULL, 100);
	gtk_box_pack_start(GTK_BOX(hbox),pd->ftd.entry[0], TRUE, TRUE, 2);
	but = bf_gtkstock_button(GTK_STOCK_ADD, G_CALLBACK(add_new_filetype_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox),but, FALSE, FALSE, 2);
/*	but = bf_gtkstock_button(GTK_STOCK_DELETE, G_CALLBACK(filetype_delete_clicked_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox),but, FALSE, FALSE, 2);*/

	pd->ftd.entry[1] = boxed_full_entry(_("Extensions (colon separated)"), NULL, 500, vbox1);
	pd->ftd.entry[2] = boxed_full_entry(_("Highlight update characters"), NULL, 500, vbox1);
	pd->ftd.entry[3] = prefs_string(_("Icon"), NULL, vbox1, pd, string_file);
}

static gchar **filefilter_create_strarr(Tprefdialog *pd) {
	gchar **strarr = g_malloc(4*sizeof(gchar *));
	strarr[0] = gtk_editable_get_chars(GTK_EDITABLE(pd->ffd.entry[0]), 0, -1);
	if (GTK_TOGGLE_BUTTON(pd->ffd.check)->active){
			strarr[1] = g_strdup("0");
		} else {
			strarr[1] = g_strdup("1");
		}
	strarr[2] = gtk_editable_get_chars(GTK_EDITABLE(pd->ffd.entry[1]), 0, -1);
	strarr[3] = NULL;
	return strarr;
}

static void filefilter_apply_changes(Tprefdialog *pd) {
	DEBUG_MSG("filefilters_apply_changes, started\n");
	if (pd->ffd.curstrarr) {
		gchar **strarr;
		strarr = filefilter_create_strarr(pd);
		if (strarr) {
			GList *tmplist;
			GtkTreeIter iter;
			gboolean retval = TRUE;

			gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pd->ffd.lstore),&iter);
			while (retval) {
				gchar *curval;
				gtk_tree_model_get(GTK_TREE_MODEL(pd->ffd.lstore),&iter,0,&curval,-1);
				if (strcmp(curval,pd->ffd.curstrarr[0])==0) {
					gtk_list_store_set(GTK_LIST_STORE(pd->ffd.lstore), &iter
						,0,strarr[0]
						,1,(strarr[1][0] == '0')
						,2,strarr[2]
						,-1);
					DEBUG_MSG("filefilters_apply_changes, changed in tree model\n");
					break;
				}
				retval = gtk_tree_model_iter_next(GTK_TREE_MODEL(pd->ffd.lstore),&iter);
			}

			tmplist = g_list_first(pd->lists[filefilters]);
			while (tmplist) {
				if (tmplist->data == pd->ffd.curstrarr) {
					g_strfreev(pd->ffd.curstrarr);
					tmplist->data = strarr;
					pd->ffd.curstrarr = strarr;
					DEBUG_MSG("filefilters_apply_changes, changed custrarr\n");
					break;
				}
				tmplist = g_list_next(tmplist);
			}
		} else {
			DEBUG_MSG("filefilters_apply_changes, NO strarr!!\n");
		}
	} else {
		DEBUG_MSG("filefilters_apply_changes, NO curstrarr!!\n");
	}
}

static void add_new_filefilter_lcb(GtkWidget *wid, Tprefdialog *pd) {
	gchar **strarr;
	strarr = filefilter_create_strarr(pd);
	if (strarr) {
		GtkTreeIter iter;
		pd->lists[filefilters] = g_list_append(pd->lists[filefilters], strarr);
		gtk_list_store_append(GTK_LIST_STORE(pd->ffd.lstore), &iter);
		gtk_list_store_set(GTK_LIST_STORE(pd->ffd.lstore), &iter
				,0,strarr[0]
				,1,(strarr[1][0]=='0')
				,2,strarr[2]
				,-1);
	}
}
static void filefilter_selection_changed_cb(GtkTreeSelection *selection, Tprefdialog *pd) {
	GtkTreeIter iter;
	GtkTreeModel *model;
	DEBUG_MSG("filefilter_selection_changed_cb, curstrarr=%p, &curstrarr=%p\n", pd->ftd.curstrarr, &pd->ffd.curstrarr);
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gchar *filefilter;
		GList *tmplist = g_list_first(pd->lists[filefilters]);
		gtk_tree_model_get(model, &iter, 0, &filefilter, -1);
		filefilter_apply_changes(pd);
		while (tmplist) {
			gchar **strarr =(gchar **)tmplist->data;
			if (strcmp(strarr[0],filefilter)==0) {
				gtk_entry_set_text(GTK_ENTRY(pd->ffd.entry[0]), strarr[0]);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->ffd.check), (strarr[1][0] == '0'));
				gtk_entry_set_text(GTK_ENTRY(pd->ffd.entry[1]), strarr[2]);
				pd->ffd.curstrarr = strarr;
				DEBUG_MSG("filefilter_selection_changed_cb, found %s, curstrarr=%p\n", filefilter, strarr);
				return;
			}
			tmplist = g_list_next(tmplist);
		}
		DEBUG_MSG("filefilter_selection_changed_cb, could not find the selected text %s\n", filefilter);
	} else {
		DEBUG_MSG("filefilter_selection_changed_cb, no selection ?!?!\n");
	}
}

static void create_filefilter_gui(Tprefdialog *pd, GtkWidget *vbox1) {
	GtkWidget *hbox, *but;
	pd->lists[filefilters] = duplicate_arraylist(main_v->props.filefilters);

	pd->ffd.lstore = gtk_list_store_new (3,G_TYPE_STRING,G_TYPE_BOOLEAN,G_TYPE_STRING);
	pd->ffd.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->ffd.lstore));
	{
		GtkTreeViewColumn *column;
		GtkWidget *scrolwin;
		GtkTreeSelection *select;
	   GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();

		column = gtk_tree_view_column_new_with_attributes ("Filefilter", renderer,"text",0,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->ffd.lview), column);
		column = gtk_tree_view_column_new_with_attributes ("Inverse filtering", renderer,"text",1,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->ffd.lview), column);
		column = gtk_tree_view_column_new_with_attributes ("Filetypes in filter", renderer,"text",2,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->ffd.lview), column);
		scrolwin = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
		gtk_container_add(GTK_CONTAINER(scrolwin), pd->ffd.lview);
		gtk_widget_set_usize(scrolwin, 150, 150);
		gtk_box_pack_start(GTK_BOX(vbox1), scrolwin, TRUE, TRUE, 2);
		
		select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->ffd.lview));
		g_signal_connect(G_OBJECT(select), "changed",G_CALLBACK(filefilter_selection_changed_cb),pd);
	}
	{
		GList *tmplist = g_list_first(pd->lists[filefilters]);
		while (tmplist) {
			gchar **strarr = (gchar **)tmplist->data;
			if (count_array(strarr)==3) {
				GtkTreeIter iter;
				gtk_list_store_append(GTK_LIST_STORE(pd->ffd.lstore), &iter);
				gtk_list_store_set(GTK_LIST_STORE(pd->ffd.lstore), &iter
					,0,strarr[0]
					,1,(strarr[1][0] == '0')
					,2,strarr[2]
					,-1);
			}
			tmplist = g_list_next(tmplist);
		}
	}
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Filefilter")), FALSE, TRUE, 3);
	pd->ffd.entry[0] = boxed_entry_with_text(NULL, 1023, hbox);
	but = bf_gtkstock_button(GTK_STOCK_ADD, G_CALLBACK(add_new_filefilter_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, TRUE, 3);

	pd->ffd.check = boxed_checkbut_with_value(_("Inverse filtering"), FALSE, vbox1);
	pd->ffd.entry[1] = boxed_full_entry(_("Filetypes (colon separated)"), NULL, 500, vbox1);
}

static gchar **highlightpattern_create_strarr(Tprefdialog *pd) {
	gchar **strarr;
	strarr = g_malloc(12*sizeof(gchar *));
	strarr[0] = g_strdup(pd->hpd.selected_filetype);
	strarr[1] = gtk_editable_get_chars(GTK_EDITABLE(pd->hpd.entry[0]),0,-1);
	if (GTK_TOGGLE_BUTTON(pd->hpd.check)->active){
		strarr[2] = g_strdup("0");
	} else {
		strarr[2] = g_strdup("1");
	}
	strarr[3] = gtk_editable_get_chars(GTK_EDITABLE(pd->hpd.entry[1]),0,-1);
	strarr[4] = gtk_editable_get_chars(GTK_EDITABLE(pd->hpd.entry[2]),0,-1);
	if (GTK_TOGGLE_BUTTON(pd->hpd.radio[0])->active){
		strarr[5] = g_strdup("1");
	} else if (GTK_TOGGLE_BUTTON(pd->hpd.radio[1])->active) {
		strarr[5] = g_strdup("2");
	} else {
		strarr[5] = g_strdup("3");
	}
	strarr[6] = gtk_editable_get_chars(GTK_EDITABLE(pd->hpd.entry[3]),0,-1);
	strarr[7] = gtk_editable_get_chars(GTK_EDITABLE(pd->hpd.entry[4]),0,-1);
	strarr[8] = gtk_editable_get_chars(GTK_EDITABLE(pd->hpd.entry[5]),0,-1);
	if (GTK_TOGGLE_BUTTON(pd->hpd.radio[3])->active){
		strarr[9] = g_strdup("0");
	} else if (GTK_TOGGLE_BUTTON(pd->hpd.radio[4])->active) {
		strarr[9] = g_strdup("1");
	} else {
		strarr[9] = g_strdup("2");
	}
	if (GTK_TOGGLE_BUTTON(pd->hpd.radio[6])->active){
		strarr[10] = g_strdup("0");
	} else if (GTK_TOGGLE_BUTTON(pd->hpd.radio[7])->active) {
		strarr[10] = g_strdup("1");
	} else {
		strarr[10] = g_strdup("2");
	}
	strarr[11] = NULL;
	DEBUG_MSG("highlightpattern_create_strarr, strarr at %p with count %d\n", strarr, count_array(strarr));
	return strarr;
}

static void highlightpattern_apply_changes(Tprefdialog *pd) {
	DEBUG_MSG("highlightpattern_apply_changes, started\n");
	if (pd->hpd.curstrarr) {
		GList *tmplist;
		tmplist = g_list_first(pd->lists[highlight_patterns]);
		while (tmplist) {
			if (tmplist->data == pd->hpd.curstrarr) {
				DEBUG_MSG("highlightpattern_apply_changes, curstrarr==tmplist->data==%p\n", tmplist->data);
				g_strfreev(tmplist->data);
				tmplist->data = highlightpattern_create_strarr(pd);
				pd->hpd.curstrarr = NULL;
				return;
			}
			tmplist = g_list_next(tmplist);
		}
		DEBUG_MSG("highlightpattern_apply_changes, nothing found for curstrarr %p?!?\n", pd->hpd.curstrarr);
	}
	DEBUG_MSG("highlightpattern_apply_changes, no curstrarr, nothing to apply\n");
}

static void highlightpattern_popmenu_activate(GtkMenuItem *menuitem,Tprefdialog *pd) {
	GList *tmplist;

	DEBUG_MSG("highlightpattern_popmenu_activate, pd=%p, menuitem=%p\n", pd, menuitem);
	tmplist = g_list_first(pd->lists[highlight_patterns]);
	if (menuitem) {
		pd->hpd.selected_filetype = gtk_label_get_text(GTK_LABEL(GTK_BIN(menuitem)->child));
	}

	highlightpattern_apply_changes(pd);

	gtk_list_store_clear(GTK_LIST_STORE(pd->hpd.lstore));
	DEBUG_MSG("highlightpattern_popmenu_activate, cleared and applied changes\n");
	/* fill list model here */
	while (tmplist) {
		gchar **strarr =(gchar **)tmplist->data;
		if (strcmp(strarr[0], pd->hpd.selected_filetype)==0) {
			GtkTreeIter iter;
			gtk_list_store_append(GTK_LIST_STORE(pd->hpd.lstore), &iter);
			gtk_list_store_set(GTK_LIST_STORE(pd->hpd.lstore), &iter, 0, strarr[1], -1);
		}
		tmplist = g_list_next(tmplist);
	}
	pd->hpd.curstrarr = NULL;
	gtk_entry_set_text(GTK_ENTRY(pd->hpd.entry[0]), "");
	gtk_entry_set_text(GTK_ENTRY(pd->hpd.entry[1]), "");
	gtk_entry_set_text(GTK_ENTRY(pd->hpd.entry[2]), "");
	gtk_entry_set_text(GTK_ENTRY(pd->hpd.entry[3]), "");
	gtk_entry_set_text(GTK_ENTRY(pd->hpd.entry[4]), "");
	gtk_entry_set_text(GTK_ENTRY(pd->hpd.entry[5]), "");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hpd.check), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hpd.radio[0]),TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hpd.radio[3]),TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hpd.radio[6]),TRUE);
	
}

static void add_new_highlightpattern_lcb(GtkWidget *wid, Tprefdialog *pd) {
	gchar *pattern = gtk_editable_get_chars(GTK_EDITABLE(pd->hpd.entry[0]),0,-1);
	if (strlen(pattern) && strlen(pd->hpd.selected_filetype)) {
		gchar **strarr = highlightpattern_create_strarr(pd);
		DEBUG_MSG("add_new_highlightpattern_lcb, appending strarr %p to list\n", strarr);
		pd->lists[highlight_patterns] = g_list_append(pd->lists[highlight_patterns], strarr);
		{
			GtkTreeIter iter;
			DEBUG_MSG("add_new_highlightpattern_lcb, appending to lview\n");
			gtk_list_store_append(GTK_LIST_STORE(pd->hpd.lstore), &iter);
			gtk_list_store_set(GTK_LIST_STORE(pd->hpd.lstore), &iter, 0, strarr[1], -1);
		}
		gtk_entry_set_text(GTK_ENTRY(pd->hpd.entry[0]), "");
	} else {
		g_free(pattern);
	}
}

static void highlightpattern_selection_changed_cb(GtkTreeSelection *selection, Tprefdialog *pd) {
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *pattern;
	DEBUG_MSG("highlightpattern_selection_changed_cb, started\n");
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GList *tmplist = g_list_first(pd->lists[highlight_patterns]);
/*		GtkWidget *menuitem = gtk_menu_get_active(GTK_MENU( gtk_option_menu_get_menu(GTK_OPTION_MENU(pd->hpd.popmenu)) ));*/
		gtk_tree_model_get(model, &iter, 0, &pattern, -1);
		highlightpattern_apply_changes(pd);
		DEBUG_MSG("changed applied, searching for the data of the new selection\n");
		while (tmplist) {
			gchar **strarr =(gchar **)tmplist->data;
#ifdef DEBUG
			if (strarr == NULL){
				DEBUG_MSG("strarr== NULL !!!!!!!!!!!!!!!\n");
			}
#endif
			if (strcmp(strarr[1], pattern)==0 && strcmp(strarr[0], pd->hpd.selected_filetype)==0) {
				DEBUG_MSG("highlightpattern_selection_changed_cb, found strarr=%p\n", strarr);
				DEBUG_MSG("0=%s, 1=%s, 2=%s, 3=%s, 4=%s\n",strarr[0],strarr[1],strarr[2],strarr[3],strarr[4]);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hpd.check), (strarr[2][0] == '0'));
				gtk_entry_set_text(GTK_ENTRY(pd->hpd.entry[0]), strarr[1]);
				gtk_entry_set_text(GTK_ENTRY(pd->hpd.entry[1]), strarr[3]);
				gtk_entry_set_text(GTK_ENTRY(pd->hpd.entry[2]), strarr[4]);
				if (strarr[5][0] == '3') {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hpd.radio[2]),TRUE);
				} else if (strarr[5][0] == '2') {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hpd.radio[1]),TRUE);
				} else {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hpd.radio[0]),TRUE);
				}
				gtk_entry_set_text(GTK_ENTRY(pd->hpd.entry[3]), strarr[6]);
				gtk_entry_set_text(GTK_ENTRY(pd->hpd.entry[4]), strarr[7]);
				gtk_entry_set_text(GTK_ENTRY(pd->hpd.entry[5]), strarr[8]);
				if (strarr[9][0] == '2') {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hpd.radio[5]),TRUE);
				} else if (strarr[9][0] == '1') {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hpd.radio[4]),TRUE);
				} else {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hpd.radio[3]),TRUE);
				}
				if (strarr[10][0] == '2') {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hpd.radio[8]),TRUE);
				} else if (strarr[10][0] == '1') {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hpd.radio[7]),TRUE);
				} else {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->hpd.radio[6]),TRUE);
				}
				pd->hpd.curstrarr = strarr;
				break;
			}
			tmplist = g_list_next(tmplist);
		}
		g_free(pattern);
	}
}
static void highlightpattern_type_toggled(GtkToggleButton *togglebutton,Tprefdialog *pd){
	DEBUG_MSG("highlightpattern_type_toggled, started\n");
	if (GTK_TOGGLE_BUTTON(pd->hpd.radio[0])->active) {
		gtk_widget_set_sensitive(pd->hpd.entry[2], TRUE);
	} else {
		gtk_widget_set_sensitive(pd->hpd.entry[2], FALSE);
	}
	DEBUG_MSG("highlightpattern_type_toggled, done\n");
}
static void highlightpattern_up_clicked_lcb(GtkWidget *wid, Tprefdialog *pd) {
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *pattern;

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->hpd.lview));
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GList *tmplist = g_list_first(pd->lists[highlight_patterns]);
		gtk_tree_model_get(model, &iter, 0, &pattern, -1);
		while (tmplist) {
			gchar **strarr =(gchar **)tmplist->data;
			if (strcmp(strarr[1], pattern)==0 && strcmp(strarr[0], pd->hpd.selected_filetype)==0) {
				if (tmplist->prev) {
					list_switch_order(tmplist, tmplist->prev);
					highlightpattern_popmenu_activate(NULL, pd);
				}
				return;
			}
			tmplist = g_list_next(tmplist);
		}
	}
}
static void highlightpattern_down_clicked_lcb(GtkWidget *wid, Tprefdialog *pd) {
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *pattern;
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->hpd.lview));
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GList *tmplist = g_list_first(pd->lists[highlight_patterns]);
		gtk_tree_model_get(model, &iter, 0, &pattern, -1);
		while (tmplist) {
			gchar **strarr =(gchar **)tmplist->data;
			if (strcmp(strarr[1], pattern)==0 && strcmp(strarr[0], pd->hpd.selected_filetype)==0) {
				if (tmplist->next) {
					list_switch_order(tmplist, tmplist->next);
					highlightpattern_popmenu_activate(NULL, pd);
					return;
				}
			}
			tmplist = g_list_next(tmplist);
		}
	}
}
static void highlightpattern_delete_clicked_lcb(GtkWidget *wid, Tprefdialog *pd) {
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *pattern;
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->hpd.lview));
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GList *tmplist = g_list_first(pd->lists[highlight_patterns]);
		gtk_tree_model_get(model, &iter, 0, &pattern, -1);
		while (tmplist) {
			gchar **strarr =(gchar **)tmplist->data;
			if (strcmp(strarr[1], pattern)==0 && strcmp(strarr[0], pd->hpd.selected_filetype)==0) {
				pd->hpd.curstrarr = NULL;
				pd->lists[highlight_patterns] = g_list_remove(pd->lists[highlight_patterns], strarr);
				g_strfreev(strarr);
				highlightpattern_popmenu_activate(NULL, pd);
				return;
			}
			tmplist = g_list_next(tmplist);
		}
	}
}

static void highlightpattern_gui_rebuild_filetype_popup(Tprefdialog *pd) {
	GList *tmplist;
	GtkWidget *menu, *menuitem;
	gtk_option_menu_remove_menu(GTK_OPTION_MENU(pd->hpd.popmenu));
	menu = gtk_menu_new();
	gtk_option_menu_set_menu(GTK_OPTION_MENU(pd->hpd.popmenu), menu);
	gtk_widget_show(menu);
	tmplist = g_list_first(pd->lists[filetypes]);
	while (tmplist) {
		gchar **arr = (gchar **)tmplist->data;
		if (count_array(arr)>=3) {
			menuitem = gtk_menu_item_new_with_label(arr[0]);
			DEBUG_MSG("highlightpattern_gui_rebuild_filetype_popup, menuitem=%p for %s\n", menuitem, arr[0]);
			g_signal_connect(GTK_OBJECT(menuitem), "activate",G_CALLBACK(highlightpattern_popmenu_activate),pd);
			gtk_menu_append(GTK_MENU(menu), menuitem);
			gtk_widget_show(menuitem);
		}
		tmplist = g_list_next(tmplist);
	}
	
}

static void create_highlightpattern_gui(Tprefdialog *pd, GtkWidget *vbox1) {
	GtkWidget *hbox, *but, *vbox3;
	pd->lists[highlight_patterns] = duplicate_arraylist(main_v->props.highlight_patterns);
	
	DEBUG_MSG("create_highlightpattern_gui, pd=%p, pd->lists[highlight_patterns]=%p\n", pd, pd->lists[highlight_patterns]);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 3);

	gtk_box_pack_start(GTK_BOX(hbox),gtk_label_new(_("filetype")),FALSE, FALSE, 3);
	pd->hpd.popmenu = gtk_option_menu_new();
	gtk_option_menu_set_menu(GTK_OPTION_MENU(pd->hpd.popmenu), gtk_menu_new());
	highlightpattern_gui_rebuild_filetype_popup(pd);
	gtk_box_pack_start(GTK_BOX(hbox),pd->hpd.popmenu,TRUE, TRUE, 3);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, TRUE, TRUE, 3);
	pd->hpd.entry[0] = boxed_full_entry(_("Pattern name"), NULL, 500, hbox);

	but = bf_gtkstock_button(GTK_STOCK_ADD, G_CALLBACK(add_new_highlightpattern_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, TRUE, 3);
	but = bf_gtkstock_button(GTK_STOCK_DELETE, G_CALLBACK(highlightpattern_delete_clicked_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 1);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, TRUE, TRUE, 0);

	pd->hpd.lstore = gtk_list_store_new (1, G_TYPE_STRING);
	pd->hpd.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->hpd.lstore));
	{
		GtkTreeViewColumn *column;
		GtkWidget *scrolwin;
		GtkTreeSelection *select;
	   GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();

		column = gtk_tree_view_column_new_with_attributes ("Pattern", renderer,"text", 0,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->hpd.lview), column);
		scrolwin = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_container_add(GTK_CONTAINER(scrolwin), pd->hpd.lview);
		gtk_box_pack_start(GTK_BOX(hbox), scrolwin, FALSE, TRUE, 2);
		
		select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->hpd.lview));
		g_signal_connect(G_OBJECT(select), "changed",G_CALLBACK(highlightpattern_selection_changed_cb),pd);
	}

	vbox3 = gtk_vbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(hbox), vbox3, FALSE, FALSE, 2);
	/* pack up and down buttons here */

	but = bf_gtkstock_button(GTK_STOCK_GO_UP, G_CALLBACK(highlightpattern_up_clicked_lcb), pd);
	gtk_box_pack_start(GTK_BOX(vbox3), but, FALSE, FALSE, 1);
	but = bf_gtkstock_button(GTK_STOCK_GO_DOWN, G_CALLBACK(highlightpattern_down_clicked_lcb), pd);
	gtk_box_pack_start(GTK_BOX(vbox3), but, FALSE, FALSE, 1);
	
	vbox3 = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox3, TRUE, TRUE, 2);

	pd->hpd.radio[0] = gtk_radio_button_new_with_label(NULL, _("Start pattern and end pattern"));
	gtk_box_pack_start(GTK_BOX(vbox3),pd->hpd.radio[0], TRUE, TRUE, 0);
	pd->hpd.radio[1] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pd->hpd.radio[0]), _("Only start pattern"));
	gtk_box_pack_start(GTK_BOX(vbox3),pd->hpd.radio[1], TRUE, TRUE, 0);
	pd->hpd.radio[2] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pd->hpd.radio[0]), _("Subpattern from parent"));
	gtk_box_pack_start(GTK_BOX(vbox3),pd->hpd.radio[2], TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(pd->hpd.radio[0]), "toggled", G_CALLBACK(highlightpattern_type_toggled), pd);
	g_signal_connect(G_OBJECT(pd->hpd.radio[1]), "toggled", G_CALLBACK(highlightpattern_type_toggled), pd);
	g_signal_connect(G_OBJECT(pd->hpd.radio[2]), "toggled", G_CALLBACK(highlightpattern_type_toggled), pd);

	pd->hpd.entry[1] = boxed_full_entry(_("Start pattern"), NULL, 500, vbox3);
	pd->hpd.entry[2] = boxed_full_entry(_("End pattern"), NULL, 500, vbox3);
	pd->hpd.check = boxed_checkbut_with_value(_("Case sensitive matching"), FALSE, vbox3);
	pd->hpd.entry[3] = boxed_full_entry(_("Parentmatch"), NULL, 100, vbox3);
	pd->hpd.entry[4] = prefs_string(_("Foreground color"), "", vbox3, pd, string_color);
	pd->hpd.entry[5] = prefs_string(_("Background color"), "", vbox3, pd, string_color);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, TRUE, TRUE, 0);
	
	vbox3 = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox3, TRUE, TRUE, 0);
	
	pd->hpd.radio[3] = gtk_radio_button_new_with_label(NULL, _("don't change weight"));
	gtk_box_pack_start(GTK_BOX(vbox3),pd->hpd.radio[3], TRUE, TRUE, 0);
	pd->hpd.radio[4] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pd->hpd.radio[3]), _("force non-bold weight"));
	gtk_box_pack_start(GTK_BOX(vbox3),pd->hpd.radio[4], TRUE, TRUE, 0);
	pd->hpd.radio[5] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pd->hpd.radio[3]), _("force bold weight"));
	gtk_box_pack_start(GTK_BOX(vbox3),pd->hpd.radio[5], TRUE, TRUE, 0);

	vbox3 = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox3, TRUE, TRUE, 0);

	pd->hpd.radio[6] = gtk_radio_button_new_with_label(NULL, _("don't change style"));
	gtk_box_pack_start(GTK_BOX(vbox3),pd->hpd.radio[6], TRUE, TRUE, 0);
	pd->hpd.radio[7] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pd->hpd.radio[6]), _("force non-italic style"));
	gtk_box_pack_start(GTK_BOX(vbox3),pd->hpd.radio[7], TRUE, TRUE, 0);
	pd->hpd.radio[8] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pd->hpd.radio[6]), _("force italic style"));
	gtk_box_pack_start(GTK_BOX(vbox3),pd->hpd.radio[8], TRUE, TRUE, 0);
}

static gchar **browser_create_strarr(Tprefdialog *pd) {
	return generic_create_strarr(2, pd->bd.entry, TRUE);
}

static void browsers_apply_changes(Tprefdialog *pd) {
	DEBUG_MSG("browsers_apply_changes, started\n");
	if (pd->bd.curstrarr) {
		gchar **strarr;
		strarr = browser_create_strarr(pd);
		if (strarr) {
			GList *tmplist;
			GtkTreeIter iter;
			gboolean retval = TRUE;

			gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pd->bd.lstore),&iter);
			while (retval) {
				gchar *curval;
				gtk_tree_model_get(GTK_TREE_MODEL(pd->bd.lstore),&iter,0,&curval,-1);
				if (strcmp(curval,pd->bd.curstrarr[0])==0) {
					gtk_list_store_set(GTK_LIST_STORE(pd->bd.lstore), &iter
						,0,strarr[0]
						,1,strarr[1]
						,-1);
					DEBUG_MSG("browsers_apply_changes, changed in tree model\n");
					break;
				}
				retval = gtk_tree_model_iter_next(GTK_TREE_MODEL(pd->bd.lstore),&iter);
			}

			tmplist = g_list_first(pd->lists[browsers]);
			while (tmplist) {
				if (tmplist->data == pd->bd.curstrarr) {
					g_strfreev(pd->bd.curstrarr);
					tmplist->data = strarr;
					pd->bd.curstrarr = strarr;
					DEBUG_MSG("browsers_apply_changes, changed custrarr\n");
					break;
				}
				tmplist = g_list_next(tmplist);
			}
		} else {
			DEBUG_MSG("browsers_apply_changes, NO strarr!!\n");
		}
	} else {
		DEBUG_MSG("browsers_apply_changes, NO curstrarr!!\n");
	}
}

static void add_new_browser_lcb(GtkWidget *wid, Tprefdialog *pd) {
	gchar **strarr;
	strarr = browser_create_strarr(pd);
	if (strarr) {
		GtkTreeIter iter;
		pd->lists[browsers] = g_list_append(pd->lists[browsers], strarr);
		gtk_list_store_append(GTK_LIST_STORE(pd->bd.lstore), &iter);
		gtk_list_store_set(GTK_LIST_STORE(pd->bd.lstore), &iter
				,0,strarr[0]
				,1,strarr[1]
				,-1);
	}
}
static void browser_selection_changed_cb(GtkTreeSelection *selection, Tprefdialog *pd) {
	DEBUG_MSG("browser_selection_changed_cb, curstrarr=%p, &curstrarr=%p\n", pd->ftd.curstrarr, &pd->bd.curstrarr);
	generic_selection_changed_cb(selection,pd->bd.entry,browsers_apply_changes,pd,browsers,2,&pd->bd.curstrarr);
	DEBUG_MSG("browser_selection_changed_cb, curstrarr=%p, &curstrarr=%p\n", pd->ftd.curstrarr, &pd->bd.curstrarr);
/*	GtkTreeIter iter;
	GtkTreeModel *model;
	DEBUG_MSG("browser_selection_changed_cb, started\n");
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gchar *filetype;
		GList *tmplist = g_list_first(pd->lists[browsers]);
		gtk_tree_model_get(model, &iter, 0, &filetype, -1);
		browsers_apply_changes(pd);
		while (tmplist) {
			gchar **strarr =(gchar **)tmplist->data;
			if (strcmp(strarr[0],filetype)==0) {
				gtk_entry_set_text(GTK_ENTRY(pd->bd.entry[0]), strarr[0]);
				gtk_entry_set_text(GTK_ENTRY(pd->bd.entry[1]), strarr[1]);
				pd->ftd.curstrarr = strarr;
				return;
			}
			tmplist = g_list_next(tmplist);
		}
	} else {
		DEBUG_MSG("browser_selection_changed_cb, no selection ?!?!\n");
	}*/
}

static void create_browsers_gui(Tprefdialog *pd, GtkWidget *vbox1) {
	GtkWidget *hbox, *but;
	pd->lists[browsers] = duplicate_arraylist(main_v->props.browsers);

	pd->bd.lstore = gtk_list_store_new (2,G_TYPE_STRING,G_TYPE_STRING);
	pd->bd.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->bd.lstore));
	{
		GtkTreeViewColumn *column;
		GtkWidget *scrolwin;
		GtkTreeSelection *select;
	   GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();

		column = gtk_tree_view_column_new_with_attributes ("Browser", renderer,"text",0,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->bd.lview), column);
		column = gtk_tree_view_column_new_with_attributes ("Command", renderer,"text",1,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->bd.lview), column);
		scrolwin = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
		gtk_container_add(GTK_CONTAINER(scrolwin), pd->bd.lview);
		gtk_widget_set_usize(scrolwin, 150, 150);
		gtk_box_pack_start(GTK_BOX(vbox1), scrolwin, TRUE, TRUE, 2);
		
		select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->bd.lview));
		g_signal_connect(G_OBJECT(select), "changed",G_CALLBACK(browser_selection_changed_cb),pd);
	}
	{
		GList *tmplist = g_list_first(pd->lists[browsers]);
		while (tmplist) {
			gchar **strarr = (gchar **)tmplist->data;
			if (count_array(strarr)==2) {
				GtkTreeIter iter;
				gtk_list_store_append(GTK_LIST_STORE(pd->bd.lstore), &iter);
				gtk_list_store_set(GTK_LIST_STORE(pd->bd.lstore), &iter
					,0,strarr[0]
					,1,strarr[1]
					,-1);
			}
			tmplist = g_list_next(tmplist);
		}
	}
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Browser")), FALSE, TRUE, 3);
	pd->bd.entry[0] = boxed_entry_with_text(NULL, 1023, hbox);
	but = bf_gtkstock_button(GTK_STOCK_ADD, G_CALLBACK(add_new_browser_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, TRUE, 3);

	gtk_box_pack_start(GTK_BOX(vbox1), gtk_label_new(_("%s = current filename")), TRUE, TRUE, 0);
	pd->bd.entry[1] = boxed_full_entry(_("Command"), NULL, 500, vbox1);
}

static gchar **external_create_strarr(Tprefdialog *pd) {
	return generic_create_strarr(2, pd->ed.entry, TRUE);
}

static void externals_apply_changes(Tprefdialog *pd) {
	DEBUG_MSG("browsers_apply_changes, started\n");
	if (pd->ed.curstrarr) {
		gchar **strarr;
		strarr = external_create_strarr(pd);
		if (strarr) {
			GList *tmplist;
			GtkTreeIter iter;
			gboolean retval = TRUE;

			gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pd->ed.lstore),&iter);
			while (retval) {
				gchar *curval;
				gtk_tree_model_get(GTK_TREE_MODEL(pd->ed.lstore),&iter,0,&curval,-1);
				if (strcmp(curval,pd->ed.curstrarr[0])==0) {
					gtk_list_store_set(GTK_LIST_STORE(pd->ed.lstore), &iter
						,0,strarr[0]
						,1,strarr[1]
						,-1);
					DEBUG_MSG("externals_apply_changes, changed in tree model\n");
					break;
				}
				retval = gtk_tree_model_iter_next(GTK_TREE_MODEL(pd->ed.lstore),&iter);
			}

			tmplist = g_list_first(pd->lists[external_commands]);
			while (tmplist) {
				if (tmplist->data == pd->ed.curstrarr) {
					g_strfreev(pd->ed.curstrarr);
					tmplist->data = strarr;
					pd->ed.curstrarr = strarr;
					DEBUG_MSG("externals_apply_changes, changed custrarr\n");
					break;
				}
				tmplist = g_list_next(tmplist);
			}
		} else {
			DEBUG_MSG("externals_apply_changes, NO strarr!!\n");
		}
	} else {
		DEBUG_MSG("externals_apply_changes, NO curstrarr!!\n");
	}
}

static void external_selection_changed_cb(GtkTreeSelection *selection, Tprefdialog *pd) {
	generic_selection_changed_cb(selection,pd->ed.entry,externals_apply_changes,pd,external_commands,2,&pd->ed.curstrarr);
}

static void add_new_external_lcb(GtkWidget *wid, Tprefdialog *pd) {
	gchar **strarr;
	strarr = external_create_strarr(pd);
	if (strarr) {
		GtkTreeIter iter;
		pd->lists[external_commands] = g_list_append(pd->lists[external_commands], strarr);
		gtk_list_store_append(GTK_LIST_STORE(pd->ed.lstore), &iter);
		gtk_list_store_set(GTK_LIST_STORE(pd->ed.lstore), &iter
				,0,strarr[0]
				,1,strarr[1]
				,-1);
	}
}

static void create_externals_gui(Tprefdialog *pd, GtkWidget *vbox1) {
	GtkWidget *hbox, *but;
	pd->lists[external_commands] = duplicate_arraylist(main_v->props.external_commands);

	pd->ed.lstore = gtk_list_store_new (2,G_TYPE_STRING,G_TYPE_STRING);
	pd->ed.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->ed.lstore));
	{
		GtkTreeViewColumn *column;
		GtkWidget *scrolwin;
		GtkTreeSelection *select;
	   GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();

		column = gtk_tree_view_column_new_with_attributes ("Label", renderer,"text",0,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->ed.lview), column);
		column = gtk_tree_view_column_new_with_attributes ("Command", renderer,"text",1,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->ed.lview), column);
		scrolwin = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
		gtk_container_add(GTK_CONTAINER(scrolwin), pd->ed.lview);
		gtk_widget_set_usize(scrolwin, 150, 150);
		gtk_box_pack_start(GTK_BOX(vbox1), scrolwin, TRUE, TRUE, 2);
		
		select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->ed.lview));
		g_signal_connect(G_OBJECT(select), "changed",G_CALLBACK(external_selection_changed_cb),pd);
	}
	{
		GList *tmplist = g_list_first(pd->lists[external_commands]);
		while (tmplist) {
			gchar **strarr = (gchar **)tmplist->data;
			if (count_array(strarr)==2) {
				GtkTreeIter iter;
				gtk_list_store_append(GTK_LIST_STORE(pd->ed.lstore), &iter);
				gtk_list_store_set(GTK_LIST_STORE(pd->ed.lstore), &iter
					,0,strarr[0]
					,1,strarr[1]
					,-1);
			}
			tmplist = g_list_next(tmplist);
		}
	}
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Label")), FALSE, TRUE, 3);
	pd->ed.entry[0] = boxed_entry_with_text(NULL, 1023, hbox);
	but = bf_gtkstock_button(GTK_STOCK_ADD, G_CALLBACK(add_new_external_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, TRUE, 3);

	gtk_box_pack_start(GTK_BOX(vbox1), gtk_label_new(_("%s = current filename\n%f = output filename (for filters)")), TRUE, TRUE, 0);
	pd->ed.entry[1] = boxed_full_entry(_("Command"), NULL, 500, vbox1);
}
static gchar **outputbox_create_strarr(Tprefdialog *pd) {
	gchar **strarr = g_malloc(8*sizeof(gchar *));
	strarr[0] = gtk_editable_get_chars(GTK_EDITABLE(pd->od.entry[0]), 0, -1);
	strarr[1] = gtk_editable_get_chars(GTK_EDITABLE(pd->od.entry[1]), 0, -1);
	strarr[2] = gtk_editable_get_chars(GTK_EDITABLE(pd->od.entry[2]), 0, -1);
	strarr[3] = gtk_editable_get_chars(GTK_EDITABLE(pd->od.entry[3]), 0, -1);
	strarr[4] = gtk_editable_get_chars(GTK_EDITABLE(pd->od.entry[4]), 0, -1);
	strarr[5] = gtk_editable_get_chars(GTK_EDITABLE(pd->od.entry[5]), 0, -1);
	strarr[6] = g_strdup((GTK_TOGGLE_BUTTON(pd->od.check)->active) ? "1" : "0");
	strarr[7] = NULL;
	return strarr;
}

static void outputbox_apply_changes(Tprefdialog *pd) {
	if (pd->od.curstrarr) {
		gchar **strarr;
		strarr = outputbox_create_strarr(pd);
		if (strarr) {
			GList *tmplist;
			GtkTreeIter iter;
			gboolean retval = TRUE;

			gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pd->od.lstore),&iter);
			while (retval) {
				gchar *curval;
				gtk_tree_model_get(GTK_TREE_MODEL(pd->od.lstore),&iter,0,&curval,-1);
				if (strcmp(curval,pd->od.curstrarr[0])==0) {
					gtk_list_store_set(GTK_LIST_STORE(pd->od.lstore), &iter
						,0,strarr[0]
						,1,strarr[1]
						,2,strarr[2]
						,3,strarr[3]
						,4,strarr[4]
						,5,strarr[5]
						,6,strarr[6]
						,-1);
					break;
				}
				retval = gtk_tree_model_iter_next(GTK_TREE_MODEL(pd->od.lstore),&iter);
			}

			tmplist = g_list_first(pd->lists[outputbox]);
			while (tmplist) {
				if (tmplist->data == pd->od.curstrarr) {
					g_strfreev(pd->od.curstrarr);
					tmplist->data = strarr;
					pd->od.curstrarr = strarr;
					break;
				}
				tmplist = g_list_next(tmplist);
			}
		} else {
			DEBUG_MSG("outputbox_apply_changes, NO strarr!!\n");
		}
	} else {
		DEBUG_MSG("outputbox_apply_changes, NO curstrarr!!\n");
	}
}	
static void add_new_outputbox_lcb(GtkWidget *wid, Tprefdialog *pd) {
	gchar **strarr;
	strarr = outputbox_create_strarr(pd);
	if (strarr) {
		GtkTreeIter iter;
		pd->lists[outputbox] = g_list_append(pd->lists[outputbox], strarr);
		gtk_list_store_append(GTK_LIST_STORE(pd->od.lstore), &iter);
		gtk_list_store_set(GTK_LIST_STORE(pd->od.lstore), &iter
						,0,strarr[0]
						,1,strarr[1]
						,2,strarr[2]
						,3,strarr[3]
						,4,strarr[4]
						,5,strarr[5]
						,6,strarr[6]
						,-1);
	}
}

static void outputbox_selection_changed_cb(GtkTreeSelection *selection, Tprefdialog *pd) {
	GtkTreeIter iter;
	GtkTreeModel *model;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gchar *name;
		GList *tmplist = g_list_first(pd->lists[outputbox]);
		gtk_tree_model_get(model, &iter, 0, &name, -1);
		outputbox_apply_changes(pd);
		while (tmplist) {
			gchar **strarr =(gchar **)tmplist->data;
			if (strcmp(strarr[0],name)==0) {
				gtk_entry_set_text(GTK_ENTRY(pd->od.entry[0]), strarr[0]);
				gtk_entry_set_text(GTK_ENTRY(pd->od.entry[1]), strarr[1]);
				gtk_entry_set_text(GTK_ENTRY(pd->od.entry[2]), strarr[2]);
				gtk_entry_set_text(GTK_ENTRY(pd->od.entry[3]), strarr[3]);
				gtk_entry_set_text(GTK_ENTRY(pd->od.entry[4]), strarr[4]);
				gtk_entry_set_text(GTK_ENTRY(pd->od.entry[5]), strarr[5]);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->od.check), (strarr[6][0] == '1'));
				pd->od.curstrarr = strarr;
				return;
			}
			tmplist = g_list_next(tmplist);
		}
	} else {
		DEBUG_MSG("outputbox_selection_changed_cb, no selection ?!?!\n");
	}
}
	

static void create_outputbox_gui(Tprefdialog *pd, GtkWidget *vbox1) {
	GtkWidget *hbox, *but;
	pd->lists[outputbox] = duplicate_arraylist(main_v->props.outputbox);

	pd->od.lstore = gtk_list_store_new (7,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
	pd->od.lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pd->od.lstore));
	{
		GtkTreeViewColumn *column;
		GtkWidget *scrolwin;
		GtkTreeSelection *select;
	   GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();

		column = gtk_tree_view_column_new_with_attributes ("Name", renderer,"text",0,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->od.lview), column);
		column = gtk_tree_view_column_new_with_attributes ("Pattern", renderer,"text",1,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->od.lview), column);
		column = gtk_tree_view_column_new_with_attributes ("File #", renderer,"text",2,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->od.lview), column);
		column = gtk_tree_view_column_new_with_attributes ("Line #", renderer,"text",3,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->od.lview), column);
		column = gtk_tree_view_column_new_with_attributes ("Output #", renderer,"text",4,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->od.lview), column);
		column = gtk_tree_view_column_new_with_attributes ("Command", renderer,"text",5,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->od.lview), column);
		column = gtk_tree_view_column_new_with_attributes ("Show all output", renderer,"text",6,NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(pd->od.lview), column);

		scrolwin = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
		gtk_container_add(GTK_CONTAINER(scrolwin), pd->od.lview);
		gtk_widget_set_usize(scrolwin, 150, 150);
		gtk_box_pack_start(GTK_BOX(vbox1), scrolwin, TRUE, TRUE, 2);
		
		select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->od.lview));
		g_signal_connect(G_OBJECT(select), "changed",G_CALLBACK(outputbox_selection_changed_cb),pd);
	}
	{
		GList *tmplist = g_list_first(pd->lists[outputbox]);
		while (tmplist) {
			gchar **strarr = (gchar **)tmplist->data;
			if (count_array(strarr)==7) {
				GtkTreeIter iter;
				gtk_list_store_append(GTK_LIST_STORE(pd->od.lstore), &iter);
				gtk_list_store_set(GTK_LIST_STORE(pd->od.lstore), &iter
					,0,strarr[0]
					,1,strarr[1]
					,2,strarr[2]
					,3,strarr[3]
					,4,strarr[4]
					,5,strarr[5]
					,6,strarr[6]
					,-1);
			}
			tmplist = g_list_next(tmplist);
		}
	}
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Name")), FALSE, TRUE, 3);
	pd->od.entry[0] = boxed_entry_with_text(NULL, 1023, hbox);
	but = bf_gtkstock_button(GTK_STOCK_ADD, G_CALLBACK(add_new_outputbox_lcb), pd);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, TRUE, 3);
	pd->od.entry[1] = boxed_full_entry(_("Pattern"), NULL, 500, vbox1);
	pd->od.entry[2] = boxed_full_entry(_("File subpattern #"), NULL, 500, vbox1);
	pd->od.entry[3] = boxed_full_entry(_("Line subpattern #"), NULL, 500, vbox1);
	pd->od.entry[4] = boxed_full_entry(_("Output subpattern #"), NULL, 500, vbox1);
	pd->od.entry[5] = boxed_full_entry(_("Command"), NULL, 500, vbox1);
	pd->od.check = boxed_checkbut_with_value(_("Show all output"), FALSE, vbox1);
}

/**************************************/
/* MAIN DIALOG FUNCTIONS              */
/**************************************/

static void preferences_destroy_lcb(GtkWidget * widget, GdkEvent *event, Tprefdialog *pd) {
	GtkTreeSelection *select;
	DEBUG_MSG("preferences_destroy_lcb, started\n");
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->ftd.lview));
	g_signal_handlers_destroy(G_OBJECT(select));
/* g_signal_handlers_destroy(G_OBJECT(GTK_COMBO(pd->ftd.combo)->list));*/
/*	g_signal_handlers_destroy(G_OBJECT(GTK_COMBO(pd->ffd.combo)->list));*/
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(pd->ffd.lview));
	g_signal_handlers_destroy(G_OBJECT(select));

	g_signal_handlers_destroy(G_OBJECT(pd->hpd.popmenu));
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
static void preferences_ok_clicked_lcb(GtkWidget *wid, Tprefdialog *pd) {
	string_apply(&main_v->props.editor_font_string, pd->prefs[editor_font_string]);
	integer_apply(&main_v->props.editor_tab_width, pd->prefs[editor_tab_width], FALSE);
	integer_apply(&main_v->props.word_wrap, pd->prefs[word_wrap], TRUE);
	integer_apply(&main_v->props.view_line_numbers, pd->prefs[view_line_numbers], TRUE);
	integer_apply(&main_v->props.defaulthighlight, pd->prefs[defaulthighlight], TRUE);
	integer_apply(&main_v->props.highlight_num_lines_count, pd->prefs[highlight_num_lines_count], FALSE);
	integer_apply(&main_v->props.lowercase_tags, pd->prefs[lowercase_tags], TRUE);
	integer_apply(&main_v->props.allow_dep, pd->prefs[allow_dep], TRUE);
	integer_apply(&main_v->props.xhtml, pd->prefs[xhtml], TRUE);
	
	string_apply(&main_v->props.newfile_default_encoding, GTK_COMBO(pd->prefs[newfile_default_encoding])->entry);
	integer_apply(&main_v->props.auto_set_encoding_meta, pd->prefs[auto_set_encoding_meta], TRUE);
	integer_apply(&main_v->props.backup_file, pd->prefs[backup_file], TRUE);
	string_apply(&main_v->props.backup_filestring, pd->prefs[backup_filestring]);
	string_apply(&main_v->props.backup_abort_style, GTK_COMBO(pd->prefs[backup_abort_style])->entry);
	integer_apply(&main_v->props.num_undo_levels, pd->prefs[num_undo_levels], FALSE);
	integer_apply(&main_v->props.clear_undo_on_save, pd->prefs[clear_undo_on_save], TRUE);
	integer_apply(&main_v->props.allow_multi_instances, pd->prefs[allow_multi_instances], TRUE);
	integer_apply(&main_v->props.restore_dimensions, pd->prefs[restore_dimensions], TRUE);
	if (!main_v->props.restore_dimensions) {
		integer_apply(&main_v->props.left_panel_width, pd->prefs[left_panel_width], FALSE);
		integer_apply(&main_v->props.main_window_h, pd->prefs[main_window_h], FALSE);
		integer_apply(&main_v->props.main_window_w, pd->prefs[main_window_w], FALSE);
	}
	string_apply(&main_v->props.tab_font_string, pd->prefs[tab_font_string]);
	integer_apply(&main_v->props.ext_browsers_in_submenu, pd->prefs[ext_browsers_in_submenu], TRUE);
	integer_apply(&main_v->props.ext_commands_in_submenu, pd->prefs[ext_commands_in_submenu], TRUE);
	integer_apply(&main_v->props.ext_outputbox_in_submenu, pd->prefs[ext_outputbox_in_submenu], TRUE);
	integer_apply(&main_v->props.transient_htdialogs, pd->prefs[transient_htdialogs], TRUE);
	
	
	integer_apply(&main_v->props.filebrowser_show_hidden_files, pd->prefs[filebrowser_show_hidden_files], TRUE);
	integer_apply(&main_v->props.filebrowser_show_others_files, pd->prefs[filebrowser_show_others_files], TRUE);
	integer_apply(&main_v->props.filebrowser_show_backup_files, pd->prefs[filebrowser_show_backup_files], TRUE);
	string_apply(&main_v->props.filebrowser_unknown_icon, pd->prefs[filebrowser_unknown_icon]);
	string_apply(&main_v->props.filebrowser_dir_icon, pd->prefs[filebrowser_dir_icon]);
	
	string_apply(&main_v->props.image_thumbnailstring, pd->prefs[image_thumbnailstring]);
	string_apply(&main_v->props.image_thumbnailtype, GTK_COMBO(pd->prefs[image_thumbnailtype])->entry);

	
	
	
	
	filetype_apply_changes(pd);
	filefilter_apply_changes(pd);
	highlightpattern_apply_changes(pd);
	browsers_apply_changes(pd);
	externals_apply_changes(pd);

	free_arraylist(main_v->props.filetypes);
	main_v->props.filetypes = pd->lists[filetypes];

	free_arraylist(main_v->props.filefilters);
	main_v->props.filefilters = pd->lists[filefilters];

	free_arraylist(main_v->props.highlight_patterns);
	main_v->props.highlight_patterns = pd->lists[highlight_patterns];
	
	free_arraylist(main_v->props.browsers);
	main_v->props.browsers = pd->lists[browsers];
	
	free_arraylist(main_v->props.external_commands);
	main_v->props.external_commands = pd->lists[external_commands];
	
	free_arraylist(main_v->props.outputbox);
	main_v->props.outputbox	 = pd->lists[outputbox];
	
	/* apply the changes to highlighting patterns and filetypes to the running program */
	filetype_highlighting_rebuild();
	filetype_menu_rebuild(NULL);
	filebrowser_filters_rebuild();
	
	menu_outputbox_rebuild();
	encoding_menu_rebuild();
	external_menu_rebuild();
	
	preferences_destroy_lcb(NULL, NULL, pd);
}
static void preferences_cancel_clicked_lcb(GtkWidget *wid, Tprefdialog *pd) {
	DEBUG_MSG("preferences_cancel_clicked_lcb, started\n");
	free_arraylist(pd->lists[filetypes]);
	free_arraylist(pd->lists[filefilters]);
	free_arraylist(pd->lists[highlight_patterns]);
	free_arraylist(pd->lists[browsers]);
	free_arraylist(pd->lists[external_commands]);
	pd->lists[filetypes] = NULL;
	pd->lists[filefilters] = NULL;
	pd->lists[highlight_patterns] = NULL;
	pd->lists[browsers] = NULL;
	pd->lists[external_commands] = NULL;
	preferences_destroy_lcb(NULL, NULL, pd);
}

static void restore_dimensions_toggled_lcb(GtkToggleButton *togglebutton,Tprefdialog *pd) {
	gtk_widget_set_sensitive(pd->prefs[left_panel_width], !togglebutton->active);
	gtk_widget_set_sensitive(pd->prefs[main_window_h], !togglebutton->active);
	gtk_widget_set_sensitive(pd->prefs[main_window_w], !togglebutton->active);
}
static void create_backup_toggled_lcb(GtkToggleButton *togglebutton,Tprefdialog *pd) {
	gtk_widget_set_sensitive(pd->prefs[backup_filestring], togglebutton->active);
	gtk_widget_set_sensitive(pd->prefs[backup_abort_style], togglebutton->active);
}

static void preferences_dialog() {
	Tprefdialog *pd;
	GtkWidget *dvbox, *frame, *vbox1, *vbox2;

	pd = g_new0(Tprefdialog,1);
	pd->win = window_full(_("Edit preferences"), GTK_WIN_POS_NONE, 0, G_CALLBACK(preferences_destroy_lcb), pd, TRUE);
	
	dvbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(pd->win), dvbox);
	pd->noteb = gtk_notebook_new();
	gtk_notebook_set_homogeneous_tabs(GTK_NOTEBOOK(pd->noteb), TRUE);
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(pd->noteb), GTK_POS_LEFT);
	
	gtk_box_pack_start(GTK_BOX(dvbox), pd->noteb, TRUE, TRUE, 0);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("Editor"),014));

	frame = gtk_frame_new(_("Editor options"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	
	pd->prefs[editor_font_string] = prefs_string(_("Font"), main_v->props.editor_font_string, vbox2, pd, string_font);
	pd->prefs[editor_tab_width] = prefs_integer(_("Tab width"), main_v->props.editor_tab_width, vbox2, pd, 1, 50);
	pd->prefs[word_wrap] = boxed_checkbut_with_value(_("Word wrap default"), main_v->props.word_wrap, vbox2);
	pd->prefs[view_line_numbers] = boxed_checkbut_with_value(_("Line numbers by default"), main_v->props.view_line_numbers, vbox2);
	pd->prefs[defaulthighlight] = boxed_checkbut_with_value(_("Highlight syntax by default"), main_v->props.defaulthighlight, vbox2);
	pd->prefs[highlight_num_lines_count] = prefs_integer(_("Highlight # lines"), main_v->props.highlight_num_lines_count, vbox2, pd, 1, 8);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("HTML"),014));

	frame = gtk_frame_new(_("HTML options"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	
	pd->prefs[lowercase_tags] = boxed_checkbut_with_value(_("Lowercase HTML tags"), main_v->props.lowercase_tags, vbox2);
	pd->prefs[allow_dep] = boxed_checkbut_with_value(_("Use deprecated tags (e.g. <font> and <nobr>)"), main_v->props.allow_dep, vbox2);
	pd->prefs[xhtml] = boxed_checkbut_with_value(_("Use XHTML style tags (<br />)"), main_v->props.xhtml, vbox2);
	
	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("Files"),014));

	frame = gtk_frame_new(_("Encoding"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	{
		GList *tmplist, *poplist = g_list_append(NULL, "");
		tmplist = g_list_first(main_v->props.encodings);
		while (tmplist) {
			gchar **strarr = (gchar **)tmplist->data;
			poplist = g_list_append(poplist, strarr[1]);
			tmplist = g_list_next(tmplist);
		}
		pd->prefs[newfile_default_encoding] = prefs_combo(_("Default character set"),main_v->props.newfile_default_encoding, vbox2, pd, poplist, TRUE);
		g_list_free(poplist);
	}	
	pd->prefs[auto_set_encoding_meta] = boxed_checkbut_with_value(_("Auto set <meta> encoding tag on change"), main_v->props.auto_set_encoding_meta, vbox2);

	frame = gtk_frame_new(_("Backup"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	pd->prefs[backup_file] = boxed_checkbut_with_value(_("Create backup on save"), main_v->props.backup_file, vbox2);
	pd->prefs[backup_filestring] = prefs_string(_("Backup file suffix"), main_v->props.backup_filestring, vbox2, pd, string_none);
	{
		GList *poplist = g_list_append(NULL, "save");
		poplist = g_list_append(poplist, "abort");
		poplist = g_list_append(poplist, "ask");
		pd->prefs[backup_abort_style] = prefs_combo(_("Action on backup failure"),main_v->props.backup_abort_style, vbox2, pd, poplist, FALSE);
		g_list_free(poplist);
	}
	create_backup_toggled_lcb(GTK_TOGGLE_BUTTON(pd->prefs[backup_file]), pd);
	g_signal_connect(G_OBJECT(pd->prefs[backup_file]), "toggled", G_CALLBACK(create_backup_toggled_lcb), pd);

	frame = gtk_frame_new(_("Undo"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	pd->prefs[num_undo_levels] = prefs_integer(_("Undo history size"), main_v->props.num_undo_levels, vbox2, pd, 50, 10000);
	pd->prefs[clear_undo_on_save] = boxed_checkbut_with_value(_("Clear undo history on save"), main_v->props.clear_undo_on_save, vbox2);

	frame = gtk_frame_new(_("Misc"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	pd->prefs[allow_multi_instances] = boxed_checkbut_with_value(_("Allow multi instances of a file"), main_v->props.allow_multi_instances, vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("User interface"), 014));

	frame = gtk_frame_new(_("Dimensions"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	pd->prefs[restore_dimensions] = boxed_checkbut_with_value(_("Restore last used dimensions"), main_v->props.restore_dimensions, vbox2);
	pd->prefs[left_panel_width] = prefs_integer(_("Initial left panel width"), main_v->props.left_panel_width, vbox2, pd, 1, 4000);
	pd->prefs[main_window_h] = prefs_integer(_("Initial window height"), main_v->props.main_window_h, vbox2, pd, 1, 4000);
	pd->prefs[main_window_w] = prefs_integer(_("Initial window width"), main_v->props.main_window_w, vbox2, pd, 1, 4000);
	restore_dimensions_toggled_lcb(GTK_TOGGLE_BUTTON(pd->prefs[restore_dimensions]), pd);
	g_signal_connect(G_OBJECT(pd->prefs[restore_dimensions]), "toggled", G_CALLBACK(restore_dimensions_toggled_lcb), pd);

	frame = gtk_frame_new(_("Misc"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	pd->prefs[ext_browsers_in_submenu] = boxed_checkbut_with_value(_("External browsers in submenu"), main_v->props.ext_browsers_in_submenu, vbox2);
	pd->prefs[ext_commands_in_submenu] = boxed_checkbut_with_value(_("External commands in submenu"), main_v->props.ext_commands_in_submenu, vbox2);
	pd->prefs[ext_outputbox_in_submenu] = boxed_checkbut_with_value(_("External outputbox in submenu"), main_v->props.ext_outputbox_in_submenu, vbox2);
	
	pd->prefs[tab_font_string] = prefs_string(_("Notebook tab font (leave empty for gtk default)"), main_v->props.tab_font_string, vbox2, pd, string_font);
	pd->prefs[transient_htdialogs] = boxed_checkbut_with_value(_("Make HTML dialogs transient"), main_v->props.transient_htdialogs, vbox2);

	frame = gtk_frame_new(_("File browser"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	pd->prefs[filebrowser_show_hidden_files] = boxed_checkbut_with_value(_("Show hidden files"), main_v->props.filebrowser_show_hidden_files, vbox2);
	pd->prefs[filebrowser_show_others_files] = boxed_checkbut_with_value(_("Show other users files"), main_v->props.filebrowser_show_others_files, vbox2);
	pd->prefs[filebrowser_show_backup_files] = boxed_checkbut_with_value(_("Show backup files"), main_v->props.filebrowser_show_backup_files, vbox2);
	pd->prefs[filebrowser_unknown_icon] = prefs_string(_("Unknown icon"), main_v->props.filebrowser_unknown_icon, vbox2, pd, string_file);
	pd->prefs[filebrowser_dir_icon] = prefs_string(_("Directory icon"), main_v->props.filebrowser_dir_icon, vbox2, pd, string_file);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("Images"), 014));

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

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("Filetypes"), 014));

	frame = gtk_frame_new(_("Filetypes"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	create_filetype_gui(pd, vbox2);
	
	frame = gtk_frame_new(_("Filefilters"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	
	create_filefilter_gui(pd, vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("Syntax highlighting"), 014));

	frame = gtk_frame_new(_("Patterns"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	create_highlightpattern_gui(pd, vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("External programs"), 014));

	frame = gtk_frame_new(_("Browsers"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	create_browsers_gui(pd, vbox2);

	frame = gtk_frame_new(_("Utilities and filters"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);

	create_externals_gui(pd, vbox2);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("Output parsers"), 014));
	
	frame = gtk_frame_new(_("Outputbox"));
	gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	
	create_outputbox_gui(pd, vbox2);
	

	/* end, create buttons for dialog now */
	{
		GtkWidget *ahbox, *but;
		ahbox = gtk_hbutton_box_new();
		gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
		gtk_button_box_set_spacing(GTK_BUTTON_BOX(ahbox), 1);

		gtk_box_pack_start(GTK_BOX(dvbox), ahbox, FALSE, FALSE, 0);

		but = bf_stock_cancel_button(GTK_SIGNAL_FUNC(preferences_cancel_clicked_lcb), pd);
		gtk_box_pack_start(GTK_BOX(ahbox), but, TRUE, TRUE, 0);

		but = bf_stock_ok_button(GTK_SIGNAL_FUNC(preferences_ok_clicked_lcb), pd);
		gtk_box_pack_start(GTK_BOX(ahbox), but, TRUE, TRUE, 0);
		gtk_window_set_default(GTK_WINDOW(pd->win), but);
	}
	gtk_widget_show_all(pd->win);
}

void open_preferences_cb(GtkWidget *wid, gpointer data) {
	preferences_dialog();
}

void open_preferences_menu_cb(gpointer callback_data,guint action,GtkWidget *widget) {
	preferences_dialog();
}
