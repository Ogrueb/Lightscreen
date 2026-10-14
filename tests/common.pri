# Included by each test's .pro. Tests compile the application's sources directly (no library).
include($$PWD/../src.pri)

QT += testlib
CONFIG += testcase console
CONFIG -= app_bundle
INCLUDEPATH += $$PWD/..
