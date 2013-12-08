
EXTRA_DIST = \
	$(fdomenu_in_files) \
	$(fdomime_in_files) \
	$(fdoappdata_in_files)

CLEANFILES = \
	$(fdomenu_DATA) \
	$(fdomime_DATA) \
	$(fdoappdata_DATA)

MAINTAINERCLEANFILES = Makefile.in


fdomenu_in_files  = bluefish.desktop.in
fdomime_in_files  = bluefish.xml.in
fdoappdata_in_files  = bluefish.appdata.xml

if INSTALL_FDOMENU
fdomenudir = $(fdomenupath)
fdomenu_DATA = $(fdomenu_in_files:.desktop.in=.desktop)
endif

if INSTALL_FDOMIME
fdomimedir = $(fdomimepath)/packages
fdomime_DATA = $(fdomime_in_files:.xml.in=.xml)
endif

if INSTALL_FDOAPPDATA
fdoappdatadir = $(fdoappdatapath)
fdoappdata_DATA = $(fdoappdata_in_files:.xml.in=.xml)
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


check-local: bluefish.desktop
	@echo  -e "\033[1mCorrect any errors before making a distribution.\033[0m"
if HAVE_DESKTOP_FILE_VALIDATE
	LANG=C $(DESKTOP_FILE_VALIDATE) $<
else
	@echo -e "\033[1mCannot check the .desktop file because 'desktop-file-validate' is missing.\033[0m"
endif

