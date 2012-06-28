dnl @synopsis BF_DEFINE_LINGUAS
dnl
dnl @summary Define HAVE_LINGUA_$lang for every language of ALL_LINGUAS
dnl
dnl Make defines in config.h for every every language of ALL_LINGUAS. This
dnl will be used for the language support list in preferences.
dnl
dnl @author Daniel Leidert <daniel.leidert@wgdd.de>
dnl @version $Date$
dnl @license AllPermissive
AC_DEFUN([BF_DEFINE_LINGUAS],[
m4_foreach_w(
	[AC_lingua],
	[$1],
	[
	 dnl AC_MSG_NOTICE([Having lingua ]AC_lingua)dnl debugging
	 AH_TEMPLATE(AS_TR_CPP([HAVE_LINGUA_]AC_lingua), [Define to 1 for ] AC_lingua [ lingua support.])
	 AC_DEFINE(AS_TR_CPP([HAVE_LINGUA_]AC_lingua))
	]
)
]) # BF_DEFINE_LINGUAS

dnl @synopsis BF_PROG_JING
dnl
dnl @summary Determine if we can use the 'jing' program.
dnl
dnl This is a simple macro to define the location of 'jing' (which can
dnl be overridden by the user) and special options to use.
dnl
dnl @category InstalledPackages
dnl @author Daniel Leidert <daniel.leidert@wgdd.de>
dnl @version $Date$
dnl @license AllPermissive
AC_DEFUN([BF_PROG_JING],[
AC_ARG_VAR(
	[JING],
	[The 'jing' binary with path. Use it to define or override the location of 'jing'.]
)
AC_PATH_PROG([JING], [jing])
if test -z "$JING" ; then
	AC_MSG_WARN(['jing' was not found. It is better then 'xmllint' for validating RELAX NG.]) ;
fi
AC_SUBST([JING])
AC_ARG_VAR(
	[JING_FLAGS],
	[Options, which should be used along with 'jing'.]
)
AC_SUBST([JING_FLAGS])
AC_MSG_CHECKING([for optional 'jing' options to use])
AC_MSG_RESULT([$JING_FLAGS])
AM_CONDITIONAL([HAVE_JING], [test "x$JING" != "x"])
]) # BF_PROG_JING

dnl @synopsis BF_PROG_MAN
dnl
dnl @summary Determine if we can use the 'man' program.
dnl
dnl This is a simple macro to define the location of 'man' (which can
dnl be overridden by the user) and special options to use.
dnl
dnl @category InstalledPackages
dnl @author Daniel Leidert <daniel.leidert@wgdd.de>
dnl @version $Date$
dnl @license AllPermissive
AC_DEFUN([BF_PROG_MAN],[
AC_ARG_VAR(
	[MAN],
	[The 'man' binary with path. Use it to define or override the location of 'man'.]
)
AC_PATH_PROG([MAN], [man])
if test -z "$MAN" ; then
	AC_MSG_WARN(['man' was not found. We cannot check the manpages for errors. See README.]) ;
fi
AC_SUBST([MAN])
AM_CONDITIONAL([HAVE_MAN], [test "x$MAN" != "x"])
]) # BF_PROG_MAN


dnl @synopsis BF_PROG_DFVAL
dnl
dnl @summary Determine if we can use the 'desktop-file-validate' program.
dnl
dnl This is a simple macro to define the location of 'desktop-file-validate'
dnl (which can be overridden by the user) and special options to use.
dnl
dnl @category InstalledPackages
dnl @author Daniel Leidert <daniel.leidert@wgdd.de>
dnl @version $Date$
dnl @license AllPermissive
AC_DEFUN([BF_PROG_DFVAL],[
AC_ARG_VAR(
        [DESKTOP_FILE_VALIDATE],
        [The 'desktop-file-validate' binary with path. Use it to define or override the location of 'man'.]
)
AC_PATH_PROG([DESKTOP_FILE_VALIDATE], [desktop-file-validate])
if test -z "$DESKTOP_FILE_VALIDATE" ; then
        AC_MSG_WARN(['desktop-file-validate' was not found. We cannot check the manpages for errors. See README.]) ;
fi
AC_SUBST([DESKTOP_FILE_VALIDATE])
AM_CONDITIONAL([HAVE_DESKTOP_FILE_VALIDATE], [test "x$DESKTOP_FILE_VALIDATE" != "x"])
]) # BF_PROG_DFVAL


dnl @synopsis BF_PROG_XMLLINT
dnl
dnl @summary Determine if we can use the 'xmllint' program.
dnl
dnl This is a simple macro to define the location of 'xmllint' (which can
dnl be overridden by the user) and special options to use.
dnl
dnl @category InstalledPackages
dnl @author Daniel Leidert <daniel.leidert@wgdd.de>
dnl @version $Date$
dnl @license AllPermissive
AC_DEFUN([BF_PROG_XMLLINT],[
if test -z "$JING"; then
AC_ARG_VAR(
	[XMLLINT],
	[The 'xmllint' binary with path. Use it to define or override the location of 'xmllint'.]
)
AC_PATH_PROG([XMLLINT], [xmllint])
if test -z "$XMLLINT" ; then
	AC_MSG_WARN(['xmllint' was not found. Either 'jing' or 'xmllint' is necessary for validating RELAX NG.]) ;
fi
AC_SUBST([XMLLINT])
AC_ARG_VAR(
	[XMLLINT_FLAGS],
	[Options, which should be used along with 'xmllint', like e.g. '--nonet'.]
)
AC_SUBST([XMLLINT_FLAGS])
AC_MSG_CHECKING([for optional 'xmllint' options to use])
AC_MSG_RESULT([$XMLLINT_FLAGS])
fi
AM_CONDITIONAL([HAVE_XMLLINT], [test "x$XMLLINT" != "x"])
]) # BF_PROG_XMLLINT

# http://bugzilla-attachments.gnome.org/attachment.cgi?id=158735
# Bluefish uses socklen_t in an accept() call in ipc_bf2bf.c, but some
# old systems do not have a socklen_t type. This check is mostly the same
# as the one in Gnulib, with minor changes.
# https://bugzilla.gnome.org/615762
AC_DEFUN([BF_TYPE_SOCKLEN_T],[
AC_CHECK_TYPE(
	[socklen_t],
	[],
	[
	 AC_MSG_CHECKING([for socklen_t equivalent])
	 AC_CACHE_VAL(
		[bf_cv_socklen_t_equiv],
		[
		 bf_cv_socklen_t_equiv=
		 for arg2 in "struct sockaddr" void; do
			for t in int size_t "unsigned int" "long int" "unsigned long int"; do
				AC_COMPILE_IFELSE(
					[AC_LANG_PROGRAM(
[[#include <sys/types.h>
#include <sys/socket.h>

int getpeername (int, $arg2 *, $t *);]],
[[$t len;
getpeername (0, 0, &len);]]
					)],
					[bf_cv_socklen_t_equiv="$t"]
				)
				test "$bf_cv_socklen_t_equiv" != "" && break
			done
			test "$bf_cv_socklen_t_equiv" != "" && break
		done
		]
	 )
	 if test "$bf_cv_socklen_t_equiv" = ""; then
		AC_MSG_ERROR([Cannot find a type to use in place of socklen_t])
	 fi
	 AC_MSG_RESULT([$bf_cv_socklen_t_equiv])
	 AC_DEFINE_UNQUOTED([socklen_t], [$bf_cv_socklen_t_equiv], [type to use in place of socklen_t if not defined])
	],
     	[
#include <sys/types.h>
#if HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
	]
)
]) # BF_TYPE_SOCKLEN_T

