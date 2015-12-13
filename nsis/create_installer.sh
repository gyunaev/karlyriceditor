#!/bin/sh

# File to get the version from
FILE_VERSION=../src/version.h

# Get current, and save the next version
VERSION_MAJOR=`sed -n 's/^\#define\s\+APP_VERSION_MAJOR\s\+\([0-9]\+\)/\1/p' $FILE_VERSION`
VERSION_MINOR=`sed -n 's/^\#define\s\+APP_VERSION_MINOR\s\+\"\?\([0-9a-z]\+\)\"\?/\1/p' $FILE_VERSION`
VERSION="$VERSION_MAJOR.$VERSION_MINOR"


# Qt libs
QT=/usr/toolchains/windows-x86-complete/i686-w64-mingw32.static/qt4-shared/
QTPATH=${QT}/bin
QTPLUGPATH=${QT}/plugins

# Start the mojo
QTLIBS="QtGui4.dll QtCore4.dll QtSvg4.dll"
QTPLUGINS="imageformats/qgif4.dll imageformats/qico4.dll imageformats/qjpeg4.dll imageformats/qmng4.dll imageformats/qsvg4.dll \
    imageformats/qtiff4.dll iconengines/qsvgicon4.dll"

find . -type l -delete

for lib in $QTLIBS; do
    if [ ! -f "$QTPATH/$lib" ]; then
        echo "Error: file $QTPATH/$lib not found"
        exit 1
    fi
    
    ln -s $QTPATH/$lib $lib || exit 1
    echo "Added Qt library $lib"
done

for plug in $QTPLUGINS; do
    
    if [ ! -f "$QTPLUGPATH/$plug" ]; then
        echo "Error: plugin $QTPLUGPATH/$plug not found"
        exit 1
    fi

    file=`basename $plug`

    ln -s "$QTPLUGPATH/$plug" $file || exit 1
    echo "Added Qt plugin $plug"
done


# Generated binary
BINARY=../src/bin/karlyriceditor.exe
cp $BINARY karlyriceditor.exe
export NSISDIR=/home/tim/bin/nsis

INSTNAME="InstallKarLyricEditor-$VERSION.exe"
echo "Creating $INSTNAME"

makensis installer.nsis || exit 1

# Remove stuff
rm karlyriceditor.exe
find . -type l -delete
mv InstallKarLyricEditor.exe $INSTNAME
