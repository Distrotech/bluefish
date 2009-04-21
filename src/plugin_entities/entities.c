/* Bluefish HTML Editor
 * entities.c -
 *
 * Copyright (C) 2006-2008 Olivier Sessink
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


#include <gtk/gtk.h>
#include <string.h>

#include "entities.h"
#include "../config.h"
#include "../plugins.h"
#include "../bluefish.h"
#include "../bf_lib.h"
#include "../document.h"
#include "../gtk_easy.h"
#include "../undo_redo.h"
#include "../dialog_utils.h"
#include "../rcfile.h"

typedef struct {
	gint convert_num;
	gint convert_iso;
	gint convert_symbol;
	gint convert_special;
	gint convert_xml;
} Tentitysetting;

typedef struct {
	Tentitysetting c2e;
	Tentitysetting e2c;
} Tentitiessession;

typedef struct {
	GHashTable* lookup;
} Tentities;

Tentities entities_v;

/* see http://www.w3.org/TR/xhtml1/dtds.html for more information about the defined entities */

static gchar *entities_iso8859_1[] = {
  /* although nsbp is part of this set, we choose not to convert it */
  "iexcl", "cent", "pound", "curren", "yen", "brvbar", "sect", "uml", "copy", "ordf",
    "laquo", "not", "shy", "reg", "macr", "deg", "plusmn", "sup2", "sup3", "acute", "micro",
    "para", "middot", "cedil", "sup1", "ordm", "raquo", "frac14", "frac12", "frac34", "iquest",
    "Agrave", "Aacute", "Acirc", "Atilde", "Auml", "Aring", "AElig", "Ccedil", "Egrave",
    "Eacute", "Ecirc", "Euml", "Igrave", "Iacute", "Icirc", "Iuml", "ETH", "Ntilde", "Ograve",
    "Oacute", "Ocirc", "Otilde", "Ouml", "times", "Oslash", "Ugrave", "Uacute", "Ucirc", "Uuml",
    "Yacute", "THORN", "szlig", "agrave", "aacute", "acirc", "atilde", "auml", "aring", "aelig",
    "ccedil", "egrave", "eacute", "ecirc", "euml", "igrave", "iacute", "icirc", "iuml", "eth",
    "ntilde", "ograve", "oacute", "ocirc", "otilde", "ouml", "divide", "oslash", "ugrave",
    "uacute", "ucirc", "uuml", "yacute", "thorn", "yuml", NULL
};
static gchar *entities_symbols[] = {
  "fnof", "Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta", "Eta", "Theta", "Iota", "Kappa",
    "Lambda", "Mu", "Nu", "Xi", "Omicron", "Pi", "Rho", "Sigma", "Tau", "Uplsilon", "Phi",
    "Chi", "Psi", "Omega", "alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta",
    "iota", "kappa", "lambda", "mu", "nu", "xi", "omicron", "pi", "rho", "sigmaf", "sigma",
    "tau", "upsilon", "phi", "chi", "psi", "omega", "thetasym", "upsih", "piv", "bull",
    "hellip", "prime", "Prime", "oline", "frasl", "weierp", "image", "real", "trade", "alefsym",
    "larr", "uarr", "rarr", "darr", "harr", "crarr", "lArr", "uArr", "rArr", "dArr", "hArr",
    "forall", "part", "exist", "empty", "nabla", "isin" "notin", "ni", "prod", "sum", "minus",
    "lowast", "radic", "prop", "infin", "ang", "and", "or", "cap", "cup", "int", "there4",
    "sim", "cong", "asymp", "ne", "equiv", "le", "ge", "sub", "sup", "nsub", "sube", "supe",
    "oplus", "otimes", "perp", "sdot", "lceil", "rceil", "lfloor", "rfloor", "lang", "rang",
    "loz", "spades", "clubs", "hearts", "diams", NULL
};
static gchar *entities_special[] = {
  "OElig", "oelig", "Scaron", "scaron", "Yuml", "circ", "tilde",
    "ensp", "emsp", "thinsp", "zwnj", "zwj", "lrm", "rlm", "ndash", "mdash", "lsquo", "rsquo",
    "sbquo", "ldquo", "rdquo", "bdquo", "dagger", "Dagger", "permil", "lsaquo", "rsaquo",
    "euro", NULL
};
static gchar *entities_xml[] = {
  "nbsp", "quot", "amp", "lt", "gt", "apos", NULL
};
/*
 * the unicode characters for iso8859_1 are 161 + the index in the array
 */
