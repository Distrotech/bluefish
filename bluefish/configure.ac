dnl ************************
dnl configure.ac
dnl $Id$
dnl
dnl Copyright (C) 2002-2011 Olivier Sessink
dnl Copyright (C) 2006-2012 Daniel Leidert
dnl
dnl This file is free software. The copyright owner gives unlimited
dnl permission to copy, distribute and modify it.
dnl ************************

AC_PREREQ([2.59])

AC_INIT([bluefish],[2.2.4],[http://bugzilla.gnome.org/buglist.cgi?product=bluefish])

dnl Have to save them here!
if test "${ac_configure_args+set}" != "set" ; then
	AC_DEFINE_UNQUOTED([CONFIGURE_OPTIONS], ["$0 $@"], [The configure options used to compile bluefish.])
else
	AC_DEFINE_UNQUOTED([CONFIGURE_OPTIONS], ["$0 $ac_configure_args"], [The configure options used to compile bluefish.])
fi

dnl The "-Wno-portability" value will suppress the GNU make warnings. Add it if necessary.
AM_INIT_AUTOMAKE([1.8 -Wall])
AC_CONFIG_SRCDIR(src/bluefish.c)
AC_CONFIG_HEADERS(src/config.h)
AC_CANONICAL_HOST

AC_REVISION($Revision$)

AM_MAINTAINER_MODE

dnl ************************
dnl   I18N options
dnl ************************

m4_define([_BF_LINGUAS], [bg ca cs da de el en es eu fi fr gl hu it ja ko nb nl nn pl pt pt_BR ro ru sk sr sv ta tr uk zh_CN zh_TW])
#ALL_LINGUAS="bg cs da de el es eu fi fr gl hu it ja ko nb nl pl pt pt_BR ro ru sk sr sv ta tr uk zh_CN zh_TW"
AC_SUBST([ALL_LINGUAS], "_BF_LINGUAS")
GETTEXT_PACKAGE=AC_PACKAGE_NAME
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [gettext domain])
AM_GLIB_GNU_GETTEXT

IT_PROG_INTLTOOL
dnl if AM_PO_SUBDIRS is available use it (most recent systems)
dnl else try to use the very old AM_WITH_NLS
dnl or try AM_GNU_GETTEXT at last 
m4_ifdef(
	[AM_PO_SUBDIRS],
	[AM_PO_SUBDIRS],
	[
	 m4_ifdef(
	 	[AM_WITH_NLS],
	 	[AM_WITH_NLS([external])],
	 	[AM_GNU_GETTEXT([external])]
	 )
	]
)

BF_DEFINE_LINGUAS(_BF_LINGUAS)

dnl ************************
dnl   Compiler options
dnl ************************

#AC_ARG_PROGRAM
AC_PROG_CC
m4_ifdef(
	[LT_INIT],
	[
	 dnl libtool 2.x macros
	 LT_INIT([disable-static dlopen win32-dll])
	 LT_PROG_RC
	],[
	 dnl libtool 1.4 macros.
	 AC_DISABLE_STATIC
	 AC_LIBTOOL_DLOPEN
	 AC_LIBTOOL_WIN32_DLL
	 AC_LIBTOOL_RC
	 AC_PROG_LIBTOOL
	]
)
m4_ifdef([DOLT], [DOLT])
AC_ISC_POSIX
AC_C_INLINE
AC_PROG_INSTALL

if test "$CC" = "gcc" ; then
	CFLAGS="$CFLAGS -Wall -pipe"
fi

case "$host_os" in
	windows* | cygwin* | mingw*)
		AC_DEFINE([WIN32], [1], [Define to 1 if you have a Cygwin/Windows OS.])
		LIBS="$LIBS -lintl.dll"
		AC_MSG_NOTICE([platform is... Windows/Cygwin])
	;;
	solaris*)
		AC_DEFINE([PLATFORM_SOLARIS], [1], [Define to 1 if you have a Solaris OS.])
		AC_MSG_NOTICE([platform is... Solaris])
	;;
