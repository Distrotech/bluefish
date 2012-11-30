/* Bluefish HTML Editor
 * bluefish.h - global prototypes
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * Copyright (C) 1999-2012 Olivier Sessink
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
/* indented with indent -ts4 -kr -l110   */

/*#define IDENTSTORING*/

/* #define HL_PROFILING */
/* if you define DEBUG here you will get debug output from all Bluefish parts */
/* #define DEBUG */

#ifndef __BLUEFISH_H_
#define __BLUEFISH_H_

/*#define MEMORY_LEAK_DEBUG*/
/*#define DEBUG_PATHS*/

#define ENABLEPLUGINS

#include "config.h"

#ifndef DEVELOPMENT
#define G_DISABLE_ASSERT
#endif

#define BLUEFISH_SPLASH_FILENAME PKGDATADIR"/bluefish_splash.png"

#ifdef WIN32
#include <windows.h>
#ifdef EXE_EXPORT_SYMBOLS
#define EXPORT __declspec(dllexport)
#else							/* EXE_EXPORT_SYMBOLS */
#define EXPORT __declspec(dllimport)
#endif							/* EXE_EXPORT_SYMBOLS */
#else							/* WIN32 */
#define EXPORT
#endif							/* WIN32 */

#ifdef HAVE_SYS_MSG_H
#ifdef HAVE_MSGRCV
#ifdef HAVE_MSGSND
/*#define WITH_MSG_QUEUE*/
#endif
#endif
#endif

#ifdef DEBUG
#define DEBUG_MSG g_print
#define DEBUG_MSG_C g_critical
#define DEBUG_MSG_E g_error
#define DEBUG_MSG_W g_warning
#else							/* not DEBUG */
#if defined(__GNUC__) || defined(__SUNPRO_C) && (__SUNPRO_C > 0x580)
#define DEBUG_MSG(args...)
#define DEBUG_MSG_C(args...)
#define DEBUG_MSG_E(args...)
#define DEBUG_MSG_W(args...)
 /**/
#else							/* notdef __GNUC__ || __SUNPRO_C */
extern void g_none(char * first, ...);
#define DEBUG_MSG g_none
#define DEBUG_MSG_C g_none
#define DEBUG_MSG_E g_none
#define DEBUG_MSG_W g_none
#endif							/* __GNUC__ || __SUNPRO_C */
#endif							/* DEBUG */

#ifdef DEBUG_PATHS
#define DEBUG_PATH g_print
#else
#if defined(__GNUC__) || defined(__SUNPRO_C)
#define DEBUG_PATH(args...)
 /**/
#else							/* notdef __GNUC__ || __SUNPRO_C */
extern void g_none(gchar * first, ...);
#endif							/* __GNUC__ || __SUNPRO_C */
#endif							/* DEBUG */

#ifdef ENABLE_NLS
#include <glib/gi18n.h>
#else							/* ENABLE_NLS */
#define _(String)(String)
#define N_(String)(String)
#define ngettext(Msgid1, Msgid2, N) \
	((N) == 1 \
	? ((void) (Msgid2), (const char *) (Msgid1)) \
	: ((void) (Msgid1), (const char *) (Msgid2)))
#endif							/* ENABLE_NLS */


#ifdef WIN32
#define DIRSTR "\\"
#define DIRCHR 92
#else							/* WIN32 */
#define DIRSTR "/"
#define DIRCHR '/'
#endif							/* WIN32 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gio/gio.h>

#define BF_FILEINFO "standard::name,standard::display-name,standard::size,standard::type,unix::mode,unix::uid,unix::gid,time::modified,time::modified-usec,etag::value,standard::fast-content-type"
#include "bftextview2.h"

#ifndef G_GOFFSET_FORMAT
#define G_GOFFSET_FORMAT G_GINT64_FORMAT
#endif


/*************************************/
/*** priorities for the main loop ****/
/*************************************/

/*
G_PRIORITY_HIGH -100 			Use this for high priority event sources. It is not used within GLib or GTK+.
G_PRIORITY_DEFAULT 0 			Use this for default priority event sources. In GLib this priority is used when adding 
										timeout functions with g_timeout_add(). 
G_PRIORITY_HIGH_IDLE 100 		Use this for high priority idle functions. 
G_PRIORITY_DEFAULT_IDLE 200 	Use this for default priority idle functions. In GLib this priority is used when adding idle 
										functions with g_idle_add().
G_PRIORITY_LOW 300

GDK  uses   0 for events from the X server.
GTK+ uses 110 for resizing operations 
GTK+ uses 120 for redrawing operations. (This is done to ensure 
										that any pending resizes are processed before any pending 
										redraws, so that widgets are not redrawn twice unnecessarily.)
*/

