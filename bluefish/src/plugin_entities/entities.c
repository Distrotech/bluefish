/* Bluefish HTML Editor
 * entities.c -
 *
 * Copyright (C) 2006-2011 Olivier Sessink
 * Copyright (C) 2011 James Hayward
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


#include <gtk/gtk.h>

#include "entities.h"
#include "../config.h"
#include "../bluefish.h"
#include "../bf_lib.h"
#include "../dialog_utils.h"
#include "../document.h"
#include "../gtk_easy.h"
#include "../plugins.h"
#include "../rcfile.h"
#include "../undo_redo.h"
#include "../xml_entity.h"


typedef struct {
	gint convert_num;
	gint convert_iso;
	gint convert_symbol;
	gint convert_special;
	gint convert_xml;
	gint IE_apos_workaround;
} Tentitysetting;

typedef struct {
	Tentitysetting c2e;
	Tentitysetting e2c;
} Tentitiessession;

typedef struct {
	GHashTable *lookup;
} Tentities;

Tentities entities_v;

typedef enum {
	mode_urlencode,
	mode_urldecode,
	mode_tolowercase,
	mode_touppercase
} Treplace_mode;

typedef enum {
	mode_char2ent,
	mode_ent2char
} Tentmode;

typedef struct {
	GtkWidget *dialog;
	GtkWidget *scope;
	GtkWidget *numerical;
	GtkWidget *iso8859_1;
	GtkWidget *symbol;
	GtkWidget *special;
	GtkWidget *xml;
	GtkWidget *IE_apos_workaround;
	Tentmode mode;
	Tbfwin *bfwin;
	Tentitysetting *eset;
} Tentwin;

static void
doc_code_selection(Tdocument * doc, Treplace_mode mode)
{
	gint start, end;
	if (doc_get_selection(doc, &start, &end)) {
		gchar *inbuf, *outbuf = NULL;

		inbuf = doc_get_chars(doc, start, end);
		switch (mode) {
		case mode_urlencode:
			outbuf = g_uri_escape_string(inbuf, NULL, FALSE);
			break;
		case mode_urldecode:
			outbuf = g_uri_unescape_string(inbuf, NULL);
			break;
		case mode_tolowercase:
			if (inbuf)
				outbuf = g_utf8_strdown(inbuf, -1);
			break;
		case mode_touppercase:
			if (inbuf)
				outbuf = g_utf8_strup(inbuf, -1);
			break;
		}
		g_free(inbuf);
		if (outbuf) {
			doc_replace_text(doc, outbuf, start, end);
			g_free(outbuf);
		}
	}
}

void
doc_entities_to_utf8(Tdocument * doc, gint start, gint end, gboolean numerical, gboolean iso8859_1,
					 gboolean symbols, gboolean specials, gboolean xml)
{
	gchar *buf;
	const gchar *found, *prevfound;
	guint docoffset = start;	/* docoffset is an offset in characters between the buffer and the GtkTextBuffer contents */

	buf = doc_get_chars(doc, start, end);
	utf8_offset_cache_reset();

	found = g_utf8_strchr(buf, -1, '&');
	while (found) {
		gchar *endfound;
		endfound = g_utf8_strchr(found, -1, ';');
		if (endfound && endfound - found <= 7) {
			gchar *entity;
			gunichar unic;

			entity = g_strndup(found + 1, (endfound - found) - 1);
			/*unic = unichar_for_entity(entity,numerical,iso8859_1,symbols,specials,xml); */
			unic = xmlentity2unichar(entity, numerical, iso8859_1, symbols, specials, xml);
			if (unic != -1) {
				guint cfound, cendfound;
				gchar tmp[7];
				DEBUG_MSG("doc_entities_to_utf8, unic=%d for entity '%s'\n", unic, entity);
				memset(tmp, 0, 7);
				g_unichar_to_utf8(unic, tmp);

				cfound = utf8_byteoffset_to_charsoffset_cached(buf, (found - buf));
				cendfound = utf8_byteoffset_to_charsoffset_cached(buf, (endfound - buf));

				doc_replace_text_backend(doc, tmp, cfound + docoffset, cendfound + docoffset + 1);
				docoffset = docoffset - (cendfound + 1 - cfound) + 1;
			}
			g_free(entity);
			prevfound = g_utf8_next_char(endfound);
			found = g_utf8_strchr(prevfound, -1, '&');
		} else {
			found = g_utf8_strchr(g_utf8_next_char(found), -1, '&');
		}
	}
}

