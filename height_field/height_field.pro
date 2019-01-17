include($${PWD}/../common.pri)

TEMPLATE = app
TARGET = height_field

UI_HEADERS_DIR = ui
OBJECTS_DIR = obj

INCLUDEPATH += \
    $$PWD/../atg/include \
    $$PWD/include 

LIBS += -L../atg/lib -latg 
QMAKE_RPATHDIR += ../atg/lib


SOURCES += $$files(src/*.cpp, true)

