/* Bluefish HTML Editor
 * bluefish.h - global prototypes
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * Copyright (C) 1999-2002 Olivier Sessink
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* if you define DEBUG here you will get debug output from all Bluefish parts */
/* #define DEBUG */

#ifndef __BLUEFISH_H_
#define __BLUEFISH_H_

#include "config.h"
#define BLUEFISH_SPLASH_FILENAME PKGDATADIR"bluefish_splash.png"

#ifdef HAVE_SYS_MSG_H
#define WITH_MSG_QUEUE
#endif

#ifdef DEBUG
#define DEBUG_MSG g_print
#else /* not DEBUG */
#ifdef __GNUC__
#define DEBUG_MSG(args...)
 /**/
#else/* notdef __GNUC__ */
extern void g_none(...);
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

/*********************/
/* undo/redo structs */
/*********************/
typedef enum {
	UndoDelete = 1, UndoInsert
} undo_op_t;

typedef struct {
	GList *entries;	/* the list of entries that should be undone in one action */
	gint changed;		/* doc changed status at this undo node */
} unregroup_t;

typedef struct {
	GList *first;
	GList *last;
	unregroup_t *current;
	gint num_groups;
	GList *redofirst;
} unre_t;

/************************/
/* filetype struct      */
/************************/

typedef struct {
	gchar *type;
	gchar **extensions;
	GdkPixbuf *icon;
	gchar *update_chars;
	GList *highlightlist;
	gboolean editable; /* this a type that can be edited by Bluefish */
	gchar *content_regex; /* a regex pattern to test the filetype using the content */
} Tfiletype;

/*******************/
/* document struct */
/*******************/
#define BFWIN(var) ((Tbfwin *)(var))
#define DOCUMENT(var) ((Tdocument *)(var))

typedef struct {
	gchar *filename; /* or NULL if no filename known */
	gchar *encoding;
	gint modified;
/*	time_t mtime; */ /* from stat() */
	struct stat statbuf;
	gint is_symlink; /* file is a symbolic link */
	gulong del_txt_id; /* text delete signal */
	gulong ins_txt_id; /* text insert signal */
	gulong ins_aft_txt_id; /* text insert-after signal, for auto-indenting */
	unre_t unre;
	GtkWidget *view;
	GtkWidget *tab_label;
	GtkWidget *tab_menu;
	GtkTextBuffer *buffer;
	gint last_rbutton_event; /* index of last 3rd button click */
	Tfiletype *hl; /* filetype & highlighting set to use for this document */
	gint need_highlighting; /* if you open 10+ documents you don't need immediate highlighting, just set this var, and notebook_switch() will trigger the actual highlighting when needed */
	gboolean highlightstate; /* does this document use highlighting ? */
	gboolean wrapstate; /* does this document use wrap?*/
	gboolean linenumberstate; /* does this document use linenumbers? */
	gboolean overwrite_mode; /* is document in overwrite mode */
	gpointer floatingview; /* a 2nd textview widget that has its own window */
	gpointer bfwin;
} Tdocument;

