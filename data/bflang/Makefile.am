
EXTRA_DIST = \
	bflang.dtd \
	$(bflang_DATA)

CLEANFILES = catalog.xml

MAINTAINERCLEANFILES = Makefile.in

bflangdtddir = $(datadir)/xml/$(PACKAGE)
bflangdtd_DATA = bflang.dtd catalog.xml

bflang_DATA = \
	c.bflang \
	cpp.bflang \
	css.bflang \
	d.bflang \
	html.bflang \
	java.bflang \
	pascal.bflang \
	perl.bflang \
	php.bflang \
	plain.bflang \
	po.bflang \
	python.bflang \
	sql.bflang \
	xml.bflang
bflangdir = $(pkgdatadir)/bflang

catalog.xml:
	$(XMLCATALOG) --noout --create \
	  --add 'public' 'Bluefish/DTD/Bflang' 'bflang.dtd' \
	  --add 'system' 'http://bluefish.openoffice.nl/DTD/bflang.dtd' 'bflang.dtd' \
	  --add 'rewriteURI' 'http://bluefish.openoffice.nl/DTD' '$(datadir)/xml/$(PACKAGE)' \
	  $@

install-data-hook:
if REGISTER_XML_CATALOG
	$(XMLCATALOG) --noout \
	  --add 'public' 'Bluefish/DTD/Bflang' 'bflang.dtd' \
	  --add 'system' 'http://bluefish.openoffice.nl/DTD/bflang.dtd' 'bflang.dtd' \
	  --add 'rewriteURI' 'http://bluefish.openoffice.nl/DTD' '$(datadir)/xml/$(PACKAGE)' \
	  $(DESTDIR)$(XML_CATALOG_FILE)
endif #REGISTER_XML_CATALOG

uninstall-hook:
if REGISTER_XML_CATALOG
	$(XMLCATALOG) --noout --del 'bflang.dtd'
	$(XMLCATALOG) --noout --del 'http://bluefish.openoffice.nl/DTD'
endif #REGISTER_XML_CATALOG

