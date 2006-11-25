
EXTRA_DIST = \
	$(PACKAGE).1 \
	bluefish.1.xml \
	bf_entities.dtd.in

DISTCLEANFILES = bf_entities.dtd

MAINTAINERCLEANFILES = Makefile.in

VPATH += @builddir@

XMLLINT_FLAGS  += --path "$(VPATH)" --noout --valid
XSLTPROC_FLAGS += --path "$(VPATH)" --param man.charmap.use.subset "0"

man1_MANS = $(PACKAGE).1

$(PACKAGE).1 : bluefish.1.xml
if HAVE_XSLTPROC
	$(XSLTPROC) $(XSLTPROC_FLAGS) $<
else
	@echo  -e "\033[1mInstall 'xsltproc', if you want to update the manpage.\033[0m"
endif

check-local:
	@echo  -e "\033[1mCorrect any errors before making a distribution.\033[0m"
if HAVE_XMLLINT
	$(XMLLINT) $(XMLLINT_FLAGS) bluefish.1.xml
else
	@echo  -e "\033[1mCannot check the XML sources because 'xmllint' is missing.\033[0m"
endif
if HAVE_MAN
	LANG=C MANWIDTH=80 $(MAN) $(srcdir)/$(PACKAGE).1 > /dev/null
else
	@echo -e "\033[1mCannot check the manpages because 'man' is missing.\033[0m"
endif