esac

case "$host" in
	*-*-mingw* | *cygwin* | *windows*)
		native_win32=yes
	;;		
	*-apple-darwin*)
		AC_DEFINE([PLATFORM_DARWIN], [1], [Define to 1 if you have a Darwin host.])
		AC_MSG_NOTICE([platform is... MacOSX])
	;;
	*)
		native_win32=no
	;;
esac

if test "x$native_win32" = "xyes"; then
	AC_CHECK_TOOL([WINDRES], [windres], [no])
	if test "x$WINDRES" = "xno"; then
		AC_MSG_ERROR([The tool 'windres' was not found. Aborting.])
	fi
	AC_PROG_SED
	AC_SUBST([SPLIT_VERSION], [`echo $VERSION | $SED -e 's/-.*//' | $SED -e 's/\./,/g'`])
fi
AM_CONDITIONAL([OS_WIN32], [test "x$native_win32" = "xyes"])

dnl ************************
dnl   Necessary tools
dnl ************************

AC_ARG_VAR(
	[GDK_PIXBUF_CSOURCE],
	[The 'gdk-pixbuf-csource' executable. Use it to define or override its location.]
)
AC_CHECK_PROG([GDK_PIXBUF_CSOURCE], [gdk-pixbuf-csource], [gdk-pixbuf-csource])

dnl ************************
dnl   Configure options e/d
dnl ************************

AC_ARG_ENABLE(
	[debugging-output],
	AC_HELP_STRING(
		[--enable-debugging-output],
		[enable massive debugging output (slows down the program) @<:@default=no@:>@]
	),
	[
	 if test "x$enableval" != "xno"; then
		 AC_DEFINE([DEBUG], [1], [Define to 1 if you want to have a massive amount of debug output.])
	 fi
	]
)

AC_ARG_ENABLE(
	[highlight-profiling],
	AC_HELP_STRING(
		[--enable-highlight-profiling],
		[enable profiling information about highlighting patterns (slows down the program) @<:@default=no@:>@]
	),
	[
	 if test "x$enableval" != "xno"; then
		 AC_DEFINE([HL_PROFILING], [1], [Define to 1 if you want to have profiling information about highlighting patterns.])
	 fi
	]
)

dnl # make DEVELOPEMENT enabled when compiling from svn, and disabled when compiling from an exported source tree.
AC_MSG_CHECKING([for svn tree or exported source tree])
BF_want_development="no"
BF_svnversion=`LANG=C svnversion -n . || echo exported`
if test "x$BF_svnversion" != "xexported" ; then
	AC_MSG_RESULT([svn])
	BF_want_development="yes"
else
	AC_MSG_RESULT([exported])
fi
AC_ARG_ENABLE(
	[development],
	AC_HELP_STRING(
		[--enable-development],
		[enable development checks, recommended when building from svn @<:@default=no@:>@]
	),
	[
		if test "x$enableval" != "xno"; then
			BF_want_development="yes"
		else
			BF_want_development="no"
		fi
	]
)
if test "x$BF_want_development" != "no" ; then
	AC_DEFINE([DEVELOPMENT], [1], [Define to 1 if you want to have development checks. Recommended when compiling from svn.])
fi

AC_ARG_ENABLE(
	[gprof-profiling],
	AC_HELP_STRING(
		[--enable-gprof-profiling],
		[enable gprof profiling (slows down the program) @<:@default=no@:>@]
	),
	[
	 if test "x$enableval" != "xno"; then
		 CFLAGS="$CFLAGS -pg"
		 LDFLAGS="$LDFLAGS -pg"
	 fi
	]
)

AC_ARG_ENABLE(
	[gcov-coverage],
	AC_HELP_STRING(
		[--enable-gcov-coverage],
		[enable coverage (slows down the program) @<:@default=no@:>@]
	),
	[
	 if test "x$enableval" != "xno"; then
		 CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
	 fi
	]
)

