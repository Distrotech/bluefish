/* Bluefish HTML Editor
 * html_diag.h - general functions to create HTML dialogs
 *
 * Copyright (C) 2000-2003 Olivier Sessink
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

#ifndef __HTML_DIAG_H_
#define __HTML_DIAG_H_

/* these will slowly be replaced by vars in bfwin->session */
typedef struct {
/*        GList *classlist;
        GList *colorlist;
        GList *targetlist;
        GList *urllist;
        GList *fontlist;*/
        GList *dtd_cblist;
        GList *headerlist;
        GList *positionlist;
} Trecent_attribs;
extern Trecent_attribs recent_attribs;

typedef struct {
	GtkWidget *name;
	GtkWidget *val;
	GtkWidget *dest;
	gint type;
} Tphpvarins; /* this struct contains a src and dest widget for PHP variable generation*/

typedef struct {
	gchar *item;
	gchar *value;
} Ttagitem;

typedef struct {
	GList *taglist;
	gint pos;
	gint end;
} Ttagpopup;

typedef struct {
	gint pos;
	gint end;
} Treplacerange;

/* the frame wizard uses dynamic widgets, this value should be lower 
then (number of combo's)/2 or lower then (number of clist's)
else the dialog will segfault */
#define MAX_FRAMES_IN_FRAMEWIZARD 5
#include "bluefish.h"
typedef struct {
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *obut;
	GtkWidget *cbut;
	GtkWidget *entry[20];
	GtkWidget *combo[11];
	GtkWidget *radio[14];
	GtkWidget *spin[8];
	GtkWidget *check[8];
	GtkWidget *clist[4];
	GtkWidget *text[1];
	GtkWidget *attrwidget[20]; /* all attribute widgets should go in here */
	Treplacerange range;
	GtkTextMark *mark_ins;
	GtkTextMark *mark_sel;
	Tphpvarins php_var_ins;
	GtkWidget *phpbutton;
	gboolean tobedestroyed; /* this will be set to TRUE on destroy */
	Tdocument *doc;
	Tbfwin *bfwin;
} Thtml_diag;

void html_diag_destroy_cb(GtkWidget * widget, Thtml_diag *dg);

void html_diag_cancel_clicked_cb(GtkWidget *widget, gpointer data);

Thtml_diag *html_diag_new(Tbfwin *bfwin,gchar *title);
GtkWidget *html_diag_table_in_vbox(Thtml_diag *dg, gint rows, gint cols);
void html_diag_finish(Thtml_diag *dg, GtkSignalFunc ok_func);
void parse_html_for_dialogvalues(gchar *dialogitems[], gchar *dialogvalues[]
		, gchar **custom, Ttagpopup *data);
void fill_dialogvalues(gchar *dialogitems[], gchar *dialogvalues[]
	, gchar **custom, Ttagpopup *data, Thtml_diag *diag);

void parse_existence_for_dialog(gchar * valuestring, GtkWidget * checkbox);
void parse_integer_for_dialog(gchar * valuestring, GtkWidget * spin, GtkWidget * entry, GtkWidget * checkbox);

gchar *insert_string_if_entry(GtkWidget * entry, gchar * itemname, gchar * string2add2, gchar *defaultvalue);
gchar *insert_integer_if_spin(GtkWidget * spin, gchar * itemname, gchar * string2add2, gboolean ispercentage, gint dontinsertonvalue);
gchar *insert_attr_if_checkbox(GtkWidget * checkbox, gchar * itemname, gchar *string2add2);
gchar *format_entry_into_string(GtkEntry * entry, gchar * formatstring);
GList *add_entry_to_stringlist(GList *which_list, GtkWidget *entry);

GtkWidget *generic_table_inside_notebookframe(GtkWidget *notebook, const gchar *title, gint rows, gint cols);
void generic_class_id_style_section(Thtml_diag *dg, gint firstattrwidget, GtkWidget *table, gint firstrowintable, gchar **tagvalues, gint firsttagvalue);

#endif /* __HTML_DIAG_H_ */
