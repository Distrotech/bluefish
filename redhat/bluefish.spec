%define	desktop_vendor 	endur
%define name  		bluefish
%define version		1.1
%define release 	020050417
%define source		bluefish-CVS-2005-04-17


Summary:	  A GTK2 web development application for experienced users.
Name:		  %{name}
Version:	  %{version}
Release:	  %{release}
Source:		  ftp://ftp.ratisbona.com/pub/bluefish/snapshots/%{source}.tar.bz2
URL:		  http://bluefish.openoffice.nl
License:	  GPL
Group:            Development/Tools
Requires:	  gtk2 >= 2.0.6, pcre >= 3.9, aspell >= 0.50, gnome-vfs2 => 2.4.1
BuildRequires:    gtk2-devel >= 2.0.6, pcre-devel >= 3.9, gnome-vfs2-devel >= 2.4.1
BuildRequires:    aspell-devel >= 0.50, desktop-file-utils, gettext
Requires(post):   desktop-file-utils, shared-mime-info
Requires(postun): desktop-file-utils, shared-mime-info
BuildRoot: %{_tmppath}/%{name}-CVS-%{release}-root

%description
Bluefish is a powerful editor for experienced web designers and programmers.
Bluefish supports many programming and markup languages, but it focuses on
editing dynamic and interactive websites.

%prep
%setup -q -n %{source}

%build
%configure --disable-update-databases
%{__make} %{?_smp_mflags}

%install
%{__rm} -rf %{buildroot}
make install DESTDIR=%{buildroot}

%find_lang %{name}

desktop-file-install --vendor %{desktop_vendor} --delete-original \
  --dir %{buildroot}%{_datadir}/applications                      \
  --add-category X-Fedora                                         \
  --add-category Application                                      \
  --add-category Development                                      \
  %{buildroot}%{_datadir}/applications/%{name}.desktop

%clean
%{__rm} -rf %{buildroot}

%post
update-mime-database %{_datadir}/mime > /dev/null 2>&1 || :
update-desktop-database %{_datadir}/applications > /dev/null 2>&1 || :

%postun
update-mime-database %{_datadir}/mime > /dev/null 2>&1 || :
update-desktop-database {_datadir}/applications > /dev/null 2>&1 || :

%files -f %{name}.lang
%defattr(-, root, root)
%doc AUTHORS COPYING INSTALL README TODO
%{_bindir}/bluefish
%dir %{_datadir}/bluefish
%{_datadir}/bluefish/*
%{_datadir}/applications/*
%{_datadir}/application-registry/*
%{_datadir}/mime/packages/*
%{_datadir}/mime-info/*
%{_datadir}/icons/hicolor/48x48/apps/*
%{_datadir}/icons/hicolor/48x48/mimetypes/*
%{_datadir}/pixmaps/*


%changelog
* Mon Apr 18 2005 Matthias Haase <matthias_haase@bennewitz.com>
- Automatic build - snapshot of 2005-04-17
