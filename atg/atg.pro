include($${PWD}/../common.pri)

TEMPLATE = lib
DESTDIR = $${PWD}/lib
TARGET = atg

UI_HEADERS_DIR = ui
OBJECTS_DIR = obj

INCLUDEPATH += \
    $$PWD/include 


HEADERS += $$files(include/*.h, true)
HEADERS += $$files(include/*.inl, true)
SOURCES += $$files(src/*.cpp, true)

