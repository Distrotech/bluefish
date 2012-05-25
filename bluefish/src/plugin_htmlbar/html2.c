/* Bluefish HTML Editor
 * html2.c - menu/toolbar callbacks, inserting functions, and other cool stuff 
 * otherwise html.c is getting so long ;-)
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

/* #define DEBUG */

#include <gtk/gtk.h>
#include <string.h>            /* strlen() */
#include <stdlib.h>            /* strtol() */

#include "htmlbar.h"
#include "htmlbar_stock_icons.h"
#include "html2.h"
#include "cap.h"
#include "../bf_lib.h"         /* string_is_color(), strip_any_whitespace()*/
#include "../dialog_utils.h"
#include "../document.h"
#include "../gtk_easy.h"
#include "../stringlist.h"

static GList *glist_with_html_tags(gint use_html5) {
	GList *tmplist = NULL;

	tmplist = g_list_prepend(tmplist, "var");
	tmplist = g_list_prepend(tmplist, "ul");
	tmplist = g_list_prepend(tmplist, "tr");
	tmplist = g_list_prepend(tmplist, "title");
	tmplist = g_list_prepend(tmplist, "thead");
	tmplist = g_list_prepend(tmplist, "th");
	tmplist = g_list_prepend(tmplist, "tfoot");
	tmplist = g_list_prepend(tmplist, "textarea");
	tmplist = g_list_prepend(tmplist, "td");
	tmplist = g_list_prepend(tmplist, "tbody");
	tmplist = g_list_prepend(tmplist, "table");
	tmplist = g_list_prepend(tmplist, "sup");
	tmplist = g_list_prepend(tmplist, "sub");
	tmplist = g_list_prepend(tmplist, "style");
	tmplist = g_list_prepend(tmplist, "strong");
	tmplist = g_list_prepend(tmplist, "span");
	tmplist = g_list_prepend(tmplist, "small");
	tmplist = g_list_prepend(tmplist, "select");
	tmplist = g_list_prepend(tmplist, "script");
	tmplist = g_list_prepend(tmplist, "samp");
	tmplist = g_list_prepend(tmplist, "q");
	tmplist = g_list_prepend(tmplist, "pre");
	tmplist = g_list_prepend(tmplist, "param");
	tmplist = g_list_prepend(tmplist, "p");
	tmplist = g_list_prepend(tmplist, "option");
	tmplist = g_list_prepend(tmplist, "optgroup");
	tmplist = g_list_prepend(tmplist, "ol");
	tmplist = g_list_prepend(tmplist, "object");
	tmplist = g_list_prepend(tmplist, "noscript");
	tmplist = g_list_prepend(tmplist, "meta");
	tmplist = g_list_prepend(tmplist, "map");
	tmplist = g_list_prepend(tmplist, "link");
	tmplist = g_list_prepend(tmplist, "li");
	tmplist = g_list_prepend(tmplist, "legend");
	tmplist = g_list_prepend(tmplist, "label");
	tmplist = g_list_prepend(tmplist, "kbd");
	tmplist = g_list_prepend(tmplist, "ins");
	tmplist = g_list_prepend(tmplist, "input");
	tmplist = g_list_prepend(tmplist, "img");
	tmplist = g_list_prepend(tmplist, "iframe");
	tmplist = g_list_prepend(tmplist, "i");
	tmplist = g_list_prepend(tmplist, "html");
	tmplist = g_list_prepend(tmplist, "hr");
	tmplist = g_list_prepend(tmplist, "head");
	tmplist = g_list_prepend(tmplist, "h6");
	tmplist = g_list_prepend(tmplist, "h5");
	tmplist = g_list_prepend(tmplist, "h4");
	tmplist = g_list_prepend(tmplist, "h3");
	tmplist = g_list_prepend(tmplist, "h2");
	tmplist = g_list_prepend(tmplist, "h1");
	tmplist = g_list_prepend(tmplist, "form");
	tmplist = g_list_prepend(tmplist, "fieldset");
	tmplist = g_list_prepend(tmplist, "em");
	tmplist = g_list_prepend(tmplist, "dt");
	tmplist = g_list_prepend(tmplist, "dl");
	tmplist = g_list_prepend(tmplist, "div");
	tmplist = g_list_prepend(tmplist, "dfn");
	tmplist = g_list_prepend(tmplist, "del");
	tmplist = g_list_prepend(tmplist, "dd");
	tmplist = g_list_prepend(tmplist, "colgroup");
	tmplist = g_list_prepend(tmplist, "col");
	tmplist = g_list_prepend(tmplist, "code");
	tmplist = g_list_prepend(tmplist, "cite");
	tmplist = g_list_prepend(tmplist, "caption");
	tmplist = g_list_prepend(tmplist, "button");
	tmplist = g_list_prepend(tmplist, "br");
	tmplist = g_list_prepend(tmplist, "body");
	tmplist = g_list_prepend(tmplist, "blockquote");
	tmplist = g_list_prepend(tmplist, "bdo");
	tmplist = g_list_prepend(tmplist, "base");
	tmplist = g_list_prepend(tmplist, "b");
	tmplist = g_list_prepend(tmplist, "area");
	tmplist = g_list_prepend(tmplist, "address");
	tmplist = g_list_prepend(tmplist, "abbr");
	tmplist = g_list_prepend(tmplist, "a");
	tmplist = g_list_prepend(tmplist, "");
	if (use_html5) {
		tmplist = g_list_prepend(tmplist, "wbr");
		tmplist = g_list_prepend(tmplist, "video");
		tmplist = g_list_prepend(tmplist, "u");
		tmplist = g_list_prepend(tmplist, "track");
		tmplist = g_list_prepend(tmplist, "time");
		tmplist = g_list_prepend(tmplist, "summary");
		tmplist = g_list_prepend(tmplist, "source");
		tmplist = g_list_prepend(tmplist, "section");
		tmplist = g_list_prepend(tmplist, "s");
		tmplist = g_list_prepend(tmplist, "ruby");
		tmplist = g_list_prepend(tmplist, "rt");
		tmplist = g_list_prepend(tmplist, "rp");
		tmplist = g_list_prepend(tmplist, "progress");
		tmplist = g_list_prepend(tmplist, "output");
		tmplist = g_list_prepend(tmplist, "nav");
		tmplist = g_list_prepend(tmplist, "meter");
		tmplist = g_list_prepend(tmplist, "menu");
		tmplist = g_list_prepend(tmplist, "mark");
		tmplist = g_list_prepend(tmplist, "keygen");
		tmplist = g_list_prepend(tmplist, "hgroup");
		tmplist = g_list_prepend(tmplist, "header");
		tmplist = g_list_prepend(tmplist, "footer");
		tmplist = g_list_prepend(tmplist, "figure");
		tmplist = g_list_prepend(tmplist, "figcaption");
		tmplist = g_list_prepend(tmplist, "embed");
		tmplist = g_list_prepend(tmplist, "details");
		tmplist = g_list_prepend(tmplist, "datalist");
		tmplist = g_list_prepend(tmplist, "command");
		tmplist = g_list_prepend(tmplist, "canvas");
		tmplist = g_list_prepend(tmplist, "bdi");
		tmplist = g_list_prepend(tmplist, "audio");
		tmplist = g_list_prepend(tmplist, "aside");
		tmplist = g_list_prepend(tmplist, "article");
	}	
	else {
		tmplist = g_list_prepend(tmplist, "tt");
		tmplist = g_list_prepend(tmplist, "noframes");
		tmplist = g_list_prepend(tmplist, "frameset");
		tmplist = g_list_prepend(tmplist, "frame");
		tmplist = g_list_prepend(tmplist, "big");
		tmplist = g_list_prepend(tmplist, "acronym");
	}
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
	/*GtkWidget *clist;*/
	GtkListStore *lstore;
	GtkWidget *lview; 
	gint selected_row;
	gboolean grab;
	GtkWidget *selector;
	GtkWidget *html5;
	GtkWidget *property;
	GtkWidget *value;
	GtkWidget *rule_add_but;
	GtkWidget *rule_update_but;
	GtkWidget *extra_but;
} Tcs3_diag;

