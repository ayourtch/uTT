TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    uSockets/Berkeley.cpp \
    uSockets/Epoll.cpp

HEADERS += \
    uSockets/Berkeley.h \
    uSockets/Epoll.h
