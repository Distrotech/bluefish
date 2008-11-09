
GtkTextTagTable *langmgr_get_tagtable(void);

Tbflang *langmgr_get_bflang_for_mimetype(const gchar *mimetype);
void langmgr_init(GList *user_styles, GList *user_highlight_styles, gboolean load_reference);