/* inserting data into a GtkTextBuffer should be in a lower priority than 
the drawing of the GUI, otherwise the bluefish GUI won't show when loading 
large files from the commandline. 
I don't understand what it interacts with, but 145 is a too high priority
so set it lower to 155 */
#define FILEINTODOC_PRIORITY 155
#define FILE2DOC_PRIORITY 155
/* doc activate will stop scanning for the old document and schedule 
for the new document. Set it between the X event (0) and the normal
gtk events (100) */
#define NOTEBOOKCHANGED_DOCACTIVATE_PRIORITY 50
/* set between a X event (0) and a normal event (100) */
#define SCANNING_IDLE_PRIORITY 10 
/* set idle after timeout scanning to 115. 
		a higher priority (109 is too high) makes bluefish go greyed-out 
		(it will not redraw if required while the loop is running)
	   and a much lower priority (199 is too low) will first draw 
	   all textstyles on screen before the next burst of scanning is done */
#define SCANNING_IDLE_AFTER_TIMEOUT_PRIORITY 115	
/*  make sure that we don't scan or spellcheck a file that will be scanned again we do timeout
scanning in a lower priority timeout than the language file notice 
so a newly loaded language file uses a priority 113 event to notice all documents to be rescanned. */
#define BUILD_LANG_FINISHED_PRIORITY 113

#define BFLANGSCAN_FINISHED_PRIORITY 101

/*********************/
/* undo/redo structs */
/*********************/
typedef enum {
	UndoDelete = 1, UndoInsert
} undo_op_t;

typedef struct {
	GList *entries;				/* the list of entries that should be undone in one action */
	gint32 changed;				/* doc changed status at this undo node */
	guint32 action_id;
} unregroup_t;

typedef struct {
	GList *first;
	GList *last;
	unregroup_t *current;
	GList *redofirst;
	gint num_groups;
} unre_t;

/*****************************************************/
/* filter struct - used in filebrowser2 and gtk_easy */
/*****************************************************/
typedef struct {
	gchar *name;
	GHashTable *filetypes;		/* hash table with mime types */
	GList *patterns;
	gushort refcount;
	gushort mode;				/* 0= hide matching files, 1=show matching files */
} Tfilter;

/********************************************************************/
/* document struct, used everywhere, most importantly in document.c */
/********************************************************************/
#define BFWIN(var) ((Tbfwin *)(var))
#define DOCUMENT(var) ((Tdocument *)(var))
#define CURDOC(bfwin) ((Tdocument *)bfwin->current_document)

typedef enum {
	DOC_STATUS_ERROR,
	DOC_STATUS_LOADING,
	DOC_STATUS_COMPLETE,
	DOC_CLOSING
} Tdocstatus;

typedef struct {
	GFile *uri;
	GFileInfo *fileinfo;
	Tdocstatus status;			/* can be DOC_STATUS_ERROR, DOC_STATUS_LOADING, DOC_STATUS_COMPLETE, DOC_CLOSING */
	gchar *encoding;
	gint modified;

	/* if an action is set, this action has to be executed after the document finishing closing/opening */
	gpointer save;				/* during document save */
	gpointer info;				/* during update of the fileinfo */
	gpointer checkmodified;		/* during check modified on disk checking */
	gpointer load;				/* during load */
	gint goto_line;
	gint goto_offset;
	gushort close_doc;
	gushort close_window;

	GList *need_autosave;		/* if autosave is needed, a direct pointer to main_v->need_autosave; */
	GList *autosave_progress;
	gpointer autosave_action;
	GList *autosaved;			/* NULL if no autosave registration, else this is a direct pointer into the main_v->autosave_journal list */
	GFile *autosave_uri;		/* if autosaved, the URI of the autosave location, else NULL */
	gint readonly;
	gboolean block_undo_reg; 	/* block the registration for undo */
	guint newdoc_autodetect_lang_id;	/* a timer function that runs for new documents to detect their mime type  */
	unre_t unre;
	GtkWidget *view;
	GtkWidget *slave; /* used in split view for the bottom view */
	GtkWidget *vsplit; /* used for split view */
	GtkWidget *tab_label;
	GtkWidget *tab_eventbox;
	GtkWidget *tab_menu;
	GtkTextBuffer *buffer;
	gboolean in_paste_operation;
	gboolean highlightstate;	/* does this document use highlighting ? */
	gpointer floatingview;		/* a 2nd textview widget that has its own window */
	gpointer bfwin;
	GList *recentpos; 	/* this points to the list element in the recent used tabs (bfwin->recentdoclist) that points to this Tdocument */
	GtkTreeIter *bmark_parent;	/* if NULL this document doesn't have bookmarks, if
								   it does have bookmarks they are children of this GtkTreeIter */
} Tdocument;

