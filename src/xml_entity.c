/* Bluefish HTML Editor
 * xml_entity.c - XML and HTML entity conversion
 *
 * Copyright (C) 2006-2010 Olivier Sessink
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

#include "bluefish.h"
#include "xml_entity.h"

/* see http://www.w3.org/TR/xhtml1/dtds.html for more information about the defined entities */

/*
* the unicode characters for iso8859_1 are 161 + the index in the array
*/
static const gchar *entities_iso8859_1[] = {
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

static const gchar *entities_symbols[] = {
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
static const gint16 entity_unicode_symbols[] = {
	402, 913, 914, 915, 916, 917, 918, 919, 920, 921, 922, 923, 924, 925, 926, 927, 928, 929, 931,
		932, 933, 934, 935, 936, 937, 945, 946, 947, 948, 949, 950, 951, 952, 953, 954, 955, 956,
		957, 958, 959, 960, 961, 962, 963, 964, 965, 966, 967, 968, 977, 978, 982, 8226, 8230, 8242,
		8443, 8254, 8260, 8472, 8465, 8476, 8482, 8501, 8592, 8593, 8594, 8595, 8596, 8629, 8656,
		8657, 8658, 8659, 8660, 8704, 8706, 8707, 8709, 8711, 8712, 8713, 8715, 8719, 8721, 8722,
		8727, 8730, 8733, 8734, 8736, 8743, 8744, 8745, 8746, 8747, 8756, 8764, 8773, 8776, 8800,
		8801, 8804, 8805, 8834, 8835, 8836, 8838, 8839, 8853, 8855, 8869, 8901, 8968, 8969, 8970,
		8971, 9001, 9002, 9674, 9824, 9827, 9829, 9830, -1
};

static const gchar *entities_special[] = {
	"OElig", "oelig", "Scaron", "scaron", "Yuml", "circ", "tilde",
		"ensp", "emsp", "thinsp", "zwnj", "zwj", "lrm", "rlm", "ndash", "mdash", "lsquo", "rsquo",
		"sbquo", "ldquo", "rdquo", "bdquo", "dagger", "Dagger", "permil", "lsaquo", "rsaquo",
		"euro", NULL
};
static const gint16 entity_unicode_special[] = {
	338, 339, 352, 353, 376, 710, 732, 8194, 8195, 8201, 8204, 8205, 8206, 8207,
		8211, 8212, 8216, 8217, 8218, 8220, 8221, 8222, 8224, 8225, 8240, 8249, 8250, 8364, -1
};

static const gchar *entities_xml[] = {
	"nbsp", "quot", "amp", "lt", "gt", "apos", NULL
};
static const gint16 entity_unicode_xml[] = {
	160, '"', '&', '<', '>', '\'', -1
};

/* returns -1 if index not found */
static gint16 index_in_array(const gchar **arr,const gchar *string) {
	gint i;
	for (i=0;arr[i]!=NULL;i++) {
		if (strcmp(arr[i],string)==0) {
			return i;
		}
	}
	return -1;
}

static gint index_in_array2(const gint16 *arr, const guint val) {
	gint i;
	for (i=0;arr[i]!=-1;i++) {
		if (arr[i]==val) {
			return i;
		}
	}
	return -1;
}

gunichar xmlentity2unichar(const gchar *entity,gboolean numerical, gboolean iso8859_1, gboolean symbols, gboolean specials, gboolean xml) {
	gint16 indx;
	if (!entity) 
		return -1;

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

gchar *unichar2xmlentity(gunichar uchar, gboolean iso8859_1, gboolean symbols, gboolean specials, gboolean xml, gboolean numerical, gboolean IE_apos_workaround) {
	gint16 indx;
	if (iso8859_1 && uchar >= 161 && uchar < 255) {
		return g_strconcat("&", entities_iso8859_1[uchar - 161], ";", NULL);
	}
	if (symbols && (uchar == 402 || (uchar >= 913 && uchar <= 982) ||(uchar >= 8226 && uchar <= 9830))) {
		indx = index_in_array2(entity_unicode_symbols, uchar);
		if (indx != -1) return g_strconcat("&", entities_symbols[indx], ";", NULL);
	}
	if (specials && ((uchar >= 338 && uchar <= 376) || uchar == 710 || uchar == 732 || (uchar >= 8194 && uchar <= 8364) )) {
		indx = index_in_array2(entity_unicode_special, uchar);
		if (indx != -1) return g_strconcat("&", entities_special[indx], ";", NULL);
	}
	if (xml) {
		indx = index_in_array2(entity_unicode_xml, uchar);
		if (indx == 5 && IE_apos_workaround)
			return g_strconcat("&#39;", NULL);
		if (indx != -1) return g_strconcat("&", entities_xml[indx], ";", NULL);
	}
	if (uchar > 127 && numerical) {
		glong tmp=uchar;
		return g_strdup_printf("&#%ld;",tmp);
	}
	return NULL;
}

/* inbuf is a NULL terminated UTF8 string */
gchar *xmlentities2utf8(const gchar *inbuf) {
	const gchar *found, *prevfound;
	gchar *outbuf, *outbufloc;

	outbuf = g_malloc0(strlen(inbuf)+1);
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
			unic = xmlentity2unichar(entity, TRUE,TRUE,TRUE,TRUE,FALSE);
			if (unic != -1) {
				DEBUG_MSG("entities_to_utf8, unic=%d for entity '%s'\n",unic,entity);
				memset(tmp, 0, 7);
				g_unichar_to_utf8(unic, tmp);
				len = strlen(tmp);
				DEBUG_MSG("entities_to_utf8, tmp='%s', len=%d\n",tmp,len);
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

gchar *utf82xmlentities(const gchar *inbuf, gboolean iso8859_1, gboolean symbols, gboolean specials, gboolean xml, gboolean numerical, gboolean IE_apos_workaround) {
	gchar *outbuf;
	const gchar *srcp = inbuf;
	gunichar unichar = g_utf8_get_char(inbuf);
	/* we use a lot of memory for this function, but it makes the code simple */
	outbuf = g_malloc0(8 * strlen(inbuf)*sizeof(gchar));
	while (unichar) {
		gchar *entity = unichar2xmlentity(unichar, iso8859_1, symbols, specials, xml, numerical, IE_apos_workaround);
		if (entity) {
			outbuf = strncat(outbuf, entity, 8);
			g_free(entity);
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
