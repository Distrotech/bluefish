
EXTRA_DIST = \
	bflib_css2.xml \
	bflib_dom.xml \
	bflib_php4.xml \
	bflib_php5_with_examples.xml \
	bflib_php5.xml \
	bflib_python_2.3.xml \
	bflib_xhtml.xml

CLEANFILES = $(bflib_DATA)

MAINTAINERCLEANFILES = Makefile.in

bflibdir = $(pkgdatadir)/bflib
bflib_DATA = $(EXTRA_DIST:.xml=.xml.gz)

%.xml.gz : %.xml
	gzip $< -c > $@