void
doc_utf8_to_entities(Tdocument * doc, gint start, gint end, gboolean iso8859_1, gboolean symbols,
					 gboolean specials, gboolean xml, gboolean numerical, gboolean IE_apos_workaround)
{
	gunichar unichar;
	gchar *buf, *srcp;
	guint docpos = start;
	buf = doc_get_chars(doc, start, end);
	srcp = buf;

	unichar = g_utf8_get_char(buf);
	while (unichar) {
		gchar *entity;
		/*entity = entity_for_unichar(unichar, iso8859_1, symbols, specials, xml); */
		entity = unichar2xmlentity(unichar, iso8859_1, symbols, specials, xml, numerical, IE_apos_workaround);
		if (entity) {
			doc_replace_text_backend(doc, entity, docpos, docpos + 1);
			docpos += (strlen(entity) - 1);
			g_free(entity);
		}
		srcp = g_utf8_next_char(srcp);
		unichar = g_utf8_get_char(srcp);
		docpos++;
	}
	g_free(buf);
}

static void
ew_response_lcb(GtkDialog * dialog, gint response, Tentwin * ew)
{
	if (response == GTK_RESPONSE_ACCEPT) {
		gint start = 0, end = -1;
		gint scope = gtk_combo_box_get_active(GTK_COMBO_BOX(ew->scope));
		if (ew->numerical)
			ew->eset->convert_num = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->numerical));
		ew->eset->convert_iso = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->iso8859_1));
		ew->eset->convert_symbol = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->symbol));
		ew->eset->convert_special = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->special));
		ew->eset->convert_xml = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->xml));
		if (ew->IE_apos_workaround)
			ew->eset->IE_apos_workaround =
				gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->IE_apos_workaround));

		if (scope == 0 || (scope == 1 && doc_get_selection(ew->bfwin->current_document, &start, &end))) {
			doc_unre_new_group_action_id(ew->bfwin->current_document, 0);
			if (ew->mode == mode_char2ent) {
				doc_utf8_to_entities(ew->bfwin->current_document, start, end,
									 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->iso8859_1)),
									 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->symbol)),
									 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->special)),
									 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->xml)),
									 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->numerical)),
									 ew->eset->IE_apos_workaround);
			} else {
				doc_entities_to_utf8(ew->bfwin->current_document, start, end,
									 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->numerical)),
									 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->iso8859_1)),
									 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->symbol)),
									 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->special)),
									 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->xml)));
			}
			doc_unre_new_group_action_id(ew->bfwin->current_document, 0);
		} else {				/* (scope == 2) */

			guint unre_action_id = new_unre_action_id();
			GList *tmplist;
			for (tmplist = g_list_first(ew->bfwin->documentlist); tmplist; tmplist = tmplist->next) {
				doc_unre_new_group_action_id(DOCUMENT(tmplist->data), unre_action_id);
				if (ew->mode == mode_char2ent) {
					doc_utf8_to_entities(DOCUMENT(tmplist->data), 0, -1,
										 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->iso8859_1)),
										 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->symbol)),
										 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->special)),
										 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->xml)),
										 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->numerical)),
										 ew->eset->IE_apos_workaround);
				} else {
					doc_entities_to_utf8(DOCUMENT(tmplist->data), 0, -1,
										 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->numerical)),
										 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->iso8859_1)),
										 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->symbol)),
										 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->special)),
										 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ew->xml)));
				}
				doc_unre_new_group_action_id(DOCUMENT(tmplist->data), 0);
			}
		}
	}
	gtk_widget_destroy(ew->dialog);
	g_free(ew);
}