static guint entity_unicode_symbols[] = {
  402, 913, 914, 915, 916, 917, 918, 919, 920, 921, 922, 923, 924, 925, 926, 927, 928, 929, 931,
    932, 933, 934, 935, 936, 937, 945, 946, 947, 948, 949, 950, 951, 952, 953, 954, 955, 956,
    957, 958, 959, 960, 961, 962, 963, 964, 965, 966, 967, 968, 977, 978, 982, 8226, 8230, 8242,
    8443, 8254, 8260, 8472, 8465, 8476, 8482, 8501, 8592, 8593, 8594, 8595, 8596, 8629, 8656,
    8657, 8658, 8659, 8660, 8704, 8706, 8707, 8709, 8711, 8712, 8713, 8715, 8719, 8721, 8722,
    8727, 8730, 8733, 8734, 8736, 8743, 8744, 8745, 8746, 8747, 8756, 8764, 8773, 8776, 8800,
    8801, 8804, 8805, 8834, 8835, 8836, 8838, 8839, 8853, 8855, 8869, 8901, 8968, 8969, 8970,
    8971, 9001, 9002, 9674, 9824, 9827, 9829, 9830, -1
};
static guint entity_unicode_special[] = {
  338, 339, 352, 353, 376, 710, 732, 8194, 8195, 8201, 8204, 8205, 8206, 8207,
    8211, 8212, 8216, 8217, 8218, 8220, 8221, 8222, 8224, 8225, 8240, 8249, 8250, 8364, -1
};
static guint entity_unicode_xml[] = {
  160, '"', '&', '<', '>', '\'', -1
};
/* retuerns -1 if index not found */
static gint index_in_array(gchar **arr,gchar *string) {
  gint i;
  for (i=0;arr[i]!=NULL;i++) {
    if (strcmp(arr[i],string)==0) {
      return i;
    }
  }
  return -1;
}

static gint index_in_array2(guint *arr, guint val) {
  gint i;
  for (i=0;arr[i]!=-1;i++) {
    if (arr[i]==val) {
      return i;
    }
  }
  return -1;
}

gunichar unichar_for_entity(gchar *entity,gboolean numerical, gboolean iso8859_1, gboolean symbols, gboolean specials, gboolean xml) {
  gint indx;
  if (!entity) return -1;
  if (entity[0] == '#') {
    if (numerical) {
      /* convert the remaining characters to a number */
      if (entity[1] == 'x') {
        /* from hexadecimal */
        return g_ascii_strtoull(&entity[2],NULL,16);
      } else {
        /* from decimal */
        return g_ascii_strtoull(&entity[1],NULL,10);
      }
    }
    return -1;
  }
  if (iso8859_1) {
    indx = index_in_array(entities_iso8859_1, entity);
    if (indx != -1) {
      return indx + 161;
    }
  }
  if (symbols) {
    indx = index_in_array(entities_symbols, entity);
    if (indx != -1) {
      return entity_unicode_symbols[indx];
    }
  }
  if (specials) {
    indx = index_in_array(entities_special, entity);
    if (indx != -1) {
      return entity_unicode_special[indx];
    }
  }
  if (xml) {
    indx = index_in_array(entities_xml, entity);
    if (indx != -1) {
      return entity_unicode_xml[indx];
    }
  }
  return -1;
}

