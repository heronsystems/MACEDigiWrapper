TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle

QT += serialport

SOURCES += main.cpp \
$$PWD/../*.cpp

HEADERS += $$PWD/../*.h\
    ../ATData/I_AT_data.h \
    ../ATData/integer.h \
    ../ATData/string.h \
    ../math_helper.h \
    ../ATData/node_discovery.h \
    ../frame_persistence_types.h \
    ../frame_persistence_behavior.h

INCLUDEPATH += $$PWD/../
