TEMPLATE = app
TARGET = pcaptest

CONFIG   += console qt

QT       += core
QT       -= gui

include(pcap.pri)

SOURCES += $$PWD/pcaptest.cpp