gchar *entity_for_unichar(gunichar uchar, gboolean iso8859_1, gboolean symbols, gboolean specials, gboolean xml) {
  gint indx;
  if (iso8859_1 && uchar >= 161 && uchar < 255) {
    return entities_iso8859_1[uchar - 161];
  }
  if (symbols && (uchar == 402 || (uchar >= 913 && uchar <= 982) ||(uchar >= 8226 && uchar <= 9830))) {
    indx = index_in_array2(entity_unicode_symbols, uchar);
    if (indx != -1) return entities_symbols[indx];
  }
  if (specials && ((uchar >= 338 && uchar <= 376) || uchar == 710 || uchar == 732 || (uchar >= 8194 && uchar <= 8364) )) {
    indx = index_in_array2(entity_unicode_special, uchar);
    if (indx != -1) return entities_special[indx];
  }
  if (xml) {
    indx = index_in_array2(entity_unicode_xml, uchar);
    if (indx != -1) return entities_xml[indx];
  }
  return NULL;
}

typedef enum {
  mode_urlencode,
  mode_urldecode,
  mode_tolowercase,
  mode_touppercase
} Treplace_mode;

static void doc_code_selection(Tdocument *doc, Treplace_mode mode) {
  gint start, end;
  if (doc_get_selection(doc, &start, &end)) {
    gchar *inbuf, *outbuf = NULL;
    
    inbuf = doc_get_chars(doc,start,end);
    switch (mode) {
      case mode_urlencode:
        outbuf = g_uri_escape_string (inbuf, NULL, FALSE);
      break;
      case mode_urldecode:
        outbuf = g_uri_unescape_string (inbuf, NULL);
      break;
      case mode_tolowercase:
        if (inbuf) outbuf = g_utf8_strdown(inbuf,-1);
      break;
      case mode_touppercase:
        if (inbuf) outbuf = g_utf8_strup(inbuf,-1);
      break;
    }   
    g_free(inbuf);
    if (outbuf) {
      doc_replace_text(doc,outbuf,start,end);
      g_free(outbuf);
    }
  }
}

/* inbuf is a NULL terminated UTF8 string */
gchar *entities_to_utf8(const gchar *inbuf) {
  const gchar *found, *prevfound;
  gchar *outbuf, *outbufloc;

  outbuf = g_malloc0(strlen(inbuf));
  prevfound = inbuf;
  outbufloc = outbuf;
  found = g_utf8_strchr(inbuf,-1, '&');
  while (found) {
    gchar *endfound;
    
    endfound = g_utf8_strchr(found,-1, ';');
    if (endfound && endfound - found <= 7) {
      gchar *entity, tmp[7];
      gint len;
      gunichar unic;
      
      entity = g_strndup(found+1, (endfound-found)-1);
      len = (found - prevfound);
      memcpy(outbufloc,prevfound,len);
      outbufloc += len;
      unic = unichar_for_entity(entity, TRUE,TRUE,TRUE,TRUE,FALSE);
      if (unic != -1) {
        g_print("unic=%d for entity '%s'\n",unic,entity);
        memset(tmp, 0, 7);
        len = g_unichar_to_utf8(unic, tmp);
        /*g_print("tmp='%s', len=%d\n",tmp,len);*/
        len = strlen(tmp);
        memcpy(outbufloc, tmp, len);
        outbufloc += len;
      } else {
        len = endfound - found+1;
        memcpy(outbufloc, found, len);
        outbufloc += len;
      }
      g_free(entity);
      
      prevfound = g_utf8_next_char(endfound);
      found = g_utf8_strchr(prevfound,-1, '&');
    } else {
      found = g_utf8_strchr(g_utf8_next_char(found),-1, '&');
    }
  }
  memcpy(outbufloc,prevfound,strlen(prevfound)+1);
  return outbuf;
}

