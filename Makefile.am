
AUTOMAKE_OPTIONS = dist-bzip2 1.7

SUBDIRS = data desktop-data images man po src

EXTRA_DIST = \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	mkinstalldirs

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update

MAINTAINERCLEANFILES = \
	ABOUT-NLS \
	aclocal.m4 \
	compile \
	config.guess \
	config.sub \
	config.h.in \
	configure \
	depcomp \
	install-sh \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	ltmain.sh \
	missing \
	mkinstalldirs \
	Makefile.in

DISTCHECK_CONFIGURE_FLAGS = --disable-update-databases --disable-xml-catalog-update

#maintainer-clean-local:
#	find $(top_srcdir) -name "*~" -exec rm "{}" ";"

