/* Bluefish HTML Editor
 * html2.c - menu/toolbar callbacks, inserting functions, and other cool stuff 
 * otherwise html.c is getting so long ;-)
 *
 * Copyright (C) 1999 Olivier Sessink
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
#include <string.h>  	/* strlen() */
#include <stdlib.h>	/* strtol() */

#include "bluefish.h"
#include "bf_lib.h" /* string_is_color*/
#include "gtk_easy.h"
#include "bf_lib.h"  /* strip_any_whitespace() */
#include "stringlist.h"
#include "document.h"
#include "html2.h"
#include "cap.h"
#include "pixmap.h" /* new_pixmap */

static GList *glist_with_html_tags(gint with_pseudo_classes) {
	GList *tmplist;

	tmplist = g_list_append(NULL, "");
	if (with_pseudo_classes) {
		tmplist = g_list_append(tmplist, "a:link");
		tmplist = g_list_append(tmplist, "a:visited");
		tmplist = g_list_append(tmplist, "a:active");
	} else {
		tmplist = g_list_append(tmplist, "a");
	}
	tmplist = g_list_append(tmplist, "abbr");
	tmplist = g_list_append(tmplist, "acronym");
	tmplist = g_list_append(tmplist, "address");
	tmplist = g_list_append(tmplist, "applet");
	tmplist = g_list_append(tmplist, "area");
	tmplist = g_list_append(tmplist, "b");
	tmplist = g_list_append(tmplist, "bdo");
	tmplist = g_list_append(tmplist, "blockquote");
	tmplist = g_list_append(tmplist, "body");
	tmplist = g_list_append(tmplist, "br");
	tmplist = g_list_append(tmplist, "button");
	tmplist = g_list_append(tmplist, "caption");
	tmplist = g_list_append(tmplist, "cite");
	tmplist = g_list_append(tmplist, "code");
	tmplist = g_list_append(tmplist, "col");
	tmplist = g_list_append(tmplist, "colgroup");
	tmplist = g_list_append(tmplist, "dd");
	tmplist = g_list_append(tmplist, "del");
	tmplist = g_list_append(tmplist, "dfn");
	tmplist = g_list_append(tmplist, "div");
	tmplist = g_list_append(tmplist, "dl");
	tmplist = g_list_append(tmplist, "dt");
	tmplist = g_list_append(tmplist, "em");
	tmplist = g_list_append(tmplist, "embed");
	tmplist = g_list_append(tmplist, "fieldset");
	tmplist = g_list_append(tmplist, "form");
	tmplist = g_list_append(tmplist, "h1");
	tmplist = g_list_append(tmplist, "h2");
	tmplist = g_list_append(tmplist, "h3");
	tmplist = g_list_append(tmplist, "h4");
	tmplist = g_list_append(tmplist, "h5");
	tmplist = g_list_append(tmplist, "h6");
	tmplist = g_list_append(tmplist, "hr");
	tmplist = g_list_append(tmplist, "i");
	tmplist = g_list_append(tmplist, "iframe");
	tmplist = g_list_append(tmplist, "img");
	tmplist = g_list_append(tmplist, "input");
	tmplist = g_list_append(tmplist, "ins");
	tmplist = g_list_append(tmplist, "kbd");
	tmplist = g_list_append(tmplist, "label");
	tmplist = g_list_append(tmplist, "legend");
	tmplist = g_list_append(tmplist, "li");
	tmplist = g_list_append(tmplist, "map");
	tmplist = g_list_append(tmplist, "noframes");
	tmplist = g_list_append(tmplist, "noscript");
	tmplist = g_list_append(tmplist, "object");
	tmplist = g_list_append(tmplist, "ol");
	tmplist = g_list_append(tmplist, "optgroup");
	tmplist = g_list_append(tmplist, "option");
	tmplist = g_list_append(tmplist, "p");
	tmplist = g_list_append(tmplist, "pre");
	tmplist = g_list_append(tmplist, "q");
	if(main_v->props.allow_ruby){
		tmplist = g_list_append(tmplist, "rb");
		tmplist = g_list_append(tmplist, "rp");
		tmplist = g_list_append(tmplist, "rt");
		tmplist = g_list_append(tmplist, "ruby");
	}
	tmplist = g_list_append(tmplist, "samp");
	tmplist = g_list_append(tmplist, "select");
	tmplist = g_list_append(tmplist, "small");
	tmplist = g_list_append(tmplist, "span");
	tmplist = g_list_append(tmplist, "strong");
	tmplist = g_list_append(tmplist, "table");
	tmplist = g_list_append(tmplist, "tbody");
	tmplist = g_list_append(tmplist, "td");
	tmplist = g_list_append(tmplist, "textarea");
	tmplist = g_list_append(tmplist, "tfoot");
	tmplist = g_list_append(tmplist, "th");
	tmplist = g_list_append(tmplist, "thead");
	tmplist = g_list_append(tmplist, "tr");
	tmplist = g_list_append(tmplist, "tt");
	tmplist = g_list_append(tmplist, "ul");
	tmplist = g_list_append(tmplist, "var");

	return tmplist;
}

typedef enum {entry, textbox, wizard} Tdest_type;
typedef enum {onestyle, multistyle} Tcs3_style;
typedef enum {but_none, but_file, but_style, but_color} Textra_button;
typedef struct {
	Tdest_type dest_type;
	GtkWidget *entry;
	Tdocument *doc;
	gint doc_start;
	gint doc_end;
} Tcs3_destination;

typedef struct {
	GtkWidget *win;
	Tcs3_destination dest;
	Tcs3_style styletype;
	GtkWidget *clist;
	gint selected_row;
	gboolean grab;
	GtkWidget *selector;
	GtkWidget *property;
	GtkWidget *value;
	GtkWidget *rule_add_but;
	GtkWidget *rule_update_but;
	GtkWidget *extra_but;
} Tcs3_diag;

typedef struct {
	GtkWidget *win;
	GtkWidget *selector;
	GtkWidget *property;
	GtkWidget *value;
	GtkWidget *extra_but;
	gint whichrow;
	Tcs3_diag *diag;
} Tcs3_pd_diag;

typedef struct {
	gchar *property; /* the name of the property */
	gchar **possibilities; /* a list of possibilities */
	gint force_pos; 	/* force the possibility to be one of the options in possibilities */
	Textra_button buttype;
} Tcs3_arr;

