
##SUBDIRS = po

EXTRA_DIST = rsync.def

MAINTAINERCLEANFILES = Makefile.in

DEFS += -DLOCALEDIR=\"$(localedir)\"

rsyncdir = $(pkglibdir)
rsync_noinst_LTLIBRARIES = rsync.la

rsync_la_SOURCES = \
	rsync.c \
	rsync.h \
	rsyncgui.c \
	rsyncgui.h

rsync_la_LDFLAGS = -module -avoid-version $(WIN32_EXTRA_LDFLAGS)

if OS_WIN32
rsync_la_LDFLAGS += -export-symbols rsync.def
endif

