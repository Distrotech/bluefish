# Bluefish HTML Editor
# configure.ac - the autoconf file
# Copyright (C) 2002-2006 Olivier Sessink
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

AC_INIT
AC_CONFIG_SRCDIR(src/bluefish.c)
AC_CONFIG_HEADER(src/config.h)

dnl ************************
dnl Initialize the unstable package
dnl Change PACKAGE when forking the stable tree
dnl ************************

PACKAGE=bluefish-unstable
VERSION=1.1
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)

AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE")
AC_DEFINE_UNQUOTED(VERSION, "$VERSION")
AC_DEFINE_UNQUOTED(CURRENT_VERSION_NAME, "$PACKAGE $VERSION HTML editor")
AC_DEFINE_UNQUOTED(CONFIGURE_OPTIONS, "$0 $*")

# these are the languages that will be installed, so these should be finished
ALL_LINGUAS="da de es fi fr it ja no pl pt pt_BR ru sr sv tr zh_CN zh_TW"
AC_SUBST(ALL_LINGUAS)

dnl ************************
dnl Compiler options
dnl ************************

AC_ARG_PROGRAM
AC_PROG_CC

AC_LIBTOOL_DLOPEN
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

AC_ISC_POSIX
AC_C_INLINE
AC_PROG_INSTALL

if test "$CC" = "gcc" ; then
	CFLAGS="$CFLAGS -Wall -pipe"
fi

#AC_CANONICAL_HOST
#    case $host_os in
#      *cygwin* ) CYGWIN=yes;;
#             * ) CYGWIN=no;;
#esac
 
if test "x$CYGWIN" = "xyes"; then
	AC_DEFINE(WIN32)
	CFLAGS="$CFLAGS"
	LIBS="$LIBS -lintl.dll"
fi  


case "$host_os" in
	solaris*)
		AC_DEFINE(PLATFORM_SOLARIS)
		echo "platform... solaris"
	;;
esac

case "${host}" in
*-apple-darwin*)
	AC_DEFINE(PLATFORM_DARWIN)
	echo "platform... Mac OSX"
	;;
esac

