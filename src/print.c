/*
* print.c - bluefish print layout and rendering
* Copyright (C) 2012 Olivier Sessink
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
#include <math.h>				/* log10() */

#include "bluefish.h"
#include "document.h"
#include "bf_lib.h"
#include "print.h"

#ifdef DEVELOPMENT

typedef struct {
	guint byte_o;
	guint char_o;
} Tpage;

typedef struct {
	gchar *buffer;
	Tdocument *doc;
	gint so;
	gint eo;
	GSList *pages;
	gint singlecharwidth;
	gint numlinecount;
	gint marginsize;
	gint headersize;
} Tbluefishprint;

static GtkPrintSettings *printsettings=NULL;

static void
add_tag(Tbluefishprint *bfprint, PangoAttrList *alist, GtkTextTag *tag, guint so, guint eo)
{
	PangoAttribute *attr;
	gboolean bgset,fgset,boldset,styleset;
	guint byte_so, byte_eo;
	
	byte_so = utf8_charoffset_to_byteoffset_cached(bfprint->buffer, so);
	byte_eo = utf8_charoffset_to_byteoffset_cached(bfprint->buffer, eo);
	
	g_object_get(tag,"background-set", &bgset,"foreground-set", &fgset,"style-set", &styleset,"weight-set", &boldset,NULL);
	if (bgset) {
		GdkColor *color;
		g_object_get(tag, "background-gdk", &color, NULL);
		attr = pango_attr_background_new(color->red, color->green, color->blue);
		gdk_color_free (color);
		attr->start_index = byte_so;
		attr->end_index = byte_eo;
		pango_attr_list_insert(alist, attr);
				
	}
	if (fgset) {
		GdkColor *color;
		g_object_get(tag, "foreground-gdk", &color, NULL);
		attr = pango_attr_foreground_new(color->red, color->green, color->blue);
		gdk_color_free (color);
		attr->start_index = byte_so;
		attr->end_index = byte_eo;
		pango_attr_list_insert(alist, attr);
	}
	if (boldset) {
		attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
		attr->start_index = byte_so;
		attr->end_index = byte_eo;
		pango_attr_list_insert(alist, attr);
	}
	if (styleset) {
		attr = pango_attr_style_new(PANGO_STYLE_ITALIC);
		attr->start_index = byte_so;
		attr->end_index = byte_eo;
		pango_attr_list_insert(alist, attr);
	}
}

static void
apply_syntax(Tbluefishprint *bfprint, PangoLayout *layout, Tpage *page_s, Tpage *page_e)
{
	PangoAttrList *alist;
	GList *tmplist;
	GtkTextIter iter;
	
	alist = pango_attr_list_new();
	
	tmplist = g_list_first(BLUEFISH_TEXT_VIEW(bfprint->doc->view)->bflang->tags);
	while (tmplist) {
		GtkTextTag *tag = tmplist->data;
		gboolean donewithtag = FALSE;
		
		g_print("apply_syntax, try tag %p, get iter at offset %d\n",tag, page_s->char_o);
		gtk_text_buffer_get_iter_at_offset(bfprint->doc->buffer, &iter, page_s->char_o);
		if (gtk_text_iter_begins_tag(&iter, tag) || gtk_text_iter_has_tag(&iter, tag)) {
			guint char_eo, char_so;
			char_so = gtk_text_iter_get_offset(&iter);
			gtk_text_iter_forward_to_tag_toggle(&iter, tag);
			char_eo = gtk_text_iter_get_offset(&iter);
			if (char_eo > page_e->char_o) {
				char_eo = page_e->char_o;
				donewithtag=TRUE;
			}
			add_tag(bfprint, alist, tag, char_so-page_s->char_o, char_eo-page_s->char_o);
		}
		while (!donewithtag && gtk_text_iter_forward_to_tag_toggle(&iter, tag)) {
			guint char_eo, char_so;
			char_so = gtk_text_iter_get_offset(&iter);
			g_print("apply_syntax, found tag %p at offset %d\n",tag,char_so);
			gtk_text_iter_forward_to_tag_toggle(&iter, tag);
			char_eo = gtk_text_iter_get_offset(&iter);
			g_print("apply_syntax, found end of tag %p at offset %d, page_end at %d\n",tag,char_eo, page_e->char_o);
			if (char_eo >= page_e->char_o) {
				char_eo = page_e->char_o;
				donewithtag=TRUE;
			}
			add_tag(bfprint, alist, tag, char_so-page_s->char_o, char_eo-page_s->char_o);
		}

		tmplist = g_list_next(tmplist);
	} 
	
	pango_layout_set_attributes(layout,alist);
	pango_attr_list_unref(alist);

	g_print("apply_syntax, done\n");
	
}