static gchar *cs3_colors[] = {"white", "black", "blue", "red", "yellow", "green", "brown", "purple", NULL};
static gchar *cs3_repeat[] = {"repeat", "repeat-x", "repeat-y", "no-repeat", NULL};
static gchar *cs3_fonts[] = {"arial, helvetica, sans-serif", "roman, 'times new roman', times, serif", "courier, fixed, monospace", "western, fantasy",  "Zapf-Chancery, cursive", "serif", "sans-serif", "cursive", "fantasy", "monospace", NULL};
static gchar *cs3_font_size_adjustments[] = {"none", NULL};
static gchar *cs3_font_stretches[] = {"normal","wider","narrower","ultra-condensed","extra-condensed","condensed","semi-condensed","semi-expanded","expanded","extra-expanded","ultra-expanded", NULL};
static gchar *cs3_font_styles[] = {"normal", "italic", "oblique", NULL};
static gchar *cs3_font_variants[] = {"normal", "small-caps", NULL};
static gchar *cs3_font_weights[] = {"normal", "bold", "bolder", "lighter", "100", "200", "300", "400", "500", "600", "700", "800", "900",  NULL};
static gchar *cs3_background_attachments[] = {"scroll", "fixed", NULL};
static gchar *cs3_background_positions[] = {"top", "center", "bottom", "left", "center", "right", "10% 10%", NULL}; /* more 2 add */
static gchar *cs3_text_decorations[] = {"none", "underline", "overline", "line-through", "blink", NULL};
static gchar *cs3_text_transforms[] = {"none", "capitalize", "uppercase", "lowercase", NULL};
static gchar *cs3_text_aligns[] = {"left", "right", "center", "justify", NULL};
static gchar *cs3_border_styles[] = {"none", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset", NULL};
static gchar *cs3_floats[] = {"left", "right", "none", NULL};
static gchar *cs3_positions[] = {"static", "relative", "absolute", "fixed", NULL};
static gchar *cs3_clears[] = {"left", "right", "none", "both", NULL};
static gchar *cs3_displays[] = {"block", "inline", "list-item", "run-in", "compact", "marker", "table", "inline-table", "table-row-group", "table-header-group", "table-footer-group", "table-row", "table-column-group", "table-column", "table-cell", "table-caption", "none",NULL};
static gchar *cs3_whitespaces[] = {"normal", "pre", "nowrap", NULL};
static gchar *cs3_list_style_types[] = {"disc", "circle", "square", "decimal", "lower-roman", "upper-roman", "lower-alpha", "upper-alpha", "none", NULL};
static gchar *cs3_list_style_positions[] = {"inside", "outside", NULL};
static gchar *cs3_zindices[] = {"auto", NULL};
static gchar *cs3_directions[] = {"ltr", "rtl", NULL};
static gchar *cs3_unicodebidis[] = {"normal", "embed", "bidi-override", NULL};
static gchar *cs3_vertical_aligns[] = {"baseline", "sub", "super", "top", "text-top", "middle", "bottom", "text-bottom", NULL};
static gchar *cs3_overflows[] = {"visible", "hidden", "scroll", "auto", NULL};
static gchar *cs3_visibilities[] = {"visible", "hidden", "collapse", NULL};
static gchar *cs3_none[] = {"none", NULL};
static gchar *cs3_caption_sides[] = {"top", "bottom", "left", "right", NULL};
static gchar *cs3_table_layouts[] = {"auto", "fixed", NULL};
static gchar *cs3_border_collapses[] = {"collapse", "separate", NULL};
static gchar *cs3_empty_cells[] = {"show", "hide", NULL};

static Tcs3_arr cs3_arr[] = {
{"font-family", cs3_fonts, 0, but_none},
{"font-stretch", cs3_font_stretches, 0, but_none},
{"font-style", cs3_font_styles, 1, but_none},
{"font-variant", cs3_font_variants, 1, but_none},
{"font-weight", cs3_font_weights, 1, but_none},
{"font-size", NULL, 0, but_none},
{"font-size-adjust", cs3_font_size_adjustments, 0, but_none},
{"font", NULL, 0, but_none},
{"color", cs3_colors, 0, but_color},
{"background-color", cs3_colors, 0, but_color},
{"background-image", NULL, 0, but_file},
{"background-repeat", cs3_repeat, 0, but_none},
{"background-attachment",cs3_background_attachments , 1, but_none},
{"background-position",cs3_background_positions , 0, but_none},
{"background", NULL, 0, but_none},
{"word-spacing", NULL, 0, but_none},
{"letter-spacing", NULL, 0, but_none},
{"text-decoration", cs3_text_decorations, 1, but_none},
{"text-transform", cs3_text_transforms, 1, but_none},
{"text-align", cs3_text_aligns, 1, but_none},
{"text-indent", NULL, 0, but_none},
{"text-shadow", cs3_none, 0, but_none},
{"line-height", NULL, 0, but_none},
{"margin-top", NULL, 0, but_none},
{"margin-right", NULL, 0, but_none},
{"margin-bottom", NULL, 0, but_none},
{"margin-left", NULL, 0, but_none},
{"margin", NULL, 0, but_none},
{"padding-top", NULL, 0, but_none},
{"padding-right", NULL, 0, but_none},
{"padding-bottom", NULL, 0, but_none},
{"padding-left", NULL, 0, but_none},
{"padding", NULL, 0, but_none},
{"border-top-width", NULL, 0, but_none},
{"border-right-width", NULL, 0, but_none},
{"border-bottom-width", NULL, 0, but_none},
{"border-left-width", NULL, 0, but_none},
{"border-width", NULL, 0, but_none},
{"border-top-color", cs3_colors, 0, but_color},
{"border-right-color", cs3_colors, 0, but_color},
{"border-bottom-color", cs3_colors, 0, but_color},
{"border-left-color", cs3_colors, 0, but_color},
{"border-color", cs3_colors, 0, but_color},
{"border-top-style", cs3_border_styles, 1, but_none},
{"border-right-style", cs3_border_styles, 1, but_none},
{"border-bottom-style", cs3_border_styles, 1, but_none},
{"border-left-style", cs3_border_styles, 1, but_none},
{"border-style", cs3_border_styles, 1, but_none},
{"border-top", NULL, 0, but_none},
{"border-right", NULL, 0, but_none},
{"border-bottom", NULL, 0, but_none},
{"border-left", NULL, 0, but_none},
{"border", NULL, 0, but_none},
{"width", NULL, 0, but_none},
{"height", NULL, 0, but_none},
{"float", cs3_floats, 1, but_none},
{"position", cs3_positions, 1, but_none},
{"top", NULL, 0, but_none},
{"right", NULL, 0, but_none},
{"bottom", NULL, 0, but_none},
{"left", NULL, 0, but_none},
{"clear", cs3_clears, 1, but_none},
{"display", cs3_displays, 1, but_none},
{"direction", cs3_directions, 1, but_none},
{"unicode-bidi", cs3_unicodebidis, 1, but_none},
{"z-index", cs3_zindices, 0, but_none},
{"min-width", NULL, 0, but_none},
{"max-width", NULL, 0, but_none},
{"min-height", NULL, 0, but_none},
{"max-height", NULL, 0, but_none},
{"line-height", NULL, 0, but_none},
{"white-space", cs3_whitespaces, 1, but_none},
{"list-style", NULL, 0, but_none},
{"list-style-type", cs3_list_style_types, 1, but_none},
{"list-style-image", NULL, 0, but_none},
{"list-style-position", cs3_list_style_positions, 1, but_none},
{"vertical-align", cs3_vertical_aligns, 0, but_none},
{"overflow", cs3_overflows, 0, but_none},
{"clip", cs3_zindices, 0, but_none},
{"visibility", cs3_visibilities, 0, but_none},
{"caption-side", cs3_caption_sides, 1, but_none},
{"table-layout", cs3_table_layouts, 1, but_none},
{"border-collapse", cs3_border_collapses, 1, but_none},
{"border-spacing", NULL, 0, but_none},
{"empty-cells", cs3_empty_cells, 1, but_none},
{NULL, NULL, 0, but_none}
};


static Tcs3_arr *cs3_arr_from_property(gchar *prop) {
	Tcs3_arr tmp;
	gint count=0;

	if (!prop) {
		return NULL;
	}
	tmp = cs3_arr[count];
	while (tmp.property) {
		if (strcmp(tmp.property, prop) ==0) {
			return &cs3_arr[count];
		}
		count++;
		tmp = cs3_arr[count];
	}
	return NULL;
}

/* the list should be freed, the contents of the list should NOT */
static GList *pointer_arr2glist(gchar **arr) {
	gchar **tmp;
	GList *retlist = NULL;

	if (!arr) {
		return NULL;
	}
	tmp = arr;
	while (*tmp) {
		retlist = g_list_append(retlist, *tmp);
		tmp++;
	}
	return retlist;
}

static void cs3d_destroy_lcb(GtkWidget * widget, Tcs3_diag *diag) {
	window_destroy(diag->win);
	g_free(diag);
}

static void cs3d_cancel_clicked_lcb(GtkWidget * widget, Tcs3_diag *diag) {
	cs3d_destroy_lcb(NULL, diag);
}

static void cs3d_ok_clicked_lcb(GtkWidget * widget, Tcs3_diag *diag) {

	Tcs3_destination dest = diag->dest;
	gchar *stylebuf=NULL;

	if (diag->styletype == onestyle) {
		gchar *tmpbuf1 = NULL;
		gchar *tmpbuf2 = g_strdup("");
		gint retval=1, row=0;
		while (retval) {
			gchar *text[4];
			retval = gtk_clist_get_text(GTK_CLIST(diag->clist), row, 0, &text[0]);
			retval = gtk_clist_get_text(GTK_CLIST(diag->clist), row, 1, &text[1]);
			if (retval) {
				tmpbuf1 = g_strconcat(tmpbuf2, text[0], ": ", text[1], "; ", NULL);
				g_free(tmpbuf2);
				tmpbuf2 = tmpbuf1;
			}
			row++;
		}
		stylebuf = tmpbuf2;
	} else { /* multistyle */
		gchar *tmpbuf1 = NULL;
		gchar *tmpbuf2 = g_strdup("");
		gchar *prev_selector = NULL;
		gint retval=1, row=0;
		while (retval) {
			gchar *text[4];
			retval = gtk_clist_get_text(GTK_CLIST(diag->clist), row, 0, &text[0]);
			retval = gtk_clist_get_text(GTK_CLIST(diag->clist), row, 1, &text[1]);
			retval = gtk_clist_get_text(GTK_CLIST(diag->clist), row, 2, &text[2]);
			if (retval) {
				if (!prev_selector) {
					prev_selector = g_strdup(text[0]);
					tmpbuf1 = g_strconcat(tmpbuf2, text[0], " {\n", NULL);
					g_free(tmpbuf2);
					tmpbuf2 = tmpbuf1;
				} 
				if (strcmp(prev_selector, text[0]) == 0 ) {
					tmpbuf1 = g_strconcat(tmpbuf2, "\t", text[1], ": ", text[2], ";\n", NULL);
				} else {
					tmpbuf1 = g_strconcat(tmpbuf2, "}\n", text[0], " {\n\t", text[1], ": ", text[2], ";\n", NULL);
					g_free(prev_selector);
					prev_selector = g_strdup(text[0]);
				}
				g_free(tmpbuf2);
				tmpbuf2 = tmpbuf1;
				row++;
			}
		}
		g_free(prev_selector);
		stylebuf = g_strconcat(tmpbuf2, "}\n", NULL);
		g_free(tmpbuf2);
	}

	if (stylebuf) {
		if (dest.dest_type == entry) {
			DEBUG_MSG("cs3d_ok_clicked_lcb, entry? %p\n", dest.entry);
			if (dest.entry && GTK_IS_WIDGET(dest.entry)) {
				DEBUG_MSG("cs3d_ok_clicked_lcb, entry!\n");
				gtk_entry_set_text(GTK_ENTRY(dest.entry), stylebuf);
			}
		} else if (dest.dest_type == textbox) {
			if (dest.doc) {
				DEBUG_MSG("cs3d_ok_clicked_lcb, textbox!\n");
				if (dest.doc_start != -1 || dest.doc_end != -1) {
					doc_replace_text(dest.doc, stylebuf, dest.doc_start, dest.doc_end);
				} else {
					doc_insert_two_strings(dest.doc, stylebuf, NULL);
				}
			}
		} else if (dest.dest_type == wizard) {
			GtkTextBuffer *buf;
			buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(dest.entry));
			gtk_text_buffer_set_text(buf, stylebuf, -1);
		} else {
#ifdef DEVELOPMENT
			g_print("cs3d_ok_clicked_lcb, an unknown dest type, this should never happen!\n");
#endif
		}
		g_free(stylebuf);
	}
	cs3d_destroy_lcb(NULL, diag);
}

