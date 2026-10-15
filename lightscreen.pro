TEMPLATE = app
TARGET = lightscreen

SOURCES += main.cpp

include(src.pri)

windows {
    RC_ICONS += images/LS.ico

    # MSVC
    contains(QMAKE_CC, cl){
        QMAKE_LFLAGS_WINDOWS += /MANIFESTUAC:level=\'asInvoker\'
    }

    CONFIG += embed_manifest_exe
}