void doc_entities_to_utf8(Tdocument *doc, gint start, gint end, gboolean numerical, gboolean iso8859_1, gboolean symbols, gboolean specials, gboolean xml) {
  gchar *buf;
  const gchar *found, *prevfound;
  guint docoffset=start; /* docoffset is an offset in characters between the buffer and the GtkTextBuffer contents */

  buf = doc_get_chars(doc,start,end);
  utf8_offset_cache_reset();
  
  found = g_utf8_strchr(buf,-1, '&');
  while (found) {
    gchar *endfound;
    endfound = g_utf8_strchr(found,-1, ';');
    if (endfound && endfound - found <= 7) {
      gchar *entity;
      gint len;
      gunichar unic;
      
      entity = g_strndup(found+1, (endfound-found)-1);
      len = (found - prevfound);
      unic = unichar_for_entity(entity,numerical,iso8859_1,symbols,specials,xml);
      if (unic != -1) {
        guint cfound,cendfound;
        gchar tmp[7];
        DEBUG_MSG("doc_entities_to_utf8, unic=%d for entity '%s'\n",unic,entity);
        memset(tmp, 0, 7);
        len = g_unichar_to_utf8(unic, tmp);
        
        cfound = utf8_byteoffset_to_charsoffset_cached(buf, (found-buf));
        cendfound = utf8_byteoffset_to_charsoffset_cached(buf, (endfound-buf));
        
        doc_replace_text_backend(doc,tmp,cfound+docoffset,cendfound+docoffset+1);
        docoffset = docoffset - (cendfound+1-cfound) + 1;
      }
      g_free(entity);
      prevfound = g_utf8_next_char(endfound);
      found = g_utf8_strchr(prevfound,-1, '&');
    } else {
      found = g_utf8_strchr(g_utf8_next_char(found),-1, '&');
    }
  }
}


gchar *utf8_to_entities(const gchar *inbuf, gboolean iso8859_1, gboolean symbols, gboolean specials, gboolean xml) {
  gchar *outbuf;
  const gchar *srcp = inbuf;
  gunichar unichar = g_utf8_get_char(inbuf);
  /* we use a lot of memory for this function, but it makes the code simple */
  outbuf = g_malloc0(8 * strlen(inbuf)*sizeof(gchar));
  while (unichar) {
    gchar *entity;
    entity = entity_for_unichar(unichar, iso8859_1, symbols, specials, xml);
    if (entity) {
      outbuf = strncat(outbuf, "&", 1);
      outbuf = strncat(outbuf, entity, 7);
      outbuf = strncat(outbuf, ";", 1);
      srcp = g_utf8_next_char(srcp);
    } else {
      gchar *nxt = g_utf8_next_char(srcp);
      outbuf = strncat(outbuf, srcp, (nxt-srcp));
      srcp = nxt;
    }
    unichar = g_utf8_get_char (srcp);
  }
  return outbuf;
}

void doc_utf8_to_entities(Tdocument *doc, gint start, gint end, gboolean iso8859_1, gboolean symbols, gboolean specials, gboolean xml) {
  gunichar unichar;
  gchar *buf, *srcp;
  guint docpos=start;
  buf = doc_get_chars(doc,start,end);
  srcp = buf;
  unichar = g_utf8_get_char(buf);
  while (unichar) {
    gchar *entity;
    entity = entity_for_unichar(unichar, iso8859_1, symbols, specials, xml);
    if (entity) {
      gchar *replacew = g_strconcat("&", entity, ";", NULL);
      doc_replace_text_backend(doc,replacew,docpos,docpos+1);
      docpos += (strlen(replacew)-1);
      g_free(replacew);
    }
    srcp = g_utf8_next_char(srcp);
    unichar = g_utf8_get_char (srcp);
    docpos++;
  }
  g_free(buf);
}

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
  Tentmode mode;
  Tbfwin *bfwin;
  Tentitysetting *eset;
} Tentwin;

