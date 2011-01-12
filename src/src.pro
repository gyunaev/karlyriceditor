TEMPLATE = app
TARGET = ../bin/karlyriceditor
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
    lyrics.h \
    version.h \
    projectsettings.h \
    recentfiles.h \
    colorbutton.h \
    dialog_selectencoding.h \
    dialog_edittimemark.h \
    gentlemessagebox.h \
    checknewversion.h \
    pianorollwidget.h \
    cdg.h \
    cdgrenderer.h \
    lyricsrenderer.h \
    cdggenerator.h \
    validator.h
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
    lyrics.cpp \
    projectsettings.cpp \
    recentfiles.cpp \
    colorbutton.cpp \
    dialog_selectencoding.cpp \
    dialog_edittimemark.cpp \
    gentlemessagebox.cpp \
    checknewversion.cpp \
    pianorollwidget.cpp \
    cdgrenderer.cpp \
    lyricsrenderer.cpp \
    cdggenerator.cpp
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
    testwindow.ui \
    dialog_about.ui \
    dialog_projectsettings.ui \
    dialog_settings.ui \
    dialog_edittimemark.ui \
    dialog_selectencoding.ui \
    gentlemessagebox.ui
win32-g++-*::LIBS += -lwsock32
