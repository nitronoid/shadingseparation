INCLUDEPATH += \
    /usr/local/include/glm/glm \
    /usr/local/include/glm \
    /usr/local/include \
    /public/devel/2018/include \
    /public/devel/2018/include/glm \
    /public/devel/2018/tbb/include \

DEPPATH = $$PWD/dep
DEPS = $$system(ls $${DEPPATH})
!isEmpty(DEPS) {
  for(d, DEPS) {
    INCLUDEPATH += $${DEPPATH}/$${d}
    INCLUDEPATH += $${DEPPATH}/$${d}/include
  }
}

QT -= core gui
CONFIG += console c++14
CONFIG -= app_bundle gui

#Linker search paths
LIBS += -L/public/devel/2018/tbb/lib/intel64/gcc4.7
# Linker libraries
LIBS +=  -ltbb -lOpenImageIO

#DEFINES += _GLIBCXX_PARALLEL
DEFINES += GLM_ENABLE_EXPERIMENTAL GLM_FORCE_CTOR_INIT GLM_FORCE_RADIANS

AUTOTEXGEN_NAMESPACE =atg
QMAKE_CXXFLAGS += \
  -DAUTOTEXGEN_NAMESPACE=\"$${AUTOTEXGEN_NAMESPACE}\" \
  -DEND_AUTOTEXGEN_NAMESPACE="}" \
  -DBEGIN_AUTOTEXGEN_NAMESPACE=\"namespace $${AUTOTEXGEN_NAMESPACE} {\"

# Standard flags
QMAKE_CXXFLAGS += -std=c++14 -g
# Optimisation flags
QMAKE_CXXFLAGS += -Ofast -march=native -frename-registers -funroll-loops -ffast-math -fassociative-math
# Intrinsics flags
QMAKE_CXXFLAGS += -mfma -mavx2 -m64 -msse -msse2 -msse3
# Enable all warnings
QMAKE_CXXFLAGS += -Wall -Wextra -pedantic-errors
# Vectorization info
QMAKE_CXXFLAGS += -ftree-vectorize -ftree-vectorizer-verbose=5

# Enable openmp
QMAKE_CXXFLAGS += -fopenmp
QMAKE_LFLAGS += -fopenmp

#QMAKE_CXXFLAGS += -fsanitize=undefined -fsanitize=address 
#QMAKE_LFLAGS += -fsanitize=undefined -fsanitize=address
