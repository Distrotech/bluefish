
EXTRA_DIST = \
	bflib_css2.xml \
	bflib_dom.xml \
	bflib_php4.xml \
	bflib_php5_with_examples.xml \
	bflib_php5.xml \
	bflib_python_2.3.xml \
	bflib_xhtml.xml \
	bflib_XHTML_1_0_Strict.xml \
	bflib_XHTML_1_0_Transitional.xml \
	bflib_XHTML_1_0_Frameset.xml \
	bflib_XHTML_1_1.xml \
	bflib_SVG_1_0.xml \
	bflib_SVG_1_1_Full.xml \
	bflib_SVG_1_1_Basic.xml \
	bflib_SVG_1_1_Tiny.xml \
	bflib_XHTML_1_0_W3C.xml \
	bflib_DocBook_4_5.xml \
	bflib_DocBook_5_0.xml

CLEANFILES = $(bflib_DATA)

MAINTAINERCLEANFILES = Makefile.in

bflibdir = $(pkgdatadir)/bflib
bflib_DATA = $(EXTRA_DIST:.xml=.xml.gz)

SUFFIXES = .xml.gz .xml

.xml.xml.gz:
	gzip $< -c > $@
