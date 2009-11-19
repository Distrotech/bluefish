
SUBDIRS = po

EXTRA_DIST = entities.def

MAINTAINERCLEANFILES = Makefile.in

DEFS += -DLOCALEDIR=\"$(localedir)\"

entitiesdir = $(pkglibdir)
entities_LTLIBRARIES = entities.la

entities_la_SOURCES = \
	entities.c \
	entities.h

entities_la_LDFLAGS = -module -avoid-version

if OS_WIN32
entities_la_LDFLAGS += -no-undefined -export-symbols $(srcdir)/entities.def
endif

maintainer-clean-local:
	cd $(srcdir)/po && rm -f boldquot.sed en@boldquot.header en@quot.header insert-header.sin Makefile.in.in quot.sed remove-potcdate.sin Rules-quot $(PACKAGE)_plugin_entities.pot

