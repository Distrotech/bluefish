SUBDIRS = po

EXTRA_DIST = charmap.def

MAINTAINERCLEANFILES = Makefile.in

DEFS += -DLOCALEDIR=\"$(localedir)\"

if ENABLE_CHARMAP_PLUGIN
charmapdir = $(pkglibdir)
charmap_LTLIBRARIES = charmap.la
endif #ENABLE_CHARMAP_PLUGIN

charmap_la_SOURCES = \
	charmap.c \
	charmap.h \
	charmap_gui.c \
	charmap_gui.h

charmap_la_CFLAGS = $(CHARMAP_PLUGIN_CFLAGS)
charmap_la_LIBADD = $(CHARMAP_PLUGIN_LIBS)
charmap_la_LDFLAGS = -module -avoid-version

if OS_WIN32
charmap_la_LDFLAGS += -no-undefined -export-symbols $(srcdir)/charmap.def
endif

maintainer-clean-local:
	cd $(srcdir)/po && rm -f boldquot.sed en@boldquot.header en@quot.header insert-header.sin Makefile.in.in quot.sed remove-potcdate.sin Rules-quot $(PACKAGE)_plugin_charmap.pot

