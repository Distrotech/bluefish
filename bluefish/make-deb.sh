#!/bin/sh
#
# Copyright (C) 2009 Alex Bodnaru <alexbodn@012.net.il
# Copyright (C) 2009 Daniel Leidert <daniel.leidert@wgdd.de>
#
# deb-make.sh
# Script to build Debian packages from the subversion source.
#
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

# TODO
# - optionally co bluefish/trunk/bluefish if not yet done
# - clean the places (maybe use TMPDIR to determine build places)
# - value of dch -D could be determined

set -e

cat << EOF

******************************************************************************
You'll need all software packages listed in the file INSTALL plus the
build-essential and devscripts packages. The script needs at least
Debian Wheezy (or newer) or Ubuntu Oneiric (or newer).

If the script fails for you, first check, if you have all necessary
packages installed.
******************************************************************************

EOF

DEB_NAME=bluefish

SVN_DIR=${SVN_DIR:-.}
SVN_URL=https://bluefish.svn.sourceforge.net/svnroot/bluefish/

make ${MAKEFLAGS} -C ${SVN_DIR} maintainer-clean || true

UPSTREAM_RELEASE=`grep AC_INIT ${SVN_DIR}/configure.ac | grep ${DEB_NAME} | cut -d, -f2 | sed -e 's/[][]//g'`
SVN_REV=`svn info ${SVN_DIR} | grep ^Revision: | cut -d' ' -f2`

SVN_UPSTREAM_VERSION=${UPSTREAM_RELEASE}~svn${SVN_REV}

DEB_SRC_DIR=${DEB_NAME}-${SVN_UPSTREAM_VERSION}
DEB_TAR=${DEB_NAME}-${SVN_UPSTREAM_VERSION}.tar.gz
DEB_TAR_ORIG=${DEB_NAME}_${SVN_UPSTREAM_VERSION}.orig.tar.gz

cd ${SVN_DIR}
 
./autogen.sh

TARDIR=`mktemp -d -p . ${DEB_SRC_DIR}.deb.XXXXXX`
cd ${TARDIR}
../configure
make dist-gzip VERSION=${SVN_UPSTREAM_VERSION}
cd ..

TEMPDIR=`mktemp -d -p . ${DEB_SRC_DIR}.deb.XXXXXX`
cd ${TEMPDIR}

mv ../${TARDIR}/${DEB_TAR} .
ln -sf ${DEB_TAR} ${DEB_TAR_ORIG}

tar xzf ${DEB_TAR_ORIG}
cd ${DEB_SRC_DIR}

svn export ${SVN_URL}/packages/debian/${DEB_NAME}/trunk/debian

dch -v ${SVN_UPSTREAM_VERSION}-0bf1 -D experimental "Automatically created Debian package by make-deb.sh $Rev$."

debuild -us -uc -b $@

cd ..

rm -rf ${DEB_SRC_DIR} ../${TARDIR}


cat << EOF

******************************************************************************
You'll find the packages in ${TEMPDIR}.
The directory is not automatically removed. You have to do this manually!
******************************************************************************

EOF

exit 0
