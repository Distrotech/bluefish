gchar **array_from_arglist(const gchar *string1, ...);
void rcfile_parse_main(void);
void rcfile_parse_highlighting(void);
void rcfile_parse_custom_menu(void);
void rcfile_check_directory(void);
void rcfile_save_configfile_menu_cb(gpointer callback_data,guint action,GtkWidget *widget);
void rcfile_save_all(void);