typedef struct {
	gint view_html_toolbar;				/* view html toolbar */
	gint view_custom_menu;			/* view custom menubar */
	gint view_main_toolbar;				/* view main toolbar */
	gint view_left_panel;  	/* view filebrowser/functionbrowser etc. */
	gint view_line_numbers; /* view line numbers on the left side by default */
	gint filebrowser_show_hidden_files;
	gint filebrowser_show_others_files;
	gint filebrowser_show_backup_files;
	gchar *filebrowser_unknown_icon;
	gchar *filebrowser_dir_icon;
	gchar *editor_font_string;		/* editor font */
	gint editor_tab_width;	/* editor tabwidth */
	gint editor_indent_wspaces; /* indent with spaces, not tabs */
	gchar *tab_font_string;		/* notebook tabs font */
	GList *browsers; /* browsers array */
	GList *external_commands;	/* external commands array */
	GList *cust_menu; 		/* entries in the custom menu */
	GList *cmenu_insert; /* custom menu inserts */
	GList *cmenu_replace; /* custom menu replaces */
	GList *quickbar_items; /* items in the quickbar toolbar */
	gint highlight_num_lines_count; /* number of lines to highlight in continous highlighting */	
	gint defaulthighlight;		/* highlight documents by default */
	GList *filetypes; /* filetypes for highlighting and filtering */
	gint numcharsforfiletype; /* maximum number of characters in the file to use to find the filetype */
	GList *filefilters; /* filebrowser.c filtering */
	gchar *last_filefilter;	/* last filelist filter type */
	GList *highlight_patterns; /* the highlight patterns */
	gint transient_htdialogs;  /* set html dialogs transient ro the main window */
	gint restore_dimensions; /* use the dimensions as used the previous run */
	gint left_panel_width; 	/* width of filelist */
	gint main_window_h;			/* main window height */
	gint main_window_w;			/* main window width */
	gint max_recent_files;	/* length of Open Recent list */
	gint max_dir_history;	/* length of directory history */
	gint backup_file; 			/* wheather to use a backup file */
	gchar *backup_filestring;  /* the string to append to the backup file */
	gint backup_abort_action; /* if the backup fails, continue save, abort save, or ask the user */
	gint backup_cleanuponclose; /* remove the backupfile after close ? */
	gchar *image_thumbnailstring;	/* string to append to thumbnail filenames */
	gchar *image_thumbnailtype;	/* fileformat to use for thumbnails, "jpeg" or "png" can be handled by gdkpixbuf*/
	gint image_thumbnail_refresh_quality; /* 1=GDK_INTERP_BILINEAR, 0=GDK_INTERP_NEAREST*/
	gint image_thumbnailsizing_type;	/* scaling ratio=0, fixed width=1, height=2, width+height (discard aspect ratio)=3 */
	gint image_thumbnailsizing_val1;	/* the width, height or ratio, depending on the value above */
	gint image_thumbnailsizing_val2; /* height if the type=3 */
	gchar *image_thumnailformatstring; /* like <a href="%r"><img src="%t"></a> or more advanced */
	gint allow_multi_instances; /* allow multiple instances of the same file */
	gint modified_check_type; /* 0=no check, 1=by mtime and size, 2=by mtime, 3=by size, 4,5,...not implemented (md5sum?) */
	gint num_undo_levels; 	/* number of undo levels per document */
	gint clear_undo_on_save; 	/* clear all undo information on file save */
	gchar *newfile_default_encoding; /* if you open a new file, what encoding will it use */
	GList *encodings; /* all encodings you can choose from */
	gint auto_set_encoding_meta; /* auto set metatag for the encoding */
	gint auto_update_meta; /* auto update author and generator meta tags on save */
	GList *outputbox; /* all outputbox commands */
	gint ext_browsers_in_submenu;
	gint ext_commands_in_submenu;
	gint ext_outputbox_in_submenu;
	GList *reference_files; /* all reference files */
	gint fref_ldoubleclick_action; /* left doubleclick in the function reference */
	gint fref_info_type; /* type of info shown in a small function reference window */
	gint document_tabposition;
	gint leftpanel_tabposition;
#ifdef HAVE_LIBASPELL
	gchar *spell_default_lang;
#endif /* HAVE_LIBASPELL */
	gint cont_highlight_full; 	/* if you want to highlight the full text or just the line */
	gint lasttime_cust_menu; /* the last time the defaultfile was checked for new entries */
	gint lasttime_highlighting; /* see above */
	gint lasttime_filetypes; /* see above */
	gint lasttime_encodings; /* see above */
	/* not yet in use */
	gchar *image_editor_cline; 	/* image editor commandline */
	gint full_p;				/* use </p> */
	gint full_li;				/* use </li> */
	gint allow_css;				/* CSS allowed */
	gint allow_dep;				/* allow <FONT>... */
	gint format_by_context; 	/* use <strong> instead of <b>, <emphasis instead of <i> etc. (W3C reccomendation) */
	gint xhtml;					/* write <br /> */
	gint allow_ruby;			/* allow <ruby> */
	gint allow_h4;				/* allow <Q>... */
	gint allow_frames;			/* allow <FRAME> */
	gint force_dtd;				/* write <!DOCTYPE...> */
	gint dtd_url;				/* URL in DTD */
	gint xml_start;				/* <?XML...> */
	gint lowercase_tags;		/* use lowercase tags */
	gint word_wrap;				/* use wordwrap */
	gint autoindent;			/* autoindent code */
	gint fontset;				/* load fontset (0 = load font) */
	gint force_def_style;  /* force white editor background */
	gint drop_at_drop_pos; 	/* drop at drop position instead of cursor position */
	gint link_management; 	/* perform link management */
	gint cont_highlight_update;	/* update the syntax highlighting continuous */
	gchar *html_ver;
	/* key conversion */
	gint conv_ctrl_enter;		/* convert control-enter key press */
	gchar *ctrl_enter_text;		/* inserted text */
	gint conv_shift_enter;		/* convert shift-enter key press */
	gchar *shift_enter_text;	/* inserted text */
	gint conv_special_char;		/* convert ctrl-'<','>','&' */
	gint open_in_running_bluefish; /* open commandline documents in already running session*/
} Tproperties;

typedef struct {
	gchar *name;
	GList *files;
	gchar *basedir;
	gchar *webdir;
} Tproject;

typedef struct {
	Tdocument *current_document; /* one object out of the documentlist, the current visible document */
	GList *documentlist; /* document.c and others: all Tdocument objects */
	Tdocument *last_activated_doc;
	Tproject *project; /* might be NULL for a default project */
	GtkWidget *main_window;
	GtkWidget *menubar;
	gint last_notebook_page; /* a check to see if the notebook changed to a new page */
	gulong notebook_switch_signal;
	GtkWidget *notebook;
	GtkWidget *notebook_fake;
	GtkWidget *notebook_box; /* Container for notebook and notebook_fake */
	GtkWidget *middlebox; /* we need this to show/hide the filebrowser */
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
	GtkWidget *output_box;
	GtkWidget *leftpanel_notebook;
	/* following are lists with dynamic menu entries */
	GList *menu_recent_files;
	GList *menu_external;
	GList *menu_encodings;
	GList *menu_outputbox;
	GtkWidget *menu_cmenu;
	GList *menu_cmenu_entries;
	GList *menu_filetypes;
	/* following is a new approach, that we have only a gpointer here, whioh is typecasted 
	in the file where it is needed */
	gpointer outputbox;
	gpointer bfspell;
	gpointer filebrowser;
	gpointer snr2;
	gpointer fref;
} Tbfwin;

typedef struct {
	Tproperties props;
	Tbfwin *current_bfwin;
	GList *filetypelist; /* highlighting.c: a list of all filetypes with their icons and highlighting sets */
	GList *bfwinlist;
	GList *recent_directories; /* a stringlist with the most recently used directories */
	gpointer filebrowserconfig;
	gpointer frefdata;
} Tmain;

extern Tmain *main_v;

/* public functions from bluefish.c */
void bluefish_exit_request(void);
#endif /* __BLUEFISH_H_ */
