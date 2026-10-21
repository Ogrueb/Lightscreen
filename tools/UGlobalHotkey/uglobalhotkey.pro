QT = core gui widgets

TARGET = UGlobalHotkey
TEMPLATE = lib
CONFIG += c++17

# Switch ABI to export (vs import, which is default)
DEFINES += UGLOBALHOTKEY_LIBRARY

include(uglobalhotkey-headers.pri)
include(uglobalhotkey-sources.pri)
include(uglobalhotkey-libs.pri)
