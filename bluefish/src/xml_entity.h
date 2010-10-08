/* Bluefish HTML Editor
 * xml_entity.h - XML and HTML entity conversion
 *
 * Copyright (C) 2010 Olivier Sessink.
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

#ifndef __XML_ENTITY_H_
#define __XML_ENTITY_H_

gunichar xmlentity2unichar(const gchar *entity,gboolean numerical, gboolean iso8859_1, gboolean symbols, gboolean specials, gboolean xml);
gchar *xmlentities2utf8(const gchar *inbuf);

gchar *unichar2xmlentity(gunichar uchar, gboolean iso8859_1, gboolean symbols, gboolean specials, gboolean xml, gboolean numerical, gboolean IE_apos_workaround);
gchar *utf82xmlentities(const gchar *inbuf, gboolean iso8859_1, gboolean symbols, gboolean specials, gboolean xml, gboolean numerical, gboolean IE_apos_workaround);

#endif /* __XML_ENTITY_H_  */