typedef struct {
	gchar *config_version; /* bluefish version string */
	gint do_periodic_check;
	gchar *editor_font_string;	/* editor font */
	gint editor_smart_cursor;
	gint editor_tab_indent_sel; /* tab key will indent a selected block */
	gint editor_auto_close_brackets;
	gint use_system_tab_font;
	gchar *tab_font_string;		/* notebook tabs font */
	/*  gchar *tab_color_normal; *//* notebook tabs text color normal.  This is just NULL! */
	gchar *tab_color_modified;	/* tab text color when doc is modified and unsaved */
	gchar *tab_color_loading;	/* tab text color when doc is loading */
	gchar *tab_color_error;		/* tab text color when doc has errors */
	gint visible_ws_mode;
	gint right_margin_pos;
	/* new replacements: */
	GList *external_command;	/* array: name,command,is_default_browser */
	GList *external_filter;		/* array: name,command */
	GList *external_outputbox;	/* array:name,command,....... */
	/*gint defaulthighlight; *//* highlight documents by default */
	gint leave_to_window_manager;	/* don't set any dimensions, leave all to window manager */
	gint restore_dimensions;	/* use the dimensions as used the previous run */
	gint left_panel_left;		/* 1 = left, 0 = right */
	gint hide_bars_on_fullscreen;
	gint cursor_size;
	gint highlight_cursor;
	gint save_accelmap; 	/* save the accelerator map on exit */
	gint max_recent_files;		/* length of Open Recent list */
	gint max_dir_history;		/* length of directory history */
	gint backup_file;			/* wheather to use a backup file */
	/* GIO has hardcoded backup file names */
/*	gchar *backup_suffix;  / * the string to append to the backup filename */
/*	gchar *backup_prefix;  / * the string to prepend to the backup filename (between the directory and the filename) */
	gint backup_abort_action;	/* if the backup fails, 0=continue save  , 1=abort save, 2=ask the user */
	gint backup_cleanuponclose;	/* remove the backupfile after close ? */
	gchar *image_thumbnailstring;	/* string to append to thumbnail filenames */
	gchar *image_thumbnailtype;	/* fileformat to use for thumbnails, "jpeg" or "png" can be handled by gdkpixbuf */
	gint modified_check_type;	/* 0=no check, 1=by mtime and size, 2=by mtime, 3=by size, 4,5,...not implemented (md5sum?) */
	gint num_undo_levels;		/* number of undo levels per document */
	gint clear_undo_on_save;	/* clear all undo information on file save */
	gchar *newfile_default_encoding;	/* if you open a new file, what encoding will it use */
	gint auto_set_encoding_meta;	/* auto set metatag for the encoding */
	gint auto_update_meta_author;	/* auto update author meta tag on save */
	gint auto_update_meta_date;	/* auto update date meta tag on save */
	gint auto_update_meta_generator;	/* auto update generator meta tag on save */
	gint strip_trailing_spaces_on_save;
	gint encoding_search_Nbytes;	/* number of bytes to look for the encoding meta tag */
	gint max_window_title; /* max. number of chars in the window title */
	gint document_tabposition;
	gint leftpanel_tabposition;
	gint switch_tabs_by_altx;
	gchar *project_suffix;
	/* not yet in use */
	gint allow_dep;				/* allow <FONT>... */
	gint format_by_context;		/* use <strong> instead of <b>, <emphasis instead of <i> etc. (W3C reccomendation) */
	gint xhtml;					/* write <br /> */
	gint smartindent;			/* add extra indent in certain situations */
	/* key conversion */
	gint open_in_running_bluefish;	/* open commandline documents in already running process */
	gint open_in_new_window;	/* open commandline files in a new window as opposed to an existing window */
	gint register_recent_mode; /* 0=none,1=all,2=project only*/
	GList *plugin_config;		/* array, 0=filename, 1=enabled, 2=name */
	gint use_system_colors;
	gchar *btv_color_str[BTV_COLOR_COUNT];	/* editor colors */
	GList *textstyles;			/* tet styles: name,foreground,background,weight,style */
	gint block_folding_mode;
	GList *highlight_styles;
	GList *bflang_options;		/* array with: lang_name, option_name, value */
	gchar *autocomp_accel_string;
	gboolean load_reference;
	gboolean show_autocomp_reference;
	gboolean show_tooltip_reference;
	gboolean delay_full_scan;
	gint delay_scan_time;
	gint autocomp_popup_mode;	/* delayed or immediately */
	gboolean reduced_scan_triggers;
	gint autosave;
	gint autosave_time;
	gint autosave_location_mode;	/* 0=~/.bluefish/autosave/, 1=original basedir */
	gchar *autosave_file_prefix;
	gchar *autosave_file_suffix;
	gchar *language;
	gint rcfile_from_old_version;
	gint adv_textview_right_margin; /* advanced option for the textview margin */
	gint adv_textview_left_margin; /* advanced option for the textview margin */
} Tproperties;

