%define name  		bluefish-unstable
%define version		1.1.2
%define release 	2
%define source		bluefish-unstable-1.1.2


Summary:	  A GTK2 web development application for experienced users
Name:		  %{name}
Version:	  %{version}
Release:	  %{release}%{?dist}
Source:		  ftp://ftp.ratisbona.com/pub/bluefish/snapshots/%{source}.tar.bz2
URL:		  http://bluefish.openoffice.nl
License:	  GPL
Group:            Development/Tools
Requires:	  gtk2, pcre, aspell, gnome-vfs2
BuildRequires:    gtk2-devel, pcre-devel, gnome-vfs2-devel
BuildRequires:    aspell-devel, desktop-file-utils, gettext, libxml2
Requires(post):   desktop-file-utils, shared-mime-info
Requires(postun): desktop-file-utils, shared-mime-info
BuildRoot: %{_tmppath}/%{name}-%{release}-root

%description
Bluefish is a powerful editor for experienced web designers and programmers.
Bluefish supports many programming and markup languages, but it focuses on
editing dynamic and interactive websites

%prep
%setup -q -n %{source}

%build
%configure --disable-update-databases \
  --disable-xml-catalog-update        \
  --without-gnome2_4-mime             \
  --without-gnome2_4-appreg           

%{__make} %{?_smp_mflags}

%install
%{__rm} -rf %{buildroot}
make install DESTDIR=%{buildroot}
%{__rm} -f %{buildroot}%{_libdir}/%{name}/*.*a

# required ugly joining
%find_lang %{name}
%find_lang %{name}_plugin_about
%find_lang %{name}_plugin_entities
%find_lang %{name}_plugin_htmlbar
%find_lang %{name}_plugin_snippets
%{__cat} %{name}_plugin_about.lang >> %{name}.lang
%{__cat} %{name}_plugin_entities.lang >> %{name}.lang
%{__cat} %{name}_plugin_htmlbar.lang >> %{name}.lang
%{__cat} %{name}_plugin_snippets.lang >> %{name}.lang

# required ugly renaming
%{__mv} %{buildroot}%{_datadir}/applications/bluefish.desktop \
  %{buildroot}%{_datadir}/applications/%{name}.desktop
%{__mv} %{buildroot}%{_datadir}/applications/bluefish-project.desktop \
  %{buildroot}%{_datadir}/applications/%{name}-project.desktop

desktop-file-install --vendor=fedora --delete-original \
  --dir %{buildroot}%{_datadir}/applications           \
  --add-category X-Fedora                              \
  --add-category Application                           \
  --add-category Development                           \
  %{buildroot}%{_datadir}/applications/%{name}.desktop
desktop-file-install --vendor=fedora --delete-original \
  --dir %{buildroot}%{_datadir}/applications           \
  --add-category X-Fedora                              \
  %{buildroot}%{_datadir}/applications/%{name}-project.desktop

%clean
%{__rm} -rf %{buildroot}

%post
update-mime-database %{_datadir}/mime > /dev/null 2>&1 || :
update-desktop-database %{_datadir}/applications > /dev/null 2>&1 || :
xmlcatalog --noout --add 'public' 'Bluefish/DTD/Bflang' 'bflang.dtd'     \
  /etc/xml/catalog
xmlcatalog --noout --add 'system'                                        \
  'http://bluefish.openoffice.nl/DTD/bflang.dtd' 'bflang.dtd'            \
  /etc/xml/catalog
xmlcatalog --noout --add 'rewriteURI'                                    \
  'http://bluefish.openoffice.nl/DTD' '/usr/share/xml/bluefish-unstable' \
  /etc/xml/catalog

%postun
update-mime-database %{_datadir}/mime > /dev/null 2>&1 || :
update-desktop-database %{_datadir}/applications > /dev/null 2>&1 || :
xmlcatalog --noout --del 'bflang.dtd' /etc/xml/catalog
xmlcatalog --noout --del 'http://bluefish.openoffice.nl/DTD' /etc/xml/catalog

%files -f %{name}.lang
%defattr(-, root, root)
%doc AUTHORS COPYING README TODO
%{_bindir}/*
%{_datadir}/%{name}
%{_libdir}/%{name}
%{_datadir}/applications/*
%{_datadir}/mime/packages/*
%{_datadir}/pixmaps/*
%{_datadir}/icons/hicolor/*/*/*
%{_datadir}/xml/%{name}
%{_mandir}/man1/*


%changelog
* Thu Jan  4 2007 Matthias Haase <matthias_haase@bennewitz.com> - 1.1.2-2
- libxml2 added to dependencies
- build with '--disable-xml-catalog-update' 
- xmlcatalog data install and remove instead for %post and %postun
- filelist enhanced and cleaned
- desktop files renamed

* Mon Nov  6 2006 Matthias Haase <matthias_haase@bennewitz.com> - 1.0.7-1
- Update to 1.0.7
- mime_icon_assign.patch removed - obsolete 

* Sat Oct 28 2006 Matthias Haase <matthias_haase@bennewitz.com> - 1.0.6-3
- Rebuild for Fedora Core 6

* Mon Oct  2 2006 Matthias Haase <matthias_haase@bennewitz.com> - 1.0.6-2
- Remove of the useless gnome 2.4 mime type registration files
- Patch added for the mime type icon assignment problem

* Tue Sep 26 2006 Matthias Haase <matthias_haase@bennewitz.com> - 1.0.6-1
- Update to 1.0.6 - using latest auto build specfile
- Minor cleanup for %find_lang and %files

* Sat May 06 2006 Matthias Haase <matthias_haase@bennewitz.com>
- Automatic build - snapshot of 2006-05-06
