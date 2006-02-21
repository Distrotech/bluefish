/* Bluefish HTML Editor
 * bluefish.h - global prototypes
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * Copyright (C) 1999-2006 Olivier Sessink
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* if you define DEBUG here you will get debug output from all Bluefish parts */
/* #define DEBUG */

#ifndef __BLUEFISH_H_
#define __BLUEFISH_H_

#define ENABLEPLUGINS

#include "config.h"
#define BLUEFISH_SPLASH_FILENAME PKGDATADIR"bluefish_splash.png"

#ifdef HAVE_SYS_MSG_H
#ifdef HAVE_MSGRCV
#ifdef HAVE_MSGSND
#define WITH_MSG_QUEUE
#endif
#endif
#endif

#ifdef DEBUG
#define DEBUG_MSG g_print
#else /* not DEBUG */
#ifdef __GNUC__
#define DEBUG_MSG(args...)
 /**/
#else/* notdef __GNUC__ */
extern void g_none(gchar *first, ...);
#define DEBUG_MSG g_none
#endif /* __GNUC__ */
#endif /* DEBUG */

#ifdef ENABLE_NLS

#include <libintl.h>
#define _(String) gettext (String)
#define N_(String) (String)

#else                                                                           

#define _(String)(String)
#define N_(String)(String)

#endif    


#define DIRSTR "/"
#define DIRCHR '/'

#include <sys/types.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pcre.h>

#include <libgnomevfs/gnome-vfs.h>

#ifdef USE_SCANNER
#include "bf-textview.h"
#endif
/*********************/
/* undo/redo structs */
/*********************/
typedef enum {
	UndoDelete = 1, UndoInsert
} undo_op_t;

typedef struct {
	GList *entries;	/* the list of entries that should be undone in one action */
	gint changed;		/* doc changed status at this undo node */
	guint action_id;
} unregroup_t;

typedef struct {
	GList *first;
	GList *last;
	unregroup_t *current;
	gint num_groups;
	GList *redofirst;
} unre_t;

/******************************************************************************************/
/* filetype struct, used for filtering, highlighting/scanning, and as document property   */
/******************************************************************************************/

typedef struct {
	gchar *type;
	gchar **extensions;
	GdkPixbuf *icon;
	gchar *update_chars;
	GList *highlightlist;
	gboolean editable; /* this a type that can be edited by Bluefish */
	gint autoclosingtag; /* 0=off, 1=xml mode, 2=html mode */
	gchar *content_regex; /* a regex pattern to test the filetype using the content */
#ifdef USE_SCANNER
	gchar *language_file;
	BfLangConfig *cfg;
#endif	
} Tfiletype;

/*****************************************************/
/* filter struct - used in filebrowser2 and gtk_easy */
/*****************************************************/
typedef struct {
	gchar *name;
	gboolean mode; /* 0= hide matching files, 1=show matching files */
	GList *filetypes; /* if NULL all files are OK */
} Tfilter;

/********************************************************************/
/* document struct, used everywhere, most importantly in document.c */
/********************************************************************/
#define BFWIN(var) ((Tbfwin *)(var))
#define DOCUMENT(var) ((Tdocument *)(var))
#define CURDOC(bfwin) (bfwin->current_document)
#define FILETYPE(var) ((Tfiletype *)(var))

typedef enum {
	DOC_STATUS_ERROR,
	DOC_STATUS_LOADING,
	DOC_STATUS_COMPLETE,
	DOC_CLOSING
} Tdocstatus;

/* if an action is set, this action has to be executed after the document finishing closing/opening */
typedef struct {
	gint goto_line;
	gint goto_offset;
	gboolean close_doc;
	gboolean close_window;
	gpointer save; /* during document save */
	gpointer info; /* during update of the fileinfo */
	gpointer checkmodified; /* during check modified on disk checking */
	gpointer load; /* during load */
} Tdoc_action;

