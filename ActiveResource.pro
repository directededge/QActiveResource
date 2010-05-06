TEMPLATE = lib
TARGET = 
QT += xml
DEPENDPATH += .
INCLUDEPATH += .
LIBS += -lcurl
QMAKE_CXXFLAGS += -funroll-loops -ffast-math -O3

# Input
HEADERS += ActiveResource.h
SOURCES += ActiveResource.cpp