/* the Tglobalsession contains all settings that can change 
over every time you run Bluefish, so things that *need* to be
saved after every run! */
typedef struct {
	gint main_window_h;			/* main window height */
	gint main_window_w;			/* main window width */
	gint two_pane_filebrowser_height;	/* position of the pane separater on the two paned file browser */
	gint left_panel_width;		/* width of filelist */
	gint bookmarks_default_store;	/* 0= temporary by default, 1= permanent by default */
	gint image_thumbnail_refresh_quality;	/* 1=GDK_INTERP_BILINEAR, 0=GDK_INTERP_NEAREST */
	gint image_thumbnailsizing_type;	/* scaling ratio=0, fixed width=1, height=2, width+height (discard aspect ratio)=3 */
	gint image_thumbnailsizing_val1;	/* the width, height or ratio, depending on the value above */
	gint image_thumbnailsizing_val2;	/* height if the type=3 */
	gchar *image_thumnailformatstring;	/* like <a href="%r"><img src="%t"></a> or more advanced */
	gint filter_on_selection_mode; /* 0=ask, 1=selection, 2=text */
	GList *filefilters;			/* filefilter.c file filtering */
	GList *reference_files;		/* all reference files */
	GList *recent_projects;
	GList *encodings;			/* all encodings you can choose from, array with 0=human name, 1=name, 2="0" or "1" if it should be user visible or not */
#ifdef WITH_MSG_QUEUE
	gint msg_queue_poll_time;	/* milliseconds, automatically tuned to your system */
#endif
} Tglobalsession;