typedef struct {
	GnomeVFSURI *uri;
	Tdoc_action action; /* see above, if set, some action has to be executed after opening/closing is done */
/*	gchar *filename;  this is the UTF-8 encoded filename, before you use it on disk you need convert to disk-encoding! */
	Tdocstatus status; /* can be DOC_STATUS_ERROR, DOC_STATUS_LOADING, DOC_STATUS_COMPLETE, DOC_CLOSING */
	gchar *encoding;
	gint modified;
	GnomeVFSFileInfo *fileinfo;
	gint is_symlink; /* file is a symbolic link */
	gulong del_txt_id; /* text delete signal */
	gulong ins_txt_id; /* text insert signal */
	gulong ins_aft_txt_id; /* text insert-after signal, for auto-indenting */
	unre_t unre;
	GtkWidget *view;
	GtkWidget *tab_label;
	GtkWidget *tab_eventbox;
	GtkWidget *tab_menu;
	GtkTextBuffer *buffer;
	gpointer paste_operation;
	gint last_rbutton_event; /* index of last 3rd button click */
	Tfiletype *hl; /* filetype & highlighting set to use for this document */
	gint need_highlighting; /* if you open 10+ documents you don't need immediate highlighting, just set this var, and notebook_switch() will trigger the actual highlighting when needed */
	gboolean highlightstate; /* does this document use highlighting ? */
	gboolean wrapstate; /* does this document use wrap?*/
	gboolean linenumberstate; /* does this document use linenumbers? */
#ifdef USE_SCANNER	
	gboolean blocksstate; /* does this document show blocks? */
	gboolean symstate; /* does this document show symbols? */	
#endif	
	gboolean overwrite_mode; /* is document in overwrite mode */
	gboolean autoclosingtag; /* does the document use autoclosing of tags */
	gpointer floatingview; /* a 2nd textview widget that has its own window */
	gpointer bfwin;
	GtkTreeIter *bmark_parent; /* if NULL this document doesn't have bookmarks, if 
									it does have bookmarks they are children of this GtkTreeIter */
} Tdocument;

typedef struct {
	gint do_periodic_check;
	gint view_line_numbers; /* view line numbers on the left side by default */
	gint filebrowser_two_pane_view; /* have one or two panes in the filebrowser */
	gchar *filebrowser_unknown_icon;
	gchar *filebrowser_dir_icon;
	gchar *editor_font_string;		/* editor font */
	gint editor_tab_width;	/* editor tabwidth */
	gint editor_smart_cursor;
	gint editor_indent_wspaces; /* indent with spaces, not tabs */
	gchar *tab_font_string;		/* notebook tabs font */
	
	/* old entries */
	GList *browsers; /* DEPRECATED browsers array ( <=1.0.1) */
	GList *external_commands;	/* DEPRECATED external commands array ( <=1.0.1) */
	GList *outputbox; /* DEPRECATED all outputbox commands ( <=1.0.1) */
	/* new replacements: */
	GList *external_command; /* array: name,command,is_default_browser */
	GList *external_filter; /* array: name,command */
	GList *external_outputbox; /* array:name,command,.......*/
	
	GList *cust_menu; 		/* DEPRECATED entries in the custom menu */
	GList *cmenu_insert; /* custom menu inserts */
	GList *cmenu_replace; /* custom menu replaces */
	gint highlight_num_lines_count; /* number of lines to highlight in continous highlighting */	
	gint defaulthighlight;		/* highlight documents by default */
	GList *filetypes; /* filetypes for highlighting and filtering */
	gint numcharsforfiletype; /* maximum number of characters in the file to use to find the filetype */
	GList *filefilters; /* filebrowser.c filtering */
#ifndef USE_SCANNER
	GList *highlight_patterns; /* the highlight patterns */
#endif
	gint transient_htdialogs;  /* set html dialogs transient ro the main window */
	gint restore_dimensions; /* use the dimensions as used the previous run */
	gint left_panel_left; /* 1 = left, 0 = right */
	gint max_recent_files;	/* length of Open Recent list */
	gint max_dir_history;	/* length of directory history */
	gint backup_file; 			/* wheather to use a backup file */
	gchar *backup_filestring;  /* the string to append to the backup file */
	gint backup_abort_action; /* if the backup fails, 0=continue save  , 1=abort save, 2=ask the user */
	gint backup_cleanuponclose; /* remove the backupfile after close ? */
	gchar *image_thumbnailstring;	/* string to append to thumbnail filenames */
	gchar *image_thumbnailtype;	/* fileformat to use for thumbnails, "jpeg" or "png" can be handled by gdkpixbuf*/
	gint modified_check_type; /* 0=no check, 1=by mtime and size, 2=by mtime, 3=by size, 4,5,...not implemented (md5sum?) */
	gint num_undo_levels; 	/* number of undo levels per document */
	gint clear_undo_on_save; 	/* clear all undo information on file save */
	gchar *newfile_default_encoding; /* if you open a new file, what encoding will it use */
	GList *encodings; /* all encodings you can choose from */
	gint auto_set_encoding_meta; /* auto set metatag for the encoding */
	gint auto_update_meta_author; /* auto update author meta tag on save */
	gint auto_update_meta_date; /* auto update date meta tag on save */
	gint auto_update_meta_generator; /* auto update generator meta tag on save */
	gint encoding_search_Nbytes; /* number of bytes to look for the encoding meta tag */
	gint document_tabposition;
	gint leftpanel_tabposition;
	gchar *project_suffix;
	/* not yet in use */
	gchar *image_editor_cline; 	/* image editor commandline */
	gint allow_dep;				/* allow <FONT>... */
	gint format_by_context; 	/* use <strong> instead of <b>, <emphasis instead of <i> etc. (W3C reccomendation) */
	gint xhtml;					/* write <br /> */
	gint allow_ruby;			/* allow <ruby> */
	gint force_dtd;				/* write <!DOCTYPE...> */
	gint dtd_url;				/* URL in DTD */
	gint xml_start;				/* <?XML...> */
	gint lowercase_tags;		/* use lowercase tags */
	gint word_wrap;				/* use wordwrap */
	gint autoindent;			/* autoindent code */
	gint drop_at_drop_pos; 	/* drop at drop position instead of cursor position */
	gint link_management; 	/* perform link management */
	gint cont_highlight_update;	/* update the syntax highlighting continuous */
	/* key conversion */
	gint open_in_running_bluefish; /* open commandline documents in already running session*/
	gint server_zope_compat;        /* add 'document_src' to uri when reading remote files */
	gint show_splash_screen;
	gchar *bflib_info_font;
	gchar *bflib_info_bkg;
	gchar *bflib_info_fg;
	GList *plugin_config; /* array, 0=filename, 1=enabled, 2=name*/
#ifdef USE_SCANNER
	gint view_blocks; /* show blocks on the left side by default */
	gint view_symbols; /* show symbols on the left side by default */	
	gint view_mbhl; /* show matching block begin-end by default */	
	gint view_cline; /* highlight current line by default */
	gint scan_mode; /* number of lines to autoscan */
	gchar *editor_fg; /* editor foreground color */
	gchar *editor_bg; /* editor background color */
	GList *syntax_styles; /* textstyles (see below) for each detected bit of syntax */
	gint view_rmargin; /* show right margin by default */
	gint rmargin_at; /* position of a right margin */
#endif	
	GList *textstyles; /* tet styles: name,foreground,background,weight,style */
} Tproperties;

