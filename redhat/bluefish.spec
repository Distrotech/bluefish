%define	desktop_vendor 	endur
%define name  		bluefish
%define version		gtk2
%define release 	20050105
%define epoch 		1
%define source		bluefish-2005-01-05


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
Requires:       shared-mime-info >= 0.15
BuildRequires:  gtk2-devel >= 2.0.6, pcre-devel >= 3.9, gnome-vfs2-devel >= 2.4.1
BuildRequires:  aspell-devel >= 0.50, desktop-file-utils, gettext

BuildRoot: %{_tmppath}/%{name}-%{version}
Patch0:	bluefish_desktop_icon.patch

%description
Bluefish is a GTK+ HTML editor for the experienced web designer or
programmer. It is not finished yet, but already a very powerful site
creating environment. Bluefish has extended support for programming
dynamic and interactive websites, there is for example a lot of PHP
support.

%prep
%setup -q -n %{name}-%{version}
%patch0 -p0

%build
%configure --disable-update-databases
%{__make} %{?_smp_mflags}

%install
%{__rm} -rf %{buildroot}
make install DESTDIR=%{buildroot}

%find_lang %{name}

desktop-file-install --vendor %{desktop_vendor} --delete-original \
  --dir %{buildroot}%{_datadir}/applications                      \
  --add-category X-Red-Hat-Base                                   \
  --add-category Application                                      \
  --add-category Development                                      \
  %{buildroot}%{_datadir}/applications/%{name}.desktop

%triggerin -- shared-mime-info
if [ $2 -eq 1 ]; then
  update-mime-database %{_datadir}/mime > /dev/null 2>&1 || :
fi

%post 
update-desktop-database -q || :

%postun
[ -x /usr/bin/update-mime-database ] && update-mime-database %{_datadir}/mime > /dev/null 2>&1 || :
update-desktop-database -q || :

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


%changelog
* Thu Jan 06 2005 Matthias Haase <matthias_haase@bennewitz.com>
- Automatic build - snapshot of 2005-01-05
