
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
	jsbeautifier/__init__.py \
	jsbeautifier/__version__.py \
	jsbeautifier/unpackers/packer.py \
	jsbeautifier/unpackers/javascriptobfuscator.py \
	jsbeautifier/unpackers/myobfuscate.py \
	jsbeautifier/unpackers/__init__.py \
	jsbeautifier/unpackers/evalbased.py \
	jsbeautifier/unpackers/urlencode.py

bfdata_SCRIPTS = \
	lorem-ipsum-generator \
	jsbeautify \
	jsmin.py \
	cssmin.py