/* the Tglobalsession contains all settings that can change 
over every time you run Bluefish, so things that *need* to be
saved after every run! */
typedef struct {
	gint main_window_h;			/* main window height */
	gint main_window_w;			/* main window width */
	gint two_pane_filebrowser_height; /* position of the pane separater on the two paned file browser */
	gint left_panel_width; 	/* width of filelist */
	gint fref_ldoubleclick_action; /* left doubleclick in the function reference */
	gint fref_info_type; /* type of info shown in a small function reference window */
	gint lasttime_cust_menu; /* the last time the defaultfile was checked for new entries */
#ifndef USE_SCANNER
	gint lasttime_highlighting; /* see above */
#endif
	gint lasttime_filetypes; /* see above */
	gint lasttime_encodings; /* see above */
	gint bookmarks_default_store; /* 0= temporary by default, 1= permanent by default */
	gint image_thumbnail_refresh_quality; /* 1=GDK_INTERP_BILINEAR, 0=GDK_INTERP_NEAREST*/
	gint image_thumbnailsizing_type;	/* scaling ratio=0, fixed width=1, height=2, width+height (discard aspect ratio)=3 */
	gint image_thumbnailsizing_val1;	/* the width, height or ratio, depending on the value above */
	gint image_thumbnailsizing_val2; /* height if the type=3 */
	gchar *image_thumnailformatstring; /* like <a href="%r"><img src="%t"></a> or more advanced */
	GList *reference_files; /* all reference files */
	GList *recent_projects;
} Tglobalsession;

typedef struct {
	GList *classlist;
	GList *colorlist;
	GList *targetlist;
	GList *urllist;
	GList *fontlist;
	GList *positionlist; /* is this used ?? */
	GList *searchlist; /* used in snr2 and for advanced_open */
	GList *replacelist; /* used in snr2 */
	GList *bmarks;
	GList *recent_files;
	GList *recent_dirs;
	gchar *opendir;
	gchar *savedir;
	gchar *encoding;
	gint   adv_open_recursive;
	gchar *last_filefilter;	/* last filelist filter type */
	gint view_custom_menu;			/* view custom menubar */
	gint view_main_toolbar;				/* view main toolbar */
	gint view_left_panel;  	/* view filebrowser/functionbrowser etc. */
	gint filebrowser_show_hidden_files;
	gint filebrowser_show_backup_files;
	gint filebrowser_focus_follow; /* have the directory of the current document in focus */
	gint bookmarks_filename_mode; /* 0=FULLPATH, 1=DIR FROM BASE 2=BASENAME */
	gint bookmarks_show_mode; /* 0=both,1=name,2=content */
#ifdef HAVE_LIBASPELL
	gchar *spell_default_lang;
#endif /* HAVE_LIBASPELL */
} Tsessionvars;

typedef struct {
	gchar *filename;
	gchar *name;
	GList *files;
	gchar *basedir;
	gchar *webdir;
	gchar *template;
	gpointer editor;
	gint word_wrap;
	Tsessionvars *session;
	gpointer bmarkdata; /* project bookmarks */
	gboolean close; /* if this is TRUE, it means the project is saved and all, 
	so after all documents are closed it just just be cleaned up and discarded */
} Tproject;

