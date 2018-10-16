TEMPLATE = app
TARGET = appgen

UI_HEADERS_DIR = ui
OBJECTS_DIR = obj

QT -= core gui
CONFIG += console c++14
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

HEADERS += $$files(include/*.h, true)
HEADERS += $$files(include/*.inl, true)
SOURCES += $$files(src/*.cpp, true)

#Linker search paths
LIBS += -L/public/devel/2018/tbb/lib/intel64/gcc4.7
# Linker libraries
LIBS +=  -ltbb -lOpenImageIO

DEFINES += _GLIBCXX_PARALLEL
DEFINES += GLM_ENABLE_EXPERIMENTAL GLM_FORCE_CTOR_INIT

AUTOTEXGEN_NAMESPACE =atg
QMAKE_CXXFLAGS += \
  -DAUTOTEXGEN_NAMESPACE=\"$${AUTOTEXGEN_NAMESPACE}\" \
  -DEND_AUTOTEXGEN_NAMESPACE="}" \
  -DBEGIN_AUTOTEXGEN_NAMESPACE=\"namespace $${AUTOTEXGEN_NAMESPACE} {\"

# Standard flags
QMAKE_CXXFLAGS += -std=c++14 -g
# Optimisation flags
QMAKE_CXXFLAGS += -Ofast -march=native -frename-registers -funroll-loops 
# Intrinsics flags
QMAKE_CXXFLAGS += -mfma -mavx2 -m64 -msse -msse2 -msse3
# Enable all warnings
QMAKE_CXXFLAGS += -Wall -Wextra -pedantic-errors

# Enable openmp
QMAKE_CXXFLAGS += -fopenmp 
QMAKE_LFLAGS += -fopenmp

