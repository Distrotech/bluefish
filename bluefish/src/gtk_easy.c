/* Bluefish HTML Editor
 *
 * Copyright (C) 1999-2002 Olivier Sessink
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
#include <stdlib.h> /* strtod() */
#include <string.h> /* strlen() */

#include "bluefish.h"
#include "gtk_easy.h"
#include "bf_lib.h"
#include "pixmap.h"

#ifdef WIN32
#define DIRSTR "\\"
#define DIRCHR 92
#else
#define DIRSTR "/"
#define DIRCHR '/'
#endif

/* void flush_queue(void)
 * handles the gtk and gdk queue
 */
void flush_queue(void)
{
	while(g_main_context_pending(NULL)) {
		g_main_context_iteration (NULL, TRUE);
	}
/*	while (gtk_events_pending()) {
		DEBUG_MSG("gtk_events_pending\n");
		gtk_main_iteration();
	}
	while (gdk_events_pending()) {
		DEBUG_MSG("gdk_events_pending\n");
		gdk_flush();
	}*/
}

/*
 * Function: window_destroy
 * Arguments:
 * 	windowname - a pointer to the window widget to destroy
 * Description:
 * 	Remove grab and signals and then destroy window
 */
void window_destroy(GtkWidget * windowname)
{
	DEBUG_MSG("window_destroy, windowname=%p\n", windowname);
	g_signal_handlers_destroy(G_OBJECT(windowname));
	gtk_grab_remove(windowname);
	gtk_widget_destroy(windowname);
}

/*
 * Function: window_close_by_widget_cb
 * Arguments:
 * 	widget - the parent window of this widget will be destroyed
 * 	data - ignored
 * Description:
 * 	destroy the toplevel window for widget
 */
void window_close_by_widget_cb(GtkWidget * widget, gpointer data)
{
	DEBUG_MSG("window_close_by_data_cb, widget=%p\n", widget);
	window_destroy(gtk_widget_get_toplevel(widget));
}

/*
 * Function: window_close_by_data_cb
 * Arguments:
 * 	widget - ignored
 * 	data - a pointer to a gtk-window which will be detroyed
 * Description:
 * 	destroy a window using the gpointer data
 */
void window_close_by_data_cb(GtkWidget * widget, gpointer data)
{
	DEBUG_MSG("window_close_by_data_cb, data=%p\n", data);
	window_destroy(GTK_WIDGET(data));
}

/*
 * Function: setup_toggle_item
 * Arguments:
 * 	ifactory - the itemfactory where the toggle item is in defined
 * 	path - the path in the itemfactory to find the toggle item
 * 	state - the state to put it in (0 or 1)
 * Return value:
 * 	void
 * Description:
 * 	change the setting of a toggle item in a menu
 */
void setup_toggle_item(GtkItemFactory * ifactory, gchar * path, gint state)
{

	GtkWidget *toggle;

	toggle = gtk_item_factory_get_widget(ifactory, path);
	if (!toggle) {
		DEBUG_MSG("warning, this menuitem (%s) does NOT exist!", path);
		return;
	}
	if ((GTK_CHECK_MENU_ITEM(toggle)->active) && state == 0) {
		GTK_CHECK_MENU_ITEM(toggle)->active = state;
		return;
	}
	if ((!GTK_CHECK_MENU_ITEM(toggle)->active) && state == 1) {
		GTK_CHECK_MENU_ITEM(toggle)->active = state;
		return;
	}
}

/*
 * Function: string_apply
 * Arguments:
 * 	config_var - a pointer to a gchar pointer with where it should be placed
 * 	entry - the entry to read
 * Return value:
 * 	void
 * Description:
 * 	free the old string, and replace it with the new string
 */