typedef struct {
	Tsessionvars *session; /* points to the global session, or to the project session */
	Tdocument *current_document; /* one object out of the documentlist, the current visible document */
	gboolean focus_next_new_doc; /* for documents loading in the background, switch to the first that is finished loading */
	gint num_docs_not_completed; /* number of documents that are loading or closing */
	GList *documentlist; /* document.c and others: all Tdocument objects */
	Tdocument *last_activated_doc;
	Tproject *project; /* might be NULL for a default project */
	GtkWidget *main_window;
	GtkWidget *toolbarbox; /* vbox on top, with main and html toolbar */
	GtkWidget *menubar;
	gint last_notebook_page; /* a check to see if the notebook changed to a new page */
	gulong notebook_switch_signal;
	guint periodic_check_id; /* used with g_timeout_add */
	GtkWidget *notebook;
	GtkWidget *notebook_fake;
	GtkWidget *notebook_box; /* Container for notebook and notebook_fake */
	GtkWidget *middlebox; /* holds the document notebook, OR the hpaned with the left panel AND the document notebook */
	GtkWidget *vpane; /* holds the middlebox AND the outputbox (which might be NULL) */
	GtkWidget *hpane; /* we need this to show/hide the filebrowser */
	GtkWidget *statusbar;
	GtkWidget *statusbar_lncol; /* where we have the line number */
	GtkWidget *statusbar_insovr; /* insert/overwrite indicator */
	GtkWidget *statusbar_editmode; /* editor mode and doc encoding */
	/* the following list contains toolbar widgets we like to reference later on */
	GtkWidget *toolbar_undo;
	GtkWidget *toolbar_redo;
	GtkWidget *toolbar_quickbar; /* the quickbar widget */
	GList *toolbar_quickbar_children; /* this list is needed to remove widgets from the quickbar */
	/* following widgets are used to show/hide stuff */
	GtkWidget *main_toolbar_hb;
	GtkWidget *html_toolbar_hb;
	GtkWidget *custom_menu_hb; /* handle box for custom menu */
	GtkWidget *leftpanel_notebook;
	/* following are lists with dynamic menu entries */
	GList *menu_recent_files;
	GList *menu_recent_projects;
	GList *menu_external;
	GList *menu_encodings;
	GList *menu_outputbox;
	GList *menu_windows;
	GtkWidget *menu_cmenu;
	GList *menu_cmenu_entries;
	GList *menu_filetypes;
	/* following is a new approach, that we have only a gpointer here, whioh is typecasted 
	in the file where it is needed */
	gpointer outputbox;
	gpointer bfspell;
/*	gpointer filebrowser;*/
	gpointer fb2; /* filebrowser2 gui */
	gpointer snr2;
	gpointer fref;
	gpointer bmark;
	gpointer bmarkdata; /* a link to the global main_v->bmarkdata, OR project->bmarkdata */
/*	GtkTreeStore *bookmarkstore; / * this is a link to project->bookmarkstore OR main_v->bookmarkstore
											  and it is only here for convenience !!!! * /
	GHashTable *bmark_files;     / * no way, I have to have list of file iters. Other way I 
	                                cannot properly load bmarks for closed files */
} Tbfwin;

typedef struct {
	Tproperties props; /* preferences */
	Tglobalsession globses; /* global session */
	GList *filetypelist; /* highlighting.c: a list of all filetypes with their icons and highlighting sets */
	GList *bfwinlist;
/*	GList *recent_directories; / * a stringlist with the most recently used directories */
	Tsessionvars *session; /* holds all session variables for non-project windows */
	gpointer filebrowserconfig;
	gpointer fb2config; /* filebrowser2config */
	GList *filefilters; /* initialized by fb2config functions */
	gpointer frefdata;
	gpointer bmarkdata;
/* 	GtkTreeStore *bookmarkstore; the global bookmarks from the global session */
	gint num_untitled_documents;
	GtkTooltips *tooltips;
	guint16 lastkp_hardware_keycode; /* for the autoclosing, we need to know the last pressed key, in the key release callback, */
	guint lastkp_keyval;             /* this is different if the modifier key is not pressed anymore during the key-release */
	pcre *autoclosingtag_regc; /* the regular expression to check for a valid tag in tag autoclosing*/
	gchar *securedir; /* temporary rwx------ directory for secure file creation */
	GSList *plugins;
	GSList *doc_view_populate_popup_cbs;
	GSList *doc_view_button_press_cbs;
#ifdef USE_SCANNER
	BfLangManager *lang_mgr;
#endif	
} Tmain;

extern Tmain *main_v;

/* public functions from bluefish.c */
void bluefish_exit_request(void);
#endif /* __BLUEFISH_H_ */
