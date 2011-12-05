
SUBDIRS = po

EXTRA_DIST = xmltools.def

MAINTAINERCLEANFILES = Makefile.in

DEFS += -DLOCALEDIR=\"$(localedir)\"

xmltoolsdir = $(pkglibdir)
xmltools_LTLIBRARIES = xmltools.la

xmltools_la_SOURCES = \
	xmltools.c \
	xmltools.h

xmltools_la_LDFLAGS = -module -avoid-version

if OS_WIN32
xmltools_la_LDFLAGS += -no-undefined -export-symbols $(srcdir)/xmltools.def
endif

maintainer-clean-local:
	cd $(srcdir)/po && rm -f boldquot.sed en@boldquot.header en@quot.header insert-header.sin Makefile.in.in quot.sed remove-potcdate.sin Rules-quot $(PACKAGE)_plugin_xmltools.pot

