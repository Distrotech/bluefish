
SUBDIRS = po

EXTRA_DIST = entities.def

DISTCLEANFILES = @OPENBSD_3_8_FIX@

MAINTAINERCLEANFILES = Makefile.in

DEFS += -DLOCALEDIR=\"$(localedir)\"

entitiesdir = $(pkglibdir)
entities_LTLIBRARIES = entities.la

entities_la_SOURCES = \
	entities.c \
	entities.h

entities_la_LDFLAGS = -module -avoid-version $(WIN32_EXTRA_LDFLAGS)

if OS_WIN32
entities_la_LDFLAGS += -export-symbols $(srcdir)/entities.def
endif

maintainer-clean-local:
	cd $(srcdir)/po && rm -f boldquot.sed en@boldquot.header en@quot.header insert-header.sin Makefile.in.in quot.sed remove-potcdate.sin Rules-quot bluefish-unstable_plugin_entities.pot