static void
set_pango_defaults(Tbluefishprint *bfprint, GtkPrintContext *context,PangoLayout *layout)
{
	gdouble width;
	PangoFontDescription *desc;
	PangoTabArray *tab_array;
	
	width = pango_units_from_double(gtk_print_context_get_width(context));
	
	desc = pango_font_description_from_string("monospace 10");
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);

	if (bfprint->singlecharwidth == 0) {
		/* calculate the size of a space */
		pango_layout_set_text(layout, "W", -1);
		pango_layout_get_size(layout, &bfprint->singlecharwidth, NULL);
	}
	bfprint->marginsize = (1 + log10(bfprint->numlinecount)) * bfprint->singlecharwidth;
	pango_layout_set_width(layout, width - bfprint->marginsize);	

	tab_array = pango_tab_array_new (1, FALSE);
	pango_tab_array_set_tab(tab_array,0,PANGO_TAB_LEFT,bfprint->singlecharwidth* BFWIN(bfprint->doc->bfwin)->session->editor_tab_width);
	pango_layout_set_tabs(layout, tab_array);
	pango_tab_array_free(tab_array);
}

static void
draw_line_numbers(Tbluefishprint *bfprint, GtkPrintContext *context, PangoLayout *layout, cairo_t *cr, Tpage *page_s)
{
	GtkTextIter iter;
	gint nextline_o=-1;
	guint nextline=1;
	gint blineoffset=-1;
	PangoLayoutIter *pliter;
	PangoLayout *numberlayout;
	PangoFontDescription *desc;
	
	numberlayout = gtk_print_context_create_pango_layout(context);
	desc = pango_font_description_from_string("monospace 10");
	pango_layout_set_font_description(numberlayout, desc);
	pango_font_description_free(desc);
	
	pliter = pango_layout_get_iter(layout);
	gtk_text_buffer_get_iter_at_offset(bfprint->doc->buffer, &iter, page_s->char_o);
	do {
		guint byte_o;
		if (nextline_o == -1 && gtk_text_iter_starts_line(&iter)) {
			nextline_o = utf8_charoffset_to_byteoffset_cached(bfprint->buffer, 
						gtk_text_iter_get_offset(&iter));
			nextline = 1+gtk_text_iter_get_line(&iter);
			gtk_text_iter_forward_line(&iter);
		}
		byte_o = pango_layout_iter_get_index(pliter);
		g_print("draw_line_numbers byte_o=%d, nextline_o=%d\n",byte_o,nextline_o);
		if (byte_o+page_s->byte_o == nextline_o) {
			guint bline;
			gchar *tmpstr;
			bline = pango_layout_iter_get_baseline(pliter);
			tmpstr = g_strdup_printf("%d",nextline);
			pango_layout_set_text(numberlayout, tmpstr, -1);
			g_free(tmpstr);
			g_print("draw line number %d\n",nextline);
			if (blineoffset == -1) {
				blineoffset = pango_layout_get_baseline(numberlayout);
			} 
			cairo_move_to(cr, 0 ,pango_units_to_double(bline - blineoffset) );
			pango_cairo_show_layout(cr, numberlayout);
			nextline_o = -1;
		}
	} while (pango_layout_iter_next_line(pliter));
	pango_layout_iter_free(pliter);	
	
}