typedef struct {
	gint enable_syntax_scan; /* syntax scan by default */
	gint wrap_text_default;		/* by default wrap text */
	gint autoindent;			/* autoindent code */
	gint editor_tab_width;		/* editor tabwidth */
	gint editor_indent_wspaces;	/* indent with spaces, not tabs */
	gint view_line_numbers;		/* view line numbers on the left side by default */
	gint view_cline;			/* highlight current line by default */
	gint view_blocks;			/* show blocks on the left side by default */
	gint view_blockstack;
	gint autocomplete;			/* whether or not to enable autocomplete by default for each new document */
	gint show_mbhl;				/* show matching block begin-end by default */

	/* snr3 advanced search and replace */
	gint snr3_type;
	gint snr3_replacetype;
	gint snr3_scope;
	gint snr3_casesens;
	gint snr3_escape_chars;
	gint snr3_dotmatchall;
	gint snr3_recursion_level;
	/* simplesearch options */
	gint ssearch_regex;
	gint ssearch_dotmatchall;
	gint ssearch_unescape;
	gint ssearch_casesens;

	gint sync_delete_deprecated;
	gint sync_include_hidden;
	gint adv_open_matchname;
	gint adv_open_recursive;
	gint bookmarks_filename_mode;	/* 0=FULLPATH, 1=DIR FROM BASE 2=BASENAME */
	gint bookmarks_show_mode;	/* 0=both,1=name,2=content */
	gint bmarksearchmode;
	gint filebrowser_focus_follow;	/* have the directory of the current document in focus */
	gint filebrowser_show_backup_files;
	gint filebrowser_show_hidden_files;
	gint filebrowser_viewmode;	/* 0=tree, 1=dual or 2=flat */
	gint snr_position_x;
	gint snr_position_y;
	gint leftpanel_active_tab;
	gint view_left_panel;		/* view filebrowser/functionbrowser etc. */
	gint view_main_toolbar;		/* view main toolbar */
	gint view_statusbar;
	gint outputb_scroll_mode;	/* 0=none, 1=first line, 2= last line */
	gint outputb_show_all_output;
	gint convertcolumn_horizontally;
	gint display_right_margin;
	gint show_visible_spacing;
	/* 44 * sizeof(gint) */
	/* IF YOU EDIT THIS STRUCTURE PLEASE EDIT THE CODE IN PROJECT.C THAT COPIES
	   A Tsessionvar INTO A NEW Tsessionvar AND ADJUST THE SIZES!!!!!!!!!!!!!!!!!!!!!! */
#ifdef HAVE_LIBENCHANT
	gint spell_check_default;
	gint spell_insert_entities;
	gchar *spell_lang;
#endif
	/* if you add strings or lists to the session, please make sure they are free'ed 
	in free_session() in project.c */
	gchar *ssearch_text;
	gchar *default_mime_type;
	gchar *template;
	gchar *convertcolumn_separator;
	gchar *convertcolumn_fillempty;
	gchar *webroot;
	gchar *documentroot;
	gchar *encoding;
	gchar *last_filefilter;		/* last filelist filter type */
	gchar *opendir;
	gchar *savedir;
	gchar *sync_local_uri;
	gchar *sync_remote_uri;
	GList *bmarks;
	GList *classlist;
	GList *colorlist;
	GList *fontlist;
	GList *positionlist;		/* is this used ?? */
	GList *recent_dirs;
	GList *recent_files;
	GList *replacelist;			/* used in snr2 */
	GList *searchlist;			/* used in snr2 and for advanced_open */
	GList *filegloblist; /* file glob filters in advanced open and search in files */
	GList *targetlist;
	GList *urllist;
} Tsessionvars;

typedef struct {
	GFile *uri;
	gchar *name;
	GList *files;
	gpointer editor;
	Tsessionvars *session;
	gpointer bmarkdata;			/* project bookmarks */
	gboolean close;				/* if this is TRUE, it means the project is saved and all,
								   so after all documents are closed it just just be cleaned up and discarded */
} Tproject;

