void init_output_box(GtkWidget *vbox);
void outputbox(gchar *pattern, gint file_subpat, gint line_subpat, gint output_subpat, gchar *command, gboolean show_all_output);

void outputbox_make();
void outputbox_weblint();
void outputbox_tidy();
void outputbox_javac();
