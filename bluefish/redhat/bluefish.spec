%define	desktop_vendor 	endur
%define name  		bluefish
%define version		gtk2
%define release 	20040317
%define epoch 		1
%define source		bluefish-2004-03-17
	

Summary:	A GTK2 web development application for experienced users.
Name:		%{name}
Version:	%{version}
Release:	%{release}
Epoch: 		%{epoch}
Source:		ftp://ftp.ratisbona.com/pub/bluefish/snapshots/%{source}.tar.bz2
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
%configure --with-gnome2-prefix=%{buildroot}/usr/share/
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_datadir}/pixmaps
mkdir -p %{buildroot}%{_datadir}/applications
mkdir -p %{buildroot}%{_datadir}/application-registry
mkdir -p %{buildroot}%{_datadir}/mime-info
make install                                            \
    bindir=%{buildroot}%{_bindir}                       \
    pkgdatadir=%{buildroot}%{_datadir}/%{name}          \
    datadir=%{buildroot}%{_datadir}                     \
    gnome2prefix=%{buildroot}%{_datadir}                \
    gnome1menupath=/_none                               \
    iconpath=%{buildroot}%{_datadir}/pixmaps            \
    localedir=%{buildroot}%{_datadir}/locale

%find_lang %{name}
install -D -m644 inline_images/bluefish_icon1.png \
    %{buildroot}%{_datadir}/pixmaps/%{name}-icon.png

rm -f %{buildroot}%{_datadir}/applications/%{name}.desktop

cat << EOF > %{name}.desktop
[Desktop Entry]
Name=Bluefish web editor
Comment=A GTK2 web development application for experienced users.
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
%{_datadir}/application-registry/%{name}.applications
%{_datadir}/pixmaps/%{name}-icon.png


%changelog
* Thu Mar 18 2004 Matthias Haase <matthias_haase@bennewitz.com>
- Automatic build - snapshot of 2004-03-17
