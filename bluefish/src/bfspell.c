#include "config.h"

#ifdef HAVE_LIBASPELL
#include <aspell.h>
#include <gtk/gtk.h>

#include "bluefish.h"
#include "gtk_easy.h"
#include "bfspell.h"
/*
 * indent -ts4 -kr
 */

typedef struct {
	AspellConfig *spell_config;
	AspellSpeller *spell_checker;
	GtkWidget *win;
	GtkWidget *lang;
	GtkWidget *dict;
	GtkWidget *runbut;
	GtkWidget *repbut;
	GtkWidget *ignbut;
	GtkWidget *in_doc;
	GtkWidget *in_sel;
	/* during the checking */
	GtkWidget *incorrectword;
	GtkWidget *suggestions;
	Tdocument *doc;
	gint offset;
	GtkTextMark* so;
	GtkTextMark* eo;
} Tbfspell;

Tbfspell bfspell = { NULL, NULL, NULL,NULL, NULL, NULL,NULL, NULL,NULL,NULL, NULL,NULL, NULL,0,NULL,NULL};

/* return value should be freed by the calling function */
gchar *doc_get_next_word(GtkTextIter *itstart, GtkTextIter *itend) {
	gboolean havestart=FALSE;
	gchar *retval;
	
	gtk_text_buffer_get_iter_at_offset(bfspell.doc->buffer,itstart,bfspell.offset);
	havestart = gtk_text_iter_starts_word(itstart);
	while (!havestart) {
		if (!gtk_text_iter_forward_char(itstart)) {
			return NULL;
		}
		havestart = gtk_text_iter_starts_word(itstart);
	}
	*itend = *itstart;
	gtk_text_iter_forward_word_end(itend);
	
	retval = gtk_text_buffer_get_text(bfspell.doc->buffer,itstart,itend,FALSE);
	bfspell.offset = gtk_text_iter_get_offset(itend);
	return retval;
}

void spell_add_to_session(gboolean to_session, const gchar * word)
{
	DEBUG_MSG("spell_add_to_session, to_session=%d, word=%s\n",to_session,word);
	if (to_session) {
		aspell_speller_add_to_session(bfspell.spell_checker, word, -1);
	} else {
		aspell_speller_add_to_personal(bfspell.spell_checker, word, -1);
	}

}

gboolean spell_check_word(gchar * tocheck, GtkTextIter *itstart, GtkTextIter *itend)
{
	int correct = aspell_speller_check(bfspell.spell_checker, tocheck, -1);
	DEBUG_MSG("word '%s' has correct=%d\n",tocheck,correct);
	if (!correct) {
		AspellWordList *awl = aspell_speller_suggest(bfspell.spell_checker, tocheck,-1);
		if (!bfspell.so || !bfspell.eo) {
			bfspell.so = gtk_text_buffer_create_mark(bfspell.doc->buffer,NULL,itstart,TRUE);
			bfspell.eo = gtk_text_buffer_create_mark(bfspell.doc->buffer,NULL,itend,FALSE);
		} else {
			gtk_text_buffer_move_mark(bfspell.doc->buffer,bfspell.so,itstart);
			gtk_text_buffer_move_mark(bfspell.doc->buffer,bfspell.eo,itend);
		}
		gtk_entry_set_text(GTK_ENTRY(bfspell.incorrectword), tocheck);
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(bfspell.suggestions)->entry), "");
		if (awl == 0) {
			DEBUG_MSG("spell_check_word error: %s\n", aspell_speller_error_message(bfspell.spell_checker));
		} else {
			GList *poplist=NULL;
			AspellStringEnumeration *els = aspell_word_list_elements(awl);
			const char *word;
			while ((word = aspell_string_enumeration_next(els)) != 0) {
				poplist = g_list_append(poplist,g_strdup(word));
			}
			gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(bfspell.suggestions)->entry), "");
			gtk_combo_set_popdown_strings(GTK_COMBO(bfspell.suggestions), poplist);
			free_stringlist(poplist);
			delete_aspell_string_enumeration(els);
		}
		return FALSE;
	} else {
		return TRUE;
	}
}

gboolean spell_run() {
	GtkTextIter itstart,itend;
	gchar *word = doc_get_next_word(&itstart,&itend);
	if (!word) {
		GList *poplist = NULL;
		DEBUG_MSG("spell_run: finished\n");
		gtk_entry_set_text(GTK_ENTRY(bfspell.incorrectword), "");
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(bfspell.suggestions)->entry), "");
		poplist = g_list_append(poplist,"");
		gtk_combo_set_popdown_strings(GTK_COMBO(bfspell.suggestions), poplist);
		g_list_free(poplist);
		bfspell.offset = 0;
		aspell_speller_save_all_word_lists(bfspell.spell_checker);
		delete_aspell_speller(bfspell.spell_checker);
		bfspell.spell_checker = NULL;
		return FALSE; /* finished */
	}
	while (word) {
		DEBUG_MSG("spell_run: word '%s'\n", word);
		if (spell_check_word(word,&itstart,&itend)) {
			g_free(word);
			word = doc_get_next_word(&itstart,&itend);
		} else {
			g_free(word);
			word = NULL;
		}
	}
	return TRUE; /* not yet finished */
}