static void cs3d_select_row_lcb(GtkWidget *clist, gint row, gint column, GdkEventButton *event, Tcs3_diag *diag) {
	diag->selected_row = row;
	if (diag->selected_row != -1) {
		gchar *text[3];
		if (diag->styletype == onestyle) {
			text[0] = NULL;
			gtk_clist_get_text(GTK_CLIST(diag->clist), diag->selected_row, 0, &text[1]);
			gtk_clist_get_text(GTK_CLIST(diag->clist), diag->selected_row, 1, &text[2]);
		} else { /* multistyle */
			gtk_clist_get_text(GTK_CLIST(diag->clist), diag->selected_row, 0, &text[0]);
			gtk_clist_get_text(GTK_CLIST(diag->clist), diag->selected_row, 1, &text[1]);
			gtk_clist_get_text(GTK_CLIST(diag->clist), diag->selected_row, 2, &text[2]);
		}
		if (text[0]) {
			gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(diag->selector)->entry), text[0]);
		}
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(diag->property)->entry), text[1]);
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(diag->value)->entry), text[2]);
		DEBUG_MSG("cs3d_select_row_lcb, sele=%s, prop=%s, val=%s\n", text[0], text[1], text[2]);
	}
}

static void cs3d_unselect_row_lcb(GtkWidget *clist, gint row, gint column, GdkEventButton *event, Tcs3_diag *diag) {
	diag->selected_row = -1;
	if (diag->styletype == multistyle) {
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(diag->selector)->entry), "");
	}
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(diag->property)->entry), "");
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(diag->value)->entry), "");
}

