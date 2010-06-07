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
