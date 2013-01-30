/* Bluefish HTML Editor
 * gtk_easy.c
 *
 * Copyright (C) 1999-2012 Olivier Sessink
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
#include <stdlib.h>				/* strtod() */
#include <string.h>				/* strlen() */
#include <gdk/gdkkeysyms.h>		/* GDK_Return */

#include "bluefish.h"
#include "gtk_easy.h"
#include "bf_lib.h"
#include "bfwin.h"
#include "pixmap.h"
#include "dialog_utils.h"
#include "filefilter.h"

#ifdef WIN32
#define DIRSTR "\\"
#define DIRCHR 92
#else
#define DIRSTR "/"
#define DIRCHR '/'
#endif

/**
 * flush_queue:
 *
 * handles the gtk and gdk queue
 *
 * Return value: void
 **/
void
flush_queue(void)
{
	while (g_main_context_pending(NULL)) {
		g_main_context_iteration(NULL, TRUE);
	}
}

/*
 * this function creates a GtkListStore with numcols columns, where numcols >= 2
 * every row will be filled from the data pointer of the list, which should be an array
 * every column is filled with the entries in the array, except the last one, that
 * will contain the array itself.
 *
 * the last column will thus always contain a pointer to the array
 *
 */
#ifdef REQUIRED
GtkTreeModel *
treemodel_from_arraylist(GList * list, gint numcols)
{
	GList *tmplist;
	GtkTreeModel *retm;
	GtkTreeIter iter;
	GType *types;
	int i;

	if (numcols < 2)
		return NULL;

	types = g_new(GType, numcols);
	for (i = 0; i < numcols - 1; i++) {
		types[i] = G_TYPE_STRING;
	}
	types[numcols - 1] = G_TYPE_POINTER;
	retm = GTK_TREE_MODEL(gtk_list_store_newv(numcols, types));

	for (tmplist = g_list_first(list); tmplist; tmplist = tmplist->next) {
		gchar **arr = (gchar **) tmplist->data;
		if (g_strv_length(arr) >= numcols - 1) {
			gtk_list_store_append(GTK_LIST_STORE(retm), &iter);
			for (i = 0; i < numcols - 1; i++) {
				/*g_print("set column %d to value %s\n", i, arr[i]);*/
				gtk_list_store_set(GTK_LIST_STORE(retm), &iter, i, arr[i], -1);
			}
			gtk_list_store_set(GTK_LIST_STORE(retm), &iter, numcols - 1, arr, -1);
		}
	}
	return retm;
}
#endif

/**
 * widget_get_string_size:
 * @widget: #GtkWidget* to put the string on
 * @string: #ghcar* with the string
 *
 * This function will calculate the width in pixels from the
 * string passed to it in string, using the font from widget
 *
 * Return value: #gint pixels
 */
gint
widget_get_string_size(GtkWidget * widget, gchar * string)
{
	PangoLayout *layout;
	gint retval = -1;
	layout = gtk_widget_create_pango_layout(widget, string);
	if (layout != NULL) {
/*		PangoFontDescription *pfd = pango_layout_get_font_description(layout);
		if (pfd) {
			gchar *tmp = pango_font_description_to_string(pfd);
			g_print("widget_get_string_size, widget=%p, use font %s\n",widget, tmp);
			g_free(tmp);
		} else {
			g_print("widget_get_string_size, widget=%p has a NULL font description\n",widget);
		}*/
		pango_layout_get_pixel_size(layout, &retval, NULL);
		g_object_unref(G_OBJECT(layout));
	}/* else {
		g_print("widget_get_string_size, failed to get a pango layout for widget %p\n",widget);
	}*/
	return retval;
}

void
widget_set_visible(GtkWidget * widget, gboolean visible)
{
	if (visible) {
		gtk_widget_show(widget);
	} else {
		gtk_widget_hide(widget);
	}
}

/**
 * window_destroy:
 * @windowname: a #GtkWidget pointer to the window
 *
 * Remove grab and signals and then destroy window
 *
 * Return value: void
 **/
void
window_destroy(GtkWidget * windowname)
{
	DEBUG_MSG("window_destroy, windowname=%p, first the signal handlers\n", windowname);
	g_signal_handlers_destroy(G_OBJECT(windowname));
	DEBUG_MSG("window_destroy, then remove the grab\n");
	gtk_grab_remove(windowname);
	DEBUG_MSG("window_destroy, then destroy the widget\n");
	gtk_widget_destroy(windowname);
	DEBUG_MSG("window_destroy, done\n");
}

/**
 * window_close_by_widget_cb:
 * 	@widget: #GtkWidget the parent window of this widget will be destroyed
 * 	@data: gpointer, ignored
 *
 * 	destroy the toplevel window for widget
 *
 * Return value: void
 */
void
window_close_by_widget_cb(GtkWidget * widget, gpointer data)
{
	DEBUG_MSG("window_close_by_widget_cb, widget=%p\n", widget);
	window_destroy(gtk_widget_get_toplevel(widget));
}

/**
 * window_close_by_data_cb:
 * 	@widget: #GtkWidget, ignored
 * 	@data: #gpointer to a gtk-window which will be detroyed
 *
 * 	destroy a window using the gpointer data
 *
 * Return value: void
 */
void
window_close_by_data_cb(GtkWidget * widget, gpointer data)
{
	DEBUG_MSG("window_close_by_data_cb, data=%p\n", data);
	window_destroy(GTK_WIDGET(data));
}

/**
 * button_apply:
 * @config_var: # gchar **, a pointer to a gchar pointer with where the content should be placed
 * 	@entry: the button #GtkWidget * to read
 *
 * 	free the old string in **config_var, and replaces it with the new string from button label
 *   I assume that button is destroyed, so I create copy of a label
 *
 * Return value:	void
 */
void
button_apply(gchar ** config_var, GtkWidget * entry)
{
	const gchar *tmpstring;
	DEBUG_MSG("button_apply, start\n");

	if (!config_var || !entry)
		return;

	tmpstring = gtk_button_get_label(GTK_BUTTON(entry));
	DEBUG_MSG("button_apply, tmpstring(%p)=%s\n", tmpstring, tmpstring);
	if (tmpstring) {
		if (*config_var != NULL) {
			g_free(*config_var);
		}
		*config_var = g_strdup(tmpstring);	/* copy */
	}
#ifdef DEBUG
	else {
		DEBUG_MSG("button_apply, ***** tmpstring == NULL *****\n");
	}
#endif
}

/**
 * string_apply:
 * @config_var: # gchar **, a pointer to a gchar pointer with where the content should be placed
 * @widget: the entry #GtkWidget * to read
 *
 * 	free the old string in **config_var, and replaces it with the new string from entry
 *
 * Return value:	void
 */