static void ew_response_lcb(GtkDialog * dialog, gint response, Tentwin * ew) {
	if (response == GTK_RESPONSE_ACCEPT) {
		gint start=0, end=-1;
		gint scope = gtk_combo_box_get_active(GTK_COMBO_BOX(ew->scope));
    	if (ew->numerical)
	    	ew->eset->convert_num = GTK_TOGGLE_BUTTON(ew->numerical)->active;
		ew->eset->convert_iso = GTK_TOGGLE_BUTTON(ew->iso8859_1)->active;
		ew->eset->convert_symbol = GTK_TOGGLE_BUTTON(ew->symbol)->active;
		ew->eset->convert_special = GTK_TOGGLE_BUTTON(ew->special)->active;
		ew->eset->convert_xml = GTK_TOGGLE_BUTTON(ew->xml)->active;
    
    
    if (scope == 0 || (scope == 1 && doc_get_selection(ew->bfwin->current_document, &start, &end))) {
      doc_unre_new_group_action_id(ew->bfwin->current_document,0);
      if (ew->mode == mode_char2ent) {
        doc_utf8_to_entities(ew->bfwin->current_document, start, end,  
            GTK_TOGGLE_BUTTON(ew->iso8859_1)->active, GTK_TOGGLE_BUTTON(ew->symbol)->active, 
            GTK_TOGGLE_BUTTON(ew->special)->active, GTK_TOGGLE_BUTTON(ew->xml)->active);
      } else {
        doc_entities_to_utf8(ew->bfwin->current_document, start, end, GTK_TOGGLE_BUTTON(ew->numerical)->active,
            GTK_TOGGLE_BUTTON(ew->iso8859_1)->active, GTK_TOGGLE_BUTTON(ew->symbol)->active, 
            GTK_TOGGLE_BUTTON(ew->special)->active, GTK_TOGGLE_BUTTON(ew->xml)->active);
      }
      doc_unre_new_group_action_id(ew->bfwin->current_document,0);
    } else /* (scope == 2) */ {
      guint unre_action_id = new_unre_action_id();
      GList *tmplist;
      for (tmplist=g_list_first(ew->bfwin->documentlist);tmplist;tmplist=tmplist->next) {
        doc_unre_new_group_action_id(DOCUMENT(tmplist->data), unre_action_id);
        if (ew->mode == mode_char2ent) {
          doc_utf8_to_entities(DOCUMENT(tmplist->data), 0, -1, 
              GTK_TOGGLE_BUTTON(ew->iso8859_1)->active, GTK_TOGGLE_BUTTON(ew->symbol)->active, 
              GTK_TOGGLE_BUTTON(ew->special)->active, GTK_TOGGLE_BUTTON(ew->xml)->active);
        } else {
          doc_entities_to_utf8(DOCUMENT(tmplist->data), 0, -1, GTK_TOGGLE_BUTTON(ew->numerical)->active,
              GTK_TOGGLE_BUTTON(ew->iso8859_1)->active, GTK_TOGGLE_BUTTON(ew->symbol)->active, 
              GTK_TOGGLE_BUTTON(ew->special)->active, GTK_TOGGLE_BUTTON(ew->xml)->active);
        }
        doc_unre_new_group_action_id(DOCUMENT(tmplist->data),0);
      }
    }
  }
  gtk_widget_destroy(ew->dialog);
  g_free(ew);
}

