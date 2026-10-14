TEMPLATE = app
TARGET = tst_optimize

include(../common.pri)

# The stub optipng built by tests/optipng_stub.
DEFINES += OPTIPNG_STUB_DIR=\\\"$$OUT_PWD/stub\\\"

SOURCES += tst_optimize.cpp
