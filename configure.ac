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
VERSION=gtk2-port
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE")
AC_DEFINE_UNQUOTED(VERSION, "$VERSION")
AC_DEFINE_UNQUOTED(CURRENT_VERSION_NAME, "$PACKAGE $VERSION HTML editor")

ALL_LINGUAS="de da fr ru ro cs es el en ja it sv pt_BR hu pl nl"

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
	CFLAGS="$CFLAGS -fnative-struct"
	LIBS="$LIBS -lintl.dll"
fi  

AC_ARG_ENABLE(gcc3-optimization, 
		AC_HELP_STRING([--enable-gcc3-optimization=type],
				[gcc3 can optimize for: i386, i486, pentium, pentium-mmx, pentiumpro, pentium2, pentium3, pentium4, k6, k6-2, k6-3, athlon, athlon-tbird, athlon-4, athlon-xp, athlon-mp, winchip-c6, winchip2, c3]),
		if gcc -dumpversion|egrep -q "^3\..*"; then
			CFLAGS="-O3 -march=$enableval -fomit-frame-pointer -fno-exceptions -fno-rtti -pipe -s -ffast-math -fexpensive-optimizations -falign-functions -falign-loops"
			echo "using $CC $CFLAGS"
		else
			echo "warning: compiler is not gcc3: not optimizing!"
		fi
)

AC_ARG_ENABLE(gcc2-optimization, 
		AC_HELP_STRING([--enable-gcc2-optimization=type],
				[gcc2 can optimize for: i386, i486, pentium, pentiumpro, k6]),
		if gcc -dumpversion | egrep -q "^2\..*" ; then
			CFLAGS="-O3 -march=$enableval -fomit-frame-pointer -fno-exceptions -fno-rtti -pipe -s -ffast-math -fexpensive-optimizations"
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

AC_PATH_PROG(PCRE_CONFIG, pcre-config, no)
if test "x$PCRE_CONFIG" = "xno" ; then
	AC_MSG_ERROR(pcre-config not found please install libpcre3-dev or similar)
fi
LIBS="$LIBS `$PCRE_CONFIG --libs`"
CFLAGS="$CFLAGS `$PCRE_CONFIG --cflags`"

AC_CHECK_LIB(posix4, nanosleep, 
	LIBS="$LIBS -lposix4"
	AC_DEFINE(HAVE_NANOSLEEP),
	AC_CHECK_LIB(rt, nanosleep,
		LIBS="$LIBS -lrt"
		AC_DEFINE(HAVE_NANOSLEEP)
	)
)

localedir='${prefix}/share/locale'
AC_SUBST(localedir)

dnl ************************
dnl Checks for header files.
dnl ************************

AC_HEADER_STDC
AC_CHECK_HEADERS(stdlib.h unistd.h string.h strings.h errno.h stdio.h sys/stat.h ctype.h getopt.h math.h time.h sys/types.h fcntl.h netdb.h netinet/in.h sys/socket.h arpa/inet.h dirent.h sys/select.h sys/time.h sys/ipc.h)

dnl ************************
dnl Checks for external programs.
dnl ************************


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

AC_PREFIX_DEFAULT("/usr/local")

if test "${prefix}" == "NONE"; then
	prefix=${ac_default_prefix}
	AC_SUBST(prefix)
fi

pkgdatadir=${datadir}/${PACKAGE}/
AC_SUBST(pkgdatadir)

AC_CONFIG_FILES(Makefile icons/Makefile src/Makefile)
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
