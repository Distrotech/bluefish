%define	desktop_vendor 	endur
%define name  		bluefish
%define snap		0.0.0.20021117
%define release 	snap.1
%define source		bluefish-gtk2port-2002-11-17
%define prefix		/usr
	

Summary:	A WYSIWYG GPLized HTML editor
Name:		%{name}
Version:	%{snap}
Release:	%{release}
Source:		ftp://ftp.ratisbona.com/pub/bluefish/snapshots/%{source}.tgz
URL:		http://bluefish.openoffice.nl
License:	GPL
Distribution: 	RedHat 8.*
Packager: 	Matthias Haase <matthias_haase@bennewitz.com>
Group:          Development/Tools
Requires:	gtk2 >= 2.0.6 pcre >= 3.9
BuildRequires:  gtk2-devel >= 2.0.6 pcre-devel >= 3.9

BuildRoot: %{_tmppath}/%{name}-%{snap}

%description
Bluefish is a GTK+ HTML editor for the experienced web designer or
programmer. It is not finished yet, but already a very powerful site
creating environment. Bluefish has extended support for programming
dynamic and interactive websites, there is for example a lot of PHP
support.

%prep
%setup -n bluefish-gtk2

%build
%configure --prefix=%{prefix}
make %{?_smp_mflags}

%install
rm -rf %{buildroot}

%makeinstall pkgdatadir=%{buildroot}%{_datadir}/%{name}

# %find_lang %{name}
install -D -m644 inline_images/bluefish_icon1.png \
	%{buildroot}%{_datadir}/pixmaps/%{name}.png

cat > %{name}.desktop << EOF
[Desktop Entry]
Name=Bluefish HTML editor
Comment=%{summary}
Exec=%{name}
Icon=%{name}.png
Terminal=0
Type=Application
EOF

mkdir -p %{buildroot}%{_datadir}/applications
desktop-file-install --vendor %{desktop_vendor} --delete-original \
  --dir %{buildroot}%{_datadir}/applications                      \
  --add-category X-Red-Hat-Base                                   \
  --add-category Application                                      \
  --add-category Development                                      \
  %{name}.desktop

%clean
rm -rf %{buildroot}

# %files -f %{name}.lang
%files
%defattr(-,root, root)
%doc PORTING
%{_bindir}/*
%dir %{_datadir}/%{name}
%{_datadir}/%{name}/*
%{_datadir}/applications/%{desktop_vendor}-%{name}.desktop
%{_datadir}/pixmaps/%{name}.png


%changelog
* Mon Nov 18 2002 Matthias Haase <matthias_haase@bennewitz.com> gtk2port-2002-11-17
- Change the desktop_vendor
- Rebuild for snap 2002-11-17
- Fix for future releases (if rpm -Uv): Version defined
- Overwritten unpacked name
- Cleanup for the files section

* Sat Nov 16 2002 Matthias Haase <matthias_haase@bennewitz.com> gtk2port-2002-11-08
- Change the desktop_vendor

* Mon Nov 11 2002 Matthias Haase <matthias_haase@bennewitz.com> gtk2port-2002-11-08
- Fixup for required pcre/pcre-devel

* Sat Nov 09 2002 Matthias Haase <matthias_haase@bennewitz.com> gtk2port-2002-11-08
- Rebuild for snap 2002-11-08

* Fri Nov 08 2002 Matthias Haase <matthias_haase@bennewitz.com> gtk2port-2002-10-21
- Initial build for RH 8.0