AC_ARG_ENABLE(
	[deprecated],
	AC_HELP_STRING(
		[--disable-deprecated],
		[enable/disable deprecated functions (e.g. Glib/Gtk) @<:@default=yes@:>@]
	),
	[
	 if test "x$enableval" = "xno"; then
		 CFLAGS="$CFLAGS -DG_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES"
		 CFLAGS="$CFLAGS -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED"
		 CFLAGS="$CFLAGS -DG_DISABLE_DEPRECATED"
		 CFLAGS="$CFLAGS -DGTK_DISABLE_DEPRECATED"
		 CFLAGS="$CFLAGS -DGSEAL_ENABLE"
	 fi
	]
)

dnl To create our catalog and maybe update the systems XML catalog, we need xmlcatalog.
dnl But because several systems like Debian or Mac use their own system, so let the user
dnl decide, if he wants to update the sytems catalog.
AC_PATH_PROG([XMLCATALOG], [xmlcatalog], [no])
if test "x$XMLCATALOG" = "xno"; then
	AC_MSG_WARN([Install the xmlcatalog application, which is delivered with the GNOME xml-library.])
fi

AC_ARG_ENABLE(
	[xml-catalog-update],
	AC_HELP_STRING(
		[--enable-xml-catalog-update],
		[
		 enable (or disable) the running of xmlcatalog after installation -
		 some distributions like Debian use their own system for registering
		 catalogs, so be careful here and also check the '--with-xml-catalog'
		 option @<:@default=yes@:>@
		]
	),
)

AC_ARG_ENABLE(
	[update-databases],
	AC_HELP_STRING(
		[--enable-update-databases],
		[
		 enable (or disable) the running of update-desktop-database/update-mime-database
		 after installation @<:@default=yes@:>@
		]
	)
)

AC_ARG_VAR(
	[UPDATE_DESKTOP_DATABASE],
	[The 'update-desktop-database' program. Use it to define or override its location.]
)
AC_ARG_VAR(
	[UPDATE_MIME_DATABASE],
	[The 'update-mime-database' program. Use it to define or override its location.]
)
if test "x$enable_update_databases" != "xno" ; then
	AC_PATH_PROG([UPDATE_DESKTOP_DATABASE], [update-desktop-database])
	AC_PATH_PROG([UPDATE_MIME_DATABASE], [update-mime-database])
fi

AM_CONDITIONAL([HAVE_UPDATE_DESKTOP_DATABASE], [test -n "$UPDATE_DESKTOP_DATABASE"])
AM_CONDITIONAL([HAVE_UPDATE_MIME_DATABASE], [test -n "$UPDATE_MIME_DATABASE"])

AC_ARG_ENABLE(
	[spell-check],
	AC_HELP_STRING(
		[--enable-spell-check],
		[enable (or disable) spell checking capabilities @<:@default=yes@:>@]
	)
)

dnl ************************
dnl   Configure options w/wo
dnl ************************

AC_ARG_WITH(
	[xml-catalog],
	AC_HELP_STRING(
		[--with-xml-catalog=@<:@FILE@:>@],
		[
		 the specified file is the systems XML catalog (/etc/xml/catalog for most systems),
		 that shall be updated after installation running
		 xmlcatalog @<:@default=${sysconfdir}/xml/catalog@:>@
		]
	),
	[xmlcatalog="$withval"],
	[xmlcatalog="${sysconfdir}/xml/catalog"]
)

AC_ARG_WITH(
	[freedesktop_org-menu],
	AC_HELP_STRING(
		[--with-freedesktop_org-menu=@<:@DIR@:>@],
		[install freedesktop.org menu files in directory DIR (if supplied) or suppress installation @<:@default=${datadir}/applications@:>@]
	),
	[fdomenupath="$withval"],
	[fdomenupath="${datadir}/applications"]
)

