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
    $$PWD/tools/taskbar.h \
    $$PWD/tools/windowpicker.h \
    $$PWD/updater/updater.h \
    $$PWD/widgets/hotkeywidget.h \
    $$PWD/tools/uploader/imageuploader.h \
    $$PWD/tools/uploader/pomfuploader.h \
    $$PWD/tools/uploader/uploader.h \
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
    $$PWD/tools/uploader/pomfuploader.cpp \
    $$PWD/tools/uploader/uploader.cpp \
    $$PWD/widgets/pomfoptionswidget.cpp

FORMS += $$PWD/dialogs/historydialog.ui \
    $$PWD/dialogs/namingdialog.ui \
    $$PWD/dialogs/optionsdialog.ui \
    $$PWD/lightscreenwindow.ui \
    $$PWD/widgets/pomfoptionswidget.ui \

RESOURCES += $$PWD/lightscreen.qrc
CODECFORSRC = UTF-8
INCLUDEPATH += $$PWD
CONFIG += c++14

QT += core gui widgets network sql concurrent

include($$PWD/tools/SingleApplication/singleapplication.pri)
include($$PWD/tools/UGlobalHotkey/uglobalhotkey.pri)

windows {
    # Qt 5 only: the HBITMAP/HICON conversions in tools/os.cpp and tools/taskbar.cpp (Qt 6 has them in QImage).
    lessThan(QT_MAJOR_VERSION, 6): QT += winextras

    SOURCES += $$PWD/tools/taskbar.cpp

    # MSVC only; MinGW is not supported
    LIBS += gdi32.lib user32.lib ole32.lib shell32.lib shlwapi.lib comctl32.lib dwmapi.lib winmm.lib uuid.lib

    #QMAKE_CXXFLAGS_DEBUG += /analyze /W3 /wd6326
}

unix:LIBS += -lX11
unix:QT += x11extras

include($$PWD/version.pri)
