#!/bin/sh

if test "x$IGE_DEBUG_LAUNCHER" != x; then
    set -x
fi

if test "x$IGE_DEBUG_GDB" != x; then
    EXEC="gdb --args"
else
    EXEC=exec
fi

name=$(basename "$0")
dirn=$(dirname "$0")

bundle=$(cd "$dirn/../../" && pwd)
bundle_contents="$bundle"/Contents
bundle_res="$bundle_contents"/Resources
bundle_lib="$bundle_res"/lib
bundle_bin="$bundle_res"/bin
bundle_data="$bundle_res"/share
bundle_etc="$bundle_res"/etc

export DYLD_LIBRARY_PATH="$bundle_lib:$DYLD_LIBRARY_PATH"
export XDG_CONFIG_DIRS="$bundle_etc/xdg:$XDG_CONFIG_DIRS"
export XDG_DATA_DIRS="$bundle_data:$XDG_DATA_DIRS"
export GTK_DATA_PREFIX="$bundle_res"
export GTK_EXE_PREFIX="$bundle_res"
export GTK_PATH="$bundle_res"

export GTK2_RC_FILES="$bundle_etc/gtk-2.0/gtkrc"
export GTK_IM_MODULE_FILE="$bundle_etc/gtk-2.0/gtk.immodules"
export GDK_PIXBUF_MODULE_FILE="$bundle_etc/gtk-2.0/gdk-pixbuf.loaders"
export PANGO_RC_FILE="$bundle_etc/pango/pangorc"

# Set the locale-related variables appropriately:

# Start by trying the Collation preference, in case it's the only setting that exists.
APPLECOLLATION=`defaults read .GlobalPreferences AppleCollationOrder`
if test "${APPLECOLLATION}"; then
  COLL=`ls -d /usr/share/locale/${APPLECOLLATION}*.UTF-8 2>> /dev/null`
  if test "${COLL}"; then
    # $COLL is potentially multi-line; concatenate lines by not using quotes.
    export LANG=`echo ${COLL} | awk '{print $1}' | awk -F/ '{print $5}'`
  fi
fi
unset APPLECOLLATION COLL

# Continue by attempting to find the Locale preference.
APPLELOCALE=`defaults read .GlobalPreferences AppleLocale`
if test "${APPLELOCALE}"; then
    LOCALELANG=`echo "${APPLELOCALE}" | awk -F@ '{print $1".UTF-8"}'`
    if test -d "/usr/share/locale/${LOCALELANG}"; then
        export LANG="${LOCALELANG}"
    fi
fi
unset LOCALELANG

# If there is still no locale value, then set US English as a default.
if test -z "${LANG}"; then
    export LANG=en_US.UTF-8
fi

# The AppleLocale setting may contain a currency-related substring.
# Attempt to act on it.
# First strip the string to just the currency symbol and the language symbol
APPLECURRENCY=`echo "${APPLELOCALE}" | awk -F= '{print $2}'`
APPLELANG=`echo "${APPLELOCALE}" | awk -F_ '{print $1}'`
if test "${APPLECURRENCY}"; then
    # The user has set a currency different from that of their locale.
    # Search for a locale that uses that currency, and set LC_MONETARY accordingly.

    # First try to find an LC_MONETARY file that combines the language with the currency.
    FILES=`find /usr/share/locale/${APPLELANG}*UTF-8 -name LC_MONETARY -exec grep -H $APPLECURRENCY {} \;`
    if test -z "$FILES"; then
        # Otherwise try to find any LC_MONETARY file that uses that currency.
        FILES=`find /usr/share/locale/*UTF-8 -name LC_MONETARY -exec grep -H $APPLECURRENCY {} \;`
    fi

    if test "$FILES"; then
        # We found a locale that matches; set LC_MONETARY.
        export LC_MONETARY=`echo ${FILES} | awk -F: '{print $1}' | awk -F/ '{print $5}'`
    fi
fi
unset APPLECURRENCY APPLELANG APPLELOCALE FILES

APP=bluefish
I18NDIR="$bundle_data/locale"

# Has a language ordering been set?
# If so, set LC_MESSAGES and LANG accordingly; otherwise skip it.
# First step uses sed to clean off the quotes and commas, to change - to _, and change the names for the chinese scripts from "Hans" to CN and "Hant" to TW.
APPLELANGUAGES=`defaults read .GlobalPreferences AppleLanguages | sed -En   -e 's/\-/_/' -e 's/Hant/TW/' -e 's/Hans/CN/' -e 's/[[:space:]]*\"?([[:alnum:]_]+)\"?,?/\1/p' `
if test "$APPLELANGUAGES"; then
    # A language ordering exists.
    # Test, item per item, to see whether there is an corresponding locale.
    for L in $APPLELANGUAGES; do
	#test for exact matches:
       if test -f "$I18NDIR/${L}/LC_MESSAGES/$APP.mo"; then
	    	export LANG=$L
            break
        fi
	#This is a special case, because often the original strings are in US
	#English and there is no translation file.
	if test "x$L" == "xen_US"; then
	    export LANG=$L
	    break
	fi
	#OK, now test for just the first two letters:
        if test -f "$I18NDIR/${L:0:2}/LC_MESSAGES/$APP.mo"; then
	    export LANG=${L:0:2}
	    break
	fi
	#Same thing, but checking for any english variant.
	if test "x${L:0:2}" == "xen"; then
	    export LANG=$L
	    break
	fi;
    done  
fi
unset APPLELANGUAGES L

echo "LANG is set to $LANG"

if test -f "$bundle_lib/charset.alias"; then
    export CHARSETALIASDIR="$bundle_lib"
fi

# Extra arguments can be added in environment.sh.
EXTRA_ARGS=
if test -f "$bundle_res/environment.sh"; then
  source "$bundle_res/environment.sh"
fi

# Strip out the argument added by the OS.
if [ x`echo "x$1" | sed -e "s/^x-psn_.*//"` == x ]; then
    shift 1
fi
cd ${bundle}/Contents/MacOS
pwd
echo 'current dir'
$EXEC "$bundle_contents/MacOS/bluefish" "$@" $EXTRA_ARGS
