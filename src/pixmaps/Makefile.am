
EXTRA_DIST = $(TOOLBAR_ICONS)

CLEANFILES = toolbar_icons.c

MAINTAINERCLEANFILES = Makefile.in


GENERAL =

TOOLBAR_ICONS = \
	bluefish_icon1.png \
	bookmarks.png \
	filetree.png \
	view_in_browser.png

TOOLBAR = \
	pixmap_bluefish_icon1 $(srcdir)/bluefish_icon1.png \
	pixmap_bookmarks $(srcdir)/bookmarks.png \
	pixmap_filetree $(srcdir)/filetree.png \
	pixmap_view_in_browser $(srcdir)/view_in_browser.png


toolbar_icons.c: $(TOOLBAR_ICONS)
	$(GDK_PIXBUF_CSOURCE) --raw --extern --build-list $(TOOLBAR) > $@


BUILT_SOURCES = toolbar_icons.c

