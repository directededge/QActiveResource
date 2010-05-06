TEMPLATE = lib
TARGET = qactiveresource
QT += xml
DEPENDPATH += .
INCLUDEPATH += .
LIBS += -lcurl
CONFIG += release

HEADERS += QActiveResource.h
SOURCES += QActiveResource.cpp

headers.files = $$HEADERS
headers.path += /usr/local/include
target.path += /usr/local/lib
INSTALLS += target headers