static void
entity_dialog(Tbfwin * bfwin, Tentmode mode, Tentitysetting * eset)
{
	Tentwin *ew;
	GtkWidget *alignment, *hbox, *vbox;

	ew = g_new(Tentwin, 1);
	ew->bfwin = bfwin;
	ew->mode = mode;
	ew->eset = eset;
	ew->dialog =
		gtk_dialog_new_with_buttons(mode ==
									mode_char2ent ? _("Characters to entities") : _("Entities to characters"),
									GTK_WINDOW(bfwin->main_window), GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
									NULL);
#if !GTK_CHECK_VERSION(3, 0, 0)
	gtk_dialog_set_has_separator(GTK_DIALOG(ew->dialog), FALSE);
#endif /* gtk3 */
	g_signal_connect(G_OBJECT(ew->dialog), "response", G_CALLBACK(ew_response_lcb), ew);
	window_delete_on_escape(GTK_WINDOW(ew->dialog));

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(ew->dialog));

	alignment = gtk_alignment_new(0, 0, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 12, 12, 6, 6);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, TRUE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(alignment), vbox);

	hbox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	ew->scope = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ew->scope), _("In current document"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ew->scope), _("In selection"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ew->scope), _("In all documents"));
	dialog_label_new(_("Sco_pe:"), 0.5, 0.5, hbox, 0);
	gtk_box_pack_start(GTK_BOX(hbox), ew->scope, FALSE, FALSE, 0);

	ew->numerical = gtk_check_button_new_with_mnemonic(_("Convert numerical encoded characters &#99;"));
	gtk_box_pack_start(GTK_BOX(vbox), ew->numerical, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ew->numerical), eset->convert_num);

	ew->iso8859_1 = gtk_check_button_new_with_mnemonic(_("Convert _iso-8859-1 characters"));
	gtk_box_pack_start(GTK_BOX(vbox), ew->iso8859_1, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ew->iso8859_1), eset->convert_iso);

	ew->special = gtk_check_button_new_with_mnemonic(_("Convert spe_cial characters"));
	gtk_box_pack_start(GTK_BOX(vbox), ew->special, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ew->special), eset->convert_special);

	ew->symbol = gtk_check_button_new_with_mnemonic(_("Convert _symbol characters"));
	gtk_box_pack_start(GTK_BOX(vbox), ew->symbol, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ew->symbol), eset->convert_symbol);

	ew->xml = gtk_check_button_new_with_mnemonic(_("Convert _XML characters < > & \" '"));
	gtk_box_pack_start(GTK_BOX(vbox), ew->xml, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ew->xml), eset->convert_xml);

	if (mode == mode_char2ent) {
		ew->IE_apos_workaround =
			gtk_check_button_new_with_mnemonic(_("Work around missing &apos; entity on IE"));
		gtk_box_pack_start(GTK_BOX(vbox), ew->IE_apos_workaround, FALSE, FALSE, 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ew->IE_apos_workaround), eset->IE_apos_workaround);
	} else {
		ew->IE_apos_workaround = NULL;
	}
	if (doc_has_selection(bfwin->current_document)) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(ew->scope), 1);
	} else {
		gtk_combo_box_set_active(GTK_COMBO_BOX(ew->scope), 0);
	}
	gtk_widget_show_all(ew->dialog);
}

static void
entity_char_to_entity (GtkAction *action, gpointer user_data)
{
	Tbfwin *bfwin = BFWIN(user_data);
	Tentitiessession *es;

	es = g_hash_table_lookup(entities_v.lookup, bfwin->session);
	entity_dialog(bfwin, mode_char2ent, &es->c2e);
}

