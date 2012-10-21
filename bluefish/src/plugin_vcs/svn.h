
void svn_status(Tbfwin *bfwin, const gchar *basedir, gpointer data);
void svn_commit_real(Tbfwin *bfwin, Tvcssession *vs, GList *files, const gchar *message);
gboolean svn_autoconfigure(Tvcssession *vs, GFile *cururi);
void svn_update(Tbfwin *bfwin, const gchar *basedir, gpointer data);

