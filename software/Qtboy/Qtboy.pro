#-------------------------------------------------
#
# Project created by QtCreator 2016-11-10T22:31:30
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Qtboy
TEMPLATE = app


SOURCES += main.cpp\
        qtboy.cpp \
    screen.cpp \
    abstractarduboy.cpp

HEADERS  += qtboy.h \
    screen.h \
    abstractarduboy.h

FORMS    += qtboy.ui

RESOURCES += \
    rsc.qrc
