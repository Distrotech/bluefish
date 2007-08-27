
SUBDIRS = po

DISTCLEANFILES = @OPENBSD_3_8_FIX@

MAINTAINERCLEANFILES = Makefile.in

DEFS += -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\"

aboutdir = $(pkglibdir)
about_LTLIBRARIES = about.la

about_la_SOURCES = \
	about.c \
	about.h

about_la_LDFLAGS = -module -avoid-version

if OS_WIN32
about_la_LDFLAGS += -no-undefined
endif

maintainer-clean-local:
	cd $(srcdir)/po && rm -f boldquot.sed en@boldquot.header en@quot.header insert-header.sin Makefile.in.in quot.sed remove-potcdate.sin Rules-quot $(PACKAGE)_plugin_about.pot