AC_ARG_WITH(
	[freedesktop_org-mime], 
	AC_HELP_STRING(
		[--with-freedesktop_org-mime=@<:@DIR@:>@],
		[install freedesktop.org mime support files in directory DIR (if supplied) or suppress installation @<:@default=${datadir}/mime@:>@]
	),
	[fdomimepath="$withval"],
	[fdomimepath="${datadir}/mime"]
)

AC_ARG_WITH(
	[theme-path],
	AC_HELP_STRING(
		[--with-theme-path=@<:@DIR@:>@],
		[install theme icons in directory DIR (if supplied) or suppress installation @<:@default=${datadir}/icons/hicolor@:>@]
	),
	[iconthemepath="$withval"],
	[iconthemepath="${datadir}/icons/hicolor"]
)

AC_ARG_WITH(
	[icon-path],
	AC_HELP_STRING(
		[--with-icon-path=@<:@DIR@:>@],
		[install GNOME-pixmap icons in directory DIR (if supplied) or suppress installation @<:@default=${datadir}/pixmaps@:>@]
	),
	[iconpath="$withval"],
	[iconpath="${datadir}/pixmaps"]
)

AC_MSG_CHECKING([for system XML catalog])
if ! test -f "`eval echo $xmlcatalog`" ; then
	AC_MSG_RESULT([not found])
	AC_MSG_NOTICE([The given systems XML catalog $xmlcatalog seems to be unavailable.])
	AC_MSG_NOTICE([We will not try to update it after installation.])
	XML_CATALOG_FILE=""
else
	AC_MSG_RESULT([$xmlcatalog])
	XML_CATALOG_FILE="$xmlcatalog"
fi
	
AC_SUBST([XML_CATALOG_FILE])
AM_CONDITIONAL([REGISTER_XML_CATALOG], [test "x$enable_xml_catalog_update" != "xno" -a -n "$XML_CATALOG_FILE" ])
AC_SUBST([fdomenupath])
AM_CONDITIONAL(INSTALL_FDOMENU, [test "x$fdomenupath" != "xno"])
AC_SUBST([fdomimepath])
AM_CONDITIONAL(INSTALL_FDOMIME, [test "x$fdomimepath" != "xno"])
AC_SUBST([iconthemepath])
AM_CONDITIONAL(INSTALL_THEMEICONS, [test "x$iconthemepath" != "xno"])
AC_SUBST([iconpath])
AM_CONDITIONAL(INSTALL_ICONS, [test "x$iconpath" != "xno"])


dnl ************************
dnl   Standard headers
dnl ************************

AC_HEADER_STDC


dnl ************************
dnl   Libraries
dnl ************************

PKG_PROG_PKG_CONFIG

# it is unclear to me if gio requires g_thread_init(), but if it does, we need to include gthread-2.0
if test "x$native_win32" = "xyes"; then
	GIO_PC="gio-2.0"
else
	GIO_PC="gio-unix-2.0"
fi

BF_dependencies="gtk+-3.0 >= 3.2.2 gdk-3.0"

AC_ARG_WITH(
	[gtk2],
	AC_HELP_STRING(
		[--with-gtk2],
		[Use GTK2+ instead of GTK3+, even if GTK3+ is available @<:@default=no@:>@]
	)
)

dnl Be explicit here and don't accept --with-gtk2=foo
if test "x$with_gtk2" != "xyes"; then
	PKG_CHECK_EXISTS(
		[$BF_dependencies],
		[with_gtk3="yes"],
		[with_gtk3="no"]
	)
fi

dnl Now if we did not find GTK3 or if GTK2 shall be used, check for it first
dnl and fall back to GTK3 without any required minimum version.
if test "x$with_gtk2" = "xyes" -o "x$with_gtk3" = "xno"; then
	BF_dependencies="gtk+-2.0 >= 2.20 gdk-2.0"
	PKG_CHECK_EXISTS(
		[$BF_dependencies],
		[with_gtk3="no"],
		[
		 BF_dependencies="gtk+-3.0 gdk-3.0"
		 with_gtk3="yes"
		 with_gtk2="no"
		]
	)
fi