void string_apply(gchar ** config_var, GtkWidget * entry)
{
	gchar *tmpstring;
	DEBUG_MSG("string_apply, start\n");

	tmpstring = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
	DEBUG_MSG("string_apply, tmpstring(%p)=%s\n", tmpstring, tmpstring);
	if (tmpstring) {
		if (config_var != NULL) {
			g_free(*config_var);
		}
		*config_var = tmpstring;
	}
#ifdef DEBUG
	else {
		DEBUG_MSG("string_apply, ***** tmpstring == NULL *****\n");
	}
#endif
}

/*
 * Function: combo_with_popdown
 * Arguments:
 * 	setstring - string to setb in textbox, if NULL set ""
 * 	which_list - GList to set in popdown widget
 * 	editable - if editable (1 or 0)
 * Return value:
 * 	Pointer to create combo
 * Description:
 * 	Create new combo and preset some values
 */
GtkWidget *combo_with_popdown(gchar * setstring, GList * which_list, gint editable)
{

	GtkWidget *returnwidget;

	returnwidget = gtk_combo_new();
	if (which_list != NULL) {
		gtk_combo_set_popdown_strings(GTK_COMBO(returnwidget), which_list);
	}
	if (setstring != NULL) {
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(returnwidget)->entry), setstring);
	} else {
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(returnwidget)->entry), "");
	}
	if (editable == 0) {
		gtk_editable_set_editable(GTK_EDITABLE(GTK_ENTRY(GTK_COMBO(returnwidget)->entry)), FALSE);
	}
	gtk_combo_disable_activate(GTK_COMBO(returnwidget));
	return returnwidget;
}

GtkWidget *boxed_combo_with_popdown(gchar * setstring, GList * which_list, gint editable, GtkWidget *box) {

	GtkWidget *returnwidget;
	
	returnwidget = combo_with_popdown(setstring, which_list, editable);
	gtk_box_pack_start(GTK_BOX(box), returnwidget, TRUE, TRUE, 3);
	return returnwidget;
}

GtkWidget *combo_with_popdown_sized(gchar * setstring, GList * which_list, gint editable, gint width) {
	GtkWidget *returnwidget;
	
	returnwidget = combo_with_popdown(setstring, which_list, editable);
	gtk_widget_set_usize(returnwidget, width,0);
	return returnwidget;
}

/*
 * Function: entry_with_text
 * Arguments:
 * 	setstring - if !NULL set this text
 * 	max_lenght - max. characters in the entry
 * Return value:
 * 	Pointer to the new entry widget
 * Description:
 * 	Create new entry with some preset values
 */

GtkWidget *entry_with_text(gchar * setstring, gint max_lenght)
{

	GtkWidget *returnwidget;

	if (max_lenght) {
		returnwidget = gtk_entry_new_with_max_length(max_lenght);
	} else {
		returnwidget = gtk_entry_new();
	}
	if (setstring) {
		gtk_entry_set_text(GTK_ENTRY(returnwidget), setstring);
	}
	return returnwidget;

}

GtkWidget *boxed_entry_with_text(gchar * setstring, gint max_lenght, GtkWidget *box) {
	GtkWidget *returnwidget;
	
	returnwidget = entry_with_text(setstring, max_lenght);
	gtk_box_pack_start(GTK_BOX(box), returnwidget, TRUE, TRUE, 3);
	return returnwidget;

}

GtkWidget *boxed_full_entry(gchar * labeltext, gchar * setstring,gint max_lenght, GtkWidget * box) {
	GtkWidget *hbox, *return_widget;

	hbox = gtk_hbox_new(FALSE,3);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(labeltext), FALSE, FALSE, 3);
	return_widget = boxed_entry_with_text(setstring, max_lenght, hbox);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 3);

	return return_widget;
}

GtkWidget *checkbut_with_value(gchar *labeltext, gint which_config_int) {
	GtkWidget *returnwidget;

	returnwidget = gtk_check_button_new_with_label(labeltext);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(returnwidget), which_config_int);
	return returnwidget;
}

