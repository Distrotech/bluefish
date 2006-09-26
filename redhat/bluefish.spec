%define name  		bluefish
%define version		1.0.6
%define release 	1
%define source		bluefish-1.0.6


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
BuildRequires:    aspell-devel, desktop-file-utils, gettext
Requires(post):   desktop-file-utils, shared-mime-info
Requires(postun): desktop-file-utils, shared-mime-info
BuildRoot: %{_tmppath}/%{name}-CVS-%{release}-root

%description
Bluefish is a powerful editor for experienced web designers and programmers.
Bluefish supports many programming and markup languages, but it focuses on
editing dynamic and interactive websites

%prep
%setup -q -n %{source}

%build
%configure --disable-update-databases
%{__make} %{?_smp_mflags}

%install
%{__rm} -rf %{buildroot}
make install DESTDIR=%{buildroot}

%find_lang %{name}

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

%postun
update-mime-database %{_datadir}/mime > /dev/null 2>&1 || :
update-desktop-database %{_datadir}/applications > /dev/null 2>&1 || :

%files -f %{name}.lang
%defattr(-, root, root)
%doc AUTHORS COPYING README TODO
%{_bindir}/*
%dir %{_datadir}/%{name}
%{_datadir}/%{name}/*
%{_datadir}/applications/*
%{_datadir}/application-registry/*
%{_datadir}/mime/packages/*
%{_datadir}/mime-info/*
%{_datadir}/pixmaps/*
%{_mandir}/man1/*


%changelog
* Tue Sep 26 2006 Matthias Haase <matthias_haase@bennewitz.com> - 1.0.6-1
- Update to 1.0.6 - using latest auto build specfile
- Minor cleanup for %find_lang and %files

* Sat May 06 2006 Matthias Haase <matthias_haase@bennewitz.com>
- Automatic build - snapshot of 2006-05-06
