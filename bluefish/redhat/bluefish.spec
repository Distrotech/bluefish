%define	desktop_vendor 	endur
%define name  		bluefish
%define version		0.13
%define release 	1
%define source		bluefish-0.13.tar.bz2
	

Summary:	A GTK2 web development application for experienced users.
Name:		%{name}
Version:	%{version}
Release:	%{release}
Source:		ftp://ftp.ratisbona.com/pub/bluefish/snapshots/%{source}
URL:		http://bluefish.openoffice.nl
License:	GPL
Group:          Development/Tools
Requires:	gtk2 >= 2.0.6, pcre >= 3.9, aspell >= 0.50, gnome-vfs2 => 2.4.1
BuildRequires:  gtk2-devel >= 2.0.6, pcre-devel >= 3.9, desktop-file-utils, aspell-devel >= 0.50
BuildRequires:  gnome-vfs2-devel >= 2.4.1

BuildRoot: %{_tmppath}/%{name}-%{version}

%description
Bluefish is a GTK+ HTML editor for the experienced web designer or
programmer. It is not finished yet, but already a very powerful site
creating environment. Bluefish has extended support for programming
dynamic and interactive websites, there is for example a lot of PHP
support.

%prep
%setup -q -n %{name}-%{version}

%build
%configure
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_datadir}/applications
mkdir -p %{buildroot}%{_datadir}/pixmaps
mkdir -p %{buildroot}%{_datadir}/application-registry
mkdir -p %{buildroot}%{_datadir}/mime-info

%makeinstall                                                  \
    pkgdatadir=%{buildroot}%{_datadir}/%{name}                \
    gnome2applications=%{buildroot}%{_datadir}/applications/  \
    gnome1menupath=we_do_not_install_this_one                 \
    gnome2icons=%{buildroot}%{_datadir}/pixmaps               \
    gnome2appreg=%{buildroot}%{_datadir}/application-registry \
    gnome2mime-info=%{buildroot}%{_datadir}/mime-info         \
    iconpath=%{buildroot}%{_datadir}/pixmaps                  

%find_lang %{name}

rm -f %{buildroot}%{_datadir}/applications/%{name}.desktop
cat > %{name}.desktop << EOF
[Desktop Entry]
Encoding=UTF-8
StartupNotify=true
Name=Bluefish web editor
Comment=A GTK2 web development application for experienced users.
Exec=%{name} -n
Icon=%{name}-icon.png
Terminal=0
Type=Application
EOF
desktop-file-install --vendor %{desktop_vendor} --delete-original \
  --dir %{buildroot}%{_datadir}/applications                      \
  --add-category X-Red-Hat-Base                                   \
  --add-category Application                                      \
  --add-category Development                                      \
  %{name}.desktop

%clean
rm -rf %{buildroot}

%files -f %{name}.lang
%defattr(-,root, root)
%doc AUTHORS COPYING INSTALL README TODO
%{_bindir}/bluefish
%{_datadir}/bluefish
%{_datadir}/applications/*.desktop
%{_datadir}/application-registry/%{name}.applications
%{_datadir}/mime-info
%{_datadir}/pixmaps/%{name}-icon.png
%{_datadir}/pixmaps/gnome-application-%{name}.png


%changelog
* Tue Apr 13 2004 Matthias Haase <matthias_haase@bennewitz.com>
- Update to version 0.13

* Tue Nov 25 2003 Matthias Haase <matthias_haase@bennewitz.com>
- Update to version 0.12
- Build on and for Fedora Core 1

* Mon Jul 28 2003 Matthias Haase <matthias_haase@bennewitz.com>
- Update to version 0.11

* Tue Jul 17 2003 Matthias Haase <matthias_haase@bennewitz.com>
- Update to version 0.10
- Some cosmetic changes

* Tue Feb 18 2003 Matthias Haase <matthias_haase@bennewitz.com>
- Rebuild for version 0.9
- Added Makefile patch to support DESTDIR.
- Thanks to Matthias Saou <matthias.saou@est.une.marmotte.net>

* Sat Feb 08 2003 Matthias Haase <matthias_haase@bennewitz.com>
- Automatic build - snapshot of 2003-02-07
