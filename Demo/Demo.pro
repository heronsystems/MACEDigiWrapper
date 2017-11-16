TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle

QT += serialport

SOURCES += main.cpp \
    ../mace_digimesh_wrapper.cpp \
    ../serial_link.cpp

HEADERS += $$PWD/../*.h\
    ../ATData/I_AT_data.h \
    ../ATData/integer.h \
    ../ATData/string.h \
    ../math_helper.h \
    ../ATData/node_discovery.h \
    ../frame-persistance/types/collect-and-timeout.h \
    ../frame-persistance/types/index.h \
    ../frame-persistance/types/shutdown-first-response.h \
    ../ATData/I_AT_data.h \
    ../ATData/index.h \
    ../ATData/integer.h \
    ../ATData/node_discovery.h \
    ../ATData/string.h \
    ../ATData/void.h \
    ../frame-persistance/behaviors/base.h \
    ../frame-persistance/behaviors/collect-and-timeout.h \
    ../frame-persistance/behaviors/index.h \
    ../frame-persistance/behaviors/shutdown-first-response.h \
    ../frame-persistance/types/collect-and-timeout.h \
    ../frame-persistance/types/index.h \
    ../frame-persistance/types/shutdown-first-response.h \
    ../callback.h \
    ../digi_mesh_baud_rates.h \
    ../i_link_events.h \
    ../mace_digimesh_wrapper.h \
    ../macedigimeshwrapper_global.h \
    ../math_helper.h \
    ../serial_configuration.h \
    ../serial_link.h \
    ../timer.h

INCLUDEPATH += $$PWD/../
