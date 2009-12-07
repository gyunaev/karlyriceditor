#!/bin/sh

# Export the source code

FILE_VERSION="src/version.h"
RPM_ARCH="i586"
RPM_OUTDIR="/usr/src/packages/RPMS/$RPM_ARCH"

# Get current version
VERSION_MAJOR=`sed -n 's/^\#define\s\+APP_VERSION_MAJOR\s\+\([0-9]\+\)/\1/p' $FILE_VERSION`
VERSION_MINOR=`sed -n 's/^\#define\s\+APP_VERSION_MINOR\s\+\([0-9]\+\)/\1/p' $FILE_VERSION`
INSTNAME="cascade-win32-$VERSION_MAJOR.$VERSION_MINOR.zip"
CURRENTVER="$VERSION_MAJOR.$VERSION_MINOR"

BUILDDIR="karlyriceditor-$CURRENTVER"

if [ -d "$BUILDDIR" ]; then
	rm -rf "$BUILDDIR"
fi

svn export . "$BUILDDIR/" || exit 1

# Example package
tar zcf examples.tar.gz "$BUILDDIR/example" || exit 1
rm -rf "$BUILDDIR/example"

# Source package without examples
tar zcf "$BUILDDIR.tar.gz" $BUILDDIR || exit 1

# Build it 
(cd "$BUILDDIR" && qmake && make -j4) || exit 1

# Making an RPM
rm -rf "$BUILDDIR/buildroot"
mkdir -p "$BUILDDIR/buildroot/usr/bin"
mkdir -p "$BUILDDIR/buildroot/usr/share/applications"
mkdir -p "$BUILDDIR/buildroot/usr/share/pixmaps"
strip --strip-all "$BUILDDIR/bin/karlyriceditor" -o "$BUILDDIR/buildroot/usr/bin/karlyriceditor" || exit 1
cp packages/karlyriceditor.desktop "$BUILDDIR/buildroot/usr/share/applications"
cp packages/karlyriceditor.png "$BUILDDIR/buildroot/usr/share/pixmaps"

rpmbuild -bb --target=$RPM_ARCH --buildroot `pwd`"/$BUILDDIR/buildroot/" packages/rpm.spec || exit 1
mv $RPM_OUTDIR/*.rpm . || exit 1
rm -rf "$BUILDDIR"
