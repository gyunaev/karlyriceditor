#!/bin/sh

# File to get the version from
FILE_VERSION=../src/version.h

# Generated binary
BINARY=../build.win32/src/bin/karlyriceditor.exe

# Qt libs
QTPATH=/usr/toolchains/windows-x86-mingw-qt46-phonon/i686-pc-mingw32/lib/
QTPLUGPATH=/usr/toolchains/windows-x86-mingw-qt46-phonon/i686-pc-mingw32/plugins/

# Start the mojo
QTLIBS="QtGui4.dll QtCore4.dll phonon4.dll"
QTPLUGINS="imageformats/qgif4.dll \
	imageformats/qico4.dll \
	imageformats/qjpeg4.dll \
	imageformats/qmng4.dll \
	imageformats/qsvg4.dll \
	imageformats/qtiff4.dll \
	iconengines/qsvgicon4.dll \
	codecs/qcncodecs4.dll \
	codecs/qjpcodecs4.dll \
	codecs/qkrcodecs4.dll \
	codecs/qtwcodecs4.dll \
	mediaservices/qdsengine4.dll \
	phonon_backend/phonon_ds94.dll"

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

cp $BINARY karlyriceditor.exe

export NSISDIR=/home/tim/bin/nsis

# Get current, and save the next version
VERSION_MAJOR=`sed -n 's/^\#define\s\+APP_VERSION_MAJOR\s\+\([0-9]\+\)/\1/p' $FILE_VERSION`
VERSION_MINOR=`sed -n 's/^\#define\s\+APP_VERSION_MINOR\s\+\([0-9]\+\)/\1/p' $FILE_VERSION`
VERSION="$VERSION_MAJOR.$VERSION_MINOR"

INSTNAME="InstallKarLyricEditor-$VERSION.exe"
echo "Creating $INSTNAME"

makensis installer.nsis || exit 1

# Remove stuff
rm karlyriceditor.exe
find . -type l -delete

mv InstallKarLyricEditor.exe $INSTNAME