typedef struct {
	Tsessionvars *session;		/* points to the global session, or to the project session */
	Tdocument *current_document;	/* one object out of the documentlist, the current visible document */
	gboolean focus_next_new_doc;	/* for documents loading in the background, switch to the first that is finished loading */
	gint num_docs_not_completed;	/* number of documents that are loading or closing */
	GList *documentlist;		/* document.c and others: all Tdocument objects in the order of the tabs */
	GList *recentdoclist; /* all Tdocument objects with the most recently used on top, every Tdocument has a pointer to it's own list element called doc->recentpos */
	Tdocument *last_activated_doc;
	Tproject *project;			/* might be NULL for a default project */
	GtkWidget *main_window;
	GtkWidget *toolbarbox;		/* vbox on top, with main and html toolbar */

	/* Main Menus & toolbar */
	GtkUIManager *uimanager;
	GtkActionGroup *globalGroup;
	GtkActionGroup *documentGroup;
	GtkActionGroup *editGroup;
	GtkActionGroup *findReplaceGroup;
	GtkActionGroup *projectGroup;
	GtkActionGroup *undoGroup;
	GtkActionGroup *bookmarkGroup;
	GtkActionGroup *filebrowserGroup;
	guint filebrowser_merge_id;

	GtkWidget *menubar;
	gint last_notebook_page;	/* a check to see if the notebook changed to a new page */
	guint notebook_changed_doc_activate_id;
	guint statusbar_pop_id;
	guint notebook_switch_signal;
	guint update_searchhistory_idle_id;
	GtkWidget *gotoline_entry;
	GtkWidget *simplesearch_combo;
	GtkWidget *simplesearch_regex;
	GtkWidget *simplesearch_casesens;
	GtkWidget *simplesearch_dotmatchall;
	GtkWidget *simplesearch_unescape;
	gpointer simplesearch_snr3run;
	GtkWidget *notebook;
	GtkWidget *notebook_fake;
	GtkWidget *notebook_box;	/* Container for notebook and notebook_fake */
	GtkWidget *middlebox;		/* holds the document notebook, OR the hpaned with the left panel AND the document notebook */
	GtkWidget *vpane;			/* holds the middlebox AND the outputbox (which might be NULL) */
	GtkWidget *hpane;			/* we need this to show/hide the filebrowser */
	GtkWidget *statusbar;
	GtkWidget *statusbar_lncol;	/* where we have the line number */
	GtkWidget *statusbar_insovr;	/* insert/overwrite indicator */
	GtkWidget *statusbar_editmode;	/* editor mode and doc encoding */
	/* the following list contains toolbar widgets we like to reference later on */
	GtkWidget *toolbar_quickbar;	/* the quickbar widget */
	GList *toolbar_quickbar_children;	/* this list is needed to remove widgets from the quickbar */
	/* following widgets are used to show/hide stuff */
	GtkWidget *main_toolbar_hb;
	GtkWidget *html_toolbar_hb;
	GtkWidget *leftpanel_notebook;
	GtkWidget *gotoline_frame;
	/* action based dynamic menus */
	GtkActionGroup *templates_group;
	guint templates_merge_id;
	GtkActionGroup *lang_mode_group;
	guint lang_mode_merge_id;
	GtkActionGroup *commands_group;
	guint commands_merge_id;
	GtkActionGroup *filters_group;
	guint filters_merge_id;
	GtkActionGroup *outputbox_group;
	guint outputbox_merge_id;
	GtkActionGroup *encodings_group;
	guint encodings_merge_id;
	GtkActionGroup *recent_group;
	GtkActionGroup *fb2_filters_group;
	guint fb2_filters_merge_id;
#ifdef HAVE_LIBENCHANT
	gpointer *ed;				/* EnchantDict */
#endif
	/* following is a new approach, that we have only a gpointer here, whioh is typecasted 
	   in the file where it is needed */
	gpointer outputbox;
	gpointer bfspell;
	gpointer fb2;				/* filebrowser2 gui */
	gpointer snr2;
	GtkTreeView *bmark;
	GtkTreeModelFilter *bmarkfilter;
	gchar *bmark_search_prefix;
	gpointer bmarkdata;			/* a link to the global main_v->bmarkdata, OR project->bmarkdata */
#ifdef IDENTSTORING
	GHashTable *identifier_jump;
	GHashTable *identifier_ac;
#endif /* IDENTSTORING */
	GSList *curdoc_changed; /* register a CurdocChangedCallback function here that is called when the current document changes*/
	GSList *doc_insert_text; /* register a DocInsertTextCallback function here that is called when text is inserted into a document */
	GSList *doc_delete_range; /* register a DocDeleteRangeCallback function here that is called when text is deleted from a document */
	GSList *doc_destroy; /* register a DocDestroyCallback function here that is called when a document is destroyed  */
} Tbfwin;

typedef struct {
	Tproperties props;			/* preferences */
	gpointer prefdialog;		/* preferences window, there should be only 1 */
	Tglobalsession globses;		/* global session */
	GList *autosave_journal;	/* holds an arraylist with autosaved documents */
	gboolean autosave_need_journal_save;
	GList *need_autosave;		/* holds Tdocument pointers */
	GList *autosave_progress;	/* holds Tdocument pointers that are being saved right now */
	guint autosave_id;			/* used with g_timeout_add */
	guint periodic_check_id;	/* used with g_timeout_add */
	GList *bfwinlist;
	Tsessionvars *session;		/* holds all session variables for non-project windows */
	gpointer fb2config;			/* filebrowser2config */
	GList *filefilters;			/* initialized by fb2config functions */
	GList *templates; 			/* loaded in rcfile.c */
	Tdocument *bevent_doc;
	gint bevent_charoffset; 	/* for example used in the spellcheck code to find on which 
											word the user clicked */
	guint autocomp_accel_key;				 /* by default <ctrl><space> activates autocompletion */
	GdkModifierType autocomp_accel_mods; /* but this shortcut is also used to switch input languages for example by chinese users */
	gpointer bmarkdata;
	gint num_untitled_documents;
	gchar *securedir;			/* temporary rwx------ directory for secure file creation */
	GtkRecentManager *recentm;
	GSList *plugins;
	GSList *doc_view_populate_popup_cbs;	/* plugins can register functions here that need to
											   be called when the right-click menu in the document is populated */
	GSList *doc_view_button_press_cbs;	/* plugins can register functions here that are called on a button press
										   in a document */
	GSList *sidepanel_initgui;	/* plugins can register a function here that is called when the side pane
								   is initialized */
	GSList *sidepanel_destroygui;	/* plugins can register a function here that is called when the side pane
									   is destroyed */
	GSList *pref_initgui; /* register a PrefInitguiCallback function here to add a preferences panel */
	GSList *pref_apply; /* PrefApplyCallback */
} Tmain;

