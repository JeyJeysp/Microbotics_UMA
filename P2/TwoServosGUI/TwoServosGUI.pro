#-------------------------------------------------
#
# Project created by QtCreator 2013-04-05T19:35:15
#
#-------------------------------------------------

QT       += core gui serialport widgets

TARGET = twoservosGUI
TEMPLATE = app


SOURCES += main.cpp\
        TwoServosGUI.cpp \
    crc.c \
    serialprotocol.c \
    tiva_remotelink.cpp

HEADERS  += TwoServosGUI.h \
    crc.h \
    serialprotocol.h \
    remotelink_messages.h \
    tiva_remotelink.h

FORMS    += \
    TwoServosGUI.ui

CONFIG   += qwt analogwidgets colorwidgets #bibliotecas adicionales.

