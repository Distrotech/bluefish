#ifndef _BFTEXTVIEW2_LANGMGR_H_
#define _BFTEXTVIEW2_LANGMGR_H_

#include "bftextview2.h"

GtkTextTagTable *langmgr_get_tagtable(void);
Tbflang *langmgr_get_bflang_for_mimetype(const gchar *mimetype);
GList *langmgr_get_languages(void);
void langmgr_init(GList *user_styles, GList *user_highlight_styles, gboolean load_reference);

#endif /* _BFTEXTVIEW2_LANGMGR_H_ */
