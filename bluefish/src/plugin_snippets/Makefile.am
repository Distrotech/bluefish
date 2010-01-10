
SUBDIRS = po

EXTRA_DIST = snippets.def

MAINTAINERCLEANFILES = Makefile.in

DEFS += -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\"

snippetsdir = $(pkglibdir)
snippets_LTLIBRARIES = snippets.la

snippets_la_SOURCES = \
	snippets.c \
	snippets.h \
	snippets_gui.c \
	snippets_gui.h \
	snippets_leaf_insert.c \
	snippets_leaf_insert.h \
	snippets_leaf_snr.c \
	snippets_leaf_snr.h \
	snippets_load.c \
	snippets_load.h \
	snippets_wizard.c \
	snippets_wizard.h \
	snippetsmenu.c \
	snippetsmenu.h

snippets_la_LDFLAGS = -module -avoid-version

if OS_WIN32
snippets_la_LDFLAGS += -no-undefined -export-symbols $(srcdir)/snippets.def
endif

maintainer-clean-local:
	cd $(srcdir)/po && rm -f boldquot.sed en@boldquot.header en@quot.header insert-header.sin Makefile.in.in quot.sed remove-potcdate.sin Rules-quot $(PACKAGE)_plugin_snippets.pot
        