typedef struct {
	GtkWidget *win;
	GtkWidget *selector;
	GtkWidget *html5;
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

static gchar *cs3_colors[] = {"aqua", "black", "blue", "fuchsia", "gray", "green", "lime", "maroon", "navy", "olive", "orange", "purple", "red", "silver", "teal", "white", "yellow", NULL};
static gchar *cs3_repeat[] = {"repeat", "repeat-x", "repeat-y", "no-repeat", NULL};
static gchar *cs3_fonts[] = {"arial, helvetica, sans-serif", "roman, 'times new roman', times, serif", "courier, fixed, monospace", "western, fantasy",  "Zapf-Chancery, cursive", "serif", "sans-serif", "cursive", "fantasy", "monospace", NULL};
//static gchar *cs3_font_size_adjustments[] = {"0.58", "none", "inherit", NULL};
//static gchar *cs3_font_stretches[] = {"normal","wider","narrower","ultra-condensed","extra-condensed","condensed","semi-condensed","semi-expanded","expanded","extra-expanded","ultra-expanded", NULL};
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
static gchar *cs3_content[] = {"normal", "none", "url()", "counter()", "counters()", "attr()",
 "open-quote","close-quote", "no-open-quote", "no-close-quote", "inherit", NULL};
static gchar *cs3_cursor[] = {"url()", "auto", "crosshair", "default", "help", "move",
 "pointer ", "progress", "text", "wait", "e-resize", "ne-resize", "nw-resize", "n-resize",
  "se-resize", "sw-resize", "s-resize", "w-resize", NULL};
static gchar *cs3_width[] = {"thin", "medium", "thick", NULL}; 

static Tcs3_arr cs3_arr[] = {
{"font-family", cs3_fonts, 0, but_none},
//{"font-stretch", cs3_font_stretches, 0, but_none}, 
// None of the major browsers support the font-stretch property.
{"font-style", cs3_font_styles, 1, but_none},
{"font-variant", cs3_font_variants, 1, but_none},
{"font-weight", cs3_font_weights, 1, but_none},
{"font-size", NULL, 0, but_none},
// {"font-size-adjust", cs3_font_size_adjustments, 0, but_none}, 
// Gecko only 
{"font", NULL, 0, but_none},
{"color", cs3_colors, 0, but_color},
{"background-color", cs3_colors, 0, but_color},
{"background-image", NULL, 0, but_file},//	<uri> | none | inherit  url("images/bg.png")
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
{"content", cs3_content, 0, but_none},
{"counter-increment", NULL, 0, but_none},
{"counter-reset", NULL, 0, but_none},
{"cursor", cs3_cursor, 0, but_none},
{"opacity", NULL, 0, but_none},
{"outline", NULL, 0, but_none},
{"outline-color", cs3_colors, 0, but_color},
{"outline-style", cs3_border_styles, 0, but_none},
{"outline-width", cs3_width, 0, but_none},
{"quotes", NULL, 0, but_none},
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
	g_slice_free(Tcs3_diag, diag);
}

static void cs3d_cancel_clicked_lcb(GtkWidget * widget, Tcs3_diag *diag) {
	cs3d_destroy_lcb(NULL, diag);
}

static void cs3d_ok_clicked_lcb(GtkWidget * widget, Tcs3_diag *diag) {
	Tcs3_destination dest = diag->dest;
	GtkTreeIter iter;
	GString *retstr;
	retstr = g_string_new("");

	if (diag->styletype == onestyle) {
		gboolean retval = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(diag->lstore), &iter);
		while (retval) {
			gchar *text[3];
			gtk_tree_model_get(GTK_TREE_MODEL(diag->lstore), &iter, 1, &text[1], 2, &text[2], -1);
			retstr = g_string_append(retstr, text[1]);
			retstr = g_string_append(retstr, ": ");
			retstr = g_string_append(retstr, text[2]);
			retstr = g_string_append(retstr, "; ");
			retval = gtk_tree_model_iter_next(GTK_TREE_MODEL(diag->lstore), &iter);
		}
	} else { /* multistyle */
		gchar *prev_selector = NULL;
		gboolean retval = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(diag->lstore), &iter);
		while (retval) {
			gchar *text[3];
			gtk_tree_model_get(GTK_TREE_MODEL(diag->lstore), &iter, 0, &text[0], 1, &text[1], 2, &text[2], -1);
			if (!prev_selector) {
				prev_selector = g_strdup(text[0]);
				retstr = g_string_append(retstr, text[0]);
				retstr = g_string_append(retstr, " {\n");
			} else if (g_strcmp0(prev_selector, text[0]) != 0 ) {
				retstr = g_string_append(retstr, "}\n");
				retstr = g_string_append(retstr, text[0]);
				retstr = g_string_append(retstr, " {\n");
				g_free(prev_selector);
				prev_selector = g_strdup(text[0]);
			}
			retstr = g_string_append(retstr, "\t");
			retstr = g_string_append(retstr, text[1]);
			retstr = g_string_append(retstr, ": ");
			retstr = g_string_append(retstr, text[2]);
			retstr = g_string_append(retstr, ";\n");
			retval = gtk_tree_model_iter_next(GTK_TREE_MODEL(diag->lstore), &iter);
		}
		g_free(prev_selector);
		/* only append '}\n' in case there is content */
		if (retstr->len > 0) {
			retstr = g_string_append(retstr, "}\n");
		}
	}

	if (retstr->len > 0) {
		if (dest.dest_type == entry) {
			DEBUG_MSG("cs3d_ok_clicked_lcb, entry? %p\n", dest.entry);
			if (dest.entry && GTK_IS_WIDGET(dest.entry)) {
				DEBUG_MSG("cs3d_ok_clicked_lcb, entry!\n");
				gtk_entry_set_text(GTK_ENTRY(dest.entry), retstr->str);
			}
		} else if (dest.dest_type == textbox) {
			if (dest.doc) {
				DEBUG_MSG("cs3d_ok_clicked_lcb, textbox!\n");
				if (dest.doc_start != -1 || dest.doc_end != -1) {
					doc_replace_text(dest.doc, retstr->str, dest.doc_start, dest.doc_end);
				} else {
					doc_insert_two_strings(dest.doc, retstr->str, NULL);
				}
			}
		} else if (dest.dest_type == wizard) {
			GtkTextBuffer *buf;
			buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(dest.entry));
			gtk_text_buffer_set_text(buf, retstr->str, -1);
		} else {
#ifdef DEVELOPMENT
			g_print("cs3d_ok_clicked_lcb, an unknown dest type, this should never happen!\n");
#endif
		}
	}
	g_string_free(retstr, TRUE);
	cs3d_destroy_lcb(NULL, diag);
}

