
SUBDIRS = po

EXTRA_DIST = snippets.def

MAINTAINERCLEANFILES = Makefile.in

DEFS += -DLOCALEDIR=\"$(localedir)\"

snippetsdir = $(pkglibdir)
snippets_LTLIBRARIES = snippets.la

snippets_la_SOURCES = \
	snippets.c \
	snippets.h \
	snippets_convert_cmenu.c \
	snippets_convert_cmenu.h \
	snippets_gui.c \
	snippets_gui.h \
	snippets_leaf_insert.c \
	snippets_leaf_insert.h \
	snippets_leaf_snr.c \
	snippets_leaf_snr.h \
	snippets_load.c \
	snippets_load.h \
	snippets_wizard.c \
	snippets_wizard.h

snippets_la_LDFLAGS = -module -avoid-version $(WIN32_EXTRA_LDFLAGS)

if OS_WIN32
snippets_la_LDFLAGS += -export-symbols $(srcdir)/snippets.def
endif

maintainer-clean-local:
	cd $(srcdir)/po && rm -f boldquot.sed en@boldquot.header en@quot.header insert-header.sin Makefile.in.in quot.sed remove-potcdate.sin Rules-quot bluefish-unstable_plugin_snippets.pot
        
