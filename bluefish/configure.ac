# Bluefish HTML Editor
# configure.ac - the autoconf file
# Copyright (C) 2002 Olivier Sessink
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

AC_INIT
AC_CONFIG_SRCDIR(src/bluefish.c)
AC_CONFIG_HEADER(src/config.h)


PACKAGE=bluefish
VERSION=pre0.12
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE")
AC_DEFINE_UNQUOTED(VERSION, "$VERSION")
AC_DEFINE_UNQUOTED(CURRENT_VERSION_NAME, "$PACKAGE $VERSION HTML editor")

ALL_LINGUAS="cs da de el es fr hu it ja nl no pl pt_BR ro ru sv"

dnl ************************
dnl Compiler options
dnl ************************

AC_ARG_PROGRAM
AC_PROG_CC
AC_ISC_POSIX
AC_C_INLINE
AC_PROG_INSTALL

if test "$CC" = "gcc" ; then
	CFLAGS="$CFLAGS -Wall -pipe"
fi

AC_CANONICAL_HOST
    case $host_os in
      *cygwin* ) CYGWIN=yes;;
             * ) CYGWIN=no;;
esac
 
if test "$CYGWIN" = "yes"; then
	AC_DEFINE(WIN32)
	CFLAGS="$CFLAGS"
	LIBS="$LIBS -lintl.dll"
fi  

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

		if $CC -dumpversion|egrep -q "^3\.2.*"; then
# gcc-3.2
			if echo $UNAMEP|grep -q "Intel(R) Pentium(R) 4 CPU"; then
				MARCH="-march=pentium4 -mmmx -mfpmath=sse -msse"
			elif echo $UNAMEP|grep -q "Pentium III"; then
				MARCH="-march=pentium3 -mmmx -mfpmath=sse"
			elif echo $UNAMEP|grep -q "AMD-K6(tm) 3D"; then
				MARCH="-march=k6-2 -m3dnow"
			elif echo $UNAMEP|grep -q "Pentium 75 - 200"; then
				MARCH=-march=pentium
			elif echo $UNAMEP|grep -q "Pentium II"; then
				MARCH="-march=pentium2 -mmmx"
			elif echo $UNAMEP|grep -q "AMD Athlon(TM) XP"; then
				MARCH="-march=athlon-xp -mfpmath=sse -mmmx -msse -m3dnow"
			else
				MARCH=""
			fi
			CFLAGS="-DG_DISABLE_ASSERT -O3 $MARCH -malign-double -fomit-frame-pointer -fno-exceptions -pipe -s -ffast-math -fexpensive-optimizations -falign-functions -falign-loops -fstrict-aliasing"
		elif $CC -dumpversion|egrep -q "^3\.0.*"; then
# gcc-3.0
			CFLAGS="-DG_DISABLE_ASSERT -O3 -fomit-frame-pointer -fno-exceptions -pipe -s -ffast-math -fexpensive-optimizations -falign-functions -falign-loops"
		elif $CC -dumpversion | egrep -q "^2\.95.*" ; then
# gcc-2.95
			if echo $UNAMEP|egrep -q "(Pentium III|Pentium II|Pentium\(R\) 4|Athlon)"; then
				MARCH=-march=pentiumpro
			elif echo $UNAMEP|grep -q "AMD-K6"; then
				MARCH=-march=k6
			elif echo $UNAMEP|grep -q "Pentium 75 - 200"; then
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

AC_ARG_WITH(debugging-output,
[  --with-debugging-output Turns debugging output on],[
   echo "With debugging output",
AC_DEFINE(DEBUG)],)

AC_ARG_ENABLE(splash-screen,
[  --disable-splash-screen disables the splash screen],[
   echo "Without splash screen",
	AC_DEFINE(NOSPLASH)],)

AC_ARG_WITH(highlight-profiling,
[  --with-highlight-profiling Enables profiling information about highlighting patterns],[
   echo "With highlighting profiling",
AC_DEFINE(HL_PROFILING)],)

AC_ARG_WITH(bookmarks,
[  --with-bookmarks    enables experimental bookmarks feature],[
   echo "With bookmarks",
AC_DEFINE(BOOKMARKS)],)


dnl ************************
dnl Check for standard headers
dnl ************************

AC_HEADER_STDC

dnl ************************
dnl Checks for librarries
dnl ************************

AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test "x$PKG_CONFIG" = "xno" ; then
	AC_MSG_ERROR(pkg-config not found please install pkg-config)
