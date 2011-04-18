#!/bin/sh

# Path to (cross-platform) mingw compiler
MINGWPATH=/usr/toolchains/windows-x86-mingw-qtsdl/bin
QMAKE=i686-pc-mingw32-qmake

BUILDDIR="build.win32"

##################################

if [ -d "$BUILDDIR" ]; then
	rm -rf "$BUILDDIR"
fi

svn export . "$BUILDDIR/" || exit 1
cd "$BUILDDIR"

# Compile it
export PATH=$MINGWPATH:$PATH
$QMAKE -r "CONFIG += release" && make -j4  || exit 1
