void hl_init();
Thighlightset *hl_get_highlightset_by_type(gchar *type);
Thighlightset *hl_get_highlightset_by_filename(gchar *filename);
void hl_reset_highlighting_type(Tdocument * doc, gchar * newfilename);
void doc_highlight_full(Tdocument *doc);
void doc_highlight_line(Tdocument *doc);
void doc_remove_highlighting(Tdocument *doc);
void hl_reset_to_default();



