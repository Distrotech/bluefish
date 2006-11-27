
EXTRA_DIST = snippets.def

MAINTAINERCLEANFILES = Makefile.in

DEFS += -DLOCALEDIR=\"$(localedir)\"

snippetsdir = $(pkglibdir)
snippets_LTLIBRARIES = snippets.la

snippets_la_SOURCES = \
	snippets.c \
	snippets.h \
	snippets_gui.c \
	snippets_gui.h \
	snippets_leaf_insert.c \
	snippets_leaf_insert.h \
	snippets_load.c \
	snippets_load.h

snippets_la_LDFLAGS = -module -avoid-version

if OS_WIN32
snippets_la_LDFLAGS += -no-undefined -export-symbols $(srcdir)/snippets.def
endif

