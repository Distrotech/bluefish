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
#define DEBUG_MSG(format, args...)
 /**/
#else/* notdef __GNUC__ */
extern void g_none(...);
#define DEBUG_MSG g_none
#endif /* __GNUC__ */
#endif /* DEBUG */

#define _(String) (String)
#define N_(String) (String)

#define DIRSTR "/"
#define DIRCHR '/'

#include <sys/types.h>
#include <regex.h>

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
	GtkWidget *menuitem; /* the menuitem to choose this for highlighting */
	gulong menuitem_activate_id;
} Tfiletype;

/*******************/
/* document struct */
/*******************/

typedef struct {
	gchar *filename;
	gchar *encoding;
	gint modified;
	time_t mtime; /* from stat() */
	uid_t owner_uid; /* user ID of owner */
	gid_t owner_gid; /* group ID of owner */
	mode_t mode; /* mode field in stat() format so any stat macro's will work*/
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
	gboolean highlightstate;
	gboolean wrapstate;
} Tdocument;

typedef struct {
	gint view_html_toolbar;				/* view html toolbar */
	gint view_custom_menu;			/* view custom menubar */
	gint view_main_toolbar;				/* view main toolbar */
	gint view_left_panel;  	/* view filebrowser/functionbrowser etc. */
	gint filebrowser_show_hidden_files;
	gint filebrowser_show_others_files;
	gint filebrowser_show_backup_files;
	gchar *filebrowser_unknown_icon;
	gchar *filebrowser_dir_icon;
	gchar *editor_font_string;		/* editor font */
	gint editor_tab_width;	/* editor tabwidth */
	gchar *tab_font_string;		/* notebook tabs font */
	GList *browsers; /* browsers array */
	GList *external_commands;	/* external commands array */
	GList *quickbar_items; /* items in the quickbar toolbar */
	gint highlight_num_lines_count; /* number of lines to highlight in continous highlighting */	
	GList *filetypes; /* filetypes for highlighting and filtering */
	GList *filefilters; /* filebrowser.c filtering */
	gchar *last_filefilter;	/* last filelist filter type */
	GList *highlight_patterns; /* the highlight patterns */
	GList *highlight_styles; /* the highlight styles */
	gint transient_htdialogs;  /* set html dialogs transient ro the main window */
	gint restore_dimensions; /* use the dimensions as used the previous run */
	gint left_panel_width; 	/* width of filelist */
	gint main_window_h;			/* main window height */
	gint main_window_w;			/* main window width */
	gint max_recent_files;	/* length of Open Recent list */
	gint max_dir_history;	/* length of directory history */
	gint backup_file; 			/* wheather to use a backup file */
	gchar *backup_filestring;  /* the string to append to the backup file */
	gchar *backup_abort_style; /* if the backup fails, continue 'save', 'abort' save, or 'ask' user */
	gchar *image_thumbnailstring;	/* string to append to thumbnail filenames */
	gchar *image_thumbnailtype;	/* fileformat to use for thumbnails, "jpeg" or "png" can be handled by gdkpixbuf*/
	gint image_thumbnail_refresh_quality; /* 1=GDK_INTERP_BILINEAR, 0=GDK_INTERP_NEAREST*/
	gint image_thumbnailsizing_type;	/* scaling ratio=0, fixed width=1, height=2, width+height (discard aspect ratio)=3 */
	gint image_thumbnailsizing_val1;	/* the width, height or ratio, depending on the value above */
	gint image_thumbnailsizing_val2; /* height if the type=3 */
	gchar *image_thumnailformatstring; /* like <a href="%r"><img src="%t"></a> or more advanced */
	gint allow_multi_instances; /* allow multiple instances of the same file */
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

	/* not yet in use */
	gchar *image_editor_cline; 	/* image editor commandline */
	gchar *cfg_weblint_cline;	/* weblint command line */
	gchar *cfg_tab_pos;			/* notebook tabs positioning */
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
	gint defaulthighlight;		/* highlight documents by default */
	gint cont_highlight_full; 	/* if you want to highlight the full text or just the line */
	gint cont_highlight_update;	/* update the syntax highlighting continuous */
	gchar *html_ver;
	GList *cust_menu; 		/* entries in the custom menu */
#ifdef WITH_SPC
	/* spell checker options */
	gchar *cfg_spc_cline      ;  /* spell checker command line */
	gchar *cfg_spc_lang       ;  /* language */
	gint  spc_accept_compound ;  /* accept compound words ? */
	gint  spc_use_esc_chars   ;  /* specify aditional characters that
                                     may be part of a word ? */
	gchar* spc_esc_chars      ;  /* which ones ? */
	gint   spc_use_pers_dict  ;  /* use a personal dictionary */
	gchar* spc_pers_dict      ;  /* which one ? */
   gint   spc_use_input_encoding ;  /* use input encoding */
   gchar* spc_input_encoding     ;  /* wich one ? */
   gint   spc_output_html_chars  ; /* output html chars ? (like &aacute;)*/
#endif
	/* key conversion */
	gint conv_ctrl_enter;		/* convert control-enter key press */
	gchar *ctrl_enter_text;		/* inserted text */
	gint conv_shift_enter;		/* convert shift-enter key press */
	gchar *shift_enter_text;	/* inserted text */
	gint conv_special_char;		/* convert ctrl-'<','>','&' */
	gint open_in_running_bluefish; /* open commandline documents in already running session*/
#ifdef AUTOCOMPLET
	gint autocomplet_html;   /* Shows autocompletion window when editing HTML */
	gint autocomplet_delay;  /* Delay to show the autocompletion window */
#endif
#ifdef PARSEDTD
	GList *doctypes;	 /* The list of doctypes recognized by bluefish */
#endif
} Tproperties;

typedef struct {
	Tdocument *current_document; /* one object out of the documentlist, the current visible document */
	Tproperties props;
	GList *documentlist; /* document.c and others: all Tdocument objects */
	GList *filetypelist; /* highlighting.c: a list of all filetypes with their icons and highlighting sets */
	GtkWidget *main_window;
	GtkWidget *menubar;
	gint last_notebook_page; /* a check to see if the notebook changed to a new page */
	gulong notebook_switch_signal;
	GtkWidget *notebook;
	GtkWidget *middlebox; /* we need this to show/hide the filebrowser */
	GtkWidget *hpane; /* we need this to show/hide the filebrowser */
	GtkWidget *statusbar;
	GtkWidget *statuslabel; /* where we have the line number */
	GList *recent_directories; /* a stringlist with the most recently used directories */
} Tmain;

extern Tmain *main_v;

/* public functions from bluefish.c */
void bluefish_exit_request();
#endif /* __BLUEFISH_H_ */