void
string_apply(gchar ** config_var, GtkWidget * widget)
{
	gchar *tmpstring;

	if (!config_var || !widget)
		return;

	if (GTK_IS_FONT_BUTTON(widget)) {
		tmpstring = g_strdup(gtk_font_button_get_font_name(GTK_FONT_BUTTON(widget)));
	} else if (GTK_IS_COLOR_BUTTON(widget)) {
		GdkColor color;
		gtk_color_button_get_color(GTK_COLOR_BUTTON(widget), &color);
		tmpstring = g_strdup_printf("#%02X%02X%02X", color.red/256, color.green/256, color.blue/256);
		DEBUG_MSG("return color %s\n",tmpstring);
		/*tmpstring = gdk_color_to_string(&color);*/
	}
#if GTK_CHECK_VERSION(2,24,0)
	else if (GTK_IS_COMBO_BOX_TEXT(widget))
#else
	else if (GTK_IS_COMBO_BOX(widget))
#endif
	{
		tmpstring = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
	} else {
		tmpstring = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
	}
	DEBUG_MSG("string_apply, tmpstring(%p)=%s\n", tmpstring, tmpstring);
	if (tmpstring) {
		if (*config_var != NULL) {
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

/**
 * string_apply:
 * @config_var: #gint*, a pointer to a gint with where the content should be placed
 * 	@entry: the spinbutton or checkbox #GtkWidget * to read
 * @is_checkbox: a gboolean, TRUE if entry is a checkbox
 *
 * 	sets the content from the widget to the variable pointed to by the config_var pointer
 *
 * Return value: void
 */
void
integer_apply(gint * config_var, GtkWidget * widget, gboolean is_checkbox)
{
	if (is_checkbox) {
		*config_var = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
	} else {
		*config_var = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	}
	DEBUG_MSG("integer_apply, config_var(%p)=%i\n", config_var, *config_var);
}

static void
boxed_widget(const gchar * labeltext, GtkWidget * widget, GtkWidget * box)
{
	GtkWidget *hbox, *label;

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 3);
	label = gtk_label_new_with_mnemonic(labeltext);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 3);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), widget);
}

GtkWidget *
combobox_with_popdown_sized(const gchar * setstring, const GList * which_list, gint editable, gint width)
{
	GtkWidget *returnwidget;

	returnwidget = combobox_with_popdown(setstring, which_list, editable);
	gtk_widget_set_size_request(returnwidget, width, -1);
	return returnwidget;
}

GtkWidget *
boxed_combobox_with_popdown(const gchar * setstring, const GList * which_list, gboolean editable, GtkWidget * box)
{
	GtkWidget *returnwidget = combobox_with_popdown(setstring, which_list, editable);
	gtk_box_pack_start(GTK_BOX(box), returnwidget, TRUE, TRUE, 3);
	return returnwidget;
}
void
combobox_empty(GtkWidget *combobox)
{
#if GTK_CHECK_VERSION(3,0,0)
	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(combobox));
#else
	gint num;
	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(combobox));
	if (!model)
		return;
	num = gtk_tree_model_iter_n_children(model, NULL);
	while (num > 0) {
		gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(combobox), 0);
		num--;
	}
#endif
}

void
combobox_fill(GtkWidget *combobox, const gchar * setstring, const GList * which_list)
{
	GList *tmplist;
	gint activenum = -1, i = 0;
	for (tmplist = g_list_last((GList *)which_list); tmplist; tmplist = g_list_previous(tmplist)) {
		if (tmplist->data) {
			DEBUG_MSG("combobox_fill, prepend %s\n",(gchar *)tmplist->data);
			gtk_combo_box_text_prepend_text(GTK_COMBO_BOX_TEXT(combobox), tmplist->data);
			if (setstring && g_strcmp0(tmplist->data, setstring) == 0) {
				activenum = i;
				DEBUG_MSG("combobox_fill, %s has got activenum=%d\n",(gchar *)tmplist->data, activenum);
			}
			i++;
		}
	}
	/* if we PREpend, the activenum is the number starting from the end!! */
	if (activenum != -1)
		activenum=(i-1)-activenum;
	if (setstring) {
		DEBUG_MSG("combobox_fill, setstring=%s, activenum=%d\n",setstring, activenum);
		if (activenum == -1) {
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), setstring);
			gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), i);
		} else {
			gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), activenum);
		}
	} else {
		/* for append this should be set to i-1, for prepend set to 0 */
		gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);
	}
}

/* if setstring is NULL the first item will be set as default, if setstring is "" an empty item will be set active */
GtkWidget *
combobox_with_popdown(const gchar * setstring, const GList * which_list, gboolean editable)
{
	GtkWidget *returnwidget;
	if (editable)
		returnwidget = gtk_combo_box_text_new_with_entry();
	else
		returnwidget = gtk_combo_box_text_new();
	combobox_fill(returnwidget, setstring, which_list);
	if (editable) {
		gtk_entry_set_activates_default(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(returnwidget))), TRUE);
	}
	return returnwidget;
}

/**
 * checkbut_with_value:
 * @labeltest: #const gchar * with the text for the label
 * 	@which_config_int: #gint whether or not to set the checkbutton active
 *
 * 	Create new checkbutton with some value
 *
 * Return value: #GtkWidget* pointer to the new checkbutton widget
 */
GtkWidget *
checkbut_with_value(gchar * labeltext, gint which_config_int)
{
	GtkWidget *returnwidget;

	returnwidget = gtk_check_button_new_with_mnemonic(labeltext);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(returnwidget), which_config_int);
	return returnwidget;
}

/**
 * boxed_checkbut_with_value:
 * @labeltest: #const gchar * with the text for the label
 * 	@which_config_int: #gint whether or not to set the checkbutton active
 * @box: the #GtkWidget* box widget to add the entry to
 *
 * 	Create new checkbutton with some value, and add it to box
 *
 * Return value: #GtkWidget* pointer to the new checkbutton widget
 */
GtkWidget *
boxed_checkbut_with_value(gchar * labeltext, gint which_config_int, GtkWidget * box)
{
	GtkWidget *returnwidget;

	returnwidget = checkbut_with_value(labeltext, which_config_int);
	gtk_box_pack_start(GTK_BOX(box), returnwidget, FALSE, FALSE, 3);
	return returnwidget;
}

/**
 * radiobut_with_value:
 * @labeltest: #const gchar * with the text for the label
 * 	@enabled: #gint if the radiobutton is enabled or not
 * @prevbut: #GtkRadioButton* pointer to the first button or NULL if none
 *
 * 	Create new radiobutton with some value, and if there is a first button
 * for this group, add this button to that group
 *
 * Return value: #GtkWidget* pointer to the new radiobutton widget
 */
GtkWidget *
radiobut_with_value(gchar * labeltext, gint enabled, GtkRadioButton * prevbut)
{
	GtkWidget *returnwidget;
	GSList *group = NULL;

	if (prevbut) {
		group = gtk_radio_button_get_group(prevbut);
	}
	returnwidget = gtk_radio_button_new_with_mnemonic(group, labeltext);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(returnwidget), enabled);
	return returnwidget;
}

/**
 * boxed_radiobut_with_value:
 * @labeltest: #const gchar * with the text for the label
 * 	@enabled: #gint if the radiobutton is enabled or not
 * @prevbut: #GtkRadioButton* pointer to the first button or NULL if none
 * @box: the #GtkWidget* box widget to add the entry to
 *
 * 	Create new radiobutton with some value, and if there is a first button
 * for this group, add this button to that group, and this button to box
 *
 * Return value: #GtkWidget* pointer to the new radiobutton widget
 */