static void cs3d_selection_changed_cb(GtkTreeSelection *selection, gpointer data) 
{
	Tcs3_diag *diag = data;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *text[3] = {NULL, NULL, NULL};
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gint *indices;
		GtkTreePath *path;
		gtk_tree_model_get(model, &iter, 0, &text[0], 1, &text[1], 2, &text[2], -1);
		
		/* now set the diag->selected_row !!!!!!!!!!!!!!!!!!! */
		path = gtk_tree_model_get_path(model, &iter);
		indices = gtk_tree_path_get_indices(path);
		DEBUG_MSG("cs3d_selection_changed_cb, selected row=%d\n",indices[0]);
		diag->selected_row = indices[0];
		gtk_tree_path_free(path);
	}
	if (diag->styletype == multistyle) {
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(diag->selector))), text[0]?text[0]:"");
	}
	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(diag->property))), text[1]?text[1]:"");
	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(diag->value))), text[2]?text[2]:"");
	
	
}

static void add_to_row(Tcs3_diag *diag, gint whichrow) {
	gchar *text[3] = {NULL, NULL, NULL};
	gboolean correct=TRUE;
	gint count=1;
	if (diag->styletype == multistyle) {
		text[0] = gtk_editable_get_chars(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(diag->selector))), 0, -1);
		count=0;
	}
	text[1] = gtk_editable_get_chars(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(diag->property))), 0, -1);
	text[2] = gtk_editable_get_chars(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(diag->value))), 0, -1);

	while (count < 3) {
		if (strlen(text[count]) == 0) {
			DEBUG_MSG("add_to_row, %s:%s:%s: %d %s is incorrect, will not add\n",text[0],text[1], text[2], count, text[count]);
			correct = FALSE;
			break;
		}
		count++;
	}
	if (correct) {
		GtkTreeIter iter;
		if (whichrow == -1) {
			gtk_list_store_append(GTK_LIST_STORE(diag->lstore),&iter);
		} else {
			gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(diag->lstore),&iter,NULL,whichrow);
		}
		gtk_list_store_set(GTK_LIST_STORE(diag->lstore), &iter, 0, text[0], 1, text[1], 2, text[2], -1);
	}
	g_free(text[0]);
	g_free(text[1]);
	g_free(text[2]);
}

