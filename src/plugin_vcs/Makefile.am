
#SUBDIRS = po

EXTRA_DIST = vcs.def

MAINTAINERCLEANFILES = Makefile.in

DEFS += -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\"

vcsdir = $(pkglibdir)
vcs_LTLIBRARIES = vcs.la

vcs_la_SOURCES = \
	vcs.c \
	vcs.h \
	vcs_gui.c \
	vcs_gui.h \
	svn.c \
	svn.h

vcs_la_LDFLAGS = -module -avoid-version

if OS_WIN32
vcs_la_LDFLAGS += -no-undefined -export-symbols $(srcdir)/vcs.def
endif

maintainer-clean-local:
	cd $(srcdir)/po && rm -f boldquot.sed en@boldquot.header en@quot.header insert-header.sin Makefile.in.in quot.sed remove-potcdate.sin Rules-quot $(PACKAGE)_plugin_vcs.pot
        