GtkWidget *
boxed_radiobut_with_value(gchar * labeltext, gint enabled, GtkRadioButton * prevbut, GtkWidget * box)
{
	GtkWidget *returnwidget;

	returnwidget = radiobut_with_value(labeltext, enabled, prevbut);
	gtk_box_pack_start(GTK_BOX(box), returnwidget, FALSE, FALSE, 0);
	return returnwidget;
}

/*static gint
is_int(gfloat testval)
{
	DEBUG_MSG("is_int, (int)testval=%d\n", (int) testval);
	if ((gfloat) (((int) testval) - ((gfloat) testval)) == 0) {
		DEBUG_MSG("is_int, %f int!\n", testval);
		return 1;
	} else {
		DEBUG_MSG("is_int, %f float!\n", testval);
		return 0;
	}
}
*/
/**
 * spinbut_with_value:
 * @value: #const gchar * with the value as string for the spinbut
 * 	@lower: a #gfloat with the lower value for the range
 * @upper: a #gfloat with the upper value for the range
 * @step_increment: a #gfloat with the stepsize for the spinbutton
 * @page_increment: a #gfloat with the pagesize (pgup/pgdn) for the spinbutton
 *
 * 	Create new spinbutton with the specified value and range
 *
 * Return value: #GtkWidget* pointer to the new spinbutton widget
 */

static gboolean
spinbut_output(GtkSpinButton *spin,gpointer data)
{
	const gchar *text;
	gchar *endptr;
	double testval;
	/* if text contains a number, we return FALSE and thus use the adjustment to update the number,
	if it contains text we return TRUE and thus do nothing */
	if (((int)gtk_adjustment_get_value(gtk_spin_button_get_adjustment(spin))) != 0) {
		return FALSE;
	}
	text = gtk_entry_get_text(GTK_ENTRY(spin));
	testval = strtod(text, &endptr);
	if (endptr == text) {
		/*g_print("spinbut_output, no conversion to float possible for '%s', adjustment has value %d, return TRUE\n", text, (int)gtk_adjustment_get_value(gtk_spin_button_get_adjustment(spin)));*/
		return TRUE;
	}
	/*g_print("spinbut_output, conversion to floatfor '%s' was %f, return FALSE\n", text, testval);*/
	return FALSE;
}

GtkWidget *
spinbut_with_value(gchar * value, gfloat lower, gfloat upper, gfloat step_increment, gfloat page_increment)
{
/*	GtkAdjustment *adj;
	GtkWidget *returnwidget;
	guint digits;
	double fvalue = 0;

	if (value && value[0]!='\0') {
		fvalue = strtod(value, NULL);
	}
	adj =
		(GtkAdjustment *) gtk_adjustment_new((gfloat) fvalue, (gfloat) lower, (gfloat) upper, step_increment,
											 page_increment, 0.0);
	digits = (is_int(lower) ? 0 : 2);
	returnwidget = gtk_spin_button_new(adj, step_increment, digits);
	gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(returnwidget), FALSE);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(returnwidget), FALSE);
	if (!value || value[0]=='\0') {
		gtk_entry_set_text(GTK_ENTRY(returnwidget), value);
	}*/

	GtkWidget *returnwidget;
	gchar *endptr;
	double testval;
	returnwidget = gtk_spin_button_new_with_range(lower,upper,step_increment);
	if (value) {
		testval = strtod(value, &endptr);
		if (endptr != value) {
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(returnwidget), testval);
		}
	}
	g_signal_connect(G_OBJECT(returnwidget), "output", G_CALLBACK(spinbut_output), returnwidget);
	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(returnwidget), GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(returnwidget), FALSE);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(returnwidget), FALSE);
	gtk_entry_set_text(GTK_ENTRY(returnwidget), value?value:"");
	return returnwidget;
}

/**
 * window_with_title:
 * 	@title: #gchar* the title string
 * 	@position: the gtk window position
 * 	@type: the gtk window type
 *
 * 	Create new window with title and some more settings
 *
 * Return value: #GtkWidget* pointer to created window
 */
GtkWidget *
window_with_title(const gchar * title, GtkWindowPosition position, gint borderwidth)
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

static gboolean
window_full_key_press_event_lcb(GtkWidget * widget, GdkEventKey * event, GtkWidget * win)
{
	DEBUG_MSG("window_full_key_press_event_lcb, started\n");
	if (event->keyval == GDK_KEY_Escape) {
		DEBUG_MSG("window_full_key_press_event_lcb, emit delete_event on %p\n", win);
/*		g_signal_emit_by_name(G_OBJECT(win), "delete_event");*/
		gtk_widget_destroy(win);
		DEBUG_MSG("window_full_key_press_event_lcb, DESTROYED %p\n", win);
		return TRUE;
	}
	return FALSE;
}

void
window_delete_on_escape(GtkWindow * window)
{
	g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(window_full_key_press_event_lcb),
					 window);
}

/**
 * window_full2:
 * 	@title: #gchar* the title string
 * 	@position: #gint the gtk window position GTK_WIN_POS_NONE, GTK_WIN_POS_CENTER, GTK_WIN_POS_MOUSE
 * 	@borderwidth: #gint border width
 * 	@close_func: #GCallback the callback function when closing the window
 * 	@close_data: #gpointer data passed to the closefunc, or NULL to pass the widget pointer as close data
 * @delete_on_escape: #gboolean if the escape key should close the dialog
 * @transientforparent: #GtkWidget* with the window to set this transient for, or NULL
 *
 * Create new window with title, callback functions, some more settings
 * and if needed set a callback so the window will be closed on escape press
 * and set it if needed transient for another window
 *
 * the close_func should have the form
 * void close_func(GtkObject *object,gpointer user_data);
 *
 * Return value: #GtkWidget* pointer to created window
 */
GtkWidget *
window_full2(const gchar * title, GtkWindowPosition position, gint borderwidth, GCallback close_func,
			 gpointer close_data, gboolean delete_on_escape, GtkWidget * transientforparent)
{

	GtkWidget *returnwidget;

	returnwidget = window_with_title(title, position, borderwidth);
/*	g_signal_connect(G_OBJECT(returnwidget), "delete_event", close_func, close_data);*/
	/* use "destroy" and not "destroy_event", 'cause that doesn't work */
	if (close_func) {
		if (close_data == NULL)
			close_data = returnwidget;
		g_signal_connect(G_OBJECT(returnwidget), "destroy", close_func, close_data);
		DEBUG_MSG("window_full, close_data=%p\n", close_data);
	}
	if (transientforparent) {
		gtk_window_set_transient_for(GTK_WINDOW(returnwidget), GTK_WINDOW(transientforparent));
	}
	if (delete_on_escape) {
		window_delete_on_escape(GTK_WINDOW(returnwidget));
		g_signal_connect(G_OBJECT(returnwidget), "key_press_event",
						 G_CALLBACK(window_full_key_press_event_lcb), returnwidget);
		/* for these windows it is also convenient if they destroy when their parent is destroyed */
		gtk_window_set_destroy_with_parent(GTK_WINDOW(returnwidget), TRUE);
	}
	DEBUG_MSG("window_full, return %p\n", returnwidget);
	return returnwidget;
}

