TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle

QT += serialport

SOURCES += main.cpp \

HEADERS +=

INCLUDEPATH += $$PWD/../

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../DigiMesh/release/ -lDigiMesh
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../DigiMesh/debug/ -lDigiMesh
else:unix: LIBS += -L$$OUT_PWD/../DigiMesh/ -lDigiMesh

INCLUDEPATH += $$PWD/../DigiMesh
DEPENDPATH += $$PWD/../DigiMesh

INCLUDEPATH += $$PWD/../common
DEPENDPATH += $$PWD/../common