fi

echo -n "checking for gtk+-2.0... "
if pkg-config --exists gtk+-2.0 ; then
	LIBS="$LIBS `pkg-config --libs gtk+-2.0`"
	CFLAGS="$CFLAGS `pkg-config --cflags gtk+-2.0`"
	echo "yes"
else
	echo "no"
	AC_MSG_ERROR(libgtk2.0-dev missing, please install libgtk2.0-dev)
fi

echo -n "checking for gtk+-2.2... "
if pkg-config gtk+-2.0 --atleast-version=2.2 ; then
	AC_DEFINE(HAVE_ATLEAST_GTK_2_2)
	echo "yes"
else
	echo "no"
fi

echo -n "checking for gnome-vfs-2.0... "
if pkg-config --exists gnome-vfs-2.0 ; then
	LIBS="$LIBS `pkg-config --libs gnome-vfs-2.0`"
	CFLAGS="$CFLAGS `pkg-config --cflags gnome-vfs-2.0`"
	echo "yes"
	AC_DEFINE(HAVE_GNOME_VFS)
else
	echo "no"
	echo "gnome-vfs-2.0 not found, opening remote files will be disabled"
fi

echo -n "checking for gnome-vfs-2.2... "
if pkg-config gnome-vfs-2.0 --atleast-version=2.2 ; then
	AC_DEFINE(HAVE_ATLEAST_GNOMEVFS_2_2)
	echo "yes"
else
	echo "no"
fi

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
		echo "libaspell not found please install libaspell-dev or similar"), 
	echo "libaspell not found please install libaspell-dev or similar"
)

localedir='${prefix}/share/locale'
AC_SUBST(localedir)

dnl ************************
dnl Checks for header files.
dnl ************************

AC_CHECK_HEADERS(stdlib.h unistd.h string.h strings.h errno.h stdio.h sys/stat.h ctype.h getopt.h math.h time.h sys/types.h fcntl.h netdb.h netinet/in.h sys/socket.h arpa/inet.h dirent.h sys/select.h sys/time.h sys/ipc.h sys/msg.h)

dnl ************************
dnl Checks for external programs.
dnl ************************
AC_PATH_PROG(EXTERNAL_FIND, find, /usr/bin/find)
AC_PATH_PROG(EXTERNAL_GREP, grep, /bin/grep)
AC_DEFINE_UNQUOTED(EXTERNAL_FIND, "$EXTERNAL_FIND")
AC_DEFINE_UNQUOTED(EXTERNAL_GREP, "$EXTERNAL_GREP")

dnl **************************
dnl  For the spell checker
dnl **************************

dnl don't define WITH_SPC if you want to build without spell-checker 
AC_DEFINE(WITH_SPC)

AC_PATH_PROG(ISPELL, ispell, /usr/bin/ispell )
AC_DEFINE_UNQUOTED(ISPELL_COMMAND, "$ISPELL")

dnl Checks select argument types
AC_FUNC_SELECT_ARGTYPES
AC_HEADER_TIME

dnl ************************
dnl Program locations
dnl ************************
AC_ARG_WITH(icon-path,
[  --with-icon-path=/path if the icon should not be installed in /usr/share/pixmaps/],[
	ICONPATH=$withval
], [
	ICONPATH=/usr/share/pixmaps/
])
AC_ARG_WITH(gnome1-menu,
[  --with-gnome1-menu=/path if the gnome1 menu is not /usr/share/gnome/apps/],[
	GNOME1MENUPATH=$withval
], [
	GNOME1MENUPATH=/usr/share/gnome/apps/
])
AC_ARG_WITH(gnome2-menu,
[  --with-gnome2-menu=/path if the gnome2 menu is not /usr/share/applications/],[
	GNOME2MENUPATH=$withval
], [
	GNOME2MENUPATH=/usr/share/applications/
])
AC_SUBST(ICONPATH)
AC_SUBST(GNOME1MENUPATH)
AC_SUBST(GNOME2MENUPATH)

AC_PREFIX_DEFAULT("/usr/local")

if test "${prefix}" == "NONE"; then
	prefix=${ac_default_prefix}
	AC_SUBST(prefix)
fi

pkgdatadir=${datadir}/${PACKAGE}/
AC_SUBST(pkgdatadir)

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
fi



AC_CONFIG_FILES(Makefile icons/Makefile src/Makefile po/Makefile data/Makefile)
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
#set