/**
 * textview_buffer_in_scrolwin:
 * 	@textview: #GtkWidget** will be filled with the textview widget pointer
 * 	@width: #gint the width of the scrolwin, -1 if default required
 * 	@height: #gint the height of the scrolwin, -1 if default required
 * 	@contents: #gchar* the initial contents of the textbox
 * 	@wrapmode: #GtkWrapMode the wrapmode for the TextView GTK_WRAP_NONE,GTK_WRAP_CHAR,GTK_WRAP_WORD
 *
 * creates a textbuffer, a textview widget (stored in the textview pointer),
 * puts that inside a scrolwin, adds the content and sets the size and returns
 * the scrolwin
 *
 * Return value: #GtkWidget* to the scolwin
 */
GtkWidget *
textview_buffer_in_scrolwin(GtkWidget ** textview, gint width, gint height, const gchar * contents,
							GtkWrapMode wrapmode)
{
	GtkWidget *scrolwin;
	GtkTextBuffer *textbuf;

	textbuf = gtk_text_buffer_new(NULL);
	*textview = gtk_text_view_new_with_buffer(textbuf);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(*textview), wrapmode);
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolwin), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scrolwin), *textview);
	gtk_widget_set_size_request(scrolwin, width, height);
	if (contents) {
		gtk_text_buffer_set_text(textbuf, contents, -1);
	}
	return scrolwin;
}

gchar *
textbuffer_get_all_chars(GtkTextBuffer * buffer)
{
	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	return gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
}

/**
 * apply_font_style:
 * 	@this_widget: #GtkWidget* the widget to apply the new fontstyle
 * 	@fontstring: #gchar* the fontset which will be loaded
 *
 * 	Change the font of a widget with a certain font from a string
 *
 * Return value: #GtkWidget* to this_widget
 */
GtkWidget *
apply_font_style(GtkWidget * this_widget, gchar * fontstring)
{
	if (fontstring && fontstring[0] != '\0') {
		PangoFontDescription *font_desc;
		font_desc = pango_font_description_from_string(fontstring);
		DEBUG_MSG("apply_font_style, set font %s (desc=%p) to widget %p\n",fontstring,font_desc,this_widget);
#if GTK_CHECK_VERSION(3,0,0)
		gtk_widget_override_font(this_widget, font_desc);
#else
		gtk_widget_modify_font(this_widget, font_desc);
#endif
		pango_font_description_free(font_desc);
	}
	return this_widget;
}


/**
 * bf_generic_frame_new:
 * @label: #const gchar* label string. Set to NULL for no label
 * @shadowtype: #GtkShadowType  border type
 * @borderwidth: #gint outside border width
 *
 * create a generic frame with shadow type and border
 *
 * Return value: #GtkWidget* pointer to created frame
 */
GtkWidget *
bf_generic_frame_new(const gchar * label, GtkShadowType shadowtype, gint borderwidth)
{
	GtkWidget *frame;

	frame = gtk_frame_new(label);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), shadowtype);
	gtk_container_set_border_width(GTK_CONTAINER(frame), borderwidth);

	return frame;
}

/**
 * bf_label_with_markup:
 * @labeltext: #const gchar* labeltext string
 */
GtkWidget *
bf_label_with_markup(const gchar * labeltext)
{
	GtkWidget *label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), labeltext);
	return label;
}

/**
 * bf_label_tad_with_markup:
 * @labeltext: #const gchar* label string
 * @xalign: #gfloat label horizontal alignment
 * @yalign: #gfloat label vertical alignment
 * @table: #GtkWidget* table label is packed into
 * @left_attach: #guint column number to attach the left side of the label to
 * @right_atach: #guint column number to attach the right side of a label to
 * @top_attach: #guint row number to attach the top of a label to
 * @bottom_attach: #guint row number to attach the bottom of a label to
 *
 *	create a label with markup, align it, and attach it to a table
 * using the table defaults
 *
 * Return value: void
 */
void
bf_label_tad_with_markup(const gchar * labeltext, gfloat xalign, gfloat yalign,
						 GtkWidget * table, guint left_attach, guint right_attach,
						 guint top_attach, guint bottom_attach)
{
	GtkWidget *label;
	label = bf_label_with_markup(labeltext);
	gtk_misc_set_alignment(GTK_MISC(label), xalign, yalign);
	gtk_table_attach_defaults(GTK_TABLE(table), label, left_attach, right_attach, top_attach, bottom_attach);
}

#ifdef NOTYETUSED
typedef struct {
	GtkWidget *win;
	GtkWidget *entry[10];
	gpointer data;
} Tmultientrywidget;

static void
multi_entry_destroy(GtkObject * object, Tmultientrywidget * mew)
{
	g_free(mew);
}

static void
multi_entry_cancel_clicked(GtkWidget * widget, Tmultientrywidget * mew)
{
	gtk_widget_destroy(mew->win);
}

Tmultientrywidget *
build_multi_entry_window(gchar * title, GCallback ok_func, gpointer data, const gchar ** labelarr)
{
	gint arrlen, i;
	GtkWidget *table, *hbox, *but;

	arrlen = g_strv_length(labelarr);
	if (arrlen > 10)
		arrlen = 10;
	Tmultientrywidget *mew = g_new0(Tmultientrywidget, 1);
	mew->data = data;
	mew->win = window_full2(title, GTK_WIN_POS_MOUSE, 10, multi_entry_destroy, mew, TRUE, NULL);
	table = gtk_table_new(arrlen + 2, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	gtk_container_add(GTK_CONTAINER(mew->win), table);
	for (i = 0; i < arrlen; i++) {
		GtkWidget *label = gtk_label_new(labelarr[i]);
		gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, i, i + 1);
		mew->entry[i] = entry_with_text(NULL, 255);
		gtk_table_attach_defaults(GTK_TABLE(table), mew->entry[i], 1, 2, i, i + 1);
	}
	gtk_table_attach_defaults(GTK_TABLE(table), gtk_hseparator_new(), 0, 2, i, i + 1);
	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 12);
	but = bf_stock_cancel_button(G_CALLBACK(multi_entry_cancel_clicked), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 0);
	but = bf_stock_ok_button(G_CALLBACK(ok_func), mew);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 0, 2, i + 1, i + 2);
	gtk_widget_show_all(mew->win);
}
#endif

#ifdef NEED_HIG_DIALOG
/**************************************************************************/
/***********************  BUTTON DIALOG FUNCTIONS  ************************/
/**************************************************************************/
/*    If this code doesn't work, blame Christian <chris@tellefsen.net>    */

/* This function is called from single_button_dialog_backend() and multiple_button_dialog_backend()
 * The dialog passed is made HIG-compliant.
 *
 * A HiG-dialog is consists of a GtkDialog, with a 2-cell vbox. The top
 * vbox contains an hbox, with an image and a label. The label has a primary
 * (strong && larger) message and an additional, more descriptive message.
 * It is not resizeable, has no title, placement is selected by the wm.
 */