GtkWidget *boxed_checkbut_with_value(gchar *labeltext, gint which_config_int, GtkWidget * box) {
	GtkWidget *returnwidget;

	returnwidget = checkbut_with_value(labeltext, which_config_int);
	gtk_box_pack_start(GTK_BOX(box), returnwidget, FALSE, FALSE, 0);
	return returnwidget;
}

GtkWidget *radiobut_with_value(gchar *labeltext, gint enabled, GtkRadioButton *prevbut) {
	GtkWidget *returnwidget;
	GSList *group=NULL;

	if (prevbut) {
		group = gtk_radio_button_group(prevbut);
	}	
	returnwidget = gtk_radio_button_new_with_label(group, labeltext);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(returnwidget), enabled);
	return returnwidget;
}

GtkWidget *boxed_radiobut_with_value(gchar *labeltext, gint enabled, GtkRadioButton *prevbut, GtkWidget *box) {
	GtkWidget *returnwidget;

	returnwidget = radiobut_with_value(labeltext, enabled, prevbut);
	gtk_box_pack_start(GTK_BOX(box), returnwidget, FALSE, FALSE, 0);
	return returnwidget;
}

static gint is_int(gfloat testval) {
	DEBUG_MSG("is_int, (int)testval=%d\n", (int)testval);
	if ((gfloat)(((int) testval) - ((gfloat) testval )) == 0) {
		DEBUG_MSG("is_int, %f int!\n", testval);
		return 1;
	} else {
		DEBUG_MSG("is_int, %f float!\n", testval);
		return 0;
	}
}


GtkWidget *spinbut_with_value(gchar *value, gfloat lower, gfloat upper, gfloat step_increment, gfloat page_increment) {
	GtkAdjustment *adj;
	GtkWidget *returnwidget;
	guint digits;
	double fvalue=0;

	if (value) {
		fvalue = strtod(value, NULL);
	}
	adj = (GtkAdjustment *) gtk_adjustment_new((gfloat)fvalue, (gfloat)lower, (gfloat)upper, step_increment, page_increment, 0.0);
	digits = (is_int(lower) ? 0 : 2);
	returnwidget = gtk_spin_button_new(adj, step_increment, digits);
/*	g_object_set(G_OBJECT(returnwidget), "numeric", TRUE, NULL);*/
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(returnwidget), FALSE);
	gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(returnwidget), FALSE);
/*	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(returnwidget), GTK_UPDATE_IF_VALID);*/
	if (!value) {
		gtk_entry_set_text(GTK_ENTRY(GTK_SPIN_BUTTON(returnwidget)), "");
	}
	
	return returnwidget;
}
/* THIS ONE IS NEVERUSED
GtkWidget *boxed_spinbut_with_value(gchar *value, gfloat lower, gfloat upper, gfloat step_increment, gfloat page_increment, GtkWidget *box) {
	GtkWidget *returnwidget;
	returnwidget = spinbut_with_value(value, lower, upper, step_increment, page_increment);
	gtk_box_pack_start(GTK_BOX(box), returnwidget, FALSE, FALSE, 0);
	return returnwidget;
}
*/
/*
 * Function: window_with_title
 * Arguments:
 * 	title - the title string
 * 	position - the gtk window position
 * 	type - the gtk window type
 * Return value:
 * 	Pointer to created windpw
 * Description:
 * 	Create new window with title and some more settings
 */
GtkWidget *window_with_title(gchar * title, GtkWindowPosition position, gint borderwidth)
{

	GtkWidget *returnwidget;

	returnwidget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	if (title) {
		gtk_window_set_title(GTK_WINDOW(returnwidget), title);
	}
	if (borderwidth) {
		gtk_container_set_border_width(GTK_CONTAINER(returnwidget), borderwidth);
	}
	if (position) {
		gtk_window_set_position(GTK_WINDOW(returnwidget), position);
	}
#ifdef DEBUG
	 else {
		g_print("window_with_title, **NOT** setting position!!\n");
	}
#endif
	return returnwidget;
}
/* GtkWindowPosition can be 
GTK_WIN_POS_NONE
GTK_WIN_POS_CENTER
GTK_WIN_POS_MOUSE */

