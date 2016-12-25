TEMPLATE = app
TARGET = ../bin/karlyriceditor
DEPENDPATH += .
LIBS += -lcrypto
DEFINES += USE_LICENSING
CONFIG += link_pkgconfig
PKGCONFIG += libavformat libavcodec libswscale  libavresample libavutil

win32-g++-cross: {
	LIBS += -lwsock32 -ldxguid
}

linux-g++-32: {
	LIBS += -L.
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
    ffmpegvideodecoder.h \
    ffmpegvideoencoder.h \
    videogenerator.h \
    lyricsevents.h \
    background.h \
    audioplayer.h \
    ffmpeg_headers.h \
    audioplayerprivate.h \
	export_params.h \
    licensing.h \
    karaokelyricstextkar.h \
    kfn_file_parser.h \
    dialog_timeadjustment.h \
    util.h \
    videoencodingprofiles.h
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
    ffmpegvideodecoder.cpp \
    ffmpegvideoencoder.cpp \
    videogenerator.cpp \
    lyricsevents.cpp \
    background.cpp \
    audioplayer.cpp \
    audioplayerprivate.cpp \
    ffmpeg_headers.cpp \
	export_params.cpp \
    licensing.cpp \
    karaokelyricstextkar.cpp \
    kfn_file_parser.cpp \
    dialog_timeadjustment.cpp \
    util.cpp \
    videoencodingprofiles.cpp
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

QT += widgets multimedia
