
uidir = $(datadir)/bluefish/ui

ui_DATA = \
    bluefish_menu_ui.xml \
    bluefish_toolbar_ui.xml

EXTRA_DIST = \
    $(ui_DATA)

MAINTAINERCLEANFILES = Makefile.in

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
