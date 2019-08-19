TEMPLATE = app
TARGET = ../bin/karlyriceditor
DEPENDPATH += .

!win32: {
        INCLUDEPATH = /usr/include/ffmpeg
	CONFIG += link_pkgconfig
	PKGCONFIG += libavformat libavcodec libswscale libavutil libswresample
	LIBS += -lcrypto
}

win32: {
    LIBS += -lwsock32 -ldxguid libeay32.lib avformat.lib avcodec.lib swscale.lib avutil.lib swresample.lib
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
    licensing.h \
    karaokelyricstextkar.h \
    kfn_file_parser.h \
    dialog_timeadjustment.h \
    util.h \
    videoencodingprofiles.h \
    videogeneratorthread.h \
    dialog_export_params.h
SOURCES += mainwindow.cpp \
    ffmpegvideodecoder.cpp \
    ffmpegvideoencoder.cpp \
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
    audioplayer.cpp \
    audioplayerprivate.cpp \
    ffmpeg_headers.cpp \
    licensing.cpp \
    karaokelyricstextkar.cpp \
    kfn_file_parser.cpp \
    dialog_timeadjustment.cpp \
    util.cpp \
    videoencodingprofiles.cpp \
    videogeneratorthread.cpp \
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

QT += widgets multimedia