static void
entity_entity_to_char (GtkAction *action, gpointer user_data)
{
	Tbfwin *bfwin = BFWIN(user_data);
	Tentitiessession *es;

	es = g_hash_table_lookup(entities_v.lookup, bfwin->session);
	entity_dialog(bfwin, mode_ent2char, &es->e2c);
}

static void
entity_to_lowercase (GtkAction *action, gpointer user_data)
{
	doc_code_selection(BFWIN(user_data)->current_document, mode_tolowercase);
}

static void
entity_to_uppercase (GtkAction *action, gpointer user_data)
{
	doc_code_selection(BFWIN(user_data)->current_document, mode_touppercase);
}

static void
entity_url_decode (GtkAction *action, gpointer user_data)
{
	doc_code_selection(BFWIN(user_data)->current_document, mode_urldecode);
}

static void
entity_url_encode (GtkAction *action, gpointer user_data)
{
	doc_code_selection(BFWIN(user_data)->current_document, mode_urlencode);
}

static void
entity_init(void)
{
#ifdef ENABLE_NLS
	DEBUG_MSG("entity_init, gettext domain-name=%s\n", PACKAGE "_plugin_entities");
	bindtextdomain(PACKAGE "_plugin_entities", LOCALEDIR);
	bind_textdomain_codeset(PACKAGE "_plugin_entities", "UTF-8");
#endif							/* ENABLE_NLS */
	entities_v.lookup = g_hash_table_new_full(NULL /* == g_direct_hash() */ ,
											  NULL /* == g_direct_equal() */ ,
											  NULL, g_free);
/*  gchar *tmp, *tmp2;
/ * const gchar *orig = "hallo &amp; &test &uuml; ä;";* /
	const gchar *orig = "&alpha;";
	g_print("orig    : '%s'\n",orig);
	tmp = entities_to_utf8(orig);
	g_print("created : '%s'\n",tmp);
	tmp2 = utf8_to_entities(tmp, TRUE, TRUE,TRUE,FALSE);
	g_print("created2: '%s'\n",tmp2);
	g_free(tmp);
	g_free(tmp2);
	exit(1);*/
}

static const gchar *entities_plugin_ui =
"<ui>"
	"<menubar name='MainMenu'>"
		"<menu action='ToolsMenu'>"
			"<menu action='ToolsConvert'>"
				"<separator/>"
				"<menuitem action='CharactersToEntities'/>"
				"<menuitem action='EntitiesToCharacters'/>"
				"<menuitem action='URLDecode'/>"
				"<menuitem action='URLEncode'/>"
				"<menuitem action='ToLowercase'/>"
				"<menuitem action='ToUppercase'/>"
			"</menu>"
		"</menu>"
	"</menubar>"
"</ui>";

static void
entity_initgui(Tbfwin * bfwin)
{
	GtkActionGroup *action_group;
	GError *error = NULL;

	static const GtkActionEntry entities_actions[] = {
		{"CharactersToEntities", NULL, N_("C_haracters to Entities"), NULL,
		 N_("Change characters to entities"), G_CALLBACK(entity_char_to_entity)},
		{"EntitiesToCharacters", NULL, N_("Ent_ities to Characters"), NULL,
		 N_("Change entities to characters"), G_CALLBACK(entity_entity_to_char)},
		{"URLDecode", NULL, N_("URL _Decode"), NULL, N_("URL decode"), G_CALLBACK(entity_url_decode)},
		{"URLEncode", NULL, N_("URL _Encode"), NULL, N_("URL encode"), G_CALLBACK(entity_url_encode)},
		{"ToLowercase", NULL, N_("To Lo_wercase"), NULL, N_("To lowercase"), G_CALLBACK(entity_to_lowercase)},
		{"ToUppercase", NULL, N_("To _Uppercase"), NULL, N_("To uppercase"), G_CALLBACK(entity_to_uppercase)}
	};

	action_group = gtk_action_group_new("EntitiesActions");
	gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE "_plugin_entities");
	gtk_action_group_add_actions(action_group, entities_actions, G_N_ELEMENTS(entities_actions), bfwin);
	gtk_ui_manager_insert_action_group(bfwin->uimanager, action_group, 0);
	g_object_unref(action_group);

	gtk_ui_manager_add_ui_from_string(bfwin->uimanager, entities_plugin_ui, -1, &error);
	if (error != NULL) {
		g_warning("building entities plugin menu failed: %s", error->message);
		g_error_free(error);
	}
}

