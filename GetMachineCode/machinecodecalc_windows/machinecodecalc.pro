#-------------------------------------------------
#
# Project created by QtCreator 2016-04-20T14:53:32
#
#-------------------------------------------------

QT       += core gui network
CONFIG += qaxcontainer

QMAKE_CFLAGS_RELEASE += -masm=intel

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = machinecodecalc
TEMPLATE = app

INCLUDEPATH += D:\boost_1_58\boost_1_58_0

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
