
SUBDIRS = bflang bflib templates

EXTRA_DIST = $(nobase_bfdata_DATA) $(bfdata_SCRIPTS)

MAINTAINERCLEANFILES = Makefile.in

bfdatadir = $(pkgdatadir)
nobase_bfdata_DATA = \
	default_accelmap \
	encodings \
	snippets \
	lipsum/__init__.py \
	lipsum/data/dictionary.txt \
	lipsum/data/sample.txt \
	lipsum/data/lipsum.xml \
	jsbeautifier/__init__.py

bfdata_SCRIPTS = \
	lorem-ipsum-generator \
	jsbeautify
	