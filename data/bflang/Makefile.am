
EXTRA_DIST = \
	$(bflang_DATA)

#CLEANFILES = catalog.xml

MAINTAINERCLEANFILES = Makefile.in

bflang_bfinc_files = \
	all-html.bfinc \
	all-javascript.bfinc \
	all-php.bfinc \
	all-vbscript.bfinc \
	css-rules.bfinc \
	css-selectors.bfinc

bflang_bflang2_files = \
	ada.bflang2 \
	asp.bflang2 \
	asp-vbscript.bflang2 \
	bflang2.bflang2 \
	c.bflang2 \
	cpp.bflang2 \
	css.bflang2 \
	d.bflang2 \
	html.bflang2 \
	java.bflang2 \
	javascript.bflang2 \
	jsp.bflang2 \
	mediawiki.bflang2 \
	nsis.bflang2 \
	perl.bflang2 \
	php.bflang2 \
	po.bflang2 \
	python.bflang2 \
	ruby.bflang2 \
	shell.bflang2 \
	smarty.bflang2 \
	text.bflang2 \
	vbscript.bflang2 \
	xhtml.bflang2 \
	xml.bflang2

#bflangdtddir = $(datadir)/xml/$(PACKAGE)
#bflangdtd_DATA = bflang.rng catalog.xml

bflangdir = $(pkgdatadir)/bflang
bflang_DATA = \
	$(bflang_bfinc_files) \
	$(bflang_bflang2_files) \
	sample.bflang2

#catalog.xml:
#	$(XMLCATALOG) --noout --create \
#	  --add 'public' 'Bluefish/DTD/Bflang' 'bflang.dtd' \
#	  --add 'system' 'http://bluefish.openoffice.nl/DTD/bflang.dtd' 'bflang.dtd' \
#	  --add 'rewriteURI' 'http://bluefish.openoffice.nl/DTD' '$(datadir)/xml/$(PACKAGE)' \
#	  $@
#
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

#check-local: check-rng check-bflang2

check-bflang2: $(bflang_bflang2_files)
if HAVE_JING
	$(JING) $(JING_FLAGS) $(srcdir)/bflang2.rng $^
else
if HAVE_XMLLINT
	$(XMLLINT) $(XMLLINT_FLAGS) --noout --noent \
	    --relaxng $(srcdir)/bflang2.rng $^
else
	@echo "Warning: RELAX NG schema not checked."
endif
endif

check-rng: bflang2.rng
if HAVE_JING
	$(JING) $(JING_FLAGS) http://relaxng.org/relaxng.rng $<
else
if HAVE_XMLLINT
	$(XMLLINT) $(XMLLINT_FLAGS) --noout --noent \
	    --relaxng http://relaxng.org/relaxng.rng $<
else
	@echo "Warning: Bflang2 files not checked against RELAX NG schema."
endif
endif
