
AUTOMAKE_OPTIONS = dist-bzip2 1.8

SUBDIRS = data desktop-data images man po src ui win32 macosx

PO_SUBDIRS = \
	po \
	src/plugin_about/po \
	src/plugin_charmap/po \
	src/plugin_entities/po \
	src/plugin_htmlbar/po \
	src/plugin_infbrowser/po \
	src/plugin_snippets/po \
	src/plugin_zencoding/po
	
#	src/plugin_xmltools/po


EXTRA_DIST = \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	Makefile.mingw

DISTCLEANFILES = \
	doltcompile \
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

DISTCHECK_CONFIGURE_FLAGS = --disable-update-databases --disable-xml-catalog-update \
                            --enable-debugging-output --enable-development

#maintainer-clean-local:
#	find $(top_srcdir) -name "*~" -exec rm "{}" ";"

update-gmo:
	@list='$(PO_SUBDIRS)'; \
	for subdir in $$list; do \
		(cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) update-gmo) || true; \
	done

update-po:
	@list='$(PO_SUBDIRS)'; \
		for subdir in $$list; do \
		(cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) update-po) || true; \
	done
