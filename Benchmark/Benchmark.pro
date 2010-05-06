TEMPLATE = app
CONFIG -= app_bundle
TARGET = benchmark
DEPENDPATH += .
INCLUDEPATH += . ..
LIBS += -lqactiveresource
QMAKE_CXXFLAGS += -funroll-loops -ffast-math -O3

# Input
SOURCES += benchmark.cpp
