#-------------------------------------------------
#
# Project created by QtCreator 2017-01-03T15:47:26
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qtBoy
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    screen.cpp \
    arduboy.cpp \
    game.cpp \
    helloworld.cpp

HEADERS  += mainwindow.h \
    screen.h \
    arduboy.h \
    game.h \
    helloworld.h \
    stream.hpp

FORMS    += mainwindow.ui
