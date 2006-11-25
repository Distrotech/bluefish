
EXTRA_DIST = \
	bluefish-unstable.1 \
	bluefish.1.xml \
	bf_entities.dtd.in

DISTCLEANFILES = bf_entities.dtd

MAINTAINERCLEANFILES = Makefile.in

VPATH += @builddir@

XMLLINT_FLAGS  += --path "$(VPATH)" --noout --valid
XSLTPROC_FLAGS += --path "$(VPATH)" --param man.charmap.use.subset "0"

man1_MANS = bluefish-unstable.1

bluefish-unstable.1 : bluefish.1.xml
if HAVE_XSLTPROC
	$(XSLTPROC) $(XSLTPROC_FLAGS) $<
else
	$(warning "Install 'xsltproc', if you want to update the manpage."
endif

check-local:
	$(info "Correct any errors before making a distribution.")
if HAVE_XMLLINT
	$(XMLLINT) $(XMLLINT_FLAGS) bluefish.1.xml
else
	$(warning "Cannot check the XML sources because 'xmllint' is missing.")
endif
if HAVE_MAN
	LANG=C MANWIDTH=80 $(MAN) $(addprefix $(srcdir)/,bluefish-unstable.1) > /dev/null
else
	$(warning "Cannot check the manpages because 'man' is missing.")
endif

