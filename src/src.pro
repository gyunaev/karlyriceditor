TEMPLATE = app
TARGET = ../bin/lrceditor
DEPENDPATH += .
INCLUDEPATH += .

# Input
HEADERS += mainwindow.h \
    wizard_newproject.h \
    project.h \
    phononhelper.h \
    playerwidget.h \
    playerbutton.h \
    editor.h \
    editortimemark.h \
    settings.h \
    viewwidget.h \
    testwindow.h \
    lyrics.h
SOURCES += mainwindow.cpp \
    main.cpp \
    wizard_newproject.cpp \
    project.cpp \
    phononhelper.cpp \
    playerwidget.cpp \
    playerbutton.cpp \
    editor.cpp \
    editortimemark.cpp \
    settings.cpp \
    viewwidget.cpp \
    testwindow.cpp \
    lyrics.cpp
RESOURCES += resources.qrc
QT += phonon
FORMS += mainwindow.ui \
    wiznewproject_lyrictype.ui \
    wiznewproject_lyrics.ui \
    wiznewproject_intro.ui \
    wiznewproject_finish.ui \
    wiznewproject_musicfile.ui \
    playerwidget.ui \
    viewwidget.ui \
    testwindow.ui
