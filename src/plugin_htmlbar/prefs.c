
#include "htmlbar.h"
#include "htmlbar_uimanager.h"
#include "../preferences.h"
#include "../gtk_easy.h"
#include "../dialog_utils.h"

typedef struct {
	GtkWidget *in_sidepanel;
	GtkWidget *transient_htdialogs;
	GtkWidget *lowercase_tags;
	GtkWidget *allow_dep;
	GtkWidget *format_by_context;
	GtkWidget *xhtml;
	GtkWidget *auto_update_meta_author;
	GtkWidget *auto_update_meta_date;
	GtkWidget *auto_update_meta_generator;
} Thtmlbarprefs;

Thtmlbarprefs *hbp = NULL;

void htmlbar_pref_apply()
{
	/*g_print("htmlbar_pref_apply\n");*/
	integer_apply(&htmlbar_v.in_sidepanel, hbp->in_sidepanel, TRUE);
	integer_apply(&htmlbar_v.transient_htdialogs, hbp->transient_htdialogs, TRUE);	
	integer_apply(&htmlbar_v.lowercase_tags, hbp->lowercase_tags, TRUE);
	integer_apply(&main_v->props.allow_dep, hbp->allow_dep, TRUE);
	integer_apply(&main_v->props.format_by_context, hbp->format_by_context, TRUE);

	integer_apply(&main_v->props.auto_update_meta_author, hbp->auto_update_meta_author, TRUE);
	integer_apply(&main_v->props.auto_update_meta_date, hbp->auto_update_meta_date, TRUE);
	integer_apply(&main_v->props.auto_update_meta_generator, hbp->auto_update_meta_generator, TRUE);
	
	if (!htmlbar_v.in_sidepanel) {
		DEBUG_MSG("htmlbar_pref_apply, in_sidepanel=%d, should we now show the htmlbar toolbar????\n", htmlbar_v.in_sidepanel);
		GList *tmplist = g_list_first(main_v->bfwinlist);
		while (tmplist) {
			Tbfwin *bfwin = BFWIN(tmplist->data);
			Thtmlbarwin *hbw;
			Thtmlbarsession *hbs;
			hbs = g_hash_table_lookup(htmlbar_v.lookup, bfwin->session);
			hbw = g_hash_table_lookup(htmlbar_v.lookup, bfwin);
			htmlbar_toolbar_show(hbw, hbs, hbs->view_htmlbar);
			tmplist = g_list_next(tmplist);
		}
	} else {
		DEBUG_MSG("htmlbar_pref_apply, in_sidepanel=%d, no gui to build, the complete sidepanel will be rebuilt by the main preferences apply function\n", htmlbar_v.in_sidepanel);
	}
	
}

void htmlbar_pref_initgui(GtkTreeStore *nstore, GtkTreeIter *pit, GSList **widgetfreelist)
{
	GtkWidget *frame, *vbox, *table, *vbox2;
	GtkTreeIter it;
	
	hbp = g_new0(Thtmlbarprefs, 1);
	
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	vbox = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	
	gtk_tree_store_append(nstore, &it, pit);
	gtk_tree_store_set(nstore, &it, NAMECOL, _("HTML Features"), WIDGETCOL, frame, -1);
	*widgetfreelist = g_slist_prepend(*widgetfreelist, frame);

	vbox2 = dialog_vbox_labeled(_("<b>HTML Toolbar</b>"), vbox);
	table = dialog_table_in_vbox_defaults(2, 1, 0, vbox2);
	hbp->in_sidepanel =
		dialog_check_button_in_table(_("Show toolbar in sidepanel"), htmlbar_v.in_sidepanel, table, 0, 1, 0,1);
	hbp->transient_htdialogs = dialog_check_button_in_table(_("Keep HTML dialogs always on top"),
															 htmlbar_v.transient_htdialogs, table, 0, 1, 1, 2);
	
	
	
	vbox2 = dialog_vbox_labeled(_("<b>HTML options</b>"), vbox);
	table = dialog_table_in_vbox_defaults(4, 1, 0, vbox2);

	hbp->lowercase_tags = dialog_check_button_in_table(_("Use lo_wercase HTML tags"),
															 htmlbar_v.lowercase_tags, table, 0, 1, 1, 2);
	hbp->allow_dep = dialog_check_button_in_table(_("Use de_precated tags (e.g. <font> and <nobr>)"),
														main_v->props.allow_dep, table, 0, 1, 2, 3);
	hbp->format_by_context =
		dialog_check_button_in_table(_
									 ("_Format according to accessibility guidelines (e.g. <strong> for <b>)"),
									 main_v->props.format_by_context, table, 0, 1, 3, 4);

	vbox2 = dialog_vbox_labeled(_("<b>Auto Update Tag Options</b>"), vbox);
	table = dialog_table_in_vbox_defaults(3, 1, 0, vbox2);

	hbp->auto_update_meta_author =
		dialog_check_button_in_table(_("Automatically update a_uthor meta tag"),
									 main_v->props.auto_update_meta_author, table, 0, 1, 0, 1);
	hbp->auto_update_meta_date =
		dialog_check_button_in_table(_("Automatically update _date meta tag"),
									 main_v->props.auto_update_meta_date, table, 0, 1, 1, 2);
	hbp->auto_update_meta_generator =
		dialog_check_button_in_table(_("Automatically update _generator meta tag"),
									 main_v->props.auto_update_meta_generator, table, 0, 1, 2, 3);

}