static void cs3d_add_clicked_lcb(GtkWidget * widget, Tcs3_diag *diag) {
	add_to_row(diag, -1);
}

static void cs3d_del_clicked_lcb(GtkWidget * widget, Tcs3_diag *diag) {
	if (diag->selected_row != -1) {
		GtkTreeIter iter;
		gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(diag->lstore),&iter,NULL,diag->selected_row); 
		gtk_list_store_remove(GTK_LIST_STORE(diag->lstore),&iter);
		/*gtk_clist_remove(GTK_CLIST(diag->clist), diag->selected_row);*/
		diag->selected_row = -1;
	}
}

static void cs3d_update_clicked_lcb(GtkWidget * widget, Tcs3_diag *diag) {
	if (diag->selected_row != -1) {
		DEBUG_MSG("cs3d_update_clicked_lcb, updating row=%d\n", diag->selected_row);
		add_to_row(diag, diag->selected_row);
	}
}

static void cs3d_html5_clicked_lcb(GtkWidget * widget, Tcs3_diag *diag) {
	GList *tmplist, *list;
	list = glist_with_html_tags(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(diag->html5)));
#if GTK_CHECK_VERSION(3,0,0)
			gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(diag->selector));
#else
			GtkListStore *store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(diag->selector)));
  			gtk_list_store_clear(store);
