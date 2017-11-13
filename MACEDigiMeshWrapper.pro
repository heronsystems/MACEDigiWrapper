#-------------------------------------------------
#
# Project created by QtCreator 2017-11-13T11:00:45
#
#-------------------------------------------------

QT += serialport
QT       -= core gui

TARGET = MACEDigiMeshWrapper
TEMPLATE = lib

DEFINES += MACEDIGIMESHWRAPPER_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += mace_digimesh_wrapper.cpp \
    serial_link.cpp

HEADERS += mace_digimesh_wrapper.h\
        macedigimeshwrapper_global.h \
    digi_mesh_baud_rates.h \
    serial_link.h \
    serial_configuration.h \
    serial_configuration.h \
    i_link_events.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