static void entity_dialog(Tbfwin *bfwin, Tentmode mode, Tentitysetting *eset) {
  Tentwin *ew;
  GtkWidget *hbox;
  
  ew = g_new(Tentwin,1);
  ew->bfwin = bfwin;
  ew->mode = mode;
  ew->eset = eset;
  ew->dialog = gtk_dialog_new_with_buttons(mode == mode_char2ent ? _("Characters to entities") : _("Entities to characters"),
      GTK_WINDOW(bfwin->main_window),GTK_DIALOG_DESTROY_WITH_PARENT, 
      GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
      GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, 
      NULL);
  g_signal_connect(G_OBJECT(ew->dialog), "response", G_CALLBACK(ew_response_lcb), ew);
  window_delete_on_escape(GTK_WINDOW(ew->dialog));
  gtk_container_set_border_width(GTK_CONTAINER(ew->dialog),10);
  gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(ew->dialog)->vbox),10);
  hbox = gtk_hbox_new(FALSE,50);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ew->dialog)->vbox), hbox, FALSE, FALSE, 0);

  ew->scope = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(ew->scope), _("In current document"));
  gtk_combo_box_append_text(GTK_COMBO_BOX(ew->scope), _("In selection"));
  gtk_combo_box_append_text(GTK_COMBO_BOX(ew->scope), _("In all documents"));
  dialog_box_label_new(_("Sco_pe:"), 0.5, 0.5, hbox, 0);
  gtk_box_pack_start(GTK_BOX(hbox), ew->scope, FALSE, FALSE, 0);
  
  if (mode == mode_ent2char) {
    ew->numerical = gtk_check_button_new_with_mnemonic(_("Convert numerical characters"));
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ew->dialog)->vbox), ew->numerical, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ew->numerical),eset->convert_num);
  } else {
    ew->numerical = NULL;
  }

  ew->iso8859_1 = gtk_check_button_new_with_mnemonic(_("Convert _iso-8859-1 characters"));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ew->dialog)->vbox), ew->iso8859_1, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ew->iso8859_1),eset->convert_iso);
  
  ew->symbol = gtk_check_button_new_with_mnemonic(_("Convert _symbol characters"));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ew->dialog)->vbox), ew->symbol, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ew->symbol),eset->convert_symbol);
  
  ew->special = gtk_check_button_new_with_mnemonic(_("Convert spe_cial characters"));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ew->dialog)->vbox), ew->special, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ew->special),eset->convert_special);
  
  ew->xml = gtk_check_button_new_with_mnemonic(_("Convert _xml characters <>&\"'"));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ew->dialog)->vbox), ew->xml, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ew->xml),eset->convert_xml);
  
/*  g_signal_connect(ew->overlappingMatches, "toggled", G_CALLBACK(snr_option_toggled), snrwin);
  gtk_tooltips_set_tip(main_v->tooltips,ew->overlappingMatches,_("After a match is found, start next search within that match."),NULL);*/
  if (doc_has_selection(bfwin->current_document)) {
    gtk_combo_box_set_active(GTK_COMBO_BOX(ew->scope), 1);
  } else {
    gtk_combo_box_set_active(GTK_COMBO_BOX(ew->scope), 0);
  }
  gtk_widget_show_all(ew->dialog);
}

static void entity_menu_lcb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget){
	Tentitiessession *es;
	es = g_hash_table_lookup(entities_v.lookup,bfwin->session);
  switch (callback_action) {
  case 0:
    entity_dialog(bfwin, mode_ent2char, &es->e2c);
  break;
  case 1:
    entity_dialog(bfwin, mode_char2ent, &es->c2e);
  break;
  case 2:
    doc_code_selection(bfwin->current_document, mode_urlencode);
  break;
  case 3:
    doc_code_selection(bfwin->current_document, mode_urldecode);
  break;
  case 4:
    doc_code_selection(bfwin->current_document, mode_tolowercase);
  break;
  case 5:
    doc_code_selection(bfwin->current_document, mode_touppercase);
  break;
  }
}

