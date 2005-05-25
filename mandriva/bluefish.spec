%define name  		bluefish
%define version		1.0
%define release 	1.102mcnl
%define section		Internet/Web Editors

Summary:	A GTK2 web development application for experienced users.
Name:		%{name}
Version:	%{version}
Release:	%{release}
Source:		%{name}-%{version}.tar.bz2
URL:		http://bluefish.openoffice.nl
License:	GPL
Group:          Development/Other
Packager: 	Steffen Van Roosbroeck <dansmug@mandrakeclub.nl>
Vendor:		Mandrivaclub.nl
Requires:	gtk2 >= 2.0.6, pcre >= 3.9, aspell >= 0.50, gnome-vfs2 => 2.4.1
Requires:       shared-mime-info >= 0.15
BuildRequires:  gtk2-devel >= 2.0.6, pcre-devel >= 3.9, gnome-vfs2-devel >= 2.4.1
BuildRequires:  aspell-devel >= 0.50, desktop-file-utils, gettext

BuildRoot: %{_tmppath}/%{name}-%{version}
#Patch0:	bluefish_desktop_icon.patch

%description
Bluefish is a GTK+ HTML editor for the experienced web designer or
programmer. It is not finished yet, but already a very powerful site
creating environment. Bluefish has extended support for programming
dynamic and interactive websites, there is for example a lot of PHP
support.

%prep
%setup -q -n %{name}-%{version}
#%patch0 -p0

%build
%configure --disable-update-databases
%{__make} %{?_smp_mflags}

%install
%{__rm} -rf %{buildroot}
make install DESTDIR=%{buildroot}

mkdir -p %{buildroot}%{_iconsdir}
install -m644 inline_images/bluefish_icon1.png %{buildroot}%{_iconsdir}/%{name}.png

mkdir -p $RPM_BUILD_ROOT%{_menudir}
cat >$RPM_BUILD_ROOT%{_menudir}/%{name} <<EOF
?package(%{name}): command="/usr/bin/%{name}" needs="X11" \
icon="%{name}.png" section="%{section}" \
title="%{name}" longtitle="%{summary}"
EOF

%find_lang %{name}
%clean
%{__rm} -rf %{buildroot}

%files -f %{name}.lang
%defattr(-,root, root)
%doc AUTHORS COPYING INSTALL README TODO
%{_bindir}/bluefish
%{_datadir}/bluefish
%{_datadir}/applications/*.desktop
%{_datadir}/application-registry/*
%{_datadir}/mime/*
%{_datadir}/mime-info/*
%{_datadir}/pixmaps/*.png
%{_menudir}/%{name}
%{_iconsdir}/%{name}.png


%changelog
* Thu May 19 2005 Steffen Van Roosbroeck <dansmug@mandrakeclub.nl> 1.0-1.102mcnl
- rebuild for Mandriva Linux LE 2005
- changed the .spec-file

* Wed Jan 12 2005 Matthias Haase <matthias_haase@bennewitz.com>
- Rebuild for version 1.0

* Fri Jan 07 2005 Matthias Haase <matthias_haase@bennewitz.com>
- Automatic build - snapshot of 2005-01-06

* Tue Feb 18 2003 Matthias Haase <matthias_haase@bennewitz.com>
- Rebuild for version 0.9
- Added Makefile patch to support DESTDIR.
- Thanks to Matthias Saou <matthias.saou@est.une.marmotte.net> for the patch