AC_ARG_ENABLE(auto-optimization, 
		AC_HELP_STRING([--enable-auto-optimization],
				[try to automatically determine the most optimal build options for your architecture]),
		UNAMEP=`uname -p`
		if test "x$UNAMEP" = "x"; then
			UNAMEP="unknown"
		fi
		if test "x$UNAMEP" = "xunknown"; then
			UNAMEP=`grep "model name" /proc/cpuinfo | cut -d: -f2`
		fi

		if $CC -dumpversion|egrep -i -q "^3\.(3|4).*"; then
# gcc-3.3 or gcc-3.4
			if echo $UNAMEP|grep -i -q "Intel(R) Pentium(R) 4 CPU"; then
				MARCH="-march=pentium4 -mmmx -mfpmath=sse -msse"
			elif echo $UNAMEP|grep -i -q "Pentium III"; then
				MARCH="-march=pentium3 -mmmx -mfpmath=sse"
			elif echo $UNAMEP|grep -i -q "AMD-K6(tm) 3D"; then
				MARCH="-march=k6-2 -m3dnow"
			elif echo $UNAMEP|grep -i -q "Pentium 75 - 200"; then
				MARCH=-march=pentium
			elif echo $UNAMEP|grep -i -q "Pentium II"; then
				MARCH="-march=pentium2 -mmmx"
			elif echo $UNAMEP|grep -i -q "AMD Athlon(TM) XP"; then
				MARCH="-march=athlon-xp -mfpmath=sse -mmmx -msse -m3dnow"
			elif echo $UNAMEP|grep -i -q "AMD Athlon(TM) MP"; then
				MARCH="-march=athlon-mp -mfpmath=sse -mmmx -msse -m3dnow"
			elif echo $UNAMEP|grep -i -q "AMD Athlon(tm) 64"; then
				MARCH="-march=k8"
			else
				MARCH=""
			fi
			CFLAGS="-DG_DISABLE_ASSERT -O3 $MARCH -malign-double -fno-exceptions -pipe -s -ffast-math"
		elif $CC -dumpversion|egrep -i -q "^3\.2.*"; then
# gcc-3.2
			if echo $UNAMEP|grep -i -q "Intel(R) Pentium(R) 4 CPU"; then
				MARCH="-march=pentium4 -mmmx -mfpmath=sse -msse"
			elif echo $UNAMEP|grep -i -q "Pentium III"; then
				MARCH="-march=pentium3 -mmmx -mfpmath=sse"
			elif echo $UNAMEP|grep -i -q "AMD-K6(tm) 3D"; then
				MARCH="-march=k6-2 -m3dnow"
			elif echo $UNAMEP|grep -i -q "Pentium 75 - 200"; then
				MARCH=-march=pentium
			elif echo $UNAMEP|grep -i -q "Pentium II"; then
				MARCH="-march=pentium2 -mmmx"
			elif echo $UNAMEP|grep -i -q "AMD Athlon(TM) XP"; then
				MARCH="-march=athlon-xp -mfpmath=sse -mmmx -msse -m3dnow"
			else
				MARCH=""
			fi
			CFLAGS="-DG_DISABLE_ASSERT -O3 $MARCH -malign-double -fomit-frame-pointer -fno-exceptions -pipe -s -ffast-math -fexpensive-optimizations -falign-functions -falign-loops -fstrict-aliasing"
		elif $CC -dumpversion|egrep -i -q "^3\.0.*"; then
# gcc-3.0
			CFLAGS="-DG_DISABLE_ASSERT -O3 -fomit-frame-pointer -fno-exceptions -pipe -s -ffast-math -fexpensive-optimizations -falign-functions -falign-loops"
		elif $CC -dumpversion | egrep -i -q "^2\.95.*" ; then
# gcc-2.95
			if echo $UNAMEP|egrep -i -q "(Pentium III|Pentium II|Pentium\(R\) 4|Athlon)"; then
				MARCH=-march=pentiumpro
			elif echo $UNAMEP|grep -i -q "AMD-K6"; then
				MARCH=-march=k6
			elif echo $UNAMEP|grep -i -q "Pentium 75 - 200"; then
				MARCH=-march=pentium
			else 
				MARCH=""
			fi
			CFLAGS="-DG_DISABLE_ASSERT $MARCH -O3 -fomit-frame-pointer -fno-exceptions -pipe -s -ffast-math -fexpensive-optimizations"
		else
			echo "warning: compiler is not known: not optimizing!"
		fi
		echo "using $CC $CFLAGS"
)

AC_ARG_ENABLE(gcc3-optimization, 
		AC_HELP_STRING([--enable-gcc3-optimization=type],
				[gcc3 can optimize for: i386, i486, pentium, pentium-mmx, pentiumpro, pentium2, pentium3, pentium4, k6, k6-2, k6-3, athlon, athlon-tbird, athlon-4, athlon-xp, athlon-mp, winchip-c6, winchip2, c3]),
		if $CC -dumpversion|egrep -q "^3\..*"; then
			CFLAGS="-O3 -march=$enableval -fomit-frame-pointer -fno-exceptions -pipe -s -ffast-math -fexpensive-optimizations -falign-functions -falign-loops"
			echo "using $CC $CFLAGS"
		else
			echo "warning: compiler is not gcc3: not optimizing!"
		fi
)

AC_ARG_ENABLE(gcc2-optimization, 
		AC_HELP_STRING([--enable-gcc2-optimization=type],
				[gcc2 can optimize for: i386, i486, pentium, pentiumpro, k6]),
		if $CC -dumpversion | egrep -q "^2\..*" ; then
			CFLAGS="-O3 -march=$enableval -fomit-frame-pointer -fno-exceptions -pipe -s -ffast-math -fexpensive-optimizations"
			echo "using $CC $CFLAGS"
		else 
			echo "warning: compiler is not gcc2: not optimizing!"
		fi
)

AC_PROG_INSTALL