/*
 * Function: window_full
 * Arguments:
 * 	title - the title string
 * 	position - the gtk window position
 * 	borderwidth - border width
 * 	close_func - the callback function when closing the window
 * 	close_data - the gpointer data passed to the closefunc
 * Return value:
 * 	Pointer to created windpw
 * Description:
 * 	Create new window with title, callback functions and some more settings
 */
GtkWidget *window_full(gchar * title, GtkWindowPosition position
			, gint borderwidth, GCallback close_func
			, gpointer close_data)
{

	GtkWidget *returnwidget;

	returnwidget = window_with_title(title, position, borderwidth);
	g_signal_connect(G_OBJECT(returnwidget), "delete_event", close_func, close_data);
	g_signal_connect(G_OBJECT(returnwidget), "destroy", close_func, close_data);

	return returnwidget;
}

/*
 * Function: apply_font_style
 * Arguments:
 * 	this_widget - the widget to apply the new fontstyle
 * 	fontstring - the fontset which will be loaded
 * Return value:
 * 	this_widget
 * Description:
 * 	Change the font of a widget with a certain font in a string
 */

GtkWidget *apply_font_style(GtkWidget * this_widget, gchar * fontstring)
{
	if (fontstring) {
		PangoFontDescription *font_desc;
		font_desc = pango_font_description_from_string(fontstring);
		gtk_widget_modify_font(this_widget, font_desc);
		pango_font_description_free(font_desc);
	}
	return this_widget;
}

/*
 * Function: bf_stock_button
 * Arguments:
 * 	Text - label string
 * 	func - pointer to signal handler
 * 	func_data - data for signal handler
 * Return value:
 * 	Pointer to create button
 * Description:
 * 	Create new button and link button with "clicked" signal handler
 */
GtkWidget *bf_stock_button(const gchar * Text, GCallback func, gpointer func_data)
{
	GtkWidget *button;

	button = gtk_button_new_with_label(Text);
	g_return_val_if_fail(button, NULL);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	g_signal_connect(G_OBJECT(button), "clicked", func, func_data);
	DEBUG_MSG("bf_stock_button, func_data=%p\n", func_data);
	return button;
};

GtkWidget *bf_gtkstock_button(const gchar * stock_id, GCallback func, gpointer func_data)
{
	GtkWidget *button;

	button = gtk_button_new_from_stock(stock_id);
	g_return_val_if_fail(button, NULL);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	g_signal_connect(G_OBJECT(button), "clicked", func, func_data);
	return button;
};


/*
 * Function: bf_stock_ok_button
 * Arguments:
 * 	func - pointer to signal handler
 * 	func_data - data for signal handler
 * Return value:
 * 	Pointer to create button
 * Description:
 * 	Create new "Ok" button
 */
GtkWidget *bf_stock_ok_button(GtkSignalFunc func, gpointer func_data) {
	return bf_gtkstock_button(GTK_STOCK_OK, func, func_data);
};

/*
 * Function: bf_stock_cancel_button
 * Arguments:
 * 	func - pointer to signal handler
 * 	func_data - data for signal handler
 * Return value:
 * 	Pointer to create button
 * Description:
 * 	Create new "Cancel" button
 */
GtkWidget *bf_stock_cancel_button(GCallback func, gpointer func_data)
{
	return bf_gtkstock_button(GTK_STOCK_CANCEL, func, func_data);
};

/*
 * Function: error_dialog
 * Arguments:
 * 	windiw_title - the title of the error window
 * 	error_string - the description of the error
 * Return value:
 * 	void
 * Description:
 * 	displays an error dialog
 */

void error_dialog(gchar * window_title, gchar * error_string)
{
	GtkWidget *dialog, *okbutton;

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), window_title);
	gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	gtk_container_border_width(GTK_CONTAINER(dialog), 10);
	gtk_window_set_role(GTK_WINDOW(dialog), "error_dialog");
	okbutton = bf_stock_ok_button(G_CALLBACK(window_close_by_data_cb), dialog);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), okbutton, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS(okbutton, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(okbutton);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), gtk_label_new(error_string), TRUE, TRUE, 0);
	gtk_widget_show_all(dialog);
}



