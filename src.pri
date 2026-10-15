# The application's sources and build settings, shared by lightscreen.pro and the tests (tests/common.pri).
# main.cpp and the executable-only settings (target, icon, manifest, linker subsystem) stay in lightscreen.pro.

DEFINES += QT_DEPRECATED_WARNINGS

HEADERS += $$PWD/dialogs/areadialog.h \
    $$PWD/dialogs/historydialog.h \
    $$PWD/dialogs/namingdialog.h \
    $$PWD/dialogs/optionsdialog.h \
    $$PWD/dialogs/previewdialog.h \
    $$PWD/dialogs/updaterdialog.h \
    $$PWD/lightscreenwindow.h \
    $$PWD/tools/os.h \
    $$PWD/tools/screenshot.h \
    $$PWD/tools/screenshotmanager.h \
    $$PWD/tools/windowpicker.h \
    $$PWD/updater/updater.h \
    $$PWD/widgets/hotkeywidget.h \
    $$PWD/tools/uploader/imageuploader.h \
    $$PWD/tools/uploader/imguruploader.h \
    $$PWD/tools/uploader/pomfuploader.h \
    $$PWD/tools/uploader/uploader.h \
    $$PWD/widgets/imguroptionswidget.h \
    $$PWD/widgets/pomfoptionswidget.h

SOURCES += $$PWD/dialogs/areadialog.cpp \
    $$PWD/dialogs/historydialog.cpp \
    $$PWD/dialogs/namingdialog.cpp \
    $$PWD/dialogs/optionsdialog.cpp \
    $$PWD/dialogs/previewdialog.cpp \
    $$PWD/dialogs/updaterdialog.cpp \
    $$PWD/lightscreenwindow.cpp \
    $$PWD/tools/os.cpp \
    $$PWD/tools/screenshot.cpp \
    $$PWD/tools/screenshotmanager.cpp \
    $$PWD/tools/windowpicker.cpp \
    $$PWD/updater/updater.cpp \
    $$PWD/widgets/hotkeywidget.cpp \
    $$PWD/tools/uploader/imageuploader.cpp \
    $$PWD/tools/uploader/imguruploader.cpp \
    $$PWD/tools/uploader/pomfuploader.cpp \
    $$PWD/tools/uploader/uploader.cpp \
    $$PWD/widgets/imguroptionswidget.cpp \
    $$PWD/widgets/pomfoptionswidget.cpp

FORMS += $$PWD/dialogs/historydialog.ui \
    $$PWD/dialogs/namingdialog.ui \
    $$PWD/dialogs/optionsdialog.ui \
    $$PWD/lightscreenwindow.ui \
    $$PWD/widgets/pomfoptionswidget.ui \
    $$PWD/widgets/imguroptionswidget.ui

RESOURCES += $$PWD/lightscreen.qrc
CODECFORSRC = UTF-8
INCLUDEPATH += $$PWD
CONFIG += c++14

QT += core gui widgets network sql multimedia concurrent

include($$PWD/tools/SingleApplication/singleapplication.pri)
include($$PWD/tools/UGlobalHotkey/uglobalhotkey.pri)

windows {
    QT += winextras

    # MinGW
    contains(QMAKE_CC, gcc){
        LIBS += libgdi32 libgcc libuser32 libole32 libshell32 libshlwapi libcomctl32 libdwmapi
        QMAKE_CXXFLAGS = -Wextra -Wall -Wpointer-arith
    }

    # MSVC
    contains(QMAKE_CC, cl){
        LIBS += gdi32.lib user32.lib ole32.lib shell32.lib shlwapi.lib comctl32.lib dwmapi.lib
        DEFINES        += _ATL_XP_TARGETING
        QMAKE_CFLAGS   += /D _USING_V110_SDK71
        QMAKE_CXXFLAGS += /D _USING_V110_SDK71
    }

    #QMAKE_CXXFLAGS_DEBUG += /analyze /W3 /wd6326
}

unix:LIBS += -lX11
unix:QT += x11extras

include($$PWD/version.pri)
