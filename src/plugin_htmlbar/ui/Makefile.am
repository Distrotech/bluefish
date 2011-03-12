
uidir = $(datadir)/bluefish/plugins/htmlbar/ui

ui_DATA = \
    htmlbar_menu_ui.xml \
    htmlbar_toolbar_ui.xml

EXTRA_DIST = \
    $(ui_DATA)

MAINTAINERCLEANFILES = Makefile.in

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
