TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    uSockets/Berkeley.cpp \
    uSockets/Epoll.cpp \
    Broker.cpp

HEADERS += \
    uSockets/Berkeley.h \
    uSockets/Epoll.h \
    Broker.h

QMAKE_CXX += -std=c++17
