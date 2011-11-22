
EXTRA_DIST = zencoding.def

MAINTAINERCLEANFILES = Makefile.in

DEFS += -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\"

if ENABLE_ZENCODING_PLUGIN
zencodingdir = $(pkglibdir)
zencoding_LTLIBRARIES = zencoding.la
endif #ENABLE_ZENCODING_PLUGIN

zencoding_la_SOURCES = \
	zencoding.c \
	zencoding.h \
	zeneditor-interface.c \
	zeneditor-interface.h

zencoding_la_LDFLAGS = -module -avoid-version

if OS_WIN32
zencoding_la_LDFLAGS += -no-undefined -export-symbols $(srcdir)/zencoding.def
endif

maintainer-clean-local:
	cd $(srcdir)/po && rm -f boldquot.sed en@boldquot.header en@quot.header insert-header.sin Makefile.in.in quot.sed remove-potcdate.sin Rules-quot $(PACKAGE)_plugin_snippets.pot
        
