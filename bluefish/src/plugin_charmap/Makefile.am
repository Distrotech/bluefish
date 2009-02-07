
SUBDIRS =

DISTCLEANFILES = @OPENBSD_3_8_FIX@

MAINTAINERCLEANFILES = Makefile.in

DEFS += -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\"

charmapdir = $(pkglibdir)
charmap_LTLIBRARIES = charmap.la

charmap_la_SOURCES = \
	charmap.c \
	charmap_gui.c

charmap_la_LDFLAGS = -module -avoid-version

if OS_WIN32
charmap_la_LDFLAGS += -no-undefined
endif

#maintainer-clean-local:
#	cd $(srcdir)/po && rm -f boldquot.sed en@boldquot.header en@quot.header insert-header.sin Makefile.in.in quot.sed remove-potcdate.sin Rules-quot $(PACKAGE)_plugin_about.pot

