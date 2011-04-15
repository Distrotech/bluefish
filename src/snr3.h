typedef enum {
	snr3type_string,
	snr3type_pcre
} Tsnr3type;

typedef enum {
	snr3scope_doc,
	snr3scope_cursor,
	snr3scope_selection,
	snr3scope_alldocs,
	snr3scope_files
} Tsnr3scope;

typedef enum {
	snr3replace_string,
	snr3replace_upper,
	snr3replace_lower
} Tsnr3replace;

typedef struct {
	gint so;
	gint eo;
} Tsnr3submatches;

typedef struct {
	gint so;
	gint eo;
	gpointer doc;
	gpointer extra; /* for submatches and such */
} Tsnr3result;

typedef enum {
	alldocs,
	singledocupdate
} Tsnr3workmode;

typedef struct {
	gpointer querydata; /* for example the compiled pcre pattern */
	Tdocument *curdoc;
	gchar *curbuf;
	guint so;
	guint eo;
	guint curoffset;	
	Tsnr3workmode workmode;
	/*void *(*callback) ();*/
	GCallback callback;
} Tsnr3working;

typedef struct {
	Tbfwin *bfwin;
	gchar *query;
	gchar *replace; /* enabled if not NULL */
	Tsnr3type type;
	Tsnr3scope scope;
	gboolean is_case_sens;
	gboolean overlapping;
	gboolean escape_chars;
	gboolean select_matches;
	gboolean bookmark_matches;
	Tsnr3working *working;
	guint idle_id;
	GQueue results;
	GList *current;
	gboolean want_submatches;
} Tsnr3run;


typedef struct {
	gpointer nothing;
} Tsnr3config;

#define MAX_CONTINUOUS_SEARCH_INTERVAL 0.1

void snr3_run_go(Tsnr3run *s3run, gboolean forward);
void snr3run_free(Tsnr3run *s3run);
gpointer simple_search_run(Tbfwin *bfwin, const gchar *string);
void snr3_advanced_dialog(Tbfwin *bfwin);
