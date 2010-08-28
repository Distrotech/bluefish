/* Bluefish HTML Editor
 * snippets_leaf_snr.h
 *
 * Copyright (C) 2006 Olivier Sessink
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

#ifndef SNIPPETS_LEAF_SNR_H
#define SNIPPETS_LEAF_SNR_H

gint snippets_snr_region_from_char(const xmlChar *region);
gint snippets_snr_matchtype_from_char(const xmlChar *matchtype);
void snippets_activate_leaf_snr(Tsnippetswin *snw, xmlNodePtr parent);


#endif /* SNIPPETS_LEAF_SNR_H */