static void
draw_page(GtkPrintOperation *operation,GtkPrintContext *context,gint page_nr,Tbluefishprint *bfprint)
{
	PangoLayout *layout;
	cairo_t *cr;
	GSList *tmpslist;
	Tpage *page_s, *page_e;
	g_print("draw page %d\n",page_nr);
	cr = gtk_print_context_get_cairo_context(context);
	layout = gtk_print_context_create_pango_layout(context);
	set_pango_defaults(bfprint,context,layout);
	
	tmpslist = g_slist_nth(bfprint->pages, page_nr);
	page_s = (Tpage *)tmpslist->data;
	tmpslist = g_slist_next(tmpslist);
	g_assert(tmpslist);
	page_e = (Tpage *)tmpslist->data;
	g_print("page_s=%p, page_e=%p\n",page_s,page_e);
	g_print("page_s->char_o=%d, page_e->char_o=%d\n",page_s->char_o,page_e->char_o);

	pango_layout_set_text(layout, bfprint->buffer+page_s->byte_o, page_e->byte_o - page_s->byte_o);
	apply_syntax(bfprint, layout, page_s, page_e);
	
	draw_line_numbers(bfprint, context, layout, cr, page_s);
	
	cairo_move_to(cr, pango_units_to_double(bfprint->marginsize) ,	pango_units_to_double(bfprint->headersize) );
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);
}
static void
begin_print(GtkPrintOperation *print,GtkPrintContext *context,Tbluefishprint *bfprint)
{
	PangoLayout *layout;
	PangoLayoutIter *pliter;
	PangoRectangle prect;
	gdouble height, offset=0.0;
	Tpage curpage = {0,0};
	Tpage *page;
	gint i=0, pagenr=0;

	/* calculate the number of pages needed */
	height = pango_units_from_double(gtk_print_context_get_height(context));
	bfprint->headersize = 0;
	bfprint->numlinecount = gtk_text_buffer_get_line_count(bfprint->doc->buffer);
	
	layout = gtk_print_context_create_pango_layout(context);
	set_pango_defaults(bfprint,context,layout);

	bfprint->buffer = doc_get_chars(bfprint->doc, bfprint->so, bfprint->eo);
	utf8_offset_cache_reset();
	
	bfprint->pages = g_slist_append(bfprint->pages, g_slice_new0(Tpage));
	
	pango_layout_set_text(layout, bfprint->buffer, -1);
	pliter = pango_layout_get_iter(layout);
	while (pango_layout_iter_next_line(pliter)) { /* because it is very unlikely that the first line is 
											already beyond the page border, start looking from the second line */ 
		i++;
		pango_layout_iter_get_line_extents(pliter, &prect, NULL);
		if (prect.y + prect.height - offset > (height - bfprint->headersize)) {
			GtkTextIter iter;
			page = g_slice_new(Tpage);
			page->byte_o = pango_layout_iter_get_index(pliter);
			page->char_o = utf8_byteoffset_to_charsoffset_cached(bfprint->buffer, page->byte_o);
			curpage = *page;
			g_print("page %d should end at pango line %i, byte=%d, chars=%d\n",pagenr,i,page->byte_o, page->char_o);
			gtk_text_buffer_get_iter_at_offset(bfprint->doc->buffer, &iter, page->char_o);
			g_print("which is at editor line %d\n",gtk_text_iter_get_line(&iter));
			offset = prect.y + prect.height;
			pagenr++;
			bfprint->pages = g_slist_append(bfprint->pages, page);
		}
	}
	/* add the end of the last page */
	page = g_slice_new(Tpage);
	page->byte_o = strlen(bfprint->buffer);
	page->char_o = utf8_byteoffset_to_charsoffset_cached(bfprint->buffer, page->byte_o);
	bfprint->pages = g_slist_append(bfprint->pages, page);
	
	pango_layout_iter_free(pliter);
	
	gtk_print_operation_set_n_pages(print, pagenr+1);
	g_object_unref(layout);
}


void
doc_print(Tdocument *doc)
{
	
	GtkPrintOperation *print;
	gchar *jobname;
	GtkPrintOperationResult res;
	GError *gerror=NULL;
	Tbluefishprint bfprint;
	GSList *tmpslist;
	print = gtk_print_operation_new();

	if(printsettings != NULL)
		gtk_print_operation_set_print_settings(print, printsettings);
	
	bfprint.pages = NULL;
	bfprint.singlecharwidth=0;
	bfprint.doc = doc;
	if (!doc_get_selection(doc, &bfprint.so, &bfprint.eo)) {
		bfprint.so = 0;
		bfprint.eo = -1;
	}
	
	jobname = g_strconcat("Bluefish ", gtk_label_get_text(GTK_LABEL(doc->tab_label)), NULL);
	gtk_print_operation_set_job_name(print, jobname);
	g_free(jobname);
	
	g_signal_connect(print, "begin_print", G_CALLBACK(begin_print), &bfprint);
	g_signal_connect(print, "draw_page", G_CALLBACK(draw_page), &bfprint);

	res = gtk_print_operation_run(print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
					GTK_WINDOW(BFWIN(doc->bfwin)->main_window), &gerror);
	if (gerror) {
		g_print("print error %s\n",gerror->message);
		g_error_free(gerror);
	}
	g_print("doc_print, res=%d\n",res);
	if(res == GTK_PRINT_OPERATION_RESULT_APPLY) {
		if(printsettings != NULL)
			g_object_unref(printsettings);
			printsettings = g_object_ref(gtk_print_operation_get_print_settings(print));
	}
	g_object_unref(printsettings);
	
	g_free(bfprint.buffer);
	for (tmpslist=bfprint.pages;tmpslist;tmpslist=g_slist_next(tmpslist)) {
		g_slice_free(Tpage, tmpslist->data);
	}
}

#endif /* DEVELOPMENT */