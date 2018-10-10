TEMPLATE = app
TARGET = appgen

UI_HEADERS_DIR = ui
OBJECTS_DIR = obj

QT += core 
CONFIG += console c++17
CONFIG -= app_bundle gui

INCLUDEPATH += \
    /usr/local/include/glm/glm \
    /usr/local/include/glm \
    /usr/local/include \
    /public/devel/2018/include \
    /public/devel/2018/include/glm \
    $$PWD/include 

DEPPATH = $$PWD/dep
DEPS = $$system(ls $${DEPPATH})
!isEmpty(DEPS) {
  for(d, DEPS) {
    INCLUDEPATH += $${DEPPATH}/$${d}
    INCLUDEPATH += $${DEPPATH}/$${d}/include
  }
}
message($${INCLUDEPATH})

HEADERS += $$files(include/*.h, true)
SOURCES += $$files(src/*.cpp, true)

LIBS += -ltbb -lstdc++fs
#DEFINES += _GLIBCXX_PARALLEL
DEFINES += GLM_ENABLE_EXPERIMENTAL GLM_FORCE_CTOR_INIT
DEFINES += DEBUG_OUTPUT_CONVERGENCE=0

QMAKE_CXXFLAGS += -std=c++17 -g
QMAKE_CXXFLAGS += -Ofast -msse -msse2 -msse3 -march=native -fopenmp -frename-registers -funroll-loops 
QMAKE_CXXFLAGS += -Wall -Wextra -pedantic-errors

QMAKE_LFLAGS += -fopenmp

