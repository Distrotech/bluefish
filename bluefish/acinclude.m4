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
