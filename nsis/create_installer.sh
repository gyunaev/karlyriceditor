#!/bin/sh

# Version
if [ -z "$1" ]; then
    echo "Usage: $0 <version>"
    exit 1
fi

# Generated binary
BINARY=../src/bin/karlyriceditor.exe
cp $BINARY karlyriceditor.exe
export NSISDIR=/home/tim/bin/nsis

INSTNAME="InstallKarLyricEditor-$1.exe"
echo "Creating $INSTNAME"

makensis installer.nsis || exit 1

# Remove stuff
rm karlyriceditor.exe
mv InstallKarLyricEditor.exe $INSTNAME
