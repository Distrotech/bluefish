
SUBDIRS = po

EXTRA_DIST = infbrowser.def

MAINTAINERCLEANFILES = Makefile.in

DEFS += -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\"

infbrowserdir = $(pkglibdir)
infbrowser_LTLIBRARIES = infbrowser.la

infbrowser_la_SOURCES = \
	infbrowser.c \
	infbrowser.h \
	infb_gui.h \
	infb_gui.c \
	infb_load.h \
	infb_load.c \
	infb_text.h \
	infb_text.c \
	infb_dtd.h \
	infb_dtd.c \
	infb_wizard.h \
	infb_wizard.c \
	infb_docbook.h \
	infb_docbook.c \
	infb_html.h \
	infb_html.c

infbrowser_la_LDFLAGS = -module -avoid-version

if OS_WIN32
infbrowser_la_LDFLAGS += -no-undefined -export-symbols $(srcdir)/infbrowser.def
endif

maintainer-clean-local:
	cd $(srcdir)/po && rm -f boldquot.sed en@boldquot.header en@quot.header insert-header.sin Makefile.in.in quot.sed remove-potcdate.sin Rules-quot $(PACKAGE)_plugin_infbrowser.pot
        