extern EXPORT Tmain *main_v;

/* public functions from bluefish.c */
void bluefish_exit_request(void);

/* Avoid lots of warnings from API depreciated in GTK 3.0. -Wdeprecated-declarations */
#if GTK_CHECK_VERSION(3,0,0)
#define gtk_hbox_new(arg, arg2) gtk_box_new(GTK_ORIENTATION_HORIZONTAL, arg2)
#define gtk_vbox_new(arg, arg2) gtk_box_new(GTK_ORIENTATION_VERTICAL, arg2)
#endif

/* backwards compatibility */
#if !GTK_CHECK_VERSION(2,24,0)
#define GDK_KEY_Enter GDK_Enter
#define GDK_KEY_Return GDK_Return
#define GDK_KEY_KP_Enter GDK_KP_Enter
#define GDK_KEY_Home GDK_Home
#define GDK_KEY_KP_Home GDK_KP_Home
#define GDK_KEY_End GDK_End
#define GDK_KEY_KP_End GDK_KP_End
#define GDK_KEY_Tab GDK_Tab
#define GDK_KEY_KP_Tab GDK_KP_Tab
#define GDK_KEY_ISO_Left_Tab GDK_ISO_Left_Tab
#define GDK_KEY_Up GDK_Up
#define GDK_KEY_Down GDK_Down
#define GDK_KEY_Page_Down GDK_Page_Down
#define GDK_KEY_Page_Up GDK_Page_Up
#define GDK_KEY_Right GDK_Right
#define GDK_KEY_KP_Right GDK_KP_Right
#define GDK_KEY_Left GDK_Left
#define GDK_KEY_KP_Left GDK_KP_Left
#define GDK_KEY_Escape GDK_Escape
#define GDK_KEY_0 GDK_0
#define GDK_KEY_1 GDK_1
#define GDK_KEY_2 GDK_2
#define GDK_KEY_3 GDK_3
#define GDK_KEY_4 GDK_4
#define GDK_KEY_5 GDK_5
#define GDK_KEY_6 GDK_6
#define GDK_KEY_7 GDK_7
#define GDK_KEY_8 GDK_8
#define GDK_KEY_9 GDK_9
#define GDK_KEY_F1 GDK_F1 
#define GDK_KEY_F12 GDK_F12
#define GDK_KEY_Delete GDK_Delete
#define GDK_KEY_BackSpace GDK_BackSpace
#define GDK_KEY_KP_Delete GDK_KP_Delete
#define GDK_KEY_Alt_L GDK_Alt_L
#define GDK_KEY_Alt_R GDK_Alt_R
#define GDK_KEY_Control_L GDK_Control_L
#define GDK_KEY_Control_R GDK_Control_R

/*#define GDK_KEY_ GDK_*/
#define GTK_COMBO_BOX_TEXT(arg) GTK_COMBO_BOX(arg)
#define gtk_combo_box_text_get_active_text gtk_combo_box_get_active_text
#define gtk_combo_box_text_new_with_entry gtk_combo_box_entry_new_text 
#define gtk_combo_box_text_new gtk_combo_box_new_text
#define gtk_combo_box_text_append_text gtk_combo_box_append_text
#define gtk_combo_box_text_prepend_text gtk_combo_box_prepend_text
#define gtk_combo_box_text_remove gtk_combo_box_remove_text
#endif /* GTK_CHECK_VERSION(2,24,0) */

#endif							/* __BLUEFISH_H_ */