/************************************************************************/

/*
 * Function: file_but_new
 * Arguments:
 * 	which_entry - GTK_ENTRY where to put the filename
 * 	win - the GTK_WINDOW where the button is placed
 * 	full_pathname - 1 or 0, if you want the full path or a relative path
 * Return value:
 * 	Pointer to file button
 * Description:
 * 	Create new "open file" button
 */

static void file_but_clicked_lcb(GtkWidget * widget, GtkWidget * which_entry)
{
	gchar *tmpstring, *tmp2string;

	tmpstring = return_file(NULL);
	if (tmpstring != NULL) {
		if (main_v->current_document->filename != NULL) {
			tmp2string = create_relative_link_to(main_v->current_document->filename, tmpstring);
			g_free(tmpstring);
			tmpstring = tmp2string;
		}
		gtk_entry_set_text(GTK_ENTRY(which_entry), tmpstring);
		gtk_signal_emit_by_name(GTK_OBJECT(which_entry), "activate");
		g_free(tmpstring);
	}
}


static void file_but_clicked_full_lcb(GtkWidget * widget, GtkWidget * which_entry)
{
	gchar *tmpstring, *setfile;
	setfile = gtk_editable_get_chars(GTK_EDITABLE(GTK_ENTRY(which_entry)),0,-1);
	tmpstring = return_file(setfile);
	g_free(setfile);
	if (tmpstring != NULL) {
		gtk_entry_set_text(GTK_ENTRY(which_entry), tmpstring);
		g_free(tmpstring);
	}
}

GtkWidget *file_but_new(GtkWidget * which_entry, GtkWidget * win, gint full_pathname)
{
/*	GtkWidget *pixmap;*/
	GtkWidget *file_but;

/*	file_but = gtk_button_new();
	pixmap = new_pixmap(177);
	gtk_widget_show(pixmap);
	gtk_container_add(GTK_CONTAINER(file_but), pixmap);
	file_but = gtk_button_new_with_label("file..");*/
	file_but = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	if (full_pathname == 1) {
		g_signal_connect(G_OBJECT(file_but), "clicked", G_CALLBACK(file_but_clicked_full_lcb), which_entry);
	} else {
		g_signal_connect(G_OBJECT(file_but), "clicked", G_CALLBACK(file_but_clicked_lcb), which_entry);
	}
	gtk_widget_show(file_but);
	return file_but;
}


/**************************************************************************/

#define MAX_MUBUDI_BUT 9
typedef struct {
	GtkWidget *win;
	GtkWidget *buts[MAX_MUBUDI_BUT];
	gint retval;
} Tmubudi;

static void mubudi_destroy_lcb(GtkWidget * widget, GdkEvent *event, 	Tmubudi *mbd) {
	DEBUG_MSG("mubudi_destroy_lcb, quitting gtk main loop\n");
	gtk_main_quit();
}

static void mubudi_but_clicked_lcb(GtkWidget * widget, Tmubudi *mbd)
{
	gint i=0;

	while (mbd->buts[i]) {
		if (mbd->buts[i] == widget) {
			mbd->retval = i;
			DEBUG_MSG("mubudi_but_clicked_lcb, retval=%d\n", i);
		}
		DEBUG_MSG("mubudi_but_clicked_lcb, checking %d, if %p=%p\n", i, mbd->buts[i], widget);
		i++;
	}
	mubudi_destroy_lcb(NULL, NULL, mbd);
}

/*
 * Function: multi_button_dialog
 * Arguments:
 * 	title - title string
 * 	defval = default value (when window is closed)
 * 	label - label string
 * 	button - NULL terminated array of strings, containing the button labels
 * Return value:
 * 	the index num of the button label which was clicked
 * 	so 0 for the first label, 1 for the 2nd label, etc.
 * Description:
 * 	create dialog with several buttons which returns a value
 */