#endif
	for (tmplist=g_list_first(list);tmplist;tmplist=g_list_next(tmplist)) {
		if (tmplist->data) {
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(diag->selector),tmplist->data);
		}
	}
	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(diag->selector))), "");
	g_list_free(list);
	g_list_free(tmplist);
	tmplist = NULL;
}

static void cs3d_prop_activate_lcb(GtkWidget * widget, Tcs3_diag *diag) {
	Tcs3_arr *tmp;
	gchar *tmpstr;

	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(diag->value))), "");
#if GTK_CHECK_VERSION(3,0,0)
			gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(diag->value));
#else
			GtkListStore *store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(diag->value)));
  			gtk_list_store_clear(store);
#endif
	tmpstr = gtk_editable_get_chars(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(diag->property))), 0, -1);
	tmp = cs3_arr_from_property(tmpstr);
	g_free(tmpstr);
	if (tmp) {
		GList *list, *tmplist;
		list = pointer_arr2glist(tmp->possibilities);
		if (list) {
			gchar *tmpstr2=NULL;
			if (!tmp->force_pos) {
					tmpstr2 = gtk_editable_get_chars(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(diag->value))), 0, -1);
			}

			for (tmplist=g_list_first(list);tmplist;tmplist=g_list_next(tmplist)) {
				if (tmplist->data) {
					gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(diag->value),tmplist->data);
				}
			}
			g_list_free(list);
			if (tmpstr2) {
				gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(diag->value))), tmpstr2);
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
		gtk_editable_set_editable(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(diag->value))), !tmp->force_pos);
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
	GtkTreeSelection *selection;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	
	diag = g_slice_new(Tcs3_diag);
	diag->win = window_full2(_("Cascading Style Sheet Builder"), GTK_WIN_POS_CENTER_ON_PARENT, 
			12, G_CALLBACK(cs3d_destroy_lcb), diag, TRUE, transient_win);
	gtk_window_set_role(GTK_WINDOW(diag->win), "css");
	diag->dest = dest;
	diag->styletype = style;
	diag->grab = grab;
	diag->selected_row = -1;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(diag->win),vbox);
	
	table = gtk_table_new(3, 6, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 12);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
	tmplist = NULL;
	
	if (diag->styletype == multistyle) {
		tmplist = glist_with_html_tags(0);
		diag->selector = combobox_with_popdown("", tmplist,1);
		dialog_mnemonic_label_in_table(_("_Selector(s):"), diag->selector, table, 0, 1, 0, 1);
		gtk_table_attach_defaults(GTK_TABLE(table), diag->selector, 1 ,5 , 0, 1);
		gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(diag->selector), 5);
		gtk_combo_box_set_add_tearoffs(GTK_COMBO_BOX(diag->selector), 1);

		diag->html5 = gtk_check_button_new_with_mnemonic("_html 5");
		gtk_table_attach_defaults(GTK_TABLE(table), diag->html5, 5,6, 0,1);
		g_signal_connect(diag->html5, "clicked", G_CALLBACK(cs3d_html5_clicked_lcb), diag);
		
		g_list_free(tmplist);
		tmplist = NULL;
	} 

	tmp = cs3_arr[count];
	while (tmp.property) {
		tmplist = g_list_append(tmplist, tmp.property);
		count++;
		tmp = cs3_arr[count];
	}
	diag->property = combobox_with_popdown("", tmplist,1);
	g_list_free(tmplist);
	tmplist = NULL;
	g_signal_connect(gtk_bin_get_child(GTK_BIN(diag->property)), "activate", G_CALLBACK(cs3d_prop_activate_lcb), diag);
	g_signal_connect(gtk_bin_get_child(GTK_BIN(diag->property)), "changed", G_CALLBACK(cs3d_prop_activate_lcb), diag);

	diag->value = combobox_with_popdown("", tmplist,1);
	dialog_mnemonic_label_in_table(_("_Property:"), diag->property, table, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), diag->property, 1, 5, 1, 2);
	gtk_combo_box_set_wrap_width(GTK_COMBO_BOX(diag->property), 4);
	gtk_combo_box_set_add_tearoffs(GTK_COMBO_BOX(diag->property), 1);
	dialog_mnemonic_label_in_table(_("_Value:"), diag->value, table, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(table), diag->value, 1, 4, 2, 3);

	
	gtk_widget_realize(diag->win);


	diag->extra_but = color_but_new(GTK_WIDGET(gtk_bin_get_child(GTK_BIN(diag->value))), diag->win);
	gtk_table_attach(GTK_TABLE(table), diag->extra_but, 4, 5, 2, 3, GTK_EXPAND, GTK_EXPAND, 0, 0);

	/* the list widget and the buttons are in a horizontal box */
	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 18);
	
	diag->lstore = gtk_list_store_new(3, G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(diag->lstore),0,GTK_SORT_ASCENDING);
	diag->lview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(diag->lstore));
	g_object_unref(G_OBJECT(diag->lstore));
	if (diag->styletype == multistyle) {
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes (_("Selector(s)"),renderer,"text", 0,NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(diag->lview), column);
	}
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (_("Property"),renderer,"text", 1,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(diag->lview), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (_("Value"),renderer,"text", 2,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(diag->lview), column);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(diag->lview));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	g_signal_connect(G_OBJECT(selection), "changed",G_CALLBACK(cs3d_selection_changed_cb),diag);
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(scrolwin, 400, 300);
	gtk_box_pack_start(GTK_BOX(hbox), scrolwin, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(scrolwin), diag->lview);
	/*if (diag->styletype == multistyle) {
		gchar *titles[] = {_("Selector"), _("Property"), _("Value"), NULL};
		diag->clist = gtk_clist_new_with_titles(3, titles);
	} else {
		gchar *titles[] = {_("Property"), _("Value"), NULL};
		diag->clist = gtk_clist_new_with_titles(2, titles);
	}
	gtk_clist_set_sort_column(GTK_CLIST(diag->clist), 0);
	gtk_clist_set_auto_sort(GTK_CLIST(diag->clist), TRUE);

	g_signal_connect(diag->clist, "select_row", G_CALLBACK(cs3d_select_row_lcb), diag);
	g_signal_connect(diag->clist, "unselect_row", G_CALLBACK(cs3d_unselect_row_lcb), diag);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolwin), diag->lview);
	*/
	
	vbox2 = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 0);
	
	but = gtk_button_new_with_mnemonic(_(" _Add "));
	g_signal_connect(but, "clicked", G_CALLBACK(cs3d_add_clicked_lcb), diag);
	gtk_box_pack_start(GTK_BOX(vbox2), but, FALSE, FALSE, 0);

	but = gtk_button_new_with_mnemonic(_(" _Update "));
	g_signal_connect(but, "clicked", G_CALLBACK(cs3d_update_clicked_lcb), diag);
	gtk_box_pack_start(GTK_BOX(vbox2), but, FALSE, FALSE, 0);
	
	but = gtk_button_new_with_mnemonic(_(" _Delete "));
	g_signal_connect(but, "clicked", G_CALLBACK(cs3d_del_clicked_lcb), diag);
	gtk_box_pack_start(GTK_BOX(vbox2), but, FALSE, FALSE, 0);

	/* the ok and cancel button are in a horizontal box below */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 12);	