static void
hig_dialog_backend(GtkDialog * dialog, gchar * primary, gchar * secondary, gchar * icon)
{
	GtkWidget *vbox, *hbox, *image, *label;
	gchar *message;
	gchar *spanstart = "<span weight=\"bold\" size=\"larger\">";
	gchar *spanend = "</span>\n\n";
	gchar *msgend = "\n";

	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	hbox = gtk_hbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 12);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, FALSE, 0);

	if (icon) {
		image = gtk_image_new_from_stock(icon, GTK_ICON_SIZE_DIALOG);	/* icon unknown ==> "broken image" displayed. */
		gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
		gtk_misc_set_alignment(GTK_MISC(image), 0, 0);
	}

	if (secondary) {			/* Creates label-content. */
		gchar *str1, *str2;
		str1 = g_markup_escape_text(primary, -1);
		str2 = g_markup_escape_text(secondary, -1);
		message = g_strconcat(spanstart, str1, spanend, str2, msgend, NULL);
		g_free(str1);
		g_free(str2);
	} else {
		gchar *str1;
		str1 = g_markup_escape_text(primary, -1);
		message = g_strconcat(spanstart, str1, spanend, NULL);
		g_free(str1);
	}

	label = gtk_label_new(message);
	g_free(message);

	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
}
#endif							/*NEED_HIG_DIALOG */
#ifdef NEED_PROGRESSBAR
/************************************************************************/
/*********************** PROGRESS-BAR FUNCTIONS *************************/
/************************************************************************/
/* Progress-bar-inspiration from the gtk-tutorial.. */

typedef struct _Tprogress {
	GtkWidget *bar;
	gboolean active;			/* if FALSE, the bar is set to "confused-mode". */
	gboolean show_text;			/* TRUE: String added to bar: text " " value " of  " maxvalue' */
	gchar *text;				/* Optional text to be displayed. */
	guint value;				/* The current value. */
	guint maxvalue;				/* The maximum value. Used to calculate a fraction between 0 and 1. */
	GtkWidget *owner;			/* The widget to be destroyed when the bar is finished. */
	guint timer;				/* keep track of the timer */

} Tprogress;

void
progress_destroy(gpointer gp)
{
	if (gp) {
		Tprogress *p = (Tprogress *) gp;
		/* bar is destroyed when owner is destroyed. */
		if (p->owner)
			gtk_widget_destroy(p->owner);
		else
			gtk_widget_destroy(p->bar);

		gtk_timeout_remove(p->timer);
		p->timer = 0;

		g_free(p);
	}
}

static gboolean
progress_update(gpointer data)
{
	gchar *msg;
	Tprogress *p = (Tprogress *) data;
	gdouble frac = (gdouble) p->value / (gdouble) p->maxvalue;
	DEBUG_MSG("Entering: progress_update()\n");

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(p->bar), frac);
	msg = g_strdup_printf(_("%d of %d"), p->value, p->maxvalue);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(p->bar), msg);
	g_free(msg);

	if (p->value < p->maxvalue) {
/*		flush_queue ();*/
		return TRUE;			/* Yes! Call me again! GIMMI! */
	} else {
		/* We're done! Signal that we're maxed now. */
		return FALSE;
	}
}

/**
 * progress_set:
 * @p The Tprogress struct assigned by a progress-creator. (i.e. progress_popup).
 * @value a #guint between 0 and p->maxvalue.
 *
 * Set a new value for the progressbar-value. Does not actually update the progress-bar.
 **/
void
progress_set(gpointer gp, guint value)
{
	Tprogress *p = (Tprogress *) gp;
	p->value = value;
}

/**
 * progress_popup:
 *
 * Creates a popup containing a progress-bar. The dialog disappears when value = maxvalue.
 * 'text' must be freeed after use!
 *
 * Returns: #gpointer camouflaged pointer to struct Tprogress, a data-structure passed to progress_update.
 **/
gpointer
progress_popup(GtkWidget * win, gchar * title, guint maxvalue)
{
	Tprogress *p;

	p = g_malloc(sizeof(Tprogress));

	p->bar = gtk_progress_bar_new();
	p->value = 0;
	p->maxvalue = maxvalue;


	/* other parameters */
	p->active = TRUE;
	p->show_text = TRUE;
	p->value = 0;
	p->maxvalue = maxvalue;

	p->owner = gtk_dialog_new();

	gtk_window_set_modal(GTK_WINDOW(p->owner), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(p->owner), GTK_WINDOW(win));


	/* Label, if applicable. Append pretty icon! */
	hig_dialog_backend(GTK_DIALOG(p->owner), title, NULL, GTK_STOCK_DIALOG_INFO);

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(p->owner))), p->bar, TRUE, TRUE, 12);

	p->timer = gtk_timeout_add(500, progress_update, p);
	gtk_widget_show_all(p->owner);

	return (gpointer) p;
}
#endif							/* NEED_PROGRESSBAR */

/************************************************************************/
/************************ file_but_* FUNCTIONS **************************/
/************************************************************************/

gchar *
run_file_select_dialog(GtkWindow *parent, const gchar *setfile, const gchar *relativeto, GtkFileChooserAction chooseraction)
{
	GtkWidget *dialog;
	gchar *tmpstring=NULL;
	dialog =
		file_chooser_dialog(NULL, _("Select File"), chooseraction, setfile, FALSE, FALSE, NULL,FALSE);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	if (chooseraction == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER) {
		gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), setfile);
	} else if (setfile) {
		gtk_file_chooser_set_uri(GTK_FILE_CHOOSER(dialog), setfile);
	}
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		if (chooseraction == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER) {
			tmpstring = gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog));
		} else {
			tmpstring = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
		}
	}
	gtk_widget_destroy(dialog);

	if (tmpstring && relativeto) {
		gchar *tmp;
		tmp = create_relative_link_to(relativeto, tmpstring);
		g_free(tmpstring);
		tmpstring = tmp;
	}
	return tmpstring;
}

typedef struct {
	GtkWidget *entry;
	Tbfwin *bfwin;
	gboolean fullpath;
	GtkFileChooserAction chooseraction;
} Tfilebut;

