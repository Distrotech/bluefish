%define	desktop_vendor 	endur
%define name  		bluefish
%define version		gtk2
%define release 	20040721
%define epoch 		1
%define source		bluefish-2004-07-21
	

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
BuildRequires:  gtk2-devel >= 2.0.6, pcre-devel >= 3.9, gnome-vfs2-devel >= 2.4.1
BuildRequires:  aspell-devel >= 0.50, desktop-file-utils, gettext

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
%{__make} %{?_smp_mflags}

%install
%{__rm} -rf %{buildroot}
mkdir -p %{buildroot}%{_datadir}/{application-registry,applications}
mkdir -p %{buildroot}%{_datadir}/{mime-info,pixmaps}

make install DESTDIR=%{buildroot}

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
Terminal=false
Type=Application
EOF
desktop-file-install --vendor %{desktop_vendor} --delete-original \
  --dir %{buildroot}%{_datadir}/applications                      \
  --add-category X-Red-Hat-Base                                   \
  --add-category Application                                      \
  --add-category Development                                      \
  %{name}.desktop

%clean
%{__rm} -rf %{buildroot}

%files -f %{name}.lang
%defattr(-,root, root)
%doc AUTHORS COPYING INSTALL README TODO
%{_bindir}/bluefish
%{_datadir}/bluefish
%{_datadir}/applications/*.desktop
%{_datadir}/application-registry/*
%{_datadir}/mime-info/*
%{_datadir}/pixmaps/*.png


%changelog
* Thu Jul 22 2004 Matthias Haase <matthias_haase@bennewitz.com>
- Automatic build - snapshot of 2004-07-21
