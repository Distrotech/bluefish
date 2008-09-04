#include <string.h>
#include <gdk/gdkkeysyms.h>
#include "bftextview2_scanner.h"
#include "bftextview2_autocomp.h"


typedef struct {
	GtkWidget *win;
	GtkListStore *store;
	GtkTreeView *tree;
} Tacwin;

static void acwin_cleanup(Tacwin * acw) {
	gtk_widget_destroy(acw->win);
	g_free(acw);
}

static gboolean acwin_key_release_lcb(GtkWidget *widget,GdkEventKey *event,gpointer user_data)
{
	GtkDialog *dlg = GTK_DIALOG(((Tacwin *)user_data)->win);
	g_print("got keyval %c\n",event->keyval);
	if (event->keyval == GDK_Return) {
		gtk_dialog_response(dlg,GTK_RESPONSE_OK);
		return TRUE;
	} else if (event->keyval == GDK_Escape) {
		gtk_dialog_response(dlg,GTK_RESPONSE_CANCEL);
		return TRUE;
	} else {
		/* pass the input character to the text widget, and append the character to the prefix ?? */
	}
	return FALSE;
}

static Tacwin *acwin_create(void) {
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	GtkWidget *scroll, *vbar;
	GtkTreeModel *sortmodel;
	Tacwin * acw;
	
	acw = g_new0(Tacwin,1);
	
	acw->win = gtk_dialog_new();
	gtk_widget_set_app_paintable(acw->win, TRUE);
	gtk_window_set_resizable(GTK_WINDOW(acw->win), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER (acw->win), 0);
	gtk_window_set_decorated(GTK_WINDOW(acw->win),FALSE);
	gtk_dialog_set_has_separator(GTK_DIALOG(acw->win),FALSE);
	
	
	acw->store = gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_STRING);
	sortmodel = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL(acw->store));
	g_object_unref(acw->store);
	acw->tree = GTK_TREE_VIEW(gtk_tree_view_new_with_model(sortmodel));
	g_object_unref(sortmodel);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (sortmodel),1, GTK_SORT_ASCENDING);
	
	gtk_tree_view_set_headers_visible(acw->tree, FALSE);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	vbar = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(scroll));
	gtk_widget_set_size_request(vbar,10,-1);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
	cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", cell, "markup", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(acw->tree), column);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(acw->tree),FALSE);
/*	gtk_tree_view_set_search_column(GTK_TREE_VIEW(acw->tree),1);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(acw->tree),acwin_tree_search_lcb,prefix,NULL);*/
	
	/*g_signal_connect_swapped(GTK_WINDOW(acw->win),"expose-event",G_CALLBACK(ac_paint),acw->win);*/
	/*gtk_window_set_position (GTK_WINDOW(acw->win), GTK_WIN_POS_MOUSE);*/
	gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(acw->tree));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(acw->win)->vbox),scroll,TRUE,TRUE,0);
	gtk_widget_show_all(scroll);
	/*gtk_widget_set_size_request(GTK_WIDGET(acw->tree),100,200);*/
	gtk_widget_set_size_request(acw->win, 100, 200);
	g_signal_connect(G_OBJECT(acw->win),"key-release-event",G_CALLBACK(acwin_key_release_lcb),acw);

	gtk_widget_hide(GTK_DIALOG(acw->win)->action_area);
	return acw;
}

static void acwin_position_at_cursor(Tacwin *acwin, BluefishTextView *bt2) {
	GtkTextIter it;
	GdkRectangle rect;
	GdkScreen *screen;
	gint x,y;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(bt2));
	screen = gtk_widget_get_screen(GTK_WIDGET(bt2));
	
	gtk_text_buffer_get_iter_at_mark(buffer,&it,gtk_text_buffer_get_insert(buffer));
	gtk_text_view_get_iter_location(GTK_TEXT_VIEW(bt2),&it,&rect);
	gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(bt2), GTK_TEXT_WINDOW_TEXT, rect.x, rect.y,&rect.x, &rect.y);
	gdk_window_get_origin(gtk_text_view_get_window(GTK_TEXT_VIEW(bt2),GTK_TEXT_WINDOW_TEXT),&x,&y);
	
	gtk_window_move(GTK_WINDOW(acwin->win),rect.x+x ,rect.y+y);
}

static void acwin_fill_tree(Tacwin *acw, GList *items) {
	GList *tmplist;
	tmplist = g_list_first(items);
	while (tmplist)	{
		GtkTreeIter it;
		gtk_list_store_append(acw->store,&it);
		gtk_list_store_set(acw->store,&it,0,tmplist->data,-1);
		tmplist = g_list_next(tmplist);
	}
}
/* this function works for words, but not for other constructs in programming 
languages such as things that start with < or with $ or *.
should be improved. possibly we need a custom function for different types of languages,
or some special per-language configuration of this function */
static gchar *autocomp_get_prefix_at_location(GtkTextBuffer *buffer, GtkTextIter *location) {
	GtkTextIter start;
	start = *location;
	gtk_text_iter_backward_word_start(&start);
	return gtk_text_buffer_get_text(buffer,&start,location,TRUE);
}

void autocomp_run(BluefishTextView *bt2) {
	Tcontext *context;
	GtkTextIter iter;
	GtkTextBuffer *buffer;
	
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(bt2));
	gtk_text_buffer_get_iter_at_mark(buffer,&iter,gtk_text_buffer_get_insert(buffer)); 
	/* first find the context at the current location, and see if there are any autocompletion
	items in this context */
	context = get_context_at_position(bt2, &iter);
	if (context && context->ac) {
		gchar *prefix;
		/* get the prefix, see if it results in any autocompletion possibilities */
		prefix = autocomp_get_prefix_at_location(buffer, &iter);
		g_print("found autocompletion prefix %s\n",prefix);
		if (prefix && *prefix != '\0') {
			gchar *newprefix;
			GList *items;
			items = g_completion_complete(context->ac,prefix,&newprefix);
			g_print("got %d autocompletion items, newprefix=%s\n",g_list_length(items),newprefix);
			if (items) {
				Tacwin * acw;
				/* create the GUI and run */
				acw = acwin_create();
				acwin_fill_tree(acw, items);
				acwin_position_at_cursor(acw,bt2);
				if (gtk_dialog_run(GTK_DIALOG(acw->win)) ==GTK_RESPONSE_OK ) {
					GtkTreeSelection *selection;
					GtkTreeIter it;
					GtkTreeModel *model;
					g_print("autocomp_run, do something\n");
					selection = gtk_tree_view_get_selection(acw->tree);
					if ( selection && gtk_tree_selection_get_selected(selection,&model,&it)) {
						gchar *string;
						gtk_tree_model_get(model,&it,0,&string,-1);
						g_print("got string %s\n",string);
						gtk_text_buffer_insert_at_cursor(buffer,string+strlen(prefix),-1);
					}
				}
				acwin_cleanup(acw);
			}
		}
	} else {
		g_print("no autocompletion data for context %p\n",context);
	}
}