AC_ARG_ENABLE(debugging-output,
	AC_HELP_STRING([--enable-debugging-output],[Turns massive debugging output on, will slow down the program!]),[
   echo "With debugging output",
AC_DEFINE(DEBUG)],)

AC_ARG_ENABLE(splash-screen,
	AC_HELP_STRING([--disable-splash-screen],[disables the splash screen]),[
   echo "Without splash screen",
	AC_DEFINE(NOSPLASH)],)

AC_ARG_ENABLE(highlight-profiling,
	AC_HELP_STRING([--enable-highlight-profiling],[Enables profiling information about highlighting patterns]),[
   echo "With highlighting profiling",
AC_DEFINE(HL_PROFILING)],)

AC_ARG_ENABLE(development,
	AC_HELP_STRING([--enable-development],[Enables development checks, will slow down the program!]),[
   echo "With development checks"
	AC_DEFINE(DEVELOPMENT)],)

AC_ARG_ENABLE(gprof-profiling,
	AC_HELP_STRING([--enable-gprof-profiling],[Enables gprof profiling, will slow down the program!]),[
   echo "With gprof profiling"
	CFLAGS="$CFLAGS -pg"
	LDFLAGS="$LDFLAGS -pg"],)

AC_ARG_ENABLE(gcov-coverage,
	AC_HELP_STRING([--enable-gcov-coverage],[Enables coverage, will slow down the program!]),[
   echo "With gcov coverage"
	CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"],)

AC_ARG_ENABLE(scanner,
	AC_HELP_STRING([--disable-scanner],[Do not use BfTextView scanner for editor widget]),
	scanner=$enableval,
	scanner=yes
)
if test "$scanner" != no; then
	AC_DEFINE(USE_SCANNER)
fi


dnl ************************
dnl Check for standard headers
dnl ************************

AC_HEADER_STDC

dnl ************************
dnl Checks for librarries
dnl ************************

AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test "x$PKG_CONFIG" = "xno" ; then
	AC_MSG_ERROR([pkg-config not found please install pkg-config])
fi

echo -n "checking for gtk+ ... "
if pkg-config --exists gtk+-2.0 ; then
	PKGCONFIGFILES="${PKGCONFIGFILES} gtk+-2.0"
	echo "yes, `pkg-config --modversion gtk+-2.0`"
else
	echo "no"
	AC_MSG_ERROR([libgtk2.0-dev missing, please install libgtk2.0-dev])
fi

echo -n "checking for gtk+ >= 2.6... "
if pkg-config gtk+-2.0 --atleast-version=2.6 ; then
	echo "yes"
else
	AC_MSG_ERROR([detected gtk $VERSION, but gtk version >= 2.6 is required])
fi

echo -n "checking for gmodule >= 2.6... "
if pkg-config gmodule-2.0 --atleast-version=2.6 ; then
	echo "yes"
	PKGCONFIGFILES="${PKGCONFIGFILES} gmodule-2.0"
else
	echo "no"
	AC_MSG_ERROR([gmodule not detected, or too old, gmodule is part of glib])
fi

echo -n "checking for gtk+ requirements: gobject, glib, gdk-pixbuf, gdk, and pango ... "
if pkg-config --exists gobject-2.0 ; then
	PKGCONFIGFILES="${PKGCONFIGFILES} gobject-2.0"
fi
if pkg-config --exists glib-2.0; then
	PKGCONFIGFILES="${PKGCONFIGFILES} glib-2.0"
fi
if pkg-config --exists pango; then
	PKGCONFIGFILES="${PKGCONFIGFILES} pango"
fi
if pkg-config --exists gdk-pixbuf-2.0; then
	PKGCONFIGFILES="${PKGCONFIGFILES} gdk-pixbuf-2.0"
fi
if pkg-config --exists gdk-2.0; then
	PKGCONFIGFILES="${PKGCONFIGFILES} gdk-2.0"
fi
echo " done"

echo -n "checking for gnome-vfs-2.0... "
if pkg-config --exists gnome-vfs-2.0 --atleast-version=2.8 ; then
	PKGCONFIGFILES="${PKGCONFIGFILES} gnome-vfs-2.0"
	echo "yes, `pkg-config --modversion gnome-vfs-2.0`"
	BF_APP_EXPECT_URIS="true"
	BF_APP_SUPPORTED_URI_SCHEMES="file,http,ftp,sftp,smb"
	AC_SUBST(BF_APP_EXPECT_URIS)
	AC_SUBST(BF_APP_SUPPORTED_URI_SCHEMES)
else
	AC_MSG_ERROR([gnome-vfs >= 2.8 is required, please install libgnomevfs2-dev])
fi

#echo -n "checking for gnome-vfs >= 2.8... "
#if pkg-config gnome-vfs-2.0 --atleast-version=2.8 ; then
#	AC_DEFINE(HAVE_ATLEAST_GNOMEVFS_2_8)
#	echo "yes"
#else
#	echo "no"
#	AC_MSG_ERROR([gnome-vfs 2.8 is required, please upgrade to at least 2.8])
#fi

echo -n "checking for libxml... "
if pkg-config --exists libxml-2.0 ; then
	PKGCONFIGFILES="${PKGCONFIGFILES} libxml-2.0"
	echo "yes, `pkg-config --modversion libxml-2.0`"
else
	AC_MSG_ERROR([libxml2 is required, please install libxml2-dev])
fi

AC_ARG_WITH(libgnomeui, 
	AC_HELP_STRING([--without-libgnomeui],[disable libgnomeui detection]),[
	LGNOMEUI=$withval
	],[
	LGNOMEUI=yes]
)
if test "x${LGNOMEUI}" = "xyes" ; then
	echo -n "checking for libgnomeui >= 2.6... "
	if pkg-config libgnomeui-2.0 --atleast-version=2.6 ; then
		AC_DEFINE(HAVE_ATLEAST_GNOMEUI_2_6)
		PKGCONFIGFILES="${PKGCONFIGFILES} libgnomeui-2.0"
		echo "yes"
	else
		echo "no"
	fi
fi

dnl ******************************************************
dnl handling all pkg-config libraries into LIBS and CFLAGS
dnl ******************************************************

LIBS="$LIBS `pkg-config --libs ${PKGCONFIGFILES}`"
CFLAGS="$CFLAGS `pkg-config --cflags ${PKGCONFIGFILES}`"

AC_PATH_PROG(PCRE_CONFIG, pcre-config, no)
if test "x$PCRE_CONFIG" = "xno" ; then
	AC_MSG_ERROR(pcre-config not found please install libpcre3-dev or similar)
else 
	LIBS="$LIBS `$PCRE_CONFIG --libs`"
	CFLAGS="$CFLAGS `$PCRE_CONFIG --cflags`"
fi

AC_CHECK_LIB(posix4, nanosleep, 
	LIBS="$LIBS -lposix4"
	AC_DEFINE(HAVE_NANOSLEEP),
	AC_CHECK_LIB(rt, nanosleep,
		LIBS="$LIBS -lrt"
		AC_DEFINE(HAVE_NANOSLEEP)
	)
)

AC_CHECK_HEADER(aspell.h, 
	AC_CHECK_LIB(aspell, new_aspell_config, 
		LIBS="$LIBS -laspell"
		AC_DEFINE(HAVE_LIBASPELL),
		OLDLIBS=$LIBS
		LIBS="$OLDSLIBS -lstdc++"
		AC_CHECK_LIB(aspell, new_aspell_config,
			LIBS="$LIBS -laspell"
			AC_DEFINE(HAVE_LIBASPELL),
			LIBS="$OLDSLIBS -laspell-common -lstdc++"
			AC_CHECK_LIB(aspell, new_aspell_config,
				LIBS="$LIBS -laspell"
				AC_DEFINE(HAVE_LIBASPELL),
				LIBS=$OLDLIBS
				echo "libaspell not found please install libaspell-dev or similar"
			)
		)
	)
)

AC_ARG_ENABLE(python,
	AC_HELP_STRING([--disable-python],[don't build the python plugin]),
	python=$enableval,
	python=yes
)
if test "$python" = yes; then
	AC_PATH_PROG(pythonpath, python)
	if test "_$pythonpath" = _ ; then
		AC_PATH_PROG(pythonpath, python2)
	fi
	if test "_$pythonpath" = _ ; then
		python=no
	else
		AC_MSG_CHECKING(Python version)
		changequote(<<, >>)dnl
		PY_VER=`$pythonpath -c 'import distutils.sysconfig; print distutils.sysconfig.get_config_vars("VERSION")[0];'`
		PY_LIB=`$pythonpath -c 'import distutils.sysconfig; print distutils.sysconfig.get_python_lib(standard_lib=1);'`
		PY_INC=`$pythonpath -c 'import distutils.sysconfig; print distutils.sysconfig.get_config_vars("INCLUDEPY")[0];'`
		$pythonpath -c "import sys; map(int,sys.version[:3].split('.'))>= [2,2] or sys.exit(1)"
		changequote([, ])dnl
		AC_MSG_RESULT($PY_VER)
		if test "$?" != "1"; then
			AC_MSG_CHECKING(Python compile flags)
			PY_PREFIX=`$pythonpath -c 'import sys; print sys.prefix'`
			PY_EXEC_PREFIX=`$pythonpath -c 'import sys; print sys.exec_prefix'`
			if test -f $PY_INC/Python.h; then
				AC_MSG_RESULT([-I${PY_INC}])
				AC_MSG_CHECKING(Python link flags)
				PYSHARED=`$pythonpath -c 'import distutils.sysconfig;print distutils.sysconfig.get_config_var("LIBS")'`
				AC_MSG_RESULT([-L${PY_LIB}/config -lpython${PY_VER} ${PYSHARED}])
				LIBS="$LIBS -L${PY_LIB}/config -lpython${PY_VER} ${PYSHARED}"
				CFLAGS="$CFLAGS -I$PY_INC"
			else
				python=no
				AC_MSG_RESULT([Can't find Python.h])
			fi
		else
			echo "Python too old. Only 2.2 or above is supported."
			python=no
		fi
	fi
fi
if test "$python" = yes; then
	AC_DEFINE(HAVE_PYTHON)
fi

localedir='${prefix}/share/locale'
AC_SUBST(localedir)

dnl ************************
dnl Checks for header files.
dnl ************************

AC_CHECK_HEADERS(stdlib.h unistd.h string.h strings.h errno.h stdio.h sys/stat.h ctype.h getopt.h math.h time.h sys/types.h fcntl.h netdb.h netinet/in.h sys/socket.h arpa/inet.h dirent.h sys/select.h sys/time.h sys/ipc.h sys/msg.h)

AC_CHECK_FUNC(msgrcv,
	AC_DEFINE(HAVE_MSGRCV)
)
AC_CHECK_FUNC(msgsnd,
	AC_DEFINE(HAVE_MSGSND)
)

AC_CHECK_FUNC(ctime,
	AC_DEFINE(HAVE_CTIME)
)
AC_CHECK_FUNC(ctime_r,
	AC_DEFINE(HAVE_CTIME_R)
)
AC_CHECK_FUNC(asctime,
	AC_DEFINE(HAVE_ASCTIME)
)
AC_CHECK_FUNC(asctime_r,
	AC_DEFINE(HAVE_ASCTIME_R)
)
AC_CHECK_FUNC(localtime,
	AC_DEFINE(HAVE_LOCALTIME)
)

dnl **************************
dnl  For the spell checker
dnl **************************
dnl don't define WITH_SPC if you want to build without spell-checker 
AC_DEFINE(WITH_SPC)

dnl Checks select argument types
AC_FUNC_SELECT_ARGTYPES
AC_HEADER_TIME

dnl ************************
dnl Program locations
dnl ************************

#####################################################

if test "${prefix}" == "NONE"; then
	prefix=${ac_default_prefix}
	AC_SUBST(prefix)
fi

AC_ARG_ENABLE([update-databases],
	AC_HELP_STRING([--disable-update-databases],[do not run the update-desktop-database or update-mime-database utilities, mostly useful for package maintainers]),[
	UPDATEDATABASES=$enableval
	],[
	UPDATEDATABASES=yes]
)

AC_ARG_WITH(freedesktop_org-menu, 
	AC_HELP_STRING([--without-freedesktop_org-menu],[disable freedesktop_org menu, use --with-freedesktop_org-menu=/path to override the default location /usr/share/applications]),[
	FDMENU=$withval
	],[
	FDMENU=yes]
)
if test "${FDMENU}" != "no" ; then
	if test "${FDMENU}" = "yes" ; then
		echo -n "checking freedesktop.org menu directory... "
		if test -d /usr/share/applications ; then
			FDMENU=/usr/share/applications
		else
			if test -d ${prefix}/share/applications ; then
				FDMENU=${prefix}/share/applications
			else
				if test -d /usr/X11R6/share/gnome/applications ; then
					FDMENU=/usr/X11R6/share/gnome/applications
				else
					if test -d ${prefix}/share/gnome/applications ; then
						FDMENU=${prefix}/share/gnome/applications
					else
						FDMENU=no
					fi
				fi
			fi
		fi
		echo $FDMENU
	fi
	if test "${FDMENU}" != "no" ; then
		INSTALLDATA="$INSTALLDATA install-fdmenu"
		AC_SUBST(FDMENU)
		AC_PATH_PROG(UPDATE_DESKTOP, update-desktop-database, "no")
		if test "${UPDATEDATABASES}" != "no" -a "${UPDATE_DESKTOP}" != "no"; then
			INSTALLDATA="$INSTALLDATA install-update-menu"
			AC_SUBST(UPDATE_DESKTOP)
		fi
	fi
fi

AC_ARG_WITH(freedesktop_org-mime, 
	AC_HELP_STRING([--without-freedesktop_org-mime],[disable freedesktop_org mime, use --with-freedesktop_org-mime=/path to override the default location /usr/share/mime]),[
	FDMIME=$withval
	],[
	FDMIME=yes]
)
if test "${FDMIME}" != "no" ; then
	if test "${FDMIME}" = "yes" ; then
		echo -n "checking freedesktop.org mime directory... "
		if test -d /usr/share/mime ; then
			FDMIME=/usr/share/mime
		else
			if test -d ${prefix}/share/mime ; then
				FDMIME=${prefix}/share/mime
			else
				if test -d /usr/X11R6/share/gnome/mime ; then
					FDMIME=/usr/X11R6/share/gnome/mime
				else
					if test -d ${prefix}/share/gnome/mime ; then
						FDMIME=${prefix}/share/gnome/mime
					else
						FDMIME=no
					fi
				fi
			fi
		fi
		echo ${FDMIME}
	fi
	if test "${FDMIME}" != "no" ; then
		INSTALLDATA="$INSTALLDATA install-fdmime"
		AC_SUBST(FDMIME)
		AC_PATH_PROG(UPDATE_MIME, update-mime-database, "no")
		if test "${UPDATEDATABASES}" != "no" -a "${UPDATE_MIME}" != "no"; then
			INSTALLDATA="install-update-mime $INSTALLDATA"
			AC_SUBST(UPDATE_MIME)
		fi
	fi
fi

AC_ARG_WITH(gnome2_4-mime, 
	AC_HELP_STRING([--without-gnome2_4-mime],[disable gnome2.4 mime install, use --with-gnome2_4-mime=/path to override the default /usr/share/mime-info]),[
	GMIMEINFO=$withval
	],[
	GMIMEINFO=yes]
)
if test "${GMIMEINFO}" != "no" ; then
	if test "${GMIMEINFO}" = "yes" ; then
		echo -n "checking gnome 2.4 mime-info directory... "
		if test -d /usr/share/mime-info ; then
			GMIMEINFO=/usr/share/mime-info
		else
			if test -d ${prefix}/share/mime-info ; then
				GMIMEINFO=${prefix}/share/mime-info
			else
				# FreeBSD defaults to:
				if test -d /usr/X11R6/share/gnome/mime-info ; then
					GMIMEINFO=/usr/X11R6/share/gnome/mime-info
				else
					if test -d ${prefix}/share/gnome/mime-info ; then
						GMIMEINFO=${prefix}/share/gnome/mime-info
					else
						GMIMEINFO=no
					fi
				fi
			fi
		fi
		echo $GMIMEINFO
	fi
	if test "${GMIMEINFO}" != "no" ; then
		AC_SUBST(GMIMEINFO)
		INSTALLDATA="$INSTALLDATA install-gnome-2.4-mime"
	fi
fi

AC_ARG_WITH(gnome2_4-appreg, 
	AC_HELP_STRING([--without-gnome2_4-appreg],[disable gnome2.4 application-registry install, use --with-gnome2_4-appreg=/path to override the default /usr/share/application-registry]),[
	GAPPREG=$withval
	],[
	GAPPREG=yes]
)
if test "${GAPPREG}" != "no" ; then
	if test "${GAPPREG}" = "yes" ; then
		echo -n "checking gnome 2.4 application-registry directory... "
		if test -d /usr/share/application-registry ; then
			GAPPREG=/usr/share/application-registry
		else
			if test -d ${prefix}/share/application-registry ; then
				GAPPREG=${prefix}/share/application-registry
			else
				# FreeBSD defaults to:
				if test -d /usr/X11R6/share/gnome/application-registry ; then
					GAPPREG=/usr/X11R6/share/gnome/application-registry
				else
					if test -d ${prefix}/share/gnome/application-registry ; then
						GAPPREG=${prefix}/share/gnome/application-registry
					else
						GAPPREG=no
					fi
				fi
			fi
		fi
		echo $GAPPREG
	fi
	if test "${GAPPREG}" != "no" ; then 
		INSTALLDATA="$INSTALLDATA install-gnome-2.4-appreg"
		AC_SUBST(GAPPREG)
	fi
fi

AC_ARG_WITH(theme-path,
	AC_HELP_STRING([--without-theme-path],[do not install theme icons in /usr/share/icons/hicolor, use --with-theme-path=/path to override the default path or theme (GNOME >= 2.8 users should use --without-icon-path in addition to this option)]),[
	THEMEPATH=$withval
], [
	THEMEPATH=yes
])

if test "x$THEMEPATH" != "xno" ; then
	if test "x${THEMEPATH}" = "xyes" ; then
		echo -n "checking theme path... "
		# the default answer is yes, so this is for users that did not use theicon-path option at all
		if test -d /usr/share/icons/hicolor ; then
			THEMEPATH=/usr/share/icons/hicolor
		else
			if test -d ${prefix}/share/icons/hicolor ; then
				THEMEPATH=${prefix}/share/icons/hicolor
			else
				# FreeBSD defaults to:
				if test -d /usr/X11R6/share/gnome/icons/hicolor ; then
					THEMEPATH=/usr/X11R6/share/gnome/icons/hicolor
				else
					if test -d ${prefix}/share/gnome/icons/hicolor ; then
						THEMEPATH=${prefix}/share/gnome/icons/hicolor
					else
						# the user did not specify an theme-path, and the default path
						# seems to be wrong for this system
						# we will not install an theme icons
						THEMEPATH=no
					fi
				fi
			fi
		fi
		echo $THEMEPATH
	fi	
	if test "${THEMEPATH}" != "no" ; then
		AC_SUBST(THEMEPATH)
		INSTALLMAIN="${INSTALLMAIN} install-theme-icon"
		INSTALLDATA="${INSTALLDATA} install-theme-icon"
	fi
fi

AC_ARG_WITH(icon-path,
	AC_HELP_STRING([--without-icon-path],[do not install icon in /usr/share/pixmaps, use --with-icon-path=/path to override the default path (you should use --without-theme-path in addition to this option; if the theme path cannot be determined, --without-icon-path will not have any effect)]),[
	ICONPATH=$withval
], [
	ICONPATH=yes
])

if test "x$ICONPATH" != "xno" -o "${THEMEPATH}" = "no" ; then
	if test "x${ICONPATH}" = "xyes" ; then
		echo -n "checking icon path... "
		# the default answer is yes, so this is for users that did not use theicon-path option at all
		if test -d /usr/share/pixmaps ; then
			ICONPATH=/usr/share/pixmaps
		else
			if test -d ${prefix}/share/pixmaps ; then
				ICONPATH=${prefix}/share/pixmaps
			else
				# FreeBSD defaults to:
				if test -d /usr/X11R6/share/gnome/pixmaps ; then
					ICONPATH=/usr/X11R6/share/gnome/pixmaps
				else
					if test -d ${prefix}/share/gnome/pixmaps ; then
						ICONPATH=${prefix}/share/gnome/pixmaps
					else
						# the user did not specify an icon-path, and the default path
						# seems to be wrong for this system
						# we will not install an icon
						ICONPATH=no
					fi
				fi
			fi
		fi
		echo $ICONPATH
	fi
	if test "${ICONPATH}" != "no" ; then
		AC_SUBST(ICONPATH)
		INSTALLMAIN="${INSTALLMAIN} install-icon"
		if test "${GMIMEINFO}" != "no" ; then
			INSTALLDATA="${INSTALLDATA} install-icon"
		fi
	fi
fi

pkgdatadir=${datadir}/${PACKAGE}/
AC_SUBST(pkgdatadir)
pkglibdir=${libdir}/${PACKAGE}/
AC_SUBST(pkglibdir)


eval BF_APP_PKGDATADIR=`echo $pkgdatadir`
AC_SUBST(BF_APP_PKGDATADIR)

dnl *******************************
dnl   GETTEXT
dnl *******************************
AM_GNU_GETTEXT
AC_PATH_PROG(MSGMERGE, msgmerge, /usr/bin/msgmerge)

if test ! -x "$GMSGFMT" ; then
	echo "no gmsgfmt - NLS disabled"
	USE_NLS=no
fi
if test ! -x "$MSGFMT" ; then
	echo "no msfmt - NLS disabled"
	USE_NLS=no
fi

dnl OpenBSD has gettext in -lintl

if test "$USE_NLS" = "yes" ; then
	AC_CHECK_LIB(intl, gettext,
		LIBS="$LIBS -lintl"
	)
	AC_DEFINE(ENABLE_NLS)
	INSTALLMAIN="${INSTALLMAIN} install-nls"
	MAKEMAIN="${MAKEMAIN} make-nls"
fi

if test "${ICONPATH}" != "no"; then
	eval ICONDIR=`echo $ICONPATH`
	BF_APP_ICON="${ICONDIR}/${PACKAGE}-icon.png"
	AC_SUBST(BF_APP_ICON)
else
	if test "${THEMEPATH}" != "no"; then
		BF_APP_ICON="${PACKAGE}"
		AC_SUBST(BF_APP_ICON)
	else
		BF_APP_ICON="${PACKAGE}-icon.png"
		AC_SUBST(BF_APP_ICON)
	fi
fi

AC_SUBST(INSTALLDATA)
AC_SUBST(INSTALLMAIN)
AC_SUBST(MAKEMAIN)

AC_CONFIG_FILES(
	Makefile
	icons/Makefile
	src/Makefile
	po/Makefile
	data/Makefile
	data/filetypes
	data/bluefish.applications
	data/bluefish.desktop
	data/bluefish-project.desktop
	data/bluefish.keys
	man/Makefile
	src/plugin_htmlbar/Makefile
	src/plugin_htmlbar/po/Makefile
	src/plugin_about/Makefile
	src/plugin_about/po/Makefile
	src/plugin_entities/Makefile
	src/plugin_entities/po/Makefile)

AC_OUTPUT

echo "-----------"
echo "If you like this program, please let me know and send me"
echo "a postcard and tell me what city/country you're from:"
echo ""
echo "Olivier Sessink"
echo "Thorbeckestraat 470"
echo "6702 CJ"
echo "Wageningen"
echo "The Netherlands"
echo "-----------"