gint multi_button_dialog_backend(gboolean use_gtk_stock_buttons, gchar *title, gint defval, gchar *label, gchar **buttons) {

	Tmubudi *mbd;

	GtkWidget *vbox, *hbox;
	gint retval, count;

	mbd = g_malloc0(sizeof(Tmubudi));
	mbd->retval = defval;

	mbd->win = window_full(title, GTK_WIN_POS_CENTER, 5,
			G_CALLBACK(mubudi_destroy_lcb), mbd);
	gtk_grab_add(mbd->win);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(mbd->win), vbox);
	gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(label), FALSE, FALSE, 0);
	hbox = gtk_hbox_new(TRUE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	count = 0;
	while (buttons[count]) {
		if (use_gtk_stock_buttons) {
			mbd->buts[count] = bf_gtkstock_button(buttons[count], G_CALLBACK(mubudi_but_clicked_lcb), mbd);
		} else {
			mbd->buts[count] = bf_stock_button(buttons[count], G_CALLBACK(mubudi_but_clicked_lcb), mbd);
		}
		if (count == defval) {
			gtk_window_set_default(GTK_WINDOW(mbd->win), mbd->buts[count]);
		}
		gtk_box_pack_start(GTK_BOX(hbox), mbd->buts[count], TRUE, TRUE, 5);
		count++;
	}

	gtk_widget_show_all(mbd->win);
	gtk_window_set_transient_for(GTK_WINDOW(mbd->win), GTK_WINDOW(main_v->main_window));
	gtk_main();
	window_destroy(mbd->win);
	retval = mbd->retval;
	free(mbd);
	return retval;
}

gint multi_stockbutton_dialog(gchar *title, gint defval, gchar *label, gchar **buttons) {
	return multi_button_dialog_backend(TRUE, title, defval, label, buttons);
}

gint multi_button_dialog(gchar *title, gint defval, gchar *label, gchar **buttons) {
	return multi_button_dialog_backend(FALSE, title, defval, label, buttons);
}

/************************************************************************/
/************    FILE SELECTION FUNCTIONS  ******************************/
/************************************************************************/
typedef struct {
	gint multipleselect;
	gchar *filename_to_return;
	GList *filenames_to_return;
	GtkWidget *fs;
} Tfileselect;

static void fs_ok_clicked_lcb(GtkWidget * widget, Tfileselect *fileselect)
{
	gchar *dirname, *selected_file;

	selected_file = g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION(fileselect->fs)));

	if (file_is_dir(selected_file)) {
		DEBUG_MSG("fs_ok_clicked_lcb,file_is_dir said %s is a dir!!!!\n", selected_file);
		dirname = ending_slash(selected_file);
		gtk_file_selection_set_filename(GTK_FILE_SELECTION(fileselect->fs), dirname);
		gtk_entry_set_text(GTK_ENTRY(GTK_FILE_SELECTION(fileselect->fs)->selection_entry), "");
		g_free(dirname);
		g_free(selected_file);
		return;
	} else {
		DEBUG_MSG("fs_ok_clicked_lcb,file_is_dir said %s is NOT a dir.\n", selected_file);
	}

	if (fileselect->multipleselect) {
		gchar **filenames, **orig;
		/* multiple files allowed --> scan trough the list for selections */
		orig = filenames = gtk_file_selection_get_selections(GTK_FILE_SELECTION(fileselect->fs));
		while (*filenames) {
			fileselect->filenames_to_return = g_list_append(fileselect->filenames_to_return, g_strdup(*filenames));
			filenames++;
		}
		g_strfreev(orig);
	} else {
		/* NO multiple files allowed --> return just one file */
		fileselect->filename_to_return = g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION(fileselect->fs)));
	}
	if ((fileselect->multipleselect 
			&& (g_list_length(fileselect->filenames_to_return) < 1)) 
		|| (!fileselect->multipleselect 
			&& strlen(fileselect->filename_to_return) < 1)){
		error_dialog(_("Bluefish error"), _("No filename known"));
	}
	
	g_free(selected_file);	
	gtk_main_quit();
	window_destroy(GTK_WIDGET(fileselect->fs));
}

