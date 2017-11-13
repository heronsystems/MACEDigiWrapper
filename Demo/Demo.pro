TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle

QT += serialport

SOURCES += main.cpp \
$$PWD/../*.cpp

HEADERS += $$PWD/../*.h\

INCLUDEPATH += $$PWD/../
