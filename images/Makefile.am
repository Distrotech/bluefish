
EXTRA_DIST = \
	$(APPS_PICS) \
	$(APPS_SVG) \
	$(MIME_PICS) \
	$(MIME_SVG) \
	$(SPLASH_PICS)

APPS_SVG = bluefish-icon.svg

APPS_PICS = \
	bluefish-icon_16.png \
	bluefish-icon_22.png \
	bluefish-icon_32.png \
	bluefish-icon_36.png \
	bluefish-icon_48.png \
	bluefish-icon_64.png \
	bluefish-icon_72.png \
	bluefish-icon_96.png \
	bluefish-icon_128.png \
	bluefish-icon_192.png

MIME_SVG = bluefish-project.svg

MIME_PICS = \
	bluefish-project_16.png \
	bluefish-project_22.png \
	bluefish-project_32.png \
	bluefish-project_36.png \
	bluefish-project_48.png \
	bluefish-project_64.png \
	bluefish-project_72.png \
	bluefish-project_96.png \
	bluefish-project_128.png \
	bluefish-project_192.png

SPLASH_PICS = \
	bluefish-splash-nb2.svg \
	bluefish-splash-nb2.png \
	bluefish-splash-nb3.svg \
	bluefish-splash-nb3.png

MAINTAINERCLEANFILES = Makefile.in

ICON_SIZES = 16 22 32 36 48 64 72 96 128 192

if INSTALL_THEMEICONS
appsiconsvgdir = $(iconthemepath)/scalable/apps
appsiconsvg_DATA = $(APPS_SVG)

mimeiconsvgdir = $(iconthemepath)/scalable/mimetypes
mimeiconsvg_DATA = $(MIME_SVG)
endif #INSTALL_THEMEICONS


install-data-local:
	@$(NORMAL_INSTALL)
	test -z "$(pkgdatadir)" || \
		$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)
	$(INSTALL_DATA) $(srcdir)/bluefish-splash-nb3.png \
		$(DESTDIR)$(pkgdatadir)/bluefish_splash.png
if INSTALL_ICONS
	test -z "$(iconpath)" || \
		$(mkinstalldirs) $(DESTDIR)$(iconpath)
	$(INSTALL_DATA) $(srcdir)/bluefish-icon_48.png \
		$(DESTDIR)$(iconpath)/$(PACKAGE).png
	$(INSTALL_DATA) $(srcdir)/bluefish-project_48.png \
		$(DESTDIR)$(iconpath)/application-x-bluefish-project.png
endif #INSTALL_ICONS
if INSTALL_THEMEICONS
	test -z "$(iconthemepath)" || \
		$(mkinstalldirs) $(DESTDIR)$(iconthemepath)
	for size in $(ICON_SIZES) ; do \
		test -z "$(iconthemepath)/$${size}x$${size}/apps" || \
			$(mkinstalldirs) $(DESTDIR)$(iconthemepath)/$${size}x$${size}/apps ; \
		$(INSTALL_DATA) $(srcdir)/bluefish-icon_$${size}.png \
			$(DESTDIR)$(iconthemepath)/$${size}x$${size}/apps/$(PACKAGE).png ; \
		test -z "$(iconthemepath)" || \
			$(mkinstalldirs) $(DESTDIR)$(iconthemepath)/$${size}x$${size}/mimetypes ; \
		$(INSTALL_DATA) $(srcdir)/bluefish-project_$${size}.png \
			$(DESTDIR)$(iconthemepath)/$${size}x$${size}/mimetypes/application-x-bluefish-project.png ; \
	done
endif #INSTALL_THEMEICONS


uninstall-hook:
	rm -f $(DESTDIR)$(pkgdatadir)/bluefish_splash.png
if INSTALL_ICONS
	rm -f $(DESTDIR)$(iconpath)/$(PACKAGE).png
	rm -f $(DESTDIR)$(iconpath)/application-x-bluefish-project.png
endif #INSTALL_ICONS
if INSTALL_THEMEICONS
	for size in $(ICON_SIZES) ; do \
		rm -f $(DESTDIR)$(iconthemepath)/$${size}x$${size}/apps/$(PACKAGE).png ; \
		rm -f $(DESTDIR)$(iconthemepath)/$${size}x$${size}/mimetypes/application-x-bluefish-project.png ; \
	done
endif #INSTALL_THEMEICONS