static void
file_but_clicked_lcb(GtkWidget * widget, Tfilebut * fb)
{
	gchar *tmpstring = NULL, *relativeto = NULL, *setfile = NULL;
	DEBUG_MSG("file_but_clicked_lcb, started, which_entry=%p\n", fb->entry);
	setfile = gtk_editable_get_chars(GTK_EDITABLE(GTK_ENTRY(fb->entry)), 0, -1);
	/* if setfile is empty we should probably use the current document basedir ? right? */
	if (!setfile || strlen(setfile) == 0) {
		if (fb->bfwin && fb->bfwin->current_document && fb->bfwin->current_document->uri) {
			if (setfile)
				g_free(setfile);
			setfile = g_file_get_uri(fb->bfwin->current_document->uri);
		} else
			setfile = NULL;
	} else if (setfile && setfile[0] != '/' && strchr(setfile, ':') == NULL && fb->bfwin
			   && fb->bfwin->current_document && fb->bfwin->current_document->uri) {
		/* if setfile is a relative name, we should try to make it a full path. relative names
		   cannot start with a slash or with a scheme (such as file://)
		 */
		GFile *parent = g_file_get_parent(fb->bfwin->current_document->uri);
		GFile *newsetfile = g_file_resolve_relative_path(parent, setfile);
		g_object_unref(parent);
		if (newsetfile) {
			g_free(setfile);
			setfile = g_file_get_uri(newsetfile);
			g_object_unref(newsetfile);
		}
	}

	if (!fb->fullpath && fb->bfwin && fb->bfwin->current_document && fb->bfwin->current_document->uri) {
		relativeto = g_file_get_uri(fb->bfwin->current_document->uri);
	}

	tmpstring = run_file_select_dialog(GTK_WINDOW(gtk_widget_get_toplevel(fb->entry))
							, setfile
							, relativeto,
							fb->chooseraction);
	if (tmpstring) {
		gtk_entry_set_text(GTK_ENTRY(fb->entry), tmpstring);
	}
	g_free(relativeto);
	/*{
		GtkWidget *dialog;
		dialog =
			file_chooser_dialog(NULL, _("Select File"), fb->chooseraction, setfile, FALSE, FALSE, NULL,
								FALSE);
		gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_widget_get_toplevel(fb->entry)));
		if (fb->chooseraction == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER) {
			gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), setfile);
		} else if (setfile) {
			gtk_file_chooser_set_uri(GTK_FILE_CHOOSER(dialog), setfile);
		}
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
			if (fb->chooseraction == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER) {
				tmpstring = gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog));
			} else {
				tmpstring = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
			}
		}
		gtk_widget_destroy(dialog);
	}
	g_free(setfile);
	DEBUG_MSG("file_but_clicked_lcb, return_file returned %s\n", tmpstring);
	if (tmpstring) {
		if (!fb->fullpath && fb->bfwin && fb->bfwin->current_document) {
			if (fb->bfwin->current_document->uri != NULL) {
				gchar *doc_curi;
				/ * the function g_file_get_relative_path cannot create links that don't
				   have the same prefix (relative links with ../../) * /
				doc_curi = g_file_get_uri(fb->bfwin->current_document->uri);
				tmp2string = create_relative_link_to(doc_curi, tmpstring);
				g_free(doc_curi);
				/ * If tmp2string is NULL we need to return the full path * /
				if (tmp2string == NULL)
					tmp2string = g_strdup(tmpstring);
			} else {
				tmp2string = g_strdup(tmpstring);
			}
			g_free(tmpstring);
			tmpstring = tmp2string;
		}
		gtk_entry_set_text(GTK_ENTRY(fb->entry), tmpstring);

	}*/

}

static void
file_but_destroy(GtkWidget * widget, Tfilebut * fb)
{
	g_slice_free(Tfilebut, fb);
}

/**
 * file_but_new:
 * @which_entry: #GtkWidget* GTK_ENTRY where to put the filename
 * @win: #GtkWidget* the GTK_WINDOW where the button is placed
 * @full_pathname: #gint 1 or 0, if you want the full path or a relative path
 * @bfwin: #Tbfwin, only required if you want a relative pathname
 * @chooseraction: #GtkFileChooserAction, usually GTK_FILE_CHOOSER_ACTION_OPEN or GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER
 *
 *	Create new "open file" button, that will put the string into an entry
 *
 * Return value: #GtkWidget* pointer to file button
 */
GtkWidget *
file_but_new2(GtkWidget * which_entry, gint full_pathname, Tbfwin * bfwin, GtkFileChooserAction chooseraction)
{
	GtkWidget *file_but;
	Tfilebut *fb;

	fb = g_slice_new(Tfilebut);
	fb->entry = which_entry;
	fb->bfwin = bfwin;
	fb->fullpath = full_pathname;
	fb->chooseraction = chooseraction;
	file_but = dialog_button_new_with_image(NULL,GTK_STOCK_OPEN, G_CALLBACK(file_but_clicked_lcb), fb, TRUE, FALSE);
	g_signal_connect(G_OBJECT(file_but), "destroy", G_CALLBACK(file_but_destroy), fb);
	DEBUG_MSG("file_but_new, entry=%p, button=%p\n", which_entry, file_but);
	gtk_widget_show(file_but);
	return file_but;
}

/************************************************************************/
/************    FILE SELECTION FUNCTIONS  ******************************/
/************************************************************************/

typedef struct {
	GtkWidget *dialog;
	GtkWidget *show_backup;
	Tfilter *filter;
} Tfchooser_filter;

static Tfchooser_filter *
new_fchooser_filter(GtkWidget * dialog, GtkWidget * show_backup, Tfilter * filter)
{
	Tfchooser_filter *cf = g_new(Tfchooser_filter, 1);
	cf->dialog = dialog;
	cf->show_backup = show_backup;
	cf->filter = filter;
	return cf;
}

static void
refresh_filter_lcb(GtkToggleButton * togglebutton, GtkWidget * dialog)
{
	gchar *uri;
/*	filter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog));
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);*/
	/* this successfully triggers the refiltering! */
	uri = gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog));
	gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), uri);
}

static gboolean
is_separator(GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
	gboolean result = FALSE;
	gchar *text;

	gtk_tree_model_get(model, iter, 0, &text, -1);

	if (g_ascii_strcasecmp(text, "separator") == 0)
		result = TRUE;

	g_free(text);

	return (result);
}

static gboolean
file_chooser_custom_filter_func(GtkFileFilterInfo * filter_info, gpointer data)
{
	/*gint ret;*/
	Tfchooser_filter *cf = data;
	if (!filter_info->display_name || filter_info->display_name[0] == '\0') {
		DEBUG_MSG("file_chooser_custom_filter_func, no display_name, don't display, return FALSE\n");
		return FALSE;			/* error condition ?? */
	}
	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cf->show_backup))) {
		gint namelen = strlen(filter_info->display_name);
		if (filter_info->display_name[namelen - 1] == '~') {
			DEBUG_MSG("file_chooser_custom_filter_func, %s is backup file, return FALSE\n", filter_info->display_name);
			return FALSE;
		}
	}
	/*if (cf->filter == NULL)
		return TRUE;
	if (!cf->filter->filetypes)
		return !cf->filter->mode;*/
	DEBUG_MSG("file_chooser_custom_filter_func, calling file_visible_in_filter for %s with mime=%s and filename=%s\n", filter_info->display_name, filter_info->mime_type, filter_info->filename);
	/* for some reason the GtkFileFilterInfo often has a NULL filename, so we use the display name to filter on */
	return file_visible_in_filter(cf->filter, filter_info->mime_type, filter_info->display_name);
	/*ret = GPOINTER_TO_INT(g_hash_table_lookup(cf->filter->filetypes, filter_info->mime_type));
	return (ret ? cf->filter->mode : !cf->filter->mode);*/
}

