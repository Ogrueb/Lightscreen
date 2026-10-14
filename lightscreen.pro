TEMPLATE = app
TARGET = lightscreen

SOURCES += main.cpp

include(src.pri)

windows {
    RC_ICONS += images/LS.ico

    # MSVC
    contains(QMAKE_CC, cl){
        QMAKE_LFLAGS_WINDOWS += /SUBSYSTEM:WINDOWS,5.01
        QMAKE_LFLAGS_WINDOWS += /MANIFESTUAC:level=\'asInvoker\'
        QMAKE_LFLAGS_CONSOLE = /SUBSYSTEM:CONSOLE,5.01
    }

    CONFIG += embed_manifest_exe
}

OTHER_FILES += TODO.txt