#if GTK_CHECK_VERSION(3,0,0)
	hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
	hbox = gtk_hbutton_box_new();
#endif
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(hbox), 12);
	
	but = bf_stock_cancel_button(G_CALLBACK(cs3d_cancel_clicked_lcb), diag);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 0);
	but = bf_stock_ok_button(G_CALLBACK(cs3d_ok_clicked_lcb), diag);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);	
	
	gtk_widget_show_all(diag->win);
	
	cs3d_prop_activate_lcb(NULL, diag);
		
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
		gchar *text[3] = {NULL, NULL, NULL};
		
		DEBUG_MSG("css_parse, onestyle\n");
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
				GtkTreeIter iter;
				gtk_list_store_append(diag->lstore, &iter);
				gtk_list_store_set(diag->lstore,&iter,1, text[0],2,text[1],-1);
				/*gtk_clist_append(GTK_CLIST(diag->clist), text);*/
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
				GtkTreeIter iter;
				gtk_list_store_append(diag->lstore, &iter);
				gtk_list_store_set(diag->lstore,&iter,0,text[0],1, text[1],2,text[2],-1);

				/*gtk_clist_append(GTK_CLIST(diag->clist), text);*/
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
	/*gtk_clist_sort(GTK_CLIST(diag->clist));*/
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

	diag = css_diag(dest, multistyle, bfwin->main_window, FALSE);
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
	gtk_box_pack_start(GTK_BOX(hbox), gtk_image_new_from_stock(BF_STOCK_CSS_SMALL, GTK_ICON_SIZE_BUTTON),FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new_with_mnemonic(_("_Style...")), TRUE, TRUE, 3);
	gtk_container_add(GTK_CONTAINER(style_but), hbox);
	g_signal_connect(style_but, "clicked", G_CALLBACK(style_but_clicked_lcb), which_entry);
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
	gtk_box_pack_start(GTK_BOX(hbox), gtk_image_new_from_stock(BF_STOCK_CSS_SMALL, GTK_ICON_SIZE_BUTTON),FALSE, FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new_with_mnemonic(_("_Style...")), TRUE, TRUE, 6);
	gtk_container_add(GTK_CONTAINER(style_but), hbox);
	g_signal_connect(style_but, "clicked", G_CALLBACK(style_but_for_wizard_clicked_lcb), textview);
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
	/* the dialog is dual functional: it can be called directly from the text, and it can be called by return_color 
	if called by return_color it is in the modal state */
	gint is_modal;
	/* for the modal state */
	gchar *returnval;

	/* for the non-modal state */
	gint startpos;
	gint endpos;
	Tbfwin *bfwin;
} Tcolsel;

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

	tmpstr = gdk_color_to_hexstring(&gcolor, FALSE);
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
	gtk_color_selection_set_has_palette(GTK_COLOR_SELECTION(csd->csel), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), csd->csel, TRUE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 12);
