
SUBDIRS = po

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = zencoding.def

DEFS += -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\"

if ENABLE_ZENCODING_PLUGIN
zencodingdir = $(pkglibdir)
zencoding_LTLIBRARIES = zencoding.la

plugindir = $(pkgdatadir)/plugins
nobase_dist_plugin_DATA = \
	zencoding/__init__.py \
	zencoding/html_matcher.py \
	zencoding/resources.py \
	zencoding/utils.py \
	zencoding/zen_settings.py \
	zencoding/actions/__init__.py \
	zencoding/actions/basic.py \
	zencoding/actions/token.py \
	zencoding/actions/traverse.py \
	zencoding/filters/__init__.py \
	zencoding/filters/comment.py \
	zencoding/filters/css.py \
	zencoding/filters/escape.py \
	zencoding/filters/format-css.py \
	zencoding/filters/format.py \
	zencoding/filters/haml.py \
	zencoding/filters/html.py \
	zencoding/filters/single-line.py \
	zencoding/filters/trim.py \
	zencoding/filters/xsl.py \
	zencoding/interface/__init__.py \
	zencoding/interface/editor.py \
	zencoding/interface/file.py \
	zencoding/parser/__init__.py \
	zencoding/parser/abbreviation.py \
	zencoding/parser/css.py \
	zencoding/parser/utils.py \
	zencoding/parser/xml.py


	zencoding/*.* \
	zencoding/actions/*.* \
	zencoding/filters/*.* \
	zencoding/interface/*.* \
	zencoding/parser/*.*
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
        
