
EXTRA_DIST = \
	$(fdomenu_in_files) \
	$(fdomime_in_files) \
	$(oldgmime_DATA)

CLEANFILES = \
	$(fdomenu_DATA) \
	$(fdomime_DATA)

DISTCLEANFILES = \
	bluefish.desktop.in \
	bluefish-project.desktop.in \
	bluefish.keys

MAINTAINERCLEANFILES = Makefile.in

fdomenu_in_files = bluefish.desktop.in bluefish-project.desktop.in
fdomime_in_files = bluefish.xml.in


if INSTALL_FDOMENU
fdomenudir = $(fdomenupath)
fdomenu_DATA = $(fdomenu_in_files:.desktop.in=.desktop)
endif

if INSTALL_FDOMIME
fdomimedir = $(fdomimepath)/packages
fdomime_DATA = $(fdomime_in_files:.xml.in=.xml)
endif

if INSTALL_GMIME
oldgmimedir = $(oldgmimepath)
oldgmime_DATA = bluefish.keys bluefish.mime
endif


@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_XML_RULE@
@INTLTOOL_KEYS_RULE@


install-data-hook:
if INSTALL_FDOMENU
if HAVE_UPDATE_DESKTOP_DATABASE
	$(UPDATE_DESKTOP_DATABASE) $(DESTDIR)$(fdomenudir)
endif
endif
if INSTALL_FDOMIME
if HAVE_UPDATE_MIME_DATABASE
	$(UPDATE_MIME_DATABASE) $(DESTDIR)$(fdomimepath)
endif
endif


uninstall-hook:
if INSTALL_FDOMENU
if HAVE_UPDATE_DESKTOP_DATABASE
	$(UPDATE_DESKTOP_DATABASE) $(DESTDIR)$(fdomenudir)
endif
endif
if INSTALL_FDOMIME
if HAVE_UPDATE_MIME_DATABASE
	$(UPDATE_MIME_DATABASE) $(DESTDIR)$(fdomimepath)
endif
endif

