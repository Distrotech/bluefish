
EXTRA_DIST = \
	bluefish_icon1.png \
	bookmarks.png \
	close.png \
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


GENERAL =

TOOLBAR = \
	pixmap_bluefish_icon1 $(srcdir)/bluefish_icon1.png \
	pixmap_bookmarks $(srcdir)/bookmarks.png \
	pixmap_close $(srcdir)/close.png \
	pixmap_filetree $(srcdir)/filetree.png \
	pixmap_frcss $(srcdir)/frcss.png \
	pixmap_frdialog $(srcdir)/frdialog.png \
	pixmap_frdialog2 $(srcdir)/frdialog2.png \
	pixmap_fref $(srcdir)/fref.png \
	pixmap_frfunction $(srcdir)/frfunction.png \
	pixmap_frgroup $(srcdir)/frgroup.png \
	pixmap_frhide $(srcdir)/frhide.png \
	pixmap_frinfo $(srcdir)/frinfo.png \
	pixmap_frlibrary $(srcdir)/frlibrary.png \
	pixmap_frlibrary_ro $(srcdir)/frlibrary_ro.png \
	pixmap_frnote $(srcdir)/frnote.png \
	pixmap_frsearch $(srcdir)/frsearch.png \
	pixmap_frsnippet $(srcdir)/frsnippet.png \
	pixmap_frtag $(srcdir)/frtag.png \
	pixmap_frvar $(srcdir)/frvar.png \
	pixmap_new $(srcdir)/new.png \
	pixmap_nothing $(srcdir)/nothing.png \
	pixmap_open_small $(srcdir)/open_small.png \
	pixmap_prefseditor $(srcdir)/prefs-editor.png \
	pixmap_prefsexternal $(srcdir)/prefs-external.png \
	pixmap_prefsfiles $(srcdir)/prefs-files.png \
	pixmap_prefsfiletypes $(srcdir)/prefs-filetypes.png \
	pixmap_prefshtml $(srcdir)/prefs-html.png \
	pixmap_prefsimages $(srcdir)/prefs-images.png \
	pixmap_prefsinterface $(srcdir)/prefs-interface.png \
	pixmap_prefsoutputparser $(srcdir)/prefs-outputparser.png \
 	pixmap_prefssyntax $(srcdir)/prefs-syntax.png \
	pixmap_view_in_browser $(srcdir)/view_in_browser.png


general_icons.c:
	gdk-pixbuf-csource --raw --extern --build-list $(GENERAL) > $@

toolbar_icons.c:
	gdk-pixbuf-csource --raw --extern --build-list $(TOOLBAR) > $@


all-local: general_icons.c toolbar_icons.c

