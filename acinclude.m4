dnl @synopsis BF_PROG_MAN
dnl
dnl @summary Determine if we can use the 'man' program.
dnl
dnl This is a simple macro to define the location of xsltproc (which can
dnl be overridden by the user) and special options to use.
dnl
dnl @category InstalledPackages
dnl @author Daniel Leidert <daniel.leidert@wgdd.de>
dnl @version $Date: 2007-06-04 00:17:14 $
dnl @license AllPermissive
AC_DEFUN([BF_PROG_MAN],[
AC_ARG_VAR(
	[MAN],
	[The 'man' binary with path. Use it to define or override the location of 'man'.]
)
AC_PATH_PROG([MAN], [man])
if test -z $MAN ; then
	AC_MSG_WARN(['man' was not found. We cannot check the manpages for errors. See README.]) ;
fi
AC_SUBST([MAN])
AM_CONDITIONAL([HAVE_MAN], [test -n $MAN])
]) # BF_PROG_MAN