static void add_to_row(Tcs3_diag *diag, gint whichrow) {
	gchar *text[4];
	gint count, correct=1;
	if (diag->styletype == multistyle) {
		DEBUG_MSG("add_to_row, multistyle\n");
		text[0] = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(diag->selector)->entry), 0, -1);
		text[1] = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(diag->property)->entry), 0, -1);
		text[2] = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(diag->value)->entry), 0, -1);
	} else {
		DEBUG_MSG("add_to_row, onestyle\n");
		text[0] = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(diag->property)->entry), 0, -1);
		text[1] = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(diag->value)->entry), 0, -1);
		text[2] = NULL;
	}
	text[3] = NULL;

	count=0;
	while (text[count]) {
		if (strlen(text[count]) == 0) {
			correct = 0;
		}
		count++;
	}
	if (correct) {
		if (whichrow == -1) {
			gtk_clist_append(GTK_CLIST(diag->clist), text);
		} else {
			DEBUG_MSG("add_to_row, removing %d\n", whichrow);
			gtk_clist_remove(GTK_CLIST(diag->clist), whichrow);
			DEBUG_MSG("add_to_row, adding new %d\n", whichrow);
			gtk_clist_insert(GTK_CLIST(diag->clist), whichrow, text);
		}
		gtk_clist_sort(GTK_CLIST(diag->clist));
	}
	count=0;
	while (text[count]) {
		g_free(text[count]);
		count++;
	}
}

static void cs3d_add_clicked_lcb(GtkWidget * widget, Tcs3_diag *diag) {
	add_to_row(diag, -1);
}

static void cs3d_del_clicked_lcb(GtkWidget * widget, Tcs3_diag *diag) {
	if (diag->selected_row != -1) {
		gtk_clist_remove(GTK_CLIST(diag->clist), diag->selected_row);
		diag->selected_row = -1;
	}
}

static void cs3d_update_clicked_lcb(GtkWidget * widget, Tcs3_diag *diag) {
	if (diag->selected_row != -1) {
		DEBUG_MSG("cs3d_update_clicked_lcb, updating row=%d\n", diag->selected_row);
		add_to_row(diag, diag->selected_row);
	}
}

static void cs3d_prop_activate_lcb(GtkWidget * widget, Tcs3_diag *diag) {
	Tcs3_arr *tmp;
	gchar *tmpstr;

	tmpstr = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(diag->property)->entry), 0, -1);
	tmp = cs3_arr_from_property(tmpstr);
	g_free(tmpstr);
	if (tmp) {
		GList *list;
		list = pointer_arr2glist(tmp->possibilities);
		if (list) {
			gchar *tmpstr2=NULL;
			if (!tmp->force_pos) {
					tmpstr2 = gtk_editable_get_chars(GTK_EDITABLE(GTK_COMBO(diag->value)->entry), 0, -1);
			}
			gtk_combo_set_popdown_strings(GTK_COMBO(diag->value), list);
			g_list_free(list);
			if (tmpstr2) {
				gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(diag->value)->entry), tmpstr2);
				g_free(tmpstr2);
			}
			switch(tmp->buttype) {
			case but_color:
				gtk_widget_set_sensitive(diag->extra_but, TRUE);
			break;
			default:
				gtk_widget_set_sensitive(diag->extra_but, FALSE);
			break;
			}
		} else {
				gtk_widget_set_sensitive(diag->extra_but, FALSE);
		}
		gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(diag->value)->entry), !tmp->force_pos);
	} else {
		gtk_widget_set_sensitive(diag->extra_but, FALSE);
	}
}