#if GTK_CHECK_VERSION(3,0,0)
	hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
	hbox = gtk_hbutton_box_new();
#endif
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(hbox), 12);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

	but = bf_stock_cancel_button(G_CALLBACK(colsel_cancel_clicked_lcb), csd);
	gtk_box_pack_start(GTK_BOX(hbox), but, TRUE, TRUE, 0);
	but = bf_stock_ok_button(G_CALLBACK(colsel_ok_clicked_lcb), csd);
	gtk_window_set_default(GTK_WINDOW(csd->win), but);
	gtk_box_pack_start(GTK_BOX(hbox), but, TRUE, TRUE, 0);

	if (bfwin && bfwin->session->colorlist) {
		GtkSettings* gtksettings;
		/* Note that this function can only be called when the GtkWidget is attached to a toplevel, since the settings object is specific to a particular GdkScreen.  */
		gtksettings = gtk_widget_get_settings(GTK_WIDGET(csd->csel));
		if (gtksettings) {
			gchar *strings;
			DEBUG_MSG("pallette list=%d\n",g_list_length(bfwin->session->colorlist));
			bfwin->session->colorlist = limit_stringlist(bfwin->session->colorlist, 20, TRUE);
			DEBUG_MSG("pallette list=%d\n",g_list_length(bfwin->session->colorlist));
			strings = stringlist_to_string(bfwin->session->colorlist, ":");
			strings[strlen(strings)-1] = '\0';
			/* this property may contain max 20 colors, otherwise gtk will crash */
			g_object_set(G_OBJECT(gtksettings), "gtk-color-palette", strings, NULL);
			g_free(strings);
		} 
/*		DEBUG_MSG("colsel_dialog, setting palette from %s\n", strings);
		if (gtk_color_selection_palette_from_string(strings, &gcolorarr, &num)) {
			DEBUG_MSG("num=%d, gcolorarr=%p\n",num,gcolorarr);
		} else {
			DEBUG_MSG("hmm, failed to parse our string :(\n");
		} */
	}

	gtk_widget_show_all(csd->win);

	DEBUG_MSG("colsel_dialog, finished\n");
	return csd;
}

void edit_color_dialog(Tdocument *doc, gchar *color, gint startpos, gint endpos) {
	colsel_dialog(doc->bfwin, color, FALSE, startpos, endpos);
}

void sel_colour_cb(GtkWidget *widget, Tbfwin *bfwin) {
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

	colsel_dialog(bfwin, tmpstr, 0, startpos, endpos);
	if (tmpstr)
		g_free(tmpstr);
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
	g_signal_connect(color_but, "clicked", G_CALLBACK(color_but_clicked), which_entry);
	gtk_widget_show(color_but);
	return color_but;
}
