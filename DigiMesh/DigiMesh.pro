#-------------------------------------------------
#
# Project created by QtCreator 2017-11-17T09:45:10
#
#-------------------------------------------------

QT += serialport
QT       -= gui

TARGET = DigiMesh
TEMPLATE = lib

DEFINES += DIGIMESH_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    serial_link.cpp \
    digimesh_radio.cpp

HEADERS += \
    ATData/I_AT_data.h \
    ATData/index.h \
    ATData/integer.h \
    ATData/message.h \
    ATData/node_discovery.h \
    ATData/string.h \
    ATData/void.h \
    frame-persistance/behaviors/base.h \
    frame-persistance/behaviors/collect-and-timeout.h \
    frame-persistance/behaviors/index.h \
    frame-persistance/behaviors/shutdown-first-response.h \
    frame-persistance/types/collect-and-timeout.h \
    frame-persistance/types/index.h \
    frame-persistance/types/shutdown-first-response.h \
    callback.h \
    DigiMesh_global.h \
    digimesh_radio.h \
    i_link_events.h \
    math_helper.h \
    serial_configuration.h \
    serial_link.h \
    timer.h \
    ATData/transmit_status.h

win32:CONFIG(release, debug|release):       copydata.commands   = $(MKDIR) $$PWD/../lib ; $(COPY_DIR) release/*.dll $$PWD/../lib/
else:win32:CONFIG(debug, debug|release):    copydata.commands   = $(MKDIR) $$PWD/../lib ; $(COPY_DIR) $$OUT_PWD/debug/*.dll $$PWD/../lib/
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata

INCLUDEPATH += $$PWD/../common
DEPENDPATH += $$PWD/../common
