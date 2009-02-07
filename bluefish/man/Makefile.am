
EXTRA_DIST = $(man_MANS)

MAINTAINERCLEANFILES = Makefile.in

man_MANS = $(PACKAGE).1

check-local:
	@echo  -e "\033[1mCorrect any errors before making a distribution.\033[0m"
if HAVE_MAN
	-LANG=C MANWIDTH=80 $(MAN) --warning -l $(srcdir)/$(PACKAGE).1 > /dev/null
else
	@echo -e "\033[1mCannot check the manpages because 'man' is missing.\033[0m"
endif