BF_dependencies="\
$BF_dependencies \
glib-2.0 >= 2.24 \
gmodule-2.0 >= 2.24 \
gobject-2.0 \
pango \
gdk-pixbuf-2.0 \
$GIO_PC >= 2.24 \
gthread-2.0 \
libxml-2.0"

if test "x$enable_spell_check" != "xno"; then
	AC_MSG_CHECKING([for optional libenchant development files])
	PKG_CHECK_EXISTS(
		[enchant],
		[
		 BF_dependencies="$BF_dependencies enchant"
		 AC_DEFINE([HAVE_LIBENCHANT], [1], [Define to 1 if you have the 'enchant' library (-lenchant).])
                 PKG_CHECK_EXISTS(
			[enchant >= 1.4],
			[AC_DEFINE([HAVE_LIBENCHANT_1_4], [1], [Define to 1 if you have the 'enchant' library (-lenchant) >= 1.4.])]
		 )
		 AC_MSG_RESULT([yes])
		],
		[
		 AC_CHECK_HEADERS(
			[enchant.h enchant/enchant.h],
			AC_CHECK_LIB(
				[enchant],
				[enchant_dict_check],
				[AC_MSG_RESULT([yes])],
				[AC_MSG_RESULT([no])]
			)
		 )
		]
	)
fi

AC_MSG_NOTICE([checking for required development files])
PKG_CHECK_MODULES([BLUEFISH_REQUIRED], [$BF_dependencies])

# from https://sourceforge.net/apps/trac/gtk-osx/wiki/Integrate
# to integrate bluefish in the OSX GUI
AC_MSG_CHECKING([for Mac OSX integration])
_gdk_tgt=`$PKG_CONFIG --variable=target gdk-2.0`
if test "x$_gdk_tgt" = "xquartz"; then
	BF_gtkmacintegration="gtk-mac-integration"
	AC_MSG_RESULT([gdk target is $_gdk_tgt])
	PKG_CHECK_MODULES(
		[BLUEFISH_GTKMAC],
		[$BF_gtkmacintegration],
		[
		 AC_DEFINE([MAC_INTEGRATION], [1], [Define to 1 for gtk Mac OSX integration.])
		]
	)
else
	AC_MSG_RESULT([no])
fi


# optionally build the charmap plugin if gucharmap is available
AC_MSG_NOTICE([checking for optional libgucharmap development files])
if test "x$with_gtk3" != "xno"; then
	PKG_CHECK_MODULES(
		[CHARMAP_PLUGIN],
		[gucharmap-2.90],
		[
			build_charmap_plugin=yes
			AC_DEFINE([HAVE_LIBGUCHARMAP_2], [1], [Define to 1 if you have the 'gucharmap-2.90' library (-lgucharmap).])
		],
		[		
	 		build_charmap_plugin=no
	 		AC_MSG_WARN([Disabling the building of the charmap plugin.])
	 		AC_MSG_WARN([Install gucharmap/gucharmap2.90 (>= 2.90) library/headers to build the plugin.])
	 	]
	)
else
	PKG_CHECK_MODULES(
		[CHARMAP_PLUGIN],
		[gucharmap-2],
		[
		 build_charmap_plugin=yes
		 AC_DEFINE([HAVE_LIBGUCHARMAP_2], [1], [Define to 1 if you have the 'gucharmap-2' library (-lgucharmap).])
		],
		[PKG_CHECK_MODULES(
			[CHARMAP_PLUGIN],
			[gucharmap >= 2.20],
			[
			 build_charmap_plugin=yes
			 AC_DEFINE([HAVE_LIBGUCHARMAP], [1], [Define to 1 if you have the 'gucharmap' library (-lgucharmap).])
			],
			[
			build_charmap_plugin=no
	 		AC_MSG_WARN([Disabling the building of the charmap plugin.])
	 		AC_MSG_WARN([Install gucharmap/gucharmap2 (>= 2.20) library/headers to build the plugin.])
	 		]
		)]
	)