static Tcs3_diag *css_diag(Tcs3_destination dest, Tcs3_style style, GtkWidget *transient_win, gboolean grab) {

	Tcs3_diag *diag;
	GtkWidget *scrolwin, *table, *but, *vbox, *hbox, *vbox2;
	GList *tmplist = NULL;
	Tcs3_arr tmp;
	gint count=0;
	
	diag = g_malloc(sizeof(Tcs3_diag));
	diag->win = window_full(_("Cascading StyleSheet dialog"), GTK_WIN_POS_MOUSE, 
			12, G_CALLBACK(cs3d_destroy_lcb), diag, TRUE);
	gtk_window_set_role(GTK_WINDOW(diag->win), "css");
	diag->dest = dest;
	diag->styletype = style;
	diag->grab = grab;
	diag->selected_row = -1;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(diag->win),vbox);
	
	table = gtk_table_new(3, 6, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 12);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
	tmplist = NULL;
	
	if (diag->styletype == multistyle) {
		tmplist = glist_with_html_tags(1);
		diag->selector = combo_with_popdown(NULL, tmplist,1);
		bf_mnemonic_label_tad_with_alignment(_("_Selector:"), diag->selector, 0, 0.5, table, 0, 1, 0, 1);
		gtk_table_attach_defaults(GTK_TABLE(table), diag->selector, 1 ,5 , 0, 1);
		g_list_free(tmplist);
		tmplist = NULL;
	} 

	tmp = cs3_arr[count];
	while (tmp.property) {
		tmplist = g_list_append(tmplist, tmp.property);
		count++;
		tmp = cs3_arr[count];
	}
	diag->property = combo_with_popdown(NULL, tmplist,1);
	g_list_free(tmplist);
	tmplist = NULL;
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(diag->property)->entry), "activate", G_CALLBACK(cs3d_prop_activate_lcb), diag);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(diag->property)->entry), "changed", G_CALLBACK(cs3d_prop_activate_lcb), diag);

	diag->value = combo_with_popdown(NULL, tmplist,1);
	bf_mnemonic_label_tad_with_alignment(_("_Property:"), diag->property, 0, 0.5, table, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), diag->property, 1, 5, 1, 2);
	bf_mnemonic_label_tad_with_alignment(_("_Value:"), diag->value, 0, 0.5, table, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(table), diag->value, 1, 5, 2, 3);
	
	gtk_widget_realize(diag->win);

	diag->extra_but = color_but_new(GTK_WIDGET(GTK_COMBO(diag->value)->entry), diag->win);
	gtk_table_attach(GTK_TABLE(table), diag->extra_but, 5, 6, 2, 3, GTK_EXPAND, GTK_EXPAND, 0, 0);

	/* the list widget and the buttons are in a horizontal box */
	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 18);

	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(scrolwin, 400, 300);
	gtk_box_pack_start(GTK_BOX(hbox), scrolwin, FALSE, FALSE, 0);
	
	if (diag->styletype == multistyle) {
		gchar *titles[] = {_("Selector"), _("Property"), _("Value"), NULL};
		diag->clist = gtk_clist_new_with_titles(3, titles);
	} else {
		gchar *titles[] = {_("Property"), _("Value"), NULL};
		diag->clist = gtk_clist_new_with_titles(2, titles);
	}
	gtk_clist_set_sort_column(GTK_CLIST(diag->clist), 0);
	gtk_clist_set_auto_sort(GTK_CLIST(diag->clist), TRUE);

/*	gtk_clist_columns_autosize(GTK_CLIST(diag->clist));*/
	gtk_signal_connect(GTK_OBJECT(diag->clist), "select_row", GTK_SIGNAL_FUNC(cs3d_select_row_lcb), diag);
	gtk_signal_connect(GTK_OBJECT(diag->clist), "unselect_row", GTK_SIGNAL_FUNC(cs3d_unselect_row_lcb), diag);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolwin), diag->clist);

	vbox2 = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 0);
	
	but = bf_generic_mnemonic_button(_(" _Add "), G_CALLBACK(cs3d_add_clicked_lcb), diag);
	gtk_box_pack_start(GTK_BOX(vbox2), but, FALSE, FALSE, 0);

	but = bf_generic_mnemonic_button(_(" _Update "), G_CALLBACK(cs3d_update_clicked_lcb), diag);
	gtk_box_pack_start(GTK_BOX(vbox2), but, FALSE, FALSE, 0);
	
	but = bf_generic_mnemonic_button(_(" _Delete "), G_CALLBACK(cs3d_del_clicked_lcb), diag);
	gtk_box_pack_start(GTK_BOX(vbox2), but, FALSE, FALSE, 0);

	/* the ok and cancel button are in a horizontal box below */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_hseparator_new(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 12);	

	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 12);
	
	but = bf_stock_cancel_button(G_CALLBACK(cs3d_cancel_clicked_lcb), diag);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 0);
	but = bf_stock_ok_button(G_CALLBACK(cs3d_ok_clicked_lcb), diag);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);	
	
	gtk_widget_show_all(diag->win);
	
	cs3d_prop_activate_lcb(NULL, diag);
	
	if (transient_win) {
		gtk_window_set_transient_for(GTK_WINDOW(diag->win), GTK_WINDOW(transient_win));
	}
	if (diag->grab) {
		gtk_grab_add(diag->win);
	}
	return diag;
}

typedef enum {property, value, end} Tcs3_prevtype_onestyle;
typedef enum {mselector, mproperty, mvalue, mend, mselectorend} Tcs3_prevtype_multistyle;

/* static void css_parse(Tcs3_diag *diag, gchar *data)
 * the buffer 'data' should contain a css stylesheet
 * the comments are removed from this buffer by this function
 * so if you don't want this, make a copy of the stylesheet
 * before you pass it on to this function
 */