/*static void
entity_enforce_session(Tbfwin * bfwin)
{
}
*/
static void
entity_cleanup(void)
{
	g_hash_table_unref(entities_v.lookup);
}

/*static void
entity_cleanup_gui(Tbfwin * bfwin)
{
}
*/
/*static GHashTable *
entity_register_globses_config(GHashTable * configlist)
{
	return configlist;
}
*/
static GHashTable *
entity_register_session_config(GHashTable * configlist, Tsessionvars * session)
{
	Tentitiessession *es;
	es = g_hash_table_lookup(entities_v.lookup, session);
	if (!es) {
		es = g_new0(Tentitiessession, 1);
		es->e2c.convert_xml = TRUE;
		es->c2e.convert_xml = TRUE;
		g_hash_table_insert(entities_v.lookup, session, es);
	}
	configlist = make_config_list_item(configlist, &es->c2e.convert_iso, 'i', "c2e.convert_iso:", 0);
	configlist = make_config_list_item(configlist, &es->c2e.convert_symbol, 'i', "c2e.convert_symbol:", 0);
	configlist = make_config_list_item(configlist, &es->c2e.convert_special, 'i', "c2e.convert_special:", 0);
	configlist = make_config_list_item(configlist, &es->c2e.convert_xml, 'i', "c2e.convert_xml:", 0);
	configlist =
		make_config_list_item(configlist, &es->c2e.IE_apos_workaround, 'i', "c2e.IE_apos_workaround:", 0);
	configlist = make_config_list_item(configlist, &es->e2c.convert_num, 'i', "e2c.convert_num:", 0);
	configlist = make_config_list_item(configlist, &es->e2c.convert_iso, 'i', "e2c.convert_iso:", 0);
	configlist = make_config_list_item(configlist, &es->e2c.convert_symbol, 'i', "e2c.convert_symbol:", 0);
	configlist = make_config_list_item(configlist, &es->e2c.convert_special, 'i', "e2c.convert_special:", 0);
	configlist = make_config_list_item(configlist, &es->e2c.convert_xml, 'i', "e2c.convert_xml:", 0);
	return configlist;
}

static void
entity_session_cleanup(Tsessionvars * session)
{
	g_hash_table_remove(entities_v.lookup, session);
}

static TBluefishPlugin bfplugin = {
	"Convert Entities",
	BFPLUGIN_VERSION,
	GTK_MAJOR_VERSION,
	sizeof(Tdocument),
	sizeof(Tsessionvars),
	sizeof(Tglobalsession),
	sizeof(Tbfwin),
	sizeof(Tproject),
	sizeof(Tmain),
	sizeof(Tproperties),
	BFPLUGIN_PRIORITY_DEFAULT,
	1,
	NULL,						/* private */
	entity_init,				/* init */
	entity_initgui,
	NULL /*entity_enforce_session*/,
	entity_cleanup,
	NULL /*entity_cleanup_gui*/,
	NULL /*entity_register_globses_config*/,
	entity_register_session_config,
	entity_session_cleanup,
	NULL,						/* binary compatibility */
	NULL,
	NULL
};

G_MODULE_EXPORT TBluefishPlugin *
getplugin(void)
{
	return &bfplugin;
}
