#!/bin/sh
#
# $Id: autogen.sh,v 1.3 2006-11-26 00:45:32 dleidert Exp $

set -e

## all initial declarations, overwrite them using e.g. 'ACLOCAL=aclocal-1.7 AUTOMAKE=automake-1.7 ./autogen.sh'
ACLOCAL=${ACLOCAL:-aclocal}
AUTOCONF=${AUTOCONF:-autoconf}
AUTOHEADER=${AUTOHEADER:-autoheader}
AUTOMAKE=${AUTOMAKE:-automake}
GETTEXTIZE=${GETTEXTIZE:-gettextize}
INTLTOOLIZE=${INTLTOOLIZE:-intltoolize}
LIBTOOLIZE=${LIBTOOLIZE:-libtoolize}

## check, if all binaries exist and fail with error 1 if not
if [ -z `which $ACLOCAL` ] ; then echo "Error. ACLOCAL=$ACLOCAL not found." >&2 && exit 1 ; fi
if [ -z `which $AUTOCONF` ] ; then echo "Error. AUTOCONF=$AUTOCONF not found." >&2 && exit 1 ; fi
if [ -z `which $AUTOHEADER` ] ; then echo "Error. AUTOHEADER=$AUTOHEADER not found." >&2 && exit 1 ; fi
if [ -z `which $AUTOMAKE` ] ; then echo "Error. AUTOMAKE=$AUTOMAKE not found." >&2 && exit 1 ; fi
if [ -z `which $GETTEXTIZE` ] ; then echo "Error. GETTEXTIZE=$GETTEXTIZE not found." >&2 && exit 1 ; fi
if [ -z `which $INTLTOOLIZE` ] ; then echo "Error. INTLTOOLIZE=$INTLTOOLIZE not found." >&2 && exit 1 ; fi
if [ -z `which $LIBTOOLIZE` ] ; then echo "Error. LIBTOOLIZE=$LIBTOOLIZE not found." >&2 && exit 1 ; fi

## find where automake is installed and get the version
AUTOMAKE_PATH=${AUTOMAKE_PATH:-`which $AUTOMAKE | sed 's|\/bin\/automake.*||'`}
AUTOMAKE_VERSION=`$AUTOMAKE --version | grep automake | awk '{print $4}' | awk -F. '{print $1"."$2}'`
ACLOCAL_VERSION=`$ACLOCAL --version | grep aclocal | awk '{print $4}' | awk -F. '{print $1"."$2}'`

## check if automake is new enough and fail with error 2 if not
if [[ $AUTOMAKE_VERSION =~ "1.4|1.5|1.6" ]] || [[ $ACLOCAL_VERSION =~ "1.4|1.5|1.6" ]] ; then
	echo "Your automake ($AUTOMAKE_VERSION) or aclocal ($ACLOCAL_VERSION) version" >&2
	echo "is not new enough. automake 1.7 and above are required." >&2
	exit 2
fi

## automake files we need to have inside our source
AUTOMAKE_FILES="
missing
mkinstalldirs
install-sh
"

## the directories which will contain the $GETTEXT_FILES
GETTEXT_PO_DIRS="
src/plugin_about/po
src/plugin_entities/po
src/plugin_htmlbar/po
"

## the gettext files we need to copy to $GETTEXT_PO_DIRS
GETTEXT_FILES="
Makefile.in.in
boldquot.sed
en@boldquot.header
en@quot.header
insert-header.sin
quot.sed
remove-potcdate.sin
Rules-quot
"

## the prefix where we expect share/gettext/po/$GETTEXT_FILES files
GETTEXT_LOCATION_LIST="
`which $GETTEXTIZE | sed 's|\/bin\/gettextize|\/share\/gettext\/po|'`
`echo $PATH | tr ':' '\n' | sed 's|bin|share|;s|$|\/gettext\/po|g'`
/usr/share/gettext/po
/usr/local/share/gettext/po
"

## use $GETTEXT_LOCATION to add a custom gettext location prefix
GETTEXT_LOCATION=${GETTEXT_LOCATION:-$GETTEXT_LOCATION_LIST}

## check if $GETTEXT_LOCATION contains the files we need and set $GETTEXT_DIR
for dir in $GETTEXT_LOCATION  ; do
	if [ -f $dir/Makefile.in.in ] ; then
		GETTEXT_DIR=$dir
		break
	fi
done

## if $GETTEXT_DIR is still undefined, fail with error 1
if [ -z $GETTEXT_DIR ] ; then
	echo "Error. GETTEXT_LOCATION=$GETTEXT_LOCATION_LIST does not exist." >&2
	exit 1
fi

## our help output - if autogen.sh was called with -h|--help or unkbown option
autogen_help() {
	echo
	echo "autogen.sh usage:"
	echo
	echo "  Produces all files necessary to build the bluefish project files."
	echo "  The files are linked by default, if you run ./autogen.sh without an option."
	echo
	echo "    -c, --copy      Copy files instead to link them."
	echo "    -h, --help      Print this message."
	echo
	echo "  You can overwrite the automatically determined location of aclocal (>= 1.7),"
	echo "  automake (>= 1.7), autoheader, autoconf, libtoolize, intltoolize and"
	echo "  gettextize using:"
	echo
	echo "    ACLOCAL=/foo/bin/aclocal-1.8 AUTOMAKE=automake-1.8 ./autogen.sh"
	echo
}

## copy $GETTEXT_FILES from $GETTEXT_DIR into $GETTEXT_PO_DIRS
prepare_gettext() {
	case "$1" in
		copy)
			command="cp"
		;;
		link)
			command="ln -s"
		;;
		*)
			echo "Error. autogen_if_missing() was called with unknown parameter $1." >&2
		;;
	esac
	
	for dir in $GETTEXT_PO_DIRS ; do
		for file in $GETTEXT_FILES ; do
			# echo "DEBUG: $command -f $GETTEXT_DIR/$file `pwd`/$dir" >&2
			$command -f $GETTEXT_DIR/$file `pwd`/$dir
		done
	done
}

## check if $AUTOMAKE_FILES were copied to our source
## link/copy them if not - necessary for e.g. gettext, which seems to always need mkinstalldirs
autogen_if_missing() {
	case "$1" in
		copy)
			command="cp"
		;;
		link)
			command="ln -s"
		;;
		*)
			echo "Error. autogen_if_missing() was called with unknown parameter $1." >&2
		;;
	esac
	
	for file in $AUTOMAKE_FILES ; do
		if [ ! -e "$file" ] ; then
			$command -f $AUTOMAKE_PATH/share/automake-$AUTOMAKE_VERSION/$file .
		fi
	done
}

## link/copy the necessary files to our source to prepare for a build
autogen() {
	case "$1" in
		copy)
			copyoption="-c"
		;;
		link)
		;;
		*)
			echo "Error. autogen() was called with unknown parameter $1." >&2
		;;
	esac
	$LIBTOOLIZE -f $copyoption
	$INTLTOOLIZE -f $copyoption
	$AUTOHEADER
	prepare_gettext $1
	$ACLOCAL
	$AUTOMAKE --gnu -a $copyoption
	autogen_if_missing $1
	$AUTOCONF
}

## the main function
case "$1" in
	-h | --help)
		autogen_help
		exit 0
	;;
	-c | --copy)
		autogen copy
	;;
	*)
		autogen link
	;;
esac

## ready to rumble
echo "Run ./configure with the appropriate options, then make and enjoy."

exit 0