static void close_modal_window_lcb(GtkWidget * widget, gpointer window)
{
	gtk_main_quit();
	window_destroy(window);
}

static void fs_history_pulldown_activate_lcb(GtkWidget *menuitem,Tfileselect *fileselect) {
	const gchar *filename = gtk_entry_get_text(GTK_ENTRY(GTK_FILE_SELECTION(fileselect->fs)->selection_entry));
	gchar *dirname = ending_slash(gtk_label_get_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(menuitem)))));
	gchar *fullpath = g_strconcat(dirname, filename, NULL);
	g_free(dirname);
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(fileselect->fs), fullpath);
	g_free(fullpath);
}

static void fs_history_pulldown_changed(GtkOptionMenu *optionmenu,Tfileselect *fileselect) {
	GtkWidget *menuitem, *menu;
	GList *tmplist;
	DEBUG_MSG("fs_history_pulldown_changed\n");
	menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(optionmenu));
	tmplist = g_list_first(main_v->recent_directories);
	while (tmplist) {
		menuitem = gtk_menu_item_new_with_label((gchar *)tmplist->data);
		g_signal_connect(G_OBJECT(menuitem),"activate",G_CALLBACK(fs_history_pulldown_activate_lcb),fileselect);
		gtk_widget_show(menuitem);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
		tmplist = g_list_next(tmplist);
	}
}

static void fileselectwin(gchar *setfile, Tfileselect *fileselect, gchar *title) {

	DEBUG_MSG("fileselectwin, started\n");
	fileselect->fs = gtk_file_selection_new(title);
	gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(fileselect->fs));
	g_signal_connect(G_OBJECT(fileselect->fs), "destroy", G_CALLBACK(close_modal_window_lcb), fileselect->fs);
	g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(fileselect->fs)->cancel_button),
					   "clicked", G_CALLBACK(close_modal_window_lcb), fileselect->fs);
	g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(fileselect->fs)->ok_button), "clicked", G_CALLBACK(fs_ok_clicked_lcb), fileselect);
	g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(fileselect->fs)->history_pulldown), "changed", G_CALLBACK(fs_history_pulldown_changed), fileselect);

	if (fileselect->multipleselect) {
		gtk_file_selection_set_select_multiple(GTK_FILE_SELECTION(fileselect->fs), TRUE);
	}
	if (setfile) {
		gtk_file_selection_set_filename(GTK_FILE_SELECTION(fileselect->fs), setfile);
	}
	gtk_window_set_role(GTK_WINDOW(fileselect->fs), "fileselect");

	gtk_widget_show(fileselect->fs);
	gtk_grab_add(GTK_WIDGET(fileselect->fs));
	gtk_widget_realize(GTK_WIDGET(fileselect->fs));
	gtk_window_set_transient_for(GTK_WINDOW(fileselect->fs), GTK_WINDOW(main_v->main_window));
}

gchar *return_file_w_title(gchar * setfile, gchar *title) {
	Tfileselect fileselect={0, NULL, NULL, NULL};

	fileselectwin(setfile, &fileselect, title);
	gtk_main();
	return fileselect.filename_to_return;
}

gchar *return_file(gchar * setfile) {
	return return_file_w_title(setfile, _("Select file"));
}

GList *return_files_w_title(gchar * setfile, gchar *title) {
	Tfileselect fileselect={1, NULL, NULL, NULL};
	
	fileselectwin(setfile, &fileselect, title);
	gtk_main();
	return fileselect.filenames_to_return;
}

GList *return_files(gchar * setfile) {
	return return_files_w_title(setfile, _("Select files"));
}

/************************************************************************/
