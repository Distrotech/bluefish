/*
* Copyright (C) 
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.	If not, see <http://www.gnu.org/licenses/>.
*
*/
#include <gtk/gtk.h>

#include "bluefish.h"
#include "document.h"
#include "print.h"

#ifdef DEVELOPMENT

static void
apply_syntax(Tdocument *doc, PangoLayout *layout)
{
	PangoAttrList *alist;
	PangoAttribute *attr;
	alist = pango_attr_list_new();
	
	attr = pango_attr_foreground_new(65534, 0, 0);
	attr->start_index = 20;
	attr->end_index = 40;
	pango_attr_list_insert(alist, attr);
	
	pango_layout_set_attributes(layout,alist);
	pango_attr_list_unref(alist);
}

static void
draw_page(GtkPrintOperation *operation,GtkPrintContext *context,gint page_nr,gpointer user_data)
{
	cairo_t *cr;
	PangoLayout *layout;
	gdouble width, text_height;
	gint layout_height;
	PangoFontDescription *desc;
	Tdocument *doc = user_data;
	gchar *text;
	g_print("draw page %d\n",page_nr);
	cr = gtk_print_context_get_cairo_context (context);
	width = gtk_print_context_get_width (context);

	cairo_set_source_rgb(cr, 0, 0, 0);
/*	cairo_rectangle(cr, 0, 0, width, 10);
	cairo_fill (cr);
*/
	layout = gtk_print_context_create_pango_layout(context);

	desc = pango_font_description_from_string("monospace 10");
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);
	
	pango_layout_set_width(layout, width * PANGO_SCALE);
	pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);

	text = doc_get_chars(doc, 0, -1);
	pango_layout_set_text(layout, text, -1);
	g_free(text);
	
	apply_syntax(doc, layout);

	pango_layout_get_size(layout, NULL, &layout_height);
	text_height = (gdouble)layout_height / PANGO_SCALE;

	cairo_move_to(cr, 10 /* margin */,	10 /* top margin */ );
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);
}
static void
begin_print(GtkPrintOperation *print,GtkPrintContext *context,Tdocument *doc)
{
	cairo_t *cr;
	PangoLayout *layout;
	gdouble width, text_height, height;
	gint layout_height;
	PangoFontDescription *desc;
	gchar *text;

	/* calculate the number of pages needed */
	/*cr = gtk_print_context_get_cairo_context(context);*/
	width = gtk_print_context_get_width(context);
	height = gtk_print_context_get_height(context);
	
	layout = gtk_print_context_create_pango_layout(context);

	desc = pango_font_description_from_string("monospace 10");
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);
	
	pango_layout_set_width(layout, width * PANGO_SCALE);
	pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);

	text = doc_get_chars(doc, 0, -1);
	pango_layout_set_text(layout, text, -1);
	g_free(text);
	
	pango_layout_get_size(layout, NULL, &layout_height);
	text_height = (gdouble)layout_height / PANGO_SCALE;

	g_print("text_height=%f, page height=%f, need %d pages\n",text_height,height,(int)(text_height/height+1));
	gtk_print_operation_set_n_pages(print, (int)(text_height/height+1));
}


void
doc_print(Tdocument *doc)
{
	
	GtkPrintOperation *print;
	GtkPrintOperationResult res;
	GError *gerror=NULL;
	print = gtk_print_operation_new();

	/*if(settings != NULL)
		gtk_print_operation_set_print_settings(print, settings);*/
	
	g_signal_connect(print, "begin_print", G_CALLBACK(begin_print), doc);
	g_signal_connect(print, "draw_page", G_CALLBACK(draw_page), doc);

	res = gtk_print_operation_run(print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
					GTK_WINDOW(BFWIN(doc->bfwin)->main_window), &gerror);
	if (gerror) {
		g_print("print error %s\n",gerror->message);
		g_error_free(gerror);
	}
	g_print("doc_print, res=%d\n",res);
	if(res == GTK_PRINT_OPERATION_RESULT_APPLY)
		{
/*			if(settings != NULL)
				g_object_unref(settings);
			settings = g_object_ref(gtk_print_operation_get_print_settings(print));*/
		}
	g_object_unref(print);
}

#endif /* DEVELOPMENT */