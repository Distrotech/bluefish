
EXTRA_DIST = \
	$(fdomenu_in_files) \
	$(fdomime_in_files)

CLEANFILES = \
	$(fdomenu_DATA) \
	$(fdomime_DATA)

MAINTAINERCLEANFILES = Makefile.in


fdomenu_in_files  = bluefish-unstable.desktop.in bluefish-unstable-project.desktop.in
fdomime_in_files  = bluefish-unstable.xml.in


if INSTALL_FDOMENU
fdomenudir = $(fdomenupath)
fdomenu_DATA = $(fdomenu_in_files:.desktop.in=.desktop)
endif

if INSTALL_FDOMIME
fdomimedir = $(fdomimepath)/packages
fdomime_DATA = $(fdomime_in_files:.xml.in=.xml)
endif


@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_XML_RULE@


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

