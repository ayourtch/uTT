TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += tests.cpp \
    ../uSockets/Berkeley.cpp \
    ../uSockets/Epoll.cpp \
    ../src/Broker.cpp

HEADERS += \
    ../uSockets/Berkeley.h \
    ../uSockets/Epoll.h \
    ../src/Broker.h

INCLUDEPATH += ../src ..
QMAKE_CXX += -std=c++17
