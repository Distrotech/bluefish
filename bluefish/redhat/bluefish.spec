%define	desktop_vendor 	endur
%define name  		bluefish
%define version		gtk2
%define release 	1
%define source		bluefish-gtk2port-2002-12-08
%define prefix		/usr
	

Summary:	A WYSIWYG GPLized HTML editor
Name:		%{name}
Version:	%{version}
Release:	%{release}
Source:		ftp://ftp.ratisbona.com/pub/bluefish/snapshots/%{source}.tgz
URL:		http://bluefish.openoffice.nl
License:	GPL
Distribution: 	RedHat 8.*
Packager: 	Matthias Haase <matthias_haase@bennewitz.com>
Group:          Development/Tools
Requires:	gtk2 >= 2.0.6, pcre >= 3.9
BuildRequires:  gtk2-devel >= 2.0.6, pcre-devel >= 3.9

BuildRoot: %{_tmppath}/%{name}-%{version}

%description
Bluefish is a GTK+ HTML editor for the experienced web designer or
programmer. It is not finished yet, but already a very powerful site
creating environment. Bluefish has extended support for programming
dynamic and interactive websites, there is for example a lot of PHP
support.

%prep
%setup -n %{name}-%{version}

%build
CFLAGS="$RPM_OPT_FLAGS"
%configure --prefix=%{prefix}
make %{?_smp_mflags}

%install
rm -rf %{buildroot}

%makeinstall pkgdatadir=%{buildroot}%{_datadir}/%{name}

# %find_lang %{name}
install -D -m644 inline_images/bluefish_icon1.png \
	%{buildroot}%{_datadir}/pixmaps/%{name}.png

cat > bluefish.desktop << EOF
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
%{_datadir}/bluefish/*
%{_datadir}/applications/%{desktop_vendor}-%{name}.desktop
%{_datadir}/pixmaps/%{name}.png


%changelog
* Tue Dec 10 2002 Matthias Haase <matthias_haase@bennewitz.com>
- Rebuild for snapshot 2002-12-08
- Based of the template for auto-builds now