void spell_start()
{	
	bfspell.spell_config = new_aspell_config();
	/*
	 * default language should come from config file, runtime from GUI,
	 * should first set the default one 
	 */
	aspell_config_replace(bfspell.spell_config, "lang", main_v->props.spell_default_lang);
	/*
	 * it is unclear from the manual if aspell supports utf-8 in the
	 * library, the utility does not support it.. 
	 */
	aspell_config_replace(bfspell.spell_config, "encoding", "utf-8");
	/*
	 * from the aspell manual 
	 */
}

static void spell_gui_destroy(GtkWidget * widget, GdkEvent *event, gpointer data) {
	window_destroy(bfspell.win);
	if (bfspell.spell_checker) {
		aspell_speller_save_all_word_lists(bfspell.spell_checker);
		delete_aspell_speller(bfspell.spell_checker);
	}
	delete_aspell_config(bfspell.spell_config);
	if (bfspell.so) {
		gtk_text_buffer_delete_mark(bfspell.doc->buffer, bfspell.so);
	}
	if (bfspell.eo) {
		gtk_text_buffer_delete_mark(bfspell.doc->buffer, bfspell.eo);
	}
	memset(&bfspell,0,sizeof(bfspell));
}

void spell_gui_cancel_clicked_cb(GtkWidget *widget, gpointer data) {
	spell_gui_destroy(NULL, NULL, data);
}

static void spell_gui_set_button_status(gboolean is_running) {
	gtk_widget_set_sensitive(bfspell.runbut,!is_running);
	gtk_widget_set_sensitive(bfspell.lang,!is_running);
	gtk_widget_set_sensitive(bfspell.repbut,is_running);
	gtk_widget_set_sensitive(bfspell.ignbut,is_running);
}

void spell_gui_ok_clicked_cb(GtkWidget *widget, gpointer data) {
	AspellCanHaveError *possible_err;
	const gchar *lang;
	bfspell.doc = main_v->current_document;
	lang = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(bfspell.lang)->entry));
	aspell_config_replace(bfspell.spell_config, "lang", lang);
	DEBUG_MSG("spell_gui_ok_clicked_cb, set lang to %s\n",lang);

	possible_err = new_aspell_speller(bfspell.spell_config);
	bfspell.spell_checker = 0;
	if (aspell_error_number(possible_err) != 0) {
		DEBUG_MSG(aspell_error_message(possible_err));
		/* now send an error, and stop */
		return;
	} else {
		bfspell.spell_checker = to_aspell_speller(possible_err);
	}

	if (spell_run()) {
		spell_gui_set_button_status(TRUE);
	} else {
		spell_gui_set_button_status(FALSE);
	}
}

void spell_gui_fill_dicts() {
	GList *retlist = NULL;
	AspellDictInfoEnumeration * dels;
	AspellDictInfoList * dlist;
	const AspellDictInfo * entry;
	dlist = get_aspell_dict_info_list(bfspell.spell_config);
	dels = aspell_dict_info_list_elements(dlist);
	while ( (entry = aspell_dict_info_enumeration_next(dels)) != 0) {
		retlist = g_list_append(retlist, g_strdup(entry->name));
	}
	delete_aspell_dict_info_enumeration(dels);
	
	gtk_combo_set_popdown_strings(GTK_COMBO(bfspell.lang), retlist);
	free_stringlist(retlist);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(bfspell.lang)->entry), main_v->props.spell_default_lang);
}

void spell_gui_add_clicked(GtkWidget *widget, gpointer data) {
	const gchar *addoption = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(bfspell.dict)->entry));
	const gchar *original = gtk_entry_get_text(GTK_ENTRY(bfspell.incorrectword));
	if (strcmp(addoption, _("personal dictionary"))==0) {
		spell_add_to_session(FALSE,original);
	} else {
		spell_add_to_session(TRUE,original);
	}
}
void spell_gui_ignore_clicked(GtkWidget *widget, gpointer data) {
	DEBUG_MSG("ignore\n");
	if (spell_run()) {
		spell_gui_set_button_status(TRUE);
	} else {
		spell_gui_set_button_status(FALSE);
	}
}
void spell_gui_replace_clicked(GtkWidget *widget, gpointer data) {
	gint start, end;
	GtkTextIter iter;
	const gchar *original = gtk_entry_get_text(GTK_ENTRY(bfspell.incorrectword));
	const gchar *newstring = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(bfspell.suggestions)->entry));

	aspell_speller_store_replacement(bfspell.spell_checker,original,-1,newstring,-1);

	gtk_text_buffer_get_iter_at_mark(bfspell.doc->buffer,&iter,bfspell.so);
	start = gtk_text_iter_get_offset(&iter);
	gtk_text_buffer_get_iter_at_mark(bfspell.doc->buffer,&iter,bfspell.eo);
	end = gtk_text_iter_get_offset(&iter);
	DEBUG_MSG("set %s from %d to %d\n",newstring,start,end);
	doc_replace_text(bfspell.doc, newstring, start, end);
	if (spell_run()) {
		spell_gui_set_button_status(TRUE);
	} else {
		spell_gui_set_button_status(FALSE);
	}
}