static void css_parse(Tcs3_diag *diag, gchar *data) {
	gchar *tmp = data;
	gint count = 0, parsable_css = 1;

	if (!data) {
		DEBUG_MSG("css_parse, no data!\n");
		return;
	}

	if (diag->styletype == onestyle) {
		gint prev_end = 0;
		Tcs3_prevtype_onestyle prevtype = end;
		gchar *text[3];
		
		DEBUG_MSG("css_parse, onestyle\n");
		text[2] = NULL;
		while (tmp[count] && parsable_css) {
			switch (tmp[count]) {
			case ':':
				if (prevtype == end) {
					text[0] = g_strndup(&tmp[prev_end], count - prev_end);
					strip_any_whitespace(text[0]);
					prev_end = count+1;
					prevtype = property;
				}
			break;
			case ';':
				if (prevtype == property) {
					text[1] = g_strndup(&tmp[prev_end], count - prev_end);
					strip_any_whitespace(text[1]);
					prev_end = count+1;
					prevtype = value;
				}
			break;
			case '*':
				if ((count > 0)&& (tmp[count-1] == '/')) {
					gint count2 = count, cont=1, movelen;
					DEBUG_MSG("css_parse, found the start of a comment on %d\n", count);
					while (tmp[++count2] && cont) {
						if (tmp[count2] == '/' && tmp[count2-1] == '*') {
							cont=0;
						}
					}
					DEBUG_MSG("css_parse, before move, tmp='%s'\n", tmp);
					movelen = strlen(&tmp[count2]);
					memmove(&tmp[count-1], &tmp[count2], movelen);
					tmp[count-1 + movelen] = '\0';
					DEBUG_MSG("css_parse, after move, tmp='%s'\n", tmp);
				}
			break;
			case '<':
			case '>':
				parsable_css = 0;
				DEBUG_MSG("css_parse, this is not a valid stylesheet\n");
			break;

			default:
				/* do nothing */
			break;
			}
			if (prevtype == value) {
				gtk_clist_append(GTK_CLIST(diag->clist), text);
				g_free(text[0]);
				g_free(text[1]);
				prevtype = end;
			}
			count++;
		}
	
	
	} else { /* multistyle */
		gint prev_end = 0, i;
		Tcs3_prevtype_multistyle prevtype = mselectorend;
		gchar *text[4] = {NULL, NULL, NULL, NULL};

		DEBUG_MSG("css_parse, multistylestyle\n");		
		
		while (tmp[count] && parsable_css) {
			switch (tmp[count]) {
			case '}': /* found end of block */
				if (prevtype != mselectorend) {
					prevtype = mselectorend;
					prev_end = count+1;
					g_free(text[0]);
					text[0]=NULL;
				}
			break;
			case '{': /* found selector */
				if (prevtype == mselectorend) {
					text[0] = g_strndup(&tmp[prev_end], count - prev_end);
					strip_any_whitespace(text[0]);
					prev_end = count+1;
					prevtype = mselector;
				}
			break;
			case ':':
				if (prevtype == mselector || prevtype == mend) {
					text[1] = g_strndup(&tmp[prev_end], count - prev_end);
					strip_any_whitespace(text[1]);
					prev_end = count+1;
					prevtype = mproperty;
				}
			break;
			case ';':
				if (prevtype == mproperty) {
					text[2] = g_strndup(&tmp[prev_end], count - prev_end);
					strip_any_whitespace(text[2]);
					prev_end = count+1;
					prevtype = mvalue;
				}
			break;
			case '*':
				if ((count > 0)&& (tmp[count-1] == '/')) {
					gint count2 = count, cont=1, movelen;
					DEBUG_MSG("css_parse, found the start of a comment on %d\n", count);
					while (tmp[++count2] && cont) {
						if (tmp[count2] == '/' && tmp[count2-1] == '*') {
							cont=0;
						}
					}
					DEBUG_MSG("css_parse, before move, tmp='%s'\n", tmp);
					movelen = strlen(&tmp[count2]);
					memmove(&tmp[count-1], &tmp[count2], movelen);
					tmp[count-1 + movelen] = '\0';
					DEBUG_MSG("css_parse, after move, tmp='%s'\n", tmp);
				}
			break;
			case '<':
			case '>':
				parsable_css = 0;
				DEBUG_MSG("css_parse, this is not a valid stylesheet\n");
			break;
			default:
				/* do nothing */
			break;
			}
			if (prevtype == mvalue) {
				gtk_clist_append(GTK_CLIST(diag->clist), text);
				g_free(text[1]);
				text[1] = NULL;
				g_free(text[2]);
				text[2] = NULL;
				prevtype = mend;
			}
			count++;
		}
		for (i=0;i<4;i++) {
			if (text[i]){
				g_free(text[i]);
			}
		}
	}
	gtk_clist_sort(GTK_CLIST(diag->clist));
	DEBUG_MSG("css_parse, finished\n");
}

void new_css_dialog(GtkWidget *wid, Tbfwin *bfwin) {
	Tcs3_destination dest;
	Tcs3_diag *diag;
	gint sel_start, sel_end;
	gboolean has_selection;

	dest.dest_type = textbox;
	dest.entry = NULL;
	dest.doc = bfwin->current_document;
	if (!doc_get_selection(dest.doc, &sel_start, &sel_end)) {
		dest.doc_start = -1;
		dest.doc_end = -1;
		has_selection = FALSE;
	} else {
		has_selection = TRUE;
		dest.doc_start = sel_start;
		dest.doc_end = sel_end;
	}
	if (dest.doc_start > dest.doc_end) {
		gint swap = dest.doc_start;
		dest.doc_start = dest.doc_end;
		dest.doc_end = swap;
	}

	diag = css_diag(dest, multistyle, NULL, FALSE);
	if (has_selection) {
		gchar *data;

		data = doc_get_chars(dest.doc, sel_start, sel_end);
		DEBUG_MSG("new_css_dialog, data=%p\n", data);
		css_parse(diag, data);
		g_free(data);
	}
}

static void style_but_clicked_lcb(GtkWidget * widget, GtkWidget * which_entry)
{
	Tcs3_destination dest;
	Tcs3_diag *diag;
	gchar *data;
	GtkWidget *win;

	dest.dest_type = entry;
	dest.entry = which_entry;
	dest.doc = NULL;

	win = gtk_widget_get_toplevel(which_entry);
	diag = css_diag(dest, onestyle, win, TRUE);
	data = gtk_editable_get_chars(GTK_EDITABLE(which_entry), 0, -1);
	css_parse(diag, data);
	g_free(data);
}

GtkWidget *style_but_new(GtkWidget * which_entry, GtkWidget * win)
{
	GtkWidget *style_but, *hbox;

	style_but = gtk_button_new();
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), new_pixmap(92),FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new_with_mnemonic(_("_Style...")), TRUE, TRUE, 3);
	gtk_container_add(GTK_CONTAINER(style_but), hbox);
	gtk_signal_connect(GTK_OBJECT(style_but), "clicked", G_CALLBACK(style_but_clicked_lcb), which_entry);
	gtk_widget_show_all(style_but);
	return style_but;
}

static void style_but_for_wizard_clicked_lcb(GtkWidget * widget, GtkWidget * textview)
{
	Tcs3_destination dest;
	Tcs3_diag *diag;
	gchar *data;
	GtkWidget *win;

	dest.dest_type = wizard;
	dest.entry = textview;
	dest.doc = NULL;

	win = gtk_widget_get_toplevel(textview);
	diag = css_diag(dest, multistyle, win, TRUE);
	{
		GtkTextBuffer *buf;
		GtkTextIter itstart, itend;
		buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
		gtk_text_buffer_get_bounds(buf,&itstart,&itend);
		data = gtk_text_buffer_get_text(buf, &itstart, &itend, FALSE);
		css_parse(diag, data);
		g_free(data);
	}
}

