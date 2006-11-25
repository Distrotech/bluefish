
EXTRA_DIST = \
	bluefish_icon1.png \
	bookmarks.png \
	close.png \
	close_small.png \
	filetree.png \
	frcss.png \
	frdialog2.png \
	frdialog.png \
	fref.png \
	frfunction.png \
	frgroup.png \
	frhide.png \
	frinfo.png \
	frlibrary.png \
	frlibrary_ro.png \
	frnote.png \
	frsearch.png \
	frsnippet.png \
	frtag.png \
	frvar.png \
	new.png \
	nothing.png \
	open_small.png \
	prefs-editor.png \
	prefs-external.png \
	prefs-files.png \
	prefs-filetypes.png \
	prefs-html.png \
	prefs-images.png \
	prefs-interface.png \
	prefs-outputparser.png \
	prefs-syntax.png \
	view_in_browser.png
	
CLEANFILES = general_icons.c toolbar_icons.c

MAINTAINERCLEANFILES = Makefile.in

vpath %.png $(top_srcdir)/src/pixmaps

GENERAL = \
	pixmap_close_small $(top_srcdir)/src/pixmaps/close_small.png

TOOLBAR = \
	pixmap_bluefish_icon1 $(top_srcdir)/src/pixmaps/bluefish_icon1.png \
	pixmap_bookmarks $(top_srcdir)/src/pixmaps/bookmarks.png \
	pixmap_close $(top_srcdir)/src/pixmaps/close.png \
	pixmap_filetree $(top_srcdir)/src/pixmaps/filetree.png \
	pixmap_frcss $(top_srcdir)/src/pixmaps/frcss.png \
	pixmap_frdialog $(top_srcdir)/src/pixmaps/frdialog.png \
	pixmap_frdialog2 $(top_srcdir)/src/pixmaps/frdialog2.png \
	pixmap_fref $(top_srcdir)/src/pixmaps/fref.png \
	pixmap_frfunction $(top_srcdir)/src/pixmaps/frfunction.png \
	pixmap_frgroup $(top_srcdir)/src/pixmaps/frgroup.png \
	pixmap_frhide $(top_srcdir)/src/pixmaps/frhide.png \
	pixmap_frinfo $(top_srcdir)/src/pixmaps/frinfo.png \
	pixmap_frlibrary $(top_srcdir)/src/pixmaps/frlibrary.png \
	pixmap_frlibrary_ro $(top_srcdir)/src/pixmaps/frlibrary_ro.png \
	pixmap_frnote $(top_srcdir)/src/pixmaps/frnote.png \
	pixmap_frsearch $(top_srcdir)/src/pixmaps/frsearch.png \
	pixmap_frsnippet $(top_srcdir)/src/pixmaps/frsnippet.png \
	pixmap_frtag $(top_srcdir)/src/pixmaps/frtag.png \
	pixmap_frvar $(top_srcdir)/src/pixmaps/frvar.png \
	pixmap_new $(top_srcdir)/src/pixmaps/new.png \
	pixmap_nothing $(top_srcdir)/src/pixmaps/nothing.png \
	pixmap_open_small $(top_srcdir)/src/pixmaps/open_small.png \
	pixmap_prefseditor $(top_srcdir)/src/pixmaps/prefs-editor.png \
	pixmap_prefsexternal $(top_srcdir)/src/pixmaps/prefs-external.png \
	pixmap_prefsfiles $(top_srcdir)/src/pixmaps/prefs-files.png \
	pixmap_prefsfiletypes $(top_srcdir)/src/pixmaps/prefs-filetypes.png \
	pixmap_prefshtml $(top_srcdir)/src/pixmaps/prefs-html.png \
	pixmap_prefsimages $(top_srcdir)/src/pixmaps/prefs-images.png \
	pixmap_prefsinterface $(top_srcdir)/src/pixmaps/prefs-interface.png \
	pixmap_prefsoutputparser $(top_srcdir)/src/pixmaps/prefs-outputparser.png \
 	pixmap_prefssyntax $(top_srcdir)/src/pixmaps/prefs-syntax.png \
	pixmap_view_in_browser $(top_srcdir)/src/pixmaps/view_in_browser.png


general_icons.c:
	gdk-pixbuf-csource --raw --extern --build-list $(GENERAL) > $@

toolbar_icons.c:
	gdk-pixbuf-csource --raw --extern --build-list $(TOOLBAR) > $@


all-local: general_icons.c toolbar_icons.c

