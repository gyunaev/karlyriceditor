
SET BUILDARCH=x86_64
SET VCARCH=amd64
SET QTPATH=C:\Qt\6.6.2\msvc2019_64
SET EXTRALIBSPATH=C:\Users\test\Builder\extralibs

SET BUILDTYPE=release

:: Set up \Microsoft Visual Studio 2017
CALL "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" %VCARCH%

:: for rc.exe
set PATH=%PATH%;C:\Program Files (x86)\Windows Kits\8.1\bin\x64\;C:\Program Files\Git\bin\git;

set EXTRALIB=%EXTRALIBSPATH%\%BUILDARCH%\

set LIB=%LIB%;%EXTRALIB%\lib;%EXTRALIB%\lib\ffmpeg\

set INCLUDE=%INCLUDE%;%EXTRALIB%\include;%INCLUDE%;%EXTRALIB%\include\ffmpeg

:: Build and install directories
set BUILDDIR=build-%BUILDARCH%-%BUILDTYPE%
set INSTALLDIR=karlyriceditor-%BUILDARCH%-%BUILDTYPE%

:: Create the build dir
cd Builds
mkdir %BUILDDIR%
cd %BUILDDIR%

%QTPATH%\bin\qmake CONFIG+=%BUILDTYPE% .. || exit 1
nmake || exit 1

mkdir %INSTALLDIR%
copy src\bin\karlyriceditor.exe  %INSTALLDIR%\

:: SSL dlls
copy %EXTRALIB%\dll\*.dll %INSTALLDIR%\

:: FFMpeg libs
copy %EXTRALIB%\dll\ffmpeg\*.dll %INSTALLDIR%\

:: copy Qt libs
%QTPATH%\bin\windeployqt --%BUILDTYPE% --compiler-runtime %INSTALLDIR%\

:: Rename vcredist arch to vcredist
rename %INSTALLDIR%\vcredist* vcredist.exe

:: Manifest
copy ..\nsis\*.manifest %INSTALLDIR%\

:: License
copy ..\nsis\license.txt %INSTALLDIR%\

:: Create ZIP archve
"C:\Program Files\7-Zip\7z.exe" a -r karlyriceditor-%BUILDARCH%-%BUILDTYPE%.zip %INSTALLDIR%

:: Move those to the output dir
mkdir ..\output
copy karlyriceditor-%BUILDARCH%-%BUILDTYPE%.zip ..\output\
::copy InstallSpivak.exe ..\output\InstallSpivak-%BUILDARCH%.exe

echo "SUCCESS"
