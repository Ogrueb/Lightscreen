INCLUDEPATH += $$PWD/

linux:lessThan(QT_MAJOR_VERSION, 6): QT += x11extras

SOURCES += \
    $$PWD/ukeysequence.cpp \
    $$PWD/uglobalhotkeys.cpp


