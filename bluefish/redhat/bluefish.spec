%define	desktop_vendor 	endur
%define name  		bluefish
%define version		0.11
%define release 	1
%define source		bluefish-0.11.tar.bz2
%define prefix		/usr
	

Summary:	Web development editor for experienced users.
Name:		%{name}
Version:	%{version}
Release:	%{release}
Source:		ftp://ftp.ratisbona.com/pub/bluefish/snapshots/%{source}
URL:		http://bluefish.openoffice.nl
License:	GPL
Group:          Development/Tools
Requires:	gtk2 >= 2.0.6, pcre >= 3.9
BuildRequires:  gtk2-devel >= 2.0.6, pcre-devel >= 3.9, desktop-file-utils

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
mkdir -p %{buildroot}%{_datadir}/pixmaps
mkdir -p %{buildroot}%{_datadir}/applications
make install                                            \
    bindir=%{buildroot}%{_bindir}                       \
    pkgdatadir=%{buildroot}%{_datadir}/%{name}          \
    datadir=%{buildroot}%{_datadir}                     \
    gnome2menupath=%{buildroot}%{_datadir}/applications \
    gnome1menupath=/_none                               \
    iconpath=%{buildroot}%{_datadir}/pixmaps            \
    localedir=%{buildroot}%{_datadir}/locale

%find_lang %{name}
install -D -m644 inline_images/bluefish_icon1.png \
    %{buildroot}%{_datadir}/pixmaps/%{name}-icon.png

rm -f %{buildroot}%{_datadir}/applications/%{name}.desktop

cat << EOF > %{name}.desktop
[Desktop Entry]
Name=Bluefish
Comment=Web development editor
Exec=%{name}
Icon=%{name}-icon.png
Terminal=0
Type=Application
EOF

install -D -m644 %{name}.desktop \
    %{buildroot}%{_datadir}/applications/%{name}.desktop

desktop-file-install --vendor %{desktop_vendor} --delete-original \
  --dir %{buildroot}%{_datadir}/applications                      \
  --add-category X-Red-Hat-Base                                   \
  --add-category Application                                      \
  --add-category Development                                      \
  %{buildroot}%{_datadir}/applications/%{name}.desktop

%clean
rm -rf %{buildroot}

%files -f %{name}.lang
%defattr(-,root, root)
%doc COPYING INSTALL
%{_bindir}/*
%dir %{_datadir}/%{name}
%{_datadir}/bluefish/*
%{_datadir}/applications/%{desktop_vendor}-%{name}.desktop
%{_datadir}/pixmaps/%{name}-icon.png


%changelog
* Mon Jul 28 2003 Matthias Haase <matthias_haase@bennewitz.com>
- Update to version 0.11

* Tue Jul 17 2003 Matthias Haase <matthias_haase@bennewitz.com>
- Update to version 0.10
- Some cosmetic changes

* Tue Feb 18 2003 Matthias Haase <matthias_haase@bennewitz.com>
- Rebuild for version 0.9
- Added Makefile patch to support DESTDIR.
- Thanks to Matthias Saou <matthias.saou@est.une.marmotte.net> for the patch

* Sat Feb 08 2003 Matthias Haase <matthias_haase@bennewitz.com>
- Automatic build - snapshot of 2003-02-07