GtkWidget *
file_chooser_dialog(Tbfwin * bfwin, const gchar * title, GtkFileChooserAction action,
					const gchar * set, gboolean localonly, gboolean multiple, const gchar * filter,
					gboolean show_encoding)
{
	GtkWidget *vbox, *hbox, *dialog, *viewbackup;
	dialog = gtk_file_chooser_dialog_new(title, bfwin ? GTK_WINDOW(bfwin->main_window) : NULL, action,
										 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
										 (action ==
										  GTK_FILE_CHOOSER_ACTION_SAVE) ? GTK_STOCK_SAVE : GTK_STOCK_OPEN,
										 GTK_RESPONSE_ACCEPT, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), localonly);
	if (set && set[0] != '\0') {
		DEBUG_MSG("file_chooser_dialog, set=%s,localonly=%d\n", set, localonly);
		if (set[0] == '/' || strchr(set, ':')!=NULL) {
			gtk_file_chooser_set_uri(GTK_FILE_CHOOSER(dialog), set);
		} else {
			gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), set);
		}
	} else if (!localonly && bfwin) {	/* localonly is used for the project files */
		if (action == GTK_FILE_CHOOSER_ACTION_SAVE) {
			DEBUG_MSG("file_chooser_dialog, opendir=%s, savedir=%s\n", bfwin->session->opendir,
					  bfwin->session->savedir);
			if (bfwin->session->savedir)
				gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), bfwin->session->savedir);
			else if (bfwin->session->opendir)
				gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), bfwin->session->opendir);
		} else {
			DEBUG_MSG("file_chooser_dialog, opendir=%s, localonly=%d\n", bfwin->session->opendir, localonly);
			if (bfwin->session->opendir)
				gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), bfwin->session->opendir);
		}
	}
#ifdef DEBUG
	DEBUG_MSG("file_chooser_dialog, current_folder_uri=%s\n",
			  gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog)));
#endif

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), multiple);
/*	if (bfwin) {
		GList *tmplist;
		/ *tmplist = g_list_first(bfwin->session->recent_dirs);* /
		tmplist = g_list_first(main_v->recent_directories);
		DEBUG_MSG("file_chooser_dialog, session dir len=%d\n",g_list_length(bfwin->session->recent_dirs));
		while (tmplist) {
			GError *error=NULL;
			gtk_file_chooser_add_shortcut_folder_uri(GTK_FILE_CHOOSER(dialog),(gchar*)tmplist->data,&error);
			DEBUG_MSG("file_chooser_dialog, adding folder %s\n",(gchar*)tmplist->data);
			if (error) {
				DEBUG_MSG("file_chooser_dialog, ERROR adding dir %s: %s\n", (gchar*)tmplist->data, error->message);
				g_error_free(error);
			}
			tmplist = g_list_next(tmplist);
		}
	}*/
	vbox = gtk_vbox_new(FALSE, 5);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 5);
	viewbackup = boxed_checkbut_with_value(_("Show backup files"), 0, hbox);
	if (bfwin && bfwin->session) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewbackup),
									 bfwin->session->filebrowser_show_backup_files);
	}
	g_signal_connect(G_OBJECT(viewbackup), "toggled", G_CALLBACK(refresh_filter_lcb), dialog);

	if (action == GTK_FILE_CHOOSER_ACTION_OPEN || action == GTK_FILE_CHOOSER_ACTION_SAVE) {
		GList *tmplist;
		GtkFileFilter *ff;
		ff = gtk_file_filter_new();
		gtk_file_filter_set_name(ff, _("All files"));
		gtk_file_filter_add_custom(ff, GTK_FILE_FILTER_DISPLAY_NAME | GTK_FILE_FILTER_MIME_TYPE,
								   (GtkFileFilterFunc) file_chooser_custom_filter_func,
								   new_fchooser_filter(dialog, viewbackup, NULL), g_free);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), ff);
		for (tmplist = g_list_first(main_v->filefilters); tmplist != NULL; tmplist = tmplist->next) {
			Tfilter *filter = tmplist->data;
			ff = gtk_file_filter_new();
			gtk_file_filter_set_name(ff, filter->name);
			gtk_file_filter_add_custom(ff, GTK_FILE_FILTER_DISPLAY_NAME | GTK_FILE_FILTER_MIME_TYPE,
									   (GtkFileFilterFunc) file_chooser_custom_filter_func,
									   new_fchooser_filter(dialog, viewbackup, filter), g_free);
			gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), ff);
		}
/*		gtk_file_filter_add_pattern(ff, "*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), ff);
		if (filter == NULL)	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), ff);
		tmplist = g_list_first(main_v->filetypelist);
		while (tmplist) {
			gchar **tmp;
			Tfiletype *ft = (Tfiletype *)tmplist->data;
			if ((strcmp(ft->type, "objectfile") != 0) && ft->extensions && *ft->extensions) {
				ff = gtk_file_filter_new();
				gtk_file_filter_set_name(ff,ft->type);
				DEBUG_MSG("file_chooser_dialog, adding filter '%s'\n", ft->type);
				tmp = ft->extensions;
				while (*tmp) {
					gchar *pattern;
					pattern = g_strconcat("*", *tmp, NULL);
					gtk_file_filter_add_pattern(ff, pattern);
					DEBUG_MSG("file_chooser_dialog, adding pattern '%s' to '%s'\n", pattern, ft->type);
					g_free(pattern);
					tmp++;
				}
				gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), ff);
				if (filter && strcmp(filter, ft->type) == 0)	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), ff);
			}
			tmplist = g_list_next(tmplist);
		}*/
	}
	if (show_encoding) {
		/* make character encoding widget */
		GtkWidget *label, *combo;
		GtkListStore *store;
		GtkTreeIter iter, seliter;
		gboolean have_seliter = FALSE;
		GtkCellRenderer *renderer;
		GList *tmplist;

		hbox = gtk_hbox_new(FALSE, 6);
		label = gtk_label_new_with_mnemonic(_("Character _Encoding:"));
		store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_BOOLEAN);
		for (tmplist = g_list_first(main_v->globses.encodings); tmplist; tmplist = tmplist->next) {
			GStrv arr = (GStrv) tmplist->data;
			if (g_strv_length(arr) == 3 && arr[2][0] == '1') {
				gchar *label = g_strdup_printf("%s (%s)", arr[0], arr[1]);
				gtk_list_store_append(store, &iter);
				gtk_list_store_set(store, &iter, 0, label, 1, arr, -1);
				g_free(label);
				if (have_seliter == FALSE && bfwin && bfwin->session && bfwin->session->encoding &&
					strcmp(arr[1], bfwin->session->encoding) == 0) {
					seliter = iter;
					have_seliter = TRUE;
				}
			}
		}

		gtk_list_store_prepend(store, &iter);
		gtk_list_store_set(store, &iter, 0, "separator", 1, NULL, -1);
		gtk_list_store_prepend(store, &iter);
		gtk_list_store_set(store, &iter, 0, _("Automatic detection"), 1, NULL, -1);

		combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
		g_object_unref(G_OBJECT(store));
		gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(combo), is_separator, NULL, NULL);
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo), have_seliter ? &seliter : &iter);

		renderer = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
		gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", 0, NULL);
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), combo);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
		gtk_box_pack_end(GTK_BOX(hbox), combo, TRUE, TRUE, 0);

		gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 5);
		g_object_set_data(G_OBJECT(dialog), "encodings", combo);
	}
	gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), vbox);
	gtk_widget_show_all(vbox);
	return dialog;
}

/************************************************************************/

