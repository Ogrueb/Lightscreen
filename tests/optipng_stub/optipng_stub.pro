# A stand-in for optipng, used by tst_optimize. Built as "optipng" into tst_optimize's stub/ directory.
TEMPLATE = app
TARGET = optipng
DESTDIR = $$OUT_PWD/../tst_optimize/stub

CONFIG -= qt app_bundle
CONFIG += console

SOURCES += main.cpp