fi
AM_CONDITIONAL([ENABLE_CHARMAP_PLUGIN], [test "x$build_charmap_plugin" = "xyes"])

AC_SEARCH_LIBS([log10], [m])
AC_SEARCH_LIBS([nanosleep], [posix4 rt])

AC_ARG_ENABLE(
	[python],
	AC_HELP_STRING(
		[--disable-python],
		[disable (or enable) python integration for zen-coding plugin @<:@default=yes@:>@]
	),
	[enable_python="$enableval"],
	[enable_python="yes"]
)

if test "x$enable_python" = "xyes" ; then
	AC_ARG_VAR(
		[PYTHON],
		[The 'python' binary with path. Use it to define or override the location of 'python'.]
	)
	AC_PATH_PROGS([PYTHON], [python python2.6 python2.5 python2.4 python2])
	if test -n "$PYTHON" ; then
		# The following was inspired by Andrew Collier's AC_PYTHON_MODULE macro.
		for python_module in "sys" "distutils.sysconfig" ; do
			AC_MSG_CHECKING([for python module '$python_module'])
			$PYTHON -c "import $python_module" 2>/dev/null
			if test "$?" -ne "0" ; then
				AC_MSG_RESULT([no])
				AC_MSG_NOTICE([This could mean, that the module '$python_module' is not available
or not shipped with the python runtime (like e.g. on SuSE systems,
where the necessary modules are in the python-devel package) or that
the python binary '$PYTHON' does not exist.])
				enable_python="no"
				break
			else
				AC_MSG_RESULT([yes])
			fi
		done
		if test "x$enable_python" != "xno" ; then
			AC_MSG_CHECKING([for Python version])
			PY_VER=`$PYTHON -c 'import distutils.sysconfig; print distutils.sysconfig.get_python_version();'`
			PY_LIB=`$PYTHON -c 'import distutils.sysconfig; print distutils.sysconfig.get_python_lib(standard_lib=1);'`
			PY_INC=`$PYTHON -c 'import distutils.sysconfig; print distutils.sysconfig.get_python_inc();'`
			AC_MSG_RESULT([$PY_VER])
			changequote([<<, >>])
			$PYTHON -c "import sys; map(int,sys.version[:3].split('.'))>= [2,4] or sys.exit(1)"
			changequote([, ])
			if test "$?" -ne "1" ; then
				AC_MSG_CHECKING([for Python $PY_VER compile flags])
				PY_PREFIX=`$PYTHON -c 'import sys; print sys.prefix'`
				PY_EXEC_PREFIX=`$PYTHON -c 'import sys; print sys.exec_prefix'`
				if test -f "$PY_INC/Python.h" ; then
					AC_MSG_RESULT([-I$PY_INC])
					AC_MSG_CHECKING([for Python $PY_VER link flags])
					PYSHARED=`$PYTHON -c 'import distutils.sysconfig; print distutils.sysconfig.get_config_var("LIBS")'`
					AC_MSG_RESULT([-L$PY_LIB/config -lpython$PY_VER $PYSHARED])
					LIBS="$LIBS -L$PY_LIB/config -lpython$PY_VER $PYSHARED"
					CFLAGS="$CFLAGS -I$PY_INC"
				else
					enable_python="no"
					AC_MSG_RESULT([Can't find Python.h])
				fi
			else
				AC_MSG_WARN([Python too old. Only 2.4 or above is supported.])
				enable_python="no"
			fi
		fi
	else
		enable_python="no"
	fi
fi
if test "x$enable_python" = "xyes"; then
	AC_DEFINE([HAVE_PYTHON], [1], [Define to 1 if you have python installed.])
	build_zencoding_plugin=yes
else
	build_zencoding_plugin=no
fi
AM_CONDITIONAL([ENABLE_ZENCODING_PLUGIN], [test "x$build_zencoding_plugin" = "xyes"])

dnl ************************
dnl   Header
dnl ************************

AC_CHECK_HEADERS([stdlib.h unistd.h string.h strings.h errno.h stdio.h sys/stat.h ctype.h getopt.h math.h time.h sys/types.h fcntl.h netdb.h netinet/in.h sys/socket.h arpa/inet.h dirent.h sys/select.h sys/time.h sys/ipc.h sys/msg.h])

AC_CHECK_FUNCS([msgrcv msgsnd ctime ctime_r asctime asctime_r localtime strcasestr])

# test for bind(), accept(), socket(); #593801 and #602133
AC_SEARCH_LIBS([bind], [socket ws2_32])

AC_CHECK_FUNCS_ONCE([bind socket accept])

dnl **************************
dnl   Spell checker
dnl **************************

AC_DEFINE([WITH_SPC], [1], [Don't define to 1 if you want to build without spell-checker.])
AC_FUNC_SELECT_ARGTYPES
AC_HEADER_TIME

dnl **************************
dnl   Types
dnl **************************

if test "x$native_win32" = "xno"; then
	BF_TYPE_SOCKLEN_T
fi

dnl ************************
dnl   Final Cflags/Libs
dnl ************************
CFLAGS="$CFLAGS $BLUEFISH_REQUIRED_CFLAGS $BLUEFISH_GTKMAC_CFLAGS"
LIBS="$LIBS $BLUEFISH_REQUIRED_LIBS $BLUEFISH_GTKMAC_LIBS"


dnl ************************
dnl   Locations
dnl ************************

if test "$prefix" == "NONE"; then
	AC_SUBST([prefix],[${ac_default_prefix}])
fi

if test "$exec_prefix" == "NONE"; then
	AC_SUBST([exec_prefix],[${prefix}])
fi

if test -z "$localedir"; then
	AC_SUBST([localedir],[${datadir}/locale])
fi

if test -z "$docdir"; then
	AC_SUBST(
		[docdir],
		[
		 m4_ifset(
			[AC_PACKAGE_TARNAME],
			[${datadir}'/doc/${PACKAGE_TARNAME}'],
			[${datadir}'/doc/${PACKAGE}']
		 )
		]
	)
fi

dnl *******************************
dnl   Manpage macros
dnl *******************************
BF_PROG_MAN
BF_PROG_JING
BF_PROG_XMLLINT
BF_PROG_DFVAL


dnl *******************************
dnl   Output Makefiles
dnl *******************************
AC_CONFIG_FILES([
	Makefile
	data/Makefile
	data/bflang/Makefile
	data/bflib/Makefile
	data/templates/Makefile
	desktop-data/Makefile
	images/Makefile
	man/Makefile
	po/Makefile.in
	src/Makefile
	src/pixmaps/Makefile
	src/plugin_about/Makefile
	src/plugin_about/po/Makefile.in
	src/plugin_charmap/Makefile
	src/plugin_charmap/po/Makefile.in
	src/plugin_entities/Makefile
	src/plugin_entities/po/Makefile.in
	src/plugin_htmlbar/Makefile
	src/plugin_htmlbar/pixmaps/Makefile
	src/plugin_htmlbar/po/Makefile.in
	src/plugin_htmlbar/ui/Makefile
	src/plugin_infbrowser/Makefile
	src/plugin_infbrowser/po/Makefile.in
	src/plugin_snippets/Makefile
	src/plugin_snippets/po/Makefile.in
	src/plugin_zencoding/Makefile
	src/plugin_zencoding/po/Makefile.in
	src/plugin_vcs/Makefile
	src/bluefish_rc.rc.in
	ui/Makefile
	macosx/Makefile
	win32/Makefile
	win32/Makefile.mingw
])
AC_OUTPUT

# 	src/plugin_xmltools/po/Makefile.in
# 	src/plugin_xmltools/Makefile

dnl *******************************
dnl   Feedback
dnl *******************************
#AC_MSG_NOTICE([
#-----------
#If you like this program, please let me know and send me
#a postcard and tell me what city/country you're from
#-----------])
#
