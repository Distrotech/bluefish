
SUBDIRS = po

EXTRA_DIST = about.def

CLEANFILES = about_rev.h

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
about_la_LDFLAGS += -no-undefined -export-symbols $(srcdir)/about.def
endif

about_rev.h: about_rev.h.tmp
	@if test ! -e $@ || test -n "`diff $< $@`" ; then cp $< $@ ; fi
       
about_rev.h.tmp:
	@SVN_REVISION=`LANG=C svnversion -n ${top_srcdir} || echo exported` ; \
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

.INTERMEDIATE: about_rev.h.tmp
.PHONY: about_rev.h.tmp

maintainer-clean-local:
	cd $(srcdir)/po && rm -f boldquot.sed en@boldquot.header en@quot.header insert-header.sin Makefile.in.in quot.sed remove-potcdate.sin Rules-quot $(PACKAGE)_plugin_about.pot

