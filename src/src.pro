TEMPLATE = app
TARGET = ../bin/karlyriceditor
QT += core gui widgets concurrent
DEFINES += USE_LICENSING

win32: {
        LIBS += -lwsock32 -ldxguid -lcrypto
}

linux-g++-32: {
	LIBS += -L.
}

mac: {
    INCLUDEPATH += /Library/Frameworks/GStreamer.framework/Headers
    LIBS += -L/Library/Frameworks/GStreamer.framework/Libraries
}

unix:!mac:{
   CONFIG += link_pkgconfig
   PKGCONFIG += libzip gstreamer-1.0 gstreamer-app-1.0 openssl
} else: {
    LIBS += -lgstapp-1.0 -lgstreamer-1.0 -lglib-2.0 -lgobject-2.0 -lcrypto
}


# Input
HEADERS += mainwindow.h \
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
    export_params.h \
    licensing.h \
    karaokelyricstextkar.h \
    kfn_file_parser.h \
    dialog_timeadjustment.h \
    util.h \
    videoencodingprofiles.h \
    mediaplayer.h \
    logger.h
SOURCES += mainwindow.cpp \
    main.cpp \
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
    export_params.cpp \
    licensing.cpp \
    karaokelyricstextkar.cpp \
    kfn_file_parser.cpp \
    dialog_timeadjustment.cpp \
    util.cpp \
    videoencodingprofiles.cpp \
    mediaplayer.cpp \
    logger.cpp
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
