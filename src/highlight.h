
void filetype_highlighting_rebuild();
void hl_init();
void doc_highlight_full(Tdocument *doc);
void doc_highlight_line(Tdocument *doc);
void doc_remove_highlighting(Tdocument *doc);

GtkTextTagTable *highlight_return_tagtable();