static void defaultlang_clicked_lcb(GtkWidget *widget,gpointer user_data) {
	const gchar *lang = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(bfspell.lang)->entry));
	g_free(main_v->props.spell_default_lang);
	DEBUG_MSG("defaultlang_clicked_lcb, default lang is now %s\n",lang);
	main_v->props.spell_default_lang = g_strdup(lang);
}

void spell_gui() {
	GtkWidget *vbox, *hbox, *but, *frame, *table;
	GList *poplist=NULL;
	bfspell.win = window_full(_("Spell checker"), GTK_WIN_POS_MOUSE, 3, G_CALLBACK(spell_gui_destroy),NULL, TRUE);
	vbox = gtk_vbox_new(FALSE, 2);
	gtk_container_add(GTK_CONTAINER(bfspell.win), vbox);
	
	frame = gtk_frame_new(_("Checking"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
	table = gtk_table_new(2,3,FALSE);
	gtk_container_add(GTK_CONTAINER(frame), table);
	
	bfspell.incorrectword = gtk_entry_new();
	gtk_entry_set_editable(GTK_ENTRY(bfspell.incorrectword),FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table), gtk_label_new(_("Original")),0,1,0,1);
	gtk_table_attach_defaults(GTK_TABLE(table), bfspell.incorrectword,1,2,0,1);
	
	bfspell.suggestions = gtk_combo_new();
	gtk_table_attach_defaults(GTK_TABLE(table), gtk_label_new(_("Replacement")),0,1,1,2);
	gtk_table_attach_defaults(GTK_TABLE(table), bfspell.suggestions,1,2,1,2);

	bfspell.ignbut = bf_stock_button(_("Ignore"), G_CALLBACK(spell_gui_ignore_clicked), NULL);
	bfspell.repbut = bf_stock_button(_("Replace"), G_CALLBACK(spell_gui_replace_clicked), NULL);
	gtk_table_attach_defaults(GTK_TABLE(table), bfspell.ignbut,2,3,0,1);
	gtk_table_attach_defaults(GTK_TABLE(table), bfspell.repbut,2,3,1,2);
	
	gtk_widget_set_sensitive(bfspell.repbut,FALSE);
	gtk_widget_set_sensitive(bfspell.ignbut,FALSE);
	
	/* lower GUI part */
	table = gtk_table_new(4,3,FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);

	bfspell.in_doc = gtk_radio_button_new_with_label(NULL, _("In document"));
	bfspell.in_sel = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(bfspell.in_doc)), _("In selection"));
	gtk_table_attach_defaults(GTK_TABLE(table), bfspell.in_doc,0,1,0,1);
	gtk_table_attach_defaults(GTK_TABLE(table), bfspell.in_sel,0,1,1,2);

	bfspell.dict = gtk_combo_new();
	poplist = g_list_append(poplist,_("personal dictionary"));
	poplist = g_list_append(poplist,_("session dictionary"));
	gtk_combo_set_popdown_strings(GTK_COMBO(bfspell.dict), poplist);
	gtk_table_attach_defaults(GTK_TABLE(table), gtk_label_new(_("Dictionary")),0,1,2,3);
	gtk_table_attach_defaults(GTK_TABLE(table), bfspell.dict,1,2,2,3);
	but = bf_stock_button(_("Add"), G_CALLBACK(spell_gui_add_clicked), NULL);
	gtk_table_attach_defaults(GTK_TABLE(table), but,2,3,2,3);

	bfspell.lang = gtk_combo_new();
	gtk_table_attach_defaults(GTK_TABLE(table), gtk_label_new(_("Language")),0,1,3,4);
	gtk_table_attach_defaults(GTK_TABLE(table), bfspell.lang,1,2,3,4);
	but = bf_stock_button(_("Set default"), G_CALLBACK(defaultlang_clicked_lcb), NULL);
	gtk_table_attach_defaults(GTK_TABLE(table), but,2,3,3,4);

	hbox = gtk_hbutton_box_new();
	gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 1);
	
	but = bf_stock_cancel_button(G_CALLBACK(spell_gui_cancel_clicked_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox),but,FALSE, FALSE, 0);
	bfspell.runbut = bf_gtkstock_button(GTK_STOCK_SPELL_CHECK,G_CALLBACK(spell_gui_ok_clicked_cb),NULL);
	gtk_box_pack_start(GTK_BOX(hbox),bfspell.runbut,FALSE, FALSE, 0);
	
	gtk_window_set_default(GTK_WINDOW(bfspell.win), bfspell.runbut);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	gtk_widget_show_all(bfspell.win);
}

void spell_check_cb(GtkWidget *widget, gpointer data) {
	if (!bfspell.win) {
		memset(&bfspell,0,sizeof(bfspell));
		spell_gui();
		flush_queue();
		spell_start();
		spell_gui_fill_dicts();
	}
}
#endif /* HAVE_LIBASPELL */