static void entity_init(void) {
#ifdef ENABLE_NLS
	DEBUG_MSG("entity_init, gettext domain-name=%s\n",PACKAGE"_plugin_entities");
	bindtextdomain(PACKAGE"_plugin_entities", LOCALEDIR);
	bind_textdomain_codeset(PACKAGE"_plugin_entities", "UTF-8");
#endif /* ENABLE_NLS */
	entities_v.lookup = g_hash_table_new_full(NULL /* == g_direct_hash() */,
					NULL /* == g_direct_equal() */,
					NULL,NULL);
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
#ifdef ENABLE_NLS
static gchar *entity_menu_translate(const gchar * path, gpointer data) {
  return _(path);
}
#endif /* ENABLE_NLS */
static void entity_initgui(Tbfwin* bfwin) {
  GtkItemFactory *ifactory;
  static GtkItemFactoryEntry menu_items[] = {
    {N_("/Edit/Replace special/Entities to characters"), NULL, entity_menu_lcb, 0, "<Item>"},
    {N_("/Edit/Replace special/Characters to entities"), NULL, entity_menu_lcb, 1, "<Item>"},
    {N_("/Edit/Replace special/URL encode selection"), NULL, entity_menu_lcb, 2, "<Item>"},
    {N_("/Edit/Replace special/URL decode selection"), NULL, entity_menu_lcb, 3, "<Item>"},
    {N_("/Edit/Replace special/To Lowercase"), NULL, entity_menu_lcb, 4, "<Item>"},
    {N_("/Edit/Replace special/To Uppercase"), NULL, entity_menu_lcb, 5, "<Item>"}
  };
  ifactory = gtk_item_factory_from_widget(bfwin->menubar);
#ifdef ENABLE_NLS
  gtk_item_factory_set_translate_func(ifactory, entity_menu_translate, "<bluefishmain>", NULL);
#endif
  gtk_item_factory_create_items(ifactory, sizeof(menu_items) / sizeof(menu_items[0]), menu_items, bfwin);
  gtk_widget_show_all(bfwin->menubar);

}
static void entity_enforce_session(Tbfwin* bfwin) {}
static void entity_cleanup(void) {}
static void entity_cleanup_gui(Tbfwin *bfwin) {}
static GHashTable *entity_register_globses_config(GHashTable *configlist) {return configlist;}
static GHashTable *entity_register_session_config(GHashTable *configlist, Tsessionvars *session) {
	Tentitiessession *es;
	es = g_hash_table_lookup(entities_v.lookup,session);
	if (!es) {
		es = g_new0(Tentitiessession,1);
		es->e2c.convert_xml = TRUE;
		es->c2e.convert_xml = TRUE;
		g_hash_table_insert(entities_v.lookup,session,es);
	}
	configlist = make_config_list_item(configlist, &es->c2e.convert_iso, 'i', "c2e.convert_iso:", 0);
	configlist = make_config_list_item(configlist, &es->c2e.convert_symbol, 'i', "c2e.convert_symbol:", 0);
	configlist = make_config_list_item(configlist, &es->c2e.convert_special, 'i', "c2e.convert_special:", 0);
	configlist = make_config_list_item(configlist, &es->c2e.convert_xml, 'i', "c2e.convert_xml:", 0);
	configlist = make_config_list_item(configlist, &es->e2c.convert_num, 'i', "e2c.convert_num:", 0);
	configlist = make_config_list_item(configlist, &es->e2c.convert_iso, 'i', "e2c.convert_iso:", 0);
	configlist = make_config_list_item(configlist, &es->e2c.convert_symbol, 'i', "e2c.convert_symbol:", 0);
	configlist = make_config_list_item(configlist, &es->e2c.convert_special, 'i', "e2c.convert_special:", 0);
	configlist = make_config_list_item(configlist, &es->e2c.convert_xml, 'i', "e2c.convert_xml:", 0);
	return configlist;
}

static TBluefishPlugin bfplugin = {
  "entities",
  BFPLUGIN_VERSION,
  sizeof(Tdocument),
  sizeof(Tsessionvars),
  sizeof(Tglobalsession),
  sizeof(Tbfwin),
  sizeof(Tproject),
  sizeof(Tmain),
  NULL, /* private */
  entity_init, /* init */
  entity_initgui, 
  entity_enforce_session,
  entity_cleanup,
  entity_cleanup_gui,
  entity_register_globses_config,
  entity_register_session_config,
  NULL, /* binary compatibility */
  NULL,
  NULL,
  NULL
};

G_MODULE_EXPORT TBluefishPlugin *getplugin(void) {
  return &bfplugin;
}
