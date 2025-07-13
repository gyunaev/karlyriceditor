TEMPLATE = app
TARGET = ../bin/karlyriceditor
DEPENDPATH += .

# Input
HEADERS += mainwindow.h \
    mediaplayer.h \
    wizard_newproject.h \
    project.h \
    playerwidget.h \
    playerbutton.h \
    editor.h \
    settings.h \
    viewwidget.h \
    testwindow.h \
    lyrics.h \
    version.h \
    projectsettings.h \
    recentfiles.h \
    colorbutton.h \
    dialog_selectencoding.h \
    gentlemessagebox.h \
    checknewversion.h \
    cdg.h \
    cdgrenderer.h \
    cdggenerator.h \
    validator.h \
    editorhighlighting.h \
    lyricsrenderer.h \
    textrenderer.h \
    lyricswidget.h \
    videogenerator.h \
    lyricsevents.h \
    background.h \
    karaokelyricstextkar.h \
    kfn_file_parser.h \
    dialog_timeadjustment.h \
    util.h \
    dialog_export_params.h
SOURCES += mainwindow.cpp \
    main.cpp \
    mediaplayer.cpp \
    wizard_newproject.cpp \
    project.cpp \
    playerwidget.cpp \
    playerbutton.cpp \
    editor.cpp \
    settings.cpp \
    viewwidget.cpp \
    testwindow.cpp \
    lyrics.cpp \
    projectsettings.cpp \
    recentfiles.cpp \
    colorbutton.cpp \
    dialog_selectencoding.cpp \
    gentlemessagebox.cpp \
    checknewversion.cpp \
    cdgrenderer.cpp \
    cdggenerator.cpp \
    editorhighlighting.cpp \
    lyricsrenderer.cpp \
    textrenderer.cpp \
    lyricswidget.cpp \
    videogenerator.cpp \
    lyricsevents.cpp \
    background.cpp \
    karaokelyricstextkar.cpp \
    kfn_file_parser.cpp \
    dialog_timeadjustment.cpp \
    util.cpp \
    dialog_export_params.cpp
RESOURCES += resources.qrc
FORMS += mainwindow.ui \
    wiznewproject_lyrictype.ui \
    wiznewproject_intro.ui \
    wiznewproject_finish.ui \
    wiznewproject_musicfile.ui \
    playerwidget.ui \
    viewwidget.ui \
    dialog_about.ui \
    dialog_projectsettings.ui \
    dialog_settings.ui \
    dialog_selectencoding.ui \
    gentlemessagebox.ui \
	dialog_export_params.ui \
	dialog_encodingprogress.ui \
    dialog_testwindow.ui \
    dialog_registration.ui \
    dialog_timeadjustment.ui \
    video_profile_dialog.ui

QT += core widgets network

# Handle libzip and GStreamer dependency with pkgconfig on Linux, specify exact paths on Mac
unix:!mac:{
    CONFIG += link_pkgconfig
    PKGCONFIG += gstreamer-1.0 gstreamer-app-1.0
} else: {
    INCLUDEPATH += /Library/Frameworks/GStreamer.framework/Headers
    LIBS += -L/Library/Frameworks/GStreamer.framework/Libraries -lgstapp-1.0 -lgstreamer-1.0 -lglib-2.0 -lgobject-2.0
}

LIBS += -lcrypto
# Windows builds debug/release libs in different directories while other platforms use a single dir
win32: {

    RC_ICONS += images/application.ico
    QMAKE_CXXFLAGS+=/Zi
    QMAKE_LFLAGS+= /INCREMENTAL:NO /Debug
    LIBS += crypt32.lib ws2_32.lib

    CONFIG(debug, debug|release) {
        LIBSUBDIR="debug"
    } else: {
        LIBSUBDIR="release"
    }
} else: {
    LIBSUBDIR=""
}
