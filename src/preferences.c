#define DEBUG

#include <gtk/gtk.h>

#include "bluefish.h"
#include "gtk_easy.h"
#include "pixmap.h"

typedef enum {
	filebrowser_show_hidden_files,
	filebrowser_show_others_files,
	filebrowser_show_backup_files,
	filebrowser_unknown_icon,
	filebrowser_dir_icon,
	editor_font_string,		/* editor font */
	editor_tab_width,	/* editor tabwidth */
	tab_font_string,		/* notebook tabs font */
	highlight_num_lines_count, /* number of lines to highlight in continous highlighting */	

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
	defaulthighlight,		/* highlight documents by default */
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
} Tproperty;

typedef struct {
	GtkWidget *prefs[property_num_max];
	GtkWidget *win;
	GtkWidget *noteb;
} Tprefdialog;

typedef enum {
	string_none,
	string_file,
	string_font
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
	gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(fsd), fontname);
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

static void preferences_destroy_lcb(GtkWidget * widget, GdkEvent *event, Tprefdialog *pd) {
	window_destroy(pd->win);
	g_free(pd);
}
static void preferences_ok_clicked_lcb(GtkWidget *wid, Tprefdialog *pd) {
	string_apply(&main_v->props.editor_font_string, pd->prefs[editor_font_string]);
	integer_apply(&main_v->props.editor_tab_width, pd->prefs[editor_tab_width], FALSE);
	integer_apply(&main_v->props.highlight_num_lines_count, pd->prefs[highlight_num_lines_count], FALSE);
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
	integer_apply(&main_v->props.transient_htdialogs, pd->prefs[transient_htdialogs], TRUE);
	integer_apply(&main_v->props.filebrowser_show_hidden_files, pd->prefs[filebrowser_show_hidden_files], TRUE);
	integer_apply(&main_v->props.filebrowser_show_others_files, pd->prefs[filebrowser_show_others_files], TRUE);
	integer_apply(&main_v->props.filebrowser_show_backup_files, pd->prefs[filebrowser_show_backup_files], TRUE);
	string_apply(&main_v->props.filebrowser_unknown_icon, pd->prefs[filebrowser_unknown_icon]);
	string_apply(&main_v->props.filebrowser_dir_icon, pd->prefs[filebrowser_dir_icon]);
	

	
	preferences_destroy_lcb(NULL, NULL, pd);
}
static void preferences_cancel_clicked_lcb(GtkWidget *wid, Tprefdialog *pd) {
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

	pd = g_new(Tprefdialog,1);
	pd->win = window_full(_("Edit preferences"), GTK_WIN_POS_NONE, 0, G_CALLBACK(preferences_destroy_lcb), pd);
	
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
	pd->prefs[highlight_num_lines_count] = prefs_integer(_("Highlight # lines"), main_v->props.highlight_num_lines_count, vbox2, pd, 1, 8);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_notebook_append_page(GTK_NOTEBOOK(pd->noteb), vbox1, hbox_with_pix_and_text(_("Files"),014));

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
	pd->prefs[tab_font_string] = prefs_string(_("Notebook tab font"), main_v->props.tab_font_string, vbox2, pd, font);
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

	
	{
		GtkWidget *ahbox, *but;
		ahbox = gtk_hbutton_box_new();
		gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
		gtk_button_box_set_spacing(GTK_BUTTON_BOX(ahbox), 1);

		gtk_box_pack_start(GTK_BOX(dvbox), ahbox, FALSE, FALSE, 0);

		but = bf_stock_ok_button(GTK_SIGNAL_FUNC(preferences_ok_clicked_lcb), pd);
		gtk_box_pack_start(GTK_BOX(ahbox), but, TRUE, TRUE, 0);
		gtk_window_set_default(GTK_WINDOW(pd->win), but);
		but = bf_stock_cancel_button(GTK_SIGNAL_FUNC(preferences_cancel_clicked_lcb), pd);
		gtk_box_pack_start(GTK_BOX(ahbox), but, TRUE, TRUE, 0);
	}
	gtk_widget_show_all(pd->win);
}

void open_preferences_cb(GtkWidget *wid, gpointer data) {
	preferences_dialog();
}
