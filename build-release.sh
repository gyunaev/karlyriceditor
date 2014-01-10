#!/bin/sh

RELEASEDIR="releases"

# Path to (cross-platform) mingw compiler
MINGWPATH=/usr/toolchains/windows-x86-mingw-qtsdl/bin
QMAKE=i686-pc-mingw32-qmake

FILE_VERSION="src/version.h"
RPM_ARCH="i586"
RPM_OUTDIR="$HOME/rpmbuild/RPMS/"

# Get current version
VERSION_MAJOR=`sed -n 's/^\#define\s\+APP_VERSION_MAJOR\s\+\([0-9]\+\)/\1/p' $FILE_VERSION`
VERSION_MINOR=`sed -n 's/^\#define\s\+APP_VERSION_MINOR\s\+\([0-9]\+\)/\1/p' $FILE_VERSION`
CURRENTVER="$VERSION_MAJOR.$VERSION_MINOR"

OUTDIR="$RELEASEDIR/$CURRENTVER"

if [ ! -d "$OUTDIR" ]; then
	mkdir -p "$OUTDIR" || exit 1
fi

BUILDDIR="karlyriceditor-$CURRENTVER"

if [ -d "$BUILDDIR" ]; then
	rm -rf "$BUILDDIR"
fi

svn export . "$BUILDDIR/" || exit 1

# Example package
tar zcf examples.tar.gz "$BUILDDIR/example" || exit 1
rm -rf "$BUILDDIR/example"

# Source package without examples
tar zcf "$OUTDIR/$BUILDDIR.tar.gz" $BUILDDIR || exit 1

# Making an RPM
rm -rf "$BUILDDIR/buildroot"
mkdir -p "$BUILDDIR/buildroot/usr/bin"
mkdir -p "$BUILDDIR/buildroot/usr/share/applications"
mkdir -p "$BUILDDIR/buildroot/usr/share/pixmaps"
cp packages/karlyriceditor.desktop "$BUILDDIR/buildroot/usr/share/applications"
cp packages/karlyriceditor.png "$BUILDDIR/buildroot/usr/share/pixmaps"

# Prepare a spec file
sed "s/^Version: [0-9.]\\+/Version: $CURRENTVER/" packages/rpm.spec > $BUILDDIR/rpm.spec

# Build a 64-bit version 
(cd "$BUILDDIR" && qmake -r -spec linux-g++-64 && make -j4) || exit 1
strip --strip-all "$BUILDDIR/bin/karlyriceditor" -o "$BUILDDIR/buildroot/usr/bin/karlyriceditor" || exit 1

# Build a 64-bit RPM
rpmbuild -bb --target=x86_64 --buildroot `pwd`"/$BUILDDIR/buildroot/" $BUILDDIR/rpm.spec || exit 1
mv $RPM_OUTDIR/x86_64/*.rpm "$OUTDIR/" || exit 1

# Clean up
pushd "$BUILDDIR"
make distclean

for lib in crypto avformat avcodec swscale avresample avutil SDL QtGui QtCore; do

	libpath=`find /lib /usr/lib/ -name lib$lib\* | sort -r | head -n1`
	if [ -z "$libpath" ]; then
		echo "No library $lib found"
		exit
	fi
	
	ln -s $libpath "src/lib$lib.so"
done
popd

# Build a 32-bit version 
rm -rf "$BUILDDIR/buildroot"
mkdir -p "$BUILDDIR/buildroot/usr/bin"
mkdir -p "$BUILDDIR/buildroot/usr/share/applications"
mkdir -p "$BUILDDIR/buildroot/usr/share/pixmaps"
cp packages/karlyriceditor.desktop "$BUILDDIR/buildroot/usr/share/applications"
cp packages/karlyriceditor.png "$BUILDDIR/buildroot/usr/share/pixmaps"

(cd "$BUILDDIR" && qmake -r -spec linux-g++-32 && make -j4) || exit 1
strip --strip-all "$BUILDDIR/bin/karlyriceditor" -o "$BUILDDIR/buildroot/usr/bin/karlyriceditor" || exit 1

# Build a 32-bit RPM
rpmbuild -bb --target=i586 --buildroot `pwd`"/$BUILDDIR/buildroot/" $BUILDDIR/rpm.spec || exit 1
mv $RPM_OUTDIR/i586/*.rpm "$OUTDIR/" || exit 1


rm -rf "$BUILDDIR"

# Win32 build
svn export . "$BUILDDIR/" || exit 1
export PATH=$MINGWPATH:$PATH
(cd $BUILDDIR && $QMAKE -r "CONFIG += release" && make -j4)  || exit 1

# installer
(cd $BUILDDIR/nsis && sh create_installer.sh "$CURRENTVER") || exit 1
mv $BUILDDIR/nsis/*.exe "$OUTDIR/"