static gboolean
accelerator_key_press_lcb(GtkWidget * widget, GdkEventKey * event, gpointer user_data)
{
	GtkDialog *dlg = GTK_DIALOG(user_data);
/*	if (!g_unichar_isalnum((gunichar)event->keyval) && event->keyval!=GDK_Escape && !g_unichar_isspace((gunichar)event->keyval))
		return FALSE;*/
	switch (event->keyval) {
	case GDK_KEY_Escape:
		gtk_dialog_response(dlg, GTK_RESPONSE_CANCEL);
		break;
	case GDK_KEY_Delete:
	case GDK_KEY_KP_Delete:
	case GDK_KEY_BackSpace:
		gtk_dialog_response(dlg, GTK_RESPONSE_REJECT);
		break;
	default:
		{
			guint accel_mods, accel_key;
			GdkDisplay *display;
			GdkModifierType consumed_modifiers;

			display = gtk_widget_get_display(widget);
			gdk_keymap_translate_keyboard_state(gdk_keymap_get_for_display(display), event->hardware_keycode,
												event->state, event->group, NULL, NULL, NULL,
												&consumed_modifiers);
			accel_mods = event->state & gtk_accelerator_get_default_mod_mask() & ~consumed_modifiers;
			accel_key = gdk_keyval_to_lower(event->keyval);
			if (accel_key != event->keyval) {
				accel_mods |= GDK_SHIFT_MASK;
			}
			if (accel_key != 0 && (accel_mods != 0 || (accel_key >= GDK_KEY_F1 && accel_key <= GDK_KEY_F12)) && gtk_accelerator_valid(accel_key, accel_mods)) {
				g_object_set_data(G_OBJECT(dlg), "keyname", gtk_accelerator_name(accel_key, accel_mods));
				gtk_dialog_response(dlg, GTK_RESPONSE_OK);
			} else {
				DEBUG_MSG("accelerator_key_press_lcb, not valid..\n");
			}
		}
		break;
	}
	return FALSE;
}

gchar *
ask_accelerator_dialog(const gchar * title)
{
	GtkWidget *dialog1;
	GtkWidget *label1;
	GtkWidget *label2;
	gint response;
	gchar *retval = NULL;

	dialog1 = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog1), title);
	gtk_window_set_position(GTK_WINDOW(dialog1), GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW(dialog1), TRUE);
	/*gtk_window_set_decorated(GTK_WINDOW(dialog1), FALSE); */
	gtk_window_set_type_hint(GTK_WINDOW(dialog1), GDK_WINDOW_TYPE_HINT_DIALOG);
#if !GTK_CHECK_VERSION(3, 0, 0)
	gtk_dialog_set_has_separator(GTK_DIALOG(dialog1), FALSE);
#endif /* gtk3 */

	label1 = gtk_label_new(_("<b>Keystroke choice</b>"));
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog1))), label1, FALSE, FALSE, 0);
	gtk_label_set_use_markup(GTK_LABEL(label1), TRUE);
	gtk_label_set_justify(GTK_LABEL(label1), GTK_JUSTIFY_CENTER);
	gtk_misc_set_padding(GTK_MISC(label1), 2, 2);

	label2 =
		gtk_label_new(_
					  ("\nPress requested key combination.\nPlease use Ctrl, Shift, Alt key with any other key.\n<i>Esc to cancel, Delete to remove the accelerator.</i>"));
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog1))), label2, FALSE, FALSE, 0);
	gtk_label_set_use_markup(GTK_LABEL(label2), TRUE);
	gtk_label_set_justify(GTK_LABEL(label2), GTK_JUSTIFY_CENTER);
	gtk_misc_set_padding(GTK_MISC(label2), 2, 2);

	gtk_button_box_set_layout(GTK_BUTTON_BOX(gtk_dialog_get_action_area(GTK_DIALOG(dialog1))),
							  GTK_BUTTONBOX_END);
	g_signal_connect(G_OBJECT(dialog1), "key-press-event", G_CALLBACK(accelerator_key_press_lcb), dialog1);

	gtk_widget_show_all(dialog1);
	response = gtk_dialog_run(GTK_DIALOG(dialog1));
	if (response == GTK_RESPONSE_OK) {
		retval = g_object_get_data(G_OBJECT(dialog1), "keyname");
	} else if (response == GTK_RESPONSE_REJECT) {
		retval = g_strdup("");
	}
	gtk_widget_destroy(dialog1);
	return retval;
}

static void
accelerator_button_clicked_lcb(GtkWidget * widget, gpointer data)
{
	gchar *tmpstr;
	tmpstr = ask_accelerator_dialog(_("Press shortcut key combination"));
	if (tmpstr && tmpstr[0] != '\0') {
		gtk_button_set_label(GTK_BUTTON(widget), tmpstr);
	}
	g_free(tmpstr);
}

GtkWidget *
accelerator_button(const gchar * accel)
{
	GtkWidget *retval;

	retval = gtk_button_new_with_label(accel);
	g_signal_connect(G_OBJECT(retval), "clicked", G_CALLBACK(accelerator_button_clicked_lcb), retval);
	return retval;
}

GtkWidget *
boxed_accelerator_button(const gchar * labeltext, const gchar * accel, GtkWidget * box)
{
	GtkWidget *returnwidget;
	returnwidget = accelerator_button(accel);
	boxed_widget(labeltext, returnwidget, box);
	return returnwidget;
}

gchar *
gdk_color_to_hexstring(GdkColor * color, gboolean websafe)
{
	gchar *tmpstr;
	tmpstr = g_malloc(8 * sizeof(char));
	if (websafe) {
		g_snprintf(tmpstr, 8, "#%.2X%.2X%.2X", (0x33 * color->red / (256 * 0x33)),
				   (0x33 * color->green / (256 * 0x33)), (0x33 * color->blue / (256 * 0x33)));
	} else {
		g_snprintf(tmpstr, 8, "#%.2X%.2X%.2X", color->red / 256, color->green / 256, color->blue / 256);
	}
	return tmpstr;
}

static void
color_entry_changed_lcb(GtkWidget * widget, gpointer data)
{
	GdkColor gcolor;
	const gchar *string;
	string = gtk_entry_get_text(GTK_ENTRY(widget));
	if (gdk_color_parse(string, &gcolor)) {
		gtk_color_button_set_color(GTK_COLOR_BUTTON(data), &gcolor);
		gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(data), FALSE);
	} else {
		gtk_color_button_set_alpha(GTK_COLOR_BUTTON(data), 0);
		gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(data), TRUE);
	}
}

static void
color_set_lcb(GtkWidget * widget, gpointer data)
{
	GdkColor gcolor;
	gchar *string;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(widget), &gcolor);
	string = gdk_color_to_hexstring(&gcolor, FALSE);
	gtk_entry_set_text(GTK_ENTRY(data), string);
	g_free(string);
}

GtkWidget *
color_but_new2(GtkWidget * entry)
{
	GtkWidget *cbut;

	cbut = gtk_color_button_new();
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(color_entry_changed_lcb), cbut);
	g_signal_connect(G_OBJECT(cbut), "color-set", G_CALLBACK(color_set_lcb), entry);
	color_entry_changed_lcb(entry, cbut);
	return cbut;
}