GtkWidget *style_but_new_for_wizard(GtkWidget * textview) {
	GtkWidget *style_but, *hbox;

	style_but = gtk_button_new();
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), new_pixmap(92),FALSE, FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new_with_mnemonic(_("_Style...")), TRUE, TRUE, 6);
	gtk_container_add(GTK_CONTAINER(style_but), hbox);
	gtk_signal_connect(GTK_OBJECT(style_but), "clicked", G_CALLBACK(style_but_for_wizard_clicked_lcb), textview);
	gtk_widget_show_all(style_but);
	return style_but;
}

/*****************/
/* colorbut code */
/*****************/

typedef struct {
	GtkWidget *win;
	GtkWidget *hexentry;
	GtkWidget *websafe;
	GtkWidget *csel;
	gint hex_changed_id;
	gint csel_changed_id;
	/* the dialog is dual functional: it can be caklled directly from the text, and it can be called by return_color 
	if called by return_color it is in the modal state */
	gint is_modal;
	/* for the modal state */
	gchar *returnval;

	/* for the non-modal state */
	gint startpos;
	gint endpos;
	Tbfwin *bfwin;
} Tcolsel;

static gchar *GdkColor_to_hexstring(GdkColor *color, gboolean websafe) {
	gchar *tmpstr;

	tmpstr = g_malloc(8*sizeof(char));
	if (websafe) {
		g_snprintf (tmpstr, 8,"#%.2X%.2X%.2X", (0x33 * color->red/(256*0x33)), (0x33 * color->green/(256*0x33)), (0x33 * color->blue/(256*0x33)) );
	} else {
		g_snprintf (tmpstr, 8,"#%.2X%.2X%.2X", color->red/256, color->green/256, color->blue/256);
	}
	return tmpstr;
}
/*
static gdouble *hex_to_gdouble_arr(const gchar *color) {
	static gdouble tmpcol[4];
	gchar tmpstr[8];
	long tmpl;

	if (!string_is_color(color)) {
		DEBUG_MSG("hex_to_gdouble_arr, color is not a valid #...... color\n");
		return NULL;
	}
	strncpy(tmpstr, &color[1], 2);
	tmpl = strtol(tmpstr, NULL, 16);
	tmpcol[0] = (gdouble) tmpl;
	
	strncpy(tmpstr, &color[3], 2);
	tmpl = strtol(tmpstr, NULL, 16);
	tmpcol[1] = (gdouble) tmpl;
	
	strncpy(tmpstr, &color[5], 2);
	tmpl = strtol(tmpstr, NULL, 16);
	tmpcol[2] = (gdouble) tmpl;

	DEBUG_MSG("hex_to_gdouble_arr, R=%d, G=%d, B=%d\n", color[0], color[1], color[2]);

	tmpcol[3] = 0;
	return tmpcol;
}
*/ 
static void colsel_destroy_lcb(GtkWidget *widget, Tcolsel *csd) {
	DEBUG_MSG("colsel_destroy_lcb, started for csd=%p\n",csd);
	g_free(csd->returnval);
	g_free(csd);
}

static void colsel_ok_clicked_lcb(GtkWidget *widget, Tcolsel *csd) { 
	gchar *tmpstr;
	GdkColor gcolor;
	/* only on a OK click we do the setcolor thing */
	gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(csd->csel), &gcolor);

	tmpstr = GdkColor_to_hexstring(&gcolor, FALSE);
	if (csd->bfwin) {
		csd->bfwin->session->colorlist = add_to_stringlist(csd->bfwin->session->colorlist, tmpstr);
	}
	
	if (csd->is_modal) {
		g_free(csd->returnval);
		csd->returnval = tmpstr;
		gtk_main_quit();
	} else {
		if (string_is_color(tmpstr)) {
			if (csd->startpos || csd->endpos) {
				DEBUG_MSG("colsel_ok_clicked_lcb, replacing with %s\n", tmpstr);
				doc_replace_text(csd->bfwin->current_document, tmpstr, csd->startpos, csd->endpos);
			} else {
				DEBUG_MSG("colsel_ok_clicked_lcb, inserting %s\n", tmpstr);
				doc_insert_two_strings(csd->bfwin->current_document,tmpstr, NULL);
			}
		}
		g_free(tmpstr);
		window_destroy(csd->win);
	}
}

static void colsel_cancel_clicked_lcb(GtkWidget *widget, Tcolsel *csd) {
	window_destroy(csd->win);
}
/* declaration needed to connect/disconnect callback */
/*static void colsel_color_changed(GtkWidget *widget, Tcolsel *csd);

static void hexentry_color_changed(GtkWidget *widget, Tcolsel *csd) {
	gdouble *color;
	gchar *tmpstr;
	tmpstr=gtk_editable_get_chars(GTK_EDITABLE(csd->hexentry), 0, -1);
	if (strlen(tmpstr) == 7){
		color = hex_to_gdouble_arr(tmpstr);
		if (color) {
			gtk_signal_disconnect(GTK_OBJECT(csd->csel), csd->csel_changed_id);
			gtk_color_selection_set_color(GTK_COLOR_SELECTION(csd->csel), color);
			csd->csel_changed_id = gtk_signal_connect(GTK_OBJECT(csd->csel), "color-changed", G_CALLBACK(colsel_color_changed), csd);
		} else {
			tmpstr=NULL;
		}
	}
	g_free(tmpstr);
}*/

/*
static void colsel_color_changed(GtkWidget *widget, Tcolsel *csd) {
	gdouble color[4];
	gchar *tmpstr;

	gtk_color_selection_get_color(GTK_COLOR_SELECTION(csd->csel), color);
	tmpstr = gdouble_arr_to_hex(color, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(csd->websafe)));

	gtk_signal_disconnect(GTK_OBJECT(csd->hexentry), csd->hex_changed_id);
	gtk_entry_set_text(GTK_ENTRY(csd->hexentry), tmpstr);
	csd->hex_changed_id = gtk_signal_connect(GTK_OBJECT(csd->hexentry), "changed", G_CALLBACK(hexentry_color_changed), csd);
	g_free(tmpstr);
}*/

