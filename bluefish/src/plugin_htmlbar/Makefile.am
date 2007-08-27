
SUBDIRS = pixmaps po

EXTRA_DIST = htmlbar.def

DISTCLEANFILES = @OPENBSD_3_8_FIX@

MAINTAINERCLEANFILES = Makefile.in

DEFS += -DLOCALEDIR=\"$(localedir)\"

htmlbardir = $(pkglibdir)
htmlbar_LTLIBRARIES = htmlbar.la

htmlbar_la_SOURCES = \
	cap.c \
	cap.h \
	html2.c \
	html2.h \
	htmlbar.c \
	htmlbar_gui.c \
	htmlbar_gui.h \
	htmlbar.h \
	html.c \
	html_diag.c \
	html_diag.h \
	html_form.c \
	html_form.h \
	html.h \
	html_table.c \
	html_table.h \
	image.c \
	image.h \
	quickstart.c \
	quickstart.h \
	rpopup.c \
	rpopup.h \
	wizards.c \
	wizards.h

htmlbar_la_LDFLAGS = -module -avoid-version

if OS_WIN32
htmlbar_la_LDFLAGS += -no-undefined -export-symbols $(srcdir)/htmlbar.def
endif

maintainer-clean-local:
	cd $(srcdir)/po && rm -f boldquot.sed en@boldquot.header en@quot.header insert-header.sin Makefile.in.in quot.sed remove-potcdate.sin Rules-quot $(PACKAGE)_plugin_htmlbar.pot

