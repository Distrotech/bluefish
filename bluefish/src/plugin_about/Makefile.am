
SUBDIRS = po

CLEANFILES = about_rev.h

DISTCLEANFILES = @OPENBSD_3_8_FIX@

MAINTAINERCLEANFILES = Makefile.in

BUILT_SOURCES = about_rev.h

DEFS += -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\"

aboutdir = $(pkglibdir)
about_LTLIBRARIES = about.la

about_la_SOURCES = \
	about.c \
	about.h \
	about_rev.h

about_la_LDFLAGS = -module -avoid-version

if OS_WIN32
about_la_LDFLAGS += -no-undefined
endif

about_rev.h: FORCE
	@SVN_REVISION=`LANG=C svnversion -n . || echo exported` ; \
	echo "/* Automatically created by the Makefile." > $@ ; \
	echo " * http://bugzilla.gnome.org/show_bug.cgi?id=578719 */" >> $@ ; \
	echo "#ifndef __ABOUT_REV_H_" >> $@ ; \
	echo "#define __ABOUT_REV_H_" >> $@ ; \
	echo "#ifdef SVN_REVISION" >> $@ ; \
	echo "#undef SVN_REVISION" >> $@ ; \
	echo "#endif /* SVN_REVISION */" >> $@ ; \
	if test -n $${SVN_REVISION} -a "x$${SVN_REVISION}" != "xexported" ; then \
		echo "#define SVN_REVISION \"$${SVN_REVISION}\"" >> $@ ; \
	fi ; \
	echo "#endif /* __ABOUT_REV_H_ */" >> $@

FORCE:

maintainer-clean-local:
	cd $(srcdir)/po && rm -f boldquot.sed en@boldquot.header en@quot.header insert-header.sin Makefile.in.in quot.sed remove-potcdate.sin Rules-quot $(PACKAGE)_plugin_about.pot