static Tcolsel *colsel_dialog(Tbfwin *bfwin,const gchar *setcolor, gint modal, gint startpos, gint endpos) {
	Tcolsel *csd;
	GtkWidget *vbox, *hbox, *but;
	GdkColor gcolor;
	const gchar *this_color=setcolor;

	csd = g_malloc(sizeof(Tcolsel));
	/* warning: bfwin might be NULL if this dialog is started by return_color() */
	csd->bfwin = bfwin;
	csd->is_modal = modal;
	csd->startpos = startpos;
	csd->endpos = endpos;
	csd->returnval = setcolor ? g_strdup(setcolor) : g_strdup("");
	
	DEBUG_MSG("colsel_dialog, malloced at %p, setcolor=%s\n", csd, setcolor);
	csd->win = window_full2(_("Bluefish: Select color"), GTK_WIN_POS_MOUSE, 12, G_CALLBACK(colsel_destroy_lcb), csd, TRUE, NULL);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(csd->win), vbox);
	csd->csel = gtk_color_selection_new();
	gtk_color_selection_set_has_opacity_control(GTK_COLOR_SELECTION(csd->csel), FALSE);
	
	if (this_color) {
		if (gdk_color_parse(this_color, &gcolor)) {
			gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(csd->csel), &gcolor);
		} else {
			this_color=NULL;
		}
	}
	if (bfwin && bfwin->session->colorlist) {
		GtkSettings* gtksettings;
/*		GdkColor *gcolorarr=NULL;
		gint num=0;*/
		gchar *strings = stringlist_to_string(bfwin->session->colorlist, ":");
		strings[strlen(strings)-1] = '\0';
		gtksettings = gtk_widget_get_settings(GTK_WIDGET(csd->csel));
		g_object_set(G_OBJECT(gtksettings), "gtk-color-palette", strings, NULL); 
/*		DEBUG_MSG("colsel_dialog, setting palette from %s\n", strings);
		if (gtk_color_selection_palette_from_string(strings, &gcolorarr, &num)) {
			DEBUG_MSG("num=%d, gcolorarr=%p\n",num,gcolorarr);
		} else {
			DEBUG_MSG("hmm, failed to parse our string :(\n");
		} */
		g_free(strings);
	}
	gtk_color_selection_set_has_palette(GTK_COLOR_SELECTION(csd->csel), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), csd->csel, TRUE, TRUE, 0);
/*	
	csd->csel_changed_id = gtk_signal_connect(GTK_OBJECT(csd->csel), "color-changed", G_CALLBACK(colsel_color_changed), csd);
	

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 12);
	
	csd->hexentry = boxed_entry_with_text(this_color, 7, hbox);
	csd->hex_changed_id = gtk_signal_connect(GTK_OBJECT(csd->hexentry), "changed", G_CALLBACK(hexentry_color_changed), csd);
	csd->websafe = boxed_checkbut_with_value(_("_websafe"), 0, hbox);
*/
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_hseparator_new(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 12);

	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 12);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

	but = bf_stock_cancel_button(G_CALLBACK(colsel_cancel_clicked_lcb), csd);
	gtk_box_pack_start(GTK_BOX(hbox), but, TRUE, TRUE, 0);
	but = bf_stock_ok_button(G_CALLBACK(colsel_ok_clicked_lcb), csd);
	gtk_window_set_default(GTK_WINDOW(csd->win), but);
	gtk_box_pack_start(GTK_BOX(hbox), but, TRUE, TRUE, 0);
	gtk_widget_show_all(csd->win);

	DEBUG_MSG("colsel_dialog, finished\n");
	return csd;
}

void edit_color_dialog(Tdocument *doc, gchar *color, gint startpos, gint endpos) {
	colsel_dialog(doc->bfwin, color, FALSE, startpos, endpos);
}

void sel_colour_cb(GtkWidget *widget, Tbfwin *bfwin) {
	Tcolsel *csd;
	gchar *tmpstr=NULL;
	gint startpos=0;
	gint endpos=0;

	if (doc_get_selection(bfwin->current_document,&startpos , &endpos)) {
		DEBUG_MSG("sel_colour_cb, selection found\n");
		if (startpos > endpos) {
			gint tmpint;
			tmpint = startpos;
			startpos = endpos;		
			endpos = tmpint;
		}
		if ((endpos - startpos) == 7) {
			tmpstr = doc_get_chars(bfwin->current_document,startpos,endpos);
			if (!string_is_color(tmpstr)) {
				startpos = endpos = 0;
			}
			DEBUG_MSG("sel_colour_cb, tmpstr = %s (startpos=%d, endpos=%d)\n", tmpstr, startpos, endpos);
		} else {
			DEBUG_MSG("sel_colour_cb, startpos=%d, endpos=%d\n", startpos, endpos);
			startpos = endpos = 0;
		}
	} else {
		DEBUG_MSG("sel_colour_cb, NO selection found\n");
	}

	csd = colsel_dialog(bfwin, tmpstr, 0, startpos, endpos);
	if (tmpstr) {
		g_free(tmpstr);
	}
}
 
gchar *return_color(gchar *start_value) {
	Tcolsel *csd;
	gchar *return_text;

	csd = colsel_dialog(NULL,start_value, 1, 0, 0);
	DEBUG_MSG("return color, started\n");
	gtk_grab_add(csd->win);
	gtk_main();

	return_text = g_strdup(csd->returnval);
	window_destroy(csd->win);
	DEBUG_MSG("return_color, the new gtk_main stopped, return_text=%s, %p\n", return_text, return_text);
	return return_text;
}

static void color_but_clicked(GtkWidget * widget, GtkWidget * entry)
{
	gchar *tmpstring, *tmpstr2;

	DEBUG_MSG("color_but_clicked, before return_color\n");
	tmpstr2 = gtk_editable_get_chars(GTK_EDITABLE(entry),0 ,-1);
	tmpstring = return_color(tmpstr2);
	DEBUG_MSG("color_but_clicked, return_color=%s\n", tmpstring);
	gtk_entry_set_text(GTK_ENTRY(entry), tmpstring);
	g_free(tmpstring);
	g_free(tmpstr2);

}

GtkWidget *color_but_new(GtkWidget * which_entry, GtkWidget * win)
{
	GtkWidget *color_but;

	color_but = gtk_button_new_from_stock(GTK_STOCK_SELECT_COLOR);
	gtk_signal_connect(GTK_OBJECT(color_but), "clicked", G_CALLBACK(color_but_clicked), which_entry);
	gtk_widget_show(color_but);
	return color_but;
}
