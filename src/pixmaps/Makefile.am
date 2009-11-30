
EXTRA_DIST = $(GENERAL_ICONS) $(TOOLBAR_ICONS)

CLEANFILES = general_icons.c toolbar_icons.c

MAINTAINERCLEANFILES = Makefile.in

GENERAL_ICONS =

GENERAL =

TOOLBAR_ICONS = \
	bluefish_icon1.png \
	bookmarks.png \
	close.png \
	filetree.png \
	new.png \
	nothing.png \
	open_small.png \
	view_in_browser.png

TOOLBAR = \
	pixmap_bluefish_icon1 $(srcdir)/bluefish_icon1.png \
	pixmap_bookmarks $(srcdir)/bookmarks.png \
	pixmap_close $(srcdir)/close.png \
	pixmap_filetree $(srcdir)/filetree.png \
	pixmap_new $(srcdir)/new.png \
	pixmap_nothing $(srcdir)/nothing.png \
	pixmap_open_small $(srcdir)/open_small.png \
	pixmap_view_in_browser $(srcdir)/view_in_browser.png


general_icons.c: $(GENERAL_ICONS)
	$(GDK_PIXBUF_CSOURCE) --raw --extern --build-list $(GENERAL) > $@

toolbar_icons.c: $(TOOLBAR_ICONS)
	$(GDK_PIXBUF_CSOURCE) --raw --extern --build-list $(TOOLBAR) > $@


BUILT_SOURCES = general_icons.c toolbar_icons.c

