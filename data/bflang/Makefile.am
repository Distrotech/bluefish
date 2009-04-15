
EXTRA_DIST = \
	$(bflang_DATA)

#CLEANFILES = catalog.xml

MAINTAINERCLEANFILES = Makefile.in

#bflangdtddir = $(datadir)/xml/$(PACKAGE)
#bflangdtd_DATA = bflang.dtd catalog.xml

bflang_DATA = \
	asp.bflang2 \
	bflang2.bflang2 \
	c.bflang2 \
	cpp.bflang2 \
	css.bflang2 \
	css-selectors.bfinc \
	css-rules.bfinc \
	d.bflang2 \
	html.bflang2 \
	java.bflang2 \
	javascript.bflang2 \
	all-javascript.bfinc \
	jsp.bflang2 \
	perl.bflang2 \
	php.bflang2 \
	po.bflang2 \
	python.bflang2 \
	ruby.bflang2 \
	shell.bflang2 \
	text.bflang2 \
	xhtml.bflang2 \
	xml.bflang2

bflangdir = $(pkgdatadir)/bflang

#catalog.xml:
#	$(XMLCATALOG) --noout --create \
#	  --add 'public' 'Bluefish/DTD/Bflang' 'bflang.dtd' \
#	  --add 'system' 'http://bluefish.openoffice.nl/DTD/bflang.dtd' 'bflang.dtd' \
#	  --add 'rewriteURI' 'http://bluefish.openoffice.nl/DTD' '$(datadir)/xml/$(PACKAGE)' \
#	  $@

#install-data-hook:
#if REGISTER_XML_CATALOG
#	$(XMLCATALOG) --noout \
#	  --add 'public' 'Bluefish/DTD/Bflang' 'bflang.dtd' \
#	  --add 'system' 'http://bluefish.openoffice.nl/DTD/bflang.dtd' 'bflang.dtd' \
#	  --add 'rewriteURI' 'http://bluefish.openoffice.nl/DTD' '$(datadir)/xml/$(PACKAGE)' \
#	  $(DESTDIR)$(XML_CATALOG_FILE)
#endif #REGISTER_XML_CATALOG
#
#uninstall-hook:
#if REGISTER_XML_CATALOG
#	$(XMLCATALOG) --noout --del 'bflang.dtd'
#	$(XMLCATALOG) --noout --del 'http://bluefish.openoffice.nl/DTD'
#endif #REGISTER_XML_CATALOG

