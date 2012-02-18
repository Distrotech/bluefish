
EXTRA_DIST = \
	$(bflang_DATA) \
	$(bflangsample_DATA) \
	$(bflangrng_DATA) \
	$(bflangxml_DATA)

MAINTAINERCLEANFILES = Makefile.in

bflang_bfinc_files = \
	all-html.bfinc \
	all-html5.bfinc \
	all-javascript.bfinc \
	all-php.bfinc \
	all-vbscript.bfinc \
	css-rules.bfinc \
	css-selectors.bfinc \
	JQuery.bfinc \
	wordpress.bfinc

bflang_bflang2_files = \
	ada.bflang2 \
	asp.bflang2 \
	asp-vbscript.bflang2 \
	bflang2.bflang2 \
	c.bflang2 \
	cfml.bflang2 \
	clojure.bflang2 \
	cpp.bflang2 \
	css.bflang2 \
	d.bflang2 \
	diff.bflang2 \
	highlighter.bflang2 \
	html.bflang2 \
	html5.bflang2 \
	java.bflang2 \
	javascript.bflang2 \
	jsp.bflang2 \
	lua.bflang2 \
	mediawiki.bflang2 \
	nsis.bflang2 \
	octave.bflang2 \
	pascal.bflang2 \
	perl.bflang2 \
	php.bflang2 \
	po.bflang2 \
	processing.bflang2 \
	python.bflang2 \
	r.bflang2 \
	ruby.bflang2 \
	scheme.bflang2 \
	shell.bflang2 \
	smarty.bflang2 \
	sql.bflang2 \
	svg.bflang2 \
	text.bflang2 \
	vala.bflang2 \
	vbscript.bflang2 \
	wordpress.bflang2 \
	xhtml.bflang2 \
	xml.bflang2

bflangdir = $(pkgdatadir)/bflang
bflang_DATA = \
	$(bflang_bfinc_files) \
	$(bflang_bflang2_files)

bflangsampledir = $(docdir)/bflang
bflangsample_DATA = sample.bflang2

bflangxmldir = $(datadir)/xml/$(PACKAGE)
bflangxml_DATA = catalog.xml

bflangrngdir = $(datadir)/xml/$(PACKAGE)/2.0
bflangrng_DATA = bflang2.rng

# If you are a packager and rely on $(DESTDIR), then the package catalog
# will not get registered in the systems catalog. Use your packaging files
# and scripts to register this entry.
install-data-hook:
if REGISTER_XML_CATALOG
	if test -z "$(DESTDIR)" -a -f $(XML_CATALOG_FILE) ; then \
		$(XMLCATALOG) --noout --add 'delegateURI' \
		    'http://bluefish.openoffice.nl/ns/bflang/2.0/' '$(datadir)/xml/$(PACKAGE)' \
		    $(XML_CATALOG_FILE) || true ; \
	fi
endif #REGISTER_XML_CATALOG

uninstall-hook:
if REGISTER_XML_CATALOG
	if test -z "$(DESTDIR)" ; then \
		$(XMLCATALOG) --noout --del 'http://bluefish.openoffice.nl/ns/bflang/2.0/' \
		    $(XML_CATALOG_FILE) || true ; \
	fi
endif #REGISTER_XML_CATALOG

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
