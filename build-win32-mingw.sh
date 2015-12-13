#!/bin/sh

# Path to (cross-platform) mingw compiler
MINGWPATH=/usr/toolchains/windows-x86-complete

# We cannot build statically because Webkit is not statically linkable
QMAKE=$MINGWPATH/i686-w64-mingw32.static/qt4-shared/bin/qmake


BUILDDIR="build.win32"

##################################

if [ -d "$BUILDDIR" ]; then
	rm -rf "$BUILDDIR"
fi

svn export . "$BUILDDIR/" || exit 1
cd "$BUILDDIR"

# Compile it
export PATH=$MINGWPATH/bin:$PATH
$QMAKE -r "CONFIG -= release_and_debug" "CONFIG += release"  && make -j4  || exit 1

if [ "x$1" == "x-nsis" ]; then
    (cd nsis && sh create_installer.sh) || exit 1
fi

