TEMPLATE = app
TARGET = tst_settings

include(../common.pri)

SOURCES += tst_settings.cpp
DISTFILES += defaults.golden roundtrip.golden
