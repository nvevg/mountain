#-------------------------------------------------
#
# Project created by QtCreator 2015-05-03T20:25:18
#
#-------------------------------------------------

QT       += core gui dbus
CONFIG += c++11
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = mountain
TEMPLATE = app


SOURCES += \
    interfaces/udisksdeviceinterface.cpp \
    interfaces/udisksinterface.cpp \
    devicewatcher.cpp \
    main.cpp \
    mainwindow.cpp \
    settingsdialog.cpp

HEADERS  += \
    interfaces/udisksdeviceinterface.h \
    interfaces/udisksinterface.h \
    devicewatcher.h \
    mainwindow.h \
    settingsdialog.h

FORMS    += mainwindow.ui \
    settingsdialog.ui

RESOURCES += \
    resources.qrc
