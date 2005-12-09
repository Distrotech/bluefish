#include <gtk/gtk.h>
#include <string.h>
#include "../config.h"
#include "../plugins.h"
#include "../bluefish.h"
#include "../bf_lib.h"
#include "../document.h"
#include "../gtk_easy.h"
#include "../undo_redo.h"
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
	"nbsp", "quot", "amp", "lt", "gt", NULL
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
	160, 34, 38, 60, 62, -1
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

gunichar unichar_for_entity(gchar *entity) {
	gint indx;
	if (!entity) return -1;
	if (entity[0] == '#') {
		/* convert the remaining characters to a number */
		if (entity[1] == 'x') {
			/* from hexadecimal */
			return g_ascii_strtoull(&entity[2],NULL,16);
		} else {
			/* from decimal */
			return g_ascii_strtoull(&entity[1],NULL,10);
		}
	}
	indx = index_in_array(entities_iso8859_1, entity);
	if (indx != -1) {
		return indx + 161;
	}
	indx = index_in_array(entities_symbols, entity);
	if (indx != -1) {
		return entity_unicode_symbols[indx];
	}
	indx = index_in_array(entities_special, entity);
	if (indx != -1) {
		g_print("found index %d for entity %s\n",indx,entity);
		return entity_unicode_special[indx];
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
			unic = unichar_for_entity(entity);
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


gchar *utf8_to_entities(const gchar *inbuf, gboolean iso8859_1, gboolean symbols, gboolean specials, gboolean xml) {
	gchar *outbuf;
	/* we use a lot of memory for this function, but it makes the code simple */
	outbuf = g_malloc0(8 * strlen(inbuf)*sizeof(gchar));
	const gchar *srcp = inbuf;
	gunichar unichar = g_utf8_get_char(inbuf);
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

void doc_utf8_to_entities(Tdocument *doc, gboolean separate_undo, gboolean iso8859_1, gboolean symbols, gboolean specials, gboolean xml) {
	gunichar unichar;
	gint start, end;
	gchar *buf, *srcp;
	guint docpos;
	if (doc_get_selection(doc, &start, &end)) {
		buf = doc_get_chars(doc,start,end);
		docpos = start;
	} else {
		buf = doc_get_chars(doc,0,-1);
		docpos = 0;
	}
	doc_unre_new_group(doc);
	srcp = buf;
	unichar = g_utf8_get_char(buf);
	while (unichar) {
		gchar *entity;
		entity = entity_for_unichar(unichar, iso8859_1, symbols, specials, xml);
		if (entity) {
			gchar *replacew = g_strconcat("&", entity, ";", NULL);
			if (separate_undo) {
				doc_replace_text(doc,replacew,docpos,docpos+1);
			} else {
				doc_replace_text_backend(doc,replacew,docpos,docpos+1);
			}
			docpos += (strlen(replacew)-1);
			g_free(replacew);
		}
		srcp = g_utf8_next_char(srcp);
		unichar = g_utf8_get_char (srcp);
		docpos++;
	}
	g_free(buf);
	doc_unre_new_group(doc);
}

typedef enum {
	mode_char2ent,
	mode_ent2char
} Tentmode;

typedef struct {
	GtkWidget *dialog;
	GtkWidget *scope;
	GtkWidget *iso8859_1;
	GtkWidget *symbol;
	GtkWidget *special;
	GtkWidget *xml;
	Tentmode mode;
	Tbfwin *bfwin;
} Tentwin;

static void ew_response_lcb(GtkDialog * dialog, gint response, Tentwin * ew) {

	/* DO SOMETHING */
	
	gtk_widget_destroy(ew->dialog);
	g_free(ew);
}

static void entity_dialog(Tbfwin *bfwin, Tentmode mode) {
	Tentwin *ew;
	GtkWidget *table;
	
	ew = g_new(Tentwin,1);
	ew->bfwin = bfwin;
	ew->mode = mode;
	
	ew->dialog = gtk_dialog_new_with_buttons(mode == mode_char2ent ? _("Characters to entities") : _("Entities to characters"),
			GTK_WINDOW(bfwin->main_window),GTK_DIALOG_DESTROY_WITH_PARENT, 
			GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, 
			NULL);
	g_signal_connect(G_OBJECT(ew->dialog), "response", G_CALLBACK(ew_response_lcb), ew);
	window_delete_on_escape(GTK_WINDOW(ew->dialog));
	
	table = dialog_table_in_vbox(5, 2, 6, GTK_DIALOG(ew->dialog)->vbox, FALSE, FALSE, 0);

	ew->scope = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(ew->scope), _("In current document"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(ew->scope), _("In selection"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(ew->scope), _("In all documents"));
	dialog_mnemonic_label_in_table(_("Sco_pe: "), ew->scope, table, 0, 1, 0, 1);
	gtk_table_attach(GTK_TABLE(table), ew->scope, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);


	gtk_widget_show_all(ew->dialog);
}

static void entity_menu_lcb(Tbfwin *bfwin,guint callback_action, GtkWidget *widget){
	if (callback_action == 0) {
		gint start, end;
		gchar *buf, *newbuf;
		if (doc_get_selection(bfwin->current_document, &start, &end)) {
			buf = doc_get_chars(bfwin->current_document,start,end);
		} else {
			buf = doc_get_chars(bfwin->current_document,0,-1);
		}
		newbuf = entities_to_utf8(buf);
		g_free(buf);
		doc_replace_text(bfwin->current_document,newbuf,0,-1);
		g_free(newbuf);
	} else {
/*		gint start, end;
		gchar *buf, *newbuf;
		if (doc_get_selection(bfwin->current_document, &start, &end)) {
			buf = doc_get_chars(bfwin->current_document,start,end);
		} else {
			buf = doc_get_chars(bfwin->current_document,0,-1);
		}
		newbuf = utf8_to_entities(buf,TRUE,TRUE,TRUE,FALSE);
		g_free(buf);
		doc_replace_text(bfwin->current_document,newbuf,0,-1);
		g_free(newbuf);*/
		/* we need a dialog on top of all these options! */
		doc_utf8_to_entities(bfwin->current_document,TRUE,TRUE, TRUE, TRUE, FALSE);
	}
}

static void entity_init(void) {
#ifdef ENABLE_NLS
	bindtextdomain(PACKAGE"_plugin_entities", LOCALEDIR);
	bind_textdomain_codeset(PACKAGE"_plugin_entities", "UTF-8");
#endif /* ENABLE_NLS */
/*	gchar *tmp, *tmp2;
/ *	const gchar *orig = "hallo &amp; &test &uuml; ä;";* /
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
		{N_("/Edit/Replace special/Characters to entities"), NULL, entity_menu_lcb, 1, "<Item>"}
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
static GList *entity_register_globses_config(GList *configlist) {return configlist;}
static GList *entity_register_session_config(GList *configlist, Tsessionvars *session) {	return configlist;}

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
